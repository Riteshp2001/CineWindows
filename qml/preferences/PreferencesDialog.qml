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
import "../controls"
import CineWindows

ResponsivePopup {
    id: root
    // Update service for checking/downloading updates
    property var updateService
    property var metadataService

    parent: Overlay.overlay
    metrics: ViewportMetrics {
        viewportWidth: root.parent ? root.parent.width : root.preferredWidth
        viewportHeight: root.parent ? root.parent.height : root.preferredHeight
    }
    modal: true
    dim: true
    preferredWidth: 460
    preferredHeight: 660
    padding: 0
    focus: true

    background: Rectangle {
        radius: 16
        color: Theme.popover
        opacity: 0.95
        border.color: Theme.popoverBorder
        border.width: 1
    }

    // Grouped settings card container
    component GroupCard: Rectangle {
        // Child content rows
        default property alias rows: col.data
        width: parent ? parent.width : 0
        radius: 12
        color: Theme.card
        clip: true
        height: col.implicitHeight
        Column {
            id: col
            width: parent.width
        }
    }

    // Horizontal separator line
    component Sep: Rectangle {
        width: parent.width
        height: 1
        color: Theme.separator
    }

    // Title with optional subtitle
    component TitleSub: Column {
        id: titleSub
        // Main title text
        property string title: ""
        // Optional secondary text
        property string subtitle: ""
        spacing: 2
        Text {
            width: parent.width
            text: titleSub.title
            color: Theme.text
            font.pixelSize: 15
            font.bold: true
            elide: Text.ElideRight
        }
        Text {
            visible: titleSub.subtitle.length > 0
            text: titleSub.subtitle
            color: Theme.mutedText
            font.pixelSize: 12
            width: parent.width
            wrapMode: Text.WordWrap
        }
    }

    // Settings row with toggle switch
    component SwitchRow: Item {
        id: switchRow
        // Label text
        property string title: ""
        // Optional description
        property string subtitle: ""
        // Toggle state
        property bool checked: false
        // Emitted when switch changes
        signal toggled(bool value)
        width: parent ? parent.width : 0
        height: visible ? Math.max(58, label.implicitHeight + 24) : 0
        TitleSub {
            id: label
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.right: rowSwitch.left
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            title: switchRow.title
            subtitle: switchRow.subtitle
        }
        CineSwitch {
            id: rowSwitch
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            checked: parent.checked
            onToggled: parent.toggled(checked)
        }
    }

    // Settings row with custom right-side controls
    component ActionRow: Item {
        id: actionRow
        // Label text
        property string title: ""
        // Optional description
        property string subtitle: ""
        // Custom control widget
        default property alias control: holder.data
        width: parent ? parent.width : 0
        height: visible ? (root.compactLayout
            ? lbl.implicitHeight + holder.implicitHeight + 34
            : Math.max(58, lbl.implicitHeight + 24)) : 0
        TitleSub {
            id: lbl
            objectName: actionRow.objectName.length > 0 ? actionRow.objectName + "Label" : ""
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.right: root.compactLayout ? parent.right : holder.left
            anchors.rightMargin: root.compactLayout ? 16 : 12
            anchors.top: root.compactLayout ? parent.top : undefined
            anchors.topMargin: root.compactLayout ? 10 : 0
            anchors.verticalCenter: root.compactLayout ? undefined : parent.verticalCenter
            title: actionRow.title
            subtitle: actionRow.subtitle
        }
        Row {
            id: holder
            objectName: actionRow.objectName.length > 0 ? actionRow.objectName + "Controls" : ""
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.bottom: root.compactLayout ? parent.bottom : undefined
            anchors.bottomMargin: root.compactLayout ? 10 : 0
            anchors.verticalCenter: root.compactLayout ? undefined : parent.verticalCenter
            spacing: 8
        }
    }

    // Settings row with dropdown selector
    component ComboRow: Item {
        id: comboRow
        // Label text
        property string title: ""
        // Optional description
        property string subtitle: ""
        // Dropdown items model
        property var model: []
        // Currently selected index
        property alias currentIndex: combo.currentIndex
        // Model role for display text
        property string textRole: ""
        // Emitted when selection changes
        signal activated(int index)

        width: parent ? parent.width : 0
        height: visible ? (root.compactLayout
            ? lbl.implicitHeight + combo.height + 34
            : Math.max(58, lbl.implicitHeight + 24)) : 0

        TitleSub {
            id: lbl
            objectName: comboRow.objectName.length > 0 ? comboRow.objectName + "Label" : ""
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.right: root.compactLayout ? parent.right : combo.left
            anchors.rightMargin: root.compactLayout ? 16 : 12
            anchors.top: root.compactLayout ? parent.top : undefined
            anchors.topMargin: root.compactLayout ? 10 : 0
            anchors.verticalCenter: root.compactLayout ? undefined : parent.verticalCenter
            title: parent.title
            subtitle: parent.subtitle
        }

        ComboBox {
            id: combo
            objectName: comboRow.objectName.length > 0 ? comboRow.objectName + "Control" : ""
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.bottom: root.compactLayout ? parent.bottom : undefined
            anchors.bottomMargin: root.compactLayout ? 10 : 0
            anchors.verticalCenter: root.compactLayout ? undefined : parent.verticalCenter
            width: 168
            height: 36
            focusPolicy: Qt.NoFocus
            textRole: parent.textRole
            model: parent.model
            onActivated: parent.activated(currentIndex)
            background: Rectangle {
                radius: Theme.radius
                color: combo.down ? Theme.glassActive : (combo.hovered ? Theme.cardHover : Theme.cardStrong)
                border.color: combo.popup.visible ? Theme.accent : "transparent"
                border.width: 1
            }
            contentItem: Text {
                leftPadding: 12
                rightPadding: 28
                text: combo.displayText
                color: Theme.text
                font.pixelSize: 13
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
            indicator: CineIcon {
                x: combo.width - width - 10
                y: (combo.height - height) / 2
                name: "cine-dropdown-symbolic"
                tint: Theme.mutedText
                width: 11
                height: 11
            }
            delegate: ItemDelegate {
                id: comboItem
                // Current item data
                required property var modelData
                // Current item index
                required property int index
                width: ListView.view ? ListView.view.width : combo.width
                height: 36
                highlighted: combo.highlightedIndex === index
                background: Rectangle {
                    anchors.fill: parent
                    anchors.margins: 4
                    radius: 7
                    color: comboItem.highlighted ? Theme.cardHover : "transparent"
                }
                contentItem: Text {
                    leftPadding: 8
                    text: combo.textRole ? comboItem.modelData[combo.textRole] : comboItem.modelData
                    color: Theme.text
                    font.pixelSize: 13
                    font.bold: combo.currentIndex === comboItem.index
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }
            popup: Popup {
                y: combo.height + 6
                width: Math.max(combo.width, 150)
                x: combo.width - width
                implicitHeight: Math.min(comboList.contentHeight + 8, 260)
                padding: 4
                background: Item {
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 4
                        radius: 12
                        color: Theme.popoverShadow
                    }
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 4
                        radius: 12
                        color: Theme.popover
                        opacity: 0.95
                        border.color: Theme.popoverBorder
                        border.width: 1
                    }
                }
                contentItem: ListView {
                    id: comboList
                    clip: true
                    implicitHeight: contentHeight
                    model: combo.delegateModel
                    currentIndex: combo.highlightedIndex
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar {
                        width: 6
                    }
                }
            }
        }
    }

    // Reset-to-default button
    component UndoBtn: CineButton {
        id: undoBtn
        // Reset callback
        // qmllint disable use-proper-function
        property var callback: function () {}
        styleVariant: "icon"
        iconName: "edit-undo-symbolic"
        buttonSize: 28
        iconSize: 12
        btnTooltip: qsTr("Reset")
        onClicked: callback()
    }

    // Adwaita-style ColorDialogButton: a rounded color chip with a subtle border
    // that opens the system color dialog when clicked.
    // Color swatch that opens a color picker
    component ColorChip: Rectangle {
        id: chip
        // Currently selected color
        property color value: "#ffffff"
        // Allow click interaction
        property bool interactive: true
        // Emitted when the chip is clicked
        signal picked
        width: 40
        height: 30
        radius: 8
        color: value
        opacity: interactive ? 1 : 0.4
        border.width: chipMouse.containsMouse && interactive ? 2 : 1
        border.color: chipMouse.containsMouse && interactive ? Theme.accent : Theme.separator
        Behavior on border.color {
            ColorAnimation {
                duration: Theme.motionFast
                easing.type: Easing.OutCubic
            }
        }
        Behavior on opacity {
            NumberAnimation {
                duration: Theme.motionNormal
                easing.type: Easing.OutCubic
            }
        }

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            border.width: 1
            border.color: Theme.chipBorder
        }

        MouseArea {
            id: chipMouse
            anchors.fill: parent
            hoverEnabled: true
            enabled: chip.interactive
            cursorShape: chip.interactive ? Qt.PointingHandCursor : Qt.ArrowCursor
                    // Emit picked signal on click
                    onClicked: chip.picked()
        }
    }

    // Compact icon-only button
    component MiniBtn: CineButton {
        styleVariant: "icon"
        buttonSize: 36
        width: 36
        height: 30
        iconSize: 16
    }

    // Subtitle color picker dialog
    SwatchColorDialog {
        id: colorDialog
        objectName: "subtitleColorDialog"
        metrics: root.metrics
        title: qsTr("Subtitle Color")
        // Apply selected color
        onAccepted: function (c) {
            SettingsManager.subtitleColor = c.toString();
        }
    }

    // Subtitle background color picker
    HsvColorDialog {
        id: bgColorDialog
        objectName: "subtitleBackgroundDialog"
        metrics: root.metrics
        title: qsTr("Subtitle Background")
        useAlpha: true
        // Apply selected background color
        onAccepted: function (c) {
            SettingsManager.subtitleBackgroundColor = c.toString();
        }
    }

    // Accent color picker
    HsvColorDialog {
        id: accentColorDialog
        objectName: "accentColorDialog"
        metrics: root.metrics
        title: qsTr("Accent Color")
        useAlpha: false
        // Apply selected accent color
        onAccepted: function (c) {
            SettingsManager.accentColor = c.toString();
        }
    }

    // Font picker (lazy loaded)
    LazyPopupLoader {
        id: fontDialog
        objectName: "fontDialogLoader"
        sourceComponent: Component {
            FontPickerDialog {
                metrics: root.metrics
                // Apply selected font family
                onAccepted: function (family) {
                    SettingsManager.subtitleFont = family;
                }
            }
        }
    }

    // Dialog header bar
    Item {
        id: headerBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 52
        Text {
            anchors.centerIn: parent
            text: qsTr("Preferences")
            color: Theme.text
            font.pixelSize: 17
            font.bold: true
        }
        // Close dialog button
        CineButton {
            id: closeBtn
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

    // Scrollable settings content area
    Flickable {
        id: preferencesFlickable
        anchors.top: headerBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 12
        anchors.topMargin: 16
        anchors.bottomMargin: 16
        readonly property int scrollbarGutter: 16
        contentWidth: Math.max(0, width - scrollbarGutter)
        contentHeight: content.implicitHeight + 8
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {
            id: preferencesScrollBar
            width: 8
            policy: ScrollBar.AsNeeded
            active: hovered || pressed || preferencesFlickable.moving
            background: Rectangle {
                color: "transparent"
            }
            contentItem: Rectangle {
                implicitWidth: 4
                radius: 2
                color: preferencesScrollBar.hovered || preferencesScrollBar.pressed ? Theme.scrollbarThumbHover : Theme.scrollbarThumb
            }
        }

        // Settings categories column
        Column {
            id: content
            width: preferencesFlickable.contentWidth
            spacing: 18

            GroupCard {
                SwitchRow {
                    title: qsTr("Open New Window for New Files")
                    checked: SettingsManager.openNewWindows
                    onToggled: value => SettingsManager.openNewWindows = value
                }
                Sep {}
                SwitchRow {
                    title: qsTr("Progress Bar Thumbnail")
                    checked: SettingsManager.thumbnailPreview
                    onToggled: value => SettingsManager.thumbnailPreview = value
                }
                Sep {}
                SwitchRow {
                    title: qsTr("Normalize Volume")
                    checked: SettingsManager.normalizeVolume
                    onToggled: value => SettingsManager.normalizeVolume = value
                }
                Sep {}
                SwitchRow {
                    title: qsTr("Hardware Acceleration")
                    subtitle: qsTr("Use GPU for video decoding when available")
                    checked: SettingsManager.hwdec !== "no"
                    // Toggle between auto-safe and disabled
                    onToggled: value => SettingsManager.hwdec = value ? "auto-safe" : "no"
                }
                Sep {}
                SwitchRow {
                    title: qsTr("Restore Saved Session")
                    subtitle: qsTr("Pick up where you left off after saving from the menu")
                    checked: SettingsManager.saveSession
                    onToggled: value => SettingsManager.saveSession = value
                }
                Sep {}
                SwitchRow {
                    title: qsTr("Save Video Position on Close")
                    subtitle: qsTr("Also save options like brightness, subtitle delay, etc.")
                    checked: SettingsManager.saveVideoPosition
                    onToggled: value => SettingsManager.saveVideoPosition = value
                }
            }

            Text {
                text: qsTr("Media Library Metadata")
                color: Theme.text
                font.pixelSize: 13
                font.bold: true
                leftPadding: 12
            }

            GroupCard {
                SwitchRow {
                    title: qsTr("TMDB Metadata")
                    subtitle: qsTr("Match local movies, TV shows, and anime with artwork and details")
                    checked: root.metadataService ? root.metadataService.enabled : false
                    enabled: !!root.metadataService
                    onToggled: function (value) {
                        root.metadataService.enabled = value;
                        if (value)
                            root.metadataService.refresh();
                    }
                }
                Sep {}
                ActionRow {
                    title: qsTr("TMDB API Token or Key")
                    subtitle: qsTr("Stored locally and sent only to api.themoviedb.org")
                    TextField {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 168
                        enabled: !!root.metadataService
                        echoMode: TextInput.Password
                        placeholderText: qsTr("Required")
                        text: root.metadataService ? root.metadataService.apiToken : ""
                        color: Theme.text
                        onEditingFinished: if (root.metadataService)
                            root.metadataService.apiToken = text
                        background: Rectangle {
                            radius: Theme.radius
                            color: Theme.cardStrong
                        }
                    }
                }
                Sep {}
                ActionRow {
                    title: qsTr("Library Matches")
                    subtitle: root.metadataService && root.metadataService.statusMessage.length > 0
                        ? root.metadataService.statusMessage
                        : qsTr("Refresh cached metadata for indexed media")
                    CineButton {
                        anchors.verticalCenter: parent.verticalCenter
                        styleVariant: "text"
                        btnText: root.metadataService && root.metadataService.busy
                            ? qsTr("Cancel") : qsTr("Refresh")
                        enabled: !!root.metadataService
                            && (root.metadataService.busy
                                || (root.metadataService.enabled
                                    && root.metadataService.apiToken.length > 0))
                        onClicked: {
                            if (root.metadataService.busy)
                                root.metadataService.cancel();
                            else
                                root.metadataService.refresh(true);
                        }
                    }
                }
                Sep {}
                Item {
                    width: parent.width
                    height: attributionText.implicitHeight + 24
                    Text {
                        id: attributionText
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("This product uses the TMDB API but is not endorsed or certified by TMDB.")
                        color: Theme.mutedText
                        font.pixelSize: Theme.fontSizeCaption
                        wrapMode: Text.WordWrap
                    }
                }
            }

            GroupCard {
                id: subtitleCard

                // --- Subtitle Color ---
                Item {
                    width: parent.width
                    height: 58

                    TitleSub {
                        anchors.left: parent.left
                        anchors.leftMargin: 16
                        anchors.right: colorRow.left
                        anchors.rightMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        title: qsTr("Subtitle Color")
                    }

                    // Color chip row
                    Row {
                        id: colorRow
                        anchors.right: parent.right
                        anchors.rightMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 8

                        // Color swatch displaying current subtitle color
                        ColorChip {
                            anchors.verticalCenter: parent.verticalCenter
                            value: SettingsManager.subtitleColor
                            // Open color picker on click
                            onPicked: colorDialog.openFor(SettingsManager.subtitleColor)
                        }

                        // Reset subtitle color to default
                        UndoBtn {
                            anchors.verticalCenter: parent.verticalCenter
                            callback: function () {
                                SettingsManager.subtitleColor = "#ebebeb";
                            }
                        }
                    }
                }

                Sep {}

                // --- Subtitle Font (activatable row) ---
                Item {
                    width: parent.width
                    height: 58

                    TitleSub {
                        anchors.left: parent.left
                        anchors.leftMargin: 16
                        anchors.right: fontRow.left
                        anchors.rightMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        title: qsTr("Subtitle Font")
                    }

                    Row {
                        id: fontRow
                        anchors.right: parent.right
                        anchors.rightMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 6

                        // Info button: "Some fonts may not work" (matches GTK)
                        CineButton {
                            id: fontInfoBtn
                            anchors.verticalCenter: parent.verticalCenter
                            styleVariant: "icon"
                            iconName: "cine-warning-symbolic"
                            buttonSize: 28
                            iconSize: 15
                            btnTooltip: qsTr("Font info")
                            // Show font info popup
                            onClicked: fontInfoPopup.open()
                            Popup {
                                id: fontInfoPopup
                                y: parent.height + 6
                                x: -width + parent.width
                                padding: 10
                                background: Rectangle {
                                    radius: 10
                                    color: Theme.popover
                                    opacity: 0.97
                                    border.color: Theme.popoverBorder
                                    border.width: 1
                                }
                                contentItem: Text {
                                    text: qsTr("Some fonts may not work")
                                    color: "#f5c211"
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                            }
                        }

                        // Current font name display
                        Text {
                            id: fontLabel
                            anchors.verticalCenter: parent.verticalCenter
                            text: SettingsManager.subtitleFont
                            color: Theme.text
                            font: Qt.font({
                                family: SettingsManager.subtitleFont,
                                pixelSize: 13,
                                bold: true
                            })
                            elide: Text.ElideRight
                            width: Math.min(implicitWidth, root.compactLayout ? 72 : 130)
                            horizontalAlignment: Text.AlignRight

                            // Click to open font picker
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: fontDialog.openFor(SettingsManager.subtitleFont)
                            }
                        }

                        // Reset subtitle font to default
                        UndoBtn {
                            anchors.verticalCenter: parent.verticalCenter
                            callback: function () {
                                SettingsManager.subtitleFont = "Adwaita Sans SemiBold";
                            }
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        color: fontRowMouse.containsMouse ? Theme.glassHover : "transparent"
                        Behavior on color {
                            ColorAnimation {
                                duration: Theme.motionFast
                                easing.type: Easing.OutCubic
                            }
                        }
                        z: -1
                    }

                    // Clickable overlay for the entire font row
                    MouseArea {
                        id: fontRowMouse
                        anchors.fill: parent
                        anchors.rightMargin: fontRow.width + 16
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: fontDialog.openFor(SettingsManager.subtitleFont)
                    }
                }

                Sep {}

                // --- Subtitle Scale ---
                Item {
                    width: parent.width
                    height: 58

                    TitleSub {
                        anchors.left: parent.left
                        anchors.leftMargin: 16
                        anchors.right: scaleRow.left
                        anchors.rightMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        title: qsTr("Subtitle Scale")
                    }

                    Row {
                        id: scaleRow
                        anchors.right: parent.right
                        anchors.rightMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 6

                        // Decrease subtitle scale
                        MiniBtn {
                            anchors.verticalCenter: parent.verticalCenter
                            iconName: "cine-list-remove-symbolic"
                            onClicked: SettingsManager.subtitleScale = Math.max(0.1, SettingsManager.subtitleScale - 0.05)
                        }
                        // Current scale value display
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 48
                            horizontalAlignment: Text.AlignHCenter
                            text: SettingsManager.subtitleScale.toFixed(2)
                            color: Theme.text
                            font.pixelSize: 14
                            font.bold: true
                        }
                        // Increase subtitle scale
                        MiniBtn {
                            anchors.verticalCenter: parent.verticalCenter
                            iconName: "cine-list-add-symbolic"
                            onClicked: SettingsManager.subtitleScale = Math.min(3.0, SettingsManager.subtitleScale + 0.05)
                        }
                    }
                }

                Sep {}

                // --- Subtitle Background (Switch) ---
                SwitchRow {
                    title: qsTr("Subtitle Background")
                    checked: SettingsManager.subtitleBackground
                    // Toggle subtitle background
                    onToggled: value => SettingsManager.subtitleBackground = value
                }

                // Conditional separator when background is enabled
                Sep {
                    visible: SettingsManager.subtitleBackground
                }

                // --- Subtitle Background Color ---
                Item {
                    width: parent.width
                    height: 58
                    visible: SettingsManager.subtitleBackground

                    TitleSub {
                        anchors.left: parent.left
                        anchors.leftMargin: 16
                        anchors.right: bgColorRow.left
                        anchors.rightMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        title: qsTr("Subtitle Background Color")
                    }

                    Row {
                        id: bgColorRow
                        anchors.right: parent.right
                        anchors.rightMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 8

                        // Background color swatch
                        ColorChip {
                            anchors.verticalCenter: parent.verticalCenter
                            value: SettingsManager.subtitleBackgroundColor
                            // Open background color picker
                            onPicked: bgColorDialog.openFor(SettingsManager.subtitleBackgroundColor)
                        }

                        // Reset background color to default
                        UndoBtn {
                            anchors.verticalCenter: parent.verticalCenter
                            callback: function () {
                                SettingsManager.subtitleBackgroundColor = "#97000000";
                            }
                        }
                    }
                }
            }

            GroupCard {
                // Preferred subtitle languages input
                ActionRow {
                    objectName: "subtitleLanguagesRow"
                    title: qsTr("Preferred Subtitle Languages")
                    TextField {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 150
                        placeholderText: qsTr("en,es,pt")
                        text: SettingsManager.subtitleLanguages
                        color: Theme.text
                        onEditingFinished: SettingsManager.subtitleLanguages = text
                        background: Rectangle {
                            radius: Theme.radius
                            color: Theme.cardStrong
                        }
                    }
                }
                Sep {}
                // Preferred audio languages input
                ActionRow {
                    objectName: "audioLanguagesRow"
                    title: qsTr("Preferred Audio Languages")
                    TextField {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 150
                        placeholderText: qsTr("en,ja,hi")
                        text: SettingsManager.audioLanguages
                        color: Theme.text
                        onEditingFinished: SettingsManager.audioLanguages = text
                        background: Rectangle {
                            radius: Theme.radius
                            color: Theme.cardStrong
                        }
                    }
                }
            }

            Text {
                text: qsTr("Mouse Actions")
                color: Theme.text
                font.pixelSize: 13
                font.bold: true
                leftPadding: 12
            }

            GroupCard {
                ComboRow {
                    objectName: "leftClickRow"
                    title: qsTr("Left Click")
                    model: [qsTr("Play/Pause"), qsTr("Focus/Play/Pause"), qsTr("Disabled")]
                    currentIndex: SettingsManager.leftClick
                    onActivated: index => SettingsManager.leftClick = index
                }
                Sep {}
                ComboRow {
                    objectName: "rightClickRow"
                    title: qsTr("Right Click")
                    model: [qsTr("Play/Pause"), qsTr("Window Menu"), qsTr("Disabled")]
                    currentIndex: SettingsManager.rightClick
                    onActivated: index => SettingsManager.rightClick = index
                }
            }

            Text {
                text: qsTr("Language")
                color: Theme.text
                font.pixelSize: 13
                font.bold: true
                leftPadding: 12
            }

            GroupCard {
                ActionRow {
                    title: qsTr("UI Language")
                    subtitle: qsTr("Requires restart to take effect")
                    ComboBox {
                        id: langCombo
                        anchors.verticalCenter: parent.verticalCenter
                        width: 168
                        height: 36
                        focusPolicy: Qt.NoFocus
                        textRole: "label"
                        model: [
                            {
                                label: "System Default",
                                code: ""
                            },
                            {
                                label: "العربية",
                                code: "ar"
                            },
                            {
                                label: "Azərbaycan",
                                code: "az"
                            },
                            {
                                label: "Беларуская",
                                code: "be"
                            },
                            {
                                label: "Български",
                                code: "bg"
                            },
                            {
                                label: "বাংলা",
                                code: "bn"
                            },
                            {
                                label: "Català",
                                code: "ca"
                            },
                            {
                                label: "Čeština",
                                code: "cs"
                            },
                            {
                                label: "Dansk",
                                code: "da"
                            },
                            {
                                label: "Deutsch",
                                code: "de"
                            },
                            {
                                label: "Ελληνικά",
                                code: "el"
                            },
                            {
                                label: "English",
                                code: "en"
                            },
                            {
                                label: "Español",
                                code: "es"
                            },
                            {
                                label: "Eesti",
                                code: "et"
                            },
                            {
                                label: "Euskara",
                                code: "eu"
                            },
                            {
                                label: "فارسی",
                                code: "fa"
                            },
                            {
                                label: "Suomi",
                                code: "fi"
                            },
                            {
                                label: "Filipino",
                                code: "fil"
                            },
                            {
                                label: "Français",
                                code: "fr"
                            },
                            {
                                label: "Gaeilge",
                                code: "ga"
                            },
                            {
                                label: "Galego",
                                code: "gl"
                            },
                            {
                                label: "ગુજરાતી",
                                code: "gu"
                            },
                            {
                                label: "עברית",
                                code: "he"
                            },
                            {
                                label: "हिन्दी",
                                code: "hi"
                            },
                            {
                                label: "Hrvatski",
                                code: "hr"
                            },
                            {
                                label: "Magyar",
                                code: "hu"
                            },
                            {
                                label: "Հայերեն",
                                code: "hy"
                            },
                            {
                                label: "Bahasa Indonesia",
                                code: "id"
                            },
                            {
                                label: "Íslenska",
                                code: "is"
                            },
                            {
                                label: "Italiano",
                                code: "it"
                            },
                            {
                                label: "日本語",
                                code: "ja"
                            },
                            {
                                label: "ქართული",
                                code: "ka"
                            },
                            {
                                label: "Қазақ",
                                code: "kk"
                            },
                            {
                                label: "ខ្មែរ",
                                code: "km"
                            },
                            {
                                label: "ಕನ್ನಡ",
                                code: "kn"
                            },
                            {
                                label: "한국어",
                                code: "ko"
                            },
                            {
                                label: "Lietuvių",
                                code: "lt"
                            },
                            {
                                label: "Latviešu",
                                code: "lv"
                            },
                            {
                                label: "Македонски",
                                code: "mk"
                            },
                            {
                                label: "മലയാളം",
                                code: "ml"
                            },
                            {
                                label: "Монгол",
                                code: "mn"
                            },
                            {
                                label: "मराठी",
                                code: "mr"
                            },
                            {
                                label: "Bahasa Melayu",
                                code: "ms"
                            },
                            {
                                label: "မြန်မာ",
                                code: "my"
                            },
                            {
                                label: "Norsk Bokmål",
                                code: "nb"
                            },
                            {
                                label: "नेपाली",
                                code: "ne"
                            },
                            {
                                label: "Nederlands",
                                code: "nl"
                            },
                            {
                                label: "ਪੰਜਾਬੀ",
                                code: "pa"
                            },
                            {
                                label: "Polski",
                                code: "pl"
                            },
                            {
                                label: "Português",
                                code: "pt"
                            },
                            {
                                label: "Română",
                                code: "ro"
                            },
                            {
                                label: "Русский",
                                code: "ru"
                            },
                            {
                                label: "සිංහල",
                                code: "si"
                            },
                            {
                                label: "Slovenčina",
                                code: "sk"
                            },
                            {
                                label: "Slovenščina",
                                code: "sl"
                            },
                            {
                                label: "Shqip",
                                code: "sq"
                            },
                            {
                                label: "Српски",
                                code: "sr"
                            },
                            {
                                label: "Svenska",
                                code: "sv"
                            },
                            {
                                label: "Kiswahili",
                                code: "sw"
                            },
                            {
                                label: "தமிழ்",
                                code: "ta"
                            },
                            {
                                label: "తెలుగు",
                                code: "te"
                            },
                            {
                                label: "ไทย",
                                code: "th"
                            },
                            {
                                label: "Türkçe",
                                code: "tr"
                            },
                            {
                                label: "Українська",
                                code: "uk"
                            },
                            {
                                label: "اردو",
                                code: "ur"
                            },
                            {
                                label: "Tiếng Việt",
                                code: "vi"
                            },
                            {
                                label: "简体中文",
                                code: "zh_CN"
                            },
                            {
                                label: "繁體中文",
                                code: "zh_TW"
                            }
                        ]
                    // Apply selected locale
                    onActivated: function (index) {
                        SettingsManager.locale = model[index].code;
                    }
                        // Restore saved locale selection on startup
                        Component.onCompleted: {
                            // Saved locale code
                            var saved = SettingsManager.locale;
                            // Loop over model to match saved locale
                            for (var i = 0; i < model.length; ++i) {
                                if (model[i].code === saved) {
                                    currentIndex = i;
                                    break;
                                }
                            }
                        }
                        background: Rectangle {
                            radius: Theme.radius
                            color: langCombo.down ? Theme.glassActive : (langCombo.hovered ? Theme.cardHover : Theme.cardStrong)
                            border.color: langCombo.popup.visible ? Theme.accent : "transparent"
                            border.width: 1
                        }
                        contentItem: Text {
                            leftPadding: 12
                            rightPadding: 28
                            text: langCombo.displayText
                            color: Theme.text
                            font.pixelSize: 13
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                        indicator: CineIcon {
                            x: langCombo.width - width - 10
                            y: (langCombo.height - height) / 2
                            name: "cine-dropdown-symbolic"
                            tint: Theme.mutedText
                            width: 11
                            height: 11
                        }
                        delegate: ItemDelegate {
                            id: langItem
                        // Current language item
                        required property var modelData
                        // Current language index
                        required property int index
                        width: ListView.view ? ListView.view.width : langCombo.width
                            height: 36
                            highlighted: langCombo.highlightedIndex === index
                            background: Rectangle {
                                anchors.fill: parent
                                anchors.margins: 4
                                radius: 7
                                color: langItem.highlighted ? Theme.cardHover : "transparent"
                            }
                            contentItem: Text {
                                leftPadding: 8
                                text: langItem.modelData.label
                                color: Theme.text
                                font.pixelSize: 13
                                font.bold: langCombo.currentIndex === langItem.index
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        popup: Popup {
                            y: langCombo.height + 6
                            width: Math.max(langCombo.width, 150)
                            x: langCombo.width - width
                            implicitHeight: Math.min(langList.contentHeight + 8, 260)
                            padding: 4
                            background: Item {
                                Rectangle {
                                    anchors.fill: parent
                                    anchors.margins: 4
                                    radius: 12
                                    color: Theme.popoverShadow
                                }
                                Rectangle {
                                    anchors.fill: parent
                                    anchors.margins: 4
                                    radius: 12
                                    color: Theme.popover
                                    opacity: 0.95
                                    border.color: Theme.popoverBorder
                                    border.width: 1
                                }
                            }
                            contentItem: ListView {
                                id: langList
                                clip: true
                                implicitHeight: contentHeight
                                model: langCombo.delegateModel
                                currentIndex: langCombo.highlightedIndex
                                boundsBehavior: Flickable.StopAtBounds
                                ScrollBar.vertical: ScrollBar {
                                    width: 6
                                }
                            }
                        }
                    }
                }
            }

            Text {
                text: qsTr("Appearance")
                color: Theme.text
                font.pixelSize: 13
                font.bold: true
                leftPadding: 12
            }

            GroupCard {
                ComboRow {
                    title: qsTr("Theme")
                    subtitle: qsTr("Use a consistent dark or light palette across every screen")
                    model: [qsTr("Dark"), qsTr("Light")]
                    currentIndex: SettingsManager.themeMode === "light" ? 1 : 0
                    // Apply selected theme mode
                    onActivated: function (index) {
                        SettingsManager.themeMode = index === 1 ? "light" : "dark";
                    }
                }
                Sep {}
                ActionRow {
                    title: qsTr("Accent Color")
                    subtitle: SettingsManager.accentColor
                    // Accent color swatch button
                    Button {
                        id: accentButton
                        width: 64
                        height: 34
                        Accessible.name: qsTr("Choose accent color")
                        // Open accent color picker
                        onClicked: accentColorDialog.openFor(SettingsManager.accentColor)
                        // Accent color preview
                        background: Rectangle {
                            radius: Theme.radius
                            color: Theme.accent
                            border.color: Theme.separator
                        }
                    }
                }
                Sep {}
                SwitchRow {
                    title: qsTr("Reduce Motion")
                    subtitle: qsTr("Disable non-essential transitions and movement")
                    checked: SettingsManager.reduceMotion
                    onToggled: value => SettingsManager.reduceMotion = value
                }
            }

            Text {
                text: qsTr("Updates & Config")
                color: Theme.text
                font.pixelSize: 13
                font.bold: true
                leftPadding: 12
            }

            GroupCard {
                SwitchRow {
                    title: qsTr("Check for Updates Automatically")
                    subtitle: qsTr("Check for new CineWindows versions on startup")
                    checked: SettingsManager.autoUpdate
                    onToggled: value => SettingsManager.autoUpdate = value
                }
                Sep {}
                ActionRow {
                    title: Qt.application.displayName
                    subtitle: qsTr("Version %1").arg(Qt.application.version)

                    CineButton {
                        id: updateBtn
                        anchors.verticalCenter: parent.verticalCenter
                        styleVariant: "text"
                        btnText: !root.updateService ? qsTr("Check for Updates")
                               : root.updateService.state === UpdateService.Checking ? qsTr("Checking...")
                               : root.updateService.state === UpdateService.Available ? qsTr("Download Update")
                               : root.updateService.state === UpdateService.Downloading ? qsTr("Cancel Download")
                               : root.updateService.state === UpdateService.ReadyToInstall ? qsTr("Install and Restart")
                               : root.updateService.state === UpdateService.Error ? qsTr("Retry")
                               : qsTr("Check for Updates")
                        enabled: !!root.updateService
                        // Handle update button based on current update state
                        onClicked: {
                            if (root.updateService.state === UpdateService.Available)
                                root.updateService.downloadUpdate();
                            else if (root.updateService.state === UpdateService.Downloading)
                                root.updateService.cancelDownload();
                            else if (root.updateService.state === UpdateService.ReadyToInstall)
                                root.updateService.installUpdate();
                            else
                                root.updateService.checkForUpdates(Qt.application.version);
                        }
                    }
                }
                Item {
                    id: updateStatusRow
                    width: parent.width
                    height: visible ? 40 : 0
                    visible: !!root.updateService && root.updateService.statusMessage.length > 0

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.updateService ? root.updateService.statusMessage : ""
                        color: root.updateService && root.updateService.updateAvailable ? Theme.accent : Theme.mutedText
                        font.pixelSize: 13
                    }
                }
                ProgressBar {
                    id: updateProgress
                    width: parent.width - 32
                    anchors.horizontalCenter: parent.horizontalCenter
                    height: visible ? 12 : 0
                    visible: !!root.updateService && root.updateService.state === UpdateService.Downloading
                    from: 0
                    to: 1
                    value: root.updateService ? root.updateService.downloadProgress : 0
                    background: Rectangle { radius: 3; color: Theme.cardStrong }
                    contentItem: Rectangle {
                        implicitHeight: 6
                        radius: 3
                        color: Theme.accent
                        width: updateProgress.visualPosition * updateProgress.width
                    }
                }
                Sep {}
                ActionRow {
                    title: qsTr("Open Configuration Folder")
                    subtitle: qsTr("Access player logs and config files")
                    CineButton {
                        id: openCfgBtn
                        anchors.verticalCenter: parent.verticalCenter
                        styleVariant: "text"
                        btnText: qsTr("Open")
                        // Open config directory in file manager
                        onClicked: SettingsManager.openConfigDirectory()
                    }
                }
            }
        }
    }
}
