# interp_config.py — CineSVP frame-interpolation settings (RIFE).
#
# Stored as plain JSON next to the mpv config (%APPDATA%\cine) so it can be read
# both by the GTK app and by the standalone VapourSynth script (rife.vpy)
# without depending on the GSettings schema (the bundled runtime ships no
# glib-compile-schemas).

import json
import os

DEFAULTS = {"enabled": True, "engine": "auto"}  # engine: auto | ncnn | trt
ENGINES = ["auto", "ncnn", "trt"]


def config_path():
    base = os.path.join(
        os.environ.get("APPDATA") or os.path.expanduser("~"), "cine"
    )
    return os.path.join(base, "interpolation.json")


def load():
    cfg = dict(DEFAULTS)
    try:
        with open(config_path(), "r", encoding="utf-8") as fh:
            data = json.load(fh)
        if isinstance(data, dict):
            for k in DEFAULTS:
                if k in data:
                    cfg[k] = data[k]
    except Exception:
        pass
    if cfg["engine"] not in ENGINES:
        cfg["engine"] = "auto"
    return cfg


def save(cfg):
    try:
        path = config_path()
        os.makedirs(os.path.dirname(path), exist_ok=True)
        out = {k: cfg.get(k, DEFAULTS[k]) for k in DEFAULTS}
        with open(path, "w", encoding="utf-8") as fh:
            json.dump(out, fh, indent=2)
    except Exception as exc:
        print("[CineSVP] interp_config save failed:", exc)
