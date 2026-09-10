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
#include <QMetaObject>
#include <QtQml/qqmlregistration.h>

class CineMpvItem;
class PlaylistModel;
class SettingsManager;

/**
 * @class PlaybackController
 * @brief Coordinates playback commands between QML, CineMpvItem, playlist state, and settings.
 *
 * @details
 * PlaybackController is the QML-facing use-case layer for user playback actions.
 * It keeps the UI from directly implementing playlist traversal, volume nudges,
 * session-friendly play requests, screenshots, and settings application.
 *
 * Responsibilities:
 * - Open local or remote media paths through PlaylistModel and CineMpvItem
 * - Navigate playlist items
 * - Apply persisted playback settings to mpv
 * - Emit short user-facing feedback messages
 *
 * This class must not own UI rendering or dialog state.
 */
class PlaybackController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(CineMpvItem* player READ player WRITE setPlayer NOTIFY playerChanged)
    Q_PROPERTY(PlaylistModel* playlist READ playlist WRITE setPlaylist NOTIFY playlistChanged)
    Q_PROPERTY(SettingsManager* settings READ settings WRITE setSettings NOTIFY settingsChanged)

public:
    /**
     * @brief Constructs a PlaybackController with an optional QObject parent.
     *
     * @usecase Created by the QML engine when a PlaybackController is declared in QML,
     *          or instantiated in C++ and set as a context property.
     *
     * @param parent Optional parent QObject for Qt ownership hierarchy.
     *
     * @sideeffect None. Player, playlist, and settings must be set separately
     *            before the controller is fully operational.
     * @thread Must be called on the Qt main thread.
     */
    explicit PlaybackController(QObject* parent = nullptr);

    /**
     * @brief Returns the currently assigned CineMpvItem instance.
     * @return Pointer to the active player, or nullptr if not set.
     * @thread Safe to call from the Qt main thread.
     */
    CineMpvItem* player() const;

    /**
     * @brief Assigns the CineMpvItem instance used for all playback commands.
     *
     * @usecase Called during initialisation to wire up the player dependency.
     *
     * @param player Pointer to a CineMpvItem. Must not be null when playback
     *               methods are invoked.
     *
     * @sideeffect Emits playerChanged().
     * @thread Must be called on the Qt main thread.
     */
    void setPlayer(CineMpvItem* player);

    /**
     * @brief Returns the currently assigned PlaylistModel instance.
     * @return Pointer to the playlist model, or nullptr if not set.
     * @thread Safe to call from the Qt main thread.
     */
    PlaylistModel* playlist() const;

    /**
     * @brief Assigns the PlaylistModel used for playlist navigation and state.
     *
     * @usecase Called during initialisation to wire up the playlist dependency.
     *
     * @param playlist Pointer to a PlaylistModel.
     *
     * @sideeffect Emits playlistChanged().
     * @thread Must be called on the Qt main thread.
     */
    void setPlaylist(PlaylistModel* playlist);

    /**
     * @brief Returns the currently assigned SettingsManager instance.
     * @return Pointer to the settings manager, or nullptr if not set.
     * @thread Safe to call from the Qt main thread.
     */
    SettingsManager* settings() const;

    /**
     * @brief Assigns the SettingsManager used to persist and restore
     *        playback preferences.
     *
     * @usecase Called during initialisation to wire up the settings dependency.
     *
     * @param settings Pointer to a SettingsManager.
     *
     * @sideeffect Emits settingsChanged().
     * @thread Must be called on the Qt main thread.
     */
    void setSettings(SettingsManager* settings);

    /**
     * @brief Opens one or more media paths for playback.
     *
     * @usecase Called by file dialogs, drag/drop, URL dialogs, and playlist add actions.
     *
     * @param paths Local filesystem paths or remote URLs accepted by mpv.
     * @param clearFirst When true, replaces the current playlist before opening.
     *
     * @sideeffects Mutates the playlist and may start playback.
     * @thread Must be called on the main Qt/QML thread.
     */
    Q_INVOKABLE void openPaths(const QStringList& paths, bool clearFirst = true);

    /** @brief Opens one path and seeks after the file has loaded. */
    Q_INVOKABLE void openPathAt(const QString& path, double position, bool clearFirst = true);

    /**
     * @brief Starts playback for a playlist item by index.
     *
     * @usecase Used when restoring a session, clicking playlist rows, or moving to
     * a known item after playlist edits.
     *
     * @param index Zero-based playlist row to play.
     *
     * @sideeffects Updates current playlist index and commands CineMpvItem to load media.
     * @thread Must be called on the main Qt/QML thread.
     */
    Q_INVOKABLE void playIndex(int index);

    /** @brief Plays a playlist item and seeks after the file has loaded. */
    Q_INVOKABLE void playIndexAt(int index, double position);

    /**
     * @brief Advances playback to the next playlist item when available.
     *
     * @usecase Triggered by the Next control and automatic end-of-file handling.
     *
     * @sideeffects Updates current playlist index and commands media loading.
     * @thread Must be called on the main Qt/QML thread.
     */
    Q_INVOKABLE void playNext();

    /**
     * @brief Moves playback to the previous playlist item when available.
     *
     * @usecase Triggered by the Previous control.
     *
     * @sideeffects Updates current playlist index and commands media loading.
     * @thread Must be called on the main Qt/QML thread.
     */
    Q_INVOKABLE void playPrevious();

    /**
     * @brief Toggles paused state on the current player.
     *
     * @usecase Used by keyboard shortcuts, video clicks, context menus, and control buttons.
     *
     * @sideeffects Changes mpv pause state.
     * @thread Must be called on the main Qt/QML thread.
     */
    Q_INVOKABLE void togglePause();

    /**
     * @brief Stops active playback and returns the player to idle.
     *
     * @usecase Used by future stop actions or shutdown flows that need to unload media.
     *
     * @sideeffects Commands mpv to stop playback.
     * @thread Must be called on the main Qt/QML thread.
     */
    Q_INVOKABLE void stop();

    /**
     * @brief Toggles mute on the current player.
     *
     * @usecase Used by the volume popup, shortcuts, and media controls.
     *
     * @sideeffects Changes mpv mute state and may emit userMessage().
     * @thread Must be called on the main Qt/QML thread.
     */
    Q_INVOKABLE void toggleMute();

    /**
     * @brief Adjusts volume by a relative amount.
     *
     * @usecase Used by volume keyboard shortcuts and wheel-style increments.
     *
     * @param delta Signed volume delta in percentage points.
     *
     * @sideeffects Updates mpv volume and persisted settings when available.
     * @thread Must be called on the main Qt/QML thread.
     */
    Q_INVOKABLE void nudgeVolume(int delta);

    /**
     * @brief Toggles playlist repeat mode.
     *
     * @usecase Used by the loop-playlist button and shortcuts.
     *
     * @sideeffects Updates the player loop-playlist option and may emit userMessage().
     * @thread Must be called on the main Qt/QML thread.
     */
    Q_INVOKABLE void togglePlaylistLoop();

    /**
     * @brief Toggles looping for the current media file.
     *
     * @usecase Used by the loop-file button and shortcuts.
     *
     * @sideeffects Updates the player loop-file option and may emit userMessage().
     * @thread Must be called on the main Qt/QML thread.
     */
    Q_INVOKABLE void toggleFileLoop();

    /**
     * @brief Randomizes playlist order while preserving playback semantics.
     *
     * @usecase Used by the Shuffle control.
     *
     * @sideeffects Sends mpv a playlist-shuffle command.
     * @thread Must be called on the main Qt/QML thread.
     */
    Q_INVOKABLE void shufflePlaylist();

    /**
     * @brief Restores playlist order after shuffling when supported.
     *
     * @usecase Reserved for UI flows that expose undoing shuffle state.
     *
     * @sideeffects Sends mpv a playlist-unshuffle command.
     * @thread Must be called on the main Qt/QML thread.
     */
    Q_INVOKABLE void unshufflePlaylist();

    /**
     * @brief Requests a screenshot using the active player.
     *
     * @usecase Triggered by screenshot buttons or keyboard shortcuts.
     *
     * @sideeffects Writes an image file through mpv and emits user feedback.
     * @thread Must be called on the main Qt/QML thread.
     */
    Q_INVOKABLE void screenshot();

    /**
     * @brief Applies persisted playback preferences to the active player.
     *
     * @usecase Called during startup, after media load, and after settings changes.
     *
     * @sideeffects Writes mpv options for volume, mute, subtitles, hardware decoding,
     * normalization, and related playback preferences.
     * @thread Must be called on the main Qt/QML thread.
     */
    Q_INVOKABLE void applySettings();

Q_SIGNALS:
    /** @brief Emitted when the assigned player instance changes. */
    void playerChanged();
    /** @brief Emitted when the assigned playlist model changes. */
    void playlistChanged();
    /** @brief Emitted when the assigned settings manager changes. */
    void settingsChanged();
    /** @brief Emitted to relay a user-facing feedback or status message to the UI.
     *  @param message Short descriptive text (e.g. "Track switched", "Volume: 50%"). */
    void userMessage(const QString& message);

private:
    /** @brief The active mpv player item that receives playback commands. */
    CineMpvItem* m_player{nullptr};
    /** @brief The playlist model used for track navigation and state. */
    PlaylistModel* m_playlist{nullptr};
    /** @brief The settings manager for persisting/restoring playback preferences. */
    SettingsManager* m_settings{nullptr};
    /** @brief Tracks the connection to CineMpvItem::fileLoaded for pending-seek logic. */
    QMetaObject::Connection m_fileLoadedConnection;
    /** @brief Path stored while waiting for fileLoaded, used by openPathAt / playIndexAt. */
    QString m_pendingResumePath;
    /** @brief Seek position stored while waiting for fileLoaded, clamped to [0, duration-1]. */
    double m_pendingResumePosition{0.0};
};
