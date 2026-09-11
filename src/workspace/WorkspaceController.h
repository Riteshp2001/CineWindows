#pragma once

#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

class ApplicationLog;
class QQmlEngine;
class VideoWorkspace;

class WorkspaceController : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    explicit WorkspaceController(QQmlEngine* engine, ApplicationLog* diagnostics, QObject* parent = nullptr);
    ~WorkspaceController() override;

    Q_INVOKABLE void open(const QStringList& paths = {});
    Q_INVOKABLE bool openCurrent(const QString& path, double position, bool paused);

private:
    QQmlEngine* m_engine;
    ApplicationLog* m_diagnostics;
    QPointer<VideoWorkspace> m_workspace;
};