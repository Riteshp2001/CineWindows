# Native-surface video decouple (Windows)

**Platform:** Windows (gated behind `CINE_DECOUPLE`, default on; Linux path unchanged)

## Problem

On Windows, Cine embeds mpv via the libmpv **render API** into a `GtkGLArea`: mpv
renders into an offscreen texture and **GTK presents the whole window**, compositing
that texture with the UI (`GtkGraphicsOffload` is inert on Windows — Wayland only).

Consequence: every UI-driven window present (seekbar tick, mouse motion, hover,
menus) re-blits the video out of phase with mpv's own frames, causing judder/drops.
Proven: a running `add_tick_callback` presents the whole window every vblank
(~180/s on a 180 Hz panel) while controls are visible; mouse motion compounds it.
Mitigations (throttle, change-based updates, timer instead of tick) reduce but do
not remove the coupling, because video presentation is fundamentally tied to GTK's
single window swapchain.

## Goal

Give mpv its **own native window/swapchain** so DWM presents the video on an
independent timeline. GTK UI presents (which no longer contain the video) can then
happen at any rate without touching playback.

## Architecture A — transparent GTK shell + mpv window behind

Two coordinated top-level windows:

- **GTK window (top, input owner):** the existing Cine window, unchanged in
  structure (headerbar, menus, floating auto-hide controls, all event controllers).
  The central video region becomes a **transparent hole**. The window background is
  transparent; only chrome/controls paint. All input continues to hit the GTK
  window (click-to-pause, double-click fullscreen, scroll-volume, hover, seekbar).
- **mpv window (behind, video owner):** a borderless `WS_POPUP` HWND we create and
  own. mpv renders into it with `vo=gpu-next`, `gpu-context=d3d11`,
  `hwdec=d3d11va`, `wid=<hwnd>` — its own DWM-vsynced swapchain, like standalone mpv.

Presentation is fully decoupled: mpv window presents video; GTK window presents UI.

### Feasibility (validated by throwaway spike, 2026-06-14)

1. GTK4 toplevel HWND obtainable via `GdkWin32.Win32Surface.get_handle()`. ✅
2. mpv embeds into our own HWND incl. `vo=gpu-next`+`hwdec=d3d11va`, no crash —
   provided `LC_NUMERIC=C` is re-asserted right before `mpv.MPV()` (GTK resets the
   locale on init; mpv segfaults otherwise). ✅
3. GTK4 window is genuinely transparent on Windows — video shows through the hole. ✅
4. mpv window stays aligned via `SetWindowPos` as the GTK window moves. ✅

## Components

### 1. `MpvWindow` helper (new module `src/views/mpv_window.py`)
Owns the native video surface, isolated from GTK.
- Registers a window class (once) and creates a borderless `WS_POPUP` HWND.
- `hwnd` property (int) — passed to mpv as `wid`.
- `place(x, y, w, h)` — `SetWindowPos` to a screen rect, inserted directly behind a
  given parent HWND (z-order).
- `stack_behind(parent_hwnd)`, `show()`, `hide()`, `destroy()`.
- All Win32 calls use explicit `argtypes`/`restype` (64-bit handle safety) and
  `DefWindowProcW` as the window proc.

### 2. `window.py` integration
- Decouple gated by `CINE_DECOUPLE` env (default on). When off, the existing
  GLArea/render-API path is used unchanged (instant A/B fallback).
- Decoupled path:
  - Create `MpvWindow` and pass `wid` + `vo=gpu-next` + `gpu-context=d3d11` +
    `hwdec=d3d11va` at `mpv.MPV(...)` construction (re-assert `LC_NUMERIC=C` first).
    Do **not** set `vo=libmpv`; do **not** create `MpvRenderContext`.
  - Replace the `GtkGLArea`/`GtkGraphicsOffload` child of `video_overlay` with a
    transparent placeholder widget that reserves the video area for geometry.
  - Make the window/video area transparent via CSS.
- Geometry sync (`_sync_video_geometry`): compute the placeholder's bounds relative
  to the window (`compute_bounds`), convert to a physical screen rect via the GTK
  HWND's `GetWindowRect` + scale factor, and `MpvWindow.place(...)` behind the GTK
  HWND. Driven by: window `notify` size, surface map, fullscreen/maximize state
  changes, and a light safety timer.
- Z-order/lifecycle: stack mpv behind on window realize/map and focus-in; hide on
  minimize; `destroy()` on window close.

### 3. Input & idle state
- Input unchanged — GTK window on top owns all events. Existing controllers move
  from the GLArea to the placeholder widget where needed (click-to-pause, etc.).
- Idle/empty (no file): mpv window shows black; GTK overlays (drop hint, open
  prompt) render over the transparent area as before.

## Out of scope (for now)
- Linux path (unchanged; offload already works there).
- Multi-monitor mixed-DPI perfection — handled via physical-px `GetWindowRect`, but
  exotic setups may need follow-up.
- Upstream PR polish (config toggle UI, etc.) — after it proves out locally.

## Risks / watch-items
- Z-order glitches when other windows interleave between GTK and mpv (re-stack on
  focus-in).
- Brief misalignment during live resize (safety timer + resize-driven sync).
- Fullscreen transitions must re-sync geometry.

## Verification
- Local eyeball in `C:\Cine`: video plays decoupled; controls visible + mouse motion
  cause **no** judder; fullscreen, resize, minimize/restore behave; alignment holds.
- A/B against `CINE_DECOUPLE=0` (old path) to confirm the difference.
