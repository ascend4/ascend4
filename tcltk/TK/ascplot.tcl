#  ascplot.tcl: a columnar data manipulator
#  by Benjamin Allan
#  August 1995
#  Part of ASCEND
#  Revision: $Revision: 1.20 $
#  Last modified on: $Date: 2003/02/06 13:49:43 $
#  Last modified by: $Author: ballan $
#  Revision control file: $RCSfile: ascplot.tcl,v $
#
#  This file is part of the ASCEND Tcl/Tk Interface.
#
#  Copyright (C) 1995-1998 Carnegie Mellon University
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

# Module: ascplot.tcl
# Tcl version: 7.1 (Tcl/Tk/XF)
# Tk version: 3.4
# XF version: 2.2
#
# This file requires the tkTable extension by Roland King,
# Copyright (c) 1994 by Roland King. This has been included in the
# ASCEND guiadd sources.
# Anyone may use and modify this code so long as this notice is
# kept intact. Bug reports to ballan@cs.cmu.edu.
# This code is used in the ASCEND project at CMU and as such
# can be considered ~75% supported.
#
# The idea here is that we can take ASCII numeric data files
# in a columnar format and feed various bizarre x/unix plot
# programs. If there is direct access to a decent spreadsheet
# or real plot package, this isn't needed. Limited spreadsheetlike
# functionality is supported.

# module contents
global moduleList
global autoLoadList
set moduleList(ascplot.tcl) { .ascplot}
set autoLoadList(ascplot.tcl) {0}

# procedures to show toplevel windows


# procedure to show window .ascplot
global ascplotvect
set ascplotvect(dummyvar) apv.dummy
proc ShowWindow.ascplot {args} {# xf ignore me 7

  global ascplotvect

  # build widget .ascplot
  if {"[info procs XFEdit]" != ""} {
    catch "XFDestroy .ascplot"
  } {
    catch "destroy .ascplot"
  }
  toplevel .ascplot 

  # Window manager configurations
  set minw [lindex [split $ascplotvect(minsize) x] 0]
  set minh [lindex [split $ascplotvect(minsize) x] 1]
  set maxw [lindex [split $ascplotvect(maxsize) x] 0]
  set maxh [lindex [split $ascplotvect(maxsize) x] 1]

  wm positionfrom .ascplot ""
  wm sizefrom .ascplot ""
  wm maxsize .ascplot $maxw $maxh
  wm minsize .ascplot $minw $minh
  wm title .ascplot {A4 Plot}
  wm protocol .ascplot WM_DELETE_WINDOW {ascplot_dook}


  # build widget .ascplot.top_frm
  frame .ascplot.top_frm
  frame .ascplot.bot_frm

  # build widget .ascplot.top_frm.datasets
  frame .ascplot.top_frm.datasets

  # build widget .ascplot.top_frm.datasets.scrollbar2
  scrollbar .ascplot.top_frm.datasets.scrollbar2 \
    -command {.ascplot.top_frm.datasets.listbox1 yview} \
    -relief {raised}

  # build widget .ascplot.top_frm.datasets.scrollbar3
  scrollbar .ascplot.top_frm.datasets.scrollbar3 \
    -command {.ascplot.top_frm.datasets.listbox1 xview} \
    -orient {horizontal} \
    -relief {raised}

  # build widget .ascplot.top_frm.datasets.listbox1
  listbox .ascplot.top_frm.datasets.listbox1 \
    -width {40} \
    -height {2} \
    -relief {raised} \
    -selectmode extended \
    -xscrollcommand {.ascplot.top_frm.datasets.scrollbar3 set} \
    -yscrollcommand {.ascplot.top_frm.datasets.scrollbar2 set} \
    -font $ascplotvect(textfont) 

  # pack widget .ascplot.top_frm.datasets
  pack append .ascplot.top_frm.datasets \
    .ascplot.top_frm.datasets.scrollbar2 {left frame center filly} \
    .ascplot.top_frm.datasets.scrollbar3 {bottom frame center fillx} \
    .ascplot.top_frm.datasets.listbox1 {left frame center expand fill}


  #
  # Build the menu bar and submenus
  #
  menu .ascplot.menubar \
    -tearoffcommand .ASCPLOT.MENUBAR \
    -tearoff 0

  # The Display menu
  menu .ascplot.menubar.display \
    -tearoffcommand .ASCPLOT.MENUBAR.DISPLAY \
    -tearoff 0
  .ascplot.menubar.display add command \
    -command {
      global ascplotvect
      ascplot_showdata $ascplotvect(showdata)
    } \
    -label {Show data} \
    -accelerator {Alt-d s} \
    -underline 0
  .ascplot.menubar.display add separator
  .ascplot.menubar.display add command \
    -command ascplot_defineplot \
    -label {Set plot titles} \
    -accelerator {Alt-d t} \
    -underline 9
  .ascplot.menubar.display add command \
    -command ascplot_loadplot \
    -label {Load old plot} \
    -accelerator {Alt-d l} \
    -underline 0
  .ascplot.menubar.display add command \
    -command ascplot_updateplot \
    -label {Update plot} \
    -accelerator {Alt-d u} \
    -underline 0
  .ascplot.menubar.display add command \
    -command ascplot_deleteplot \
    -label {Delete plot} \
    -accelerator {Alt-d d} \
    -underline 0


  # The Edit menu
  menu .ascplot.menubar.edit \
    -tearoffcommand .ASCPLOT.MENUBAR.EDIT \
    -tearoff 0
  .ascplot.menubar.edit add command \
    -command ascplot_mergedata \
    -accelerator {Alt-e m} \
    -label {Merge data sets} \
    -underline 0


  # the Execute menu
  menu .ascplot.menubar.execute \
    -tearoffcommand .ASCPLOT.MENUBAR.EXECUTE \
    -tearoff 0
  .ascplot.menubar.execute add command \
    -command ascplot_viewgraph \
    -label {View plot file} \
    -accelerator {Alt-x v} \
    -underline 0
  .ascplot.menubar.execute add command \
    -command ascplot_writegraph \
    -label {Write plot file} \
    -accelerator {Alt-x w} \
    -underline 0
  .ascplot.menubar.execute add separator
  .ascplot.menubar.execute add command \
    -command {ascplot_insert col} \
    -label {Insert column} \
    -accelerator {Alt-x c} \
    -underline 7
  .ascplot.menubar.execute add command \
    -command {ascplot_recalc col} \
    -label {Recalculate column}
  .ascplot.menubar.execute add command \
    -command {ascplot_insert row} \
    -label {Insert row} \
    -accelerator {Alt-x r} \
    -underline 7
  .ascplot.menubar.execute add command \
    -command {ascplot_recalc col} \
    -label {Recalculate row}


  # The File menu
  menu .ascplot.menubar.file \
    -tearoffcommand .ASCPLOT.MENUBAR.FILE \
    -tearoff 0
  .ascplot.menubar.file add command \
    -command ascplot_loaddata \
    -label {Read data set} \
    -accelerator {Alt-f r} \
    -underline 0
  .ascplot.menubar.file add command \
    -command ascplot_loaddata \
    -label {Open data set} \
    -accelerator {Alt-f o} \
    -underline 0
  .ascplot.menubar.file add command \
    -command ascplot_savedata \
    -label {Save data set} \
    -accelerator {Alt-f s} \
    -underline 0
  .ascplot.menubar.file add command \
    -command {ascplot_reloaddata 1} \
    -accelerator {Alt-f l} \
    -label {Reload data set} \
    -underline 2
  .ascplot.menubar.file add command \
    -command {ascplot_unloaddata 1} \
    -label {Unload data set} \
    -accelerator {Alt-f u} \
    -underline 0
  .ascplot.menubar.file add separator
  .ascplot.menubar.file add command \
    -command {ascplot_dook} \
    -accelerator {Alt-f c} \
    -label {Close window} \
    -underline 0


  # The Help menu
  menu .ascplot.menubar.help \
    -tearoffcommand .ASCPLOT.MENUBAR.HELP \
    -tearoff 0
  .ascplot.menubar.help add command \
    -command ascplot_sshelp \
    -label {Using the data table}
  .ascplot.menubar.help add command \
    -command ascplot_ssbinds \
    -label {Data table key bindings}
  .ascplot.menubar.help add command \
    -command ascplot_credits \
    -label {Whodunnit}
  .ascplot.menubar.help add command \
    -command ascplot_sources \
    -label {Where to get it}


  # The Options menu
  menu .ascplot.menubar.options \
    -tearoffcommand .ASCPLOT.MENUBAR.OPTIONS \
    -tearoff 0
  .ascplot.menubar.options add command \
    -command ascplot_grill \
    -accelerator {Alt-o o} \
    -label {See options...} \
    -underline 4
  .ascplot.menubar.options add separator
  .ascplot.menubar.options add cascade \
    -menu {.ascplot.menubar.options.graph} \
    -accelerator {Alt-o g} \
    -label {Select grapher} \
    -underline 7

  # the Grapher submenu under the Options menu
  menu .ascplot.menubar.options.graph \
    -tearoffcommand .ASCPLOT.MENUBAR.EDIT.GRAPH \
    -tearoff 0
  .ascplot.menubar.options.graph add command \
    -command "ascplot_selplotter xgraph" \
    -label {Xgraph} \
    -underline 0
  .ascplot.menubar.options.graph add command \
    -command "ascplot_selplotter xmgr" \
    -label {XMGR} \
    -underline 1
  .ascplot.menubar.options.graph add command \
    -command "ascplot_selplotter gnuplot" \
    -label {gnuplot} \
    -underline 0


  #
  # Add the menus as cascades of the toplevel's menu;
  # add the toplevel's menu to the toplevel
  #
  .ascplot.menubar add cascade \
    -menu .ascplot.menubar.file \
    -label {File} \
    -underline 0
  .ascplot.menubar add cascade \
    -menu .ascplot.menubar.edit \
    -label {Edit} \
    -underline 0
  .ascplot.menubar add cascade \
    -menu .ascplot.menubar.execute \
    -label {Execute} \
    -underline 1
  .ascplot.menubar add cascade \
    -menu .ascplot.menubar.display \
    -label {Display} \
    -underline 0
  .ascplot.menubar add cascade \
    -menu .ascplot.menubar.options \
    -label {Options} \
    -underline 0
  .ascplot.menubar add cascade \
    -menu .ascplot.menubar.help \
    -underline 0 \
    -label {Help}
  .ascplot configure \
    -menu .ascplot.menubar


  # build widget .ascplot.bot_frm.message
  frame .ascplot.bot_frm.message

  # build widget .ascplot.bot_frm.message.scrollbar1
  scrollbar .ascplot.bot_frm.message.scrollbar1 \
    -command {.ascplot.bot_frm.message.text2 yview}

  # build widget .ascplot.bot_frm.message.text2
  text .ascplot.bot_frm.message.text2 \
    -exportselection {0} \
    -width {28} \
    -wrap {word} \
    -yscrollcommand {.ascplot.bot_frm.message.scrollbar1 set}

  # bindings
  bind .ascplot.bot_frm.message.text2 <Any-Key> {}

  # pack widget .ascplot.bot_frm.message
  pack append .ascplot.bot_frm.message \
    .ascplot.bot_frm.message.scrollbar1 {left frame center filly} \
    .ascplot.bot_frm.message.text2 {top frame center expand fill}

  # build widget .ascplot.bot_frm.table_frm
  frame .ascplot.bot_frm.table_frm

  # build widget .ascplot.bot_frm.table_frm.datatable
  frame .ascplot.bot_frm.table_frm.datatable

  # build widget .ascplot.bot_frm.table_frm.datatable.scrollbar2
  scrollbar .ascplot.bot_frm.table_frm.datatable.scrollbar2 \
    -command {.ascplot.bot_frm.table_frm.datatable.table1 yview}

  # build widget .ascplot.bot_frm.table_frm.datatable.scrollbar3
  scrollbar .ascplot.bot_frm.table_frm.datatable.scrollbar3 \
    -command {.ascplot.bot_frm.table_frm.datatable.table1 xview} \
    -orient {horizontal}

  # build widget .ascplot.bot_frm.table_frm.datatable.table1
  table .ascplot.bot_frm.table_frm.datatable.table1 \
    -exportselection 0 \
    -titlerows 3 \
    -titlecols 1 \
    -font $ascplotvect(textfont) \
    -roworigin -3 \
    -colorigin -1 \
    -height 0 \
    -width 10 \
    -variable $ascplotvect(dummyvar) \
    -rows 6 \
    -cols 6 \
    -colseparator "\t" \
    -rowseparator "\n" \
    -selectmode extended \
    -colstretch all \
    -rowstretch all \
    -browsecommand {set table(current) %S} \
    -xscrollcommand {.ascplot.bot_frm.table_frm.datatable.scrollbar3 set} \
    -yscrollcommand {.ascplot.bot_frm.table_frm.datatable.scrollbar2 set}
    # -batchmode 1 \

  # pack widget .ascplot.bot_frm.table_frm.datatable
  pack append .ascplot.bot_frm.table_frm.datatable \
    .ascplot.bot_frm.table_frm.datatable.scrollbar3 {top frame center fillx} \
    .ascplot.bot_frm.table_frm.datatable.scrollbar2 {left frame center filly} \
    .ascplot.bot_frm.table_frm.datatable.table1 {left frame center fill}

  # pack widget .ascplot.bot_frm.table_frm
  pack append .ascplot.bot_frm.table_frm \
    .ascplot.bot_frm.table_frm.datatable {top frame center fill}

  # build widget .ascplot.top_frm.varfrm
  frame .ascplot.top_frm.varfrm

  # build widget .ascplot.top_frm.varfrm.varlbl
  frame .ascplot.top_frm.varfrm.varlbl
  label .ascplot.top_frm.varfrm.varlbl.left \
    -font $ascplotvect(btnfont) \
    -text "unused variables"

  label .ascplot.top_frm.varfrm.varlbl.right \
    -font $ascplotvect(btnfont) \
    -text "plotted variables"

  pack append .ascplot.top_frm.varfrm.varlbl \
    .ascplot.top_frm.varfrm.varlbl.left {left frame center expand fill} \
    .ascplot.top_frm.varfrm.varlbl.right {right frame center expand fill}
  # build widget .ascplot.top_frm.varfrm.indepvar
  frame .ascplot.top_frm.varfrm.indepvar

  # build widget .ascplot.top_frm.varfrm.indepvar.entry5
  entry .ascplot.top_frm.varfrm.indepvar.entry5 \
    -state disabled \
    -font $ascplotvect(textfont)

  # build widget .ascplot.top_frm.varfrm.indepvar.label4
  label .ascplot.top_frm.varfrm.indepvar.label4 \
    -font $ascplotvect(btnfont) \
    -text {Independent:}

  # pack widget .ascplot.top_frm.varfrm.indepvar
  pack append .ascplot.top_frm.varfrm.indepvar \
    .ascplot.top_frm.varfrm.indepvar.label4 {left frame center} \
    .ascplot.top_frm.varfrm.indepvar.entry5 {top frame center expand fill}

  # build widget .ascplot.top_frm.varfrm.varsel
  frame .ascplot.top_frm.varfrm.varsel

  # build widget .ascplot.top_frm.varfrm.varsel.ctrbtns
  frame .ascplot.top_frm.varfrm.varsel.ctrbtns 

  # build widget .ascplot.top_frm.varfrm.varsel.ctrbtns.bleft
  button .ascplot.top_frm.varfrm.varsel.ctrbtns.bleft \
    -font $ascplotvect(btnfont) \
    -command ascplot_selindependentleft \
    -text {V}

  # build widget .ascplot.top_frm.varfrm.varsel.ctrbtns.bmid
  button .ascplot.top_frm.varfrm.varsel.ctrbtns.bmid \
    -font $ascplotvect(btnfont) \
    -command ascplot_unseldependent \
    -text {<<}

  # build widget .ascplot.top_frm.varfrm.varsel.ctrbtns.bright
  button .ascplot.top_frm.varfrm.varsel.ctrbtns.bright \
    -font $ascplotvect(btnfont) \
    -command ascplot_selindependentright \
    -text {V}

  # build widget .ascplot.top_frm.varfrm.varsel.ctrbtns.btop
  button .ascplot.top_frm.varfrm.varsel.ctrbtns.btop \
    -font $ascplotvect(btnfont) \
    -command ascplot_seldependent \
    -text {>>}

  # pack widget .ascplot.top_frm.varfrm.varsel.ctrbtns
  pack append .ascplot.top_frm.varfrm.varsel.ctrbtns \
    .ascplot.top_frm.varfrm.varsel.ctrbtns.btop {top frame center expand fill}\
    .ascplot.top_frm.varfrm.varsel.ctrbtns.bmid {top frame center expand fill}\
    .ascplot.top_frm.varfrm.varsel.ctrbtns.bleft \
       {left frame center expand fill} \
    .ascplot.top_frm.varfrm.varsel.ctrbtns.bright \
       {left frame center expand fill}

  # build widget .ascplot.top_frm.varfrm.varsel.depvar
  frame .ascplot.top_frm.varfrm.varsel.depvar

  # build widget .ascplot.top_frm.varfrm.varsel.depvar.scrollbar2
  scrollbar .ascplot.top_frm.varfrm.varsel.depvar.scrollbar2 \
    -command {.ascplot.top_frm.varfrm.varsel.depvar.listbox1 yview} 

  # build widget .ascplot.top_frm.varfrm.varsel.depvar.scrollbar3
  scrollbar .ascplot.top_frm.varfrm.varsel.depvar.scrollbar3 \
    -command {.ascplot.top_frm.varfrm.varsel.depvar.listbox1 xview} \
    -orient {horizontal}

  # build widget .ascplot.top_frm.varfrm.varsel.depvar.listbox1
  listbox .ascplot.top_frm.varfrm.varsel.depvar.listbox1 \
    -width {20} \
    -height {2} \
    -xscrollcommand {.ascplot.top_frm.varfrm.varsel.depvar.scrollbar3 set} \
    -yscrollcommand {.ascplot.top_frm.varfrm.varsel.depvar.scrollbar2 set} \
    -font $ascplotvect(textfont) \
    -selectmode extended \

  # pack widget .ascplot.top_frm.varfrm.varsel.depvar
  pack append .ascplot.top_frm.varfrm.varsel.depvar \
    .ascplot.top_frm.varfrm.varsel.depvar.scrollbar2 \
      {right frame center filly} \
    .ascplot.top_frm.varfrm.varsel.depvar.listbox1 \
      {top frame center expand fill} \
    .ascplot.top_frm.varfrm.varsel.depvar.scrollbar3 \
      {bottom frame center fillx}

  # build widget .ascplot.top_frm.varfrm.varsel.varlist
  frame .ascplot.top_frm.varfrm.varsel.varlist

  # build widget .ascplot.top_frm.varfrm.varsel.varlist.scrollbar2
  scrollbar .ascplot.top_frm.varfrm.varsel.varlist.scrollbar2 \
    -command {.ascplot.top_frm.varfrm.varsel.varlist.listbox1 yview} 

  # build widget .ascplot.top_frm.varfrm.varsel.varlist.scrollbar3
  scrollbar .ascplot.top_frm.varfrm.varsel.varlist.scrollbar3 \
    -command {.ascplot.top_frm.varfrm.varsel.varlist.listbox1 xview} \
    -orient {horizontal} 

  # build widget .ascplot.top_frm.varfrm.varsel.varlist.listbox1
  listbox .ascplot.top_frm.varfrm.varsel.varlist.listbox1 \
    -width {20} \
    -height {2} \
    -xscrollcommand {.ascplot.top_frm.varfrm.varsel.varlist.scrollbar3 set} \
    -yscrollcommand {.ascplot.top_frm.varfrm.varsel.varlist.scrollbar2 set} \
    -font $ascplotvect(textfont) \
    -selectmode extended 


  # pack widget .ascplot.top_frm.varfrm.varsel.varlist
  pack append .ascplot.top_frm.varfrm.varsel.varlist \
    .ascplot.top_frm.varfrm.varsel.varlist.scrollbar2 \
       {left frame center filly} \
    .ascplot.top_frm.varfrm.varsel.varlist.listbox1 \
       {top frame center expand fill} \
    .ascplot.top_frm.varfrm.varsel.varlist.scrollbar3 \
       {bottom frame center fillx}

  # pack widget .ascplot.top_frm.varfrm.varsel
  pack append .ascplot.top_frm.varfrm.varsel \
    .ascplot.top_frm.varfrm.varsel.varlist {left frame center expand fill} \
    .ascplot.top_frm.varfrm.varsel.ctrbtns {left frame center filly} \
    .ascplot.top_frm.varfrm.varsel.depvar {left frame center expand fill}

  # pack widget .ascplot.top_frm.varfrm
  pack append .ascplot.top_frm.varfrm \
    .ascplot.top_frm.varfrm.varlbl {top frame center fillx} \
    .ascplot.top_frm.varfrm.varsel {top frame center expand fill} \
    .ascplot.top_frm.varfrm.indepvar {top frame center fill}

  # pack widget .ascplot.top_frm
  pack append .ascplot.top_frm \
    .ascplot.top_frm.datasets {top frame center expand fill} \
    .ascplot.top_frm.varfrm {top frame center expand fill}

  # pack widget .ascplot.bot_frm
  pack append .ascplot.bot_frm \
    .ascplot.bot_frm.message {top frame center expand fill} \
    .ascplot.bot_frm.table_frm {top frame center fill}

  # pack widget .ascplot
  pack append .ascplot \
    .ascplot.top_frm {top frame center expand fill} \
    .ascplot.bot_frm {top frame center expand fill}

  .ascplot.bot_frm.message.text2 insert end {}
  .ascplot.top_frm.varfrm.indepvar.entry5 insert end {ivar}



  if {"[info procs XFEdit]" != ""} {
    catch "XFMiscBindWidgetTree .ascplot"
    after 2 "catch {XFEditSetShowWindows}"
  }
}

proc DestroyWindow.ascplot {} {# xf ignore me 7
  if {"[info procs XFEdit]" != ""} {
    if {"[info commands .ascplot]" != ""} {
      global xfShowWindow.ascplot
      set xfShowWindow.ascplot 0
      XFEditSetPath .
      after 2 "XFSaveAsProc .ascplot; XFEditSetShowWindows"
    }
  } {
    catch "destroy .ascplot"
    update
  }
}


# User defined procedures


# Internal procedures

# eof
#

