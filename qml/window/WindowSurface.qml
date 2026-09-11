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
 * Last modified: 2026-09-11
 * Modified by: Ritesh Pandit
 */

import QtQuick
import QtQuick.Window
import "../style" as Style

Rectangle {
    id: root

    required property var targetWindow
    property bool clientSideDecorated: false
    property bool compact: false
    property bool backdropActive: false
    readonly property bool restored: !!targetWindow
        && targetWindow.visibility === Window.Windowed
    readonly property bool rounded: clientSideDecorated && restored
    readonly property int restoredRadius: Style.Theme.windowRadius
    readonly property int compactRadius: Style.Theme.windowCompactRadius

    color: backdropActive
        ? Style.Theme.windowBackdropTint
        : Style.Theme.background
    radius: rounded ? (compact ? compactRadius : restoredRadius) : 0
    border.width: rounded ? Style.Theme.windowBorderWidth : 0
    border.color: Style.Theme.windowBorderColor
    clip: true

    Rectangle {
        objectName: "windowSurfaceForegroundBorder"
        anchors.fill: parent
        z: 100000
        visible: root.rounded
        color: "transparent"
        radius: root.radius
        border.width: root.border.width
        border.color: root.border.color
    }
}