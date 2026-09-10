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

#include <QColor>
#include <QQuickPaintedItem>
#include <QtQml/qqmlregistration.h>

class QrCodeItem : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QColor foreground READ foreground WRITE setForeground NOTIFY foregroundChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)

public:
    /**
     * @brief Constructs a QR code item that renders a QR code via QPainter.
     * @param parent Optional parent QQuickItem.
     */
    explicit QrCodeItem(QQuickItem* parent = nullptr);

    /** @brief Returns the current QR code payload text. */
    QString text() const;
    /** @brief Sets the QR code payload text and triggers a repaint. */
    void setText(const QString& text);
    /** @brief Returns the current foreground color used for QR modules. */
    QColor foreground() const;
    /** @brief Sets the foreground color for QR modules. */
    void setForeground(const QColor& color);
    /** @brief Returns the current background color. */
    QColor backgroundColor() const;
    /** @brief Sets the background color behind the QR code. */
    void setBackgroundColor(const QColor& color);
    /** @brief Paints the QR code using the given painter. */
    void paint(QPainter* painter) override;

Q_SIGNALS:
    void textChanged();
    void foregroundChanged();
    void backgroundColorChanged();

private:
    QString m_text;                                   //!< QR code payload text
    QColor m_foreground{QStringLiteral("#102022")};   //!< QR module foreground color
    QColor m_background{QStringLiteral("#f5ffff")};   //!< Background color behind the QR code
};
