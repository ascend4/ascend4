#  display.tcl
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.22 $
#  Last modified on: $Date: 1998/06/18 15:55:24 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: display.tcl,v $
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

# Module: display.tcl
# Tcl version: 7.1 (Tcl/Tk/XF)
# Tk version: 3.4
# XF version: 2.2
#

# module contents
global moduleList
global autoLoadList
set moduleList(display.tcl) { .display}
set autoLoadList(display.tcl) {0}

# procedures to show toplevel windows


# procedure to show window ShowWindow.display
# proc ShowWindow.display { args}
proc ShowWindow.display { args} { # xf ignore me 7
  global env ascGlobalVect ascDispVect

StartupSrc.display

  # build widget .display
  if {"[info procs XFEdit]" != ""} {
    catch "XFDestroy .display"
  } {
    catch "destroy .display"
  }
  toplevel .display \
    -relief {raised}

  # Ascend patched window manager configurations
  wm iconify .display
  wm positionfrom .display user
  wm sizefrom .display user
  wm iconname .display {Display}
  wm minsize .display 100 100
  wm title .display {A4 Display}
  wm protocol .display WM_DELETE_WINDOW {Toggle_Remote ascDispVect}


  # build widget .display.entry_frm
  frame .display.entry_frm \
    -borderwidth {2} \
    -relief {raised}

  # build widget .display.entry_frm.entry_box
  entry .display.entry_frm.entry_box \
    -exportselection 0 \
    -textvariable ascDispVect(entry)

  # pack widget .display.entry_frm
  pack append .display.entry_frm \
    .display.entry_frm.entry_box {left frame center expand fillx}


  # build widget .display.main_frm
  frame .display.main_frm \
    -borderwidth {0} \
    -relief {raised}

  # build widget .display.main_frm.display_box
  frame .display.main_frm.display_box \
    -relief {raised}

  # build widget .display.main_frm.display_box.scrollbar1
  scrollbar .display.main_frm.display_box.scrollbar1 \
    -command {.display.main_frm.display_box.text2 yview}

  # build widget .display.main_frm.display_box.text2
  text .display.main_frm.display_box.text2 \
    -exportselection 0 \
    -borderwidth {2}  \
    -relief {raised} \
    -wrap {word} \
    -yscrollcommand {.display.main_frm.display_box.scrollbar1 set} \
    -font $ascDispVect(font) \
    -height 1

  # pack widget .display.main_frm.display_box
  pack append .display.main_frm.display_box \
    .display.main_frm.display_box.scrollbar1 {right frame center filly} \
    .display.main_frm.display_box.text2 {top frame center expand fill}

  # pack widget .display.main_frm
  pack append .display.main_frm \
    .display.main_frm.display_box {top frame center expand fill}


  #
  # Build the menu bar and submenus
  #
  menu .display.menubar \
    -tearoffcommand .DISPLAY.MENUBAR \
    -tearoff 0

  # The File menu
  menu .display.menubar.file \
    -tearoffcommand .DISPLAY.MENUBAR.FILE \
    -tearoff 0
  .display.menubar.file add command \
    -command {DispExecutePrint} \
    -label {Print...} \
    -accelerator {Alt-f p} \
    -underline 0
  .display.menubar.file add command \
    -command {Toggle_Remote ascDispVect} \
    -label {Close window} \
    -accelerator {Alt-f c} \
    -underline 0
  .display.menubar.file add command \
    -command {Script_do_Exit} \
    -accelerator {Alt-f e} \
    -label {Exit ASCEND...} \
    -underline 0

  # The Edit menu
  menu .display.menubar.edit \
    -tearoffcommand .DISPLAY.MENUBAR.EDIT \
    -tearoff 0
  .display.menubar.edit add command \
    -command {Disp_do_Cut} \
    -label {Cut} \
    -accelerator {F2} \
    -underline -1
  .display.menubar.edit add command \
    -command {Disp_do_Copy} \
    -label {Copy} \
    -accelerator {F3} \
    -underline -1
  .display.menubar.edit add command \
    -command {Disp_do_Paste} \
    -label Paste \
    -accelerator {F4} \
    -underline -1

  # The View menu
  menu .display.menubar.view \
    -tearoffcommand .DISPLAY.MENUBAR.VIEW \
    -tearoff 0
  .display.menubar.view add checkbutton \
    -command { } \
    -offvalue {0} \
    -onvalue {1} \
    -variable {ascDispVect(ShowComments)} \
    -label {Show comments in code} \
    -accelerator {Alt-v c} \
    -underline 5
  .display.menubar.view add command \
    -command {Disp_do_SaveOptions} \
    -label {Save Display options} \
    -accelerator {Alt-v d} \
    -underline 5
  .display.menubar.view add separator
  .display.menubar.view add command \
    -command {Disp_do_Font} \
    -accelerator {Alt-v f} \
    -label {Font ...} \
    -underline 0
  .display.menubar.view add checkbutton \
    -offvalue {0} \
    -onvalue {1} \
    -variable {ascDispVect(visibility)} \
    -label {Open automatically} \
    -accelerator {Alt-v o} \
    -underline 0
  .display.menubar.view add command \
    -command {View_Set_Save_Options display} \
    -label {Save window appearance} \
    -accelerator {Alt-v s} \
    -underline 0


  # The Help menu
  menu .display.menubar.help \
    -tearoffcommand .DISPLAY.MENUBAR.HELP \
    -tearoff 0
  .display.menubar.help add command \
    -command {Disp_do_Help} \
    -label {On Display} \
    -underline 3


  #
  # Add the menus as cascades of the toplevel's menu;
  # add the toplevel's menu to the toplevel
  #
  .display.menubar add cascade \
    -menu .display.menubar.file \
    -label {File} \
    -underline 0
  .display.menubar add cascade \
    -menu .display.menubar.edit \
    -label Edit \
    -underline 0
  .display.menubar add cascade \
    -menu .display.menubar.view \
    -label {View} \
    -underline 0
  .display.menubar add cascade \
    -menu .display.menubar.help \
    -label {Help} \
    -underline 0
  .display configure \
    -menu .display.menubar


  #
  #  Add the entry and main frames into the toplevel .display
  #
  pack append .display \
    .display.main_frm {top frame center expand fill} \
    .display.entry_frm {top frame center fillx}


  .display.entry_frm.entry_box insert end {}
  .display.main_frm.display_box.text2 insert end {}

  EndSrc.display

  if {"[info procs XFEdit]" != ""} {
    catch "XFMiscBindWidgetTree .display"
    after 2 "catch {XFEditSetShowWindows}"
  }
}

# proc DestroyWindow.display {}
proc DestroyWindow.display {} {# xf ignore me 7
  if {"[info procs XFEdit]" != ""} {
    if {"[info commands .display]" != ""} {
      global xfShowWindow.display
      set xfShowWindow.display 0
      XFEditSetPath .
      after 2 "XFSaveAsProc .display; XFEditSetShowWindows"
    }
  } {
    catch "destroy .display"
    update
  }
}

# proc StartupSrc.display {args}
proc StartupSrc.display {args} {
# display startup entrance
# as much of the C/global dependencies managed here as possible
  global ascDispVect ascGlobalVect env
  if {[catch {set ascDispVect(windowname)} ]} {
    set ascDispVect(minsize) 100x100
    set ascDispVect(geometry) 200x400+10+10
    set ascDispVect(iconname) Display
    set ascDispVect(initialstate) normal
    set ascDispVect(font) "-*-*"
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

proc EndSrc.display {} {
 # display startup exit
  global ascDispVect
  set ascDispVect(geometry) [sanegeometry $ascDispVect(geometry)]
  set minw [lindex [split $ascDispVect(minsize) x] 0]
  set minh [lindex [split $ascDispVect(minsize) x] 1]
  set gw [lindex [split [lindex [split $ascDispVect(geometry) +] 0] x] 0]
  set gh [lindex [split [lindex [split $ascDispVect(geometry) +] 0] x] 1]
  set gp "+[lindex [split [split $ascDispVect(geometry) x] +] 1]+[lindex [split [split $ascDispVect(geometry) x] +] 2]"
  if {[expr $gw < $minw]} {set gw $minw}
  if {[expr $gh < $minh]} {set gh $minh}
  set gwh "${gw}x${gh}"
  wm positionfrom .display user
  wm sizefrom .display user
  wm minsize .display $minw $minh
  wm geometry .display [osgpos $gwh$gp]
  wm iconname .display $ascDispVect(iconname)

  if {$ascDispVect(initialstate)!="iconic" && \
      $ascDispVect(initialstate)!="iconified" && \
      $ascDispVect(initialstate)!="withdrawn"} {
    wm deiconify .display
  }
  if {$ascDispVect(initialstate)=="withdrawn"} {
    wm withdraw .display
  }
}



# Internal procedures

# eof
#

