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

#include "player/MpvConfigFilter.h"

#include <QSet>
#include <QStringList>

namespace {

/**
 * @brief Normalizes an mpv option name for comparison.
 * @details Trims whitespace, lowercases, replaces underscores with hyphens,
 *          and strips the "no-" prefix used for boolean negation.
 * @param name Raw option name string.
 * @return Normalized option name suitable for blocked-option lookups.
 */
QString normalizedOptionName(QString name)
{
    name = name.trimmed().toLower();
    name.replace(QLatin1Char('_'), QLatin1Char('-'));
    // Strip the "no-" prefix from boolean negation options (e.g. "no-border" -> "border")
    if (name.startsWith(QStringLiteral("no-")))
    {
        name = name.mid(3);
    }
    return name;
}

} // namespace

namespace MpvConfigFilter {

/**
 * @brief Extracts the mpv option name from a single configuration line.
 * @details Strips comment/blank lines, removes leading "--" if present, and
 *          splits on '=' or whitespace to isolate the option name.
 * @param line A single line from an mpv.conf file.
 * @return The normalized option name, or empty string if the line is a comment or blank.
 */
QString optionNameForLine(const QString& line)
{
    QString text = line.trimmed();

    // Skip comments (starting with # or ;) and section headers (starting with [)
    if (text.isEmpty() || text.startsWith(QLatin1Char('#')) || text.startsWith(QLatin1Char(';'))
        || text.startsWith(QLatin1Char('[')))
    {
        return {};
    }

    // Strip optional double-dash prefix (e.g. "--no-border" -> "no-border")
    if (text.startsWith(QStringLiteral("--")))
    {
        text = text.mid(2);
    }

    // Find the split point at '=' or first whitespace
    const qsizetype equals = text.indexOf(QLatin1Char('='));
    qsizetype whitespace = -1;
    for (qsizetype i = 0; i < text.size(); ++i)
    {
        if (text.at(i).isSpace())
        {
            whitespace = i;
            break;
        }
    }

    // Use the earliest split point
    qsizetype split = text.size();
    if (equals >= 0)
    {
        split = equals;
    }
    if (whitespace >= 0 && whitespace < split)
    {
        split = whitespace;
    }

    return normalizedOptionName(text.left(split));
}

/**
 * @brief Checks whether an mpv option is blocked from being set in embedded config.
 * @details Blocks options that are security-sensitive, windowing-related, or
 *          platform-specific (d3d11, win32, wayland, x11, drm prefixes), as
 *          these must be controlled by the application, not user config.
 * @param name The mpv option name to check (will be normalized internally).
 * @return True if the option is blocked and should be removed from embedded config.
 */
bool isBlockedEmbeddedOption(const QString& name)
{
    // Options that must be managed by CineWindows itself, not user config
    static const QSet<QString> blocked = {
        QStringLiteral("include"),
        QStringLiteral("config"),
        QStringLiteral("config-dir"),
        QStringLiteral("load-scripts"),
        QStringLiteral("script"),
        QStringLiteral("script-opts"),
        QStringLiteral("input-conf"),
        QStringLiteral("input-default-bindings"),
        QStringLiteral("input-vo-keyboard"),
        QStringLiteral("vo"),
        QStringLiteral("wid"),
        QStringLiteral("window-id"),
        QStringLiteral("force-window"),
        QStringLiteral("force-window-position"),
        QStringLiteral("fullscreen"),
        QStringLiteral("fs"),
        QStringLiteral("geometry"),
        QStringLiteral("autofit"),
        QStringLiteral("autofit-larger"),
        QStringLiteral("autofit-smaller"),
        QStringLiteral("window-scale"),
        QStringLiteral("border"),
        QStringLiteral("ontop"),
        QStringLiteral("screen"),
        QStringLiteral("screen-name"),
        QStringLiteral("fs-screen"),
        QStringLiteral("fs-screen-name"),
        QStringLiteral("keepaspect-window"),
        QStringLiteral("snap-window"),
        QStringLiteral("title"),
        QStringLiteral("x11-name"),
        QStringLiteral("gpu-api"),
        QStringLiteral("gpu-context"),
        QStringLiteral("gpu-hwdec-interop"),
        QStringLiteral("d3d11-output-format"),
        QStringLiteral("hwdec"),
        QStringLiteral("hwdec-codecs"),
        QStringLiteral("hwdec-extra-frames"),
    };

    const QString option = normalizedOptionName(name);
    // Check explicit blocklist and platform-specific prefixes
    return blocked.contains(option) || option.startsWith(QStringLiteral("d3d11-"))
           || option.startsWith(QStringLiteral("win32-")) || option.startsWith(QStringLiteral("wayland-"))
           || option.startsWith(QStringLiteral("x11-")) || option.startsWith(QStringLiteral("drm-"));
}

/**
 * @brief Sanitizes the contents of an mpv.conf file by removing blocked options.
 * @details Iterates each line, extracts the option name, and replaces blocked
 *          options with a comment noting the removal. Non-blocked lines pass through.
 * @param contents Raw contents of an mpv.conf file.
 * @return Sanitized configuration string with blocked options replaced by comments.
 */
QString sanitizeMpvConf(const QString& contents)
{
    QStringList output;
    const QStringList lines = contents.split(QLatin1Char('\n'));
    output.reserve(lines.size());

    for (QString line : lines)
    {
        // Normalize Windows-style line endings
        if (line.endsWith(QLatin1Char('\r')))
        {
            line.chop(1);
        }

        // Check if this line contains a blocked option
        const QString name = optionNameForLine(line);
        if (!name.isEmpty() && isBlockedEmbeddedOption(name))
        {
            // Replace blocked options with a comment explaining the removal
            output.append(QStringLiteral("# CineWindows ignored embedded-unsafe option: %1").arg(name));
            continue;
        }

        output.append(line);
    }

    return output.join(QLatin1Char('\n'));
}

} // namespace MpvConfigFilter
