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
    property bool searchAttempted: false
    parent: Overlay.overlay
    metrics: ViewportMetrics {
        viewportWidth: root.parent ? root.parent.width : root.preferredWidth
        viewportHeight: root.parent ? root.parent.height : root.preferredHeight
    }
    preferredWidth: 680
    preferredHeight: subtitleService && subtitleService.results.length > 0 ? 560 : 440
    modal: true
    focus: true
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle {
        radius: Theme.radius
        color: Theme.popover
        border.color: Theme.popoverBorder
    }
    onOpened: searchField.forceActiveFocus()

    function submitSearch() {
        if (!subtitleService || subtitleService.busy)
            return;
        subtitleService.apiKey = apiKeyField.text.trim();
        searchAttempted = true;
        subtitleService.search(languages.text.trim(), searchField.text.trim());
    }

    contentItem: Item {
        objectName: "dialogSurface"
        Accessible.role: Accessible.Dialog
        Accessible.name: qsTr("Download Subtitles")

        Item {
            id: header
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 56
            Text {
                anchors.left: parent.left
                anchors.right: closeButton.left
                anchors.leftMargin: 20
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Download Subtitles")
                color: Theme.text
                font.pixelSize: 17
                font.bold: true
                elide: Text.ElideRight
            }
            Controls.CineButton {
                id: closeButton
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
        Item {
            id: footer
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: Math.max(56, statusText.implicitHeight + 24)
            Rectangle {
                anchors.top: parent.top
                width: parent.width
                height: 1
                color: Theme.separator
            }
            Text {
                id: statusText
                anchors.left: parent.left
                anchors.right: doneButton.left
                anchors.leftMargin: 20
                anchors.rightMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                text: root.subtitleService ? root.subtitleService.statusMessage : ""
                color: Theme.mutedText
                font.pixelSize: Theme.fontSizeCaption
                wrapMode: Text.Wrap
                maximumLineCount: 3
                elide: Text.ElideRight
                textFormat: Text.PlainText
            }
            Controls.CineButton {
                id: doneButton
                anchors.right: parent.right
                anchors.rightMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                styleVariant: "dialog"
                btnText: qsTr("Close")
                width: 88
                height: 34
                onClicked: root.close()
            }
        }

        Controls.CineScrollView {
            id: bodyScroll
            anchors.top: header.bottom
            anchors.bottom: footer.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 16

            ColumnLayout {
                width: bodyScroll.availableWidth
                spacing: 16

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: searchForm.implicitHeight + 32
                    radius: Theme.radius
                    color: Theme.card
                    ColumnLayout {
                        id: searchForm
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 16
                        spacing: 10
                        Text {
                            text: qsTr("Wyzie API key")
                            color: Theme.text
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
                        }

                        TextField {
                            id: apiKeyField
                            Layout.fillWidth: true
                            Accessible.name: qsTr("Wyzie API key")
                            placeholderText: qsTr("API key")
                            placeholderTextColor: Theme.mutedText
                            echoMode: TextInput.Password
                            text: root.subtitleService ? root.subtitleService.apiKey : ""
                            onEditingFinished: {
                                if (root.subtitleService)
                                    root.subtitleService.apiKey = text;
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
                                    ColorAnimation {
                                        duration: Theme.motionFast
                                    }
                                }
                            }
                        }

                        Text {
                            text: qsTr("Movie, show or ID")
                            color: Theme.text
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
                        }
                        TextField {
                            id: searchField
                            objectName: "initialFocusControl"
                            Layout.fillWidth: true
                            Accessible.name: qsTr("Subtitle search query")
                            placeholderText: qsTr("Title, IMDb ID or TMDB ID")
                            placeholderTextColor: Theme.mutedText
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
                                    ColorAnimation {
                                        duration: Theme.motionFast
                                    }
                                }
                            }
                            onAccepted: root.submitSearch()
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: root.width < 400 ? 1 : 2
                            columnSpacing: 10
                            rowSpacing: 10

                            TextField {
                                id: languages
                                Layout.fillWidth: true
                                Accessible.name: qsTr("Subtitle languages")
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
                                        ColorAnimation {
                                            duration: Theme.motionFast
                                        }
                                    }
                                }
                            }

                            Controls.CineButton {
                                id: searchBtn
                                Layout.preferredWidth: 120
                                Layout.fillWidth: root.width < 400
                                Layout.preferredHeight: 40
                                styleVariant: "dialog"
                                colorVariant: "primary"
                                btnText: root.subtitleService && root.subtitleService.busy ? qsTr("Searching...") : qsTr("Search")
                                focusPolicy: Qt.StrongFocus
                                enabled: !!root.subtitleService && !root.subtitleService.busy
                                onClicked: root.submitSearch()
                            }
                        }
                    }
                }

                ListView {
                    id: resultsList
                    Layout.fillWidth: true
                    Layout.preferredHeight: count > 0 ? Math.min(260, contentHeight) : 48
                    clip: true
                    spacing: 6
                    model: root.subtitleService ? root.subtitleService.results : []
                    delegate: ItemDelegate {
                        id: subtitleRow
                        required property var modelData
                        width: Math.max(0, ListView.view.width - Theme.scrollbarGutter)
                        height: 72
                        Accessible.name: modelData.display || modelData.fileName
                        HoverHandler {
                            cursorShape: Qt.PointingHandCursor
                        }
                        onClicked: root.subtitleService.download(modelData.url, modelData.fileName)
                        background: Rectangle {
                            radius: 8
                            color: subtitleRow.hovered ? Theme.cardHover : Theme.cardStrong
                            border.color: subtitleRow.hovered ? Theme.accent : "transparent"
                            Behavior on border.color {
                                ColorAnimation {
                                    duration: 120
                                }
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
                                sourceSize: Qt.size(56, 40)
                                asynchronous: true
                                fillMode: Image.PreserveAspectFit
                                visible: subtitleRow.modelData.flagUrl !== ""
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 3

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6
                                    Text {
                                        Layout.fillWidth: true
                                        text: subtitleRow.modelData.display || subtitleRow.modelData.language.toUpperCase()
                                        elide: Text.ElideRight
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
                    ScrollBar.vertical: ScrollBar {
                        width: 8
                    }
                    Text {
                        anchors.centerIn: parent
                        width: Math.max(0, parent.width - Theme.scrollbarGutter)
                        visible: resultsList.count === 0
                        text: root.subtitleService && root.subtitleService.busy ? qsTr("Searching...") : root.searchAttempted ? qsTr("No subtitles found") : qsTr("No results yet")
                        color: Theme.mutedText
                        font.pixelSize: Theme.fontSizeSmall
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }
}
