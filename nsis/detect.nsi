;---------------------------------------------------------------------
; ROUTINES TO DETECT PYTHON, PYGTK, PYGOBJECT, PYCAIRO and TCL/TK.

;---------------------------------------------------------------------
; Look for Python in HKLM and HKCU

Function DetectPython
!ifdef INST64
	SetRegView 64
!endif
	ReadRegStr $R6 HKLM "SOFTWARE\Python\PythonCore\${PYVERSION}\InstallPath" ""
	${If} $R6 == ''
		;MessageBox MB_OK "No Python in HKLM"
		ReadRegStr $R6 HKCU "SOFTWARE\Python\PythonCore\${PYVERSION}\InstallPath" ""
		${If} $R6 == ''
			;MessageBox MB_OK "No Python in HKCU"
			StrCpy $HAVE_PYTHON "NOK"
			StrCpy $PYPATH "No registry key found"
			Return
		${EndIf}
	${EndIf}
	
	${If} ${FileExists} "$R6\python.exe"
		StrCpy $PYPATH "$R6"
		StrCpy $HAVE_PYTHON "OK"
	${Else}
		;MessageBox MB_OK "No python.exe in $R6"	
		StrCpy $PYPATH "No python.exe found"
		StrCpy $HAVE_PYTHON "NOK"
	${EndIf}
FunctionEnd

;--------------------------------------------------------------------
; Prefer the current user's installation of GTK, fall back to the local machine

Function DetectGTK
!ifdef INST64
	SetRegView 64
!endif	
	; Search in the registry in the first instance
	ReadRegStr $R6 HKLM "SOFTWARE\GTK+-${GTK_VER}" "InstallDir"
	${If} $R6 == ''
		; If not found in the registory, look in ${GTKSEARCHPATH}
		;MessageBox MB_OK "No GTK found in HKLM"
		${If} ${FileExists} "${GTKSEARCHPATH}\manifest\${GTK_MFT}"
				;MessageBox MB_OK "GTK OK in ${GTKSEARCHPATH}\manifest"
				StrCpy $GTKPATH "${GTKSEARCHPATH}\bin"
				StrCpy $HAVE_GTK "OK"
				Return
		${EndIf}
	${Else}
		; Found in the registry. Check for the GTK DLL, but don't insist
		; on exactly matching manifest ID in this case.
		${If} ${FileExists} "$R6\bin\libgtk-win32-2.0-0.dll"
			;MessageBox MB_OK "GTK OK in $R6 (from registry)"
			StrCpy $GTKPATH "$R6\bin"
			StrCpy $HAVE_GTK "OK"
			Return
		${EndIf}
	${EndIf}
	;MessageBox MB_OK "Failed to locate GTK (searched registry\nand also ${GTKSEARCHPATH})"
	StrCpy $GTKPATH "GTK not found in registry or ${GTKSEARCHPATH}"
	StrCpy $HAVE_GTK "NOK"
FunctionEnd

;--------------------------------------------------------------------
; Are necessary PyGTK bits and pieces available?

Function DetectPyGTK
	${If} ${FileExists} "$PYPATH\Lib\site-packages\gtk-2.0\gtk\__init__.py"
		StrCpy $HAVE_PYGTK "OK"
	${Else}
		;MessageBox MB_OK "No PyGTK in $PYPATH"		
		StrCpy $HAVE_PYGTK "NOK"
	${EndIf}
FunctionEnd

Function DetectPyCairo
	${If} ${FileExists} "$PYPATH\Lib\site-packages\cairo\__init__.py"
		StrCpy $HAVE_PYCAIRO "OK"
	${Else}
		;MessageBox MB_OK "No PyCairo in $PYPATH"		
		StrCpy $HAVE_PYCAIRO "NOK"
	${EndIf}
FunctionEnd

Function DetectPyGObject
	${If} ${FileExists} "$PYPATH\Lib\site-packages\gtk-2.0\gobject\__init__.py"
		StrCpy $HAVE_PYGOBJECT "OK"
	${Else}
		;MessageBox MB_OK "No PyGObject in $PYPATH"		
		StrCpy $HAVE_PYGOBJECT "NOK"
	${EndIf}
FunctionEnd
