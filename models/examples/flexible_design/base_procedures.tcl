#                        base_procedures.tcl
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

#  $Date: 1997/06/09 19:50:48 $
#  $Revision: 1.2 $
#  $Author: ballan $
#  $Source: /afs/cs.cmu.edu/project/ascend/Repository/models/examples/flexible_design/base_procedures.tcl,v $


# Module: base_procedures.tcl
# Tcl version: 7.3 (Tcl/Tk/XF)
# Tk version: 3.6
# XF version: 2.3
#

# module contents
global moduleList
global autoLoadList
set moduleList(base_procedures.tcl) {IsaInt CHKSOL SOLST GET_NUMBER VAL clear_virtual_stuff CollocationTraySolve HeatBalance SolvCol1 CreepSet MultipleMeetSpec stop_solve OptStep OptSolve opt}

set autoLoadList(base_procedures.tcl) {0}

global square_solver
set square_solver QRSlv

# procedures to show toplevel windows


# User defined procedures

# Procedure: IsaInt
proc IsaInt { var} {

    ##########
    #Procedure:IsaInt
    #Description:Check if tcl variable is an integer
    #Arguments:var - name of tcl variable to be checked
    #Returns:1 if it is an integer, 0 if not
    #Sideffects:none
    ##########


    if {([scan $var %d y]) && ($y == $var)} {
	return 1
    } else {
	return 0
    }
}


# Procedure: CHKSOL
proc CHKSOL { name} {
    
    ##########
    #Procedure:CHKSOL
    #Description:Check if solver thinks it is converged
    #Arguments:none
    #Returns:none
    #Sideffects:puts message if converged, error if not
    ##########

    global ascSolvStatVect
    if {$ascSolvStatVect(converged)} {
	puts "$name converged"
    } else {
	error "$name not converged"
    }
    return
}



# Procedure: SOLST
proc SOLST {} {
    
    ##########
    #Procedure:SOLST
    #Description:Check if solver thinks it is converged
    #Arguments:none
    #Returns:1 if solver is converged, 0 if not
    #Sideffects:none
    ##########
    
    global ascSolvStatVect
    return $ascSolvStatVect(converged)
}

# Procedure: GET_NUMBER
proc GET_NUMBER { vname} {
    
    ##########
    #Procedure:GET_NUMBER
    #Description:Gets variable number given name
    #Arguments:vname - qlfdid of variable
    #Returns:none
    #Sideffects:puts out variable number
    ##########
    
    set sname [slv_get_pathname]
    foreach i [dbg_list_vars 0] {
	set pname [string trim [string trim [dbg_write_var 2 $i 0] "{"] "}"]
	set pname "$sname.$pname"
	if {$vname == $pname} {
	    puts "$vname is number $i"
	    break
	}
    }
}


# Procedure: VAL
proc VAL { var} {
    
    ##########
    #Procedure: VAL
    #Description: get value of ASCEND variable
    #Arguments: var - qualified id of ASCEND variable
    #Returns: value if var
    #Sideffects:none
    ##########

    qlfdid $var
    if {[inst type search] == "integer"} {
	set a [lindex [lindex [u_getval $var] 0] 0]
    } else {
	set a [inst atomvalue search]
    }
    return $a
}


# Procedure: clear_virtual_stuff
proc clear_virtual_stuff {} {
    
    ##########
    #Procedure:clear_virtual_stuff
    #Description:Destroy all virtual information
    #Arguments:none
    #Returns:none
    #Sideffects:none
    ##########
    
    __user_data_destroy all
}


# Procedure: CollocationTraySolve
proc CollocationTraySolve { name} {
    
    ##########
    #Procedure:CollocationTraySolve
    #Description:Solve each tray in collocation column
    #Arguments:name - Qualified id of collocation column model
    #Returns:"done" if done
    #Sideffects:none
    ##########
 
    global square_solver
    
    RUN $name.condenser.reset
    puts "solving condenser"
    SOLVE $name.condenser WITH $square_solver
    CHKSOL condenser
 for {set i 1} {$i <= [expr 1 + [VAL $name.nfeeds]]} {incr i} {
     set ncolls $name.coll_stack\[$i\].ncolls
     for {set j 1} {$j <= [VAL $ncolls]} {incr j} {
	 set ntrays $name.coll_stack\[$i\].coll\[$j\].ntrays
	 for {set k 1} {$k <= [VAL $ntrays]} {incr k} {
	     RUN $name.coll_stack\[$i\].coll\[$j\].tray\[$k\].reset
	     puts solving\ $name.coll_stack\[$i\].coll\[$j\].tray\[$k\]
	     SOLVE $name.coll_stack\[$i\].coll\[$j\].tray\[$k\] WITH $square_solver
	     CHKSOL $name.coll_stack\[$i\].coll\[$j\].tray\[$k\]
	 }
     }
 }
 for {set i 1} {$i <= [VAL $name.nfeeds]} {incr i} {
     RUN $name.feed_tray\[$i\].reset
     puts "solving feed tray $i"
     SOLVE $name.feed_tray\[$i\] WITH $square_solver
     CHKSOL $name.feed_tray\[$i\]
 }
 RUN $name.reboiler.reset
 puts "solving reboiler"
 SOLVE $name.reboiler WITH $square_solver
 CHKSOL reboiler
 return "done"
}


# Procedure: HeatBalance
proc HeatBalance { name reduce} {
    
    ##########
    #Procedure:HeatBalance
    #Description:Attempt to meet heat balance on column
    #Arguments:name - Qualified id of collocation column
    #          reduce - fraction to reduce heat by
    #Returns:1 if successful, 0 if failed
    #Sideffects:none
    ##########
    
    global square_solver
    
    WRITE_VIRTUAL $name temp.virtual
    RUN $name.heat_balance
    ASSIGN $name.reduce $reduce
    set trys [expr 1/(1-$reduce)]
    for {set i 1} {$i <= $trys} {incr i} {
	RUN $name.reduce_Q
	SOLVE $name WITH $square_solver
	if {![SOLST]} {
	    READ_VIRTUAL temp.virtual
            puts failed
	    return 0
	}
    }
    RUN $name.zero_Q
    SOLVE $name WITH $square_solver
    if {![SOLST]} {
	READ_VIRTUAL temp.virtual
	return 0
    }
    return 1
}


# Procedure: SolvCol1
proc SolvCol1 { name} {
    
    ##########
    #Procedure:SolvCol1
    #Description:Solve collocation column to heat balance and equilibrium
    #Arguments:name- name of model that is of type coll_column 
    #           (or refinement)
    #Returns:none
    #Sideffects:Outputs progress.
    ##########
    
    global square_solver

    SOLVE $name WITH $square_solver
    CHKSOL {first solve}
    REFINE $name TO {equilibrium_coll_column}
    RUN $name.trans_poly
    RUN $name.z_based_poly
    # CollocationTraySolve $name
    RUN $name.reset
    SOLVE $name WITH $square_solver
    CHKSOL {equilibrium column}
    # WRITE VALUES $name /afs/cs/project/ascend/ascend3/rhuss/temp_values/$name.values
    WRITE_VIRTUAL $name temp.virtual
    PROBE current $name.s_stack {}
    PROBE current $name.condenser.reflux_ratio {}
    PROBE current $name.condenser.totprod.Ftot {}
    PROBE current $name.xsi {}
    if {![HeatBalance $name 0.5]} {
	set answer [tk_dialog .heatbalance {HB not met}  {Heat balance not met quickly, want to try to meet it slowly?}  "" 0 Yes No]
	if {$answer == 0} {
	    if {![HeatBalance $name 0.9]} {
		puts "Failed to converge column with heat balance, recovering CMO model"
		READ_VIRTUAL temp.virtual
		return
	    }
	}
    }
    
	WRITE_VIRTUAL $name temp.virtual
	puts "Okay, equilibrium, heat balanced column solved."
}


# Procedure: CreepSet
proc CreepSet { key spec delta min_step} {
    
    ##########
    #Procedure:CreepSet
    #Description:Make step for meet spec procedures
    #Arguments:key - qlfdid of variable to be changed
    #          spec - eventual specification for key variable
    #          delta - current size of step to be taken
    #          min_step - smallest size of step allowed
    #Returns:value of step
    #Sideffects:Assigns variable to new point
    ##########
    
    set diff [expr $spec - [VAL $key]]
    if {$diff > 0} {
	set step [expr $delta*$diff]
	if {$diff < $min_step} {
	    set step $diff
	} elseif {$step < $min_step} {
	    set step $min_step
	}
 } else {
     set step [expr $delta*$diff]
     if {$diff > [expr -1.0*$min_step]} {
	 set step $diff
     } elseif {$step > [expr -1*$min_step]} {
	 set step [expr -1.0*$min_step]
     }
 }
 ASSIGN $key [expr [VAL $key] + $step]
 puts step\ =\ $step
 return $step
}


# Procedure: MultipleMeetSpec
proc MultipleMeetSpec { name keys specs delta} {
    
    ##########
    #Procedure:MultipleMeetSpec
    #Description:Meet specification on an arbitrary number
    #            of variables for a specified model
    #Arguments:name - qlfdid of model to be solved
    #          keys - set of qlfdid's for key variables
    #          specs - set of specifications for key variables
    #          delta - initial relative step size (1 means go
    #                 all the way)
    #Returns:1 if successful, 0 if not
    #Sideffects:Performs multiple solutions of model
    ##########
    
    global square_solver
    global stopflag
    set stopflag 0
    for {set i 0} {$i <= [expr [llength $keys] - 1]} {incr i} {
	set key_array($i) [lindex $keys $i]
	set spec_array($i) [lindex $specs $i]
	set min_array($i) [expr sqrt(pow([expr $delta*0.2*[expr [VAL $key_array($i)] - $spec_array($i)]],2))]
	puts $min_array($i)
	if {[VAL $key_array($i).fixed] == "FALSE"} {
	    puts "a key variable is not fixed:"
	    puts $key_array($i)
	    return
	}
    }
    foreach i [lsort [array names key_array]] {
	if {$min_array($i) == 0.0} {
	    puts "One of your key variables is already at its spec:"
	    puts $key_array($i)
	    puts "Removing it from the list"
	    unset key_array($i)
	    unset spec_array($i)
	    unset min_array($i)
	}
    }

    SOLVE $name WITH $square_solver
    if {![SOLST]} {
	puts "Starting point is not converged."
	return
    }
    set fail 0
    WRITE_VIRTUAL $name temp.virtual
    set check_i [lindex [lsort [array names key_array]] 0]

    while {[VAL $key_array($check_i)] != $spec_array($check_i)} {
	if {$stopflag} {
	    puts "received external stop signal"
	    READ_VIRTUAL temp.virtual
	    puts delta:\ $delta
	    return 0
	}
	foreach i [array names key_array] {
	    set step($i) [CreepSet $key_array($i) $spec_array($i) $delta $min_array($i)]
	}
	puts delta:\ $delta
	slv_resolve
	Solve_do_Optimize
	if {[SOLST]} {
	    puts "Step made, saving progress and making next step with same delta."
	    WRITE_VIRTUAL $name temp.virtual
	    if {$fail > 0} {incr fail -1}
	    set delta [expr $delta*1.2]
	    if {$delta >= 1.0} {set $delta 0.9}
	} else {
	    puts "fail:$fail min:$min_array($check_i) step:[expr sqrt([expr pow($step($check_i),2)])]"
	    if {$min_array($check_i) >= [expr 0.99*sqrt([expr pow($step($check_i),2)])]} {
		puts "Not converging, getting last converged and exiting. Specification not met."
		READ_VIRTUAL temp.virtual
		puts delta:\ $delta
		return 0
	    }
	    if {$fail > 6} {
		puts "Not converging, getting last converged and exiting. Specification not met."
		READ_VIRTUAL temp.virtual
		puts delta:\ $delta
		return 0
	    }
	    puts "Step failed, reducing delta and trying again."
	    incr fail
	    set delta [expr 0.7*$delta/$fail]
	    
	    READ_VIRTUAL temp.virtual
	}
    }
#    set answer [tk_dialog .meetspec {Specification Met} {Specification Met
#    Would you like to 
#    save the values?} "" 0 No Yes]
#    if {$answer == 1} {
#	WRITE VALUES $name /afs/cs/project/ascend/ascend3/rhuss/temp_values/$name.values
#	
	puts "Specification met."
	return 1
#    }
}


# Procedure: stop_solve
proc stop_solve {} {
    
    ##########
    #Procedure:stop_solve
    #Description:Sets global stopflag to 1.  Will interupt some procedures
    #Arguments:none
    #Returns:none
    #Sideffects:none
    ##########
    
    global stopflag
    set stopflag 1
}

proc SOLVE {qlfdid args} {
  global ascSolvVect ascSolvStatVect
  if {[slv_import_qlfdid $qlfdid test]} {error "$qlfdid not solvable instance"}
  if {$ascSolvVect(visibility)} {newraise .solver}
  set argc [llength $args]
  case $argc in {
   {0} {
         Solve_Import_Any $qlfdid
         Solve_do_Select 0
         Solve_do_Solve
         return
       }
   {1} { set solname [lindex $args 0]
         # go off of C structure here...
         case $solname in {
           {Slv slv} {set num 0}
           {MINOS} {set num 1}
           {QRSlv} {set num 3}
           default {error "SOLVE called with $solname. expected Slv, MINOS, QRSlv."}
         }
         set ascSolvVect(simname) [lindex [split $qlfdid .] 0]
         slv_import_qlfdid $qlfdid
         set ascSolvStatVect(menubreak) 0
         slv_set_haltflag 0
         set ascSolvStatVect(empty) 0
         Solve_do_Select $num
         Solve_Update_Listbox
         Solve_Update_MenuBar
         Solve_SLV_Downdate_ParmBox
         if {$ascSolvVect(debuggerup)} {
           Debug_Trace on
         }
         if {$ascSolvVect(mtxup)} {
           Solve_do_DispIncidence
         }
         Solve_do_Solve
         return
       }
   {2} { set WITH [lindex $args 0]
         set solname [lindex $args 1]
         if {[string tolower $WITH] != "with"} {
           puts stderr  "Non-fatal script error: $WITH found. WITH expected."
         }
         case $solname in {
           {Slv} {set num 0}
           {MINOS} {set num 1}
           {QRSlv} {set num 3}
           default {error "SOLVE called with $solname. expected Slv, MINOS, QRSlv."}
         }
         set ascSolvVect(simname) [lindex [split $qlfdid .] 0]
         slv_import_qlfdid $qlfdid
         set ascSolvStatVect(menubreak) 0
         slv_set_haltflag 0
         set ascSolvStatVect(empty) 0
         Solve_do_Select $num
         Solve_Update_Listbox
         Solve_Update_MenuBar
         Solve_SLV_Downdate_ParmBox
         if {$ascSolvVect(debuggerup)} {
           Debug_Trace on
         }
         if {$ascSolvVect(mtxup)} {
           Solve_do_DispIncidence
         }
         Solve_do_Solve
         return
       }
    default {error "Syntax: SOLVE instance [WITH] [solvername]"}
  }
}

#Procedure OptSlope
proc OptSlope {objective var} {
    
    ##########
    #Procedure:OptSlope
    #Description:Gets the slope of objective with respect to each
    #            variable in the set var
    #Arguments:objective - qlfdid of objective variable
    #          var - set of qlfdid's of search variables
    #Returns:none
    #Sideffects:Multiple solves, puts out progress.
    ##########
    

    global OptVect

    set i $var
    set delta_obj 0.0
    if {[expr $OptVect(slope,$i)*$OptVect(delta,$i)] >= 0.0 } {
	set OptVect(delta,$i) [expr -$OptVect(delta,$i)]
    }
    while { [expr sqrt(pow($delta_obj,2))] <= 0.0} {

	puts "delta $i $OptVect(delta,$i)"
	ASSIGN $i [expr [VAL $i] + $OptVect(delta,$i)]
	puts [VAL $i]
		slv_resolve
	Solve_do_Optimize

	CHKSOL $OptVect(iname)
	set OptVect(obj_out) [VAL $objective]
	set delta_obj [expr 1.0*[expr $OptVect(obj_out) - $OptVect(obj_in)]/$OptVect(nominal,$objective)]

	set OptVect(delta,$i) [expr $OptVect(delta,$i)*10.0]
    }
    puts "objective change for $i $delta_obj"
    set OptVect(slope,$i) [expr $delta_obj*$OptVect(nominal,$i)/$OptVect(delta,$i)]
    puts OptVect(slope,$i)\ $OptVect(slope,$i)
    if {$delta_obj < 0} {
	    WRITE_VIRTUAL $OptVect(iname) $OptVect(iname).virtual
	set OptVect(obj_in) $OptVect(obj_out)
    } else {
	READ_VIRTUAL $OptVect(iname).virtual
    }
    puts "actual obj: $OptVect(obj_in) scaled objective: [expr 1.0*$OptVect(obj_in)/$OptVect(nominal,$objective)]"
    set OptVect(delta,$i) [expr $OptVect(delta,$i)*0.1]
    
	
}

#Procedure OptStep
proc OptStep {} {
    
    ##########
    #Procedure:OptStep
    #Description:Sets a gradient based step for current optimization problem
    #Arguments:none
    #Returns:1 if it made step, 0 if not (becase of low slope)
    #Sideffects:Sets variables to new values.
    ##########
    


    global OptVect


    puts OptVect(ave_slope)\ $OptVect(ave_slope)
    puts Taking\ step\ $OptVect(step_size)
    set minslope 100000.0
    set minvar "none"
    foreach i $OptVect(var_set) {
	set abs_slope($i) [expr sqrt(pow($OptVect(slope,$i),2))]
	puts "abs_slope($i) $abs_slope($i)"
	if {$abs_slope($i) < $OptVect(min)} {
	    puts "Not changing $i"
	} elseif {$abs_slope($i) < $minslope} {
	    set minslope $abs_slope($i)
	    set minvar $i
	}
    }
    foreach i $OptVect(var_set) {
	puts $i
	if {$abs_slope($i) >= $OptVect(min)} {
	    set step($i) [expr -$OptVect(step_size)*$OptVect(nominal,$i)*$minslope/$OptVect(slope,$i)]
	    puts $step($i)
	puts "$i: slope=$OptVect(slope,$i), step = $step($i)"
	    ASSIGN $i [expr [VAL $i] + $step($i)]
	    
	    if {[VAL $i] > [VAL $i.upper_bound]} {
		puts $i\ at\ upper\ bound
		ASSIGN $i [VAL $i.upper_bound]
	    } elseif {[VAL $i] < [VAL $i.lower_bound]} {
		puts $i\ at\ lower\ bound
		ASSIGN $i [VAL $i.lower_bound]
	    }
	}

    }
    if {$minvar == "none"} {
	READ_VIRTUAL $OptVect(iname).virtual
	return 0
    }
    return 1
}

#Procedure OptSolve
proc OptSolve {} {
    
    ##########
    #Procedure:OptSolve
    #Description:Solves current step in optimization
    #Arguments:none
    #Returns: 0 if it couldn't make a objective reducing step 
    #         1 if it could
    #Sideffects:Multiple solves, spewing progress report
    ##########
    

    global OptVect
    global stopflag
    set stopflag 0

    if ![OptStep] {
	puts "average slope at below minimum"
	return 1
    }
    	slv_resolve
	Solve_do_Optimize

    if {![SOLST]} {
	set conv 0
	while {!$conv} {
	    if {$stopflag} {
		puts stopping
		return 0
	    }
	    READ_VIRTUAL $OptVect(iname).virtual
	    if {$OptVect(step_size) < 1.0} {
		puts "Not converging at small step size, stopping."
		return 0
	    } else {
		set OptVect(step_size) [expr $OptVect(step_size)*0.5]
	    }
	    if ![OptStep] {
		puts "average slope at below minimum"
		return 1
	    }
	    	slv_resolve
	Solve_do_Optimize

	    set conv [SOLST]
	}
    }
}
	    


proc opt {iname objective delta step_size args} {
    
    ##########
    #Procedure:opt
    #Description:Do gradient based optimization on model
    #Arguments:iname - qlfdid of model to be solved
    #          objective = qlfdid of objective variable
    #          delta = change for finite difference calc of gradients
    #          step_size - initial precentage stepsize
    #          args - set of variables to be used as search variables for
    #                 the optimization
    #Returns:none
    #Sideffects:Lots of solves, spew progress
    ##########
    

    global OptVect
    global square_solver
    set OptVect(t_delta) $delta
    set OptVect(var_set) $args
    set OptVect(min) 0.01
    set OptVect(step_size) $step_size
#    set OptVect(step_size) [expr $step_size*$OptVect(t_delta)]
    set OptVect(iname) $iname
    SOLVE $iname WITH $square_solver
    slv_resolve
    WRITE_VIRTUAL $OptVect(iname) $OptVect(iname).virtual
    foreach i $OptVect(var_set) {
	set OptVect(slope,$i) 1
	set OptVect(nominal,$i) [VAL $i]
	set OptVect(delta,$i) [expr -$OptVect(t_delta)*$OptVect(nominal,$i)]
	if {[VAL $i.fixed] == "FALSE"} {
	    puts "A search variable is not fixed"
	    puts $i
	    return
	}
    }
    set OptVect(nominal,$objective) [VAL $objective]
    global stopflag
    set stopflag 0
    set OptVect(ave_slope) [expr 10*$OptVect(min)]
    set OptVect(obj_in) [expr 1.0*[VAL $objective]]
    puts "initial objective $OptVect(obj_in)"
#    puts "objective $varset 

    WRITE_VIRTUAL $OptVect(iname) $OptVect(iname).virtual
    while {$OptVect(ave_slope) > $OptVect(min)} {
	
	if {$stopflag} {
	    puts stopping
	    return
	}
	set OptVect(ave_slope) 0.0
	foreach i $OptVect(var_set) {

	    OptSlope $objective $i
	    set OptVect(ave_slope) [expr [expr sqrt(pow($OptVect(ave_slope),2)) + sqrt(pow($OptVect(slope,$i),2))]/2]

	}
	
	OptSolve

	set OptVect(obj_out) [expr 1.0*[VAL $objective]]
	puts "in $OptVect(obj_in) out $OptVect(obj_out)"
	puts "check delta [expr $OptVect(obj_in) - $OptVect(obj_out)]"
	while {[expr $OptVect(obj_in) - $OptVect(obj_out)] < 0} {
	puts "in $OptVect(obj_in) out $OptVect(obj_out)"
	puts "check delta [expr $OptVect(obj_in) - $OptVect(obj_out)]"
	    if {$stopflag} {
		puts stopping
	     return
	    }
	    puts "Step did not reduce objective"
	    puts Increased\ objective\ $OptVect(obj_out)
	    READ_VIRTUAL $OptVect(iname).virtual

	    if {$OptVect(step_size) < 0.01} {
		puts "no progress at small step size"
		return
	    } else {
		set OptVect(step_size) [expr $OptVect(step_size)*0.5]
	    }

	    OptSolve
	    set OptVect(obj_out) [expr 1.0*[VAL $objective]]
	} 
	set OptVect(step_size) [expr $OptVect(step_size)*1.5]
	if {$OptVect(step_size) > 0.5} {
	    set OptVect(step_size) 0.5
	}
	WRITE_VIRTUAL $OptVect(iname) $OptVect(iname).virtual
    }
    puts "Average slope below minimum: $OptVect(ave_slope)"
}

  
  


# Internal procedures

# eof
#

