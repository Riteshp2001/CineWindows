@echo off
REM CineWindows Launcher
REM Uses bundled runtime first, falls back to MSYS2

setlocal

set "CINE_DIR=%~dp0"

REM Try bundled runtime first
if exist "%CINE_DIR%\runtime\bin\python.exe" (
    set "PATH=%CINE_DIR%\runtime\bin;%PATH%"
    set "GSETTINGS_SCHEMA_DIR=%CINE_DIR%data"
    set "LANG=C"
    set "LC_ALL=C"
    "%CINE_DIR%\runtime\bin\python.exe" "%CINE_DIR%start_cine.py" %*
    if %ERRORLEVEL% NEQ 0 (
        echo.
        echo CineWindows exited with code %ERRORLEVEL%
        pause
    )
    exit /b %ERRORLEVEL%
)

REM Fallback: try MSYS2
set "MSYS2_DIR=C:\msys64"
set "MINGW_DIR=%MSYS2_DIR%\mingw64"

if not exist "%MINGW_DIR%\bin\python.exe" (
    echo ERROR: No Python runtime found.
    echo Please reinstall CineWindows using the official installer.
    pause
    exit /b 1
)

set "PATH=%MINGW_DIR%\bin;%PATH%"
set "GSETTINGS_SCHEMA_DIR=%CINE_DIR%data"
set "LANG=C"
set "LC_ALL=C"

"%MINGW_DIR%\bin\python.exe" "%CINE_DIR%start_cine.py" %*
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo CineWindows exited with code %ERRORLEVEL%
    pause
)
