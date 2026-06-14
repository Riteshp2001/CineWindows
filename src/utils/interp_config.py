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


def svp_install_dir():
    for folder in (
        os.environ.get("CINE_SVP_DIR"),
        r"C:\Program Files (x86)\SVP 4",
        r"C:\Program Files\SVP 4",
    ):
        if folder and os.path.isdir(os.path.join(folder, "rife")):
            return folder
    return None


def bundled_rife_dir():
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(os.path.dirname(here))
    folder = os.path.join(root, "rife")
    return folder if os.path.exists(os.path.join(folder, "rife_vs.dll")) else None


def ncnn_available():
    return bundled_rife_dir() is not None or svp_install_dir() is not None


def trt_available():
    svp_dir = svp_install_dir()
    if not svp_dir:
        return False
    rife_dir = os.path.join(svp_dir, "rife")
    return (
        os.path.exists(os.path.join(rife_dir, "vstrt.dll"))
        and os.path.exists(os.path.join(rife_dir, "akarin.dll"))
        and os.path.isdir(os.path.join(rife_dir, "vsmlrt-cuda"))
    )


def backend_available(engine):
    if engine == "trt":
        return trt_available()
    if engine == "ncnn":
        return ncnn_available()
    return ncnn_available() or trt_available()


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
