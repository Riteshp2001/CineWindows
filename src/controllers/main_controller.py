# main.py
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

import os
import gi
import sys
import json
import mpv
import threading
from typing import cast
from gettext import gettext as _
from urllib.request import urlopen, Request
from urllib.error import URLError

from ..utils.constants import APP_DEVELOPER, APP_ID, APP_NAME, APP_REPOSITORY_URL, RESOURCE_PREFIX

# GTK4's default GSK renderer on Windows is the Vulkan one, which is frequently
# slower/jankier than the GL renderer for transparent, layered windows like our
# decoupled video overlay. Try Cairo first (most compatible, works on all GPUs),
# and let GTK fall back to its own defaults if Cairo isn't available.
# Users can still override with CINE_GSK_RENDERER=vulkan|gl|ngl|cairo to force a
# specific renderer.
_renderer = os.environ.get("CINE_GSK_RENDERER")
if not _renderer and sys.platform == "win32":
    _renderer = "cairo"
if _renderer:
    os.environ.setdefault("GSK_RENDERER", _renderer)

gi.require_version("Adw", "1")
gi.require_version("Gio", "2.0")
gi.require_version("GLib", "2.0")
gi.require_version("Gtk", "4.0")
from gi.repository import Adw, Gio, GLib, Gtk

from ..views.window import CineWindow
from ..views.preferences import Preferences, settings
from ..models.session import is_same_playlist
from ..utils.utils import is_media_file

class CineApplication(Adw.Application):
    """The main application singleton class."""

    def __init__(self, version="1.0.0"):
        super().__init__(
            application_id=APP_ID,
            flags=Gio.ApplicationFlags.HANDLES_OPEN,
            resource_base_path=RESOURCE_PREFIX,
        )
        self.app_version = version

        self.add_main_option(
            "new-window",
            ord("n"),
            GLib.OptionFlags.NONE,
            GLib.OptionArg.NONE,
            "Open a new window",
            None,
        )

        self.connect("shutdown", self._on_shutdown)

    def do_startup(self):
        Adw.Application.do_startup(self)
        Adw.StyleManager.get_default().props.color_scheme = Adw.ColorScheme.FORCE_DARK

        self._create_action("new-window", lambda *a: self.activate(), ["<primary>n"])
        self._create_action("quit", lambda *a: self.quit(), ["<primary>q"])
        self._create_action("about", self._on_about_action)
        self._create_action(
            "preferences", self.on_preferences_action, ["<primary>comma"]
        )
        self._create_action("check-updates", self._on_check_updates_action)
        self._create_action("shortcuts", self._on_shortcuts_action, ["<primary>question", "<primary><shift>slash", "<primary>slash"])

        if settings.get_boolean("auto-update"):
            GLib.idle_add(self._check_for_updates)

    def do_activate(self):
        win = CineWindow(application=self, is_activate=True)
        win.present()

    def do_command_line(self, command_line):
        options = command_line.get_options_dict()
        if options.contains("new-window"):
            self.activate()
            return 0

        files = command_line.get_arguments()[1:]
        if files:
            gfiles = [Gio.File.new_for_commandline_arg(f) for f in files]
            self.do_open(gfiles, len(gfiles), "")
            return 0

        self.activate()
        return 0

    def do_open(self, files, n_files, hint):
        win = self.props.active_window
        open_new = settings.get_boolean("open-new-windows") or not win

        if open_new:
            win = CineWindow(application=self)
            win.start_page.set_visible(False)

            first_video_path = None
            for gfile in files:
                first_video_path = self.find_first_file(gfile)

                if first_video_path:
                    break

            if first_video_path:
                try:
                    import subprocess
                    cmd = [
                        "ffprobe",
                        "-v",
                        "error",
                        "-select_streams",
                        "v:0",
                        "-show_entries",
                        "stream=width,height:stream_side_data=rotation",
                        "-of",
                        "csv=s=x:p=0",
                        first_video_path,
                    ]
                    output = subprocess.check_output(
                        cmd, text=True, timeout=2, stderr=subprocess.DEVNULL
                    ).strip()

                    if output:
                        parts = output.splitlines()[0].split("x")

                        width = int(parts[0])
                        height = int(parts[1])

                        try:
                            rotation = int(parts[2]) if len(parts) > 2 else 0
                        except Exception:
                            rotation = 0

                        if abs(rotation) in (90, 270):
                            w = height
                            h = width
                        else:
                            w = width
                            h = height

                        win._set_window_size(w, h)
                except Exception as e:
                    print(f"Metadata probe failed: {e}")
            win.present()
        else:
            win.present()
            try:
                if is_same_playlist(win.mpv.playlist):
                    win.mpv.write_watch_later_config()
                win.mpv.stop()
            except (AttributeError, mpv.ShutdownError):
                pass

        for gfile in files:
            path = gfile.get_path() or gfile.get_uri()
            if path:
                try:
                    win.mpv.loadfile(path, "append-play")
                except (mpv.ShutdownError, RuntimeError):
                    break

        for window in self.get_windows():
            try:
                w = window
                if hasattr(w, 'mpv') and w != win and not getattr(w.mpv, 'idle_active', True):
                    w.mpv.pause = True
            except (mpv.ShutdownError, RuntimeError):
                pass

        try:
            win._hide_ui_timeout()
        except Exception:
            pass

    def find_first_file(self, gfile, visited=None):
        """Local-only recursive search."""
        if gfile.get_uri_scheme() != "file":
            return None

        if visited is None:
            visited = set()

        path = gfile.get_path()
        if not path or path in visited:
            return None
        visited.add(path)

        try:
            info = gfile.query_info(
                "standard::type", Gio.FileQueryInfoFlags.NOFOLLOW_SYMLINKS, None
            )
            f_type = info.get_file_type()

            if f_type == Gio.FileType.REGULAR:
                if is_media_file(path):
                    return path
                return None

            if f_type == Gio.FileType.DIRECTORY:
                enumerator = gfile.enumerate_children(
                    "standard::name,standard::type",
                    Gio.FileQueryInfoFlags.NOFOLLOW_SYMLINKS,
                    None,
                )

                subdirectories = []
                for child in enumerator:
                    child_type = child.get_file_type()
                    name = child.get_name()

                    if name.startswith("."):
                        continue

                    if child_type == Gio.FileType.REGULAR:
                        child_path = gfile.get_child(name).get_path()
                        if is_media_file(child_path):
                            return child_path
                    elif child_type == Gio.FileType.DIRECTORY:
                        subdirectories.append(gfile.get_child(name))

                for folder in subdirectories:
                    found = self.find_first_file(folder, visited)
                    if found:
                        return found
        except Exception:
            pass
        return None

    # From showtime
    def do_handle_local_options(self, options: GLib.VariantDict):
        """Handle local command line arguments."""
        self.register()  # This is so props.is_remote works

        if self.props.is_remote:
            if options.contains("new-window"):
                return -1

            print(f"{APP_NAME} is running; use --new-window to open another window.")
            return 0

        return -1

    def on_preferences_action(self, *args):
        """Callback for the app.preferences action."""
        preferences = Preferences(self.props.active_window)
        preferences.present(self.props.active_window)

    def _on_about_action(self, *args):
        """Callback for the app.about action."""
        _icon = self._load_windows_app_icon()

        website = APP_REPOSITORY_URL.replace(".git", "")
        about = Adw.AboutDialog(
            application_name=_(APP_NAME),
            application_icon=_icon,
            developer_name=APP_DEVELOPER,
            version=self.app_version,
            copyright=f"(C) 2026 {APP_DEVELOPER}",
            issue_url=f"{APP_REPOSITORY_URL}/issues",
            license_type=Gtk.License.GPL_3_0,
            website=website,
            support_url=website,
        )
        try:
            about.set_translator_credits(_("translator-credits"))
        except NameError:
            pass

        about.add_acknowledgement_section(
            None,
            [
                "MPV https://mpv.io/",
                "python-mpv https://pypi.org/project/python-mpv/",
                "Celluloid https://celluloid-player.github.io/",
                "Showtime https://apps.gnome.org/Showtime/",
                "Workbench https://apps.gnome.org/Workbench/",
            ],
        )

        about.present(self.props.active_window)

    def _on_check_updates_action(self, *args):
        self._check_for_updates(show_up_to_date=True)

    def _on_shortcuts_action(self, *args):
        win = self.get_active_window()
        if win and hasattr(win, "_present_shortcuts"):
            win._present_shortcuts()

    def _check_for_updates(self, show_up_to_date=False):
        version = self.app_version

        def check():
            try:
                url = f"{APP_REPOSITORY_URL.replace('.git', '')}/releases"
                req = Request(
                    url.replace("github.com", "api.github.com/repos") + "?per_page=1",
                    headers={"Accept": "application/vnd.github.v3+json", "User-Agent": f"{APP_NAME}/{version}"},
                )
                with urlopen(req, timeout=10) as resp:
                    data = json.loads(resp.read().decode())
                
                if isinstance(data, list) and len(data) > 0:
                    data = data[0]
                else:
                    return

                latest_tag = data.get("tag_name", "").lstrip("v")
                latest_name = data.get("name", latest_tag) or latest_tag
                html_url = data.get("html_url", "")

                def do_notify():
                    win = self.props.active_window
                    if not win:
                        return

                    def parse_version(v):
                        try:
                            return tuple(int(x) for x in v.split("."))
                        except Exception:
                            return (0, 0, 0)

                    current = parse_version(version)
                    latest = parse_version(latest_tag)

                    if latest > current:
                        toast = Adw.Toast.new(
                            _("Update {} available — {}").format(latest_tag, latest_name)
                        )
                        toast.set_button_label(_("Download"))
                        toast.connect("button-clicked", lambda *a: Gtk.UriLauncher.new(html_url).launch(win, None))
                        win.toast_overlay.add_toast(toast)
                    elif show_up_to_date:
                        toast = Adw.Toast.new(_("CineWindows is up to date ({})").format(version))
                        win.toast_overlay.add_toast(toast)

                GLib.idle_add(do_notify)

            except Exception as e:
                if show_up_to_date:
                    def show_error():
                        win = self.props.active_window
                        if win:
                            toast = Adw.Toast.new(_("Could not check for updates: {}").format(str(e)))
                            win.toast_overlay.add_toast(toast)
                    GLib.idle_add(show_error)

        thread = threading.Thread(target=check, daemon=True)
        thread.start()

    def _load_windows_app_icon(self):
        """Load application icon from file for Windows (icon theme unavailable)."""
        icon_path = os.path.join(
            os.path.dirname(os.path.abspath(__file__)),
            "..", "data", "icons", "hicolor", "scalable", "apps",
            "io.github.gyrolet.CineWindows.png"
        )
        icon_path = os.path.normpath(icon_path)
        if os.path.exists(icon_path):
            try:
                return Gio.FileIcon.new(Gio.File.new_for_path(icon_path))
            except Exception:
                pass
        return APP_ID

    def _create_action(self, name, callback, shortcuts=None):
        """Add an application action."""
        action = Gio.SimpleAction.new(name, None)
        action.connect("activate", callback)
        self.add_action(action)
        if shortcuts:
            self.set_accels_for_action(f"app.{name}", shortcuts)

    def _on_shutdown(self, *args):
        for win in self.get_windows():
            win.close()


def main(version):
    """The application's entry point."""
    app = CineApplication(version=version)
    return app.run(sys.argv)
