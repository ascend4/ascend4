#  ascStudy.tcl: Script for handling case studies
#  By Benjamin Allan
#  Created: January 1998
#  Part of ASCEND
#  Revision: $Revision: 1.6 $
#  Last modified on: $Date: 1998/06/18 15:55:12 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: ascStudy.tcl,v $
#
#  This file is part of the ASCEND Tcl/Tk Interface.
#
#  Copyright (C) 1998 Carnegie Mellon University
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

# Script for handling case studies of the form:
# STUDY {annual_profit} {tc.conv} {tc.fs.fl1.vap_to_feed_ratio} \
# IN tfc VARYING {tc.fs.sp1.split[1] 0.1 0.2 0.3 0.4 0.5 0.6 0.7} \
# USING {QRSlv} OUTFILE study.dat ERROR stop;
# 
# This could be connected to interactivity through ascplot, and should.
# But first we script it.

proc Study_keyword {word} {
  switch [string tolower $word] {
  study -
  in -
  varying -
  cases -
  using -
  outfile -
  error {
      return 1;
    }
  default {
      return 0;
    }
  }
}

proc Study_syntax {} {
  puts stderr "syntax:\
  STUDY variable-list \\\n\tIN simulation-context \\\n\
\tVARYING parameter-variation-list \\\n\
\tUSING solver-name OUTFILE disk-file-name \\\n\
\tERROR <STOP,IGNORE,WARN>;"

  puts stderr "For example:"
  puts stderr "Variable-list could be: {annual_cost} {recycle_rate}"
  puts stderr \
    "Simulation-context could be: {myflowsheet.plant2.reactor\[4\]}"
  puts stderr "Parameter-variation-list could be items that look like:"
  puts stderr "\t{purge_rate {0.01 {mole/s}} {0.02 {mole/s}}}"
  puts stderr "Solver-name: one of the Solver names available, eg. QRSlv."
  puts stderr "Disk-File-Name could be: {c:\\temp\\mystudy.dat}"
  puts stderr \
    "ERROR specifies what you want the study to do if a case cannot solve."
  error "Parametric case study specified incorrectly"
}

# proc study
# STUDY varlist IN rootname VARYING var-range-list
# USING solvername OUTFILE pathname ERROR <STOP,IGNORE,WARN,userfunc>;
# notes:
# it's big, it's ugly, it's a parser, you expect miracles?
#
proc STUDY {args} {
  global ascStudyVect ;# for autoplot only
  # parse states:
  # 0  varlist, 1 rootname, 2 var-range, 3 solver, 4 path, 5 errhandle
  set mode 0;
  set rootname -1
  set vstart -1
  set vend -1
  set vrstart -1
  set vrend -1
  set solvername -1
  set pathname -1
  set error -1
  set counter -1
  foreach i $args {
    incr counter
    if {[Study_keyword $i]} {
      # check keyword order and change modes
      switch $mode {
      0 {
          if {$vstart < 0} {
            puts stderr \
              "STUDY keyword $i not allowed before variable list."
            Study_syntax
          }
          if {[string compare "in" [string tolower $i]] == 0} {
            set mode 1
            continue;
          }
          puts stderr "STUDY keyword $i not allowed before IN"
          Study_syntax
        }
      1 {
          if {[string compare $rootname "-1"]==0} {
            puts stderr \
              "STUDY needs a simulation or part name to search for vars."
            Study_syntax
          } 
          if {[string compare "varying" [string tolower $i]] == 0} {
            set mode 2
            continue;
          }
          puts stderr "STUDY keyword $i not allowed before VARYING"
          Study_syntax
        }
      2 {
          if {$vrstart < 0} {
            puts stderr \
              "STUDY keyword $i not allowed before parameter list."
            Study_syntax
          }
          if {[string compare "using" [string tolower $i]] == 0} {
            set mode 3
            continue;
          }
          puts stderr "STUDY keyword $i not allowed before USING"
          Study_syntax
        }
      3 {
          if {[string compare $solvername "-1"]==0} {
            puts stderr \
              "STUDY needs the name of a known solver to use before $i"
            Study_syntax
          } 
          if {[string compare "outfile" [string tolower $i]] == 0} {
            set mode 4
            continue;
          }
          puts stderr "STUDY keyword $i not allowed before OUTFILE"
          Study_syntax
        }
      4 {
          if {[string compare $pathname "-1"]==0} {
            puts stderr "STUDY needs the name of a file to use before $i"
            Study_syntax
          } 
          if {[string compare "error" [string tolower $i]] == 0} {
            set mode 5
            continue;
          }
          puts stderr "STUDY keyword $i not allowed before ERROR keyword"
          Study_syntax
        }
      default {
          puts stderr "Unrecognized or misplaced keyword $i"
          Study_syntax;
          break;
        }
      }
    } else {
      # eat an argument for the current mode.
  # 0  varlist, 1 rootname, 2 par-range, 3 solver, 4 path, 5 errhandle
      switch $mode {
      0 {
          if {$vend >= 0} {
            incr vend
          } else {
            set vstart $counter 
            set vend $counter 
          }
        }
      1 {
          if {[string compare $rootname "-1"]==0} {
            set rootname $i;
            # check instance exists, is MODEL/array of MODEL
          } else {
            puts stderr "STUDY given more than 1 rootname:"
            puts stderr "\t$rootname"
            puts stderr "\t$i"
            Study_syntax
          }
        }
      2 {
          if {$vrend >= 0} {
            incr vrend
          } else {
            set vrstart $counter 
            set vrend $counter 
          }
        }
      3 {
          if {[string compare $solvername "-1"]==0} {
            set solvername $i;
            # check solver exists
          } else {
            puts stderr "STUDY given more than 1 solver name:"
            puts stderr "\t$solvername"
            puts stderr "\t$i"
            Study_syntax
          }
        }
      4 {
          if {[string compare $pathname "-1"]==0} {
            set pathname $i;
            # check path is writable
          } else {
            puts stderr "STUDY given more than 1 disk file name:"
            puts stderr "\t$pathname"
            puts stderr "\t$i"
            puts stderr "\tPerhaps the path needs {} around it."
            Study_syntax
          }
        }
      5 {
          if {[string compare $error "-1"]==0} {
            set error $i;
            # check error is in list
          } else {
            puts stderr \
              "STUDY given more than 1 error handling specification:"
            puts stderr "\t$error"
            puts stderr "\t$i"
            Study_syntax
          }
        }
      default {
          puts stderr "Unknown STUDY parser state at $i"
          Study_syntax
        }
      }
    }
  }
  # 0  varlist, 1 rootname, 2 par-range, 3 solver, 4 path, 5 errhandle
  if {$vstart < 0} {
    puts stderr "STUDY missing output variable list"
    Study_syntax
  }
  if {[string compare $rootname "-1"]==0} {
    puts stderr "STUDY missing simulation or part context name"
    Study_syntax
  }
  if {$vrstart < 0} {
    puts stderr "STUDY missing parameter variation list"
    Study_syntax
  }
  if {[string compare $solvername "-1"]==0} {
    puts stderr "STUDY missing solver name"
    Study_syntax
  }
  if {[string compare $pathname "-1"]==0} {
    puts stderr "STUDY missing output file name"
    Study_syntax
  }
  if {[string compare $error "-1"]==0} {
    puts stderr "STUDY missing error handling specification"
    Study_syntax
  }
  # vstart -1 set vend -1 set vrstart -1 set vrend -1
  # collect vstart to vend in a userdata and vrstart to vrend also 
  # write recursive tcl function to vary over vrlist and generate
  # potentially multidimensional independent variable data sets.
  # iterate over vrend fastest, vrstart slowest, save last successful
  # solution at each level by read/write virtual for recoveries.
  # bypass ui updates except at solution or really long times.
  # pop up a little box with cases solved, cases failed and update
  # only these vars. watch ^C. if hit twice in same case, stop.
  puts stderr "STUDY $args"
  puts -nonewline "STUDY "
  puts [lrange $args $vstart $vend]
  puts stderr "IN $rootname"
  puts -nonewline "VARYING "
  puts [lrange $args $vrstart $vrend]
  puts stderr "USING $solvername"
  puts stderr "OUTFILE $pathname"
  puts stderr "ERROR $error"
  # check da solver option
  if {[lsearch [string tolower [slv_available]] \
               [string tolower $solvername]] == -1} {
    puts stderr "STUDY cannot use unknown solver $solvername"
    puts stderr "Known solvers are: [slv_available]."
    Study_syntax
  }
  # check da error option
  set handler funcdummy
  switch [string tolower $error] {
  ignore {
      set handler Study_ignore
    }
  stop {
      set handler Study_stop
    }
  warn {
      set handler Study_warn
    }
  default {
      puts stderr "STUDY ERROR option must be one of: IGNORE, WARN, STOP"
      Study_syntax
    }
  }
  # check da file option
  # could add an append option later
  if {[file exists $pathname]} {
    if {![file writable $pathname] || [file isdir $pathname]} {
      puts stderr "STUDY cannot write to file $pathname."
      Study_syntax
    }
  } else {
    if {[catch {open $pathname w+} ferr]} {
      puts stderr "STUDY cannot write to file $pathname:\n$ferr"
      Study_syntax
    } else {
      close $ferr
    }
  }
  set outlist {}
  set parlist {}
  # set up independent variables in left columns
  foreach i [lrange $args $vrstart $vrend] {
    lappend outlist $rootname.[lindex $i 0]
    lappend parlist $rootname.[lindex $i 0]
  }
  # set up dependent variables in right columns
  foreach i [lrange $args $vstart $vend] {
    lappend outlist $rootname.$i
  }
  # set a log gl_list in C land
  foreach i $outlist {
    if {[catch {qlfdid $i} errmsg]} {
      puts stderr "STUDY cannot find variable $i"
      Study_syntax
    } else {
      switch [inst kind search] {
      BOOLEAN_INST -
      REAL_INST -
      INTEGER_INST -
      SYMBOL_INST -
      SET_ATOM_INST -
      WHEN_INST -
      MODEL_INST -
      ARRAY_INT_INST -
      ARRAY_ENUM_INST {
          puts stderr "STUDY cannot monitor non-variable:\n\t$i"
          Study_syntax
        }
      BOOLEAN_ATOM_INST -
      SYMBOL_ATOM_INST {
          puts stderr "STUDY cannot monitor discrete variables yet:\n\t$i"
          return 
        }
      }
    }
  }
  # check for relevant and fixed and assignable as given
  set counter [expr $vrstart -1]
  foreach i $parlist {
    incr counter;
    qlfdid $i;
    switch [inst kind search] {
    REAL_ATOM_INST {
        # check if solvervar and fixed != TRUE -> error.
        if {[lsearch [libr_query -ancestors -type [inst type search]] \
                     solver_var] != -1 && \
            [catch {qlfdid $i.fixed} errmessage] == 0 && \
            [string compare [inst atomvalue search] "TRUE"]} {
          puts stderr "STUDY: Unfixed parameter $i. $i.fixed must be TRUE"
          return
        }
        qlfdid $i;
        set check 0
        set oldval [inst atomvalue search]
        foreach tuple [lrange [lindex $args $counter] 1 end] {
          if {[ catch {
                 qassgn2 $i [lindex $tuple 0] [lindex $tuple 1]
                } errmsg
              ]} {
            puts stderr "Unable to assign value $tuple to $i because:"
            puts stderr "\t$errmsg"
            return
          } else {
            incr check
          }
        }
        catch {qassgn2 $i [lindex $oldval 0] [lindex $oldval 1]}
        if {$check == 0} {
          puts stderr "No case values specified for parameter $i"
          Study_syntax
        }
      }
    BOOLEAN_ATOM_INST -
    INTEGER_ATOM_INST -
    SYMBOL_ATOM_INST {
        puts stderr "Incorrect parameter $i."
        puts stderr "STUDY over discrete parameters not yet supported."
        puts stderr "Fix ascStudy.tcl"
        return
      }
    BOOLEAN_INST {
        puts stderr "Incorrect parameter $i."
        puts stderr "STUDY over degrees of freedom is not supported."
        return
      }
    default {
        puts stderr "STUDY parameter $i cannot affect solution."
        return
      }
    }
  }
  # So vars/parameters/file/error and solver are ok.
  # set global array of iteration information
  # call recursive function head to set values and call solver
  # f {rootname, oblistid, args, vrstart, vrend, solvername}
  set oblist [asc_study_create_observations]
  foreach i $outlist {
    asc_study_add_observation $oblist $i
  }
  if {[catch {asc_study_observations_file $oblist $pathname} errm]} {
    puts stderr "STUDY unable to start log file $pathname"
    return;
  }
  set noplot [catch {Study_cases $oblist $rootname $solvername \
                     $vrstart $vrend \
                     $parlist $args $handler} err]
  puts stderr $err
  asc_study_destroy_observations $oblist
  if {!$noplot && $ascStudyVect(autoplot)} {
    ASCPLOT $pathname
  }
}

# args are:
# C id oblistid, rootinst of solve system, solver, index of
# parameter this call will vary over, index of last parameter,
# list of varied parameters
# argument list from the call to STUDY, error handling function.
#
global ascStudyVect

# set up study constants and message protocols, then call recursion
proc Study_cases {oblistid rootinst solver vrstart \
                  vrend parlist pardata handler} {
  global ascStudyVect ascSolv32767Vect
  set ascStudyVect(autoplot) 0
  set ascStudyVect(nextobs) 0
  set ascStudyVect(tried) 0
  set ascStudyVect(failed) 0
  set ascStudyVect(lastsolution) study_virtual_file
  if {[__userdata_query exists $ascStudyVect(lastsolution)]} {
    __userdata_destroy one $ascStudyVect(lastsolution)
  }
  # most times to repush the solve button magically
  set ascStudyVect(retrymax) 5
  # if writeset 0, does not write line of junk at END of solution attempt
  set ascStudyVect(writeset) 1
  set ascStudyVect(vrstart) $vrstart
  set ascStudyVect(vrend) $vrend
  set counter $vrstart
  foreach i $parlist {
    set ascStudyVect($counter) $i
    incr counter
  }
  # override the general UI speed parameters
  # these should perhaps become persistent general options
  set ascStudyVect(lasttime) $ascSolv32767Vect(update_time)
  set ascStudyVect(lastfreq) $ascSolv32767Vect(update_frequency)
  set ascStudyVect(timelimit) 15
  set ascStudyVect(iterlimit) 200
  set ascSolv32767Vect(update_time) $ascStudyVect(timelimit)
  set ascSolv32767Vect(update_frequency) $ascStudyVect(iterlimit)
  # do it
  if {[catch {Study_cases_recursion $oblistid $rootinst $solver \
              $vrstart $vrend $pardata $handler} err]} {
    puts "SCR fail: $err"
    error $err
  }
  set ascSolv32767Vect(update_time) $ascStudyVect(lasttime)
  set ascSolv32767Vect(update_frequency) $ascStudyVect(lastfreq)
  return "CASE STUDIES completed"
}

# stops on interrupt, OTHERWISE, just suppresses bogus data
# output and moves back to last solution and moves back to last solution.
proc Study_ignore {root} {
  global ascSolvStatVect ascStudyVect
  if {$ascSolvStatVect(menubreak)} {
    error "USER halted case-studies"
  }
  incr ascStudyVect(failed)
  set ascStudyVect(writeset) 0
  puts stderr "Case $ascStudyVect(tried) failed. Output ignored."
  READ_VIRTUAL $ascStudyVect(lastsolution)
}

# stops on interrupt. composes a failure message, and sends it to user
# reloads last saved point. suppresses output of failure point.
# then returns for continuation of cases
proc Study_warn {root} {
  global ascSolvStatVect ascStudyVect
  set ascStudyVect(writeset) 0
  incr ascStudyVect(failed)
  if {$ascSolvStatVect(menubreak)} {
    error "USER halted case-studies"
  } else {
    set msg "Warning of unsolved case $ascStudyVect(tried). "
    append msg "Output suppressed. Parameters:"
    for {set i $ascStudyVect(vrstart); $i <= $ascStudyVect(vrend); incr i} {
      append msg "\n\t$ascStudyVect(par_$i) = $ascStudyVect($i)"
    }
    puts stderr $msg; # should be popup
  }
  # should offer a popup that lets user choose to keep last or
  # examine failure point.
  READ_VIRTUAL $ascStudyVect(lastsolution)
}

# stops on interrupt, composes a failure message, and returns it as
# an error. clears last saved point buffer, as it is assumed the
# user will want to see a failure point. this is not necessarily
# a good assumption. suppresses output of failure point.
proc Study_stop {root} {
  global ascSolvStatVect ascStudyVect
  incr ascStudyVect(failed)
  set ascStudyVect(writeset) 0
  if {$ascSolvStatVect(menubreak)} {
    error "USER halted case-studies"
  } else {
    set msg "Stopping at unsolved case $ascStudyVect(tried). Values:"
    for {set i $ascStudyVect(vrstart); $i <= $ascStudyVect(vrend); incr i} {
      append msg "\n\t$ascStudyVect(par_$i) = $ascStudyVect($i)"
    }
    error $msg
  }
  # should offer a popup that lets user choose to keep last or
  # examine failure point.
  __userdata_destroy one $ascStudyVect(lastsolution)
}

proc Study_extra_iterations {} {
  global ascStudyVect
  puts stderr "STUDY: extra iterations on case $ascStudyVect(tried)."
}

# solves all cases. iterates fastest over last parameter given.
# so VARYING {a {1} {2} {3}} {b {4} {6}} will yield output that
# looks like (if foo is the output variables being monitored)
# 1 4 foo
# 1 6 foo
# 2 4 foo
# 2 6 foo
# 3 4 foo
# 3 6 foo
#
proc Study_cases_recursion {oblistid rootinst solver currentpar \
                            lastpar pardata handler} {
  global ascStudyVect ascSolvStatVect
  # puts "SCRin: $oblistid $rootinst $solver $currentpar $lastpar"
  set i $ascStudyVect($currentpar);
  qlfdid $i;
  if {$lastpar <= $currentpar} {
    foreach tuple [lrange [lindex $pardata $lastpar] 1 end] { 
      set counter 0
      qassgn2 $i [lindex $tuple 0] [lindex $tuple 1]
      set ascStudyVect(par_$lastpar) $tuple ;# for messages
      WRITE_VIRTUAL $rootinst $ascStudyVect(lastsolution) 
      incr ascStudyVect(tried)
 # probably need a catch here.
      SOLVE $rootinst WITH $solver;
      while {$ascSolvStatVect(ready2solve) && 
             !$ascSolvStatVect(menubreak) &&
             $counter < $ascStudyVect(retrymax)} {
        incr counter
        Study_extra_iterations
        Solve_do_Solve $rootinst WITH $solver;
      }
 # should catch {RUN inst.check_self} here and include that
 # in the RUN part. make check optional and user defined?
      if {$ascSolvStatVect(converged) != 1} {
        $handler $rootinst
      }
      __userdata_destroy one $ascStudyVect(lastsolution)
      if {$ascStudyVect(writeset)} {
        asc_study_write_observation $oblistid
      } else {
        # suppressed. unsuppress for next point.
        set ascStudyVect(writeset) 1
      }
    }
  } else {
 # may need some catch/err foo here to avoid deep stack messages.
    set nextpar $currentpar
    incr nextpar
    foreach tuple [lrange [lindex $pardata $currentpar] 1 end] { 
      qassgn2 $i [lindex $tuple 0] [lindex $tuple 1]
      set ascStudyVect(par_$currentpar) $tuple ;# for messages
      Study_cases_recursion $oblistid $rootinst $solver $nextpar \
                            $lastpar $pardata $handler
    }
  }
}

# set to 0 if in ascend
if 0 {
global ascSolv32767Vect 
set ascSolv32767Vect(update_frequency) 10
set ascSolv32767Vect(update_time) 10
proc qlfdid {args} {
  puts "qlfdid $args"
}
proc inst {what where} {
  switch $what {
  type { return solver_var }
  kind { return REAL_ATOM_INST }
  atomvalue { return TRUE }
  }
}
proc libr_type_ancestors {args} {
  return solver_var
}
proc SOLVE {args} {
  global ascSolvStatVect
  set ascSolvStatVect(ready2solve) 0
  set ascSolvStatVect(converged) 1
  set ascSolvStatVect(menubreak) 0
}
proc qassgn2 {args} {
  puts "assign $args"
}
proc Solve_do_Solve {args} {
  SOLVE
}
proc slv_available {} {
  return "QRSlv CONOPT"
}
proc READ_VIRTUAL {args} {
  puts "LOAD $args"
}
proc WRITE_VIRTUAL {args} {
  puts "SAVE $args"
}
proc __userdata_destroy {args} {
  puts "destroy $args"
}
proc __userdata_query {args} {
  return 1
}
}
# set 0 when implemented in C as 
# set token [asc_study -create]
# asc_study -add $token $qlfdid
# asc_study -destroy [$token,all]
# asc_study -prolog $token $filename
# asc_study -write $token
if 1 {
set ascStudyVect(nextobs) 0
# closes output file and destroys tcl overhead
proc asc_study_destroy_observations {num} {
  global __asvdata
  close $__asvdata(fid.$num)
  catch {unset $__asvdata(fid.$num)} err
  # catch {unset $__asvdata(file.$num)} err
  # catch {unset $__asvdata(nobs.$num)} err
  # catch {unset $__asvdata(obslist.$num)} err
}
# writes a line of values to an already setup observation file
proc asc_study_write_observation {num} {
  global __asvdata
  set fid $__asvdata(fid.$num)
  foreach i $__asvdata(obslist.$num) {
    puts -nonewline $fid "\t"
    puts -nonewline $fid [lindex [lindex [u_getval $i] 0] 0]
  }
  puts $fid ""
}
# opens file and writes header. vars must be defined first.
proc asc_study_observations_file {num fname} {
  global __asvdata ascStudyVect
  set __asvdata(file.$num) $fname
  set __asvdata(fid.$num) [open $fname w+]
  set fid $__asvdata(fid.$num)
  set datevar [clock format [clock seconds] -format "%a %b %d %H:%M:%S %Y"]
  puts $fid "DATASET $datevar"
  puts $fid "Observations: (user index) (name) (units)"
  set var [lindex $__asvdata(obslist.$num) 0]
  set units [lindex [lindex [u_getval $var] 0] 1]
  puts $fid "{indvar}\t\{[lindex $__asvdata(obslist.$num) 0]\}\t\{$units\}"
  for {set i 1} {$i < $__asvdata(nobs.$num)} {incr i} {
    set var [lindex $__asvdata(obslist.$num) $i]
    set units [lindex [lindex [u_getval $var] 0] 1]
    puts $fid "\{$i\}\t\{$var\}\t\{$units\}"
  }
  puts -nonewline $fid "\tindvar"
  for {set i 1} {$i < $__asvdata(nobs.$num)} {incr i} {
    puts -nonewline $fid "\t$i"
  }
  puts $fid ""
  for {set i 0} {$i < $__asvdata(nobs.$num)} {incr i} {
    puts -nonewline $fid "\t---"
  }
  puts $fid ""
}
# adds an observation to the list studied
proc asc_study_add_observation {num var} {
  global __asvdata
  lappend __asvdata(obslist.$num) $var
  incr __asvdata(nobs.$num)
}
# sets up output overhead
proc asc_study_create_observations {args} {
  global ascStudyVect __asvdata
  incr ascStudyVect(nextobs)
  set __asvdata(fid.$ascStudyVect(nextobs)) stdout
  set __asvdata(file.$ascStudyVect(nextobs)) ""
  set __asvdata(nobs.$ascStudyVect(nextobs)) 0
  set __asvdata(obslist.$ascStudyVect(nextobs)) {}
  return $ascStudyVect(nextobs)
}
};#END dummies
