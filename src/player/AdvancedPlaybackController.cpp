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

#include "player/AdvancedPlaybackController.h"

#include "player/CineMpvItem.h"

#include <QSettings>

namespace {
// Center frequencies (Hz) for the 10-band graphic equaliser
const int EqualizerFrequencies[] = {31, 62, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
}

/**
 * @brief Constructs the controller and restores persisted advanced settings.
 *
 * Reads equalizer, visualization, stereo, and HDR values from QSettings,
 * clamping gains to [-12, 12] dB and targetPeak to [100, 1000].
 *
 * @param parent Optional QObject parent.
 */
AdvancedPlaybackController::AdvancedPlaybackController(QObject* parent)
    : QObject(parent)
{
    QSettings settings;
    // Restore equalizer state and per-band gains
    m_equalizerEnabled = settings.value(QStringLiteral("audio/equalizerEnabled"), false).toBool();
    const QVariantList gains = settings.value(QStringLiteral("audio/equalizerBands")).toList();
    if (gains.size() == m_equalizerBands.size())
    {
        for (int i = 0; i < gains.size(); ++i)
            m_equalizerBands[i] = qBound(-12.0, gains.at(i).toDouble(), 12.0);
    }
    // Restore visualization, stereo, and HDR settings
    m_visualization = settings.value(QStringLiteral("audio/visualization"), m_visualization).toString();
    m_stereoInput = settings.value(QStringLiteral("video/stereoInput"), m_stereoInput).toString();
    m_stereoOutput = settings.value(QStringLiteral("video/stereoOutput"), m_stereoOutput).toString();
    m_toneMapping = settings.value(QStringLiteral("video/toneMapping"), m_toneMapping).toString();
    m_targetPeak = qBound(100, settings.value(QStringLiteral("video/targetPeak"), m_targetPeak).toInt(), 1000);
}

/** @brief Returns the assigned CineMpvItem instance. */
CineMpvItem* AdvancedPlaybackController::player() const { return m_player; }

/**
 * @brief Assigns the player and reapplies all advanced settings on file load.
 *
 * Disconnects the previous player's signals, then connects the new player's
 * fileLoaded signal to applyAll() so equalizer, visualization, stereo, and
 * HDR settings are restored for each new media file.
 *
 * @param player Pointer to the new player instance (may be null).
 * @sideeffect Emits playerChanged().
 */
void AdvancedPlaybackController::setPlayer(CineMpvItem* player)
{
    if (m_player == player)
        return;
    if (m_player)
        disconnect(m_player, nullptr, this, nullptr);
    m_player = player;
    if (m_player)
        // Reapply all advanced settings whenever a new file is loaded
        connect(m_player, &CineMpvItem::fileLoaded, this, &AdvancedPlaybackController::applyAll);
    Q_EMIT playerChanged();
}

/** @brief Returns whether the graphic equalizer is currently enabled. */
bool AdvancedPlaybackController::equalizerEnabled() const { return m_equalizerEnabled; }

/**
 * @brief Returns a QVariantList copy of the 10 equalizer band gains.
 * @return List of doubles in dB, indexed 0 (31 Hz) through 9 (16 kHz).
 */
QVariantList AdvancedPlaybackController::equalizerBands() const
{
    QVariantList result;
    for (double gain : m_equalizerBands)
        result.append(gain);
    return result;
}

/** @brief Returns the audio visualization mode. */
QString AdvancedPlaybackController::visualization() const { return m_visualization; }

/** @brief Returns the stereo 3D input format. */
QString AdvancedPlaybackController::stereoInput() const { return m_stereoInput; }

/** @brief Returns the stereo 3D output format. */
QString AdvancedPlaybackController::stereoOutput() const { return m_stereoOutput; }

/** @brief Returns the HDR tone mapping algorithm. */
QString AdvancedPlaybackController::toneMapping() const { return m_toneMapping; }

/** @brief Returns the target peak brightness in nits. */
int AdvancedPlaybackController::targetPeak() const { return m_targetPeak; }

/**
 * @brief Enables or disables the graphic equalizer and applies the change to mpv.
 * @param enabled True to activate the equalizer, false to bypass it.
 * @sideeffect Persists setting and emits equalizerChanged().
 */
void AdvancedPlaybackController::setEqualizerEnabled(bool enabled)
{
    if (m_equalizerEnabled == enabled)
        return;
    m_equalizerEnabled = enabled;
    save();
    applyEqualizer();
    Q_EMIT equalizerChanged();
}

/**
 * @brief Sets the gain for a single equalizer band.
 *
 * @param index Band index 0-9 (31 Hz - 16 kHz).
 * @param gain Gain in dB, clamped to [-12, 12].
 * @sideeffect Persists bands and emits equalizerChanged().
 */
void AdvancedPlaybackController::setEqualizerBand(int index, double gain)
{
    if (index < 0 || index >= m_equalizerBands.size())
        return;
    gain = qBound(-12.0, gain, 12.0);
    if (qFuzzyCompare(m_equalizerBands.at(index), gain))
        return;
    m_equalizerBands[index] = gain;
    save();
    applyEqualizer();
    Q_EMIT equalizerChanged();
}

/**
 * @brief Applies a named equalizer preset and enables the equalizer.
 *
 * Supported presets: "bass", "treble", "voice". Unknown names reset to flat.
 *
 * @param preset Preset name (case-sensitive).
 * @sideeffect Enables equalizer, persists state, emits equalizerChanged().
 */
void AdvancedPlaybackController::applyEqualizerPreset(const QString& preset)
{
    // Set predefined gain curves for common listening profiles
    if (preset == QStringLiteral("bass"))
        m_equalizerBands = {6, 5, 4, 2, 0, -1, -2, -2, -1, 0};
    else if (preset == QStringLiteral("treble"))
        m_equalizerBands = {-2, -2, -1, 0, 1, 2, 4, 5, 6, 6};
    else if (preset == QStringLiteral("voice"))
        m_equalizerBands = {-3, -2, -1, 1, 3, 4, 4, 2, 0, -2};
    else
        m_equalizerBands.fill(0.0);
    m_equalizerEnabled = true;
    save();
    applyEqualizer();
    Q_EMIT equalizerChanged();
}

/**
 * @brief Resets all equalizer bands to 0 dB and disables the equalizer.
 * @sideeffect Persists state and emits equalizerChanged().
 */
void AdvancedPlaybackController::resetEqualizer()
{
    m_equalizerBands.fill(0.0);
    m_equalizerEnabled = false;
    save();
    applyEqualizer();
    Q_EMIT equalizerChanged();
}

/**
 * @brief Sets the audio visualization mode.
 * @param mode "off", "waves" (waveform), or "spectrum" (spectrogram).
 * @sideeffect Persists setting, applies to mpv, emits visualizationChanged().
 */
void AdvancedPlaybackController::setVisualization(const QString& mode)
{
    if (m_visualization == mode)
        return;
    m_visualization = mode;
    save();
    applyVisualization();
    Q_EMIT visualizationChanged();
}

/**
 * @brief Sets the stereo 3D input format.
 * @param input Source format (e.g. "off", "sbs2l", "arcd").
 * @sideeffect Persists setting, applies to mpv, emits stereoChanged().
 */
void AdvancedPlaybackController::setStereoInput(const QString& input)
{
    if (m_stereoInput == input)
        return;
    m_stereoInput = input;
    save();
    applyStereo();
    Q_EMIT stereoChanged();
}

/**
 * @brief Sets the stereo 3D output format.
 * @param output Target format (e.g. "arcd", "sbs2l").
 * @sideeffect Persists setting, applies to mpv, emits stereoChanged().
 */
void AdvancedPlaybackController::setStereoOutput(const QString& output)
{
    if (m_stereoOutput == output)
        return;
    m_stereoOutput = output;
    save();
    applyStereo();
    Q_EMIT stereoChanged();
}

/**
 * @brief Sets the HDR tone mapping algorithm.
 * @param toneMapping Algorithm name ("auto", "hable", "mobius", "bt.2390", etc.).
 * @sideeffect Persists setting, applies to mpv, emits hdrChanged().
 */
void AdvancedPlaybackController::setToneMapping(const QString& toneMapping)
{
    if (m_toneMapping == toneMapping)
        return;
    m_toneMapping = toneMapping;
    save();
    applyHdr();
    Q_EMIT hdrChanged();
}

/**
 * @brief Sets the target peak brightness for tone mapping.
 * @param peak Brightness in nits, clamped to [100, 1000].
 * @sideeffect Persists setting, applies to mpv, emits hdrChanged().
 */
void AdvancedPlaybackController::setTargetPeak(int peak)
{
    peak = qBound(100, peak, 1000);
    if (m_targetPeak == peak)
        return;
    m_targetPeak = peak;
    save();
    applyHdr();
    Q_EMIT hdrChanged();
}

/**
 * @brief Applies all advanced processing settings to the current mpv player.
 *
 * Convenience method called on fileLoaded to restore the full processing chain:
 * equalizer, visualization, stereo 3D, and HDR tone mapping.
 */
void AdvancedPlaybackController::applyAll()
{
    applyEqualizer();
    applyVisualization();
    applyStereo();
    applyHdr();
}

/**
 * @brief Sends the current equalizer filter graph to mpv via the af command.
 *
 * Removes any previous @cine_eq filter, then adds lavfi equalizer filters
 * for each active band followed by a limiter at 0.95.
 * Does nothing if equalizer is disabled or no player is set.
 */
void AdvancedPlaybackController::applyEqualizer()
{
    if (!m_player)
        return;
    // Remove any previous equalizer filter chain
    m_player->runCommandAsync({QStringLiteral("af"), QStringLiteral("remove"), QStringLiteral("@cine_eq")});
    if (!m_equalizerEnabled)
        return;
    // Build individual equalizer filters for each non-zero band
    QStringList filters;
    for (int i = 0; i < m_equalizerBands.size(); ++i)
        filters.append(QStringLiteral("equalizer=f=%1:t=q:w=1:g=%2")
                           .arg(EqualizerFrequencies[i]).arg(m_equalizerBands.at(i), 0, 'f', 1));
    // Add a limiter after the equalizer to prevent clipping
    filters.append(QStringLiteral("alimiter=limit=0.95"));
    // Apply the combined filter chain as a labeled lavfi filter
    m_player->runCommandAsync({QStringLiteral("af"), QStringLiteral("add"),
                               QStringLiteral("@cine_eq:lavfi=[%1]").arg(filters.join(QLatin1Char(',')))});
}

/**
 * @brief Configures lavfi-complex for audio visualization when no video track is active.
 *
 * Uses showwaves (waveform) or showspectrum (spectrogram) depending on
 * m_visualization mode. Clears lavfi-complex when visualization is off
 * or a video track is present.
 */
void AdvancedPlaybackController::applyVisualization()
{
    if (!m_player)
        return;
    // Skip visualization when a video track is actively playing
    const bool hasVideo = m_player->mpvOption(QStringLiteral("vid")).toInt() > 0;
    if (m_visualization == QStringLiteral("off") || hasVideo)
    {
        // Clear any existing lavfi-complex
        if (!m_player->mpvOption(QStringLiteral("lavfi-complex")).toString().isEmpty())
            m_player->setMpvOption(QStringLiteral("lavfi-complex"), QString());
        return;
    }
    // Ensure an audio track is selected (aid > 0)
    const int aid = m_player->mpvOption(QStringLiteral("aid")).toInt();
    if (aid <= 0)
        return;
    // Pick filter based on selected visualization mode
    const QString filter = m_visualization == QStringLiteral("waves")
        ? QStringLiteral("showwaves=s=1280x720:mode=cline:rate=30:colors=10c7d1")
        : QStringLiteral("showspectrum=s=1280x720:slide=scroll:mode=combined:color=intensity:scale=log");
    // Wire audio output to both the audio sink and the visualization filter
    m_player->setMpvOption(QStringLiteral("lavfi-complex"),
                           QStringLiteral("[aid%1]asplit[ao],[viz];[viz]%2[vo]").arg(aid).arg(filter));
}

/**
 * @brief Adds or removes the stereo3d video filter via mpv's vf command.
 *
 * Removes any previous @cine_stereo filter, then adds a new one if
 * m_stereoInput is not "off".
 */
void AdvancedPlaybackController::applyStereo()
{
    if (!m_player)
        return;
    // Remove any previous stereo3d filter
    m_player->runCommandAsync({QStringLiteral("vf"), QStringLiteral("remove"), QStringLiteral("@cine_stereo")});
    if (m_stereoInput != QStringLiteral("off"))
        // Apply stereo3d conversion from input format to output format
        m_player->runCommandAsync({QStringLiteral("vf"), QStringLiteral("add"),
                                   QStringLiteral("@cine_stereo:lavfi=[stereo3d=%1:%2]")
                                       .arg(m_stereoInput, m_stereoOutput)});
}

/**
 * @brief Writes HDR tone mapping options to mpv.
 *
 * Sets tone-mapping algorithm, target-peak brightness, and enables
 * hdr-compute-peak for dynamic HDR metadata analysis.
 */
void AdvancedPlaybackController::applyHdr()
{
    if (!m_player)
        return;
    // Configure HDR rendering pipeline via mpv options
    m_player->setMpvOption(QStringLiteral("tone-mapping"), m_toneMapping);
    m_player->setMpvOption(QStringLiteral("target-peak"), m_targetPeak);
    m_player->setMpvOption(QStringLiteral("hdr-compute-peak"), QStringLiteral("yes"));
}

/**
 * @brief Persists all advanced settings to QSettings.
 *
 * Writes equalizer enable state, per-band gains, visualization mode,
 * stereo 3D formats, and HDR configuration.
 */
void AdvancedPlaybackController::save() const
{
    QSettings settings;
    QVariantList gains;
    for (double gain : m_equalizerBands)
        gains.append(gain);
    settings.setValue(QStringLiteral("audio/equalizerEnabled"), m_equalizerEnabled);
    settings.setValue(QStringLiteral("audio/equalizerBands"), gains);
    settings.setValue(QStringLiteral("audio/visualization"), m_visualization);
    settings.setValue(QStringLiteral("video/stereoInput"), m_stereoInput);
    settings.setValue(QStringLiteral("video/stereoOutput"), m_stereoOutput);
    settings.setValue(QStringLiteral("video/toneMapping"), m_toneMapping);
    settings.setValue(QStringLiteral("video/targetPeak"), m_targetPeak);
}
