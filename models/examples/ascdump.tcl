#
# Dump utilities for various things in the ascend/tcl environment.
# Ben Allan, Sept. 1996.
# The GNU License, version 2.0, applies to this file.
# See the LICENSE file elsewhere in the ascend system.
# $Revision: 1.1 $
# $Date: 1996/09/24 18:18:00 $
#
# info comm dump* to see what functions are defined in this file.
#
# proc dumpblockvars {}
#----------------------------------------------------------------------------
# dump vars rels in block to file stolen from Debug_do_BlkVarVal
#----------------------------------------------------------------------------
proc dumpblockvars {file} {
  global ascDebuVect ascSolvVect
  if {$ascDebuVect(blkcur)!=""} {
    set part [dbg_get_varpartition]
    set parts [split $part /]
    set vars  [lindex $parts $ascDebuVect(blkcur)]
    set fid [open $file w]
    foreach i $vars {
      puts $fid "<$i> [dbg_write_var 2 $i 1]"
    }
    close $fid
  }
}

#
# proc dumpblockrels {}
#----------------------------------------------------------------------------
# dump rels rels in block to file stolen from Debug_do_BlkVarVal
#----------------------------------------------------------------------------
proc dumpblockrels {file} {
  global ascDebuVect ascSolvVect
  if {$ascDebuVect(blkcur)!=""} {
    set part [dbg_get_eqnpartition]
    set parts [split $part /]
    set vars  [lindex $parts $ascDebuVect(blkcur)]
    set fid [open $file w]
    foreach i $vars {
      puts $fid "<$i> [dbg_write_rel 2 $i 0]"
    }
    close $fid
  }
}
#
# proc dumpblocks {}
#----------------------------------------------------------------------------
# Compute the sum and max of block residuals.
# Compute the sum and max of block times.
# Compute the sum of function and jacobian times.
# Print the data for nonsingleton blocks
# Mostly stolen from Solve_Check_Block_Errs in SolverProc.tcl
# from get_slv_cost_page
#Each list element is:        *
# 0 block size
# 1 iterations in block
# 2 function calls  na
# 3 jacobian calls  na
# 4 cpu time
# 5 residual
# 6 function time
# 7 jacobian time
#----------------------------------------------------------------------------
proc dumpblocks {} {
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
  set gscp [get_slv_cost_page]
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
      if {[lindex $i 0] >1} {
        incr itb
        puts stdout "block $ct: size= [lindex $i 0], its = [lindex $i 1], \
time= [lindex $i 4], resid= [lindex $i 5], fc= [lindex $i 2], \
jc= [lindex $i 3], ft = [lindex $i 6], jt = [lindex $i 7]"
      }
      incr ct
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
  if {1} {
    puts "Block error total: $sumbe"
    puts "Block error max($maxbeblock): $maxbe"
    puts "CPU total: $ascSolvStatVect(cpuelapsed)"
    puts "Expensive block($maxbtblock): $maxbt"
    puts "Presolve: $pst"
    puts "Functions: $sumft"
    puts "Derivatives: $sumjt"
  }
}

#
# proc dumparray {}
#----------------------------------------------------------------------------
# dumps the contents of an array in alphabetical order given the name
# of a tcl array variable.
#----------------------------------------------------------------------------
proc dumparray {n} {
  d_dumpary $n
}
