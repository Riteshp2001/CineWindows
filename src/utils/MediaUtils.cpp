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

#include "utils/MediaUtils.h"

#include <QDir>
#include <QDirIterator>
#include <QSet>
#include <QUrlQuery>

#include <cmath>

namespace {
QString normalizedExtension(QString extension)
{
    extension = extension.trimmed().toLower();
    if (extension.isEmpty())
    {
        return {};
    }
    if (!extension.startsWith(QLatin1Char('.')))
    {
        extension.prepend(QLatin1Char('.'));
    }
    return extension;
}

QStringList normalized(QStringList values)
{
    QSet<QString> seen;
    QStringList result;
    for (const QString& value : values)
    {
        const QString extension = normalizedExtension(value);  // Normalised, deduplicated extension
        if (!extension.isEmpty() && !seen.contains(extension))
        {
            seen.insert(extension);
            result.append(extension);
        }
    }
    return result;
}
} // namespace

namespace MediaUtils {

QStringList mediaExtensions()
{
    static const QStringList exts = normalized(  // Known media extensions
        {".264",  ".265",  ".3g2",  ".3gp", ".3gpp", ".aac",  ".ac3",  ".aif",  ".aiff", ".ape",  ".asf",  ".ass",
         ".avi",  ".av1",  ".bmp",  ".cue", ".dff",  ".divx", ".dsf",  ".dts",  ".eac3", ".flac", ".flv",  ".gif",
         ".heic", ".heif", ".ifo",  ".iso", ".ivf",  ".jpeg", ".jpg",  ".m2ts", ".m4a",  ".m4b",  ".m4v",  ".mka",
         ".mkv",  ".mov",  ".mp3",  ".mp4", ".mpeg", ".mpg",  ".mts",  ".ogg",  ".ogm",  ".ogv",  ".opus", ".png",
         ".ra",   ".rm",   ".rmvb", ".srt", ".ssa",  ".sub",  ".sup",  ".ts",   ".vob",  ".wav",  ".webm", ".webp",
         ".wma",  ".wmv",  ".xspf", ".m3u", ".m3u8", ".pls",  ".strm", ".url"});
    return exts;
}

QStringList subtitleExtensions()
{
    static const QStringList exts = normalized({".aqt",  ".ass",  ".dfxp", ".idx", ".jss", ".lrc", ".mks",   ".mpl",  // Known subtitle extensions
                                                ".mpl2", ".sami", ".sbv",  ".scc", ".smi", ".srt", ".ssa",   ".stl",
                                                ".sub",  ".sup",  ".ttml", ".txt", ".usf", ".vtt", ".webvtt"});
    return exts;
}

bool hasExtension(const QString& path, const QStringList& extensions)
{
    const QString suffix = normalizedExtension(QFileInfo(path).suffix());  // Normalised file suffix
    for (const QString& extension : extensions)
    {
        if (normalizedExtension(extension) == suffix)
            return true;
    }
    return false;
}

bool isMediaFile(const QString& path)
{
    return hasExtension(path, mediaExtensions());
}

bool isSubtitleFile(const QString& path)
{
    return hasExtension(path, subtitleExtensions());
}

bool isLocalPath(const QString& path)
{
    const QUrl url = QUrl::fromUserInput(path);  // Parsed user input as URL
    return url.isLocalFile() || url.scheme().isEmpty() || url.scheme().size() == 1;
}

QString displayNameForPath(const QString& path)
{
    const QUrl url = QUrl::fromUserInput(path);
    if (url.isLocalFile())
    {
        return QFileInfo(url.toLocalFile()).fileName();
    }
    const QFileInfo info(path);
    if (!info.fileName().isEmpty())
    {
        return info.fileName();
    }
    return path;
}

QString youtubeVideoId(const QString& locator)
{
    const QUrl url = QUrl::fromUserInput(locator);  // Parsed YouTube URL
    const QString host = url.host().toLower();  // Normalised hostname for matching
    const QStringList segments = url.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);
    QString id;
    if (host == QStringLiteral("youtu.be") || host == QStringLiteral("www.youtu.be"))
    {
        if (!segments.isEmpty())
            id = segments.constFirst();
    }
    else if (host == QStringLiteral("youtube.com") || host.endsWith(QStringLiteral(".youtube.com")))
    {
        id = QUrlQuery(url).queryItemValue(QStringLiteral("v"));
        if (id.isEmpty() && segments.size() >= 2
            && (segments.at(0) == QStringLiteral("shorts") || segments.at(0) == QStringLiteral("embed")
                || segments.at(0) == QStringLiteral("live")))
        {
            id = segments.at(1);
        }
    }
    return id.trimmed();
}

QString youtubeThumbnailUrl(const QString& locator)
{
    const QString id = youtubeVideoId(locator);  // Extracted video ID
    return id.isEmpty() ? QString() : QStringLiteral("https://i.ytimg.com/vi/%1/mqdefault.jpg").arg(id);
}

QUrl localOrRemoteUrl(const QString& value)
{
    if (value.trimmed().isEmpty())
        return {};
    return isLocalPath(value) ? QUrl::fromLocalFile(value) : QUrl(value);
}

QString formatTime(double seconds)
{
    if (!std::isfinite(seconds) || seconds <= 0.0)
    {
        return QStringLiteral("0:00");
    }

    const qint64 total = static_cast<qint64>(seconds);  // Total whole seconds
    const qint64 days = total / 86400;  // Days component
    const qint64 hours = (total % 86400) / 3600;  // Hours component
    const qint64 minutes = (total % 3600) / 60;  // Minutes component
    const qint64 secs = total % 60;  // Seconds component

    if (days > 0)
    {
        return QStringLiteral("%1:%2:%3:%4")
            .arg(days)
            .arg(hours, 2, 10, QLatin1Char('0'))
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(secs, 2, 10, QLatin1Char('0'));
    }
    if (hours > 0)
    {
        return QStringLiteral("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(secs, 2, 10, QLatin1Char('0'));
    }
    return QStringLiteral("%1:%2").arg(minutes).arg(secs, 2, 10, QLatin1Char('0'));
}

QStringList listMediaFiles(const QString& folderPath)
{
    QStringList result;  // Accumulated absolute file paths
    QDirIterator it(folderPath, QDir::Files, QDirIterator::Subdirectories);  // Recursive folder scan
    while (it.hasNext())
    {
        const QString path = it.next();
        if (isMediaFile(path))
        {
            result.append(QFileInfo(path).absoluteFilePath());
        }
    }
    result.sort(Qt::CaseInsensitive);
    return result;
}

QString urlToMpvPath(const QUrl& url)
{
    if (url.isLocalFile())
    {
        return QDir::toNativeSeparators(url.toLocalFile());
    }
    return url.toString();
}

} // namespace MediaUtils
