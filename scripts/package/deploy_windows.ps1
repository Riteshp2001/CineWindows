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
#
# Project: CineWindows
# Author:  Ritesh Pandit
# Purpose: Windows deployment and packaging script
#
param(
    [string]$BuildDir = "build/release",
    [string]$Configuration = "Release",
    [string]$StageDir = "packaging/windows/stage",
    [string]$ArtifactDir = "packaging/windows/artifacts",
    [string]$MpvQtBin = ".deps/mpvqt-install/bin",
    [string]$LibmpvBin = ".deps/libmpv-static/bin",
    [string]$MediaToolsBin = ".deps/media-tools/bin",
    [string]$MsysBin = "C:\msys64\mingw64\bin",
    [string]$QtBin = "",
    [string]$Generator = "",
    [switch]$Build,
    [switch]$SkipInstaller,
    [switch]$SkipGuiStartupCheck
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$buildPath = Join-Path $repoRoot $BuildDir
$stagePath = Join-Path $repoRoot $StageDir
$artifactPath = Join-Path $repoRoot $ArtifactDir
$mpvQtBinPath = if ([IO.Path]::IsPathRooted($MpvQtBin)) { $MpvQtBin } else { Join-Path $repoRoot $MpvQtBin }
$libmpvBinPath = if ([IO.Path]::IsPathRooted($LibmpvBin)) { $LibmpvBin } else { Join-Path $repoRoot $LibmpvBin }
$mediaToolsBinPath = if ([IO.Path]::IsPathRooted($MediaToolsBin)) { $MediaToolsBin } else { Join-Path $repoRoot $MediaToolsBin }
$qmlDir = Join-Path $repoRoot "qml"
$cmakeFile = Join-Path $repoRoot "CMakeLists.txt"

if ($QtBin -and (Test-Path $QtBin)) {
    $env:PATH = "$(Resolve-Path $QtBin);$env:PATH"
}
if ($MsysBin -and (Test-Path $MsysBin)) {
    $env:PATH = "$(Resolve-Path $MsysBin);$env:PATH"
}

function Find-OnPath {
    param([string]$FileName)

    foreach ($dir in ($env:PATH -split ';')) {
        if ([string]::IsNullOrWhiteSpace($dir)) {
            continue
        }

        $candidate = Join-Path $dir $FileName
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    return $null
}

function Find-WindeployQt {
    $fromPath = Find-OnPath "windeployqt.exe"
    if ($fromPath) {
        return $fromPath
    }

    $candidates = @()
    if ($env:QT_ROOT_DIR) {
        $candidates += (Join-Path $env:QT_ROOT_DIR "bin\windeployqt.exe")
    }
    if ($env:Qt6_DIR) {
        $qtRootFromCmake = Split-Path (Split-Path (Split-Path $env:Qt6_DIR))
        $candidates += (Join-Path $qtRootFromCmake "bin\windeployqt.exe")
    }
    $candidates += Get-ChildItem -Path "C:\Qt\*\mingw_64\bin\windeployqt.exe" -ErrorAction SilentlyContinue |
        Sort-Object FullName -Descending |
        ForEach-Object { $_.FullName }

    foreach ($candidate in $candidates) {
        if ($candidate -and (Test-Path $candidate)) {
            return (Resolve-Path $candidate).Path
        }
    }

    return $null
}

if ($Build) {
    $configureArgs = @("-S", $repoRoot, "-B", $buildPath, "-DCMAKE_BUILD_TYPE=$Configuration")
    if ($Generator -and -not (Test-Path (Join-Path $buildPath "CMakeCache.txt"))) {
        $configureArgs += @("-G", $Generator)
    }
    cmake @configureArgs
    cmake --build $buildPath --config $Configuration
}

$exeCandidates = @(
    (Join-Path $buildPath "bin/$Configuration/CineWindows.exe"),
    (Join-Path $buildPath "bin/CineWindows.exe"),
    (Join-Path $buildPath "$Configuration/CineWindows.exe"),
    (Join-Path $buildPath "CineWindows.exe")
)

$sourceExe = $exeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $sourceExe) {
    throw "CineWindows.exe was not found under $buildPath. Build that directory first or pass -Build."
}

$temporarySourceExe = $null
$sourceExePath = (Resolve-Path $sourceExe).Path
$stageFullPath = [IO.Path]::GetFullPath($stagePath).TrimEnd('\')
if ($sourceExePath.StartsWith("$stageFullPath\", [StringComparison]::OrdinalIgnoreCase)) {
    $temporarySourceExe = Join-Path ([IO.Path]::GetTempPath()) "CineWindows-$([guid]::NewGuid()).exe"
    Copy-Item -LiteralPath $sourceExePath -Destination $temporarySourceExe
    $sourceExe = $temporarySourceExe
}

if (Test-Path $stagePath) {
    Remove-Item -LiteralPath $stagePath -Recurse -Force
}
New-Item -ItemType Directory -Path $stagePath | Out-Null
New-Item -ItemType Directory -Path $artifactPath -Force | Out-Null

# Strip debug symbols from binaries to reduce size
function Strip-Binaries {
    param([string]$Directory)
    $stripExe = Find-OnPath "strip.exe"
    if (-not $stripExe) { return }
    Get-ChildItem -Path $Directory -Include "*.dll","*.exe" -File -Recurse | ForEach-Object {
        & $stripExe --strip-unneeded $_.FullName 2>$null
    }
}

$stageExe = Join-Path $stagePath "CineWindows.exe"
Copy-Item -LiteralPath $sourceExe -Destination $stageExe
if ($temporarySourceExe) {
    Remove-Item -LiteralPath $temporarySourceExe -Force
}

$windeployqt = Find-WindeployQt
$adsDll = Get-ChildItem -Path @(
    (Join-Path $buildPath "x64/bin/$Configuration/*qtadvanceddocking-qt6*.dll"),
    (Join-Path $buildPath "x64/bin/*qtadvanceddocking-qt6*.dll")
) -File -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $adsDll) {
    throw "Qt Advanced Docking System DLL was not found under $buildPath/x64/bin."
}
Copy-Item -LiteralPath $adsDll.FullName -Destination $stagePath -Force
$adsLicenseDir = Join-Path $stagePath "licenses/Qt-Advanced-Docking-System"
New-Item -ItemType Directory -Path $adsLicenseDir -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $repoRoot "third_party/Qt-Advanced-Docking-System/LICENSE") -Destination $adsLicenseDir
Copy-Item -LiteralPath (Join-Path $repoRoot "third_party/Qt-Advanced-Docking-System/gnu-lgpl-v2.1.md") -Destination $adsLicenseDir
if (-not $windeployqt) {
    throw "windeployqt.exe was not found. Pass -QtBin or set QT_ROOT_DIR/Qt6_DIR."
}

$windeployArgs = @(
    "--release",
    "--compiler-runtime",
    "--no-translations",
    "--skip-plugin-types", "qmltooling",
    "--qmldir", $qmlDir,
    $stageExe
)
& $windeployqt @windeployArgs

# Remove debug/profiler QML tooling (not needed in release)
foreach ($toolingDir in @("qmltooling", "qml/QtQuick/tooling")) {
    $qmltoolingDir = Join-Path $stagePath $toolingDir
    if (Test-Path $qmltoolingDir) {
        Remove-Item -LiteralPath $qmltoolingDir -Recurse -Force
    }
}

# Remove unused Qt Quick Controls styles (only Basic is needed)
$stylesDir = Join-Path $stagePath "qml/QtQuick/Controls"
if (Test-Path $stylesDir) {
    Get-ChildItem -LiteralPath $stylesDir -Directory | Where-Object {
        $_.Name -notin @("Basic", "impl")
    } | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
}

foreach ($styleDll in @(
    "Qt6QuickControls2FluentWinUI3*.dll",
    "Qt6QuickControls2Fusion*.dll",
    "Qt6QuickControls2Imagine*.dll",
    "Qt6QuickControls2Material*.dll",
    "Qt6QuickControls2Universal*.dll",
    "Qt6QuickControls2Windows*.dll"
)) {
    Get-ChildItem -Path $stagePath -Filter $styleDll -File -ErrorAction SilentlyContinue |
        Remove-Item -Force -ErrorAction SilentlyContinue
}

# CineWindows only uses SQLite; other SQL plugins require database client runtimes.
$sqlDriversDir = Join-Path $stagePath "sqldrivers"
if (Test-Path $sqlDriversDir) {
    Get-ChildItem -LiteralPath $sqlDriversDir -Filter "*.dll" -File | Where-Object {
        $_.Name -ne "qsqlite.dll"
    } | Remove-Item -Force
}

# Remove unused QML import modules
$unusedQmlImports = @(
    "QtQml/WorkerScript"
    "QtQml/XmlListModel"
    "QtQuick/Controls/FluentWinUI3"
    "QtQuick/Controls/Fusion"
    "QtQuick/Controls/Imagine"
    "QtQuick/Controls/Material"
    "QtQuick/Controls/Universal"
    "QtQuick/Controls/Windows"
    "QtQuick/NativeStyle"
    "QtQuick/Particles"
    "QtQuick/Timeline"
    "QtQuick/VectorImage"
    "QtQuick/LocalStorage"
    "QtQuick/Dialogs/quickimpl/qml/+Fusion"
    "QtQuick/Dialogs/quickimpl/qml/+Imagine"
    "QtQuick/Dialogs/quickimpl/qml/+Material"
    "QtQuick/Dialogs/quickimpl/qml/+Universal"
    "QtQuick/Dialogs/quickimpl/qml/impl"
    "QtQuick/Xml"
)
foreach ($mod in $unusedQmlImports) {
    $modPath = Join-Path $stagePath "qml/$mod"
    if (Test-Path $modPath) {
        Remove-Item -LiteralPath $modPath -Recurse -Force -ErrorAction SilentlyContinue
    }
}

# Remove unused Qt plugins (keep only platforms, imageformats, styles/Basic, sqldrivers/qsqlite)
$pluginDirs = @(
    "bearer"
    "canbus"
    "designer"
    "generic"
    "geometryloaders"
    "iconengines"
    "multimedia"
    "networkinformation"
    "position"
    "printsupport"
    "renderplugins"
    "sceneparsers"
    "styles"
    "texttospeech"
    "virtualkeyboard"
    "wayland-compositors"
    "webview"
)
foreach ($pluginDir in $pluginDirs) {
    $pluginPath = Join-Path $stagePath "plugins/$pluginDir"
    if (Test-Path $pluginPath) {
        Remove-Item -LiteralPath $pluginPath -Recurse -Force -ErrorAction SilentlyContinue
    }
}

# Remove unnecessary Qt translations (keep only common ones)
$translationsDir = Join-Path $stagePath "translations"
if (Test-Path $translationsDir) {
    Get-ChildItem -Path $translationsDir -Filter "*.qm" | Where-Object {
        $_.BaseName -notmatch '^qt_(base|help|quick)'
    } | Remove-Item -Force -ErrorAction SilentlyContinue
}

$mpvQtDll = Join-Path $mpvQtBinPath "libMpvQt.dll"
if (Test-Path $mpvQtDll) {
    Copy-Item -LiteralPath $mpvQtDll -Destination $stagePath -Force
} else {
    throw "libMpvQt.dll was not found at $mpvQtDll. Run scripts\setup\bootstrap_mpvqt_windows.ps1 first or pass -MpvQtBin."
}

$mpvDll = if ($env:MPV_DLL) { $env:MPV_DLL } else { Join-Path $libmpvBinPath "libmpv-2.dll" }
if (Test-Path $mpvDll) {
    Copy-Item -LiteralPath $mpvDll -Destination $stagePath -Force
} else {
    throw "Standalone libmpv-2.dll was not found at $mpvDll. Run scripts\setup\bootstrap_libmpv_windows.ps1 first or pass -LibmpvBin."
}

# Stage media tools separately (optional download)
$mediaStagePath = Join-Path $repoRoot "packaging/windows/stage-media"
if (Test-Path $mediaStagePath) {
    Remove-Item -LiteralPath $mediaStagePath -Recurse -Force
}
New-Item -ItemType Directory -Path $mediaStagePath | Out-Null

foreach ($tool in @("yt-dlp.exe", "ffmpeg.exe", "ffprobe.exe")) {
    $toolPath = Join-Path $mediaToolsBinPath $tool
    if (-not (Test-Path $toolPath)) {
        throw "$tool was not found at $toolPath. Run scripts\setup\bootstrap_media_tools_windows.ps1 first or pass -MediaToolsBin."
    }
    Copy-Item -LiteralPath $toolPath -Destination $mediaStagePath -Force
}

foreach ($doc in @("README.md", "LICENSE.md", "THIRD_PARTY_NOTICES.md")) {
    $docPath = Join-Path $repoRoot $doc
    if (Test-Path $docPath) {
        Copy-Item -LiteralPath $docPath -Destination $stagePath -Force
    }
}

# Strip debug symbols from all staged binaries
Write-Host "==> Stripping debug symbols..."
Strip-Binaries -Directory $stagePath

$verifyScript = Join-Path $PSScriptRoot "verify_windows_deployment.ps1"
& $verifyScript -StageDir $stagePath -MsysBin $MsysBin -SkipGuiStartupCheck:$SkipGuiStartupCheck

foreach ($toolCheck in @(
    @{ Name = "ffmpeg.exe"; Argument = "-version" },
    @{ Name = "ffprobe.exe"; Argument = "-version" },
    @{ Name = "yt-dlp.exe"; Argument = "--version" }
)) {
    $toolCheckPath = Join-Path $mediaStagePath $toolCheck.Name
    if (Test-Path $toolCheckPath) {
        & $toolCheckPath $toolCheck.Argument 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "$($toolCheck.Name) failed its packaged startup check with exit code $LASTEXITCODE."
        }
    }
}

$version = "1.0.0"
$cmakeText = Get-Content -Raw $cmakeFile
if ($cmakeText -match 'project\(CineWindows\s+VERSION\s+([0-9.]+)') {
    $version = $Matches[1]
}

# Create portable zip (core app only, no media tools)
$portableZip = Join-Path $artifactPath "CineWindows-$version-win64-portable.zip"
if (Test-Path $portableZip) {
    Remove-Item -LiteralPath $portableZip -Force
}
$sevenZip = Find-OnPath "7z.exe"
if ($sevenZip) {
    & $sevenZip a -t7z -mx=9 -mmt=on $portableZip (Join-Path $stagePath "*") | Out-Null
} else {
    Compress-Archive -Path (Join-Path $stagePath "*") -DestinationPath $portableZip -Force
}

# Create media tools zip (optional download)
$mediaZip = Join-Path $artifactPath "CineWindows-$version-win64-media-tools.zip"
if (Test-Path $mediaZip) {
    Remove-Item -LiteralPath $mediaZip -Force
}
if ($sevenZip) {
    & $sevenZip a -t7z -mx=9 -mmt=on $mediaZip (Join-Path $mediaStagePath "*") | Out-Null
} else {
    Compress-Archive -Path (Join-Path $mediaStagePath "*") -DestinationPath $mediaZip -Force
}

# NSIS installer (core app only, no media tools)
if (-not $SkipInstaller) {
    $makensis = Find-OnPath "makensis.exe"
    $nsi = Join-Path $repoRoot "packaging/windows/setup.nsi"
    if ($makensis -and (Test-Path $nsi)) {
        Push-Location (Split-Path -Parent $nsi)
        try {
            & $makensis "/DAPP_VERSION=$version" "/DSTAGE_DIR=$stagePath" "/DOUTPUT_DIR=$artifactPath" $nsi
        } finally {
            Pop-Location
        }
    } else {
        Write-Warning "makensis.exe was not found. Staged app is ready at $stagePath."
    }
}

Write-Host "CineWindows staged at $stagePath"
Write-Host "Artifacts written to $artifactPath"
