# CineWindows - Video Player
# Copyright (c) 2026 Ritesh Pandit
#
# CineWindows Community License
#
# This source code is made available for personal, non-commercial
# use only. Organizations may not use, copy, modify, or distribute
# this code without written permission from Ritesh Pandit.
#
# See the LICENSE.md file for full license terms.
#
# Project: CineWindows
# Author:  Ritesh Pandit
# Last modified: 2026-09-10
# Modified by: Ritesh Pandit
# Purpose: MpvQt library bootstrap script for Windows
#
param(
    [string]$QtRoot = "C:\Qt\6.11.1\mingw_64",
    [string]$QtCompilerBin = "C:\Qt\Tools\mingw1310_64\bin",
    [string]$MsysRoot = "C:\msys64",
    [string]$MsysPrefix = "C:\msys64\mingw64",
    [string]$MpvQtTag = "v1.2.0",
    [string]$InstallPrefix = ".deps\mpvqt-install",
    [string]$StaticLibmpvPrefix = ".deps\libmpv-static",
    [switch]$SkipMsysPackageInstall
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$installPath = if ([IO.Path]::IsPathRooted($InstallPrefix)) {
    $InstallPrefix
} else {
    Join-Path $repoRoot $InstallPrefix
}
$staticLibmpvPath = if ([IO.Path]::IsPathRooted($StaticLibmpvPrefix)) {
    $StaticLibmpvPrefix
} else {
    Join-Path $repoRoot $StaticLibmpvPrefix
}

function Add-PathPrefix {
    param([string[]]$Paths)
    $existing = $env:PATH -split ';'
    $next = @()
    foreach ($path in $Paths) {
        if (-not [string]::IsNullOrWhiteSpace($path) -and (Test-Path $path)) {
            $next += (Resolve-Path $path).Path
        }
    }
    $env:PATH = (($next + $existing) | Select-Object -Unique) -join ';'
}

function Find-Tool {
    param([string]$Name, [string[]]$Candidates = @())
    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path $candidate)) {
            return (Resolve-Path $candidate).Path
        }
    }
    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }
    return $null
}

function Invoke-Native {
    param(
        [string]$FilePath,
        [string[]]$Arguments
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$FilePath failed with exit code $LASTEXITCODE"
    }
}

function Package-Prefix {
    param([string]$PrefixPath)
    if ($PrefixPath -match 'ucrt64') { return 'mingw-w64-ucrt-x86_64' }
    if ($PrefixPath -match 'clang64') { return 'mingw-w64-clang-x86_64' }
    return 'mingw-w64-x86_64'
}

$qtBin = Join-Path $QtRoot "bin"
$cmakeBin = "C:\Qt\Tools\CMake_64\bin"
$ninjaBin = "C:\Qt\Tools\Ninja"
$msysBin = Join-Path $MsysRoot "usr\bin"
$msysMingwBin = Join-Path $MsysPrefix "bin"

Add-PathPrefix @($cmakeBin, $ninjaBin, $QtCompilerBin, $qtBin, $msysMingwBin, $msysBin)

$qtCmake = Find-Tool "qt-cmake.bat" @((Join-Path $qtBin "qt-cmake.bat"))
$cmake = Find-Tool "cmake.exe" @((Join-Path $cmakeBin "cmake.exe"))
$ninja = Find-Tool "ninja.exe" @((Join-Path $ninjaBin "ninja.exe"))
$git = Find-Tool "git.exe"
$pacman = Find-Tool "pacman.exe" @((Join-Path $msysBin "pacman.exe"))
$gcc = Find-Tool "gcc.exe" @((Join-Path $QtCompilerBin "gcc.exe"))
$gxx = Find-Tool "g++.exe" @((Join-Path $QtCompilerBin "g++.exe"))

foreach ($tool in @(
    @{Name = "qt-cmake"; Value = $qtCmake},
    @{Name = "cmake"; Value = $cmake},
    @{Name = "ninja"; Value = $ninja},
    @{Name = "git"; Value = $git},
    @{Name = "gcc"; Value = $gcc},
    @{Name = "g++"; Value = $gxx}
)) {
    if (-not $tool.Value) {
        throw "$($tool.Name) was not found. Check QtRoot/QtCompilerBin or install the tool."
    }
}

if (-not $SkipMsysPackageInstall) {
    if (-not $pacman) {
        throw "pacman.exe was not found. Pass -SkipMsysPackageInstall after installing ECM, pkgconf, and mpv development files yourself."
    }

    $pkgPrefix = Package-Prefix $MsysPrefix
    Invoke-Native $pacman @(
        "-S", "--needed", "--noconfirm",
        "$pkgPrefix-extra-cmake-modules",
        "$pkgPrefix-pkgconf"
    )
}

$ecmConfig = Get-ChildItem -Path $MsysPrefix -Recurse -Filter ECMConfig.cmake -ErrorAction SilentlyContinue |
    Select-Object -First 1
if (-not $ecmConfig) {
    if (-not $pacman) {
        throw "ECMConfig.cmake was not found and pacman is not available to install it. Install extra-cmake-modules manually."
    }
    Write-Warning "ECMConfig.cmake not found under $MsysPrefix. Installing extra-cmake-modules..."
    $pkgPrefix = Package-Prefix $MsysPrefix
    Invoke-Native $pacman @("-S", "--noconfirm", "$pkgPrefix-extra-cmake-modules")
    $ecmConfig = Get-ChildItem -Path $MsysPrefix -Recurse -Filter ECMConfig.cmake -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if (-not $ecmConfig) {
        throw "ECMConfig.cmake still not found after installing $pkgPrefix-extra-cmake-modules."
    }
}

$staticMpvHeader = Join-Path $staticLibmpvPath "include\mpv\client.h"
$staticMpvImportLib = Join-Path $staticLibmpvPath "lib\libmpv.dll.a"
$useStaticLibmpv = (Test-Path $staticMpvHeader) -and (Test-Path $staticMpvImportLib)
$mpvRoot = if ($useStaticLibmpv) { $staticLibmpvPath } else { $MsysPrefix }
$mpvHeader = Join-Path $mpvRoot "include\mpv\client.h"
$mpvImportLib = Join-Path $mpvRoot "lib\libmpv.dll.a"
if (-not (Test-Path $mpvHeader) -or -not (Test-Path $mpvImportLib)) {
    if (-not $SkipMsysPackageInstall -and $pacman) {
        Write-Warning "libmpv development files not found under $MsysPrefix. Installing MSYS2 mpv package..."
        $pkgPrefix = Package-Prefix $MsysPrefix
        Invoke-Native $pacman @("-S", "--noconfirm", "$pkgPrefix-mpv")
        if (-not (Test-Path $mpvHeader) -or -not (Test-Path $mpvImportLib)) {
            throw "libmpv development files still not found after installing $pkgPrefix-mpv."
        }
    } else {
        throw "libmpv development files were not found under $MsysPrefix. Install the MSYS2 mpv package."
    }
}
if ($useStaticLibmpv) {
    Write-Host "Using statically linked libmpv from $staticLibmpvPath"
} else {
    Write-Warning "Using the MSYS2 shared libmpv package. Run scripts\setup\bootstrap_libmpv_windows.ps1 for a smaller release bundle."
}

$env:PKG_CONFIG_PATH = (Join-Path $MsysPrefix "lib\pkgconfig")
$env:PKG_CONFIG_LIBDIR = "$($env:PKG_CONFIG_PATH);$(Join-Path $MsysPrefix 'share\pkgconfig')"

$depsDir = Join-Path $repoRoot ".deps"
$srcDir = Join-Path $depsDir "mpvqt-src"
$buildDir = Join-Path $depsDir "mpvqt-build"
New-Item -ItemType Directory -Path $depsDir -Force | Out-Null

if (-not (Test-Path $srcDir)) {
    Invoke-Native $git @("clone", "--branch", $MpvQtTag, "--depth", "1", "https://github.com/KDE/mpvqt.git", $srcDir)
} else {
    Invoke-Native $git @("-C", $srcDir, "fetch", "--tags", "--depth", "1", "origin", $MpvQtTag)
    Invoke-Native $git @("-C", $srcDir, "checkout", $MpvQtTag)
}

# Q_STATIC_LOGGING_CATEGORY was introduced in Qt 6.9.
# Qt 6.8 uses Q_LOGGING_CATEGORY instead — patch the source.
$mpvRendererFile = Join-Path $srcDir "src/mpvrenderer.cpp"
if (Test-Path $mpvRendererFile) {
    $content = Get-Content $mpvRendererFile -Raw
    if ($content -match 'Q_STATIC_LOGGING_CATEGORY') {
        $content = $content -replace 'Q_STATIC_LOGGING_CATEGORY', 'static Q_LOGGING_CATEGORY'
        Set-Content -Path $mpvRendererFile -Value $content -NoNewline
        Write-Host "Patched mpvrenderer.cpp: Q_STATIC_LOGGING_CATEGORY -> static Q_LOGGING_CATEGORY (Qt 6.8 compat)"
    }
}

if (Test-Path $buildDir) {
    Remove-Item -LiteralPath $buildDir -Recurse -Force
}

$mpvIncludeRoot = Join-Path $mpvRoot "include"

Invoke-Native $qtCmake @(
    "-S", $srcDir,
    "-B", $buildDir,
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_INSTALL_PREFIX=$installPath",
    "-DCMAKE_C_COMPILER=$gcc",
    "-DCMAKE_CXX_COMPILER=$gxx",
    "-DCMAKE_PREFIX_PATH=$QtRoot;$mpvRoot;$MsysPrefix",
    "-DECM_DIR=$((Resolve-Path (Split-Path $ecmConfig.FullName)).Path)",
    "-DLibmpv_INCLUDE_DIRS=$mpvIncludeRoot",
    "-DLibmpv_LIBRARIES=$mpvImportLib"
)

Invoke-Native $cmake @("--build", $buildDir, "--config", "Release")
Invoke-Native $cmake @("--install", $buildDir, "--config", "Release")

$mpvQtConfig = Join-Path $installPath "lib\cmake\MpvQt\MpvQtConfig.cmake"
if (-not (Test-Path $mpvQtConfig)) {
    throw "MpvQt build finished, but $mpvQtConfig was not created."
}

Write-Host "MpvQt installed."
Write-Host "MpvQt_DIR=$((Resolve-Path (Split-Path $mpvQtConfig)).Path)"
Write-Host "Qt Creator can now configure this project without extra MpvQt arguments."
