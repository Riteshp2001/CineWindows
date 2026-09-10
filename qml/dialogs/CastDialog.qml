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
    property var castService
    property string mediaUrl: ""
    parent: Overlay.overlay
    metrics: ViewportMetrics {
        viewportWidth: root.parent ? root.parent.width : root.preferredWidth
        viewportHeight: root.parent ? root.parent.height : root.preferredHeight
    }
    preferredWidth: 520
    preferredHeight: 420
    modal: true
    focus: true
    padding: 18
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle {
        radius: 16
        color: Theme.popover
        border.color: Theme.popoverBorder
    }
    onOpened: {
        if (castService)
            castService.discover()
        if (scanButton.enabled)
            scanButton.forceActiveFocus()
        else
            deviceList.forceActiveFocus()
    }
    contentItem: ColumnLayout {
        objectName: "dialogSurface"
        spacing: 12
        Accessible.role: Accessible.Dialog
        Accessible.name: qsTr("Cast to Device")

        GridLayout {
            objectName: "actionLayout"
            Layout.fillWidth: true
            columns: root.compactLayout ? 1 : 2
            columnSpacing: 12
            rowSpacing: 12

            Text {
                Layout.fillWidth: true
                text: qsTr("Cast to Device")
                color: Theme.text
                font.pixelSize: 20
                font.bold: true
                wrapMode: Text.WordWrap
            }
            Controls.CineButton {
                id: scanButton
                objectName: "initialFocusControl"
                Layout.fillWidth: root.compactLayout
                styleVariant: "dialog"
                colorVariant: "primary"
                btnText: root.castService && root.castService.discovering
                    ? qsTr("Searching...") : qsTr("Scan Again")
                focusPolicy: Qt.StrongFocus
                enabled: !!root.castService && !root.castService.discovering
                onClicked: root.castService.discover()
            }
        }
        Text {
            Layout.fillWidth: true
            text: qsTr("Discover DLNA renderers on your local network. Direct web media URLs are supported in this release.")
            color: Theme.mutedText
            wrapMode: Text.WordWrap
        }
        Text {
            Layout.fillWidth: true
            text: root.castService ? root.castService.statusMessage : ""
            color: Theme.mutedText
            wrapMode: Text.WordWrap
        }
        ListView {
            id: deviceList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 6
            activeFocusOnTab: true
            Accessible.name: qsTr("Available cast devices")
            model: root.castService ? root.castService.devices : []
            delegate: ItemDelegate {
                id: deviceRow
                required property int index
                required property var modelData
                width: ListView.view.width
                height: 58
                Accessible.name: modelData.name
                HoverHandler { cursorShape: Qt.PointingHandCursor }
                onClicked: root.castService.castUrl(index, root.mediaUrl)
                background: Rectangle {
                    radius: Theme.radius
                    color: deviceRow.hovered ? Theme.cardHover : Theme.cardStrong
                }
                contentItem: RowLayout {
                    Text {
                        text: deviceRow.modelData.name
                        color: Theme.text
                        font.bold: true
                        Layout.fillWidth: true
                    }
                    Text { text: qsTr("Cast"); color: Theme.accent; font.bold: true }
                }
            }
            ScrollBar.vertical: ScrollBar {}
        }
    }
}
