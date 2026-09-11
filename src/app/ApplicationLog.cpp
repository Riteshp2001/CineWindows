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
 * Last modified: 2026-09-11
 * Modified by: Ritesh Pandit
 */

#include "app/ApplicationLog.h"

#include "app/LoggingCategories.h"
#include "utils/PathUtils.h"

#include <qtlogger/qtlogger.h>

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>

namespace {
constexpr int MaximumLogFileBytes = 5 * 1024 * 1024;
constexpr int MaximumLogFileCount = 5;
}

ApplicationLog::ApplicationLog()
{
    const QString requestedPath = PathUtils::logFilePath();
    const QFileInfo logFile(requestedPath);
    if (logFile.dir().exists())
        m_filePath = requestedPath;

    const auto rotationOptions = QtLogger::RotatingFileSink::RotationOnStartup
        | QtLogger::RotatingFileSink::RotationDaily
        | QtLogger::RotatingFileSink::Compression;
    gQtLogger.configure(m_filePath, MaximumLogFileBytes, MaximumLogFileCount,
                        rotationOptions, false);
    m_configured = true;

    if (m_filePath.isEmpty())
        qCWarning(cineAppLog) << "Log directory is unavailable; using console logging only";
    else
        qCInfo(cineAppLog).noquote() << "Logging to" << QDir::toNativeSeparators(m_filePath);
}

ApplicationLog::~ApplicationLog()
{
    if (!m_configured)
        return;

    qCInfo(cineAppLog) << "CineWindows shutting down";
    gQtLogger.flush();
}

QString ApplicationLog::filePath() const
{
    return m_filePath;
}
