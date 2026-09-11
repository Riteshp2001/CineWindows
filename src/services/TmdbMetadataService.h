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
 * Last modified: 2026-09-11
 * Modified by: Ritesh Pandit
 */

#pragma once

#include "services/MediaLibraryService.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QObject>
#include <QPointer>
#include <QQueue>
#include <QString>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class QNetworkReply;

class TmdbMetadataService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(MediaLibraryService* library READ library WRITE setLibrary NOTIFY libraryChanged)
    Q_PROPERTY(QString apiToken READ apiToken WRITE setApiToken NOTIFY apiTokenChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(int completed READ completed NOTIFY progressChanged)
    Q_PROPERTY(int total READ total NOTIFY progressChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit TmdbMetadataService(QObject* parent = nullptr);

    MediaLibraryService* library() const;
    void setLibrary(MediaLibraryService* library);

    QString apiToken() const;
    void setApiToken(const QString& token);

    bool enabled() const;
    void setEnabled(bool enabled);

    bool busy() const;
    int completed() const;
    int total() const;
    QString statusMessage() const;

    Q_INVOKABLE void refresh(bool force = false);
    Q_INVOKABLE void cancel();

Q_SIGNALS:
    void libraryChanged();
    void apiTokenChanged();
    void enabledChanged();
    void busyChanged();
    void progressChanged();
    void statusMessageChanged();

private:
    QNetworkRequest requestFor(const QVariantMap& item) const;
    void processNext();
    void finishRefresh(const QString& message);
    void setBusy(bool busy);
    void setStatusMessage(const QString& message);

    QPointer<MediaLibraryService> m_library;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QQueue<QVariantMap> m_queue;
    QVariantMap m_currentItem;
    QString m_apiToken;
    QString m_statusMessage;
    bool m_enabled{false};
    bool m_busy{false};
    int m_completed{0};
    int m_total{0};
    int m_updated{0};
    int m_failed{0};
};