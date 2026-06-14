import os
import sys
import json
import vapoursynth as vs

core = vs.core

# ---------------------------------------------------------------------------
# Compatibility shim: Cine's R65 MINGW VapourSynth binding does not export the
# back-compat top-level constants (vs.RGBH, vs.GRAY, ...) that SVP's vsmlrt.py
# and helpers.py expect. Alias all enum members onto the module.
# ---------------------------------------------------------------------------
for _enum in ("PresetVideoFormat", "ColorFamily", "SampleType"):
    _e = getattr(vs, _enum, None)
    if _e:
        for _m in _e:
            if not hasattr(vs, _m.name):
                setattr(vs, _m.name, _m)


def _svp_install_dir():
    for d in (
        os.environ.get("CINE_SVP_DIR"),
        r"C:\Program Files (x86)\SVP 4",
        r"C:\Program Files\SVP 4",
    ):
        if d and os.path.isdir(os.path.join(d, "rife")):
            return d
    return None


def _read_active_profile():
    """Read SVP's profiles.cfg and return the active RIFE profile's settings,
    so 'SVP decides' the engine + model. Returns {} if unavailable."""
    cfg = os.path.join(os.environ.get("APPDATA", ""), "SVP4", "settings", "profiles.cfg")
    if not os.path.isfile(cfg):
        return {}
    try:
        with open(cfg, "r", encoding="utf-8") as fh:
            text = "\n".join(
                ln for ln in fh.read().splitlines() if not ln.lstrip().startswith("//")
            )
        flat = json.loads(text)
    except Exception:
        return {}
    profiles = {}
    for key, val in flat.items():
        if "/" not in key:
            continue
        guid, _, field = key.partition("/")
        profiles.setdefault(guid, {})[field] = val
    # The main interpolation profile is the one that actually enables RIFE.
    for guid, p in profiles.items():
        if p.get("rife") is True and p.get("on") is True:
            return p
    return {}


def _rife_dir(svp_dir):
    # Bundled copy inside CineSVP wins; else SVP's install.
    try:
        here = os.path.dirname(os.path.abspath(__file__))
        bundled = os.path.join(os.path.dirname(here), "rife")
        if os.path.exists(os.path.join(bundled, "rife_vs.dll")):
            return bundled
    except NameError:
        pass
    if svp_dir:
        return os.path.join(svp_dir, "rife")
    return None


clip = video_in
try:
    src_fps = float(container_fps) if container_fps and container_fps > 0.1 else 24.0
except NameError:
    src_fps = 24.0

def _engine_override():
    """User's engine choice from CineSVP preferences: auto | ncnn | trt."""
    p = os.path.join(os.environ.get("APPDATA", ""), "cine", "interpolation.json")
    try:
        with open(p, "r", encoding="utf-8") as fh:
            return (json.load(fh) or {}).get("engine", "auto")
    except Exception:
        return "auto"


prof = _read_active_profile()
svp_dir = _svp_install_dir()
engine_choice = _engine_override()

# Target frame rate / multiplier. SVP stores a target fps (fi_target); the RIFE
# frame multiplier is the nearest integer >= 2.
# Target frame rate, honoring SVP's profile target mode:
#   "Fixed N fps" -> fi_target = N, an ABSOLUTE output fps (e.g. 48, 60, 90)
#   "Movie xN"    -> fi_target = small multiplier (e.g. 2, 2.5)
#   "To screen"   -> fi_target = 0; use the display refresh rate
fi_target = float(prof.get("fi_target", 0) or 0)
disp_fps = float(globals().get("display_fps", 0) or 0)
if fi_target <= 0:
    target_fps = disp_fps or 60.0
elif fi_target < 8:
    target_fps = src_fps * fi_target          # Movie xN multiplier
else:
    target_fps = fi_target                      # Fixed N fps (absolute)

# Integer multiplier used by the TRT engine (exact when src divides target).
mult = max(1, int(target_fps / src_fps + 0.5)) if src_fps > 0 else 2

# Skip interpolation when the source already meets/exceeds the target.
do_interp = src_fps > 0 and target_fps > src_fps * 1.02

# Engine selection. The CineSVP preference wins; "auto" defers to SVP's
# rife_engine (1 = TensorRT, 0 = ncnn-Vulkan).
if engine_choice == "trt":
    use_trt = True
elif engine_choice == "ncnn":
    use_trt = False
else:
    use_trt = int(prof.get("rife_engine", 0)) == 1

src_fmt = clip.format.id

if not do_interp:
    smooth = clip
elif use_trt and svp_dir:
    rife_dir = os.path.join(svp_dir, "rife")
    cuda = os.path.join(rife_dir, "vsmlrt-cuda")
    os.environ["PATH"] = cuda + os.pathsep + rife_dir + os.pathsep + os.environ.get("PATH", "")
    if rife_dir not in sys.path:
        sys.path.append(rife_dir)
    if "trt" not in [p.namespace for p in core.plugins()]:
        core.std.LoadPlugin(path=os.path.join(rife_dir, "vstrt.dll"))
    if "akarin" not in [p.namespace for p in core.plugins()]:
        core.std.LoadPlugin(path=os.path.join(rife_dir, "akarin.dll"))

    from vsmlrt import Backend
    from helpers import RIFE_imp

    model = prof.get("rife_trt_model", "4.25_v2")
    trt_gpu = int(prof.get("rife_trt_gpu", 0))
    streams = int(prof.get("rife_threads", 2)) or 2

    # TRT static-shape engines are per-resolution; SVP pads input to a multiple
    # of `align` so it can reuse one engine (and the engines it pre-built). Match
    # that padding (and crop back) so we hit SVP's cached engines instead of
    # triggering a multi-minute rebuild for every odd resolution.
    try:
        _p = model.replace(".", "_").split("_")
        _m1, _m2 = int(_p[0]), int(_p[1])
    except Exception:
        _m1, _m2 = 4, 25
    if _m1 == 4 and _m2 == 25 and "lite" in model:
        align = 128
    elif (_m1 >= 4 and _m2 >= 26) or "heavy" in model:
        align = 64
    else:
        align = 32
    pw = (-clip.width) % align
    ph = (-clip.height) % align
    padded = clip.std.AddBorders(0, pw, 0, ph) if (pw or ph) else clip

    rgb = padded.resize.Bicubic(format=vs.PresetVideoFormat.RGBH, matrix_in_s="709")
    backend = Backend.TRT(device_id=trt_gpu, num_streams=streams)
    # vs-mlrt RIFE only does integer multipliers; use the nearest whole multiple.
    out = RIFE_imp(rgb, multi=max(2, mult), model=model, backend=backend)
    smooth = out.resize.Bicubic(format=src_fmt, matrix_s="709")
    if pw or ph:
        smooth = smooth.std.CropRel(0, pw, 0, ph)
else:
    # ncnn-Vulkan path (bundled, fully self-contained).
    rife_dir = _rife_dir(svp_dir)
    if not rife_dir or not os.path.exists(os.path.join(rife_dir, "rife_vs.dll")):
        smooth = clip
    else:
        os.environ["PATH"] = rife_dir + os.pathsep + os.environ.get("PATH", "")
        if "rife" not in [p.namespace for p in core.plugins()]:
            core.std.LoadPlugin(path=os.path.join(rife_dir, "rife_vs.dll"))
        ncnn_gpu = int(prof.get("rife_gpu", 0))
        rgb = clip.resize.Bicubic(format=vs.PresetVideoFormat.RGBS, matrix_in_s="709")
        common = dict(
            model_path=os.path.join(rife_dir, "rife-v4"),
            gpu_id=ncnn_gpu,
            gpu_thread=2,
            tta=False,
            sc=False,
        )
        # ncnn RIFE can hit the absolute target fps exactly for any source.
        from fractions import Fraction
        fr = Fraction(target_fps).limit_denominator(1000)
        out = core.rife.RIFE(rgb, fps_num=fr.numerator, fps_den=fr.denominator, **common)
        smooth = out.resize.Bicubic(format=src_fmt, matrix_s="709")

smooth.set_output()
