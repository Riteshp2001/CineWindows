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

MenuItem {
    id: root
    HoverHandler { cursorShape: Qt.PointingHandCursor }
    /// Responsive metrics shared by the containing menu
    readonly property var _owningMenu: root.menu
    property ViewportMetrics metrics: _owningMenu && _owningMenu.metrics
        ? _owningMenu.metrics
        : fallbackMetrics
    /// Keyboard shortcut text displayed right-aligned in the item
    property string shortcut: ""
    /// Minimum width of the menu item in pixels
    property int minItemWidth: Math.round(222 * metrics.visualScale)

    ViewportMetrics {
        id: fallbackMetrics
    }

    implicitWidth: Math.max(minItemWidth, parent && parent.width > 0 ? parent.width : minItemWidth)
    implicitHeight: visible ? metrics.controlStandard : 0
    leftPadding: metrics.spacingSm + metrics.spacingXs
    rightPadding: metrics.spacingSm + metrics.spacingXs
    indicator: null

    contentItem: Item {
        implicitWidth: Math.max(0, root.implicitWidth - root.leftPadding - root.rightPadding)
        implicitHeight: root.implicitHeight

        Rectangle {
            id: radioIndicator
            visible: root.checkable
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: 14
            height: 14
            radius: 7
            color: "transparent"
            border.color: Theme.text
            border.width: 1.5

            Rectangle {
                anchors.centerIn: parent
                width: 6
                height: 6
                radius: 3
                color: Theme.text
                visible: root.checked
            }
        }

        Text {
            objectName: "menuItemLabel"
            anchors.left: root.checkable ? radioIndicator.right : parent.left
            anchors.leftMargin: root.checkable ? 8 : 0
            anchors.right: shortcutText.visible ? shortcutText.left : parent.right
            anchors.rightMargin: shortcutText.visible ? 12 : 0
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            color: root.enabled ? Theme.text : "#88ffffff"
            font.pixelSize: root.metrics.fontCaption
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        Text {
            id: shortcutText
            visible: root.shortcut.length > 0
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            text: root.shortcut
            color: Theme.mutedText
            font.pixelSize: Math.max(10, root.metrics.fontCaption - 1)
            verticalAlignment: Text.AlignVCenter
        }
    }

    background: Rectangle {
        implicitWidth: root.implicitWidth
        implicitHeight: root.implicitHeight
        radius: Math.max(6, root.metrics.popupRadius - 2)
        color: root.down ? Theme.glassActive : (root.hovered || root.highlighted ? Theme.glassHover : "transparent")
        anchors.fill: parent
        anchors.leftMargin: 3
        anchors.rightMargin: 3
        anchors.topMargin: 2
        anchors.bottomMargin: 2
        Behavior on color {
            ColorAnimation {
                duration: Theme.motionFast
                easing.type: Easing.OutCubic
            }
        }
    }
}
