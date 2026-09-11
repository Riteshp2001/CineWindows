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
import QtQuick.Controls
import CineWindows

AbstractButton {
    id: root

    enum SizeRole {
        CompactSize,
        StandardSize,
        ProminentSize
    }

    enum WindowControlRole {
        NoWindowControl,
        MenuControl,
        MinimizeControl,
        MaximizeControl,
        RestoreControl,
        CloseControl
    }

    /* ---- Style ---- */
    property string styleVariant: "icon"
    property string colorVariant: "default"

    /* ---- Content ---- */
    property string iconName: ""
    property string btnText: ""
    property string btnTooltip: ""
    property string glyph: ""
    property color glyphColor: Theme.text
    property color windowControlTint: Theme.text
    property bool filledIcon: false

    /* ---- Sizing ---- */
    property ViewportMetrics metrics
    property int sizeRole: CineButton.CompactSize
    property int buttonSize: 0
    property int iconSize: 0
    property bool playerControl: false

    /* ---- Window controls ---- */
    property int windowControlRole: CineButton.NoWindowControl
    property bool tooltipBelow: false

    /* ---- State ---- */
    property color checkedBackground: Theme.isLight ? Theme.accent : "white"
    property color checkedIconTint: "white"

    /* ---- Computed ---- */
    readonly property bool _isIcon: styleVariant === "icon"
    readonly property bool _isPill: styleVariant === "pill"
    readonly property bool _isDialog: styleVariant === "dialog"
    readonly property bool _isText: styleVariant === "text"
    readonly property bool _isGlyph: glyph.length > 0
    readonly property bool _isWindowCtrl: windowControlRole !== CineButton.NoWindowControl
    readonly property bool _isClose: styleVariant === "close"
                                     || windowControlRole === CineButton.CloseControl
    readonly property int _semanticButtonSize: sizeRole === CineButton.ProminentSize
        ? (metrics ? metrics.controlProminent : Theme.controlProminent)
        : sizeRole === CineButton.StandardSize
          ? (metrics ? metrics.controlStandard : Theme.controlStandard)
          : (metrics ? metrics.controlCompact : Theme.controlCompact)
    readonly property int _semanticIconSize: sizeRole === CineButton.ProminentSize
        ? (metrics ? metrics.iconLg : Theme.iconLg)
        : sizeRole === CineButton.StandardSize
          ? (metrics ? metrics.iconMd : Theme.iconMd)
          : (metrics ? metrics.iconSm : Theme.iconSm)
    readonly property int _effectiveButtonSize: buttonSize > 0 ? buttonSize : _semanticButtonSize
    readonly property int _requestedIconSize: iconSize > 0 ? iconSize : _semanticIconSize
    readonly property int _effectiveIconSize: playerControl
        ? Math.max(18, _requestedIconSize)
        : _requestedIconSize
    readonly property int _windowGlyphSize: Math.max(8, Math.round(_effectiveIconSize * 0.625))
    readonly property real _windowStrokeWidth: Math.max(1, _effectiveIconSize / 12)

    implicitWidth: root._isPill ? Math.max(96, pillLabel.implicitWidth + 34)
                : root._isDialog ? 92
                : root._isText ? 88
                : _effectiveButtonSize
    implicitHeight: root._isPill ? 38
                 : root._isDialog ? 38
                 : root._isText ? 34
                 : _effectiveButtonSize
    leftPadding: root._isPill ? 17 : 0
    rightPadding: root._isPill ? 17 : 0

    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    HoverHandler { cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor }
    scale: (root.down && !root._isText && root.styleVariant !== "close") ? (root._isPill ? Theme.pillActiveScale : 0.92)
         : (!root._isText && (root.hovered && root.styleVariant !== "dialog" && root.styleVariant !== "windowCtrl" && root.styleVariant !== "close")) ? 1.04
         : 1.0

    Behavior on scale {
        NumberAnimation { duration: Theme.motionFast; easing.type: Easing.OutCubic }
    }

    Accessible.name: btnTooltip.length > 0 ? btnTooltip : btnText

    /* ---- Tooltip ---- */
    CineTooltip {
        objectName: "buttonTooltip"
        anchorItem: root
        text: root.btnTooltip.length > 0 ? root.btnTooltip : root.btnText
        active: root.hovered && (root._isIcon || root._isWindowCtrl || root._isClose)
        below: root.tooltipBelow
    }

    /* ---- Background ---- */
    background: Rectangle {
        id: bg
        anchors.fill: parent
        anchors.leftMargin: root._isDialog ? 0 : 0
        anchors.rightMargin: root._isDialog ? 0 : 0
        radius: root._isClose ? width / 2
              : root._isPill ? height / 2
              : root._isWindowCtrl ? width / 2
              : root._isDialog ? Theme.shapeSmall
              : root._isText ? Theme.shapeSmall
              : Theme.shapeMedium

        color: {
            if (root._isPill) {
                if (root.colorVariant === "accentAction")
                    return Qt.alpha(Theme.accent, root.down ? 0.34 : root.hovered ? 0.26 : 0.18);
                if (root.colorVariant === "primary")
                    return root.hovered ? Theme.pillPrimaryHover : Theme.pillPrimary;
                return root.hovered ? Theme.pillSecondaryHover : Theme.pillSecondary;
            }
            if (root._isClose) {
                return root.down ? "#cc5555" : (root.hovered ? Theme.closeHover : "transparent");
            }
            if (root.checked && root._isIcon) {
                if (root.down) return Theme.toggleCheckedActiveGlow;
                if (root.hovered) return Qt.lighter(root.checkedBackground, 1.15);
                return root.checkedBackground;
            }
            if (root._isDialog) {
                if (root.colorVariant === "primary")
                    return !enabled ? Theme.cardStrong : (root.down ? "#1a5fb4" : (root.hovered ? "#2b70c5" : "#3584e4"));
                return root.down ? Theme.glassActive : (root.hovered ? Theme.glassHover : "transparent");
            }
            if (root._isText) {
                return root.down ? Theme.glassActive : (root.hovered ? Theme.glassHover : "transparent");
            }
            if (root._isWindowCtrl) {
                return root.down ? Theme.glassActive : (root.hovered ? Theme.glassHover : "transparent");
            }
            if (root.playerControl && root.colorVariant === "danger" && root.down)
                return Qt.darker(Theme.danger, 1.18);
            if (root.playerControl && root.colorVariant === "danger" && root.hovered)
                return Theme.danger;
            if (root.playerControl && root.down) return "#40ffffff";
            if (root.playerControl && root.hovered) return "#28ffffff";
            if (root.down) return "#3a000000";
            if (root.hovered) return "#24000000";
            return "transparent";
        }

        Behavior on color {
            ColorAnimation { duration: Theme.motionFast; easing.type: Easing.OutCubic }
        }

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            border.width: root.checked && root._isIcon && (root.hovered || root.down) ? 1 : 0
            border.color: root.checked && root.down ? Theme.toggleCheckedActiveGlow : Theme.toggleCheckedHoverGlow
            visible: root.checked && root._isIcon && (root.hovered || root.down)
        }

        Rectangle {
            objectName: "keyboardFocusRing"
            anchors.fill: parent
            anchors.margins: 2
            radius: Math.max(0, parent.radius - anchors.margins)
            color: "transparent"
            border.width: Theme.focusRingWidth
            border.color: Theme.focusRing
            opacity: root.visualFocus ? 1 : 0

            Behavior on opacity {
                NumberAnimation { duration: Theme.motionFast; easing.type: Easing.OutCubic }
            }
        }
    }

    /* ---- Content ---- */
    contentItem: Item {
        anchors.fill: parent

        /* Pill text */
        Text {
            id: pillLabel
            objectName: "pillLabel"
            visible: root._isPill
            anchors.centerIn: parent
            width: Math.max(0, root.width - root.leftPadding - root.rightPadding)
            height: parent.height
            text: root.btnText
            color: root.colorVariant === "accentAction" ? Theme.accent
                 : root.colorVariant === "primary" ? Theme.pillPrimaryText
                 : Theme.pillSecondaryText
            font.pixelSize: Theme.fontSizeSmall
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.NoWrap
            maximumLineCount: 1
            elide: Text.ElideRight
        }

        /* Dialog action text */
        Text {
            visible: root._isDialog
            anchors.centerIn: parent
            text: root.btnText
            color: root.colorVariant === "primary" ? (enabled ? Theme.pillPrimaryText : Theme.mutedText) : Theme.text
            font.pixelSize: 13
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        /* Text button */
        Text {
            visible: root._isText && !root._isGlyph && root.iconName.length === 0
            anchors.centerIn: parent
            text: root.btnText
            color: enabled ? Theme.text : Theme.mutedText
            font.pixelSize: 13
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        /* Glyph (+, −, ×, etc.) */
        Text {
            objectName: "printableGlyph"
            visible: root.glyph.length > 0
                     && root.windowControlRole === CineButton.NoWindowControl
            anchors.centerIn: parent
            text: root.glyph
            color: root.glyphColor
            font.pixelSize: root._effectiveIconSize
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        /* Icon button */
        CineIcon {
            objectName: "primaryIcon"
            visible: root.iconName.length > 0 && !root._isGlyph && !root._isWindowCtrl
            anchors.centerIn: parent
            width: root._effectiveIconSize
            height: root._effectiveIconSize
            name: root.iconName
            filled: root.filledIcon || root.hovered || root.down || root.checked
            tint: root.checked ? root.checkedIconTint : Theme.iconDefault
            shadowEnabled: root.playerControl
        }

        /* Window-control visuals are selected by semantic role, never glyph text. */
        Column {
            objectName: "menuControlShape"
            visible: root.windowControlRole === CineButton.MenuControl
            anchors.centerIn: parent
            spacing: Math.max(2, Math.round(root._effectiveIconSize * 0.16))
            Repeater {
                model: 3
                Rectangle {
                    width: root._windowGlyphSize
                    height: root._windowStrokeWidth
                    radius: height / 2
                    color: root.windowControlTint
                }
            }
        }
        Rectangle {
            objectName: "minimizeControlShape"
            visible: root.windowControlRole === CineButton.MinimizeControl
            anchors.centerIn: parent
            width: root._windowGlyphSize
            height: root._windowStrokeWidth
            radius: height / 2
            color: root.windowControlTint
        }
        Rectangle {
            objectName: "maximizeControlShape"
            visible: root.windowControlRole === CineButton.MaximizeControl
            anchors.centerIn: parent
            width: root._windowGlyphSize
            height: root._windowGlyphSize
            radius: Math.max(1, Math.round(root._windowGlyphSize * 0.15))
            color: "transparent"
            border.color: root.windowControlTint
            border.width: root._windowStrokeWidth
        }
        Item {
            objectName: "restoreControlShape"
            visible: root.windowControlRole === CineButton.RestoreControl
            anchors.centerIn: parent
            width: root._windowGlyphSize + Math.round(root._windowStrokeWidth * 2)
            height: width

            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                width: root._windowGlyphSize * 0.8
                height: width
                radius: Math.max(1, Math.round(root._windowGlyphSize * 0.12))
                color: "transparent"
                border.color: root.windowControlTint
                border.width: root._windowStrokeWidth
            }
            Rectangle {
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                width: root._windowGlyphSize * 0.8
                height: width
                radius: Math.max(1, Math.round(root._windowGlyphSize * 0.12))
                color: Theme.background
                border.color: root.windowControlTint
                border.width: root._windowStrokeWidth
            }
        }
        Item {
            objectName: "closeControlShape"
            visible: root.windowControlRole === CineButton.CloseControl
            anchors.centerIn: parent
            width: root._windowGlyphSize
            height: width

            Repeater {
                model: [-45, 45]
                Rectangle {
                    required property real modelData
                    anchors.centerIn: parent
                    width: root._windowGlyphSize
                    height: root._windowStrokeWidth
                    radius: height / 2
                    rotation: modelData
                    color: root.windowControlTint
                }
            }
        }
    }
}
