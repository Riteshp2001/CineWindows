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
import QtQuick.Effects
import CineWindows

Item {
    id: icon
    /// Name of the icon (maps to qrc:/cinewindows/icons/actions/<name>.svg)
    property string name: ""
    /// Color to tint the icon
    property color tint: Theme.iconDefault
    /// Uses the matching Solar Bold asset instead of the default Linear asset.
    property bool filled: false
    /// Adds depth when an icon must remain legible over moving video.
    property bool shadowEnabled: false
    readonly property string resolvedName: name + (filled ? "-filled" : "")
    readonly property string _iconSource: name.length > 0 ? "qrc:/cinewindows/icons/actions/" + resolvedName + ".svg" : ""
    visible: name.length > 0

    implicitWidth: 16
    implicitHeight: 16
    opacity: enabled ? 1.0 : 0.35
    scale: filled ? 1.06 : 1.0

    Behavior on scale {
        NumberAnimation { duration: Theme.motionFast; easing.type: Easing.OutCubic }
    }

    Image {
        id: img
        anchors.fill: parent
        source: icon.name.length > 0 ? icon._iconSource : ""
        sourceSize.width: width
        sourceSize.height: height
        fillMode: Image.PreserveAspectFit
        smooth: true
        visible: false
    }

    MultiEffect {
        anchors.fill: parent
        source: img
        brightness: 1.0
        colorization: 1.0
        colorizationColor: icon.tint
        shadowEnabled: icon.shadowEnabled
        shadowColor: "#d9000000"
        shadowBlur: 0.65
        shadowHorizontalOffset: 0
        shadowVerticalOffset: 2
        visible: img.status !== Image.Error
    }

    Rectangle {
        id: fallback
        anchors.fill: parent
        color: icon.tint
        radius: width * 0.2
        visible: img.status === Image.Error
    }
}
