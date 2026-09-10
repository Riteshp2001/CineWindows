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

class CineMpvItem;

/**
 * @class VideoOptionsController
 * @brief Provides QML-exposed controls for video filter and playback adjustments.
 * @details Wraps mpv filter commands for aspect ratio, crop, rotation, flip, zoom,
 *          contrast, brightness, gamma, saturation, hue, subtitle delay, audio delay,
 *          and playback speed. All adjustments are applied via mpv properties and
 *          video filters, with helpers to reset individual or all options.
 */
class VideoOptionsController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(CineMpvItem* player READ player WRITE setPlayer NOTIFY playerChanged)

public:
    /**
     * @brief Constructs a VideoOptionsController with an optional parent.
     * @param parent Optional QObject parent for Qt ownership.
     */
    explicit VideoOptionsController(QObject* parent = nullptr);

    /**
     * @brief Returns the currently assigned mpv player instance.
     * @return Pointer to the CineMpvItem, or nullptr if none is set.
     */
    CineMpvItem* player() const;

    /**
     * @brief Sets the mpv player instance on which all video options operate.
     * @param player Pointer to the CineMpvItem to assign.
     */
    void setPlayer(CineMpvItem* player);

    /**
     * @brief Resets all video and playback options to their defaults.
     */
    Q_INVOKABLE void resetAll();

    /**
     * @brief Sets the video aspect ratio.
     * @param ratio Aspect ratio string (e.g. "16:9", "4:3", "-1" for automatic).
     */
    Q_INVOKABLE void setAspectRatio(const QString& ratio);

    /**
     * @brief Sets the video crop ratio.
     * @param ratio Crop ratio string (e.g. "16:9", "4:3", "disabled" to remove).
     */
    Q_INVOKABLE void setCropRatio(const QString& ratio);

    /**
     * @brief Rotates the video 90 degrees counter-clockwise.
     */
    Q_INVOKABLE void rotateLeft();

    /**
     * @brief Rotates the video 90 degrees clockwise.
     */
    Q_INVOKABLE void rotateRight();

    /**
     * @brief Resets the video rotation angle to zero.
     */
    Q_INVOKABLE void resetRotation();

    /**
     * @brief Flips the video horizontally (mirror effect).
     */
    Q_INVOKABLE void flipHorizontal();

    /**
     * @brief Flips the video vertically.
     */
    Q_INVOKABLE void flipVertical();

    /**
     * @brief Resets both horizontal and vertical flip to normal.
     */
    Q_INVOKABLE void resetFlip();

    /**
     * @brief Sets the video zoom level.
     * @param value Zoom factor (1.0 = no zoom, >1.0 = zoom in, <1.0 = zoom out).
     */
    Q_INVOKABLE void setZoom(double value);

    /**
     * @brief Sets the video contrast level.
     * @param value Contrast value in mpv's range (typically -100 to 100, 0 = default).
     */
    Q_INVOKABLE void setContrast(int value);

    /**
     * @brief Sets the video brightness level.
     * @param value Brightness value in mpv's range (typically -100 to 100, 0 = default).
     */
    Q_INVOKABLE void setBrightness(int value);

    /**
     * @brief Sets the video gamma level.
     * @param value Gamma value in mpv's range (typically -100 to 100, 0 = default).
     */
    Q_INVOKABLE void setGamma(int value);

    /**
     * @brief Sets the video saturation level.
     * @param value Saturation value in mpv's range (typically -100 to 100, 0 = default).
     */
    Q_INVOKABLE void setSaturation(int value);

    /**
     * @brief Sets the video hue shift.
     * @param value Hue value in mpv's range (typically -100 to 100, 0 = default).
     */
    Q_INVOKABLE void setHue(int value);

    /**
     * @brief Sets the subtitle display delay.
     * @param value Delay in seconds (positive delays subtitles, negative shows them earlier).
     */
    Q_INVOKABLE void setSubtitleDelay(double value);

    /**
     * @brief Sets the audio output delay.
     * @param value Delay in seconds (positive delays audio, negative shows it earlier).
     */
    Q_INVOKABLE void setAudioDelay(double value);

    /**
     * @brief Sets the playback speed.
     * @param value Speed multiplier (1.0 = normal, 2.0 = double speed, etc.).
     */
    Q_INVOKABLE void setSpeed(double value);

Q_SIGNALS:
    /**
     * @brief Emitted when the player property changes.
     */
    void playerChanged();

private:
    /** @brief Currently assigned mpv player instance. */
    CineMpvItem* m_player{nullptr};
};
