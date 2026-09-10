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

#include <QByteArray>
#include <QString>

/** @brief Represents an HTTP asset response with status code and body. */
struct RemoteControlAsset
{
    /** @brief HTTP status code (200 on success, 404 if the resource is missing). */
    int status;
    /** @brief Raw body content of the asset. */
    QByteArray body;
};

/**
 * @brief Loads a remote-control web asset from the Qt resource system.
 * @param resourcePath Qt resource path to the asset (e.g. ":/cinewindows/remote/companion.js").
 * @return A RemoteControlAsset with status 200 and the file contents on success,
 *         or status 404 with an error message if the resource cannot be opened.
 */
RemoteControlAsset loadRemoteControlAsset(const QString& resourcePath);
