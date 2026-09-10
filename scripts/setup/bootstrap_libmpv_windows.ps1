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
# Downloads the statically linked Windows libmpv development package recommended
# by mpv.io. It replaces the large MSYS2 shared-library closure with one libmpv DLL.

param(
    [string]$InstallPrefix = ".deps\libmpv-static",
    [ValidateSet("x86_64", "x86_64-v3")]
    [string]$Architecture = "x86_64",
    [string]$ReleaseTag = ""
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$installPath = if ([IO.Path]::IsPathRooted($InstallPrefix)) {
    [IO.Path]::GetFullPath($InstallPrefix)
} else {
    [IO.Path]::GetFullPath((Join-Path $repoRoot $InstallPrefix))
}
$depsPath = Join-Path $repoRoot ".deps"
$downloadPath = Join-Path $depsPath "downloads"
$extractPath = Join-Path $depsPath "libmpv-static-extract"

function Find-7Zip {
    foreach ($candidate in @(
        (Get-Command "7z.exe" -ErrorAction SilentlyContinue).Source,
        (Get-Command "7zz.exe" -ErrorAction SilentlyContinue).Source,
        "C:\Program Files\7-Zip\7z.exe",
        "C:\Program Files (x86)\7-Zip\7z.exe"
    )) {
        if ($candidate -and (Test-Path -LiteralPath $candidate)) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }
    return $null
}

$sevenZip = Find-7Zip
if (-not $sevenZip) {
    throw "7-Zip was not found. Install 7-Zip or add 7z.exe to PATH."
}

$headers = @{
    Accept = "application/vnd.github+json"
    "User-Agent" = "CineWindows-libmpv-bootstrap"
    "X-GitHub-Api-Version" = "2022-11-28"
}
$releaseUri = if ($ReleaseTag) {
    "https://api.github.com/repos/shinchiro/mpv-winbuild-cmake/releases/tags/$ReleaseTag"
} else {
    "https://api.github.com/repos/shinchiro/mpv-winbuild-cmake/releases/latest"
}
$release = Invoke-RestMethod -Headers $headers -Uri $releaseUri
$asset = $release.assets |
    Where-Object { $_.name -like "mpv-dev-$Architecture-*.7z" } |
    Select-Object -First 1
if (-not $asset) {
    throw "Release '$($release.tag_name)' has no mpv-dev-$Architecture archive."
}

New-Item -ItemType Directory -Path $downloadPath -Force | Out-Null
$archivePath = Join-Path $downloadPath $asset.name
if (-not (Test-Path -LiteralPath $archivePath)) {
    Write-Host "Downloading $($asset.name)..."
    Invoke-WebRequest -Headers $headers -Uri $asset.browser_download_url -OutFile $archivePath
}

if (Test-Path -LiteralPath $extractPath) {
    $resolvedExtract = (Resolve-Path -LiteralPath $extractPath).Path
    if (-not $resolvedExtract.StartsWith($depsPath, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clear extraction directory outside $depsPath"
    }
    Remove-Item -LiteralPath $resolvedExtract -Recurse -Force
}
New-Item -ItemType Directory -Path $extractPath -Force | Out-Null

& $sevenZip x $archivePath "-o$extractPath" -y | Out-Null
if ($LASTEXITCODE -ne 0) {
    throw "7-Zip failed with exit code $LASTEXITCODE."
}

$dll = Get-ChildItem -LiteralPath $extractPath -Recurse -File -Filter "libmpv-2.dll" |
    Select-Object -First 1
$header = Get-ChildItem -LiteralPath $extractPath -Recurse -File -Filter "client.h" |
    Where-Object { $_.Directory.Name -eq "mpv" } |
    Select-Object -First 1
$importLibrary = Get-ChildItem -LiteralPath $extractPath -Recurse -File |
    Where-Object { $_.Name -in @("libmpv.dll.a", "libmpv-2.dll.a") } |
    Select-Object -First 1

if (-not $dll -or -not $header -or -not $importLibrary) {
    throw "The downloaded development archive is missing libmpv-2.dll, mpv/client.h, or its import library."
}

$binPath = Join-Path $installPath "bin"
$includePath = Join-Path $installPath "include\mpv"
$libPath = Join-Path $installPath "lib"
New-Item -ItemType Directory -Path $binPath, $includePath, $libPath -Force | Out-Null

Copy-Item -LiteralPath $dll.FullName -Destination (Join-Path $binPath "libmpv-2.dll") -Force
Copy-Item -Path (Join-Path $header.Directory.FullName "*") -Destination $includePath -Recurse -Force
Copy-Item -LiteralPath $importLibrary.FullName -Destination (Join-Path $libPath "libmpv.dll.a") -Force

$dllSize = [math]::Round((Get-Item (Join-Path $binPath "libmpv-2.dll")).Length / 1MB, 1)
Write-Host "Static libmpv $($release.tag_name) installed at $installPath ($dllSize MiB)."
Write-Host "Run scripts\setup\bootstrap_mpvqt_windows.ps1 to link MpvQt against it."
