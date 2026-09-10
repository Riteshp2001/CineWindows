/*
 * Copyright (c) 2026 Ritesh Pandit
 * Last modified: 2026-09-10
 * Modified by: Ritesh Pandit
 */

import QtQuick
import QtQuick.Controls

Switch {
    id: sw
    implicitWidth: 48
    implicitHeight: 28
    HoverHandler { cursorShape: sw.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor }
    indicator: Rectangle {
        implicitWidth: 48
        implicitHeight: 28
        radius: 14
        color: sw.checked ? "#3584e4" : Theme.switchTrackOff
        Behavior on color {
            ColorAnimation {
                duration: Theme.motionFast
            }
        }
        Rectangle {
            width: 22
            height: 22
            radius: 11
            color: "white"
            anchors.verticalCenter: parent.verticalCenter
            x: sw.checked ? parent.width - width - 3 : 3
            Behavior on x {
                NumberAnimation {
                    duration: Theme.motionFast
                    easing.type: Easing.OutCubic
                }
            }
        }
    }
    contentItem: Item {}
}
