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

Menu {
    id: root
    popupType: Popup.Item
    /// Responsive metrics for the menu viewport and visual scale
    property ViewportMetrics metrics: ViewportMetrics {
        viewportWidth: root.parent ? root.parent.width : 1280
        viewportHeight: root.parent ? root.parent.height : 720
    }
    /// Width of the menu popup
    property int menuWidth: 236
    /// Whether the pointer arrow is visible on the menu
    property bool pointerVisible: true
    /// Whether the pointer arrow is positioned at the bottom edge
    property bool pointerAtBottom: false
    /// X position of the pointer arrow relative to menu width
    property real pointerX: menuWidth / 2
    /// Minimum edge margin from screen boundaries
    property int edgeMargin: 8
    /// Gap between the anchor item and the menu popup
    property int popupGap: 10
    /// The item this menu is anchored to
    property var anchorItem: null
    /// Placement direction: "above" or "below"
    property string placementMode: ""

    readonly property real maximumWidth: parent
        ? Math.max(0, parent.width - 2 * edgeMargin)
        : menuWidth
    readonly property real maximumHeight: parent
        ? Math.max(0, parent.height - 2 * edgeMargin)
        : menuContentHeight + topPadding + bottomPadding
    readonly property real menuContentHeight: {
        let total = 0;
        for (let index = 0; index < count; ++index) {
            const item = itemAt(index);
            if (item && item.visible)
                total += item.implicitHeight;
        }
        return total;
    }

    implicitWidth: Math.min(menuWidth, maximumWidth)
    implicitHeight: Math.min(
        menuContentHeight + topPadding + bottomPadding,
        maximumHeight)

    // Toggle support: when a menu is open and its trigger button is clicked again,
    // the menu first dismisses itself (close-on-press-outside) and then the button's
    // onClicked fires. Without this guard it would immediately reopen. Record the
    // close time and refuse to reopen within a short window -> second click closes.
    /// Timestamp of the last close event, used to prevent immediate reopen
    property double _closedAt: 0
    /// Records the close timestamp to enable the reopen guard
    onClosed: _closedAt = Date.now()
    /// Checks whether the menu was closed recently (within 250ms)
    /// @returns true if the menu was closed less than 250ms ago
    function _justClosed() {
        return (Date.now() - _closedAt) < 250;
    }

    /// Clamps a value within the specified range
    /// @param value The value to clamp
    /// @param minValue Minimum allowed value
    /// @param maxValue Maximum allowed value
    /// @returns Clamped value between minValue and maxValue
    function _clamp(value, minValue, maxValue) {
        return Math.max(minValue, Math.min(maxValue, value));
    }

    /// Calculates the center X of the anchor item in parent coordinates
    /// @returns Center X coordinate in the parent's coordinate space
    function _anchorCenterX() {
        if (!anchorItem || !parent)
            return width / 2;
        return anchorItem.mapToItem(parent, anchorItem.width / 2, anchorItem.height / 2).x;
    }

    /// Calculates the top Y of the anchor item in parent coordinates
    /// @returns Top Y coordinate in the parent's coordinate space
    function _anchorTopY() {
        if (!anchorItem || !parent)
            return 0;
        return anchorItem.mapToItem(parent, anchorItem.width / 2, 0).y;
    }

    /// Calculates the bottom Y of the anchor item in parent coordinates
    /// @returns Bottom Y coordinate in the parent's coordinate space
    function _anchorBottomY() {
        if (!anchorItem || !parent)
            return 0;
        return anchorItem.mapToItem(parent, anchorItem.width / 2, anchorItem.height).y;
    }

    /// Checks whether the menu fits above the anchor item
    /// @returns true if there is enough space above the anchor
    function _fitsAbove() {
        return parent && (_anchorTopY() - height - popupGap) >= edgeMargin;
    }

    /// Checks whether the menu fits below the anchor item
    /// @returns true if there is enough space below the anchor
    function _fitsBelow() {
        return parent && (_anchorBottomY() + height + popupGap) <= (parent.height - edgeMargin);
    }

    /// Calculates the optimal X position for the menu within parent bounds
    /// @returns Clamped X coordinate in parent space
    function _anchoredX() {
        if (!parent)
            return 0;
        return Math.round(_clamp(_anchorCenterX() - width / 2, edgeMargin, Math.max(edgeMargin, parent.width - width - edgeMargin)));
    }

    /// Calculates the optimal Y position preferring placementMode direction
    /// @returns Clamped Y coordinate in parent space
    function _anchoredY() {
        if (!parent)
            return 0;
        var aboveY = _anchorTopY() - height - popupGap;
        var belowY = _anchorBottomY() + popupGap;
        var preferAbove = placementMode === "above" && (_fitsAbove() || !_fitsBelow());
        var wantedY = preferAbove ? aboveY : belowY;
        return Math.round(_clamp(wantedY, edgeMargin, Math.max(edgeMargin, parent.height - height - edgeMargin)));
    }

    /// Calculates the pointer arrow X position relative to the menu
    /// @returns Clamped pointer X within menu bounds
    function _anchoredPointerX() {
        return _clamp(_anchorCenterX() - x, 18, Math.max(18, width - 18));
    }

    /// Opens the menu anchored to the given item with the specified placement
    /// @param anchor The item to anchor the menu to
    /// @param boundsItem Optional bounds parent for position clamping
    /// @param placement Direction string "above" or "below"
    function _openAnchored(anchor, boundsItem, placement) {
        if (_justClosed())
            return;
        anchorItem = anchor;
        placementMode = placement;
        parent = boundsItem ? boundsItem : anchor;
        pointerVisible = true;
        pointerAtBottom = Qt.binding(function () {
            return root.placementMode === "above" && (root._fitsAbove() || !root._fitsBelow());
        });
        x = Qt.binding(function () {
            return root._anchoredX();
        });
        y = Qt.binding(function () {
            return root._anchoredY();
        });
        pointerX = Qt.binding(function () {
            return root._anchoredPointerX();
        });
        open();
    }

    /// Opens the menu positioned below the anchor item
    /// @param anchor The item to anchor below
    /// @param boundsItem Optional bounds parent for position clamping
    function openBelow(anchor, boundsItem) {
        _openAnchored(anchor, boundsItem, "below");
    }

    /// Opens the menu below the anchor (left-aligned variant, delegates to openBelow)
    /// @param anchor The item to anchor below
    /// @param boundsItem Optional bounds parent for position clamping
    function openBelowLeft(anchor, boundsItem) {
        openBelow(anchor, boundsItem);
    }

    /// Opens the menu below the anchor (right-aligned variant, delegates to openBelow)
    /// @param anchor The item to anchor below
    /// @param boundsItem Optional bounds parent for position clamping
    function openBelowRight(anchor, boundsItem) {
        openBelow(anchor, boundsItem);
    }

    /// Opens the menu positioned above the anchor item
    /// @param anchor The item to anchor above
    /// @param boundsItem Optional bounds parent for position clamping
    function openAbove(anchor, boundsItem) {
        _openAnchored(anchor, boundsItem, "above");
    }

    contentItem: ListView {
        objectName: "menuListView"
        implicitHeight: root.menuContentHeight
        model: root.contentModel
        currentIndex: root.currentIndex
        clip: true
        interactive: contentHeight > height
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }
    }

    background: Rectangle {
        implicitWidth: root.menuWidth
        color: "transparent"

        Rectangle {
            anchors.fill: parent
            anchors.margins: 4
            anchors.bottomMargin: 6
            radius: 12
            color: Theme.popoverShadow
            opacity: 0.5
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: 4
            color: Theme.popover
            opacity: 0.95
            border.color: Theme.popoverBorder
            border.width: 1
            radius: 12
        }

        Rectangle {
            width: 12
            height: 12
            x: Math.max(18, Math.min(root.menuWidth - 30, root.pointerX - width / 2))
            y: root.pointerAtBottom ? parent.height - 10 : 2
            z: -1
            rotation: 45
            radius: 1
            color: Theme.popover
            opacity: 0.95
            visible: root.pointerVisible
        }
    }

    topPadding: 10
    bottomPadding: 10
    leftPadding: 7
    rightPadding: 7
}
