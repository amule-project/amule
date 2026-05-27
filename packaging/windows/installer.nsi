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
!insertmacro MUI_UNPAGE_COMPONENTS
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

; If a previous install exists at a recorded location, uninstall it
; silently first so stale DLLs / removed files from the older version
; don't linger. NSIS's File /r overwrites but never deletes; without
; this an upgrade leaves orphaned binaries behind.
Function CheckPriorInstall
  ReadRegStr $0 HKLM "Software\aMule" "InstallLocation"
  ${If} $0 != ""
  ${AndIf} ${FileExists} "$0\Uninstall.exe"
    DetailPrint "Removing previous aMule installation at $0..."
    ; /S = silent; _?=$0 keeps the uninstaller from copying itself to
    ; %TEMP% and detaching, so ExecWait actually waits and the user
    ; data prompt is suppressed (silent runs default to the section's
    ; default state — un.SecRemoveUserData is /o, i.e. unchecked).
    ExecWait '"$0\Uninstall.exe" /S _?=$0' $1
    ${If} $1 != 0
      MessageBox MB_OK|MB_ICONSTOP \
        "Could not remove the previous aMule installation at $0.$\r$\n\
        Please close any running aMule processes and try again, or \
        uninstall the previous version manually first."
      Abort
    ${EndIf}
    ; The uninstaller can't delete its own .exe while running; clean
    ; up the stub and its parent dir if it's now empty.
    Delete "$0\Uninstall.exe"
    RMDir  "$0"
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

  Call CheckPriorInstall
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

; The first uninstaller section must be named "Uninstall" — NSIS uses
; that as the entry point. `un.SecUninstall` ID lets the Components
; page reference it for its description; SectionIn RO grays it out so
; it can't be unchecked.
Section "Uninstall" un.SecUninstall
  SectionIn RO

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

; Opt-in: nuke %APPDATA%\aMule. Default off so a normal uninstall is
; "remove the app, keep my data" (the common Windows convention). The
; `un.` ID prefix associates this with the uninstaller block.
;
; SetShellVarContext current here resolves $APPDATA to the elevated
; user's profile, NOT All Users (the latter is wrong for per-user
; config). On a single-admin machine the elevated context IS the user
; who installed; multi-user machines need each user to run the
; uninstaller themselves to clean their own profile (unavoidable, no
; way for an elevated process to enumerate every desktop session).
Section /o "Remove user data (config, ED2K servers, Kad nodes, partfiles)" un.SecRemoveUserData
  SetShellVarContext current
  ${If} ${FileExists} "$APPDATA\aMule"
    DetailPrint "Removing user data at $APPDATA\aMule..."
    RMDir /r "$APPDATA\aMule"
  ${EndIf}
  SetShellVarContext all
SectionEnd

!insertmacro MUI_UNFUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${un.SecUninstall} \
    "Remove aMule application files, Start Menu / desktop shortcuts, autostart Run-key entry, and Add/Remove Programs entry (required)."
  !insertmacro MUI_DESCRIPTION_TEXT ${un.SecRemoveUserData} \
    "Permanently delete %APPDATA%\aMule for the current user (aMule.conf, ED2K server list, Kad nodes, partfiles, IP filters, friends list). Leave unchecked to keep your settings."
!insertmacro MUI_UNFUNCTION_DESCRIPTION_END

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
