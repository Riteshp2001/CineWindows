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

#include "services/RemoteControlService.h"

#include "services/RemoteControlAssets.h"

#include "player/CineMpvItem.h"
#include "player/PlaybackController.h"
#include "utils/MediaUtils.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QRandomGenerator>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace
{
constexpr qsizetype MaximumHeaderBytes = 16 * 1024;
constexpr qsizetype MaximumBodyBytes = 64 * 1024;
constexpr int MaximumClients = 32;
constexpr int MaximumBrowseEntries = 1000;
constexpr int MaximumPairingFailures = 5;
constexpr int MaximumTrackedPairingClients = 256;
constexpr qint64 PairingFailureWindowMs = 60 * 1000;
constexpr qint64 PairingBlockDurationMs = 60 * 1000;
constexpr int RequestTimeoutMs = 10 * 1000;
}

/**
 * @brief Constructs a RemoteControlService, generates an initial pairing code,
 *        and wires up the TCP server's new-connection signal.
 * @param parent Optional QObject parent for Qt ownership.
 */
RemoteControlService::RemoteControlService(QObject* parent)
    : QObject(parent)
{
    m_authenticationClock.start();
    regeneratePairingCode();
    connect(&m_server, &QTcpServer::newConnection, this, &RemoteControlService::handleConnection);
}

CineMpvItem* RemoteControlService::player() const { return m_player; }

/** @brief Assigns the player and emits playerChanged. */
void RemoteControlService::setPlayer(CineMpvItem* player)
{
    if (m_player == player)
        return;
    m_player = player;
    Q_EMIT playerChanged();
}

PlaybackController* RemoteControlService::controller() const { return m_controller; }

/** @brief Assigns the controller and emits controllerChanged. */
void RemoteControlService::setController(PlaybackController* controller)
{
    if (m_controller == controller)
        return;
    m_controller = controller;
    Q_EMIT controllerChanged();
}

bool RemoteControlService::enabled() const { return m_server.isListening(); }

/** @brief Starts or stops the TCP server and emits enabledChanged. */
void RemoteControlService::setEnabled(bool enabled)
{
    if (enabled == m_server.isListening())
        return;
    if (enabled)
    {
        regeneratePairingCode();
        m_authenticationAttempts.clear();
        // Attempt to bind to the configured port
        if (!m_server.listen(QHostAddress::AnyIPv4, m_port))
        {
            setErrorString(m_server.errorString());
            Q_EMIT enabledChanged();
            return;
        }
        setErrorString({});
    }
    else
    {
        m_server.close();
        const QList<QTcpSocket*> clients = m_clients.values();
        for (QTcpSocket* socket : clients)
            socket->disconnectFromHost();
        m_clients.clear();
        m_authenticationAttempts.clear();
    }
    Q_EMIT enabledChanged();
}

quint16 RemoteControlService::port() const { return m_port; }

/** @brief Changes the port, restarting the server if it is already active. */
void RemoteControlService::setPort(quint16 port)
{
    if (port == 0 || m_port == port)
        return;
    const bool restart = enabled();
    if (restart)
        setEnabled(false);
    m_port = port;
    Q_EMIT portChanged();
    if (restart)
        setEnabled(true);
}

QString RemoteControlService::pairingCode() const { return m_pairingCode; }

/** @brief Builds the full remote URL with the pairing code as a query token. */
QString RemoteControlService::remoteUrl() const
{
    return enabled() ? QStringLiteral("http://%1:%2/?token=%3").arg(localAddress()).arg(m_port).arg(m_pairingCode)
                     : QString();
}

QString RemoteControlService::errorString() const { return m_errorString; }

/**
 * @brief Replaces the pairing code with a new random 6-digit value.
 * @details Also re-emits enabledChanged so the remoteUrl property updates.
 */
void RemoteControlService::regeneratePairingCode()
{
    m_pairingCode = QString::number(QRandomGenerator::system()->bounded(100000, 1000000));
    m_authenticationAttempts.clear();
    Q_EMIT pairingCodeChanged();
    if (enabled())
        Q_EMIT enabledChanged();
}

/**
 * @brief Resolves the first non-loopback IPv4 address for this machine.
 * @return An IPv4 address string, or "127.0.0.1" as fallback.
 */
QString RemoteControlService::localAddress() const
{
    for (const QHostAddress& address : QNetworkInterface::allAddresses())
    {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback())
            return address.toString();
    }
    return QStringLiteral("127.0.0.1");
}

/** @brief Accepts pending connections and wires up an incremental HTTP read buffer. */
void RemoteControlService::handleConnection()
{
    while (QTcpSocket* socket = m_server.nextPendingConnection())
    {
        if (m_clients.size() >= MaximumClients)
        {
            sendResponse(socket, 503, "text/plain", "Too many connections");
            socket->deleteLater();
            continue;
        }
        m_clients.insert(socket);
        QTimer::singleShot(RequestTimeoutMs, socket, [socket] {
            if (socket->state() != QAbstractSocket::UnconnectedState)
                socket->disconnectFromHost();
        });
        // Accumulate data until the full HTTP request (including body) is received
        connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
            QByteArray request = socket->property("requestBuffer").toByteArray();
            const QByteArray incoming = socket->readAll();
            if (incoming.size() > MaximumHeaderBytes + MaximumBodyBytes - request.size())
            {
                sendResponse(socket, 413, "text/plain", "Request too large");
                return;
            }
            request.append(incoming);
            const qsizetype headerEnd = request.indexOf("\r\n\r\n");
            if (headerEnd < 0)
            {
                if (request.size() > MaximumHeaderBytes)
                {
                    sendResponse(socket, 431, "text/plain", "Request headers too large");
                    return;
                }
                socket->setProperty("requestBuffer", request);
                return;
            }
            if (headerEnd > MaximumHeaderBytes)
            {
                sendResponse(socket, 431, "text/plain", "Request headers too large");
                return;
            }

            // Parse Content-Length to ensure the entire body has arrived
            qsizetype contentLength = 0;
            bool hasContentLength = false;
            const QList<QByteArray> headerLines = request.left(headerEnd).split('\n');
            for (const QByteArray& rawLine : headerLines)
            {
                const QByteArray line = rawLine.trimmed();
                if (line.toLower().startsWith("content-length:"))
                {
                    bool ok = false;
                    const qint64 parsedLength = line.mid(line.indexOf(':') + 1).trimmed().toLongLong(&ok);
                    if (hasContentLength || !ok || parsedLength < 0)
                    {
                        sendResponse(socket, 400, "text/plain", "Invalid Content-Length");
                        return;
                    }
                    if (parsedLength > MaximumBodyBytes)
                    {
                        sendResponse(socket, 413, "text/plain", "Request body too large");
                        return;
                    }
                    hasContentLength = true;
                    contentLength = parsedLength;
                }
            }
            if (request.size() < headerEnd + 4 + contentLength)
            {
                socket->setProperty("requestBuffer", request);
                return;
            }
            // Complete request received — clear buffer and dispatch
            socket->setProperty("requestBuffer", {});
            handleRequest(socket, request.left(headerEnd + 4 + contentLength));
        });
        connect(socket, &QTcpSocket::disconnected, this, [this, socket] {
            m_clients.remove(socket);
            socket->deleteLater();
        });
    }
}

/**
 * @brief Parses the HTTP request line and routes to the appropriate handler.
 * @details Serves static assets, the companion HTML page, API state/browse
 *          endpoints, and the command POST endpoint. All API calls (except
 *          static assets) require a valid ?token= query parameter.
 */
void RemoteControlService::handleRequest(QTcpSocket* socket, const QByteArray& requestData)
{
    const QList<QByteArray> lines = requestData.split('\n');
    if (lines.isEmpty())
        return;
    const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
    if (requestLine.size() < 2)
    {
        sendResponse(socket, 400, "text/plain", "Bad request");
        return;
    }
    const QByteArray method = requestLine.at(0);
    const QUrl url = QUrl::fromEncoded(requestLine.at(1));

    // Serve static companion assets (no auth required)
    if (method == "GET" && url.path() == QStringLiteral("/companion.js"))
    {
        const RemoteControlAsset script = loadRemoteControlAsset(QStringLiteral(":/cinewindows/remote/companion.js"));
        sendResponse(socket, script.status, "text/javascript; charset=utf-8", script.body);
        return;
    }
    if (method == "GET" && url.path() == QStringLiteral("/companion.css"))
    {
        const RemoteControlAsset stylesheet = loadRemoteControlAsset(QStringLiteral(":/cinewindows/remote/companion.css"));
        sendResponse(socket, stylesheet.status, "text/css; charset=utf-8", stylesheet.body);
        return;
    }
    if (method == "GET" && url.path() == QStringLiteral("/favicon.ico"))
    {
        sendResponse(socket, 204, "image/x-icon", {});
        return;
    }

    // All remaining endpoints require a valid pairing token
    const QString token = QUrlQuery(url).queryItemValue(QStringLiteral("token"));
    const QString clientAddress = socket->peerAddress().toString();
    const qint64 now = m_authenticationClock.elapsed();
    auto attempt = m_authenticationAttempts.find(clientAddress);
    if (attempt != m_authenticationAttempts.end() && attempt->blockedUntilMs > now)
    {
        sendResponse(socket, 429, "text/plain", "Too many pairing attempts");
        return;
    }
    if (token != m_pairingCode)
    {
        if (attempt == m_authenticationAttempts.end()
            && m_authenticationAttempts.size() >= MaximumTrackedPairingClients)
        {
            for (auto it = m_authenticationAttempts.begin(); it != m_authenticationAttempts.end();)
            {
                const bool expired = it->blockedUntilMs <= now
                    && now - it->lastFailureMs > PairingFailureWindowMs;
                if (expired)
                    it = m_authenticationAttempts.erase(it);
                else
                    ++it;
            }
            if (m_authenticationAttempts.size() >= MaximumTrackedPairingClients)
            {
                sendResponse(socket, 429, "text/plain", "Too many pairing clients");
                return;
            }
        }
        AuthenticationAttempt& failedAttempt = m_authenticationAttempts[clientAddress];
        if (now - failedAttempt.lastFailureMs > PairingFailureWindowMs)
            failedAttempt.failures = 0;
        ++failedAttempt.failures;
        failedAttempt.lastFailureMs = now;
        if (failedAttempt.failures >= MaximumPairingFailures)
            failedAttempt.blockedUntilMs = now + PairingBlockDurationMs;
        sendResponse(socket, 403, "text/plain", "Pairing code required");
        return;
    }
    if (attempt != m_authenticationAttempts.end())
        m_authenticationAttempts.erase(attempt);

    // Serve the companion web UI
    if (method == "GET" && url.path() == QStringLiteral("/"))
    {
        sendResponse(socket, 200, "text/html; charset=utf-8", companionPage());
        return;
    }

    // Return current player state as JSON
    if (method == "GET" && url.path() == QStringLiteral("/api/state"))
    {
        const QJsonObject state{{QStringLiteral("title"), m_player ? m_player->mediaTitle() : QString()},
                                {QStringLiteral("path"), m_player ? m_player->currentPath() : QString()},
                                {QStringLiteral("position"), m_player ? m_player->position() : 0.0},
                                {QStringLiteral("duration"), m_player ? m_player->duration() : 0.0},
                                {QStringLiteral("paused"), m_player ? m_player->pause() : true},
                                {QStringLiteral("idle"), m_player ? m_player->idle() : true},
                                {QStringLiteral("muted"), m_player ? m_player->muted() : false},
                                {QStringLiteral("volume"), m_player ? m_player->volume() : 0},
                                {QStringLiteral("playlistPosition"), m_player ? m_player->playlistPosition() : -1},
                                {QStringLiteral("playlistCount"), m_player ? m_player->playlistCount() : 0}};
        sendResponse(socket, 200, "application/json", QJsonDocument(state).toJson(QJsonDocument::Compact));
        return;
    }

    // Browse a local directory via JSON listing
    if (method == "GET" && url.path() == QStringLiteral("/api/browse"))
    {
        const QJsonObject result = browseDirectory(QUrlQuery(url).queryItemValue(QStringLiteral("path")));
        const int status = result.value(QStringLiteral("ok")).toBool() ? 200 : 400;
        sendResponse(socket, status, "application/json", QJsonDocument(result).toJson(QJsonDocument::Compact));
        return;
    }

    // Dispatch playback / navigation commands
    if (method == "POST" && url.path() == QStringLiteral("/api/command"))
    {
        const qsizetype bodyStart = requestData.indexOf("\r\n\r\n");
        const QJsonObject command = QJsonDocument::fromJson(requestData.mid(bodyStart + 4)).object();
        const QString action = command.value(QStringLiteral("action")).toString();
        const QString value = command.value(QStringLiteral("value")).toString().trimmed();

        // Transport controls
        if (m_controller && action == QStringLiteral("toggle"))
            m_controller->togglePause();
        else if (m_player && action == QStringLiteral("play"))
            m_player->setPause(false);
        else if (m_player && action == QStringLiteral("pause"))
            m_player->setPause(true);
        else if (m_controller && action == QStringLiteral("next"))
            m_controller->playNext();
        else if (m_controller && action == QStringLiteral("previous"))
            m_controller->playPrevious();
        else if (m_controller && action == QStringLiteral("stop"))
            m_controller->stop();
        else if (m_controller && action == QStringLiteral("mute"))
            m_controller->toggleMute();

        // Seek to an absolute or relative position
        else if (m_player && action == QStringLiteral("seek"))
            m_player->setPosition(command.value(QStringLiteral("value")).toDouble());
        else if (m_player && action == QStringLiteral("seekRelative"))
            m_player->setPosition(qBound(0.0,
                                         m_player->position() + command.value(QStringLiteral("value")).toDouble(),
                                         qMax(0.0, m_player->duration())));

        // Volume control
        else if (m_player && action == QStringLiteral("volume"))
            m_player->setVolume(qBound(0, command.value(QStringLiteral("value")).toInt(), 200));

        // Open a single media URL or local file
        else if (m_controller && action == QStringLiteral("open"))
        {
            const QUrl mediaUrl(value);
            const QFileInfo localFile(value);
            const bool isRemote = mediaUrl.isValid()
                && (mediaUrl.scheme().compare(QStringLiteral("http"), Qt::CaseInsensitive) == 0
                    || mediaUrl.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0);
            const bool isLocalMedia = localFile.exists() && localFile.isFile() && MediaUtils::isMediaFile(value);
            if (value.isEmpty() || (!isRemote && !isLocalMedia))
            {
                sendResponse(socket, 400, "application/json", "{\"ok\":false,\"error\":\"Enter a valid HTTP(S) media link or choose a media file.\"}");
                return;
            }
            m_controller->openPaths({value}, true);
        }

        // Open all media files from a folder
        else if (m_controller && action == QStringLiteral("openFolder"))
        {
            const QFileInfo folder(value);
            const QStringList media = folder.exists() && folder.isDir() ? MediaUtils::listMediaFiles(folder.absoluteFilePath())
                                                                        : QStringList{};
            if (media.isEmpty())
            {
                sendResponse(socket, 400, "application/json", "{\"ok\":false,\"error\":\"No playable media was found in this folder.\"}");
                return;
            }
            m_controller->openPaths(media, true);
        }
        else
        {
            sendResponse(socket, 400, "application/json", "{\"ok\":false,\"error\":\"Unsupported command.\"}");
            return;
        }
        sendResponse(socket, 200, "application/json", "{\"ok\":true}");
        return;
    }
    sendResponse(socket, 404, "text/plain", "Not found");
}

/**
 * @brief Writes a complete HTTP/1.1 response then closes the connection.
 * @param socket      The client socket.
 * @param status      HTTP status code (e.g. 200, 400, 404).
 * @param contentType Value for the Content-Type header.
 * @param body        Response payload (may be empty).
 */
void RemoteControlService::sendResponse(QTcpSocket* socket, int status, const QByteArray& contentType, const QByteArray& body)
{
    const QByteArray reason = status == 200 ? "OK"
        : status == 204                    ? "No Content"
        : status == 413                    ? "Payload Too Large"
        : status == 429                    ? "Too Many Requests"
        : status == 431                    ? "Request Header Fields Too Large"
        : status == 503                    ? "Service Unavailable"
        : status == 403                    ? "Forbidden"
        : status == 404                    ? "Not Found"
                                           : "Bad Request";
    socket->write("HTTP/1.1 " + QByteArray::number(status) + " " + reason + "\r\nContent-Type: " + contentType
                  + "\r\nContent-Length: " + QByteArray::number(body.size())
                  + "\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nReferrer-Policy: no-referrer"
                    "\r\nConnection: close\r\n\r\n" + body);
    socket->disconnectFromHost();
}

/** @brief Reads the embedded companion HTML page from the Qt resource system. */
QByteArray RemoteControlService::companionPage() const
{
    QFile page(QStringLiteral(":/cinewindows/remote/companion.html"));
    return page.open(QIODevice::ReadOnly) ? page.readAll() : QByteArrayLiteral("Remote UI unavailable");
}

/**
 * @brief Builds a JSON directory listing for the remote file browser.
 * @param requestedPath Absolute path to browse, or empty for drive roots.
 * @return A JSON object with ok, path, parent, and items keys.
 */
QJsonObject RemoteControlService::browseDirectory(const QString& requestedPath) const
{
    QJsonArray items;

    // Empty path returns the list of drive roots
    if (requestedPath.trimmed().isEmpty())
    {
        const QFileInfoList roots = QDir::drives();
        for (const QFileInfo& root : roots)
        {
            const QString path = QDir::cleanPath(root.absoluteFilePath());
            items.append(QJsonObject{{QStringLiteral("name"), QDir::toNativeSeparators(path)},
                                     {QStringLiteral("path"), path},
                                     {QStringLiteral("directory"), true}});
        }
        return {{QStringLiteral("ok"), true},
                {QStringLiteral("path"), QString()},
                {QStringLiteral("parent"), QString()},
                {QStringLiteral("items"), items}};
    }

    // Validate the requested directory
    const QFileInfo requested(QDir::cleanPath(requestedPath));
    if (!requested.exists() || !requested.isDir() || !requested.isReadable())
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("This folder is not available.")}};

    // Enumerate directory contents, filtering to directories and media files
    const QDir directory(requested.absoluteFilePath());
    const QFileInfoList entries = directory.entryInfoList(QDir::AllDirs | QDir::Files | QDir::Readable | QDir::NoDotAndDotDot,
                                                           QDir::DirsFirst | QDir::IgnoreCase | QDir::Name);
    bool truncated = false;
    for (const QFileInfo& entry : entries)
    {
        if (!entry.isDir() && !MediaUtils::isMediaFile(entry.absoluteFilePath()))
            continue;
        if (items.size() >= MaximumBrowseEntries)
        {
            truncated = true;
            break;
        }
        items.append(QJsonObject{{QStringLiteral("name"), entry.fileName()},
                                 {QStringLiteral("path"), entry.absoluteFilePath()},
                                 {QStringLiteral("directory"), entry.isDir()}});
    }

    // Resolve the parent directory (empty at drive root)
    const QString absolutePath = directory.absolutePath();
    const bool atRoot = QDir(absolutePath).isRoot();
    QDir parentDirectory(absolutePath);
    const QString parent = !atRoot && parentDirectory.cdUp() ? parentDirectory.absolutePath() : QString();
    return {{QStringLiteral("ok"), true},
            {QStringLiteral("path"), absolutePath},
            {QStringLiteral("parent"), parent},
            {QStringLiteral("truncated"), truncated},
            {QStringLiteral("items"), items}};
}

/**
 * @brief Sets a new error string and emits the errorStringChanged signal.
 * @param error The new error description (empty string clears the error).
 */
void RemoteControlService::setErrorString(const QString& error)
{
    if (m_errorString == error)
        return;
    m_errorString = error;
    Q_EMIT errorStringChanged();
}
