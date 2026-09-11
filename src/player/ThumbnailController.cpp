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
#include "app/LoggingCategories.h"
#include "utils/MediaUtils.h"

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
#include <limits>

namespace {
constexpr int kSeekPeriodMs = 30;
constexpr int kPollIntervalMs = 16;
constexpr int kMaxPollAttempts = 320;
constexpr int kExactSeekDelayTicks = 5;
constexpr int kBytesPerPixel = 4;

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
    m_frameCache.setMaxCost(8 * 1024 * 1024);
    m_idleTimer.setSingleShot(true);
    m_idleTimer.setInterval(60000);
    connect(&m_idleTimer, &QTimer::timeout, this, &ThumbnailController::destroyWorker);
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

    if (m_player)
        disconnect(m_player, nullptr, this, nullptr);
    clear();
    destroyWorker();
    m_player = player;
    if (m_player)
    {
        connect(m_player, &CineMpvItem::fileStarted, this, [this] {
            clear();
            destroyWorker();
        });
        connect(m_player, &CineMpvItem::videoGeometryChanged, this, [this] {
            clear();
            destroyWorker();
        });
    }
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
    if (!m_player || path.trimmed().isEmpty() || width <= 0 || height <= 0 || !std::isfinite(time)
        || m_player->videoWidth() <= 0 || time > static_cast<double>(std::numeric_limits<int>::max() - 1))
    {
        clear();
        return;
    }

    time = std::max(0.0, time);
    if (m_player->duration() > 0)
        time = std::min(time, std::max(0.0, m_player->duration() - 0.05));
    const int second = static_cast<int>(std::floor(time));

    // Calculate physical pixels for High-DPI support
    const double dpr = (m_player && m_player->window()) ? m_player->window()->devicePixelRatio() : 1.0;
    const int physicalX = static_cast<int>(std::round(posX * dpr));
    const int physicalY = static_cast<int>(std::round(posY * dpr));
    const QSize physicalSize(static_cast<int>(std::round(width * dpr)), static_cast<int>(std::round(height * dpr)));
    const QSize sourceSize = physicalSize.width() > 512 || physicalSize.height() > 512
        ? physicalSize.scaled(QSize(512, 512), Qt::KeepAspectRatio) : physicalSize;

    const bool workerShapeChanged = m_path != path || m_workerSize != sourceSize;

    m_path = path;
    m_time = std::max(0.0, time);
    m_second = second;
    m_posX = physicalX;
    m_posY = physicalY;
    m_size = physicalSize;
    m_sourceSize = sourceSize;
    m_active = true;
    m_idleTimer.stop();

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
        m_sourcePath = path;
        if (m_player)
        {
            QVariant vidVar = m_player->mpvOption(QStringLiteral("vid"));
            if (vidVar.isValid())
                m_cachedVid = vidVar.toInt();
            QVariant rotateVar = m_player->mpvOption(QStringLiteral("video-rotate"));
            if (rotateVar.isValid())
                m_cachedRotate = rotateVar.toInt();
            if (!MediaUtils::isLocalPath(path))
            {
                const QString resolved = m_player->mpvOption(QStringLiteral("stream-open-filename")).toString();
                if (!resolved.isEmpty())
                    m_sourcePath = resolved;
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
        const QImage* cached = m_frameCache.object(m_second);
        if (cached && !cached->isNull())
        {
            m_currentImage = *cached;
            m_haveFrame = true;
            m_lastRequestedSecond = m_second;
            m_pendingSeek = false;
            m_seekPeriodTimer.stop();
            Q_EMIT thumbnailReady(m_time);
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
    m_active = false;
    m_pendingSeek = false;
    m_seekPeriodTimer.stop();
    if (!m_inFlight)
        m_pollTimer.stop();
    m_haveFrame = false;
    m_lastRequestedSecond = -1;
    m_currentImage = QImage();
    Q_EMIT thumbnailCleared();
    if (m_mpv)
        m_idleTimer.start();
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
    if (!m_currentImage.isNull())
    {
        painter->setRenderHint(QPainter::SmoothPixmapTransform);
        const QSizeF imageSize = m_currentImage.size().scaled(boundingRect().size().toSize(), Qt::KeepAspectRatio);
        const QRectF target(QPointF((width() - imageSize.width()) / 2, (height() - imageSize.height()) / 2), imageSize);
        painter->drawImage(target, m_currentImage);
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

    // Dummy video/audio outputs
    setOption(m_mpv, "vo", "null");
    setOption(m_mpv, "ao", "null");

    // Sync active video track from player
    if (m_cachedVid > 0)
        setOption(m_mpv, "vid", QByteArray::number(m_cachedVid).constData());
    setOption(m_mpv, "video-rotate", QByteArray::number(m_cachedRotate).constData());
    if (m_player && !MediaUtils::isLocalPath(m_path))
    {
        for (const char* option : {"user-agent", "referrer"})
        {
            const QByteArray value = m_player->mpvOption(QString::fromLatin1(option)).toString().toUtf8();
            if (!value.isEmpty())
                setOption(m_mpv, option, value.constData());
        }
        const QStringList headers = m_player->mpvOption(QStringLiteral("http-header-fields")).toStringList();
        QList<QByteArray> encodedHeaders;
        QList<mpv_node> values;
        for (const QString& header : headers)
            encodedHeaders.append(header.toUtf8());
        for (QByteArray& header : encodedHeaders)
        {
            mpv_node value{};
            value.format = MPV_FORMAT_STRING;
            value.u.string = header.data();
            values.append(value);
        }
        if (!values.isEmpty())
        {
            mpv_node_list list{};
            list.num = static_cast<int>(values.size());
            list.values = values.data();
            mpv_node node{};
            node.format = MPV_FORMAT_NODE_ARRAY;
            node.u.list = &list;
            mpv_set_option(m_mpv, "http-header-fields", MPV_FORMAT_NODE, &node);
        }
    }

    const QByteArray vf = vfString().toUtf8();
    setOption(m_mpv, "vf", vf.constData());

    const int initialized = mpv_initialize(m_mpv);
    if (initialized < 0)
    {
        qCWarning(cinePlayerLog) << "Thumbnail worker initialization failed:" << mpv_error_string(initialized);
        destroyWorker();
        return false;
    }

    return true;
}

/**
 * @brief Issues a seek (or loadfile) command to the mpv worker.
 * @param fast If true, uses keyframe seek; otherwise exact seek.
 */
void ThumbnailController::performSeek(bool fast)
{
    if (!m_active || m_path.isEmpty())
        return;
    if (m_inFlight)
    {
        m_pendingSeek = true;
        m_pendingFastSeek = fast;
        return;
    }
    if (!ensureWorker())
    {
        m_seekPeriodTimer.stop();
        return;
    }

    m_captureSecond = m_second;
    m_captureTime = m_time;
    m_captureExact = !fast;
    m_inFlight = true;
    m_frameAvailable = false;
    m_screenshotPending = false;
    m_pollAttempts = 0;

    THUMBNAIL_LOG << "ThumbnailController::performSeek time:" << m_time;

    // Use cached vid/rotate - these are populated once in request() when the file changes.
    // Avoids blocking synchronous mpvOption calls on every seek during hover.
    mpv_set_property_string(m_mpv, "hr-seek", fast ? "no" : "yes");

    const QByteArray path = m_sourcePath.toUtf8();
    const QByteArray start = QString::number(m_time, 'f', 3).toUtf8();

    if (m_loadedPath != m_path)
    {
        mpv_set_property_string(m_mpv, "start", start.constData());
        const char* cmd[] = {"loadfile", path.constData(), "replace", nullptr};
        if (mpv_command_async(m_mpv, kReplySeek, cmd) < 0)
        {
            m_pollTimer.stop();
            m_inFlight = false;
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
            m_inFlight = false;
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

    if (m_inFlight && m_frameAvailable && !m_screenshotPending)
        requestScreenshot();

    if (m_inFlight && ++m_pollAttempts > kMaxPollAttempts)
    {
        qCWarning(cinePlayerLog) << "Thumbnail capture timed out";
        clear();
        destroyWorker();
    }
}

void ThumbnailController::requestScreenshot()
{
    if (!m_mpv || !m_inFlight || m_screenshotPending)
        return;
    const char* command[] = {"screenshot-raw", "video", nullptr};
    m_screenshotPending = mpv_command_async(m_mpv, kReplyScreenshot, command) >= 0;
}

void ThumbnailController::finishCapture()
{
    m_inFlight = false;
    m_frameAvailable = false;
    m_screenshotPending = false;
    m_pollTimer.stop();
    if (m_pendingSeek && m_active)
    {
        const bool fast = m_pendingFastSeek;
        m_pendingSeek = false;
        QTimer::singleShot(0, this, [this, fast] { performSeek(fast); });
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
            case MPV_EVENT_PLAYBACK_RESTART:
            {
                m_frameAvailable = m_inFlight;
                requestScreenshot();
                break;
            }
            case MPV_EVENT_COMMAND_REPLY:
            {
                THUMBNAIL_LOG << "Command reply userdata:" << event->reply_userdata << "error:" << event->error;
                if (event->reply_userdata == kReplySeek && event->error < 0)
                {
                    qCWarning(cinePlayerLog) << "Thumbnail seek failed:" << mpv_error_string(event->error);
                    m_loadedPath.clear();
                    finishCapture();
                }
                else if (event->reply_userdata == kReplyScreenshot && m_inFlight)
                {
                    m_screenshotPending = false;
                    mpv_event_command* cmd_event = static_cast<mpv_event_command*>(event->data);
                    if (event->error >= 0 && cmd_event && cmd_event->result.format == MPV_FORMAT_NODE_MAP)
                    {
                        mpv_node_list* map = cmd_event->result.u.list;
                        int w = 0, h = 0, stride = 0;
                        char* format = nullptr;
                        void* data = nullptr;
                        size_t dataSize = 0;

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
                            else if (strcmp(key, "data") == 0 && value.format == MPV_FORMAT_BYTE_ARRAY && value.u.ba)
                            {
                                data = value.u.ba->data;
                                dataSize = value.u.ba->size;
                            }
                        }

                        QImage::Format qtFormat = QImage::Format_Invalid;
                        if (format && (strcmp(format, "bgra") == 0 || strcmp(format, "bgr0") == 0))
                        {
                            qtFormat = QImage::Format_RGB32;
                        }
                        else if (format && strcmp(format, "rgba") == 0)
                        {
                            qtFormat = QImage::Format_RGBA8888;
                        }
                        else if (format && strcmp(format, "rgb0") == 0)
                        {
                            qtFormat = QImage::Format_RGBX8888;
                        }

                        THUMBNAIL_LOG << "Screenshot result: w:" << w << "h:" << h << "format:" << format
                                      << "stride:" << stride << "data:" << (data != nullptr)
                                      << "mapped format:" << qtFormat;

                        if (w > 0 && w <= 1024 && h > 0 && h <= 1024 && stride >= w * kBytesPerPixel && stride <= 16384
                            && data && qtFormat != QImage::Format_Invalid
                            && static_cast<size_t>(stride) * static_cast<size_t>(h) <= dataSize)
                        {
                            QImage image = QImage(reinterpret_cast<uchar*>(data), w, h, stride, qtFormat).copy();
                            if (image.isNull())
                                break;
                            if (qtFormat == QImage::Format_RGB32)
                            {
                                for (int row = 0; row < image.height(); ++row)
                                {
                                    auto* pixels = reinterpret_cast<QRgb*>(image.scanLine(row));
                                    for (int column = 0; column < image.width(); ++column)
                                        pixels[column] |= 0xff000000u;
                                }
                            }
                            if (m_captureExact)
                                m_frameCache.insert(m_captureSecond, new QImage(image), static_cast<int>(image.sizeInBytes()));
                            if (m_active && m_captureSecond == m_second)
                            {
                                m_currentImage = image;
                                m_haveFrame = true;
                                m_frameSize = QSize(w, h);
                                m_lastRequestedSecond = m_captureSecond;
                                Q_EMIT thumbnailReady(m_captureTime);
                                update();
                            }
                            finishCapture();
                        }
                    }
                }
                break;
            }
            case MPV_EVENT_END_FILE:
            case MPV_EVENT_SHUTDOWN:
                m_loadedPath.clear();
                finishCapture();
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
    m_seekPeriodTimer.stop();
    m_pollTimer.stop();
    m_idleTimer.stop();
    m_inFlight = false;
    m_screenshotPending = false;
    m_frameAvailable = false;
    m_pendingSeek = false;
    m_loadedPath.clear();
    m_workerSize = QSize();
    m_frameCache.clear();
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
               "pad=w=%1:h=%2:x=-1:y=-1,setsar=1,format=bgra")
        .arg(m_sourceSize.width())
        .arg(m_sourceSize.height());
}
