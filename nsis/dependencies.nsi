;---------------------------------------------------------------------
; CUSTOM PAGE to DOWNLOAD REQUIRED DEPENDENCIES

Var CHECKPY

!macro setCheckboxChecked CB
	SendMessage ${CB} ${BM_SETCHECK} 0x0001 0
	Pop $0
!macroend

Function onManualInstallClick
    pop $0
    ExecShell "open" "http://sourceforge.net/projects/pygobjectwin32/files/" 
FunctionEnd

Function dependenciesCreate
	
	${If} $HAVE_PYTHON == 'OK'
	${AndIf} $HAVE_GTK == 'OK'
	${AndIf} $HAVE_PYGOBJECT == 'OK'
	${AndIf} $HAVE_PYCAIRO == 'OK'
	;${AndIf} $TCLOK == 'OK'
		; do nothing in this page
	${Else}
		nsDialogs::Create /NOUNLOAD 1018
		Pop $0

		${NSD_CreateLabel} 0% 0 100% 25% "The following additional packages are required for ASCEND to function correctly. You can check python to download and install automatically (the installer may require you to click 'next' a few times). Unfortunately you need to install other dependencies manually. Please download PyGI/PyGObject for Windows from http://sourceforge.net/projects/pygobjectwin32/files/"
		Pop $0
		${NSD_CreateLink} 0% 25% 100% 7% "http://sourceforge.net/projects/pygobjectwin32/files/"
		Pop $0
		${NSD_OnClick} $0 onManualInstallClick
		${NSD_CreateLabel} 0% 32% 100% 15% "and install required packages. This installer will then install only the parts for which the prerequisites are already satisfied."
		
		${If} $HAVE_PYTHON == 'NOK'
			${NSD_CreateCheckbox} 10% 50% 100% 8u "Python ${PYVERSION} (${NNBIT})"
			Pop $CHECKPY
			!insertmacro setCheckboxChecked $CHECKPY
		${Else}
			${NSD_CreateLabel} 10% 50% 100% 10% "Python:$\t$\t OK"
			Pop $0
		${EndIf}	

		${If} $HAVE_GTK == 'NOK'
			${NSD_CreateLabel} 10% 60% 100% 10% "GTK3:$\t$\t not found"
			Pop $0
		${Else}
			${NSD_CreateLabel} 10% 60% 100% 10% "GTK3:$\t$\t OK"
			Pop $0
		${EndIf}
		
		${If} $HAVE_PYCAIRO == 'NOK'
			${NSD_CreateLabel} 10% 70% 100% 10% "PyCairo:$\t not found"
			Pop $0
		${Else}
			${NSD_CreateLabel} 10% 70% 100% 10% "PyCairo:$\t OK"
			Pop $0
		${EndIf}

		${If} $HAVE_PYGOBJECT == 'NOK'
			${NSD_CreateLabel} 10% 80% 100% 10% "PyGObject:$\t not found"
			Pop $0
		${Else}
			${NSD_CreateLabel} 10% 80% 100% 10% "PyGObject:$\t OK"
			Pop $0
		${EndIf}

		nsDialogs::Show
	${EndIf}
	
FunctionEnd

Function DependenciesLeave
	SendMessage $CHECKPY        ${BM_GETCHECK} 0 0 $NEED_PYTHON
FunctionEnd
	