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

#include "controls/QrCodeItem.h"

#include "qrcodegen.hpp"

#include <QPainter>

#include <algorithm>
#include <exception>

/**
 * @brief Constructs a QrCodeItem with antialiasing disabled and opaque painting enabled.
 */
QrCodeItem::QrCodeItem(QQuickItem* parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(false);
    setOpaquePainting(true);
}

QString QrCodeItem::text() const { return m_text; }

/**
 * @brief Sets the text to encode as a QR code and triggers a repaint.
 * @param text The payload string to encode.
 */
void QrCodeItem::setText(const QString& text)
{
    if (m_text == text)
        return;
    m_text = text;
    update();                       // Trigger repaint with new content
    Q_EMIT textChanged();
}

QColor QrCodeItem::foreground() const { return m_foreground; }

/**
 * @brief Sets the foreground color for QR code modules.
 * @param color The new foreground color.
 */
void QrCodeItem::setForeground(const QColor& color)
{
    if (m_foreground == color)
        return;
    m_foreground = color;
    update();                       // Trigger repaint with updated color
    Q_EMIT foregroundChanged();
}

QColor QrCodeItem::backgroundColor() const { return m_background; }

/**
 * @brief Sets the background color behind the QR code.
 * @param color The new background color.
 */
void QrCodeItem::setBackgroundColor(const QColor& color)
{
    if (m_background == color)
        return;
    m_background = color;
    update();                       // Trigger repaint with updated background
    Q_EMIT backgroundColorChanged();
}

/**
 * @brief Paints the QR code centered within the item's bounding rect.
 * @param painter The QPainter instance to draw with.
 */
void QrCodeItem::paint(QPainter* painter)
{
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->fillRect(boundingRect(), m_background);     // Fill background
    if (m_text.isEmpty())                                // Nothing to encode
        return;

    try
    {
        // Encode the text as a QR code at ECC MEDIUM
        const QByteArray encoded = m_text.toUtf8();
        const qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(encoded.constData(), qrcodegen::QrCode::Ecc::MEDIUM);
        constexpr int quietZone = 4;
        const int modules = qr.getSize() + quietZone * 2;
        const int scale = std::max(1, static_cast<int>(std::min(width(), height())) / modules);
        const int renderedSize = modules * scale;
        const int offsetX = (static_cast<int>(width()) - renderedSize) / 2;
        const int offsetY = (static_cast<int>(height()) - renderedSize) / 2;

        painter->setPen(Qt::NoPen);
        painter->setBrush(m_foreground);
        for (int y = 0; y < qr.getSize(); ++y)
        {
            for (int x = 0; x < qr.getSize(); ++x)
            {
                if (qr.getModule(x, y))
                {
                    // Draw filled module at scaled position with quiet zone offset
                    painter->drawRect(offsetX + (x + quietZone) * scale,
                                      offsetY + (y + quietZone) * scale,
                                      scale,
                                      scale);
                }
            }
        }
    }
    catch (const std::exception&)
    {
        // An empty background is safer than painting a partial or invalid code.
    }
}
