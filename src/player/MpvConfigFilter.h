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

namespace MpvConfigFilter {

/**
 * @brief Extracts the mpv option name from a configuration line.
 * @param line A single line from an mpv.conf file.
 * @return The option name (text before '=' or first space), or empty if the line is a comment/blank.
 */
QString optionNameForLine(const QString& line);

/**
 * @brief Checks whether an mpv option is blocked from being set in embedded config.
 * @param name The mpv option name to check.
 * @return True if the option is blocked (e.g. security-sensitive options).
 */
bool isBlockedEmbeddedOption(const QString& name);

/**
 * @brief Sanitizes the contents of an mpv.conf file by removing blocked options.
 * @param contents Raw contents of an mpv.conf file.
 * @return Sanitized configuration string with blocked options removed.
 */
QString sanitizeMpvConf(const QString& contents);

} // namespace MpvConfigFilter
