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

#include "services/YouTubeSearchService.h"

#include "app/LoggingCategories.h"
#include "utils/PathUtils.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace
{
constexpr qsizetype MaximumSearchOutputBytes = 4 * 1024 * 1024;
constexpr qsizetype MaximumSearchErrorBytes = 256 * 1024;
constexpr qsizetype MaximumSearchQueryCharacters = 256;
constexpr int SearchTimeoutMs = 45 * 1000;

bool appendBounded(QByteArray& destination, const QByteArray& data, qsizetype maximumBytes)
{
    if (data.size() > maximumBytes - destination.size())
        return false;
    destination.append(data);
    return true;
}
}

/**
 * @brief Constructs a YouTubeSearchService and wires yt-dlp process signals.
 * @param parent Optional QObject parent for Qt ownership.
 */
YouTubeSearchService::YouTubeSearchService(QObject* parent)
    : QObject(parent)
{
    m_timeoutTimer.setSingleShot(true);
    m_timeoutTimer.setInterval(SearchTimeoutMs);
    connect(&m_timeoutTimer, &QTimer::timeout, this, [this] {
        if (m_process.state() != QProcess::NotRunning)
        {
            m_timedOut = true;
            m_process.kill();
        }
    });
    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this] {
        if (!appendBounded(m_standardOutput, m_process.readAllStandardOutput(), MaximumSearchOutputBytes))
        {
            m_outputTooLarge = true;
            m_process.kill();
        }
    });
    connect(&m_process, &QProcess::readyReadStandardError, this, [this] {
        if (!appendBounded(m_standardError, m_process.readAllStandardError(), MaximumSearchErrorBytes))
        {
            m_outputTooLarge = true;
            m_process.kill();
        }
    });
    // Handle yt-dlp process completion: parse JSON output into result list
    connect(&m_process, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus status) {
        m_timeoutTimer.stop();
        m_outputTooLarge = m_outputTooLarge
            || !appendBounded(m_standardOutput, m_process.readAllStandardOutput(), MaximumSearchOutputBytes)
            || !appendBounded(m_standardError, m_process.readAllStandardError(), MaximumSearchErrorBytes);
        setBusy(false);
        // If cancellation was requested, ignore the result
        if (m_cancelRequested)
        {
            m_cancelRequested = false;
            return;
        }
        if (m_timedOut)
        {
            setErrorString(tr("YouTube search timed out"));
            return;
        }
        if (m_outputTooLarge)
        {
            setErrorString(tr("YouTube search returned too much data"));
            return;
        }
        // Check for process errors and non-zero exit codes
        if (status != QProcess::NormalExit || exitCode != 0)
        {
            QString error = QString::fromUtf8(m_standardError).trimmed();
            if (error.isEmpty())
                error = tr("yt-dlp exited with code %1").arg(exitCode);
            setErrorString(error);
            return;
        }
        // Parse the JSON output from yt-dlp's --dump-single-json
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(m_standardOutput, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject())
        {
            setErrorString(tr("YouTube returned an invalid response"));
            return;
        }
        // Extract each video entry from the JSON response
        QVariantList results;
        for (const QJsonValue& value : document.object().value(QStringLiteral("entries")).toArray())
        {
            const QJsonObject entry = value.toObject();
            const QString id = entry.value(QStringLiteral("id")).toString();
            if (id.isEmpty())
                continue;
            const QString thumbnail = QStringLiteral("https://i.ytimg.com/vi/%1/mqdefault.jpg").arg(id);

            // Use channel name, falling back to uploader if channel is empty
            QString channel = entry.value(QStringLiteral("channel")).toString();
            if (channel.isEmpty())
                channel = entry.value(QStringLiteral("uploader")).toString();
            const QString liveStatus = entry.value(QStringLiteral("live_status")).toString();
            results.append(QVariantMap{{QStringLiteral("id"), id},
                                        {QStringLiteral("title"), entry.value(QStringLiteral("title")).toString()},
                                        {QStringLiteral("channel"), channel},
                                        {QStringLiteral("duration"), entry.value(QStringLiteral("duration")).toDouble()},
                                        {QStringLiteral("viewCount"), entry.value(QStringLiteral("view_count")).toDouble()},
                                        {QStringLiteral("uploadDate"), entry.value(QStringLiteral("upload_date")).toString()},
                                        {QStringLiteral("timestamp"), entry.value(QStringLiteral("timestamp")).toDouble()},
                                        {QStringLiteral("live"), liveStatus == QStringLiteral("is_live")},
                                        {QStringLiteral("thumbnail"), thumbnail},
                                        {QStringLiteral("url"), QStringLiteral("https://www.youtube.com/watch?v=%1").arg(id)}});
        }
        // Store results and notify QML listeners
        m_results = results;
        setErrorString({});
        Q_EMIT resultsChanged();
    });
    // Handle process-level errors like failed to start
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (m_cancelRequested)
            return;
        if (error == QProcess::FailedToStart)
        {
            m_timeoutTimer.stop();
            setBusy(false);
            setErrorString(tr("Could not start yt-dlp: %1").arg(m_process.errorString()));
        }
    });
}

QVariantList YouTubeSearchService::results() const { return m_results; }
bool YouTubeSearchService::busy() const { return m_busy; }
QString YouTubeSearchService::errorString() const { return m_errorString; }

/**
 * @brief Initiates a YouTube search by launching yt-dlp with the given query.
 * @param query The search string to send to YouTube.
 */
void YouTubeSearchService::search(const QString& query)
{
    const QString searchQuery = query.trimmed();
    // Ignore empty queries or searches while already busy
    if (searchQuery.isEmpty() || m_busy)
        return;
    if (searchQuery.size() > MaximumSearchQueryCharacters)
    {
        setErrorString(tr("YouTube search is too long"));
        return;
    }
    // Locate the yt-dlp executable; abort if not found
    const QString executable = PathUtils::mediaToolPath(QStringLiteral("yt-dlp"));
    if (executable.isEmpty())
    {
        setErrorString(tr("yt-dlp was not found"));
        return;
    }
    // Clear previous results and mark the service as busy
    m_results.clear();
    Q_EMIT resultsChanged();
    setErrorString({});
    setBusy(true);
    m_cancelRequested = false;
    m_timedOut = false;
    m_outputTooLarge = false;
    m_standardOutput.clear();
    m_standardError.clear();
    // Launch yt-dlp with flags for JSON output and search mode
    m_process.start(executable, {QStringLiteral("--ignore-config"), QStringLiteral("--flat-playlist"),
                                  QStringLiteral("--dump-single-json"), QStringLiteral("--skip-download"),
                                  QStringLiteral("--ignore-errors"), QStringLiteral("--no-warnings"),
                                  QStringLiteral("ytsearch12:%1").arg(searchQuery)});
    m_timeoutTimer.start();
}

/**
 * @brief Cancels the running search by killing the yt-dlp process.
 */
void YouTubeSearchService::cancel()
{
    // Kill the process if it is still running
    if (m_process.state() != QProcess::NotRunning)
    {
        m_cancelRequested = true;
        m_timeoutTimer.stop();
        m_process.kill();
    }
}

/**
 * @brief Sets the busy state and emits the busyChanged signal if changed.
 * @param busy New busy state value.
 */
void YouTubeSearchService::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    Q_EMIT busyChanged();
}

/**
 * @brief Sets the error string and emits the errorStringChanged signal if changed.
 * @param error New error description text.
 */
void YouTubeSearchService::setErrorString(const QString& error)
{
    if (m_errorString == error)
        return;
    m_errorString = error;
    if (!error.isEmpty())
        qCWarning(cineServiceLog).noquote() << "YouTube search:" << error;
    Q_EMIT errorStringChanged();
}
