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

#include "player/VideoOptionsController.h"

#include "player/CineMpvItem.h"

VideoOptionsController::VideoOptionsController(QObject* parent)
    : QObject(parent)
{}

CineMpvItem* VideoOptionsController::player() const
{
    return m_player;
}

/**
 * @brief Assigns the mpv player instance and emits playerChanged.
 * @details Guards against redundant assignments to avoid unnecessary signal emission.
 */
void VideoOptionsController::setPlayer(CineMpvItem* player)
{
    if (m_player == player)
    {
        return;
    }
    m_player = player;
    Q_EMIT playerChanged();
}

/**
 * @brief Resets all video and playback options to their default values.
 * @details Calls each individual setter with its default value:
 *          aspect ratio to auto, crop disabled, rotation/flip reset,
 *          zoom/contrast/brightness/gamma/saturation/hue to 0,
 *          subtitle/audio delay to 0, and speed to 1.0.
 */
void VideoOptionsController::resetAll()
{
    setAspectRatio(QStringLiteral("no"));
    setCropRatio(QStringLiteral(""));
    resetRotation();
    resetFlip();
    setZoom(0);
    setContrast(0);
    setBrightness(0);
    setGamma(0);
    setSaturation(0);
    setHue(0);
    setSubtitleDelay(0);
    setAudioDelay(0);
    setSpeed(1.0);
}

/**
 * @brief Overrides the video aspect ratio via the mpv property.
 * @param ratio Aspect ratio string (e.g. "16:9", "4:3"). Empty string maps to auto ("no").
 */
void VideoOptionsController::setAspectRatio(const QString& ratio)
{
    if (m_player)
    {
        m_player->setMpvOption(QStringLiteral("video-aspect-override"), ratio.isEmpty() ? QStringLiteral("no") : ratio);
    }
}

/**
 * @brief Applies a crop ratio by computing pixel dimensions from the source video.
 * @details Parses the ratio string (supports "W:H" or decimal formats), reads the
 *          current video resolution via video-params, and computes the crop rectangle.
 *          Falls back to passing the raw ratio string to mpv if parsing fails.
 */
void VideoOptionsController::setCropRatio(const QString& ratio)
{
    if (!m_player)
    {
        return;
    }

    // Disable crop if the ratio is empty or explicitly "no"
    if (ratio.isEmpty() || ratio == QStringLiteral("no"))
    {
        m_player->setMpvOption(QStringLiteral("video-crop"), QStringLiteral(""));
        return;
    }

    // Parse aspect ratio (e.g. "16:9" or "1.33")
    double targetRatio = 0.0;
    QStringList parts = ratio.split(QLatin1Char(':'));
    if (parts.size() == 2)
    {
        bool ok1 = false, ok2 = false;
        double num = parts[0].toDouble(&ok1);
        double den = parts[1].toDouble(&ok2);
        if (ok1 && ok2 && den != 0.0)
        {
            targetRatio = num / den;
        }
    }
    else
    {
        bool ok = false;
        targetRatio = ratio.toDouble(&ok);
        if (!ok)
        {
            targetRatio = 0.0;
        }
    }

    if (targetRatio <= 0.0)
    {
        m_player->setMpvOption(QStringLiteral("video-crop"), QStringLiteral(""));
        return;
    }

    // Retrieve video parameters to calculate crop dimensions
    const QVariant paramsVar = m_player->mpvOption(QStringLiteral("video-params"));
    if (paramsVar.canConvert<QVariantMap>())
    {
        const QVariantMap params = paramsVar.toMap();
        const int w = params.value(QStringLiteral("w")).toInt();
        const int h = params.value(QStringLiteral("h")).toInt();
        if (w > 0 && h > 0)
        {
            const double currentRatio = static_cast<double>(w) / h;
            int new_w = w;
            int new_h = h;

            if (currentRatio > targetRatio)
            {
                // Image is wider than target: crop the sides
                new_w = static_cast<int>(h * targetRatio);
            }
            else
            {
                // Image is taller than target: crop the top and bottom
                new_h = static_cast<int>(w / targetRatio);
            }

            m_player->setMpvOption(QStringLiteral("video-crop"),
                                   QString::number(new_w) + QLatin1Char('x') + QString::number(new_h));
            return;
        }
    }

    // Fallback: pass the raw ratio to mpv
    m_player->setMpvOption(QStringLiteral("video-crop"), ratio);
}

/**
 * @brief Rotates the video 90 degrees counter-clockwise.
 * @details Reads the current rotation value and subtracts 90 degrees (mod 360).
 */
void VideoOptionsController::rotateLeft()
{
    if (m_player)
    {
        const int value = m_player->mpvOption(QStringLiteral("video-rotate")).toInt();
        m_player->setMpvOption(QStringLiteral("video-rotate"), (value + 270) % 360);
    }
}

/**
 * @brief Rotates the video 90 degrees clockwise.
 * @details Reads the current rotation value and adds 90 degrees (mod 360).
 */
void VideoOptionsController::rotateRight()
{
    if (m_player)
    {
        const int value = m_player->mpvOption(QStringLiteral("video-rotate")).toInt();
        m_player->setMpvOption(QStringLiteral("video-rotate"), (value + 90) % 360);
    }
}

/**
 * @brief Resets the video rotation angle to zero.
 */
void VideoOptionsController::resetRotation()
{
    if (m_player)
    {
        m_player->setMpvOption(QStringLiteral("video-rotate"), 0);
    }
}

/**
 * @brief Toggles the horizontal flip (mirror) filter.
 * @details Uses mpv's vf toggle command with a labeled filter for idempotent toggling.
 */
void VideoOptionsController::flipHorizontal()
{
    if (m_player)
    {
        m_player->runCommandAsync({QStringLiteral("vf"), QStringLiteral("toggle"), QStringLiteral("@hflip:hflip")});
    }
}

/**
 * @brief Toggles the vertical flip filter.
 */
void VideoOptionsController::flipVertical()
{
    if (m_player)
    {
        m_player->runCommandAsync({QStringLiteral("vf"), QStringLiteral("toggle"), QStringLiteral("@vflip:vflip")});
    }
}

/**
 * @brief Removes both horizontal and vertical flip filters.
 * @details Uses mpv's vf remove command on labeled filters to ensure clean reset.
 */
void VideoOptionsController::resetFlip()
{
    if (m_player)
    {
        m_player->runCommandAsync({QStringLiteral("vf"), QStringLiteral("remove"), QStringLiteral("@hflip")});
        m_player->runCommandAsync({QStringLiteral("vf"), QStringLiteral("remove"), QStringLiteral("@vflip")});
    }
}

/**
 * @brief Sets the video zoom level.
 * @param value Zoom factor (1.0 = no zoom, >1.0 = zoom in, <1.0 = zoom out).
 */
void VideoOptionsController::setZoom(double value)
{
    if (m_player)
        m_player->setMpvOption(QStringLiteral("video-zoom"), value);
}

/**
 * @brief Sets the video contrast level.
 * @param value Contrast value in mpv range (typically -100 to 100, 0 = default).
 */
void VideoOptionsController::setContrast(int value)
{
    if (m_player)
        m_player->setMpvOption(QStringLiteral("contrast"), value);
}

/**
 * @brief Sets the video brightness level.
 * @param value Brightness value in mpv range (typically -100 to 100, 0 = default).
 */
void VideoOptionsController::setBrightness(int value)
{
    if (m_player)
        m_player->setMpvOption(QStringLiteral("brightness"), value);
}

/**
 * @brief Sets the video gamma level.
 * @param value Gamma value in mpv range (typically -100 to 100, 0 = default).
 */
void VideoOptionsController::setGamma(int value)
{
    if (m_player)
        m_player->setMpvOption(QStringLiteral("gamma"), value);
}

/**
 * @brief Sets the video saturation level.
 * @param value Saturation value in mpv range (typically -100 to 100, 0 = default).
 */
void VideoOptionsController::setSaturation(int value)
{
    if (m_player)
        m_player->setMpvOption(QStringLiteral("saturation"), value);
}

/**
 * @brief Sets the video hue shift.
 * @param value Hue value in mpv range (typically -100 to 100, 0 = default).
 */
void VideoOptionsController::setHue(int value)
{
    if (m_player)
        m_player->setMpvOption(QStringLiteral("hue"), value);
}

/**
 * @brief Sets the subtitle display delay.
 * @param value Delay in seconds (positive delays subtitles, negative shows them earlier).
 */
void VideoOptionsController::setSubtitleDelay(double value)
{
    if (m_player)
        m_player->setMpvOption(QStringLiteral("sub-delay"), value);
}

/**
 * @brief Sets the audio output delay.
 * @param value Delay in seconds (positive delays audio, negative shows it earlier).
 */
void VideoOptionsController::setAudioDelay(double value)
{
    if (m_player)
        m_player->setMpvOption(QStringLiteral("audio-delay"), value);
}

/**
 * @brief Sets the playback speed.
 * @param value Speed multiplier (1.0 = normal, 2.0 = double speed, etc.).
 */
void VideoOptionsController::setSpeed(double value)
{
    if (m_player)
        m_player->setSpeed(value);
}
