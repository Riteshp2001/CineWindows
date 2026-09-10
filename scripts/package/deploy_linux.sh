#!/usr/bin/env bash
# CineWindows - Video Player
# Copyright (c) 2026 Ritesh Pandit
#
# CineWindows Community License
#
# This source code is made available for personal, non-commercial
# use only. Organizations may not use, copy, modify, or distribute
# this code without written permission from Ritesh Pandit.
#
# See the LICENSE.md file for full license terms.
#
# Project: CineWindows
# Author:  Ritesh Pandit
# Last modified: 2026-09-10
# Modified by: Ritesh Pandit

set -euo pipefail

# CineWindows - Linux AppImage deployment script
# Uses linuxdeploy + linuxdeploy-plugin-qt to create an AppImage.

BUILD_DIR="${1:-build/release}"
ARTIFACT_DIR="${2:-packaging/linux/artifacts}"
APP_NAME="CineWindows"
DESKTOP_FILE="packaging/linux/${APP_NAME}.desktop"
APPDIR="AppDir"

if [ "$(uname -m)" != "x86_64" ]; then
    echo "ERROR: Linux AppImage packaging currently supports x86_64 only."
    exit 1
fi

EXEC_PATH="${BUILD_DIR}/bin/${APP_NAME}"
if [ ! -f "$EXEC_PATH" ]; then
    EXEC_PATH="${BUILD_DIR}/app/${APP_NAME}"
fi
if [ ! -f "$EXEC_PATH" ]; then
    EXEC_PATH="${BUILD_DIR}/${APP_NAME}"
fi
if [ ! -f "$EXEC_PATH" ]; then
    echo "ERROR: Build artifact not found in ${BUILD_DIR}. Build the project first."
    exit 1
fi

mkdir -p "${ARTIFACT_DIR}"
rm -rf "${APPDIR}"

cleanup() {
    rm -rf linuxdeploy linuxdeploy-plugin-qt linuxdeploy-plugin-appimage "${APPDIR}"
}
trap cleanup EXIT

download_verified() {
    local asset_url="$1"
    local destination="$2"
    local expected_sha256="$3"

    echo "==> Downloading ${destination}..."
    curl --fail --location --retry 3 \
        --header "Accept: application/octet-stream" \
        --header "X-GitHub-Api-Version: 2022-11-28" \
        "$asset_url" --output "$destination"
    echo "${expected_sha256}  ${destination}" | sha256sum --check --status
    chmod +x "$destination"
}

# Pin immutable GitHub release asset IDs and verify their published digests.
download_verified \
    "https://api.github.com/repos/linuxdeploy/linuxdeploy/releases/assets/538917371" \
    linuxdeploy \
    "36a2d7e274d12e1050d0e9ecfe11d339ed54720b2bec464c286d53f8b07f5c62"
download_verified \
    "https://api.github.com/repos/linuxdeploy/linuxdeploy-plugin-qt/releases/assets/525032210" \
    linuxdeploy-plugin-qt \
    "cfc1055b2b9dbc08412b579f20990b7b41a17b61beaa5847dc9477c96c9e9617"
download_verified \
    "https://api.github.com/repos/linuxdeploy/linuxdeploy-plugin-appimage/releases/assets/538914683" \
    linuxdeploy-plugin-appimage \
    "0441769ab38009504d2678c38cd7e526955388dd30a215b4a20afaa5471652f2"

echo "==> Creating AppDir..."
mkdir -p "${APPDIR}/usr/bin"
mkdir -p "${APPDIR}/usr/share/applications"
mkdir -p "${APPDIR}/usr/share/icons/hicolor/256x256/apps"

cp "${EXEC_PATH}" "${APPDIR}/usr/bin/"
cp "${DESKTOP_FILE}" "${APPDIR}/usr/share/applications/"
cp "resources/icons/apps/CineWindows.svg" "${APPDIR}/usr/share/icons/hicolor/256x256/apps/"

# linuxdeploy-plugin-qt cannot discover QML imports that were compiled into the
# executable unless it is explicitly given the source QML directory. Without
# this, modules such as QtQuick.Controls are omitted and the AppImage fails at
# startup with "qtquickcontrols2plugin not found".
export QML_SOURCES_PATHS="$(pwd)/qml${QML_SOURCES_PATHS:+:${QML_SOURCES_PATHS}}"

# Force the deployment plugin to inspect the exact Qt installation used by the
# build instead of accidentally picking a distro qmake/Qt installation.
if [ -z "${QMAKE:-}" ]; then
    if [ -n "${QT_ROOT_DIR:-}" ] && [ -x "${QT_ROOT_DIR}/bin/qmake" ]; then
        export QMAKE="${QT_ROOT_DIR}/bin/qmake"
    elif command -v qmake6 >/dev/null 2>&1; then
        export QMAKE="$(command -v qmake6)"
    elif command -v qmake >/dev/null 2>&1; then
        export QMAKE="$(command -v qmake)"
    else
        echo "ERROR: qmake was not found. Set QMAKE or QT_ROOT_DIR to the Qt used for this build."
        exit 1
    fi
fi

if [ -n "${QT_ROOT_DIR:-}" ] && [ -d "${QT_ROOT_DIR}/lib" ]; then
    export LD_LIBRARY_PATH="${QT_ROOT_DIR}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
fi

# Explicitly request the Qt Wayland module and native platform plugins. Qt's
# xcb plugin remains the fallback, so the same AppImage works in X11 sessions.
export EXTRA_QT_MODULES="${EXTRA_QT_MODULES:+${EXTRA_QT_MODULES};}waylandcompositor"
export EXTRA_PLATFORM_PLUGINS="${EXTRA_PLATFORM_PLUGINS:+${EXTRA_PLATFORM_PLUGINS};}libqwayland-egl.so;libqwayland-generic.so"

echo "==> Running linuxdeploy with Qt/QML deployment..."
./linuxdeploy --appdir "${APPDIR}" \
    --plugin qt \
    --desktop-file "${APPDIR}/usr/share/applications/${APP_NAME}.desktop" \
    --icon-file "${APPDIR}/usr/share/icons/hicolor/256x256/apps/CineWindows.svg"

# Do not publish another broken AppImage. Validate the two runtime components
# that caused the reported Fedora/Wayland startup failure before packaging.
if ! find "${APPDIR}" -type f -path '*/qml/QtQuick/Controls/libqtquickcontrols2plugin.so' -print -quit | grep -q .; then
    echo "ERROR: QtQuick.Controls was not deployed into the AppDir."
    echo "QML_SOURCES_PATHS=${QML_SOURCES_PATHS}"
    exit 1
fi

if ! find "${APPDIR}" -type f \( -name 'libqwayland-egl.so' -o -name 'libqwayland-generic.so' \) -print -quit | grep -q .; then
    echo "ERROR: Qt Wayland platform plugin was not deployed into the AppDir."
    echo "Ensure the Qt installation used for packaging contains Qt Wayland."
    exit 1
fi

if ! find "${APPDIR}" -type f -name 'libmpv.so*' -print -quit | grep -q .; then
    echo "ERROR: libmpv was not deployed into the AppDir."
    exit 1
fi

echo "==> Creating AppImage..."
export LDAI_OUTPUT="${APP_NAME}-x86_64.AppImage"
./linuxdeploy --appdir "${APPDIR}" --output appimage

if [ -f "${LDAI_OUTPUT}" ]; then
    if ! file "${LDAI_OUTPUT}" | grep -q 'x86-64'; then
        echo "ERROR: AppImage does not contain an x86_64 executable."
        exit 1
    fi
    mv "${LDAI_OUTPUT}" "${ARTIFACT_DIR}/"
    echo "==> AppImage created: ${ARTIFACT_DIR}/${LDAI_OUTPUT}"
else
    echo "ERROR: AppImage creation failed."
    exit 1
fi

echo "==> Done."
