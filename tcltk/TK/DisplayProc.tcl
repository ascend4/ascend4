#  DisplayProc.tcl: Tcl Code for Display window
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.44 $
#  Last modified on: $Date: 1998/06/18 15:54:43 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: DisplayProc.tcl,v $
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
# proc set_Display_Defaults {}
#----------------------------------------------------------------------
# Display startup setting
#----------------------------------------------------------------------
proc set_Display_Defaults {} {

 # puts "setting Display buttons"

  global ascDispVect
  # used for controlling indentation in the Display
  set ascDispVect(indent) 0

  set ascDispVect(windowname) .display
  set ascDispVect(ShowComments) 1
  set ascDispVect(entry) ""
  set ascDispVect(textBox) .display.main_frm.display_box.text2
  set ascDispVect(entryBox) .display.entry_frm.entry_box
  # set ptr
  .display configure -cursor left_ptr
  Configure_Display;

  # Update Enabled/Disabled entries when a menu is posted
  #
  .display.menubar.view configure \
     -postcommand Disp_Update_View_Buttons
}

#
# Configure_Display {}
#----------------------------------------------------------------------
# set more stuff
#----------------------------------------------------------------------
proc Configure_Display {} {
  global ascDispVect
  $ascDispVect(textBox) configure -font $ascDispVect(font);
  catch {emacs-bind $ascDispVect(textBox)}
  bind $ascDispVect(textBox) <F2> Disp_do_Cut
  bind $ascDispVect(textBox) <F3> Disp_do_Copy
  bind $ascDispVect(entryBox) <F3> Disp_do_CopyEntry
  bind $ascDispVect(textBox) <F4> Disp_do_Paste
}

proc Disp_do_CopyEntry {} {
  global ascDispVect
  asc_export_selection $ascDispVect(entryBox)
  event generate $ascDispVect(entryBox) <<Copy>>
}

proc Disp_do_Copy {} {
  global ascDispVect
  asc_export_selection $ascDispVect(textBox)
  event generate $ascDispVect(textBox) <<Copy>>
}

proc Disp_do_Paste {} {
  global ascDispVect
  event generate $ascDispVect(textBox) <<Paste>>
}

proc Disp_do_Cut {} {
  global ascDispVect
  event generate $ascDispVect(textBox) <<Cut>>
}

#
# proc DispSetEntry {line}
#----------------------------------------------------------------------
# set the subtitle of the display window to $line
#----------------------------------------------------------------------
proc DispSetEntry {line} {
  global ascDispVect
  set ascDispVect(entry) $line
}

proc Disp_do_SaveOptions {} {
  View_Save_Window_Options display
}

#
# proc Disp_Update_View_Buttons {}
#----------------------------------------------------------------------
# Configure state of the buttons in the view menu
#---------------------------------------------------------------------
proc Disp_Update_View_Buttons {} {
  # save option item
  global ascGlobalVect

  set mb .display.menubar.view

  if {$ascGlobalVect(saveoptions) == 0} {
    $mb entryconfigure 3 -state disabled
  } else {
    $mb entryconfigure 3 -state normal
  }
}

#
# proc DispClear {}
#----------------------------------------------------------------------
# empty DispTextBox
#----------------------------------------------------------------------
proc DispClear {} {
  global ascDispVect
  $ascDispVect(textBox) delete 1.0 end;
}

#
# proc DispInsert2  {infolist {pos "end"}}
#----------------------------------------------------------------------
# insert list double spaced at pos to ascDispVect(textBox)
#----------------------------------------------------------------------
proc DispInsert2 {infolist {pos "end"}} {
  global ascDispVect
  foreach info $infolist {
    $ascDispVect(textBox) insert $pos "$info\n\n";
  }
}

#
# proc DispInsert  {infolist {pos "end"}}
#----------------------------------------------------------------------
# insert list at pos to ascDispVect(textBox)
#----------------------------------------------------------------------
proc DispInsert {infolist {pos "end"}} {
  global ascDispVect
  foreach info $infolist {
    $ascDispVect(textBox) insert $pos "$info\n";
  }
}
#

#
# proc DispInsert3  {info {pos "end"}}
#----------------------------------------------------------------------
# insert list at pos to ascDispVect(textBox)
#----------------------------------------------------------------------
proc DispInsert3 {info {pos "end"}} {
  global ascDispVect
  $ascDispVect(textBox) insert $pos ${info}\n;
}


# proc FastFileInText {w file}
#----------------------------------------------------------------------
# read in first 10000 bytes from file to text w
#----------------------------------------------------------------------
proc FastFileInText {w file} {
  set f [open $file]
  $w delete 1.0 end
  while {![eof $f]} {
    $w insert end [read $f 10000]
  }
  close $f
}


#
# FileUniqueName  {{prefix "/tmp/"}}
# FileUniqueName  {{prefix "c:/tmp/"}} windoze only
#----------------------------------------------------------------------
# (guaranteed unique result, within reason,by appending ver)
# filename returned will not exist
# Returns an id mashed from the process id and the current date.
# Assumes the tmp path set on utilities page unless told otherwise.
# Prefix should include a trailing / if no leaf prefix is supplied.
# eg, if you want a file in /tmp with just a number:
# FileUniqueName         (because /tmp/ is the FileUniqueName default)
# or
# FileUniqueName /tmp/
# will return /tmp/1234.23454 or similar.
# If you want a file with some prefix give full pathname up to end of
# prefix:
# FileUniqueName /tmp/scratchplot
# returns /tmp/scratchplot1234.23545
# For a real example see: DispWriteSelection
#---------------------------------------------------------------------
global tcl_platform
;#ifdef windoze
if {[string compare $tcl_platform(platform) "windows"]==0} {
proc FileUniqueName {{prefix "c:/temp/"}} {
  global env
  if {![file exists $prefix] && 
      [info exists env(TMPDIR)] && [file exists $env(TMPDIR)]} {
    set prefix $env(TMPDIR)/
  }
  set pidvar [pid]
  set datevar [clock format [clock seconds] -format %y%m%d%H%M%S]
  set fid "$prefix$pidvar\.$datevar"
  set ifid $fid
  set i 1
  while {[file exists $ifid]} {set ifid "$fid\.$i" ; incr i}
  return $ifid
}
#else
} else {
proc FileUniqueName {{prefix "/tmp/"}} {
  set pidvar [pid]
  set datevar [clock format [clock seconds] -format %y%m%d%H%M%S]
  set fid "$prefix$pidvar\.$datevar"
  set ifid $fid
  set i 1
  while {[file exists $ifid]} {set ifid "$fid\.$i" ; incr i}
  return $ifid
}
}
#endif /* windoze */
#
# DispWriteSelection {win}
#---------------------------------------------------------------------
# saves to a scratch file the selection of a window
# of class Listbox or Text. If no selection is made, saves whole window.
# returns name of scratchfile
#---------------------------------------------------------------------
proc DispWriteSelection {win} {

  global  ascUtilVect

  set dispfile [FileUniqueName "$ascUtilVect(asctmp)/ascdissel"];
  set fid 4
 # puts "Opening File $dispfile with fd = $jnk";
  #
  # Write the entire file if nothing selected.
  switch [winfo class $win] {
    {Text} { if {[catch {set l [$win get sel.first sel.last]} ]} {
               set l [$win get 1.0 end]
             }
             catch {set fid [open $dispfile w+]} jnk;
             puts $fid $l
           }
    {Listbox} {
                catch {set fid [open $dispfile w+]} jnk;
                if {[$win curselection] == ""} {
                  set last [$win size]
                  for {set l 0} {$l < $last} {incr l} {
                    puts $fid [$win get $l]
                  }
                } {
                  foreach l [$win curselection] {
                    puts $fid $l
                  }
                }
              }
     {Canvas} {
                set corners [$win bbox all]
                set x [lindex $corners 0]
                set y [lindex $corners 1]
                set w [expr [lindex $corners 2] - $x]
                set h [expr [lindex $corners 3] - $y]
                set ar [expr (1.0*$w)/(1.0*$h)]
                set rotate 0
                set pageopt "-pagewidth 6.5i"
                if {$ar < 0.7222222222} {
                  set pageopt "-pageheight 9.0i"
                }
                if {$ar > 1.3846} {
                  set pageopt "-pagewidth 9.0i"
                  set rotate 1
                }
                eval $win postscript -file $dispfile -x $x -y $y -width $w \
                    -height $h -rotate $rotate $pageopt
              }
     default {error "DispWriteSelection called on unsupported window class"}
  }
  catch {close $fid}
  return $dispfile;
}

#
# DispWriteCanvasRegion {win corners}
#---------------------------------------------------------------------
# saves to a scratch file the region given by corners
# of class Canvas. If no selection is made, saves whole window.
# returns name of scratchfile
#---------------------------------------------------------------------
proc DispWriteCanvasRegion {win corners} {

  global ascUtilVect

  set dispfile [FileUniqueName "$ascUtilVect(asctmp)/ascdissel"];
  set fid 4
 # puts "Opening File $dispfile with fd = $jnk";
  #
  # Write the entire file if nothing selected.
  set x [lindex $corners 0]
  set y [lindex $corners 1]
  set w [expr [lindex $corners 2] - $x]
  set h [expr [lindex $corners 3] - $y]
  set ar [expr (1.0*$w)/(1.0*$h)]
  set rotate 0
  set pageopt "-pagewidth 6.5i"
  if {$ar < 0.7222222222} {
    set pageopt "-pageheight 9.0i"
  }
  if {$ar > 1.3846} {
    set pageopt "-pagewidth 9.0i"
    set rotate 1
  }
  eval $win postscript -file $dispfile -x $x -y $y -width $w \
      -height $h -rotate $rotate $pageopt
  catch {close $fid}
  return $dispfile;
}

#
# proc DispPrint {file}
#---------------------------------------------------------------------
# also used in Probe_do_Print
# prints the file with the ToolVect print command set on utils page
# OR through dialog
#---------------------------------------------------------------------
proc DispPrint {file} {

  global ascToolVect
  if {$file==""} {error "DispPrint called without filename"}
  if {[file size $file] < 2} { #want more than a newline
    puts "Will not print empty files!!"
    return
  }
  if {$ascToolVect(printargs) == ""} {
    set printcmd "lpr";
  } else {
    set printcmd $ascToolVect(printargs);
    puts "Executing process : $printcmd $file";
    if {[catch {eval exec $printcmd $file} returnval]} {
      puts stderr "Printing Error : $returnval"
    }
  }
  catch {file delete $file}
}

#
# proc DispExecutePrint {}
#---------------------------------------------------------------------
# display execute.print button
#---------------------------------------------------------------------
proc DispExecutePrint {} {
  global ascDispVect
  Print_configure $ascDispVect(textBox)
  if {[Print_cancelcheck]} {
    return
  }
  DispPrint [DispWriteSelection $ascDispVect(textBox)]
  HUB_Message_to_HUB WINDOWPRINTED DISPLAY
}

#
# proc Is_Fundamental_Type {type}
#---------------------------------------------------------------------
# boolean check if type is one of the know compiler types.
# This code should really be based on the fundamental type list in
# C-land. Otherwise if new fundamental types are added or removed
# we could be caught flat footed.
#---------------------------------------------------------------------
proc Is_Fundamental_Type {type} {
  if {$type == "real" || $type == "integer" ||
      $type == "boolean" || $type == "set" ||
      $type == "symbol" || $type == "mutable_integer"} {
    return 1;
  } {
    return 0;
  }
}

#
# proc Disp_Raise_Alert {errmsg {label "Error"} {geom "200x70+480+200"}}
#---------------------------------------------------------------------
# display error box
#---------------------------------------------------------------------
proc Disp_Raise_Alert {errmsg {label "Error"} {geom "200x70+480+200"}} {
   Script_Raise_Alert $errmsg $label $geom
}
#
# proc Disp_ShowCode  {type inputfile outputfile}
#---------------------------------------------------------------------
# button internals for Disp_do_ShowCode
#---------------------------------------------------------------------
proc Disp_ShowCode {type inputfile outputfile} {
  # cant show code for fundamental types
  global ascDispVect env
  if {$type == "" || [Is_Fundamental_Type $type]} {
    Script_Raise_Alert "Cannot show code for fundamental types" FYI
  }
  set nok 0
  if {$ascDispVect(ShowComments)} {
    set nok \
      [catch \
	  {libr_extract_type -c $type $inputfile -s} \
	   err_msg]
  } else {
    set nok \
      [catch \
	  {libr_extract_type $type $inputfile -s} \
	   err_msg]
  }
  if {$nok} {
    Disp_Raise_Alert "Error encountered \n in displaying code :$err_msg"
    return 1;
  }
  set ascDispVect(lastcode) $err_msg
  return 0;
}

#
# proc Disp_do_ShowCode {}
#---------------------------------------------------------------------
# library display.code button
#---------------------------------------------------------------------
proc Disp_do_ShowCode {args} {
  global ascLibrVect ascDispVect ascUtilVect 
  set type $args
  if {$args == ""} {
    set type $ascLibrVect(selectedtype);
    if {$type == ""} {
      return;
    }
  }
  set inputfile [file_by_type $type]
  set outputfile [FileUniqueName "$ascUtilVect(asctmp)/ascdiscode"]
  # outputfile unused in Disp_ShowCode
  set result [Disp_ShowCode $type $inputfile $outputfile]
  if {!$result} {
    DispClear
    $ascDispVect(textBox) insert end $ascDispVect(lastcode)
  } else {
    return
  }
  if {0} { #ifdef ignore, obsolete
  if {[file exists $outputfile]} {
    file delete $outputfile
  }
  }
  DispSetEntry "Code for $type"
  newraise .display
}
#
# proc Disp_do_ShowAncestry {}
#---------------------------------------------------------------------
# library display.ancestry button
#---------------------------------------------------------------------
proc Disp_do_ShowAncestry {} {
  global ascLibrVect
  if {$ascLibrVect(selectedtype) == ""} {
    return;
  }
  Type_OpenTree $ascLibrVect(selectedtype)
  return
}
#
# proc Disp_do_ShowPendings {}
#---------------------------------------------------------------------
# library display.code button
#---------------------------------------------------------------------
proc Disp_do_ShowPendings {} {
  global ascDispVect ascUtilVect ascSimsVect
  set sim $ascSimsVect(selectedsim)
  if {$sim == ""} {
    return;
  }
  set outputfile [FileUniqueName "$ascUtilVect(asctmp)/ascdispending"]
  simlistpending $sim $outputfile
  if {[file exists $outputfile]} {
    FastFileInText $ascDispVect(textBox) $outputfile
    file delete $outputfile
  }
  DispSetEntry "Pendings for $sim"
  # puts "raising display"
  newraise .display
}
#
# proc DispHkeep {{prefix "adisp"}}
#---------------------------------------------------------------------
# remove files /tmp/prefix*
#---------------------------------------------------------------------
proc DispHkeep {{prefix "adisp"}} {
  catch {file delete [glob /tmp/$prefix*]}
}

#
# proc revlist {list}
#---------------------------------------------------------------------
# return a list in reverse order of the one sent
#---------------------------------------------------------------------
proc revlist {list} {# used for reversing lists.
  set b "";
  foreach i $list {
    set b [linsert $b 0 $i]
  }
  return $b;
}

#
# proc DispPrintTypeTree {type}
#---------------------------------------------------------------------
# pretty print a type to ascDispVect(textBox) for DispDoHierarchy
#---------------------------------------------------------------------
proc DispPrintTypeTree {type} {# used by DispDoHierrarchy
  global ascDispVect
  set TAB 4
  set reflist [drefines_me $type]
  set nch [llength $reflist]
  if {$nch == "0"} {
    return
  }
  set reflist [lsort $reflist]
  incr ascDispVect(indent) $TAB
  foreach ref $reflist {
    if {$ref != "$type"} {
      for {set i 1} {$i <= $ascDispVect(indent)} {incr i} {
        $ascDispVect(textBox) insert end " "
      }
      $ascDispVect(textBox) insert end "$ref\n"
      DispPrintTypeTree "$ref"
   }
 }
 incr ascDispVect(indent) -$TAB
}

#
# proc DispPrintHierarchy {{dlist ""}}
#---------------------------------------------------------------------
# print hierarchy. used to Disp_do_Hierarchy
#---------------------------------------------------------------------
proc DispPrintHierarchy {{dlist ""}} {# used to Display hierarchy
   global ascDispVect
    foreach type $dlist {
       if {[disroot_type $type] == "1"} {
 	  $ascDispVect(textBox) insert end "$type\n"
          DispPrintTypeTree $type
       }
    }
}

#
# proc Disp_do_Hierarchy
#---------------------------------------------------------------------
# library display.hierarchy button
#---------------------------------------------------------------------
proc Disp_do_Hierarchy {} { # attached to the Library Hierarchy Button
  global ascDispVect ascLibrVect

  catch {DispClear} jnk;
  DispPrintHierarchy [disp define];
  DispSetEntry "Refinement line for all types"
  set location [$ascDispVect(textBox) search \
    -exact $ascLibrVect(selectedtype) 1.0 end]
  $ascDispVect(textBox) see $location
  newraise .display
}

#
# proc Disp_do_PrintExtFuncLibrary
#---------------------------------------------------------------------
# write to window the external functions library.
#---------------------------------------------------------------------
proc Disp_do_PrintExtFuncLibrary {} {
  global ascDispVect
  set result [libr_query -externalfunctions]
  if {[string compare $result {}] == 0} {
    set result {NONE-LOADED {There are no external, global functions defined}}
  }
  catch {DispClear}
  foreach info $result {
    foreach p $info {
      $ascDispVect(textBox) insert end "$p\n"
    }
    $ascDispVect(textBox) insert end "\n"
  }
  newraise .display
}

#
# proc Disp_do_Font {}
#---------------------------------------------------------------------
# font select button for display window
# uses xfontsel widget for display fonts. wrapper baa
#---------------------------------------------------------------------
proc Disp_do_Font {args} {
  global ascDispVect
  set font  ""
  if {$args != ""} {
    set font $args
  } else {
    set font [ascFontGet]
  }
  if {"$font" == ""} {
    return;
  }
  $ascDispVect(textBox) configure -font $font
  set ascDispVect(font) [lindex [$ascDispVect(textBox) configure -font] 4]
  DispRepack
}

#
# proc DispRepack {}
#---------------------------------------------------------------------
# repack the listbox after fontset
#---------------------------------------------------------------------
proc DispRepack {} {
  pack forget .display.main_frm .display.entry_frm
  pack append .display .display.main_frm {top frame center expand fill} \
              .display.entry_frm {top frame center fillx}

}
#
# Disp_do_Help {}
# Disp_do_BindHelp {}
#---------------------------------------------------------------------
# help calls for On viewing and On DISPLAY
#----------------------------------------------------------------------
proc Disp_do_Help {} {
  Help_button display
}
proc Disp_do_BindHelp {} {
  Help_button display.help ondisplay
}

#
# proc TextSearch {w string tag}
#----------------------------------------------------------------------
# Some fun stuff
# The utility procedure below searches for all instances of a
# given string in a text widget and applies a given tag to each
# instance found.
# Arguments:
#
# w -           The window in which to search.  Must be a text widget.
# string -      The string to search for.  The search is done using
#               exact matching only;  no special characters.
# tag -         Tag to apply to each instance of a matching string.
#----------------------------------------------------------------------
proc TextSearch {w string tag} {
    $w tag remove search 0.0 end
    scan [$w index end] %d numLines
    set l [string length $string]
    for {set i 1} {$i <= $numLines} {incr i} {
        if {[string first $string [$w get $i.0 $i.1000]] == -1} {
            continue
        }
        set line [$w get $i.0 $i.1000]
        set offset 0
        while 1 {
            set index [string first $string $line]
            if {$index < 0} {
                break
            }
            incr offset $index
            $w tag add $tag $i.[expr $offset] $i.[expr $offset+$l]
            incr offset $l
            set line [string range $line [expr $index+$l] 1000]
        }
    }
}
