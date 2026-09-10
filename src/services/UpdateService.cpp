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

#include "services/UpdateService.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>
#include <QVersionNumber>

namespace
{
constexpr qint64 MaximumInstallerBytes = 512LL * 1024 * 1024;

bool isValidInstallerName(const QString& name)
{
    return !name.isEmpty() && !name.contains(QLatin1Char('/')) && !name.contains(QLatin1Char('\\'))
        && name.endsWith(QStringLiteral("-win64-setup.exe"), Qt::CaseInsensitive);
}

bool isTrustedInstallerUrl(const QUrl& url)
{
    return url.isValid() && url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0
        && url.host().compare(QStringLiteral("github.com"), Qt::CaseInsensitive) == 0
        && url.path().startsWith(QStringLiteral("/Riteshp2001/CineWindows/releases/download/"));
}

bool isValidSha256Digest(const QString& digest)
{
    static const QRegularExpression pattern(QStringLiteral("^sha256:[0-9a-f]{64}$"),
                                            QRegularExpression::CaseInsensitiveOption);
    return pattern.match(digest).hasMatch();
}

bool fileMatchesSha256(const QString& path, const QString& digest)
{
    if (!isValidSha256Digest(digest))
        return false;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QCryptographicHash hash(QCryptographicHash::Sha256);
    return hash.addData(&file)
        && QString::fromLatin1(hash.result().toHex()).compare(digest.mid(7), Qt::CaseInsensitive) == 0;
}

bool appendDownloadData(QNetworkReply* reply, QSaveFile* file)
{
    if (!reply || !file)
        return false;
    const QByteArray data = reply->readAll();
    if (data.size() > MaximumInstallerBytes - file->pos())
        return false;
    return file->write(data) == data.size();
}
}

UpdateService::UpdateService(QObject* parent)
    : QObject(parent)
{}

bool UpdateService::busy() const
{
    return m_busy;
}
bool UpdateService::updateAvailable() const
{
    return m_updateAvailable;
}
QString UpdateService::latestVersion() const
{
    return m_latestVersion;
}
QString UpdateService::statusMessage() const
{
    return m_statusMessage;
}
UpdateService::State UpdateService::state() const { return m_state; }
QString UpdateService::releaseUrl() const { return m_releaseUrl; }
QString UpdateService::releaseNotes() const { return m_releaseNotes; }
qint64 UpdateService::downloadedBytes() const { return m_downloadedBytes; }
qint64 UpdateService::totalBytes() const { return m_totalBytes; }
double UpdateService::downloadProgress() const
{
    return m_totalBytes > 0 ? static_cast<double>(m_downloadedBytes) / static_cast<double>(m_totalBytes) : 0.0;
}
QString UpdateService::downloadedFilePath() const { return m_downloadedFilePath; }

/**
 * @brief Sends an HTTP request to GitHub's API to check for a newer release.
 * @param currentVersion The currently installed version string for comparison.
 */
void UpdateService::checkForUpdates(const QString& currentVersion)
{
    if (m_state == Checking || m_state == Downloading)
    {
        return;
    }

    setBusy(true);
    setState(Checking);
    resetRelease();
    setStatusMessage(tr("Checking for updates..."));

    // Fetch the latest release metadata from GitHub API
    QNetworkRequest request(
        QUrl(QStringLiteral("https://api.github.com/repos/Riteshp2001/CineWindows/releases/latest")));
    request.setRawHeader("User-Agent", QStringLiteral("CineWindows/%1").arg(currentVersion).toUtf8());
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setTransferTimeout(10000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply* reply = m_network.get(request);
    m_reply = reply;

    // Handle API response: parse JSON and compare versions
    connect(reply, &QNetworkReply::finished, this, [this, reply, currentVersion] {
        if (m_reply == reply)
            m_reply = nullptr;
        setBusy(false);

        // Check for network or HTTP errors
        if (reply->error() != QNetworkReply::NoError || !reply->isOpen())
        {
            setState(Error);
            setStatusMessage(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404
                                 ? tr("No CineWindows releases have been published yet")
                                 : tr("Could not check for updates: %1").arg(reply->errorString()));
            reply->deleteLater();
            return;
        }

        // Parse the GitHub release JSON response
        const QByteArray data = reply->readAll();
        reply->deleteLater();
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull())
        {
            setState(Error);
            setStatusMessage(tr("Invalid update server response"));
            return;
        }
        const QJsonObject root = doc.object();
        // Strip leading 'v' from version tag if present
        QString tag = root.value(QStringLiteral("tag_name")).toString();
        if (tag.startsWith(QLatin1Char('v'), Qt::CaseInsensitive))
            tag.remove(0, 1);
        if (tag.isEmpty())
        {
            setState(Error);
            setStatusMessage(tr("Release response did not include a version"));
            return;
        }
        m_latestVersion = tag;
        m_releaseUrl = root.value(QStringLiteral("html_url")).toString();
        m_releaseNotes = root.value(QStringLiteral("body")).toString();

        // Find the Windows installer asset in the release
        const QJsonArray assets = root.value(QStringLiteral("assets")).toArray();
        for (const QJsonValue& value : assets)
        {
            const QJsonObject asset = value.toObject();
            const QString name = asset.value(QStringLiteral("name")).toString();
            const QUrl installerUrl(asset.value(QStringLiteral("browser_download_url")).toString());
            const QString installerDigest = asset.value(QStringLiteral("digest")).toString();
            const qint64 installerSize = asset.value(QStringLiteral("size")).toInteger();
            if (isValidInstallerName(name) && isTrustedInstallerUrl(installerUrl)
                && isValidSha256Digest(installerDigest)
                && installerSize > 0 && installerSize <= MaximumInstallerBytes)
            {
                m_installerName = name;
                m_installerUrl = installerUrl.toString();
                m_installerDigest = installerDigest;
                m_totalBytes = installerSize;
                break;
            }
        }

        // Compare versions to determine if an update is available
        const QVersionNumber latest = QVersionNumber::fromString(tag);
        const QVersionNumber current = QVersionNumber::fromString(currentVersion);
        const bool available = !latest.isNull() && QVersionNumber::compare(latest, current) > 0;
        m_updateAvailable = available;
        setState(available ? Available : Idle);
        Q_EMIT updateAvailableChanged();
        Q_EMIT downloadProgressChanged();
        setStatusMessage(available
                             ? (m_installerUrl.isEmpty() ? tr("Update %1 is available on GitHub").arg(tag)
                                                          : tr("Update available: %1").arg(tag))
                             : tr("CineWindows is up to date"));
    });
}

/**
 * @brief Downloads the available installer to a temporary directory.
 *        Falls back to opening the release page if no direct URL is available.
 */
void UpdateService::downloadUpdate()
{
    if (m_state != Available || !isValidInstallerName(m_installerName)
        || !isTrustedInstallerUrl(QUrl(m_installerUrl)) || !isValidSha256Digest(m_installerDigest)
        || m_totalBytes <= 0 || m_totalBytes > MaximumInstallerBytes)
    {
        openReleasePage();
        return;
    }
    // Create the update download folder in the system temp directory
    const QString directory = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                  .filePath(QStringLiteral("CineWindowsUpdates"));
    if (!QDir().mkpath(directory))
    {
        setState(Error);
        setStatusMessage(tr("Could not create the update download folder"));
        return;
    }
    m_downloadedFilePath = QDir(directory).filePath(m_installerName);
    m_downloadFile = std::make_unique<QSaveFile>(m_downloadedFilePath);
    if (!m_downloadFile->open(QIODevice::WriteOnly))
    {
        m_downloadFile.reset();
        setState(Error);
        setStatusMessage(tr("Could not open the update file"));
        return;
    }

    m_downloadedBytes = 0;
    setBusy(true);
    setState(Downloading);
    setStatusMessage(tr("Downloading %1...").arg(m_latestVersion));
    // Stream the installer download with progress tracking
    QNetworkRequest request{QUrl(m_installerUrl)};
    request.setRawHeader("User-Agent", QStringLiteral("CineWindows/%1").arg(QCoreApplication::applicationVersion()).toUtf8());
    request.setTransferTimeout(30000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    m_reply = m_network.get(request);
    connect(m_reply, &QNetworkReply::readyRead, this, [this] {
        if (m_reply && m_downloadFile && !appendDownloadData(m_reply, m_downloadFile.get()))
        {
            m_reply->setProperty("cineDownloadFailure", tr("Update download exceeded its size limit or could not be written"));
            m_reply->abort();
        }
    });
    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (m_reply && (received > MaximumInstallerBytes || total > MaximumInstallerBytes))
        {
            m_reply->setProperty("cineDownloadFailure", tr("Update download exceeded its size limit"));
            m_reply->abort();
            return;
        }
        m_downloadedBytes = received;
        if (total > 0)
            m_totalBytes = total;
        Q_EMIT downloadProgressChanged();
    });
    // Handle download completion: verify and commit the file
    connect(m_reply, &QNetworkReply::finished, this, [this] {
        QNetworkReply* reply = m_reply;
        m_reply = nullptr;
        setBusy(false);
        if (!reply)
            return;
        const bool writeOk = m_downloadFile && appendDownloadData(reply, m_downloadFile.get());
        const QString downloadFailure = reply->property("cineDownloadFailure").toString();
        const bool networkOk = reply->error() == QNetworkReply::NoError && downloadFailure.isEmpty();
        const QString networkError = downloadFailure.isEmpty() ? reply->errorString() : downloadFailure;
        reply->deleteLater();
        // Commit the file only if the download was successful
        if (!networkOk || !writeOk || !m_downloadFile->commit())
        {
            m_downloadFile.reset();
            QFile::remove(m_downloadedFilePath);
            setState(Error);
            setStatusMessage(tr("Update download failed: %1").arg(networkError));
            return;
        }
        m_downloadFile.reset();

        // Verify the downloaded file against the mandatory release digest.
        if (!fileMatchesSha256(m_downloadedFilePath, m_installerDigest))
        {
            QFile::remove(m_downloadedFilePath);
            setState(Error);
            setStatusMessage(tr("The downloaded update failed its integrity check"));
            return;
        }
        setState(ReadyToInstall);
        setStatusMessage(tr("Update %1 is ready to install").arg(m_latestVersion));
    });
}

/**
 * @brief Cancels the active download, aborts the network request, and cleans up.
 */
void UpdateService::cancelDownload()
{
    if (m_state != Downloading)
        return;
    // Abort the active network reply
    if (m_reply)
    {
        QNetworkReply* reply = m_reply;
        m_reply = nullptr;
        disconnect(reply, nullptr, this, nullptr);
        reply->abort();
        reply->deleteLater();
    }
    // Cancel pending file writes and remove partial data
    if (m_downloadFile)
        m_downloadFile->cancelWriting();
    m_downloadFile.reset();
    QFile::remove(m_downloadedFilePath);
    setBusy(false);
    setState(Available);
    setStatusMessage(tr("Update download canceled"));
}

/**
 * @brief Launches the downloaded installer in a detached process and quits the app.
 * @return True if the installer was successfully started.
 */
bool UpdateService::installUpdate()
{
    if (m_state != ReadyToInstall)
        return false;
    if (!fileMatchesSha256(m_downloadedFilePath, m_installerDigest))
    {
        QFile::remove(m_downloadedFilePath);
        setState(Error);
        setStatusMessage(tr("The downloaded update failed its integrity check"));
        return false;
    }
    // Launch the installer detached so it runs after app exits
    if (!QProcess::startDetached(m_downloadedFilePath, {}))
    {
        setState(Error);
        setStatusMessage(tr("Could not start the update installer"));
        return false;
    }
    QCoreApplication::quit();
    return true;
}

/**
 * @brief Opens the GitHub release page in the default web browser.
 */
void UpdateService::openReleasePage() const
{
    if (!m_releaseUrl.isEmpty())
        QDesktopServices::openUrl(QUrl(m_releaseUrl));
}

/**
 * @brief Sets the busy state and emits busyChanged() if different.
 * @param busy New busy state value.
 */
void UpdateService::setBusy(bool busy)
{
    if (m_busy == busy)
    {
        return;
    }
    m_busy = busy;
    Q_EMIT busyChanged();
}

/**
 * @brief Sets the status message and emits statusMessageChanged() if different.
 * @param message New status text.
 */
void UpdateService::setStatusMessage(const QString& message)
{
    if (m_statusMessage == message)
    {
        return;
    }
    m_statusMessage = message;
    Q_EMIT statusMessageChanged();
}

/**
 * @brief Sets the service state and emits stateChanged() if different.
 * @param state New State enum value.
 */
void UpdateService::setState(State state)
{
    if (m_state == state)
        return;
    m_state = state;
    Q_EMIT stateChanged();
}

/**
 * @brief Resets all release metadata and download state to defaults.
 */
void UpdateService::resetRelease()
{
    m_updateAvailable = false;
    m_latestVersion.clear();
    m_releaseUrl.clear();
    m_releaseNotes.clear();
    m_installerUrl.clear();
    m_installerName.clear();
    m_installerDigest.clear();
    m_downloadedFilePath.clear();
    m_downloadedBytes = 0;
    m_totalBytes = 0;
    Q_EMIT updateAvailableChanged();
    Q_EMIT downloadProgressChanged();
}
