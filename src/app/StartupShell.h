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

#include <QBackingStore>
#include <QWindow>

class QGuiApplication;

class StartupShell final : public QWindow
{
public:
    /**
     * @brief Constructs a centered, frameless splash window.
     * @param app Reference to the QGuiApplication for screen geometry.
     */
    explicit StartupShell(const QGuiApplication& app);

    /** @brief Paints the splash screen content (gradient, logo, title). */
    void render();

protected:
    /** @brief Re-renders on expose and resize events. */
    bool event(QEvent* event) override;

private:
    QBackingStore m_backingStore;    //!< Off-screen raster buffer for the splash window
};
