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

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import CineWindows

Slider {
    id: root

    property bool showTicks: true
    property real markerValue: Number.NaN
    property bool fillFromMarker: false
    readonly property int railThickness: 4
    readonly property int thumbSize: 16

    padding: 0
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    snapMode: stepSize > 0 ? Slider.SnapAlways : Slider.NoSnap
    opacity: root.enabled ? 1 : 0.5
    HoverHandler { cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor }

    implicitWidth: root.horizontal ? 200 : 32
    implicitHeight: root.horizontal ? 32 : 160

    readonly property int steps: root.stepSize > 0
        ? Math.floor(Number((Math.abs(root.to - root.from) / root.stepSize).toFixed(6)))
        : 0
    readonly property bool hasTicks: root.showTicks && root.steps > 0 && root.steps <= 20
    readonly property bool showMarker: isFinite(root.markerValue)
        && root.markerValue >= Math.min(root.from, root.to)
        && root.markerValue <= Math.max(root.from, root.to)
    readonly property real markerFraction: root.showMarker && root.to !== root.from
        ? (root.markerValue - root.from) / (root.to - root.from)
        : 0
    readonly property real fillOrigin: visualFraction(root.fillFromMarker && root.showMarker ? root.markerFraction : 0)

    function visualFraction(fraction) {
        return root.horizontal && !root.mirrored ? fraction : 1 - fraction;
    }

    background: Item {
        id: trackBg
        objectName: "sliderTrack"
        x: root.leftPadding + (root.horizontal ? root.thumbSize / 2 : (root.availableWidth - width) / 2)
        y: root.topPadding + (root.horizontal ? (root.availableHeight - height) / 2 : root.thumbSize / 2)
        width: root.horizontal ? Math.max(0, root.availableWidth - root.thumbSize) : root.railThickness
        height: root.horizontal ? root.railThickness : Math.max(0, root.availableHeight - root.thumbSize)

        Rectangle {
            anchors.fill: parent
            radius: root.railThickness / 2
            color: Theme.sliderTrack
        }

        Rectangle {
            objectName: "fill"
            color: Theme.sliderFill
            radius: root.railThickness / 2
            x: root.horizontal ? Math.min(root.fillOrigin, root.visualPosition) * trackBg.width : 0
            y: root.horizontal ? 0 : Math.min(root.fillOrigin, root.visualPosition) * trackBg.height
            width: root.horizontal ? Math.abs(root.visualPosition - root.fillOrigin) * trackBg.width : trackBg.width
            height: root.horizontal ? trackBg.height : Math.abs(root.visualPosition - root.fillOrigin) * trackBg.height
        }

        Repeater {
            objectName: "ticksRepeater"
            model: root.hasTicks ? root.steps + 1 : 0
            Rectangle {
                required property int index
                readonly property real fraction: root.visualFraction(root.steps > 0 ? index / root.steps : 0)
                width: 2
                height: 2
                radius: width / 2
                color: Theme.sliderTick
                x: root.horizontal ? fraction * trackBg.width - width / 2 : (trackBg.width - width) / 2
                y: root.horizontal ? (trackBg.height - height) / 2 : fraction * trackBg.height - height / 2
            }
        }

        Rectangle {
            objectName: "marker"
            visible: root.showMarker
            width: root.horizontal ? 2 : 10
            height: root.horizontal ? 10 : 2
            radius: 1
            color: Theme.sliderMarker
            x: root.horizontal ? root.visualFraction(root.markerFraction) * parent.width - width / 2 : (parent.width - width) / 2
            y: root.horizontal ? (parent.height - height) / 2 : root.visualFraction(root.markerFraction) * parent.height - height / 2
        }
    }

    handle: Rectangle {
        objectName: "thumb"
        implicitWidth: root.thumbSize
        implicitHeight: root.thumbSize
        radius: width / 2
        color: Theme.sliderThumb
        border.color: Theme.sliderThumbBorder
        border.width: 2
        x: root.horizontal
           ? root.leftPadding + root.visualPosition * Math.max(0, root.availableWidth - width)
           : root.leftPadding + (root.availableWidth - width) / 2
        y: root.horizontal
           ? root.topPadding + (root.availableHeight - height) / 2
           : root.topPadding + root.visualPosition * Math.max(0, root.availableHeight - height)
        scale: root.pressed ? 1.15 : 1

        Behavior on scale {
            NumberAnimation { duration: Theme.motionFast; easing.type: Easing.OutCubic }
        }

        Rectangle {
            objectName: "sliderFocusRing"
            anchors.centerIn: parent
            width: 28
            height: 28
            radius: width / 2
            z: -1
            color: Theme.sliderHalo
            border.width: root.visualFocus ? Theme.focusRingWidth : 0
            border.color: Theme.focusRing
            opacity: root.visualFocus || root.pressed ? 1 : root.hovered ? 0.6 : 0

            Behavior on opacity {
                NumberAnimation { duration: Theme.motionFast }
            }
        }
    }
}
