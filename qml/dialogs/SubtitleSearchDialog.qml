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
import CineWindows
import "../controls" as Controls

Controls.ResponsivePopup {
    id: root
    property var subtitleService
    parent: Overlay.overlay
    metrics: ViewportMetrics {
        viewportWidth: root.parent ? root.parent.width : root.preferredWidth
        viewportHeight: root.parent ? root.parent.height : root.preferredHeight
    }
    preferredWidth: 680
    preferredHeight: 560
    modal: true
    focus: true
    padding: 20
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle {
        radius: 16
        color: Theme.popover
        border.color: Theme.popoverBorder
    }
    onOpened: searchField.forceActiveFocus()

    contentItem: ScrollView {
        id: bodyScroll
        objectName: "dialogSurface"
        clip: true
        Accessible.role: Accessible.Dialog
        Accessible.name: qsTr("Download Subtitles")
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: bodyScroll.availableWidth
            spacing: 16

            ColumnLayout {
                spacing: 4
                Text {
                    text: qsTr("Download Subtitles")
                    color: Theme.text
                    font.pixelSize: 22
                    font.bold: true
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Search by movie/show name or paste a TMDB/IMDB ID.")
                    color: Theme.mutedText
                    wrapMode: Text.WordWrap
                    font.pixelSize: 13
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 1
                color: Theme.separator
            }

            TextField {
                id: apiKeyField
                Layout.fillWidth: true
                placeholderText: qsTr("Wyzie API key (free at store.wyzie.io)")
                echoMode: TextInput.PasswordEchoOnEdit
                text: root.subtitleService ? root.subtitleService.apiKey : ""
                onEditingFinished: {
                    if (root.subtitleService)
                        root.subtitleService.apiKey = text
                }
                color: Theme.text
                font.pixelSize: 14
                leftPadding: 12
                rightPadding: 12
                implicitHeight: 40
                background: Rectangle {
                    radius: 8
                    color: Theme.cardStrong
                    border.color: parent.activeFocus ? Theme.accent : Theme.separator
                    Behavior on border.color {
                        ColorAnimation { duration: 150 }
                    }
                }
            }

            TextField {
                id: searchField
                objectName: "initialFocusControl"
                Layout.fillWidth: true
                placeholderText: qsTr("Search... e.g. The Matrix, Interstellar, tt0133093")
                color: Theme.text
                font.pixelSize: 14
                leftPadding: 12
                rightPadding: 12
                implicitHeight: 40
                background: Rectangle {
                    radius: 8
                    color: Theme.cardStrong
                    border.color: parent.activeFocus ? Theme.accent : Theme.separator
                    Behavior on border.color {
                        ColorAnimation { duration: 150 }
                    }
                }
                onAccepted: searchBtn.clicked()
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                TextField {
                    id: languages
                    Layout.fillWidth: true
                    text: "en"
                    placeholderText: qsTr("Languages (en, hi, es...)")
                    color: Theme.text
                    font.pixelSize: 14
                    leftPadding: 12
                    rightPadding: 12
                    implicitHeight: 40
                    background: Rectangle {
                        radius: 8
                        color: Theme.cardStrong
                        border.color: parent.activeFocus ? Theme.accent : Theme.separator
                        Behavior on border.color {
                            ColorAnimation { duration: 150 }
                        }
                    }
                }

                Controls.CineButton {
                    id: searchBtn
                    Layout.preferredWidth: 120
                    Layout.fillHeight: true
                    styleVariant: "dialog"
                    colorVariant: "primary"
                    btnText: root.subtitleService && root.subtitleService.busy
                        ? qsTr("Searching...") : qsTr("Search")
                    focusPolicy: Qt.StrongFocus
                    enabled: !!root.subtitleService && !root.subtitleService.busy
                    onClicked: {
                        let q = searchField.text.trim()
                        if (q.isEmpty() && root.subtitleService)
                            q = ""
                        if (root.subtitleService)
                            root.subtitleService.search(languages.text, q)
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                text: root.subtitleService ? root.subtitleService.statusMessage : ""
                color: Theme.mutedText
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                visible: text !== ""
            }

            ListView {
                Layout.fillWidth: true
                Layout.preferredHeight: root.shortLayout ? 160 : Math.max(260, root.height - 280)
                clip: true
                spacing: 6
                model: root.subtitleService ? root.subtitleService.results : []
                delegate: ItemDelegate {
                    id: subtitleRow
                    required property var modelData
                    width: ListView.view.width
                    height: 72
                    Accessible.name: modelData.display || modelData.fileName
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                    onClicked: root.subtitleService.download(modelData.url, modelData.fileName)
                    background: Rectangle {
                        radius: 8
                        color: subtitleRow.hovered ? Theme.cardHover : Theme.cardStrong
                        border.color: subtitleRow.hovered ? Theme.accent : "transparent"
                        Behavior on border.color {
                            ColorAnimation { duration: 120 }
                        }
                    }
                    contentItem: RowLayout {
                        spacing: 12
                        anchors.fill: parent
                        anchors.leftMargin: 4
                        anchors.rightMargin: 4

                        Image {
                            Layout.preferredWidth: 28
                            Layout.preferredHeight: 20
                            source: subtitleRow.modelData.flagUrl || ""
                            fillMode: Image.PreserveAspectFit
                            visible: subtitleRow.modelData.flagUrl !== ""
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3

                            RowLayout {
                                spacing: 6
                                Text {
                                    text: subtitleRow.modelData.display || subtitleRow.modelData.language.toUpperCase()
                                    color: Theme.text
                                    font.bold: true
                                    font.pixelSize: 13
                                }
                                Rectangle {
                                    Layout.preferredWidth: srcLabel.implicitWidth + 10
                                    Layout.preferredHeight: 18
                                    radius: 4
                                    color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.15)
                                    Text {
                                        id: srcLabel
                                        anchors.centerIn: parent
                                        text: subtitleRow.modelData.source
                                        color: Theme.accent
                                        font.pixelSize: 9
                                        font.bold: true
                                    }
                                }
                                Rectangle {
                                    Layout.preferredWidth: fmtLabel.implicitWidth + 10
                                    Layout.preferredHeight: 18
                                    radius: 4
                                    color: Theme.pillSecondary
                                    visible: subtitleRow.modelData.format !== ""
                                    Text {
                                        id: fmtLabel
                                        anchors.centerIn: parent
                                        text: subtitleRow.modelData.format.toUpperCase()
                                        color: Theme.pillSecondaryText
                                        font.pixelSize: 9
                                        font.bold: true
                                    }
                                }
                                Rectangle {
                                    Layout.preferredWidth: 20
                                    Layout.preferredHeight: 18
                                    radius: 4
                                    color: Theme.danger
                                    visible: subtitleRow.modelData.isHearingImpaired
                                    Text {
                                        anchors.centerIn: parent
                                        text: "Hi"
                                        color: "white"
                                        font.pixelSize: 9
                                        font.bold: true
                                    }
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: subtitleRow.modelData.release || subtitleRow.modelData.fileName || ""
                                color: Theme.mutedText
                                font.pixelSize: 11
                                elide: Text.ElideRight
                                maximumLineCount: 1
                            }
                        }

                        Text {
                            text: qsTr("Download")
                            color: Theme.accent
                            font.bold: true
                            font.pixelSize: 12
                            Layout.alignment: Qt.AlignVCenter
                        }
                    }
                }
                ScrollBar.vertical: ScrollBar {}
            }

            GridLayout {
                id: actionRow
                objectName: "actionLayout"
                Layout.fillWidth: true
                columns: root.compactLayout ? 1 : 2
                columnSpacing: 10
                rowSpacing: 10

                Controls.CineButton {
                    Layout.fillWidth: root.compactLayout
                    Layout.preferredWidth: 110
                    Layout.preferredHeight: 34
                    styleVariant: "dialog"
                    colorVariant: "default"
                    btnText: qsTr("Cancel")
                    focusPolicy: Qt.StrongFocus
                    onClicked: root.close()
                }
                Controls.CineButton {
                    Layout.fillWidth: root.compactLayout
                    Layout.preferredWidth: 130
                    Layout.preferredHeight: 34
                    styleVariant: "dialog"
                    colorVariant: "primary"
                    btnText: root.subtitleService && root.subtitleService.busy
                        ? qsTr("Searching...") : qsTr("Search")
                    focusPolicy: Qt.StrongFocus
                    enabled: !!root.subtitleService && !root.subtitleService.busy
                    onClicked: searchBtn.clicked()
                }
            }
        }
    }
}
