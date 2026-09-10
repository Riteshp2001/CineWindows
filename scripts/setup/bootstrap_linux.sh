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

# CineWindows - Linux bootstrap script
# Installs system dependencies for building on Debian/Ubuntu.

echo "==> Installing Qt 6 and libmpv development packages..."

sudo apt update
sudo apt install -y \
  cmake \
  ninja-build \
  qt6-base-dev \
  qt6-declarative-dev \
  qt6-quickcontrols2-dev \
  qt6-quickdialogs2-dev \
  qt6-svg-dev \
  qt6-linguist \
  libmpv-dev \
  libgl1-mesa-dev \
  libglu1-mesa-dev \
  libxkbcommon-dev \
  libdbus-1-dev \
  pkg-config

echo "==> All dependencies installed."
echo "==> Configure with: cmake --preset release"
echo "==> Build with:     cmake --build --preset release"
