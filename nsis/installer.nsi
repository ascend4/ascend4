; NSIS script to create an ASCEND binary installer for Windows
; by John Pye, 2006-2012.
;
;--------------------------------

; The name of the installer

Name "ASCEND ${VERSION}"

;SetCompressor /FINAL zlib
SetCompressor /SOLID lzma

!include LogicLib.nsh
!include nsDialogs.nsh
!include x64.nsh

; The file to write
OutFile ${OUTFILE}

!if "${INSTARCH}" == "x64"
!define INST64
!endif

; The default installation directory
!ifdef INST64
InstallDir $PROGRAMFILES64\ASCEND
!else
InstallDir $PROGRAMFILES32\ASCEND
!endif

; NOTE we *don't* user InstallDirRegKey because it doesn't work correctly on Win64.
;InstallDirRegKey HKLM "Software\ASCEND" "Install_Dir"

RequestExecutionLevel admin

;--------------------------------

; Pages

Page license
LicenseData "..\LICENSE.txt"

Page components
Page directory
Page custom dependenciesCreate dependenciesLeave
Page instfiles
Page custom ascendIniCreate ascendIniLeave
Page custom ascendEnvVarCreate ascendEnvVarLeave

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

!define GTKSEARCHPATH "c:\GTK"

Var DEFAULTPATH
Var HAVE_PYTHON
Var PYPATH
Var HAVE_GTK
Var GTKPATH
Var HAVE_PYGTK
Var HAVE_PYGOBJECT
Var HAVE_PYCAIRO
Var PYINSTALLED

Var PDFINSTALLED

Var PATH

Var NEED_PYTHON
Var NEED_GTK
Var NEED_PYGTK
Var NEED_PYCAIRO
Var NEED_PYGOBJECT

Var ASCENDINIFOUND
Var ASCENDENVVARFOUND
Var ASCENDLIBRARY

Var PYTHONTARGETDIR

; .onInit has been moved to after section decls so that they can be references

;------------------------------------------------------------
; DOWNLOAD AND INSTALL DEPENDENCIES FIRST

; Use the official python.org Python packages
!define PYTHON_VERSION "${PYVERSION}${PYPATCH}"
!define PYTHON_FN "python-${PYTHON_VERSION}${PYARCH}.msi"
!define PYTHON_URL "http://python.org/ftp/python/${PYTHON_VERSION}/${PYTHON_FN}"
!define PYTHON_CMD "msiexec /i $DAI_TMPFILE /passive ALLUSERS=1 TARGETDIR=$PYTHONTARGETDIR"

!define THIRDPARTY_DIR "http://downloads.sourceforge.net/project/ascend-sim/thirdparty/"
!define GTK_VER "2.22"

!ifdef INST64
!define WINXX "win64"
!define AMDXX ".win-amd64"
!define NNBIT "64-bit"
!define X64I386 "x64"
!define GTK_PATCH ".1-20101229"
!else
!define WINXX "win32"
!define AMDXX ".win32"
!define X64I386 "i386"
!define NNBIT "32-bit"
!define GTK_PATCH ".1-20101227"
!endif

; Host our own GTK bundles, repackaged as installers.
; User should still be able to use the ftp.gnome.org zip files, we just can't easily install them from here.
; Also, but having GTK installer, we can store the installation location in the registry (and have both 64 and 32 bit versions)
!define GTK_FN "gtk+-${GTK_VER}${GTK_PATCH}-${X64I386}-a4.exe"
!define GTK_URL "${THIRDPARTY_DIR}${GTK_FN}"
!define GTK_MFT "gtk+-bundle_${GTK_VER}${GTK_PATCH}_${WINXX}.mft"
!define GTK_CMD "$DAI_TMPFILE /S"

; We will host the PyGTK, PyGObject and PyCairo dependencies on SF.net ourselves... for the moment.
; Note that PyGTK version should match GTK+ version.
!define PYGTK_PATCH ".0"
!define PYCAIRO_VER "1.10.0"
!define PYGOBJECT_VER "2.28.6"
!define PYGTK_FN "pygtk-${GTK_VER}${PYGTK_PATCH}${AMDXX}-py${PYVERSION}.exe"
!define PYCAIRO_FN "py2cairo-${PYCAIRO_VER}${AMDXX}-py${PYVERSION}.exe"
!define PYGOBJECT_FN "pygobject-${PYGOBJECT_VER}${AMDXX}-py${PYVERSION}.exe"
!define PYGTK_URL "${THIRDPARTY_DIR}${PYGTK_FN}"
!define PYCAIRO_URL "${THIRDPARTY_DIR}${PYCAIRO_FN}"
!define PYGOBJECT_URL "${THIRDPARTY_DIR}${PYGOBJECT_FN}"
!define PYGTK_CMD "$DAI_TMPFILE"
!define PYCAIRO_CMD "$DAI_TMPFILE"
!define PYGOBJECT_CMD "$DAI_TMPFILE"

!include "download.nsi"

Section "-python"
	DetailPrint "--- DOWNLOAD PYTHON ---"
        ${If} $NEED_PYTHON == '1'
		!insertmacro downloadAndInstall "Python" "${PYTHON_URL}" "${PYTHON_FN}" "${PYTHON_CMD}"
		Call DetectPython
		${If} $HAVE_PYTHON == 'NOK'
			MessageBox MB_OK "Python installation appears to have failed. You may need to retry manually."
		${EndIf}
        ${EndIf}
SectionEnd

Section "-gtk"
	DetailPrint "--- DOWNLOAD GTK ---"
	${If} $NEED_GTK == '1'
		!insertmacro downloadAndInstall "GTK" "${GTK_URL}" "${GTK_FN}" "${GTK_CMD}"
		Call DetectGTK
		${If} $HAVE_GTK == 'NOK'
			MessageBox MB_OK "GTK installation appears to have failed. You may need to retry manually."
		${EndIf}
        ${EndIf}
SectionEnd

Section "-pygtk"
	DetailPrint "--- DOWNLOAD PYGTK ---"
	${If} $NEED_PYGTK == '1'
		!insertmacro downloadAndInstall "PyGTK" "${PYGTK_URL}" "${PYGTK_FN}" "${PYGTK_CMD}"
		Call DetectPyGTK
		${If} $HAVE_PYGTK == 'NOK'
			MessageBox MB_OK "PyGTK installation appears to have failed. You may need to retry manually"
		${EndIf}
        ${EndIf}
SectionEnd

Section "-pycairo"
	DetailPrint "--- DOWNLOAD PYCAIRO ---"
	${If} $NEED_PYCAIRO == '1'
		!insertmacro downloadAndInstall "PyCAIRO" "${PYCAIRO_URL}" "${PYCAIRO_FN}" "${PYCAIRO_CMD}"
		Call DetectPyCairo
		${If} $HAVE_PYCAIRO == 'NOK'
			MessageBox MB_OK "PyCairo installation appears to have failed. You may need to retry manually."
		${EndIf}
        ${EndIf}
SectionEnd

Section "-pygobject"
	DetailPrint "--- DOWNLOAD PYGOBJECT ---"
	${If} $NEED_PYGOBJECT == '1'
		!insertmacro downloadAndInstall "PyGObject" "${PYGOBJECT_URL}" "${PYGOBJECT_FN}" "${PYGOBJECT_CMD}"
		Call DetectPyGObject
		${If} $HAVE_PYGOBJECT == 'NOK'
			MessageBox MB_OK "PyGObject installation appears to have failed. You may need to retry manually."
		${EndIf}
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
	${FILE_IPOPT_1}
	${FILE_IPOPT_2}
	${FILE_IPOPT_3}
	${FILE_IPOPT_4}
	${FILE_IPOPT_5}
	
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

	; Check for pre-existing .ascend.ini for current user (warn after installation, if so)
	${If} ${FileExists} "$APPDATA\.ascend.ini"
		StrCpy $ASCENDINIFOUND "1"
	${Else}
		; Set 'librarypath' in .ascend.ini
		WriteINIstr $APPDATA\.ascend.ini Directories librarypath "$DOCUMENTS\ascdata;$INSTDIR\models"
	${EndIf}
	
	; Check for ASCENDLIBRARY environment variable for current user
	ExpandEnvStrings $ASCENDLIBRARY "%ASCENDLIBRARY%"
	${IfNot} $ASCENDLIBRARY == "%ASCENDLIBRARY%"
		StrCpy $ASCENDENVVARFOUND "1"
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
!ifdef INST64
	SetRegView 64
!endif
	; Check the dependencies of the PyGTK GUI before proceding...
	${If} $HAVE_PYTHON == 'NOK'
		MessageBox MB_OK "PyGTK GUI can not be installed, because Python was not found on this system.$\nIf you do want to use the PyGTK GUI, please check the installation instructions$\n$\n(PYPATH=$PYPATH)"
	${ElseIf} $HAVE_GTK == 'NOK'
		MessageBox MB_OK "PyGTK GUI cannot be installed, because GTK+ 2.x was not found on this system.$\nIf you do want to use the PyGTK GUI, please check the installation instructions$\n$\n(GTKPATH=$GTKPATH)"
	${ElseIf} $HAVE_PYGTK == "NOK"
		MessageBox MB_OK "PyGTK GUI cannot be installed, because PyGTK was not found on this system.$\nPlease check the installation instructions.$\n$\n(PYPATH=$PYPATH)"
	${ElseIf} $HAVE_PYCAIRO == "NOK"
		MessageBox MB_OK "PyGTK GUI cannot be installed, because PyCairo was not found on this system.$\nPlease check the installation instructions.$\n$\n(PYPATH=$PYPATH)"
	${ElseIf} $HAVE_PYGOBJECT == "NOK"
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
		File "..\ascxx\_ascpy.pyd"
		File "..\ascxx\ascpy.py"
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

;	; Link to Tcl/Tk GUI  
;	${If} $TCLINSTALLED == "1"
;		CreateShortCut "$SMPROGRAMS\ASCEND\ASCEND Tcl/Tk.lnk" "$INSTDIR\ascend4.exe" "" "$INSTDIR\ascend4.exe" 0
;	${EndIf}
	
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
!ifdef INST64
	SetRegView 64
!endif

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

;	ReadRegDWORD $0 HKLM "SOFTWARE\ASCEND" "TclTk"
;	${If} $0 <> 0
;		DetailPrint "--- REMOVING TCL/TK COMPONENTS ---"
;		Delete $INSTDIR\ascendtcl.dll
;		Delete $INSTDIR\ascend4.exe
;		RMDir /r $INSTDIR\tcltk
;	${EndIf}

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

	Delete $INSTDIR\ascend-config
	Delete $INSTDIR\ascend-config.bat	
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

	${DEL_IPOPT_1}
	${DEL_IPOPT_2}
	${DEL_IPOPT_3}
	${DEL_IPOPT_4}
	${DEL_IPOPT_5}
	
	; Remove directories used

	Delete $INSTDIR\uninstall.exe
	RMDir $INSTDIR

SectionEnd

!include "dependencies.nsi"

!include "detect.nsi"

!include "ascendini.nsi"

!include "envvarwarning.nsi"

Function .onInit
!ifdef INST64
	${IfNot} ${RunningX64}
		MessageBox MB_OK "This ASCEND installer is for 64-bit Windows versions only.$\n$\nVisit http://ascend4.org for 32-bit versions."
		Abort
	${EndIf}
	SetRegView 64
!endif

	;Get the previously-chosen $INSTDIR
	ReadRegStr $0 HKLM "SOFTWARE\ASCEND" "Install_Dir"
	${If} $0 != ""
		StrCpy $INSTDIR $0
	${EndIf}

	;set the default python target dir
	StrCpy $PYTHONTARGETDIR "c:\Python${PYVERSION}"
!ifndef INST64
	${If} ${RunningX64}
		; this is a 32-bit installer on 64-bit Windows: install Python to a special location
		StrCpy $PYTHONTARGETDIR "c:\Python${PYVERSION}_32"
	${EndIf}
	; FIXME we should check whether that directory already exists before going ahead...
!endif

	StrCpy $PYINSTALLED ""
	StrCpy $ASCENDINIFOUND ""
	StrCpy $PDFINSTALLED ""
	StrCpy $ASCENDENVVARFOUND ""
	
	ExpandEnvStrings $DEFAULTPATH "%WINDIR%;%WINDIR%\system32"

	Call DetectPython
	Call DetectGTK
	Call DetectPyGTK
	Call DetectPyGObject
	Call DetectPyCairo
	
	;MessageBox MB_OK "GTK path is $GTKPATH"
	StrCpy $PATH "$GTKPATH;$DEFAULTPATH;$PYPATH"

	ReadRegStr $0 HKLM "SOFTWARE\ASCEND" "Install_Dir"
	${If} $0 != ""	
		;MessageBox MB_OK "Previous installation detected..."

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


Function un.onInit
!ifdef INST64
	SetRegView 64
!endif

	;Get the previously-chosen $INSTDIR
	ReadRegStr $0 HKLM "SOFTWARE\ASCEND" "Install_Dir"
	${If} $0 != ""
		StrCpy $INSTDIR $0
	${EndIf}
FunctionEnd
