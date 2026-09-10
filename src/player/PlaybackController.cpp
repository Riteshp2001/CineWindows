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

#include "player/PlaybackController.h"

#include "app/SettingsManager.h"
#include "models/PlaylistModel.h"
#include "player/CineMpvItem.h"
#include "utils/PathUtils.h"

#include <QFileInfo>
#include <QStringList>
#include <QtGlobal>

#include <cmath>

PlaybackController::PlaybackController(QObject* parent)
    : QObject(parent)
{}

CineMpvItem* PlaybackController::player() const
{
    return m_player;
}
PlaylistModel* PlaybackController::playlist() const
{
    return m_playlist;
}
SettingsManager* PlaybackController::settings() const
{
    return m_settings;
}

/**
 * @brief Assigns the CineMpvItem instance and wires the file-loaded resume handler.
 *
 * Disconnects any previous fileLoaded handler, then sets up a lambda that
 * auto-seeks the player to a stored position once the media finishes loading.
 *
 * @param player Pointer to the new player instance (may be null to clear).
 * @sideeffect Emits playerChanged(). Disconnects prior fileLoaded connection.
 */
void PlaybackController::setPlayer(CineMpvItem* player)
{
    if (m_player == player)
    {
        return;
    }
    if (m_fileLoadedConnection)
        disconnect(m_fileLoadedConnection);
    m_player = player;
    if (m_player)
    {
        // Wire fileLoaded handler to seek to the pending position after media loads
        m_fileLoadedConnection = connect(m_player, &CineMpvItem::fileLoaded, this, [this] {
            if (!m_player || m_pendingResumePath.isEmpty()
                || m_player->currentPath() != m_pendingResumePath)
                return;
            // Clamp position to valid range, leaving 1s margin at end
            const double duration = m_player->duration();
            const double position = duration > 0.0
                ? qBound(0.0, m_pendingResumePosition, qMax(0.0, duration - 1.0))
                : qMax(0.0, m_pendingResumePosition);
            if (position > 0.0)
                m_player->setPosition(position);
            m_pendingResumePath.clear();
            m_pendingResumePosition = 0.0;
        });
    }
    Q_EMIT playerChanged();
}

/**
 * @brief Assigns the PlaylistModel used for playlist navigation.
 * @param playlist Pointer to the new playlist model (may be null).
 * @sideeffect Emits playlistChanged().
 */
void PlaybackController::setPlaylist(PlaylistModel* playlist)
{
    if (m_playlist == playlist)
    {
        return;
    }
    m_playlist = playlist;
    Q_EMIT playlistChanged();
}

/**
 * @brief Assigns the SettingsManager and wires live-update connections to mpv.
 *
 * Disconnects any previous settings connections, then connects every
 * settings signal to its corresponding mpv option setter.
 *
 * @param settings Pointer to the new settings manager (may be null).
 * @sideeffect Emits settingsChanged(). Registers per-setting lambdas.
 */
void PlaybackController::setSettings(SettingsManager* settings)
{
    if (m_settings == settings)
    {
        return;
    }
    if (m_settings)
    {
        disconnect(m_settings, nullptr, this, nullptr);
    }
    m_settings = settings;
    if (m_settings)
    {
        // Realtime: add/remove loudnorm audio filter via mpv af command
        connect(m_settings, &SettingsManager::normalizeVolumeChanged, this, [this]() {
            if (m_player && m_settings)
            {
                if (m_settings->normalizeVolume())
                {
                    m_player->runCommandAsync({QStringLiteral("af"), QStringLiteral("add"),
                                               QStringLiteral("@cine_loudnorm:lavfi=[loudnorm=I=-20]")});
                }
                else
                {
                    m_player->runCommandAsync(
                        {QStringLiteral("af"), QStringLiteral("remove"), QStringLiteral("@cine_loudnorm")});
                }
            }
        });

        // Realtime: update mpv subtitle options when settings change
        connect(m_settings, &SettingsManager::subtitleFontChanged, this, [this]() {
            if (m_player && m_settings)
            {
                m_player->setMpvOption(QStringLiteral("sub-font"), m_settings->subtitleFont());
            }
        });
        connect(m_settings, &SettingsManager::subtitleScaleChanged, this, [this]() {
            if (m_player && m_settings)
            {
                m_player->setMpvOption(QStringLiteral("sub-scale"), m_settings->subtitleScale());
            }
        });
        connect(m_settings, &SettingsManager::subtitleColorChanged, this, [this]() {
            if (m_player && m_settings)
            {
                m_player->setMpvOption(QStringLiteral("sub-color"), m_settings->subtitleColor());
            }
        });
        connect(m_settings, &SettingsManager::subtitleBackgroundChanged, this, [this]() {
            if (m_player && m_settings)
            {
                m_player->setMpvOption(QStringLiteral("sub-border-style"), m_settings->subtitleBackground()
                                                                                ? QStringLiteral("background-box")
                                                                                : QStringLiteral("outline-and-shadow"));
            }
        });
        connect(m_settings, &SettingsManager::subtitleBackgroundColorChanged, this, [this]() {
            if (m_player && m_settings)
            {
                m_player->setMpvOption(QStringLiteral("sub-back-color"), m_settings->subtitleBackgroundColor());
            }
        });
        connect(m_settings, &SettingsManager::subtitleLanguagesChanged, this, [this]() {
            if (m_player && m_settings)
            {
                m_player->setMpvOption(QStringLiteral("slang"), m_settings->subtitleLanguages());
            }
        });
    }
    Q_EMIT settingsChanged();
}

/**
 * @brief Adds media paths to the playlist and starts playback from the first new entry.
 * @param paths One or more local/remote media paths.
 * @param clearFirst If true, clears the existing playlist before adding.
 * @sideeffect Mutates playlist state and triggers media loading via playIndex().
 */
void PlaybackController::openPaths(const QStringList& paths, bool clearFirst)
{
    if (!m_playlist || !m_player || paths.isEmpty())
    {
        return;
    }

    const int previousCount = clearFirst ? 0 : m_playlist->count();
    m_playlist->addPaths(paths, clearFirst);
    if (m_playlist->count() > previousCount)
        playIndex(previousCount);
}

/**
 * @brief Opens a single path and prepares a seek after the file loads.
 *
 * Stores the desired position so the fileLoaded handler in setPlayer()
 * can seek once playback begins.
 *
 * @param path Local or remote media path.
 * @param position Seek position in seconds (clamped to valid range on load).
 * @param clearFirst If true, clears the existing playlist.
 * @sideeffect Mutates playlist and triggers deferred seek.
 */
void PlaybackController::openPathAt(const QString& path, double position, bool clearFirst)
{
    if (path.trimmed().isEmpty())
        return;
    const int previousCount = clearFirst ? 0 : (m_playlist ? m_playlist->count() : 0);
    if (!m_playlist)
        return;
    m_playlist->addPaths({path}, clearFirst);
    playIndexAt(previousCount, position);
}

/**
 * @brief Loads and starts playback of the playlist item at the given index.
 *
 * Updates the playlist's current index, then tells mpv to load the file
 * via CineMpvItem::loadFile with the "replace" load flag.
 *
 * @param index Zero-based playlist row.
 * @sideeffect Changes mpv's currently loaded file and unpauses.
 */
void PlaybackController::playIndex(int index)
{
    if (!m_playlist || !m_player || index < 0 || index >= m_playlist->count())
    {
        return;
    }

    // Update playlist state and load the media file in mpv
    m_playlist->setCurrentIndex(index);
    m_player->loadFile(m_playlist->pathAt(index), QStringLiteral("replace"));
    m_player->setPause(false);
}

/**
 * @brief Plays a playlist item and schedules a seek to a given position.
 *
 * Stores the path and position for the fileLoaded resume handler,
 * then delegates to playIndex().
 *
 * @param index Zero-based playlist row.
 * @param position Desired seek position in seconds.
 * @sideeffect Triggers deferred seek once media finishes loading.
 */
void PlaybackController::playIndexAt(int index, double position)
{
    if (!m_playlist || index < 0 || index >= m_playlist->count())
        return;
    // Store resume info for the fileLoaded handler to consume
    m_pendingResumePath = m_playlist->pathAt(index);
    m_pendingResumePosition = std::isfinite(position) ? qMax(0.0, position) : 0.0;
    playIndex(index);
}

/**
 * @brief Advances to the next item in the playlist, wrapping around at the end.
 * @sideeffect Triggers media load for the next item via playIndex().
 */
void PlaybackController::playNext()
{
    if (!m_playlist || m_playlist->count() == 0)
    {
        return;
    }
    const int next = (m_playlist->currentIndex() + 1) % m_playlist->count();
    playIndex(next);
}

/**
 * @brief Moves to the previous item in the playlist, wrapping to the last item at the start.
 * @sideeffect Triggers media load for the previous item via playIndex().
 */
void PlaybackController::playPrevious()
{
    if (!m_playlist || m_playlist->count() == 0)
    {
        return;
    }
    int previous = m_playlist->currentIndex() - 1;
    if (previous < 0)
    {
        previous = m_playlist->count() - 1;
    }
    playIndex(previous);
}

/**
 * @brief Toggles pause state via mpv's no-osd cycle pause command.
 * @sideeffect Changes mpv internal pause state; does not show OSD feedback.
 */
void PlaybackController::togglePause()
{
    if (m_player)
    {
        // Cycle pause without showing mpv OSD (UI provides its own indicator)
        m_player->runCommandAsync(
            {QStringLiteral("no-osd"), QStringLiteral("cycle"), QStringLiteral("pause")});
    }
}

/**
 * @brief Stops playback and unloads the current media via mpv.
 * @sideeffect Sends mpv the "stop" command, returning it to idle.
 */
void PlaybackController::stop()
{
    if (m_player)
    {
        m_player->runCommandAsync({QStringLiteral("stop")});
    }
}

/**
 * @brief Toggles the muted state on the current player.
 * @sideeffect Updates mpv mute property.
 */
void PlaybackController::toggleMute()
{
    if (m_player)
    {
        m_player->setMuted(!m_player->muted());
    }
}

/**
 * @brief Adjusts volume by a relative delta and persists the new value.
 * @param delta Signed adjustment in percentage points (clamped to [0, 200]).
 * @sideeffect Updates mpv volume and writes the new value to settings.
 */
void PlaybackController::nudgeVolume(int delta)
{
    if (!m_player)
    {
        return;
    }
    // Clamp final volume to mpv's supported range
    const int targetVolume = std::clamp(m_player->volume() + delta, 0, 200);
    m_player->setVolume(targetVolume);
    if (m_settings)
    {
        m_settings->setVolume(targetVolume);
    }
}

/**
 * @brief Toggles infinite loop-playlist mode and disables loop-file when activating.
 * @sideeffect Updates mpv loop-playlist and loop-file options.
 */
void PlaybackController::togglePlaylistLoop()
{
    if (m_player)
    {
        // Query current mpv loop-playlist state and toggle it
        const bool active = m_player->mpvOption(QStringLiteral("loop-playlist")).toString() == QStringLiteral("inf");
        m_player->setMpvOption(QStringLiteral("loop-playlist"), active ? QStringLiteral("no") : QStringLiteral("inf"));
        if (!active)
        {
            // Disable file loop when enabling playlist loop
            m_player->setMpvOption(QStringLiteral("loop-file"), QStringLiteral("no"));
        }
    }
}

/**
 * @brief Toggles infinite loop-file mode and disables loop-playlist when activating.
 * @sideeffect Updates mpv loop-file and loop-playlist options.
 */
void PlaybackController::toggleFileLoop()
{
    if (m_player)
    {
        // Query current mpv loop-file state and toggle it
        const bool active = m_player->mpvOption(QStringLiteral("loop-file")).toString() == QStringLiteral("inf");
        m_player->setMpvOption(QStringLiteral("loop-file"), active ? QStringLiteral("no") : QStringLiteral("inf"));
        if (!active)
        {
            // Disable playlist loop when enabling file loop
            m_player->setMpvOption(QStringLiteral("loop-playlist"), QStringLiteral("no"));
        }
    }
}

/**
 * @brief Randomizes the playlist order via mpv's playlist-shuffle command.
 * @sideeffect Sends an mpv async command; playlist order changes immediately.
 */
void PlaybackController::shufflePlaylist()
{
    if (m_player)
    {
        m_player->runCommandAsync({QStringLiteral("playlist-shuffle")});
    }
}

/**
 * @brief Restores the original playlist order after a shuffle.
 * @sideeffect Sends an mpv async command.
 */
void PlaybackController::unshufflePlaylist()
{
    if (m_player)
    {
        m_player->runCommandAsync({QStringLiteral("playlist-unshuffle")});
    }
}

/**
 * @brief Triggers a screenshot via mpv and emits a user-facing confirmation.
 * @sideeffect Writes a screenshot image file through mpv; emits userMessage().
 */
void PlaybackController::screenshot()
{
    if (m_player)
    {
        // Run mpv's built-in screenshot command (uses screenshot-directory / template)
        m_player->runCommandAsync({QStringLiteral("screenshot")});
        Q_EMIT userMessage(tr("Screenshot saved"));
    }
}

/**
 * @brief Applies all persisted playback settings to the active mpv player.
 *
 * Writes volume, mute, hardware decoding, subtitle, audio language,
 * and loudness normalization options to the running mpv instance.
 *
 * @sideeffect Issues multiple mpv setMpvOption and runCommandAsync calls.
 */
void PlaybackController::applySettings()
{
    if (!m_player || !m_settings)
    {
        return;
    }

    // Core playback: volume, mute, hardware decoding
    m_player->setVolume(m_settings->volume());
    m_player->setMuted(m_settings->muted());
    m_player->setMpvOption(QStringLiteral("hwdec"), m_settings->hwdec());

    // Subtitle appearance and language preferences
    m_player->setMpvOption(QStringLiteral("sub-font"), m_settings->subtitleFont());
    m_player->setMpvOption(QStringLiteral("sub-scale"), m_settings->subtitleScale());
    m_player->setMpvOption(QStringLiteral("sub-color"), m_settings->subtitleColor());
    m_player->setMpvOption(QStringLiteral("slang"), m_settings->subtitleLanguages());
    m_player->setMpvOption(QStringLiteral("alang"), m_settings->audioLanguages());
    m_player->setMpvOption(QStringLiteral("sub-border-style"), m_settings->subtitleBackground()
                                                                    ? QStringLiteral("background-box")
                                                                    : QStringLiteral("outline-and-shadow"));
    m_player->setMpvOption(QStringLiteral("sub-back-color"), m_settings->subtitleBackgroundColor());

    // Loudness normalization via lavfi loudnorm filter
    if (m_settings->normalizeVolume())
    {
        m_player->runCommandAsync(
            {QStringLiteral("af"), QStringLiteral("add"), QStringLiteral("@cine_loudnorm:lavfi=[loudnorm=I=-20]")});
    }
    else
    {
        m_player->runCommandAsync({QStringLiteral("af"), QStringLiteral("remove"), QStringLiteral("@cine_loudnorm")});
    }
}
