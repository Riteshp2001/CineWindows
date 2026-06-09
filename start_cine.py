import locale
import os
import sys

locale.setlocale(locale.LC_NUMERIC, "C")

project_root = os.path.dirname(os.path.abspath(__file__))

# Add MSYS2 MINGW64 to PATH so ctypes can find DLLs
mingw_bin = r"C:\msys64\mingw64\bin"
if os.path.exists(mingw_bin) and mingw_bin not in os.environ.get("PATH", ""):
    os.environ["PATH"] = mingw_bin + os.pathsep + os.environ.get("PATH", "")

# Ensure the cine package can be imported: try src/ and junction paths
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
except ModuleNotFoundError:
    from src import main

sys.exit(main.main("1.5.2"))
