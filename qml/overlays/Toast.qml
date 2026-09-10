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



import QtQuick
import QtQuick.Controls
import CineWindows

Label {
    id: root
    required property ViewportMetrics metrics

    /// Duration in ms before the toast auto-hides
    property int timeoutMs: 1600

    /// Shows a toast message that auto-hides after timeoutMs
    function showMessage(message) {
        text = message;
        opacity = 1;
        hideTimer.restart();
    }

    text: ""
    color: Theme.text
    font.pixelSize: metrics.fontBody
    padding: Math.round(12 * metrics.visualScale)
    width: Math.min(implicitWidth, Math.max(0, metrics.viewportWidth - 2 * metrics.spacingMd))
    wrapMode: Text.Wrap
    horizontalAlignment: Text.AlignHCenter
    opacity: 0

    Behavior on opacity {
        NumberAnimation {
            duration: Theme.motionNormal
        }
    }

    background: Rectangle {
        color: Theme.popover
        opacity: 0.95
        radius: root.metrics.surfaceRadius
        border.color: Theme.popoverBorder
        border.width: 1
    }

    Timer {
        id: hideTimer
        interval: root.timeoutMs
        onTriggered: root.opacity = 0
    }
}
