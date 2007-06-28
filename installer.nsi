; NSIS script to create an ASCEND binary installer for Windows
; by John Pye, 2006-2007.
;
;--------------------------------

; The name of the installer

!ifndef VERSION
!define VERSION 0.svn
!endif

Name "ASCEND ${VERSION}"

!include LogicLib.nsh

!ifndef PYVERSION
!define PYVERSION "2.5"
!endif

; The file to write
!ifdef OUTFILE
OutFile ${OUTFILE}
!else
OutFile "ascend-${VERSION}-py${PYVERSION}.exe"
!endif

;SetCompressor /FINAL zlib
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

Var /GLOBAL DEFAULTPATH
Var /GLOBAL PYOK
Var /GLOBAL PYPATH
Var /GLOBAL GTKOK
Var /GLOBAL GTKPATH
Var /GLOBAL GLADEOK
Var /GLOBAL PYGTKOK
Var /GLOBAL PYGOBJECTOK
Var /GLOBAL PYCAIROOK
Var /GLOBAL GLADEPATH
Var /GLOBAL PYINSTALLED
Var /GLOBAL TCLOK
Var /GLOBAL TCLPATH
Var /GLOBAL TCLINSTALLED
Var /GLOBAL PATH

Function .onInit
	StrCpy $PYINSTALLED ""
	StrCpy $TCLINSTALLED ""
	
	ExpandEnvStrings $DEFAULTPATH "%WINDIR%;%WINDIR%\system32"

	Call DetectPython
	Pop $PYOK
	Pop $PYPATH
	
	Call DetectGTK
	Pop $GTKOK
	Pop $GTKPATH

	Call DetectGlade
	Pop $GLADEOK
	Pop $GLADEPATH	
	
	Call DetectTcl
	Pop $TCLOK
	Pop $TCLPATH
	
	Call DetectPyGTK
	Pop $PYGTKOK

	Call DetectPyGObject
	Pop $PYGOBJECTOK

	Call DetectPyCairo
	Pop $PYCAIROOK
	
	StrCpy $PATH "$DEFAULTPATH;$PYPATH;$GTKPATH"

FunctionEnd


; The stuff to install
Section "ASCEND (required)"
	SectionIn RO

	DetailPrint "--- COMMON FILES ---"

	; Set output path to the installation directory.
	SetOutPath $INSTDIR
	File "ascend.dll"
	File "ascend-config"
	File "pygtk\glade\ascend.ico"
	File "LICENSE.txt"
	File "CHANGELOG.txt"
	File "README-windows.txt"
	
	; Model Library
	SetOutPath $INSTDIR\models
	File /r /x .svn "models\*.a4*"
	File /r /x .svn "models\*.tcl"
	File /r /x .svn "models\*.dll" ; extension modules
	File /r /x .svn "models\*.py"; python modules
	
	SetOutPath $INSTDIR\solvers
	File "solvers\qrslv\qrslv.dll"
	File "solvers\conopt\conopt.dll"
	File "solvers\lrslv\lrslv.dll"
	File "solvers\cmslv\cmslv.dll"
	File "solvers\lsode\lsode.dll"
;	File "solvers\ida\ida.dll"

	SetOutPath $INSTDIR
	;File "Makefile.bt"
	File "tools\textpad\ascend.syn"

	${If} ${FileExists} "$APPDATA\.ascend.ini"
		MessageBox MB_OK "The '$APPDATA\.ascend.ini' is NOT being updated. Manually delete this file if ASCEND doesn't behave as expected."
	${Else}
		; Set 'librarypath' in .ascend.ini
		WriteINIstr $APPDATA\.ascend.ini Directories librarypath "$DOCUMENTS\ascdata;$INSTDIR\models"
	${EndIf}

	; Write the installation path into the registry
	WriteRegStr HKLM SOFTWARE\ASCEND "Install_Dir" "$INSTDIR"

	; Write the uninstall keys for Windows
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ASCEND" "DisplayName" "ASCEND Simulation Environment"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ASCEND" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ASCEND" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ASCEND" "NoRepair" 1
	WriteUninstaller "uninstall.exe"

	; Write file locations to the registry for access from ascend-config
	WriteRegStr HKLM SOFTWARE\ASCEND "INSTALL_LIB" "$INSTDIR"
	WriteRegStr HKLM SOFTWARE\ASCEND "INSTALL_BIN" "$INSTDIR"
	WriteRegStr HKLM SOFTWARE\ASCEND "INSTALL_INCLUDE" "$INSTDIR\include"
	WriteRegStr HKLM SOFTWARE\ASCEND "INSTALL_ASCDATA" "$INSTDIR"
	WriteRegStr HKLM SOFTWARE\ASCEND "INSTALL_MODELS" "$INSTDIR\models"
	WriteRegStr HKLM SOFTWARE\ASCEND "INSTALL_SOLVERS" "$INSTDIR\solvers"
	WriteRegStr HKLM SOFTWARE\ASCEND "GTKLIBS" "$GTKPATH"

	; Create 'ascend-config.bat' batch file for launching the python script 'ascend-config'.
	ClearErrors
	FileOpen $0 $INSTDIR\ascend-config.bat w
	IfErrors ascendconfigerror
	FileWrite $0 "@echo off"
	FileWriteByte $0 "13"
	FileWriteByte $0 "10"
	FileWrite $0 "set PATH=$PATH"
	FileWriteByte $0 "13"
	FileWriteByte $0 "10"
	FileWrite $0 "cd "
	FileWrite $0 $INSTDIR 
	FileWriteByte $0 "13"
	FileWriteByte $0 "10"
	FileWrite $0 "$PYPATH\python "
	FileWriteByte $0 "34" 
	FileWrite $0 "$INSTDIR\ascend-config"
	FileWriteByte $0 "34"
	FileWrite $0 " %1 %2 %3 %4 %5 %6 %7 %8"
	FileWriteByte $0 "13"
	FileWriteByte $0 "10"

	FileClose $0

	Return
ascendconfigerror:
	MessageBox MB_OK "The 'ascend-config.bat' file was not installed properly; problems writing to that file."	
	
SectionEnd

;--------------------------------

Section "PyGTK GUI"
	; Check the dependencies of the PyGTK GUI before proceding...
	${If} $PYOK == 'NOK'
		MessageBox MB_OK "PyGTK GUI can not be installed, because Python was not found on this system.$\nIf you do want to use the PyGTK GUI, please check the installation instructions$\n$\n(PYPATH=$PYPATH)"
	${ElseIf} $GTKOK == 'NOK'
		MessageBox MB_OK "PyGTK GUI cannot be installed, because GTK+ 2.x was not found on this system.$\nIf you do want to use the PyGTK GUI, please check the installation instructions$\n$\n(GTKPATH=$GTKPATH)"
	${ElseIf} $GLADEOK == 'NOK'
		MessageBox MB_OK "PyGTK GUI cannot be installed, because Glade 2.x was not found on this system.$\nIf you do want to use the PyGTK GUI, please check the installation instructions$\n$\n(GTKPATH=$GTKPATH)"
	${ElseIf} $PYGTKOK == "NOK"
		MessageBox MB_OK "PyGTK GUI cannot be installed, because PyGTK was not found on this system.$\nPlease check the installation instructions.$\n$\n(PYPATH=$PYPATH)"
	${ElseIf} $PYCAIROOK == "NOK"
		MessageBox MB_OK "PyGTK GUI cannot be installed, because PyCairo was not found on this system.$\nPlease check the installation instructions.$\n$\n(PYPATH=$PYPATH)"
	${ElseIf} $PYGOBJECTOK == "NOK"
		MessageBox MB_OK "PyGTK GUI cannot be installed, because PyGObject was not found on this system.$\nPlease check the installation instructions.$\n$\n(PYPATH=$PYPATH)"
	${Else}
		;MessageBox MB_OK "Python: $PYPATH, GTK: $GTKPATH"

		DetailPrint "--- PYTHON INTERFACE ---"

		; Set output path to the installation directory.
		SetOutPath $INSTDIR

		; Python interface
		File /nonfatal "pygtk\_ascpy.pyd"
		File "pygtk\*.py"
		File "pygtk\ascend"
		File "pygtk\glade\ascend-doc.ico"

		SetOutPath $INSTDIR\glade
		File "pygtk\glade\*.glade"
		File "pygtk\glade\*.png"
		File "pygtk\glade\*.svg"

		StrCpy $PYINSTALLED "1"
		WriteRegDWORD HKLM "SOFTWARE\ASCEND" "Python" 1	

		;---- file association ----

		; back up old value of .a4c file association
		ReadRegStr $1 HKCR ".a4c" ""
		StrCmp $1 "" a4cnobkp
		StrCmp $1 "ASCEND.model" a4cnobkp

		; Remember the old file association if necessary
		WriteRegStr HKLM "SOFTWARE\ASCEND" "BackupAssocA4C" $1

a4cnobkp:
		WriteRegStr HKCR ".a4c" "" "ASCEND.model"

		; back up old value of .a4c file association
		ReadRegStr $1 HKCR ".a4l" ""
		StrCmp $1 "" a4lnobkp
		StrCmp $1 "ASCEND.model" a4lnobkp

		; Remember the old file association if necessary
		WriteRegStr HKLM "SOFTWARE\ASCEND" "BackupAssocA4L" $1

a4lnobkp:
		WriteRegStr HKCR ".a4l" "" "ASCEND.model"

		; So, what does an A4L or A4C file actually do?

		ReadRegStr $0 HKCR "ASCEND.model" ""
		StrCmp $0 "" 0 a4cskip

		WriteRegStr HKCR "ASCEND.model" "" "ASCEND model file"
		WriteRegStr HKCR "ASCEND.model\shell" "" "open"
		WriteRegStr HKCR "ASCEND.model\DefaultIcon" "" "$INSTDIR\ascend-doc.ico"

a4cskip:
		WriteRegStr HKCR "ASCEND.model\shell\open\command" "" '$PYPATH\pythonw "$INSTDIR\ascend" "%1"'

		System::Call 'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'

	${EndIf}
	Return

SectionEnd

;---------------------------------

Section "Tcl/Tk GUI"

${If} $TCLOK != 'OK'
	MessageBox MB_OK "Tck/Tk GUI can not be installed, because ActiveTcl was not found on this system. If do you want to use the Tcl/Tk GUI, please check the installation instructions ($TCLPATH)"
${Else}
	DetailPrint "--- TCL/TK INTERFACE ---"
	SetOutPath $INSTDIR\tcltk
	File /r /x .svn "tcltk\TK\*"
	SetOutPath $INSTDIR
	File "tcltk\generic\interface\ascendtcl.dll"
	File "tcltk\generic\interface\ascend4.exe"
	
	StrCpy $TCLINSTALLED "1"
	WriteRegDWORD HKLM "SOFTWARE\ASCEND" "TclTk" 1

${EndIf}

SectionEnd

;---------------------------------

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"
  
  WriteRegDWORD HKLM "SOFTWARE\ASCEND" "StartMenu" 1
  
  CreateDirectory "$SMPROGRAMS\ASCEND"  

  ; Link to PyGTK GUI
  StrCmp $PYINSTALLED "" smdone 0
  CreateShortCut "$SMPROGRAMS\ASCEND\ASCEND.lnk" "$PYPATH\pythonw.exe" '"$INSTDIR\ascend"' "$INSTDIR\ascend.ico" 0
smdone:

  ; Model library shortcut
  CreateShortCut "$SMPROGRAMS\ASCEND\Model Library.lnk" "$INSTDIR\models" "" "$INSTDIR\models" 0

 
  ; Link to Tcl/Tk GUI  
  StrCmp $TCLINSTALLED "" smnotcl 0  
  CreateShortCut "$SMPROGRAMS\ASCEND\ASCEND Tcl/Tk.lnk" "$INSTDIR\ascend4.exe" "" "$INSTDIR\ascend4.exe" 0
smnotcl:

  ; Information files
  CreateShortCut "$SMPROGRAMS\ASCEND\LICENSE.lnk" "$INSTDIR\LICENSE.txt" '' "$INSTDIR\LICENSE.txt" 0
  CreateShortCut "$SMPROGRAMS\ASCEND\CHANGELOG.lnk" "$INSTDIR\CHANGELOG.txt" '' "$INSTDIR\CHANGELOG.txt" 0
  CreateShortCut "$SMPROGRAMS\ASCEND\README.lnk" "$INSTDIR\README-windows.txt" '' "$INSTDIR\README-windows.txt" 0

  CreateShortCut "$SMPROGRAMS\ASCEND\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  
SectionEnd

;------------------------------------------------------------------
; UNINSTALLER

Section "Uninstall"

;--- python components ---

	ReadRegDWORD $0 HKLM "SOFTWARE\ASCEND" "Python"
	IntCmp $0 0 unnopython unpython
  
unpython:
	DetailPrint "--- REMOVING PYTHON COMPONENTS ---"
	Delete $INSTDIR\_ascpy.pyd
	Delete $INSTDIR\ascend
	Delete $INSTDIR\*.py
	Delete $INSTDIR\*.pyc
	Delete $INSTDIR\glade\*.glade
	Delete $INSTDIR\glade\*.png
	Delete $INSTDIR\glade\*.svg
	RmDir $INSTDIR\glade
	Delete $INSTDIR\ascend-doc.ico

;--- file association (for Python GUI) ---
  
	DetailPrint "--- REMOVING FILE ASSOCIATION ---"
	;start of restore script
	ReadRegStr $1 HKCR ".a4c" ""
	${If} $1 == "ASCEND.model"
		ReadRegStr $1 HKLM "SOFTWARE\ASCEND" "BackupAssocA4C"
		${If} $1 == ""
			; nothing to restore: delete it
			DeleteRegKey HKCR ".a4c"
		${Else}
			WriteRegStr HKCR ".a4c" "" $1
		${EndIf}
		DeleteRegValue HKLM "SOFTWARE\ASCEND" "BackupAssocA4C"
	${EndIf}

	ReadRegStr $1 HKCR ".a4l" ""	
	${If} $1 == "ASCEND.model"
		ReadRegStr $1 HKLM "SOFTWARE\ASCEND" "BackupAssocA4L"
		${If} $1 == ""
			; nothing to restore: delete it
			DeleteRegKey HKCR ".a4l"
		${Else}
			WriteRegStr HKCR ".a4l" "" $1
		${EndIf}
		DeleteRegValue HKLM "SOFTWARE\ASCEND" "BackupAssocA4L"
	${EndIf}

	DeleteRegKey HKCR "ASCEND.model" ;Delete key with association settings

	System::Call 'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'
	;rest of script

unnopython:

;--- tcl/tk components ---

	ReadRegDWORD $0 HKLM "SOFTWARE\ASCEND" "TclTk"
	${If} $0 != 0
		DetailPrint "--- REMOVING TCL/TK COMPONENTS ---"
		Delete $INSTDIR\ascendtcl.dll
		Delete $INSTDIR\ascend4.exe
		RMDir /r $INSTDIR\tcltk
	${EndIf}

;--- start menu ---

	ReadRegDWORD $1 HKLM "SOFTWARE\ASCEND" "StartMenu"
	IntCmp $1 0 unnostart unstart 
unstart:
	; Remove shortcuts, if any
	DetailPrint "--- REMOVING START MENU SHORTCUTS ---"
	RmDir /r "$SMPROGRAMS\ASCEND"

unnostart:

;--- common components ---

	DetailPrint "--- REMOVING COMMON COMPONENTS ---"
	; Remove registry keys

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ASCEND"
	DeleteRegKey HKLM "SOFTWARE\ASCEND"

	; Remove files and uninstaller

	Delete $INSTDIR\ascend-config
	Delete $INSTDIR\ascend-config.bat
	Delete $INSTDIR\ascend.dll
	Delete $INSTDIR\LICENSE.txt
	Delete $INSTDIR\README-windows.txt
	Delete $INSTDIR\CHANGELOG.txt
	Delete $INSTDIR\ascend.ico
	Delete $INSTDIR\Makefile.bt
	Delete $INSTDIR\ascend.syn
	RMDir /r $INSTDIR\models
	Delete $INSTDIR\solvers\qrslv.dll
	Delete $INSTDIR\solvers\conopt.dll
	Delete $INSTDIR\solvers\lrslv.dll
	Delete $INSTDIR\solvers\cmslv.dll
	Delete $INSTDIR\solvers\lsode.dll
	Delete $INSTDIR\solvers\ida.dll

	; Remove directories used

	Delete $INSTDIR\uninstall.exe
	RMDir $INSTDIR

SectionEnd

;---------------------------------------------------------------------
; UTILITY ROUTINES

Function DetectPython
	ReadRegStr $R6 HKCU "SOFTWARE\Python\PythonCore\${PYVERSION}\InstallPath" ""
	${If} $R6 == ''
		ReadRegStr $R6 HKLM "SOFTWARE\Python\PythonCore\${PYVERSION}\InstallPath" ""
		${If} $R6 == ''
			Push "No registry key found"
			Push "NOK"
			Return
		${EndIf}
	${EndIf}
	
	${If} ${FileExists} "$R6\python.exe"
		Push "$R6"
		Push "OK"
	${Else}
		Push "No python.exe found"
		Push "NOK"
	${EndIf}
FunctionEnd

;--------------------------------------------------------------------
; Prefer the current user's installation of GTK, fall back to the local machine

Function DetectGTK
	ReadRegStr $R6 HKCU "SOFTWARE\GTK\2.0" "DllPath"
	${If} $R6 == ''
		ReadRegStr $R6 HKLM "SOFTWARE\GTK\2.0" "DllPath"
		${If} $R6 == ''
			Push "No GTK registry key found"
			Push "NOK"
			Return
		${EndIf}
	${EndIf}

	${If} ${FileExists} "$R6\libgtk-win32-2.0-0.dll"
		Push "$R6"
		Push "OK"
	${Else}
		Push "No libgtk-win32-2.0-0.dll found in'$R6'"
		Push "NOK"
	${EndIf}
FunctionEnd

;--------------------------------------------------------------------
; Are necessary PyGTK bits and pieces available?

Function DetectPyGTK
	${If} ${FileExists} "$PYPATH\Lib\site-packages\gtk-2.0\gtk\__init__.py"
		Push "OK"
	${Else}
		Push "NOK"
	${EndIf}
FunctionEnd

Function DetectPyCairo
	${If} ${FileExists} "$PYPATH\Lib\site-packages\cairo\__init__.py"
		Push "OK"
	${Else}
		Push "NOK"
	${EndIf}
FunctionEnd

Function DetectPyGObject
	${If} ${FileExists} "$PYPATH\Lib\site-packages\gtk-2.0\gobject\__init__.py"
		Push "OK"
	${Else}
		Push "NOK"
	${EndIf}
FunctionEnd

;--------------------------------------------------------------------
; Prefer the current user's installation of GTK, fall back to the local machine

Function DetectGlade
	ReadRegStr $R6 HKCU "SOFTWARE\GTK\2.0" "DllPath"
	${If} $R6 == ''
		ReadRegStr $R6 HKLM "SOFTWARE\GTK\2.0" "DllPath"
		${If} $R6 == ''
			Push "No GTK registry key found"
			Push "NOK"
			Return
		${EndIf}
	${EndIf}

	${If} ${FileExists} "$R6\libglade-2.0-0.dll"
		Push "$R6"
		Push "OK"
	${Else}
		Push "No libglade-2.0-0.dll found in'$R6'"
		Push "NOK"
	${EndIf}
FunctionEnd

;--------------------------------------------------------------------

Function DetectTcl
	ReadRegStr $R6 HKCU "SOFTWARE\ActiveState\ActiveTcl" "CurrentVersion"
	${If} $R6 == ''
		ReadRegStr $R6 HKLM "SOFTWARE\ActiveState\ActiveTcl" "CurrentVersion"
		${If} $R6 == ''
			Push "No 'CurrentVersion' registry key"
			Push "NOK"
			Return
		${Else}
			StrCpy $R7 "SOFTWARE\ActiveState\ActiveTcl\$R6"
			ReadRegStr $R8 HKLM $R7 ""		
		${EndIf}
	${Else}
		StrCpy $R7 "SOFTWARE\ActiveState\ActiveTcl\$R6"
		ReadRegStr $R8 HKCU $R7 ""		
	${EndIf}

	${If} $R8 == ''
		Push "No value for $R7"
		Push "NOK"
	${Else}
		Push "$R8\bin"
		Push "OK"
	${EndIf}
FunctionEnd
