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
import QtQuick.Window
import CineWindows

Item {
    id: root
    /// Tooltip text content
    property string text: ""
    /// Whether the tooltip is currently visible
    property bool active: false
    /// When true, tooltip appears below the parent; otherwise above
    property bool below: false
    /// Item whose dimensions constrain tooltip placement, normally the window content item
    property Item boundsItem: root.Window.window ? root.Window.window.contentItem : null
    /// Minimum distance retained from each edge of boundsItem
    property int edgeMargin: 8
    /// Visual item the popup follows; controls should set this explicitly.
    property Item anchorItem: parent
    /// Window overlay used to escape clipping ancestors
    readonly property Item overlayItem: root.Overlay.overlay
    property int _positionRevision: 0

    visible: tooltipPopup.visible
    width: tooltipPopup.width
    height: tooltipPopup.height
    x: anchorItem && overlayItem
        ? Math.round(anchorItem.mapFromItem(
            overlayItem, _popupX(width, _positionRevision), _popupY(height, _positionRevision)).x)
        : 0
    y: anchorItem && overlayItem
        ? Math.round(anchorItem.mapFromItem(
            overlayItem, _popupX(width, _positionRevision), _popupY(height, _positionRevision)).y)
        : 0

    onActiveChanged: {
        if (active)
            Qt.callLater(root._refreshPosition);
    }

    function _refreshPosition() {
        _positionRevision += 1;
    }

    function _clamp(value, minimum, maximum) {
        return Math.max(minimum, Math.min(maximum, value));
    }

    function _boundsOrigin() {
        if (!boundsItem || !overlayItem)
            return Qt.point(0, 0);
        return boundsItem.mapToItem(overlayItem, 0, 0);
    }

    function _popupX(popupWidth, revision) {
        if (!anchorItem || !overlayItem)
            return 0;

        anchorItem.x;
        anchorItem.y;
        anchorItem.width;
        const anchorCenter = anchorItem.mapToItem(
            overlayItem, anchorItem.width / 2, anchorItem.height / 2).x;
        if (!boundsItem)
            return Math.round(anchorCenter - popupWidth / 2);

        boundsItem.x;
        boundsItem.y;
        boundsItem.width;
        const origin = _boundsOrigin();
        const minimum = origin.x + edgeMargin;
        const maximum = Math.max(
            minimum, origin.x + boundsItem.width - edgeMargin - popupWidth);
        return Math.round(_clamp(anchorCenter - popupWidth / 2, minimum, maximum));
    }

    function _popupY(popupHeight, revision) {
        if (!anchorItem || !overlayItem)
            return 0;

        anchorItem.x;
        anchorItem.y;
        anchorItem.height;
        const anchorTop = anchorItem.mapToItem(overlayItem, 0, 0).y;
        const anchorBottom = anchorItem.mapToItem(overlayItem, 0, anchorItem.height).y;
        const above = anchorTop - popupHeight - 8;
        const belowAnchor = anchorBottom + 8;
        var preferred = below ? belowAnchor : above;
        if (!boundsItem)
            return Math.round(preferred);

        boundsItem.x;
        boundsItem.y;
        boundsItem.height;
        const origin = _boundsOrigin();
        const minimum = origin.y + edgeMargin;
        const maximum = Math.max(
            minimum, origin.y + boundsItem.height - edgeMargin - popupHeight);
        if (below && belowAnchor > maximum && above >= minimum)
            preferred = above;
        else if (!below && above < minimum && belowAnchor <= maximum)
            preferred = belowAnchor;
        return Math.round(_clamp(preferred, minimum, maximum));
    }

    Popup {
        id: tooltipPopup
        objectName: "tooltipPopup"
        parent: root.overlayItem
        popupType: Popup.Item
        modal: false
        dim: false
        focus: false
        closePolicy: Popup.NoAutoClose
        padding: 0
        z: 10000

        visible: root.active && root.text.length > 0 && parent !== null
        width: root.boundsItem
            ? Math.min(label.implicitWidth + 18,
                       Math.max(0, root.boundsItem.width - root.edgeMargin * 2))
            : label.implicitWidth + 18
        height: 30
        x: root._popupX(width, root._positionRevision)
        y: root._popupY(height, root._positionRevision)

        enter: Transition {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: Theme.motionFast
                easing.type: Easing.OutCubic
            }
        }

        exit: Transition {
            NumberAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: Theme.motionFast
                easing.type: Easing.OutCubic
            }
        }

        background: Rectangle {
            radius: 7
            color: Theme.popover
            border.color: Theme.tooltipBorder
            border.width: 1
            opacity: 0.98
        }

        contentItem: Text {
            id: label
            text: root.text
            color: Theme.text
            font.pixelSize: 12
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.NoWrap
            elide: Text.ElideRight
            maximumLineCount: 1
            Accessible.role: Accessible.ToolTip
            Accessible.name: root.text
        }
    }
}
