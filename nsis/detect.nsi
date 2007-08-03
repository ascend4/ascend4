;---------------------------------------------------------------------
; ROUTINES TO DETECT PYTHON, PYGTK, PYGOBJECT, PYCAIRO and TCL/TK.

;---------------------------------------------------------------------
; Look for Python in HKLM. No attempt to detect it in HKCU at this stage.

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
