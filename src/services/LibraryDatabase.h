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

#include <QSqlDatabase>
#include <QString>
#include <QVariantList>

namespace LibraryDatabase {

/** Open (or reuse) the library database connection.
    @param connectionName Unique name for the SQL connection.
    @param error  Optional pointer to receive an error message on failure.
    @return The opened QSqlDatabase handle (caller should check isOpen()). */
QSqlDatabase open(const QString& connectionName, QString* error = nullptr);

/** Create the database schema and enable WAL/journal-mode pragmas.
    @param database An open QSqlDatabase handle.
    @param error  Optional pointer to receive an error message on failure.
    @return true on success, false on failure. */
bool initialize(QSqlDatabase& database, QString* error = nullptr);

/** Build a normalised locator key from a file path or URL.
    @param locator Local file path or remote URL.
    @return A canonical string suitable for deduplication lookups. */
QString locatorKey(const QString& locator);

/** Retrieve the 500 most recent playback visits with media metadata.
    @param database An open QSqlDatabase handle.
    @return A QVariantList of maps, each representing one history entry. */
QVariantList playbackHistory(QSqlDatabase& database);

/** Remove all playback visits for a given media item.
    @param database An open QSqlDatabase handle.
    @param mediaId  The media_id whose history should be deleted.
    @return true if the DELETE succeeded. */
bool removePlaybackHistory(QSqlDatabase& database, qint64 mediaId);

} // namespace LibraryDatabase
