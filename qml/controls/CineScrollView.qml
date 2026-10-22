import QtQuick
import QtQuick.Controls.Basic
import CineWindows

ScrollView {
    id: root
    clip: true
    rightPadding: Theme.scrollbarGutter
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ScrollBar.vertical: ScrollBar {
        id: verticalBar
        parent: root
        x: root.width - width
        y: root.topPadding
        width: 8
        height: root.availableHeight
        policy: ScrollBar.AsNeeded
        background: Item {}
        contentItem: Rectangle {
            implicitWidth: 4
            radius: 2
            color: verticalBar.hovered || verticalBar.pressed
                ? Theme.scrollbarThumbHover : Theme.scrollbarThumb
        }
    }
}