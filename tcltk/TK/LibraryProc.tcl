#  LibraryProc.tcl: Library Tcl Code
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.73 $
#  Last modified on: $Date: 2003/01/19 01:27:44 $
#  Last modified by: $Author: ballan $
#  Revision control file: $RCSfile: LibraryProc.tcl,v $
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
# proc set_Library_Defaults {}
#-------------------------------------------------------------------
#  the usual setup of window defaults
#-------------------------------------------------------------------
proc set_Library_Defaults {} {

  global xfShowWindow.library
  global ascLibrVect
  global ascLibrVect
  global ascSimsVect

  # The ascLibrVect(fullfilename) array member is being used
  # solely for the purpose of having something to stuff in
  # in the file selection box on repeated calls.

  set ascLibrVect(compileC) 0
  set ascLibrVect(btuifstop) 1
  set ascLibrVect(ignorestop) 0
  set ascLibrVect(parserWarnings) 1
  set ascLibrVect(compilerWarnings) 1
  set ascLibrVect(simplifyRelations) 1
  set ascLibrVect(useCopyAnon) 1
  set ascLibrVect(lastreadextension) ".a4c"
  set ascLibrVect(fullfilename) ""
  set ascLibrVect(selectedtype) ""
  set ascLibrVect(target_type) ""
  set ascLibrVect(target_units) ""
  set ascLibrVect(basefiletypes) {
    {{Models} {.a4c .asc} }
    {{Libraries} {.a4l .lib}  }
    {{All ASCEND} {.a4c .a4l .asc .lib .a4p .a4v .a4u}  }
    {{Most} {.*} }
    {{All} {*} }
  }
  set ascLibrVect(filetypes) $ascLibrVect(basefiletypes)
  set ascLibrVect(windowname) .library
  # The status line is being controlled by a -textvariable bound
  # to the entry box. To write to the box, then just set the below variable.
  set ascLibrVect(entryline) ""

  # As types or type.child are suppressed or unsuppressed,
  # they should be added to the Libraries tcl list using
  # the Libr_recordhide function
  # Libr_recordhide add typename
  # Libr_recordhide add typename.partname
  # and removed
  # Libr_recordhide delete typename
  # Libr_recordhide delete typename.partname
  # from the tcl list hiddentypes.
  Libr_recordhide add unSELECTed_part

  if {[catch {set ascLibrVect(initialized)}]} {
    set ascLibrVect(initialized) "FALSE"
  }

  set ascSimsVect(windowname) ".sims";
  set ascSimsVect(target_instance) "";
  set ascSimsVect(instancetype) "";
  set ascSimsVect(selectedsim) "";
  set ascSimsVect(deletelist) "";
  set ascSimsVect(filename) "";
  set ascSimsVect(filetypes) {
      {{Pending statements} {.pendings} }
      {{Simulation structure} {.a4i} }
      {{Most} {.*} }
      {{All} {*} }
    }

  set ascLibrVect(moduleBox)  .library.main_frm.file_box.listbox1
  set ascLibrVect(typeBox)    .library.main_frm.model_box.listbox1
  set ascLibrVect(entryBox)   .library.entry_frm.lib_entry
  set ascLibrVect(simsBox)    .library.sims_frm.sims_box.listbox1
  set ascLibrVect(autorun) default_self

  set ascLibrVect(btfirst) 1

  # init method generation stuff
  set ascLibrVect(standardmethods) {}
  set ascLibrVect(generate_ADDMETHOD) 0
  set_MethodsDefaults

  Configure_Library
}

proc Sims_SetupBinTokenCC {} {
  global ascLibrVect ascUtilVect tcl_platform
  if {$ascLibrVect(btfirst)} {
    set ascLibrVect(btfirst) 0

   # set up code generation options
    set ascLibrVect(btmaxrel) 1000 ;# arbitrary limit. 0 if bt to be ignored.
    set ascLibrVect(g_uid) 1 ;# a unique id counter
    set ascLibrVect(btdir) \
      [file dirname [file dirname [info nameofexecutable]]]/lib
    set ascLibrVect(btdir) [file nativename $ascLibrVect(btdir)]
    set ascLibrVect(btverbose) 1 ;# don't comment the code
    set ascLibrVect(bthousekeep) 0 ;# do housekeep
    set ascLibrVect(bttarg) $ascUtilVect(asctmp)/asc[ascwhoami]bt[pid]_
    set ascLibrVect(bttarg) [file nativename $ascLibrVect(bttarg)]
    if {![file isdirectory $ascLibrVect(btdir)] || \
        ![file exists $ascLibrVect(btdir)/btprolog.h] || \
         ($tcl_platform(platform) == "unix" && \
          ![file exists $ascLibrVect(btdir)/Makefile.bt]) ||
         ($tcl_platform(platform) == "windows" && \
          ![file exists $ascLibrVect(btdir)/Makefile.bt.vc]) } {
      set ascLibrVect(compileC) 0 ;# no build files. suppress bintokens.
    }
    set ascLibrVect(btbuildfmt)  "%s -f \""
    append ascLibrVect(btbuildfmt)  $ascLibrVect(btdir)
    append ascLibrVect(btbuildfmt)  "%s\" RM=\"%s\" "
    append ascLibrVect(btbuildfmt)  "BTINCLUDES=-I\""
    append ascLibrVect(btbuildfmt)  $ascLibrVect(btdir)
    append ascLibrVect(btbuildfmt)  "\" BTTARGET=\""
    append ascLibrVect(btbuildfmt)  $ascLibrVect(bttarg)
    append ascLibrVect(btbuildfmt)  %d
    append ascLibrVect(btbuildfmt)  "\" \""
    append ascLibrVect(btbuildfmt)  $ascLibrVect(bttarg)
    append ascLibrVect(btbuildfmt)  %d
    append ascLibrVect(btbuildfmt)  "\" TMPDIR=\""
    append ascLibrVect(btbuildfmt)  [file nativename $ascUtilVect(asctmp)]
    append ascLibrVect(btbuildfmt)  "\""
    switch $tcl_platform(platform) {
    unix {
      set ascLibrVect(btunlink) {/bin/rm -f}
      set ascLibrVect(btmakeutil) make
      set ascLibrVect(btmakefile) /Makefile.bt
      }
    windows {
      set ascLibrVect(btunlink) del
      set ascLibrVect(btmakeutil) nmake
      set ascLibrVect(btmakefile) \\Makefile.bt.vc
    }
    default {
      set ascLibrVect(compileC) 0 ;# 0 --> bt to be ignored.
    }
    }
   ;# done first time only
  }
  incr ascLibrVect(g_uid)
  set srcname $ascLibrVect(bttarg)$ascLibrVect(g_uid).c
  set objname $ascLibrVect(bttarg)$ascLibrVect(g_uid).o
  set libname \
    $ascLibrVect(bttarg)$ascLibrVect(g_uid)[info sharedlibextension]
 # need 'if windows' here to handle TOOLS32 and ASCENDLIB
  set buildcommand \
    [format $ascLibrVect(btbuildfmt) $ascLibrVect(btmakeutil) \
       $ascLibrVect(btmakefile) $ascLibrVect(btunlink) \
       $ascLibrVect(g_uid) $ascLibrVect(g_uid)]
  set rellimit 0
  if {$ascLibrVect(compileC)} {
    set rellimit $ascLibrVect(btmaxrel)
    puts "C compiling by:"
    puts $buildcommand
  }
  sim_BinTokenSetOptions \
    $srcname \
    $objname \
    $libname \
    $buildcommand \
    $ascLibrVect(btunlink) \
    $rellimit \
    $ascLibrVect(btverbose) \
    $ascLibrVect(bthousekeep)
    
}

#
# proc Configure_Library {}
#-------------------------------------------------------------------
# bindings for library
#-------------------------------------------------------------------
proc Configure_Library {} {
  global ascLibrVect

  ascclearlist $ascLibrVect(moduleBox)
  ascclearlist $ascLibrVect(typeBox)
  ascclearlist $ascLibrVect(simsBox)

  # Update Enabled/Disabled entries when a menu is posted
  #
  .library.menubar.display configure \
     -postcommand Libr_Update_Display_Buttons
  .library.menubar.edit configure \
     -postcommand Libr_Update_Edit_Buttons
  .library.menubar.export configure \
     -postcommand Libr_Update_Export_Buttons
  .library.menubar.file configure \
     -postcommand Libr_Update_File_Buttons
  .library.menubar.find configure \
     -postcommand Libr_Update_Find_Buttons
  .library.menubar.view configure \
     -postcommand Libr_Update_View_Buttons


  bind $ascLibrVect(moduleBox) <Button-1> {
    set ndx [%W nearest %y]
    set ascLibrVect(entryline) ""
    if {$ndx != ""} {
      Libr_do_ModuleBox $ndx
    }
  }
  bind $ascLibrVect(entryBox) <F3> Libr_File_do_Copy

  ascRightMouseAddCommand $ascLibrVect(moduleBox) LibrDeleteAllState \
    command -label "Delete all types..." \
    -underline 11 -command Libr_do_DeleteAll

  ascRightMouseAddCommand $ascLibrVect(moduleBox) normal \
    command -label "Close window" \
    -underline 0 -command {Toggle_Remote ascLibrVect}


  bind $ascLibrVect(typeBox) <ButtonRelease-1> {
    set ndx [%W curselection]
    if {$ndx != ""} {
      set type [%W get $ndx]
      Libr_do_StatusLine $type
    }
  }

  bind $ascLibrVect(typeBox) <Double-1> {
    set ndx [%W curselection]
    if {$ndx != ""} {
      set type [%W get $ndx]
      Libr_do_StatusLine $type
      Libr_do_compile
    }
  }

  bind $ascLibrVect(typeBox) <Button-3> {
    set ndx [%W nearest %y]
    if {$ndx != ""} {
      set type [%W get $ndx]
      Libr_do_StatusLine $type
    }
  }

  ascRightMouseAddCommand $ascLibrVect(typeBox) LibrCreateSimState \
    command -label "Create simulation..." \
    -underline 0 -command Libr_do_compile

  ascRightMouseAddCommand $ascLibrVect(typeBox) LibrDeleteAllState \
    command -label "Find ATOM by units..." \
    -underline 0 -command Libr_do_FindAtom

  ascRightMouseAddCommand $ascLibrVect(typeBox) LibrSourceState \
    command -label "Display source code" \
    -underline 0 -command Disp_do_ShowCode

  ascRightMouseAddCommand $ascLibrVect(typeBox) LibrAncestryState \
    command -label "Display type ancestry" \
    -underline 0 -command Disp_do_ShowAncestry

  #
  # Bindings for the Simulations listbox
  #
  bind $ascLibrVect(simsBox) <1> {
    global ascSimsVect;
    set ndx [%W curselection];
    if {$ndx != ""} {
      set select [%W get $ndx];
      if {$select != ""} {
	set sim [lindex $select 0];
	set ascSimsVect(selectedsim) $sim;
      }
    }
  }

  bind $ascLibrVect(simsBox) <Double-1> {
    global ascSimsVect;
    set ndx [%W curselection];
    if {$ndx != ""} {
      set select [%W get $ndx];
      if {$select != ""} {
	set sim [lindex $select 0];
	set ascSimsVect(selectedsim) $sim;
      }
      Sims_Export2Browser $sim;
    }
  }

  bind $ascLibrVect(simsBox) <3> {
    global ascSimsVect;
    set ndx [%W nearest %y];
    if {$ndx != ""} {
      set select [%W get $ndx];
      if {$select != ""} {
	set sim [lindex $select 0];
	set ascSimsVect(selectedsim) $sim;
      }
    }
  }

  ascRightMouseAddCommand $ascLibrVect(simsBox) LibrSimPendingState \
    command -label "Display pending statements" \
    -underline -1 -command Disp_do_ShowPendings

  ascRightMouseAddCommand $ascLibrVect(simsBox) LibrSimSolverState \
    command -label "Export simulation to Solver" \
    -underline -1 -command Sims_do_Export2Solver

  ascRightMouseAddCommand $ascLibrVect(simsBox) LibrSimBrowserState \
    command -label "Export simulation to Browser" \
    -underline -1 \
    -command {Script_ClearInterrupt; BROWSE $ascSimsVect(selectedsim)}

  ascRightMouseAddCommand $ascLibrVect(simsBox) normal \
    separator

  ascRightMouseAddCommand $ascLibrVect(simsBox) LibrSimDeleteState \
    command -label "Delete simulation" \
    -underline -1 -command {DELETE $ascSimsVect(selectedsim)}

  VPane-Bind [winfo parent [winfo parent $ascLibrVect(moduleBox)]] \
     [winfo name [winfo parent $ascLibrVect(moduleBox)]] \
     [winfo name [winfo parent $ascLibrVect(typeBox)]] 10 0.333

  set ascLibrVect(initialized) "TRUE"
}


#
# proc Libr_do_Font {args}
#---------------------------------------------------------------------
# font select button for window
#---------------------------------------------------------------------
proc Libr_do_Font {args} {
  global ascLibrVect;
  set font ""
  if {$args != ""} {
    set font $args
  } else {
    set font  [ascFontGet]
  }
  if {$font == ""} {
    return;
  }
  $ascLibrVect(moduleBox) configure -font $font
  $ascLibrVect(typeBox) configure -font $font
  $ascLibrVect(entryBox) configure -font $font
  $ascLibrVect(simsBox) configure -font $font
  set ascLibrVect(font) [$ascLibrVect(typeBox) cget -font]
}

#
# proc Libr_recordhide {option} {typename} {partname ""}
#-------------------------------------------------------------------
#  Keeps a tcl list of what is suppressed and what isn't,
#  so that delete types doesn't force us to mark it all over
#  again.
#  Each time a file is read, just hide everything in this list
#  with a catch{} around the hide command so that no errors are
#  reported.
#  Examples-
#  to hide relations:
#  Libr_recordhide add relation
#  to hide the just the included flag on relations:
#  Libr_recordhide add relation.included
#  to unhide the relation or the relation.included flag:
#  Libr_recordhide delete relation
#  Libr_recordhide delete relation.included
#-------------------------------------------------------------------
proc Libr_recordhide {option typename args} {
  global ascLibrVect
  set argc [llength $args]
  if {$argc} {
    lappend typename $args
  }
  switch $option {
    {add} {
      if {[info exist ascLibrVect(hiddentypes)] == 0} {
        set ascLibrVect(hiddentypes) $typename
      } else {
        if {[lsearch -exact $ascLibrVect(hiddentypes) $typename] == -1} {
          lappend ascLibrVect(hiddentypes) $typename
        }
      }
    }
    {delete} {
      if {[info exist ascLibrVect(hiddentypes)] == 1} {
        set ascLibrVect(hiddentypes) \
          [delete_list_item $ascLibrVect(hiddentypes) $typename]
      }
    }
    default {
      error "Libr_recordhide called with unknown option (want add/delete)"
    }
  }
}

#
# proc Libr_updatehidden {}
#-------------------------------------------------------------------
# rehide everything, in Case any of it is not hidden and should be.
#-------------------------------------------------------------------
proc Libr_updatehidden {} {
  global ascLibrVect
  if {[info exist ascLibrVect(hiddentypes)] == 1} {
    foreach i $ascLibrVect(hiddentypes) {
      if {[llength $i]==1} {
        catch "Libr_hide_type $i" errmsg
      } else {
        catch "Libr_hide_type [lindex $i 0] [lindex $i 1]" errmsg
      }
    }
  }
}

# return normal if type is selected.
proc LibrTypeSelState {} {
  global ascLibrVect
  if {![info exists ascLibrVect(selectedtype)] || \
      [string compare $ascLibrVect(selectedtype) ""]==0} {
    return disabled
  }
  return normal
}
# reconfigure label of compile popup
proc LibrCreateSimState {} {
  global ascLibrVect ascPopInfo
  set lbl "Create $ascLibrVect(selectedtype) simulation..."
  $ascPopInfo(menu) entryconfigure $ascPopInfo(index) -label $lbl
  return [LibrTypeSelState]
}
proc LibrSourceState {} {
  global ascLibrVect ascPopInfo
  set lbl "Show $ascLibrVect(selectedtype) code..."
  $ascPopInfo(menu) entryconfigure $ascPopInfo(index) -label $lbl
  return [LibrTypeSelState]
}
proc LibrAncestryState {} {
  global ascLibrVect ascPopInfo
  set lbl "Show $ascLibrVect(selectedtype) ancestry..."
  $ascPopInfo(menu) entryconfigure $ascPopInfo(index) -label $lbl
  return [LibrTypeSelState]
}

proc LibrSimSelState {} {
  global ascSimsVect
  if {![info exists ascSimsVect(selectedsim)] || \
      [string compare $ascSimsVect(selectedsim) ""]==0} {
    return disabled
  }
  return normal
}

proc LibrSimPendingState {} {
  global ascSimsVect ascPopInfo
  set lbl "Check $ascSimsVect(selectedsim) for pending statements"
  $ascPopInfo(menu) entryconfigure $ascPopInfo(index) -label $lbl
  return [LibrSimSelState]
}

proc LibrSimSolverState {} {
  global ascSimsVect ascPopInfo
  set lbl "Export $ascSimsVect(selectedsim) to solver"
  $ascPopInfo(menu) entryconfigure $ascPopInfo(index) -label $lbl
  return [LibrSimSelState]
}

proc LibrSimBrowserState {} {
  global ascSimsVect ascPopInfo
  set lbl "Browse $ascSimsVect(selectedsim)"
  $ascPopInfo(menu) entryconfigure $ascPopInfo(index) -label $lbl
  return [LibrSimSelState]
}

proc LibrSimDeleteState {} {
  global ascSimsVect ascPopInfo
  set lbl "Delete $ascSimsVect(selectedsim)"
  $ascPopInfo(menu) entryconfigure $ascPopInfo(index) -label $lbl
  return [LibrSimSelState]
}
#
# proc Libr_do_DeleteAll {args}
#-------------------------------------------------------------------
# remove all definitions from the library
# pops a confirm button if called without anything in args.
#-------------------------------------------------------------------
proc Libr_do_DeleteAll {args} {
  global ascLibrVect
  if {$args == ""} {
    set position [setpos $ascLibrVect(windowname) 40 60]
    set res [VShowWindow.ascConfirm "220x50$position" "Delete All"]
    if {$res != 1} {
      return
    }
  }
  puts stderr "*************************************************"
  libr_destroy_types
  ascclearlist $ascLibrVect(typeBox)
  set ascLibrVect(entryline) ""
  set ascLibrVect(selectedtype) ""
  puts stdout "All types and simulations deleted."
  Libr_updatemod
  puts stderr "*************************************************"
  HUB_Message_to_HUB LIBDESTROYED
}

# proc Libr_ModuleBoxSelect {modulename}
#-------------------------------------------------------------------
# Simply selects the modulename given in the module box.
#-------------------------------------------------------------------
proc Libr_ModuleBoxSelect {modulename} {
  global ascLibrVect
  set modulelist [lsort -dictionary [libr_query -modulelist -mtype 0]]
  if {[llength $modulelist] == 0} {
    return;
  }
  set ndx [lsearch $modulelist $modulename]
  $ascLibrVect(moduleBox) selection clear 0 end
  $ascLibrVect(moduleBox) selection set $ndx
}

#
# proc Libr_do_StatusLine {type}
#-------------------------------------------------------------------
# set status line to type IN file
# The status line is being controlled by a -textvariable bound
# to the entry box. To write to the box, then just set the below variable.
# See the code in library.tcl for details.
#-------------------------------------------------------------------
proc Libr_do_StatusLine {type} {
  global ascLibrVect
  set moduleinfo [libr_moduleinfo [libr_query -findtype -type $type]]
  set filename [lindex $moduleinfo 1]
  set ascLibrVect(selectedtype) $type
  set ascLibrVect(entryline) "$type IN "
  append ascLibrVect(entryline) $filename
}

#
# proc Libr_updatetype {modname typename}
#-------------------------------------------------------------------
# stuff module contents into typebox. overwritten modules appear empty
# set status line.  If typename is provided, highlight that type.
#-------------------------------------------------------------------
proc Libr_updatetype {modname {typename ""}} {

  global ascLibrVect
  set typelist [lsort [libr_types_in_module $modname]];
  ascclearlist $ascLibrVect(typeBox);
  if {[llength $typelist] == 0} {
    return;
  }
  foreach type $typelist {
    $ascLibrVect(typeBox) insert end $type;
  }
  if {$typename != ""} {
    set ndx [lsearch $typelist $typename]
    $ascLibrVect(typeBox) see $ndx
    $ascLibrVect(typeBox) selection set $ndx
    Libr_do_StatusLine $typename
  }
}

#
# proc Libr_do_ModuleBox {ndx}
#-------------------------------------------------------------------
# If c library list size nonzero, transforms listbox index to c index
# and stuffs typebox accordingly.
# A little math required as the list is displayed in
# the reverse order or which it is stored internally.
# Here ndx is the unadjusted listbox index. The int_ndx is short for
# internal index (in Case you had not figured).
#-------------------------------------------------------------------
proc Libr_do_ModuleBox {ndx} {
  global ascLibrVect
  if {$ndx == ""} {
    return;
  }
  if {[$ascLibrVect(moduleBox) size] == 0} {
    return;
  }
  set modulename [$ascLibrVect(moduleBox) get $ndx]
  Libr_updatetype $modulename
}

#
# Libr_Redraw {}
#-------------------------------------------------------------------
# repaint after restart of libr window
#-------------------------------------------------------------------
proc Libr_Redraw {} {
  Libr_updatemod
}
#
# proc Libr_file_get {fullname}
#-------------------------------------------------------------------
# read a file to library
# Now takes an additional arguement which gets passed onto
# librread. This flag if set to 0, will disable the parsing
# of relations.
#-------------------------------------------------------------------
proc Libr_file_get {fullname {dorelns "1"}} {

  global ascLibrVect
  set fullname [file nativename $fullname]
  global ascLibrVect
  #   puts -nonewline stderr "libr_file_get args: "
  #   puts stderr $fullname
  if {[file isfile $fullname]} {
    #   puts stdout "File $fullname was found. Now opening...\n";
    set ascLibrVect(fullfilename) $fullname;
    #
    # read and parse the file and update the module and
    # type boxes. This the only file-based interface call to the compiler
    # as of jun 1 1997. lets keep it that way.
    asc_compiler_option -parserWarnings $ascLibrVect(parserWarnings)
    asc_compiler_option -compilerWarnings $ascLibrVect(compilerWarnings)
    asc_compiler_option -simplifyRelations $ascLibrVect(simplifyRelations)
    set modulename [librread $fullname $dorelns];
    Libr_updatemod;
    Libr_updatehidden;
    Libr_updatetype $modulename;
    #
    # set up the status line. and select the
    # main module that was opened.
    set ascLibrVect(entryline) ""
    Libr_ModuleBoxSelect $modulename;
    HUB_Message_to_HUB SOURCEREAD $fullname
    return 0;
  } else {
    set errmsg "File\n"
    append errmsg $fullname
    append errmsg "\n not found!"
    puts stderr $errmsg
    Script_Raise_Alert $errmsg "File Error"
    return 1;
  }
}

#
# proc Libr_do_read {}
#-------------------------------------------------------------------
# libr read button
#-------------------------------------------------------------------
proc Libr_do_read {} {
  global ascLibrVect ascGlobalVect asc_tkfbox
  set defaultname [file dirname "$ascLibrVect(fullfilename)"]
  if {$defaultname == "."} { set defaultname [pwd]}
  set asc_tkfbox(otherdirs) $ascGlobalVect(librarypathdirs)

  set fullname [tk_getOpenFile \
    -defaultextension "" \
    -filetypes $ascLibrVect(filetypes) \
    -initialdir $defaultname \
    -title {Read Ascend IV atoms/models} \
    -parent .library]

  if {$fullname == "" || [file isdirectory $fullname]} {
    if {!$asc_tkfbox(cancelled)} {
      tk_dialog .fileerr FYI \
        "\"$fullname\" cannot be read." "" 0 OK
    }
    return
  }
  if {[file extension $fullname] != ""} {
    set ascLibrVect(lastreadextension) [file extension $fullname]
    ascresort_filetypes ascLibrVect lastreadextension
    lappend ascGlobalVect(librarypathdirs) [file dirname $fullname]
  }
  Libr_file_get $fullname
}


#
# proc Libr_updatemod
#-------------------------------------------------------------------
# stuff module box in library from c struct
#-------------------------------------------------------------------
proc Libr_updatemod {} {
  global ascLibrVect
  set modulelist [lsort -dictionary [libr_query -modulelist -mtype 0]];
  ascclearlist $ascLibrVect(moduleBox);
  if {[llength $modulelist] == 0} {
    return;
  }
  foreach module $modulelist {
    $ascLibrVect(moduleBox) insert end $module;
  }
}

#
# proc newraise {w}
#-------------------------------------------------------------------
# raise a window, deiconifying if need be -- uses raise.
#-------------------------------------------------------------------
proc newraise {w} {
  if {![winfo exists $w]} {
     return
  }
  if {![winfo ismapped $w]} {
     switch $w {
     {.browser} {Toggle_Remote ascBrowVect}
     {.units} {Toggle_Remote ascUnitVect}
     {.solver} {Toggle_Remote ascSolvVect}
     {.library} {Toggle_Remote ascLibrVect}
     {.display} {Toggle_Remote ascDispVect}
     {.toolbox} {Toggle_Remote ascToolVect}
     {.probe} {Toggle_Remote ascProbVect}
     default {wm deiconify $w}
     }
   } else {
     raise $w
   }
}

#
# proc Libr_do_compile {}
#-------------------------------------------------------------------
# library create compile button
# For ease of code reading this procedure is now in the file SimsProc.tcl.
#-------------------------------------------------------------------

# internal use only
# proc Libr_do_FindFuzzy_ftmessage {}
#-------------------------------------------------------------------
#-------------------------------------------------------------------
proc Libr_do_FindFuzzy_ftmessage {} {
  global AscMonoEntry1 
  set help "Enter a name of the sort of thing you want,"
  append help " and this finds reasonable matches for it."
  append help " You can use * to match any characters or ? to match a "
  append help " single character."
  if {$AscMonoEntry1(button)==3} {
    Script_Raise_Alert $help "Find type by fuzzy name"
    return 1
  }
}

# internal use only
# proc Libr_do_FindAtom_message {}
#-------------------------------------------------------------------
#-------------------------------------------------------------------
proc Libr_do_FindAtom_message {} {
  global AscMonoEntry1
  set help "Enter the units you want in a real,"
  append help " and this finds matching atoms and real_constants."
  append help " This works better when atoms.a4l is loaded."
  if {$AscMonoEntry1(button)==3} {
    Script_Raise_Alert $help "Find atoms by their units"
    return 1
  }
}

#
# proc Libr_do_FindAtom  {}
#-------------------------------------------------------------------
# The library find ATOM button. This button will take the units given
# and search the entire type library for matching atoms
#-------------------------------------------------------------------
proc Libr_do_FindAtom {} {
  global ascLibrVect
  global AscMonoEntry1
  if {[$ascLibrVect(moduleBox) size] == 0 } {
    return 1;
  }
  #
  # Grab the units from the user
  #
  set AscMonoEntry1(font) $ascLibrVect(font)
  set target_units [lindex [VShowWindow.ascMonoEntry1 \
                           "Match units?" \
                           "600x100[setpos .library 100 50]" \
                           $ascLibrVect(target_units) \
                           "" \
                           Libr_do_FindAtom_message] \
                   1]
  if {$AscMonoEntry1(button)==2} {
    return 1
  }
  if {[catch {set atoms [u_get_atoms $target_units]} err]} {
    Script_Raise_Alert "Unable to match $target_units. $err" " failure"
    return 1
  }
  set ascLibrVect(target_units) $target_units
  if {[llength $atoms] == 0} {
    Script_Raise_Alert \
      "No matches of $target_units found. You need to define a new ATOM." \
      " failure"
    return 1
  }
  Libr_Setup_UnitsMatches $target_units $atoms
}
#
# proc Libr_do_FindFuzzy  {}
#-------------------------------------------------------------------
# The library find button. This button will take the name given
# and search the entire type library for it ignoring Case and
# leading/trailing characters and _s.
#-------------------------------------------------------------------
proc Libr_do_FindFuzzy {} {
  global ascLibrVect
  global AscMonoEntry1
  if {[$ascLibrVect(moduleBox) size] == 0 } {
    return 1;
  }
  #
  # Grab the name of the type from the user
  #
  set AscMonoEntry1(font) $ascLibrVect(font)
  set target_type [lindex [VShowWindow.ascMonoEntry1 \
                           "Match pattern?" \
                           "600x100[setpos .library 100 50]" \
                           $ascLibrVect(target_type) \
                           "" \
                           Libr_do_FindFuzzy_ftmessage] \
                   1]
  if {$AscMonoEntry1(button)==2} {
    return 1
  }
  if {$target_type == ""} {
    return 1
  }
  set types [libr_query -catalog]
  set base [stringcompact [string tolower $target_type]]
  set matches ""
  foreach i $types {
    set t [string tolower $i]
    # check against user wildcards
    if {[string match $base $t]} {
      lappend matches $i
      continue
    }
    # check against embedded possibility
    if {[string match *$base* $t]} {
      lappend matches $i
      continue
    }
    # check against embedded possibility
    if {[string match *$base* $t]} {
      lappend matches $i
      continue
    }
    regsub -all -- _ $t "" noub
    # check against embedded possibility
    if {[string match $base $noub]} {
      lappend matches $i
      continue
    }
    # check against embedded possibility
    if {[string match *$base* $noub]} {
      lappend matches $i
      continue
    }
  }
  if {$matches == ""} {
    Script_Raise_Alert "No matches of $target_type found." "Fuzz failure"
    return 1
  }
  Libr_Setup_FuzzyMatches $target_type $matches
}

# internal use only
# proc Libr_do_FindType_ftmessage {}
#-------------------------------------------------------------------
#-------------------------------------------------------------------
proc Libr_do_FindType_ftmessage {} {
  global AscMonoEntry1
  set help "Enter the exact name of the type you want,"
  append help " and this takes you to its module"
  if {$AscMonoEntry1(button)==3} {
    Script_Raise_Alert $help "Find type by name"
    return 1
  }
}
#
# proc Libr_do_FindType  {}
#-------------------------------------------------------------------
# The library find button. This button will take the name selected
# and search the entire type library for it. When found will focus
# the selection in the typebox onto the name of the type found.
# the module associated with the type will also be focused in the
# ModuleBox.
#-------------------------------------------------------------------
proc Libr_do_FindType {} {
  global ascLibrVect
  global AscMonoEntry1
  if {[$ascLibrVect(moduleBox) size] == 0 } {
    return 1;
  }
  #
  # Grab the name of the type from the user
  #
  set target_type [lindex [VShowWindow.ascMonoEntry1 \
                           "Find Which Type?" \
                           "400x100[setpos .library 100 50]" \
                           $ascLibrVect(target_type) \
                           "" \
                           Libr_do_FindType_ftmessage] \
                   1]
  if {$AscMonoEntry1(button)==2} {return 1}
  if {$target_type ==""} { return 1 }
  Libr_FocusType $target_type
}

proc Libr_FocusType {target_type} {
  global ascLibrVect
  set ascLibrVect(target_type) $target_type;
  #
  # Call the c-routine to get the filename
  #
  set nok [catch {libr_query -findtype -type $target_type} name_or_errmsg]
  if {$nok} {
    Script_Raise_Alert $name_or_errmsg "Find Error";
    return;
  }
  set modulename $name_or_errmsg
  #
  # Now that we have the associated file name, then set it in
  # C-land, and prepare to update the boxes. We wont use the
  # normal Libr_update type procs as we need to do some focusing, and
  # we dont want to call "updatetype" twice; updateype is fairly expensive.
  #
  Libr_updatetype $modulename $target_type
  Libr_ModuleBoxSelect $modulename
}

#
# proc Libr_Update_File_Buttons {}
#-------------------------------------------------------------------------
# This procedure is bound to the File Menu Button.
# Used for enabling/disabling the items depending on the whether selections
# exist within the ascLibrVect(typeBox)
# selected instance
#-------------------------------------------------------------------------
proc Libr_Update_File_Buttons {} {
  # Read file entry is always valid, so just return
  #global ascLibrVect
  #set mb .library.menubar.file
  return
}

# proc LibrDeleteAllState {}
#-------------------------------------------------------------------------
# returns the state for the delete all menu button.
# Basically requires that some modules must exist.
#-------------------------------------------------------------------------
proc LibrDeleteAllState {} {
  global ascLibrVect
  if {[$ascLibrVect(moduleBox) size] > 0} {
    return normal
  }
  return disabled
}

#
# proc Libr_Update_Edit_Buttons {}
#-------------------------------------------------------------------------
# For disabling the Edit Menu Button
# Used for enabling/disabling the items depending on the whether
# files exist within the ascLibrVect(moduleBox)
#-------------------------------------------------------------------------
proc Libr_Update_Edit_Buttons {} {
  global ascLibrVect
  set mb .library.menubar.edit

  foreach i {0 1 3 4} {
    $mb entryconfigure $i -state disabled;
  }

  # Create simulation
  if {[$ascLibrVect(typeBox) curselection] != ""} {
    $mb entryconfigure 0 -state normal;
  }
  # Suggest methods
  set type "$ascLibrVect(selectedtype)"
  if {$type != ""} {
    $mb entryconfigure 1 -state normal;
  }
  # Delete simulation
  if {[$ascLibrVect(simsBox) curselection] != ""} {
    $mb entryconfigure 3 -state normal
  }
  # Delete all types
  $mb entryconfigure 4 -state [LibrDeleteAllState]
}

#
# proc Libr_Update_Display_Buttons {}
#-------------------------------------------------------------------------
# This procedure is bound to the Display Menu Button.
# Used for enabling/disabling the items depending on the whether selections
# exist within the ascLibrVect(typeBox) selected instance.
# The enabling/disabling of the Hide/Unhide Type depends also of the
# current status of the type selected. i.e. only the appropriate item
# will be enabled.VRR
#-------------------------------------------------------------------------
proc Libr_Update_Display_Buttons {} {
  global ascLibrVect
  set mb .library.menubar.display

  # External Functions and Hide/Show Fundamentals are always active
  $mb entryconfigure 3 -state normal
  $mb entryconfigure 6 -state normal

  # Disable everything else
  $mb entryconfigure 0 -state disabled;
  $mb entryconfigure 1 -state disabled;
  $mb entryconfigure 2 -state disabled;
  $mb entryconfigure 4 -state disabled;
  $mb entryconfigure 5 -state disabled;

  # If no type is selected, we are done
  if {[$ascLibrVect(typeBox) curselection] == ""} {
    return
  }

  set type "$ascLibrVect(selectedtype)"
  if {$type != ""} {
    $mb entryconfigure 0 -state normal
    $mb entryconfigure 1 -state normal
    $mb entryconfigure 2 -state normal

    # To check if a type is being shown
    set is_type_shown [libr_type_is_shown $type]
    if {$is_type_shown} {
      $mb entryconfigure 4 -state normal;
    } else {
      $mb entryconfigure 5 -state normal;
    }
  }
}

#
# proc Libr_Update_Find_Buttons {}
#-------------------------------------------------------------------------
# This procedure is bound to the Find Menu Button.
# Used for enabling/disabling the items depending on the whether
# files exist within the ascLibrVect(moduleBox)
#-------------------------------------------------------------------------
proc Libr_Update_Find_Buttons {} {
  global ascLibrVect
  set mb .library.menubar.find

  if {[$ascLibrVect(moduleBox) size] == 0} {
    $mb entryconfigure 0 -state disabled;
    $mb entryconfigure 1 -state disabled;
    $mb entryconfigure 2 -state disabled;
  } else {
    $mb entryconfigure 0 -state normal;
    $mb entryconfigure 1 -state normal;
    $mb entryconfigure 2 -state normal;
  }
  if {[$ascLibrVect(simsBox) curselection] == ""} {
    $mb entryconfigure 3 -state disabled;
  } else {
    $mb entryconfigure 3 -state normal;
  }
}


#
# proc Libr_Update_View_Buttons {}
#-------------------------------------------------------------------------
# This procedure is bound to the View Menu Button.
# Used for enabling/disabling the items depending on the whether
# the ascGlobalVect(saveoptions) is on or off
#-------------------------------------------------------------------------
proc Libr_Update_View_Buttons {} {
  global ascLibrVect ascGlobalVect

  set mb .library.menubar.view

 if {$ascGlobalVect(saveoptions) == 0} {
    $mb entryconfigure 2 -state disabled
  } else {
    $mb entryconfigure 2 -state normal
  }

}

#
# proc Libr_Update_Export_Buttons {}
#-------------------------------------------------------------------------
# This procedure is bound to the Export Menu Button.
# Used for enabling/disabling the items depending on the whether
# simulations exist within the ascLibrVect(simsBox)
#-------------------------------------------------------------------------
proc Libr_Update_Export_Buttons {} {
  global ascLibrVect
  set mb .library.menubar.export

  if {[$ascLibrVect(simsBox) curselection] == ""} {
    $mb entryconfigure 0 -state disabled;
    $mb entryconfigure 1 -state disabled;
    $mb entryconfigure 2 -state disabled;
  } else {
    $mb entryconfigure 0 -state normal;
    $mb entryconfigure 1 -state normal;
    $mb entryconfigure 2 -state normal;
  }
}

#
# proc Libr_do_Help {}
# proc Libr_do_BindHelp
#-------------------------------------------------------------------
# the library help buttons
#-------------------------------------------------------------------
proc Libr_do_Help {} {
  Help_button library
}
proc Libr_do_BindHelp {} {
  Help_button library.help onlibrary
}

#
proc Libr_hide_type {type} {
  if {[catch {libr_hide_type $type} msg]} {
    Script_Raise_Alert "For >$type<:\n $msg" "Hide error"
  }
}
# proc Libr_do_Hide_Type  {}
#-------------------------------------------------------------------
# this procedure calls Libr_hide_type for the type selected by the
# user. It will cause that all of the instances of such a type
# will be ignored for browsing purposes.
#-------------------------------------------------------------------
proc Libr_do_Hide_Type {} {
  global  ascLibrVect
  set type "$ascLibrVect(selectedtype)"
  if {$type == ""} {
    return;
  }

  if {[catch {Libr_hide_type $type} msg]} {
    Script_Raise_Alert "For >$type<:\n $msg" "Hide error"
  }
  Libr_recordhide add $type
  HUB_Message_to_HUB TYPEHIDDEN
  Libr_Update_Display_Buttons;
  return
}

# proc Libr_do_Hide_Type_Refinements  {}
#-------------------------------------------------------------------
# this procedure calls Libr_hide_type for the type selected by the
# user. It will cause that all of the instances of such a type
# will be ignored for browsing purposes.
# It also hides all refinements.
#-------------------------------------------------------------------
proc Libr_do_Hide_Type_Refinements {} {
  global ascLibrVect
  set type "$ascLibrVect(selectedtype)"
  if {$type == ""} {
    return;
  }
  set list [drefines_meall $type]
  lappend list $type
  foreach i $list {
    Libr_hide_type $i
    Libr_recordhide add $i
  }
  HUB_Message_to_HUB TYPEHIDDEN
  Libr_Update_Display_Buttons;
  return
}

# proc Libr_do_UnHide_Type  {}
#-------------------------------------------------------------------
# this procedure calls libr_unhide_type for the type selected by the
# user. It will cause that all of the instances of such a type
# (previously hidden) will be considered for browsing purposes.
#-------------------------------------------------------------------
proc Libr_do_UnHide_Type {} {
  global ascLibrVect
  set type "$ascLibrVect(selectedtype)"
  if {$type == ""} {
    return;
  }
  libr_unhide_type $type
  Libr_recordhide delete $type
  HUB_Message_to_HUB TYPEHIDDEN
  Libr_Update_Display_Buttons;
  return
}
#
# proc Libr_do_UnHide_Type_Refinements {}
#-------------------------------------------------------------------
# this procedure calls libr_unhide_type for the type selected by the
# user. It will cause that all of the instances of such a type
# (previously hidden) will be considered for browsing purposes.
#-------------------------------------------------------------------
proc Libr_do_UnHide_Type_Refinements {} {
  global ascLibrVect
  set type "$ascLibrVect(selectedtype)"
  if {$type == ""} {
    return;
  }
  set list [drefines_meall $type]
  lappend list $type
  foreach i $list {
    libr_unhide_type $i
    Libr_recordhide delete $i
  }
  HUB_Message_to_HUB TYPEHIDDEN
  Libr_Update_Display_Buttons;
  return
}


# proc Libr_do_Hide_Fundamentals  {}
#-------------------------------------------------------------------
# Allows to hide/unhide fundamental type whose definitions are not
# displayed as included in some library.
#-------------------------------------------------------------------
proc Libr_do_Hide_Fundamentals {} {
  Libr_Setup_FundamentalsBox
  return
}

# proc Libr_Setup_FundamentalsBox
#-------------------------------------------------------------------------
# Sets up the box of fundamental types so that the hidding/unhidding of
# fundamentals may be done.
#-------------------------------------------------------------------------
proc Libr_Setup_FundamentalsBox {} {

  global ascListSelectB1Box ascLibrVect
  set list ""
  catch {set list [libr_query -fundamentals]}

  set ascListSelectB1Box(grab) 0
  set ascListSelectB1Box(btn2name) TagAll
  set ascListSelectB1Box(btn3name) Hide
  set ascListSelectB1Box(btn4name) UnHide
  set ascListSelectB1Box(btn5name) ""
  set ascListSelectB1Box(btn2destroy) 0
  set ascListSelectB1Box(btn3destroy) 0
  set ascListSelectB1Box(btn4destroy) 0
  set ascListSelectB1Box(btn2command) Libr_SelectAll
  set ascListSelectB1Box(btn3command) Libr_HideListSelect
  set ascListSelectB1Box(btn4command) Libr_UnHideListSelect
  set ascListSelectB1Box(title) "Show/Hide Fundamentals"
  set ascListSelectB1Box(toplevelname) ".librfund"
  set ascListSelectB1Box(selectmode) extended
  set ascListSelectB1Box(font) $ascLibrVect(font)
  set ascListSelectB1Box(headline) "Fundamental Types:"

   if {$list==""} {puts stderr "fundamental types not found" ; return}
   set button [AscListSelectB1Box $list \
		  250x240[setpos .library 150 20]]
}

# internal use only
# proc Libr_FuzzRefines {}
#-------------------------------------------------------------------
#-------------------------------------------------------------------
proc Libr_FuzzRefines {} {
  global ascListSelectB1Box
  set type $ascListSelectB1Box(itemselected)
  Type_OpenTree $type
}

# internal use only
# proc Libr_FuzzCode {}
#-------------------------------------------------------------------
#-------------------------------------------------------------------
proc Libr_FuzzCode {} {
  global ascListSelectB1Box
  set type $ascListSelectB1Box(itemselected)
  Libr_FocusType $type
  Disp_do_ShowCode $type
}

# internal use only
# proc Libr_FuzzCompile {}
#-------------------------------------------------------------------
#-------------------------------------------------------------------
proc Libr_FuzzCompile {} {
  global ascListSelectB1Box
  set type $ascListSelectB1Box(itemselected)
  Libr_FocusType $type
  Libr_do_compile
}

# proc Libr_Setup_FuzzyMatches
#-------------------------------------------------------------------------
# Sets up the box of matching types so that they may be used.
#-------------------------------------------------------------------------
proc Libr_Setup_FuzzyMatches {query list} {

  global ascListSelectB1Box ascLibrVect

  set ascListSelectB1Box(grab) 0
  set ascListSelectB1Box(btn2name) Code
  set ascListSelectB1Box(btn3name) Refines
  set ascListSelectB1Box(btn4name) Compile
  set ascListSelectB1Box(btn5name) ""
  set ascListSelectB1Box(btn2destroy) 0
  set ascListSelectB1Box(btn3destroy) 0
  set ascListSelectB1Box(btn4destroy) 0
  set ascListSelectB1Box(btn2command) Libr_FuzzCode
  set ascListSelectB1Box(btn3command) Libr_FuzzRefines
  set ascListSelectB1Box(btn4command) Libr_FuzzCompile
  set ascListSelectB1Box(font) $ascLibrVect(font)
  set ascListSelectB1Box(title) "Fuzzy matches"
  set ascListSelectB1Box(toplevelname) .librfuzz
  set ascListSelectB1Box(selectmode) single
  set ascListSelectB1Box(headline) $query

  set button [AscListSelectB1Box $list 350x240[setpos .library 150 20]]
}

# proc Libr_Setup_UnitsMatches
#-------------------------------------------------------------------------
# Sets up the box of matching atoms so that they may be used.
#-------------------------------------------------------------------------
proc Libr_Setup_UnitsMatches {query list} {

  global ascListSelectB1Box ascLibrVect

  set ascListSelectB1Box(grab) 0
  set ascListSelectB1Box(btn2name) Code
  set ascListSelectB1Box(btn3name) Refines
  set ascListSelectB1Box(btn4name) ""
  set ascListSelectB1Box(btn5name) ""
  set ascListSelectB1Box(btn2destroy) 0
  set ascListSelectB1Box(btn3destroy) 0
  set ascListSelectB1Box(btn4destroy) 0
  set ascListSelectB1Box(btn2command) Libr_FuzzCode ;# works for atoms too
  set ascListSelectB1Box(btn3command) Libr_FuzzRefines ;# works for atoms too
  set ascListSelectB1Box(font) $ascLibrVect(font)
  set ascListSelectB1Box(title) "real and real_constant matching"
  set ascListSelectB1Box(toplevelname) .libratomunits
  set ascListSelectB1Box(selectmode) single
  set ascListSelectB1Box(headline) $query

  set button [AscListSelectB1Box $list 350x240[setpos .library 150 20]]
}

#
# proc Libr_SelectAll {{tl ""}}
#-------------------------------------------------------------------------
# select all in the asclistselectb1box associate with tl, or
# select all in the last asclistselectb1box created if tl == ""
#-------------------------------------------------------------------------
proc Libr_SelectAll {{tl ""}} {
  AscListSelectB1SelectAll $tl
}

#
# proc Libr_HideListSelect {}
#-------------------------------------------------------------------------
# for all of the fundamental types selected, it calls  Libr_hide_type.
# It will cause that all of the instances of such  types
# will be ignored for browsing purposes.
#-------------------------------------------------------------------------
proc Libr_HideListSelect {} {
  global ascListSelectB1Box
  set list $ascListSelectB1Box(itemselected)
  foreach i $list {
    Libr_hide_type $i
    Libr_recordhide add $i
  }
  HUB_Message_to_HUB TYPEHIDDEN
  Libr_Update_Display_Buttons;
}
#
# proc Libr_UnHideListSelect {}
#-------------------------------------------------------------------------
# for all of the fundamental types selected, it calls  libr_unhide_type.
# It will cause that all of the instances of such types
# (previously hidden) will be considered for browsing purposes.
#-------------------------------------------------------------------------
proc Libr_UnHideListSelect {} {
  global ascListSelectB1Box
  set list $ascListSelectB1Box(itemselected)
  foreach i $list {
    libr_unhide_type $i
    Libr_recordhide delete $i
  }
  HUB_Message_to_HUB TYPEHIDDEN
  Libr_Update_Display_Buttons;
}


# proc Libr_Get_CurrentSim
#------------------------------------------------------------------------
# The following code grabs the currently selected simulation in the
# sims listbox in the Library window. It will return the empty string,
# or the simulation name
#------------------------------------------------------------------------
proc Libr_Get_CurrentSim {} {
  global ascLibrVect
  set ndx [$ascLibrVect(simsBox) curselection];
  if {$ndx == ""} {
    return "";
  }
  return [lindex [$ascLibrVect(simsBox) get $ndx] 0];
}

#
# proc Sims_update_SimsBox
#------------------------------------------------------------------------
# stuff sims from c structure, autoselect last compiled
#------------------------------------------------------------------------
proc Sims_update_SimsBox {} {
  global ascSimsVect;
  global ascLibrVect

  set simlist [slist];
  set len [llength $simlist]
  if {$len == 0} {
    set ascSimsVect(selectedsim) "";
    return;
  }
  ascclearlist $ascLibrVect(simsBox);
  foreach sim $simlist {
    set sim_type [simtype $sim]
    $ascLibrVect(simsBox) insert end "$sim IS_A $sim_type";
  }
  set ndx [lsearch $simlist $ascSimsVect(target_instance)]
  $ascLibrVect(simsBox) select set $ndx
}

#
# proc Sims_CreateInstance {tinst type}
#------------------------------------------------------------------------
# Creates an instance. returns 1 if failed.,0 if succeeded.
# Tests for uniqueness in tcl and in C. In C so commandline
# create works, and in tcl so we can popup a box based on a test.
# tinst is the target instancee. type is the type of instance.
#------------------------------------------------------------------------
proc Sims_CreateInstance {tinst type} {
  global ascSimsVect ascLibrVect

  if {$tinst == ""} {
    Script_Raise_Alert "You must specify\nan instance name." "Create Error";
    return 1;
  }
  if {![sim_unique $tinst]} {
    Script_Raise_Alert "Simulation with name\n$tinst already exists!" \
      "Create Error"
    return 1;
  }
  asc_compiler_option -useCopyAnon $ascLibrVect(useCopyAnon)
  set nok [catch {
             sim_instantiate $tinst $type -m $ascLibrVect(autorun)
           } err_msg]
  if {$nok} {
    puts stderr $err_msg
    Script_Raise_Alert \
      "CreateInstance\n $tinst failed.\n$err_msg" \
      "Create Error"
    return 1;
  }
  return 0;
}

#
#
proc Library_Handle_Configure {inst type} {
  # C compiler already does it.
  return
}
proc Libr_do_SaveOption {} {
  View_Save_Window_Options library
}

#
# proc Sims_InquireInstStatus {}
#------------------------------------------------------------------------
# this takes care of everything required to make a new sim
# ascSimsVect(instanceType) is assumed to have been set.
# type  -- the type of the instance.
# tinst -- the target instance.
#------------------------------------------------------------------------
proc Sims_InquireInstStatus {type} {
  global ascSimsVect
  global AscMonoEntry1

  set tinst [lindex [VShowWindow.ascMonoEntry1 \
    "Simulation name:" "400x100[setpos .library 50 50]" \
    $ascSimsVect(target_instance) "" ""] 1]
  if {$AscMonoEntry1(button)==2} {return 1}
  if {$AscMonoEntry1(button)==3} {
    set expl {Enter a name for the new simulation. No "." or  "[" or "]" or "'" allowed.}
    Script_Raise_Alert $expl "Name help:"
    return 1
  }
  if {$tinst ==""} {
    Script_Raise_Alert "You must specify an instance name." "Create Error";
    return 1;
  }
  # cache the instance name for the next call
  set ascSimsVect(target_instance) [string trim $tinst]
  set result [Sims_CreateInstance $tinst $type];
  return $result;
}

#
# proc Libr_do_compile {}
#-------------------------------------------------------------------
# library create compile button
# For ease of code reading this code is now in the
# file SimsProc.tcl.
#-------------------------------------------------------------------
proc Libr_do_compile {} {

  global ascSimsVect ascLibrVect
  set type $ascLibrVect(selectedtype)
  if {$type == ""} {
    return;
  }
  if {[Sims_InquireInstStatus $type] != 0} {
    return;
  }
  newraise $ascSimsVect(windowname);
  HUB_Message_to_HUB SIMCREATED $ascSimsVect(target_instance) $type
  Sims_update_SimsBox ;
}

#
# proc Sims_Redraw {}
#------------------------------------------------------------------------
# standard Proc to restart after interface reload
#------------------------------------------------------------------------
proc Sims_Redraw {} {
  Sims_update_SimsBox
}


#
# proc Sims_Export2Browser {sim}
#------------------------------------------------------------------------
# just what it says
#------------------------------------------------------------------------
proc Sims_Export2Browser {sim} {

  global ascSimsVect ascLibrVect
  global ascBrowVect

  if {$sim == ""} {
    return;
  } else {
    # set the external sim -- used ONLY for bringing up the
    # name of the simulation in the entry box, for subsequent
    # calls.
    set ascBrowVect(currentsim) $sim;
    # set the internal sim -- this is the important one.
    if {[bnumpendings simulation $sim] > 0} {
      Script_Raise_Alert \
	"Simulation $sim\nhas pending instances.\n Please correct code\nbefore \
proceeding." "Create Error"
    puts "\n--------------------------------------------------------------\n\
          Pendings statements for simulation $sim\n"
    bwritependings $sim
    puts "\n--------------------------------------------------------------"
    }
    Script_ClearInterrupt
    BROWSE $sim
  }
}

proc Sims_do_Export2Browser {} {
  set sim [Libr_Get_CurrentSim]
  if {$sim == ""} {return}
  Sims_Export2Browser $sim
}

#
# proc Sims_Delete {sim}
#------------------------------------------------------------------------
# This function deletes a simulation. It will send a message to the HUB
# that the named simulation is about to be destroyed. This will tell
# everyone to take their hands off, and to clean up the windows displaying
# information related to the sim.
# Do not call sim_destroy from _anywhere_ else.
#------------------------------------------------------------------------
proc Sims_Delete {sim} {
  if {$sim != "" && "[lsearch -exact [slist] $sim]" != "-1"} {
    HUB_Message_to_HUB "SIMDELETED" $sim;
    sim_destroy $sim;
  } else {
    if {$sim != ""} {
      puts "Simulation $sim doesn't exist or already deleted"
    }
  }
}

#
# proc Sims_do_Delete {args}
#------------------------------------------------------------------------
# This is the command bound to the simulation delete menubutton. It grabs
# the currently selected simulation to be deleted and passes it to
# Sims_Delete to do the real work. It then cleans up its listbox.
# This function asks for confirmation unless `args' is non-empty
#------------------------------------------------------------------------
proc Sims_do_Delete {args} {
  # The command bound to the MenuuButton
  global ascLibrVect;

  set ndx [$ascLibrVect(simsBox) curselection];
  if {$ndx == ""} {
    return;
  }
  set sim [Libr_Get_CurrentSim];

  if {$args == ""} {
    set position [setpos $ascLibrVect(windowname) 40 60]
    set res [VShowWindow.ascConfirm "220x50$position" "Delete $sim"]
    if {$res != 1} {
      return
    }
  }

  Sims_Delete $sim
  $ascLibrVect(simsBox) delete $ndx $ndx
}

#
# proc Sims_Pendings_To_Stdout {}
#------------------------------------------------------------------------
# This command is bound to a menu item in the Sims window and will write
# the pendings instance list to stdout.
#------------------------------------------------------------------------
proc Sims_Pendings_To_Stdout {} {

  set sim [Libr_Get_CurrentSim]
  if {$sim == ""} {return}
  if {0 && [bnumpendings simulation $sim] > 3} {
    # here we should be checking the number of pendings
    # and asking for the filename to write them to if too big.
    simlistpending $sim
  }
  simlistpending $sim
}

#
# proc Sims_Pendings_To_File {}
#------------------------------------------------------------------------
# This command is bound to a menu item in the Sims window and will write
# the pendings instance list to stdout.
#------------------------------------------------------------------------
proc Sims_Pendings_To_File {} {
  global ascSimsVect

  set sim [Libr_Get_CurrentSim]
  if {$sim == ""} {
    return;
  }
  puts [bnumpendings simulation $sim]

  set defaultname $ascSimsVect(filename)
  set filename [tk_getSaveFile \
    -defaultextension "" \
    -filetypes $ascSimsVect(filetypes) \
    -initialfile $defaultname \
    -parent .library \
    -title {Write pending file}]

  if {$filename == ""} {
    return 1;
  } {
    set ascSimsVect(filename) $filename
    simlistpending $sim $filename
    puts "Wrote pendings file $filename."
  }
}

#
# proc Sims_Export2solver {}
#------------------------------------------------------------------------
# just what it says. needs work. baa
#------------------------------------------------------------------------
proc Sims_Export2Solver {sim} {

  if {$sim == ""} {return}
  return [Solve_Import_Any $sim]
}

proc Sims_do_Export2Solver {} {
  global ascSimsVect
  set sim $ascSimsVect(selectedsim)
  if {$sim == ""} {
    return;
  }
  Sims_Export2Solver $sim
}

#
# proc Sims_export2probe {}
#------------------------------------------------------------------------
# just what it says. needs work. baa
#------------------------------------------------------------------------
proc Sims_Export2Probe {sim} {
  if {$sim == ""} {return}
  PROBE ALL $sim
}

proc Sims_do_Export2Probe {} {
  set sim [Libr_Get_CurrentSim]
  if {$sim == ""} {
    return;
  }
  Sims_Export2Probe $sim
}

#
# proc Sims_HandleInstanceMoved {args}
#------------------------------------------------------------------------
# redraw sims, in Case any types have changed
#------------------------------------------------------------------------
proc Sims_HandleInstanceMoved {args} {
  Sims_Redraw
}

#
# proc Sims_HandleTypesDelete {args}
#------------------------------------------------------------------------
# redraw sims, in Case any types have changed
#------------------------------------------------------------------------
proc Sims_HandleTypesDelete {args} {
  global ascLibrVect
  $ascLibrVect(simsBox) select set 0 end
  set sellist [$ascLibrVect(simsBox) curselection]
  if {$sellist==""} {return}
  foreach i $sellist {
    set item [$ascLibrVect(simsBox) get $i]
    if {$item!=""} {
      set sim [lindex [lindex $item 0] 0];
      Sims_Delete $sim
    }
  }
  $ascLibrVect(simsBox) delete 0 end
  Sims_Redraw
}

# call the method generation functions and stuff output in the 
# display window where user can copy/paste to models file.
proc Libr_SuggestMethods {} {
  global ascUtilVect ascDispVect ascLibrVect

  set type "$ascLibrVect(selectedtype)"
  if {$type == ""} { return }

  DispClear;
  set outputfile [FileUniqueName "$ascUtilVect(asctmp)/ascsuggmeth"]
  set fid [open $outputfile w+]
  asc_suggest_methods $fid $type
  close $fid
  # read chunks of 10k
  set fid [open $outputfile r]
  while {![eof $fid]} {
    $ascDispVect(textBox) insert end [read $fid 10000]
  }
  close $fid
  file delete $outputfile
  DispSetEntry "Suggested methods code for $type"
  newraise .display
}

proc Libr_File_do_Copy {} {
  global ascLibrVect
  asc_export_selection $ascLibrVect(entryBox)
  event generate $ascLibrVect(entryBox) <<Copy>>
}
