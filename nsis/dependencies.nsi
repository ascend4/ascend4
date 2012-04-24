;---------------------------------------------------------------------
; CUSTOM PAGE to DOWNLOAD REQUIRED DEPENDENCIES

Var CHECKPY
Var CHECKGTK
Var CHECKPYGTK
Var CHECKPYCAIRO
Var CHECKPYGOBJECT

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
			${NSD_CreateCheckbox} 10% 50% 100% 8u "Python ${PYVERSION} (${NNBIT})"
			Pop $CHECKPY
			!insertmacro setCheckboxChecked $CHECKPY
		${EndIf}

		${If} $HAVE_GTK == 'NOK'
			${NSD_CreateCheckbox} 10% 58% 100% 8u "GTK+ bundle ${GTK_VER} (${NNBIT})"
			Pop $CHECKGTK
			!insertmacro setCheckboxChecked $CHECKGTK
		${EndIf}


		${If} $HAVE_PYGTK == 'NOK'
			${NSD_CreateCheckbox} 10% 64% 100% 8u "PyGTK"
			Pop $CHECKPYGTK
			!insertmacro setCheckboxChecked $CHECKPYGTK
		${EndIf}
		
		${If} $HAVE_PYCAIRO == 'NOK'
			${NSD_CreateCheckbox} 10% 72% 100% 8u "PyCairo"
			Pop $CHECKPYCAIRO
			!insertmacro setCheckboxChecked $CHECKPYCAIRO
		${EndIf}

		${If} $HAVE_PYGOBJECT == 'NOK'
			${NSD_CreateCheckbox} 10% 80% 100% 8u "PyGObject"
			Pop $CHECKPYGOBJECT
			!insertmacro setCheckboxChecked $CHECKPYGOBJECT
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
	