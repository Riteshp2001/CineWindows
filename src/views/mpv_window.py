"""Native borderless video window for the decoupled rendering path (Windows).

On Windows, embedding mpv via the libmpv render API into a GtkGLArea couples video
presentation to GTK's window swapchain: any UI-driven repaint re-presents the video
out of phase with mpv's frames and causes judder. To decouple, mpv gets its OWN
native window (its own DWM-vsynced swapchain) pinned directly behind a transparent
region of the GTK window. This module owns that native window.

It is intentionally free of any GTK dependency: it just creates/owns an HWND and
exposes geometry/z-order/lifecycle operations.
"""

import ctypes
from ctypes import wintypes

_user32 = ctypes.windll.user32
_gdi32 = ctypes.windll.gdi32
_kernel32 = ctypes.windll.kernel32
try:
    _dwmapi = ctypes.windll.dwmapi
except OSError:
    _dwmapi = None

# --- Win32 prototypes (explicit so 64-bit handles are never truncated) ---
_LRESULT = ctypes.c_ssize_t
_user32.DefWindowProcW.restype = _LRESULT
_user32.DefWindowProcW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
_user32.CreateWindowExW.restype = wintypes.HWND
_user32.CreateWindowExW.argtypes = [
    wintypes.DWORD, wintypes.LPCWSTR, wintypes.LPCWSTR, wintypes.DWORD,
    ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int,
    wintypes.HWND, wintypes.HMENU, wintypes.HINSTANCE, wintypes.LPVOID,
]
_user32.DestroyWindow.argtypes = [wintypes.HWND]
_user32.SetWindowPos.restype = wintypes.BOOL
_user32.SetWindowPos.argtypes = [
    wintypes.HWND, wintypes.HWND, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, wintypes.UINT
]
_user32.ShowWindow.argtypes = [wintypes.HWND, ctypes.c_int]
_user32.RegisterClassW.argtypes = [ctypes.c_void_p]
_kernel32.GetModuleHandleW.restype = wintypes.HMODULE
_kernel32.GetModuleHandleW.argtypes = [wintypes.LPCWSTR]
_gdi32.GetStockObject.restype = wintypes.HGDIOBJ
_gdi32.GetStockObject.argtypes = [ctypes.c_int]

# --- constants ---
_WS_POPUP = 0x80000000
_WS_VISIBLE = 0x10000000
_WS_CHILD = 0x40000000
# Extended styles: keep the video surface off the taskbar and out of alt-tab, and
# never let it take activation/focus, so it is hardwired to the GTK window and the
# user can't swap to a bare mpv window.
_WS_EX_TOOLWINDOW = 0x00000080
_WS_EX_NOACTIVATE = 0x08000000
# DWM: round the corners to match the GTK window (Windows 11).
_DWMWA_WINDOW_CORNER_PREFERENCE = 33
_DWMWCP_ROUND = 2
_SWP_NOACTIVATE = 0x0010
_SWP_NOSIZE = 0x0001
_SWP_NOMOVE = 0x0002
_SWP_NOREDRAW = 0x0008
_SW_HIDE = 0
_SW_SHOWNA = 8  # show without activating
_BLACK_BRUSH = 4

_WNDPROC = ctypes.WINFUNCTYPE(_LRESULT, wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM)


def _default_wndproc(hwnd, msg, wparam, lparam):
    return _user32.DefWindowProcW(hwnd, msg, wparam, lparam)


_wndproc_ref = _WNDPROC(_default_wndproc)  # keep a strong ref so it isn't GC'd


class _WNDCLASS(ctypes.Structure):
    _fields_ = [
        ("style", wintypes.UINT),
        ("lpfnWndProc", _WNDPROC),
        ("cbClsExtra", ctypes.c_int),
        ("cbWndExtra", ctypes.c_int),
        ("hInstance", wintypes.HINSTANCE),
        ("hIcon", wintypes.HICON),
        ("hCursor", wintypes.HANDLE),
        ("hbrBackground", wintypes.HBRUSH),
        ("lpszMenuName", wintypes.LPCWSTR),
        ("lpszClassName", wintypes.LPCWSTR),
    ]


_CLASS_NAME = "CineMpvVideoWindow"
_class_registered = False


def _ensure_class():
    global _class_registered
    if _class_registered:
        return
    hinst = _kernel32.GetModuleHandleW(None)
    wc = _WNDCLASS()
    wc.lpfnWndProc = _wndproc_ref
    wc.hInstance = hinst
    wc.lpszClassName = _CLASS_NAME
    wc.hbrBackground = _gdi32.GetStockObject(_BLACK_BRUSH)
    if not _user32.RegisterClassW(ctypes.byref(wc)):
        # 1410 == ERROR_CLASS_ALREADY_EXISTS: fine, reuse it
        err = ctypes.get_last_error()
        if err not in (0, 1410):
            raise ctypes.WinError(err)
    _class_registered = True


class MpvWindow:
    """Owns a borderless native window for mpv to render into via ``wid``."""

    def __init__(self):
        _ensure_class()
        hinst = _kernel32.GetModuleHandleW(None)
        # Created hidden: visibility is driven by the GTK window so it never
        # flashes at its default position while idle.
        self._hwnd = _user32.CreateWindowExW(
            _WS_EX_TOOLWINDOW | _WS_EX_NOACTIVATE, _CLASS_NAME, "Cine Video",
            _WS_POPUP,
            0, 0, 320, 200,
            None, None, hinst, None,
        )
        if not self._hwnd:
            raise ctypes.WinError(ctypes.get_last_error())
        if _dwmapi is not None:
            try:
                pref = ctypes.c_int(_DWMWCP_ROUND)
                _dwmapi.DwmSetWindowAttribute(
                    self._hwnd, _DWMWA_WINDOW_CORNER_PREFERENCE,
                    ctypes.byref(pref), ctypes.sizeof(pref),
                )
            except Exception:
                pass

    @property
    def hwnd(self) -> int:
        """The native window handle to hand mpv as ``wid``."""
        return int(self._hwnd)

    def place(self, parent_hwnd, x, y, w, h):
        """Position/size the window at a screen rect, stacked behind ``parent_hwnd``."""
        if not self._hwnd:
            return
        _user32.SetWindowPos(
            self._hwnd, wintypes.HWND(parent_hwnd),
            int(x), int(y), max(int(w), 1), max(int(h), 1),
            _SWP_NOACTIVATE,
        )

    def stack_behind(self, parent_hwnd):
        """Re-stack directly behind ``parent_hwnd`` without moving/resizing."""
        if not self._hwnd:
            return
        _user32.SetWindowPos(
            self._hwnd, wintypes.HWND(parent_hwnd),
            0, 0, 0, 0,
            _SWP_NOACTIVATE | _SWP_NOSIZE | _SWP_NOMOVE,
        )

    def show(self):
        if self._hwnd:
            _user32.ShowWindow(self._hwnd, _SW_SHOWNA)

    def hide(self):
        if self._hwnd:
            _user32.ShowWindow(self._hwnd, _SW_HIDE)

    def destroy(self):
        if self._hwnd:
            _user32.DestroyWindow(self._hwnd)
            self._hwnd = None
