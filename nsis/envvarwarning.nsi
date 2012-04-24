;---------------------------------------------------------------------
; CUSTOM PAGE to tell the user about 

Function ascendEnvVarCreate
	
	${If} $ASCENDENVVARFOUND == '1'
	${AndIf} $HAVE_PYTHON == "OK"

		nsDialogs::Create /NOUNLOAD 1018
		Pop $0

		${NSD_CreateLabel} 10% 0 75% 100% "The environment variable ASCENDLIBRARY is set to '$ASCENDLIBRARY' for the current user.$\n$\nBe careful to review this variable if unexpected behaviour occurs. This environment variable has NOT been modified by this installer."
		Pop $0

		nsDialogs::Show
	${EndIf}
	
FunctionEnd

Function ascendEnvVarLeave
	; nothing here
FunctionEnd
