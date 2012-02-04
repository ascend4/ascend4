;---------------------------------------------------------------------
; ROUTINES TO DETECT PYTHON, PYGTK, PYGOBJECT, PYCAIRO and TCL/TK.

;---------------------------------------------------------------------
; Look for Python in HKLM. No attempt to detect it in HKCU at this stage.

Function DetectPython
!if ${INST64}
	SetRegView 64
!endif
	ReadRegStr $R6 HKCU "SOFTWARE\Python\PythonCore\${PYVERSION}\InstallPath" ""
	${If} $R6 == ''
		;MessageBox MB_OK "No Python in HKCU"
		ReadRegStr $R6 HKLM "SOFTWARE\Python\PythonCore\${PYVERSION}\InstallPath" ""
		${If} $R6 == ''
			;MessageBox MB_OK "No Python in HKLM"
			Push "No registry key found"
			Push "NOK"
			Return
		${EndIf}
	${EndIf}
	
	${If} ${FileExists} "$R6\python.exe"
		Push "$R6"
		Push "OK"
	${Else}
		;MessageBox MB_OK "No python.exe in $R6"	
		Push "No python.exe found"
		Push "NOK"
	${EndIf}
FunctionEnd

;--------------------------------------------------------------------
; Prefer the current user's installation of GTK, fall back to the local machine

Function DetectGTK
	${If} ${FileExists} "${GTKSEARCHPATH}\manifest\gtk+-bundle_2.22.1-20101229_win64.mft"
		MessageBox MB_OK "GTK OK in ${GTKSEARCHPATH}\manifest"
		Push "${GTKSEARCHPATH}\bin"
		Push "OK"
	${Else}
 		MessageBox MB_OK "gtk+-bundle_2.22.1-20101229_win64.mft not found in ${GTKSEARCHPATH}\manifest"
		Push "gtk+-bundle_2.22.1-20101229_win64.mft not found in ${GTKSEARCHPATH}\manifest"
		Push "NOK"
	${EndIf}
FunctionEnd

;--------------------------------------------------------------------
; Are necessary PyGTK bits and pieces available?

Function DetectPyGTK
	${If} ${FileExists} "$PYPATH\Lib\site-packages\gtk-2.0\gtk\__init__.py"
		Push "OK"
	${Else}
		MessageBox MB_OK "No PyGTK in $PYPATH"		
		Push "NOK"
	${EndIf}
FunctionEnd

Function DetectPyCairo
	${If} ${FileExists} "$PYPATH\Lib\site-packages\cairo\__init__.py"
		Push "OK"
	${Else}
		MessageBox MB_OK "No PyCairo in $PYPATH"		
		Push "NOK"
	${EndIf}
FunctionEnd

Function DetectPyGObject
	${If} ${FileExists} "$PYPATH\Lib\site-packages\gtk-2.0\gobject\__init__.py"
		Push "OK"
	${Else}
		MessageBox MB_OK "No PyGObject in $PYPATH"		
		Push "NOK"
	${EndIf}
FunctionEnd

;--------------------------------------------------------------------
; Prefer the current user's installation of GTK, fall back to the local machine

; don't need glade any more, we converted to gtkbuilder.
;Function DetectGlade
;	${If} $PYOK == "OK"
;		${If} ${FileExists} "$PYPATH\Lib\site-packages\gtk-2.0\runtime\bin\libglade-2.0-0.dll"
;			Push "$PYPATH\Lib\site\packages\gtk-2.0\runtime\bin"
;			Push "OK"
;		${Else}
;			Push "libglade-2.0-0.dll not found in $PYPATH\Lib\site\packages\gtk-2.0\runtime\bin"
;			Push "NOK"
;		${EndIf}
;	${Else}
;		Push "Python not detected (we are looking for PyGTK All-in-one package)"
;		Push "NOK"
;	${EndIf}
;FunctionEnd

;--------------------------------------------------------------------

;Function DetectTcl
;!if ${INST64}
;	SetRegView 64
;!endif
;	ReadRegStr $R6 HKCU "SOFTWARE\ActiveState\ActiveTcl" "CurrentVersion"
;	${If} $R6 == ''
;		ReadRegStr $R6 HKLM "SOFTWARE\ActiveState\ActiveTcl" "CurrentVersion"
;		${If} $R6 == ''
;			Push "No 'CurrentVersion' registry key"
;			Push "NOK"
;			Return
;		${Else}
;			StrCpy $R7 "SOFTWARE\ActiveState\ActiveTcl\$R6"
;			ReadRegStr $R8 HKLM $R7 ""		
;		${EndIf}
;	${Else}
;		StrCpy $R7 "SOFTWARE\ActiveState\ActiveTcl\$R6"
;		ReadRegStr $R8 HKCU $R7 ""		
;	${EndIf}
;	
;	${If} $R8 == ''
;		Push "No value for $R7"
;		Push "NOK"
;	${Else}
;		Push "$R8\bin"
;		Push "OK"
;	${EndIf}
;FunctionEnd
