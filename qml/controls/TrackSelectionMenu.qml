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
import CineWindows

CineMenu {
    id: root

    /// Model providing track items with trackId and label properties
    property var trackModel
    /// Text for the "Add" action button at the top; empty hides it
    property string addActionText: ""
    /// Whether the add action button is visible
    readonly property bool addActionVisible: addActionText.length > 0

    /// Emitted when user clicks the add track button
    signal addTrackRequested
    /// Emitted when a track is selected from the menu
    signal trackSelected(int trackId)

    Item {
        visible: root.addActionVisible
        implicitWidth: Math.max(0, root.width - root.leftPadding - root.rightPadding)
        implicitHeight: visible ? root.metrics.controlStandard + root.metrics.spacingXs : 0

        CineButton {
            id: addButton
            anchors.fill: parent
            anchors.margins: 4
            anchors.bottomMargin: 6
            styleVariant: "text"
            btnText: root.addActionText
            btnTooltip: ""
            visible: root.addActionVisible
            onClicked: {
                root.close();
                root.addTrackRequested();
            }
        }
    }

    CineMenuItem {
        metrics: root.metrics
        text: qsTr("None")
        checkable: true
        checked: root.trackModel ? root.trackModel.selectedTrack === 0 : true
        onTriggered: root.trackSelected(0)
    }

    Instantiator {
        model: root.trackModel

        delegate: CineMenuItem {
            required property int trackId
            required property string label

            metrics: root.metrics
            text: label
            checkable: true
            checked: root.trackModel ? root.trackModel.selectedTrack === trackId : false
            onTriggered: root.trackSelected(trackId)
        }

        onObjectAdded: (index, object) => root.insertItem(index + 2, object)
        onObjectRemoved: (index, object) => root.removeItem(object)
    }
}
