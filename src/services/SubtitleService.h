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

#include <QNetworkAccessManager>
#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class SubtitleService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(CineMpvItem* player READ player WRITE setPlayer NOTIFY playerChanged)
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit SubtitleService(QObject* parent = nullptr);

    CineMpvItem* player() const;
    void setPlayer(CineMpvItem* player);

    QString apiKey() const;
    void setApiKey(const QString& apiKey);

    QVariantList results() const;
    bool busy() const;
    QString statusMessage() const;

    Q_INVOKABLE void search(const QString& languages = QStringLiteral("en"),
                            const QString& query = QString(),
                            int season = -1,
                            int episode = -1);

    Q_INVOKABLE void download(const QString& subtitleUrl, const QString& fileName);

Q_SIGNALS:
    void playerChanged();
    void apiKeyChanged();
    void resultsChanged();
    void busyChanged();
    void statusMessageChanged();
    void subtitleReady(const QString& path);

private:
    QNetworkRequest request(const QUrl& url) const;
    void setBusy(bool busy);
    void setStatusMessage(const QString& message);

    CineMpvItem* m_player{nullptr};
    QNetworkAccessManager m_network;
    QString m_apiKey;
    QVariantList m_results;
    bool m_busy{false};
    QString m_statusMessage;
};
