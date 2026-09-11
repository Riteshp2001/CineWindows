#include "WorkspaceController.h"

#include "app/ApplicationLog.h"
#include "app/LoggingCategories.h"
#include "app/SettingsManager.h"
#include "player/CineMpvItem.h"
#include "services/FileService.h"

#include <DockAreaWidget.h>
#include <DockManager.h>
#include <DockWidget.h>
#include <FloatingDockContainer.h>

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QCursor>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaProperty>
#include <QMimeData>
#include <QPalette>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlError>
#include <QQueue>
#include <QQuickWidget>
#include <QScreen>
#include <QSettings>
#include <QStatusBar>
#include <QStyle>
#include <QStyleHints>
#include <QTimer>
#include <QToolBar>
#include <QUuid>
#include <algorithm>
#include <cmath>

namespace
{
QString mediaPath(const QString& input)
{
    FileService files;
    return files.normalizeMediaPath(input);
}

void keepOnScreen(QWidget* window)
{
    if (!window || window->isMaximized() || window->isFullScreen())
        return;
    QScreen* screen = QGuiApplication::screenAt(window->frameGeometry().center());
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    if (!screen)
        return;
    const QRect available = screen->availableGeometry().adjusted(16, 32, -16, -16);
    const QSize size = window->size().boundedTo(available.size());
    window->resize(size);
    window->move(std::clamp(window->x(), available.left(), std::max(available.left(), available.right() - size.width() + 1)),
        std::clamp(window->y(), available.top(), std::max(available.top(), available.bottom() - size.height() + 1)));
}

void initializeNativePalette(QQmlEngine* engine, QObject* owner)
{
    auto* theme = engine->singletonInstance<QObject*>("CineWindows", "Theme");
    if (!theme)
        return;
    const auto apply = [theme] {
        QPalette palette = QApplication::style()->standardPalette();
        const QColor background = theme->property("background").value<QColor>();
        const QColor foreground = theme->property("text").value<QColor>();
        const QColor panel = theme->property("panel").value<QColor>();
        const QColor strong = theme->property("panelStrong").value<QColor>();
        const QColor muted = theme->property("mutedText").value<QColor>();
        palette.setColor(QPalette::Window, background);
        palette.setColor(QPalette::WindowText, foreground);
        palette.setColor(QPalette::Base, panel);
        palette.setColor(QPalette::AlternateBase, strong);
        palette.setColor(QPalette::Text, foreground);
        palette.setColor(QPalette::Button, strong);
        palette.setColor(QPalette::ButtonText, foreground);
        palette.setColor(QPalette::ToolTipBase, panel);
        palette.setColor(QPalette::ToolTipText, foreground);
        palette.setColor(QPalette::Mid, theme->property("separator").value<QColor>());
        palette.setColor(QPalette::Highlight, theme->property("accent").value<QColor>());
        palette.setColor(QPalette::HighlightedText, theme->property("iconOnDark").value<QColor>());
        palette.setColor(QPalette::PlaceholderText, muted);
        palette.setColor(QPalette::Disabled, QPalette::Text, muted);
        palette.setColor(QPalette::Disabled, QPalette::WindowText, muted);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, muted);
        QApplication::setPalette(palette);
    };
    auto* themeRefresh = new QTimer(owner);
    themeRefresh->setSingleShot(true);
    QObject::connect(themeRefresh, &QTimer::timeout, owner, apply);
    const auto refresh = QTimer::staticMetaObject.method(QTimer::staticMetaObject.indexOfSlot("start()"));
    for (const char* name : {"background", "panel", "panelStrong", "text", "mutedText", "accent", "separator", "iconOnDark"})
    {
        const auto property = theme->metaObject()->property(theme->metaObject()->indexOfProperty(name));
        if (property.hasNotifySignal())
            QObject::connect(theme, property.notifySignal(), themeRefresh, refresh);
    }
    apply();
}
}

class VideoWorkspace : public QMainWindow
{
public:
    VideoWorkspace(QQmlEngine* engine, ApplicationLog* diagnostics)
        : m_engine(engine), m_diagnostics(diagnostics)
    {
        setAttribute(Qt::WA_DeleteOnClose);
        setAcceptDrops(true);
        setMinimumSize(360, 260);
        setWindowTitle(tr("Video Workspace - %1").arg(QGuiApplication::applicationDisplayName()));
        setWindowIcon(QApplication::windowIcon());
        ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting, true);
        ads::CDockManager::setConfigFlag(ads::CDockManager::EqualSplitOnInsertion, true);
        ads::CDockManager::setConfigFlag(ads::CDockManager::MiddleMouseButtonClosesTab, true);
        ads::CDockManager::setConfigFlag(ads::CDockManager::DisableStylesheet, true);
        m_manager = new ads::CDockManager(this);
        m_manager->setColorSchemeMode(ads::CDockManager::ColorSchemeMode::FollowPalette);
        m_fileService = new FileService(this);
        connect(m_fileService, &FileService::mediaChosen, this, [this](const QStringList& paths) { openPaths(paths); });

        auto* fileMenu = menuBar()->addMenu(tr("&File"));
        auto* openAction = fileMenu->addAction(style()->standardIcon(QStyle::SP_DialogOpenButton), tr("Open Media..."));
        openAction->setShortcut(QKeySequence::Open);
        connect(openAction, &QAction::triggered, this, [this] { openFiles(); });
        auto* urlAction = fileMenu->addAction(tr("Open Path or URL..."));
        connect(urlAction, &QAction::triggered, this, [this] { openFiles(); });
        fileMenu->addSeparator();
        auto* closeAction = fileMenu->addAction(tr("Close Video"));
        closeAction->setShortcut(QKeySequence::Close);
        connect(closeAction, &QAction::triggered, this, [this] {
            if (auto* dock = m_manager->focusedDockWidget())
                dock->closeDockWidget();
        });
        auto* closeAllAction = fileMenu->addAction(tr("Close All Videos"));
        connect(closeAllAction, &QAction::triggered, this, [this] { closeAll(); });
        fileMenu->addSeparator();
        auto* closeWindowAction = fileMenu->addAction(tr("Close Workspace"));
        connect(closeWindowAction, &QAction::triggered, this, &QWidget::close);
        m_paneActions = {openAction, closeAction};

        auto* playbackMenu = menuBar()->addMenu(tr("&Playback"));
        auto* playAction = playbackMenu->addAction(style()->standardIcon(QStyle::SP_MediaPlay), tr("Play All"));
        auto* pauseAction = playbackMenu->addAction(style()->standardIcon(QStyle::SP_MediaPause), tr("Pause All"));
        auto* muteAction = playbackMenu->addAction(style()->standardIcon(QStyle::SP_MediaVolumeMuted), tr("Mute All"));
        connect(playAction, &QAction::triggered, this, [this] { setAll("setPaused", false); });
        connect(pauseAction, &QAction::triggered, this, [this] { setAll("setPaused", true); });
        connect(muteAction, &QAction::triggered, this, [this] { setAll("setMuted", true); });
        auto* listenAction = playbackMenu->addAction(tr("Listen to Focused Video"));
        connect(listenAction, &QAction::triggered, this, [this] {
            const auto* focused = m_manager->focusedDockWidget();
            if (!focused)
                return;
            for (const auto& pane : m_panes)
                command(pane.view, "setMuted", pane.dock != focused);
        });
        auto* unmuteAction = playbackMenu->addAction(tr("Unmute All"));
        connect(unmuteAction, &QAction::triggered, this, [this] { setAll("setMuted", false); });

        auto* layoutMenu = menuBar()->addMenu(tr("&Layout"));
        auto* tileAction = layoutMenu->addAction(style()->standardIcon(QStyle::SP_TitleBarNormalButton), tr("Tile Videos"));
        connect(tileAction, &QAction::triggered, this, [this] { arrange(Layout::Tiles); });
        auto* sideAction = layoutMenu->addAction(tr("Side by Side"));
        connect(sideAction, &QAction::triggered, this, [this] { arrange(Layout::Columns); });
        auto* stackAction = layoutMenu->addAction(tr("Stack Vertically"));
        connect(stackAction, &QAction::triggered, this, [this] { arrange(Layout::Rows); });
        auto* tabsAction = layoutMenu->addAction(tr("Tabs"));
        connect(tabsAction, &QAction::triggered, this, [this] { arrange(Layout::Tabs); });
        layoutMenu->addSeparator();
        auto* saveAction = layoutMenu->addAction(tr("Remember Layout"));
        connect(saveAction, &QAction::triggered, this, [this] {
            m_savedLayout = m_manager->saveState(1);
            m_savedNames = m_manager->dockWidgetsMap().keys();
            statusBar()->showMessage(tr("Layout remembered for the current videos."), 4000);
        });
        auto* restoreAction = layoutMenu->addAction(tr("Restore Layout"));
        connect(restoreAction, &QAction::triggered, this, [this] {
            if (m_savedLayout.isEmpty() || m_savedNames != m_manager->dockWidgetsMap().keys())
            {
                statusBar()->showMessage(tr("The remembered layout needs the same open videos."), 5000);
                return;
            }
            if (!m_manager->restoreState(m_savedLayout, 1))
                arrange(Layout::Tiles);
            QTimer::singleShot(0, this, [this] { recoverWindows(); });
        });

        auto* toolbar = addToolBar(tr("Workspace"));
        toolbar->setMovable(false);
        toolbar->setIconSize(QSize(22, 22));
        toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        toolbar->addAction(openAction);
        toolbar->addSeparator();
        toolbar->addAction(playAction);
        toolbar->addAction(pauseAction);
        toolbar->addAction(muteAction);
        toolbar->addSeparator();
        toolbar->addAction(tileAction);

        m_settings = m_engine->singletonInstance<SettingsManager*>("CineWindows", "SettingsManager");
        m_theme = m_engine->singletonInstance<QObject*>("CineWindows", "Theme");
        applyPalette();
        resize(1280, 800);
        QSettings settings;
        if (!restoreGeometry(settings.value(QStringLiteral("workspace/geometry")).toByteArray()))
        {
            QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
            if (!screen)
                screen = QGuiApplication::primaryScreen();
            if (screen)
            {
                const QRect available = screen->availableGeometry();
                resize(size().boundedTo(available.size() * 0.9));
                move(available.center() - rect().center());
            }
        }
        keepOnScreen(this);
        connect(qApp, &QGuiApplication::screenRemoved, this, [this] {
            QTimer::singleShot(0, this, [this] { recoverWindows(); });
        });
        updateCount();
    }

    ~VideoWorkspace() override
    {
        m_closing = true;
        m_pending.clear();
        for (const auto& pane : m_panes)
            stop(pane.view);
        const auto docks = m_ownedDocks;
        for (const auto& dock : docks)
            delete dock.data();
        delete m_manager;
        m_manager = nullptr;
    }

    void openPaths(const QStringList& paths, double position = 0, bool paused = false)
    {
        int skipped = 0;
        for (const QString& input : paths)
        {
            const QString path = mediaPath(input);
            if (path.isEmpty())
                ++skipped;
            else
                m_pending.enqueue({path, std::isfinite(position) ? std::max(0.0, position) : 0.0, paused});
        }
        if (skipped > 0)
        {
            statusBar()->showMessage(tr("Skipped %1 unavailable files or unsupported URLs.").arg(skipped), 10000);
            qCWarning(cinePlayerLog) << "Workspace skipped unavailable inputs:" << skipped;
        }
        processNext();
    }

protected:
    void closeEvent(QCloseEvent* event) override
    {
        m_closing = true;
        QSettings settings;
        settings.setValue(QStringLiteral("workspace/geometry"), saveGeometry());
        closeAll();
        QMainWindow::closeEvent(event);
    }

    void dragEnterEvent(QDragEnterEvent* event) override
    {
        if (event->mimeData()->hasUrls())
            event->acceptProposedAction();
    }

    void dropEvent(QDropEvent* event) override
    {
        if (event->mimeData()->hasUrls())
        {
            FileService files;
            openPaths(files.urlsToPaths(event->mimeData()->urls()));
            event->acceptProposedAction();
        }
    }

    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove)
        {
            auto* drag = static_cast<QDragMoveEvent*>(event);
            if (drag->mimeData()->hasUrls())
            {
                drag->acceptProposedAction();
                return true;
            }
        }
        else if (event->type() == QEvent::Drop)
        {
            auto* drop = static_cast<QDropEvent*>(event);
            if (drop->mimeData()->hasUrls())
            {
                dropEvent(drop);
                return true;
            }
        }
        return QMainWindow::eventFilter(watched, event);
    }

private:
    enum class Layout { Tiles, Columns, Rows, Tabs };
    struct PendingVideo { QString path; double position; bool paused; };
    struct Pane { QPointer<ads::CDockWidget> dock; QPointer<QQuickWidget> view; };

    static void command(QQuickWidget* view, const char* method, bool value)
    {
        if (view && view->rootObject())
            QMetaObject::invokeMethod(view->rootObject(), method, Q_ARG(QVariant, QVariant(value)));
    }

    static void stop(QQuickWidget* view)
    {
        if (view && view->rootObject())
            QMetaObject::invokeMethod(view->rootObject(), "stopPlayback");
    }

    void setAll(const char* method, bool value)
    {
        for (const auto& pane : m_panes)
            command(pane.view, method, value);
        if (qstrcmp(method, "setPaused") == 0)
        {
            for (auto& pending : m_pending)
                pending.paused = value;
        }
    }

    void openFiles()
    {
        m_fileService->openMediaDialog();
    }

    void processNext()
    {
        if (m_closing || m_graphicsFailed || m_initializing || m_pending.isEmpty())
            return;
        const PendingVideo pending = m_pending.dequeue();
        QString title = QFileInfo(pending.path).fileName();
        const QUrl url(pending.path);
        if (!url.scheme().isEmpty() && !QDir::isAbsolutePath(pending.path))
            title = url.fileName().isEmpty() ? url.host() : url.fileName();
        auto* dock = m_manager->createDockWidget(title);
        dock->setObjectName(QStringLiteral("video-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
        dock->setIcon(QApplication::windowIcon());
        dock->setFeatures(ads::CDockWidget::DockWidgetClosable | ads::CDockWidget::DockWidgetMovable
            | ads::CDockWidget::DockWidgetFloatable | ads::CDockWidget::DockWidgetFocusable
            | ads::CDockWidget::DockWidgetDeleteOnClose | ads::CDockWidget::DockWidgetForceCloseWithArea);
        auto* view = new QQuickWidget(m_engine, dock);
        view->setResizeMode(QQuickWidget::SizeRootObjectToView);
        if (m_theme)
            view->setClearColor(m_theme->property("videoBackground").value<QColor>());
        view->setMinimumSize(320, 220);
        view->quickWindow()->setPersistentSceneGraph(true);
        view->quickWindow()->setPersistentGraphics(true);
        if (m_diagnostics)
            m_diagnostics->observeWindow(view->quickWindow());
        connect(view, &QQuickWidget::sceneGraphError, this, [this, view](QQuickWindow::SceneGraphError, const QString& message) {
            m_graphicsFailed = true;
            stop(view);
            setAll("setPaused", true);
            m_pending.clear();
            m_initializing.clear();
            qCCritical(cinePlayerLog) << "Workspace renderer failed:" << message;
            statusBar()->showMessage(tr("Video rendering failed. See application logs."));
            auto* alert = new QMessageBox(QMessageBox::Critical, tr("Video Rendering"), message, QMessageBox::Ok, this);
            alert->setAttribute(Qt::WA_DeleteOnClose);
            alert->open();
        });
        view->installEventFilter(this);
        view->addActions(m_paneActions);
        dock->setWidget(view, ads::CDockWidget::ForceNoScrollArea);

        ads::CDockAreaWidget* target = nullptr;
        for (int index = 0; index < m_manager->dockAreaCount(); ++index)
        {
            auto* area = m_manager->dockArea(index);
            if (!target || area->width() * area->height() > target->width() * target->height())
                target = area;
        }
        ads::DockWidgetArea placement = ads::CenterDockWidgetArea;
        if (target && target->width() >= 660)
            placement = ads::RightDockWidgetArea;
        else if (target && target->height() >= 510)
            placement = ads::BottomDockWidgetArea;
        m_manager->addDockWidget(placement, dock, target);
        dock->setAsCurrentTab();
        if (m_graphicsFailed)
        {
            m_manager->removeDockWidget(dock);
            delete dock;
            return;
        }

        auto* component = new QQmlComponent(m_engine, view);
        component->loadFromModule("CineWindows", "DockedVideo", QQmlComponent::PreferSynchronous);
        const QVariantMap properties{{QStringLiteral("sourcePath"), pending.path},
            {QStringLiteral("requestedPosition"), pending.position}, {QStringLiteral("requestedPause"), pending.paused},
            {QStringLiteral("requestedMute"), !m_panes.isEmpty() || (m_settings && m_settings->muted())},
            {QStringLiteral("requestedVolume"), m_settings ? m_settings->volume() : 100}};
        QObject* root = component->createWithInitialProperties(properties);
        if (!qobject_cast<QQuickItem*>(root))
        {
            qCCritical(cinePlayerLog) << "Could not create docked video:" << component->errors();
            statusBar()->showMessage(tr("Could not create the video pane. See application logs."), 10000);
            delete root;
            m_manager->removeDockWidget(dock);
            delete dock;
            QTimer::singleShot(0, this, [this] { processNext(); });
            return;
        }
        view->setContent(component->url(), component, root);
        m_panes.append({dock, view});
        m_ownedDocks.append(dock);
        connect(dock, &QObject::destroyed, this, [this] {
            m_ownedDocks.removeIf([](const QPointer<ads::CDockWidget>& owned) { return owned.isNull(); });
        });
        m_initializing = view;
        connect(dock, &ads::CDockWidget::closed, this, [this, dock, view] {
            stop(view);
            if (m_initializing == view)
                m_initializing.clear();
            m_panes.removeIf([dock](const Pane& pane) { return pane.dock == dock; });
            if (!m_closing)
            {
                updateCount();
                QTimer::singleShot(0, this, [this] { processNext(); });
            }
        });
        if (auto* player = root->findChild<CineMpvItem*>(QStringLiteral("dockPlayer")))
        {
            connect(player, &CineMpvItem::rendererReadyChanged, this, [this, view, player] {
                if (m_initializing == view && player->rendererReady())
                {
                    m_initializing.clear();
                    QTimer::singleShot(0, this, [this] { processNext(); });
                }
            });
            if (player->rendererReady())
            {
                m_initializing.clear();
                QTimer::singleShot(0, this, [this] { processNext(); });
            }
        }
        view->setFocus(Qt::OtherFocusReason);
        updateCount();
        qCInfo(cinePlayerLog) << "Opened workspace video; active panes:" << m_panes.size();
    }

    void closeAll()
    {
        m_pending.clear();
        m_initializing.clear();
        const auto panes = m_panes;
        for (const auto& pane : panes)
        {
            stop(pane.view);
            if (pane.dock)
                pane.dock->closeDockWidget();
        }
    }

    void arrange(Layout layout)
    {
        if (m_panes.isEmpty())
            return;
        const int count = static_cast<int>(m_panes.size());
        const int maximumColumns = std::max(1, m_manager->width() / 340);
        const int maximumRows = std::max(1, m_manager->height() / 260);
        int columns = layout == Layout::Columns ? count : static_cast<int>(std::ceil(std::sqrt(count)));
        if (layout == Layout::Rows || layout == Layout::Tabs)
            columns = 1;
        columns = std::clamp(columns, 1, maximumColumns);
        const int rows = layout == Layout::Tabs || layout == Layout::Columns ? 1 : std::min(maximumRows, (count + columns - 1) / columns);
        const int capacity = columns * rows;
        setUpdatesEnabled(false);
        for (const auto& pane : m_panes)
            m_manager->removeDockWidget(pane.dock);
        QList<ads::CDockAreaWidget*> areas;
        for (int index = 0; index < count; ++index)
        {
            auto* dock = m_panes[index].dock.data();
            if (index >= capacity)
                m_manager->addDockWidgetTabToArea(dock, areas[index % capacity]);
            else if (index < columns)
                areas.append(m_manager->addDockWidget(index == 0 ? ads::CenterDockWidgetArea : ads::RightDockWidgetArea, dock));
            else
                areas.append(m_manager->addDockWidget(ads::BottomDockWidgetArea, dock, areas[index - columns]));
        }
        for (auto* area : areas)
        {
            if (area->dockWidgetsCount() > 0)
                area->dockWidget(0)->setAsCurrentTab();
        }
        setUpdatesEnabled(true);
        keepOnScreen(this);
    }

    void recoverWindows()
    {
        keepOnScreen(this);
        for (auto* floating : m_manager->floatingWidgets())
            keepOnScreen(floating);
    }

    void updateCount()
    {
        statusBar()->showMessage(tr("%1 videos").arg(m_panes.size()));
    }

    void applyPalette()
    {
        m_manager->setStyleSheet(QStringLiteral(
            "ads--CDockContainerWidget { background: palette(window); }"
            "ads--CDockAreaWidget { background: palette(base); border: 1px solid palette(mid); }"
            "ads--CDockAreaTitleBar { background: palette(button); }"
            "ads--CDockWidgetTab { background: palette(button); color: palette(window-text); padding: 5px 8px; }"
            "ads--CDockWidgetTab[activeTab=\"true\"] { background: palette(base); border-bottom: 2px solid palette(highlight); }"
            "ads--CDockSplitter::handle { background: palette(window); }"));
    }

    QQmlEngine* m_engine;
    ApplicationLog* m_diagnostics;
    SettingsManager* m_settings = nullptr;
    QObject* m_theme = nullptr;
    FileService* m_fileService = nullptr;
    ads::CDockManager* m_manager = nullptr;
    QList<Pane> m_panes;
    QList<QPointer<ads::CDockWidget>> m_ownedDocks;
    QQueue<PendingVideo> m_pending;
    QPointer<QQuickWidget> m_initializing;
    QList<QAction*> m_paneActions;
    QByteArray m_savedLayout;
    QStringList m_savedNames;
    bool m_closing = false;
    bool m_graphicsFailed = false;
};

WorkspaceController::WorkspaceController(QQmlEngine* engine, ApplicationLog* diagnostics, QObject* parent)
    : QObject(parent), m_engine(engine), m_diagnostics(diagnostics)
{
    initializeNativePalette(engine, this);
}

WorkspaceController::~WorkspaceController()
{
    delete m_workspace.data();
}

void WorkspaceController::open(const QStringList& paths)
{
    if (!m_workspace)
        m_workspace = new VideoWorkspace(m_engine, m_diagnostics);
    if (m_workspace->isMinimized())
        m_workspace->showNormal();
    else
        m_workspace->show();
    m_workspace->raise();
    m_workspace->activateWindow();
    m_workspace->openPaths(paths);
}

bool WorkspaceController::openCurrent(const QString& path, double position, bool paused)
{
    const QString normalized = mediaPath(path);
    if (normalized.isEmpty())
        return false;
    open();
    m_workspace->openPaths({normalized}, position, paused);
    return true;
}