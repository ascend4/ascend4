##                        procedures.tcl
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

#  $Date: 1996/05/24 17:46:31 $
#  $Revision: 1.2 $
#  $Author: mthomas $
#  $Source: /afs/cs.cmu.edu/project/ascend/Repository/models/examples/flexible_design/procedures.tcl,v $

# Module: procedures.tcl
# Tcl version: 7.3 (Tcl/Tk/XF)
# Tk version: 3.6
# XF version: 2.3
#

# module contents
global moduleList
global autoLoadList
set moduleList(procedures.tcl) { ComponentsList CheckComponents WriteGeneral IncreaseTrays CopyElements DeleteElements SetFlows SetRecovery Setcolor Setcolors Setglobals Setnfeeds RefluxWrite ProcedureMenu}
set autoLoadList(procedures.tcl) {0}

# procedures to show toplevel windows


# User defined procedures


# Procedure: ComponentsList
proc ComponentsList {} {
    
    ##########
    #Procedure:ComponentsList
    #Description:List components currently checkable for azeotropes
    #Arguments:none
    #Returns:none
    #Sideffects:none
    ##########
    
    set all_components {
	acetone         benzene         chloro_benzene  chloroform
	diethylbenzene  diphenyl        ethanol         ethylbenzene
	ethylene_glycol i_butane        i_butanol       i_pentane
	i_propanol      methanol        n_butane        n_butanol
	n_decane        n_heptane       n_hexane        n_octane
	n_pentane       n_propanol      pentanol_1      propane
	propylene       styrene         toluene         water
    }
	
	puts $all_components
}


# Procedure: CheckComponents
proc CheckComponents {} {
    
    ##########
    #Procedure:CheckComponents
    #Description:Checks component set for azeotropes and liq-liq behavior
    #Arguments:none
    #Returns:none
    #Sideffects:Outputs information about components, sets up some global
    #           variables
    ##########
    

    global collVect
    
    set comp_error 0
    set all_components {
	acetone         benzene         chloro_benzene  chloroform
	diethylbenzene  diphenyl        ethanol         ethylbenzene
	ethylene_glycol i_butane        i_butanol       i_pentane
	i_propanol      methanol        n_butane        n_butanol
	n_decane        n_heptane       n_hexane        n_octane
	n_pentane       n_propanol      pentanol_1      propane
	propylene       styrene         toluene         water
    }
    foreach i $collVect(components) {
	if {[lsearch $all_components $i] == -1} {
	    puts component\ $i\ is\ not\ in\ the\ list\ of\ components.
	    puts "Check the ComponentsList again and check for errors."
	    set comp_error $comp_error+1   
	}
    } 
    if {$comp_error > 0} {return}
    global comp
    global comp_name
    
    if {[info exists comp]} {unset comp}
    if {[info exists comp_name]} {unset comp_name}
    if {[info exists collVect(tbs)]} {unset collVect(tbs)}
    if {[info exists collVect(complist)]} {unset collVect(complist)}
    if {[info exists collVect(ncomps)]} {unset collVect(ncomps)}
    global comp
    global comp_name
    global collVect(tbs)
    global collVect(complist)
    global collVect(ncomps)
    if {[info exists collVect(components)]} {
	set tb_list(acetone) 329.4 
	set tb_list(benzene) 353.252 
	set tb_list(chloro_benzene) 404.9 
	set tb_list(chloroform) 334.3 
	set tb_list(diethylbenzene) 456.9 
	set tb_list(diphenyl) 529.3 
	set tb_list(ethanol) 351.5 
	set tb_list(ethylbenzene) 409.3 
	set tb_list(ethylene_glycol) 470.5 
	set tb_list(i_butane) 261.424 
	set tb_list(i_butanol) 381 
	set tb_list(i_pentane) 300.999 
	set tb_list(i_propanol) 355.4 
	set tb_list(methanol) 337.8 
	set tb_list(n_butane) 272.665 
	set tb_list(n_butanol) 390.9 
	set tb_list(n_decane) 447.3 
	set tb_list(n_heptane) 371.6 
	set tb_list(n_hexane) 341.887 
	set tb_list(n_octane) 398.8 
	set tb_list(n_pentane) 309.187 
	set tb_list(n_propanol) 370.4 
	set tb_list(pentanol_1) 411.1 
	set tb_list(propane) 229.991 
	set tb_list(propylene) 225.423 
	set tb_list(styrene) 418.3 
	set tb_list(toluene) 383.776 
	set tb_list(water) 373.15 
	source $collVect(source_dir)/azeo_data.tcl
	puts ""
	set sub_components $collVect(components)
	foreach i $collVect(components) {
	    set sub_components [lreplace $sub_components  0 0]
	    foreach j $sub_components {
		case $azeo($i,$j) {
		    {min} {puts "$i and $j will form a minimum boiling azeotrope."}
		    {max} {puts "$i and $j will form a maximum boiling azeotrope."}
		    {min hetero} {puts "$i and $j will form a maximum boiling azeotrope"
		    puts "and will demonstrate heterogeneous behavior."
		    puts "The thermo library will not accurately model this system."}
		    {unconv} {puts "$i and $j may cause problems with the thermodynamics."}
		}
	    }
	    append collVect(tbs) $tb_list($i)\ 
	}
	set ii 0
	foreach t [lsort $collVect(tbs)] {
	    set ii [expr $ii+1]
	    foreach i $collVect(components) {
		if {$t == $tb_list($i)} {
		    set comp($ii) $i
		}
	    }
	}
	puts "Your components will be numbered in decreasing volatility:"
	unset collVect(components)
	foreach i [lsort [array names comp]] {
	    set comp_name($i) c$i\_$comp($i) 
	    append collVect(complist) '$comp_name($i)',
	    lappend collVect(components) $comp($i)
	    puts $comp_name($i)
	}
    } else {
	puts "You haven't defined the set of components yet"
    }
    set collVect(complist) [string trim $collVect(complist) ,]
    set collVect(ncomps) $ii
}


# Procedure: WriteGeneral
proc WriteGeneral {} {
    
    ##########
    #Procedure:WriteGeneral
    #Description:Write ASCEND model for current settings
    #Arguments:none
    #Returns:none
    #Sideffects:none
    ##########
    

    global collVect
    global comp
    global comp_name
    global feed
    
    set dist 0
    foreach i [lsort [array names comp]] {
	set dist [expr [lindex $feed(1,$comp_name($i)) 0] + $dist]
	if {$comp_name($i) == $collVect(light_key)} {
	    break
	}
    }
    set distillate $dist\ \{mol/s\}
    
    # 
    
    
    if {[file exists $collVect(filename)]} {
	puts "$collVect(filename) exists. Appending model $collVect(name) to $collVect(filename)" 
    } else {
	puts "Creating $collVect(filename) and writing $collVect(name) to it"
    }
    set fid [open $collVect(filename) a+];
    
    puts $fid "MODEL $collVect(name) REFINES standard_cost\;"
    puts $fid "\tcol.components := \[$collVect(complist)\]\;"
    puts $fid "\tcol.coll_stack\[1..col.nfeeds+1\].inactive_component := '$comp_name(2)'\;"
    foreach i [lsort [array names comp]] {
	puts $fid "\tcol.data\['$comp_name($i)'\] IS_REFINED_TO $comp($i)\;"
    }
    puts $fid "\tnfeed_points := $collVect(nfeeds)\;"
    puts $fid {
	METHODS
	METHOD values;
    }
    foreach i [lsort [array names comp]] {
	puts $fid "\tcol.feed_tray\[1\].alpha\['$comp_name($i)'\] := [expr 1 + [expr $collVect(ncomps) -$i] * 0.2]\;"
    }
    
    foreach i [lsort [array names comp]] {
	puts $fid "\tcol.feed_tray\[1\].input\['feed'\].f\['$comp_name($i)'\] := $feed(1,$comp_name($i))\;"
    }
    for {set j 1} {$j <= $collVect(nfeeds)} {incr j} {
	foreach i [lsort [array names comp]] {
	    puts $fid "\tfeed\[$j\].f\['$comp_name($i)'\] := $feed($j,$comp_name($i))\;"
	}
    }
    puts $fid "\tcol.condenser.totprod.Ftot := $distillate\;"
    puts $fid {
	RUN col.propogate_feed;
	FOR j IN [1..2] DO
	col.coll_stack[j].split[1] := 0.5;
	col.coll_stack[j].stot := 2;
	col.coll_stack[j].coll[1].z_set.stot := 1;
	col.coll_stack[j].coll[1..col.coll_stack[1].ncolls].z_set.a := 0.1;
	END;
	col.feed_tray[1].q := 1.0;
	col.condenser.prodsplit['vapor_product'] := 0.0;
	col.reboiler.prodsplit['vapor_product'] := 0.0;
	col.condenser.reflux_ratio := 0.5;
	END values;
    }
    puts $fid "END $collVect(name)\;"
    close $fid
}


# Procedure: CopyElements
proc CopyElements { to args} {
    
    ##########
    #Procedure:CopyElements
    #Description:Copies args or current selection to list
    #Arguments:to -name of listbox to be appended
    #          args -strings or set of strings to be appended to list
    #Returns:none
    #Sideffects:none
    ##########
    
    if {$args != ""} {
	foreach i [split $args " "] {
	    $to insert end [string trim $i {\{ \}}]
	}
    } else {
	foreach i [selection get] {
	    $to insert end $i
	}
    }
}


# Procedure: DeleteElements
proc DeleteElements { from} {
    
    ##########
    #Procedure:DeleteElements
    #Description:Delete selection from list
    #Arguments:from -name of list to be shortened
    #Returns:none
    #Sideffects:none
    ##########
    
    set first [lindex [$from curselection] 0]
    foreach i [$from curselection] {
	$from delete $first
    }
}


# Procedure: SetFlows
proc SetFlows {} {
    
    ##########
    #Procedure:SetFlows
    #Description:Create window for setting feed flowrates
    #Arguments:none
    #Returns:none
    #Sideffects:Changes global variables describing feed flowrates
    ##########
    
    catch "destroy .setfeed"
    
    global collVect
    global feed
    global flow
    global units
    global comp
    global comp_name
    
    if [info exist feed] {unset feed}
    
    global feed
    global flow
    global units
    global bobcolor
    
    toplevel .setfeed 
    
    # Window manager configurations
    global tkVersion
    wm positionfrom .setfeed ""
    wm sizefrom .setfeed user
    wm maxsize .setfeed 1000 1000
    wm minsize .setfeed 10 10
    wm title .setfeed setfeed

    .setfeed configure -bg $bobcolor(base)
    
    
    for {set j 1} {$j <= $collVect(nfeeds)} {incr j} {
	
	frame .setfeed.feed$j  -background $bobcolor(base)
	
	label .setfeed.feed$j.label  -text "Feed $j"  -bg $bobcolor(base)   -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*"   -fg $bobcolor(dark)
	
	frame .setfeed.feed$j.lbl_frm -background $bobcolor(base)
	frame .setfeed.feed$j.flow_frm -background $bobcolor(base)
	frame .setfeed.feed$j.unit_frm -background $bobcolor(base)
	
	
	foreach i [lsort [array names comp]] {
	    if {![info exists flow($j,$i)]} {
		set flow($j,$i) 3.0
	    }
	    if {![info exists units($j,$i)]} {
		set units($j,$i) "mole/s"
	    }
	    	    
	    # build widget .setfeed.complabel$j$i
	    label .setfeed.feed$j.lbl_frm.complabel$i   -text $comp($i)  -textvariable $comp($i)  -bg $bobcolor(base)  -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*"  -fg $bobcolor(dark)
	    
	    # build widget .setfeed.frame$i.flow$j$i
	    entry .setfeed.feed$j.flow_frm.flow$i   -relief {sunken}   -textvariable flow($j,$i)   -width {11} -bg $bobcolor(base)  -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*"  -fg $bobcolor(dark)  -insertbackground $bobcolor(light)  -selectbackground $bobcolor(light)  -selectforeground $bobcolor(dark)
	    
	    
	    # build widget .setfeed.frame$i.units$j$i
	    entry .setfeed.feed$j.unit_frm.units$i   -relief {sunken}   -textvariable units($j,$i)   -width {18}  -bg $bobcolor(base)  -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*"  -fg $bobcolor(dark)  -insertbackground $bobcolor(light)  -selectbackground $bobcolor(light)  -selectforeground $bobcolor(dark)
	    
	}
	
	# pack widget .setfeed
	
	pack append .setfeed .setfeed.feed$j {top frame center fill}
	pack append .setfeed.feed$j .setfeed.feed$j.label {top frame center fill expand}
	pack append .setfeed.feed$j .setfeed.feed$j.lbl_frm {left frame center fill}
	pack append .setfeed.feed$j .setfeed.feed$j.flow_frm {left frame center expand fill}
	pack append .setfeed.feed$j .setfeed.feed$j.unit_frm {left frame center expand fill}
	
	
	foreach i [lsort [array names comp]] {
	    
	    pack append .setfeed.feed$j.lbl_frm  .setfeed.feed$j.lbl_frm.complabel$i {top frame center expand fill}
	    pack append .setfeed.feed$j.flow_frm .setfeed.feed$j.flow_frm.flow$i {top frame center expand fill}
	    pack append .setfeed.feed$j.unit_frm .setfeed.feed$j.unit_frm.units$i {top frame center expand fill}   
	}
    }
    frame .setfeed.buttons -bg $bobcolor(base)
    
    
    # build widget .setfeed.buttons.set
    button .setfeed.buttons.set   -command {for {set j 1} {$j <= $collVect(nfeeds)} {incr j} {foreach i [lsort [array names comp]]  {  set feed($j,$comp_name($i)) "$flow($j,$i) {$units($j,$i)}"}};  destroy .setfeed}   -text "  ok  "  -activebackground $bobcolor(light)  -activeforeground $bobcolor(dark)  -background $bobcolor(base)  -disabledforeground $bobcolor(light)  -foreground $bobcolor(dark)  -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*" 
    
    
    pack append .setfeed .setfeed.buttons {bottom frame center}
    pack append .setfeed.buttons .setfeed.buttons.set {left frame w padx 20}
}


# Procedure: SetRecovery
proc SetRecovery {} {
    
    ##########
    #Procedure:SetRecovery
    #Description:Create window to set recovery specifications 
    #            for component set
    #Arguments:none
    #Returns:none
    #Sideffects:Changes some global variables for recovery specs
    ##########
    
    catch "destroy .setrecover"

    global comp
    global comp_name
    global bobcolor
    toplevel .setrecovery 
    
    # Window manager configurations
    global tkVersion
    wm positionfrom .setrecovery ""
    wm sizefrom .setrecovery ""
    wm maxsize .setrecovery 1000 1000
    wm minsize .setrecovery 10 10
    wm title .setrecovery {Set Recovery Specs}
    
    .setrecovery configure -bg $bobcolor(base)
    
    # build widget .setrecovery.buttons
    frame .setrecovery.buttons  -borderwidth {2} -bg $bobcolor(base)

    
    # build widget .setrecovery.buttons.set
    button .setrecovery.buttons.set  -command {
	if {$collVect(light_key) != $collVect(heavy_key)} {
	    if {[lindex [lsort [list $collVect(light_key) $collVect(heavy_key)]] 0] == $collVect(light_key)} {
		if {$collVect(light_spec) > $collVect(heavy_spec)} {
		    puts "$collVect(light_key) $collVect(light_spec), $collVect(heavy_key) $collVect(heavy_spec)"
		    destroy .setrecovery
		} else {
		    set answer [tk_dialog .badspec "Bad Specification" {Your heavy key  specification is greater  than your light key  specification. Please  change your specifications.} {} 0 ok "use this spec!"]
		    if {$answer == 1} {
			puts "$collVect(light_key) $collVect(light_spec), $collVect(heavy_key) $collVect(heavy_spec)"
			destroy .setrecovery
		    }
		}
	    } else {
		set answer [tk_dialog .badspec "Bad Specification" {You have chosen a  light key that is less volatile  than your heavy key. Please  change your choices for light  and heavy key.} {} 0 ok "use this spec!"]
		if {$answer == 1} {
		    puts "$collVect(light_key) $collVect(light_spec), $collVect(heavy_key) $collVect(heavy_spec)"
		    destroy .setrecovery
		}
	    }
	} else {
	    tk_dialog .badspec "Bad Specification" {You have chosen  the same component for your  light key and heavy key.  Please change your specifications.} {} 0 ok
	}
    }  -text ok  -activebackground $bobcolor(light)  -activeforeground $bobcolor(dark)  -background $bobcolor(base)  -disabledforeground $bobcolor(light)  -foreground $bobcolor(dark)  -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*" 
    
    
    
    # pack widget .setrecovery.buttons
    pack append .setrecovery.buttons  .setrecovery.buttons.set {left frame center padx 24 pady 7}  
    
    # build widget .setrecovery.heavykey
    frame .setrecovery.heavykey  -borderwidth {2}  -relief {groove} -background $bobcolor(base)
    
    # build multiple widgets
	foreach i [lsort [array names comp]] {
	    # build widget .setrecovery.heavykey.comp1
	    radiobutton .setrecovery.heavykey.comp$i  -text $comp_name($i)  -variable collVect(heavy_key)  -value $comp_name($i)  -activebackground $bobcolor(light)  -activeforeground $bobcolor(dark)  -background $bobcolor(base)  -disabledforeground $bobcolor(light)  -foreground $bobcolor(dark)  -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*" 
	    
	    
	}
	
	# build widget .setrecovery.heavykey.keylabel
	label .setrecovery.heavykey.keylabel  -text {Heavy Key:}  -bg $bobcolor(base)  -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*"  -fg $bobcolor(dark)
	
	# build widget .setrecovery.heavykey.keyspec
	entry .setrecovery.heavykey.keyspec  -relief {sunken}  -textvariable {collVect(heavy_spec)}  -width {9}  -bg $bobcolor(base)  -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*"  -fg $bobcolor(dark)  -insertbackground $bobcolor(light)  -selectbackground $bobcolor(light)  -selectforeground $bobcolor(dark)
	
	
	# pack widget .setrecovery.heavykey
	pack append .setrecovery.heavykey  .setrecovery.heavykey.keylabel {left frame center padx 21}  .setrecovery.heavykey.keyspec {right frame center padx 23}
	
	#packit
	foreach i [lsort [array names comp]] {
	    pack append .setrecovery.heavykey  .setrecovery.heavykey.comp$i {top frame center pady 6}  
	}
	
	# build widget .setrecovery.instructions
	message .setrecovery.instructions  -aspect {1500}  -justify {center}  -padx {5}  -pady {2}  -text {Select light key and heavy key and set
	fraction of the feed you want the distillate.}  -bg $bobcolor(base)  -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*"  -fg $bobcolor(dark)
	
	
	# build widget .setrecovery.lightkey
	frame .setrecovery.lightkey  -borderwidth {2}  -relief {groove} -bg $bobcolor(base)
	foreach i [lsort [array names comp]] {
	    # build widget .setrecovery.lightkey.comp2
	    radiobutton .setrecovery.lightkey.comp$i  -text $comp_name($i)  -variable collVect(light_key)  -value $comp_name($i)  -activebackground $bobcolor(light)  -activeforeground $bobcolor(dark)  -background $bobcolor(base)  -disabledforeground $bobcolor(light)  -foreground $bobcolor(dark)  -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*" 
	    
	    
	}
	
	# build widget .setrecovery.lightkey.keylabel
	label .setrecovery.lightkey.keylabel  -text {Light Key:}  -bg $bobcolor(base)  -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*"  -fg $bobcolor(dark)
	
	# build widget .setrecovery.lightkey.keyspec
	entry .setrecovery.lightkey.keyspec  -relief {sunken}  -textvariable {collVect(light_spec)}  -width {9}  -bg $bobcolor(base)  -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*"  -fg $bobcolor(dark)  -insertbackground $bobcolor(light)  -selectbackground $bobcolor(light)  -selectforeground $bobcolor(dark)
	
	# pack widget .setrecovery.lightkey
	pack append .setrecovery.lightkey  .setrecovery.lightkey.keylabel {left frame center padx 21}  .setrecovery.lightkey.keyspec {right frame center padx 23}
	foreach i [lsort [array names comp]] {
	    pack append .setrecovery.lightkey  .setrecovery.lightkey.comp$i {top frame center pady 6} 
	}
	
	
	
	# pack widget .setrecovery
	pack append .setrecovery  .setrecovery.lightkey {top frame center pady 6}  .setrecovery.heavykey {top frame center pady 6}  .setrecovery.instructions {top frame center}  .setrecovery.buttons {top frame center}
}


# Procedure: Setcolor
proc Setcolor { colorname} {
    
    ##########
    #Procedure:Setcolor
    #Description:Bring up ColorBox to set the color
    #Arguments:colorname -color entry to set
    #Returns:none
    #Sideffects:sets the color
    ##########
    
  
    global bobcolor
    
    set bobcolor($colorname) [ColorBox]
}


# Procedure: Setcolors
proc Setcolors {} {
    
    ##########
    #Procedure:Setcolors
    #Description:Create window for setting color variables
    #Arguments:none
    #Returns:none
    #Sideffects:none
    ##########
    
  
    global bobcolor

    # build widget .setcolors
    if {"[info procs XFEdit]" != ""} {
	catch "XFDestroy .setcolors"
    } {
	catch "destroy .setcolors"
    }
  toplevel .setcolors -background $bobcolor(base)
    
    # Window manager configurations
    global tkVersion
  wm positionfrom .setcolors program
    wm sizefrom .setcolors program
    wm maxsize .setcolors 998 1024
    wm minsize .setcolors 94 9
    wm title .setcolors {Set Colors}
    
    button .setcolors.apply -command {bobcolors
    destroy .setcolors} -text "Apply" -activebackground $bobcolor(light)  -activeforeground $bobcolor(dark)  -background $bobcolor(base)  -disabledforeground $bobcolor(light)  -foreground $bobcolor(dark)  -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*" 
    
    pack append .setcolors .setcolors.apply {bottom frame center}
    
    
    # build widget .setcolors.lbl_frm
    frame .setcolors.lbl_frm  -borderwidth {2}
    
    # build widget .setcolors.val_frm
    frame .setcolors.val_frm  -borderwidth {2} -background $bobcolor(base)
    
    foreach i [lsort [array names bobcolor]] {
	label .setcolors.lbl_frm.$i  -text "$i color"  -bg $bobcolor(base)  -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*"  -fg $bobcolor(dark)
	
	
	pack append .setcolors.lbl_frm  .setcolors.lbl_frm.$i {top frame center expand fill} 
	
	entry .setcolors.val_frm.$i  -relief {sunken}  -textvariable "bobcolor($i)"  -bg $bobcolor(base)  -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*"  -fg $bobcolor(dark)  -insertbackground $bobcolor(light)  -selectbackground $bobcolor(light)  -selectforeground $bobcolor(dark)
	
	
	# pack widget .setcolors.$i
	pack append .setcolors.val_frm  .setcolors.val_frm.$i {top frame center expand fill} 
	bind .setcolors.val_frm.$i <Key-Return> {#nothing}  
	bind .setcolors.val_frm.$i <Control-Button-1> "Setcolor $i"
    }
    # pack widget .setcolors
    pack append .setcolors  .setcolors.lbl_frm {left frame center fill}  .setcolors.val_frm {left frame center expand fill}
    
    if {"[info procs XFEdit]" != ""} {
	catch "XFMiscBindWidgetTree .setcolors"
	after 2 "catch {XFEditSetShowWindows}"
    }
}


# Procedure: Setglobals
proc Setglobals {} {
     
    ##########
    #Procedure:Setgpbals
    #Description:Create window for setting variables in collVect
    #Arguments:none
    #Returns:none
    #Sideffects:none
    ##########
    
   
    global collVect
    global bobcolor
    
  # build widget .setglobals
    if {"[info procs XFEdit]" != ""} {
	catch "XFDestroy .setglobals"
    } {
	catch "destroy .setglobals"
    }
    toplevel .setglobals -background $bobcolor(base)
    
    # Window manager configurations
    global tkVersion
    wm positionfrom .setglobals program
    wm sizefrom .setglobals program
    wm maxsize .setglobals 998 1024
    wm minsize .setglobals 94 9
    wm title .setglobals {Set Global Variables}
    
    button .setglobals.apply -command {destroy .setglobals} -text "Ok" -activebackground $bobcolor(light)  -activeforeground $bobcolor(dark)  -background $bobcolor(base)  -disabledforeground $bobcolor(light)  -foreground $bobcolor(dark)  -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*" 
    
    pack append .setglobals .setglobals.apply {bottom frame center}
    
    
  # build widget .setglobals.lbl_frm
    frame .setglobals.lbl_frm  -borderwidth {2} -background $bobcolor(base)
    
    # build widget .setglobals.val_frm
    frame .setglobals.val_frm  -borderwidth {2} -background $bobcolor(base) 
    
    foreach i [lsort [array names collVect]] {
	label .setglobals.lbl_frm.$i  -text "collVect($i)"  -bg $bobcolor(base)  -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*"  -fg $bobcolor(dark)
	
	
	pack append .setglobals.lbl_frm  .setglobals.lbl_frm.$i {top frame center expand fill} 
	
	entry .setglobals.val_frm.$i  -relief {sunken}  -textvariable "collVect($i)" -bg $bobcolor(base)  -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*"  -fg $bobcolor(dark)  -insertbackground $bobcolor(light)  -selectbackground $bobcolor(light)  -selectforeground $bobcolor(dark)
	
	# pack widget .setglobals.$i
	pack append .setglobals.val_frm  .setglobals.val_frm.$i {top frame center expand fill} 
	bind .setglobals.val_frm.$i <Key-Return> {#nothing}  
	bind .setglobals.val_frm.$i <Control-Button-1> "Setcolor $i"
    }
    # pack widget .setglobals
    pack append .setglobals  .setglobals.lbl_frm {left frame center fill}  .setglobals.val_frm {left frame center expand fill}
    
    if {"[info procs XFEdit]" != ""} {
	catch "XFMiscBindWidgetTree .setglobals"
	after 2 "catch {XFEditSetShowWindows}"
    }
}


# Procedure: Setnfeeds
proc Setnfeeds {} {
    
    ##########
    #Procedure:Setnfeeds
    #Description:Create window for choosing number of feeds
    #Arguments:none
    #Returns:none
    #Sideffects:Will run SetFlows when closed
    ##########
    
    catch "destroy .setnfeeds"

    global collVect
    global bobcolor
    
    toplevel .setnfeeds 
    
    # Window manager configurations
    global tkVersion
    wm positionfrom .setnfeeds ""
    wm sizefrom .setnfeeds user
    wm maxsize .setnfeeds 1000 1000
    wm minsize .setnfeeds 10 10
    wm title .setnfeeds setnfeeds

    .setnfeeds configure -bg $bobcolor(base)
    
    label .setnfeeds.label -text "Set number of feed streams" -textvariable flowmessage -bg $bobcolor(base)  -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*"  -fg $bobcolor(dark)
    
    entry .setnfeeds.entry -relief {sunken}   -textvariable collVect(nfeeds)   -width {11} -bg $bobcolor(base)  -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*"  -fg $bobcolor(dark)  -insertbackground $bobcolor(light)  -selectbackground $bobcolor(light)  -selectforeground $bobcolor(dark)
    
    button .setnfeeds.button  -text "  ok  "  -activebackground $bobcolor(light)  -activeforeground $bobcolor(dark)  -background $bobcolor(base)  -disabledforeground $bobcolor(light)  -foreground $bobcolor(dark)  -font "-Adobe-Helvetica-Bold-R-Normal--*-120-*"  -command {if [IsaInt $collVect(nfeeds)] {
	set flowmessage {Set number of feed streams}
	destroy .setnfeeds
	SetFlows
    } else {
	set flowmessage {Please input an integer value!}}
    }
    
    pack append .setnfeeds .setnfeeds.label {top frame center expand fill}
    pack append .setnfeeds .setnfeeds.entry {top frame center expand fill}
    pack append .setnfeeds .setnfeeds.button {top frame center expand fill}
}


# Procedure: RefluxWrite
proc RefluxWrite { filename name} {
    
    ##########
    #Procedure:RefluxWrite
    #Description:Create model for fitting current reflux data
    #Arguments:filename - name of file for model to be written to
    #          name - name for model
    #Returns:none
    #Sideffects:none
    ##########
    
    
    global reflux
    
    if {[file exists $filename]} {
	puts "$filename exists. Appending model $name to $filename" 
    } else {
	puts "Creating $filename and writing $name to it"
    }
    
    set fid [open $filename a+]
    
    puts $fid "MODEL $name REFINES approx;"
    puts $fid "fit.npoints := [array size reflux];"
    puts $fid "fit.order := 2;"
    puts $fid {
	METHODS
      METHOD values;}
      set ii 1
      foreach i [lsort [array names reflux]] {
	  puts $fid "	fit.reflux\[$ii\] := $reflux($i);"
	  puts $fid "	fit.stot\[$ii\] := [lindex [split $i ,] 0];"
	  puts $fid "	fit.frac\[$ii\] := [lindex [split $i ,] 1];"
	  set ii [expr $ii + 1]
 }
 puts $fid "	END values;"
 puts $fid "END $name;"
 close $fid
}


# Procedure: ProcedureMenu
proc ProcedureMenu {} {
    
    ##########
    #Procedure:ProcedureMenu
    #Description:(Re)Create menu item for all procedures
    #Arguments:none
    #Returns:none
    #Sideffects:Creates widget
    ##########
    
    catch "destroy .designtop.frame0.procedures"
    
    global moduleList
    menubutton .designtop.frame0.procedures  -activebackground {#fef9b508b508}  -activeforeground {#000000000000}  -background {#fef9e3e1c413}  -disabledforeground {#fef9b508b508}  -foreground {#000000000000}  -menu {.designtop.frame0.procedures.m}  -text {Procedures }  -underline {0}
    
    menu .designtop.frame0.procedures.m  -activebackground {#fef9b508b508}  -activeforeground {#000000000000}  -background {#fef9e3e1c413}  -disabledforeground {#fef9b508b508}  -foreground {#000000000000}
    
    foreach i $moduleList(procedures.tcl) {
	.designtop.frame0.procedures.m add command  -command $i  -label $i
    }
    pack append .designtop.frame0 .designtop.frame0.procedures {left frame center}
}

proc SetupMeetSpec {} {
    global collVect

    set choice [expr 1+ [tk_dialog .setchoice "Choose Setup" {Select meetspec setup
    1 = Meet recoveries and slopes
    2 = Meet recoveries holding slopes constant
    3 = Meet recoveries holding number of trays constant
    Note, each method will first try to meet both at the same
    time and then meet each individually} {} 0 1 2 3]]
    Solve_do_Flush
    RUN $collVect(name).col.reset
    if {$choice == 1} {
	set collVect(delta) 0.3
	ASSIGN $collVect(name).col.xsi\[$collVect(light_key)\].fixed TRUE
	ASSIGN $collVect(name).col.xsi\[$collVect(heavy_key)\].fixed TRUE
	ASSIGN $collVect(name).col.condenser.reflux_ratio.fixed FALSE
	ASSIGN $collVect(name).col.condenser.totprod.Ftot.fixed FALSE
	ASSIGN $collVect(name).col.coll_stack\[1\].coll\[2\].slope\[$collVect(light_key)\].fixed TRUE {}
	ASSIGN $collVect(name).col.coll_stack\[2\].coll\[1\].slope\[$collVect(heavy_key)\].fixed TRUE {}
	ASSIGN $collVect(name).col.s_stack\[1\].fixed FALSE {}
	ASSIGN $collVect(name).col.s_stack\[2\].fixed FALSE {}
	if {[info exist collVect(keys)]} {unset collVect(keys)}
	if {[info exist collVect(specs)]} {unset collVect(specs)}
	lappend collVect(keys) $collVect(name).col.xsi\[$collVect(light_key)\] $collVect(name).col.xsi\[$collVect(heavy_key)\] $collVect(name).col.coll_stack\[1\].coll\[2\].slope\[$collVect(light_key)\] $collVect(name).col.coll_stack\[2\].coll\[1\].slope\[$collVect(heavy_key)\]
	lappend collVect(specs) $collVect(light_spec) $collVect(heavy_spec) 0.01 0.01
    } elseif {$choice == 2} {
	set collVect(delta) 0.3
	ASSIGN $collVect(name).col.xsi\[$collVect(light_key)\].fixed TRUE
	ASSIGN $collVect(name).col.xsi\[$collVect(heavy_key)\].fixed TRUE
	ASSIGN $collVect(name).col.condenser.reflux_ratio.fixed FALSE
	ASSIGN $collVect(name).col.condenser.totprod.Ftot.fixed FALSE
	ASSIGN $collVect(name).col.coll_stack\[1\].coll\[2\].slope\[$collVect(light_key)\].fixed TRUE {}
	ASSIGN $collVect(name).col.coll_stack\[2\].coll\[1\].slope\[$collVect(heavy_key)\].fixed TRUE {}
	ASSIGN $collVect(name).col.s_stack\[1\].fixed FALSE {}
	ASSIGN $collVect(name).col.s_stack\[2\].fixed FALSE {}
	if {[info exist collVect(keys)]} {unset collVect(keys)}
	if {[info exist collVect(specs)]} {unset collVect(specs)}
	lappend collVect(keys) $collVect(name).col.xsi\[$collVect(light_key)\] $collVect(name).col.xsi\[$collVect(heavy_key)\] 
	lappend collVect(specs) $collVect(light_spec) $collVect(heavy_spec)
    } elseif {$choice == 3} {
	set collVect(delta) 0.3
	ASSIGN $collVect(name).col.xsi\[$collVect(light_key)\].fixed TRUE
	ASSIGN $collVect(name).col.xsi\[$collVect(heavy_key)\].fixed TRUE
	ASSIGN $collVect(name).col.condenser.reflux_ratio.fixed FALSE
	ASSIGN $collVect(name).col.condenser.totprod.Ftot.fixed FALSE
	if {[info exist collVect(keys)]} {unset collVect(keys)}
	if {[info exist collVect(specs)]} {unset collVect(specs)}
	lappend collVect(keys) $collVect(name).col.xsi\[$collVect(light_key)\] $collVect(name).col.xsi\[$collVect(heavy_key)\] 
	lappend collVect(specs) $collVect(light_spec) $collVect(heavy_spec)
    }
}
proc SingleOpt {} {

    global collVect
    Solve_do_Flush

    RUN $collVect(name).cc.reset
    ASSIGN $collVect(name).col.xsi\[$collVect(light_key)\].fixed TRUE
    ASSIGN $collVect(name).col.xsi\[$collVect(heavy_key)\].fixed TRUE
    ASSIGN $collVect(name).col.condenser.reflux_ratio.fixed FALSE
    ASSIGN $collVect(name).col.condenser.totprod.Ftot.fixed FALSE
    ASSIGN $collVect(name).col.s_stack\[1\].fixed FALSE
    ASSIGN $collVect(name).col.s_stack\[2\].fixed FALSE
    ASSIGN $collVect(name).col.stot.fixed TRUE
    ASSIGN $collVect(name).col.split\[1\].fixed TRUE

    opt $collVect(name).cc $collVect(name).cc.cost_calc.cost 0.00001 0.05 $collVect(name).col.stot $collVect(name).col.split\[1\]
}
# Internal procedures

# eof
#

