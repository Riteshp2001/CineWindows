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

Loader {
    id: root

    active: false
    asynchronous: true

    // qmllint disable missing-property
    readonly property bool popupVisible: item ? item.visible : false
    property string pendingMethod: ""
    property var pendingArguments: []

    function invoke(method, args) {
        pendingMethod = method;
        pendingArguments = args || [];
        active = true;
        if (status === Loader.Ready)
            dispatchPending();
    }

    function dispatchPending() {
        if (!item || pendingMethod.length === 0)
            return;

        var method = pendingMethod;
        var args = pendingArguments;
        pendingMethod = "";
        pendingArguments = [];

        switch (method) {
        case "open":
            // qmllint disable missing-property
            item.open();
            break;
        case "openFor":
            item.openFor(args[0]);
            break;
        case "openAbove":
            item.openAbove(args[0], args[1]);
            break;
        case "popup":
            if (args.length >= 3)
                item.popup(args[0], args[1], args[2]);
            else
                item.popup();
            break;
        }
    }

    function open() {
        invoke("open", []);
    }

    function openFor(value) {
        invoke("openFor", [value]);
    }

    function openAbove(anchorItem, parentItem) {
        invoke("openAbove", [anchorItem, parentItem]);
    }

    function popup(parentItem, x, y) {
        if (arguments.length >= 3)
            invoke("popup", [parentItem, x, y]);
        else
            invoke("popup", []);
    }

    function close() {
        pendingMethod = "";
        pendingArguments = [];
        if (item)
            // qmllint disable missing-property
            item.close();
    }

    onLoaded: dispatchPending()
}
