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

#include "player/MpvConfig.h"

#include "app/LoggingCategories.h"
#include "player/CineMpvItem.h"
#include "player/MpvConfigFilter.h"
#include "utils/PathUtils.h"

#include <QChar>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QMap>
#include <QProcess>
#include <QSaveFile>
#include <QUrl>

#include <utility>

namespace {

constexpr int MaxIncludeDepth = 8;

const QString& mpvConfTemplate()
{
    static const QString text = QStringLiteral(
        "# CineWindows mpv configuration\n"
        "#\n"
        "# This folder belongs to CineWindows only; the standalone mpv player's\n"
        "# configuration is never read or changed.\n"
        "#\n"
        "# Use regular mpv.conf syntax: https://mpv.io/manual/stable/#configuration-files\n"
        "# Supported: [profiles], profile-cond auto profiles, include=, ~~/ paths, hwdec,\n"
        "# ao, glsl-shaders, script-opts and per-script options in script-opts/<name>.conf.\n"
        "# Ignored: options that own the window or video output (vo, gpu-api, gpu-context,\n"
        "# fullscreen, geometry, autofit, border, ontop, osc, idle).\n"
        "# Values set here override CineWindows preferences when the player starts.\n"
        "#\n"
        "# Examples:\n"
        "# profile=high-quality\n"
        "# glsl-shaders=~~/shaders/FSRCNNX_x2_8-0-4-1.glsl\n"
        "# sub-auto=fuzzy\n");
    return text;
}

const QString& inputConfTemplate()
{
    static const QString text = QStringLiteral(
        "# CineWindows key bindings\n"
        "#\n"
        "# Use regular mpv input.conf syntax: https://mpv.io/manual/stable/#input-conf\n"
        "# Keys bound here take priority over CineWindows shortcuts.\n"
        "# \"quit\" closes CineWindows and \"cycle fullscreen\" toggles the window.\n"
        "#\n"
        "# Examples:\n"
        "# Ctrl+h cycle-values hwdec auto-safe no\n"
        "# Ctrl+1 no-osd change-list glsl-shaders toggle \"~~/shaders/Anime4K_Upscale_CNN_x2_M.glsl\"\n");
    return text;
}

/** @brief Returns the template text for a config file name, or nullptr for unknown names. */
const QString* templateFor(const QString& fileName)
{
    if (fileName == QStringLiteral("mpv.conf"))
        return &mpvConfTemplate();
    if (fileName == QStringLiteral("input.conf"))
        return &inputConfTemplate();
    return nullptr;
}

/**
 * @brief Reads a config file as lines, decoding BOM-marked UTF-8/UTF-16 and stripping CR.
 * @param path File to read.
 * @param ok Optional output set to whether the file could be opened.
 */
QStringList readConfigLines(const QString& path, bool* ok = nullptr)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        if (file.exists())
            qCWarning(cinePlayerLog).noquote()
                << "Could not read" << QDir::toNativeSeparators(path) << ':' << file.errorString();
        if (ok)
            *ok = false;
        return {};
    }
    if (ok)
        *ok = true;
    QStringList lines = MpvConfigFilter::decodeConfigText(file.readAll()).split(QLatin1Char('\n'));
    for (QString& line : lines)
    {
        if (line.endsWith(QLatin1Char('\r')))
            line.chop(1);
    }
    return lines;
}

/** @brief Evaluates an mpv flag line such as "load-scripts", "no-load-scripts" or "load-scripts=no". */
bool flagEnabled(const QString& line)
{
    QString name = line.trimmed();
    if (name.startsWith(QStringLiteral("--")))
        name = name.mid(2);
    name = name.section(QLatin1Char('='), 0, 0).trimmed().toLower();
    if (name.startsWith(QStringLiteral("no-")))
        return false;
    bool hasValue = false;
    const QString value = MpvConfigFilter::optionValueForLine(line, &hasValue).trimmed().toLower();
    return !hasValue || value.isEmpty() || value == QStringLiteral("yes");
}

/** @brief Returns true for file types mpv's load-script command understands. */
bool isScriptFile(const QFileInfo& info)
{
    const QString suffix = info.suffix().toLower();
#ifdef Q_OS_WIN
    const QString pluginSuffix = QStringLiteral("dll");
#else
    const QString pluginSuffix = QStringLiteral("so");
#endif
    return suffix == QStringLiteral("lua") || suffix == QStringLiteral("js") || suffix == pluginSuffix;
}

/** @brief Returns true when the file still contains the unmodified CineWindows template. */
bool isUntouchedTemplate(const QString& path)
{
    const QString* text = templateFor(QFileInfo(path).fileName());
    if (!text)
        return false;
    QFile file(path);
    return file.open(QIODevice::ReadOnly) && MpvConfigFilter::decodeConfigText(file.readAll()) == *text;
}

Qt::CaseSensitivity pathCaseSensitivity()
{
#ifdef Q_OS_WIN
    return Qt::CaseInsensitive;
#else
    return Qt::CaseSensitive;
#endif
}

/**
 * @brief Maps a multi-character key name to a canonical, spelling-independent form
 *        shared by mpv's input.conf names and Qt's KeyBindings sequence strings.
 * @param raw Raw key-name string to normalise (e.g. "space", "escape", "pageup").
 * @return Canonical key string in the common mpv/Qt spelling.
 */
QString canonicalBase(const QString& raw)
{
    const QString lowered = raw.toLower();
    if (lowered == QStringLiteral("space"))
        return QStringLiteral("space");
    if (lowered == QStringLiteral("esc") || lowered == QStringLiteral("escape"))
        return QStringLiteral("esc");
    if (lowered == QStringLiteral("return") || lowered == QStringLiteral("enter") || lowered == QStringLiteral("ret")
        || lowered == QStringLiteral("kp_enter"))
        return QStringLiteral("enter");
    if (lowered == QStringLiteral("pgup") || lowered == QStringLiteral("pageup") || lowered == QStringLiteral("prior"))
        return QStringLiteral("pageup");
    if (lowered == QStringLiteral("pgdwn") || lowered == QStringLiteral("pgdown") || lowered == QStringLiteral("pagedown")
        || lowered == QStringLiteral("next"))
        return QStringLiteral("pagedown");
    if (lowered == QStringLiteral("bs") || lowered == QStringLiteral("backspace"))
        return QStringLiteral("backspace");
    if (lowered == QStringLiteral("del") || lowered == QStringLiteral("delete"))
        return QStringLiteral("del");
    if (lowered == QStringLiteral("ins") || lowered == QStringLiteral("insert"))
        return QStringLiteral("ins");
    if (lowered == QStringLiteral("sharp") || lowered == QStringLiteral("#"))
        return QStringLiteral("#");
    if (lowered == QStringLiteral("kp_add"))
        return QStringLiteral("kp_add");
    if (lowered == QStringLiteral("kp_subtract"))
        return QStringLiteral("kp_subtract");
    if (lowered == QStringLiteral("kp_multiply"))
        return QStringLiteral("kp_multiply");
    if (lowered == QStringLiteral("kp_divide"))
        return QStringLiteral("kp_divide");
    // left/right/up/down/home/end/tab/pause and function keys (f1..) pass
    // through unchanged once lowercased.
    return lowered;
}

} // namespace

MpvConfig::MpvConfig(QObject* parent)
    : QObject(parent)
    , m_configDir(PathUtils::mpvConfigDir())
{}

CineMpvItem* MpvConfig::player() const
{
    return m_player;
}

void MpvConfig::setPlayer(CineMpvItem* player)
{
    if (m_player == player)
    {
        return;
    }
    m_player = player;
    Q_EMIT playerChanged();
    if (m_player)
    {
        apply();
    }
}

int MpvConfig::revision() const
{
    return m_revision;
}

QString MpvConfig::configDirectory() const
{
    return QDir::toNativeSeparators(m_configDir);
}

QString MpvConfig::standardConfigDirectory() const
{
    return QDir::toNativeSeparators(PathUtils::standardMpvConfigDir());
}

bool MpvConfig::standardConfigAvailable() const
{
    const QString standard = QDir::cleanPath(PathUtils::standardMpvConfigDir());
    return QFileInfo(standard).isDir() && standard.compare(m_configDir, pathCaseSensitivity()) != 0;
}

QStringList MpvConfig::loadedScripts() const
{
    return m_loadedScripts;
}

QStringList MpvConfig::ignoredOptions() const
{
    return m_ignoredOptions;
}

int MpvConfig::bindingCount() const
{
    return static_cast<int>(m_overrides.size());
}

void MpvConfig::reload()
{
    apply();
}

bool MpvConfig::isOverridden(const QString& qtSequence) const
{
    if (qtSequence.isEmpty())
    {
        return false;
    }
    return m_overrides.contains(normalizeKey(qtSequence, false));
}

bool MpvConfig::isMpvKeyBound(const QString& mpvKey) const
{
    return !mpvKey.isEmpty() && m_overrides.contains(normalizeKey(mpvKey, true));
}

QString MpvConfig::commandFor(const QString& qtSequence) const
{
    return m_overrides.value(normalizeKey(qtSequence, false));
}

bool MpvConfig::openConfigFile(const QString& fileName)
{
    const QString* text = templateFor(fileName);
    if (!text)
        return false;

    const QString path = QDir(m_configDir).filePath(fileName);
    if (!QFileInfo::exists(path))
    {
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(text->toUtf8()) < 0 || !file.commit())
        {
            qCWarning(cinePlayerLog).noquote()
                << "Could not create" << QDir::toNativeSeparators(path) << ':' << file.errorString();
            return false;
        }
    }

    if (QDesktopServices::openUrl(QUrl::fromLocalFile(path)))
        return true;
#ifdef Q_OS_WIN
    // .conf files usually have no associated editor on Windows.
    return QProcess::startDetached(QStringLiteral("notepad.exe"), {QDir::toNativeSeparators(path)});
#else
    return QDesktopServices::openUrl(QUrl::fromLocalFile(m_configDir));
#endif
}

int MpvConfig::importStandardConfig()
{
    if (!standardConfigAvailable())
        return -1;

    const QDir source(PathUtils::standardMpvConfigDir());
    const QDir target(m_configDir);
    int copied = 0;
    QDirIterator it(source.absolutePath(), QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        const QString file = it.next();
        const QString relative = source.relativeFilePath(file);
        const QString top = relative.section(QLatin1Char('/'), 0, 0).toLower();
        if (top == QStringLiteral("watch_later") || top == QStringLiteral("cache"))
            continue;

        const QString destination = target.filePath(relative);
        if (QFileInfo::exists(destination))
        {
            if (!isUntouchedTemplate(destination) || !QFile::remove(destination))
                continue;
        }
        QDir().mkpath(QFileInfo(destination).absolutePath());
        if (QFile::copy(file, destination))
            ++copied;
        else
            qCWarning(cinePlayerLog).noquote() << "Could not import" << QDir::toNativeSeparators(file);
    }
    qCInfo(cinePlayerLog).noquote() << "Imported" << copied << "files from" << QDir::toNativeSeparators(source.path());
    reload();
    return copied;
}

/**
 * @brief Applies the config folder to the player in mpv's own order.
 * @details All player calls are queued-synchronous on the mpv controller thread, so the
 *          include, the script-opts merge, key bindings and scripts run strictly in order
 *          and after any option CineWindows set earlier.
 */
void MpvConfig::apply()
{
    m_overrides.clear();
    m_ignoredOptions.clear();

    if (m_player)
    {
        if (!m_appScriptOptionsCaptured)
        {
            m_appScriptOptions = m_player->getProperty(QStringLiteral("script-opts")).toMap();
            m_appScriptOptionsCaptured = true;
        }
        // Drop script-opts merged from conf files by a previous load so edits take effect.
        for (const QString& key : std::as_const(m_fileScriptOptionKeys))
            m_player->command({QStringLiteral("change-list"), QStringLiteral("script-opts"), QStringLiteral("remove"), key});
        m_fileScriptOptionKeys.clear();

        LoaderOptions options;
        options.inputConf = QDir(m_configDir).filePath(QStringLiteral("input.conf"));
        m_includeCounter = 0;
        const QString mpvConf = QDir(m_configDir).filePath(QStringLiteral("mpv.conf"));
        if (QFileInfo::exists(mpvConf))
        {
            const QString embedded = writeEmbeddedConfig(mpvConf, 0, &options);
            if (!embedded.isEmpty())
                m_player->setProperty(QStringLiteral("include"), embedded);
        }

        applyScriptOptions(m_appScriptOptions);
        // libmpv disables conditional profiles; "auto" only loads the helper when one exists.
        if (!options.autoProfilesSet)
            m_player->setProperty(QStringLiteral("load-auto-profiles"), QStringLiteral("auto"));
        if (!m_autoProfilesLoaded
            && m_player->getProperty(QStringLiteral("load-auto-profiles")).toString() != QStringLiteral("no"))
        {
            m_player->command({QStringLiteral("load-script"), QStringLiteral("@auto_profiles.lua")});
            m_autoProfilesLoaded = true;
        }
        loadInputConf(options.inputConf);
        loadScripts(options);

        qCInfo(cinePlayerLog).noquote()
            << "Applied mpv configuration from" << QDir::toNativeSeparators(m_configDir) << "with"
            << m_overrides.size() << "key bindings and" << m_loadedScripts.size() << "scripts";
        if (!m_ignoredOptions.isEmpty())
            qCInfo(cinePlayerLog).noquote()
                << "Ignored embedded-unsafe mpv options:" << m_ignoredOptions.join(QStringLiteral(", "));
    }

    ++m_revision;
    Q_EMIT revisionChanged();
}

/**
 * @brief Writes a sanitized copy of an mpv config file and returns its path.
 * @details Blocked options are commented out, ~~/ meta paths are expanded, nested
 *          include= files are sanitized recursively and options libmpv only honours at
 *          startup (input-conf, scripts, load-scripts) are collected into @p options.
 * @param sourcePath Config file to read.
 * @param depth Include nesting depth; 0 for mpv.conf.
 * @param options Collected loader options, or nullptr when the file is included inside a profile.
 * @return Path of the sanitized copy, or an empty string on failure.
 */
QString MpvConfig::writeEmbeddedConfig(const QString& sourcePath, int depth, LoaderOptions* options)
{
    if (depth > MaxIncludeDepth)
    {
        qCWarning(cinePlayerLog).noquote() << "mpv include nesting is too deep at" << QDir::toNativeSeparators(sourcePath);
        return {};
    }
    bool ok = false;
    const QStringList lines = readConfigLines(sourcePath, &ok);
    if (!ok)
        return {};

    if (!m_embeddedConfig)
    {
        auto directory =
            std::make_unique<QTemporaryDir>(QDir(QDir::tempPath()).filePath(QStringLiteral("CineWindows-mpv-XXXXXX")));
        if (!directory->isValid())
        {
            qCWarning(cinePlayerLog).noquote()
                << "Could not create the embedded mpv configuration folder:" << directory->errorString();
            return {};
        }
        m_embeddedConfig = std::move(directory);
    }

    QStringList output;
    output.reserve(lines.size());
    bool topLevel = true;
    for (const QString& line : lines)
    {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QLatin1Char('[')))
        {
            const qsizetype close = trimmed.indexOf(QLatin1Char(']'));
            const QString section = (close > 0 ? trimmed.mid(1, close - 1) : trimmed.mid(1)).trimmed();
            topLevel = section.isEmpty() || section.compare(QStringLiteral("default"), Qt::CaseInsensitive) == 0;
            output.append(line);
            continue;
        }

        const QString name = MpvConfigFilter::optionNameForLine(line);
        if (name.isEmpty())
        {
            output.append(line);
            continue;
        }
        const QString base = MpvConfigFilter::baseOptionName(name);
        LoaderOptions* collected = topLevel ? options : nullptr;

        if (base == QStringLiteral("include"))
        {
            const QString target = resolveConfigPath(MpvConfigFilter::optionValueForLine(line));
            const QString nested = target.isEmpty() ? QString() : writeEmbeddedConfig(target, depth + 1, collected);
            output.append(nested.isEmpty() ? QStringLiteral("# CineWindows could not include: %1").arg(target)
                                           : QStringLiteral("include=") + MpvConfigFilter::fixedLengthValue(nested));
            continue;
        }
        if (base == QStringLiteral("input-conf") || base == QStringLiteral("script")
            || base == QStringLiteral("scripts") || base == QStringLiteral("load-scripts"))
        {
            if (collected)
            {
                const QString value = MpvConfigFilter::optionValueForLine(line);
                if (base == QStringLiteral("input-conf"))
                {
                    collected->inputConf = resolveConfigPath(value);
                }
                else if (base == QStringLiteral("load-scripts"))
                {
                    collected->loadScripts = flagEnabled(line);
                }
                else if (name.endsWith(QStringLiteral("-clr")))
                {
                    collected->scripts.clear();
                }
                else
                {
                    const bool single = base == QStringLiteral("script") || name.endsWith(QStringLiteral("-append"))
                                        || name.endsWith(QStringLiteral("-remove")) || name.endsWith(QStringLiteral("-del"));
                    const QStringList items = single ? QStringList{value}
                                                     : value.split(QDir::listSeparator(), Qt::SkipEmptyParts);
                    if (name == QStringLiteral("scripts") || name.endsWith(QStringLiteral("-set")))
                        collected->scripts.clear();
                    for (const QString& item : items)
                    {
                        const QString path = resolveConfigPath(item);
                        if (name.endsWith(QStringLiteral("-remove")) || name.endsWith(QStringLiteral("-del")))
                            collected->scripts.removeAll(path);
                        else if (!path.isEmpty())
                            collected->scripts.append(path);
                    }
                }
            }
            output.append(QStringLiteral("# CineWindows applies %1 itself").arg(name));
            continue;
        }
        if (base == QStringLiteral("load-auto-profiles") && collected)
            collected->autoProfilesSet = true;

        if (MpvConfigFilter::isBlockedEmbeddedOption(name))
        {
            if (!m_ignoredOptions.contains(name))
                m_ignoredOptions.append(name);
            output.append(QStringLiteral("# CineWindows ignored embedded-unsafe option: %1").arg(name));
            continue;
        }

        if (line.contains(QStringLiteral("~~")))
        {
            bool hasValue = false;
            const QString value = MpvConfigFilter::optionValueForLine(line, &hasValue);
            if (hasValue)
            {
                const QString rawName = trimmed.left(trimmed.indexOf(QLatin1Char('='))).trimmed();
                output.append(rawName + QLatin1Char('=')
                              + MpvConfigFilter::fixedLengthValue(MpvConfigFilter::expandMetaPaths(value, m_configDir)));
                continue;
            }
        }
        output.append(line);
    }

    const QString fileName =
        depth == 0 ? QStringLiteral("mpv.conf") : QStringLiteral("include-%1.conf").arg(++m_includeCounter);
    const QString destinationPath = m_embeddedConfig->filePath(fileName);
    QSaveFile destination(destinationPath);
    if (!destination.open(QIODevice::WriteOnly) || destination.write(output.join(QLatin1Char('\n')).toUtf8()) < 0
        || !destination.commit())
    {
        qCWarning(cinePlayerLog).noquote()
            << "Could not write the embedded mpv configuration:" << destination.errorString();
        return {};
    }
    return destinationPath;
}

/**
 * @brief Merges script-opts/<script>.conf files and CineWindows defaults into "script-opts".
 * @details Values already present after mpv.conf was included win, matching mpv where the
 *          script-opts option overrides per-script config files.
 * @param defaults script-opts CineWindows set before mpv.conf (e.g. the bundled yt-dlp path).
 */
void MpvConfig::applyScriptOptions(const QVariantMap& defaults)
{
    QMap<QString, QString> fileOptions;
    const QDir directory(QDir(m_configDir).filePath(QStringLiteral("script-opts")));
    const QFileInfoList files = directory.entryInfoList({QStringLiteral("*.conf")}, QDir::Files, QDir::Name);
    for (const QFileInfo& file : files)
    {
        const QString script = file.completeBaseName();
        for (const QString& line : readConfigLines(file.absoluteFilePath()))
        {
            // Same rules as mpv's options.lua: '#' at column 0 is a comment, key=value otherwise.
            if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
                continue;
            const qsizetype equals = line.indexOf(QLatin1Char('='));
            if (equals <= 0)
                continue;
            fileOptions.insert(script + QLatin1Char('-') + line.left(equals),
                               MpvConfigFilter::expandMetaPaths(line.mid(equals + 1), m_configDir));
        }
    }

    const QVariantMap current = m_player->getProperty(QStringLiteral("script-opts")).toMap();
    const auto append = [this](const QString& key, const QString& value) {
        m_player->command({QStringLiteral("change-list"), QStringLiteral("script-opts"), QStringLiteral("append"),
                           key + QLatin1Char('=') + value});
    };
    for (auto it = defaults.cbegin(); it != defaults.cend(); ++it)
    {
        if (!current.contains(it.key()) && !fileOptions.contains(it.key()))
            append(it.key(), it.value().toString());
    }
    for (auto it = fileOptions.cbegin(); it != fileOptions.cend(); ++it)
    {
        if (current.contains(it.key()))
            continue;
        append(it.key(), it.value());
        m_fileScriptOptionKeys.insert(it.key());
    }
}

/** @brief Expands meta paths and resolves relative paths against the config folder. */
QString MpvConfig::resolveConfigPath(const QString& path) const
{
    QString expanded = MpvConfigFilter::expandMetaPaths(path.trimmed(), m_configDir);
    if (expanded.isEmpty())
        return {};
    if (expanded.startsWith(QStringLiteral("~/")) || expanded.startsWith(QStringLiteral("~\\")))
        expanded = QDir::homePath() + expanded.mid(1);
    expanded = QDir::fromNativeSeparators(expanded);
    if (QDir::isRelativePath(expanded))
        expanded = QDir(m_configDir).filePath(expanded);
    return QDir::cleanPath(expanded);
}

/**
 * @brief Parses input.conf and registers each default-section binding with "keybind".
 * @param path input.conf path (mpv.conf may redirect it with input-conf=).
 */
void MpvConfig::loadInputConf(const QString& path)
{
    QStringList bindings;
    for (const QString& raw : readConfigLines(path))
    {
        const QString line = raw.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;

        // First whitespace-delimited token is the key; the rest is the command.
        qsizetype split = -1;
        for (qsizetype i = 0; i < line.size(); ++i)
        {
            if (line.at(i).isSpace())
            {
                split = i;
                break;
            }
        }
        if (split < 0)
            continue;

        const QString key = line.left(split);
        QString cmd = line.mid(split + 1).trimmed();
        if (cmd.startsWith(QLatin1Char('{')))
        {
            // "key {section} command": only the default section maps to keybind.
            const qsizetype close = cmd.indexOf(QLatin1Char('}'));
            if (close < 0 || cmd.mid(1, close - 1).trimmed() != QStringLiteral("default"))
                continue;
            cmd = cmd.mid(close + 1).trimmed();
        }
        if (cmd.isEmpty() || cmd.startsWith(QLatin1Char('#'))
            || key.compare(QStringLiteral("default-bindings"), Qt::CaseInsensitive) == 0)
            continue;

        cmd = MpvConfigFilter::expandCommandMetaPaths(cmd, m_configDir);
        bindings.append(key + QLatin1Char(' ') + cmd);

        const QString canonical = normalizeKey(key, true);
        if (!canonical.isEmpty())
            m_overrides.insert(canonical, cmd);
    }
    m_player->command({QStringLiteral("define-section"), QStringLiteral("cinewindows-config"),
                       bindings.join(QLatin1Char('\n')), QStringLiteral("force")});
    m_player->command({QStringLiteral("enable-section"), QStringLiteral("cinewindows-config")});
}

/**
 * @brief Loads scripts/ (unless load-scripts=no) plus scripts named in mpv.conf.
 * @details Supports .lua and .js files, script folders with main.lua or main.js and
 *          C plugins. Scripts already running are skipped because libmpv cannot unload them.
 */
void MpvConfig::loadScripts(const LoaderOptions& options)
{
    QStringList candidates;
    if (options.loadScripts)
    {
        const QDir scriptsDir(QDir(m_configDir).filePath(QStringLiteral("scripts")));
        const QFileInfoList entries = scriptsDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                                                               QDir::Name | QDir::IgnoreCase);
        for (const QFileInfo& entry : entries)
        {
            if (entry.fileName().startsWith(QLatin1Char('.')))
                continue;
            if (entry.isDir())
            {
                const QDir package(entry.absoluteFilePath());
                if (package.exists(QStringLiteral("main.lua")) || package.exists(QStringLiteral("main.js")))
                    candidates.append(entry.absoluteFilePath());
            }
            else if (isScriptFile(entry))
            {
                candidates.append(entry.absoluteFilePath());
            }
        }
    }
    candidates.append(options.scripts);

    for (const QString& path : std::as_const(candidates))
        loadScript(path);
}

void MpvConfig::loadScript(const QString& path)
{
    const QFileInfo info(path);
    if (!info.exists())
    {
        qCWarning(cinePlayerLog).noquote() << "mpv script not found:" << QDir::toNativeSeparators(path);
        return;
    }
    QString key = QDir::cleanPath(info.absoluteFilePath());
    if (pathCaseSensitivity() == Qt::CaseInsensitive)
        key = key.toLower();
    if (m_loadedScriptPaths.contains(key))
        return;
    m_loadedScriptPaths.insert(key);
    m_player->command({QStringLiteral("load-script"), info.absoluteFilePath()});
    m_loadedScripts.append(info.fileName());
}

/**
 * @brief Canonicalises a key token so mpv (input.conf) and Qt (KeyBindings) spellings
 *        of the same chord compare equal.
 * @param tokenIn  Raw key token string (e.g. "Shift+L", "ctrl+f").
 * @param mpvStyle If true, treats a bare uppercase letter as Shift+letter (mpv convention).
 * @return The normalised key string in a canonical form.
 */
QString MpvConfig::normalizeKey(const QString& tokenIn, bool mpvStyle)
{
    QString token = tokenIn.trimmed();
    if (token.isEmpty())
    {
        return {};
    }

    // Modifier-flag accumulators.
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
    bool meta = false;

    // Peel off leading modifier prefixes (case-insensitive). A '+' at index 0
    // means '+' is itself the base key, so stop there.
    for (;;)
    {
        const qsizetype plus = token.indexOf(QLatin1Char('+'));
        if (plus <= 0)
        {
            break;
        }
        const QString mod = token.left(plus).toLower();
        if (mod == QStringLiteral("ctrl") || mod == QStringLiteral("control"))
        {
            ctrl = true;
        }
        else if (mod == QStringLiteral("alt") || mod == QStringLiteral("option"))
        {
            alt = true;
        }
        else if (mod == QStringLiteral("shift"))
        {
            shift = true;
        }
        else if (mod == QStringLiteral("meta") || mod == QStringLiteral("super") || mod == QStringLiteral("cmd")
                 || mod == QStringLiteral("command") || mod == QStringLiteral("win"))
        {
            meta = true;
        }
        else
        {
            break; // not a modifier; the remainder is the base key
        }
        token = token.mid(plus + 1);
    }

    // Normalise the base key part.
    QString base = token;
    if (base.size() == 1 && base.at(0).isLetter())
    {
        // mpv writes Shift+letter as an uppercase letter; Qt always uppercases
        // letters and states Shift explicitly.
        if (mpvStyle && base.at(0).isUpper())
        {
            shift = true;
        }
        base = base.toLower();
    }
    else
    {
        base = canonicalBase(base);
        // Like mpv, Shift is part of a printed symbol (e.g. "?"), not a separate modifier.
        if (base.size() == 1 && base.at(0).isPrint() && !base.at(0).isSpace())
            shift = false;
    }

    // Rebuild the key string in canonical modifier+base order.
    QString result;
    if (ctrl)
        result += QStringLiteral("ctrl+");
    if (alt)
        result += QStringLiteral("alt+");
    if (shift)
        result += QStringLiteral("shift+");
    if (meta)
        result += QStringLiteral("meta+");
    result += base;
    return result;
}
