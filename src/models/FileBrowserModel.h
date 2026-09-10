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
#include <QTimer>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

/** @brief Model for browsing the local filesystem for media files.
 *  @details Provides a QML-accessible list model for navigating directories,
 *           filtering for media files, and tracking navigation history. */
class FileBrowserModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QUrl currentFolder READ currentFolder WRITE navigateTo NOTIFY currentFolderChanged)
    Q_PROPERTY(QString displayPath READ displayPath NOTIFY currentFolderChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY navigationStateChanged)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY navigationStateChanged)
    Q_PROPERTY(bool canGoUp READ canGoUp NOTIFY navigationStateChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    /** @brief Custom roles exposed to QML for file browser data access. */
    enum Role
    {
        NameRole = Qt::UserRole + 1, ///< File or directory display name
        PathRole,                     ///< Absolute file system path
        UrlRole,                      ///< QUrl representation of the path
        DirectoryRole,                ///< Whether this entry is a directory
        MediaRole,                    ///< Whether this entry is a supported media file
        SizeRole,                     ///< File size in bytes
        ModifiedRole,                 ///< Last modified timestamp (ms since epoch)
        IconRole                      ///< Icon name for the entry
    };
    Q_ENUM(Role)

    /** @brief Constructs a FileBrowserModel with the given parent, defaults to the Movies folder. */
    explicit FileBrowserModel(QObject* parent = nullptr);
    /** @brief Returns the number of file entries in the current folder. */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    /** @brief Returns file entry data for the given role at the specified index. */
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    /** @brief Returns the mapping of role IDs to role names for QML. */
    QHash<int, QByteArray> roleNames() const override;

    /** @brief Returns the current folder as a QUrl. */
    QUrl currentFolder() const;
    /** @brief Returns the current folder path in native separator format. */
    QString displayPath() const;
    /** @brief Returns the current search filter text. */
    QString searchText() const;
    /** @brief Sets the search filter text and reloads the current folder. */
    void setSearchText(const QString& searchText);
    /** @brief Returns true if there is a previous folder in navigation history. */
    bool canGoBack() const;
    /** @brief Returns true if there is a next folder in navigation history. */
    bool canGoForward() const;
    /** @brief Returns true if the current folder has a parent directory. */
    bool canGoUp() const;
    /** @brief Returns true if the model is currently loading entries. */
    bool loading() const;
    /** @brief Returns the last error message, or an empty string. */
    QString errorString() const;

    /** @brief Navigates to the given folder URL. */
    Q_INVOKABLE void navigateTo(const QUrl& folder);
    /** @brief Navigates to the previous folder in history. */
    Q_INVOKABLE void navigateBack();
    /** @brief Navigates to the next folder in history. */
    Q_INVOKABLE void navigateForward();
    /** @brief Navigates to the parent directory. */
    Q_INVOKABLE void navigateUp();
    /** @brief Reloads the current folder contents. */
    Q_INVOKABLE void refresh();
    /** @brief Returns a QVariantMap of entry data at the given row. */
    Q_INVOKABLE QVariantMap get(int row) const;
    /** @brief Returns a list of standard system places (Home, Videos, etc.) as QVariantMaps. */
    Q_INVOKABLE QVariantList places() const;

Q_SIGNALS:
    /** @brief Emitted when the current folder changes. */
    void currentFolderChanged();
    /** @brief Emitted when the search text changes. */
    void searchTextChanged();
    /** @brief Emitted when the back/forward/up navigation state changes. */
    void navigationStateChanged();
    /** @brief Emitted when the loading state changes. */
    void loadingChanged();
    /** @brief Emitted when the error string changes. */
    void errorStringChanged();
    /** @brief Emitted when the entry count changes. */
    void countChanged();

private:
    /** @brief Internal data structure for a single filesystem entry. */
    struct Entry
    {
        QString name;        ///< File or directory display name
        QString path;        ///< Absolute file system path
        bool directory{false}; ///< True if this entry is a directory
        bool media{false};    ///< True if this entry is a supported media file
        qint64 size{0};       ///< File size in bytes (0 for directories)
        qint64 modified{0};   ///< Last modified timestamp in ms since epoch
        QString icon;         ///< Icon name for QML rendering
    };

    /** @brief Loads entries from the given directory path asynchronously. */
    void load(const QString& path);
    /** @brief Sets the loading state and emits loadingChanged. */
    void setLoading(bool loading);
    /** @brief Sets the error string and emits errorStringChanged. */
    void setErrorString(const QString& errorString);

    QList<Entry> m_entries;           ///< Current list of file/directory entries
    QString m_currentPath;            ///< Currently displayed folder path
    QString m_searchText;             ///< Active search filter text
    QString m_errorString;            ///< Last error message
    QStringList m_backHistory;        ///< Stack of previously visited folders (backward)
    QStringList m_forwardHistory;     ///< Stack of forward navigation folders
    bool m_loading{false};            ///< Whether an async load operation is in progress
    int m_generation{0};              ///< Monotonically increasing counter to discard stale loads
    QTimer m_searchTimer;              ///< Debounces directory rescans while the user types
};
