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
param(
    [string]$BuildDir = "build/release",
    [string]$Configuration = "Release",
    [switch]$ReleaseOnly,
    [switch]$SkipSourceLanguageFill
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$buildPath = if ([IO.Path]::IsPathRooted($BuildDir)) { $BuildDir } else { Join-Path $repoRoot $BuildDir }
$cacheFile = Join-Path $buildPath "CMakeCache.txt"

if (-not (Test-Path $cacheFile)) {
    cmake -S $repoRoot -B $buildPath -DCMAKE_BUILD_TYPE=$Configuration
}

if (-not $ReleaseOnly) {
    cmake --build $buildPath --target update_translations --config $Configuration
}

if (-not $SkipSourceLanguageFill) {
    $englishTs = Join-Path $repoRoot "i18n\CineWindows_en.ts"
    if (Test-Path $englishTs) {
        [xml]$doc = Get-Content -Raw -Encoding UTF8 $englishTs
        $messages = $doc.SelectNodes("//message")
        foreach ($message in $messages) {
            $source = $message.SelectSingleNode("source")
            $translation = $message.SelectSingleNode("translation")
            if ($source -and $translation) {
                [void]$translation.RemoveAttribute("type")
                $translation.InnerText = $source.InnerText
            }
        }

        $settings = [System.Xml.XmlWriterSettings]::new()
        $settings.Encoding = [System.Text.UTF8Encoding]::new($false)
        $settings.Indent = $true
        $settings.NewLineChars = "`r`n"
        $settings.OmitXmlDeclaration = $false

        $writer = [System.Xml.XmlWriter]::Create($englishTs, $settings)
        try {
            $doc.Save($writer)
        } finally {
            $writer.Close()
        }
    }
}

cmake --build $buildPath --target release_translations --config $Configuration
