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

#include <QHash>
#include <QImage>
#include <QQuickPaintedItem>
#include <QSize>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include <mpv/client.h>

class CineMpvItem;

/**
 * @class ThumbnailController
 * @brief Generates hover thumbnails for the playback timeline using a lightweight mpv worker in-memory.
 * @details Spawns a headless mpv instance that seeks to and captures frames at the
 *          user's hover position. The captured frame is painted as an overlay on the
 *          timeline seekbar. Seek requests are debounced via an internal timer to
 *          avoid overwhelming the worker during fast scrubbing.
 */
class ThumbnailController : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(CineMpvItem* player READ player WRITE setPlayer NOTIFY playerChanged)

public:
    /**
     * @brief Constructs a thumbnail controller attached to a QQuickItem parent.
     * @param parent Optional QQuickItem parent for Qt parent-child ownership.
     * @usecase Called by QML engine when ThumbnailController is declared in a .qml file.
     * @sideeffects Creates internal timers but does not start them; no mpv worker is spawned yet.
     * @thread Must be called on the Qt main / GUI thread (QQuickItem requirement).
     */
    explicit ThumbnailController(QQuickItem* parent = nullptr);

    /**
     * @brief Destroys the controller, cleaning up the mpv worker process and all resources.
     * @usecase Called automatically when the QML object is destroyed or the controller goes out of scope.
     * @sideeffects Kills the internal mpv worker via destroyWorker() and stops all timers.
     * @thread Must be called on the Qt main thread.
     */
    ~ThumbnailController() override;

    /**
     * @brief Returns the currently attached player instance.
     * @return Pointer to the CineMpvItem, or nullptr if none is set.
     * @usecase Used internally by paint() to determine overlay positioning, and externally by QML bindings.
     * @thread Safe to call from any thread since the pointer is set once and only changed via the setter (main thread).
     */
    CineMpvItem* player() const;

    /**
     * @brief Sets the player that receives thumbnail overlay commands.
     * @param player Pointer to the CineMpvItem to assign.
     * @usecase Called from QML when the player property binding changes.
     * @sideeffects Updates the internal player pointer and emits playerChanged().
     * @thread Must be called on the Qt main thread.
     */
    void setPlayer(CineMpvItem* player);

    /**
     * @brief Requests a thumbnail at a specific timeline position for the given media file.
     * @param path  Absolute or relative path to the media file.
     * @param time  Timestamp in seconds to capture the thumbnail at.
     * @param posX  Screen x-coordinate where the thumbnail overlay should be drawn.
     * @param posY  Screen y-coordinate where the thumbnail overlay should be drawn.
     * @param width Desired display width of the thumbnail in pixels.
     * @param height Desired display height of the thumbnail in pixels.
     * @usecase Invoked from QML when the user hovers over the timeline seekbar at a given position.
     * @sideeffects Cancels any in-flight thumbnail request, updates internal position/size state,
     *              and schedules a seek in the mpv worker. If the path differs from the currently
     *              loaded media, the worker is re-initialised with the new file.
     * @thread Must be called on the Qt main thread (Q_INVOKABLE from QML).
     */
    Q_INVOKABLE void request(const QString& path, double time, int posX, int posY, int width, int height);

    /**
     * @brief Hides the current thumbnail preview and cancels any pending seek / poll work.
     * @usecase Called from QML when the user moves the cursor away from the timeline.
     * @sideeffects Stops seek and poll timers, resets capture state, clears the cached image,
     *              and triggers a repaint (the overlay disappears).
     * @thread Must be called on the Qt main thread.
     */
    Q_INVOKABLE void clear();

    /**
     * @brief Paints the cached thumbnail image onto the overlay area.
     * @param painter The QPainter provided by the Qt Quick rendering engine.
     * @usecase Called automatically by the Qt Quick render loop whenever the item needs repainting
     *          (e.g. after clear() or when m_currentImage is updated).
     * @sideeffects Draws the currently cached QImage at the stored (m_posX, m_posY) position. If no frame
     *              is available (m_haveFrame == false) this is a no-op.
     * @thread Called on the Qt render / GUI thread depending on the rendering backend (QSG).
     */
    void paint(QPainter* painter) override;

Q_SIGNALS:
    /**
     * @brief Emitted when the player property changes.
     * @usecase QML bindings connect to this signal to react when a new CineMpvItem is assigned.
     * @thread Emitted on the Qt main thread.
     */
    void playerChanged();

    /**
     * @brief Emitted when a thumbnail frame has been successfully captured.
     * @param time The exact timestamp (in seconds) of the captured frame, as reported by mpv.
     * @usecase QML can use this to synchronise overlay positioning or display timing.
     * @thread Emitted on the Qt main thread from the poll timer callback.
     */
    void thumbnailReady(double time);

private:
    /** @brief Starts the seek period timer which paces repeated seek requests. */
    void scheduleSeek();

    /**
     * @brief Issues a seek command to the mpv worker at the current target time.
     * @param fast If true, uses "absolute+exact" seek mode; otherwise uses a precise
     *             seek. Fast seeks are used when the user is scrubbing rapidly.
     * @usecase Called by onSeekPeriodTimeout() after the debounce interval elapses.
     */
    void performSeek(bool fast);

    /**
     * @brief Callback for the seek period timer - triggers performSeek when the
     *        debounce window expires.
     * @usecase Connected to QTimer::timeout for m_seekPeriodTimer.
     */
    void onSeekPeriodTimeout();

    /**
     * @brief Polls the mpv worker for a newly rendered video frame.
     * @details Checks mpv's event queue for a frame-ready event. If a frame is available,
     *          it copies the pixel data into m_currentImage, stops the poll timer, and
     *          emits thumbnailReady().
     * @usecase Called periodically by m_pollTimer after a seek is issued.
     * @sideeffects Updates m_currentImage, m_haveFrame, and may stop the poll timer.
     */
    void pollOutput();

    /**
     * @brief Drains all pending events from the mpv worker's event queue.
     * @usecase Called during clear() and before re-initialisation to keep the queue from growing unbounded.
     */
    void drainEvents();

    /**
     * @brief Creates and starts the background mpv worker process if it does not already exist.
     * @return true if the worker was successfully created or already running, false on failure.
     * @usecase Called on first request() and whenever the media path changes.
     * @sideeffects Spawns a new mpv instance, sets up the video filter chain, and starts the seek timer.
     */
    bool ensureWorker();

    /**
     * @brief Destroys the mpv worker process and releases associated resources.
     * @usecase Called from the destructor and during clear() when a full reset is needed.
     * @sideeffects Shuts down the mpv handle and stops both timers.
     */
    void destroyWorker();

    /**
     * @brief Builds the mpv --vf (video filter) string for downscaling the capture.
     * @return A filter string such as "scale=w=87:h=48" or empty if no scaling is needed.
     * @usecase Called during worker initialisation to set the output frame size.
     */
    QString vfString() const;

    CineMpvItem* m_player{nullptr}; /**< Attached player instance receiving overlay commands. */
    mpv_handle* m_mpv{nullptr};     /**< Opaque handle to the background mpv worker instance. */
    QTimer m_seekPeriodTimer;       /**< Debounce timer that paces seek requests during scrubbing. */
    QTimer m_pollTimer;             /**< Timer that periodically polls the worker for a rendered frame. */
    QString m_path;                 /**< Media path requested by the most recent call to request(). */
    QString m_loadedPath;           /**< Media path that is currently loaded in the mpv worker. */
    double m_time{0.0};             /**< Target timestamp (seconds) for the current thumbnail request. */
    double m_captureTime{0.0};      /**< Actual timestamp (seconds) of the captured frame from mpv. */
    int m_second{-1};               /**< Integer seconds floor of m_time, used for change detection. */
    int m_captureSecond{-1};        /**< Integer seconds floor of m_captureTime. */
    int m_lastRequestedSecond{-1};  /**< Seconds value of the last completed request, used to avoid duplicate work. */
    int m_posX{0};                  /**< Screen x-coordinate where the thumbnail overlay is drawn. */
    int m_posY{0};                  /**< Screen y-coordinate where the thumbnail overlay is drawn. */
    QSize m_size{174, 96};          /**< Display size of the thumbnail overlay in pixels. */
    QSize m_sourceSize{87, 48};     /**< Source capture resolution fed to the mpv video filter. */
    QSize m_workerSize;             /**< Actual output resolution reported by the worker. */
    QSize m_frameSize;              /**< Dimensions of the most recently captured frame. */
    bool m_haveFrame{false};        /**< True when a valid frame has been captured and is ready for display. */
    int m_pollAttempts{0};          /**< Number of poll iterations since the last seek, used for timeout detection. */
    int m_seekPeriodCounter{0};     /**< Counter incremented each seek period tick, used to throttle fast seeks. */
    bool m_allowFastSeek{true};     /**< When true, the next seek may use fast (non-exact) mode. */
    QImage m_currentImage;          /**< Cached thumbnail frame data, drawn by paint(). */
    int m_cachedVid{-1};            /**< Cached video track ID to avoid sync mpvOption queries. */
    int m_cachedRotate{0};          /**< Cached rotation angle to avoid sync mpvOption queries. */
    QHash<int, QImage> m_frameCache; /**< Frame cache keyed by second for fast re-hover. */

    static constexpr int kMaxFrameCacheSize = 24;
};
