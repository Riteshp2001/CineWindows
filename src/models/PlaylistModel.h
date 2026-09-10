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

#include <QAbstractListModel>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

/** @brief Model managing a playable list of media items.
 *  @details Provides a QML-accessible list model for media file playlists.
 *           Supports adding files/folders, reordering, saving as M3U,
 *           and tracking the currently playing item. */
class PlaylistModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    /** @brief Custom roles exposed to QML for playlist data access. */
    enum Role
    {
        PathRole = Qt::UserRole + 1, ///< Full file path of the item
        NameRole,                     ///< Display name of the item
        PlayingRole,                  ///< Whether this item is currently playing
        LocalRole                     ///< Whether the item is a local file
    };
    Q_ENUM(Role)

    /** @brief Constructs a PlaylistModel with the given parent. */
    explicit PlaylistModel(QObject* parent = nullptr);

    /** @brief Returns the number of items in the playlist. */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    /** @brief Returns playlist item data for the given role at the specified index. */
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    /** @brief Returns the mapping of role IDs to role names for QML. */
    QHash<int, QByteArray> roleNames() const override;

    /** @brief Returns the index of the currently playing item. */
    int currentIndex() const;
    /** @brief Sets the currently playing item index. */
    void setCurrentIndex(int index);
    /** @brief Returns the total number of items in the playlist. */
    int count() const;

    /** @brief Returns the file path of the item at the given index. */
    Q_INVOKABLE QString pathAt(int index) const;
    /** @brief Appends a single file path to the playlist. */
    Q_INVOKABLE void addPath(const QString& path);
    /** @brief Appends multiple file paths, optionally clearing first. */
    Q_INVOKABLE void addPaths(const QStringList& paths, bool clearFirst = false);
    /** @brief Appends multiple URLs, optionally clearing first. */
    Q_INVOKABLE void addUrls(const QList<QUrl>& urls, bool clearFirst = false);
    /** @brief Scans a folder for media files and adds them, optionally clearing first. Returns the number of files added. */
    Q_INVOKABLE int addFolder(const QString& folderPath, bool clearFirst = false);
    /** @brief Removes the item at the specified index. */
    Q_INVOKABLE void removeAt(int index);
    /** @brief Removes all items from the playlist. */
    Q_INVOKABLE void clear();
    /** @brief Moves an item from index @p from to index @p to. */
    Q_INVOKABLE void moveItem(int from, int to);
    /** @brief Saves the playlist as an M3U file to the given destination. Returns true on success. */
    Q_INVOKABLE bool saveM3u(const QUrl& destination) const;
    /** @brief Returns the number of items whose name matches the given query. */
    Q_INVOKABLE int matchCount(const QString& query) const;

Q_SIGNALS:
    /** @brief Emitted when the current playing item index changes. */
    void currentIndexChanged();
    /** @brief Emitted when the item count changes. */
    void countChanged();
    /** @brief Emitted when the playlist content changes (add, remove, reorder). */
    void playlistChanged();

private:
    /** @brief Internal data structure for a single playlist item. */
    struct Item
    {
        QString path;  ///< File path or URL to the media
        QString name;  ///< Display name of the media item
        bool local{true}; ///< True if the media file is on the local filesystem
    };

    QList<Item> m_items;         ///< Internal list of playlist items
    int m_currentIndex{-1};      ///< Index of the currently playing item (-1 if none)
};
