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
import QtQuick.Controls.Basic
import CineWindows

Slider {
    id: root

    /// Render a tick dot at each step
    property bool showTicks: true
    /// Optional reference line drawn at this value (e.g. the 100% volume marker)
    property real markerValue: -1

    readonly property bool reduceMotion: SettingsManager.reduceMotion

    padding: 0
    opacity: root.enabled ? 1 : 0.5
    HoverHandler { cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor }

    implicitWidth: root.horizontal ? 200 : 28
    implicitHeight: root.horizontal ? 28 : 200

    /// Spring-followed display position (0..1), eases toward `position` with weight
    property real glide: root.position

    Behavior on glide {
        enabled: !root.reduceMotion
        SpringAnimation {
            spring: 3
            damping: 0.3
            epsilon: 0.0001
        }
    }

    /// Glide clamped to the track so spring overshoot never pushes the thumb past the ends
    readonly property real pos: Math.max(0, Math.min(1, root.glide))

    /// Whole steps inside the range; ticks are only drawn for a sane count
    readonly property int steps: root.stepSize > 0
        ? Math.floor(Number(((root.to - root.from) / root.stepSize).toFixed(6)))
        : 0
    readonly property bool hasTicks: root.showTicks && root.steps > 0 && root.steps <= 50
    readonly property bool showMarker: root.markerValue >= root.from && root.markerValue <= root.to
    readonly property real markerFraction: root.to > root.from
        ? (root.markerValue - root.from) / (root.to - root.from)
        : 0
    readonly property int tickInset: 3

    background: Rectangle {
        id: trackBg
        x: root.leftPadding
        y: root.topPadding
        width: root.availableWidth
        height: root.availableHeight
        radius: 3
        color: Theme.sliderTrack

        Rectangle {
            id: fill
            objectName: "fill"
            color: Theme.sliderFill
            x: root.horizontal ? 0 : (trackBg.width - width) / 2
            y: root.horizontal ? 0 : (1 - root.pos) * trackBg.height
            width: root.horizontal ? root.pos * trackBg.width : trackBg.width
            height: root.horizontal ? trackBg.height : root.pos * trackBg.height
        }

        Repeater {
            id: ticksRepeater
            objectName: "ticksRepeater"
            model: root.hasTicks ? root.steps + 1 : 0
            Rectangle {
                required property int index
                readonly property real fraction: root.steps > 0 ? index / root.steps : 0
                readonly property real tickSize: Math.min(4, Math.max(2,
                    ((root.horizontal ? trackBg.width : trackBg.height) - 2 * root.tickInset)
                    / root.steps - 2))
                width: tickSize
                height: tickSize
                radius: width / 2
                color: Theme.sliderTick
                x: root.horizontal
                   ? root.tickInset + fraction * (trackBg.width - 2 * root.tickInset) - width / 2
                   : (trackBg.width - width) / 2
                y: root.horizontal
                   ? (trackBg.height - height) / 2
                   : root.tickInset + fraction * (trackBg.height - 2 * root.tickInset) - height / 2
            }
        }

        Rectangle {
            id: marker
            objectName: "marker"
            visible: root.markerValue >= root.from && root.markerValue <= root.to
            width: root.horizontal ? 2 : parent.width
            height: root.horizontal ? parent.height : 2
            radius: 1
            color: Theme.sliderMarker
            x: root.horizontal ? root.markerFraction * (parent.width - width) : 0
            y: root.horizontal ? 0 : root.markerFraction * (parent.height - height)
        }
    }

    handle: Rectangle {
        id: thumb
        objectName: "thumb"
        color: "transparent"
        implicitWidth: root.horizontal ? 6 : 20
        implicitHeight: root.horizontal ? 20 : 6

        x: root.horizontal
           ? root.leftPadding + root.pos * (root.availableWidth - width)
           : root.leftPadding + (root.availableWidth - width) / 2
        y: root.horizontal
           ? root.topPadding + (root.availableHeight - height) / 2
           : root.topPadding + (1 - root.pos) * (root.availableHeight - height)

        Rectangle {
            anchors.centerIn: parent
            width: parent.width + 8
            height: parent.height + 8
            radius: 5
            color: "transparent"
            border.width: Theme.focusRingWidth
            border.color: Theme.focusRing
            visible: root.activeFocus
        }

        Rectangle {
            id: bar
            anchors.centerIn: parent
            width: parent.width
            height: parent.height
            radius: 2
            color: Theme.sliderThumb
            transform: Scale {
                origin.x: bar.width / 2
                origin.y: bar.height / 2
                xScale: root.vertical && root.pressed ? 1.35 : 1
                yScale: root.horizontal && root.pressed ? 1.35 : 1

                Behavior on xScale {
                    enabled: !root.reduceMotion
                    SpringAnimation {
                        spring: 2.4
                        damping: 0.15
                        epsilon: 0.001
                    }
                }

                Behavior on yScale {
                    enabled: !root.reduceMotion
                    SpringAnimation {
                        spring: 2.4
                        damping: 0.15
                        epsilon: 0.001
                    }
                }
            }
        }
    }
}
