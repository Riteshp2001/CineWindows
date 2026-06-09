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
import subprocess
from typing import cast
from gettext import gettext as _

from .compat import IS_WINDOWS
from .constants import APP_DEVELOPER, APP_ID, APP_NAME, APP_REPOSITORY_URL, RESOURCE_PREFIX

gi.require_version("Adw", "1")
gi.require_version("Gio", "2.0")
gi.require_version("GLib", "2.0")
gi.require_version("Gtk", "4.0")
from gi.repository import Adw, Gio, GLib, Gtk

from .window import CineWindow
from .preferences import Preferences, settings
from .mpris import MPRIS
from .save_session import is_same_playlist

if IS_WINDOWS:
    os.environ["GSK_RENDERER"] = "cairo"
else:
    os.environ["GSK_RENDERER"] = "gl"
    os.environ["PIPEWIRE_PROPS"] = f'{{application.icon-name="{APP_ID}"}}'


class CineApplication(Adw.Application):
    """The main application singleton class."""

    def __init__(self):
        super().__init__(
            application_id=APP_ID,
            flags=Gio.ApplicationFlags.HANDLES_OPEN,
            resource_base_path=RESOURCE_PREFIX,
        )

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
        if not IS_WINDOWS:
            try:
                MPRIS(self)
            except Exception:
                pass

        Adw.Application.do_startup(self)
        Adw.StyleManager.get_default().props.color_scheme = Adw.ColorScheme.FORCE_DARK

        self._create_action("new-window", lambda *a: self.activate(), ["<primary>n"])
        self._create_action("quit", lambda *a: self.quit(), ["<primary>q"])
        self._create_action("about", self._on_about_action)
        self._create_action(
            "preferences", self.on_preferences_action, ["<primary>comma"]
        )

    def do_activate(self):
        win = CineWindow(application=self, is_activate=True)
        win.present()

    def do_open(self, files, n_files, hint):
        win: CineWindow = cast(CineWindow, self.props.active_window)
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
                        # "1920x1080x-90" or just "1920x1080"
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
            if is_same_playlist(win.mpv.playlist):
                win.mpv.write_watch_later_config()
            win.mpv.stop()

        for gfile in files:
            path = gfile.get_path() or gfile.get_uri()
            if path:
                win.mpv.loadfile(path, "append-play")

        for window in self.get_windows():
            w = cast(CineWindow, window)
            # Pause previous opened windows
            w.mpv.pause = w != win

        win._hide_ui_timeout()

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
                return path

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
                        return gfile.get_child(name).get_path()
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
        APP_VERSION = getattr(sys.modules["__main__"], "VERSION")

        if IS_WINDOWS:
            _icon = self._load_windows_app_icon()
        else:
            _icon = APP_ID

        about = Adw.AboutDialog(
            application_name=_(APP_NAME),
            application_icon=_icon,
            developer_name=APP_DEVELOPER,
            version=APP_VERSION,
            copyright=f"(C) 2026 {APP_DEVELOPER}",
            issue_url=f"{APP_REPOSITORY_URL}/issues",
            license_type=Gtk.License.GPL_3_0,
        )
        try:
            # Translators: Replace "translator-credits" with your name/username, and optionally an email or URL.
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
    app = CineApplication()
    return app.run(sys.argv)
