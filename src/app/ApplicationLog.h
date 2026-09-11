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

#include <QObject>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <QtQml/qqmlregistration.h>

#include <memory>

class QLockFile;
class QQuickWindow;

/** Owns the process-wide QtLogger configuration and shutdown lifecycle. */
class ApplicationLog : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QString directoryPath READ directoryPath CONSTANT)
    Q_PROPERTY(QString filePath READ filePath CONSTANT)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit ApplicationLog(QObject* parent = nullptr);
    ~ApplicationLog() override;

    Q_DISABLE_COPY_MOVE(ApplicationLog)

    /** @return Absolute path to the active rotating log file, or empty on fallback. */
    QString filePath() const;
    QString directoryPath() const;
    QString statusMessage() const;
    QString systemInformation() const;
    void observeWindow(QQuickWindow* window);

    Q_INVOKABLE QUrl suggestedExportUrl() const;
    Q_INVOKABLE bool openLogsFolder();
    Q_INVOKABLE bool exportLogs(const QUrl& destination);

Q_SIGNALS:
    void statusMessageChanged();

private:
    void setStatusMessage(const QString& message);

    QString m_filePath;
    QString m_statusMessage;
    QString m_graphicsInformation{QStringLiteral("Not initialized")};
    std::unique_ptr<QLockFile> m_fileLock;
    bool m_configured{false};
};
