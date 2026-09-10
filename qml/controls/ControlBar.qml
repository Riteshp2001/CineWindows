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

Item {
    id: root
    /// Responsive metrics owned by the containing playback window
    required property ViewportMetrics metrics
    /// Reference to the active media player instance
    property var player
    /// Playback controller interface for media control commands
    property var controller
    /// Whether the volume popup is currently open
    readonly property bool volumePopupOpen: volumePopup.opened
    /// Whether to show remaining time instead of elapsed time
    property bool showRemaining: SettingsManager.showRemaining
    /// Whether subtitles are currently enabled for display
    property bool subtitlesEnabled: true
    /// Whether audio output is currently enabled
    property bool audioEnabled: true
    /// Whether video output is currently enabled
    property bool videoEnabled: true
    /// Whether the player window is in fullscreen mode
    property bool fullscreen: false
    /// Model containing chapter markers for the current media
    property var chapters
    /// Whether playlist shuffle mode is active
    property bool _shuffled: false
    /// Current loop mode: 0=none, 1=loop file, 2=loop playlist
    property int _loopState: 0
    /// Time in seconds at the current seek slider hover position
    property real hoverTime: 0
    /// Local X position of cursor on the progress slider
    property real hoverSliderX: 0
    /// Title of the chapter at the current hover position
    property string hoverChapterTitle: ""
    /// Whether the thumbnail preview popup is visible
    property bool thumbnailVisible: false
    /// Current calculated thumbnail preview width
    property real _thumbWidth: 174
    /// Current calculated thumbnail preview height
    property real _thumbHeight: 96
    /// Maximum allowed thumbnail preview width (read-only)
    readonly property int _thumbMaxWidth: 174
    /// Maximum allowed thumbnail preview height (read-only)
    readonly property int _thumbMaxHeight: 96
    /// Inset padding around the thumbnail inside its frame (read-only)
    readonly property int _thumbInset: 8
    /// Secondary actions collapse into one menu below the compact breakpoint.
    readonly property bool compactControls: state === "compact"
    /// Gap between adjacent control buttons
    readonly property int _buttonGap: metrics.spacingSm

    /// Emitted when the playlist toggle button is clicked
    signal playlistRequested
    /// Emitted when the options menu button is clicked
    /// @param button The button item that was clicked
    signal optionsRequested(var button)
    /// Emitted when the subtitles button is clicked
    /// @param button The button item that was clicked
    signal subtitlesRequested(var button)
    /// Emitted when the audio button is clicked
    /// @param button The button item that was clicked
    signal audioRequested(var button)
    /// Emitted when the video button is clicked
    /// @param button The button item that was clicked
    signal videoRequested(var button)
    /// Emitted when the chapters button is clicked
    /// @param button The button item that was clicked
    signal chaptersRequested(var button)
    /// Emitted when the fullscreen toggle button is clicked
    signal fullscreenRequested
    /// Emitted when Picture-in-Picture mode is requested
    signal pictureInPictureRequested

    height: metrics.controlCompact * 2 + metrics.spacingLg + metrics.spacingSm
    state: metrics.widthClass === ViewportMetrics.CompactWidth ? "compact" : "expanded"
    states: [
        State { name: "compact" },
        State { name: "expanded" }
    ]

    /// Persists the show-remaining preference to SettingsManager when toggled
    onShowRemainingChanged: SettingsManager.showRemaining = showRemaining

    /// Returns the formatted remaining playback time string
    /// @returns String like "-1:23:45" or "0:00" if no player
    function remainingText() {
        if (!player)
            return "0:00";
        var remaining = Math.max(0, player.duration - player.position);  // Seconds left in playback
        return "-" + TimeUtils.formatTime(remaining);
    }

    /// Returns the icon name corresponding to the current volume level
    /// @returns Icon name string for mute, low, mid, max, or overamp
    function volumeIcon() {
        if (!player || player.mute || player.volume <= 0)
            return "cine-volume-mute-symbolic";
        if (player.volume > 100)
            return "cine-volume-overamp-symbolic";
        if (player.volume < 33)
            return "cine-volume-low-symbolic";
        if (player.volume < 66)
            return "cine-volume-mid-symbolic";
        return "cine-volume-max-symbolic";
    }

    /// Toggles playlist shuffle mode on/off and notifies the controller
    function toggleShuffle() {
        _shuffled = !_shuffled;
        if (_shuffled)
            controller.shufflePlaylist();
        else
            controller.unshufflePlaylist();
    }

    /// Cycles loop mode through: none -> loop file -> loop playlist
    function cycleLoop() {
        _loopState = (_loopState + 1) % 3;
        if (_loopState === 0) {
            player.setMpvOption("loop-file", "no");
            player.setMpvOption("loop-playlist", "no");
        } else if (_loopState === 1) {
            player.setMpvOption("loop-file", "inf");
            player.setMpvOption("loop-playlist", "no");
        } else {
            player.setMpvOption("loop-file", "no");
            player.setMpvOption("loop-playlist", "inf");
        }
    }

    /// Finds the chapter title that applies at the given time position
    /// @param seconds Time in seconds to look up
    /// @returns Chapter title string, or empty string if none found
    function chapterForTime(seconds) {
        if (!chapters || chapters.count <= 0)
            return "";
        var title = "";                            // Chapter title at the given time, blank if none
        for (var i = 0; i < chapters.count; ++i) { // Chapter index iterator
            var chapter = chapters.at(i);          // Current chapter object in iteration
            if (Number(chapter.time) <= seconds)
                title = chapter.title;
            else
                break;
        }
        return title;
    }

    /// Recalculates thumbnail dimensions based on video aspect ratio
    function updateThumbnailSize() {
        var aspect = 16 / 9;                                                         // Fallback aspect ratio
        var params = player ? player.mpvOption("video-out-params") : null;            // Video output params from mpv
        if (params) {
            var videoW = Number(params["dw"] || params["w"] || params["dwidth"] || 0);   // Native video width in pixels
            var videoH = Number(params["dh"] || params["h"] || params["dheight"] || 0);  // Native video height in pixels
            if (videoW > 0 && videoH > 0)
                aspect = videoW / videoH;
        }

        var width = _thumbMaxWidth;                              // Calculated thumbnail width constrained by max
        var height = Math.round(width / aspect);                 // Calculated thumbnail height matching aspect
        if (height > _thumbMaxHeight) {
            height = _thumbMaxHeight;
            width = Math.round(height * aspect);
        }

        _thumbWidth = Math.max(48, Math.min(_thumbMaxWidth, width));
        _thumbHeight = Math.max(36, Math.min(_thumbMaxHeight, height));
    }

    /// Positions the thumbnail popup relative to the slider position
    /// @param localX Local X coordinate on the progress slider
    function placeThumbnail(localX) {
        hoverSliderX = progress.x + localX;
        var popupX = hoverSliderX - thumbnailPopup.width / 2;    // Desired X centering the popup under cursor
        thumbnailPopup.x = Math.max(0, Math.min(root.width - thumbnailPopup.width, popupX));

        var rootScenePos = root.mapToItem(null, 0, 0);           // Root position in window coordinates
        var minY = -rootScenePos.y + 8;                          // Minimum Y to keep popup inside window
        thumbnailPopup.y = Math.max(minY, progress.y - thumbnailPopup.height - 14);
    }

    /// Requests a thumbnail preview image for the given time position
    /// @param time Time in seconds to request the thumbnail at
    function requestThumbnail(time) {
        if (!player || !SettingsManager.thumbnailPreview)
            return;
        var imagePos = root.mapToItem(null, thumbnailPopup.x + _thumbInset, thumbnailPopup.y + _thumbInset); // Target in scene coords
        thumbnailer.request(player.currentPath, time, Math.round(imagePos.x), Math.round(imagePos.y), Math.round(_thumbWidth), Math.round(_thumbHeight));
    }

    /// Hides the thumbnail preview and clears the current thumbnail
    function hideThumbnailPreview() {
        thumbnailVisible = false;
        thumbnailer.clear();
    }

    Connections {
        target: SettingsManager
        /// Hides the thumbnail preview when the setting is disabled externally
        function onThumbnailPreviewChanged() {
            if (!SettingsManager.thumbnailPreview)
                root.hideThumbnailPreview();
        }
    }

    VolumePopup {
        id: volumePopup
        objectName: "volumePopup"
        parent: root
        metrics: root.metrics
        player: root.player
        controller: root.controller
    }

    CineMenu {
        id: moreMenu
        objectName: "moreMenu"
        metrics: root.metrics

        CineMenuItem {
            text: qsTr("Subtitles")
            onTriggered: root.subtitlesRequested(moreButton)
        }
        CineMenuItem {
            text: qsTr("Audio")
            onTriggered: root.audioRequested(moreButton)
        }
        CineMenuItem {
            text: qsTr("Video")
            onTriggered: root.videoRequested(moreButton)
        }
        CineMenuItem {
            text: qsTr("Chapters")
            onTriggered: root.chaptersRequested(moreButton)
        }
        CineMenuItem {
            text: qsTr("Shuffle")
            checkable: true
            checked: root._shuffled
            onTriggered: root.toggleShuffle()
        }
        CineMenuItem {
            text: qsTr("Picture in Picture")
            onTriggered: root.pictureInPictureRequested()
        }
        CineMenuItem {
            text: root._loopState === 0 ? qsTr("No Loop")
                  : root._loopState === 1 ? qsTr("Loop File")
                  : qsTr("Loop Playlist")
            checkable: true
            checked: root._loopState > 0
            onTriggered: root.cycleLoop()
        }
        CineMenuItem {
            text: qsTr("Playlist")
            onTriggered: root.playlistRequested()
        }
        CineMenuItem {
            text: qsTr("Options")
            onTriggered: root.optionsRequested(moreButton)
        }
    }

    Connections {
        target: thumbnailer
        /// Shows the thumbnail when the thumbnailer finishes loading the requested frame
        /// @param time Time in seconds of the thumbnail that finished loading
        function onThumbnailReady(time) {
            if (Math.floor(time) === Math.floor(root.hoverTime))
                root.thumbnailVisible = true;
        }
    }

    // Subtle scrim so controls stay legible over bright video.
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: Theme.controlScrimStart
            }
            GradientStop {
                position: 1.0
                color: Theme.controlScrimEnd
            }
        }
    }

    Row {
        id: buttonsFlow
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: root.metrics.spacingSm + root.metrics.spacingXs
        anchors.rightMargin: root.metrics.spacingSm + root.metrics.spacingXs
        anchors.bottom: progress.top
        anchors.bottomMargin: root.metrics.spacingSm + root.metrics.spacingXs
        spacing: 0

        Row {
            id: leftButtons
            spacing: root._buttonGap

            CineButton {
                id: previousButton
                objectName: "previousButton"
                metrics: root.metrics
                playerControl: true
                iconName: "cine-skip-backward-symbolic"
                btnText: qsTr("Previous")
                /// Seeks to the previous playlist item
                onClicked: root.controller.playPrevious()
            }

            CineButton {
                id: playPauseButton
                objectName: "playPauseButton"
                metrics: root.metrics
                playerControl: true
                iconName: root.player && !root.player.pause ? "cine-playback-pause-symbolic" : "cine-playback-start-symbolic"
                btnText: qsTr("Play/Pause")
                /// Toggles between play and pause states
                onClicked: root.controller.togglePause()
            }

            CineButton {
                id: nextButton
                objectName: "nextButton"
                metrics: root.metrics
                playerControl: true
                iconName: "cine-skip-forward-symbolic"
                btnText: qsTr("Next")
                /// Seeks to the next playlist item
                onClicked: root.controller.playNext()
            }

            CineButton {
                id: volumeButton
                objectName: "volumeButton"
                metrics: root.metrics
                playerControl: true
                iconName: root.volumeIcon()
                btnText: qsTr("Volume")
                /// Opens the volume slider popup above this button
                onClicked: volumePopup.openAbove(volumeButton, root.Window.window.contentItem)
            }

            CineButton {
                id: subtitlesButton
                objectName: "subtitlesButton"
                visible: !root.compactControls
                metrics: root.metrics
                playerControl: true
                iconName: root.subtitlesEnabled ? "cine-subtitles-symbolic" : "cine-subtitles-off-symbolic"
                btnText: qsTr("Subtitles")
                /// Emits subtitlesRequested to open the subtitles menu
                onClicked: root.subtitlesRequested(subtitlesButton)
            }

            CineButton {
                id: audioButton
                objectName: "audioButton"
                visible: !root.compactControls
                metrics: root.metrics
                playerControl: true
                iconName: root.audioEnabled ? "cine-audio-symbolic" : "cine-audio-off-symbolic"
                btnText: qsTr("Audio")
                /// Emits audioRequested to open the audio track menu
                onClicked: root.audioRequested(audioButton)
            }

            CineButton {
                id: videoButton
                objectName: "videoButton"
                visible: !root.compactControls
                metrics: root.metrics
                playerControl: true
                iconName: root.videoEnabled ? "cine-video-symbolic" : "cine-video-x-generic-symbolic"
                btnText: qsTr("Video")
                /// Emits videoRequested to open the video track menu
                onClicked: root.videoRequested(videoButton)
            }

            CineButton {
                id: chaptersButton
                objectName: "chaptersButton"
                visible: !root.compactControls
                metrics: root.metrics
                playerControl: true
                iconName: "cine-chapters-symbolic"
                btnText: qsTr("Chapters")
                /// Emits chaptersRequested to open the chapters menu
                onClicked: root.chaptersRequested(chaptersButton)
            }
        }

        Item {
            id: spacer
            width: Math.max(0, buttonsFlow.width - leftButtons.width - rightButtons.width)
            height: 1
            visible: width > 0
        }

        Row {
            id: rightButtons
            spacing: root._buttonGap

            CineButton {
                id: shuffleButton
                objectName: "shuffleButton"
                visible: !root.compactControls
                metrics: root.metrics
                playerControl: true
                iconName: "cine-playlist-shuffle-symbolic"
                btnText: qsTr("Shuffle")
                btnTooltip: qsTr("Shuffle playlist")
                checkable: true
                checked: root._shuffled
                checkedBackground: "#44000000"
                /// Toggles playlist shuffle mode on/off
                onClicked: root.toggleShuffle()
            }

            CineButton {
                id: pictureInPictureButton
                objectName: "pictureInPictureButton"
                visible: !root.compactControls
                metrics: root.metrics
                playerControl: true
                iconName: "cine-picture-in-picture-symbolic"
                btnText: qsTr("Picture in Picture")
                btnTooltip: qsTr("Picture in Picture")
                onClicked: root.pictureInPictureRequested()
            }

            CineButton {
                id: loopButton
                objectName: "loopButton"
                visible: !root.compactControls
                metrics: root.metrics
                playerControl: true
                iconName: root._loopState === 1 ? "cine-repeat-file-symbolic" : "cine-playlist-repeat-symbolic"
                btnText: root._loopState === 0 ? qsTr("No Loop") : (root._loopState === 1 ? qsTr("Loop File") : qsTr("Loop Playlist"))
                checkable: true
                checked: root._loopState > 0
                checkedBackground: "#44000000"
                /// Cycles loop mode: none -> file -> playlist
                onClicked: root.cycleLoop()
            }

            CineButton {
                id: playlistButton
                objectName: "playlistButton"
                visible: !root.compactControls
                metrics: root.metrics
                playerControl: true
                iconName: "cine-playlist-symbolic"
                btnText: qsTr("Playlist")
                /// Emits playlistRequested to toggle the playlist panel
                onClicked: root.playlistRequested()
            }

            CineButton {
                id: optionsButton
                objectName: "optionsButton"
                visible: !root.compactControls
                metrics: root.metrics
                playerControl: true
                iconName: "cine-options-symbolic"
                btnText: qsTr("Options")
                /// Emits optionsRequested to open the options menu
                onClicked: root.optionsRequested(optionsButton)
            }

            CineButton {
                id: moreButton
                objectName: "moreButton"
                visible: root.compactControls
                metrics: root.metrics
                playerControl: true
                iconName: "cine-options-symbolic"
                btnText: qsTr("More")
                onClicked: moreMenu.openAbove(moreButton, root)
            }

            CineButton {
                id: fullscreenButton
                objectName: "fullscreenButton"
                metrics: root.metrics
                playerControl: true
                iconName: root.fullscreen ? "cine-view-restore-symbolic" : "cine-view-fullscreen-symbolic"
                btnText: root.fullscreen ? qsTr("Restore") : qsTr("Fullscreen")
                /// Emits fullscreenRequested to toggle fullscreen mode
                onClicked: root.fullscreenRequested()
            }
        }
    }

    FontMetrics {
        id: timeFontMetrics
        font {
            pixelSize: root.metrics.fontCaption
            bold: true
        }
    }

    /// Computes a fixed time-label width from the current duration string.
    /// This prevents seekbar jitter as numbers tick while keeping the label
    /// compact for short videos and roomy for long ones.
    property real _timeWidth: timeFontMetrics.advanceWidth("59:59")
    /// Fixed width for the negative / remaining-time label
    property real _timeWidthRemaining: timeFontMetrics.advanceWidth("-59:59")

    function _recalcTimeWidth() {
        if (!root.player || root.player.duration <= 0) {
            _timeWidth = timeFontMetrics.advanceWidth("59:59");
            _timeWidthRemaining = timeFontMetrics.advanceWidth("-59:59");
            return;
        }
        var duration = root.player.duration;                     // Current media duration in seconds
        var fmt = duration < 3600 ? "59:59" : (duration < 36000 ? "9:59:59" : "99:59:59");  // Format string matching duration length
        _timeWidth = timeFontMetrics.advanceWidth(fmt);
        _timeWidthRemaining = timeFontMetrics.advanceWidth("-" + fmt);
    }

    Connections {
        target: root.player
        function onDurationChanged() { root._recalcTimeWidth() }
    }

    Row {
        id: times
        anchors.right: parent.right
        anchors.rightMargin: root.metrics.spacingMd + root.metrics.spacingXs
        anchors.verticalCenter: progress.verticalCenter
        spacing: root.metrics.spacingSm + root.metrics.spacingXs
        Text {
            id: positionTime
            text: root.showRemaining ? root.remainingText() : (root.player ? root.player.formattedPosition : "0:00")
            color: Theme.text
            font: timeFontMetrics.font
            style: Text.Outline
            styleColor: Theme.textOutline
            horizontalAlignment: Text.AlignRight
            width: Math.max(root._timeWidth, root._timeWidthRemaining)
            MouseArea {
                anchors.fill: parent
                /// Toggles between elapsed time and remaining time display
                onClicked: root.showRemaining = !root.showRemaining
                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Toggle remaining time")
            }
        }
        Rectangle {
            width: Math.max(1, Math.round(2 * root.metrics.visualScale))
            height: root.metrics.iconSm
            radius: 1
            color: Theme.timelineDivider
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            id: durationTime
            text: root.player ? root.player.formattedDuration : "0:00"
            color: Theme.mutedText
            font: timeFontMetrics.font
            style: Text.Outline
            styleColor: Theme.textOutline
            horizontalAlignment: Text.AlignLeft
            width: root._timeWidth
        }
    }

    Item {
        id: thumbnailPopup
        readonly property int labelGap: 6
        readonly property int labelHeight: root.hoverChapterTitle.length > 0 ? 40 : 28
        width: root._thumbWidth + root._thumbInset * 2
        height: root._thumbInset + root._thumbHeight + labelGap + labelHeight + 8
        visible: opacity > 0
        opacity: root.thumbnailVisible ? 1 : 0
        Behavior on opacity {
            NumberAnimation {
                duration: Theme.motionNormal
                easing.type: Easing.OutCubic
            }
        }

        Rectangle {
            id: thumbnailFrame
            x: root._thumbInset
            y: root._thumbInset
            width: root._thumbWidth
            height: root._thumbHeight
            radius: 8
            color: "#101014"
            border.color: Theme.popoverBorder
            border.width: 1
            clip: true

            ThumbnailController {
                id: thumbnailer
                anchors.fill: parent
                player: root.player
            }
        }

        Rectangle {
            id: thumbnailLabel
            x: root._thumbInset
            y: root._thumbInset + root._thumbHeight + thumbnailPopup.labelGap
            width: root._thumbWidth
            height: thumbnailPopup.labelHeight
            radius: 8
            color: Theme.popover
            border.color: Theme.popoverBorder
            border.width: 1

            Text {
                visible: root.hoverChapterTitle.length > 0
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                anchors.topMargin: 6
                text: root.hoverChapterTitle
                color: Theme.text
                font.pixelSize: Theme.fontSizeCaption
                font.bold: true
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                id: thumbnailText
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: root.hoverChapterTitle.length > 0 ? 6 : 7
                color: Theme.text
                text: "0:00"
                font.pixelSize: Theme.fontSizeCaption
                font.bold: true
            }
        }

        Rectangle {
            width: 12
            height: 12
            x: Math.max(root._thumbInset, Math.min(thumbnailPopup.width - width - root._thumbInset, root.hoverSliderX - thumbnailPopup.x - width / 2))
            y: thumbnailLabel.y + thumbnailLabel.height - 6
            z: -1
            rotation: 45
            radius: 1
            color: Theme.popover
        }
    }

    CineSeekbar {
        id: progress
        objectName: "progressSeekbar"
        metrics: root.metrics
        anchors.left: parent.left
        anchors.right: times.left
        anchors.leftMargin: root.metrics.spacingSm
        anchors.rightMargin: root.metrics.spacingXs
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.metrics.spacingSm
        from: 0
        maximum: root.player ? Math.max(1, root.player.duration) : 1
        value: root.player ? root.player.position : 0
        player: root.player
        chapters: root.chapters
        focusPolicy: Qt.NoFocus
        Accessible.name: qsTr("Seek")

        /// Hides the thumbnail preview when dragging starts
        onPressedChanged: if (pressed)
            root.hideThumbnailPreview()
        /// Seeks the player to the slider position when dragged
        /// @param position New seek position in seconds
        onMoved: function(position) {
            if (root.player) {
                var clamped = Math.max(0, Math.min(root.player.duration, position));
                root.player.position = clamped;
            }
        }

        HoverHandler {
            id: progressHover
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            property point currentPos: point.position
            /// Hides the thumbnail preview when hovering ends or when dragging
            onHoveredChanged: if (!hovered)
                root.hideThumbnailPreview()
            /// Updates thumbnail preview position and time as the cursor moves
            onCurrentPosChanged: {
                if (!hovered || progress.pressed)
                    return;
                if (!SettingsManager.thumbnailPreview) {
                    root.hideThumbnailPreview();
                    return;
                }
                if (!root.player || root.player.duration <= 0 || root.player.currentPath === "") {
                    root.hideThumbnailPreview();
                    return;
                }

                var hoverTime = progress.snappedPositionForPointer(currentPos.x);  // Time at the pointer snapped to seek range
                var railX = progress.railXForValue(hoverTime);                     // Pixel X on the slider rail for hoverTime
                root.hoverTime = hoverTime;
                root.hoverChapterTitle = root.chapterForTime(hoverTime);
                root.updateThumbnailSize();
                root.placeThumbnail(progress.sliderXForRail(railX));
                root.requestThumbnail(hoverTime);

                thumbnailText.text = TimeUtils.formatTime(hoverTime);
            }
        }

        /// Hides the thumbnail when the slider loses hover and no handler is active
        onHoveredChanged: {
            if (!progress._hovered && !progressHover.hovered) {
                root.hideThumbnailPreview();
            }
        }
    }
}
