#  script.tcl
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.37 $
#  Last modified on: $Date: 1998/06/18 15:55:38 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: script.tcl,v $
#
#  This file is part of the ASCEND Tcl/Tk Interface.
#
#  Copyright (C) 1994-1998 Carnegie Mellon University
#
#  The ASCEND Tcl/Tk Interface is free software; you can redistribute
#  it and/or modify it under the terms of the GNU General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  The ASCEND Tcl/Tk Interface is distributed in hope that it will be
#  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
#  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with the program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the
#  file named COPYING.  COPYING is found in ../compiler.

# Module: script.tcl
# Tcl version: 7.1 (Tcl/Tk/XF)
# Tk version: 3.4
# XF version: 2.2
#

# module contents
global moduleList
global autoLoadList
set moduleList(script.tcl) { .script}
set autoLoadList(script.tcl) {0}

# procedures to show toplevel windows

#----------------------------
# procedure to show window ShowWindow.script
# proc ShowWindow.script { args}
proc ShowWindow.script { args} {
  # xf ignore me 7
  global ascScripVect ascGlobalVect env

  StartupSrc.script

  # build widget .script
  if {"[info procs XFEdit]" != ""} {
    catch "XFDestroy .script"
  } {
    catch "destroy .script"
  }
  toplevel .script

  #
  # Window manager configurations
  #
  wm positionfrom .script user
  wm sizefrom .script user
  wm iconname .script {Script}
  wm minsize .script 100 100
  wm title .script {A4 Script}
  wm protocol .script WM_DELETE_WINDOW {Script_do_Exit}

  #
  # Build frame along bottom of script to hold
  # record button and file name
  #
  frame .script.check_frm \
    -borderwidth {2} \
    -relief {raised}

  entry .script.check_frm.file_entry \
    -relief {raised} \
    -width {24} \
    -textvariable ascScripVect(filename) \
    -font $ascScripVect(font) \
    -state disabled \
    -exportselection 0

  label .script.check_frm.record_btn \
    -text { } \
    -font $ascGlobalVect(font)

  pack append .script.check_frm \
     .script.check_frm.record_btn {left frame center padx 10 pady 6} \
     .script.check_frm.file_entry {left frame w pady 5 expand fill}

  #
  # Build the main frame of the script.  This holds the text boxes.
  #
  frame .script.main_frm \
    -borderwidth {0}

  # Build the first script-text-box (script_box_0)
  build_scriptbox .script.main_frm 0 $ascScripVect(font)
  pack append .script.main_frm \
    .script.main_frm.script_box_0 {top frame center expand fill}

  #
  # Build the menu bar and submenus
  #
  menu .script.menubar \
    -tearoffcommand .SCRIPT.MENUBAR \
    -tearoff 0

  # The Edit menu.
  menu .script.menubar.edit \
    -tearoffcommand .SCRIPT.MENUBAR.EDIT \
    -tearoff 0
  .script.menubar.edit add checkbutton  \
    -variable ascScripVect(Record) \
    -offvalue {0} \
    -onvalue {1} \
    -label {Record actions} \
    -accelerator {Alt-e r} \
    -underline 0
  .script.menubar.edit add command  \
    -command {Script_do_SelectAll} \
    -label {Select all} \
    -accelerator {Alt-e s} \
    -underline 0
  .script.menubar.edit add separator
  .script.menubar.edit add command  \
    -command {Script_do_RemoveStats} \
    -accelerator {Alt-e d} \
    -label {Delete statements} \
    -underline 0
  .script.menubar.edit add command \
    -command Script_do_Cut \
    -label Cut \
    -accelerator F2 \
    -underline -1
  .script.menubar.edit add command \
    -command Script_do_Copy \
    -label Copy \
    -accelerator F3 \
    -underline -1
  .script.menubar.edit add command \
    -command Script_do_Paste \
    -label Paste \
    -accelerator F4 \
    -underline -1


  # The Execute menu.
  menu .script.menubar.execute \
    -tearoffcommand .SCRIPT.MENUBAR.EXECUTE \
    -tearoff 0
  .script.menubar.execute add command  \
    -command {Script_do_ExecuteStats}  \
    -label {Run statements selected} \
    -accelerator {F5} \
    -underline 0
   .script.menubar.execute add command  \
    -command {Script_do_ExecuteStats 0}  \
    -label {Step through statements selected} \
    -accelerator {F6} \
    -underline 0


  # The File menu
  menu .script.menubar.file \
    -tearoffcommand .SCRIPT.MENUBAR.FILE \
    -tearoff 0
  .script.menubar.file add command \
    -command {Script_do_NewFile} \
    -accelerator {Alt-f n} \
    -label {New file} \
    -underline 0
  .script.menubar.file add command  \
    -command {Script_do_ReadFile} \
    -label {Read file...} \
    -accelerator {Alt-f r} \
    -underline 0
  .script.menubar.file add command  \
    -command {Script_do_Import_File} \
    -label {Import file...} \
    -accelerator {Alt-f i} \
    -underline 0
  .script.menubar.file add separator
  .script.menubar.file add command  \
    -command {Script_do_Exit} \
    -accelerator {Alt-f e} \
    -underline 0 \
    -label {Exit ASCEND...}
  .script.menubar.file add separator
  .script.menubar.file add command  \
    -command {Script_do_WriteBuf} \
    -label {Save} \
    -accelerator {Alt-f s} \
    -underline 0
  .script.menubar.file add command  \
    -command {Script_do_WriteBufAs} \
    -label {Save As...} \
    -accelerator {Alt-f a} \
    -underline 5
  .script.menubar.file add separator


  # The Help menu.
  menu .script.menubar.help \
    -tearoffcommand .SCRIPT.MENUBAR.HELP \
    -tearoff 0
  .script.menubar.help add command \
    -command {Script_do_Help} \
    -label {On Script} \
    -underline 3
  .script.menubar.help add command \
    -command {Script_getting_started} \
    -label {On getting started with ASCEND} \
    -underline 3
  .script.menubar.help add command \
    -command {ascShowInfo 1} \
    -label {About ASCEND IV} \
    -underline 3


  # The Tools menu.
  menu .script.menubar.tools \
    -tearoffcommand .SCRIPT.MENUBAR.TOOLS \
    -tearoff 0
  .script.menubar.tools add checkbutton \
    -variable ascLibrVect(window.open) \
    -onvalue 1 \
    -offvalue 0 \
    -label {Type class library ...} \
    -accelerator {Alt-t l} \
    -underline 11
  .script.menubar.tools add checkbutton \
    -variable ascBrowVect(window.open) \
    -onvalue 1 \
    -offvalue 0 \
    -accelerator {Alt-t b} \
    -label {Instance browser ...} \
    -underline 9
  .script.menubar.tools add checkbutton \
    -variable ascProbVect(window.open) \
    -onvalue 1 \
    -offvalue 0 \
    -label {Data probe ...} \
    -accelerator {Alt-t p} \
    -underline 5
  .script.menubar.tools add checkbutton \
    -variable ascSolvVect(window.open) \
    -onvalue 1 \
    -offvalue 0 \
    -label {Solvers ...} \
    -accelerator {Alt-t s} \
    -underline 0
  .script.menubar.tools add checkbutton \
    -variable ascDispVect(window.open) \
    -onvalue 1 \
    -offvalue 0 \
    -label {Display slave ...} \
    -accelerator {Alt-t d} \
    -underline 0
  .script.menubar.tools add checkbutton \
    -variable ascUnitVect(window.open) \
    -onvalue 1 \
    -offvalue 0 \
    -accelerator {Alt-t m} \
    -label {Measuring units ...} \
    -underline 0
  .script.menubar.tools add checkbutton \
    -variable ascToolVect(window.open) \
    -onvalue 1 \
    -offvalue 0 \
    -label {Toolbox ...} \
    -accelerator {Alt-t t} \
    -underline 0
  .script.menubar.tools add separator
  .script.menubar.tools add command \
    -command {NoteBrowse} \
    -label {Notes on models ...} \
    -accelerator {Alt-t n} \
    -underline 0
  .script.menubar.tools add command \
    -command {ascconsole_open} \
    -label {Open console} \
    -accelerator {Alt-t c} \
    -underline 5
  .script.menubar.tools add command \
    -command {ascplot_open} \
    -accelerator {Alt-t a} \
    -label {ASCPLOT ...} \
    -underline 0
  .script.menubar.tools add command \
    -command {Tool_do_UtilBox} \
    -accelerator {Alt-t u} \
    -label {System utilities ...} \
    -underline 7
  .script.menubar.tools add command \
    -command {Tool_do_Callbacks} \
    -accelerator {Alt-t i} \
    -label {Internal functions ...} \
    -underline 0
  .script.menubar.tools add command \
    -command {Tool_do_Bugs} \
    -accelerator {Alt-t g} \
    -label {Report bug ...} \
    -underline 9


  # The options menu.
  menu .script.menubar.options \
    -tearoffcommand .SCRIPT.MENUBAR.OPTIONS \
    -tearoff 0
  .script.menubar.options add command  \
    -command {View_Save_Values}  \
    -label {Save all options and appearances for all windows} \
    -accelerator {Alt-o s} \
    -underline 0

  # The View menu.
  menu .script.menubar.view \
    -tearoffcommand .SCRIPT.MENUBAR.VIEW \
    -tearoff 0
  .script.menubar.view add command  \
    -command {Script_do_Font}  \
    -label {Font ...} \
    -accelerator {Alt-v f} \
    -underline 0
  .script.menubar.view add command  \
    -command {View_Set_Save_Options script}  \
    -label {Save Script appearance} \
    -accelerator {Alt-v s} \
    -underline 0
  .script.menubar.view add command  \
    -command {Script_do_SaveOptions}  \
    -label {Save all appearances} \
    -accelerator {Alt-v a} \
    -underline 5


  #
  # Add the menus as cascades of the toplevel's menu;
  # add the toplevel's menu to the toplevel
  #
  .script.menubar add cascade \
    -menu .script.menubar.file \
    -label {File} \
    -underline 0
  .script.menubar add cascade \
    -menu .script.menubar.edit \
    -label {Edit} \
    -underline 0
  .script.menubar add cascade \
    -menu .script.menubar.execute \
    -label {Execute} \
    -underline 1
  .script.menubar add cascade \
    -menu .script.menubar.options \
    -label {Options} \
    -underline 0
  .script.menubar add cascade \
    -menu .script.menubar.view \
    -label {View} \
    -underline 0
  .script.menubar add cascade \
    -menu .script.menubar.tools \
    -label {Tools} \
    -underline 0
  .script.menubar add cascade \
    -menu .script.menubar.help \
    -label {Help} \
    -underline 0
  .script configure \
    -menu .script.menubar

  #
  # Add the main frame and the bottom frame to the toplevel .script
  #
  pack append .script \
    .script.main_frm {top frame center expand fill} \
    .script.check_frm {top frame center fillx}

  EndSrc.script

  if {"[info procs XFEdit]" != ""} {
    catch "XFMiscBindWidgetTree .script"
    after 2 "catch {XFEditSetShowWindows}"
  }
}

# proc StartupSrc.script {args}
proc StartupSrc.script {args} {
# script startup entrance
# as much of the C/global dependencies managed here as possible
  global ascScripVect ascGlobalVect env
  if {[catch {set ascScripVect(windowname)} ]} {
    set ascScripVect(minsize) 10x10
    set ascScripVect(geometry) 37x21+10+10
    set ascScripVect(iconname) Script
    set ascScripVect(initialstate) normal
    set ascScripVect(font) "-*-*"
  }
  if {[catch {set ascGlobalVect(font)} ]} {
    set ascGlobalVect(font) "-*-*"
    set ascGlobalVect(labelfont) "-*-*"
    set ascGlobalVect(tbg) "white"
    set ascGlobalVect(tfg) "black"
    set ascGlobalVect(bg) "white"
    set ascGlobalVect(fg) "black"
    set ascGlobalVect(afg) "white"
    set ascGlobalVect(abg) "black"
    set ascGlobalVect(sfg) "white"
    set ascGlobalVect(sbg) "black"
    set ascGlobalVect(visibility) 1
    set ascGlobalVect(c_loaded) "0"
    set ascGlobalVect(toolbitmap) \
      "@$env(ASCENDBITMAPS)/toolAttributes.xbm"
  }
}

# proc EndSrc.script {}
proc EndSrc.script {} {
# script startup exit
  global ascScripVect
  set ascScripVect(geometry) [sanegeometry $ascScripVect(geometry)]
  set minw [lindex [split $ascScripVect(minsize) x] 0]
  set minh [lindex [split $ascScripVect(minsize) x] 1]
  set gw [lindex [split [lindex [split $ascScripVect(geometry) +-] 0] x] 0]
  set gh [lindex [split [lindex [split $ascScripVect(geometry) +-] 0] x] 1]
  set gpx [lindex [split [split $ascScripVect(geometry) x] +-] 1]
  set gpy [lindex [split [split $ascScripVect(geometry) x] +-] 2]
  if {[expr $gw < $minw]} {set gw $minw}
  if {[expr $gh < $minh]} {set gh $minh}
  set gwh "${gw}x${gh}"
  wm minsize .script $minw $minh
  wm geometry .script "${gwh}+${gpx}+$gpy"
  # wm geometry .script [gospos .script $gpx $gpy]
  wm iconname .script $ascScripVect(iconname)

  if {$ascScripVect(initialstate)!="iconic" && \
      $ascScripVect(initialstate)!="iconified" && \
      $ascScripVect(initialstate) !="withdrawn"} {
    wm deiconify .script
  }
  if {"$ascScripVect(initialstate)"=="withdrawn"} {
    wm withdraw .script
  }

}


#
# proc build_scriptbox {parentname winnum font}
#---------------------------------------------------------------------------
# creates a text widget with scrollbars in the context
# of parentname, if same does not already exist.
# the name of the text widget is
# $parentname.script_box_$winnum
# returns the name $parentname.script_box_$winnum
# The widget with this name has a child named text2.
# scrollbars are done with bg/fg colors
# text is done with tbg/tfg, sbg/sfg for plain and selected text,
# respectively.
# if the widget already exists, recolors it from the given input.
#---------------------------------------------------------------------------
#
proc build_scriptbox {parentname winnum font} {

  if {[winfo exists  $parentname.script_box_$winnum]} {
    return $parentname.script_box_$winnum
  }

  # build widget $parentname.script_box_$winnum
  frame $parentname.script_box_$winnum \
    -relief {raised}

  # build widget $parentname.script_box_$winnum.scrollbar1
  scrollbar $parentname.script_box_$winnum.scrollbar1 \
    -command "$parentname.script_box_$winnum.text2 yview"

  # build widget $parentname.script_box_$winnum.text2
  text $parentname.script_box_$winnum.text2  \
    -exportselection 0 \
    -borderwidth {2} \
    -height {2} \
    -relief {raised} \
    -setgrid {1} \
    -width {10} \
    -wrap {word} \
    -yscrollcommand "$parentname.script_box_$winnum.scrollbar1 set" \
    -font $font

  # pack widget $parentname.script_box_$winnum
  pack append $parentname.script_box_$winnum \
     $parentname.script_box_$winnum.scrollbar1 {right frame center filly} \
     $parentname.script_box_$winnum.text2 {top frame center expand fill}
  catch {emacs-bind $parentname.script_box_$winnum.text2}
  set w $parentname.script_box_$winnum.text2
  bind $w <F2> Script_do_Cut
  bind $w <F3> Script_do_Copy
  bind $w <F4> Script_do_Paste
  bind $w <F5> {
    Script_do_ExecuteStats
  }
  bind $w <F6> {
    Script_do_ExecuteStats 0
  }
  $w mark set insert end
}


# Internal procedures

# eof
#
