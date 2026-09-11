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

#include "app/SettingsManager.h"

#include "utils/PathUtils.h"

#include <QColor>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QJSEngine>
#include <QProcess>
#include <QSettings>
#include <QUrl>
#include <QVariant>
#include <QtGlobal>

#include <algorithm>

/**
 * @brief Constructs the SettingsManager and loads persisted preferences.
 * @param parent Optional QObject parent.
 *
 * On Windows the default hwdec backend is overridden to "no" to avoid
 * driver compatibility issues with D3D11VA.
 */
SettingsManager::SettingsManager(QObject* parent)
    : QObject(parent)
{
#ifdef Q_OS_WIN
    // Default to software decoding on Windows for broad compatibility
    m_hwdec = QStringLiteral("no");
#endif
    load();
}

/**
 * @brief Creates and returns the singleton SettingsManager instance.
 * @param engine   QML engine (unused; retained for QML type-system registration).
 * @param jsEngine QJS engine (unused; retained for QML type-system registration).
 * @return The shared SettingsManager singleton.
 *
 * Ownership is set to CppOwnership so Qt does not garbage-collect the instance.
 */
SettingsManager* SettingsManager::create(QQmlEngine*, QJSEngine*)
{
    static SettingsManager instance;
    QJSEngine::setObjectOwnership(&instance, QJSEngine::CppOwnership);
    return &instance;
}

/**
 * @brief Loads all persisted settings from QSettings into member variables.
 *
 * Reads every preference group, clamps values where necessary, and
 * migrates legacy or invalid entries.
 */
void SettingsManager::load()
{
    QSettings settings;
    m_initialSize = settings.value(QStringLiteral("window/size"), m_initialSize).toSize();
    // Clamp volume to valid range [0, MaxVolume]
    m_volume = std::clamp(settings.value(QStringLiteral("playback/volume"), m_volume).toInt(), 0, MaxVolume);
    m_muted = settings.value(QStringLiteral("playback/muted"), m_muted).toBool();
    m_openNewWindows = settings.value(QStringLiteral("behavior/openNewWindows"), m_openNewWindows).toBool();
    m_normalizeVolume = settings.value(QStringLiteral("playback/normalizeVolume"), m_normalizeVolume).toBool();
    m_hwdec = settings.value(QStringLiteral("playback/hwdec"), m_hwdec).toString();
    // Migrate legacy d3d11va-copy backend to "no"
    if (m_hwdec == QStringLiteral("d3d11va-copy"))
    {
        m_hwdec = QStringLiteral("no");
        settings.setValue(QStringLiteral("playback/hwdec"), m_hwdec);
    }
    m_saveVideoPosition = settings.value(QStringLiteral("session/saveVideoPosition"), m_saveVideoPosition).toBool();
    m_saveSession = settings.value(QStringLiteral("session/saveSession"), m_saveSession).toBool();
    m_thumbnailPreview = settings.value(QStringLiteral("ui/thumbnailPreview"), m_thumbnailPreview).toBool();
    m_showRemaining = settings.value(QStringLiteral("ui/showRemaining"), m_showRemaining).toBool();
    m_subtitleFont = settings.value(QStringLiteral("subtitles/font"), m_subtitleFont).toString();
    m_subtitleScale = settings.value(QStringLiteral("subtitles/scale"), m_subtitleScale).toDouble();
    m_subtitleColor = settings.value(QStringLiteral("subtitles/color"), m_subtitleColor).toString();
    m_subtitleBackground = settings.value(QStringLiteral("subtitles/background"), m_subtitleBackground).toBool();
    m_subtitleBackgroundColor =
        settings.value(QStringLiteral("subtitles/backgroundColor"), m_subtitleBackgroundColor).toString();
    m_subtitleLanguages = settings.value(QStringLiteral("subtitles/languages"), m_subtitleLanguages).toString();
    m_audioLanguages = settings.value(QStringLiteral("audio/languages"), m_audioLanguages).toString();
    m_autoUpdate = settings.value(QStringLiteral("updates/autoUpdate"), m_autoUpdate).toBool();
    m_leftClick = settings.value(QStringLiteral("behavior/leftClick"), m_leftClick).toInt();
    m_rightClick = settings.value(QStringLiteral("behavior/rightClick"), m_rightClick).toInt();
    m_locale = settings.value(QStringLiteral("ui/locale"), m_locale).toString();
    m_keyBindings = settings.value(QStringLiteral("input/keyBindings"), m_keyBindings).toString();
    m_themeMode = settings.value(QStringLiteral("ui/themeMode"), m_themeMode).toString();
    // Fall back to dark if stored value is invalid
    if (m_themeMode != QStringLiteral("dark") && m_themeMode != QStringLiteral("light"))
        m_themeMode = QStringLiteral("dark");
    m_accentColor = settings.value(QStringLiteral("ui/accentColor"), m_accentColor).toString();
    // Fall back to default cyan if stored colour is invalid
    if (!QColor::isValidColorName(m_accentColor))
        m_accentColor = QStringLiteral("#10c7d1");
    m_reduceMotion = settings.value(QStringLiteral("ui/reduceMotion"), m_reduceMotion).toBool();
    m_systemBackdrop = settings.value(QStringLiteral("ui/systemBackdrop"), m_systemBackdrop).toBool();

    // Purge thumbnail scripts from the legacy mpv config location
    removeLegacyThumbnailScripts();
}

void SettingsManager::saveValue(const QString& key, const QVariant& value)
{
    QSettings settings;
    settings.setValue(key, value);
}

QSize SettingsManager::initialSize() const
{
    return m_initialSize;
}
int SettingsManager::volume() const
{
    return m_volume;
}
bool SettingsManager::muted() const
{
    return m_muted;
}
bool SettingsManager::openNewWindows() const
{
    return m_openNewWindows;
}
bool SettingsManager::normalizeVolume() const
{
    return m_normalizeVolume;
}
QString SettingsManager::hwdec() const
{
    return m_hwdec;
}
bool SettingsManager::saveVideoPosition() const
{
    return m_saveVideoPosition;
}
bool SettingsManager::saveSession() const
{
    return m_saveSession;
}
bool SettingsManager::thumbnailPreview() const
{
    return m_thumbnailPreview;
}
bool SettingsManager::showRemaining() const
{
    return m_showRemaining;
}
QString SettingsManager::subtitleFont() const
{
    return m_subtitleFont;
}
double SettingsManager::subtitleScale() const
{
    return m_subtitleScale;
}
QString SettingsManager::subtitleColor() const
{
    return m_subtitleColor;
}
bool SettingsManager::subtitleBackground() const
{
    return m_subtitleBackground;
}
QString SettingsManager::subtitleBackgroundColor() const
{
    return m_subtitleBackgroundColor;
}
QString SettingsManager::subtitleLanguages() const
{
    return m_subtitleLanguages;
}
QString SettingsManager::audioLanguages() const
{
    return m_audioLanguages;
}
bool SettingsManager::autoUpdate() const
{
    return m_autoUpdate;
}

void SettingsManager::setVolume(int value)
{
    value = std::clamp(value, 0, MaxVolume);
    if (m_volume == value)
    {
        return;
    }
    m_volume = value;
    saveValue(QStringLiteral("playback/volume"), value);
    Q_EMIT volumeChanged();
}

void SettingsManager::setMuted(bool value)
{
    if (m_muted == value)
    {
        return;
    }
    m_muted = value;
    saveValue(QStringLiteral("playback/muted"), value);
    Q_EMIT mutedChanged();
}

void SettingsManager::setOpenNewWindows(bool value)
{
    if (m_openNewWindows == value)
    {
        return;
    }
    m_openNewWindows = value;
    saveValue(QStringLiteral("behavior/openNewWindows"), value);
    Q_EMIT openNewWindowsChanged();
}

void SettingsManager::setNormalizeVolume(bool value)
{
    if (m_normalizeVolume == value)
    {
        return;
    }
    m_normalizeVolume = value;
    saveValue(QStringLiteral("playback/normalizeVolume"), value);
    Q_EMIT normalizeVolumeChanged();
}

void SettingsManager::setHwdec(const QString& value)
{
    if (m_hwdec == value)
    {
        return;
    }
    m_hwdec = value;
    saveValue(QStringLiteral("playback/hwdec"), value);
    Q_EMIT hwdecChanged();
}

void SettingsManager::setSaveVideoPosition(bool value)
{
    if (m_saveVideoPosition == value)
    {
        return;
    }
    m_saveVideoPosition = value;
    saveValue(QStringLiteral("session/saveVideoPosition"), value);
    Q_EMIT saveVideoPositionChanged();
}

void SettingsManager::setSaveSession(bool value)
{
    if (m_saveSession == value)
    {
        return;
    }
    m_saveSession = value;
    saveValue(QStringLiteral("session/saveSession"), value);
    Q_EMIT saveSessionChanged();
}

void SettingsManager::setThumbnailPreview(bool value)
{
    if (m_thumbnailPreview == value)
    {
        return;
    }
    m_thumbnailPreview = value;
    saveValue(QStringLiteral("ui/thumbnailPreview"), value);
    Q_EMIT thumbnailPreviewChanged();
}

void SettingsManager::setShowRemaining(bool value)
{
    if (m_showRemaining == value)
    {
        return;
    }
    m_showRemaining = value;
    saveValue(QStringLiteral("ui/showRemaining"), value);
    Q_EMIT showRemainingChanged();
}

void SettingsManager::setSubtitleFont(const QString& value)
{
    if (m_subtitleFont == value)
    {
        return;
    }
    m_subtitleFont = value;
    saveValue(QStringLiteral("subtitles/font"), value);
    Q_EMIT subtitleFontChanged();
}

void SettingsManager::setSubtitleScale(double value)
{
    if (qFuzzyCompare(m_subtitleScale, value))
    {
        return;
    }
    m_subtitleScale = value;
    saveValue(QStringLiteral("subtitles/scale"), value);
    Q_EMIT subtitleScaleChanged();
}

void SettingsManager::setSubtitleColor(const QString& value)
{
    if (m_subtitleColor == value)
    {
        return;
    }
    m_subtitleColor = value;
    saveValue(QStringLiteral("subtitles/color"), value);
    Q_EMIT subtitleColorChanged();
}

void SettingsManager::setSubtitleBackground(bool value)
{
    if (m_subtitleBackground == value)
    {
        return;
    }
    m_subtitleBackground = value;
    saveValue(QStringLiteral("subtitles/background"), value);
    Q_EMIT subtitleBackgroundChanged();
}

/**
 * @brief Sets the subtitle background colour and auto-toggles background visibility.
 * @param value Colour in hex ARGB format.
 *
 * When the colour has a non-zero alpha channel the subtitle background
 * is automatically enabled.
 */
void SettingsManager::setSubtitleBackgroundColor(const QString& value)
{
    if (m_subtitleBackgroundColor == value)
    {
        return;
    }
    m_subtitleBackgroundColor = value;
    saveValue(QStringLiteral("subtitles/backgroundColor"), value);
    Q_EMIT subtitleBackgroundColorChanged();

    // Enable background automatically when alpha is non-zero
    QColor col(value);
    bool hasBg = col.isValid() && col.alpha() > 0;
    setSubtitleBackground(hasBg);
}

void SettingsManager::setSubtitleLanguages(const QString& value)
{
    if (m_subtitleLanguages == value)
    {
        return;
    }
    m_subtitleLanguages = value;
    saveValue(QStringLiteral("subtitles/languages"), value);
    Q_EMIT subtitleLanguagesChanged();
}

void SettingsManager::setAudioLanguages(const QString& value)
{
    if (m_audioLanguages == value)
    {
        return;
    }
    m_audioLanguages = value;
    saveValue(QStringLiteral("audio/languages"), value);
    Q_EMIT audioLanguagesChanged();
}

void SettingsManager::setAutoUpdate(bool value)
{
    if (m_autoUpdate == value)
    {
        return;
    }
    m_autoUpdate = value;
    saveValue(QStringLiteral("updates/autoUpdate"), value);
    Q_EMIT autoUpdateChanged();
}

int SettingsManager::leftClick() const
{
    return m_leftClick;
}
int SettingsManager::rightClick() const
{
    return m_rightClick;
}
QString SettingsManager::locale() const
{
    return m_locale;
}
QString SettingsManager::keyBindings() const
{
    return m_keyBindings;
}
QString SettingsManager::themeMode() const { return m_themeMode; }
QString SettingsManager::accentColor() const { return m_accentColor; }
bool SettingsManager::reduceMotion() const { return m_reduceMotion; }
bool SettingsManager::systemBackdrop() const { return m_systemBackdrop; }

/**
 * @brief Sets the left-click action binding.
 * @param value Action identifier; clamped to [0, MaxClickAction).
 */
void SettingsManager::setLeftClick(int value)
{
    // Reject out-of-range action identifiers
    if (value < 0 || value >= MaxClickAction)
    {
        return;
    }
    if (m_leftClick == value)
    {
        return;
    }
    m_leftClick = value;
    saveValue(QStringLiteral("behavior/leftClick"), value);
    Q_EMIT leftClickChanged();
}

/**
 * @brief Sets the right-click action binding.
 * @param value Action identifier; clamped to [0, MaxClickAction).
 */
void SettingsManager::setRightClick(int value)
{
    // Reject out-of-range action identifiers
    if (value < 0 || value >= MaxClickAction)
    {
        return;
    }
    if (m_rightClick == value)
    {
        return;
    }
    m_rightClick = value;
    saveValue(QStringLiteral("behavior/rightClick"), value);
    Q_EMIT rightClickChanged();
}

void SettingsManager::setLocale(const QString& value)
{
    if (m_locale == value)
    {
        return;
    }
    m_locale = value;
    saveValue(QStringLiteral("ui/locale"), value);
    Q_EMIT localeChanged();
}

void SettingsManager::setKeyBindings(const QString& value)
{
    if (m_keyBindings == value)
    {
        return;
    }
    m_keyBindings = value;
    saveValue(QStringLiteral("input/keyBindings"), value);
    Q_EMIT keyBindingsChanged();
}

void SettingsManager::setThemeMode(const QString& value)
{
    if ((value != QStringLiteral("dark") && value != QStringLiteral("light")) || m_themeMode == value)
        return;
    m_themeMode = value;
    saveValue(QStringLiteral("ui/themeMode"), value);
    Q_EMIT themeModeChanged();
}

void SettingsManager::setAccentColor(const QString& value)
{
    if (!QColor::isValidColorName(value) || m_accentColor == value)
        return;
    m_accentColor = value;
    saveValue(QStringLiteral("ui/accentColor"), value);
    Q_EMIT accentColorChanged();
}

void SettingsManager::setReduceMotion(bool value)
{
    if (m_reduceMotion == value)
        return;
    m_reduceMotion = value;
    saveValue(QStringLiteral("ui/reduceMotion"), value);
    Q_EMIT reduceMotionChanged();
}

void SettingsManager::setSystemBackdrop(bool value)
{
    if (m_systemBackdrop == value)
        return;
    m_systemBackdrop = value;
    saveValue(QStringLiteral("ui/systemBackdrop"), value);
    Q_EMIT systemBackdropChanged();
}

/**
 * @brief Removes legacy thumbfast thumbnail scripts that were migrated to a new location.
 */
void SettingsManager::removeLegacyThumbnailScripts()
{
    QFile::remove(PathUtils::mpvConfigDir() + QStringLiteral("/scripts/thumbfast.lua"));
    QFile::remove(PathUtils::mpvConfigDir() + QStringLiteral("/script-opts/thumbfast.conf"));
}

/**
 * @brief Restores all playback-related preferences to their default values.
 */
void SettingsManager::resetPlaybackSettings()
{
    setVolume(100);
    setMuted(false);
    setNormalizeVolume(false);
#ifdef Q_OS_WIN
    setHwdec(QStringLiteral("no"));
#else
    setHwdec(QStringLiteral("auto-safe"));
#endif
    setSubtitleScale(1.0);
    setSubtitleColor(QStringLiteral("#ebebeb"));
    setSubtitleBackgroundColor(QStringLiteral("#97000000"));
    setSubtitleBackground(false);
}

/**
 * @brief Persists the last application window size and emits change notification.
 * @param width  Window width in logical pixels.
 * @param height Window height in logical pixels.
 *
 * Ignores invalid (non-positive) dimensions and no-op when size is unchanged.
 */
void SettingsManager::saveWindowSize(int width, int height)
{
    // Ignore invalid dimensions
    if (width <= 0 || height <= 0)
    {
        return;
    }
    const QSize size(width, height);
    // Skip save if size hasn't changed
    if (size == m_initialSize)
    {
        return;
    }
    m_initialSize = size;
    saveValue(QStringLiteral("window/size"), size);
    Q_EMIT windowSettingsChanged();
}

/**
 * @brief Opens the mpv user configuration directory in the platform file manager.
 */
void SettingsManager::openConfigDirectory()
{
    // The user's standard mpv folder, where mpv.conf / input.conf / scripts live.
    QDesktopServices::openUrl(QUrl::fromLocalFile(PathUtils::mpvUserConfigDir()));
}

/**
 * @brief Starts a new CineWindows instance in a separate process.
 */
void SettingsManager::launchNewWindow()
{
    QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList());
}
