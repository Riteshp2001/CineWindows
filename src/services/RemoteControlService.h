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

#include "player/CineMpvItem.h"
#include "player/PlaybackController.h"

#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QJsonObject>
#include <QSet>
#include <QTcpServer>
#include <QtQml/qqmlregistration.h>

class QTcpSocket;

/**
 * @class RemoteControlService
 * @brief Serves a web-based remote control UI over TCP.
 * @details Hosts an HTTP server on the local network that provides a companion
 *          web page for controlling playback, browsing media, and viewing the
 *          current player state. The connection is secured with a pairing code.
 *          Exposed to QML for integrated lifecycle management.
 */
class RemoteControlService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(CineMpvItem* player READ player WRITE setPlayer NOTIFY playerChanged)
    Q_PROPERTY(PlaybackController* controller READ controller WRITE setController NOTIFY controllerChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(quint16 port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(QString pairingCode READ pairingCode NOTIFY pairingCodeChanged)
    Q_PROPERTY(QString remoteUrl READ remoteUrl NOTIFY enabledChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

public:
    /**
     * @brief Constructs a RemoteControlService with an optional parent.
     * @param parent Optional QObject parent for Qt ownership.
     */
    explicit RemoteControlService(QObject* parent = nullptr);

    /** @brief Returns the assigned player item, or nullptr. */
    CineMpvItem* player() const;

    /** @brief Assigns the player item to control. */
    void setPlayer(CineMpvItem* player);

    /** @brief Returns the assigned playback controller, or nullptr. */
    PlaybackController* controller() const;

    /** @brief Assigns the playback controller. */
    void setController(PlaybackController* controller);

    /** @brief True when the TCP server is actively listening. */
    bool enabled() const;

    /** @brief Starts or stops the TCP server. */
    void setEnabled(bool enabled);

    /** @brief Returns the configured TCP port. */
    quint16 port() const;

    /** @brief Sets the TCP port (restarts the server if already listening). */
    void setPort(quint16 port);

    /** @brief Returns the current 6-digit pairing code. */
    QString pairingCode() const;

    /** @brief Returns the full remote control URL, or empty if disabled. */
    QString remoteUrl() const;

    /** @brief Returns the last error string, or empty. */
    QString errorString() const;

    /** @brief Generates a new random 6-digit pairing code. */
    Q_INVOKABLE void regeneratePairingCode();

Q_SIGNALS:
    /** @brief Emitted when the player property changes. */
    void playerChanged();
    /** @brief Emitted when the controller property changes. */
    void controllerChanged();
    /** @brief Emitted when the server enable state changes. */
    void enabledChanged();
    /** @brief Emitted when the port property changes. */
    void portChanged();
    /** @brief Emitted when the pairing code changes. */
    void pairingCodeChanged();
    /** @brief Emitted when the error string changes. */
    void errorStringChanged();

private:
    struct AuthenticationAttempt
    {
        int failures{0};
        qint64 lastFailureMs{0};
        qint64 blockedUntilMs{0};
    };

    /** @brief Accepts incoming TCP connections and wires up the read buffers. */
    void handleConnection();

    /** @brief Parses and dispatches an HTTP request. */
    void handleRequest(QTcpSocket* socket, const QByteArray& request);

    /** @brief Writes a complete HTTP response and closes the connection. */
    void sendResponse(QTcpSocket* socket, int status, const QByteArray& contentType, const QByteArray& body);

    /** @brief Returns the embedded companion HTML page. */
    QByteArray companionPage() const;

    /** @brief Builds a JSON directory listing at the given path. */
    QJsonObject browseDirectory(const QString& requestedPath) const;

    /** @brief Resolves the first non-loopback IPv4 address for this host. */
    QString localAddress() const;

    /** @brief Sets the error string and emits errorStringChanged. */
    void setErrorString(const QString& error);

    /** @brief Player item used for state queries and commands. */
    CineMpvItem* m_player{nullptr};
    /** @brief Playback controller used for transport commands. */
    PlaybackController* m_controller{nullptr};
    /** @brief Underlying TCP server instance. */
    QTcpServer m_server;
    /** @brief Listening port (default 32322). */
    quint16 m_port{32322};
    /** @brief Six-digit pairing code for client authentication. */
    QString m_pairingCode;
    /** @brief Most recent error message. */
    QString m_errorString;
    /** @brief Monotonic clock used for pairing-attempt throttling. */
    QElapsedTimer m_authenticationClock;
    /** @brief Failed pairing attempts keyed by remote IPv4 address. */
    QHash<QString, AuthenticationAttempt> m_authenticationAttempts;
    /** @brief Active companion sockets, bounded to prevent connection exhaustion. */
    QSet<QTcpSocket*> m_clients;
};
