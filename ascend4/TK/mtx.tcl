#  mtx.tcl
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.10 $
#  Last modified on: $Date: 1998/06/18 15:55:33 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: mtx.tcl,v $
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

# Module: mtx.tcl
# Tcl version: 7.1 (Tcl/Tk/XF)
# Tk version: 3.4
# XF version: 2.2
#

# module contents
global moduleList
global autoLoadList
set moduleList(mtx.tcl) { ShowWindow.mtx .mtx}
set autoLoadList(mtx.tcl) {0}

# procedures to show toplevel windows


# procedure to show window .mtx
# proc ShowWindow.mtx {args}
proc ShowWindow.mtx {args} {# xf ignore me 7

  global ascMtxVect ascGlobalVect ascSolvVect
  StartupSrc.mtx

  # build widget .mtx
  if {"[info procs XFEdit]" != ""} {
    catch "XFDestroy .mtx"
  } {
    catch "destroy .mtx"
  }
  toplevel .mtx

  # Window manager configurations
  wm positionfrom .mtx user
  wm sizefrom .mtx ""
  wm maxsize .mtx 1152 900
  wm title .mtx {MtxView Incidence}
  wm geometry .mtx $ascMtxVect(geometry)
  wm protocol .mtx WM_DELETE_WINDOW {Mtx_do_OK}


  # build widget .mtx.btn_mtx_red
  button .mtx.btn_mtx_red \
    -command {Mtx_do_Redraw} \
    -font $ascMtxVect(font) \
    -text { Redraw }

  # build widget .mtx.btn_ok
  label .mtx.btn_ok \
    -font $ascMtxVect(font) \
    -text { Magnification: }

  # build widget .mtx.can_mtx
  frame .mtx.can_mtx

  # build widget .mtx.can_mtx.scrollbar1
  scrollbar .mtx.can_mtx.scrollbar1 \
    -command {.mtx.can_mtx.canvas2 yview}

  # build widget .mtx.can_mtx.scrollbar3
  scrollbar .mtx.can_mtx.scrollbar3 \
    -command {.mtx.can_mtx.canvas2 xview} \
    -orient {horizontal}

  # build widget .mtx.can_mtx.canvas2
  canvas .mtx.can_mtx.canvas2 \
    -height {207} \
    -insertofftime {600} \
    -scrollregion {0c 0c 20c 20c} \
    -width {295} \
    -xscrollcommand {.mtx.can_mtx.scrollbar3 set} \
    -yscrollcommand {.mtx.can_mtx.scrollbar1 set}

  # pack widget .mtx.can_mtx
  pack append .mtx.can_mtx \
    .mtx.can_mtx.scrollbar1 {right frame center filly} \
    .mtx.can_mtx.canvas2 {top frame center expand fill} \
    .mtx.can_mtx.scrollbar3 {top frame center fillx}

  # build widget .mtx.col
  frame .mtx.col \
    -relief {raised}

  # build widget .mtx.col.entry5
  entry .mtx.col.entry5 \
    -font $ascMtxVect(font) \
    -exportselection {0} \
    -textvariable {ascMtxVect(varnum)}

  # build widget .mtx.col.label4
  label .mtx.col.label4 \
    -font $ascMtxVect(font) \
    -width 10 \
    -text {Variable:}

  # pack widget .mtx.col
  pack append .mtx.col \
    .mtx.col.label4 {left frame center} \
    .mtx.col.entry5 {top frame center expand fill}

  # build widget .mtx.eqn
  frame .mtx.eqn \
    -relief {raised}

  # build widget .mtx.eqn.entry5
  entry .mtx.eqn.entry5 \
    -font $ascMtxVect(font) \
    -exportselection {0} \
    -textvariable {ascMtxVect(eqnname)}

  # build widget .mtx.eqn.label4
  label .mtx.eqn.label4 \
    -font $ascMtxVect(font) \
    -width 10 \
    -text {Eqn Name:}

  # pack widget .mtx.eqn
  pack append .mtx.eqn \
    .mtx.eqn.label4 {left frame center} \
    .mtx.eqn.entry5 {top frame center expand fill}

  # build widget .mtx.row
  frame .mtx.row \
    -relief {raised}

  # build widget .mtx.row.entry5
  entry .mtx.row.entry5 \
    -font $ascMtxVect(font) \
    -exportselection {0} \
    -textvariable {ascMtxVect(eqnnum)}

  # build widget .mtx.row.label4
  label .mtx.row.label4 \
    -font $ascMtxVect(font) \
    -width 10 \
    -text {Equation:}

  # pack widget .mtx.row
  pack append .mtx.row \
    .mtx.row.label4 {left frame center} \
    .mtx.row.entry5 {top frame center expand fill}

  # build widget .mtx.var
  frame .mtx.var

  # build widget .mtx.var.entry5
  entry .mtx.var.entry5 \
    -font $ascMtxVect(font) \
    -exportselection {0} \
    -textvariable {ascMtxVect(varname)}

  # build widget .mtx.var.label4
  label .mtx.var.label4 \
    -font $ascMtxVect(font) \
    -width 10 \
    -text {Var Name:}

  # pack widget .mtx.var
  pack append .mtx.var \
    .mtx.var.label4 {left frame center} \
    .mtx.var.entry5 {top frame center expand fill}

  # build widget .mtx.blk
  frame .mtx.blk \
    -relief {raised}

  # build widget .mtx.blk.entry5
  entry .mtx.blk.entry5 \
    -font $ascMtxVect(font) \
    -exportselection {0} \
    -width 8 \
    -textvariable {ascMtxVect(blknumber)}

  # build widget .mtx.blk.label4
  label .mtx.blk.label4 \
    -font $ascMtxVect(font) \
    -width 10 \
    -text {Block:}

  # build widget .mtx.blk.printblk_btn
  button .mtx.blk.printblk_btn \
    -command {Mtx_do_PrintBlock} \
    -font $ascMtxVect(font) \
    -text {PrintBlock }

  # build widget .mtx.blk.print_btn
  button .mtx.blk.print_btn \
    -command {Mtx_do_Print} \
    -font $ascMtxVect(font) \
    -text {Print }

  # pack widget .mtx.blk
  pack append .mtx.blk \
    .mtx.blk.label4 {left frame center} \
    .mtx.blk.entry5 {left frame center expand fill}
#    .mtx.blk.print_btn {right frame center expand fill} \
    .mtx.blk.printblk_btn {right frame center expand fill}

  # build widget .mtx.zoom
  scale .mtx.zoom \
    -from {1} \
    -orient {horizontal} \
    -to {14}

  #
  #  Build the menubar and the menus
  #
  menu .mtx.menubar \
    -tearoffcommand .MTX.MENUBAR \
    -tearoff 0

  # The File menu
  menu .mtx.menubar.file \
    -tearoffcommand .MTX.MENUBAR.FILE \
    -tearoff 0
  .mtx.menubar.file add command \
    -command {Mtx_do_Print} \
    -label {Print...} \
    -underline 0
  .mtx.menubar.file add command \
    -command {Mtx_do_PrintBlock} \
    -label {Print block...} \
    -underline 6
  .mtx.menubar.file add separator
  .mtx.menubar.file add command \
    -command {Mtx_do_OK} \
    -label {Close window} \
    -underline 0

  # The Help menu
  menu .mtx.menubar.help \
    -tearoffcommand .MTX.MENUBAR.HELP \
    -tearoff 0
  .mtx.menubar.help add command \
    -command {Mtx_do_Help} \
    -label {On Mtx View Incidence} \
    -underline 3

  # The View menu
  menu .mtx.menubar.view \
    -tearoffcommand .MTX.MENUBAR.VIEW \
    -tearoff 0
  .mtx.menubar.view add command \
    -command {Mtx_do_Font} \
    -label {Font...} \
    -underline 0
  

  # Add the menus to the menubar and the menubar to the toplebel
  .mtx.menubar add cascade \
    -menu .mtx.menubar.file \
    -label {File} \
    -underline 0
  .mtx.menubar add cascade \
    -menu .mtx.menubar.view \
    -label {View} \
    -underline 0
  .mtx.menubar add cascade \
    -menu .mtx.menubar.help \
    -label {Help} \
    -underline 0
  .mtx configure \
    -menu .mtx.menubar

  # pack widget .mtx
  pack append .mtx \
    .mtx.col {top frame center fill} \
    .mtx.var {top frame center fillx} \
    .mtx.row {top frame center fill} \
    .mtx.eqn {top frame center fill} \
    .mtx.blk {top frame center fillx} \
    .mtx.can_mtx {top frame center expand fill} \
    .mtx.btn_mtx_red {right frame center fill} \
    .mtx.zoom {right frame center expand fillx} \
    .mtx.btn_ok {right frame center fill}

  # build canvas items .mtx.can_mtx.canvas2

  EndSrc.mtx

  if {"[info procs XFEdit]" != ""} {
    catch "XFMiscBindWidgetTree .mtx"
    after 2 "catch {XFEditSetShowWindows}"
  }
}

# proc DestroyWindow.mtx {}
proc DestroyWindow.mtx {} {# xf ignore me 7
  if {"[info procs XFEdit]" != ""} {
    if {"[info commands .mtx]" != ""} {
      global xfShowWindow.mtx
      set xfShowWindow.mtx 0
      XFEditSetPath .
      after 2 "XFSaveAsProc .mtx; XFEditSetShowWindows"
    }
  } {
    catch "destroy .mtx"
    update
  }
}

# proc StartupSrc.mtx {args}
proc StartupSrc.mtx {args} {
  # mtx startup entrance
  global ascMtxVect ascSolvVect
  if {[info exists  ascMtxVect(font)]} {
    return
  }
  if {[ catch {set ascMtxVect(font) $ascSolvVect(font)} ]} {
    set ascMtxVect(font) "-*-*"
  }
}

# proc EndSrc.mtx {}
proc EndSrc.mtx {} {
# mtx startup exit
global ascMtxVect
# if {$ascMtxVect(initialstate)!="uniconified"} {wm iconify .mtx}
}


# User defined procedures


# Internal procedures

# eof
#

