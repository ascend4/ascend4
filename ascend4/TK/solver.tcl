#  solver.tcl
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.35 $
#  Last modified on: $Date: 1998/06/18 15:55:40 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: solver.tcl,v $
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

# Module: solver.tcl
# Tcl version: 7.1 (Tcl/Tk/XF)
# Tk version: 3.4
# XF version: 2.2
#

# module contents
global moduleList
global autoLoadList
set moduleList(solver.tcl) { .solver}
set autoLoadList(solver.tcl) {0}

# procedures to show toplevel windows


# procedure to show window .solver
# proc ShowWindow.solver {args}
proc ShowWindow.solver {args} {# xf ignore me 7

  global env ascGlobalVect ascSolvVect
  StartupSrc.solver
  set ascSolvVect(statreport) 1

  # build widget .solver
  if {"[info procs XFEdit]" != ""} {
    catch "XFDestroy .solver"
  } {
    catch "destroy .solver"
  }
  toplevel .solver

  # Window manager configurations
  set ascSolvVect(geometry) [sanegeometry $ascSolvVect(geometry)]
  set minw [lindex [split $ascSolvVect(minsize) x] 0]
  set minh [lindex [split $ascSolvVect(minsize) x] 1]
  set gw [lindex [split [lindex [split $ascSolvVect(geometry) +-] 0] x] 0]
  set gh [lindex [split [lindex [split $ascSolvVect(geometry) +-] 0] x] 1]
  set gp "+[lindex [split [split $ascSolvVect(geometry) x] +-] 1]+[lindex [split [split $ascSolvVect(geometry) x] +-] 2]"
  if {[expr $gw < $minw]} {
    set gw $minw
  }
  if {[expr $gh < $minh]} {
    set gh $minh
  }
  set gwh "${gw}x${gh}"
  wm positionfrom .solver user
  wm sizefrom .solver user
  wm minsize .solver $minw $minh
  wm geometry .solver [osgpos $gwh$gp]
  wm iconname .solver $ascSolvVect(iconname)
  wm protocol .solver WM_DELETE_WINDOW {Toggle_Remote ascSolvVect}
  wm title .solver "A4 Solver"

  # build widget .solver.lbl_frm
  frame .solver.lbl_frm \
    -relief {raised}

  # build widget .solver.lbl_frm.label
  label .solver.lbl_frm.label \
    -highlightthickness 0 \
    -text {SOLVER: }

  # build widget .solver.lbl_frm.entry5
  menubutton .solver.lbl_frm.entry5 \
    -menu .solver.lbl_frm.entry5.m \
    -relief {raised} \
    -highlightthickness 0 \
    -textvariable {ascSolvStatVect(solver)}
  menu .solver.lbl_frm.entry5.m \
    -tearoff 0

  menubutton .solver.lbl_frm.btn_opts \
    -menu .solver.lbl_frm.btn_opts.m \
    -relief {raised} \
    -highlightthickness 0 \
    -text {Options}
  menu .solver.lbl_frm.btn_opts.m \
    -tearoff 0

  .solver.lbl_frm.btn_opts.m add checkbutton \
    -offvalue 0 \
    -onvalue 1 \
    -variable ascSolvVect(statreport) \
    -label {Pop-up status report} \
    -accelerator Ctrl-p \
    -underline -1

  set ascSolvVect(saveparmenu) .solver.lbl_frm.btn_opts.m.savepar

  .solver.lbl_frm.btn_opts.m add cascade  \
    -menu .solver.lbl_frm.btn_opts.m.savepar \
    -label {Save solver's parameters} \
    -underline -1
  .solver.lbl_frm.btn_opts.m add command \
    -command {Solve_do_Parms open General} \
    -accelerator Ctrl-g \
    -label {General ...}
  .solver.lbl_frm.btn_opts.m add separator

  # The savepar menu under the options menu
  menu .solver.lbl_frm.btn_opts.m.savepar \
    -tearoffcommand .SOLVER.LBL_FRM.BTN_OPTS.M.SAVEPAR \
    -tearoff 0
  .solver.lbl_frm.btn_opts.m.savepar add command \
     -command {View_Save_Solvers_Params} \
     -label {All Solvers}


  label .solver.lbl_frm.lbl_run \
    -bitmap {feet} \
    -padx 15

  # build widget .solver.lbl_frm.btn_int
  button .solver.lbl_frm.btn_int \
    -command Solve_do_Interrupt \
    -bitmap {stop}

  # build widget .solver.lbl_frm.lbl_int
  button .solver.lbl_frm.lbl_int \
    -command Solve_do_Interrupt \
    -text {Halt}

  # pack widget .solver.lbl_frm
  pack append .solver.lbl_frm \
    .solver.lbl_frm.label {left frame center filly} \
    .solver.lbl_frm.entry5 {left frame center filly} \
    .solver.lbl_frm.btn_opts {left frame center filly} \
    .solver.lbl_frm.lbl_int {right frame center filly} \
    .solver.lbl_frm.btn_int {right frame center filly} \
    .solver.lbl_frm.lbl_run {right frame center filly padx 20}

  # build widget .solver.main_frm
  frame .solver.main_frm

  # build widget .solver.main_frm.inst_entry
  entry .solver.main_frm.inst_entry \
    -exportselection 0 \
    -font $ascSolvVect(font) \
    -highlightthickness 0 \
    -state {disabled} \
    -exportselection 0 \
    -textvariable {ascSolvVect(instname)}

  # build widget .solver.main_frm.objvar_entry
  entry .solver.main_frm.objvar_entry \
    -exportselection 0 \
    -font $ascSolvVect(font) \
    -highlightthickness 0 \
    -state {disabled} \
    -exportselection 0 \
    -textvariable {ascSolvVect(objvarname)}

  if {$ascSolvVect(modelbar)} {
    # build widget .solver.main_frm.instpath
    frame .solver.main_frm.instpath 

    # build widget .solver.main_frm.btn_expo
    menubutton .solver.main_frm.btn_expo \
      -font $ascGlobalVect(font) \
      -menu {.solver.main_frm.btn_expo.m} \
      -text {Import} \
      -underline {0}

    # build widget .solver.main_frm.btn_expo.m
    menu .solver.main_frm.btn_expo.m \
      -tearoffcommand .SOLVER.MAIN_FRM.BTN_EXPO.M \
      -font $ascGlobalVect(font) \
      -tearoff 0

    .solver.main_frm.btn_expo.m add command \
      -command {Solve_do_Import} \
      -label {This model}

    # pack widget .solver.main_frm
    pack append .solver.main_frm \
      .solver.main_frm.inst_entry {bottom frame center fill} \
      .solver.main_frm.objvar_entry {bottom frame center fill} \
      .solver.main_frm.instpath {left frame center fill} \
      .solver.main_frm.btn_expo {right frame center}
  } else {
    pack append .solver.main_frm \
      .solver.main_frm.inst_entry {top frame center fill} \
      .solver.main_frm.objvar_entry {top frame center fill}
  }


  #
  # Build the menu bar and submenus
  #
  menu .solver.menubar \
    -tearoffcommand .SOLVER.MENUBAR \
    -tearoff 0

  # The Ananlyze menu
  menu .solver.menubar.analyze \
    -tearoffcommand .SOLVER.MENUBAR.ANALYZE \
    -tearoff 0
  .solver.menubar.analyze add command \
    -command {Solve_do_Reanalyze} \
    -label {Reanalyze} \
    -accelerator {Alt-a r} \
    -underline 0 \
    -state {disabled}
  .solver.menubar.analyze add command \
    -command {Solve_do_Debug} \
    -label {Debugger} \
    -accelerator {Alt-a g} \
    -underline 4 \
    -state {disabled}
  .solver.menubar.analyze add command \
    -command {Solve_do_Overspecified} \
    -label {Overspecified} \
    -accelerator {Alt-a o} \
    -underline 0
  .solver.menubar.analyze add cascade \
    -menu {.solver.menubar.analyze.depend} \
    -label {Find dependent eqns.} \
    -accelerator {Alt-a d} \
    -underline 5
  .solver.menubar.analyze add command \
    -command {Solve_do_FindUnAssEqns} \
    -label {Find unassigned eqns.} \
    -accelerator {Alt-a u} \
    -underline 7
  .solver.menubar.analyze add command \
    -command {Solve_do_EvalUnincluded} \
    -label {Evaluate unincluded eqns.} \
    -accelerator {Alt-a e} \
    -underline 0
  .solver.menubar.analyze add command \
    -command {Solve_Find_Near_Bounds} \
    -label {Find vars near bounds} \
    -accelerator {Alt-a b} \
    -underline 15
  .solver.menubar.analyze add command \
    -command {Solve_Find_Far_From_Nominals} \
    -label {Find vars far from nominal} \
    -accelerator {Alt-a n} \
    -underline 19

  # The Dependency menu under the Analyze menu
  menu .solver.menubar.analyze.depend \
    -tearoffcommand .SOLVER.MENUBAR.ANALYZE.DEPEND \
    -tearoff 0
  .solver.menubar.analyze.depend add command \
    -command {Solve_do_NumDepends} \
    -label {Numeric Dependency} \
    -underline 0
  .solver.menubar.analyze.depend add command \
    -command {Solve_do_StrucDepends} \
    -label {Structural Dependency} \
    -underline 0

  # The Display menu
  menu .solver.menubar.display \
    -tearoffcommand .SOLVER.MENUBAR.DISPLAY \
    -tearoff 0
  .solver.menubar.display add command \
    -command {Solve_do_Display_Status} \
    -accelerator {Alt-d s}\
    -label {Status} \
    -underline 0
  .solver.menubar.display add command \
    -command {Solve_do_DispUnattached} \
    -label {Unattached variables} \
    -accelerator {Alt-d v} \
    -underline 11
  .solver.menubar.display add command \
    -command {Solve_do_DispUnincluded} \
    -label {Unincluded relations} \
    -accelerator {Alt-d r} \
    -underline 11
  .solver.menubar.display add command \
    -command {Solve_do_DispIncidence} \
    -label {Incidence matrix} \
    -accelerator {Alt-d i} \
    -underline 0

  # The Edit menu
  menu .solver.menubar.edit \
    -tearoffcommand .SOLVER.MENUBAR.EDIT \
    -tearoff 0
  .solver.menubar.edit add command \
    -command {Solve_do_Flush} \
    -label {Remove instance} \
    -accelerator {Alt-e r} \
    -underline 0
  .solver.menubar.edit add command \
    -command {Solve_do_Select_Objective} \
    -state {disabled} \
    -label {Select objective ...} \
    -accelerator {Alt-e o} \
    -underline 7

  # The File menu
  menu .solver.menubar.file \
    -tearoffcommand .SOLVER.MENUBAR.FILE \
    -tearoff 0
  .solver.menubar.file add command \
    -command {Solve_do_Close} \
    -label {Close Window} \
    -accelerator {Alt-f c} \
    -underline 0
  .solver.menubar.file add command \
    -command {Solve_do_Exit} \
    -label {Exit ASCEND} \
    -accelerator {Alt-f e} \
    -underline 0

  # The Execute menu
  menu .solver.menubar.execute \
    -tearoffcommand .SOLVER.MENUBAR.EXECUTE \
    -tearoff 0
  .solver.menubar.execute add command \
    -command {Solve_do_Solve} \
    -label {Solve} \
    -accelerator {F5} \
    -underline -1
  .solver.menubar.execute add command \
    -command {Solve_do_Iterate} \
    -label {Single step} \
    -accelerator {F6} \
    -underline 2
  .solver.menubar.execute add command \
    -command {Solve_do_Integrate} \
    -label {Integrate} \
    -accelerator {F7} \
    -underline 0


  # The Export menu
  menu .solver.menubar.export \
    -tearoffcommand .SOLVER.MENUBAR.EXPORT \
    -tearoff 0
  .solver.menubar.export add command \
    -command {Solve_do_Export2Browser} \
    -label {to Browser} \
    -accelerator {Alt-x b} \
    -underline 3
  .solver.menubar.export add command \
    -command {Solve_do_Export2Probe} \
    -label {to Probe} \
    -accelerator {Alt-x p} \
    -underline 3


  # The Help menu
  menu .solver.menubar.help \
    -tearoffcommand .SOLVER.MENUBAR.HELP \
    -tearoff 0
  .solver.menubar.help add command \
    -command {Solve_do_Help} \
    -label {On Solvers} \
    -underline 3


  # The View menu
  menu .solver.menubar.view \
    -tearoffcommand .SOLVER.MENUBAR.VIEW \
    -tearoff 0
  .solver.menubar.view add command  \
    -command {Solve_do_Font}  \
    -label {Font ...} \
    -accelerator {Alt-v f} \
    -underline 0
  .solver.menubar.view add checkbutton \
    -offvalue {0} \
    -onvalue {1} \
    -variable {ascSolvVect(visibility)} \
    -label {Open automatically} \
    -accelerator {Alt-v o} \
    -underline 0
  .solver.menubar.view add command  \
    -command {View_Set_Save_Options solver}  \
    -label {Save Solver appearance} \
    -accelerator {Alt-v s} \
    -underline 0


  #
  # Add the menus as cascades of the toplevel's menu;
  # add the toplevel's menu to the toplevel
  #
  .solver.menubar add cascade \
    -menu .solver.menubar.file \
    -label {File} \
    -underline 0
  .solver.menubar add cascade \
    -menu .solver.menubar.edit \
    -label {Edit} \
    -underline 0
  .solver.menubar add cascade \
    -menu .solver.menubar.display \
    -label {Display} \
    -underline 0
  .solver.menubar add cascade \
    -menu .solver.menubar.execute \
    -label {Execute} \
    -underline 3
  .solver.menubar add cascade \
    -menu .solver.menubar.analyze \
    -label {Analyze} \
    -underline 0
  .solver.menubar add cascade \
    -menu .solver.menubar.view \
    -label {View} \
    -underline 0
  .solver.menubar add cascade \
    -menu .solver.menubar.export \
    -label {Export} \
    -underline 1
  .solver.menubar add cascade \
    -menu .solver.menubar.help \
    -label {Help} \
    -underline 0
  .solver configure \
    -menu .solver.menubar

  # build widget .solver.msg_frm
  entry .solver.msg_frm \
    -font $ascSolvVect(font) \
    -relief {raised} \
    -exportselection 0 \
    -state {disabled} \
    -textvariable {ascSolvVect(modeltype)}

  # build widget .solver.text_frm
  frame .solver.text_frm 

  # build widget .solver.text_frm.text2
  text .solver.text_frm.text2   \
    -exportselection 0 \
    -borderwidth 2 \
    -height 2 \
    -relief raised \
    -width 10 \
    -wrap none \
    -state disabled \
    -yscrollcommand {.solver.text_frm.scrollbar1 set} \
    -xscrollcommand {.solver.text_frm.scrollbar3 set} \
    -font $ascSolvVect(font)

  # yes, at the moment the scroll bars might be redundant.
  # build widget .solver.text_frm.scrollbar1
  scrollbar .solver.text_frm.scrollbar1 \
    -command {.solver.text_frm.text2 yview}

  # build widget .solver.text_frm.scrollbar3
  scrollbar .solver.text_frm.scrollbar3 \
    -orient horizontal \
    -command {.solver.text_frm.text2 xview}

  # pack widget .solver.text_frm
  pack append .solver.text_frm \
    .solver.text_frm.scrollbar1 {right frame center filly} \
    .solver.text_frm.text2 {top frame center expand fill} \
    .solver.text_frm.scrollbar3 {top frame center fillx}

  # build text and table region
  # pack widget .solver
  pack append .solver \
    .solver.lbl_frm {top frame center fillx} \
    .solver.main_frm {top frame center fillx} \
    .solver.msg_frm {top frame center fillx} \
    .solver.text_frm {top frame center expand fill}


  EndSrc.solver

  if {"[info procs XFEdit]" != ""} {
    catch "XFMiscBindWidgetTree .solver"
    after 2 "catch {XFEditSetShowWindows}"
  }
}

# proc DestroyWindow.solver {}
proc DestroyWindow.solver {} {# xf ignore me 7
  if {[info procs XFEdit] != ""} {
    if {[info commands .solver] != ""} {
      global xfShowWindow.solver
      set xfShowWindow.solver 0
      XFEditSetPath .
      after 2 "XFSaveAsProc .solver; XFEditSetShowWindows"
    }
  } else {
    catch "destroy .solver"
    update
  }
}

# proc StartupSrc.solver {args}
#-----------
# This function defines the labels in the cells
# The function Solve_Update_TableVect (in SolverProc.tcl) defines
# the values in the cells.
#-----------
proc StartupSrc.solver {args} {
# solver startup entrance
  global ascSolvTableVect
  global ascSolvStatVect

  set ascSolvTableVect(0,0)  {Relations:}
  set ascSolvTableVect(1,0)  { Active:}
  set ascSolvTableVect(2,0)  {  included equalities:}
  set ascSolvTableVect(3,0)  {  included inequalities:}
  set ascSolvTableVect(6,0)  {  Inactive:}
  set ascSolvTableVect(7,0)  { included equalities:}
  set ascSolvTableVect(8,0)  { included inequalities:}
  set ascSolvTableVect(9,0)  {Unincluded:}
  set ascSolvTableVect(11,0) {Objective:}
  set ascSolvTableVect(13,0) {Error:}

  set ascSolvTableVect(0,2)  {Variables:}
  set ascSolvTableVect(1,2)  {Active:}
  set ascSolvTableVect(2,2)  {  free:}
  set ascSolvTableVect(3,2)  {  fixed:}
  set ascSolvTableVect(4,2)  {State:}
  set ascSolvTableVect(6,2)  {Inactive:}
  set ascSolvTableVect(7,2)  { free:}
  set ascSolvTableVect(8,2)  { fixed:}
  set ascSolvTableVect(9,2)  {Unattached:}
  set ascSolvTableVect(11,2) {Blocks:}
  set ascSolvTableVect(12,2) {Iterations:}
  set ascSolvTableVect(13,2) {Current block:}
  set ascSolvTableVect(14,2) {Solved variables:}
}

# proc EndSrc.solver {}
proc EndSrc.solver {} {
# solver startup exit
# this is getting messy enought that it ought to go to SovlerProc
  global ascSolvVect
  if {$ascSolvVect(initialstate)!="iconic" && \
      $ascSolvVect(initialstate)!="iconified" && \
      $ascSolvVect(initialstate) !="withdrawn"} {
    wm deiconify .solver
  }
  set ascSolvVect(statTable) .solver.text_frm.text2.statTable
  set ascSolvVect(statButton) .solver.text_frm.text2.statbutton
  set ascSolvVect(textBox) .solver.text_frm.text2
  set ascSolvVect(oldwidth) 0
  bind .solver.text_frm.text2 <Configure> {
    Solv_Update_Stattable
  }
  if {"$ascSolvVect(initialstate)"=="withdrawn"} {
    wm withdraw .solver
  }
  # bindings on the toplevel descend to all subwidgets. beware? be grateful?
  # bind  .solver <Configure> { puts %W }
}


# User defined procedures



# Internal procedures

# eof
#
