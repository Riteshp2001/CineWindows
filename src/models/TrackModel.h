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

/** @brief Model representing audio/video/subtitle tracks from a media file.
 *  @details Provides a list model for media track metadata (id, type, title,
 *           language, label) consumed by QML UI. Supports filtering by track
 *           type and tracking the currently selected track. */
class TrackModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString filterType READ filterType WRITE setFilterType NOTIFY filterTypeChanged)
    Q_PROPERTY(int selectedTrack READ selectedTrack WRITE setSelectedTrack NOTIFY selectedTrackChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    /** @brief Custom roles exposed to QML for track data access. */
    enum Role
    {
        IdRole = Qt::UserRole + 1, ///< Track ID from mpv
        TypeRole,                   ///< Track type (audio/video/sub)
        TitleRole,                  ///< Track title string
        LanguageRole,               ///< Track language code
        LabelRole,                  ///< Track label for display
        SelectedRole                ///< Whether this track is currently selected
    };
    Q_ENUM(Role)

    /** @brief Constructs a TrackModel with the given parent.
     *  @param parent Parent QObject (optional). */
    explicit TrackModel(QObject* parent = nullptr);

    /** @brief Returns the number of tracks.
     *  @param parent Model index (unused for flat list; only valid for no parent).
     *  @return Number of tracks in the model. */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    /** @brief Returns track data for the given role at the specified index.
     *  @param index Model index of the requested track.
     *  @param role  Data role (e.g. IdRole, TypeRole, TitleRole).
     *  @return Track data for the requested role, or empty QVariant on failure. */
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    /** @brief Returns the mapping of role IDs to role names for QML.
     *  @return Hash mapping custom roles (IdRole, TypeRole, etc.) to property names. */
    QHash<int, QByteArray> roleNames() const override;

    /** @brief Returns the current filter type (empty means no filter).
     *  @return Active filter type string, or empty for all. */
    QString filterType() const;
    /** @brief Sets the filter type to show only matching tracks.
     *  @param type Track type to filter by (e.g. "audio", "video", "sub").
     *              Pass empty string to clear the filter. */
    void setFilterType(const QString& type);
    /** @brief Returns the ID of the currently selected track.
     *  @return Track ID of the selected track, or 0 if none selected. */
    int selectedTrack() const;
    /** @brief Sets the selected track by its track ID.
     *  @param trackId Track ID to select. */
    void setSelectedTrack(int trackId);
    /** @brief Returns the number of tracks.
     *  @return Number of tracks in the model (after filtering). */
    int count() const;

    /** @brief Replaces internal track list from mpv track data.
     *  @param tracks List of QVariantMap entries from mpv containing id, type, title, lang, selected. */
    Q_INVOKABLE void updateFromMpv(const QVariantList& tracks);
    /** @brief Removes all tracks from the model. */
    Q_INVOKABLE void clear();

Q_SIGNALS:
    /** @brief Emitted when the filter type changes. */
    void filterTypeChanged();
    /** @brief Emitted when the selected track ID changes. */
    void selectedTrackChanged();
    /** @brief Emitted when the track count changes. */
    void countChanged();

private:
    /** @brief Internal data structure for a single track entry. */
    struct Track
    {
        int id{0};          ///< Track ID from mpv
        QString type;       ///< Track type (e.g. "audio", "video", "sub")
        QString title;      ///< Track title or description
        QString language;   ///< Language code (e.g. "en", "ja")
        QString label;      ///< Display label for the track
    };

    QList<Track> m_tracks;       ///< Internal list of track entries
    QString m_filterType;        ///< Active filter by track type (empty = no filter)
    int m_selectedTrack{0};      ///< Currently selected track ID
};
