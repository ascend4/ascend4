#  browser.tcl
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.43 $
#  Last modified on: $Date: 1998/06/18 15:55:20 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: browser.tcl,v $
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

# Module: browser.tcl
# Tcl version: 7.1 (Tcl/Tk/XF)
# Tk version: 3.4
# XF version: 2.2
#

# module contents
global moduleList
global autoLoadList
set moduleList(browser.tcl) { .browser}
set autoLoadList(browser.tcl) {0}

# procedures to show toplevel windows


# procedure to show window ShowWindow.browser
proc ShowWindow.browser { args} { # xf ignore me 7

  global ascBrowVect ascGlobalVect
StartupSrc.browser

  # build widget .browser
  if {"[info procs XFEdit]" != ""} {
    catch "XFDestroy .browser"
  } {
    catch "destroy .browser"
  }
  toplevel .browser

  # Window manager configurations
  wm iconify .browser
  wm positionfrom .browser user
  wm sizefrom .browser user
  wm iconname .browser {Browser}
  wm minsize .browser 100 100
  wm title .browser {A4 Browser}
  wm protocol .browser WM_DELETE_WINDOW {Toggle_Remote ascBrowVect}


  # build widget .browser.main_frm
  frame .browser.main_frm

  # build widget .browser.main_frm.child_box
  frame .browser.main_frm.child_box

  # build widget .browser.main_frm.child_box.scrollbar2
  scrollbar .browser.main_frm.child_box.scrollbar2 \
    -command {.browser.main_frm.child_box.listbox1 yview}

  # build widget .browser.main_frm.child_box.scrollbar3
  scrollbar .browser.main_frm.child_box.scrollbar3 \
    -command {.browser.main_frm.child_box.listbox1 xview} \
    -orient {horizontal}

  # build widget .browser.main_frm.child_box.listbox1
  listbox .browser.main_frm.child_box.listbox1 \
    -relief {raised} \
    -xscrollcommand {.browser.main_frm.child_box.scrollbar3 set} \
    -yscrollcommand {.browser.main_frm.child_box.scrollbar2 set} \
    -font $ascBrowVect(font) \
    -width 18 \
    -height 2 \
    -exportselection 0


  # pack widget .browser.main_frm.child_box
  pack append .browser.main_frm.child_box \
    .browser.main_frm.child_box.scrollbar2 {right frame center filly} \
    .browser.main_frm.child_box.listbox1 {top frame center expand fill} \
    .browser.main_frm.child_box.scrollbar3 {bottom frame center fillx}

  # build widget .browser.main_frm.parents_box
  frame .browser.main_frm.parents_box

  # build widget .browser.main_frm.parents_box.scrollbar2
  scrollbar .browser.main_frm.parents_box.scrollbar2 \
    -command {.browser.main_frm.parents_box.listbox1 yview}

  # build widget .browser.main_frm.parents_box.scrollbar3
  scrollbar .browser.main_frm.parents_box.scrollbar3 \
    -command {.browser.main_frm.parents_box.listbox1 xview} \
    -orient {horizontal}

  # build widget .browser.main_frm.parents_box.listbox1
  listbox .browser.main_frm.parents_box.listbox1\
    -relief {raised} \
    -xscrollcommand {.browser.main_frm.parents_box.scrollbar3 set} \
    -yscrollcommand {.browser.main_frm.parents_box.scrollbar2 set} \
    -font $ascBrowVect(font) \
    -width 12 \
    -height 2

  # pack widget .browser.main_frm.parents_box
  pack append .browser.main_frm.parents_box \
      .browser.main_frm.parents_box.scrollbar2 {left frame center filly} \
      .browser.main_frm.parents_box.listbox1 {top frame center expand fill} \
      .browser.main_frm.parents_box.scrollbar3 {bottom frame center fillx}

  # pack widget .browser.main_frm
  pack append .browser.main_frm \
      .browser.main_frm.parents_box {left frame center filly} \
      .browser.main_frm.child_box {left frame center expand fill}


  # build widget .browser.local_frm
  frame .browser.local_frm

  # build widget .browser.local_frm.child_box
  frame .browser.local_frm.child_box

  # build widget .browser.local_frm.child_box.scrollbar2
  scrollbar .browser.local_frm.child_box.scrollbar2 \
    -command {.browser.local_frm.child_box.listbox1 yview}

  # build widget .browser.local_frm.child_box.scrollbar3
  scrollbar .browser.local_frm.child_box.scrollbar3 \
    -command {.browser.local_frm.child_box.listbox1 xview} \
    -orient {horizontal}

  # build widget .browser.local_frm.child_box.listbox1
  listbox .browser.local_frm.child_box.listbox1 \
    -relief {raised} \
    -xscrollcommand {.browser.local_frm.child_box.scrollbar3 set} \
    -yscrollcommand {.browser.local_frm.child_box.scrollbar2 set} \
    -font $ascBrowVect(font) \
    -width 18 \
    -height 2 \
    -exportselection 0


  # pack widget .browser.local_frm.child_box
  pack append .browser.local_frm.child_box \
    .browser.local_frm.child_box.scrollbar2 {right frame center filly} \
    .browser.local_frm.child_box.listbox1 {top frame center expand fill} \
    .browser.local_frm.child_box.scrollbar3 {bottom frame center fillx}

  # build widget .browser.local_frm.check_box
  frame .browser.local_frm.check_box
  
  set rr .browser.local_frm.check_box

  # The command option of these checkbuttons is ugly.
  # should instead of passing an arg, just use the traces
  # on ascBrowVect(localshow,*) to set a (lasttoggled) value
  # so that Brow_do_UpdateLocalBox doesn't need an argument.

  # build rr.b1
  checkbutton $rr.b1 \
    -variable ascBrowVect(localshow,REAL_ATOM_INST) \
    -command {Brow_do_UpdateLocalBox REAL_ATOM_INST} \
    -text RV 

  # build rr.b2
  checkbutton $rr.b2 \
    -variable ascBrowVect(localshow,INTEGER_ATOM_INST) \
    -command {Brow_do_UpdateLocalBox INTEGER_ATOM_INST} \
    -text DV

  # build rr.b3
  checkbutton $rr.b3 \
    -variable ascBrowVect(localshow,REL_INST) \
    -command {Brow_do_UpdateLocalBox REL_INST} \
    -text RR

  # build rr.b4
  checkbutton $rr.b4 \
    -variable ascBrowVect(localshow,LREL_INST) \
    -command {Brow_do_UpdateLocalBox LREL_INST} \
    -text LR

  # build rr.b5
  checkbutton $rr.b5 \
    -variable ascBrowVect(localshow,REAL_CONSTANT_INST) \
    -command {Brow_do_UpdateLocalBox REAL_CONSTANT_INST} \
    -text RC

  # build rr.b6
  checkbutton $rr.b6 \
    -variable ascBrowVect(localshow,SET_ATOM_INST) \
    -command {Brow_do_UpdateLocalBox SET_ATOM_INST} \
    -text DC 

  # build rr.label
  label $rr.label \
    -textvariable ascBrowVect(locallabel) \
    -justify right

  # pack widget .browser.local_frm.check_box
  pack append .browser.local_frm.check_box \
      $rr.b1 {left frame center fill} \
      $rr.b2 {left frame center fill} \
      $rr.b3 {left frame center fill} \
      $rr.b4 {left frame center fill} \
      $rr.b5 {left frame center fill} \
      $rr.b6 {left frame center fill} \
      $rr.label {left frame center expand fill}

  # pack widget .browser.main_frm
  pack append .browser.local_frm \
      .browser.local_frm.child_box {top frame center expand fill} \
      .browser.local_frm.check_box {top frame center fill}

  #
  # Build the menu bar and submenus
  #
  menu .browser.menubar \
    -tearoffcommand .BROWSER.MENUBAR \
    -tearoff 0

  # The Display menu
  menu .browser.menubar.display \
    -tearoffcommand .BROWSER.MENUBAR.DISPLAY \
    -tearoff 0
  .browser.menubar.display add command  \
    -command {Brow_do_DispAttr} \
    -label {Attributes} \
    -accelerator {Alt-d a} \
    -underline 0
  .browser.menubar.display add command  \
    -command {Brow_do_DispRelations} \
    -label {Relations} \
    -accelerator {Alt-d r} \
    -underline 0
  .browser.menubar.display add command  \
    -command {Brow_do_DispCondRels} \
    -label {Conditional Relations} \
    -accelerator {Alt-d c} \
    -underline 0
  .browser.menubar.display add command  \
    -command {Brow_do_DispLogRels} \
    -label {Logical Relations} \
    -accelerator {Alt-d l} \
    -underline 0
  .browser.menubar.display add command  \
    -command {Brow_do_DispCondLogRels} \
    -label {Conditional Logical Relations} \
    -accelerator {Alt-d o} \
    -underline 1
  .browser.menubar.display add command  \
    -command {Brow_do_DispWhens} \
    -label {Whens} \
    -accelerator {Alt-d w} \
    -underline 0
  .browser.menubar.display add separator
  .browser.menubar.display add command  \
    -command {Brow_do_Plot} \
    -accelerator {Alt-d p} \
    -label {Plot} \
    -underline 0
  .browser.menubar.display add command  \
    -command {Brow_do_Statistics} \
    -accelerator {Alt-d s} \
    -label {Statistics} \
    -underline 0

  # The File menu
  menu .browser.menubar.file \
    -tearoffcommand .BROWSER.MENUBAR.FILE \
    -tearoff 0
  .browser.menubar.file add command \
    -command {Brow_do_Write} \
    -label {Write values...} \
    -accelerator {Alt-f w} \
    -underline 0
  .browser.menubar.file add command \
    -command {Brow_do_Read} \
    -label {Read values...} \
    -accelerator {Alt-f r} \
    -underline 0
  .browser.menubar.file add separator
  .browser.menubar.file add command  \
    -command {Toggle_Remote ascBrowVect} \
    -accelerator {Alt-f c} \
    -label {Close window} \
    -underline 0
  .browser.menubar.file add command  \
    -command {Script_do_Exit} \
    -accelerator {Alt-f e} \
    -label {Exit ASCEND...} \
    -underline 0

  # The Edit menu
  menu .browser.menubar.edit \
    -tearoffcommand .BROWSER.MENUBAR.EDIT \
    -tearoff 0
  .browser.menubar.edit add command  \
    -command {Browser_do_Methods} \
    -label {Run method...} \
    -accelerator {Alt-e u} \
    -underline 1
  .browser.menubar.edit add command  \
    -command {Browser_do_ClearVars}  \
    -accelerator {Alt-e c} \
    -label {Clear Vars} \
    -underline 0
  .browser.menubar.edit add separator
  .browser.menubar.edit add command \
    -command {Browser_do_SetValue} \
    -accelerator {Alt-e v} \
    -label {Set value...} \
    -underline 4
    # -command {Brow_do_ParentsSetValue}
  .browser.menubar.edit add command \
    -command {Browser_do_Refine} \
    -accelerator {Alt-e r} \
    -label {Refine...} \
    -underline 0
  .browser.menubar.edit add command \
    -command {Browser_do_Merge} \
    -accelerator {Alt-e m} \
    -label {Merge...} \
    -underline 0
  .browser.menubar.edit add separator
  .browser.menubar.edit add cascade \
    -label {Compile} \
    -menu {.browser.menubar.edit.compile}

  # build widget .browser.menubar.edit.compile
  menu .browser.menubar.edit.compile \
    -tearoffcommand .BROWSER.MENUBAR.EDIT.COMPILE \
    -tearoff 0
  .browser.menubar.edit.compile add command \
    -command {Browser_do_Resume} \
    -accelerator {F5} \
    -underline -1 \
    -label {Resume Compilation}
  .browser.menubar.edit.compile add command \
    -command {Browser_do_CreatePart}  \
    -label {Create Part} \
    -state disabled


  # The Export menu
  menu .browser.menubar.export \
    -tearoffcommand .BROWSER.MENUBAR.EXPORT \
    -tearoff 0
  .browser.menubar.export add command \
    -command {Brow_do_Export2Solver} \
    -label {to Solver} \
    -accelerator {Alt-x s} \
    -underline 3
  .browser.menubar.export add command \
    -command {Brow_do_Export2Probe} \
    -label {Many to Probe ...} \
    -accelerator {Alt-x m} \
    -underline 0
  .browser.menubar.export add command \
    -command {Brow_do_ExportOne2Probe} \
    -accelerator {Alt-x p} \
    -label {Item to Probe} \
    -underline 8


  # The Find menu
  menu .browser.menubar.find \
    -tearoffcommand .BROWSER.MENUBAR.FIND \
    -tearoff 0
  .browser.menubar.find add command \
    -command {Brow_do_FindbyName} \
    -label {By name...} \
    -accelerator {Alt-i n} \
    -underline 3
  .browser.menubar.find add command \
    -command {Brow_do_FindbyType} \
    -label {By type...} \
    -accelerator {Alt-i t} \
    -underline 3
  .browser.menubar.find add command \
    -command {Brow_do_FindAliases} \
    -label {Aliases} \
    -accelerator {Alt-i a} \
    -underline 0
  .browser.menubar.find add command \
    -command {Brow_do_FindISAs} \
    -label {Where created} \
    -accelerator {Alt-i w} \
    -underline 0
  .browser.menubar.find add command \
    -command {Brow_do_FindClique} \
    -accelerator {Alt-i c} \
    -label {Clique} \
    -underline 0
  .browser.menubar.find add command \
    -command {Brow_do_FindEligible} \
    -label {Eligible variables} \
    -accelerator {Alt-i e} \
    -underline 0
  .browser.menubar.find add command \
    -command {Brow_do_FindActive} \
    -label {Active relations} \
    -accelerator {Alt-i r} \
    -underline 7
  .browser.menubar.find add command \
    -command {Brow_do_FindOpers} \
    -label {Operands} \
    -accelerator {Alt-i o} \
    -underline 0
  .browser.menubar.find add command \
    -command {Brow_do_FindParents} \
    -label {Parents} \
    -accelerator {Alt-i p} \
    -underline 0
  .browser.menubar.find add command \
    -command {Brow_do_FindPendings} \
    -label {Pendings} \
    -accelerator {Alt-i d} \
    -underline 3


  # The Help menu
  menu .browser.menubar.help \
    -tearoffcommand .BROWSER.MENUBAR.HELP \
    -tearoff 0
  .browser.menubar.help add command \
    -command {Brow_do_Help} \
    -label {On Browser} \
    -underline 3


  # The Options menu
  menu .browser.menubar.options \
    -tearoffcommand .BROWSER.MENUBAR.options \
    -tearoff 0
  .browser.menubar.options add checkbutton \
    -command {Brow_do_TypeorValue} \
    -offvalue {PASSED} \
    -onvalue {} \
    -variable {ascBrowVect(HidePassed)} \
    -accelerator {Alt-o p} \
    -underline 5 \
    -label {Hide Passed Parts}
  .browser.menubar.options add checkbutton \
    -command {Brow_do_TypeorValue} \
    -offvalue ATOMS \
    -onvalue {} \
    -variable {ascBrowVect(ShowAtoms)} \
    -accelerator {Alt-o a} \
    -underline 9 \
    -label {Suppress atoms}
  .browser.menubar.options add checkbutton \
    -command {Brow_do_TypeorValue} \
    -offvalue {TYPE} \
    -onvalue {VALUE} \
    -variable {ascBrowVect(TypeorValue)} \
    -accelerator {Alt-o v} \
    -underline 13 \
    -label {Display Atom Values}
  .browser.menubar.options add checkbutton \
    -offvalue {0} \
    -onvalue {1} \
    -variable {ascBrowVect(dimconsistency)} \
    -accelerator {Alt-o d} \
    -underline 6 \
    -label {Check dimensionality}
  .browser.menubar.options add command \
    -command {Brow_do_SaveOptions} \
    -label {Save options} \
    -accelerator {Alt-o s} \
    -underline 0
  .browser.menubar.options add separator
  .browser.menubar.options add command \
    -command {Brow_do_HideNames} \
    -label {Hide names...} \
    -accelerator {Alt-o h} \
    -underline 0
  .browser.menubar.options add command \
    -command {Brow_do_UnHideNames} \
    -accelerator {Alt-o u} \
    -label {Unhide names...} \
    -underline 0
  
  # The View menu
  menu .browser.menubar.view \
    -tearoffcommand .BROWSER.MENUBAR.VIEW \
    -tearoff 0
  .browser.menubar.view add command  \
    -command {Brow_do_Font}  \
    -label {Font ...} \
    -accelerator {Alt-v f} \
    -underline 0
  .browser.menubar.view add checkbutton \
    -offvalue {0} \
    -onvalue {1} \
    -variable {ascBrowVect(visibility)} \
    -label {Open automatically} \
    -accelerator {Alt-v o} \
    -underline 0
  .browser.menubar.view add command  \
    -command {View_Set_Save_Options browser}  \
    -label {Save window appearance} \
    -accelerator {Alt-v s} \
    -underline 0

  #
  # Add the menus as cascades of the toplevel's menu;
  # add the toplevel's menu to the toplevel
  #
  .browser.menubar add cascade \
    -menu .browser.menubar.file \
    -label {File} \
    -underline 0
  .browser.menubar add cascade \
    -menu .browser.menubar.edit \
    -label {Edit} \
    -underline 0
  .browser.menubar add cascade \
    -menu .browser.menubar.display \
    -label {Display} \
    -underline 0
  .browser.menubar add cascade \
    -menu .browser.menubar.find \
    -label {Find} \
    -underline 1
  .browser.menubar add cascade \
    -menu .browser.menubar.options \
    -label {Options} \
    -underline 0
  .browser.menubar add cascade \
    -menu .browser.menubar.view \
    -label {View} \
    -underline 0
  .browser.menubar add cascade \
    -menu .browser.menubar.export \
    -label {Export} \
    -underline 1
  .browser.menubar add cascade \
    -menu .browser.menubar.help \
    -label {Help} \
    -underline 0
  .browser configure \
    -menu .browser.menubar


  #
  #  Add the main frame to the browser toplevel
  #
  pack append .browser \
    .browser.main_frm {top frame center expand fill} \
    .browser.local_frm {top frame center expand fill}

  EndSrc.browser

  if {"[info procs XFEdit]" != ""} {
    catch "XFMiscBindWidgetTree .browser"
    after 2 "catch {XFEditSetShowWindows}"
  }
}

# proc DestroyWindow.browser {}
proc DestroyWindow.browser {} {# xf ignore me 7
  if {"[info procs XFEdit]" != ""} {
    if {"[info commands .browser]" != ""} {
      global xfShowWindow.browser
      set xfShowWindow.browser 0
      XFEditSetPath .
      after 2 "XFSaveAsProc .browser; XFEditSetShowWindows"
    }
  } {
    catch "destroy .browser"
    update
  }
}

# proc StartupSrc.browser {args}
proc StartupSrc.browser {args} {
 # browser startup entrance
  global ascBrowVect ascGlobalVect env
  if {[catch {set ascBrowVect(windowname)} ]} {
    set ascBrowVect(minsize) 100x100
    set ascBrowVect(geometry) 200x400+10+10
    set ascBrowVect(iconname) Browser
    set ascBrowVect(initialstate) normal
    set ascBrowVect(font) "-*-*"
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

# proc EndSrc.browser {}
proc EndSrc.browser {} {
 # browser startup exit
  global ascBrowVect
  set ascBrowVect(geometry) [sanegeometry $ascBrowVect(geometry)]
  set minw [lindex [split $ascBrowVect(minsize) x] 0]
  set minh [lindex [split $ascBrowVect(minsize) x] 1]
  set gw [lindex [split [lindex [split $ascBrowVect(geometry) +] 0] x] 0]
  set gh [lindex [split [lindex [split $ascBrowVect(geometry) +] 0] x] 1]
  set gp "+[lindex [split [split $ascBrowVect(geometry) x] +] 1]+[lindex [split [split $ascBrowVect(geometry) x] +] 2]"
  if {[expr $gw < $minw]} {set gw $minw}
  if {[expr $gh < $minh]} {set gh $minh}
  set gwh "${gw}x${gh}"
  wm minsize .browser $minw $minh
  wm geometry .browser [osgpos $gwh$gp]
  wm iconname .browser $ascBrowVect(iconname)

  if {$ascBrowVect(initialstate)!="iconic" && \
      $ascBrowVect(initialstate)!="iconified" &&
      $ascBrowVect(initialstate)!="withdrawn"} {
    wm deiconify .browser
  }
  if {"$ascBrowVect(initialstate)"=="withdrawn"} {
    wm withdraw .browser
  }
}


# Internal procedures

# eof
#
