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
    parent: Overlay.overlay
    metrics: ViewportMetrics {
        viewportWidth: root.parent ? root.parent.width : root.preferredWidth
        viewportHeight: root.parent ? root.parent.height : root.preferredHeight
    }
    modal: true
    dim: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    preferredWidth: 460
    preferredHeight: 420
    padding: 24
    onOpened: closeButton.forceActiveFocus()

    background: Rectangle {
        radius: 18
        color: Theme.popover
        border.color: Theme.popoverBorder
        border.width: 1
    }

    contentItem: ScrollView {
        id: bodyScroll
        objectName: "dialogSurface"
        clip: true
        Accessible.role: Accessible.Dialog
        Accessible.name: qsTr("About CineWindows")
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: bodyScroll.availableWidth
            spacing: 18

            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                Rectangle {
                    Layout.preferredWidth: 76
                    Layout.preferredHeight: 76
                    radius: 18
                    color: Theme.panelStrong
                    border.color: Theme.separator
                    border.width: 1

                    Image {
                        anchors.centerIn: parent
                        width: 58
                        height: 58
                        source: "qrc:/cinewindows/icons/apps/CineWindows.svg"
                        sourceSize.width: 58
                        sourceSize.height: 58
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("CineWindows")
                        color: Theme.text
                        font.pixelSize: 26
                        font.bold: true
                        elide: Text.ElideRight
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Quietly cinematic playback")
                        color: Theme.mutedText
                        font.pixelSize: Theme.fontSizeSmall
                        elide: Text.ElideRight
                    }
                    Rectangle {
                        Layout.preferredWidth: versionLabel.implicitWidth + 16
                        Layout.preferredHeight: 24
                        radius: 12
                        color: Qt.alpha(Theme.accent, 0.14)
                        Text {
                            id: versionLabel
                            anchors.centerIn: parent
                            text: qsTr("Version %1").arg(Qt.application.version)
                            color: Theme.accent
                            font.pixelSize: Theme.fontSizeCaption
                            font.bold: true
                        }
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.separator }

            Text {
                Layout.fillWidth: true
                text: qsTr("A Windows-first media player built for focused viewing, precise control, and dependable local or streamed playback.")
                wrapMode: Text.WordWrap
                color: Theme.text
                font.pixelSize: Theme.fontSizeBody
                lineHeight: 1.35
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: detailsLayout.implicitHeight + 28
                radius: 12
                color: Theme.panelStrong
                border.color: Theme.separator
                border.width: 1

                GridLayout {
                    id: detailsLayout
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 14
                    columns: root.compactLayout ? 1 : 2
                    columnSpacing: 24
                    rowSpacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { text: qsTr("Playback engine"); color: Theme.mutedText; font.pixelSize: Theme.fontSizeCaption }
                        Text { text: qsTr("mpv"); color: Theme.text; font.pixelSize: Theme.fontSizeSmall; font.bold: true }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { text: qsTr("Interface"); color: Theme.mutedText; font.pixelSize: Theme.fontSizeCaption }
                        Text { text: qsTr("Qt Quick"); color: Theme.text; font.pixelSize: Theme.fontSizeSmall; font.bold: true }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { text: qsTr("Platform"); color: Theme.mutedText; font.pixelSize: Theme.fontSizeCaption }
                        Text { text: qsTr("Windows"); color: Theme.text; font.pixelSize: Theme.fontSizeSmall; font.bold: true }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { text: qsTr("Focus"); color: Theme.mutedText; font.pixelSize: Theme.fontSizeCaption }
                        Text { text: qsTr("Local and streamed media"); color: Theme.text; font.pixelSize: Theme.fontSizeSmall; font.bold: true; elide: Text.ElideRight }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Copyright (c) 2026 Ritesh Pandit")
                    color: Theme.mutedText
                    font.pixelSize: Theme.fontSizeCaption
                    wrapMode: Text.WordWrap
                }
                Controls.CineButton {
                    id: closeButton
                    objectName: "initialFocusControl"
                    styleVariant: "dialog"
                    btnText: qsTr("Close")
                    Layout.preferredWidth: 104
                    Layout.preferredHeight: 36
                    focusPolicy: Qt.StrongFocus
                    onClicked: root.close()
                }
            }
        }
    }
}
