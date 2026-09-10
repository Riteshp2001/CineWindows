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

#pragma once

#include "player/IpcProtocol.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonValue>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QTcpServer>
#include <QTcpSocket>

class CineMpvItem;
class QThread;

/**
 * @brief Result of executing an IPC command against the player.
 * @details Contains the error string, optional response data, and a flag
 *          indicating whether the command requested a shutdown ("quit").
 * @usecase Populated by executeIpcCommand() and serialized to JSON response.
 */
struct IpcCommandResult
{
    QString error{QStringLiteral("success")};   ///< Error string ("success" on success)
    QJsonValue data{QJsonValue::Undefined};      ///< Optional response data payload
    bool quitRequested{false};                   ///< True if "quit" command was issued
};

/**
 * @brief Executes a parsed JSON-RPC command against the mpv player.
 * @param player The active CineMpvItem instance to dispatch commands to.
 * @param command JSON array where first element is command name, rest are args.
 * @return IpcCommandResult with error/data/quit status.
 * @usecase Called by both IpcServer and ConsoleReader for command dispatch.
 */
IpcCommandResult executeIpcCommand(CineMpvItem* player, const QJsonArray& command);

/**
 * @class IpcServer
 * @brief TCP JSON-RPC server implementing the mpv IPC protocol.
 * @details Listens on 127.0.0.1:32321 (configurable) for newline-delimited
 *          JSON-RPC commands. Supports property observation per client session,
 *          dynamic property subscriptions, event broadcasting, and full mpv
 *          command passthrough. Each connected client gets its own session
 *          state tracking observed properties.
 * @usecase External tools (browser extensions, scripts, mpvipc clients) can
 *          connect to control playback, query properties, and receive events.
 * @thread Main thread only (QTcpServer/QTcpSocket are async).
 */
class IpcServer : public QObject
{
    Q_OBJECT
    /// The active mpv player instance to control.
    Q_PROPERTY(CineMpvItem* player READ player WRITE setPlayer)
    /// TCP port to listen on (default: 32321).
    Q_PROPERTY(quint16 port READ port WRITE setPort)

public:
    /**
     * @brief Constructs an IpcServer without starting it.
     * @param parent Qt parent object.
     * @usecase Created after CineMpvItem is available.
     */
    explicit IpcServer(QObject* parent = nullptr);

    /**
     * @brief Returns the bound player instance.
     * @return Pointer to the CineMpvItem, or nullptr if none is set.
     */
    CineMpvItem* player() const;

    /**
     * @brief Sets the player instance and connects property observation signals.
     * @param player Pointer to the CineMpvItem to assign.
     */
    void setPlayer(CineMpvItem* player);

    /**
     * @brief Returns the configured listen port.
     * @return The TCP port number (default: 32321).
     */
    quint16 port() const;

    /**
     * @brief Sets the listen port. Must be called before start().
     * @param port The TCP port to listen on (localhost only).
     */
    void setPort(quint16 port);

    /**
     * @brief Starts the TCP server and begins accepting connections.
     * @returns True on success, false if port is in use or player is null.
     * @usecase Called once during application startup after player is set.
     */
    Q_INVOKABLE bool start();

    /**
     * @brief Stops the server and disconnects all clients.
     * @usecase Called during application shutdown.
     */
    Q_INVOKABLE void stop();

private:
    /**
     * @brief Tracks a dynamically observed mpv property.
     * @details Maps an mpv observation ID to a reference count.
     *          A property is observed once and reference-counted when
     *          multiple clients subscribe to the same property.
     */
    struct DynamicObservation
    {
        quint64 mpvId{0};    ///< mpv observation ID from observeProperty()
        int references{0};   ///< Number of clients observing this property
    };

    /** @brief Accepts a new TCP client connection. */
    void onNewConnection();
    /** @brief Reads and processes incoming data from a client. */
    void onReadyRead();
    /** @brief Cleans up session state when a client disconnects. */
    void onDisconnected();
    /** @brief Parses a single line and dispatches to handleCommand/handleObserve. */
    void processLine(QTcpSocket* socket, const QByteArray& line);
    /** @brief Dispatches a parsed command to the player. */
    void handleCommand(QTcpSocket* socket, const IpcParsedLine& parsed);
    /** @brief Registers a property observation for a client. */
    void handleObserve(QTcpSocket* socket, const IpcParsedLine& parsed, bool stringValue);
    /** @brief Unregisters a property observation for a client. */
    void handleUnobserve(QTcpSocket* socket, const IpcParsedLine& parsed);
    /** @brief Sends a JSON-RPC response to a client. */
    void sendResponse(QTcpSocket* socket, const QJsonValue& requestId, const QString& error,
                      const QJsonValue& data = QJsonValue(QJsonValue::Undefined));
    /** @brief Queues bounded output or disconnects a client that is not reading. */
    bool writeToClient(QTcpSocket* socket, const QByteArray& data);
    /** @brief Sends a property-change event to a specific client. */
    void sendPropertyChange(QTcpSocket* socket, const IpcObservation& observation, const QVariant& value);
    /** @brief Broadcasts an event (e.g. file-loaded, end-file) to all clients. */
    void broadcastEvent(const QString& eventName, const QJsonObject& fields = {});
    /** @brief Forwards mpv property changes to subscribed clients. */
    void onPlayerPropertyChanged(const QString& property, const QVariant& value);
    /** @brief Registers a dynamic mpv property observation if not already active. */
    void acquirePropertyObservation(const QString& property);
    /** @brief Releases a dynamic mpv property observation when no clients need it. */
    void releasePropertyObservation(const QString& property);
    /** @brief Releases all observations held by a disconnecting client. */
    void releaseSessionObservations(QTcpSocket* socket);
    /** @brief Returns true if the property is already observed by the player by default. */
    static bool isBuiltInObservedProperty(const QString& property);

    CineMpvItem* m_player{nullptr};                    ///< Bound player instance
    QTcpServer* m_server;                               ///< TCP server for incoming connections
    quint16 m_port{32321};                              ///< Listen port (localhost only)
    QSet<QTcpSocket*> m_clients;                        ///< Set of currently connected clients
    QHash<QTcpSocket*, IpcSessionState> m_sessions;     ///< Per-client observation sessions
    QHash<QString, DynamicObservation> m_dynamicObservations; ///< Ref-counted dynamic property observations
};

/**
 * @class ConsoleReader
 * @brief Stdin-based CLI reader for interactive mpv command input.
 * @details Reads newline-delimited JSON-RPC commands from stdin on a
 *          background thread and dispatches them to the player. Supports
 *          the same property observation and command protocol as IpcServer.
 *          Created when the application is launched with the --cli flag.
 * @usecase Interactive CLI mode for power users and debugging.
 * @thread Stdin reading runs on a QThread; all player interaction is
 *         marshaled to the main thread via queued connections.
 */
class ConsoleReader : public QObject
{
    Q_OBJECT
    /// The active mpv player instance to control.
    Q_PROPERTY(CineMpvItem* player READ player WRITE setPlayer)

public:
    /**
     * @brief Constructs a ConsoleReader without starting the stdin thread.
     * @param parent Qt parent object.
     */
    explicit ConsoleReader(QObject* parent = nullptr);

    /** @brief Stops the stdin reading thread and cleans up. */
    ~ConsoleReader() override;

    /**
     * @brief Returns the bound player instance.
     * @return Pointer to the CineMpvItem, or nullptr if none is set.
     */
    CineMpvItem* player() const;

    /**
     * @brief Sets the player instance and connects property observation signals.
     * @param player Pointer to the CineMpvItem to assign.
     */
    void setPlayer(CineMpvItem* player);

    /**
     * @brief Starts reading stdin on a background thread.
     * @returns True if the thread started successfully, false if player is null.
     * @usecase Called during --cli mode startup.
     */
    Q_INVOKABLE bool start();

private:
    /**
     * @brief Tracks a dynamically observed mpv property (same as IpcServer).
     */
    struct DynamicObservation
    {
        quint64 mpvId{0};    ///< mpv observation ID
        int references{0};   ///< Reference count across observations
    };

    /** @brief Processes a single line read from stdin. */
    void onStdinLine(const QString& line);
    /** @brief Dispatches a parsed command from stdin. */
    void handleCommand(const IpcParsedLine& parsed);
    /** @brief Registers a property observation. */
    void handleObserve(const IpcParsedLine& parsed, bool stringValue);
    /** @brief Unregisters a property observation. */
    void handleUnobserve(const IpcParsedLine& parsed);
    /** @brief Writes a JSON-RPC response to stdout. */
    void writeOutput(const QByteArray& data);
    /** @brief Sends a property-change event to stdout. */
    void sendPropertyChange(const IpcObservation& observation, const QVariant& value);
    /** @brief Forwards mpv property changes to observed stdout output. */
    void onPlayerPropertyChanged(const QString& property, const QVariant& value);
    /** @brief Registers a dynamic mpv property observation. */
    void acquirePropertyObservation(const QString& property);
    /** @brief Releases a dynamic mpv property observation. */
    void releasePropertyObservation(const QString& property);

    CineMpvItem* m_player{nullptr};                    ///< Bound player instance
    QPointer<QThread> m_thread;                         ///< Background stdin reading thread
    IpcSessionState m_session;                          ///< Stdin session observation state
    QHash<QString, DynamicObservation> m_dynamicObservations; ///< Ref-counted dynamic observations
};
