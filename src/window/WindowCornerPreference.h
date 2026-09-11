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

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QWindow>
#include <QtQmlIntegration/qqmlintegration.h>

/**
 * @class WindowCornerPreference
 * @brief Manages native frame effects and client-side window shaping.
 *
 * @details
 * Applies DWM corner, dark-mode, and backdrop preferences on Windows. For
 * client-side decorations on other platforms it applies a rounded window mask.
 * Native state is refreshed when the platform surface, size, or window state changes.
 */
class WindowCornerPreference : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    /** @brief The native window whose corner style is managed. */
    Q_PROPERTY(QWindow* targetWindow READ targetWindow WRITE setTargetWindow NOTIFY targetWindowChanged)
    /** @brief Whether window corners should be rounded. */
    Q_PROPERTY(bool rounded READ rounded WRITE setRounded NOTIFY roundedChanged)
    Q_PROPERTY(int cornerRadius READ cornerRadius WRITE setCornerRadius NOTIFY cornerRadiusChanged)
    Q_PROPERTY(bool clientSideDecorated READ clientSideDecorated WRITE setClientSideDecorated NOTIFY clientSideDecoratedChanged)
    Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode NOTIFY darkModeChanged)
    Q_PROPERTY(bool backdropEnabled READ backdropEnabled WRITE setBackdropEnabled NOTIFY backdropEnabledChanged)
    Q_PROPERTY(bool backdropActive READ backdropActive NOTIFY backdropActiveChanged)
    Q_PROPERTY(bool clientSideDecorationsRecommended READ clientSideDecorationsRecommended CONSTANT)
    Q_PROPERTY(QString decorationStyle READ decorationStyle CONSTANT)

public:
    /**
     * @brief Constructs a WindowCornerPreference object.
     * @param parent Optional QObject parent.
     */
    explicit WindowCornerPreference(QObject* parent = nullptr);
    /**
     * @brief Removes the event filter from the previously tracked window.
     */
    ~WindowCornerPreference() override;

    /** @brief Returns the currently tracked native window. @return Target QWindow pointer, or null. */
    QWindow* targetWindow() const;
    /** @brief Sets the target native window and installs an event filter. @param window Window to track. */
    void setTargetWindow(QWindow* window);

    /** @brief Returns whether window corners are rounded. @return True if rounded, false if sharp. */
    bool rounded() const;
    /** @brief Sets whether window corners should be rounded. @param rounded True to round, false for sharp. */
    void setRounded(bool rounded);

    int cornerRadius() const;
    void setCornerRadius(int radius);
    bool clientSideDecorated() const;
    void setClientSideDecorated(bool decorated);
    bool darkMode() const;
    void setDarkMode(bool darkMode);
    bool backdropEnabled() const;
    void setBackdropEnabled(bool enabled);
    bool backdropActive() const;
    bool clientSideDecorationsRecommended() const;
    QString decorationStyle() const;

Q_SIGNALS:
    /** @brief Emitted when the target window pointer changes. */
    void targetWindowChanged();
    /** @brief Emitted when the rounded-corner preference changes. */
    void roundedChanged();
    void cornerRadiusChanged();
    void clientSideDecoratedChanged();
    void darkModeChanged();
    void backdropEnabledChanged();
    void backdropActiveChanged();

protected:
    /**
     * @brief Filters platform-surface events to re-apply the corner preference.
     * @param watched The object that received the event.
     * @param event  The event data.
     * @return False to allow further event processing.
     */
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    /** @brief Applies native backdrop, corner, and client-side mask preferences. */
    void applyPreference();
    void setBackdropActive(bool active);

    QPointer<QWindow> m_targetWindow;             ///< The tracked native window.
    QMetaObject::Connection m_destroyedConnection; ///< Connection to the window's destroyed signal.
    bool m_rounded = true;                        ///< Whether rounded corners are currently enabled.
    int m_cornerRadius{12};
    bool m_clientSideDecorated{false};
    bool m_darkMode{true};
    bool m_backdropEnabled{true};
    bool m_backdropActive{false};
};
