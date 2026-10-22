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
#include <QPointer>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <memory>

class CineMpvItem;

/**
 * @class MpvConfig
 * @brief Loads the CineWindows-specific mpv config folder into the embedded player.
 *
 * @details
 * The folder (see PathUtils::mpvConfigDir(), e.g. %APPDATA%/CineWindows/mpv) is
 * separate from the standalone mpv configuration. libmpv defaults to config=no
 * and MpvQt initializes mpv before an embedder can change that, so the folder is
 * applied at runtime in the same order mpv uses:
 *  - mpv.conf      -> sanitized copy loaded with the "include" option (profiles,
 *                     conditional profiles, nested include= and ~~/ paths work)
 *  - script-opts/  -> each <script>.conf merged into the "script-opts" option
 *  - input.conf    -> each binding registered with the "keybind" command
 *  - scripts/      -> Lua/JavaScript files, script folders and C plugins loaded
 *                     with the "load-script" command
 *
 * Keys bound in input.conf take priority over the CineWindows shortcuts; QML asks
 * isMpvKeyBound() before dispatching an app action.
 */
class MpvConfig : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(CineMpvItem* player READ player WRITE setPlayer NOTIFY playerChanged)
    // Bumped after each (re)load so QML bindings on isOverridden() re-evaluate.
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
    Q_PROPERTY(QString configDirectory READ configDirectory CONSTANT)
    Q_PROPERTY(QString standardConfigDirectory READ standardConfigDirectory CONSTANT)
    Q_PROPERTY(bool standardConfigAvailable READ standardConfigAvailable NOTIFY revisionChanged)
    Q_PROPERTY(QStringList loadedScripts READ loadedScripts NOTIFY revisionChanged)
    Q_PROPERTY(QStringList ignoredOptions READ ignoredOptions NOTIFY revisionChanged)
    Q_PROPERTY(int bindingCount READ bindingCount NOTIFY revisionChanged)

public:
    explicit MpvConfig(QObject* parent = nullptr);

    CineMpvItem* player() const;

    /**
     * @brief Assigns the player and applies the config folder to it.
     * @param player The embedded player; assign it after app preferences were applied so
     *               mpv.conf values win at startup.
     */
    void setPlayer(CineMpvItem* player);

    int revision() const;
    QString configDirectory() const;
    QString standardConfigDirectory() const;
    bool standardConfigAvailable() const;
    QStringList loadedScripts() const;
    QStringList ignoredOptions() const;
    int bindingCount() const;

    /**
     * @brief Re-reads the config folder and re-applies options, bindings and new scripts.
     * @note Scripts that are already running cannot be unloaded by libmpv; they stay
     *       loaded until the application restarts.
     */
    Q_INVOKABLE void reload();

    /**
     * @brief Checks whether a KeyBindings-style sequence (e.g. "Ctrl+F") is bound in input.conf.
     * @param qtSequence A Qt key sequence string.
     */
    Q_INVOKABLE bool isOverridden(const QString& qtSequence) const;

    /**
     * @brief Checks whether an mpv key name (e.g. "Ctrl+f", "WHEEL_UP") is bound in input.conf.
     * @param mpvKey Key name in mpv spelling, as produced by InputRouter::mpvKeyName().
     */
    Q_INVOKABLE bool isMpvKeyBound(const QString& mpvKey) const;

    /**
     * @brief Returns the input.conf command bound to a KeyBindings-style sequence.
     * @param qtSequence A Qt key sequence string.
     * @return The command, or an empty string if the key is not bound.
     */
    Q_INVOKABLE QString commandFor(const QString& qtSequence) const;

    /**
     * @brief Opens mpv.conf or input.conf in the user's editor, creating a commented template first.
     * @param fileName Either "mpv.conf" or "input.conf".
     * @return True if an editor could be launched.
     */
    Q_INVOKABLE bool openConfigFile(const QString& fileName);

    /**
     * @brief Copies the standalone mpv configuration into the CineWindows folder and reloads.
     * @details Existing CineWindows files are kept; untouched templates are replaced.
     *          watch_later and cache folders are skipped.
     * @return Number of copied files, or -1 when no standalone mpv configuration exists.
     */
    Q_INVOKABLE int importStandardConfig();

Q_SIGNALS:
    void playerChanged();
    void revisionChanged();

private:
    /** @brief Options collected from the top level of mpv.conf that libmpv cannot apply at runtime. */
    struct LoaderOptions
    {
        QString inputConf;
        QStringList scripts;
        bool loadScripts{true};
        bool autoProfilesSet{false};
    };

    void apply();
    QString writeEmbeddedConfig(const QString& sourcePath, int depth, LoaderOptions* options);
    void applyScriptOptions(const QVariantMap& defaults);
    void loadInputConf(const QString& path);
    void loadScripts(const LoaderOptions& options);
    void loadScript(const QString& path);
    QString resolveConfigPath(const QString& path) const;

    /**
     * @brief Canonicalises a key token so mpv (input.conf) and Qt (KeyBindings) spellings
     *        of the same chord compare equal.
     * @param token    Raw key token string (e.g. "Shift+L", "ctrl+f").
     * @param mpvStyle If true, treats a bare uppercase letter as Shift+letter (mpv convention).
     * @return The normalised key string in a canonical form.
     */
    static QString normalizeKey(const QString& token, bool mpvStyle);

    QPointer<CineMpvItem> m_player;
    QString m_configDir;
    std::unique_ptr<QTemporaryDir> m_embeddedConfig;
    int m_includeCounter{0};
    int m_revision{0};
    bool m_appScriptOptionsCaptured{false};
    bool m_autoProfilesLoaded{false};
    QHash<QString, QString> m_overrides; /**< Canonicalised key -> mpv command parsed from input.conf. */
    QVariantMap m_appScriptOptions;      /**< script-opts set by CineWindows before mpv.conf was applied. */
    QSet<QString> m_fileScriptOptionKeys; /**< script-opts keys added from script-opts/<name>.conf files. */
    QSet<QString> m_loadedScriptPaths;
    QStringList m_loadedScripts;
    QStringList m_ignoredOptions;
};
