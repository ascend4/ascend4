#  UnitsProc.tcl: Units window Tcl code
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.25 $
#  Last modified on: $Date: 1998/06/18 15:55:05 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: UnitsProc.tcl,v $
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
# proc set_Units_Defaults {}
#------------------------------------------------------------------------
# put Units bindings, etc here
#------------------------------------------------------------------------
proc set_Units_Defaults {} {
 # puts "setting units buttons"
  global env ascUnitVect
  set ascUnitVect(windowname) .units
  set ascUnitVect(stdunits) $env(ASCENDDIST)/models/measures.a4l
  set ascUnitVect(filename) "mysi.a4u"
  set ascUnitVect(visibility) 1
  set ascUnitVect(dimnames) "M Q T L TMP C E LUM P S"
  set ascUnitVect(cascade.M)    .units.menubar.edit.basic.m0
  set ascUnitVect(cascade.Q)    .units.menubar.edit.basic.m1
  set ascUnitVect(cascade.L)    .units.menubar.edit.basic.m2
  set ascUnitVect(cascade.T)    .units.menubar.edit.basic.m3
  set ascUnitVect(cascade.TMP)  .units.menubar.edit.basic.m4
  set ascUnitVect(cascade.C)    .units.menubar.edit.basic.m5
  set ascUnitVect(cascade.E)    .units.menubar.edit.basic.m6
  set ascUnitVect(cascade.LUM)  .units.menubar.edit.basic.m7
  set ascUnitVect(cascade.P)    .units.menubar.edit.basic.m8
  set ascUnitVect(cascade.S)    .units.menubar.edit.basic.m9
  set ascUnitVect(atombox) .units.main_frm.atom_frm.listbox1
  set ascUnitVect(basicbox) .units.entry_frm.listbox1
  set ascUnitVect(unitbox) .units.main_frm.units_box.listbox1
  set ascUnitVect(entrybox) .units.entry_setunits.entry5
  set ascUnitVect(basefiletypes) {
    {{New preferred units} {.a4u} }
    {{Old preferred units} {.uasc} }
  }
  set ascUnitVect(filetypes) $ascUnitVect(basefiletypes)
  set ascUnitVect(lastextension) .a4u

  set ascUnitVect(atomname) ""
  set ascUnitVect(atomdispunits) "*"
  Units_update_DisplayBtn
  Units_do_UpdateAtomBox
  Units_updatedefaultentry
  Units_update_ViewBtn
  # bindings
  bind .units.entry_setunits.entry5 <Return> {Units_CreateUnit}
  #tk_listboxSingleSelect $ascUnitVect(atombox)
  #tk_listboxSingleSelect $ascUnitVect(unitbox)
  VPane-Bind .units.main_frm atom_frm units_box 10 0.55
  $ascUnitVect(atombox) config -exportselection 0
  bind $ascUnitVect(atombox) <B1-ButtonRelease> {
    set atom ""
    if {[catch { set atom [$ascUnitVect(atombox) get \
                 [$ascUnitVect(atombox) curselection]] } ]} {
      Units_do_UpdateAtomBox
      Units_update_DisplayBtn
    }
    if {$atom != ""} {Units_UpdateUnitBox $atom}
    set ascUnitVect(atomname) $atom
  }
  bind $ascUnitVect(unitbox) <B1-ButtonRelease> {
    set unit ""
    catch {
      set unit \
        [$ascUnitVect(unitbox) get [$ascUnitVect(unitbox) curselection]]
    }
    set ascUnitVect(atomdispunits) $unit
    if {$unit != ""} {Units_UpdateDisplayUnit $unit}
  }
  catch {u_setprec $ascUnitVect(precision)}

  ascRightMouseAddCommand $ascUnitVect(unitbox) normal \
    command -label "Close window" \
    -underline 0 -command {Toggle_Remote ascUnitVect}

  ascRightMouseAddCommand $ascUnitVect(atombox) normal \
    command -label "Close window" \
    -underline 0 -command {Toggle_Remote ascUnitVect}

}
#
# proc Units_do_Font {}
#---------------------------------------------------------------------
# font select button for window
#---------------------------------------------------------------------
proc Units_do_Font {args} {
  global ascUnitVect
  set ascUnitVect(atombox) .units.main_frm.atom_frm.listbox1
  set ascUnitVect(unitbox) .units.main_frm.units_box.listbox1
  set font ""
  if {$args == ""} {
    set font  [ascFontGet]
  } else {
    set font $args
  }
  if {"$font" == ""} {
    return;
  }
  $ascUnitVect(atombox) configure -font $font
  $ascUnitVect(unitbox) configure -font $font
  $ascUnitVect(entrybox) configure -font $font
  $ascUnitVect(basicbox) configure -font $font
  set ascUnitVect(font) [lindex [$ascUnitVect(unitbox) configure -font] 4]
}

#
# proc Units_Redraw {}
#------------------------------------------------------------------------
# repaint the units window after ascend.tcl restart
#------------------------------------------------------------------------
proc Units_Redraw {} {
  Units_updatedefaultentry
  Units_do_UpdateAtomBox
  Units_update_DisplayBtn
}

#
# proc Units_updatedefaultentry {}
#------------------------------------------------------------------------
# updates the string in the bottom entry.
#------------------------------------------------------------------------
proc Units_updatedefaultentry {} {
  global ascUnitVect
  set def [u_getbasedef]
  $ascUnitVect(basicbox) configure -selectforeground \
    [$ascUnitVect(basicbox) cget -foreground]
  $ascUnitVect(basicbox) configure -selectbackground \
    [$ascUnitVect(basicbox) cget -background]
  $ascUnitVect(basicbox) delete 0 end
  $ascUnitVect(basicbox) insert end {Basic units:}
  foreach j [u_getbasedef] {
    $ascUnitVect(basicbox) insert end $j
  }
}
#
# proc Units_setdefunit {basedim unit}
#------------------------------------------------------------------------
# sets the default display unit for basedim to be unit
#------------------------------------------------------------------------
proc Units_setdefunit {basedim unit} {
  global ascUnitVect
  u_change_baseunit $unit
  Units_updatedefaultentry
  HUB_Message_to_HUB UNITSUPDATED
}
#
# proc Units_do_AE {}
#------------------------------------------------------------------------
# sets the display base units to be american engineering, including dollar
#------------------------------------------------------------------------
proc Units_do_AE {} {
  global ascUnitVect
  catch {Libr_file_get $ascUnitVect(stdunits)} msg
  if {[catch {
               foreach i {lbm s ft R A lb_mole cd deg srad US} {
                 u_change_baseunit $i
               }
             } err]} {
    error "Using AE units requires first loading measures.a4l"
  }
  Units_updatedefaultentry
  HUB_Message_to_HUB UNITSUPDATED
}
#
# proc Units_do_CGS {}
#------------------------------------------------------------------------
# sets the display base units to be cgs (but doesn't reset currency)
#------------------------------------------------------------------------
proc Units_do_CGS {} {
  global ascUnitVect
  catch {Libr_file_get $ascUnitVect(stdunits)} msg
  if {[catch {
               foreach i {g s cm K A g_mole cd rad srad CR} {
                 u_change_baseunit $i
               }
             } err]} {
    error "Using CGS units requires first loading measures.a4l"
  }
  Units_updatedefaultentry
  HUB_Message_to_HUB UNITSUPDATED
}
#
# proc Units_do_ShowAll {}
#------------------------------------------------------------------------
# show all the units
#------------------------------------------------------------------------
proc Units_do_ShowAll {} {
  DispClear
  DispInsert "UNITS"
  set l [lsort [u_dump 2]]
  set old_d 0
  foreach i $l {
    if {[string compare [lindex $i 0] $old_d]} {
      lappend l2 ""
      if {![string length [lindex $i 4]]} {
        lappend l2 "\t (* DIMENSIONLESS *)"
      } else {
        if {[string compare [lindex $i 4] *]} {
          lappend l2 "\t (* DIMENSION [lindex $i 4] *)"
        } else {
          lappend l2 "\t (* DIMENSION WILD *)"
        }
      }
      set old_d [lindex $i 0]
    }
    lappend l2 "\t[lindex $i 1]\t =\t \{[lindex $i 2]*[lindex $i 3]\};"
  }
  DispInsert "$l2"
  DispInsert "END UNITS;"
  newraise .display
}

#
# proc Units_do_SI {}
#------------------------------------------------------------------------
# sets the display base units to be SI mks (but doesn't reset currency)
#------------------------------------------------------------------------
proc Units_do_SI {} {
  global ascUnitVect
  if {[catch {
               foreach i {kg s m K A mole cd rad srad CR} {
                 u_change_baseunit $i
               }
             } err]} {
    error "Using abbreviated SI units requires first loading measures.a4l"
  }
  Units_updatedefaultentry
  HUB_Message_to_HUB UNITSUPDATED
}
#
# proc Units_do_ReadFile {}
#------------------------------------------------------------------------
# read a units user specs file
#------------------------------------------------------------------------
proc Units_do_ReadFile {} {
  global ascUnitVect ascGlobalVect asc_tkfbox
  set defaultname "[pwd]"
  set asc_tkfbox(otherdirs) $ascGlobalVect(librarypathdirs)
  set filename [tk_getOpenFile \
    -defaultextension "" \
    -filetypes $ascUnitVect(filetypes) \
    -initialdir $defaultname \
    -parent .units \
    -title {Read preferred units file}]

  if {$filename == "" || [file isdirectory $filename]} {
    return 1;
  } {
    set newext "[file extension $filename]"
    if {[catch {source $filename} ]} {error "Problem reading $filename"}
    set ascUnitVect(filename) $filename
    set ascUnitVect(lastextension) $newext
    ascresort_filetypes ascUnitVect lastextension
    if {$ascUnitVect(visibility)} {
      newraise .units
    }
    update idletasks
  }
  Units_updatedefaultentry
  Units_UpdateUnitBox $ascUnitVect(atomname)
  HUB_Message_to_HUB UNITSUPDATED
}
#
# proc Units_do_WriteFile {}
#------------------------------------------------------------------------
# get filename and save user set and fundamental units there
#------------------------------------------------------------------------
proc Units_do_WriteFile {} {
  global ascUnitVect
  set defaultname $ascUnitVect(filename)
  set filename [tk_getSaveFile \
    -defaultextension .uasc \
    -filetypes $ascUnitVect(filetypes) \
    -initialfile $defaultname \
    -parent .units \
    -title {Save preferred units}]

  if {$filename == ""} {
    return 1;
  } {
    if {[catch {Units_Put $filename} ]} {error "Problem writing $filename"}
    set ascUnitVect(filename) $filename
    newraise .units
    update idletasks
  }
}
#
# proc Units_Put {file}
#------------------------------------------------------------------------
# write units save file
#------------------------------------------------------------------------
proc Units_Put {file} {
  if {[catch {set ufile [open $file w]} ]} {
    puts "Error writing $file"
    return
  }
  puts $ufile \
    "\# Units in this file may be changed, saved, and read at any time."
  puts $ufile \
    "\# To automatically load it, put the line"
  puts $ufile  "\# \"source $file\" in your ~/.ascendrc"
  puts $ufile \
    "\# after the line which sources ascend.tcl."
  foreach i [u_getbasedef] {
    puts $ufile "u_change_baseunit $i"
  }
  foreach i [u_get_list] {
    puts $ufile "u_set_user $i"
  }
  close $ufile
  puts stdout "wrote units data to $file"
}

#
# proc Units_HandleSourceRead {args}
#------------------------------------------------------------------------
# Update the atoms box in the units window when library changed.
#------------------------------------------------------------------------
proc Units_HandleSourceRead {args} {
  Units_do_UpdateAtomBox
  Units_update_DisplayBtn
  global ascUnitVect
  if {$ascUnitVect(atomname) != ""} {
    Units_UpdateUnitBox $ascUnitVect(atomname)
  }
}
#
# proc Units_HandleLibDestroyed {args}
#------------------------------------------------------------------------
# Update the atoms box in the units window when library deleted.
#------------------------------------------------------------------------
proc Units_HandleLibDestroyed {args} {
  Units_do_UpdateAtomBox
  global ascUnitVect
  set ascUnitVect(atomname) ""
  ascclearlist $ascUnitVect(unitbox)
}

#
# Units_ChangePrecision {}
#------------------------------------------------------------------------
# notify the hub of precision change
#------------------------------------------------------------------------
proc Units_ChangePrecision {} {
  HUB_Message_to_HUB UNITSUPDATED
}
#
# proc Units_do_Precision {}
#------------------------------------------------------------------------
# set the display precision with a slider
#------------------------------------------------------------------------
proc Units_do_Precision {} {
  global ascUnitVect
  ascPopSlide popslide [setpos .units 40 40] \
    4 16 "Display Precision" \
    Units_ChangePrecision \
    [u_getprec] u_setprec
}

#
# proc Units_UpdateDisplayUnit {unit}
#------------------------------------------------------------------------
# set the internal display unit for objects of current atom dims to be
# existing unit.
#------------------------------------------------------------------------
proc Units_UpdateDisplayUnit {unit} {
  global ascUnitVect
  if {$unit == "default"} {
    u_clear_user $ascUnitVect(atomname)
  } else {Units_CreateUnit}
  HUB_Message_to_HUB UNITSUPDATED
}
#
# proc Units_update_DisplayBtn {}
#------------------------------------------------------------------------
# this procedure reconfigures the cascade menus under the display button
# to show all simple units available.
#------------------------------------------------------------------------
proc Units_update_DisplayBtn {} {
  global ascUnitVect
  foreach dn $ascUnitVect(dimnames) {
    $ascUnitVect(cascade.$dn) delete 0 last
    set ulist [u_frombasedim [u_dim2num $dn]]
    foreach u $ulist {
      $ascUnitVect(cascade.$dn) add command \
        -label $u \
        -command "Units_setdefunit $dn $u"
    }
  }
}

#
# proc Units_update_ViewBtn {}
#------------------------------------------------------------------------
# this procedure configures the view button
#------------------------------------------------------------------------
proc Units_update_ViewBtn {} {

  global ascUnitVect 
  global ascGlobalVect

  set mb .units.menubar.view

 if {$ascGlobalVect(saveoptions) == 1} {
    $mb entryconfigure 6 -state normal
  } else {
    $mb entryconfigure 6 -state disabled
  }
}

#
# proc Units_GetAtomBoxList {}
#------------------------------------------------------------------------
# sets the list of representative atoms for dimension -> units assignment
#------------------------------------------------------------------------
proc Units_GetAtomBoxList {} {
  set udlist {}
  set ralist ""
  foreach i [u_getdimatoms] {
    if {[lsearch $udlist [lindex $i 1]] == "-1"} {
      lappend udlist [lindex $i 1]
      lappend ralist [lindex $i 0]
    }
  }
  return $ralist
}
#
# proc Units_do_UpdateAtomBox {}
#------------------------------------------------------------------------
# stuffs the atom box in the units window.
#------------------------------------------------------------------------
proc Units_do_UpdateAtomBox {} {
  global ascUnitVect
  set ascUnitVect(atomlist) [Units_GetAtomBoxList]
  ascclearlist $ascUnitVect(atombox)
  foreach i $ascUnitVect(atomlist) {
    $ascUnitVect(atombox) insert end $i
  }
}

#
# proc Units_GetUnitBoxList {atom}
#------------------------------------------------------------------------
# sets the list of representative atoms for dimension -> units assignment
#------------------------------------------------------------------------
proc Units_GetUnitBoxList {{atom "solver_var"}} {
  set rulist ""
  catch {set rulist "default [u_fromatomdim $atom]"}
  return $rulist
}
#
# proc Units_UpdateUnitBox {atom}
#------------------------------------------------------------------------
# stuffs the unit box in the units window with units match dim of atom
#------------------------------------------------------------------------
proc Units_UpdateUnitBox {atom} {
  global ascUnitVect
  ascclearlist $ascUnitVect(unitbox)
  set ascUnitVect(atomdispunits) ""
  if {$atom!=""} {
    set ascUnitVect(unitlist) [Units_GetUnitBoxList $atom]
    foreach i $ascUnitVect(unitlist) {
      $ascUnitVect(unitbox) insert end $i
    }
    set ascUnitVect(atomdispunits) [u_get_units $atom]
    set i [lsearch $ascUnitVect(unitlist) [u_get_user $atom]]
    $ascUnitVect(unitbox) select set $i
  }
}

#
# proc Units_CreateUnit {}
#------------------------------------------------------------------------
# attempts to create a unit definition (from existing ones) and set the
# matching dimensionality to have that user defined unit set.
#------------------------------------------------------------------------
proc Units_CreateUnit {} {
  global ascUnitVect
  set units $ascUnitVect(atomdispunits)
  regsub -all { } $units "" units
  if {[catch {u_set_user $units} ]} {
    Units_UpdateUnitBox $ascUnitVect(atomname)
    error "Unable to parse user specified units. Check spelling."
  } else {
    catch {u_change_baseunit $units}
  }
  Units_UpdateUnitBox $ascUnitVect(atomname)
  HUB_Message_to_HUB UNITSUPDATED
}
#
# proc Units_do_Help {}
# proc Units_do_BindHelp {}
#------------------------------------------------------------------------
# units help button calls
#------------------------------------------------------------------------
proc Units_do_Help {} {
  Help_button units
}
proc Units_do_BindHelp {} {
  Help_button units.help.onunits
}
