# utils.py
#
# Copyright 2026 Diego Povliuk
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: GPL-3.0-or-later

from __future__ import annotations

import math
import os
from collections.abc import Iterable
from shutil import move
from urllib.parse import urlparse

import gi

gi.require_version("GLib", "2.0")
from gi.repository import GLib  # noqa: F401  # Kept for modules that import GLib from utils.

from .compat import (
    LAST_PLAYLIST_FILE,
    MPV_CONFIG_DIR,
    PLAYLIST_DIR as playlist_dir,
    _get_platform_config_dir,
    _get_platform_screenshots_dir,
)


SCREENSHOT_DIR = _get_platform_screenshots_dir()
CONFIG_DIR = _get_platform_config_dir()
INPUT_CONF = os.path.join(MPV_CONFIG_DIR, "input.conf")


def _migrate_legacy_playlist_file() -> None:
    """Move the old playlist file to the new playlist directory once.

    This is intentionally non-fatal because a failed migration should never
    prevent the player from starting.
    """

    old_last_pl_file = os.path.join(CONFIG_DIR, "last-playlist.m3u8")

    if not os.path.exists(old_last_pl_file):
        return

    try:
        os.makedirs(playlist_dir, exist_ok=True)

        # Prefer the canonical compat path when available. If the destination
        # already exists, keep the newer/canonical file and remove nothing.
        destination = LAST_PLAYLIST_FILE or os.path.join(playlist_dir, "last-playlist.m3u8")
        if not os.path.exists(destination):
            move(old_last_pl_file, destination)
    except Exception as exc:
        print("playlist migration warning:", exc)


_migrate_legacy_playlist_file()


def get_has_host_permission() -> bool:
    return True


def has_host_permission(path: str | os.PathLike[str] | None = None) -> bool:
    return True


def get_mouse_bindings(bindings: Iterable[dict]) -> dict[str, str]:
    active_mouse_bindings: dict[str, str] = {}

    try:
        for binding in bindings:
            key = str(binding.get("key", ""))
            cmd = str(binding.get("cmd", ""))

            if "MBTN" in key:
                active_mouse_bindings[key] = cmd
    except Exception as exc:
        print("get_mouse_bindings error:", exc)

    return active_mouse_bindings


def parse_nonrepeat_bindings(bindings: Iterable[dict]) -> set[str]:
    non_repeatable: set[str] = set()

    try:
        for binding in bindings:
            key = binding.get("key")
            cmd = str(binding.get("cmd", ""))

            if not key or "nonrepeatable" not in cmd:
                continue

            key = str(key)

            # GTK reports capital letters as plain "A", "B", etc. mpv input
            # syntax expects Shift+A for those keys.
            if len(key) == 1 and key.isupper() and key.isalpha():
                key = f"Shift+{key}"

            non_repeatable.add(key)
    except Exception as exc:
        print("parse_nonrepeat_bindings error:", exc)

    return non_repeatable


def is_local_path(path: str | os.PathLike[str]) -> bool:
    """Return True for normal local paths, file:// URLs, and Windows drive paths."""

    parsed = urlparse(str(path))
    return not parsed.scheme or parsed.scheme == "file" or len(parsed.scheme) == 1


def get_gpu_vendor(display, libgl=None):
    from .compat import get_gpu_vendor as _compat_gpu_vendor

    return _compat_gpu_vendor(display)


def format_time(seconds) -> str:
    try:
        seconds = float(seconds)
    except (TypeError, ValueError):
        return "0:00"

    if not math.isfinite(seconds) or seconds <= 0:
        return "0:00"

    total_seconds = int(seconds)
    d = total_seconds // 86400
    h = (total_seconds % 86400) // 3600
    m = (total_seconds % 3600) // 60
    s = total_seconds % 60

    if d > 0:
        return f"{d}:{h:02d}:{m:02d}:{s:02d}"
    if h > 0:
        return f"{h}:{m:02d}:{s:02d}"
    return f"{m}:{s:02d}"


MBTN_MAP: dict[int, str] = {
    1: "MBTN_LEFT",
    2: "MBTN_MID",
    3: "MBTN_RIGHT",
    8: "MBTN_BACK",
    9: "MBTN_FORWARD",
}


KEY_REMAP: dict[str, str] = {
    "F1": "F1",
    "F2": "F2",
    "F3": "F3",
    "F4": "F4",
    "F5": "F5",
    "F6": "F6",
    "F7": "F7",
    "F8": "F8",
    "F9": "F9",
    "F10": "F10",
    "F11": "F11",
    "F12": "F12",
    "F13": "F13",
    "F14": "F14",
    "F15": "F15",
    "F16": "F16",
    "F17": "F17",
    "F18": "F18",
    "F19": "F19",
    "F20": "F20",
    "Escape": "ESC",
    "BackSpace": "BS",
    "Page_Up": "PGUP",
    "Page_Down": "PGDWN",
    "Left": "LEFT",
    "Right": "RIGHT",
    "Up": "UP",
    "Down": "DOWN",
    "Home": "HOME",
    "End": "END",
    "Insert": "INS",
    "Delete": "DEL",
    "Pause": "PAUSE",
    "space": "SPACE",
    "KP_Add": "KP_ADD",
    "KP_Subtract": "KP_SUBTRACT",
    "KP_Divide": "KP_DIVIDE",
    "KP_Multiply": "KP_MULTIPLY",
    "KP_0": "KP0",
    "KP_1": "KP1",
    "KP_2": "KP2",
    "KP_3": "KP3",
    "KP_4": "KP4",
    "KP_5": "KP5",
    "KP_6": "KP6",
    "KP_7": "KP7",
    "KP_8": "KP8",
    "KP_9": "KP9",
    "KP_Decimal": "KP_DEC",
    "KP_Enter": "KP_ENTER",
    "KP_End": "KP_END",
    "KP_Down": "KP_DOWN",
    "KP_Page_Down": "KP_PGDWN",
    "KP_Left": "KP_LEFT",
    "KP_Begin": "KP_BEGIN",
    "KP_Right": "KP_RIGHT",
    "KP_Home": "KP_HOME",
    "KP_Up": "KP_UP",
    "KP_Page_Up": "KP_PGUP",
    "XF86AudioRaiseVolume": "VOLUME_UP",
    "XF86AudioLowerVolume": "VOLUME_DOWN",
    "XF86AudioMute": "MUTE",
    "XF86PowerOff": "POWER",
    "XF86AudioPlay": "PLAY",
    "XF86AudioPause": "PAUSE",
    "XF86AudioStop": "STOP",
    "XF86AudioNext": "NEXT",
    "XF86AudioPrev": "PREV",
    "ZoomIn": "ZOOMIN",
    "ZoomOut": "ZOOMOUT",
}


def _normalize_extension(extension: str) -> str:
    extension = extension.strip().lower()

    if not extension:
        return ""

    if not extension.startswith("."):
        extension = f".{extension}"

    return extension


def _dedupe_extensions(*groups: Iterable[str]) -> tuple[str, ...]:
    seen: set[str] = set()
    result: list[str] = []

    for group in groups:
        for extension in group:
            normalized = _normalize_extension(extension)
            if normalized and normalized not in seen:
                seen.add(normalized)
                result.append(normalized)

    return tuple(result)


# These extension lists are intentionally broad. mpv relies heavily on
# FFmpeg/libavformat, so real playback support depends on how mpv/FFmpeg was
# built and which demuxers/codecs/protocols are available on the target system.
# Treat these as file-picker hints, not as a strict guarantee of playback.

VIDEO_EXTS: tuple[str, ...] = _dedupe_extensions(
    (
        ".264",
        ".265",
        ".3g2",
        ".3gp",
        ".3gp2",
        ".3gpp",
        ".amv",
        ".asf",
        ".avi",
        ".av1",
        ".bdm",
        ".bdmv",
        ".bik",
        ".cine",
        ".divx",
        ".dv",
        ".dvr-ms",
        ".evo",
        ".f4v",
        ".flc",
        ".fli",
        ".flv",
        ".gxf",
        ".h261",
        ".h263",
        ".h264",
        ".h265",
        ".hevc",
        ".ivf",
        ".m1v",
        ".m2p",
        ".m2t",
        ".m2ts",
        ".m2v",
        ".m4v",
        ".mj2",
        ".mjp2",
        ".mjpeg",
        ".mjpg",
        ".mkv",
        ".mlv",
        ".mod",
        ".mov",
        ".mp4",
        ".mp4v",
        ".mpe",
        ".mpeg",
        ".mpg",
        ".mpl",
        ".mpls",
        ".mts",
        ".mxf",
        ".nsv",
        ".nut",
        ".ogm",
        ".ogv",
        ".ps",
        ".rec",
        ".rm",
        ".rmvb",
        ".roq",
        ".rv",
        ".smk",
        ".swf",
        ".tod",
        ".trp",
        ".ts",
        ".tts",
        ".vob",
        ".vro",
        ".webm",
        ".wm",
        ".wmv",
        ".wtv",
        ".y4m",
        ".yuv",
    )
)

AUDIO_EXTS: tuple[str, ...] = _dedupe_extensions(
    (
        ".3ga",
        ".669",
        ".8svx",
        ".aac",
        ".ac3",
        ".adts",
        ".aif",
        ".aifc",
        ".aiff",
        ".alac",
        ".amr",
        ".ape",
        ".au",
        ".caf",
        ".dff",
        ".dsf",
        ".dts",
        ".dtshd",
        ".eac3",
        ".f32",
        ".f64",
        ".flac",
        ".gsm",
        ".it",
        ".kar",
        ".latm",
        ".loas",
        ".m4a",
        ".m4b",
        ".m4r",
        ".mka",
        ".mlp",
        ".mod",
        ".mp1",
        ".mp2",
        ".mp3",
        ".mpa",
        ".mpc",
        ".mpp",
        ".oga",
        ".ogg",
        ".oma",
        ".opus",
        ".ra",
        ".rmi",
        ".s3m",
        ".shn",
        ".spx",
        ".tak",
        ".tta",
        ".voc",
        ".vqf",
        ".w64",
        ".wav",
        ".weba",
        ".wma",
        ".wv",
        ".xm",
        ".mid",
        ".midi",
    )
)

SUB_EXTS: tuple[str, ...] = _dedupe_extensions(
    (
        ".aqt",
        ".ass",
        ".dfxp",
        ".dks",
        ".idx",
        ".jss",
        ".lrc",
        ".mks",
        ".mpl",
        ".mpl2",
        ".pjs",
        ".psb",
        ".rt",
        ".sami",
        ".sbv",
        ".scc",
        ".smi",
        ".srt",
        ".ssa",
        ".stl",
        ".sub",
        ".sup",
        ".ttml",
        ".txt",
        ".usf",
        ".vtt",
        ".webvtt",
    )
)

IMAGE_EXTS: tuple[str, ...] = _dedupe_extensions(
    (
        ".apng",
        ".avif",
        ".bmp",
        ".cur",
        ".dds",
        ".dib",
        ".exr",
        ".gif",
        ".hdr",
        ".heic",
        ".heif",
        ".ico",
        ".jfif",
        ".jpe",
        ".jpeg",
        ".jpg",
        ".jxl",
        ".pam",
        ".pbm",
        ".pcx",
        ".pfm",
        ".pgm",
        ".png",
        ".pnm",
        ".ppm",
        ".psd",
        ".qoi",
        ".ras",
        ".sgi",
        ".svg",
        ".tga",
        ".tif",
        ".tiff",
        ".wbmp",
        ".webp",
        ".xbm",
        ".xpm",
    )
)

PLAYLIST_EXTS: tuple[str, ...] = _dedupe_extensions(
    (
        ".asx",
        ".cue",
        ".m3u",
        ".m3u8",
        ".mpcpl",
        ".pls",
        ".ram",
        ".strm",
        ".url",
        ".wax",
        ".wpl",
        ".wvx",
        ".xspf",
    )
)

DISC_IMAGE_EXTS: tuple[str, ...] = _dedupe_extensions(
    (
        ".bin",
        ".cue",
        ".ifo",
        ".img",
        ".iso",
    )
)

MEDIA_EXTS: tuple[str, ...] = _dedupe_extensions(
    VIDEO_EXTS,
    AUDIO_EXTS,
    IMAGE_EXTS,
    PLAYLIST_EXTS,
    DISC_IMAGE_EXTS,
)

# Useful when the UI wants to accept both media files and external subtitle files.
OPENABLE_EXTS: tuple[str, ...] = _dedupe_extensions(MEDIA_EXTS, SUB_EXTS)

_ext_set_cache: dict[int, frozenset[str]] = {}


def has_extension(path: str | os.PathLike[str], extensions: Iterable[str]) -> bool:
    key = id(extensions)
    ext_set = _ext_set_cache.get(key)
    if ext_set is None:
        ext_set = frozenset(extensions)
        _ext_set_cache[key] = ext_set
    return os.path.splitext(str(path))[1].lower() in ext_set


def is_video_file(path: str | os.PathLike[str]) -> bool:
    return has_extension(path, VIDEO_EXTS)


def is_audio_file(path: str | os.PathLike[str]) -> bool:
    return has_extension(path, AUDIO_EXTS)


def is_subtitle_file(path: str | os.PathLike[str]) -> bool:
    return has_extension(path, SUB_EXTS)


def is_image_file(path: str | os.PathLike[str]) -> bool:
    return has_extension(path, IMAGE_EXTS)


def is_playlist_file(path: str | os.PathLike[str]) -> bool:
    return has_extension(path, PLAYLIST_EXTS)


def is_media_file(path: str | os.PathLike[str]) -> bool:
    return has_extension(path, MEDIA_EXTS)


def is_openable_file(path: str | os.PathLike[str]) -> bool:
    return has_extension(path, OPENABLE_EXTS)
