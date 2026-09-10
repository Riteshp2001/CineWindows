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
#include <QtQml/qqmlregistration.h>

class CineMpvItem;

/**
 * @class InputRouter
 * @brief Translates Qt keyboard and mouse input into mpv key events.
 * @details Routes pressed keys, mouse wheel scrolls, and forward mouse buttons
 *          from the Qt event system to the mpv player instance using mpv's
 *          key binding naming conventions. Exposed to QML for seamless integration
 *          with the CineWindows UI layer.
 */
class InputRouter : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(CineMpvItem* player READ player WRITE setPlayer NOTIFY playerChanged)

public:
    /**
     * @brief Constructs an InputRouter with an optional QObject parent.
     * @param parent Optional parent object for Qt ownership.
     */
    explicit InputRouter(QObject* parent = nullptr);

    /**
     * @brief Returns the currently assigned mpv player instance.
     * @return Pointer to the CineMpvItem, or nullptr if none is set.
     */
    CineMpvItem* player() const;

    /**
     * @brief Sets the mpv player instance to receive routed key events.
     * @param player Pointer to the CineMpvItem to assign.
     */
    void setPlayer(CineMpvItem* player);

    /**
     * @brief Handles a Qt key event and forwards it as an mpv key command.
     * @param key       Qt key code (e.g. Qt::Key_A).
     * @param modifiers Bitmask of Qt keyboard modifiers (Qt::ShiftModifier, etc.).
     * @param pressed   True on key press, false on release.
     * @return True if the key was successfully forwarded to mpv.
     */
    Q_INVOKABLE bool handleKey(int key, int modifiers, bool pressed);

    /**
     * @brief Handles mouse wheel scroll events for timeline seeking.
     * @param delta Scroll delta in eighths of a degree; positive scrolls forward.
     */
    Q_INVOKABLE void wheelSeek(double delta);

    /**
     * @brief Forwards a forward/extra mouse button event to mpv.
     * @param button Button identifier string (e.g. "BTN_FORWARD", "BTN_BACK").
     */
    Q_INVOKABLE void forwardMouseButton(const QString& button);

Q_SIGNALS:
    /**
     * @brief Emitted when the player property changes.
     */
    void playerChanged();

private:
    /**
     * @brief Converts a Qt key code and modifiers into an mpv key name string.
     * @param key       Qt key code.
     * @param modifiers Bitmask of Qt keyboard modifiers.
     * @return mpv key name (e.g. "Ctrl+a") or empty string if unmapped.
     */
    QString mpvKey(int key, unsigned modifiers) const;

    /** @brief Currently assigned mpv player instance. */
    CineMpvItem* m_player{nullptr};
};
