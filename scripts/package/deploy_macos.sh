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

# CineWindows - macOS deployment script
# Uses macdeployqt to bundle Qt frameworks/QML plugins and creates a DMG.

BUILD_DIR="${1:-build/release}"
ARTIFACT_DIR="${2:-packaging/macos/artifacts}"
ARCH="${3:-$(uname -m)}"
APP_NAME="CineWindows"

if [ ! -d "${BUILD_DIR}/${APP_NAME}.app" ] && [ ! -d "${BUILD_DIR}/app/${APP_NAME}.app" ]; then
    echo "ERROR: Build artifact (${APP_NAME}.app) not found in ${BUILD_DIR}. Build the project first."
    exit 1
fi

APP_PATH="${BUILD_DIR}/${APP_NAME}.app"
if [ ! -d "$APP_PATH" ]; then
    APP_PATH="${BUILD_DIR}/app/${APP_NAME}.app"
fi

mkdir -p "${ARTIFACT_DIR}"
LICENSE_DIR="${APP_PATH}/Contents/Resources/licenses/CineWindows"
mkdir -p "${LICENSE_DIR}"
cp LICENSE.md THIRD_PARTY_NOTICES.md "${LICENSE_DIR}/"
mkdir -p "${APP_PATH}/Contents/Resources/licenses/Qt-Advanced-Docking-System"
cp third_party/Qt-Advanced-Docking-System/LICENSE "${APP_PATH}/Contents/Resources/licenses/Qt-Advanced-Docking-System/"
cp third_party/Qt-Advanced-Docking-System/gnu-lgpl-v2.1.md "${APP_PATH}/Contents/Resources/licenses/Qt-Advanced-Docking-System/"

if ! command -v macdeployqt >/dev/null 2>&1; then
    echo "ERROR: macdeployqt was not found in PATH. Use the same Qt installation that built CineWindows."
    exit 1
fi

# Resolve the Qt installation used for this build. install-qt-action exports
# QT_ROOT_DIR in CI; qtpaths/macdeployqt provide fallbacks for local packaging.
QT_PREFIX="${QT_ROOT_DIR:-}"
if [ -z "$QT_PREFIX" ]; then
    if command -v qtpaths6 >/dev/null 2>&1; then
        QT_PREFIX="$(qtpaths6 --query QT_INSTALL_PREFIX)"
    elif command -v qtpaths >/dev/null 2>&1; then
        QT_PREFIX="$(qtpaths --query QT_INSTALL_PREFIX)"
    else
        QT_PREFIX="$(cd "$(dirname "$(command -v macdeployqt)")/.." && pwd)"
    fi
fi

if [ ! -d "${QT_PREFIX}/qml" ] || [ ! -d "${QT_PREFIX}/plugins" ]; then
    echo "ERROR: Could not resolve a valid Qt installation at: ${QT_PREFIX}"
    exit 1
fi

# Qt's CMake MACOS_BUNDLE_POST_BUILD deployment creates development-time QML
# symlinks into the build bundle. Remove that tree before macdeployqt so the
# release package receives real, relocatable QML plugin files instead of links
# back to the CI machine's Qt installation.
rm -rf "${APP_PATH}/Contents/Resources/qml"

# CineWindows uses QtSql only for SQLite. macdeployqt deploys every SQL driver when
# QtSql is linked, including Mimer/ODBC/PostgreSQL plugins whose SDK dylibs are
# intentionally absent on GitHub runners. Temporarily hide those unused drivers
# while macdeployqt runs, then restore the Qt installation on exit.
SQL_DRIVER_DIR="${QT_PREFIX}/plugins/sqldrivers"
SQL_DRIVER_BACKUP=""
restore_sql_drivers() {
    if [ -n "$SQL_DRIVER_BACKUP" ] && [ -d "$SQL_DRIVER_BACKUP" ]; then
        if [ -d "$SQL_DRIVER_DIR" ]; then
            for driver in "$SQL_DRIVER_BACKUP"/*; do
                [ -e "$driver" ] || continue
                mv "$driver" "$SQL_DRIVER_DIR/"
            done
        fi
        rm -rf "$SQL_DRIVER_BACKUP"
    fi
}
trap restore_sql_drivers EXIT

if [ -d "$SQL_DRIVER_DIR" ]; then
    SQL_DRIVER_BACKUP="$(mktemp -d)"
    for driver in "$SQL_DRIVER_DIR"/*.dylib; do
        [ -e "$driver" ] || continue
        if [ "$(basename "$driver")" != "libqsqlite.dylib" ]; then
            mv "$driver" "$SQL_DRIVER_BACKUP/"
        fi
    done
fi

echo "==> Running macdeployqt with QML import scanning..."
macdeployqt "${APP_PATH}" \
    -qmldir="$(pwd)/qml" \
    -qmlimport="${QT_PREFIX}/qml" \
    -libpath="${BUILD_DIR}/x64/lib" \
    -always-overwrite \
    -verbose=2

# Restore the local/CI Qt installation as soon as deployment is complete.
restore_sql_drivers
SQL_DRIVER_BACKUP=""
trap - EXIT

QML_CONTROLS_PLUGIN="${APP_PATH}/Contents/Resources/qml/QtQuick/Controls/libqtquickcontrols2plugin.dylib"
if [ ! -e "$QML_CONTROLS_PLUGIN" ]; then
    echo "ERROR: QtQuick.Controls plugin was not bundled by macdeployqt."
    exit 1
fi

# A release bundle must not contain QML symlinks that resolve outside the app.
APP_ABS="$(cd "$(dirname "$APP_PATH")" && pwd)/$(basename "$APP_PATH")"
EXTERNAL_QML_LINK=0
while IFS= read -r link; do
    resolved="$(realpath "$link" 2>/dev/null || true)"
    case "$resolved" in
        "${APP_ABS}"/*) ;;
        *)
            echo "ERROR: QML symlink escapes the app bundle: $link -> ${resolved:-<missing>}"
            EXTERNAL_QML_LINK=1
            ;;
    esac
done < <(find "${APP_PATH}/Contents/Resources/qml" -type l 2>/dev/null || true)
if [ "$EXTERNAL_QML_LINK" -ne 0 ]; then
    exit 1
fi

# Only SQLite is used by CineWindows. Ensure no accidental database-driver runtime
# dependency is shipped in the app bundle.
SQLITE_PLUGIN="${APP_PATH}/Contents/PlugIns/sqldrivers/libqsqlite.dylib"
if [ ! -e "$SQLITE_PLUGIN" ]; then
    echo "ERROR: SQLite Qt SQL driver was not bundled."
    exit 1
fi
if find "${APP_PATH}/Contents/PlugIns/sqldrivers" -maxdepth 1 -type f -name '*.dylib' ! -name 'libqsqlite.dylib' -print -quit | grep -q .; then
    echo "ERROR: Unused Qt SQL drivers were bundled:"
    find "${APP_PATH}/Contents/PlugIns/sqldrivers" -maxdepth 1 -type f -name '*.dylib' ! -name 'libqsqlite.dylib' -print
    exit 1
fi

FRAMEWORKS_DIR="${APP_PATH}/Contents/Frameworks"
MAIN_BINARY="${APP_PATH}/Contents/MacOS/${APP_NAME}"
MPV_BUNDLED="${FRAMEWORKS_DIR}/libmpv.2.dylib"
if ! find "$FRAMEWORKS_DIR" -type f -name 'libqtadvanceddocking-qt6*.dylib' -print -quit | grep -q .; then
    echo "ERROR: Qt Advanced Docking System was not bundled into Contents/Frameworks."
    exit 1
fi

# macdeployqt follows CineWindows -> MpvQt -> libmpv and deploys the observed
# dylib dependency graph. Treat a missing bundled libmpv as a hard packaging
# failure rather than trying to patch a partially deployed app afterward.
if [ ! -e "$MPV_BUNDLED" ]; then
    echo "ERROR: libmpv.2.dylib was not bundled into Contents/Frameworks."
    exit 1
fi

echo "==> Verifying architecture and runtime dependencies..."
for binary in "$MAIN_BINARY" "$MPV_BUNDLED"; do
    if ! lipo -archs "$binary" | tr ' ' '\n' | grep -qx "$ARCH"; then
        echo "ERROR: $(basename "$binary") does not contain requested architecture: $ARCH"
        lipo -archs "$binary"
        exit 1
    fi
done

# Every non-system absolute Mach-O dependency must resolve inside the bundle.
# This catches Homebrew, CI Qt, Postgres/ODBC SDK, or other host-machine paths.
BAD_REFS=0
while IFS= read -r binary; do
    if file "$binary" 2>/dev/null | grep -q 'Mach-O'; then
        refs="$(otool -L "$binary" 2>/dev/null | tail -n +2 | awk '{print $1}' | grep '^/' | grep -Ev '^(/System/Library/|/usr/lib/)' || true)"
        if [ -n "$refs" ]; then
            echo "ERROR: Host-machine runtime references remain in $binary:"
            echo "$refs"
            BAD_REFS=1
        fi
    fi
done < <(find "${APP_PATH}/Contents" -type f)

if [ "$BAD_REFS" -ne 0 ]; then
    exit 1
fi

echo "==> Bundle dependency validation passed."

# CI builds one architecture per runner, so label the artifact truthfully.
DMG_NAME="${APP_NAME}-${ARCH}.dmg"
rm -f "${ARTIFACT_DIR}/${DMG_NAME}"

echo "==> Creating ${DMG_NAME}..."
if ! command -v create-dmg >/dev/null 2>&1; then
    if command -v brew >/dev/null 2>&1; then
        echo "==> Installing create-dmg..."
        brew install create-dmg
    else
        echo "ERROR: create-dmg was not found."
        exit 1
    fi
fi

create-dmg \
    --volname "${APP_NAME}" \
    --volicon "packaging/macos/CineWindows.icns" \
    --window-pos 200 120 \
    --window-size 800 400 \
    --icon-size 100 \
    --icon "${APP_NAME}.app" 200 190 \
    --hide-extension "${APP_NAME}.app" \
    --app-drop-link 600 185 \
    "${ARTIFACT_DIR}/${DMG_NAME}" \
    "${APP_PATH}"

echo "==> DMG created: ${ARTIFACT_DIR}/${DMG_NAME}"
echo "==> Done."
