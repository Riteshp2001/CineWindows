param(
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
$DistDir = Join-Path $Root "dist\CineWindows"

$MingwDir = if ($env:MINGW_PREFIX -and (Test-Path (Join-Path $env:MINGW_PREFIX "bin\python.exe"))) {
    $env:MINGW_PREFIX
} elseif (Test-Path "C:\msys64\mingw64\bin\python.exe") {
    "C:\msys64\mingw64"
} elseif (Test-Path "C:\msys2\mingw64\bin\python.exe") {
    "C:\msys2\mingw64"
} else {
    throw "MSYS2 MINGW64 Python was not found. Run setup_windows.ps1 first."
}

$BinDir = Join-Path $MingwDir "bin"

$env:PATH = "$BinDir;$env:PATH"

if (Test-Path (Join-Path $BinDir "blueprint-compiler.exe")) {
    Get-ChildItem (Join-Path $Root "src\*.blp") | ForEach-Object {
        $ui = [System.IO.Path]::ChangeExtension($_.FullName, ".ui")
        & (Join-Path $BinDir "blueprint-compiler.exe") compile $_.FullName --output $ui
    }
} else {
    Write-Host "blueprint-compiler.exe not found; using checked-in .ui files." -ForegroundColor Yellow
}

& (Join-Path $BinDir "glib-compile-resources.exe") `
    (Join-Path $Root "src\cine.gresource.xml") `
    "--target=$(Join-Path $Root 'src\cine.gresource')" `
    "--sourcedir=$(Join-Path $Root 'src')"

& (Join-Path $BinDir "glib-compile-schemas.exe") (Join-Path $Root "data")

if (Test-Path (Join-Path $BinDir "gcc.exe")) {
    & (Join-Path $BinDir "gcc.exe") -O2 -mwindows `
        (Join-Path $Root "src\launcher.c") `
        -o (Join-Path $Root "CineWindows.exe")
} else {
    Write-Host "gcc.exe not found; skipping native EXE launcher." -ForegroundColor Yellow
}

if (Test-Path $DistDir) {
    Remove-Item -LiteralPath $DistDir -Recurse -Force
}

New-Item -ItemType Directory -Path $DistDir | Out-Null
Copy-Item -Path (Join-Path $Root "src") -Destination $DistDir -Recurse
Copy-Item -Path (Join-Path $Root "data") -Destination $DistDir -Recurse
Copy-Item -Path (Join-Path $Root "start_cine.py") -Destination $DistDir
Copy-Item -Path (Join-Path $Root "requirements.txt") -Destination $DistDir
Copy-Item -Path (Join-Path $Root "run_cine.bat") -Destination $DistDir
Copy-Item -Path (Join-Path $Root "cine.ps1") -Destination $DistDir
$readmeSrc = Join-Path $Root "README-Windows.md"
if (-not (Test-Path $readmeSrc)) {
    $readmeSrc = Join-Path $Root "README.md"
}
Copy-Item -Path $readmeSrc -Destination (Join-Path $DistDir "README.txt")

if (Test-Path (Join-Path $Root "CineWindows.exe")) {
    Copy-Item -Path (Join-Path $Root "CineWindows.exe") -Destination $DistDir
}

Write-Host "Built $Configuration package at $DistDir" -ForegroundColor Green
