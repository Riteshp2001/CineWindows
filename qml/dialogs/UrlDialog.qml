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
    /// The URL text being edited (aliased to the text field)
    property alias urlText: field.text
    /// Title displayed in the dialog header
    property string title: qsTr("Open URL")
    /// Service used to validate and interact with URLs
    property var fileService
    /// Emitted with the accepted URL string when user confirms
    signal acceptedUrl(string url)

    metrics: ViewportMetrics {
        viewportWidth: root.parent ? root.parent.width : root.preferredWidth
        viewportHeight: root.parent ? root.parent.height : root.preferredHeight
    }
    modal: true
    dim: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    preferredWidth: 480
    preferredHeight: 226
    padding: 0

    background: Rectangle {
        radius: 14
        color: Theme.popover
        opacity: 0.98
        border.color: Theme.popoverBorder
        border.width: 1
    }

    /// Returns true if the current text field value is a valid/acceptable URL
    function canAccept() {
        var value = field.text.trim();
        return fileService ? fileService.isSupportedUrl(value) : value.length > 0;
    }

    /// Emits acceptedUrl and closes the dialog if the input is valid
    function acceptUrl() {
        if (!canAccept())
            return;
        acceptedUrl(field.text.trim());
        field.text = "";
        root.close();
    }

    onOpened: {
        if (fileService && field.text.trim().length === 0) {
            var clip = fileService.clipboardText();
            if (fileService.isSupportedUrl(clip))
                field.text = clip;
        }
        field.forceActiveFocus();
        field.selectAll();
    }

    contentItem: ColumnLayout {
        objectName: "dialogSurface"
        spacing: 0
        Accessible.role: Accessible.Dialog
        Accessible.name: root.title

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 54

            Text {
                id: titleText
                anchors.left: parent.left
                anchors.right: closeButton.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 20
                anchors.rightMargin: 8
                text: root.title
                color: Theme.text
                font.pixelSize: 16
                font.bold: true
                elide: Text.ElideRight
            }
            Controls.CineButton {
                id: closeButton
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 12
                styleVariant: "close"
                iconName: "cine-close-symbolic"
                btnTooltip: qsTr("Close")
                focusPolicy: Qt.StrongFocus
                onClicked: root.close()
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.separator
        }
        ScrollView {
            id: bodyScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: bodyScroll.availableWidth
                spacing: 8

                Item { Layout.preferredHeight: 15 }
                TextField {
                    id: field
                    objectName: "initialFocusControl"
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.preferredHeight: 44
                    placeholderText: qsTr("Paste a stream URL or media path")
                    placeholderTextColor: "#7f838c"
                    selectByMouse: true
                    color: Theme.text
                    leftPadding: 13
                    rightPadding: 13
                    verticalAlignment: TextInput.AlignVCenter
                    background: Rectangle {
                        color: Theme.cardStrong
                        border.color: field.activeFocus ? Theme.accent : (field.hovered ? Theme.chipBorder : Theme.popoverBorder)
                        border.width: 1
                        radius: 9
                        Behavior on border.color {
                            ColorAnimation {
                                duration: Theme.motionNormal
                                easing.type: Easing.OutCubic
                            }
                        }
                    }
                    onAccepted: root.acceptUrl()
                }
                Text {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.preferredHeight: 32
                    opacity: field.text.trim().length > 0 && !root.canAccept() ? 1 : 0
                    text: qsTr("Enter a supported stream URL or local media path.")
                    color: Theme.danger
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    Behavior on opacity {
                        NumberAnimation {
                            duration: Theme.motionFast
                            easing.type: Easing.OutCubic
                        }
                    }
                }
                GridLayout {
                    id: actions
                    objectName: "actionLayout"
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    columns: root.compactLayout ? 1 : 2
                    columnSpacing: 10
                    rowSpacing: 10

                    Controls.CineButton {
                        id: cancelButton
                        Layout.fillWidth: root.compactLayout
                        Layout.preferredWidth: 88
                        Layout.preferredHeight: 34
                        styleVariant: "dialog"
                        colorVariant: "default"
                        btnText: qsTr("Cancel")
                        focusPolicy: Qt.StrongFocus
                        onClicked: root.close()
                    }
                    Controls.CineButton {
                        id: openButton
                        Layout.fillWidth: root.compactLayout
                        Layout.preferredWidth: 88
                        Layout.preferredHeight: 34
                        styleVariant: "dialog"
                        colorVariant: "primary"
                        btnText: qsTr("Open")
                        focusPolicy: Qt.StrongFocus
                        enabled: root.canAccept()
                        onClicked: root.acceptUrl()
                    }
                }
                Item { Layout.preferredHeight: 10 }
            }
        }
    }
}
