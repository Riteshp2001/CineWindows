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
import QtQuick.Window
import CineWindows

Item {
    id: root
    required property ViewportMetrics metrics
    /// The player instance for accessing current media title and state
    property var player
    property var updateService
    property bool hubVisible: false
    property bool clientSideDecorated: true
    property string decorationStyle: "generic"
    /// Whether the Open menu is currently open
    readonly property bool openMenuOpen: openMenu.visible
    /// Whether the main menu is currently open
    readonly property bool mainMenuOpen: mainMenu.visible
    signal openFiles
    signal openFolder
    signal openUrl
    signal showPlaylist
    signal showMediaHub(string section)
    signal showPreferences
    signal showAdvancedPlayback
    signal youtubeSearchRequested
    signal subtitleSearchRequested
    signal remoteControlRequested
    signal castRequested
    signal showShortcuts
    signal showAbout
    signal closeRequested
    signal toggleMaximizeRequested
    signal minimizeWindowRequested
    signal addFiles
    signal addFolder
    signal addUrl
    signal addSubtitleTrack
    signal addAudioTrack
    signal newWindowRequested
    signal saveSessionRequested

    height: metrics.titleBarHeight

    CineMenu {
        id: openMenu
        menuWidth: 224
        CineMenuItem {
            text: qsTr("Open Files")
            shortcut: "Ctrl+O"
            onTriggered: root.openFiles()
        }
        CineMenuItem {
            text: qsTr("Open Folder")
            shortcut: "Ctrl+I"
            onTriggered: root.openFolder()
        }
        CineMenuItem {
            text: qsTr("Open URL")
            shortcut: "Ctrl+U"
            onTriggered: root.openUrl()
        }
        MenuSeparator {}
        CineMenuItem {
            text: qsTr("Add Files")
            shortcut: "Ctrl+Shift+O"
            onTriggered: root.addFiles()
        }
        CineMenuItem {
            text: qsTr("Add Folder")
            shortcut: "Ctrl+Shift+I"
            onTriggered: root.addFolder()
        }
        CineMenuItem {
            text: qsTr("Add URL")
            shortcut: "Ctrl+Shift+U"
            onTriggered: root.addUrl()
        }
        MenuSeparator {}
        CineMenuItem {
            text: qsTr("Add Subtitle Track")
            onTriggered: root.addSubtitleTrack()
        }
        CineMenuItem {
            visible: root.player && root.player.currentPath.length > 0
            text: qsTr("Download Subtitles")
            onTriggered: root.subtitleSearchRequested()
        }
        CineMenuItem {
            text: qsTr("Add Audio Track")
            onTriggered: root.addAudioTrack()
        }
    }

    CineMenu {
        id: mainMenu
        menuWidth: 236
        CineMenuItem {
            visible: root.updateService && (root.updateService.updateAvailable
                     || root.updateService.state === UpdateService.Downloading
                     || root.updateService.state === UpdateService.ReadyToInstall)
            text: root.updateService && root.updateService.state === UpdateService.ReadyToInstall
                  ? qsTr("Install Update")
                  : root.updateService && root.updateService.state === UpdateService.Downloading
                    ? qsTr("Downloading Update...")
                    : qsTr("Update to %1").arg(root.updateService ? root.updateService.latestVersion : "")
            enabled: root.updateService && root.updateService.state !== UpdateService.Downloading
            onTriggered: {
                if (root.updateService.state === UpdateService.ReadyToInstall)
                    root.updateService.installUpdate();
                else
                    root.updateService.downloadUpdate();
            }
        }
        CineMenuItem {
            text: qsTr("Search YouTube")
            onTriggered: root.youtubeSearchRequested()
        }
        MenuSeparator {
            visible: root.updateService && root.updateService.updateAvailable
            implicitHeight: visible ? 8 : 0
        }
        CineMenuItem {
            text: qsTr("New Window")
            shortcut: "Ctrl+N"
            onTriggered: root.newWindowRequested()
        }
        CineMenuItem {
            text: qsTr("Playlist")
            shortcut: "Ctrl+P"
            onTriggered: root.showPlaylist()
        }
        CineMenuItem {
            text: qsTr("Media Library")
            shortcut: "Ctrl+L"
            onTriggered: root.showMediaHub("library")
        }
        CineMenuItem {
            text: qsTr("History")
            shortcut: "Ctrl+H"
            onTriggered: root.showMediaHub("history")
        }
        CineMenuItem {
            text: qsTr("Preferences")
            shortcut: "Ctrl+,"
            onTriggered: root.showPreferences()
        }
        CineMenuItem {
            text: qsTr("Equalizer")
            onTriggered: root.showAdvancedPlayback()
        }
        CineMenuItem {
            text: qsTr("Keyboard Shortcuts")
            shortcut: "Ctrl+/"
            onTriggered: root.showShortcuts()
        }
        CineMenuItem {
            text: qsTr("Companion Remote")
            onTriggered: root.remoteControlRequested()
        }
        CineMenuItem {
            visible: root.player && root.player.currentPath.length > 0
            text: qsTr("Cast to Device")
            onTriggered: root.castRequested()
        }
        MenuSeparator {}
        CineMenuItem {
            text: qsTr("About CineWindows")
            onTriggered: root.showAbout()
        }
        MenuSeparator {}
        CineMenuItem {
            text: qsTr("Save Session and Close")
            shortcut: "Shift+Q"
            onTriggered: root.saveSessionRequested()
        }
    }

    Item {
        id: splitOpenControl
        objectName: "headerLeftGroup"
        anchors.left: parent.left
        anchors.leftMargin: root.metrics.spacingMd
        anchors.verticalCenter: parent.verticalCenter
        width: root.metrics.widthClass === ViewportMetrics.CompactWidth
            ? root.metrics.controlStandard * 2 + root.metrics.spacingSm
            : root.metrics.controlStandard * 4 + root.metrics.spacingSm
        height: root.metrics.controlStandard
        visible: root.player && root.player.currentPath.length > 0 && !root.player.idle

        AbstractButton {
            id: backSegment
            objectName: "backSegment"
            width: root.metrics.controlStandard
            height: parent.height
            hoverEnabled: true
            focusPolicy: Qt.TabFocus
            HoverHandler { cursorShape: Qt.PointingHandCursor }
            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Stop and return home")
            onClicked: root.showMediaHub("home")
            background: Rectangle {
                property color fillColor: backSegment.down ? Theme.headerActive
                    : backSegment.hovered ? Theme.headerHover : "transparent"
                radius: height / 2
                color: fillColor
                border.width: 1
                border.color: backSegment.activeFocus
                    ? Qt.alpha(Theme.accent, 0.78) : "transparent"
                Behavior on fillColor {
                    ColorAnimation { duration: Theme.motionFast; easing.type: Easing.OutCubic }
                }
            }
            contentItem: Item {
                CineIcon {
                    objectName: "backSegmentIcon"
                    anchors.centerIn: parent
                    width: root.metrics.iconSm
                    height: width
                    name: "cine-go-back-symbolic"
                    filled: backSegment.hovered || backSegment.down || backSegment.activeFocus
                    tint: Theme.headerIcon
                }
            }
        }

        AbstractButton {
            id: openSegment
            objectName: "openSegment"
            anchors.left: backSegment.right
            anchors.leftMargin: root.metrics.spacingSm
            anchors.right: parent.right
            height: parent.height
            hoverEnabled: true
            focusPolicy: Qt.TabFocus
            HoverHandler { cursorShape: Qt.PointingHandCursor }
            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Open media")
            onClicked: openMenu.openBelowLeft(openSegment, root.Window.window ? root.Window.window.contentItem : root)
            background: Rectangle {
                property color fillColor: openSegment.down ? Theme.headerActive
                    : openSegment.hovered ? Theme.headerHover : Theme.headerBg
                radius: height / 2
                color: fillColor
                border.width: 1
                border.color: openSegment.activeFocus
                    ? Qt.alpha(Theme.accent, 0.78) : Theme.headerBorder
                Behavior on fillColor {
                    ColorAnimation { duration: Theme.motionFast; easing.type: Easing.OutCubic }
                }
            }
            contentItem: Item {
                Row {
                    anchors.centerIn: parent
                    spacing: root.metrics.spacingSm
                    CineIcon {
                        objectName: "openSegmentIcon"
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.max(18, root.metrics.iconSm)
                        height: width
                        name: "cine-folder-symbolic"
                        filled: false
                        tint: Theme.headerIcon
                    }
                    Text {
                        objectName: "openSegmentLabel"
                        visible: root.metrics.widthClass !== ViewportMetrics.CompactWidth
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Open")
                        color: Theme.headerText
                        font.bold: true
                        font.pixelSize: Theme.fontSizeSmall
                    }
                    CineIcon {
                        objectName: "openSegmentChevron"
                        visible: root.metrics.widthClass !== ViewportMetrics.CompactWidth
                        anchors.verticalCenter: parent.verticalCenter
                        name: "cine-dropdown-symbolic"
                        tint: Theme.headerChevron
                        width: 11
                        height: 11
                    }
                }
            }
        }
    }

    Text {
        id: titleLabel
        objectName: "headerTitle"
        readonly property real availableLeft: splitOpenControl.visible
            ? splitOpenControl.x + splitOpenControl.width + root.metrics.spacingSm
            : root.metrics.spacingMd
        readonly property real availableRight: windowControls.x - root.metrics.spacingSm
        readonly property real availableWidth: Math.max(0, availableRight - availableLeft)
        readonly property real preferredWidth: Math.min(root.width * 0.45, 560)
        x: Math.round(Math.max(availableLeft,
            Math.min((root.width - width) / 2, availableRight - width)))
        y: Math.round((root.height - height) / 2)
        text: root.player && root.player.mediaTitle.length > 0
            ? root.player.mediaTitle
            : Qt.application.displayName
        color: Theme.headerText
        font.bold: true
        font.pixelSize: root.metrics.fontBody
        elide: Text.ElideMiddle
        width: Math.min(preferredWidth, availableWidth)
        visible: !root.hubVisible && width >= root.metrics.iconMd
        horizontalAlignment: Text.AlignHCenter
        style: Text.Outline
        styleColor: Theme.headerTitleOutline
    }

    Item {
        anchors.fill: parent
        z: -1
        TapHandler {
            enabled: root.clientSideDecorated
            onTapped: if (tapCount === 2)
                root.toggleMaximizeRequested()
            gesturePolicy: TapHandler.DragThreshold
        }
        DragHandler {
            enabled: root.clientSideDecorated
            target: null
            onActiveChanged: if (active && root.Window.window)
                root.Window.window.startSystemMove()
        }
    }

    Row {
        id: windowControls
        objectName: "headerRightGroup"
        anchors.right: parent.right
        anchors.rightMargin: root.metrics.spacingMd
        anchors.verticalCenter: parent.verticalCenter
        spacing: root.decorationStyle === "kde" ? root.metrics.spacingXs : root.metrics.spacingSm

        CineButton {
            id: menuButton
            objectName: "menuButton"
            styleVariant: "windowCtrl"
            metrics: root.metrics
            windowControlRole: CineButton.MenuControl
            windowControlTint: root.hubVisible ? Theme.text : Theme.headerIcon
            btnTooltip: qsTr("Main Menu")
            tooltipBelow: true
            onClicked: mainMenu.openBelowRight(menuButton, root.Window.window ? root.Window.window.contentItem : root)
        }

        CineButton {
            id: minimizeButton
            objectName: "minimizeButton"
            visible: root.clientSideDecorated
            styleVariant: "windowCtrl"
            metrics: root.metrics
            buttonSize: root.decorationStyle === "kde"
                ? root.metrics.controlStandard
                : root.metrics.controlCompact
            squareWindowControl: root.decorationStyle === "kde"
            windowControlRole: CineButton.MinimizeControl
            windowControlTint: root.hubVisible ? Theme.text : Theme.headerIcon
            btnTooltip: qsTr("Minimize")
            tooltipBelow: true
            onClicked: root.minimizeWindowRequested()
        }

        CineButton {
            id: maximizeButton
            objectName: "maximizeButton"
            visible: root.clientSideDecorated
            styleVariant: "windowCtrl"
            metrics: root.metrics
            buttonSize: root.decorationStyle === "kde"
                ? root.metrics.controlStandard
                : root.metrics.controlCompact
            squareWindowControl: root.decorationStyle === "kde"
            windowControlRole: root.Window.window
                && (root.Window.window.visibility === Window.Maximized
                    || root.Window.window.visibility === Window.FullScreen)
                ? CineButton.RestoreControl
                : CineButton.MaximizeControl
            windowControlTint: root.hubVisible ? Theme.text : Theme.headerIcon
            btnTooltip: windowControlRole === CineButton.RestoreControl
                ? qsTr("Restore")
                : qsTr("Maximize")
            tooltipBelow: true
            onClicked: root.toggleMaximizeRequested()
        }

        CineButton {
            id: closeButton
            objectName: "closeButton"
            visible: root.clientSideDecorated
            styleVariant: "close"
            metrics: root.metrics
            buttonSize: root.decorationStyle === "kde"
                ? root.metrics.controlStandard
                : root.metrics.controlCompact
            squareWindowControl: root.decorationStyle === "kde"
            windowControlRole: CineButton.CloseControl
            windowControlTint: closeButton.hovered ? "white"
                : root.hubVisible ? Theme.text : Theme.headerIcon
            btnTooltip: qsTr("Close")
            tooltipBelow: true
            onClicked: root.closeRequested()
        }
    }
}
