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

#include <QObject>
#include <QQueue>
#include <QSet>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class MediaThumbnailService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY pendingCountChanged)

public:
    /**
     * @brief Constructs a MediaThumbnailService with an optional QObject parent.
     * @param parent Optional parent object for Qt ownership.
     */
    explicit MediaThumbnailService(QObject* parent = nullptr);

    /**
     * @brief Returns the number of thumbnails currently pending (queued + active).
     * @return Total pending thumbnail count.
     */
    int pendingCount() const;

    /**
     * @brief Returns a cached thumbnail URL for the given path, or enqueues a request.
     * @param path File path to the media file.
     * @return Local file URL if cached, otherwise an empty QUrl (enqueues request).
     */
    Q_INVOKABLE QUrl thumbnailFor(const QString& path);

    /**
     * @brief Enqueues a thumbnail generation request for the given media file.
     * @param path File path to the media file.
     */
    Q_INVOKABLE void requestThumbnail(const QString& path);

    /**
     * @brief Clears all cached thumbnail images from disk.
     */
    Q_INVOKABLE void clearCache();

Q_SIGNALS:
    /**
     * @brief Emitted when the pending count changes.
     */
    void pendingCountChanged();

    /**
     * @brief Emitted when a thumbnail has been successfully generated.
     * @param path Original media file path.
     * @param url  Local file URL of the generated thumbnail.
     */
    void thumbnailReady(const QString& path, const QUrl& url);

    /**
     * @brief Emitted when thumbnail generation fails for a file.
     * @param path Original media file path.
     */
    void thumbnailFailed(const QString& path);

private:
    /** @brief Represents a pending thumbnail generation job. */
    struct Job
    {
        QString path;   ///< Source media file path.
        QString output; ///< Destination thumbnail file path.
    };

    /**
     * @brief Computes the cached thumbnail file path for a media file.
     * @param path Original media file path.
     * @return Local file path for the cached thumbnail.
     */
    QString cachePath(const QString& path) const;

    /**
     * @brief Starts the next queued thumbnail job(s), up to MaxWorkers concurrent.
     */
    void startNext();

    /** @brief Queue of pending thumbnail generation jobs. */
    QQueue<Job> m_queue;
    /** @brief Set of paths currently queued or in-flight (deduplication). */
    QSet<QString> m_pending;
    /** @brief Number of currently active ffmpeg processes. */
    int m_active{0};
    /** @brief Maximum number of concurrent thumbnail generation processes. */
    static constexpr int MaxWorkers = 2;
};
