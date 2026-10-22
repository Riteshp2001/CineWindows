#include "player/CineMpvItem.h"
#include "utils/PathUtils.h"

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJSEngine>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class PlaybackTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void subtitlesAndResume()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
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
        player.setMpvOption("resume-playback", true);
        player.setMpvOption("save-position-on-quit", false);
        const QString resumeDirectory = directory.filePath("watch_later");
        player.setMpvOption("watch-later-dir", resumeDirectory);
        QSignalSpy loaded(&player, &CineMpvItem::fileLoaded);
        player.loadFile(mediaPath);
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
        player.savePlaybackPosition();
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

        player.commandBlocking({"stop"});
        QTRY_VERIFY(player.idle());
        player.loadFile(mediaPath);
        QTRY_VERIFY_WITH_TIMEOUT(loaded.count() == 2, 15000);
        QTRY_VERIFY(player.position() >= 12.0);
    }
};

int main(int argc, char* argv[])
{
    qputenv("QSG_RHI_BACKEND", "opengl");
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QGuiApplication application(argc, argv);
    application.setApplicationName("CineWindowsPlaybackTests");
    PlaybackTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "PlaybackTests.moc"