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
import QtQuick.Layouts
import CineWindows

Item {
    id: root
    required property ViewportMetrics metrics

    property var fileBrowser
    property var mediaLibrary
    property var thumbnailService
    property var controller
    property string section: "home"
    property bool playerActive: false
    property string searchText: ""
    property string historySort: "newest"
    property string historyFilter: "all"
    readonly property bool compact: metrics.widthClass === ViewportMetrics.CompactWidth
    readonly property bool shortHeight: metrics.heightClass === ViewportMetrics.ShortHeight
    readonly property int sidebarWidth: compact ? Theme.sidebarCollapsedWidth : Theme.sidebarExpandedWidth
    readonly property int cardColumns: Math.max(1, Math.floor(
        (mediaGrid.width + Theme.libraryGridSpacing)
        / (Theme.mediaCardMinWidth + Theme.libraryGridSpacing)))

    signal openFilesRequested
    signal openFolderRequested
    signal openUrlRequested
    signal addLibraryFolderRequested
    signal closeRequested
    signal sectionRequested(string section)

    function clearSearch() {
        searchText = "";
        if (searchField.text.length > 0)
            searchField.clear();
        if (fileBrowser)
            fileBrowser.searchText = "";
    }

    function requestSection(requestedSection) {
        clearSearch();
        sectionRequested(requestedSection);
    }
    function collectionFor(name) {
        if (!mediaLibrary)
            return [];
        if (name === "history")
            return historyItems();
        var source = name === "favorites" ? mediaLibrary.favorites
                   : name === "library" ? mediaLibrary.library
                   : mediaLibrary.recent;
        var query = searchText.trim().toLowerCase();
        if (query.length === 0)
            return source;
        return source.filter(function (item) {
            return String(item.name || "").toLowerCase().indexOf(query) !== -1
                || String(item.path || "").toLowerCase().indexOf(query) !== -1;
        });
    }

    function historyDateKey(openedAt) {
        const date = new Date(Number(openedAt));
        const month = String(date.getMonth() + 1).padStart(2, "0");
        const day = String(date.getDate()).padStart(2, "0");
        return date.getFullYear() + "-" + month + "-" + day;
    }

    function historyDateLabel(openedAt) {
        const date = new Date(Number(openedAt));
        const today = new Date();
        const yesterday = new Date();
        yesterday.setDate(yesterday.getDate() - 1);
        if (historyDateKey(date.getTime()) === historyDateKey(today.getTime()))
            return qsTr("Today");
        if (historyDateKey(date.getTime()) === historyDateKey(yesterday.getTime()))
            return qsTr("Yesterday");
        return date.toLocaleDateString(Qt.locale(), Locale.ShortFormat);
    }

    function historyItems() {
        if (!mediaLibrary)
            return [];
        const query = searchText.trim().toLowerCase();
        return mediaLibrary.history.filter(function (item) {
            if (historyFilter === "resumable" && item.completed)
                return false;
            if (historyFilter === "completed" && !item.completed)
                return false;
            return query.length === 0
                || String(item.name || "").toLowerCase().indexOf(query) !== -1
                || String(item.path || "").toLowerCase().indexOf(query) !== -1;
        });
    }

    function historyRows() {
        const groups = {};
        for (const item of historyItems()) {
            const key = historyDateKey(item.openedAt);
            if (!groups[key])
                groups[key] = [];
            groups[key].push(item);
        }
        const keys = Object.keys(groups);
        keys.sort(function (first, second) {
            return historySort === "oldest"
                ? first.localeCompare(second)
                : second.localeCompare(first);
        });
        const chronological = function (first, second) {
            const timeDifference = Number(first.openedAt) - Number(second.openedAt);
            if (timeDifference !== 0)
                return historySort === "oldest" ? timeDifference : -timeDifference;
            const visitDifference = Number(first.visitId) - Number(second.visitId);
            return historySort === "oldest" ? visitDifference : -visitDifference;
        };
        const newestTieBreak = function (first, second) {
            const timeDifference = Number(second.openedAt) - Number(first.openedAt);
            return timeDifference !== 0
                ? timeDifference
                : Number(second.visitId) - Number(first.visitId);
        };
        const rows = [];
        for (const key of keys) {
            const items = groups[key];
            if (historySort === "title") {
                items.sort(function (first, second) {
                    const titleDifference = String(first.name || "").localeCompare(String(second.name || ""));
                    return titleDifference !== 0 ? titleDifference : newestTieBreak(first, second);
                });
            } else if (historySort === "watched") {
                items.sort(function (first, second) {
                    const watchedDifference = Number(second.watchedMs || 0) - Number(first.watchedMs || 0);
                    return watchedDifference !== 0 ? watchedDifference : newestTieBreak(first, second);
                });
            } else {
                items.sort(chronological);
            }
            rows.push({ isHeader: true, key: key, label: historyDateLabel(items[0].openedAt) });
            for (const item of items)
                rows.push({ isHeader: false, key: key, item: item });
        }
        return rows;
    }

    function historySortLabel() {
        if (historySort === "oldest")
            return qsTr("Oldest");
        if (historySort === "title")
            return qsTr("Title");
        if (historySort === "watched")
            return qsTr("Watched duration");
        return qsTr("Newest");
    }

    function historyFilterLabel() {
        if (historyFilter === "resumable")
            return qsTr("Resumable");
        if (historyFilter === "completed")
            return qsTr("Completed");
        return qsTr("All");
    }

    function historyWatchedText(watchedMs) {
        const seconds = Math.max(0, Math.round(Number(watchedMs || 0) / 1000));
        if (seconds < 60)
            return qsTr("%1 sec watched").arg(seconds);
        if (seconds < 3600)
            return qsTr("%1 min watched").arg(Math.round(seconds / 60));
        const hours = seconds / 3600;
        return qsTr("%1 h watched").arg(hours >= 10 ? Math.round(hours) : hours.toFixed(1));
    }

    function historyVisitText(visitCount) {
        const count = Number(visitCount || 0);
        return count === 1 ? qsTr("1 visit") : qsTr("%1 visits").arg(count);
    }

    function historyEndReason(reason) {
        if (reason === "eof")
            return qsTr("Finished");
        if (reason === "stopped")
            return qsTr("Stopped");
        if (reason === "replaced")
            return qsTr("Opened another item");
        if (reason === "quit")
            return qsTr("Closed player");
        return qsTr("Playback ended");
    }

    function openMedia(path, position) {
        controller.openPathAt(path, position || 0, true);
    }

    function clearCurrentCollection() {
        if (!mediaLibrary)
            return;
        if (section === "history")
            mediaLibrary.clearHistory();
        else if (section === "recent")
            mediaLibrary.clearRecent();
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.background
    }

    Rectangle {
        id: sidebar
        objectName: "mediaHubSidebar"
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: root.sidebarWidth
        color: Theme.panel

        Behavior on width {
            NumberAnimation { duration: Theme.motionPanel; easing.type: Easing.OutCubic }
        }

        ScrollView {
            id: sidebarScroll
            anchors.fill: parent
            anchors.topMargin: root.metrics.titleBarHeight + root.metrics.spacingMd
            anchors.bottomMargin: 16
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            clip: true
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            Column {
                width: sidebarScroll.availableWidth
                spacing: 4

            Repeater {
                model: [
                    { key: "home", label: qsTr("Home"), icon: "cine-applications-multimedia-symbolic" },
                    { key: "browse", label: qsTr("Browse"), icon: "cine-folder-symbolic" },
                    { key: "library", label: qsTr("Library"), icon: "cine-playlist-symbolic" },
                    { key: "favorites", label: qsTr("Favorites"), icon: "cine-heart-symbolic" },
                    { key: "recent", label: qsTr("Recent"), icon: "cine-history-symbolic" },
                    { key: "history", label: qsTr("History"), icon: "cine-view-list-symbolic" }
                ]

                delegate: AbstractButton {
                    id: navButton
                    required property var modelData
                    width: parent.width
                    height: 44
                    hoverEnabled: true
                    focusPolicy: Qt.TabFocus
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                    Accessible.name: modelData.label
                    Accessible.role: Accessible.Button
                    onClicked: root.requestSection(modelData.key)
                    background: Rectangle {
                        radius: Theme.radius
                        color: root.section === navButton.modelData.key
                               ? Qt.alpha(Theme.accent, 0.16)
                               : navButton.hovered ? Theme.cardHover : "transparent"
                        border.width: navButton.visualFocus ? Theme.focusRingWidth : 0
                        border.color: Theme.focusRing
                        Rectangle {
                            visible: root.section === navButton.modelData.key
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            width: 3
                            height: 22
                            radius: 2
                            color: Theme.accent
                        }
                    }
                    contentItem: Row {
                        anchors.fill: parent
                        leftPadding: 13
                        spacing: 12
                        CineIcon {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 18
                            height: 18
                            name: navButton.modelData.icon
                            filled: navButton.hovered || navButton.down || navButton.activeFocus || root.section === navButton.modelData.key
                            tint: root.section === navButton.modelData.key ? Theme.accent : Theme.mutedText
                        }
                        Text {
                            visible: !root.compact
                            anchors.verticalCenter: parent.verticalCenter
                            text: navButton.modelData.label
                            color: Theme.text
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: root.section === navButton.modelData.key
                        }
                    }
                    CineTooltip {
                        parent: root
                        x: sidebar.width + 8
                        y: navButton.mapToItem(root, 0, 0).y + Math.round((navButton.height - height) / 2)
                        text: navButton.modelData.label
                        active: root.compact && navButton.hovered
                    }
                }
            }

            Item { visible: !root.compact; width: 1; height: 12 }
            Rectangle { visible: !root.compact; width: parent.width; height: 1; color: Theme.separator }
            Item { visible: !root.compact; width: 1; height: 8 }

            Text {
                visible: !root.compact
                leftPadding: 12
                text: qsTr("PLACES")
                color: Theme.mutedText
                font.pixelSize: Theme.fontSizeTiny
                font.bold: true
            }

            Repeater {
                model: root.fileBrowser ? root.fileBrowser.places() : []
                delegate: AbstractButton {
                    id: placeButton
                    required property var modelData
                    visible: !root.compact
                    width: parent.width
                    height: visible ? 38 : 0
                    hoverEnabled: true
                    focusPolicy: Qt.TabFocus
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                    Accessible.name: modelData.name
                    onClicked: {
                        root.requestSection("browse");
                        root.fileBrowser.navigateTo(modelData.url);
                    }
                    background: Rectangle {
                        radius: Theme.radius
                        color: placeButton.hovered ? Theme.cardHover : "transparent"
                    }
                    contentItem: Row {
                        leftPadding: 13
                        spacing: 12
                        CineIcon {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 17
                            height: 17
                            name: "cine-folder-symbolic"
                            filled: placeButton.hovered || placeButton.down || placeButton.activeFocus
                            tint: Theme.mutedText
                        }
                        Text {
                            visible: !root.compact
                            anchors.verticalCenter: parent.verticalCenter
                            text: placeButton.modelData.name
                            color: Theme.text
                            font.pixelSize: Theme.fontSizeCaption
                            elide: Text.ElideRight
                            width: 160
                        }
                    }
                }
            }

            ItemDelegate {
                id: addLibraryButton
                objectName: "addLibraryFolderRow"
                visible: !root.compact
                width: parent.width
                height: 42
                hoverEnabled: true
                focusPolicy: Qt.TabFocus
                HoverHandler { cursorShape: Qt.PointingHandCursor }
                Accessible.name: qsTr("Add Library Folder")
                onClicked: root.addLibraryFolderRequested()
                background: Rectangle {
                    objectName: "addLibraryFolderBackground"
                    radius: Theme.radius
                    color: addLibraryButton.hovered ? Theme.cardHover : Theme.cardStrong
                    Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                }
                contentItem: RowLayout {
                    spacing: 12
                    CineIcon {
                        Layout.preferredWidth: 18
                        Layout.preferredHeight: 18
                        name: "cine-list-add-symbolic"
                        filled: addLibraryButton.hovered || addLibraryButton.down || addLibraryButton.activeFocus
                        tint: Theme.accent
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Add Library Folder")
                        color: Theme.text
                        font.pixelSize: Theme.fontSizeCaption
                        font.bold: true
                    }
                }
            }
            }
        }
    }

    Item {
        id: content
        anchors.top: parent.top
        anchors.topMargin: root.metrics.titleBarHeight
        anchors.bottom: parent.bottom
        anchors.left: sidebar.right
        anchors.right: parent.right

        Item {
            id: toolbar
            objectName: "mediaHubToolbar"
            readonly property bool hasSearch: root.section !== "home"
            readonly property bool narrow: width < 620
                                                   || (root.section === "history" && width < 900)
            readonly property bool stacked: root.compact && hasSearch
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: stacked ? 104 : 58

            Row {
                id: toolbarLeadingActions
                objectName: "toolbarLeadingActions"
                x: root.compact ? root.metrics.spacingSm : Theme.libraryContentMargin
                y: toolbar.stacked ? root.metrics.spacingSm
                                   : Math.round((toolbar.height - height) / 2)
                spacing: 12

                Text {
                    objectName: "mediaHubSectionTitle"
                    anchors.verticalCenter: parent.verticalCenter
                    visible: root.section !== "browse"
                    text: root.section === "home" ? qsTr("Media Hub")
                        : root.section === "library" ? qsTr("Media Library")
                        : root.section === "favorites" ? qsTr("Favorites")
                        : root.section === "history" ? qsTr("History") : qsTr("Recent Files")
                    color: Theme.text
                    font.pixelSize: root.metrics.fontHeading
                    font.bold: true
                }

                Rectangle {
                    visible: (root.section === "history" || root.section === "recent") && !toolbar.narrow
                    width: 1
                    height: 18
                    color: Theme.separator
                    anchors.verticalCenter: parent.verticalCenter
                }

                CineButton {
                    objectName: "browseBackButton"
                    visible: root.section === "browse"
                    width: 36; height: 36; buttonSize: 36
                    metrics: root.metrics
                    iconName: "cine-go-back-symbolic"
                    btnTooltip: qsTr("Back")
                    enabled: root.fileBrowser && root.fileBrowser.canGoBack
                    focusPolicy: Qt.TabFocus
                    onClicked: root.fileBrowser.navigateBack()
                }

                CineButton {
                    objectName: "browseForwardButton"
                    visible: root.section === "browse" && !root.compact
                    width: 36; height: 36; buttonSize: 36
                    metrics: root.metrics
                    iconName: "cine-go-forward-symbolic"
                    btnTooltip: qsTr("Forward")
                    enabled: root.fileBrowser && root.fileBrowser.canGoForward
                    focusPolicy: Qt.TabFocus
                    onClicked: root.fileBrowser.navigateForward()
                }

                CineButton {
                    objectName: "browseUpButton"
                    visible: root.section === "browse" && !root.compact
                    width: 36; height: 36; buttonSize: 36
                    metrics: root.metrics
                    iconName: "cine-go-up-symbolic"
                    btnTooltip: qsTr("Up")
                    enabled: root.fileBrowser && root.fileBrowser.canGoUp
                    focusPolicy: Qt.TabFocus
                    onClicked: root.fileBrowser.navigateUp()
                }

                CineButton {
                    objectName: "browseRefreshButton"
                    visible: root.section === "browse" && !toolbar.narrow
                    width: 36; height: 36; buttonSize: 36
                    metrics: root.metrics
                    iconName: "cine-refresh-symbolic"
                    btnTooltip: qsTr("Refresh")
                    enabled: root.fileBrowser && !root.fileBrowser.loading
                    focusPolicy: Qt.TabFocus
                    onClicked: root.fileBrowser.refresh()
                }

                CineButton {
                    id: clearCollectionButton
                    objectName: "clearCollectionButton"
                    visible: (root.section === "history" || root.section === "recent")
                             && !toolbar.narrow
                    metrics: root.metrics
                    styleVariant: "text"
                    btnText: root.section === "history" ? qsTr("Clear History") : qsTr("Clear Recent")
                    focusPolicy: Qt.TabFocus
                    onClicked: root.clearCurrentCollection()
                }

                Rectangle {
                    visible: historySortButton.visible && clearCollectionButton.visible
                    width: 1
                    height: 14
                    color: Theme.separator
                    anchors.verticalCenter: parent.verticalCenter
                }

                CineButton {
                    id: historySortButton
                    objectName: "historySortButton"
                    visible: root.section === "history" && !toolbar.narrow
                    width: 144
                    metrics: root.metrics
                    styleVariant: "text"
                    btnText: qsTr("Sort: %1").arg(root.historySortLabel())
                    focusPolicy: Qt.TabFocus
                    onClicked: historySortMenu.openBelow(historySortButton, root)
                }

                Rectangle {
                    visible: historyFilterButton.visible
                    width: 1
                    height: 14
                    color: Theme.separator
                    anchors.verticalCenter: parent.verticalCenter
                }

                CineButton {
                    id: historyFilterButton
                    objectName: "historyFilterButton"
                    visible: root.section === "history" && !toolbar.narrow
                    width: 124
                    metrics: root.metrics
                    styleVariant: "text"
                    btnText: qsTr("Filter: %1").arg(root.historyFilterLabel())
                    focusPolicy: Qt.TabFocus
                    onClicked: historyFilterMenu.openBelow(historyFilterButton, root)
                }

                CineButton {
                    id: toolbarMoreButton
                    objectName: "toolbarMoreButton"
                    visible: (root.section === "browse" && (root.compact || toolbar.narrow))
                             || ((root.section === "history" || root.section === "recent")
                                 && toolbar.narrow)
                    width: 36; height: 36; buttonSize: 36
                    metrics: root.metrics
                    glyph: "..."
                    btnTooltip: qsTr("More")
                    focusPolicy: Qt.TabFocus
                    onClicked: toolbarMoreMenu.openBelow(toolbarMoreButton, root)
                }
            }

            TextField {
                id: searchField
                objectName: "mediaHubSearch"
                readonly property real trailingEdge: closeHubButton.visible
                    ? closeHubButton.x - 8
                    : toolbar.width - (root.compact ? root.metrics.spacingSm : Theme.libraryContentMargin)
                x: toolbar.stacked ? root.metrics.spacingSm : trailingEdge - width
                y: toolbar.stacked ? toolbar.height - height - root.metrics.spacingSm
                                   : Math.round((toolbar.height - height) / 2)
                width: toolbar.stacked
                    ? Math.max(0, toolbar.width - 2 * root.metrics.spacingSm)
                    : Math.min(300, Math.max(170, toolbar.width * 0.32))
                height: 36
                visible: toolbar.hasSearch
                placeholderText: root.section === "browse" ? qsTr("Search this folder") : qsTr("Search media")
                color: Theme.text
                placeholderTextColor: Theme.mutedText
                Accessible.name: placeholderText
                onTextChanged: {
                    root.searchText = text;
                    if (root.section === "browse" && root.fileBrowser)
                        root.fileBrowser.searchText = text;
                }
                background: Rectangle {
                    radius: 18
                    color: Theme.cardStrong
                    border.color: searchField.activeFocus ? Theme.accent : Theme.separator
                }
            }

            CineMenu {
                id: toolbarMoreMenu
                objectName: "toolbarMoreMenu"
                metrics: root.metrics

                CineMenuItem {
                    objectName: "forwardMoreAction"
                    visible: root.section === "browse" && root.compact
                    text: qsTr("Forward")
                    enabled: root.fileBrowser && root.fileBrowser.canGoForward
                    onTriggered: root.fileBrowser.navigateForward()
                }
                CineMenuItem {
                    objectName: "upMoreAction"
                    visible: root.section === "browse" && root.compact
                    text: qsTr("Up")
                    enabled: root.fileBrowser && root.fileBrowser.canGoUp
                    onTriggered: root.fileBrowser.navigateUp()
                }
                CineMenuItem {
                    objectName: "refreshMoreAction"
                    visible: root.section === "browse" && (root.compact || toolbar.narrow)
                    text: qsTr("Refresh")
                    enabled: root.fileBrowser && !root.fileBrowser.loading
                    onTriggered: root.fileBrowser.refresh()
                }
                CineMenuItem {
                    objectName: "clearCollectionMoreAction"
                    visible: (root.section === "history" || root.section === "recent")
                             && toolbar.narrow
                    text: root.section === "history" ? qsTr("Clear History") : qsTr("Clear Recent")
                    onTriggered: root.clearCurrentCollection()
                }
                CineMenuItem {
                    objectName: "historySortNewestAction"
                    visible: root.section === "history" && toolbar.narrow
                    text: qsTr("Sort: Newest")
                    checkable: true
                    checked: root.historySort === "newest"
                    onTriggered: root.historySort = "newest"
                }
                CineMenuItem {
                    objectName: "historySortOldestAction"
                    visible: root.section === "history" && toolbar.narrow
                    text: qsTr("Sort: Oldest")
                    checkable: true
                    checked: root.historySort === "oldest"
                    onTriggered: root.historySort = "oldest"
                }
                CineMenuItem {
                    objectName: "historySortTitleAction"
                    visible: root.section === "history" && toolbar.narrow
                    text: qsTr("Sort: Title")
                    checkable: true
                    checked: root.historySort === "title"
                    onTriggered: root.historySort = "title"
                }
                CineMenuItem {
                    objectName: "historySortWatchedAction"
                    visible: root.section === "history" && toolbar.narrow
                    text: qsTr("Sort: Watched duration")
                    checkable: true
                    checked: root.historySort === "watched"
                    onTriggered: root.historySort = "watched"
                }
                CineMenuItem {
                    objectName: "historyFilterAllAction"
                    visible: root.section === "history" && toolbar.narrow
                    text: qsTr("Filter: All")
                    checkable: true
                    checked: root.historyFilter === "all"
                    onTriggered: root.historyFilter = "all"
                }
                CineMenuItem {
                    objectName: "historyFilterResumableAction"
                    visible: root.section === "history" && toolbar.narrow
                    text: qsTr("Filter: Resumable")
                    checkable: true
                    checked: root.historyFilter === "resumable"
                    onTriggered: root.historyFilter = "resumable"
                }
                CineMenuItem {
                    objectName: "historyFilterCompletedAction"
                    visible: root.section === "history" && toolbar.narrow
                    text: qsTr("Filter: Completed")
                    checkable: true
                    checked: root.historyFilter === "completed"
                    onTriggered: root.historyFilter = "completed"
                }
            }

            CineMenu {
                id: historySortMenu
                metrics: root.metrics

                CineMenuItem { text: qsTr("Newest"); checkable: true; checked: root.historySort === "newest"; onTriggered: root.historySort = "newest" }
                CineMenuItem { text: qsTr("Oldest"); checkable: true; checked: root.historySort === "oldest"; onTriggered: root.historySort = "oldest" }
                CineMenuItem { text: qsTr("Title"); checkable: true; checked: root.historySort === "title"; onTriggered: root.historySort = "title" }
                CineMenuItem { text: qsTr("Watched duration"); checkable: true; checked: root.historySort === "watched"; onTriggered: root.historySort = "watched" }
            }

            CineMenu {
                id: historyFilterMenu
                metrics: root.metrics

                CineMenuItem { text: qsTr("All"); checkable: true; checked: root.historyFilter === "all"; onTriggered: root.historyFilter = "all" }
                CineMenuItem { text: qsTr("Resumable"); checkable: true; checked: root.historyFilter === "resumable"; onTriggered: root.historyFilter = "resumable" }
                CineMenuItem { text: qsTr("Completed"); checkable: true; checked: root.historyFilter === "completed"; onTriggered: root.historyFilter = "completed" }
            }

            CineButton {
                id: closeHubButton
                objectName: "returnToPlayerButton"
                x: parent.width - width - (root.compact ? root.metrics.spacingSm : 16)
                y: toolbar.stacked ? root.metrics.spacingSm
                                   : Math.round((parent.height - height) / 2)
                width: 36; height: 36; buttonSize: 36
                metrics: root.metrics
                visible: root.playerActive && root.section !== "browse" && root.section !== "history"
                iconName: "cine-playback-start-symbolic"
                btnTooltip: qsTr("Return to player")
                onClicked: root.closeRequested()
            }
        }

        Rectangle {
            anchors.top: toolbar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: Theme.separator
        }

        Item {
            id: contentBody
            anchors.top: toolbar.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: root.compact ? root.metrics.spacingSm : Theme.libraryContentMargin

            Flickable {
                id: homeFlickable
                objectName: "homeFlickable"
                anchors.fill: parent
                visible: root.section === "home"
                enabled: visible
                clip: true
                contentWidth: width
                contentHeight: homeContent.height
                boundsBehavior: Flickable.StopAtBounds
                interactive: contentHeight > height
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                Item {
                    id: homeContent
                    width: homeFlickable.width
                    height: Math.max(homeFlickable.height,
                                     homeColumn.height + 2 * root.metrics.spacingLg)

                    Column {
                        id: homeColumn
                        x: Math.round((parent.width - width) / 2)
                        y: root.shortHeight
                            ? root.metrics.spacingLg
                            : Math.max(root.metrics.spacingLg,
                                       Math.round((parent.height - height) / 2))
                        width: Math.min(620, parent.width)
                        height: implicitHeight
                        spacing: root.metrics.spacingMd

                        Image {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: root.metrics.iconDisplay
                            height: root.metrics.iconDisplay
                            source: "qrc:/cinewindows/icons/apps/CineWindows.svg"
                        }
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: parent.width
                            text: qsTr("Your cinema, organized")
                            color: Theme.text
                            font.pixelSize: root.metrics.fontDisplay
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                        }
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: parent.width
                            text: qsTr("Browse files, continue watching, or build a local media library.")
                            color: Theme.mutedText
                            font.pixelSize: root.metrics.fontCaption
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                        }
                        Flow {
                            id: homeActions
                            objectName: "homeActions"
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: root.compact
                                ? parent.width
                                : Math.min(parent.width,
                                           openFilesButton.implicitWidth
                                           + openFolderButton.implicitWidth
                                           + openUrlButton.implicitWidth
                                           + 2 * spacing)
                            height: childrenRect.height
                            spacing: root.metrics.spacingSm

                            CineButton {
                                id: openFilesButton
                                metrics: root.metrics
                                styleVariant: "pill"; colorVariant: "primary"
                                btnText: qsTr("Open Files")
                                onClicked: root.openFilesRequested()
                            }
                            CineButton {
                                id: openFolderButton
                                metrics: root.metrics
                                styleVariant: "pill"; colorVariant: "default"
                                btnText: qsTr("Open Folder")
                                onClicked: root.openFolderRequested()
                            }
                            CineButton {
                                id: openUrlButton
                                metrics: root.metrics
                                styleVariant: "pill"; colorVariant: "default"
                                btnText: qsTr("Open URL")
                                onClicked: root.openUrlRequested()
                            }
                        }
                        GridLayout {
                            visible: root.mediaLibrary && Number(root.mediaLibrary.statistics.playCount || 0) > 0
                            width: parent.width
                            columns: root.compact ? 2 : 4
                            columnSpacing: root.metrics.spacingSm
                            rowSpacing: root.metrics.spacingXs
                            Repeater {
                                model: root.mediaLibrary ? [
                                    qsTr("%1 titles played").arg(root.mediaLibrary.statistics.uniqueMedia || 0),
                                    qsTr("%1 sessions").arg(root.mediaLibrary.statistics.playCount || 0),
                                    qsTr("%1 h watched").arg(Math.round(Number(root.mediaLibrary.statistics.watchedMs || 0) / 3600000)),
                                    qsTr("%1 completed").arg(root.mediaLibrary.statistics.completions || 0)
                                ] : []
                                delegate: Text {
                                    required property string modelData
                                    Layout.fillWidth: true
                                    text: modelData
                                    color: Theme.mutedText
                                    font.pixelSize: Theme.fontSizeCaption
                                    horizontalAlignment: Text.AlignHCenter
                                    elide: Text.ElideRight
                                }
                            }
                        }
                        CineButton {
                            visible: root.mediaLibrary && root.mediaLibrary.recent.length > 0
                            anchors.horizontalCenter: parent.horizontalCenter
                            metrics: root.metrics
                            styleVariant: "pill"; colorVariant: "accentAction"
                            width: Math.min(parent.width, Math.min(380, implicitWidth))
                            btnText: qsTr("Continue %1").arg(root.mediaLibrary && root.mediaLibrary.recent.length > 0 ? root.mediaLibrary.recent[0].name : "")
                            onClicked: {
                                var item = root.mediaLibrary.recent[0];
                                root.openMedia(item.path, item.position);
                            }
                        }
                    }
                }
            }

            Column {
                anchors.fill: parent
                spacing: 10
                visible: root.section === "browse"
                enabled: visible
                Text {
                    width: parent.width
                    text: root.fileBrowser ? root.fileBrowser.displayPath : ""
                    color: Theme.mutedText
                    font.pixelSize: Theme.fontSizeCaption
                    elide: Text.ElideMiddle
                }
                ListView {
                    width: parent.width
                    height: parent.height - 30
                    clip: true
                    spacing: 4
                    model: root.fileBrowser
                    boundsBehavior: Flickable.StopAtBounds
                    delegate: ItemDelegate {
                        id: fileRow
                        objectName: "browseRow"
                        required property string name
                        required property string path
                        required property url url
                        required property bool isDirectory
                        required property bool isMedia
                        required property double size
                        required property double modified
                        required property string iconName
                        property url thumbnailUrl: ""
                        readonly property bool videoFile: !isDirectory
                            && iconName === "cine-video-x-generic-symbolic"
                        HoverHandler { cursorShape: Qt.PointingHandCursor }
                        width: ListView.view.width
                        height: 48
                        focusPolicy: Qt.TabFocus
                        Accessible.name: name
                        Accessible.description: isDirectory ? qsTr("Folder") : qsTr("Media file")
                        onClicked: isDirectory ? root.fileBrowser.navigateTo(url) : root.openMedia(path, 0)
                        function updateThumbnail() {
                            thumbnailUrl = videoFile && root.thumbnailService
                                ? root.thumbnailService.thumbnailFor(path) : "";
                        }
                        Component.onCompleted: updateThumbnail()
                        onPathChanged: Qt.callLater(updateThumbnail)
                        Connections {
                            target: root.thumbnailService || null
                            ignoreUnknownSignals: true
                            function onThumbnailReady(path, url) {
                                if (path === fileRow.path)
                                    fileRow.thumbnailUrl = url;
                            }
                        }
                        background: Rectangle {
                            objectName: "browseRowBackground"
                            radius: Theme.radius
                            color: fileRow.highlighted || fileRow.hovered ? Theme.cardHover : Theme.cardStrong
                            Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                        }
                        contentItem: RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 10
                            spacing: 12
                            Item {
                                Layout.preferredWidth: fileRow.videoFile ? 52 : 28
                                Layout.preferredHeight: 36
                                Layout.alignment: Qt.AlignVCenter

                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 52
                                    height: 34
                                    radius: 6
                                    color: Theme.panelStrong
                                    visible: fileRow.videoFile
                                    clip: true

                                    Image {
                                        id: browseThumbnail
                                        objectName: "browseThumbnail"
                                        anchors.fill: parent
                                        source: fileRow.thumbnailUrl
                                        visible: status === Image.Ready
                                        fillMode: Image.PreserveAspectCrop
                                        asynchronous: true
                                        cache: true
                                    }
                                }
                                CineIcon {
                                    objectName: "browseThumbnailFallback"
                                    anchors.centerIn: parent
                                    width: 20
                                    height: 20
                                    name: fileRow.iconName
                                    visible: !fileRow.videoFile || browseThumbnail.status !== Image.Ready
                                    filled: fileRow.hovered || fileRow.down
                                    tint: fileRow.isDirectory ? Theme.accent : Theme.mutedText
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 36
                                Layout.alignment: Qt.AlignVCenter
                                text: fileRow.name
                                color: Theme.text
                                font.pixelSize: Theme.fontSizeSmall
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.alignment: Qt.AlignVCenter
                                visible: !root.compact && !fileRow.isDirectory
                                text: fileRow.size > 0 ? (fileRow.size / 1048576).toFixed(1) + " MB" : ""
                                color: Theme.mutedText
                                font.pixelSize: Theme.fontSizeCaption
                            }
                            CineButton {
                                id: browseFavoriteButton
                                objectName: "browseFavoriteButton"
                                Layout.preferredWidth: 36; Layout.preferredHeight: 36
                                Layout.alignment: Qt.AlignVCenter
                                buttonSize: 36
                                iconName: "cine-heart-symbolic"
                                checkable: true
                                checked: root.mediaLibrary && root.mediaLibrary.isFavorite(fileRow.path)
                                checkedBackground: "transparent"
                                checkedIconTint: Theme.accent
                                btnTooltip: qsTr("Toggle favorite")
                                onClicked: root.mediaLibrary.toggleFavorite(fileRow.path, fileRow.isDirectory ? "folder" : "media")
                                contentItem: Item {
                                    CineIcon {
                                        objectName: "favoriteHeartIcon"
                                        anchors.centerIn: parent
                                        width: 18
                                        height: 18
                                        name: "cine-heart-symbolic"
                                        filled: browseFavoriteButton.hovered || browseFavoriteButton.down || browseFavoriteButton.checked
                                        tint: browseFavoriteButton.checked ? Theme.accent : Theme.mutedText
                                    }
                                }
                            }
                        }
                    }
                    ScrollBar.vertical: ScrollBar {}
                }
            }

            GridView {
                id: mediaGrid
                objectName: "mediaGrid"
                anchors.fill: parent
                visible: root.section === "library" || root.section === "favorites" || root.section === "recent"
                enabled: visible
                clip: true
                model: root.collectionFor(root.section)
                cellWidth: width / root.cardColumns
                cellHeight: 178
                boundsBehavior: Flickable.StopAtBounds
                delegate: ItemDelegate {
                    id: mediaCard
                    required property var modelData
                    property url thumbnailUrl: modelData.thumbnail || ""
                    width: GridView.view.cellWidth - Theme.libraryGridSpacing
                    height: GridView.view.cellHeight - Theme.libraryGridSpacing
                    focusPolicy: Qt.TabFocus
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                    Accessible.name: modelData.name
                    Accessible.description: modelData.completed ? qsTr("Completed") : qsTr("Resume at %1 seconds").arg(Math.round(modelData.position || 0))
                    onClicked: {
                        if (modelData.kind === "folder") {
                            root.requestSection("browse");
                            root.fileBrowser.navigateTo(Qt.resolvedUrl("file:///" + modelData.path.replace(/\\/g, "/")));
                        } else {
                            root.openMedia(modelData.path, modelData.completed ? 0 : modelData.position);
                        }
                    }
                    Component.onCompleted: {
                        if (thumbnailUrl.toString().length === 0 && root.thumbnailService)
                            thumbnailUrl = root.thumbnailService.thumbnailFor(modelData.path);
                    }
                    Connections {
                        target: root.thumbnailService || null
                        ignoreUnknownSignals: true
                        function onThumbnailReady(path, url) {
                            if (path === mediaCard.modelData.path)
                                mediaCard.thumbnailUrl = url;
                        }
                    }
                    background: Rectangle {
                        radius: 12
                        color: mediaCard.hovered ? Theme.cardHover : Theme.cardStrong
                        border.color: mediaCard.activeFocus ? Theme.accent : Theme.separator
                        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                    }
                    contentItem: Column {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8
                        Rectangle {
                            width: parent.width
                            height: 86
                            radius: 8
                            color: Theme.panelStrong
                            Image {
                                id: mediaCardThumbnail
                                objectName: "mediaCardThumbnailImage"
                                anchors.fill: parent
                                source: mediaCard.thumbnailUrl
                                visible: source.toString().length > 0 && status !== Image.Error
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                cache: true
                            }
                            CineIcon { anchors.centerIn: parent; width: 34; height: 34; visible: mediaCardThumbnail.source.toString().length === 0 || mediaCardThumbnail.status === Image.Error; name: "cine-video-x-generic-symbolic"; filled: mediaCard.hovered || mediaCard.down; tint: Theme.mutedText }
                            Rectangle {
                                visible: mediaCard.modelData.progress > 0
                                anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                                height: 3; color: Theme.separator
                                Rectangle { width: parent.width * mediaCard.modelData.progress; height: parent.height; color: Theme.accent }
                            }
                            CineButton {
                                id: mediaCardFavoriteButton
                                objectName: "mediaCardFavoriteButton"
                                anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 6
                                width: 32; height: 32; buttonSize: 32
                                iconName: "cine-heart-symbolic"
                                checkable: true
                                checked: Boolean(mediaCard.modelData.favorite)
                                checkedBackground: "transparent"
                                checkedIconTint: Theme.accent
                                btnTooltip: qsTr("Toggle favorite")
                                onClicked: root.mediaLibrary.toggleFavorite(mediaCard.modelData.path)
                                contentItem: Item {
                                    CineIcon {
                                        objectName: "favoriteHeartIcon"
                                        anchors.centerIn: parent
                                        width: 18
                                        height: 18
                                        name: "cine-heart-symbolic"
                                        filled: mediaCardFavoriteButton.hovered || mediaCardFavoriteButton.down || mediaCardFavoriteButton.checked
                                        tint: mediaCardFavoriteButton.checked ? Theme.accent : Theme.mutedText
                                    }
                                }
                            }
                        }
                        Text { width: parent.width; text: mediaCard.modelData.name; color: Theme.text; font.pixelSize: Theme.fontSizeSmall; font.bold: true; elide: Text.ElideRight }
                        Text { width: parent.width; text: mediaCard.modelData.completed ? qsTr("Completed") : mediaCard.modelData.playCount > 0 ? qsTr("Played %1 times").arg(mediaCard.modelData.playCount) : qsTr("Not played"); color: Theme.mutedText; font.pixelSize: Theme.fontSizeCaption; elide: Text.ElideRight }
                    }
                }
                ScrollBar.vertical: ScrollBar {}
            }

            ListView {
                id: historyList
                anchors.fill: parent
                visible: root.section === "history"
                enabled: visible
                model: root.historyRows()
                clip: true
                spacing: 4
                boundsBehavior: Flickable.StopAtBounds
                delegate: Item {
                    id: historyEntry
                    required property var modelData
                    readonly property var item: modelData.isHeader ? ({}) : modelData.item
                    width: ListView.view.width
                    height: modelData.isHeader ? 32 : 78

                    Text {
                        visible: historyEntry.modelData.isHeader
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 5
                        text: historyEntry.modelData.label || ""
                        color: Theme.mutedText
                        font.pixelSize: Theme.fontSizeCaption
                        font.bold: true
                    }

                    ItemDelegate {
                        id: historyRow
                        objectName: historyEntry.modelData.isHeader ? "" : "historyRow"
                        visible: !historyEntry.modelData.isHeader
                        anchors.fill: parent
                        hoverEnabled: true
                        focusPolicy: Qt.TabFocus
                        HoverHandler { cursorShape: Qt.PointingHandCursor }
                        Accessible.name: historyEntry.item.name || ""
                        Accessible.description: historyEntry.item.completed
                                                ? qsTr("Completed")
                                                : qsTr("Resume at %1 seconds").arg(Math.round(historyEntry.item.position || 0))
                        onClicked: root.openMedia(historyEntry.item.path,
                                                  historyEntry.item.completed ? 0 : historyEntry.item.position)
                        background: Rectangle {
                            objectName: historyEntry.modelData.isHeader ? "" : "historyRowBackground"
                            radius: Theme.radius
                            color: historyRow.hovered ? Theme.cardHover : Theme.cardStrong
                            Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                        }
                        contentItem: RowLayout {
                            spacing: 12
                            Rectangle {
                                id: historyThumbnailFrame
                                Layout.preferredWidth: 86
                                Layout.preferredHeight: 54
                                radius: 7
                                color: Theme.panelStrong
                                clip: true

                                property url thumbnailUrl: historyEntry.item.thumbnail || ""

                                Component.onCompleted: {
                                    if (!historyEntry.modelData.isHeader
                                            && thumbnailUrl.toString().length === 0
                                            && root.thumbnailService) {
                                        thumbnailUrl = root.thumbnailService.thumbnailFor(historyEntry.item.path);
                                    }
                                }

                                Image {
                                    id: historyThumbnail
                                    objectName: historyEntry.modelData.isHeader ? "" : "historyThumbnailImage"
                                    anchors.fill: parent
                                    source: historyThumbnailFrame.thumbnailUrl
                                    visible: source.toString().length > 0 && status !== Image.Error
                                    fillMode: Image.PreserveAspectCrop
                                    asynchronous: true
                                    cache: true
                                }
                                CineIcon {
                                    objectName: historyEntry.modelData.isHeader ? "" : "historyThumbnailFallback"
                                    anchors.centerIn: parent
                                    width: 25
                                    height: 25
                                    visible: historyThumbnail.source.toString().length === 0
                                             || historyThumbnail.status === Image.Error
                                    name: "cine-video-x-generic-symbolic"
                                    filled: historyRow.hovered || historyRow.down
                                    tint: Theme.mutedText
                                }
                                Rectangle {
                                    visible: Number(historyEntry.item.progress || 0) > 0
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    height: 3
                                    color: Theme.separator
                                    Rectangle {
                                        width: parent.width * Number(historyEntry.item.progress || 0)
                                        height: parent.height
                                        color: Theme.accent
                                    }
                                }
                                Connections {
                                    target: root.thumbnailService || null
                                    ignoreUnknownSignals: true
                                    function onThumbnailReady(path, url) {
                                        if (path === historyEntry.item.path)
                                            historyThumbnailFrame.thumbnailUrl = url;
                                    }
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text {
                                    Layout.fillWidth: true
                                    text: historyEntry.item.name || ""
                                    color: Theme.text
                                    font.pixelSize: Theme.fontSizeSmall
                                    font.bold: true
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: historyEntry.item.openedAt
                                          ? new Date(Number(historyEntry.item.openedAt)).toLocaleTimeString(Qt.locale(), Locale.ShortFormat)
                                          : ""
                                    color: Theme.mutedText
                                    font.pixelSize: Theme.fontSizeCaption
                                }
                                Text {
                                    objectName: historyEntry.modelData.isHeader ? "" : "historyMetadataText"
                                    Layout.fillWidth: true
                                    text: root.historyWatchedText(historyEntry.item.watchedMs)
                                          + "  |  " + root.historyVisitText(historyEntry.item.visitCount)
                                          + "  |  " + root.historyEndReason(historyEntry.item.endReason)
                                    color: Theme.mutedText
                                    font.pixelSize: Theme.fontSizeCaption
                                    elide: Text.ElideRight
                                }
                            }
                            Text {
                                objectName: historyEntry.modelData.isHeader ? "" : "historyStatusText"
                                visible: !root.compact
                                text: historyEntry.item.completed ? qsTr("Completed") : qsTr("Resume")
                                color: historyEntry.item.completed ? Theme.mutedText : Theme.accent
                                font.pixelSize: Theme.fontSizeCaption
                                font.bold: true
                            }
                            CineButton {
                                objectName: historyEntry.modelData.isHeader ? "" : "removeHistoryButton"
                                Layout.preferredWidth: 36
                                Layout.preferredHeight: 36
                                buttonSize: 36
                                iconName: "cine-close-symbolic"
                                btnTooltip: qsTr("Remove from history")
                                onClicked: root.mediaLibrary.removeHistoryItem(historyEntry.item.mediaId)
                            }
                        }
                    }
                }
                ScrollBar.vertical: ScrollBar {}
            }

            Column {
                anchors.centerIn: parent
                width: Math.min(parent.width, 440)
                visible: {
                    if (root.section === "home")
                        return false;
                    if (root.section === "browse")
                        return root.fileBrowser && !root.fileBrowser.loading
                            && (root.fileBrowser.errorString.length > 0 || root.fileBrowser.count === 0);
                    if (root.section === "library" && root.mediaLibrary && root.mediaLibrary.indexing)
                        return false;
                    return root.collectionFor(root.section).length === 0;
                }
                spacing: 8
                Text {
                    width: parent.width
                    text: root.section === "browse" && root.fileBrowser && root.fileBrowser.errorString.length > 0
                        ? root.fileBrowser.errorString
                        : root.searchText.trim().length > 0 ? qsTr("No matches found") : qsTr("Nothing here yet")
                    color: Theme.text
                    font.pixelSize: 20
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                }
                Text {
                    width: parent.width
                    text: root.searchText.trim().length > 0
                        ? qsTr("Try a different search.")
                        : root.section === "browse" ? qsTr("This folder does not contain playable media.")
                        : root.section === "library" ? qsTr("Add a library folder to index your media.")
                        : qsTr("Items appear here as you use CineWindows.")
                    color: Theme.mutedText
                    font.pixelSize: Theme.fontSizeSmall
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                }
            }

            CineSpinner {
                metrics: root.metrics
                anchors.centerIn: parent
                visible: (root.fileBrowser && root.fileBrowser.loading && root.section === "browse")
                    || (root.mediaLibrary && root.mediaLibrary.indexing && root.section === "library")
            }
        }
    }
}
