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
import CineWindows

Rectangle {
    id: root
    required property ViewportMetrics metrics   // viewport scaling and metrics provider

    /// Whether the drop zone is active (files are being dragged over)
    property bool active: false
    /// Name of the icon displayed in the center
    property string iconName: ""
    /// Label text shown below the icon
    property string label: ""

    radius: Math.round(Theme.dropRadius * metrics.visualScale)
    color: Theme.dropBg
    visible: opacity > 0
    opacity: active ? 1 : 0
    border.width: 0

    Behavior on opacity {
        NumberAnimation {
            duration: Theme.motionPanel
            easing.type: Easing.OutCubic
        }
    }

    Canvas {
        id: dropDash
        anchors.fill: parent
        antialiasing: true
        onWidthChanged: requestPaint()           // repaint on resize
        onHeightChanged: requestPaint()          // repaint on resize
        onPaint: {
            var context = getContext("2d");      // canvas rendering context
            context.clearRect(0, 0, width, height);
            context.lineWidth = Math.max(1, Math.round(2 * root.metrics.visualScale));
            context.strokeStyle = Theme.dropBorder;
            context.setLineDash([Math.round(12 * root.metrics.visualScale), root.metrics.spacingSm]);
            context.beginPath();

            var radius = root.radius;            // corner radius for rounded rect
            var posX = 1;                        // horizontal inset for border
            var posY = 1;                        // vertical inset for border
            var drawWidth = width - 2;           // draw width minus border insets
            var drawHeight = height - 2;         // draw height minus border insets

            // draw rounded-rect path
            context.moveTo(posX + radius, posY);
            context.lineTo(posX + drawWidth - radius, posY);
            context.quadraticCurveTo(posX + drawWidth, posY, posX + drawWidth, posY + radius);
            context.lineTo(posX + drawWidth, posY + drawHeight - radius);
            context.quadraticCurveTo(posX + drawWidth, posY + drawHeight, posX + drawWidth - radius, posY + drawHeight);
            context.lineTo(posX + radius, posY + drawHeight);
            context.quadraticCurveTo(posX, posY + drawHeight, posX, posY + drawHeight - radius);
            context.lineTo(posX, posY + radius);
            context.quadraticCurveTo(posX, posY, posX + radius, posY);
            context.stroke();
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: Math.round(12 * root.metrics.visualScale)

        CineIcon {
            objectName: "dropIcon"
            anchors.horizontalCenter: parent.horizontalCenter
            width: root.metrics.overlayIconSize
            height: root.metrics.overlayIconSize
            name: root.iconName
            tint: Theme.accent
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.label
            color: Theme.accent
            font.pixelSize: root.metrics.fontDisplay
            font.bold: true
            style: Text.Outline
            styleColor: Theme.textOutlineStrong
        }
    }
}
