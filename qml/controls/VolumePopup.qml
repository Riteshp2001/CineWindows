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

Popup {
    id: root

    /// Responsive metrics owned by the containing playback window
    required property ViewportMetrics metrics
    /// Reference to the active media player instance
    property var player
    /// Playback controller interface
    property var controller
    /// Timestamp of the last close event, used to prevent immediate reopen
    property double closedAt: 0
    /// Minimum edge margin from screen boundaries
    property int edgeMargin: metrics.spacingSm
    /// Gap between the anchor item and the popup
    property int popupGap: metrics.spacingSm
    /// The button item this popup is anchored to
    property var anchorItem: null
    /// X position of the pointer arrow, calculated from anchor position
    property real pointerX: _anchoredPointerX()

    /// Records the close timestamp when the popup is dismissed
    onClosed: closedAt = Date.now()

    width: Math.max(160, Math.round(180 * metrics.visualScale))
    height: metrics.controlCompact + metrics.spacingSm + metrics.spacingMd
    x: _anchoredX()
    y: _anchoredY()
    leftPadding: metrics.spacingSm + metrics.spacingXs
    rightPadding: metrics.spacingSm + metrics.spacingXs
    topPadding: metrics.spacingSm
    bottomPadding: metrics.spacingMd
    transformOrigin: Item.Bottom

    /// Returns the volume label text for display
    /// @returns "Muted", "0%", or the current volume percentage string
    function volumeLabel() {
        if (!player)
            return "0%";
        if (player.mute)
            return qsTr("Muted");
        return Math.round(player.volume) + "%";
    }

    /// Checks whether the popup was closed recently (within 250ms)
    /// @returns true if the popup was closed less than 250ms ago
    function justClosed() {
        return (Date.now() - closedAt) < 250;
    }

    /// Calculates the center X of the anchor item in parent coordinates
    /// @returns Center X coordinate in the parent's coordinate space
    function anchorCenterX() {
        if (!anchorItem || !parent)
            return width / 2;
        return anchorItem.mapToItem(parent, anchorItem.width / 2, anchorItem.height / 2).x;
    }

    /// Calculates the top Y of the anchor item in parent coordinates
    /// @returns Top Y coordinate in the parent's coordinate space
    function anchorTopY() {
        if (!anchorItem || !parent)
            return 0;
        return anchorItem.mapToItem(parent, anchorItem.width / 2, 0).y;
    }

    function _anchoredX() {
        if (!anchorItem || !parent)
            return 0;
        var parentPos = parent.mapToItem(null, 0, 0);
        var win = parent.Window.window;
        var winWidth = win ? win.width : parent.width;
        var minX = -parentPos.x + edgeMargin;
        var maxX = winWidth - parentPos.x - width - edgeMargin;
        var targetX = anchorCenterX() - width / 2;
        return Math.round(Math.max(minX, Math.min(maxX, targetX)));
    }

    function _anchoredY() {
        if (!anchorItem || !parent)
            return 0;
        var parentPos = parent.mapToItem(null, 0, 0);
        var win = parent.Window.window;
        var winHeight = win ? win.height : parent.height;
        var minY = -parentPos.y + edgeMargin;
        var maxY = winHeight - parentPos.y - height - edgeMargin;
        var targetY = anchorTopY() - height - popupGap;
        return Math.round(Math.max(minY, Math.min(maxY, targetY)));
    }

    function _anchoredPointerX() {
        if (!anchorItem || !parent)
            return width / 2;
        var localArrowX = anchorCenterX() - x;
        return Math.max(metrics.controlCompact, Math.min(width - metrics.controlCompact, localArrowX));
    }

    /// Opens the popup positioned above the given anchor item
    /// @param anchor The button item to anchor above
    /// @param boundsItem Optional container item to use as parent/bounds
    function openAbove(anchor, boundsItem) {
        if (justClosed())
            return;
        anchorItem = anchor;
        if (boundsItem)
            parent = boundsItem;
        x = Qt.binding(function () { return root._anchoredX(); });
        y = Qt.binding(function () { return root._anchoredY(); });
        pointerX = Qt.binding(function () { return root._anchoredPointerX(); });
        open();
    }

    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: Theme.motionFast
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            property: "scale"
            from: 0.96
            to: 1.0
            duration: Theme.motionFast
            easing.type: Easing.OutCubic
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1.0
            to: 0.0
            duration: Theme.motionFast
            easing.type: Easing.InCubic
        }
    }

    background: Rectangle {
        color: "transparent"

        Rectangle {
            anchors.fill: parent
            anchors.bottomMargin: root.metrics.spacingSm + root.metrics.spacingXs
            color: Theme.popover
            opacity: 0.95
            border.color: Theme.popoverBorder
            border.width: 1
            radius: root.metrics.popupRadius
        }

        Rectangle {
            width: root.metrics.iconXs
            height: width
            x: Math.max(root.metrics.controlCompact,
                        Math.min(parent.width - root.metrics.controlCompact - width,
                                 root.pointerX - width / 2))
            y: parent.height - root.metrics.spacingMd
            z: -1
            rotation: 45
            radius: 1
            color: Theme.popover
            opacity: 0.95
        }
    }

    contentItem: Row {
        width: root.availableWidth
        height: root.metrics.controlCompact
        spacing: root.metrics.spacingSm + root.metrics.spacingXs

        CineButton {
            id: muteButton
            anchors.verticalCenter: parent.verticalCenter
            styleVariant: "icon"
            metrics: root.metrics
            iconName: root.player && root.player.mute ? "cine-volume-mute-symbolic" : "cine-volume-max-symbolic"
            btnTooltip: qsTr("Mute")
            onClicked: if (root.controller)
                root.controller.toggleMute()
        }

        Item {
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(0, parent.width - muteButton.width - parent.spacing)
            height: root.metrics.controlCompact

            CineRangeSlider {
                id: volumeSlider
                anchors.fill: parent
                focusPolicy: Qt.NoFocus
                Accessible.name: qsTr("Volume")
                from: 0
                to: 200
                stepSize: 1
                markerValue: 100
                value: root.player ? root.player.volume : 100
                /// Sets the player volume and unmutes if muted and volume > 0
                onMoved: {
                    if (!root.player)
                        return;
                    root.player.volume = value;
                    if (value > 0 && root.player.mute)
                        root.player.mute = false;
                }
            }
        }
    }
}
