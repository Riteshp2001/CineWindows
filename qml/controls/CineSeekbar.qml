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
import CineWindows

Item {
    id: root

    // Public API
    /// Responsive metrics owned by the containing playback window
    required property ViewportMetrics metrics
    property real from: 0 // Minimum value (start of range)
    property real maximum: 1 // Maximum value (end of range)
    property real value: 0 // Current playback position in seconds
    property bool pressed: false // Whether seek handle is being dragged
    property bool hoverEnabled: true // Whether hover interactions are enabled
    property var player // Reference to the player object for duration/chapter data
    property var chapters // Chapter model for snap-to-chapter behavior
    property int chapterSnapDistance: metrics.spacingSm + metrics.spacingXs // Pixels within which a chapter marker will snap
    property color trackColor: Theme.seekTrack // Background track line color
    property color fillColor: Theme.seekFill // Filled (progress) track color
    property color handleColor: Theme.seekHandle // Seek handle color
    property int trackHeight: Math.max(3, Math.round(5 * metrics.visualScale)) // Visual height of the track bar
    property int handleSize: Math.max(8, Math.round(10 * metrics.visualScale)) // Diameter of the seek handle

    signal moved(real position) // Emitted when user seeks to a new position

    readonly property bool hovered: mouseArea.containsMouse // True when mouse is over the seekbar
    readonly property bool _hovered: hovered // Internal alias for hovered
    readonly property bool seekActive: hovered || pressed // True during any seek interaction
    readonly property real availableWidth: Math.max(0, width - leftPadding - rightPadding) // Usable track width after padding
    readonly property real leftPadding: metrics.spacingSm // Left inset for the track
    readonly property real rightPadding: metrics.spacingSm // Right inset for the track
    readonly property real effectiveValue: pressed ? dragValue : value // Value to display (preview during drag, actual otherwise)
    readonly property real visualPosition: maximum > from // Normalized position 0..1
        ? Math.max(0, Math.min(1, (effectiveValue - from) / (maximum - from)))
        : 0
    readonly property real railProgressX: visualPosition * availableWidth // Current fill width in pixels

    property real dragValue: value // Temporary value during drag operations
    property real hoverRailX: 0 // Marker X position during hover

    function railXForPointer(pointerX) { // Convert pointer x to rail-local x
        return Math.max(0, Math.min(availableWidth, pointerX - leftPadding));
    }

    function sliderXForRail(railX) { // Convert rail-local x to global x
        return leftPadding + railX;
    }

    function valueForRail(railX) { // Convert rail x position to playback value
        var ratio = railX / Math.max(1, availableWidth); // Normalized position within rail
        var val = from + ratio * (maximum - from);
        return Math.max(from, Math.min(maximum, val)); // Clamp to valid range
    }

    function railXForValue(position) { // Convert playback value to rail x position
        if (maximum <= from)
            return 0;
        var ratio = (position - from) / (maximum - from); // Normalized position within range
        return Math.max(0, Math.min(availableWidth, ratio * availableWidth));
    }

    function snapToNearbyChapter(candidateValue, railX) { // Snap value to nearest chapter marker within snap distance
        if (!chapters || chapters.count <= 0 || maximum <= from || availableWidth <= 0)
            return candidateValue;

        var snappedValue = candidateValue; // Best candidate after snapping
        var nearestDistance = chapterSnapDistance + 1; // Closest snap distance found
        for (var i = 0; i < chapters.count; ++i) { // Loop index over chapters
            var chapterTime = Number(chapters.at(i).time); // Current chapter's time in seconds
            if (isNaN(chapterTime) || chapterTime <= from || chapterTime >= maximum)
                continue;

            var chapterX = ((chapterTime - from) / (maximum - from)) * availableWidth; // Chapter marker x position
            var distance = Math.abs(chapterX - railX); // Distance from pointer to chapter marker
            if (distance <= chapterSnapDistance && distance < nearestDistance) {
                nearestDistance = distance;
                snappedValue = chapterTime;
            }
        }
        return snappedValue;
    }

    function snappedPositionForPointer(pointerX) { // Get snapped value from pointer position
        var railX = railXForPointer(pointerX); // Rail-local x from pointer
        return snapToNearbyChapter(valueForRail(railX), railX);
    }

    function updateHoverPosition(pointerX) { // Update hover indicator and return snapped position
        var position = snappedPositionForPointer(pointerX); // Snapped position from pointer
        hoverRailX = railXForValue(position);
        return position;
    }

    function moveTo(pointerX) { // Execute seek to pointer position
        dragValue = updateHoverPosition(pointerX);
        moved(dragValue);
    }

    implicitHeight: metrics.controlCompact

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: root.hoverEnabled
        cursorShape: Qt.PointingHandCursor

        onPressed: function(mouse) {
            root.pressed = true;
            root.moveTo(mouse.x);
            mouse.accepted = true;
        }

        onPositionChanged: function(mouse) {
            if (root.pressed)
                root.moveTo(mouse.x);
            else
                root.updateHoverPosition(mouse.x);
        }

        onReleased: function(mouse) {
            root.moveTo(mouse.x);
            root.pressed = false;
        }

        onCanceled: root.pressed = false
    }

    Item {
        id: rail
        x: root.leftPadding
        anchors.verticalCenter: parent.verticalCenter
        width: root.availableWidth
        height: Math.max(root.metrics.iconSm, root.trackHeight)

        Rectangle {
            id: track
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width
            height: root.trackHeight
            radius: height / 2
            color: root.trackColor
            opacity: root.seekActive ? 0.82 : 0.62

            Behavior on opacity {
                NumberAnimation {
                    duration: Theme.motionFast
                    easing.type: Easing.OutCubic
                }
            }
        }

        Rectangle {
            anchors.verticalCenter: track.verticalCenter
            width: root.railProgressX
            height: track.height
            radius: height / 2
            color: root.fillColor
        }

        Repeater {
            model: root.chapters ? root.chapters : null

            delegate: Rectangle {
                required property real time

                visible: root.player && root.player.duration > 0 && time > 0 && time < root.player.duration
                width: Math.max(1, Math.round(2 * root.metrics.visualScale))
                height: root.seekActive
                    ? root.metrics.spacingSm + root.metrics.spacingXs
                    : root.metrics.spacingSm
                radius: 1
                color: Theme.chapterMarker
                x: Math.max(0, Math.min(parent.width - width, (time / root.maximum) * parent.width - width / 2))
                anchors.verticalCenter: parent.verticalCenter
                opacity: root.seekActive ? 0.8 : 0.45

                Behavior on height {
                    NumberAnimation {
                        duration: Theme.motionFast
                        easing.type: Easing.OutCubic
                    }
                }

                Behavior on opacity {
                    NumberAnimation {
                        duration: Theme.motionFast
                        easing.type: Easing.OutCubic
                    }
                }
            }
        }

        Rectangle {
            visible: root._hovered && !root.pressed
            x: root.hoverRailX - width / 2
            anchors.verticalCenter: parent.verticalCenter
            width: 1
            height: root.metrics.iconXs
            color: Theme.seekHandle
            opacity: 0.72
        }
    }

    Rectangle {
        x: root.leftPadding + root.railProgressX - width / 2
        anchors.verticalCenter: parent.verticalCenter
        width: root.handleSize
        height: width
        radius: width / 2
        color: root.handleColor
        opacity: root.seekActive ? 1 : 0
        scale: root.pressed ? 1.15 : 1

        Behavior on opacity {
            NumberAnimation {
                duration: Theme.motionFast
                easing.type: Easing.OutCubic
            }
        }

        Behavior on scale {
            NumberAnimation {
                duration: Theme.motionFast
                easing.type: Easing.OutCubic
            }
        }
    }
}
