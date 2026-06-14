# CineSVP — Frame Interpolation Settings

CineSVP runs **SVP's RIFE** frame interpolation *inside* Cine (in-process VapourSynth),
so SVP itself does not need to be running. Interpolation generates AI in-between
frames for smoother motion (e.g. a 24 fps film played at 48 fps).

Settings come from two places:

1. **CineSVP Preferences** (open with **Ctrl+,** → *Frame Interpolation* group) — the
   on/off switch and engine choice. Stored in
   `%APPDATA%\cine\interpolation.json`.
2. **SVP's own profile** (*SVP Control Panel → Video profiles → "RIFE AI engine"*) —
   the model, target fps, GPU, etc. Read live from
   `%APPDATA%\SVP4\settings\profiles.cfg`. Change them in SVP and CineSVP follows
   on the next play.

---

## CineSVP Preferences (Ctrl+,)

### Smooth Motion (RIFE)
Master on/off switch for interpolation.

- **On** — applies the RIFE filter during playback and forces copy-back hardware
  decoding (`hwdec=auto-copy`), which VapourSynth filters require.
- **Off** — normal playback, no interpolation, no extra GPU cost.

Toggling it takes effect immediately on the currently playing video.

### Engine
Which RIFE backend to use.

| Choice | Meaning |
|---|---|
| **Automatic (SVP)** | Use whatever SVP's profile is set to (the *Neural network engine* field below). Recommended. |
| **ncnn — any GPU** | Force the Vulkan/ncnn engine. Works on any GPU (incl. AMD/Intel). Fully self-contained — works even if SVP is uninstalled. |
| **TensorRT — NVIDIA** | Force the TensorRT engine. NVIDIA-only, fastest. Requires SVP installed (uses its TensorRT files + pre-built engines). |

Changing this reloads the filter live.

---

## SVP profile settings (decided in the SVP Control Panel)

These mirror the fields in *SVP → Video profiles → "RIFE AI engine"*.

### Neural network engine
The RIFE backend SVP is configured for.

- **NVIDIA TensorRT** — GPU-accelerated via TensorRT. Fastest; uses per-resolution
  engines pre-built for your exact GPU. NVIDIA-only.
- **ncnn / Vulkan** — portable Vulkan engine. Slower than TensorRT but runs on any
  GPU.

CineSVP follows this when **Engine = Automatic**.

### AI model
The RIFE neural-network model, e.g. **4.25 (v2)**. Trade quality vs speed:

- **Standard** (e.g. `4.25`) — balanced; SVP's default.
- **lite** — faster, slightly lower quality. Good for 4K headroom.
- **heavy** — highest quality, most GPU load.

The model is **decided by SVP** — CineSVP reads `rife_trt_model` and passes it
straight through.

### Target frame rate
What output frame rate to interpolate to. Set by the four buttons in SVP:

| SVP button | Meaning | Example (24 fps source) |
|---|---|---|
| **To screen** | Match the monitor's refresh rate | → e.g. 180 fps on a 180 Hz screen |
| **Movie ×2** | Double the source rate | 24 → 48 |
| **Movie ×2½** | 2.5× the source rate | 24 → 60 |
| **Fixed N fps** | An absolute target, any source → N | 24 → 48, 30 → 48 |

CineSVP reads `fi_target` and applies it as:
- **Fixed N fps** → output exactly N fps.
- **Movie ×N** → output `source × N`.
- **To screen** → output the display refresh (`display_fps`).
- If the source already meets/exceeds the target, interpolation is skipped
  (no needless processing of high-frame-rate content).

> **ncnn** hits the exact target fps from any source. **TensorRT** only does whole
> multipliers, so it's exact when the source divides the target (e.g. 24 → 48) and
> otherwise lands on the nearest multiple (e.g. 30 → 60 instead of a fixed 48).

### GPU device
Which GPU performs interpolation (`rife_trt_gpu` / `rife_gpu`). Defaults to your
primary GPU (device 0).

### GPU threads
How many inference streams run in parallel (`rife_threads`, default 2). More can
raise throughput on capable GPUs at the cost of VRAM/latency.

### Performance boost (TensorRT only)
`rife_trt_boost` — uses static-shape engines (one optimized engine per resolution).
Faster inference; the first time a new resolution is played, the engine is built
once (~30–90 s) and cached for next time.

### Scene change detection
Whether to avoid interpolating across hard cuts (which can cause morphing
artifacts). Currently **off** in your profile; CineSVP does not yet apply SVP's
scene-change post-pass (`SmoothFps_RIFE`).

### Duplicate frames removal
SVP option to drop duplicated source frames before interpolation. Not applied by
CineSVP's script.

---

## Where settings are stored

| File | Controls |
|---|---|
| `%APPDATA%\cine\interpolation.json` | CineSVP on/off + engine choice |
| `%APPDATA%\SVP4\settings\profiles.cfg` | Model, target fps, GPU, threads, boost (the "RIFE AI engine" profile) |
| `C:\Cine\CineSVP\scripts\rife.vpy` | The VapourSynth script that reads both and builds the RIFE filter |

## Requirements

- **ncnn** engine: self-contained (bundled in `CineSVP\rife\`); works without SVP.
- **TensorRT** engine: requires SVP 4 installed (CineSVP references its
  `rife\` TensorRT files and pre-built `.engine` files in place).
