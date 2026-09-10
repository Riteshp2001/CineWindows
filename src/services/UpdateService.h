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

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QObject>
#include <QPointer>
#include <QSaveFile>
#include <QtQml/qqmlregistration.h>

#include <memory>

class QNetworkReply;

/**
 * @class UpdateService
 * @brief Checks for newer application releases from a remote update endpoint.
 * @details Queries a configured update server for the latest available version
 *          and compares it against the current running version. Exposes the
 *          update state (busy, available, version string, status message) as
 *          QML-bound properties. Uses QNetworkAccessManager for HTTP requests.
 */
class UpdateService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateAvailableChanged)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY updateAvailableChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString releaseUrl READ releaseUrl NOTIFY updateAvailableChanged)
    Q_PROPERTY(QString releaseNotes READ releaseNotes NOTIFY updateAvailableChanged)
    Q_PROPERTY(qint64 downloadedBytes READ downloadedBytes NOTIFY downloadProgressChanged)
    Q_PROPERTY(qint64 totalBytes READ totalBytes NOTIFY downloadProgressChanged)
    Q_PROPERTY(double downloadProgress READ downloadProgress NOTIFY downloadProgressChanged)
    Q_PROPERTY(QString downloadedFilePath READ downloadedFilePath NOTIFY stateChanged)

public:
    enum State
    {
        Idle,
        Checking,
        Available,
        Downloading,
        ReadyToInstall,
        Error
    };
    Q_ENUM(State)

    /**
     * @brief Constructs an UpdateService with an optional parent.
     * @param parent Optional QObject parent for Qt ownership.
     */
    explicit UpdateService(QObject* parent = nullptr);

    /**
     * @brief Returns whether an update check is currently in progress.
     * @return True if a network request is pending.
     */
    bool busy() const;

    /**
     * @brief Returns whether a newer version is available.
     * @return True when the latest version exceeds the current version.
     */
    bool updateAvailable() const;

    /**
     * @brief Returns the latest available version string.
     * @return Version string (e.g. "2.1.0"), or empty if not yet fetched.
     */
    QString latestVersion() const;

    /**
     * @brief Returns a human-readable status message about the last check.
     * @return Status text (e.g. "Up to date", "Update available", or error).
     */
    QString statusMessage() const;

    /**
     * @brief Returns the current update service state.
     * @return Current State enum value (Idle, Checking, Available, etc.).
     */
    State state() const;

    /**
     * @brief Returns the URL of the release page on GitHub.
     * @return Release page URL string, or empty if not yet fetched.
     */
    QString releaseUrl() const;

    /**
     * @brief Returns the release notes body for the latest release.
     * @return Release notes text, or empty if not yet fetched.
     */
    QString releaseNotes() const;

    /**
     * @brief Returns the number of bytes downloaded so far.
     * @return Downloaded byte count.
     */
    qint64 downloadedBytes() const;

    /**
     * @brief Returns the total size of the download in bytes.
     * @return Total byte count, or 0 if unknown.
     */
    qint64 totalBytes() const;

    /**
     * @brief Returns the download progress as a fraction from 0.0 to 1.0.
     * @return Progress value between 0.0 and 1.0.
     */
    double downloadProgress() const;

    /**
     * @brief Returns the local file path of the downloaded installer.
     * @return Absolute file path, or empty if no download has completed.
     */
    QString downloadedFilePath() const;

    /**
     * @brief Initiates an asynchronous update check against the remote server.
     * @param currentVersion The currently running application version string.
     */
    Q_INVOKABLE void checkForUpdates(const QString& currentVersion);

    /**
     * @brief Starts downloading the available update package.
     */
    Q_INVOKABLE void downloadUpdate();

    /**
     * @brief Cancels an active download and cleans up partial files.
     */
    Q_INVOKABLE void cancelDownload();

    /**
     * @brief Launches the downloaded installer and quits the application.
     * @return True if the installer was started successfully.
     */
    Q_INVOKABLE bool installUpdate();

    /**
     * @brief Opens the release page in the default system browser.
     */
    Q_INVOKABLE void openReleasePage() const;

Q_SIGNALS:
    /**
     * @brief Emitted when the busy state changes.
     */
    void busyChanged();

    /**
     * @brief Emitted when update availability or version info changes.
     */
    void updateAvailableChanged();

    /**
     * @brief Emitted when the status message changes.
     */
    void statusMessageChanged();

    /**
     * @brief Emitted when the service state transitions (Checking, Available, etc.).
     */
    void stateChanged();

    /**
     * @brief Emitted when download byte progress updates.
     */
    void downloadProgressChanged();

private:
    /**
     * @brief Sets the busy state and emits busyChanged().
     * @param busy New busy state.
     */
    void setBusy(bool busy);

    /**
     * @brief Sets the status message and emits statusMessageChanged().
     * @param message New status text.
     */
    void setStatusMessage(const QString& message);

    /**
     * @brief Sets the service state and emits stateChanged().
     * @param state New State value.
     */
    void setState(State state);

    /**
     * @brief Resets all release-related data (version, URLs, download state).
     */
    void resetRelease();

    /** @brief Network manager used for HTTP update requests. */
    QNetworkAccessManager m_network;
    /** @brief True when an update check is in progress. */
    bool m_busy{false};
    /** @brief True when a newer version has been detected. */
    bool m_updateAvailable{false};
    /** @brief Latest version string fetched from the update server. */
    QString m_latestVersion;
    /** @brief Human-readable status message for the UI. */
    QString m_statusMessage;
    /** @brief Current state of the update service lifecycle. */
    State m_state{Idle};
    /** @brief URL to the release page on GitHub. */
    QString m_releaseUrl;
    /** @brief Release notes markdown body. */
    QString m_releaseNotes;
    /** @brief Direct download URL for the installer asset. */
    QString m_installerUrl;
    /** @brief Filename of the installer asset. */
    QString m_installerName;
    /** @brief SHA-256 digest of the installer (prefixed with "sha256:"). */
    QString m_installerDigest;
    /** @brief Local path where the installer was saved. */
    QString m_downloadedFilePath;
    /** @brief Bytes downloaded so far. */
    qint64 m_downloadedBytes{0};
    /** @brief Total installer size in bytes. */
    qint64 m_totalBytes{0};
    /** @brief Pointer to the active network reply for download/check. */
    QPointer<QNetworkReply> m_reply;
    /** @brief File handle for safe atomic writes during download. */
    std::unique_ptr<QSaveFile> m_downloadFile;
};
