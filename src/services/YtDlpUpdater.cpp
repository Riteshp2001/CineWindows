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
 */

#include "services/YtDlpUpdater.h"

#include "app/LoggingCategories.h"
#include "utils/PathUtils.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSysInfo>
#include <QUrl>

namespace
{
constexpr qint64 MaximumDownloadBytes = 256LL * 1024 * 1024;
constexpr qint64 CheckIntervalSeconds = 24LL * 60 * 60;
constexpr qint64 MinimumForcedCheckIntervalMs = 10LL * 60 * 1000;

QString releaseAssetName()
{
#if defined(Q_OS_WIN)
    return QSysInfo::currentCpuArchitecture() == QStringLiteral("arm64") ? QStringLiteral("yt-dlp_arm64.exe")
                                                                          : QStringLiteral("yt-dlp.exe");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("yt-dlp_macos");
#elif defined(Q_OS_LINUX)
    const QString architecture = QSysInfo::currentCpuArchitecture();
    if (architecture == QStringLiteral("x86_64"))
        return QStringLiteral("yt-dlp_linux");
    if (architecture == QStringLiteral("arm64"))
        return QStringLiteral("yt-dlp_linux_aarch64");
    return {};
#else
    return {};
#endif
}

QString managedExecutablePath()
{
#ifdef Q_OS_WIN
    return QDir(PathUtils::managedToolsDir()).filePath(QStringLiteral("yt-dlp.exe"));
#else
    return QDir(PathUtils::managedToolsDir()).filePath(QStringLiteral("yt-dlp"));
#endif
}

QString versionFilePath()
{
    return QDir(PathUtils::managedToolsDir()).filePath(QStringLiteral("yt-dlp.version"));
}

QString installedVersion()
{
    QFile file(versionFilePath());
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()).trimmed() : QString();
}

void writeVersion(const QString& version)
{
    QSaveFile file(versionFilePath());
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(version.toUtf8());
        file.commit();
    }
}

bool isTrustedDownloadUrl(const QUrl& url)
{
    return url.isValid() && url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0
        && url.host().compare(QStringLiteral("github.com"), Qt::CaseInsensitive) == 0
        && url.path().startsWith(QStringLiteral("/yt-dlp/yt-dlp/releases/download/"));
}

bool isValidSha256Digest(const QString& digest)
{
    static const QRegularExpression pattern(QStringLiteral("^sha256:[0-9a-f]{64}$"),
                                            QRegularExpression::CaseInsensitiveOption);
    return pattern.match(digest).hasMatch();
}

QNetworkRequest githubRequest(const QUrl& url, int timeoutMs)
{
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent",
                         QStringLiteral("CineWindows/%1").arg(QCoreApplication::applicationVersion()).toUtf8());
    request.setTransferTimeout(timeoutMs);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    return request;
}
}

YtDlpUpdater::YtDlpUpdater(QObject* parent)
    : QObject(parent)
{}

YtDlpUpdater::~YtDlpUpdater()
{
    if (m_reply)
        m_reply->abort();
}

void YtDlpUpdater::checkForUpdate(bool force)
{
    const QString assetName = releaseAssetName();
    if (m_reply || assetName.isEmpty())
        return;

    const bool installed = QFileInfo::exists(managedExecutablePath());
    if (force)
    {
        if (m_lastForcedCheck.isValid() && m_lastForcedCheck.elapsed() < MinimumForcedCheckIntervalMs)
            return;
        m_lastForcedCheck.start();
    }
    else if (installed)
    {
        const QFileInfo versionInfo(versionFilePath());
        if (versionInfo.exists()
            && versionInfo.lastModified().secsTo(QDateTime::currentDateTime()) < CheckIntervalSeconds)
            return;
    }

    QNetworkRequest request = githubRequest(
        QUrl(QStringLiteral("https://api.github.com/repos/yt-dlp/yt-dlp/releases/latest")), 15000);
    request.setRawHeader("Accept", "application/vnd.github+json");
    QNetworkReply* reply = m_network.get(request);
    m_reply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, assetName, installed] {
        reply->deleteLater();
        if (m_reply == reply)
            m_reply = nullptr;
        if (reply->error() != QNetworkReply::NoError)
        {
            qCWarning(cineServiceLog) << "Could not check for yt-dlp updates:" << reply->errorString();
            return;
        }

        const QJsonObject release = QJsonDocument::fromJson(reply->readAll()).object();
        const QString version = release.value(QStringLiteral("tag_name")).toString();
        if (version.isEmpty())
            return;
        if (installed && version == installedVersion())
        {
            writeVersion(version);
            return;
        }

        for (const QJsonValue& value : release.value(QStringLiteral("assets")).toArray())
        {
            const QJsonObject asset = value.toObject();
            if (asset.value(QStringLiteral("name")).toString() != assetName)
                continue;
            const QUrl url(asset.value(QStringLiteral("browser_download_url")).toString());
            const QString digest = asset.value(QStringLiteral("digest")).toString();
            const qint64 size = asset.value(QStringLiteral("size")).toInteger();
            if (isTrustedDownloadUrl(url) && isValidSha256Digest(digest) && size > 0 && size <= MaximumDownloadBytes)
                download(url, digest, size, version);
            else
                qCWarning(cineServiceLog) << "Ignoring yt-dlp release asset without a trusted URL and digest";
            return;
        }
    });
}

void YtDlpUpdater::download(const QUrl& url, const QString& digest, qint64 size, const QString& version)
{
    const QString target = managedExecutablePath();
    m_file = std::make_unique<QSaveFile>(target);
    if (!m_file->open(QIODevice::WriteOnly))
    {
        qCWarning(cineServiceLog) << "Could not write yt-dlp to" << QDir::toNativeSeparators(target);
        m_file.reset();
        return;
    }
    m_hash.reset();
    qCInfo(cineServiceLog).noquote() << "Downloading yt-dlp" << version;

    QNetworkReply* reply = m_network.get(githubRequest(url, 60000));
    m_reply = reply;
    const auto append = [this, reply, size] {
        const QByteArray data = reply->readAll();
        if (!m_file || m_file->pos() + data.size() > size || m_file->write(data) != data.size())
        {
            reply->abort();
            return false;
        }
        m_hash.addData(data);
        return true;
    };
    connect(reply, &QNetworkReply::readyRead, this, append);
    connect(reply, &QNetworkReply::finished, this, [this, reply, append, digest, size, version, target] {
        reply->deleteLater();
        if (m_reply == reply)
            m_reply = nullptr;
        if (!m_file)
            return;
        const bool received = reply->error() == QNetworkReply::NoError && append() && m_file->pos() == size;
        const bool verified = received
            && QString::fromLatin1(m_hash.result().toHex()).compare(digest.mid(7), Qt::CaseInsensitive) == 0;
        std::unique_ptr<QSaveFile> file = std::move(m_file);
        if (!verified)
        {
            file->cancelWriting();
            qCWarning(cineServiceLog) << "Discarded yt-dlp download that failed verification:"
                                      << reply->errorString();
            return;
        }
        // Fails while a playback extraction is still running the old binary; the next check retries.
        if (!file->commit())
        {
            qCWarning(cineServiceLog) << "Could not replace yt-dlp:" << file->errorString();
            return;
        }
#ifndef Q_OS_WIN
        QFile::setPermissions(target, QFile::permissions(target) | QFileDevice::ExeOwner | QFileDevice::ExeUser
                                          | QFileDevice::ExeGroup | QFileDevice::ExeOther);
#endif
        writeVersion(version);
        qCInfo(cineServiceLog).noquote() << "Updated yt-dlp to" << version;
        Q_EMIT updated(version);
    });
}
