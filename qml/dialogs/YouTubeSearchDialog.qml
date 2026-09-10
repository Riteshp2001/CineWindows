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
    property var searchService
    signal playRequested(string url)

    parent: Overlay.overlay
    metrics: ViewportMetrics {
        viewportWidth: root.parent ? root.parent.width : root.preferredWidth
        viewportHeight: root.parent ? root.parent.height : root.preferredHeight
    }
    preferredWidth: 760
    preferredHeight: 620
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 18
    background: Rectangle {
        radius: 16
        color: Theme.popover
        border.color: Theme.popoverBorder
    }
    onOpened: queryField.forceActiveFocus()

    function twoDigits(value) {
        return value < 10 ? "0" + value : String(value);
    }

    function formatDuration(seconds) {
        const total = Math.max(0, Math.round(Number(seconds || 0)));
        if (total === 0)
            return "";
        const hours = Math.floor(total / 3600);
        const minutes = Math.floor((total % 3600) / 60);
        const remaining = total % 60;
        return hours > 0
            ? hours + ":" + twoDigits(minutes) + ":" + twoDigits(remaining)
            : minutes + ":" + twoDigits(remaining);
    }

    function formatViews(count) {
        const views = Number(count || 0);
        if (views <= 0)
            return "";
        if (views >= 1000000000)
            return (views / 1000000000).toFixed(1).replace(".0", "") + "B views";
        if (views >= 1000000)
            return (views / 1000000).toFixed(1).replace(".0", "") + "M views";
        if (views >= 1000)
            return (views / 1000).toFixed(1).replace(".0", "") + "K views";
        return Math.round(views) + " views";
    }

    function publishedText(result) {
        if (Number(result.timestamp || 0) > 0)
            return Qt.formatDate(new Date(Number(result.timestamp) * 1000), "MMM d, yyyy");
        const date = String(result.uploadDate || "");
        return date.length === 8
            ? date.slice(0, 4) + "-" + date.slice(4, 6) + "-" + date.slice(6, 8)
            : "";
    }

    contentItem: ScrollView {
        id: bodyScroll
        objectName: "dialogSurface"
        clip: true
        Accessible.role: Accessible.Dialog
        Accessible.name: qsTr("Search YouTube")
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: bodyScroll.availableWidth
            spacing: 12

            Text {
                text: qsTr("Search YouTube")
                color: Theme.text
                font.pixelSize: 20
                font.bold: true
            }
            GridLayout {
                objectName: "actionLayout"
                Layout.fillWidth: true
                columns: root.compactLayout ? 1 : 2

                TextField {
                    id: queryField
                    objectName: "initialFocusControl"
                    Layout.fillWidth: true
                    placeholderText: qsTr("Videos, channels, or topics")
                    color: Theme.text
                    onAccepted: {
                        if (root.searchService)
                            root.searchService.search(text)
                    }
                    background: Rectangle {
                        radius: Theme.radius
                        color: Theme.cardStrong
                        border.color: queryField.activeFocus ? Theme.accent : Theme.separator
                    }
                }
                Controls.CineButton {
                    Layout.fillWidth: root.compactLayout
                    styleVariant: "dialog"
                    colorVariant: "primary"
                    btnText: root.searchService && root.searchService.busy
                        ? qsTr("Searching...") : qsTr("Search")
                    focusPolicy: Qt.StrongFocus
                    enabled: !!root.searchService && !root.searchService.busy
                    onClicked: root.searchService.search(queryField.text)
                }
            }
            Text {
                Layout.fillWidth: true
                visible: !!root.searchService && root.searchService.errorString.length > 0
                text: root.searchService ? root.searchService.errorString : ""
                color: Theme.danger
                wrapMode: Text.WordWrap
            }
            ListView {
                Layout.fillWidth: true
                Layout.preferredHeight: root.shortLayout ? 180 : Math.max(280, root.height - 150)
                clip: true
                spacing: 6
                model: root.searchService ? root.searchService.results : []
                delegate: ItemDelegate {
                    id: resultRow
                    required property var modelData
                    width: ListView.view.width
                    height: root.compactLayout ? 86 : 96
                    focusPolicy: Qt.TabFocus
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                    Accessible.name: modelData.title
                    onClicked: {
                        root.playRequested(modelData.url)
                        root.close()
                    }
                    background: Rectangle {
                        radius: Theme.radius
                        color: resultRow.hovered ? Theme.cardHover : Theme.cardStrong
                    }
                    contentItem: RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 12
                        spacing: 12
                        Rectangle {
                            Layout.preferredWidth: root.compactLayout ? 72 : 112
                            Layout.preferredHeight: root.compactLayout ? 48 : 63
                            radius: 7
                            color: Theme.panelStrong
                            clip: true

                            Image {
                                id: resultThumbnail
                                anchors.fill: parent
                                source: resultRow.modelData.thumbnail
                                sourceSize.width: width
                                sourceSize.height: height
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                cache: true
                            }
                            Controls.CineIcon {
                                anchors.centerIn: parent
                                width: 22
                                height: 22
                                visible: resultThumbnail.status === Image.Loading
                                    || resultThumbnail.status === Image.Error
                                name: "cine-video-x-generic-symbolic"
                                tint: Theme.mutedText
                            }
                            Rectangle {
                                visible: durationLabel.text.length > 0
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                anchors.margins: 4
                                width: durationLabel.implicitWidth + 8
                                height: 20
                                radius: 5
                                color: "#d9000000"
                                Text {
                                    id: durationLabel
                                    anchors.centerIn: parent
                                    text: root.formatDuration(resultRow.modelData.duration)
                                    color: Theme.iconOnDark
                                    font.pixelSize: Theme.fontSizeTiny
                                    font.bold: true
                                }
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Text {
                                Layout.fillWidth: true
                                text: resultRow.modelData.title
                                color: Theme.text
                                font.bold: true
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.fillWidth: true
                                text: resultRow.modelData.channel || qsTr("YouTube")
                                color: Theme.mutedText
                                font.pixelSize: Theme.fontSizeCaption
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.fillWidth: true
                                readonly property string views: root.formatViews(resultRow.modelData.viewCount)
                                readonly property string published: root.publishedText(resultRow.modelData)
                                text: resultRow.modelData.live ? qsTr("Live now")
                                    : [views, published].filter(function(value) { return value.length > 0; }).join("  -  ")
                                visible: text.length > 0
                                color: resultRow.modelData.live ? Theme.accent : Theme.mutedText
                                font.pixelSize: Theme.fontSizeTiny
                                font.bold: resultRow.modelData.live
                                elide: Text.ElideRight
                            }
                        }
                        Text { text: qsTr("Play"); color: Theme.accent; font.bold: true }
                    }
                }
                ScrollBar.vertical: ScrollBar {}
            }
        }
    }
}
