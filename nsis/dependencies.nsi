;---------------------------------------------------------------------
; CUSTOM PAGE to DOWNLOAD REQUIRED DEPENDENCIES

Var CHECKPY
Var CHECKGTK
Var CHECKGTKSOURCEVIEW

!macro setCheckboxChecked CB
	SendMessage ${CB} ${BM_SETCHECK} 0x0001 0
	Pop $0
!macroend

Function dependenciesCreate
	
	${If} $HAVE_PYTHON == 'OK'
	${AndIf} $HAVE_GTK == 'OK'
	${AndIf} $HAVE_GTKSOURCEVIEW == 'OK'
		; do nothing in this page
	${Else}
		nsDialogs::Create /NOUNLOAD 1018
		Pop $0

		${NSD_CreateLabel} 0% 0 100% 40% "The following additional packages are required for ASCEND to function correctly. Checked items will be downloaded and installed (some of the installers may require you to click 'next' a fewtimes). If you don't want additional components to be downloaded you can unckeck them. This installer will then install only the parts for which the prerequisites are already satisfied."
		Pop $0
		${NSD_CreateLabel} 10% 40% 100% 10% "GTK GUI requirments:"
		Pop $0

		${If} $HAVE_PYTHON == 'NOK'
			${NSD_CreateCheckbox} 10% 50% 100% 10% "Python ${PYVERSION} (${NNBIT})"
			Pop $CHECKPY
			!insertmacro setCheckboxChecked $CHECKPY
		${Else}
			${NSD_CreateLabel} 10% 50% 100% 10% "Python:$\t$\t OK"
			Pop $0
		${EndIf}	

		${If} $HAVE_GTK == 'NOK'
			${NSD_CreateCheckbox} 10% 60% 100% 10% "Gtk3+ (${NNBIT})"
			Pop $CHECKGTK
			!insertmacro setCheckboxChecked $CHECKGTK
		${Else}
			${NSD_CreateLabel} 10% 60% 100% 10% "GTK3:$\t$\t OK"
			Pop $0
		${EndIf}
		
		${If} $HAVE_GTKSOURCEVIEW == 'NOK'
			${NSD_CreateCheckbox} 10% 70% 100% 10% "GtkSourceView (${NNBIT})"
			Pop $CHECKGTKSOURCEVIEW
			!insertmacro setCheckboxChecked $CHECKGTKSOURCEVIEW
		${Else}
			${NSD_CreateLabel} 10% 70% 100% 10% "GTKSourceView:$\t OK"
			Pop $0
		${EndIf}

		nsDialogs::Show
	${EndIf}
	
FunctionEnd

Function DependenciesLeave
	SendMessage $CHECKPY        	${BM_GETCHECK} 0 0 $NEED_PYTHON
	SendMessage $CHECKGTK        	${BM_GETCHECK} 0 0 $NEED_GTK
	SendMessage $CHECKGTKSOURCEVIEW ${BM_GETCHECK} 0 0 $NEED_GTKSOURCEVIEW
FunctionEnd
	