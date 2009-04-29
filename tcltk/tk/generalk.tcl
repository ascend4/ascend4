#  generalk.tcl
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.29 $
#  Last modified on: $Date: 1998/06/18 15:55:25 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: generalk.tcl,v $
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

# Module: generalk.tcl
# Tcl version: 7.1 (Tcl/Tk/XF)
# Tk version: 3.4
# XF version: 2.2
#
#

# module contents
global moduleList
global autoLoadList
 #warning: THIS PROCEDURE LIST IS OUT OF DATE. XF MAY FREAK!
set moduleList(generalk.tcl) {
  Alias
  GetSelection
  MenuPopupAdd
  MenuPopupMotion
  MenuPopupPost
  MenuPopupRelease
  NoFunction
  OptionButtonGet
  OptionButtonSet
  SN
  SymbolicName
  Unalias
}
set autoLoadList(generalk.tcl) {0}



# User defined procedures

#
# proc listgetgeom {wlist} {
#------------------------------------------------------------------------
# proc to list the geometries of the windows in wlist
# windows not existing are ignored
# does not note gridded windows properly
#------------------------------------------------------------------------
proc listgetgeom {wlist} {
  set glist ""
  foreach w $wlist {catch { lappend glist "$w [winfo geometry $w]" } }
  return $glist
}

#
# proc listsetgeom {glist}
#------------------------------------------------------------------------
# proc to set the geometries of the windows in glist
# messed up entries are ignored, probably
# doesnot handle gridded windows properly
#------------------------------------------------------------------------
proc listsetgeom {glist} {
  foreach g $glist {
    catch {set win [lindex [split $g] 0]}
    catch {set geom [lindex [split $g] 1]}
    catch {wm geometry $win $geom}
  }
}

proc listbindings {{n ""} {conly ""} {match ""}} {
  if {$n==""} {
    puts stderr "listbindings w      : bindings on class and widget w"
    puts stderr "listbindings w -noc : bindings widget w"
    puts stderr "listbindings c      : bindings class c"
    puts stderr "listbindings c -match pat : c events string matching pat"
    puts stderr "listbindings c -body pat : c actions string matching pat"
    return
  }
  set class Crapola
  set p ""
  if {[winfo exists $n]} {
    set class [winfo class $n]
    set p $n
  } else {
    set class $n
  }
  if {$conly == ""} {
    puts "Class bindings for $class:"
    foreach i [bind $class] {
      puts "bind $class $i \{\n[bind $class $i]\}"
    }
  }
  if { $conly == "-match"} {
    puts "Class bindings for $class matching event $match:"
    foreach i [bind $class] {
      if {[string match $match $i]} {
        puts "bind $class $i \{\n[bind $class $i]\}"
      }
    }
  }
  if { $conly == "-body"} {
    puts "Class bindings for $class matching action $conly:"
    foreach i [bind $class] {
      if {[string match $match [bind $class $i]]} {
        puts "bind $class $i \{\n[bind $class $i]\}"
      }
    }
  }
  if {$p != ""} {
    puts "Bindings for widget $p:"
      foreach i [bind $p] {
        puts "bind $p $i \{\n[bind $p $i]\}"
      }
  }
  puts "listbindings $n $conly $match"
}
#
# proc attr {w}
#------------------------------------------------------------------------
# proc to list all attributes of a given widget
#------------------------------------------------------------------------
proc attr {w} {
  foreach i [lsort [$w configure]] {
    puts $i
  }
}

# proc Table_calc_column_width {table column}
#------------------------------------------------------------------------
# returns the maximum characters in a column of a table for
# a table widget
#------------------------------------------------------------------------
proc Table_calc_column_width {table column} {
  if {![winfo exists $table]} {return 0}
  set ap [$table cget -variable]
  upvar #0 $ap PA
  global $ap
  set maxwidth 0
  set cindex "*,$column"
  foreach i [array names PA] {
    if {[string match $cindex $i]} {
      set w [string length $PA($i)]
      if {$w > $maxwidth} {
        set maxwidth $w
      }
    }
  }
  return $maxwidth
}

#
# proc wich {w}
#------------------------------------------------------------------------
# proc to list all the children of a given window
#------------------------------------------------------------------------
proc wich {w} {
  set children [winfo children $w]
  foreach child $children {
    puts $child
    wich $child
  }
}

#
# proc witchhunt {w}
#------------------------------------------------------------------------
# proc to list all the children of a given window that export selection.
#------------------------------------------------------------------------
proc witchhunt {{w .}} {
  set children [winfo children $w]
  catch {  if {[lindex [$w configure -exportselection] 4]} {
             puts "[winfo class $w] $w exports selection"
           }
        }
  foreach child $children {
    witchhunt $child
  }
}

#----------------------------------------------------------------------------
# returns the calling procedure name. This was directly grabbed from the
# EDRC SEED project.
#----------------------------------------------------------------------------
proc procName {} {
  set depth [expr {[info level]-1}]
  return [lindex [info level $depth] 0]
}

#
# proc menu_disable_all {m}
#-------------------------------------------------------------------------
# disable all entries on a menu widget of any length
#-------------------------------------------------------------------------
proc menu_disable_all {m} {
  set l [$m index last]
  if {$l=="none"} {return}
  for {set i 0} {$i <= $l} {incr i} {
    catch {$m entryconfigure $i -state disabled}
  }
  # separators normally cause an error
}


#####################

#
# proc ascclearlist {listWidget}
#------------------------------------------------------------------------
# empty a listbox
#------------------------------------------------------------------------
proc ascclearlist {listWidget} {
  if {[$listWidget size] > 0} {
    $listWidget delete 0 end;
  }
}

#
# proc delete_list_item {list item}
#------------------------------------------------------------------------
# finds first occurence of item in list, and
# returns a new version of the list without that item.
# if item is not found, returns original list.
#------------------------------------------------------------------------
proc delete_list_item {l i} {
  set p [lsearch -exact $l $i]
  if {$p == -1} {
    return $l
  }
  return [lreplace $l $p $p]
}
#
# proc updatelist {mlist w}
#------------------------------------------------------------------------
# stuff each of the items in mlist into list widget in order
#------------------------------------------------------------------------
proc updatelist {mlist w} {# general list update
  foreach i $mlist {
    $w insert end $i;
  }
}

global tcl_platform tk_version
if {$tk_version >= 8.0 && "$tcl_platform(platform)"!="unix"}  {
#
#-----------------------------------------------------------------------
# Bare bones ls hack for pcs who are too stupid to know better.
#-----------------------------------------------------------------------
proc ls {args} {
  set pattern "__nopattern"
  set outstyle 0
  foreach i $args {
    # parse switches
    if {"[string index $i 0]" == "-"} {
      set len [string len $i]
      for {set  c 1} {$c < $len} {incr c} {
        set opt "[string index $i $c]"
        if {"$opt" == "l"} {
          set outstyle 1
        } else {
          puts "option $opt ignored"
        }
      }
    } else {
      if {"$pattern" == "__nopattern"} {
        set pattern $i
      } else {
        append pattern " $i"
      }
    }
  }
  if {"$pattern" == "__nopattern"} {
    set pattern "*"
  }
  set flist [lsort [glob $pattern]]
  set maxlen 0
  set listlen 0
  foreach i $flist {
    if {[string length $i] > $maxlen} {
      set maxlen [string length $i]
    }
    incr listlen
  }
  set cols [expr 80/($maxlen +4)]
  set field [expr 80/$cols]
  if {"$outstyle" != "0"} {
    foreach i $flist {
      if {[file isdirectory $i]} {
        puts stdout "$i/"
      } else {
        puts stdout "$i"
      }
    }
  } else {
    set r 0
    set c 0
    foreach i $flist {
      if {[file isdirectory $i]} {
        puts -nonewline stdout [format "%-${field}s" "$i/"]
      } else {
        puts -nonewline stdout [format "%-${field}s" $i]
      }
      incr c
      if {$c == $cols} {
        puts stdout ""
        set c 0
      }
    }
    if {$c} {
      puts stdout ""
    }
  }
}
}
#end if tkversion for ls proc

#
# proc do_raise_lower {w}
#------------------------------------------------------------------------
# toggle the iconicness of a window
#------------------------------------------------------------------------
proc do_raise_lower {w} {

  if {[winfo exists $w]} {
    if {[winfo ismapped $w]} {
      wm withdraw $w;
    } else {
      wm deiconify $w;
      raise $w
    }
  } else {
    return 1;
  }
}

#
# proc do_raise {w}
#------------------------------------------------------------------------
# deiconify w if it exists and is iconified
#------------------------------------------------------------------------
proc do_raise {w} {
  if {[winfo exists $w]} {
    if {![winfo ismapped $w]} {
      wm deiconify $w;
    }
  }
}

#
# proc d_dumpary {ary}
#----------------------------------------------------------------------------
# utility routine for dumping an alphabetized array vector . baa 1-94       #
#----------------------------------------------------------------------------
proc d_dumpary {ary} {
  parray $ary
}

#
# proc d_dumpproclist {lst}
#----------------------------------------------------------------------------
# utility to dump a list of procedure names and their associated args       #
# alphabetizes                                                              #
#----------------------------------------------------------------------------
proc d_dumpproclist {lst} {
  set tmp [lsort $lst]
  foreach i $tmp {
    puts "$i {[info args $i]}"
  }
}

#
# proc d_dumplist {lst}
#----------------------------------------------------------------------------
# utility to dump a list alphabetically                                     #
#----------------------------------------------------------------------------
proc d_dumplist {lst} {
  set tmp [lsort $lst]
  foreach i $tmp {
    puts "$i"
  }
}

#
# proc d_dumpfile {out filename}
#-----------------------------------------------------------------------
# dump a text file. out is assumed open and writable
# filename is assumed readable.
#-----------------------------------------------------------------------
proc d_dumpfile {out filename} {
  set fid [open $filename r]
  set blob [read $fid]
  close $fid
  puts $out "\n$blob"
}

#
# ascPopSlide {{name "ascpop"} {geometry "100x50+%X+%Y"} \
#     {from "0"} {to "10"} {label ""} {okcommand ""} {value "0"}
#     {setcommand "puts"} {orient "horizontal"}}
# by Ben Allan April 25 1994.
#------------------------------------------------------------------------
# popup slider for a number input.  Grabs application.
# configs:
# AscPopSlide(fg) AscPopSlide(bg) AscPopSlide(font)
# AscPopSlide(afg) AscPopSlide(abg)
global AscPopSlide
set AscPopSlide(fg) black
set AscPopSlide(bg) white
set AscPopSlide(abg) black
set AscPopSlide(afg) white
set AscPopSlide(font) -*-*
#------------------------------------------------------------------------
proc ascPopSlide {{name "ascpop"} {geometry ""} \
               {from "0"} {to "10"} {label "Value"} {okcommand ""} \
               {value "0"} {setcommand "puts"}  \
               {orient "horizontal"}} {

  global AscPopSlide
  if {[winfo exists .$name]} {destroy .$name}
  toplevel .$name
  wm geometry .$name $geometry
  wm title .$name "$label"
  wm maxsize .$name 400 1000
  # make slider
  scale .$name.slide \
    -command $setcommand \
    -from $from \
    -label $label \
    -orient $orient \
    -font $AscPopSlide(font) \
    -to $to
  .$name.slide set $value
  button .$name.ok_btn \
    -text "OK" \
    -font $AscPopSlide(font) \
    -width [string length $label] \
    -command "$okcommand; grab release .$name; destroy .$name"

  # pack widget .$name
  pack append .$name \
    .$name.slide {top frame center expand fill} \
    .$name.ok_btn {top frame center fillx}
  grab .$name
  update idletasks
}

proc ascPushText {str} {
  global ascStackText
  set ascStackText($ascStackText(len)) $str
  incr ascStackText(len)
}
proc ascPopText {} {
  global ascStackText
  if {!$ascStackText(len)} { return ""}
  incr ascStackText(len) -1
  return $ascStackText($ascStackText(len))
}

global ascStackText
set ascStackText(len) 0
ascPushText ""

# proc emacs-bind {textwidgetname}
# These bindings rely on the tk8 binding model that widget bindings
# get called before class bindings do. With this assumption, we
# capture the text to be copied/deleted in a stack.
# These do not require widgets to export selection to paste between
# text boxes. These are not class bindings, but they assume the standard
# class bindings.
# ^k kill to eol
# ^w kill selection
# meta-w copy selection
# ^y paste previous copy/kill, but grouping sequences not supported.
# meta-y replace last paste with previous kill, and queue 'last paste'
# at the back not supported
proc emacs-bind {textw} {
  global tcl_platform
  if {$tcl_platform(platform) != "unix"} { return }
  # buffer the delete to eol
  bind $textw <Control-Key-k> {
    if !$tk_strictMotif {
       if [%W compare insert != {insert lineend}] {
           ascPushText [%W get insert {insert lineend}]
       }
    }
  }
  # buffer the selection
  bind $textw <Meta-Key-w> {
    if !$tk_strictMotif {
      if {[%W tag nextrange sel 1.0 end] != ""} {
        ascPushText [%W get sel.first sel.last]
      }
    }
  }
  # buffer the selection, and delete it. Interaction with multiple selection?
  bind $textw <Control-Key-w> {
    if !$tk_strictMotif {
      if {[%W tag nextrange sel 1.0 end] != ""} {
        ascPushText [%W get sel.first sel.last]
        %W delete sel.first sel.last
      }
    }
  }
  # insert from buffer
  bind $textw <Control-Key-y> {
    if !$tk_strictMotif {
      set old [%W index insert]
      %W insert insert [ascPopText]
      %W mark set insert $old
    }
  }
}
#------------------------------------------------------------------------



#------------------------------------------------------------------------
# PROCEDURES SWIPED WHOLESALE FROM TK/XF
#------------------------------------------------------------------------
# Procedure: OptionButtonGet
proc OptionButtonGet { widget} {

  if {"[winfo class $widget.value]" == "Label"} {
    return [lindex [$widget.value config -text] 4]
  } {
    if {"[winfo class $widget.value]" == "Entry"} {
      return [$widget.value get]
    }
  }
}


# Procedure: OptionButtonSet
proc OptionButtonSet { widget} {

  if {"[winfo class $widget.value]" == "Label"} {
    $widget.value config \
      -text [lindex
	     [$widget.menubutton2.m entryconfig
	      [$widget.menubutton2.m index active] -label] 4]
  } {
    if {"[winfo class $widget.value]" == "Entry"} {
      $widget.value delete 0 end
      $widget.value insert 0 [lindex
			      [$widget.menubutton2.m entryconfig
			       [$widget.menubutton2.m index active] -label] 4]
    }
  }
}


# Internal procedures


# Procedure: Alias
if {"[info procs Alias]" == ""} {
proc Alias { args} {
# xf ignore me 7
#------------------------------------------------------------------------
# Procedure: Alias
# Description: establish an alias for a procedure
# Arguments: args - no argument means that a list of all aliases
#                   is returned. Otherwise the first parameter is
#                   the alias name, and the second parameter is
#                   the procedure that is aliased.
# Returns: nothing, the command that is bound to the alias or a
#          list of all aliases - command pairs.
# Sideeffects: internalAliasList is updated, and the alias
#              proc is inserted
#------------------------------------------------------------------------
  global internalAliasList

  if {[llength $args] == 0} {
    return $internalAliasList
  } {
    if {[llength $args] == 1} {
      set xfTmpIndex [lsearch $internalAliasList "[lindex $args 0] *"]
      if {$xfTmpIndex != -1} {
        return [lindex [lindex $internalAliasList $xfTmpIndex] 1]
      }
    } {
      if {[llength $args] == 2} {
        eval "proc [lindex $args 0] {args} {#xf ignore me 4
return \[eval \"[lindex $args 1] \$args\"\]}"
        set xfTmpIndex [lsearch $internalAliasList "[lindex $args 0] *"]
        if {$xfTmpIndex != -1} {
          set internalAliasList [lreplace $internalAliasList $xfTmpIndex $xfTmpIndex "[lindex $args 0] [lindex $args 1]"]
        } {
          lappend internalAliasList "[lindex $args 0] [lindex $args 1]"
        }
      } {
        error "Alias: wrong number or args: $args"
      }
    }
  }
}
}


# Procedure: GetSelection
if {"[info procs GetSelection]" == ""} {
proc GetSelection {} {
# xf ignore me 7
#------------------------------------------------------------------------
# Procedure: GetSelection
# Description: get current selection
# Arguments: none
# Returns: none
# Sideeffects: none
#------------------------------------------------------------------------
  set xfSelection ""
  catch "selection get" xfSelection
  if {"$xfSelection" == "selection doesn't exist or form \"STRING\" not defined"} {
    return ""
  } {
    return $xfSelection
  }
}
}


#------------------------------------------------------------------------
# Procedure: MenuPopupAdd
if {"[info procs MenuPopupAdd]" == ""} {
proc MenuPopupAdd { xfW xfButton xfMenu {xfModifier ""} {xfCanvasTag ""}} {

# xf ignore me 7
# the popup menu handling is from (I already gave up with popup handling :-):
#
# Copyright 1991,1992 by James Noble.
# Everyone is granted permission to copy, modify and redistribute.
# This notice must be preserved on all copies or derivates.
#
##########
# Procedure: MenuPopupAdd
# Description: attach a popup menu to widget
# Arguments: xfW - the widget
#            xfButton - the button we use
#            xfMenu - the menu to attach
#            {xfModifier} - a optional modifier
#            {xfCanvasTag} - a canvas tagOrId
# Returns: none
# Sideeffects: none
#------------------------------------------------------------------------
  ###global tk_popupPriv

  set tk_popupPriv($xfMenu,focus) ""
  set tk_popupPriv($xfMenu,grab) ""
  if {"$xfModifier" != ""} {
    set press "$xfModifier-"
    set motion "$xfModifier-"
    set release "Any-"
  } {
    set press ""
    set motion ""
    set release ""
  }

  bind $xfMenu "<${motion}B${xfButton}-Motion>"  "MenuPopupMotion $xfMenu %W %X %Y"
  bind $xfMenu "<${release}ButtonRelease-${xfButton}>"  "MenuPopupRelease $xfMenu %W"
  if {"$xfCanvasTag" == ""} {
    bind $xfW "<${press}ButtonPress-${xfButton}>"  "MenuPopupPost $xfMenu %X %Y"
    bind $xfW "<${release}ButtonRelease-${xfButton}>"  "MenuPopupRelease $xfMenu %W"
  } {
    $xfW bind $xfCanvasTag "<${press}ButtonPress-${xfButton}>"  "MenuPopupPost $xfMenu %X %Y"
    $xfW bind $xfCanvasTag "<${release}ButtonRelease-${xfButton}>"  "MenuPopupRelease $xfMenu %W"
  }
}
}


# Procedure: MenuPopupMotion
if {"[info procs MenuPopupMotion]" == ""} {
proc MenuPopupMotion { xfMenu xfW xfX xfY} {
# xf ignore me 7
#------------------------------------------------------------------------
# Procedure: MenuPopupMotion
# Description: handle the popup menu motion
# Arguments: xfMenu - the topmost menu
#            xfW - the menu
#            xfX - the root x coordinate
#            xfY - the root x coordinate
# Returns: none
# Sideeffects: none
#------------------------------------------------------------------------
  global tk_popupPriv

  if {"[info commands $xfW]" != "" && [winfo ismapped $xfW] &&
      "[winfo class $xfW]" == "Menu" &&
      [info exists tk_popupPriv($xfMenu,focus)] &&
      "$tk_popupPriv($xfMenu,focus)" != "" &&
      [info exists tk_popupPriv($xfMenu,grab)] &&
      "$tk_popupPriv($xfMenu,grab)" != ""} {
    set xfPopMinX [winfo rootx $xfW]
    set xfPopMaxX [expr $xfPopMinX+[winfo width $xfW]]
    if {$xfX >= $xfPopMinX && $xfX <= $xfPopMaxX} {
      $xfW activate @[expr $xfY-[winfo rooty $xfW]]
      if {![catch "$xfW entryconfig @[expr $xfY-[winfo rooty $xfW]] -menu" result]} {
        if {"[lindex $result 4]" != ""} {
          foreach binding [bind $xfMenu] {
            bind [lindex $result 4] $binding [bind $xfMenu $binding]
          }
        }
      }
    } {
      $xfW activate none
    }
  }
}
}


#------------------------------------------------------------------------
# Procedure: MenuPopupPost
if {"[info procs MenuPopupPost]" == ""} {
proc MenuPopupPost { xfMenu xfX xfY} {
# xf ignore me 7
##########
# Procedure: MenuPopupPost
# Description: post the popup menu
# Arguments: xfMenu - the menu
#            xfX - the root x coordinate
#            xfY - the root x coordinate
# Returns: none
# Sideeffects: none
#------------------------------------------------------------------------
  global tk_popupPriv

  if {"[info commands $xfMenu]" != ""} {
    if {![info exists tk_popupPriv($xfMenu,focus)]} {
      set tk_popupPriv($xfMenu,focus) [focus]
    } {
      if {"$tk_popupPriv($xfMenu,focus)" == ""} {
        set tk_popupPriv($xfMenu,focus) [focus]
      }
    }
    set tk_popupPriv($xfMenu,grab) $xfMenu

    catch "$xfMenu activate none"
    catch "$xfMenu post $xfX $xfY"
    catch "focus $xfMenu"
    catch "grab -global $xfMenu"
  }
}
}


#------------------------------------------------------------------------
# Procedure: MenuPopupRelease
if {"[info procs MenuPopupRelease]" == ""} {
proc MenuPopupRelease { xfMenu xfW} {
# xf ignore me 7
##########
# Procedure: MenuPopupRelease
# Description: remove the popup menu
# Arguments: xfMenu - the topmost menu widget
#            xfW - the menu widget
# Returns: none
# Sideeffects: none
#------------------------------------------------------------------------
  global tk_popupPriv
  global tk_version

  if {"[info commands $xfW]" != "" && [winfo ismapped $xfW] &&
      "[winfo class $xfW]" == "Menu" &&
      [info exists tk_popupPriv($xfMenu,focus)] &&
      "$tk_popupPriv($xfMenu,focus)" != "" &&
      [info exists tk_popupPriv($xfMenu,grab)] &&
      "$tk_popupPriv($xfMenu,grab)" != ""} {
    if {$tk_version >= 3.0} {
      catch "grab release $tk_popupPriv($xfMenu,grab)"
    } {
      catch "grab none"
    }
    catch "focus $tk_popupPriv($xfMenu,focus)"
    set tk_popupPriv($xfMenu,focus) ""
    set tk_popupPriv($xfMenu,grab) ""
    if {"[$xfW index active]" != "none"} {
      $xfW invoke active; catch "$xfMenu unpost"
    }
  }
  catch "$xfMenu unpost"
}
}


#------------------------------------------------------------------------
# Procedure: NoFunction
if {"[info procs NoFunction]" == ""} {
proc NoFunction { args} {
# xf ignore me 7
##########
# Procedure: NoFunction
# Description: do nothing (especially with scales and scrollbars)
# Arguments: args - a number of ignored parameters
# Returns: none
# Sideeffects: none
#------------------------------------------------------------------------
}
}


#------------------------------------------------------------------------
# Procedure: SN
if {"[info procs SN]" == ""} {
proc SN { {xfName ""}} {
# xf ignore me 7
##########
# Procedure: SN
# Description: map a symbolic name to the widget path
# Arguments: xfName
# Returns: the symbolic name
# Sideeffects: none
#------------------------------------------------------------------------

  SymbolicName $xfName
}
}


#------------------------------------------------------------------------
# Procedure: SymbolicName
if {"[info procs SymbolicName]" == ""} {
proc SymbolicName { {xfName ""}} {
# xf ignore me 7
##########
# Procedure: SymbolicName
# Description: map a symbolic name to the widget path
# Arguments: xfName
# Returns: the symbolic name
# Sideeffects: none
#------------------------------------------------------------------------

  global symbolicName

  if {"$xfName" != ""} {
    set xfArrayName ""
    append xfArrayName symbolicName ( $xfName )
    if {![catch "set \"$xfArrayName\"" xfValue]} {
      return $xfValue
    } {
      if {"[info commands XFProcError]" != ""} {
        XFProcError "Unknown symbolic name:\n$xfName"
      } {
        puts stderr "XF error: unknown symbolic name:\n$xfName"
      }
    }
  }
  return ""
}
}


#------------------------------------------------------------------------
# Procedure: Unalias
if {"[info procs Unalias]" == ""} {
proc Unalias { aliasName} {
# xf ignore me 7
##########
# Procedure: Unalias
# Description: remove an alias for a procedure
# Arguments: aliasName - the alias name to remove
# Returns: none
# Sideeffects: internalAliasList is updated, and the alias
#              proc is removed
#------------------------------------------------------------------------
  global internalAliasList

  set xfIndex [lsearch $internalAliasList "$aliasName *"]
  if {$xfIndex != -1} {
    rename $aliasName ""
    set internalAliasList [lreplace $internalAliasList $xfIndex $xfIndex]
  }
}
}

# eof for xf goo
#

# Generic right mouse button popup code
# for ascend iv
# By Ben Allan
# 4/16/98.
# Copyright 1998 Carnegie Mellon University

# the purpose of this is to make the menu go away
# while you hold down the button but only after you
# move the pointer outside the menu and after a delay
# so the slow wristed don't shoot.
# tk's MODEL of popups in unix is that they disappear
# after any kind of button event in any window (global
# focus and grab). This may be motif madness and may not
# be TRUE on windoze.
global ascPopdata
set ascPopdata(delay) 1000

# 
#- widget is something largish, typically text or
# listbox, possibly frame, rarely a toplevel.
#- enabler is a function that returns normal or disabled
# as desired. if it returns an error or anything else or is
# not provided, the menu item will be disabled.
# If no function evaluation is necessary for proper use, "normal" and "disabled"
# may also be given for enabler.
# enabler functions can be entire scripts in braces.
# enabler functions will find %W %x and %y of the button press event
# that posted the menu stored in global array ascPopInfo should location
# information be needed. These are stored as pW, px, py respectively.
# The name of the popup menu is stored in ascPopInfo(menu) and the
# index of the entry being checked with the call to enabler is
# in ascPopInfo(index)
#- kind IS_A menu item type (separator, command, checkbutton, etc)
# and args is all the normal arguments to entryconfigure
# for a menu item.
#
# warning: handling cascades with this is messy. See BrowswerProc.tcl
# for and example of how to bind the cascade.
proc ascRightMouseAddCommand {widget enabler {kind command} args} {
  global ascPopdata tk_version
  set b $widget.childpop
  # create if first entry
  if {![winfo exists $b]} {
    # build widget $widget.childpop
    menu $b \
      -tearoffcommand [string toupper $b] \
      -tearoff 0

    # make it go away when user leaves it for more than half a second
    set ascPopdata($b.in) 0
    switch $tk_version {
	8.4 -
	8.5 {
    bind $b <Leave> "
      set ascPopdata($b.in) 0
      set ascPopdata($b.id) \[after \$ascPopdata(delay) \{if \{!\$ascPopdata($b.in)\} \{ tk::MenuUnpost $b \} \}\]
    "
	}
	default {
    bind $b <Leave> "
      set ascPopdata($b.in) 0
      set ascPopdata($b.id) \[after \$ascPopdata(delay) \{if \{!\$ascPopdata($b.in)\} \{ tkMenuUnpost $b \} \}\]
    "
	}
    }
    bind $b <Any-Enter> "
      set ascPopdata($b.in) 1
      catch \{after cancel \$ascPopdata($b.id)\}
      ascRightMouseUpdateButtons $b %W %x %y
    "
    bind $widget <ButtonPress-3> "+
      ascRightMouseUpdateButtons $b %W %x %y
      tk_popup $b %X %Y
    "
  }
  # add the entry to the widget
  set cmd $b
  append cmd " add $kind "
  append cmd $args
  eval $cmd
  # add the command enabler
  set n [$b index last]
  set ascPopdata($b.enabler.$n) $enabler
}

# updates the state of menuentries of b 
# according to their enablers.
# not yet very robust
proc ascRightMouseUpdateButtons {b pW px py} {
  global ascPopdata ascPopInfo
  set ascPopInfo(pW) $pW
  set ascPopInfo(px) $px
  set ascPopInfo(py) $py
  set ascPopInfo(menu) $b
  set n [$b index last]
  for {set e 0} {$e <= $n} {incr e} {
    set ascPopInfo(index) $e
    switch $ascPopdata($b.enabler.$e) {
    normal {
        catch {$b entryconfigure $e -state normal}
      }
    disabled {
        catch {$b entryconfigure $e -state disabled}
      }
    default {
        set st disabled
        catch {set st [$ascPopdata($b.enabler.$e)]} err
        switch $st {
        normal {
            catch {$b entryconfigure $e -state normal}
          }
        default {
            catch {$b entryconfigure $e -state disabled}
          }
        }
      }
    }
  }
}

