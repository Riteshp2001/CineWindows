#include "app/SettingsManager.h"
#include "models/FileBrowserModel.h"
#include "models/PlaylistModel.h"
#include "player/CineMpvItem.h"
#include "player/PlaybackController.h"
#include "player/ThumbnailController.h"
#include "services/LibraryDatabase.h"
#include "services/MediaLibraryService.h"
#include "utils/PathUtils.h"

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QJSEngine>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QPainter>
#include <QRegularExpression>
#include <QSettings>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>

class PlaybackTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void folderRules()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        qputenv("CINEWINDOWS_DATA_DIR", directory.path().toUtf8());
        const QString root = directory.filePath("Media");
        const QString excluded = root + "/Private";
        const QString sibling = root + "/Private-backup";
        QVERIFY(QDir().mkpath(excluded));
        QVERIFY(QDir().mkpath(sibling));
        const QStringList mediaPaths{root + "/allowed.mp4", excluded + "/hidden.mp4", sibling + "/keep.mp4"};
        for (const QString& path : mediaPaths)
        {
            QFile media(path);
            QVERIFY(media.open(QIODevice::WriteOnly));
        }

        FileBrowserModel browser;
        browser.setIncludedFolders({root});
        browser.setFoldersRestricted(true);
        browser.setExcludedFolders({excluded});
        browser.navigateTo(QUrl::fromLocalFile(root));
        QTRY_VERIFY(!browser.loading());
        QCOMPARE(browser.rowCount(), 2);
        QVERIFY(!browser.canGoUp());
        browser.navigateTo(QUrl::fromLocalFile(excluded));
        QCOMPARE(browser.currentFolder(), QUrl::fromLocalFile(root));
        QVERIFY(!browser.errorString().isEmpty());
        browser.navigateTo(QUrl::fromLocalFile(sibling));
        QTRY_VERIFY(!browser.loading());
        QCOMPARE(browser.rowCount(), 1);
        browser.navigateBack();
        QCOMPARE(browser.currentFolder(), QUrl::fromLocalFile(root));
        browser.setHiddenPlaces({root});
        QCOMPARE(browser.places().size(), 0);
        QCOMPARE(browser.availablePlaces().size(), 1);
        browser.setHiddenPlaces({});
        QCOMPARE(browser.places().size(), 1);
        browser.setExcludedFolders({root});
        QVERIFY(browser.currentFolder().isEmpty());
        QCOMPARE(browser.rowCount(), 0);
        browser.setExcludedFolders({excluded});
        browser.setIncludedFolders({});
        QVERIFY(browser.currentFolder().isEmpty());
        QCOMPARE(browser.rowCount(), 0);

        MediaLibraryService library;
        library.initialize();
        QVERIFY(library.addRoot(QUrl::fromLocalFile(root)));
        QVERIFY(library.addExcludedFolder(QUrl::fromLocalFile(excluded)));
        QTRY_VERIFY(!library.indexing());
        QCOMPARE(library.library().size(), 2);
        for (const QVariant& value : library.library())
            QVERIFY(!PathUtils::isPathWithinDirectory(value.toMap().value("path").toString(), excluded));
        MediaLibraryService reloaded;
        reloaded.initialize();
        QCOMPARE(reloaded.excludedFolders().size(), 1);
        QCOMPARE(reloaded.excludedFolders().first().toMap().value("path").toString(), excluded);
        library.removeExcludedFolder(excluded);
        QTRY_VERIFY(!library.indexing());
        QCOMPARE(library.library().size(), 3);
        library.rescanAll();
        library.removeRoot(root);
        QTRY_VERIFY(!library.indexing());
        QVERIFY(library.library().isEmpty());
        QVERIFY(library.roots().isEmpty());
    }

    void librarySchemaMigration()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        qputenv("CINEWINDOWS_DATA_DIR", directory.path().toUtf8());
        auto database = LibraryDatabase::open("folder-rules-migration");
        QVERIFY(database.isOpen());
        QSqlQuery query(database);
        QVERIFY(query.exec("INSERT INTO app_meta(key, value) VALUES('migration-marker', 'preserved')"));
        QVERIFY(query.exec("DROP TABLE library_exclusions"));
        QVERIFY(query.exec("PRAGMA user_version = 2"));
        QVERIFY(LibraryDatabase::initialize(database));
        QVERIFY(query.exec("SELECT value FROM app_meta WHERE key='migration-marker'"));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), QString("preserved"));
        QVERIFY(query.exec("SELECT locator FROM library_exclusions"));
        query.finish();
        QVERIFY(query.exec("PRAGMA user_version"));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 3);
        query.finish();
        QVERIFY(LibraryDatabase::initialize(database));
        database.close();
    }

    void thumbnails_data()
    {
        QTest::addColumn<bool>("network");
        QTest::addColumn<bool>("cookieProtected");
        QTest::addColumn<bool>("headerProtected");
        QTest::addColumn<bool>("redirected");
        QTest::addColumn<bool>("thumbfast");
        QTest::newRow("local") << false << false << false << false << false;
        QTest::newRow("http-range") << true << false << false << false << false;
        QTest::newRow("http-cookie") << true << true << false << false << false;
        QTest::newRow("http-headers") << true << false << true << false << false;
        QTest::newRow("http-redirect") << true << false << true << true << false;
        QTest::newRow("thumbfast-local") << false << false << false << false << true;
        QTest::newRow("thumbfast-http-cookie") << true << true << false << false << true;
    }

    void thumbnails()
    {
        QFETCH(bool, network);
        QFETCH(bool, cookieProtected);
        QFETCH(bool, headerProtected);
        QFETCH(bool, redirected);
        QFETCH(bool, thumbfast);
        if (thumbfast)
            qunsetenv("CINEWINDOWS_DISABLE_THUMBFAST");
        else
            qputenv("CINEWINDOWS_DISABLE_THUMBFAST", "1");
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        qputenv("CINEWINDOWS_DATA_DIR", directory.path().toUtf8());
        PathUtils::setupMpvEnvironment();
        QByteArray video("YUV4MPEG2 W64 H36 F2:1 Ip A1:1 C420jpeg\n");
        for (int frame = 0; frame < 60; ++frame)
        {
            video += "FRAME\n";
            video += QByteArray(64 * 36, static_cast<char>(40 + frame * 2));
            video += QByteArray(32 * 18 * 2, static_cast<char>(128));
        }
        const QString generatedMediaPath = directory.filePath("preview.y4m");
        if (network || qEnvironmentVariableIsEmpty("CINEWINDOWS_THUMBNAIL_TEST_MEDIA"))
        {
            QFile media(generatedMediaPath);
            QVERIFY(media.open(QIODevice::WriteOnly));
            QCOMPARE(media.write(video), qint64(video.size()));
            media.close();
        }
        const QString mediaPath = network ? generatedMediaPath
            : qEnvironmentVariable("CINEWINDOWS_THUMBNAIL_TEST_MEDIA", generatedMediaPath);
        QVERIFY(QFile::exists(mediaPath));

        int rejectedRequests = 0;
        QTcpServer server;
        connect(&server, &QTcpServer::newConnection, &server, [&] {
            while (server.hasPendingConnections())
            {
                QTcpSocket* socket = server.nextPendingConnection();
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
                connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
                    QByteArray request = socket->property("request").toByteArray() + socket->readAll();
                    socket->setProperty("request", request);
                    if (!request.contains("\r\n\r\n") || socket->property("responded").toBool())
                        return;
                    socket->setProperty("responded", true);
                    if (request.startsWith("GET /redirect ") || request.startsWith("HEAD /redirect "))
                    {
                        socket->write("HTTP/1.1 302 Found\r\nLocation: /preview.y4m\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
                        socket->disconnectFromHost();
                        return;
                    }
                    if ((cookieProtected && !request.contains("cine_preview=allowed"))
                        || (headerProtected && (!request.contains("User-Agent: CineWindows-preview-test")
                            || !request.contains("Referer: http://preview.example/")
                            || !request.contains("X-Cine-Preview: allowed"))))
                    {
                        ++rejectedRequests;
                        qWarning().noquote() << "Rejected preview fixture request:" << request;
                        socket->write("HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
                        socket->disconnectFromHost();
                        return;
                    }
                    const QRegularExpression range("^Range:\\s*bytes=(\\d+)-(\\d*)",
                        QRegularExpression::MultilineOption | QRegularExpression::CaseInsensitiveOption);
                    const auto match = range.match(QString::fromLatin1(request));
                    const qint64 start = match.hasMatch() ? match.captured(1).toLongLong() : 0;
                    const qint64 last = match.hasMatch() && !match.captured(2).isEmpty()
                        ? qMin(match.captured(2).toLongLong(), qint64(video.size() - 1)) : video.size() - 1;
                    if (start > last || start >= video.size())
                    {
                        socket->write("HTTP/1.1 416 Range Not Satisfiable\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
                        socket->disconnectFromHost();
                        return;
                    }
                    const QByteArray payload = video.mid(start, last - start + 1);
                    QByteArray response = match.hasMatch() ? "HTTP/1.1 206 Partial Content\r\n" : "HTTP/1.1 200 OK\r\n";
                    response += "Accept-Ranges: bytes\r\nContent-Type: application/octet-stream\r\n";
                    response += "Content-Length: " + QByteArray::number(payload.size()) + "\r\n";
                    if (match.hasMatch())
                        response += "Content-Range: bytes " + QByteArray::number(start) + "-"
                            + QByteArray::number(last) + "/" + QByteArray::number(video.size()) + "\r\n";
                    response += "Connection: close\r\n\r\n";
                    socket->write(response);
                    if (!request.startsWith("HEAD "))
                        socket->write(payload);
                    socket->disconnectFromHost();
                });
            }
        });
        QVERIFY(server.listen(QHostAddress::LocalHost));
        const QString source = network
            ? QString("http://127.0.0.1:%1/%2").arg(server.serverPort()).arg(redirected ? "redirect" : "preview.y4m") : mediaPath;
        const QString cookiesPath = directory.filePath("cookies.txt");
        QFile cookies(cookiesPath);
        QVERIFY(cookies.open(QIODevice::WriteOnly));
        QVERIFY(cookies.write("# Netscape HTTP Cookie File\n") > 0);
        const QList<QByteArray> cookieDomains{
            QByteArray("127.0.0.1"), QByteArray("127.0.0.1:") + QByteArray::number(server.serverPort())};
        for (const QByteArray& domain : cookieDomains)
            QVERIFY(cookies.write(domain + "\tFALSE\t/\tFALSE\t4102444800\tcine_preview\tallowed\n") > 0);
        cookies.close();

        QQuickWindow window;
        window.resize(640, 360);
        CineMpvItem player(window.contentItem());
        player.setSize(QSizeF(640, 360));
        window.show();
        QTRY_VERIFY_WITH_TIMEOUT(player.rendererReady(), 15000);
        if (thumbfast && !player.thumbfastAvailable())
            QSKIP("A standalone mpv executable is required for this integration row");
        player.setMpvOption("ao", "null");
        player.setMpvOption("ytdl", "no");
        player.setMpvOption("cookies", cookieProtected);
        player.setMpvOption("cookies-file", cookiesPath);
        QCOMPARE(player.mpvOption("cookies").toBool(), cookieProtected);
        QCOMPARE(player.mpvOption("cookies-file").toString(), cookiesPath);
        if (headerProtected)
        {
            player.setMpvOption("user-agent", "CineWindows-preview-test");
            player.setMpvOption("referrer", "http://preview.example/");
            player.setMpvOption("http-header-fields", QStringList{"X-Cine-Preview: allowed"});
        }
        QSignalSpy loaded(&player, &CineMpvItem::fileLoaded);
        player.loadFile(source);
        QTRY_VERIFY_WITH_TIMEOUT(loaded.count() == 1, 15000);
        QTRY_VERIFY_WITH_TIMEOUT(player.videoWidth() > 0, 15000);

        if (!network && !thumbfast && qEnvironmentVariableIsEmpty("CINEWINDOWS_THUMBNAIL_TEST_MEDIA"))
        {
            connect(&window, &QQuickWindow::widthChanged, &player, [&window, &player] {
                player.setWidth(window.width());
            });
            connect(&window, &QQuickWindow::heightChanged, &player, [&window, &player] {
                player.setHeight(window.height());
            });
            const auto videoColor = [&window] {
                const QImage image = window.grabWindow();
                return image.isNull() ? QColor() : image.pixelColor(image.width() / 2, image.height() / 2);
            };
            QTRY_VERIFY2(videoColor().isValid() && videoColor().value() > 16,
                qPrintable(QStringLiteral("No presented video frame: color=%1 video-pts=%2 vo-configured=%3")
                    .arg(videoColor().name(QColor::HexArgb), player.mpvOption("video-pts").toString(),
                         player.mpvOption("vo-configured").toString())));
            player.setPause(true);
            const QColor baseline = videoColor();
            const auto frameMatches = [&videoColor, baseline] {
                const QColor current = videoColor();
                return current.isValid() && qAbs(current.red() - baseline.red()) <= 8
                    && qAbs(current.green() - baseline.green()) <= 8
                    && qAbs(current.blue() - baseline.blue()) <= 8;
            };
            QQmlEngine engine;
            QQmlComponent component(&engine);
            component.setData("import QtQuick\nRectangle { width: 160; height: 40; clip: true; color: '#222222'; "
                              "Text { anchors.centerIn: parent; text: 'Volume: 75%'; color: 'white' } }", QUrl());
            QScopedPointer<QObject> object(component.create());
            auto* overlay = qobject_cast<QQuickItem*>(object.data());
            QVERIFY2(overlay, qPrintable(component.errorString()));
            overlay->setParentItem(window.contentItem());
            for (const bool shown : {true, false, true, false})
            {
                overlay->setVisible(shown);
                QTRY_VERIFY(frameMatches());
            }
            window.showFullScreen();
            player.setFullscreen(true);
            QTRY_COMPARE(window.visibility(), QWindow::FullScreen);
            QTRY_VERIFY(frameMatches());
            overlay->setVisible(true);
            QTRY_VERIFY(frameMatches());
            overlay->setVisible(false);
            window.showNormal();
            player.setFullscreen(false);
            QTRY_COMPARE(window.visibility(), QWindow::Windowed);
            QTRY_VERIFY(frameMatches());
        }

        player.setPause(true);
        ThumbnailController thumbnails;
        thumbnails.setSize(QSizeF(174, 98));
        thumbnails.setPlayer(&player);
        QSignalSpy ready(&thumbnails, &ThumbnailController::thumbnailReady);
        QElapsedTimer clock;
        qint64 firstFrameLatencyMs = -1;
        bool measureFrameLatency = false;
        connect(&thumbnails, &ThumbnailController::thumbnailReady, &thumbnails, [&](double) {
            if (measureFrameLatency && firstFrameLatencyMs < 0)
                firstFrameLatencyMs = clock.elapsed();
        });
        clock.start();
        measureFrameLatency = true;
        thumbnails.request(source, 5.0, 0, 0, 174, 98);
        QCOMPARE(thumbnails.thumbfastMode(), thumbfast && !cookieProtected);
        if (thumbnails.thumbfastMode())
        {
            QTRY_COMPARE_WITH_TIMEOUT(ready.count(), 1, 15000);
            const QDir tempDirectory(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
            QElapsedTimer frameWait;
            frameWait.start();
            while (tempDirectory.entryList({QStringLiteral("thumbfast.out*.bgra")}, QDir::Files).isEmpty()
                   && frameWait.elapsed() < 10000)
                QTest::qWait(50);
            QVERIFY2(!tempDirectory.entryList({QStringLiteral("thumbfast.out*.bgra")}, QDir::Files).isEmpty(),
                     "thumbfast did not write a raw frame");
            const QFileInfoList frames = tempDirectory.entryInfoList(
                {QStringLiteral("thumbfast.out*.bgra")}, QDir::Files, QDir::Time);
            QVERIFY(!frames.isEmpty());
            QVERIFY(frames.first().size() > 0);
            QCOMPARE(rejectedRequests, 0);
            QVERIFY(player.pause());
            QCOMPARE(QDir::fromNativeSeparators(player.currentPath()), QDir::fromNativeSeparators(source));
            thumbnails.clear();
            return;
        }
        QTRY_VERIFY_WITH_TIMEOUT(firstFrameLatencyMs >= 0, 15000);
        qInfo() << QTest::currentDataTag() << "cold first-frame ms:" << firstFrameLatencyMs;
        QTRY_VERIFY_WITH_TIMEOUT(ready.count() >= 2, 5000);
        QTest::qWait(500);
        for (const double seekTime : {15.0, 25.0, 10.0})
        {
            const int previousReadyCount = ready.count();
            firstFrameLatencyMs = -1;
            clock.restart();
            thumbnails.request(source, seekTime, 0, 0, 174, 98);
            QTRY_VERIFY_WITH_TIMEOUT(firstFrameLatencyMs >= 0, 15000);
            qInfo() << QTest::currentDataTag() << "warm first-frame ms at" << seekTime << ":"
                    << firstFrameLatencyMs;
            QTRY_VERIFY_WITH_TIMEOUT(ready.count() >= previousReadyCount + 2, 5000);
            QTest::qWait(500);
        }
        QImage image(174, 98, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        thumbnails.paint(&painter);
        painter.end();
        QVERIFY(qAlpha(image.pixel(87, 49)) > 0);
        thumbnails.clear();
        ready.clear();
        clock.restart();
        thumbnails.request(source, 5.0, 0, 0, 174, 98);
        QCOMPARE(ready.count(), 1);
        qInfo() << QTest::currentDataTag() << "cached thumbnail ms:" << clock.elapsed();
        QCOMPARE(rejectedRequests, 0);
        QVERIFY(player.pause());
        QCOMPARE(QDir::fromNativeSeparators(player.currentPath()), QDir::fromNativeSeparators(source));
        thumbnails.clear();
    }

    void thumbfastIntegration()
    {
        if (PathUtils::mediaToolPath(QStringLiteral("mpv")).isEmpty())
            QSKIP("A standalone mpv executable is required for this integration test");

        qunsetenv("CINEWINDOWS_DISABLE_THUMBFAST");
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        qputenv("CINEWINDOWS_DATA_DIR", directory.path().toUtf8());
        PathUtils::setupMpvEnvironment();

        QByteArray video("YUV4MPEG2 W64 H36 F2:1 Ip A1:1 C420jpeg\n");
        for (int frame = 0; frame < 60; ++frame)
        {
            video += "FRAME\n";
            video += QByteArray(64 * 36, static_cast<char>(40 + frame * 2));
            video += QByteArray(32 * 18 * 2, static_cast<char>(128));
        }
        const QString mediaPath = directory.filePath("thumbfast.y4m");
        QFile media(mediaPath);
        QVERIFY(media.open(QIODevice::WriteOnly));
        QCOMPARE(media.write(video), qint64(video.size()));
        media.close();

        QQuickWindow window;
        window.resize(640, 360);
        CineMpvItem player(window.contentItem());
        player.setSize(QSizeF(640, 360));
        window.show();
        QTRY_VERIFY_WITH_TIMEOUT(player.rendererReady(), 15000);
        QVERIFY(player.thumbfastAvailable());
        player.setMpvOption("ao", "null");
        player.setMpvOption("ytdl", "no");
        QSignalSpy loaded(&player, &CineMpvItem::fileLoaded);
        player.loadFile(mediaPath);
        QTRY_COMPARE_WITH_TIMEOUT(loaded.count(), 1, 15000);
        QTRY_VERIFY_WITH_TIMEOUT(player.videoWidth() > 0, 15000);
        player.setPause(true);

        ThumbnailController thumbnails;
        thumbnails.setPlayer(&player);
        QSignalSpy ready(&thumbnails, &ThumbnailController::thumbnailReady);
        const QString thumbnailPath = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                          .filePath(QStringLiteral("thumbfast.out%1.bgra")
                                                        .arg(QCoreApplication::applicationPid()));
        QFile::remove(thumbnailPath);
        thumbnails.request(mediaPath, 5.0, 120, 80, 174, 96);
        QCOMPARE(ready.count(), 1);
        QTRY_VERIFY_WITH_TIMEOUT(QFileInfo(thumbnailPath).size() > 0, 10000);
        thumbnails.clear();
    }

    void subtitlesAndResume()
    {
        qputenv("CINEWINDOWS_DISABLE_THUMBFAST", "1");
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, directory.filePath("settings"));
        SettingsManager settings;
        QVERIFY(settings.saveVideoPosition());
        settings.setSaveVideoPosition(false);
        SettingsManager optedOutSettings;
        QVERIFY(!optedOutSettings.saveVideoPosition());
        settings.setSaveVideoPosition(true);
        qputenv("CINEWINDOWS_DATA_DIR", directory.path().toUtf8());
        PathUtils::setupMpvEnvironment();

        const QString mediaPath = directory.filePath("sample.wav");
        QFile media(mediaPath);
        QVERIFY(media.open(QIODevice::WriteOnly));
        constexpr quint32 sampleRate = 8000;
        constexpr quint32 dataSize = sampleRate * 2 * 45;
        QDataStream output(&media);
        output.setByteOrder(QDataStream::LittleEndian);
        output.writeRawData("RIFF", 4);
        output << quint32(dataSize + 36);
        output.writeRawData("WAVEfmt ", 8);
        output << quint32(16) << quint16(1) << quint16(1) << sampleRate
               << quint32(sampleRate * 2) << quint16(2) << quint16(16);
        output.writeRawData("data", 4);
        output << dataSize;
        QCOMPARE(media.write(QByteArray(dataSize, '\0')), qint64(dataSize));
        media.close();

        const QString subtitlePath = directory.filePath("sample.srt");
        QFile subtitle(subtitlePath);
        QVERIFY(subtitle.open(QIODevice::WriteOnly));
        QVERIFY(subtitle.write("1\n00:00:00,000 --> 00:00:44,000\nSubtitle regression\n") > 0);
        subtitle.close();

        QQuickWindow window;
        window.resize(640, 360);
        CineMpvItem player(window.contentItem());
        player.setSize(QSizeF(640, 360));
        window.show();
        QTRY_VERIFY_WITH_TIMEOUT(player.rendererReady(), 15000);
        player.setMpvOption("ao", "null");
        PlaylistModel playlist;
        PlaybackController controller;
        controller.setPlayer(&player);
        controller.setPlaylist(&playlist);
        controller.setSettings(&settings);
        controller.applySettings();
        player.setMpvOption("save-position-on-quit", false);
        const QString resumeDirectory = directory.filePath("watch_later");
        player.setMpvOption("watch-later-dir", resumeDirectory);
        QSignalSpy loaded(&player, &CineMpvItem::fileLoaded);
        controller.openPaths({mediaPath}, true);
        QTRY_VERIFY_WITH_TIMEOUT(loaded.count() == 1, 15000);
        QTRY_VERIFY(player.duration() > 40);
        player.setPause(true);
        player.addSubtitle(subtitlePath);
        QTRY_VERIFY(player.mpvOption("sid").toInt() > 0);

        QFile application(QStringLiteral(CINEWINDOWS_SOURCE_DIR "/qml/App.qml"));
        QVERIFY(application.open(QIODevice::ReadOnly));
        const QString source = QString::fromUtf8(application.readAll());
        const QRegularExpression selector(
            "function selectSubtitleTrack\\(trackId\\)\\s*\\{.*?\\n    \\}",
            QRegularExpression::DotMatchesEverythingOption);
        const QRegularExpressionMatch match = selector.match(source);
        QVERIFY(match.hasMatch());
        QJSEngine engine;
        engine.globalObject().setProperty("player", engine.newQObject(&player));
        engine.globalObject().setProperty("subtitleTracks", engine.newObject());
        QVERIFY(!engine.evaluate(match.captured()).isError());
        const QJSValue result = engine.evaluate("selectSubtitleTrack(0)");
        QVERIFY2(!result.isError(), qPrintable(result.toString()));
        QTRY_VERIFY(player.mpvOption("sid").toInt() <= 0);

        player.setPosition(12.5);
        QTRY_VERIFY(qAbs(player.position() - 12.5) < 0.5);
        controller.stop();
        QTRY_VERIFY(player.idle());
        const QDir saved(resumeDirectory);
        const QStringList entries = saved.entryList(QDir::Files | QDir::NoDotAndDotDot);
        bool savedPosition = false;
        for (const QString& entry : entries)
        {
            QFile state(saved.filePath(entry));
            QVERIFY(state.open(QIODevice::ReadOnly));
            savedPosition = savedPosition || state.readAll().contains("start=12");
        }
        QVERIFY(savedPosition);

        controller.openPaths({mediaPath}, true);
        QTRY_VERIFY_WITH_TIMEOUT(loaded.count() == 2, 15000);
        QTRY_VERIFY(player.position() >= 12.0);
        controller.openPathAt(mediaPath, 0.0);
        QTRY_VERIFY_WITH_TIMEOUT(loaded.count() == 3, 15000);
        QTRY_VERIFY(player.position() < 1.0);
    }
};

int main(int argc, char* argv[])
{
    qputenv("QSG_RHI_BACKEND", "opengl");
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QGuiApplication application(argc, argv);
    application.setApplicationName("CineWindowsPlaybackTests");
    application.setOrganizationName("CineWindowsTests");
    PlaybackTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "PlaybackTests.moc"