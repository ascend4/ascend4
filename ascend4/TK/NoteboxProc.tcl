#  NoteboxProc.tcl: Functions for handling the notes database display box
#  By Benjamin Andrew Allan
#  May 3, 1998
#  Part of ASCEND
#  Revision: $Revision: 1.3 $
#  Last modified on: $Date: 1998/06/18 15:54:51 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: NoteboxProc.tcl,v $
#
#  This file is part of the ASCEND Tcl/Tk Interface.
#
#  Copyright (C) 1998 Carnegie Mellon University
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


global ascNotesVect
set ascNotesVect(defaulted) 0

proc set_Notebox_defaults {} {
  global ascNotesVect
  if {$ascNotesVect(defaulted)} {
    return
  }
  set ascNotesVect(stringmatch) {}
  set ascNotesVect(windowname) .notebox
  set ascNotesVect(textwidth) 30
  set ascNotesVect(font) {courier 10}
  set ascNotesVect(sort1) Type
  set ascNotesVect(sort2) None 
  set ascNotesVect(current) {Loaded Libraries} 
  set ascNotesVect(global_built) 0
  set ascNotesVect(defaulted) 1
}

proc NoteBind {} {
  global ascNotesVect
  #$ascNotesVect(typelist) delete 0 end
  #$ascNotesVect(langlist) delete 0 end
  #$ascNotesVect(namelist) delete 0 end
  #$ascNotesVect(methlist) delete 0 end
  #$ascNotesVect(textlist) delete 0 end
  #$ascNotesVect(fililist) delete 0 end
  bind $ascNotesVect(dbbutton) <Any-Enter> {+NoteUpdateDBbutton}
  HPane-Bind .notebox.main_frm list_frm text_frm 10 0.5
  bind $ascNotesVect(textlist) <ButtonRelease-1> \
    {+NoteUpdateText [%W nearest %y]}
  bind $ascNotesVect(fililist) <ButtonRelease-1> \
    {+NoteUpdateFileEntry [%W nearest %y]}
}

proc NoteUpdateDBbutton {} {
  global ascNotesVect
  set ascNotesVect(dbchoices) [libr_query -notesdblist]
  $ascNotesVect(dbmenu) delete 0 end
  foreach i $ascNotesVect(dbchoices) {
    $ascNotesVect(dbmenu) add radiobutton \
      -variable ascNotesVect(current) \
      -value $i \
      -label $i
  }
  $ascNotesVect(dbmenu) entryconfigure 0 -state active
}

proc NoteOpen {} {
  if {![winfo exists .notebox]} {
    set_Notebox_defaults
    ShowWindow.notebox
    NoteBind
  }
  wm deiconify .notebox
  raise .notebox
}

proc NoteUpdateFileEntry {index} {
  global ascNotesVect
  set ascNotesVect(entrytext) [NoteGetFile $index]
}

proc NoteUpdateText {index} {
  global ascNotesVect
  set text [NoteGetText $index]
  $ascNotesVect(textbox) delete 1.0 end
  $ascNotesVect(textbox) insert end $text
}

proc NoteGetFile {index} {
  global NotesRowToRecord
  global ascNotesVect
  if {![info exists NotesRowToRecord($index)]} {
    error "NoteText called with bad row index $index"
  }
  set nptr $NotesRowToRecord($index)
  set note [libr_query -notes -dbid $ascNotesVect(current) -record $nptr]
  if {[llength $note]} {
    return [lindex [lindex $note 0] 5]:[lindex [lindex $note 0] 6]
  }
}

proc NoteGetText {index} {
  global NotesRowToRecord
  global ascNotesVect
  if {![info exists NotesRowToRecord($index)]} {
    error "NoteText called with bad row index $index"
  }
  set nptr $NotesRowToRecord($index)
  set note [libr_query -notes -dbid $ascNotesVect(current) -record $nptr]
  if {[llength $note]} {
    return [lindex [lindex $note 0] 4]
  }
}

proc NoteKeyToColumn {key} {
  switch -exact -- $key {
  Type {
      return 0
    }
  Language {
      return 1
    }
  Name {
      return 2
    }
  Method {
      return 3
    }
  File {
      return 4
    }
  None -
  default {
      return -1
    }
  }
}

proc NoteSortLists {pkey skey} {
  global ascNotesVect
 # translate keys to columns
  set e1 [NoteKeyToColumn $pkey]
  set e2 [NoteKeyToColumn $skey]
 # enforce column sanity on keys
  if {$e1 < 0 && $e2 > $e1} {
    set ascNotesVect(sort1) $skey
    set ascNotesVect(sort2) None
  }
  if {$e1 == $e2} {
    set ascNotesVect(sort2) None
    set skey None
  }
  set e1 [NoteKeyToColumn $pkey]
  set e2 [NoteKeyToColumn $skey]

 # puts "sorting with $pkey $e1 $skey $e2"
  if {$e1 < 0 && $e2 < 0 &&
      ![string length [string trim $ascNotesVect(stringmatch)]]} {
    return [libr_query -notesdump -dbid $ascNotesVect(current) \
             -textwidth $ascNotesVect(textwidth)]
  }
  set data [libr_query -notesdump -dbid $ascNotesVect(current) \
             -textwidth $ascNotesVect(textwidth)]
  set l(0) [lindex $data 0]
  set l(1) [lindex $data 1]
  set l(2) [lindex $data 2]
  set l(3) [lindex $data 3]
  set l(4) [lindex $data 4]
  set l(5) [lindex $data 5]
  set l(6) [lindex $data 6]
  unset data
  set master {}
  if {$e2 < 0} {
    # if unsorted last key, the natural order is mostly preserved by this
    # and it's much faster.
    set master [lsort -ascii $l($e1)]
    unset l($e1)
  } else {
    set ascNotesVect(l2) $l($e2)
    set master [lsort -command NoteSort $l($e1)]
    unset l($e1)
  }
  set data {}
  for {set i 0} {$i < 7} {incr i} {
    if {$i == $e1} {
      lappend data $master
    } else {
      set list {}
      foreach j $master {
        lappend list [lindex $l($i) [lindex  $j 1]]
      }
      lappend data $list
      unset list
      unset l($i)
    }
  }
  return $data
}

proc NoteSort {k1 k2} {
  global ascNotesVect
  set r [string compare [lindex $k1 0] [lindex $k2 0]]
  if {$r} {
    return $r
  }
  return [string compare \
           [lindex [lindex $ascNotesVect(l2) [lindex $k1 1]] 0] \
           [lindex [lindex $ascNotesVect(l2) [lindex $k2 1]] 0] \
         ]
}

proc NoteFillLists {} {
  global ascNotesVect
  global NotesRowToRecord NotesRecordToRow
  set data [NoteSortLists $ascNotesVect(sort1) $ascNotesVect(sort2)]
  $ascNotesVect(textlist) configure -width $ascNotesVect(textwidth)
  $ascNotesVect(typelist) delete 0 end
  $ascNotesVect(langlist) delete 0 end
  $ascNotesVect(namelist) delete 0 end
  $ascNotesVect(methlist) delete 0 end
  $ascNotesVect(textlist) delete 0 end
  $ascNotesVect(fililist) delete 0 end
 #uncommenting all the row/check statements verifies the sort
 #set check(-1) {}
  set col [lindex $data 0]
 #set row 0
  foreach i $col {
    $ascNotesVect(typelist) insert end [lindex $i 0]
 #  lappend check($row) [lindex $i 1]
 #  incr row
  }
  set col [lindex $data 1]
 #set row 0
  foreach i $col {
    $ascNotesVect(langlist) insert end [lindex $i 0]
 #  lappend check($row) [lindex $i 1]
 #  incr row
  }
  set col [lindex $data 2]
 #set row 0
  foreach i $col {
    $ascNotesVect(namelist) insert end [lindex $i 0]
 #  lappend check($row) [lindex $i 1]
 #  incr row
  }
  set col [lindex $data 3]
 #set row 0
  foreach i $col {
    $ascNotesVect(methlist) insert end [lindex $i 0]
 #  lappend check($row) [lindex $i 1]
 #  incr row
  }
  set col [lindex $data 4]
 #set row 0
  foreach i $col {
    $ascNotesVect(textlist) insert end [lindex $i 0]
 #  lappend check($row) [lindex $i 1]
 #  incr row
  }
  set col [lindex $data 5]
 #set row 0
  foreach i $col {
    $ascNotesVect(fililist) insert end [file tail [lindex $i 0]]
 #  lappend check($row) [lindex $i 1]
 #  incr row
  }
 # parray check
  set col [lindex $data 6]
  set row 0
  catch {unset NotesRowToRecord}
  catch {unset NotesRecordToRow}
  set NotesRecordToRow(-1) 0; #NULL note, keep array existent
  set NotesRowToRecord(-1) 0; #NULL note, keep array existent
  set row 0
  foreach i $col {
    set NotesRowToRecord($row) [lindex $i 0]
    set NotesRecordToRow([lindex $i 0]) $row
    incr row
  }
}

proc NoteBrowse {args} {
  NoteOpen
  NoteFillLists
}

proc Notes_Handle_Sourceread {args} {
  global ascNotesVect
  
  set_Notebox_defaults
  if {![winfo exists $ascNotesVect(windowname)] || \
      ![winfo ismapped $ascNotesVect(windowname)]} {
    return
  }
  NoteBrowse
}
