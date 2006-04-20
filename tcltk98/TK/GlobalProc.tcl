#  GlobalProc.tcl: global commands to do with window management
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.71 $
#  Last modified on: $Date: 1998/07/01 09:06:32 $
#  Last modified by: $Author: ballan $
#  Revision control file: $RCSfile: GlobalProc.tcl,v $
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
# proc ascsetdebug {{on 1}}
# -------------------------------------------------------------------------
# Note that in the following we see the feature that all tcl procs are
# global and that procedures may define procedures.
# Use examples:
# ascsetdebug 0
#   this turns debugging spew info off.
# ascsetdebug 1
#   this turns debugging spew info on.
# ascsetdebug
#   this also turns debugging spew on because the DEFAULT given is 1.
# -------------------------------------------------------------------------
proc ascsetdebug {{on 1}} {
  if {$on} {
    proc entertrace {args} {
        set depth [expr {[info level]-1}]
        puts "entering [lindex [info level $depth] 0] : $args"
    }
    proc leavetrace {args} {
        set depth [expr {[info level]-1}]
        puts "leaving [lindex [info level $depth] 0] : $args"
    }
  } else {
    proc entertrace {args} {}
    proc leavetrace {args} {}
  }
}

ascsetdebug 0

# this handles functions with arguments nicely for startup
proc asc_catch {func} {
  if {[catch {$func} errmessage]} {
    puts stderr "*** Error calling $func"
    puts stderr $errmessage
    puts stderr "***"
  }
}

# requires double braces to deal with xfs dot qualified procedure names.

#
# proc awin {window}
#-----------------------------------------------------------------------
# start (or destroy) windows. awin help for detail
#-----------------------------------------------------------------------
proc awin {window} {

  global xfShowWindow.browser xfShowWindow.display xfShowWindow.library
  global xfShowWindow.probe   xfShowWindow.script
  global xfShowWindow.solver  xfShowWindow.toolbox xfShowWindow.units
  global xfShowWindow.debug   xfShowWindow.mtx
  global ascSolvVect
  global env

  switch $window {
 # getting the toolbox requires all its attachments.
    {t*} -
    {all} -
    {t*} -
    {T*} {
              set xfShowWindow.browser 1
              ShowWindow.browser
              puts -nonewline "."; flush stdout
              set xfShowWindow.display 1
              ShowWindow.display
              puts -nonewline "."; flush stdout
              set xfShowWindow.library 1
              ShowWindow.library
              puts -nonewline "."; flush stdout
              set xfShowWindow.probe 1
              ShowWindow.probe
              puts -nonewline "."; flush stdout
              set xfShowWindow.script 1
              ShowWindow.script
              puts -nonewline "."; flush stdout
              set xfShowWindow.solver 1
              ShowWindow.solver
              puts -nonewline "."; flush stdout
              set xfShowWindow.units 1
              ShowWindow.units
              puts -nonewline "."; flush stdout
              set xfShowWindow.toolbox 1
              ShowWindow.toolbox
              puts "."; flush stdout
              update idletask
 # resource everything
              puts "source $env(ASCENDTK)/ToolboxProc.tcl"
              asc_source "$env(ASCENDTK)/ToolboxProc.tcl"
              puts "source $env(ASCENDTK)/UtilProc.tcl"
              asc_source "$env(ASCENDTK)/UtilProc.tcl"
              puts "source $env(ASCENDTK)/DisplayProc.tcl"
              asc_source "$env(ASCENDTK)/DisplayProc.tcl"
              puts "source $env(ASCENDTK)/UnitsProc.tcl"
              asc_source "$env(ASCENDTK)/UnitsProc.tcl"
              puts "source $env(ASCENDTK)/ProbeProc.tcl"
              asc_source "$env(ASCENDTK)/ProbeProc.tcl"
              puts "source $env(ASCENDTK)/LibraryProc.tcl"
              asc_source "$env(ASCENDTK)/LibraryProc.tcl"
              puts "source $env(ASCENDTK)/TypetreeProc.tcl"
              asc_source "$env(ASCENDTK)/TypetreeProc.tcl"
              puts "source $env(ASCENDTK)/BrowserProc.tcl"
              asc_source "$env(ASCENDTK)/BrowserProc.tcl"
              puts "source $env(ASCENDTK)/DebugProc.tcl"
              asc_source "$env(ASCENDTK)/DebugProc.tcl"
              puts "source $env(ASCENDTK)/MtxProc.tcl"
              asc_source "$env(ASCENDTK)/MtxProc.tcl"
              puts "source $env(ASCENDTK)/SolverProc.tcl"
              asc_source "$env(ASCENDTK)/SolverProc.tcl"
              puts "source $env(ASCENDTK)/ScriptProc.tcl"
              asc_source "$env(ASCENDTK)/ScriptProc.tcl"
              puts "source $env(ASCENDTK)/WWWHelpProc.tcl"
              asc_source "$env(ASCENDTK)/WWWHelpProc.tcl"
              puts "source $env(ASCENDTK)/HubProc.tcl"
              asc_source "$env(ASCENDTK)/HubProc.tcl"
              puts "source $env(ASCENDTK)/mps.tcl"
              asc_source "$env(ASCENDTK)/mps.tcl"
              puts "source $env(ASCENDTK)/CallbackProc.tcl"
              asc_source "$env(ASCENDTK)/CallbackProc.tcl"
              puts "source $env(ASCENDTK)/NoteboxProc.tcl"
              asc_source "$env(ASCENDTK)/NoteboxProc.tcl"
              puts "source $env(ASCENDTK)/methods.tcl"
              asc_source "$env(ASCENDTK)/methods.tcl"
#             puts "Wiring up the buttons..."
              asc_catch set_Browser_Defaults
              asc_catch set_Display_Defaults
              asc_catch set_Library_Defaults
              asc_catch set_Typetree_Defaults
              asc_catch set_Probe_Defaults
              asc_catch set_Script_Defaults
              asc_catch set_Solver_Defaults
              # Overwriting with parameters defined by user rc file
              # but now we don't really need an rc file do we?
              asc_catch View_Source_Params_Files
              asc_catch set_Debug_Defaults
              asc_catch set_Mtx_Defaults
              asc_catch set_Units_Defaults
              asc_catch set_Toolbox_Defaults
              asc_catch set_Util_Defaults
              asc_catch set_HUB_Defaults
              asc_catch set_Help_Defaults

           }
     {b*} -
     {B*} {set xfShowWindow.browser 1
           ShowWindow.browser
           puts "source $env(ASCENDTK)/BrowserProc.tcl"
           asc_source "$env(ASCENDTK)/BrowserProc.tcl"
           set_Browser_Defaults
           update idletask}

     {d*} -
     {D*} {set xfShowWindow.display 1
           ShowWindow.display
           puts "source $env(ASCENDTK)/DisplayProc.tcl"
#           update idletask
           asc_source "$env(ASCENDTK)/DisplayProc.tcl"
           set_Display_Defaults
           update idletask}

     {l*} -
     {L*} {set xfShowWindow.library 1
           ShowWindow.library
           puts "source $env(ASCENDTK)/LibraryProc.tcl"
#           update idletask
           asc_source "$env(ASCENDTK)/LibraryProc.tcl"
           set_Library_Defaults
           update idletask}

     {p*} -
     {P*} {set xfShowWindow.probe 1
           ShowWindow.probe
           puts "source $env(ASCENDTK)/ProbeProc.tcl"
#           update idletask
           asc_source "$env(ASCENDTK)/ProbeProc.tcl"
           set_Probe_Defaults
           update idletask}

     {sc*} -
     {Sc*} -
     {SC*} {set xfShowWindow.script 1
            ShowWindow.script
            puts "source $env(ASCENDTK)/ScriptProc.tcl"
#            update idletask
            asc_source "$env(ASCENDTK)/ScriptProc.tcl"
            set_Script_Defaults
            update idletask}

 # theoretically  the general parameters are always there
     {so*} -
     {So*} -
     {SO*} { set xfShowWindow.solver 1
            ShowWindow.solver
            puts "source $env(ASCENDTK)/SolverProc.tcl"
#            update idletask
            asc_source "$env(ASCENDTK)/SolverProc.tcl"
            set_Solver_Defaults
            #
            # Overwriting with parameters defined by user
            #
            View_Source_Params_Files
            update idletask}

      {u*} -
      {U*} {set xfShowWindow.units 1
            ShowWindow.units
            puts "source $env(ASCENDTK)/UnitsProc.tcl"
#            update idletask
            asc_source "$env(ASCENDTK)/UnitsProc.tcl"
            set_Units_Defaults
            update idletask}

      {noall} -
      {noT*} -
      {not*} {
               if {${xfShowWindow.toolbox}} {DestroyWindow.toolbox
                                             DestroyWindow.util}
               if {${xfShowWindow.browser}} {DestroyWindow.browser}
               if {${xfShowWindow.display}} {DestroyWindow.display}
               if {${xfShowWindow.library}} {DestroyWindow.library}
               if {${xfShowWindow.probe}} {DestroyWindow.probe}
               if {${xfShowWindow.script}} {DestroyWindow.script}
               if {${xfShowWindow.solver}} {DestroyWindow.solver}
               if {${xfShowWindow.units}} {DestroyWindow.units}
               set xfShowWindow.browser 0
               set xfShowWindow.display 0
               set xfShowWindow.library 0
               set xfShowWindow.probe 0
               set xfShowWindow.script 0
               set xfShowWindow.solver 0
               set xfShowWindow.units 0
               set xfShowWindow.toolbox 0
               update idletask }

       {nob*} - 
       {noB*} {if {${xfShowWindow.browser}} {DestroyWindow.browser}
                    set xfShowWindow.browser 0}

       {nod*} -
       {noD*} {if {${xfShowWindow.browser}} {DestroyWindow.browser}
                    set xfShowWindow.display 0}

       {nol*} -
       {noL*} {if {${xfShowWindow.library}} {DestroyWindow.library}
                    set xfShowWindow.library 0}

       {nop*} -
       {noP*} {if {${xfShowWindow.probe}} {DestroyWindow.probe}
                    set xfShowWindow.probe 0}

       {nosc*} -
       {noSC*} -
       {noSc*} {if {${xfShowWindow.script}} {DestroyWindow.script}
                            set xfShowWindow.script 0}

       {noso*} -
       {noSo*} -
       {noSO*} {
               if {${xfShowWindow.solver}} {DestroyWindow.solver}
               set xfShowWindow.solver 0}

        {noU*} -
        {nou*} {if {${xfShowWindow.units}} {DestroyWindow.units}
                     set xfShowWindow.units 0}
        default { puts "awin syntax: awin \[no\]arg"
                  puts "     where arg is 'all' or the name of a window." }
    }
 #end switch
  set_ascplot_defaults
 #  update idletask
 #  update
}
 # end awin

#
# proc asc_fixiconnames {}
#-----------------------------------------------------------------------
# asc iconname silliness under twm, maybe mwm. once the hurly burly's done,
# call this. still buggy under sun3twm
#-----------------------------------------------------------------------
proc asc_fixiconnames {} {
  global ascBrowVect ascLibrVect ascSolvVect ascProbVect
  global ascToolVect ascSimsVect ascUnitVect ascDispVect
  global ascScripVect

  catch {wm iconname .browser}
  catch {wm title .}
  catch {wm iconname .}
  catch {wm iconname .display}
  catch {wm iconname .library}
  catch {wm iconname .probe}
  catch {wm iconname .script}
  catch {wm iconname .solver}
  catch {wm iconname .units }
  catch {wm iconname .toolbox}
  wm title . "Ascend IV"
  wm iconname . "Ascend IV"
  # update idletasks
  wm title . "Ascend IV"
  wm iconname .browser $ascBrowVect(iconname)
  wm iconname .display $ascDispVect(iconname)
  wm iconname .library $ascLibrVect(iconname)
  wm iconname .probe $ascProbVect(iconname)
  wm iconname .script $ascScripVect(iconname)
  wm iconname .solver $ascSolvVect(iconname)
  wm iconname .toolbox $ascToolVect(iconname)
  wm iconname .units $ascUnitVect(iconname)

}

#
# proc Redraw {}
#-----------------------------------------------------------------------
# Repaint info (as much as possible) from C structure after interface 
# resourced
#-----------------------------------------------------------------------
proc Redraw {} {
  Libr_Redraw
  Solve_Redraw
  Probe_Redraw
  Units_Redraw
  Sims_Redraw
}

#
# Binding hacks for Tk widgets in ASCEND.
# Ben Allan, 5/97.
#
proc set_cmuBindings {} {
  global tcl_platform tk_strictMotif
# we don't understand why scanning is supposed to work only with
# b1 down. haven't been able to find any unwanted side effects of this
# baa 6/12/97, tk80b1-no patches
  bind Menubutton <Motion> {
    global tk_strictMotif
    switch $tk_version {
    8.5 -
    8.4 {
        if {$tk_strictMotif} {
          tk::MbMotion %W up %X %Y
        } else {
          tk::MbMotion %W down %X %Y
        }
      }
    8.3 {
        if {$tk_strictMotif} {
          tkMbMotion %W up %X %Y
        } else {
          tkMbMotion %W down %X %Y
        }
      }
    }
  }
# puts "Adding cmu Entry and Text bindings"
  #
  # Entries:
  #
  # define Ctrl-u for unix folks.
  bind Entry <Control-Key-u> {
    if !$tk_strictMotif {
      %W delete 0 end
    }
  }
  #
  #define delete and backspace to be backspace.
  bind Entry <Key-Delete> {
    if !$tk_strictMotif {
      tkEntryBackspace %W
    }
  }
  #
  #Texts
  #
  # munge both backspace and delete to
  if {$tcl_platform(platform)=="unix" && !$tk_strictMotif} {
    # the standard tcl text bindings. Swap them.
    # bind Text <Delete> {
    #    if {[%W tag nextrange sel 1.0 end] != ""} {
    #        %W delete sel.first sel.last
    #    } else {
    #        %W delete insert
    #        %W see insert
    #    }
    #}
    #bind Text <BackSpace> {
    #    if {[%W tag nextrange sel 1.0 end] != ""} {
    #        %W delete sel.first sel.last
    #    } elseif [%W compare insert != 1.0] {
    #        %W delete insert-1c
    #        %W see insert
    #    }
    #}
    bind Text <BackSpace> {
        if {[%W tag nextrange sel 1.0 end] != ""} {
            %W delete sel.first sel.last
        } else {
            %W delete insert
            %W see insert
        }
    }
    bind Text <Delete> {
        if {[%W tag nextrange sel 1.0 end] != ""} {
            %W delete sel.first sel.last
        } elseif [%W compare insert != 1.0] {
            %W delete insert-1c
            %W see insert
        }
    }
  }
  if {!$tk_strictMotif} {
    if {$tcl_platform(platform) =="windows"} {
      bind Text <Control-Key-x> {
          asctk_textCut %W
      }
      bind Text <Control-Key-c> {
          asctk_textCopy %W
      }
      bind Text <Control-Key-v> {
          asctk_textPaste %W
      }
    }
    proc asctk_textCut {w} {
        set expo [$w cget -exportselection]
        $w configure -exportselection 1
        selection own $w
        tk_textCut $w
        $w configure -exportselection $expo
    }
    proc asctk_textCopy {w} {
        set expo [$w cget -exportselection]
        $w configure -exportselection 1
        selection own $w
        tk_textCopy $w
        $w configure -exportselection $expo
    }
    proc asctk_textPaste {w} {
        set expo [$w cget -exportselection]
        $w configure -exportselection 1
        tk_textPaste $w
        $w configure -exportselection $expo
    }
  }
}

#
# Function to manage which ascend window, if any, claims the
# PRIMARY X selection. With this function, PRIMARY should
# shadow CLIPBOARD as long as ASCEND owns primary.
# We must have no more than 1 widget at a time exporting or
# life is very difficult.
# calling this function with a widget name makes that widget
# the exporting window and unmakes the previous widget exporterness.
proc asc_export_selection {w} {
  global  ascGlobalVect
  if {![string length $w]} {
    return
  }
  if {![winfo exists $w]} {
    return
  }
  catch {$ascGlobalVect(selection_exporter) configure -exportselection 0} err
  set ascGlobalVect(selection_exporter) $w
  $w configure -exportselection 1
}

#
# proc set_Global_Defaults {}
#-----------------------------------------------------------------------
# proc set_Global_Defaults
# These following must be set here to control window creation in Main.
#-----------------------------------------------------------------------
proc set_Global_Defaults {} {
  set_cmuBindings

  global env tcl_platform asc_tkfbox
  global ascBrowVect ascLibrVect ascSolvVect ascProbVect ascDebuVect
  global ascToolVect ascSimsVect ascUnitVect ascDispVect ascGlobalVect
  global ascScripVect ascMtxVect

  #
  # Check if the directory for saving window options exists or can be
  # created
  #
  if {[string compare $tcl_platform(platform) "unix"]==0} {
    if { [ file exists ~/ascdata ]} {
      set ascGlobalVect(saveoptions) 1
    } else {
      if { [catch {file mkdir ~/ascdata} direrror] } {
        set ascGlobalVect(saveoptions) 0
        puts $direrror
      } else {
        set ascGlobalVect(saveoptions) 1
        puts "directory ~/ascdata has been created"
      }
    }
    set ascGlobalVect(userhome) ""
    catch {set ascGlobalVect(userhome) "[glob ~/ascdata]"}
  }
  if {[string compare $tcl_platform(platform) "windows"]==0} {
    set uhome {}
    catch {set uhome [glob ~]}
    if {[info exists env(HOME)] &&
        [file isdirectory $env(HOME)] &&
        [file writable $env(HOME)]} {
      set uhome $env(HOME)
    }
    set upfound 0
    if {[string length $uhome] < 4} {
      if {[info exists env(USERPROFILE)] &&
          [file isdirectory $env(USERPROFILE)] &&
          [file writable $env(USERPROFILE)]} {
        set uhome $env(USERPROFILE)
        set upfound 1
      }
    }
    if {!$upfound && [string length $uhome] < 4} {
      if {[info exists env(USERHOME)] &&
          [file isdirectory $env(USERHOME)] &&
          [file writable $env(USERHOME)]} {
        set uhome $env(USERHOME)
        set upfound 1
      }
    }
    append uhome /ascdata
    puts stderr "data home = $uhome"

    if { [ file exists $uhome ] && \
	 [file writable $uhome] && \
	 [file isdirectory $uhome]} {
      set ascGlobalVect(saveoptions) 1
    } else {
      if { [catch {file mkdir $uhome} direrror] } {
        set ascGlobalVect(saveoptions) 0
        puts $direrror
      } else {
        set ascGlobalVect(saveoptions) 1
        puts "directory $uhome has been created"
      }
    }
    set ascGlobalVect(userhome) $uhome
  }

  if {[info exists env(ASCENDLIBRARY)] == 0} {
    puts stderr "setting lib in global to [pwd]"
    set env(ASCENDLIBRARY) [pwd]
  }
  if {$ascGlobalVect(saveoptions)} {
    switch $tcl_platform(platform) {
    unix {
        set env(ASCENDLIBRARY) $ascGlobalVect(userhome):$env(ASCENDLIBRARY)
      }
    windows {
        set crap [file nativename $ascGlobalVect(userhome)]
	append crap ";"
	append crap $env(ASCENDLIBRARY)
        set env(ASCENDLIBRARY) $crap
      }
    macintosh {
      puts stderr "ASCEND IV's Tcl/Tk interface has not yet been ported"
      puts stderr "to Macintosh. Lots and lots of things are broken."
      error "ASCEND load aborted on macintosh."
      }
    }
  }
  puts stderr "Library directories:"
  puts stderr $env(ASCENDLIBRARY)
  set ascGlobalVect(librarypathdirs) ""
  if {$tcl_platform(platform) == "unix"} {
    set ascGlobalVect(librarypathdirs) [split $env(ASCENDLIBRARY) :]
  } else { #windoze. mac? mac uses : instead of /, ugh.
    set ascGlobalVect(librarypathdirs) [split $env(ASCENDLIBRARY) \;]
  }
  if {$tcl_platform(platform) == "unix"} {
    set ascGlobalVect(ascend2gms) \
      [file nativename [file dirname [info nameofexecutable]]]/ascend2gms.pl
    set ascGlobalVect(gms2ascend) \
      [file nativename [file dirname [info nameofexecutable]]]/convertoutput.pl
  }

  # take care of the PRIMARY selection under unix. the last window to
  # have <<Copy>> call should be the only window to export selection.
  set ascGlobalVect(selection_exporter) .

  set slist ""
  set s 0
  catch {set slist [slv_available]}
  set ascSolvVect(numberofsolvers) [llength $slist]
  foreach i $slist {
    set ascSolvVect(name.$s) $i
    if {[string range $i 0 2]=="no_"} {
      set ascSolvVect(available.$s) 0
      set ascSolvVect(name.$s) [string range $i 3 end]
    } else { set ascSolvVect(available.$s) 1}
    incr s
  }

  set winshortlist \
     "Solv Brow Libr Prob Debu Tool Scrip Mtx Unit Disp Global"
  foreach w $winshortlist {
    set asc${w}Vect(initialstate) "withdrawn"
    set asc${w}Vect(geometry) "200x200+10+10"
    set asc${w}Vect(iconname) $w
    set asc${w}Vect(font) {helvetica 12 bold}
    set asc${w}Vect(visibility) 1
  }
  #this next line sets the toolbitmap under the assumption that GlobalProc
  # will never be present under xf. toolAttributes is C-registered
  set ascGlobalVect(toolbitmap) toolAttributes

  set ascToolVect(initialstate) "withdrawn"
  set ascGlobalVect(maxutilrow) 8
  set ascGlobalVect(hideinfoforever) 0
  set ascGlobalVect(warrinfo) 0
  set ascGlobalVect(liceinfo) 0
  set ascGlobalVect(authinfo) 0

  set ascDebuVect(windowname) .debug
  set ascDispVect(windowname) .display
  set ascMtxVect(windowname) .mtx
  set ascScripVect(windowname) .script
  set ascToolVect(windowname) .toolbox
  set ascUnitVect(windowname) .units
  set ascSolvVect(windowname) .solver
  set ascBrowVect(windowname) .browser
  set ascLibrVect(windowname) .library
  set ascProbVect(windowname) .probe
 # do the option database load which will override above settings
 # the next line sets a dummy value for ScrollCommand at a low
 # priority. if you see NoFunction returned from an option query
 # you should apply some default instead. The tk Class default for
 # ScrollCommand is {} so NoFunction is ok.
  option add *ScrollComand NoFunction 20
#
# New tcl file ascend.ad
# View contains the procedures required by ascend.ad
#
  puts "source $env(ASCENDTK)/View.tcl"
  asc_source "$env(ASCENDTK)/View.tcl"
  puts "source $env(ASCENDTK)/ascend.ad"
  asc_source $env(ASCENDTK)/ascend.ad

  update
  if {[file exists $ascGlobalVect(userhome)/ascend.ad]} {
    if {[catch {asc_source $ascGlobalVect(userhome)/ascend.ad} aderror]} {
        puts $aderror
    }
  }
  update

  Set_View_Option_Values
  View_Source_Option_Files

#
# # make ascend global ->Xglobal
# # do not add these at priorities >=20 as this will give conflicts
# # when reading the options with Foreground in get_opt
#   option add *Font $ascGlobalVect(font) 60
   option add *Text*Foreground $ascGlobalVect(tfg) 40
   option add *Listbox*Foreground $ascGlobalVect(tfg) 40
   option add *Text*Background $ascGlobalVect(tbg) 40
   option add *Listbox*Background $ascGlobalVect(tbg) 40
#   option add *Background $ascGlobalVect(bg) 60
#   option add *Foreground $ascGlobalVect(fg) 60
#  # initialize read/write virtual.
  __userdata_init
  set ascGlobalVect(w_load) "1"
}

# set color/etc vectors for transients with arrays
proc set_Template_defaults {} {
  global ascGlobalVect
  global AscMonoEntry1 AscMonoEntry2 AscMonoEntry4 AscConfirm VPaned
  global AscPopSlide ascParPageVect   ascMsgVect
  global ascListSelectBox ascListSelectB1Box
  set AscMonoEntry1(font) $ascGlobalVect(font)
  set AscMonoEntry2(font) $ascGlobalVect(font)
  set AscMonoEntry4(font) $ascGlobalVect(font)
  set AscConfirm(font) $ascGlobalVect(font)
  set VPaned(knobcolor) white
  set AscPopSlide(font) $ascGlobalVect(font)
  set ascParPageVect(btn_font) $ascGlobalVect(font)
  set ascListSelectB1Box(font) $ascGlobalVect(font)
  set ascListSelectB1Box(grab) 1
  set ascListSelectBox(font) $ascGlobalVect(font)
  set ascListSelectBox(grab) 1
  set ascMsgVect(lbl_font) $ascGlobalVect(font)
  set ascMsgVect(btn_font) $ascGlobalVect(font)
}

#
# proc Glob_do_GNU {}
#-----------------------------------------------------------------------
# the gnubutton callback
#-----------------------------------------------------------------------
 # d_dumpfile defined in generalk.tcl, so this is silent until main loads
 # generalk
proc Glob_do_GNU {} {
 # old version
 #  global env
 #  set path "[lindex [split $env(ASCENDHELP) :] 0]/notices/startup"
 #  catch {d_dumpfile stdout $path}
 #  wm iconify .
  ascShowInfo
}

proc ascShowInfo {{force 0}} {
  global ascGlobalVect env

  wm withdraw .
  if {[winfo exists .itop]==1} {
    destroy .itop
  }
  if {$ascGlobalVect(hideinfoforever)==1 && !$force} {
    return
  }
  set f .itop.info
  wm title . "ASCEND IV"
  toplevel .itop
  wm positionfrom .itop user
  wm title .itop "ASCEND IV"
  wm iconname .itop "ASCEND IV"
  frame $f -highlightthickness 2 -borderwidth 2 -relief sunken
  set t $f.text
  text $t -yscrollcommand "$f.scroll set" -setgrid true \
          -font {Helvetica 8} -width 80 -exportselection 0 \
	  -height 35 -wrap word -highlightthickness 0 -borderwidth 0
  pack append $f $t {right frame center expand fill}
  scrollbar $f.scroll -command "$t yview"
  pack append $f $f.scroll {left frame center expand filly}
  pack append .itop $f {bottom frame center expand fill}
  $t tag configure center -justify center -spacing1 5m -spacing3 5m
  $t tag configure buttons -lmargin1 1c -lmargin2 1c -rmargin 1c \
	  -spacing1 3m -spacing2 0 -spacing3 0

  button $t.click -text "Authors" \
    -command "ascAuthInfo"  \
    -font {Helvetica 8} \
    -cursor top_left_arrow
  button $t.delete -text "Help (starts browser)" \
    -command  {Script_getting_started}  \
    -font {Helvetica 8} \
    -cursor top_left_arrow
  button $t.goaway -text "Dismiss" \
    -command {destroy .itop}  \
    -font {Helvetica 8} \
    -cursor top_left_arrow
  button $t.die -text "Dismiss forever" \
      -command "Global_Dismiss_Forever"  \
    -font {Helvetica 8} \
    -cursor top_left_arrow
  button $t.gnu_btn \
    -bitmap "@$env(ASCENDBITMAPS)/gnu-ascend.xbm" \
    -state normal \
    -text {}


  $t insert end "                       ASCEND IV\n"
  $t insert end "Advanced System for Computations in ENgineering Design\n"
  $t insert end "(C) Copyright 1992-1998 Carnegie Mellon University\n"
  $t insert end "\tRelease 0.9.0, June 19, 1998\n"
  $t insert end "\n"
  $t insert end "Click on the buttons below if you want to see who we are. "
  $t insert end "By using ASCEND IV you are agreeing to the terms of "
  $t insert end "the GNU Public License and Warranty in the first Script:."
  $t insert end "License-Warranty.tcl\n"
  $t insert end "\n"
  $t insert end "ASCEND was developed by the students of Art Westerberg "
  $t insert end "from 1984 to 1998. A number of other students have also "
  $t insert end "contributed models to the ASCEND libraries. A list "
  $t insert end "of the known contributors (in any capacity) is here:  "
  $t window create end -window $t.click
  $t insert end "(Please let us know if we missed you.)"
  $t insert end "\n"
  $t insert end "\n"
  $t insert end "Help for getting started with ASCEND should be available "
  $t insert end "at "
  $t insert end "\n"
  $t insert end "       http://www.cs.cmu.edu/~ascend/pdfhelp.htm\n"
  $t insert end "(you should be able to paste the address above with a mouse)"
  $t insert end " where you will find links to our on-line help and link to "
  $t insert end "our bug report form.\n\n"
  $t insert end "  E-mail: ascend+help@cs.cmu.edu\n\n"

  $t insert end "        "
  $t window create end -window $t.goaway
  $t insert end "        "
  $t window create end -window $t.die
  $t insert end "\n\n           "
  $t window create end -window $t.gnu_btn
  $t insert end "\n\n"
  $t insert end "USER DATA DIRECTORY $ascGlobalVect(userhome)"
  $t insert end "\n\n"
  wm deiconify .itop
}

#
# proc Global_Dismiss_Forever {}
#_________________________________________________________________
# Save the value of hideinfoforever = 1 in the file global.a4o
#_________________________________________________________________
proc Global_Dismiss_Forever {} {
entertrace
  global ascGlobalVect

  if {$ascGlobalVect(saveoptions) == 0} {
    return
  }

  Global hideinfoforever 1 
  View_Save_Window_Options global 
  ascShowInfo
  return

leavetrace
}

#
# proc ascAuthInfo {}
#_________________________________________________________________
# pops up a tkdialog with the content of TK/WhoDunnit.tcl
# all merged into a single string.
#_________________________________________________________________
proc ascAuthInfo {} {
  global env ascScripVect
  set fileInFile [open $env(ASCENDTK)/WhoDunnit.tcl r]
  set textValue [read $fileInFile]
  # regsub -all \n $textValue { } textValue2
  asctk_dialog .authinfo $ascScripVect(font) "ASCEND Authors" \
    $textValue toolAttributes 0 OK
}
