#  View.tcl: assigns the default params to the global vectors of each window
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.20 $
#  Last modified on: $Date: 1998/06/18 20:11:42 $
#  Last modified by: $Author: ballan $
#  Revision control file: $RCSfile: View.tcl,v $
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
# This file contains support Tcl code for the assignment of the default
# parameters to the global vectors of each ascend window
#


# this is the first batch that must deal with geometry,
# so we define a mess of geometry handlers that used to be
# in generalk.tcl here.
#
# proc setpos {master {offsetx "70"} {offsety "70"}}
#-------------------------------------------------------------------------
# calculate screen position as deltas from position of an existing window
# existing window does not have to be a toplevel. returns +x+y
#-------------------------------------------------------------------------
proc setpos {master {offsetx "70"} {offsety "70"}} {# calculates offsets
  set xpos [expr [winfo rootx $master] + $offsetx]
  set ypos [expr [winfo rooty $master] + $offsety]
  return "+$xpos+$ypos"
}

#
# proc getpos {master}
#-------------------------------------------------------------------------
# master is a toplevel wm knows about. returns +x+y of master
#-------------------------------------------------------------------------
proc getpos {master} {
  set list [split [wm geometry $master] +]
  set xpos [lindex $list 1]
  set ypos [lindex $list 2]
  return "+$xpos+$ypos"
}
# reparse geometry to a more sane state.
# tk broken. returns a geometry spec with extra signs deleted.
proc sanegeometry {geom} {
  set result {}
  set len [string length $geom]
  set rlen 0
  set lastsign {}
  set state digit; # other state is sign
  for {set i 0} {$i < $len} {incr i} {
    set c [string index $geom $i]
    switch -exact -- $c {
    0 -
    1 -
    2 -
    3 -
    4 -
    5 -
    6 -
    7 -
    8 -
    9 -
    X -
    x {
        switch -exact -- $lastsign {
        + -
        - {
             append result $lastsign
             set lastsign {}
          }
        }
        append result $c
      }
    - -
    + {
        set lastsign $c
      }
    default {
        error "bad geometry $geom specified"
      }
    }
  }
  return $result
}

#
# proc osgpos {geom}
#-------------------------------------------------------------------------
# given a complete geometry of the form WxH+X+Y returns a geometry
# such that the lower right corner will be on the screen. if this geometry
# would force the upper left corner to be above/left of +2+2,
# returns a geometry such that upper left corner is +2+2
# ignores signs on X Y arguments
# does not understand gridded windows, window dressing may result in
# slight misalignment due to the style of wm-Tk geometry interaction
#-------------------------------------------------------------------------
proc osgpos {geom} {
  set geom [sanegeometry $geom]
  set sh [winfo screenheight .]
  set sw [winfo screenwidth .]
  set glist [split $geom +-]
  if {[llength $glist] != 3} {error "Invalid geometry spec passed to osg"}
  set x [lindex $glist 1]
  set y [lindex $glist 2]
  set slist [split [lindex $glist 0] x]
  if {[llength $slist] != 2} {error "Invalid window size given to osg"}
  set w [lindex $slist 0]
  set h [lindex $slist 1]
  set newx $x
  set newy $y
  if {($x + $w) > $sw} {set newx [expr $sw - $w]}
  if {($y + $h) > $sh} {set newy [expr $sh - $h -20]}
  if { $newx < 10 } {set newx 10}
  if { $newy < 10 } {set newy 10}
  return [lindex $glist 0]+$newx+$newy
}

#
# proc ospos {w h x y}
#-------------------------------------------------------------------------
# given a geometry of the form W H X Y returns a geometry
# such that the lower right corner will be on the screen. if this geometry
# would force the upper left corner to be above/left of +2+2,
# returns a geometry such that upper left corner is +2+2
# does not understand gridded windows, window dressing may result in
# slight misalignment due to the style of wm-Tk geometry interaction
#-------------------------------------------------------------------------
proc ospos {w h x y} {
  set sh [winfo screenheight .]
  set sw [winfo screenwidth .]
  set newx $x
  set newy $y
  if {($x + $w) > $sw} {set newx [expr $sw - $w]}
  if {($y + $h) > $sh} {set newy [expr $sh - $h -20]}
  if { $newx < 2 } {set newx 2}
  if { $newy < 2 } {set newy 2}
  return ${w}x${h}+$newx+$newy
}

#
# proc gospos {w x y}
#-------------------------------------------------------------------------
# given a gridded window, w, and coordinates x y, generate geometry
# such that the lower right corner will be on the screen. If this geometry
# would force the upper left corner to be above/left of +2+2,
# returns a geometry such that upper left corner is +2+2.
# Window dressing may result in
# slight misalignment due to the style of wm-Tk geometry interaction
# Note: this requires that the window in question already have the desired
# size set. you may have to do an idletask update to get the geom set.
# The geometry returned is useful only for relocation, not creation.
#-------------------------------------------------------------------------
proc gospos {gw x y} {
  set h [winfo height $gw]
  set w [winfo width $gw]
  set pgeom [ospos $w $h $x $y]
  set px +[lindex [split $pgeom +] 1]
  set py +[lindex [split $pgeom +] 2]
  set wgeom [wm geometry $gw]
  set wsize [lindex [split $wgeom +] 0]
  return ${wsize}${px}${py}
}


# Vector containing the name of the vector for each ascend window
global ascViewWindowVect

#
# proc Browser {args}
#------------------------------------------------------------------------
# Sets the default parameters for the vector ascBrowVect
#------------------------------------------------------------------------
proc Browser {args} {

  global ascBrowVect

  if {$args == {}} {
    puts "ERROR: No arguments passed to Browser"
    return
  }

  set ind 0
  set attr [lindex $args $ind]

  if {[llength $args] == 1} {
    return $ascBrowVect($attr)
  } else {
    incr ind
    set value [lrange $args $ind end]
    set ascBrowVect($attr) $value
    return $value
  }
}


#
#
# proc Debugger {args}
#------------------------------------------------------------------------
# Sets the default parameters for the vector ascDebuVect
#------------------------------------------------------------------------
proc Debugger {args} {

  global ascDebuVect

  if {$args == {}} {
    puts "ERROR: No arguments passed to Debugger"
    return
  }

  set ind 0
  set attr [lindex $args $ind]

  if {[llength $args] == 1} {
    return $ascDebuVect($attr)
  } else {
    incr ind
    set value [lrange $args $ind end]
    set ascDebuVect($attr) $value
    return $value
  }
}


#
#
# proc Display {args}
#------------------------------------------------------------------------
# Sets the default parameters for the vector ascDispVect
#------------------------------------------------------------------------
proc Display {args} {

  global ascDispVect

  if {$args == {}} {
    puts "ERROR: No arguments passed to Display"
    return
  }

  set ind 0
  set attr [lindex $args $ind]

  if {[llength $args] == 1} {
    return $ascDispVect($attr)
  } else {
    incr ind
    set value [lrange $args $ind end]
    set ascDispVect($attr) $value
    return $value
  }
}


#
# This file contains support Tcl code for the assignment of the default
# parameters to the global vectors of each ascend window
#
#
# proc Global {args}
#------------------------------------------------------------------------
# Sets the default parameters for the vector ascGlobalVect
#------------------------------------------------------------------------
proc Global {args} {

  global ascGlobalVect

  if {$args == {}} {
    puts "ERROR: No arguments passed to Global"
    return
  }

  set ind 0
  set attr [lindex $args $ind]

  if {[llength $args] == 1} {
    return $ascGlobalVect($attr)
  } else {
    incr ind
    set value [lrange $args $ind end]
    set ascGlobalVect($attr) $value
    return $value
  }
}


#
#
# proc Library {args}
#------------------------------------------------------------------------
# Sets the  default parameters for the vector ascLibrVect
#------------------------------------------------------------------------
proc Library {args} {

  global ascLibrVect

  if {$args == {}} {
    puts "ERROR: No arguments passed to Library"
    return
  }

  set ind 0
  set attr [lindex $args $ind]

  if {[llength $args] == 1} {
    return $ascLibrVect($attr)
  } else {
    incr ind
    set value [lrange $args $ind end]
    set ascLibrVect($attr) $value
    return $value
  }
}


#
#
# proc Matrix {args}
#------------------------------------------------------------------------
# Sets the default parameters for the vector ascMtxVect
#------------------------------------------------------------------------
proc Matrix {args} {

  global ascMtxVect

  if {$args == {}} {
    puts "ERROR: No arguments passed to Matrix"
    return
  }

  set ind 0
  set attr [lindex $args $ind]

  if {[llength $args] == 1} {
    return $ascMtxVect($attr)
  } else {
    incr ind
    set value [lrange $args $ind end]
    set ascMtxVect($attr) $value
    return $value
  }
}


#
#
# proc Probe {args}
#------------------------------------------------------------------------
# Sets the default parameters for the vector ascProbVect
#------------------------------------------------------------------------
proc Probe {args} {

  global ascProbVect

  if {$args == {}} {
    puts "ERROR: No arguments passed to Probe"
    return
  }

  set ind 0
  set attr [lindex $args $ind]

  if {[llength $args] == 1} {
    return $ascProbVect($attr)
  } else {
    incr ind
    set value [lrange $args $ind end]
    set ascProbVect($attr) $value
    return $value
  }
}


#
#
# proc Script {args}
#------------------------------------------------------------------------
# Sets the default parameters for the vector ascScripVect
#------------------------------------------------------------------------
proc Script {args} {

  global ascScripVect

  if {$args == {}} {
    puts "ERROR: No arguments passed to Script"
    return
  }

  set ind 0
  set attr [lindex $args $ind]

  if {[llength $args] == 1} {
    return $ascScripVect($attr)
  } else {
    incr ind
    set value [lrange $args $ind end]
    set ascScripVect($attr) $value
    return $value
  }
}


#
#
# proc Simulations {args}
#------------------------------------------------------------------------
# Sets the default parameters for the vector ascSimsVect
#------------------------------------------------------------------------
proc Simulations {args} {

  global ascSimsVect

  if {$args == {}} {
    puts "ERROR: No arguments passed to Simulations"
    return
  }

  set ind 0
  set attr [lindex $args $ind]

  if {[llength $args] == 1} {
    return $ascSimsVect($attr)
  } else {
    incr ind
    set value [lrange $args $ind end]
    set ascSimsVect($attr) $value
    return $value
  }
}


#
#
# proc Solver {args}
#------------------------------------------------------------------------
# Sets the default parameters for the vector ascSolvVect
#------------------------------------------------------------------------
proc Solver {args} {

  global ascSolvVect

  if {$args == {}} {
    puts "ERROR: No arguments passed to Solver"
    return
  }

  set ind 0
  set attr [lindex $args $ind]

  if {[llength $args] == 1} {
    return $ascSolvVect($attr)
  } else {
    incr ind
    set value [lrange $args $ind end]
    set ascSolvVect($attr) $value
    return $value
  }
}
#
#
# proc SolverGeneral {args}
#------------------------------------------------------------------------
# Sets the default parameters for the vector ascSolv32767Vect
#------------------------------------------------------------------------
proc SolverGeneral {args} {

  global ascSolv32767Vect

  if {$args == {}} {
    puts "ERROR: No arguments passed to SolverGeneral"
    return
  }

  set ind 0
  set attr [lindex $args $ind]

  if {[llength $args] == 1} {
    return $ascSolv32767Vect($attr)
  } else {
    incr ind
    set value [lrange $args $ind end]
    set ascSolv32767Vect($attr) $value
    return $value
  }
}


#
#
# proc Toolbox {args}
#------------------------------------------------------------------------
# Sets the default parameters for the vector ascToolVect
#------------------------------------------------------------------------
proc Toolbox {args} {

  global ascToolVect

  if {$args == {}} {
    puts "ERROR: No arguments passed to Toolbox"
    return
  }

  set ind 0
  set attr [lindex $args $ind]

  if {[llength $args] == 1} {
    return $ascToolVect($attr)
  } else {
    incr ind
    set value [lrange $args $ind end]
    set ascToolVect($attr) $value
    return $value
  }
}


#
#
# proc Units {args}
#------------------------------------------------------------------------
# Sets the default parameters for the vector ascUnitVect
#------------------------------------------------------------------------
proc Units {args} {

  global ascUnitVect

  if {$args == {}} {
    puts "ERROR: No arguments passed to Units"
    return
  }

  set ind 0
  set attr [lindex $args $ind]

  if {[llength $args] == 1} {
    return $ascUnitVect($attr)
  } else {
    incr ind
    set value [lrange $args $ind end]
    set ascUnitVect($attr) $value
    return $value
  }
}

#
#
# proc AscPlot {args}
#------------------------------------------------------------------------
# Sets the default parameters for the vector ascplotvect
#------------------------------------------------------------------------
proc AscPlot {args} {

  global ascplotvect

  if {$args == {}} {
    puts "ERROR: No arguments passed to Units"
    return
  }

  set ind 0
  set attr [lindex $args $ind]

  if {[llength $args] == 1} {
    return $ascplotvect($attr)
  } else {
    incr ind
    set value [lrange $args $ind end]
    set ascplotvect($attr) $value
    return $value
  }
}

#
#
# proc Environment {args}
#------------------------------------------------------------------------
# Sets the default parameters for the vector env
#------------------------------------------------------------------------
proc Environment {args} {

  global env

  if {$args == {}} {
    puts "ERROR: No arguments passed to Environment"
    return
  }

  set ind 0
  set attr [lindex $args $ind]

  if {[llength $args] == 1} {
    return $env($attr)
  } else {
    incr ind
    set value [lrange $args $ind end]
    set env($attr) $value
    return $value
  }
}


#
#
# proc Set_ViewVect_Values {}
#------------------------------------------------------------------------
# Define a vector containing the names of the vectors corresponding to each
# of the ASCEND window
#------------------------------------------------------------------------
proc Set_ViewVect_Values {} {
  global ascViewWindowVect

  set ascViewWindowVect(browser) "ascBrowVect"
  set ascViewWindowVect(debugger) "ascDebuVect"
  set ascViewWindowVect(display) "ascDispVect"
  set ascViewWindowVect(global) "ascGlobalVect"
  set ascViewWindowVect(library) "ascLibrVect"
  set ascViewWindowVect(matrix) "ascMtxVect"
  set ascViewWindowVect(probe) "ascProbVect"
  set ascViewWindowVect(script) "ascScripVect"
  set ascViewWindowVect(simulations) "ascSimsVect"
  set ascViewWindowVect(solver) "ascSolvVect"
  set ascViewWindowVect(toolbox) "ascToolVect"
  set ascViewWindowVect(units) "ascUnitVect"
  set ascViewWindowVect(ascplot) "ascplotvect"
  set ascViewWindowVect(environment) "env"
  set ascViewWindowVect(slv32767parms) "ascSolv32767Vect"

  return
}

#
#
# proc Set_View_Option_Values {}
#------------------------------------------------------------------------
# For the global vectors of ASCEND, this functions define an element
# called viewoptions, which are the names of other array elements (in
# the same vector) that the user may want be able to get and modify.
#
# For each toplevel window, we define a set of cosmetic appearance
# (viewoptions) and a set of saved control parameters (controloptions).
# The split among these is somewhat arbitrary, in a few cases.
# cosmetic options should just be saved whenever we feel like,
# while the user should direct us to save control options because
# these change default behavior from that which we describe in the
# documentation.
#------------------------------------------------------------------------
proc Set_View_Option_Values {} {

  global ascBrowVect ascDebuVect ascDispVect ascGlobalVect ascLibrVect
  global ascMtxVect ascProbVect ascScripVect ascSimsVect ascSolvVect
  global ascToolVect ascUnitVect ascplotvect env ascSolv32767Vect

  # View_InitDialog
  set ascBrowVect(controloptions) [list \
    TypeorValue \
    ShowAtoms \
    HidePassed \
    dimconsistency \
    queryfile \
    localshow,BOOLEAN_ATOM_INST \
    localshow,BOOLEAN_CONSTANT_INST \
    localshow,INTEGER_ATOM_INST \
    localshow,INTEGER_CONSTANT_INST \
    localshow,LREL_INST  \
    localshow,REAL_ATOM_INST  \
    localshow,REAL_CONSTANT_INST  \
    localshow,REL_INST \
    localshow,SET_ATOM_INST \
    localshow,SYMBOL_ATOM_INST \
    localshow,SYMBOL_CONSTANT_INST \
  ]
  set ascSolv32767Vect(viewoptions) {}
  # this list should follow from  ascSolv32767Vect(namelist)
  # but may not yet exist at startup.
  set ascSolv32767Vect(controloptions) [list \
    lnmepsilon \
    update_frequency \
    update_time dtmin \
    dtmax \
    dtzero \
    moststeps \
    newlog \
    checksing \
    showavgs \
    yfilename \
    obsfilename \
    logsi \
    logcol \
    nearbound \
    farnom
  ]
  set ascBrowVect(viewoptions) [list \
    font \
    geometry \
    minsize \
    iconname \
    initialstate \
    visibility \
  ]
  set ascDebuVect(controloptions) {}
  set ascDebuVect(viewoptions) [list \
    geometry \
    minsize \
    iconname \
    initialstate \
    visibility \
  ]
  set ascDispVect(controloptions) "ShowComments"
  set ascDispVect(viewoptions) "geometry minsize iconname initialstate font"
  set ascGlobalVect(viewoptions) "font labelfont tbg tfg"
  set ascGlobalVect(controloptions) hideinfoforever
  set ascLibrVect(controloptions) [list \
    ignorestop \
    compileC \
    compilerWarnings \
    parserWarnings \
    simplifyRelations \
    lastreadextension \
  ]
  set ascLibrVect(viewoptions) [list \
    font \
    geometry \
    minsize \
    iconname \
    initialstate \
    visibility \
    entryFont \
  ]
  set ascMtxVect(controloptions) {}
  set ascMtxVect(viewoptions) "geometry font initialstate visibility"
  set ascProbVect(controloptions) {}
  set ascProbVect(viewoptions) [list \
    font \
    geometry \
    minsize \
    iconname \
    initialstate \
    visibility \
  ]
  set ascScripVect(controloptions) lastreadextension
  set ascScripVect(viewoptions) [list \
    font \
    geometry \
    minsize \
    iconname \
    initialstate \
    visibility
  ]
  set ascSimsVect(controloptions) {}
  set ascSimsVect(viewoptions) [list \
    iconname \
    initialstate \
    visibility
  ]
  set ascSolvVect(controloptions) [list font \
    statreport \
    modelbar \
  ]
  set ascSolvVect(viewoptions) [list font \
    geometry \
    minsize \
    iconname \
    initialstate \
    visibility \
  ]
  set ascToolVect(controloptions) {}
  set ascToolVect(viewoptions) [list geometry minsize iconname initialstate]
  set ascUnitVect(viewoptions) [list font \
    precision geometry minsize iconname initialstate]
  set ascUnitVect(controloptions) {}
  set ascplotvect(controloptions) {}
  set ascplotvect(viewoptions) "minsize maxsize btnfont textfont titlefont"
  set env(viewoptions) {}
  set env(controloptions) {}
  return
}

#
# proc View_Options {window}
#------------------------------------------------------------------------
# return the value of the array element viewoptions for the global vector
# of the ASCEND window given by the argument window.
#------------------------------------------------------------------------
proc View_Options {window} {

  global ascBrowVect ascDebuVect ascDispVect ascGlobalVect ascLibrVect
  global ascMtxVect ascProbVect ascScripVect ascSimsVect ascSolvVect
  global ascToolVect ascUnitVect ascplotvect env ascSolv32767Vect
  global ascViewWindowVect

  Set_ViewVect_Values
  Set_View_Option_Values

  set vectorname $ascViewWindowVect($window)
  return [set [set vectorname](controloptions)]
}

#
# proc View_Appearance {window}
#------------------------------------------------------------------------
# return the value of the array element viewoptions for the global vector
# of the ASCEND window given by the argument window.
#------------------------------------------------------------------------
proc View_Appearance {window} {

  global ascBrowVect ascDebuVect ascDispVect ascGlobalVect ascLibrVect
  global ascMtxVect ascProbVect ascScripVect ascSimsVect ascSolvVect
  global ascToolVect ascUnitVect ascplotvect env ascSolv32767Vect
  global ascViewWindowVect

  Set_ViewVect_Values
  Set_View_Option_Values

  set vectorname $ascViewWindowVect($window)
  return [set [set vectorname](viewoptions)]
}


#
# proc View_Get_Proc_Name {window}
#------------------------------------------------------------------------
# take a util default subscript and return its variable
# for the utility box. you better have set these vars
# elsewhere or trap those not ever set
#------------------------------------------------------------------------
proc View_Get_Proc_Name {window} {
  switch $window {
    {browser}         {return "Browser"}
    {debugger}        {return "Debugger"}
    {display}         {return "Display"}
    {global}          {return "Global"}
    {library}         {return "Library"}
    {matrix}          {return "Matrix"}
    {probe}           {return "Probe"}
    {script}          {return "Script"}
    {simulations}     {return "Simulations"}
    {solver}          {return "Solver"}
    {slv32767parms}   {return "SolverGeneral"}
    {toolbox}         {return "Toolbox"}
    {units}           {return "Units"}
    {ascplot}         {return "AscPlot"}
    {environment}     {return "Environment"}
    default       {error "View_Get_Proc_Name called with bad option $window"}
  }
}

#
# proc View_Save_Values {}
#------------------------------------------------------------------------
# save ascend.ad  overwriting the previous file. It does not store values
# for windows which are created only when needed, like ascplot, utilities
# matrix, and debugger. The viewoptions for those windows are saved when
# one closes them after using them
#------------------------------------------------------------------------
proc View_Save_Values {} {

  global env ascGlobalVect

  set window_list [list \
    browser display global library \
    probe script simulations solver slv32767parms \
    toolbox units environment
  ]
  set outfile $ascGlobalVect(userhome)/ascend.ad
  if {[catch {set ascendfile [open $outfile w]} ]} {
    puts "Error writing $outfile."
    return
  }

  puts $ascendfile "\#"
  puts $ascendfile "\# This is a generated file containing the Tcl code for"
  puts $ascendfile "\# the assignment of the Application defaults."
  catch {
    puts $ascendfile "\# Saved [clock format [clock clicks]] on [info hostname]"
  }
  puts $ascendfile "\#"
  puts $ascendfile " "

  puts $ascendfile "\# Appearance Settings"
  foreach win $window_list {
    set windowproc [View_Get_Proc_Name $win]
    set option_list [View_Appearance $win]
    foreach opt $option_list {
      set value [$windowproc $opt]
      if {$opt == "geometry"} {
	if {[wm state [$windowproc windowname]] != "withdrawn" } {
	  set value [wm geometry [$windowproc windowname]]  
	}
      }
      puts $ascendfile "$windowproc $opt $value"
    }
  }
  puts $ascendfile "\# Behavior Option Settings"
  foreach win $window_list {
    set windowproc [View_Get_Proc_Name $win]
    set option_list [View_Options $win]
    foreach opt $option_list {
      set value [$windowproc $opt]
      puts $ascendfile "$windowproc $opt $value"
    }
  }
  puts $ascendfile "set readoptions OK"
  close $ascendfile
  puts stdout "Wrote ascend.ad file $outfile."

  # write appearances, global, script options
  Script_do_SaveOptions
  
  # write misc options
  View_Save_Window_Options browser
  View_Save_Window_Options display
  View_Save_Window_Options library
  
  # write solver options
  View_Save_Solvers_Params
}

# proc View_InitDialog {}
#-------------------------
# init vector to control 
# info save.
#-------------------------
proc View_InitDialog {} {
  global ascViewSaveVect
  global tcl_platform ascViewSaveVect env

  set ascViewSaveVect(grab) 0
  set ascViewSaveVect(cancellable) 1
  set ascViewSaveVect(entrywidth) 20
  set ascViewSaveVect(npages) 1
  set ascViewSaveVect(toplevel) .viewsaveconfig
  set ascViewSaveVect(title) "Save options for all windows"
  set ascViewSaveVect(helpcommand) {View_Help}
  set ascViewSaveVect(whenokcommand) {View_Save_Interface_Values}
  set ascViewSaveVect(namelist) [list \
    windowpositions \
    fonts \
    colors \
    solversparams \
  ]
  set ascViewSaveVect(windowpositions) 1
  set ascViewSaveVect(fonts) 1
  set ascViewSaveVect(colors) 1
  set ascViewSaveVect(solversparams) 1
  set ascViewSaveVect(windowpositions.label) "Save window placements"
  set ascViewSaveVect(fonts.label) "Save current font selections"
  set ascViewSaveVect(colors.label) "Save current color selections"
  set ascViewSaveVect(solversparams.label) "Save solvers parameters"
  set ascViewSaveVect(windowpositions.type) bool
  set ascViewSaveVect(fonts.type) bool
  set ascViewSaveVect(colors.type) bool
  set ascViewSaveVect(solversparams.type) bool

}

#
# The following procedures are used to set the values specific to
# a individual windows
#
#

# proc View_WindowInitDialog {}
#-------------------------
# init vector to control save information for specific window
# info save.
#-------------------------
proc View_WindowInitDialog {window} {
  global ascViewSaveWindowVect

  set ascViewSaveWindowVect(grab) 0
  set ascViewSaveWindowVect(cancellable) 1
  set ascViewSaveWindowVect(entrywidth) 20
  set ascViewSaveWindowVect(npages) 1
  set ascViewSaveWindowVect(toplevel) .viewsavewindow
  set ascViewSaveWindowVect(title) "Save window options"
  set ascViewSaveWindowVect(helpcommand) {View_Help}
  set ascViewSaveWindowVect(whenokcommand) "View_Save_Window_Appearance $window"
  set ascViewSaveWindowVect(namelist) [list \
    windowpositions \
    fonts \
    colors \
  ]
  set ascViewSaveWindowVect(windowpositions) 1
  set ascViewSaveWindowVect(fonts) 1
  set ascViewSaveWindowVect(colors) 1
  set ascViewSaveWindowVect(windowpositions.label) "Save window placements"
  set ascViewSaveWindowVect(fonts.label) "Save current font selections"
  set ascViewSaveWindowVect(colors.label) "Save current color selections"
  set ascViewSaveWindowVect(windowpositions.type) bool
  set ascViewSaveWindowVect(fonts.type) bool
  set ascViewSaveWindowVect(colors.type) bool
} 


#
# proc View_Set_Save_Options {window}
#------------------------------------------------------------------------
# Ask for the selection of the values to to be saved.
# saves the appearances.
#------------------------------------------------------------------------
proc View_Set_Save_Options {window} {
  global ascBrowVect ascDebuVect ascDispVect ascGlobalVect ascLibrVect
  global ascMtxVect ascProbVect ascScripVect ascSimsVect ascSolvVect
  global ascToolVect ascUnitVect ascplotvect env
  global ascViewWindowVect
  global ascViewSaveWindowVect
  global ascParPageVect

  Set_ViewVect_Values
  Set_View_Option_Values
  View_WindowInitDialog $window

  # set vectorname $ascViewWindowVect($window)
  # ascParPage ascViewSaveWindowVect [setpos [set [set vectorname](windowname)] 0 0] 1 0
  # just do it for appearances only
  View_Save_Window_Appearance $window
}


#
# proc View_Save_Window_Appearance {window}
#------------------------------------------------------------------------
# save window.a4o appearance options overwriting the previous file
#------------------------------------------------------------------------
proc View_Save_Window_Appearance {window} {

  global ascViewSaveWindowVect ascGlobalVect
  set outfile $ascGlobalVect(userhome)/$window.a4o
  if {[catch {set ascendfile [open $outfile w]} ]} {
    puts "Error writing $outfile "
    return
  }
  if {![info exists ascViewSaveWindowVect(fonts)]} {
    set ascViewSaveWindowVect(fonts) 1
  }
  if {![info exists ascViewSaveWindowVect(colors)]} {
    set ascViewSaveWindowVect(colors) 1
  }
  if {![info exists ascViewSaveWindowVect(windowpositions)]} {
    set ascViewSaveWindowVect(windowpositions) 1
  }

  set windowproc [View_Get_Proc_Name $window]
  set option_list [View_Appearance $window]

  puts $ascendfile "set read_appearance error-$window"
  foreach opt $option_list {
    set value [$windowproc $opt]

    if {[string first "font" [string tolower $opt]] != -1} {
      if {$ascViewSaveWindowVect(fonts) == 0} {
        continue;
      }
    }

    if {  $opt == "bg" || $opt == "fg" ||
          $opt == "abg" || $opt == "afg" ||
          $opt == "sbg" || $opt == "sfg" ||
          $opt == "tbg" || $opt == "tfg" } {
      if {$ascViewSaveWindowVect(colors) == 0} {
        continue;
      }
    }
    
    if {$opt == "geometry" || $opt == "minsize" || $opt == "maxsize"} {
      if {$ascViewSaveWindowVect(windowpositions) == 0} {
        continue;
      }
      if {$opt == "geometry"} {
	if {[wm state [$windowproc windowname]] != "withdrawn" } {
	  set value1 [wm geometry [$windowproc windowname]]
          # disallow saving negative window positions. flip signs.
          regsub -all -- - $value1 + value
	}
      }
    }

    puts $ascendfile "$windowproc $opt $value"
  }

  catch {
    puts $ascendfile "\# Saved [clock format [clock clicks]] on [info hostname]"
  }
  puts $ascendfile "update"
  puts $ascendfile "set read_appearance OK"
  close $ascendfile
  puts stdout "Wrote $window appearance file $outfile."

  set ascViewSaveWindowVect(fonts) 1

  return
}

#
# proc View_Save_Window_Options {window}
#------------------------------------------------------------------------
# save window_opt.a4o appearance options overwriting the previous file
#------------------------------------------------------------------------
proc View_Save_Window_Options {window} {

  global ascViewSaveWindowVect ascGlobalVect
  set outfile $ascGlobalVect(userhome)/${window}_opt.a4o
  if {[catch {set ascendfile [open $outfile w]} ]} {
    puts "Error writing $outfile "
    return
  }

  set windowproc [View_Get_Proc_Name $window]
  set option_list [View_Options $window]

  foreach opt $option_list {
    set value [$windowproc $opt]
    puts $ascendfile "$windowproc $opt $value"
  }

  catch {
    puts $ascendfile "\# Saved [clock format [clock clicks]] on [info hostname]"
  }
  puts $ascendfile "update"
  puts $ascendfile "set read_options OK"
  close $ascendfile
  puts stdout "Wrote $window options file $outfile."

  return
}

#
# proc View_Save_SpecialWindow_Values {opt_win}
#------------------------------------------------------------------------
# save opt_win.a4o  overwriting the previous file. This function should
# be called only for those windows which are dynamically created:
# Matrix, ascplot and debugger. It is also
# used for environment vectors like env and global. The window utilities
# already have its own save options button.
#------------------------------------------------------------------------
proc View_Save_SpecialWindow_Values {opt_win} {
entertrace

  global ascViewSaveVect
  global ascGlobalVect

  if {$ascGlobalVect(saveoptions) == 0} {
    return
  }
  set outfile $ascGlobalVect(userhome)/$opt_win.a4o
  if {[catch {set ascendfile [open $outfile w]} ]} {
    puts "Error writing $outfile "
    return
  }

  set windowproc [View_Get_Proc_Name $opt_win]
  set option_list [View_Appearance $opt_win]
  if {![info exists ascViewSaveVect(colors)]} {
    set ascViewSaveVect(colors) 1
  }
  if {![info exists ascViewSaveVect(fonts)]} {
    set ascViewSaveVect(fonts) 1
  }
  if {![info exists ascViewSaveVect(windowpositions)]} {
    set ascViewSaveVect(windowpositions) 1
  }

  foreach opt $option_list {
    set value [$windowproc $opt]

    if {$opt == "font" || $opt == "labelfont" 
        || $opt == "textfont" || $opt == "titlefont" || $opt == "btnfont" 
        || $opt == "entryFont" || $opt == "textFont" } {
      if {$ascViewSaveVect(fonts) == 0} {
        continue;
      }
    }

    if {  $opt == "bg" || $opt == "fg" ||
          $opt == "abg" || $opt == "afg" ||
          $opt == "sbg" || $opt == "sfg" ||
          $opt == "tbg" || $opt == "tfg" } {
      if {$ascViewSaveVect(colors) == 0} {
        continue;
      }
    }

   if {$opt == "geometry" || $opt == "minsize" || $opt == "maxsize"} {
      if {$ascViewSaveVect(windowpositions) == 0} {
        continue;
      }
      if {$opt == "geometry"} {
	if {[wm state [$windowproc windowname]] != "withdrawn" } {
	  set value [wm geometry [$windowproc windowname]]
	}
      }
    }

    puts $ascendfile "$windowproc $opt $value"
  }

  puts $ascendfile "set readoptions OK"
  close $ascendfile
  puts stdout "Wrote $opt_win options file $outfile."

  return

leavetrace
}

#
# proc View_Save_Windows_Values {}
#------------------------------------------------------------------------
# save files *.a4o for all of the windows in the window list
#------------------------------------------------------------------------
proc View_Save_Windows_Values {} {
entertrace
  global env

  set window_list [list \
    browser display library \
    probe script solver units toolbox
  ]

  foreach window $window_list {
    View_Save_Window_Appearance $window
  }

  return
leavetrace
}


#
# proc View_Save_Interface_Values {}
#------------------------------------------------------------------------
# save current interface settings
#------------------------------------------------------------------------
proc View_Save_Interface_Values {} {
entertrace
  global ascViewSaveVect ascViewSaveWindowVect

  set ascViewSaveWindowVect(fonts) 1
  set ascViewSaveWindowVect(windowpositions) 1
  set ascViewSaveWindowVect(colors) 1

  catch {
    set ascViewSaveWindowVect(fonts) $ascViewSaveVect(fonts)
    set ascViewSaveWindowVect(windowpositions) \
      $ascViewSaveVect(windowpositions)
    set ascViewSaveWindowVect(colors) $ascViewSaveVect(colors)
  }
  
  # View_Save_Values
  View_Save_SpecialWindow_Values global
  View_Save_Windows_Values
  # catch those options which don't have buttons for saving yet.
  View_Save_Window_Options global
  View_Save_Window_Options script

  return
leavetrace
}

#
# proc View_Save_Solver_Params {number}
#------------------------------------------------------------------------
# save solverx_params.a4o  overwriting the previous file
#------------------------------------------------------------------------
proc View_Save_Solver_Params {number} {
entertrace
  global ascSolvVect
  set name $ascSolvVect(name.$number)
  global ascSolv${name}Vect ascGlobalVect
  set outfile $ascGlobalVect(userhome)/${name}params.a4o
  if {[catch {set ascendfile [open $outfile w]} ]} {
    puts "Error writing $outfile "
    return
  }

  puts $ascendfile "global ascSolv${name}Vect"

  set par_list [set ascSolv${name}Vect(namelist) ]

  foreach par $par_list {
      set value [ set ascSolv${name}Vect($par) ]
    puts $ascendfile "set ascSolv${name}Vect($par) $value"
  }

  puts $ascendfile "set readoptions OK"
  close $ascendfile
  puts "Wrote ${name}params options file $outfile."

  return
leavetrace
}


#
# proc View_Save_Solvers_Params {}
#------------------------------------------------------------------------
# save solvers_params.a4o  overwriting the previous file
#------------------------------------------------------------------------
proc View_Save_Solvers_Params {} {
entertrace
  global ascSolvVect

  set number $ascSolvVect(numberofsolvers)

  for {set j 0} {$j < $number} {incr j} {
    set solver $j
    View_Save_Solver_Params $solver
  }
  View_Save_Window_Options solver
  View_Save_Window_Options slv32767parms
  return
leavetrace
}

#
# proc View_Source_Option_Files {}
#------------------------------------------------------------------------
# source option files
#------------------------------------------------------------------------
proc View_Source_Option_Files {} {
entertrace

  global ascGlobalVect errorInfo
  set file_list [list \
    global browser debugger display library \
    probe script solver slv32767parms toolbox units matrix ascplot \
    global_opt browser_opt debugger_opt display_opt library_opt \
    probe_opt script_opt solver_opt slv32767parms_opt \
    toolbox_opt units_opt matrix_opt \
    ascplot_opt
  ]
  foreach optionfile $file_list {
    set infile $ascGlobalVect(userhome)/${optionfile}.a4o
    if { [ file readable $infile ]} {
      if {[catch {asc_source $infile} aderror]} {
      } else {
        puts "source $infile"
      }
      # puts $aderror
    }
  } 
  return
leavetrace
}

#
# proc View_Source_Params_Files {}
#------------------------------------------------------------------------
# source solver option files
#------------------------------------------------------------------------
proc View_Source_Params_Files {} {
entertrace

  global ascSolvVect ascGlobalVect

  set number $ascSolvVect(numberofsolvers)

  for {set j 0} {$j < $number} {incr j} {

    set name $ascSolvVect(name.$j)
    set optionfile  ${name}params

    set infile $ascGlobalVect(userhome)/${optionfile}.a4o
    if { [ file exists $infile ]} {
      if {[catch {asc_source $infile} aderror]} {
      } else {
        puts "source $infile"
      }
     # puts $aderror
    }
  }
  return
leavetrace
}

#
# proc View_Help {}
#------------------------------------------------------------------------
# source solver option files
#------------------------------------------------------------------------
proc View_Help {} {
entertrace

  bell
  puts "No help available for Save options"

  return
leavetrace
}
