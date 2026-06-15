# interp_config.py — CineSVP frame-interpolation settings (RIFE).
#
# Stored as plain JSON next to the mpv config (%APPDATA%\cine) so it can be read
# both by the GTK app and by the standalone VapourSynth script (rife.vpy)
# without depending on the GSettings schema (the bundled runtime ships no
# glib-compile-schemas).

import json
import os
import threading
import urllib.request

DEFAULTS = {"enabled": False, "engine": "auto"}  # engine: auto | ncnn | trt
ENGINES = ["auto", "ncnn", "trt"]
PORTABLE_RIFE_MODEL_DIR = "rife-v4"
UPSTREAM_RIFE_MODEL_DIR = "rife-v4.26_ensembleFalse"

RIFE_DLL_URL = (
    "https://github.com/styler00dollar/VapourSynth-RIFE-ncnn-Vulkan/releases/download/"
    "r9_mod_v33/librife_windows_x86-64.dll"
)
RIFE_MODEL_FILES = [
    (
        "flownet.bin",
        "https://raw.githubusercontent.com/styler00dollar/"
        f"VapourSynth-RIFE-ncnn-Vulkan/master/models/{UPSTREAM_RIFE_MODEL_DIR}/"
        "flownet.bin",
    ),
    (
        "flownet.param",
        "https://raw.githubusercontent.com/styler00dollar/"
        f"VapourSynth-RIFE-ncnn-Vulkan/master/models/{UPSTREAM_RIFE_MODEL_DIR}/"
        "flownet.param",
    ),
]
RIFE_REQUIRED_FILES = [
    "rife_vs.dll",
    *[os.path.join(PORTABLE_RIFE_MODEL_DIR, name) for name, _url in RIFE_MODEL_FILES],
]


def _appdata_rife_dir():
    return os.path.join(
        os.environ.get("APPDATA") or os.path.expanduser("~"), "cine", "rife"
    )


def _portable_backend_complete(rife_dir):
    return all(os.path.exists(os.path.join(rife_dir, name)) for name in RIFE_REQUIRED_FILES)


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
    appdata = _appdata_rife_dir()
    if _portable_backend_complete(appdata):
        return appdata
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(os.path.dirname(here))
    folder = os.path.join(root, "rife")
    return folder if _portable_backend_complete(folder) else None


def ncnn_available():
    rife = bundled_rife_dir()
    if rife:
        return True
    svp = svp_install_dir()
    return bool(svp and os.path.exists(os.path.join(svp, "rife", "rife_vs.dll")))


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


def _download_file(url, dest, desc="file", progress_callback=None):
    """Download a single file with optional progress callback.
    progress_callback(current_bytes, total_bytes, description)
    """
    tmp_dest = f"{dest}.download"
    try:
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        if os.path.exists(tmp_dest):
            os.remove(tmp_dest)
        req = urllib.request.Request(url, headers={"User-Agent": "CineWindows/1.0"})
        with urllib.request.urlopen(req, timeout=60) as resp:
            total = int(resp.headers.get("Content-Length", 0))
            downloaded = 0
            with open(tmp_dest, "wb") as fh:
                while True:
                    chunk = resp.read(65536)
                    if not chunk:
                        break
                    fh.write(chunk)
                    downloaded += len(chunk)
                    if progress_callback and total:
                        progress_callback(downloaded, total, desc)
        if total and downloaded != total:
            raise OSError(f"incomplete download ({downloaded}/{total} bytes)")
        os.replace(tmp_dest, dest)
        if progress_callback and not total:
            progress_callback(downloaded, downloaded or 1, desc)
        return True
    except Exception as exc:
        try:
            if os.path.exists(tmp_dest):
                os.remove(tmp_dest)
        except Exception:
            pass
        print(f"[CineSVP] Failed to download {desc}: {exc}")
        return False


def setup_bundled_rife(progress_callback=None):
    """Download and install the portable RIFE ncnn backend (no SVP required).

    progress_callback(current_bytes, total_bytes, description) is called for
    each chunk of every file.  Returns True if all components were installed.
    """
    rife_dir = _appdata_rife_dir()
    model_dir = os.path.join(rife_dir, PORTABLE_RIFE_MODEL_DIR)

    os.makedirs(rife_dir, exist_ok=True)
    os.makedirs(model_dir, exist_ok=True)

    dll_dest = os.path.join(rife_dir, "rife_vs.dll")

    ok = True

    if not os.path.exists(dll_dest):
        ok &= _download_file(RIFE_DLL_URL, dll_dest, "RIFE DLL", progress_callback)
    else:
        print("[CineSVP] RIFE DLL already present, skipping download")

    for name, url in RIFE_MODEL_FILES:
        dest = os.path.join(model_dir, name)
        if not os.path.exists(dest):
            ok &= _download_file(url, dest, f"model ({name})", progress_callback)
        else:
            print(f"[CineSVP] Model {name} already present, skipping download")

    if ok:
        print(f"[CineSVP] Bundled RIFE backend installed at {rife_dir}")
    else:
        print("[CineSVP] Some components failed to download")

    return ok


def install_bundled_rife_async(callback=None, progress_callback=None):
    """Run setup_bundled_rife in a background thread.

    callback(success: bool) is called on success/failure (from the thread).
    progress_callback(current_bytes, total_bytes, description) is called for
    each chunk of every file (also from the thread — use GLib.idle_add to
    update UI).
    """
    def task():
        result = setup_bundled_rife(progress_callback)
        if callback:
            callback(result)
    t = threading.Thread(target=task, daemon=True)
    t.start()
