#  WWWHelpProc.tcl: a rather generic help system
#  by Benjamin A. Allan
#  Created: August 28, 1996
#  Part of ASCEND
#  Revision: $Revision: 1.23 $
#  Last modified on: $Date: 1998/06/18 15:55:10 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: WWWHelpProc.tcl,v $
#
#  This file is part of the ASCEND Tcl/Tk Interface.
#
#  Copyright (C) 1996-1998 Carnegie Mellon University
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

#---------------------------------------------------------------------------
# a rather generic help system.  in this Case being used for ascend.
# The help info is basically a directory structure of html
# The magic part is done here which defines bindings for nearly
# all ascend gui  buttons.

#
# proc set_Help_Defaults {}
#---------------------------------------------------------------------------
# Initializes help utility information.
# these may be customized per site or per user in the
# AscendRC or .ascendrc file, since those files will
# be read (one or the other) after this function is called.
# The three important global variables are:
#    ascHelpVect(rootURL)
#	This should be a directory name ending with a /
#	This is not a URL!
#	The directory is the directory containing the top
#	level ascend help files for your site.
#    ascHelpVect(start_command)
#	This is the command we should execute to start a web
#	browser with a specific URL. It is used to spawn a
#	web browser independent of ASCEND. Every place that %U
#	appears in this command, we will substitute the
#	URL to be viewed. This is only called after the
#	restart_command has failed.
#    ascHelpVect(restart_command)
#	This is the command we should execute to redirect
#	the attention of your already running web browser.
#	If this command returns an error code, we will
#	attempt to start a new browser.
#	If your favorite browser does not support restarting,
#	set ascHelpVect(restart_command) "false" This will cause
#	a new browser to start for every help query from the
#	ASCEND interface.
#	Every place that %U appears in this command, we will
#	substitute the URL to be viewed.
#---------------------------------------------------------------------------
proc set_Help_Defaults {} {
  global ascHelpVect env tcl_platform
  if {$tcl_platform(platform) == "windows"} {
    set ascHelpVect(rootURL) $env(ASCENDDIST)
    regsub -all -- {\\}  $env(ASCENDDIST) / ascHelpVect(rootURL)
    append ascHelpVect(rootURL) /help/
    set ascHelpVect(start_command) \
      {{C:/Program Files/Plus!/Microsoft Internet/Iexplore.exe} %U}
    set ascHelpVect(restart_command) \
      {{C:/Program Files/Netscape/Navigator/Program/netscape.exe} %U}
    if {[file exists \
          {C:/Program Files/Netscape/Communicator/Program/netscape.exe}]} {
      set ascHelpVect(restart_command) \
        {{C:/Program Files/Netscape/Communicator/Program/netscape.exe} %U}
    }
  } else {
    set ascHelpVect(rootURL) $env(ASCENDDIST)
    append ascHelpVect(rootURL) /help/
    set ascHelpVect(start_command) "netscape %U"
    set ascHelpVect(restart_command) "netscape -remote openURL(%U)"
  }
  # above probably go in utilities window.
  set ascHelpVect(extension) ".htm"
  global ascHelpVect
#>>  asc_bv_init
}

#
# proc Help_do_Help {}
#---------------------------------------------------------------------------
# the help help button
#---------------------------------------------------------------------------
proc Help_do_Help {} {
  Help_button help.help.intro
}

#
# proc Help_Open {}
#---------------------------------------------------------------------------
# fire up the help window, on nothing in particular
#---------------------------------------------------------------------------
proc Help_Open {} {
  Help_button "beginner"
}
#
# proc Help_do_AskExpert {}
#---------------------------------------------------------------------------
# spawn ask expert mail.modal
#---------------------------------------------------------------------------
proc Help_do_AskExpert {} {
  global ascHelpVect
  catch {eval "exec" $ascHelpVect(askmail)}
}

#
# proc Help_do_Window {$win}
#---------------------------------------------------------------------------
#---------------------------------------------------------------------------
proc Help_do_Window {$win} {
  Help_button [winfo toplevel $win]
}

#>>#### initialization stuff
#>>#
#>># proc Help_BindAllM3 {}
#>>#--------------------------------------------------------------------------
#>># bind every goddamned button with an M3 call, keeping overhead as small as
#>># maintainably possible
#>>#--------------------------------------------------------------------------
#>>proc Help_BindAllM3 {} {
#>>  Help_BindButtons
#>>  Help_BindMenuButtons
#>>  Help_BindRadioButtons
#>>  Help_BindMenus
#>>  Help_BindCascades
#>>  puts "Click M3 on any button for button help."
#>>}
#>>
#>>#### FUNCTIONS THAT SET UP THE BINDINGS EN MASSE
#>>#
#>># proc Help_BindButtons {}
#>>#-------------------------------------------------------------------------
#>>#set M3 bindings on buttons for nontransients. transients ibnd themselves
#>># ascend specific
#>>#-------------------------------------------------------------------------
#>>proc Help_BindButtons {} {
#>>  foreach bb {
#>>   {{.solver.lbl_frm.btn_int} {solver.interrupt}}
#>>   {{.toolbox.bottom_frm.browser_btn} {toolbox.window}}
#>>   {{.toolbox.bottom_frm.display_btn} {toolbox.window}}
#>>   {{.toolbox.bottom_frm.library_btn} {toolbox.window}}
#>>   {{.toolbox.bottom_frm.probe_btn} {toolbox.window}}
#>>   {{.toolbox.bottom_frm.script_btn} {toolbox.window}}
#>>   {{.toolbox.bottom_frm.sims_btn} {toolbox.window}}
#>>   {{.toolbox.bottom_frm.solver_btn} {toolbox.window}}
#>>   {{.toolbox.bottom_frm.units_btn} {toolbox.window}}
#>>   {{.toolbox.top_frm.util_btn} {utilities}}
#>>   {{.toolbox.top_frm.exit_btn} {toolbox.exit}}
#>>   {{.toolbox.top_frm.hlp_btn} {help.help.intro}}
#>>   {{.toolbox.top_frm.filler5} {toolbox.bugreport}}
#>>              } {
#>>    Help_BindButton [lindex $bb 0] [lindex $bb 1]
#>>  }
#>>}
#>>
#>>
#>>#
#>># proc Help_BindRadioButtons {}
#>>#------------------------------------------------------------------------
#>>#set M3 bindings on radiobuttons for nontransients. 
#>>#transients ibnd themselves
#>># ascend specific
#>>#------------------------------------------------------------------------
#>>proc Help_BindRadioButtons {} {
#>>  foreach rb {
#>>              } {
#>>    Help_BindButton [lindex $rb 0] [lindex $rb 1]
#>>  }
#>>}
#>>
#>>#
#>># proc Help_BindMenuButtons {}
#>>#-------------------------------------------------------------------------
#>>#set M3 bindings on menubuttons  transients bind their own
#>># ascend specific
#>>#-------------------------------------------------------------------------
#>>proc Help_BindMenuButtons {} {
#>>  foreach mb {
#>>   {{.browser.menubar.display} {browser.display}}
#>>   {{.browser.menubar.edit} {browser.edit}}
#>>   {{.browser.menubar.export} {browser.export}}
#>>   {{.browser.menubar.find} {browser.find}}
#>>   {{.browser.menubar.help} {help.help.intro}}
#>>   {{.display.menubar.file} {display.execute}}
#>>   {{.display.menubar.view} {display.view}}
#>>   {{.display.menubar.help} {help.help.intro}}
#>>   {{.library.menubar.create} {library.create}}
#>>   {{.library.menubar.display} {library.display}}
#>>   {{.library.menubar.edit} {library.edit}}
#>>   {{.library.menubar.find} {library.find}}
#>>   {{.library.menubar.help} {help.help.intro}}
#>>   {{.probe.menubar.edit} {probe.edit}}
#>>   {{.probe.menubar.file} {probe.execute}}
#>>   {{.probe.menubar.export} {probe.export}}
#>>   {{.probe.menubar.help} {help.help.intro}}
#>>   {{.script.menubar.edit} {script.edit}}
#>>   {{.script.menubar.execute} {script.execute}}
#>>   {{.script.menubar.help} {help.help.intro}}
#>>   {{.sims.menubar.edit} {simulations.edit}}
#>>   {{.sims.menubar.export} {simulations.export}}
#>>   {{.sims.menubar.help} {help.help.intro}}
#>>   {{.solver.menubar.analyze} {solver.analyze}}
#>>   {{.solver.menubar.display} {solver.display}}
#>>   {{.solver.menubar.edit} {solver.edit}}
#>>   {{.solver.menubar.edit.options} {solver.grill}}
#>>   {{.solver.menubar.execute} {solver.execute}}
#>>   {{.solver.menubar.export} {solver.export}}
#>>   {{.solver.menubar.help} {help.help.intro}}
#>>   {{.units.menubar.edit} {units.edit}}
#>>   {{.units.menubar.help} {help.help.intro}}
#>>              } {
#>>    Help_BindButton [lindex $mb 0] [lindex $mb 1]
#>>  }
#>>  global ascSolvVect
#>>  if {$ascSolvVect(modelbar)} {
#>>    Help_BindButton .solver.main_frm.btn_expo solver.import
#>>    Help_BindMenu .solver.main_frm.btn_expo.m solver.import
#>>  }
#>>}
#>>
#>>#
#>># proc Help_BindMenus {}
#>>#-------------------------------------------------------------------------
#>>#set M3 bindings on menus. transients bind their own.
#>># ascend specific
#>>#-------------------------------------------------------------------------
#>>proc Help_BindMenus {} {
#>>  foreach m {
#>>   {{.browser.menubar.display} {browser.display}}
#>>   {{.browser.menubar.edit} {browser.edit}}
#>>   {{.browser.menubar.export} {browser.export}}
#>>   {{.browser.menubar.find} {browser.find}}
#>>   {{.browser.menubar.help} {browser.help}}
#>>   {{.display.menubar.file} {display.execute}}
#>>   {{.display.menubar.view} {display.view}}
#>>   {{.display.menubar.help} {display.help}}
#>>   {{.library.menubar.create} {library.create}}
#>>   {{.library.menubar.display} {library.display}}
#>>   {{.library.menubar.edit} {library.edit}}
#>>   {{.library.menubar.find} {library.find}}
#>>   {{.library.menubar.help} {library.help}}
#>>   {{.probe.menubar.edit} {probe.edit}}
#>>   {{.probe.menubar.file} {probe.execute}}
#>>   {{.probe.menubar.export} {probe.export}}
#>>   {{.probe.menubar.help} {probe.help}}
#>>   {{.script.menubar.edit} {script.edit}}
#>>   {{.script.menubar.execute} {script.execute}}
#>>   {{.script.menubar.help} {script.help}}
#>>   {{.sims.menubar.edit} {simulations.edit}}
#>>   {{.sims.menubar.export} {simulations.export}}
#>>   {{.sims.menubar.help} {simulations.help}}
#>>   {{.solver.menubar.analyze} {solver.analyze}}
#>>   {{.solver.menubar.display} {solver.display}}
#>>   {{.solver.menubar.edit} {solver.edit}}
#>>   {{.solver.menubar.edit.options} {solver.grill}}
#>>   {{.solver.menubar.execute} {solver.execute}}
#>>   {{.solver.menubar.export} {solver.export}}
#>>   {{.solver.menubar.help} {solver.help}}
#>>   {{.units.menubar.edit} {units.edit}}
#>>   {{.units.menubar.help} {units.help}}
#>>            } {
#>>    Help_BindMenu [lindex $m 0] [lindex $m 1]
#>>  }
#>>}
#>>
#>>#
#>># proc Help_BindCascades {}
#>>#-------------------------------------------------------------------------
#>>#set M3 bindings on cascade menus. transients bind their own.
#>># ascend specific
#>>#-------------------------------------------------------------------------
#>>proc Help_BindCascades {} {
#>>  foreach m {
#>>   {{.browser.menubar.edit.compile} {browser.edit} {compile}}
#>>   {{.solver.menubar.analyze.depend} {solver.analyze} {finddependenteqns}}
#>>   {{.solver.menubar.edit.solvers} {solver.edit} {selectsolver}}
#>>            } {
#>>    Help_BindCascadeMenu [lindex $m 0] [lindex $m 1] [lindex $m 2]
#>>  }
#>>}
#>>
#>>#### FUNCTIONS THAT CREATE BINDINGS
#>>#
#>># proc Help_BindButton {b p args}
#>>#-------------------------------------------------------------------------
#>>#set M3 binding for button b to Help_button p args
#>>#-------------------------------------------------------------------------
#>>proc Help_BindButton {b p args} {
#>>  if {[llength $args]} {
#>>    bind $b <B3-ButtonRelease> "Help_button $p $args"
#>>  } else {
#>>    bind $b <B3-ButtonRelease> "Help_button $p"
#>>  }
#>>}
#>>
#>>
#>>#
#>># proc Help_BindCascadeMenu {m p}
#>>#-------------------------------------------------------------------------
#>>#set M3 binding for menu w with Help_menu prefix p
#>>#-------------------------------------------------------------------------
#>>proc Help_BindCascadeMenu {m p q} {
#>>  bind $m <B3-ButtonRelease> "Help_CascadeItemBinding $m $p $q"
#>>}
#>>#
#>># proc Help_BindMenu {m p}
#>>#-------------------------------------------------------------------------
#>>#set M3 binding for menu w with Help_menu prefix p
#>>#-------------------------------------------------------------------------
#>>proc Help_BindMenu {m p} {
#>>  bind $m <B3-ButtonRelease> "Help_MenuItemBinding $m $p"
#>>}
#>>
#>>### BINDINGS THAT GET CALLED
#>>
#>>#
#>># proc Help_MenuItemBinding {m p}
#>>#-------------------------------------------------------------------------
#>># m is the menu widget name, p is the qlfdid prefix for m
#>># mashes button entry name into something safe and calls help on
#>># prefix.mash
#>>#-------------------------------------------------------------------------
#>>proc Help_MenuItemBinding {m p} {
#>>  set l [$m index active]
#>>  if {$l == "none"} {return}
#>>  set l [lindex [$m entryconfigure $l -label] 4]
#>>  regsub  -all { } $l "" l
#>>  set l [string tolower $l]
#>>  Help_button $p $l
#>>}
#>>#
#>># proc Help_CascadeItemBinding {m p q}
#>>#-------------------------------------------------------------------------
#>># m is the menu widget name, p is the qlfdid prefix for m
#>># q is the cascaded menu name mashed
#>># mashes p q names into something safe and calls help.
#>>#-------------------------------------------------------------------------
#>>proc Help_CascadeItemBinding {m p q} {
#>>  set l [$m index active]
#>>  if {$l == "none"} {return}
#>>  set l [lindex [$m entryconfigure $l -label] 4]
#>>  regsub  -all { } $l "" l
#>>  set l [string tolower $l]
#>>  Help_button $p $q.$l
#>>}

#
# Help_button {qlfdid item}
#---------------------------------------------------------------------------
# callback M3-release
# if argument is {none} returns immediately.
# Argument is the qlfdid relative to root of the button.
# args, if present, should be a menu item and will be
# treated as a <name> tag in html.
#
# if args empty:
# qlfdid is a filename or valid url portion, relative to root
# function to bind to fullblown buttons of various sorts
# e.g. menu button, radio button, just plain buttons
#
# if args !empty:
# menu items and cascades need an intermediate binding to
# concoct this item argument which follows the # in html
#---------------------------------------------------------------------------
proc Help_button {hqlfdid args} {
  global ascHelpVect ascScripVect
  if {$hqlfdid=="none"} {return}
  set data "See $hqlfdid.pdf section $args\nat\n"
  append data http://www.cs.cmu.edu/~ascend/pdfhelp.htm
  asctk_dialog .helpmsg $ascScripVect(font) "Help location" $data info 0 OK
  return
  # old code
  set qlist [split $hqlfdid .]
  set url [join $qlist /]
  set url "$ascHelpVect(rootURL)$url$ascHelpVect(extension)"
  if {[llength $args]} {
    set url "$url#$args"
  }
  Help_do_URL $url
}

#### utility function. simple eh?

# this function opens or reopens a URL with the user defined web protocol
proc Help_do_URL {url} {
  global ascHelpVect
  set comm "$ascHelpVect(rootURL)"
  set err 0
  set errmess WebOK
  regsub -all %U $ascHelpVect(restart_command) $url comm
  puts "Executing: $comm &"
  set err [catch {eval "exec $comm"} errmess]
  if {$err} {
    puts stderr "Error: $errmess"
    puts stderr "Trying to start a WWW browser"
    puts stderr "If it doesn't work send mail to ascend+help@cs.cmu.edu"
    puts stderr "after verifying your WWW info in the utilities window."
    regsub -all %U $ascHelpVect(start_command) $url comm
    eval "exec $comm &"
  }
}


#>># defines bindings for balloon help
#>>proc asc_bv_init {} {
#>>  global bv
#>>  set bv(popped) 0
#>>  set bv(ow) .
#>>  set bv(label) root
#>>  set bv(popup) .
#>>  set bv(lastleave) .
#>>  bind Menu <<MenuSelect>> {asc_bv_monitor %W}
#>>  bind all <Leave>  {asc_bv_leavekill %W}
#>>  bind all <Enter>  {asc_bv_enterkill %W}
#>>}
#>>
#>># with a few twisty exceptions, leaving any widget kills the popup
#>>proc asc_bv_leavekill {w} {
#>>  set to [winfo containing [winfo pointerx $w] [winfo pointery $w]]
#>>  if {$to == $w} {return ; #damn overeager wm }
#>>  catch {$w entrycget active -menu}
#>>  if {[winfo class $w] == "Menu" && 
#>>      [$w cget -type] == "menubar" &&
#>>      $to == [$w entrycget active -menu]} {
#>>    return
#>>  }
#>>  set root ""
#>>  catch {set root [string range $to 0 4]}
#>>  if {$root == ".ball"} {return}
#>> # puts "allleave destroy $w $to $root"
#>>  destroy .balloon
#>>}
#>>
#>># entering any widget except the popup kills the popup
#>>proc asc_bv_enterkill {w} {
#>>  set to [winfo containing [winfo pointerx $w] [winfo pointery $w]]
#>>  set root ""
#>>  catch {set root [string range $to 0 4]}
#>>  if {$root != ".ball" && 
#>>      [winfo class $w] != "Menu" && 
#>>      [winfo class $w] != "Menubutton"} {
#>> #  puts "allenter destroy"
#>>    destroy .balloon
#>>  }
#>>}
#>>
#>>
#>>proc asc_bv_monitor {w} {
#>>  if {$w == ".balloon"} {return}
#>>  if {[llength [split $w .]]==3} {return}
#>>  if {![$w yposition active]} { return } ;# ignore separators
#>>  set rootx 0
#>>  set rooty 0
#>>  if {[$w type active] == "cascade" } {
#>>    set rootx [expr [winfo rootx $w] + [winfo width $w] -15]
#>>    set rooty [expr [winfo rooty $w] + [$w yposition active] + 15]
#>>  } else {
#>>    set rootx [expr [winfo rootx $w] + [winfo width $w] -4]
#>>    set rooty [expr [winfo rooty $w] + [$w yposition active] + 5]
#>>  }
#>>  asc_bv_post $rootx $rooty "[$w entrycget active -label]" \
#>>         [string tolower [$w cget -tearoffcommand]]
#>>}
#>>
#>>proc asc_bv_post {x y l ow} {
#>>  global bv
#>>  
#>>  if {$bv(ow) == $ow && $bv(label) == $l || $bv(popped)} {return}
#>>  set bv(label) $l
#>>  set bv(ow) $ow
#>> # puts "wipost destroy"
#>>  destroy .balloon
#>>  if {$l == ""} {return} ;# no help on menubars
#>>  toplevel .balloon -class Help
#>>  wm withdraw .balloon
#>>  wm overrideredirect .balloon 1
#>>  .balloon configure -cursor question_arrow
#>>  set se "we need to lookup the short help"
#>>  button .balloon.s  \
#>>    -text $se \
#>>    -foreground black \
#>>    -background yellow \
#>>    -justify left \
#>>    -highlightthickness 0 \
#>>    -padx 0 \
#>>    -pady 0 \
#>>    -command "puts \"help $ow $l\""
#>>  button .balloon.m \
#>>    -text "More on $l" \
#>>    -foreground black \
#>>    -background yellow \
#>>    -justify left \
#>>    -highlightthickness 0 \
#>>    -padx 0 \
#>>    -pady 0 \
#>>    -command balloon_more
#>>  button .balloon.p \
#>>    -text "Stop balloon help" \
#>>    -foreground black \
#>>    -background yellow \
#>>    -justify left \
#>>    -highlightthickness 0 \
#>>    -padx 0 \
#>>    -pady 0 \
#>>    -command balloon_stop
#>>  button .balloon.q \
#>>    -text "Dismiss" \
#>>    -foreground black \
#>>    -background yellow \
#>>    -justify left \
#>>    -highlightthickness 0 \
#>>    -padx 0 \
#>>    -pady 0 \
#>>    -command balloon_pop
#>>
#>>  pack append .balloon \
#>>    .balloon.s {top frame center expand fillx} \
#>>    .balloon.m {top frame center expand fillx} \
#>>    .balloon.p {top frame center expand fillx} \
#>>    .balloon.q {top frame center expand fillx}
#>>
#>>  wm geometry .balloon ""
#>>
#>>  asc_bv_popup .balloon $x $y
#>>
#>>}
#>>
#>>proc asc_bv_popup {menu x y {entry {}}} {
#>>    global bv
#>>    global tcl_platform
#>> #  if {($bv(popup) != "") || ($bv(postedMb) != "")} {
#>> #      catch {.balloon unpost}
#>> #  }
#>>    asc_bv_PostOverPoint $menu $x $y $entry
#>>}
#>>
#>>
#>>proc asc_bv_PostOverPoint {menu x y {entry {}}}  {
#>>    global tcl_platform
#>> # this function needs to be smarter about off screen popups
#>>    wm geometry .balloon "+$x+$y"
#>>    wm deiconify .balloon
#>>}

