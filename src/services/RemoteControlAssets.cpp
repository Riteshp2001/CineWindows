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

#include "services/RemoteControlAssets.h"

#include <QFile>

/**
 * @brief Load a remote-control web asset from the Qt resource system.
 * @param resourcePath Qt resource path to the asset.
 * @return A RemoteControlAsset with status 200 and body on success,
 *         or status 404 with an error message if the resource is missing.
 */
RemoteControlAsset loadRemoteControlAsset(const QString& resourcePath)
{
    QFile asset(resourcePath);
    if (!asset.open(QIODevice::ReadOnly))
        return {404, QByteArrayLiteral("Not found")};

    return {200, asset.readAll()};
}
