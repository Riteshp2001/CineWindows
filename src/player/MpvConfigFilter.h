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

namespace MpvConfigFilter {

/**
 * @brief Extracts the mpv option name from a configuration line.
 * @param line A single line from an mpv.conf file.
 * @return The normalized option name, or empty if the line is a comment, blank or section header.
 */
QString optionNameForLine(const QString& line);

/**
 * @brief Extracts an option value using mpv.conf rules (quotes, %n% fixed-length values, # comments).
 * @param line A single option line from an mpv.conf file.
 * @param hasValue Optional output set to true when the line contains '='.
 * @return The unquoted value, or an empty string for flag-style lines.
 */
QString optionValueForLine(const QString& line, bool* hasValue = nullptr);

/**
 * @brief Removes list-operation suffixes such as -append, -add or -set from an option name.
 * @param name A normalized option name.
 * @return The base option name, e.g. "scripts" for "scripts-append".
 */
QString baseOptionName(const QString& name);

/**
 * @brief Checks whether an mpv option is blocked from being set in embedded config.
 * @param name The mpv option name to check.
 * @return True if the option controls the window, video output or input ownership of the app.
 */
bool isBlockedEmbeddedOption(const QString& name);

/**
 * @brief Formats a value with mpv's %n% fixed-length syntax so any character is preserved.
 * @param value Raw option value.
 * @return The value as "%<utf8 length>%<value>".
 */
QString fixedLengthValue(const QString& value);

/**
 * @brief Expands mpv's ~~/, ~~home/, ~~state/, ~~cache/, ~~desktop/ and ~~exe_dir/ prefixes.
 * @param text Text that may contain mpv meta paths.
 * @param configDir The CineWindows mpv configuration directory that "~~" refers to.
 * @return The text with every recognised prefix replaced by an absolute path.
 */
QString expandMetaPaths(const QString& text, const QString& configDir);

/**
 * @brief Expands mpv meta paths inside an input.conf command, quoting expanded unquoted arguments.
 * @param command The command part of an input.conf line.
 * @param configDir The CineWindows mpv configuration directory that "~~" refers to.
 * @return The command with meta paths expanded and safe against spaces in the expanded path.
 */
QString expandCommandMetaPaths(const QString& command, const QString& configDir);

/**
 * @brief Decodes a text config file, honouring UTF-8/UTF-16/UTF-32 byte order marks.
 * @param data Raw file contents.
 * @return Decoded text without a byte order mark.
 */
QString decodeConfigText(const QByteArray& data);

} // namespace MpvConfigFilter
