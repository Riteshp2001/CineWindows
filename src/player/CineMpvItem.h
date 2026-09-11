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

#include <MpvAbstractItem>

#include <QQuickItem>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

/**
 * @class CineMpvItem
 * @brief QML-accessible mpv video player item based on MpvAbstractItem.
 *
 * @details
 * CineMpvItem wraps an mpv render context inside a QQuickItem, exposing
 * playback properties (position, duration, volume, speed, etc.) and
 * control methods (load, seek, subtitle, audio track switching) to QML.
 *
 * Responsibilities:
 * - Bridge between QML property bindings and mpv property observations
 * - Queue and flush media load requests
 * - Emit Qt signals on mpv property changes and file events
 * - Expose formatted time strings for UI display
 *
 * @thread Must be used on the Qt main thread (QML rendering thread).
 */
class CineMpvItem : public MpvAbstractItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString mediaTitle READ mediaTitle NOTIFY mediaTitleChanged)
    Q_PROPERTY(QString currentPath READ currentPath NOTIFY currentPathChanged)
    Q_PROPERTY(double position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(double duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(QString formattedPosition READ formattedPosition NOTIFY positionChanged)
    Q_PROPERTY(QString formattedDuration READ formattedDuration NOTIFY durationChanged)
    Q_PROPERTY(bool pause READ pause WRITE setPause NOTIFY pauseChanged)
    Q_PROPERTY(bool idle READ idle NOTIFY idleChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool mute READ muted WRITE setMuted NOTIFY muteChanged)
    Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(int playlistPosition READ playlistPosition NOTIFY playlistPositionChanged)
    Q_PROPERTY(int playlistCount READ playlistCount NOTIFY playlistCountChanged)
    Q_PROPERTY(bool rendererReady READ rendererReady NOTIFY rendererReadyChanged)
    Q_PROPERTY(int videoWidth READ videoWidth NOTIFY videoGeometryChanged)
    Q_PROPERTY(int videoHeight READ videoHeight NOTIFY videoGeometryChanged)
    Q_PROPERTY(double videoAspectRatio READ videoAspectRatio NOTIFY videoGeometryChanged)

public:
    /**
     * @brief Constructs a CineMpvItem with an optional QQuickItem parent.
     *
     * @usecase Created by the QML engine when declaring a CineMpvItem in a QML scene.
     *
     * @param parent Optional parent QQuickItem for Qt ownership hierarchy.
     *
     * @sideeffects Initialises the mpv render context, sets up property observers,
     *              and applies default mpv configuration.
     * @thread Must be called on the Qt main thread.
     */
    explicit CineMpvItem(QQuickItem* parent = nullptr);

    /**
     * @brief Returns the title of the currently loaded media.
     * @return Media title string as reported by mpv, or empty if none loaded.
     * @thread Safe to call from the Qt main thread.
     */
    QString mediaTitle() const;

    /**
     * @brief Returns the file path or URL of the currently loaded media.
     * @return Absolute path or remote URL as a string.
     * @thread Safe to call from the Qt main thread.
     */
    QString currentPath() const;

    /**
     * @brief Returns the current playback position in seconds.
     * @return Position in seconds (0.0 if no media loaded).
     * @thread Safe to call from the Qt main thread.
     */
    double position() const;

    /**
     * @brief Seeks to the given position in seconds.
     * @param value Target position in seconds.
     * @sideeffect Updates mpv playback position.
     * @thread Must be called on the Qt main thread.
     */
    void setPosition(double value);

    /**
     * @brief Returns the total duration of the currently loaded media in seconds.
     * @return Duration in seconds, or 0.0 if no media or duration unknown.
     * @thread Safe to call from the Qt main thread.
     */
    double duration() const;

    /**
     * @brief Returns the current position formatted as a human-readable string (e.g. "1:23:45").
     * @return Formatted position string.
     * @thread Safe to call from the Qt main thread.
     */
    QString formattedPosition() const;

    /**
     * @brief Returns the total duration formatted as a human-readable string (e.g. "2:30:00").
     * @return Formatted duration string.
     * @thread Safe to call from the Qt main thread.
     */
    QString formattedDuration() const;

    /**
     * @brief Returns whether playback is currently paused.
     * @return true if paused or stopped, false if playing.
     * @thread Safe to call from the Qt main thread.
     */
    bool pause() const;

    /**
     * @brief Sets the paused state of playback.
     * @param value true to pause, false to resume.
     * @sideeffect Changes mpv pause property, possibly pausing or resuming playback.
     * @thread Must be called on the Qt main thread.
     */
    void setPause(bool value);

    /**
     * @brief Returns whether the player is in idle state (no media loaded).
     * @return true when idle, false when media is loaded or loading.
     * @thread Safe to call from the Qt main thread.
     */
    bool idle() const;

    /**
     * @brief Returns the current audio volume level.
     * @return Volume integer between 0 and MaxVolume (130).
     * @thread Safe to call from the Qt main thread.
     */
    int volume() const;

    /**
     * @brief Sets the audio volume level.
     * @param value Volume value, typically 0..130. Clamped internally by mpv.
     * @sideeffect Changes mpv volume property and audible output level.
     * @thread Must be called on the Qt main thread.
     */
    void setVolume(int value);

    /**
     * @brief Returns whether audio is muted.
     * @return true if muted, false otherwise.
     * @thread Safe to call from the Qt main thread.
     */
    bool muted() const;

    /**
     * @brief Sets the mute state of audio output.
     * @param value true to mute, false to unmute.
     * @sideeffect Changes mpv mute property.
     * @thread Must be called on the Qt main thread.
     */
    void setMuted(bool value);

    /**
     * @brief Returns the current playback speed multiplier.
     * @return Speed multiplier (clamped between MinSpeed (0.25) and MaxSpeed (4.0)).
     * @thread Safe to call from the Qt main thread.
     */
    double speed() const;

    /**
     * @brief Sets the playback speed multiplier.
     * @param value Speed multiplier. Clamped to [MinSpeed, MaxSpeed] range.
     * @sideeffect Changes mpv speed property, affecting playback rate.
     * @thread Must be called on the Qt main thread.
     */
    void setSpeed(double value);

    /**
     * @brief Returns the index of the currently playing item in the mpv playlist.
     * @return Zero-based playlist position, or -1 if nothing is playing.
     * @thread Safe to call from the Qt main thread.
     */
    int playlistPosition() const;

    /**
     * @brief Returns the total number of items in the mpv playlist.
     * @return Playlist item count, or 0 if the playlist is empty.
     * @thread Safe to call from the Qt main thread.
     */
    int playlistCount() const;

    /**
     * @brief Returns whether the mpv renderer is fully initialised and ready.
     * @return true once the render context and OpenGL/windowing resources are ready.
     * @thread Safe to call from the Qt main thread.
     */
    bool rendererReady() const;
    /** @brief Returns the rotation-adjusted video display width. */
    int videoWidth() const;
    /** @brief Returns the rotation-adjusted video display height. */
    int videoHeight() const;
    /** @brief Returns the rotation-adjusted video display aspect ratio. */
    double videoAspectRatio() const;

    /**
     * @brief Loads a media item into mpv.
     *
     * @usecase Called from PlaybackController and QML when opening a file or URL.
     *
     * @param path Local path or remote URL accepted by mpv/yt-dlp.
     * @param mode mpv load mode, usually "replace" or "append-play".
     *
     * @sideeffect Updates the mpv playlist and may start playback. If a previous
     *            load is still pending, the call is deferred and flushed once
     *            the renderer is ready.
     * @thread Must be called on the Qt main thread.
     */
    Q_INVOKABLE void loadFile(const QString& path, const QString& mode = QStringLiteral("replace"));

    /**
     * @brief Loads and attaches an external subtitle file to the current media.
     *
     * @usecase Called when a user drag-and-drops a subtitle file or uses the
     *          subtitle file picker from QML.
     *
     * @param path Absolute file path to the subtitle file (srt, ass, vtt, etc.).
     *
     * @sideeffect Adds an external subtitle track to mpv; may become active immediately.
     * @thread Must be called on the Qt main thread.
     */
    Q_INVOKABLE void addSubtitle(const QString& path);

    /**
     * @brief Loads and attaches an external audio track to the current media.
     *
     * @usecase Called when a user drag-and-drops an audio file or uses the
     *          audio track picker from QML.
     *
     * @param path Absolute file path to the external audio file.
     *
     * @sideeffect Adds an external audio track to mpv; may become active immediately.
     * @thread Must be called on the Qt main thread.
     */
    Q_INVOKABLE void addAudio(const QString& path);

    /**
     * @brief Seeks forward or backward by a relative amount of seconds.
     *
     * @usecase Called by arrow keys, seek gestures, or on-screen seek buttons.
     *
     * @param seconds Signed relative offset in seconds (positive = forward, negative = backward).
     *
     * @sideeffect Moves mpv playback position.
     * @thread Must be called on the Qt main thread.
     */
    Q_INVOKABLE void seekRelative(double seconds);

    /**
     * @brief Sets an mpv track property (video, audio, or subtitle) to a specific track ID.
     *
     * @usecase Called when the user selects a track from the track selection UI.
     *
     * @param property mpv property name, e.g. "sid", "aid", or "vid".
     * @param trackId Track ID to activate. Use setTrackDisabled() to disable a track.
     *
     * @sideeffect Changes the active track selection in mpv.
     * @thread Must be called on the Qt main thread.
     */
    Q_INVOKABLE void setTrack(const QString& property, int trackId);

    /**
     * @brief Disables a track property (video, audio, or subtitle).
     *
     * @usecase Called when the user selects "disable" or "none" for a track in the UI.
     *
     * @param property mpv property name, e.g. "sid", "aid", or "vid".
     *
     * @sideeffect Sets the given track property to "no" (disabled) in mpv.
     * @thread Must be called on the Qt main thread.
     */
    Q_INVOKABLE void setTrackDisabled(const QString& property);

    /**
     * @brief Sets an arbitrary mpv option to the given value.
     *
     * @usecase Generic interface for configuring mpv options from QML without
     *          adding dedicated setter methods.
     *
     * @param name mpv option or property name.
     * @param value Desired value (type depends on the mpv option).
     *
     * @sideeffect Changes the specified mpv option.
     * @thread Must be called on the Qt main thread.
     */
    Q_INVOKABLE void setMpvOption(const QString& name, const QVariant& value);

    /**
     * @brief Reads the current value of an mpv option.
     *
     * @usecase Used by QML bindings to read mpv state that does not have a
     *          dedicated getter property.
     *
     * @param name mpv option or property name to query.
     *
     * @return Current value of the mpv option as a QVariant.
     * @thread Must be called on the Qt main thread.
     */
    Q_INVOKABLE QVariant mpvOption(const QString& name);

    /**
     * @brief Executes an mpv command synchronously.
     *
     * @usecase Used for ad-hoc mpv commands that do not have a dedicated wrapper.
     *
     * @param params Command name followed by arguments, e.g. ["sub-step", "1"].
     *
     * @sideeffect Depends on the specific mpv command executed.
     * @thread Must be called on the Qt main thread.
     */
    Q_INVOKABLE void runCommand(const QStringList& params);

    /**
     * @brief Executes an mpv command asynchronously.
     *
     * @usecase Used for commands where the caller does not need to wait for
     *          completion or does not require a return value.
     *
     * @param params Command name followed by arguments.
     *
     * @sideeffect Depends on the specific mpv command executed. The command is queued
     *            and processed on the mpv event loop.
     * @thread Must be called on the Qt main thread.
     */
    Q_INVOKABLE void runCommandAsync(const QStringList& params);

    /**
     * @brief Returns the libmpv client API version used by this player instance.
     *
     * @usecase Used for runtime feature detection or compatibility checks.
     *
     * @return The mpv client API version as a packed integer (e.g. 0x020100).
     * @thread Must be called on the Qt main thread.
     */
    quint64 mpvClientApiVersion();

    /**
     * @brief Returns mpv's monotonic internal clock value.
     *
     * @usecase Used for synchronisation with external time sources or
     *          measuring elapsed playback time at the mpv level.
     *
     * @return Internal clock time in microseconds, or 0 if the mpv handle is unavailable.
     * @thread Must be called on the Qt main thread.
     */
    qint64 mpvInternalTimeUs();

Q_SIGNALS:
    /** @brief Emitted when the media title property changes. */
    void mediaTitleChanged();
    /** @brief Emitted when the current file path changes. */
    void currentPathChanged();
    /** @brief Emitted when the playback position changes. */
    void positionChanged();
    /** @brief Emitted when the media duration is determined or changes. */
    void durationChanged();
    /** @brief Emitted when the paused state changes. */
    void pauseChanged();
    /** @brief Emitted when the player enters or exits idle state. */
    void idleChanged();
    /** @brief Emitted when the volume level changes. */
    void volumeChanged();
    /** @brief Emitted when the mute state changes. */
    void muteChanged();
    /** @brief Emitted when the playback speed changes. */
    void speedChanged();
    /** @brief Emitted when the playlist index changes. */
    void playlistPositionChanged();
    /** @brief Emitted when the playlist item count changes. */
    void playlistCountChanged();
    /** @brief Emitted when the renderer becomes ready (or loses readiness). */
    void rendererReadyChanged();
    /** @brief Emitted when decoded video display geometry changes. */
    void videoGeometryChanged();
    /** @brief Emitted when the list of available media tracks is updated.
     *  @param tracks List of track info objects (variant maps). */
    void tracksChanged(const QVariantList& tracks);
    /** @brief Emitted when the list of chapters is updated.
     *  @param chapters List of chapter info objects (variant maps). */
    void chaptersChanged(const QVariantList& chapters);
    /** @brief Emitted when a new file starts loading. */
    void fileStarted();
    /** @brief Emitted when a file has finished loading and is ready for playback. */
    void fileLoaded();
    /** @brief Emitted when the current file playback ends.
     *  @param reason End-of-file reason string from mpv (e.g. "eof", "stop", "error"). */
    void endFile(const QString& reason);
    /** @brief Emitted when the video output is reconfigured (resolution change, etc.). */
    void videoReconfigured();
    /** @brief Emitted when a playback error occurs.
     *  @param message Human-readable error description. */
    void playbackError(const QString& message);
    /** @brief Emitted for every property update received from libmpv. */
    void mpvPropertyChanged(const QString& property, const QVariant& value);

private:
    /**
     * @brief Connects mpv property observation signals to internal update slots.
     *
     * @usecase Called once during construction to wire up mpv event handlers.
     *
     * @sideeffect Registers observers for all tracked mpv properties.
     * @thread Must be called on the Qt main thread.
     */
    void setupConnections();

    /**
     * @brief Applies initial mpv configuration options at startup.
     *
     * @usecase Called once during construction to set default mpv options
     *          (volume, config directory, OSD, subtitle styling, hwdec, etc.).
     *
     * @sideeffect Writes a batch of mpv options.
     * @thread Must be called on the Qt main thread.
     */
    void configureDefaults();

    /**
     * @brief Handles an observed mpv property change and updates the corresponding member.
     *
     * @usecase Called by the mpv observation callback whenever a tracked property changes.
     *
     * @param property Name of the changed mpv property.
     * @param value New value of the property.
     *
     * @sideeffect Updates the cached member variable and emits the associated Qt signal.
     * @thread May be called from the mpv event thread; must marshal to the Qt main thread.
     */
    void onPropertyChanged(const QString& property, const QVariant& value);

    /**
     * @brief Flushes any deferred loadFile request once the renderer is ready.
     *
     * @usecase Called when rendererReady transitions to true and a pending
     *          load was queued while the renderer was still initialising.
     *
     * @sideeffect Triggers the actual loadFileNow() call for the pending path.
     * @thread Must be called on the Qt main thread.
     */
    void flushPendingLoad();

    /**
     * @brief Issues the loadfile mpv command immediately.
     *
     * @usecase Internal dispatcher called by loadFile() and flushPendingLoad().
     *
     * @param path Media path or URL.
     * @param mode mpv load mode.
     *
     * @sideeffect Directly changes the mpv playlist.
     * @thread Must be called on the Qt main thread.
     */
    void loadFileNow(const QString& path, const QString& mode);

    /**
     * @brief Rebuilds the formatted position string from the raw position value.
     *
     * @usecase Called internally when position changes, to keep m_formattedPosition in sync.
     *
     * @sideeffect Updates m_formattedPosition string.
     * @thread Must be called on the Qt main thread.
     */
    void updateFormattedPosition();

    /**
     * @brief Rebuilds the formatted duration string from the raw duration value.
     *
     * @usecase Called internally when duration changes, to keep m_formattedDuration in sync.
     *
     * @sideeffect Updates m_formattedDuration string.
     * @thread Must be called on the Qt main thread.
     */
    void updateFormattedDuration();
    /** @brief Refreshes display dimensions and aspect ratio from mpv. */
    void updateVideoGeometry();
    /** @brief Clears display geometry while a replacement file is loading. */
    void resetVideoGeometry();

    /** @brief Maximum allowed volume level (mpv permits values above 100). */
    static constexpr int MaxVolume = 200;
    /** @brief Minimum allowed playback speed multiplier. */
    static constexpr double MinSpeed = 0.25;
    /** @brief Maximum allowed playback speed multiplier. */
    static constexpr double MaxSpeed = 4.0;

    /** @brief Title of the currently loaded media file. */
    QString m_mediaTitle;
    /** @brief Absolute path or URL of the currently loaded media. */
    QString m_currentPath;
    /** @brief Current playback position in seconds. */
    double m_position{0.0};
    /** @brief Total duration of the loaded media in seconds. */
    double m_duration{0.0};
    /** @brief Human-readable formatted position string (e.g. "1:23:45"). */
    QString m_formattedPosition{QStringLiteral("0:00")};
    /** @brief Human-readable formatted duration string (e.g. "2:30:00"). */
    QString m_formattedDuration{QStringLiteral("0:00")};
    /** @brief true when playback is paused or stopped. */
    bool m_pause{true};
    /** @brief true when no media is loaded. */
    bool m_idle{true};
    /** @brief Current audio volume level (0..130). */
    int m_volume{100};
    /** @brief true when audio output is muted. */
    bool m_muted{false};
    /** @brief Current playback speed multiplier. */
    double m_speed{1.0};
    /** @brief Zero-based index of the current item in the mpv playlist (-1 if none). */
    int m_playlistPosition{-1};
    /** @brief Number of items in the mpv playlist. */
    int m_playlistCount{0};
    /** @brief true once the mpv render context is fully initialised. */
    bool m_rendererReady{false};
    /** @brief Rotation-adjusted display width reported by mpv. */
    int m_videoWidth{0};
    /** @brief Rotation-adjusted display height reported by mpv. */
    int m_videoHeight{0};
    /** @brief Rotation-adjusted display aspect ratio reported by mpv. */
    double m_videoAspectRatio{0.0};
    /** @brief true when a loadFile() call is deferred until the renderer is ready. */
    bool m_hasPendingLoad{false};
    /** @brief Path stored for a deferred media load request. */
    QString m_pendingLoadPath;
    /** @brief Load mode stored for a deferred media load request. */
    QString m_pendingLoadMode;
};
