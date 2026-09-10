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

#include "models/FileBrowserModel.h"

#include "utils/MediaUtils.h"

#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QStandardPaths>
#include <QtConcurrentRun>

/** @brief Constructs a FileBrowserModel, defaulting to the Movies folder or home directory. */
FileBrowserModel::FileBrowserModel(QObject* parent)
    : QAbstractListModel(parent)
{
    m_searchTimer.setSingleShot(true);
    m_searchTimer.setInterval(180);
    connect(&m_searchTimer, &QTimer::timeout, this, [this] { load(m_currentPath); });

    // Start at the Movies location, falling back to home directory
    QString initial = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    if (initial.isEmpty() || !QFileInfo(initial).isDir())
        initial = QDir::homePath();
    m_currentPath = QDir::cleanPath(initial);
    load(m_currentPath);
}

int FileBrowserModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_entries.size());
}

/** @brief Returns file entry data for the given role at the specified index. */
QVariant FileBrowserModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};
    const Entry& entry = m_entries.at(index.row());
    switch (role)
    {
        case NameRole:
        case Qt::DisplayRole:
            return entry.name;
        case PathRole:
            return entry.path;
        case UrlRole:
            return QUrl::fromLocalFile(entry.path);
        case DirectoryRole:
            return entry.directory;
        case MediaRole:
            return entry.media;
        case SizeRole:
            return entry.size;
        case ModifiedRole:
            return entry.modified;
        case IconRole:
            return entry.icon;
        default:
            return {};
    }
}

QHash<int, QByteArray> FileBrowserModel::roleNames() const
{
    return {{NameRole, "name"}, {PathRole, "path"}, {UrlRole, "url"}, {DirectoryRole, "isDirectory"},
            {MediaRole, "isMedia"}, {SizeRole, "size"}, {ModifiedRole, "modified"}, {IconRole, "iconName"}};
}

QUrl FileBrowserModel::currentFolder() const { return QUrl::fromLocalFile(m_currentPath); }
QString FileBrowserModel::displayPath() const { return QDir::toNativeSeparators(m_currentPath); }
QString FileBrowserModel::searchText() const { return m_searchText; }

/** @brief Sets the search filter text and reloads the current folder with the filter applied. */
void FileBrowserModel::setSearchText(const QString& searchText)
{
    if (m_searchText == searchText)
        return;
    m_searchText = searchText;
    ++m_generation;
    setLoading(true);
    Q_EMIT searchTextChanged();
    m_searchTimer.start();
}

bool FileBrowserModel::canGoBack() const { return !m_backHistory.isEmpty(); }
bool FileBrowserModel::canGoForward() const { return !m_forwardHistory.isEmpty(); }
/** @brief Returns true if the current folder has a parent directory above root. */
bool FileBrowserModel::canGoUp() const
{
    const QDir directory(m_currentPath);
    return directory.exists() && directory.absolutePath() != QDir(directory.absolutePath()).rootPath();
}
bool FileBrowserModel::loading() const { return m_loading; }
QString FileBrowserModel::errorString() const { return m_errorString; }

/** @brief Navigates to the given folder URL, pushing the current folder onto back history. */
void FileBrowserModel::navigateTo(const QUrl& folder)
{
    const QString path = folder.isLocalFile() ? folder.toLocalFile() : folder.toString();
    const QFileInfo info(path);
    if (!info.isDir())
    {
        setErrorString(tr("Folder is not available"));
        return;
    }
    const QString clean = QDir::cleanPath(info.absoluteFilePath());
    // If already at the target folder, just refresh
    if (clean == m_currentPath)
    {
        refresh();
        return;
    }
    // Push current folder to back history and clear forward history
    if (!m_currentPath.isEmpty())
        m_backHistory.append(m_currentPath);
    m_forwardHistory.clear();
    m_currentPath = clean;
    Q_EMIT currentFolderChanged();
    Q_EMIT navigationStateChanged();
    load(m_currentPath);
}

/** @brief Navigates to the previous folder in back history, pushing current to forward history. */
void FileBrowserModel::navigateBack()
{
    if (m_backHistory.isEmpty())
        return;
    m_forwardHistory.prepend(m_currentPath);
    m_currentPath = m_backHistory.takeLast();
    Q_EMIT currentFolderChanged();
    Q_EMIT navigationStateChanged();
    load(m_currentPath);
}

/** @brief Navigates to the next folder in forward history, pushing current to back history. */
void FileBrowserModel::navigateForward()
{
    if (m_forwardHistory.isEmpty())
        return;
    m_backHistory.append(m_currentPath);
    m_currentPath = m_forwardHistory.takeFirst();
    Q_EMIT currentFolderChanged();
    Q_EMIT navigationStateChanged();
    load(m_currentPath);
}

/** @brief Navigates to the parent directory of the current folder. */
void FileBrowserModel::navigateUp()
{
    QDir directory(m_currentPath);
    if (directory.cdUp())
        navigateTo(QUrl::fromLocalFile(directory.absolutePath()));
}

/** @brief Reloads the current folder contents. */
void FileBrowserModel::refresh() { load(m_currentPath); }

/** @brief Returns a QVariantMap of entry data at the given row. */
QVariantMap FileBrowserModel::get(int row) const
{
    if (row < 0 || row >= m_entries.size())
        return {};
    const Entry& entry = m_entries.at(row);
    return {{QStringLiteral("name"), entry.name}, {QStringLiteral("path"), entry.path},
            {QStringLiteral("url"), QUrl::fromLocalFile(entry.path)},
            {QStringLiteral("isDirectory"), entry.directory}, {QStringLiteral("isMedia"), entry.media},
            {QStringLiteral("size"), entry.size}, {QStringLiteral("modified"), entry.modified},
            {QStringLiteral("iconName"), entry.icon}};
}

/** @brief Returns a list of standard system places (Home, Videos, Music, Downloads, drives). */
QVariantList FileBrowserModel::places() const
{
    QVariantList result;
    // Add standard QStandardPaths locations
    const auto add = [&result](const QString& name, QStandardPaths::StandardLocation location) {
        const QString path = QStandardPaths::writableLocation(location);
        if (!path.isEmpty() && QFileInfo(path).isDir())
            result.append(QVariantMap{{QStringLiteral("name"), name}, {QStringLiteral("url"), QUrl::fromLocalFile(path)}});
    };
    add(tr("Home"), QStandardPaths::HomeLocation);
    add(tr("Videos"), QStandardPaths::MoviesLocation);
    add(tr("Music"), QStandardPaths::MusicLocation);
    add(tr("Downloads"), QStandardPaths::DownloadLocation);
    // Append available drives
    for (const QFileInfo& drive : QDir::drives())
        result.append(QVariantMap{{QStringLiteral("name"), QDir::toNativeSeparators(drive.absoluteFilePath())},
                                  {QStringLiteral("url"), QUrl::fromLocalFile(drive.absoluteFilePath())}});
    return result;
}

/** @brief Loads entries from the given directory asynchronously using QtConcurrent.
 *  @details Filters for directories and media files, applies the current search text,
 *           and uses a generation counter to discard stale results. */
void FileBrowserModel::load(const QString& path)
{
    m_searchTimer.stop();
    const int generation = ++m_generation;
    const QString search = m_searchText;
    setLoading(true);
    setErrorString({});

    // Spawn asynchronous directory scan
    auto* watcher = new QFutureWatcher<QList<Entry>>(this);
    connect(watcher, &QFutureWatcher<QList<Entry>>::finished, this, [this, watcher, generation, path] {
        const QList<Entry> entries = watcher->result();
        watcher->deleteLater();
        // Discard result if a newer navigation has occurred
        if (generation != m_generation)
            return;
        if (!QFileInfo(path).isDir())
            setErrorString(tr("Folder is no longer available"));
        beginResetModel();
        m_entries = entries;
        endResetModel();
        setLoading(false);
        Q_EMIT countChanged();
    });
    watcher->setFuture(QtConcurrent::run([path, search]() -> QList<Entry> {
        QList<Entry> entries;
        const QDir directory(path);
        const QFileInfoList infos = directory.entryInfoList(
            QDir::AllDirs | QDir::Files | QDir::Readable | QDir::NoDotAndDotDot,
            QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
        for (const QFileInfo& info : infos)
        {
            // Only include directories and supported media files (not subtitles)
            const bool media = info.isFile() && MediaUtils::isMediaFile(info.absoluteFilePath())
                && !MediaUtils::isSubtitleFile(info.absoluteFilePath());
            if (!info.isDir() && !media)
                continue;
            // Apply search text filter (case-insensitive)
            if (!search.trimmed().isEmpty() && !info.fileName().contains(search, Qt::CaseInsensitive))
                continue;
            entries.append({info.fileName(), info.absoluteFilePath(), info.isDir(), media, info.size(),
                            info.lastModified().toMSecsSinceEpoch(),
                            info.isDir() ? QStringLiteral("cine-folder-symbolic")
                                         : QStringLiteral("cine-video-x-generic-symbolic")});
        }
        return entries;
    }));
}

/** @brief Sets the loading state and emits loadingChanged if changed. */
void FileBrowserModel::setLoading(bool loading)
{
    if (m_loading == loading)
        return;
    m_loading = loading;
    Q_EMIT loadingChanged();
}

/** @brief Sets the error string and emits errorStringChanged if changed. */
void FileBrowserModel::setErrorString(const QString& errorString)
{
    if (m_errorString == errorString)
        return;
    m_errorString = errorString;
    Q_EMIT errorStringChanged();
}
