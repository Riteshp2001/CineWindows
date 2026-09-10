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

#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

class CineMpvItem;

/**
 * @class MpvConfig
 * @brief Loads the user's standard mpv config folder into the embedded player.
 *
 * @details
 * libmpv defaults to config=no, so it never reads mpv.conf, input.conf or
 * auto-loads scripts. mpvqt also initializes mpv immediately and cannot be
 * patched (it is a fetched dependency), so the config folder is applied at
 * runtime instead:
 *  - mpv.conf  -> set as the "include" option (runtime-settable options apply)
 *  - input.conf-> each binding registered with the "keybind" command (top
 *                 priority for keys forwarded into libmpv)
 *  - scripts/  -> loaded with the "load-script" command
 *
 * It also exposes which app key sequences are also present in input.conf so the
 * Keyboard Shortcuts UI can show that collision.
 */
class MpvConfig : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(CineMpvItem* player READ player WRITE setPlayer NOTIFY playerChanged)
    // Bumped after each (re)load so QML bindings on isOverridden() re-evaluate.
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)

public:
    /**
     * @brief Constructs an MpvConfig loader with an optional QObject parent.
     * @param parent Optional parent QObject for Qt memory management.
     * @usecase Created by QML engine when declared in a .qml file, or manually in C++.
     * @sideeffects Does NOT load config yet - the caller must set a player and call reload().
     * @thread Must be called on the Qt main thread.
     */
    explicit MpvConfig(QObject* parent = nullptr);

    /**
     * @brief Returns the currently attached player instance.
     * @return Pointer to the CineMpvItem, or nullptr if none is set.
     * @usecase Used by apply() to send mpv commands, and by QML bindings.
     */
    CineMpvItem* player() const;

    /**
     * @brief Assigns the player instance that config commands will be applied to.
     * @param player Pointer to a CineMpvItem instance. Must not be null during reload().
     * @usecase Called from QML via the player property setter.
     * @sideeffects Emits playerChanged().
     */
    void setPlayer(CineMpvItem* player);

    /**
     * @brief Returns the current revision counter, bumped after each config (re)load.
     * @return Monotonically increasing integer revision number.
     * @usecase QML bindings on isOverridden() depend on this to re-evaluate when the config changes.
     */
    int revision() const;

    /**
     * @brief Re-reads the mpv config directory and re-applies all settings, bindings, and scripts.
     * @details Scans the user's mpv config folder for mpv.conf, input.conf, and scripts/,
     *          then pushes them into the running player via libmpv commands.
     * @usecase Called on startup and can be called again at runtime to pick up config file changes.
     * @sideeffects Clears any previously loaded overrides hash, increments m_revision,
     *              and may modify the player's mpv options, key bindings, and loaded scripts.
     * @thread Must be called on the Qt main thread.
     */
    Q_INVOKABLE void reload();

    /**
     * @brief Checks whether a given Qt key sequence is also bound in input.conf.
     * @param qtSequence A Qt key sequence string (e.g. "Space", "Ctrl+F").
     * @return true if the key has a mapping in the user's input.conf, false otherwise.
     * @usecase Called from the Keyboard Shortcuts settings UI to warn about binding collisions.
     * @thread Safe to call from any thread (read-only lookup into m_overrides).
     */
    Q_INVOKABLE bool isOverridden(const QString& qtSequence) const;

    /**
     * @brief Returns the mpv command associated with a Qt key sequence in input.conf.
     * @param qtSequence A Qt key sequence string (e.g. "Space", "Ctrl+F").
     * @return The mpv command string as written in input.conf, or an empty string if the key is not bound.
     * @usecase Used by the Keyboard Shortcuts settings UI to display the conflicting mpv command.
     * @thread Safe to call from any thread (read-only lookup into m_overrides).
     */
    Q_INVOKABLE QString commandFor(const QString& qtSequence) const;

Q_SIGNALS:
    /** @brief Emitted when the player property is changed via setPlayer(). */
    void playerChanged();

    /** @brief Emitted after a reload() completes, signalling that isOverridden() results may have changed. */
    void revisionChanged();

private:
    /**
     * @brief Applies the loaded config data to the player via libmpv commands.
     * @details Sends the "include" option for mpv.conf, registers each input.conf binding
     *          with the "keybind" command, and loads each script with the "load-script" command.
     * @usecase Called at the end of reload() after all config files have been parsed.
     * @sideeffects Modifies the player's runtime mpv configuration.
     */
    void apply();

    /**
     * @brief Parses the user's input.conf file and populates the m_overrides hash map.
     * @param dir Absolute path to the mpv config directory.
     * @usecase Called during reload() to discover custom key bindings.
     * @sideeffects Fills m_overrides with canonicalised key-to-command entries.
     */
    void loadInputConf(const QString& dir);

    /**
     * @brief Reads the user's mpv.conf and queues it to be loaded via the "include" option.
     * @param dir Absolute path to the mpv config directory.
     * @usecase Called during reload() to apply user-defined mpv options.
     */
    void loadMpvConf(const QString& dir);

    /**
     * @brief Loads all Lua/JS scripts found in the mpv scripts/ subdirectory.
     * @param dir Absolute path to the mpv config directory.
     * @usecase Called during reload() to register user scripts with the player.
     * @sideeffects Each script is loaded into the player via the "load-script" mpv command.
     */
    void loadScripts(const QString& dir);

    /**
     * @brief Canonicalises a key token so mpv (input.conf) and Qt (KeyBindings) spellings
     *        of the same chord compare equal.
     * @param token   Raw key token string (e.g. "Shift+L", "ctrl+f").
     * @param mpvStyle If true, treats a bare uppercase letter as Shift+letter (mpv convention).
     * @return The normalised key string in a canonical form.
     * @usecase Used internally when building the m_overrides lookup table and when
     *          checking overrides against Qt key sequences.
     */
    static QString normalizeKey(const QString& token, bool mpvStyle);

    CineMpvItem* m_player{nullptr};      /**< Attached player instance receiving config commands. */
    int m_revision{0};                   /**< Monotonically increasing revision counter, bumped each reload. */
    QHash<QString, QString> m_overrides; /**< Map of canonicalised key -> mpv command parsed from input.conf. */
};
