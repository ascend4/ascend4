#  ascend.tcl: startup tcl code for ASCEND called via the .ascendrc
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.43 $
#  Last modified on: $Date: 2003/02/06 04:08:26 $
#  Last modified by: $Author: ballan $
#  Revision control file: $RCSfile: ascend.tcl,v $
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

# startup tcl code for ASCEND called via the .ascendrc sourced in the binary
#
# Global init file for windows configurations, button configurations, var inits
# assumes toplevel windows have  not  already been created
#
# all procedures called at the bottom.
#

proc AscGnuButton {} {
  global env
  # pack widget .
  wm positionfrom . user
  wm geometry . 1x1
  wm sizefrom . ""
  wm iconname . "Ascend IV Information"
  wm protocol . WM_DELETE_WINDOW {EXIT}
  wm title . {Ascend IV}
  wm withdraw .
}

proc Help {{arg1 ""} args} {
  if {[catch {set foo [help "$arg1"]} msg]} {
    puts stdout $msg
  } else {
    foreach i $foo {
      puts stdout "$i"
    }
  }
}

# stupid little function to get around the brain-deadisms that
# source possesses in tcl8
proc asc_source {file} {
  global ascSourceVect errorInfo
  if {![info exists ascSourceVect(count)]} {
    set ascSourceVect(count) 0
    set ascSourceVect(errcount) 0
    set ascSourceVect(tails) {}
  }
  incr ascSourceVect(count)
  set fn $ascSourceVect(count)
  set ascSourceVect($fn) $file
  set ascSourceVect($fn,error) none
  if {![file readable $file]} {
    incr ascSourceVect(errcount)
    puts stderr "*** Unable to open ${fn}th sourced file $file"
    puts stderr "*** ASCEND probably will not work well without it."
    set ascSourceVect($fn,error) $errorInfo
    return
  }
  append ascSourceVect(tails) \n[file tail $file]
  if {[catch {source $file} err]} {
    incr ascSourceVect(errcount)
    set ascSourceVect($fn,error) $errorInfo
    puts stderr "*** Error while loading $file:"
    puts stderr $err
    puts stderr "* details *"
    puts stderr $ascSourceVect($fn,error)
    puts stderr "******"
  }
}

#
# methods to test global var setting/reading when all else fails
# during startup or later. Would be nice to activate these from
# the Internal window
#
proc trace_var {a s} {
  global $a
  if {![info exists ${a}($s)]} {
    puts stderr "Trace: init trace on ${a}($s)"
    set ${a}($s) {}
  }
  trace variable  ${a}($s) w trace_log
  trace variable  ${a}($s) r trace_log
  trace variable  ${a}($s) u trace_log
  trace variable  $a u trace_log
}

proc trace_log {n1 n2 op} {
  puts stderr "Trace: $n1 $n2 $op"
  if {$op == "w"} { global $n1; puts "New value: [set ${n1}($n2)]" }
}

proc load_Templates {} {
  global env
  puts "loading templates from $env(ASCENDTK)/templates/"
  puts "source $env(ASCENDTK)/templates/AscMonoEntry1.t"
  asc_source "$env(ASCENDTK)/templates/AscMonoEntry1.t"
  puts "source $env(ASCENDTK)/templates/AscMonoEntry2.t"
  asc_source "$env(ASCENDTK)/templates/AscMonoEntry2.t"
  puts "source $env(ASCENDTK)/templates/AscMonoEntry4.t"
  asc_source "$env(ASCENDTK)/templates/AscMonoEntry4.t"
  puts "source $env(ASCENDTK)/templates/AscConfirm.t"
  asc_source "$env(ASCENDTK)/templates/AscConfirm.t"
  puts "source $env(ASCENDTK)/templates/ascfontsel.tcl"
  asc_source "$env(ASCENDTK)/templates/ascfontsel.tcl"
  puts "source $env(ASCENDTK)/templates/balloon.tcl"
  asc_source "$env(ASCENDTK)/templates/balloon.tcl"
  puts "source $env(ASCENDTK)/templates/ascparm.tcl"
  asc_source "$env(ASCENDTK)/templates/ascparm.tcl"
  puts "source $env(ASCENDTK)/templates/ascKeepOnTop.tcl"
  asc_source "$env(ASCENDTK)/templates/ascKeepOnTop.tcl"
  puts "source $env(ASCENDTK)/templates/ascListSelect.tcl"
  asc_source "$env(ASCENDTK)/templates/ascListSelect.tcl"
  puts "source $env(ASCENDTK)/templates/ascListSelectB1.tcl"
  asc_source "$env(ASCENDTK)/templates/ascListSelectB1.tcl"
  puts "source $env(ASCENDTK)/templates/amlscroll.tcl"
  asc_source "$env(ASCENDTK)/templates/amlscroll.tcl"
  global tcl_platform tk_version
  if {$tcl_platform(platform) == "unix"} {
    switch $tk_version {
    8.5 -
    8.4 {
        puts "source $env(ASCENDTK)/templates/asctkfbox84.tcl"
        asc_source "$env(ASCENDTK)/templates/asctkfbox84.tcl"
      }
    default  {
        puts "source $env(ASCENDTK)/templates/asctkfbox.tcl"
        asc_source "$env(ASCENDTK)/templates/asctkfbox.tcl"
      }
    }
  }
  puts "source $env(ASCENDTK)/templates/ascdialog.tcl"
  asc_source "$env(ASCENDTK)/templates/ascdialog.tcl"
  puts "source $env(ASCENDTK)/templates/ascMsgBox.tcl"
  asc_source "$env(ASCENDTK)/templates/ascMsgBox.tcl"
  puts "source $env(ASCENDTK)/ascplotproc.tcl"
  asc_source "$env(ASCENDTK)/ascplotproc.tcl"
  puts "source $env(ASCENDTK)/ascplot.tcl"
  asc_source "$env(ASCENDTK)/ascplot.tcl"
  puts "source $env(ASCENDTK)/mergedat.tcl"
  asc_source "$env(ASCENDTK)/mergedat.tcl"
  puts "source $env(ASCENDTK)/ascStudy.tcl"
  asc_source "$env(ASCENDTK)/ascStudy.tcl"
  puts "source $env(ASCENDTK)/callback.tcl"
  asc_source "$env(ASCENDTK)/callback.tcl"
  puts "source $env(ASCENDTK)/notebox.tcl"
  asc_source "$env(ASCENDTK)/notebox.tcl"
}


#  Create a temporary EXIT command so that if something goes
#  wrong during startup, we can still exit the application.
#  If startup goes well, this EXIT will be overwritten by the
#  one in ScriptProc.tcl
#
if {[info command EXIT] == ""} {
  proc EXIT {args} {
    catch {destroy .}
    catch {user_shutdown}
    return;
  }
}

proc mcgraw_notice {} {
  puts "\n% The authors of ASCEND thank McGraw-Hill for permission to"
  puts "% distribute the database from Reid, Prausnitz, and Poling"
  puts "% Properties of Liquids and Gases, 4th Edition (RPP)"
  puts "% http://www.bookstore.mcgraw-hill.com"
  puts "% via the World Wide Web as part of ASCEND IV."
  puts "% RPP is copyright 1987, The McGraw-Hill Companies"
}

proc balloon_notice {} {
puts "\n# balloon.tcl - procedures used by balloon help from vTcl."
puts "# Copyright (C) 1996-1998 Stewart Allen"
puts "# Modified and used in compliance with the GNU License 2.0 or later."
puts "# See the ASCEND License for details."

}

global env
AscGnuButton


#stuff that has to be set before windows are shown goes in set_global_defaults
# which is in GlobalProc.tcl.
# i.e. flags that check for whether features are available that are used
#  determine button states, or toplevel existences.
#
# load delayed window instantiation code [ proc awin ] is in GlobalProc too

puts "source $env(ASCENDTK)/GlobalProc.tcl"
source "$env(ASCENDTK)/GlobalProc.tcl"
set_Global_Defaults
Glob_do_GNU

# main sources the window definitions
# main also sources some needed templates in generalk.tcl
puts "source $env(ASCENDTK)/main.tcl"
source "$env(ASCENDTK)/main.tcl"

puts "tk sourced"


# The plot window and the solver window require the
# TkTable exension package by Roland King
# mthomas 98.05.07: require 1.8 until we debug the memory errors in 2.0
if {[catch {package require Tktable 2.8} err]} {
  # print a message to stderr since a catch further up the call stack
  # eats the call to "error"
  puts stderr "ASCEND cannot locate the tkTable package."
  puts stderr "internal message: $err"
  puts stderr "tck_pkgPath: $tcl_pkgPath"
  puts stderr "auto_path: $auto_path"
  error "ASCEND cannot locate the tkTable package.\ninternal message: $err"
}


# load separate templates for transient toplevels
# must be done before buttons using those templates are wired via
# global variable vectors
###puts "NOT LOADING TEMPLATES"
load_Templates
puts "templates loaded"

# configure swiped widgets: set_Template_defaults
# routine is in GlobalProc.tcl
set_Template_defaults

# Load button smarts files. Each file is responsible for checking
# existence of its needed global variables (i.e. do the
# initialization of things at the top of the *Proc.tcl
global ascGlobalVect
 # puts "[info commands awin] w_load = $ascGlobalVect(w_load)"
#
 # Switch to determine whether to load all the windows or no.
if {$ascGlobalVect(w_load)} {
  puts "All windows will be loaded"
  awin all
} {
  puts "Nowindows will be loaded"
}
# load in .ascend-config
puts "Reading utilities"
Util_do_Read
puts stdout "Interface Loaded."

global ascSourceVect
if {$ascSourceVect(errcount)} {
  puts stderr "'parray ascSourceVect' to review error messages."
}
#
# Giving the location of the directory in which the user options
# are saved (if it exists)
#
  if {$ascGlobalVect(saveoptions) == 1} {
    puts stdout " "
    puts stdout "-----------------------------------"
    puts stdout "User data directory is $ascGlobalVect(userhome)"
    puts stdout "-----------------------------------"
    puts stdout " "
  }

# try to straighten out iconnames
asc_fixiconnames

balloon_notice
mcgraw_notice
# bind help to buttons
#>>Help_BindAllM3


