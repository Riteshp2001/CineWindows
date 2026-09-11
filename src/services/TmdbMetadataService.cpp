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

#include "services/TmdbMetadataService.h"

#include <QDateTime>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLocale>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>
#include <QUrlQuery>

#include <algorithm>

namespace {
constexpr qint64 MaximumResponseBytes = 2 * 1024 * 1024;
constexpr qint64 MetadataCacheLifetimeMs = 30LL * 24 * 60 * 60 * 1000;

QVariantMap searchDescriptor(const QVariantMap& item)
{
    QString title = QFileInfo(item.value(QStringLiteral("path")).toString()).completeBaseName();
    static const QRegularExpression yearPattern(QStringLiteral("(?:^|[ ._(-])((?:19|20)\\d{2})(?=$|[ ._)])"));
    const QRegularExpressionMatch yearMatch = yearPattern.match(title);
    const int year = yearMatch.hasMatch() ? yearMatch.captured(1).toInt() : 0;

    title.remove(QRegularExpression(QStringLiteral("\\[[^]]*\\]|\\([^)]*\\)")));
    title.remove(QRegularExpression(QStringLiteral("\\bS\\d{1,2}(?:E\\d{1,3})?\\b"),
                                    QRegularExpression::CaseInsensitiveOption));
    title.replace(QRegularExpression(QStringLiteral("[._-]+")), QStringLiteral(" "));
    title.remove(QRegularExpression(
        QStringLiteral("\\b(?:2160p|1080p|720p|480p|web[ .-]?dl|webrip|bluray|brrip|dvdrip|hdtv|hdr10?|"
                       "remux|x26[45]|h26[45]|hevc|av1|aac|dts|flac|proper|repack)\\b.*$"),
        QRegularExpression::CaseInsensitiveOption));
    title = title.simplified();

    QVariantMap descriptor = item;
    descriptor.insert(QStringLiteral("searchTitle"), title);
    descriptor.insert(QStringLiteral("searchYear"), year);
    return descriptor;
}

int resultYear(const QJsonObject& result)
{
    QString date = result.value(QStringLiteral("release_date")).toString();
    if (date.isEmpty())
        date = result.value(QStringLiteral("first_air_date")).toString();
    return date.left(4).toInt();
}

bool isSupportedResult(const QJsonObject& result)
{
    const QString type = result.value(QStringLiteral("media_type")).toString();
    return type == QStringLiteral("movie") || type == QStringLiteral("tv");
}

QJsonObject bestResult(const QJsonArray& results, int expectedYear)
{
    QJsonObject fallback;
    for (const QJsonValue& value : results)
    {
        const QJsonObject result = value.toObject();
        if (!isSupportedResult(result))
            continue;
        if (fallback.isEmpty())
            fallback = result;
        if (expectedYear > 0 && resultYear(result) == expectedYear)
            return result;
    }
    return fallback;
}

QString mediaCategory(const QJsonObject& result)
{
    const QJsonArray genreIds = result.value(QStringLiteral("genre_ids")).toArray();
    const bool animation = std::any_of(genreIds.cbegin(), genreIds.cend(), [](const QJsonValue& genre) {
        return genre.toInt() == 16;
    });
    if (animation && result.value(QStringLiteral("original_language")).toString() == QStringLiteral("ja"))
        return QStringLiteral("anime");
    return result.value(QStringLiteral("media_type")).toString();
}

QVariantMap normalizedMetadata(const QJsonObject& result)
{
    const QString mediaType = result.value(QStringLiteral("media_type")).toString();
    const QString titleKey = mediaType == QStringLiteral("movie") ? QStringLiteral("title") : QStringLiteral("name");
    QString artworkPath = result.value(QStringLiteral("backdrop_path")).toString();
    QString artworkSize = QStringLiteral("w780");
    if (artworkPath.isEmpty())
    {
        artworkPath = result.value(QStringLiteral("poster_path")).toString();
        artworkSize = QStringLiteral("w500");
    }

    return {{QStringLiteral("id"), result.value(QStringLiteral("id")).toInteger()},
            {QStringLiteral("type"), mediaCategory(result)},
            {QStringLiteral("title"), result.value(titleKey).toString()},
            {QStringLiteral("overview"), result.value(QStringLiteral("overview")).toString()},
            {QStringLiteral("year"), resultYear(result)},
            {QStringLiteral("rating"), result.value(QStringLiteral("vote_average")).toDouble()},
            {QStringLiteral("artworkUrl"), artworkPath.isEmpty()
                 ? QString()
                 : QStringLiteral("https://image.tmdb.org/t/p/%1%2").arg(artworkSize, artworkPath)}};
}
} // namespace

TmdbMetadataService::TmdbMetadataService(QObject* parent)
    : QObject(parent)
{
    QSettings settings;
    m_apiToken = settings.value(QStringLiteral("integrations/tmdbApiToken")).toString();
    m_enabled = settings.value(QStringLiteral("integrations/tmdbEnabled"), false).toBool();
}

MediaLibraryService* TmdbMetadataService::library() const { return m_library.data(); }
void TmdbMetadataService::setLibrary(MediaLibraryService* library)
{
    if (m_library == library)
        return;
    if (m_busy)
        cancel();
    m_library = library;
    Q_EMIT libraryChanged();
}

QString TmdbMetadataService::apiToken() const { return m_apiToken; }
void TmdbMetadataService::setApiToken(const QString& token)
{
    const QString trimmed = token.trimmed();
    if (m_apiToken == trimmed)
        return;
    if (m_busy)
        cancel();
    m_apiToken = trimmed;
    QSettings().setValue(QStringLiteral("integrations/tmdbApiToken"), m_apiToken);
    Q_EMIT apiTokenChanged();
}

bool TmdbMetadataService::enabled() const { return m_enabled; }
void TmdbMetadataService::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    QSettings().setValue(QStringLiteral("integrations/tmdbEnabled"), enabled);
    if (!enabled && m_busy)
        cancel();
    Q_EMIT enabledChanged();
}

bool TmdbMetadataService::busy() const { return m_busy; }
int TmdbMetadataService::completed() const { return m_completed; }
int TmdbMetadataService::total() const { return m_total; }
QString TmdbMetadataService::statusMessage() const { return m_statusMessage; }

QNetworkRequest TmdbMetadataService::requestFor(const QVariantMap& item) const
{
    QUrl url(QStringLiteral("https://api.themoviedb.org/3/search/multi"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("query"), item.value(QStringLiteral("searchTitle")).toString());
    query.addQueryItem(QStringLiteral("include_adult"), QStringLiteral("false"));
    query.addQueryItem(QStringLiteral("language"), QLocale::system().name().replace(QLatin1Char('_'), QLatin1Char('-')));
    if (!m_apiToken.startsWith(QStringLiteral("eyJ")))
        query.addQueryItem(QStringLiteral("api_key"), m_apiToken);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent", "CineWindows/0.0.1");
    if (m_apiToken.startsWith(QStringLiteral("eyJ")))
        request.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + m_apiToken.toUtf8());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(15000);
    return request;
}

void TmdbMetadataService::refresh(bool force)
{
    if (m_busy)
        return;
    if (!m_enabled)
    {
        setStatusMessage(tr("TMDB metadata is disabled"));
        return;
    }
    if (m_apiToken.isEmpty())
    {
        setStatusMessage(tr("Enter a TMDB API token or key"));
        return;
    }
    if (!m_library)
    {
        setStatusMessage(tr("Media library is not available"));
        return;
    }

    m_queue.clear();
    const qint64 cacheCutoff = QDateTime::currentMSecsSinceEpoch() - MetadataCacheLifetimeMs;
    for (const QVariant& value : m_library->library())
    {
        const QVariantMap item = value.toMap();
        if (item.value(QStringLiteral("kind")).toString() != QStringLiteral("local")
            || item.value(QStringLiteral("mediaId")).toLongLong() <= 0)
            continue;
        if (!force && item.value(QStringLiteral("metadataUpdatedAt")).toLongLong() >= cacheCutoff)
            continue;
        const QVariantMap descriptor = searchDescriptor(item);
        if (!descriptor.value(QStringLiteral("searchTitle")).toString().isEmpty())
            m_queue.enqueue(descriptor);
    }

    m_completed = 0;
    m_total = static_cast<int>(m_queue.size());
    m_updated = 0;
    m_failed = 0;
    Q_EMIT progressChanged();
    if (m_queue.isEmpty())
    {
        setStatusMessage(tr("Library metadata is up to date"));
        return;
    }
    setBusy(true);
    setStatusMessage(tr("Matching library metadata..."));
    processNext();
}

void TmdbMetadataService::cancel()
{
    m_queue.clear();
    if (m_reply)
        m_reply->abort();
    setBusy(false);
    setStatusMessage(tr("Metadata refresh cancelled"));
}

void TmdbMetadataService::processNext()
{
    if (!m_busy)
        return;
    if (m_queue.isEmpty())
    {
        finishRefresh(m_failed > 0
            ? tr("Updated %1 titles; %2 lookups failed").arg(m_updated).arg(m_failed)
            : tr("Updated metadata for %1 titles").arg(m_updated));
        return;
    }

    m_currentItem = m_queue.dequeue();
    QNetworkReply* reply = m_network.get(requestFor(m_currentItem));
    m_reply = reply;
    connect(reply, &QNetworkReply::downloadProgress, reply, [reply](qint64 received, qint64 total) {
        if (received > MaximumResponseBytes || total > MaximumResponseBytes)
        {
            reply->setProperty("cineResponseTooLarge", true);
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (!m_busy)
        {
            reply->deleteLater();
            return;
        }
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 401 || status == 403)
        {
            reply->deleteLater();
            m_queue.clear();
            finishRefresh(tr("TMDB rejected the API token or key"));
            return;
        }
        if (status == 429)
        {
            reply->deleteLater();
            m_queue.clear();
            finishRefresh(tr("TMDB request limit reached; try again later"));
            return;
        }

        bool lookupCompleted = false;
        if (!reply->property("cineResponseTooLarge").toBool()
            && reply->error() == QNetworkReply::NoError)
        {
            QJsonParseError parseError;
            const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
            if (parseError.error == QJsonParseError::NoError && document.isObject())
            {
                const QJsonObject result = bestResult(document.object().value(QStringLiteral("results")).toArray(),
                                                      m_currentItem.value(QStringLiteral("searchYear")).toInt());
                const QVariantMap metadata = result.isEmpty() ? QVariantMap() : normalizedMetadata(result);
                if (m_library)
                {
                    lookupCompleted = m_library->updateTmdbMetadata(
                        m_currentItem.value(QStringLiteral("mediaId")).toLongLong(), metadata);
                }
                if (lookupCompleted && !metadata.isEmpty())
                    ++m_updated;
            }
        }
        reply->deleteLater();
        m_reply.clear();
        if (!lookupCompleted)
            ++m_failed;
        ++m_completed;
        Q_EMIT progressChanged();
        setStatusMessage(tr("Matching library metadata %1 of %2...").arg(m_completed).arg(m_total));
        QTimer::singleShot(150, this, &TmdbMetadataService::processNext);
    });
}

void TmdbMetadataService::finishRefresh(const QString& message)
{
    m_reply.clear();
    setBusy(false);
    if (m_library && m_updated > 0)
        m_library->refresh();
    setStatusMessage(message);
}

void TmdbMetadataService::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    Q_EMIT busyChanged();
}

void TmdbMetadataService::setStatusMessage(const QString& message)
{
    if (m_statusMessage == message)
        return;
    m_statusMessage = message;
    Q_EMIT statusMessageChanged();
}