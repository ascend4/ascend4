#                        flexible.tcl
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

#  $Date: 1997/06/09 19:50:50 $
#  $Revision: 1.2 $
#  $Author: ballan $
#  $Source: /afs/cs.cmu.edu/project/ascend/Repository/models/examples/flexible_design/flexible.tcl,v $

# Module: flexible.tcl
# Tcl version: 7.3 (Tcl/Tk/XF)
# Tk version: 3.6
# XF version: 2.3
#

# module contents
global moduleList
global autoLoadList
set moduleList(flexible.tcl) { fracmax fracmin stotmax stotmin GetRefluxData SetupAll SaveValues UpdateValues FitSet FindSpot Doapprox FitOpt StepOpt DoOptimize CompleteOptimize FlexibleMenu}
set autoLoadList(flexible.tcl) {0}

# procedures to show toplevel windows


# User defined procedures


# Procedure: fracmax
proc fracmax {} {
    
    ##########
    #Procedure:fracmax
    #Description:Move current column to max point relative to
    #            current feed position
    #Arguments:none
    #Returns:none
    #Sideffects:none
    ##########
    
    global collVect
    if {[info exist keys]} {unset keys}
    if {[info exist specs]} {unset specs}
    lappend keys $collVect(name).col.split\[1\]
    lappend specs [expr $collVect(max) * [VAL $collVect(name).col.split\[1\]]]
    MultipleMeetSpec $collVect(name).col $keys $specs $collVect(delta)
}


# Procedure: fracmin
proc fracmin {} {
    
    ##########
    #Procedure:fracmin
    #Description:Move current column to min point relative to
    #            current feed position
    #Arguments:none
    #Returns:none
    #Sideffects:none
    ##########
    
    global collVect
    if {[info exist keys]} {unset keys}
    if {[info exist specs]} {unset specs}
    lappend keys $collVect(name).col.split\[1\]
    lappend specs [expr $collVect(min) * [VAL $collVect(name).col.split\[1\]]]
    MultipleMeetSpec $collVect(name).col $keys $specs $collVect(delta)
}


# Procedure: stotmax
proc stotmax {} {
    
    ##########
    #Procedure:stotmax
    #Description:Move current column to max point relative to
    #            current number of trays
    #Arguments:none
    #Returns:none
    #Sideffects:none
    ##########
    
    global collVect
    if {[info exist keys]} {unset keys}
    if {[info exist specs]} {unset specs}
    lappend keys $collVect(name).col.stot
    lappend specs [expr $collVect(max) * [VAL $collVect(name).col.stot]]
    MultipleMeetSpec $collVect(name).col $keys $specs $collVect(delta)
}


# Procedure: stotmin
proc stotmin {} {
    
    ##########
    #Procedure:stotmin
    #Description:Move current column to min point relative to
    #            current number of trays
    #Arguments:none
    #Returns:none
    #Sideffects:none
    ##########
    
    global collVect
    if {[info exist keys]} {unset keys}
    if {[info exist specs]} {unset specs}
    lappend keys $collVect(name).col.stot
    lappend specs [expr $collVect(min) * [VAL $collVect(name).col.stot]]
    MultipleMeetSpec $collVect(name).col $keys $specs $collVect(delta)
}


# Procedure: GetRefluxData
proc GetRefluxData {} {
    
    ##########
    #Procedure:GetRefluxData
    #Description:Solve multiple points for reflux as a function of
    #            number of trays and feed location.
    #Arguments:none
    #Returns:none
    #Sideffects:Multiple solves, updating virtual save
    ##########
    

    global collVect
    global reflux
    global stot
    global frac
    global stopflag

    set stopflag 0

    if {$stopflag == 1} {error stopped}
    
    WRITE_VIRTUAL $collVect(name).col base.[VAL $collVect(name).current_col].virtual

    set current [VAL $collVect(name).current_col]

    
    
    if {$collVect(spot) == "nn"} {
	
	puts "no need to re-approximate, at local optimum"
	foreach i {1 2 3 4 5 6 7 8 9} {
	    set reflux($i) [VAL $collVect(name).col_fit\[[VAL $collVect(name).current_col]\].reflux\[$i\]]
	    set stot($i) [VAL $collVect(name).col_fit\[$current\].stot\[$i\]]
	    set frac($i) [VAL $collVect(name).col_fit\[$current\].frac\[$i\]] 
	}

    } elseif {$collVect(spot) == "ll"} {
	puts ll

	foreach i {1 2 4 5} {
	    set reflux([expr $i + 4]) [VAL $collVect(name).col_fit\[$current\].reflux\[$i\]]
	    set stot([expr $i + 4]) [VAL $collVect(name).col_fit\[$current\].stot\[$i\]]
	    set frac([expr $i + 4]) [VAL $collVect(name).col_fit\[$current\].frac\[$i\]] 
	}
    
    if {$stopflag == 1} {error stopped}

	fracmin
	set reflux(4) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(4) [VAL $collVect(name).col.stot]
	set frac(4) [VAL $collVect(name).col.split\[1\]]
    if {$stopflag == 1} {error stopped}
	stotmax
	set reflux(7) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(7) [VAL $collVect(name).col.stot]
	set frac(7) [VAL $collVect(name).col.split\[1\]]
	READ_VIRTUAL base.$current.virtual
    if {$stopflag == 1} {error stopped}
	stotmin
	set reflux(2) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(2) [VAL $collVect(name).col.stot]
	set frac(2) [VAL $collVect(name).col.split\[1\]]
	WRITE_VIRTUAL $collVect(name).col rtemp.virtual
    if {$stopflag == 1} {error stopped}
	fracmin
	set reflux(1) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(1) [VAL $collVect(name).col.stot]
	set frac(1) [VAL $collVect(name).col.split\[1\]]
	READ_VIRTUAL rtemp.virtual
    if {$stopflag == 1} {error stopped}
	fracmax
	set reflux(3) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(3) [VAL $collVect(name).col.stot]
	set frac(3) [VAL $collVect(name).col.split\[1\]]
    
    } elseif {$collVect(spot) == "lu"} {
	puts lu


	foreach i {2 3 5 6} {
	    set reflux([expr $i + 2]) [VAL $collVect(name).col_fit\[$current\].reflux\[$i\]]
	    set stot([expr $i + 2]) [VAL $collVect(name).col_fit\[$current\].stot\[$i\]]
	    set frac([expr $i + 2]) [VAL $collVect(name).col_fit\[$current\].frac\[$i\]] 
	}
    if {$stopflag == 1} {error stopped}
	fracmax
	set reflux(6) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(6) [VAL $collVect(name).col.stot]
	set frac(6) [VAL $collVect(name).col.split\[1\]]
    if {$stopflag == 1} {error stopped}
	stotmax
	set reflux(9) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(9) [VAL $collVect(name).col.stot]
	set frac(9) [VAL $collVect(name).col.split\[1\]]
	READ_VIRTUAL base.$current.virtual
    if {$stopflag == 1} {error stopped}
	stotmin
	set reflux(2) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(2) [VAL $collVect(name).col.stot]
	set frac(2) [VAL $collVect(name).col.split\[1\]]
	WRITE_VIRTUAL $collVect(name).col rtemp.virtual
    if {$stopflag == 1} {error stopped}
	fracmin
	set reflux(1) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(1) [VAL $collVect(name).col.stot]
	set frac(1) [VAL $collVect(name).col.split\[1\]]
    if {$stopflag == 1} {error stopped}
	READ_VIRTUAL rtemp.virtual
	fracmax
	set reflux(3) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(3) [VAL $collVect(name).col.stot]
	set frac(3) [VAL $collVect(name).col.split\[1\]]

    } elseif {$collVect(spot) == "uu"} {

	foreach i {5 6 8 9} {
	    set reflux([expr $i - 4]) [VAL $collVect(name).col_fit\[$current\].reflux\[$i\]]
	    set stot([expr $i - 4]) [VAL $collVect(name).col_fit\[$current\].stot\[$i\]]
	    set frac([expr $i - 4]) [VAL $collVect(name).col_fit\[$current\].frac\[$i\]] 
	}

    	puts uu
    if {$stopflag == 1} {error stopped}
	fracmax
	set reflux(6) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(6) [VAL $collVect(name).col.stot]
	set frac(6) [VAL $collVect(name).col.split\[1\]]
    if {$stopflag == 1} {error stopped}
	stotmin
	set reflux(3) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(3) [VAL $collVect(name).col.stot]
	set frac(3) [VAL $collVect(name).col.split\[1\]]
	READ_VIRTUAL base.$current.virtual
    if {$stopflag == 1} {error stopped}
	stotmax
	set reflux(8) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(8) [VAL $collVect(name).col.stot]
	set frac(8) [VAL $collVect(name).col.split\[1\]]
	WRITE_VIRTUAL $collVect(name).col rtemp.virtual
    if {$stopflag == 1} {error stopped}
	fracmin
	set reflux(7) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(7) [VAL $collVect(name).col.stot]
	set frac(7) [VAL $collVect(name).col.split\[1\]]
	READ_VIRTUAL rtemp.virtual
    if {$stopflag == 1} {error stopped}
	fracmax
	set reflux(9) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(9) [VAL $collVect(name).col.stot]
	set frac(9) [VAL $collVect(name).col.split\[1\]]

    } elseif {$collVect(spot) == "ul"} {

	foreach i {4 5 7 8} {
	    set reflux([expr $i - 2]) [VAL $collVect(name).col_fit\[$current\].reflux\[$i\]]
	    set stot([expr $i - 2]) [VAL $collVect(name).col_fit\[$current\].stot\[$i\]]
	    set frac([expr $i - 2]) [VAL $collVect(name).col_fit\[$current\].frac\[$i\]] 
	}

    	puts ul
    if {$stopflag == 1} {error stopped}
	fracmin
	set reflux(4) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(4) [VAL $collVect(name).col.stot]
	set frac(4) [VAL $collVect(name).col.split\[1\]]
    if {$stopflag == 1} {error stopped}
	stotmin
	set reflux(1) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(1) [VAL $collVect(name).col.stot]
	set frac(1) [VAL $collVect(name).col.split\[1\]]
	READ_VIRTUAL base.$current.virtual
    if {$stopflag == 1} {error stopped}
	stotmax
	set reflux(8) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(8) [VAL $collVect(name).col.stot]
	set frac(8) [VAL $collVect(name).col.split\[1\]]
	WRITE_VIRTUAL $collVect(name).col rtemp.virtual
    if {$stopflag == 1} {error stopped}
	fracmin
	set reflux(7) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(7) [VAL $collVect(name).col.stot]
	set frac(7) [VAL $collVect(name).col.split\[1\]]
	READ_VIRTUAL rtemp.virtual
    if {$stopflag == 1} {error stopped}
	fracmax
	set reflux(9) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(9) [VAL $collVect(name).col.stot]
	set frac(9) [VAL $collVect(name).col.split\[1\]]

    } elseif {$collVect(spot) == "ln"} {
	foreach i {1 2 3 4 5 6} {
	    set reflux([expr $i + 3]) [VAL $collVect(name).col_fit\[$current\].reflux\[$i\]]
	    set stot([expr $i + 3]) [VAL $collVect(name).col_fit\[$current\].stot\[$i\]]
	    set frac([expr $i + 3]) [VAL $collVect(name).col_fit\[$current\].frac\[$i\]] 
	}
	puts ln
    if {$stopflag == 1} {error stopped}
	stotmin
	set reflux(2) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(2) [VAL $collVect(name).col.stot]
	set frac(2) [VAL $collVect(name).col.split\[1\]]
	WRITE_VIRTUAL $collVect(name).col rtemp.virtual
    if {$stopflag == 1} {error stopped}
	fracmin
	set reflux(1) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(1) [VAL $collVect(name).col.stot]
	set frac(1) [VAL $collVect(name).col.split\[1\]]
	READ_VIRTUAL rtemp.virtual
    if {$stopflag == 1} {error stopped}
	fracmax
	set reflux(3) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(3) [VAL $collVect(name).col.stot]
	set frac(3) [VAL $collVect(name).col.split\[1\]]

    } elseif {$collVect(spot) == "un"} {
	foreach i {4 5 6 7 8 9} {
	    set reflux([expr $i - 3]) [VAL $collVect(name).col_fit\[$current\].reflux\[$i\]]
	    set stot([expr $i - 3]) [VAL $collVect(name).col_fit\[$current\].stot\[$i\]]
	    set frac([expr $i - 3]) [VAL $collVect(name).col_fit\[$current\].frac\[$i\]] 
	}
	puts un
    if {$stopflag == 1} {error stopped}
	stotmax
	set reflux(8) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(8) [VAL $collVect(name).col.stot]
	set frac(8) [VAL $collVect(name).col.split\[1\]]
	WRITE_VIRTUAL $collVect(name).col rtemp.virtual
    if {$stopflag == 1} {error stopped}
	fracmin
	set reflux(7) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(7) [VAL $collVect(name).col.stot]
	set frac(7) [VAL $collVect(name).col.split\[1\]]
	READ_VIRTUAL rtemp.virtual
    if {$stopflag == 1} {error stopped}
	fracmax
	set reflux(9) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(9) [VAL $collVect(name).col.stot]
	set frac(9) [VAL $collVect(name).col.split\[1\]]

    } elseif {$collVect(spot) == "nl"} {
	foreach i {1 2 4 5 7 8} {
	    set reflux([expr $i + 1]) [VAL $collVect(name).col_fit\[$current\].reflux\[$i\]]
	    set stot([expr $i + 1]) [VAL $collVect(name).col_fit\[$current\].stot\[$i\]]
	    set frac([expr $i + 1]) [VAL $collVect(name).col_fit\[$current\].frac\[$i\]] 
	}
	puts nl
    if {$stopflag == 1} {error stopped}
	fracmin
	set reflux(4) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(4) [VAL $collVect(name).col.stot]
	set frac(4) [VAL $collVect(name).col.split\[1\]]
	WRITE_VIRTUAL $collVect(name).col rtemp.virtual
    if {$stopflag == 1} {error stopped}
	stotmin
	set reflux(1) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(1) [VAL $collVect(name).col.stot]
	set frac(1) [VAL $collVect(name).col.split\[1\]]
	READ_VIRTUAL rtemp.virtual
    if {$stopflag == 1} {error stopped}
	stotmax
	set reflux(7) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(7) [VAL $collVect(name).col.stot]
	set frac(7) [VAL $collVect(name).col.split\[1\]]

    } elseif {$collVect(spot) == "nu"} {
	foreach i {2 3 5 6 8 9} {
	    set reflux([expr $i - 1]) [VAL $collVect(name).col_fit\[$current\].reflux\[$i\]]
	    set stot([expr $i - 1]) [VAL $collVect(name).col_fit\[$current\].stot\[$i\]]
	    set frac([expr $i - 1]) [VAL $collVect(name).col_fit\[$current\].frac\[$i\]] 
	}
	puts nu
    if {$stopflag == 1} {error stopped}
	fracmax
	set reflux(6) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(6) [VAL $collVect(name).col.stot]
	set frac(6) [VAL $collVect(name).col.split\[1\]]
	WRITE_VIRTUAL $collVect(name).col rtemp.virtual
    if {$stopflag == 1} {error stopped}
	stotmin
	set reflux(3) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(3) [VAL $collVect(name).col.stot]
	set frac(3) [VAL $collVect(name).col.split\[1\]]
	READ_VIRTUAL rtemp.virtual
    if {$stopflag == 1} {error stopped}
	stotmax
	set reflux(9) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(9) [VAL $collVect(name).col.stot]
	set frac(9) [VAL $collVect(name).col.split\[1\]]

    } elseif {$collVect(spot) == "none"} {
	puts "none"


	set reflux(5) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(5) [VAL $collVect(name).col.stot]
	set frac(5) [VAL $collVect(name).col.split\[1\]]
    if {$stopflag == 1} {error stopped}
	fracmax
	set reflux(4) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(4) [VAL $collVect(name).col.stot]
	set frac(4) [VAL $collVect(name).col.split\[1\]]
	READ_VIRTUAL base.$current.virtual
    if {$stopflag == 1} {error stopped}
	fracmin
	set reflux(6) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(6) [VAL $collVect(name).col.stot]
	set frac(6) [VAL $collVect(name).col.split\[1\]]
	READ_VIRTUAL base.$current.virtual
    if {$stopflag == 1} {error stopped}
	stotmin
	set reflux(2) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(2) [VAL $collVect(name).col.stot]
	set frac(2) [VAL $collVect(name).col.split\[1\]]
	WRITE_VIRTUAL $collVect(name).col rtemp.virtual
    if {$stopflag == 1} {error stopped}
	fracmax
	set reflux(1) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(1) [VAL $collVect(name).col.stot]
	set frac(1) [VAL $collVect(name).col.split\[1\]]
	READ_VIRTUAL rtemp.virtual
    if {$stopflag == 1} {error stopped}
	fracmin
	set reflux(3) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(3) [VAL $collVect(name).col.stot]
	set frac(3) [VAL $collVect(name).col.split\[1\]]
	READ_VIRTUAL base.$current.virtual
    if {$stopflag == 1} {error stopped}
	stotmax
	set reflux(8) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(8) [VAL $collVect(name).col.stot]
	set frac(8) [VAL $collVect(name).col.split\[1\]]
	WRITE_VIRTUAL $collVect(name).col rtemp.virtual
    if {$stopflag == 1} {error stopped}
	fracmax
	set reflux(7) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(7) [VAL $collVect(name).col.stot]
	set frac(7) [VAL $collVect(name).col.split\[1\]]
	READ_VIRTUAL rtemp.virtual
    if {$stopflag == 1} {error stopped}
	fracmin
	set reflux(9) [VAL $collVect(name).col.condenser.reflux_ratio]
	set stot(9) [VAL $collVect(name).col.stot]
	set frac(9) [VAL $collVect(name).col.split\[1\]]
    }
}


# Procedure: SetupAll
proc SetupAll {} {
    
    ##########
    #Procedure:SetupAll
    #Description:Solve initial point for each feed.  Assumes
    #            the current position meets the purity
    #            specification for the first feed column
    #Arguments:none
    #Returns:none
    #Sideffects:Performs multiple solves and saves values
    ##########
    
   
    global collVect
    global comp_name
    Solve_do_Flush
    RUN $collVect(name).spec1
    ASSIGN $collVect(name).col.xsi\[$collVect(light_key)\].fixed TRUE
    ASSIGN $collVect(name).col.xsi\[$collVect(heavy_key)\].fixed TRUE
    ASSIGN $collVect(name).col.condenser.reflux_ratio.fixed FALSE
    ASSIGN $collVect(name).col.condenser.totprod.Ftot.fixed FALSE
    
    WRITE VALUES $collVect(name).col $collVect(values_dir)/$collVect(name).base1.values
    WRITE_VIRTUAL $collVect(name).col base1.virtual
    
    for {set i 2} {$i <= $collVect(nfeeds)} {incr i} {
	Solve_do_Flush
	ASSIGN $collVect(name).current_col $i
	if {[info exist keys]} {unset keys}
	if {[info exist specs]} {unset specs}
	foreach j [array names comp_name] {
	    lappend keys $collVect(name).col.feed_tray\[1\].input\['feed'\].f\[$comp_name($j)\]
	    lappend specs [VAL $collVect(name).feed\[$i\].f\[$comp_name($j)\]]
	}
	if [MultipleMeetSpec $collVect(name).col $keys $specs $collVect(delta)] {
	    WRITE VALUES $collVect(name).col $collVect(values_dir)/$collVect(name).base$i.values
	    WRITE_VIRTUAL $collVect(name).col base$i.virtual
	} else {
	    puts "solution for feed $i did not come through,"
	    puts "trying to get point from first solve and meeting recovery"
	    puts "will need your help for meeting recovery spec"
	    READ_VIRTUAL base.virtual
	    set movefeed [MultipleMeetSpec $collVect(name).col $keys $specs $collVect(delta)]
	    SetupMeetSpec
	    if {![MultipleMeetSpec $collVect(name).col $collVect(keys) $collVect(specs) $collVect(delta)]} {
		set answer [tk_dialog .continue "Continue?" {Spec not met, try \
			again?} {} 0 yes no]
		if {$answer == 0} {
		    SetupMeetSpec
		    MultipleMeetSpec $collVect(name).col $collVect(keys) $collVect(specs) $collVect(delta)
		    WRITE VALUES $collVect(name).col $collVect(values_dir)/$collVect(name).base$i.values
		    WRITE_VIRTUAL $collVect(name).col base$i.virtual
		} else {
		    puts "saving values for this feed that DO NOT meet purity specs"
		    WRITE VALUES $collVect(name).col $collVect(values_dir)/$collVect(name).base$i.values
		    WRITE_VIRTUAL $collVect(name).col base$i.virtual
		}
	    }

	}
	READ_VIRTUAL base1.virtual
    }
    set collVect(spot) "none"
    set collVect(save_set) {}
    set collVect(delta) 1.0
    set collVect(min) 0.7
    set collVect(max) 1.3
}


# Procedure: SaveValues
proc SaveValues {} {
    ##########
    #Procedure:SAVEValues
    #Description:Write out values for each feed 
    #            position from virtual saves
    #Arguments:none
    #Returns:none
    #Sideffects:none
    ##########
    
   global collVect
    for {set i 1} {$i <= $collVect(nfeeds)} {incr i} {
	READ_VIRTUAL base$i.virtual
	WRITE VALUES $collVect(name).col $collVect(values_dir)/$collVect(name).base$i.values

    }
    WRITE VALUES $collVect(name).col_set $collVect(values_dir)/$collVect(name).col_set.values
    WRITE VALUES $collVect(name).col_fit $collVect(values_dir)/$collVect(name).col_fit.values
}


# Procedure: UpdateValues
proc UpdateValues {} {
     
    ##########
    #Procedure:UpdateValues
    #Description:Read in values for each feed 
    #            position and create virtual save
    #Arguments:none
    #Returns:none
    #Sideffects:none
    ##########
    
   global collVect
    REFINE $collVect(name).col TO equilibrium_coll_column
    for {set i 1} {$i <= $collVect(nfeeds)} {incr i} {
	READ VALUES $collVect(values_dir)/$collVect(name).base$i.values
	WRITE_VIRTUAL $collVect(name).col base$i.virtual
    }
    READ VALUES $collVect(values_dir)/$collVect(name).col_set.values
    READ VALUES $collVect(values_dir)/$collVect(name).col_fit.values
}


# Procedure: FitSet
proc FitSet {} {
     
    ##########
    #Procedure:FitSet
    #Description:Put current reflux array into fitting
    #            model for current column
    #Arguments:none
    #Returns:none
    #Sideffects:Changes values of variables
    ##########
    
   
    global reflux
    global stot
    global frac
    global collVect
    
    set current [VAL $collVect(name).current_col]
    set ii 1
    foreach i [lsort [array names reflux]] {
	ASSIGN $collVect(name).col_fit\[$current\].reflux\[$ii\] $reflux($i)
	ASSIGN $collVect(name).col_fit\[$current\].stot\[$ii\] $stot($i)
	ASSIGN $collVect(name).col_fit\[$current\].frac\[$ii\] $frac($i)
	set ii [expr $ii + 1]
    }
}


# Procedure: FindSpot
proc FindSpot {} {
    
    ##########
    #Procedure:FindSpot
    #Description:Determine where current solution is for
    #             the current feed relative to bounds on
    #             number of trays and feed position
    #Arguments:none
    #Returns:none
    #Sideffects:Changes collVect(spot) and collVect(save_set)
    ##########
    

    global collVect

    set i [VAL $collVect(name).current_col]

    set stot [VAL $collVect(name).col_set.apcol\[$i\].stot]
    set frac [VAL $collVect(name).col_set.apcol\[$i\].split\[1\]]


    set low_stot [VAL $collVect(name).col_set.apcol\[$i\].stot.lower_bound]
    set low_frac [VAL $collVect(name).col_set.apcol\[$i\].split\[1\].lower_bound]
    set upp_stot [VAL $collVect(name).col_set.apcol\[$i\].stot.upper_bound]
    set upp_frac [VAL $collVect(name).col_set.apcol\[$i\].split\[1\].upper_bound]

    if {$stot == $low_stot && $frac == $low_frac} {
	set collVect(spot) ll
	set collVect(save_set) {1 2 4 5}
	puts "at lower left corner  lower bound on trays, lower bound on frac"
    } elseif {$stot == $low_stot && $frac == $upp_frac} {
	set collVect(spot) lu
	set collVect(save_set) {2 3 5 6}
	puts "at upper left corner  lower bound on trays, upper bound on frac"
    } elseif {$stot == $upp_stot && $frac == $low_frac} {
	set collVect(spot) ul
	set collVect(save_set) {4 5 7 8}
	puts "at lower right corner  upper bound on trays, lower bound on frac"
    } elseif {$stot == $upp_stot && $frac == $upp_frac} {
	set collVect(spot) uu
	set collVect(save_set) {5 6 8 9}
	puts "at upper right corner  upper bound on trays, upper bound on frac"
    } elseif {$stot == $low_stot} {
	set collVect(spot) ln
	set collVect(save_set) {1 2 3 4 5 6}
	puts "on lower bound for trays, not bounded on frac"
    } elseif {$stot == $upp_stot} {
	set collVect(spot) un
	set collVect(save_set) {4 5 6 7 8 9}
	puts "on upper bound for trays, not bounded on frac"
    } elseif {$frac == $low_frac} {
	set collVect(spot) nl
	set collVect(save_set) {1 2 4 5 7 8}
	puts "not bounded on trays, at lower bound on frac"
    } elseif {$frac == $upp_frac} {
	set collVect(spot) nu
	set collVect(save_set) {2 3 5 6 8 9}
	puts "not bounded on trays, at upper bound on frac"
    } else {
	puts "not bounded on either trays or frac!"
	set collVect(spot) nn
	set collVect(save_set) {1 2 3 4 5 6 7 8 9}
    }
}


# Procedure: Doapprox
proc Doapprox {} {
    
    ##########
    #Procedure:Doapprox
    #Description:Perform initial approximation and optimization
    #Arguments:none
    #Returns:none
    #Sideffects:Multiple solves, changes many variables
    ##########
    
    
    global collVect
    global reflux
    global stot
    global frac
    global stopflag

    set stopflag 0

    #get important stuff in probe

    PROBE current $collVect(name).col.condenser.reflux_ratio {}
    PROBE current $collVect(name).col.condenser.totprod.Ftot {}
    PROBE current $collVect(name).col.stot {}
    PROBE current $collVect(name).col.split {}
    PROBE current $collVect(name).col_set.cost {}
    PROBE current $collVect(name).col_set.condenser_cost {}
    PROBE current $collVect(name).col_set.apcol\[1\].stot {}
    for {set i 1} {$i <= $collVect(nfeeds)} {incr i} {
   
	PROBE current $collVect(name).col_set.apcol\[$i\].split {}
	PROBE current $collVect(name).col_set.apcol\[$i\].F {}
	PROBE current $collVect(name).col_set.apcol\[$i\].condenser_cost {}
	PROBE current $collVect(name).col_set.apcol\[$i\].reflux_ratio {}
	PROBE current $collVect(name).col_set.apcol\[$i\].calc_reflux_ratio {}
	PROBE current $collVect(name).col_fit\[$i\].tot_error {}
    }
	
    if {$stopflag == 1} {error stopped}

    # Get degrees of freedom right for column solve
    Solve_do_Flush

    RUN $collVect(name).cc.reset
    RUN $collVect(name).spec1
    ASSIGN $collVect(name).col.xsi\[$collVect(light_key)\].fixed TRUE
    ASSIGN $collVect(name).col.xsi\[$collVect(heavy_key)\].fixed TRUE
    ASSIGN $collVect(name).col.condenser.reflux_ratio.fixed FALSE
    ASSIGN $collVect(name).col.condenser.totprod.Ftot.fixed FALSE

    # Get initial cost point for each feed, and get reflux
    # data for each feed. 
    for {set i 1} {$i <= $collVect(nfeeds)} {incr i} {
	ASSIGN $collVect(name).current_col $i
#	RUN $collVect(name).specfeed$i
	READ_VIRTUAL base$i.virtual
	puts "getting cost info for column$i at this point"
	SOLVE $collVect(name).cc WITH Slv
	RUN $collVect(name).setapcol
	puts "casting about for approximation data for column$i"
	catch "unset reflux"
	catch "unset stot"
	catch "unset frac"
	GetRefluxData
	puts "reflux data for column$i"
	foreach j [lsort [array names reflux]] {
	    puts "set reflux($j) $reflux($j)"
	    puts "set stot($j) $stot($j)"
	    puts "set frac($j) $frac($j)"
	}
	FitSet
    if {$stopflag == 1} {error stopped}
    }

    FitOpt

    PRINT PROBE

}

# Procedure FitOpt
proc FitOpt {} {

    global collVect
    global stopflag

    set stop 0

    for {set i 1} {$i <= $collVect(nfeeds)} {incr i} {

	puts "Fitting data for column$i"
	RUN $collVect(name).col_fit\[$i\].reset
	SOLVE $collVect(name).col_fit\[$i\] WITH Slv
	RUN $collVect(name).col_fit\[$i\].free
	SOLVE $collVect(name).col_fit\[$i\] WITH MINOS
	if {![SOLST]} {
	    SOLVE $collVect(name).col_fit\[$i\] WITH MINOS
	    if {![SOLST]} {
		puts "didn't get fit for column $i"
		set stop 1
	    }   
	}
	if {$stopflag == 1} {error stopped}
    }
    if {$stop == 1} {error "one or more fits didn't converge"}
    # Setup approximation model for solving
    ASSIGN $collVect(name).col_set.min_d $collVect(min)
    ASSIGN $collVect(name).col_set.max_d $collVect(max)
    RUN $collVect(name).setup_opt
    RUN $collVect(name).col_set.reset
   
    WRITE VALUES $collVect(name).col_fit $collVect(values_dir)/$collVect(name).col_fit.values
    WRITE VALUES $collVect(name).col_set $collVect(values_dir)/$collVect(name).col_set.values
    Solve_do_Flush
    if {$stopflag == 1} {error stopped}
    #Optimize approximation model
    RUN $collVect(name).col_set.free

    ASSIGN $collVect(name).col_set.apcol\[1\].F.fixed TRUE

    SOLVE $collVect(name).col_set WITH MINOS
    if {![SOLST]} {
	SOLVE $collVect(name).col_set WITH MINOS
	if {![SOLST]} {
	    puts "cost optimization didn't converge, stopping"
	    error "cost optimization didn't converge, stopping"
	}   
    }

}


# Procedure: StepOpt
proc StepOpt {} {
    
    ##########
    #Procedure:StepOpt
    #Description:Do approximation around current point, update
    #            coefficients, optimize locally.  This assumes
    #            a starting point of a bounded local optimum
    #Arguments:none
    #Returns:none
    #Sideffects:Many solves and changing variables
    ##########
    
    global collVect
    global reflux
    global stot
    global frac
    global stopflag
    set stopflag 0

    Solve_do_Flush
    RUN $collVect(name).cc.reset
    RUN $collVect(name).spec1
    ASSIGN $collVect(name).col.xsi\[$collVect(light_key)\].fixed TRUE
    ASSIGN $collVect(name).col.xsi\[$collVect(heavy_key)\].fixed TRUE
    ASSIGN $collVect(name).col.condenser.reflux_ratio.fixed FALSE
    ASSIGN $collVect(name).col.condenser.totprod.Ftot.fixed FALSE

    set spot_info 0
    for {set i 1} {$i <= $collVect(nfeeds)} {incr i} {
	if {$stopflag == 1} {error stopped}
	puts "Moving base column$i to current design spec"
	Solve_do_Flush
	ASSIGN $collVect(name).current_col $i
#	RUN $collVect(name).specfeed$i
	FindSpot
	set spot($i) $collVect(spot)
	if {$spot($i) == "nn"} {
	    set spot_info [expr $spot_info + 1]
	}
	READ_VIRTUAL base$i.virtual
	if {[info exist keys]} {unset keys}
    	if {[info exist specs]} {unset specs}
	lappend keys $collVect(name).col.stot $collVect(name).col.split\[1\]
	lappend specs [VAL $collVect(name).col_set.apcol\[1\].stot] [VAL $collVect(name).col_set.apcol\[$i\].split\[1\]]
	set met [MultipleMeetSpec $collVect(name).col $keys $specs $collVect(delta)]
	puts $met
	if {$met == 0} {
	    if {[info exist keys]} {unset keys}
	    if {[info exist specs]} {unset specs}
	    lappend keys $collVect(name).col.split\[1\]
	    lappend specs [VAL $collVect(name).col_set.apcol\[$i\].split\[1\]]
	    set met1 [MultipleMeetSpec $collVect(name).col $keys $specs $collVect(delta)]
	    puts $met1
	    unset keys
	    unset specs
	    lappend keys $collVect(name).col.stot 
	    lappend specs [VAL $collVect(name).col_set.apcol\[1\].stot] 
	    set met2 [MultipleMeetSpec $collVect(name).col $keys $specs $collVect(delta)]
	    puts $met2
	    set met [expr $met1 + $met2]
	    puts $met
	
	    if {$met != 2} {
		puts "$met1 $met2 $met"
		puts "sorry, column $i cannot move to current point"
		return 0
	    }
	}
	WRITE_VIRTUAL $collVect(name).col base$i.virtual
	puts "getting cost info for column$i at this point"
	ASSIGN $collVect(name).cc.cost_calc.F\[1\] [VAL $collVect(name).col_set.apcol\[$i\].F]
	SOLVE $collVect(name).cc WITH Slv
	puts "column [VAL $collVect(name).col.condenser.reflux_ratio]"
	puts "approx [VAL $collVect(name).col_set.apcol\[[VAL $collVect(name).current_col]\].reflux_ratio]"
	RUN $collVect(name).setapcol
	puts "casting about for approximation data for column$i"
	catch "unset reflux"
	catch "unset stot"
	catch "unset frac"
	GetRefluxData

	puts "reflux data for column$i"
#	foreach j [lsort [array names reflux]] {
#	    puts "set reflux($j) $reflux($j)"
#	    puts "set stot($j) $stot($j)"
#	    puts "set frac($j) $frac($j)"
#	}
	FitSet
	
    }

    FitOpt

    set spot_info 0
    for {set i 1} {$i <= $collVect(nfeeds)} {incr i} {
	ASSIGN $collVect(name).current_col $i
	set spot($i) $collVect(spot)
	if {$spot($i) == "nn"} {
	    set spot_info [expr $spot_info + 1]
	}
	FindSpot
    }
    
    if {$spot_info == $collVect(nfeeds)} {
	puts "Nobody on bounds! Achieved local optimium with current approximation"
	return 2
    }

    return 1
}


# Procedure: DoOptimize
proc DoOptimize {} {

    ##########
    #Procedure:DoOptimize
    #Description:Do one series of StepOpt procedure
    #Arguments:none
    #Returns:0 - if problem making step
    #        1 - if iteration limit hit
    #        2 - if at local optimum and not on bounds
    #Sideffects:none
    ##########
    

    global collVect
    global stopflag
    set stopflag 0

    set ii 0
    set step 1

    while {($ii <= 10) && ($step == 1)} {
    if {$stopflag == 1} {error stopped}
	set ii [expr $ii + 1]
	set step [StepOpt]
	PRINT PROBE
    }

    if {$step == 0} {
	puts "troubles"
	return 0
    } elseif {$step == 1} {
	puts "iteration limit hit"
	return 1
    } elseif {$step == 2} {
	puts "local optimum found."
	return 2
    }
}


# Procedure: CompleteOptimize
proc CompleteOptimize {} {

    ##########
    #Procedure:CompleteOptimize
    #Description:Automate optimization of flexible
    #            distillation problem
    #Arguments:none
    #Returns:0 - if problem making step
    #        1 - if iteration limit hit and not continued
    #        2 - if at local optimum after tightening
    #Sideffects:none
    ##########
    
    global collVect
    global stopflag

    set stopflag 0
    set collVect(spot) "none"
    set collVect(save_set) {}
    set collVect(delta) 1.0


    Doapprox
    if {$stopflag == 1} {error stopped}

    set result [DoOptimize]

    while {$result == 1} {
    if {$stopflag == 1} {error stopped}
	set answer [tk_dialog .continue "Iteration Limit" "Do you want to
	continue?" {} 0 yes no]
	if {$answer == 0} {
	    set result DoOptimize
	} else {
	    return 1
	}
    }
    if {$result == 0} {
	return 0
    } elseif {$result == 2} {
	puts "Tightening approximation"
	set collVect(spot) "none"
	set collVect(save_set) {}
	set collVect(delta) 1.0
	set collVect(min) 0.9
	set collVect(max) 1.1
    if {$stopflag == 1} {error stopped}
	
	Doapprox
	set result [DoOptimize]
	return $result
    }
}


# Procedure: FlexibleMenu
proc FlexibleMenu {} {
    
    ##########
    #Procedures:FlexibleMenu
    #Description:(Re)Create menu item for all procedures
    #Arguments:none
    #Returns:none
    #Sideffects:Creates widget
    ##########
    
    catch "destroy .designtop.frame0.flexible"
    
    global moduleList
    menubutton .designtop.frame0.flexible  -activebackground {#fef9b508b508}  -activeforeground {#000000000000}  -background {#fef9e3e1c413}  -disabledforeground {#fef9b508b508}  -foreground {#000000000000}  -menu {.designtop.frame0.flexible.m}  -text {Flexible }  -underline {0}
    
    menu .designtop.frame0.flexible.m  -activebackground {#fef9b508b508}  -activeforeground {#000000000000}  -background {#fef9e3e1c413}  -disabledforeground {#fef9b508b508}  -foreground {#000000000000}
    
    foreach i $moduleList(flexible.tcl) {
	.designtop.frame0.flexible.m add command  -command $i  -label $i
    }
    pack append .designtop.frame0 .designtop.frame0.flexible {left frame center}
}


# Internal procedures

# eof
#

