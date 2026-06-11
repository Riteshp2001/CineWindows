import struct
import subprocess
import os
import shutil
import tempfile

SVG_PATH = "data/icons/hicolor/scalable/apps/io.github.gyrolet.CineWindows.svg"
ICO_PATH = "CineWindows.ico"
SIZES = [16, 20, 24, 28, 32, 40, 48, 64, 96, 128, 256]

def svg_to_png(svg_path, size, output_path):
    try:
        subprocess.run(
            ["rsvg-convert", "-w", str(size), "-h", str(size), "-o", output_path, svg_path],
            check=True, capture_output=True, timeout=30
        )
        return True
    except (subprocess.SubprocessError, FileNotFoundError):
        pass
    try:
        subprocess.run(
            ["inkscape", f"--export-filename={output_path}", f"--export-width={size}", f"--export-height={size}", svg_path],
            check=True, capture_output=True, timeout=30
        )
        return True
    except (subprocess.SubprocessError, FileNotFoundError):
        pass
    try:
        subprocess.run(
            ["magick", "convert", "-background", "none", "-size", f"{size}x{size}", svg_path, output_path],
            check=True, capture_output=True, timeout=30
        )
        return True
    except (subprocess.SubprocessError, FileNotFoundError):
        pass
    return False

def pack_ico(png_files, output_path):
    count = len(png_files)
    header_size = 6
    dir_entry_size = 16

    with open(output_path, "wb") as f:
        f.write(struct.pack("<HHH", 0, 1, count))
        offset = header_size + dir_entry_size * count
        for png_path, size in png_files:
            with open(png_path, "rb") as pf:
                data = pf.read()
            w = size if size < 256 else 0
            h = size if size < 256 else 0
            f.write(struct.pack(
                "<BBBBHHII",
                w, h, 0, 0, 1, 32, len(data), offset
            ))
            offset += len(data)
        for png_path, _ in png_files:
            with open(png_path, "rb") as pf:
                f.write(pf.read())

def main():
    tmpdir = tempfile.mkdtemp()
    try:
        png_files = []
        for size in SIZES:
            out = os.path.join(tmpdir, f"icon_{size}.png")
            if svg_to_png(SVG_PATH, size, out):
                png_files.append((out, size))
                print(f"  Rendered {size}x{size}")
            else:
                print(f"  Warning: Could not render {size}x{size}")

        if not png_files:
            fallback_png = "data/icons/hicolor/scalable/apps/io.github.gyrolet.CineWindows.png"
            if os.path.exists(fallback_png):
                print(f"  Using fallback PNG for 32x32")
                png_files.append((fallback_png, 32))
            else:
                print("  ERROR: No icon source available!")
                return 1

        pack_ico(png_files, ICO_PATH)
        print(f"Created {ICO_PATH} ({len(png_files)} sizes)")

        for png_path, _ in png_files:
            if os.path.dirname(png_path) == tmpdir:
                try:
                    os.remove(png_path)
                except OSError:
                    pass
    finally:
        try:
            shutil.rmtree(tmpdir)
        except OSError:
            pass
    return 0

if __name__ == "__main__":
    exit(main())
