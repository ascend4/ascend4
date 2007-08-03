;---------------------------------------------------------------------
; CUSTOM PAGE to tell the user about 

Function ascendIniCreate
	
	${If} $ASCENDINIFOUND == '1'
	${AndIf} $PYOK == "OK"

		nsDialogs::Create /NOUNLOAD 1018
		Pop $0

		${NSD_CreateLabel} 10% 0 75% 100% "An ASCEND settings file '$APPDATA\.ascend.ini' was found for the current user. This file has NOT been overwritten by the installer. You should manually delete that file and re-run the installer if ASCEND doesn't behave as expected."
		Pop $0
		
		; nothing else here

		nsDialogs::Show
	${EndIf}
	
FunctionEnd

Function ascendIniLeave
	; nothing here
FunctionEnd
