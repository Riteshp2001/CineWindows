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

#include "player/ThumbnailController.h"

#include "player/CineMpvItem.h"

#include <QColor>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QPainter>
#include <QPainterPath>
#include <QQuickWindow>

// #define THUMBNAIL_DEBUG
#ifdef THUMBNAIL_DEBUG
#define THUMBNAIL_LOG qWarning()
#else
#define THUMBNAIL_LOG \
    if (false)        \
    qWarning()
#endif

#include <algorithm>
#include <cmath>

namespace {
constexpr int kSeekPeriodMs = 30;
constexpr int kPollIntervalMs = 16;
constexpr int kMaxPollAttempts = 80;
constexpr int kExactSeekDelayTicks = 5;
constexpr int kBytesPerPixel = 4;

constexpr uint64_t kTimePosReplyId = 1001;
constexpr uint64_t kReplyScreenshot = 1002;
constexpr uint64_t kReplySeek = 1003;

void setOption(mpv_handle* mpv, const char* name, const char* value)
{
    mpv_set_option_string(mpv, name, value);
}
} // namespace

/**
 * @brief Constructs the controller, sets up debounce and poll timers.
 * @param parent Optional QQuickItem parent.
 */
ThumbnailController::ThumbnailController(QQuickItem* parent)
    : QQuickPaintedItem(parent)
{
    m_seekPeriodTimer.setSingleShot(false);
    m_seekPeriodTimer.setInterval(kSeekPeriodMs);
    connect(&m_seekPeriodTimer, &QTimer::timeout, this, &ThumbnailController::onSeekPeriodTimeout);

    m_pollTimer.setInterval(kPollIntervalMs);
    connect(&m_pollTimer, &QTimer::timeout, this, &ThumbnailController::pollOutput);
}

/**
 * @brief Destroys the controller, clears state, and shuts down the mpv worker.
 */
ThumbnailController::~ThumbnailController()
{
    clear();
    destroyWorker();
}

/**
 * @brief Returns the attached player instance.
 * @return Pointer to CineMpvItem, or nullptr if unset.
 */
CineMpvItem* ThumbnailController::player() const
{
    return m_player;
}

/**
 * @brief Assigns the player and emits playerChanged().
 * @param player Pointer to the CineMpvItem to assign.
 */
void ThumbnailController::setPlayer(CineMpvItem* player)
{
    if (m_player == player)
    {
        return;
    }

    m_player = player;
    Q_EMIT playerChanged();
}

/**
 * @brief Requests a thumbnail at the given timeline position.
 * @param path  Media file path.
 * @param time  Timestamp in seconds for the capture.
 * @param posX  Screen x-coordinate for the overlay.
 * @param posY  Screen y-coordinate for the overlay.
 * @param width Desired display width in pixels.
 * @param height Desired display height in pixels.
 */
void ThumbnailController::request(const QString& path, double time, int posX, int posY, int width, int height)
{
    if (!m_player || path.trimmed().isEmpty() || width <= 0 || height <= 0)
    {
        clear();
        return;
    }

    const int second = static_cast<int>(std::floor(std::max(0.0, time)));

    // Calculate physical pixels for High-DPI support
    const double dpr = (m_player && m_player->window()) ? m_player->window()->devicePixelRatio() : 1.0;
    const int physicalX = static_cast<int>(std::round(posX * dpr));
    const int physicalY = static_cast<int>(std::round(posY * dpr));
    const QSize physicalSize(static_cast<int>(std::round(width * dpr)), static_cast<int>(std::round(height * dpr)));
    const QSize sourceSize = physicalSize;

    const bool workerShapeChanged = m_path != path || m_workerSize != sourceSize;

    m_path = path;
    m_time = std::max(0.0, time);
    m_second = second;
    m_posX = physicalX;
    m_posY = physicalY;
    m_size = physicalSize;
    m_sourceSize = sourceSize;

    THUMBNAIL_LOG << "ThumbnailController::request path:" << path << "time:" << time << "second:" << second;

    if (workerShapeChanged)
    {
        destroyWorker();
        m_loadedPath.clear();
        m_workerSize = sourceSize;
        m_haveFrame = false;
        m_lastRequestedSecond = -1;
        m_captureSecond = -1;
        m_frameCache.clear();

        // Cache vid/rotate once when file changes instead of querying sync on every seek
        m_cachedVid = -1;
        m_cachedRotate = 0;
        if (m_player)
        {
            QVariant vidVar = m_player->mpvOption(QStringLiteral("vid"));
            if (vidVar.isValid())
                m_cachedVid = vidVar.toInt();
            QVariant rotateVar = m_player->mpvOption(QStringLiteral("video-params/rotate"));
            if (rotateVar.isValid())
                m_cachedRotate = rotateVar.toInt();
            else
            {
                rotateVar = m_player->mpvOption(QStringLiteral("video-rotate"));
                if (rotateVar.isValid())
                    m_cachedRotate = rotateVar.toInt();
            }
        }
    }
    else if (m_haveFrame && m_second == m_lastRequestedSecond)
    {
        update();
        return;
    }
    else if (!workerShapeChanged)
    {
        // Check frame cache for this second before scheduling a seek
        auto it = m_frameCache.constFind(m_second);
        if (it != m_frameCache.constEnd() && !it->isNull())
        {
            m_currentImage = it.value();
            m_haveFrame = true;
            m_captureSecond = m_second;
            m_captureTime = m_time;
            m_lastRequestedSecond = m_second;
            Q_EMIT thumbnailReady(m_captureTime);
            update();
            return;
        }
    }

    m_allowFastSeek = true;
    if (m_player)
    {
        double duration = m_player->duration();
        if (duration > 0.0 && duration < 30.0)
        {
            m_allowFastSeek = false;
        }
    }

    scheduleSeek();
}

/**
 * @brief Hides the thumbnail overlay and cancels pending seek/poll work.
 */
void ThumbnailController::clear()
{
    THUMBNAIL_LOG << "ThumbnailController::clear";
    m_seekPeriodTimer.stop();
    m_pollTimer.stop();
    m_haveFrame = false;
    m_captureSecond = -1;
    m_lastRequestedSecond = -1;
    m_currentImage = QImage();
    m_frameCache.clear();
    update();
}

/**
 * @brief Paints the cached thumbnail frame onto the overlay area.
 * @param painter QPainter from the Qt Quick render engine.
 */
void ThumbnailController::paint(QPainter* painter)
{
    THUMBNAIL_LOG << "ThumbnailController::paint m_currentImage null?:" << m_currentImage.isNull();
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(boundingRect(), 8, 8);
    painter->setClipPath(path);
    painter->fillPath(path, QColor(QStringLiteral("#101014")));

    if (!m_currentImage.isNull())
    {
        painter->drawImage(boundingRect(), m_currentImage);
    }

    painter->restore();
}

/**
 * @brief Starts or resets the seek debounce timer.
 */
void ThumbnailController::scheduleSeek()
{
    if (m_seekPeriodTimer.isActive())
    {
        m_seekPeriodCounter = 0;
    }
    else
    {
        m_seekPeriodTimer.start();
        performSeek(m_allowFastSeek);
        m_seekPeriodCounter = 1;
    }
}

/**
 * @brief Creates and initialises the background mpv worker.
 * @return true on success, false on failure.
 */
bool ThumbnailController::ensureWorker()
{
    if (m_mpv)
    {
        return true;
    }

    m_mpv = mpv_create();
    if (!m_mpv)
    {
        return false;
    }

    setOption(m_mpv, "config", "no");
    setOption(m_mpv, "msg-level", "all=no");
    setOption(m_mpv, "idle", "yes");
    setOption(m_mpv, "pause", "yes");
    setOption(m_mpv, "keep-open", "always");
    setOption(m_mpv, "terminal", "no");
    setOption(m_mpv, "load-scripts", "no");
    setOption(m_mpv, "osc", "no");
    setOption(m_mpv, "ytdl", "no");
    setOption(m_mpv, "audio", "no");
    setOption(m_mpv, "sub", "no");
    setOption(m_mpv, "hwdec", "no");
    setOption(m_mpv, "hr-seek", m_allowFastSeek ? "no" : "yes");
    setOption(m_mpv, "demuxer-readahead-secs", "0");
    setOption(m_mpv, "demuxer-max-bytes", "128KiB");
    setOption(m_mpv, "vd-lavc-skiploopfilter", "all");
    setOption(m_mpv, "vd-lavc-software-fallback", "1");
    setOption(m_mpv, "vd-lavc-threads", "2");
    setOption(m_mpv, "zimg-sw", "no");
    setOption(m_mpv, "sws-scaler", "fast-bilinear");

    // Performance and silence settings from thumbfast
    setOption(m_mpv, "really-quiet", "yes");
    setOption(m_mpv, "load-stats-overlay", "no");
    setOption(m_mpv, "load-osd-console", "no");
    setOption(m_mpv, "load-auto-profiles", "no");

    // Encoding formats to force video filter/decoding chain under vo=null
    setOption(m_mpv, "ovc", "rawvideo");
    setOption(m_mpv, "of", "image2");
    setOption(m_mpv, "ofopts", "update=1");

    // Dummy video/audio outputs
    setOption(m_mpv, "vo", "null");
    setOption(m_mpv, "ao", "null");

    // Sync active video track from player
    if (m_player)
    {
        QVariant vidVar = m_player->mpvOption(QStringLiteral("vid"));
        if (vidVar.isValid())
        {
            setOption(m_mpv, "vid", vidVar.toString().toUtf8().constData());
        }

        // Sync rotation
        QVariant rotateVar = m_player->mpvOption(QStringLiteral("video-params/rotate"));
        int rotate = 0;
        if (rotateVar.isValid())
        {
            rotate = rotateVar.toInt();
        }
        else
        {
            rotateVar = m_player->mpvOption(QStringLiteral("video-rotate"));
            if (rotateVar.isValid())
            {
                rotate = rotateVar.toInt();
            }
        }
        setOption(m_mpv, "video-rotate", QByteArray::number(rotate).constData());
    }

    const QByteArray vf = vfString().toUtf8();
    setOption(m_mpv, "vf", vf.constData());

    if (mpv_initialize(m_mpv) < 0)
    {
        destroyWorker();
        return false;
    }

    mpv_observe_property(m_mpv, kTimePosReplyId, "time-pos", MPV_FORMAT_DOUBLE);

    return true;
}

/**
 * @brief Issues a seek (or loadfile) command to the mpv worker.
 * @param fast If true, uses keyframe seek; otherwise exact seek.
 */
void ThumbnailController::performSeek(bool fast)
{
    if (m_path.isEmpty() || !ensureWorker())
    {
        return;
    }

    m_haveFrame = false;
    m_captureSecond = m_second;
    m_captureTime = m_time;
    m_pollAttempts = 0;

    THUMBNAIL_LOG << "ThumbnailController::performSeek time:" << m_time;

    // Use cached vid/rotate - these are populated once in request() when the file changes.
    // Avoids blocking synchronous mpvOption calls on every seek during hover.
    if (m_cachedVid >= 0)
    {
        setOption(m_mpv, "vid", QByteArray::number(m_cachedVid).constData());
    }
    setOption(m_mpv, "video-rotate", QByteArray::number(m_cachedRotate).constData());

    setOption(m_mpv, "hr-seek", fast ? "no" : "yes");

    const QByteArray path = m_path.toUtf8();
    const QByteArray start = QStringLiteral("start=%1").arg(m_time, 0, 'f', 3).toUtf8();

    if (m_loadedPath != m_path)
    {
        const char* cmd[] = {"loadfile", path.constData(), "replace", "-1", start.constData(), nullptr};
        if (mpv_command_async(m_mpv, kReplySeek, cmd) < 0)
        {
            m_pollTimer.stop();
            return;
        }
        m_loadedPath = m_path;
    }
    else
    {
        const QByteArray time = QString::number(m_time, 'f', 3).toUtf8();
        const QByteArray seekMode = fast ? "absolute+keyframes" : "absolute+exact";
        const char* cmd[] = {"seek", time.constData(), seekMode.constData(), nullptr};
        if (mpv_command_async(m_mpv, kReplySeek, cmd) < 0)
        {
            m_pollTimer.stop();
            return;
        }
    }

    m_pollTimer.start();
}

/**
 * @brief Callback for seek debounce timer ticks; throttles seek requests.
 */
void ThumbnailController::onSeekPeriodTimeout()
{
    if (m_seekPeriodCounter == 0)
    {
        performSeek(m_allowFastSeek);
        m_seekPeriodCounter = 1;
    }
    else if (m_seekPeriodCounter >= kExactSeekDelayTicks)
    {
        if (m_allowFastSeek)
        {
            m_seekPeriodTimer.stop();
            performSeek(false); // Exact seek when user stops hovering
        }
        else
        {
            m_seekPeriodTimer.stop();
        }
    }
    else
    {
        m_seekPeriodCounter++;
    }
}

/**
 * @brief Polls the mpv worker event queue for a rendered frame.
 */
void ThumbnailController::pollOutput()
{
    drainEvents();

    if (++m_pollAttempts > kMaxPollAttempts)
    {
        THUMBNAIL_LOG << "ThumbnailController::pollOutput max poll attempts reached!";
        m_pollTimer.stop();
    }
}

/**
 * @brief Drains and processes all pending mpv events (screenshot, property changes, etc.).
 */
void ThumbnailController::drainEvents()
{
    if (!m_mpv)
    {
        return;
    }

    while (true)
    {
        mpv_event* event = mpv_wait_event(m_mpv, 0);
        if (!event || event->event_id == MPV_EVENT_NONE)
        {
            break;
        }

        THUMBNAIL_LOG << "ThumbnailController::drainEvents event_id:" << event->event_id;

        switch (event->event_id)
        {
            case MPV_EVENT_PROPERTY_CHANGE:
            {
                mpv_event_property* prop = static_cast<mpv_event_property*>(event->data);
                if (prop && strcmp(prop->name, "time-pos") == 0)
                {
                    THUMBNAIL_LOG << "Property time-pos change, format:" << prop->format;
                    if (prop->format == MPV_FORMAT_DOUBLE && prop->data)
                    {
                        double position = *static_cast<double*>(prop->data);
                        THUMBNAIL_LOG << "time-pos double value:" << position;
                        if (m_mpv)
                        {
                            const char* cmd[] = {"screenshot-raw", nullptr};
                            mpv_command_async(m_mpv, kReplyScreenshot, cmd);
                        }
                    }
                }
                break;
            }
            case MPV_EVENT_COMMAND_REPLY:
            {
                THUMBNAIL_LOG << "Command reply userdata:" << event->reply_userdata << "error:" << event->error;
                if (event->reply_userdata == kReplyScreenshot)
                {
                    mpv_event_command* cmd_event = static_cast<mpv_event_command*>(event->data);
                    if (event->error >= 0 && cmd_event && cmd_event->result.format == MPV_FORMAT_NODE_MAP)
                    {
                        mpv_node_list* map = cmd_event->result.u.list;
                        int w = 0, h = 0, stride = 0;
                        char* format = nullptr;
                        void* data = nullptr;

                        for (int i = 0; i < map->num; ++i)
                        {
                            char* key = map->keys[i];
                            mpv_node value = map->values[i];
                            if (strcmp(key, "w") == 0 && value.format == MPV_FORMAT_INT64)
                            {
                                w = static_cast<int>(value.u.int64);
                            }
                            else if (strcmp(key, "h") == 0 && value.format == MPV_FORMAT_INT64)
                            {
                                h = static_cast<int>(value.u.int64);
                            }
                            else if (strcmp(key, "stride") == 0 && value.format == MPV_FORMAT_INT64)
                            {
                                stride = static_cast<int>(value.u.int64);
                            }
                            else if (strcmp(key, "format") == 0 && value.format == MPV_FORMAT_STRING)
                            {
                                format = value.u.string;
                            }
                            else if (strcmp(key, "data") == 0 && value.format == MPV_FORMAT_BYTE_ARRAY)
                            {
                                data = value.u.ba->data;
                            }
                        }

                        QImage::Format qtFormat = QImage::Format_Invalid;
                        if (strcmp(format, "bgra") == 0 || strcmp(format, "bgr0") == 0)
                        {
                            qtFormat = QImage::Format_RGB32;
                        }
                        else if (strcmp(format, "rgba") == 0 || strcmp(format, "rgb0") == 0)
                        {
                            qtFormat = QImage::Format_RGBA8888;
                        }

                        THUMBNAIL_LOG << "Screenshot result: w:" << w << "h:" << h << "format:" << format
                                      << "stride:" << stride << "data:" << (data != nullptr)
                                      << "mapped format:" << qtFormat;

                        if (w > 0 && h > 0 && stride > 0 && data && qtFormat != QImage::Format_Invalid)
                        {
                            m_currentImage = QImage(reinterpret_cast<uchar*>(data), w, h, stride, qtFormat).copy();
                            m_haveFrame = true;
                            m_frameSize = QSize(w, h);
                            m_lastRequestedSecond = m_captureSecond;
                            m_pollTimer.stop();

                            // Cache frame by second for fast re-hover
                            if (m_frameCache.size() >= kMaxFrameCacheSize)
                            {
                                m_frameCache.erase(m_frameCache.begin());
                            }
                            m_frameCache.insert(m_captureSecond, m_currentImage);

                            Q_EMIT thumbnailReady(m_captureTime);
                            update();
                        }
                    }
                    if (cmd_event)
                    {
                        mpv_free_node_contents(&cmd_event->result);
                    }
                }
                break;
            }
            case MPV_EVENT_END_FILE:
            case MPV_EVENT_SHUTDOWN:
                m_pollTimer.stop();
                break;
            default:
                break;
        }
    }
}

/**
 * @brief Shuts down the mpv worker and releases its handle.
 */
void ThumbnailController::destroyWorker()
{
    if (!m_mpv)
    {
        return;
    }

    mpv_terminate_destroy(m_mpv);
    m_mpv = nullptr;
}

/**
 * @brief Builds the mpv video-filter string for downscaling.
 * @return Filter string (e.g. "scale=w=87:h=48:...").
 */
QString ThumbnailController::vfString() const
{
    return QStringLiteral(
               "scale=w=%1:h=%2:force_original_aspect_ratio=decrease,"
               "pad=w=%1:h=%2:x=-1:y=-1,format=bgra")
        .arg(m_sourceSize.width())
        .arg(m_sourceSize.height());
}
