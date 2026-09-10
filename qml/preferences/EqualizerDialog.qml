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
import QtQuick.Layouts
import "../controls"
import CineWindows

ResponsivePopup {
    id: root
    property var advanced

    parent: Overlay.overlay
    metrics: ViewportMetrics {
        viewportWidth: root.parent ? root.parent.width : root.preferredWidth
        viewportHeight: root.parent ? root.parent.height : root.preferredHeight
    }
    preferredWidth: 760
    preferredHeight: 650
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 0
    focus: true
    onOpened: closeButton.forceActiveFocus()

    readonly property int equalizerColumns: compactLayout
        ? Math.max(2, Math.min(5, Math.floor((Math.max(0, width - 44) + 6) / 54)))
        : 10
    readonly property bool presetActionsWrapped: presetFlow.height > 34

    component RoundedComboBox: ComboBox {
        id: combo
        implicitHeight: 40
        Layout.preferredHeight: 40
        focusPolicy: Qt.TabFocus

        background: Rectangle {
            radius: Theme.radius
            color: combo.down ? Theme.glassActive
                : combo.hovered ? Theme.cardHover : Theme.cardStrong
            border.color: combo.popup.visible ? Theme.accent : "transparent"
            border.width: 1
        }
        contentItem: Text {
            leftPadding: 12
            rightPadding: 28
            text: combo.displayText
            color: combo.enabled ? Theme.text : Theme.mutedText
            font.pixelSize: 13
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        indicator: CineIcon {
            x: combo.width - width - 10
            y: (combo.height - height) / 2
            name: "cine-dropdown-symbolic"
            tint: Theme.mutedText
            width: 11
            height: 11
        }
        delegate: ItemDelegate {
            id: comboItem
            required property var modelData
            required property int index
            width: ListView.view ? ListView.view.width : combo.width
            height: 36
            highlighted: combo.highlightedIndex === index
            background: Rectangle {
                anchors.fill: parent
                anchors.margins: 4
                radius: 7
                color: comboItem.highlighted ? Theme.cardHover : "transparent"
            }
            contentItem: Text {
                leftPadding: 8
                text: combo.textRole ? comboItem.modelData[combo.textRole] : comboItem.modelData
                color: Theme.text
                font.pixelSize: 13
                font.bold: combo.currentIndex === comboItem.index
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
        popup: Popup {
            y: combo.height + 6
            width: Math.max(combo.width, 150)
            x: combo.width - width
            implicitHeight: Math.min(comboList.contentHeight + 8, 260)
            padding: 4
            background: Item {
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 4
                    radius: 12
                    color: Theme.popoverShadow
                }
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 4
                    radius: 12
                    color: Theme.popover
                    opacity: 0.95
                    border.color: Theme.popoverBorder
                    border.width: 1
                }
            }
            contentItem: ListView {
                id: comboList
                clip: true
                implicitHeight: contentHeight
                model: combo.delegateModel
                currentIndex: combo.highlightedIndex
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {
                    width: 6
                }
            }
        }
    }

    background: Rectangle {
        radius: 18
        color: Theme.popover
        border.color: Theme.popoverBorder
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 0
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 76

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 22
                anchors.verticalCenter: parent.verticalCenter
                spacing: 12

                Rectangle {
                    width: 36
                    height: 36
                    radius: 10
                    color: Qt.alpha(Theme.accent, 0.16)

                    CineIcon {
                        anchors.centerIn: parent
                        width: 20
                        height: 20
                        name: "cine-audio-symbolic"
                        tint: Theme.accent
                        filled: true
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2
                    Text { text: qsTr("Equalizer"); color: Theme.text; font.pixelSize: Theme.fontSizeHeading; font.bold: true }
                    Text { text: qsTr("Shape audio and fine-tune playback output"); color: Theme.mutedText; font.pixelSize: Theme.fontSizeCaption }
                }
            }
            CineButton {
                id: closeButton
                objectName: "initialFocusControl"
                anchors.right: parent.right; anchors.rightMargin: 14; anchors.verticalCenter: parent.verticalCenter
                styleVariant: "close";                 iconName: "cine-close-symbolic"; buttonSize: 34; btnTooltip: qsTr("Close"); focusPolicy: Qt.StrongFocus; onClicked: root.close()
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.separator }
        Flickable {
            objectName: "dialogSurface"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            Accessible.role: Accessible.Dialog
            Accessible.name: qsTr("Equalizer")
            contentHeight: settingsColumn.implicitHeight + 40
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}

            ColumnLayout {
                id: settingsColumn
                x: 22; y: 20
                width: parent.width - 44
                spacing: 14

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: equalizerSection.implicitHeight + 32
                    radius: 12
                    color: Theme.panelStrong
                    border.color: Theme.separator
                    border.width: 1

                    ColumnLayout {
                        id: equalizerSection
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 16
                        spacing: 14

                        RowLayout {
                            Layout.fillWidth: true
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text { text: qsTr("10-band equalizer"); color: Theme.text; font.pixelSize: Theme.fontSizeBody; font.bold: true }
                                Text { text: qsTr("Adjust frequency gain from low bass to high treble"); color: Theme.mutedText; font.pixelSize: Theme.fontSizeCaption }
                            }
                            CineSwitch {
                                checked: root.advanced ? root.advanced.equalizerEnabled : false
                                Accessible.name: qsTr("Enable equalizer")
                                onToggled: if (root.advanced) root.advanced.equalizerEnabled = checked
                            }
                        }

                        GridLayout {
                            id: equalizerGrid
                            Layout.fillWidth: true
                            columns: root.equalizerColumns
                            rowSpacing: 12
                            columnSpacing: 6
                            Repeater {
                                model: [{label:"31",key:"31"},{label:"62",key:"62"},{label:"125",key:"125"},{label:"250",key:"250"},{label:"500",key:"500"},{label:"1k",key:"1k"},{label:"2k",key:"2k"},{label:"4k",key:"4k"},{label:"8k",key:"8k"},{label:"16k",key:"16k"}]
                                delegate: ColumnLayout {
                                    id: band
                                    required property int index
                                    required property var modelData
                                    Layout.preferredWidth: Math.max(42,
                                        (equalizerGrid.width - (root.equalizerColumns - 1) * equalizerGrid.columnSpacing)
                                        / root.equalizerColumns)
                                    Layout.preferredHeight: 176
                                    spacing: 4
                                    Text { Layout.alignment: Qt.AlignHCenter; text: (root.advanced ? Number(root.advanced.equalizerBands[band.index]).toFixed(1) : "0.0"); color: Theme.accent; font.pixelSize: Theme.fontSizeTiny; font.bold: true }
                                    CineRangeSlider {
                                        Layout.alignment: Qt.AlignHCenter
                                        Layout.fillHeight: true
                                        orientation: Qt.Vertical
                                        from: -12; to: 12; stepSize: 0.5
                                        value: root.advanced ? root.advanced.equalizerBands[band.index] : 0
                                        Accessible.name: qsTr("%1 Hz gain").arg(band.modelData.label)
                                        onMoved: if (root.advanced) root.advanced.setEqualizerBand(band.index, value)
                                    }
                                    Text { Layout.alignment: Qt.AlignHCenter; text: band.modelData.label; color: Theme.text; font.pixelSize: Theme.fontSizeTiny; font.bold: true }
                                    Text { Layout.alignment: Qt.AlignHCenter; text: "Hz"; color: Theme.mutedText; font.pixelSize: Theme.fontSizeTiny }
                                }
                            }
                        }
                    }
                }
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: presetFlow.height

                    Flow {
                        id: presetFlow
                        objectName: "equalizerPresetActions"
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: childrenRect.height
                        spacing: 8
                        CineButton { styleVariant: "text"; btnText: qsTr("Flat"); enabled: !!root.advanced; onClicked: root.advanced.applyEqualizerPreset("flat") }
                        CineButton { styleVariant: "text"; btnText: qsTr("Bass Boost"); enabled: !!root.advanced; onClicked: root.advanced.applyEqualizerPreset("bass") }
                        CineButton { styleVariant: "text"; btnText: qsTr("Voice"); enabled: !!root.advanced; onClicked: root.advanced.applyEqualizerPreset("voice") }
                        CineButton { styleVariant: "text"; btnText: qsTr("Treble Boost"); enabled: !!root.advanced; onClicked: root.advanced.applyEqualizerPreset("treble") }
                        CineButton { styleVariant: "text"; btnText: qsTr("Reset"); enabled: !!root.advanced; onClicked: root.advanced.resetEqualizer() }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: outputSettings.implicitHeight + 32
                    radius: 12
                    color: Theme.panelStrong
                    border.color: Theme.separator
                    border.width: 1

                    ColumnLayout {
                        id: outputSettings
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 16
                        spacing: 12

                        Text { text: qsTr("Output tools"); color: Theme.text; font.pixelSize: Theme.fontSizeBody; font.bold: true }
                        GridLayout {
                            columns: root.compactLayout ? 1 : 2
                            Layout.fillWidth: true
                            columnSpacing: 16
                            rowSpacing: 12

                            ColumnLayout {
                                Layout.fillWidth: true
                                Text { text: qsTr("Audio visualization"); color: Theme.mutedText; font.pixelSize: Theme.fontSizeCaption }
                                RoundedComboBox {
                                    Layout.fillWidth: true
                                    model: [{label:qsTr("Off"), value:"off"}, {label:qsTr("Spectrum"), value:"spectrum"}, {label:qsTr("Waveform"), value:"waves"}]
                                    textRole: "label"
                                    currentIndex: root.advanced && root.advanced.visualization === "spectrum" ? 1 : root.advanced && root.advanced.visualization === "waves" ? 2 : 0
                                    onActivated: if (root.advanced) root.advanced.visualization = model[currentIndex].value
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                Text { text: qsTr("3D source layout"); color: Theme.mutedText; font.pixelSize: Theme.fontSizeCaption }
                                RoundedComboBox {
                                    Layout.fillWidth: true
                                    model: [{label:qsTr("Off"),value:"off"},{label:qsTr("Side-by-side, left first"),value:"sbsl"},{label:qsTr("Side-by-side, right first"),value:"sbsr"},{label:qsTr("Top/bottom, left first"),value:"abl"},{label:qsTr("Top/bottom, right first"),value:"abr"}]
                                    textRole: "label"
                                    onActivated: if (root.advanced) root.advanced.stereoInput = model[currentIndex].value
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                Text { text: qsTr("3D display mode"); color: Theme.mutedText; font.pixelSize: Theme.fontSizeCaption }
                                RoundedComboBox {
                                    Layout.fillWidth: true
                                    enabled: !!root.advanced && root.advanced.stereoInput !== "off"
                                    model: [{label:qsTr("Red/cyan anaglyph"),value:"arcd"},{label:qsTr("Left eye"),value:"ml"},{label:qsTr("Right eye"),value:"mr"},{label:qsTr("Side-by-side"),value:"sbsl"}]
                                    textRole: "label"
                                    onActivated: if (root.advanced) root.advanced.stereoOutput = model[currentIndex].value
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                Text { text: qsTr("HDR tone mapping"); color: Theme.mutedText; font.pixelSize: Theme.fontSizeCaption }
                                RoundedComboBox {
                                    Layout.fillWidth: true
                                    model: ["auto", "bt.2390", "mobius", "hable", "spline"]
                                    onActivated: if (root.advanced) root.advanced.toneMapping = currentText
                                }
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: qsTr("HDR target peak"); color: Theme.mutedText; font.pixelSize: Theme.fontSizeCaption }
                            CineRangeSlider { Layout.fillWidth: true; from: 100; to: 1000; stepSize: 10; value: root.advanced ? root.advanced.targetPeak : 203; Accessible.name: qsTr("HDR target peak"); onMoved: if (root.advanced) root.advanced.targetPeak = value }
                            Text { text: (root.advanced ? root.advanced.targetPeak : 203) + " nits"; color: Theme.text; font.pixelSize: Theme.fontSizeCaption; font.bold: true; Layout.preferredWidth: 72 }
                        }
                    }
                }
            }
        }
    }
}
