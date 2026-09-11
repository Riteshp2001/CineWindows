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
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Dialogs
import CineWindows

ApplicationWindow {
    id: window
    readonly property bool hasSavedSize: SettingsManager.initialSize.width >= 400 && SettingsManager.initialSize.height >= 300 &&
                                         (SettingsManager.initialSize.width / SettingsManager.initialSize.height) >= 1.2 &&
                                         (SettingsManager.initialSize.width / SettingsManager.initialSize.height) <= 2.4
    readonly property int defaultWidth: 1200
    readonly property int defaultHeight: 800
    readonly property int initialWidth: Math.min(
        hasSavedSize ? SettingsManager.initialSize.width : defaultWidth,
        Math.max(1, Screen.desktopAvailableWidth))
    readonly property int initialHeight: Math.min(
        hasSavedSize ? SettingsManager.initialSize.height : defaultHeight,
        Math.max(1, Screen.desktopAvailableHeight))

    width: initialWidth
    height: initialHeight
    x: Screen.virtualX + Math.round((Screen.desktopAvailableWidth - initialWidth) / 2)
    y: Screen.virtualY + Math.round((Screen.desktopAvailableHeight - initialHeight) / 2)
    minimumWidth: compactMode > 0 ? 320 : Math.min(900, defaultWidth)
    minimumHeight: compactMode > 0 ? 180 : Math.min(560, defaultHeight)
    visible: true
    color: "transparent"
    title: player.mediaTitle.length > 0
        ? player.mediaTitle + " - " + Qt.application.displayName
        : Qt.application.displayName
    flags: Qt.Window | Qt.FramelessWindowHint | (compactMode === 2 ? Qt.WindowStaysOnTopHint : 0)

    property int normalWidth: initialWidth ///< Saved width of windowed geometry for restore after fullscreen/maximize
    property int normalHeight: initialHeight ///< Saved height of windowed geometry for restore after fullscreen/maximize
    property int normalX: Screen.virtualX + Math.round((Screen.width - normalWidth) / 2) ///< Saved X position of windowed geometry for restore after fullscreen/maximize
    property int normalY: Screen.virtualY + Math.round((Screen.height - normalHeight) / 2) ///< Saved Y position of windowed geometry for restore after fullscreen/maximize
    property bool deferredStartupStarted: false ///< Prevents first-frame startup work from running more than once
    property bool sessionRestoreAttempted: false ///< Prevents an early close from replacing a session before it was restored
    property var startupPaths: [] ///< Files or URLs supplied by the desktop shell or command line

    // True while a fullscreen/maximize transition is in flight. Used to
    // (a) reject re-entrant toggles that previously corrupted native window
    // state and crashed the app, and (b) stop the geometry handlers below from
    // saving intermediate transition values as the "normal" geometry.
    property bool stateTransitioning: false ///< Prevents re-entrant fullscreen/maximize toggles during transitions
    property bool wasMaximizedBeforeFullscreen: false ///< Remembers maximized state before entering fullscreen
    readonly property bool isFullscreen: window.visibility === Window.FullScreen
    property bool ignoreVolumeOsd: true
    property int compactMode: 0 ///< 0 normal, 2 picture in picture
    property bool pipControlsVisible: true
    property bool pipTransparent: false
    property int pipSubtitleTrack: 0

    ViewportMetrics {
        id: viewportMetrics
        viewportWidth: window.width
        viewportHeight: window.height
        compact: window.compactMode > 0
        pictureInPicture: window.compactMode === 2
        reducedMotion: SettingsManager.reduceMotion
    }

    WindowCornerPreference {
        targetWindow: window
        rounded: rootContainer.rounded
    }

    onWidthChanged: {
        if (compactMode === 0 && window.visibility === Window.Windowed && !minimizeAnimation.running && !stateTransitioning && !isFullscreen)
            normalWidth = window.width;
    }
    onHeightChanged: {
        if (compactMode === 0 && window.visibility === Window.Windowed && !minimizeAnimation.running && !stateTransitioning && !isFullscreen)
            normalHeight = window.height;
    }
    onXChanged: {
        if (compactMode === 0 && window.visibility === Window.Windowed && !minimizeAnimation.running && !stateTransitioning && !isFullscreen)
            normalX = window.x;
    }
    onYChanged: {
        if (compactMode === 0 && window.visibility === Window.Windowed && !minimizeAnimation.running && !stateTransitioning && !isFullscreen)
            normalY = window.y;
    }

    /// Records the current window position and size into normal geometry properties.
    function captureNormalGeometry() {
        normalX = window.x;
        normalY = window.y;
        normalWidth = window.width;
        normalHeight = window.height;
    }

    /// Returns the current screen area not reserved by taskbars or system UI.
    function availableScreenGeometry() {
        return Qt.rect(Screen.virtualX, Screen.virtualY,
                       Screen.desktopAvailableWidth, Screen.desktopAvailableHeight);
    }

    /// Clamps a window rectangle so its complete surface remains reachable.
    function correctedWindowGeometry(x, y, width, height, bounds) {
        const correctedWidth = Math.max(1, Math.min(width, bounds.width));
        const correctedHeight = Math.max(1, Math.min(height, bounds.height));
        const maximumX = bounds.x + bounds.width - correctedWidth;
        const maximumY = bounds.y + bounds.height - correctedHeight;
        return {
            x: Math.max(bounds.x, Math.min(x, maximumX)),
            y: Math.max(bounds.y, Math.min(y, maximumY)),
            width: correctedWidth,
            height: correctedHeight
        };
    }

    function ensureWindowGeometryVisible() {
        const corrected = correctedWindowGeometry(
            window.x, window.y, window.width, window.height,
            availableScreenGeometry());
        window.x = corrected.x;
        window.y = corrected.y;
        window.width = corrected.width;
        window.height = corrected.height;
        if (compactMode === 0)
            captureNormalGeometry();
    }

    /// Restores the window to its previously captured normal position and size.
    function restoreNormalGeometry() {
        const corrected = correctedWindowGeometry(
            normalX, normalY, normalWidth, normalHeight,
            availableScreenGeometry());
        window.x = corrected.x;
        window.y = corrected.y;
        window.width = corrected.width;
        window.height = corrected.height;
        captureNormalGeometry();
    }

    NumberAnimation {
        id: fadeAnimation
        target: window
        property: "opacity"
        from: 0.0
        to: 1.0
        duration: Theme.motionPanel
        easing.type: Easing.OutCubic
    }

    ParallelAnimation {
        id: minimizeAnimation
        NumberAnimation {
            target: window
            property: "opacity"
            from: 1.0
            to: 0.0
            duration: Theme.motionNormal
            easing.type: Easing.InCubic
        }
        NumberAnimation {
            target: window
            property: "y"
            to: window.visibility === Window.Windowed
                ? Screen.virtualY + Screen.desktopAvailableHeight
                : window.y
            duration: Theme.motionNormal
            easing.type: Easing.InCubic
        }
        onFinished: {
            window.visibility = Window.Minimized;
            // Restore the remembered geometry (centered half-screen by default,
            // or the user's resized values) so the window reappears correctly
            // placed instead of at the slid-off-screen position the slide left.
            window.restoreNormalGeometry();
            window.opacity = 1.0;
        }
    }

    /// Fires the pending state transition callback and clears the transitioning flag.
    Timer {
        id: stateTransitionTimer
        interval: 200
        repeat: false
        property var callback: null
        onTriggered: {
            var pendingCallback = callback;
            callback = null;
            if (pendingCallback)
                pendingCallback();
            window.stateTransitioning = false;
        }
    }

    /// Begins a fullscreen/maximize transition guard and fires the callback after the transition timer expires.
    /// @param callback Optional function to execute after the transition timer completes.
    function beginWindowStateTransition(callback) {
        stateTransitioning = true;
        stateTransitionTimer.stop();
        stateTransitionTimer.callback = callback || null;
        stateTransitionTimer.start();
    }

    /// Toggles the window between fullscreen and windowed/maximized state, preserving geometry.
    function toggleFullscreen() {
        if (stateTransitioning)
            return;
        if (window.isFullscreen) {
            if (wasMaximizedBeforeFullscreen) {
                beginWindowStateTransition(null);
                window.visibility = Window.Maximized;
            } else {
                beginWindowStateTransition(function () {
                    window.restoreNormalGeometry();
                });
                window.visibility = Window.Windowed;
            }
        } else {
            if (window.visibility === Window.Maximized) {
                wasMaximizedBeforeFullscreen = true;
            } else {
                wasMaximizedBeforeFullscreen = false;
                captureNormalGeometry();
            }
            beginWindowStateTransition(null);
            window.visibility = Window.FullScreen;
        }
    }

    /// Toggles the window between maximized and windowed state, preserving geometry for restore.
    function toggleMaximize() {
        if (stateTransitioning)
            return;
        if (window.isFullscreen) {
            toggleFullscreen();
            return;
        }
        if (window.visibility === Window.Maximized) {
            beginWindowStateTransition(function () {
                window.restoreNormalGeometry();
            });
            window.visibility = Window.Windowed;
        } else {
            if (!window.isFullscreen)
                captureNormalGeometry();
            beginWindowStateTransition(null);
            window.visibility = Window.Maximized;
        }
    }

    /// Minimizes the window with a slide-out animation after capturing normal geometry.
    function minimizeWindow() {
        if (window.visibility === Window.Windowed && !isFullscreen)
            captureNormalGeometry();
        minimizeAnimation.start();
    }

    property bool chromeVisible: true ///< Whether the header bar and control bar are visible
    property bool cursorHidden: false ///< Whether the cursor is hidden (blank cursor in fullscreen)
    property bool dropActive: false ///< Whether a drag-and-drop operation is currently active over the window
    property var appHandledKeys: ({}) ///< Set of key codes that have been handled by the app to prevent re-handling on release
    property int accumulatedSeekOsdSeconds: 0 ///< Accumulated seek offset in seconds for OSD display aggregation
    property int statsPage: 1 ///< Currently selected mpv stats page number (0-5)
    property bool statsVisible: false ///< Whether the mpv stats overlay is visible
    property bool statsPinned: false ///< Whether the stats overlay is pinned (does not auto-hide)
    property bool consoleInputActive: false ///< Whether the mpv console input mode is active
    property bool mediaHubVisible: true ///< Whether the browsing, library, and history hub is visible
    property string mediaHubSection: "home" ///< Active Media Hub section
    readonly property bool mediaActive: player.currentPath.length > 0 && !player.idle
    property string dropActionIcon: "cine-playback-start-symbolic" ///< Icon name shown in the drop overlay
    property string dropActionText: qsTr("Play") ///< Label text shown in the drop overlay

    readonly property bool anyPopupOpen: {
        return videoContextMenu.popupVisible || subtitleMenu.popupVisible || audioMenu.popupVisible || videoMenu.popupVisible || chaptersMenu.popupVisible || playlistDrawer.popupVisible || preferencesDialog.popupVisible || optionsDialog.popupVisible || aboutDialog.popupVisible || shortcutsDialog.popupVisible || urlDialog.popupVisible || addUrlDialog.popupVisible || (typeof controls !== "undefined" && controls.volumePopupOpen) || (typeof header !== "undefined" && (header.openMenuOpen || header.mainMenuOpen));
    }

    /// Converts a QKeyEvent into a canonical key sequence string via the KeyBindings registry.
    /// @param event The key event to convert.
    /// @returns The canonical sequence string, or empty string if unmappable.
    function sequenceFromKeyEvent(event) {
        return KeyBindings.sequenceFromEvent(event);
    }

    /// Looks up the action identifier bound to a given key sequence.
    /// @param sequence The canonical key sequence string to look up.
    /// @returns The matching action id string, or empty string if no match.
    function actionForSequence(sequence) {
        KeyBindings.revision;
        var actions = KeyBindings.actions;
        for (var i = 0; i < actions.length; ++i) {
            if (KeyBindings.sequenceFor(actions[i].id) === sequence)
                return actions[i].id;
        }
        return "";
    }

    /// Returns true if the sequence is a registered window shortcut (action or help dialog).
    /// @param sequence The canonical key sequence string to test.
    /// @returns True if the sequence matches a shortcut or the help key combo.
    function isWindowShortcut(sequence) {
        return sequence === "Ctrl+?" || sequence === "Ctrl+Shift+/" || actionForSequence(sequence).length > 0;
    }

    /// Dispatches a key sequence to the shortcuts dialog, stats overlay, or an action.
    /// @param sequence The canonical key sequence string to dispatch.
    /// @returns True if the sequence was handled by any dispatch target.
    function dispatchKeySequence(sequence) {
        if (sequence === "Ctrl+?" || sequence === "Ctrl+Shift+/") {
            shortcutsDialog.open();
            return true;
        }

        if (dispatchStatsSequence(sequence))
            return true;

        var actionId = actionForSequence(sequence);
        if (actionId.length === 0)
            return false;
        dispatch(actionId);
        return true;
    }

    /// Maps a canonical key sequence to the mpv stats key equivalent.
    /// @param sequence The canonical key sequence string to translate.
    /// @returns The mpv key name string, or empty string if unmappable.
    function mpvStatsKeyForSequence(sequence) {
        switch (sequence) {
        case "0":
            return "0";
        case "1":
            return "1";
        case "2":
            return "2";
        case "3":
            return "3";
        case "4":
            return "4";
        case "5":
            return "5";
        case "Up":
            return "UP";
        case "Down":
            return "DOWN";
        case "/":
            return "/";
        case "Esc":
            return "ESC";
        }
        return "";
    }

    /// Dispatches a key sequence to the mpv stats overlay when it is visible.
    /// @param sequence The canonical key sequence to forward to stats.
    /// @returns True if the sequence was consumed by the stats overlay.
    function dispatchStatsSequence(sequence) {
        if (!statsVisible)
            return false;

        var key = mpvStatsKeyForSequence(sequence);
        if (key.length === 0)
            return false;

        player.runCommandAsync(["keypress", key]);
        if (sequence >= "0" && sequence <= "5")
            statsPage = Number(sequence);

        if (sequence === "Esc") {
            statsVisible = false;
            statsPinned = false;
            statsOneshotTimer.stop();
        } else if (!statsPinned) {
            statsOneshotTimer.restart();
        }
        return true;
    }

    /// Shows a specific mpv stats page, activating the overlay if not already visible.
    /// @param page The stats page number (1-5) to display.
    function showStatsPage(page) {
        statsPage = page;
        if (statsVisible) {
            player.runCommandAsync(["script-binding", "stats/display-page-" + page]);
            if (!statsPinned)
                statsOneshotTimer.restart();
            return;
        }

        statsVisible = true;
        statsPinned = false;
        statsOneshotTimer.restart();
        player.runCommandAsync(["script-binding", "stats/display-page-" + page]);
    }

    /// Shows the mpv stats overlay temporarily (oneshot, unpinned).
    function showStatsOneshot() {
        statsVisible = true;
        statsPinned = false;
        statsOneshotTimer.restart();
        player.runCommandAsync(["script-binding", "stats/display-stats"]);
    }

    /// Toggles the mpv stats overlay between pinned and hidden states.
    function toggleStatsOverlay() {
        if (statsVisible && !statsPinned) {
            player.runCommandAsync(["script-binding", "stats/display-stats-toggle"]);
            return;
        }

        statsPinned = !statsPinned;
        statsVisible = statsPinned;
        statsOneshotTimer.stop();
        player.runCommandAsync(["script-binding", "stats/display-stats-toggle"]);
    }

    /// Toggles a specific persistent mpv stats page while keeping local routing state in sync.
    /// @param page The stats page number (0-5) to toggle.
    function toggleStatsPage(page) {
        if (statsVisible && !statsPinned) {
            showStatsPage(page);
            return;
        }

        var shouldShow = !statsVisible;
        statsPage = page;
        statsVisible = shouldShow;
        statsPinned = shouldShow;
        statsOneshotTimer.stop();
        player.runCommandAsync(["script-binding", "stats/display-page-" + page + "-toggle"]);
    }

    /// Opens the mpv console input for text commands.
    function openMpvConsole() {
        consoleInputActive = true;
        player.runCommandAsync(["script-binding", "commands/open"]);
    }

    /// Marks a key code as handled by the app so the key release event is consumed.
    /// @param key The Qt key code to mark.
    function markAppHandledKey(key) {
        appHandledKeys[String(key)] = true;
    }

    /// Checks and removes a previously marked key, returning whether it was consumed.
    /// @param key The Qt key code to test and consume.
    /// @returns True if the key was previously marked as handled.
    function consumeAppHandledKey(key) {
        var keyId = String(key);
        if (!appHandledKeys[keyId])
            return false;
        delete appHandledKeys[keyId];
        return true;
    }

    /// Opens the Media Hub and unloads active media so hidden playback cannot continue.
    /// @param section Hub section to display.
    function enterMediaHub(section) {
        if (section && section.length > 0)
            mediaHubSection = section;
        if (mediaHubVisible)
            return;

        mediaHubVisible = true;
        if (mediaActive)
            controller.stop();
        mediaLibrary.refresh();
    }

    /// Closes the Media Hub when new media is ready to be shown.
    function leaveMediaHub() {
        mediaHubVisible = false;
    }

    PlaylistModel {
        id: playlistModel
    }
    TrackModel {
        id: subtitleTracks
        filterType: "sub"
    }
    TrackModel {
        id: audioTracks
        filterType: "audio"
    }
    TrackModel {
        id: videoTracks
        filterType: "video"
    }
    ChapterModel {
        id: chapterModel
    }
    FileService {
        id: fileService
    }

    function enterCompactMode(mode) {
        if (!mediaActive || mode !== 2)
            return;
        window.leaveMediaHub();
        if (compactMode === 0 && visibility === Window.Windowed)
            captureNormalGeometry();
        if (isFullscreen || visibility === Window.Maximized)
            visibility = Window.Windowed;
        compactMode = mode;
        pipTransparent = false;
        opacity = 1.0;
        pipControlsVisible = true;
        pipSubtitleTrack = subtitleTracks.selectedTrack > 0 ? subtitleTracks.selectedTrack : 0;
        const bounds = availableScreenGeometry();
        const compactGeometry = correctedWindowGeometry(
            bounds.x + bounds.width - 384 - 24,
            bounds.y + bounds.height - 216 - 24,
            384, 216, bounds);
        width = compactGeometry.width;
        height = compactGeometry.height;
        x = compactGeometry.x;
        y = compactGeometry.y;
        raise();
        requestActivate();
        Qt.callLater(schedulePipControlsHide);
    }

    function exitCompactMode() {
        if (compactMode === 0)
            return;
        pipControlsHideTimer.stop();
        compactMode = 0;
        pipTransparent = false;
        pipControlsVisible = true;
        opacity = 1.0;
        restoreNormalGeometry();
        showChrome(2200);
    }

    function pipControlsPinned() {
        return player.pause || pipSeekbar.pressed || pipTopLeftHover.hovered
            || pipTopRightHover.hovered || pipCenterHover.hovered || pipSeekHover.hovered;
    }

    function schedulePipControlsHide() {
        pipControlsHideTimer.stop();
        if (compactMode === 2 && mediaActive && !pipControlsPinned())
            pipControlsHideTimer.start();
    }

    function showPipControls() {
        if (compactMode !== 2)
            return;
        pipControlsVisible = true;
        schedulePipControlsHide();
    }

    function togglePipTransparency() {
        pipTransparent = !pipTransparent;
        opacity = pipTransparent ? 0.7 : 1.0;
        showPipControls();
    }

    function togglePipCaptions() {
        if (subtitleTracks.selectedTrack > 0) {
            pipSubtitleTrack = subtitleTracks.selectedTrack;
            selectSubtitleTrack(0);
        } else if (pipSubtitleTrack > 0) {
            selectSubtitleTrack(pipSubtitleTrack);
        }
        showPipControls();
    }

    Timer {
        id: pipControlsHideTimer
        interval: 2000
        repeat: false
        onTriggered: {
            if (window.compactMode === 2 && !window.pipControlsPinned())
                window.pipControlsVisible = false;
        }
    }
    FileBrowserModel {
        id: fileBrowserModel
    }
    MediaLibraryService {
        id: mediaLibrary
        player: player
        onUserMessage: function (message) { toastText.showMessage(message); }
        onIndexingChanged: if (!indexing && tmdbMetadata.enabled) tmdbMetadata.refresh()
    }
    TmdbMetadataService {
        id: tmdbMetadata
        library: mediaLibrary
    }
    MediaThumbnailService {
        id: mediaThumbnailService
    }
    SessionManager {
        id: sessionManager
    }
    UpdateService {
        id: appUpdateService
        onUpdateAvailableChanged: {
            if (updateAvailable)
                toastText.showMessage(qsTr("CineWindows %1 is available").arg(latestVersion));
        }
    }
    InputRouter {
        id: inputRouter
        player: player
    }
    MpvConfig {
        id: mpvConfig
        player: player
    }
    VideoOptionsController {
        id: videoOptions
        player: player
    }
    YouTubeSearchService {
        id: youtubeSearchService
    }
    SubtitleService {
        id: appSubtitleService
        player: player
        onSubtitleReady: function (path) { toastText.showMessage(qsTr("Subtitle downloaded and added")); }
    }
    RemoteControlService {
        id: remoteControlService
        player: player
        controller: controller
    }
    CastService {
        id: appCastService
    }
    AdvancedPlaybackController {
        id: advancedPlayback
        player: player
    }
    PlaybackController {
        id: controller
        player: player
        playlist: playlistModel
        settings: SettingsManager
        onUserMessage: function (msg) {
            toastText.showMessage(msg);
            window.showTextOsd(msg, false);
        }
    }

    /// Opens media files from URL strings, optionally clearing the current playlist first.
    /// @param urls Array of file URLs to open.
    /// @param clearFirst If true, clears the playlist before adding new items.
    function openUrls(urls, clearFirst) {
        var paths = fileService.urlsToPaths(urls);
        controller.openPaths(paths, clearFirst);
    }

    /// Opens media files from a folder URL, optionally clearing the current playlist first.
    /// @param folderUrl The folder URL to scan for media files.
    /// @param clearFirst If true, clears the playlist before adding new items.
    function openFolderUrl(folderUrl, clearFirst) {
        var paths = fileService.listMediaFiles(folderUrl);
        controller.openPaths(paths, clearFirst);
    }

    /// Returns true if any UI element is active that should keep the chrome visible.
    /// @returns True if chrome should remain pinned (hovered or popup/dialog open).
    function chromePinned() {
        return headerHover.hovered || controlsHover.hovered || anyPopupOpen;
    }

    /// Shows the chrome (header bar, control bar) and schedules auto-hide.
    /// @param timeout Optional auto-hide delay in milliseconds (default 2000).
    function showChrome(timeout) {
        cursorHidden = false;
        if (player.idle) {
            chromeVisible = true;
            chromeHideTimer.stop();
            return;
        }
        chromeVisible = true;
        chromeHideTimer.interval = timeout || 2000;
        chromeHideTimer.restart();
    }

    /// Schedules the chrome to hide after a given timeout, unless idle or pinned.
    /// @param timeout Optional hide delay in milliseconds (default 2000).
    function scheduleChromeHide(timeout) {
        if (player.idle) {
            return;
        }
        chromeHideTimer.interval = timeout || 2000;
        chromeHideTimer.restart();
    }

    /// Hides the chrome (and hides cursor in fullscreen) unless pinned or idle.
    function hideChrome() {
        if (player.idle) {
            chromeVisible = true;
            cursorHidden = false;
            return;
        }
        if (chromePinned()) {
            scheduleChromeHide(1200);
            return;
        }
        chromeVisible = false;
        cursorHidden = window.isFullscreen;
    }

    /// Updates the drop overlay icon and label based on the dragged URLs and current playback state.
    /// @param urls Array of dropped URL strings to evaluate.
    function updateDropAction(urls) {
        var firstPath = "";
        if (urls && urls.length > 0)
            firstPath = String(fileService.urlsToPaths([urls[0]])[0] || "");

        if (mediaActive && firstPath.length > 0 && fileService.isSubtitle(firstPath)) {
            dropActionIcon = "cine-subtitles-symbolic";
            dropActionText = qsTr("Add Subtitle Track");
            return;
        }

        dropActionIcon = "cine-playback-start-symbolic";
        dropActionText = qsTr("Play");
    }

    /// Selects or disables a subtitle track by id and updates the menu model.
    /// @param trackId Track id to select, or <= 0 to disable subtitles.
    function selectSubtitleTrack(trackId) {
        if (trackId <= 0)
            player.setTrackOff("sid");
        else
            player.setTrack("sid", trackId);
        subtitleTracks.selectedTrack = Math.max(0, trackId);
    }

    /// Selects or disables an audio track by id and updates the menu model.
    /// @param trackId Track id to select, or <= 0 to disable audio.
    function selectAudioTrack(trackId) {
        if (trackId <= 0)
            player.setTrackOff("aid");
        else
            player.setTrack("aid", trackId);
        audioTracks.selectedTrack = Math.max(0, trackId);
    }

    /// Selects or disables a video track by id and updates the menu model.
    /// @param trackId Track id to select, or <= 0 to disable video.
    function selectVideoTrack(trackId) {
        if (trackId <= 0)
            player.setTrackOff("vid");
        else
            player.setTrack("vid", trackId);
        videoTracks.selectedTrack = Math.max(0, trackId);
    }

    /// Seeks by a relative amount and shows an aggregated OSD with the total seek offset.
    /// @param seconds Number of seconds to seek (positive forward, negative backward).
    function showSeek(seconds) {
        player.seekRelative(seconds);
        if (!seekOsdResetTimer.running || accumulatedSeekOsdSeconds === 0 || (accumulatedSeekOsdSeconds > 0) !== (seconds > 0)) {
            accumulatedSeekOsdSeconds = seconds;
        } else {
            accumulatedSeekOsdSeconds += seconds;
        }
        var absSeconds = Math.abs(accumulatedSeekOsdSeconds);
        var direction = accumulatedSeekOsdSeconds >= 0 ? "+" : "-";
        showTextOsd(qsTr("Seek: %1%2s").arg(direction).arg(absSeconds), false);
        seekOsdResetTimer.restart();
    }

    /// Applies a speed multiplier and shows the new speed value in an OSD.
    /// @param multiplier Factor to multiply the current playback speed by.
    function showSpeed(multiplier) {
        var nextSpeed = Math.max(0.25, Math.min(4.0, player.speed * multiplier));
        showTextOsd(qsTr("Speed: %1x").arg(nextSpeed.toFixed(2)), false);
    }

    /// Shows the central icon-based OSD display with an icon, text, and danger state.
    /// @param iconName Name of the icon to display.
    /// @param textVal Text label for the OSD.
    /// @param isDangerVal Whether to use danger (warning) styling.
    function showOsd(iconName, textVal, isDangerVal) {
        osdDisplay.show(iconName, textVal, isDangerVal);
    }

    /// Shows a text-only OSD in the volume/text OSD area.
    /// @param textVal The text string to display.
    /// @param isDangerVal Whether to use danger (warning) styling.
    function showTextOsd(textVal, isDangerVal) {
        volumeTextOsd.showText(textVal, isDangerVal);
    }

    /// Shows icon-only feedback for a play/pause state change.
    function showPlaybackOsd(paused) {
        showOsd(paused ? "cine-playback-pause-symbolic" : "cine-playback-start-symbolic", "", false);
    }

    /// Toggles playback pause state on the player controller.
    function togglePause() {
        controller.togglePause();
    }

    /// Cycles to the next or previous selected track in a TrackModel.
    /// @param model The TrackModel to cycle through.
    /// @param forward If true, cycle forward; if false, cycle backward.
    /// @param selectFn Callback function receiving the track id to select.
    function cycleTrack(model, forward, selectFn) {
        if (!model || model.count <= 0)
            return;
        var idx = forward ? 0 : model.count - 1;
        for (var i = 0; i < model.count; ++i) {
            if (model.get(i).selected) {
                idx = forward ? (i + 1) % model.count : (i - 1 + model.count) % model.count;
                break;
            }
        }
        selectFn(model.get(idx).trackId);
    }

    /// Central dispatcher that maps a KeyBindings action id to its behaviour.
    /// @param id The action identifier string from the KeyBindings registry.
    function dispatch(id) {
        switch (id) {
        // General
        case "openFiles":
            openFilesDialog.open();
            break;
        case "newWindow":
            header.newWindowRequested();
            break;
        case "openFolder":
            openFolderDialog.open();
            break;
        case "openUrl":
            urlDialog.open();
            break;
        case "addFiles":
            addFilesDialog.open();
            break;
        case "addFolder":
            addFolderDialog.open();
            break;
        case "addUrl":
            addUrlDialog.open();
            break;
        case "playlist":
            playlistDrawer.open();
            break;
        case "preferences":
            preferencesDialog.open();
            break;
        case "shortcuts":
            shortcutsDialog.open();
            break;
        case "subtitleMenu":
            subtitleMenu.popup();
            break;
        case "audioMenu":
            audioMenu.popup();
            break;
        case "chaptersMenu":
            chaptersMenu.popup();
            break;
        case "saveSession":
            header.saveSessionRequested();
            break;
        case "closeWindow":
            window.close();
            break;
        case "quit":
            window.close();
            break;
        case "closeWindowQ":
            window.close();
            break;

        // Playback
        case "playPause":
        case "playPauseK":
        case "playPauseP":
            togglePause();
            break;
        case "speedDown":
            showSpeed(1 / 1.1);
            player.runCommandAsync(["multiply", "speed", "1/1.1"]);
            break;
        case "speedUp":
            showSpeed(1.1);
            player.runCommandAsync(["multiply", "speed", "1.1"]);
            break;
        case "speedHalf":
            showSpeed(0.5);
            player.runCommandAsync(["multiply", "speed", "0.5"]);
            break;
        case "speedDouble":
            showSpeed(2.0);
            player.runCommandAsync(["multiply", "speed", "2.0"]);
            break;
        case "speedReset":
            player.runCommandAsync(["set", "speed", "1.0"]);
            showTextOsd(qsTr("Speed: 1.00x"), false);
            break;
        case "revertSeek":
            player.runCommandAsync(["revert-seek"]);
            break;
        case "markRevertSeek":
            player.runCommandAsync(["revert-seek", "mark"]);
            break;
        case "loopFile":
            controller.toggleFileLoop();
            showTextOsd(qsTr("Loop File"), false);
            break;
        case "abLoop":
            player.runCommandAsync(["ab-loop"]);
            showTextOsd(qsTr("A-B Loop"), false);
            break;

        // Navigation
        case "seekFwd5":
            showSeek(5);
            break;
        case "seekBack5":
            showSeek(-5);
            break;
        case "seekFwd60":
            showSeek(60);
            break;
        case "seekBack60":
            showSeek(-60);
            break;
        case "seekFwd1Exact":
            player.runCommandAsync(["no-osd", "seek", "1", "exact"]);
            break;
        case "seekBack1Exact":
            player.runCommandAsync(["no-osd", "seek", "-1", "exact"]);
            break;
        case "seekFwd5Exact":
            player.runCommandAsync(["no-osd", "seek", "5", "exact"]);
            break;
        case "seekBack5Exact":
            player.runCommandAsync(["no-osd", "seek", "-5", "exact"]);
            break;
        case "seekStart":
            player.runCommandAsync(["seek", "0", "absolute"]);
            break;
        case "seekFwd600":
            showSeek(600);
            break;
        case "seekBack600":
            showSeek(-600);
            break;
        case "prevChapter":
            player.runCommandAsync(["add", "chapter", "-1"]);
            showTextOsd(qsTr("Previous Chapter"), false);
            break;
        case "nextChapter":
            player.runCommandAsync(["add", "chapter", "1"]);
            showTextOsd(qsTr("Next Chapter"), false);
            break;
        case "prevSubLine":
            player.runCommandAsync(["no-osd", "sub-seek", "-1"]);
            break;
        case "nextSubLine":
            player.runCommandAsync(["no-osd", "sub-seek", "1"]);
            break;
        case "prevSubStep":
            player.runCommandAsync(["sub-step", "-1"]);
            break;
        case "nextSubStep":
            player.runCommandAsync(["sub-step", "1"]);
            break;
        case "prevFrame":
            player.runCommandAsync(["frame-back-step"]);
            showTextOsd(qsTr("Previous Frame"), false);
            break;
        case "nextFrame":
            player.runCommandAsync(["frame-step"]);
            showTextOsd(qsTr("Next Frame"), false);
            break;
        case "playlistPrev":
            controller.playPrevious();
            break;
        case "playlistNext":
        case "playlistNextEnter":
            controller.playNext();
            break;
        case "playlistFirst":
            controller.playIndex(0);
            break;
        case "playlistLast":
            if (playlistModel.count > 0)
                controller.playIndex(playlistModel.count - 1);
            break;

        // Audio & Volume
        case "volumeUp":
        case "volumeUpStar":
            controller.nudgeVolume(2);
            break;
        case "volumeDown":
        case "volumeDownSlash":
            controller.nudgeVolume(-2);
            break;
        case "mute":
            controller.toggleMute();
            break;
        case "nextAudio":
            player.runCommandAsync(["cycle", "audio"]);
            showTextOsd(qsTr("Next Audio Track"), false);
            break;
        case "prevAudio":
            cycleTrack(audioTracks, false, selectAudioTrack);
            showTextOsd(qsTr("Previous Audio Track"), false);
            break;
        case "audioDelayDown":
            player.runCommandAsync(["add", "audio-delay", "-0.1"]);
            showTextOsd(qsTr("Audio Delay: -0.1s"), false);
            break;
        case "audioDelayUp":
            player.runCommandAsync(["add", "audio-delay", "0.1"]);
            showTextOsd(qsTr("Audio Delay: +0.1s"), false);
            break;

        // Subtitles
        case "toggleSubs":
            player.runCommandAsync(["cycle", "sub-visibility"]);
            showTextOsd(qsTr("Subtitles Toggled"), false);
            break;
        case "toggleSecondarySubs":
            player.runCommandAsync(["cycle", "secondary-sub-visibility"]);
            showTextOsd(qsTr("Secondary Subtitles Toggled"), false);
            break;
        case "assUseVideoData":
            player.runCommandAsync(["cycle", "sub-ass-use-video-data"]);
            showTextOsd(qsTr("ASS Video Data"), false);
            break;
        case "subStyleOverride":
            player.runCommandAsync(["cycle-values", "sub-ass-override", "force", "scale"]);
            showTextOsd(qsTr("Subtitle Style Override"), false);
            break;
        case "nextSub":
            player.runCommandAsync(["cycle", "sub"]);
            showTextOsd(qsTr("Next Subtitle Track"), false);
            break;
        case "prevSub":
            player.runCommandAsync(["cycle", "sub", "down"]);
            showTextOsd(qsTr("Previous Subtitle Track"), false);
            break;
        case "nextSecondarySub":
            player.runCommandAsync(["cycle", "secondary-sid"]);
            showTextOsd(qsTr("Next Secondary Subtitle"), false);
            break;
        case "prevSecondarySub":
            player.runCommandAsync(["cycle", "secondary-sid", "down"]);
            showTextOsd(qsTr("Previous Secondary Subtitle"), false);
            break;
        case "subDelayDown":
            player.runCommandAsync(["add", "sub-delay", "-0.1"]);
            showTextOsd(qsTr("Subtitle Delay: -0.1s"), false);
            break;
        case "subDelayUp":
        case "subDelayUpX":
            player.runCommandAsync(["add", "sub-delay", "0.1"]);
            showTextOsd(qsTr("Subtitle Delay: +0.1s"), false);
            break;
        case "subUp":
            player.runCommandAsync(["add", "sub-pos", "-1"]);
            showTextOsd(qsTr("Subtitle Up"), false);
            break;
        case "subDown":
        case "subDownT":
            player.runCommandAsync(["add", "sub-pos", "1"]);
            showTextOsd(qsTr("Subtitle Down"), false);
            break;
        case "subScaleDown":
            player.runCommandAsync(["add", "sub-scale", "-0.1"]);
            showTextOsd(qsTr("Subtitle Scale Down"), false);
            break;
        case "subScaleUp":
            player.runCommandAsync(["add", "sub-scale", "0.1"]);
            showTextOsd(qsTr("Subtitle Scale Up"), false);
            break;

        // Display & Video
        case "fullscreen":
            window.toggleFullscreen();
            break;
        case "zoomIn":
            player.runCommandAsync(["add", "video-zoom", "0.1"]);
            showTextOsd(qsTr("Zoom In"), false);
            break;
        case "zoomOut":
            player.runCommandAsync(["add", "video-zoom", "-0.1"]);
            showTextOsd(qsTr("Zoom Out"), false);
            break;
        case "zoomReset":
            player.runCommandAsync(["set", "video-zoom", "0"]);
            player.runCommandAsync(["no-osd", "set", "panscan", "0"]);
            player.runCommandAsync(["no-osd", "set", "video-pan-x", "0"]);
            player.runCommandAsync(["no-osd", "set", "video-pan-y", "0"]);
            player.runCommandAsync(["no-osd", "set", "video-align-x", "0"]);
            player.runCommandAsync(["no-osd", "set", "video-align-y", "0"]);
            showTextOsd(qsTr("Zoom and Pan Reset"), false);
            break;
        case "aspectRatio":
            player.runCommandAsync(["cycle-values", "video-aspect-override", "16:9", "4:3", "2.35:1", "no"]);
            showTextOsd(qsTr("Aspect Ratio"), false);
            break;
        case "deband":
            player.runCommandAsync(["cycle", "deband"]);
            showTextOsd(qsTr("Debanding"), false);
            break;
        case "deinterlace":
            player.runCommandAsync(["cycle", "deinterlace"]);
            showTextOsd(qsTr("Deinterlace"), false);
            break;
        case "panscanDown":
            player.runCommandAsync(["add", "panscan", "-0.1"]);
            showTextOsd(qsTr("Panscan Down"), false);
            break;
        case "panscanUp":
        case "panscanUpE":
            player.runCommandAsync(["add", "panscan", "0.1"]);
            showTextOsd(qsTr("Panscan Up"), false);
            break;
        case "contrastDown":
            player.runCommandAsync(["add", "contrast", "-1"]);
            showTextOsd(qsTr("Contrast Down"), false);
            break;
        case "contrastUp":
            player.runCommandAsync(["add", "contrast", "1"]);
            showTextOsd(qsTr("Contrast Up"), false);
            break;
        case "brightnessDown":
            player.runCommandAsync(["add", "brightness", "-1"]);
            showTextOsd(qsTr("Brightness Down"), false);
            break;
        case "brightnessUp":
            player.runCommandAsync(["add", "brightness", "1"]);
            showTextOsd(qsTr("Brightness Up"), false);
            break;
        case "gammaDown":
            player.runCommandAsync(["add", "gamma", "-1"]);
            showTextOsd(qsTr("Gamma Down"), false);
            break;
        case "gammaUp":
            player.runCommandAsync(["add", "gamma", "1"]);
            showTextOsd(qsTr("Gamma Up"), false);
            break;
        case "saturationDown":
            player.runCommandAsync(["add", "saturation", "-1"]);
            showTextOsd(qsTr("Saturation Down"), false);
            break;
        case "saturationUp":
            player.runCommandAsync(["add", "saturation", "1"]);
            showTextOsd(qsTr("Saturation Up"), false);
            break;

        // Miscellaneous
        case "screenshot":
            controller.screenshot();
            break;
        case "screenshotClean":
            player.runCommandAsync(["screenshot", "video"]);
            showTextOsd(qsTr("Screenshot saved (no subs)"), false);
            break;
        case "screenshotWindow":
            player.runCommandAsync(["screenshot", "window"]);
            showTextOsd(qsTr("Screenshot saved (window)"), false);
            break;
        case "screenshotEachFrame":
            player.runCommandAsync(["screenshot", "each-frame"]);
            showTextOsd(qsTr("Screenshot every frame"), false);
            break;
        case "osdLevel":
            player.runCommandAsync(["no-osd", "cycle-values", "osd-level", "3", "1"]);
            break;
        case "showProgress":
        case "showProgressP":
            player.runCommandAsync(["show-progress"]);
            break;
        case "statsToggle":
            showStatsOneshot();
            break;
        case "statsPage1":
            showStatsPage(1);
            break;
        case "statsPage2":
            showStatsPage(2);
            break;
        case "statsPage3":
            showStatsPage(3);
            break;
        case "statsPage4":
            showStatsPage(4);
            break;
        case "statsPage5":
            showStatsPage(5);
            break;
        case "statsOverlay":
            toggleStatsOverlay();
            break;
        case "keyBindingsPage":
            toggleStatsPage(4);
            break;
        case "debugConsole":
            openMpvConsole();
            break;
        }
    }

    /// Runs non-visual startup work after the first frame is on screen.
    function runDeferredStartup() {
        mediaLibrary.initialize();
        tmdbMetadata.refresh();
        sessionRestoreAttempted = true;
        if (startupPaths.length > 0)
            controller.openPaths(startupPaths, true);
        else if (SettingsManager.saveSession && sessionManager.restore(playlistModel))
            controller.playIndexAt(Math.max(0, sessionManager.restoredIndex()), sessionManager.restoredPosition());
        if (SettingsManager.autoUpdate)
            appUpdateService.checkForUpdates(Qt.application.version);
    }

    /// Applies the cheap player settings needed for the first frame.
    Component.onCompleted: {
        ensureWindowGeometryVisible();
        controller.applySettings();
        Qt.callLater(function () {
            ignoreVolumeOsd = false;
        });
    }

    onFrameSwapped: {
        if (!deferredStartupStarted) {
            deferredStartupStarted = true;
            Qt.callLater(runDeferredStartup);
        }
    }

    /// Saves window geometry, session state, and watch-later config on close.
    onClosing: {
        SettingsManager.saveWindowSize(normalWidth, normalHeight);
        if (SettingsManager.saveSession && sessionRestoreAttempted)
            sessionManager.save(playlistModel, playlistModel.currentIndex, player.position);
        mediaLibrary.flushPlaybackState();
        if (SettingsManager.saveVideoPosition)
            player.runCommandAsync(["write-watch-later-config"]);
    }

    /// Shows chrome and clears cursor hide when returning from minimized/hidden states.
    /// @param visibility The new Window visibility state.
    onVisibilityChanged: function (visibility) {
        if (visibility !== Window.Minimized && visibility !== Window.Hidden) {
            if (window.opacity < 1.0) {
                fadeAnimation.start();
            }
        }
        if (visibility !== Window.FullScreen && !window.isFullscreen)
            cursorHidden = false;
        if (!player.idle)
            showChrome(2200);
    }

    /// Hides the chrome after the auto-hide delay expires.
    Timer {
        id: chromeHideTimer
        interval: 2000
        repeat: false
        onTriggered: window.hideChrome()
    }

    /// Resets the accumulated seek OSD value after inactivity.
    Timer {
        id: seekOsdResetTimer
        interval: 750
        repeat: false
        onTriggered: window.accumulatedSeekOsdSeconds = 0
    }

    /// Hides the stats overlay after the oneshot timeout unless pinned.
    Timer {
        id: statsOneshotTimer
        interval: 4200
        repeat: false
        onTriggered: {
            if (!window.statsPinned)
                window.statsVisible = false;
        }
    }

    /// Hides the buffering spinner after a timeout if it is still visible.
    Timer {
        id: bufferingTimeout
        interval: 10000
        repeat: false
        onTriggered: {
            if (bufferingSpinner.visible)
                bufferingSpinner.visible = false;
        }
    }

    WindowFrame {
        id: rootContainer
        anchors.fill: parent
        targetWindow: window
        compact: window.compactMode === 2

        CineMpvItem {
            id: player
            anchors.fill: rootContainer
            visible: !window.mediaHubVisible
            focus: true
            Keys.onShortcutOverride: function (event) {
                if (window.consoleInputActive) {
                    event.accepted = true;
                    return;
                }
                var sequence = window.sequenceFromKeyEvent(event);
                if (sequence.length > 0 && window.isWindowShortcut(sequence))
                    event.accepted = true;
            }
            Keys.onPressed: function (event) {
                if (window.consoleInputActive) {
                    event.accepted = inputRouter.handleKey(event.key, event.modifiers, true);
                    if (event.key === Qt.Key_Escape || (event.key === Qt.Key_D && (event.modifiers & Qt.ControlModifier))) {
                        window.consoleInputActive = false;
                    }
                    return;
                }
                if (event.key === Qt.Key_F11) {
                    window.toggleFullscreen();
                    window.markAppHandledKey(event.key);
                    event.accepted = true;
                    return;
                }
                if (event.key === Qt.Key_Escape && window.statsVisible) {
                    window.dispatchStatsSequence("Esc");
                    window.markAppHandledKey(event.key);
                    event.accepted = true;
                    return;
                }
                if (event.key === Qt.Key_Escape && window.isFullscreen) {
                    window.toggleFullscreen();
                    window.markAppHandledKey(event.key);
                    event.accepted = true;
                    return;
                }
                if (event.key === Qt.Key_Space && event.modifiers === Qt.NoModifier) {
                    if (!window.dispatchKeySequence("Space"))
                        window.dispatch("playPause");
                    window.markAppHandledKey(event.key);
                    event.accepted = true;
                    return;
                }
                var sequence = window.sequenceFromKeyEvent(event);
                if (sequence.length > 0 && window.dispatchKeySequence(sequence)) {
                    window.markAppHandledKey(event.key);
                    event.accepted = true;
                    return;
                }
                event.accepted = inputRouter.handleKey(event.key, event.modifiers, true);
            }
            Keys.onReleased: function (event) {
                if (window.consoleInputActive) {
                    event.accepted = inputRouter.handleKey(event.key, event.modifiers, false);
                    return;
                }
                if (window.consumeAppHandledKey(event.key)) {
                    event.accepted = true;
                    return;
                }
                event.accepted = inputRouter.handleKey(event.key, event.modifiers, false);
            }

            onTracksChanged: function (tracks) {
                subtitleTracks.updateFromMpv(tracks);
                audioTracks.updateFromMpv(tracks);
                videoTracks.updateFromMpv(tracks);
            }
            onChaptersChanged: function (chapters) {
                chapterModel.updateFromMpv(chapters);
            }
            onFileStarted: {
                window.leaveMediaHub();
                player.setPause(false);
                window.ignoreVolumeOsd = true;
                window.pipSubtitleTrack = 0;
                subtitleTracks.clear();
                audioTracks.clear();
                videoTracks.clear();
                chapterModel.clear();
                bufferingSpinner.visible = true;
                bufferingTimeout.start();
            }
            onIdleChanged: {
                if (player.idle) {
                    if (!window.mediaHubVisible)
                        window.enterMediaHub("home");
                    window.chromeVisible = true;
                    window.cursorHidden = false;
                    chromeHideTimer.stop();
                } else {
                    window.showChrome(2600);
                }
            }
            onEndFile: function (reason) {
                bufferingSpinner.visible = false;
                if (reason === "eof" && playlistModel.currentIndex + 1 < playlistModel.count)
                    controller.playNext();
                else if (reason === "error")
                    toastText.showMessage(qsTr("Unable to play this media. Check the URL or update yt-dlp."));
            }

            onPauseChanged: {
                window.showPlaybackOsd(player.pause);
                if (window.compactMode === 2) {
                    window.pipControlsVisible = true;
                    window.schedulePipControlsHide();
                }
            }
            onMuteChanged: {
                if (!window.ignoreVolumeOsd)
                    volumeTextOsd.showVolume(player.volume, player.mute);
            }
            onVolumeChanged: {
                if (!window.ignoreVolumeOsd)
                    volumeTextOsd.showVolume(player.volume, player.mute);
            }
            onFileLoaded: {
                bufferingSpinner.visible = false;
                window.leaveMediaHub();
                controller.applySettings();
                player.forceActiveFocus();
                Qt.callLater(function () {
                    window.ignoreVolumeOsd = false;
                });
            }
        }

        // Click handler overlay on the video area
        MouseArea {
            id: videoMouseArea
            anchors.fill: player
            z: 2
            visible: window.mediaActive && !window.mediaHubVisible
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.BackButton | Qt.ForwardButton
            Accessible.role: Accessible.Slider
            Accessible.name: qsTr("Video player")
            cursorShape: window.cursorHidden ? Qt.BlankCursor : Qt.ArrowCursor

            property bool isLongPressSpeedUp: false
            property real originalSpeed: 1.0

            Timer {
                id: longPressTimer
                interval: 500
                repeat: false
                onTriggered: {
                    videoMouseArea.originalSpeed = player.speed;
                    videoMouseArea.isLongPressSpeedUp = true;
                    player.speed = 3.0;
                    window.showTextOsd(qsTr("Speed: 3.0x"), false);
                    toastText.showMessage(qsTr("3.0x Speed"));
                }
            }

            onPressed: function (mouse) {
                // Reclaim keyboard focus/activation so shortcuts (Space, arrows…) keep
                // working after interacting with controls or losing window activation.
                window.requestActivate();
                player.forceActiveFocus();
                window.showPipControls();
                window.showChrome(2200);
                if (mouse.button === Qt.LeftButton) {
                    if (SettingsManager.leftClick !== 2) {
                        longPressTimer.restart();
                    }
                }
            }

            onPositionChanged: {
                window.requestActivate();
                player.forceActiveFocus();
                window.showPipControls();
                window.showChrome(2000);
            }

            onWheel: function (wheel) {
                window.requestActivate();
                player.forceActiveFocus();
                window.showChrome(2200);

                if (wheel.angleDelta.y > 0) {
                    controller.nudgeVolume(2);
                } else if (wheel.angleDelta.y < 0) {
                    controller.nudgeVolume(-2);
                }

                if (wheel.angleDelta.x > 0) {
                    window.showSeek(10);
                } else if (wheel.angleDelta.x < 0) {
                    window.showSeek(-10);
                }

                wheel.accepted = true;
            }

            onReleased: function (mouse) {
                longPressTimer.stop();
                if (mouse.button === Qt.LeftButton) {
                    if (isLongPressSpeedUp) {
                        isLongPressSpeedUp = false;
                        player.speed = originalSpeed;
                        toastText.showMessage(qsTr("Normal Speed"));
                    } else {
                        if (SettingsManager.leftClick === 0) {
                            window.togglePause();
                        } else if (SettingsManager.leftClick === 1) {
                            window.raise();
                            window.requestActivate();
                            window.togglePause();
                        }
                    }
                } else if (mouse.button === Qt.RightButton) {
                    if (SettingsManager.rightClick === 0) {
                        window.togglePause();
                    } else if (SettingsManager.rightClick === 1) {
                        videoContextMenu.popup(videoMouseArea, mouse.x, mouse.y);
                    }
                } else if (mouse.button === Qt.BackButton) {
                    controller.playPrevious();
                } else if (mouse.button === Qt.ForwardButton) {
                    controller.playNext();
                }
            }

            onDoubleClicked: function (mouse) {
                if (mouse.button === Qt.LeftButton) {
                    longPressTimer.stop();
                    if (isLongPressSpeedUp) {
                        isLongPressSpeedUp = false;
                        player.speed = originalSpeed;
                    }
                    if (window.compactMode === 2)
                        window.exitCompactMode();
                    else
                        window.toggleFullscreen();
                }
            }
        }

        Rectangle {
            anchors.fill: parent
            color: !window.mediaActive ? Theme.background : "transparent"
            visible: !window.mediaActive
        }

        ChromeGradient {
            metrics: viewportMetrics
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            topEdge: true
            active: window.mediaActive && !window.mediaHubVisible
            shown: window.chromeVisible && !window.mediaHubVisible
        }

        ChromeGradient {
            metrics: viewportMetrics
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            topEdge: false
            active: window.mediaActive && !window.mediaHubVisible
            shown: window.chromeVisible && !window.mediaHubVisible
        }

        CineSpinner {
            id: bufferingSpinner
            metrics: viewportMetrics
            anchors.centerIn: parent
            visible: false
            z: 5
        }

        OsdDisplay {
            id: osdDisplay
            metrics: viewportMetrics
            anchors.centerIn: parent
            z: 40
            visible: !window.mediaHubVisible && (showing || opacity > 0)
        }

        VolumeOsd {
            id: volumeTextOsd
            metrics: viewportMetrics
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: Math.round(42 * viewportMetrics.visualScale)
            anchors.topMargin: Math.round(50 * viewportMetrics.visualScale)
            z: 40
            visible: !window.mediaHubVisible && (showing || opacity > 0)
        }

        MediaHub {
            id: mediaHub
            metrics: viewportMetrics
            anchors.fill: parent
            clip: true
            z: 7
            visible: window.mediaHubVisible
            fileBrowser: fileBrowserModel
            mediaLibrary: mediaLibrary
            thumbnailService: mediaThumbnailService
            controller: controller
            section: window.mediaHubSection
            playerActive: window.mediaActive
            onSectionRequested: function (section) { window.mediaHubSection = section }
            onOpenFilesRequested: openFilesDialog.open()
            onOpenFolderRequested: openFolderDialog.open()
            onOpenUrlRequested: urlDialog.open()
            onAddLibraryFolderRequested: libraryFolderDialog.open()
            onCloseRequested: if (window.mediaActive) window.leaveMediaHub()
        }

        DropArea {
            anchors.fill: parent
            onEntered: function (drop) {
                window.updateDropAction(drop.urls);
                window.dropActive = true;
            }
            onExited: window.dropActive = false
            onDropped: function (drop) {
                window.dropActive = false;
                var playable = [];
                for (var i = 0; i < drop.urls.length; ++i) {
                    var p = fileService.urlsToPaths([drop.urls[i]])[0];
                    if (window.mediaActive && fileService.isSubtitle(p))
                        player.addSubtitle(p);
                    else
                        playable.push(p);
                }
                if (playable.length > 0)
                    controller.openPaths(playable, true);
            }
        }

        DropOverlay {
            metrics: viewportMetrics
            z: 30
            anchors.fill: parent
            anchors.margins: Math.round(12 * viewportMetrics.visualScale)
            active: window.dropActive
            iconName: window.dropActionIcon
            label: window.dropActionText
        }

        HeaderBar {
            id: header
            z: 20
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            opacity: player.idle || window.chromeVisible || window.mediaHubVisible ? 1 : 0
            enabled: player.idle || window.chromeVisible || window.mediaHubVisible
            metrics: viewportMetrics
            player: player
            updateService: appUpdateService
            hubVisible: window.mediaHubVisible
            visible: window.compactMode === 0
            Behavior on opacity {
                NumberAnimation {
                    duration: Theme.motionNormal
                    easing.type: Easing.OutCubic
                }
            }
            HoverHandler {
                id: headerHover
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                onHoveredChanged: hovered ? window.showChrome(2600) : window.scheduleChromeHide(800)
            }
            onOpenFiles: openFilesDialog.open()
            onOpenFolder: openFolderDialog.open()
            onOpenUrl: urlDialog.open()
            onAddFiles: addFilesDialog.open()
            onAddFolder: addFolderDialog.open()
            onAddUrl: addUrlDialog.open()
            onAddSubtitleTrack: addSubtitleDialog.open()
            onAddAudioTrack: addAudioDialog.open()
            onSaveSessionRequested: {
                if (SettingsManager.saveSession)
                    sessionManager.save(playlistModel, playlistModel.currentIndex, player.position);
                window.close();
            }
            onShowPlaylist: playlistDrawer.open()
            onShowMediaHub: function (section) {
                window.enterMediaHub(section);
            }
            onShowPreferences: preferencesDialog.open()
            onShowAdvancedPlayback: advancedPlaybackDialog.open()
            onYoutubeSearchRequested: youtubeSearchDialog.open()
            onSubtitleSearchRequested: subtitleSearchDialog.open()
            onRemoteControlRequested: remoteControlDialog.open()
            onCastRequested: castDialog.open()
            onShowShortcuts: shortcutsDialog.open()
            onShowAbout: aboutDialog.open()
            onNewWindowRequested: SettingsManager.launchNewWindow()
            onCloseRequested: window.close()
            onToggleMaximizeRequested: window.toggleMaximize()
            onMinimizeWindowRequested: window.minimizeWindow()
        }

        ControlBar {
            id: controls
            metrics: viewportMetrics
            z: 20
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            visible: window.mediaActive && !window.mediaHubVisible && window.compactMode === 0
            opacity: window.chromeVisible ? 1 : 0
            enabled: window.chromeVisible
            player: player
            controller: controller
            subtitlesEnabled: subtitleTracks.selectedTrack > 0
            audioEnabled: audioTracks.selectedTrack > 0
            videoEnabled: videoTracks.selectedTrack > 0
            fullscreen: window.isFullscreen
            chapters: chapterModel
            Behavior on opacity {
                NumberAnimation {
                    duration: Theme.motionNormal
                    easing.type: Easing.OutCubic
                }
            }
            HoverHandler {
                id: controlsHover
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                onHoveredChanged: hovered ? window.showChrome(2600) : window.scheduleChromeHide(800)
            }
            onPlaylistRequested: playlistDrawer.open()
            onPictureInPictureRequested: window.enterCompactMode(2)
            onOptionsRequested: function (button) {
                optionsDialog.openAbove(button, window.contentItem);
            }
            onFullscreenRequested: window.toggleFullscreen()
            onSubtitlesRequested: function (button) {
                subtitleMenu.openAbove(button, window.contentItem);
            }
            onAudioRequested: function (button) {
                audioMenu.openAbove(button, window.contentItem);
            }
            onVideoRequested: function (button) {
                videoMenu.openAbove(button, window.contentItem);
            }
            onChaptersRequested: function (button) {
                chaptersMenu.openAbove(button, window.contentItem);
            }
        }

        AudioOnlyIndicator {
            metrics: viewportMetrics
            z: 6
            anchors.centerIn: parent
            active: window.mediaActive && !window.mediaHubVisible && videoTracks.selectedTrack <= 0 && audioTracks.selectedTrack > 0
            visible: !window.mediaHubVisible && (active || opacity > 0)
        }

        Toast {
            id: toastText
            metrics: viewportMetrics
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: window.compactMode === 2 ? pipSeekbar.top : controls.top
            anchors.bottomMargin: Math.round((window.compactMode === 2 ? 10 : 18)
                                             * viewportMetrics.visualScale)
        }

        LazyPopupLoader {
            id: playlistDrawer
            sourceComponent: Component {
                PlaylistDrawer {
                    playlist: playlistModel
                    controller: controller
                    fileService: fileService
                    onAddFilesRequested: addFilesDialog.open()
                    onAddFolderRequested: addFolderDialog.open()
                    onAddUrlRequested: addUrlDialog.open()
                    onSavePlaylistRequested: savePlaylistDialog.open()
                }
            }
        }

        LazyPopupLoader {
            id: urlDialog
            sourceComponent: Component {
                UrlDialog {
                    fileService: fileService
                    onAcceptedUrl: function (url) {
                        controller.openPaths([url], true);
                    }
                }
            }
        }

        LazyPopupLoader {
            id: addUrlDialog
            sourceComponent: Component {
                UrlDialog {
                    title: qsTr("Add URL")
                    fileService: fileService
                    onAcceptedUrl: function (url) {
                        controller.openPaths([url], false);
                    }
                }
            }
        }

        LazyPopupLoader {
            id: preferencesDialog
            sourceComponent: Component {
                PreferencesDialog {
                    updateService: appUpdateService
                    metadataService: tmdbMetadata
                }
            }
        }
        LazyPopupLoader {
            id: optionsDialog
            sourceComponent: Component {
                OptionsDialog {
                    options: videoOptions
                }
            }
        }
        LazyPopupLoader {
            id: aboutDialog
            sourceComponent: Component {
                AboutDialog {}
            }
        }
        LazyPopupLoader {
            id: shortcutsDialog
            sourceComponent: Component {
                ShortcutsDialog {
                    mpvConfig: mpvConfig
                }
            }
        }

        LazyPopupLoader {
            id: videoContextMenu
            sourceComponent: Component {
                CineMenu {
                    pointerVisible: false
                    CineMenuItem {
                        text: qsTr("Play / Pause")
                        onTriggered: window.togglePause()
                    }
                    CineMenuItem {
                        text: qsTr("Toggle Fullscreen")
                        onTriggered: window.toggleFullscreen()
                    }
                    MenuSeparator {}
                    CineMenuItem {
                        text: qsTr("Minimize")
                        onTriggered: window.minimizeWindow()
                    }
                    CineMenuItem {
                        text: qsTr("Maximize")
                        onTriggered: window.toggleMaximize()
                    }
                    CineMenuItem {
                        text: qsTr("Close")
                        onTriggered: window.close()
                    }
                    MenuSeparator {}
                    CineMenuItem {
                        text: qsTr("Open File...")
                        onTriggered: openFilesDialog.open()
                    }
                    CineMenuItem {
                        text: qsTr("Open Folder...")
                        onTriggered: openFolderDialog.open()
                    }
                    CineMenuItem {
                        text: qsTr("Playlist")
                        onTriggered: playlistDrawer.open()
                    }
                    MenuSeparator {}
                    CineMenuItem {
                        text: qsTr("Preferences")
                        onTriggered: preferencesDialog.open()
                    }
                }
            }
        }

        LazyPopupLoader {
            id: subtitleMenu
            sourceComponent: Component {
                TrackSelectionMenu {
                    trackModel: subtitleTracks
                    addActionText: qsTr("Add Subtitle Track")
                    onAddTrackRequested: addSubtitleDialog.open()
                    onTrackSelected: function (trackId) {
                        window.selectSubtitleTrack(trackId);
                    }
                }
            }
        }

        LazyPopupLoader {
            id: audioMenu
            sourceComponent: Component {
                TrackSelectionMenu {
                    trackModel: audioTracks
                    addActionText: qsTr("Add Audio Track")
                    onAddTrackRequested: addAudioDialog.open()
                    onTrackSelected: function (trackId) {
                        window.selectAudioTrack(trackId);
                    }
                }
            }
        }

        LazyPopupLoader {
            id: videoMenu
            sourceComponent: Component {
                TrackSelectionMenu {
                    trackModel: videoTracks
                    onTrackSelected: function (trackId) {
                        window.selectVideoTrack(trackId);
                    }
                }
            }
        }

        LazyPopupLoader {
            id: chaptersMenu
            sourceComponent: Component {
                CineMenu {
                    id: chaptersPopup
                    menuWidth: Math.min(360, Math.max(280, window.width - 48))
                    topPadding: 12
                    bottomPadding: 12
                    leftPadding: 9
                    rightPadding: 9

                    CineMenuItem {
                        visible: chapterModel.count === 0
                        enabled: false
                        text: qsTr("No chapters")
                        minItemWidth: chaptersPopup.menuWidth - chaptersPopup.leftPadding - chaptersPopup.rightPadding
                        leftPadding: 14
                        rightPadding: 14
                    }

                    Instantiator {
                        model: chapterModel
                        delegate: CineMenuItem {
                            required property int chapterIndex
                            required property string title
                            required property real time

                            text: title
                            shortcut: TimeUtils.formatTime(time)
                            implicitHeight: 40
                            minItemWidth: chaptersPopup.menuWidth - chaptersPopup.leftPadding - chaptersPopup.rightPadding
                            leftPadding: 14
                            rightPadding: 14
                            onTriggered: player.setMpvOption("chapter", chapterIndex)
                        }
                        onObjectAdded: (index, object) => chaptersPopup.insertItem(index, object)
                        onObjectRemoved: (index, object) => chaptersPopup.removeItem(object)
                    }
                }
            }
        }

        LazyPopupLoader {
            id: openFilesDialog
            sourceComponent: Component {
                FileDialog {
                    fileMode: FileDialog.OpenFiles
                    nameFilters: fileService.mediaNameFilters()
                    onAccepted: window.openUrls(selectedFiles, true)
                }
            }
        }

        LazyPopupLoader {
            id: addFilesDialog
            sourceComponent: Component {
                FileDialog {
                    title: qsTr("Add Files")
                    fileMode: FileDialog.OpenFiles
                    nameFilters: fileService.mediaNameFilters()
                    onAccepted: window.openUrls(selectedFiles, false)
                }
            }
        }

        LazyPopupLoader {
            id: addSubtitleDialog
            sourceComponent: Component {
                FileDialog {
                    fileMode: FileDialog.OpenFiles
                    nameFilters: fileService.subtitleNameFilters()
                    onAccepted: {
                        var paths = fileService.urlsToPaths(selectedFiles);
                        for (var i = 0; i < paths.length; ++i)
                            player.addSubtitle(paths[i]);
                    }
                }
            }
        }

        LazyPopupLoader {
            id: addAudioDialog
            sourceComponent: Component {
                FileDialog {
                    fileMode: FileDialog.OpenFiles
                    nameFilters: fileService.audioNameFilters()
                    onAccepted: {
                        var paths = fileService.urlsToPaths(selectedFiles);
                        for (var i = 0; i < paths.length; ++i)
                            player.addAudio(paths[i]);
                    }
                }
            }
        }

        LazyPopupLoader {
            id: openFolderDialog
            sourceComponent: Component {
                FolderDialog {
                    onAccepted: window.openFolderUrl(selectedFolder, true)
                }
            }
        }

        LazyPopupLoader {
            id: addFolderDialog
            sourceComponent: Component {
                FolderDialog {
                    title: qsTr("Add Folder")
                    onAccepted: window.openFolderUrl(selectedFolder, false)
                }
            }
        }
        LazyPopupLoader {
            id: youtubeSearchDialog
            sourceComponent: Component {
                YouTubeSearchDialog {
                    searchService: youtubeSearchService
                    onPlayRequested: function (url) { controller.openPaths([url], true); }
                }
            }
        }

        Item {
            z: 23
            anchors.fill: parent
            visible: window.compactMode > 0

            DragHandler {
                target: null
                acceptedButtons: Qt.LeftButton
                onActiveChanged: {
                    if (active)
                        window.startSystemMove();
                }
            }
        }
        LazyPopupLoader {
            id: subtitleSearchDialog
            sourceComponent: Component {
                SubtitleSearchDialog { subtitleService: appSubtitleService }
            }
        }
        LazyPopupLoader {
            id: remoteControlDialog
            sourceComponent: Component {
                RemoteControlDialog { remoteService: remoteControlService }
            }
        }
        LazyPopupLoader {
            id: castDialog
            sourceComponent: Component {
                CastDialog { castService: appCastService; mediaUrl: player.currentPath }
            }
        }

        Item {
            id: pipControls
            z: 24
            anchors.fill: parent
            visible: window.compactMode === 2 && window.mediaActive && !window.mediaHubVisible
            enabled: window.pipControlsVisible
            opacity: window.pipControlsVisible ? 1 : 0

            Behavior on opacity {
                NumberAnimation {
                    duration: Theme.motionNormal
                    easing.type: Easing.OutCubic
                }
            }

            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: viewportMetrics.titleBarHeight + viewportMetrics.spacingSm
                gradient: Gradient {
                    GradientStop { position: 0; color: "#78000000" }
                    GradientStop { position: 1; color: "#00000000" }
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: viewportMetrics.controlProminent + viewportMetrics.spacingSm
                gradient: Gradient {
                    GradientStop { position: 0; color: "#00000000" }
                    GradientStop { position: 1; color: "#70000000" }
                }
            }

            Row {
                id: pipTopLeft
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.topMargin: viewportMetrics.spacingSm
                anchors.leftMargin: viewportMetrics.spacingSm
                spacing: viewportMetrics.spacingXs

                HoverHandler {
                    id: pipTopLeftHover
                    onHoveredChanged: {
                        if (hovered)
                            window.showPipControls();
                        else
                            window.schedulePipControlsHide();
                    }
                }

                Repeater {
                    model: [
                        { icon: player.mute ? "cine-volume-mute-symbolic" : "cine-volume-max-symbolic", label: player.mute ? qsTr("Unmute") : qsTr("Mute"), active: player.mute, action: function () { controller.toggleMute(); } },
                        { icon: "cine-transparency-symbolic", label: window.pipTransparent ? qsTr("Full opacity") : qsTr("70% opacity"), active: window.pipTransparent, action: function () { window.togglePipTransparency(); } },
                        { icon: subtitleTracks.selectedTrack > 0 ? "cine-subtitles-symbolic" : "cine-subtitles-off-symbolic", label: subtitleTracks.selectedTrack > 0 ? qsTr("Hide captions") : qsTr("Show captions"), active: subtitleTracks.selectedTrack > 0, action: function () { window.togglePipCaptions(); } }
                    ]

                    delegate: CineButton {
                        required property var modelData
                        styleVariant: "icon"
                        metrics: viewportMetrics
                        playerControl: true
                        buttonSize: viewportMetrics.controlCompact
                        iconSize: viewportMetrics.iconSm
                        btnTooltip: modelData.label
                        iconName: modelData.icon
                        filledIcon: modelData.active
                        checkedBackground: "#5c000000"
                        colorVariant: modelData.active ? "accent" : "default"
                        onClicked: {
                            modelData.action();
                            window.showPipControls();
                        }
                    }
                }
            }

            Row {
                id: pipTopRight
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: viewportMetrics.spacingSm
                anchors.rightMargin: viewportMetrics.spacingSm
                spacing: viewportMetrics.spacingXs

                HoverHandler {
                    id: pipTopRightHover
                    onHoveredChanged: {
                        if (hovered)
                            window.showPipControls();
                        else
                            window.schedulePipControlsHide();
                    }
                }

                Repeater {
                    model: [
                        { icon: "cine-close-symbolic", label: qsTr("Close"), danger: true, action: function () { window.close(); } },
                        { icon: "cine-picture-in-picture-exit-symbolic", label: qsTr("Return to main window"), danger: false, action: function () { window.exitCompactMode(); } }
                    ]

                    delegate: CineButton {
                        id: pipRightButton
                        required property var modelData
                        styleVariant: "icon"
                        colorVariant: modelData.danger ? "danger" : "default"
                        metrics: viewportMetrics
                        playerControl: true
                        buttonSize: viewportMetrics.controlCompact
                        iconSize: viewportMetrics.iconSm
                        btnTooltip: modelData.label
                        iconName: modelData.icon
                        onClicked: modelData.action()
                    }
                }
            }

            CineButton {
                id: pipPlayPauseButton
                anchors.centerIn: parent
                styleVariant: "icon"
                metrics: viewportMetrics
                playerControl: true
                buttonSize: viewportMetrics.controlStandard
                iconSize: viewportMetrics.iconMd
                btnTooltip: player.pause ? qsTr("Play") : qsTr("Pause")
                iconName: player.pause ? "cine-playback-start-symbolic" : "cine-playback-pause-symbolic"
                onClicked: {
                    controller.togglePause();
                    window.showPipControls();
                }
                HoverHandler {
                    id: pipCenterHover
                    onHoveredChanged: {
                        if (hovered)
                            window.showPipControls();
                        else
                            window.schedulePipControlsHide();
                    }
                }
            }

            CineSeekbar {
                id: pipSeekbar
                metrics: viewportMetrics
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: viewportMetrics.spacingSm + viewportMetrics.spacingXs
                anchors.rightMargin: viewportMetrics.spacingSm + viewportMetrics.spacingXs
                anchors.bottomMargin: viewportMetrics.spacingXs
                height: viewportMetrics.controlCompact
                from: 0
                maximum: Math.max(1, player.duration)
                value: player.position
                player: player
                chapters: chapterModel
                trackColor: "#70ffffff"
                fillColor: Theme.iconOnDark
                handleColor: Theme.iconOnDark
                onMoved: function(position) {
                    player.position = position;
                    window.showPipControls();
                }
                onPressedChanged: {
                    window.pipControlsVisible = true;
                    if (!pressed)
                        window.schedulePipControlsHide();
                }
                HoverHandler {
                    id: pipSeekHover
                    onHoveredChanged: {
                        if (hovered)
                            window.showPipControls();
                        else
                            window.schedulePipControlsHide();
                    }
                }
            }
        }

        LazyPopupLoader {
            id: advancedPlaybackDialog
            sourceComponent: Component {
                EqualizerDialog { advanced: advancedPlayback }
            }
        }

        LazyPopupLoader {
            id: libraryFolderDialog
            sourceComponent: Component {
                FolderDialog {
                    title: qsTr("Add Library Folder")
                    onAccepted: {
                        if (!mediaLibrary.addRoot(selectedFolder))
                            toastText.showMessage(qsTr("Could not add library folder"));
                    }
                }
            }
        }

        LazyPopupLoader {
            id: savePlaylistDialog
            sourceComponent: Component {
                FileDialog {
                    title: qsTr("Save Playlist")
                    fileMode: FileDialog.SaveFile
                    nameFilters: ["Playlist files (*.m3u *.m3u8)"]
                    defaultSuffix: "m3u8"
                    onAccepted: {
                        if (playlistModel.saveM3u(selectedFile))
                            toastText.showMessage(qsTr("Playlist saved successfully"));
                        else
                            toastText.showMessage(qsTr("Failed to save playlist"));
                    }
                }
            }
        }

        // All player shortcuts are generated from the remappable KeyBindings
        // registry so the bindings, the shortcuts dialog and the dispatcher can
        // never drift apart. Player focus handles the same registry explicitly so
        // mpv input bindings cannot swallow CineWindows OSD feedback.
        Instantiator {
            model: KeyBindings.actions
            delegate: Shortcut {
                required property var modelData
                sequence: KeyBindings.sequenceFor(modelData.id)
                onActivated: window.dispatchKeySequence(KeyBindings.sequenceFor(modelData.id))
            }
        }

        // Extra dialog-open accelerators that aren't user-remappable actions.
        Shortcut {
            sequences: ["Ctrl+?", "Ctrl+Shift+/"]
            onActivated: shortcutsDialog.open()
        }
    }

    ResizeHandles {
        anchors.fill: parent
        targetWindow: window
        z: 9999
    }
}
