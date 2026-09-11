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

#include "services/LibraryDatabase.h"

#include "app/LoggingCategories.h"
#include "utils/MediaUtils.h"

#include "utils/PathUtils.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>

namespace {
/// Execute a SQL statement and capture the error text on failure.
bool exec(QSqlQuery& query, const QString& statement, QString* error)
{
    if (query.exec(statement))
        return true;
    const QString message = query.lastError().text();
    qCWarning(cineLibraryLog).noquote() << "Database statement failed:" << message;
    if (error)
        *error = message;
    return false;
}

bool columnExists(QSqlDatabase& database, const QString& table, const QString& column)
{
    QSqlQuery query(database);
    if (!query.exec(QStringLiteral("PRAGMA table_info(%1)").arg(table)))
    {
        qCWarning(cineLibraryLog).noquote()
            << "Could not inspect the database schema:" << query.lastError().text();
        return false;
    }
    while (query.next())
    {
        if (query.value(1).toString() == column)
            return true;
    }
    return false;
}
} // namespace

namespace LibraryDatabase {

/** @copydoc LibraryDatabase::open */
QSqlDatabase open(const QString& connectionName, QString* error)
{
    // Reuse an existing connection or create a new QSQLITE one.
    const bool existed = QSqlDatabase::contains(connectionName);
    QSqlDatabase database = existed
        ? QSqlDatabase::database(connectionName)
        : QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    database.setDatabaseName(PathUtils::libraryDatabasePath());
    if (database.isOpen())
    {
        return database;
    }
    if (!database.open())
    {
        const QString message = database.lastError().text();
        qCWarning(cineLibraryLog).noquote() << "Could not open the media database:" << message;
        if (error)
            *error = message;
        return database;
    }
    // Ensure schema is up to date; close on failure.
    if (!initialize(database, error))
        database.close();
    return database;
}

/** @copydoc LibraryDatabase::initialize */
bool initialize(QSqlDatabase& database, QString* error)
{
    QSqlQuery query(database);
    // Enable foreign keys, WAL mode, NORMAL sync, and a busy timeout.
    if (!exec(query, QStringLiteral("PRAGMA busy_timeout = 5000"), error)
        || !exec(query, QStringLiteral("PRAGMA foreign_keys = ON"), error)
        || !exec(query, QStringLiteral("PRAGMA journal_mode = WAL"), error)
        || !exec(query, QStringLiteral("PRAGMA synchronous = NORMAL"), error))
        return false;

    int schemaVersion = 0;
    if (!query.exec(QStringLiteral("PRAGMA user_version")) || !query.next())
    {
        const QString message = query.lastError().text();
        qCWarning(cineLibraryLog).noquote() << "Could not read the database schema version:" << message;
        if (error)
            *error = message;
        return false;
    }
    schemaVersion = query.value(0).toInt();
    query.finish();
    if (schemaVersion >= 2)
        return true;

    if (!exec(query, QStringLiteral("BEGIN IMMEDIATE"), error))
        return false;
    if (!exec(query, QStringLiteral("PRAGMA user_version"), error) || !query.next())
    {
        database.rollback();
        return false;
    }
    schemaVersion = query.value(0).toInt();
    query.finish();
    if (schemaVersion >= 2)
    {
        database.rollback();
        return true;
    }

    // Core schema: metadata, media items, playback visits, favourites, library roots/entries, saved sessions.
    const QStringList statements = {
        QStringLiteral("CREATE TABLE IF NOT EXISTS app_meta (key TEXT PRIMARY KEY, value TEXT NOT NULL)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS media_items ("
                       "media_id INTEGER PRIMARY KEY, locator TEXT NOT NULL, locator_key TEXT NOT NULL UNIQUE, "
                       "display_name TEXT, media_title TEXT, source_kind TEXT NOT NULL DEFAULT 'local', "
                       "duration_ms INTEGER NOT NULL DEFAULT 0, file_size_bytes INTEGER, file_modified_at_ms INTEGER, "
                       "thumbnail_path TEXT, first_seen_at_ms INTEGER NOT NULL, last_seen_at_ms INTEGER NOT NULL, "
                       "last_played_at_ms INTEGER, play_count INTEGER NOT NULL DEFAULT 0, "
                       "position_ms INTEGER NOT NULL DEFAULT 0, completed INTEGER NOT NULL DEFAULT 0, missing INTEGER NOT NULL DEFAULT 0)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS playback_visits ("
                       "visit_id INTEGER PRIMARY KEY, media_id INTEGER NOT NULL REFERENCES media_items(media_id) ON DELETE CASCADE, "
                       "opened_at_ms INTEGER NOT NULL, ended_at_ms INTEGER, start_position_ms INTEGER NOT NULL DEFAULT 0, "
                       "end_position_ms INTEGER NOT NULL DEFAULT 0, watched_ms INTEGER NOT NULL DEFAULT 0, "
                       "completed INTEGER NOT NULL DEFAULT 0, end_reason TEXT)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS favorites ("
                       "locator_key TEXT PRIMARY KEY, locator TEXT NOT NULL, kind TEXT NOT NULL DEFAULT 'media', "
                       "added_at_ms INTEGER NOT NULL)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS library_roots ("
                       "root_id INTEGER PRIMARY KEY, locator TEXT NOT NULL, locator_key TEXT NOT NULL UNIQUE, "
                       "added_at_ms INTEGER NOT NULL, last_scan_at_ms INTEGER, enabled INTEGER NOT NULL DEFAULT 1)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS library_entries ("
                       "root_id INTEGER NOT NULL REFERENCES library_roots(root_id) ON DELETE CASCADE, "
                       "media_id INTEGER NOT NULL REFERENCES media_items(media_id) ON DELETE CASCADE, "
                       "last_seen_at_ms INTEGER NOT NULL, PRIMARY KEY(root_id, media_id))"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS saved_sessions ("
                       "session_key TEXT PRIMARY KEY, updated_at_ms INTEGER NOT NULL, current_ordinal INTEGER, "
                       "position_ms INTEGER NOT NULL DEFAULT 0)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS saved_session_items ("
                       "session_key TEXT NOT NULL REFERENCES saved_sessions(session_key) ON DELETE CASCADE, "
                       "ordinal INTEGER NOT NULL, locator TEXT NOT NULL, PRIMARY KEY(session_key, ordinal))"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS media_last_played_idx ON media_items(last_played_at_ms DESC)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS visits_opened_idx ON playback_visits(opened_at_ms DESC)")};

    // Execute all DDL inside a single transaction.
    for (const QString& statement : statements)
    {
        if (!exec(query, statement, error))
        {
            database.rollback();
            return false;
        }
    }

    if (schemaVersion < 2)
    {
        const QStringList metadataColumns = {
            QStringLiteral("tmdb_id INTEGER"),
            QStringLiteral("tmdb_media_type TEXT"),
            QStringLiteral("tmdb_title TEXT"),
            QStringLiteral("tmdb_overview TEXT"),
            QStringLiteral("tmdb_release_year INTEGER"),
            QStringLiteral("tmdb_rating REAL"),
            QStringLiteral("tmdb_artwork_url TEXT"),
            QStringLiteral("tmdb_updated_at_ms INTEGER")};
        for (const QString& definition : metadataColumns)
        {
            const QString column = definition.section(QLatin1Char(' '), 0, 0);
            if (columnExists(database, QStringLiteral("media_items"), column))
                continue;
            if (!exec(query, QStringLiteral("ALTER TABLE media_items ADD COLUMN %1").arg(definition), error))
            {
                database.rollback();
                return false;
            }
        }
        if (!exec(query, QStringLiteral("PRAGMA user_version = 2"), error))
        {
            database.rollback();
            return false;
        }
    }
    if (!database.commit())
    {
        const QString message = database.lastError().text();
        qCWarning(cineLibraryLog).noquote() << "Could not commit the database schema:" << message;
        if (error)
            *error = message;
        return false;
    }
    return true;
}

/** @copydoc LibraryDatabase::locatorKey */
QString locatorKey(const QString& locator)
{
    const QUrl url = QUrl::fromUserInput(locator);
    // Remote URLs: normalise path segments and strip trailing slashes.
    if (!url.isLocalFile() && !url.scheme().isEmpty())
        return url.adjusted(QUrl::NormalizePathSegments | QUrl::StripTrailingSlash).toString(QUrl::FullyEncoded);

    // Local paths: resolve to an absolute, cleaned path.
    QString path = url.isLocalFile() ? url.toLocalFile() : locator;
    QFileInfo info(path);
    path = QDir::cleanPath(info.absoluteFilePath());
#ifdef Q_OS_WIN
    // Case-fold on Windows so "C:\Foo" and "c:\foo" produce the same key.
    path = path.toCaseFolded();
#endif
    return QDir::fromNativeSeparators(path);
}

/** @copydoc LibraryDatabase::playbackHistory */
QVariantList playbackHistory(QSqlDatabase& database)
{
    QVariantList history;
    if (!database.isOpen())
        return history;
    QSqlQuery query(database);
    // Rank visits per media_id (most recent first), aggregate watched_ms and visit_count,
    // then return the top 500 rows joined against media_items and favourites.
    if (!query.exec(QStringLiteral(
            "WITH ranked AS ("
            "SELECT v.*, "
            "ROW_NUMBER() OVER (PARTITION BY v.media_id ORDER BY v.opened_at_ms DESC, v.visit_id DESC) AS history_rank, "
            "COUNT(*) OVER (PARTITION BY v.media_id) AS visit_count, "
            "SUM(v.watched_ms) OVER (PARTITION BY v.media_id) AS cumulative_watched_ms "
            "FROM playback_visits v) "
            "SELECT v.visit_id, v.opened_at_ms, v.ended_at_ms, v.end_position_ms, "
            "v.cumulative_watched_ms, v.visit_count, v.end_reason, "
            "m.media_id, m.locator, COALESCE(NULLIF(m.tmdb_title, ''), NULLIF(m.media_title, ''), m.display_name) AS title, "
            "m.duration_ms, v.end_position_ms AS position_ms, v.completed, m.play_count, m.last_played_at_ms, "
            "m.missing, COALESCE(NULLIF(m.tmdb_artwork_url, ''), m.thumbnail_path) AS thumbnail_path, "
            "m.source_kind AS kind, m.tmdb_id, m.tmdb_media_type, m.tmdb_overview, "
            "m.tmdb_release_year, m.tmdb_rating, m.tmdb_updated_at_ms, "
            "EXISTS(SELECT 1 FROM favorites f WHERE f.locator_key=m.locator_key) AS favorite "
            "FROM ranked v JOIN media_items m ON m.media_id=v.media_id "
            "WHERE v.history_rank=1 ORDER BY v.opened_at_ms DESC, v.visit_id DESC LIMIT 500")))
        return history;

    while (query.next())
    {
        // Convert milliseconds to seconds for duration/position; compute progress ratio.
        const double duration = static_cast<double>(query.value(QStringLiteral("duration_ms")).toLongLong()) / 1000.0;
        const double position = static_cast<double>(query.value(QStringLiteral("position_ms")).toLongLong()) / 1000.0;
        history.append(QVariantMap{
            {QStringLiteral("mediaId"), query.value(QStringLiteral("media_id"))},
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
            {QStringLiteral("metadataUpdatedAt"), query.value(QStringLiteral("tmdb_updated_at_ms"))},
            {QStringLiteral("visitId"), query.value(QStringLiteral("visit_id"))},
            {QStringLiteral("openedAt"), query.value(QStringLiteral("opened_at_ms"))},
            {QStringLiteral("endedAt"), query.value(QStringLiteral("ended_at_ms"))},
            {QStringLiteral("watchedMs"), query.value(QStringLiteral("cumulative_watched_ms"))},
            {QStringLiteral("visitCount"), query.value(QStringLiteral("visit_count"))},
            {QStringLiteral("endReason"), query.value(QStringLiteral("end_reason"))}});
    }
    return history;
}

/** @copydoc LibraryDatabase::removePlaybackHistory */
bool removePlaybackHistory(QSqlDatabase& database, qint64 mediaId)
{
    if (!database.isOpen())
        return false;
    QSqlQuery query(database);
    query.prepare(QStringLiteral("DELETE FROM playback_visits WHERE media_id=?"));
    query.addBindValue(mediaId);
    return query.exec();
}

} // namespace LibraryDatabase
