#!/usr/bin/env bash

export PIPEWIRE_MODULE_DIR="${PIPEWIRE_MODULE_DIR:-${APPDIR:-$this_dir}/usr/lib/pipewire-0.3}"
export SPA_PLUGIN_DIR="${SPA_PLUGIN_DIR:-${APPDIR:-$this_dir}/usr/lib/spa-0.2}"
export PIPEWIRE_CONFIG_DIR="${PIPEWIRE_CONFIG_DIR:-${APPDIR:-$this_dir}/usr/share/pipewire}"