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

#include "player/CineMpvItem.h"

#include "utils/MediaUtils.h"
#include "utils/PathUtils.h"

#include <MpvController>

#include <QDir>
#include <QtGlobal>

#include <algorithm>

CineMpvItem::CineMpvItem(QQuickItem* parent)
    : MpvAbstractItem(parent)
{
    observeProperty(QStringLiteral("media-title"), MPV_FORMAT_STRING);
    observeProperty(QStringLiteral("path"), MPV_FORMAT_STRING);
    observeProperty(QStringLiteral("time-pos"), MPV_FORMAT_DOUBLE);
    observeProperty(QStringLiteral("duration"), MPV_FORMAT_DOUBLE);
    observeProperty(QStringLiteral("pause"), MPV_FORMAT_FLAG);
    observeProperty(QStringLiteral("idle-active"), MPV_FORMAT_FLAG);
    observeProperty(QStringLiteral("volume"), MPV_FORMAT_INT64);
    observeProperty(QStringLiteral("mute"), MPV_FORMAT_FLAG);
    observeProperty(QStringLiteral("speed"), MPV_FORMAT_DOUBLE);
    observeProperty(QStringLiteral("playlist-pos"), MPV_FORMAT_INT64);
    observeProperty(QStringLiteral("playlist-count"), MPV_FORMAT_INT64);
    observeProperty(QStringLiteral("track-list"), MPV_FORMAT_NODE);
    observeProperty(QStringLiteral("chapter-list"), MPV_FORMAT_NODE);
    observeProperty(QStringLiteral("sid"), MPV_FORMAT_NODE);
    observeProperty(QStringLiteral("aid"), MPV_FORMAT_NODE);
    observeProperty(QStringLiteral("vid"), MPV_FORMAT_NODE);

    setupConnections();
    configureDefaults();

    connect(this, &MpvAbstractItem::ready, this, [this]() {
        if (m_rendererReady)
        {
            return;
        }
        m_rendererReady = true;
        Q_EMIT rendererReadyChanged();
        flushPendingLoad();
    });
}

void CineMpvItem::setupConnections()
{
    connect(mpvController(), &MpvController::propertyChanged, this, &CineMpvItem::onPropertyChanged,
            Qt::QueuedConnection);
    connect(mpvController(), &MpvController::fileStarted, this, [this] {
        resetVideoGeometry();
        Q_EMIT fileStarted();
    }, Qt::QueuedConnection);
    connect(mpvController(), &MpvController::fileLoaded, this, &CineMpvItem::fileLoaded, Qt::QueuedConnection);
    connect(mpvController(), &MpvController::endFile, this, &CineMpvItem::endFile, Qt::QueuedConnection);
    connect(mpvController(), &MpvController::videoReconfig, this, [this] {
        updateVideoGeometry();
        Q_EMIT videoReconfigured();
    }, Qt::QueuedConnection);
}

void CineMpvItem::configureDefaults()
{
    setProperty(QStringLiteral("osc"), false);
    setProperty(QStringLiteral("load-osd-console"), true);
    setProperty(QStringLiteral("keep-open"), true);
    setProperty(QStringLiteral("ytdl"), true);
    const QString ytDlpPath = PathUtils::mediaToolPath(QStringLiteral("yt-dlp"));
    if (!ytDlpPath.isEmpty())
    {
        command(QStringList{QStringLiteral("change-list"), QStringLiteral("script-opts"), QStringLiteral("append"),
                            QStringLiteral("ytdl_hook-ytdl_path=%1")
                                .arg(QDir::fromNativeSeparators(ytDlpPath))});
    }
    setProperty(QStringLiteral("volume-max"), 200);
    setProperty(QStringLiteral("screenshot-directory"), PathUtils::screenshotsDir());
    setProperty(QStringLiteral("screenshot-template"), QStringLiteral("cine_%n"));
    setProperty(QStringLiteral("audio-file-auto"), QStringLiteral("fuzzy"));
    setProperty(QStringLiteral("sub-auto"), QStringLiteral("fuzzy"));
    setProperty(QStringLiteral("sub-file-paths"),
                QStringLiteral("sub:subs:subtitles:Sub:Subs:Subtitles:srt:srts:Srt:Srts"));
    setProperty(QStringLiteral("sub-border-size"), 2);
    setProperty(QStringLiteral("sub-shadow-offset"), 0.6);
    setProperty(QStringLiteral("sub-border-color"), QStringLiteral("#B6000000"));
    setProperty(QStringLiteral("sub-shadow-color"), QStringLiteral("#97000000"));
    setProperty(QStringLiteral("sub-color"), QStringLiteral("#ebebeb"));
    setProperty(QStringLiteral("sub-use-margins"), false);
    setProperty(QStringLiteral("sub-font"), QStringLiteral("Adwaita Sans SemiBold"));
    setProperty(QStringLiteral("osd-font"), QStringLiteral("Adwaita Sans"));
    setProperty(QStringLiteral("osd-bold"), true);
    setProperty(QStringLiteral("osd-bar"), false);
    setProperty(QStringLiteral("osd-margin-x"), 66);
    setProperty(QStringLiteral("osd-margin-y"), 66);
    setProperty(QStringLiteral("gpu-api"), QStringLiteral("opengl"));
#ifdef Q_OS_WIN
    setProperty(QStringLiteral("hwdec"), QStringLiteral("no"));
#else
    setProperty(QStringLiteral("hwdec"), QStringLiteral("auto-safe"));
#endif

    command(QStringList{QStringLiteral("change-list"), QStringLiteral("watch-later-options"), QStringLiteral("remove"),
                        QStringLiteral("vid")});
    command(QStringList{QStringLiteral("change-list"), QStringLiteral("watch-later-options"), QStringLiteral("remove"),
                        QStringLiteral("aid")});
    command(QStringList{QStringLiteral("change-list"), QStringLiteral("watch-later-options"), QStringLiteral("remove"),
                        QStringLiteral("volume")});
}

void CineMpvItem::onPropertyChanged(const QString& property, const QVariant& value)
{
    Q_EMIT mpvPropertyChanged(property, value);

    if (property == QStringLiteral("media-title"))
    {
        const QString title = value.toString();
        if (m_mediaTitle != title)
        {
            m_mediaTitle = title;
            Q_EMIT mediaTitleChanged();
        }
    }
    else if (property == QStringLiteral("path"))
    {
        const QString path = value.toString();
        if (m_currentPath != path)
        {
            m_currentPath = path;
            Q_EMIT currentPathChanged();
        }
    }
    else if (property == QStringLiteral("time-pos"))
    {
        double position = value.toDouble();
        if (m_duration > 0)
            position = std::clamp(position, 0.0, m_duration);
        if (!qFuzzyCompare(m_position, position))
        {
            m_position = position;
            updateFormattedPosition();
            Q_EMIT positionChanged();
        }
    }
    else if (property == QStringLiteral("duration"))
    {
        m_duration = value.toDouble();
        updateFormattedDuration();
        Q_EMIT durationChanged();
    }
    else if (property == QStringLiteral("pause"))
    {
        const bool pauseValue = value.toBool();
        if (m_pause != pauseValue)
        {
            m_pause = pauseValue;
            Q_EMIT pauseChanged();
        }
    }
    else if (property == QStringLiteral("idle-active"))
    {
        const bool idleValue = value.toBool();
        if (m_idle != idleValue)
        {
            m_idle = idleValue;
            Q_EMIT idleChanged();
        }
    }
    else if (property == QStringLiteral("volume"))
    {
        const int volumeValue = value.toInt();
        if (m_volume != volumeValue)
        {
            m_volume = volumeValue;
            Q_EMIT volumeChanged();
        }
    }
    else if (property == QStringLiteral("mute"))
    {
        const bool muteValue = value.toBool();
        if (m_muted != muteValue)
        {
            m_muted = muteValue;
            Q_EMIT muteChanged();
        }
    }
    else if (property == QStringLiteral("speed"))
    {
        const double speedValue = value.toDouble();
        if (!qFuzzyCompare(m_speed, speedValue))
        {
            m_speed = speedValue;
            Q_EMIT speedChanged();
        }
    }
    else if (property == QStringLiteral("playlist-pos"))
    {
        const int position = value.toInt();
        if (m_playlistPosition != position)
        {
            m_playlistPosition = position;
            Q_EMIT playlistPositionChanged();
        }
    }
    else if (property == QStringLiteral("playlist-count"))
    {
        const int count = value.toInt();
        if (m_playlistCount != count)
        {
            m_playlistCount = count;
            Q_EMIT playlistCountChanged();
        }
    }
    else if (property == QStringLiteral("track-list"))
    {
        Q_EMIT tracksChanged(value.toList());
    }
    else if (property == QStringLiteral("chapter-list"))
    {
        Q_EMIT chaptersChanged(value.toList());
    }
}

void CineMpvItem::updateFormattedPosition()
{
    m_formattedPosition = MediaUtils::formatTime(m_position);
}

void CineMpvItem::updateFormattedDuration()
{
    m_formattedDuration = MediaUtils::formatTime(m_duration);
}

QString CineMpvItem::mediaTitle() const
{
    return m_mediaTitle;
}
QString CineMpvItem::currentPath() const
{
    return m_currentPath;
}
double CineMpvItem::position() const
{
    return m_position;
}
double CineMpvItem::duration() const
{
    return m_duration;
}
QString CineMpvItem::formattedPosition() const
{
    return m_formattedPosition;
}
QString CineMpvItem::formattedDuration() const
{
    return m_formattedDuration;
}
bool CineMpvItem::pause() const
{
    return m_pause;
}
bool CineMpvItem::idle() const
{
    return m_idle;
}
int CineMpvItem::volume() const
{
    return m_volume;
}
bool CineMpvItem::muted() const
{
    return m_muted;
}
double CineMpvItem::speed() const
{
    return m_speed;
}
int CineMpvItem::playlistPosition() const
{
    return m_playlistPosition;
}
int CineMpvItem::playlistCount() const
{
    return m_playlistCount;
}
bool CineMpvItem::rendererReady() const
{
    return m_rendererReady;
}
int CineMpvItem::videoWidth() const { return m_videoWidth; }
int CineMpvItem::videoHeight() const { return m_videoHeight; }
double CineMpvItem::videoAspectRatio() const { return m_videoAspectRatio; }

void CineMpvItem::updateVideoGeometry()
{
    const QVariantMap parameters = getProperty(QStringLiteral("video-out-params")).toMap();
    int width = parameters.value(QStringLiteral("dw")).toInt();
    int height = parameters.value(QStringLiteral("dh")).toInt();
    if (width <= 0 || height <= 0)
    {
        width = parameters.value(QStringLiteral("w")).toInt();
        height = parameters.value(QStringLiteral("h")).toInt();
    }

    double aspectRatio = parameters.value(QStringLiteral("aspect")).toDouble();
    if (aspectRatio <= 0.0 && width > 0 && height > 0)
        aspectRatio = static_cast<double>(width) / static_cast<double>(height);

    const int rotation = qAbs(parameters.value(QStringLiteral("rotate")).toInt()) % 360;
    if ((rotation == 90 || rotation == 270) && width > 0 && height > 0)
    {
        std::swap(width, height);
        if (aspectRatio > 0.0)
            aspectRatio = 1.0 / aspectRatio;
    }

    if (m_videoWidth == width && m_videoHeight == height
        && qFuzzyCompare(m_videoAspectRatio, aspectRatio))
        return;
    m_videoWidth = width;
    m_videoHeight = height;
    m_videoAspectRatio = aspectRatio;
    Q_EMIT videoGeometryChanged();
}

void CineMpvItem::resetVideoGeometry()
{
    if (m_videoWidth == 0 && m_videoHeight == 0 && m_videoAspectRatio == 0.0)
        return;
    m_videoWidth = 0;
    m_videoHeight = 0;
    m_videoAspectRatio = 0.0;
    Q_EMIT videoGeometryChanged();
}

void CineMpvItem::setPosition(double value)
{
    if (m_duration > 0)
        value = std::clamp(value, 0.0, m_duration);
    if (qFuzzyCompare(m_position, value))
    {
        return;
    }
    commandAsync(QStringList{QStringLiteral("seek"), QString::number(value, 'f', 3), QStringLiteral("absolute")});
}

void CineMpvItem::setPause(bool value)
{
    if (m_pause == value)
    {
        return;
    }
    m_pause = value;
    Q_EMIT pauseChanged();
    setPropertyAsync(QStringLiteral("pause"), value);
}

void CineMpvItem::setVolume(int value)
{
    value = std::clamp(value, 0, MaxVolume);
    if (m_volume == value)
    {
        return;
    }
    m_volume = value;
    Q_EMIT volumeChanged();
    setPropertyAsync(QStringLiteral("volume"), value);
}

void CineMpvItem::setMuted(bool value)
{
    if (m_muted == value)
    {
        return;
    }
    setPropertyAsync(QStringLiteral("mute"), value);
}

void CineMpvItem::setSpeed(double value)
{
    value = std::clamp(value, MinSpeed, MaxSpeed);
    if (qFuzzyCompare(m_speed, value))
    {
        return;
    }
    setPropertyAsync(QStringLiteral("speed"), value);
}

void CineMpvItem::loadFile(const QString& path, const QString& mode)
{
    if (path.trimmed().isEmpty())
    {
        return;
    }
    m_currentPath = path;
    Q_EMIT currentPathChanged();

    if (!m_rendererReady)
    {
        m_pendingLoadPath = path;
        m_pendingLoadMode = mode.isEmpty() ? QStringLiteral("replace") : mode;
        m_hasPendingLoad = true;
        return;
    }

    loadFileNow(path, mode);
}

void CineMpvItem::flushPendingLoad()
{
    if (!m_hasPendingLoad)
    {
        return;
    }

    const QString path = m_pendingLoadPath;
    const QString mode = m_pendingLoadMode.isEmpty() ? QStringLiteral("replace") : m_pendingLoadMode;
    m_pendingLoadPath.clear();
    m_pendingLoadMode.clear();
    m_hasPendingLoad = false;

    loadFileNow(path, mode);
}

void CineMpvItem::loadFileNow(const QString& path, const QString& mode)
{
    // lavfi-complex graphs contain track IDs from the current file and must not
    // survive into a replacement load. AdvancedPlaybackController reapplies the
    // selected visualization after the new file reports fileLoaded.
    setPropertyAsync(QStringLiteral("lavfi-complex"), QString());
    commandAsync(QStringList{QStringLiteral("loadfile"), path, mode.isEmpty() ? QStringLiteral("replace") : mode});
}

void CineMpvItem::addSubtitle(const QString& path)
{
    commandAsync(QStringList{QStringLiteral("sub-add"), path, QStringLiteral("select")});
}

void CineMpvItem::addAudio(const QString& path)
{
    commandAsync(QStringList{QStringLiteral("audio-add"), path, QStringLiteral("select")});
}

void CineMpvItem::seekRelative(double seconds)
{
    commandAsync(QStringList{QStringLiteral("seek"), QString::number(seconds, 'f', 3), QStringLiteral("relative")});
}

void CineMpvItem::setTrack(const QString& property, int trackId)
{
    setPropertyAsync(property, trackId);
}

void CineMpvItem::setTrackDisabled(const QString& property)
{
    setPropertyAsync(property, QStringLiteral("no"));
}

void CineMpvItem::setMpvOption(const QString& name, const QVariant& value)
{
    setPropertyAsync(name, value);
}

QVariant CineMpvItem::mpvOption(const QString& name)
{
    return getProperty(name);
}

void CineMpvItem::runCommand(const QStringList& params)
{
    command(params);
}

void CineMpvItem::runCommandAsync(const QStringList& params)
{
    commandAsync(params);
}

quint64 CineMpvItem::mpvClientApiVersion()
{
    return static_cast<quint64>(mpv_client_api_version());
}

qint64 CineMpvItem::mpvInternalTimeUs()
{
    mpv_handle* handle = mpvController()->mpv();
    return handle ? static_cast<qint64>(mpv_get_time_us(handle)) : 0;
}
