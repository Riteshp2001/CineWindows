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
import CineWindows

Popup {
    id: root

    required property ViewportMetrics metrics
    property real preferredWidth: implicitContentWidth + leftPadding + rightPadding
    property real preferredHeight: implicitContentHeight + topPadding + bottomPadding
    property int edgeMargin: 8
    readonly property bool compactLayout: metrics.widthClass === ViewportMetrics.CompactWidth
    readonly property bool shortLayout: metrics.heightClass === ViewportMetrics.ShortHeight
    readonly property real availablePopupWidth: parent
        ? Math.max(0, parent.width - 2 * edgeMargin)
        : preferredWidth
    readonly property real availablePopupHeight: parent
        ? Math.max(0, parent.height - 2 * edgeMargin)
        : preferredHeight

    width: Math.min(preferredWidth, availablePopupWidth)
    height: Math.min(preferredHeight, availablePopupHeight)
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
}
