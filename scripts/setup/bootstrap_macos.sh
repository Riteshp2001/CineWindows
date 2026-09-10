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

# CineWindows - macOS bootstrap script
# Installs system dependencies for native arm64 or x86_64 builds via Homebrew.

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/../.." && pwd -P)"

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "ERROR: This bootstrap script must be run on macOS." >&2
    exit 1
fi

if ! command -v brew >/dev/null 2>&1; then
    echo "ERROR: Homebrew is required. Install it from https://brew.sh first." >&2
    exit 1
fi

ARCH="$(uname -m)"
case "${ARCH}" in
    arm64|x86_64)
        ;;
    *)
        echo "ERROR: Unsupported macOS architecture: ${ARCH}" >&2
        exit 1
        ;;
esac

echo "==> Installing CineWindows build dependencies for ${ARCH}..."

brew install \
    cmake \
    ninja \
    qt@6 \
    mpv \
    pkg-config \
    extra-cmake-modules \
    librsvg \
    create-dmg

QT_PREFIX="$(brew --prefix qt@6)"
MPV_PREFIX="$(brew --prefix mpv)"
ECM_PREFIX="$(brew --prefix extra-cmake-modules)"

echo "==> All dependencies installed."
echo "==> Repository: ${REPO_ROOT}"
echo "==> Generate the app icon before configuring:"
echo "    ${REPO_ROOT}/scripts/generate/generate_icns.sh"
echo "==> Configure a native ${ARCH} build:"
echo "    cmake --preset release -DCINEWINDOWS_MACOS_ARCHITECTURES=${ARCH} -DCMAKE_PREFIX_PATH=\"${QT_PREFIX};${MPV_PREFIX};${ECM_PREFIX}\""
echo "==> Build and package:"
echo "    cmake --build --preset release"
echo "    ${REPO_ROOT}/scripts/package/deploy_macos.sh \"${REPO_ROOT}/build/release\" \"${REPO_ROOT}/packaging/macos/artifacts\" ${ARCH}"
