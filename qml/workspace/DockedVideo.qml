pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import CineWindows

FocusScope {
    id: root
    required property string sourcePath
    property bool requestedPause: false
    property bool requestedMute: true
    property int requestedVolume: 100
    property real requestedSpeed: 1
    property real requestedPosition: 0
    property bool loading: true
    property string errorMessage: ""
    property bool closing: false
    property bool thumbnailVisible: false
    property real previewTime: 0
    property real previewCenter: 0

    width: 640
    height: 400
    focus: true

    function setPaused(paused) {
        requestedPause = paused;
        if (!paused && video.duration > 0 && video.position >= video.duration - 0.1)
            video.position = 0;
        video.pause = paused;
    }

    function setMuted(muted) {
        requestedMute = muted;
        video.mute = muted;
    }

    function openMedia() {
        if (closing)
            return;
        loading = true;
        errorMessage = "";
        video.volume = requestedVolume;
        video.mute = requestedMute;
        video.speed = requestedSpeed;
        video.pause = requestedPause;
        video.loadFile(sourcePath);
    }

    function stopPlayback() {
        closing = true;
        requestedPause = true;
        video.pause = true;
        video.mute = true;
        video.runCommandAsync(["stop"]);
    }

    Component.onCompleted: {
        settingsController.applySettings();
        Qt.callLater(openMedia);
    }

    Keys.onPressed: function (event) {
        if (event.modifiers !== Qt.NoModifier)
            return;
        if (event.key === Qt.Key_Space) {
            setPaused(!requestedPause);
        } else if (event.key === Qt.Key_Left) {
            video.seekRelative(-5);
        } else if (event.key === Qt.Key_Right) {
            video.seekRelative(5);
        } else if (event.key === Qt.Key_M) {
            setMuted(!requestedMute);
        } else {
            return;
        }
        event.accepted = true;
    }

    ViewportMetrics {
        id: paneMetrics
        viewportWidth: root.width
        viewportHeight: root.height
    }
    TrackModel { id: subtitleTracks; filterType: "sub" }
    TrackModel { id: audioTracks; filterType: "audio" }
    ChapterModel { id: chapterModel }
    PlaybackController {
        id: settingsController
        player: video
        settings: SettingsManager
    }
    MpvConfig { player: video }

    Rectangle { anchors.fill: parent; color: Theme.videoBackground }
    CineMpvItem {
        id: video
        objectName: "dockPlayer"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: controls.top
        onFileStarted: root.loading = true
        onFileLoaded: {
            if (root.closing) {
                root.stopPlayback();
                return;
            }
            volume = root.requestedVolume;
            mute = root.requestedMute;
            speed = root.requestedSpeed;
            pause = root.requestedPause;
            if (root.requestedPosition > 0 && duration > 0)
                position = Math.min(root.requestedPosition, Math.max(0, duration - 0.1));
            root.requestedPosition = 0;
            root.loading = false;
        }
        onEndFile: function (reason) {
            root.loading = false;
            if (!root.closing && reason === "error" && root.errorMessage.length === 0)
                root.errorMessage = qsTr("Unable to play this media.");
        }
        onPlaybackError: function (message) {
            if (!root.closing) {
                root.loading = false;
                root.errorMessage = message;
            }
        }
        onPauseChanged: if (!root.loading) root.requestedPause = pause
        onMuteChanged: if (!root.loading) root.requestedMute = mute
        onVolumeChanged: if (!root.loading) root.requestedVolume = volume
        onSpeedChanged: if (!root.loading) root.requestedSpeed = speed
        onTracksChanged: function (tracks) {
            subtitleTracks.updateFromMpv(tracks);
            audioTracks.updateFromMpv(tracks);
        }
        onChaptersChanged: function (entries) { chapterModel.updateFromMpv(entries); }
        onMpvPropertyChanged: function (property, value) {
            if (property === "sid")
                subtitleTracks.selectedTrack = Number(value) > 0 ? Number(value) : 0;
            else if (property === "aid")
                audioTracks.selectedTrack = Number(value) > 0 ? Number(value) : 0;
        }

        TapHandler {
            acceptedButtons: Qt.LeftButton
            onTapped: {
                root.forceActiveFocus();
                root.setPaused(!root.requestedPause);
            }
        }
    }

    BusyIndicator {
        anchors.centerIn: video
        width: 40
        height: 40
        running: root.loading && root.errorMessage.length === 0
        visible: running
    }
    Column {
        anchors.centerIn: video
        width: Math.max(0, parent.width - 32)
        spacing: 8
        visible: root.errorMessage.length > 0
        Text {
            width: parent.width
            text: root.errorMessage
            color: Theme.iconOnDark
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.fontSizeSmall
        }
        CineButton {
            anchors.horizontalCenter: parent.horizontalCenter
            styleVariant: "pill"
            colorVariant: "primary"
            btnText: qsTr("Retry")
            onClicked: root.openMedia()
        }
    }

    Rectangle {
        id: controls
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 80
        color: Theme.controlSurface

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 2
            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                CineButton {
                    playerControl: true
                    sizeRole: CineButton.StandardSize
                    iconName: root.requestedPause ? "cine-playback-start-symbolic" : "cine-playback-pause-symbolic"
                    btnTooltip: root.requestedPause ? qsTr("Play") : qsTr("Pause")
                    onClicked: root.setPaused(!root.requestedPause)
                }
                CineButton {
                    playerControl: true
                    iconName: root.requestedMute ? "cine-volume-mute-symbolic" : "cine-volume-max-symbolic"
                    btnTooltip: root.requestedMute ? qsTr("Unmute") : qsTr("Mute")
                    onClicked: root.setMuted(!root.requestedMute)
                }
                Slider {
                    Layout.preferredWidth: 80
                    Layout.maximumWidth: 100
                    visible: root.width >= 500
                    from: 0
                    to: 100
                    value: video.volume
                    palette.highlight: Theme.accent
                    palette.button: Theme.controlSurface
                    palette.buttonText: Theme.iconOnDark
                    Accessible.name: qsTr("Volume")
                    onMoved: {
                        root.requestedVolume = Math.round(value);
                        video.volume = root.requestedVolume;
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: video.formattedPosition + " / " + video.formattedDuration
                    color: Theme.iconOnDark
                    font.pixelSize: Theme.fontSizeCaption
                    elide: Text.ElideRight
                }
                CineButton {
                    id: subtitlesButton
                    playerControl: true
                    iconName: "cine-subtitles-symbolic"
                    btnTooltip: qsTr("Subtitles")
                    onClicked: subtitleMenu.openAbove(subtitlesButton, root)
                }
                CineButton {
                    id: audioButton
                    playerControl: true
                    iconName: "cine-audio-symbolic"
                    btnTooltip: qsTr("Audio Tracks")
                    onClicked: audioMenu.openAbove(audioButton, root)
                }
                CineButton {
                    id: speedButton
                    playerControl: true
                    iconName: "cine-options-symbolic"
                    btnTooltip: qsTr("Playback Speed")
                    onClicked: speedMenu.openAbove(speedButton, root)
                }
            }
            CineSeekbar {
                id: paneSeekbar
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                metrics: paneMetrics
                player: video
                chapters: chapterModel
                maximum: Math.max(1, video.duration)
                value: video.position
                trackColor: Theme.controlSeekTrack
                enabled: video.duration > 0
                onMoved: function (position) { video.position = position; }
                onPressedChanged: if (pressed) paneThumbnail.clear()
                HoverHandler {
                    id: timelineHover
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                    property point cursorPosition: point.position
                    function refreshPreview() {
                        if (!hovered || paneSeekbar.pressed || !SettingsManager.thumbnailPreview || video.duration <= 0) {
                            paneThumbnail.clear();
                            return;
                        }
                        root.previewTime = paneSeekbar.snappedPositionForPointer(cursorPosition.x);
                        root.previewCenter = paneSeekbar.mapToItem(root, cursorPosition.x, 0).x;
                        paneThumbnail.request(root.sourcePath, root.previewTime, 0, 0,
                            Math.round(preview.imageWidth), Math.round(preview.imageHeight));
                    }
                    onHoveredChanged: refreshPreview()
                    onCursorPositionChanged: refreshPreview()
                }
            }
        }
    }

    Rectangle {
        id: preview
        readonly property real aspect: video.videoAspectRatio > 0 ? video.videoAspectRatio : 16 / 9
        readonly property real imageWidth: Math.max(1, Math.min(174, 96 * aspect))
        readonly property real imageHeight: Math.max(1, Math.min(96, 174 / aspect))
        width: imageWidth + 16
        height: imageHeight + 40
        x: Math.max(8, Math.min(root.width - width - 8, root.previewCenter - width / 2))
        y: Math.max(4, controls.y - height - 8)
        radius: 8
        color: Theme.popover
        border.color: Theme.popoverBorder
        visible: root.thumbnailVisible && SettingsManager.thumbnailPreview && timelineHover.hovered && !paneSeekbar.pressed
        Rectangle {
            x: 8
            y: 8
            width: preview.imageWidth
            height: preview.imageHeight
            radius: 8
            color: Theme.videoBackground
            ThumbnailController {
                id: paneThumbnail
                anchors.fill: parent
                player: video
                onThumbnailReady: root.thumbnailVisible = timelineHover.hovered && !paneSeekbar.pressed
                onThumbnailCleared: root.thumbnailVisible = false
            }
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 7
            text: TimeUtils.formatTime(root.previewTime)
            color: Theme.text
            font.pixelSize: Theme.fontSizeCaption
        }
    }
    Connections {
        target: SettingsManager
        function onThumbnailPreviewChanged() {
            if (!SettingsManager.thumbnailPreview)
                paneThumbnail.clear();
        }
    }

    TrackSelectionMenu {
        id: subtitleMenu
        metrics: paneMetrics
        trackModel: subtitleTracks
        addActionText: qsTr("Add Subtitle Track")
        onAddTrackRequested: subtitleDialog.open()
        onTrackSelected: function (trackId) {
            if (trackId > 0)
                video.setTrack("sid", trackId);
            else
                video.setTrackDisabled("sid");
        }
    }
    TrackSelectionMenu {
        id: audioMenu
        metrics: paneMetrics
        trackModel: audioTracks
        onTrackSelected: function (trackId) {
            if (trackId > 0)
                video.setTrack("aid", trackId);
            else
                video.setTrackDisabled("aid");
        }
    }
    CineMenu {
        id: speedMenu
        metrics: paneMetrics
        Instantiator {
            model: [0.25, 0.5, 0.75, 1, 1.25, 1.5, 2]
            delegate: CineMenuItem {
                required property real modelData
                text: modelData + "x"
                checkable: true
                checked: Math.abs(video.speed - modelData) < 0.01
                onTriggered: {
                    root.requestedSpeed = modelData;
                    video.speed = modelData;
                }
            }
            onObjectAdded: function (index, object) { speedMenu.insertItem(index, object); }
            onObjectRemoved: function (index, object) { speedMenu.removeItem(object); }
        }
    }
    FileDialog {
        id: subtitleDialog
        title: qsTr("Add Subtitle Track")
        nameFilters: [qsTr("Subtitles (*.srt *.ass *.ssa *.vtt *.sub)"), qsTr("All files (*)")]
        onAccepted: video.addSubtitle(selectedFile.toString())
    }
}