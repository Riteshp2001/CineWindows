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

#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QStringConverter>
#include <QStringDecoder>
#include <QStringList>

#include <array>

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

/** @brief Resolves the directory an mpv "~~<prefix>/" meta path refers to, or empty if unknown. */
QString metaDirectory(const QString& prefix, const QString& configDir)
{
    if (prefix.isEmpty() || prefix == QStringLiteral("home") || prefix == QStringLiteral("state")
        || prefix == QStringLiteral("old_home"))
        return QDir::fromNativeSeparators(configDir);
    if (prefix == QStringLiteral("cache"))
        return QDir(QDir::fromNativeSeparators(configDir)).filePath(QStringLiteral("cache"));
    if (prefix == QStringLiteral("desktop"))
        return QDir::fromNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    if (prefix == QStringLiteral("exe_dir"))
        return QCoreApplication::applicationDirPath();
    return {};
}

/** @brief Escapes text for use inside an mpv command double-quoted string. */
QString escapeDoubleQuoted(QString text)
{
    text.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    text.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    return text;
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
 * @brief Extracts an option value using mpv.conf rules.
 * @details Supports "quoted" and 'quoted' values, mpv's %n%fixed-length values and
 *          strips trailing # comments from unquoted values.
 * @param line A single option line from an mpv.conf file.
 * @param hasValue Optional output set to true when the line contains '='.
 * @return The unquoted value, or an empty string for flag-style lines.
 */
QString optionValueForLine(const QString& line, bool* hasValue)
{
    if (hasValue)
        *hasValue = false;
    if (optionNameForLine(line).isEmpty())
        return {};

    const qsizetype equals = line.indexOf(QLatin1Char('='));
    if (equals < 0)
        return {};
    if (hasValue)
        *hasValue = true;

    qsizetype start = equals + 1;
    while (start < line.size() && line.at(start).isSpace())
        ++start;
    const QString rest = line.mid(start);
    if (rest.isEmpty())
        return {};

    const QChar first = rest.at(0);
    if (first == QLatin1Char('"') || first == QLatin1Char('\''))
    {
        const qsizetype end = rest.indexOf(first, 1);
        return end < 0 ? rest.mid(1) : rest.mid(1, end - 1);
    }

    static const QRegularExpression fixedLength(QStringLiteral("^%(\\d+)%"));
    const QRegularExpressionMatch match = fixedLength.match(rest);
    if (match.hasMatch())
    {
        const qsizetype bytes = match.captured(1).toLongLong();
        return QString::fromUtf8(rest.mid(match.capturedLength()).toUtf8().left(bytes));
    }

    const qsizetype comment = rest.indexOf(QLatin1Char('#'));
    return (comment < 0 ? rest : rest.left(comment)).trimmed();
}

/**
 * @brief Removes list-operation suffixes such as -append, -add or -set from an option name.
 * @param name A normalized option name.
 * @return The base option name, e.g. "scripts" for "scripts-append".
 */
QString baseOptionName(const QString& name)
{
    static const std::array suffixes{QStringLiteral("-append"), QStringLiteral("-add"), QStringLiteral("-set"),
                                     QStringLiteral("-pre"), QStringLiteral("-clr"), QStringLiteral("-del"),
                                     QStringLiteral("-remove"), QStringLiteral("-toggle")};
    for (const QString& suffix : suffixes)
    {
        if (name.size() > suffix.size() && name.endsWith(suffix))
            return name.left(name.size() - suffix.size());
    }
    return name;
}

/**
 * @brief Checks whether an mpv option is blocked from being set in embedded config.
 * @details Blocks options that own the video output, the native window or input
 *          routing. Everything else, including hwdec, ao, shaders and profiles,
 *          is passed through to the embedded player.
 * @param name The mpv option name to check (will be normalized internally).
 * @return True if the option is blocked and should be removed from embedded config.
 */
bool isBlockedEmbeddedOption(const QString& name)
{
    static const QSet<QString> blocked = {
        QStringLiteral("config"),
        QStringLiteral("config-dir"),
        QStringLiteral("input-default-bindings"),
        QStringLiteral("input-vo-keyboard"),
        QStringLiteral("idle"),
        QStringLiteral("osc"),
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
        QStringLiteral("auto-window-resize"),
        QStringLiteral("window-scale"),
        QStringLiteral("window-maximized"),
        QStringLiteral("window-minimized"),
        QStringLiteral("border"),
        QStringLiteral("title-bar"),
        QStringLiteral("ontop"),
        QStringLiteral("ontop-level"),
        QStringLiteral("screen"),
        QStringLiteral("screen-name"),
        QStringLiteral("fs-screen"),
        QStringLiteral("fs-screen-name"),
        QStringLiteral("keepaspect-window"),
        QStringLiteral("snap-window"),
        QStringLiteral("title"),
        QStringLiteral("gpu-api"),
        QStringLiteral("gpu-context"),
    };

    const QString option = baseOptionName(normalizedOptionName(name));
    return blocked.contains(option) || option.startsWith(QStringLiteral("d3d11-"))
           || option.startsWith(QStringLiteral("win32-")) || option.startsWith(QStringLiteral("wayland-"))
           || option.startsWith(QStringLiteral("x11-")) || option.startsWith(QStringLiteral("drm-"))
           || option.startsWith(QStringLiteral("macos-")) || option.startsWith(QStringLiteral("cocoa-"));
}

/**
 * @brief Formats a value with mpv's %n% fixed-length syntax so any character is preserved.
 * @param value Raw option value.
 * @return The value as "%<utf8 length>%<value>".
 */
QString fixedLengthValue(const QString& value)
{
    return QLatin1Char('%') + QString::number(value.toUtf8().size()) + QLatin1Char('%') + value;
}

/**
 * @brief Expands mpv's ~~/, ~~home/, ~~state/, ~~cache/, ~~desktop/ and ~~exe_dir/ prefixes.
 * @details libmpv runs with config=no, so mpv itself cannot resolve these paths.
 * @param text Text that may contain mpv meta paths.
 * @param configDir The CineWindows mpv configuration directory that "~~" refers to.
 * @return The text with every recognised prefix replaced by an absolute path.
 */
QString expandMetaPaths(const QString& text, const QString& configDir)
{
    if (!text.contains(QStringLiteral("~~")))
        return text;

    static const QRegularExpression pattern(QStringLiteral("~~([a-z_]*)/"));
    QString result;
    qsizetype last = 0;
    QRegularExpressionMatchIterator it = pattern.globalMatch(text);
    while (it.hasNext())
    {
        const QRegularExpressionMatch match = it.next();
        const QString directory = metaDirectory(match.captured(1), configDir);
        if (directory.isEmpty())
            continue;
        result += text.mid(last, match.capturedStart() - last);
        result += directory + QLatin1Char('/');
        last = match.capturedEnd();
    }
    result += text.mid(last);
    return result;
}

/**
 * @brief Expands mpv meta paths inside an input.conf command, quoting expanded unquoted arguments.
 * @details Unquoted arguments that start with "~~" are re-emitted as double-quoted strings so
 *          a user profile path containing spaces is still passed as a single argument.
 * @param command The command part of an input.conf line.
 * @param configDir The CineWindows mpv configuration directory that "~~" refers to.
 * @return The command with meta paths expanded.
 */
QString expandCommandMetaPaths(const QString& command, const QString& configDir)
{
    if (!command.contains(QStringLiteral("~~")))
        return command;

    QString out;
    out.reserve(command.size() * 2);
    const qsizetype size = command.size();
    qsizetype i = 0;
    while (i < size)
    {
        const QChar c = command.at(i);
        if (c == QLatin1Char('#'))
        {
            out += command.mid(i); // trailing comment
            break;
        }
        if (c == QLatin1Char('"') || c == QLatin1Char('\''))
        {
            // Copy the quoted segment, honouring backslash escapes inside double quotes.
            qsizetype end = i + 1;
            QString content;
            while (end < size && command.at(end) != c)
            {
                if (c == QLatin1Char('"') && command.at(end) == QLatin1Char('\\') && end + 1 < size)
                {
                    content += command.mid(end, 2);
                    end += 2;
                    continue;
                }
                content += command.at(end);
                ++end;
            }
            if (content.contains(QStringLiteral("~~")))
            {
                // Single-quoted text has no escapes, so re-emit it double-quoted to stay safe.
                const QString literal = c == QLatin1Char('\'') ? escapeDoubleQuoted(content) : content;
                out += QLatin1Char('"') + expandMetaPaths(literal, configDir) + QLatin1Char('"');
            }
            else
            {
                out += command.mid(i, end - i + 1);
            }
            i = end + 1;
            continue;
        }

        const bool tokenStart = i == 0 || command.at(i - 1).isSpace() || command.at(i - 1) == QLatin1Char(';');
        if (tokenStart && command.mid(i, 2) == QStringLiteral("~~"))
        {
            qsizetype end = i;
            while (end < size && !command.at(end).isSpace() && command.at(end) != QLatin1Char(';')
                   && command.at(end) != QLatin1Char('#'))
                ++end;
            out += QLatin1Char('"') + escapeDoubleQuoted(expandMetaPaths(command.mid(i, end - i), configDir))
                   + QLatin1Char('"');
            i = end;
            continue;
        }

        out += c;
        ++i;
    }
    return out;
}

/**
 * @brief Decodes a text config file, honouring UTF-8/UTF-16/UTF-32 byte order marks.
 * @details Notepad and other Windows editors may save configs as UTF-16 or UTF-8 with BOM.
 * @param data Raw file contents.
 * @return Decoded text without a byte order mark.
 */
QString decodeConfigText(const QByteArray& data)
{
    const std::optional<QStringConverter::Encoding> encoding = QStringConverter::encodingForData(data);
    QStringDecoder decoder(encoding.value_or(QStringConverter::Utf8));
    QString text = decoder.decode(data);
    if (text.startsWith(QChar(0xFEFF)))
        text.remove(0, 1);
    return text;
}

} // namespace MpvConfigFilter
