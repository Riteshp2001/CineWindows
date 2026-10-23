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

#include "utils/PathUtils.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QtGlobal>

#include <algorithm>
#include <utility>

namespace {
/**
 * @brief Ensures a directory exists, creating it if necessary.
 * @param path Directory path to ensure.
 * @return The same path after ensuring the directory exists.
 */
QString ensureDir(const QString& path)
{
    QDir().mkpath(path);
    return path;
}

/**
 * @brief Returns the roaming configuration base directory.
 * @details On Windows this reads the APPDATA environment variable; on other
 *          platforms it falls back to the standard config location.
 */
QString roamingConfigBase()
{
#ifdef Q_OS_WIN
    const QString appData = qEnvironmentVariable("APPDATA");
    if (!appData.isEmpty())
    {
        return QDir::fromNativeSeparators(appData);
    }
#endif

    const QString config = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return config.isEmpty() ? QDir::homePath() + QStringLiteral("/.config") : config;
}

/**
 * @brief Creates standard mpv subdirectory structure under the given path.
 * @param path Base mpv configuration directory.
 */
void ensureMpvSubdirs(const QString& path)
{
    const QDir dir(path);
    ensureDir(dir.filePath(QStringLiteral("scripts")));
    ensureDir(dir.filePath(QStringLiteral("script-opts")));
    ensureDir(dir.filePath(QStringLiteral("shaders")));
    ensureDir(dir.filePath(QStringLiteral("fonts")));
}
} // namespace

namespace PathUtils {

/**
 * @brief Resolves a bundled media tool before falling back to the system PATH.
 * @param executableName Tool name with or without the Windows .exe suffix.
 * @return Absolute executable path, or an empty string when the tool is unavailable.
 */
QString mediaToolPath(const QString& executableName)
{
    QString toolName = executableName.trimmed();
    if (toolName.isEmpty())
        return {};
#ifdef Q_OS_WIN
    if (!toolName.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive))
        toolName += QStringLiteral(".exe");
#endif

    const QString appDir = QCoreApplication::applicationDirPath();
    QStringList searchDirs;

    // Check environment override first, then bundling locations, finally PATH
    const QString overrideDir = qEnvironmentVariable("CINEWINDOWS_MEDIA_TOOLS_DIR");
    if (!overrideDir.isEmpty())
        searchDirs.append(QDir::fromNativeSeparators(overrideDir));
    searchDirs.append(managedToolsDir());
    searchDirs.append(appDir);
    searchDirs.append(QDir(appDir).filePath(QStringLiteral("tools")));
    searchDirs.append(QDir(appDir).filePath(QStringLiteral("../.deps/media-tools/bin")));

    for (const QString& dir : std::as_const(searchDirs))
    {
        const QFileInfo candidate(QDir(dir).filePath(toolName));
        if (candidate.isFile())
            return candidate.absoluteFilePath();
    }
    return QStandardPaths::findExecutable(toolName);
}

/**
 * @brief Returns the application configuration directory path.
 * @return Absolute path to the CineWindows config directory.
 */
QString appConfigDir()
{
    const QString overridePath = qEnvironmentVariable("CINEWINDOWS_DATA_DIR");
    if (!overridePath.isEmpty())
    {
        return ensureDir(QDir::fromNativeSeparators(overridePath));
    }
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return ensureDir(base.isEmpty() ? QDir::homePath() + QStringLiteral("/.config/CineWindows") : base);
}

QString managedToolsDir()
{
    return ensureDir(QDir(appConfigDir()).filePath(QStringLiteral("tools")));
}

/**
 * @brief Returns the CineWindows-specific mpv configuration directory.
 * @details A "portable_config" folder next to the executable wins (mpv's portable
 *          convention), then CINEWINDOWS_DATA_DIR/mpv, then %APPDATA%/CineWindows/mpv
 *          (~/.config/CineWindows/mpv elsewhere). The standalone mpv folder is never used.
 * @return Absolute path to the CineWindows mpv config directory.
 */
QString mpvConfigDir()
{
    QString path = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("portable_config"));
    if (!QFileInfo(path).isDir())
    {
        const QString overridePath = qEnvironmentVariable("CINEWINDOWS_DATA_DIR");
        path = overridePath.isEmpty()
            ? QDir(roamingConfigBase()).filePath(QStringLiteral("CineWindows/mpv"))
            : QDir(QDir::fromNativeSeparators(overridePath)).filePath(QStringLiteral("mpv"));
    }
    path = QDir::cleanPath(path);
    ensureDir(path);
    ensureMpvSubdirs(path);
    return path;
}

/**
 * @brief Returns the standalone mpv player's configuration directory without creating it.
 * @return Absolute path such as %APPDATA%/mpv, used only as an explicit import source.
 */
QString standardMpvConfigDir()
{
    return QDir(roamingConfigBase()).filePath(QStringLiteral("mpv"));
}

/** @brief Returns the folder older releases used as MPV_HOME, without creating it. */
QString legacyMpvConfigDir()
{
    return QDir(roamingConfigBase()).filePath(QStringLiteral("cine"));
}

/**
 * @brief Sets up the mpv environment by configuring MPV_HOME and PATH.
 * @details Points MPV_HOME to the CineWindows mpv config directory and prepends
 *          the yt-dlp tool directory to PATH if not already present.
 */
void setupMpvEnvironment()
{
    qputenv("MPV_HOME", QDir::toNativeSeparators(mpvConfigDir()).toLocal8Bit());

    // Locate yt-dlp and ensure its directory is on PATH for mpv's lua scripts
    const QString ytDlp = mediaToolPath(QStringLiteral("yt-dlp"));
    if (ytDlp.isEmpty())
        return;

    const QString toolDir = QFileInfo(ytDlp).absolutePath();
    const QChar separator = QDir::listSeparator();
    QStringList pathEntries = QString::fromLocal8Bit(qgetenv("PATH")).split(separator, Qt::SkipEmptyParts);
    const Qt::CaseSensitivity sensitivity =
#ifdef Q_OS_WIN
        Qt::CaseInsensitive;
#else
        Qt::CaseSensitive;
#endif
    const bool alreadyPresent = std::any_of(pathEntries.cbegin(), pathEntries.cend(), [&](const QString& entry) {
        return QDir::cleanPath(entry).compare(QDir::cleanPath(toolDir), sensitivity) == 0;
    });
    if (!alreadyPresent)
    {
        pathEntries.prepend(toolDir);
        qputenv("PATH", QDir::toNativeSeparators(pathEntries.join(separator)).toLocal8Bit());
    }
}

/**
 * @brief Returns the directory path where screenshots are saved.
 * @return Absolute path to the screenshots directory.
 */
QString screenshotsDir()
{
    const QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    return ensureDir((pictures.isEmpty() ? QDir::homePath() : pictures) + QStringLiteral("/CineWindows"));
}

/**
 * @brief Returns the file path for the session persistence file.
 * @return Absolute path to the session JSON file.
 */
QString sessionFilePath()
{
    return appConfigDir() + QStringLiteral("/last-session.json");
}

/** @brief Returns the SQLite database used by sessions and the media library. */
QString libraryDatabasePath()
{
    return appConfigDir() + QStringLiteral("/cinewindows.sqlite3");
}

/** @brief Returns the persistent media thumbnail cache directory. */
QString thumbnailCacheDir()
{
    const QString cache = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    return ensureDir(QDir(cache.isEmpty() ? appConfigDir() : cache).filePath(QStringLiteral("thumbnails")));
}

QString logFilePath()
{
    const QString overridePath = qEnvironmentVariable("CINEWINDOWS_DATA_DIR");
    if (!overridePath.isEmpty())
    {
        return QDir(ensureDir(QDir(QDir::fromNativeSeparators(overridePath))
                                  .filePath(QStringLiteral("logs"))))
            .filePath(QStringLiteral("cinewindows.log"));
    }
    const QString data = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const QString root = data.isEmpty() ? appConfigDir() : data;
    return QDir(ensureDir(QDir(root).filePath(QStringLiteral("logs"))))
        .filePath(QStringLiteral("cinewindows.log"));
}

} // namespace PathUtils
