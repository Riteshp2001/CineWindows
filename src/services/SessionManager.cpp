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

#include "services/SessionManager.h"

#include "models/PlaylistModel.h"
#include "services/LibraryDatabase.h"
#include "utils/PathUtils.h"

#include <QFile>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>

#include <algorithm>
#include <cmath>

namespace {
constexpr auto SessionKey = "last";

/** @brief Migrate a legacy JSON session file into the database. */
bool importLegacySession(QSqlDatabase& database)
{
    if (!database.isOpen())
        return false;

    // Check if legacy import was already performed
    QSqlQuery marker(database);
    marker.prepare(QStringLiteral("SELECT value FROM app_meta WHERE key = 'legacy_session_import_v1'"));
    if (marker.exec() && marker.next())
        return false;

    // Open the legacy session file
    QFile file(PathUtils::sessionFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject())
        return false;

    // Extract playlist paths from the JSON root
    const QJsonObject root = document.object();
    const QJsonArray items = root.value(QStringLiteral("playlist")).toArray();
    QStringList paths;
    for (const QJsonValue& value : items)
    {
        const QString path = value.toString().trimmed();
        if (!path.isEmpty())
            paths.append(path);
    }
    if (paths.isEmpty() || !database.transaction())
        return false;

    // Clamp legacy index and position to valid ranges
    int index = root.value(QStringLiteral("currentIndex")).toInt(-1);
    index = std::clamp(index, -1, static_cast<int>(paths.size()) - 1);
    double position = root.value(QStringLiteral("position")).toDouble(0.0);
    if (!std::isfinite(position) || position < 0.0)
        position = 0.0;

    // Insert the session header row
    QSqlQuery session(database);
    session.prepare(QStringLiteral("INSERT OR REPLACE INTO saved_sessions "
                                   "(session_key, updated_at_ms, current_ordinal, position_ms) VALUES (?, ?, ?, ?)"));
    session.addBindValue(QString::fromLatin1(SessionKey));
    session.addBindValue(QDateTime::currentMSecsSinceEpoch());
    session.addBindValue(index);
    session.addBindValue(qRound64(position * 1000.0));
    if (!session.exec())
    {
        database.rollback();
        return false;
    }

    // Migrate each playlist item
    QSqlQuery item(database);
    item.prepare(QStringLiteral("INSERT INTO saved_session_items (session_key, ordinal, locator) VALUES (?, ?, ?)"));
    for (int i = 0; i < paths.size(); ++i)
    {
        item.bindValue(0, QString::fromLatin1(SessionKey));
        item.bindValue(1, i);
        item.bindValue(2, paths.at(i));
        if (!item.exec())
        {
            database.rollback();
            return false;
        }
    }
    // Mark the migration as complete
    QSqlQuery imported(database);
    if (!imported.exec(QStringLiteral("INSERT OR REPLACE INTO app_meta (key, value) "
                                      "VALUES ('legacy_session_import_v1', '1')")))
    {
        database.rollback();
        return false;
    }
    return database.commit();
}
} // namespace

SessionManager::SessionManager(QObject* parent)
    : QObject(parent)
{}

/**
 * @brief Serialize the active playlist and playback state into the database.
 * @param playlist  Source playlist model to serialize.
 * @param currentIndex  Index of the currently playing item.
 * @param position  Playback position in seconds.
 * @return True on success, false on failure.
 */
bool SessionManager::save(PlaylistModel* playlist, int currentIndex, double position) const
{
    if (!playlist)
    {
        return false;
    }

    // Open the session database within a transaction
    QString error;
    QSqlDatabase database = LibraryDatabase::open(QStringLiteral("cine-session"), &error);
    if (!database.isOpen() || !database.transaction())
        return false;

    // Sanitize inputs
    if (!std::isfinite(position) || position < 0.0)
        position = 0.0;
    currentIndex = std::clamp(currentIndex, -1, playlist->count() - 1);

    // Upsert the session header
    QSqlQuery session(database);
    session.prepare(QStringLiteral("INSERT OR REPLACE INTO saved_sessions "
                                   "(session_key, updated_at_ms, current_ordinal, position_ms) VALUES (?, ?, ?, ?)"));
    session.addBindValue(QString::fromLatin1(SessionKey));
    session.addBindValue(QDateTime::currentMSecsSinceEpoch());
    session.addBindValue(currentIndex);
    session.addBindValue(qRound64(position * 1000.0));
    if (!session.exec())
    {
        database.rollback();
        return false;
    }

    // Clear stale items for this session key
    QSqlQuery clear(database);
    clear.prepare(QStringLiteral("DELETE FROM saved_session_items WHERE session_key = ?"));
    clear.addBindValue(QString::fromLatin1(SessionKey));
    if (!clear.exec())
    {
        database.rollback();
        return false;
    }

    // Write each playlist path as a row
    QSqlQuery item(database);
    item.prepare(QStringLiteral("INSERT INTO saved_session_items (session_key, ordinal, locator) VALUES (?, ?, ?)"));
    for (int i = 0; i < playlist->count(); ++i)
    {
        item.bindValue(0, QString::fromLatin1(SessionKey));
        item.bindValue(1, i);
        item.bindValue(2, playlist->pathAt(i));
        if (!item.exec())
        {
            database.rollback();
            return false;
        }
    }
    return database.commit();
}

/**
 * @brief Restore a previously saved session from the database.
 * @param playlist  Playlist model to populate with saved paths.
 * @return True if a session was found and loaded successfully.
 */
bool SessionManager::restore(PlaylistModel* playlist) const
{
    if (!playlist)
    {
        return false;
    }

    // Reset cached restore values
    m_restoredIndex = -1;
    m_restoredPosition = 0.0;

    // Open session database and run legacy migration
    QString error;
    QSqlDatabase database = LibraryDatabase::open(QStringLiteral("cine-session"), &error);
    if (!database.isOpen())
        return false;

    importLegacySession(database);

    // Read session header (ordinal + position)
    QSqlQuery session(database);
    session.prepare(QStringLiteral("SELECT current_ordinal, position_ms FROM saved_sessions WHERE session_key = ?"));
    session.addBindValue(QString::fromLatin1(SessionKey));
    if (!session.exec() || !session.next())
        return false;

    // Read saved playlist items in ordinal order
    QStringList paths;
    QSqlQuery items(database);
    items.prepare(QStringLiteral("SELECT locator FROM saved_session_items WHERE session_key = ? ORDER BY ordinal"));
    items.addBindValue(QString::fromLatin1(SessionKey));
    if (!items.exec())
        return false;
    while (items.next())
        paths.append(items.value(0).toString());

    // Populate the playlist and cache resume position
    playlist->addPaths(paths, true);
    m_restoredIndex = std::clamp(session.value(0).toInt(), -1, static_cast<int>(paths.size()) - 1);
    m_restoredPosition = qMax(0.0, static_cast<double>(session.value(1).toLongLong()) / 1000.0);
    return !paths.isEmpty();
}

int SessionManager::restoredIndex() const
{
    return m_restoredIndex;
}

double SessionManager::restoredPosition() const
{
    return m_restoredPosition;
}
