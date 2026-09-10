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
# Downloads standalone Windows FFmpeg and yt-dlp binaries for packaging.

param(
    [string]$InstallPrefix = ".deps\media-tools",
    [string]$FfmpegAsset = "ffmpeg-master-latest-win64-gpl.zip"
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$installPath = if ([IO.Path]::IsPathRooted($InstallPrefix)) {
    [IO.Path]::GetFullPath($InstallPrefix)
} else {
    [IO.Path]::GetFullPath((Join-Path $repoRoot $InstallPrefix))
}
$downloadsPath = Join-Path $repoRoot ".deps\downloads"
$extractPath = Join-Path $repoRoot ".deps\media-tools-extract"
$binPath = Join-Path $installPath "bin"
$headers = @{
    Accept = "application/vnd.github+json"
    "User-Agent" = "CineWindows-media-tools-bootstrap"
    "X-GitHub-Api-Version" = "2022-11-28"
}

function Get-ReleaseAsset {
    param([string]$Repository, [string]$AssetName)

    $release = Invoke-RestMethod -Headers $headers -Uri "https://api.github.com/repos/$Repository/releases/latest"
    $asset = $release.assets | Where-Object Name -eq $AssetName | Select-Object -First 1
    if (-not $asset) {
        throw "Latest $Repository release has no $AssetName asset."
    }
    return @{ Release = $release; Asset = $asset }
}

function Save-VerifiedAsset {
    param($ReleaseAsset, [string]$Destination)

    if (-not (Test-Path -LiteralPath $Destination)) {
        Write-Host "Downloading $($ReleaseAsset.Asset.name)..."
        Invoke-WebRequest -Headers $headers -Uri $ReleaseAsset.Asset.browser_download_url -OutFile $Destination
    }

    if ($ReleaseAsset.Asset.digest -match '^sha256:(.+)$') {
        $expected = $Matches[1]
        $actual = (Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash
        if ($actual -ne $expected) {
            throw "SHA-256 mismatch for $($ReleaseAsset.Asset.name)."
        }
    }
}

New-Item -ItemType Directory -Path $downloadsPath, $binPath -Force | Out-Null

$ffmpeg = Get-ReleaseAsset -Repository "BtbN/FFmpeg-Builds" -AssetName $FfmpegAsset
$ffmpegArchive = Join-Path $downloadsPath "$($ffmpeg.Release.id)-$FfmpegAsset"
Save-VerifiedAsset -ReleaseAsset $ffmpeg -Destination $ffmpegArchive

if (Test-Path -LiteralPath $extractPath) {
    Remove-Item -LiteralPath $extractPath -Recurse -Force
}
Expand-Archive -LiteralPath $ffmpegArchive -DestinationPath $extractPath -Force
foreach ($tool in @("ffmpeg.exe", "ffprobe.exe")) {
    $source = Get-ChildItem -LiteralPath $extractPath -Recurse -File -Filter $tool | Select-Object -First 1
    if (-not $source) {
        throw "$FfmpegAsset is missing $tool."
    }
    Copy-Item -LiteralPath $source.FullName -Destination (Join-Path $binPath $tool) -Force
}

$ytDlp = Get-ReleaseAsset -Repository "yt-dlp/yt-dlp" -AssetName "yt-dlp.exe"
$ytDlpDownload = Join-Path $downloadsPath "$($ytDlp.Release.tag_name)-yt-dlp.exe"
Save-VerifiedAsset -ReleaseAsset $ytDlp -Destination $ytDlpDownload
Copy-Item -LiteralPath $ytDlpDownload -Destination (Join-Path $binPath "yt-dlp.exe") -Force

Remove-Item -LiteralPath $extractPath -Recurse -Force
Write-Host "Standalone media tools installed at $binPath"
