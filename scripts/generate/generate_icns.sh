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

# CineWindows - macOS .icns icon generator
# Converts the SVG app icon to the exact PNG set expected by iconutil.

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/../.." && pwd -P)"
ICON_SVG="${1:-${REPO_ROOT}/resources/icons/apps/CineWindows.svg}"
OUTPUT_ICNS="${2:-${REPO_ROOT}/packaging/macos/CineWindows.icns}"

if [[ ! -f "${ICON_SVG}" ]]; then
    echo "ERROR: Source icon not found: ${ICON_SVG}" >&2
    exit 1
fi

if ! command -v iconutil >/dev/null 2>&1; then
    echo "ERROR: iconutil is required and is available only on macOS." >&2
    exit 1
fi

if command -v rsvg-convert >/dev/null 2>&1; then
    RENDERER="rsvg"
elif command -v inkscape >/dev/null 2>&1; then
    RENDERER="inkscape"
elif command -v magick >/dev/null 2>&1; then
    RENDERER="magick"
elif command -v convert >/dev/null 2>&1; then
    RENDERER="convert"
else
    echo "ERROR: Install librsvg, Inkscape, or ImageMagick to render the SVG icon." >&2
    echo "       Recommended: brew install librsvg" >&2
    exit 1
fi

WORK_DIR="$(mktemp -d "${TMPDIR:-/tmp}/cinewindows-icon.XXXXXX")"
ICONSET_DIR="${WORK_DIR}/CineWindows.iconset"
trap 'rm -rf -- "${WORK_DIR}"' EXIT

mkdir -p -- "${ICONSET_DIR}" "$(dirname -- "${OUTPUT_ICNS}")"

ICON_NAMES=(
    "icon_16x16.png"
    "icon_16x16@2x.png"
    "icon_32x32.png"
    "icon_32x32@2x.png"
    "icon_128x128.png"
    "icon_128x128@2x.png"
    "icon_256x256.png"
    "icon_256x256@2x.png"
    "icon_512x512.png"
    "icon_512x512@2x.png"
)
ICON_PIXELS=(16 32 32 64 128 256 256 512 512 1024)

render_icon() {
    local pixels="$1"
    local output="$2"

    case "${RENDERER}" in
        rsvg)
            rsvg-convert --width "${pixels}" --height "${pixels}" \
                --output "${output}" "${ICON_SVG}"
            ;;
        inkscape)
            inkscape "${ICON_SVG}" \
                --export-width="${pixels}" \
                --export-height="${pixels}" \
                --export-filename="${output}"
            ;;
        magick)
            magick -background none "${ICON_SVG}" -resize "${pixels}x${pixels}" "${output}"
            ;;
        convert)
            convert -background none "${ICON_SVG}" -resize "${pixels}x${pixels}" "${output}"
            ;;
    esac
}

for index in "${!ICON_NAMES[@]}"; do
    render_icon "${ICON_PIXELS[${index}]}" "${ICONSET_DIR}/${ICON_NAMES[${index}]}"
done

echo "==> Generating ${OUTPUT_ICNS} with ${RENDERER} and iconutil..."
iconutil --convert icns "${ICONSET_DIR}" --output "${OUTPUT_ICNS}"

if [[ ! -s "${OUTPUT_ICNS}" ]]; then
    echo "ERROR: iconutil did not create a usable .icns file." >&2
    exit 1
fi

echo "==> Created: ${OUTPUT_ICNS}"
