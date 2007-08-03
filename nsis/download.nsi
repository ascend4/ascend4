Var DAI_RET
Var DAI_TMPFILE

!macro downloadAndInstall DAI_NAME DAI_URL DAI_FN DAI_CMD

	MessageBox MB_OK "Downloading ${DAI_NAME}..."

	StrCpy $DAI_TMPFILE "$TEMP\${DAI_FN}"
	nsisdl::download /TIMEOUT=30000 ${DAI_URL} $DAI_TMPFILE
	Pop $DAI_RET ;Get the return value
	${If} $DAI_RET == "success"
		MessageBox MB_OK "Installing ${DAI_NAME}..."
		ExecWait "${DAI_CMD}" $0
		MessageBox MB_OK "${DAI_NAME} installer returned $0"
		Delete $DAI_TMPFILE
		Push $0
	${ElseIf} $DAI_RET == "cancel"
		MessageBox MB_OK "${DAI_NAME} download cancelled"
		Push 2
	${Else}
		MessageBox MB_OK "Download failed: $DAI_RET"
		Push 1
	${EndIf}

!macroend