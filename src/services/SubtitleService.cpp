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

#include "services/SubtitleService.h"

#include "player/CineMpvItem.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QUrlQuery>

namespace
{
constexpr qint64 MaximumSearchResponseBytes = 2 * 1024 * 1024;
constexpr qint64 MaximumSubtitleBytes = 10 * 1024 * 1024;

bool isHttpsUrl(const QUrl& url)
{
    return url.isValid() && !url.host().isEmpty()
        && url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0;
}

void abortIfOversized(QNetworkReply* reply, qint64 maximumBytes)
{
    QObject::connect(reply, &QNetworkReply::downloadProgress, reply,
                     [reply, maximumBytes](qint64 received, qint64 total) {
        if (received > maximumBytes || total > maximumBytes)
        {
            reply->setProperty("cineResponseTooLarge", true);
            reply->abort();
        }
    });
}

QString safeSubtitleSuffix(const QString& fileName)
{
    static const QSet<QString> suffixes{QStringLiteral("ass"), QStringLiteral("smi"),
                                        QStringLiteral("srt"), QStringLiteral("ssa"),
                                        QStringLiteral("sub"), QStringLiteral("vtt")};
    const QString suffix = QFileInfo(fileName).suffix().toLower();
    return suffixes.contains(suffix) ? suffix : QStringLiteral("srt");
}
}

SubtitleService::SubtitleService(QObject* parent)
    : QObject(parent)
{
    m_apiKey = QSettings().value(QStringLiteral("integrations/wyzieApiKey")).toString();
}

CineMpvItem* SubtitleService::player() const { return m_player; }
void SubtitleService::setPlayer(CineMpvItem* player)
{
    if (m_player == player)
        return;
    m_player = player;
    Q_EMIT playerChanged();
}

QString SubtitleService::apiKey() const { return m_apiKey; }
void SubtitleService::setApiKey(const QString& apiKey)
{
    const QString trimmed = apiKey.trimmed();
    if (m_apiKey == trimmed)
        return;
    m_apiKey = trimmed;
    QSettings().setValue(QStringLiteral("integrations/wyzieApiKey"), m_apiKey);
    Q_EMIT apiKeyChanged();
}

QVariantList SubtitleService::results() const { return m_results; }
bool SubtitleService::busy() const { return m_busy; }
QString SubtitleService::statusMessage() const { return m_statusMessage; }

QNetworkRequest SubtitleService::request(const QUrl& url) const
{
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "CineWindows v1");
    request.setRawHeader("Accept", "application/json");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(15000);
    return request;
}

void SubtitleService::search(const QString& languages, const QString& query, int season, int episode)
{
    if (m_busy)
        return;
    if (m_apiKey.isEmpty())
    {
        setStatusMessage(tr("Enter a Wyzie API key to search subtitles"));
        return;
    }

    QString searchQuery = query.trimmed();

    // If query is empty, try to extract title from current video filename
    if (searchQuery.isEmpty() && m_player && !m_player->currentPath().isEmpty())
    {
        QString fileName = QFileInfo(m_player->currentPath()).completeBaseName();
        // Clean up common release tags to get a cleaner title
        fileName.remove(QRegularExpression(QStringLiteral("\\.(1080|720|2160|480)[pi]?")));
        fileName.remove(QRegularExpression(QStringLiteral("\\.(WEB-?DL|BluRay|BRRip|HDRip|DVDRip|WEBRip|HDR|HDR10|HEVC|x264|x265|AAC|DTS|FLAC)"), QRegularExpression::CaseInsensitiveOption));
        fileName.remove(QRegularExpression(QStringLiteral("\\[.*?\\]")));  // Remove bracket tags
        fileName.replace(QRegularExpression(QStringLiteral("[._\\-]+")), QStringLiteral(" "));
        fileName = fileName.trimmed();
        searchQuery = fileName;
    }

    if (searchQuery.isEmpty())
    {
        setStatusMessage(tr("Enter a movie/show name or TMDB/IMDB ID to search"));
        return;
    }

    QUrl url(QStringLiteral("https://sub.wyzie.io/search"));
    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QStringLiteral("id"), searchQuery);
    urlQuery.addQueryItem(QStringLiteral("key"), m_apiKey);

    if (!languages.trimmed().isEmpty())
        urlQuery.addQueryItem(QStringLiteral("language"), languages.trimmed());

    if (season > 0 && episode > 0)
    {
        urlQuery.addQueryItem(QStringLiteral("season"), QString::number(season));
        urlQuery.addQueryItem(QStringLiteral("episode"), QString::number(episode));
    }

    url.setQuery(urlQuery);

    setBusy(true);
    setStatusMessage(tr("Searching for \"%1\"...").arg(searchQuery));

    QNetworkReply* reply = m_network.get(request(url));
    abortIfOversized(reply, MaximumSearchResponseBytes);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        setBusy(false);
        if (reply->property("cineResponseTooLarge").toBool())
        {
            reply->deleteLater();
            setStatusMessage(tr("Subtitle search response was too large"));
            return;
        }
        const bool ok = reply->error() == QNetworkReply::NoError && reply->isOpen();
        if (!ok)
        {
            const QString error = reply->errorString();
            reply->deleteLater();
            setStatusMessage(tr("Subtitle search failed: %1").arg(error));
            return;
        }
        const QByteArray data = reply->readAll();
        reply->deleteLater();
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isArray())
        {
            setStatusMessage(tr("Invalid subtitle search response"));
            return;
        }
        QVariantList results;

        const QJsonArray subtitles = document.array();
        for (const QJsonValue& value : subtitles)
        {
            const QJsonObject sub = value.toObject();
            results.append(QVariantMap{
                {QStringLiteral("id"), sub.value(QStringLiteral("id")).toString()},
                {QStringLiteral("url"), sub.value(QStringLiteral("url")).toString()},
                {QStringLiteral("flagUrl"), sub.value(QStringLiteral("flagUrl")).toString()},
                {QStringLiteral("format"), sub.value(QStringLiteral("format")).toString()},
                {QStringLiteral("encoding"), sub.value(QStringLiteral("encoding")).toString()},
                {QStringLiteral("display"), sub.value(QStringLiteral("display")).toString()},
                {QStringLiteral("language"), sub.value(QStringLiteral("language")).toString()},
                {QStringLiteral("media"), sub.value(QStringLiteral("media")).toString()},
                {QStringLiteral("isHearingImpaired"), sub.value(QStringLiteral("isHearingImpaired")).toBool()},
                {QStringLiteral("source"), sub.value(QStringLiteral("source")).toString()},
                {QStringLiteral("release"), sub.value(QStringLiteral("release")).toString()},
                {QStringLiteral("fileName"), sub.value(QStringLiteral("fileName")).toString()},
                {QStringLiteral("origin"), sub.value(QStringLiteral("origin")).toString()}
            });
        }

        m_results = results;
        Q_EMIT resultsChanged();
        setStatusMessage(results.isEmpty() ? tr("No matching subtitles found")
                                           : tr("Found %1 subtitle matches").arg(results.size()));
    });
}

void SubtitleService::download(const QString& subtitleUrl, const QString& fileName)
{
    if (m_busy || subtitleUrl.isEmpty())
        return;

    const QUrl url(subtitleUrl);
    if (!isHttpsUrl(url))
    {
        setStatusMessage(tr("Subtitle download link is not secure"));
        return;
    }

    setBusy(true);
    setStatusMessage(tr("Downloading subtitle..."));

    QNetworkReply* fileReply = m_network.get(request(url));
    abortIfOversized(fileReply, MaximumSubtitleBytes);
    connect(fileReply, &QNetworkReply::finished, this, [this, fileReply, fileName] {
        if (fileReply->property("cineResponseTooLarge").toBool())
        {
            fileReply->deleteLater();
            setBusy(false);
            setStatusMessage(tr("Subtitle file was too large"));
            return;
        }
        const bool fileOk = fileReply->error() == QNetworkReply::NoError && fileReply->isOpen();
        if (!fileOk)
        {
            const QString fileError = fileReply->errorString();
            fileReply->deleteLater();
            setBusy(false);
            setStatusMessage(tr("Subtitle download failed: %1").arg(fileError));
            return;
        }
        const QByteArray contents = fileReply->readAll();
        fileReply->deleteLater();
        setBusy(false);

        const QString directory = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
                                      .filePath(QStringLiteral("subtitles"));
        if (!QDir().mkpath(directory))
        {
            setStatusMessage(tr("Could not create the subtitle cache"));
            return;
        }

        QTemporaryFile file(QDir(directory).filePath(
            QStringLiteral("subtitle-XXXXXX.%1").arg(safeSubtitleSuffix(fileName))));
        if (!file.open() || file.write(contents) != contents.size() || !file.flush())
        {
            setStatusMessage(tr("Could not save the downloaded subtitle"));
            return;
        }
        const QString path = file.fileName();
        file.setAutoRemove(false);
        file.close();

        if (m_player)
            m_player->addSubtitle(path);
        setStatusMessage(tr("Subtitle downloaded and added"));
        Q_EMIT subtitleReady(path);
    });
}

void SubtitleService::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    Q_EMIT busyChanged();
}

void SubtitleService::setStatusMessage(const QString& message)
{
    if (m_statusMessage == message)
        return;
    m_statusMessage = message;
    Q_EMIT statusMessageChanged();
}
