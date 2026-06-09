# CineWindows Setup Script
# Run this in PowerShell as Administrator

$ErrorActionPreference = "Stop"
$CINE_DIR = $PSScriptRoot

Write-Host "=== CineWindows Setup ===" -ForegroundColor Cyan
Write-Host ""

# 1. Check if MSYS2 is installed
$MSYS2_PATH = "C:\msys64"
$MINGW64_PATH = "$MSYS2_PATH\mingw64"

if (-not (Test-Path "$MSYS2_PATH\usr\bin\bash.exe")) {
    Write-Host "[1/5] Installing MSYS2..." -ForegroundColor Yellow
    $msys2Installer = "$env:TEMP\msys2-installer.exe"
    # Fetch latest MSYS2 installer
    $latest = Invoke-RestMethod -Uri "https://api.github.com/repos/msys2/msys2-installer/releases/latest"
    $asset = $latest.assets | Where-Object { $_.name -like "msys2-x86_64-*.exe" } | Select-Object -First 1
    if (-not $asset) {
        # Fallback to known URL
        $url = "https://github.com/msys2/msys2-installer/releases/latest/download/msys2-x86_64.exe"
    } else {
        $url = $asset.browser_download_url
    }
    Write-Host "  Downloading MSYS2 from $url ..."
    Invoke-WebRequest -Uri $url -OutFile $msys2Installer
    Start-Process -Wait -FilePath $msys2Installer -ArgumentList "--quiet --confirm-command"
    Write-Host "MSYS2 installed. You may need to restart this script after MSYS2 installs."
} else {
    Write-Host "[1/5] MSYS2 found at $MSYS2_PATH" -ForegroundColor Green
}

# 2. Update MSYS2 and install dependencies
Write-Host "[2/5] Updating MSYS2 package database..." -ForegroundColor Yellow
& "$MSYS2_PATH\usr\bin\bash.exe" -l -c "pacman -Syu --noconfirm 2>&1" | Out-Null
Write-Host "[2/5] Installing MINGW64 packages..." -ForegroundColor Yellow
$packages = @(
    "mingw-w64-x86_64-gtk4"
    "mingw-w64-x86_64-libadwaita"
    "mingw-w64-x86_64-python"
    "mingw-w64-x86_64-python-gobject"
    "mingw-w64-x86_64-python-pip"
    "mingw-w64-x86_64-mpv"
    "mingw-w64-x86_64-blueprint-compiler"
    "mingw-w64-x86_64-adwaita-icon-theme"
    "mingw-w64-x86_64-yt-dlp"
    "mingw-w64-x86_64-ffmpeg"
)
& "$MSYS2_PATH\usr\bin\bash.exe" -l -c "pacman -S --noconfirm $packages 2>&1" | Out-Null
Write-Host "[2/5] Packages installed." -ForegroundColor Green

# 3. Compile Blueprint UI files -> .ui
Write-Host "[3/5] Compiling Blueprint UI files..." -ForegroundColor Yellow
$BLUEPRINT_COMPILER = "$MINGW64_PATH\bin\blueprint-compiler.exe"
$SRC_DIR = "$CINE_DIR\src"
if (Test-Path $BLUEPRINT_COMPILER) {
    Get-ChildItem "$SRC_DIR\*.blp" | ForEach-Object {
        $blp = $_.FullName
        $ui = [System.IO.Path]::ChangeExtension($blp, ".ui")
        & $BLUEPRINT_COMPILER compile $blp --output $ui
        Write-Host "  Compiled: $($_.Name) -> $([System.IO.Path]::GetFileName($ui))"
    }
} else {
    Write-Host "  blueprint-compiler.exe not found; using checked-in .ui files." -ForegroundColor Yellow
}

# 4. Compile GResource bundle
Write-Host "[4/5] Compiling GResource bundle..." -ForegroundColor Yellow
$GLIB_COMPILE_RESOURCES = "$MINGW64_PATH\bin\glib-compile-resources.exe"
& $GLIB_COMPILE_RESOURCES "$SRC_DIR\cine.gresource.xml" --target="$SRC_DIR\cine.gresource" --sourcedir="$SRC_DIR"
Write-Host "[4/5] GResource bundle created." -ForegroundColor Green

# 5. Compile GSettings schema
Write-Host "[5/5] Compiling GSettings schema..." -ForegroundColor Yellow
$GLIB_COMPILE_SCHEMAS = "$MINGW64_PATH\bin\glib-compile-schemas.exe"
$SCHEMA_DIR = "$CINE_DIR\data"
& $GLIB_COMPILE_SCHEMAS $SCHEMA_DIR
Write-Host "[5/5] GSettings schema compiled." -ForegroundColor Green

Write-Host ""
Write-Host "=== Setup Complete ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Quick launch:"
Write-Host "  run_cine.bat          (double-click from Explorer)"
Write-Host "  .\cine.ps1            (from PowerShell)"
Write-Host ""
Write-Host "Or from MINGW64 terminal:"
Write-Host "  cd '$CINE_DIR'"
Write-Host "  python start_cine.py"
Write-Host ""
Write-Host "Build release package:"
Write-Host "  .\build_windows.ps1"
