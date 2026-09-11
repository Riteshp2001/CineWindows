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

#include "models/TrackModel.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QLocale>
#include <QRegularExpression>
#include <QVariantMap>

namespace {
QString normalizedLanguageCode(const QString& value)
{
    return value.trimmed().section(QRegularExpression(QStringLiteral("[-_]")), 0, 0).toLower();
}

QString inferredSubtitleLanguage(const QVariantMap& track)
{
    const QString baseName = QFileInfo(track.value(QStringLiteral("external-filename")).toString()).completeBaseName();
    const QStringList tokens = baseName.split(QRegularExpression(QStringLiteral("[._ -]+")), Qt::SkipEmptyParts);
    for (auto token = tokens.crbegin(); token != tokens.crend(); ++token)
    {
        const QString code = normalizedLanguageCode(*token);
        if ((code.size() == 2 || code.size() == 3)
            && QLocale::codeToLanguage(code) != QLocale::AnyLanguage)
        {
            return code;
        }
    }
    return {};
}

QString languageLabel(const QString& code)
{
    const QString normalized = normalizedLanguageCode(code);
    const QLocale::Language language = QLocale::codeToLanguage(normalized);
    if (language == QLocale::AnyLanguage)
    {
        return code.trimmed().toUpper();
    }
    return QStringLiteral("%1 (%2)").arg(QLocale::languageToString(language), normalized);
}
} // namespace

TrackModel::TrackModel(QObject* parent)
    : QAbstractListModel(parent)
{}

int TrackModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_tracks.size());
}

QVariant TrackModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_tracks.size())
    {
        return {};
    }
    const Track& track = m_tracks.at(index.row());
    switch (role)
    {
        case IdRole:
            return track.id;
        case TypeRole:
            return track.type;
        case TitleRole:
            return track.title;
        case LanguageRole:
            return track.language;
        case LabelRole:
        case Qt::DisplayRole:
            return track.label;
        case SelectedRole:
            return track.id == m_selectedTrack;
        default:
            return {};
    }
}

QHash<int, QByteArray> TrackModel::roleNames() const
{
    static const QHash<int, QByteArray> names{{IdRole, "trackId"},  {TypeRole, "type"},
                                              {TitleRole, "title"}, {LanguageRole, "language"},
                                              {LabelRole, "label"}, {SelectedRole, "selected"}};
    return names;
}

QString TrackModel::filterType() const
{
    return m_filterType;
}

void TrackModel::setFilterType(const QString& type)
{
    if (m_filterType == type)
    {
        return;
    }
    m_filterType = type;
    Q_EMIT filterTypeChanged();
}

int TrackModel::selectedTrack() const
{
    return m_selectedTrack;
}

int TrackModel::count() const
{
    return static_cast<int>(m_tracks.size());
}

void TrackModel::setSelectedTrack(int trackId)
{
    if (m_selectedTrack == trackId)
    {
        return;
    }
    const int previous = m_selectedTrack;
    m_selectedTrack = trackId;
    for (int row = 0; row < m_tracks.size(); ++row)
    {
        if (m_tracks.at(row).id == previous || m_tracks.at(row).id == trackId)
        {
            Q_EMIT dataChanged(index(row), index(row), {SelectedRole});
        }
    }
    Q_EMIT selectedTrackChanged();
}

void TrackModel::updateFromMpv(const QVariantList& tracks)
{
    QList<Track> next;
    int nextSelected = 0;
    bool hasExplicitSelection = false;
    bool previousSelectionStillExists = false;
    for (const QVariant& entry : tracks)
    {
        const QVariantMap map = entry.toMap();
        const QString type = map.value(QStringLiteral("type")).toString();
        if (!m_filterType.isEmpty() && type != m_filterType)
        {
            continue;
        }
        if (type == QStringLiteral("video") && map.value(QStringLiteral("albumart")).toBool())
        {
            continue;
        }

        Track track;
        track.id = map.value(QStringLiteral("id")).toInt();
        track.type = type;
        track.title = map.value(QStringLiteral("title")).toString();
        track.language = normalizedLanguageCode(map.value(QStringLiteral("lang")).toString());
        if (track.language.isEmpty() && type == QStringLiteral("sub"))
        {
            track.language = inferredSubtitleLanguage(map);
        }
        QStringList parts;
        const QString displayLanguage = languageLabel(track.language);
        if (!displayLanguage.isEmpty())
        {
            parts.append(displayLanguage);
        }
        const QString title = track.title.trimmed();
        if (!title.isEmpty() && title.compare(track.language, Qt::CaseInsensitive) != 0
            && title.compare(displayLanguage, Qt::CaseInsensitive) != 0)
        {
            parts.append(title);
        }
        track.label = parts.isEmpty() ? QCoreApplication::translate("TrackModel", "Track %1").arg(track.id)
                                      : parts.join(QStringLiteral(" - "));
        next.append(track);
        if (track.id == m_selectedTrack)
        {
            previousSelectionStillExists = true;
        }

        if (map.value(QStringLiteral("selected")).toBool())
        {
            nextSelected = track.id;
            hasExplicitSelection = true;
        }
    }

    if (!hasExplicitSelection && previousSelectionStillExists)
    {
        nextSelected = m_selectedTrack;
    }

    beginResetModel();
    m_tracks = next;
    endResetModel();
    if (m_selectedTrack != nextSelected)
    {
        m_selectedTrack = nextSelected;
        Q_EMIT selectedTrackChanged();
    }
    Q_EMIT countChanged();
}

void TrackModel::clear()
{
    beginResetModel();
    m_tracks.clear();
    endResetModel();
    if (m_selectedTrack != 0)
    {
        m_selectedTrack = 0;
        Q_EMIT selectedTrackChanged();
    }
    Q_EMIT countChanged();
}
