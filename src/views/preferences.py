# preferences.py
#
# Copyright 2025 Diego Povliuk
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

import gi
import os
import subprocess
from gettext import gettext as _

from ..utils.constants import APP_ID, APP_NAME, APP_DEVELOPER, APP_REPOSITORY_URL, RESOURCE_PREFIX
from ..utils import interp_config

gi.require_version("Adw", "1")
gi.require_version("Gdk", "4.0")
gi.require_version("GLib", "2.0")
gi.require_version("Gio", "2.0")
gi.require_version("Gtk", "4.0")
from gi.repository import Adw, Gdk, Gio, GLib, Gtk

settings = Gio.Settings.new(APP_ID)


def apply_hwdec_setting(window):
    """Apply the correct decode mode for current render/filter path."""
    mpv = window.mpv

    if getattr(window, "_rife_on", False):
        mpv["hwdec"] = "auto-copy"
        return

    if getattr(window, "decouple", False):
        mpv["hwdec"] = "d3d11va"
        return

    if settings.get_boolean("hwdec"):
        mpv.command_async("vf", "remove", "@hflip")
        mpv.command_async("vf", "remove", "@vflip")
        mpv["hwdec"] = window.conf_hwdec + ["auto"]
    else:
        mpv["hwdec"] = "no"


def sync_mpv_with_settings(window):
    """Apply settings values to the mpv instance"""
    mpv = window.mpv
    mpv["sub-color"] = settings.get_string("subtitle-color")
    mpv["sub-scale"] = settings.get_double("subtitle-scale")
    mpv["sub-font"] = settings.get_string("subtitle-font")
    mpv["slang"] = settings.get_string("subtitle-languages")
    mpv["alang"] = settings.get_string("audio-languages")
    mpv["volume"] = settings.get_int("volume")
    norm_enabled = settings.get_boolean("normalize-volume")

    sub_bg = settings.get_boolean("subtitle-bg")
    mpv["sub-border-style"] = "background-box" if sub_bg else "outline-and-shadow"
    mpv["sub-shadow-offset"] = 8 if sub_bg else 0.6
    mpv["sub-back-color"] = (
        settings.get_string("subtitle-bg-color") if sub_bg else "#97000000"
    )
    mpv["sub-border-color"] = mpv["sub-back-color"]

    apply_hwdec_setting(window)

    if norm_enabled:
        mpv.command("af", "add", "@cine_loudnorm:lavfi=[loudnorm=I=-20]")


@Gtk.Template(resource_path=f"{RESOURCE_PREFIX}/preferences.ui")
class Preferences(Adw.Dialog):
    __gtype_name__ = "Preferences"

    prefs_page: Adw.PreferencesPage = Gtk.Template.Child()
    open_new_row: Adw.SwitchRow = Gtk.Template.Child()
    thumb_preview_row: Adw.SwitchRow = Gtk.Template.Child()
    hwdec_row: Adw.SwitchRow = Gtk.Template.Child()
    normalize_volume_row: Adw.SwitchRow = Gtk.Template.Child()
    save_session_switch: Gtk.Switch = Gtk.Template.Child()
    save_position_switch: Gtk.Switch = Gtk.Template.Child()
    sub_color_row: Adw.ActionRow = Gtk.Template.Child()
    reset_sub_color: Gtk.Button = Gtk.Template.Child()
    reset_sub_font: Gtk.Button = Gtk.Template.Child()
    font_row: Adw.ActionRow = Gtk.Template.Child()
    font_label: Gtk.Label = Gtk.Template.Child()
    subtitle_scale_row: Adw.SpinRow = Gtk.Template.Child()
    sub_color_btn: Gtk.ColorDialogButton = Gtk.Template.Child()
    sub_bg_color_btn: Gtk.ColorDialogButton = Gtk.Template.Child()
    left_click_row: Adw.ComboRow = Gtk.Template.Child()
    right_click_row: Adw.ComboRow = Gtk.Template.Child()
    subtitle_bg_switch: Gtk.Switch = Gtk.Template.Child()
    subtitle_lang_row: Adw.EntryRow = Gtk.Template.Child()
    audio_lang_row: Adw.EntryRow = Gtk.Template.Child()

    auto_update_row: Adw.SwitchRow = Gtk.Template.Child()
    about_version_label: Gtk.Label = Gtk.Template.Child()
    check_updates_btn: Gtk.Button = Gtk.Template.Child()
    about_app_name_label: Gtk.Label = Gtk.Template.Child()

    def __init__(self, window, **kwargs):
        super().__init__(**kwargs)
        self.win = window
        self.mpv = window.mpv

        self._bind_ui()
        self._setup_mpv_updates()
        self._setup_about_section()
        self._setup_interpolation_ui()
        self._setup_config_section()

        font = settings.get_string("subtitle-font")
        self.font_label.set_label(font)

        self.sub_color_btn.connect("notify::rgba", self._on_sub_color_selected)
        self.reset_sub_color.connect("clicked", self._on_sub_color_reset)
        self.font_row.connect("activated", self._on_font_activated)
        self.reset_sub_font.connect("clicked", self._on_font_reset)

        self.sub_color = Gdk.RGBA()
        self.sub_color.parse(settings.get_string("subtitle-color"))
        self.sub_color_btn.set_dialog(
            Gtk.ColorDialog(title=_("Subtitle Color"), modal=True, with_alpha=False)
        )
        self.sub_color_btn.set_rgba(self.sub_color)

        self.sub_bg_color_btn.connect("notify::rgba", self._on_sub_bg_color_selected)

        bg_hex = settings.get_string("subtitle-bg-color").lstrip("#")
        self.sub_bg_color = Gdk.RGBA()
        self.sub_bg_color.alpha = int(bg_hex[0:2], 16) / 255
        self.sub_bg_color.red = int(bg_hex[2:4], 16) / 255
        self.sub_bg_color.green = int(bg_hex[4:6], 16) / 255
        self.sub_bg_color.blue = int(bg_hex[6:8], 16) / 255

        self.sub_bg_color_btn.set_dialog(
            Gtk.ColorDialog(title=_("Subtitle Background"), modal=True, with_alpha=True)
        )
        self.sub_bg_color_btn.set_rgba(self.sub_bg_color)

        self.connect("closed", self._disconnect_settings)

    def _bind_ui(self):
        settings.bind(
            "open-new-windows",
            self.open_new_row,
            "active",
            Gio.SettingsBindFlags.DEFAULT,
        )
        settings.bind(
            "thumbnail-preview",
            self.thumb_preview_row,
            "active",
            Gio.SettingsBindFlags.DEFAULT,
        )
        settings.bind(
            "normalize-volume",
            self.normalize_volume_row,
            "active",
            Gio.SettingsBindFlags.DEFAULT,
        )
        settings.bind(
            "hwdec",
            self.hwdec_row,
            "active",
            Gio.SettingsBindFlags.DEFAULT,
        )
        settings.bind(
            "save-session",
            self.save_session_switch,
            "active",
            Gio.SettingsBindFlags.DEFAULT,
        )
        settings.bind(
            "save-video-position",
            self.save_position_switch,
            "active",
            Gio.SettingsBindFlags.DEFAULT,
        )
        settings.bind(
            "subtitle-scale",
            self.subtitle_scale_row,
            "value",
            Gio.SettingsBindFlags.DEFAULT,
        )
        settings.bind(
            "subtitle-bg",
            self.subtitle_bg_switch,
            "active",
            Gio.SettingsBindFlags.DEFAULT,
        )
        settings.bind(
            "left-click",
            self.left_click_row,
            "selected",
            Gio.SettingsBindFlags.DEFAULT,
        )
        settings.bind(
            "right-click",
            self.right_click_row,
            "selected",
            Gio.SettingsBindFlags.DEFAULT,
        )
        settings.bind(
            "subtitle-languages",
            self.subtitle_lang_row,
            "text",
            Gio.SettingsBindFlags.DEFAULT,
        )
        settings.bind(
            "audio-languages",
            self.audio_lang_row,
            "text",
            Gio.SettingsBindFlags.DEFAULT,
        )
        settings.bind(
            "auto-update",
            self.auto_update_row,
            "active",
            Gio.SettingsBindFlags.DEFAULT,
        )

    def _setup_mpv_updates(self):
        handlers = {
            "subtitle-color": self._on_sub_color_changed,
            "subtitle-scale": self._on_sub_scale_changed,
            "subtitle-font": self._on_sub_font_changed,
            "subtitle-languages": self._on_slang_changed,
            "subtitle-bg-color": self._on_sub_bg_color_changed,
            "subtitle-bg": self._on_sub_bg_changed,
            "audio-languages": self._on_alang_changed,
            "thumbnail-preview": self._on_thumb_preview_changed,
            "hwdec": self._on_hwdec_changed,
            "normalize-volume": self._on_norm_volume_changed,
        }

        self._setting_ids = [
            settings.connect(f"changed::{key}", callback)
            for key, callback in handlers.items()
        ]

    def _disconnect_settings(self, *a):
        for connection_id in self._setting_ids:
            settings.disconnect(connection_id)
        self.win = None
        self.mpv = None

    def _setup_interpolation_ui(self):
        """CineSVP: a Frame Interpolation group (RIFE). Built in code because the
        bundled runtime has no resource compiler to edit preferences.ui."""
        cfg = interp_config.load()

        group = Adw.PreferencesGroup(
            title=_("Frame Interpolation"),
            description=_("Generate in-between frames for smoother motion (SVP RIFE)"),
        )

        self.interp_enable_row = Adw.SwitchRow(
            title=_("Smooth Motion (RIFE)"),
            subtitle=_("Interpolate to a higher frame rate while playing"),
        )
        engine = cfg.get("engine", "auto")
        if engine == "trt" and not interp_config.trt_available():
            engine = "ncnn"
            cfg["engine"] = engine
            interp_config.save(cfg)
        backend_ok = interp_config.backend_available(engine)
        self.interp_enable_row.set_active(backend_ok and bool(cfg.get("enabled", True)))
        self._interp_enable_handler_id = self.interp_enable_row.connect(
            "notify::active", self._on_interp_enable
        )
        group.add(self.interp_enable_row)

        self.interp_engine_row = Adw.ComboRow(
            title=_("Engine"),
            subtitle=_("Automatic follows your SVP configuration"),
        )
        self.interp_engine_row.set_model(
            Gtk.StringList.new(
                [_("Automatic (SVP)"), _("ncnn — any GPU"), _("TensorRT — NVIDIA")]
            )
        )
        self.interp_engine_row.set_selected(
            interp_config.ENGINES.index(engine) if engine in interp_config.ENGINES else 0
        )
        self._interp_engine_handler_id = self.interp_engine_row.connect(
            "notify::selected", self._on_interp_engine
        )
        group.add(self.interp_engine_row)

        self.download_revealer = Gtk.Revealer()
        self.download_revealer.set_transition_type(Gtk.RevealerTransitionType.CROSSFADE)
        self.download_revealer.set_transition_duration(1200)

        self.download_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=4)
        self.download_box.set_margin_top(8)
        self.download_box.set_margin_bottom(8)
        self.download_box.set_margin_start(12)
        self.download_box.set_margin_end(12)

        self.download_label = Gtk.Label(label=_("Downloading..."))
        self.download_label.set_halign(Gtk.Align.START)
        self.download_label.set_hexpand(True)
        self.download_box.append(self.download_label)

        self.download_progress = Gtk.ProgressBar()
        self.download_progress.set_show_text(True)
        self.download_progress.set_hexpand(True)
        self.download_box.append(self.download_progress)

        self.download_revealer.set_child(self.download_box)
        self.download_revealer.set_reveal_child(False)
        self.download_revealer.set_visible(False)
        group.add(self.download_revealer)

        self.prefs_page.add(group)

    def _set_interp_switch_active(self, active):
        handler_id = getattr(self, "_interp_enable_handler_id", 0)
        if handler_id:
            self.interp_enable_row.handler_block(handler_id)
            self.interp_enable_row.set_active(active)
            self.interp_enable_row.handler_unblock(handler_id)
        else:
            self.interp_enable_row.set_active(active)

    def _set_interp_engine(self, engine):
        idx = interp_config.ENGINES.index(engine) if engine in interp_config.ENGINES else 0
        handler_id = getattr(self, "_interp_engine_handler_id", 0)
        if handler_id:
            self.interp_engine_row.handler_block(handler_id)
            self.interp_engine_row.set_selected(idx)
            self.interp_engine_row.handler_unblock(handler_id)
        else:
            self.interp_engine_row.set_selected(idx)

    def _save_scroll_position(self):
        scrolled = self._find_scrolled_window()
        if scrolled:
            vadj = scrolled.get_vadjustment()
            if vadj:
                self._saved_scroll_pos = vadj.get_value()

    def _restore_scroll_position(self):
        scrolled = self._find_scrolled_window()
        if scrolled:
            vadj = scrolled.get_vadjustment()
            if vadj and hasattr(self, "_saved_scroll_pos"):
                vadj.set_value(self._saved_scroll_pos)

    def _find_scrolled_window(self):
        def find(w):
            if isinstance(w, Gtk.ScrolledWindow):
                return w
            child = w.get_first_child()
            while child:
                found = find(child)
                if found:
                    return found
                child = child.get_next_sibling()
            return None
        return find(self.prefs_page)

    def _show_download_ui(self):
        self.download_box.remove_css_class("download-success")
        self.download_box.remove_css_class("error")
        self.download_revealer.set_visible(True)
        self.download_revealer.set_reveal_child(True)
        self.download_label.set_text(_("Downloading..."))
        self.download_progress.set_text("")
        self.download_progress.set_fraction(0)

    def _on_download_progress(self, current, total, desc):
        GLib.idle_add(self._update_download_ui, current, total, desc)

    def _update_download_ui(self, current, total, desc):
        if not self.win:
            return False
        fraction = min(current / total, 1.0) if total > 0 else 0
        pct = int(fraction * 100)
        self.download_progress.set_fraction(fraction)
        current_mb = current / (1024 * 1024)
        total_mb = total / (1024 * 1024)
        self.download_progress.set_text(f"{pct}% ({current_mb:.1f}/{total_mb:.1f} MB)")
        self.download_label.set_text(desc)
        return False

    def _on_download_success(self):
        self.download_box.add_css_class("download-success")
        self.download_progress.set_fraction(1.0)
        self.download_progress.set_text("100%")
        self.download_label.set_text(_("Download Complete"))
        GLib.timeout_add(2000, self._fade_out_download_row)

    def _on_download_failed(self):
        self.download_label.set_text(_("Download Failed"))
        self.download_progress.set_text("")
        self.download_box.add_css_class("error")
        GLib.timeout_add(3000, self._fade_out_download_row)

    def _fade_out_download_row(self):
        if not self.win:
            return False
        self.download_revealer.set_reveal_child(False)
        GLib.timeout_add(1500, self._cleanup_download_row)
        return False

    def _cleanup_download_row(self):
        if not self.win:
            return False
        self.download_revealer.set_visible(False)
        return False

    def _setup_config_section(self):
        base = os.environ.get("APPDATA") or os.path.expanduser("~")
        config_dir = os.path.join(base, "cine")

        group = Adw.PreferencesGroup(
            title=_("Configuration"),
        )

        row = Adw.ActionRow(
            title=_("Open Configuration Folder"),
            subtitle=config_dir,
            icon_name="folder-open-symbolic",
        )

        btn = Gtk.Button(label=_("Open"))
        btn.add_css_class("pill")
        btn.add_css_class("small")
        btn.connect("clicked", lambda *a: subprocess.Popen(["explorer", config_dir]))
        row.add_suffix(btn)
        row.set_activatable_widget(btn)

        group.add(row)
        self.prefs_page.add(group)

    def _on_interp_enable(self, row, *_a):
        enabled = bool(row.get_active())
        cfg = interp_config.load()
        engine = cfg.get("engine", "auto")
        if not self.win:
            cfg["enabled"] = enabled
            interp_config.save(cfg)
            return

        if enabled and engine == "trt" and not interp_config.trt_available():
            engine = "ncnn"
            cfg["engine"] = engine
            interp_config.save(cfg)
            self._set_interp_engine(engine)

        if enabled and not interp_config.backend_available(engine):
            self._save_scroll_position()
            self._start_backend_download(row, cfg)
            return
        try:
            active = self.win._apply_rife(enabled)
            cfg["enabled"] = active if enabled else False
            interp_config.save(cfg)
            if enabled and not active:
                self._set_interp_switch_active(False)
        except Exception as exc:
            print("[CineSVP] interpolation toggle failed:", exc)

    def _start_backend_download(self, row, cfg):
        if not self.win:
            return
        cfg["engine"] = "ncnn"
        cfg["enabled"] = False
        interp_config.save(cfg)
        self._set_interp_engine("ncnn")
        self._set_interp_switch_active(False)
        self._restore_scroll_position()
        self._show_download_ui()

        def on_done(success):
            GLib.idle_add(self._on_backend_download_finished, success, row, cfg)

        interp_config.install_bundled_rife_async(
            callback=on_done,
            progress_callback=self._on_download_progress,
        )

    def _on_backend_download_finished(self, success, row, cfg):
        if not self.win:
            return
        if success and interp_config.backend_available(cfg.get("engine", "auto")):
            self._on_download_success()
            try:
                active = self.win._apply_rife(True)
                cfg["enabled"] = active
                interp_config.save(cfg)
                if active:
                    self._set_interp_switch_active(True)
            except Exception as exc:
                print("[CineSVP] post-download enable failed:", exc)
            return
        self._on_download_failed()

    def _on_interp_engine(self, row, *_a):
        idx = row.get_selected()
        engine = interp_config.ENGINES[idx] if 0 <= idx < len(interp_config.ENGINES) else "auto"
        cfg = interp_config.load()
        cfg["engine"] = engine
        interp_config.save(cfg)
        if self.win and getattr(self, "interp_enable_row", None) and self.interp_enable_row.get_active():
            if engine == "trt" and not interp_config.trt_available():
                engine = "ncnn"
                cfg["engine"] = engine
                interp_config.save(cfg)
                self._set_interp_engine(engine)

            if not interp_config.backend_available(engine):
                try:
                    self.win._apply_rife(False)
                except Exception as exc:
                    print("[CineSVP] interpolation disable failed:", exc)
                self._set_interp_switch_active(False)
                cfg["enabled"] = False
                interp_config.save(cfg)
                self._save_scroll_position()
                self._start_backend_download(self.interp_enable_row, cfg)
                return
            try:
                active = self.win.reload_rife()
                if not active:
                    cfg["enabled"] = False
                    interp_config.save(cfg)
                    self._set_interp_switch_active(False)
            except Exception as exc:
                print("[CineSVP] interpolation engine change failed:", exc)

    def _setup_about_section(self):
        version = getattr(self.win.get_application(), "app_version", "1.0.0")
        self.about_app_name_label.set_label(_(APP_NAME))
        self.about_version_label.set_label(version)
        self.check_updates_btn.connect("clicked", self._on_check_updates_clicked)

    def _on_check_updates_clicked(self, _btn):
        app = self.win.get_application()
        if app and hasattr(app, "_check_for_updates"):
            app._check_for_updates(show_up_to_date=True)

    def _on_sub_color_changed(self, settings, key):
        self.mpv["sub-color"] = settings.get_string(key)

    def _on_sub_bg_color_changed(self, _settings, key):
        if settings.get_boolean("subtitle-bg"):
            self.mpv["sub-back-color"] = settings.get_string(key)
            self.mpv["sub-border-color"] = settings.get_string(key)

    def _on_sub_scale_changed(self, settings, key):
        self.mpv["sub-scale"] = settings.get_double(key)

    def _on_sub_font_changed(self, settings, key):
        self.mpv["sub-font"] = settings.get_string(key)

    def _on_sub_bg_changed(self, settings, key):
        sub_bg = settings.get_boolean(key)
        if sub_bg:
            self.mpv["sub-shadow-offset"] = 8
            self.mpv["sub-border-style"] = "background-box"
            self.mpv["sub-back-color"] = settings.get_string("subtitle-bg-color")
            self.mpv["sub-border-color"] = settings.get_string("subtitle-bg-color")
        else:
            self.mpv["sub-shadow-offset"] = 0.6
            self.mpv["sub-border-style"] = "outline-and-shadow"
            self.mpv["sub-shadow-color"] = "#97000000"

    def _on_slang_changed(self, settings, key):
        self.mpv["slang"] = settings.get_string(key)

    def _on_alang_changed(self, settings, key):
        self.mpv["alang"] = settings.get_string(key)

    def _on_thumb_preview_changed(self, settings, key):
        if not settings.get_boolean(key) and self.win.preview_player:
            self.win.preview_player.terminate()
            self.win.preview_player = None
            self.win.thumb_preview.props.visible = False
        elif not self.mpv.idle_active:
            self.win.thumb_preview.props.visible = True
            self.win.setup_preview_player()

    def _on_hwdec_changed(self, settings, key):
        apply_hwdec_setting(self.win)

    def _on_norm_volume_changed(self, settings, key):
        norm_enabled = settings.get_boolean(key)
        if norm_enabled:
            self.mpv.command("af", "add", "@cine_loudnorm:lavfi=[loudnorm=I=-20]")
        else:
            self.mpv.command("af", "remove", "@cine_loudnorm")

    def _on_sub_color_selected(self, color_btn, *arg):
        rgba = color_btn.get_rgba()
        hex_color = "#{:02x}{:02x}{:02x}".format(
            int(rgba.red * 255), int(rgba.green * 255), int(rgba.blue * 255)
        )
        settings.set_string("subtitle-color", hex_color)

    def _on_sub_bg_color_selected(self, color_btn, *arg):
        rgba = color_btn.get_rgba()
        # sub-back-color is #AARRGGBB
        hex_color = "#{:02x}{:02x}{:02x}{:02x}".format(
            int(rgba.alpha * 255),
            int(rgba.red * 255),
            int(rgba.green * 255),
            int(rgba.blue * 255),
        )
        settings.set_string("subtitle-bg-color", hex_color)

    def _on_sub_color_reset(self, _button):
        default_color = "#ebebeb"
        self.sub_color.parse(default_color)
        self.sub_color_btn.set_rgba(self.sub_color)

    def _on_font_activated(self, _row):
        dialog = Gtk.FontDialog()

        def callback(dialog, result):
            try:
                face = dialog.choose_face_finish(result)

                family_obj = face.get_family()
                family_name = family_obj.get_name()
                style_name = face.get_face_name()

                ignored_styles = [
                    "Regular",
                    "Normal",
                    "Roman",
                    "Book",
                    "Standard",
                    "Plain",
                    "Text",
                    "Semi",
                    "Semi-Bold",
                    "Demi",
                    "Demi-Bold",
                    "Upright",
                    "Alt",
                ]

                if any(s == style_name for s in ignored_styles):
                    font_full = family_name
                else:
                    # prevents "Font Bold Bold"
                    if style_name.lower() in family_name.lower():
                        font_full = family_name
                    else:
                        font_full = f"{family_name} {style_name}"

                font_full = " ".join(font_full.split())

                settings.set_string("subtitle-font", font_full)
                self.font_label.set_label(font_full)

            except Exception as e:
                print(f"Features selection error: {e}")

        dialog.choose_face(self.win, None, None, callback)

    def _on_font_reset(self, _button):
        default_font = "Adwaita Sans SemiBold"
        settings.set_string("subtitle-font", default_font)
        self.font_label.set_label(default_font)



