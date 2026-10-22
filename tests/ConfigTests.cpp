#include "player/MpvConfigFilter.h"
#include "utils/PathUtils.h"
#include "models/PlaylistModel.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStringConverter>
#include <QStringEncoder>
#include <QTemporaryDir>
#include <QtGlobal>

#include <cstdlib>

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    int failures = 0;
    const auto check = [&failures](bool passed, const char* name) {
        if (!passed)
        {
            qCritical("Failed: %s", name);
            ++failures;
        }
    };
    using namespace MpvConfigFilter;
    check(optionNameForLine("no-border") == "border", "negated option");
    check(optionNameForLine("--gpu_api = opengl") == "gpu-api", "option normalization");
    check(optionNameForLine("[profile]").isEmpty(), "profile header");
    check(optionValueForLine("path='C:/My Files/#test'") == "C:/My Files/#test", "quoted path");
    check(optionValueForLine("value=yes # comment") == "yes", "inline comment");
    check(optionValueForLine("value=" + fixedLengthValue("a # b")) == "a # b", "fixed value");
    check(isBlockedEmbeddedOption("vo-append"), "video output ownership");
    check(!isBlockedEmbeddedOption("hwdec"), "hardware decoding configurable");
    check(!isBlockedEmbeddedOption("script-opts"), "script options configurable");
    const QString config = "C:/Users/Test User/CineWindows/mpv";
    check(expandMetaPaths("~~/shaders/a.glsl", config) == config + "/shaders/a.glsl", "shader path");
    check(expandCommandMetaPaths("change-list glsl-shaders toggle ~~/a.glsl", config)
              == "change-list glsl-shaders toggle \"" + config + "/a.glsl\"", "command path quoting");
    check(decodeConfigText(QByteArray::fromHex("efbbbf") + "sub-auto=all") == "sub-auto=all", "UTF-8 BOM");
    QStringEncoder encoder(QStringConverter::Utf16LE, QStringConverter::Flag::WriteBom);
    check(decodeConfigText(encoder.encode("sub-auto=all")) == "sub-auto=all", "UTF-16 BOM");

    QTemporaryDir temporary;
    check(temporary.isValid(), "isolated data folder");
    qputenv("CINEWINDOWS_DATA_DIR", temporary.path().toUtf8());
    if (!QFileInfo(QCoreApplication::applicationDirPath() + "/portable_config").isDir())
    {
        const QString directory = PathUtils::mpvConfigDir();
        check(directory == temporary.path() + "/mpv", "dedicated mpv folder");
        check(QFileInfo(directory + "/script-opts").isDir(), "script options folder");
        check(QFileInfo(directory + "/shaders").isDir(), "shaders folder");
        check(QFileInfo(directory + "/fonts").isDir(), "fonts folder");
        check(directory != PathUtils::standardMpvConfigDir(), "standalone mpv isolation");
    }
    PlaylistModel playlist;
    const QStringList original{"one.mp4", "two.mp4", "one.mp4", "three.mp4"};
    playlist.addPaths(original);
    playlist.setCurrentIndex(2);
    playlist.shuffle();
    check(playlist.pathAt(playlist.currentIndex()) == "one.mp4", "shuffle preserves active media");
    playlist.unshuffle();
    check(playlist.currentIndex() == 2, "unshuffle preserves duplicate identity");
    for (int index = 0; index < original.size(); ++index)
        check(playlist.pathAt(index) == original.at(index), "unshuffle restores queue order");
    playlist.shuffle();
    playlist.addPath("four.mp4");
    playlist.unshuffle();
    check(playlist.pathAt(4) == "four.mp4", "new items survive unshuffle");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}