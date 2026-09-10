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
#include <QtQml/qqmlregistration.h>

/** @brief Model representing chapter entries in a media file.
 *  @details Provides a list model for chapter metadata (index, title, time)
 *           consumed by QML UI. Populated from mpv chapter data via updateFromMpv(). */
class ChapterModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    /** @brief Custom roles exposed to QML for chapter data access. */
    enum Role
    {
        IndexRole = Qt::UserRole + 1, ///< Chapter index
        TitleRole,                     ///< Chapter title string
        TimeRole                       ///< Chapter timestamp in seconds
    };
    Q_ENUM(Role)

    /** @brief Constructs a ChapterModel with the given parent. */
    explicit ChapterModel(QObject* parent = nullptr);

    /** @brief Returns the number of chapters. */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    /** @brief Returns chapter data for the given role at the specified index. */
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    /** @brief Returns the mapping of role IDs to role names for QML. */
    QHash<int, QByteArray> roleNames() const override;
    /** @brief Returns the number of chapters (convenience for QML property). */
    int count() const;

    /** @brief Replaces internal chapter list from mpv chapter data. */
    Q_INVOKABLE void updateFromMpv(const QVariantList& chapters);
    /** @brief Returns a QVariantMap of chapter data at the given index. */
    Q_INVOKABLE QVariantMap at(int index) const;
    /** @brief Removes all chapters from the model. */
    Q_INVOKABLE void clear();

Q_SIGNALS:
    /** @brief Emitted when the chapter count changes. */
    void countChanged();

private:
    /** @brief Internal data structure for a single chapter entry. */
    struct Chapter
    {
        int index{0};   ///< Sequential chapter index
        QString title;  ///< Chapter title text
        double time{0.0}; ///< Chapter start time in seconds
    };

    QList<Chapter> m_chapters; ///< Internal list of chapter entries
};
