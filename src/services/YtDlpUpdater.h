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
 */

#pragma once

#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>

#include <memory>

class QNetworkReply;
class QSaveFile;
class QUrl;

/**
 * @brief Keeps a SHA-256-verified copy of the latest official yt-dlp release in the app data folder.
 * @details YouTube routinely rejects streams resolved by outdated yt-dlp builds, so playback
 *          prefers this managed copy over bundled or system installations.
 */
class YtDlpUpdater : public QObject
{
    Q_OBJECT

public:
    explicit YtDlpUpdater(QObject* parent = nullptr);
    ~YtDlpUpdater() override;

    /**
     * @brief Downloads a newer yt-dlp release when one is available.
     * @param force Ignore the daily check interval, e.g. after a stream was refused.
     */
    void checkForUpdate(bool force = false);

Q_SIGNALS:
    /** @brief Emitted after a verified release has replaced the managed yt-dlp copy. */
    void updated(const QString& version);

private:
    void download(const QUrl& url, const QString& digest, qint64 size, const QString& version);

    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    std::unique_ptr<QSaveFile> m_file;
    QCryptographicHash m_hash{QCryptographicHash::Sha256};
    QElapsedTimer m_lastForcedCheck;
};
