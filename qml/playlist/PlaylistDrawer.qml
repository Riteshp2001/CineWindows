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

Popup {
    id: root
    /// The playlist model being displayed
    property var playlist
    /// The player controller for playback actions
    property var controller
    /// Service for file operations (open location, etc.)
    property var fileService

    /// Emitted when user requests adding files to the playlist
    signal addFilesRequested
    /// Emitted when user requests adding a folder to the playlist
    signal addFolderRequested
    /// Emitted when user requests adding a URL to the playlist
    signal addUrlRequested
    /// Emitted when user requests saving the playlist
    signal savePlaylistRequested

    parent: Overlay.overlay
    x: 0
    y: 0
    width: parent ? parent.width : 900
    height: parent ? parent.height : 620
    padding: 0
    modal: true
    dim: false
    closePolicy: Popup.CloseOnEscape

    /// Returns a localized string showing the item count (e.g. "3 items")
    function itemCountText() {
        if (!playlist || playlist.count === 0)
            return qsTr("0 items");
        if (playlist.count === 1)
            return qsTr("1 item");
        return qsTr("%1 items").arg(playlist.count);
    }

    onOpened: {
        if (playlist && playlist.currentIndex >= 0)
            Qt.callLater(function () {
                list.positionViewAtIndex(playlist.currentIndex, ListView.Center);
            });
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                target: bgRect
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: Theme.motionPanel
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                target: panelTranslation
                property: "y"
                from: panel.height
                to: 0
                duration: Theme.motionPanel
                easing.type: Easing.OutCubic
            }
        }
    }

    exit: Transition {
        ParallelAnimation {
            NumberAnimation {
                target: bgRect
                property: "opacity"
                from: 1.0
                to: 0.0
                duration: Theme.motionPanel
                easing.type: Easing.InCubic
            }
            NumberAnimation {
                target: panelTranslation
                property: "y"
                from: 0
                to: panel.height
                duration: Theme.motionPanel
                easing.type: Easing.InCubic
            }
        }
    }

    background: Rectangle {
        id: bgRect
        color: "#b0000000"
    }

    CineMenu {
        id: addMenu
        CineMenuItem {
            text: qsTr("Add Files")
            onTriggered: root.addFilesRequested()
        }
        CineMenuItem {
            text: qsTr("Add Folder")
            onTriggered: root.addFolderRequested()
        }
        CineMenuItem {
            text: qsTr("Add URL")
            onTriggered: root.addUrlRequested()
        }
    }




    contentItem: Item {
        id: stage
        width: root.width
        height: root.height

        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
        }

        Item {
            id: panel
            z: 1
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            width: Math.min(610, parent.width - 40)
            height: Math.min(parent.height * 0.65, 480)

            transform: Translate {
                id: panelTranslation
                y: 0
            }

            Rectangle {
                id: panelBg
                anchors.fill: parent
                anchors.bottomMargin: -20
                color: Theme.playlistBg
                radius: 16
                border.color: "#28ffffff"
                border.width: 1

                MouseArea {
                    anchors.fill: parent
                    anchors.bottomMargin: 20
                    propagateComposedEvents: false
                    onClicked: {}
                }
            }

            Rectangle {
                id: grabber
                width: 36
                height: 4
                radius: 2
                color: "#40ffffff"
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 8
            }

            Item {
                id: toolbar
                anchors.top: parent.top
                anchors.topMargin: 18
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.right: parent.right
                anchors.rightMargin: 16
                height: 36

                Row {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    CineButton {
                        id: addButton
                        styleVariant: "icon"
                        iconName: "cine-list-add-symbolic"
                        buttonSize: 30
                        iconSize: 15
                        btnTooltip: qsTr("Add Media")
                        onClicked: addMenu.openBelow(addButton, stage)
                    }

                    CineButton {
                        id: searchButton
                        styleVariant: "icon"
                        iconName: "edit-find-symbolic"
                        buttonSize: 30
                        iconSize: 15
                        btnTooltip: qsTr("Search Playlist")
                        checkable: true
                    }

                    CineButton {
                        styleVariant: "icon"
                        iconName: "document-edit-symbolic"
                        buttonSize: 30
                        iconSize: 15
                        btnTooltip: qsTr("Save Playlist")
                        onClicked: root.savePlaylistRequested()
                    }
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 1
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Playlist")
                        color: Theme.text
                        font.pixelSize: 14
                        font.bold: true
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: root.itemCountText()
                        color: Theme.mutedText
                        font.pixelSize: 10
                    }
                }

                CineButton {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    styleVariant: "close"
                    iconName: "cine-close-symbolic"
                    buttonSize: 30
                    iconSize: 15
                    btnTooltip: qsTr("Close")
                    onClicked: root.close()
                }
            }

            TextField {
                id: search
                anchors.top: toolbar.bottom
                anchors.topMargin: 8
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.right: parent.right
                anchors.rightMargin: 16
                visible: height > 0
                height: searchButton.checked ? 34 : 0
                clip: true
                placeholderText: qsTr("Search")
                color: Theme.text
                placeholderTextColor: Theme.mutedText
                leftPadding: 12
                rightPadding: 12
                background: Rectangle {
                    color: "#bf1b1b1e"
                    radius: 8
                    border.color: search.activeFocus ? "#66ffffff" : "#28ffffff"
                    border.width: 1
                }
                Behavior on height {
                    NumberAnimation {
                        duration: Theme.motionNormal
                        easing.type: Easing.OutCubic
                    }
                }
            }

            ListView {
                id: list
                anchors.top: search.bottom
                anchors.topMargin: searchButton.checked ? 12 : 20
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.right: parent.right
                anchors.rightMargin: 16
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 16
                model: root.playlist
                spacing: 0
                clip: true
                ScrollBar.vertical: ScrollBar {}

                delegate: Item {
                    id: delegateItem
                    required property string name
                    required property string path
                    required property bool playing
                    required property bool local
                    required property int index

                    property bool isMatch: search.text.length === 0 || delegateItem.name.toLowerCase().indexOf(search.text.toLowerCase()) !== -1

                    width: list.width
                    height: isMatch ? 55 : 0
                    visible: isMatch

                    Rectangle {
                        id: rowShadow
                        anchors.fill: contentRect
                        anchors.topMargin: 3
                        radius: contentRect.radius
                        color: "#80000000"
                        opacity: 0.35
                    }

                    Rectangle {
                        id: contentRect
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        height: 46
                        radius: 9
                        color: delegateItem.playing ? "#7a69696e" : mouseArea.containsMouse ? "#735f5f64" : "#6558585d"
                        border.color: delegateItem.playing ? "#66ffffff" : "#18ffffff"
                        border.width: 1

                        CineIcon {
                            id: typeIcon
                            anchors.left: contentRect.left
                            anchors.leftMargin: 14
                            anchors.verticalCenter: contentRect.verticalCenter
                            width: 15
                            height: 15
                            name: root.fileService ? root.fileService.iconNameForPath(delegateItem.path) : "cine-applications-multimedia-symbolic"
                            filled: mouseArea.containsMouse || mouseArea.pressed || delegateItem.playing
                            tint: Theme.text
                        }

                        Text {
                            anchors.left: typeIcon.right
                            anchors.leftMargin: 14
                            anchors.right: playButton.left
                            anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            text: delegateItem.name
                            color: Theme.text
                            elide: Text.ElideMiddle
                            font.pixelSize: 13
                            font.bold: delegateItem.playing
                        }

                        CineButton {
                            id: playButton
                            anchors.right: parent.right
                            anchors.rightMargin: 10
                            anchors.verticalCenter: parent.verticalCenter
                            styleVariant: "icon"
                            buttonSize: 26
                            iconSize: 15
                            iconName: delegateItem.playing ? "cine-playback-pause-symbolic" : "cine-playback-start-symbolic"
                            btnTooltip: delegateItem.playing ? qsTr("Pause") : qsTr("Play")
                            onClicked: delegateItem.playing ? root.controller.togglePause() : root.controller.playIndex(delegateItem.index)
                        }

                        CineMenu {
                            id: itemMenu
                            CineMenuItem {
                                text: delegateItem.playing ? qsTr("Pause") : qsTr("Play")
                                onTriggered: delegateItem.playing ? root.controller.togglePause() : root.controller.playIndex(delegateItem.index)
                            }
                            CineMenuItem {
                                text: delegateItem.local ? qsTr("Open Item Location") : qsTr("Open URL")
                                onTriggered: if (root.fileService)
                                    root.fileService.openLocation(delegateItem.path)
                            }
                            MenuSeparator {}
                            CineMenuItem {
                                text: qsTr("Remove from Playlist")
                                onTriggered: root.playlist.removeAt(delegateItem.index)
                            }
                        }

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            anchors.rightMargin: playButton.width + 14
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            onDoubleClicked: function (mouse) {
                                if (mouse.button === Qt.LeftButton)
                                    root.controller.playIndex(delegateItem.index);
                            }
                            onPressed: function (mouse) {
                                if (mouse.button === Qt.RightButton)
                                    itemMenu.popup(contentRect, mouse.x, mouse.y);
                            }

                            CineTooltip {
                                text: delegateItem.path
                                active: mouseArea.containsMouse
                            }
                        }
                    }
                }
            }

            Text {
                anchors.centerIn: list
                visible: root.playlist && root.playlist.count === 0
                text: qsTr("Playlist is empty")
                color: Theme.mutedText
                font.pixelSize: 14
                font.bold: true
            }

            Text {
                anchors.centerIn: list
                visible: root.playlist && root.playlist.count > 0 && search.text.length > 0 && root.playlist.matchCount(search.text) === 0
                text: qsTr("No results")
                color: Theme.mutedText
                font.pixelSize: 14
                font.bold: true
            }
        }
    }
}
