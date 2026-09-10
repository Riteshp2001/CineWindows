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
    [Parameter(Mandatory = $true)]
    [string]$StageDir,
    [string]$MsysBin = "C:\msys64\mingw64\bin",
    [ValidateRange(2, 60)]
    [int]$StartupTimeoutSeconds = 8,
    [switch]$SkipGuiStartupCheck
)

$ErrorActionPreference = "Stop"

$stagePath = (Resolve-Path $StageDir).Path
$noticesPath = Join-Path $stagePath "THIRD_PARTY_NOTICES.md"
if (-not (Test-Path -LiteralPath $noticesPath -PathType Leaf)) {
    throw "Deployment is missing THIRD_PARTY_NOTICES.md."
}

$objdump = Join-Path $MsysBin "objdump.exe"
if (-not (Test-Path -LiteralPath $objdump)) {
    $objdumpCommand = Get-Command "objdump.exe" -ErrorAction SilentlyContinue
    if (-not $objdumpCommand) {
        throw "objdump.exe was not found. Pass the MSYS2 bin directory with -MsysBin."
    }
    $objdump = $objdumpCommand.Source
}

$systemDirectories = @(
    (Join-Path $env:SystemRoot "System32"),
    (Join-Path $env:SystemRoot "SysWOW64")
)
$missing = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$binaries = Get-ChildItem -LiteralPath $stagePath -Recurse -File | Where-Object {
    $_.Extension -in @(".dll", ".exe")
}

foreach ($binary in $binaries) {
    $output = & $objdump -p $binary.FullName 2>$null
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to inspect PE imports for $($binary.FullName)."
    }

    foreach ($line in $output) {
        if ($line -notmatch "DLL Name:\s*(.+?)\s*$") {
            continue
        }

        $dependency = $Matches[1]
        if ($dependency -match "^(api-ms-|ext-ms-)") {
            continue
        }

        $resolved = Test-Path -LiteralPath (Join-Path $stagePath $dependency)
        if (-not $resolved) {
            $resolved = Test-Path -LiteralPath (Join-Path $binary.DirectoryName $dependency)
        }
        if (-not $resolved) {
            foreach ($systemDirectory in $systemDirectories) {
                if (Test-Path -LiteralPath (Join-Path $systemDirectory $dependency)) {
                    $resolved = $true
                    break
                }
            }
        }

        if (-not $resolved) {
            [void]$missing.Add("$dependency (imported by $($binary.Name))")
        }
    }
}

if ($missing.Count -gt 0) {
    $details = ($missing | Sort-Object) -join [Environment]::NewLine
    throw "Deployment has unresolved runtime dependencies:$([Environment]::NewLine)$details"
}

Write-Host "Verified runtime imports for $($binaries.Count) deployed executables and DLLs."

$appPath = Join-Path $stagePath "CineWindows.exe"
if (-not (Test-Path -LiteralPath $appPath -PathType Leaf)) {
    throw "Deployment is missing CineWindows.exe."
}

$versionProcess = $null
$startupProcess = $null
$hadStandardPathsMode = Test-Path Env:QT_STANDARDPATHS_TEST_MODE
$previousStandardPathsMode = $env:QT_STANDARDPATHS_TEST_MODE
$hadDiskCacheMode = Test-Path Env:QML_DISABLE_DISK_CACHE
$previousDiskCacheMode = $env:QML_DISABLE_DISK_CACHE
$env:QT_STANDARDPATHS_TEST_MODE = "1"
$env:QML_DISABLE_DISK_CACHE = "1"

try {
    $versionStartInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $versionStartInfo.FileName = $appPath
    $versionStartInfo.Arguments = "--version"
    $versionStartInfo.WorkingDirectory = $stagePath
    $versionStartInfo.UseShellExecute = $false
    $versionStartInfo.CreateNoWindow = $true
    $versionStartInfo.RedirectStandardOutput = $true
    $versionStartInfo.RedirectStandardError = $true
    $versionProcess = [System.Diagnostics.Process]::new()
    $versionProcess.StartInfo = $versionStartInfo
    if (-not $versionProcess.Start()) {
        throw "Packaged CineWindows.exe --version could not be started."
    }
    $versionStdoutTask = $versionProcess.StandardOutput.ReadToEndAsync()
    $versionStderrTask = $versionProcess.StandardError.ReadToEndAsync()
    if (-not $versionProcess.WaitForExit($StartupTimeoutSeconds * 1000)) {
        throw "Packaged CineWindows.exe --version did not exit within $StartupTimeoutSeconds seconds."
    }
    $versionProcess.WaitForExit()
    $versionStdoutText = $versionStdoutTask.GetAwaiter().GetResult().Trim()
    $versionStderrText = $versionStderrTask.GetAwaiter().GetResult().Trim()
    if ($versionProcess.ExitCode -ne 0) {
        $details = if ($versionStderrText) { "`n$versionStderrText" } else { "" }
        throw "Packaged CineWindows.exe --version failed with exit code $($versionProcess.ExitCode).$details"
    }
    if ($versionStdoutText) {
        Write-Host $versionStdoutText
    }

    if (-not $SkipGuiStartupCheck) {
        $startupStartInfo = [System.Diagnostics.ProcessStartInfo]::new()
        $startupStartInfo.FileName = $appPath
        $startupStartInfo.WorkingDirectory = $stagePath
        $startupStartInfo.UseShellExecute = $false
        $startupStartInfo.CreateNoWindow = $true
        $startupStartInfo.RedirectStandardOutput = $true
        $startupStartInfo.RedirectStandardError = $true
        $startupProcess = [System.Diagnostics.Process]::new()
        $startupProcess.StartInfo = $startupStartInfo
        if (-not $startupProcess.Start()) {
            throw "Packaged CineWindows.exe could not be started."
        }
        $startupStdoutTask = $startupProcess.StandardOutput.ReadToEndAsync()
        $startupStderrTask = $startupProcess.StandardError.ReadToEndAsync()
        if ($startupProcess.WaitForExit($StartupTimeoutSeconds * 1000)) {
            $startupProcess.WaitForExit()
            $stdout = $startupStdoutTask.GetAwaiter().GetResult().Trim()
            $stderr = $startupStderrTask.GetAwaiter().GetResult().Trim()
            $diagnosticText = if ($stderr) { $stderr } else { $stdout }
            $details = if ($diagnosticText) { "`n$diagnosticText" } else { "" }
            throw "Packaged CineWindows.exe exited during startup with code $($startupProcess.ExitCode).$details"
        }

        Write-Host "Verified packaged GUI startup for $StartupTimeoutSeconds seconds."
    } else {
        Write-Host "Skipped packaged GUI startup check for headless environment."
    }
} finally {
    foreach ($process in @($startupProcess, $versionProcess)) {
        if ($process -and -not $process.HasExited) {
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
            Wait-Process -Id $process.Id -ErrorAction SilentlyContinue
        }
        if ($process) {
            $process.Dispose()
        }
    }
    if ($hadStandardPathsMode) {
        $env:QT_STANDARDPATHS_TEST_MODE = $previousStandardPathsMode
    } else {
        Remove-Item Env:QT_STANDARDPATHS_TEST_MODE -ErrorAction SilentlyContinue
    }
    if ($hadDiskCacheMode) {
        $env:QML_DISABLE_DISK_CACHE = $previousDiskCacheMode
    } else {
        Remove-Item Env:QML_DISABLE_DISK_CACHE -ErrorAction SilentlyContinue
    }
}
