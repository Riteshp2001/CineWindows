import ctypes
import os
import sys
import gi

from .constants import APP_DEVELOPER, APP_NAME

gi.require_version("Gdk", "4.0")
gi.require_version("GLib", "2.0")
from gi.repository import Gdk, GLib

IS_WINDOWS = sys.platform == "win32"
IS_LINUX = not IS_WINDOWS


def _get_platform_config_dir():
    if IS_WINDOWS:
        base = os.environ.get("APPDATA") or os.path.expanduser("~")
        return os.path.join(base, APP_DEVELOPER, APP_NAME)
    return os.path.join(GLib.get_user_config_dir(), "cine")


def _find_windows_mpv_portable_config():
    if not IS_WINDOWS:
        return None

    for folder in os.environ.get("PATH", "").split(os.pathsep):
        if not folder:
            continue
        mpv_exe = os.path.join(folder, "mpv.exe")
        portable_dir = os.path.join(folder, "portable_config")
        if os.path.exists(mpv_exe) and os.path.isdir(portable_dir):
            return portable_dir
    return None


def _get_mpv_config_dir():
    if IS_WINDOWS:
        portable_dir = _find_windows_mpv_portable_config()
        if portable_dir:
            return portable_dir
        base = os.environ.get("APPDATA") or os.path.expanduser("~")
        return os.path.join(base, "mpv")
    return os.path.join(GLib.get_user_config_dir(), "mpv")


def _get_platform_pictures_dir():
    if IS_WINDOWS:
        profile = os.environ.get("USERPROFILE") or os.path.expanduser("~")
        pictures = os.path.join(profile, "Pictures")
        return pictures if os.path.isdir(pictures) else profile
    xdg = GLib.get_user_special_dir(GLib.UserDirectory.DIRECTORY_PICTURES)
    return xdg or os.path.expanduser("~/Pictures")


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

if IS_WINDOWS:
    for filename in ("mpv.conf", "input.conf"):
        path = os.path.join(MPV_CONFIG_DIR, filename)
        if not os.path.exists(path):
            open(path, "w", encoding="utf-8").close()


def get_gpu_vendor(display):
    try:
        context = display.get_default_seat().get_display().create_gl_context()
        context.realize()
        context.make_current()
        if IS_WINDOWS:
            try:
                opengl32 = ctypes.CDLL("opengl32.dll")
                glGetString = opengl32.glGetString
                glGetString.restype = ctypes.c_char_p
                glGetString.argtypes = [ctypes.c_uint]
                vendor = glGetString(0x1F00).decode("utf-8").lower()
                return vendor
            except Exception:
                return None
        else:
            libgl = ctypes.CDLL("libGL.so.1")
            glGetString = libgl.glGetString
            glGetString.restype = ctypes.c_char_p
            glGetString.argtypes = [ctypes.c_uint]
            return glGetString(0x1F00).decode("utf-8").lower()
    except Exception as e:
        print(f"get_gpu_vendor error: {e}")
        return None


def _epoxy_get_proc_addr(epoxy):
    fn = epoxy.epoxy_get_proc_address
    fn.restype = ctypes.c_void_p
    fn.argtypes = [ctypes.c_char_p]
    return fn


def load_egl():
    if IS_WINDOWS:
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
    else:
        egl = ctypes.CDLL("libEGL.so.1")
        egl_get_proc = egl.eglGetProcAddress
        egl_get_proc.restype = ctypes.c_void_p
        egl_get_proc.argtypes = [ctypes.c_char_p]
        return egl, egl_get_proc


def load_gl():
    if IS_WINDOWS:
        gl = ctypes.CDLL("opengl32.dll")
    else:
        gl = ctypes.CDLL("libGL.so.1")
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
    param = {}
    if IS_WINDOWS:
        return param
    try:
        gi.require_version("GdkWayland", "4.0")
        gi.require_version("GdkX11", "4.0")
        from gi.repository import GdkWayland, GdkX11

        def get_pointer(obj):
            ctypes.pythonapi.PyCapsule_GetPointer.restype = ctypes.c_void_p
            ctypes.pythonapi.PyCapsule_GetPointer.argtypes = (ctypes.py_object,)
            return ctypes.pythonapi.PyCapsule_GetPointer(obj.__gpointer__, None)

        if isinstance(display, GdkWayland.WaylandDisplay):
            gtk = ctypes.CDLL("libgtk-4.so.1")
            gtk.gdk_wayland_display_get_wl_display.restype = ctypes.c_void_p
            gtk.gdk_wayland_display_get_wl_display.argtypes = [ctypes.c_void_p]
            ptr = gtk.gdk_wayland_display_get_wl_display(get_pointer(display))
            if ptr:
                param["wl_display"] = ptr
        elif isinstance(display, GdkX11.X11Display):
            gtk = ctypes.CDLL("libgtk-4.so.1")
            gtk.gdk_x11_display_get_xdisplay.restype = ctypes.c_void_p
            gtk.gdk_x11_display_get_xdisplay.argtypes = [ctypes.c_void_p]
            ptr = gtk.gdk_x11_display_get_xdisplay(get_pointer(display))
            if ptr:
                param["x11_display"] = ptr
    except Exception as e:
        print(f"Display param detection: {e}")
    return param
