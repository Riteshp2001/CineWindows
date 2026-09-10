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

#include <QObject>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

/**
 * @class FileService
 * @brief QML-facing helper for media file selection, path conversion, and OS file actions.
 *
 * @details
 * FileService keeps filesystem and URL utility logic out of QML views. It provides
 * dialog filters, drag/drop path conversion, folder scanning, display names, and
 * platform-safe "open containing location" behavior.
 *
 * Responsibilities:
 * - Expose supported media/subtitle/audio filters to QML dialogs
 * - Convert QUrl values from dialogs and drag/drop into mpv-ready paths
 * - Scan local folders for playable media
 * - Provide display names and icons for playlist rows
 *
 * This class must not own playlist state or playback commands.
 */
class FileService : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    /**
     * @brief Constructs a FileService instance.
     * @param parent Optional QObject parent for Qt memory management.
     */
    explicit FileService(QObject* parent = nullptr);

    /**
     * @brief Returns QFileDialog name filters for playable media formats.
     *
     * @details Each entry follows Qt's name-filter format:
     *          @c "Description (*.ext1 *.ext2 ...)". Supported formats
     *          include common video containers and audio codecs.
     *
     * @usecase Bound to open/add media file dialogs.
     *
     * @return Name filter strings suitable for Qt Quick FileDialog.
     *
     * @sideeffects None.
     * @thread May be called on the main Qt/QML thread.
     */
    Q_INVOKABLE QStringList mediaNameFilters() const;

    /**
     * @brief Returns QFileDialog name filters for subtitle track files.
     *
     * @details Entries follow Qt's name-filter format and cover common
     *          subtitle formats (SRT, ASS, SSA, SUB, VTT, etc.).
     *
     * @usecase Bound to the add-subtitle dialog.
     *
     * @return Name filter strings suitable for Qt Quick FileDialog.
     *
     * @sideeffects None.
     * @thread May be called on the main Qt/QML thread.
     */
    Q_INVOKABLE QStringList subtitleNameFilters() const;

    /**
     * @brief Returns QFileDialog name filters for external audio tracks.
     *
     * @details Entries follow Qt's name-filter format and cover common
     *          audio-only formats (MP3, FLAC, AAC, OGG, WAV, etc.).
     *
     * @usecase Bound to the add-audio dialog.
     *
     * @return Name filter strings suitable for Qt Quick FileDialog.
     *
     * @sideeffects None.
     * @thread May be called on the main Qt/QML thread.
     */
    Q_INVOKABLE QStringList audioNameFilters() const;

    /**
     * @brief Converts QML-provided URLs to local filesystem paths or remote URL strings.
     *
     * @details Handles file:// URLs (extracts the local path), remote
     *          http(s) URLs (passed through as-is), and relative paths.
     *          Network paths (smb://, etc.) are preserved for mpv.
     *
     * @usecase Used by file dialogs and drag/drop handlers before opening media.
     *
     * @param urls URLs supplied by Qt Quick controls (from FileDialog, DropArea, etc.).
     * @return mpv-ready local paths or encoded remote URL strings.
     *
     * @sideeffects None.
     * @thread May be called on the main Qt/QML thread.
     */
    Q_INVOKABLE QStringList urlsToPaths(const QList<QUrl>& urls) const;

    /**
     * @brief Recursively lists playable media files in a local folder.
     *
     * @details Scans the given directory and its subdirectories for files
     *          whose extensions match the supported media format list.
     *          Results are sorted alphabetically.
     *
     * @usecase Used when the user opens or adds a folder via QML.
     *
     * @param folder Folder URL selected by QML (must be a local file:// URL).
     * @return Sorted list of absolute local media paths.
     *
     * @sideeffects Iterates the filesystem subtree; may be slow for deep or
     *             network-mounted directories.
     * @thread Should be called from the main thread; prefer invoking via
     *         a worker thread for large directory trees.
     */
    Q_INVOKABLE QStringList listMediaFiles(const QUrl& folder) const;

    /**
     * @brief Checks whether a path is a supported subtitle file by file extension.
     *
     * @usecase Used by drag/drop to decide whether to attach or play a dropped file.
     *
     * @param path Local path or URL-like string to inspect.
     * @return true when the file extension matches a known subtitle format.
     *
     * @sideeffects None.
     * @thread May be called from any thread (no mutable state accessed).
     */
    Q_INVOKABLE bool isSubtitle(const QString& path) const;

    /**
     * @brief Checks whether a path is supported media by file extension.
     *
     * @usecase Used by validation, playlist, and filtering flows.
     *
     * @param path Local path or URL-like string to inspect.
     * @return true when the file extension matches a known media format.
     *
     * @sideeffects None.
     * @thread May be called from any thread (no mutable state accessed).
     */
    Q_INVOKABLE bool isMedia(const QString& path) const;

    /**
     * @brief Returns a user-facing display name for a path or URL.
     *
     * @details For local files, returns the file name (with extension).
     *          For remote URLs, returns the host and path portion.
     *          Falls back to the input string when parsing fails.
     *
     * @usecase Used by playlist rows and file previews in QML.
     *
     * @param path Local path or remote URL to format.
     * @return Human-readable display name, or the original path as fallback.
     *
     * @sideeffects None.
     * @thread May be called from any thread (no mutable state accessed).
     */
    Q_INVOKABLE QString displayName(const QString& path) const;

    /**
     * @brief Selects an icon identifier for a given path based on its type.
     *
     * @details Maps known file categories (video, audio, subtitle, image,
     *          playlist) to icon names from the application icon theme.
     *
     * @usecase Used by playlist and file-related UI rows to show type icons.
     *
     * @param path Local path or remote URL to classify.
     * @return Icon name string suitable for use with QML Icon or Image source.
     *
     * @sideeffects None.
     * @thread May be called from any thread (no mutable state accessed).
     */
    Q_INVOKABLE QString iconNameForPath(const QString& path) const;

    /**
     * @brief Validates whether a user-entered URL is supported by the player.
     *
     * @details Checks the URL scheme against a whitelist of supported
     *          protocols (http, https, ftp, rtsp, rtmp, mms, etc.) and
     *          validates the overall URL structure.
     *
     * @usecase Used by UrlDialog to enable/disable the accept button.
     *
     * @param input User-entered URL text from QML input field.
     * @return true when the URL scheme and format are recognized as playable.
     *
     * @sideeffects None.
     * @thread May be called on the main Qt/QML thread.
     */
    Q_INVOKABLE bool isSupportedUrl(const QString& input) const;

    /**
     * @brief Reads plain text from the system clipboard.
     *
     * @usecase Used by URL dialogs to prefill from copied links.
     *
     * @return Clipboard text or an empty string.
     *
     * @sideeffects Reads application clipboard state.
     * @thread Must be called on the main Qt/QML thread.
     */
    Q_INVOKABLE QString clipboardText() const;

    /**
     * @brief Opens the platform file manager at a path location.
     *
     * @usecase Used by playlist/context actions that reveal local media.
     *
     * @param path Local file or folder path to reveal/open.
     *
     * @sideeffects Launches an external platform file-manager process or URL handler.
     * @thread Must be called on the main Qt/QML thread.
     */
    Q_INVOKABLE void openLocation(const QString& path) const;
};
