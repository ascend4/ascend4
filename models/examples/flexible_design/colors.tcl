#                        colors.tcl
#                        by Robert S. Huss
#                        Part of the Ascend Library
#
#This file is part of the Ascend library.
#
#Copyright (C) 1994
#
#The Ascend library is free software; you can redistribute
#it and/or modify it under the terms of the GNU General Public License as
#published by the Free Software Foundation; either version 2 of the
#License, or (at your option) any later version.
#
#The Ascend Language Interpreter is distributed in hope that it will be
#useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#General Public License for more details.
#
#You should have received a copy of the GNU General Public License along with
#the program; if not, write to the Free Software Foundation, Inc., 675
#Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.

#  $Date: 1996/05/20 22:05:45 $
#  $Revision: 1.1.1.1 $
#  $Author: mthomas $
#  $Source: /afs/cs.cmu.edu/project/ascend/Repository/models/examples/flexible_design/colors.tcl,v $

# Module: colors.tcl
# Tcl version: 7.3 (Tcl/Tk/XF)
# Tk version: 3.6
# XF version: 2.3
#

# module contents
global moduleList
global autoLoadList
set moduleList(colors.tcl) { bobbutton bobcolors bobentry bobframe boblabel boblistbox bobscrollbar}
set autoLoadList(colors.tcl) {0}

# procedures to show toplevel windows


# User defined procedures


# Procedure: bobbutton
proc bobbutton { arg} {
  global bobcolor
  foreach i $arg {
    $i configure -activebackground $bobcolor(light)
    $i configure -activeforeground $bobcolor(dark)
    $i configure -background $bobcolor(base)
    $i configure -disabledforeground $bobcolor(light)
    $i configure -foreground $bobcolor(dark)
    $i configure -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*"
  }
}


# Procedure: bobcolors
proc bobcolors {} {

global frames
global buttons
global entries
global labels
global listboxes
global scrollbars

 bobframe $frames
 bobbutton $buttons
 bobentry $entries
 boblabel $labels
 boblistbox $listboxes
 bobscrollbar $scrollbars
}


# Procedure: bobentry
proc bobentry { arg} {
  global bobcolor
  foreach i $arg {
    $i configure -background $bobcolor(base)
    $i configure -font "-Adobe-Helvetica-Medium-R-Normal--*-120-*"
    $i configure -foreground $bobcolor(dark)
    $i configure -insertbackground $bobcolor(light)
    $i configure -selectbackground $bobcolor(light)
    $i configure -selectforeground $bobcolor(dark)
  }
}


# Procedure: bobframe
proc bobframe { arg} {
  global bobcolor
 foreach i $arg {
  $i configure -background $bobcolor(base)
 }
}


# Procedure: boblabel
proc boblabel { arg} {
  global bobcolor
  foreach i $arg {
    $i configure -background $bobcolor(base)
    $i configure -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*"
    $i configure -foreground $bobcolor(dark)
  }
}


# Procedure: boblistbox
proc boblistbox { arg} {
  global bobcolor
  foreach i $arg {
    $i configure -background $bobcolor(base)
    $i configure -foreground $bobcolor(dark)
    $i configure -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*"
    $i configure -selectbackground $bobcolor(light)
    $i configure -selectforeground $bobcolor(dark)
  }
}


# Procedure: bobscrollbar
proc bobscrollbar { arg} {
  global bobcolor
  foreach i $arg {
    $i configure -activeforeground $bobcolor(reallylight)
    $i configure -background $bobcolor(base)
    $i configure -foreground $bobcolor(light)
  }
}


# Internal procedures

# eof
#

