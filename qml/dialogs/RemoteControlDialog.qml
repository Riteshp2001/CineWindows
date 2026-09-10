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
import QtQuick.Layouts
import CineWindows
import "../controls" as Controls

Controls.ResponsivePopup {
    id: root
    property var remoteService
    parent: Overlay.overlay
    metrics: ViewportMetrics {
        viewportWidth: root.parent ? root.parent.width : root.preferredWidth
        viewportHeight: root.parent ? root.parent.height : root.preferredHeight
    }
    preferredWidth: 520
    preferredHeight: remoteService && remoteService.enabled ? 590 : 300
    modal: true
    focus: true
    padding: 22
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle {
        radius: 22
        color: Theme.popover
        border.color: Theme.popoverBorder
    }
    onOpened: enableSwitch.forceActiveFocus()

    contentItem: ScrollView {
        id: bodyScroll
        objectName: "dialogSurface"
        clip: true
        Accessible.role: Accessible.Dialog
        Accessible.name: qsTr("Companion Remote")
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: bodyScroll.availableWidth
            spacing: 18

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Rectangle {
                    Layout.preferredWidth: 44
                    Layout.preferredHeight: 44
                    radius: 15
                    color: Qt.alpha(Theme.accent, 0.16)

                    Controls.CineIcon {
                        anchors.centerIn: parent
                        width: 22
                        height: 22
                        name: "cine-globe-symbolic"
                        filled: true
                        tint: Theme.accent
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: qsTr("Companion Remote")
                        color: Theme.text
                        font.pixelSize: Theme.fontSizeHeading
                        font.bold: true
                    }
                    Text {
                        text: qsTr("Your player, within reach")
                        color: Theme.mutedText
                        font.pixelSize: Theme.fontSizeCaption
                    }
                }

                Rectangle {
                    Layout.preferredWidth: statusContent.implicitWidth + 18
                    Layout.preferredHeight: 28
                    radius: height / 2
                    color: root.remoteService && root.remoteService.enabled
                           ? Qt.alpha(Theme.accent, 0.14) : Theme.card

                    Row {
                        id: statusContent
                        anchors.centerIn: parent
                        spacing: 7
                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 7
                            height: 7
                            radius: width / 2
                            color: root.remoteService && root.remoteService.enabled
                                   ? Theme.accent : Theme.mutedText
                            Behavior on color {
                                ColorAnimation { duration: Theme.motionNormal; easing.type: Easing.OutCubic }
                            }
                        }
                        Text {
                            text: root.remoteService && root.remoteService.enabled
                                  ? qsTr("Live") : qsTr("Off")
                            color: root.remoteService && root.remoteService.enabled
                                   ? Theme.accent : Theme.mutedText
                            font.pixelSize: Theme.fontSizeCaption
                            font.bold: true
                        }
                    }
                }
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("Control playback, paste media links, and browse this computer's folders from another device on the same Wi-Fi network.")
                color: Theme.mutedText
                wrapMode: Text.WordWrap
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 66
                radius: 18
                color: Theme.cardStrong

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 12
                    spacing: 12
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text {
                            text: qsTr("Remote access")
                            color: Theme.text
                            font.pixelSize: Theme.fontSizeBody
                            font.bold: true
                        }
                        Text {
                            text: qsTr("Protected by a six-digit pairing code")
                            color: Theme.mutedText
                            font.pixelSize: Theme.fontSizeCaption
                        }
                    }
                    Controls.CineSwitch {
                        id: enableSwitch
                        objectName: "initialFocusControl"
                        checked: root.remoteService ? root.remoteService.enabled : false
                        Accessible.name: qsTr("Enable remote access")
                        onToggled: {
                            if (root.remoteService)
                                root.remoteService.enabled = checked
                        }
                    }
                }
            }

            GridLayout {
                id: connectionLayout
                objectName: "actionLayout"
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                columns: root.compactLayout ? 1 : 2
                rowSpacing: 12
                columnSpacing: 18
                visible: !!root.remoteService && root.remoteService.enabled
                opacity: visible ? 1 : 0

                Behavior on opacity {
                    NumberAnimation { duration: Theme.motionPanel; easing.type: Easing.OutCubic }
                }

                Rectangle {
                    Layout.preferredWidth: 154
                    Layout.preferredHeight: 154
                    Layout.alignment: Qt.AlignHCenter
                    radius: 20
                    color: "#f5ffff"
                    clip: true

                    QrCodeItem {
                        anchors.fill: parent
                        anchors.margins: 7
                        text: root.remoteService && root.remoteService.enabled
                              ? root.remoteService.remoteUrl : ""
                        foreground: "#102022"
                        backgroundColor: "#f5ffff"
                        Accessible.role: Accessible.Graphic
                        Accessible.name: qsTr("QR code for the companion remote address")
                    }

                    Column {
                        anchors.centerIn: parent
                        width: parent.width - 24
                        spacing: 5
                        visible: !root.remoteService || !root.remoteService.enabled
                        Controls.CineIcon {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: 24
                            height: 24
                            name: "cine-globe-symbolic"
                            tint: "#607173"
                        }
                        Text {
                            width: parent.width
                            text: qsTr("Enable to create QR")
                            color: "#425355"
                            font.pixelSize: Theme.fontSizeCaption
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: root.compactLayout
                    Layout.maximumWidth: root.compactLayout ? 280 : 210
                    Layout.alignment: root.compactLayout ? Qt.AlignHCenter : Qt.AlignVCenter
                    spacing: 8
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Scan to connect")
                        color: Theme.text
                        font.pixelSize: Theme.fontSizeBody
                        font.bold: true
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Open your phone's camera and scan. The paired companion address opens instantly.")
                        color: Theme.mutedText
                        font.pixelSize: Theme.fontSizeCaption
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        Layout.topMargin: 5
                        text: qsTr("PAIRING CODE")
                        color: Theme.mutedText
                        font.pixelSize: Theme.fontSizeTiny
                        font.bold: true
                        font.letterSpacing: 1.2
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text {
                            Layout.fillWidth: true
                            text: root.remoteService ? root.remoteService.pairingCode : ""
                            color: Theme.accent
                            font.pixelSize: 25
                            font.bold: true
                            font.letterSpacing: 3
                        }
                        Controls.CineButton {
                            id: newCodeButton
                            styleVariant: "text"
                            btnText: qsTr("New Code")
                            focusPolicy: Qt.StrongFocus
                            onClicked: {
                                if (root.remoteService)
                                    root.remoteService.regeneratePairingCode()
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8
                visible: !!root.remoteService && root.remoteService.enabled
                Text {
                    text: qsTr("Open this address on your other device")
                    color: Theme.text
                    font.pixelSize: Theme.fontSizeSmall
                    font.bold: true
                }
                TextField {
                    id: addressField
                    Layout.fillWidth: true
                    readOnly: true
                    text: root.remoteService ? root.remoteService.remoteUrl : ""
                    placeholderText: qsTr("Enable remote access to get the address")
                    color: Theme.text
                    selectByMouse: true
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Controls.CineButton {
                        Layout.fillWidth: true
                        styleVariant: "dialog"
                        btnText: qsTr("Copy Address")
                        enabled: addressField.text.length > 0
                        focusPolicy: Qt.StrongFocus
                        onClicked: {
                            addressField.selectAll()
                            addressField.copy()
                            addressField.select(0, 0)
                        }
                    }
                    Controls.CineButton {
                        Layout.fillWidth: true
                        styleVariant: "dialog"
                        colorVariant: "primary"
                        btnText: qsTr("Preview Remote")
                        enabled: addressField.text.length > 0
                        focusPolicy: Qt.StrongFocus
                        onClicked: Qt.openUrlExternally(addressField.text)
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                visible: !!root.remoteService && root.remoteService.enabled
                text: qsTr("Keep this window open while pairing. Anyone with the current address and code can browse playable media until remote access is switched off.")
                color: Theme.mutedText
                font.pixelSize: Theme.fontSizeCaption
                wrapMode: Text.WordWrap
            }
            Text {
                Layout.fillWidth: true
                visible: !!root.remoteService && root.remoteService.errorString.length > 0
                text: root.remoteService ? root.remoteService.errorString : ""
                color: Theme.danger
                wrapMode: Text.WordWrap
            }
        }
    }
}
