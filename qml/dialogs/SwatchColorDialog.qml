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

// Swatch-grid colour picker matching the CineWindows "Subtitle Color" dialog:
// a grid of preset shades plus a "Custom" row whose "+" opens the full HSV picker.
ResponsivePopup {
    id: root

    parent: Overlay.overlay
    metrics: ViewportMetrics {
        viewportWidth: root.parent ? root.parent.width : root.preferredWidth
        viewportHeight: root.parent ? root.parent.height : root.preferredHeight
    }
    modal: true
    dim: true
    preferredWidth: 520
    preferredHeight: 420
    padding: 0
    focus: true

    /// Dialog title shown in the header
    property string title: qsTr("Subtitle Color")
    /// The currently selected color (highlighted in the grid)
    property color currentColor: "#ffffff"
    /// Emitted with the chosen color when user presses Select
    signal accepted(color selectedColor)

    /// Opens the dialog pre-selected to color `c`
    // c: colour to pre-select in the dialog
    function openFor(c) {
        currentColor = c;
        customColor = c;
        open();
    }

    /// The colour shown in the Custom chip (last value picked via the HSV dialog)
    property color customColor: "#000000"

    // 9 hue columns × 5 shade rows, mirroring the reference palette.
    readonly property var swatchRows: [["#99c1f1", "#8ff0a4", "#f9f06b", "#ffbe6f", "#f66151", "#dc8add", "#cdab8f", "#ffffff", "#9a9996"], ["#62a0ea", "#57e389", "#f8e45c", "#ffa348", "#ed333b", "#c061cb", "#b5835a", "#f6f5f4", "#77767b"], ["#3584e4", "#33d17a", "#f6d32d", "#ff7800", "#e01b24", "#9141ac", "#986a44", "#deddda", "#5e5c64"], ["#1c71d8", "#2ec27e", "#f5c211", "#e66100", "#c01c28", "#813d9c", "#865e3c", "#c0bfbc", "#3d3846"], ["#1a5fb4", "#26a269", "#e5a50a", "#c64600", "#a51d2d", "#613583", "#63452c", "#9a9996", "#241f31"]]
    // Flat list of all swatch color values, joined from swatchRows
    readonly property var swatches: [].concat.apply([], swatchRows)
    // Number of swatch columns that fit the available width
    readonly property int swatchColumns: Math.max(1, Math.min(9,
        Math.floor((Math.max(0, width - 40) + 6) / 50)))
    // Whether the Cancel/Select buttons have wrapped to a second row
    readonly property bool actionsWrapped: actionFlow.height > 38

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
        height: 52
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

    // A single selectable colour swatch in the grid
    component Swatch: Rectangle {
        id: sw
        // The colour this swatch represents
        property color value: "#ffffff"
        // True when this swatch matches the dialog's current selection
        readonly property bool isSelected: Qt.colorEqual(root.currentColor, value)
        width: 44
        height: 44
        radius: 9
        color: value
        border.width: isSelected ? 3 : 1
        border.color: isSelected ? Theme.accent : "#33ffffff"
        Behavior on border.color {
            ColorAnimation {
                duration: Theme.motionFast
            }
        }
        // Check mark when selected.
        CineIcon {
            anchors.centerIn: parent
            visible: sw.isSelected
            name: "cine-check-symbolic"
            tint: "#06181a"
            width: 18
            height: 18
        }
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.currentColor = sw.value
        }
    }

    Flickable {
        id: swatchFlickable
        anchors.top: headerBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: actionFlow.top
        anchors.topMargin: 18
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        anchors.bottomMargin: 12
        contentWidth: width
        contentHeight: body.implicitHeight
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Column {
            id: body
            width: swatchFlickable.width
            spacing: 18

            Grid {
                id: swatchGrid
                anchors.horizontalCenter: parent.horizontalCenter
                columns: root.swatchColumns
                spacing: 6
                width: columns * 44 + Math.max(0, columns - 1) * spacing
                height: childrenRect.height
                Repeater {
                    model: root.swatches
                    Swatch {
                        // Colour value provided by the swatches model
                        required property var modelData
                        value: modelData
                    }
                }
            }

            Text {
                text: qsTr("Custom")
                color: Theme.text
                font.pixelSize: 14
                font.bold: true
            }

            Row {
                spacing: 10

                CineButton {
                    id: addBtn
                    styleVariant: "text"
                    iconName: "cine-list-add-symbolic"
                    buttonSize: 44
                    width: 44
                    height: 44
                    onClicked: hsvDialog.openFor(root.customColor)
                }

                Swatch {
                    value: root.customColor
                }
            }
        }
    }

    // --- Cancel / Select ---
    Flow {
        id: actionFlow
        anchors.right: parent.right
        anchors.bottom: parent.bottom
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
                root.accepted(root.currentColor);
                root.close();
            }
        }
    }

    // Nested HSV picker used by the "+" button.
    HsvColorDialog {
        id: hsvDialog
        objectName: "customHsvDialog"
        metrics: root.metrics
        title: qsTr("Custom Color")
        useAlpha: false
        // c: colour chosen from the HSV picker
        onAccepted: function (c) {
            root.customColor = c;
            root.currentColor = c;
        }
    }
}
