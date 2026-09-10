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

// Full HSV colour picker matching the CineWindows "Subtitle Background" dialog:
// a hue bar, a saturation/value box with a draggable crosshair, a live preview,
// a hex field and an eyedropper button.
ResponsivePopup {
    id: root

    parent: Overlay.overlay
    metrics: ViewportMetrics {
        viewportWidth: root.parent ? root.parent.width : root.preferredWidth
        viewportHeight: root.parent ? root.parent.height : root.preferredHeight
    }
    modal: true
    dim: true
    preferredWidth: 470
    preferredHeight: 360
    padding: 0
    focus: true

    /// Dialog title shown in the header
    property string title: qsTr("Subtitle Background")
    /// Whether the alpha channel is editable and included in the result
    property bool useAlpha: true
    /// The currently composed colour (updated as sliders move)
    property color currentColor: "#ff0000"
    /// Emitted with the final colour when user presses Select
    signal accepted(color selectedColor)

    /// Hue value (0..1)
    property real hue: 0
    /// Saturation value (0..1)
    property real saturation: 1
    /// Value / brightness (0..1)
    property real value: 1
    /// Alpha channel (0..1), only used when useAlpha is true
    property real alpha: 1
    /// Guards the hex field from fighting the sliders during programmatic updates
    property bool updatingHex: false

    /// The composed colour from current HSV(A) values
    readonly property color composed: Qt.hsva(hue, saturation, value, useAlpha ? alpha : 1)
    /// Whether the action buttons are wrapped to a second line (narrow layout)
    readonly property bool actionsWrapped: actionFlow.height > 38

    /// Opens the dialog pre-selected to colour c
    function openFor(c) {
        setFromColor(c);
        open();
    }

    /// Sets all HSV(A) properties from a given colour value
    function setFromColor(c) {
        var col = Qt.color(c); // resolved colour object to extract HSV channels
        hue = col.hsvHue >= 0 ? col.hsvHue : 0;
        saturation = col.hsvSaturation;
        value = col.hsvValue;
        alpha = col.a;
        syncHexField();
    }

    /// Converts a colour to a hex string (with alpha prefix if useAlpha and alpha < 1)
    function toHex(c) {
        /// Converts a 0..1 channel to a two-digit hex string
        function toHexPair(value) {
            var s = Math.round(value * 255).toString(16); // 0-255 hex string
            return s.length < 2 ? "0" + s : s;
        }
        var col = Qt.color(c); // resolved colour for channel extraction
        if (useAlpha && alpha < 1)
            return ("#" + toHexPair(col.a) + toHexPair(col.r) + toHexPair(col.g) + toHexPair(col.b)).toUpperCase();
        return ("#" + toHexPair(col.r) + toHexPair(col.g) + toHexPair(col.b)).toUpperCase();
    }

    /// Syncs the hex text field with the current composed colour
    function syncHexField() {
        updatingHex = true;
        hexField.text = toHex(composed);
        updatingHex = false;
    }

    onComposedChanged: if (!updatingHex)
        syncHexField()

    background: Rectangle {
        radius: 16
        color: Theme.popover
        opacity: 0.97
        border.color: Theme.popoverBorder
        border.width: 1
    }

    // --- Header ---
    Item {
        id: headerBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 50
        Text {
            anchors.centerIn: parent
            text: root.title
            color: Theme.text
            font.pixelSize: 17
            font.bold: true
        }
        CineButton {
            id: closeBtn
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            styleVariant: "close"
            iconName: "cine-close-symbolic"
            btnTooltip: qsTr("Close")
            onClicked: root.close()
        }
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.separator
        }
    }

    // --- Top row: eyedropper + preview + hex ---
    Row {
        id: topRow
        anchors.top: headerBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 14
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 10
        height: 36

        // Eyedropper (decorative).
        CineButton {
            id: eyedropBtn
            styleVariant: "text"
            width: 36
            height: 36
            enabled: false
            Accessible.name: qsTr("Eyedropper")
            background: Rectangle {
                radius: 8
                color: eyedropBtn.hovered ? Theme.cardHover : Theme.cardStrong
                opacity: eyedropBtn.enabled ? 1 : 0.5
                border.width: 1
                border.color: Theme.separator
            }
            contentItem: Item {
                Rectangle {
                    anchors.centerIn: parent
                    width: 2
                    height: 16
                    radius: 1
                    color: Theme.text
                    rotation: 45
                }
                Rectangle {
                    anchors.centerIn: parent
                    anchors.horizontalCenterOffset: 5
                    anchors.verticalCenterOffset: -5
                    width: 7
                    height: 5
                    radius: 2
                    color: Theme.text
                    rotation: 45
                }
            }
            CineTooltip {
                text: qsTr("Eyedropper unavailable")
                active: eyedropBtn.hovered
            }
        }

        // Live preview chip.
        Rectangle {
            objectName: "hsvColorPreview"
            width: Math.max(0, parent.width - eyedropBtn.width - hexBox.width - parent.spacing * 2)
            height: 36
            radius: 8
            color: root.composed
            border.width: 1
            border.color: "#33ffffff"
        }

        // Hex field.
        Rectangle {
            id: hexBox
            width: Math.min(120, Math.max(72,
                topRow.width - eyedropBtn.width - topRow.spacing * 2))
            height: 36
            radius: 8
            color: Theme.cardStrong
            border.width: hexField.activeFocus ? 1 : 0
            border.color: Theme.accent
            TextField {
                id: hexField
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                verticalAlignment: Text.AlignVCenter
                color: Theme.text
                font.pixelSize: 14
                font.bold: true
                background: Item {}
                onTextEdited: {
                    var t = text.trim(); // typed hex string, stripped of whitespace
                    if (!/^#?[0-9a-fA-F]{6}([0-9a-fA-F]{2})?$/.test(t))
                        return;
                    if (t[0] !== "#")
                        t = "#" + t;
                    root.updatingHex = true;
                    root.setFromColor(t);
                    root.updatingHex = false;
                }
            }
        }
    }

    // --- Picker area: hue bar + saturation/value box ---
    Row {
        id: pickerRow
        anchors.top: topRow.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: actionFlow.top
        anchors.topMargin: 12
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 12
        spacing: 12

        // Vertical hue bar.
        Item {
            id: hueBar
            width: 22
            height: parent.height
            Rectangle {
                anchors.fill: parent
                radius: 6
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#ff0000" }
                    GradientStop { position: 0.17; color: "#ffff00" }
                    GradientStop { position: 0.33; color: "#00ff00" }
                    GradientStop { position: 0.5; color: "#00ffff" }
                    GradientStop { position: 0.67; color: "#0000ff" }
                    GradientStop { position: 0.83; color: "#ff00ff" }
                    GradientStop { position: 1.0; color: "#ff0000" }
                }
            }
            // Hue handle.
            Rectangle {
                width: parent.width + 6
                height: 4
                radius: 2
                x: -3
                y: Math.max(0, Math.min(parent.height - height, root.hue * parent.height - height / 2))
                color: "white"
                border.width: 1
                border.color: "#80000000"
            }
            MouseArea {
                anchors.fill: parent
                function pick(my) { // my = mouse Y relative to the hue bar
                    root.hue = Math.max(0, Math.min(1, my / height));
                }
                onPressed: function (m) { pick(m.y); }
                onPositionChanged: function (m) { if (pressed) pick(m.y); }
            }
        }

        // Saturation (x) / value (y) box.
        Item {
            id: svBox
            width: parent.width - hueBar.width - parent.spacing
            height: parent.height

            Rectangle {
                anchors.fill: parent
                radius: 6
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "#ffffff" }
                    GradientStop { position: 1.0; color: Qt.hsva(root.hue, 1, 1, 1) }
                }
            }
            Rectangle {
                anchors.fill: parent
                radius: 6
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#00000000" }
                    GradientStop { position: 1.0; color: "#ff000000" }
                }
            }
            // Crosshair.
            Rectangle {
                width: 16
                height: 16
                radius: 8
                color: "transparent"
                border.width: 2
                border.color: "white"
                x: Math.max(0, Math.min(parent.width - width, root.saturation * parent.width - width / 2))
                y: Math.max(0, Math.min(parent.height - height, (1 - root.value) * parent.height - height / 2))
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 2
                    radius: 6
                    color: "transparent"
                    border.width: 1
                    border.color: "#80000000"
                }
            }
            MouseArea {
                anchors.fill: parent
                function pick(mx, my) { // mx,my = mouse coords relative to the SV box
                    root.saturation = Math.max(0, Math.min(1, mx / width));
                    root.value = Math.max(0, Math.min(1, 1 - my / height));
                }
                onPressed: function (m) { pick(m.x, m.y); }
                onPositionChanged: function (m) { if (pressed) pick(m.x, m.y); }
            }
        }
    }

    // --- Cancel / Select ---
    Flow {
        id: actionFlow
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.bottomMargin: 14
        width: Math.min(194, Math.max(0, parent.width - 32))
        height: childrenRect.height
        spacing: 10
        CineButton {
            id: cancelBtn
            styleVariant: "dialog"
            colorVariant: "default"
            btnText: qsTr("Cancel")
            width: 92
            height: 38
            onClicked: root.close()
        }
        CineButton {
            id: selectBtn
            styleVariant: "dialog"
            colorVariant: "primary"
            btnText: qsTr("Select")
            width: 92
            height: 38
            onClicked: {
                root.accepted(root.composed);
                root.close();
            }
        }
    }
}
