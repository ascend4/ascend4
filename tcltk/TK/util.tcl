#  util.tcl: utility window code for ascend
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.10 $
#  Last modified on: $Date: 1998/06/18 15:55:47 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: util.tcl,v $
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

#
# proc VShowWindow.util {args}
#------------------------------------------------------------------------
# procedure to show window .util
# builds a line for every thing in utilvect
# but in sets of ascglobalbect(maxutilrow)
# pages are numbered from 0 internally
# if a line is called for without the variable
# set first, this will set it to something dumb but displayable
#------------------------------------------------------------------------
proc VShowWindow.util {args} {

  global ascUtilVect ascGlobalVect ascScripVect
  Util_Init
  set rows [array size ascUtilVect]
  if {$rows ==0 } {return}
  set lblwid [Util_Label_Width]
  # build widget .util
  if {[info commands .util] != ""} {set utilup 1} {set utilup 0}

  set argc [llength $args]
  if {$argc==1} {
    set nrows $ascGlobalVect(maxutilrow)
    set pagenum $args
    set maxpage [expr [llength [array names ascUtilVect]] / $nrows]
    if {[expr $rows % $nrows] ==0} { incr maxpage -1}
    if {$maxpage==0} {set nomore 1} {set nomore 0}
  }  else {error "VShowWindow.util wants 1 page num arg"}

  if {!$utilup} {
    # Window manager configurations
    toplevel .util
    wm positionfrom .util user
    wm sizefrom .util user
    wm minsize .util 10 10
    wm protocol .util WM_DELETE_WINDOW {Util_do_OK}
    wm title .util {A4 Utilities}


    # build widget .util.box
    frame .util.box \
      -borderwidth {0} \
      -relief {raised}

    # build widget .util.box.label
    label .util.box.label \
      -font $ascGlobalVect(labelfont) \
      -borderwidth {5} \
      -relief {groove} \
      -text {ASCEND Utility Settings}

    # build widget .util.box.main_frm
    frame .util.box.main_frm \
      -borderwidth {2} \
      -relief {raised}

    # build widget .util.box.main_frm.lbl_frm
    frame .util.box.main_frm.lbl_frm \
      -relief {raised}

    # build widget .util.box.main_frm.val_frm
    frame .util.box.main_frm.val_frm \
      -relief {raised}
  }

  if {[expr $pagenum >= $maxpage]} {
    set nextpage 0
    set pagenum $maxpage
  } else {
    set nextpage [expr $pagenum +1]
  }
  set subs [lsort [array names ascUtilVect]]
  set first [expr $pagenum * $nrows]
  set last [expr $first + $nrows -1]
  set psubs [lrange $subs $first $last]

# forget anyone who's packed now in val_frm or lbl_frm
  set slaves "[winfo children .util.box.main_frm.lbl_frm]"
  append slaves " [winfo children .util.box.main_frm.val_frm]"
  foreach s $slaves {pack forget $s}
# build all the entries, if not out there, and pack, if on current page
  foreach s $subs {
#
# init unset vars
    set varname [Util_Get_Var $s]
    global $varname
    if {[info var $varname]==""} {
      set $varname "unset"
    }
    if {[info command ".util.box.main_frm.lbl_frm.$s"] ==""} {
# build widget .util.box.main_frm.lbl_frm.$s
      set tmplbl "[Util_Get_Label $s]"
      label .util.box.main_frm.lbl_frm.$s \
        -font $ascScripVect(font) \
        -width "$lblwid" \
        -text "[Util_Get_Label $s]"
      catch {
        set btnpath [string trim $tmplbl]
        set btnpath [string tolower $btnpath]
        set btnpath [join [split $btnpath] .]
        Help_BindButton .util.box.main_frm.lbl_frm.$s utilities $btnpath
      }
# build widget .util.box.main_frm.val_frm.$s
      entry .util.box.main_frm.val_frm.$s \
        -font $ascScripVect(font) \
        -borderwidth {3} \
        -exportselection 0 \
        -relief {groove} \
        -textvariable "$varname"
    }

    if {[lsearch $psubs $s] != "-1"} {
# pack widget .util.box.main_frm.lbl_frm, if displayed on this page
      pack append .util.box.main_frm.lbl_frm \
        .util.box.main_frm.lbl_frm.$s {top frame center expand fill}
# pack widget .util.box.main_frm.val_frm if displayed on this page
      pack append .util.box.main_frm.val_frm \
        .util.box.main_frm.val_frm.$s {top frame center expand fill}
    }
  }
# pack rest if not done already
  if {!$utilup} {
    # pack widget .util.box.main_frm
    pack append .util.box.main_frm \
      .util.box.main_frm.lbl_frm {left frame center fill} \
      .util.box.main_frm.val_frm {left frame center expand fill}

    # build widget .util.box.mb_frm
    frame .util.box.mb_frm \
      -relief {raised}

    # build widget .util.box.mb_frm.btn_hlp
    button .util.box.mb_frm.btn_hlp \
      -font $ascScripVect(font) \
      -command {Util_do_Help} \
      -text { Help }

    # build widget .util.box.mb_frm.btn_ok
    button .util.box.mb_frm.btn_ok \
      -font $ascScripVect(font) \
      -command {Util_do_OK} \
      -text {  OK  }

    # build widget .util.box.mb_frm.btn_read
    button .util.box.mb_frm.btn_read \
      -font $ascScripVect(font) \
      -command {Util_do_Read} \
      -text { Read }

    # build widget .util.box.mb_frm.btn_save
    button .util.box.mb_frm.btn_save \
      -font $ascScripVect(font) \
      -command {Util_do_Save} \
      -text { Save }

    # build widget .util.box.mb_frm.btn_more
    if {!$nomore} {
      button .util.box.mb_frm.btn_more \
        -font $ascScripVect(font) \
        -command "VShowWindow.util $nextpage" \
        -text { More }

    # pack widget .util.box.mb_frm
      pack append .util.box.mb_frm \
        .util.box.mb_frm.btn_ok {left frame center expand fill} \
        .util.box.mb_frm.btn_save {left frame center expand fill} \
        .util.box.mb_frm.btn_read {left frame center expand fill} \
        .util.box.mb_frm.btn_more {left frame center expand fill} \
        .util.box.mb_frm.btn_hlp {left frame center expand fill}
    } else {
      pack append .util.box.mb_frm \
        .util.box.mb_frm.btn_ok {left frame center expand fill} \
        .util.box.mb_frm.btn_save {left frame center expand fill} \
        .util.box.mb_frm.btn_read {left frame center expand fill} \
        .util.box.mb_frm.btn_hlp {left frame center expand fill}
    }
    # pack widget .util.box
    pack append .util.box \
      .util.box.label {top frame center fillx} \
      .util.box.mb_frm {bottom frame center fill} \
      .util.box.main_frm {top frame center expand fill}

    # pack widget .util
    pack append .util \
      .util.box {top frame center expand fill}

    if {"[info procs XFEdit]" != ""} {
      catch "XFMiscBindWidgetTree .util"
      after 2 "catch {XFEditSetShowWindows}"
    }
  } else {
    if {!$nomore} {
      .util.box.mb_frm.btn_more config -command "VShowWindow.util $nextpage"
    }
  }
}

# proc DestroyWindow.util {}
proc DestroyWindow.util {} {# xf ignore me 7
  if {"[info procs XFEdit]" != ""} {
    if {"[info commands .util]" != ""} {
      global xfShowWindow.util
      set xfShowWindow.util 0
      XFEditSetPath .
      after 2 "XFSaveAsProc .util; XFEditSetShowWindows"
    }
  } {
    catch "destroy .util"
    update
  }
}

