#  units.tcl
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.21 $
#  Last modified on: $Date: 1998/06/18 15:55:45 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: units.tcl,v $
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

# Module: units.tcl
# Tcl version: 7.1 (Tcl/Tk/XF)
# Tk version: 3.4
# XF version: 2.2
#

# module contents
global moduleList
global autoLoadList
set moduleList(units.tcl) { .units}
set autoLoadList(units.tcl) {0}

# procedures to show toplevel windows


# procedure to show window ShowWindow.units
# proc ShowWindow.units { args}
proc ShowWindow.units { args} {
# xf ignore me 7

  global env ascUnitVect ascGlobalVect

  StartupSrc.units

  # build widget .units
  if {"[info procs XFEdit]" != ""} {
    catch "XFDestroy .units"
  } {
    catch "destroy .units"
  }
  toplevel .units

  # Ascend patched window manager configurations
  wm iconify .units
  wm positionfrom .units user
  wm sizefrom .units user
  wm iconname .units {Units}
  wm minsize .units 100 100
  wm title .units {A4 Units}
  wm protocol .units WM_DELETE_WINDOW {Toggle_Remote ascUnitVect}


  # build widget .units.entry_frm
  frame .units.entry_frm \
    -borderwidth 2 \
    -relief raised

  # build widget .units.entry_frm.scrollbar2
  scrollbar .units.entry_frm.scrollbar2 \
    -command {.units.entry_frm.listbox1 yview}

  # build widget .units.entry_frm.listbox1
  listbox .units.entry_frm.listbox1 \
    -relief {raised} \
    -width 20 \
    -height 11 \
    -exportselection 0 \
    -yscrollcommand {.units.entry_frm.scrollbar2 set} \
    -font $ascUnitVect(font)

  # pack widget .units.main_frm.atom_frm
  pack append .units.entry_frm \
    .units.entry_frm.scrollbar2 {left frame center filly} \
    .units.entry_frm.listbox1 {top frame center expand fill}

  # build widget .units.entry_setunits
  frame .units.entry_setunits \
    -relief {raised}

  # build widget .units.entry_setunits.entry5
  entry .units.entry_setunits.entry5 \
    -exportselection 0 \
    -font $ascUnitVect(font) \
    -relief {raised} \
    -textvariable {ascUnitVect(atomdispunits)}

  # build widget .units.entry_setunits.label4
  label .units.entry_setunits.label4 \
    -font $ascUnitVect(font) \
    -height {1} \
    -padx {0} \
    -relief {raised} \
    -text {Set units:}

  # pack widget .units.entry_setunits
  pack append .units.entry_setunits \
    .units.entry_setunits.label4 {left frame center} \
    .units.entry_setunits.entry5 {top frame center expand fill}


  # build widget .units.main_frm
  frame .units.main_frm \
    -borderwidth {0} \
    -relief {raised}

  # build widget .units.main_frm.atom_frm
  frame .units.main_frm.atom_frm \
    -width 5

  # build widget .units.main_frm.atom_frm.scrollbar2
  scrollbar .units.main_frm.atom_frm.scrollbar2 \
    -command {.units.main_frm.atom_frm.listbox1 yview}

  # build widget .units.main_frm.atom_frm.listbox1
  listbox .units.main_frm.atom_frm.listbox1 \
    -relief {raised} \
    -width 20 \
    -height 10 \
    -exportselection 0 \
    -yscrollcommand {.units.main_frm.atom_frm.scrollbar2 set} \
    -font $ascUnitVect(font)


  # pack widget .units.main_frm.atom_frm
  pack append .units.main_frm.atom_frm \
    .units.main_frm.atom_frm.scrollbar2 {left frame center filly} \
    .units.main_frm.atom_frm.listbox1 {top frame center expand fill}

  # build widget .units.main_frm.units_box
  frame .units.main_frm.units_box \
    -relief {raised} \
    -width 20

  # build widget .units.main_frm.units_box.scrollbar2
  scrollbar .units.main_frm.units_box.scrollbar2 \
    -command {.units.main_frm.units_box.listbox1 yview}

  # build widget .units.main_frm.units_box.listbox1
  listbox .units.main_frm.units_box.listbox1 \
    -relief {raised} \
    -width 10 \
    -height 10 \
    -exportselection 0 \
    -yscrollcommand {.units.main_frm.units_box.scrollbar2 set} \
    -font $ascUnitVect(font)

  # pack widget .units.main_frm.units_box
  pack append .units.main_frm.units_box \
    .units.main_frm.units_box.scrollbar2 {right frame center filly} \
    .units.main_frm.units_box.listbox1 {top frame center expand fill}

  # pack widget .units.main_frm
  pack append .units.main_frm \
    .units.main_frm.atom_frm {left frame center expand fill} \
    .units.main_frm.units_box {left frame center expand fill}

  #
  # Build the menu bar and submenus
  #
  menu .units.menubar \
    -tearoffcommand .UNITS.MENUBAR \
    -tearoff 0

  # The Display menu
  menu .units.menubar.disp \
    -tearoffcommand .UNITS.MENUBAR.DISP \
    -tearoff 0
  .units.menubar.disp add command \
    -label {Show all units} \
    -underline 0 \
    -accelerator {Alt-d s} \
    -command Units_do_ShowAll

  # The File menu
  menu .units.menubar.file \
    -tearoffcommand .UNITS.MENUBAR.FILE \
    -tearoff 0
  .units.menubar.file add command \
    -command {Units_do_ReadFile} \
    -accelerator {Alt-f r} \
    -underline 0 \
    -label {Read file ...}
  .units.menubar.file add command \
    -command {Units_do_WriteFile} \
    -underline 0 \
    -accelerator {Alt-f s} \
    -label {Save file ...}
  .units.menubar.file add separator
  .units.menubar.file add command \
    -command {Toggle_Remote ascUnitVect} \
    -accelerator {Alt-f c} \
    -label {Close window} \
    -underline 0
  .units.menubar.file add command \
    -command {Script_do_Exit} \
    -label {Exit ASCEND...} \
    -accelerator {Alt-f e} \
    -underline 0

  # The Edit menu
  menu .units.menubar.edit \
    -tearoffcommand .UNITS.MENUBAR.EDIT \
    -tearoff 0
  .units.menubar.edit add command \
    -command {Units_do_Precision} \
    -label {Set precision} \
    -accelerator {Alt-e p} \
    -underline 4
  .units.menubar.edit add cascade \
    -label {Set basic units} \
    -menu .units.menubar.edit.basic

  # basic units cascade
  menu .units.menubar.edit.basic \
    -tearoffcommand .UNITS.MENUBAR.EDIT.BASIC \
    -tearoff 0

  .units.menubar.edit.basic add cascade \
    -label {Mass (M)} \
    -menu {.units.menubar.edit.basic.m0}
  .units.menubar.edit.basic add cascade \
    -label {Quantity (Q)} \
    -menu {.units.menubar.edit.basic.m1}
  .units.menubar.edit.basic add cascade \
    -label {Length (L)} \
    -menu {.units.menubar.edit.basic.m2}
  .units.menubar.edit.basic add cascade \
    -label {Time (T)} \
    -menu {.units.menubar.edit.basic.m3}
  .units.menubar.edit.basic add cascade \
    -label {Temperature (TMP)} \
    -menu {.units.menubar.edit.basic.m4}
  .units.menubar.edit.basic add cascade \
    -label {Money (C)} \
    -menu {.units.menubar.edit.basic.m5}
  .units.menubar.edit.basic add cascade \
    -label {Current (E)} \
    -menu {.units.menubar.edit.basic.m6}
  .units.menubar.edit.basic add cascade \
    -label {Light (LUM)} \
    -menu {.units.menubar.edit.basic.m7}
  .units.menubar.edit.basic add cascade \
    -label {Pln. Angle (P)} \
    -menu {.units.menubar.edit.basic.m8}
  .units.menubar.edit.basic add cascade \
    -label {Sol. Angle (S)} \
    -menu {.units.menubar.edit.basic.m9}

  # build widget .units.menubar.edit.basic.m0
  menu .units.menubar.edit.basic.m0 \
    -tearoffcommand .UNITS.MENUBAR.DISP.M0 \
    -tearoff 0
  .units.menubar.edit.basic.m0 add command \
    -label {kilogram}

  # build widget .units.menubar.edit.basic.m1
  menu .units.menubar.edit.basic.m1 \
    -tearoffcommand .UNITS.MENUBAR.DISP.M1 \
    -tearoff 0
  .units.menubar.edit.basic.m1 add command \
    -label {mole}

  # build widget .units.menubar.edit.basic.m2
  menu .units.menubar.edit.basic.m2 \
    -tearoffcommand .UNITS.MENUBAR.DISP.M2 \
    -tearoff 0
  .units.menubar.edit.basic.m2 add command \
    -label {meter}

  # build widget .units.menubar.edit.basic.m3
  menu .units.menubar.edit.basic.m3 \
    -tearoffcommand .UNITS.MENUBAR.DISP.M3 \
    -tearoff 0
  .units.menubar.edit.basic.m3 add command \
    -label {second}

  # build widget .units.menubar.edit.basic.m4
  menu .units.menubar.edit.basic.m4 \
    -tearoffcommand .UNITS.MENUBAR.DISP.M4 \
    -tearoff 0
  .units.menubar.edit.basic.m4 add command \
    -label {Kelvin}

  # build widget .units.menubar.edit.basic.m5
  menu .units.menubar.edit.basic.m5 \
    -tearoffcommand .UNITS.MENUBAR.DISP.M5 \
    -tearoff 0
  .units.menubar.edit.basic.m5 add command \
    -label {US}

  # build widget .units.menubar.edit.basic.m6
  menu .units.menubar.edit.basic.m6 \
    -tearoffcommand .UNITS.MENUBAR.DISP.M6 \
    -tearoff 0
  .units.menubar.edit.basic.m6 add command \
    -label {ampere}

  # build widget .units.menubar.edit.basic.m7
  menu .units.menubar.edit.basic.m7 \
    -tearoffcommand .UNITS.MENUBAR.DISP.M7 \
    -tearoff 0
  .units.menubar.edit.basic.m7 add command \
    -label {candela}

  # build widget .units.menubar.edit.basic.m8
  menu .units.menubar.edit.basic.m8 \
    -tearoffcommand .UNITS.MENUBAR.DISP.M8 \
    -tearoff 0
  .units.menubar.edit.basic.m8 add command \
    -label {radian}

  # build widget .units.menubar.edit.basic.m9
  menu .units.menubar.edit.basic.m9 \
    -tearoffcommand .UNITS.MENUBAR.DISP.M9 \
    -tearoff 0
  .units.menubar.edit.basic.m9 add command \
    -label {steradian}


  # The View menu
  menu .units.menubar.view \
    -tearoffcommand .UNITS.MENUBAR.VIEW \
    -tearoff 0
  .units.menubar.view add command \
    -label {SI (MKS) set} \
    -underline 1 \
    -accelerator {Alt-v i} \
    -command Units_do_SI
  .units.menubar.view add command \
    -label {US Engineering set} \
    -underline 0 \
    -accelerator {Alt-v u} \
    -command Units_do_AE
  .units.menubar.view add command \
    -label {CGS set} \
    -underline 0 \
    -accelerator {Alt-v c} \
    -command Units_do_CGS
  .units.menubar.view add separator
  .units.menubar.view add command  \
    -command {Units_do_Font}  \
    -accelerator {Alt-v f} \
    -underline 0 \
    -label {Font}
  .units.menubar.view add checkbutton \
    -offvalue {0} \
    -onvalue {1} \
    -variable {ascUnitVect(visibility)} \
    -accelerator {Alt-v o} \
    -underline 0 \
    -label {Open automatically}
 .units.menubar.view add command  \
    -command {View_Set_Save_Options units}  \
    -underline 0 \
    -accelerator {Alt-v s} \
    -label {Save window appearance}

  # The Help menu
  menu .units.menubar.help \
    -tearoffcommand .UNITS.MENUBAR.HELP \
    -tearoff 0
  .units.menubar.help add command \
    -command {Units_do_Help} \
    -label {On Units} \
    -underline 3


  #
  # Add the menus as cascades of the toplevel's menu;
  # add the toplevel's menu to the toplevel
  #
  .units.menubar add cascade \
    -menu .units.menubar.file \
    -label {File} \
    -underline 0
  .units.menubar add cascade \
    -menu .units.menubar.edit \
    -label {Edit} \
    -underline 0
  .units.menubar add cascade \
    -menu .units.menubar.disp \
    -label {Display} \
    -underline 0
  .units.menubar add cascade \
    -menu .units.menubar.view \
    -label {View} \
    -underline 0
  .units.menubar add cascade \
    -menu .units.menubar.help \
    -label {Help} \
    -underline 0
  .units configure \
    -menu .units.menubar


  #
  #  Pack the parts into the .units toplevel
  #
  pack append .units \
   .units.main_frm {top frame center expand fill} \
   .units.entry_setunits {top frame center fillx} \
   .units.entry_frm {top frame center fillx}

  EndSrc.units

  if {"[info procs XFEdit]" != ""} {
    catch "XFMiscBindWidgetTree .units"
    after 2 "catch {XFEditSetShowWindows}"
  }
}

# proc DestroyWindow.units {}
proc DestroyWindow.units {} {# xf ignore me 7
  if {"[info procs XFEdit]" != ""} {
    if {"[info commands .units]" != ""} {
      global xfShowWindow.units
      set xfShowWindow.units 0
      XFEditSetPath .
      after 2 "XFSaveAsProc .units; XFEditSetShowWindows"
    }
  } {
    catch "destroy .units"
    update
  }
}

proc StartupSrc.units {args} {
# proc StartupSrc.units {args}
# units startup entrance
# as much of the C/global dependencies managed here as possible
  global ascUnitVect ascGlobalVect env
  if {[catch {set ascUnitVect(windowname)} ]} {
    set ascUnitVect(minsize) 200x200
    set ascUnitVect(geometry) 290x420+10+10
    set ascUnitVect(iconname) Units
    set ascUnitVect(initialstate) normal
    set ascUnitVect(font) "-*-*"
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

# proc EndSrc.units {}
proc EndSrc.units {} {
# units startup exit
  global ascUnitVect
  global env ascUnitVect ascGlobalVect

  set ascUnitVect(geometry) [sanegeometry $ascUnitVect(geometry)]
  set minw [lindex [split $ascUnitVect(minsize) x] 0]
  set minh [lindex [split $ascUnitVect(minsize) x] 1]
  set gw [lindex [split [lindex [split $ascUnitVect(geometry) +] 0] x] 0]
  set gh [lindex [split [lindex [split $ascUnitVect(geometry) +] 0] x] 1]
  set gp "+[lindex [split [split $ascUnitVect(geometry) x] +] 1]+[lindex [split [split $ascUnitVect(geometry) x] +] 2]"
  if {[expr $gw < $minw]} {set gw $minw}
  if {[expr $gh < $minh]} {set gh $minh}
  set gwh "${gw}x${gh}"
  wm minsize .units $minw $minh
  wm geometry .units [osgpos $gwh$gp]
  wm iconname .units $ascUnitVect(iconname)
  if {$ascUnitVect(initialstate)!="iconic" && \
      $ascUnitVect(initialstate)!="iconified" && \
      $ascUnitVect(initialstate)!="withdrawn"} {
    wm deiconify .units
  }
  if {"$ascUnitVect(initialstate)"=="withdrawn"} {
    wm withdraw .units
  }
}



# Internal procedures

# eof
#

