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

Control {
    id: root
    required property ViewportMetrics metrics  // Viewport scale/dpi metrics for HiDPI
    /// Whether the spinner animation is running
    property bool running: true  // Whether the spinner animation is running
    /// Color of the spinning arc
    property color arcColor: Theme.spinnerArc  // Color of the spinning arc
    /// Width of the spinning arc stroke in pixels
    property int arcWidth: metrics.spacingXs  // Stroke width of the spinning arc

    implicitWidth: Math.round(Theme.spinnerSize * metrics.visualScale)
    implicitHeight: Math.round(Theme.spinnerSize * metrics.visualScale)

    background: Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: Qt.rgba(0, 0, 0, 0.5)
    }

    contentItem: Canvas {
        id: canvas
        objectName: "spinnerArc"
        anchors.fill: parent
        anchors.margins: Math.round(10 * root.metrics.visualScale)
        rotation: 0

        onPaint: {
            var context = getContext("2d");  // Canvas 2D rendering context
            context.clearRect(0, 0, width, height);
            var centerX = width / 2;  // Horizontal center of canvas
            var centerY = height / 2;  // Vertical center of canvas
            var radius = Math.min(centerX, centerY) - 2;  // Arc radius with 2px padding

            context.beginPath();
            context.arc(centerX, centerY, radius, -Math.PI / 2, -Math.PI / 2 + Math.PI * 4 / 3, false);
            context.strokeStyle = root.arcColor;
            context.lineWidth = root.arcWidth;
            context.lineCap = "round";
            context.stroke();
        }

        RotationAnimator {
            target: canvas
            from: 0
            to: 360
            duration: Math.max(1, Theme.spinnerRotationDuration)
            loops: Animation.Infinite
            running: root.visible && root.running && Theme.spinnerRotationDuration > 0
        }

        onVisibleChanged: {
            if (visible) {
                rotation = 0;
                canvas.requestPaint();
            }
        }
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        Connections {
            target: root
            function onArcColorChanged() { canvas.requestPaint(); }
            function onArcWidthChanged() { canvas.requestPaint(); }
        }
    }
}
