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

#if defined(__MINGW32__) || defined(__MINGW64__)
extern "C" {
    int __argc = 0;
    char** __argv = nullptr;
    int* __imp___argc = &__argc;
    char*** __imp___argv = &__argv;
}
#endif

#include <QDir>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCursor>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QLocale>
#include <QPointer>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QTimer>
#include <QTranslator>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "app/ApplicationLog.h"
#include "app/LoggingCategories.h"
#include "app/StartupShell.h"
#include "player/CineMpvItem.h"
#include "player/IpcServer.h"
#include "utils/PathUtils.h"

#include <limits>
#include <utility>

namespace {
/**
 * @brief Loads and installs a QTranslator based on the user's locale setting.
 * @param app The QGuiApplication instance (owns the translator).
 *
 * Reads the "ui/locale" setting; falls back to QLocale::system().name().
 * Tries the full locale code first (e.g. "zh_CN"), then the short code (e.g. "zh").
 */
void loadTranslator(QGuiApplication& app)
{
    QTranslator* translator = new QTranslator(&app);
    QSettings settings;
    QString localeName = settings.value(QStringLiteral("ui/locale")).toString();

    if (localeName.isEmpty())
    {
        localeName = QLocale::system().name();
    }

    // Try full locale first (e.g. zh_CN), then short code (e.g. zh)
    const QString languageName = localeName.section(QLatin1Char('_'), 0, 0)
                                     .section(QLatin1Char('-'), 0, 0)
                                     .section(QLatin1Char('@'), 0, 0);
    QStringList localeCandidates{localeName};
    if (!languageName.isEmpty() && languageName != localeName)
    {
        localeCandidates.append(languageName);
    }

    for (const QString& candidate : std::as_const(localeCandidates))
    {
        const QString resourcePath = QStringLiteral(":/i18n/CineWindows_%1.qm").arg(candidate);
        if (translator->load(resourcePath))
        {
            app.installTranslator(translator);
            return;
        }
    }

    delete translator;
}
} // namespace

/**
 * @brief Checks whether a command-line argument matches a given name.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @param name The string to match against each argument.
 * @return True if any argument equals @p name.
 */
static bool hasArg(int argc, char* argv[], const QString& name)
{
    for (int i = 1; i < argc; ++i)
    {
        if (QString::fromLocal8Bit(argv[i]) == name)
            return true;
    }
    return false;
}

#ifdef Q_OS_WIN
/**
 * @brief Checks whether a given Windows standard handle is redirected.
 * @param handleId STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, or STD_ERROR_HANDLE.
 * @return True if the handle is a pipe or disk file.
 */
static bool isRedirectedStandardHandle(DWORD handleId)
{
    const HANDLE handle = GetStdHandle(handleId);
    if (handle == nullptr || handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    const DWORD type = GetFileType(handle);
    return type == FILE_TYPE_PIPE || type == FILE_TYPE_DISK;
}

/**
 * @brief Attaches or allocates a Windows console and redirects stdio streams.
 *
 * Only redirects handles that are not already piped to a parent process.
 * Unbuffered I/O is set on all three standard streams.
 */
static void prepareWindowsConsole()
{
    const bool stdinRedirected = isRedirectedStandardHandle(STD_INPUT_HANDLE);
    const bool stdoutRedirected = isRedirectedStandardHandle(STD_OUTPUT_HANDLE);
    const bool stderrRedirected = isRedirectedStandardHandle(STD_ERROR_HANDLE);

    if ((!stdinRedirected || !stdoutRedirected || !stderrRedirected)
        && !AttachConsole(ATTACH_PARENT_PROCESS))
    {
        AllocConsole();
    }

    FILE* fp = nullptr;
    if (!stdinRedirected)
    {
        freopen_s(&fp, "CONIN$", "r", stdin);
    }
    if (!stdoutRedirected)
    {
        freopen_s(&fp, "CONOUT$", "w", stdout);
    }
    if (!stderrRedirected)
    {
        freopen_s(&fp, "CONOUT$", "w", stderr);
    }
    setvbuf(stdin, nullptr, _IONBF, 0);
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
}
#endif

/**
 * @brief Application entry point.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return EXIT_SUCCESS or EXIT_FAILURE.
 *
 * Initialises the graphics API, parses CLI arguments, sets up the QML engine,
 * displays a splash window, starts IPC / CLI services, and enters the Qt
 * event loop.
 */
int main(int argc, char* argv[])
{
    QElapsedTimer startupTimer;
    startupTimer.start();
    const bool consoleRequested = hasArg(argc, argv, QStringLiteral("--cli"))
        || hasArg(argc, argv, QStringLiteral("--help")) || hasArg(argc, argv, QStringLiteral("-h"))
        || hasArg(argc, argv, QStringLiteral("--version")) || hasArg(argc, argv, QStringLiteral("-v"));

#ifdef Q_OS_WIN
    // Attach or create a console when running in CLI / help / version mode
    if (consoleRequested)
    {
        prepareWindowsConsole();
    }
#else
    Q_UNUSED(consoleRequested);
#endif

    // MpvQt uses QQuickFramebufferObject and libmpv's OpenGL render API, so the
    // Qt Quick scene graph must use OpenGL on every supported desktop platform.
#if defined(Q_OS_WIN)
    qputenv("QSG_RHI_BACKEND", QByteArrayLiteral("opengl"));
    qputenv("QT_OPENGL", QByteArrayLiteral("desktop"));
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
#elif defined(Q_OS_LINUX)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
#elif defined(Q_OS_MACOS)
    qputenv("QSG_RHI_BACKEND", QByteArrayLiteral("opengl"));
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
#endif

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("CineWindows"));
    app.setApplicationDisplayName(QStringLiteral(APP_DISPLAY_NAME));
    app.setApplicationVersion(QStringLiteral(APP_VERSION));
    app.setOrganizationName(QStringLiteral("gyrolet"));
    app.setOrganizationDomain(QStringLiteral("io.github.gyrolet"));
    app.setWindowIcon(QIcon(QStringLiteral(":/cinewindows/icons/apps/CineWindows.svg")));
    ApplicationLog applicationLog;
    qCInfo(cineAppLog).noquote() << app.applicationDisplayName() << app.applicationVersion()
                                 << "starting with Qt" << qVersion()
                                 << "after" << startupTimer.elapsed() << "ms";

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("CineWindows media player with optional mpv-compatible JSON IPC."));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption cliOption({QStringLiteral("c"), QStringLiteral("cli")},
                                       QStringLiteral("Read newline-delimited JSON IPC or raw mpv commands from stdin."));
    const QCommandLineOption ipcOption(
        QStringLiteral("ipc-server"),
        QStringLiteral("Listen for newline-delimited JSON IPC on localhost TCP port <port>."),
        QStringLiteral("port"));
    parser.addOption(cliOption);
    parser.addOption(ipcOption);
    parser.addPositionalArgument(QStringLiteral("files"),
                                 QStringLiteral("Media files or URLs to open."),
                                 QStringLiteral("[files...]"));
    parser.process(app);

    const bool cliMode = parser.isSet(cliOption);
    const QStringList startupPaths = parser.positionalArguments();
    quint16 ipcPort = 0;
    // Validate the --ipc-server port argument
    if (parser.isSet(ipcOption))
    {
        bool ok = false;
        const uint value = parser.value(ipcOption).toUInt(&ok);
        if (!ok || value == 0 || value > std::numeric_limits<quint16>::max())
        {
            qCCritical(cineIpcLog) << "--ipc-server requires a port in the range 1..65535";
            return EXIT_FAILURE;
        }
        ipcPort = static_cast<quint16>(value);
    }

    // Initialise mpv environment and ensure the config directory exists
    PathUtils::setupMpvEnvironment();
    PathUtils::appConfigDir();

    QQuickStyle::setStyle(QStringLiteral("Basic"));
    loadTranslator(app);

    QPointer<QScreen> startupScreen = QGuiApplication::screenAt(QCursor::pos());
    if (!startupScreen)
        startupScreen = app.primaryScreen();
    StartupShell startupShell(app, startupScreen.data());
    startupShell.show();
    app.processEvents();

    // Point the QML disk cache to the standard writable cache location
    qputenv("QML_DISK_CACHE_PATH",
            QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).toLocal8Bit());

    QQmlApplicationEngine engine;
    const QRect startupBounds = startupScreen
        ? startupScreen->availableGeometry() : QRect(0, 0, 1200, 800);
    engine.setInitialProperties({{QStringLiteral("startupPaths"), startupPaths},
                                 {QStringLiteral("startupScreenGeometry"), startupBounds},
                                 {QStringLiteral("diagnostics"), QVariant::fromValue(&applicationLog)}});

    // Exit application if the QML engine fails to create the root component
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] {
            qCCritical(cineAppLog) << "QML root component creation failed";
            QCoreApplication::exit(EXIT_FAILURE);
        },
        Qt::QueuedConnection);
    qCInfo(cineAppLog) << "Creating the main QML window after" << startupTimer.elapsed() << "ms";
    engine.loadFromModule(QStringLiteral("CineWindows"), QStringLiteral("App"));

    if (auto* rootWindow = qobject_cast<QQuickWindow*>(engine.rootObjects().value(0)))
    {
        applicationLog.observeWindow(rootWindow);
        if (startupScreen)
            rootWindow->setScreen(startupScreen.data());
        rootWindow->show();
        startupShell.hide();
        rootWindow->raise();
        rootWindow->requestActivate();
        rootWindow->update();
        qCInfo(cineAppLog) << "Main window shown on"
                          << (rootWindow->screen() ? rootWindow->screen()->name() : QString())
                          << rootWindow->geometry() << "after" << startupTimer.elapsed() << "ms";
    }
    else
    {
        startupShell.hide();
        return EXIT_FAILURE;
    }
    // Start IPC server and/or CLI reader once the QML engine has created the player
    QTimer::singleShot(0, [&engine, cliMode, ipcPort]() {
        CineMpvItem* player = nullptr;
        const auto rootObjects = engine.rootObjects();
        for (QObject* root : rootObjects)
        {
            player = root->findChild<CineMpvItem*>();
            if (player)
                break;
        }
        if (!player)
        {
            qCCritical(cineAppLog) << "QML root did not expose a CineMpvItem";
            return;
        }

        // Launch the TCP JSON IPC server if --ipc-server was given
        if (ipcPort > 0)
        {
            auto* ipc = new IpcServer(qApp);
            ipc->setPlayer(player);
            ipc->setPort(ipcPort);
            ipc->start();
        }

        // Launch the stdin CLI reader if --cli was given
        if (cliMode)
        {
            auto* console = new ConsoleReader(qApp);
            console->setPlayer(player);
            if (console->start())
            {
                qCInfo(cineIpcLog) << "CineWindows CLI ready. Send JSON IPC or raw mpv commands.";
            }
            else
            {
                qCWarning(cineIpcLog) << "CLI input reader failed to start";
            }
        }
    });

    return app.exec();
}
