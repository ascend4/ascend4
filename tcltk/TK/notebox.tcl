#  notebox.tcl
#  by Benjamin Andrew Allan
#  Created May 3, 1998.
#  Part of ASCEND
#  Revision: $Revision: 1.4 $
#  Last modified on: $Date: 1998/06/18 15:55:34 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: notebox.tcl,v $
#
#  This file is part of the ASCEND Tcl/Tk Interface.
#
#  Copyright (C) 1998 Carnegie Mellon University
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

# Module: notedata.tcl
# Tcl version: 8.0 (Tcl/Tk/XF)
# Tk version: 8.0
# XF version: 4.0
#

# module contents
global moduleList
global autoLoadList
set moduleList(notedata.tcl) { .notebox}
set autoLoadList(notedata.tcl) {0}

# procedures to show toplevel windows

# procedure to show window .notebox
proc ShowWindow.notebox {args} {# xf ignore me 7

  catch "destroy .notebox"
  toplevel .notebox 
  global ascNotesVect ascMLVect

  # Window manager configurations
  wm positionfrom .notebox user
  wm sizefrom .notebox ""
  wm maxsize .notebox 2000 2000
  wm minsize .notebox 10 10
  wm protocol .notebox WM_DELETE_WINDOW {wm withdraw .notebox}
  wm title .notebox {A4 NOTES}


  # build widget .notebox.frame1
  frame .notebox.frame1 \
    -borderwidth {0} \
    -height {30} \
    -width {30}

  # build widget .notebox.frame1.label2
  label .notebox.frame1.label2 \
    -borderwidth {0} \
    -anchor w \
    -justify {left} \
    -text {NOTES}

  # build widget .notebox.frame1.db
  frame .notebox.frame1.db \
    -borderwidth {2}

  # build widget .notebox.frame1.db.value
  entry .notebox.frame1.db.value \
    -state disabled \
    -textvariable ascNotesVect(current) \
    -width {8}

  set ascNotesVect(dbbutton) .notebox.frame1.db.menubutton2
  # build widget .notebox.frame1.db.menubutton2
  menubutton .notebox.frame1.db.menubutton2 \
    -menu {.notebox.frame1.db.menubutton2.m} \
    -relief raised \
    -padx {5} \
    -pady {4} \
    -text {from:}

  set ascNotesVect(dbmenu) .notebox.frame1.db.menubutton2.m
  # build widget .notebox.frame1.db.menubutton2.m
  menu .notebox.frame1.db.menubutton2.m \
    -tearoff 0
  # must add menu entries in Procs from current listing.

  # build widget .notebox.frame1.label4
  label .notebox.frame1.label4 \
    -borderwidth {0} \
    -anchor w \
    -justify {left} \
    -text {matching text}

  # build widget .notebox.frame1.matchentry
  entry .notebox.frame1.matchentry \
    -textvariable ascNotesVect(stringmatch) \
    -exportselection 0 \
    -width {20}


  # build widget .notebox.frame1.button3
  button .notebox.frame1.button3 \
    -padx {11} \
    -pady {4} \
    -command {wm withdraw .notebox} \
    -text {Close}

  # build widget .notebox.main_frm
  frame .notebox.main_frm \
    -borderwidth {0} \
    -height {30} \
    -width {30}

  # build widget .notebox.main_frm.list_frm
  frame .notebox.main_frm.list_frm

  catch {unset ascMLVect(.notebox.main_frm.list_frm)}
  set ascNotesVect(typelist) \
    [ascLabeledMultiList .notebox.main_frm.list_frm Type 12]
  set ascNotesVect(langlist) \
    [ascLabeledMultiList .notebox.main_frm.list_frm Language 6]
  set ascNotesVect(namelist) \
    [ascLabeledMultiList .notebox.main_frm.list_frm Name 10]
  set ascNotesVect(methlist) \
    [ascLabeledMultiList .notebox.main_frm.list_frm Method 12]
  set ascNotesVect(textlist) \
    [ascLabeledMultiList .notebox.main_frm.list_frm Text 24]
  set ascNotesVect(fililist) \
    [ascLabeledMultiList .notebox.main_frm.list_frm File:Line 12]

  $ascNotesVect(typelist) configure -font $ascNotesVect(font)
  $ascNotesVect(langlist) configure -font $ascNotesVect(font)
  $ascNotesVect(namelist) configure -font $ascNotesVect(font)
  $ascNotesVect(methlist) configure -font $ascNotesVect(font)
  $ascNotesVect(textlist) configure -font $ascNotesVect(font)
  $ascNotesVect(fililist) configure -font $ascNotesVect(font)

  # build widget .notebox.main_frm.text_frm
  frame .notebox.main_frm.text_frm \
    -relief flat

  # build widget .notebox.main_frm.text_frm.scrollbar1
  scrollbar .notebox.main_frm.text_frm.scrollbar1 \
    -command {.notebox.main_frm.text_frm.text2 yview}

  set ascNotesVect(textbox) .notebox.main_frm.text_frm.text2
  # build widget .notebox.main_frm.text_frm.text2
  text .notebox.main_frm.text_frm.text2 \
    -height {10} \
    -exportselection 0 \
    -font $ascNotesVect(font) \
    -width {9} \
    -wrap {none} \
    -yscrollcommand {.notebox.main_frm.text_frm.scrollbar1 set}

  # build widget .notebox.bottom_frm
  frame .notebox.bottom_frm \
    -borderwidth {0} \
    -height {30} \
    -width {30}

  set ascNotesVect(entrybox) .notebox.bottom_frm.entry
  # build widget .notebox.bottom_frm.entry
  entry .notebox.bottom_frm.entry \
    -textvariable ascNotesVect(entrytext) \
    -exportselection 0 \
    -relief flat \
    -borderwidth 0 \
    -state disabled

  # build widget .notebox.bottom_frm.sort1
  frame .notebox.bottom_frm.sort1 \
    -borderwidth {2}

  # build widget .notebox.bottom_frm.sort1.value
  entry .notebox.bottom_frm.sort1.value \
    -exportselection 0 \
    -state disabled \
    -textvariable ascNotesVect(sort1) \
    -width {8}

  # build widget .notebox.bottom_frm.sort1.menubutton2
  menubutton .notebox.bottom_frm.sort1.menubutton2 \
    -menu {.notebox.bottom_frm.sort1.menubutton2.m} \
    -relief raised \
    -padx {5} \
    -pady {4} \
    -text {Sort 1}

  # build widget .notebox.bottom_frm.sort1.menubutton2.m
  menu .notebox.bottom_frm.sort1.menubutton2.m \
    -tearoff 0
  .notebox.bottom_frm.sort1.menubutton2.m add radiobutton \
    -variable ascNotesVect(sort1) \
    -value None \
    -label None \
    -state active
  .notebox.bottom_frm.sort1.menubutton2.m add radiobutton \
    -variable ascNotesVect(sort1) \
    -value Type \
    -label Type \
    -state active
  .notebox.bottom_frm.sort1.menubutton2.m add radiobutton \
    -variable ascNotesVect(sort1) \
    -value Language \
    -label Language 
  .notebox.bottom_frm.sort1.menubutton2.m add radiobutton \
    -variable ascNotesVect(sort1) \
    -value Name \
    -label Name 
  .notebox.bottom_frm.sort1.menubutton2.m add radiobutton \
    -variable ascNotesVect(sort1) \
    -value Method \
    -label Method 
  .notebox.bottom_frm.sort1.menubutton2.m add radiobutton \
    -variable ascNotesVect(sort1) \
    -value File \
    -label File 

  # build widget .notebox.bottom_frm.sort2
  frame .notebox.bottom_frm.sort2 \
    -borderwidth {2}

  # build widget .notebox.bottom_frm.sort2.value
  entry .notebox.bottom_frm.sort2.value \
    -state disabled \
    -textvariable ascNotesVect(sort2) \
    -width {8}

  # build widget .notebox.bottom_frm.sort2.menubutton2
  menubutton .notebox.bottom_frm.sort2.menubutton2 \
    -menu {.notebox.bottom_frm.sort2.menubutton2.m} \
    -relief raised \
    -padx {5} \
    -pady {4} \
    -text {Sort 2}

  # build widget .notebox.bottom_frm.sort2.menubutton2.m
  menu .notebox.bottom_frm.sort2.menubutton2.m \
    -tearoff 0
  .notebox.bottom_frm.sort2.menubutton2.m add radiobutton \
    -variable ascNotesVect(sort2) \
    -value None \
    -label None \
    -state active
  .notebox.bottom_frm.sort2.menubutton2.m add radiobutton \
    -variable ascNotesVect(sort2) \
    -value Type \
    -label Type
  .notebox.bottom_frm.sort2.menubutton2.m add radiobutton \
    -variable ascNotesVect(sort2) \
    -value Language \
    -label Language 
  .notebox.bottom_frm.sort2.menubutton2.m add radiobutton \
    -variable ascNotesVect(sort2) \
    -value Name \
    -label Name 
  .notebox.bottom_frm.sort2.menubutton2.m add radiobutton \
    -variable ascNotesVect(sort2) \
    -value Method \
    -label Method 
  .notebox.bottom_frm.sort2.menubutton2.m add radiobutton \
    -variable ascNotesVect(sort2) \
    -value File \
    -label File 

  # pack master .notebox.frame1
  pack configure .notebox.frame1.label2 \
    -side left
  pack configure .notebox.frame1.db \
    -side left
  pack configure .notebox.frame1.label4 \
    -side left
  pack configure .notebox.frame1.matchentry \
    -expand 1 \
    -fill x \
    -side left
  pack configure .notebox.frame1.button3 \
    -side right
  # pack master .notebox.frame1.db
  pack configure .notebox.frame1.db.menubutton2 \
    -fill both \
    -side left
  pack configure .notebox.frame1.db.value \
    -expand 1 \
    -fill both \
    -side right


  # pack master .notebox.main_frm
  pack configure .notebox.main_frm.list_frm \
    -fill x
  pack configure .notebox.main_frm.text_frm \
    -expand 1 \
    -fill both

  # pack master .notebox.main_frm.text_frm
  pack configure .notebox.main_frm.text_frm.scrollbar1 \
    -fill y \
    -side right
  pack configure .notebox.main_frm.text_frm.text2 \
    -expand 1 \
    -fill both

  # pack master .notebox.bottom_frm
  pack configure .notebox.bottom_frm.sort1 \
    -side left
  pack configure .notebox.bottom_frm.sort2 \
    -side left
  pack configure .notebox.bottom_frm.entry \
    -expand 1 \
    -fill x \
    -side left

  # pack master .notebox.bottom_frm.sort1
  pack configure .notebox.bottom_frm.sort1.menubutton2 \
    -fill both \
    -side left
  pack configure .notebox.bottom_frm.sort1.value \
    -expand 1 \
    -fill both \
    -side right

  # pack master .notebox.bottom_frm.sort2
  pack configure .notebox.bottom_frm.sort2.menubutton2 \
    -fill both \
    -side left
  pack configure .notebox.bottom_frm.sort2.value \
    -expand 1 \
    -fill both \
    -side right

  # pack master .notebox
  pack configure .notebox.frame1 \
    -fill x
  pack configure .notebox.main_frm \
    -expand 1 \
    -fill both
  pack configure .notebox.bottom_frm \
    -fill x \
    -side bottom

  .notebox.main_frm.text_frm.text2 insert end {safsf}
  .notebox.bottom_frm.sort1.value insert end {Value 3}

}

proc DestroyWindow.notebox {} {# xf ignore me 7
  catch "destroy .notebox"
  update
}


# User defined procedures

# User defined images

# Internal procedures

# eof
#

