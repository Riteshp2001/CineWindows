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

#include "services/MediaThumbnailService.h"

#include "utils/PathUtils.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QProcess>

MediaThumbnailService::MediaThumbnailService(QObject* parent)
    : QObject(parent)
{}

int MediaThumbnailService::pendingCount() const
{
    // Queued items + currently active ffmpeg processes
    return static_cast<int>(m_queue.size()) + m_active;
}

/**
 * @brief Computes a deterministic cached thumbnail path for a media file.
 * @details Uses SHA-256 of the path, file size, and last-modified time to
 *          create a unique filename that invalidates when the source changes.
 * @param path Original media file path.
 * @return Local filesystem path for the cached thumbnail JPEG.
 */
QString MediaThumbnailService::cachePath(const QString& path) const
{
    const QFileInfo info(path);
    // Build a unique identity hash from path + size + mtime
    const QByteArray identity = path.toUtf8() + QByteArray::number(info.size())
        + QByteArray::number(info.lastModified().toMSecsSinceEpoch());
    const QString name = QString::fromLatin1(QCryptographicHash::hash(identity, QCryptographicHash::Sha256).toHex())
        + QStringLiteral(".jpg");
    return QDir(PathUtils::thumbnailCacheDir()).filePath(name);
}

/**
 * @brief Returns a cached thumbnail URL or enqueues a generation request.
 * @param path File path to the media file.
 * @return Local file URL if the thumbnail exists on disk, empty QUrl otherwise.
 */
QUrl MediaThumbnailService::thumbnailFor(const QString& path)
{
    // Check if a cached thumbnail already exists on disk
    const QString cached = cachePath(path);
    if (QFileInfo(cached).size() > 0)
        return QUrl::fromLocalFile(cached);
    // No cache hit — enqueue an async generation request
    requestThumbnail(path);
    return {};
}

/**
 * @brief Enqueues a thumbnail generation request for the given media file.
 * @details Validates the file exists, checks for an existing cached thumbnail,
 *          and deduplicates against pending requests before enqueueing.
 * @param path File path to the media file.
 */
void MediaThumbnailService::requestThumbnail(const QString& path)
{
    // Ignore non-existent files
    if (!QFileInfo(path).isFile())
        return;
    const QString output = cachePath(path);
    // Already cached? Emit immediately
    if (QFileInfo(output).size() > 0)
    {
        Q_EMIT thumbnailReady(path, QUrl::fromLocalFile(output));
        return;
    }
    // Deduplicate: skip if already queued or in-flight
    if (m_pending.contains(path))
        return;
    m_pending.insert(path);
    m_queue.enqueue({path, output});
    Q_EMIT pendingCountChanged();
    startNext();
}

/**
 * @brief Launches queued thumbnail generation jobs via ffmpeg, up to MaxWorkers concurrent.
 * @details Spawns ffmpeg subprocesses for each queued job, extracting a single
 *          frame at 5 seconds, scaled to 480px wide. Uses a finalize lambda to
 *          handle completion, cleanup, and cascading to the next job.
 */
void MediaThumbnailService::startNext()
{
    while (m_active < MaxWorkers && !m_queue.isEmpty())
    {
        const Job job = m_queue.dequeue();
        // Resolve the platform-appropriate bundled executable before PATH.
        const QString executable = PathUtils::mediaToolPath(QStringLiteral("ffmpeg"));
        if (executable.isEmpty())
        {
            m_pending.remove(job.path);
            Q_EMIT thumbnailFailed(job.path);
            continue;
        }

        ++m_active;
        auto* process = new QProcess(this);
        process->setProgram(executable);
        // ffmpeg args: hide banner, seek to 5s, grab 1 frame, scale to 480p
        process->setArguments({QStringLiteral("-hide_banner"), QStringLiteral("-loglevel"), QStringLiteral("error"),
                               QStringLiteral("-ss"), QStringLiteral("5"), QStringLiteral("-i"), job.path,
                               QStringLiteral("-frames:v"), QStringLiteral("1"), QStringLiteral("-vf"),
                               QStringLiteral("scale=480:-2"), QStringLiteral("-q:v"), QStringLiteral("4"),
                               QStringLiteral("-y"), job.output});
        // Shared completion handler: guard against double-finalize via process property
        const auto finalize = [this, process, job](bool succeeded) {
            if (process->property("cinewindowsFinalized").toBool())
                return;
            process->setProperty("cinewindowsFinalized", true);
            --m_active;
            m_pending.remove(job.path);
            process->deleteLater();
            if (succeeded && QFileInfo(job.output).size() > 0)
                Q_EMIT thumbnailReady(job.path, QUrl::fromLocalFile(job.output));
            else
            {
                // Clean up partial output on failure
                QFile::remove(job.output);
                Q_EMIT thumbnailFailed(job.path);
            }
            Q_EMIT pendingCountChanged();
            startNext();
        };
        connect(process, &QProcess::finished, this,
                [finalize](int exitCode, QProcess::ExitStatus status) {
                    finalize(status == QProcess::NormalExit && exitCode == 0);
                });
        connect(process, &QProcess::errorOccurred, this, [finalize](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart)
                finalize(false);
        });
        process->start();
    }
    Q_EMIT pendingCountChanged();
}

/**
 * @brief Removes all cached thumbnail JPEG files from the thumbnail cache directory.
 */
void MediaThumbnailService::clearCache()
{
    QDir directory(PathUtils::thumbnailCacheDir());
    for (const QString& file : directory.entryList({QStringLiteral("*.jpg")}, QDir::Files))
        directory.remove(file);
}
