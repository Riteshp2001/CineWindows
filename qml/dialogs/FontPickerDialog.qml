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

// Searchable font picker styled to match CineWindows. Each family name is
// rendered in its own font, with a live preview line and Cancel / Select.
ResponsivePopup {
    id: root

    parent: Overlay.overlay
    metrics: ViewportMetrics {
        viewportWidth: root.parent ? root.parent.width : root.preferredWidth
        viewportHeight: root.parent ? root.parent.height : root.preferredHeight
    }
    modal: true
    dim: true
    preferredWidth: 440
    preferredHeight: 560
    padding: 0
    focus: true

    /// Currently highlighted font family
    property string currentFamily: ""
    /// Emitted with the selected font family when user presses Select
    signal accepted(string family)

    /// Opens the dialog pre-selected to the given font family
    function openFor(family) {
        currentFamily = family || "";
        searchField.text = "";
        rebuildModel();
        Qt.callLater(scrollToCurrent);
        open();
    }

    /// Complete list of font families available on this system (read-only)
    readonly property var allFamilies: Qt.fontFamilies()
    /// Filtered list of font families matching the search query
    property var families: []
    /// True when Cancel / Select buttons stack vertically instead of sitting side-by-side
    readonly property bool actionsWrapped: actionFlow.height > 38

    /// Rebuilds the filtered families list based on the search field text
    function rebuildModel() {
        var query = searchField.text.trim().toLowerCase(); // Normalized search text for case-insensitive matching
        if (query.length === 0) {
            families = allFamilies;
            return;
        }
        var out = []; // Accumulator for families that match the query
        for (var i = 0; i < allFamilies.length; ++i) { // Iterate over every system font family
            if (allFamilies[i].toLowerCase().indexOf(query) !== -1)
                out.push(allFamilies[i]);
        }
        families = out;
    }

    /// Scrolls the font list to the currently selected family
    function scrollToCurrent() {
        for (var i = 0; i < families.length; ++i) {
            if (families[i] === currentFamily) {
                fontList.positionViewAtIndex(i, ListView.Center);
                fontList.currentIndex = i;
                return;
            }
        }
    }

    background: Rectangle {
        radius: 16
        color: Theme.popover
        opacity: 0.97
        border.color: Theme.popoverBorder
        border.width: 1
    }

    // --- Header ---
    Item {
        id: headerBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 52
        Text {
            anchors.centerIn: parent
            text: qsTr("Pick a Font")
            color: Theme.text
            font.pixelSize: 17
            font.bold: true
        }
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

    // --- Search field ---
    Rectangle {
        id: searchBox
        anchors.top: headerBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: 14
        height: 40
        radius: 10
        color: Theme.cardStrong
        border.width: searchField.activeFocus ? 1 : 0
        border.color: Theme.accent

        Item {
            id: searchIcon
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            width: 16
            height: 16
            Rectangle {
                width: 10
                height: 10
                radius: 5
                color: "transparent"
                border.width: 1.6
                border.color: Theme.mutedText
                x: 0
                y: 0
            }
            Rectangle {
                width: 5
                height: 1.6
                radius: 1
                color: Theme.mutedText
                x: 9
                y: 11
                rotation: 45
                transformOrigin: Item.Left
            }
        }
        TextField {
            id: searchField
            anchors.left: searchIcon.right
            anchors.leftMargin: 8
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            placeholderText: qsTr("Search font name")
            placeholderTextColor: Theme.mutedText
            color: Theme.text
            font.pixelSize: 14
            background: Item {}
            onTextChanged: {
                root.rebuildModel();
                fontList.currentIndex = -1;
            }
        }
    }

    // --- Font list ---
    ListView {
        id: fontList
        anchors.top: searchBox.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: previewBar.top
        anchors.leftMargin: 8
        anchors.rightMargin: 6
        anchors.topMargin: 10
        anchors.bottomMargin: 8
        clip: true
        model: root.families
        boundsBehavior: Flickable.StopAtBounds
        currentIndex: -1
        ScrollBar.vertical: ScrollBar {
            width: 8
            policy: ScrollBar.AsNeeded
            contentItem: Rectangle {
                implicitWidth: 4
                radius: 2
                color: Theme.scrollbarThumb
            }
            background: Rectangle {
                color: "transparent"
            }
        }

        delegate: ItemDelegate {
            id: fontItem
            required property var modelData
            required property int index
            width: ListView.view ? ListView.view.width : fontList.width
            height: 42
            highlighted: root.currentFamily === modelData
            HoverHandler { cursorShape: Qt.PointingHandCursor }
            onClicked: {
                root.currentFamily = modelData;
                fontList.currentIndex = index;
            }
            background: Rectangle {
                anchors.fill: parent
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                anchors.topMargin: 2
                anchors.bottomMargin: 2
                radius: 8
                color: fontItem.highlighted ? Theme.accent : (fontItem.hovered ? Theme.cardHover : "transparent")
                Behavior on color {
                    ColorAnimation {
                        duration: Theme.motionFast
                    }
                }
            }
            contentItem: Text {
                leftPadding: 16
                rightPadding: 12
                text: fontItem.modelData
                color: fontItem.highlighted ? "#06181a" : Theme.text
                font.family: fontItem.modelData
                font.pixelSize: 17
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
    }

    // --- Preview line ---
    Rectangle {
        id: previewBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: actionFlow.top
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 12
        height: 44
        radius: 10
        color: Theme.cardStrong
        Text {
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("The quick brown fox jumps over the lazy dog.")
            color: Theme.text
            font.family: root.currentFamily.length > 0 ? root.currentFamily : "sans-serif"
            font.pixelSize: 15
            elide: Text.ElideRight
        }
    }

    // --- Cancel / Select ---
    Flow {
        id: actionFlow
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.bottomMargin: 14
        width: Math.min(194, Math.max(0, parent.width - 32))
        height: childrenRect.height
        spacing: 10

        CineButton {
            id: cancelBtn
            styleVariant: "dialog"
            colorVariant: "default"
            btnText: qsTr("Cancel")
            width: 92
            height: 38
            onClicked: root.close()
        }
        CineButton {
            id: selectBtn
            styleVariant: "dialog"
            colorVariant: "primary"
            btnText: qsTr("Select")
            width: 92
            height: 38
            enabled: root.currentFamily.length > 0
            onClicked: {
                root.accepted(root.currentFamily);
                root.close();
            }
        }
    }
}
