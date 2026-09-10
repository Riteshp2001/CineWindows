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

#include "player/InputRouter.h"

#include "player/CineMpvItem.h"

#include <QKeySequence>
#include <Qt>

InputRouter::InputRouter(QObject* parent)
    : QObject(parent)
{}

CineMpvItem* InputRouter::player() const
{
    return m_player;
}

void InputRouter::setPlayer(CineMpvItem* player)
{
    if (m_player == player)
    {
        return;
    }
    m_player = player;
    Q_EMIT playerChanged();
}

bool InputRouter::handleKey(int key, int modifiers, bool pressed)
{
    // Guard: no player assigned
    if (!m_player)
    {
        return false;
    }

    // Map Qt key+modifiers to an mpv key name
    const QString mapped = mpvKey(key, static_cast<unsigned>(modifiers));
    if (mapped.isEmpty())
    {
        return false;
    }

    // Forward to mpv as keypress or keyup command
    m_player->runCommandAsync({pressed ? QStringLiteral("keypress") : QStringLiteral("keyup"), mapped});
    return true;
}

/**
 * @brief Routes mouse wheel scroll events as relative seek commands to mpv.
 * @param delta Scroll delta; positive scrolls forward, negative backward.
 */
void InputRouter::wheelSeek(double delta)
{
    if (m_player)
    {
        // Seek 5 seconds per wheel step in the direction of scroll
        m_player->seekRelative(delta > 0 ? 5.0 : -5.0);
    }
}

/**
 * @brief Forwards forward/extra mouse button events (e.g. BTN_FORWARD, BTN_BACK) to mpv.
 * @param button Button identifier string as expected by mpv's key binding system.
 */
void InputRouter::forwardMouseButton(const QString& button)
{
    if (m_player)
    {
        // Send a keypress command directly with the button name
        m_player->runCommandAsync({QStringLiteral("keypress"), button});
    }
}

/**
 * @brief Converts a Qt key code and modifier bitmask into an mpv key name string.
 * @details Handles keypad keys, named special keys, shifted symbols, and letter case
 *          normalization according to mpv's key naming convention. Emulates the
 *          behavior of mpv's key name resolution for Qt input events.
 * @param key       Qt key code (e.g. Qt::Key_A).
 * @param modifiers Bitmask of Qt keyboard modifiers.
 * @return mpv key name string (e.g. "Ctrl+Shift+A", "LEFT"), or empty if unmapped.
 */
QString InputRouter::mpvKey(int key, unsigned modifiers) const
{
    QString base;
    bool shiftedSymbol = false;

    // --- Keypad keys: map Qt keypad codes to mpv KP_* names ---
    if (modifiers & Qt::KeypadModifier)
    {
        modifiers &= ~Qt::KeypadModifier;
        switch (key)
        {
            case Qt::Key_0:
                base = QStringLiteral("KP0");
                break;
            case Qt::Key_1:
                base = QStringLiteral("KP1");
                break;
            case Qt::Key_2:
                base = QStringLiteral("KP2");
                break;
            case Qt::Key_3:
                base = QStringLiteral("KP3");
                break;
            case Qt::Key_4:
                base = QStringLiteral("KP4");
                break;
            case Qt::Key_5:
                base = QStringLiteral("KP5");
                break;
            case Qt::Key_6:
                base = QStringLiteral("KP6");
                break;
            case Qt::Key_7:
                base = QStringLiteral("KP7");
                break;
            case Qt::Key_8:
                base = QStringLiteral("KP8");
                break;
            case Qt::Key_9:
                base = QStringLiteral("KP9");
                break;
            case Qt::Key_Plus:
                base = QStringLiteral("KP_ADD");
                break;
            case Qt::Key_Minus:
                base = QStringLiteral("KP_SUBTRACT");
                break;
            case Qt::Key_Asterisk:
                base = QStringLiteral("KP_MULTIPLY");
                break;
            case Qt::Key_Slash:
                base = QStringLiteral("KP_DIVIDE");
                break;
            case Qt::Key_Enter:
                base = QStringLiteral("KP_ENTER");
                break;
            case Qt::Key_Home:
                base = QStringLiteral("KP_HOME");
                break;
            case Qt::Key_End:
                base = QStringLiteral("KP_END");
                break;
            case Qt::Key_Left:
                base = QStringLiteral("KP_LEFT");
                break;
            case Qt::Key_Right:
                base = QStringLiteral("KP_RIGHT");
                break;
            case Qt::Key_Up:
                base = QStringLiteral("KP_UP");
                break;
            case Qt::Key_Down:
                base = QStringLiteral("KP_DOWN");
                break;
            case Qt::Key_PageUp:
                base = QStringLiteral("KP_PGUP");
                break;
            case Qt::Key_PageDown:
                base = QStringLiteral("KP_PGDWN");
                break;
            case Qt::Key_Clear:
                base = QStringLiteral("KP_BEGIN");
                break;
            default:
                break;
        }
    }

    // --- Named special keys: map Qt named keys to mpv uppercase names ---
    if (base.isEmpty())
    {
        switch (key)
        {
            case Qt::Key_Space:
                base = QStringLiteral("SPACE");
                break;
            case Qt::Key_Left:
                base = QStringLiteral("LEFT");
                break;
            case Qt::Key_Right:
                base = QStringLiteral("RIGHT");
                break;
            case Qt::Key_Up:
                base = QStringLiteral("UP");
                break;
            case Qt::Key_Down:
                base = QStringLiteral("DOWN");
                break;
            case Qt::Key_Escape:
                base = QStringLiteral("ESC");
                break;
            case Qt::Key_PageUp:
                base = QStringLiteral("PGUP");
                break;
            case Qt::Key_PageDown:
                base = QStringLiteral("PGDWN");
                break;
            case Qt::Key_Home:
                base = QStringLiteral("HOME");
                break;
            case Qt::Key_End:
                base = QStringLiteral("END");
                break;
            case Qt::Key_Insert:
                base = QStringLiteral("INS");
                break;
            case Qt::Key_Delete:
                base = QStringLiteral("DEL");
                break;
            case Qt::Key_Pause:
                base = QStringLiteral("PAUSE");
                break;
            case Qt::Key_Comma:
                base = QStringLiteral(",");
                break;
            case Qt::Key_Period:
                base = QStringLiteral(".");
                break;
            case Qt::Key_Slash:
                base = QStringLiteral("/");
                break;
            case Qt::Key_Backslash:
                base = QStringLiteral("\\");
                break;
            case Qt::Key_BracketLeft:
                base = QStringLiteral("[");
                break;
            case Qt::Key_BracketRight:
                base = QStringLiteral("]");
                break;
            case Qt::Key_Minus:
                base = QStringLiteral("-");
                break;
            case Qt::Key_Equal:
                base = QStringLiteral("=");
                break;
            // --- Shifted symbols: mark to strip Shift from modifiers later ---
            case Qt::Key_Plus:
                base = QStringLiteral("+");
                shiftedSymbol = true;
                break;
            case Qt::Key_Asterisk:
                base = QStringLiteral("*");
                shiftedSymbol = true;
                break;
            case Qt::Key_NumberSign:
                base = QStringLiteral("SHARP");
                shiftedSymbol = true;
                break;
            case Qt::Key_BraceLeft:
                base = QStringLiteral("{");
                shiftedSymbol = true;
                break;
            case Qt::Key_BraceRight:
                base = QStringLiteral("}");
                shiftedSymbol = true;
                break;
            case Qt::Key_Less:
                base = QStringLiteral("<");
                shiftedSymbol = true;
                break;
            case Qt::Key_Greater:
                base = QStringLiteral(">");
                shiftedSymbol = true;
                break;
            case Qt::Key_Question:
                base = QStringLiteral("?");
                shiftedSymbol = true;
                break;
            case Qt::Key_Exclam:
                base = QStringLiteral("!");
                shiftedSymbol = true;
                break;
            case Qt::Key_At:
                base = QStringLiteral("@");
                shiftedSymbol = true;
                break;
            case Qt::Key_Underscore:
                base = QStringLiteral("_");
                shiftedSymbol = true;
                break;
            case Qt::Key_Bar:
                base = QStringLiteral("|");
                shiftedSymbol = true;
                break;
            case Qt::Key_AsciiTilde:
                base = QStringLiteral("~");
                shiftedSymbol = true;
                break;
            case Qt::Key_QuoteLeft:
                base = QStringLiteral("`");
                break;
            default:
                base = QKeySequence(key).toString(QKeySequence::PortableText);
                break;
        }
    }

    if (base.isEmpty())
    {
        return {};
    }

    // Strip Shift from modifiers when the symbol itself encodes it
    if (shiftedSymbol)
    {
        modifiers &= ~Qt::ShiftModifier;
    }

    // mpv key convention: single alpha keys use lowercase for unshifted,
    // uppercase for Shift+letter. Qt's PortableText always reports alpha as
    // uppercase, so we correct it here — uppercase encodes the Shift state.
    if (base.length() == 1 && base[0].isLetter() && base[0].isUpper())
    {
        if (modifiers & Qt::ShiftModifier)
        {
            // Keep uppercase — mpv uses uppercase to represent Shift+letter
            modifiers &= ~Qt::ShiftModifier;
        }
        else
        {
            // No shift held: mpv expects lowercase
            base = base.toLower();
        }
    }

    // Assemble modifier prefix list in mpv order: Ctrl > Alt > Shift > Meta
    QStringList parts;
    if (modifiers & Qt::ControlModifier)
        parts << QStringLiteral("Ctrl");
    if (modifiers & Qt::AltModifier)
        parts << QStringLiteral("Alt");
    if (modifiers & Qt::ShiftModifier)
        parts << QStringLiteral("Shift");
    if (modifiers & Qt::MetaModifier)
        parts << QStringLiteral("Meta");
    parts << base;
    return parts.join(QLatin1Char('+'));
}
