import locale
import os
import sys

locale.setlocale(locale.LC_NUMERIC, "C")

project_root = os.path.dirname(os.path.abspath(__file__))

# Add bundled runtime bin to PATH so DLLs can be found
runtime_bin = os.path.join(project_root, "runtime", "bin")
runtime_root = os.path.join(project_root, "runtime")
if os.path.exists(os.path.join(runtime_bin, "pythonw.exe")) or os.path.exists(os.path.join(runtime_bin, "python.exe")):
    os.environ["PATH"] = runtime_bin + os.pathsep + os.environ.get("PATH", "")
    os.environ["PYTHONHOME"] = runtime_root
    # GObject introspection
    typelib = os.path.join(project_root, "runtime", "lib", "girepository-1.0")
    if os.path.isdir(typelib):
        os.environ["GI_TYPELIB_PATH"] = typelib
    # GIO modules
    gio_mod = os.path.join(project_root, "runtime", "lib", "gio", "modules")
    if os.path.isdir(gio_mod):
        os.environ["PATH"] = gio_mod + os.pathsep + os.environ.get("PATH", "")
    # Icon theme
    share = os.path.join(project_root, "runtime", "share")
    if os.path.isdir(share):
        os.environ["XDG_DATA_DIRS"] = share
else:
    # Fallback: try MSYS2 MINGW64
    mingw_bin = r"C:\msys64\mingw64\bin"
    if os.path.exists(mingw_bin) and mingw_bin not in os.environ.get("PATH", ""):
        os.environ["PATH"] = mingw_bin + os.pathsep + os.environ.get("PATH", "")
    mingw_share = r"C:\msys64\mingw64\share"
    if os.path.exists(mingw_share):
        os.environ["XDG_DATA_DIRS"] = mingw_share + os.pathsep + os.environ.get("XDG_DATA_DIRS", "")

# Ensure the cine package can be imported
src_dir = os.path.join(project_root, "src")
if os.path.isdir(src_dir) and src_dir not in sys.path:
    sys.path.insert(0, src_dir)
if project_root not in sys.path:
    sys.path.insert(0, project_root)

import gi

gi.require_version("Gio", "2.0")
from gi.repository import Gio

gresource_path = os.path.join(project_root, "src", "cine.gresource")
if os.path.exists(gresource_path):
    resource = Gio.Resource.load(gresource_path)
    resource._register()

schema_dir = os.path.join(project_root, "data")
if os.path.exists(os.path.join(schema_dir, "gschemas.compiled")):
    os.environ.setdefault("GSETTINGS_SCHEMA_DIR", schema_dir)

try:
    from cine import main
    from cine.utils.constants import APP_VERSION
except ModuleNotFoundError:
    from src.controllers import main_controller as main
    from src.utils.constants import APP_VERSION

sys.exit(main.main(APP_VERSION))
