#  baatest.tcl
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.5 $
#  Last modified on: $Date: 1998/06/18 15:55:19 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: baatest.tcl,v $
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

# Module: batest.tcl
# Tcl version: 7.1 (Tcl/Tk/XF)
# Tk version: 3.4
# XF version: 2.2
#

# module contents
global moduleList
global autoLoadList
set moduleList(batest.tcl) { envVal loadAppDefaults showVars}
set autoLoadList(batest.tcl) {0}

source templates/ascDoubleE.tcl

# procedures to show toplevel windows


# User defined procedures


# Procedure: envVal
proc envVal { envValName} {
  global env
  if [info exists env($envValName)] {return $env($envValName)} {return {}}
}


# Procedure: loadAppDefaults
proc loadAppDefaults { classNameList {priority "startupFile"}} {
  set filepath "[split [envVal XUSERFILESEARCHPATH] :]  [envVal XAPPLRESDIR]                 [split [envVal XFILESEARCHPATH] :]     /usr/lib/X11"
  foreach i $classNameList {
    foreach j $filepath {
      if {[file exists $j/$i]} {
        option readfile $j/$i $priority; break
      }
    }
  }
}


# Procedure: showVars
proc showVars { w args} {
  catch {destroy $w}
  toplevel $w
  wm title $w "Variable values"
  label $w.title \
    -text "Variable values:" \
    -width 20 \
    -anchor center \
    -font {helvetica 18 bold}
  pack append $w $w.title {top fillx}
  foreach i $args {
    frame $w.$i
    label $w.$i.name -text "$i: "
    label $w.$i.value -textvar $i
    pack append $w.$i $w.$i.name left $w.$i.value left
    pack append $w $w.$i {top frame w}
  }
  button $w.ok -text OK -command "destroy $w"
  pack append $w $w.ok {bottom pady 2}
}

