; example2.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install example2.nsi into a directory that the user selects,

;--------------------------------

; The name of the installer
Name "ASCEND PyGTK"

; The file to write
!ifdef OUTFILE
OutFile ${OUTFILE}.exe
!else
OutFile ascend-setup.exe
!endif


SetCompressor /SOLID lzma

; The default installation directory
InstallDir $PROGRAMFILES\ASCEND

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\ASCEND" "Install_Dir"

;--------------------------------

; Pages

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "ASCEND (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Python interface
  File "_ascpy.dll"
  File "..\ascend.dll"
  File "config.py"
  File "*.py"
  SetOutPath $INSTDIR\glade
  File "glade\*.glade"
  File "glade\*.png"

  ; Model Library
  SetOutPath $INSTDIR\models
  File /r "..\models\*.a4*"
  File /r "..\models\*.bz2"

  ; Tcl/Tk interface
  SetOutPath $INSTDIR\tcltk
  File /r "..\tcltk\TK\*"
  SetOutPath $INSTDIR
  File "..\tcltk\generic\interface\ascendtcl.dll"
  File "..\tcltk\generic\interface\ascend4.exe"

  SetOutPath $INSTDIR
  File "Makefile.bt"
  File "ascend.syn"

  ; Set 'librarypath' in .ascend.ini
  WriteINIstr $APPDATA\.ascend.ini Directories librarypath "$DOCUMENTS\ascdata;$INSTDIR\models"

  ; Create 'ascend.bat' launcher for PyGTK interface
  ClearErrors
  FileOpen $0 $INSTDIR\ascend.bat w
  IfErrors done
  FileWrite $0 "@echo off"
  FileWriteByte $0 "13"
  FileWriteByte $0 "10"
  FileWrite $0 "set PATH=c:\Python24;c:\GTK\bin"
  FileWriteByte $0 "13"
  FileWriteByte $0 "10"
  FileWrite $0 "cd "
  FileWrite $0 $INSTDIR 
  FileWriteByte $0 "13"
  FileWriteByte $0 "10"
  FileWrite $0 "c:\Python24\python gtkbrowser.py %%"
  FileWriteByte $0 "13"
  FileWriteByte $0 "10"

  FileClose $0
  done:
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\ASCEND "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ASCEND" "DisplayName" "ASCEND Simulation Environment"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ASCEND" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ASCEND" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ASCEND" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\ASCEND"
  CreateShortCut "$SMPROGRAMS\ASCEND\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\ASCEND\ASCEND.lnk" "$INSTDIR\ascend.bat" "" "$INSTDIR\ascend.bat" 0
  CreateShortCut "$SMPROGRAMS\ASCEND\ASCEND Tcl/Tk.lnk" "$INSTDIR\ascend4.exe" "" "$INSTDIR\ascend4.exe" 0
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ASCEND"
  DeleteRegKey HKLM SOFTWARE\ASCEND

  ; Remove files and uninstaller
  Delete $INSTDIR\_ascpy.dll
  Delete $INSTDIR\ascendtcl.dll
  Delete $INSTDIR\ascend.dll
  Delete $INSTDIR\ascend.bat
  Delete $INSTDIR\*.py
  Delete $INSTDIR\glade\*
  Delete $INSTDIR\Makefile.bt
  Delete $INSTDIR\ascend.syn
  Delete $INSTDIR\models\*
  Delete $INSTDIR\models\johnpye\*
  RMDir /r $INSTDIR\tcltk
  RMDir $INSTDIR\models\johnpye
  RMDIR $INSTDIR\models

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\ASCEND\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\ASCEND"
  RMDir "$INSTDIR"

SectionEnd
