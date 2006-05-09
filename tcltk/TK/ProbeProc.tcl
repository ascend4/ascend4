#  ProbeProc.tcl: Tcl code for Probe window
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.34 $
#  Last modified on: $Date: 1998/06/18 15:54:52 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: ProbeProc.tcl,v $
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

# temporary debugging containers
proc probe {args} {
  set str "probe "
  append str $args
  append str "\n The probe has been reimplemented completely."
  append str "\n" 'help  __probe' for details"
  Script_Raise_Alert $str
}

#
# proc set_Probe_Defaults {}
#------------------------------------------------------------------------
# startup Probe after windows
#
# ascProbVect is also home to probe window state information.
# in particular file menu data.
# ascProbVect(collection) is the presently viewed buffer. -1 is the
# bogus initialization value.
#
# ascProbVect(maxbufnum) is the highest available buffer num.
#bugs buffile not used properly yet.
# ascProbVect(buffile.$c) is the filename corresponding to buffer $c
# ascProbVect(bufopen.$c) is the closed/open status of the buffer.
#   note we need some file menu reconstruction if close is to be used.
#------------------------------------------------------------------------
proc set_Probe_Defaults {} {
  global ascProbVect ascProbImportVect ascParPageVect
  global ascGlobalVect

 #puts "setting Probe buttons"

  if {![info exists ascProbVect(font)]} {
    set ascProbVect(font) $ascGlobalVect(font)
  }
  set ascProbVect() [pwd]
  set ascProbVect(basefiletypes) {
    {{Names files} {.a4p} }
    {{Most} {.*} }
    {{All} {*} }
  }
  set ascProbVect(filetypes) $ascProbVect(basefiletypes)
  set ascProbVect(windowname) .probe
  set ascProbVect(collection) 0
  set ascProbVect(initialized) FALSE
  #  This is done in ProbeSwitchBuf
  #  set ascProbVect(vbox)   .probe.main_frm.probe_box_1.listbox1
  # buffer management:
  set ascProbVect(listbasename) .probe.main_frm.probe_box_
  set ascProbVect(bufferentry) .probe.buffer_frm.buffer_entry
  set ascProbVect(fileBtn)   .probe.menubar.file
  set ascProbVect(editBtn)   .probe.menubar.edit
  set ascProbVect(expoBtn)    .probe.menubar.export

  set ascProbImportVect(namelist) [list rootname Buffer]
  set filterlist [__probe filters]
  foreach i $filterlist {
    set parts [split $i /]
    set sub [lindex $parts 0]
    lappend ascProbImportVect(namelist) $sub
    lappend ascProbImportVect(filterlist) $sub
    set ascProbImportVect($sub) 0
    set ascProbImportVect($sub.type) bool
    set ascProbImportVect($sub.label) [lindex $parts 1]
  }
  # it's ugly, but we need a default.
  set ascProbImportVect(VisitReals) 1
  set ascProbImportVect(rootname) ""
  set ascProbImportVect(rootname.label) "Exporting from"
  set ascProbImportVect(rootname.type) string
  set ascProbImportVect(Buffer) current
  set ascProbImportVect(Buffer.label) "Probe buffer:"
  set ascProbImportVect(Buffer.type) string

  set ascProbImportVect(grab) 1
  set ascProbImportVect(npages) 1
  set ascProbImportVect(toplevel) .probimport
  set ascProbImportVect(title) "Probe export filters"
  set ascProbImportVect(helpcommand) {Help_button browser.export.many}
  set ascProbImportVect(whenokcommand) ""

  set ascParPageVect(btn_font) $ascProbVect(font)
  set ascParPageVect(lbl_font) $ascProbVect(font)

  set ascProbVect(mainframe) .probe.main_frm
  set ascProbVect(collection) -1
  set ascProbVect(maxbufnum) -1

  Configure_Probe
}


# proc Configure_Probe {}
#------------------------------------------------------------------------
# misc bindings
#------------------------------------------------------------------------
proc Configure_Probe {} {
  global ascProbVect;

  Probe_do_NewBuffer
  ascclearlist $ascProbVect(vbox);

  # Update Enabled/Disabled entries when a menu is posted
  #
  $ascProbVect(editBtn) configure \
    -postcommand Probe_Update_EditButtons

  $ascProbVect(expoBtn) configure \
    -postcommand Probe_Update_ExpButtons

  $ascProbVect(fileBtn) configure \
    -postcommand Probe_Update_FileButtons

  .probe.menubar.view configure \
    -postcommand Probe_Update_View_Buttons

  # set pointer
  .probe configure -cursor left_ptr
}

#
# proc Probe_do_Font {}
#---------------------------------------------------------------------
# font select button for Probe window. updates all listboxes.
#---------------------------------------------------------------------
proc Probe_do_Font {args} {
  global ascProbVect
  set font ""
  if {$args != ""} {
    set font $args
  } else {
    set font  [ascFontGet]
  }
  if {"$font" == ""} {
    return;
  }
  set len $ascProbVect(maxbufnum)
  set ascProbVect(font) $font
  for {set c 0} { $c <= $len} { incr c} {
    $ascProbVect(listbasename)$c.listbox1 configure -font $font
  }
  $ascProbVect(bufferentry) configure -font $font
}

#
# proc Probe_Update_EditButtons {}
#------------------------------------------------------------------------
# dis/enable edit buttons
#------------------------------------------------------------------------
proc Probe_Update_EditButtons {} {
  global ascProbVect
  set m $ascProbVect(editBtn)

  if {[$ascProbVect(vbox) size] == 0} {
    $m entryconfigure 1 -state disabled
    $m entryconfigure 2 -state disabled
  } else {
    $m entryconfigure 1 -state normal
    $m entryconfigure 2 -state normal
  }
}


#
# proc Probe_Update_FileButtons {}
#------------------------------------------------------------------------
# dis/enable File buttons
#------------------------------------------------------------------------
proc Probe_Update_FileButtons {} {
  global ascProbVect
  set m $ascProbVect(fileBtn)

  # New buffer is always available
  $m entryconfigure 0 -state normal

  # Read
  $m entryconfigure 1 -state normal

  # Print
  if {[$ascProbVect(vbox) size] == 0} {
    $m entryconfigure 2 -state disabled
    $m entryconfigure 3 -state disabled
    $m entryconfigure 4 -state disabled
  } {
    $m entryconfigure 2 -state normal
    $m entryconfigure 3 -state normal
    $m entryconfigure 4 -state normal
  }
}

#
# proc Probe_Update_View_Buttons {}
#------------------------------------------------------------------------
# dis/enable View buttons
#------------------------------------------------------------------------
proc Probe_Update_View_Buttons {} {
  global ascProbVect ascGlobalVect

  set mb .probe.menubar.view

 if {$ascGlobalVect(saveoptions) == 0} {
    $mb entryconfigure 2 -state disabled
  } else {
    $mb entryconfigure 2 -state normal
  }

}

#
# proc Probe_Update_ExpButtons {}
#------------------------------------------------------------------------
# dis/enable expo buttons.
# The export to Display menu item can be used as a sort of reporting
# feature, where results from the probe can be embedded into the display.
#------------------------------------------------------------------------
proc Probe_Update_ExpButtons {} {
  global ascProbVect
  set m $ascProbVect(expoBtn)

  if {[$ascProbVect(vbox) size] == 0} {
    $m entryconfigure 0 -state disabled
    $m entryconfigure 1 -state disabled
  } {
    $m entryconfigure 0 -state normal
    $m entryconfigure 1 -state normal
  }
}


#
# proc Probe_Import {collection name args}
#------------------------------------------------------------------------
# Send named item to probe. collection may be
# 'new', 'current' or the number of an existing collection.
# Collections number consecutively from 0 as they are created.
# If args is not empty, it must be a well-formed filter-list for
# __probe add
# This function should be the only hub-notifying and window updating
# functions. All other import functions should be wrappers to this.
#------------------------------------------------------------------------
proc Probe_Import {collection name args} {
  global ascProbVect
# puts "Probe_Import $collection $name $args"
  set logcollection $collection
  if {$collection == "" || $name ==""} {
    return
  }
  if {$collection == "current"} {
    set collection $ascProbVect(collection)
  }
  if {$collection == "new"} {
    set collection [Probe_do_NewBuffer]
  }
  if {$collection <0 || $collection > $ascProbVect(collection)} {
    Script_Raise_Alert \
    "Probe_Import called with bad collection number $collection."
  }
  set commandstring "__probe add $collection "
  append commandstring \{ $name \}
  foreach i [stripbraces $args] {
    append commandstring " $i"
  }
  eval $commandstring
  Probe_Update $collection
  if {$ascProbVect(visibility)} {
    newraise $ascProbVect(windowname);
  }
  HUB_Message_to_HUB INSTPROBED $logcollection $name $args
}

#
# Probe_Import_List
#------------------------------------------------------------------------
# imports a list of complete names
#------------------------------------------------------------------------
proc Probe_Import_List {number list} {
  foreach i $list {
    Probe_Import $number $i
  }
}

#
# proc Probe_Import_Filtered {caller name {automatic 0}}
#------------------------------------------------------------------------
# caller is the name of the toplevel window calling this function.
# Send instances in named subtree to probe.
# if automatic, then no dialog is used to get filter options.
# collections are C structures.
#------------------------------------------------------------------------
proc Probe_Import_Filtered {caller name {automatic 0}} {
  global ascProbVect
  global ascProbImportVect

  set ascProbImportVect(rootname) $name
  set ascProbImportVect(rootname.choices) $ascProbImportVect(rootname)
  set ascProbImportVect(Buffer.choices) "current new"
  set len $ascProbVect(maxbufnum)
  for {set c 0} {$c <= $len} {incr c} {
    append ascProbImportVect(Buffer.choices) " $c"
  }
  set ascProbImportVect(title) "Filtering "
  append ascProbImportVect(title) $ascProbImportVect(rootname)
  if {$automatic == 0} {
    set ascParPageVect(btn_font) $ascProbVect(font)
    set ascParPageVect(lbl_font) $ascProbVect(font)
    ascParPage ascProbImportVect [setpos $caller 0 0] 1
    if {$ascProbImportVect(__,cancelled)} {
      return
    }
  }
  set filter ""
  foreach i $ascProbImportVect(filterlist) {
    lappend filter $ascProbImportVect($i)
  }
  Probe_Import $ascProbImportVect(Buffer) $name $filter
  set ascProbImportVect(Buffer) current
}


#
#---------------------------------------------------------------------------
#buffer management stuff
#---------------------------------------------------------------------------

#
# proc ProbeSwitchToNewBuf {{fname ""}}
#---------------------------------------------------------------------------
# ascProbVect(buffile.$num) the filename of numbered buffer
# ascProbVect(mainframe) the name of the parent for all probe_box widgets
# ascProbVect(collection) the number of the buffer in use currently
#
# switches to the numbered buffer. if the buffer isn't open,
# creates it. If fname is given != "" on a closed buffer,
# will read the buffer from disk, eventually...
# as we can see, this function needs to be decomposed.
#---------------------------------------------------------------------------
proc ProbeSwitchToNewBuf {{fname ""}} {
  global ascProbVect ascGlobalVect

  set num [__probe expand]
  set ascProbVect(maxbufnum) $num
  set filename "foobar"
  set parentname $ascProbVect(mainframe)
  if {$fname == ""} {
    set filename NoName$num.a4p
    set ascProbVect(buffile.$num) $filename
    set ascProbVect(bufopen.$num) 0
  } else {
    set filename $fname
  }
  set oldnum $ascProbVect(collection)
  set oldbufname .probe_box_$oldnum
  catch {pack forget $parentname$oldbufname}
  set ascProbVect(collection) $num
  set ascProbVect(filename) $filename
  set ascProbVect(vbox) $parentname.probe_box_$num.listbox1

  set winlist [build_probebox $parentname $num $ascProbVect(font)]
  Probe_bindListbox $winlist.listbox1

  # pack widget $parentname
  pack append $parentname \
    $parentname.probe_box_$num {top frame center expand fill}

  $ascProbVect(fileBtn) add command  \
     -command "ProbeSwitchToOldBuf $num $filename" \
     -label $filename
  set ascProbVect(bufopen.$num) 1
  $parentname.probe_box_$num.listbox1 insert end {}

  set ascProbVect(maxbufnum) $num
  update
  update idletasks
}


proc ProbeSwitchToOldBuf {num fname} {
  global ascProbVect

  set parentname $ascProbVect(mainframe)
  set oldnum $ascProbVect(collection)
  set oldbufname .probe_box_$oldnum
  catch {pack forget $parentname$oldbufname}
  set ascProbVect(collection) $num
  set ascProbVect(filename) $fname
  set ascProbVect(vbox) $parentname.probe_box_$num.listbox1
  pack append $parentname \
    $parentname.probe_box_$num {top frame center expand fill}
  Probe_Update $num
  update
  update idletasks
}


# proc Probe_do_NewBuffer
#------------------------------------------------------------------------
#  open a new buffer named NoNameN.a4p
#  where N is the current max buffer number
#------------------------------------------------------------------------
proc Probe_do_NewBuffer {} {
  global ascProbVect
  set num [__probe size]
  ProbeSwitchToNewBuf
  return  $num
}

#
# proc Probe_load_file {file bufnumber}
#------------------------------------------------------------------------
# collection names read from file to collection number given
# collection must already exist. returns the number of errors.
#------------------------------------------------------------------------
proc Probe_load_file {filename bufnum} {
  global ascProbVect
  if {$ascProbVect(maxbufnum) < $bufnum} {
    Script_Raise_Alert "Cannot read into nonexistent probe buffer $bufnum"
    return 1
  }
  set fname [file nativename $filename]
  if {[file isfile $fname] && [file readable $fname]} { } else {
    set err "Cannot read probe names file "
    append err $fname
    Script_Raise_Alert $err
    return 1
  }
  set ec 0
  if {[catch {
               set cid [open $fname r]
               set input [read -nonewline $cid]
               close $cid
               foreach i $input {
                 if {$i != "" && [string index $i 0] != "#"} {
                   if {[catch {__probe add $bufnum $i} err]} {
                     incr ec
                   }
                 }
               }
             } msg]} {
    Script_Raise_Alert "Error reading values file"
    return 1;
  }
  catch {__probe update $bufnum}
  set ascProbVect(filename) $fname
  return $ec
}

#
# proc Probe_do_Read {}
#------------------------------------------------------------------------
# move names from file to current collection
#------------------------------------------------------------------------
proc Probe_do_ReadFile {} {
  global ascProbVect asc_tkfbox ascGlobalVect
  set defaultname [file dirname $ascProbVect(filename)]
  set asc_tkfbox(otherdirs) $ascGlobalVect(librarypathdirs)
  set filename [tk_getOpenFile \
    -defaultextension "" \
    -filetypes $ascProbVect(filetypes) \
    -initialdir $defaultname \
    -parent $ascProbVect(windowname)\
    -title {Read saved names file}]

  if {$filename == "" && $asc_tkfbox(cancelled)==0} {
    set err "Cannot read \"\""
    Script_Raise_Alert $err
    return
  }
  if {$asc_tkfbox(cancelled)} {
    return
  }
  if {[file isdirectory $filename]} {
    set err "Cannot read names from directory "
    append err $filename
    Script_Raise_Alert $err
    return
  }
  Probe_load_file $filename $ascProbVect(collection)
  Probe_Update $ascProbVect(collection)
}

#
# proc Probe_write {col file}
#------------------------------------------------------------------------
# dump specified collection names to file named.
#------------------------------------------------------------------------
proc Probe_write {col file} {
  set cid [open $file w+]
  puts -nonewline $cid "#$col\{"
  puts -nonewline $cid $file
  puts $cid "\}"
  set max [__probe size $col]
  for {set i 0} {$i < $max} {incr i} {
    puts -nonewline "$col $i "
    puts [__probe name $col $i]
    puts -nonewline $cid  "\{"
    puts -nonewline $cid [__probe name $col $i]
    puts $cid "\}"
  }
  close $cid
  puts -nonewline stdout "Wrote "
  puts stdout $file
}

#
# proc Probe_do_WriteBuf {}
#------------------------------------------------------------------------
# dump current collection names to associated file.
#------------------------------------------------------------------------
proc Probe_do_WriteBuf {} {
  global ascProbVect
  set col $ascProbVect(collection)
  if {[__probe size $col]} {
    if {[file exists $ascProbVect(buffile.$col)]} {
      if {[file writable  $ascProbVect(buffile.$col)]} {
        # fsbox takes care of overwrite query
        Probe_write $col $ascProbVect(buffile.$col)
      } else {
        set mess "File "
        append mess $ascProbVect(buffile.$col)
        append mess " is not writable"
        Script_Raise_Alert $mess FYI
      }
    } else {
      Probe_write $col $ascProbVect(buffile.$col)
    }
  } else {
    Script_Raise_Alert "Empty probe $col cannot be saved" "FYI"
  }
}

#
# proc Probe_do_WriteBufAs {}
#------------------------------------------------------------------------
# dump current collection names to user specified file
#------------------------------------------------------------------------
proc Probe_do_WriteBufAs {} {
  global ascProbVect asc_tkfbox
  set col $ascProbVect(collection)
  if {[__probe size $col]} {
    set defaultname $ascProbVect(filename)
    set filename [tk_getSaveFile \
      -defaultextension "" \
      -filetypes $ascProbVect(filetypes) \
      -initialfile $defaultname \
      -parent .probe \
      -title {Save probe names AS}]
    if {$filename == "" && !$asc_tkfbox(cancelled)} {
      Script_Raise_Alert "No file name given. Not saved" "FYI"
    }
    set fname [file nativename $filename]
    if {[file exists $fname]} {
      if {[file writable $fname]} {
        # fsbox takes care of overwrite query
        Probe_write $col $fname
      } else {
        set mess "File "
        append mess $fname
        append mess " is not writable"
        Script_Raise_Alert $mess FYI
      }
    } else {
      Probe_write $col $fname
    }
  } else {
    Script_Raise_Alert "Empty probe $col cannot be saved" "FYI"
  }
}

#
# proc Probe_do_Print {}
#------------------------------------------------------------------------
# dump collection view to file
#------------------------------------------------------------------------
proc Probe_do_Print {} {
  global ascProbVect
  $ascProbVect(vbox) selection clear 0 end
  Print_configure ascProbVect(windowname)
  if {[Print_cancelcheck]} {
    return
  }
  DispPrint [DispWriteSelection $ascProbVect(vbox)]
  HUB_Message_to_HUB WINDOWPRINTED PROBE
}

#
# proc Probe_Get_Selection {collection}
#------------------------------------------------------------------------
# returns current probe selection
#------------------------------------------------------------------------
proc Probe_Get_Selection {collection} {
  global ascProbVect;
  set sel_list [$ascProbVect(vbox) curselection]
  return $sel_list;
}

#
# proc Probe_Update {collection}
#------------------------------------------------------------------------
# stuff probe window of the collection given
#------------------------------------------------------------------------
proc Probe_Update {collection} {
  global ascProbVect
  ascclearlist $ascProbVect(vbox)
  foreach item [__probe get $collection] {
    $ascProbVect(vbox) insert end $item
  }
}

# proc Probe_Remove_Pattern
#------------------------------------------------------------------------
# This function will take a pattern and remove all things that
# match the pattern from the probe. This might be used to
# say remove : everything that matches T*, or lower_bound etc.
# It accepts a list of indices, eliminates what does not match
# and returns the modified list to be used by Probe_Delete.
# Assumes that we DONT have disjoint listbox selections.
# Remember that we always count from 1 up on the C-side of
# things.
#------------------------------------------------------------------------
proc Probe_Remove_Pattern {list pattern} {
  Script_Raise_Alert "Probe_Remove_Pattern not implemented"
}

#
# proc Probe_do_SelectAll
#------------------------------------------------------------------------
# function to select all in probe
#------------------------------------------------------------------------
proc Probe_do_SelectAll {} {
  global ascProbVect
  $ascProbVect(vbox) select set 0 end
}


#
# proc Probe_do_RemoveSelections
#------------------------------------------------------------------------
# removes the selected item in the current collection.
# selection may be disjoint.
#------------------------------------------------------------------------
proc Probe_do_RemoveSelections {} {# The command bound to the MenuButton

  global ascProbVect
  set collection $ascProbVect(collection)
  set delete_list "[Probe_Get_Selection $collection]"
  if {$delete_list != ""} {
    set pccommand "__probe clear $collection $delete_list"
    eval $pccommand
    Probe_Update $collection
  }
}

#
# proc Probe_do_RemoveAll
#------------------------------------------------------------------------
# delete all probe stuff in current buffer.
#------------------------------------------------------------------------
proc Probe_do_RemoveAll {} {
  global ascProbVect
  set collection $ascProbVect(collection)
  __probe clear $collection
  Probe_Update $collection
}

#
# proc Probe_do_RemoveUncertain
#------------------------------------------------------------------------
# delete all uncertain stuff in current buffer.
#------------------------------------------------------------------------
proc Probe_do_RemoveUncertain {} {
  global ascProbVect
  set collection $ascProbVect(collection)
  __probe trash
  Probe_Update $collection
}

#
# proc Probe_do_Copy
#------------------------------------------------------------------------
# Copy current probe to clipboard
#------------------------------------------------------------------------
proc Probe_do_Copy {} {
  global ascProbVect
  asc_export_selection $ascProbVect(vbox)
  event generate $ascProbVect(vbox) <<Copy>>
}


#
# proc Probe_HandleInstanceMoved
#------------------------------------------------------------------------
# This procedure will be registered with the HUB.
# Whenever an instance is ABOUT to moved in memory, such as with a merge,
# refine or are_alike, This procedure will be invoked first so that
# ALL instances become uncertain in all probes.
#------------------------------------------------------------------------
proc Probe_HandleInstanceMoved {args} {
  global ascProbVect
  __probe invalidate
  Probe_Update $ascProbVect(collection)
}

#
# proc Probe_HandleNewInstances
#------------------------------------------------------------------------
# This procedure will be registered with the HUB.
# Whenever new instances are in memory, such as after a merge,
# refine or are_alike, This procedure will be invoked first so that
# uncertain instances become defined again.
#------------------------------------------------------------------------
proc Probe_HandleNewInstances {args} {
  global ascProbVect
  __probe update
  Probe_Update $ascProbVect(collection)
}

#
# proc Probe_HandleSimsDelete
#------------------------------------------------------------------------
# This procedure will be registered with the HUB.
# It removes all references i.e in all collections, for a simulation that is
# ABOUT to be deleted.
#------------------------------------------------------------------------
proc Probe_HandleSimsDelete {{list ""}} {
  Probe_HandleInstanceMoved $list
}

#
# proc Probe_HandleVariableUpdated
#------------------------------------------------------------------------
# This procedure will be registered with the HUB.
# Whenever a variables value has changed because of:
# 1) solving completed.
# 2) a procedure has been run.
# 3) a variable has been assigned etc...
#   this procedure will be invoked to update only the currently focused
# probe. The act of selecting a new probe collection via the radio buttons
# will update that collection at the given time.
#------------------------------------------------------------------------
proc Probe_HandleVariableUpdated {{list ""}} {
  global ascProbVect
  Probe_Update $ascProbVect(collection)
}

#
# proc Probe_do_Export2Browser {}
#------------------------------------------------------------------------
# export first of current selection in probe to browser.
# The browser is expected to defend itself against UNCERTAIN names.
#------------------------------------------------------------------------
proc Probe_do_Export2Browser {} {
  global ascProbVect
  set collection $ascProbVect(collection)
  set ndx_list [Probe_Get_Selection $collection]
  set ndx [lindex $ndx_list 0]
  if {$ndx == ""} {
    return;
  }
  set name [__probe name $collection $ndx]
  if {$name == ""} {return}
  Brow_Export_Any_2Browser $name
}

#
# proc Probe_do_Export2Display {}
#------------------------------------------------------------------------
# button.  wrapper of export to display
# Will take each item in the probe, which is a valid tcl list,
# split of any braces and insert them one a time in the display
# window at the current insertion cursor. The information that will
# be exported will be the displayed information in the probe, and not
# pulled up from the internal C data_structure.
#------------------------------------------------------------------------
proc Probe_do_Export2Display {} {# bound to the menu_button
  global ascProbVect ascDispVect

  if {[winfo exists $ascDispVect(textBox)] == "0"} {return}
  set collection $ascProbVect(collection)
  set index_list [Probe_Get_Selection $collection]
  foreach index $index_list {
    set data [$ascProbVect(vbox) get $index]
    $ascDispVect(textBox) insert insert $data
    $ascDispVect(textBox) insert insert "\n"
  }
  newraise .display
}

#
# proc Probe_do_Help {}
# proc Probe_do_BindHelp {}
#------------------------------------------------------------------------
# probe help buttons
#------------------------------------------------------------------------
proc Probe_do_Help {} {
  Help_button probe
}
proc Probe_do_BindHelp {} {
  Help_button probe.help onprobe
}
#
# proc Probe_Redraw {}
#------------------------------------------------------------------------
# Redraw probe after interface restart
#------------------------------------------------------------------------
proc Probe_Redraw {} {
 # data seems to linger across restarts, so no redraw
}

#
# proc Probe_bindListbox {probelistbox}
#------------------------------------------------------------------------
# bind probe after creation
#------------------------------------------------------------------------
proc Probe_bindListbox {w} {
  bind $w <Button-3> {
    set d [%W nearest %y]
    if {$d != ""} {
      Probe_do_SetValue $d
    }
  }
  bind $w <Double-1> {
    set d [%W nearest %y]
    if {$d != ""} {
      Probe_do_BrowseItem $d
    }
  }
}

#
# proc Probe_do_SetValue {ndx}
#----------------------------------------------------------------------
# pull up an assignment dialog. this hsould maybe pull up a parameter
# style page for assigning value and all assignable children.
#----------------------------------------------------------------------
proc Probe_do_SetValue {ndx} {
  global ascProbVect
  set name [lindex [$ascProbVect(vbox) get $ndx] 0]
  if {[catch {qlfdid $name} errmsg]} {return}
  Browser_do_SetValue $ascProbVect(vbox) $name
}

#
# proc Probe_do_BrowseItem {ndx}
#----------------------------------------------------------------------
# browse the selected item at line ndx in the current probe.
#----------------------------------------------------------------------
proc Probe_do_BrowseItem {ndx} {
  global ascProbVect
  set name [lindex [$ascProbVect(vbox) get $ndx] 0]
  if {[catch {qlfdid $name} errmsg]} {return}
  BROWSE $name
}

#------------------------------------------------------------------------
#------------------------------------------------------------------------
#
# User Data functions. They will be kept here until we can find a
# place for them to reside. These are most closely related to the
# probe, but only by a vague similarity.
#
#------------------------------------------------------------------------
#------------------------------------------------------------------------

#
# proc WRITE_VIRTUAL
#------------------------------------------------------------------------
# Saves the information from the specified qualified id into a virtual
# file with the given name. At the moment now only saves the information
# associated with reals.
#------------------------------------------------------------------------
proc WRITE_VIRTUAL {from to} {
  # first set up the search instance to look at the "from" instance.
  #
  set nok [catch "qlfdid \{$from\}" data_or_error]
  if {$nok} {
    Script_Raise_Alert "$data_or_error" "Probe Error"
    return 1;
  }
  # attempt to save the data.
  #
  set nok [catch "__userdata_save search $to" data_or_error]
  if {$nok} {
    Script_Raise_Alert "$data_or_error" "Probe Error"
    return 1;
  }
  return 0;
}

#
# proc READ_VIRTUAL
#------------------------------------------------------------------------
# Will restore the values from the virtual file back to the instance
# tree. See the notes concerning writing of this virtual file.
#------------------------------------------------------------------------
proc READ_VIRTUAL {id} {
  set nok [catch "__userdata_restore $id" data_or_error]
  if {$nok} {
    Script_Raise_Alert "$data_or_error" "Probe Error"
    return 1;
  }
  HUB_Message_to_HUB VARIABLEUPDATED
  return 0;
}

#
# proc Probe_UserData_HandleSimsDelete
#------------------------------------------------------------------------
# This function will be registered with the hub. It needs to be invoked
# under the same conditions that will require a flush of the probe.
#------------------------------------------------------------------------
proc  Probe_UserData_HandleSimsDelete {args} {
  __userdata_destroy all
  __userdata_init
  return 0
}

#
# proc Probe_UserData_HandleInstanceMoved
#------------------------------------------------------------------------
# This function will be registered with the hub. It needs to be invoked
# under the same conditions that will require a flush of the probe.
#------------------------------------------------------------------------
proc Probe_UserData_HandleInstanceMoved {args} {
  Probe_UserData_HandleSimsDelete $args
  return 0
}
