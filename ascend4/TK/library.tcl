#  library.tcl
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.28 $
#  Last modified on: $Date: 2003/01/19 01:27:47 $
#  Last modified by: $Author: ballan $
#  Revision control file: $RCSfile: library.tcl,v $
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

# Module: library.tcl
# Tcl version: 7.1 (Tcl/Tk/XF)
# Tk version: 3.4
# XF version: 2.2
#

# module contents
global moduleList
global autoLoadList
set moduleList(library.tcl) {.library}
set autoLoadList(library.tcl) {0}

# procedures to show toplevel windows


# procedure to show window ShowWindow.library
# proc ShowWindow.library { args}
proc ShowWindow.library { args} {
  # xf ignore me 7

  global env ascGlobalVect ascLibrVect
  StartupSrc.library

  # build widget .library
  if {"[info procs XFEdit]" != ""} {
    catch "XFDestroy .library"
  } {
    catch "destroy .library"
  }
  toplevel .library

  # Ascend patched window manager configurations
  wm iconify .library
  wm positionfrom .library user
  wm sizefrom .library user
  wm iconname .library {Library}
  wm minsize .library 100 100
  wm title .library {A4 Library}
  wm protocol .library WM_DELETE_WINDOW {Toggle_Remote ascLibrVect}

  # build widget .library.main_frm
  frame .library.main_frm \
    -borderwidth 0

  # build widget .library.main_frm.file_box
  frame .library.main_frm.file_box

  # build widget .library.main_frm.file_box.scrollbar2
  scrollbar .library.main_frm.file_box.scrollbar2 \
    -command {.library.main_frm.file_box.listbox1 yview}

  # build widget .library.main_frm.file_box.listbox1
  listbox .library.main_frm.file_box.listbox1 \
    -relief {raised} \
    -yscrollcommand {.library.main_frm.file_box.scrollbar2 set} \
    -font $ascLibrVect(font) \
    -exportselection 0 \
    -width 20 \
    -height 2

  # pack widget .library.main_frm.file_box
  pack append .library.main_frm.file_box \
    .library.main_frm.file_box.scrollbar2 {left frame center filly} \
    .library.main_frm.file_box.listbox1 {top frame center expand fill}

  # build widget .library.main_frm.model_box
  frame .library.main_frm.model_box

  # build widget .library.main_frm.model_box.scrollbar2
  scrollbar .library.main_frm.model_box.scrollbar2 \
    -command {.library.main_frm.model_box.listbox1 yview}

  # build widget .library.main_frm.model_box.listbox1
  listbox .library.main_frm.model_box.listbox1 \
    -relief {raised} \
    -yscrollcommand {.library.main_frm.model_box.scrollbar2 set} \
    -font $ascLibrVect(font) \
    -exportselection 0 \
    -width 8 \
    -height 2

  # pack widget .library.main_frm.model_box
  pack append .library.main_frm.model_box \
    .library.main_frm.model_box.scrollbar2 {right frame center filly} \
    .library.main_frm.model_box.listbox1 {top frame center expand fill}

  # pack widget .library.main_frm
  pack append .library.main_frm \
    .library.main_frm.file_box {left frame center fill} \
    .library.main_frm.model_box {left frame center expand fill}


  #
  #  Build the entry box
  #
  frame .library.entry_frm \
    -borderwidth {0} \
    -relief {raised}

  entry .library.entry_frm.lib_entry \
    -relief {raised} \
    -width {24} \
    -textvariable ascLibrVect(entryline) \
    -font $ascLibrVect(entryFont) \
    -exportselection 0 \
    -cursor hand2

  # pack widget .library.entry_frm
  pack append .library.entry_frm \
    .library.entry_frm.lib_entry {left frame w pady 5 expand fill}


  #
  # Build the scrollable listbox to hold the simulation names
  #
  frame .library.sims_frm \
    -borderwidth {0}
  frame .library.sims_frm.sims_box \
    -borderwidth {0}

  # pack widget .library.sims_frm
  pack append .library.sims_frm \
    .library.sims_frm.sims_box {left frame center expand fill}

  # build widget .library.sims_frm.sims_box.listbox1
  listbox .library.sims_frm.sims_box.listbox1 \
    -relief {raised} \
    -xscrollcommand {.library.sims_frm.sims_box.scrollbar3 set} \
    -yscrollcommand {.library.sims_frm.sims_box.scrollbar2 set} \
    -font $ascLibrVect(font) \
    -exportselection 0 \
    -width 1 \
    -height 3

  # build widget .library.sims_frm.sims_box.scrollbar2
  scrollbar .library.sims_frm.sims_box.scrollbar2 \
    -command {.library.sims_frm.sims_box.listbox1 yview}

  # build widget .library.sims_frm.sims_box.scrollbar3
  scrollbar .library.sims_frm.sims_box.scrollbar3 \
    -command {.library.sims_frm.sims_box.listbox1 xview} \
    -orient {horizontal}

  # pack widget .library.sims_frm.sims_box
  pack append .library.sims_frm.sims_box \
    .library.sims_frm.sims_box.scrollbar2 {right frame center filly} \
    .library.sims_frm.sims_box.listbox1 {top frame center expand fill} \
    .library.sims_frm.sims_box.scrollbar3 {bottom frame center fillx}


  #
  # Build the menu bar and submenus
  #
  menu .library.menubar \
    -tearoffcommand .LIBRARY.MENUBAR \
    -tearoff 0

  # The Display menu
  menu .library.menubar.display \
    -tearoffcommand .LIBRARY.MENUBAR.DISPLAY \
    -tearoff 0
  .library.menubar.display add command \
    -command {Disp_do_ShowCode} \
    -label {Code} \
    -accelerator {Alt-d c} \
    -underline 0
  .library.menubar.display add command \
    -command {Disp_do_ShowAncestry} \
    -label {Ancestry} \
    -accelerator {Alt-d a} \
    -underline 0
  .library.menubar.display add command \
    -command {Disp_do_Hierarchy} \
    -label {Refinement hierarchy} \
    -accelerator {Alt-d r} \
    -underline 0
  .library.menubar.display add command \
    -command {Disp_do_PrintExtFuncLibrary} \
    -label {External functions} \
    -accelerator {Alt-d e} \
    -underline 0
  .library.menubar.display add cascade \
    -menu .library.menubar.display.hide \
    -label {Hide type} \
    -accelerator {Alt-d h} \
    -underline 0
  .library.menubar.display add cascade \
    -menu .library.menubar.display.unhide \
    -label {UnHide type} \
    -accelerator {Alt-d u} \
    -underline 0
  .library.menubar.display add command \
    -command {Libr_do_Hide_Fundamentals} \
    -label {Hide/Show Fundamentals...} \
    -accelerator {Alt-d f} \
    -underline 10

  # The Hide menu under the Display menu
  menu .library.menubar.display.hide \
    -tearoffcommand .LIBRARY.MENUBAR.DISPLAY.HIDE \
    -tearoff 0
  .library.menubar.display.hide add command \
     -command {Libr_do_Hide_Type} \
     -label {Hide selected type} \
     -accelerator {Alt-d h t} \
     -underline 14
  .library.menubar.display.hide add command \
     -command {Libr_do_Hide_Type_Refinements} \
     -label {Hide type and its refinements} \
     -accelerator {Alt-d h r} \
     -underline 18

  # The UnHide menu under the Display menu
  menu .library.menubar.display.unhide \
    -tearoffcommand .LIBRARY.MENUBAR.DISPLAY.UNHIDE \
    -tearoff 0
  .library.menubar.display.unhide add command \
     -command {Libr_do_UnHide_Type} \
     -label {UnHide selected type} \
     -accelerator {Alt-d u t} \
     -underline 16
  .library.menubar.display.unhide add command \
     -command {Libr_do_UnHide_Type_Refinements} \
     -label {UnHide type and its refinements} \
     -accelerator {Alt-d u r} \
     -underline 20

  # The Edit menu
  menu .library.menubar.edit \
    -tearoffcommand .LIBRARY.MENUBAR.EDIT \
    -tearoff 0
  .library.menubar.edit add command \
    -command {Libr_do_compile} \
    -label {Create simulation...} \
    -accelerator {Alt-e c} \
    -underline 0
  .library.menubar.edit add command \
    -command Libr_SuggestMethods \
    -label {Suggest methods...} \
    -accelerator {Alt-e m} \
    -underline 8
  .library.menubar.edit add separator
  .library.menubar.edit add command  \
    -command {Sims_do_Delete} \
    -label {Delete simulation...} \
    -accelerator {Alt-e s} \
    -underline 7
  .library.menubar.edit add command \
    -command {Libr_do_DeleteAll} \
    -label {Delete all types...} \
    -accelerator {Alt-e t} \
    -underline 11


  # The Export menu
  menu .library.menubar.export \
    -tearoffcommand .LIBRARY.MENUBAR.EXPORT \
    -tearoff 0
  .library.menubar.export add command \
    -command {Sims_do_Export2Browser} \
    -label {Simulation to Browser} \
    -accelerator {Alt-x b} \
    -underline 14
  .library.menubar.export add command \
    -command {Sims_do_Export2Solver} \
    -label {Simulation to Solver} \
    -accelerator {Alt-x s} \
    -underline 14
  .library.menubar.export add command \
    -command {Sims_do_Export2Probe} \
    -label {Simulation to Probe} \
    -accelerator {Alt-x p} \
    -underline 14


  # The File menu
  menu .library.menubar.file \
    -tearoffcommand .LIBRARY.MENUBAR.FILE \
    -tearoff 0
  .library.menubar.file add command \
    -command {Libr_do_read} \
    -label {Read types from file...} \
    -accelerator {Alt-f r} \
    -underline 0
  .library.menubar.file add separator
  .library.menubar.file add command \
    -command {Toggle_Remote ascLibrVect} \
    -label {Close window} \
    -accelerator {Alt-f c} \
    -underline 0
  .library.menubar.file add command \
    -command {Script_do_Exit} \
    -accelerator {Alt-f e} \
    -label {Exit ASCEND...} \
    -underline 0


  # The Find menu
  menu .library.menubar.find \
    -tearoffcommand .LIBRARY.MENUBAR.FIND \
    -tearoff 0
  .library.menubar.find add command \
    -command {Libr_do_FindAtom} \
    -label {ATOM by units...} \
    -accelerator {Alt-i a} \
    -underline 0
  .library.menubar.find add command \
    -command {Libr_do_FindType} \
    -label {Type by name...} \
    -accelerator {Alt-i t} \
    -underline 0
  .library.menubar.find add command \
    -command {Libr_do_FindFuzzy} \
    -label {Type by fuzzy name...} \
    -accelerator {Alt-i f} \
    -underline 8
  .library.menubar.find add cascade \
    -menu .library.menubar.find.pendings \
    -accelerator {Alt-i p} \
    -label {Pending statements} \
    -underline 0

  # The Pendings submenu under the Find menu
  menu .library.menubar.find.pendings \
    -tearoffcommand .LIBRARY.MENUBAR.FIND.PENDINGS \
    -tearoff 0
  .library.menubar.find.pendings add command  \
    -command {Disp_do_ShowPendings} \
    -accelerator {Alt-i p d} \
    -label {To Display} \
    -underline 3
  .library.menubar.find.pendings add command  \
    -command {Sims_Pendings_To_Stdout} \
    -accelerator {Alt-i p c} \
    -label {To Console} \
    -underline 3
  .library.menubar.find.pendings add command  \
    -command {Sims_Pendings_To_File} \
    -accelerator {Alt-i p f} \
    -label {To File ...} \
    -underline 3



  # The Help menu
  menu .library.menubar.help \
    -tearoffcommand .LIBRARY.MENUBAR.HELP \
    -tearoff 0
  .library.menubar.help add command \
    -command {Libr_do_Help} \
    -label {On Library} \
    -underline 3


  # The option menu
  menu .library.menubar.option \
    -tearoffcommand .LIBRARY.MENUBAR.OPTION \
    -tearoff 0
  .library.menubar.option add radiobutton \
    -label {Report all parser messages} \
    -underline 6 \
    -accelerator {Alt-o a} \
    -selectcolor red \
    -variable ascLibrVect(parserWarnings) \
    -value 1
  .library.menubar.option add radiobutton \
    -label {Suppress parser style messages} \
    -underline 9 \
    -accelerator {Alt-o p} \
    -selectcolor red \
    -variable ascLibrVect(parserWarnings) \
    -value 2
  .library.menubar.option add radiobutton \
    -label {Suppress parser warning messages} \
    -underline 16 \
    -accelerator {Alt-o w} \
    -selectcolor red \
    -variable ascLibrVect(parserWarnings) \
    -value 3
  .library.menubar.option add radiobutton \
    -label {Suppress parser error messages} \
    -underline 16 \
    -accelerator {Alt-o e} \
    -selectcolor red \
    -variable ascLibrVect(parserWarnings) \
    -value 4
  .library.menubar.option add separator
  .library.menubar.option add checkbutton \
    -offvalue {0} \
    -onvalue {1} \
    -variable {ascLibrVect(compileC)} \
    -label {Generate C binary} \
    -accelerator {Alt-o b} \
    -underline 11
  .library.menubar.option add checkbutton \
    -offvalue {1} \
    -onvalue {0} \
    -variable {ascLibrVect(ignorestop)} \
    -label {Ignore STOP in METHODS} \
    -accelerator {Alt-o i} \
    -underline 0
  .library.menubar.option add checkbutton \
    -offvalue {1} \
    -onvalue {0} \
    -variable {ascLibrVect(compilerWarnings)} \
    -label {Suppress compiler error messages} \
    -accelerator {Alt-o m} \
    -underline 24
  .library.menubar.option add checkbutton \
    -offvalue {0} \
    -onvalue {1} \
    -variable {ascLibrVect(simplifyRelations)} \
    -label {Simplify compiled equations} \
    -accelerator {Alt-o c} \
    -underline 9
  .library.menubar.option add checkbutton \
    -offvalue {0} \
    -onvalue {1} \
    -variable {ascLibrVect(useCopyAnon)} \
    -label {Use relation sharing by anonymous type} \
    -accelerator {Alt-o r} \
    -underline 4
  .library.menubar.option add separator
  .library.menubar.option add command  \
    -command {Libr_do_SaveOption}  \
    -label {Save options} \
    -accelerator {Alt-o s} \
    -underline 0

  # The view menu
  menu .library.menubar.view \
    -tearoffcommand .LIBRARY.MENUBAR.VIEW \
    -tearoff 0
  .library.menubar.view add command  \
    -command {Libr_do_Font}  \
    -label {Font ...} \
    -accelerator {Alt-v f} \
    -underline 0
  .library.menubar.view add checkbutton \
    -offvalue {0} \
    -onvalue {1} \
    -variable {ascLibrVect(visibility)} \
    -label {Open automatically} \
    -accelerator {Alt-v o} \
    -underline 0
  .library.menubar.view add command  \
    -command {View_Save_Window_Appearance library}  \
    -accelerator {Alt-v s} \
    -label {Save appearance} \
    -underline 0

  #
  # Add the menus as cascades of the toplevel's menu;
  # add the toplevel's menu to the toplevel
  #
  .library.menubar add cascade \
    -menu .library.menubar.file \
    -label {File} \
    -underline 0
  .library.menubar add cascade \
    -menu .library.menubar.edit \
    -label {Edit} \
    -underline 0
  .library.menubar add cascade \
    -menu .library.menubar.display \
    -label {Display} \
    -underline 0
  .library.menubar add cascade \
    -menu .library.menubar.find \
    -label {Find} \
    -underline 1
  .library.menubar add cascade \
    -menu .library.menubar.option \
    -label {Options} \
    -underline 0
  .library.menubar add cascade \
    -menu .library.menubar.view \
    -label {View} \
    -underline 0
  .library.menubar add cascade \
    -menu .library.menubar.export \
    -label {Export} \
    -underline 1
  .library.menubar add cascade \
    -menu .library.menubar.help \
    -label {Help} \
    -underline 0
  .library configure \
    -menu .library.menubar


  #
  #  Pack the library widget
  #
  pack append .library  \
    .library.main_frm {top frame center expand fill} \
    .library.entry_frm {top frame center fillx} \
    .library.sims_frm {top frame center fillx}

  EndSrc.library

  if {"[info procs XFEdit]" != ""} {
    catch "XFMiscBindWidgetTree .library"
    after 2 "catch {XFEditSetShowWindows}"
  }
}

# proc DestroyWindow.library {}
proc DestroyWindow.library {} {# xf ignore me 7
  if {"[info procs XFEdit]" != ""} {
    if {"[info commands .library]" != ""} {
      global xfShowWindow.library
      set xfShowWindow.library 0
      XFEditSetPath .
      after 2 "XFSaveAsProc .library; XFEditSetShowWindows"
    }
  } {
    catch "destroy .library"
    update
  }
}

# proc StartupSrc.library {args}
proc StartupSrc.library {args} {
# library startup entrance
# as much of the C/global dependencies managed here as possible
  global ascLibrVect ascGlobalVect env
  if {[catch {set ascLibrVect(windowname)} ]} {
    set ascLibrVect(minsize) 100x100
    set ascLibrVect(geometry) 350x350+10+10
    set ascLibrVect(iconname) Library
    set ascLibrVect(initialstate) normal
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

# proc EndSrc.library {}
proc EndSrc.library {} {
# library startup exit
  global ascLibrVect env
  set ascLibrVect(geometry) [sanegeometry $ascLibrVect(geometry)]
  set minw [lindex [split $ascLibrVect(minsize) x] 0]
  set minh [lindex [split $ascLibrVect(minsize) x] 1]
  set gw [lindex [split [lindex [split $ascLibrVect(geometry) +] 0] x] 0]
  set gh [lindex [split [lindex [split $ascLibrVect(geometry) +] 0] x] 1]
  set gp "+[lindex [split [split $ascLibrVect(geometry) x] +] 1]+[lindex [split [split $ascLibrVect(geometry) x] +] 2]"
  if {[expr $gw < $minw]} {set gw $minw}
  if {[expr $gh < $minh]} {set gh $minh}
  set gwh "${gw}x${gh}"
  wm minsize .library $minw $minh
  wm geometry .library [osgpos $gwh$gp]
  wm iconname .library $ascLibrVect(iconname)

  if {$ascLibrVect(initialstate)!="iconic" && \
      $ascLibrVect(initialstate)!="iconified" && \
      $ascLibrVect(initialstate)!="withdrawn"} {
    wm deiconify .library
  }
  if {$ascLibrVect(initialstate)=="withdrawn"} {
    wm withdraw .library
  }
}


# eof
#
