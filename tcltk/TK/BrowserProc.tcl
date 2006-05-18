#  BrowserProc.tcl: support Tcl code for the Browser
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.99 $
#  Last modified on: $Date: 1998/07/06 10:28:10 $
#  Last modified by: $Author: ballan $
#  Revision control file: $RCSfile: BrowserProc.tcl,v $
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
# proc qassgn {qid val {units ""}}
#------------------------------------------------------------------------
# holdover patch for old scripts.
# do not use this function in new scripts, use qassgn3 instead.
#------------------------------------------------------------------------
proc qassgn {qid val {units ""}} {
  return [qassqn3 $qid $val $units]
}

#
# proc set_Browser_Defaults {}
#------------------------------------------------------------------------
# set browser on startup
#------------------------------------------------------------------------
proc set_Browser_Defaults {} {
  global ascBrowVect ascGlobalVect xfShowWindow.browser

  if {${xfShowWindow.browser}} {
    # puts "setting browser buttons"
    set ascBrowVect(windowname) .browser
    set ascBrowVect(filename) [pwd]
    set ascBrowVect(basefiletypes) {
      {{New values} {.a4v} }
      {{Old values} {.values} }
      {{Most} {.*} }
      {{All} {*} }
    }
    set ascBrowVect(filetypes)  $ascBrowVect(basefiletypes)
    set ascBrowVect(currentsim) ""
    set ascBrowVect(depth) ""
    set ascBrowVect(qlfdid) $ascBrowVect(currentsim)
    set ascBrowVect(procedurelist) ""

    if {![info exists ascBrowVect(dimconsistency)]} {
      set ascBrowVect(dimconsistency) 1
    }
    if {![info exists ascBrowVect(lastreadextension)]} {
      set ascBrowVect(lastreadextension) ".a4v"
    }
    if {![info exists ascBrowVect(ShowAtoms)]} {
      set ascBrowVect(ShowAtoms) "" ;# don't show by default
    }
    if {![info exists ascBrowVect(HidePassed)]} {
      set ascBrowVect(HidePassed) "" ;# don't show by default
    }
    if {![info exists ascBrowVect(localkinds)]} {
      set ascBrowVect(localkinds) REAL_ATOM_INST
    }
    set ascBrowVect(locallist) [list \
      REAL_ATOM_INST \
      INTEGER_ATOM_INST \
      BOOLEAN_ATOM_INST  \
      SYMBOL_ATOM_INST \
      SET_ATOM_INST \
      REL_INST  \
      LREL_INST  \
      REAL_CONSTANT_INST  \
      INTEGER_CONSTANT_INST \
      BOOLEAN_CONSTANT_INST \
      SYMBOL_CONSTANT_INST \
    ]
    foreach i $ascBrowVect(locallist) {
      if {![info exists ascBrowVect(localshow,$i)]} {
        set ascBrowVect(localshow,$i) 0
      }
    }
    # here we establish the traces which keep the vars in sync
    # with the lead inst kind of their group, as grouped on the
    # check buttons in the browser
    trace variable ascBrowVect(localshow,SET_ATOM_INST) w Brow_LeadCheckBtn
    trace variable ascBrowVect(localshow,INTEGER_ATOM_INST) w Brow_LeadCheckBtn

    if {![info exists ascBrowVect(localshow,REAL_ATOM_INST)]} {
      set ascBrowVect(localshow,REAL_ATOM_INST) 1
    } else {
      set ascBrowVect(localshow,REAL_ATOM_INST) \
        $ascBrowVect(localshow,REAL_ATOM_INST) ;# tweak the trace
    }
    Brow_UpdateLocalKinds
    set ascBrowVect(initialized) "FALSE"

    set ascBrowVect(parents) .browser.main_frm.parents_box.listbox1
    set ascBrowVect(children) .browser.main_frm.child_box.listbox1
    set ascBrowVect(local) .browser.local_frm.child_box.listbox1
    set ascBrowVect(locallabel) nothing

    $ascBrowVect(parents) configure \
      -exportselection {0}

    bind $ascBrowVect(windowname) <F5> Browser_do_ResumeCompile

    bind $ascBrowVect(parents) <1> {
      set d [%W nearest %y]
      if {$d != ""} {
        if {$d == [%W size] - 1} {
          # no change of focus, just do something else
          Brow_do_SameParentsBox $d
        } else {
          Brow_do_ParentsBox $d
        }
      }
    }

#   bind $ascBrowVect(parents) <3> {# Parentsbox
#     set d [%W nearest %y]
#     if {$d != "" } {
#       Brow_do_ParentsSetValue $d
#     }
#   }

    ascRightMouseAddCommand $ascBrowVect(parents) BrowUpdateFindCascade \
      cascade -label "Find" \
      -underline -1 -menu $ascBrowVect(parents).childpop.mfind

    ascRightMouseAddCommand $ascBrowVect(parents) BrowUpdateRunCascade \
      cascade -label "Run" \
      -underline -1 -menu $ascBrowVect(parents).childpop.mrun

    ascRightMouseAddCommand $ascBrowVect(parents) BrowSetValueState \
      command -label "Set value" \
      -underline -1 -command Browser_do_SetValue

    ascRightMouseAddCommand $ascBrowVect(parents) BrowSetAttributeState \
      command -label "Set attribute values" \
      -underline -1 -command Brow_do_ParentsSetValue

    ascRightMouseAddCommand $ascBrowVect(parents) BrowShowCodeState \
      command -label "Show code" \
      -underline -1 -command {Disp_do_ShowCode [inst type]}

    ascRightMouseAddCommand $ascBrowVect(parents) BrowSolveState \
      command -label "Solve..." \
      -underline -1 -command Brow_do_Export2Solver

    ascRightMouseAddCommand $ascBrowVect(parents) normal \
      command -label "Probe..." \
      -underline -1 -command Brow_do_Export2Probe

    ascRightMouseAddCommand $ascBrowVect(parents) normal \
      separator

    ascRightMouseAddCommand $ascBrowVect(parents) normal \
      command -command {Toggle_Remote ascBrowVect} \
      -label {Close window} -underline -1


    #  remove global ListBox bindings from the ChildBox so we
    #  cannot select objects in the ChildBox
    #  default bindtags is $ascBrowVect(children) Listbox .browser all
    set bind_tags [bindtags $ascBrowVect(children)]
    if {[set findit [lsearch $bind_tags Listbox]] != -1} {
      bindtags $ascBrowVect(children) [lreplace $bind_tags $findit $findit]
    }

    bind $ascBrowVect(children) <1> {
      # browse down
      set ndx [%W nearest %y]
      if {[%W size] != "0"} {
	Brow_do_ChildBox $ndx
      }
    }

    bind $ascBrowVect(children) <Double-2> {
      set ndx [%W nearest %y]
      if {[%W size] != "0"} {
        Brow_do_BooleanToggle $ndx
      }
    }

    bind $ascBrowVect(children) <3> {
      # set value
      set ndx [%W nearest %y]
      if {[%W size] != "0"} {
        Brow_do_ChildSetValue $ndx
      }
    }

    bind $ascBrowVect(local) <1> { # export to child box
      set ndx [%W nearest %y]
      if {[%W size] != "0"} {
        Brow_do_LocalBrowse $ndx
      }
    }

    bind $ascBrowVect(local) <3> { # set value, if sane
      set ndx [%W nearest %y]
      if {[%W size] != 0} {
        Brow_do_LocalSetValue $ndx
      }
    }

    # set up the multiple setvalue box
    Brow_InitSetvalue

    # Update Enabled/Disabled entries when a menu is posted
    #
    .browser.menubar.file configure \
      -postcommand Brow_Update_File_Buttons

    .browser.menubar.display configure \
      -postcommand Brow_Update_Display_Buttons

    .browser.menubar.edit configure \
      -postcommand Brow_Update_Edit_Buttons

    .browser.menubar.export configure \
      -postcommand Brow_Update_Export_Buttons

    .browser.menubar.find configure \
      -postcommand Brow_Update_Find_Buttons

    .browser.menubar.view configure \
      -postcommand Brow_Update_View_Buttons


    # Set Initialized flag
    #
    set ascBrowVect(initialized) "FALSE"

    # bind "^C"
    bind .browser <Control-Key-C> "Tool_exit"

    VPane-Bind .browser.main_frm parents_box child_box 10 0.333
    HPane-Bind .browser main_frm local_frm 10 0.5

    # set pointer
    .browser config -cursor left_ptr

  # setup trace on dim consistency checking.
    if {[trace vinfo ascBrowVect(dimconsistency)]==""} {
      trace variable ascBrowVect(dimconsistency) w Brow_SetDimNoise
    }
  } else {
    puts "browser doesn't exist! buttons not set"
  }
}
#update disabling on find popup
proc BrowUpdateFindCascade {} {
  global ascBrowVect ascPopdata tk_version
    set m $ascBrowVect(parents).childpop.mfind
  if {![winfo exists $m]} {
    menu $m \
      -tearoffcommand [string toupper $m] \
      -tearoff 0
    $m add command \
      -command {Brow_do_FindFixed TRUE} \
      -label {Fixed variables} \
      -underline -1
    $m add command \
      -command {Brow_do_FindOpers} \
      -label {Operands} \
      -underline -1
    $m add command \
      -command {Brow_do_FindFixed FALSE} \
      -label {Free variables} \
      -underline -1
    $m add command \
      -command {Brow_do_FindEligible} \
      -label {Eligible variables} \
      -underline -1
    $m add command \
      -command {Brow_do_FindActive} \
      -label {Active variables} \
      -underline -1
    $m add command \
      -command {Brow_do_FindUndefined} \
      -label {Undefined values} \
      -underline -1
    # bindings don't stick if you leave out this update.
    update
    switch $tk_version {
    8.4 -
    8.5 {
      bind $m <Any-Leave> "+
      set ascPopdata($ascBrowVect(parents).childpop.in) 0
      set ascPopdata($ascBrowVect(parents).childpop.id) \
        \[after \$ascPopdata(delay) \{if \{!\$ascPopdata($ascBrowVect(parents).childpop.in)\} \
             \{ tk::MenuUnpost $ascBrowVect(parents).childpop \} \}\]
    "
	}
    default {
      bind $m <Any-Leave> "+
      set ascPopdata($ascBrowVect(parents).childpop.in) 0
      set ascPopdata($ascBrowVect(parents).childpop.id) \
        \[after \$ascPopdata(delay) \{if \{!\$ascPopdata($ascBrowVect(parents).childpop.in)\} \
             \{ tkMenuUnpost $ascBrowVect(parents).childpop \} \}\]
    "
      }
    }
    bind $m <Any-Enter> "+
      set ascPopdata($ascBrowVect(parents).childpop.in) 1
      catch \{after cancel \$ascPopdata($ascBrowVect(parents).childpop.id)\}
    "
  }
  menu_disable_all $m
  if {[$ascBrowVect(parents) size] != 0} {
    $m entryconfigure 0 -state normal
    $m entryconfigure 5 -state normal
  }
  switch [inst kind] {
  WHEN_INST -
  REL_INST -
  LREL_INST {
      $m entryconfigure 1 -state normal
    }
  MODEL_INST -
  ARRAY_INT_INST -
  ARRAY_ENUM_INST {
      $m entryconfigure 2 -state normal
      $m entryconfigure 3 -state normal
    }
  default { }
  }
  
  return normal
}
#update methods menu
proc BrowUpdateRunCascade {} {
  global ascBrowVect ascPopdata
  set m $ascBrowVect(parents).childpop.mrun
  if {[winfo exists $m]} {
    catch {destroy $m}
  }
  set mlist ""
  if {[string compare [inst kind] "MODEL_INST"] == 0} {
    set itype [inst type]
    if {![llength [libr_query -methods -type $itype]] && \
        ![llength [libr_query -basemethods]]} {
      return disabled
    }
    set mlist [libr_query -methods -type $itype]
    set blist [libr_query -basemethods]
    foreach i $blist {
      if {[lsearch -exact $mlist $i] == -1} {
        lappend mlist $i
      }
    }
    set nlist [lsort $mlist]
    set mlist $nlist
  } else {
    return disabled
  }
  set root [Brow_get_subname]
  menu $m \
    -tearoffcommand [string toupper $m] \
    -tearoff 0
  foreach i $mlist {
    $m add command \
      -command "RUN $root.$i" \
      -label $i \
      -underline -1
  }
  update
  bind $m <Any-Leave> "+
    set ascPopdata($ascBrowVect(parents).childpop.in) 0
    set ascPopdata($ascBrowVect(parents).childpop.id) \
      \[after \$ascPopdata(delay) \{if \{!\$ascPopdata($ascBrowVect(parents).childpop.in)\} \
           \{ tkMenuUnpost $ascBrowVect(parents).childpop \} \}\]
  "
  bind $m <Any-Enter> "+
    set ascPopdata($ascBrowVect(parents).childpop.in) 1
    catch \{after cancel \$ascPopdata($ascBrowVect(parents).childpop.id)\}
  "
  return normal
}
#
# proc Brow_trans_option {opt}
#-----------------------------------------------------------------------
# Brow_trans_option: translate Xname into vector name for the options
# and return subscript, vector id in a list
# If option unrecognized, return same name and Brow
# Note: a useless function unless the browser resources are standardized
# like everyone elses
#-----------------------------------------------------------------------
proc Brow_trans_option {opt} {
  switch $opt {
   {displayAtomValue} {return {TypeorValue Brow}}
   {queryFile} {return {queryfile Brow}}
   default {return "$opt Brow"}
  }
}

#
# proc Brow_do_Font {args}
#---------------------------------------------------------------------
# font select button for browser window
#---------------------------------------------------------------------
proc Brow_do_Font {args} {
  global ascBrowVect
  set font  ""
  if {$args !=""} {
    set font $args
  } else {
    set font  [ascFontGet]
  }
  if {"$font" == ""} {
    return
  }
  $ascBrowVect(children) configure -font $font
  $ascBrowVect(parents) configure -font $font
  $ascBrowVect(local) configure -font $font
  set ascBrowVect(font) [$ascBrowVect(children) cget -font]
}


#
# proc Brow_InitBrowser {}
#-------------------------------------------------------------------------
# empty browserboxes and set currentsim/child/parent null
#-------------------------------------------------------------------------
proc Brow_InitBrowser {} {
  global ascBrowVect

  rootinit;
  ascclearlist $ascBrowVect(parents);
  ascclearlist $ascBrowVect(children);
  ascclearlist $ascBrowVect(local);
}

#
# proc BrowMakeName
#-------------------------------------------------------------------------
# return qlfdid of child
#-------------------------------------------------------------------------
proc BrowMakeName {} {
  global ascBrowVect

  set name [Brow_get_subname]
  set ascBrowVect(qlfdid) $name
  set ascBrowVect(instkind) [inst kind]
 #  puts "-->$ascBrowVect(qlfdid) -- [inst kind] -- [inst nchild]<--"
  return $ascBrowVect(qlfdid)
}

#
# proc Brow_do_ChildBox {ndx}
#-------------------------------------------------------------------------
# Gets the child selected in the child box at index ndx. Strips the \{ and
# \} and sets the 0th element as the potential new parent.
# Calls the registerd command "root $newparent". The "root" command will
# for the instance name and adjust the depth if found. If all is ok the
# newparent is inserted in the parent listbox.
# Brow_do_UpdateChild is then used to update the child box. Set the new
# selection to the newparent.
#-------------------------------------------------------------------------
proc Brow_do_ChildBox {ndx} {
  global ascBrowVect

  set a [$ascBrowVect(children) get $ndx]
  set newparent [stripbraces $a]
  switch [inst kind] {
  ERROR_INST -
  REAL_ATOM_INST -
  INTEGER_ATOM_INST -
  BOOLEAN_ATOM_INST -
  SYMBOL_ATOM_INST -
  SET_ATOM_INST -
  REL_INST -
  LREL_INST {return; #disable descent into ATOM children}
  }
  set aftername [string last " IS_A " $newparent]
  if {$aftername == -1} {
    set aftername [string last " = " $newparent]
  }
  if {$aftername >= 0 } {
    set prettyP [string range $newparent 0 $aftername]
    set prettyP [string trim $prettyP]
    set candidate [string trim $prettyP "\[\'\]"]
    set nok [catch {root $candidate} errmsg]
    if {$nok} {
      if {$errmsg == "At leaves of the Instance Tree"} {
	puts stderr "--> $errmsg"
      }
      $ascBrowVect(children) delete 0 end
      return
    } else {
      $ascBrowVect(parents) insert end $prettyP
      Brow_do_UpdateChild \
        $ascBrowVect(TypeorValue) \
        $ascBrowVect(ShowAtoms) \
        $ascBrowVect(HidePassed)
    }
    Brow_Setup_Selection $ascBrowVect(parents)
    return;
  }
}

#
# Brow_get_partname {listindex}
#-------------------------------------------------------------------------
# returns the name up to element ndx (which counts from 0) from
# the browser parent box, less any trailing non-MODEL names.
# if listindex is end, returns complete less any trailing array/ATOM names.
#-------------------------------------------------------------------------
proc Brow_get_partname {{ndx -1}} {
  global ascBrowVect
  if {$ndx < 0 || "$ndx" == "end"} {
    set ndx [$ascBrowVect(parents) size]
  }
  set name "[$ascBrowVect(parents) get 0 0]"
  set lastpart $name
  if {[$ascBrowVect(parents) size] > 1} {
    foreach i [$ascBrowVect(parents) get 1 $ndx] {
      if { [string index $i 0] != "\["} {
        append name .$i
      } else {
        append name $i
      }
      qlfdid $name 
      if {![string compare [inst kind search] "MODEL_INST"]} {
        set lastpart $name
      }
    }
  }
  return $lastpart
}

#
# Brow_get_subname {listindex}
#-------------------------------------------------------------------------
# returns the name up to element ndx (which counts from 0) from
# the browser parent box.
# if listindex is end, returns complete name.
#-------------------------------------------------------------------------
proc Brow_get_subname {{ndx -1}} {
  global ascBrowVect
  if {$ndx < 0 || "$ndx" == "end"} {
    set ndx [$ascBrowVect(parents) size]
  }
  set name "[$ascBrowVect(parents) get 0 0]"
  if {[$ascBrowVect(parents) size] > 1} {
    foreach i [$ascBrowVect(parents) get 1 $ndx] {
      if { [string index $i 0] != "\["} {
        append name .$i
      } else {
        append name $i
      }
    }
  }
  return $name
}

#
#  proc Brow_do_SameParentsBox {ndx}
#-------------------------------------------------------------------------
# Does things that we want when not changing the focus.
# Creates a popup menu for:
# running methods, what else?
#-------------------------------------------------------------------------
proc Brow_do_SameParentsBox {ndx} {
  # puts we should put a menu here.
}
#
#  proc Brow_do_ParentsBox {ndx}
#-------------------------------------------------------------------------
#  The main binding for ascending the Browser Parents Box. 
#  Sets the current working
#  instance based on the listbox index. Updates the child box. Sets the
#  current selection. A valid listbox index must be sent to this function.
#  i.e. A valid instance must exist and must have been selected.
#-------------------------------------------------------------------------
proc Brow_do_ParentsBox {ndx} {
  global ascBrowVect ascScripVect
  set depth $ndx
  incr depth
  set muffle $ascScripVect(executing)
  if {!$muffle} {
    set ascScripVect(executing) 1
  }
  if {[$ascBrowVect(parents) size] > 1} {
    set name [Brow_get_subname $ndx]
    Brow_Export_Any_2Browser $name
  }
  if {!$muffle} {
    set ascScripVect(executing) 0
  }
}

# left side browser
proc Brow_do_ParentsSetValue {{ndx 0}} {
  global ascBrowVect
  set name [Brow_get_subname]
  puts $name
  if {[catch {qlfdid $name} errmsg]} {return}
  Browser_SetvalueBox $ascBrowVect(windowname) $name
}

# right side browser
proc Brow_do_ChildSetValue {ndx} {
  global ascBrowVect
  set i [lindex [$ascBrowVect(children) get $ndx] 0]
  set name [Brow_childname [Brow_get_subname] $i]
  if {[catch {qlfdid $name} errmsg]} {return}
  Browser_do_SetValue $ascBrowVect(windowname) $name
}

# bottom browser
proc Brow_do_LocalSetValue {ndx} {
  global ascBrowVect
  set i [lindex [$ascBrowVect(local) get $ndx] 0]
  set name [Brow_childname [Brow_get_subname] $i]
  if {[catch {qlfdid $name} errmsg]} {return}
  Browser_do_SetValue $ascBrowVect(local) $name
}

proc Brow_do_LocalBrowse {ndx} {
  global ascBrowVect
  set i [lindex [$ascBrowVect(local) get $ndx] 0]
  Script_ClearInterrupt
  BROWSE [Brow_childname [Brow_get_subname] $i]
}

proc Brow_UpdateLocalLabel {leadkind} {
  global ascBrowVect
  switch $leadkind {
  REAL_ATOM_INST {set ascBrowVect(locallabel) {Real variables}}
  INTEGER_ATOM_INST {set ascBrowVect(locallabel) {Discrete variables}}
  REL_INST {set ascBrowVect(locallabel) {Real relations}}
  LREL_INST {set ascBrowVect(locallabel) {Logical relations}}
  REAL_CONSTANT_INST {set ascBrowVect(locallabel) {Real constants}}
  SET_ATOM_INST {set ascBrowVect(locallabel) {Discrete constants}}
  }
}

#
# proc Brow_do_UpdateCore {basename listbox instkindlist}
#-------------------------------------------------------------------------
# # Prototype implementation. Not to be optimized until we prove it needs
# # optimizing.
# This cannot be called until after Brow_do_ParentsBox.
# This should not be called until after Brow_do_UpdateChild
# Updates the local box based on the available information in the instance
# and ascBrowVect(localkinds).
# This implementation could be extremely speeded up by reconsidering
# whether we should have multiple local_frm.childbox of which only
# one is shown (a la probe) and whether we should revisit C code
# to return additional info to reduce the tcl work load.
# Additionally, the ui needs more buttons: this is a slight overload of
# the View button.
#
# dumb assumptions:
# all things in localbox are atoms (in the sense of having a value)
# user wants to see ATOM attributes/units
#         [brow_child_list search all $ascBrowVect(TypeorValue)]
#-------------------------------------------------------------------------
proc Brow_do_UpdateCore {base box kindlist} {
  ascclearlist $box
  set connector .
  set rootkind {}
  if {[catch {set rootkind [inst kind current]} err]} {
    return
  }
  set childlist [inst child]
  if {$rootkind=="ARRAY_ENUM_INST" || $rootkind=="ARRAY_INT_INST"} {
    set connector ""
  }
  foreach c $childlist {
    set name $base$connector$c
    if {[catch {qlfdid $name} errmsg]} {continue}
    set instkind [inst kind search]
    if {[lsearch -exact $kindlist $instkind] != -1 &&
        [libr_type_is_shown [inst type search]] != 0} {
      set line ""
      set esym " = "
      set val [inst atomvalue search]
      switch $instkind {
      REL_INST -
      LREL_INST {
          set esym " : "
        }
      }
      catch {set val [lindex [u_browgetval search] 0]} errmsg
      append line $c $esym $val
      $box insert end $line
    }
  }
}

#
# proc Brow_do_UpdateLocalBox {}
#-------------------------------------------------------------------------
# # Prototype implementation. Not to be optimized until we prove it needs
# # optimizing.
# This cannot be called until after Brow_do_ParentsBox.
# This should not be called until after Brow_do_UpdateChild
# Updates the local box based on the available information in the instance
# and ascBrowVect(localkinds).
# This implementation could be extremely speeded up by reconsidering
# whether we should have multiple local_frm.childbox of which only
# one is shown (a la probe) and whether we should revisit C code
# to return additional info to reduce the tcl work load.
# Additionally, the ui needs more buttons: this is a slight overload of
# the View button.
#
# dumb assumptions:
# all things in localbox are atoms (in the sense of having a value)
# user wants to see ATOM attributes/units
#         [brow_child_list search all $ascBrowVect(TypeorValue)]
#
# args should be empty or the lead type of a group of types corresponding
# to a checkbutton on the browser.
# It tells the user which button they toggled last, to help explain
# the 2 letter button labels.
#-------------------------------------------------------------------------
proc Brow_do_UpdateLocalBox {args} {
  global ascBrowVect
  Brow_UpdateLocalLabel $args
  set base [Brow_get_subname]
  # here we need to assemble localkinds from the current set of booleans
  Brow_UpdateLocalKinds
  Brow_do_UpdateCore [Brow_get_subname] \
                     $ascBrowVect(local) \
                     $ascBrowVect(localkinds)
}

#
# proc Brow_LeadCheckBtn {name1 name2 op}
#-------------------------------------------------------------------------
# updates the groups of variables based on the name of the lead
# instance type toggled by the check buttons.
#-------------------------------------------------------------------------
proc Brow_LeadCheckBtn {name1 name2 op} {
  global ascBrowVect
  if {$name1 != "ascBrowVect" || $op != "w"} {
    return
  }
  switch $name2 {
  localshow,REL_INST -
  localshow,LREL_INST -
  localshow,REAL_CONSTANT_INST -
  localshow,REAL_ATOM_INST {
    # do nothing for these, as they are singletons
      return
    }
  localshow,INTEGER_ATOM_INST {
      set ascBrowVect(localshow,BOOLEAN_ATOM_INST) \
        $ascBrowVect(localshow,INTEGER_ATOM_INST)
      set ascBrowVect(localshow,SYMBOL_ATOM_INST) \
        $ascBrowVect(localshow,INTEGER_ATOM_INST)
    }
  localshow,BOOLEAN_ATOM_INST -
  localshow,SYMBOL_ATOM_INST {
    # do nothing for these, as they are followers of INTEGER_ATOM_INST
      return
    }
  localshow,SET_ATOM_INST {
      set ascBrowVect(localshow,INTEGER_CONSTANT_INST) \
        $ascBrowVect(localshow,SET_ATOM_INST)
      set ascBrowVect(localshow,BOOLEAN_CONSTANT_INST) \
        $ascBrowVect(localshow,SET_ATOM_INST)
      set ascBrowVect(localshow,SYMBOL_CONSTANT_INST) \
        $ascBrowVect(localshow,SET_ATOM_INST)
    }
  localshow,INTEGER_CONSTANT_INST -
  localshow,BOOLEAN_CONSTANT_INST -
  localshow,SYMBOL_CONSTANT_INST {
    # do nothing for these, as they are followers of SET_ATOM_INST
      return
    }
  default {return}
  }
}
#
# proc Brow_UpdateLocalKinds {}
#-------------------------------------------------------------------------
# fills in the value of ascBrowVect(localkinds)
# given the current values of the localshow booleans
#-------------------------------------------------------------------------
proc Brow_UpdateLocalKinds {} {
  global ascBrowVect
  set ascBrowVect(localkinds) ""
  foreach i $ascBrowVect(locallist) {
    if {$ascBrowVect(localshow,$i)} {
      lappend ascBrowVect(localkinds) $i
    }
  }
}

#
#  proc Brow_do_UpdateChild { {TypeorVal TYPE} {atoms ATOMS} {passed PASSED}}
#-------------------------------------------------------------------------
#  Updates the child box based on the current instance. Will use as
#  default arguments the ascBrowVect(TypeorVal) variable.
#  Calls the brow_child_list routines with
#  these arguments to display the child (aka subitem) info. Sets the
#  current selection back to the Brow parents Box. The C-call WILL NOT
#  return an error if the current instance is NULL. This simplifies the
#  code a lot!!!
#-------------------------------------------------------------------------
proc Brow_do_UpdateChild { {TypeorVal TYPE} {atoms ATOMS} {passed PASSED}} {
   global ascBrowVect

   set nok [catch { 
                    brow_child_list current all $TypeorVal $atoms $passed
                  } childlist_or_err]
   if {$nok} {
     if {$childlist_or_err == "At leaves of the Instance Tree"} {
       $ascBrowVect(children) delete 0 end
       return
     }
     if {$childlist_or_err == "Child not found - check your root"} {
       $ascBrowVect(children) delete 1 end
       return
     }
   }
   set childlist $childlist_or_err
   $ascBrowVect(children) delete 0 end
   foreach child $childlist {
     $ascBrowVect(children) insert end $child
   }
   Brow_do_UpdateLocalBox
}

#
# proc Brow_SetDimNoise {n1 n2 mode}
#-------------------------------------------------------------------------
# toggle the C setting of dim noise whenever the user does.
#-------------------------------------------------------------------------
proc Brow_SetDimNoise {n1 n2 mode} {
  global ascBrowVect
  u_dim_setverify $ascBrowVect(dimconsistency)
}
#
# Brow_do_TypeorValue
#-------------------------------------------------------------------------
# This is the command that is bound to the checkbutton for setting the
# view in the browser to variable types or values.
#-------------------------------------------------------------------------
proc Brow_do_TypeorValue {} {
  global ascBrowVect
  Brow_do_UpdateChild \
    $ascBrowVect(TypeorValue) \
    $ascBrowVect(ShowAtoms) \
    $ascBrowVect(HidePassed)
}


#
# proc Brow_Update_Edit_Buttons {}
#-------------------------------------------------------------------------
# This procedure is bound to the Edit Menu Button.
# Used for enabling the Edit_Menu items depending on the type of the
# selected instance
#-------------------------------------------------------------------------
 # by default everybody is disabled until proven useful
proc Brow_Update_Edit_Buttons {} {
  global ascBrowVect
  set mb .browser.menubar.edit

  menu_disable_all $mb
  if {[$ascBrowVect(parents) size] == 0 || [catch {inst kind} ]} {
    return
  }

  # Run method -- menu_item 0
  #
  if {[string compare [inst kind] "MODEL_INST"] == 0} {
    set itype [inst type]
    if {[llength [libr_query -methods -type $itype]] || \
        [llength [libr_query -basemethods]]} {
      $mb entryconfigure 0 -state normal
    }
  }
  # Clear Vars -- menu_item 1
  #
  if {![inst atomchild]} {
    $mb entryconfigure 1 -state normal
  }
  # Set Value -- menu item 3
  #
  $mb entryconfigure 3 -state [BrowRealSetValueState]

  # read/write values always work
  # refine button 4
  # disable if the instance has no potential refinements or if
  # the instance is the child of an atom.
  if {[is_type_refined] != 0  && ![inst atomchild]} {
    $mb entryconfigure 4 -state normal
  }
  # merge button 5 and resume button 7
  if {![inst atomchild]} {
    $mb entryconfigure 5 -state normal
    $mb entryconfigure 7 -state normal
  }
}

proc BrowSolveState {} {
  if {![string compare [inst kind] "MODEL_INST"]} {
    return normal
  }
  return disabled
}
proc BrowShowCodeState {} {
  global ascPopInfo
  if {[inst type]==""} {
    return disabled
  }
  set lbl "Show code of [inst type]..."
  $ascPopInfo(menu) entryconfigure $ascPopInfo(index) -label $lbl
  return normal
}

proc BrowRealSetValueState {} {
  if {[inst isassignable]} {
    return normal
  }
  return disabled
}

proc BrowSetValueState {} {
  global ascPopInfo
  set lbl "Set value of [Brow_get_subname]"
  $ascPopInfo(menu) entryconfigure $ascPopInfo(index) -label $lbl
  return [BrowRealSetValueState]
}

proc BrowSetAttributeState {} {
  global ascPopInfo
  set lbl "Set attributes of [Brow_get_subname]"
  $ascPopInfo(menu) entryconfigure $ascPopInfo(index) -label $lbl
  return normal
}
#
# proc Brow_Update_View_Buttons {}
#-------------------------------------------------------------------------
# Disable or enable the view menu items.
#-------------------------------------------------------------------------
proc Brow_Update_View_Buttons {} {
  # save option and hide/unhide names items
  global ascBrowVect ascGlobalVect
  set mb .browser.menubar.view

  if {$ascGlobalVect(saveoptions) == 0} {
    $mb entryconfigure 9 -state disabled
  } else {
    $mb entryconfigure 9 -state normal
  }

  if {[$ascBrowVect(parents) size] == 0 || [catch {inst kind} ]} {
    return
  }
  set kind [inst kind]
  if {$kind != "ARRAY_ENUM_INST" && $kind != "ARRAY_INT_INST"} {
    $mb entryconfigure 4 -state normal
    $mb entryconfigure 5 -state normal
  } else {
    $mb entryconfigure 4 -state disabled
    $mb entryconfigure 5 -state disabled
  }
}

#
# proc Brow_Update_File_Buttons {}
#-------------------------------------------------------------------------
# Disable or enable the file menu items.
#-------------------------------------------------------------------------
proc Brow_Update_File_Buttons {} {
 # all buttons always valid currently.
}
#
# proc Brow_Update_Display_Buttons {}
#-------------------------------------------------------------------------
# Disable or enable the display menu items.
#-------------------------------------------------------------------------
proc Brow_Update_Display_Buttons {} {
  global ascBrowVect
  set mb .browser.menubar.display

  menu_disable_all $mb
  if {[$ascBrowVect(parents) size] == 0 || [catch {inst kind} ]} {
    return
  }
  set kind [inst kind]
  #  Atrributes -- menu_item 0
  #  implemented for real variables
  #$mb entryconfigure 1 -state normal


  # Attributes item 0
  if {$kind == "REAL_ATOM_INST"} {
    $mb entryconfigure 0 -state normal
  }

  #  Relations -- menu_item 1
  #  Enable if is a model instance, is a relation, or is
  #  an array of relation. The command __brow_isrelation handles the
  #  2nd and 3rd Case. Also enable if the instance is a
  #  REAL_ATOM_INST -- a little overloading here. What we
  #  do is display the relations that the atom is incident in.
  if {$kind == "MODEL_INST" || $kind == "REAL_ATOM_INST"} {
    $mb entryconfigure 1 -state normal
  } else {
    if {[__brow_isrelation current] == 1} {
      $mb entryconfigure 1 -state normal
    } {
      $mb entryconfigure 1 -state disabled
    }
  }

  #  Conditional Relations -- menu_item 2
  #  Enable if is a model instance

  if {$kind =="MODEL_INST" } {
    $mb entryconfigure 2 -state normal
  } else {
    $mb entryconfigure 2 -state disabled
  }

  #  Logical Relations -- menu_item 3
  #  Enable if is a model instance, is a logical relation, or is
  #  an array of logical relation. The command __brow_islogrel handles the
  #  2nd and 3rd Case. Also enable if the instance is a
  #  BOOLEAN_ATOM_INST. What we do is display the logical relations
  #  that the atom is incident in.
  #

  if {$kind == "MODEL_INST" || $kind == "BOOLEAN_ATOM_INST"} {
    $mb entryconfigure 3 -state normal
  } else {
    if {[__brow_islogrel current] == 1} {
      $mb entryconfigure 3 -state normal
    } else {
      $mb entryconfigure 3 -state disabled
    }
  }

  #  Conditional Logical Relations -- menu_item 4
  #  Enable if is a model instance

  if {$kind =="MODEL_INST" } {
    $mb entryconfigure 4 -state normal
  } else {
    $mb entryconfigure 4 -state disabled
  }


  #  When Statement -- menu_item 5
  #  Enable if is a model instance, is a when, or is
  #  an array of when. The command __brow_iswhen handles the
  #  2nd and 3rd Case.
  #

  if {$kind == "MODEL_INST"} {
    $mb entryconfigure 5 -state normal
  } else {
    if {[__brow_iswhen current] == 1} {
      $mb entryconfigure 5 -state normal
    } else {
      if {[__brow_isinstanceinwhen current] == 1 } {
        $mb entryconfigure 5 -state normal
      }  else {
        $mb entryconfigure 5 -state disabled
      }
    }
  }

  # Plot  -- menu item 7
  #
  if {[b_isplottable current]} {
    $mb entryconfigure 7 -state normal
  } else {
    $mb entryconfigure 7 -state disabled
  }
  # Statistics -- menu item 8
  #
  $mb entryconfigure 8 -state normal
}
#
# proc Brow_Update_Find_Buttons {}
#-------------------------------------------------------------------------
# Disable or enable the find menu items.
#-------------------------------------------------------------------------
proc Brow_Update_Find_Buttons {} {
  global ascBrowVect
  set mb .browser.menubar.find

  menu_disable_all $mb
  if {[$ascBrowVect(parents) size] == 0 || [catch {inst kind} ]} {
    return
  }
  # Enable all that are currently implemented, some for certain types only
  set disabled_list [list 7 8]
  set enabled_list  [list 0 1 2 3 4 5 6 9]

  foreach item $enabled_list {
    $mb entryconfigure $item -state normal
  }
  # type
  if {[$ascBrowVect(parents) size] != 0} {
    $mb entryconfigure 1 -state normal
  } else {
    $mb entryconfigure 1 -state disabled
  }
  # eligible
  if {[inst kind] == "MODEL_INST"} {
    $mb entryconfigure 5 -state normal
  } else {
    $mb entryconfigure 5 -state disabled
  }
  # active
  if {[inst kind] == "MODEL_INST"} {
    $mb entryconfigure 6 -state normal
  } else {
    $mb entryconfigure 6 -state disabled
  }
  # operands
  switch [inst kind] {
  WHEN_INST -
  LREL_INST -
  REL_INST {
      $mb entryconfigure 7 -state normal
    }
  default {
      $mb entryconfigure 7 -state disabled
    }
  }
}
#
# proc Brow_Update_Export_Buttons {}
#-------------------------------------------------------------------------
# Disable or enable the export menu items.
#-------------------------------------------------------------------------
proc Brow_Update_Export_Buttons {} {
  global ascBrowVect
  set mb .browser.menubar.export

  menu_disable_all $mb
  if {[$ascBrowVect(parents) size] == 0 || [catch {inst kind} ]} {
    return
  }
  # Enable export to solver
  # Will later be enabled for models and arrays of models.
  #
  if {[inst kind]=="MODEL_INST"} {
    $mb entryconfigure 0 -state normal
  }
  # probe legal always once not null
  if {![inst atomchild]} {
    $mb entryconfigure 1 -state normal
  }
  $mb entryconfigure 2 -state normal
}


#
# proc Brow_do_BooleanToggle {ndx}
#-------------------------------------------------------------------------
# flip the truth of child ndx in the childbox
#-------------------------------------------------------------------------
proc Brow_do_BooleanToggle {ndx} {
  global ascBrowVect
  set int_ndx [expr $ndx + 1]
  set element [brow_child_list current $int_ndx VALUE]
  if {$element == ""} {
    return
  }
  set childname [lindex [lindex $element 0] 0]
  #
  # PUT SOME SAFETY CHECKS HERE !!!!
  # (put some safety checks in your c code, nincompoop)
  #
  if {[catch {root $childname} ]} {return}
  set kind [inst kind]
  if {$kind == "BOOLEAN_INST" || $kind == "BOOLEAN_ATOM_INST"} {
    set childvalue [lindex [stripbraces $element] 2]
    set childname [lindex [stripbraces $element] 0]
    if {$childvalue == "TRUE"} {
       set childvalue "FALSE"
    } else {
       set childvalue "TRUE"
    }
    brow_assign $childvalue
    set name [Brow_get_subname].$childname
    oldinst;
    Brow_do_UpdateChild \
      $ascBrowVect(TypeorValue) \
      $ascBrowVect(ShowAtoms) \
      $ascBrowVect(HidePassed)
    HUB_Message_to_HUB BOOLEANUPDATED [sims getc]
    HUB_Message_to_HUB VARIABLEUPDATED [sims getc]
    HUB_Message_to_HUB VALUESET $name $childvalue
  } else {
    oldinst;
  }
}

#
# proc Brow_do_UpdateParent {qlfdid}
#-------------------------------------------------------------------------
# fill browser parent box with qlfdid.
# This function assumes that the name list is a preformatted
# name list, such as that returned from a call to qlfdid.
# example : a.b['q.z.w'][12].lower_bound would be in a list as:
#           a  b  ['q.z.w']  [12]  lower_bound.
# The Qlfdid_SplitPretty function served well for a long time,
# but choked on q['a.b.c'] (being fooled by the dots in the
# symbol.
#-------------------------------------------------------------------------
proc Brow_do_UpdateParent {name_list} {
  global ascBrowVect

  set w $ascBrowVect(parents)
  set len [llength $name_list]
  if {$len == "0"} {
    return
  }
  $w delete 0 end
  for {set c 0} {$c < $len} {incr c} {
     $w insert end [lindex $name_list $c]
  }
}

#
# proc Brow_do_Export2Probe {}
#------------------------------------------------------------------------
# browser export 2 probe  button
#------------------------------------------------------------------------
proc Brow_do_Export2Probe {} {# Attached to the Browser Export Menu
  global ascBrowVect
  Probe_Import_Filtered $ascBrowVect(windowname) [Brow_get_subname]
}

#
# proc Brow_do_ExportOne2Probe {}
#-------------------------------------------------------------------------
# Exports a single item to the current probe.
# kind of a dull thing to do.
#-------------------------------------------------------------------------
proc Brow_do_ExportOne2Probe {} {
  Probe_Import current [Brow_get_subname]
}

#
# proc Brow_Export_Any_2Browser {qlfdid}
#-------------------------------------------------------------------------
# focus browser on qlfdid. The call to btransfer is to set
# up the instance pointers. This call actually goes through
# the qlfdid search *again*. This can be made more efficient.!!
# One approach would be save the search list as a UserData Node
# and retrieve on the next call.
#-------------------------------------------------------------------------
proc Brow_Export_Any_2Browser {{name ""}} {
  global ascBrowVect
  if {$name == ""} {
    return
  }
  set nok [catch "qlfdid \{$name\}" errmsg]
  if {$nok} {
    set msg "Unable to locate simulation $name. "
    append msg $errmsg
    Brow_Raise_Alert $msg "Export Error"
    return
  }
  set name_list "$errmsg"
  set nok [catch "btransfer \{$name\}" errmsg]
  if {$nok} {
    Brow_Raise_Alert $errmsg
    return
  }
  Brow_do_UpdateParent "$name_list"
  Brow_do_UpdateChild \
    $ascBrowVect(TypeorValue) \
    $ascBrowVect(ShowAtoms) \
    $ascBrowVect(HidePassed)
  set ascBrowVect(currentsim) [lindex $name_list 0]
  Brow_Setup_Selection $ascBrowVect(parents)
  HUB_Message_to_HUB INSTBROWSED $name
  if {$ascBrowVect(visibility)} {
    newraise .browser
  }

}

# proc Brow_HandleSimsDelete
#------------------------------------------------------------------------
# This procedure will be registered with the HUB.
# If the current sim is the sim that is to be deleted then the g_instlist
# for the browser will be initialized, the currentsim (all this in C-land)
# will be set to NULL. The BrowserParents and ChildBox will be returned
# to a clean state. This will prepare the system for the deletion of a
# simulation
#------------------------------------------------------------------------
proc Brow_HandleSimsDelete {sims} {
  global ascBrowVect
  if {[sims getcurrent] != $sims} {
    return 0;
  }
  Brow_InitBrowser;
  return 0;
}

#
# proc Brow_HandleInstRefined
#------------------------------------------------------------------------
# This procedure will be called from the HUB.
# If an instance HAS been refined, this procedure will check to see
# if the instance was present in the browser by checking the name
# of its simulation. If it was the instance will be re-exported to the
# browser. Otherwise nothing will happen.
#------------------------------------------------------------------------
proc Brow_HandleInstRefined {args} {
  if {$args == ""} {return}
  set inst_name [lindex $args 0]
  set sim_name [lindex [split $inst_name .] 0]
  set cur_sim [sims getcurrent]
  if {$cur_sim == $sim_name} {
    Brow_Export_Any_2Browser $inst_name
    return 0;
  }
}

#
# proc Brow_HandleInstMerged
#------------------------------------------------------------------------
# This procedure will be called from the HUB.
# If an instance HAS been merged, this should hanlde the re-exporting to
# the browser. The same process as Brow_HandleInstRefined is used...
# For the time being this seems sufficient.
#------------------------------------------------------------------------
proc Brow_HandleInstMerged {args} {
  Brow_HandleInstRefined {args}
}

#
# proc Brow_HandleVariableUpdated
#-------------------------------------------------------------------------
# This function will be called for the browser whenever a variables
# values has changed. This call will be dispatched from the HUB.
#-------------------------------------------------------------------------
proc Brow_HandleVariableUpdated {{list ""}} {
  global ascBrowVect
  if {"[__brow_iname]" == "NULL_INSTANCE"} {
    return 0;
  }
  Brow_do_UpdateChild \
    $ascBrowVect(TypeorValue) \
    $ascBrowVect(ShowAtoms) \
    $ascBrowVect(HidePassed)
  return 0;
}

#
# proc Brow_HandleTypeHidden
#-------------------------------------------------------------------------
# This function will be called for the browser whenever a the TYPESHOW
# bit of a type description has changed. This call will be dispatched
# from the HUB.
#-------------------------------------------------------------------------
proc Brow_HandleTypeHidden {{list ""}} {
  global ascBrowVect
  if {"[__brow_iname]" == "NULL_INSTANCE"} {
    return 0;
  }
  Brow_do_UpdateChild \
    $ascBrowVect(TypeorValue) \
    $ascBrowVect(ShowAtoms) \
    $ascBrowVect(HidePassed)
  return 0;
}

#
# Do a series of finds to find undefined sets, integer const, 
# symbol const, boolean const, real const, integer const
# vars,
# var flags
proc Brow_do_FindUndefined {{inst current}} {
}

#
# proc Brow_do_FindbyName {}
#-------------------------------------------------------------------------
# Find.byname buttton in the browser
#-------------------------------------------------------------------------
proc Brow_do_FindbyName {} {
  global ascMonoEntry1 ascBrowVect
  set pos [setpos .browser 150 70]
  set current_name [Brow_get_subname]
  set AscMonoEntry1(font) $ascBrowVect(font)
  set data [VShowWindow.ascMonoEntry1 "Enter Name" $pos \
		"$current_name" "Find by Name"]
  set btn [lindex $data 0]
  if {$btn != 1} {
    return
  }
  set name [lindex $data 1]
  if {$name == ""} {
    return
  }
  Brow_Export_Any_2Browser $name
}

# proc Brow_Setup_FindBox
#-------------------------------------------------------------------------
# Sets up the box of found instances so that an export to the probe
# may be done. There is a much more efficient way of doing these finds
# which would maintain a list in C-land, rather than get instance names
# and then reconvert them when exporting. For the time being this is how
# it is being done. The same applies for cliques and aliases.
#-------------------------------------------------------------------------
proc Brow_Setup_FindBox {list {currentname ""}} {
  global ascListSelectB1Box ascBrowVect

  if {![string length $currentname]} {
    set currentname [Brow_get_subname]
  }
  set ascListSelectB1Box(grab) 0
  set ascListSelectB1Box(btn2name) TagAll
  set ascListSelectB1Box(btn3name) Browse
  set ascListSelectB1Box(btn4name) Probe
  set ascListSelectB1Box(btn5name) ""
  set ascListSelectB1Box(btn2destroy) 0
  set ascListSelectB1Box(btn3destroy) 0
  set ascListSelectB1Box(btn4destroy) 0
  set ascListSelectB1Box(btn2command) Browser_FindSelectAll
  set ascListSelectB1Box(btn3command) Browser_BrowseListSelect
  set ascListSelectB1Box(btn4command) Browser_ProbeListSelect
  set ascListSelectB1Box(title) "Instances Found in [Brow_get_subname]"
  set ascListSelectB1Box(toplevelname) ".browfindbytype"
  set ascListSelectB1Box(font) $ascBrowVect(font)
  set ascListSelectB1Box(selectmode) extended
  set ascListSelectB1Box(headline) "Instances matched:"


  if {$list==""} {puts stderr "no matches found" ; return}
  set newlist {}
  set sep .
  switch [inst kind] {
  ARRAY_INT_INST -
  ARRAY_ENUM_INST {
      set sep {}
    }
  }
  foreach i $list {
    lappend newlist $currentname$sep$i
  }
  set button [AscListSelectB1Box $newlist \
		  250x240[setpos .browser 150 20]]
}

#proc Brow_Convert_TRUE_FALSE
#-------------------------------------------------------------------------
# Normalise TRUE or FALSE to their respective numeric representations.
# It is cleaner to do it here rather than in C-land. The function
# __brow_find_type expects 1 or 0 for boolean type values.
#-------------------------------------------------------------------------
proc Brow_Convert_TRUE_FALSE {value} {
  if {$value == "TRUE" || $value == "true"} {
    return 1
  } elseif {$value == "FALSE" || $value == "false"} {
    return 0
  } else {
    return $value
  }
}

#
# proc Brow_FindbyType {}
#-------------------------------------------------------------------------
# Sets up the data to make the C-call to do the
# real find_by_type.
#-------------------------------------------------------------------------
proc Brow_FindbyType {query_list} {

  set type [string trim [lindex $query_list 0]]
  set attr [string trim [lindex $query_list 1]]
  set lowvalue [Brow_Convert_TRUE_FALSE [string trim [lindex $query_list 2]]]
  set hivalue  [Brow_Convert_TRUE_FALSE [string trim [lindex $query_list 3]]]
  if {$type == ""} {
    Brow_Raise_Alert "No type given,\nto find" "Find Error"
    return 1
  }
  if {$attr == ""} {
    set nok [catch {__brow_find_type cur $type} err_or_data]
  } elseif {$lowvalue == ""} {
    set nok [catch {__brow_find_type cur $type $attr} err_or_data]
  } elseif {$hivalue == ""} {
    set nok [catch {__brow_find_type cur $type $attr $lowvalue} err_or_data]
  } else {
    set nok [catch {__brow_find_type cur $type $attr $lowvalue $hivalue} \
		 err_or_data]
  }
  if {$nok} {
    Brow_Raise_Alert $err_or_data
    return 1
  }
  if {"$err_or_data"==""} {
    Brow_Raise_Alert "No matches found."
    return 1
  }
  Brow_Setup_FindBox $err_or_data
}

#
# proc Brow_do_FindbyType {}
#-------------------------------------------------------------------------
# Find.bytype button in the browser
#-------------------------------------------------------------------------
proc Brow_do_FindbyType {} {
  global AscMonoEntry4 ascBrowVect

  # The following code assumes that AscMonoEntry4.t was explicitly
  # sourced rather than auto-loaded. As such the AscMonoEntry4 array
  # exists, and can be written to. We want to stuff the box with the
  # last query made.

  set pos [setpos .browser 150 70]
  if {$AscMonoEntry4(count) == 0} {
    set AscMonoEntry4(resultNW) solver_var
    set AscMonoEntry4(resultNE) fixed
    set AscMonoEntry4(resultSW) TRUE
    set AscMonoEntry4(resultSE) ""
  }
  incr AscMonoEntry4(count)
  set AscMonoEntry4(font) $ascBrowVect(font)
  set data [VShowWindow.ascMonoEntry4 "Find by Type" $pos \
		"Type" "Attribute" "Low Value" "High Value" \
		$AscMonoEntry4(resultNW) $AscMonoEntry4(resultNE) \
		$AscMonoEntry4(resultSW) $AscMonoEntry4(resultSE)]
  set btn [lindex $data 0]
  if {$btn == "1"} {
    set query_list [lrange $data 1 4]
    set res [Brow_FindbyType $query_list]
  }
  if {$btn =="3"} {Help_button browser find.bytype}
}

#
# proc Brow_RelationType{}
#-------------------------------------------------------------------------
# Find.bytype button in the browser
#-------------------------------------------------------------------------
proc Brow_RelationType {relinst} {

  qlfdid $inst
  set nok [catch {qlfdid $name} err_msg]
  if {$nok} {
    puts "$err_msg"
    return -2
  }
  set nok [catch {__brow_reln_relopsearch} err_msg]
  return $err_msg
}


#
# proc Brow_RelationSatisfied {}
#-------------------------------------------------------------------------
#-------------------------------------------------------------------------
proc Brow_do_RelationSatisfied {} {
  error "Brow_do_RelationSatisfied not implemented"
}


#
# proc Browser_FindSelectAll {{tl ""}}
#-------------------------------------------------------------------------
# select all in the asclistselectb1box associate with tl, or
# select all in the last asclistselectb1box created if tl == ""
#-------------------------------------------------------------------------
proc Browser_FindSelectAll {{tl ""}} {
  AscListSelectB1SelectAll "$tl"
}

#
# proc Browser_BrowseListSelect {}
#-------------------------------------------------------------------------
# send first of any aliases selected/clique to the browser
#-------------------------------------------------------------------------
proc Browser_BrowseListSelect {} {
  global ascListSelectB1Box
  set list $ascListSelectB1Box(itemselected)
  if {$list != ""} {
    set item [lindex $list 0]
    Brow_Export_Any_2Browser $item
  }
}
#
# proc Browser_ProbeListSelect {}
#-------------------------------------------------------------------------
# send  any aliases/clique selected to the probe as single items
#-------------------------------------------------------------------------
proc Browser_ProbeListSelect {} {
  global ascListSelectB1Box
  set list $ascListSelectB1Box(itemselected)
  Probe_Import_List current $list
}

#
# proc Brow_do_FindAliases {}
#-------------------------------------------------------------------------
# Find.aliases button in the browser
#-------------------------------------------------------------------------
proc Brow_do_FindAliases {} {
  global ascListSelectB1Box ascBrowVect
  set list ""
  catch {set list [aliases current]}

  set ascListSelectB1Box(grab) 0
  set ascListSelectB1Box(btn2name) Browse
  set ascListSelectB1Box(btn3name) Probe
  set ascListSelectB1Box(btn4name) ""
  set ascListSelectB1Box(btn5name) ""
  set ascListSelectB1Box(btn2destroy) 0
  set ascListSelectB1Box(btn3destroy) 0
  set ascListSelectB1Box(btn4destroy) 0
  set ascListSelectB1Box(btn5destroy) 0
  set ascListSelectB1Box(btn2command) Browser_BrowseListSelect
  set ascListSelectB1Box(btn3command) Browser_ProbeListSelect
  set ascListSelectB1Box(title) "Aliases for [Brow_get_subname]"
  set ascListSelectB1Box(toplevelname) ".browaliases"
  set ascListSelectB1Box(font) $ascBrowVect(font)
  set ascListSelectB1Box(selectmode) extended
  set ascListSelectB1Box(headline) "ARE_THE_SAME instances:"


  if {$list==""} {puts stderr "no aliases" ; return}
  set button [AscListSelectB1Box $list \
		  250x240[setpos .browser 150 20]]
}

#
# proc Brow_do_FindISAs {}
#-------------------------------------------------------------------------
# Where.created button in the browser
#-------------------------------------------------------------------------
proc Brow_do_FindISAs {} {
  global ascListSelectB1Box ascBrowVect
  set list ""
  catch {set list [isas current]}

  set ascListSelectB1Box(grab) 0
  set ascListSelectB1Box(btn2name) Browse
  set ascListSelectB1Box(btn3name) Probe
  set ascListSelectB1Box(btn4name) ""
  set ascListSelectB1Box(btn5name) ""
  set ascListSelectB1Box(btn2destroy) 0
  set ascListSelectB1Box(btn3destroy) 0
  set ascListSelectB1Box(btn4destroy) 0
  set ascListSelectB1Box(btn5destroy) 0
  set ascListSelectB1Box(btn2command) Browser_BrowseListSelect
  set ascListSelectB1Box(btn3command) Browser_ProbeListSelect
  set ascListSelectB1Box(title) "[Brow_get_subname] created as"
  set ascListSelectB1Box(toplevelname) ".browisas"
  set ascListSelectB1Box(font) $ascBrowVect(font)
  set ascListSelectB1Box(selectmode) extended
  set ascListSelectB1Box(headline) "Constructed instances:"


  if {$list==""} {puts stderr "no isas, which is very odd!" ; return}
  set button [AscListSelectB1Box $list \
		  250x240[setpos .browser 150 20]]
}
#
# proc Brow_do_FindClique {}
#-------------------------------------------------------------------------
# Find.clique button in the browser
#-------------------------------------------------------------------------
proc Brow_do_FindClique {} {
  global ascListSelectB1Box ascBrowVect
  set list ""
  catch {set list [cliques]}

  set ascListSelectB1Box(grab) 0
  set ascListSelectB1Box(btn2name) TagAll
  set ascListSelectB1Box(btn3name) Browse
  set ascListSelectB1Box(btn4name) Probe
  set ascListSelectB1Box(btn5name) ""
  set ascListSelectB1Box(btn2destroy) 0
  set ascListSelectB1Box(btn3destroy) 0
  set ascListSelectB1Box(btn4destroy) 0
  set ascListSelectB1Box(btn2command) Browser_FindSelectAll
  set ascListSelectB1Box(btn3command) Browser_BrowseListSelect
  set ascListSelectB1Box(btn4command) Browser_ProbeListSelect
  set ascListSelectB1Box(title) "Clique of [Brow_get_subname]"
  set ascListSelectB1Box(toplevelname) ".browclique"
  set ascListSelectB1Box(font) $ascBrowVect(font)
  set ascListSelectB1Box(selectmode) extended
  set ascListSelectB1Box(headline) "ARE_ALIKE instances:"


  if {$list==""} {puts stderr "no clique!" ; return}
  set button [AscListSelectB1Box $list \
		  250x240[setpos .browser 150 20]]
}
#
# proc Brow_do_FindFixed {fixed}
#-------------------------------------------------------------------------
# Find free/fixed variables button in the browser popup
#-------------------------------------------------------------------------
proc Brow_do_FindFixed {fixed} {
  global ascListSelectBox ascSolvVect ascBrowVect

  switch $fixed {
  TRUE {
      Brow_FindbyType {real fixed 1}
    }
  FALSE -
  default {
      Brow_FindbyType {real fixed 0}
    }
  }
}
#
# proc Brow_do_FindEligible {}
#-------------------------------------------------------------------------
# Find.eligible variables button in the browser
#-------------------------------------------------------------------------
proc Brow_do_FindEligible {} {
  global ascListSelectBox ascSolvVect ascBrowVect

  set list {}
  set prefix "[Brow_get_subname]"
  if {[slv_checksys] && 0 == \
      [string compare [sims getc] [lindex [split [slv_get_pathname] .] 0]]} {
    set list [__brow_find_type cur solver_var message eligible]
    set ascListSelectBox(title) "Eligible in Solver"
  } else {
    set list ""
    set refresh 0
    catch {set list [lindex [brow_find_eligible 2] 0]}
    set ascListSelectBox(title) "Eligible in [Brow_get_subname]"
  }

  set ascListSelectBox(grab) 1
  set ascListSelectBox(btn3name) ""
  set ascListSelectBox(btn4name) ""
  set ascListSelectBox(btn5name) ""
  set ascListSelectBox(toplevelname) ".broweligible"
  set ascListSelectBox(font) $ascBrowVect(font)
  set ascListSelectBox(selectmode) browse
  set ascListSelectBox(headline) "Select to fix one of:"
  set newlist ""

  foreach i $list {
    lappend newlist $prefix.$i
  }
  if {$newlist==""} {
    puts stderr "No variables eligible to be fixed."
    Brow_Raise_Alert "No variables eligible\n to be fixed." "DOF analysis"
    return
  }
  set alist [lsort $newlist]
  set button [AscListSelectBox $newlist \
                250x240[setpos .browser 50 20]]
  if {$button==2} {return}
  Solve_EligListSelect 0 ;# what is this doing here?
}

#
# proc Brow_do_FindActiveRels {}
#-------------------------------------------------------------------------
# Find.active rels button in the browser
#-------------------------------------------------------------------------
proc Brow_do_FindActiveRels {} {
entertrace
  global ascListSelectB1Box ascSolvVect ascBrowVect

  set list {}
  set prefix "[Brow_get_subname]"
  if {[slv_checksys] && 0 == \
      [string compare [sims getc] [lindex [split [slv_get_pathname] .] 0]]} {
    set list [__brow_find_type cur relation message active]
    set ascListSelectB1Box(title) "Active Relations in Solver"
  } else {
    set list ""
    set refresh 0
    catch {set list [lindex [brow_find_activerels 2] 0]}
    set ascListSelectB!Box(title) "Active Relations in [Brow_get_subname]"
  }

  set ascListSelectB1Box(grab) 0
  set ascListSelectB1Box(btn2name) TagAll
  set ascListSelectB1Box(btn3name) Browse
  set ascListSelectB1Box(btn4name) Probe
  set ascListSelectB1Box(btn5name) ""
  set ascListSelectB1Box(btn2destroy) 0
  set ascListSelectB1Box(btn3destroy) 0
  set ascListSelectB1Box(btn4destroy) 0
  set ascListSelectB1Box(btn2command) Browser_FindSelectAll
  set ascListSelectB1Box(btn3command) Browser_BrowseListSelect
  set ascListSelectB1Box(btn4command) Browser_ProbeListSelect
  set ascListSelectB1Box(title) "Active Relations of [Brow_get_subname]"
  set ascListSelectB1Box(toplevelname) ".browactive"
  set ascListSelectB1Box(font) $ascBrowVect(font)
  set ascListSelectB1Box(selectmode) extended
  set ascListSelectB1Box(headline) "Active Relations:"

  foreach i $list {
    lappend newlist $prefix.$i
  }
  if {$newlist==""} {
    puts stderr "No active relations."
    Brow_Raise_Alert "No Active Relation\n" "Solver Configuration"
    return
  }
  set alist [lsort $newlist]
  set button [AscListSelectB1Box $newlist \
                250x240[setpos .browser 150 20]]
  return
leavetrace
}

#
# proc Brow_do_FindRels {}
#-------------------------------------------------------------------------
# Find.relations button in the browser
#-------------------------------------------------------------------------
proc Brow_do_FindRels {} {
  puts stdout "Finding relations not implemented"
}
#
# proc Brow_do_FindOpers {}
#-------------------------------------------------------------------------
# Find.operands button in the browser
#-------------------------------------------------------------------------
proc Brow_do_FindOpers {} {
  set list [inst operands -current]
  set root "[Brow_get_partname]"
  if {[llength $list]} {
    Brow_Setup_FindBox $list $root
  } else {
    Brow_Raise_Alert "No operands found in [Brow_get_subname]" \
      "Operands message"
  }
}
#
# proc Brow_do_FindParents {}
#-------------------------------------------------------------------------
# Find.parents button in the browser
#-------------------------------------------------------------------------
proc Brow_do_FindParents {} {
  puts stdout "Finding parents not implemented"
}
#
# proc Brow_do_FindPendings {}
#-------------------------------------------------------------------------
# Find.pendings button in the browser
#-------------------------------------------------------------------------
proc Brow_do_FindPendings {} {
  if {[bnumpendings instance current] > 0} {
    puts "\n--------------------------------------------------------------\n\
          Pendings statements for the instance\n"
    bwritependings [sims getcurrentsim]
    puts "\n--------------------------------------------------------------"
      return;
  } else {
    puts "\n--------------------------------------------------------------\n"
    puts "No pendings statements for the browser instance\n"
  }
}

#
# proc Qlfdid_SplitPretty {qlfdid}
#-------------------------------------------------------------------------
# parse qlfdid: .->spc [ -> spc[
#-------------------------------------------------------------------------
proc Qlfdid_SplitPretty {name} {
  set b $name
  regsub -all {\.} $b " " b;
  regsub -all {\[} $b " \[" b;
  regsub -all {\]} $b "\]" b;
  return $b;
}

#
# Brow_Setup_Selection {listbox {startpos "end"} {endpos "end"}}
#-------------------------------------------------------------------------
# clear present list selection and select from startpos to endpos
#-------------------------------------------------------------------------
proc Brow_Setup_Selection {listbox {startpos "end"} {endpos "end"}} {

    $listbox selection clear 0 end
    $listbox selection set $startpos $endpos
}

#
# proc Browser_ProcShowCode
#-------------------------------------------------------------------------
# show procedure code for each procedure in $ascListSelectBox(itemselected)
# assuming each item is an proc of the current instance.
#-------------------------------------------------------------------------
proc Browser_ProcShowCode {currentname} {
  global ascListSelectBox ascUtilVect ascDispVect
  if {$ascListSelectBox(itemselected)==""} {return}

  DispClear;
  qlfdid $currentname
  foreach i $ascListSelectBox(itemselected) {
    set outputfile [FileUniqueName "$ascUtilVect(asctmp)/ascdisproc"]
    bgetproc $i $outputfile search
    set f [open $outputfile]
    # read chunks of 10k
    while {![eof $f]} {
      $ascDispVect(textBox) insert end [read $f 10000]
    }
    close $f
    file delete $outputfile
  }
  DispSetEntry "Method code from $currentname"
  newraise .display
}

#
# proc Browser_do_Methods {}
#-------------------------------------------------------------------------
# browser Edit.RunMethods button
#-------------------------------------------------------------------------
proc Browser_do_Methods {} {
  global ascBrowVect
  global ascListSelectBox
  global ascLibrVect

  set currentname [Brow_get_subname]
  set ascListSelectBox(grab) 0
  set ascListSelectBox(btn3name) Show
  set ascListSelectBox(btn4name) ""
  set ascListSelectBox(btn5name) ""
  set ascListSelectBox(btn3destroy) 0
  set ascListSelectBox(btn3command) "Browser_ProcShowCode $currentname"
  set ascListSelectBox(headline) "Methods ($currentname):"
  set ascListSelectBox(toplevelname) ".browprocedures"
  set ascListSelectBox(font) $ascBrowVect(font)
  set ascListSelectBox(selectmode) extended
  set ascListSelectBox(title) "Select Method"

  set typename [inst type]
  set proc_list {}
  if {[catch {set proc_list [libr_query -methods -type $typename]} err]} {
    return;
  }
  if {$proc_list=="1" && [llength [libr_query -basemethods]] == 0} {
    return
  }
  foreach i [libr_query -basemethods] {
    if {[lsearch -exact $proc_list $i] == -1} {
      lappend proc_list $i
    }
  }
  set button [AscListSelectBox [lsort -dictionary $proc_list] \
                250x240[setpos .browser 50 50]]
  if {$button==2} {return}
  set Proc_list $ascListSelectBox(itemselected)
  if {$Proc_list != "" } {
    foreach Proc $Proc_list {
      if {[string length $Proc]==0} {
        continue
      }
      set nok [catch {
                 brow_runmethod -method $Proc \
                 -qlfdid $currentname \
                 -backtrace $ascLibrVect(btuifstop) \
                 -stopOnErr $ascLibrVect(ignorestop)
               } err]
      puts -nonewline "Running method $Proc in "
      puts $currentname
      if {$nok} {
	 Brow_Raise_Alert $err" "Method Error"
	 return 1
      }
      HUB_Message_to_HUB PROCRUN $currentname.$Proc
    }
    Brow_do_UpdateChild \
      $ascBrowVect(TypeorValue) \
      $ascBrowVect(ShowAtoms) \
      $ascBrowVect(HidePassed)
    HUB_Message_to_HUB VARIABLEUPDATED [sims getc]
    HUB_Message_to_HUB WHENVARUPDATED [sims getc]
#
# This is not required anymore. Its consequences are now  a part
# of the consequences of WHENVARUPDATED.
# (wish this were TRUE: baa 6/11/97
    HUB_Message_to_HUB BOOLEANUPDATED [sims getc]
#
    return 0;
  }
  Brow_Raise_Alert "No method was selected" "Intialization Error"
  return 1;
}

#
# proc Browser_do_ClearVars {}
#-------------------------------------------------------------------------
# browser Edit.ClearVars button
#-------------------------------------------------------------------------
proc Browser_do_ClearVars {} {
    Solve_do_Flush do_not_record
    free_all_vars
    HUB_Message_to_HUB CLEARVARS [Brow_get_subname]
}

#
# proc Browser_do_SetValue {{toplevel self} {instname ""}}
#-------------------------------------------------------------------------
# browser Edit.setvalue button
# if a name is supplied, sets that value instead of current instance
#-------------------------------------------------------------------------
proc Browser_do_SetValue {{toplevel self} {instname ""}} {
  global ascBrowVect
  global AscEntryBox2
  
  if {[string compare $toplevel "self"]==0} {
    set toplevel $ascBrowVect(windowname)
  }
  if {$instname == ""} {
    set instname [Brow_get_subname]
  }
  if {[catch {qlfdid $instname} errmsg]} {
    append msg "Browser_do_SetValue unable to find" $instname
    error $msg "This isn't a bug-- it's a user mistake"
  }
  if {![inst isassignable search]} {
    return
  }

  if {[catch {set cur_value [u_browgetval search]} err]} {
    set cur_value [inst atomvalue search]
    set cur_units "\*"
  } else {
    set cur_value [stripbraces $cur_value]
    set cur_units [lindex [split $cur_value] 1]
    set cur_value [lindex $cur_value 0]
  }
  set pos [setpos $toplevel 65 65]
  set title {}
  append title $instname 
  switch [inst kind search] {
  SYMBOL_INST -
  SYMBOL_ATOM_INST -
  SYMBOL_CONSTANT_INST {
      append title "\nValue / Units\n (do not include ' around value)"
    }
  default {
      append title "\nValue / Units"
    }
  }
  set data [VShowWindow.ascMonoEntry2 $title $pos $cur_value $cur_units]
  set btn [lindex $data 0]
  if {$btn == 1 && ( \
      [string compare $cur_value [lindex $data 1]] !=0 || \
      [string compare $cur_units [lindex $data 2]] !=0) } {
    set value [lindex $data 1]
    set units [lindex $data 2]
    brow_assign -search $value $units
    HUB_Message_to_HUB VALUESET $instname $value $units
    Brow_do_UpdateChild \
      $ascBrowVect(TypeorValue) \
      $ascBrowVect(ShowAtoms) \
      $ascBrowVect(HidePassed)
    # Brow_do_UpdateChild (in local Core update) moves qlfdid.
    qlfdid $instname
    if {[inst iswhenvar search]} {
      HUB_Message_to_HUB WHENVARUPDATED $instname
        #
        # solver needs to know about potential changes of configuration
        #
    } else {
      if {[inst kind search]=="BOOLEAN_INST" || \
          [inst kind search]=="BOOLEAN_ATOM_INST"} {
        HUB_Message_to_HUB BOOLEANUPDATED $instname
        #
        # solver needs to know about potential dof changes
        #
      } else {
        HUB_Message_to_HUB VARIABLEUPDATED $instname
      }
    }
  }
}
#
proc Brow_InitSetvalue {} {
  global ascSetvalueVect ascBrowVect
  set ascSetvalueVect(basenamelist) [list rootname]
  set ascSetvalueVect(namelist) [list rootname]
  set ascSetvalueVect(rootname) ""
  set ascSetvalueVect(rootname.label) "Assigning in:"
  set ascSetvalueVect(rootname.type) string

  set ascSetvalueVect(grab) 1
  set ascSetvalueVect(cancellable) 1
  set ascSetvalueVect(npages) 1
  set ascSetvalueVect(toplevel) .setvalue
  set ascSetvalueVect(titlebase) "Set instance values: "
  set ascSetvalueVect(helpcommand) {Help_button browser.set.values}
  set ascSetvalueVect(whenokcommand) Brow_CheckSetvaluesInput

  set ascParPageVect(btn_font) $ascBrowVect(font)
  set ascParPageVect(lbl_font) $ascBrowVect(font)
}

#
# proc Brow_CheckSetvaluesInput {} 
#------------------------------------------------------------------------
# Checks the string entries in a for not containing ' characters.
# checks ${c}__uni_ string entries in a for being legal units.
#------------------------------------------------------------------------
proc Brow_CheckSetvaluesInput {} {
 # this shouldn't be necessary, ASSIGN should be robust to bad input
 # and we should catch around ASSIGN.
}

#
# proc Brow_SetvalueConfigure {name}
#------------------------------------------------------------------------
# Sets up the namelist in ascSetvalueVect based on name.
# Do not call this with a bad instance name.
# On return, ascSetvalueVect(vallist) is the list of leaf names of
# parts in instance $name which were offered to the user for assignment.
# Does not unroll arrays.
# If a part named foo in vallist is of type 'real' then there will also be
# an element foo__uni_ of type 'string'
# there may be other elements in ascSetvalueVect, but they are irrelevant
# if not in vallist.
#------------------------------------------------------------------------
proc Brow_SetvalueConfigure {name} {
  global ascSetvalueVect

  set ascSetvalueVect(namelist) $ascSetvalueVect(basenamelist)
  set ascSetvalueVect(entrywidth) 20
  set ascSetvalueVect(rootname) $name
  set ascSetvalueVect(rootname.choices) $ascSetvalueVect(rootname)
  set ascSetvalueVect(title) $ascSetvalueVect(titlebase)
  append ascSetvalueVect(title) $ascSetvalueVect(rootname)
  # if name is assignable (atomic & !relation), create rootvalue
  # entry based on type.
  # get childlist of name, find assignables and their types,
  # build name list, including a units entry for reals
  qlfdid $name ;# set g_search_inst in C land
  set clist [inst child search] ;# want the full list, not just viewables
  set ascSetvalueVect(vallist) ""
  set counter 0
  set page 1
  foreach c $clist {
    incr counter
    if {$page * 10 < $counter} { incr page }
    set cname [Brow_childname $name $c]
    if {[catch {qlfdid $cname} errmsg] || ![inst isassignable search]} {
      continue ;# skip NULL children and unassignables
    }
    lappend ascSetvalueVect(namelist) $c
    lappend ascSetvalueVect(vallist) $c
    set ascSetvalueVect($c.label) $c
    set ascSetvalueVect($c.page) $page
    set tmplen 0
    catch {set tmplen [string length [inst atomvalue search]]}
    if {$tmplen && $tmplen < 40 && $tmplen > $ascSetvalueVect(entrywidth)} {
      set ascSetvalueVect(entrywidth) $tmplen
    }
    switch [inst kind search] {
    REAL_INST -
    REAL_ATOM_INST -
    REAL_CONSTANT_INST { 
                         set cur_value ""
                         set cur_units ""
                         if {[catch {set cur_value [u_browgetval search]} \
                                err]} {
                           set cur_value [inst atomvalue search]
                           set cur_units "\*"
                         } else {
                           set cur_value [lindex $cur_value 0]
                           set cur_units [lindex [split $cur_value] 1]
                           set cur_value [lindex $cur_value 0]
                         }
                         set ascSetvalueVect($c) $cur_value
                         set ascSetvalueVect($c.type) real
                         set ascSetvalueVect($c.old) $cur_value
                         lappend ascSetvalueVect(namelist) ${c}__uni_
                         set tmplen 0
                         catch {set tmplen [string length $cur_units]}
                         if {$tmplen < 40 && \
                             $tmplen > $ascSetvalueVect(entrywidth)} {
                           set ascSetvalueVect(entrywidth) $tmplen
                         }
                         set ascSetvalueVect(${c}__uni_) $cur_units
                         set ascSetvalueVect(${c}__uni_.type) string
                         set ascSetvalueVect(${c}__uni_.page) $page
                         set ascSetvalueVect(${c}__uni_.old) $cur_units
                         set ascSetvalueVect(${c}__uni_.label) "Units for "
                         append ascSetvalueVect(${c}__uni_.label) $c
                       }
    INTEGER_INST -
    INTEGER_ATOM_INST -
    INTEGER_CONSTANT_INST {
                            set ascSetvalueVect($c) [inst atomvalue search]
                            set ascSetvalueVect($c.old) $ascSetvalueVect($c)
                            set ascSetvalueVect($c.type) int
                          }
    BOOLEAN_INST -
    BOOLEAN_ATOM_INST -
    BOOLEAN_CONSTANT_INST {
                            set ascSetvalueVect($c) [inst atomvalue search]
                            switch $ascSetvalueVect($c) {
                            FALSE {
                              set ascSetvalueVect($c) 0
                              set ascSetvalueVect($c.old) 0
                              set ascSetvalueVect($c.type) bool
                            }
                            TRUE {
                              set ascSetvalueVect($c) 1
                              set ascSetvalueVect($c.old) 1
                              set ascSetvalueVect($c.type) bool
                            }
                            UNDEFINED {
                              set ascSetvalueVect($c) UNDEFINED
                              set ascSetvalueVect($c.old) UNDEFINED
                              set ascSetvalueVect($c.type) bool
                            }
                            default { 
                              error \
                              "Brow_SetvalueConfigure bool confused"
                            }
                            }; # END inner switch
                          }
    SYMBOL_INST -
    SYMBOL_ATOM_INST -
    SYMBOL_CONSTANT_INST {
                           set ascSetvalueVect($c) [inst atomvalue search]
                           set ascSetvalueVect($c.old) $ascSetvalueVect($c)
                           set ascSetvalueVect($c.type) string
                         }
    SET_ATOM_INST -
    SET_INST {
               # do nothing with sets yet; # too complicated
             }
    default { 
               # above should be a complete listing of our assignable kinds.
               error \
                 "Brow_SetvalueConfigure can't cope with [inst kind search]"
            }
    }
  }
  set ascSetvalueVect(npages) $page
}

#
# proc Brow_childname {root leaf}
#------------------------------------------------------------------------
# Figures out how to add leaf to root and returns the combination.
# Basically, this centralizes the checks for gluing array elements 
# to root names in qualified id production. 
#------------------------------------------------------------------------
proc Brow_childname {root leaf} {
  if {$leaf == ""} {return $root}
  if { [string index $leaf 0] != "\["} {
    append root .$leaf
  } else {
    append root $leaf
  }
  return $root
}

#
# proc Browser_SetvalueBox {caller name}
#------------------------------------------------------------------------
# Caller is the name of the toplevel window calling this function.
# Set values dialog for assignable children of name.
# $name may point to any sort of instance that has assignable children.
# If nothing to do, returns silently.
# name is a full qualified identifier.
# Uses the ASSIGN operator because we can't cope with the
# hub event generation here without creating an impenetrable mess.
#------------------------------------------------------------------------
proc Browser_SetvalueBox {caller name} {
  global ascProbVect
  global ascSetvalueVect

  Brow_SetvalueConfigure $name
  if {$ascSetvalueVect(vallist) == ""} {
    return; # don't call parpage as it may barf on empty.
  }
  ascParPage ascSetvalueVect [setpos $caller 0 0] 1 0
  if {$ascSetvalueVect(__,cancelled)} {return}
  foreach i $ascSetvalueVect(vallist) {
    switch $ascSetvalueVect($i.type) {
    real {
           if {$ascSetvalueVect($i) == "UNDEFINED"} {continue}
           if {$ascSetvalueVect($i.old) != $ascSetvalueVect($i) || \
               $ascSetvalueVect(${i}__uni_.old) != \
                   $ascSetvalueVect(${i}__uni_)} {
             puts $i
             if {$i != "__atom_value__"} {
               if {[catch {ASSIGN [Brow_childname $name $i] \
                           $ascSetvalueVect($i) $ascSetvalueVect(${i}__uni_)
                          } errmsg]} {
                 append errmsg ": " $i " " $ascSetvalueVect($i) " "
                 append errmsg $ascSetvalueVect(${i}__uni_)
                 puts $errmsg
               }
             } else {
               if {[catch {ASSIGN $name $ascSetvalueVect($i) \
                                  $ascSetvalueVect(${i}__uni_)
                          } errmsg]} {
                 append errmsg ": " $name " " $ascSetvalueVect($i) " "
                 append errmsg $ascSetvalueVect(${i}__uni_)
                 puts $errmsg
               }
             }
           }
         }
    int -
    bool -
    string {
             if {$ascSetvalueVect($i) == "UNDEFINED"} {continue}
             if {$ascSetvalueVect($i.old) != $ascSetvalueVect($i)} {
               if {$i != "__atom_value__"} {
                 if {[catch {ASSIGN [Brow_childname $name $i] \
                                $ascSetvalueVect($i)} errmsg]} {
                   append errmsg ": " $i " " $ascSetvalueVect($i) " "
                   append errmsg $ascSetvalueVect(${i}__uni_)
                   puts $errmsg
                 }
               } else {
                 if {[catch {ASSIGN $name $ascSetvalueVect($i)} errmsg]} {
                   append errmsg ": " $name " " $ascSetvalueVect($i) " "
                   append errmsg $ascSetvalueVect(${i}__uni_)
                   puts $errmsg
                 }
               }
             }
           }
    default {
              error "binary data type in Brow_SetvalueBox???"
            }
    }
  }
}

#===========================================
#
# proc Brow_do_Read {}
#-------------------------------------------------------------------------
# Read values back into an instance.
# instance should have at least the namespace of the instance which was used
# when writing the read file.
# Instance doesn't have to be in the browser.
#-------------------------------------------------------------------------
proc Brow_do_Read {} {
  global ascBrowVect
  set defaultname [file dirname $ascBrowVect(filename)]
  set filename [tk_getOpenFile \
    -defaultextension "" \
    -filetypes $ascBrowVect(filetypes) \
    -initialdir $defaultname \
    -parent .browser \
    -title {Read saved values file}]

  if {$filename == "" || [file isdirectory $filename]} {
    return 1;
  } else {
    puts "Reading values from $filename"
    if {[catch {Brow_parse_values $filename 0} err]} {
      puts "Problem reading values file:"
      puts "error>>>$err<<<"
    }
    set ascBrowVect(filename) $filename
    set newext "[file extension $filename]"
    if {$newext != ""} {
      set ascBrowVect(lastreadextension) $newext
      ascresort_filetypes ascBrowVect lastreadextension
    }

    update idletasks
  }
  # here we should be grabbing the first line of filename, taking its
  #second to last item, and issuing the updated calls with that sim name.
  HUB_Message_to_HUB VARIABLEUPDATED
  HUB_Message_to_HUB BOOLEANUPDATED
  HUB_Message_to_HUB DATAREAD $filename
}

proc Brow_do_SaveOptions {} {
  View_Save_Window_Options browser
}
#
# proc Brow_do_Write {}
#-------------------------------------------------------------------------
# write real and boolean values from current instance downward to file.
# not particularly picky about what ARE_THE_SAMEd parts get called.
# The prefix that is used at the moment is : "qassgn3 \{".
# "qassgn3" is the fastest version of the reading code.
# The dummy_name is a requirement of teh bwritevalues command.
#-------------------------------------------------------------------------
proc Brow_do_Write {} {
  global ascBrowVect

  set defaultname $ascBrowVect(filename)
  set filename [tk_getSaveFile \
    -defaultextension "" \
    -filetypes $ascBrowVect(filetypes) \
    -initialfile $defaultname \
    -parent .browser \
    -title {Save variable values}]

  if {$filename == ""} {
    return 1;
  } {
    set ascBrowVect(filename) $filename
    bwritevalues $filename  "qassgn3 \{" current "dummy_name" #fast
    puts "Wrote values file $filename."
  }
  HUB_Message_to_HUB DATAWRITE [Brow_get_subname] $filename
}

#
# proc Browser_RefineShowCode {}
#-------------------------------------------------------------------------
# display code of type (found in ascListSelectBox) in the display window
#-------------------------------------------------------------------------
proc Browser_RefineShowCode {} {
  global ascListSelectBox ascUtilVect ascDispVect

  if {$ascListSelectBox(itemselected)==""} {return}
  set type [lindex $ascListSelectBox(itemselected) 0]
  set inputfile [file_by_type $type]
  set outputfile [FileUniqueName "$ascUtilVect(asctmp)/ascdiscode"]
  set result [Disp_ShowCode $type $inputfile $outputfile]
  if {$result == "0"} {
    FastFileInText $ascDispVect(textBox) $outputfile
  }
  file delete $outputfile
  DispSetEntry "Refinement code of $type"
  newraise .display
}

#
# proc Browser_Refine {}
#-------------------------------------------------------------------------
# browser Edit.refine button
# target_type is the target_type to refine to. This procedure does the
# actual work of refining the types by calling the C-code. inst_context
# specifies whether the current or search instance.
# NOTE: The current sim will be re-exported to the browser. This is a fast
# operation and ensures that none of the browser instances are all update
# after the refine has been done, and that they are not looking at *moved*
# instances.
#-------------------------------------------------------------------------
proc Browser_Refine {target_type {inst_context "current"}} {
  if {$target_type == ""} {return 1}
  set nok [catch "brefine $target_type $inst_context" err_msg]
  if {$nok} {
    Brow_Raise_Alert "$err_msg" "Refine Error";
    return 1;
  } {
    return 0;
  }
}
#
# proc Browser_do_Refine {}
#-------------------------------------------------------------------------
# browser Edit.refine button
# target_type is the target_type to refine to.
#-------------------------------------------------------------------------
proc Browser_do_Refine {} {
  global ascBrowVect

  global ascListSelectBox
  set ascListSelectBox(grab) 1
  set ascListSelectBox(btn4name) ""
  set ascListSelectBox(btn3name) Show
  set ascListSelectBox(btn5name) ""
  set ascListSelectBox(btn3destroy) 0
  set ascListSelectBox(btn3command) Browser_RefineShowCode
  set ascListSelectBox(headline) "Eligible types for refinement:"
  set ascListSelectBox(toplevelname) ".browrefine"
  set ascListSelectBox(selectmode) browse
  set ascListSelectBox(font) $ascBrowVect(font)
  set ascListSelectBox(title) "Refining [inst type] [Brow_get_subname]:"

  set type [inst type]
  set button [AscListSelectBox [drefines_meall $type] \
                250x240[setpos .browser 50 50]]
  if {$button==2} {return 1}
  if {$button==1} {
    set target_type $ascListSelectBox(itemselected)
  }
  if {$target_type ==""} {
    Brow_Raise_Alert "You must specify\na type." "Refine Error";
    return 1;
  }
  #
  # Save the old names so that we can set back up
  # the browser when finished. Inform all interested parties
  # that the move is about to take place. THEN do the refine.
  #
  set old_name [Brow_get_subname]
  set old_sim  [sims getcurrent]
  HUB_Message_to_HUB INSTANCEMOVED $old_sim
  set nok [Browser_Refine $target_type current]
  if {$nok} { return 1 }

# The following should handle the re-exporting of the given
# instance to the Browser. We don't do it here directly as a
# we need to send the message to the HUB that something WAS refined
# in any Case but don't want to set up a potential infinite loop, or
# end up calling the re-export code twice.

  HUB_Message_to_HUB INSTREFINED $old_name $target_type
}

# proc Browser_Merge {}
#-------------------------------------------------------------------------
# This command ASSUMES that g_search_inst is looking at the instance to
# be merged with. Will attempt to merge the current and the search inst.
# Hence it takes no args. This routine could be made more general, by
# setting up more instance pointers. The code for BrowMergeCmd in
# interface/BrowserProc.c should then be modified accordingly.
#-------------------------------------------------------------------------
proc Browser_Merge {} {
  set nok [catch "bmerge" err_msg]
  if {$nok} {
    Brow_Raise_Alert "$err_msg" "Merge Error"
    return 1
  }
  return 0
}

#
# proc Browser_do_Merge {}
#-------------------------------------------------------------------------
# This command is bound to the merge menu item on the browser. It will
# prompt for a dialog box for the name of the instance to be merged
# with. Will call 'qlfdid' to search for the instance and if found
# leave g_search_isnt looking at the instance. Brow_Merge will be then
# called to do the true merge.
# target_inst is the fully qualified name of the instance that is going
# to be merged with the current instance. Will work only on the current
# instance.
#-------------------------------------------------------------------------
proc Browser_do_Merge {} {
  global AscMonoEntry1 ascBrowVect

  set sim [sims getc]
  set name [Brow_get_subname]
  set AscMonoEntry1(font) $ascBrowVect(font)
  set target_inst [lindex [VShowWindow.ascMonoEntry1 \
    "Instance Name :" "420x100[setpos .browser 50 50]" \
    "" "Enter fully qualified instance name to merge with."] 1]
  if {$AscMonoEntry1(button)==2} {return 1}
  if {$AscMonoEntry1(button)==3} {puts "No help yet"; return 1}
  if {$target_inst ==""} {
    Brow_Raise_Alert "You must specify\nan instance name." "Merge Error";
    return 1;
  }
  set nok [catch "qlfdid {$target_inst}" err_msg]
  if {$nok} {
    Brow_Raise_Alert "Error in finding instance\n$err_msg" "Merge Error"
    return 1;
  }

# Inform all interested parties that an instance is about
# to be moved in memory. This includes the Browser itself.
# THEN do the merge.

  HUB_Message_to_HUB INSTANCEMOVED $sim
  set nok [Browser_Merge]
  if {$nok} { return 1 }

# The re-exporting of the instance to the browser is handled by
# the below code. See the note in Browser_do_Refine for more details.
#
  HUB_Message_to_HUB INSTMERGED $name $target_inst
}

#
# proc Browser_do_ResumeCompile {}
#-------------------------------------------------------------------------
# browser Edit.compile.resumecompilation button
#-------------------------------------------------------------------------
proc Browser_do_ResumeCompile {} {
  global ascBrowVect
  set old_name [Brow_get_subname]
  set old_sim  [sims getcurrent]
  HUB_Message_to_HUB INSTANCEMOVED $old_sim
  sim_reinstantiate $old_name
  global ascScripVect
  set muffle $ascScripVect(executing)
  if {!$muffle} {set ascScripVect(executing) 1}
  Brow_Export_Any_2Browser $old_name
  if {!$muffle} {set ascScripVect(executing) 0}
  #if {$nok} { return 1 }
  HUB_Message_to_HUB INSTANCERESUMED $old_sim
}

#
# proc Brow_Raise_Alert {errmsg {label "Error"} {geom "200x70+480+200"}}
#-------------------------------------------------------------------------
# browser alertbox
#-------------------------------------------------------------------------
proc Brow_Raise_Alert {errmsg {label "Error"} {geom "200x70+480+200"}} {
  Script_Raise_Alert $errmsg $label
}
#
# proc Brow_CreatePart_ErrorCheck {partname parttype}
#-------------------------------------------------------------------------
# inverse boolean sanity check on create part button
# Needed for Create Part only.
#-------------------------------------------------------------------------
proc Brow_CreatePart_ErrorCheck {partname parttype} {
  if {$partname == ""} {
    set errmsg "No name was given\n for the part"
    Brow_Raise_Alert $errmsg "Create Part Error"
    return 1;
  }
  if {$parttype == ""} {
    set errmsg "No type was given"
    Brow_Raise_Alert $errmsg "Create Part Error"
    return 1;
  }
  puts "Check childexist"
  set list [inst child]
  if {[lsearch $list $partname] == -1} {
    set errmsg "The name given\nalready exists"
    Brow_Raise_Alert $errmsg "Create Part Error"
    return 1;
  }
  puts "Check typeexist"
  if {![libr_query -exists -type $parttype]} {
    set errmsg "The specified type\ndoes not exist"
    Brow_Raise_Alert $errmsg "Create Part Error"
    return 1;
  }
  return 0;
}
#
# proc Browser_do_CreatePart {}
#-------------------------------------------------------------------------
# browser Edit.compile.createpart  button
#-------------------------------------------------------------------------
proc Browser_do_CreatePart {} {
  global ascBrowVect AscMonoEntry2

# disabled until problem with typeexist (library.h:FindType) fixed.
  Brow_Raise_Alert "Create Part not\nyet implemented"
  return 1;

  set pos [setpos .browser 65 65]
  set AscMonoEntry2(font) $ascBrowVect(font)
  set data \
    [VShowWindow.ascMonoEntry2 "Part name          Type name" \
          "$pos" "newpart" "\*"]
  if {$data == "" || [lindex $data 0] == 2} {
    return;
  }
  set partname [lindex $data 1]
  set parttype [lindex $data 2]
  set nok [Brow_CreatePart_ErrorCheck $partname $parttype]
  if {$nok} {
    return 1;
  }
  set nok [catch "createpart $partname $parttype" err]
  if {$nok} {
    return 1;
  } {
    puts "Part $partname OF $parttype Created"
    return 0;
  }
}
#-------------------------------------------------------------------------
#-------------------------------------------------------------------------
#DISPLAY CODE

#
# proc Brow_InvokePlotProgram
#-------------------------------------------------------------------------
# Accepts the name of a plot file and will invoke a plotting program on it.
#-------------------------------------------------------------------------
proc Brow_InvokePlotProgram {filename {plotcommand ""}} {
  global ascUtilVect
  if {$plotcommand != ""} {
    set plot_cmd $plotcommand
  } {
    set plot_cmd $ascUtilVect(plot_command)
    if {$plot_cmd  == ""} {return}
  }
 #exec $plot_cmd $filename
  if {[catch {eval "exec" $plot_cmd $filename &} msg]} {
    puts "Error invoking $plot_cmd $filename"
  }
}

#
# proc Brow_PrepPlotFile
#-------------------------------------------------------------------------
# Prepares and writes the plotfile based on the type of plot, which is
# currently one of plain, xgraph, gnu_plot. Valid instances are
# 'current' or 'search'.
#-------------------------------------------------------------------------
proc Brow_PrepPlotFile {plot_type {whichinst "current"}} {
  global ascUtilVect
  set username [ascwhoami]
  set file_prefix $ascUtilVect(asctmp)/asc$username
  set filename [FileUniqueName "$file_prefix.$plot_type"]

  # call the C-code to write the plotfile for the current inst.
  set res [b_prepplotfile $whichinst $filename $plot_type]
  puts "Generating plot file \"$filename\""
  return $filename;
}

#
# proc Brow_do_Plot
#-------------------------------------------------------------------------
# This is the function bound the menu item plot under mb display in the
# browser. A unique filename is created from the code FileUniqueName
# function in DisplayProc.tcl.
#-------------------------------------------------------------------------
proc Brow_do_Plot {} {
  global ascUtilVect

  # Next C-call returns 1 if we can plot.
  set can_plot [b_isplottable current];
  if {$can_plot == 0} {return}
  set plot_type $ascUtilVect(plot_type)

# This will create a filename of the form /tmp/ascka0p.xgraph6459.113525
  set filename [Brow_PrepPlotFile $plot_type]
  if {$filename == ""} {return}
  if {$ascUtilVect(plot_command) != ""} {
    Brow_InvokePlotProgram $filename $ascUtilVect(plot_command)
    HUB_Message_to_HUB PLOTMADE [Brow_get_subname] $filename
  } {error "Plot command not set in utilities window!"}
}
#
# proc Brow_do_Statistics {}
#-------------------------------------------------------------------------
# Display.Statistics button in the browser
#-------------------------------------------------------------------------
proc Brow_do_Statistics {} {
  puts stdout "Instance Statistics for [Brow_get_subname]:"
  bstatistics
  puts stdout "====================================================="
}


#
# proc Brow_do_DispAttr  {}
#-------------------------------------------------------------------------
# This is command will display the attributes associated with
# the currently focused instance if it is a variable. needs to be
# expanded.
#-------------------------------------------------------------------------
proc Brow_do_DispAttr {} {
  global ascDispVect
  set list "none"
  catch {set list [dbg_write_qlfattr [Brow_get_subname]]}
  DispClear;
  DispSetEntry "Attributes of [Brow_get_subname]"
  DispInsert $list
  if {$ascDispVect(visibility)} {newraise .display}
}

#
# proc Brow_do_HideNames
#-------------------------------------------------------------------------
# pops up a list of visible children of type (see ChildVisible, child.h,
# or maybe just "inst child") and strip off the leading goo of each)
# and makes invisible those selected.
# Children hidden are hidden in all contexts similar to the current one,
# because the hide bit is part of the typedescriptions child list.
# maybe we should have hide child and hide child in all refinements.
#-------------------------------------------------------------------------
proc Brow_do_HideNames {} {
  global ascListSelectB1Box ascBrowVect
  set list ""
  set type "[inst type]"
  set plist ""
  catch {set list [brow_child_list current all TYPE ATOMS PASSED]}
  foreach i $list {
    lappend plist "[lindex $i 0]"
  }

  if {$plist == ""} {
    Brow_Raise_Alert "Hide names called without children to hide" \
                  "Unhide error"
    return
  }
  set ascListSelectB1Box(grab) 0
  set ascListSelectB1Box(btn2name) TagAll
  set ascListSelectB1Box(btn3name) Hide
  set ascListSelectB1Box(btn4name) ""
  set ascListSelectB1Box(btn5name) ""
  set ascListSelectB1Box(btn2destroy) 0
  set ascListSelectB1Box(btn3destroy) 0
  set ascListSelectB1Box(btn4destroy) 0
  set ascListSelectB1Box(btn2command) Browser_FindSelectAll
  set ascListSelectB1Box(btn3command) Brow_HideListSelect
  set ascListSelectB1Box(title) "Hide names in $type"
  set ascListSelectB1Box(toplevelname) ".browhidename"
  set ascListSelectB1Box(selectmode) extended
  set ascListSelectB1Box(headline) "Hideable $type Parts"
  set ascBrowVect(hideparts_type) $type

  set button [AscListSelectB1Box $plist \
                  250x240[setpos .browser 150 20]]
}

proc Brow_HideListSelect {} {
  global ascListSelectB1Box ascBrowVect
  set list $ascListSelectB1Box(itemselected)
  set type $ascBrowVect(hideparts_type)
  foreach i $list {
    libr_hide_type $type $i
    Libr_recordhide add $type $i
  }
  HUB_Message_to_HUB TYPEHIDDEN
}
#
# proc Brow_do_UnHideNames {}
#-------------------------------------------------------------------------
# pops up a list of invisible children of type (see ChildVisible, child.h)
# and makes visible those selected.
#-------------------------------------------------------------------------
proc Brow_do_UnHideNames {} {
  global ascListSelectB1Box ascBrowVect
  set list ""
  set alllist ""
  set hiddenlist ""
  set type "[inst type]"
  set plist ""
  catch {set list [brow_child_list current all TYPE ATOMS PASSED]}
  foreach i $list {
    lappend plist "[lindex $i 0]"
  }
  catch {set alllist [inst child]}
  if {$alllist == "0"} {
    Brow_Raise_Alert "UnHide names called\non childless instance"  \
      "Unhide error"
    return
  }
  foreach i $alllist {
    if {[lsearch -exact $plist $i] == -1} {
      lappend hiddenlist $i
    }
  }
  if {$hiddenlist == ""} {
    Brow_Raise_Alert "UnHide names found no hidden children" \
      "Unhide message"
    return
  }

  set ascListSelectB1Box(grab) 0
  set ascListSelectB1Box(btn2name) TagAll
  set ascListSelectB1Box(btn3name) UnHide
  set ascListSelectB1Box(btn4name) ""
  set ascListSelectB1Box(btn5name) ""
  set ascListSelectB1Box(btn2destroy) 0
  set ascListSelectB1Box(btn3destroy) 0
  set ascListSelectB1Box(btn4destroy) 0
  set ascListSelectB1Box(btn2command) Browser_FindSelectAll
  set ascListSelectB1Box(btn3command) Brow_UnHideListSelect
  set ascListSelectB1Box(title) "UnHide names in $type"
  set ascListSelectB1Box(toplevelname) ".browshowname"
  set ascListSelectB1Box(selectmode) extended
  set ascListSelectB1Box(headline) "$type hidden Parts"
  set ascBrowVect(hideparts_type) $type

  set button [AscListSelectB1Box $hiddenlist \
                  250x240[setpos .browser 150 20]]
}

proc Brow_UnHideListSelect {} {
  global ascListSelectB1Box ascBrowVect
  set list $ascListSelectB1Box(itemselected)
  set type $ascBrowVect(hideparts_type)
  foreach i $list {
    libr_unhide_type $type $i
    Libr_recordhide delete $type $i
  }
  HUB_Message_to_HUB TYPEHIDDEN
}

# proc Brow_DispRelsForAtom
#-------------------------------------------------------------------------
# This actually does the work of fetching the relations associated
# with a REAL_ATOM_INST and soon to come BOOLEAN_ATOM_INSTs and stuffing
# them in the Display window.
#-------------------------------------------------------------------------
proc Brow_DispRelsForAtom {{context "current"}} {
  global ascDispVect

  set rel_list [__brow_relsforatom $context];
  if {$rel_list == ""} {
    newraise .display;
    DispClear;
    return
  }
  # clear the display box.
  DispClear;
  DispSetEntry "Relations for atom [Brow_get_subname]"
  DispInsert2 $rel_list;
  newraise .display
}


# proc Brow_DispRelations
#-------------------------------------------------------------------------
# This actually does the work of fetching the relations and stuffing them
# in the display box. Valid options are INFIX or POSFIX.
#-------------------------------------------------------------------------
proc Brow_DispRelations {{infix_or_postfix "INFIX"}} {
  global ascDispVect
  if {$infix_or_postfix == "INFIX"} {
    set rel_list [bgetrels current];
  } {
    set rel_list [bgetrelspf current];
  }
  if {$rel_list == ""} {
    newraise .display;
    DispClear;
    return
  }
  # clear the display box.
  DispClear;
  DispSetEntry "Relations in [Brow_get_subname]"
  DispInsert2 $rel_list;
  newraise .display
}


#
# proc Brow_do_DispRelations  {}
#-------------------------------------------------------------------------
# This command will display the relations associated with
# the currently focused instance. The relations will be displayed in the
# Display window. A postfix representation of the display is coded but
# not bound to the button.
#-------------------------------------------------------------------------
proc Brow_do_DispRelations {} {
  global ascBrowVect

  set inst_kind [inst kind]
  # if not model type then check for arrays of relations

  switch $inst_kind {
    {MODEL_INST} {
      Brow_DispRelations INFIX;
    }
    {REAL_ATOM_INST} {
      Brow_DispRelsForAtom current;
    }
    default {
      set is_rel_type [__brow_isrelation current]
      if {$is_rel_type} {
	Brow_DispRelations INFIX;
      }
    }
  }
}


# proc Brow_DispCondRels
#-------------------------------------------------------------------------
# This actually does the work of fetching the conditional relations and
# stuffing them in the display box. Only INFIX representation.
#-------------------------------------------------------------------------
proc Brow_DispCondRels {} {
  global ascDispVect

  set rel_list [bgetcondrels current];

  if {$rel_list == ""} {
    newraise .display;
    DispClear;
    return
  }
  # clear the display box.
  DispClear;
  DispSetEntry "Conditional Relations in [Brow_get_subname]"
  DispInsert2 $rel_list;
  newraise .display
}


#
# proc Brow_do_DispCondRels  {}
#-------------------------------------------------------------------------
# This command will display the relations associated with
# the currently focused instance. The relations will be displayed in the
# Display window.
#-------------------------------------------------------------------------
proc Brow_do_DispCondRels {} {
  global ascBrowVect

  set inst_kind [inst kind]
  # if not model type then check for arrays of relations

  switch $inst_kind {
    {MODEL_INST} {
      Brow_DispCondRels;
    }
    default {
      set is_rel_type [__brow_isrelation current]
      if {$is_rel_type} {
	Brow_DispCondRels;
      }
    }
  }
}



# proc Brow_DispLogRelsForAtom
#-------------------------------------------------------------------------
# This actually does the work of fetching the logical relations associated
# with a BOOLEAN_ATOM_INST and stuffing
# them in the Display window.
#-------------------------------------------------------------------------
proc Brow_DispLogRelsForAtom {{context "current"}} {
  global ascDispVect

  set lrel_list [__brow_logrelsforatom $context];
  if {$lrel_list == ""} {
    newraise .display;
    DispClear;
    return
  }
  # clear the display box.
  DispClear;
  DispSetEntry "Logical Relations for atom [Brow_get_subname]"
  DispInsert2 $lrel_list;
  newraise .display
}


# proc Brow_DispLogRels
#-------------------------------------------------------------------------
# This actually does the work of fetching the logical relations
# and stuffing them in the display box.
# Valid options are INFIX or POSFIX.
#-------------------------------------------------------------------------
proc Brow_DispLogRels {{infix_or_postfix "INFIX"}} {
  global ascDispVect
  if {$infix_or_postfix == "INFIX"} {
    set lrel_list [bgetlogrels current];
  } {
    set lrel_list [bgetlogrelspf current];
  }
  if {$lrel_list == ""} {
    newraise .display;
    DispClear;
    return
  }
  # clear the display box.
  DispClear;
  DispSetEntry "Logical Relations in [Brow_get_subname]"
  DispInsert2 $lrel_list;
  newraise .display
}


#
# proc Brow_do_DispLogRels  {}
#-------------------------------------------------------------------------
# This command will display the logical relations associated with
# the currently focused instance. The logical relations will be displayed
# in the Display window. A postfix representation of the display is coded
# but not bound to the button.
#-------------------------------------------------------------------------
proc Brow_do_DispLogRels {} {
  global ascBrowVect

  set inst_kind [inst kind]
  # if not model type then check for arrays of logical relations

  switch $inst_kind {
    {MODEL_INST} {
      Brow_DispLogRels INFIX;
    }
    {BOOLEAN_ATOM_INST} {
      Brow_DispLogRelsForAtom current;
    }
    default {
      set is_logrel_type [__brow_islogrel current]
      if {$is_logrel_type} {
	Brow_DispLogRels INFIX;
      }
    }
  }
}



# proc Brow_DispCondLogRels
#-------------------------------------------------------------------------
# This actually does the work of fetching the conditional logical
# relations and stuffing them in the display box.
# Only INFIX representation.
#-------------------------------------------------------------------------
proc Brow_DispCondLogRels {} {
  global ascDispVect

  set logrel_list [bgetcondlogrels current];

  if {$logrel_list == ""} {
    newraise .display;
    DispClear;
    return
  }
  # clear the display box.
  DispClear;
  DispSetEntry "Conditional Logical Relations in [Brow_get_subname]"
  DispInsert2 $logrel_list;
  newraise .display
}


#
# proc Brow_do_DispCondLogRels  {}
#-------------------------------------------------------------------------
# This command will display the logical relations associated with
# the currently focused instance. The log relations will be displayed
# in the Display window.
#-------------------------------------------------------------------------
proc Brow_do_DispCondLogRels {} {
  global ascBrowVect

  set inst_kind [inst kind]
  # if not model type then check for arrays of relations

  switch $inst_kind {
    {MODEL_INST} {
      Brow_DispCondLogRels;
    }
    default {
      set is_logrel_type [__brow_islogrel current]
      if {$is_logrel_type} {
	Brow_DispCondLogRels;
      }
    }
  }
}








# proc Brow_DispWhensForInstance
#-------------------------------------------------------------------------
# This actually do the work of fetching the whens associated
# with an instance and stuffing them in the Display window.
# Instance can be boolean, integer, symbol, relation. It will
# collect the WHENs which include such an instance in either
# the list of variables or in some of the CASEs.
#-------------------------------------------------------------------------
proc Brow_DispWhensForInstance {{context "current"}} {
  global ascDispVect

  set when_list [__brow_whensforinstance $context];
  if {$when_list == ""} {
    newraise .display;
    DispClear;
    return
  }
  # clear the display box.
  DispClear;
  DispSetEntry "Whens for instance [Brow_get_subname]"
  DispInsert2 $when_list;
  newraise .display
}


# proc Brow_DispWhens
#-------------------------------------------------------------------------
# This actually does the work of fetching the whens
# and stuffing them in the display box. It works for models, a WHEN or
# array of WHENs
#-------------------------------------------------------------------------
proc Brow_DispWhens {} {
  global ascDispVect
#
  set when_list [bgetwhens current];
  if {$when_list == ""} {
    newraise .display;
    DispClear;
    return
  }
  # clear the display box.
  DispClear;
  DispSetEntry "Whens in [Brow_get_subname]"
  DispInsert2 $when_list;
  newraise .display
}


#
# proc Brow_do_DispWhens  {}
#-------------------------------------------------------------------------
# This command will display the WHENs associated with
# the currently focused instance. The whens will be displayed
# in the Display window.
#-------------------------------------------------------------------------
proc Brow_do_DispWhens {} {
  global ascBrowVect

  # To check for model
  set inst_kind [inst kind]

  if { $inst_kind =="MODEL_INST" } {
    Brow_DispWhens;
    return
  }

  # To check for when or array of when
  set is_when_type [__brow_iswhen current]

  if {$is_when_type} {
    Brow_DispWhens;
    return
  }

  # if not model type or when then check for instance in when
  set is_instance_in_when [__brow_isinstanceinwhen current]

  if {$is_instance_in_when} {
    Brow_DispWhensForInstance;
    return
  }
}

#-------------------------------------------------------------------------
#-------------------------------------------------------------------------


# proc Brow_do_Help {}
# proc Brow_do_BindHelp {}
#-------------------------------------------------------------------------
# browser help onbrowsing, onbrowser, respectively
#-------------------------------------------------------------------------
proc Brow_do_Help {} {
  Help_button browser
}
proc Brow_do_BindHelp {} {
  Help_button browser.help onbrowser
}

#
# misc random header, unmatched by any function
#-------------------------------------------------------------------------
# proc Brow_export_to_probe
#
# defined in ProbeProc.tcl
#
# This code will be for return values from the brow_child_list or inst type
# routines of the form 'name IS_A type' or 'name = value'. It is perhaps
# easiest to retunr this compound information as a long list and to use
# the following function to strip apart. For example the ascBrowVect(children)
# needs the compound information but the ascParentsBox needs only the name.
#-------------------------------------------------------------------------

#
# proc strip {tvlist} {
#-------------------------------------------------------------------------
# remove ' [ ] from a string tvlist
#-------------------------------------------------------------------------
proc strip {tvlist} {

  regsub -all {\'} $tvlist "" b;
  regsub -all {\[} $b "" b; regsub -all {\]} $b "" b;
  return $b;
}

#
# proc stripbraces {listelement}
#-------------------------------------------------------------------------
# remove any outer { } from a string
#-------------------------------------------------------------------------
proc stripbraces {listelement} {
  set listelement [string trimleft  $listelement " \{"];
  set list [string trimright $listelement " \}"];
  return $list
}

#
# proc Brow_parse_values {filename force}
#-------------------------------------------------------------------------
# function to parse a values file. unexecutable tcl lines will be reported
# as will the total line count parsed.
#-------------------------------------------------------------------------
proc Brow_parse_values {filename force} {
  global ascBrowVect
  set ok 1
  set lc 0
  set line ""
  set errors ""
  set ec 0
  # open file
  set fileid [open $filename r]
  set cc [gets $fileid line]
  # get first line for preprocessing
  if {$cc<0} {
    close $fileid
    puts stdout "Read $lc lines from $filename."
    return
  }
  # check header for new style relative addressing
  if {[string first qlfdid $line] == 0} {
    set root [lindex $line 1]
    set label ""
    set title ""
    if {[catch {eval $line} err]} {
      set title "Read values into what object? ($root not found)"
      set label "Name:"
      set force 0
    } else {
      set title "Values to be read into:"
      set label "Object:"
    }
    global AscMonoEntry1
    set AscMonoEntry2(font) $ascBrowVect(font)
    if {$force == 0} { # confirm read-to location
      set tinst [lindex [VShowWindow.ascMonoEntry1 \
        $label "500x100[setpos .library 50 50]" \
        $root $title ""] 1]
      if {$AscMonoEntry1(button)==2} {
        close $fileid
        return
      }
      if {$AscMonoEntry1(button)==3} {
        set expl {Enter a simulation or qualified name.}
        Script_Raise_Alert $expl "$label help:"
        close $fileid
        return
      }
      if {[catch {qlfdid $tinst} msg]} {
        Script_Raise_Alert "$tinst not found!" "Undefined instance"
        close $fileid
        return
      }
    } 
    # else the qlfdid passed above in catch eval line
  } else {
    incr lc
    if {[catch $line ] && $line != ""} {
      incr ec
      lappend errors "$lc:"
      lappend errors $line
    }
  }
  while {$ok} {
    set cc [gets $fileid line]
    if {$cc<0} {break}
    incr lc
    if {[catch $line ] && $line != ""} {
      # puts stderr "Unable to parse value file line: $lc\n $line"
      if {[string first qlfdid $line] == 0} {
	puts stderr "exiting read values due to qlfdid failure\n"
	close $fileid
      }
      incr ec
      lappend errors "$lc:"
      lappend errors $line
    }
  }
  close $fileid
  puts stdout "Read $lc lines from $filename."
  if {$ec} {
    puts stderr "Errors on $ec lines."
    puts stderr "  Type \"Brow_parse_errors\" to see the lines."
  }
  set ascBrowVect(parseerrors) $errors
}

proc Brow_parse_errors {} {
  global ascBrowVect
  puts stderr "Parse errors: -------------------------------------"
  foreach i $ascBrowVect(parseerrors) {
    puts stderr $i
  }
  puts stderr "End of parse errors -------------------------------------"
}

#
# eof
#
