import ctypes
import os
import sys
import gi

from .constants import APP_DEVELOPER, APP_NAME

gi.require_version("Gdk", "4.0")
gi.require_version("GLib", "2.0")
from gi.repository import Gdk, GLib

display = Gdk.Display.get_default()

def _get_platform_config_dir():
    base = os.environ.get("APPDATA") or os.path.expanduser("~")
    return os.path.join(base, APP_DEVELOPER, APP_NAME)

def _get_mpv_config_dir():
    # Use Cine's own isolated mpv config dir (%APPDATA%\cine) instead of the
    # shared global mpv config (%APPDATA%\mpv), so Cine doesn't inherit the
    # user's standalone mpv.conf and vice versa.
    base = os.environ.get("APPDATA") or os.path.expanduser("~")
    return os.path.join(base, "cine")

def _get_platform_pictures_dir():
    profile = os.environ.get("USERPROFILE") or os.path.expanduser("~")
    pictures = os.path.join(profile, "Pictures")
    return pictures if os.path.isdir(pictures) else profile

def _get_platform_screenshots_dir():
    pics = _get_platform_pictures_dir()
    return os.path.join(pics, "Cine Screenshots")

PLAYLIST_DIR = os.path.join(_get_platform_config_dir(), "last-playlist")
LAST_PLAYLIST_FILE = os.path.join(PLAYLIST_DIR, "last-playlist.m3u8")
MPV_CONFIG_DIR = _get_mpv_config_dir()

os.makedirs(_get_platform_config_dir(), exist_ok=True)
os.makedirs(MPV_CONFIG_DIR, exist_ok=True)
os.makedirs(PLAYLIST_DIR, exist_ok=True)

MPV_WATCH_LATER_DIR = os.path.join(MPV_CONFIG_DIR, "watch_later")
MPV_SCRIPTS_DIR = os.path.join(MPV_CONFIG_DIR, "scripts")
MPV_SCRIPT_OPTS_DIR = os.path.join(MPV_CONFIG_DIR, "script-opts")

os.makedirs(MPV_WATCH_LATER_DIR, exist_ok=True)
os.makedirs(MPV_SCRIPTS_DIR, exist_ok=True)
os.makedirs(MPV_SCRIPT_OPTS_DIR, exist_ok=True)

for filename in ("mpv.conf", "input.conf"):
    path = os.path.join(MPV_CONFIG_DIR, filename)
    if not os.path.exists(path):
        open(path, "w", encoding="utf-8").close()

def get_gpu_vendor():
    if not display:
        return None
    try:
        if seat := display.get_default_seat():
            context = seat.get_display().create_gl_context()
            context.realize()
            context.make_current()
            try:
                opengl32 = ctypes.CDLL("opengl32.dll")
                glGetString = opengl32.glGetString
                glGetString.restype = ctypes.c_char_p
                glGetString.argtypes = [ctypes.c_uint]
                vendor = glGetString(0x1F00).decode("utf-8").lower()
                return vendor
            except Exception:
                return None
    except Exception as e:
        print(f"get_gpu_vendor error: {e}")
        return None

def _epoxy_get_proc_addr(epoxy):
    fn = epoxy.epoxy_get_proc_address
    fn.restype = ctypes.c_void_p
    fn.argtypes = [ctypes.c_char_p]
    return fn

def load_egl():
    try:
        epoxy = ctypes.CDLL("libepoxy-0.dll")
        prim = _epoxy_get_proc_addr(epoxy)
        return epoxy, _make_proc_address_with_fallback(prim)
    except OSError:
        pass
    try:
        egl = ctypes.CDLL("libEGL.dll")
        egl_get_proc = egl.eglGetProcAddress
        egl_get_proc.restype = ctypes.c_void_p
        egl_get_proc.argtypes = [ctypes.c_char_p]
        return egl, _make_proc_address_with_fallback(egl_get_proc)
    except OSError:
        pass
    opengl32 = ctypes.CDLL("opengl32.dll")
    wgl_get_proc = opengl32.wglGetProcAddress
    wgl_get_proc.restype = ctypes.c_void_p
    wgl_get_proc.argtypes = [ctypes.c_char_p]
    return opengl32, _make_proc_address_with_fallback(wgl_get_proc)

def load_gl():
    gl = ctypes.CDLL("opengl32.dll")
    glGetIntegerv = gl.glGetIntegerv
    glGetIntegerv.argtypes = [ctypes.c_uint, ctypes.POINTER(ctypes.c_int)]
    return gl, glGetIntegerv

def _make_proc_address_with_fallback(primary_fn):
    try:
        opengl32 = ctypes.CDLL("opengl32.dll")
        gl_handle = opengl32._handle
        get_proc = ctypes.windll.kernel32.GetProcAddress
        get_proc.restype = ctypes.c_void_p
        get_proc.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
    except Exception:
        gl_handle = None
        get_proc = None

    def get_proc_address(name):
        ptr = primary_fn(name)
        if ptr:
            return ptr
        if gl_handle and get_proc:
            try:
                return get_proc(gl_handle, name)
            except Exception:
                pass
        return None

    return get_proc_address

GL_FRAMEBUFFER_BINDING = 0x8CA6

def get_display_param(display):
    return {}
