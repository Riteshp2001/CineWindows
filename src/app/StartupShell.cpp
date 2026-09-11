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

#include "StartupShell.h"

#include <QGuiApplication>
#include <QIcon>
#include <QLinearGradient>
#include <QPainter>
#include <QScreen>

#include <algorithm>

/**
 * @brief Creates a centered, frameless splash window sized to half the screen.
 * @param app Application instance used to query primary screen geometry.
 */
StartupShell::StartupShell(const QGuiApplication& app, QScreen* screen)
    : m_backingStore(this)
{
    setTitle(QStringLiteral("CineWindows"));
    setFlags(Qt::Window | Qt::FramelessWindowHint);
    setSurfaceType(QSurface::RasterSurface);

    QScreen* selectedScreen = screen ? screen : app.primaryScreen();
    if (selectedScreen)
        setScreen(selectedScreen);
    const QRect available = selectedScreen
        ? selectedScreen->availableGeometry() : QRect(0, 0, 1200, 800);
    const QSize shellSize(std::max(1, (available.width() + 1) / 2),
                          std::max(1, (available.height() * 3 + 2) / 5));
    resize(shellSize);
    setPosition(available.x() + (available.width() - shellSize.width() + 1) / 2,
                available.y() + (available.height() - shellSize.height() + 1) / 2);
}

/**
 * @brief Draws the branded splash screen with gradient, logo, and status text.
 * @details Renders a vertical gradient, centered logo, application name, and
 *          status text into the backing store, then flushes it to the screen.
 */
void StartupShell::render()
{
    if (!isExposed())
    {
        return;
    }

    m_backingStore.resize(size());
    const QRegion region(QRect(QPoint(), size()));
    m_backingStore.beginPaint(region);

    // Paint vertical gradient background
    QPainter painter(m_backingStore.paintDevice());
    QLinearGradient background(0, 0, 0, height());
    background.setColorAt(0.0, QColor(QStringLiteral("#17171c")));
    background.setColorAt(1.0, QColor(QStringLiteral("#09090c")));
    painter.fillRect(QRect(QPoint(), size()), background);

    // Draw centered logo
    const QPixmap logo =
        QIcon(QStringLiteral(":/cinewindows/icons/apps/CineWindows.svg")).pixmap(QSize(88, 88));
    const QPoint logoPosition((width() - logo.width()) / 2, (height() - logo.height()) / 2 - 58);
    painter.drawPixmap(logoPosition, logo);

    // Draw application title
    QFont font = QGuiApplication::font();
    font.setPixelSize(24);
    font.setWeight(QFont::DemiBold);
    painter.setFont(font);
    painter.setPen(QColor(QStringLiteral("#f5f5f7")));
    painter.drawText(QRect(0, logoPosition.y() + logo.height() + 20, width(), 36),
                     Qt::AlignHCenter | Qt::AlignTop, QStringLiteral("CineWindows"));

    // Draw status text below title
    font.setPixelSize(13);
    font.setWeight(QFont::Normal);
    painter.setFont(font);
    painter.setPen(QColor(QStringLiteral("#a8a8b2")));
    painter.drawText(QRect(0, logoPosition.y() + logo.height() + 58, width(), 24),
                     Qt::AlignHCenter | Qt::AlignTop, QStringLiteral("Starting player\u2026"));

    painter.end();
    m_backingStore.endPaint();
    m_backingStore.flush(region);
}

/**
 * @brief Filters events to trigger re-rendering on expose and resize.
 * @param event The incoming event.
 * @return True if the event was handled by the base class.
 */
bool StartupShell::event(QEvent* event)
{
    const bool handled = QWindow::event(event);
    if (event->type() == QEvent::Expose || event->type() == QEvent::Resize)
    {
        render();
    }
    return handled;
}
