#  toolbox.tcl
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.13 $
#  Last modified on: $Date: 1998/06/18 15:55:42 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: toolbox.tcl,v $
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

# Module: toolbox.tcl
# Tcl version: 7.1 (Tcl/Tk/XF)
# Tk version: 3.4
# XF version: 2.2
#

# module contents
global moduleList
global autoLoadList
set moduleList(toolbox.tcl) { .toolbox do_raise do_raise_lower}
set autoLoadList(toolbox.tcl) {0}

# procedures to show toplevel windows


# procedure to show window ShowWindow.toolbox
# proc ShowWindow.toolbox { args}
proc ShowWindow.toolbox { args} {
# xf ignore me 7

  global env ascGlobalVect ascToolVect
StartupSrc.toolbox

  # build widget .toolbox
  if {"[info procs XFEdit]" != ""} {
    catch "XFDestroy .toolbox"
  } {
    catch "destroy .toolbox"
  }
  toplevel .toolbox   -borderwidth {2}

  # Window manager configurations
  wm iconify .toolbox
  wm positionfrom .toolbox user
  wm sizefrom .toolbox user
  wm iconname .toolbox {Toolbox}
  wm minsize .toolbox 100 100
  wm title .toolbox {A4 Toolbox}
  wm protocol .toolbox WM_DELETE_WINDOW {set ascToolVect(window.open) 0}

  # build widget .toolbox.bottom_frm
  frame .toolbox.bottom_frm

  # build widget .toolbox.bottom_frm.browser_btn
  button .toolbox.bottom_frm.browser_btn \
    -text {BROWSER} \
    -font $ascGlobalVect(font)


  # build widget .toolbox.bottom_frm.display_btn
  button .toolbox.bottom_frm.display_btn \
    -text {DISPLAY} \
    -font $ascGlobalVect(font)

  # build widget .toolbox.bottom_frm.library_btn
  button .toolbox.bottom_frm.library_btn \
    -text {LIBRARY} \
    -font $ascGlobalVect(font)

  # build widget .toolbox.bottom_frm.probe_btn
  button .toolbox.bottom_frm.probe_btn \
    -text {PROBE} \
    -font $ascGlobalVect(font)

  # build widget .toolbox.bottom_frm.script_btn
  button .toolbox.bottom_frm.script_btn \
    -text {SCRIPT} \
    -font $ascGlobalVect(font)

  # build widget .toolbox.bottom_frm.solver_btn
  button .toolbox.bottom_frm.solver_btn \
    -text {SOLVER} \
    -font $ascGlobalVect(font)

  # build widget .toolbox.bottom_frm.units_btn
  button .toolbox.bottom_frm.units_btn \
    -text {UNITS} \
    -font $ascGlobalVect(font)

  # build widget .toolbox.bottom_frm.util_btn
  button .toolbox.bottom_frm.util_btn \
    -text {utilities} \
    -font $ascGlobalVect(font) \
    -command Tool_do_UtilBox

  # build widget .toolbox.bottom_frm.exit_btn
  button .toolbox.bottom_frm.exit_btn \
    -text {exit} \
    -font $ascGlobalVect(font)

  # build widget .toolbox.bottom_frm.callback_btn
  button .toolbox.bottom_frm.callback_btn \
    -font $ascGlobalVect(font) \
    -text {internals} \
    -command {Tool_do_Callbacks}

  # build widget .toolbox.bottom_frm.bug_btn
  button .toolbox.bottom_frm.bug_btn \
    -font $ascGlobalVect(font) \
    -text {bug report} \
    -command {Tool_do_Bugs}

  # build widget .toolbox.bottom_frm.plot_btn
  button .toolbox.bottom_frm.plot_btn \
    -font $ascGlobalVect(font) \
    -text {ascplot} \
    -command {ascplot_open}

  # build widget .toolbox.bottom_frm.hlp_btn
  button .toolbox.bottom_frm.hlp_btn \
    -text {help} \
    -font $ascGlobalVect(font)

  # pack widget .toolbox.bottom_frm
  pack append .toolbox.bottom_frm \
    .toolbox.bottom_frm.exit_btn {top frame center expand fillx} \
    .toolbox.bottom_frm.plot_btn {top frame center expand fillx} \
    .toolbox.bottom_frm.hlp_btn {top frame center expand fillx} \
    .toolbox.bottom_frm.util_btn {top frame center expand fillx} \
    .toolbox.bottom_frm.callback_btn {top frame center expand fillx} \
    .toolbox.bottom_frm.bug_btn {top frame center expand fillx} \
    .toolbox.bottom_frm.library_btn {top frame center expand fillx} \
    .toolbox.bottom_frm.browser_btn {top frame center expand fillx} \
    .toolbox.bottom_frm.solver_btn {top frame center expand fillx} \
    .toolbox.bottom_frm.probe_btn {top frame center expand fillx} \
    .toolbox.bottom_frm.units_btn {top frame center expand fillx} \
    .toolbox.bottom_frm.display_btn {top frame center expand fillx} \
    .toolbox.bottom_frm.script_btn {top frame center expand fillx}

  # pack widget .toolbox
  pack append .toolbox  \
    .toolbox.bottom_frm {top frame center expand fill}

EndSrc.toolbox

  if {"[info procs XFEdit]" != ""} {
    catch "XFMiscBindWidgetTree .toolbox"
    after 2 "catch {XFEditSetShowWindows}"
  }
}

# proc DestroyWindow.toolbox {}
proc DestroyWindow.toolbox {} {# xf ignore me 7
  if {"[info procs XFEdit]" != ""} {
    if {"[info commands .toolbox]" != ""} {
      global xfShowWindow.toolbox
      set xfShowWindow.toolbox 0
      XFEditSetPath .
      after 2 "XFSaveAsProc .toolbox; XFEditSetShowWindows"
    }
  } {
    catch "destroy .toolbox"
    update
  }
}

# proc StartupSrc.toolbox {args}
proc StartupSrc.toolbox {args} {
#toolbox startup entrance
  global ascToolVect ascGlobalVect env
  if {[catch {set ascToolVect(windowname)} ]} {
    set ascToolVect(minsize) 20x100 
    set ascToolVect(geometry) +10+10
    set ascToolVect(iconname) Toolbox
    set ascToolVect(initialstate) normal
    set ascToolVect(font) "-*-*"
  }
  if {[catch {set ascGlobalVect(font)} ]} {
    set ascGlobalVect(font) "-*-*"
    set ascGlobalVect(labelfont) "-*-*"
    set ascGlobalVect(visibility) 1
    set ascGlobalVect(c_loaded) 0
    set ascGlobalVect(toolbitmap) \
      "@$env(ASCENDBITMAPS)/toolAttributes.xbm"
  }
}

# proc EndSrc.toolbox {}
proc EndSrc.toolbox {} {
  global ascToolVect
  set ascToolVect(geometry) [sanegeometry $ascToolVect(geometry)]
  set minw [lindex [split $ascToolVect(minsize) x] 0]
  set minh [lindex [split $ascToolVect(minsize) x] 1]
  set gw [lindex [split [lindex [split $ascToolVect(geometry) +] 0] x] 0]
  set gh [lindex [split [lindex [split $ascToolVect(geometry) +] 0] x] 1]
  set gp "+[lindex [split [split $ascToolVect(geometry) x] +] 1]+[lindex [split [split $ascToolVect(geometry) x] +] 2]"
  if {[expr $gw < $minw]} {set gw $minw}
  if {[expr $gh < $minh]} {set gh $minh}
  set gwh "${gw}x${gh}"
  wm positionfrom .toolbox ""
  wm sizefrom .toolbox ""
  wm maxsize .toolbox 1000 1000
  wm minsize .toolbox $minw $minh
 # wm geometry .toolbox [osgpos $gwh$gp]
  wm title .toolbox {A4 Toolbox}
  wm iconname .toolbox $ascToolVect(iconname)

  if {$ascToolVect(initialstate)!="iconic" && \
      $ascToolVect(initialstate)!="iconified" && \
      $ascToolVect(initialstate)!="withdrawn"} {
    wm deiconify .toolbox
  }
  if {$ascToolVect(initialstate)=="withdrawn"} {
    wm withdraw .toolbox
  }
}


# User defined procedures


# Internal procedures

# eof
#

