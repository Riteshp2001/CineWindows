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

#include <QObject>
#include <QSize>
#include <QString>
#include <QtQml/qqmlregistration.h>

class QJSEngine;
class QQmlEngine;

/**
 * @class SettingsManager
 * @brief QSettings-backed singleton that exposes persistent app preferences to QML.
 *
 * @details
 * SettingsManager owns startup/default preference values and writes user changes
 * through QSettings. It is registered as a QML singleton so views can bind to
 * preferences without passing an object through every component.
 *
 * Responsibilities:
 * - Load persisted settings during singleton construction
 * - Expose playback, subtitle, UI, update, language, and input preferences
 * - Persist setting changes immediately
 * - Provide helper actions for config directories and bundled scripts
 *
 * This class must not render UI or issue playback commands directly.
 */
class SettingsManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    /** @brief Initial window size in logical pixels. */
    Q_PROPERTY(QSize initialSize READ initialSize NOTIFY windowSettingsChanged)
    /** @brief Audio volume level (0 - MaxVolume). */
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    /** @brief Whether audio output is muted. */
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    /** @brief Whether media should open in a new window instead of the current one. */
    Q_PROPERTY(bool openNewWindows READ openNewWindows WRITE setOpenNewWindows NOTIFY openNewWindowsChanged)
    /** @brief Whether to apply volume normalisation (RG / EBU R128). */
    Q_PROPERTY(bool normalizeVolume READ normalizeVolume WRITE setNormalizeVolume NOTIFY normalizeVolumeChanged)
    /** @brief Hardware-accelerated decoding backend (e.g. "auto-safe", "vaapi", "cuda"). */
    Q_PROPERTY(QString hwdec READ hwdec WRITE setHwdec NOTIFY hwdecChanged)
    /** @brief Whether to remember the last playback position per file. */
    Q_PROPERTY(bool saveVideoPosition READ saveVideoPosition WRITE setSaveVideoPosition NOTIFY saveVideoPositionChanged)
    /** @brief Whether to restore the previous session on startup. */
    Q_PROPERTY(bool saveSession READ saveSession WRITE setSaveSession NOTIFY saveSessionChanged)
    /** @brief Whether to show thumbnail previews in the seek-bar / chapter list. */
    Q_PROPERTY(bool thumbnailPreview READ thumbnailPreview WRITE setThumbnailPreview NOTIFY thumbnailPreviewChanged)
    /** @brief Whether to display remaining time (HH:MM:-SS) instead of elapsed. */
    Q_PROPERTY(bool showRemaining READ showRemaining WRITE setShowRemaining NOTIFY showRemainingChanged)
    /** @brief Font family used for on-screen subtitles. */
    Q_PROPERTY(QString subtitleFont READ subtitleFont WRITE setSubtitleFont NOTIFY subtitleFontChanged)
    /** @brief Relative scale factor for subtitle text size. */
    Q_PROPERTY(double subtitleScale READ subtitleScale WRITE setSubtitleScale NOTIFY subtitleScaleChanged)
    /** @brief Colour of subtitle text (hex ARGB / named colour). */
    Q_PROPERTY(QString subtitleColor READ subtitleColor WRITE setSubtitleColor NOTIFY subtitleColorChanged)
    /** @brief Whether to draw a background box behind subtitles. */
    Q_PROPERTY(
        bool subtitleBackground READ subtitleBackground WRITE setSubtitleBackground NOTIFY subtitleBackgroundChanged)
    /** @brief Background colour behind subtitle text (hex ARGB). */
    Q_PROPERTY(QString subtitleBackgroundColor READ subtitleBackgroundColor WRITE setSubtitleBackgroundColor NOTIFY
                   subtitleBackgroundColorChanged)
    /** @brief Comma-separated list of preferred subtitle language codes. */
    Q_PROPERTY(
        QString subtitleLanguages READ subtitleLanguages WRITE setSubtitleLanguages NOTIFY subtitleLanguagesChanged)
    /** @brief Comma-separated list of preferred audio language codes. */
    Q_PROPERTY(QString audioLanguages READ audioLanguages WRITE setAudioLanguages NOTIFY audioLanguagesChanged)
    /** @brief Whether to automatically check for and apply application updates. */
    Q_PROPERTY(bool autoUpdate READ autoUpdate WRITE setAutoUpdate NOTIFY autoUpdateChanged)
    /** @brief Action ID for single left-click on the video area. */
    Q_PROPERTY(int leftClick READ leftClick WRITE setLeftClick NOTIFY leftClickChanged)
    /** @brief Action ID for single right-click on the video area. */
    Q_PROPERTY(int rightClick READ rightClick WRITE setRightClick NOTIFY rightClickChanged)
    /** @brief Application locale code (e.g. "en-US", "de-DE"). */
    Q_PROPERTY(QString locale READ locale WRITE setLocale NOTIFY localeChanged)
    /** @brief Serialised keyboard shortcut bindings. */
    Q_PROPERTY(QString keyBindings READ keyBindings WRITE setKeyBindings NOTIFY keyBindingsChanged)
    /** @brief Active application palette: dark or light. */
    Q_PROPERTY(QString themeMode READ themeMode WRITE setThemeMode NOTIFY themeModeChanged)
    /** @brief User-selected accent color shared by every application surface. */
    Q_PROPERTY(QString accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    /** @brief Whether non-essential interface motion should be disabled. */
    Q_PROPERTY(bool reduceMotion READ reduceMotion WRITE setReduceMotion NOTIFY reduceMotionChanged)
    /** @brief Whether supported platforms should use the native system backdrop. */
    Q_PROPERTY(bool systemBackdrop READ systemBackdrop WRITE setSystemBackdrop NOTIFY systemBackdropChanged)

public:
    /**
     * @brief Constructs the SettingsManager singleton.
     * @param parent Optional QObject parent.
     */
    explicit SettingsManager(QObject* parent = nullptr);

    /**
     * @brief Creates the singleton instance used by the QML engine.
     *
     * @usecase Called by Qt's QML type system when the SettingsManager singleton
     * is first referenced.
     *
     * @return Shared SettingsManager instance for the active engine.
     *
     * @sideeffects Allocates and initializes settings state on first use.
     * @thread Must be called on the main Qt/QML thread.
     */
    static SettingsManager* create(QQmlEngine*, QJSEngine*);

    /** @brief Returns the initial window size. */
    QSize initialSize() const;
    /** @brief Returns the current audio volume level (0 - MaxVolume). */
    int volume() const;
    /** @brief Returns whether audio is muted. */
    bool muted() const;
    /** @brief Returns whether new media opens in a new window. */
    bool openNewWindows() const;
    /** @brief Returns whether volume normalisation is enabled. */
    bool normalizeVolume() const;
    /** @brief Returns the hardware decoding backend identifier. */
    QString hwdec() const;
    /** @brief Returns whether video playback position is being saved. */
    bool saveVideoPosition() const;
    /** @brief Returns whether session state is being persisted. */
    bool saveSession() const;
    /** @brief Returns whether thumbnail previews are visible. */
    bool thumbnailPreview() const;
    /** @brief Returns whether remaining (instead of elapsed) time is shown. */
    bool showRemaining() const;
    /** @brief Returns the subtitle font family name. */
    QString subtitleFont() const;
    /** @brief Returns the subtitle text scale factor. */
    double subtitleScale() const;
    /** @brief Returns the subtitle text colour. */
    QString subtitleColor() const;
    /** @brief Returns whether subtitle background rendering is enabled. */
    bool subtitleBackground() const;
    /** @brief Returns the subtitle background colour. */
    QString subtitleBackgroundColor() const;
    /** @brief Returns the comma-separated preferred subtitle language codes. */
    QString subtitleLanguages() const;
    /** @brief Returns the comma-separated preferred audio language codes. */
    QString audioLanguages() const;
    /** @brief Returns whether automatic updates are enabled. */
    bool autoUpdate() const;
    /** @brief Returns the action ID bound to left-click. */
    int leftClick() const;
    /** @brief Returns the action ID bound to right-click. */
    int rightClick() const;
    /** @brief Returns the application locale code. */
    QString locale() const;
    /** @brief Returns the serialised keyboard shortcut configuration. */
    QString keyBindings() const;
    /** @brief Returns the active application palette: dark or light. */
    QString themeMode() const;
    /** @brief Returns the user-selected accent colour. */
    QString accentColor() const;
    /** @brief Returns whether non-essential interface motion should be disabled. */
    bool reduceMotion() const;
    /** @brief Returns whether the native system backdrop is enabled. */
    bool systemBackdrop() const;

    /** @brief Sets the audio volume level. @param value Volume level (0 - MaxVolume). */
    void setVolume(int value);
    /** @brief Sets whether audio is muted. @param value True to mute, false to unmute. */
    void setMuted(bool value);
    /** @brief Sets whether new media opens in a new window. @param value True to open in new window. */
    void setOpenNewWindows(bool value);
    /** @brief Sets whether volume normalisation is enabled. @param value True to enable normalisation. */
    void setNormalizeVolume(bool value);
    /** @brief Sets the hardware decoding backend. @param value Backend identifier (e.g. "auto-safe"). */
    void setHwdec(const QString& value);
    /** @brief Sets whether video playback position is saved. @param value True to save position. */
    void setSaveVideoPosition(bool value);
    /** @brief Sets whether session state is persisted. @param value True to save session. */
    void setSaveSession(bool value);
    /** @brief Sets whether thumbnail previews are shown. @param value True to show thumbnails. */
    void setThumbnailPreview(bool value);
    /** @brief Sets whether remaining (instead of elapsed) time is displayed. @param value True to show remaining. */
    void setShowRemaining(bool value);
    /** @brief Sets the subtitle font family. @param value Font family name. */
    void setSubtitleFont(const QString& value);
    /** @brief Sets the subtitle text scale factor. @param value Scale factor (1.0 = default). */
    void setSubtitleScale(double value);
    /** @brief Sets the subtitle text colour. @param value Colour in hex ARGB or named format. */
    void setSubtitleColor(const QString& value);
    /** @brief Sets whether subtitle background rendering is enabled. @param value True to enable background. */
    void setSubtitleBackground(bool value);
    /** @brief Sets the subtitle background colour. @param value Colour in hex ARGB format. */
    void setSubtitleBackgroundColor(const QString& value);
    /** @brief Sets the preferred subtitle language codes. @param value Comma-separated language codes. */
    void setSubtitleLanguages(const QString& value);
    /** @brief Sets the preferred audio language codes. @param value Comma-separated language codes. */
    void setAudioLanguages(const QString& value);
    /** @brief Sets whether automatic updates are enabled. @param value True to enable auto-updates. */
    void setAutoUpdate(bool value);
    /** @brief Sets the action ID for left-click. @param value Action identifier. */
    void setLeftClick(int value);
    /** @brief Sets the action ID for right-click. @param value Action identifier. */
    void setRightClick(int value);
    /** @brief Sets the application locale code. @param value Locale string (e.g. "en-US"). */
    void setLocale(const QString& value);
    /** @brief Sets the serialised keyboard shortcut configuration. @param value Key-bindings string. */
    void setKeyBindings(const QString& value);
    /** @brief Sets the application palette. @param value "dark" or "light". */
    void setThemeMode(const QString& value);
    /** @brief Sets the accent colour. @param value Colour in hex ARGB or named format. */
    void setAccentColor(const QString& value);
    /** @brief Sets whether non-essential motion is disabled. @param value True to reduce motion. */
    void setReduceMotion(bool value);
    /** @brief Enables or disables the native system backdrop. @param value True to enable it. */
    void setSystemBackdrop(bool value);

    /**
     * @brief Restores playback-related preferences to default values.
     *
     * @usecase Used by preferences UI when the user wants to reset playback options.
     *
     * @sideeffects Updates QSettings-backed values and emits change notifications.
     * @thread Must be called on the main Qt/QML thread.
     */
    Q_INVOKABLE void resetPlaybackSettings();

    /**
     * @brief Persists the last application window size.
     *
     * @usecase Called during window shutdown so the next launch can restore size.
     *
     * @param width Window width in logical pixels.
     * @param height Window height in logical pixels.
     *
     * @sideeffects Writes width and height to persistent settings.
     * @thread Must be called on the main Qt/QML thread.
     */
    Q_INVOKABLE void saveWindowSize(int width, int height);

    /**
     * @brief Opens the application configuration directory in the platform file manager.
     *
     * @usecase Used by preferences/support flows for advanced manual config edits.
     *
     * @sideeffects Creates the config directory when needed and launches an external handler.
     * @thread Must be called on the main Qt/QML thread.
     */
    Q_INVOKABLE void openConfigDirectory();

    /**
     * @brief Launches a new instance of CineWindows in a separate process.
     *
     * @usecase Triggered by Ctrl+N / "New Window" menu action.
     *
     * @sideeffects Starts a new OS process with the same executable path.
     * @thread Must be called on the main Qt/QML thread.
     */
    Q_INVOKABLE void launchNewWindow();

Q_SIGNALS:
    /** @brief Emitted when the initial window size changes. */
    void windowSettingsChanged();
    /** @brief Emitted when the audio volume changes. */
    void volumeChanged();
    /** @brief Emitted when the muted state changes. */
    void mutedChanged();
    /** @brief Emitted when the open-new-window preference changes. */
    void openNewWindowsChanged();
    /** @brief Emitted when the volume normalisation preference changes. */
    void normalizeVolumeChanged();
    /** @brief Emitted when the hardware decoding backend changes. */
    void hwdecChanged();
    /** @brief Emitted when the save-video-position preference changes. */
    void saveVideoPositionChanged();
    /** @brief Emitted when the save-session preference changes. */
    void saveSessionChanged();
    /** @brief Emitted when the thumbnail preview preference changes. */
    void thumbnailPreviewChanged();
    /** @brief Emitted when the show-remaining-time preference changes. */
    void showRemainingChanged();
    /** @brief Emitted when the subtitle font changes. */
    void subtitleFontChanged();
    /** @brief Emitted when the subtitle scale factor changes. */
    void subtitleScaleChanged();
    /** @brief Emitted when the subtitle text colour changes. */
    void subtitleColorChanged();
    /** @brief Emitted when the subtitle background toggle changes. */
    void subtitleBackgroundChanged();
    /** @brief Emitted when the subtitle background colour changes. */
    void subtitleBackgroundColorChanged();
    /** @brief Emitted when the preferred subtitle languages change. */
    void subtitleLanguagesChanged();
    /** @brief Emitted when the preferred audio languages change. */
    void audioLanguagesChanged();
    /** @brief Emitted when the auto-update preference changes. */
    void autoUpdateChanged();
    /** @brief Emitted when the left-click action binding changes. */
    void leftClickChanged();
    /** @brief Emitted when the right-click action binding changes. */
    void rightClickChanged();
    /** @brief Emitted when the locale preference changes. */
    void localeChanged();
    /** @brief Emitted when the keyboard shortcut configuration changes. */
    void keyBindingsChanged();
    /** @brief Emitted when the theme mode changes. */
    void themeModeChanged();
    /** @brief Emitted when the accent colour changes. */
    void accentColorChanged();
    /** @brief Emitted when the reduce-motion preference changes. */
    void reduceMotionChanged();
    /** @brief Emitted when the native system-backdrop preference changes. */
    void systemBackdropChanged();

private:
    /** @brief Loads all settings from persistent storage into member variables. */
    void load();
    /** @brief Writes a single setting key/value to QSettings. @param key Setting key. @param value Setting value. */
    void saveValue(const QString& key, const QVariant& value);
    /** @brief Removes legacy thumbnail script entries that were migrated to a new format. */
    void removeLegacyThumbnailScripts();

    static constexpr int MaxVolume = 200;        ///< Maximum allowable volume level.
    static constexpr int MaxClickAction = 3;     ///< Number of valid click-action identifiers.

    QSize m_initialSize;                         ///< Saved window size; invalid on first launch.
    int m_volume{100};                           ///< Current audio volume level.
    bool m_muted{false};                         ///< Whether audio output is muted.
    bool m_openNewWindows{true};                 ///< Whether to open media in a new window.
    bool m_normalizeVolume{false};               ///< Whether volume normalisation is active.
    QString m_hwdec{QStringLiteral("auto-safe")}; ///< Hardware decoding backend identifier.
    bool m_saveVideoPosition{false};             ///< Whether to remember video playback position.
    bool m_saveSession{false};                   ///< Whether to restore session on startup.
    bool m_thumbnailPreview{true};               ///< Whether to show thumbnail previews.
    bool m_showRemaining{false};                 ///< Whether to show remaining instead of elapsed time.
    QString m_subtitleFont{QStringLiteral("Adwaita Sans SemiBold")}; ///< Subtitle font family name.
    double m_subtitleScale{1.0};                 ///< Subtitle text scale factor.
    QString m_subtitleColor{QStringLiteral("#ebebeb")}; ///< Subtitle text colour.
    bool m_subtitleBackground{false};            ///< Whether subtitle background is enabled.
    QString m_subtitleBackgroundColor{QStringLiteral("#97000000")}; ///< Subtitle background colour.
    QString m_subtitleLanguages;                 ///< Preferred subtitle language codes.
    QString m_audioLanguages;                    ///< Preferred audio language codes.
    bool m_autoUpdate{true};                     ///< Whether automatic updates are enabled.
    QString m_locale;                            ///< Application locale code.
    int m_leftClick{0};                          ///< Action ID for left-click.
    int m_rightClick{1};                         ///< Action ID for right-click.
    QString m_keyBindings;                       ///< Serialised keyboard shortcut configuration.
    QString m_themeMode{QStringLiteral("dark")};        ///< Active theme mode: dark or light.
    QString m_accentColor{QStringLiteral("#10c7d1")};   ///< Application accent colour.
    bool m_reduceMotion{false};                         ///< Whether reduced motion is preferred.
    bool m_systemBackdrop{true};                        ///< Whether native backdrop materials are enabled.
};
