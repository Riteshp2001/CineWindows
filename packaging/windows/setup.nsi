; CineWindows - Video Player
; Copyright (c) 2026 Ritesh Pandit
;
; CineWindows Community License
;
; This source code is made available for personal, non-commercial
; use only. Organizations may not use, copy, modify, or distribute
; this code without written permission from Ritesh Pandit.
;
; See the LICENSE.md file for full license terms.
;
; Project: CineWindows
; Author:  Ritesh Pandit
; Last modified: 2026-09-10
; Modified by: Ritesh Pandit

!include "MUI2.nsh"

SetCompressor /SOLID lzma
SetCompressorDictSize 64

!ifndef APP_VERSION
!define APP_VERSION "1.0.0"
!endif

!ifndef STAGE_DIR
!define STAGE_DIR "stage"
!endif

!ifndef OUTPUT_DIR
!define OUTPUT_DIR "."
!endif

!define APP_PUBLISHER "CineWindows"
!define APP_EXE "CineWindows.exe"
!define APP_REGKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\CineWindows"
!define APP_PROGID "CineWindows.Media"
!define APP_CAPABILITIES "Software\CineWindows\Capabilities"

!macro RegisterMediaExtension EXTENSION
    WriteRegStr HKCU "${APP_CAPABILITIES}\FileAssociations" "${EXTENSION}" "${APP_PROGID}"
    WriteRegStr HKCU "Software\Classes\Applications\${APP_EXE}\SupportedTypes" "${EXTENSION}" ""
!macroend

Name "CineWindows"
OutFile "${OUTPUT_DIR}\CineWindows-${APP_VERSION}-win64-setup.exe"
InstallDir "$LOCALAPPDATA\CineWindows"
InstallDirRegKey HKCU "Software\CineWindows" "InstallDir"
RequestExecutionLevel user
Unicode true

!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\${APP_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT "Launch CineWindows"
!define MUI_ICON "..\..\resources\windows\CineWindows.ico"
!define MUI_UNICON "..\..\resources\windows\CineWindows.ico"
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE.md"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Section "CineWindows" SecCore
    SectionIn RO
    SetShellVarContext current
    SetOutPath "$INSTDIR"
    File /r "${STAGE_DIR}\*.*"

    WriteUninstaller "$INSTDIR\Uninstall.exe"
    WriteRegStr HKCU "Software\CineWindows" "InstallDir" "$INSTDIR"
    WriteRegStr HKCU "${APP_REGKEY}" "DisplayName" "CineWindows"
    WriteRegStr HKCU "${APP_REGKEY}" "DisplayVersion" "${APP_VERSION}"
    WriteRegStr HKCU "${APP_REGKEY}" "Publisher" "${APP_PUBLISHER}"
    WriteRegStr HKCU "${APP_REGKEY}" "DisplayIcon" "$INSTDIR\${APP_EXE}"
    WriteRegStr HKCU "${APP_REGKEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKCU "${APP_REGKEY}" "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr HKCU "${APP_REGKEY}" "QuietUninstallString" "$INSTDIR\Uninstall.exe /S"
    WriteRegDWORD HKCU "${APP_REGKEY}" "NoModify" 1
    WriteRegDWORD HKCU "${APP_REGKEY}" "NoRepair" 1

    WriteRegStr HKCU "Software\Classes\${APP_PROGID}" "" "CineWindows media file"
    WriteRegStr HKCU "Software\Classes\${APP_PROGID}" "FriendlyTypeName" "CineWindows media file"
    WriteRegStr HKCU "Software\Classes\${APP_PROGID}\DefaultIcon" "" "$INSTDIR\${APP_EXE},0"
    WriteRegStr HKCU "Software\Classes\${APP_PROGID}\shell" "" "open"
    WriteRegStr HKCU "Software\Classes\${APP_PROGID}\shell\open" "FriendlyAppName" "CineWindows"
    WriteRegStr HKCU "Software\Classes\${APP_PROGID}\shell\open\command" "" "$\"$INSTDIR\${APP_EXE}$\" $\"%1$\""

    WriteRegStr HKCU "Software\Classes\Applications\${APP_EXE}" "FriendlyAppName" "CineWindows"
    WriteRegStr HKCU "Software\Classes\Applications\${APP_EXE}" "ApplicationDescription" "Modern high-performance media player"
    WriteRegStr HKCU "Software\Classes\Applications\${APP_EXE}" "ApplicationIcon" "$INSTDIR\${APP_EXE},0"
    WriteRegStr HKCU "Software\Classes\Applications\${APP_EXE}\shell\open\command" "" "$\"$INSTDIR\${APP_EXE}$\" $\"%1$\""

    WriteRegStr HKCU "${APP_CAPABILITIES}" "ApplicationName" "CineWindows"
    WriteRegStr HKCU "${APP_CAPABILITIES}" "ApplicationDescription" "Modern high-performance media player"
    WriteRegStr HKCU "${APP_CAPABILITIES}" "ApplicationIcon" "$INSTDIR\${APP_EXE},0"
    !insertmacro RegisterMediaExtension ".264"
    !insertmacro RegisterMediaExtension ".265"
    !insertmacro RegisterMediaExtension ".3g2"
    !insertmacro RegisterMediaExtension ".3gp"
    !insertmacro RegisterMediaExtension ".3gpp"
    !insertmacro RegisterMediaExtension ".aac"
    !insertmacro RegisterMediaExtension ".ac3"
    !insertmacro RegisterMediaExtension ".aif"
    !insertmacro RegisterMediaExtension ".aiff"
    !insertmacro RegisterMediaExtension ".ape"
    !insertmacro RegisterMediaExtension ".asf"
    !insertmacro RegisterMediaExtension ".ass"
    !insertmacro RegisterMediaExtension ".avi"
    !insertmacro RegisterMediaExtension ".av1"
    !insertmacro RegisterMediaExtension ".bmp"
    !insertmacro RegisterMediaExtension ".cue"
    !insertmacro RegisterMediaExtension ".dff"
    !insertmacro RegisterMediaExtension ".divx"
    !insertmacro RegisterMediaExtension ".dsf"
    !insertmacro RegisterMediaExtension ".dts"
    !insertmacro RegisterMediaExtension ".eac3"
    !insertmacro RegisterMediaExtension ".flac"
    !insertmacro RegisterMediaExtension ".flv"
    !insertmacro RegisterMediaExtension ".gif"
    !insertmacro RegisterMediaExtension ".heic"
    !insertmacro RegisterMediaExtension ".heif"
    !insertmacro RegisterMediaExtension ".ifo"
    !insertmacro RegisterMediaExtension ".iso"
    !insertmacro RegisterMediaExtension ".ivf"
    !insertmacro RegisterMediaExtension ".jpeg"
    !insertmacro RegisterMediaExtension ".jpg"
    !insertmacro RegisterMediaExtension ".m2ts"
    !insertmacro RegisterMediaExtension ".m4a"
    !insertmacro RegisterMediaExtension ".m4b"
    !insertmacro RegisterMediaExtension ".m4v"
    !insertmacro RegisterMediaExtension ".mka"
    !insertmacro RegisterMediaExtension ".mkv"
    !insertmacro RegisterMediaExtension ".mov"
    !insertmacro RegisterMediaExtension ".mp3"
    !insertmacro RegisterMediaExtension ".mp4"
    !insertmacro RegisterMediaExtension ".mpeg"
    !insertmacro RegisterMediaExtension ".mpg"
    !insertmacro RegisterMediaExtension ".mts"
    !insertmacro RegisterMediaExtension ".ogg"
    !insertmacro RegisterMediaExtension ".ogm"
    !insertmacro RegisterMediaExtension ".ogv"
    !insertmacro RegisterMediaExtension ".opus"
    !insertmacro RegisterMediaExtension ".png"
    !insertmacro RegisterMediaExtension ".ra"
    !insertmacro RegisterMediaExtension ".rm"
    !insertmacro RegisterMediaExtension ".rmvb"
    !insertmacro RegisterMediaExtension ".srt"
    !insertmacro RegisterMediaExtension ".ssa"
    !insertmacro RegisterMediaExtension ".sub"
    !insertmacro RegisterMediaExtension ".sup"
    !insertmacro RegisterMediaExtension ".ts"
    !insertmacro RegisterMediaExtension ".vob"
    !insertmacro RegisterMediaExtension ".wav"
    !insertmacro RegisterMediaExtension ".webm"
    !insertmacro RegisterMediaExtension ".webp"
    !insertmacro RegisterMediaExtension ".wma"
    !insertmacro RegisterMediaExtension ".wmv"
    !insertmacro RegisterMediaExtension ".xspf"
    !insertmacro RegisterMediaExtension ".m3u"
    !insertmacro RegisterMediaExtension ".m3u8"
    !insertmacro RegisterMediaExtension ".pls"
    !insertmacro RegisterMediaExtension ".strm"
    !insertmacro RegisterMediaExtension ".url"
    WriteRegStr HKCU "Software\RegisteredApplications" "CineWindows" "${APP_CAPABILITIES}"
    System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
SectionEnd

Section "Start Menu shortcuts" SecStartMenu
    SetShellVarContext current
    CreateDirectory "$SMPROGRAMS\CineWindows"
    CreateShortcut "$SMPROGRAMS\CineWindows\CineWindows.lnk" "$INSTDIR\${APP_EXE}"
    CreateShortcut "$SMPROGRAMS\CineWindows\Uninstall CineWindows.lnk" "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Desktop shortcut" SecDesktop
    SetShellVarContext current
    CreateShortcut "$DESKTOP\CineWindows.lnk" "$INSTDIR\${APP_EXE}"
SectionEnd

Section "Uninstall"
    SetShellVarContext current
    Delete "$DESKTOP\CineWindows.lnk"
    Delete "$SMPROGRAMS\CineWindows\CineWindows.lnk"
    Delete "$SMPROGRAMS\CineWindows\Uninstall CineWindows.lnk"
    RMDir "$SMPROGRAMS\CineWindows"
    RMDir /r "$INSTDIR"
    DeleteRegValue HKCU "Software\RegisteredApplications" "CineWindows"
    DeleteRegKey HKCU "Software\Classes\Applications\${APP_EXE}"
    DeleteRegKey HKCU "Software\Classes\${APP_PROGID}"
    DeleteRegKey HKCU "${APP_CAPABILITIES}"
    DeleteRegKey HKCU "${APP_REGKEY}"
    DeleteRegKey /ifempty HKCU "Software\CineWindows"
    System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
SectionEnd

LangString DESC_SecCore ${LANG_ENGLISH} "Install the CineWindows media player and required runtime files."
LangString DESC_SecStartMenu ${LANG_ENGLISH} "Create Start Menu shortcuts for CineWindows."
LangString DESC_SecDesktop ${LANG_ENGLISH} "Create a desktop shortcut."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} $(DESC_SecCore)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecStartMenu} $(DESC_SecStartMenu)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop} $(DESC_SecDesktop)
!insertmacro MUI_FUNCTION_DESCRIPTION_END
