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

// Keyboard shortcuts viewer + editor. The list is built entirely from the
// KeyBindings registry (so it can never drift from what the app actually does),
// and each row can be re-bound by capturing a key chord.
ResponsivePopup {
    id: root
    parent: Overlay.overlay
    metrics: ViewportMetrics {
        viewportWidth: root.parent ? root.parent.width : root.preferredWidth
        viewportHeight: root.parent ? root.parent.height : root.preferredHeight
    }
    modal: true
    dim: true
    preferredWidth: 680
    preferredHeight: 640
    padding: 0
    focus: true

    // The action id currently capturing a new chord ("" when idle)
    property string capturingId: ""

    // The MpvConfig instance, so rows can show when input.conf also binds a key
    property var mpvConfig: null

    onClosed: capturingId = ""
    onOpened: searchField.forceActiveFocus()

    background: Rectangle {
        radius: Theme.radius
        color: Theme.popover
        border.color: Theme.popoverBorder
        border.width: 1
    }

    // --- Ordered list of categories as they first appear in the registry. ---
    readonly property var categories: {
        var seen = ({}); // Track category names already encountered
        var out = []; // Accumulated unique category names
        var actions = KeyBindings.actions; // All action definitions from the registry
        for (var i = 0; i < actions.length; ++i) {
            if (!seen[actions[i].category]) {
                seen[actions[i].category] = true;
                out.push(actions[i].category);
            }
        }
        return out;
    }

    // Returns all action objects belonging to the given category
    function actionsIn(category) {
        var out = []; // Actions matching the requested category
        var actions = KeyBindings.actions; // All action definitions from the registry
        const query = searchField.text.trim().toLowerCase();
        KeyBindings.revision;
        for (var i = 0; i < actions.length; ++i)
            if (actions[i].category === category && (query.length === 0
                    || actions[i].title.toLowerCase().includes(query)
                    || category.toLowerCase().includes(query)
                    || KeyBindings.sequenceFor(actions[i].id).toLowerCase().includes(query)))
                out.push(actions[i]);
        return out;
    }

    // Translates a Keys.onPressed event into a Qt key-sequence string, or "" if not bindable
    function sequenceFromEvent(event) {
        return KeyBindings.sequenceFromEvent(event);
    }

    // Attempts to bind `id` to the key chord from `event`. Returns true on success.
    function applyCapture(id, event) {
        var seq = sequenceFromEvent(event); // Parsed key chord string
        if (seq.length === 0)
            return false;
        var clash = KeyBindings.conflict(seq, id); // Conflicting action ID, if any
        if (clash.length > 0) {
            conflictText.text = qsTr("\"%1\" is already used by \"%2\"").arg(seq).arg(KeyBindings.titleFor(clash));
            conflictText.visible = true;
            return false;
        }
        conflictText.visible = false;
        KeyBindings.setBinding(id, seq);
        return true;
    }

    component KeyChip: Rectangle {
        property string label: "" // Text displayed on the chip
        property bool active: false // Whether this chip is in capturing state
        implicitWidth: Math.max(42, keyText.implicitWidth + 20)
        implicitHeight: 26
        radius: 6
        color: active ? Theme.accent : Theme.panelStrong
        border.color: active ? Theme.accent : Theme.separator
        Behavior on color {
            ColorAnimation {
                duration: Theme.motionFast
            }
        }
        Text {
            id: keyText
            anchors.centerIn: parent
            text: parent.label
            color: parent.active ? "#06181a" : Theme.text
            font.pixelSize: 12
            font.bold: true
        }
    }

    component BindRow: Item {
        id: bindRow
        required property var action // Action definition object for this row
        readonly property bool stacked: root.width < 430
        property bool showDivider: true
        width: parent ? parent.width : 0
        height: stacked ? 68 : 44
        Rectangle {
            anchors.fill: parent
            radius: Theme.radius
            color: bindRow.capturing ? Theme.glassActive : editHover.hovered ? Theme.glassHover : "transparent"
        }
        HoverHandler { id: editHover }
        CineDivider {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            visible: bindRow.showDivider
        }

        readonly property bool capturing: root.capturingId === action.id // Is this row actively capturing a chord?
        readonly property string sequence: KeyBindings.sequenceFor(action.id) // Current key chord for this action
        readonly property bool isCustom: action.id in KeyBindings.overrides // Was the binding customized by the user?
        readonly property bool alsoBoundInMpv: { // Also mapped in mpv input.conf?
            if (!root.mpvConfig)
                return false;
            root.mpvConfig.revision; // re-evaluate when the mpv config (re)loads
            return root.mpvConfig.isOverridden(sequence);
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.right: bindRow.stacked ? parent.right : rowControls.left
            anchors.rightMargin: bindRow.stacked ? 14 : 16
            anchors.top: bindRow.stacked ? parent.top : undefined
            anchors.topMargin: bindRow.stacked ? 8 : 0
            anchors.verticalCenter: bindRow.stacked ? undefined : parent.verticalCenter
            text: bindRow.action.title
            color: Theme.text
            font.pixelSize: 14
            elide: Text.ElideRight
        }

        Row {
            id: rowControls
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.bottom: bindRow.stacked ? parent.bottom : undefined
            anchors.bottomMargin: bindRow.stacked ? 6 : 0
            anchors.verticalCenter: bindRow.stacked ? undefined : parent.verticalCenter
            spacing: 8

            // input.conf collision indicator.
            Item {
                id: overrideBadge
                anchors.verticalCenter: parent.verticalCenter
                implicitWidth: 20
                implicitHeight: 20
                visible: bindRow.alsoBoundInMpv && !bindRow.capturing

                CineIcon {
                    anchors.centerIn: parent
                    width: 16
                    height: 16
                    name: "cine-warning-symbolic"
                    tint: Theme.accent
                }

                HoverHandler {
                    id: overrideHover
                }
                ToolTip {
                    parent: overrideBadge
                    visible: overrideHover.hovered
                    delay: 150
                    text: qsTr("Overridden by input.conf: %1").arg(root.mpvConfig ? root.mpvConfig.commandFor(bindRow.sequence) : "")
                }
            }

            KeyChip {
                anchors.verticalCenter: parent.verticalCenter
                active: bindRow.capturing
                label: bindRow.capturing ? qsTr("Press keys...") : bindRow.sequence || qsTr("Unassigned")
            }

            // Reset-to-default (only when customized).
            CineButton {
                id: resetBtn
                anchors.verticalCenter: parent.verticalCenter
                visible: bindRow.isCustom && !bindRow.capturing
                styleVariant: "icon"
                iconName: "edit-undo-symbolic"
                buttonSize: 26
                iconSize: 12
                btnTooltip: qsTr("Reset %1").arg(bindRow.action.title)
                onClicked: KeyBindings.reset(bindRow.action.id)
            }

            // Edit / capture toggle.
            CineButton {
                id: editBtn
                anchors.verticalCenter: parent.verticalCenter
                styleVariant: "icon"
                iconName: "document-edit-symbolic"
                buttonSize: 26
                iconSize: 12
                btnTooltip: qsTr("Edit %1").arg(bindRow.action.title)
                checkable: true
                checked: bindRow.capturing
                checkedBackground: bindRow.capturing ? Theme.accent : "transparent"
                checkedIconTint: "#06181a"
                onClicked: {
                    if (bindRow.capturing) {
                        root.capturingId = "";
                    } else {
                        conflictText.visible = false;
                        root.capturingId = bindRow.action.id;
                        captureCatcher.forceActiveFocus();
                    }
                }
            }
        }
    }

    // Invisible focus catcher that records the next chord while capturing.
    Item {
        id: captureCatcher
        focus: root.capturingId.length > 0
        Keys.onShortcutOverride: function (event) { event.accepted = root.capturingId.length > 0; }
        Keys.onPressed: function (event) {
            if (root.capturingId.length === 0)
                return;
            if (event.key === Qt.Key_Escape) {
                root.capturingId = "";
                conflictText.visible = false;
                searchField.forceActiveFocus();
                event.accepted = true;
                return;
            }
            if (root.applyCapture(root.capturingId, event))
                root.capturingId = "";
            event.accepted = true;
        }
    }

    // --- Header ---
    Item {
        id: headerBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 54

        Text {
            id: titleLabel
            objectName: "shortcutsTitle"
            anchors.left: resetAllBtn.right
            anchors.leftMargin: 8
            anchors.right: closeBtn.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Keyboard Shortcuts")
            color: Theme.text
            font.pixelSize: 17
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
        }

        // Reset all.
        CineButton {
            id: resetAllBtn
            objectName: "shortcutsResetAll"
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            styleVariant: "text"
            btnText: qsTr("Reset all")
            width: implicitWidth
            height: 30
            onClicked: {
                root.capturingId = "";
                conflictText.visible = false;
                KeyBindings.resetAll();
            }
        }

        CineButton {
            id: closeBtn
            objectName: "shortcutsClose"
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

    TextField {
        id: searchField
        objectName: "shortcutSearch"
        anchors.top: headerBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 16
        anchors.leftMargin: 18
        anchors.rightMargin: 18
        height: 40
        placeholderText: qsTr("Search shortcuts")
        Accessible.name: qsTr("Search shortcuts")
        color: Theme.text
        placeholderTextColor: Theme.mutedText
        selectByMouse: true
        leftPadding: 12
        rightPadding: 12
        onTextChanged: {
            root.capturingId = "";
            conflictText.visible = false;
            shortcutsFlickable.contentY = 0;
        }
        background: Rectangle {
            radius: Theme.radius
            color: Theme.cardStrong
            border.color: searchField.activeFocus ? Theme.focusRing : Theme.separator
            border.width: searchField.activeFocus ? Theme.focusRingWidth : 1
        }
    }

    Text {
        id: conflictText
        anchors.top: searchField.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10
        visible: false
        text: ""
        color: Theme.danger
        font.pixelSize: 12
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
    }

    Flickable {
        id: shortcutsFlickable
        readonly property int scrollbarGutter: Theme.scrollbarGutter

        anchors.top: conflictText.visible ? conflictText.bottom : searchField.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 18
        contentWidth: Math.max(0, width - scrollbarGutter)
        contentHeight: body.implicitHeight
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        ScrollBar.vertical: ScrollBar {
            id: shortcutsScrollBar
            width: 6
            policy: ScrollBar.AsNeeded
        }

        Column {
            id: body
            width: shortcutsFlickable.contentWidth
            spacing: 18

            Repeater {
                model: root.categories
                Column {
                    id: catCol
                    required property var modelData // Category name from the repeater model
                    readonly property var matchingActions: root.actionsIn(modelData)
                    visible: matchingActions.length > 0
                    width: parent.width
                    spacing: 8

                    Text {
                        text: catCol.modelData
                        color: Theme.text
                        font.pixelSize: 12
                        font.bold: true
                        leftPadding: 12
                    }

                    Rectangle {
                        width: parent.width
                        height: sectionRows.implicitHeight + 8
                        radius: Theme.radius
                        color: Theme.card
                        Column {
                            id: sectionRows
                            width: parent.width
                            y: 4
                            Repeater {
                                model: catCol.matchingActions
                                BindRow {
                                    required property var modelData
                                    required property int index
                                    action: modelData
                                    showDivider: index < catCol.matchingActions.length - 1
                                }
                            }
                        }
                    }
                }
            }
            Text {
                width: parent.width
                visible: {
                    for (const category of root.categories) {
                        if (root.actionsIn(category).length > 0)
                            return false;
                    }
                    return true;
                }
                text: qsTr("No shortcuts found")
                color: Theme.mutedText
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Theme.fontSizeSmall
                topPadding: 24
            }
        }
    }
}
