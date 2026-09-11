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
import QtQuick.Window

Item {
    id: root

    /// The window that this resize handles control
    property var targetWindow
    /// Whether the window uses application-side decorations.
    property bool clientSideDecorated: true
    /// Thickness of the interactive resize border in pixels
    readonly property int borderSize: 6

    visible: clientSideDecorated
        && targetWindow
        && targetWindow.visibility === Window.Windowed

    MouseArea {
        width: root.borderSize
        height: root.borderSize
        anchors.left: parent.left
        anchors.top: parent.top
        cursorShape: Qt.SizeFDiagCursor
        onPressed: root.targetWindow.startSystemResize(Qt.TopLeftCorner)
    }

    MouseArea {
        height: root.borderSize
        anchors.left: parent.left
        anchors.leftMargin: root.borderSize
        anchors.right: parent.right
        anchors.rightMargin: root.borderSize
        anchors.top: parent.top
        cursorShape: Qt.SizeVerCursor
        onPressed: root.targetWindow.startSystemResize(Qt.TopEdge)
    }

    MouseArea {
        width: root.borderSize
        height: root.borderSize
        anchors.right: parent.right
        anchors.top: parent.top
        cursorShape: Qt.SizeBDiagCursor
        onPressed: root.targetWindow.startSystemResize(Qt.TopRightCorner)
    }

    MouseArea {
        width: root.borderSize
        anchors.top: parent.top
        anchors.topMargin: root.borderSize
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.borderSize
        anchors.right: parent.right
        cursorShape: Qt.SizeHorCursor
        onPressed: root.targetWindow.startSystemResize(Qt.RightEdge)
    }

    MouseArea {
        width: root.borderSize
        height: root.borderSize
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        cursorShape: Qt.SizeFDiagCursor
        onPressed: root.targetWindow.startSystemResize(Qt.BottomRightCorner)
    }

    MouseArea {
        height: root.borderSize
        anchors.left: parent.left
        anchors.leftMargin: root.borderSize
        anchors.right: parent.right
        anchors.rightMargin: root.borderSize
        anchors.bottom: parent.bottom
        cursorShape: Qt.SizeVerCursor
        onPressed: root.targetWindow.startSystemResize(Qt.BottomEdge)
    }

    MouseArea {
        width: root.borderSize
        height: root.borderSize
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        cursorShape: Qt.SizeBDiagCursor
        onPressed: root.targetWindow.startSystemResize(Qt.BottomLeftCorner)
    }

    MouseArea {
        width: root.borderSize
        anchors.top: parent.top
        anchors.topMargin: root.borderSize
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.borderSize
        anchors.left: parent.left
        cursorShape: Qt.SizeHorCursor
        onPressed: root.targetWindow.startSystemResize(Qt.LeftEdge)
    }
}
