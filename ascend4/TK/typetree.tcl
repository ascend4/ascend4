#  typetree.tcl
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.7 $
#  Last modified on: $Date: 1998/06/18 15:55:43 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: typetree.tcl,v $
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

# Program: typetree.tcl
# Tcl version: 7.1 (Tcl/Tk/XF)
# Tk version: 3.4
# XF version: 2.2
#

# module inclusion
global env

# procedure to show window .typetree
proc VShowWindow.typetree {args} {

  # build widget .typetree
  if {"[info procs XFEdit]" != ""} {
    catch "XFDestroy .typetree"
  } {
    catch "destroy .typetree"
  }
  toplevel .typetree

  # Window manager configurations
  global ascTypeTreeVect
  wm positionfrom .typetree user
  wm sizefrom .typetree user
  wm maxsize .typetree 1152 900
  wm minsize .typetree 100 100
  wm geometry .typetree $ascTypeTreeVect(geometry)
  wm title .typetree {A4 Type refinements}
  wm protocol .typetree WM_DELETE_WINDOW {Type_do_OK}

  # build widget .typetree.can
  frame .typetree.can \
    -borderwidth {1} \
    -relief {raised}

  # build widget .typetree.can.scrollbar1
  scrollbar .typetree.can.scrollbar1 \
    -command {.typetree.can.canvas2 yview}

  # build widget .typetree.can.scrollbar3
  scrollbar .typetree.can.scrollbar3 \
    -command {.typetree.can.canvas2 xview} \
    -orient {horizontal}

  # build widget .typetree.can.canvas2
  canvas .typetree.can.canvas2 \
    -borderwidth {0} \
    -height {10} \
    -insertofftime {600} \
    -scrollregion {0c 0c 20c 20c} \
    -width {80} \
    -xscrollcommand {.typetree.can.scrollbar3 set} \
    -yscrollcommand {.typetree.can.scrollbar1 set}
  # bindings
  bind .typetree.can.canvas2 <Key-A> {puts "atoms menu"}

  # pack widget .typetree.can
  pack append .typetree.can \
    .typetree.can.scrollbar1 {right frame center filly} \
    .typetree.can.canvas2 {top frame center expand fill} \
    .typetree.can.scrollbar3 {top frame center fillx}

  # build widget .typetree.curtype_frm
  frame .typetree.curtype_frm \
    -relief {raised}

  # build widget .typetree.curtype_frm.entry5
  entry .typetree.curtype_frm.entry5 \
    -exportselection {0} \
    -relief {raised} \
    -state disabled \
    -textvariable {ascTypeTreeVect(curtype)}

  # build widget .typetree.curtype_frm.label4
  label .typetree.curtype_frm.label4 \
    -borderwidth {0} \
    -padx {2} \
    -text {Current Type:}

  # build widget .typetree.curtype_frm.ok_btn
  button .typetree.curtype_frm.ok_btn \
    -borderwidth {0} \
    -height {1} \
    -command  Type_do_OK \
    -text { OK }

  button .typetree.curtype_frm.print_btn \
    -borderwidth {0} \
    -height {1} \
    -command  Type_do_Print \
    -text {Print }

  # pack widget .typetree.curtype_frm
  pack append .typetree.curtype_frm \
    .typetree.curtype_frm.label4 {left frame center} \
    .typetree.curtype_frm.entry5 {left frame center expand fill} \
    .typetree.curtype_frm.ok_btn  {right frame center} \
    .typetree.curtype_frm.print_btn  {right frame center}

  # build widget .typetree.lbl_frm
  frame .typetree.lbl_frm \
    -borderwidth {4} \
    -height {1} \
    -relief {groove}

  # build widget .typetree.lbl_frm.label
  label .typetree.lbl_frm.label \
    -borderwidth {0} \
    -height {1} \
    -text {Type Refinement Hierarchy}

  # pack widget .typetree.lbl_frm
  pack append .typetree.lbl_frm \
    .typetree.lbl_frm.label {top frame center}

  # build widget .typetree.mb_frm
  frame .typetree.mb_frm \
    -borderwidth {2} \
    -height {1} \
    -relief {raised}

  # build widget .typetree.mb_frm.atoms_btn
  button .typetree.mb_frm.atoms_btn \
    -height {1} \
    -borderwidth 0 \
    -command Type_do_Atoms \
    -text {Atoms }

  # build widget .typetree.mb_frm.code_btn
  button .typetree.mb_frm.code_btn \
    -height {1} \
    -borderwidth 0 \
    -command Type_do_Code \
    -text {Code }

  # build widget .typetree.mb_frm.help_btn
  button .typetree.mb_frm.help_btn \
    -borderwidth {0} \
    -height {1} \
    -command Type_do_Help \
    -text {Help}

  # build widget .typetree.mb_frm.roots_btn
  button .typetree.mb_frm.roots_btn \
    -borderwidth {0} \
    -height {1} \
    -command Type_do_Roots \
    -text {Roots }

  # build widget .typetree.mb_frm.back_btn
  button .typetree.mb_frm.back_btn \
    -borderwidth {0} \
    -height {1} \
    -command Type_do_Back \
    -text {<<< }

  # build widget .typetree.mb_frm.parts_btn
  button .typetree.mb_frm.parts_btn \
    -height {1} \
    -borderwidth 0 \
    -command Type_do_Parts \
    -text {Parts}

  # pack widget .typetree.mb_frm
  pack append .typetree.mb_frm \
    .typetree.mb_frm.atoms_btn {left frame center} \
    .typetree.mb_frm.code_btn {left frame center} \
    .typetree.mb_frm.parts_btn {left frame center} \
    .typetree.mb_frm.help_btn {right frame center} \
    .typetree.mb_frm.roots_btn {right frame center} \
    .typetree.mb_frm.back_btn {right frame center}

  # pack widget .typetree
  pack append .typetree \
    .typetree.lbl_frm {top frame center fill} \
    .typetree.mb_frm {top frame center fill} \
    .typetree.curtype_frm {top frame center fill} \
    .typetree.can {top frame center expand fill}

  if {"[info procs XFEdit]" != ""} {
    catch "XFMiscBindWidgetTree .typetree"
    after 2 "catch {XFEditSetShowWindows}"
  }
}

proc DestroyWindow.typetree {} {# xf ignore me 7
  if {"[info procs XFEdit]" != ""} {
    if {"[info commands .typetree]" != ""} {
      global xfShowWindow.typetree
      set xfShowWindow.typetree 0
      XFEditSetPath .
      after 2 "XFSaveAsProc .typetree; XFEditSetShowWindows"
    }
  } {
    catch "destroy .typetree"
    update
  }
}


# eof
#

