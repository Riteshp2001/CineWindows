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

    /// Whether the audio-only indicator is active
    property bool active: false

    width: metrics.overlaySize + metrics.spacingMd
    height: metrics.overlaySize + metrics.spacingMd
    radius: width / 2
    color: Theme.audioOnlyBackground
    visible: active || opacity > 0
    opacity: active ? 0.33 : 0

    Behavior on opacity {
        NumberAnimation {
            duration: Theme.motionNormal
            easing.type: Easing.OutCubic
        }
    }

    CineIcon {
        objectName: "audioIcon"
        anchors.centerIn: parent
        width: root.metrics.overlayIconSize
        height: root.metrics.overlayIconSize
        name: "cine-audio-x-generic-symbolic"
        tint: Theme.text
    }
}
