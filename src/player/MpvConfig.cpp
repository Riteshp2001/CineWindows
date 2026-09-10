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

#include "player/CineMpvItem.h"
#include "player/MpvConfigFilter.h"
#include "utils/PathUtils.h"

#include <QChar>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QSaveFile>
#include <QTextStream>

namespace {

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

/**
 * @brief Constructs an MpvConfig loader instance.
 * @param parent Optional parent QObject for Qt memory management.
 * @note Does NOT load config — caller must set a player and call reload().
 */
MpvConfig::MpvConfig(QObject* parent)
    : QObject(parent)
{}

/**
 * @brief Returns the currently attached player instance.
 * @return Pointer to the CineMpvItem, or nullptr if none is set.
 */
CineMpvItem* MpvConfig::player() const
{
    return m_player;
}

/**
 * @brief Assigns the player instance that config commands will be applied to.
 * @param player Pointer to a CineMpvItem instance.
 * @note If the new player differs from the current one, the config is re-applied automatically.
 */
void MpvConfig::setPlayer(CineMpvItem* player)
{
    // No-op if the same instance is already attached.
    if (m_player == player)
    {
        return;
    }
    m_player = player;
    Q_EMIT playerChanged();
    // Re-apply config whenever a (non-null) player is attached.
    if (m_player)
    {
        apply();
    }
}

/**
 * @brief Returns the current revision counter, bumped after each config (re)load.
 * @return Monotonically increasing integer revision number.
 */
int MpvConfig::revision() const
{
    return m_revision;
}

/**
 * @brief Re-reads the mpv config directory and re-applies all settings, bindings, and scripts.
 * @note Currently delegates directly to apply(); may be extended to re-scan the filesystem.
 */
void MpvConfig::reload()
{
    apply();
}

/**
 * @brief Checks whether a given Qt key sequence is also bound in input.conf.
 * @param qtSequence A Qt key sequence string (e.g. "Space", "Ctrl+F").
 * @return true if the key has a mapping in the user's input.conf, false otherwise.
 */
bool MpvConfig::isOverridden(const QString& qtSequence) const
{
    if (qtSequence.isEmpty())
    {
        return false;
    }
    return m_overrides.contains(normalizeKey(qtSequence, false));
}

/**
 * @brief Returns the mpv command associated with a Qt key sequence in input.conf.
 * @param qtSequence A Qt key sequence string (e.g. "Space", "Ctrl+F").
 * @return The mpv command string, or an empty QString if the key is not bound.
 */
QString MpvConfig::commandFor(const QString& qtSequence) const
{
    return m_overrides.value(normalizeKey(qtSequence, false));
}

/**
 * @brief Applies the loaded config data to the player via libmpv commands.
 * @details Clears the overrides hash, then reads and applies mpv.conf, input.conf,
 *          and scripts/ from the user's mpv config directory through the attached player.
 * @note Called on startup (via setPlayer/reload) and whenever config should be refreshed.
 */
void MpvConfig::apply()
{
    // Discard any previously loaded key-override state.
    m_overrides.clear();

    // Apply config only when a player instance is attached.
    if (m_player)
    {
        const QString directory = PathUtils::mpvUserConfigDir();
        loadMpvConf(directory);
        loadInputConf(directory);
        loadScripts(directory);
    }

    // Bump revision so QML bindings on isOverridden() re-evaluate.
    ++m_revision;
    Q_EMIT revisionChanged();
}

/**
 * @brief Reads the user's mpv.conf and queues it to be loaded via the "include" option.
 * @param dir Absolute path to the mpv config directory.
 * @note The raw config is sanitised before it is written to a temporary file and included,
 *       because standalone mpv options (vo/gpu/wid/fullscreen/hwdec) break MpvQt embedding.
 */
void MpvConfig::loadMpvConf(const QString& dir)
{
    // Build the path to the user's mpv.conf.
    const QString path = QDir(dir).filePath(QStringLiteral("mpv.conf"));
    QFile source(path);
    if (!source.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

    // Strip options that are unsafe for the embedded player.
    const QString filtered = MpvConfigFilter::sanitizeMpvConf(QString::fromUtf8(source.readAll()));
    const QString filteredPath = QDir(PathUtils::mpvConfigDir()).filePath(QStringLiteral("embedded-mpv.conf"));
    QSaveFile destination(filteredPath);
    if (!destination.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return;
    }

    destination.write(filtered.toUtf8());
    if (!destination.commit())
    {
        return;
    }

    // Point the running player at the sanitised config file via the "include" option.
    m_player->setMpvOption(QStringLiteral("include"), filteredPath);
}

/**
 * @brief Parses the user's input.conf file and registers each binding with the player.
 * @param dir Absolute path to the mpv config directory.
 * @details Each key-binding line is split on the first whitespace: the left side is the key,
 *          the right side is the mpv command. The binding is registered with the "keybind"
 *          command at top priority and recorded in m_overrides for collision detection.
 */
void MpvConfig::loadInputConf(const QString& dir)
{
    // Open the user's input.conf.
    const QString path = QDir(dir).filePath(QStringLiteral("input.conf"));
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd())
    {
        const QString line = in.readLine().trimmed();
        // Skip blank lines and comments.
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
        {
            continue;
        }

        // First whitespace-delimited token is the key; the rest is the command.
        int split = -1;
        for (int i = 0; i < line.size(); ++i)
        {
            if (line.at(i).isSpace())
            {
                split = i;
                break;
            }
        }
        if (split < 0)
        {
            continue; // key with no command
        }

        const QString key = line.left(split);
        const QString cmd = line.mid(split + 1).trimmed();
        if (cmd.isEmpty())
        {
            continue;
        }

        // Register the binding at top priority so forwarded input.conf keys
        // beat mpv's built-in defaults inside libmpv.
        m_player->runCommand({QStringLiteral("keybind"), key, cmd});

        // Record the canonical key → command mapping for the overrides lookup.
        const QString canonical = normalizeKey(key, true);
        if (!canonical.isEmpty())
        {
            m_overrides.insert(canonical, cmd);
        }
    }
}

/**
 * @brief Loads all Lua/JS scripts found in the mpv scripts/ subdirectory.
 * @param dir Absolute path to the mpv config directory.
 * @details Supports individual .lua/.js/.mjs files and script packages (directories
 *          containing a main.lua entry point). Each script is registered with the
 *          player via the "load-script" mpv command.
 */
void MpvConfig::loadScripts(const QString& dir)
{
    // Resolve the scripts subdirectory inside the mpv config folder.
    const QDir scriptsDir(QDir(dir).filePath(QStringLiteral("scripts")));
    if (!scriptsDir.exists())
    {
        return;
    }

    const QFileInfoList entries = scriptsDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& entry : entries)
    {
        if (entry.isDir())
        {
            // mpv script package: a directory with a main.lua entry point.
            if (QFileInfo::exists(QDir(entry.absoluteFilePath()).filePath(QStringLiteral("main.lua"))))
            {
                m_player->runCommand({QStringLiteral("load-script"), entry.absoluteFilePath()});
            }
            continue;
        }
        // Load individual scripts with recognised extensions.
        const QString suffix = entry.suffix().toLower();
        if (suffix == QStringLiteral("lua") || suffix == QStringLiteral("js") || suffix == QStringLiteral("mjs"))
        {
            m_player->runCommand({QStringLiteral("load-script"), entry.absoluteFilePath()});
        }
    }
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
