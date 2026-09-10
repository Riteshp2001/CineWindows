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



pragma Singleton

import QtQuick
import CineWindows

QtObject {
    readonly property bool isLight: SettingsManager.themeMode === "light"
    readonly property int motionFast: SettingsManager.reduceMotion ? 0 : 110
    readonly property int motionNormal: SettingsManager.reduceMotion ? 0 : 160
    readonly property int motionPanel: SettingsManager.reduceMotion ? 0 : 220
    /// Emphasized motion for prominent state changes; remains restrained on desktop.
    readonly property int motionEmphasized: SettingsManager.reduceMotion ? 0 : 260
    readonly property int titleBarHeight: 46
    readonly property int sidebarExpandedWidth: 232
    readonly property int sidebarCollapsedWidth: 68
    readonly property int libraryContentMargin: 20
    readonly property int libraryGridSpacing: 16
    readonly property int mediaCardMinWidth: 188
    readonly property int iconXs: 12
    readonly property int iconSm: 16
    readonly property int iconMd: 20
    readonly property int iconLg: 24
    readonly property int iconDisplay: 64
    readonly property int controlCompact: 28
    readonly property int controlStandard: 34
    readonly property int controlProminent: 40
    readonly property int spacingXs: 4
    readonly property int spacingSm: 8
    readonly property int spacingMd: 16
    readonly property int spacingLg: 20
    /// Main application background color
    readonly property color background: isLight ? "#f4f5f7" : "#050506"
    /// Panel/section background color
    readonly property color panel: isLight ? "#ffffff" : "#141417"
    /// Strong panel background for elevated surfaces
    readonly property color panelStrong: isLight ? "#e9ebef" : "#1f2024"
    /// Primary text color
    readonly property color text: isLight ? "#17181b" : "#f5f5f7"
    /// Muted / secondary text color
    readonly property color mutedText: isLight ? "#626772" : "#b8bac4"
    /// Accent / highlight color (teal)
    readonly property color accent: SettingsManager.accentColor
    /// Danger / error color (red)
    readonly property color danger: "#ff6b6b"
    /// Semi-transparent black overlay
    readonly property color glass: isLight ? "#55ffffff" : "#66000000"
    /// Hover state for glass overlays
    readonly property color glassHover: isLight ? "#16000000" : "#2bffffff"
    /// Active/pressed state for glass overlays
    readonly property color glassActive: isLight ? "#24000000" : "#3fffffff"
    /// Default corner radius
    readonly property int radius: 8
    /// Material 3-inspired shape scale used to communicate component hierarchy.
    readonly property int shapeSmall: 8
    readonly property int shapeMedium: 12
    readonly property int shapeLarge: 18
    readonly property int shapeFull: 999
    /// Keyboard focus indicator shared by interactive controls.
    readonly property color focusRing: isLight ? Qt.darker(accent, 1.18) : Qt.lighter(accent, 1.22)
    readonly property int focusRingWidth: 2
    /// Restored top-level window corner radius
    readonly property int windowRadius: 12
    /// Compact/PiP top-level window corner radius
    readonly property int windowCompactRadius: 14
    /// Subtle border width for restored top-level windows
    readonly property int windowBorderWidth: 1
    /// Subtle border color for restored top-level windows
    readonly property color windowBorderColor: isLight ? "#24000000" : "#38ffffff"
    /// Floating popup corner radius
    readonly property int popupRadius: 10
    /// Icon tint for dark backgrounds
    readonly property color iconOnDark: "#ffffff"
    /// Icon tint for light backgrounds
    readonly property color iconOnLight: "#000000"
    /// Subtle text outline color
    readonly property color textOutline: "#99000000"
    /// Stronger text outline color
    readonly property color textOutlineStrong: "#94000000"
    /// Tiny font size (10px)
    readonly property int fontSizeTiny: 10
    /// Caption font size (12px)
    readonly property int fontSizeCaption: 12
    /// Small font size (13px)
    readonly property int fontSizeSmall: 13
    /// Body font size (15px)
    readonly property int fontSizeBody: 15
    /// Heading font size (20px)
    readonly property int fontSizeHeading: 20
    /// Display / heading font size (34px)
    readonly property int fontSizeDisplay: 34

    // Pill buttons (start page / suggested actions)
    /// Primary pill button background
    readonly property color pillPrimary: isLight ? accent : "#ededed"
    /// Primary pill button hover background
    readonly property color pillPrimaryHover: isLight ? Qt.lighter(accent, 1.08) : "#ffffff"
    /// Primary pill button text color
    readonly property color pillPrimaryText: isLight ? "#ffffff" : "#0a0a0b"
    /// Secondary pill button background
    readonly property color pillSecondary: isLight ? "#e1e4e9" : "#36363a"
    /// Secondary pill button hover background
    readonly property color pillSecondaryHover: isLight ? "#d5d9e0" : "#45454b"
    /// Secondary pill button text color
    readonly property color pillSecondaryText: text
    /// Scale factor when a pill button is pressed
    readonly property real pillActiveScale: 0.98

    // Seek bar
    /// Seek bar fill/progress color
    readonly property color seekFill: "#10c7d1"
    /// Seek bar track background color
    readonly property color seekTrack: isLight ? "#28000000" : "#33ffffff"
    /// Seek bar handle color
    readonly property color seekHandle: "#ffffff"

    // Range slider (beui-style bar thumb)
    /// Range slider track background
    readonly property color sliderTrack: seekTrack
    /// Range slider fill from the start edge to the thumb
    readonly property color sliderFill: isLight ? "#2617181b" : "#26f5f5f7"
    /// Range slider tick dot color
    readonly property color sliderTick: isLight ? "#4017181b" : "#40f5f5f7"
    /// Range slider thumb color
    readonly property color sliderThumb: text
    /// Range slider reference marker line color
    readonly property color sliderMarker: "#6effffff"

    // Cards (options / preferences rows)
    /// Default card background
    readonly property color card: isLight ? "#eceef2" : "#2a2a2e"
    /// Card hover background
    readonly property color cardHover: isLight ? "#dce0e8" : "#34343a"
    /// Stronger card background
    readonly property color cardStrong: isLight ? "#f0f1f4" : "#1d1d21"
    /// Card border for light mode contrast
    readonly property color cardBorder: isLight ? "#d4d7dd" : "transparent"
    /// Separator line color
    readonly property color separator: isLight ? "#18000000" : "#22ffffff"
    /// Menu action item background
    readonly property color menuActionBackground: isLight ? "#f1f2f5" : "#242428"
    /// Timeline divider line color
    readonly property color timelineDivider: "#66dddddd"
    /// Chapter marker color
    readonly property color chapterMarker: "#e8ffffff"

    // Floating panels / popovers
    /// Popover/bubble background color
    readonly property color popover: isLight ? "#ffffff" : "#19191b"
    /// Popover border color
    readonly property color popoverBorder: isLight ? "#d7dae0" : "#212122"
    /// Popover drop shadow color
    readonly property color popoverShadow: "#66000000"

    // OSD / toast / overlay
    /// OSD background color
    readonly property color osdBg: "#80000000"
    /// OSD corner radius
    readonly property int osdRadius: 20
    /// OSD container size in pixels
    readonly property int overlaySize: 112
    /// OSD icon size in pixels
    readonly property int osdIconSize: 64
    /// Audio-only indicator background
    readonly property color audioOnlyBackground: "#22000000"
    /// Top chrome gradient height
    readonly property int chromeTopGradientHeight: 120
    /// Bottom chrome gradient height
    readonly property int chromeBottomGradientHeight: 180
    /// Top chrome gradient start color
    readonly property color chromeTopGradientStart: "#24000000"
    /// Top chrome gradient midpoint color
    readonly property color chromeTopGradientMid: "#14000000"
    /// Bottom chrome gradient midpoint color
    readonly property color chromeBottomGradientMid: "#1a000000"
    /// Bottom chrome gradient end color
    readonly property color chromeBottomGradientEnd: "#33000000"
    /// Control scrim gradient start (transparent)
    readonly property color controlScrimStart: "#00000000"
    /// Control scrim gradient end
    readonly property color controlScrimEnd: "#33000000"

    // Text & icon shadows
    /// Shadow opacity for text/icon outlines
    readonly property real textShadowOpacity: 0.6
    /// Shadow blur radius for text/icon outlines
    readonly property int textShadowRadius: 6
    /// Shadow offset for text/icon outlines
    readonly property int textShadowOffset: 1

    // Playlist
    /// Playlist drawer background
    readonly property color playlistBg: "#c7000000"
    /// Playlist row background
    readonly property color playlistRowBg: "#b3606061"
    /// Playlist row hover background
    readonly property color playlistRowHover: "#b3505052"
    /// Playlist row active background
    readonly property color playlistRowActive: "#cc505052"
    /// Playing item highlight color
    readonly property color playlistPlayingHighlight: "#26f5f5ff"
    /// Playing item border color
    readonly property color playlistPlayingBorder: "#3584e4"
    /// Playing item icon color
    readonly property color playlistPlayingIcon: "#3584e4"

    // Checked toggle glow
    /// Toggle checked hover glow
    readonly property color toggleCheckedHoverGlow: "#ffffff"
    /// Toggle checked active glow
    readonly property color toggleCheckedActiveGlow: "#c2c2c2"

    // Drop indicator
    /// Drop overlay background
    readonly property color dropBg: "#dd141417"
    /// Drop overlay border color
    readonly property color dropBorder: "#10c7d1"
    /// Drop overlay corner radius
    readonly property int dropRadius: 7

    // Spinner
    /// Spinner control size in pixels
    readonly property int spinnerSize: 90
    /// Spinner rotation duration in milliseconds.
    readonly property int spinnerRotationDuration: SettingsManager.reduceMotion ? 0 : 900

    // Header bar
    /// Header background (semi-transparent overlay)
    readonly property color headerBg: isLight ? "#c4f0f1f4" : "#c414161b"
    /// Header hover state
    readonly property color headerHover: isLight ? Qt.alpha(accent, 0.18) : "#24ffffff"
    /// Header active/pressed state
    readonly property color headerActive: isLight ? Qt.alpha(accent, 0.30) : "#36ffffff"
    /// Header border color
    readonly property color headerBorder: isLight ? "#28000000" : "#32ffffff"
    /// Header icon tint
    readonly property color headerIcon: isLight ? "#17181b" : "#f4f7fa"
    /// Header label text
    readonly property color headerText: isLight ? "#17181b" : "#f4f7fa"
    /// Header chevron tint
    readonly property color headerChevron: isLight ? "#626772" : "#aeb5c0"
    /// Header title text outline color (light: subtle, dark: stronger)
    readonly property color headerTitleOutline: isLight ? Qt.rgba(1, 1, 1, 0.7) : Qt.rgba(0, 0, 0, 0.6)

    // Switch track (OFF state)
    /// Switch track background when unchecked
    readonly property color switchTrackOff: isLight ? "#c8c8cc" : "#1d1d21"
    /// Switch thumb color
    readonly property color switchThumb: "#ffffff"

    // Window controls background
    /// Window control button hover background
    readonly property color windowCtrlHover: isLight ? Qt.rgba(0, 0, 0, 0.08) : "#24ffffff"
    /// Window control button active/pressed background
    readonly property color windowCtrlActive: isLight ? Qt.rgba(0, 0, 0, 0.12) : "#36ffffff"
    /// Close button hover background
    readonly property color closeHover: isLight ? "#ff6b6b" : Theme.danger

    // Icon defaults
    /// Default icon tint for light backgrounds
    readonly property color iconDefault: isLight ? "#17181b" : "#ffffff"
    /// Spinner arc color
    readonly property color spinnerArc: isLight ? Theme.accent : "#ffffff"

    // Scrollbar
    /// Scrollbar thumb rest color
    readonly property color scrollbarThumb: isLight ? "#c0c0c4" : "#55ffffff"
    /// Scrollbar thumb active/hover color
    readonly property color scrollbarThumbHover: isLight ? "#999999" : "#99ffffff"

    // Tooltip
    /// Tooltip border color
    readonly property color tooltipBorder: isLight ? "#d4d7dd" : "#33ffffff"

    // Color chip / swatch border
    /// Inner border for color chips
    readonly property color chipBorder: isLight ? "#20000000" : "#44ffffff"

    // Search field background (dark overlay panels)
    /// Search field background for overlay panels
    readonly property color searchBg: isLight ? "#f0f1f4" : "#bf1b1b1e"
    /// Search field border
    readonly property color searchBorder: isLight ? "#d4d7dd" : "#28ffffff"
    /// Search field focused border
    readonly property color searchBorderFocus: isLight ? Theme.accent : "#66ffffff"
}
