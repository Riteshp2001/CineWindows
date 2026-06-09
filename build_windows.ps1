param(
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
$DistDir = Join-Path $Root "dist\CineWindows"

$MingwDir = ""
if ($env:MINGW_PREFIX) {
    $cygPath = if (Test-Path "C:\msys64\usr\bin\cygpath.exe") { "C:\msys64\usr\bin\cygpath.exe" }
              elseif (Test-Path "C:\msys2\usr\bin\cygpath.exe") { "C:\msys2\usr\bin\cygpath.exe" }
              else { $null }
    if ($cygPath) { $MingwDir = & $cygPath -w "$env:MINGW_PREFIX" 2>$null }
}
if (-not $MingwDir -or -not (Test-Path "$MingwDir\bin\python.exe")) {
    if (Test-Path "C:\msys64\mingw64\bin\python.exe") { $MingwDir = "C:\msys64\mingw64" }
    elseif (Test-Path "C:\msys2\mingw64\bin\python.exe") { $MingwDir = "C:\msys2\mingw64" }
    else { throw "MSYS2 MINGW64 Python was not found. Run setup_windows.ps1 first." }
}

$BinDir = Join-Path $MingwDir "bin"
$env:PATH = "$BinDir;$env:PATH"

# 1. Compile Blueprint UI files -> .ui
if (Test-Path (Join-Path $BinDir "blueprint-compiler.exe")) {
    Get-ChildItem (Join-Path $Root "src\*.blp") | ForEach-Object {
        $ui = [System.IO.Path]::ChangeExtension($_.FullName, ".ui")
        & (Join-Path $BinDir "blueprint-compiler.exe") compile $_.FullName --output $ui
    }
} else {
    Write-Host "blueprint-compiler.exe not found; using checked-in .ui files." -ForegroundColor Yellow
}

# 2. Compile GResource
& (Join-Path $BinDir "glib-compile-resources.exe") `
    (Join-Path $Root "src\cine.gresource.xml") `
    "--target=$(Join-Path $Root 'src\cine.gresource')" `
    "--sourcedir=$(Join-Path $Root 'src')"

# 3. Compile GSettings schemas
& (Join-Path $BinDir "glib-compile-schemas.exe") (Join-Path $Root "data")

# 4. Generate Windows icon (.ico)
Write-Host "[Icon] Generating CineWindows.ico from SVG..." -ForegroundColor Yellow
Push-Location $Root
try {
    & (Join-Path $BinDir "python.exe") create_icon.py
} catch {
    Write-Host "[Icon] Warning: Icon generation failed: $_" -ForegroundColor Yellow
    if (-not (Test-Path (Join-Path $Root "CineWindows.ico"))) {
        Write-Host "[Icon] No icon available; will compile launcher without icon." -ForegroundColor Yellow
    }
} finally {
    Pop-Location
}

# 5. Compile launcher with icon resources
if (Test-Path (Join-Path $BinDir "gcc.exe")) {
    $icoPath = Join-Path $Root "CineWindows.ico"
    $rcPath = Join-Path $Root "src\launcher.rc"
    $resPath = Join-Path $Root "src\launcher.o"

    if ((Test-Path $icoPath) -and (Test-Path (Join-Path $BinDir "windres.exe"))) {
        Write-Host "[Launcher] Compiling resources with windres..." -ForegroundColor Yellow
        & (Join-Path $BinDir "windres.exe") $rcPath -O coff -o $resPath
        & (Join-Path $BinDir "gcc.exe") -O2 -mwindows `
            (Join-Path $Root "src\launcher.c") `
            $resPath `
            -o (Join-Path $Root "CineWindows.exe")
    } else {
        & (Join-Path $BinDir "gcc.exe") -O2 -mwindows `
            (Join-Path $Root "src\launcher.c") `
            -o (Join-Path $Root "CineWindows.exe")
    }
} else {
    Write-Host "gcc.exe not found; skipping native EXE launcher." -ForegroundColor Yellow
}

# 6. Bundle MSYS2 Python runtime and all DLLs
Write-Host "[Bundle] Bundling MSYS2 Python runtime and dependencies..." -ForegroundColor Yellow

$BundleStaging = Join-Path $Root "dist\.bundle_staging"
if (Test-Path $BundleStaging) {
    Remove-Item -LiteralPath $BundleStaging -Recurse -Force
}
New-Item -ItemType Directory -Path $BundleStaging | Out-Null
New-Item -ItemType Directory -Path (Join-Path $BundleStaging "bin") | Out-Null
New-Item -ItemType Directory -Path (Join-Path $BundleStaging "lib") | Out-Null

# Copy Python interpreter (use pythonw.exe for no terminal window)
Copy-Item (Join-Path $BinDir "python.exe") (Join-Path $BundleStaging "bin\python.exe") -Force
Copy-Item (Join-Path $BinDir "pythonw.exe") (Join-Path $BundleStaging "bin\pythonw.exe") -Force

# Find and copy all DLL dependencies using ntldd or objdump
$depsCache = @{}
function Get-DllDeps {
    param([string]$BinaryPath, [int]$Depth = 0)
    if ($Depth -gt 10 -or -not (Test-Path $BinaryPath)) { return }
    $realPath = (Resolve-Path $BinaryPath).Path
    if ($depsCache.ContainsKey($realPath)) { return }
    $depsCache[$realPath] = $true

    $dlls = @()
    if (Get-Command "ntldd" -ErrorAction SilentlyContinue) {
        try {
            $lines = & ntldd $realPath 2>$null | Out-String -Stream
            foreach ($line in $lines) {
                if ($line -match "=>\s*(.+mingw64.+\.dll)\s+\(0x") {
                    $dlls += $matches[1].Trim()
                } elseif ($line -match "=>\s*(.+\.dll)\s+\(0x") {
                    $dllPath = $matches[1].Trim()
                    if ($dllPath -match '\\mingw64\\') {
                        $dlls += $dllPath
                    }
                }
            }
        } catch {}
    } elseif (Get-Command "objdump" -ErrorAction SilentlyContinue) {
        try {
            $lines = & objdump -p $realPath 2>$null | Out-String -Stream
            $found = $false
            foreach ($line in $lines) {
                if ($line -match "^The Data Directory") { $found = $false }
                if ($line -match "^DLL Name:") { $found = $true; continue }
                if ($found -and $line -match "^\s+(.+)") {
                    $dllName = $matches[1].Trim()
                    $dllPath = Join-Path $BinDir $dllName
                    if (Test-Path $dllPath) {
                        $dlls += $dllPath
                    }
                }
            }
        } catch {}
    }

    foreach ($dll in $dlls) {
        try {
            Copy-Item -LiteralPath $dll (Join-Path $BundleStaging "bin\") -Force -ErrorAction SilentlyContinue
            Get-DllDeps -BinaryPath $dll -Depth ($Depth + 1)
        } catch {}
    }
}

Write-Host "[Bundle] Tracing DLL dependencies for python.exe..."
Get-DllDeps -BinaryPath (Join-Path $BinDir "python.exe")

$gtkDlls = @(
    "libgtk-4-1.dll", "libadwaita-1-0.dll",
    "libgio-2.0-0.dll", "libglib-2.0-0.dll", "libgobject-2.0-0.dll", "libgmodule-2.0-0.dll", "libgirepository-2.0-0.dll",
    "libpango-1.0-0.dll", "libpangocairo-1.0-0.dll", "libpangoft2-1.0-0.dll", "libpangowin32-1.0-0.dll",
    "libcairo-2.dll", "libcairo-gobject-2.dll", "libcairo-script-interpreter-2.dll",
    "libgdk_pixbuf-2.0-0.dll",
    "libharfbuzz-0.dll", "libharfbuzz-gobject-0.dll", "libharfbuzz-subset-0.dll",
    "libfribidi-0.dll", "libpcre2-8-0.dll",
    "libepoxy-0.dll", "libmpv-2.dll", "libpython3*.dll",
    "libffi-8.dll", "libintl-8.dll", "libiconv-2.dll",
    "libfontconfig-1.dll", "libfreetype-6.dll", "libpng16*.dll",
    "libjpeg*.dll", "libtiff*.dll",
    "zlib1.dll", "libbz2-1.dll", "liblzma-5.dll",
    "libstdc++-6.dll", "libgcc_s_seh-1.dll", "libwinpthread-1.dll",
    "libsqlite3-0.dll", "libexpat-1.dll",
    "libgraphene-1.0-0.dll", "libtasn1-6.dll",
    "libdatrie-1.dll", "libthai-0.dll",
    "libxml2-2.dll", "libp11-kit-0.dll",
    "libcurl-4.dll", "libnghttp2-14.dll", "libidn2-0.dll",
    "libpsl-5.dll", "libssh2-1.dll", "libcares-2.dll",
    "libbrotlidec.dll", "libbrotlicommon.dll",
    "libgcrypt-20.dll", "libgpg-error-0.dll",
    "libfftw3-3.dll", "libltdl-7.dll",
    "librsvg-2-2.dll", "vulkan-1.dll",
    "libgst*-1.0-0.dll"
)

foreach ($pattern in $gtkDlls) {
    Get-ChildItem -Path $BinDir -Filter $pattern -ErrorAction SilentlyContinue | ForEach-Object {
        $dest = Join-Path $BundleStaging "bin\"
        if (-not (Test-Path (Join-Path $dest $_.Name))) {
            Copy-Item $_.FullName $dest -Force -ErrorAction SilentlyContinue
            Get-DllDeps -BinaryPath $_.FullName
        }
    }
}

# Copy Python standard library
$pyVer = (& python -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')" 2>$null)
if (-not $pyVer) {
    $pyVer = "3.13"
}
$pyLib = Join-Path $MingwDir "lib\python$pyVer"
$pyLibDest = Join-Path $BundleStaging "lib\python$pyVer"
if (Test-Path $pyLib) {
    Write-Host "[Bundle] Copying Python standard library (this may take a moment)..."
    New-Item -ItemType Directory -Path (Join-Path $BundleStaging "lib") -Force | Out-Null
    Copy-Item -Path $pyLib -Destination $pyLibDest -Recurse -Force -ErrorAction SilentlyContinue
}

# Copy GObject introspection typelib files
$typelibDir = Join-Path $MingwDir "lib\girepository-1.0"
if (Test-Path $typelibDir) {
    $typelibDest = Join-Path $BundleStaging "lib\girepository-1.0"
    New-Item -ItemType Directory -Path $typelibDest -Force | Out-Null
    Copy-Item -Path "$typelibDir\*.typelib" -Destination $typelibDest -Force -ErrorAction SilentlyContinue
}

# Copy GTK4/GIO modules
$gioModules = Join-Path $MingwDir "lib\gio\modules"
if (Test-Path $gioModules) {
    $gioDest = Join-Path $BundleStaging "lib\gio\modules"
    New-Item -ItemType Directory -Path $gioDest -Force | Out-Null
    Copy-Item -Path "$gioModules\*.dll" -Destination $gioDest -Force -ErrorAction SilentlyContinue
}

$gdkPixbufLoaders = Join-Path $MingwDir "lib\gdk-pixbuf-2.0"
if (Test-Path $gdkPixbufLoaders) {
    $pixbufDest = Join-Path $BundleStaging "lib\gdk-pixbuf-2.0"
    Copy-Item -Path $gdkPixbufLoaders -Destination $pixbufDest -Recurse -Force -ErrorAction SilentlyContinue
}

$gtk4Icons = Join-Path $MingwDir "share\icons\Adwaita"
$iconsDest = Join-Path $BundleStaging "share\icons\Adwaita"
if (Test-Path $gtk4Icons) {
    New-Item -ItemType Directory -Path (Join-Path $BundleStaging "share\icons") -Force | Out-Null
    Copy-Item -Path $gtk4Icons -Destination $iconsDest -Recurse -Force -ErrorAction SilentlyContinue
}

# Copy yt-dlp
$ytdlp = Join-Path $BinDir "yt-dlp.exe"
if (Test-Path $ytdlp) {
    Copy-Item $ytdlp (Join-Path $BundleStaging "bin\yt-dlp.exe") -Force
    Get-DllDeps -BinaryPath $ytdlp
}

Copy-Item (Join-Path $BinDir "ffmpeg.exe") (Join-Path $BundleStaging "bin\ffmpeg.exe") -Force -ErrorAction SilentlyContinue
Copy-Item (Join-Path $BinDir "ffprobe.exe") (Join-Path $BundleStaging "bin\ffprobe.exe") -Force -ErrorAction SilentlyContinue

$gtkModules = @("libspell-1.dll", "libenchant-2.dll")  # optional
foreach ($mod in $gtkModules) {
    $modPath = Join-Path $BinDir $mod
    if (Test-Path $modPath) {
        Copy-Item $modPath (Join-Path $BundleStaging "bin\") -Force -ErrorAction SilentlyContinue
        Get-DllDeps -BinaryPath $modPath
    }
}

# Copy GTK4 DLLs that don't get traced (loaded at runtime via gi)
$runtimeLoadDlls = @("libgst*-1.0-0.dll", "vulkan-1.dll")
foreach ($pattern in $runtimeLoadDlls) {
    Get-ChildItem -Path $BinDir -Filter $pattern -ErrorAction SilentlyContinue | ForEach-Object {
        $dest = Join-Path $BundleStaging "bin\"
        if (-not (Test-Path (Join-Path $dest $_.Name))) {
            Copy-Item $_.FullName $dest -Force -ErrorAction SilentlyContinue
        }
    }
}

Write-Host "[Bundle] Bundle size: $((Get-ChildItem -Recurse $BundleStaging | Measure-Object -Property Length -Sum).Sum / 1MB -as [int]) MB"

# 7. Copy everything to dist
if (Test-Path $DistDir) {
    Remove-Item -LiteralPath $DistDir -Recurse -Force
}

New-Item -ItemType Directory -Path $DistDir | Out-Null
Copy-Item -Path (Join-Path $Root "src") -Destination $DistDir -Recurse
Copy-Item -Path (Join-Path $Root "data") -Destination $DistDir -Recurse
Copy-Item -Path (Join-Path $Root "start_cine.py") -Destination $DistDir
# requirements.txt not needed at runtime

# Copy runtime bundle from staging
Move-Item -Path $BundleStaging -Destination (Join-Path $DistDir "runtime") -Force

# Clean up dist - remove dev and unnecessary files
Write-Host "[Cleanup] Removing development files from dist..." -ForegroundColor Yellow
$cleanupPatterns = @(
    "__pycache__", "*.blp", "*.pyc", "*.pyo",
    "meson.build", "*.desktop.in", "*.service.in", "*.metainfo.xml.in",
    "launcher.c", "launcher.rc", "launcher.o",
    "cine.in", "*.pot", "POTFILES.in", "LINGUAS",
    "*.blp~", "*.ui~"
)
foreach ($pattern in $cleanupPatterns) {
    Get-ChildItem -Path $DistDir -Recurse -Include $pattern -Force -ErrorAction SilentlyContinue |
        Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
}
# Remove empty po directory if present
$poDir = Join-Path $DistDir "po"
if (Test-Path $poDir) {
    Remove-Item -LiteralPath $poDir -Recurse -Force -ErrorAction SilentlyContinue
}
# Remove .gitkeep/empty dirs
Get-ChildItem -Path $DistDir -Directory -Recurse -ErrorAction SilentlyContinue |
    Where-Object { (Get-ChildItem $_.FullName -Force -ErrorAction SilentlyContinue).Count -eq 0 } |
    Remove-Item -Force -ErrorAction SilentlyContinue

$readmeSrc = Join-Path $Root "README-Windows.md"
if (-not (Test-Path $readmeSrc)) {
    $readmeSrc = Join-Path $Root "README.md"
}
Copy-Item -Path $readmeSrc -Destination (Join-Path $DistDir "README.txt")

if (Test-Path (Join-Path $Root "CineWindows.exe")) {
    Copy-Item -Path (Join-Path $Root "CineWindows.exe") -Destination $DistDir
}
if (Test-Path (Join-Path $Root "CineWindows.ico")) {
    Copy-Item -Path (Join-Path $Root "CineWindows.ico") -Destination $DistDir
}

# 8. Pre-compile Python bytecode for faster startup
Write-Host "[Optimize] Pre-compiling Python bytecode..." -ForegroundColor Yellow
& (Join-Path $BinDir "python.exe") -m compileall -q -f "$DistDir\src" "$DistDir\start_cine.py" 2>&1 | Out-Null

Write-Host "Built $Configuration package at $DistDir" -ForegroundColor Green
