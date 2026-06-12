#Requires -Version 5.1
[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    # Keeps Python sources for easier debugging by default.
    # Use -StripPythonSources for smaller release bundles.
    [switch]$StripPythonSources,

    [switch]$SkipBytecode,
    [switch]$SkipIcon,
    [switch]$SkipLauncher,

    # Optional: signs CineWindows.exe when a signing certificate is available.
    [switch]$SignBinaries,
    [string]$SignToolPath = "signtool.exe",
    [string]$CertificateThumbprint = "",
    [string]$TimestampServer = "http://timestamp.digicert.com"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
if ($PSVersionTable.PSVersion.Major -ge 7) {
    $PSNativeCommandUseErrorActionPreference = $true
}

$AppName = "CineWindows"
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$SrcDir = Join-Path $Root "src"
$DataDir = Join-Path $Root "data"
$DistRoot = Join-Path $Root "dist"
$DistDir = Join-Path $DistRoot $AppName
$BundleStaging = Join-Path $DistRoot ".bundle_staging"

function Write-Step {
    param([Parameter(Mandatory)][string]$Message)
    Write-Host "`n[$AppName] $Message" -ForegroundColor Cyan
}

function Write-Warn {
    param([Parameter(Mandatory)][string]$Message)
    Write-Host "[Warning] $Message" -ForegroundColor Yellow
}

function New-CleanDirectory {
    param([Parameter(Mandatory)][string]$Path)
    if (Test-Path -LiteralPath $Path) {
        Remove-Item -LiteralPath $Path -Recurse -Force
    }
    New-Item -ItemType Directory -Path $Path -Force | Out-Null
}

function Invoke-Native {
    param(
        [Parameter(Mandatory)][string]$FilePath,
        [string[]]$Arguments = @(),
        [int[]]$SuccessExitCodes = @(0)
    )

    & $FilePath @Arguments
    $exitCode = $LASTEXITCODE
    if ($SuccessExitCodes -notcontains $exitCode) {
        throw "Command failed with exit code $($exitCode): $FilePath $($Arguments -join ' ')"
    }
}

function Invoke-RoboCopy {
    param(
        [Parameter(Mandatory)][string]$Source,
        [Parameter(Mandatory)][string]$Destination,
        [string[]]$Options = @()
    )

    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    & robocopy $Source $Destination @Options | Out-Null
    $exitCode = $LASTEXITCODE

    # Robocopy uses 0-7 as success / non-fatal differences; 8+ means failure.
    if ($exitCode -ge 8) {
        throw "Robocopy failed with exit code $($exitCode): $Source -> $Destination"
    }
}

function Get-ToolPath {
    param(
        [Parameter(Mandatory)][string]$Directory,
        [Parameter(Mandatory)][string]$Name,
        [switch]$Required
    )

    $path = Join-Path $Directory $Name
    if (Test-Path -LiteralPath $path) {
        return $path
    }

    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    if ($Required) {
        throw "$Name was not found. Expected it in: $Directory"
    }

    return $null
}

function Resolve-MingwDir {
    $candidates = New-Object System.Collections.Generic.List[string]

    if ($env:MINGW_PREFIX) {
        $prefix = $env:MINGW_PREFIX
        if ($prefix -match "^/[a-zA-Z0-9_/-]+") {
            $cygPathCandidates = @(
                "C:\msys64\usr\bin\cygpath.exe",
                "C:\msys2\usr\bin\cygpath.exe"
            )
            foreach ($cyg in $cygPathCandidates) {
                if (Test-Path -LiteralPath $cyg) {
                    try {
                        $converted = (& $cyg -w $prefix 2>$null).Trim()
                        if ($converted) { $candidates.Add($converted) }
                    } catch {}
                    break
                }
            }
        } else {
            $candidates.Add($prefix)
        }
    }

    @(
        "C:\msys64\mingw64",
        "C:\msys2\mingw64"
    ) | ForEach-Object { $candidates.Add($_) }

    foreach ($candidate in $candidates | Select-Object -Unique) {
        if (Test-Path -LiteralPath (Join-Path $candidate "bin\python.exe")) {
            return (Resolve-Path $candidate).Path
        }
    }

    throw "MSYS2 MINGW64 Python was not found. Run setup_windows.ps1 first."
}

function Copy-FileIfExists {
    param(
        [Parameter(Mandatory)][string]$Source,
        [Parameter(Mandatory)][string]$Destination
    )

    if (Test-Path -LiteralPath $Source) {
        New-Item -ItemType Directory -Path (Split-Path $Destination -Parent) -Force | Out-Null
        Copy-Item -LiteralPath $Source -Destination $Destination -Force
        return $true
    }
    return $false
}

function Copy-GlobFiles {
    param(
        [Parameter(Mandatory)][string]$SourceDir,
        [Parameter(Mandatory)][string]$Pattern,
        [Parameter(Mandatory)][string]$DestinationDir,
        [scriptblock]$AfterCopy = $null
    )

    if (-not (Test-Path -LiteralPath $SourceDir)) { return }
    New-Item -ItemType Directory -Path $DestinationDir -Force | Out-Null

    Get-ChildItem -LiteralPath $SourceDir -Filter $Pattern -File -ErrorAction SilentlyContinue | ForEach-Object {
        $dest = Join-Path $DestinationDir $_.Name
        if (-not (Test-Path -LiteralPath $dest)) {
            Copy-Item -LiteralPath $_.FullName -Destination $dest -Force
        }
        if ($AfterCopy) { & $AfterCopy $_.FullName }
    }
}

function Get-PythonVersion {
    param([Parameter(Mandatory)][string]$PythonExe)

    $output = & $PythonExe -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')" 2>$null
    if ($LASTEXITCODE -ne 0 -or -not $output) {
        throw "Unable to determine Python version from $PythonExe"
    }
    return $output.Trim()
}

function Remove-EmptyDirectories {
    param([Parameter(Mandatory)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) { return }

    Get-ChildItem -LiteralPath $Path -Directory -Recurse -Force -ErrorAction SilentlyContinue |
        Sort-Object { $_.FullName.Length } -Descending |
        Where-Object { -not (Get-ChildItem -LiteralPath $_.FullName -Force -ErrorAction SilentlyContinue) } |
        Remove-Item -Force -ErrorAction SilentlyContinue
}

function Sign-FileIfRequested {
    param([Parameter(Mandatory)][string]$FilePath)

    if (-not $SignBinaries) { return }
    if (-not (Test-Path -LiteralPath $FilePath)) { return }

    $signTool = (Get-Command $SignToolPath -ErrorAction SilentlyContinue)
    if (-not $signTool) {
        throw "SignTool was requested but not found: $SignToolPath"
    }

    $args = @("sign", "/fd", "SHA256", "/tr", $TimestampServer, "/td", "SHA256")
    if ($CertificateThumbprint) {
        $args += @("/sha1", $CertificateThumbprint)
    } else {
        $args += "/a"
    }
    $args += $FilePath

    Write-Step "Signing $([System.IO.Path]::GetFileName($FilePath))"
    Invoke-Native -FilePath $signTool.Source -Arguments $args
}

$MingwDir = Resolve-MingwDir
$BinDir = Join-Path $MingwDir "bin"
$env:PATH = "$BinDir;$env:PATH"

$PythonExe = Get-ToolPath -Directory $BinDir -Name "python.exe" -Required
$PythonwExe = Get-ToolPath -Directory $BinDir -Name "pythonw.exe" -Required
$GlibCompileResources = Get-ToolPath -Directory $BinDir -Name "glib-compile-resources.exe" -Required
$GlibCompileSchemas = Get-ToolPath -Directory $BinDir -Name "glib-compile-schemas.exe" -Required
$BlueprintCompiler = Get-ToolPath -Directory $BinDir -Name "blueprint-compiler.exe"
$GccExe = Get-ToolPath -Directory $BinDir -Name "gcc.exe"
$WindresExe = Get-ToolPath -Directory $BinDir -Name "windres.exe"
$NtlddExe = Get-ToolPath -Directory $BinDir -Name "ntldd.exe"
$ObjdumpExe = Get-ToolPath -Directory $BinDir -Name "objdump.exe"

Write-Step "Using MSYS2 MINGW64: $MingwDir"
Write-Host "Configuration: $Configuration"
Write-Host "Output: $DistDir"

# 1. Compile Blueprint UI files.
Write-Step "Compiling Blueprint UI files"
if ($BlueprintCompiler) {
    $blpDir = Join-Path $SrcDir "ui"
    Get-ChildItem -LiteralPath $blpDir -Filter "*.blp" -File -ErrorAction SilentlyContinue | ForEach-Object {
        $ui = [System.IO.Path]::ChangeExtension($_.FullName, ".ui")
        Invoke-Native -FilePath $BlueprintCompiler -Arguments @("compile", $_.FullName, "--output", $ui)
    }
} else {
    Write-Warn "blueprint-compiler.exe not found; using checked-in .ui files."
}

# 2. Compile GResource.
Write-Step "Compiling GResource"
Invoke-Native -FilePath $GlibCompileResources -Arguments @(
    (Join-Path $SrcDir "cine.gresource.xml"),
    "--target=$(Join-Path $SrcDir 'cine.gresource')",
    "--sourcedir=$SrcDir"
)

# 3. Compile GSettings schemas.
Write-Step "Compiling GSettings schemas"
Invoke-Native -FilePath $GlibCompileSchemas -Arguments @($DataDir)

# 4. Generate Windows icon.
if (-not $SkipIcon) {
    Write-Step "Generating Windows icon"
    Push-Location $Root
    try {
        Invoke-Native -FilePath $PythonExe -Arguments @((Join-Path $PSScriptRoot "create_icon.py"))
    } catch {
        Write-Warn "Icon generation failed: $_"
        if (-not (Test-Path -LiteralPath (Join-Path $Root "CineWindows.ico"))) {
            Write-Warn "No CineWindows.ico exists. The launcher may be built without an icon."
        }
    } finally {
        Pop-Location
    }
}

# 5. Compile launcher.
if (-not $SkipLauncher) {
    Write-Step "Compiling native launcher"
    if (-not $GccExe) { throw "gcc.exe was not found; cannot build the native launcher." }

    $icoPath = Join-Path $Root "CineWindows.ico"
    $rcPath = Join-Path $SrcDir "launcher.rc"
    $resPath = Join-Path $SrcDir "launcher.o"
    $launcherSource = Join-Path $SrcDir "launcher.c"
    $launcherOut = Join-Path $Root "CineWindows.exe"

    if (-not (Test-Path -LiteralPath $launcherSource)) {
        throw "Launcher source not found: $launcherSource"
    }

    $gccFlags = if ($Configuration -eq "Debug") { @("-Og", "-g", "-mwindows") } else { @("-O2", "-s", "-mwindows") }
    $gccArgs = @()
    $gccArgs += $gccFlags
    $gccArgs += $launcherSource

    if ((Test-Path -LiteralPath $icoPath) -and (Test-Path -LiteralPath $rcPath) -and $WindresExe) {
        Invoke-Native -FilePath $WindresExe -Arguments @($rcPath, "-O", "coff", "-o", $resPath)
        $gccArgs += $resPath
    } else {
        Write-Warn "Icon resource was not compiled. Missing .ico, launcher.rc, or windres.exe."
    }

    $gccArgs += @("-o", $launcherOut)
    Invoke-Native -FilePath $GccExe -Arguments $gccArgs
    Sign-FileIfRequested -FilePath $launcherOut
}

# 6. Bundle MSYS2 Python runtime and DLLs.
Write-Step "Creating runtime bundle"
New-CleanDirectory -Path $BundleStaging
New-Item -ItemType Directory -Path (Join-Path $BundleStaging "bin") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $BundleStaging "lib") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $BundleStaging "share") -Force | Out-Null

$RuntimeBin = Join-Path $BundleStaging "bin"
$RuntimeLib = Join-Path $BundleStaging "lib"
$depsCache = @{}

function Get-DllDependencies {
    param([Parameter(Mandatory)][string]$BinaryPath)

    $dlls = New-Object System.Collections.Generic.List[string]
    $mingwPattern = [regex]::Escape($MingwDir)
    $binPattern = [regex]::Escape($BinDir)

    if ($NtlddExe) {
        try {
            $lines = & $NtlddExe $BinaryPath 2>$null | Out-String -Stream
            foreach ($line in $lines) {
                $candidate = $null
                if ($line -match "=>\s*(?<path>[A-Za-z]:\\[^()]+?\.dll)\s+\(") {
                    $candidate = $matches["path"].Trim()
                } elseif ($line -match "^\s*(?<path>[A-Za-z]:\\[^()]+?\.dll)\s+\(") {
                    $candidate = $matches["path"].Trim()
                }

                if ($candidate -and ($candidate -match "^$mingwPattern" -or $candidate -match "^$binPattern") -and (Test-Path -LiteralPath $candidate)) {
                    $dlls.Add((Resolve-Path $candidate).Path)
                }
            }
        } catch {
            Write-Warn "ntldd failed for $BinaryPath. Falling back when possible."
        }
    }

    if (($dlls.Count -eq 0) -and $ObjdumpExe) {
        try {
            $lines = & $ObjdumpExe -p $BinaryPath 2>$null | Out-String -Stream
            foreach ($line in $lines) {
                if ($line -match "^\s*DLL Name:\s*(?<name>.+\.dll)\s*$") {
                    $dllName = $matches["name"].Trim()
                    $dllPath = Join-Path $BinDir $dllName
                    if (Test-Path -LiteralPath $dllPath) {
                        $dlls.Add((Resolve-Path $dllPath).Path)
                    }
                }
            }
        } catch {
            Write-Warn "objdump failed for $BinaryPath."
        }
    }

    return $dlls | Select-Object -Unique
}

function Add-DllAndDependencies {
    param(
        [Parameter(Mandatory)][string]$BinaryPath,
        [int]$Depth = 0
    )

    if ($Depth -gt 16 -or -not (Test-Path -LiteralPath $BinaryPath)) { return }

    $realPath = (Resolve-Path $BinaryPath).Path
    if ($depsCache.ContainsKey($realPath)) { return }
    $depsCache[$realPath] = $true

    foreach ($dll in Get-DllDependencies -BinaryPath $realPath) {
        $dest = Join-Path $RuntimeBin ([System.IO.Path]::GetFileName($dll))
        if (-not (Test-Path -LiteralPath $dest)) {
            Copy-Item -LiteralPath $dll -Destination $dest -Force
        }
        Add-DllAndDependencies -BinaryPath $dll -Depth ($Depth + 1)
    }
}

Copy-Item -LiteralPath $PythonExe -Destination (Join-Path $RuntimeBin "python.exe") -Force
Copy-Item -LiteralPath $PythonwExe -Destination (Join-Path $RuntimeBin "pythonw.exe") -Force
Add-DllAndDependencies -BinaryPath $PythonExe
Add-DllAndDependencies -BinaryPath $PythonwExe

$runtimeDllPatterns = @(
    "libgtk-4-1.dll", "libadwaita-1-0.dll",
    "libgio-2.0-0.dll", "libglib-2.0-0.dll", "libgobject-2.0-0.dll", "libgmodule-2.0-0.dll", "libgirepository-2.0-0.dll",
    "libpango-1.0-0.dll", "libpangocairo-1.0-0.dll", "libpangoft2-1.0-0.dll", "libpangowin32-1.0-0.dll",
    "libcairo-2.dll", "libcairo-gobject-2.dll", "libcairo-script-interpreter-2.dll",
    "libgdk_pixbuf-2.0-0.dll", "libgraphene-1.0-0.dll",
    "libharfbuzz-0.dll", "libharfbuzz-gobject-0.dll", "libharfbuzz-subset-0.dll",
    "libfribidi-0.dll", "libpcre2-8-0.dll", "libepoxy-0.dll", "libmpv-2.dll", "libpython3*.dll",
    "libffi-8.dll", "libintl-8.dll", "libiconv-2.dll",
    "libfontconfig-1.dll", "libfreetype-6.dll", "libpng16*.dll", "libjpeg*.dll", "libtiff*.dll",
    "zlib1.dll", "libbz2-1.dll", "liblzma-5.dll", "libzstd.dll",
    "libstdc++-6.dll", "libgcc_s_seh-1.dll", "libwinpthread-1.dll",
    "libsqlite3-0.dll", "libexpat-1.dll", "libxml2-2.dll",
    "libtasn1-6.dll", "libdatrie-1.dll", "libthai-0.dll", "libp11-kit-0.dll",
    "libcurl-4.dll", "libnghttp2-14.dll", "libidn2-0.dll", "libpsl-5.dll", "libssh2-1.dll", "libcares-2.dll",
    "libbrotlidec.dll", "libbrotlicommon.dll", "libbrotlienc.dll",
    "libgcrypt-20.dll", "libgpg-error-0.dll", "libfftw3-3.dll", "libltdl-7.dll", "librsvg-2-2.dll",
    "vulkan-1.dll", "libgst*-1.0-0.dll", "liborc-0.4-0.dll", "libgraphite2.dll"
)

foreach ($pattern in $runtimeDllPatterns) {
    Copy-GlobFiles -SourceDir $BinDir -Pattern $pattern -DestinationDir $RuntimeBin -AfterCopy {
        param($copiedFrom)
        Add-DllAndDependencies -BinaryPath $copiedFrom
    }
}

foreach ($toolName in @("yt-dlp.exe", "ffmpeg.exe", "ffprobe.exe")) {
    $toolPath = Join-Path $BinDir $toolName
    if (Test-Path -LiteralPath $toolPath) {
        Copy-Item -LiteralPath $toolPath -Destination (Join-Path $RuntimeBin $toolName) -Force
        Add-DllAndDependencies -BinaryPath $toolPath
    }
}

foreach ($optionalDll in @("libspell-1.dll", "libenchant-2.dll")) {
    $dllPath = Join-Path $BinDir $optionalDll
    if (Test-Path -LiteralPath $dllPath) {
        Copy-Item -LiteralPath $dllPath -Destination (Join-Path $RuntimeBin $optionalDll) -Force
        Add-DllAndDependencies -BinaryPath $dllPath
    }
}

# Python standard library.
$pyVer = Get-PythonVersion -PythonExe $PythonExe
$pyLib = Join-Path $MingwDir "lib\python$pyVer"
$pyLibDest = Join-Path $RuntimeLib "python$pyVer"
if (-not (Test-Path -LiteralPath $pyLib)) {
    throw "Python standard library not found: $pyLib"
}

Write-Step "Copying Python standard library $pyVer"
Invoke-RoboCopy -Source $pyLib -Destination $pyLibDest -Options @(
    "/E", "/NDL", "/NFL", "/NJH", "/NJS", "/NC", "/NS",
    "/XD", "test", "tests", "__pycache__"
)

# GObject introspection typelibs.
$typelibDir = Join-Path $MingwDir "lib\girepository-1.0"
$typelibDest = Join-Path $RuntimeLib "girepository-1.0"
Copy-GlobFiles -SourceDir $typelibDir -Pattern "*.typelib" -DestinationDir $typelibDest

# GIO modules.
$gioModules = Join-Path $MingwDir "lib\gio\modules"
$gioDest = Join-Path $RuntimeLib "gio\modules"
Copy-GlobFiles -SourceDir $gioModules -Pattern "*.dll" -DestinationDir $gioDest -AfterCopy {
    param($copiedFrom)
    Add-DllAndDependencies -BinaryPath $copiedFrom
}

# gdk-pixbuf loaders.
$gdkPixbufLoaders = Join-Path $MingwDir "lib\gdk-pixbuf-2.0"
$pixbufDest = Join-Path $RuntimeLib "gdk-pixbuf-2.0"
if (Test-Path -LiteralPath $gdkPixbufLoaders) {
    Invoke-RoboCopy -Source $gdkPixbufLoaders -Destination $pixbufDest -Options @(
        "/E", "/NDL", "/NFL", "/NJH", "/NJS", "/NC", "/NS"
    )
}

# Keep only common Adwaita icon sizes to reduce package size.
$gtk4Icons = Join-Path $MingwDir "share\icons\Adwaita"
$iconsDest = Join-Path $BundleStaging "share\icons\Adwaita"
if (Test-Path -LiteralPath $gtk4Icons) {
    Write-Step "Copying compact Adwaita icon set"
    Invoke-RoboCopy -Source $gtk4Icons -Destination $iconsDest -Options @(
        "/E", "/NDL", "/NFL", "/NJH", "/NJS", "/NC", "/NS",
        "/XD", "cursors", "8x8", "10x10", "11x11", "12x12", "14x14", "18x18", "20x20", "22x22",
        "28x28", "36x36", "40x40", "44x44", "56x56", "64x64", "72x72", "80x80", "96x96",
        "108x108", "128x128", "192x192", "216x216", "256x256", "384x384", "512x512",
        "256@2x", "512@2x", "scalable-up-to-32", "scalable-up-to-64"
    )
}

$bundleSizeBytes = (Get-ChildItem -LiteralPath $BundleStaging -Recurse -File -ErrorAction SilentlyContinue | Measure-Object -Property Length -Sum).Sum
$bundleSizeMb = [Math]::Round(($bundleSizeBytes / 1MB), 1)
Write-Host "Runtime bundle size: $bundleSizeMb MB"

# 7. Build dist package.
Write-Step "Creating distribution directory"
New-CleanDirectory -Path $DistDir

Invoke-RoboCopy -Source $SrcDir -Destination (Join-Path $DistDir "src") -Options @(
    "/E", "/NDL", "/NFL", "/NJH", "/NJS", "/NC", "/NS",
    "/XD", "__pycache__", ".pytest_cache",
    "/XF", "*.blp~", "*.ui~", "*.pyc", "*.pyo", "launcher.o"
)

Invoke-RoboCopy -Source $DataDir -Destination (Join-Path $DistDir "data") -Options @(
    "/E", "/NDL", "/NFL", "/NJH", "/NJS", "/NC", "/NS",
    "/XD", "__pycache__", ".pytest_cache",
    "/XF", "*.pyc", "*.pyo"
)

Copy-Item -LiteralPath (Join-Path $Root "start_cine.py") -Destination $DistDir -Force
Move-Item -LiteralPath $BundleStaging -Destination (Join-Path $DistDir "runtime") -Force

$readmeSrc = Join-Path $Root "README-Windows.md"
if (-not (Test-Path -LiteralPath $readmeSrc)) {
    $readmeSrc = Join-Path $Root "README.md"
}
if (Test-Path -LiteralPath $readmeSrc) {
    Copy-Item -LiteralPath $readmeSrc -Destination (Join-Path $DistDir "README.txt") -Force
}

Copy-FileIfExists -Source (Join-Path $Root "CineWindows.exe") -Destination (Join-Path $DistDir "CineWindows.exe") | Out-Null
Copy-FileIfExists -Source (Join-Path $Root "CineWindows.ico") -Destination (Join-Path $DistDir "CineWindows.ico") | Out-Null

# Clean dev files from dist.
Write-Step "Cleaning development files"
$cleanupPatterns = @(
    "meson.build", "*.desktop.in", "*.service.in", "*.metainfo.xml.in",
    "launcher.c", "launcher.rc", "launcher.o", "cine.in", "*.pot", "POTFILES.in", "LINGUAS",
    "*.blp~", "*.ui~", ".gitkeep"
)
foreach ($pattern in $cleanupPatterns) {
    Get-ChildItem -LiteralPath $DistDir -Recurse -Include $pattern -Force -ErrorAction SilentlyContinue |
        Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
}

$poDir = Join-Path $DistDir "po"
if (Test-Path -LiteralPath $poDir) {
    Remove-Item -LiteralPath $poDir -Recurse -Force -ErrorAction SilentlyContinue
}

# 8. Optional bytecode optimization.
if (-not $SkipBytecode) {
    Write-Step "Pre-compiling Python bytecode"
    # -b creates importable module.pyc files beside source files.
    # This is safer than moving module.cpython-*.pyc out of __pycache__ manually.
    Invoke-Native -FilePath $PythonExe -Arguments @(
        "-m", "compileall", "-q", "-f", "-b",
        (Join-Path $DistDir "src"),
        (Join-Path $DistDir "runtime\lib"),
        (Join-Path $DistDir "start_cine.py")
    )

    Get-ChildItem -LiteralPath $DistDir -Directory -Filter "__pycache__" -Recurse -Force -ErrorAction SilentlyContinue |
        Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
}

if ($StripPythonSources) {
    Write-Step "Stripping Python source files"
    # Keep start_cine.py because launchers usually call the script directly.
    foreach ($path in @((Join-Path $DistDir "src"), (Join-Path $DistDir "runtime\lib"))) {
        if (Test-Path -LiteralPath $path) {
            Get-ChildItem -LiteralPath $path -Recurse -Filter "*.py" -File -Force -ErrorAction SilentlyContinue |
                Remove-Item -Force -ErrorAction SilentlyContinue
        }
    }
}

Remove-EmptyDirectories -Path $DistDir

$distSizeBytes = (Get-ChildItem -LiteralPath $DistDir -Recurse -File -ErrorAction SilentlyContinue | Measure-Object -Property Length -Sum).Sum
$distSizeMb = [Math]::Round(($distSizeBytes / 1MB), 1)
Write-Host "`nBuilt $Configuration package at: $DistDir" -ForegroundColor Green
Write-Host "Final package size: $distSizeMb MB" -ForegroundColor Green
