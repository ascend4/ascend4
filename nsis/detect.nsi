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
; Is GTK3 available?

Function DetectGTK
	${If} ${FileExists} "$PYPATH\Lib\site-packages\gnome\libgtk-3-0.dll"
		StrCpy $HAVE_GTK "OK"
	${Else}
		;MessageBox MB_OK "No PyCairo in $PYPATH"		
		StrCpy $HAVE_GTK "NOK"
	${EndIf}
FunctionEnd

;--------------------------------------------------------------------
; Are necessary Python packages and pieces available?

Function DetectPyCairo
	${If} ${FileExists} "$PYPATH\Lib\site-packages\gnome\libcairo-gobject-2.dll"
		StrCpy $HAVE_PYCAIRO "OK"
	${Else}
		;MessageBox MB_OK "No PyCairo in $PYPATH"		
		StrCpy $HAVE_PYCAIRO "NOK"
	${EndIf}
FunctionEnd

Function DetectPyGObject
	${If} ${FileExists} "$PYPATH\Lib\site-packages\gnome\libgobject-2.0-0.dll"
		StrCpy $HAVE_PYGOBJECT "OK"
	${Else}
		;MessageBox MB_OK "No PyGObject in $PYPATH"		
		StrCpy $HAVE_PYGOBJECT "NOK"
	${EndIf}
FunctionEnd
