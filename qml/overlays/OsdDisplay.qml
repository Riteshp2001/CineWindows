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

    /// Whether the OSD is currently visible
    property bool showing: false
    /// Optional text label shown below the icon
    property string text: ""
    /// When true, icon and text are tinted with the danger color
    property bool isDanger: false

    /// Shows the OSD with the given icon name, optional text, and danger flag
    function show(iconName, textVal, isDangerVal) {
        osdIcon.name = iconName;
        root.text = textVal || "";
        root.isDanger = isDangerVal || false;
        showing = true;
        pulse.restart();
        osdTimer.restart();
    }

    width: metrics.overlaySize + (root.text.length > 0 ? metrics.spacingLg : 0)
    height: metrics.overlaySize + (root.text.length > 0 ? metrics.spacingLg : 0)
    visible: showing || opacity > 0
    opacity: showing ? 1.0 : 0.0
    scale: showing ? 1.0 : 0.94

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
            value: 0.88
        }
        NumberAnimation {
            target: root
            property: "scale"
            to: 1.0
            duration: Theme.motionNormal
            easing.type: Easing.OutCubic
        }
    }

    CineIcon {
        x: Math.round((parent.width - width) / 2)
        y: (root.text.length > 0 ? root.metrics.spacingMd : Math.round((parent.height - height) / 2))
           + Math.round(2 * root.metrics.visualScale)
        width: root.metrics.overlayIconSize
        height: root.metrics.overlayIconSize
        name: osdIcon.name
        tint: "#aa000000"
        opacity: 0.66
    }

    CineIcon {
        id: osdIcon
        objectName: "osdIcon"
        x: Math.round((parent.width - width) / 2)
        y: root.text.length > 0 ? root.metrics.spacingMd : Math.round((parent.height - height) / 2)
        tint: root.isDanger ? Theme.danger : Theme.iconOnDark
        width: root.metrics.overlayIconSize
        height: root.metrics.overlayIconSize
    }

    Text {
        x: osdText.x
        y: osdText.y + Math.round(2 * root.metrics.visualScale)
        text: root.text
        color: "#aa000000"
        font.pixelSize: Math.round((Theme.fontSizeCaption + 3) * root.metrics.visualScale)
        font.bold: true
        visible: root.text.length > 0
    }

    Text {
        id: osdText
        anchors.top: osdIcon.bottom
        anchors.topMargin: root.metrics.spacingSm
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.text
        color: root.isDanger ? Theme.danger : Theme.iconOnDark
        font.pixelSize: Math.round(18 * root.metrics.visualScale)
        font.bold: true
        visible: text.length > 0
        style: Text.Outline
        styleColor: Theme.textOutlineStrong
    }

    Timer {
        id: osdTimer
        interval: 950
        onTriggered: root.showing = false
    }
}
