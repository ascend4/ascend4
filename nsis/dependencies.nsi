;---------------------------------------------------------------------
; CUSTOM PAGE to DOWNLOAD REQUIRED DEPENDENCIES

Var CHECKPY
Var CHECKGTK
;Var CHECKTCL

!macro setCheckboxChecked CB
	SendMessage ${CB} ${BM_SETCHECK} 0x0001 0
	Pop $0
!macroend

Function dependenciesCreate
	
	${If} $HAVE_PYTHON == 'OK'
	${AndIf} $HAVE_GTK == 'OK'
	${AndIf} $HAVE_PYGTK == 'OK'
	${AndIf} $HAVE_PYGOBJECT == 'OK'
	${AndIf} $HAVE_PYCAIRO == 'OK'
	;${AndIf} $TCLOK == 'OK'
		; do nothing in this page
	${Else}
		nsDialogs::Create /NOUNLOAD 1018
		Pop $0

		${NSD_CreateLabel} 0% 0 100% 48% "The following additional packages are required for ASCEND to function correctly. Checked items will be downloaded and installed (some of the installers may require you to click 'next' a few times). If you don't want additional components to be downloaded you can unckeck them. This installer will then install only the parts for which the prerequisites are already satisfied."
		Pop $0

		${If} $HAVE_PYTHON == 'NOK'
			${NSD_CreateCheckbox} 10% 50% 100% 8u "Python"
			Pop $CHECKPY
			!insertmacro setCheckboxChecked $CHECKPY
		${EndIf}

		${If} $HAVE_GTK == 'NOK'
			${NSD_CreateCheckbox} 10% 58% 100% 8u "PyGTK All-in-one"
			Pop $CHECKGTK
			!insertmacro setCheckboxChecked $CHECKGTK
		${EndIf}
		
		nsDialogs::Show
	${EndIf}
	
FunctionEnd

Function DependenciesLeave
	SendMessage $CHECKPY        ${BM_GETCHECK} 0 0 $NEED_PYTHON
	SendMessage $CHECKGTK       ${BM_GETCHECK} 0 0 $NEED_GTK
	SendMessage $CHECKPYGTK     ${BM_GETCHECK} 0 0 $NEED_PYGTK
	SendMessage $CHECKPYCAIRO   ${BM_GETCHECK} 0 0 $NEED_PYCAIRO
	SendMessage $CHECKPYGOBJECT ${BM_GETCHECK} 0 0 $NEED_PYGOBJECT
FunctionEnd
	