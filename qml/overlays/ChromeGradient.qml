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
import CineWindows

Rectangle {
    id: root
    required property ViewportMetrics metrics

    /// Whether this gradient is at the top edge (true) or bottom edge (false)
    property bool topEdge: true
    /// Whether the gradient overlay is enabled at all
    property bool active: true
    /// Whether the gradient is currently shown (controls fade opacity)
    property bool shown: true

    height: topEdge ? metrics.chromeTopGradientHeight : metrics.chromeBottomGradientHeight
    visible: active
    opacity: shown ? 1 : 0

    Behavior on opacity {
        NumberAnimation {
            duration: Theme.motionNormal
            easing.type: Easing.OutCubic
        }
    }

    gradient: Gradient {
        GradientStop {
            position: 0.0
            color: root.topEdge ? Theme.chromeTopGradientStart : "transparent"
        }
        GradientStop {
            position: 0.4
            color: root.topEdge ? Theme.chromeTopGradientMid : Theme.chromeBottomGradientMid
        }
        GradientStop {
            position: 1.0
            color: root.topEdge ? "transparent" : Theme.chromeBottomGradientEnd
        }
    }
}
