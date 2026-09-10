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

// Central, remappable keyboard-shortcut registry.
//
// `actions` is the canonical, ordered list of every player action together with
// its category, human title and default key sequence. Per-user overrides are
// kept in `overrides` (id -> sequence string) and persisted as a JSON blob via
// SettingsManager.keyBindings. App.qml builds its Shortcut objects from this
// list, and ShortcutsDialog renders / edits it, so the two can never drift.
QtObject {
    id: registry

    // id            : stable action key used by App.qml's dispatch() switch
    // category      : grouping shown in the shortcuts dialog
    // title         : human-readable label
    // sequence      : default key sequence (Qt sequence string)
    /// Ordered list of all player actions with category, title, and default key sequence
    readonly property var actions: [
        // General
        {
            id: "openFiles",
            category: "General",
            title: qsTr("Open Files"),
            sequence: "Ctrl+O"
        },
        {
            id: "newWindow",
            category: "General",
            title: qsTr("New Window"),
            sequence: "Ctrl+N"
        },
        {
            id: "openFolder",
            category: "General",
            title: qsTr("Open Folder"),
            sequence: "Ctrl+I"
        },
        {
            id: "openUrl",
            category: "General",
            title: qsTr("Open URL"),
            sequence: "Ctrl+U"
        },
        {
            id: "addFiles",
            category: "General",
            title: qsTr("Add Files"),
            sequence: "Ctrl+Shift+O"
        },
        {
            id: "addFolder",
            category: "General",
            title: qsTr("Add Folder"),
            sequence: "Ctrl+Shift+I"
        },
        {
            id: "addUrl",
            category: "General",
            title: qsTr("Add URL to Playlist"),
            sequence: "Ctrl+Shift+U"
        },
        {
            id: "playlist",
            category: "General",
            title: qsTr("Playlist"),
            sequence: "Ctrl+P"
        },
        {
            id: "preferences",
            category: "General",
            title: qsTr("Preferences"),
            sequence: "Ctrl+,"
        },
        {
            id: "shortcuts",
            category: "General",
            title: qsTr("Shortcuts"),
            sequence: "Ctrl+/"
        },
        {
            id: "subtitleMenu",
            category: "General",
            title: qsTr("Subtitle Menu"),
            sequence: "Ctrl+Shift+S"
        },
        {
            id: "audioMenu",
            category: "General",
            title: qsTr("Audio Menu"),
            sequence: "Ctrl+A"
        },
        {
            id: "chaptersMenu",
            category: "General",
            title: qsTr("Chapters Menu"),
            sequence: "Ctrl+C"
        },
        {
            id: "saveSession",
            category: "General",
            title: qsTr("Save Session and Close"),
            sequence: "Shift+Q"
        },
        {
            id: "closeWindow",
            category: "General",
            title: qsTr("Close Window"),
            sequence: "Ctrl+W"
        },
        {
            id: "quit",
            category: "General",
            title: qsTr("Quit"),
            sequence: "Ctrl+Q"
        },
        {
            id: "closeWindowQ",
            category: "General",
            title: qsTr("Close Window (Q)"),
            sequence: "Q"
        },

        // Playback
        {
            id: "playPause",
            category: "Playback",
            title: qsTr("Play / Pause"),
            sequence: "Space"
        },
        {
            id: "playPauseK",
            category: "Playback",
            title: qsTr("Play / Pause (K)"),
            sequence: "K"
        },
        {
            id: "playPauseP",
            category: "Playback",
            title: qsTr("Play / Pause (P)"),
            sequence: "P"
        },
        {
            id: "speedDown",
            category: "Playback",
            title: qsTr("Decrease Speed"),
            sequence: "["
        },
        {
            id: "speedUp",
            category: "Playback",
            title: qsTr("Increase Speed"),
            sequence: "]"
        },
        {
            id: "speedHalf",
            category: "Playback",
            title: qsTr("Halve Speed"),
            sequence: "{"
        },
        {
            id: "speedDouble",
            category: "Playback",
            title: qsTr("Double Speed"),
            sequence: "}"
        },
        {
            id: "speedReset",
            category: "Playback",
            title: qsTr("Reset Speed"),
            sequence: "Backspace"
        },
        {
            id: "revertSeek",
            category: "Playback",
            title: qsTr("Undo Previous Seek"),
            sequence: "Shift+Backspace"
        },
        {
            id: "markRevertSeek",
            category: "Playback",
            title: qsTr("Mark Position for Undo Seek"),
            sequence: "Ctrl+Shift+Backspace"
        },
        {
            id: "loopFile",
            category: "Playback",
            title: qsTr("Loop File"),
            sequence: "Shift+L"
        },
        {
            id: "abLoop",
            category: "Playback",
            title: qsTr("Set / Clear A-B Loop"),
            sequence: "L"
        },

        // Navigation
        {
            id: "seekFwd5",
            category: "Navigation",
            title: qsTr("Seek 5s Forward"),
            sequence: "Right"
        },
        {
            id: "seekBack5",
            category: "Navigation",
            title: qsTr("Seek 5s Backward"),
            sequence: "Left"
        },
        {
            id: "seekFwd60",
            category: "Navigation",
            title: qsTr("Seek 1m Forward"),
            sequence: "Up"
        },
        {
            id: "seekBack60",
            category: "Navigation",
            title: qsTr("Seek 1m Backward"),
            sequence: "Down"
        },
        {
            id: "seekFwd1Exact",
            category: "Navigation",
            title: qsTr("Seek 1s Forward (Exact)"),
            sequence: "Shift+Right"
        },
        {
            id: "seekBack1Exact",
            category: "Navigation",
            title: qsTr("Seek 1s Backward (Exact)"),
            sequence: "Shift+Left"
        },
        {
            id: "seekFwd5Exact",
            category: "Navigation",
            title: qsTr("Seek 5s Forward (Exact)"),
            sequence: "Shift+Up"
        },
        {
            id: "seekBack5Exact",
            category: "Navigation",
            title: qsTr("Seek 5s Backward (Exact)"),
            sequence: "Shift+Down"
        },
        {
            id: "seekStart",
            category: "Navigation",
            title: qsTr("Seek to Start"),
            sequence: "Home"
        },
        {
            id: "seekFwd600",
            category: "Navigation",
            title: qsTr("Seek 10m Forward"),
            sequence: "Shift+PgUp"
        },
        {
            id: "seekBack600",
            category: "Navigation",
            title: qsTr("Seek 10m Backward"),
            sequence: "Shift+PgDown"
        },
        {
            id: "prevChapter",
            category: "Navigation",
            title: qsTr("Previous Chapter"),
            sequence: "PgDown"
        },
        {
            id: "nextChapter",
            category: "Navigation",
            title: qsTr("Next Chapter"),
            sequence: "PgUp"
        },
        {
            id: "prevSubLine",
            category: "Navigation",
            title: qsTr("Previous Subtitle Line"),
            sequence: "Ctrl+Left"
        },
        {
            id: "nextSubLine",
            category: "Navigation",
            title: qsTr("Next Subtitle Line"),
            sequence: "Ctrl+Right"
        },
        {
            id: "prevSubStep",
            category: "Navigation",
            title: qsTr("Sync to Previous Subtitle"),
            sequence: "Ctrl+Shift+Left"
        },
        {
            id: "nextSubStep",
            category: "Navigation",
            title: qsTr("Sync to Next Subtitle"),
            sequence: "Ctrl+Shift+Right"
        },
        {
            id: "prevFrame",
            category: "Navigation",
            title: qsTr("Previous Frame"),
            sequence: ","
        },
        {
            id: "nextFrame",
            category: "Navigation",
            title: qsTr("Next Frame"),
            sequence: "."
        },
        {
            id: "playlistPrev",
            category: "Navigation",
            title: qsTr("Previous Playlist Item"),
            sequence: "<"
        },
        {
            id: "playlistNext",
            category: "Navigation",
            title: qsTr("Next Playlist Item"),
            sequence: ">"
        },
        {
            id: "playlistNextEnter",
            category: "Navigation",
            title: qsTr("Next Playlist Item (Enter)"),
            sequence: "Enter"
        },
        {
            id: "playlistFirst",
            category: "Navigation",
            title: qsTr("First Playlist Item"),
            sequence: "Shift+Home"
        },
        {
            id: "playlistLast",
            category: "Navigation",
            title: qsTr("Last Playlist Item"),
            sequence: "Shift+End"
        },

        // Audio & Volume
        {
            id: "volumeUp",
            category: "Audio",
            title: qsTr("Volume Increase"),
            sequence: "0"
        },
        {
            id: "volumeDown",
            category: "Audio",
            title: qsTr("Volume Decrease"),
            sequence: "9"
        },
        {
            id: "volumeUpStar",
            category: "Audio",
            title: qsTr("Volume Increase (*)"),
            sequence: "*"
        },
        {
            id: "volumeDownSlash",
            category: "Audio",
            title: qsTr("Volume Decrease (/)"),
            sequence: "/"
        },
        {
            id: "mute",
            category: "Audio",
            title: qsTr("Mute / Unmute"),
            sequence: "M"
        },
        {
            id: "nextAudio",
            category: "Audio",
            title: qsTr("Next Audio Track"),
            sequence: "#"
        },
        {
            id: "prevAudio",
            category: "Audio",
            title: qsTr("Previous Audio Track"),
            sequence: "Ctrl+Shift+A"
        },
        {
            id: "audioDelayDown",
            category: "Audio",
            title: qsTr("Decrease Audio Delay"),
            sequence: "Ctrl+-"
        },
        {
            id: "audioDelayUp",
            category: "Audio",
            title: qsTr("Increase Audio Delay"),
            sequence: "Ctrl++"
        },

        // Subtitles
        {
            id: "toggleSubs",
            category: "Subtitles",
            title: qsTr("Show / Hide Subtitles"),
            sequence: "V"
        },
        {
            id: "toggleSecondarySubs",
            category: "Subtitles",
            title: qsTr("Show / Hide Secondary Subtitles"),
            sequence: "Alt+V"
        },
        {
            id: "assUseVideoData",
            category: "Subtitles",
            title: qsTr("Cycle ASS Video Data"),
            sequence: "Shift+V"
        },
        {
            id: "subStyleOverride",
            category: "Subtitles",
            title: qsTr("Toggle ASS Style Override"),
            sequence: "U"
        },
        {
            id: "nextSub",
            category: "Subtitles",
            title: qsTr("Next Subtitle Track"),
            sequence: "J"
        },
        {
            id: "prevSub",
            category: "Subtitles",
            title: qsTr("Previous Subtitle Track"),
            sequence: "Shift+J"
        },
        {
            id: "nextSecondarySub",
            category: "Subtitles",
            title: qsTr("Next Secondary Subtitle"),
            sequence: "Alt+Z"
        },
        {
            id: "prevSecondarySub",
            category: "Subtitles",
            title: qsTr("Previous Secondary Subtitle"),
            sequence: "Ctrl+Z"
        },
        {
            id: "subDelayDown",
            category: "Subtitles",
            title: qsTr("Decrease Subtitle Delay"),
            sequence: "Z"
        },
        {
            id: "subDelayUp",
            category: "Subtitles",
            title: qsTr("Increase Subtitle Delay"),
            sequence: "Shift+Z"
        },
        {
            id: "subDelayUpX",
            category: "Subtitles",
            title: qsTr("Increase Subtitle Delay (X)"),
            sequence: "X"
        },
        {
            id: "subUp",
            category: "Subtitles",
            title: qsTr("Move Subtitles Up"),
            sequence: "R"
        },
        {
            id: "subDown",
            category: "Subtitles",
            title: qsTr("Move Subtitles Down"),
            sequence: "Shift+R"
        },
        {
            id: "subDownT",
            category: "Subtitles",
            title: qsTr("Move Subtitles Down (T)"),
            sequence: "T"
        },
        {
            id: "subScaleDown",
            category: "Subtitles",
            title: qsTr("Decrease Subtitle Scale"),
            sequence: "Shift+F"
        },
        {
            id: "subScaleUp",
            category: "Subtitles",
            title: qsTr("Increase Subtitle Scale"),
            sequence: "Shift+G"
        },

        // Display & Video
        {
            id: "fullscreen",
            category: "Display",
            title: qsTr("Fullscreen"),
            sequence: "F"
        },
        {
            id: "zoomIn",
            category: "Display",
            title: qsTr("Zoom In"),
            sequence: "Alt++"
        },
        {
            id: "zoomOut",
            category: "Display",
            title: qsTr("Zoom Out"),
            sequence: "Alt+-"
        },
        {
            id: "zoomReset",
            category: "Display",
            title: qsTr("Reset Zoom and Pan"),
            sequence: "Alt+Backspace"
        },
        {
            id: "aspectRatio",
            category: "Display",
            title: qsTr("Cycle Aspect Ratio"),
            sequence: "Shift+A"
        },
        {
            id: "deband",
            category: "Display",
            title: qsTr("Toggle Debanding"),
            sequence: "B"
        },
        {
            id: "deinterlace",
            category: "Display",
            title: qsTr("Toggle Deinterlace"),
            sequence: "D"
        },
        {
            id: "panscanDown",
            category: "Display",
            title: qsTr("Decrease Panscan"),
            sequence: "W"
        },
        {
            id: "panscanUp",
            category: "Display",
            title: qsTr("Increase Panscan"),
            sequence: "Shift+W"
        },
        {
            id: "panscanUpE",
            category: "Display",
            title: qsTr("Increase Panscan (E)"),
            sequence: "E"
        },
        {
            id: "contrastDown",
            category: "Display",
            title: qsTr("Decrease Contrast"),
            sequence: "1"
        },
        {
            id: "contrastUp",
            category: "Display",
            title: qsTr("Increase Contrast"),
            sequence: "2"
        },
        {
            id: "brightnessDown",
            category: "Display",
            title: qsTr("Decrease Brightness"),
            sequence: "3"
        },
        {
            id: "brightnessUp",
            category: "Display",
            title: qsTr("Increase Brightness"),
            sequence: "4"
        },
        {
            id: "gammaDown",
            category: "Display",
            title: qsTr("Decrease Gamma"),
            sequence: "5"
        },
        {
            id: "gammaUp",
            category: "Display",
            title: qsTr("Increase Gamma"),
            sequence: "6"
        },
        {
            id: "saturationDown",
            category: "Display",
            title: qsTr("Decrease Saturation"),
            sequence: "7"
        },
        {
            id: "saturationUp",
            category: "Display",
            title: qsTr("Increase Saturation"),
            sequence: "8"
        },

        // Miscellaneous
        {
            id: "screenshot",
            category: "Miscellaneous",
            title: qsTr("Screenshot (with subtitles)"),
            sequence: "S"
        },
        {
            id: "screenshotClean",
            category: "Miscellaneous",
            title: qsTr("Screenshot (without subtitles)"),
            sequence: "Shift+S"
        },
        {
            id: "screenshotWindow",
            category: "Miscellaneous",
            title: qsTr("Screenshot Window"),
            sequence: "Ctrl+S"
        },
        {
            id: "screenshotEachFrame",
            category: "Miscellaneous",
            title: qsTr("Screenshot Every Frame"),
            sequence: "Alt+S"
        },
        {
            id: "osdLevel",
            category: "Miscellaneous",
            title: qsTr("Toggle OSD Level"),
            sequence: "Shift+O"
        },
        {
            id: "showProgress",
            category: "Miscellaneous",
            title: qsTr("Show Progress"),
            sequence: "O"
        },
        {
            id: "showProgressP",
            category: "Miscellaneous",
            title: qsTr("Show Progress (P)"),
            sequence: "Shift+P"
        },
        {
            id: "statsToggle",
            category: "Miscellaneous",
            title: qsTr("Statistics (Temporary)"),
            sequence: "I"
        },
        {
            id: "statsPage1",
            category: "Miscellaneous",
            title: qsTr("Stats Page 1 (Default)"),
            sequence: "F1"
        },
        {
            id: "statsPage2",
            category: "Miscellaneous",
            title: qsTr("Stats Page 2 (Frame Timings)"),
            sequence: "F2"
        },
        {
            id: "statsPage3",
            category: "Miscellaneous",
            title: qsTr("Stats Page 3 (Cache)"),
            sequence: "F3"
        },
        {
            id: "statsPage4",
            category: "Miscellaneous",
            title: qsTr("Stats Page 4 (Key Bindings)"),
            sequence: "F4"
        },
        {
            id: "statsPage5",
            category: "Miscellaneous",
            title: qsTr("Stats Page 5 (Tracks)"),
            sequence: "F5"
        },
        {
            id: "statsOverlay",
            category: "Miscellaneous",
            title: qsTr("Statistics Overlay (Persistent)"),
            sequence: "Shift+I"
        },
        {
            id: "keyBindingsPage",
            category: "Miscellaneous",
            title: qsTr("mpv Key Bindings Page"),
            sequence: "?"
        },
        {
            id: "debugConsole",
            category: "Miscellaneous",
            title: qsTr("Debug Console"),
            sequence: "`"
        }
    ]

    // id -> overridden sequence string.
    /// Map of action IDs to user-overridden key sequence strings
    property var overrides: ({})

    // Bumped whenever a binding changes so QML bindings on sequenceFor() refresh.
    /// Revision counter incremented on every binding change to trigger QML re-evaluation
    property int revision: 0

    /// Emitted whenever a key binding is modified, reset, or restored
    signal bindingsChanged

    /// Loads persisted key binding overrides on component initialization
    Component.onCompleted: load()

    /// Checks whether the given text is a single alphanumeric character
    /// @param text The text to check (expected single character)
    /// @returns true if the text is a single digit or letter
    function isAlphaNumeric(text) {
        if (text.length !== 1)
            return false;
        var code = text.charCodeAt(0);  // UTF-16 code unit of the single character
        return (code >= 48 && code <= 57) || (code >= 65 && code <= 90) || (code >= 97 && code <= 122);
    }

    /// Converts a QML KeyEvent into a canonical key sequence string (e.g. "Ctrl+Shift+A")
    /// @param event The QML KeyEvent to convert
    /// @returns Canonical key sequence string, or empty string for modifier-only presses
    function sequenceFromEvent(event) {
        var key = event.key;                             // Qt key code from the event
        if (key === Qt.Key_Control || key === Qt.Key_Shift || key === Qt.Key_Alt || key === Qt.Key_Meta) {
            return "";
        }

        var modifiers = event.modifiers;                 // modifier flags from the event
        var keyText = "";                                // resolved key text for the sequence
        var text = event.text || "";                     // raw character text from the event

        if (text.length === 1 && text.charCodeAt(0) >= 0x20 && !isAlphaNumeric(text) && !(key >= Qt.Key_0 && key <= Qt.Key_9)) {
            keyText = text;
            modifiers &= ~Qt.ShiftModifier;
        }

        var named = ({});                                // maps Qt key codes to human-readable names
        named[Qt.Key_Right] = "Right";
        named[Qt.Key_Up] = "Up";
        named[Qt.Key_Down] = "Down";
        named[Qt.Key_Space] = "Space";
        named[Qt.Key_Backspace] = "Backspace";
        named[Qt.Key_Return] = "Return";
        named[Qt.Key_Enter] = "Enter";
        named[Qt.Key_Escape] = "Esc";
        named[Qt.Key_PageUp] = "PgUp";
        named[Qt.Key_PageDown] = "PgDown";
        named[Qt.Key_Home] = "Home";
        named[Qt.Key_End] = "End";
        named[Qt.Key_Tab] = "Tab";
        named[Qt.Key_Delete] = "Del";
        named[Qt.Key_Comma] = ",";
        named[Qt.Key_Period] = ".";
        named[Qt.Key_Slash] = "/";
        named[Qt.Key_Backslash] = "\\";
        named[Qt.Key_BracketLeft] = "[";
        named[Qt.Key_BracketRight] = "]";
        named[Qt.Key_BraceLeft] = "{";
        named[Qt.Key_BraceRight] = "}";
        named[Qt.Key_Minus] = "-";
        named[Qt.Key_Equal] = "=";
        named[Qt.Key_Plus] = "+";
        named[Qt.Key_Asterisk] = "*";
        named[Qt.Key_NumberSign] = "#";
        named[Qt.Key_Less] = "<";
        named[Qt.Key_Greater] = ">";
        named[Qt.Key_Question] = "?";
        named[Qt.Key_QuoteLeft] = "`";
        named[Qt.Key_Dead_Grave] = "`";

        var namedKey = named[key];                       // human-readable name for this key code, if mapped
        if (keyText.length === 0 && namedKey !== undefined) {
            keyText = namedKey;
            if (keyText.length === 1 && !isAlphaNumeric(keyText))
                modifiers &= ~Qt.ShiftModifier;
        } else if (keyText.length === 0 && key >= Qt.Key_F1 && key <= Qt.Key_F35) {
            keyText = "F" + (key - Qt.Key_F1 + 1);
        } else if (keyText.length === 0 && key >= Qt.Key_A && key <= Qt.Key_Z) {
            keyText = String.fromCharCode(key);
        } else if (keyText.length === 0 && key >= Qt.Key_0 && key <= Qt.Key_9) {
            keyText = String.fromCharCode(key);
        } else if (keyText.length === 0 && text.length === 1 && text.charCodeAt(0) >= 0x20) {
            keyText = text.toUpperCase();
        }

        if (keyText.length === 0)
            return "";

        var parts = [];                                  // accumulates modifier prefixes and final key text
        if (modifiers & Qt.ControlModifier)
            parts.push("Ctrl");
        if (modifiers & Qt.AltModifier)
            parts.push("Alt");
        if (modifiers & Qt.ShiftModifier)
            parts.push("Shift");
        if (modifiers & Qt.MetaModifier)
            parts.push("Meta");
        parts.push(keyText);
        return parts.join("+");
    }

    /// Loads key binding overrides from SettingsManager and parses the JSON blob
    function load() {
        var raw = SettingsManager.keyBindings;            // JSON string of persisted overrides, or empty
        if (raw && raw.length > 0) {
            try {
                overrides = JSON.parse(raw) || {};
            } catch (e) {
                overrides = {};
            }
        }
    }

    /// Persists the current overrides map as a JSON string to SettingsManager
    function persist() {
        SettingsManager.keyBindings = JSON.stringify(overrides);
    }

    /// Returns the default key sequence for the given action ID
    /// @param id The action identifier string
    /// @returns Default key sequence string, or empty string if not found
    function defaultFor(id) {
        for (var i = 0; i < actions.length; ++i)          // scan the actions list for a matching id
            if (actions[i].id === id)
                return actions[i].sequence;
        return "";
    }

    // The active sequence for an action: override if present, else default.
    /// Returns the active (possibly overridden) key sequence for the given action ID
    /// @param id The action identifier string
    /// @returns Active key sequence string (override or default)
    function sequenceFor(id) {
        revision; // create a binding dependency so changes re-evaluate
        if (overrides.hasOwnProperty(id))
            return overrides[id];
        return defaultFor(id);
    }

    /// Returns the human-readable title for the given action ID
    /// @param id The action identifier string
    /// @returns Title string, or the id itself if not found
    function titleFor(id) {
        for (var i = 0; i < actions.length; ++i)          // scan the actions list for a matching id
            if (actions[i].id === id)
                return actions[i].title;
        return id;
    }

    // Returns the id already bound to `sequence`, or "" if none (ignoring `exceptId`).
    /// Finds an action that is already using the given key sequence
    /// @param sequence The key sequence to check for conflicts
    /// @param exceptId Action ID to exclude from the conflict check
    /// @returns The conflicting action ID, or empty string if no conflict
    function conflict(sequence, exceptId) {
        for (var i = 0; i < actions.length; ++i) {       // scan the actions list for conflicts
            var a = actions[i];                           // current action being checked
            if (a.id === exceptId)
                continue;
            if (sequenceFor(a.id) === sequence)
                return a.id;
        }
        return "";
    }

    /// Assigns a key sequence to an action, persisting the override
    /// @param id The action identifier to bind
    /// @param sequence The key sequence to assign
    function setBinding(id, sequence) {
        var next = {};                                   // copy of overrides with the new binding applied
        for (var key in overrides)
            next[key] = overrides[key];                  // shallow-copy existing overrides
        if (sequence === defaultFor(id))
            delete next[id];
        else
            next[id] = sequence;
        overrides = next;
        revision++;
        persist();
        bindingsChanged();
    }

    /// Resets a single action back to its default key sequence
    /// @param id The action identifier to reset
    function reset(id) {
        if (!overrides.hasOwnProperty(id))
            return;
        var next = {};                                   // copy of overrides excluding the reset action
        for (var key in overrides)
            if (key !== id)
                next[key] = overrides[key];              // keep every override except the one being reset
        overrides = next;
        revision++;
        persist();
        bindingsChanged();
    }

    /// Resets all actions back to their default key sequences
    function resetAll() {
        overrides = ({});
        revision++;
        persist();
        bindingsChanged();
    }
}
