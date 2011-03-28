Var DAI_RET
Var DAI_MSG
Var DAI_TMPFILE
Var DAI_REMOVE

!macro downloadAndInstall DAI_NAME DAI_URL DAI_FN DAI_CMD DAI_MD5
	Push $0
	Push $1

	StrCpy $DAI_RET ""
	StrCpy $DAI_REMOVE ""

	${If} ${FileExists} "${DAI_FN}"
		DetailPrint "Found local file ${DAI_FN}..."
		${If} ${Cmd} `MessageBox MB_ICONQUESTION|MB_YESNO "File ${DAI_FN} was found in the current directory, so it may not be necessary to download it now.$\n$\nWould you like to run this local copy of the installer?" IDYES `
			StrCpy $DAI_RET "success"
			StrCpy $DAI_TMPFILE "${DAI_FN}"
		${EndIf}

; Unable to get reliable behaviour with md5dll or Crypto plugins for NSIS...	
;		md5dll::GetMD5File "${DAI_FN}"
;		;Crypto::HashFile "MD5" "${DAI_FN}"
;		;Pop $0
;		${If} $0 == "${DAI_MD5}"
;			StrCpy $DAI_RET "success"
;			StrCpy $DAI_TMPFILE "${DAI_FN}"
;		${Else}
;			${If} ${Cmd} `MessageBox MB_ICONQUESTION|MB_YESNO "File ${DAI_FN} was found (with MD5 sum '$0')$\n but did not have expected MD5 '${DAI_MD5}'.$\n$\nWould you like this file to be used?" IDYES `
				StrCpy $DAI_RET "success"
				StrCpy $DAI_TMPFILE "${DAI_FN}"
;			${EndIf}
;		${EndIf}
;	${Else}
;		MessageBox MB_OK "File ${DAI_FN} not found. Will be downloaded."
	${EndIf}
	
	${If} $DAI_RET != "success"
		DetailPrint "Downloading file ${DAI_FN}..."
		StrCpy $DAI_TMPFILE "$TEMP\${DAI_FN}"
		nsisdl::download /TIMEOUT=30000 "${DAI_URL}" "$DAI_TMPFILE"
		Pop $DAI_RET ;Get the return value		
			
		${DoWhile} $DAI_RET != "success"
			${If} $DAI_RET == "cancel"
				StrCpy $DAI_MSG "cancelled"
			${Else}
				StrCpy $DAI_MSG "failed (return '$DAI_RET')"
			${EndIf}
			
			DetailPrint "Download of ${DAI_FN} $DAI_MSG."
			${IfNot} ${Cmd} `MessageBox MB_ICONEXCLAMATION|MB_YESNO "${DAI_NAME} download $DAI_MSG.$\n$\nDo you wish to re-attempt the download?" IDYES `
				; response was no
				;MessageBox MB_OK "File ${DAI_NAME} will not be installed..."
				Pop $1
				Pop $0
				Push 1 ; error code
				Return
			${EndIf}
			
			;MessageBox MB_OK "Will re-attempt download of ${DAI_NAME}"
			${If} ${FileExists} "$DAI_TMPFILE"
				Delete "$DAI_TMPFILE"
			${EndIf}
		${Loop}
		
		StrCpy $DAI_REMOVE "1"
	${EndIf}
	

	;MessageBox MB_OK "Installing ${DAI_NAME}...$\n$\nCommand: ${DAI_CMD}"
	DetailPrint "Installing ${DAI_NAME} (${DAI_FN})"
	ExecWait "${DAI_CMD}" $0
	DetailPrint "Installer return code = $0"
	${If} $0 != "0"
		MessageBox MB_ICONEXCLAMATION|MB_OK "${DAI_NAME} installer returned a non-zero error code '$0'"
	${EndIf}
	
	${If} $DAI_REMOVE != ""
		;MessageBox MB_OK "Deleting $DAI_TMPFILE..."
		Delete "$DAI_TMPFILE"
	${EndIf}
	
	; Restore registers
	Pop $1
	Pop $0
	
	; Return values
	Push $0
!macroend