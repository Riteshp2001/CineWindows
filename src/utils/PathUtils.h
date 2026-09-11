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

#include <QString>

namespace PathUtils {

/**
 * @brief Returns the application configuration directory path.
 * @return Absolute path to the CineWindows config directory.
 */
QString appConfigDir();

/**
 * @brief Returns the mpv configuration directory path used by CineWindows.
 * @return Absolute path to the internal mpv config directory.
 */
QString mpvConfigDir();

/**
 * @brief Returns the user's own mpv configuration directory path.
 * @return Absolute path to the user's mpv config directory (e.g. ~/.config/mpv).
 */
QString mpvUserConfigDir();

/**
 * @brief Sets up the mpv environment by configuring the MPV_HOME and related variables.
 */
void setupMpvEnvironment();

/**
 * @brief Resolves a bundled media tool before falling back to the system PATH.
 * @param executableName Tool name with or without the Windows .exe suffix.
 * @return Absolute executable path, or an empty string when the tool is unavailable.
 */
QString mediaToolPath(const QString& executableName);

/**
 * @brief Returns the directory path where screenshots are saved.
 * @return Absolute path to the screenshots directory.
 */
QString screenshotsDir();

/**
 * @brief Returns the file path for the session persistence file.
 * @return Absolute path to the session JSON file.
 */
QString sessionFilePath();

/** @brief Returns the SQLite database used by sessions and the media library. */
QString libraryDatabasePath();

/** @brief Returns the persistent media thumbnail cache directory. */
QString thumbnailCacheDir();

/** @brief Returns the rotating application log file path. */
QString logFilePath();

} // namespace PathUtils
