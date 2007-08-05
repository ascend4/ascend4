#  ScriptProc.tcl: Tcl code for Script window
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.89 $
#  Last modified on: $Date: 2003/03/20 21:26:19 $
#  Last modified by: $Author: aw0a $
#  Revision control file: $RCSfile: ScriptProc.tcl,v $
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
# proc ascconsole_open {}
#------------------------------------------------------------------------
# opens up a console, if it can.
#------------------------------------------------------------------------
proc ascconsole_open {} {
  global ascScripVect
  if {[catch {console show} err]} {
    asctk_dialog .consolerr $ascScripVect(font) FYI \
      "This version was not built with a console" "" 0 OK
    return
  }
  console title {Ascend console}
}

#
# ascresort_filetypes {arrayname extelt}
#------------------------------------------------------------------------
# This function takes the name of a global array and, assuming the elements
# $extelt, basefiletypes, and filetypes appear, reorders the
# definition of filetypes so that the entry of basefiletypes
# indicated in lastextension comes
# first while all the others come in the order of basefiletypes.
# This is so tk_GetOpen/SaveFile can 'remember' where they last were.
# This implementation is very ugly.
#------------------------------------------------------------------------
proc ascresort_filetypes {an extelt} {
  global $an
  set le [set ${an}($extelt)]
  set firsttype ""
  if {$le == ""} { return }
  foreach i [set ${an}(basefiletypes)] {
    set elist [lindex $i 1]
    if {[lsearch -exact $elist $le] != -1} {
      set firsttype $i
      break
    }
  }
  if {[string compare $firsttype ""] == 0} {
    # this ought not be possible, but sometimes happens.
    set firsttype [lindex [set ${an}(basefiletypes)] 0]
  }
  set ${an}(filetypes) ""
  lappend ${an}(filetypes) $firsttype
  foreach i [set ${an}(basefiletypes)] {
    lappend  ${an}(filetypes) $i
  }
}

#
# clears the interrupt flag of the script.
#
proc Script_ClearInterrupt {} {
  global ascScripVect
  set ascScripVect(menubreak) 0
}
#
# proc Script_CheckInterrupt {}
#------------------------------------------------------------------------
# returns an error (do not surround this call with catch)
# if the user interrupt has been detected. Clears that interrupt, too.
# All script that want to play nice should start with this call.
#------------------------------------------------------------------------
proc Script_CheckInterrupt {} {
  global ascScripVect
  if {$ascScripVect(menubreak) != 0} {
    Script_ClearInterrupt
    error "User interrupted the script"
  }
}

proc Toggle_Remote {a} {
  global $a
  if {[info exists ${a}(window.open)] == 0} {
    trace vdelete ${a}(window.open) w Toggle_Window
    if {[winfo ismapped [set ${a}(windowname]]} {
      set ${a}(window.open) 1
    } else {
      set ${a}(window.open) 0
    }
    trace variable ${a}(window.open) w Toggle_Window
  }
  if {[set ${a}(window.open)]} {
    set ${a}(window.open) 0
  } else {
    set ${a}(window.open) 1
  }
}

proc Toggle_Window {a s m} {
  global $a
  do_raise_lower [set ${a}(windowname)]
}

#
# proc set_Script_Defaults {}
#------------------------------------------------------------------------
# standard startup once window is created
#
# ascScripVect is also home to script window state information.
# in particular file menu data.
# ascScripVect(curbufnum) is the presently viewed buffer. -1 is the
# bogus initialization value.
# ascScripVect(maxbufnum) is the highest available buffer num.
# ascScripVect(buffile.$c) is the filename corresponding to buffer $c
# ascScripVect(bufopen.$c) is the closed/open status of the buffer.
#   note we need some file menu reconstruction if close is to be used.
#------------------------------------------------------------------------
proc set_Script_Defaults {} {
 # puts "setting script buttons"
  global ascScripVect env ascToolVect
  set ascScripVect(filename) "[pwd]/."
  Script_ClearInterrupt
  # ascScripVect(menubreak) is a linked to C int variable.
  set ascScripVect(Record) 0
  trace variable ascScripVect(Record) w Script_record_label
  set ascScripVect(executing) 0
  set ascScripVect(count) 0
  set ascScripVect(initialized) "FALSE"
  set ascScripVect(keywords) ""
  set ascScripVect(lastimportextension) ".s"
  set ascScripVect(lastreadextension) ".a4s"
  set ascScripVect(basefiletypes) {
    {{ASCEND scripts} {.a4s .s}  }
    {{Tcl scripts} {.t .tcl}  }
    {{Most} {.*}  }
    {{All} {*}  }
  }
  global ascLibrVect ascSimsVect ascBrowVect ascProbVect
  global ascSolvVect ascDispVect ascUnitVect ascToolVect
  trace variable ascLibrVect(window.open) w Toggle_Window
  trace variable ascSimsVect(window.open) w Toggle_Window
  trace variable ascBrowVect(window.open) w Toggle_Window
  trace variable ascProbVect(window.open) w Toggle_Window
  trace variable ascSolvVect(window.open) w Toggle_Window
  trace variable ascDispVect(window.open) w Toggle_Window
  trace variable ascUnitVect(window.open) w Toggle_Window
  trace variable ascToolVect(window.open) w Toggle_Window

  set ascScripVect(filetypes) $ascScripVect(basefiletypes)

  set ascScripVect(fileBtn)   .script.menubar.file
  set ascScripVect(editBtn)   .script.menubar.edit
  set ascScripVect(execBtn)   .script.menubar.execute
  set ascScripVect(RecordBtn) .script.check_frm.record_btn
  set ascScripVect(fileentry) .script.check_frm.file_entry
  # check for cmu. normally unset outside cmu.
  if {[string first cmu.edu [info hostname]] != -1 || \
       [string first gams.com [info hostname]] != -1} {
    set ascScripVect(developer) 1
  }
 # set ascScripVect(scripBox)      now done in ScriptSwitchBuf
 # buffer manager inits
  set ascScripVect(mainframe) .script.main_frm
  set ascScripVect(curbufnum) -1
  set ascScripVect(maxbufnum) 0
  ScriptSwitchToNewBuf 0 "License-Warranty.tcl"
  Script_Read_File $env(ASCENDTK)/License-Warranty.tcl
  Configure_Script
}

#
# proc Configure_Script {}
#------------------------------------------------------------------------
# set script bindings and some other misc stuff like the keywords list
#------------------------------------------------------------------------
proc Configure_Script {} {
  global ascScripVect
  if {$ascScripVect(initialized) == "TRUE"} {
    return;
  }

  bind $ascScripVect(fileentry) <F3> ScriptFile_do_Copy

  # Update Enabled/Disabled entries when a menu is posted
  #
  $ascScripVect(editBtn) configure \
    -postcommand Script_Update_Edit_Buttons

  $ascScripVect(execBtn) configure \
    -postcommand Script_Update_Exec_Buttons

  $ascScripVect(fileBtn) configure \
    -postcommand Script_Update_File_Buttons

  .script.menubar.view configure \
    -postcommand Script_Update_View_Buttons

  # ScriptAddRightMenu

  $ascScripVect(scripBox) insert 1.0 "\n"
  # All registered keywords are expected to call Script_CheckInterrupt
  # as they start execution.

  set ascScripVect(keywords) [list \
    READ \
    COMPILE \
    DISPLAY \
    RUN \
    PRINT \
    DELETE \
    SOLVE \
    INTEGRATE \
    RESTORE \
    WRITE \
    MERGE \
    REFINE \
    RESUME \
    SAVE \
    PLOT \
    SHOW \
    OBJECTIVE \
    BROWSE \
    DISPVAL \
    DISPATTR \
    ASSIGN \
    PROBE \
    ASCPLOT \
    EXIT]
}

proc ScriptAddRightMenu {} {
  global ascScripVect
  # build right popup menu
  ascRightMouseAddCommand $ascScripVect(scripBox) ScriptSelectState \
    command -label "Execute selected statements" \
    -underline -1 -command Script_do_ExecuteStats

  ascRightMouseAddCommand $ascScripVect(scripBox) ScriptSelectState \
    command -label "Step through statements selected" \
    -underline -1 -command {Script_do_ExecuteStats 0}

  ascRightMouseAddCommand $ascScripVect(scripBox) normal \
    checkbutton -variable ascScripVect(Record) \
    -offvalue {0} \
    -onvalue {1} \
    -label {Record actions} \
    -underline -1   

  ascRightMouseAddCommand $ascScripVect(scripBox) normal \
    command -label "Select all" \
    -underline -1 -command Script_do_SelectAll

  ascRightMouseAddCommand $ascScripVect(scripBox) ScriptSaveState \
    command -label "Save" \
    -underline -1 -command Script_do_WriteBuf 

  ascRightMouseAddCommand $ascScripVect(scripBox) normal \
    command -label "Exit ASCEND..." \
    -underline -1 -command Script_do_Exit
}

proc Script_do_SaveOptions {} {
  global ascScripVect 
  # since its appearance only, just do it.
  View_Save_Interface_Values
  # ascParPage ascViewSaveVect [setpos $ascScripVect(windowname) 0 0] 1 0
}
#
#---------------------------------------------------------------------------
#buffer management stuff
#---------------------------------------------------------------------------

proc ScriptSwitchBuf {num {fname ""}} {
  ScriptSwitchToNewBuf $num $fname
}
#
# proc ScriptSwitchToNewBuf {num {fname ""}}
#---------------------------------------------------------------------------
# valid num are 1..infinity integer.
# caller is expected to keep track of what good nums are.
# Expected to be set unless num is new:
# ascScripVect(bufopen.$num) open/closed status of prior buffers.
#   could be used to make a reopen menu...
# ascScripVect(buffile.$num) the filename of numbered buffer
# ascScripVect(mainframe) the name of the parent for all script_box widgets
# ascScripVect(curbufnum) the number of the buffer in use currently
#
# switches to the numbered buffer. if the buffer isn't open,
# creates it. If fname is given != "" on a closed buffer,
# will read the buffer from disk, eventually...
# as we can see, this function needs to be decomposed.
#---------------------------------------------------------------------------
proc ScriptSwitchToNewBuf {num fname} {
  global ascScripVect ascGlobalVect
  if {$ascScripVect(curbufnum) == $num} {
    return
  }
  set filename "foobar"
  set parentname $ascScripVect(mainframe)
  if {$fname == ""} {
    set filename NoName$num.s
    set ascScripVect(buffile.$num) $filename
    set ascScripVect(bufopen.$num) 0
  } else {
    set filename $fname
  }
  set oldnum $ascScripVect(curbufnum)
  set oldbufname .script_box_$oldnum
  catch {pack forget $parentname$oldbufname}
  set ascScripVect(curbufnum) $num
  set ascScripVect(scripBox) $parentname.script_box_$num.text2

  build_scriptbox $parentname $num $ascScripVect(font)

  # pack widget $parentname
  pack append $parentname \
    $parentname.script_box_$num {top frame center expand fill}
  $ascScripVect(fileBtn) add command  \
     -command "ScriptSwitchToOldBuf $num \{$filename\}" \
     -label $filename
  set ascScripVect(bufopen.$num) 1
  $parentname.script_box_$num.text2 insert end {}

  incr ascScripVect(maxbufnum)
  ScriptAddRightMenu
  update
  update idletasks
}


proc ScriptSwitchToOldBuf {num fname} {

  global ascScripVect 

  set parentname $ascScripVect(mainframe)
  set oldnum $ascScripVect(curbufnum)
  set oldbufname .script_box_$oldnum
  catch {pack forget $parentname$oldbufname}
  set ascScripVect(curbufnum) $num
  set ascScripVect(filename) "$fname"
  set ascScripVect(scripBox) $parentname.script_box_$num.text2
  pack append $parentname \
    $parentname.script_box_$num {top frame center expand fill}

  update
  update idletasks
}

#
#---------------------------------------------------------------------------
# ASCEND Script keyword implementations:
# Script keywords are commands defined for ASCEND (in CAPS) which may be
# used on the commandline or in the Script. Keywords are actually Tcl
# functions which encapsulate 1 or more of the C primitives and other
# Tcl procedures so the user can conveniently emulate button presses.
# Each keyword takes 0 or more arguments.
#
# <arg> indicates the use of arg is NOT optional.
# <a1,a2> indicates that the use of either a1 or a2 is required
# <a1 a2> indicates use of both a1 and a2 required. Usually written <a1> <a2>
# [a1] indicate the use of a1 is optional.
# [a,b] indicates that either a or b is optional but not both.
#
# qlfdid is short for 'QuaLiFieD IDentifier'
# qlfpid is short for 'QuaLiFied Procedure IDentifier'
#
# OF, WITH, TO, and other args in all CAPS are modifiers to the keyword
# which make it do different things.
#
# It is generally best to enclose all object names and units in {braces} to
# prevent Tcl from performing string substitution or otherwise operating
# on the arguments before passing them to the keyword function.
#
# Quick reference:
#   ASSIGN     set the value of something atomic
#   ASCPLOT    generate a defaulted graph from input file
#   BROWSE     export an object to the browser
#   COMPILE    compile a simulation of a given type
#   DELETE     delete a simulation or the type library or the solver MODEL
# * DISPLAY    display something
#   EXIT       exit ascend
#   INTEGRATE  run an IVP integrator
#   MERGE      perform an ARE_THE_SAME
#   PLOT       create a plot file
#   PRINT      print one of the printable windows
#   PROBE      export an object to the probe
#   READ       read in a model, script, or values file.
#   REFINE     perform an IS_REFINED_TO
# * RESTORE    read a simulation from disk.
#   RESUME     resume compiling a simulation
#   RUN        run a procedure
# * SAVE       write a simulation to disk
#   SHOW       call a unix plot program on a file from PLOT
#   SOLVE      run the solver
#   WRITE      write values in Tcl format to disk
#
#---------------------------------------------------------------------------
#
# proc ASSIGN <qlfdid> <value> [units]
#------------------------------------------------------------------------
# set the value of atom 'qlfdid' from the script. If value is real, give a set
# of units compatible with the dimensions of the variable. If the variable
# has no dimensions yet, ASSIGN will fix the dimensions.
#------------------------------------------------------------------------
proc ASSIGN {qlfdid args} {
  Script_CheckInterrupt
  set argc [llength $args]
  switch $argc {
    {1} {set val $args; set units ""}
    {2} {set val [lindex $args 0]
         set units [lindex $args 1]
        }
    default {error "ASSIGN expected: qlfdid value \[units\]"}
  }
  qassgn3 $qlfdid $val $units
  if {$val=="FALSE" || $val =="TRUE" || $val=="false" || $val =="true" } {
    HUB_Message_to_HUB WHENVARUPDATED $qlfdid
    HUB_Message_to_HUB BOOLEANUPDATED $qlfdid
  } else {
    HUB_Message_to_HUB VARIABLEUPDATED $qlfdid
  }
  HUB_Message_to_HUB VALUESET $qlfdid $val $units
}

#
# proc BROWSE <qlfdid>
#------------------------------------------------------------------------
# export qlfdid to the browser
#------------------------------------------------------------------------
proc BROWSE {qlfdid} {
  Script_CheckInterrupt
  global ascBrowVect
  if {$ascBrowVect(visibility)} {newraise .browser}
  Brow_Export_Any_2Browser $qlfdid
}


#
# proc READ [FILE,<VALUES,SCRIPT>] <filename>
#------------------------------------------------------------------------
# Load a file from disk.
# Searches for files in directories (Working directory):.:$ASCENDLIBRARY
# unless a full path name is given for filename.
# FILE indicates ASCEND source code (.asc usually)
# VALUES indicates variable data written by WRITE VALUES (.values usually)
# SCRIPT indicates a file to load at the end of the Script window. (.a4s,.s)
# If neither VALUES nor SCRIPT found, FILE will be assumed.
#
# Note: You will get quite a spew from the parser if you leave out the
# SCRIPT or VALUES modifier by accident.
# Capitalization on file,script, and values will be ignored.
#
#------------------------------------------------------------------------
proc READ {args} {
  Script_CheckInterrupt
  global ascLibrVect env ascToolVect ascBrowVect
  set argc [llength $args]
  set type ""
  set force 0
  switch $argc {
    {1} {
      set file $args
      set type file
      set extension "[file extension $file]"
      set found 0
      if {$extension != ""} {
        set found \
          [expr [lsearch -exact [libr_query -filetypes] $extension] != -1]
      }
      if {!$found && $extension != ".patch"} {
	set geom 200x120+480+200
	set errmsg \
	    "File $file\n may not be a valid ASCEND model file.\nContinue ?"
	set btn [Script_Raise_Alert $errmsg "Odd file name"]
	if {$btn} {return}
      }
    }
    {2} {
      set file [lindex $args 1]
      set type [lindex $args 0]
      set type [string tolower $type]
      if {$type != "file" && $type != "values" && $type != "script"} {
	puts stderr "Non-fatal script error: [lindex $args 0] found."
	puts stderr "FILE, SCRIPT or VALUES expected. FILE assumed."
	set type FILE
      }
    }
    {3} {
      set file [lindex $args 1]
      set type [lindex $args 0]
      set type [string tolower $type]
      set nc [string tolower [lindex $args 2]]
      if {$type != "values"} {
        error "READ expected VALUES <filename> NOCONFIRM"
      }
      if {[string compare $nc noconfirm]} {
        error "READ expected VALUES <filename> NOCONFIRM"
      }
      set force 1
    }
    default {
      error "READ expected [FILE,VALUES,SCRIPT] <filename> [NOCONFIRM]"
    }
  }
  set badname $file
  set file [ascFindFile $file first \
    $ascToolVect(dirinput) "." $env(ASCENDLIBRARY)]
  if {$file==""} {
    set mesg "READ file \""
    append mesg $badname
    append mesg "\" not found in . or \n"
    append mesg $ascToolVect(dirinput)
    append mesg " or any of \n"
    append mesg $env(ASCENDLIBRARY)
    append mesg "\n                Continue?                           "
    set btn [Script_Raise_Alert $mesg]
    if {$btn} {
      error "Cannot continue without file."
    }
  } else {
    puts -nonewline "READing file "
    puts $file
  }
  switch $type {
    {file} {
      global ascLibrVect
      if {$ascLibrVect(visibility)} {newraise .library}
      set nok [catch {Libr_file_get $file} msg]
      if {$nok} {
	error "Problem reading $file: $msg"
      }
    }
    {values} {
      puts "Reading values from $file"
      if {[catch {Brow_parse_values $file $force} err]} {
	puts stderr "Problem reading values file:"
	puts stderr "error>>>$err<<<"
      }
      set ascBrowVect(filename) $file
      #
      # here we should be grabbing the first line of filename, taking its
      # second to last item, and issuing the updated calls with that sim name.
      #
      HUB_Message_to_HUB VARIABLEUPDATED
      HUB_Message_to_HUB BOOLEANUPDATED
      HUB_Message_to_HUB WHENVARUPDATED
      HUB_Message_to_HUB DATAREAD $file
    }
    {script} {
      set extension [file extension $file]
      if {$extension != ".s" && \
          $extension != ".tcl" && \
          $extension != ".a4s"} {
	set geom "200x120+480+200"
	set errmsg \ "File\n"
        append errmsg $file
        append errmsg "\n may not be a valid ASCEND script\n  Continue ?"
        set btn [Script_Raise_Alert $errmsg]
	if {$btn == "1"} {return}
      }
      Script_File_Get $file
    }
  }
}

#
# proc ASCPLOT <filename>
#------------------------------------------------------------------------
# Build a graph from a .dat file.
# Assumes lots of things about the plot because there are lots
# of alternatives.
# If filename is close, closes the ascplot window.
# Returns the number of the last data set from the file read,
# or -1 if closing.
# should put a DELETE option in here to call _unload $args
#------------------------------------------------------------------------
proc ASCPLOT {filename args} {
  global ascplotvect
  switch [string tolower $filename] {
    close {
        ascplot_dook
        return -1
    }
    default {
      ascplot_open
      set dset [ascplot_parse_file $filename]
      if {$dset >= 0} {
        ascplot_drawsets
        ascplot_select_set_by_number $dset
        set ilist {}
        set len [$ascplotvect(varsname) index end]
        for {set i 0} {$i < $len} {incr i} { 
          lappend ilist $i
        }
        ascplot_seldependent_list $ilist
        ascplot_showdata 1
        ascplot_setlegends
        ascplot_viewgraph
        return $dset;
      } else {
        error "ASCPLOT: bad input file \"$filename.\""
      }
    }
  }
}

#
# proc COMPILE <simname> [OF] <type>.
#------------------------------------------------------------------------
# Build a simulation of the type given with name simname.
# You can get away with leaving out OF or spelling it wrong.
#------------------------------------------------------------------------
proc COMPILE {args} {
  Script_CheckInterrupt
  global ascSimsVect
  set argc [llength $args]
  switch $argc {
    {2} {
      set sim [lindex $args 0]
      set type [lindex $args 1]
    }
    {3} {
      set sim [lindex $args 0]
      set type [lindex $args 2]
      if {[lindex $args 1] != "OF"} {
	puts stderr \
            "Non-fatal script error: [lindex $args 1] found. OF expected."
      }
    }
    default {
      error "COMPILE expected <simname> [OF] <type>."
    }
  }
  if {$ascSimsVect(visibility)} {newraise .sims}
  update
  set ascSimsVect(instancetype) $type
  if {![sim_unique $sim]} {
    error "Simulation named $sim already exists!"
  }
  puts stdout "COMPILEing $sim OF $type"
  set nok [catch "sim_instantiate $sim $type" err_msg]
  if {$nok} {
    error "$err_msg"
  }
  HUB_Message_to_HUB SIMCREATED $sim $type
  Sims_update_SimsBox
}

proc PATCH {args} {
  Script_CheckInterrupt
  global ascSimsVect
  set argc [llength $args]
  switch $argc {
    {2} {
      set sim [lindex $args 0]
      set type [lindex $args 1]
    }
    {3} {
      set sim [lindex $args 0]
      set type [lindex $args 2]
      if {[lindex $args 1] != "OF"} {
	puts stderr \
            "Non-fatal script error: [lindex $args 1] found. OF expected."
      }
    }
    default {
      error "PATCH expected <simname> [OF] <patch_type>."
    }
  }
  if {$ascSimsVect(visibility)} {newraise .sims}
  update
  set ascSimsVect(instancetype) $type
  if {![sim_unique $sim]} {
    error "Simulation named $sim already exists!"
  }
  puts stdout "COMPILEing $sim PATCH for $type"
  set nok [catch "sim_instantiate $sim $type -p" err_msg]
  if {$nok} {
    error "$err_msg"
  }
  HUB_Message_to_HUB SIMCREATED $sim $type
  Sims_update_SimsBox
}

#
# proc RUN <qlfpid>
#------------------------------------------------------------------------
# runs the procedure qlfpid as if from the browser Initialize button.
#------------------------------------------------------------------------
proc RUN {qlfpid} {
  Script_CheckInterrupt
  global ascLibrVect
  set id [split $qlfpid .]
  set len [llength $id]
  set pid [lindex $id [expr $len -1]]
  set id [lrange $id 0 [expr $len -2]]
  set qlfdid [join $id .]
  brow_runmethod -method $pid -qlfdid $qlfdid \
                 -backtrace $ascLibrVect(btuifstop) \
                 -stopOnErr $ascLibrVect(ignorestop) 

  HUB_Message_to_HUB PROCRUN $qlfpid
  HUB_Message_to_HUB VARIABLEUPDATED $qlfpid
  HUB_Message_to_HUB WHENVARUPDATED $qlfpid
  HUB_Message_to_HUB BOOLEANUPDATED $qlfpid
}

#
# proc PRINT <PROBE,DISPLAY>
#------------------------------------------------------------------------
# Prints out the Probe or Display text in view.
#------------------------------------------------------------------------
proc PRINT {topid} {
  Script_CheckInterrupt
  switch $topid {
    {PROBE}   { Probe_do_Print; return}
    {DISPLAY} { DispExecutePrint ; return}
    default   { error "PRINT <PROBE or DISPLAY>"}
  }
}

#
# proc CLEAR_VARS <qlfdid>
#------------------------------------------------------------------------
# Sets all fixed flags in qlfdid to FALSE
#------------------------------------------------------------------------
proc CLEAR_VARS {qlfdid} {
  Script_CheckInterrupt
entertrace
Solve_do_Flush do_not_record
free_all_vars $qlfdid
leavetrace
}

#
# proc PROTOTYPE <simulation name>
#------------------------------------------------------------------------
# Creates a PROTOTYPE of the given simulation name
#------------------------------------------------------------------------
proc PROTOTYPE {name} {
  Script_CheckInterrupt
 __sims_proto $name
}


#
# proc DELETE <TYPES,simname,SYSTEM>
#------------------------------------------------------------------------
# Delete all types or delete specified simulation, or flush solver.
# If you name a simulation TYPES or SYSTEM you get what you deserve.
#------------------------------------------------------------------------
proc DELETE {foo args} {
  Script_CheckInterrupt
  switch $foo {
    {TYPES} {Libr_do_DeleteAll 1; return}
    {SYSTEM} {Solve_do_Flush; return}
    default {
      Sims_Delete $foo
      Sims_update_SimsBox
    }
  }
}

#
# proc SOLVE <qlfdid> [WITH] [solvername]
#------------------------------------------------------------------------
# Fires off current solver unless another is specified. WITH optional.
# Whatever is in the solver window gets displaced.
# Solvername must be given as it appears on the menu buttons.
#------------------------------------------------------------------------
proc SOLVE {qlfdid args} {
  Script_CheckInterrupt
entertrace
  global ascSolvVect ascSolvStatVect
  if {[slv_import_qlfdid $qlfdid test]} {
    error "$qlfdid not solvable instance" 
  }
  if {$ascSolvVect(visibility)} {newraise .solver}
  set argc [llength $args]
  switch $argc {
   {0} {
         Solve_Import_Any $qlfdid
         Solve_do_Select QRSlv
         Solve_do_Solve
         leavetrace; return
       }
   {1} { set solname [lindex $args 0]
         # go off of C structure here...
#
# Now we call solvers by name
#
         switch $solname {
           {Slv}    -
	   {slv}    {set solname Slv}
           {MINOS}  -
	   {minos}  {set solname MINOS}
           {QRSlv}  -
	   {qrslv}  {set solname QRSlv}
           {LSSlv}  -
	   {lsslv}  {set solname LSSlv}
	   {NGSlv}  -
	   {ngslv}  {set solname NGSlv}
	   {CONOPT} -
	   {conopt} {set solname CONOPT}
	   {LRSlv} -
	   {lrslv}  {set solname LRSlv}
	   {CMSlv} -
	   {cmslv}  {set solname CMSlv}
           default {error "SOLVE called with $solname. (Unrecognized)"}
         }
         set ascSolvVect(simname) [lindex [split $qlfdid .] 0]
         slv_import_qlfdid $qlfdid
         set ascSolvStatVect(menubreak) 0
         slv_set_haltflag 0
         set ascSolvStatVect(empty) 0
#         Solve_do_Select $numz
         Solve_do_Select $solname
         Solve_Update_Listbox
         Solve_Update_MenuBar
         Solve_Downdate_ParmBox
         if {$ascSolvVect(debuggerup)} {
           Debug_Trace on
         }
         if {$ascSolvVect(mtxup)} {
           Solve_do_DispIncidence
         }
         Solve_do_Solve
         leavetrace; return
       }
   {2} { set WITH [lindex $args 0]
         set solname [lindex $args 1]
         if {[string tolower $WITH] != "with"} {
           puts stderr \
           "Non-fatal script error: $WITH found. WITH expected."
         }
#
# Now we call solvers by name
#
         switch $solname {
           {Slv}    -
	   {slv}    {set solname Slv}
           {MINOS}  -
	   {minos}  {set solname MINOS}
           {QRSlv}  -
	   {qrslv}  {set solname QRSlv}
           {LSSlv}  -
	   {lsslv}  {set solname LSSlv}
	   {NGSlv}  -
	   {ngslv}  {set solname NGSlv}
	   {CONOPT} -
	   {conopt} {set solname CONOPT}
	   {LRSlv} -
	   {lrslv}  {set solname LRSlv}
	   {CMSlv} -
	   {cmslv}  {set solname CMSlv}
           default {
	     error "SOLVE called with $solname. \
                  expected Slv, MINOS. QRSlv, LSSlv, NGSlv, LRSlv, CMSlv"
	   }
         }
         set ascSolvVect(simname) [lindex [split $qlfdid .] 0]
         slv_import_qlfdid $qlfdid
         set ascSolvStatVect(menubreak) 0
         slv_set_haltflag 0
         set ascSolvStatVect(empty) 0
#         Solve_do_Select $num
         Solve_do_Select $solname
         Solve_Update_Listbox
         Solve_Update_MenuBar
         Solve_Downdate_ParmBox
         if {$ascSolvVect(debuggerup)} {
           Debug_Trace on
         }
         if {$ascSolvVect(mtxup)} {
           Solve_do_DispIncidence
         }
         Solve_do_Solve
         leavetrace; return
       }
    default {error "Syntax: SOLVE instance [WITH] [solvername]"}
  }
}

#
# proc OPTIMIZE <objname> <IN> <qlfdid> <WITH> <solvername>
#------------------------------------------------------------------------
# Fires off solvername on qlfdid with obj as the objective function.
# Whatever is in the solver window gets displaced.
# Solvername must be given as it appears on the menu buttons.
#------------------------------------------------------------------------
proc OPTIMIZE {objname in qlfdid with solname} {
  Script_CheckInterrupt
entertrace
  global ascSolvVect ascSolvStatVect
  if {[slv_import_qlfdid $qlfdid test]} {
    error "$qlfdid not solvable instance"
  }
  
  if {$ascSolvVect(visibility)} {newraise .solver}
  switch $solname {
      {Slv} - {slv} {set solname Slv}
      {MINOS} - {minos} {set solname MINOS}
      {QRSlv} - {qrslv} {set solname QRSlv}
      {LSSlv} - {lsslv} {set solname LSSlv}
      {NGSlv} - {ngslv} {set solname NGSlv}
      {CONOPT} - {conopt} {set solname CONOPT}
      {LRSlv} - {lrslv}  {set solname LRSlv}
      {CMSlv} - {cmslv}  {set solname CMSlv}
      default {error "OPTIMIZE called with $solname. (Unrecognized)"}
  }
  set ascSolvVect(simname) [lindex [split $qlfdid .] 0]
  slv_import_qlfdid $qlfdid
  set ascSolvStatVect(menubreak) 0
  slv_set_haltflag 0
  set ascSolvStatVect(empty) 0
  Solve_do_Select $solname
  Solve_Update_Listbox
  Solve_Update_MenuBar
  Solve_Downdate_ParmBox

  set obj_num [Solve_GetObjRelNum $objname]
  catch {slv_set_obj_by_num $obj_num}
  
  if {$ascSolvVect(debuggerup)} {
      Debug_Trace on
  }
  if {$ascSolvVect(mtxup)} {
      Solve_do_DispIncidence
  }
  Solve_do_Solve
  leavetrace; return
}

#
# proc INTEGRATE_syntax {qlfdid args}
#------------------------------------------------------------------------
# INTEGRATE_syntax error message
#------------------------------------------------------------------------
proc INTEGRATE_syntax {qlfdid args} {
  Script_CheckInterrupt
  puts stderr "Error parsing $args."
  puts stderr "Integrator script syntax is:"
  puts stderr "INTEGRATE $qlfdid (assumes range and BLSODE)"
  puts stderr "INTEGRATE $qlfdid WITH integrator (assumes range)"
  puts stderr "INTEGRATE $qlfdid FROM n1 TO n2 (assumes BLSODE)"
  puts stderr "INTEGRATE $qlfdid FROM n1 TO n2 WITH integrator"
  return "INTEGRATE miscalled."
}
#
# proc INTEGRATE {qlfdid args}
#------------------------------------------------------------------------
# Run an integrator on qlfdid. There are several permutations
# on the syntax. It is best to have solved qlfdid before hand to have
# good initial values.
# INTEGRATE qlfdid (assumes BLSODE and entire range)
# INTEGRATE qlfdid WITH (assumes entire range)
# INTEGRATE qlfdid FROM n1 TO n2 (assumes BLSODE)
# INTEGRATE qlfdid FROM n1 TO n2 WITH integrator
# Requires:
#   n1 < n2
#   qlfdid be of an integrable type (a refinement of ivp or blsode-ified.)
#------------------------------------------------------------------------
proc INTEGRATE {qlfdid args} {
  Script_CheckInterrupt
  global ascSolvVect ascSolvStatVect
  if {[slv_import_qlfdid $qlfdid test]} {
    error "$qlfdid not solvable instance"
  }
  qlfdid $qlfdid
 # if {![integrate_able search ivp]} {error "$qlfdid not a refinement of ivp"}
  if {$ascSolvVect(visibility)} {newraise .solver}
  set argc [llength $args]
  switch $argc {
   {0} {
         set ivpsolver BLSODE
         set n1 first
         set n2 last
         Solve_do_Select QRSlv
       }
   {2} {
         set WITH [lindex $args 0]
         if {[string tolower $WITH]!="with"} {
           error "[INTEGRATE_syntax $qlfdid $args]"
         }
         set ivpsolver [lindex $args 1]
         set n1 first
         set n2 last
       }
   {4} {
         set FROM [lindex $args 0]
         if {[string tolower $FROM]!="from"} {
           error "[INTEGRATE_syntax $qlfdid $args]"
         }
         set TO [lindex $args 2]
         if {[string tolower $TO]!="to"} {
           error "[INTEGRATE_syntax $qlfdid $args]"
         }
         set ivpsolver BLSODE
         set n1 [lindex $args 1]
         set n2 [lindex $args 3]
         Solve_do_Select QRSlv
       }
   {6} {
         set FROM [lindex $args 0]
         if {[string tolower $FROM]!="from"} {
           error "[INTEGRATE_syntax $qlfdid $args]"
         }
         set TO [lindex $args 2]
         if {[string tolower $TO]!="to"} {
           error "[INTEGRATE_syntax $qlfdid $args]"
         }
         set WITH [lindex $args 4]
         if {[string tolower $WITH]!="with"} {
           error "[INTEGRATE_syntax $qlfdid $args]"
         }
         set ivpsolver [lindex $args 5]
         if {$ivpsolver == "LSODE" && ![integrate_able search ivp]} {
            error "$qlfdid not a refinement of lsode."
         }
         set n1 [lindex $args 1]
         set n2 [lindex $args 3]
       }
    default {error "[INTEGRATE_syntax $qlfdid $args]"}
  }
  puts "$qlfdid $n1 $n2 $ivpsolver"
  set ascSolvVect(simname) [lindex [split $qlfdid .] 0]
  slv_import_qlfdid $qlfdid
  set ascSolvStatVect(menubreak) 0
  set ascSolvStatVect(empty) 0
  Solve_Update_StatusBox;# <<< was missing from distributed version.
 # Solve_do_Select 0
  Solve_Update_Listbox
  Solve_Update_MenuBar
  Solve_Downdate_ParmBox
  if {$ascSolvVect(debuggerup)} {
    Debug_Trace on
  }
  if {$ascSolvVect(mtxup)} {
    Solve_do_DispIncidence
  }
  Solve_Integrate $ivpsolver $n1 $n2
}

#
# proc RESTORE <file>
#------------------------------------------------------------------------
# reload a simulation from disk
#------------------------------------------------------------------------
proc RESTORE {filename} {
  Script_CheckInterrupt
  error "Restoring simulations not implemented yet.\n"
}

#
# proc WRITE <kind> <qlfdid> <file> [args]
#------------------------------------------------------------------------
# Write something (what sort of write indicated by kind) about
# qlfdid to a file. args may modify as determined by kind.
# At present only VALUES is supported. SYSTEM (for solver dump) would be nice.
# e.g.  WRITE VALUES filename.
#------------------------------------------------------------------------
proc WRITE {kind inst filename args} {
  Script_CheckInterrupt
  set argc [llength $args]
  switch $kind {
    {VALUES} {
      set ascBrowVect(filename) $filename
      set sim [stripbraces [lindex [split $inst .] 0]]
      puts $sim
      if {[catch {qlfdid $inst} err_msg]} {
        error "WRITE failed to find instance specified to it.\n $err_msg"
      }
      bwritevalues $filename "qassgn3 \{" qualified $inst fast
      puts "Wrote values file $filename."
    }
   default {error "WRITE called with unknown output kind $kind"}
  }
}

#
# proc MERGE <qlfdid1> [WITH] <qlfdid2>
#------------------------------------------------------------------------
# ARE_THE_SAME qlfdid1 and qlfdid2 if possible.
#------------------------------------------------------------------------
proc MERGE {qlf1 args} {
  Script_CheckInterrupt
  set argc [llength $args]
  switch $argc {
   {0} {
         error "MERGE requires an instance to merge with"
       }
   {1} {
         if {[lindex [split $qlf1 .] 0] != [lindex [split $args .] 0]} {
           error "MERGE requires two instances in the same simulation"
         }
         HUB_Message_to_HUB INSTANCEMOVED $qlf1
         HUB_Message_to_HUB INSTANCEMOVED $qlf2
         smerge $qlf1 $args
         HUB_Message_to_HUB INSTMERGED $qlf1 $qlf2
         return
       }
   {2} { set WITH [lindex $args 0]
         set qlf2 [lindex $args 1]
         if {[string tolower $WITH] != "with"} {
           puts stderr \
           "Non-fatal script error: $WITH found. WITH expected."
         }
         if {[lindex [split $qlf1 .] 0] != [lindex [split $qlf2 .] 0]} {
           error "MERGE requires two instances in the same simulation"
         }
         HUB_Message_to_HUB INSTANCEMOVED $qlf1
         HUB_Message_to_HUB INSTANCEMOVED $qlf2
         smerge $qlf1 $qlf2
         HUB_Message_to_HUB INSTMERGED $qlf1 $qlf2
         return
       }
    default {error "Syntax: MERGE instance [WITH] instance"}
  }
}

#
# proc REFINE <qlfdid> [TO] <type>
#------------------------------------------------------------------------
# Refine qlfdid to given type if they are conformable.
#------------------------------------------------------------------------
proc REFINE {qlfdid args} {
  Script_CheckInterrupt
  set argc [llength $args]
  switch $argc {
   {0} {
         error "REFINE requires a type to refine the instance to"
       }
   {1} {
         if {![libr_query -exists -type $args]} {
           error "REFINE $qlfdid called with nonexistent type $args"
         }
         HUB_Message_to_HUB INSTANCEMOVED $qlfdid
         srefine $args search $qlfdid
         HUB_Message_to_HUB INSTREFINED $qlfdid $args
         return
       }
   {2} { set TO [lindex $args 0]
         set Type [lindex $args 1]
         if {$TO != "TO"} {
           puts stderr \
           "Non-fatal script error: $TO found. TO expected."
         }
         HUB_Message_to_HUB INSTANCEMOVED $qlfdid
         srefine $Type search $qlfdid
         HUB_Message_to_HUB INSTREFINED $qlfdid $Type
         return
       }
    default {error "Syntax: REFINE instance [TO] [typename]"}
  }
}

#
# proc RESUME <simname>
#------------------------------------------------------------------------
# Reinvoke compiler on simname.
#------------------------------------------------------------------------
proc RESUME {args} {
  Script_CheckInterrupt
  set argc [llength $args]
  switch $argc {
    {0} {
          Browser_do_ResumeCompile
        }
    {1} { set sim [lindex [split [lindex $args 0] .] 0]
          puts stdout "RESUMEing compilation of $sim."
          HUB_Message_to_HUB INSTANCEMOVED $sim
          set i [sim_reinstantiate $sim]
          if {$i!="1"} {
            puts stderr \
            "Unable to find $sim."
          } else {
            HUB_Message_to_HUB INSTANCERESUMED $sim
          }
        }
    default {error "RESUME expected <simname>."}
  }
}

#
# proc SAVE <sim> [TO] <filename>
#------------------------------------------------------------------------
# filename will be assumed to be in Working directory (on utils page)
# unless it starts with a / or a ~
#------------------------------------------------------------------------
proc SAVE {sim args} {
  Script_CheckInterrupt
  error "Saving simulations not implemented yet.\n"
}

#
# proc PLOT <qlfdid> [filename]
#------------------------------------------------------------------------
# Writes plot data from qlfdid, which must be a plottable instance,
# to filename.
#------------------------------------------------------------------------
proc PLOT {qlfdid args} {
  Script_CheckInterrupt
  global ascScripVect ascUtilVect
  set filename ""
  set nok [catch {qlfdid $qlfdid} err_msg]
  if {$nok} {
    error "PLOT: Error in finding instance $qlfdid"
  }
  catch {set args [glob $args]}
  if {$args=="."} {set args ""}
  if {[llength $args]=="1"} {
  # if {[file exists $args]} {error "File $args already exists"}
    set filename $args
  } else {
    set username [ascwhoami]
    set file_prefix $ascUtilVect(asctmp)/asc$username
    set filename [FileUniqueName "$file_prefix.$ascUtilVect(plot_type)"]
  }
  if {[b_isplottable search]} {
    puts stdout "PLOTting to file: $filename"
    b_prepplotfile search $filename $ascUtilVect(plot_type)
    set ascScripVect(lastplot) $filename
    HUB_Message_to_HUB PLOTMADE $qlfdid $filename
  } else { error "$qlfdid is not a plottable kind."}
}

#
# proc SHOW <filename,LAST>
#------------------------------------------------------------------------
# Invokes the plotter program on the filename given or on the file LAST
# generated by PLOT.
#------------------------------------------------------------------------
proc SHOW {filename} {
  Script_CheckInterrupt
  global ascScripVect ascUtilVect
  if {$filename=="LAST"} {
    if {[catch {set ascScripVect(lastplot)} ] || \
        $ascScripVect(lastplot)==""} {
       error "SHOW LAST called without a previous plot existing."
    }
    set filename $ascScripVect(lastplot)
  }
  if {$ascUtilVect(plot_command) != ""} {
    Brow_InvokePlotProgram $filename $ascUtilVect(plot_command)
  } else {error "Plot command not set in utilities window!"}
}

#
# proc OBJECTIVE
#------------------------------------------------------------------------
# semantics of OBJECTIVE that will be supported are unclear as no
# OBJECTIVE other than the declarative one is yet supported
#------------------------------------------------------------------------
proc OBJECTIVE {qlfdid} {
  Script_CheckInterrupt
  error "Select objective not implemented yet.\n"
}

#
# proc DISPLAY <kind> [OF] <qlfdid>
#------------------------------------------------------------------------
# How qlfdid is displayed varies with kind.
# kinds are: VALUE ATTRIBUTES CODE ANCESTRY
#------------------------------------------------------------------------
proc DISPLAY {kind args} {
  Script_CheckInterrupt

  set argc [llength $args]
  switch $argc {
    {1} {set qlfdid $args}
    {2} { set OF [lindex $args 0]
          set qlfdid [lindex $args 1]
          if {$OF != "OF"} {
             puts stderr \
             "Non-fatal script error: $OF found. OF expected."
          }
        }
    default {error "DISPLAY called with unexpected number of args"}
  }
  switch $kind {
    {VALUE} -
    {ATTRIBUTES} -
    {CODE} -
    {ANCESTRY} {
      puts stderr "Script DISPLAY not implemented.\n"
    }
    default {error "DISPLAY called with unknown kind $kind."}
  }
}

#
# proc PROBE <arg1 args>
#------------------------------------------------------------------------
# PROBE ONE qlfdid    exports the item qlfdid to the Probe.
# PROBE ALL qlfdid    exports items found in qlfdid matching
#                     all variables and relations by default.
# PROBE qlfdid        is as PROBE ALL qlfdid.
# PROBE number qlfdid filter-list
#                     imports to the probe indicated by number
#                     from the instance qlfdid. if no filterlist is given,
#                     only the name itself goes to the probe.
# Items always go to currently selected probe context.
#------------------------------------------------------------------------
proc PROBE {arg1 {buf ""} args} {
  global ascScripVect
  global ascProbVect ascBrowVect
  Script_CheckInterrupt
  if {$buf ==""} {
    Probe_Import_Filtered $ascScripVect(windowname) $arg1 1
  } else {
    # grandfather the old scripts
    if {$arg1=="ONE"} {
      Probe_Import current $buf
      return
    }
    if {$arg1=="ALL"} {
     # Probe_Import_Filtered $ascScripVect(windowname) $buf 1
      Probe_Import current $buf 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
      return
    }
    # all new probe uses should go to this.
    Probe_Import $arg1 $buf $args
  }
}

#
# proc EXIT [NOCONFIRM]
#------------------------------------------------------------------------
# EXIT
# EXIT NOCONFIRM
# Quits ASCEND. A confirmation is requested unless NOCONFIRM is given
# If any argument other than NOCONFIRM is given, an error occurs.
#------------------------------------------------------------------------
proc EXIT {args} {
  Script_CheckInterrupt
  if {$args=="NOCONFIRM"} {Tool_exit_internal; return}
  if {$args==""} {Tool_exit; return}
  error "Illegal argument to EXIT"
}

#------------------------------------------------------------------------
# end of script keyword functions
#------------------------------------------------------------------------

#
# proc DISPVAL {qlfname}
#------------------------------------------------------------------------
# print the instance part of qlfname
#------------------------------------------------------------------------
proc DISPVAL {qlfname} {
  Script_CheckInterrupt
   if {$qlfname == ""} {
     Script_Raise_Error "Invalid Name"
     return 0;
   }
   set res [catch {qlfdid $qlfname} err]
   if {$res} {
     Script_Raise_Error $err
     return 1;
   } {
     puts "[__brow_iname search]"
   }
}
#
# proc Script_Raise_Alert
#------------------------------------------------------------------------
# script alertbox call
#------------------------------------------------------------------------
proc Script_Raise_Alert {errmsg {label "Error"} {geom ""}} {
  global ascScripVect
  set errorbtn [asctk_dialog .scripterror $ascScripVect(font) \
                  $label $errmsg "" 0 OK Cancel];
  return $errorbtn
}

#
# proc Script_record_label {args}
#------------------------------------------------------------------------
# script recording variable trace function to configure the recording label
#------------------------------------------------------------------------
proc Script_record_label {args} {
  global ascScripVect
  if {$ascScripVect(Record)} {
    $ascScripVect(RecordBtn) configure -text "Recording"
  } else {
    $ascScripVect(RecordBtn) configure -text " "
  }
}

#
# proc Script_File_Loaded  {filename}
#------------------------------------------------------------------------
# checks to see if a buffer with the name given has been opened
# and not yet closed.
# returns -1 if not currently open or else the buffer number
# (0..$ascScripVect(maxbufnum)) of the corresponding buffer.
#------------------------------------------------------------------------
proc Script_File_Loaded  {filename} {
  global ascScripVect
  for {set c 0} {$c <= $ascScripVect(maxbufnum)} {incr c} {
    if {$ascScripVect(bufopen.$c) && \
        "$ascScripVect(buffile.$c)" == "$filename"} {
      return $c
    }
  }
  return -1
}

#
# proc Script_Read_File  {filename}
#------------------------------------------------------------------------
# read a file without parsing first.
# appends it to the current text box
#------------------------------------------------------------------------
proc Script_Read_File {filename} {
  global ascScripVect
  FileInText $ascScripVect(scripBox) $filename
}

# proc Script_Selection{}
#------------------------------------------------------------------------
# Returns the selection in the currently visible Script Text window.
# If no text is selected, returns an empty string.
#------------------------------------------------------------------------
proc Script_Selection {} {
  global ascScripVect
  if {[catch "$ascScripVect(scripBox) get sel.first sel.last" sel] == 0} {
    return $sel;
  }
  return "";
}



#
# proc Script_File_Get {filename}
#------------------------------------------------------------------------
# Read in a script file, deal with the buffers and all that.
#------------------------------------------------------------------------
proc Script_File_Get {filename} {
  global ascScripVect
  set filename [file nativename $filename]
  if {[file isfile $filename]} {
    set ascScripVect(filename) $filename;
  } else {
    Script_Raise_Alert "File Not Found"
    return 1;
  }
 # update menus and switch text box widgets
  set num $ascScripVect(maxbufnum)
  incr num
  set ascScripVect(bufopen.$num) 0
  ScriptSwitchToNewBuf $num $filename
 # load the text
  Script_Read_File $filename
}

#
#------------------------------------------------------------------------
# proc check_time. null proc
#------------------------------------------------------------------------
proc check_time {} {
}

#
# proc Script_do_Font {}
#---------------------------------------------------------------------
# font select button for script window
#---------------------------------------------------------------------
proc Script_do_Font {args} {
  global ascScripVect
  set font ""
  if {$args != ""} {
    set font $args
  } else {
    set font [ascFontGet]
  }
  if {"$font" == ""} {
    return;
  }
  $ascScripVect(scripBox) configure -font $font
  set ascScripVect(font) [lindex [$ascScripVect(scripBox) configure -font] 4]
}


proc Script_do_NewFile {} {
  global ascScripVect
  set num $ascScripVect(maxbufnum)
  incr num
  ScriptSwitchToNewBuf $num {}
}

proc ascwhoami {} {
  global env tcl_platform
  if {[info exists env(USERNAME)]} {
    return $env(USERNAME)
  }
  if {[info exists env(USER)]} {
    return $env(USER)
  }
  if {[info exists env(User)]} {
    return $env(User)
  }
  if {[info exists env(user)]} {
    return $env(user)
  }
  return "anonymous[pid]"
}
#
# proc Script_do_Import_File {}
#------------------------------------------------------------------------
# displays a filefind box and adds the file specified to the end of
# the current script. would be nice if inserted at current point.
#------------------------------------------------------------------------
proc Script_do_Import_File {} {
  global ascScripVect asc_tkfbox ascToolVect
  set defaultname "$ascToolVect(dirinput)"
  set filename [tk_getOpenFile \
    -defaultextension "" \
    -filetypes $ascScripVect(filetypes) \
    -initialdir $defaultname \
    -parent .script \
    -title {Import script lines}]

  set filename [file nativename $filename]
  if {$filename == "" || [file isdirectory $filename]} {
    if {!$asc_tkfbox(cancelled)} {
      set msg "\""
      append msg $filename "\" cannot be read."
      asctk_dialog .fileerr $ascScripVect(font) FYI $msg "" 0 OK
    }
    return 1;
  } else {
    if {[file isfile $filename] == 0} {
      Script_Raise_Alert "File Not Found"
      return 1;
    }
    set newext "[file extension $filename]"
    if {$newext != ""} {
      set ascScripVect(lastimportextension) $newext
      ascresort_filetypes ascScripVect lastimportextension
    }
    Script_Read_File $filename
    if {$ascScripVect(visibility)} {newraise .script}
    update idletasks
  }
}

#
# proc Script_do_ReadFile
#------------------------------------------------------------------------
# get a file from user and read it in
#------------------------------------------------------------------------
proc Script_do_ReadFile {} {
  global ascScripVect asc_tkfbox ascGlobalVect ascToolVect
  set defaultname "$ascToolVect(dirinput)"
  set asc_tkfbox(otherdirs) $ascGlobalVect(librarypathdirs)
  set filename [tk_getOpenFile \
    -defaultextension "" \
    -filetypes $ascScripVect(filetypes) \
    -initialdir $defaultname \
    -parent .script \
    -title {Read Ascend IV interface script}]

  if {$filename == "" || [file isdirectory $filename]} {
    if {!$asc_tkfbox(cancelled)} {
      set msg "\""
      append msg $filename "\" cannot be read."
      asctk_dialog .fileerr $ascScripVect(font) FYI $msg "" 0 OK
    }
    return 1;
  } else {
    set newext ""
    set newext [file extension $filename]
    if {$newext != ""} {
      set ascScripVect(lastreadextension) $newext
      ascresort_filetypes ascScripVect lastreadextension
      lappend ascGlobalVect(librarypathdirs) [file dirname $filename]
    }
    Script_File_Get $filename
    if {$ascScripVect(visibility)} {newraise .script}
    update idletasks
  }
}

#
# proc Script_File_OpenandWrite {filename}
#------------------------------------------------------------------------
# internal to Script_do_WriteFile
#------------------------------------------------------------------------
proc Script_File_OpenandWrite {filename} {
  global ascScripVect
  set w $ascScripVect(scripBox);
  set nok [catch {set fdesc [open $filename w]}];
  if {$nok} {
    Script_Raise_Alert $fdesc "File Writing Error"
    return 1;
  }
  set data [Script_Selection]
  if {"$data" == ""} {
    Script_Raise_Alert "Nothing to Write" "Error"
    return 1;
  }
  puts $fdesc $data;
  close $fdesc;
  return 0;
}
#
# proc Script_do_WriteFile {}
#------------------------------------------------------------------------
# save selection in script to a file selected via file box
#------------------------------------------------------------------------
proc Script_do_WriteFile {} {
  global ascScripVect
  set defaultname $ascScripVect(filename)
  set filename [tk_getSaveFile \
    -defaultextension "" \
    -filetypes $ascScripVect(filetypes) \
    -initialfile $defaultname \
    -parent .script \
    -title {Save Ascend IV interface script}]

  if {$filename == ""} {
    return 1;
  } else {
    Script_File_OpenandWrite $filename;
  }
}

#
# proc Script_do_WriteBuf {}
#------------------------------------------------------------------------
# save selection in script to a file selected
#------------------------------------------------------------------------
proc Script_do_WriteBuf {} {
  global ascScripVect
  set defaultname $ascScripVect(filename)
  set w $ascScripVect(scripBox)
  $ascScripVect(scripBox) tag add sel 1.0 [$ascScripVect(scripBox) index end]
  set data [Script_Selection]
  if {![catch {set fdesc [open $defaultname w]}]} {
    puts $fdesc $data
    close $fdesc
  } else {
	puts "ERROR: failed to save file $defaultname"
  }
  $ascScripVect(scripBox) tag remove \
    sel 1.0 [$ascScripVect(scripBox) index end]
}

#
# proc Script_do_WriteBufAs {}
#------------------------------------------------------------------------
# save selection in script to a file selected
#------------------------------------------------------------------------
proc Script_do_WriteBufAs {} {
  global ascScripVect
  set defaultname $ascScripVect(filename)
  set filename [tk_getSaveFile \
    -defaultextension "" \
    -filetypes $ascScripVect(filetypes) \
    -initialfile $defaultname \
    -parent .script \
    -title {Save interface script AS}]

  if {$filename == ""} {
    return 1;
  } else {
    $ascScripVect(scripBox) tag add \
      sel 1.0 [$ascScripVect(scripBox) index end]
    Script_File_OpenandWrite $filename;
    $ascScripVect(scripBox) tag remove sel \
      1.0 [$ascScripVect(scripBox) index end]
  }
  Script_File_Get $filename
}

#
# proc Script_do_SelectAll {}
#------------------------------------------------------------------------
# highlights all of the script.
# assumes text, which is quite sensible
#------------------------------------------------------------------------
proc Script_do_SelectAll {} {
  global ascScripVect
  $ascScripVect(scripBox) tag add sel 1.0 [$ascScripVect(scripBox) index end]
  update idletasks
  update
}

#
# proc Script_find_Semi {}
#------------------------------------------------------------------------
# Finds char before next semicolon, starting at the beginning of current
# selection. Ignores the very first character, to avoid being
# stuck if you start at a semicolon.
# Returns end of text if semicolon never found.
# Isn't clever about disjoint selections:
# could be if we stopped at selend instead of textend.
# This sucker is shockingly fast.
#------------------------------------------------------------------------
proc Script_find_Semi {ScriptBox} {
  set psel ""
  set psel [$ScriptBox tag ranges sel]
  if {$psel==""} {error "no selection"}
  set p0 [lindex [split $psel] 0]
  set pend [$ScriptBox index end]
  for {set offset 1} \
      {![catch {set pc [$ScriptBox index "$p0 + $offset chars"]}] &&
       [$ScriptBox compare $pc < $pend] && \
	   "[$ScriptBox get $pc]" != "\;"} \
      {incr offset} {}
  return $pc
}

#
# proc Script_unsel {}
#------------------------------------------------------------------------
# unselect up through next semicolon, starting at beginning of cur selection
#------------------------------------------------------------------------
proc Script_unsel {ScriptBox} {
  set slist [$ScriptBox tag ranges sel]
  if {$slist==""} {return}
  set oselbeg [lindex $slist 0]
  set oselend [$ScriptBox index "[Script_find_Semi $ScriptBox] + 1 chars"]

  # Adjust the position of the insertion cursor, so that it is
  # set up at the start of the line just completed.

  $ScriptBox mark set insert "$oselend linestart"

  # remove the selection
  #
  $ScriptBox tag remove sel $oselbeg $oselend
}

#
# proc Script_do_RemoveStats -Version 2 - TextBox Version
#------------------------------------------------------------------------
# delete disjoint selection. baa
#------------------------------------------------------------------------
proc Script_do_RemoveStats {} {
  global ascScripVect
  set w $ascScripVect(scripBox)
  set tlist "[$ascScripVect(scripBox) tag ranges sel]"
  if {$tlist==""} {return}
  set nr [expr [llength $tlist] /2]
  for {set r [expr $nr -1]} {$r>=0} {incr r -1} {
    set start [lindex $tlist [expr 2*$r]]
    set stop [lindex $tlist [expr 2*$r+1]]
    $w delete $start $stop
  }
}

proc ScriptFile_do_Copy {} {
  global ascScripVect
  $ascScripVect(fileentry) configure -state normal
  asc_export_selection $ascScripVect(fileentry)
  event generate $ascScripVect(fileentry) <<Copy>>
  $ascScripVect(fileentry) configure -state disabled
}

proc Script_do_Copy {} {
  global ascScripVect
  asc_export_selection $ascScripVect(scripBox)
  event generate $ascScripVect(scripBox) <<Copy>>
}

proc Script_do_Cut {} {
  global ascScripVect
  event generate $ascScripVect(scripBox) <<Cut>>
}

proc Script_do_Paste {} {
  global ascScripVect
  event generate $ascScripVect(scripBox) <<Paste>>
}

#
# proc Script_do_Record {n1 n2 mode}
#------------------------------------------------------------------------
# not needed.
# toggle recorder system. call will be from trace if at all
#------------------------------------------------------------------------
proc Script_do_Record {n1 n2 mode} {
  global ascScripVect
  update
  Script_ClearEvents
  update idletasks
  if {$ascScripVect(Record)} {
  }
}

# returns normal if something is selected and disabled if not.
proc ScriptSelectState {} {
  global ascScripVect
  if {"[$ascScripVect(scripBox) tag ranges sel]" !=""} {
    return normal
  }
  return disabled
}

# always returns normal. updates the menu entry.
proc ScriptSaveState {} {
  global ascScripVect ascPopInfo
  set lbl "Save $ascScripVect(filename)"
  $ascPopInfo(menu) entryconfigure $ascPopInfo(index) -label $lbl
  return normal
}
#
# proc Script_Update_File_Buttons {}
#------------------------------------------------------------------------
# does what it says
#------------------------------------------------------------------------
proc Script_Update_File_Buttons {} {
  global ascScripVect
  set mb "$ascScripVect(fileBtn)"
  $mb entryconfigure 0 -state normal
  $mb entryconfigure 1 -state normal
  $mb entryconfigure 2 -state normal
  $mb entryconfigure 3 -state normal
 # note 4: separators don't have states
  $mb entryconfigure 5 -state normal
 # note 6: separators don't have states
  $mb entryconfigure 7 -state normal
  $mb entryconfigure 8 -state normal
 # note 9: separators don't have states
}

#
# proc Script_Update_EditButtons {}
#------------------------------------------------------------------------
# does what it says
#------------------------------------------------------------------------
proc Script_Update_Edit_Buttons {} {
  global ascScripVect
  set mb "$ascScripVect(editBtn)"
  switch [ScriptSelectState] {
  normal {
      $mb entryconfigure 3 -state normal
  } 
  default {
      $mb entryconfigure 3 -state disabled
    }
  }
}

#
# proc Script_Update_View_Buttons {}
#------------------------------------------------------------------------
# enable/disable options in the view menu
#------------------------------------------------------------------------
proc Script_Update_View_Buttons {} {
  global ascScripVect ascGlobalVect

  set mb .script.menubar.view

  if {$ascGlobalVect(saveoptions) == 0} {
    $mb entryconfigure 1 -state disabled
    $mb entryconfigure 2 -state disabled
  } else {
    $mb entryconfigure 1 -state normal
    $mb entryconfigure 2 -state normal
  }

}

#
# proc Script_Update_ExecButtons {}
#------------------------------------------------------------------------
# does what it says
#------------------------------------------------------------------------
proc Script_Update_Exec_Buttons {} {
  global ascScripVect
  set mb $ascScripVect(execBtn)
  switch [ScriptSelectState] {
  normal {
      $mb entryconfigure 0 -state normal
      $mb entryconfigure 1 -state normal
    }
  default {
      $mb entryconfigure 0 -state disabled
      $mb entryconfigure 1 -state disabled
    }
  }
}

proc Script_SetCursor_Normal {ScriptBox} {
  $ScriptBox config -cursor xterm
}

proc Script_SetCursor_Executing {ScriptBox} {
  $ScriptBox config -cursor watch
}

proc Script_ExitGeom {} {
  return [setpos .script 40 40]
}

#
# Script_do_Exit
#------------------------------------------------------------------------
# exit ascend button
#------------------------------------------------------------------------
proc Script_do_Exit {} {
  global ascScripVect
  set ascScripVect(menubreak) 1
  set position [Script_ExitGeom]
  set res [VShowWindow.ascConfirm "190x50$position" "Exit"]
  if {$res == 1} {
    Script_ClearInterrupt
    EXIT NOCONFIRM
  }
}

#
#proc do_ScriptExecuteBox {}
#------------------------------------------------------------------------
# Creates a widget to ask whether the execution of the statements
# in the script is going to be continuous or step by step
#------------------------------------------------------------------------
#
proc do_ScriptExecuteBox {} {
entertrace
  global ascScripVect ascScriptExecute

  set tl .scriptsteptrough
  # build widget
  toplevel $tl

  # Window manager configurations
  #global tk_version

  wm positionfrom $tl user
  wm sizefrom $tl user
  wm minsize $tl 250 60
  wm geometry $tl 250x60[setpos .display 90 190]
  wm title $tl ""


  # build widget $tl.buttons_frm
  frame $tl.buttons_frm \
    -borderwidth 0

  # build widget $tl.buttons_frm.next_button
  button $tl.buttons_frm.next_button \
    -font $ascScripVect(font) \
    -text Next \
    -width 7 \
    -command "
	global ascScriptExecute
	set ascScriptExecute(button) 1
  	destroy $tl"

  # build widget $tl.buttons_frm.btn2
    button $tl.buttons_frm.btn2 \
      -borderwidth 2 \
      -font $ascScripVect(font) \
      -text Go \
      -width 4 \
      -command "
         global ascScriptExecute
         set ascScriptExecute(button) 2
         destroy $tl"

  # build widget $tl.buttons_frm.btn3
    button $tl.buttons_frm.btn3 \
      -borderwidth 3 \
      -font $ascScripVect(font) \
      -text Stop \
      -width 7 \
      -command "
         global ascScriptExecute
         set ascScriptExecute(button) 3
         destroy $tl"
 
  # pack widget $tl.buttons_frm
  pack append $tl.buttons_frm \
    $tl.buttons_frm.next_button {left frame center expand fill} \
    $tl.buttons_frm.btn2 {left frame center expand fill} \
    $tl.buttons_frm.btn3 {left frame center expand fill}

  # build widget $tl.lbl_frm
  frame $tl.lbl_frm

  # build widget $tl.lbl_frm.main_label
  label $tl.lbl_frm.main_label \
    -text "Script Executing Statements"

  # pack widget $tl.lbl_frm
  pack append $tl.lbl_frm \
    $tl.lbl_frm.main_label {top frame center pady 5 fillx}

  # pack widget $tl
  pack append $tl \
    $tl.lbl_frm {top frame center pady 5 fillx} \
    $tl.buttons_frm {top frame center fill}

  bind $tl <Visibility> "ascKeepOnTop $tl"
  proc DestroyWindow$tl {} "
    destroy $tl
    update"

  # wait for the box to be destroyed
  tkwait window $tl
  return $ascScriptExecute(button)
leavetrace
}



#
# Script_do_ExecuteStats {contmode}
#------------------------------------------------------------------------
# if contmode not given, it is assumed 1.
# menubutton bindings are not supposed to require arguments.
# Steps through tcl code in delimited chunks
# Loops with intermediate ; 
#
# If contmode = 0:
#  A)It will put each of the statements and previous
#    comments in the Display window
#  B)It stops after the execution of each statement to 
#    ask if you
#    a)want to execute the next statement
#    b)want to stop
#    c)want to execute the rest of the statements without interruption.
#
# it will bomb Eval
# Modified to use script_eval a registered call rather than tcls' eval
# so as to evaluate things in the global sphere.
#------------------------------------------------------------------------
proc Script_do_ExecuteStats {{contmode 1}} {
  global ascScripVect ascSolvStatVect ascDispVect
# need to store scriptbox incase a command in the script
# changes to a new script buffer
  set locScriptBox $ascScripVect(scripBox)
  set com_list [Script_Selection]
  set statlist [split $com_list ";"]
  set ascScripVect(executing) 1
  Script_ClearInterrupt
  set continuous_mode $contmode
  set len [llength $statlist]
  set counter 1
  DispClear;
  DispSetEntry "Script statement just executed"
  Script_SetCursor_Executing $locScriptBox
  foreach stat $statlist {
    set counter [expr $counter + 1]
    if {$ascScripVect(menubreak) != 0} {
      puts stderr "Script interrupted"
      set ascSolvStatVect(menubreak) 0
      break
    }
    if {$continuous_mode != 1} {
      DispInsert3 $stat
      if {$ascDispVect(visibility)} {newraise .display}
    }
    if {[catch {script_eval $stat} jnk]} {#script_eval is a registered call
      set jnk [string trim $jnk]
      puts stderr "$jnk\n"
      puts "in script code: >>$stat<<"
      if {$continuous_mode != 1} {
        DispInsert3 "\n"
        DispInsert3 "$jnk\n"
        DispInsert3 "in script code: >>$stat<<"
        if {$ascDispVect(visibility)} {newraise .display}
      }
      # this should be done by individual commandslike
      # SOLVE rather than here
      if {[string range $jnk 0 4]=="Float"} {
        set ascScripVect(executing) 0
        error $jnk}
      Script_SetCursor_Normal $locScriptBox
      break
    }
    Script_unsel $locScriptBox
    update idletasks
    update
    if {$continuous_mode != 1} {
      if {$counter < $len} {
        set execmode [do_ScriptExecuteBox]
	DispClear;
        if {$execmode == 2} {
          set continuous_mode 1 
          wm iconify .display       
        }
        if {$execmode == 3} { 
          wm iconify .display
          set ascScripVect(executing) 0
          set ascSolvStatVect(menubreak) 0
          Script_SetCursor_Normal $locScriptBox
          break     
        }  
      }
    }
  }
  set ascScripVect(executing) 0
  Script_SetCursor_Normal $locScriptBox
  return
}

#
# proc Script_do_Help {}
# proc Script_do_BindHelp {}
#------------------------------------------------------------------------
# Help button calls
#------------------------------------------------------------------------
proc Script_do_Help {} {
  Help_button script
}
proc Script_do_BindHelp {} {
  Help_button {script.help onascend/tclscripts}
}
proc Script_getting_started {} {
  Help_button {howto-ascend} on modeling
}

#------------------------------------------------------------------------
# RECORDing system calls.
# all of these look at the global variable ascScripVect(executing) to
# see if they should record or not. Any user events that happen while a
# script is running will be ignored, in all likelihood.
# The script event counter ascScripVect(count) will be incremented. This
# counter is used to insure that statements get inserted in the proper
# order. (maybe)
#------------------------------------------------------------------------
# events recorded:
#   ASSIGN
#   BROWSE
#   READ FILE
#   READ VALUES
#   COMPILE
#   MERGE
#   REFINE
#   DELETE
#   PLOT
#   DISPLAY
#   PROBE
#   PRINT
#   RUN
#   SOLVE
#   WRITE
# events ignored, for whatever reason
#   RESTORE
#   SAVE
#   INTEGRATE
#   OBJECTIVE

#
# proc Script_AppendEvent  {line counter}
#------------------------------------------------------------------------
# insert line into the script window at end
#------------------------------------------------------------------------
proc Script_AppendEvent  {line counter} {
  global ascScripVect
  $ascScripVect(scripBox) insert end $line
}
#
# proc Script_Record_Solve {inst snum args}
#------------------------------------------------------------------------
# record solving with solver snum if not t already.
#------------------------------------------------------------------------
proc Script_Record_Solve {args} {
  global ascScripVect
  if {!$ascScripVect(executing) && $ascScripVect(Record)} {
    set inst [lindex $args 0]
    set sname [lindex $args 1]

    set pname [slv_get_pathname]
    set objnum [slv_get_obj_num 2]
    if {$objnum >= 0} {
      set objname "$pname.[stripbraces [dbg_write_obj 2 $objnum 0]]"
      set line "\nOPTIMIZE \{$objname\} IN \{$inst\} WITH $sname;"
    } else {
      set line "\nSOLVE \{$inst\} WITH $sname;"
    }
    set c [incr ascScripVect(count)]
    Script_AppendEvent $line $c
  }
}
#
# proc Script_Record_Flush {args}
#------------------------------------------------------------------------
# record flushing the solver. this needs to be smarter if we have
# multiple problems in the solver simultaneously.
#------------------------------------------------------------------------
proc Script_Record_Flush {args} {
  global ascScripVect
  if {!$ascScripVect(executing) && $ascScripVect(Record)} {
    set inst [lindex $args 0]
    set sname [lindex $args 1]
    set line "\nDELETE SYSTEM;"
    set c [incr ascScripVect(count)]
    Script_AppendEvent $line $c
  }
}

#
# proc Script_Record_Read {file args}
#------------------------------------------------------------------------
# record file read in.
# change backslashes \ to forward slashes / and put the name in
# double quotes to protect spaces in the file name.
#------------------------------------------------------------------------
proc Script_Record_Read {file args} {
  global ascScripVect
  if {!$ascScripVect(executing) && $ascScripVect(Record)} {
    regsub -all {\\} $file / properFile
    set line "\nREAD FILE \"$properFile\";"
    set c [incr ascScripVect(count)]
    Script_AppendEvent $line $c
  }
}

#
# proc Script_Record_ValueRead {file args}
#------------------------------------------------------------------------
# record values file read in.
#------------------------------------------------------------------------
proc Script_Record_ValueRead {file args} {
  global ascScripVect
  if {!$ascScripVect(executing) && $ascScripVect(Record)} {
    regsub -all {\\} $file / properFile
    set line "\nREAD VALUES \"$properFile\";"
    set c [incr ascScripVect(count)]
    Script_AppendEvent $line $c
  }
}

#
# proc Script_Record_ValueWrite {args}
#------------------------------------------------------------------------
# record values file written.
#------------------------------------------------------------------------
proc Script_Record_ValueWrite {args} {
  global ascScripVect
  if {!$ascScripVect(executing) && $ascScripVect(Record)} {
    set inst [lindex $args 0]
    set filename [lindex $args 1]
    set line "\nWRITE VALUES \{$inst\} $filename;"
    set c [incr ascScripVect(count)]
    Script_AppendEvent $line $c
  }
}


#
# proc Script_Record_Compile {args}
#------------------------------------------------------------------------
# record instantiation
#------------------------------------------------------------------------
proc Script_Record_Compile {args} {
  global ascScripVect
  if {!$ascScripVect(executing) && $ascScripVect(Record)} {
    set sim [lindex $args 0]
    set type [lindex $args 1]
    set line "\nCOMPILE $sim OF $type;"
    set c [incr ascScripVect(count)]
    Script_AppendEvent $line $c
  }
}

#
# proc Script_Record_Refine {args}
#------------------------------------------------------------------------
# record interactive refinement
#------------------------------------------------------------------------
proc Script_Record_Refine {args} {
  global ascScripVect
  if {!$ascScripVect(executing) && $ascScripVect(Record)} {
    set inst [lindex $args 0]
    set type [lindex $args 1]
    set line "\nREFINE \{$inst\} TO $type;"
    set c [incr ascScripVect(count)]
    Script_AppendEvent $line $c
  }
}

#
# proc Script_Record_Resume {args}
#------------------------------------------------------------------------
# record interactive resume compile
#------------------------------------------------------------------------
proc Script_Record_Resume {args} {
  global ascScripVect
  set inst "a"
  set line "a"
  if {!$ascScripVect(executing) && $ascScripVect(Record)} {
    if {[llength $args] >0} {
      set inst [lindex $args 0]
      set line "\nRESUME \{$inst\};"
    } else { set line "RESUME;"}
    set c [incr ascScripVect(count)]
    Script_AppendEvent $line $c
  }
}
#
# proc Script_Record_Merge {args}
#------------------------------------------------------------------------
# record interactive merge
#------------------------------------------------------------------------
proc Script_Record_Merge {args} {
  global ascScripVect
  if {!$ascScripVect(executing) && $ascScripVect(Record)} {
    set inst1 [lindex $args 0]
    set inst2 [lindex $args 1]
    set line "\nMERGE \{$inst1\} WITH \{$inst2\};"
    set c [incr ascScripVect(count)]
    Script_AppendEvent $line $c
  }
}

#
# proc Script_Record_DeleteTypes {args}
#------------------------------------------------------------------------
# record type deletion
#------------------------------------------------------------------------
proc Script_Record_DeleteTypes {args} {
  global ascScripVect
  if {!$ascScripVect(executing) && $ascScripVect(Record)} {
    set line "\nDELETE TYPES;"
    set c [incr ascScripVect(count)]
    Script_AppendEvent $line $c
  }
}

#
# proc Script_Record_Delete {sim args}
#------------------------------------------------------------------------
# record sim deletion
#------------------------------------------------------------------------
proc Script_Record_Delete {sim args} {
  global ascScripVect
  if {!$ascScripVect(executing) && $ascScripVect(Record)} {
    set line "\nDELETE $sim;"
    set c [incr ascScripVect(count)]
    Script_AppendEvent $line $c
  }
}

#
# proc Script_Record_Browse {inst args}
#------------------------------------------------------------------------
# record export for browsing of an instance
#------------------------------------------------------------------------
proc Script_Record_Browse {inst args} {
  global ascScripVect
  if {!$ascScripVect(executing) && $ascScripVect(Record)} {
    set line "\nBROWSE \{$inst\};"
    set c [incr ascScripVect(count)]
    Script_AppendEvent $line $c
  }
}

#
# proc Script_Record_Assign {args}
#------------------------------------------------------------------------
# record assignment
#------------------------------------------------------------------------
proc Script_Record_Assign {args} {
  global ascScripVect
  if {!$ascScripVect(executing) && $ascScripVect(Record)} {
    set inst [lindex $args 0]
    set value [lindex $args 1]
    if {[catch {set units [lindex $args 2]} ]} {set units ""}
    set line "\nASSIGN \{$inst\} $value \{$units\};"
    set c [incr ascScripVect(count)]
    Script_AppendEvent $line $c
  }
}

#
# proc Script_Record_Plot {args}
#------------------------------------------------------------------------
# record plot
#------------------------------------------------------------------------
proc Script_Record_Plot {args} {
  global ascScripVect
  if {!$ascScripVect(executing) && $ascScripVect(Record)} {
    set inst [lindex $args 0]
    set filename [lindex $args 1]
    set leafname [file tail $filename]
    set scrpref "asc[ascwhoami]"
    if {[string range $leafname 0 [string length $scrpref]]=="$scrpref."} {
      set filename ""
    }
    set line "\nPLOT \{$inst\} $filename;"
    set c [incr ascScripVect(count)]
    Script_AppendEvent $line $c
    set line "\nSHOW LAST;"
    set c [incr ascScripVect(count)]
    Script_AppendEvent $line $c
  }
}

#
# proc Script_Record_Run {instproc args}
#------------------------------------------------------------------------
# record initialization routine
#------------------------------------------------------------------------
proc Script_Record_Run {instproc args} {
  global ascScripVect
  if {!$ascScripVect(executing) && $ascScripVect(Record)} {
    set line "\nRUN \{$instproc\};"
    set c [incr ascScripVect(count)]
    Script_AppendEvent $line $c
  }
}

#
# proc Script_Record_Probe {args}
#------------------------------------------------------------------------
# record export to probe
#------------------------------------------------------------------------
proc Script_Record_Probe {args} {
  global ascScripVect
  set a1 [lindex $args 0]
  set a2 [lindex $args 1]
  set a3 [lindex $args 2]
  if {!$ascScripVect(executing) && $ascScripVect(Record)} {
    set line "\nPROBE "
    append line $a1
    append line " " \{ $a2 \}
    append line " " "\{[stripbraces $a3]\}\;"
    set c [incr ascScripVect(count)]
    Script_AppendEvent $line $c
  }
}

#
# proc Script_Record_Display {item args}
#------------------------------------------------------------------------
# record export to display
#------------------------------------------------------------------------
proc Script_Record_Display {item args} {
  global ascScripVect
  if {!$ascScripVect(executing) && $ascScripVect(Record)} {
    puts $item; puts $args
  }
}

#
# proc Script_Record_Print {item args}
#------------------------------------------------------------------------
# record printing probe or display
#------------------------------------------------------------------------
proc Script_Record_Print {item args} {
  global ascScripVect
  if {!$ascScripVect(executing) && $ascScripVect(Record)} {
    set line "\nPRINT $item"
    set c [incr ascScripVect(count)]
    Script_AppendEvent $line $c
  }
}
#
# proc Script_Record_ClearVars {inst args}
#------------------------------------------------------------------------
# record solving with solver snum if not t already.
#------------------------------------------------------------------------
proc Script_Record_ClearVars {args} {
  global ascScripVect
  if {!$ascScripVect(executing) && $ascScripVect(Record)} {
    set inst [lindex $args 0]
    set line "\nCLEAR_VARS \{$inst\};"
    set c [incr ascScripVect(count)]
    Script_AppendEvent $line $c
  }
}

#####################################################################
# XF theft
#------------------------------------------------------------------------
# Procedure: FileInText
# Description: fill a text with the contents of the file
# Arguments: textWidget - the widget
#            {fileName} - filename to read
# Returns: none
# Sideeffects: the text widget is filled
#------------------------------------------------------------------------
proc FileInText {textWidget {fileName ""}} {# xf ignore me 5

  # check file existance
  if {"$fileName" == ""} {
    puts stderr "no filename specified"
    return
  }
  set fileName [file nativename $fileName]
  if {[catch {set fileInFile [open $fileName r]}]} {
    asctk_dialog .fileerr $ascScripVect(font) \
                 Load-Error $fileInFile "" 0 OK
	return
  }

  set textValue [read $fileInFile]
  $textWidget insert end "$textValue"
  close $fileInFile
}

# eof

#####################################################################
# some text widget utils
#
# proc taglines {w}
#------------------------------------------------------------------------
# appears to tag first 80 char of lines or some such....
#------------------------------------------------------------------------
proc taglines {w} {
  set end [$w index end]
  set endl [split $end "."]
  set endl [lindex $endl 0]
  for {set c 1} {$c <= $endl} {incr c} {
    set start "$c\.0"
    set stop "$c\.80"
    $w tag add "line$c" "$start" "$stop"
  }
}

#
# proc tagdelete {w}
#------------------------------------------------------------------------
# scrap all tags but sel
#------------------------------------------------------------------------
proc tagdelete {w} {
 set nm [$w tag names]
 foreach tag $nm {
   if {$tag != "sel"} {
     $w tag del $tag
   }
 }
}
