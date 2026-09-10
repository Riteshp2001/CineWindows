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

#include "models/ChapterModel.h"

#include <QCoreApplication>
#include <QVariantMap>

ChapterModel::ChapterModel(QObject* parent)
    : QAbstractListModel(parent)
{}

int ChapterModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_chapters.size());
}

/** @brief Returns chapter data for the given role at the specified index. */
QVariant ChapterModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_chapters.size())
    {
        return {};
    }
    const Chapter& chapter = m_chapters.at(index.row());
    switch (role)
    {
        case IndexRole:
            return chapter.index;
        case TitleRole:
        case Qt::DisplayRole:
            return chapter.title;
        case TimeRole:
            return chapter.time;
        default:
            return {};
    }
}

QHash<int, QByteArray> ChapterModel::roleNames() const
{
    static const QHash<int, QByteArray> names{{IndexRole, "chapterIndex"}, {TitleRole, "title"}, {TimeRole, "time"}};
    return names;
}

int ChapterModel::count() const
{
    return static_cast<int>(m_chapters.size());
}

/** @brief Returns a QVariantMap of chapter data at the given index. */
QVariantMap ChapterModel::at(int index) const
{
    if (index < 0 || index >= m_chapters.size())
    {
        return {};
    }

    const Chapter& chapter = m_chapters.at(index);
    return {{QStringLiteral("chapterIndex"), chapter.index},
            {QStringLiteral("title"), chapter.title},
            {QStringLiteral("time"), chapter.time}};
}

/** @brief Replaces internal chapter list from mpv chapter data.
 *  @details Parses the QVariantList of chapter maps, assigns a fallback title
 *           for unnamed chapters, and resets the model. */
void ChapterModel::updateFromMpv(const QVariantList& chapters)
{
    // Parse mpv chapter data and build internal chapter list
    QList<Chapter> next;
    for (int i = 0; i < chapters.size(); ++i)
    {
        const QVariantMap map = chapters.at(i).toMap();
        Chapter chapter;
        chapter.index = i;
        chapter.time = map.value(QStringLiteral("time")).toDouble();
        chapter.title = map.value(QStringLiteral("title")).toString();
        // Use a localized fallback title if the chapter is unnamed
        if (chapter.title.isEmpty())
        {
            chapter.title = QCoreApplication::translate("ChapterModel", "Chapter %1").arg(i + 1);
        }
        next.append(chapter);
    }

    // Replace the model contents
    beginResetModel();
    m_chapters = next;
    endResetModel();
    Q_EMIT countChanged();
}

/** @brief Removes all chapters from the model. */
void ChapterModel::clear()
{
    beginResetModel();
    m_chapters.clear();
    endResetModel();
    Q_EMIT countChanged();
}
