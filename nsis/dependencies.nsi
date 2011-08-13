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
	
	${If} $PYOK == 'OK'
	${AndIf} $GTKOK == 'OK'
	${AndIf} $PYGTKOK == 'OK'
	${AndIf} $PYGOBJECTOK == 'OK'
	${AndIf} $PYCAIROOK == 'OK'
	;${AndIf} $TCLOK == 'OK'
		; do nothing in this page
	${Else}
		nsDialogs::Create /NOUNLOAD 1018
		Pop $0

		${NSD_CreateLabel} 0% 0 100% 48% "The following additional packages are required for ASCEND to function correctly. Checked items will be downloaded and installed (some of the installers may require you to click 'next' a few times). If you don't want additional components to be downloaded you can unckeck them. This installer will then install only the parts for which the prerequisites are already satisfied."
		Pop $0

		${If} $PYOK == 'NOK'
			${NSD_CreateCheckbox} 10% 50% 100% 8u "Python"
			Pop $CHECKPY
			!insertmacro setCheckboxChecked $CHECKPY
		${EndIf}

		${If} $GTKOK == 'NOK'
			${NSD_CreateCheckbox} 10% 58% 100% 8u "PyGTK All-in-one"
			Pop $CHECKGTK
			!insertmacro setCheckboxChecked $CHECKGTK
		${EndIf}
		
;		${If} $TCLOK == 'NOK'
;			${NSD_CreateCheckbox} 10% 90% 100% 8u "Tcl/Tk"
;			Pop $CHECKTCL
;			!insertmacro setCheckboxChecked $CHECKTCL
;		${EndIf}

		nsDialogs::Show
	${EndIf}
	
FunctionEnd

Function DependenciesLeave
	SendMessage $CHECKPY ${BM_GETCHECK} 0 0 $PYDOWNLOAD
	SendMessage $CHECKGTK ${BM_GETCHECK} 0 0 $GTKDOWNLOAD
;	SendMessage $CHECKTCL ${BM_GETCHECK} 0 0 $TCLDOWNLOAD
FunctionEnd
	