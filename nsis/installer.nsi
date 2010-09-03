; NSIS script to create an ASCEND binary installer for Windows
; by John Pye, 2006-2007.
;
;--------------------------------

; The name of the installer

!ifndef VERSION
!define VERSION 0.svn
!endif

Name "ASCEND ${VERSION}"

;SetCompressor /FINAL zlib
SetCompressor /SOLID lzma

!include LogicLib.nsh
!include nsDialogs.nsh

!ifndef PYVERSION
!define PYVERSION "2.6"
!endif

!ifndef PYPATCH
!define PYPATCH ".1"
!endif

; The file to write
!ifdef OUTFILE
OutFile ${OUTFILE}
!else
OutFile "ascend-${VERSION}-py${PYVERSION}.exe"
!endif


; The default installation directory
InstallDir $PROGRAMFILES\ASCEND

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\ASCEND" "Install_Dir"

;--------------------------------

; Pages

Page license
LicenseData "..\LICENSE.txt"

Page components
Page directory
Page custom dependenciesCreate dependenciesLeave
Page instfiles
Page custom ascendIniCreate ascendIniLeave

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

Var /GLOBAL PDFINSTALLED

Var /GLOBAL PATH

Var /GLOBAL PYDOWNLOAD
Var /GLOBAL PYGTKDOWNLOAD
Var /GLOBAL PYGOBJECTDOWNLOAD
Var /GLOBAL PYCAIRODOWNLOAD
Var /GLOBAL GTKDOWNLOAD
Var /GLOBAL TCLDOWNLOAD

Var /GLOBAL ASCENDINIFOUND

; .onInit has been moved to after section decls so that they can be references

;------------------------------------------------------------
; DOWNLOAD AND INSTALL DEPENDENCIES FIRST

!define PYTHON_VERSION "${PYVERSION}${PYPATCH}"
!define PYTHON_FN "python-${PYTHON_VERSION}.msi"
!define PYTHON_URL "http://www.python.org/ftp/python/${PYTHON_VERSION}/${PYTHON_FN}"
!define PYTHON_CMD "msiexec /i $DAI_TMPFILE /passive"

!define GTK_FN "glade3-3.6.7-with-GTK+.exe"
!define GTK_URL "http://ftp.gnome.org/pub/GNOME/binaries/win32/glade3/3.6/${GTK_FN}"
!define GTK_CMD "$DAI_TMPFILE"

!define PYGOBJECT_VER "2.14"
!define PYGOBJECT_FN "pygobject-${PYGOBJECT_VER}.2-2.win32-py${PYVERSION}.exe"
!define PYGOBJECT_URL "http://ftp.gnome.org/pub/GNOME/binaries/win32/pygobject/${PYGOBJECT_VER}/${PYGOBJECT_FN}"
!define PYGOBJECT_CMD "$DAI_TMPFILE"

!define PYCAIRO_VER "1.4"
!define PYCAIRO_FN "pycairo-${PYCAIRO_VER}.12-2.win32-py${PYVERSION}.exe"
!define PYCAIRO_URL "http://ftp.gnome.org/pub/GNOME/binaries/win32/pycairo/${PYCAIRO_VER}/${PYCAIRO_FN}"
!define PYCAIRO_CMD "$DAI_TMPFILE"

!define PYGTK_VER "2.12"
!define PYGTK_FN "pygtk-${PYGTK_VER}.1-3.win32-py${PYVERSION}.exe"
!define PYGTK_URL "http://ftp.gnome.org/pub/GNOME/binaries/win32/pygtk/${PYGTK_VER}/${PYGTK_FN}"
!define PYGTK_CMD "$DAI_TMPFILE"

!define TCL_VERSION "8.5.8.2"
!define TCL_PATCH ".292682"
!define TCL_FN "ActiveTcl${TCL_VERSION}${TCL_PATCH}-win32-ix86-threaded.exe"
!define TCL_URL "http://downloads.activestate.com/ActiveTcl/releases/${TCL_VERSION}/${TCL_FN}"
!define TCL_CMD "$DAI_TMPFILE"
!include "download.nsi"

Section "-python"
	DetailPrint "--- DOWNLOAD PYTHON ---"
        ${If} $PYDOWNLOAD == '1'
		!insertmacro downloadAndInstall "Python" "${PYTHON_URL}" "${PYTHON_FN}" "${PYTHON_CMD}"
		Call DetectPython
		Pop $PYOK
		Pop $PYPATH
		${If} $PYOK == 'NOK'
			MessageBox MB_OK "Python installation appears to have failed"
		${EndIf}
        ${EndIf}
SectionEnd
Section "-gtk"
	DetailPrint "--- DOWNLOAD GTK+ ---"
	${If} $GTKDOWNLOAD == '1'
		!insertmacro downloadAndInstall "GTK+" ${GTK_URL} ${GTK_FN} ${GTK_CMD}
		Call DetectGTK
		Pop $GTKOK
		Pop $GTKPATH
		Call DetectGlade
		Pop $GLADEOK
		Pop $GLADEPATH
        ${EndIf}
SectionEnd
Section "-pygobject"
	DetailPrint "--- DOWNLOAD PYGOBJECT ---"
        ${If} $PYGOBJECTDOWNLOAD == '1'
        ${AndIf} $PYOK == 'OK'
		!insertmacro downloadAndInstall "PyGObject" ${PYGOBJECT_URL} ${PYGOBJECT_FN} ${PYGOBJECT_CMD}
		Call DetectPyGObject
		Pop $PYGOBJECTOK
        ${EndIf}
SectionEnd
Section "-pycairo"
	DetailPrint "--- DOWNLOAD PYCAIRO ---"
        ${If} $PYCAIRODOWNLOAD == '1'
        ${AndIf} $PYOK == 'OK'
		!insertmacro downloadAndInstall "PyCairo" ${PYCAIRO_URL} ${PYCAIRO_FN} ${PYCAIRO_CMD}
		Call DetectPyCairo
		Pop $PYCAIROOK
        ${EndIf}
SectionEnd
Section "-pygtk"
	DetailPrint "--- DOWNLOAD PYGTK ---"
        ${If} $PYGTKDOWNLOAD == '1'
        ${AndIf} $PYOK == 'OK'
		!insertmacro downloadAndInstall "PyGTK" ${PYGTK_URL} ${PYGTK_FN} ${PYGTK_CMD}
		Call DetectPyGTK
		Pop $PYGTKOK

        ${EndIf}
SectionEnd
Section "-tcl"
	DetailPrint "--- DOWNLOAD TCL/TK ---"
	${If} $TCLDOWNLOAD == '1'
		!insertmacro downloadAndInstall "Tcl/Tk" ${TCL_URL} ${TCL_FN} ${TCL_CMD}
		Call DetectTcl
		Pop $TCLOK
		Pop $TCLPATH
        ${EndIf}
SectionEnd

;------------------------------------------------------------------------
; INSTALL CORE STUFF including model library

; The stuff to install
Section "ASCEND (required)"
	SectionIn RO

	DetailPrint "--- COMMON FILES ---"

	; Set output path to the installation directory.
	SetOutPath $INSTDIR
	File "..\ascend.dll"
	File "..\ascend-config"
	File "..\pygtk\glade\ascend.ico"
	File "..\LICENSE.txt"
	File "..\CHANGELOG.txt"
	File "..\README-windows.txt"
	File "${IPOPTDLL}"
	
	; Model Library
	SetOutPath $INSTDIR\models
	File /r /x .svn "..\models\*.a4*"
	File /r /x .svn "..\models\*.tcl"
	File /r /x .svn "..\models\*_ascend.dll" ; extension modules
	File /r /x .svn "..\models\*.py"; python modules
	
	SetOutPath $INSTDIR\solvers
	File "..\solvers\qrslv\qrslv_ascend.dll"
	File "..\solvers\conopt\conopt_ascend.dll"
	File "..\solvers\lrslv\lrslv_ascend.dll"
	File "..\solvers\cmslv\cmslv_ascend.dll"
	File "..\solvers\lsode\lsode_ascend.dll"
	File "..\solvers\ida\ida_ascend.dll"
	File "..\solvers\dopri5\dopri5_ascend.dll"
	File "..\solvers\ipopt\ipopt_ascend.dll"
	
	SetOutPath $INSTDIR
	;File "Makefile.bt"
	File "..\tools\textpad\ascend.syn"

	${If} ${FileExists} "$APPDATA\.ascend.ini"
		StrCpy $ASCENDINIFOUND "1"
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
	
	; Write default values of ASCENDLIBRARY and ASCENDSOLVERS (user can override with env vars)
	WriteRegStr HKLM SOFTWARE\ASCEND "ASCENDLIBRARY" "$INSTDIR\models"
	WriteRegStr HKLM SOFTWARE\ASCEND "ASCENDSOLVERS" "$INSTDIR\solvers"
		
	Return
SectionEnd

;--------------------------------

Section "PyGTK GUI" sect_pygtk
	; Check the dependencies of the PyGTK GUI before proceding...
	${If} $PYOK == 'NOK'
		MessageBox MB_OK "PyGTK GUI can not be installed, because Python was not found on this system.$\nIf you do want to use the PyGTK GUI, please check the installation instructions$\n$\n(PYPATH=$PYPATH)"
	${ElseIf} $GTKOK == 'NOK'
		MessageBox MB_OK "PyGTK GUI cannot be installed, because GTK+ 2.x was not found on this system.$\nIf you do want to use the PyGTK GUI, please check the installation instructions$\n$\n(GTKPATH=$GTKPATH)"
	${ElseIf} $GLADEOK == 'NOK'
		MessageBox MB_OK "PyGTK GUI cannot be installed, because Glade 2.x was not found on this system.$\nIf you do want to use the PyGTK GUI, please check the installation instructions$\n$\n(GTKPATH=$GTKPATH).\n\nIf you do have GTK+ runtime installed, make sure\nyou have a version that includes support for Glade."
	${ElseIf} $PYGTKOK == "NOK"
		MessageBox MB_OK "PyGTK GUI cannot be installed, because PyGTK was not found on this system.$\nPlease check the installation instructions.$\n$\n(PYPATH=$PYPATH)"
	${ElseIf} $PYCAIROOK == "NOK"
		MessageBox MB_OK "PyGTK GUI cannot be installed, because PyCairo was not found on this system.$\nPlease check the installation instructions.$\n$\n(PYPATH=$PYPATH)"
	${ElseIf} $PYGOBJECTOK == "NOK"
		MessageBox MB_OK "PyGTK GUI cannot be installed, because PyGObject was not found on this system.$\nPlease check the installation instructions.$\n$\n(PYPATH=$PYPATH)"
	${Else}
		;MessageBox MB_OK "Python: $PYPATH, GTK: $GTKPATH"

		DetailPrint "--- PYTHON INTERFACE ---"

		; File icon
		SetOutPath $INSTDIR
		File "..\pygtk\glade\ascend-doc.ico"
		File "..\pygtk\ascend"		

		; Python interface
		SetOutPath $INSTDIR\python
		File "..\pygtk\_ascpy.pyd"
		File "..\pygtk\*.py"
		
		; FPROPS: python bindings
		File "..\models\johnpye\fprops\python\_fprops.pyd"
		File "..\models\johnpye\fprops\python\*.py"
		
		; GLADE assets
		SetOutPath $INSTDIR\glade
		File "..\pygtk\glade\*.glade"
		File "..\pygtk\glade\*.png"
		File "..\pygtk\glade\*.svg"

		StrCpy $PYINSTALLED "1"
		WriteRegDWORD HKLM "SOFTWARE\ASCEND" "Python" 1	

		; Create 'ascend.bat' batch file for launching the PyGTK GUI.
		ClearErrors
		FileOpen $0 $INSTDIR\ascend.bat w
		${If} ${Errors}
			MessageBox MB_OK "The 'ascend.bat' file was not installed properly; problems writing to that file."	
		${Else}
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
			FileWrite $0 "$INSTDIR\ascend"
			FileWriteByte $0 "34"
			FileWrite $0 " %1 %2 %3 %4 %5 %6 %7 %8"
			FileWriteByte $0 "13"
			FileWriteByte $0 "10"
			FileClose $0
		${EndIf}

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
		WriteRegStr HKCR "ASCEND.model\shell\open\command" "" '$PYPATH\pythonw.exe "$INSTDIR\ascend" "%1"'

		System::Call 'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'

	${EndIf}
	Return

SectionEnd

;---------------------------------

Section "Tcl/Tk GUI" sect_tcltk

	${If} $TCLOK != 'OK'
		MessageBox MB_OK "Tck/Tk GUI can not be installed, because ActiveTcl was not found on this system. If do you want to use the Tcl/Tk GUI, please check the installation instructions ($TCLPATH)"
	${Else}
		DetailPrint "--- TCL/TK INTERFACE ---"
		SetOutPath $INSTDIR\tcltk
		; FIXME we should be a bit more selective here?
		File /r /x .svn "..\tcltk\tk\*"
		SetOutPath $INSTDIR
		File "..\tcltk\interface\ascendtcl.dll"
		File "..\tcltk\interface\ascend4.exe"

		StrCpy $TCLINSTALLED "1"
		WriteRegDWORD HKLM "SOFTWARE\ASCEND" "TclTk" 1

	${EndIf}

SectionEnd

;---------------------------------

Section "Documentation" sect_doc
	SetOutPath $INSTDIR
	File "..\doc\book.pdf"
	StrCpy $PDFINSTALLED "1"
	WriteRegDWORD HKLM "SOFTWARE\ASCEND" "PDF" 1
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts" sect_menu
  
	WriteRegDWORD HKLM "SOFTWARE\ASCEND" "StartMenu" 1

	CreateDirectory "$SMPROGRAMS\ASCEND"  

	; Link to PyGTK GUI
	${If} $PYINSTALLED == "1"
		CreateShortCut "$SMPROGRAMS\ASCEND\ASCEND.lnk" "$PYPATH\pythonw.exe" '"$INSTDIR\ascend"' "$INSTDIR\ascend.ico" 0
	${EndIf}

	; Model library shortcut
	CreateShortCut "$SMPROGRAMS\ASCEND\Model Library.lnk" "$INSTDIR\models" "" "$INSTDIR\models" 0

	; Link to Tcl/Tk GUI  
	${If} $TCLINSTALLED == "1"
		CreateShortCut "$SMPROGRAMS\ASCEND\ASCEND Tcl/Tk.lnk" "$INSTDIR\ascend4.exe" "" "$INSTDIR\ascend4.exe" 0
	${EndIf}
	
	; Documentation
	${If} $PDFINSTALLED == "1"
		CreateShortCut "$SMPROGRAMS\ASCEND\User's Manual.lnk" "$INSTDIR\book.pdf" "" "$INSTDIR\book.pdf" 0
	${EndIf}

	; Information files
	CreateShortCut "$SMPROGRAMS\ASCEND\LICENSE.lnk" "$INSTDIR\LICENSE.txt" '' "$INSTDIR\LICENSE.txt" 0
	CreateShortCut "$SMPROGRAMS\ASCEND\CHANGELOG.lnk" "$INSTDIR\CHANGELOG.txt" '' "$INSTDIR\CHANGELOG.txt" 0
	CreateShortCut "$SMPROGRAMS\ASCEND\README.lnk" "$INSTDIR\README-windows.txt" '' "$INSTDIR\README-windows.txt" 0

	CreateShortCut "$SMPROGRAMS\ASCEND\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  
SectionEnd

;------------------------------------------------------------------
; HEADER FILES for DEVELOPERS

Section /o "Header files (for developers)" sect_devel
	WriteRegDWORD HKLM "SOFTWARE\ASCEND" "HeaderFiles" 1

	SetOutPath $INSTDIR\include\ascend
	File /r /x .svn "..\ascend\*.h"
	
	; Create 'ascend-config.bat' batch file for launching the python script 'ascend-config'.
	ClearErrors
	FileOpen $0 $INSTDIR\ascend-config.bat w
	${If} ${Errors}
		MessageBox MB_OK "The 'ascend-config.bat' file was not installed properly; problems writing to that file."	
	${Else}
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
	${EndIf}
	SetOutPath $INSTDIR
SectionEnd

;------------------------------------------------------------------
; UNINSTALLER

Section "Uninstall"

;--- python components ---

	ReadRegDWORD $0 HKLM "SOFTWARE\ASCEND" "Python"
	${If} $0 <> 0
  
		DetailPrint "--- REMOVING PYTHON COMPONENTS ---"
		Delete $INSTDIR\python\_ascpy.pyd
		Delete $INSTDIR\python\_fprops.pyd
		Delete $INSTDIR\python\*.py
		Delete $INSTDIR\python\*.pyc
		Delete $INSTDIR\glade\*.glade
		Delete $INSTDIR\glade\*.png
		Delete $INSTDIR\glade\*.svg
		Delete $INSTDIR\ascend-doc.ico
		Delete $INSTDIR\ascend
		Delete $INSTDIR\ascend.bat
		RmDir $INSTDIR\glade
		RmDir $INSTDIR\python

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

	${EndIf}

;--- tcl/tk components ---

	ReadRegDWORD $0 HKLM "SOFTWARE\ASCEND" "TclTk"
	${If} $0 <> 0
		DetailPrint "--- REMOVING TCL/TK COMPONENTS ---"
		Delete $INSTDIR\ascendtcl.dll
		Delete $INSTDIR\ascend4.exe
		RMDir /r $INSTDIR\tcltk
	${EndIf}

;--- documentation ---

	ReadRegDWORD $0 HKLM "SOFTWARE\ASCEND" "PDF"
	${If} $0 <> 0
		DetailPrint "--- REMOVING DOCUMENTATION ---"
		Delete $INSTDIR\book.pdf
	${EndIf}

;--- header files ---

	ReadRegDWORD $0 HKLM "SOFTWARE\ASCEND" "HeaderFiles"
	${If} $0 <> 0
		DetailPrint "--- REMOVING HEADER FILES ---"
		RMDir /r $INSTDIR\include
		Delete $INSTDIR\ascend-config
		Delete $INSTDIR\ascend-config.bat	
	${EndIf}
	
;--- start menu ---

	ReadRegDWORD $0 HKLM "SOFTWARE\ASCEND" "StartMenu"
	${If} $0 <> 0
		; Remove shortcuts, if any
		DetailPrint "--- REMOVING START MENU SHORTCUTS ---"
		RmDir /r "$SMPROGRAMS\ASCEND"
	${EndIf}

;--- common components ---

	DetailPrint "--- REMOVING COMMON COMPONENTS ---"
	; Remove registry keys

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ASCEND"
	DeleteRegKey HKLM "SOFTWARE\ASCEND"

	; Remove files and uninstaller

	Delete $INSTDIR\ascend.dll
	Delete $INSTDIR\LICENSE.txt
	Delete $INSTDIR\README-windows.txt
	Delete $INSTDIR\CHANGELOG.txt
	Delete $INSTDIR\ascend.ico
	Delete $INSTDIR\Makefile.bt
	Delete $INSTDIR\ascend.syn
	Delete $INSTDIR\ipopt38.dll
	RMDir /r $INSTDIR\models
	Delete $INSTDIR\solvers\qrslv_ascend.dll
	Delete $INSTDIR\solvers\conopt_ascend.dll
	Delete $INSTDIR\solvers\lrslv_ascend.dll
	Delete $INSTDIR\solvers\cmslv_ascend.dll
	Delete $INSTDIR\solvers\lsode_ascend.dll
	Delete $INSTDIR\solvers\ida_ascend.dll
	Delete $INSTDIR\solvers\dopri5_ascend.dll
	Delete $INSTDIR\solvers\ipopt_ascend.dll
	RMDir $INSTDIR\solvers

	; Remove directories used

	Delete $INSTDIR\uninstall.exe
	RMDir $INSTDIR

SectionEnd

!include "dependencies.nsi"

!include "detect.nsi"

!include "ascendini.nsi"

Function .onInit
	StrCpy $PYINSTALLED ""
	StrCpy $TCLINSTALLED ""
	StrCpy $ASCENDINIFOUND ""
	StrCpy $PDFINSTALLED ""
	
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

	ReadRegStr $0 HKLM "SOFTWARE\ASCEND" "Install_Dir"
	${If} $0 != ""	
		;MessageBox MB_OK "Previous installation detected..."
		; If user previous deselected Tcl/Tk, then deselect it by
		; default now, i.e don't force the user to install it.
		ReadRegDWORD $0 HKLM "SOFTWARE\ASCEND" "TclTk"
		${If} $0 = 0
			;MessageBox MB_OK "Tcl/Tk was previously deselected"
			SectionGetFlags "${sect_tcltk}" $1
			IntOp $1 $1 ^ ${SF_SELECTED}
			SectionSetFlags "${sect_tcltk}" $1
		${Else}
			; If previously installed, force it to stay installed;
			; the only way to uninstall a component is via complete
			; uninstall.
			SectionGetFlags "${sect_tcltk}" $1
			IntOp $1 $1 ^ ${SF_RO}
			SectionSetFlags "${sect_tcltk}" $1
		${EndIf}

		ReadRegDWORD $0 HKLM "SOFTWARE\ASCEND" "Python"
		${If} $0 = 0
			;MessageBox MB_OK "Python was previously deselected"
			SectionGetFlags "${sect_pygtk}" $1
			IntOp $1 $1 ^ ${SF_SELECTED}
			SectionSetFlags "${sect_pygtk}" $1
		${Else}
			SectionGetFlags "${sect_pygtk}" $1
			IntOp $1 $1 ^ ${SF_RO}
			SectionSetFlags "${sect_pygtk}" $1		
		${EndIf}

		ReadRegDWORD $0 HKLM "SOFTWARE\ASCEND" "PDF"
		${If} $0 = 0
			;MessageBox MB_OK "Documentation was previously deselected"
			SectionGetFlags "${sect_doc}" $1
			IntOp $1 $1 ^ ${SF_SELECTED}
			SectionSetFlags "${sect_doc}" $1
		${Else}
			SectionGetFlags "${sect_doc}" $1
			IntOp $1 $1 ^ ${SF_RO}
			SectionSetFlags "${sect_doc}" $1
		${EndIf}

		ReadRegDWORD $0 HKLM "SOFTWARE\ASCEND" "StartMenu"
		${If} $0 = 0
			;MessageBox MB_OK "Start Menu was previously deselected"
			SectionGetFlags "${sect_menu}" $1
			IntOp $1 $1 ^ ${SF_SELECTED}
			SectionSetFlags "${sect_menu}" $1
		${Else}
			SectionGetFlags "${sect_menu}" $1
			IntOp $1 $1 ^ ${SF_RO}
			SectionSetFlags "${sect_menu}" $1
		${EndIf}
		
		ReadRegDWORD $0 HKLM "SOFTWARE\ASCEND" "HeaderFiles"
		${If} $0 <> 0
			;MessageBox MB_OK "Header files were previously selected"
			SectionGetFlags "${sect_devel}" $1
			IntOp $1 $1 | ${SF_SELECTED}
			IntOp $1 $1 | ${SF_RO}
			SectionSetFlags "${sect_devel}" $1
		${EndIf}
	${EndIf}	

FunctionEnd
