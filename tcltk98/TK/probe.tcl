#  probe.tcl
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.27 $
#  Last modified on: $Date: 1998/06/18 15:55:37 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: probe.tcl,v $
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

# Module: probe.tcl
# Tcl version: 7.1 (Tcl/Tk/XF)
# Tk version: 3.4
# XF version: 2.2
#

# module contents
global moduleList
global autoLoadList
set moduleList(probe.tcl) { .probe}
set autoLoadList(probe.tcl) {0}

# procedures to show toplevel windows


# procedure to show window ShowWindow.probe
# proc ShowWindow.probe { args}
proc ShowWindow.probe {args} {
# xf ignore me 7

  global env
  global ascGlobalVect
  global ascProbVect

  StartupSrc.probe

  # build widget .probe
  if {"[info procs XFEdit]" != ""} {
    catch "XFDestroy .probe"
  } {
    catch "destroy .probe"
  }
  toplevel .probe

  # Ascend patched window manager configurations
  wm iconify .probe
  wm positionfrom .probe user
  wm sizefrom .probe user
  wm iconname .probe {Probe}
  wm minsize .probe 100 100
  wm title .probe {A4 Probe}
  wm protocol .probe WM_DELETE_WINDOW {Toggle_Remote ascProbVect}


  # build widget .probe.main_frm
  frame .probe.main_frm \
    -borderwidth 0 \
    -relief {raised}

  build_probebox .probe.main_frm 0 $ascGlobalVect(font)

  # pack widget .probe.main_frm
  pack append .probe.main_frm \
    .probe.main_frm.probe_box_0 {top frame center expand fill}


  #
  # Build the menu bar and submenus
  #
  menu .probe.menubar \
    -tearoffcommand .PROBE.MENUBAR \
    -tearoff 0

  # The Edit menu
  menu .probe.menubar.edit \
    -tearoffcommand .PROBE.MENUBAR.EDIT \
    -tearoff 0
  .probe.menubar.edit add command \
    -command {Probe_do_SelectAll} \
    -label {Highlight all} \
    -accelerator {Alt-e h} \
    -underline 0
  .probe.menubar.edit add command \
    -command {Probe_do_RemoveSelections} \
    -label {Remove selected names} \
    -accelerator {Alt-e s} \
    -underline 7
  .probe.menubar.edit add command \
    -command {Probe_do_RemoveAll} \
    -label {Remove all names} \
    -accelerator {Alt-e a} \
    -underline 7
  .probe.menubar.edit add command \
    -command {Probe_do_RemoveUncertain} \
    -label {Remove UNCERTAIN names} \
    -accelerator {Alt-e u} \
    -underline 7
  .probe.menubar.edit add command \
    -command {Probe_do_Copy} \
    -accelerator {Alt-e c} \
    -label {Copy} \
    -underline 0


  # The Export menu
  menu .probe.menubar.export \
    -tearoffcommand .PROBE.MENUBAR.EXPORT \
    -tearoff 0
  .probe.menubar.export add command \
    -command {Probe_do_Export2Browser}  \
    -accelerator {Alt-x b} \
    -label {to Browser} \
    -underline 3
  .probe.menubar.export add command \
    -command {Probe_do_Export2Display} \
    -accelerator {Alt-x d} \
    -label {to Display} \
    -underline 3


  # The File menu
  menu .probe.menubar.file \
    -tearoffcommand .PROBE.MENUBAR.FILE \
    -tearoff 0
  .probe.menubar.file add command \
    -command {Probe_do_NewBuffer} \
    -label {New buffer...} \
    -accelerator {Alt-f n} \
    -underline 0
  .probe.menubar.file add command \
    -command {Probe_do_ReadFile} \
    -label {Read...} \
    -accelerator {Alt-f r} \
    -underline 0
  .probe.menubar.file add command \
    -command {Probe_do_WriteBuf} \
    -label {Save...} \
    -accelerator {Alt-f s} \
    -underline 0
  .probe.menubar.file add command \
    -command {Probe_do_WriteBufAs} \
    -label {Save As...} \
    -accelerator {Alt-f a} \
    -underline 5
  .probe.menubar.file add command \
    -command {Probe_do_Print} \
    -label {Print...} \
    -accelerator {Alt-f p} \
    -underline 0
  .probe.menubar.file add command \
    -command {Toggle_Remote ascProbVect} \
    -accelerator {Alt-f c} \
    -label {Close window} \
    -underline 0
  .probe.menubar.file add command \
    -command {Script_do_Exit} \
    -label {Exit ASCEND...} \
    -accelerator {Alt-f e} \
    -underline 0
  .probe.menubar.file add separator


  # The Help menu
  menu .probe.menubar.help \
    -tearoffcommand .PROBE.MENUBAR.HELP \
    -tearoff 0
  .probe.menubar.help add command \
    -command {Probe_do_Help} \
    -label {On Probe} \
    -underline 3


  # The View menu
  menu .probe.menubar.view \
    -tearoffcommand .PROBE.MENUBAR.VIEW \
    -tearoff 0
  .probe.menubar.view add command  \
    -command {Probe_do_Font}  \
    -label {Font ...} \
    -accelerator {Alt-v f} \
    -underline 0
  .probe.menubar.view add checkbutton \
    -offvalue {0} \
    -onvalue {1} \
    -variable {ascProbVect(visibility)} \
    -accelerator {Alt-v o} \
    -label {Open automatically} \
    -underline 0
 .probe.menubar.view add command  \
    -command {View_Set_Save_Options probe}  \
    -label {Save window appearance} \
    -accelerator {Alt-v s} \
    -underline 0


  #
  # Add the menus as cascades of the toplevel's menu;
  # add the toplevel's menu to the toplevel
  #
  .probe.menubar add cascade \
    -menu .probe.menubar.file \
    -label {File} \
    -underline 0
  .probe.menubar add cascade \
    -menu .probe.menubar.edit \
    -label {Edit} \
    -underline 0
  .probe.menubar add cascade \
    -menu .probe.menubar.view \
    -label {View} \
    -underline 0
  .probe.menubar add cascade \
    -menu .probe.menubar.export \
    -label {Export} \
    -underline 1
  .probe.menubar add cascade \
    -menu .probe.menubar.help \
    -label {Help} \
    -underline 0
  .probe configure \
    -menu .probe.menubar


  # build frame and entry at bottom of window to hold current buffer name
  frame .probe.buffer_frm \
    -borderwidth {2} \
    -relief {raised}

  entry .probe.buffer_frm.buffer_entry \
    -relief {raised} \
    -width {24} \
    -state {disabled} \
    -exportselection 0 \
    -textvariable ascProbVect(filename) \
    -font $ascProbVect(font)

  pack append .probe.buffer_frm \
    .probe.buffer_frm.buffer_entry {left frame center expand fillx}


  #
  #  Pack the widgets into the probe toplevel
  #
  pack append .probe \
    .probe.main_frm {top frame center expand fill} \
    .probe.buffer_frm {top frame center fillx}

  EndSrc.probe

  if {"[info procs XFEdit]" != ""} {
    catch "XFMiscBindWidgetTree .probe"
    after 2 "catch {XFEditSetShowWindows}"
  }
}

# proc DestroyWindow.probe {}
proc DestroyWindow.probe {} {# xf ignore me 7
  if {"[info procs XFEdit]" != ""} {
    if {"[info commands .probe]" != ""} {
      global xfShowWindow.probe
      set xfShowWindow.probe 0
      XFEditSetPath .
      after 2 "XFSaveAsProc .probe; XFEditSetShowWindows"
    }
  } {
    catch "destroy .probe"
    update
  }
}

# proc StartupSrc.probe {args}
# probe startup entrance
# as much of the C/global dependencies managed here as possible
proc StartupSrc.probe {args} {

  global env
  global ascGlobalVect
  global ascProbVect
 
  if {[info procs Probe_bindListbox] != "Probe_bindListbox"} {
    proc Probe_bindListbox {args} {
      error "Driver needs to define Probe_bindListbox!"
    }
  }

  if {[catch {set ascProbVect(windowname)} ]} {
    set ascProbVect(minsize) 100x100
    set ascProbVect(geometry) 200x400+10+10
    set ascProbVect(iconname) Probe
    set ascProbVect(initialstate) normal
    set ascProbVect(font) "courier 12 bold"
  }
  if {[catch {set ascGlobalVect(font)} ]} {
    set ascGlobalVect(font) "courier 10 bold"
    set ascGlobalVect(labelfont) "courier 12 bold"
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

# proc EndSrc.probe {}
proc EndSrc.probe {} {
# probe startup exit
global ascProbVect
  set ascProbVect(geometry) [sanegeometry $ascProbVect(geometry)]
  set minw [lindex [split $ascProbVect(minsize) x] 0]
  set minh [lindex [split $ascProbVect(minsize) x] 1]
  set gw [lindex [split [lindex [split $ascProbVect(geometry) +] 0] x] 0]
  set gh [lindex [split [lindex [split $ascProbVect(geometry) +] 0] x] 1]
  set gp "+[lindex [split [split $ascProbVect(geometry) x] +] 1]+[lindex [split [split $ascProbVect(geometry) x] +] 2]"
  if {[expr $gw < $minw]} {set gw $minw}
  if {[expr $gh < $minh]} {set gh $minh}
  set gwh "${gw}x${gh}"
  wm positionfrom .probe user
  wm sizefrom .probe user
  wm minsize .probe $minw $minh
  wm geometry .probe [osgpos $gwh$gp]
  wm iconname .probe $ascProbVect(iconname)

  if {$ascProbVect(initialstate)!="iconic" && \
      $ascProbVect(initialstate)!="iconified" &&
      $ascProbVect(initialstate)!="withdrawn"} {
    wm deiconify .probe
  }
  if {"$ascProbVect(initialstate)"=="withdrawn"} {
    wm withdraw .probe
  }

}



# proc build_probebox {parentname winnum font}
#---------------------------------------------------------------------------
# creates a listbox widget with scrollbars in the context
# of parentname, if same does not already exist.
# the name of the listbox widget is
# $parentname.probe_box_$winnum
# returns the name $parentname.script_box_$winnum
# The widget with this name has a child named listbox1.
# scrollbars are done with bg/fg colors
# text is done with tbg/tfg, sbg/sfg for plain and selected text,
# respectively.
# if the widget already exists, recolors it from the given input.
#---------------------------------------------------------------------------
#
proc build_probebox {parentname winnum font} {
  if { ! [winfo exists  $parentname.probe_box_$winnum] } {
    # It does not exist, build it.

    # build Frame to hold scrollbars and listbox
    frame $parentname.probe_box_$winnum

    # build Verticle Scrollbar
    scrollbar $parentname.probe_box_$winnum.scrollbarV \
      -command "$parentname.probe_box_$winnum.listbox1 yview"

    # build Horizontal Scrollbar
    scrollbar $parentname.probe_box_$winnum.scrollbarH \
      -command "$parentname.probe_box_$winnum.listbox1 xview" \
      -orient {horizontal}

    # build Listbox to hold probe variables
    listbox $parentname.probe_box_$winnum.listbox1 \
      -relief {raised} \
      -xscrollcommand "$parentname.probe_box_$winnum.scrollbarH set" \
      -yscrollcommand "$parentname.probe_box_$winnum.scrollbarV set" \
      -width 1 \
      -height 1 \
      -font $font \
      -exportselection 0 \
      -selectmode extended

    # pack widget $parentname.probe_box_$winnum
    pack append $parentname.probe_box_$winnum \
      $parentname.probe_box_$winnum.scrollbarV {right frame center filly} \
      $parentname.probe_box_$winnum.listbox1 {top frame center expand fill} \
      $parentname.probe_box_$winnum.scrollbarH {bottom frame center fillx}
  }

  $parentname.probe_box_$winnum.listbox1 configure \
    -font $font
 
  return $parentname.probe_box_$winnum
}


# Internal procedures

# eof
#

