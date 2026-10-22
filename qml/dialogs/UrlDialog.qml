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
    property string actionText: qsTr("Open")
    readonly property string normalizedInput: fileService
        ? fileService.normalizeMediaPath(field.text) : field.text.trim()
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
    preferredWidth: 560
    preferredHeight: (width < 360 ? 300 : 246)
        + (field.text.trim().length > 0 && !canAccept() ? 28 : 0)
    padding: 0

    background: Rectangle {
        radius: Theme.radius
        color: Theme.popover
        border.color: Theme.popoverBorder
        border.width: 1
    }

    /// Returns true if the current text field value is a valid/acceptable URL
    function canAccept() {
        return normalizedInput.length > 0;
    }

    /// Emits acceptedUrl and closes the dialog if the input is valid
    function acceptUrl() {
        if (!canAccept())
            return;
        acceptedUrl(normalizedInput);
        field.text = "";
        root.close();
    }

    onOpened: {
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
            Layout.preferredHeight: 56

            Text {
                id: titleText
                anchors.left: parent.left
                anchors.right: closeButton.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 20
                anchors.rightMargin: 8
                text: root.title
                color: Theme.text
                font.pixelSize: 17
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
        Controls.CineScrollView {
            id: bodyScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: bodyScroll.availableWidth
                spacing: 16

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.topMargin: 20
                    Layout.preferredHeight: inputSection.implicitHeight + 32
                    radius: Theme.radius
                    color: Theme.card

                    ColumnLayout {
                        id: inputSection
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 16
                        spacing: 10

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("URL or media path")
                            color: Theme.text
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
                        }
                        TextField {
                            id: field
                            objectName: "initialFocusControl"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 44
                            Accessible.name: qsTr("URL or media path")
                            placeholderText: "https://"
                            placeholderTextColor: Theme.mutedText
                            selectByMouse: true
                            color: Theme.text
                            leftPadding: 12
                            rightPadding: 12
                            background: Rectangle {
                                color: Theme.cardStrong
                                border.color: field.activeFocus ? Theme.focusRing : Theme.separator
                                border.width: field.activeFocus ? Theme.focusRingWidth : 1
                                radius: Theme.radius
                                Behavior on border.color {
                                    ColorAnimation { duration: Theme.motionFast }
                                }
                            }
                            onAccepted: root.acceptUrl()
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: field.text.trim().length > 0 && !root.canAccept()
                            text: qsTr("This URL or media file is unavailable.")
                            color: Theme.danger
                            font.pixelSize: Theme.fontSizeCaption
                            wrapMode: Text.Wrap
                            textFormat: Text.PlainText
                        }
                    }
                }
                GridLayout {
                    id: actions
                    objectName: "actionLayout"
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    columns: root.width < 360 ? 1 : 3
                    columnSpacing: 10
                    rowSpacing: 10

                    Item { Layout.fillWidth: true; visible: root.width >= 360 }
                    Controls.CineButton {
                        id: cancelButton
                        Layout.fillWidth: root.width < 360
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
                        Layout.fillWidth: root.width < 360
                        Layout.preferredWidth: Math.max(88, implicitWidth)
                        Layout.preferredHeight: 34
                        styleVariant: "dialog"
                        colorVariant: "primary"
                        btnText: root.actionText
                        focusPolicy: Qt.StrongFocus
                        enabled: root.canAccept()
                        onClicked: root.acceptUrl()
                    }
                }
                Item { Layout.preferredHeight: 4 }
            }
        }
    }
}
