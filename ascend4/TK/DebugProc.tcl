#  DebugProc.tcl: Solver Debugger Tcl procedures
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.10 $
#  Last modified on: $Date: 1998/06/18 15:54:41 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: DebugProc.tcl,v $
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
# File structure:
# default set procedure
# button direct callbacks
# button internals
# utility routines
# routines that should be in other files
# re-sourceing this file while debugger up may lead to duplicate traces.
#
# set_Debug_Defaults {}
#----------------------------------------------------------------------------
# init vars for debugger window
#----------------------------------------------------------------------------
proc set_Debug_Defaults {} {
# puts "setting debugger buttons"
  global ascDebuVect ascSolvVect
  set ascDebuVect(eqncur) 0
  set ascDebuVect(varcur) 0
  set ascDebuVect(blkcur) 0
  set ascDebuVect(eqnmax) 0
  set ascDebuVect(varmax) 0
  set ascDebuVect(blkmax) 0
  set ascDebuVect(blksizes) "Rows:"
  set ascDebuVect(visible) 1
  set ascDebuVect(windowname) .debug
  Debug_font_configure
}
proc Debug_font_configure {} {
  global ascDebuVect ascSolvVect
  if {[lsearch [font names] $ascDebuVect(font)]  != -1} {
    set fa [font actual $ascSolvVect(font)]
    font configure $ascDebuVect(font) \
      -family [lindex $fa 1] \
      -size   [lindex $fa 3] \
      -weight [lindex $fa 5] \
      -slant  [lindex $fa 7] \
      -underline   [lindex $fa 9] \
      -overstrike  [lindex $fa 11]
  }
}

#
# proc Debug_do_VarName {}
#----------------------------------------------------------------------------
# display focused variable name and value{dims} to stderr                   #
#----------------------------------------------------------------------------
proc Debug_do_VarName {} {
  global ascDebuVect ascSolvVect
  if {$ascDebuVect(varcur)!=""} {
    dbg_write_var 1 $ascDebuVect(varcur) 2 0 [slv_get_pathname]
  }
}

#
# proc Debug_do_VarAttr {}
#----------------------------------------------------------------------------
# display atomtype, dims, vaseetype, val, indexnum, qualified parent name,  #
# and atom parts to stderr                                                  #
#----------------------------------------------------------------------------
proc Debug_do_VarAttr {} {
  global ascDebuVect ascSolvVect
  if {$ascDebuVect(varcur)!=""} {
    puts [dbg_write_varattr $ascDebuVect(varcur)]
    puts \n
  }
}

#
# proc Debug_do_Var2Browser {}
#----------------------------------------------------------------------------
# export simulation and variable to browser.                                #
#----------------------------------------------------------------------------
proc Debug_do_Var2Browser {} {
  global ascDebuVect ascSolvVect
  if {$ascDebuVect(varcur)!=""} {
    set tname [dbg_write_var 2 $ascDebuVect(varcur) 0 0]
    regsub \{ $tname "" tname
    regsub \} $tname "" tname
    set tname [slv_get_pathname].$tname
    Brow_Export_Any_2Browser $tname
  }
}

#
# proc Debug_do_Var2Probe {}
#----------------------------------------------------------------------------
# export variable to probe.                                                 #
#----------------------------------------------------------------------------
proc Debug_do_Var2Probe {} {
  global ascDebuVect ascSolvVect
  if {$ascDebuVect(varcur)!=""} {
    set tname [dbg_write_var 2 $ascDebuVect(varcur) 0 0]
    regsub \{ $tname "" tname
    regsub \} $tname "" tname
    set tname [slv_get_pathname].$tname
    PROBE ONE $tname
  }
}

#
# proc Debug_do_EqnName {}
#----------------------------------------------------------------------------
# display focused eqn name and resid to stderr                              #
#----------------------------------------------------------------------------
proc Debug_do_EqnName {} {
  global ascDebuVect ascSolvVect
  if {$ascDebuVect(eqncur)!=""} {
    dbg_write_rel 1 $ascDebuVect(eqncur) 2 [slv_get_pathname]
  }
}
#
# proc Debug_do_EqnAttr {}
#----------------------------------------------------------------------------
# display relation indexnum, eqn as infix, residual, include flag to stderr #
#----------------------------------------------------------------------------
proc Debug_do_EqnAttr {} {
  global ascDebuVect ascSolvVect
  if {$ascDebuVect(eqncur)!=""} {
    dbg_write_rel 1 $ascDebuVect(eqncur) 4
    dbg_write_rel 1 $ascDebuVect(eqncur) 2 [slv_get_pathname]
  }
}

#
# proc Debug_do_Eqn2Browser {}
#----------------------------------------------------------------------------
# export simulation and equation to browser.                                #
#----------------------------------------------------------------------------
proc Debug_do_Eqn2Browser {} {
  global ascDebuVect ascSolvVect
  if {$ascDebuVect(eqncur)!=""} {
    set tname [dbg_write_rel 2  $ascDebuVect(eqncur)  0]
    regsub \{ $tname "" tname
    regsub \} $tname "" tname
    set tname [slv_get_pathname].$tname
    Brow_Export_Any_2Browser $tname
  }
}

#
# proc Debug_do_Eqn2Probe {}
#----------------------------------------------------------------------------
# export equation to browser.                                               #
#----------------------------------------------------------------------------
proc Debug_do_Eqn2Probe {} {
  global ascDebuVect ascSolvVect
  if {$ascDebuVect(eqncur)!=""} {
    set tname [dbg_write_rel 2  $ascDebuVect(eqncur)  0]
    regsub \{ $tname "" tname
    regsub \} $tname "" tname
    set tname [slv_get_pathname].$tname
    PROBE ONE $tname
  }
}

#
# proc Debug_do_BlkSize {} {
#----------------------------------------------------------------------------
# display block num of vars/rels to stderr                                  #
# blocks of fixed variables generally have no relations                     #
#----------------------------------------------------------------------------
proc Debug_do_BlkSize {} {
  global ascDebuVect
  if {$ascDebuVect(blkcur) != ""} {
    set part [dbg_get_varpartition]
    set parts [split $part /]
    set size [llength [lindex $parts $ascDebuVect(blkcur)]]
    set ascDebuVect(blksizes) "Cols: $size"
 #   puts stderr "There are $size variables in block $ascDebuVect(blkcur)."
    set part [dbg_get_eqnpartition]
    set parts [split $part /]
    set size [llength [lindex $parts $ascDebuVect(blkcur)]]
    set ascDebuVect(blksizes) "Rows: $size $ascDebuVect(blksizes)"
 #   puts stderr "There are $size equations in block $ascDebuVect(blkcur)."
  } else {
    set ascDebuVect(blksizes) "Rows: 0 Cols: 0"
  }
}

#
# proc Debug_do_BlkVarVal {}
#----------------------------------------------------------------------------
# name/val/dims of vars in block to stderr                                  #
#----------------------------------------------------------------------------
proc Debug_do_BlkVarVal {} {
  global ascDebuVect ascSolvVect
  if {$ascDebuVect(blkcur)!=""} {
    set part [dbg_get_varpartition]
    set parts [split $part /]
    set vars  [lindex $parts $ascDebuVect(blkcur)]
    set pname [slv_get_pathname]
    foreach i $vars {dbg_write_var 1 $i 3 0 $pname}
  }
}
#
# proc Debug_do_BlkVarAttr {}
#----------------------------------------------------------------------------
# display atomtype, dims, vaseetype, val, indexnum, qualified parent name,  #
# and atom parts to stderr for all vars in block                            #
#----------------------------------------------------------------------------
proc Debug_do_BlkVarAttr {} {
  global ascDebuVect ascSolvVect
  if {$ascDebuVect(blkcur)!=""} {
    set part [dbg_get_varpartition]
    set parts [split $part /]
    set vars  [lindex $parts $ascDebuVect(blkcur)]
    foreach i $vars {puts [dbg_write_varattr $i]\n}
  }
}

#
# proc Debug_do_BlkVar2Probe {} {
#----------------------------------------------------------------------------
# export vars in block to probe                                             #
#----------------------------------------------------------------------------
proc Debug_do_BlkVar2Probe {} {
  global ascDebuVect ascSolvVect
  if {$ascDebuVect(blkcur)!=""} {
    set part [dbg_get_varpartition]
    set parts [split $part /]
    set vars  [lindex $parts $ascDebuVect(blkcur)]
    set pname [slv_get_pathname]
    foreach i $vars {
      set tname [lindex [dbg_write_var 2 $i 0 0] 0]
      set tname $pname.$tname
      PROBE current $tname
    }
  }
}
#
# proc Debug_do_BlkEqnResids {} {
#----------------------------------------------------------------------------
# display focused eqn name and resid to stderr for eqns in block            #
#----------------------------------------------------------------------------
proc Debug_do_BlkEqnResids {} {
  global ascDebuVect ascSolvVect
  if {$ascDebuVect(blkcur)!=""} then {
    set part [dbg_get_eqnpartition]
    set parts [split $part /]
    set rels  [lindex $parts $ascDebuVect(blkcur)]
    set pname [slv_get_pathname]
    foreach i $rels {
      dbg_write_rel 1 $i 2 $pname
    }
  }
  return 0
}

#
# proc Debug_do_BlkEqnAttr {} {
#----------------------------------------------------------------------------
# display relation indexnum, eqn as infix, residual, include flag to stderr #
# for all equations in block                                                #
#----------------------------------------------------------------------------
proc Debug_do_BlkEqnAttr {} {
  global ascDebuVect ascSolvVect
  if {$ascDebuVect(blkcur)!=""} {
    set part [dbg_get_eqnpartition]
    set parts [split $part /]
    set rels  [lindex $parts $ascDebuVect(blkcur)]
    set pname [slv_get_pathname]
    foreach i $rels {
      puts stderr "-------------------- RELATION $i ------------------------"
      dbg_write_rel 1 $i 2 $pname
      dbg_write_rel 1 $i 4
    }
  }
}

#
# proc Debug_do_BlkEqnDep {} {
#----------------------------------------------------------------------------
# run numeric dependency check on block in focus                            #
#----------------------------------------------------------------------------
proc Debug_do_BlkEqnDep {} {
  puts Debug_do_BlkEqnDep
}

#
# proc Debug_do_BlkEqn2Probe {} {
#----------------------------------------------------------------------------
# export eqns in block to probe                                             #
#----------------------------------------------------------------------------
proc Debug_do_BlkEqn2Probe {} {
  global ascDebuVect ascSolvVect
  if {$ascDebuVect(blkcur)!=""} {
    set part [dbg_get_eqnpartition]
    set parts [split $part /]
    set rels  [lindex $parts $ascDebuVect(blkcur)]
    set pname [slv_get_pathname]
    foreach i $rels {
      set tname [lindex [dbg_write_rel 2  $i  0] 0]
      set tname $pname.$tname
      PROBE current $tname
    }
  }
}
#
# proc Debug_do_Blk2Probe {} {
#----------------------------------------------------------------------------
# export eqns and vars in block to probe                                    #
#----------------------------------------------------------------------------
proc Debug_do_Blk2Probe {} {
  Debug_do_BlkEqn2Probe
  Debug_do_BlkVar2Probe
}
#
# proc Debug_do_SysVarVal {} {
#----------------------------------------------------------------------------
# print free & incident var list to the screen                              #
#----------------------------------------------------------------------------
proc Debug_do_SysVarVal {} {
  global ascDebuVect ascSolvVect
  set vars  [dbg_list_vars 5]
  set pname [slv_get_pathname]
  foreach i $vars {dbg_write_var 1 $i 3 0 $pname}
}
#
# proc Debug_do_SysVarAttr {} {
#----------------------------------------------------------------------------
# print attributes for all (partitioned) var list to the screen             #
#----------------------------------------------------------------------------
proc Debug_do_SysVarAttr {} {
  global ascDebuVect ascSolvVect
  puts Debug_do_SysVarAttr
}

#
# proc Debug_do_SysVar2Nom {} {
#----------------------------------------------------------------------------
# Reset all free incident variables to their nominal values                 #
#----------------------------------------------------------------------------
proc Debug_do_SysVar2Nom {} {
  global ascDebuVect ascSolvVect
  puts stderr "Resetting all solver variables to their nominal values."
  var_free2nom
  HUB_Message_to_HUB VARIABLEUPDATED $ascSolvVect(simname)
}

#
# proc Debug_do_SysNom2Var {} {
#----------------------------------------------------------------------------
# Reset all free incident variable nominals to their var's current values   #
#----------------------------------------------------------------------------
proc Debug_do_SysNom2Var {} {
  global ascDebuVect ascSolvVect
  puts stderr \
   "Resetting all solver variable nominals to current variable values."
  var_nom2free
  HUB_Message_to_HUB VARIABLEUPDATED $ascSolvVect(simname)
}


#
# proc Debug_do_Sys2Probe {} {
#----------------------------------------------------------------------------
# export entire var list/eqn list of the instance to the probe              #
# this isn't necessarily in solver index or master index order. it may
# coincidentally be in master index order.
#----------------------------------------------------------------------------
proc Debug_do_Sys2Probe {} {
  global ascSolvVect
  set pname [slv_get_pathname]
  # reals, possibly not all solver_var.
  PROBE current $pname {0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0}
  # real relations
  PROBE current $pname {1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0}
  # integers, possibly not all in whens
  PROBE current $pname {0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0}
  # booleans, possibly not all in whens
  PROBE current $pname {0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0}
  # logical relations
  PROBE current $pname {0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0}
}
#
# proc Debug_do_OK {} {
#----------------------------------------------------------------------------
# close up the debugger                                                     #
#----------------------------------------------------------------------------
proc Debug_do_OK {} {
  View_Save_SpecialWindow_Values debugger
  Debug_Trace off
  Solve_CloseDebugger
}

#
# proc Debug_do_Help {} {
#----------------------------------------------------------------------------
# run global help on the debugger                                           #
#----------------------------------------------------------------------------
proc Debug_do_Help {} {
  Debug_Help_Operations
}

#
# Debug_Help_Operations {}
#----------------------------------------------------------------------------
# help button for ascend debugger window
#----------------------------------------------------------------------------
proc Debug_Help_Operations {} {
  puts "==================================================="
  puts "Here are some of the C calls :"
  dbghelp l
  puts "==================================================="
  puts "Here are some of the tcl calls :"
  set tmp [info procs Debu*]
  d_dumpproclist $tmp
  puts "==================================================="
  Help_button solver.debugger.intro

}

#
# proc Debug_Trace {state}
#----------------------------------------------------------------------------
# set debugger limits,traces for dialogue control                           #
# note that blocks, rels, vars are numbered 0 to max-1 in C                 #
#----------------------------------------------------------------------------
proc Debug_Trace {state} {
  global ascSolvVect ascSolvStatVect ascDebuVect
  set ascDebuVect(eqncur)    ""
  set ascDebuVect(varcur)    ""
  set ascDebuVect(blkcur)    ""
  set ascDebuVect(varlast)   ""
  set ascDebuVect(eqnlast)   ""
  set ascDebuVect(blklast)   ""
 # set traces if on, else unset traces, 0 maxima
  if {$state == "on"} {
    Solve_Update_StatVect
    set ascDebuVect(eqnmax) $ascSolvStatVect(rels)
    set ascDebuVect(varmax) $ascSolvStatVect(vars)
    set ascDebuVect(blkmax) $ascSolvStatVect(block.number)
    trace variable ascDebuVect(varcur) w Debug_Var_Trace
    trace variable ascDebuVect(eqncur) w Debug_Eqn_Trace
    trace variable ascDebuVect(blkcur) w Debug_Blk_Trace
  } else {
    set ascDebuVect(eqnmax)    0
    set ascDebuVect(varmax)    0
    set ascDebuVect(blkmax)    0
    trace vdelete ascDebuVect(varcur) w Debug_Var_Trace
    trace vdelete ascDebuVect(eqncur) w Debug_Eqn_Trace
    trace vdelete ascDebuVect(blkcur) w Debug_Blk_Trace
  }
}

#
# Debug_Blk_Trace {n1 n2 mode}
#----------------------------------------------------------------------------
# trace action for the Block entry. called on blkcur write                  #
#----------------------------------------------------------------------------
proc Debug_Blk_Trace {n1 n2 mode} {
  global ascDebuVect
 #  puts stderr "Debug_Blk_Trace called."
  Debug_do_BlkSize
  if {[focus] == ".debug.entry_blk"} {
    set ascDebuVect(eqncur)   ""
    set ascDebuVect(varcur)   ""
    set ascDebuVect(varlast)  ""
    set ascDebuVect(eqnlast)  ""
    if {[scan $ascDebuVect(blkcur) %d tmpcur] == "1"} {
      if {$tmpcur<0 || $tmpcur >= $ascDebuVect(blkmax)} {
        puts stderr "!!Block specified not within system!!"
        set ascDebuVect(blkcur) $ascDebuVect(blklast)
      } else {
        .debug.entry_blk delete 0 end
        .debug.entry_blk insert 0 $tmpcur
        set ascDebuVect(blkcur) $tmpcur
        set ascDebuVect(blklast) $tmpcur
      }
    } else {
      if {$ascDebuVect(blkcur)==""} {
        set ascDebuVect(eqnlast) ""
      } else {
        puts stderr "!!Illegal block specification!!"
        set ascDebuVect(blkcur) $ascDebuVect(blklast)
      }
    }
  }
}
#
# Debug_Eqn_Trace {n1 n2 mode}
#----------------------------------------------------------------------------
# trace action for the eqn entry. called on eqncur write                    #
#----------------------------------------------------------------------------
proc Debug_Eqn_Trace {n1 n2 mode} {
  global ascDebuVect
 #  puts stderr "Debug_Eqn_Trace called"
  if {[focus] == ".debug.entry_eqn"} {
    set ascDebuVect(varcur) ""
    set ascDebuVect(varlast) ""
    if {[scan $ascDebuVect(eqncur) %d tmpcur] == "1"} {
      if {$tmpcur <0 || $tmpcur >= $ascDebuVect(eqnmax)} {
        puts stderr "!!Equation specified does not exist!!"
        set ascDebuVect(eqncur) $ascDebuVect(eqnlast)
      } else {
        .debug.entry_eqn delete 0 end
        .debug.entry_eqn insert 0 $tmpcur
        set ascDebuVect(eqncur) $tmpcur
        set ascDebuVect(eqnlast) $tmpcur
        set tmpblk [ dbg_get_blk_of_eqn $tmpcur ]
        if {$tmpblk == "none"} {
          set ascDebuVect(blkcur)  ""
 #         puts stderr "Equation specified is not assigned to a partition"
        } else {
          set ascDebuVect(blkcur) $tmpblk
        }
        set ascDebuVect(blklast) $ascDebuVect(blkcur)
      }
    } else {
      if {$ascDebuVect(eqncur)==""} {
        set ascDebuVect(eqnlast) ""
      } else {
        puts stderr "!!Illegal equation specification!!"
        set ascDebuVect(eqncur) $ascDebuVect(eqnlast)
      }
    }
  }
}

#
# Debug_Var_Trace {n1 n2 mode}
#----------------------------------------------------------------------------
# trace action for the var entry. called on varcur write                    #
#----------------------------------------------------------------------------
proc Debug_Var_Trace {n1 n2 mode} {
  global ascDebuVect
  if {[focus] == ".debug.entry_var"} {
    if {[scan $ascDebuVect(varcur) %d tmpcur] == "1"} {
      if {$tmpcur<0 || $tmpcur >= $ascDebuVect(varmax)} {
        puts stderr "!!Variable specified does not exist!!"
        set ascDebuVect(varcur) $ascDebuVect(varlast)
      } else {
        .debug.entry_var delete 0 end
        .debug.entry_var insert 0 $tmpcur
        set ascDebuVect(varcur) $tmpcur
        set ascDebuVect(varlast) $tmpcur
 #        set tmpeqn [ dbg_get_eqn_of_var $tmpcur ]
 # temporary patch to avoid crash. need to get linsol semantics
 # from joe.
        set tmpeqn none
        set ascDebuVect(blkcur)  ""
        if {$tmpeqn == "none"} {
          set ascDebuVect(eqncur)  ""
        } else {
          set ascDebuVect(eqncur)  $tmpeqn
        }
        set tmpblk [ dbg_get_blk_of_var $tmpcur ]
        if {$tmpblk == "none"} {
          set ascDebuVect(blkcur)  ""
 #         puts stderr "Variable not assigned to a partition."
        } else {
          set ascDebuVect(blkcur) $tmpblk
        }
        set ascDebuVect(eqnlast) $ascDebuVect(eqncur)
        set ascDebuVect(blklast) $ascDebuVect(blkcur)
      }
    } else {
      if {$ascDebuVect(varcur)==""} {
        set ascDebuVect(varlast) ""
      } else {
        puts stderr "!!Illegal variable specification!!"
        set ascDebuVect(varcur) $ascDebuVect(varlast)
      }
    }
  }
}

#
# proc uinfo {type} {
#----------------------------------------------------------------------------
# procedure to tell about various commands origins                          #
# assumes tcl/tk 7.1/3.4 and computes which user defined commands are       #
# C calls and which are TCL procedures                                      #
# excludes widget names (C calls starting with dots) from user C call list  #
# all variables are set after the first call to unifo.
# bugs: fucking slow
# These 3 vars are lists of the predefined procedures, C calls, and both
# due to TCL
# global asctcl_procbase
# global asctcl_Cbase
# global asctcl_base
# These 3 vars are lists of the predefined procedures, C calls, and both
# due to TK
# global asctk_procbase
# global asctk_Cbase
# global asctk_base
# These 3 vars are lists of the predefined procedures, C calls, and both
# due to TCL+TK
# global asc_procbase
# global asc_Cbase
# global asc_base
# These 3 vars are lists of the predefined procedures, C calls, and both
# due to ASCEND. They are computed by this procedure.
# global asc_userproc
# global asc_userC
# global asc_user
# C calls that are widgets (starting with .) are segregated, though
# global asc_userwidgets
#----------------------------------------------------------------------------
proc uinfo {type} {
  global asctcl_procbase
  global asctcl_Cbase
  global asctcl_base
  global asctk_procbase
  global asctk_Cbase
  global asctk_base
  global asc_procbase
  global asc_Cbase
  global asc_base
  global asc_userproc
  global asc_userC
  global asc_user
  global asc_userwidgets

  set asctcl_procbase "unknown auto_execok auto_mkindex auto_reset auto_load"

  set asctcl_Cbase [list \
     tell open eof pwd glob list pid exec time eval lrange lsearch \
     gets lappend proc break llength return linsert error catch info \
     split array if concat join lreplace source global switch close for \
     cd file append format read set scan trace seek while flush continue \
     uplevel foreach rename regexp upvar expr unset regsub history exit \
     puts incr lindex lsort string \
  ]

  set asctk_procbase [list \
    tk_nextMenuEntry tk_bindForTraversal tk_menuBar tk_getMenuButtons \
    tk_traverseToMenu tk_firstMenu tk_menus tk_invokeMenu tkMenuButtonDown \
    tk_nextMenu tk_mbUnpost tk_traverseWithinMenu tk_mbPost 
  ]

  set asctk_Cbase [list menubutton listbox pack checkbutton canvas \
    message place bind tkwait raise option toplevel update wm scale \
    label focus radiobutton button lower destroy after winfo scrollbar \
    grab menu . selection entry send frame text tk \
  ]

  set asctcl_base "$asctcl_procbase $asctcl_Cbase"
  set asctk_base "$asctk_procbase $asctk_Cbase"

  set asc_Cbase "$asctcl_Cbase $asctk_Cbase"
  set asc_procbase "$asctcl_procbase $asctk_procbase"
  set asc_base "$asc_procbase $asc_Cbase"

  puts stderr "collecting widget names..."
  set asc_userwidgets [info commands .*]

  puts stderr "collecting user procedure names..."
  set asc_proc [info procs]
  set asc_userproc ""
  foreach i $asc_proc {
    if {[lsearch $asc_procbase $i]=="-1"} {
      lappend asc_userproc $i
    }
  }
  unset asc_proc

  puts stderr "collecting user C call names..."
  set asc_comm [info commands]
  set asc_proc "$asc_userproc $asc_base $asc_userwidgets"
  set asc_userC ""
  foreach i $asc_comm {
    if {[lsearch $asc_proc $i]=="-1"} {
      lappend asc_userC $i
    }
  }

  unset asc_comm
  unset asc_proc

  set asc_user "$asc_userC $asc_userproc"
  switch $type {

   {p} { puts "======= user defined TCL procedures  ========"
         puts $asc_userproc }
   {c} { puts "======= user defined C calls ========"
         puts $asc_userC }
   {w} { puts "==== window names ===="
         puts $asc_userwidgets }

   {pa} { puts "==== user defined TCL procedures (alphabetizing) ===="
          d_dumpproclist $asc_userproc }
   {ca}  { puts "==== user defined C calls (alphabetizing) ===="
           d_dumplist $asc_userC }
   {wa } { puts "==== window names (alphabetizing) ===="
           d_dumplist $asc_userwidgets }
   {help} {puts "uinfo sets the following global list variables:"
           puts "(tcl/tk 7.1/3.4 assumed)"
           puts "asctcl_procbase asctcl_Cbase asctcl_base"
           puts "asctk_procbase asctk_Cbase asctk_base"
           puts "asc_procbase asc_Cbase asc_base"
           puts "asc_userproc asc_userC asc_user asc_userwidgets"
           puts ""
           puts "uinfo will list (and optionally alphabetize)"
           puts \
            "asc_userproc (p,pa) asc_userC (c,ca) asc_userwidgets (w,wa)."}
   default {puts "global variables set. Call uinfo with: c,p,w,ca,pa,wa,help"}
  }
}
