<#
.SYNOPSIS
    Launches CineWindows on Windows using bundled or MSYS2 Python runtime.
.DESCRIPTION
    Uses bundled runtime first (from \runtime\bin\), falls back to MSYS2 MINGW64.
    Supports passing video files as arguments.
.PARAMETER FilePath
    One or more video files to open.
.EXAMPLE
    .\cine.ps1
    .\cine.ps1 "D:\videos\movie.mp4"
    .\cine.ps1 "D:\videos\*.mp4"
#>

param(
    [Parameter(Position=0, ValueFromRemainingArguments=$true)]
    [string[]]$FilePath
)

$ErrorActionPreference = "Stop"
$CineDir = Split-Path -Parent $PSCommandPath
$BundledPython = "$CineDir\runtime\bin\python.exe"

if (Test-Path $BundledPython) {
    $env:PATH = "$CineDir\runtime\bin;$env:PATH"
    $env:GSETTINGS_SCHEMA_DIR = "$CineDir\data"
    $env:LANG = "C"
    $env:LC_ALL = "C"
    $python = $BundledPython
} else {
    $Msys2Dir = "C:\msys64"
    $MingwDir = "$Msys2Dir\mingw64"
    if (-not (Test-Path "$MingwDir\bin\python.exe")) {
        Write-Error "No Python runtime found. Please reinstall CineWindows using the official installer."
        exit 1
    }
    $env:PATH = "$MingwDir\bin;$env:PATH"
    $env:GSETTINGS_SCHEMA_DIR = "$CineDir\data"
    $env:LANG = "C"
    $env:LC_ALL = "C"
    $python = "$MingwDir\bin\python.exe"
}

$script = "$CineDir\start_cine.py"

if ($FilePath.Count -gt 0) {
    & $python $script @FilePath
} else {
    & $python $script
}

if ($LASTEXITCODE -ne 0) {
    Write-Host "CineWindows exited with code $LASTEXITCODE" -ForegroundColor Red
    Read-Host "Press Enter to exit"
}
