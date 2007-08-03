;---------------------------------------------------------------------
; CUSTOM PAGE to DOWNLOAD REQUIRED DEPENDENCIES

!define DEPINI "dependencies.ini"
!define DEPINIPATH "$PLUGINSDIR\${DEPINI}"

Function dependenciesCreate
	MessageBox MB_OK "creating dependencies dialog"

	${If} $PYOK == 'OK'
	${AndIf} $GTKOK == 'OK'
	${AndIf} $PYGTKOK == 'OK'
	${AndIf} $PYGOBJECTOK == 'OK'
	${AndIf} $PYCAIROOK == 'OK'
		MessageBox MB_OK "all required dependencies were found"
		; do nothing in this page
	${Else}
		MessageBox MB_OK "running dependencies dialog"
        	InitPluginsDir
        	File ${DEPINI}
        	StrCpy $0 "${DEPINIPATH}"

        	Call CreateDialogFromINI
	${EndIf}

FunctionEnd

; this function finds which inputs were selected and sets global variables accordingly
Function dependenciesLeave
         ReadINIStr $PYDOWNLOAD ${DEPINIPATH} "Field 3" "State"
         ReadINIStr $PYGTKDOWNLOAD ${DEPINIPATH} "Field 4" "State"
         ReadINIStr $PYGOBJECTDOWNLOAD ${DEPINIPATH} "Field 5" "State"
         ReadINIStr $PYCAIRODOWNLOAD ${DEPINIPATH} "Field 6" "State"
         ReadINIStr $GTKDOWNLOAD ${DEPINIPATH} "Field 7" "State"
FunctionEnd
