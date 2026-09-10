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

#include "player/CineMpvItem.h"

#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

/**
 * @class AdvancedPlaybackController
 * @brief Manages advanced audio/video processing features for the mpv player.
 *
 * @details
 * Provides QML-exposed controls for:
 * - 10-band graphic equalizer with presets
 * - Audio visualization (waveform / spectrogram)
 * - Stereo 3D input/output conversion
 * - HDR tone mapping configuration
 *
 * All settings are persisted via QSettings and reapplied on file load.
 */
class AdvancedPlaybackController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(CineMpvItem* player READ player WRITE setPlayer NOTIFY playerChanged)
    Q_PROPERTY(bool equalizerEnabled READ equalizerEnabled WRITE setEqualizerEnabled NOTIFY equalizerChanged)
    Q_PROPERTY(QVariantList equalizerBands READ equalizerBands NOTIFY equalizerChanged)
    Q_PROPERTY(QString visualization READ visualization WRITE setVisualization NOTIFY visualizationChanged)
    Q_PROPERTY(QString stereoInput READ stereoInput WRITE setStereoInput NOTIFY stereoChanged)
    Q_PROPERTY(QString stereoOutput READ stereoOutput WRITE setStereoOutput NOTIFY stereoChanged)
    Q_PROPERTY(QString toneMapping READ toneMapping WRITE setToneMapping NOTIFY hdrChanged)
    Q_PROPERTY(int targetPeak READ targetPeak WRITE setTargetPeak NOTIFY hdrChanged)

public:
    /**
     * @brief Constructs an AdvancedPlaybackController and restores persisted settings.
     * @param parent Optional QObject parent for Qt ownership.
     */
    explicit AdvancedPlaybackController(QObject* parent = nullptr);

    /** @brief Returns the assigned CineMpvItem instance. */
    CineMpvItem* player() const;

    /** @brief Assigns the player and wires the applyAll handler to fileLoaded. */
    void setPlayer(CineMpvItem* player);

    /** @brief Returns whether the graphic equalizer is enabled. */
    bool equalizerEnabled() const;

    /** @brief Returns a QVariantList copy of the 10-band equalizer gains. */
    QVariantList equalizerBands() const;

    /** @brief Returns the current visualization mode ("off", "waves", "spectrum"). */
    QString visualization() const;

    /** @brief Returns the stereo 3D input format (e.g. "off", "sbs2l", "arcd"). */
    QString stereoInput() const;

    /** @brief Returns the stereo 3D output format (e.g. "arcd", "sbs2l"). */
    QString stereoOutput() const;

    /** @brief Returns the HDR tone mapping algorithm (e.g. "auto", "hable", "mobius"). */
    QString toneMapping() const;

    /** @brief Returns the target peak brightness in nits for tone mapping. */
    int targetPeak() const;

    /** @brief Enables or disables the graphic equalizer. */
    void setEqualizerEnabled(bool enabled);

    /** @brief Sets gain for one equalizer band and updates mpv. */
    Q_INVOKABLE void setEqualizerBand(int index, double gain);

    /** @brief Applies a named equalizer preset ("bass", "treble", "voice"). */
    Q_INVOKABLE void applyEqualizerPreset(const QString& preset);

    /** @brief Resets all equalizer bands to 0 dB and disables the equalizer. */
    Q_INVOKABLE void resetEqualizer();

    /** @brief Sets the audio visualization mode. */
    void setVisualization(const QString& mode);

    /** @brief Sets the stereo 3D input format. */
    void setStereoInput(const QString& input);

    /** @brief Sets the stereo 3D output format. */
    void setStereoOutput(const QString& output);

    /** @brief Sets the HDR tone mapping algorithm. */
    void setToneMapping(const QString& toneMapping);

    /** @brief Sets the target peak brightness in nits. */
    void setTargetPeak(int peak);

    /** @brief Applies all advanced settings (equalizer, visualization, stereo, HDR). */
    Q_INVOKABLE void applyAll();

Q_SIGNALS:
    /** @brief Emitted when the assigned player instance changes. */
    void playerChanged();
    /** @brief Emitted when equalizer enable state or band gains change. */
    void equalizerChanged();
    /** @brief Emitted when the visualization mode changes. */
    void visualizationChanged();
    /** @brief Emitted when stereo 3D input or output format changes. */
    void stereoChanged();
    /** @brief Emitted when tone mapping algorithm or target peak changes. */
    void hdrChanged();

private:
    /** @brief Sends the current equalizer filter graph to mpv via af command. */
    void applyEqualizer();
    /** @brief Configures lavfi-complex for waveform or spectrogram visualization. */
    void applyVisualization();
    /** @brief Adds or removes the stereo3d video filter via vf command. */
    void applyStereo();
    /** @brief Writes tone-mapping, target-peak, and hdr-compute-peak options to mpv. */
    void applyHdr();
    /** @brief Persists all current advanced settings to QSettings. */
    void save() const;

    /** @brief The active mpv player item. */
    CineMpvItem* m_player{nullptr};
    /** @brief Whether the 10-band graphic equalizer is active. */
    bool m_equalizerEnabled{false};
    /** @brief Per-band gain values in dB, range [-12, 12], index 0 = 31 Hz. */
    QList<double> m_equalizerBands{0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    /** @brief Audio visualization mode: "off", "waves", or "spectrum". */
    QString m_visualization{QStringLiteral("off")};
    /** @brief Stereo 3D source format (e.g. "off", "sbs2l", "arcd"). */
    QString m_stereoInput{QStringLiteral("off")};
    /** @brief Stereo 3D output format (e.g. "arcd", "sbs2l"). */
    QString m_stereoOutput{QStringLiteral("arcd")};
    /** @brief HDR tone mapping algorithm ("auto", "hable", "mobius", etc.). */
    QString m_toneMapping{QStringLiteral("auto")};
    /** @brief Target peak brightness in nits, range [100, 1000]. */
    int m_targetPeak{203};
};
