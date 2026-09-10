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

#include <QObject>
#include <QtQml/qqmlregistration.h>

class PlaylistModel;

/**
 * @class SessionManager
 * @brief Serializes and deserializes the current playback session to disk.
 * @details Handles saving the active playlist, the currently-playing item index,
 *          and its playback position to a local session file. On restore, the
 *          playlist is rebuilt and playback resumes at the saved position.
 *          Exposed to QML for automatic session persistence.
 */
class SessionManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    /**
     * @brief Constructs a SessionManager with an optional parent.
     * @param parent Optional QObject parent for Qt ownership.
     */
    explicit SessionManager(QObject* parent = nullptr);

    /**
     * @brief Persist the active playlist and playback index.
     * @param playlist Source playlist model to serialize.
     * @param currentIndex Currently playing item index.
     * @param position Current playback position in seconds.
     * @return True when the session file was written successfully.
     */
    Q_INVOKABLE bool save(PlaylistModel* playlist, int currentIndex, double position) const;

    /**
     * @brief Restores a previously saved session by rebuilding the playlist.
     * @param playlist The playlist model to populate from the session file.
     * @return True if a session file was found and loaded successfully.
     */
    Q_INVOKABLE bool restore(PlaylistModel* playlist) const;

    /**
     * @brief Returns the restored item index from the last successful restore.
     * @return The playlist index to resume playback at, or -1 if no session was restored.
     */
    Q_INVOKABLE int restoredIndex() const;

    /**
     * @brief Returns the restored playback position from the last successful restore.
     * @return The playback position in seconds, or 0.0 if no session was restored.
     */
    Q_INVOKABLE double restoredPosition() const;

private:
    /** @brief Cached playlist index restored from the session file. */
    mutable int m_restoredIndex{-1};
    /** @brief Cached playback position restored from the session file. */
    mutable double m_restoredPosition{0.0};
};
