#  SolverProc.tcl: Solver Tcl Code
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.80 $
#  Last modified on: $Date: 1998/06/18 15:55:00 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: SolverProc.tcl,v $
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

# File structure:
# default set procedures
# menu button direct callbacks
# menu button internals
# utility routines
# routines that should be in other files

global SolverNames
set SolverNames [list Slv MINOS Opt QRSlv CSlv makeMPS NGSlv CMSlv LRSlv]


#
#
# proc set_Solver_Defaults {}
#------------------------------------------------------------------------
# set vars/bindings on solver
#------------------------------------------------------------------------
proc set_Solver_Defaults {} {
entertrace
# puts "setting solver buttons"
# solver defaults vectors
# G(32767): general
# 0:slv, 1:MINOS, 2:opt, 3:QRSlv beta, 4: cslv
# 5:LSSLv beta, 6:makeMPS, 7:NGSlv, 8:CONOPT, 9: CMSlv, 9a: LRSlv
  global ascSolvVect
  global ascSolv32767Vect
  global ascSolv0Vect
  global ascSolv1Vect
  global ascSolv2Vect
  global ascSolv3Vect
  global ascSolv4Vect
  global ascSolv5Vect
  global ascSolv6Vect
  global ascSolv7Vect
  global ascSolv8Vect
  global ascSolv9Vect
  global ascSolv9aVect
  global ascSolvStatVect

  Solve_set_SolverRegisteredNumbers

  if {[catch {set ascSolvStatVect(empty)} ]} {
    # not a restart. set things
    # solver window miscellany: buckets of this should come from option get
    set ascSolvVect(modeltype) ""
    # set in globaldefs
    #   set ascSolvVect(modelbar) 1
    set ascSolvVect(pathname) ""
    set ascSolvVect(statVisible) 1
    set ascSolvVect(instname) ""
    set ascSolvVect(objvarname) ""
    set ascSolvVect(simname) ""
    set ascSolvVect(windowname) ".solver"
    set ascSolvVect(debuggerup) 0
    set ascSolvVect(mtxup) 0
    # status box information and related detail
    set ascSolvStatVect(statreport) 1
    set ascSolvStatVect(empty) 1
    set ascSolvStatVect(eventqueue) ""
    set ascSolvStatVect(running) 0
    set ascSolvStatVect(objval) none
    set ascSolvStatVect(solvernum) 0
    set ascSolvStatVect(ok) 1
    set ascSolvStatVect(menubreak) 0
    set ascSolvStatVect(overdefined) 1
    set ascSolvStatVect(underdefined) 1
    set ascSolvStatVect(structsingular) 1
    set ascSolvStatVect(fpcaught) 0
    set ascSolvStatVect(ready2solve) 1
    set ascSolvStatVect(converged) 1
    set ascSolvStatVect(diverged) 1
    set ascSolvStatVect(inconsistent) 1
    set ascSolvStatVect(calcok) 1
    set ascSolvStatVect(itnlim_exceeded) 1
    set ascSolvStatVect(timlim_exceeded) 1
    set ascSolvStatVect(iteration) 1
    set ascSolvStatVect(cpuelapsed) 1
    set ascSolvStatVect(iterations) "0/0"

    set ascSolvStatVect(block.number) ""
    set ascSolvStatVect(block.current) ""
    set ascSolvStatVect(block.size) ""
    set ascSolvStatVect(block.prevtotsize) ""
    set ascSolvStatVect(block.iteration) ""
    set ascSolvStatVect(block.cpuelapsed) ""
    set ascSolvStatVect(block.residual) ""

    set ascSolvStatVect(totalrels) ""
    set ascSolvStatVect(rels) ""
    set ascSolvStatVect(in_rels) ""
    set ascSolvStatVect(inc_rels) ""
    set ascSolvStatVect(eqals) ""
    set ascSolvStatVect(inc_eqals) ""
    set ascSolvStatVect(ineqals) ""
    set ascSolvStatVect(inc_ineqals) ""
    set ascSolvStatVect(in_inc_eqals) ""
    set ascSolvStatVect(in_inc_ineqals) ""
    set ascSolvStatVect(uninc_rels) ""
    set ascSolvStatVect(totalvars) ""
    set ascSolvStatVect(in_vars) ""
    set ascSolvStatVect(vars) ""
    set ascSolvStatVect(inc_vars) ""
    set ascSolvStatVect(un_vars) ""
    set ascSolvStatVect(free_vars) ""
    set ascSolvStatVect(fixed_vars) ""
    set ascSolvStatVect(in_free_vars) ""
    set ascSolvStatVect(in_fixed_vars) ""
    set ascSolvStatVect(solver) "QRSlv"
    set ascSolvStatVect(solved_vars) ""
    set ascSolvStatVect(state) ""
    set ascSolvStatVect(start_block) ""
    set ascSolvStatVect(stop_block) ""

    set_SolvG_Defaults
    #    set_Solv0_Defaults
    #    set_Solv1_Defaults
    #    set_Solv6_Defaults
    for {set s 0} {$s<$ascSolvVect(numberofsolvers)} {incr s} {
        set_defaults $s
    }

    set fpe [string tolower $ascSolvVect(trapFPEdefault)]
    if {$fpe=="no" || $fpe=="0" || $fpe=="false"} {
      slv_trapfp
      slv_trapint
      #side effect sets the interrupt trap. need to fix that.
      slv_untrapfp
    } {
      slv_trapfp
    }
  }

  set ascSolv32767Vect(monitor) 0 ;# set this to 1 in .ascend.ad
  Solve_Build_EngineMenus
  Solve_Build_SaveParsMenus
  Solve_Update_MenuBar
  bind .solver <Control-Key-C> Solve_do_Interrupt
  bind .solver <F5> Solve_do_Solve
  bind .solver <F6> Solve_do_Iterate
  bind .solver <F7> Solve_do_Integrate
  bind .solver <Control-Key-p> Solve_do_Display_Status
  bind .solver <Control-Key-g> {Solve_do_Parms open General}
  bind .solver.lbl_frm.lbl_run <B3-ButtonRelease> {Help_button solver.running}

leavetrace
}


#
#proc set_defaults {number}
#------------------------------------------------------------------
# automatically builds procedures for setting up solvers parameter
# pages. Calls procedure after creating also.
# Desired improvements:
#         automaticaly detect and set maxlines.
#         error detection in c code and appropriate bailout in tcl
#------------------------------------------------------------------
proc set_defaults {number} {
entertrace
    global ascSolvVect
    # nest call actually gets defaults
    set list [slv_get_parmsnew $number]
    set length [llength $list]
    set name $ascSolvVect(name.$number)
    set parm_num 0
    set max_page 0
    set display_list ""
    set set_list ""
    for {set i 0} {$i < $length} {incr i} {
      if {[string compare [lindex $list $i] "New_Parm"] == 0} {
        incr i
        if {[string compare [lindex $list $i] "char_parm"] != 0} {
          set parm_type {}
          switch -exact -- [lindex $list $i] {
          int_parm {
              set parm_type int
            }
          bool_parm {
              set parm_type bool
            } 
          real_parm {
              set parm_type real
            }
          default {
              puts "Big problem in set_defaults: unrecognized type"
              # need better bailout / error checking here
              continue
            }
          }
          set j $i
          incr j
          set parm_name  [lindex $list $j]; incr j
          set parm_label  [lindex $list $j]; incr j
          set parm_val  [lindex $list $j]; incr j
          set parm_hi  [lindex $list $j]; incr j
          set parm_lo  [lindex $list $j]; incr j
          set parm_page  [lindex $list $j]; incr j
          set parm_help  [lindex $list $j]; incr j
          if {$parm_page > 0} {
            lappend display_list $parm_name
            if {$parm_page > $max_page} {
              set max_page $parm_page
            }
          }
          append set_list \$ascSolv${name}Vect($parm_name)
          append set_list " "
	  proc ${name}_parm$parm_num {} "
            entertrace
            global ascSolv${name}Vect
            set ascSolv${name}Vect($parm_name) $parm_val
            set ascSolv${name}Vect($parm_name.type) $parm_type
            set ascSolv${name}Vect($parm_name.label) \{$parm_label\}
            set ascSolv${name}Vect($parm_name.lo) $parm_lo
            set ascSolv${name}Vect($parm_name.hi) $parm_hi
            set ascSolv${name}Vect($parm_name.page) $parm_page
            set ascSolv${name}Vect($parm_name.help) \{$parm_help\}
            "
	  incr parm_num
	  set i [expr $i + 7]
	} else {
          set j $i
          incr j
	  set parm_type string
	  set parm_name  [lindex $list $j]; incr j
	  set parm_label  [lindex $list $j]; incr j
	  set parm_val  [lindex $list $j]; incr j
	  set parm_hi  [lindex $list $j]; incr j
	  set parm_list ""
	  for {set j 0} {$j < $parm_hi} {incr j} {
	    lappend parm_list [lindex $list [expr $i + 5 + $j]]
	  }
	  append set_list \$ascSolv${name}Vect($parm_name)
	  append set_list " "
	  set parm_page  [lindex $list [expr $i + 5 + $parm_hi]]
          set parm_help  [lindex $list [expr $i + 6 + $parm_hi]]
	  if {$parm_page > 0} {
	    lappend display_list $parm_name
	    if {$parm_page > $max_page} {
	      set max_page $parm_page
	    }
	  }
	  proc ${name}_parm$parm_num {} "
            entertrace
            global ascSolv${name}Vect
            set ascSolv${name}Vect($parm_name) \{$parm_val\}
            set ascSolv${name}Vect($parm_name.type) $parm_type
            set ascSolv${name}Vect($parm_name.label) \{$parm_label\}
            set ascSolv${name}Vect($parm_name.choices) \{$parm_list\}
            set ascSolv${name}Vect($parm_name.page) $parm_page
            set ascSolv${name}Vect($parm_name.page) \{$parm_help\}
            "
	  incr parm_num
	  set i [expr $i + 5 + $parm_hi]
	}
      }
    }

    # define a Defaults proc, which is rather a messy thing to do
    proc set_${name}_Defaults {} "
    entertrace
    global ascSolv${name}Vect
    set ascSolv${name}Vect(namelist) \{$display_list\}
    set ascSolv${name}Vect(toplevel) .slv${name}parms
    set ascSolv${name}Vect(title) \"$name Parameters\"
    set ascSolv${name}Vect(maxlines) 12
    set ascSolv${name}Vect(onesize) 0
    set ascSolv${name}Vect(npages) $max_page
    set ascSolv${name}Vect(grab) 0
    set ascSolv${name}Vect(cancellable) 0
    set ascSolv${name}Vect(helpcommand) \
       \"Help_button solver.$name.parameters\"
    set ascSolv${name}Vect(whenokcommand) \"Solve_do_Parms close $name\"
    for {set p 0} { \$p < $parm_num} {incr p} {
        ${name}_parm\$p
    }
    leavetrace
    "

    proc Solve_${name}_Downdate_ParmBoxNew {} "
      entertrace
      global ascSolv${name}Vect
      set_slv_parmsnew $number $set_list
      leavetrace
      "

  # Now call procedure to set defaults
  set_${name}_Defaults
}

#
# proc set_SolvG_Defaults {}
# ----------------------------------------------------------------------
# set defaults for control of the General solver, that is, a page full
# of parameters for anyone to fall back on if so desired or during
# development of the interface to a new solver.
# Subparameters, that is solver specific controls should only be mentioned
# in ascSolvXVect.
#
# Not all of the parameters on this page correspond to C variables directly.
# ----------------------------------------------------------------------
proc set_SolvG_Defaults {} {
entertrace
  global ascSolv32767Vect
# General parameter page setup variables
  set ascSolv32767Vect(namelist) [list \
    lnmepsilon update_frequency update_time dtmin dtmax dtzero \
    moststeps newlog checksing showavgs \
    yfilename obsfilename logsi logcol nearbound farnom]
  set ascSolv32767Vect(toplevel) .slv32767parms
  set ascSolv32767Vect(title) "General Parameters"
  set ascSolv32767Vect(maxlines) 12
  set ascSolv32767Vect(npages) 2
  set ascSolv32767Vect(grab) 0
  set ascSolv32767Vect(cancellable) 0
  set ascSolv32767Vect(helpcommand) ""
  set ascSolv32767Vect(whenokcommand) "Solve_do_Parms close General"
  if {![info exists ascSolv32767Vect(checksing)]} {
    # set defaults for vars, OTHERWISE assume optoin file set them.
    set ascSolv32767Vect(checksing) 0
    set ascSolv32767Vect(showavgs) 0
    set ascSolv32767Vect(update_frequency) 10
    set ascSolv32767Vect(update_time) 3
    set ascSolv32767Vect(lnmepsilon) 1e-8
    set ascSolv32767Vect(farnom) 10e3
    set ascSolv32767Vect(nearbound) 1e-3
    set ascSolv32767Vect(newlog) 1
    set ascSolv32767Vect(logcol) variable
    set ascSolv32767Vect(logsi) display
    set ascSolv32767Vect(obsfilename) "obs.dat"
    set ascSolv32767Vect(yfilename) "y.dat"
    set ascSolv32767Vect(dtzero) 0
    set ascSolv32767Vect(dtmax) 0
    set ascSolv32767Vect(dtmin) 0
    set ascSolv32767Vect(moststeps) 0
  }
  set ascSolv32767Vect(checksing.type) bool
  set ascSolv32767Vect(checksing.label) "check numeric rank after solving"
  set ascSolv32767Vect(checksing.help) \
    "checks automatically for Jacobian matrix singularity\n"
  append ascSolv32767Vect(checksing.help) "after solution with a solver that\n"
  append ascSolv32767Vect(checksing.help) "uses a Jacobian matrix."
  set ascSolv32767Vect(showavgs.type) bool
  set ascSolv32767Vect(showavgs.label) "show block summary"
  set ascSolv32767Vect(showavgs.help) \
    "automatically displays time and function/gradient statistics\n"
  append ascSolv32767Vect(showavgs.help) "for solvers which produce them\n"
  set ascSolv32767Vect(update_frequency.type) int
  set ascSolv32767Vect(update_frequency.lo) 1
  set ascSolv32767Vect(update_frequency.label) \
    "iterations before screen update"
  set ascSolv32767Vect(update_frequency.help) \
    "The GUI update at each solver iteration can be expensive sometimes.\n"
  append ascSolv32767Vect(update_frequency.help) \
    "For faster performance, with less user feedback, make this number large."
  append ascSolv32767Vect(update_frequency.help) \
    "\nFor maximum interactivity, make this number 1."
  set ascSolv32767Vect(update_time.type) int
  set ascSolv32767Vect(update_time.lo) 1
  set ascSolv32767Vect(update_time.label) "cpu sec before screen update"
  set ascSolv32767Vect(update_time.help) \
    "This is the maximum time, regardless of iteration count, allowed before \n"
  append ascSolv32767Vect(update_time.help) \
    "a GUI update will be scheduled. Once scheduled, the GUI update will\n"
  append ascSolv32767Vect(update_time.help) \
    "occur at the end of the next iteration where the solver returns control\n"
  append ascSolv32767Vect(update_time.help) "to ASCEND."

  set ascSolv32767Vect(lnmepsilon.label) "modified log epsilon"
  set ascSolv32767Vect(lnmepsilon.lo) 1e-16
  set ascSolv32767Vect(lnmepsilon.hi) 0.5
  set ascSolv32767Vect(lnmepsilon.type) real
  set ascSolv32767Vect(moststeps.page) 2
  set ascSolv32767Vect(moststeps.label) "most integrator steps per time sample"
  set ascSolv32767Vect(moststeps.lo) 0
  set ascSolv32767Vect(moststeps.type) int
  set ascSolv32767Vect(moststeps.help) \
   "If this number is 0, integrators will use their built-in limit.\n"
  append ascSolv32767Vect(moststeps.help) \
   "You can tell the integrator a different limit by setting this to non-0."
  set ascSolv32767Vect(dtmin.page) 2
  set ascSolv32767Vect(dtmin.label) "minimum integrator step (SI units)"
  set ascSolv32767Vect(dtmin.lo) 0
  set ascSolv32767Vect(dtmin.hi) 20.0
  set ascSolv32767Vect(dtmin.type) real
  set ascSolv32767Vect(dtmin.help) \
   "If this number is 0, integrators will use their built-in limit.\n"
  append ascSolv32767Vect(dtmin.help) \
   "You can tell the integrator a different limit by setting this to non-0."
  set ascSolv32767Vect(dtmax.page) 2
  set ascSolv32767Vect(dtmax.label) "maximum integrator step (SI units)"
  set ascSolv32767Vect(dtmax.lo) 0
  set ascSolv32767Vect(dtmax.hi) 20.0
  set ascSolv32767Vect(dtmax.type) real
  set ascSolv32767Vect(dtmax.help) \
   "If this number is 0, integrators will use their built-in limit.\n"
  append ascSolv32767Vect(dtmax.help) \
   "You can tell the integrator a different limit by setting this to non-0."
  set ascSolv32767Vect(dtzero.page) 2
  set ascSolv32767Vect(dtzero.label) "initial integrator step size (SI units)"
  set ascSolv32767Vect(dtzero.lo) 0
  set ascSolv32767Vect(dtzero.hi) 20.0
  set ascSolv32767Vect(dtzero.type) real
  set ascSolv32767Vect(dtzero.help) \
   "If this number is 0, integrators will use their built-in initial step.\n"
  append ascSolv32767Vect(dtzero.help) \
   "You can tell the integrator a different start by setting this to non-0.\n"
  append ascSolv32767Vect(dtzero.help) \
   "If your problem is very stiff, setting this to a small value may help."
  set ascSolv32767Vect(yfilename.page) 2
  set ascSolv32767Vect(yfilename.label) "integrator state log"
  set ascSolv32767Vect(yfilename.type) string
  set ascSolv32767Vect(yfilename.help) \
    "directory path and file to store log of state and derivative values"
  set ascSolv32767Vect(obsfilename.page) 2
  set ascSolv32767Vect(obsfilename.label) "integrator observation log"
  set ascSolv32767Vect(obsfilename.type) string
  set ascSolv32767Vect(obsfilename.help) \
    "directory path and file to store observation profiles"
  set ascSolv32767Vect(logsi.page) 2
  set ascSolv32767Vect(logsi.type) string
  set ascSolv32767Vect(logsi.label) "integrator log SI units"
  set ascSolv32767Vect(logsi.choices) "si display"
  set ascSolv32767Vect(logsi.help) "Observation and state logs are written\n"
  append ascSolv32767Vect(logsi.help) \
    "in either SI or your current displayed units"
  set ascSolv32767Vect(logcol.page) 2
  set ascSolv32767Vect(logcol.type) string
  set ascSolv32767Vect(logcol.label) "integrator log columns"
  set ascSolv32767Vect(logcol.choices) "variable fixed"
  set ascSolv32767Vect(logcol.help) "Observation and state logs are written\n"
  append ascSolv32767Vect(logcol.help) \
    "in either fixed or variable column width for consumption by other software"
  set ascSolv32767Vect(newlog.page) 2
  set ascSolv32767Vect(newlog.type) bool
  set ascSolv32767Vect(newlog.label) "overwrite integrator logs"
  set ascSolv32767Vect(nearbound.label) "bound check epsilon"
  set ascSolv32767Vect(nearbound.lo) 0
  set ascSolv32767Vect(nearbound.type) real
  set ascSolv32767Vect(farnom.label) "far from nom bignum"
  set ascSolv32767Vect(farnom.lo) 0
  set ascSolv32767Vect(farnom.type) real
leavetrace
}

#
# proc Solve_Build_EngineMenus {}
# ----------------------------------------------------------------------
# construct choices for all the linked solvers
# ----------------------------------------------------------------------
proc Solve_Build_EngineMenus {} {
  entertrace
  global ascSolvVect
  for {set s 0} {$s<$ascSolvVect(numberofsolvers)} {incr s} {
    if {$ascSolvVect(available.$s)} {
      set state active
    } else {
      set state disabled
    }
    .solver.lbl_frm.entry5.m add command \
      -command "Solve_do_Select $ascSolvVect(name.$s)" \
      -label "$ascSolvVect(name.$s)" \
      -state $state
    .solver.lbl_frm.btn_opts.m add command \
      -command "Solve_do_Parms open $ascSolvVect(name.$s)" \
      -label "$ascSolvVect(name.$s) ..." \
      -state $state
  }
leavetrace
}

#
# proc Solve_Build_SaveParsMenus {}
# ----------------------------------------------------------------------
# construct choices for all the linked solvers
# ----------------------------------------------------------------------
proc Solve_Build_SaveParsMenus {} {
  entertrace
  global ascSolvVect
  for {set s 0} {$s<$ascSolvVect(numberofsolvers)} {incr s} {

    $ascSolvVect(saveparmenu) add command \
    -command "View_Save_Solver_Params $s" \
    -label $ascSolvVect(name.$s)

  }
leavetrace
}

#
# proc set_Solv0_Defaults {}
# ----------------------------------------------------------------------
# set defaults for control of Slv
# ----------------------------------------------------------------------
proc set_Solv0_Defaults {} {
entertrace
  global ascSolv0Vect
  # SLV parameter page setup variables
  set ascSolv0Vect(namelist) [list partition showlessimportant timelimit \
   iterationlimit singtol pivottol feastol rho autoresolve \
   showlessimportantds savlin]
  set ascSolv0Vect(toplevel) .slv0parms
  set ascSolv0Vect(title) "Slv Parameters"
  set ascSolv0Vect(maxlines) 12
  set ascSolv0Vect(npages) 2
  set ascSolv0Vect(grab) 0
  set ascSolv0Vect(helpcommand) "Help_button solver.slv.parameters"
  set ascSolv0Vect(whenokcommand) "Solve_do_Parms close Slv"
# not window page supported Slv parms
  set ascSolv0Vect(ignorebounds) "0"
  set ascSolv0Vect(showmoreimportant) "1"
  set ascSolv0Vect(termtol) 1e-11
# SLV parameter page variables
  set ascSolv0Vect(rho) "100"
  set ascSolv0Vect(rho.page) 1
  set ascSolv0Vect(rho.type) real
  set ascSolv0Vect(rho.lo) 0
  set ascSolv0Vect(rho.label) "penalty parameter"
  set ascSolv0Vect(partition) "1"
  set ascSolv0Vect(partition.page) 1
  set ascSolv0Vect(partition.type) bool
  set ascSolv0Vect(partition.label) "partitioning enabled"
  set ascSolv0Vect(bppivoting) "0"
  set ascSolv0Vect(bppivoting.page) 2
  set ascSolv0Vect(bppivoting.type) bool
  set ascSolv0Vect(bppivoting.label) "bipartial pivoting"
  set ascSolv0Vect(showlessimportant) "0"
  set ascSolv0Vect(showlessimportant.page) 1
  set ascSolv0Vect(showlessimportant.type) bool
  set ascSolv0Vect(showlessimportant.label) "detailed solving info required"
  set ascSolv0Vect(showlessimportantds) 0
  set ascSolv0Vect(showlessimportantds.page) 2
  set ascSolv0Vect(showlessimportantds.type) bool
  set ascSolv0Vect(showlessimportantds.label) "show singletons details"
  set ascSolv0Vect(savlin) 0
  set ascSolv0Vect(savlin.page) 2
  set ascSolv0Vect(savlin.type) bool
  set ascSolv0Vect(savlin.label) "write to file SlvLinsol.dat"
  set ascSolv0Vect(autoresolve) 1
  set ascSolv0Vect(autoresolve.page) 1
  set ascSolv0Vect(autoresolve.type) bool
  set ascSolv0Vect(autoresolve.label) auto-resolve
  set ascSolv0Vect(timelimit) 1000
  set ascSolv0Vect(timelimit.page) 1
  set ascSolv0Vect(timelimit.type) int
  set ascSolv0Vect(timelimit.lo) 1
  set ascSolv0Vect(timelimit.label) "time limit (CPU sec/block)"
  set ascSolv0Vect(iterationlimit) 20
  set ascSolv0Vect(iterationlimit.page) 1
  set ascSolv0Vect(iterationlimit.lo) 1
  set ascSolv0Vect(iterationlimit.type) int
  set ascSolv0Vect(iterationlimit.label) "maximum iterations/block"
  set ascSolv0Vect(stattol) "1e-6"
  set ascSolv0Vect(stattol.page) 1
  set ascSolv0Vect(termtol) "1e-12"
  set ascSolv0Vect(singtol) "1e-12"
  set ascSolv0Vect(singtol.page) 1
  set ascSolv0Vect(singtol.type) real
  set ascSolv0Vect(singtol.lo) 1e-12
  set ascSolv0Vect(singtol.label) "epsilon (minimum pivot)"
  set ascSolv0Vect(pivottol) "0.01"
  set ascSolv0Vect(pivottol.page) 1
  set ascSolv0Vect(pivottol.label) "pivot tolerance"
  set ascSolv0Vect(pivottol.lo) 0
  set ascSolv0Vect(pivottol.hi) 1
  set ascSolv0Vect(pivottol.type) real
  set ascSolv0Vect(feastol) "1e-8"
  set ascSolv0Vect(feastol.page) 1
  set ascSolv0Vect(feastol.lo) "1e-13"
  set ascSolv0Vect(feastol.type) real
  set ascSolv0Vect(feastol.label) "max. residual (absolute)"
leavetrace
}
#
# proc set_Solv1_Defaults {}
# ----------------------------------------------------------------------
# set defaults for control of minos
# keep me until we resurrect minos
# ----------------------------------------------------------------------
proc set_Solv1_Defaults {} {
entertrace
  global ascSolv1Vect
  # SLV parameter page setup variables
  set ascSolv1Vect(namelist) [list damp mindamp tolsing tolfeas tolstat  \
    timelimit majits rho showlessimportant autoresolve \
    completion crash cfreq ffreq uselg lfreq deriv minits mulpr \
    parpr printJ printF printL printX printB scale param verify \
    fdiff cdiff fprec lstol lufto luuto ludto lusto luwto \
    subsp radius objlim steplm summary filesumm \
    lobjwt soln lcons]
  set ascSolv1Vect(toplevel) .slv1parms
  set ascSolv1Vect(title) "MINOS Parameters"
  set ascSolv1Vect(maxlines) 15
  set ascSolv1Vect(npages) 4
  set ascSolv1Vect(grab) 0
  set ascSolv1Vect(helpcommand) "Help_button solver.minos.parameters"
  set ascSolv1Vect(whenokcommand) "Solve_do_Parms close MINOS"
  # not window page supported minos parms
  set ascSolv1Vect(showmoreimportant) "1"
  # MINOS parameter page variables

  set ascSolv1Vect(tolsing) "1e-11"
  set ascSolv1Vect(tolsing.page) 1
  set ascSolv1Vect(tolsing.type) real
  set ascSolv1Vect(tolsing.lo) 1e-16
  set ascSolv1Vect(tolsing.label) "Epsilon (Pivot Tolerance)"
  set ascSolv1Vect(tolfeas) "1e-8"
  set ascSolv1Vect(tolfeas.page) 1
  set ascSolv1Vect(tolfeas.label) "Max. residual"
  set ascSolv1Vect(tolfeas.lo) 1e-16
  set ascSolv1Vect(tolfeas.type) real
  set ascSolv1Vect(tolstat) "1e-8"
  set ascSolv1Vect(tolstat.page) 2
  set ascSolv1Vect(tolstat.label) "Optimality tolerance"
  set ascSolv1Vect(tolstat.lo) 1e-16
  set ascSolv1Vect(tolstat.type) real
  set ascSolv1Vect(majits) "20"
  set ascSolv1Vect(majits.page) "1"
  set ascSolv1Vect(majits.lo) 1
  set ascSolv1Vect(majits.type) int
  set ascSolv1Vect(majits.label) "Major iterations limit"

  set ascSolv1Vect(timelimit) "500"
  set ascSolv1Vect(timelimit.page) 1
  set ascSolv1Vect(timelimit.type) int
  set ascSolv1Vect(timelimit.lo) 1
  set ascSolv1Vect(timelimit.label) "Time limit (CPU sec)"
  set ascSolv1Vect(rho) "1e-11"
  set ascSolv1Vect(rho.page) 1
  set ascSolv1Vect(rho.type) real
  set ascSolv1Vect(rho.lo) 0
  set ascSolv1Vect(rho.label) "Penalty parameter"
  set ascSolv1Vect(autoresolve) "0"
  set ascSolv1Vect(autoresolve.page) "1"
  set ascSolv1Vect(autoresolve.type) bool
  set ascSolv1Vect(autoresolve.label) auto-resolve
  set ascSolv1Vect(showlessimportant) 0
  set ascSolv1Vect(showlessimportant.page) 1
  set ascSolv1Vect(showlessimportant.type) bool
  set ascSolv1Vect(showlessimportant.label) "detailed solving info required"

  set ascSolv1Vect(completion) 1
  set ascSolv1Vect(completion.page) 2
  set ascSolv1Vect(completion.type) bool
  set ascSolv1Vect(completion.label) "Full completion"
  set ascSolv1Vect(crash) 1
  set ascSolv1Vect(crash.page) 2
  set ascSolv1Vect(crash.type) int
  set ascSolv1Vect(crash.lo) 0
  set ascSolv1Vect(crash.hi) 4
  set ascSolv1Vect(crash.label) "Crash option"
  set ascSolv1Vect(deriv) 3
  set ascSolv1Vect(deriv.page) 4
  set ascSolv1Vect(deriv.type) int
  set ascSolv1Vect(deriv.lo) 0
  set ascSolv1Vect(deriv.hi) 3
  set ascSolv1Vect(deriv.label) "Derivative level"
  set ascSolv1Vect(cfreq) 30
  set ascSolv1Vect(cfreq.page) 4
  set ascSolv1Vect(cfreq.lo) 1
  set ascSolv1Vect(cfreq.type) int
  set ascSolv1Vect(cfreq.label) "Linear check frequency"
  set ascSolv1Vect(ffreq) 50
  set ascSolv1Vect(ffreq.page) 4
  set ascSolv1Vect(ffreq.lo) 0
  set ascSolv1Vect(ffreq.type) int
  set ascSolv1Vect(ffreq.label) "Basis factorization frequency"
  set ascSolv1Vect(efreq) 0
  set ascSolv1Vect(efreq.page) 4
  set ascSolv1Vect(efreq.lo) 0
  set ascSolv1Vect(efreq.type) int
  set ascSolv1Vect(efreq.label) "Expand frequency"
  set ascSolv1Vect(uselg) 1
  set ascSolv1Vect(uselg.page) 2
  set ascSolv1Vect(uselg.type) bool
  set ascSolv1Vect(uselg.label) "Lagrangian subproblems"
  set ascSolv1Vect(lfreq) 10
  set ascSolv1Vect(lfreq.page) 3
  set ascSolv1Vect(lfreq.lo) 1
  set ascSolv1Vect(lfreq.type) int
  set ascSolv1Vect(lfreq.label) "Log frequency"
  set ascSolv1Vect(minits) 40
  set ascSolv1Vect(minits.page) 2
  set ascSolv1Vect(minits.lo) 0
  set ascSolv1Vect(minits.type) int
  set ascSolv1Vect(minits.label) "Minor iterations limit"
  set ascSolv1Vect(mulpr) 1
  set ascSolv1Vect(mulpr.page) 2
  set ascSolv1Vect(mulpr.lo) 0
  set ascSolv1Vect(mulpr.type) int
  set ascSolv1Vect(mulpr.label) "Multiple price"

  set ascSolv1Vect(parpr) 0
  set ascSolv1Vect(parpr.page) 2
  set ascSolv1Vect(parpr.lo) 0
  set ascSolv1Vect(parpr.type) int
  set ascSolv1Vect(parpr.label) "Partial price"
  set ascSolv1Vect(summary) 0
  set ascSolv1Vect(summary.page) 3
  set ascSolv1Vect(summary.type) bool
  set ascSolv1Vect(summary.label) "Show PRINT output"
  set ascSolv1Vect(filesumm) 0
  set ascSolv1Vect(filesumm.page) 3
  set ascSolv1Vect(filesumm.type) bool
  set ascSolv1Vect(filesumm.label) "Save summary file"
  set ascSolv1Vect(printJ) 0
  set ascSolv1Vect(printJ.page) 3
  set ascSolv1Vect(printJ.type) bool
  set ascSolv1Vect(printJ.label) "Print jacobian"
  set ascSolv1Vect(printF) 0
  set ascSolv1Vect(printF.page) 3
  set ascSolv1Vect(printF.type) bool
  set ascSolv1Vect(printF.label) "Print nonlinear residuals"
  set ascSolv1Vect(printL) 0
  set ascSolv1Vect(printL.page) 3
  set ascSolv1Vect(printL.type) bool
  set ascSolv1Vect(printL.label) "Print multiplier estimates"
  set ascSolv1Vect(printX) 0
  set ascSolv1Vect(printX.page) 3
  set ascSolv1Vect(printX.type) bool
  set ascSolv1Vect(printX.label) "Print nonlinear variables"
  set ascSolv1Vect(soln) 0
  set ascSolv1Vect(soln.page) 3
  set ascSolv1Vect(soln.type) bool
  set ascSolv1Vect(soln.label) "Print solution"
  set ascSolv1Vect(printB) 0
  set ascSolv1Vect(printB.page) 3
  set ascSolv1Vect(printB.type) bool
  set ascSolv1Vect(printB.label) "Print basis stats"
  set ascSolv1Vect(scale) 0
  set ascSolv1Vect(scale.page) 4
  set ascSolv1Vect(scale.type) bool
  set ascSolv1Vect(scale.label) "Let minos scale, too"
  set ascSolv1Vect(param) 0
  set ascSolv1Vect(param.page) 3
  set ascSolv1Vect(param.type) bool
  set ascSolv1Vect(param.label) "Suppress parameter echo"
  set ascSolv1Vect(verify) -1
  set ascSolv1Vect(verify.page) 4
  set ascSolv1Vect(verify.type) int
  set ascSolv1Vect(verify.lo) -1
  set ascSolv1Vect(verify.hi) 3
  set ascSolv1Vect(verify.label) "Verify level"
  set ascSolv1Vect(lcons) 1
  set ascSolv1Vect(lcons.page) 3
  set ascSolv1Vect(lcons.type) bool
  set ascSolv1Vect(lcons.label) "Force nonlinearity"
  set ascSolv1Vect(damp) 2.0
  set ascSolv1Vect(damp.page) 2
  set ascSolv1Vect(damp.type) real
  set ascSolv1Vect(damp.lo) 0
  set ascSolv1Vect(damp.label) "Major damping parameter"
  set ascSolv1Vect(mindamp) 2.0
  set ascSolv1Vect(mindamp.page) 2
  set ascSolv1Vect(mindamp.type) real
  set ascSolv1Vect(mindamp.lo) 0
  set ascSolv1Vect(mindamp.label) "Minor damping parameter"
  set ascSolv1Vect(fdiff) 0
  set ascSolv1Vect(fdiff.page) 4
  set ascSolv1Vect(fdiff.type) real
  set ascSolv1Vect(fdiff.lo) 0
  set ascSolv1Vect(fdiff.label) "Difference interval"
  set ascSolv1Vect(cdiff) 0
  set ascSolv1Vect(cdiff.page) 4
  set ascSolv1Vect(cdiff.type) real
  set ascSolv1Vect(cdiff.lo) 0
  set ascSolv1Vect(cdiff.label) "Central difference interval"
  set ascSolv1Vect(fprec) 1e-6
  set ascSolv1Vect(fprec.page) 4
  set ascSolv1Vect(fprec.type) real
  set ascSolv1Vect(fprec.lo) 0
  set ascSolv1Vect(fprec.label) "Function precision"
  set ascSolv1Vect(lstol) "0.1"
  set ascSolv1Vect(lstol.page) 2
  set ascSolv1Vect(lstol.type) real
  set ascSolv1Vect(lstol.lo) 0
  set ascSolv1Vect(lstol.hi) 1
  set ascSolv1Vect(lstol.label) "Linesearch tolerance"
  set ascSolv1Vect(lufto) 10
  set ascSolv1Vect(lufto.page) 2
  set ascSolv1Vect(lufto.type) real
  set ascSolv1Vect(lufto.lo) 1
  set ascSolv1Vect(lufto.label) "LU factor tolerance"
  set ascSolv1Vect(luuto) 10
  set ascSolv1Vect(luuto.page) 2
  set ascSolv1Vect(luuto.type) real
  set ascSolv1Vect(luuto.lo) 1
  set ascSolv1Vect(luuto.label) "LU update tolerance"
  set ascSolv1Vect(luwto) 0
  set ascSolv1Vect(luwto.page) 4
  set ascSolv1Vect(luwto.type) real
  set ascSolv1Vect(luwto.lo) 0
  set ascSolv1Vect(luwto.label) "LU swap tolerance"
  set ascSolv1Vect(lusto) 0
  set ascSolv1Vect(lusto.page) 4
  set ascSolv1Vect(lusto.type) real
  set ascSolv1Vect(lusto.lo) 0
  set ascSolv1Vect(lusto.label) "LU singularity tolerance"
  set ascSolv1Vect(ludto) 0
  set ascSolv1Vect(ludto.page) 4
  set ascSolv1Vect(ludto.type) real
  set ascSolv1Vect(ludto.lo) 0
  set ascSolv1Vect(ludto.hi) 1
  set ascSolv1Vect(ludto.label) "LU density tolerance"
  set ascSolv1Vect(radius) 0.01
  set ascSolv1Vect(radius.page) 2
  set ascSolv1Vect(radius.type) real
  set ascSolv1Vect(radius.lo) 0
  set ascSolv1Vect(radius.label) "Radius of convergence"
  set ascSolv1Vect(subsp) 0.5
  set ascSolv1Vect(subsp.page) 2
  set ascSolv1Vect(subsp.type) real
  set ascSolv1Vect(subsp.lo) 0
  set ascSolv1Vect(subsp.hi) 1
  set ascSolv1Vect(subsp.label) "Subspace tolerance"
  set ascSolv1Vect(objlim) 1e20
  set ascSolv1Vect(objlim.page) 4
  set ascSolv1Vect(objlim.type) real
  set ascSolv1Vect(objlim.lo) 0
  set ascSolv1Vect(objlim.label) "Unbounded obj. value"
  set ascSolv1Vect(steplm) 1e10
  set ascSolv1Vect(steplm.page) 4
  set ascSolv1Vect(steplm.type) real
  set ascSolv1Vect(steplm.lo) 0
  set ascSolv1Vect(steplm.label) "Unbounded step size"
  set ascSolv1Vect(lobjwt) 0
  set ascSolv1Vect(lobjwt.page) 4
  set ascSolv1Vect(lobjwt.type) real
  set ascSolv1Vect(lobjwt.lo) 0
  set ascSolv1Vect(lobjwt.label) "Weight on linear obj."
leavetrace
}

#
# proc set_Solv6_Defaults {}
# ----------------------------------------------------------------------
# keep me until we resurrect mps.
# set defaults for control of makeMPS,   modified by CWS 5/95
# ----------------------------------------------------------------------
proc set_Solv6_Defaults {} {
entertrace
  global ascSolv6Vect

#  general parameters
   set ascSolv6Vect(timelimit)          1000
   set ascSolv6Vect(iterationlimit)     20
   set ascSolv6Vect(termtol)            1e-12
   set ascSolv6Vect(feastol)            1e-8
   set ascSolv6Vect(pivottol)           0.01
   set ascSolv6Vect(singtol)            1e-12
   set ascSolv6Vect(stattol)            1e-6
   set ascSolv6Vect(rho)                100
   set ascSolv6Vect(partition)          0
   set ascSolv6Vect(ignorebounds)       0
   set ascSolv6Vect(showmoreimportant)  1
   set ascSolv6Vect(showlessimportant)  0
   set ascSolv6Vect(bppivoting)         0

#   subparameters
#   solve nonlinear model by linearizing at current point
    set ascSolv6Vect(nonlin)            0
    set ascSolv6Vect(relaxed)           0
    set ascSolv6Vect(nonneg)            0
    set ascSolv6Vect(obj)               0
    set ascSolv6Vect(binary)            0
    set ascSolv6Vect(integer)           0
    set ascSolv6Vect(semi)              0
    set ascSolv6Vect(sos1)              0
    set ascSolv6Vect(sos2)              0
    set ascSolv6Vect(sos3)              0
    set ascSolv6Vect(bo)                0
    set ascSolv6Vect(eps)               0
    set ascSolv6Vect(boval)             ""
    set ascSolv6Vect(epsval)            ""
#   Note: pinf and minf should be entered by the user, instead of being
#         hard coded in
    set ascSolv6Vect(pinf)              1e+20
    set ascSolv6Vect(minf)              -1e+20
    set ascSolv6Vect(filename)          output.mps


#   dialog variables

    set ascSolv6Vect(SOS) 0
#   filename is what appears in dialog, expandname is exapanded ~/*.mps, etc.
    set {ascSolv6Vect(expandname)} {output.mps}
    set {ascSolv6Vect(solver)} {QOMILP}

#  set {symbolicName(ascSolv6Vect(help))} {.mps.frame14.button3}
#  set {symbolicName(ascSolv6Vect(makemps))} {.mps.frame14.button1}
#  set {symbolicName(ascSolv6Vect(ok))} {.mps.frame14.button0}
#  set {symbolicName(ascSolv6Vect(run))} {.mps.frame14.button2}

# dialogup true when dialog is visible
  set ascSolv6Vect(dialogup) 0

# massaged, expanded filenames
  set ascSolv6Vect(mpsname) output.mps
  set ascSolv6Vect(mapname) output.map
  set ascSolv6Vect(errname) output.err
  set ascSolv6Vect(outname) output.out

leavetrace
}


# can we delete this?
# proc set_Solv9a_Defaults {}
# ----------------------------------------------------------------------
# set defaults for control of LRSlv
# ----------------------------------------------------------------------
proc set_Solv9a_Defaults {} {
entertrace
  global ascSolv9aVect
  # LRSlv parameter page setup variables
    set ascSolv9aVect(namelist) [showlessimportant timelimit \
   iterationlimit autoresolve]
  set ascSolv9aVect(toplevel) .slv9aparms
  set ascSolv9aVect(title) "LRSlv Parameters"
  set ascSolv9aVect(maxlines) 12
  set ascSolv9aVect(npages) 2
  set ascSolv9aVect(grab) 0
  set ascSolv9aVect(helpcommand) "Help_button solver.LRSlv.parameters"
  set ascSolv9aVect(whenokcommand) "Solve_do_Parms close LRSlv"
  # not window page supported Slv parms
  set ascSolv9aVect(showmoreimportant) "1"
  set ascSolv9aVect(showlessimportant) "1"
  set ascSolv9aVect(showlessimportant.page) 2
  set ascSolv9aVect(showlessimportant.type) bool
  set ascSolv9aVect(showlessimportant.label) "detailed solving info"
  set ascSolv9aVect(autoresolve) "1"
  set ascSolv9aVect(autoresolve.page) 2
  set ascSolv9aVect(autoresolve.type) bool
  set ascSolv9aVect(autoresolve.label) auto-resolve
  set ascSolv9aVect(timelimit) "1500"
  set ascSolv9aVect(timelimit.type) int
  set ascSolv9aVect(timelimit.lo) 1
  set ascSolv9aVect(timelimit.label) "time limit (CPU sec/block)"
  set ascSolv9aVect(iterationlimit) "30"
  set ascSolv9aVect(iterationlimit.lo) 1
  set ascSolv9aVect(iterationlimit.type) int
  set ascSolv9aVect(iterationlimit.label) "max iterations/block"
leavetrace
}



#
#----------------------------------------------------------------------------
# Direct callbacks section. procedures are all Solve_do_* and are supposed  #
# to pertain to the .solver window and environs. Debugger is separated.     #
# mtxview is separated also.                                                #
#----------------------------------------------------------------------------

#
# proc Solve_do_Select {name}
#----------------------------------------------------------------------------
# Solver Selection buttons action                                           #
# If system not empty, downdates control parameters and calls presolve      #
# does not check sanity of selection wrt problem.                           #
#----------------------------------------------------------------------------
proc Solve_do_Select {name} {
entertrace
  global ascSolvVect ascSolvStatVect
  if { $ascSolvVect($name) == -1 } {
      puts " Solver $name not available"
      leavetrace
      return
  }
  set ascSolvStatVect(solver) $name
  set ascSolvStatVect(solvernum) $ascSolvVect($name)
# KHACK: only need to select solver if different from current solver
  if {[slv_checksys]} {
    if {$name != [Solve_do_SolverCur]} {
	  slv_select_solver $ascSolvStatVect(solvernum)
    }
    if {[catch {slv_presolve} ]} {
      Solve_FP_error "Initial values,cause float error.,Please fix this."
    }
  }
  Solve_Update_StatusBox
leavetrace
}

#
# proc Solve_Close_Slaves {}
#----------------------------------------------------------------------------
# shut down windows dependent on solve system presence: debug, mtx, debug2
#----------------------------------------------------------------------------
proc Solve_Close_Slaves {} {
entertrace
  global ascSolvVect
  if {$ascSolvVect(debuggerup)} {
    Solve_CloseDebugger
  }
  if {$ascSolvVect(mtxup)} {
    Solve_CloseMtx
  }
leavetrace
}
#
# proc Solve_do_Flush {args}
#----------------------------------------------------------------------------
# Solve RemoveInstance (Flush solver)                                       #
# If any args are supplied the HUB is not notified of the flush.            #
# This makes for a neater looking script when a call which is               #
# being recorded calls this function.                                       #
#----------------------------------------------------------------------------
proc Solve_do_Flush {args} {
entertrace
  global ascSolvVect ascSolvStatVect
  Solve_Close_Slaves
  slv_flush_solver
  Solve_destroy_monitors
  if {[llength $args] == 0} {
      HUB_Message_to_HUB SYSFLUSH
  }
  set ascSolvStatVect(empty) 1
  set ascSolvVect(instname) ""
  set ascSolvVect(objvarname) ""
  set ascSolvVect(simname) ""
  Solve_Update_StatusBox
  Solve_Update_Listbox
  Solve_Update_MenuBar
leavetrace
}

#
# proc Solve_do_Select_Objective {}
#----------------------------------------------------------------------------
# Select Objective variable                                                 #
# list select from free variable list and objs list                         #
# This needs to be updated to cope with objective vars.
# u_getval and slv_set_objective_var should be useful.
#----------------------------------------------------------------------------
proc Solve_do_Select_Objective {} {
entertrace
  if {![slv_checksys]} {leavetrace; return}
  Solve_Find_Objectives 1
leavetrace
}

#
# proc Solve_do_Display_Status {}
#----------------------------------------------------------------------------
# Displays solver status button box,                                        #
#----------------------------------------------------------------------------
proc Solve_do_Display_Status {} {
entertrace
  global ascGlobalVect

  Solve_Update_StatusBox
  Solve_Status_Infobox
leavetrace
}

#
# proc Solve_do_Spreadsheet {}
#----------------------------------------------------------------------------
# Display spreadsheet. dumps vars/relations to file                         #
# rather low priority                                                       #
#----------------------------------------------------------------------------
proc Solve_do_Spreadsheet {} {
entertrace
  puts "Not yet implemented"
leavetrace
}

#
# proc Solve_do_DispUnattached {}
#----------------------------------------------------------------------------
# Display unattached vars                                                   #
#----------------------------------------------------------------------------
proc Solve_do_DispUnattached {} {
entertrace
  global ascSolvVect
  puts stderr "Unattached variables in $ascSolvVect(instname):"
  dbg_write_unattvar 1 $ascSolvVect(simname)
leavetrace
}


#
# proc Solve_do_DispUnincluded {}
#----------------------------------------------------------------------------
# Display unincluded relations                                              #
#----------------------------------------------------------------------------
proc Solve_do_DispUnincluded {} {
entertrace
  global ascSolvVect
  puts stderr "Unincluded relations in $ascSolvVect(instname):"
  foreach i [dbg_list_rels 1 not] {
    dbg_write_rel 0 $i 0 $ascSolvVect(simname)
  }
leavetrace
}

#
# proc Solve_do_DispIncidence {}
#----------------------------------------------------------------------------
# Display incidence matrix                                                  #
#----------------------------------------------------------------------------
proc Solve_do_DispIncidence {} {
entertrace
  global ascSolvStatVect
  if {$ascSolvStatVect(empty)} {
    Solve_CloseMtx
  } else {
    if {![slv_checksys]} {leavetrace; return}
    Solve_OpenMtx
  }
leavetrace
}

#
# proc Solve_do_Solve {}
#----------------------------------------------------------------------------
# Solve button                                                              #
#----------------------------------------------------------------------------
proc Solve_do_Solve {} {
entertrace
  if {![slv_checksys]} {
    return
  }
  Solve_monitor_init
  Solve_Solve
  Solve_Status_Infobox
leavetrace
}

#
# proc Solve_do_Iterate {}
#----------------------------------------------------------------------------
# Single step button                                                        #
#----------------------------------------------------------------------------
proc Solve_do_Iterate {} {
entertrace
  if {![slv_checksys]} {leavetrace; return }
  Solve_Iterate
leavetrace
}

#
# proc Solve_do_Integrate {}
#----------------------------------------------------------------------------
# Integration button                                                        #
#----------------------------------------------------------------------------
proc Solve_do_Integrate {} {
entertrace
  if {![slv_checksys]} {leavetrace; return }
  Solve_Integrate
leavetrace
}

#
# proc Solve_do_Optimize {}
#----------------------------------------------------------------------------
# Optimization button                                                       #
#----------------------------------------------------------------------------
proc Solve_do_Optimize {} {
entertrace
  Solve_Optimize
  Solve_Status_Infobox
leavetrace
}

#
# proc Solve_do_Debug {}
#----------------------------------------------------------------------------
# Debugger  button                                                    #
#----------------------------------------------------------------------------
proc Solve_do_Debug {} {
entertrace
  if {![slv_checksys]} { leavetrace; return }
  global ascSolvVect
  set_Debug_Defaults
  if {!$ascSolvVect(debuggerup)} {
    Solve_OpenDebugger
  } else {
    Solve_CloseDebugger
    Solve_OpenDebugger
  }
  .debug configure -cursor left_ptr
leavetrace
}

#
# proc Solve_do_Overspecified {}
#----------------------------------------------------------------------------
# Overspecified Button                                                      #
#----------------------------------------------------------------------------
proc Solve_do_Overspecified {} {
entertrace
  Solve_FindOverspec 1
leavetrace
}
#
# proc Solve_FindOverspec {refresh}
#----------------------------------------------------------------------------
# Overspecified internals if refresh 1 update status after free             #
#----------------------------------------------------------------------------
proc Solve_FindOverspec {refresh} {
entertrace
  global ascListSelectBox ascSolvVect
  set list ""
  catch {set list [lindex [dbg_struct_singular 2 -1] 2]}
  set ascListSelectBox(grab) 1
  set ascListSelectBox(btn3name) ""
  set ascListSelectBox(btn4name) ""
  set ascListSelectBox(btn5name) ""
  set ascListSelectBox(btn3destroy) 1
  set ascListSelectBox(btn4destroy) 0
  set ascListSelectBox(btn4command) ""
  set ascListSelectBox(title) "Overspecified"
  set ascListSelectBox(toplevelname) ".solvoverspec"
  set ascListSelectBox(font) $ascSolvVect(font)
  set ascListSelectBox(selectmode) browse
  set ascListSelectBox(headline) "Select to free one of:"
  set newlist ""
  set pname [slv_get_pathname]
  foreach i $list {
    lappend newlist "$pname.[stripbraces [dbg_write_var 2 $i 0 0]]"
  }
  if {$newlist==""} {
    if {!$refresh} {
      puts stderr "No variables to free."
      Solve_do_StrucDepends
    }
    leavetrace; return
  }
  set alist [lsort $newlist]
  set button [AscListSelectBox $alist \
                250x240[setpos .solver 50 20]]
  if {$button==2} {
    leavetrace
    return
  }
  Solve_OverListSelect $refresh

leavetrace
}

#
# proc Solve_do_NumDepends
#----------------------------------------------------------------------------
# Numeric Dependency Button                                                 #
#----------------------------------------------------------------------------
proc Solve_do_NumDepends {} {
entertrace
  Solve_Check_RowsPivoted 1
leavetrace
}

#
# proc Solve_do_StrucDepends {}
#----------------------------------------------------------------------------
# Structural Dependency Button                                              #
#----------------------------------------------------------------------------
proc Solve_do_StrucDepends {} {
entertrace

  set slist [Solve_get_unassigned_rels]
  if {$slist==""} {
    puts stderr "There are no unassignable relations."
    leavetrace; return
  }
#leavetrace; return
# need to box this up.
  foreach rel $slist {
    set vr [dbg_struct_singular 2 $rel]
    set rl [lindex $vr 0]
    set vl [lindex $vr 1]
    set fl [lindex $vr 2]
    puts stdout ""
    puts stdout "Unassignable relation [stripbraces [dbg_write_rel 2 $rel 0]]"
    puts stdout "is in the structurally singular group:"
    foreach i $rl {puts "<$i> [stripbraces [dbg_write_rel 2 $i 0]]"}
    if {[llength $vl] > 0} {
      puts stdout "This singularity involves variables:"
     foreach i $vl {puts stdout "<$i> [stripbraces [dbg_write_var 2 $i 0 0]]"}
    }
    if {[llength $fl] >0} {
      puts stdout "This singularity is reduced by freeing a variable from:"
     foreach i $fl {puts stdout "<$i> [stripbraces [dbg_write_var 2 $i 0 0]]"}
    }
  }
leavetrace
}

#
# proc Solve_Find_Near_Bounds {}
#----------------------------------------------------------------------------
# Near Bounds button.                                                       #
# if refresh then update status/presolve after fix                          #
#----------------------------------------------------------------------------
proc Solve_Find_Near_Bounds {} {
entertrace
  if {![slv_checksys]} {leavetrace; return}
  global ascSolv32767Vect
  set list ""
  catch {set list [lindex [slv_near_bounds $ascSolv32767Vect(nearbound) 2] 0]}
  set low_violations [lindex $list 0]
  set up_violations [lindex $list 1]
  set count 2
  set pname [slv_get_pathname]
  if {$low_violations == 0 && $up_violations == 0} {
      puts "\nNO VARS NEAR BOUNDS\n"
  }
  if {$low_violations > 0} {
      puts "\nVARIABLES NEAR LOWER BOUND:"
  }
  for {set j 0} {$j < $low_violations} {incr j} {
      set i  [lindex $list $count]
      puts "$pname.[stripbraces [dbg_write_var 2 $i 0 0]]"
      incr count
  }
  if {$up_violations > 0} {
      puts "\nVARIABLES NEAR UPPER BOUND:"
  }
  for {set j 0} {$j < $up_violations} {incr j} {
      set i  [lindex $list $count]
      puts "$pname.[stripbraces [dbg_write_var 2 $i 0 0]]"
      incr count
  }

leavetrace
}


#
# proc Solve_Find_Far_From_Nominals {}
#----------------------------------------------------------------------------
# Far From Nominals button.                                                 #
# if refresh then update status/presolve after fix                          #
#----------------------------------------------------------------------------
proc Solve_Find_Far_From_Nominals {} {
entertrace
  if {![slv_checksys]} {leavetrace; return}
  global ascSolv32767Vect
  set list ""
  catch {
    set list [lindex [slv_far_from_nominals $ascSolv32767Vect(farnom) 2] 0]
  }
  set pname [slv_get_pathname]
  set length [llength $list]
  if {$length == 0} {
      puts "\nNO VARS FAR FROM NOMINAL VALUE\n"
      return
  }
  puts "VARIABLES FAR FROM NOMINALS:"
  for {set j 0} {$j < $length} {incr j} {
      set i  [lindex $list $j]
      puts "$pname.[stripbraces [dbg_write_var 2 $i 0 0]]"
  }
leavetrace
}

#
# proc Solve_do_FindUnAssEqns {}
#----------------------------------------------------------------------------
# Find unassigned eqns Button. unincluded eqns don't count.                 #
#----------------------------------------------------------------------------
proc Solve_do_FindUnAssEqns {} {
entertrace
  if {![slv_checksys]} { leavetrace; return }
  global ascSolvVect
  puts stderr \
    "Included but unpartitioned relations in $ascSolvVect(instname):"
  set shlist [Solve_get_unassigned_rels]
  foreach i $shlist {
    dbg_write_rel 0 $i 2 $ascSolvVect(simname)
  }
  if {$shlist==""} {
    puts stdout "All included equations have been assigned."
  }
leavetrace
}

#
# proc Solve_do_EvalUnincluded {}
#----------------------------------------------------------------------------
# evaluate unincluded equations                                             #
#----------------------------------------------------------------------------
proc Solve_do_EvalUnincluded {} {
entertrace
  global ascSolvVect
  puts stderr "Unincluded relations in $ascSolvVect(instname):"
  foreach i [dbg_list_rels 1 not] {
    dbg_write_rel 0 $i 2 $ascSolvVect(simname)
  }
leavetrace
}

#
# proc Solve_do_Export2Browser {}
#----------------------------------------------------------------------------
# export instance to browser                                                #
#----------------------------------------------------------------------------
proc Solve_do_Export2Browser {} {
entertrace
  global ascSolvVect
  Brow_Export_Any_2Browser $ascSolvVect(instname)
leavetrace
}

#
# proc Solve_do_Export2Probe {}
#----------------------------------------------------------------------------
# export instance to probe                                                  #
#----------------------------------------------------------------------------
proc Solve_do_Export2Probe {} {
entertrace
  global ascSolvVect
  PROBE ALL $ascSolvVect(instname)
leavetrace
}

#
# proc Solve_do_Interrupt {}
#----------------------------------------------------------------------------
# interrupt button                                                          #
#----------------------------------------------------------------------------
proc Solve_do_Interrupt {} {
entertrace
  global ascSolvStatVect
  if { ! $ascSolvStatVect(empty) } {
    set ascSolvStatVect(menubreak) 1
    slv_set_haltflag 1
  }
leavetrace
}

#
# proc Solve_do_Help
#----------------------------------------------------------------------------
# help button                                                               #
#----------------------------------------------------------------------------
proc Solve_do_Help {} {
  Help_button solver
}

#
# proc Solve_do_Credit {solver}
#----------------------------------------------------------------------------
# credits button for solver codes                                           #
#----------------------------------------------------------------------------
proc Solve_do_Credit {solver} {
entertrace
  switch $solver {
    {slv} {Help_button solver.credits slv
           leavetrace; return}
    {minos} {Help_button solver.credits minos
           leavetrace; return}
    {opt _opt} {Help_button solver.credits opt
           leavetrace; return}
    {lsode} {Help_button solver.credits lsode
           leavetrace; return}
    {lsgrg} {Help_button solver.credits lsgrg
           leavetrace; return}
    {qrslv} {Help_button solver.credits qrslv
           leavetrace; return}
    {ngslv} {Help_button solver.credits ngslv
           leavetrace; return}
    {dslv} {Help_button solver.credits dslv
           leavetrace; return}
    {makemps} {Help_button solver.credits makemps
           leavetrace; return}
    {conopt} {Help_button solver.credits conopt
           leavetrace; return}
    default {puts "credits not yet implemented for $solver"}
  }
 # endswtich 
leavetrace
}

#
# proc Solve_do_Reanalyze {}
#----------------------------------------------------------------------------
# import qlfdid name, if eligible.                                          #
#  strip trailing . if needed                                               #
#----------------------------------------------------------------------------
proc Solve_do_Reanalyze {} {
entertrace
  global ascSolvStatVect ascSolvVect
  if { ! [slv_checksys] } { leavetrace; return 0 }
 # cant import from a null system
  set tname $ascSolvVect(instname)
  leavetrace; return [Solve_Import_Any $tname]
leavetrace
}

#
# proc Solve_do_Import {}
#----------------------------------------------------------------------------
# import qlfdid name, if eligible.                                          #
#  strip trailing . if needed                                               #
#----------------------------------------------------------------------------
proc Solve_do_Import {} {
entertrace
  global ascSolvStatVect ascSolvVect
  if { ! [slv_checksys] } {
    leavetrace
    return 0
  }
 # cant import from a null system
  set tname [string trim $ascSolvVect(pathname)]
  set tname [string trim $tname .]
  leavetrace; return [Solve_Import_Any $tname]
leavetrace
}

#
#----------------------------------------------------------------------------
# button internals section. includes parm page procedures, named as         #
# Solve_XXX_* where XXX is the 3 letter solver code in all caps.            #
#----------------------------------------------------------------------------


#
# proc Solve_EligListSelect {refresh}
#-------------------------------------------------------------------------
# fixes first of any variables selected. if refresh, updates screen      #
#-------------------------------------------------------------------------
proc Solve_EligListSelect {refresh} {
entertrace
  global ascListSelectBox
  set list $ascListSelectBox(itemselected)
  if {$list != ""} {
    set item [lindex $list 0]
    qassgn3 $item.fixed TRUE
    HUB_Message_to_HUB BOOLEANUPDATED $item
    if {$refresh} {
      slv_presolve
      Solve_Update_StatusBox
      update
      Solve_FindEligible 1
    }
  } else {
    Solve_Alert "Input Error" "Make selection or, choose Cancel"
    if {$refresh} {
      Solve_FindEligible 1
    }
  }
leavetrace
}

#
# proc Solve_GlobalEligListSelect {refresh}
#-------------------------------------------------------------------------
# fixes first of any variables selected. if refresh, updates screen      #
#-------------------------------------------------------------------------
proc Solve_GloablEligListSelect {refresh} {
entertrace
  global ascListSelectBox
  set list $ascListSelectBox(itemselected)
  if {$list != ""} {
    set item [lindex $list 0]
    qassgn3 $item.fixed TRUE
    HUB_Message_to_HUB BOOLEANUPDATED $item
    if {$refresh} {
      slv_presolve
      Solve_Update_StatusBox
      update
      Solve_FindGlobalEligible 1
    }
  } else {
    Solve_Alert "Input Error" "Make selection or, choose Cancel"
    if {$refresh} {
      Solve_FindGlobalEligible 1
    }
  }
leavetrace
}

#
# proc Solve_FixConsistentSet
#-------------------------------------------------------------------------
# Selection of the consistent set of varaibles. It fixes all of the 
# variables selected
#-------------------------------------------------------------------------
proc Solve_FixConsistentSet {} {
entertrace
  global ascListSelectB1Box
  set list $ascListSelectB1Box(itemselected)
  if {$list != ""} {
    foreach i $list {
      qassgn3  $i.fixed TRUE
    }
    set item  [lindex $list 0]
    HUB_Message_to_HUB BOOLEANUPDATED $item
    slv_presolve
    Solve_Update_StatusBox
    update
  } else {
    Solve_Alert "Input Error" "Make Selection or Choose Dismiss"
  }
  newraise .solvconsistent
leavetrace
}


#
# proc Solve_ConsistentListSelect {refresh}
#-------------------------------------------------------------------------
# fixes all of the variables selected. if refresh, updates screen
# It works with an SelectBox instead of a SelectB1Box (function of
# above )
#-------------------------------------------------------------------------
proc Solve_ConsistentListSelect {refresh} {
entertrace
  global ascListSelectBox
  set list $ascListSelectBox(itemselected)
  if {$list != ""} {

    set length [llength $list]
    if {$length == 0} {
      return
    }

    for {set j 0} {$j < $length} {incr j} {
      set item  [lindex $list $j]
      qassgn3 $item.fixed TRUE
    }

    set item  [lindex $list 0]
    HUB_Message_to_HUB BOOLEANUPDATED $item

    if {$refresh} {
      slv_presolve
      Solve_Update_StatusBox
      update
    }
  } else {
    Solve_Alert "Input Error" "Select all or Choose Cancel"
  }
leavetrace
}


#
# proc Solve_OverListSelect {refresh}
#-------------------------------------------------------------------------
# frees first of any variables selected. if refresh,updates screen.
#-------------------------------------------------------------------------
proc Solve_OverListSelect {refresh} {
entertrace
  global ascListSelectBox
  set list $ascListSelectBox(itemselected)
  if {$list != ""} {
    set item [lindex $list 0]
    qassgn3 $item.fixed FALSE
    HUB_Message_to_HUB BOOLEANUPDATED $item
    if {$refresh} {
      slv_presolve
      Solve_Update_StatusBox
      update
    }
    Solve_FindOverspec 1
  } else {
    Solve_Alert "Input Error" "Make selection or, choose Cancel"
    Solve_FindOverspec 1
  }
leavetrace
}

#
# proc Solve_GetObjeRelNum {name}
#-------------------------------------------------------------------------
# returns the solver list number of the objective which qlfdid 'name'    #
# returns -1 if name is "none" and -2 if name not on list                #
#-------------------------------------------------------------------------
proc Solve_GetObjRelNum {name} {
entertrace
  if {[string match "none" $name]} {
    leavetrace
    return -1
  }
  set list ""
  catch {set list [lindex [slv_get_obj_list 2] 0]}  
  set pname [slv_get_pathname]
  foreach i $list {
    if {[string match \
      "$pname.[stripbraces [dbg_write_obj 2 $i 0]]" \
      $name]} {
      leavetrace
      return $i
    }
  }
  return -2
leavetrace
}

#
# proc Solve_ObjListSelect {refresh}
#-------------------------------------------------------------------------
# sets solvers objective to the selected objective function              #
#-------------------------------------------------------------------------
proc Solve_ObjListSelect {refresh} {
entertrace
  global ascListSelectBox
  set list $ascListSelectBox(itemselected)
  if {$list != ""} {
    set item [lindex $list 0]
    set obj_num [Solve_GetObjRelNum $item]
    if {$obj_num == -2} {
      Solve_Alert "Unexpected Error in Solve_ObjListSelect" \
	      "choose Cancel and send bug report"
      if {$refresh} {
        Solve_Find_Objectives 1
      }
    } else {
      set cur_obj_num [slv_get_obj_num 2]
      if {$cur_obj_num != $obj_num} {
	catch {slv_set_obj_by_num $obj_num}
	# may be able to get away without a full presolve when switching objective
	# but just go with the full thing for now
	catch {slv_presolve}
      }
    }
  } else {
    Solve_Alert "Input Error" "Make selection or, choose Cancel"
    if {$refresh} {
      Solve_Find_Objectives 1
    }
  }
leavetrace
}


#
# proc Solve_get_unassigned_rels {}
#----------------------------------------------------------------------------
# leavetrace; return a list of indexes of all unassigned, but included,
# relations.
#----------------------------------------------------------------------------
proc Solve_get_unassigned_rels {} {
entertrace
  set rlist "[dbg_list_rels 4]"
  if {$rlist==""} {
    leavetrace
    return ""
  }
  # if nobody assigned, not an assigning solver.
  set rlist [dbg_list_rels 4 not]
  set uilist [dbg_list_rels 1 not]
  set shlist ""
  foreach i $rlist {
    if {[lsearch $uilist $i]=="-1"} {
      lappend shlist $i
    }
  }
leavetrace
  leavetrace; return $shlist
}

#
# proc Solve_Running {n1 n2 mode}
#----------------------------------------------------------------------------
# put the wings and colors on and off the shoes.
# ignores the arguments.
#----------------------------------------------------------------------------
proc Solve_Running {n1 n2 mode} {
entertrace
  global ascSolvStatVect
  if {$ascSolvStatVect(running)} {
    .solver.lbl_frm.lbl_run configure -bitmap wfeet -background green
    .solver.lbl_frm.lbl_int configure -background red
    .solver.lbl_frm.btn_int configure -background red
  } else {
    set color [.solver.lbl_frm cget -background]
    .solver.lbl_frm.lbl_run configure -bitmap feet -background $color 
    .solver.lbl_frm.lbl_int configure -background $color
    .solver.lbl_frm.btn_int configure -background $color
  }
leavetrace
}
#
# proc Solve_Import_Any {qlfdid}
#----------------------------------------------------------------------------
# load qlfdid into the solver, if possible.
# Not possible if:
# - type inappropriate (not model) or not safe (nulls,pendings found)
# - this function called during an interface update called from
#   within a non-iterable solver (a halt button check).
# - called with qlfdid==""
# Leavetrace; Returns 0 if succesful, 1 if not. If not, reason will be left in
# global variable ascSolvStatVect(importerror).
#
# Anyone trying to load the solver from anywhere besides here is likely
# to cause errors.
# The sanity this affords far outweighs the cost of the qlfdid search.
#----------------------------------------------------------------------------
proc Solve_Import_Any {qlfdid} {
entertrace
  global ascSolvVect ascSolvStatVect 

 # check running
  if {$ascSolvStatVect(running)=="1"} {
    puts stderr "Solve in progress."
    set sherrmsg "Solve in progress. Cannot import until done or halted."
    set halt  [asctk_dialog .solvhalt $ascSolvVect(font) \
               "Solver Alert:" $sherrmsg "" 1 OK Halt]
    if {$halt} {
      set ascSolvStatVect(menubreak) 1
      slv_set_haltflag 1
    }
    set ascSolvStatVect(importerror) "Solver is running."
    leavetrace; return 1
  }
 # check qlfdid
  if {$qlfdid==""} {
    Solve_do_Flush;
    set ascSolvStatVect(importerror) "Import called with empty argument."
    leavetrace; return 1
  }
  set ascSolvStatVect(menubreak) 0
  slv_set_haltflag 0
  set notok [slv_import_qlfdid $qlfdid test]
  if {$notok} {
    set ascSolvStatVect(importerror) "Instance not a complete model instance."
    leavetrace; return 1
  }
  # set simname
  set ascSolvVect(simname) [lindex [split $qlfdid .] 0]
  if {$ascSolvVect(visibility)} {
    newraise $ascSolvVect(windowname)
  }
  # import valid system: builds rel/var/obj lists
  slv_import_qlfdid $qlfdid
  set ascSolvVect(instname) "[slv_get_pathname]"
 # put last sets of used parameters into C vect
  Solve_Downdate_ParmBox
 # verify dimensionality
  Solve_Check_Dimensions
 # find appropriate solver
  Solve_Check_Eligible
 # get chosen solver. will be last one used if last one was eligible.
 # also presolves, sets status flags
  Solve_do_Select $ascSolvStatVect(solver)
 # over/under
  Solve_Check_DOF
  #
  # Update active flag of relations
  #
  Solve_FlagActive
  Solve_Update_StatusBox
 # redraw windows
  Solve_Update_Listbox
  Solve_Update_MenuBar
  catch {Solve_Update_Slaves}
  leavetrace; return 0
leavetrace
}


#
# proc Solve_Update_Slaves {}
#----------------------------------------------------------------------------
# update auxillary windows: debugger, mtx, debugger2
#----------------------------------------------------------------------------
proc Solve_Update_Slaves {} {
entertrace
  global ascSolvVect
  if {$ascSolvVect(debuggerup)} {
    Debug_Trace on
  }
  if {$ascSolvVect(mtxup)} {
    Solve_do_DispIncidence
  }
leavetrace
}
#
# proc Solve_Check_Block_Err {}
#----------------------------------------------------------------------------
# Compute the sum and max of block residuals.
# Compute the sum and max of block times.
# Compute the sum of function and jacobian times.
#----------------------------------------------------------------------------
proc Solve_Check_Block_Err {} {
entertrace
  global ascSolvStatVect ascSolv32767Vect
  set ct 0
  set itb 0
  set sumbe 0
  set sumft 0
  set sumjt 0
  set maxbe 0
  set maxbeblock 0
  set maxbt 0
  set maxbtblock 0
  set gscp [slv_get_cost_page]
  foreach i $gscp {
    if {[lindex $i 0] > 0} {
      set sumbe [expr $sumbe + [lindex $i 5]]
      set sumft [expr $sumft + [lindex $i 6]]
      set sumjt [expr $sumjt + [lindex $i 7]]
      if {[lindex $i 5] > $maxbe} {
        set maxbe [lindex $i 5]
        set maxbeblock $ct
      }
      if {[lindex $i 4] > $maxbt} {
        set maxbt [lindex $i 4]
        set maxbtblock $ct
      }
      incr ct
      if {[lindex $i 0] >1} {
        incr itb
      }
    }
  }
  set pst [llength $gscp]
  set pst [lindex [lindex $gscp [incr pst -1] ] 4]
  set ascSolvStatVect(maxblockerr) $maxbe
  set ascSolvStatVect(worstblock) $maxbeblock
  set ascSolvStatVect(sumblockerr) $sumbe
  if {$itb >0} {
    set ascSolvStatVect(avgblockerr) [expr $sumbe/$itb]
  } else {
    set ascSolvStatVect(avgblockerr) 0
  }
  if {$ascSolv32767Vect(showavgs)} {
    puts "Block error total: $sumbe"
    puts "Block error max($maxbeblock): $maxbe"
    puts "CPU total: $ascSolvStatVect(cpuelapsed)"
    puts "Expensive block($maxbtblock): $maxbt"
    puts "Presolve: $pst"
    puts "Functions: $sumft"
    puts "Derivatives: $sumjt"
  }
leavetrace
}

#
# proc Solve_Check_Dimensions {}
#----------------------------------------------------------------------------
# Check and derive dimensionality of rellist, varlist in solver.
# this should be a wrapper around something that the browser can
# also use.
#----------------------------------------------------------------------------
proc Solve_Check_Dimensions {} {
entertrace
leavetrace
}
#
# proc Solve_Check_Eligible
#----------------------------------------------------------------------------
# pick appropriate solver. will be last one used if last one was eligible.
# Changes ascSolvStatVect(solvernum) if necessary.
# If noone is eligible, returns SLV as solver; not necessarily a bright
# move, but better the devil you know.
# Should handle menu disabling on edit.selectsolver menu.
#----------------------------------------------------------------------------
proc Solve_Check_Eligible {} {
entertrace
  leavetrace; return
    global ascSolvVect
    set retval [asctk_dialog .solvinel $ascSolvVect(font) \
      "Solver diagnostic:" \
      "Current solver ($ascSolvStatVect(solver)) not eligible" "" 0 OK Why?]
    if {$retval} {
      puts stderr "Sorry, no help available on why yet."
    }

leavetrace
}

#
# proc Solve_Check_DOF {}
#----------------------------------------------------------------------------
# if DOF change needed pop up dialog.
#----------------------------------------------------------------------------
proc Solve_Check_DOF {} {
entertrace
  global ascSolvStatVect
  if {$ascSolvStatVect(overdefined)} {
    Solve_FindOverspec 1
    leavetrace
    return
  }
  if {"[string range $ascSolvStatVect(objval) 0 3]" != "none"} {
    leavetrace
    return
  }
  if {$ascSolvStatVect(underdefined)} {
    Solve_FindEligible 1
    leavetrace
    return}
  if {$ascSolvStatVect(structsingular)} {
    Solve_do_StrucDepends
    leavetrace
    return}
leavetrace
}

#
# proc Solve_Check_RowsPivoted {noisy}
#----------------------------------------------------------------------------
# look for numeric singularity of rows after solution.
# leavetrace; return 1 if checkable 0 if not inverted >=2 if singularity found
# number of singularities is leavetrace; return value -1
# if noisy is 1, pops up alertbox about uninvertedness
#----------------------------------------------------------------------------
proc Solve_Check_RowsPivoted {noisy} {
entertrace
    global ascSolvStatVect
    if {$ascSolvStatVect(running)} {
        puts stderr "Cannot check for singularity while solver is running."
        leavetrace
    return 1
    }
    puts stderr "Checking blocks for numeric row dependency:"
    # if QRSlv or NGSlv
    if {$ascSolvStatVect(solver) == "QRSlv"
	| $ascSolvStatVect(solver)== "NGSlv"} {
        set ret 0
        for {set blk 0} {$blk < $ascSolvStatVect(block.number)} {incr blk} {
            if {![catch {set dep [dbg_num_block_singular 2 $blk r]} msg]} {
                foreach r $dep {
                    incr ret
                    set eqn [lindex $r 0]
                    puts stderr \
                      "=== Found unpivoted relation $eqn in block $blk ==="
                    puts stderr "Which is the sum of:"
                    set eqns [lrange $r 1 end]
                    foreach e $eqns {
                        puts stderr "Relation ([lindex $e 0]) * [lindex $e 1]"
                    }
                    puts stderr "========================================="
                }
                incr ret
            } else {
                if {$noisy} {
                    global ascMsgVect ascSolvVect
                    set ascMsgVect(grab) 0
                    set ascMsgVect(title) "Linsol:"
                    set ascMsgVect(lbl_font) $ascSolvVect(font)
                    set ascMsgVect(btn_font) $ascSolvVect(font)
                    set ascMsgVect(btn_label) Dismiss
                    set ascMsgVect(toplevelname) .linsolmsg
                    set ascMsgVect(position) [setpos .solver 50 125]
                    ascMsgBox "Error in singularity\nchecking process."
                    raise .linsolmsg
                    puts stderr $msg
                }
            }
        }
        if {$noisy} {
	  puts stderr "All blocks checked."
	}
        leavetrace
    return $ret
    }
    #else non linsolqr system: print error
    if {$noisy} {
        global ascMsgVect ascSolvVect
        set ascMsgVect(grab) 0
        set ascMsgVect(title) "Numeric Depend:"
        set ascMsgVect(lbl_font) $ascSolvVect(font)
        set ascMsgVect(btn_font) $ascSolvVect(font)
        set ascMsgVect(btn_label) Dismiss
        set ascMsgVect(toplevelname) .linsolmsg
        set ascMsgVect(position) [setpos .solver 50 125]
        ascMsgBox "Must call Numeric Dependency Check from QRSlv."
        raise .linsolmsg
        puts "Error: no blocks checked\n"
    }
leavetrace

    leavetrace
    return 0
}



#
# proc Solve_FindEligible {refresh}
#----------------------------------------------------------------------------
# Find eligible vars button. nonincident don't count                        #
# if refresh then update status/presolve after fix                          #
#----------------------------------------------------------------------------
proc Solve_FindEligible {refresh} {
entertrace
  global ascListSelectBox ascSolvVect
  set list ""
  catch {set list [lindex [dbg_find_eligible 2] 0]}
  set ascListSelectBox(grab) 1
  set ascListSelectBox(btn3name) ""
  set ascListSelectBox(btn4name) ""
  set ascListSelectBox(btn5name) ""
  set ascListSelectBox(title) "Eligible"
  set ascListSelectBox(toplevelname) ".solveligible"
  set ascListSelectBox(font) $ascSolvVect(font)
  set ascListSelectBox(selectmode) browse
  set ascListSelectBox(headline) "Select to fix one of:"
  set newlist ""
  set pname [slv_get_pathname]
  foreach i $list {
    lappend newlist \
      "$pname.[stripbraces [dbg_write_var 2 $i 0 0]]"
  }
  if {$newlist=="" && !$refresh} {
    puts stderr "No variables eligible to be fixed."
    leavetrace
    return
  }
  set alist [lsort $newlist]
  set button [AscListSelectBox $alist \
                250x240[setpos .solver 50 20]]
  if {$button==2} {
    leavetrace
    return
  }
  Solve_EligListSelect $refresh
leavetrace
}



#
# proc Solve_FindGlobalEligible {refresh}
#----------------------------------------------------------------------------
# Find the set of "globally" (conditional model) eligible vars.
# if refresh then update status/presolve after fix 
#----------------------------------------------------------------------------
proc Solve_FindGlobalEligible {refresh} {
entertrace
  global ascListSelectBox ascSolvVect
  set list ""
  catch {set list [lindex [dbg_global_eligible 2] 0]}
  set ascListSelectBox(grab) 1
  set ascListSelectBox(btn3name) ""
  set ascListSelectBox(btn4name) ""
  set ascListSelectBox(btn5name) ""
  set ascListSelectBox(title) "Globally Eligible"
  set ascListSelectBox(toplevelname) ".solvgeligible"
  set ascListSelectBox(font) $ascSolvVect(font)
  set ascListSelectBox(selectmode) browse
  set ascListSelectBox(headline) "Select to fix one of:"
  set newlist ""
  set pname [slv_get_pathname]
  foreach i $list {
    lappend newlist \
      "$pname.[stripbraces [dbg_write_var 2 $i 0 1]]"
  }
  if {$newlist=="" && !$refresh} {
    puts stderr "No variables eligible to be fixed."
    leavetrace
    return
  }
  set alist [lsort $newlist]
  set button [AscListSelectBox $alist \
                250x240[setpos .solver 50 20]]
  if {$button==2} {
    leavetrace
    return
  }
  Solve_GlobalEligListSelect $refresh
leavetrace
}

#
# proc Solve_ConsistencyAnalysis {refresh}
#----------------------------------------------------------------------------
# Find the variables which makes a consitent set of decision variables,
# consistent in the "overall" conditional model
#----------------------------------------------------------------------------
proc Solve_ConsistencyAnalysis {refresh} {
entertrace
  global ascListSelectB1Box ascSolvVect
  set list ""
  catch {set list [lindex [dbg_consistency_analysis 2] 0]}
  set ascListSelectB1Box(grab) 0
  set ascListSelectB1Box(btn2name) "Tag All"
  set ascListSelectB1Box(btn3name) "Fix Selection"
  set ascListSelectB1Box(btn4name) ""
  set ascListSelectB1Box(btn5name) ""
  set ascListSelectB1Box(btn2destroy) 0
  set ascListSelectB1Box(btn3destroy) 0
  set ascListSelectB1Box(btn4destroy) 0
  set ascListSelectB1Box(btn2command) Solve_SelectAll
  set ascListSelectB1Box(btn3command) Solve_FixConsistentSet
  set ascListSelectB1Box(title) "Consistent Set"
  set ascListSelectB1Box(toplevelname) ".solvconsistent"
  set ascListSelectB1Box(selectmode) extended
  set ascListSelectBox(font) $ascSolvVect(font)
  set ascListSelectB1Box(headline) "Could Fix the Set:"
  set newlist ""
  set pname [slv_get_pathname]
  foreach i $list {
    lappend newlist \
      "$pname.[stripbraces [dbg_write_var 2 $i 0 1]]"
  }
  if {$newlist =="" && !$refresh} {
    puts stderr "No set of variables make a consistent partition."
    leavetrace
    return
  }

  set alist [lsort $newlist]
  set button [AscListSelectB1Box $alist \
                250x240[setpos .solver 50 20]]
leavetrace
}


#
# proc Solve_SelectAll {{tl ""}}
#-------------------------------------------------------------------------
# select all in the asclistselectb1box associate with tl, or
# select all in the last asclistselectb1box created if tl == ""
#-------------------------------------------------------------------------
proc Solve_SelectAll {{tl ""}} {
  AscListSelectB1SelectAll $tl
}

#
# proc Solve_ConsistencyAnalysis2 {refresh}
#----------------------------------------------------------------------------
# Find the variables which makes a consitent set of decision variables,
# consistent in the "overall" conditional model. It uses a SelectBox
# instead of a SelectB1Box (function of above)
#----------------------------------------------------------------------------
proc Solve_ConsistencyAnalysis2 {refresh} {
entertrace
  global ascListSelectBox ascSolvVect
  set list ""
  catch {set list [lindex [dbg_consistency_analysis 2] 0]}
  set ascListSelectBox(grab) 1
  set ascListSelectBox(btn3name) ""
  set ascListSelectBox(btn4name) ""
  set ascListSelectBox(btn5name) ""
  set ascListSelectBox(title) "Consistent Set"
  set ascListSelectBox(toplevelname) ".solvconsistent"
  set ascListSelectBox(font) $ascSolvVect(font)
  set ascListSelectBox(selectmode) browse
  set ascListSelectBox(headline) "Could Fix the Set:"
  set newlist ""
  set pname [slv_get_pathname]
  foreach i $list {
    lappend newlist \
      "$pname.[stripbraces [dbg_write_var 2 $i 0 1] ]"
  }
  if {$newlist =="" || !$refresh} {
    puts stderr "No set of variables make a consistent partition."
    leavetrace
    return
  }
  set alist [lsort $newlist]
  set button [AscListSelectBox $alist \
                250x240[setpos .solver 50 20]]
  if {$button==2} {
    leavetrace
    return
  }
  Solve_ConsistentListSelect $refresh
leavetrace
}

#
# proc Solve_FlagActive {}
#----------------------------------------------------------------------------
# Flag active relations instances.
#----------------------------------------------------------------------------
proc Solve_FlagActive {} {
entertrace

 dbg_find_activerels 0

leavetrace
}


#
# proc Solve_Find_Objectives {refresh}
#----------------------------------------------------------------------------
# Select objective button.                                                  #
# if refresh then update status/presolve after fix                          #
#----------------------------------------------------------------------------
proc Solve_Find_Objectives {refresh} {
entertrace
  global ascListSelectBox ascSolvVect
  set list ""
  catch {set list [lindex [slv_get_obj_list 2] 0]}
  set ascListSelectBox(grab) 1
  set ascListSelectBox(btn3name) ""
  set ascListSelectBox(btn4name) ""
  set ascListSelectBox(btn5name) ""
  set ascListSelectBox(title) "Objectives"
  set ascListSelectBox(toplevelname) ".solvobjectives"
  set ascListSelectBox(font) $ascSolvVect(font)
  set ascListSelectBox(selectmode) browse
  set ascListSelectBox(headline) "Select one objective:"
  set newlist ""
  set pname [slv_get_pathname]
  lappend newlist "none"
  foreach i $list {
    lappend newlist \
      "$pname.[stripbraces [dbg_write_obj 2 $i 0]]"
  }
  if {$newlist=="none" && !$refresh} {
    puts stderr "No included objectives."
    leavetrace
    return
  }
  set alist [lsort $newlist]
  set button [AscListSelectBox $alist \
                250x240[setpos .solver 50 20]]
  if {$button==2} {
    leavetrace
    return}
  Solve_ObjListSelect $refresh
leavetrace
}

#
# proc Solve_Update_ParmBox {args}
#----------------------------------------------------------------------------
# load parms from C structure for current solver/solve system. If no system,
# do nothing. if args not empty, load parms for system with name in args
#----------------------------------------------------------------------------
proc Solve_Update_ParmBox {args} {
entertrace
  global ascSolvStatVect ascSolvVect
  if {![slv_checksys]} {
    leavetrace
    return
  }
  if {$args!=""} {set name $args} {
    set name $ascSolvStatVect(solver)
  }
  if {$ascSolvVect($name) == -1} {
    leavtrace
    return
  }
  switch $name {
    {Slv} -
    {CSlv} { Solve_SLV_Update_ParmBox}
    {MINOS} { Solve_MINOS_Update_ParmBox}
    {Opt} { Solve_OPTSQP_Update_ParmBox}
    {QRSlv} { Solve_QRSLV_Update_ParmBox}
    {makeMPS} { Solve_MPS_Update_ParmBox}
    {NGSlv} { Solve_NGSLV_Update_ParmBox}
    {CONOPT} { Solve_CONOPT_Update_ParmBox}
    {General} { Solve_General_Update_ParmBox}
    default {
     puts "Don't know how to update solver $ascSolvStatVect(solver) parms."
    }
  }
leavetrace
}

#
# proc Solve_Downdate_ParmBox {args}
#----------------------------------------------------------------------------
# load parms to C structure for current solver/solve system. If no system,
# do nothing. if args not empty, downdate parms for system with name in args
#----------------------------------------------------------------------------
proc Solve_Downdate_ParmBox {args} {
entertrace
  global ascSolvStatVect ascSolvVect
  if {![slv_checksys]} {
    leavetrace
    return
  }
  if {$args!=""} {
    set name $args
  } {
    set name $ascSolvStatVect(solver)
  }
  if {$ascSolvVect($name) == -1} {
    leavtrace
    return
  }
  if {[catch Solve_${name}_Downdate_ParmBoxNew]} {
      puts "Don't know how to downdate solver $ascSolvStatVect(solver) parms."
  }
leavetrace
}

#
# proc Solve_Status_Infobox {}
#----------------------------------------------------------------------------
# solve status alertbox                                                     #
#----------------------------------------------------------------------------
proc Solve_Status_Infobox {} {
entertrace
  global ascSolvVect ascSolvStatVect ascMsgVect ascGlobalVect
  #
  # Update active flag of relations
  #
  Solve_FlagActive
  set ascMsgVect(lbl_font) $ascSolvVect(font)
  set ascMsgVect(btn_font) $ascSolvVect(font)
  set ascMsgVect(title) "Solve system status:"
  set ascMsgVect(btn_label) "Dismiss"
  set ascMsgVect(position) [setpos .solver 50 125]
  set ascMsgVect(grab) 0
  set ascMsgVect(toplevelname) .solverstatmsg
  set slist [join [split [string trim $ascSolvVect(status) ,] ,] \n]
  if {$ascSolvVect(statreport)} {
    ascMsgBox $slist
    raise .solverstatmsg
  } else {
    puts stdout $slist
  }
leavetrace
}

#
# proc Solve_Alert {title msg}
#----------------------------------------------------------------------------
# solve generic alertbox requires title msg                                 #
# msg may be multiline comma separated                                      #
#----------------------------------------------------------------------------
proc Solve_Alert {title msg} {
entertrace
  global ascSolvVect ascSolvStatVect ascMsgVect ascGlobalVect
  set ascMsgVect(lbl_font) $ascSolvVect(font)
  set ascMsgVect(btn_font) $ascSolvVect(font)
  set ascMsgVect(title) $title
  set ascMsgVect(btn_label) "Dismiss"
  set ascMsgVect(position) [setpos .solver 50 125]
  set ascMsgVect(grab) 0
  set ascMsgVect(toplevelname) .solveralertmsg
  set slist [join [split [string trim $msg ,] ,] \n]
  ascMsgBox $slist
  raise .solveralertmsg
leavetrace
}

#
# proc Solve_FP_error {msg}
#----------------------------------------------------------------------------
# solve floatingpoint error alertbox  and analysis.                         #
#----------------------------------------------------------------------------
proc Solve_FP_error {msg} {
entertrace
  global ascSolvVect ascSolvStatVect ascMsgVect ascGlobalVect
  set ascSolvStatVect(fpcaught) 1
  set ascMsgVect(lbl_font) $ascSolvVect(font)
  set ascMsgVect(btn_font) $ascSolvVect(font)
  set ascMsgVect(title) "Floating point error:"
  set ascMsgVect(btn_label) "Dismiss"
  set ascMsgVect(position) [setpos .solver 50 125]
  set ascMsgVect(grab) 0
  set ascMsgVect(toplevelname) .solvererr
  set slist [join [split [string trim $msg ,] ,] \n]
  ascMsgBox $slist
  raise .solvererr
  set elist [dbg_check_rels]
  puts "Examining relations for math errors:"
  foreach r $elist {
    set i [lindex $r 0]
    puts stderr "Relation <$i> [dbg_write_rel 2 $i 0] has math errors:"
    set ls [lindex $r 1]
    set rs [lindex $r 2]
    set dls [lindex $r 3]
    set drs [lindex $r 4]
    if {$ls} {
      puts stderr "   Error in the LHS"
    }
    if {$rs} {
      puts stderr "   Error in the RHS"
    }
    if {$ls==0 && $dls==1} {
      puts stderr "   Error in the LHS derivative."
    }
    if {$rs==0 && $drs==1} {
      puts stderr "   Error in the RHS derivative."
    }
  }
leavetrace
}

#
# proc Solve_Solve {}
#----------------------------------------------------------------------------
# Execute/Solve button internals                                            #
#----------------------------------------------------------------------------
proc Solve_Solve {} {
entertrace
  if {![slv_checksys]} { leavetrace; return }
# needs better sanity checking and interrupt checking.
  global ascSolvVect ascSolvStatVect ascSolv32767Vect ascSolv3Vect
  set ascSolvStatVect(menubreak) 0
  slv_set_haltflag 0
  if {[slv_checksys]} {
    set ascSolvStatVect(ready2solve) 1
  }
  Solve_Downdate_ParmBox
  if {[catch {slv_presolve} ]} {
    Solve_FP_error "Initial values,cause float error.,Please fix this."
    leavetrace
    return
  }
  HUB_Message_to_HUB SOLVINGSTARTED $ascSolvVect(instname) \
   [lindex [slv_available] [slv_get_solver]]
  Solve_Update_StatusBox
  if {!$ascSolvStatVect(calcok)} {
    Solve_FP_error "Initial values,cause range error.,Please fix this."
    leavetrace
    return
  }
  update
  update idletasks
  puts "$ascSolvVect(status)"
  while {$ascSolvStatVect(ready2solve) && !$ascSolvStatVect(menubreak)} {
    set ascSolvStatVect(running) 1
    Solve_Running 0 0 0
    update idletasks
    if {[catch {slv_iterate \
          $ascSolv32767Vect(update_frequency) \
          $ascSolv32767Vect(update_time)} ermsg]} {
      set  ascSolvStatVect(running) 0
      Solve_Running 0 0 0
      Solve_FP_error "Float error.,Check bounds & scaling."
    }
    Solve_update_monitor
 # temporary hack. get this slv3 reference out of here. baa 8/95
 # logically disabled 9/95 baa
#  set relnom ""
#  if {0 && $ascSolv3Vect(relnom) && [slv_get_solver] == 3} {
#    set relnom "[dbg_calc_relnoms]"
#  }
    if {0 && "$relnom" != ""} {
      Solve_FP_error \
        "Initial values,cause relation,scaling float error.,Please fix this."
      puts stderr "relindex lhsbad rhsbad nominal"
      foreach i $relnom {
        puts stderr $i
      }
   #  leavetrace
      return
    }
    set ascSolvStatVect(running) 0
    Solve_Running 0 0 0
    Solve_Update_StatusBox 1
    Solve_ClearQueue
    update
    if {!$ascSolvStatVect(calcok)} {
      HUB_Message_to_HUB SOLVINGDONE $ascSolvVect(simname)
      Solve_FP_error \
        "Range error in,functions or derivatives.,Check bounds & scaling."
      leavetrace
    return
    }
  }
  HUB_Message_to_HUB SOLVINGDONE $ascSolvVect(simname)
  if {$ascSolv32767Vect(checksing)} {
    Solve_Check_RowsPivoted 0
  }
leavetrace
}

#
# proc Solve_Optimize {}
#----------------------------------------------------------------------------
# Execute/Optimize button internals                                         #
#----------------------------------------------------------------------------
proc Solve_Optimize {} {
entertrace
# needs better sanity checking and interrupt checking.
  global ascSolvVect ascSolvStatVect ascSolv32767Vect

  set ascSolvStatVect(menubreak) 0
  slv_set_haltflag 0
  if {[slv_checksys]} {
    set ascSolvStatVect(ready2solve) 1
  }
 # something else here might be appropriate
 # if {[catch {slv_presolve} ]} {
 #   Solve_FP_error "Initial values,cause float error.,Please fix this."
 #   leavetrace
    return
 # }
  HUB_Message_to_HUB SOLVINGSTARTED $ascSolvVect(instname) \
   [lindex [slv_available] [slv_get_solver]]
  Solve_Update_StatusBox
  if {!$ascSolvStatVect(calcok)} {
    Solve_FP_error "Initial values,cause range error.,Please fix this."
    leavetrace
    return
  }
  update
  update idletasks
  puts "$ascSolvVect(status)"
  while {$ascSolvStatVect(ready2solve) && !$ascSolvStatVect(menubreak)} {
    set ascSolvStatVect(running) 1
    Solve_Running 0 0 0
    update idletasks
    if {[catch {slv_iterate \
          $ascSolv32767Vect(update_frequency) \
          $ascSolv32767Vect(update_time)} ermsg]} {
      set  ascSolvStatVect(running) 0
      Solve_Running 0 0 0
      Solve_FP_error "Float error.,Check bounds & scaling."
    }
    set ascSolvStatVect(running) 0
    Solve_Running 0 0 0
    Solve_Update_StatusBox 1
    Solve_ClearQueue
    update
    if {!$ascSolvStatVect(calcok)} {
      HUB_Message_to_HUB SOLVINGDONE $ascSolvVect(simname)
      Solve_FP_error \
        "Range error in,functions or derivatives.,Check bounds & scaling."
      leavetrace
    return
    }
  }
  HUB_Message_to_HUB SOLVINGDONE $ascSolvVect(simname)
  if {$ascSolv32767Vect(checksing)} {
    Solve_Check_RowsPivoted 0
  }
leavetrace
}

#
# proc Solve_Integrate {}
#----------------------------------------------------------------------------
# Execute/Integrate button internals                                         #
#----------------------------------------------------------------------------
proc Solve_Integrate {args} {
entertrace
# needs better sanity checking and interrupt checking.
  global ascSolvVect ascSolvStatVect ascSolv32767Vect

  set ascSolvStatVect(menubreak) 0
  if {[slv_checksys]} {
    set ascSolvStatVect(ready2solve) 1
  }
  if {[catch {slv_presolve} ]} {
    Solve_FP_error "Initial values,cause float error.,Please fix this."
    leavetrace
    return
  }
  HUB_Message_to_HUB SOLVINGSTARTED $ascSolvVect(instname) \
   [lindex [slv_available] [slv_get_solver]]
  Solve_Update_StatusBox
  if {!$ascSolvStatVect(calcok)} {
    Solve_FP_error "Initial values,cause range error.,Please fix this."
    leavetrace
    return
  }
  set n1 0
  set n2 0
  catch {integrate_logunits "$ascSolv32767Vect(logsi)"}
  catch {integrate_logformat "$ascSolv32767Vect(logcol)"}
  if {$ascSolv32767Vect(newlog)} {
    catch {file delete $ascSolv32767Vect(yfilename) \
                $ascSolv32767Vect(obsfilename)
          } fileerr
  }
  integrate_set_y_file "$ascSolv32767Vect(yfilename)"
  integrate_set_obs_file "$ascSolv32767Vect(obsfilename)"
  if {$args != ""} {
    if {[llength $args] !=3} {
      error "Solve integrate called with incorrect args type i1 i2"
    }
    set itype "[lindex $args 0]"
    set n1 [lindex $args 1]
    if {$n1 == "first"} {
      set n1 0
    }
    set n2 [lindex $args 2]
    if {$n2 == "last"} {
      if {"[string tolower $itype]"=="lsode"} {
        set n2 [lindex [lindex [u_getval [slv_get_pathname].nstep] 0] 0]
      } else {
        if {[catch {set n2 [llength [lindex [integrate_get_samples] 1]]} ]} {
          set n2 -1
        } else {
          incr n2 -1
        }
      }
    }
  }
  update
  update idletasks
  puts "$ascSolvVect(status)"
  set ascSolvStatVect(running) 1
  Solve_Running 0 0 0
  update idletasks
  if {$args == ""} {
    set integ_time [asc_clock "integrate_setup -engine BLSODE \
                     -dt0 $ascSolv32767Vect(dtzero) \
                     -dtmin $ascSolv32767Vect(dtmin) \
                     -dtmax $ascSolv32767Vect(dtmax) \
                     -moststeps $ascSolv32767Vect(moststeps)"]
  } else {
    set integ_time \
      [asc_clock "integrate_setup -engine $itype -i0 $n1 -i1 $n2 \
                     -dt0 $ascSolv32767Vect(dtzero) \
                     -dtmin $ascSolv32767Vect(dtmin) \
                     -dtmax $ascSolv32767Vect(dtmax) \
                     -moststeps $ascSolv32767Vect(moststeps)"]
  }
  integrate_cleanup
  puts "Time for integration: $integ_time"
  set  ascSolvStatVect(running) 0
  Solve_Running 0 0 0
  Solve_Update_StatusBox
  Solve_ClearQueue
  update
    if {!$ascSolvStatVect(calcok)} {
      HUB_Message_to_HUB SOLVINGDONE $ascSolvVect(simname)
      Solve_FP_error \
        "Range error in,functions or derivatives.,Check bounds & scaling."
      leavetrace
    return
    }
  HUB_Message_to_HUB SOLVINGDONE $ascSolvVect(simname)
  if {$ascSolv32767Vect(checksing)} {
    Solve_Check_RowsPivoted 0
  }
leavetrace
}


#
# proc Solve_Iterate {}
#----------------------------------------------------------------------------
# Execute/Iterate button internals does one of karls iterations.            #
#----------------------------------------------------------------------------
proc Solve_Iterate {} {
entertrace
  global ascSolvVect ascSolvStatVect
  set ascSolvStatVect(menubreak) 0
  slv_set_haltflag 0
  Solve_Update_StatusBox
  set ascSolvStatVect(running) 1
  Solve_Running 0 0 0
  update idletasks
  slv_iterate 1
  set ascSolvStatVect(running) 0
  Solve_Running 0 0 0
  Solve_update_monitor
  Solve_Update_StatusBox
  update idletasks
  HUB_Message_to_HUB SOLVINGDONE $ascSolvVect(simname)
leavetrace
}

#
# proc Solve_General_Update_ParmBox {}
#----------------------------------------------------------------------------
# this  updates ascSolv32767Vect variables                                  #
#----------------------------------------------------------------------------
proc Solve_General_Update_ParmBox {} {
entertrace
  global ascSolv32767Vect
  set ascSolv32767Vect(lnmepsilon) [slv_lnmget]
leavetrace
}
#
# proc Solve_General_Downdate_ParmBox {}
#----------------------------------------------------------------------------
# this  downdates ascSolv32767Vect variables                                #
#----------------------------------------------------------------------------
proc Solve_General_Downdate_ParmBox {} {
entertrace
  catch {slv_lnmset $ascSolv32767Vect(lnmepsilon)}
leavetrace
}
#
# proc Solve_SLV_Update_ParmBox {}
#----------------------------------------------------------------------------
# this  updates ascSolv0Vect variables from the C structure                 #
#----------------------------------------------------------------------------
proc Solve_SLV_Update_ParmBox {} {
entertrace
  global ascSolv0Vect
  set tmplist [slv_get_parms 0]
  if { [llength $tmplist]==17 && [lindex $tmplist 0]==0} {
    set ascSolv0Vect(timelimit)           [lindex $tmplist 1]
    set ascSolv0Vect(iterationlimit)      [lindex $tmplist 2]
    set ascSolv0Vect(termtol)             [lindex $tmplist 3]
    set ascSolv0Vect(feastol)             [lindex $tmplist 4]
    set ascSolv0Vect(pivottol)            [lindex $tmplist 5]
    set ascSolv0Vect(singtol)             [lindex $tmplist 6]
    set ascSolv0Vect(stattol)             [lindex $tmplist 7]
    set ascSolv0Vect(rho)                 [lindex $tmplist 8]
    set ascSolv0Vect(partition)           [lindex $tmplist 9]
    set ascSolv0Vect(ignorebounds)        [lindex $tmplist 10]
    set ascSolv0Vect(showmoreimportant)   [lindex $tmplist 11]
    set ascSolv0Vect(showlessimportant)   [lindex $tmplist 12]
    set ascSolv0Vect(bppivoting)          [lindex $tmplist 13]
    set ascSolv0Vect(showlessimportantds) [lindex $tmplist 14]
    set ascSolv0Vect(savlin)              [lindex $tmplist 15]
  } else {
    puts "Error in Slv call to slv_get_parms"
  }
leavetrace
}
#
# proc Solve_QRSLV_Update_ParmBox {}
#----------------------------------------------------------------------------
# this  updates ascSolv3Vect variables from the C structure                 #
#----------------------------------------------------------------------------
proc Solve_QRSLV_Update_ParmBox {} {
entertrace
  global ascSolv3Vect
  global ascSolvVect
  set registered_number $ascSolvVect(QRSlv)

  if { $registered_number < 0 } {
    leavetrace
    return
  }

  set tmplist [slv_get_parms $registered_number]
  if { [llength $tmplist]==41 && [lindex $tmplist 0]==$registered_number} {
    set ascSolv3Vect(timelimit)           [lindex $tmplist 1]
    set ascSolv3Vect(iterationlimit)      [lindex $tmplist 2]
    set ascSolv3Vect(termtol)             [lindex $tmplist 3]
    set ascSolv3Vect(feastol)             [lindex $tmplist 4]
    set ascSolv3Vect(pivottol)            [lindex $tmplist 5]
    set ascSolv3Vect(singtol)             [lindex $tmplist 6]
    set ascSolv3Vect(stattol)             [lindex $tmplist 7]
    set ascSolv3Vect(rho)                 [lindex $tmplist 8]
    set ascSolv3Vect(partition)           [lindex $tmplist 9]
    set ascSolv3Vect(ignorebounds)        [lindex $tmplist 10]
    set ascSolv3Vect(showmoreimportant)   [lindex $tmplist 11]
    set ascSolv3Vect(showlessimportant)   [lindex $tmplist 12]
    set ascSolv3Vect(bppivoting) \
      [Solve_QRSLV_int_to_bppivoting      [lindex $tmplist 13]]
    set ascSolv3Vect(lifds)               [lindex $tmplist 14]
    set ascSolv3Vect(savlin)              [lindex $tmplist 15]
    set ascSolv3Vect(relnomscale)         [lindex $tmplist 16]
    set ascSolv3Vect(cutoff)              [lindex $tmplist 17]
    set ascSolv3Vect(upjac)               [lindex $tmplist 18]
    set ascSolv3Vect(upwts)               [lindex $tmplist 19]
    set ascSolv3Vect(upnom)               [lindex $tmplist 20]
    set ascSolv3Vect(reduce)              [lindex $tmplist 21]
    set ascSolv3Vect(exact)               [lindex $tmplist 22]
    set ascSolv3Vect(cncols)              [lindex $tmplist 23]
    set ascSolv3Vect(btrunc)              [lindex $tmplist 24]
    set ascSolv3Vect(reorder)             [lindex $tmplist 25]
    set ascSolv3Vect(safe_calc)           [lindex $tmplist 26]
    set ascSolv3Vect(uprelnom)            [lindex $tmplist 27]
    set ascSolv3Vect(itscalelim)          [lindex $tmplist 28]
    set ascSolv3Vect(scaleopt)            [lindex $tmplist 29]
    set ascSolv3Vect(toosmall)            [lindex $tmplist 30]
    set ascSolv3Vect(cnlow)               [lindex $tmplist 31]
    set ascSolv3Vect(cnhigh)              [lindex $tmplist 32]
    set ascSolv3Vect(tobnds)              [lindex $tmplist 33]
    set ascSolv3Vect(posdef)              [lindex $tmplist 34]
    set ascSolv3Vect(detzero)             [lindex $tmplist 35]
    set ascSolv3Vect(steperrmax)          [lindex $tmplist 36]
    set ascSolv3Vect(prngmin)             [lindex $tmplist 37]
    set ascSolv3Vect(mincoef)             [lindex $tmplist 38]
    set ascSolv3Vect(maxcoef)             [lindex $tmplist 39]
    set ascSolv3Vect(itscaletol)          [lindex $tmplist 40]
  } else {
    puts "Error IN QRSlv call to slv_get_parms"
  }
leavetrace
}
# proc Solve_NGSLV_Update_ParmBox {}
#----------------------------------------------------------------------------
# this  updates ascSolv7Vect variables from the C structure                 #
#----------------------------------------------------------------------------
proc Solve_NGSLV_Update_ParmBox {} {
entertrace
  global ascSolv7Vect
  global ascSolvVect
  set registered_number $ascSolvVect(NGSlv)

  if { $registered_number < 0 }  {
    leavetrace
    return
  }

  set tmplist [slv_get_parms $registered_number]
  if { [llength $tmplist]==38 && [lindex $tmplist 0]==$registered_number} {
    set ascSolv7Vect(timelimit)           [lindex $tmplist 1]
    set ascSolv7Vect(iterationlimit)      [lindex $tmplist 2]
    set ascSolv7Vect(termtol)             [lindex $tmplist 3]
    set ascSolv7Vect(feastol)             [lindex $tmplist 4]
    set ascSolv7Vect(pivottol)            [lindex $tmplist 5]
    set ascSolv7Vect(singtol)             [lindex $tmplist 6]
    set ascSolv7Vect(stattol)             [lindex $tmplist 7]
    set ascSolv7Vect(rho)                 [lindex $tmplist 8]
    set ascSolv7Vect(partition)           [lindex $tmplist 9]
    set ascSolv7Vect(ignorebounds)        [lindex $tmplist 10]
    set ascSolv7Vect(showmoreimportant)   [lindex $tmplist 11]
    set ascSolv7Vect(showlessimportant)   [lindex $tmplist 12]
    set ascSolv7Vect(bppivoting) \
      [Solve_NGSLV_int_to_bppivoting      [lindex $tmplist 13]]
    set ascSolv7Vect(lifds)               [lindex $tmplist 14]
    set ascSolv7Vect(savlin)              [lindex $tmplist 15]
    set ascSolv7Vect(relnom)              [lindex $tmplist 16]
    set ascSolv7Vect(cutoff)              [lindex $tmplist 17]
    set ascSolv7Vect(upjac)               [lindex $tmplist 18]
    set ascSolv7Vect(upwts)               [lindex $tmplist 19]
    set ascSolv7Vect(upnom)               [lindex $tmplist 20]
    set ascSolv7Vect(reduce)              [lindex $tmplist 21]
    set ascSolv7Vect(exact)               [lindex $tmplist 22]
    set ascSolv7Vect(cncols)              [lindex $tmplist 23]
    set ascSolv7Vect(btrunc)              [lindex $tmplist 24]
    set ascSolv7Vect(reorder)             [lindex $tmplist 25]
    set ascSolv7Vect(safe_calc)           [lindex $tmplist 26]
    set ascSolv7Vect(toosmall)            [lindex $tmplist 27]
    set ascSolv7Vect(cnlow)               [lindex $tmplist 28]
    set ascSolv7Vect(cnhigh)              [lindex $tmplist 29]
    set ascSolv7Vect(tobnds)              [lindex $tmplist 30]
    set ascSolv7Vect(posdef)              [lindex $tmplist 31]
    set ascSolv7Vect(detzero)             [lindex $tmplist 32]
    set ascSolv7Vect(steperrmax)          [lindex $tmplist 33]
    set ascSolv7Vect(prngmin)             [lindex $tmplist 34]
    set ascSolv7Vect(mincoef)             [lindex $tmplist 35]
    set ascSolv7Vect(maxcoef)             [lindex $tmplist 36]
    set ascSolv7Vect(gradmult)            [lindex $tmplist 37]
  } else {
    puts "Error IN NGSlv call to slv_get_parms"
  }
leavetrace
}


#
# proc Solve_CONOPT_Update_ParmBox {}
#----------------------------------------------------------------------------
# this  updates ascSolv8Vect variables from the C structure                 #
#----------------------------------------------------------------------------
proc Solve_CONOPT_Update_ParmBox {} {
entertrace
  global ascSolv8Vect
  global ascSolvVect
  set registered_number $ascSolvVect(CONOPT)

  if { $registered_number < 0 } {
    leavetrace
    return
  }

  set tmplist [slv_get_parms $registered_number]
  if { [llength $tmplist]==41 && [lindex $tmplist 0]==$registered_number} {
    set ascSolv8Vect(timelimit)           [lindex $tmplist 1]
    set ascSolv8Vect(iterationlimit)      [lindex $tmplist 2]
    set ascSolv8Vect(termtol)             [lindex $tmplist 3]
    set ascSolv8Vect(feastol)             [lindex $tmplist 4]
    set ascSolv8Vect(pivottol)            [lindex $tmplist 5]
    set ascSolv8Vect(singtol)             [lindex $tmplist 6]
    set ascSolv8Vect(stattol)             [lindex $tmplist 7]
    set ascSolv8Vect(rho)                 [lindex $tmplist 8]
    set ascSolv8Vect(partition)           [lindex $tmplist 9]
    set ascSolv8Vect(ignorebounds)        [lindex $tmplist 10]
    set ascSolv8Vect(showmoreimportant)   [lindex $tmplist 11]
    set ascSolv8Vect(showlessimportant)   [lindex $tmplist 12]
    set ascSolv8Vect(bppivoting) \
      [Solve_CONOPT_int_to_bppivoting      [lindex $tmplist 13]]
    set ascSolv8Vect(lifds)               [lindex $tmplist 14]
    set ascSolv8Vect(savlin)              [lindex $tmplist 15]
    set ascSolv8Vect(relnomscale)         [lindex $tmplist 16]
    set ascSolv8Vect(cutoff)              [lindex $tmplist 17]
    set ascSolv8Vect(upjac)               [lindex $tmplist 18]
    set ascSolv8Vect(upwts)               [lindex $tmplist 19]
    set ascSolv8Vect(upnom)               [lindex $tmplist 20]
    set ascSolv8Vect(reduce)              [lindex $tmplist 21]
    set ascSolv8Vect(exact)               [lindex $tmplist 22]
    set ascSolv8Vect(cncols)              [lindex $tmplist 23]
    set ascSolv8Vect(btrunc)              [lindex $tmplist 24]
    set ascSolv8Vect(reorder)             [lindex $tmplist 25]
    set ascSolv8Vect(safe_calc)           [lindex $tmplist 26]
    set ascSolv8Vect(uprelnom)            [lindex $tmplist 27]
    set ascSolv8Vect(itscalelim)          [lindex $tmplist 28]
    set ascSolv8Vect(scaleopt)            [lindex $tmplist 29]
    set ascSolv8Vect(toosmall)            [lindex $tmplist 30]
    set ascSolv8Vect(cnlow)               [lindex $tmplist 31]
    set ascSolv8Vect(cnhigh)              [lindex $tmplist 32]
    set ascSolv8Vect(tobnds)              [lindex $tmplist 33]
    set ascSolv8Vect(posdef)              [lindex $tmplist 34]
    set ascSolv8Vect(detzero)             [lindex $tmplist 35]
    set ascSolv8Vect(steperrmax)          [lindex $tmplist 36]
    set ascSolv8Vect(prngmin)             [lindex $tmplist 37]
    set ascSolv8Vect(mincoef)             [lindex $tmplist 38]
    set ascSolv8Vect(maxcoef)             [lindex $tmplist 39]
    set ascSolv8Vect(itscaletol)          [lindex $tmplist 40]
  } else {
    puts "Error IN CONOPT call to slv_get_parms"
  }
leavetrace
}

#
# proc Solve_LSSLV_Update_ParmBox {}
#----------------------------------------------------------------------------
# this  updates ascSolv5Vect variables from the C structure                 #
#----------------------------------------------------------------------------
proc Solve_LSSLV_Update_ParmBox {} {
entertrace
  global ascSolv5Vect
  set tmplist [slv_get_parms 5]
  if { [llength $tmplist]==16 && [lindex $tmplist 0]==5} {
    set ascSolv5Vect(timelimit)           [lindex $tmplist 1]
    set ascSolv5Vect(iterationlimit)      [lindex $tmplist 2]
    set ascSolv5Vect(termtol)             [lindex $tmplist 3]
    set ascSolv5Vect(feastol)             [lindex $tmplist 4]
    set ascSolv5Vect(pivottol)            [lindex $tmplist 5]
    set ascSolv5Vect(singtol)             [lindex $tmplist 6]
    set ascSolv5Vect(stattol)             [lindex $tmplist 7]
    set ascSolv5Vect(rho)                 [lindex $tmplist 8]
    set ascSolv5Vect(partition)           [lindex $tmplist 9]
    set ascSolv5Vect(ignorebounds)        [lindex $tmplist 10]
    set ascSolv5Vect(showmoreimportant)   [lindex $tmplist 11]
    set ascSolv5Vect(showlessimportant)   [lindex $tmplist 12]
    set ascSolv5Vect(bppivoting)          [lindex $tmplist 13]
    set ascSolv5Vect(showlessimportantds) [lindex $tmplist 14]
    set ascSolv5Vect(savlin)              [lindex $tmplist 15]
  } else {
    puts "Error IN LSSlv call to slv_get_parms"
  }
leavetrace
}

#
# proc Solve_SLV_Downdate_ParmBox {}
#----------------------------------------------------------------------------
# this updates C structure from the ascSolv0Vect                            #
# hacked for slv4 at the moment
#----------------------------------------------------------------------------
proc Solve_SLV_Downdate_ParmBox {} {
entertrace
  global  ascSolv0Vect
  set_slv_parms 0 \
    $ascSolv0Vect(timelimit)         \
    $ascSolv0Vect(iterationlimit)    \
    $ascSolv0Vect(termtol)           \
    $ascSolv0Vect(feastol)           \
    $ascSolv0Vect(pivottol)          \
    $ascSolv0Vect(singtol)           \
    $ascSolv0Vect(stattol)           \
    $ascSolv0Vect(rho)               \
    $ascSolv0Vect(partition)         \
    $ascSolv0Vect(ignorebounds)      \
    $ascSolv0Vect(showmoreimportant) \
    $ascSolv0Vect(showlessimportant) \
    $ascSolv0Vect(bppivoting)        \
    $ascSolv0Vect(showlessimportantds) \
    $ascSolv0Vect(savlin)
#  set_slv_parms 4 \
#    $ascSolv0Vect(timelimit)         \
#    $ascSolv0Vect(iterationlimit)    \
#    $ascSolv0Vect(termtol)           \
#    $ascSolv0Vect(feastol)           \
#    $ascSolv0Vect(pivottol)          \
#    $ascSolv0Vect(singtol)           \
#    $ascSolv0Vect(stattol)           \
#    $ascSolv0Vect(rho)               \
#    $ascSolv0Vect(partition)         \
#    $ascSolv0Vect(ignorebounds)      \
#    $ascSolv0Vect(showmoreimportant) \
#    $ascSolv0Vect(showlessimportant) \
#    $ascSolv0Vect(showlessimportantds) \
#    $ascSolv0Vect(savlin)
leavetrace
}

#
# proc Solve_QRSLV_bppivoting_to_int {choice}
#----------------------------------------------------------------------------
# this is a total hack. fix it, probably with an appropriate C primitive.
# assumes the 4 methods of interest are in choices 0-3 and slv3 indexes
# its fmethod choice the same way.
#----------------------------------------------------------------------------
proc Solve_QRSLV_bppivoting_to_int {choice} {
entertrace
  global ascSolv3Vect
  set i [lsearch -exact $ascSolv3Vect(bppivoting.choices) $choice]
  if {$i == -1 || $i >4} {
    leavetrace
    return 4
  }
  leavetrace
  return $i
leavetrace
}

#
# proc Solve_QRSLV_int_to_bppivoting {i}
#----------------------------------------------------------------------------
# this is a total hack. fix it, probably with an appropriate C primitive.
# assumes the 4 methods of interest are in choices 0-3 and slv3 indexes
# its fmethod choice the same way.
#----------------------------------------------------------------------------
proc Solve_QRSLV_int_to_bppivoting {i} {
entertrace
  global ascSolv3Vect
  if {$i <0 || $i >4} {
    leavetrace
    return "Fast-SPK1/RANKI+ROW"
  }
  set choice "[lindex $ascSolv3Vect(bppivoting.choices) $i]"
  leavetrace
  return $choice
leavetrace
}

#
# proc Solve_QRSLV_Downdate_ParmBox {}
#----------------------------------------------------------------------------
# this updates C structure from the ascSolv3Vect                            #
#----------------------------------------------------------------------------
proc Solve_QRSLV_Downdate_ParmBox {} {
entertrace
  global  ascSolv3Vect ascSolvVect

  if {$ascSolvVect(QRSlv) == -1} {
    leavetrace
    return
  }
  set_slv_parms $ascSolvVect(QRSlv) \
    $ascSolv3Vect(timelimit)         \
    $ascSolv3Vect(iterationlimit)    \
    $ascSolv3Vect(termtol)           \
    $ascSolv3Vect(feastol)           \
    $ascSolv3Vect(pivottol)          \
    $ascSolv3Vect(singtol)           \
    $ascSolv3Vect(stattol)           \
    $ascSolv3Vect(rho)               \
    $ascSolv3Vect(partition)         \
    $ascSolv3Vect(ignorebounds)      \
    $ascSolv3Vect(showmoreimportant) \
    $ascSolv3Vect(showlessimportant) \
    "[Solve_QRSLV_bppivoting_to_int $ascSolv3Vect(bppivoting)]" \
    $ascSolv3Vect(lifds) \
    $ascSolv3Vect(savlin) \
    $ascSolv3Vect(relnomscale) \
    $ascSolv3Vect(cutoff) \
    $ascSolv3Vect(upjac) \
    $ascSolv3Vect(upwts) \
    $ascSolv3Vect(upnom) \
    $ascSolv3Vect(reduce) \
    $ascSolv3Vect(exact) \
    $ascSolv3Vect(cncols) \
    $ascSolv3Vect(btrunc) \
    $ascSolv3Vect(reorder) \
    $ascSolv3Vect(safe_calc) \
    $ascSolv3Vect(uprelnom) \
    $ascSolv3Vect(itscalelim) \
    $ascSolv3Vect(scaleopt) \
    $ascSolv3Vect(toosmall) \
    $ascSolv3Vect(cnlow) \
    $ascSolv3Vect(cnhigh) \
    $ascSolv3Vect(tobnds) \
    $ascSolv3Vect(posdef) \
    $ascSolv3Vect(detzero) \
    $ascSolv3Vect(steperrmax) \
    $ascSolv3Vect(prngmin) \
    $ascSolv3Vect(mincoef) \
    $ascSolv3Vect(maxcoef) \
    $ascSolv3Vect(itscaletol)
leavetrace
}

#
# proc Solve_NGSLV_bppivoting_to_int {choice}
#----------------------------------------------------------------------------
# this is a total hack. fix it, probably with an appropriate C primitive.
# assumes the 4 methods of interest are in choices 0-3 and slv7 indexes
# its fmethod choice the same way.
#----------------------------------------------------------------------------
proc Solve_NGSLV_bppivoting_to_int {choice} {
entertrace
  global ascSolv7Vect
  set i [lsearch -exact $ascSolv7Vect(bppivoting.choices) $choice]
  if {$i == -1 || $i >3} {
    leavetrace
    return 2
  }
#  leavetrace; return $i
  leavetrace
  return 2
leavetrace
}

#
# proc Solve_NGSLV_int_to_bppivoting {i}
#----------------------------------------------------------------------------
# this is a total hack. fix it, probably with an appropriate C primitive.
# assumes the 4 methods of interest are in choices 0-3 and slv7 indexes
# its fmethod choice the same way.
#----------------------------------------------------------------------------
proc Solve_NGSLV_int_to_bppivoting {i} {
entertrace
  global ascSolv7Vect
  if {$i <0 || $i >3} {
    leavetrace
    return "SPK1/RANKI+COL"
  }
  set choice "[lindex $ascSolv7Vect(bppivoting.choices) $i]"
  leavetrace
  return $choice
leavetrace
}

#
# proc Solve_NGSLV_Downdate_ParmBox {}
#----------------------------------------------------------------------------
# this updates C structure from the ascSolv7Vect                            #
#----------------------------------------------------------------------------
proc Solve_NGSLV_Downdate_ParmBox {} {
entertrace
  global  ascSolv7Vect ascSolvVect
  if {$ascSolvVect(NGSlv) == -1} {
    leavetrace
    return
  }
  set_slv_parms  $ascSolvVect(NGSlv)\
    $ascSolv7Vect(timelimit)         \
    $ascSolv7Vect(iterationlimit)    \
    $ascSolv7Vect(termtol)           \
    $ascSolv7Vect(feastol)           \
    $ascSolv7Vect(pivottol)          \
    $ascSolv7Vect(singtol)           \
    $ascSolv7Vect(stattol)           \
    $ascSolv7Vect(rho)               \
    $ascSolv7Vect(partition)         \
    $ascSolv7Vect(ignorebounds)      \
    $ascSolv7Vect(showmoreimportant) \
    $ascSolv7Vect(showlessimportant) \
    "[Solve_NGSLV_bppivoting_to_int $ascSolv7Vect(bppivoting)]" \
    $ascSolv7Vect(lifds) \
    $ascSolv7Vect(savlin) \
    $ascSolv7Vect(relnom) \
    $ascSolv7Vect(cutoff) \
    $ascSolv7Vect(upjac) \
    $ascSolv7Vect(upwts) \
    $ascSolv7Vect(upnom) \
    $ascSolv7Vect(reduce) \
    $ascSolv7Vect(exact) \
    $ascSolv7Vect(cncols) \
    $ascSolv7Vect(btrunc) \
    $ascSolv7Vect(reorder) \
    $ascSolv7Vect(safe_calc) \
    $ascSolv7Vect(toosmall) \
    $ascSolv7Vect(cnlow) \
    $ascSolv7Vect(cnhigh) \
    $ascSolv7Vect(tobnds) \
    $ascSolv7Vect(posdef) \
    $ascSolv7Vect(detzero) \
    $ascSolv7Vect(steperrmax) \
    $ascSolv7Vect(prngmin) \
    $ascSolv7Vect(mincoef) \
    $ascSolv7Vect(maxcoef) \
    $ascSolv7Vect(gradmult)
leavetrace
}


#
# proc Solve_CONOPT_bppivoting_to_int {choice}
#----------------------------------------------------------------------------
# this is a total hack. fix it, probably with an appropriate C primitive.
# assumes the 4 methods of interest are in choices 0-3 and slv3 indexes
# its fmethod choice the same way.
#----------------------------------------------------------------------------
proc Solve_CONOPT_bppivoting_to_int {choice} {
entertrace
  global ascSolv8Vect
  set i [lsearch -exact $ascSolv8Vect(bppivoting.choices) $choice]
  if {$i == -1 || $i >4} {
    leavetrace
    return 4
  }
  leavetrace
  return $i
leavetrace
}



#
# proc Solve_CONOPT_int_to_bppivoting {i}
#----------------------------------------------------------------------------
# this is a total hack. fix it, probably with an appropriate C primitive.
# assumes the 4 methods of interest are in choices 0-3 and slv3 indexes
# its fmethod choice the same way.
#----------------------------------------------------------------------------
proc Solve_CONOPT_int_to_bppivoting {i} {
entertrace
  global ascSolv8Vect
  if {$i <0 || $i >4} {
    leavetrace
    return "Fast-SPK1/RANKI+ROW"
  }
  set choice "[lindex $ascSolv8Vect(bppivoting.choices) $i]"
  leavetrace
  return $choice
leavetrace
}


#
# proc Solve_CONOPT_Downdate_ParmBox {}
#----------------------------------------------------------------------------
# this updates C structure from the ascSolv8Vect                            #
#----------------------------------------------------------------------------
proc Solve_CONOPT_Downdate_ParmBox {} {
entertrace
  global  ascSolv8Vect ascSolvVect

  if {$ascSolvVect(CONOPT) == -1} {
    leavetrace
    return
  }
  set_slv_parms $ascSolvVect(CONOPT) \
    $ascSolv8Vect(timelimit)         \
    $ascSolv8Vect(iterationlimit)    \
    $ascSolv8Vect(termtol)           \
    $ascSolv8Vect(feastol)           \
    $ascSolv8Vect(pivottol)          \
    $ascSolv8Vect(singtol)           \
    $ascSolv8Vect(stattol)           \
    $ascSolv8Vect(rho)               \
    $ascSolv8Vect(partition)         \
    $ascSolv8Vect(ignorebounds)      \
    $ascSolv8Vect(showmoreimportant) \
    $ascSolv8Vect(showlessimportant) \
    "[Solve_CONOPT_bppivoting_to_int $ascSolv8Vect(bppivoting)]" \
    $ascSolv8Vect(lifds) \
    $ascSolv8Vect(savlin) \
    $ascSolv8Vect(relnomscale) \
    $ascSolv8Vect(cutoff) \
    $ascSolv8Vect(upjac) \
    $ascSolv8Vect(upwts) \
    $ascSolv8Vect(upnom) \
    $ascSolv8Vect(reduce) \
    $ascSolv8Vect(exact) \
    $ascSolv8Vect(cncols) \
    $ascSolv8Vect(btrunc) \
    $ascSolv8Vect(reorder) \
    $ascSolv8Vect(safe_calc) \
    $ascSolv8Vect(uprelnom) \
    $ascSolv8Vect(itscalelim) \
    $ascSolv8Vect(scaleopt) \
    $ascSolv8Vect(toosmall) \
    $ascSolv8Vect(cnlow) \
    $ascSolv8Vect(cnhigh) \
    $ascSolv8Vect(tobnds) \
    $ascSolv8Vect(posdef) \
    $ascSolv8Vect(detzero) \
    $ascSolv8Vect(steperrmax) \
    $ascSolv8Vect(prngmin) \
    $ascSolv8Vect(mincoef) \
    $ascSolv8Vect(maxcoef) \
    $ascSolv8Vect(itscaletol)
leavetrace
}

#
# proc Solve_LSSLV_Downdate_ParmBox {}
#----------------------------------------------------------------------------
# this updates C structure from the ascSolv5Vect                            #
#----------------------------------------------------------------------------
proc Solve_LSSLV_Downdate_ParmBox {} {
entertrace
  global  ascSolv5Vect ascSolvVect
  if {!$ascSolvVect(available.5)} {
    leavetrace
    return
  }
  set_slv_parms 5 \
    $ascSolv5Vect(timelimit)         \
    $ascSolv5Vect(iterationlimit)    \
    $ascSolv5Vect(termtol)           \
    $ascSolv5Vect(feastol)           \
    $ascSolv5Vect(pivottol)          \
    $ascSolv5Vect(singtol)           \
    $ascSolv5Vect(stattol)           \
    $ascSolv5Vect(rho)               \
    $ascSolv5Vect(partition)         \
    $ascSolv5Vect(ignorebounds)      \
    $ascSolv5Vect(showmoreimportant) \
    $ascSolv5Vect(showlessimportant) \
    $ascSolv5Vect(bppivoting)        \
    $ascSolv5Vect(showlessimportantds) \
    $ascSolv5Vect(savlin)
leavetrace
}

#
# proc Solve_MPS_Downdate_ParmBox {}
#----------------------------------------------------------------------------
# this updates C structure from the ascSolv6Vect                            #
# modified by CWS, 5/95
#----------------------------------------------------------------------------
proc Solve_MPS_Downdate_ParmBox {} {
entertrace

  global  ascSolv6Vect ascSolvVect

  if {!$ascSolvVect(available.6)} {
    leavetrace
    return
  }

# DEBUG:
#puts "Starting Solve_MPS_Downdate_ParmBox"

# update stuff: see mps.tcl
  Solve_CloseMakeMPS

  set_slv_parms 6 \
    $ascSolv6Vect(timelimit)         \
    $ascSolv6Vect(iterationlimit)    \
    $ascSolv6Vect(termtol)           \
    $ascSolv6Vect(feastol)           \
    $ascSolv6Vect(pivottol)          \
    $ascSolv6Vect(singtol)           \
    $ascSolv6Vect(stattol)           \
    $ascSolv6Vect(rho)               \
    $ascSolv6Vect(partition)         \
    $ascSolv6Vect(ignorebounds)      \
    $ascSolv6Vect(showmoreimportant) \
    $ascSolv6Vect(showlessimportant) \
    $ascSolv6Vect(bppivoting)        \
    $ascSolv6Vect(nonlin)            \
    $ascSolv6Vect(relaxed)           \
    $ascSolv6Vect(nonneg)            \
    $ascSolv6Vect(obj)               \
    $ascSolv6Vect(binary)            \
    $ascSolv6Vect(integer)           \
    $ascSolv6Vect(semi)              \
    $ascSolv6Vect(sos1)              \
    $ascSolv6Vect(sos2)              \
    $ascSolv6Vect(sos3)              \
    $ascSolv6Vect(bo)                \
    $ascSolv6Vect(eps)               \
    $ascSolv6Vect(boval)             \
    $ascSolv6Vect(epsval)            \
    $ascSolv6Vect(pinf)              \
    $ascSolv6Vect(minf)              \
    $ascSolv6Vect(mpsname)

leavetrace
}

#
# proc Solve_MINOS_Downdate_ParmBox {}
#----------------------------------------------------------------------------
# this updates C structure from the ascSolv0Vect                            #
#----------------------------------------------------------------------------
proc Solve_MINOS_Downdate_ParmBox {} {
entertrace
  global  ascSolv1Vect  ascSolvVect
  if {!$ascSolvVect(available.1)} {
    leavetrace
    return
  }
  set jflxb 0
  if {$ascSolv1Vect(printJ)} {
    set jflxb 10000
  }
  if {$ascSolv1Vect(printF)} {
    set jflxb [expr $jflxb+1000]
  }
  if {$ascSolv1Vect(printL)} {
    set jflxb [expr $jflxb+100]
  }
  if {$ascSolv1Vect(printX)} {
    set jflxb [expr $jflxb+10]
  }
  if {$ascSolv1Vect(printB)} {
    set jflxb [expr $jflxb+1]
  }
  set_slv_parms 1 \
    $ascSolv1Vect(timelimit) \
    $ascSolv1Vect(majits) \
    0 \
    $ascSolv1Vect(tolfeas) \
    0 \
    $ascSolv1Vect(tolsing) \
    $ascSolv1Vect(tolstat) \
    $ascSolv1Vect(rho) \
    0 \
    0 \
    $ascSolv1Vect(showmoreimportant) \
    $ascSolv1Vect(showlessimportant) \
    0 \
    $ascSolv1Vect(completion) \
    $ascSolv1Vect(minits) \
    $ascSolv1Vect(crash) \
    $ascSolv1Vect(deriv) \
    $ascSolv1Vect(cfreq) \
    $ascSolv1Vect(ffreq) \
    $ascSolv1Vect(uselg) \
    $ascSolv1Vect(lfreq) \
    $ascSolv1Vect(mulpr) \
    $ascSolv1Vect(parpr) \
    $jflxb \
    $ascSolv1Vect(scale) \
    $ascSolv1Vect(soln) \
    $ascSolv1Vect(param) \
    $ascSolv1Vect(verify) \
    $ascSolv1Vect(efreq) \
    $ascSolv1Vect(summary) \
    $ascSolv1Vect(filesumm) \
    1 \
    $ascSolv1Vect(damp) \
    $ascSolv1Vect(fdiff) \
    $ascSolv1Vect(cdiff) \
    $ascSolv1Vect(fprec) \
    $ascSolv1Vect(lstol) \
    $ascSolv1Vect(lufto) \
    $ascSolv1Vect(luuto) \
    $ascSolv1Vect(radius) \
    $ascSolv1Vect(subsp) \
    $ascSolv1Vect(objlim) \
    $ascSolv1Vect(steplm) \
    $ascSolv1Vect(lobjwt) \
    $ascSolv1Vect(mindamp) \
    $ascSolv1Vect(ludto) \
    $ascSolv1Vect(lusto) \
    $ascSolv1Vect(luwto)
leavetrace
}
#
# proc Solve_MINOS_Update_ParmBox {}
#----------------------------------------------------------------------------
# this  updates ascSolv1Vect variables from the C structure                 #
# if minos not linked, ignores leavetrace; return from C
#----------------------------------------------------------------------------
proc Solve_MINOS_Update_ParmBox {} {
entertrace
  global ascSolv1Vect
  set tmplist [slv_get_parms 1]
  if { [llength $tmplist]==49 && [lindex $tmplist 0]==1} {
    set ascSolv1Vect(timelimit)          [lindex $tmplist 1]
    set ascSolv1Vect(iterationlimit)     [lindex $tmplist 2]
    set ascSolv1Vect(tolfeas)            [lindex $tmplist 4]
    set ascSolv1Vect(tolsing)            [lindex $tmplist 6]
    set ascSolv1Vect(tolstat)            [lindex $tmplist 7]
    set ascSolv1Vect(rho)                [lindex $tmplist 8]
    set ascSolv1Vect(showmoreimportant)  [lindex $tmplist 11]
    set ascSolv1Vect(showlessimportant)  [lindex $tmplist 12]
    set ascSolv1Vect(completion)         [lindex $tmplist 14]
    set ascSolv1Vect(minits)             [lindex $tmplist 15]
    set ascSolv1Vect(crash)              [lindex $tmplist 16]
    set ascSolv1Vect(deriv)              [lindex $tmplist 17]
    set ascSolv1Vect(cfreq)              [lindex $tmplist 18]
    set ascSolv1Vect(ffreq)              [lindex $tmplist 19]
    set ascSolv1Vect(uselg)              [lindex $tmplist 20]
    set ascSolv1Vect(lfreq)              [lindex $tmplist 21]
    set ascSolv1Vect(mulpr)              [lindex $tmplist 22]
    set ascSolv1Vect(parpr)              [lindex $tmplist 23]
    set jflxb                            [lindex $tmplist 24]
    set ascSolv1Vect(scale)              [lindex $tmplist 25]
    set ascSolv1Vect(soln)               [lindex $tmplist 26]
    set ascSolv1Vect(param)              [lindex $tmplist 27]
    set ascSolv1Vect(verify)             [lindex $tmplist 28]
    set ascSolv1Vect(efreq)              [lindex $tmplist 29]
    set ascSolv1Vect(summary)            [lindex $tmplist 30]
    set ascSolv1Vect(filesumm)           [lindex $tmplist 31]

#    set ascSolv1Vect(lcons)              [lindex $tmplist 32]
    set ascSolv1Vect(lcons)              1

    set ascSolv1Vect(damp)               [lindex $tmplist 33]
    set ascSolv1Vect(fdiff)              [lindex $tmplist 34]
    set ascSolv1Vect(cdiff)              [lindex $tmplist 35]
    set ascSolv1Vect(fprec)              [lindex $tmplist 36]
    set ascSolv1Vect(lstol)              [lindex $tmplist 37]
    set ascSolv1Vect(lufto)              [lindex $tmplist 38]
    set ascSolv1Vect(luuto)              [lindex $tmplist 39]
    set ascSolv1Vect(radius)             [lindex $tmplist 40]
    set ascSolv1Vect(subsp)              [lindex $tmplist 41]
    set ascSolv1Vect(objlim)             [lindex $tmplist 42]
    set ascSolv1Vect(steplm)             [lindex $tmplist 43]
    set ascSolv1Vect(lobjwt)             [lindex $tmplist 44]
    set ascSolv1Vect(damp)               [lindex $tmplist 45]
    set ascSolv1Vect(ludto)              [lindex $tmplist 46]
    set ascSolv1Vect(lusto)              [lindex $tmplist 47]
    set ascSolv1Vect(luwto)              [lindex $tmplist 48]
    # take apart a 5 digit binary number
    foreach i {printJ printX printB printL printF} {
      set ascSolv1Vect($i) 0
    }
    if {[string length $jflxb]==5} {
      set ascSolv1Vect(printJ) 1
      set jflxb [expr 1* [string range $jflxb 1 end]]
    }
    if {[string length $jflxb]==4} {
      set ascSolv1Vect(printF) 1
      set jflxb [expr 1* [string range $jflxb 1 end]]
    }
    if {[string length $jflxb]==3} {
      set ascSolv1Vect(printL) 1
      set jflxb [expr 1* [string range $jflxb 1 end]]
    }
    if {[string length $jflxb]==2} {
      set ascSolv1Vect(printX) 1
      set jflxb [expr 1* [string range $jflxb 1 end]]
    }
    if {$jflxb==1} {
      set ascSolv1Vect(printB) 1
    }
  } else {
    puts "Error in MINOS call to slv_get_parms"
  }
leavetrace
}

# proc Solve_OPTSQP_Downdate_ParmBox {}
#----------------------------------------------------------------------------
# this updates C structure from the ascSolv2Vect                            #
#----------------------------------------------------------------------------
proc Solve_OPTSQP_Downdate_ParmBox {} {
entertrace
  global  ascSolv2Vect  ascSolvVect
  if {!$ascSolvVect(available.2)} {
    leavetrace
    return
  }
  set_slv_parms 2 \
    $ascSolv2Vect(timelimit)         \
    $ascSolv2Vect(iterationlimit)    \
    0           \
    0           \
    0          \
    0           \
    0           \
    0               \
    0         \
    0                                \
    $ascSolv2Vect(showmoreimportant) \
    0                                \
    $ascSolv2Vect(linesearch)        \
    $ascSolv2Vect(iscale)            \
    $ascSolv2Vect(ichoose)           \
    $ascSolv2Vect(imult)             \
    $ascSolv2Vect(isafe)             \
    $ascSolv2Vect(icorr)             \
    $ascSolv2Vect(kprint)            \
    $ascSolv2Vect(iiexact)           \
    $ascSolv2Vect(idebug)            \
    $ascSolv2Vect(eps)               \
    $ascSolv2Vect(vv)
leavetrace
}

#
# proc Solve_OPTSQP_Update_ParmBox {}
#----------------------------------------------------------------------------
# this  updates ascSolv2Vect variables from the C structure                 #
#----------------------------------------------------------------------------
proc Solve_OPTSQP_Update_ParmBox {} {
entertrace
  global ascSolv2Vect ascSolvVect
  if {!$ascSolvVect(available.2)} {
    leavetrace
    return
  }
  set tmplist [slv_get_parms 2]
  if { [llength $tmplist]==24 && [lindex $tmplist 0]==2} {
    set ascSolv2Vect(timelimit)           [lindex $tmplist 1]
    set ascSolv2Vect(iterationlimit)      [lindex $tmplist 2]
    set ascSolv2Vect(termtol)             [lindex $tmplist 3]
    set ascSolv2Vect(feastol)             [lindex $tmplist 4]
    set ascSolv2Vect(pivottol)            [lindex $tmplist 5]
    set ascSolv2Vect(singtol)             [lindex $tmplist 6]
    set ascSolv2Vect(stattol)             [lindex $tmplist 7]
    set ascSolv2Vect(rho)                 [lindex $tmplist 8]
    set ascSolv2Vect(partition)           [lindex $tmplist 9]
    set ascSolv2Vect(ignorebounds)        [lindex $tmplist 10]
    set ascSolv2Vect(showmoreimportant)   [lindex $tmplist 11]
    set ascSolv2Vect(showlessimportant)   [lindex $tmplist 12]
    set ascSolv2Vect(linesearch)          [lindex $tmplist 13]
    set ascSolv2Vect(iscale)              [lindex $tmplist 14]
    set ascSolv2Vect(ichoose)             [lindex $tmplist 15]
    set ascSolv2Vect(imult)               [lindex $tmplist 16]
    set ascSolv2Vect(isafe)               [lindex $tmplist 17]
    set ascSolv2Vect(icorr)               [lindex $tmplist 18]
    set ascSolv2Vect(kprint)              [lindex $tmplist 19]
    set ascSolv2Vect(iiexact)             [lindex $tmplist 20]
    set ascSolv2Vect(idebug)              [lindex $tmplist 21]
    set ascSolv2Vect(eps)                 [lindex $tmplist 22]
    set ascSolv2Vect(vv)                  [lindex $tmplist 23]
  } else {
    puts "Error in OPTSQP call to slv_get_parms"
  }
leavetrace
}

#
# proc Solve_MPS_Update_ParmBox {}
#----------------------------------------------------------------------------
# this updates ascSolv6Vect variables from the C structure                  #
# if MPS not linked, ignores leavetrace; return from C                      
# modified by CWS, 5/95                                                     #
#----------------------------------------------------------------------------
proc Solve_MPS_Update_ParmBox {} {
entertrace
  global ascSolv6Vect
  set tmplist [slv_get_parms 6]
  if { [llength $tmplist]==31 && [lindex $tmplist 0]==6} {

#   general parameters
    set ascSolv6Vect(timelimit)           [lindex $tmplist 1]
    set ascSolv6Vect(iterationlimit)      [lindex $tmplist 2]
    set ascSolv6Vect(termtol)             [lindex $tmplist 3]
    set ascSolv6Vect(feastol)             [lindex $tmplist 4]
    set ascSolv6Vect(pivottol)            [lindex $tmplist 5]
    set ascSolv6Vect(singtol)             [lindex $tmplist 6]
    set ascSolv6Vect(stattol)             [lindex $tmplist 7]
    set ascSolv6Vect(rho)                 [lindex $tmplist 8]
    set ascSolv6Vect(partition)           [lindex $tmplist 9]
    set ascSolv6Vect(ignorebounds)        [lindex $tmplist 10]
    set ascSolv6Vect(showmoreimportant)   [lindex $tmplist 11]
    set ascSolv6Vect(showlessimportant)   [lindex $tmplist 12]
    set ascSolv6Vect(bppivoting)          [lindex $tmplist 13]

#   subparameters
    set ascSolv6Vect(nonlin)              [lindex $tmplist 14]
    set ascSolv6Vect(relaxed)             [lindex $tmplist 15]
    set ascSolv6Vect(nonneg)              [lindex $tmplist 16]
    set ascSolv6Vect(obj)                 [lindex $tmplist 17]
    set ascSolv6Vect(binary)              [lindex $tmplist 18]
    set ascSolv6Vect(integer)             [lindex $tmplist 19]
    set ascSolv6Vect(semi)                [lindex $tmplist 20]
    set ascSolv6Vect(sos1)                [lindex $tmplist 21]
    set ascSolv6Vect(sos2)                [lindex $tmplist 22]
    set ascSolv6Vect(sos3)                [lindex $tmplist 23]
    set ascSolv6Vect(bo)                  [lindex $tmplist 24]
    set ascSolv6Vect(eps)                 [lindex $tmplist 25]
    set ascSolv6Vect(boval)               [lindex $tmplist 26]
    set ascSolv6Vect(epsval)              [lindex $tmplist 27]
    set ascSolv6Vect(pinf)                [lindex $tmplist 28]
    set ascSolv6Vect(minf)                [lindex $tmplist 29]
    set ascSolv6Vect(mpsname)             [lindex $tmplist 30]

  } else {
    puts "Error IN makeMPS call to slv_get_parms"
  }

leavetrace
}

#
# proc Solve_do_Parms {openclose number}
#----------------------------------------------------------------------------
# open and close parameter page of solver $number                           #
#                                                                           #
# note: makeMPS (slv6) uses its own parameter page, and so it must be       #
#       handled as a special Case                                           #
#       modified by CWS, 5/95                                               #
#----------------------------------------------------------------------------
proc Solve_do_Parms {openclose name} {
entertrace $openclose $name
  global ascSolvVect ascParPageVect ascSolvStatVect
  if {$openclose == "open"} {
      if {  $name == "makeMPS" } {
#          see mps.tcl for this procedure
           Solve_OpenMakeMPS
         } else {
           set ascParPageVect(lbl_font) $ascSolvVect(font)
           if { $name != "General" } {
               if {![info exists ascSolvVect($name)] ||
                   $ascSolvVect($name) < 0 } {
                   puts "Solver $name unavailable"
                   leavetrace unavailable solver
                   return
               }
           } else {
               set name 32767
           }
           ascParPage ascSolv${name}Vect [setpos .solver 0 0] 1
         }
      leavetrace
    return

  }
  if {$openclose == "close"} {
# only need to downdate parameters of current solver
      if {$ascSolvStatVect(empty)} {
          leavetrace
          return
      }
      if { $name == [Solve_do_SolverCur] } {
          Solve_Downdate_ParmBox $name
      }
      leavetrace
    return
  }
  puts "Solve_do_Parms expects open or close as an arg"
  leavetrace
}


#
# proc Solve_do_Font {}
#---------------------------------------------------------------------
# font select button for solver window
#---------------------------------------------------------------------
proc Solve_do_Font {args} {
  global ascSolvVect
  set font ""
  if {$args != ""} {
    set font $args
  } else {
    set font  [ascFontGet]
  }
  if {"$font" == ""} {
    return;
  }
  catch {$ascSolvVect(statTable) configure -font $font}
  .solver.msg_frm configure -font $font
  .solver.main_frm.inst_entry configure -font $font
  Debug_font_configure
  set ascSolvVect(font) [lindex [.solver.msg_frm configure -font] 4] 
}

#
# proc Solve_SetModelPath {qid}
#----------------------------------------------------------------------------
# modelbar update                                                           #
# Description: set the model for importing                                  #
# Arguments: qid - the new path name                                        #
# Recreates the solver modelbar with pathname of pulldowns                  #
# Enables/disables import button as appropriate                             #
# Qid should always be passed as a list to this, unless it is a sim name    #
# If you don't know what kind of name you're sending, wrap it in {}         #
# Does nothing if ascsolvvect(modelbar)=0                                   #
# Those who think the modelbar is too complex can turn it off.              #
#----------------------------------------------------------------------------
proc Solve_SetModelPath {qid} {
entertrace
  global ascSolvVect ascGlobalVect

if {$ascSolvVect(modelbar)} {
  set asCurrentItem ""
  set asItemCounter 1
  set asOldPosition 0
 # flush bar on empty name
  if {$qid==""||$qid=="{}"} {
    foreach asCounter [winfo children .solver.main_frm.instpath] {
      destroy $asCounter
    }
    leavetrace
    return
  }

  if {[llength $qid]=="1" } {
    if {[string index $qid 0]=="\{" } {
      set qtmp [string range $qid 1 end]
      set qtmp2 [string range $qtmp 0 [expr [string length $qtmp] -2]]
      set qid $qtmp2
    }
  }
  if {"[info commands .solver.main_frm.instpath]" == ""} {
    set ascSolvVect(pathname) $qid
    leavetrace
    return
  }
  set asPathLength [string length $qid]
  set nroot [lindex [split $qid .] 0]
  set asPosition [expr 1+ [string length [lindex [split $qid .] 0]]]
  set asOldPosition $asPosition
 # guard line to prevent premature destruction of path buttons
  get_model_children $nroot
 # blow away old menubuttons
  foreach asCounter [winfo children .solver.main_frm.instpath] {
    destroy $asCounter
  }
 # handle the simulation name as a special Case
  button .solver.main_frm.instpath.pathLabel0 \
    -borderwidth 0 \
    -text $nroot \
    -command "Solve_SetModelPath \{$nroot\}"

  menubutton .solver.main_frm.instpath.pathMenu0 \
    -borderwidth 0 \
    -text "." \
    -menu ".solver.main_frm.instpath.pathMenu0.m"

  menu .solver.main_frm.instpath.pathMenu0.m \
    -tearoffcommand .SOLVER.MAIN_FRM.INSTPATH.PATHMENU0.M \
    -tearoff 0

  foreach asCounter [get_model_children $nroot] {
    .solver.main_frm.instpath.pathMenu0.m add command \
      -label "$asCounter" \
      -command "Solve_SetModelPath \{$nroot.$asCounter\}"
  }


  if {$asPosition == $asPathLength} {
    set ascSolvVect(pathname) $qid
  }
  while {$asPosition < $asPathLength} {
    while {$asPosition < $asPathLength} {
      set asCurrent [string index $qid $asPosition]
      if {[string match $asCurrent "."] &&
          ![string match $asCurrent "\*"]} {
        break
      }
      incr asPosition 1
    }
    set ascSolvVect(pathname) [string range $qid 0 [expr $asPosition-1]]
    set asCurrentItem [string range $qid $asOldPosition [expr $asPosition-1]]
    button .solver.main_frm.instpath.pathLabel$asItemCounter \
      -borderwidth 0 \
      -relief flat \
      -text "$asCurrentItem" \
      -command "Solve_SetModelPath \{$ascSolvVect(pathname)\}"

    menubutton .solver.main_frm.instpath.pathMenu$asItemCounter \
      -borderwidth 0 \
      -text {.} \
      -menu ".solver.main_frm.instpath.pathMenu$asItemCounter.m"

    menu .solver.main_frm.instpath.pathMenu$asItemCounter.m \
      -tearoffcommand .SOLVER.MAIN_FRM.INSTPATH.PATHMENU$ASITEMCOUNTER.M \
      -tearoff 0

    foreach asChildren [get_model_children $ascSolvVect(pathname)] {
      .solver.main_frm.instpath.pathMenu$asItemCounter.m add command \
        -label "$asChildren" \
        -command "Solve_SetModelPath \{$ascSolvVect(pathname).$asChildren\}"
    }
    incr asItemCounter
    incr asPosition
    set asOldPosition $asPosition
  }

  set asCounter 0
  while {$asCounter < $asItemCounter} {
    pack append .solver.main_frm.instpath \
      .solver.main_frm.instpath.pathLabel$asCounter {left}
    pack append .solver.main_frm.instpath \
      .solver.main_frm.instpath.pathMenu$asCounter {left}
    incr asCounter 1
  }
  set ascSolvVect(pathname) $qid
  set notok [slv_import_qlfdid $qid test]
  if {$notok} {
    .solver.main_frm.btn_expo.m entryconfigure 0 -state disabled
  } else {
    .solver.main_frm.btn_expo.m entryconfigure 0 -state normal
  }
}
 # end of modelbar code
leavetrace
}

#
# proc Solve_Update_Listbox {}
#----------------------------------------------------------------------------
# listbox update and typebar update                                         #
#----------------------------------------------------------------------------
proc Solve_Update_Listbox {} {
entertrace
  global ascSolvVect ascSolvStatVect

  set ascSolvVect(instname) [slv_get_pathname]
  if {$ascSolvVect(instname)=="none"} {
    set ascSolvVect(instname) ""
  }
  if {!$ascSolvStatVect(empty)} {
    set ascSolvVect(modeltype) "IS_A [slv_get_insttype]"
  } else {
    set ascSolvVect(modeltype) ""
  }
  Solve_SetModelPath "\{$ascSolvVect(instname)\}"
leavetrace
}

#
# proc Solve_Update_StatusBox {}
#----------------------------------------------------------------------------
# this updates ascSolvStat Page variables and interrupt button color        #
#----------------------------------------------------------------------------
proc Solve_Update_StatusBox {args} {
entertrace
  global ascSolvStatVect
  global ascSolvVect
  Solve_Update_StatVect

  # state variable
  set ascSolvStatVect(state) "undefined"
  if {($ascSolvStatVect(overdefined)=="1")} {
    set ascSolvStatVect(degf) [expr $ascSolvStatVect(inc_eqals) + \
        $ascSolvStatVect(fixed_vars) - $ascSolvStatVect(inc_vars)]
    set ascSolvStatVect(state) "over ($ascSolvStatVect(degf))"
  }
  if {($ascSolvStatVect(underdefined)=="1")} {
    set ascSolvStatVect(degf) [expr -$ascSolvStatVect(inc_eqals) - \
        $ascSolvStatVect(fixed_vars) + $ascSolvStatVect(inc_vars)]
    set ascSolvStatVect(state) "under ($ascSolvStatVect(degf))"
  }
  if {($ascSolvStatVect(inconsistent)=="1")} {
    set ascSolvStatVect(state) "inconsistent"
  }
  if {!($ascSolvStatVect(overdefined)=="1") && \
      !($ascSolvStatVect(underdefined)=="1") && \
      !($ascSolvStatVect(inconsistent)=="1") && \
      !($ascSolvStatVect(empty)=="1") } {
    set ascSolvStatVect(state) "square"
  }
 # square is meaningless in optimizing
 # this needs cleaning up
  if { ("[string range $ascSolvStatVect(objval) 0 3]" != "none") \
      && !($ascSolvStatVect(empty)=="1") } {
     set ascSolvStatVect(state) "deg.f: [expr -$ascSolvStatVect(inc_eqals) - \
        $ascSolvStatVect(fixed_vars) + $ascSolvStatVect(inc_vars)]"
  }
  set ascSolvStatVect(iterations) \
      "$ascSolvStatVect(iteration)/$ascSolvStatVect(block.iteration)"
  # status button variable
  set ascSolvVect(status) ""
  if { ($ascSolvStatVect(ok)=="1") } {
    set ascSolvVect(status) "$ascSolvVect(status) OK" }
  if { ($ascSolvStatVect(overdefined)=="1") } {
    set ascSolvVect(status) "$ascSolvVect(status), over specified" }
  if { ($ascSolvStatVect(underdefined)=="1") } {
    set ascSolvVect(status) "$ascSolvVect(status), under specified" }
  if { ($ascSolvStatVect(structsingular)=="1") } {
    set ascSolvVect(status) "$ascSolvVect(status), structurally singular" }
  if { ($ascSolvStatVect(ready2solve)=="1") } {
    set ascSolvVect(status) "$ascSolvVect(status), ready to solve" }
  if { ($ascSolvStatVect(converged)=="1") } {
    Solve_Check_Block_Err
    set metmp "max. block error ($ascSolvStatVect(worstblock))"
    set metmp "$metmp, $ascSolvStatVect(maxblockerr)"
    set ascSolvVect(status) "$ascSolvVect(status), converged, $metmp"
  }
  if { ($ascSolvStatVect(diverged)=="1") } {
    set ascSolvVect(status) "$ascSolvVect(status), diverged" }
  if { ($ascSolvStatVect(inconsistent)=="1") } {
    set ascSolvVect(status) "$ascSolvVect(status), inconsistent" }
  if { ($ascSolvStatVect(calcok)=="0") } {
    set ascSolvVect(status) "$ascSolvVect(status), error in calculation" }
  if { ($ascSolvStatVect(itnlim_exceeded)=="1") } {
    set ascSolvVect(status) "$ascSolvVect(status), iteration limit exceeded" }
  if { ($ascSolvStatVect(timlim_exceeded)=="1") } {
    set ascSolvVect(status) "$ascSolvVect(status), time limit exceeded" }
  set ascSolvVect(status) "$ascSolvVect(status)."
  if { ($ascSolvStatVect(empty)=="1") } {set $ascSolvVect(status) ""}

  if {![slv_checksys]} {
    set ascSolvStatVect(totalrels) ""
    set ascSolvStatVect(in_rels) ""
    set ascSolvStatVect(rels) ""
    set ascSolvStatVect(inc_rels) ""
    set ascSolvStatVect(eqals) ""
    set ascSolvStatVect(inc_eqals) ""
    set ascSolvStatVect(ineqals) ""
    set ascSolvStatVect(inc_ineqals) ""
    set ascSolvStatVect(in_inc_eqals) ""
    set ascSolvStatVect(in_inc_ineqals) ""
    set ascSolvStatVect(uninc_rels) ""
    set ascSolvStatVect(totalvars) ""
    set ascSolvStatVect(vars) ""
    set ascSolvStatVect(in_vars) ""
    set ascSolvStatVect(inc_vars) ""
    set ascSolvStatVect(un_vars) ""
    set ascSolvStatVect(free_vars) ""
    set ascSolvStatVect(fixed_vars) ""
    set ascSolvStatVect(in_free_vars) ""
    set ascSolvStatVect(in_fixed_vars) ""
    set ascSolvStatVect(block.residual) ""
    set ascSolvStatVect(free_vars) ""
    set ascSolvStatVect(fixed_vars) ""
    set ascSolvStatVect(block.number) ""
#    set ascSolvStatVect(solver) ""
    set ascSolvStatVect(iterations) ""
    set ascSolvStatVect(solved_vars) ""
    set ascSolvStatVect(block.current) ""
    set ascSolvStatVect(objval) ""
    set ascSolvStatVect(state) "empty"
    set ascSolvStatVect(start_block) ""
    set ascSolvStatVect(stop_block) ""
  }
  if {[llength $args] == 1 && $ascSolvStatVect(ready2solve)} {
    Solv_Update_Stattable 1
  } else {
    Solv_Update_Stattable 0
  }
leavetrace
}


#
# proc Solve_Update_StatVect {}
#----------------------------------------------------------------------------
# this updates ascSolvStatVect variable from the C data structure           #
#----------------------------------------------------------------------------
proc Solve_Update_StatVect {} {
entertrace
  global ascSolvStatVect
  global ascSolvVect
  if {[slv_checksys]} {
    set tmplist [slv_get_stat_page]
    set tmpvrlist [slv_get_vr [slv_get_solver]]
#    set ascSolvStatVect(solver) $ascSolvVect(name.[slv_get_solver])
    set ascSolvStatVect(objval) [slv_get_objval]
    set ascSolvStatVect(empty) "0"
  } else {
    set ascSolvStatVect(empty) "1"
    set tmplist "no model"
    set tmpvrlist "no model"
#    set ascSolvStatVect(solver) ""
  }
  if {[llength $tmplist]==20 } {
    set ascSolvStatVect(ok)               [lindex $tmplist 0]
    set ascSolvStatVect(overdefined)      [lindex $tmplist 1]
    set ascSolvStatVect(underdefined)     [lindex $tmplist 2]
    set ascSolvStatVect(structsingular)   [lindex $tmplist 3]
    set ascSolvStatVect(ready2solve)      [lindex $tmplist 4]
    set ascSolvStatVect(converged)        [lindex $tmplist 5]
    set ascSolvStatVect(diverged)         [lindex $tmplist 6]
    set ascSolvStatVect(inconsistent)     [lindex $tmplist 7]
    set ascSolvStatVect(calcok)           [lindex $tmplist 8]
    set ascSolvStatVect(itnlim_exceeded)  [lindex $tmplist 9]
    set ascSolvStatVect(timlim_exceeded)  [lindex $tmplist 10]
    set ascSolvStatVect(iteration)        [lindex $tmplist 11]
    set ascSolvStatVect(cpuelapsed)       [lindex $tmplist 12]

    set ascSolvStatVect(block.number)      [lindex $tmplist 13]
    set ascSolvStatVect(block.current)     [lindex $tmplist 14]
    set ascSolvStatVect(block.size)        [lindex $tmplist 15]
    set ascSolvStatVect(block.prevtotsize) [lindex $tmplist 16]
    set ascSolvStatVect(block.iteration)   [lindex $tmplist 17]
    set ascSolvStatVect(block.cpuelapsed)  [lindex $tmplist 18]
    set ascSolvStatVect(block.residual)    [lindex $tmplist 19]
    set ascSolvStatVect(solved_vars) $ascSolvStatVect(block.prevtotsize)
    set ascSolvStatVect(iterations) \
      "$ascSolvStatVect(iteration)/$ascSolvStatVect(block.iteration)"
    # this takes care of the Case where float error happened before
    # update_status in the solver got to fix ready2solve and calcok.
    if {$ascSolvStatVect(ready2solve) && $ascSolvStatVect(fpcaught)} {
      set ascSolvStatVect(ready2solve) 0
      set ascSolvStatVect(fpcaught) 0
    }
  } else {
    set ascSolvStatVect(ok)                 ""
    set ascSolvStatVect(overdefined)        ""
    set ascSolvStatVect(underdefined)       ""
    set ascSolvStatVect(structsingular)     ""
    set ascSolvStatVect(ready2solve)        ""
    set ascSolvStatVect(fpcaught)           0
    set ascSolvStatVect(converged)          ""
    set ascSolvStatVect(diverged)           ""
    set ascSolvStatVect(inconsistent)       ""
    set ascSolvStatVect(calcok)             ""
    set ascSolvStatVect(itnlim_exceeded)    ""
    set ascSolvStatVect(timlim_exceeded)    ""
    set ascSolvStatVect(iteration)          ""
    set ascSolvStatVect(cpuelapsed)         ""
    set ascSolvStatVect(objval)             ""

    set ascSolvStatVect(block.number)       ""
    set ascSolvStatVect(block.current)      ""
    set ascSolvStatVect(block.size)         ""
    set ascSolvStatVect(block.prevtotsize)  ""
    set ascSolvStatVect(block.iteration)    ""
    set ascSolvStatVect(block.cpuelapsed)   ""
    set ascSolvStatVect(block.residual)     ""
    set ascSolvStatVect(solved_vars)        ""
  }
  if {[llength $tmpvrlist]==17 } {
    set ascSolvStatVect(totalrels)        [lindex $tmpvrlist 0]
    set ascSolvStatVect(rels)             [lindex $tmpvrlist 1]
    set ascSolvStatVect(inc_rels)         [lindex $tmpvrlist 2]
    set ascSolvStatVect(totalvars)        [lindex $tmpvrlist 3]
    set ascSolvStatVect(vars)             [lindex $tmpvrlist 4]
    set ascSolvStatVect(free_vars)        [lindex $tmpvrlist 5]
    set ascSolvStatVect(eqals)            [lindex $tmpvrlist 6]
    set ascSolvStatVect(inc_eqals)        [lindex $tmpvrlist 7]
    set ascSolvStatVect(ineqals)          [lindex $tmpvrlist 8]
    set ascSolvStatVect(inc_ineqals)      [lindex $tmpvrlist 9]
    set ascSolvStatVect(in_inc_eqals)     [lindex $tmpvrlist 10]
    set ascSolvStatVect(in_inc_ineqals)   [lindex $tmpvrlist 11]
    set ascSolvStatVect(uninc_rels)       [lindex $tmpvrlist 12]
    set ascSolvStatVect(fixed_vars)       [lindex $tmpvrlist 13]
    set ascSolvStatVect(in_free_vars)     [lindex $tmpvrlist 14]
    set ascSolvStatVect(in_fixed_vars)    [lindex $tmpvrlist 15]
    set ascSolvStatVect(un_vars)          [lindex $tmpvrlist 16]
    set ascSolvStatVect(inc_vars) \
        [expr $ascSolvStatVect(free_vars) + $ascSolvStatVect(fixed_vars)]
    set ascSolvStatVect(in_rels) \
        [expr $ascSolvStatVect(totalrels) - $ascSolvStatVect(rels)]
    set ascSolvStatVect(in_vars) \
        [expr $ascSolvStatVect(totalvars) - $ascSolvStatVect(vars)]
  } else {
    set ascSolvStatVect(totalrels) ""
    set ascSolvStatVect(rels) ""
    set ascSolvStatVect(in_rels) ""
    set ascSolvStatVect(inc_rels) ""
    set ascSolvStatVect(eqals) ""
    set ascSolvStatVect(inc_eqals) ""
    set ascSolvStatVect(ineqals) ""
    set ascSolvStatVect(inc_ineqals) ""
    set ascSolvStatVect(in_inc_eqals) ""
    set ascSolvStatVect(in_inc_ineqals) ""
    set ascSolvStatVect(uninc_rels) ""
    set ascSolvStatVect(totalvars) ""
    set ascSolvStatVect(vars) ""
    set ascSolvStatVect(in_vars) ""
    set ascSolvStatVect(inc_vars) ""
    set ascSolvStatVect(un_vars) ""
    set ascSolvStatVect(free_vars) ""
    set ascSolvStatVect(fixed_vars) ""
    set ascSolvStatVect(in_free_vars) ""
    set ascSolvStatVect(in_fixed_vars) ""
  }
  # the following call updates the table
  Solve_Update_TableVect
leavetrace
}

# proc Solv_Update_Stattable {{running 0}} 
#----------------------------------------------------------------------------
# user may delete with button the
# table widget, so it must be recreated.
# the idea is that the text is really just a container
# for frame windows which can alternate with single
# lines (button widgets)
#----------------------------------------------------------------------------
proc Solv_Update_Stattable {{running 0}} {
  global ascSolvVect
  $ascSolvVect(textBox) configure -state normal
  if {![winfo exists $ascSolvVect(statButton)]} {
    checkbutton $ascSolvVect(statButton) \
      -onvalue 1 \
      -offvalue 0 \
      -text {Show system statistics:} \
      -variable ascSolvVect(statVisible) \
      -command Solv_Update_Stattable
    $ascSolvVect(textBox) window create 1.0 \
      -window $ascSolvVect(statButton)
    $ascSolvVect(textBox) insert \
      [expr 1 + [$ascSolvVect(textBox) index $ascSolvVect(statButton)]] \
      "\n"
  }
  if {!$ascSolvVect(statVisible)} {
    if {[winfo exists $ascSolvVect(statTable)]} {
      destroy $ascSolvVect(statTable)
    }
    return
  }
  if {![winfo exists $ascSolvVect(statTable)]} {
    # build widget $ascSolvVect(statTable)
    table $ascSolvVect(statTable) \
      -variable ascSolvTableVect \
      -anchor w \
      -rows 15 \
      -cols 4 \
      -colstretchmode all \
      -exportselection 0 \
      -yscrollcommand {} \
      -font $ascSolvVect(font) \
      -state disabled
    $ascSolvVect(textBox) window create 2.0 \
      -window $ascSolvVect(statTable)
  }
  Solv_update_statTable_widths $running
  $ascSolvVect(textBox) configure -state disabled
}

# proc Solv_update_statTable_widths {running} 
#----------------------------------------------------------------------------
# this function reconfigures the stat table columns to see all text
#----------------------------------------------------------------------------
proc Solv_update_statTable_widths {running} {
  global ascSolvVect ascSolvStatVect
  global ascStatTable
  if {![winfo exists $ascSolvVect(statTable)]} {
    return
  }
  foreach i {0 1 2 3} {
    set w [Table_calc_column_width $ascSolvVect(statTable) $i]
    if {!$running || \
        [$ascSolvVect(statTable) width $i] < $w} {
      $ascSolvVect(statTable) width $i $w
    }
  }
}

#
# proc Solve_Update_TableVect {}
#----------------------------------------------------------------------------
# Brings the values in ascSolvTableVect inline with those in ascSolvStatVect
#----------------------------------------------------------------------------
proc Solve_Update_TableVect {} {
  global ascSolvTableVect
  global ascSolvStatVect

  set ascSolvTableVect(0,1) $ascSolvStatVect(totalrels)
  set ascSolvTableVect(1,1) $ascSolvStatVect(rels)
  set ascSolvTableVect(2,1) $ascSolvStatVect(inc_eqals)
  set ascSolvTableVect(3,1) $ascSolvStatVect(inc_ineqals)
  set ascSolvTableVect(6,1) $ascSolvStatVect(in_rels)
  set ascSolvTableVect(7,1) $ascSolvStatVect(in_inc_eqals)
  set ascSolvTableVect(8,1) $ascSolvStatVect(in_inc_ineqals)
  set ascSolvTableVect(9,1) $ascSolvStatVect(uninc_rels)
  set ascSolvTableVect(11,1) $ascSolvStatVect(objval)

#cheating the column format...
  set ascSolvTableVect(13,0) "Error: $ascSolvStatVect(block.residual)"
  
  set ascSolvTableVect(0,3) $ascSolvStatVect(totalvars)
  set ascSolvTableVect(1,3) $ascSolvStatVect(vars)
  set ascSolvTableVect(2,3) $ascSolvStatVect(free_vars)
  set ascSolvTableVect(3,3) $ascSolvStatVect(fixed_vars)
  set ascSolvTableVect(4,3) $ascSolvStatVect(state)
  set ascSolvTableVect(6,3) $ascSolvStatVect(in_vars)
  set ascSolvTableVect(7,3) $ascSolvStatVect(in_free_vars)
  set ascSolvTableVect(8,3) $ascSolvStatVect(in_fixed_vars)
  set ascSolvTableVect(9,3) $ascSolvStatVect(un_vars)
  set ascSolvTableVect(11,3) $ascSolvStatVect(block.number)
  set ascSolvTableVect(12,3) $ascSolvStatVect(iterations)
  set ascSolvTableVect(13,3) $ascSolvStatVect(block.current)
  set ascSolvTableVect(14,3) $ascSolvStatVect(solved_vars)
}


#
# proc Solve_OpenDebugger {}
#----------------------------------------------------------------------------
# popup the debugger window.                                                #
#----------------------------------------------------------------------------
proc Solve_OpenDebugger {} {
entertrace
  global ascSolvVect ascDebuVect
  set ascSolvVect(debuggerup) 1
  #set ascDebuVect(geometry) [osgpos 156x482[setpos .solver 229 42]]
  ShowWindow.debug
  Debug_Trace on
leavetrace
}

#
# proc Solve_CloseDebugger {}
#----------------------------------------------------------------------------
# pop down the debugger window.                                             #
#----------------------------------------------------------------------------
proc Solve_CloseDebugger {} {
entertrace
  global ascSolvVect
  set ascSolvVect(debuggerup) 0
  Debug_Trace off
  DestroyWindow.debug
leavetrace
}


#
# proc Solve_Help_Solvers {}
#----------------------------------------------------------------------------
# help button for ascend solver codes
#----------------------------------------------------------------------------
proc Solve_Help_Solvers {} {
entertrace
  puts "==================================================="
  puts "Here are some of the C calls :"
  slvhelp l
  puts "==================================================="
  puts "Here are some of the tcl calls :"
  puts [info procs Solv*]
  puts "==================================================="
  Help_button solver.help.onsolvers
leavetrace
}

#
# proc Solve_Update_MenuBar {}
#----------------------------------------------------------------------------
# En/Dis-able all menus on bar but help menu                                #
# update indexing if menu structure changes                                 #
#----------------------------------------------------------------------------
proc Solve_Update_MenuBar {} {
entertrace
  global ascSolvStatVect 
  global ascGlobalVect

    foreach i {2 3} {
        .solver.menubar.edit entryconfigure $i -state disabled
    }
    foreach i {0 1 2 3} {
      .solver.menubar.display entryconfigure $i -state disabled
    }
    foreach i {0 1 2} {
      .solver.menubar.execute entryconfigure $i -state disabled
    }
    foreach i {0 1 2 3 4 5 6 7} {
      .solver.menubar.analyze entryconfigure $i -state disabled
    }
    foreach i {0 1 } {
      .solver.menubar.export entryconfigure $i -state disabled
    }
    if {$ascGlobalVect(saveoptions) == 1} {
      foreach i {0 1 2 3} {
        .solver.menubar.view entryconfigure $i -state normal
      }
    } else {
      foreach i {2 3} {
        .solver.menubar.view entryconfigure $i -state disabled
      }
    }
 
  if {[slv_checksys]} {
    foreach i {0 1 2 3} {
      .solver.menubar.edit entryconfigure $i -state normal
    }
    foreach i {0 4} {
      .solver.menubar.display entryconfigure $i -state normal
    }
    if {$ascSolvStatVect(un_vars)!=0} {
       .solver.menubar.display entryconfigure 1 -state normal}
    if {[expr $ascSolvStatVect(rels)-$ascSolvStatVect(inc_eqals)- \
         $ascSolvStatVect(inc_ineqals)] > 0} {
       .solver.menubar.display entryconfigure 2 -state normal}
    foreach i {0 1} {
      .solver.menubar.execute entryconfigure $i -state normal
    }
    if {[integrate_able solver blsode]} {
      .solver.menubar.execute entryconfigure 2 -state normal
    }
    foreach i {0 1 3 6 7} {
      .solver.menubar.analyze entryconfigure $i -state normal
    }
    foreach i {0 1} {
      .solver.menubar.export entryconfigure $i -state normal
    }
    if {$ascSolvStatVect(overdefined)} {
      .solver.menubar.analyze entryconfigure 2 -state normal
    }
    if {$ascSolvStatVect(structsingular)} {
      .solver.menubar.analyze entryconfigure 4 -state normal
    }
    if {[expr $ascSolvStatVect(rels)-$ascSolvStatVect(inc_eqals)- \
         $ascSolvStatVect(inc_ineqals)] > 0} {
      .solver.menubar.analyze entryconfigure 5 -state normal
    }
  }
leavetrace
}

#
# proc Solve_Redraw {}
#----------------------------------------------------------------------------
# Redraw the solver window, with whatever model there is.                   #
# does a presolve.                                                          #
#----------------------------------------------------------------------------
proc Solve_Redraw {} {
entertrace
  global ascSolvStatVect
  if {![slv_checksys]} {
    set ascSolvStatVect(state) "empty"
  } else {
    slv_presolve
  }
  Solve_Update_StatusBox
  Solve_Update_Listbox
  Solve_Update_MenuBar
leavetrace
}

proc Solve_do_Close {} {
  global ascSolvVect
  set ascSolvVect(window.open) 0
}

proc Solve_do_Exit {} {
  Script_do_Exit
}

#
# proc Solve_OpenMtx {}
#----------------------------------------------------------------------------
# Draw the incidence window, if appropriate.                                #
#----------------------------------------------------------------------------
proc Solve_OpenMtx {} {
entertrace
  global ascSolvStatVect ascMtxVect ascSolvVect

  if {!$ascSolvVect(mtxup)} {
    set ascMtxVect(geometry) [osgpos 314x373[setpos .solver 0 42]]
    ShowWindow.mtx
    set ascSolvVect(mtxup) 1
  }
  raise $ascMtxVect(windowname)
  if {[expr $ascSolvStatVect(rels)>0]} {
    set ht [lindex [.mtx.can_mtx.canvas2 configure -height ] 4]
    set ascMtxVect(sf) [expr $ht/$ascSolvStatVect(rels)]
  } else {
    set ascMtxVect(sf) 14
  }
  if {[expr $ascMtxVect(sf) <1]} {
    set ascMtxVect(sf) 1
  }
  if {[expr $ascMtxVect(sf) >14]} {
    set ascMtxVect(sf) 14
  }
  .mtx.zoom set $ascMtxVect(sf)
  if {![slv_checksys]} {
    Solve_CloseMtx
    leavetrace
    return
  }
 # old version
 # Mtx_Plot_CIncidence $ascSolvStatVect(vars) \
 #                       $ascSolvStatVect(rels) $ascMtxVect(sf) \
 #                       [dbg_get_order r] [dbg_get_order col] 1
  Mtx_Plot_FIncidence $ascSolvStatVect(vars) \
                        $ascSolvStatVect(rels) $ascMtxVect(sf)
  set ascMtxVect(eqnnum) ""
  set ascMtxVect(varnum) ""
  set ascMtxVect(eqnname) ""
  set ascMtxVect(varname) ""
leavetrace
}

#
# proc Solve_CloseMtx {}
#----------------------------------------------------------------------------
# Blow away the incidence matrix window.                                    #
#----------------------------------------------------------------------------
proc Solve_CloseMtx {} {
entertrace
  global ascSolvVect
  DestroyWindow.mtx
  set ascSolvVect(mtxup) 0
leavetrace
}
#
#
#----------------------------------------------------------------------------
# routines that handle hub calls
#----------------------------------------------------------------------------
#
# proc Solve_PrependQueue {qaction}
#----------------------------------------------------------------------------
# Add to top of list of things to do once Solve/Interate leavetrace;
# returns from C to
# TCL control. The queue will be cleared between Solver calls to C.
#----------------------------------------------------------------------------
proc Solve_PrependQueue {qaction} {
entertrace
  global ascSolvStatVect
  set ascSolvStatVect(eventqueue) [linsert \
    ascSolvStatVect(eventqueue) 0 $qaction]
leavetrace
}
#
# proc Solve_AppendQueue {qaction}
#----------------------------------------------------------------------------
# Add to end of list of things to do once Solve/Interate leavetrace; 
# returns from C to
# TCL control. The queue will be cleared between Solver calls to C.
#----------------------------------------------------------------------------
proc Solve_AppendQueue {qaction} {
entertrace
  global ascSolvStatVect
  lappend ascSolvStatVect(eventqueue) $qaction
leavetrace
}
#
# proc Solve_ClearQueue {}
#----------------------------------------------------------------------------
# Clear list of things to do once Solve/Interate leavetrace;
# returns from C to TCL control
# no excuses.
#----------------------------------------------------------------------------
proc Solve_ClearQueue {} {
entertrace
  global ascSolvStatVect
  foreach i $ascSolvStatVect(eventqueue) {
    if {[catch {eval $i} ]} {
      puts "Error executing solver queue item\n>>>$i<<<\nContinuing on queue"
    }
  }
  set ascSolvStatVect(eventqueue) ""
leavetrace
}
#
# proc Solve_HandleTypesDelete {}
#----------------------------------------------------------------------------
# flush solver if solver_var deleted
#----------------------------------------------------------------------------
proc Solve_HandleTypesDelete {args} {
entertrace
  global ascSolvVect
  if {![slv_checksys]} { leavetrace; return }
  Solve_do_Flush
leavetrace
}
#
# proc Solve_HandleSimsDelete {sims}
#----------------------------------------------------------------------------
# flush solver if sim deleted is current sim
#----------------------------------------------------------------------------
proc Solve_HandleSimsDelete {sims} {
entertrace
  global ascSolvVect
  if {![slv_checksys]} {
    leavetrace
    return
  }
  if {$sims==$ascSolvVect(simname)} {
    Solve_do_Flush
  }
leavetrace
}
#
# proc Solve_HandleInstanceMoved {args}
#----------------------------------------------------------------------------
# Flush solver if sim moved is current sim. But only after we have
# safely escaped from C control.
# requires tk if . exists.
# If . does not exist, user is fucking around on the command line and
# shouldn't be able to do things that relocate an instance while the
# solver is also running.
#----------------------------------------------------------------------------
proc Solve_HandleInstanceMoved {args} {
entertrace
  global ascSolvVect ascSolvStatVect
  if {![slv_checksys]} {
    leavetrace
    return
  }
  if {$args == ""} {
    if {$ascSolvStatVect(running)} {
      if {[winfo exists .]} {
        set ascSolvStatVect(menubreak) 1
        slv_set_haltflag 1
        frame .solvewaiting
        Solve_AppendQueue "destroy .solvewaiting"
        tkwait window .solvewaiting
      }
    }
    Solve_do_Flush
    leavetrace
    return
  }
  set simname [string trim [lindex [split $args .] 0] \{\}]
  if {$simname==$ascSolvVect(simname)} {
    if {$ascSolvStatVect(running)} {
      if {[winfo exists .]} {
        set ascSolvStatVect(menubreak) 1
        slv_set_haltflag 1
        frame .solvewaiting
        Solve_AppendQueue "destroy .solvewaiting"
        tkwait window .solvewaiting
      }
    }
    Solve_do_Flush
  }
leavetrace
}
#
# proc Solve_HandlePrecisionUpdated {args}
#----------------------------------------------------------------------------
# display precision update routine
#----------------------------------------------------------------------------
proc Solve_HandlePrecisionUpdated {args} {
entertrace
leavetrace
}
#
# proc Solve_HandleBooleanUpdated {args}
#----------------------------------------------------------------------------
# if simname sent is in redo import because structure may have changed
# if no simname sent, flush solver because we may be wrong.
# Solver will catch up next time it returns from C.
# Because of aliasing in ascend we can't check any more rigorously
# than at the simulation level that we need to redo things.
# With anticipated external instance references, even that won't
# be a sufficient test and we will have to write C to figure out
# whether the change in question may have affected us. sigh.
#----------------------------------------------------------------------------
proc Solve_HandleBooleanUpdated {args} {
entertrace
  global ascSolvStatVect ascSolvVect
  if {![slv_checksys]} {
    leavetrace slv_checksys
    return
  }
  if {$args == ""} {
    if {$ascSolvStatVect(running)} {
      set ascSolvStatVect(menubreak) 1
      slv_set_haltflag 1
      Solve_AppendQueue Solve_do_Flush
    } else {
      Solve_do_Flush
    }
    leavetrace
    return
  }

#The following line was added durring the ASCEND IV conversion
#because args was being passed in with braces.
  set args [stripbraces $args]
  set simname [lindex [split $args .] 0]

  if {$simname==$ascSolvVect(simname)} {
    if {$ascSolvStatVect(running)} {
      set ascSolvStatVect(menubreak) 1
      slv_set_haltflag 1
      Solve_AppendQueue "Solve_Import_Any $ascSolvVect(instname)"
    } else {
      Solve_Import_Any $ascSolvVect(instname)
    }
  }
leavetrace end
}


#
# proc Solve_When_Updated {qlfdid}
#----------------------------------------------------------------------------
# Change the active flag for variables and relations after the value of
# a variable in a whenvarlist has changed. Structural analysis is also
# performed.
# Leavetrace; Returns 0 if succesful, 1 if not. If not, reason will be left in
# global variable ascSolvStatVect(importerror).
#----------------------------------------------------------------------------
proc Solve_When_Updated {qlfdid} {
entertrace
  global ascSolvVect ascSolvStatVect

 # check running
  if {$ascSolvStatVect(running)=="1"} {
    puts stderr "Solve in progress."
    set sherrmsg \
      "Solve in progress.Cannot update configuration until done or halted"
    set halt  [asctk_dialog .solvhalt \
                $ascSolvVect(font) "Solver Alert:" $sherrmsg "" 1 OK Halt]
    if {$halt} {
      set ascSolvStatVect(menubreak) 1
      slv_set_haltflag 1
    }
    set ascSolvStatVect(importerror) "Solver is running."
    leavetrace; return 1
  }
 # check qlfdid
  if {$qlfdid==""} {
    Solve_do_Flush;
    set ascSolvStatVect(importerror) "Update config called with no argument."
    leavetrace; return 1
  }
  set ascSolvStatVect(menubreak) 0
  slv_set_haltflag 0
  set notok [slv_import_qlfdid $qlfdid test]
  if {$notok} {
    set ascSolvStatVect(importerror) "Instance not a complete model instance."
    leavetrace; return 1
  }
 # set simname
  set ascSolvVect(simname) [lindex [split $qlfdid .] 0]
  if {$ascSolvVect(visibility)} {newraise $ascSolvVect(windowname) }
 # over/under
  Solve_Check_DOF
  #
  # Update active flag of relations
  #
  Solve_FlagActive
  Solve_Update_StatusBox
 # redraw windows
  Solve_Update_Listbox
  Solve_Update_MenuBar
  catch {Solve_Update_Slaves}
  leavetrace; return 0
leavetrace
}


#
# proc Solve_HandleWhenVarUpdated {args}
#----------------------------------------------------------------------------
# Redo import because structure have changed. This is called after
# running a procedure or after modifying the value of a variable
# included in a whenvarlist. It has to be fixed. slv__check_and reanalyze
# has to be  executed when only a conditional variable was modified,
# slv_reanalyze has to be executed after running a procedure. Currently
# slv_check_and_reanalyze is not called.
#----------------------------------------------------------------------------
proc Solve_HandleWhenVarUpdated {args} {
entertrace
  global ascSolvStatVect ascSolvVect
  if {![slv_checksys]} {
    leavetrace slv_checksys
    return
  }
#
# if no simname sent, flush solver because we may be wrong.
#
  if {$args == ""} {
    if {$ascSolvStatVect(running)} {
      set ascSolvStatVect(menubreak) 1
      slv_set_haltflag 1
      Solve_AppendQueue Solve_do_Flush
    } else {
      Solve_do_Flush
    }
    leavetrace
    return
  }
#
# Here we need to differentiate between reanalyze and check_and_reanalyze.
#
  set args [stripbraces $args]
  set inst_name [lindex $args 0]
  set simname [lindex [split $args .] 0]

  if {$simname==$ascSolvVect(simname)} {
    if {$ascSolvStatVect(running)} {
      set ascSolvStatVect(menubreak) 1
      slv_set_haltflag 1
      Solve_AppendQueue slv_reanalyze
    } else {
      slv_reanalyze
    }
    Solve_When_Updated $ascSolvVect(instname)
  }
leavetrace end
}

#
# proc Solve_HandleVariableUpdated {args}
#----------------------------------------------------------------------------
# If applicable, redo presolve. scaling/bounds may have changed.
# In general, solvers have scratch copies of variable values internally and
# will stomp on interface changes made while solving.
#----------------------------------------------------------------------------
proc Solve_HandleVariableUpdated {args} {
entertrace
  if {![slv_checksys]} {
    leavetrace
    return
  }

leavetrace
}
#
#----------------------------------------------------------------------------
# routines that should be in other files, or obsolete once a solver         #
# import protocol is established.                                           #
#----------------------------------------------------------------------------
#

#
# proc Brow_do_Export2Solver {}
#----------------------------------------------------------------------------
# Browser export to Solver button
# Currently allows the exporting a MODEL_INST or an
# ARRAY of MODEL_INSTs.
#
# leavetrace; returns 0 if ok, 1 if not
#----------------------------------------------------------------------------
proc Brow_do_Export2Solver {} {
entertrace
  set qlfdid [Brow_get_subname]
  if {[inst kind]=="MODEL_INST"} {
    leavetrace
    return [Solve_Import_Any $qlfdid]
  }
  Script_Raise_Alert "Instance is not MODEL. Not exported."
  leavetrace
    return 1
leavetrace
}

# TEMPORARY INTERFACE HACK #
# the following functions are working towards using names
# instead of numbers.  These functions should be renamed
# and used where appropriate

#
# proc Solve_get_SolvernameCur {}
#----------------------------------------------------------------------------
# Returns the name of the current solver                                    #
#----------------------------------------------------------------------------
proc Solve_do_SolverCur {} {
entertrace
return [lindex [slv_available] [slv_get_solver]]
}

#
# proc Solve_set_SolverRegisteredNumbers {}
#----------------------------------------------------------------------------
# Fills ascsolvVect(solvername) with registered solver numbers              #
# For unregistered solvers ascSolvVect(solvername) = -1                     #
# This function should be called after initial registration process and     #
# after any subsequent dynamic solver registrations.                        #
#----------------------------------------------------------------------------
proc Solve_set_SolverRegisteredNumbers {} {
global ascSolvVect
global SolverNames
entertrace
    foreach name $SolverNames {
        set ascSolvVect($name) -1
    }
    set available [slv_available]
    set registered_number 0
    foreach name $available {
        set ascSolvVect($name)  $registered_number
        incr registered_number
    }
    set ascSolvVect(General) 32767
leavetrace
}

#################################################################
# monitor functions
# set ascSolv32767Vect(monitor) 0
# to disable

global ascSolv32767Vect
if {![info exists  ascSolv32767Vect(update_frequency)]} {
  set ascSolv32767Vect(update_frequency) 1
}
# valid types are value, speed, (both for vars) and residual.
if {![info exists  ascSolv32767Vect(monitor.type)]} {
  set ascSolv32767Vect(monitor.type) residual
}

proc Solve_monitor_init {} {
  global ascSolv32767Vect
  catch {destroy .monitor}
  catch {destroy .monitor.c}
  if {!$ascSolv32767Vect(monitor)} {return}
  if {$ascSolv32767Vect(update_frequency) > 1} {
     puts stderr "update_frequency: $ascSolv32767Vect(update_frequency)"
  }
  toplevel .monitor
  wm withdraw .monitor
  canvas .monitor.c

  set c .monitor.c
  bind $c <3> "Solve_itemMark $c %x %y"
  bind $c <B3-Motion> "Solve_itemStroke $c %x %y"
  bind $c <1> "Solve_itemsUnderArea $c"

  pack append .monitor .monitor.c {top frame center expand fill}
  wm geometry .monitor 600x400+20+20
  # split the canvas . would be nicer if we also 
  # grid the canvas to show integer values in the y axis
  .monitor.c create line 0 200 600 200 -fill red
  wm deiconify .monitor

  # nuke an old monitor. Note this is unnecessary, since the
  # monitor detects when the problem changes. Once things are
  # firmed up, we should just create 1 at startup and destroy it
  # at shutdown.
  catch {$ascSolv32767Vect(monitor.id) destroy}
  set ascSolv32767Vect(monitor.id) [slv_monitor]
  $ascSolv32767Vect(monitor.id) foo geometry 580 380 10 10 1e-3 1e3 2
  puts "started $ascSolv32767Vect(monitor.id) $ascSolv32767Vect(monitor.type)"
  set ascSolv32767Vect(X1) 0
  set ascSolv32767Vect(Y1) 0
  set ascSolv32767Vect(X2) 0
  set ascSolv32767Vect(Y2) 0
}

# kills any outstanding monitors.
proc Solve_destroy_monitors {} {
  foreach i [info comm slv_monitor*] {
    catch {$i destroy}
  }
}

# button 3 down delete old rectangle
proc Solve_itemMark {c x y} {
    global ascSolv32767Vect
    set ascSolv32767Vect(X1) [$c canvasx $x]
    set ascSolv32767Vect(Y1) [$c canvasy $y]
    $c delete area
}
#update rectangle button 3 drag
proc Solve_itemStroke {c x y} {
    global ascSolv32767Vect
    set x [$c canvasx $x]
    set y [$c canvasy $y]
    if {($ascSolv32767Vect(X1) != $x) && ($ascSolv32767Vect(Y1) != $y)} {
	$c delete area; #kill old box
	$c addtag area withtag \
           [$c create rect $ascSolv32767Vect(X1) $ascSolv32767Vect(Y1) $x $y \
		-outline black] ;# make and tag new box
	set ascSolv32767Vect(X2) $x
	set ascSolv32767Vect(Y2) $y
    }
}

# this function should return a listbox instead of going to stdout
proc Solve_itemsUnderArea {c} {
    global ascSolv32767Vect
    set area [$c find withtag area]
    set items ""
    foreach i [$c find enclosed $ascSolv32767Vect(X1) $ascSolv32767Vect(Y1) \
      $ascSolv32767Vect(X2) $ascSolv32767Vect(Y2)] {
        lappend items [$c gettags $i]
    }
    set rellist ""
    foreach i $items {
      foreach j $i {
        if {[string match v* $j]} {
          lappend rellist [string range $j 1 end]
        }
      }
    }
    if {[string index $ascSolv32767Vect(monitor.type) 0] =="r"} {
      puts stdout "Relations enclosed by area: $items"
      foreach i $rellist {
        dbg_write_rel 0 $i 1
      }
    } else {
      puts stdout "Variables enclosed by area: $items"
      foreach i $rellist {
        dbg_write_var 0 $i 1 0
      }
    }
}

# this redraws those bitmaps which have changed since the last call
# to plotdata
# see the tk man page for canvas
proc Solve_update_monitor {} {
  global ascSolv32767Vect

  if {!$ascSolv32767Vect(monitor)} { 
    # in this if, more intelligence could be updated, like checking
    # for and ignoring singletons or the end of the solution sequence. 
    return
  }
  set list [$ascSolv32767Vect(monitor.id) foo plotdata \
             $ascSolv32767Vect(monitor.type)]
  foreach t $list {
    catch {.monitor.c delete v[lindex $t 2]} 
    # asc_sq_3 is a 3x3 pixel square, could be another
    .monitor.c create bitmap [lindex $t 0] [lindex $t 1] \
      -bitmap asc_sq_3 -tags v[lindex $t 2] -foreground black
  }
}

# doesn't work, as coords does not return error when no tag match
# probably should work and may be a tk bug.
# we would like to be able to just move the existing bitmaps instead
# of deleting and recreating them as move is faster.
catch {
    if {[catch {.monitor.c coords v[lindex $t 2] \
                   [lindex $t 0] [lindex $t 1]} ]} {
       .monitor.c create bitmap [lindex $t 0] [lindex $t 1] \
        -bitmap sqr1x1_bits -tags v[lindex $t 2] -foreground black
    }
}

