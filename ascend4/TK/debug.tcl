#  debug.tcl
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.9 $
#  Last modified on: $Date: 1998/06/18 15:55:23 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: debug.tcl,v $
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

# Module: debug.tcl
# Tcl version: 7.1 (Tcl/Tk/XF)
# Tk version: 3.4
# XF version: 2.2
#

# module contents
global moduleList
global autoLoadList
set moduleList(debug.tcl) { .debug}
set autoLoadList(debug.tcl) {1}

# procedures to show toplevel windows


# procedure to show window .debug
# proc ShowWindow.debug {args}
proc ShowWindow.debug {args} {# xf ignore me 7

  global ascDebuVect ascGlobalVect ascSolvVect
  # build widget .debug
  if {"[info procs XFEdit]" != ""} {
    catch "XFDestroy .debug"
  } {
    catch "destroy .debug"
  }
  toplevel .debug

  # Window manager configurations
  wm positionfrom .debug user
  wm sizefrom .debug user
  wm minsize .debug 128 458
  wm title .debug {A4 System Debugger}
  wm iconname .debug {Debugger}
  wm geometry .debug $ascDebuVect(geometry)
  wm protocol .debug WM_DELETE_WINDOW {Debug_do_OK}

  catch {font create debuggerfont}
  set ascDebuVect(font) debuggerfont

  # build widget .debug.btn_hlp
  button .debug.btn_hlp \
    -command {Debug_do_Help} \
    -font $ascDebuVect(font) \
    -text { Help }

  # build widget .debug.btn_attr_eqn
  button .debug.btn_attr_eqn \
    -borderwidth {0} \
    -font $ascDebuVect(font) \
    -command {Debug_do_EqnAttr} \
    -text {Attributes}

  # build widget .debug.btn_attr_var
  button .debug.btn_attr_var \
    -borderwidth {0} \
    -command {Debug_do_VarAttr} \
    -font $ascDebuVect(font) \
    -text {Attributes}

  # build widget .debug.btn_blk_eqns
  menubutton .debug.btn_blk_eqns \
    -font $ascDebuVect(font) \
    -menu {.debug.btn_blk_eqns.m} \
    -text {Equations}

  # build widget .debug.btn_blk_eqns.m
  menu .debug.btn_blk_eqns.m \
    -tearoffcommand .DEBUG.BTN_BLK_EQNS.M -font $ascDebuVect(font) \
    -tearoff 0

  .debug.btn_blk_eqns.m add command \
    -command {Debug_do_BlkEqnResids} \
    -label {Residuals} \
    -state {active}
  .debug.btn_blk_eqns.m add command \
    -command {Debug_do_BlkEqnAttr} \
    -label {Attributes}
  .debug.btn_blk_eqns.m add command \
    -command {Debug_do_BlkEqnDep} \
    -label {Find Dependent}
  .debug.btn_blk_eqns.m add command \
    -command {Debug_do_BlkEqn2Probe} \
    -label {Export to Probe}

  global ascDebuVect
  # build widget .debug.btn_blk_size
  label .debug.btn_blk_size \
    -font $ascDebuVect(font) \
    -textvariable ascDebuVect(blksizes)

  # build widget .debug.btn_blk_var
  menubutton .debug.btn_blk_var \
    -menu {.debug.btn_blk_var.m} \
    -font $ascDebuVect(font) \
    -text {Variables}

  # build widget .debug.btn_blk_var.m
  menu .debug.btn_blk_var.m \
    -tearoffcommand .DEBUG.BTN_BLK_VAR.M -font $ascDebuVect(font) \
    -tearoff 0

  .debug.btn_blk_var.m add command \
    -command {Debug_do_BlkVarVal} \
    -label {Values} \
    -state {active}
  .debug.btn_blk_var.m add command \
    -command {Debug_do_BlkVarAttr} \
    -label {Attributes}
  .debug.btn_blk_var.m add command \
    -command {Debug_do_BlkVar2Probe} \
    -label {Probe}

  # build widget .debug.btn_expo_blk
  button .debug.btn_expo_blk \
    -borderwidth {0} \
    -command {Debug_do_Blk2Probe} \
    -font $ascDebuVect(font) \
    -text {Export to probe}

  # build widget .debug.btn_expo_eqn
  menubutton .debug.btn_expo_eqn \
    -menu {.debug.btn_expo_eqn.m} \
    -font $ascDebuVect(font) \
    -text {Export}

  # build widget .debug.btn_expo_eqn.m
  menu .debug.btn_expo_eqn.m \
    -tearoffcommand .DEBUG.BTN_EXPO_EQN.M -font $ascDebuVect(font) \
    -tearoff 0

  .debug.btn_expo_eqn.m add command \
    -command {Debug_do_Eqn2Browser} \
    -label {Browser}
  .debug.btn_expo_eqn.m add command \
    -command {Debug_do_Eqn2Probe} \
    -label {Probe}

  # build widget .debug.btn_expo_sys
  button .debug.btn_expo_sys \
    -font $ascDebuVect(font) \
    -command {Debug_do_Sys2Probe} \
    -text {Export to probe}

  # build widget .debug.btn_expo_var
  menubutton .debug.btn_expo_var \
    -font $ascDebuVect(font) \
    -menu {.debug.btn_expo_var.m} \
    -text {Export}

  # build widget .debug.btn_expo_var.m
  menu .debug.btn_expo_var.m \
    -tearoffcommand .DEBUG.BTN_EXPO_VAR.M -font $ascDebuVect(font) \
    -tearoff 0

  .debug.btn_expo_var.m add command \
    -command {Debug_do_Var2Browser} \
    -label {Browser}
  .debug.btn_expo_var.m add command \
    -command {Debug_do_Var2Probe} \
    -label {Probe}

  # build widget .debug.btn_name_eqn
  button .debug.btn_name_eqn \
    -font $ascDebuVect(font) \
    -command {Debug_do_EqnName} \
    -text {Name}

  # build widget .debug.btn_name_var
  button .debug.btn_name_var \
    -font $ascDebuVect(font) \
    -command {Debug_do_VarName} \
    -text {Name}

  # build widget .debug.btn_ok
  button .debug.btn_ok \
    -font $ascDebuVect(font) \
    -command {Debug_do_OK} \
    -text {  OK  }

  # build widget .debug.btn_sys_var
  menubutton .debug.btn_sys_var \
    -menu {.debug.btn_sys_var.m} \
    -font $ascDebuVect(font) \
    -text {Variables}

  # build widget .debug.btn_sys_var.m
  menu .debug.btn_sys_var.m \
    -tearoffcommand .DEBUG.BTN_SYS_VAR.M -font $ascDebuVect(font) \
    -tearoff 0

  .debug.btn_sys_var.m add command \
    -command {Debug_do_SysVarVal} \
    -label {Values} \
    -state {active}
  .debug.btn_sys_var.m add command \
    -command {Debug_do_SysVarAttr} \
    -label {Attributes}
  .debug.btn_sys_var.m add command \
    -command {Debug_do_SysVar2Nom} \
    -label {Reset Values}
  .debug.btn_sys_var.m add command \
    -command {Debug_do_SysNom2Var} \
    -label {Reset Nominals}

  # build widget .debug.entry_blk
  entry .debug.entry_blk \
    -exportselection {0} \
    -font $ascSolvVect(font) \
    -textvariable {ascDebuVect(blkcur)}

  # build widget .debug.entry_eqn
  entry .debug.entry_eqn \
    -exportselection {0} \
    -font $ascSolvVect(font) \
    -textvariable {ascDebuVect(eqncur)}

  # build widget .debug.entry_var
  entry .debug.entry_var \
    -exportselection {0} \
    -font $ascSolvVect(font) \
    -relief {groove} \
    -textvariable {ascDebuVect(varcur)}

  # build widget .debug.label_blank
  label .debug.label_blank \
    -text {}

  # build widget .debug.label_blk
  label .debug.label_blk \
    -anchor {w} \
    -font $ascDebuVect(font) \
    -height {2} \
    -text { Block:}

  # build widget .debug.label_eqn
  label .debug.label_eqn \
    -anchor {w} \
    -font $ascDebuVect(font) \
    -text { Equation:}
  # bindings
  bind .debug.label_eqn <Button-2> {
    MenuPopupPost .debug.label_eqn.poptest %X %Y
  }
  bind .debug.label_eqn <ButtonRelease-2> {
    MenuPopupRelease .debug.label_eqn.poptest %W
  }

  # build widget .debug.label_sys
  label .debug.label_sys \
    -anchor {w} \
    -font $ascDebuVect(font) \
    -height {2} \
    -relief {raised} \
    -text { System:}

  # build widget .debug.label_var
  label .debug.label_var \
    -anchor {w} \
    -font $ascDebuVect(font) \
    -foreground {black} \
    -height {2} \
    -text { Variable:}

  # pack widget .debug
  pack append .debug \
    .debug.label_var {top frame center fillx} \
    .debug.entry_var {top frame center fillx} \
    .debug.btn_name_var {top frame center fillx} \
    .debug.btn_attr_var {top frame center fillx} \
    .debug.btn_expo_var {top frame center fillx} \
    .debug.label_eqn {top frame center fillx} \
    .debug.entry_eqn {top frame center fillx} \
    .debug.btn_name_eqn {top frame center fillx} \
    .debug.btn_attr_eqn {top frame center fillx} \
    .debug.btn_expo_eqn {top frame center fillx} \
    .debug.label_blk {top frame center fillx} \
    .debug.entry_blk {top frame center fillx} \
    .debug.btn_blk_size {top frame center fillx} \
    .debug.btn_blk_var {top frame center fillx} \
    .debug.btn_blk_eqns {top frame center fillx} \
    .debug.btn_expo_blk {top frame center fillx} \
    .debug.label_sys {top frame center fillx} \
    .debug.btn_sys_var {top frame center fillx} \
    .debug.btn_expo_sys {top frame center fillx} \
    .debug.label_blank {top frame center fillx} \
    .debug.btn_ok {left frame center expand fill} \
    .debug.btn_hlp {left frame center expand fill}

  if {"[info procs XFEdit]" != ""} {
    catch "XFMiscBindWidgetTree .debug"
    after 2 "catch {XFEditSetShowWindows}"
  }
}

# proc DestroyWindow.debug {}
proc DestroyWindow.debug {} {# xf ignore me 7
  if {"[info procs XFEdit]" != ""} {
    if {"[info commands .debug]" != ""} {
      global xfShowWindow.debug
      set xfShowWindow.debug 0
      XFEditSetPath .
      after 2 "XFSaveAsProc .debug; XFEditSetShowWindows"
    }
  } {
    catch "destroy .debug"
    update
  }
}

# User defined procedures


# Internal procedures

# eof
#

