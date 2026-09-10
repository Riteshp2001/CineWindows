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

#include "WindowCornerPreference.h"

#include <QEvent>
#include <QPlatformSurfaceEvent>
#include <QWindow>

#ifdef Q_OS_WIN
#include <dwmapi.h>
#include <windows.h>
#endif

/**
 * @brief Constructs a WindowCornerPreference.
 * @param parent Optional QObject parent.
 */
WindowCornerPreference::WindowCornerPreference(QObject* parent)
    : QObject(parent)
{
}

/**
 * @brief Destructor. Removes the event filter from the tracked window.
 */
WindowCornerPreference::~WindowCornerPreference()
{
    if (m_targetWindow)
        m_targetWindow->removeEventFilter(this);
}

QWindow* WindowCornerPreference::targetWindow() const
{
    return m_targetWindow.data();
}

/**
 * @brief Sets the target native window and installs an event filter.
 * @param window The QWindow to track; may be null to clear.
 *
 * Removes the event filter from the previous window and wires a
 * destroyed-signal connection to auto-clear the pointer.
 */
void WindowCornerPreference::setTargetWindow(QWindow* window)
{
    if (m_targetWindow == window)
        return;

    // Tear down tracking for the previous window
    if (m_targetWindow)
        m_targetWindow->removeEventFilter(this);
    QObject::disconnect(m_destroyedConnection);

    m_targetWindow = window;
    if (window)
    {
        // Install filter so we can re-apply preference on surface creation
        window->installEventFilter(this);
        m_destroyedConnection = connect(window, &QObject::destroyed, this, [this] {
            m_targetWindow = nullptr;
            Q_EMIT targetWindowChanged();
        });
    }

    Q_EMIT targetWindowChanged();
    applyPreference();
}

bool WindowCornerPreference::rounded() const
{
    return m_rounded;
}

/**
 * @brief Sets whether window corners should be rounded.
 * @param rounded True to round corners, false for sharp corners.
 *
 * Applies the preference immediately via DWM.
 */
void WindowCornerPreference::setRounded(bool rounded)
{
    if (m_rounded == rounded)
        return;

    m_rounded = rounded;
    Q_EMIT roundedChanged();
    applyPreference();
}

/**
 * @brief Filters events on the target window to detect surface (re)creation.
 * @param watched The object that received the event.
 * @param event  The event data.
 * @return False to allow further event processing.
 *
 * When the platform surface is created (e.g. after the window is hidden and
 * shown again), the DWM corner preference is re-applied.
 */
bool WindowCornerPreference::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_targetWindow && event->type() == QEvent::PlatformSurface)
    {
        const auto* surfaceEvent = static_cast<QPlatformSurfaceEvent*>(event);
        // Re-apply preference whenever the native surface is freshly created
        if (surfaceEvent->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated)
            applyPreference();
    }

    return QObject::eventFilter(watched, event);
}

/**
 * @brief Applies the rounded / sharp corner preference via DwmSetWindowAttribute.
 *
 * Uses attribute 33 (DWMWCP) with value 2 (round) or 1 (don't round).
 * Failure is silently ignored because the attribute is cosmetic and may not
 * be supported on older DWM versions.
 */
void WindowCornerPreference::applyPreference()
{
#ifdef Q_OS_WIN
    if (!m_targetWindow)
        return;

    const WId nativeId = m_targetWindow->winId();
    if (nativeId == 0)
        return;

    constexpr DWORD windowCornerPreferenceAttribute = 33;
    constexpr DWORD doNotRoundPreference = 1;
    constexpr DWORD roundPreference = 2;
    const DWORD preference = m_rounded ? roundPreference : doNotRoundPreference;
    const HWND windowHandle = reinterpret_cast<HWND>(nativeId);

    // Cosmetic attribute; unsupported DWM versions may silently reject it
    (void)DwmSetWindowAttribute(windowHandle,
                                windowCornerPreferenceAttribute,
                                &preference,
                                sizeof(preference));
#endif
}
