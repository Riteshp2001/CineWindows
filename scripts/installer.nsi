Unicode True
RequestExecutionLevel admin

SetCompressor /SOLID lzma
SetCompressorDictSize 64

!include "MUI2.nsh"
!include "x64.nsh"
!include "LogicLib.nsh"
!include "WordFunc.nsh"

!ifndef APP_VERSION
  !define APP_VERSION "1.0.2"
!endif

!ifndef APP_VERSION_QUAD
  !define APP_VERSION_QUAD "1.0.2.0"
!endif

!define APP_NAME "CineWindows"
!define APP_PUBLISHER "gyrolet"
!define APP_URL "https://github.com/Riteshp2001/CineWindows"
!define APP_EXE "CineWindows.exe"
!define APP_ICO "CineWindows.ico"
!define APP_ICO_PATH "..\CineWindows.ico"

Name "${APP_NAME} ${APP_VERSION}"
OutFile "..\CineWindows-${APP_VERSION}-windows-x64.exe"
InstallDir "$PROGRAMFILES64\${APP_NAME}"
InstallDirRegKey HKLM "Software\${APP_NAME}" "InstallDir"

VIProductVersion "${APP_VERSION_QUAD}"
VIAddVersionKey "ProductName" "${APP_NAME}"
VIAddVersionKey "ProductVersion" "${APP_VERSION}"
VIAddVersionKey "CompanyName" "${APP_PUBLISHER}"
VIAddVersionKey "FileDescription" "${APP_NAME} Installer"
VIAddVersionKey "FileVersion" "${APP_VERSION}"

!define MUI_ABORTWARNING
!define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\win.bmp"
!define MUI_ICON "${APP_ICO_PATH}"
!define MUI_UNICON "${APP_ICO_PATH}"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_RUN "$INSTDIR\${APP_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT "Launch CineWindows"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_ICONSTOP "This installer is for 64-bit Windows only."
    Abort
  ${EndIf}

  SetRegView 64
  SetShellVarContext all
FunctionEnd

Function un.onInit
  SetRegView 64
  SetShellVarContext all
FunctionEnd

!define MEDIA_EXTS \
  ".3gp .3g2 .asf .avi .avif .divx .f4v .flv .h264 .h265 .hevc \
   .m2ts .m2v .m4v .mkv .mov .mp4 .mpeg .mpg .mts .mxf .ogv \
   .rm .rmvb .ts .vob .webm .wmv .wtv \
   .aac .ac3 .aiff .alac .ape .au .dts .dtshd .eac3 .flac \
   .m4a .mid .midi .mp3 .oga .ogg .opus .ra .wav .wma \
   .gif .jpg .jpeg .png .apng .webp .bmp .tiff .tif"

Section "CineWindows (required)" SecApp
  SectionIn RO

  SetOutPath "$INSTDIR"

  File /r "..\dist\CineWindows\*.*"
  File /oname=${APP_ICO} "${APP_ICO_PATH}"

  WriteUninstaller "$INSTDIR\uninstall.exe"

  WriteRegStr HKLM "Software\${APP_NAME}" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "Software\${APP_NAME}" "Version" "${APP_VERSION}"

  CreateDirectory "$SMPROGRAMS\${APP_NAME}"
  CreateShortCut "$SMPROGRAMS\${APP_NAME}\CineWindows.lnk" "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_ICO}"
  CreateShortCut "$SMPROGRAMS\${APP_NAME}\Uninstall CineWindows.lnk" "$INSTDIR\uninstall.exe"

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName" "${APP_NAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayVersion" "${APP_VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "Publisher" "${APP_PUBLISHER}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "URLInfoAbout" "${APP_URL}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayIcon" "$INSTDIR\${APP_ICO}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoRepair" 1

  ; Register "Open with CineWindows" in Windows Explorer context menu
  WriteRegStr HKLM "SOFTWARE\Classes\Applications\${APP_EXE}" "FriendlyAppName" "${APP_NAME}"
  WriteRegStr HKLM "SOFTWARE\Classes\Applications\${APP_EXE}\shell\open\command" "" '"$INSTDIR\${APP_EXE}" "%1"'

  StrCpy $0 1
loop_exts:
  ${WordFind} "${MEDIA_EXTS}" " " "E+$0" $1
  IfErrors done_exts
  WriteRegStr HKLM "SOFTWARE\Classes\Applications\${APP_EXE}\SupportedTypes" "$1" ""
  WriteRegStr HKLM "SOFTWARE\Classes\$1\OpenWithProgids" "CineWindows.Media" ""
  IntOp $0 $0 + 1
  Goto loop_exts
done_exts:

  ; Register ProgId so CineWindows can be set as default for media files
  WriteRegStr HKLM "SOFTWARE\Classes\CineWindows.Media\DefaultIcon" "" "$INSTDIR\${APP_ICO}"
  WriteRegStr HKLM "SOFTWARE\Classes\CineWindows.Media\shell\open\command" "" '"$INSTDIR\${APP_EXE}" "%1"'
SectionEnd

Section "Desktop Shortcut" SecDesktop
  CreateShortCut "$DESKTOP\CineWindows.lnk" "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_ICO}"
SectionEnd

LangString DESC_SecApp ${LANG_ENGLISH} "CineWindows core application files, launcher, and runtime."
LangString DESC_SecDesktop ${LANG_ENGLISH} "Add a shortcut to CineWindows on your desktop."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecApp} $(DESC_SecApp)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop} $(DESC_SecDesktop)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Section "Uninstall"
  SetRegView 64
  SetShellVarContext all

  Delete "$SMPROGRAMS\${APP_NAME}\CineWindows.lnk"
  Delete "$SMPROGRAMS\${APP_NAME}\Uninstall CineWindows.lnk"
  RMDir "$SMPROGRAMS\${APP_NAME}"

  Delete "$DESKTOP\CineWindows.lnk"

  RMDir /r "$INSTDIR"

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
  DeleteRegKey HKLM "Software\${APP_NAME}"

  ; Remove file association registrations
  DeleteRegKey HKLM "SOFTWARE\Classes\Applications\${APP_EXE}"
  DeleteRegKey HKLM "SOFTWARE\Classes\CineWindows.Media"
SectionEnd