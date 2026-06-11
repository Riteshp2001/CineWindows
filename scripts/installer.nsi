Unicode True
RequestExecutionLevel admin

!include "MUI2.nsh"

!ifndef APP_VERSION
  !define APP_VERSION "1.0.1"
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
InstallDirRegKey HKLM "Software\${APP_NAME}" ""

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

Section "CineWindows (required)" SecApp
  SectionIn RO

  SetOutPath "$INSTDIR"
  File /r "..\dist\CineWindows\*.*"

  WriteRegStr HKLM "Software\${APP_NAME}" "" "$INSTDIR"

  CreateDirectory "$SMPROGRAMS\${APP_NAME}"
  CreateShortCut "$SMPROGRAMS\${APP_NAME}\CineWindows.lnk" "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_ICO}"
  CreateShortCut "$SMPROGRAMS\${APP_NAME}\Uninstall CineWindows.lnk" "$INSTDIR\uninstall.exe"

  WriteUninstaller "$INSTDIR\uninstall.exe"

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName" "${APP_NAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayVersion" "${APP_VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "Publisher" "${APP_PUBLISHER}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "URLInfoAbout" "${APP_URL}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayIcon" "$INSTDIR\${APP_ICO}"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoRepair" 1
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
  Delete "$SMPROGRAMS\${APP_NAME}\CineWindows.lnk"
  Delete "$SMPROGRAMS\${APP_NAME}\Uninstall CineWindows.lnk"
  RMDir "$SMPROGRAMS\${APP_NAME}"
  Delete "$DESKTOP\CineWindows.lnk"

  RMDir /r "$INSTDIR"

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
  DeleteRegKey HKLM "Software\${APP_NAME}"
SectionEnd
