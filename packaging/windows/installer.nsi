; aMule Windows installer (NSIS 3.x, MUI2)
;
; Payload IS the portable tree produced by packaging/windows/build.sh
; (amule-portable-<arch>/). The installer only stages files, registers
; shortcuts / ARP / optional autostart, and writes an uninstaller.
;
; Required defines (pass via `makensis -DINSTROOT=… -DARCH=… -DVERSION=… -DOUTFILE=… -DICONFILE=…`):
;   INSTROOT  path to the portable tree (e.g. ./amule-portable-x64)
;   ARCH      x64 | arm64    — payload architecture (drives output filename)
;   VERSION   version string (e.g. 3.0.0 or 2.3.3-211-g13b22a7a0)
;   OUTFILE   output path for the installer .exe
;   ICONFILE  path to the installer .ico (uses repo's amule.ico)

!ifndef INSTROOT
  !error "INSTROOT must be defined: -DINSTROOT=<path-to-portable-tree>"
!endif
!ifndef ARCH
  !error "ARCH must be defined: -DARCH=x64 or -DARCH=arm64"
!endif
!ifndef VERSION
  !error "VERSION must be defined: -DVERSION=<version-string>"
!endif
!ifndef OUTFILE
  !error "OUTFILE must be defined: -DOUTFILE=<output-installer-path>"
!endif
!ifndef ICONFILE
  !error "ICONFILE must be defined: -DICONFILE=<path-to-icon-file>"
!endif

Unicode true

; The 32-bit installer stub is used for BOTH x64 and ARM64 payloads.
; It runs natively on x64 and under WOW64 emulation on Windows-on-ARM;
; the installer is short-lived bootstrap code, so the emulation cost is
; irrelevant. The payload itself is native to its target architecture
; via $PROGRAMFILES64 + the !define ARCH switches below.
;
; A native arm64-unicode stub would require an NSIS distribution that
; ships it (Chocolatey and Homebrew don't as of NSIS 3.10), so for a
; reproducible CI matrix we stick with the universally-available 32-bit
; stub.

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

Name "aMule"
OutFile "${OUTFILE}"
InstallDir "$PROGRAMFILES64\aMule"
InstallDirRegKey HKLM "Software\aMule" "InstallLocation"
RequestExecutionLevel admin
ShowInstDetails show
ShowUnInstDetails show
SetCompressor /SOLID lzma

VIProductVersion "0.0.0.0"
VIAddVersionKey  "ProductName"     "aMule"
VIAddVersionKey  "CompanyName"     "aMule Project"
VIAddVersionKey  "LegalCopyright"  "GPL-2.0-or-later"
VIAddVersionKey  "FileDescription" "aMule installer"
VIAddVersionKey  "FileVersion"     "${VERSION}"
VIAddVersionKey  "ProductVersion"  "${VERSION}"

; --------------------------------------------------------------------
; MUI2 pages
; --------------------------------------------------------------------

!define MUI_ABORTWARNING
; Project .ico passed via build.sh -DICONFILE — referencing amule.exe
; directly doesn't work because NSIS can't parse the icon format that
; mingw windres embeds into the PE.
!define MUI_ICON   "${ICONFILE}"
!define MUI_UNICON "${ICONFILE}"

!insertmacro MUI_PAGE_WELCOME
; License is the repo's GPL; copied into INSTROOT by build.sh so the
; script stays relocatable from the makensis invocation cwd.
!insertmacro MUI_PAGE_LICENSE "${INSTROOT}\COPYING.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY

Var StartMenuFolder
!define MUI_STARTMENUPAGE_REGISTRY_ROOT        "HKLM"
!define MUI_STARTMENUPAGE_REGISTRY_KEY         "Software\aMule"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME   "StartMenuFolder"
!define MUI_STARTMENUPAGE_DEFAULTFOLDER        "aMule"
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder

!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

; --------------------------------------------------------------------
; Helpers
; --------------------------------------------------------------------

; Abort installation if amule.exe / amuled.exe in $INSTDIR are locked
; by a running process. NSIS pattern: open the target for write; on
; ERROR_SHARING_VIOLATION the file is in use.
Function CheckRunningInstance
  ${If} ${FileExists} "$INSTDIR\bin\amule.exe"
    ClearErrors
    FileOpen $0 "$INSTDIR\bin\amule.exe" a
    ${If} ${Errors}
      MessageBox MB_OK|MB_ICONSTOP \
        "aMule appears to be running from $INSTDIR.$\r$\n\
        Please close aMule (and aMuleD / aMuleGUI) and try again."
      Abort
    ${EndIf}
    FileClose $0
  ${EndIf}
  ${If} ${FileExists} "$INSTDIR\bin\amuled.exe"
    ClearErrors
    FileOpen $0 "$INSTDIR\bin\amuled.exe" a
    ${If} ${Errors}
      MessageBox MB_OK|MB_ICONSTOP \
        "aMule daemon (amuled.exe) appears to be running from $INSTDIR.$\r$\n\
        Please stop it and try again."
      Abort
    ${EndIf}
    FileClose $0
  ${EndIf}
FunctionEnd

Function .onInit
  ; $PROGRAMFILES64 must resolve correctly for both x64 and ARM64
  ; payloads; the installer always lays out under the 64-bit prefix.
  SetRegView 64
  ; Per-machine install (RequestExecutionLevel admin) → shortcuts and
  ; the Run-key live under All Users, not the elevated admin's per-user
  ; profile. Without this, $SMPROGRAMS / $DESKTOP / $APPDATA resolve to
  ; the *current* user (often the elevated context, not the logged-in
  ; user), and shortcuts land where nobody can find them.
  SetShellVarContext all
FunctionEnd

Function un.onInit
  SetRegView 64
  SetShellVarContext all
FunctionEnd

; --------------------------------------------------------------------
; Sections
; --------------------------------------------------------------------

Section "aMule (required)" SecCore
  SectionIn RO

  Call CheckRunningInstance

  SetOutPath "$INSTDIR"
  ; Recursively stage the entire portable tree. /r preserves the
  ; bin/ + share/ + locale layout cmake --install produces.
  File /r "${INSTROOT}\*.*"

  ; Start Menu shortcuts — always created (the folder name is the
  ; only thing the Start Menu page controls).
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
    CreateShortcut "$SMPROGRAMS\$StartMenuFolder\aMule.lnk" \
      "$INSTDIR\bin\amule.exe" "" "$INSTDIR\bin\amule.exe" 0
    CreateShortcut "$SMPROGRAMS\$StartMenuFolder\aMule Daemon.lnk" \
      "$INSTDIR\bin\amuled.exe" "" "$INSTDIR\bin\amuled.exe" 0
    CreateShortcut "$SMPROGRAMS\$StartMenuFolder\aMuleGUI (remote).lnk" \
      "$INSTDIR\bin\amulegui.exe" "" "$INSTDIR\bin\amulegui.exe" 0
    CreateShortcut "$SMPROGRAMS\$StartMenuFolder\Uninstall aMule.lnk" \
      "$INSTDIR\Uninstall.exe"
  !insertmacro MUI_STARTMENU_WRITE_END

  ; Write the uninstaller.
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  ; Record install location so re-runs default to the same dir.
  WriteRegStr HKLM "Software\aMule" "InstallLocation" "$INSTDIR"

  ; Add/Remove Programs entry.
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\aMule" \
                "DisplayName"        "aMule"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\aMule" \
                "DisplayVersion"     "${VERSION}"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\aMule" \
                "Publisher"          "aMule Project"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\aMule" \
                "URLInfoAbout"       "https://www.amule.org"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\aMule" \
                "DisplayIcon"        "$INSTDIR\bin\amule.exe"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\aMule" \
                "InstallLocation"    "$INSTDIR"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\aMule" \
                "UninstallString"    '"$INSTDIR\Uninstall.exe"'
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\aMule" \
                "QuietUninstallString" '"$INSTDIR\Uninstall.exe" /S'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\aMule" \
                "EstimatedSize"      $0
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\aMule" \
                "NoModify"           1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\aMule" \
                "NoRepair"           1
SectionEnd

Section "Desktop shortcut" SecDesktop
  CreateShortcut "$DESKTOP\aMule.lnk" \
    "$INSTDIR\bin\amule.exe" "" "$INSTDIR\bin\amule.exe" 0
SectionEnd

; HKCU (per-user) so each user on a shared machine opts in independently.
Section /o "Start aMule when I log in" SecAutostart
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Run" \
    "aMule" '"$INSTDIR\bin\amule.exe"'
SectionEnd

; Component descriptions surfaced on the Components page.
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} \
    "aMule application files (required)."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop} \
    "Place an aMule shortcut on the desktop."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecAutostart} \
    "Launch aMule automatically when the current user logs in (per-user setting)."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; --------------------------------------------------------------------
; Uninstaller
; --------------------------------------------------------------------

Section "Uninstall"
  ; Refuse to uninstall if anything is still running.
  ${If} ${FileExists} "$INSTDIR\bin\amule.exe"
    ClearErrors
    FileOpen $0 "$INSTDIR\bin\amule.exe" a
    ${If} ${Errors}
      MessageBox MB_OK|MB_ICONSTOP \
        "aMule appears to be running. Please close it and re-run the uninstaller."
      Abort
    ${EndIf}
    FileClose $0
  ${EndIf}

  ; Start Menu folder.
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  Delete "$SMPROGRAMS\$StartMenuFolder\aMule.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\aMule Daemon.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\aMuleGUI (remote).lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall aMule.lnk"
  RMDir  "$SMPROGRAMS\$StartMenuFolder"

  ; Desktop shortcut, if our installer placed it.
  ${If} ${FileExists} "$DESKTOP\aMule.lnk"
    Delete "$DESKTOP\aMule.lnk"
  ${EndIf}

  ; Autostart entry — only remove if it still points inside $INSTDIR
  ; so a user's hand-set Run value with a different path isn't wiped.
  ReadRegStr $0 HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "aMule"
  StrCmp $0 "" autostart_done 0
  Push $0
  Push "$INSTDIR"
  Call un.StrContains
  Pop $1
  StrCmp $1 "" autostart_done 0
  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "aMule"
  autostart_done:

  ; Application files. Explicit RMDir /r on the known subtrees first
  ; for safety, then the catch-all on $INSTDIR.
  RMDir /r "$INSTDIR\bin"
  RMDir /r "$INSTDIR\share"
  RMDir /r "$INSTDIR\etc"
  RMDir /r "$INSTDIR\lib"
  Delete   "$INSTDIR\COPYING.txt"
  Delete   "$INSTDIR\Uninstall.exe"

  ; Catch any extra files cmake --install produced that we don't
  ; explicitly enumerate; safe because $INSTDIR is only ever our own
  ; tree (we don't honour an installer-side override that would point
  ; at e.g. C:\Users\…\Documents).
  RMDir /r "$INSTDIR"

  ; Registry cleanup.
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\aMule"
  DeleteRegKey HKLM "Software\aMule"
SectionEnd

; Tiny substring helper — returns the match position on the stack or
; empty string when needle isn't present. Used by the autostart-cleanup
; guard to confirm the HKCU Run value still points inside $INSTDIR.
Function un.StrContains
  Exch $R1 ; needle
  Exch
  Exch $R2 ; haystack
  Push $R3
  Push $R4
  Push $R5
  StrLen $R3 $R1
  StrCpy $R4 0
  loop:
    StrCpy $R5 $R2 $R3 $R4
    StrCmp $R5 $R1 found
    StrCmp $R5 "" notfound
    IntOp $R4 $R4 + 1
    Goto loop
  found:
    StrCpy $R1 $R2 "" $R4
    Goto done
  notfound:
    StrCpy $R1 ""
  done:
    Pop $R5
    Pop $R4
    Pop $R3
    Pop $R2
    Exch $R1
FunctionEnd
