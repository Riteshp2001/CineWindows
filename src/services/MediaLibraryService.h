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

#include <QElapsedTimer>
#include <QObject>
#include <QSqlDatabase>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class MediaLibraryService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(CineMpvItem* player READ player WRITE setPlayer NOTIFY playerChanged)
    Q_PROPERTY(QVariantList recent READ recent NOTIFY collectionsChanged)
    Q_PROPERTY(QVariantList history READ history NOTIFY collectionsChanged)
    Q_PROPERTY(QVariantList favorites READ favorites NOTIFY collectionsChanged)
    Q_PROPERTY(QVariantList library READ library NOTIFY collectionsChanged)
    Q_PROPERTY(QVariantList roots READ roots NOTIFY rootsChanged)
    Q_PROPERTY(QVariantMap statistics READ statistics NOTIFY collectionsChanged)
    Q_PROPERTY(bool indexing READ indexing NOTIFY indexingChanged)
    Q_PROPERTY(int scannedCount READ scannedCount NOTIFY scanProgressChanged)

public:
    /** Construct the service and set up the checkpoint timer.
        @param parent QObject parent. */
    explicit MediaLibraryService(QObject* parent = nullptr);

    /** Destructor – flushes playback state and closes the database. */
    ~MediaLibraryService() override;

    /** @return The currently attached mpv player item. */
    CineMpvItem* player() const;

    /** Attach (or detach) an mpv player item and wire playback signals.
        @param player The player to observe, or nullptr to detach. */
    void setPlayer(CineMpvItem* player);

    /** @return Recently played media items (last_played_at_ms descending). */
    QVariantList recent() const;

    /** @return Detailed playback-visit history with per-visit metadata. */
    QVariantList history() const;

    /** @return Favourited media items sorted by addition date descending. */
    QVariantList favorites() const;

    /** @return All media items currently present in scanned library roots. */
    QVariantList library() const;

    /** @return Configured library root folders with their last-scan timestamps. */
    QVariantList roots() const;

    /** @return Aggregate statistics (unique media, play count, watched ms, completions). */
    QVariantMap statistics() const;

    /** @return true while a background scan is in progress. */
    bool indexing() const;

    /** @return Number of files discovered in the most recent scan. */
    int scannedCount() const;

    /** Open the library database and populate all in-memory collections.
        Safe to call multiple times – subsequent calls are no-ops. */
    Q_INVOKABLE void initialize();

    /** Rebuild in-memory collections (recent, history, favorites, library, roots, statistics)
        from the current database state and emit collectionsChanged(). */
    Q_INVOKABLE void refresh();

    /** Add a folder as a library root and trigger a full rescan.
        @param folder Local folder URL.
        @return true if the root was added successfully. */
    Q_INVOKABLE bool addRoot(const QUrl& folder);

    /** Remove a library root by location and refresh collections.
        @param path File-system path of the root to remove. */
    Q_INVOKABLE void removeRoot(const QString& path);

    /** Start a concurrent scan of all library roots for media files.
        Already running or empty-root scans are no-ops. */
    Q_INVOKABLE void rescanAll();

    /** Check whether a path is in the favourites table.
        @param path File path or URL locator.
        @return true if favourited. */
    Q_INVOKABLE bool isFavorite(const QString& path) const;

    /** Add or remove a path from the favourites table.
        @param path File path or URL locator.
        @param kind Optional categorisation tag (default "media"). */
    Q_INVOKABLE void toggleFavorite(const QString& path, const QString& kind = QStringLiteral("media"));

    /** Delete every row from the playback_visits table. */
    Q_INVOKABLE void clearHistory();

    /** Reset recent-playback state on all media_items (position, play count, etc.). */
    Q_INVOKABLE void clearRecent();

    /** Delete playback visits for a specific media item.
        @param mediaId Target media_id. */
    Q_INVOKABLE void removeHistoryItem(qint64 mediaId);

    /** Immediately persist the current playback position and watched time. */
    Q_INVOKABLE void flushPlaybackState();

    /** Cache TMDB metadata for a local library item.
        @param mediaId Target media row.
        @param metadata Normalized metadata, or an empty map to record a completed lookup.
        @return true when the row was updated. */
    bool updateTmdbMetadata(qint64 mediaId, const QVariantMap& metadata);

Q_SIGNALS:
    void playerChanged();
    void collectionsChanged();
    void rootsChanged();
    void indexingChanged();
    void scanProgressChanged();
    void userMessage(const QString& message);

private:
    QVariantList queryCollection(const QString& statement) const;
    void beginPlayback();
    void updateActiveMetadata();
    void finishPlayback(const QString& reason);
    void checkpointPlayback();
    void updateRoots();
    void setIndexing(bool indexing);

    CineMpvItem* m_player{nullptr};            ///< Attached mpv player instance.
    QSqlDatabase m_database;                    ///< Open SQLite library database handle.
    QString m_connectionName;                   ///< Unique name for the SQL connection.
    QVariantList m_recent;                      ///< Cached recent-playback collection.
    QVariantList m_history;                     ///< Cached playback-visit history.
    QVariantList m_favorites;                   ///< Cached favourites collection.
    QVariantList m_library;                     ///< Cached library-items collection.
    QVariantList m_roots;                       ///< Cached library-root folders.
    QVariantMap m_statistics;                   ///< Cached aggregate statistics.
    QTimer m_checkpointTimer;                   ///< Periodic timer (15 s) for persisting position.
    QElapsedTimer m_watchClock;                 ///< Elapsed-time accumulator for the active file.
    qint64 m_activeMediaId{-1};                 ///< media_id of the currently playing item.
    qint64 m_activeVisitId{-1};                 ///< visit_id of the active playback-visit row.
    qint64 m_watchedMs{0};                      ///< Accumulated watched milliseconds this session.
    bool m_wasPlaying{false};                   ///< Playback state during the current watch-time interval.
    bool m_indexing{false};                     ///< true while a background scan is running.
    int m_scannedCount{0};                      ///< File count from the most recent scan.
    int m_scanGeneration{0};                    ///< Monotonically increasing scan counter (stale-guard).
    bool m_initialized{false};                  ///< Whether initialize() has completed.
};
