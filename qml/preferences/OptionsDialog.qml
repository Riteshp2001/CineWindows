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
    /// The options object providing mpv property read/write access
    property var options // Options provider for mpv property read/write
    /// Minimum margin from the screen edge when positioning
    property int edgeMargin: 16 // Minimum margin from screen edge
    /// Gap between the popup and its anchor item
    property int popupGap: 10 // Gap between popup and anchor item
    /// The item the popup is anchored to
    property var anchorItem: null // The item the popup is anchored to
    /// Calculated X position of the pointer/triangle indicator
    property real pointerX: anchorItem && parent ? Math.max(24, Math.min(width - 24, anchorCenterX() - x)) : width - 38 // Calculated X position of the pointer indicator

    // Toggle guard: clicking the Options button while open dismisses then re-fires
    // onClicked; refuse to reopen within a short window so a second click closes it.
    property double closedAt: 0 // Timestamp when dialog was last closed (toggle guard)
    onClosed: closedAt = Date.now()
    /// Returns true if the dialog was closed less than 250ms ago (toggle guard)
    function justClosed() { // True if dialog was closed less than 250ms ago (toggle guard)
        return (Date.now() - closedAt) < 250;
    }

    /// Returns the X position of the anchor item's center in parent coordinates
    function anchorCenterX() { // Returns X position of anchor item's center in parent coords
        if (!anchorItem || !parent)
            return width / 2;
        return anchorItem.mapToItem(parent, anchorItem.width / 2, anchorItem.height / 2).x;
    }

    /// Returns the Y position of the top of the anchor item in parent coordinates
    function anchorTopY() { // Returns Y position of anchor item's top in parent coords
        if (!anchorItem || !parent)
            return 0;
        return anchorItem.mapToItem(parent, anchorItem.width / 2, 0).y;
    }

    /// Opens the popup above the given anchor item, within the given bounds item
    function openAbove(anchor, boundsItem) { // Opens popup above given anchor, within bounds
        if (justClosed())
            return;
        parent = boundsItem ? boundsItem : Overlay.overlay;
        anchorItem = anchor;
        open();
    }

    parent: Overlay.overlay
    width: 312
    height: Math.min(380, Math.max(250, (parent ? parent.height : 600) - 220))
    modal: false
    dim: false
    leftPadding: 10

    Binding {
        target: root
        property: "x"
        value: root.parent ? Math.round(Math.max(root.edgeMargin, Math.min(root.parent.width - root.width - root.edgeMargin, root.anchorItem ? root.anchorCenterX() - root.width / 2 : root.parent.width - root.width - root.edgeMargin))) : 0
    }

    Binding {
        target: root
        property: "y"
        value: root.parent ? Math.round(Math.max(56, Math.min(root.parent.height - root.height - root.edgeMargin, root.anchorItem ? root.anchorTopY() - root.height - root.popupGap : root.parent.height - root.height - 128))) : 64
    }
    rightPadding: 10
    topPadding: 10
    bottomPadding: 18

    background: Item {
        Rectangle {
            anchors.fill: parent
            anchors.bottomMargin: 8
            radius: 14
            color: Theme.popover
            opacity: 0.95
            border.color: Theme.popoverBorder
            border.width: 1
        }

        Rectangle {
            width: 12
            height: 12
            x: Math.max(18, Math.min(parent.width - 30, root.pointerX - width / 2))
            y: parent.height - 14
            z: -1
            rotation: 45
            radius: 1
            color: Theme.popover
            opacity: 0.95
        }
    }

    // --- Reusable row pieces ---

    component UndoButton: CineButton { // Reusable undo/reset button
        // qmllint disable use-proper-function
        property var callback: function () {} // Callback invoked when undo is clicked
        styleVariant: "icon"
        iconName: "edit-undo-symbolic"
        buttonSize: 26
        iconSize: 12
        btnTooltip: qsTr("Reset")
            onClicked: callback()
    }

    component MiniButton: AbstractButton { // Small icon-only button for stepper/row actions
        id: miniBtn
        property string actionIcon: "" // Icon name for the button
        implicitWidth: 38
        implicitHeight: 30
        hoverEnabled: true
        Accessible.name: miniBtn.actionIcon.length > 0 ? miniBtn.actionIcon : ""
        background: Rectangle {
            radius: Theme.radius
            color: miniBtn.down ? Theme.panelStrong : (miniBtn.hovered ? Theme.cardHover : Theme.cardStrong)
        }
        contentItem: Item {
            anchors.fill: parent
            CineIcon {
                visible: miniBtn.actionIcon.length > 0
                anchors.centerIn: parent
                width: 16
                height: 16
                name: miniBtn.actionIcon
                filled: miniBtn.hovered || miniBtn.down || miniBtn.activeFocus
            }
        }
    }

    component RowLabel: Text { // Bold row label text
        color: Theme.text
        font.pixelSize: 14
        font.bold: true
        verticalAlignment: Text.AlignVCenter
    }

    component StepperRow: Rectangle { // Row with undo button, label, and +/- stepper
        id: stepper
        property string label: "" // Display label
        property real value: 0 // Current stepper value
        property real from: -100 // Minimum range
        property real to: 100 // Maximum range
        property real step: 1 // Step increment
        property real reset: 0 // Reset value
        property int decimals: 0 // Decimal places to display
        // qmllint disable use-proper-function
        property var apply: function (v) {} // Callback when value changes

        width: parent ? parent.width : 0
        height: visible ? 46 : 0
        radius: Theme.radius
        color: Theme.card

        function setValue(v) { // Clamp and apply a new value
            var newValue = Math.max(from, Math.min(to, v));
            value = newValue;
            apply(newValue);
        }

        UndoButton {
            id: undo
            anchors.left: parent.left
            anchors.leftMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            callback: function () {
                stepper.setValue(stepper.reset);
            }
        }
        RowLabel {
            anchors.left: undo.right
            anchors.leftMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            text: stepper.label
        }
        Row {
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6
            // Layout: − value +  (value centred between the stepper buttons).
            MiniButton {
                actionIcon: "cine-list-remove-symbolic"
                onClicked: stepper.setValue(stepper.value - stepper.step)
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: 48
                horizontalAlignment: Text.AlignHCenter
                text: stepper.value.toFixed(stepper.decimals)
                color: Theme.text
                font.pixelSize: 14
                font.bold: true
            }
            MiniButton {
                actionIcon: "cine-list-add-symbolic"
                onClicked: stepper.setValue(stepper.value + stepper.step)
            }
        }
    }

    component ComboRow: Rectangle { // Row with undo button, label, and combo box
        id: comboRow
        property string label: "" // Display label
        property var model: [] // Combo box model
        property var apply: function (idx, text) {} // Callback on selection
        // qmllint disable use-proper-function
        property var resetAct: function () {} // Callback on reset
        property alias currentIndex: combo.currentIndex // Currently selected index
        width: parent ? parent.width : 0
        height: visible ? 46 : 0
        radius: Theme.radius
        color: Theme.card

        UndoButton {
            id: cundo
            anchors.left: parent.left
            anchors.leftMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            callback: function () {
                combo.currentIndex = 0;
                comboRow.resetAct();
            }
        }
        RowLabel {
            anchors.left: cundo.right
            anchors.leftMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            text: comboRow.label
        }
        ComboBox {
            id: combo
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: 124
            height: 34
            focusPolicy: Qt.NoFocus
            model: parent.model
            onActivated: comboRow.apply(currentIndex, currentText)
            background: Rectangle {
                radius: Theme.radius
                color: combo.down ? Theme.glassActive : (combo.hovered ? Theme.cardHover : Theme.cardStrong)
                border.color: combo.popup.visible ? Theme.accent : "transparent"
                border.width: 1
            }
            contentItem: Text {
                leftPadding: 10
                rightPadding: 26
                text: combo.displayText
                color: Theme.text
                font.pixelSize: 13
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
            indicator: CineIcon {
                x: combo.width - width - 9
                y: (combo.height - height) / 2
                name: "cine-dropdown-symbolic"
                tint: Theme.mutedText
                width: 11
                height: 11
            }
            delegate: ItemDelegate {
                id: comboItem
                required property var modelData
                required property int index
                width: ListView.view ? ListView.view.width : combo.width
                height: 34
                highlighted: combo.highlightedIndex === index
                background: Rectangle {
                    anchors.fill: parent
                    anchors.margins: 4
                    radius: 7
                    color: comboItem.highlighted ? Theme.cardHover : "transparent"
                }
                contentItem: Text {
                    leftPadding: 8
                    text: comboItem.modelData
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
                background: Rectangle {
                    radius: 12
                    color: Theme.popover
                    border.color: Theme.popoverBorder
                    border.width: 1
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

    component ButtonsRow: Rectangle { // Row with undo button, label, and two action buttons
        id: btnRow
        property string label: "" // Display label
        property string leftIcon: "" // Left button icon
        property string rightIcon: "" // Right button icon
        // qmllint disable use-proper-function
        property var leftAct: function () {} // Left button callback
        property var rightAct: function () {} // Right button callback
        property var resetAct: function () {} // Reset callback
        width: parent ? parent.width : 0
        height: visible ? 46 : 0
        radius: Theme.radius
        color: Theme.card

        UndoButton {
            id: bundo
            anchors.left: parent.left
            anchors.leftMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            callback: btnRow.resetAct
        }
        RowLabel {
            anchors.left: bundo.right
            anchors.leftMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            text: btnRow.label
        }
        Row {
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6
            MiniButton {
                actionIcon: btnRow.leftIcon
                onClicked: btnRow.leftAct()
            }
            MiniButton {
                actionIcon: btnRow.rightIcon
                onClicked: btnRow.rightAct()
            }
        }
    }

    readonly property var ratios: [qsTr("Original"), "16:9", "4:3", "1:1", "16:10", "2.00:1", "2.21:1", "2.35:1", "2.39:1", "5:4"] // Predefined aspect ratio and crop options

    onOpened: {
        if (!options || !options.player)
            return;
        var player = options.player; // Player interface from options

        // 1. Flip visibility under native hwdec
        var hwdec = String(player.mpvOption("hwdec-current") || ""); // Current hwdec string
        var usingNativeHwdec = hwdec !== "" && hwdec !== "None" && hwdec !== "no" && hwdec.indexOf("-copy") === -1; // True if native (non-copy) hwdec active
        flipRow.visible = !usingNativeHwdec;

        // 2. Aspect Ratio override check
        var aspect = player.mpvOption("video-aspect-override"); // Current aspect ratio override
        var aspectVal = -1.0; // Parsed aspect float
        if (aspect && aspect !== "no" && aspect !== -1) {
            aspectVal = parseFloat(aspect);
        }
        var aspectIndex = 0; // Matched index in ratios
        var ratiosFloats = [-1.0, 16 / 9, 4 / 3, 1 / 1, 16 / 10, 2.00, 2.21, 2.35, 2.39, 5 / 4]; // Float values matching ratio strings
        for (var i = 1; i < ratiosFloats.length; ++i) {
            if (Math.abs(ratiosFloats[i] - aspectVal) < 0.001) {
                aspectIndex = i;
                break;
            }
        }
        aspectRow.currentIndex = aspectIndex;

        // 3. Crop ratio check
        var cropStr = String(player.mpvOption("video-crop") || ""); // Current crop string from mpv
        var cropIndex = 0; // Matched index in ratios
        if (cropStr && cropStr.length > 0) {
            try {
                var parts = cropStr.split("x"); // Split crop string parts
                if (parts.length >= 2) {
                    var w = parseInt(parts[0]); // Crop width
                    var h = parseInt(parts[1].split("+")[0]); // Crop height
                    if (w > 0 && h > 0) {
                        var currentRatio = w / h; // Calculated crop ratio
                        for (var j = 1; j < ratiosFloats.length; ++j) {
                            if (Math.abs(ratiosFloats[j] - currentRatio) < 0.01) {
                                cropIndex = j;
                                break;
                            }
                        }
                    }
                }
            } catch (e) {
                // ignore
            }
        }
        cropRow.currentIndex = cropIndex;

        // 4. Set spin/slider values
        function getOpt(name, def) { // Read mpv option as number with fallback default
            var val = player.mpvOption(name); // Raw mpv option value
            return (val !== undefined && val !== null) ? Number(val) : def;
        }

        zoomRow.value = getOpt("video-zoom", 0);
        contrastRow.value = getOpt("contrast", 0);
        brightnessRow.value = getOpt("brightness", 0);
        gammaRow.value = getOpt("gamma", 0);
        saturationRow.value = getOpt("saturation", 0);
        hueRow.value = getOpt("hue", 0);
        subDelayRow.value = getOpt("sub-delay", 0);
        audioDelayRow.value = getOpt("audio-delay", 0);
        speedRow.value = player.speed;
    }

    contentItem: Flickable {
        id: optionsFlickable
        readonly property int scrollbarGutter: 10

        contentWidth: width - scrollbarGutter
        contentHeight: content.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {
            id: optionsScrollBar
            parent: optionsFlickable
            anchors.top: optionsFlickable.top
            anchors.right: optionsFlickable.right
            anchors.bottom: optionsFlickable.bottom
            width: 6
            policy: ScrollBar.AsNeeded
            background: Item {}
            contentItem: Rectangle {
                implicitWidth: 6
                radius: 3
                color: optionsScrollBar.hovered || optionsScrollBar.pressed ? Theme.scrollbarThumbHover : Theme.scrollbarThumb
            }
        }

        Column {
            id: content
            width: optionsFlickable.contentWidth
            spacing: 8

            CineButton {
                id: resetAllBtn
                styleVariant: "text"
                btnText: qsTr("Reset All")
                width: parent.width
                height: 40
                onClicked: {
                    root.options.resetAll();
                    // trigger refresh of settings locally
                    zoomRow.value = 0;
                    contrastRow.value = 0;
                    brightnessRow.value = 0;
                    gammaRow.value = 0;
                    saturationRow.value = 0;
                    hueRow.value = 0;
                    subDelayRow.value = 0;
                    audioDelayRow.value = 0;
                    speedRow.value = 1.0;
                    aspectRow.currentIndex = 0;
                    cropRow.currentIndex = 0;
                }
            }

            ComboRow {
                id: aspectRow
                label: qsTr("Aspect Ratio")
                model: root.ratios
                apply: function (idx, text) {
                    root.options.setAspectRatio(idx === 0 ? "no" : text);
                }
                resetAct: function () {
                    root.options.setAspectRatio("no");
                }
            }
            ComboRow {
                id: cropRow
                label: qsTr("Crop")
                model: root.ratios
                apply: function (idx, text) {
                    root.options.setCropRatio(idx === 0 ? "" : text);
                }
                resetAct: function () {
                    root.options.setCropRatio("");
                }
            }
            ButtonsRow {
                id: rotateRow
                label: qsTr("Rotate")
                leftIcon: "object-rotate-left-symbolic"
                rightIcon: "object-rotate-right-symbolic"
                leftAct: function () {
                    root.options.rotateLeft();
                }
                rightAct: function () {
                    root.options.rotateRight();
                }
                resetAct: function () {
                    root.options.resetRotation();
                }
            }
            ButtonsRow {
                id: flipRow
                label: qsTr("Flip")
                leftIcon: "object-flip-horizontal-symbolic"
                rightIcon: "object-flip-vertical-symbolic"
                leftAct: function () {
                    root.options.flipHorizontal();
                }
                rightAct: function () {
                    root.options.flipVertical();
                }
                resetAct: function () {
                    root.options.resetFlip();
                }
            }
            StepperRow {
                id: zoomRow
                label: qsTr("Zoom")
                from: -3
                to: 3
                step: 0.05
                reset: 0
                decimals: 2
                apply: function (v) {
                    root.options.setZoom(v);
                }
            }
            StepperRow {
                id: contrastRow
                label: qsTr("Contrast")
                from: -100
                to: 100
                step: 1
                reset: 0
                decimals: 0
                apply: function (v) {
                    root.options.setContrast(v);
                }
            }
            StepperRow {
                id: brightnessRow
                label: qsTr("Brightness")
                from: -100
                to: 100
                step: 1
                reset: 0
                decimals: 0
                apply: function (v) {
                    root.options.setBrightness(v);
                }
            }
            StepperRow {
                id: gammaRow
                label: qsTr("Gamma")
                from: -100
                to: 100
                step: 1
                reset: 0
                decimals: 0
                apply: function (v) {
                    root.options.setGamma(v);
                }
            }
            StepperRow {
                id: saturationRow
                label: qsTr("Saturation")
                from: -100
                to: 100
                step: 1
                reset: 0
                decimals: 0
                apply: function (v) {
                    root.options.setSaturation(v);
                }
            }
            StepperRow {
                id: hueRow
                label: qsTr("Hue")
                from: -100
                to: 100
                step: 1
                reset: 0
                decimals: 0
                apply: function (v) {
                    root.options.setHue(v);
                }
            }
            StepperRow {
                id: subDelayRow
                label: qsTr("Subtitle Delay")
                from: -999.9
                to: 999.9
                step: 0.1
                reset: 0
                decimals: 1
                apply: function (v) {
                    root.options.setSubtitleDelay(v);
                }
            }
            StepperRow {
                id: audioDelayRow
                label: qsTr("Audio Delay")
                from: -999.9
                to: 999.9
                step: 0.1
                reset: 0
                decimals: 1
                apply: function (v) {
                    root.options.setAudioDelay(v);
                }
            }
            StepperRow {
                id: speedRow
                label: qsTr("Playback Speed")
                from: 0.25
                to: 4
                step: 0.25
                reset: 1
                decimals: 2
                value: 1
                apply: function (v) {
                    root.options.setSpeed(v);
                }
            }
        }
    }
}
