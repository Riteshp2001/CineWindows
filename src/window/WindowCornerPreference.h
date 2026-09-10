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
#include <QWindow>
#include <QtQmlIntegration/qqmlintegration.h>

/**
 * @class WindowCornerPreference
 * @brief Manages the rounded-corner preference for a native QWindow on Windows.
 *
 * @details
 * Applies the DWM window-corner preference (DWMWCP_ROUND / DWMWCP_DONOTROUND)
 * via DwmSetWindowAttribute and re-applies it when the platform surface is
 * (re)created. Installs an event filter on the target window to react to
 * QPlatformSurfaceEvent.
 *
 * On non-Windows platforms all methods are no-ops.
 */
class WindowCornerPreference : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    /** @brief The native window whose corner style is managed. */
    Q_PROPERTY(QWindow* targetWindow READ targetWindow WRITE setTargetWindow NOTIFY targetWindowChanged)
    /** @brief Whether window corners should be rounded. */
    Q_PROPERTY(bool rounded READ rounded WRITE setRounded NOTIFY roundedChanged)

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

Q_SIGNALS:
    /** @brief Emitted when the target window pointer changes. */
    void targetWindowChanged();
    /** @brief Emitted when the rounded-corner preference changes. */
    void roundedChanged();

protected:
    /**
     * @brief Filters platform-surface events to re-apply the corner preference.
     * @param watched The object that received the event.
     * @param event  The event data.
     * @return False to allow further event processing.
     */
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    /** @brief Applies the DWM corner-preference attribute on the target window. */
    void applyPreference();

    QPointer<QWindow> m_targetWindow;             ///< The tracked native window.
    QMetaObject::Connection m_destroyedConnection; ///< Connection to the window's destroyed signal.
    bool m_rounded = true;                        ///< Whether rounded corners are currently enabled.
};
