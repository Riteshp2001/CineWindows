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

#include "models/PlaylistModel.h"

#include "utils/MediaUtils.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

PlaylistModel::PlaylistModel(QObject* parent)
    : QAbstractListModel(parent)
{}

/** @brief Returns the number of items in the playlist. */
int PlaylistModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

/** @brief Returns playlist item data for the given role.
 *  @details Maps role requests to the internal Item struct fields. */
QVariant PlaylistModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
    {
        return {};
    }

    const Item& item = m_items.at(index.row());
    switch (role)
    {
        case PathRole:
            return item.path;
        case NameRole:
        case Qt::DisplayRole:
            return item.name;
        case PlayingRole:
            return index.row() == m_currentIndex;
        case LocalRole:
            return item.local;
        default:
            return {};
    }
}

QHash<int, QByteArray> PlaylistModel::roleNames() const
{
    static const QHash<int, QByteArray> names{{PathRole, "path"},
                                              {NameRole, "name"},
                                              {PlayingRole, "playing"},
                                              {LocalRole, "local"}};
    return names;
}

int PlaylistModel::currentIndex() const
{
    return m_currentIndex;
}

/** @brief Sets the currently playing item index and emits data change notifications
 *         for the old and new index rows. */
void PlaylistModel::setCurrentIndex(int index)
{
    if (index < -1 || index >= m_items.size() || m_currentIndex == index)
    {
        return;
    }

    const int previous = m_currentIndex;
    m_currentIndex = index;

    if (previous >= 0)
    {
        Q_EMIT dataChanged(this->index(previous), this->index(previous), {PlayingRole});
    }
    if (m_currentIndex >= 0)
    {
        Q_EMIT dataChanged(this->index(m_currentIndex), this->index(m_currentIndex), {PlayingRole});
    }
    Q_EMIT currentIndexChanged();
}

int PlaylistModel::count() const
{
    return static_cast<int>(m_items.size());
}

QString PlaylistModel::pathAt(int index) const
{
    if (index < 0 || index >= m_items.size())
    {
        return {};
    }
    return m_items.at(index).path;
}

void PlaylistModel::addPath(const QString& path)
{
    addPaths({path}, false);
}

/** @brief Appends multiple file paths to the playlist, optionally clearing first. */
void PlaylistModel::addPaths(const QStringList& paths, bool clearFirst)
{
    if (clearFirst)
    {
        clear();
    }

    // Build a list of pending items from the provided paths
    QList<Item> pending;
    for (const QString& path : paths)
    {
        if (path.trimmed().isEmpty())
        {
            continue;
        }
        pending.append({path, MediaUtils::displayNameForPath(path), MediaUtils::isLocalPath(path)});
    }

    if (pending.isEmpty())
    {
        return;
    }

    // Insert rows into the model and append items
    const int first = static_cast<int>(m_items.size());
    beginInsertRows({}, first, first + static_cast<int>(pending.size()) - 1);
    m_items.append(pending);
    endInsertRows();

    Q_EMIT countChanged();
    Q_EMIT playlistChanged();
}

void PlaylistModel::addUrls(const QList<QUrl>& urls, bool clearFirst)
{
    QStringList paths;
    for (const QUrl& url : urls)
    {
        paths.append(MediaUtils::urlToMpvPath(url));
    }
    addPaths(paths, clearFirst);
}

int PlaylistModel::addFolder(const QString& folderPath, bool clearFirst)
{
    const QStringList paths = MediaUtils::listMediaFiles(folderPath);
    addPaths(paths, clearFirst);
    return static_cast<int>(paths.size());
}

/** @brief Removes the item at the specified index and adjusts the current index. */
void PlaylistModel::removeAt(int index)
{
    if (index < 0 || index >= m_items.size())
    {
        return;
    }

    beginRemoveRows({}, index, index);
    m_items.removeAt(index);
    endRemoveRows();

    // Adjust current index if the removed item was at or before it
    if (m_currentIndex == index)
    {
        m_currentIndex = -1;
        Q_EMIT currentIndexChanged();
    }
    else if (m_currentIndex > index)
    {
        --m_currentIndex;
        Q_EMIT currentIndexChanged();
    }

    Q_EMIT countChanged();
    Q_EMIT playlistChanged();
}

/** @brief Removes all items from the playlist and resets the current index. */
void PlaylistModel::clear()
{
    if (m_items.isEmpty())
    {
        return;
    }

    beginResetModel();
    m_items.clear();
    m_currentIndex = -1;
    endResetModel();

    Q_EMIT currentIndexChanged();
    Q_EMIT countChanged();
    Q_EMIT playlistChanged();
}

/** @brief Moves an item from one index to another, updating the current index. */
void PlaylistModel::moveItem(int from, int to)
{
    if (from < 0 || from >= m_items.size() || to < 0 || to >= m_items.size() || from == to)
    {
        return;
    }

    const int modelTo = to > from ? to + 1 : to;
    beginMoveRows({}, from, from, {}, modelTo);
    m_items.move(from, to);
    endMoveRows();

    // Adjust current index if the moved item or surrounding indices changed
    if (m_currentIndex == from)
    {
        m_currentIndex = to;
        Q_EMIT currentIndexChanged();
    }
    else if (from < m_currentIndex && to >= m_currentIndex)
    {
        --m_currentIndex;
        Q_EMIT currentIndexChanged();
    }
    else if (from > m_currentIndex && to <= m_currentIndex)
    {
        ++m_currentIndex;
        Q_EMIT currentIndexChanged();
    }
    Q_EMIT playlistChanged();
}

/** @brief Saves the playlist as an M3U file to the given destination.
 *  @return true on success, false if the file could not be opened. */
bool PlaylistModel::saveM3u(const QUrl& destination) const
{
    const QString path = destination.isLocalFile() ? destination.toLocalFile() : destination.toString();
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }

    // Write M3U header followed by each item path
    QTextStream stream(&file);
    stream << "#EXTM3U\n";
    for (const Item& item : m_items)
    {
        stream << item.path << '\n';
    }
    return true;
}

/** @brief Returns the number of items whose display name matches the given query (case-insensitive). */
int PlaylistModel::matchCount(const QString& query) const
{
    if (query.trimmed().isEmpty())
    {
        return static_cast<int>(m_items.size());
    }
    // Count items whose name contains the query string (case-insensitive)
    int count = 0;
    const QString lowerQuery = query.toLower();
    for (const Item& item : m_items)
    {
        if (item.name.toLower().contains(lowerQuery))
        {
            count++;
        }
    }
    return count;
}
