#  callback.tcl
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.5 $
#  Last modified on: $Date: 1998/06/18 15:55:22 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: callback.tcl,v $
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

# Module: callback.tcl
# Tcl version: 8.0 (Tcl/Tk/XF)
# Tk version: 8.0
# XF version: 4.0
#

# module contents
global moduleList
global autoLoadList
set moduleList(callback.tcl) { .callback}
set autoLoadList(callback.tcl) {0}

# procedures to show toplevel windows


# procedure to show window .$t
proc VShowWindow.callback {args} {# xf ignore me 7

  set t callback
  # build widget .$t
  catch {destroy .$t}
  toplevel .$t 

  # Window manager configurations
  wm positionfrom .$t ""
  wm sizefrom .$t ""
  wm maxsize .$t 1137 870
  wm minsize .$t 1 1
  wm protocol .$t WM_DELETE_WINDOW "destroy $t"
  wm title .$t {A4 Tcl-C callbacks}


  # build widget .$t.grp_frm
  frame .$t.grp_frm \
    -background {red} \
    -borderwidth 0

  # build widget .$t.grp_frm.com_list
  frame .$t.grp_frm.com_list

  set ascCallbackVect(grouplabel) "Command groups:"
  # build widget .$t.grp_frm.com_list.label
  label .$t.grp_frm.com_list.label \
    -textvariable ascCallbackVect(grouplabel)

  # build widget .$t.grp_frm.com_list.scrollbar3
  scrollbar .$t.grp_frm.com_list.scrollbar3 \
    -command ".$t.grp_frm.com_list.listbox1 xview" \
    -orient {horizontal} \
    -relief flat

  # build widget .$t.grp_frm.com_list.scrollbar2
  scrollbar .$t.grp_frm.com_list.scrollbar2 \
    -command ".$t.grp_frm.com_list.listbox1 yview" \
    -relief flat

  # build widget .$t.grp_frm.com_list.listbox1
  listbox .$t.grp_frm.com_list.listbox1 \
    -relief flat \
    -exportselection 0 \
    -xscrollcommand ".$t.grp_frm.com_list.scrollbar3 set" \
    -yscrollcommand ".$t.grp_frm.com_list.scrollbar2 set"

  # build widget .$t.grp_frm.com_expl
  frame .$t.grp_frm.com_expl \
    -relief flat

  set ascCallbackVect(purposelabel) "Group purpose:"

  # build widget .$t.grp_frm.com_expl.label
  label .$t.grp_frm.com_expl.label \
    -textvariable ascCallbackVect(purposelabel)

  # build widget .$t.grp_frm.com_expl.scrollbar1
  scrollbar .$t.grp_frm.com_expl.scrollbar1 \
    -command ".$t.grp_frm.com_expl.text2 yview" \
    -relief flat

  # build widget .$t.grp_frm.com_expl.text2
  text .$t.grp_frm.com_expl.text2 \
    -exportselection 0 \
    -height 2 \
    -relief flat \
    -width 2 \
    -wrap word \
    -yscrollcommand ".$t.grp_frm.com_expl.scrollbar1 set"

  # build widget .$t.com_frm
  frame .$t.com_frm \
    -borderwidth 0

  # build widget .$t.com_frm.frame
  frame .$t.com_frm.frame

  set ascCallbackVect(listlabel) "Commands:"
  # build widget .$t.com_frm.frame.label
  label .$t.com_frm.frame.label \
    -textvariable ascCallbackVect(listlabel)

  # build widget .$t.com_frm.frame.scrollbar3
  scrollbar .$t.com_frm.frame.scrollbar3 \
    -command ".$t.com_frm.frame.listbox1 xview" \
    -orient {horizontal} \
    -relief flat

  # build widget .$t.com_frm.frame.scrollbar2
  scrollbar .$t.com_frm.frame.scrollbar2 \
    -command ".$t.com_frm.frame.listbox1 yview" \
    -relief flat

  # build widget .$t.com_frm.frame.listbox1
  listbox .$t.com_frm.frame.listbox1 \
    -exportselection 0 \
    -relief flat \
    -xscrollcommand ".$t.com_frm.frame.scrollbar3 set" \
    -yscrollcommand ".$t.com_frm.frame.scrollbar2 set"

  # build widget .$t.com_frm.frame11
  frame .$t.com_frm.frame11 \
    -relief flat

  # build widget .$t.com_frm.frame11.scrollbar1
  scrollbar .$t.com_frm.frame11.scrollbar1 \
    -command ".$t.com_frm.frame11.text2 yview" \
    -relief flat

  # build widget .$t.com_frm.frame11.text2
  text .$t.com_frm.frame11.text2 \
    -exportselection 0 \
    -relief flat \
    -wrap word \
    -yscrollcommand ".$t.com_frm.frame11.scrollbar1 set"

  # build widget .$t.frame0
  frame .$t.frame0 \
    -borderwidth 0 \
    -relief flat

  # build widget .$t.menubar
  menu .$t.menubar
  
  # The File menu
  menu .$t.menubar.file \
    -tearoffcommand .[string toupper $t].MENUBAR.FILE \
    -tearoff 0
  .$t.menubar.file add command \
    -command {Callback_do_Print} \
    -label {Print ...} \
    -underline 0
  .$t.menubar.file add separator
  .$t.menubar.file add command \
    -command {Callback_do_Close} \
    -label {Close window} \
    -underline 0

  # The Find menu
  menu .$t.menubar.search \
    -tearoffcommand .[string toupper $t].MENUBAR.SEARCH \
    -tearoff 0
  .$t.menubar.search add command \
    -command {Callback_do_wordsearch} \
    -state disabled \
    -label {Keywords ...} \
    -underline 0
  .$t.menubar.search add command \
    -command {Callback_do_usagesearch} \
    -state disabled \
    -label {Short explanations ...} \
    -underline 0
  .$t.menubar.search add command \
    -command {Callback_do_textsearch} \
    -state disabled \
    -label {Long explanations ...} \
    -underline 0
  .$t.menubar.search add command \
    -command {Callback_do_command} \
    -state disabled \
    -label {Name of command ...} \
    -underline 0

  # The View menu
  menu .$t.menubar.view \
    -tearoffcommand .[string toupper $t].MENUBAR.VIEW \
    -tearoff 0
  # the group list cascade under view.
  menu .$t.menubar.view.groups \
    -tearoffcommand .[string toupper $t].MENUBAR.VIEW.GROUPS \
    -tearoff 0
  .$t.menubar.view add command \
    -command {Callback_do_All} \
    -label {All callbacks} \
    -underline 0
  .$t.menubar.view add command \
    -command {Callback_do_Groups} \
    -label {Group of callbacks} \
    -underline 0
  .$t.menubar.view add command \
    -command {Callback_do_Arrays} \
    -label {Global arrays} \
    -underline 7
  .$t.menubar.view add separator
  .$t.menubar.view add command \
    -command {Callback_do_Font} \
    -label {Font ...} \
    -underline 0

  # The Help menu
  menu .$t.menubar.help \
    -tearoffcommand .[string toupper $t].MENUBAR.HELP \
    -tearoff 0
  .$t.menubar.help add command \
    -command {Callback_do_About} \
    -label {About ASCEND Callbacks} \
    -underline 0
  .$t.menubar.help add command \
    -command {Callback_do_syntax} \
    -label {Callback options} \
    -underline 0
  .$t.menubar.help add command \
    -command {Callback_do_TOC} \
    -label {Table of contents} \
    -underline 0

  #
  # Add the menus as cascades of the toplevel's menu;
  # add the toplevel's menu to the toplevel
  #
  .$t.menubar add cascade \
    -menu .$t.menubar.file \
    -label {File} \
    -underline 0
  .$t.menubar add cascade \
    -menu .$t.menubar.search \
    -label {Find} \
    -underline 0
  .$t.menubar add cascade \
    -menu .$t.menubar.view \
    -label {View} \
    -underline 0
  .$t.menubar add cascade \
    -menu .$t.menubar.help \
    -label {Help} \
    -underline 0
  .$t configure \
    -menu .$t.menubar

  # pack master .$t.grp_frm
  pack configure .$t.grp_frm.com_list \
    -expand 1 \
    -fill both \
    -side left
  pack configure .$t.grp_frm.com_expl \
    -expand 1 \
    -fill both \
    -side left

  # pack master .$t.grp_frm.com_list
  pack configure .$t.grp_frm.com_list.label \
    -fill x \
    -side top
  pack configure .$t.grp_frm.com_list.scrollbar2 \
    -fill y \
    -side left
  pack configure .$t.grp_frm.com_list.listbox1 \
    -expand 1 \
    -fill both
  pack configure .$t.grp_frm.com_list.scrollbar3 \
    -fill x \
    -side bottom

  # pack master .$t.grp_frm.com_expl
  pack configure .$t.grp_frm.com_expl.label \
    -fill x \
    -side top
  pack configure .$t.grp_frm.com_expl.scrollbar1 \
    -fill y \
    -side right
  pack configure .$t.grp_frm.com_expl.text2 \
    -expand 1 \
    -fill both

  # pack master .$t.com_frm
  pack configure .$t.com_frm.frame \
    -expand 1 \
    -fill both \
    -side left
  pack configure .$t.com_frm.frame11 \
    -expand 1 \
    -fill both \
    -side left

  # pack master .$t.com_frm.frame
  pack configure .$t.com_frm.frame.label \
    -fill x \
    -side top
  pack configure .$t.com_frm.frame.scrollbar2 \
    -fill y \
    -side left
  pack configure .$t.com_frm.frame.listbox1 \
    -expand 1 \
    -fill both
  pack configure .$t.com_frm.frame.scrollbar3 \
    -fill x \
    -side bottom

  # pack master .$t.com_frm.frame11
  pack configure .$t.com_frm.frame11.scrollbar1 \
    -fill y \
    -side right
  pack configure .$t.com_frm.frame11.text2 \
    -expand 1 \
    -fill both

  pack configure .$t.grp_frm \
    -expand 1 \
    -fill both
  pack configure .$t.com_frm \
    -expand 1 \
    -fill both

  .$t.grp_frm.com_list.listbox1 insert end {list of groups}
  .$t.grp_frm.com_expl.text2 insert end {explanation of selected group}
  .$t.com_frm.frame.listbox1 insert end {list of commands}
  .$t.com_frm.frame11.text2 insert end {explanation of command selected at left}


  EndSrc.callback
}

proc DestroyWindow.callback {} {# xf ignore me 7
  catch "destroy .callback"
}


# User defined procedures

proc EndSrc.callback {} {
  Callback_Update
}

# User defined images


# Internal procedures

# eof
#

