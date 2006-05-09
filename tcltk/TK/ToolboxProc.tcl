#  ToolboxProc.tcl: Code for the toolbox buttons and aliases
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.16 $
#  Last modified on: $Date: 1998/06/18 15:55:03 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: ToolboxProc.tcl,v $
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
# proc set_Toolbox_Defaults {}
#------------------------------------------------------------------------
# startup  toolbox vars
#------------------------------------------------------------------------
proc set_Toolbox_Defaults {} {

  global ascToolVect ascGlobalVect {xfShowWindow.toolbox}
  global env ascScripVect
  if {${xfShowWindow.toolbox}} {

 #   puts "setting toolbox buttons"
    set ascToolVext(windowname) .toolbox
    if {[info exists ascGlobalVect(userhome)] && \
        [file isdirectory $ascGlobalVect(userhome)]} {
      if {![info exists ascScripVect(developer)] ||
          !$ascScripVect(developer)} {
        cd $ascGlobalVect(userhome)
      }
    }
    set ascToolVect(dirargs) [pwd]
    set ascToolVect(dirinput) [pwd]
    set ascToolVect(printargs) "xa2ps -Pmirage"
    set ascToolVect(exitargs) ""

    global ascToolVect

    set ascToolVect(utilBtn) .toolbox.bottom_frm.util_btn
    set ascToolVect(helpBtn) .toolbox.bottom_frm.hlp_btn
    set ascToolVect(exitBtn) .toolbox.bottom_frm.exit_btn
    set ascToolVect(librBtn) .toolbox.bottom_frm.library_btn
    set ascToolVect(browBtn) .toolbox.bottom_frm.browser_btn
    set ascToolVect(solvBtn) .toolbox.bottom_frm.solver_btn
    set ascToolVect(probBtn) .toolbox.bottom_frm.probe_btn
    set ascToolVect(unitBtn) .toolbox.bottom_frm.units_btn
    set ascToolVect(dispBtn) .toolbox.bottom_frm.display_btn
    set ascToolVect(scripBtn) .toolbox.bottom_frm.script_btn

    # set ptr
    .toolbox configure -cursor left_ptr

    Configure_Toolbox

  } else {
    puts "toolbox doesn't exist! buttons not set"
  }
}

#
# proc Configure_Toolbox {}
#------------------------------------------------------------------------
# Now attaching some do_raise_lower procedures to the items of
# the Toolboxand the Toolbox itself. -- attached to its label.!!
#------------------------------------------------------------------------
proc Configure_Toolbox {} {
  global ascToolVect

  $ascToolVect(librBtn)   configure -command {Toggle_Remote ascLibrVect}
  $ascToolVect(browBtn)   configure -command {Toggle_Remote ascBrowVect}
  $ascToolVect(solvBtn)  configure -command {Toggle_Remote ascSolvVect}
  $ascToolVect(probBtn)  configure -command {Toggle_Remote ascProbVect}
  $ascToolVect(unitBtn)  configure -command {Toggle_Remote ascUnitVect}
  $ascToolVect(dispBtn)   configure -command {Toggle_Remote ascDispVect}
  $ascToolVect(scripBtn) configure -command {do_raise_lower .script}

  $ascToolVect(helpBtn)  configure -command Tool_do_Help
  $ascToolVect(exitBtn)   configure -command Tool_exit
}


#
# proc Tool_do_UtilBox {}
#------------------------------------------------------------------------
# utility box button
#------------------------------------------------------------------------
proc Tool_do_UtilBox {} {
  VShowWindow.util 0
}

#
# proc Tool_do_Bugs {}
#------------------------------------------------------------------------
# bug mail call button
#------------------------------------------------------------------------
proc Tool_do_Bugs {} {
  Help_button toolbox.bugreport
}

#
# proc Tool_do_Help {}
#------------------------------------------------------------------------
# help button for toolbox. starts up help
#------------------------------------------------------------------------
proc Tool_do_Help {} {
  Help_Open
}

#
# proc Tool_do_Callbacks {}
#------------------------------------------------------------------------
# callbacks button for toolbox. starts up callbacks
#------------------------------------------------------------------------
proc Tool_do_Callbacks {} {
  global ascScripVect
  if {![info exists ascScripVect(developer)]} {
    error "Internals browser disabled by default. Set ascScripVect(developer) to enable it."
  }
  Callback_Open
}

#
# proc Tool_set_dir {}
#------------------------------------------------------------------------
# should be modified to work with util box
#------------------------------------------------------------------------
proc Tool_set_dir {} {

  global ascToolVect

  if {$ascToolVect(dirinput) == ""}  {
    set ascToolVect(dirinput) [pwd];
  }
  if {[file isdirectory $ascToolVect(dirinput)]} {
    set ascToolVect(dirargs) $ascToolVect(dirinput);
    return;
  } else {
    set errmsg "Directory\n"
    append errmsg $ascToolVect(dirinput)
    append errmsg "\n not found!"
    Script_Raise_Alert $errmsg "Path Error"
    set ascToolVect(dirinput) $ascToolVect(dirargs)
    error "bad directory";
  }
}

#
# proc Tool_printinstr
#------------------------------------------------------------------------
# sanity check on print option string. weak.
#------------------------------------------------------------------------
proc Tool_printinstr {} {

  global ascToolVect
  global env
  if {$ascToolVect(printargs) == ""}  {
    set ascToolVect(printargs) "lpr -P $env(PRINTER)"
    return;
  }
}

#
# proc Tool_ExitGeom {}
#------------------------------------------------------------------------
# calc some misc geometry for exit widget to be near exit button
#------------------------------------------------------------------------
proc Tool_ExitGeom {} {

  #set geom [wm geometry .toolbox]
  #set data [split $geom "x+"]
  #set xpos [lindex $data 0]
  #set ypos [lindex $data 1]
  #set xoff [lindex $data 2]
  #set yoff [lindex $data 3]
  #set centrx [expr $xpos / 2 + $xoff - 190 / 2]
  #set centry [expr $ypos / 2 + $yoff]
  #return "$centrx\+$centry"
  return [setpos .toolbox.bottom_frm.exit_btn 20 12]
}

#
# proc ExitProbe
#------------------------------------------------------------------------
# flush probe C structures.  call on ascend exit only.
#------------------------------------------------------------------------
proc ExitProbe {} {
  __probe destroy
}
#
# proc ExitLibrary {}
#------------------------------------------------------------------------
# flush the library and interface module lists. call on ascend exit only.
#------------------------------------------------------------------------
proc ExitLibrary {} {
  libr_destroy_types
  libr_destroy_libr
}

#
# proc ExitUnits {}
#------------------------------------------------------------------------
# flush the library and interface module lists. call on ascend exit only.
#------------------------------------------------------------------------
proc ExitUnits {} {
  u_destroy_units
}

#
# proc ExitIVP {}
#------------------------------------------------------------------------
# empty integrate interface buffers steps and filenames
#------------------------------------------------------------------------
proc ExitIVP {} {
  integrate_set_samples
  integrate_set_y_file ""
  integrate_set_obs_file ""
}

#
# proc Tool_exit {}
#------------------------------------------------------------------------
# the exit button callup
#------------------------------------------------------------------------
proc Tool_exit {} {

  set position [Tool_ExitGeom]
  set res [VShowWindow.ascConfirm "240x50$position" "Exit"]
  if {$res == 1} {
    Tool_exit_internal
  }
}
#
# proc Tool_exit_internal {}
#------------------------------------------------------------------------
# the exit/confirm button actions
#------------------------------------------------------------------------
proc Tool_exit_internal {} {

  catch {Solve_do_Flush}
  catch {ExitProbe}
  catch {ExitLibrary}
  catch {ExitUnits}
  ExitIVP
  destroy .
  catch {user_shutdown}
}
