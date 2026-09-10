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

#include <QByteArray>
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class YouTubeSearchService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

public:
    /**
     * @brief Constructs a YouTubeSearchService with an optional parent.
     * @param parent Optional QObject parent for Qt ownership.
     */
    explicit YouTubeSearchService(QObject* parent = nullptr);

    /**
     * @brief Returns the list of search results.
     * @return QVariantList containing maps of video metadata (id, title, channel, etc.).
     */
    QVariantList results() const;

    /**
     * @brief Returns whether a search is currently in progress.
     * @return True if a search operation is active.
     */
    bool busy() const;

    /**
     * @brief Returns the last error message, if any.
     * @return Error description string, or empty if no error occurred.
     */
    QString errorString() const;

    /**
     * @brief Initiates a YouTube search via yt-dlp.
     * @param query The search query string to look up on YouTube.
     */
    Q_INVOKABLE void search(const QString& query);

    /**
     * @brief Cancels the currently running search operation.
     */
    Q_INVOKABLE void cancel();

Q_SIGNALS:
    void resultsChanged();
    void busyChanged();
    void errorStringChanged();

private:
    /**
     * @brief Sets the busy state and emits busyChanged().
     * @param busy New busy state.
     */
    void setBusy(bool busy);

    /**
     * @brief Sets the error string and emits errorStringChanged().
     * @param error New error description.
     */
    void setErrorString(const QString& error);

    /** @brief QProcess used to launch yt-dlp searches. */
    QProcess m_process;
    /** @brief Cached list of parsed search results. */
    QVariantList m_results;
    /** @brief True when a search operation is active. */
    bool m_busy{false};
    /** @brief True when the user has requested search cancellation. */
    bool m_cancelRequested{false};
    /** @brief True when the active search exceeded its deadline. */
    bool m_timedOut{false};
    /** @brief True when yt-dlp produced more output than the service accepts. */
    bool m_outputTooLarge{false};
    /** @brief Bounded standard output captured from yt-dlp. */
    QByteArray m_standardOutput;
    /** @brief Bounded standard error captured from yt-dlp. */
    QByteArray m_standardError;
    /** @brief Terminates searches that stop making progress indefinitely. */
    QTimer m_timeoutTimer;
    /** @brief Last error message, empty if no error. */
    QString m_errorString;
};
