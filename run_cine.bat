@echo off
REM CineWindows Launcher
REM Requires MSYS2 MINGW64 installed at C:\msys64

setlocal
set "MSYS2_DIR=C:\msys64"
set "MINGW_DIR=%MSYS2_DIR%\mingw64"

if not exist "%MINGW_DIR%\bin\python.exe" (
    echo ERROR: MSYS2 MINGW64 not found at %MSYS2_DIR%
    echo Please run setup_windows.ps1 first.
    pause
    exit /b 1
)

set "PATH=%MINGW_DIR%\bin;%PATH%"
set "GSETTINGS_SCHEMA_DIR=%~dp0data"
set "LANG=C"
set "LC_ALL=C"

"%MINGW_DIR%\bin\python.exe" "%~dp0start_cine.py" %*
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo CineWindows exited with code %ERRORLEVEL%
    pause
)
