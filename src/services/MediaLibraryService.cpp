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

#include "services/MediaLibraryService.h"

#include "player/CineMpvItem.h"
#include "services/LibraryDatabase.h"
#include "utils/MediaUtils.h"

#include <QDateTime>
#include <QDirIterator>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QSqlError>
#include <QSqlQuery>
#include <QtConcurrentRun>

namespace {
/// Map a single media_items row (via QSqlQuery) to a QVariantMap with seconds-based duration/position.
QVariantMap mediaMap(const QSqlQuery& query)
{
    // Convert stored milliseconds to seconds for external consumption; compute 0..1 progress.
    const double duration = static_cast<double>(query.value(QStringLiteral("duration_ms")).toLongLong()) / 1000.0;
    const double position = static_cast<double>(query.value(QStringLiteral("position_ms")).toLongLong()) / 1000.0;
    return {{QStringLiteral("mediaId"), query.value(QStringLiteral("media_id"))},
            {QStringLiteral("path"), query.value(QStringLiteral("locator"))},
            {QStringLiteral("name"), query.value(QStringLiteral("title"))},
            {QStringLiteral("duration"), duration},
            {QStringLiteral("position"), position},
            {QStringLiteral("progress"), duration > 0.0 ? qBound(0.0, position / duration, 1.0) : 0.0},
            {QStringLiteral("completed"), query.value(QStringLiteral("completed")).toBool()},
            {QStringLiteral("playCount"), query.value(QStringLiteral("play_count"))},
            {QStringLiteral("lastPlayed"), query.value(QStringLiteral("last_played_at_ms"))},
            {QStringLiteral("favorite"), query.value(QStringLiteral("favorite")).toBool()},
            {QStringLiteral("kind"), query.value(QStringLiteral("kind")).toString()},
            {QStringLiteral("missing"), query.value(QStringLiteral("missing")).toBool()},
            {QStringLiteral("thumbnail"), MediaUtils::localOrRemoteUrl(query.value(QStringLiteral("thumbnail_path")).toString())},
            {QStringLiteral("tmdbId"), query.value(QStringLiteral("tmdb_id"))},
            {QStringLiteral("metadataType"), query.value(QStringLiteral("tmdb_media_type"))},
            {QStringLiteral("overview"), query.value(QStringLiteral("tmdb_overview"))},
            {QStringLiteral("releaseYear"), query.value(QStringLiteral("tmdb_release_year"))},
            {QStringLiteral("rating"), query.value(QStringLiteral("tmdb_rating"))},
            {QStringLiteral("metadataUpdatedAt"), query.value(QStringLiteral("tmdb_updated_at_ms"))}};
}
} // namespace

/** @copydoc MediaLibraryService::MediaLibraryService */
MediaLibraryService::MediaLibraryService(QObject* parent)
    : QObject(parent)
    // Generate a unique connection name per instance (using the object address).
    , m_connectionName(QStringLiteral("cine-library-%1").arg(reinterpret_cast<quintptr>(this)))
{
    // Persist playback position every 15 seconds while media is active.
    m_checkpointTimer.setInterval(15000);
    connect(&m_checkpointTimer, &QTimer::timeout, this, &MediaLibraryService::checkpointPlayback);
}

/** @copydoc MediaLibraryService::~MediaLibraryService */
MediaLibraryService::~MediaLibraryService()
{
    // Persist any in-flight position data before closing.
    flushPlaybackState();
    m_database.close();
}

CineMpvItem* MediaLibraryService::player() const { return m_player; }

/** @copydoc MediaLibraryService::setPlayer */
void MediaLibraryService::setPlayer(CineMpvItem* player)
{
    if (m_player == player)
        return;
    // Disconnect the previous player entirely.
    if (m_player)
        disconnect(m_player, nullptr, this, nullptr);
    m_player = player;
    if (m_player)
    {
        // Wire lifecycle signals: file change, title update, end-of-file, pause.
        connect(m_player, &CineMpvItem::fileStarted, this, [this] { finishPlayback(QStringLiteral("replaced")); });
        connect(m_player, &CineMpvItem::fileLoaded, this, &MediaLibraryService::beginPlayback);
        connect(m_player, &CineMpvItem::mediaTitleChanged, this, &MediaLibraryService::updateActiveMetadata);
        connect(m_player, &CineMpvItem::endFile, this, &MediaLibraryService::finishPlayback);
        connect(m_player, &CineMpvItem::pauseChanged, this, &MediaLibraryService::checkpointPlayback);
    }
    Q_EMIT playerChanged();
}

QVariantList MediaLibraryService::recent() const { return m_recent; }
QVariantList MediaLibraryService::history() const { return m_history; }
QVariantList MediaLibraryService::favorites() const { return m_favorites; }
QVariantList MediaLibraryService::library() const { return m_library; }
QVariantList MediaLibraryService::roots() const { return m_roots; }
QVariantMap MediaLibraryService::statistics() const { return m_statistics; }
bool MediaLibraryService::indexing() const { return m_indexing; }
int MediaLibraryService::scannedCount() const { return m_scannedCount; }

/** @copydoc MediaLibraryService::initialize */
void MediaLibraryService::initialize()
{
    if (m_initialized)
        return;
    m_initialized = true;

    QString error;
    m_database = LibraryDatabase::open(m_connectionName, &error);
    if (!m_database.isOpen())
    {
        Q_EMIT userMessage(tr("Could not open the media library: %1").arg(error));
        return;
    }
    // Populate in-memory collections from the freshly opened database.
    refresh();
}

/** @copydoc MediaLibraryService::queryCollection */
QVariantList MediaLibraryService::queryCollection(const QString& statement) const
{
    QVariantList result;
    if (!m_database.isOpen())
        return result;
    QSqlQuery query(m_database);
    if (!query.exec(statement))
        return result;
    // Map every result row through the shared mediaMap() helper.
    while (query.next())
        result.append(mediaMap(query));
    return result;
}

/** @copydoc MediaLibraryService::refresh */
void MediaLibraryService::refresh()
{
    if (!m_database.isOpen())
        return;
    // Shared column list: includes a sub-query to check favourite status.
    const QString fields = QStringLiteral(
        "m.media_id, m.locator, COALESCE(NULLIF(m.tmdb_title, ''), NULLIF(m.media_title, ''), m.display_name) AS title, "
        "m.duration_ms, m.position_ms, m.completed, m.play_count, m.last_played_at_ms, "
        "m.missing, COALESCE(NULLIF(m.tmdb_artwork_url, ''), m.thumbnail_path) AS thumbnail_path, "
        "m.source_kind AS kind, m.tmdb_id, m.tmdb_media_type, m.tmdb_overview, "
        "m.tmdb_release_year, m.tmdb_rating, m.tmdb_updated_at_ms, "
        "EXISTS(SELECT 1 FROM favorites f WHERE f.locator_key=m.locator_key) AS favorite ");
    // Rebuild the four main collections.
    m_recent = queryCollection(QStringLiteral("SELECT %1 FROM media_items m WHERE m.last_played_at_ms IS NOT NULL "
                                              "ORDER BY m.last_played_at_ms DESC LIMIT 100").arg(fields));
    m_favorites = queryCollection(QStringLiteral("SELECT %1 FROM media_items m JOIN favorites f ON f.locator_key=m.locator_key "
                                                 "ORDER BY f.added_at_ms DESC").arg(fields));
    m_library = queryCollection(QStringLiteral("SELECT DISTINCT %1 FROM media_items m JOIN library_entries le ON le.media_id=m.media_id "
                                               "ORDER BY title COLLATE NOCASE").arg(fields));

    // Compute aggregate statistics in a single query.
    QSqlQuery statisticsQuery(m_database);
    if (statisticsQuery.exec(QStringLiteral(
            "SELECT COUNT(CASE WHEN play_count > 0 THEN 1 END), COALESCE(SUM(play_count),0), "
            "COALESCE((SELECT SUM(watched_ms) FROM playback_visits),0), "
            "COALESCE((SELECT COUNT(*) FROM playback_visits WHERE completed=1),0) FROM media_items"))
        && statisticsQuery.next())
    {
        m_statistics = {{QStringLiteral("uniqueMedia"), statisticsQuery.value(0)},
                        {QStringLiteral("playCount"), statisticsQuery.value(1)},
                        {QStringLiteral("watchedMs"), statisticsQuery.value(2)},
                        {QStringLiteral("completions"), statisticsQuery.value(3)}};
    }

    m_history = LibraryDatabase::playbackHistory(m_database);
    updateRoots();
    Q_EMIT collectionsChanged();
}

/** @copydoc MediaLibraryService::updateRoots */
void MediaLibraryService::updateRoots()
{
    QVariantList roots;
    QSqlQuery query(m_database);
    // Query enabled library roots sorted case-insensitively.
    if (query.exec(QStringLiteral("SELECT locator, last_scan_at_ms FROM library_roots WHERE enabled=1 ORDER BY locator COLLATE NOCASE")))
    {
        while (query.next())
            roots.append(QVariantMap{{QStringLiteral("path"), query.value(0)}, {QStringLiteral("lastScan"), query.value(1)}});
    }
    // Only emit rootsChanged when the list actually changes (avoids spurious signal storms).
    if (roots != m_roots)
    {
        m_roots = roots;
        Q_EMIT rootsChanged();
    }
}

/** @copydoc MediaLibraryService::addRoot */
bool MediaLibraryService::addRoot(const QUrl& folder)
{
    const QString path = folder.isLocalFile() ? folder.toLocalFile() : folder.toString();
    // Only accept existing directories with an open database.
    if (!QFileInfo(path).isDir() || !m_database.isOpen())
        return false;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("INSERT OR IGNORE INTO library_roots(locator, locator_key, added_at_ms) VALUES(?, ?, ?)"));
    query.addBindValue(QDir::cleanPath(path));
    query.addBindValue(LibraryDatabase::locatorKey(path));
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    if (!query.exec())
        return false;
    updateRoots();
    // Immediately scan the newly added folder.
    rescanAll();
    return true;
}

/** @copydoc MediaLibraryService::removeRoot */
void MediaLibraryService::removeRoot(const QString& path)
{
    if (!m_database.isOpen())
        return;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM library_roots WHERE locator_key=?"));
    query.addBindValue(LibraryDatabase::locatorKey(path));
    query.exec();
    // Refresh collections to remove entries that belonged only to this root.
    refresh();
}

/** @copydoc MediaLibraryService::rescanAll */
void MediaLibraryService::rescanAll()
{
    if (m_indexing || m_roots.isEmpty())
        return;
    const QVariantList rootsSnapshot = m_roots;
    // Bump generation so stale async results (from a previously cancelled scan) are ignored.
    const int generation = ++m_scanGeneration;
    m_scannedCount = 0;
    Q_EMIT scanProgressChanged();
    setIndexing(true);

    auto* watcher = new QFutureWatcher<QVariantList>(this);
    connect(watcher, &QFutureWatcher<QVariantList>::finished, this, [this, watcher, generation] {
        const QVariantList files = watcher->result();
        watcher->deleteLater();
        // Discard results if a newer scan was started after this one.
        if (generation != m_scanGeneration)
            return;

        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (!m_database.transaction())
        {
            setIndexing(false);
            return;
        }
        // Replace library_entries entirely for a fresh snapshot.
        QSqlQuery clear(m_database);
        clear.exec(QStringLiteral("DELETE FROM library_entries"));
        // Upsert each discovered file into media_items.
        QSqlQuery media(m_database);
        media.prepare(QStringLiteral(
            "INSERT INTO media_items(locator, locator_key, display_name, source_kind, file_size_bytes, file_modified_at_ms, first_seen_at_ms, last_seen_at_ms) "
            "VALUES(?, ?, ?, 'local', ?, ?, ?, ?) ON CONFLICT(locator_key) DO UPDATE SET "
            "locator=excluded.locator, display_name=excluded.display_name, file_size_bytes=excluded.file_size_bytes, "
            "file_modified_at_ms=excluded.file_modified_at_ms, last_seen_at_ms=excluded.last_seen_at_ms, missing=0"));
        QSqlQuery mediaId(m_database);
        mediaId.prepare(QStringLiteral("SELECT media_id FROM media_items WHERE locator_key=?"));
        QSqlQuery rootId(m_database);
        rootId.prepare(QStringLiteral("SELECT root_id FROM library_roots WHERE locator_key=?"));
        QSqlQuery entry(m_database);
        entry.prepare(QStringLiteral("INSERT OR REPLACE INTO library_entries(root_id, media_id, last_seen_at_ms) VALUES(?, ?, ?)"));
        for (const QVariant& value : files)
        {
            const QVariantMap file = value.toMap();
            const QString path = file.value(QStringLiteral("path")).toString();
            const QString key = LibraryDatabase::locatorKey(path);
            media.bindValue(0, path);
            media.bindValue(1, key);
            media.bindValue(2, file.value(QStringLiteral("name")));
            media.bindValue(3, file.value(QStringLiteral("size")));
            media.bindValue(4, file.value(QStringLiteral("modified")));
            media.bindValue(5, now);
            media.bindValue(6, now);
            if (!media.exec())
                continue;
            // Retrieve the auto-generated media_id for the entry link.
            mediaId.bindValue(0, key);
            if (!mediaId.exec() || !mediaId.next())
                continue;
            rootId.bindValue(0, LibraryDatabase::locatorKey(file.value(QStringLiteral("root")).toString()));
            if (!rootId.exec() || !rootId.next())
                continue;
            entry.bindValue(0, rootId.value(0));
            entry.bindValue(1, mediaId.value(0));
            entry.bindValue(2, now);
            entry.exec();
        }
        // Mark all roots as freshly scanned.
        QSqlQuery scanned(m_database);
        scanned.prepare(QStringLiteral("UPDATE library_roots SET last_scan_at_ms=?"));
        scanned.addBindValue(now);
        scanned.exec();
        m_database.commit();
        m_scannedCount = static_cast<int>(files.size());
        Q_EMIT scanProgressChanged();
        setIndexing(false);
        refresh();
    });

    // Background thread: walk every root directory recursively collecting media files.
    watcher->setFuture(QtConcurrent::run([rootsSnapshot] {
        QVariantList files;
        for (const QVariant& rootValue : rootsSnapshot)
        {
            const QString root = rootValue.toMap().value(QStringLiteral("path")).toString();
            QDirIterator iterator(root, QDir::Files | QDir::Readable | QDir::NoDotAndDotDot,
                                  QDirIterator::Subdirectories);
            while (iterator.hasNext())
            {
                const QString path = iterator.next();
                // Skip non-media files and subtitle files.
                if (!MediaUtils::isMediaFile(path) || MediaUtils::isSubtitleFile(path))
                    continue;
                const QFileInfo info = iterator.fileInfo();
                files.append(QVariantMap{{QStringLiteral("root"), root},
                                         {QStringLiteral("path"), info.absoluteFilePath()},
                                         {QStringLiteral("name"), info.fileName()},
                                         {QStringLiteral("size"), info.size()},
                                         {QStringLiteral("modified"), info.lastModified().toMSecsSinceEpoch()}});
            }
        }
        return files;
    }));
}

/** @copydoc MediaLibraryService::isFavorite */
bool MediaLibraryService::isFavorite(const QString& path) const
{
    if (!m_database.isOpen())
        return false;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT 1 FROM favorites WHERE locator_key=?"));
    query.addBindValue(LibraryDatabase::locatorKey(path));
    // If a row exists, query.next() returns true → the path is favourited.
    return query.exec() && query.next();
}

/** @copydoc MediaLibraryService::toggleFavorite */
void MediaLibraryService::toggleFavorite(const QString& path, const QString& kind)
{
    if (path.trimmed().isEmpty() || !m_database.isOpen())
        return;
    const QString key = LibraryDatabase::locatorKey(path);
    QSqlQuery query(m_database);
    if (isFavorite(path))
    {
        // Remove existing favourite.
        query.prepare(QStringLiteral("DELETE FROM favorites WHERE locator_key=?"));
        query.addBindValue(key);
    }
    else
    {
        // Ensure a media_items row exists before inserting the favourite.
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        QSqlQuery media(m_database);
        media.prepare(QStringLiteral(
            "INSERT OR IGNORE INTO media_items(locator, locator_key, display_name, source_kind, first_seen_at_ms, last_seen_at_ms) "
            "VALUES(?, ?, ?, ?, ?, ?)"));
        media.addBindValue(path);
        media.addBindValue(key);
        media.addBindValue(MediaUtils::displayNameForPath(path));
        media.addBindValue(QFileInfo(path).isDir() ? QStringLiteral("folder") : kind);
        media.addBindValue(now);
        media.addBindValue(now);
        media.exec();
        query.prepare(QStringLiteral("INSERT INTO favorites(locator_key, locator, kind, added_at_ms) VALUES(?, ?, ?, ?)"));
        query.addBindValue(key);
        query.addBindValue(path);
        query.addBindValue(kind);
        query.addBindValue(now);
    }
    query.exec();
    refresh();
}

/** @copydoc MediaLibraryService::clearHistory */
void MediaLibraryService::clearHistory()
{
    if (!m_database.isOpen())
        return;
    QSqlQuery(m_database).exec(QStringLiteral("DELETE FROM playback_visits"));
    refresh();
}

/** @copydoc MediaLibraryService::clearRecent */
void MediaLibraryService::clearRecent()
{
    if (!m_database.isOpen())
        return;
    QSqlQuery(m_database).exec(QStringLiteral("UPDATE media_items SET last_played_at_ms=NULL, play_count=0, position_ms=0, completed=0"));
    refresh();
}

/** @copydoc MediaLibraryService::removeHistoryItem */
void MediaLibraryService::removeHistoryItem(qint64 mediaId)
{
    LibraryDatabase::removePlaybackHistory(m_database, mediaId);
    refresh();
}

bool MediaLibraryService::updateTmdbMetadata(qint64 mediaId, const QVariantMap& metadata)
{
    if (!m_database.isOpen() || mediaId <= 0)
        return false;

    QSqlQuery query(m_database);
    if (metadata.isEmpty())
    {
        query.prepare(QStringLiteral("UPDATE media_items SET tmdb_updated_at_ms=? WHERE media_id=?"));
        query.addBindValue(QDateTime::currentMSecsSinceEpoch());
        query.addBindValue(mediaId);
        return query.exec();
    }

    query.prepare(QStringLiteral(
        "UPDATE media_items SET tmdb_id=?, tmdb_media_type=?, tmdb_title=?, tmdb_overview=?, "
        "tmdb_release_year=?, tmdb_rating=?, tmdb_artwork_url=?, tmdb_updated_at_ms=? WHERE media_id=?"));
    query.addBindValue(metadata.value(QStringLiteral("id")));
    query.addBindValue(metadata.value(QStringLiteral("type")));
    query.addBindValue(metadata.value(QStringLiteral("title")));
    query.addBindValue(metadata.value(QStringLiteral("overview")));
    query.addBindValue(metadata.value(QStringLiteral("year")));
    query.addBindValue(metadata.value(QStringLiteral("rating")));
    query.addBindValue(metadata.value(QStringLiteral("artworkUrl")));
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    query.addBindValue(mediaId);
    return query.exec();
}

/** @copydoc MediaLibraryService::beginPlayback */
void MediaLibraryService::beginPlayback()
{
    if (!m_initialized)
        initialize();
    if (!m_player || !m_database.isOpen() || m_player->currentPath().isEmpty())
        return;
    // Finish any prior playback session first.
    finishPlayback(QStringLiteral("replaced"));

    const QString path = m_player->currentPath();
    const QFileInfo info(path);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const QString key = LibraryDatabase::locatorKey(path);
    const QString thumbnail = MediaUtils::youtubeThumbnailUrl(path);
    // Upsert media_items: insert or update locator, title, duration, thumbnail, timestamps, and increment play_count.
    QSqlQuery media(m_database);
    media.prepare(QStringLiteral(
        "INSERT INTO media_items(locator, locator_key, display_name, media_title, source_kind, duration_ms, file_size_bytes, "
        "file_modified_at_ms, thumbnail_path, first_seen_at_ms, last_seen_at_ms, last_played_at_ms, play_count) "
        "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 1) ON CONFLICT(locator_key) DO UPDATE SET "
        "locator=excluded.locator, display_name=excluded.display_name, media_title=excluded.media_title, "
        "duration_ms=excluded.duration_ms, thumbnail_path=excluded.thumbnail_path, last_seen_at_ms=excluded.last_seen_at_ms, "
        "last_played_at_ms=excluded.last_played_at_ms, play_count=media_items.play_count+1, missing=0"));
    media.addBindValue(path);
    media.addBindValue(key);
    media.addBindValue(MediaUtils::displayNameForPath(path));
    media.addBindValue(m_player->mediaTitle());
    media.addBindValue(MediaUtils::isLocalPath(path) ? QStringLiteral("local") : QStringLiteral("url"));
    media.addBindValue(qRound64(m_player->duration() * 1000.0));
    media.addBindValue(info.exists() ? info.size() : QVariant());
    media.addBindValue(info.exists() ? info.lastModified().toMSecsSinceEpoch() : QVariant());
    media.addBindValue(thumbnail);
    media.addBindValue(now);
    media.addBindValue(now);
    media.addBindValue(now);
    if (!media.exec())
        return;

    // Retrieve the media_id for the newly upserted row.
    QSqlQuery id(m_database);
    id.prepare(QStringLiteral("SELECT media_id, position_ms FROM media_items WHERE locator_key=?"));
    id.addBindValue(key);
    if (!id.exec() || !id.next())
        return;
    m_activeMediaId = id.value(0).toLongLong();

    // Open a new playback-visit row with the current position.
    QSqlQuery visit(m_database);
    visit.prepare(QStringLiteral("INSERT INTO playback_visits(media_id, opened_at_ms, start_position_ms) VALUES(?, ?, ?)"));
    visit.addBindValue(m_activeMediaId);
    visit.addBindValue(now);
    visit.addBindValue(qRound64(m_player->position() * 1000.0));
    if (visit.exec())
        m_activeVisitId = visit.lastInsertId().toLongLong();
    m_watchedMs = 0;
    m_watchClock.start();
    m_wasPlaying = !m_player->pause() && !m_player->idle();
    m_checkpointTimer.start();
    refresh();
}

/** @copydoc MediaLibraryService::updateActiveMetadata */
void MediaLibraryService::updateActiveMetadata()
{
    if (m_activeMediaId < 0 || !m_player || !m_database.isOpen())
        return;
    const QString title = m_player->mediaTitle().trimmed();
    const QString thumbnail = MediaUtils::youtubeThumbnailUrl(m_player->currentPath());
    // Only update if we actually have new metadata to persist.
    if (title.isEmpty() && thumbnail.isEmpty())
        return;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "UPDATE media_items SET media_title=COALESCE(NULLIF(?, ''), media_title), "
        "thumbnail_path=COALESCE(NULLIF(?, ''), thumbnail_path) WHERE media_id=?"));
    query.addBindValue(title);
    query.addBindValue(thumbnail);
    query.addBindValue(m_activeMediaId);
    if (query.exec())
        refresh();
}

/** @copydoc MediaLibraryService::checkpointPlayback */
void MediaLibraryService::checkpointPlayback()
{
    if (m_activeMediaId < 0 || !m_player || !m_database.isOpen())
        return;
    // Accumulate real watch time (skipping paused/idle time).
    if (m_watchClock.isValid())
    {
        if (m_wasPlaying)
            m_watchedMs += m_watchClock.elapsed();
        m_watchClock.restart();
    }
    m_wasPlaying = !m_player->pause() && !m_player->idle();
    const qint64 position = qMax<qint64>(0, qRound64(m_player->position() * 1000.0));
    const qint64 duration = qMax<qint64>(0, qRound64(m_player->duration() * 1000.0));
    // Persist position and duration on the media item.
    QSqlQuery media(m_database);
    media.prepare(QStringLiteral("UPDATE media_items SET position_ms=?, duration_ms=? WHERE media_id=?"));
    media.addBindValue(position);
    media.addBindValue(duration);
    media.addBindValue(m_activeMediaId);
    media.exec();
    // Update the active visit row with end position and cumulative watched time.
    if (m_activeVisitId >= 0)
    {
        QSqlQuery visit(m_database);
        visit.prepare(QStringLiteral("UPDATE playback_visits SET end_position_ms=?, watched_ms=? WHERE visit_id=?"));
        visit.addBindValue(position);
        visit.addBindValue(m_watchedMs);
        visit.addBindValue(m_activeVisitId);
        visit.exec();
    }
}

/** @copydoc MediaLibraryService::finishPlayback */
void MediaLibraryService::finishPlayback(const QString& reason)
{
    if (m_activeMediaId < 0)
        return;
    // Persist latest position before closing out the session.
    checkpointPlayback();
    m_checkpointTimer.stop();
    const qint64 position = m_player ? qRound64(m_player->position() * 1000.0) : 0;
    const qint64 duration = m_player ? qRound64(m_player->duration() * 1000.0) : 0;
    // Mark completed when the end-of-file signal fires or when 92 %+ of the duration was reached.
    const bool completed = reason == QStringLiteral("eof")
        || (duration > 0 && static_cast<double>(position) >= static_cast<double>(duration) * 0.92);
    QSqlQuery media(m_database);
    media.prepare(QStringLiteral("UPDATE media_items SET completed=?, position_ms=? WHERE media_id=?"));
    media.addBindValue(completed);
    media.addBindValue(completed ? 0 : qMax<qint64>(0, position));
    media.addBindValue(m_activeMediaId);
    media.exec();
    // Finalise the visit row with end timestamp, final position, and completion state.
    if (m_activeVisitId >= 0)
    {
        QSqlQuery visit(m_database);
        visit.prepare(QStringLiteral("UPDATE playback_visits SET ended_at_ms=?, end_position_ms=?, watched_ms=?, completed=?, end_reason=? WHERE visit_id=?"));
        visit.addBindValue(QDateTime::currentMSecsSinceEpoch());
        visit.addBindValue(qMax<qint64>(0, position));
        visit.addBindValue(m_watchedMs);
        visit.addBindValue(completed);
        visit.addBindValue(reason);
        visit.addBindValue(m_activeVisitId);
        visit.exec();
    }
    // Reset active-session state.
    m_activeMediaId = -1;
    m_activeVisitId = -1;
    m_watchedMs = 0;
    m_wasPlaying = false;
    m_watchClock.invalidate();
    refresh();
}

/** @copydoc MediaLibraryService::flushPlaybackState */
void MediaLibraryService::flushPlaybackState()
{
    checkpointPlayback();
}

/** @copydoc MediaLibraryService::setIndexing */
void MediaLibraryService::setIndexing(bool indexing)
{
    if (m_indexing == indexing)
        return;
    m_indexing = indexing;
    Q_EMIT indexingChanged();
}
