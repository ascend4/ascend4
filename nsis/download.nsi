Var DAI_RET
Var DAI_TMPFILE
Var DAI_MSG

!macro downloadAndInstall DAI_NAME DAI_URL DAI_FN DAI_CMD DAI_MD5

	StrCpy $DAI_RET ""

	${If} ${FileExists} ${DAI_FN}
		Crypto::HashFile "MD5" "${DAI_FN}"
		Pop $0
		${If} $0 == "${DAI_MD5}"
			StrCpy $DAI_RET "success"
			StrCpy $DAI_TMPFILE ""
		${Else}
			MessageBox MB_OK "File ${DAI_FN} was found but did not have expected MD5 sum value. File will be downloaded."
		${EndIf}
	${Else}
		MessageBox MB_OK "File ${DAI_FN} not found. Will be downloaded."
	${EndIf}
	
	${If} $DAI_RET != "success"
		StrCpy $DAI_TMPFILE "$TEMP\${DAI_FN}"
		nsisdl::download /TIMEOUT=30000 "${DAI_URL}" "${DAI_TMPFILE}"
		Pop $DAI_RET ;Get the return value		
			
		${If} $DAI_RET != "success"
			${If} $DAI_RET == "cancel"
				StrCpy DAI_MSG "cancelled"
			${Else}
				StrCpy DAI_MSG "failed (return '$DAI_RET')"
			${EndIf}
			
			${IfNot} ${Cmd} `MessageBox MB_ICONEXCLAMATION|MB_YESNO "${DAI_NAME} download ${DAI_MSG}. Do you wish to re-attempt the download?" IDYES `
				; response was no
				MessageBox MB_OK "File ${DAI_NAME} will not be installed..."
				Push 1
				Return
			${EndIf}
		${EndIf}
	${EndIf}

	MessageBox MB_OK "Installing ${DAI_NAME}..."
	DetailPrint "Installing ${DAI_NAME} (${DAI_FN})"
	ExecWait "${DAI_CMD}" $0
	DetailPrint "Return code: $0"
	${If} $0 != "0"
		MessageBox MB_ICONEXCLAMATION|MB_OK "${DAI_NAME} installer returned a non-zero error code '$0'"
	${EndIf}
	
	
	${If} $DAI_TMPFILE != ""
		MessageBox MB_OK "Deleting ${DAI_TMPFILE}..."
		Delete $DAI_TMPFILE
	${EndIf}
	Push $0

!macroend