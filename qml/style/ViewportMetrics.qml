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

QtObject {
    enum WidthClass {
        CompactWidth,
        RegularWidth,
        WideWidth
    }

    enum HeightClass {
        ShortHeight,
        RegularHeight
    }

    property real viewportWidth: 1280
    property real viewportHeight: 720
    property bool compact: false
    property bool pictureInPicture: false
    property bool reducedMotion: false

    readonly property int widthClass: viewportWidth < 720
        ? ViewportMetrics.CompactWidth
        : viewportWidth < 1200
          ? ViewportMetrics.RegularWidth
          : ViewportMetrics.WideWidth
    readonly property int heightClass: viewportHeight < 480
        ? ViewportMetrics.ShortHeight
        : ViewportMetrics.RegularHeight

    // The smaller axis prevents wide, short windows from retaining desktop-scale visuals.
    readonly property real visualScale: Math.max(
        0.72,
        Math.min(1.10, Math.min(viewportWidth / 1280, viewportHeight / 720)))

    readonly property int iconXs: Math.round(Theme.iconXs * visualScale)
    readonly property int iconSm: Math.round(Theme.iconSm * visualScale)
    readonly property int iconMd: Math.round(Theme.iconMd * visualScale)
    readonly property int iconLg: Math.round(Theme.iconLg * visualScale)
    readonly property int iconDisplay: Math.round(Theme.iconDisplay * visualScale)

    // Hit targets stay stable while their visual contents scale independently.
    readonly property int controlCompact: Theme.controlCompact
    readonly property int controlStandard: Theme.controlStandard
    readonly property int controlProminent: Theme.controlProminent

    readonly property int spacingXs: Math.round(Theme.spacingXs * visualScale)
    readonly property int spacingSm: Math.round(Theme.spacingSm * visualScale)
    readonly property int spacingMd: Math.round(Theme.spacingMd * visualScale)
    readonly property int spacingLg: Math.round(Theme.spacingLg * visualScale)

    readonly property int fontCaption: Math.round(Theme.fontSizeCaption * visualScale)
    readonly property int fontBody: Math.round(Theme.fontSizeBody * visualScale)
    readonly property int fontHeading: Math.round(Theme.fontSizeHeading * visualScale)
    readonly property int fontDisplay: Math.round(Theme.fontSizeDisplay * visualScale)

    readonly property int titleBarHeight: Math.max(
        controlProminent, Math.round(Theme.titleBarHeight * visualScale))
    readonly property int sidebarWidth: compact
        ? Theme.sidebarCollapsedWidth
        : Theme.sidebarExpandedWidth
    readonly property int overlaySize: Math.round(Theme.overlaySize * visualScale)
    readonly property int overlayIconSize: Math.round(Theme.osdIconSize * visualScale)

    readonly property int windowRadius: Math.round(Theme.windowRadius * visualScale)
    readonly property int surfaceRadius: Math.round(Theme.radius * visualScale)
    readonly property int popupRadius: Math.round(Theme.popupRadius * visualScale)

    readonly property int chromeTopGradientHeight: Math.round(
        Theme.chromeTopGradientHeight * visualScale)
    readonly property int chromeBottomGradientHeight: Math.round(
        Theme.chromeBottomGradientHeight * visualScale)
}
