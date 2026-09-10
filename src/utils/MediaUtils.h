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

#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QUrl>

namespace MediaUtils {

/**
 * @brief Returns the list of recognised media file extensions.
 * @return QStringList of extensions (e.g. "mp4", "mkv", "avi").
 */
QStringList mediaExtensions();

/**
 * @brief Returns the list of recognised subtitle file extensions.
 * @return QStringList of extensions (e.g. "srt", "ass", "sub").
 */
QStringList subtitleExtensions();

/**
 * @brief Checks whether a file path has one of the given extensions.
 * @param path       The file path to check.
 * @param extensions List of extensions to match against (without leading dot).
 * @return True if the path's extension is in the list.
 */
bool hasExtension(const QString& path, const QStringList& extensions);

/**
 * @brief Determines whether a file path points to a recognised media file.
 * @param path The file path to check.
 * @return True if the extension matches a known media type.
 */
bool isMediaFile(const QString& path);

/**
 * @brief Determines whether a file path points to a recognised subtitle file.
 * @param path The file path to check.
 * @return True if the extension matches a known subtitle type.
 */
bool isSubtitleFile(const QString& path);

/**
 * @brief Checks whether a path is a local filesystem path (not a URL).
 * @param path The path string to check.
 * @return True if the path is local (not starting with a URI scheme).
 */
bool isLocalPath(const QString& path);

/**
 * @brief Returns a human-readable display name for a file path.
 * @param path The file path to extract a name from.
 * @return The file name (without directory) suitable for UI display.
 */
QString displayNameForPath(const QString& path);

/**
 * @brief Extracts the video ID from a supported YouTube URL.
 * @param locator YouTube URL (watch, short, embed, or youtu.be).
 * @return The 11-character video ID, or an empty string if not found.
 */
QString youtubeVideoId(const QString& locator);

/**
 * @brief Returns the stable thumbnail URL for a YouTube video.
 * @param locator YouTube URL or video ID.
 * @return The mqdefault.jpg URL, or an empty string if the ID could not be extracted.
 */
QString youtubeThumbnailUrl(const QString& locator);

/**
 * @brief Converts a path string to a local file URL or remote URL.
 * @param value Local path or remote URL string.
 * @return QUrl in the appropriate form, or a default QUrl if input is empty.
 */
QUrl localOrRemoteUrl(const QString& value);

/**
 * @brief Formats a time duration in seconds as a human-readable string.
 * @param seconds Duration in seconds.
 * @return Formatted string (e.g. "1:23:45" or "3:45").
 */
QString formatTime(double seconds);

/**
 * Return all playable files in a folder using deterministic directory/name order.
 *
 * @param folderPath Local folder path to scan recursively.
 * @return Absolute file paths accepted by CineWindows media extension filters.
 * @sideeffect Reads the filesystem but never modifies it.
 */
QStringList listMediaFiles(const QString& folderPath);

/**
 * Convert a QML/Qt URL into the string mpv expects.
 *
 * @param url Local file URL or remote playback URL.
 * @return Local filesystem path for file URLs, otherwise the encoded URL string.
 */
QString urlToMpvPath(const QUrl& url);

} // namespace MediaUtils
