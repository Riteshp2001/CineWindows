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

Item {
    id: root
    required property ViewportMetrics metrics

    /// Whether the volume OSD is currently visible
    property bool showing: false
    /// Text displayed on the OSD (e.g. "Volume: 75%")
    property string displayText: ""
    /// When true, text is tinted with the danger color (e.g. volume > 100)
    property bool danger: false

    /// Shows arbitrary text on the OSD with optional danger highlighting
    function showText(text, isDanger) {
        displayText = text || "";
        danger = isDanger || false;
        showing = true;
        pulse.restart();
        hideTimer.restart();
    }

    /// Shows a volume level (0-100) or "Muted" text on the OSD
    function showVolume(volume, muted) {
        var vol = Math.round(volume);
        showText(muted ? qsTr("Volume: Muted") : qsTr("Volume: %1%").arg(vol), !muted && vol > 100);
    }

    width: label.implicitWidth + metrics.spacingXs
    height: label.implicitHeight + metrics.spacingXs
    visible: showing || opacity > 0
    enabled: false
    opacity: showing ? 1.0 : 0.0
    scale: showing ? 1.0 : 0.985

    Behavior on opacity {
        NumberAnimation {
            duration: Theme.motionNormal
            easing.type: Easing.OutCubic
        }
    }
    Behavior on scale {
        NumberAnimation {
            duration: Theme.motionNormal
            easing.type: Easing.OutCubic
        }
    }

    SequentialAnimation {
        id: pulse
        PropertyAction {
            target: root
            property: "scale"
            value: 0.96
        }
        NumberAnimation {
            target: root
            property: "scale"
            to: 1.0
            duration: Theme.motionNormal
            easing.type: Easing.OutCubic
        }
    }

    Text {
        x: Math.max(1, Math.round(root.metrics.visualScale))
        y: Math.max(1, Math.round(2 * root.metrics.visualScale))
        text: root.displayText
        color: "#aa000000"
        font.pixelSize: root.metrics.iconLg
        font.bold: true
    }

    Text {
        id: label
        objectName: "volumeLabel"
        text: root.displayText
        color: root.danger ? Theme.danger : Theme.iconOnDark
        font.pixelSize: root.metrics.iconLg
        font.bold: true
        style: Text.Outline
        styleColor: Theme.textOutlineStrong
    }

    Timer {
        id: hideTimer
        interval: 1150
        repeat: false
        onTriggered: root.showing = false
    }
}
