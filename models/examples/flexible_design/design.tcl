#                        design.tcl
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

#  $Date: 1996/05/20 22:05:45 $
#  $Revision: 1.1.1.1 $
#  $Author: mthomas $
#  $Source: /afs/cs.cmu.edu/project/ascend/Repository/models/examples/flexible_design/design.tcl,v $

# Module: design.tcl
# Tcl version: 7.3 (Tcl/Tk/XF)
# Tk version: 3.6
# XF version: 2.3
#

# module contents
global moduleList
global autoLoadList
set moduleList(design.tcl) { . .designtop}
set autoLoadList(design.tcl) {0}

# procedures to show toplevel windows


# procedure to show window .designtop
proc ShowWindow.designtop {args} {
# xf ignore me 7

StartupSrc.designtop

  # build widget .designtop
  if {"[info procs XFEdit]" != ""} {
    catch "XFDestroy .designtop"
  } {
    catch "destroy .designtop"
  }
  toplevel .designtop  \
    -background {#fef9e3e1c413}

  # Window manager configurations
  global tkVersion
  wm positionfrom .designtop ""
  wm sizefrom .designtop program
  wm maxsize .designtop 1000 1000
  wm minsize .designtop 320 524
  wm title .designtop {Flexible Column Creator}

  # bindings
  bind .designtop <Enter> {set message $messageVect(main)}

  # build widget .designtop.components
  frame .designtop.components \
    -background {#fef9e3e1c413} \
    -borderwidth {2}

  # build widget .designtop.components.buttons
  frame .designtop.components.buttons \
    -background {#fef9e3e1c413} \
    -borderwidth {2}

  # build widget .designtop.components.buttons.copy
  button .designtop.components.buttons.copy \
    -activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -command {CopyElements .designtop.components.components.frame1.frame3.listbox8} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000} \
    -state {disabled} \
    -text {---->}
  # bindings
  bind .designtop.components.buttons.copy <Enter> {set message {Hit this button to copy selected 
component(s) from list at left 
into your set at right}
if {[selection own] == $complistwidget} {
 %W configure -state normal
} else {
 %W configure -state disabled
}
tk_butEnter %W}

  # build widget .designtop.components.buttons.remove
  button .designtop.components.buttons.remove \
    -activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -command {DeleteElements $yourlistwidget} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000} \
    -state {disabled} \
    -text {Remove}
  # bindings
  bind .designtop.components.buttons.remove <Enter> {set message {Hit this button to remove
selected component(s) from your
list at right}
if {[selection own] == $yourlistwidget} {
  %W configure -state normal
} else {
  %W configure -state disabled
}
tk_butEnter %W}

  # build widget .designtop.components.buttons.checkcomps
  button .designtop.components.buttons.checkcomps \
    -activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -command {if {[info exist collVect(components)]} {unset collVect(components)};
for {set i 0} {$i < [$yourlistwidget size]} {incr i} {
  lappend collVect(components) [$yourlistwidget get $i]
}

CheckComponents 
ClearList $yourlistwidget
CopyElements $yourlistwidget $collVect(components)
puts new
puts $collVect(components)

} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000} \
    -state {disabled} \
    -text "Check"
  # bindings
  bind .designtop.components.buttons.checkcomps <Enter> {set message {Hit this button to check your set of 
components.  This will order them in terms of relative
volatility and tell you if azeotropes or heterogeneous 
behavior are likely.}
if {[$yourlistwidget size] != 0} {
 %W configure -state normal
} else {
 %W configure -state disabled
}
tk_butEnter %W}

  # pack widget .designtop.components.buttons
  pack append .designtop.components.buttons \
    .designtop.components.buttons.copy {top frame n pady 15 fillx} \
    .designtop.components.buttons.remove {top frame n pady 15} \
    .designtop.components.buttons.checkcomps {top frame center}

  # build widget .designtop.components.components
  frame .designtop.components.components \
    -background {#fef9e3e1c413} \
    -borderwidth {2}

  # build widget .designtop.components.components.componentslabel
  label .designtop.components.components.componentslabel \
    -background {#fef9e3e1c413} \
    -foreground {#000000000000} \
    -relief {raised} \
    -text {Your Components}
  # bindings
  bind .designtop.components.components.componentslabel <Enter> {set message $messageVect(yourcomps)}

  # build widget .designtop.components.components.frame1
  frame .designtop.components.components.frame1 \
    -background {#fef9e3e1c413}

  # build widget .designtop.components.components.frame1.frame3
  frame .designtop.components.components.frame1.frame3 \
    -background {#fef9e3e1c413} \
    -borderwidth {2}

  # build widget .designtop.components.components.frame1.frame3.listbox8
  listbox .designtop.components.components.frame1.frame3.listbox8 \
    -background {#fef9e3e1c413} \
    -foreground {#000000000000} \
    -geometry {17x6} \
    -relief {sunken} \
    -selectbackground {#fef9b508b508} \
    -selectforeground {#000000000000} \
    -yscrollcommand {.designtop.components.components.frame1.frame4.scrollbar9 set}
  # bindings
  bind .designtop.components.components.frame1.frame3.listbox8 <Enter> {set message $messageVect(yourcomps)}

  # pack widget .designtop.components.components.frame1.frame3
  pack append .designtop.components.components.frame1.frame3 \
    .designtop.components.components.frame1.frame3.listbox8 {left frame center expand fill}

  # build widget .designtop.components.components.frame1.frame4
  frame .designtop.components.components.frame1.frame4 \
    -background {#fef9e3e1c413} \
    -borderwidth {2}

  # build widget .designtop.components.components.frame1.frame4.scrollbar9
  scrollbar .designtop.components.components.frame1.frame4.scrollbar9 \
    -activeforeground {#fef9fef9fef9} \
    -background {#fef9e3e1c413} \
    -command {.designtop.components.components.frame1.frame3.listbox8 yview} \
    -foreground {#fef9b508b508} \
    -relief {sunken} \
    -width {13}

  # pack widget .designtop.components.components.frame1.frame4
  pack append .designtop.components.components.frame1.frame4 \
    .designtop.components.components.frame1.frame4.scrollbar9 {top frame center expand filly}

  # pack widget .designtop.components.components.frame1
  pack append .designtop.components.components.frame1 \
    .designtop.components.components.frame1.frame3 {left frame center expand fill} \
    .designtop.components.components.frame1.frame4 {right frame center filly}

  # pack widget .designtop.components.components
  pack append .designtop.components.components \
    .designtop.components.components.componentslabel {top frame center expand fill} \
    .designtop.components.components.frame1 {top frame center expand fillx}

  # build widget .designtop.components.componentslist
  frame .designtop.components.componentslist \
    -background {#fef9e3e1c413} \
    -borderwidth {2}

  # build widget .designtop.components.componentslist.frame1
  frame .designtop.components.componentslist.frame1 \
    -background {#fef9e3e1c413}

  # build widget .designtop.components.componentslist.frame1.frame3
  frame .designtop.components.componentslist.frame1.frame3 \
    -background {#fef9e3e1c413} \
    -borderwidth {2}

  # build widget .designtop.components.componentslist.frame1.frame3.listbox8
  listbox .designtop.components.componentslist.frame1.frame3.listbox8 \
    -background {#fef9e3e1c413} \
    -foreground {#000000000000} \
    -geometry {17x6} \
    -relief {sunken} \
    -selectbackground {#fef9b508b508} \
    -selectforeground {#000000000000} \
    -yscrollcommand {.designtop.components.componentslist.frame1.frame4.scrollbar9 set}
  # bindings
  bind .designtop.components.componentslist.frame1.frame3.listbox8 <Enter> {set message $messageVect(complist)}

  # pack widget .designtop.components.componentslist.frame1.frame3
  pack append .designtop.components.componentslist.frame1.frame3 \
    .designtop.components.componentslist.frame1.frame3.listbox8 {left frame center expand fill}

  # build widget .designtop.components.componentslist.frame1.frame4
  frame .designtop.components.componentslist.frame1.frame4 \
    -background {#fef9e3e1c413} \
    -borderwidth {2}

  # build widget .designtop.components.componentslist.frame1.frame4.scrollbar9
  scrollbar .designtop.components.componentslist.frame1.frame4.scrollbar9 \
    -activeforeground {#fef9fef9fef9} \
    -background {#fef9e3e1c413} \
    -command {.designtop.components.componentslist.frame1.frame3.listbox8 yview} \
    -foreground {#fef9b508b508} \
    -relief {sunken} \
    -width {13}

  # pack widget .designtop.components.componentslist.frame1.frame4
  pack append .designtop.components.componentslist.frame1.frame4 \
    .designtop.components.componentslist.frame1.frame4.scrollbar9 {top frame center expand filly}

  # pack widget .designtop.components.componentslist.frame1
  pack append .designtop.components.componentslist.frame1 \
    .designtop.components.componentslist.frame1.frame3 {left frame center expand fill} \
    .designtop.components.componentslist.frame1.frame4 {right frame center filly}

  # build widget .designtop.components.componentslist.listlabel
  label .designtop.components.componentslist.listlabel \
    -background {#fef9e3e1c413} \
    -foreground {#000000000000} \
    -relief {raised} \
    -text {Components Library}
  # bindings
  bind .designtop.components.componentslist.listlabel <Enter> {set message $messageVect(complist)}

  # pack widget .designtop.components.componentslist
  pack append .designtop.components.componentslist \
    .designtop.components.componentslist.frame1 {bottom frame center expand fill} \
    .designtop.components.componentslist.listlabel {bottom frame center expand fill}

  # pack widget .designtop.components
  pack append .designtop.components \
    .designtop.components.componentslist {left frame center expand fillx} \
    .designtop.components.buttons {left frame center} \
    .designtop.components.components {left frame center expand fillx}

  # build widget .designtop.creating
  frame .designtop.creating \
    -background {#fef9e3e1c413} \
    -borderwidth {2} \
    -relief {ridge}

  # build widget .designtop.creating.readfile
  button .designtop.creating.readfile \
    -activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -command {READ FILE $collVect(filename);
COMPILE $collVect(name) OF $collVect(name);
RUN $collVect(name).values;
RUN $collVect(name).reset;
BROWSE $collVect(name);} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000} \
    -state {disabled} \
    -text {Read and Compile}
  # bindings
  bind .designtop.creating.readfile <Enter> {set message "This button will read in $collVect(filename),
compile your model, $collVect(name), initialize it, and
send it to the browser"
if {[$yourlistwidget size] != 0} {
 %W configure -state normal
} else {
 %W configure -state disabled
}
tk_butEnter %W}

  # build widget .designtop.creating.solve
  button .designtop.creating.solve \
    -activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -command {SolvCol1 $collVect(name).col
WRITE_VIRTUAL $collVect(name).col base.virtual} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000} \
    -state {disabled} \
    -text {Solve Column}
  # bindings
  bind .designtop.creating.solve <Enter> {set message "This button will solve your column to
full thermodynamics with heat balance. Your model
must be Read and Compiled first"
if {[$yourlistwidget size] != 0} {
 %W configure -state normal
} else {
 %W configure -state disabled
}
tk_butEnter %W}

  # build widget .designtop.creating.plotliq
  button .designtop.creating.plotliq \
    -activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -command {RUN $collVect(name).plots.plot_values
PLOT $collVect(name).plots.plots\[plotx_s\] $collVect(plot_dir)/$collVect(name).plot
SHOW LAST
} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000} \
    -state {disabled} \
    -text {Plot Column}
  # bindings
  bind .designtop.creating.plotliq <Enter> {set message "This button will create a plot of
the liquid compositions for the current state
of your model"
if {[$yourlistwidget size] != 0} {
 %W configure -state normal
} else {
 %W configure -state disabled
}
tk_butEnter %W}


  # pack widget .designtop.creating
  pack append .designtop.creating \
    .designtop.creating.readfile {left frame center expand} \
    .designtop.creating.solve {left frame center expand} \
    .designtop.creating.plotliq {left frame center expand}

  # build widget .designtop.filename
  frame .designtop.filename \
    -background {#fef9e3e1c413} \
    -borderwidth {2}

  # build widget .designtop.filename.entry
  entry .designtop.filename.entry \
    -background {#fef9e3e1c413} \
    -foreground {#000000000000} \
    -insertbackground {#fef9b508b508} \
    -relief {sunken} \
    -selectbackground {#fef9b508b508} \
    -selectforeground {#000000000000} \
    -textvariable {collVect(filename)}
  # bindings
  bind .designtop.filename.entry <Enter> {set message $messageVect(file)}

  # build widget .designtop.filename.label
  label .designtop.filename.label \
    -background {#fef9e3e1c413} \
    -foreground {#000000000000} \
    -text {File Name:}
  # bindings
  bind .designtop.filename.label <Enter> {set message $messageVect(file)}

  # pack widget .designtop.filename
  pack append .designtop.filename \
    .designtop.filename.label {left frame e padx 19 pady 6} \
    .designtop.filename.entry {left frame e padx 16}

  # build widget .designtop.frame0
  frame .designtop.frame0 \
    -background {#fef9e3e1c413} \
    -borderwidth {2} \
    -relief {raised}

  # build widget .designtop.frame0.menubutton1
  menubutton .designtop.frame0.menubutton1 \
    -activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000} \
    -menu {.designtop.frame0.menubutton1.m} \
    -text {File  } \
    -underline {0}

  # build widget .designtop.frame0.menubutton1.m
  menu .designtop.frame0.menubutton1.m \
    -activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000}
  .designtop.frame0.menubutton1.m add command \
    -command {set collFSBox(pattern) "*.col"
set savename [collFSBox "Select Save File" {$savename} {} {}]
if {$savename != ""} {
 source $savename
 ClearList $yourlistwidget
 CopyElements $yourlistwidget $collVect(components)
}} \
    -label {Open Settings}
  .designtop.frame0.menubutton1.m add command \
    -command {set collFSBox(pattern) "*.col"
set savename [collFSBox "Select Save File" {$savename} {} {}]
if {$savename != ""} {
set fid [open $savename w]

 puts $fid "catch {unset  comp}"
 puts $fid "catch {unset  comp_name}"
 puts $fid "catch {unset  feed}"
 puts $fid "catch {unset  flow}"
 puts $fid "catch {unset  units}"

foreach i [array names collVect] {
    puts $fid "catch {unset collVect($i)}"
}

 puts $fid "catch {unset  stopflag}"

foreach i [array names comp] {
 puts $fid "set comp($i) \"$comp($i)\""
}
foreach i [array names comp_name] {
 puts $fid "set comp_name($i) \"$comp_name($i)\""
}
foreach i [array names feed] {
 puts $fid "set feed($i) \"$feed($i)\""
}
foreach i [array names flow] {
 puts $fid "set flow($i) \"$flow($i)\""
}
foreach i [array names units] {
 puts $fid "set units($i) \"$units($i)\""
}

foreach i [array names collVect] {
    puts $fid "set collVect($i) \{$collVect($i)\}"
}

 puts $fid "set stopflag \"$stopflag\""

 close $fid
}} \
    -label {Save Settings}
  .designtop.frame0.menubutton1.m add command \
    -command {killmessagecenter} \
    -label {Disable Message Box}
  .designtop.frame0.menubutton1.m add command \
    -command {pack append .designtop .designtop.messageframe {top frame center expand fill}} \
    -label {Enable Message Box}
  .designtop.frame0.menubutton1.m add command \
    -command {WRITE VALUES $collVect(name) $collVect(values_dir)/$collVect(name).values} \
    -label {Write Values}
  .designtop.frame0.menubutton1.m add command \
    -command {READ VALUES $collVect(values_dir)/$collVect(name).values} \
    -label {Read Values}
  .designtop.frame0.menubutton1.m add command \
    -command {SaveValues} \
    -label {Save All Values}
  .designtop.frame0.menubutton1.m add command \
    -command {UpdateValues} \
    -label {Update Values}
  .designtop.frame0.menubutton1.m add command \
    -command {destroy .designtop} \
    -label {Quit}

  # build widget .designtop.frame0.menubutton2
  menubutton .designtop.frame0.menubutton2 \
    -activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000} \
    -menu {.designtop.frame0.menubutton2.m} \
    -text {Help} \
    -underline {0}

  # build widget .designtop.frame0.menubutton2.m
  menu .designtop.frame0.menubutton2.m \
    -activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000}

  # build widget .designtop.frame0.options
  menubutton .designtop.frame0.options \
    -activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000} \
    -menu {.designtop.frame0.options.m} \
    -text {Options} \
    -underline {0}

  # build widget .designtop.frame0.options.m
  menu .designtop.frame0.options.m \
    -activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000}
  .designtop.frame0.options.m add command \
    -command {ClearList $complistwidget
CopyElements $complistwidget $alphalist} \
    -label {List components alphabetically}
  .designtop.frame0.options.m add command \
    -command {ClearList $complistwidget
CopyElements $complistwidget $boillist} \
    -label {List components by boiling point}
  .designtop.frame0.options.m add command \
    -command {set message {You can set some of the
global variables from here. Don't
play with these unless you know 
what you are doing.}
Setglobals} \
    -label {Set globals}
  .designtop.frame0.options.m add command \
    -command {set message {You can choose your own
color settings from here}
Setcolors} \
    -label {Color Settings}
  .designtop.frame0.options.m add command \
    -command {ProcedureMenu
FlexibleMenu
} \
    -label {Procedure Menues}

  # pack widget .designtop.frame0
  pack append .designtop.frame0 \
    .designtop.frame0.menubutton1 {left frame center} \
    .designtop.frame0.menubutton2 {right frame center} \
    .designtop.frame0.options {left frame center}

  # build widget .designtop.instructions
  message .designtop.instructions \
    -aspect {1500} \
    -background {#fef9e3e1c413} \
    -foreground {#000000000000} \
    -padx {5} \
    -pady {2} \
    -text {Select components from list at left
}

  # build widget .designtop.messageframe
  frame .designtop.messageframe \
    -background {#fef9e3e1c413} \
    -borderwidth {16} \
    -height {88} \
    -relief {groove} \
    -width {250}

  # build widget .designtop.messageframe.messages
  message .designtop.messageframe.messages \
    -aspect {1500} \
    -background {#fef9e3e1c413} \
    -borderwidth {5} \
    -foreground {#000000000000} \
    -justify {center} \
    -padx {5} \
    -pady {2} \
    -text {
Message Center
} \
    -textvariable {message}
  # bindings
  bind .designtop.messageframe.messages <Enter> {set message $messageVect(main)}

  # pack widget .designtop.messageframe
  pack append .designtop.messageframe \
    .designtop.messageframe.messages {right frame center expand}

  # build widget .designtop.modelname
  frame .designtop.modelname \
    -background {#fef9e3e1c413} \
    -borderwidth {2}

  # build widget .designtop.modelname.entry
  entry .designtop.modelname.entry \
    -background {#fef9e3e1c413} \
    -foreground {#000000000000} \
    -insertbackground {#fef9b508b508} \
    -relief {sunken} \
    -selectbackground {#fef9b508b508} \
    -selectforeground {#000000000000} \
    -textvariable {collVect(name)}
  # bindings
  bind .designtop.modelname.entry <Enter> {set message $messageVect(model)}

  # build widget .designtop.modelname.label
  label .designtop.modelname.label \
    -background {#fef9e3e1c413} \
    -foreground {#000000000000} \
    -text {Model Name:}
  # bindings
  bind .designtop.modelname.label <Enter> {set message $messageVect(model)}

  # pack widget .designtop.modelname
  pack append .designtop.modelname \
    .designtop.modelname.label {left frame e padx 19 pady 6} \
    .designtop.modelname.entry {left frame center padx 16}

  # build widget .designtop.running
  frame .designtop.running \
    -background {#fef9e3e1c413} \
    -borderwidth {2} \
    -relief {ridge}

	

  # build widget .designtop.running.meetspec
  button .designtop.running.meetspec \
    -activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -command {if {[MultipleMeetSpec $collVect(name).col $collVect(keys) $collVect(specs) $collVect(delta)] == 0} {
    if {[tk_dialog .question "Do Singly?" {Attempt each component individually?} {} 0 yes no] == 0} {
	MultipleMeetSpec $collVect(name).col $collVect(name).col.xsi\[$collVect(light_key)\] $collVect(light_spec) 1.0
	MultipleMeetSpec $collVect(name).col $collVect(name).col.xsi\[$collVect(heavy_key)\] $collVect(heavy_spec) 1.0
    }
}
} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000} \
    -state {disabled} \
    -text {Meet Specs}
  # bindings
  bind .designtop.running.meetspec <Enter> {set message "This button will run the procedure to
meet your separation specifications"
if {[$yourlistwidget size] != 0} {
 %W configure -state normal
} else {
 %W configure -state disabled
}
tk_butEnter %W}

  # build widget .designtop.running.singleopt
button .designtop.running.singleopt \
	-activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -command {SingleOpt} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000} \
    -state {disabled} \
    -text {Optimize This}
  # bindings
  bind .designtop.running.singleopt <Enter> {set message "This button will cost optimize the current column
 for this particular feed. (Not the flexible problem)."
if {[$yourlistwidget size] != 0} {
 %W configure -state normal
} else {
 %W configure -state disabled
}
tk_butEnter %W}

  # build widget .designtop.running.setmeetspec
  button .designtop.running.setmeetspec \
    -activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -command {SetupMeetSpec} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000} \
    -state {disabled} \
    -text {Setup MeetSpec}
  # bindings
  bind .designtop.running.setmeetspec <Enter> {set message "This button will create a window for
setting your specifications decisions."
if {[$yourlistwidget size] != 0} {
 %W configure -state normal
} else {
 %W configure -state disabled
}
tk_butEnter %W}

  # build widget .designtop.running.setup
  button .designtop.running.setup \
    -activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -command {SetupAll} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000} \
    -state {disabled} \
    -text {Setup All}
  # bindings
  bind .designtop.running.setup <Enter> {set message "Set up all the feeds"
if {[$yourlistwidget size] != 0} {
 %W configure -state normal
} else {
 %W configure -state disabled
}
tk_butEnter %W}

  # pack widget .designtop.running
  pack append .designtop.running \
    .designtop.running.setmeetspec {left frame center expand} \
    .designtop.running.meetspec {left frame center expand} \
    .designtop.running.singleopt {left frame center expand} \
    .designtop.running.setup {left frame center pady 10 expand}

  # build widget .designtop.settings
  frame .designtop.settings \
    -background {#fef9e3e1c413} \
    -borderwidth {2} \
    -relief {ridge}


  # build widget .designtop.settings.setflows
  button .designtop.settings.setflows \
    -activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -command {if {[info exist compcheck]} {unset compcheck};
for {set i 0} {$i < [$yourlistwidget size]} {incr i} {
  lappend compcheck [$yourlistwidget get $i]
}
if {$compcheck == $collVect(components)} {
  Setnfeeds
  set collVect(flowcheck) 1
} else {
 set collVect(flowcheck) 0
tk_dialog .flowmessage Recheck {Your current list of components
 hasn't been checked. Please hit Check Components} {} 0 ok
}} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000} \
    -state {disabled} \
    -text {Set Feed}
  # bindings
  bind .designtop.settings.setflows <Enter> {set message {This button will bring up a dialog box for
you to input the molar flows of each feed component.
You must run Check Components before setting feed.}
if {[$yourlistwidget size] != 0} {
 %W configure -state normal
} else {
 %W configure -state disabled
}
tk_butEnter %W}

  # build widget .designtop.settings.setpurity
  button .designtop.settings.setpurity \
    -activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -command {if {[info exist compcheck]} {unset compcheck};
for {set i 0} {$i < [$yourlistwidget size]} {incr i} {
  lappend compcheck [$yourlistwidget get $i]
}
if {$compcheck == $collVect(components)} {
  SetRecovery
  set collVect(recoverycheck) 1
} else {
  set collVect(recoverycheck) 0
tk_dialog .flowmessage Recheck {Your current list of components
 hasn't been checked. Please hit Check Components} {} 0 ok
}
} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000} \
    -state {disabled} \
    -text {Set Recovery}
  # bindings
  bind .designtop.settings.setpurity <Enter> {set message {This button will bring up a dialog box for
you to input the fractional recovery of the key
components. You must run Check Components 
before setting feed.}
if {[$yourlistwidget size] != 0} {
 %W configure -state normal
} else {
 %W configure -state disabled
}
tk_butEnter %W}

  # build widget .designtop.settings.writefile
  button .designtop.settings.writefile \
    -activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -command {if {[info exist compcheck]} {unset compcheck};
if {[expr $collVect(recoverycheck) + $collVect(flowcheck)] == 2} {
 for {set i 0} {$i < [$yourlistwidget size]} {incr i} {
   lappend compcheck [$yourlistwidget get $i]
 }
 if {$compcheck == $collVect(components)} {
 
   WriteGeneral
 } else {
   tk_dialog .writemessage Recheck {Your current list   
     of components has not
     been checked. Please hit
     Check Components, Set Feed Flows,
     and Set Recovery} {} 0 ok
 }
} else {
   tk_dialog .writemessage {Set flows and recovery}  {You haven't set the flows and recovery yet.}  {} 0 ok
}} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000} \
    -state {disabled} \
    -text {Write Model}
  # bindings
  bind .designtop.settings.writefile <Enter> {set message "This button will create a model named 
$collVect(name) in the file $collVect(filename) for your current set
of components. You must run Check Components,
Set Feed Flows, and Set Recovery first."
if {[$yourlistwidget size] != 0} {
 %W configure -state normal
} else {
 %W configure -state disabled
}
tk_butEnter %W}

  # pack widget .designtop.settings
  pack append .designtop.settings \
    .designtop.settings.setflows {left frame center expand} \
    .designtop.settings.setpurity {left frame center expand} \
    .designtop.settings.writefile {left frame center expand}

  # build widget .designtop.flexible
  frame .designtop.flexible \
    -background {#fef9e3e1c413} \
    -borderwidth {2} \
    -relief {ridge}

  # build widget .designtop.flexible.opt
  button .designtop.flexible.opt \
    -activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -command {DoOptimize} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000} \
    -state {disabled} \
    -text {Optimize Once}
  # bindings
  bind .designtop.flexible.opt <Enter> {set message "Solve for local minimum.
Initial Approximation should
be done already."
if {[$yourlistwidget size] != 0} {
 %W configure -state normal
} else {
 %W configure -state disabled
}
tk_butEnter %W}

  # build widget .designtop.flexible.optimize
  button .designtop.flexible.optimize \
    -activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -command {CompleteOptimize} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000} \
    -state {disabled} \
    -text {Do Optimization}
  # bindings
  bind .designtop.flexible.optimize <Enter> {set message "Startup optimization and run optimization 
routine. Notethat this does initiation and
multiple optimizations"
if {[$yourlistwidget size] != 0} {
 %W configure -state normal
} else {
 %W configure -state disabled
}
tk_butEnter %W}

  # build widget .designtop.flexible.doapprox
  button .designtop.flexible.doapprox \
    -activebackground {#fef9b508b508} \
    -activeforeground {#000000000000} \
    -background {#fef9e3e1c413} \
    -command {Doapprox} \
    -disabledforeground {#fef9b508b508} \
    -foreground {#000000000000} \
    -state {disabled} \
    -text {Initiate Approximation}
  # bindings
  bind .designtop.flexible.doapprox <Enter> {set message "Do starting point approximation
and local optimization"
if {[$yourlistwidget size] != 0} {
 %W configure -state normal
} else {
 %W configure -state disabled
}
tk_butEnter %W}

  # pack widget .designtop.flexible
  pack append .designtop.flexible \
    .designtop.flexible.optimize {left frame center expand} \
    .designtop.flexible.doapprox {left frame center expand} \
    .designtop.flexible.opt {left frame center expand}

  # pack widget .designtop
  pack append .designtop \
    .designtop.frame0 {top frame center fillx} \
    .designtop.filename {top frame center} \
    .designtop.modelname {top frame center} \
    .designtop.components {top frame center fillx} \
    .designtop.instructions {top frame center} \
    .designtop.settings {top frame center pady 10 fillx} \
    .designtop.creating {top frame center pady 10 fillx} \
    .designtop.running {top frame center pady 10 fillx} \
    .designtop.flexible {top frame center pady 10 fillx} \
    .designtop.messageframe {top frame center expand fill}

  global tkVersion
  if {$tkVersion >= 3.0} {
    tk_menuBar .designtop.frame0 .designtop.frame0.menubutton1 .designtop.frame0.menubutton2 .designtop.frame0.options
  } {
    tk_menus .designtop .designtop.frame0.menubutton1 .designtop.frame0.menubutton2 .designtop.frame0.options
  }

  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {acetone}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {benzene}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {chloro_benzene}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {chloroform}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {diethylbenzene}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {diphenyl}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {ethanol}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {ethylbenzene}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {ethylene_glycol}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {i_butane}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {i_butanol}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {i_pentane}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {i_propanol}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {methanol}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {n_butane}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {n_butanol}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {n_decane}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {n_heptane}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {n_hexane}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {n_octane}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {n_pentane}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {n_propanol}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {pentanol_1}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {propane}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {propylene}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {styrene}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {toluene}
  .designtop.components.componentslist.frame1.frame3.listbox8 insert end {water}



  if {"[info procs XFEdit]" != ""} {
    catch "XFMiscBindWidgetTree .designtop"
    after 2 "catch {XFEditSetShowWindows}"
  }
}

proc DestroyWindow.designtop {} {# xf ignore me 7
  if {"[info procs XFEdit]" != ""} {
    if {"[info commands .designtop]" != ""} {
      global xfShowWindow.designtop
      set xfShowWindow.designtop 0
      XFEditSetPath .
      after 2 "XFSaveAsProc .designtop; XFEditSetShowWindows"
    }
  } {
    catch "destroy .designtop"
    update
  }
}

proc StartupSrc.designtop {args} {

global complistwidget
global yourlistwidget
global collVect
global messageVect
global env

set complistwidget .designtop.components.componentslist.frame1.frame3.listbox8
set yourlistwidget .designtop.components.components.frame1.frame3.listbox8

  global {collVect(filename)}
  set {collVect(filename)} {myfile.asc}
  global {collVect(name)}
  set {collVect(name)} {mymodel}
  global {collVect(source_dir)}
  set {collVect(source_dir)} "$env(ASCENDDIST)/models/examples/flexible_design"
  global {stopflag}
  set {stopflag} {0.0}
  global {collVect(values_dir)}
  set {collVect(values_dir)} {/tmp}
  global {collVect(plot_dir)}
  set {collVect(plot_dir)} {/tmp}
  global {collVect(logfile)}
  set {collVect(logfile)} {/tmp/flexlog}
  global {messageVect(model)}
  set {messageVect(model)} {This is the name your model will be given.
You can change the name in the entry box}
  global {collVect(recoverycheck)}
  set {collVect(recoverycheck)} {0}
  global collVect(delta)
  set collVect(delta) 0.8
  global collVect(min)
  set collVect(min) 0.8
  global collVect(max)
  set collVect(max) 1.2



  global {messageVect(yourcomps)}
  set {messageVect(yourcomps)} {This is the list of components you 
will have in your model. You can add to this 
list from the library list to the left.}

  global {messageVect(complist)}
  set {messageVect(complist)} {This is the list of all components 
available. Select components and hit
arrow to copy to your own list}
  global {messageVect(file)}
  set {messageVect(file)} {Your model will be written to this file.
You can change the name of this file in the entry box.

}
  global {messageVect(main)}
  set {messageVect(main)} {
Message Center
}


  global {alphalist}
  set {alphalist} {acetone benzene chloro_benzene chloroform diethylbenzene diphenyl ethanol ethylbenzene ethylene_glycol i_butane i_butanol i_pentane i_propanol methanol n_butane n_butanol n_decane n_heptane n_hexane n_octane n_pentane n_propanol pentanol_1 propane propylene styrene toluene water}
  global {bobcolor}
  set {bobcolor(base)} {#fef9e3e1c413}
  set {bobcolor(dark)} {#000000000000}
  set {bobcolor(light)} {#fef9b508b508}
  set {bobcolor(reallylight)} {#fef9fef9fef9}
  global {boillist}
  set {boillist} {propylene propane i_butane n_butane i_pentane n_pentane acetone chloroform methanol n_hexane ethanol benzene i_propanol n_propanol n_heptane water i_butanol toluene n_butanol n_octane chloro_benzene ethylbenzene pentanol_1 styrene diethylbenzene n_decane ethylene_glycol diphenyl}
  global {buttons}
  set {buttons} {
.designtop.creating.readfile
.designtop.creating.solve
.designtop.settings.writefile
.designtop.running.meetspec
.designtop.running.setup
.designtop.running.singleopt
.designtop.creating.plotliq
.designtop.running.setmeetspec
.designtop.components.buttons.checkcomps
.designtop.settings.setflows
.designtop.settings.setpurity
.designtop.components.buttons.copy
.designtop.components.buttons.remove
.designtop.frame0.menubutton1
.designtop.frame0.menubutton1.m
.designtop.frame0.menubutton2
.designtop.frame0.menubutton2.m
.designtop.frame0.options
.designtop.frame0.options.m
.designtop.flexible.opt
.designtop.flexible.optimize
.designtop.flexible.doapprox
}
  global {collVect}

  set collVect(components) {}

  global {colorBox}
  set {colorBox(activeBackground)} {}
  set {colorBox(activeForeground)} {}
  set {colorBox(background)} {}
  set {colorBox(colorName)} {#fef9e3e1c413}
  set {colorBox(font)} {}
  set {colorBox(foreground)} {}
  set {colorBox(oldWidget)} {}
  set {colorBox(palette)} {#fef9e3e1c413 black gray50 blue red green yellow orange}
  set {colorBox(paletteNr)} {0}
  set {colorBox(scrollActiveForeground)} {}
  set {colorBox(scrollBackground)} {}
  set {colorBox(scrollForeground)} {}
  set {colorBox(scrollSide)} {left}
  set {colorBox(type)} {hsv}

  global {e}
  set {e} {;}


  global {entries}
  set {entries} {
.designtop.filename.entry
.designtop.modelname.entry
}

  global {frames}
  set {frames} {
.designtop
.designtop.components
.designtop.creating
.designtop.filename
.designtop.frame0
.designtop.messageframe
.designtop.modelname
.designtop.running
.designtop.settings
.designtop.flexible
.designtop.components.buttons
.designtop.components.components
.designtop.components.componentslist
.designtop.components.components.frame1
.designtop.components.componentslist.frame1
.designtop.components.components.frame1.frame3
.designtop.components.components.frame1.frame4
.designtop.components.componentslist.frame1.frame3
.designtop.components.componentslist.frame1.frame4
}
  global {collFSBox}
  set {collFSBox(activeBackground)} {}
  set {collFSBox(activeForeground)} {}
  set {collFSBox(all)} {0}
  set {collFSBox(background)} {}
  set {collFSBox(button)} {0}
  set {collFSBox(extensions)} {0}
  set {collFSBox(font)} {}
  set {collFSBox(foreground)} {}
  set {collFSBox(internalPath)} [exec pwd]
  set {collFSBox(name)} {new.col}
  set {collFSBox(path)} [exec pwd]
  set {collFSBox(pattern)} {*.col}
  set {collFSBox(scrollActiveForeground)} {}
  set {collFSBox(scrollBackground)} {}
  set {collFSBox(scrollForeground)} {}
  set {collFSBox(scrollSide)} {left}
  set {collFSBox(showPixmap)} {0}

  global {labels}
  set {labels} {
.designtop.filename.label
.designtop.messageframe.messages
.designtop.modelname.label
.designtop.instructions
.designtop.components.components.componentslabel
.designtop.components.componentslist.listlabel
}
  global {listboxes}
  set {listboxes} {
.designtop.components.components.frame1.frame3.listbox8
.designtop.components.componentslist.frame1.frame3.listbox8
}

  global {scrollbars}
  set {scrollbars} {
.designtop.components.components.frame1.frame4.scrollbar9
.designtop.components.componentslist.frame1.frame4.scrollbar9
}
  global {stopflag}
  set {stopflag} {0.0}



catch {source $collVect(source_dir)/coll_lib.s
} sourcing

set sourcing
puts $sourcing
}

proc killmessagecenter {} {
  if {[winfo exists .designtop.messageframe]} {
    pack forget .designtop.messageframe
    wm withdraw .designtop
    wm minsize .designtop 0 0
    wm deiconify .designtop
  }
}
# procedure to show window .
proc ShowWindow. {args} {# xf ignore me 7

  # Window manager configurations
  global tkVersion
  wm positionfrom . user
  wm sizefrom . ""
  wm maxsize . 1280 1024
  wm minsize . 0 0
  wm title . {xf}

  # bindings
  bind . <Button-3> {MenuPopupPost .designtop.frame0.options %X %Y}
  bind . <ButtonRelease-3> {MenuPopupRelease .designtop.frame0.options %W}

  if {"[info procs XFEdit]" != ""} {
    catch "XFMiscBindWidgetTree ."
    after 2 "catch {XFEditSetShowWindows}"
  }
}


# User defined procedures


# Internal procedures

# eof
#

