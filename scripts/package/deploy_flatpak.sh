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

set -euo pipefail

APP_ID="com.gyrolet.CineWindows"
MANIFEST="${1:-packaging/flatpak/${APP_ID}.yml}"
ARTIFACT_DIR="${2:-packaging/flatpak/artifacts}"
BUILD_DIR="${3:-build/flatpak}"
REPO_DIR="${4:-build/flatpak-repo}"
BUNDLE_NAME="CineWindows-x86_64.flatpak"

if [ "$(uname -m)" != "x86_64" ]; then
    echo "ERROR: Flatpak bundle generation currently supports x86_64 only."
    exit 1
fi

for command_name in flatpak flatpak-builder; do
    if ! command -v "${command_name}" >/dev/null 2>&1; then
        echo "ERROR: ${command_name} is required to build the Flatpak bundle."
        exit 1
    fi
done

if [ ! -f "${MANIFEST}" ]; then
    echo "ERROR: Flatpak manifest not found: ${MANIFEST}"
    exit 1
fi

if command -v desktop-file-validate >/dev/null 2>&1; then
    desktop-file-validate packaging/linux/CineWindows.desktop
fi
if command -v appstreamcli >/dev/null 2>&1; then
    appstreamcli validate --no-net packaging/linux/${APP_ID}.metainfo.xml
fi

mkdir -p "${ARTIFACT_DIR}"
rm -rf "${BUILD_DIR}" "${REPO_DIR}"

echo "==> Building ${APP_ID} with flatpak-builder..."
flatpak-builder \
    --disable-rofiles-fuse \
    --force-clean \
    --default-branch=stable \
    --repo="${REPO_DIR}" \
    "${BUILD_DIR}" \
    "${MANIFEST}"

echo "==> Verifying installed Flatpak files..."
flatpak-builder --run "${BUILD_DIR}" "${MANIFEST}" sh -eu -c '
    test -x /app/bin/CineWindows
    test -x /app/bin/yt-dlp
    test -f /app/share/applications/com.gyrolet.CineWindows.desktop
    test -f /app/share/icons/hicolor/scalable/apps/com.gyrolet.CineWindows.svg
    test -f /app/share/metainfo/com.gyrolet.CineWindows.metainfo.xml
    test -f /app/share/licenses/com.gyrolet.CineWindows/LICENSE.md
    test -f /app/share/licenses/com.gyrolet.CineWindows/THIRD_PARTY_NOTICES.md
    ! ldd /app/bin/CineWindows | grep -q "not found"
'

echo "==> Creating ${BUNDLE_NAME}..."
flatpak build-bundle \
    --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo \
    "${REPO_DIR}" \
    "${ARTIFACT_DIR}/${BUNDLE_NAME}" \
    "${APP_ID}" \
    stable

if [ ! -s "${ARTIFACT_DIR}/${BUNDLE_NAME}" ]; then
    echo "ERROR: Flatpak bundle creation failed."
    exit 1
fi

echo "==> Flatpak bundle created: ${ARTIFACT_DIR}/${BUNDLE_NAME}"