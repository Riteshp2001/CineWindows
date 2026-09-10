/*
 * CineWindows - Video Player
 * Copyright (c) 2026 Ritesh Pandit
 *
 * CineWindows Community License
 *
 * This source code is made available for personal, non-commercial
 * use only. Organizations may not use, copy, modify, or distribute
 * this code without written permission from Ritesh Pandit.
 *
 * See the LICENSE.md file for full license terms.
 *
 * Project: CineWindows
 * Author:  Ritesh Pandit
 * Last modified: 2026-09-10
 * Modified by: Ritesh Pandit
 */

#include "player/IpcServer.h"

#include "player/CineMpvItem.h"

#include <MpvController>

#include <QCoreApplication>
#include <QFile>
#include <QJsonObject>
#include <QMetaObject>
#include <QTextStream>
#include <QThread>
#include <QTimer>

#include <atomic>
#include <cmath>
#include <cstdio>
#include <limits>
#include <optional>
#include <utility>

namespace {

constexpr qint64 MaxIpcLineBytes = 1024 * 1024;
constexpr qint64 MaxIpcPendingWriteBytes = 1024 * 1024;
constexpr int MaxIpcClients = 32;
constexpr qsizetype MaxIpcObservationsPerClient = 256;
std::atomic<quint64> s_nextObservationId{quint64{1} << 48};

/**
 * @brief Generates a unique observation ID for dynamic property observations.
 * @details Uses an atomic counter starting at 2^48 to avoid collisions with
 *          mpv's built-in observation IDs.
 * @return A unique 64-bit observation identifier.
 */
quint64 nextObservationId()
{
    return s_nextObservationId.fetch_add(1, std::memory_order_relaxed);
}

/**
 * @brief Checks if a QVariant contains an mpv error return.
 * @param value The QVariant to inspect.
 * @return The mpv error code if the variant wraps ErrorReturn, otherwise std::nullopt.
 */
std::optional<int> variantError(const QVariant& value)
{
    if (value.metaType() == QMetaType::fromType<ErrorReturn>())
    {
        return value.value<ErrorReturn>().error;
    }
    return std::nullopt;
}

/**
 * @brief Converts an mpv error code to a human-readable string.
 * @param error The mpv error code.
 * @return Lowercase error description, or "command error" if the error string is empty.
 */
QString mpvErrorString(int error)
{
    const QString message = MpvController::getError(error).trimmed().toLower();
    return message.isEmpty() ? QStringLiteral("command error") : message;
}

/**
 * @brief Safely extracts a 64-bit integer from a QJsonValue.
 * @details Validates that the value is a finite double with no fractional part
 *          and within qint64 range.
 * @param value The QJsonValue to extract from.
 * @return The integer value if valid, otherwise std::nullopt.
 */
std::optional<qint64> jsonInteger(const QJsonValue& value)
{
    if (!value.isDouble())
    {
        return std::nullopt;
    }
    const double number = value.toDouble();
    // Reject non-finite, fractional, or out-of-range values
    if (!std::isfinite(number) || std::trunc(number) != number
        || number < static_cast<double>(std::numeric_limits<qint64>::min())
        || number > static_cast<double>(std::numeric_limits<qint64>::max()))
    {
        return std::nullopt;
    }
    return value.toInteger();
}

/**
 * @brief Checks whether a property is observed by default (built-in).
 * @details These properties are always observed by the player and do not
 *          need dynamic observation management.
 * @param property The mpv property name to check.
 * @return True if the property is in the built-in observation set.
 */
bool isBuiltInProperty(const QString& property)
{
    static const QSet<QString> properties{
        QStringLiteral("media-title"),   QStringLiteral("path"),          QStringLiteral("time-pos"),
        QStringLiteral("duration"),      QStringLiteral("pause"),         QStringLiteral("idle-active"),
        QStringLiteral("volume"),        QStringLiteral("mute"),          QStringLiteral("speed"),
        QStringLiteral("playlist-pos"),  QStringLiteral("playlist-count"), QStringLiteral("track-list"),
        QStringLiteral("chapter-list"),  QStringLiteral("sid"),           QStringLiteral("aid"),
        QStringLiteral("vid")};
    return properties.contains(property);
}

/**
 * @brief Converts a property value to a QJsonValue for IPC transmission.
 * @param value The property value to convert.
 * @param stringValue If true, the value is converted to a string.
 * @return The value as a QJsonValue (string or structured JSON).
 */
QJsonValue observationValue(const QVariant& value, bool stringValue)
{
    return stringValue ? QJsonValue(value.toString()) : IpcProtocol::variantToJson(value);
}

} // namespace

/**
 * @brief Executes a parsed IPC command against the mpv player.
 * @details Dispatches standard mpv IPC commands (get_property, set_property,
 *          get_version, get_time_us, client_name, quit) and forwards all
 *          other commands via commandBlocking. Observe/unobserve commands
 *          return an error here since they require session context.
 */
IpcCommandResult executeIpcCommand(CineMpvItem* player, const QJsonArray& command)
{
    IpcCommandResult result;
    if (!player)
    {
        result.error = QStringLiteral("uninitialized");
        return result;
    }
    if (command.isEmpty() || !command.at(0).isString())
    {
        result.error = QStringLiteral("invalid parameter");
        return result;
    }

    const QString name = command.at(0).toString();

    // Handle property read commands
    if (name == QStringLiteral("get_property") || name == QStringLiteral("get_property_string"))
    {
        if (command.size() < 2 || !command.at(1).isString() || command.at(1).toString().isEmpty())
        {
            result.error = QStringLiteral("invalid parameter");
            return result;
        }

        const QVariant value = player->getProperty(command.at(1).toString());
        if (const auto error = variantError(value))
        {
            result.error = mpvErrorString(*error);
            return result;
        }
        result.data = name == QStringLiteral("get_property_string") ? QJsonValue(value.toString())
                                                                      : IpcProtocol::variantToJson(value);
        return result;
    }

    // Handle property write commands
    if (name == QStringLiteral("set_property") || name == QStringLiteral("set_property_string"))
    {
        if (command.size() < 3 || !command.at(1).isString() || command.at(1).toString().isEmpty())
        {
            result.error = QStringLiteral("invalid parameter");
            return result;
        }
        if (name == QStringLiteral("set_property_string") && !command.at(2).isString())
        {
            result.error = QStringLiteral("invalid parameter");
            return result;
        }

        const QVariant value = name == QStringLiteral("set_property_string") ? QVariant(command.at(2).toString())
                                                                               : command.at(2).toVariant();
        const int error = player->setPropertyBlocking(command.at(1).toString(), value);
        if (error < 0)
        {
            result.error = mpvErrorString(error);
        }
        return result;
    }

    // Handle informational queries
    if (name == QStringLiteral("get_version"))
    {
        result.data = static_cast<qint64>(player->mpvClientApiVersion());
        return result;
    }
    if (name == QStringLiteral("get_time_us"))
    {
        result.data = player->mpvInternalTimeUs();
        return result;
    }
    if (name == QStringLiteral("client_name"))
    {
        result.data = QStringLiteral("cinewindows");
        return result;
    }

    // Handle quit command
    if (name == QStringLiteral("quit"))
    {
        result.quitRequested = true;
        return result;
    }

    // Observe/unobserve are handled at the session level, not here
    if (name == QStringLiteral("observe_property") || name == QStringLiteral("observe_property_string")
        || name == QStringLiteral("unobserve_property"))
    {
        result.error = QStringLiteral("invalid parameter");
        return result;
    }

    // Fall through to generic mpv command dispatch
    const QVariant commandResult = player->commandBlocking(IpcProtocol::commandArguments(command));
    if (const auto error = variantError(commandResult))
    {
        result.error = mpvErrorString(*error);
        return result;
    }
    result.data = commandResult.isValid() ? IpcProtocol::variantToJson(commandResult) : QJsonValue::Null;
    return result;
}

/**
 * @brief Constructs an IpcServer instance and creates the TCP server.
 * @details Connects the QTcpServer::newConnection signal to onNewConnection().
 */
IpcServer::IpcServer(QObject* parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &IpcServer::onNewConnection);
}

CineMpvItem* IpcServer::player() const
{
    return m_player;
}

/**
 * @brief Sets the mpv player instance and connects IPC-relevant signals.
 * @details Disconnects the previous player (if any) and releases all dynamic
 *          observations before reassigning. Connects mpvPropertyChanged,
 *          fileStarted, fileLoaded, and endFile signals for event broadcasting.
 *          Re-acquires any observations that existing sessions may need.
 */
void IpcServer::setPlayer(CineMpvItem* player)
{
    if (m_player == player)
    {
        return;
    }

    // Clean up previous player's dynamic observations
    if (m_player)
    {
        for (const DynamicObservation& observation : std::as_const(m_dynamicObservations))
        {
            m_player->unobserveProperty(observation.mpvId);
        }
        disconnect(m_player, nullptr, this, nullptr);
    }
    m_dynamicObservations.clear();
    m_player = player;
    if (!m_player)
    {
        return;
    }

    // Connect player signals for property observation and event broadcasting
    connect(m_player, &CineMpvItem::mpvPropertyChanged, this, &IpcServer::onPlayerPropertyChanged);
    connect(m_player, &CineMpvItem::fileStarted, this, [this]() {
        broadcastEvent(QStringLiteral("start-file"));
    });
    connect(m_player, &CineMpvItem::fileLoaded, this, [this]() {
        broadcastEvent(QStringLiteral("file-loaded"));
    });
    connect(m_player, &CineMpvItem::endFile, this, [this](const QString& reason) {
        broadcastEvent(QStringLiteral("end-file"), {{QStringLiteral("reason"), reason}});
    });

    // Re-acquire observations for existing sessions
    for (const IpcSessionState& session : std::as_const(m_sessions))
    {
        for (const IpcObservation& observation : session.observations())
        {
            acquirePropertyObservation(observation.property);
        }
    }
}

quint16 IpcServer::port() const
{
    return m_port;
}

/**
 * @brief Sets the TCP listen port.
 * @details Only applies when the server is not currently listening.
 */
void IpcServer::setPort(quint16 port)
{
    if (!m_server->isListening())
    {
        m_port = port;
    }
}

/**
 * @brief Starts the TCP server on localhost at the configured port.
 * @return True if already listening or successfully started; false if player is null or port is 0.
 */
bool IpcServer::start()
{
    if (!m_player || m_port == 0)
    {
        return false;
    }
    return m_server->isListening() || m_server->listen(QHostAddress::LocalHost, m_port);
}

/**
 * @brief Stops the server and disconnects all clients.
 * @details Releases all session observations, disconnects each client socket,
 *          clears session state, and closes the TCP server.
 */
void IpcServer::stop()
{
    const QList<QTcpSocket*> clients = m_clients.values();
    for (QTcpSocket* socket : clients)
    {
        releaseSessionObservations(socket);
        socket->disconnect(this);
        socket->disconnectFromHost();
    }
    m_clients.clear();
    m_sessions.clear();
    m_server->close();
}

/**
 * @brief Accepts new TCP client connections.
 * @details For each pending connection, creates a session state entry and
 *          connects readyRead and disconnected signals.
 */
void IpcServer::onNewConnection()
{
    while (QTcpSocket* socket = m_server->nextPendingConnection())
    {
        if (m_clients.size() >= MaxIpcClients)
        {
            socket->disconnectFromHost();
            socket->deleteLater();
            continue;
        }
        m_clients.insert(socket);
        m_sessions.insert(socket, IpcSessionState{});
        connect(socket, &QTcpSocket::readyRead, this, &IpcServer::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &IpcServer::onDisconnected);
    }
}

/**
 * @brief Reads and processes incoming data from a client.
 * @details Reads complete lines from the socket buffer, enforces a maximum
 *          line size limit, and delegates each line to processLine(). If
 *          the accumulated data exceeds the limit without a newline, the
 *          client is disconnected.
 */
void IpcServer::onReadyRead()
{
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
    {
        return;
    }

    // Process all complete lines in the buffer
    while (socket->canReadLine())
    {
        const QByteArray line = socket->readLine();
        if (line.size() > MaxIpcLineBytes)
        {
            sendResponse(socket, 0, QStringLiteral("invalid parameter"));
            socket->disconnectFromHost();
            return;
        }
        processLine(socket, line);
    }

    // Disconnect if the buffered data exceeds the limit (no newline found)
    if (socket->bytesAvailable() > MaxIpcLineBytes)
    {
        sendResponse(socket, 0, QStringLiteral("invalid parameter"));
        socket->disconnectFromHost();
    }
}

/**
 * @brief Cleans up session state when a client disconnects.
 */
void IpcServer::onDisconnected()
{
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
    {
        return;
    }
    releaseSessionObservations(socket);
    m_clients.remove(socket);
    socket->deleteLater();
}

/**
 * @brief Parses a raw line and dispatches it to handleCommand().
 * @details Silently ignores empty/comment lines and sends error responses
 *          for malformed input.
 */
void IpcServer::processLine(QTcpSocket* socket, const QByteArray& line)
{
    const IpcParsedLine parsed = IpcProtocol::parseLine(line);
    if (parsed.kind == IpcParsedLine::Kind::Ignored)
    {
        return;
    }
    if (parsed.kind == IpcParsedLine::Kind::Error)
    {
        sendResponse(socket, parsed.requestId, parsed.error);
        return;
    }
    handleCommand(socket, parsed);
}

/**
 * @brief Dispatches a parsed command to the appropriate handler.
 * @details Routes observe_property/unobserve_property to their specific
 *          handlers and all other commands to executeIpcCommand(). If a
 *          quit command is detected, schedules application shutdown.
 */
void IpcServer::handleCommand(QTcpSocket* socket, const IpcParsedLine& parsed)
{
    const QString name = parsed.command.at(0).toString();
    if (name == QStringLiteral("observe_property") || name == QStringLiteral("observe_property_string"))
    {
        handleObserve(socket, parsed, name == QStringLiteral("observe_property_string"));
        return;
    }
    if (name == QStringLiteral("unobserve_property"))
    {
        handleUnobserve(socket, parsed);
        return;
    }

    // Execute the command and send the response
    const IpcCommandResult result = executeIpcCommand(m_player, parsed.command);
    sendResponse(socket, parsed.requestId, result.error, result.data);
    if (result.quitRequested)
    {
        QTimer::singleShot(0, qApp, &QCoreApplication::quit);
    }
}

/**
 * @brief Registers a property observation for a client.
 * @details Validates the command parameters, checks that the property exists,
 *          sets or replaces the observation in the session state, and sends
 *          an initial property change notification with the current value.
 */
void IpcServer::handleObserve(QTcpSocket* socket, const IpcParsedLine& parsed, bool stringValue)
{
    if (parsed.command.size() < 3 || !parsed.command.at(2).isString()
        || parsed.command.at(2).toString().isEmpty())
    {
        sendResponse(socket, parsed.requestId, QStringLiteral("invalid parameter"));
        return;
    }
    const auto id = jsonInteger(parsed.command.at(1));
    if (!id)
    {
        sendResponse(socket, parsed.requestId, QStringLiteral("invalid parameter"));
        return;
    }

    const QString property = parsed.command.at(2).toString();
    IpcSessionState& session = m_sessions[socket];
    if (!session.hasObservation(*id) && session.observationCount() >= MaxIpcObservationsPerClient)
    {
        sendResponse(socket, parsed.requestId, QStringLiteral("too many observations"));
        return;
    }

    // Verify the property exists by attempting to read it
    const QVariant currentValue = m_player->getProperty(property);
    if (const auto error = variantError(currentValue))
    {
        sendResponse(socket, parsed.requestId, mpvErrorString(*error));
        return;
    }

    // Update session state and acquire the observation
    if (const auto replaced = session.setObservation(*id, property, stringValue))
    {
        releasePropertyObservation(replaced->property);
    }
    acquirePropertyObservation(property);

    // Confirm and send the initial value
    sendResponse(socket, parsed.requestId, QStringLiteral("success"));
    sendPropertyChange(socket, IpcObservation{*id, property, stringValue}, currentValue);
}

/**
 * @brief Unregisters a property observation for a client.
 */
void IpcServer::handleUnobserve(QTcpSocket* socket, const IpcParsedLine& parsed)
{
    if (parsed.command.size() < 2)
    {
        sendResponse(socket, parsed.requestId, QStringLiteral("invalid parameter"));
        return;
    }
    const auto id = jsonInteger(parsed.command.at(1));
    if (!id)
    {
        sendResponse(socket, parsed.requestId, QStringLiteral("invalid parameter"));
        return;
    }

    // Remove the observation and release the property if no longer needed
    if (const auto removed = m_sessions[socket].removeObservation(*id))
    {
        releasePropertyObservation(removed->property);
    }
    sendResponse(socket, parsed.requestId, QStringLiteral("success"));
}

/**
 * @brief Sends a JSON-RPC response to a specific client.
 * @details Only writes if the socket is still connected.
 */
void IpcServer::sendResponse(QTcpSocket* socket, const QJsonValue& requestId, const QString& error,
                             const QJsonValue& data)
{
    writeToClient(socket, IpcProtocol::response(requestId, error, data));
}

bool IpcServer::writeToClient(QTcpSocket* socket, const QByteArray& data)
{
    if (!socket || socket->state() == QAbstractSocket::UnconnectedState)
        return false;
    if (data.size() > MaxIpcPendingWriteBytes
        || socket->bytesToWrite() > MaxIpcPendingWriteBytes - data.size())
    {
        socket->disconnectFromHost();
        return false;
    }
    const qint64 written = socket->write(data);
    if (written != data.size())
    {
        socket->disconnectFromHost();
        return false;
    }
    return true;
}

/**
 * @brief Sends a property-change event to a specific client.
 */
void IpcServer::sendPropertyChange(QTcpSocket* socket, const IpcObservation& observation,
                                   const QVariant& value)
{
    const QJsonObject fields{{QStringLiteral("id"), observation.id},
                             {QStringLiteral("name"), observation.property},
                             {QStringLiteral("data"), observationValue(value, observation.stringValue)}};
    writeToClient(socket, IpcProtocol::event(QStringLiteral("property-change"), fields));
}

/**
 * @brief Broadcasts an event to all connected clients.
 * @details Serializes the event once and writes the same data to every client.
 */
void IpcServer::broadcastEvent(const QString& eventName, const QJsonObject& fields)
{
    const QByteArray data = IpcProtocol::event(eventName, fields);
    for (QTcpSocket* socket : std::as_const(m_clients))
    {
        writeToClient(socket, data);
    }
}

/**
 * @brief Forwards mpv property change notifications to subscribed clients.
 * @details Iterates all sessions and sends a property-change event to every
 *          client that is observing the changed property.
 */
void IpcServer::onPlayerPropertyChanged(const QString& property, const QVariant& value)
{
    for (auto it = m_sessions.cbegin(); it != m_sessions.cend(); ++it)
    {
        for (const IpcObservation& observation : it.value().observationsForProperty(property))
        {
            sendPropertyChange(it.key(), observation, value);
        }
    }
}

/**
 * @brief Registers a dynamic mpv property observation with reference counting.
 * @details Only observes properties not already covered by built-in observations.
 *          Uses a reference count so the property is only observed once even
 *          when multiple clients subscribe to it.
 */
void IpcServer::acquirePropertyObservation(const QString& property)
{
    if (!m_player || isBuiltInObservedProperty(property))
    {
        return;
    }

    auto it = m_dynamicObservations.find(property);
    if (it == m_dynamicObservations.end())
    {
        // First reference: register the observation with mpv
        const quint64 id = nextObservationId();
        m_player->observeProperty(property, MPV_FORMAT_NODE, id);
        it = m_dynamicObservations.insert(property, DynamicObservation{id, 0});
    }
    ++it->references;
}

/**
 * @brief Releases a dynamic mpv property observation with reference counting.
 * @details Decrements the reference count and unregisters the observation
 *          from mpv when no clients need it.
 */
void IpcServer::releasePropertyObservation(const QString& property)
{
    if (!m_player || isBuiltInObservedProperty(property))
    {
        return;
    }

    auto it = m_dynamicObservations.find(property);
    if (it == m_dynamicObservations.end())
    {
        return;
    }
    --it->references;
    if (it->references <= 0)
    {
        // Last reference released: unregister from mpv
        m_player->unobserveProperty(it->mpvId);
        m_dynamicObservations.erase(it);
    }
}

/**
 * @brief Releases all observations held by a disconnecting client.
 */
void IpcServer::releaseSessionObservations(QTcpSocket* socket)
{
    const auto it = m_sessions.constFind(socket);
    if (it == m_sessions.cend())
    {
        return;
    }
    for (const IpcObservation& observation : it.value().observations())
    {
        releasePropertyObservation(observation.property);
    }
    m_sessions.remove(socket);
}

bool IpcServer::isBuiltInObservedProperty(const QString& property)
{
    return isBuiltInProperty(property);
}

/**
 * @brief Constructs a ConsoleReader without starting the stdin thread.
 */
ConsoleReader::ConsoleReader(QObject* parent)
    : QObject(parent)
{}

/**
 * @brief Destructor that stops the stdin thread and releases the player.
 */
ConsoleReader::~ConsoleReader()
{
    if (m_thread)
    {
        m_thread->requestInterruption();
    }
    setPlayer(nullptr);
}

CineMpvItem* ConsoleReader::player() const
{
    return m_player;
}

/**
 * @brief Sets the mpv player and connects stdin-relevant signals.
 * @details Disconnects the previous player and releases all dynamic observations.
 *          Connects to mpvPropertyChanged and file events for stdout output.
 */
void ConsoleReader::setPlayer(CineMpvItem* player)
{
    if (m_player == player)
    {
        return;
    }

    // Clean up previous player's dynamic observations
    if (m_player)
    {
        for (const DynamicObservation& observation : std::as_const(m_dynamicObservations))
        {
            m_player->unobserveProperty(observation.mpvId);
        }
        disconnect(m_player, nullptr, this, nullptr);
    }
    m_dynamicObservations.clear();
    m_player = player;
    if (!m_player)
    {
        return;
    }

    // Connect player signals for property observation and event output
    connect(m_player, &CineMpvItem::mpvPropertyChanged, this, &ConsoleReader::onPlayerPropertyChanged);
    connect(m_player, &CineMpvItem::fileStarted, this, [this]() {
        writeOutput(IpcProtocol::event(QStringLiteral("start-file")));
    });
    connect(m_player, &CineMpvItem::fileLoaded, this, [this]() {
        writeOutput(IpcProtocol::event(QStringLiteral("file-loaded")));
    });
    connect(m_player, &CineMpvItem::endFile, this, [this](const QString& reason) {
        writeOutput(IpcProtocol::event(QStringLiteral("end-file"), {{QStringLiteral("reason"), reason}}));
    });

    // Re-acquire observations from session state
    for (const IpcObservation& observation : m_session.observations())
    {
        acquirePropertyObservation(observation.property);
    }
}

/**
 * @brief Starts reading stdin on a background thread.
 * @details Creates a QThread that reads lines from stdin and dispatches them
 *          to the main thread via Qt::QueuedConnection for safe player access.
 * @return True if the thread started, false if player is null or thread already exists.
 */
bool ConsoleReader::start()
{
    if (!m_player || m_thread)
    {
        return m_player && m_thread;
    }

    const QPointer<ConsoleReader> guard(this);
    QThread* thread = QThread::create([guard]() {
        QFile input;
        if (!input.open(stdin, QIODevice::ReadOnly | QIODevice::Text))
        {
            return;
        }
        QTextStream stream(&input);
        while (!QThread::currentThread()->isInterruptionRequested())
        {
            const QString line = stream.readLine();
            if (line.isNull() || !guard)
            {
                break;
            }
            // Marshal back to the main thread for player-safe dispatching
            QMetaObject::invokeMethod(
                guard.data(),
                [guard, line]() {
                    if (guard)
                    {
                        guard->onStdinLine(line);
                    }
                },
                Qt::QueuedConnection);
        }
    });
    m_thread = thread;
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
    return true;
}

/**
 * @brief Processes a single line read from stdin.
 */
void ConsoleReader::onStdinLine(const QString& line)
{
    const IpcParsedLine parsed = IpcProtocol::parseLine(line.toUtf8());
    if (parsed.kind == IpcParsedLine::Kind::Ignored)
    {
        return;
    }
    if (parsed.kind == IpcParsedLine::Kind::Error)
    {
        writeOutput(IpcProtocol::response(parsed.requestId, parsed.error));
        return;
    }
    handleCommand(parsed);
}

/**
 * @brief Dispatches a parsed command from stdin.
 * @details Routes observe/unobserve commands to their handlers and all other
 *          commands to executeIpcCommand(). Handles quit by scheduling shutdown.
 */
void ConsoleReader::handleCommand(const IpcParsedLine& parsed)
{
    const QString name = parsed.command.at(0).toString();
    if (name == QStringLiteral("observe_property") || name == QStringLiteral("observe_property_string"))
    {
        handleObserve(parsed, name == QStringLiteral("observe_property_string"));
        return;
    }
    if (name == QStringLiteral("unobserve_property"))
    {
        handleUnobserve(parsed);
        return;
    }

    const IpcCommandResult result = executeIpcCommand(m_player, parsed.command);
    writeOutput(IpcProtocol::response(parsed.requestId, result.error, result.data));
    if (result.quitRequested)
    {
        QTimer::singleShot(0, qApp, &QCoreApplication::quit);
    }
}

/**
 * @brief Registers a property observation from stdin.
 */
void ConsoleReader::handleObserve(const IpcParsedLine& parsed, bool stringValue)
{
    if (parsed.command.size() < 3 || !parsed.command.at(2).isString()
        || parsed.command.at(2).toString().isEmpty())
    {
        writeOutput(IpcProtocol::response(parsed.requestId, QStringLiteral("invalid parameter")));
        return;
    }
    const auto id = jsonInteger(parsed.command.at(1));
    if (!id)
    {
        writeOutput(IpcProtocol::response(parsed.requestId, QStringLiteral("invalid parameter")));
        return;
    }

    const QString property = parsed.command.at(2).toString();
    const QVariant currentValue = m_player->getProperty(property);
    if (const auto error = variantError(currentValue))
    {
        writeOutput(IpcProtocol::response(parsed.requestId, mpvErrorString(*error)));
        return;
    }

    if (const auto replaced = m_session.setObservation(*id, property, stringValue))
    {
        releasePropertyObservation(replaced->property);
    }
    acquirePropertyObservation(property);
    writeOutput(IpcProtocol::response(parsed.requestId, QStringLiteral("success")));
    sendPropertyChange(IpcObservation{*id, property, stringValue}, currentValue);
}

/**
 * @brief Unregisters a property observation from stdin.
 */
void ConsoleReader::handleUnobserve(const IpcParsedLine& parsed)
{
    if (parsed.command.size() < 2)
    {
        writeOutput(IpcProtocol::response(parsed.requestId, QStringLiteral("invalid parameter")));
        return;
    }
    const auto id = jsonInteger(parsed.command.at(1));
    if (!id)
    {
        writeOutput(IpcProtocol::response(parsed.requestId, QStringLiteral("invalid parameter")));
        return;
    }
    if (const auto removed = m_session.removeObservation(*id))
    {
        releasePropertyObservation(removed->property);
    }
    writeOutput(IpcProtocol::response(parsed.requestId, QStringLiteral("success")));
}

/**
 * @brief Writes raw data to stdout.
 */
void ConsoleReader::writeOutput(const QByteArray& data)
{
    if (data.isEmpty())
    {
        return;
    }
    std::fwrite(data.constData(), 1, static_cast<size_t>(data.size()), stdout);
    std::fflush(stdout);
}

/**
 * @brief Sends a property-change event to stdout.
 */
void ConsoleReader::sendPropertyChange(const IpcObservation& observation, const QVariant& value)
{
    const QJsonObject fields{{QStringLiteral("id"), observation.id},
                             {QStringLiteral("name"), observation.property},
                             {QStringLiteral("data"), observationValue(value, observation.stringValue)}};
    writeOutput(IpcProtocol::event(QStringLiteral("property-change"), fields));
}

/**
 * @brief Forwards mpv property changes to observed stdout output.
 */
void ConsoleReader::onPlayerPropertyChanged(const QString& property, const QVariant& value)
{
    for (const IpcObservation& observation : m_session.observationsForProperty(property))
    {
        sendPropertyChange(observation, value);
    }
}

/**
 * @brief Registers a dynamic property observation with reference counting (ConsoleReader).
 */
void ConsoleReader::acquirePropertyObservation(const QString& property)
{
    if (!m_player || isBuiltInProperty(property))
    {
        return;
    }
    auto it = m_dynamicObservations.find(property);
    if (it == m_dynamicObservations.end())
    {
        const quint64 id = nextObservationId();
        m_player->observeProperty(property, MPV_FORMAT_NODE, id);
        it = m_dynamicObservations.insert(property, DynamicObservation{id, 0});
    }
    ++it->references;
}

/**
 * @brief Releases a dynamic property observation with reference counting (ConsoleReader).
 */
void ConsoleReader::releasePropertyObservation(const QString& property)
{
    if (!m_player || isBuiltInProperty(property))
    {
        return;
    }
    auto it = m_dynamicObservations.find(property);
    if (it == m_dynamicObservations.end())
    {
        return;
    }
    --it->references;
    if (it->references <= 0)
    {
        m_player->unobserveProperty(it->mpvId);
        m_dynamicObservations.erase(it);
    }
}
