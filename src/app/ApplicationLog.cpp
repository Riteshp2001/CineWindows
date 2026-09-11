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

#include "app/ApplicationLog.h"

#include "app/LoggingCategories.h"
#include "utils/PathUtils.h"

#include <qtlogger/qtlogger.h>

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QLockFile>
#include <QLoggingCategory>
#include <QMutexLocker>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScreen>
#include <QStandardPaths>
#include <QStringList>
#include <QSysInfo>
#include <QThread>

#ifdef Q_OS_WIN
#include <windows.h>
#elif defined(Q_OS_LINUX)
#include <sys/sysinfo.h>
#elif defined(Q_OS_MACOS)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include <utility>

namespace {
constexpr int MaximumLogFileBytes = 5 * 1024 * 1024;
constexpr int MaximumLogFileCount = 5;
constexpr qint64 MaximumExportLogBytes = 10 * 1024 * 1024;

QString redactReport(const QString& text)
{
    static const QRegularExpression urls(
        QStringLiteral(R"((?:https?|rtsp|ftp|smb)://[^\s<>"']+)"),
        QRegularExpression::CaseInsensitiveOption);
    QString result;
    qsizetype previousEnd = 0;
    auto matches = urls.globalMatch(text);
    while (matches.hasNext())
    {
        const auto match = matches.next();
        result.append(text.mid(previousEnd, match.capturedStart() - previousEnd));
        const QUrl url(match.captured());
        result.append(url.adjusted(QUrl::RemoveUserInfo | QUrl::RemoveQuery | QUrl::RemoveFragment).toString());
        previousEnd = match.capturedEnd();
    }
    result.append(text.mid(previousEnd));

    static const QRegularExpression authorization(
        QStringLiteral(R"(\b(Bearer|Basic)\s+[A-Za-z0-9_+/.=-]+)"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression secrets(
        QStringLiteral(R"(\b((?:api[_-]?(?:key|token)|access[_-]?token|refresh[_-]?token|pairing[_-]?code|token|password|secret)["']?\s*[=:]\s*)(?:"[^"]*"|'[^']*'|[^\s&;]+))"),
        QRegularExpression::CaseInsensitiveOption);
    result.replace(authorization, QStringLiteral("\\1 [redacted]"));
    result.replace(secrets, QStringLiteral("\\1[redacted]"));
    return result;
}

quint64 totalMemoryBytes()
{
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memory{};
    memory.dwLength = static_cast<DWORD>(sizeof(memory));
    if (GlobalMemoryStatusEx(&memory))
        return memory.ullTotalPhys;
#elif defined(Q_OS_LINUX)
    struct sysinfo memory{};
    if (sysinfo(&memory) == 0)
        return static_cast<quint64>(memory.totalram) * memory.mem_unit;
#elif defined(Q_OS_MACOS)
    quint64 memory = 0;
    size_t size = sizeof(memory);
    if (sysctlbyname("hw.memsize", &memory, &size, nullptr, 0) == 0)
        return memory;
#endif
    return 0;
}
}

ApplicationLog::ApplicationLog(QObject* parent)
    : QObject(parent)
{
    const QString requestedPath = PathUtils::logFilePath();
    const QFileInfo logFile(requestedPath);
    if (logFile.dir().exists())
    {
        for (quint64 slot = 1; ; ++slot)
        {
            const QString name = slot == 1 ? logFile.fileName()
                : QStringLiteral("%1-%2.%3").arg(logFile.completeBaseName())
                      .arg(slot).arg(logFile.suffix());
            const QString candidate = logFile.dir().filePath(name);
            auto lock = std::make_unique<QLockFile>(candidate + QStringLiteral(".lock"));
            lock->setStaleLockTime(0);
            if (lock->tryLock(0))
            {
                m_filePath = candidate;
                m_fileLock = std::move(lock);
                break;
            }
            if (lock->error() != QLockFile::LockFailedError)
                break;
        }
    }

    const auto rotationOptions = QtLogger::RotatingFileSink::RotationOnStartup
        | QtLogger::RotatingFileSink::RotationDaily
        | QtLogger::RotatingFileSink::Compression;
    gQtLogger.configure(m_filePath, MaximumLogFileBytes, MaximumLogFileCount,
                        rotationOptions, false);
    m_configured = true;

    if (m_filePath.isEmpty())
        qCWarning(cineAppLog) << "Log file is unavailable; using console logging only";
    else
        qCInfo(cineAppLog).noquote() << "Logging to" << QDir::toNativeSeparators(m_filePath);
    qCInfo(cineAppLog).noquote() << "System diagnostics:\n" << systemInformation();
}

ApplicationLog::~ApplicationLog()
{
    if (!m_configured)
        return;

    qCInfo(cineAppLog) << "CineWindows shutting down";
    QtLogger::Logger::restorePreviousMessageHandler();
    gQtLogger.lock();
    gQtLogger.flush();
    gQtLogger.clear();
    gQtLogger.unlock();
}

QString ApplicationLog::filePath() const
{
    return m_filePath;
}

QString ApplicationLog::directoryPath() const
{
    return QFileInfo(m_filePath.isEmpty() ? PathUtils::logFilePath() : m_filePath).absolutePath();
}

QString ApplicationLog::statusMessage() const
{
    return m_statusMessage;
}

void ApplicationLog::setStatusMessage(const QString& message)
{
    if (m_statusMessage == message)
        return;
    m_statusMessage = message;
    Q_EMIT statusMessageChanged();
}

QUrl ApplicationLog::suggestedExportUrl() const
{
    QString directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (directory.isEmpty())
        directory = QDir::homePath();
    const QString name = QStringLiteral("CineWindows-diagnostics-%1.txt")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
    return QUrl::fromLocalFile(QDir(directory).filePath(name));
}

bool ApplicationLog::openLogsFolder()
{
    const QString directory = directoryPath();
    if (!QDir(directory).exists() || !QDesktopServices::openUrl(QUrl::fromLocalFile(directory)))
    {
        setStatusMessage(tr("Could not open the logs folder."));
        return false;
    }
    setStatusMessage(tr("Logs folder opened."));
    return true;
}

bool ApplicationLog::exportLogs(const QUrl& destination)
{
    if (!destination.isLocalFile() || destination.toLocalFile().isEmpty())
    {
        setStatusMessage(tr("Choose a local file for the diagnostic report."));
        return false;
    }
    const QFileInfo outputFile(destination.toLocalFile());
    const QString canonicalLogs = QDir(directoryPath()).canonicalPath();
    const QString canonicalParent = outputFile.dir().canonicalPath();
    const QString outputPath = QDir(canonicalParent.isEmpty() ? outputFile.absolutePath() : canonicalParent)
        .filePath(outputFile.fileName());
    const QString relativePath = QDir(canonicalLogs.isEmpty() ? directoryPath() : canonicalLogs)
        .relativeFilePath(outputPath);
    if (outputFile.isSymLink() || (!QDir::isAbsolutePath(relativePath) && relativePath != QStringLiteral("..")
        && !relativePath.startsWith(QStringLiteral("../"))))
    {
        setStatusMessage(tr("Save the report outside the logs folder to protect active logs."));
        return false;
    }

    QByteArray logData;
    QString readError;
    bool truncated = false;
    {
        QMutexLocker locker(gQtLogger.mutex());
        gQtLogger.flush();
        QFile source(m_filePath);
        if (m_filePath.isEmpty() || !source.open(QIODevice::ReadOnly))
        {
            readError = tr("The current instance's log is unavailable.");
        }
        else
        {
            truncated = source.size() > MaximumExportLogBytes;
            if (truncated && !source.seek(source.size() - MaximumExportLogBytes))
                readError = source.errorString();
            if (readError.isEmpty())
            {
                logData = source.read(MaximumExportLogBytes);
                if (source.error() != QFileDevice::NoError)
                    readError = source.errorString();
            }
        }
    }

    QString report = QStringLiteral("CineWindows diagnostic report\nGenerated: %1\n\n%2\n\n")
        .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODate), systemInformation());
    report.append(QStringLiteral("Log: %1\n").arg(QFileInfo(m_filePath).fileName()));
    report.append(QStringLiteral("URL credentials, query strings, and common token fields are redacted.\n"
                                 "Local file paths may remain; review before sharing.\n\n"));
    if (!readError.isEmpty())
        report.append(QStringLiteral("Log could not be read: %1\n").arg(readError));
    if (truncated)
        report.append(QStringLiteral("Only the most recent 10 MiB of log data is included.\n"));
    report.append(QString::fromLocal8Bit(logData));
    const QByteArray contents = redactReport(report).toUtf8();

    QSaveFile output(outputPath);
    if (!output.open(QIODevice::WriteOnly) || output.write(contents) != contents.size() || !output.commit())
    {
        setStatusMessage(tr("Could not export logs: %1").arg(output.errorString()));
        return false;
    }
    setStatusMessage(readError.isEmpty() ? tr("Diagnostic report exported.")
                                       : tr("System report exported; the log could not be read."));
    qCInfo(cineAppLog) << "Diagnostic report exported by the user";
    return true;
}

QString ApplicationLog::systemInformation() const
{
    QStringList details{
        QStringLiteral("Application: %1 %2").arg(QGuiApplication::applicationDisplayName(),
                                               QCoreApplication::applicationVersion()),
        QStringLiteral("Qt: %1").arg(QString::fromLatin1(qVersion())),
        QStringLiteral("Operating system: %1").arg(QSysInfo::prettyProductName()),
        QStringLiteral("Kernel: %1 %2").arg(QSysInfo::kernelType(), QSysInfo::kernelVersion()),
        QStringLiteral("CPU architecture: %1").arg(QSysInfo::currentCpuArchitecture()),
        QStringLiteral("Build architecture: %1").arg(QSysInfo::buildCpuArchitecture()),
        QStringLiteral("Logical CPUs: %1").arg(QThread::idealThreadCount()),
        QStringLiteral("Qt platform: %1").arg(QGuiApplication::platformName()),
        QStringLiteral("Process ID: %1").arg(QCoreApplication::applicationPid()),
        QStringLiteral("Graphics: %1").arg(m_graphicsInformation)
    };
    const quint64 memory = totalMemoryBytes();
    if (memory > 0)
        details.append(QStringLiteral("Physical RAM: %1 MiB").arg(memory / (1024 * 1024)));

    const auto rectangleText = [](const QRect& bounds) {
        return QStringLiteral("%1,%2 %3x%4").arg(bounds.x()).arg(bounds.y())
            .arg(bounds.width()).arg(bounds.height());
    };
    const auto screens = QGuiApplication::screens();
    details.append(QStringLiteral("Monitor count: %1").arg(screens.size()));
    for (qsizetype index = 0; index < screens.size(); ++index)
    {
        const QScreen* screen = screens.at(index);
        details.append(QStringLiteral("Monitor %1: %2%3").arg(index + 1)
                           .arg(screen->name(), screen == QGuiApplication::primaryScreen()
                                    ? QStringLiteral(" (primary)") : QString()));
        details.append(QStringLiteral("  Geometry: %1; work area: %2")
                           .arg(rectangleText(screen->geometry()), rectangleText(screen->availableGeometry())));
        details.append(QStringLiteral("  Scale: %1; logical DPI: %2; refresh: %3 Hz")
                           .arg(screen->devicePixelRatio(), 0, 'f', 2)
                           .arg(screen->logicalDotsPerInch(), 0, 'f', 1)
                           .arg(screen->refreshRate(), 0, 'f', 2));
    }
    return details.join(QLatin1Char('\n'));
}

void ApplicationLog::observeWindow(QQuickWindow* window)
{
    if (!window)
        return;
    connect(window, &QQuickWindow::sceneGraphInitialized, this, [this] {
        QOpenGLContext* context = QOpenGLContext::currentContext();
        if (!context)
            return;
        QOpenGLFunctions* functions = context->functions();
        const auto graphicsString = [functions](GLenum field) {
            const GLubyte* value = functions->glGetString(field);
            return value ? QString::fromLatin1(reinterpret_cast<const char*>(value)) : QString();
        };
        const QString graphics = QStringLiteral("%1; %2; OpenGL %3")
            .arg(graphicsString(GL_VENDOR), graphicsString(GL_RENDERER), graphicsString(GL_VERSION));
        QMetaObject::invokeMethod(this, [this, graphics] {
            m_graphicsInformation = graphics;
            qCInfo(cineAppLog).noquote() << "Graphics diagnostics:" << graphics;
        }, Qt::QueuedConnection);
    }, Qt::DirectConnection);
    connect(window, &QWindow::screenChanged, this, [this](QScreen*) {
        qCInfo(cineAppLog).noquote() << "Monitor configuration changed:\n" << systemInformation();
    });
}
