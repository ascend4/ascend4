#  CallbackProc.tcl: Tcl functions to animate the callback help window.
#  By Benjamin Allan
#  March 14, 1998
#  Part of ASCEND
#  Revision: $Revision: 1.5 $
#  Last modified on: $Date: 1998/06/18 15:54:40 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: CallbackProc.tcl,v $
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

# based on the ascend callback 'help', processes info into easy-eye form

proc set_Callback_Defaults {} {
  global ascCallbackVect
  set ascCallbackVect(allorgroup) group ;# all is the alternative
  set ascCallbackVect(curgroup) none ;# currently listed group
  set ascCallbackVect(curcommand) none ;# currently explained command
  set ascCallbackVect(grouplist) .callback.grp_frm.com_list.listbox1
  set ascCallbackVect(grouptext) .callback.grp_frm.com_expl.text2
  set ascCallbackVect(comlist) .callback.com_frm.frame.listbox1
  set ascCallbackVect(comtext) .callback.com_frm.frame11.text2
  set ascCallbackVect(helpgroups) [help groups]
  set ascCallbackVect(dbglist) [dbghelp]
  set ascCallbackVect(slvlist) [slvhelp]
  set ascCallbackVect(mtxlist) [mtxhelp]
  set ascCallbackVect(ulist) [uhelp]
  set ascCallbackVect(mode) a;# c is for commands, a for arrays
}

proc set_Callback_bindings {} {
  global ascCallbackVect
  # put in sliders
  HPane-Bind .callback grp_frm com_frm 10 0.333
  VPane-Bind [winfo parent [winfo parent $ascCallbackVect(grouplist)]] \
   [winfo name [winfo parent $ascCallbackVect(grouplist)]] \
   [winfo name [winfo parent $ascCallbackVect(grouptext)]] 10 0.333
  VPane-Bind [winfo parent [winfo parent $ascCallbackVect(comlist)]] \
   [winfo name [winfo parent $ascCallbackVect(comlist)]] \
   [winfo name [winfo parent $ascCallbackVect(comtext)]] 10 0.333
  # bind m1 in group list to update explanation
  bind $ascCallbackVect(grouplist) <Button-1> {
    global ascCallbackVect
    set ndx [%W nearest %y]
    if {$ndx != ""} {
      set ascCallbackVect(curgroup) [%W get $ndx]
      Callback_Update_group_expl $ascCallbackVect(curgroup)
      Callback_Update_Commands
    }
  }
  # bind double m1 in group list to update command list
  bind $ascCallbackVect(grouplist) <Double-1> {
    set ndx [%W curselection]
    if {$ndx != ""} {
      set ascCallbackVect(curgroup) [%W get $ndx]
      Callback_Update_Commands
    }
  }
  # bind m1 in command list to update explanation
  bind $ascCallbackVect(comlist) <Button-1> {
    global ascCallbackVect
    set ndx [%W nearest %y]
    if {$ndx != ""} {
      set ascCallbackVect(curcommand) [%W get $ndx]
      Callback_Update_command_expl $ascCallbackVect(curcommand)
    }
  }
  Callback_UpdateMode
}

# change window to reflect current value of mode
proc Callback_UpdateMode {} {
  global ascCallbackVect
  switch $ascCallbackVect(mode) {
  a {
      set ascCallbackVect(purposelabel) "Array purpose:"
      set ascCallbackVect(grouplabel) "Global arrays"
      set ascCallbackVect(listlabel) "Array elements"
    }
  c -
  default {
      set ascCallbackVect(purposelabel) "Group purpose:"
      set ascCallbackVect(grouplabel) "Command groups:"
      set ascCallbackVect(listlabel) "Commands:"
    }
  }
}

proc Callback_Open {} {
  global ascCallbackVect
  if {![info exists ascCallbackVect(allorgroup)]} {
    set_Callback_Defaults
  }
  VShowWindow.callback ;# which calls Callback_Update when done
  set_Callback_bindings
  Callback_Update
}

proc Callback_Update_group_expl {group} {
  global ascCallbackVect
  $ascCallbackVect(grouptext) delete 1.0 end
  switch $ascCallbackVect(mode) {
  a {
      switch $group {
      none {$ascCallbackVect(grouptext) insert end \
         {purpose of selected array}}
      default {
           global $group
           if {[info exists ${group}(purpose)]} {
             $ascCallbackVect(grouptext) insert end ${group}(purpose)
           } else {
             $ascCallbackVect(grouptext) insert end \
                {Purpose not internally documented.}
           }
        }
      }
    }
  c -
  default {
      switch $group {
      none {$ascCallbackVect(grouptext) insert end \
         {explanation of selected group}}
      default {
           $ascCallbackVect(grouptext) insert end [lindex [help $group] 0]
        }
      }
    }
  }
}

proc Callback_Update_command_expl {command} {
  global ascCallbackVect
  $ascCallbackVect(comtext) configure -state normal
  $ascCallbackVect(comtext) delete 1.0 end
  
  if {[string compare $ascCallbackVect(mode) "a"]== 0} {
    upvar #0 $ascCallbackVect(curgroup) PA
    catch {$ascCallbackVect(comtext) insert end $PA($command)} err
    $ascCallbackVect(comtext) configure -state disabled
    return
  }
  # else handle callbacks mess
  switch $command {
  none {
      $ascCallbackVect(comtext) insert end \
           {explanation of command selected at left}
    }
  default {
       append data "Internal documentation of $command is missing.\n" \
         "Mail ascend+help@edrc.cmu.edu with your question about it.\n" \
         "IFF you are ambitious, " \
         "check the C headers in directory ascend4/interface/."
       if {[catch {set data [help $command]} err]} {
         if {[lsearch $ascCallbackVect(slvlist) $command] != -1} {
            set data {See the output of 'slvhelp long' on the console.}
            slvhelp long
         }
         if {[lsearch $ascCallbackVect(dbglist) $command] != -1} {
            set data {See the output of 'dbghelp long' on the console.}
            dbghelp long
         }
         if {[lsearch $ascCallbackVect(ulist) $command] != -1} {
            set data {See the output of 'uhelp long' on the console.}
            uhelp long
         }
         if {[lsearch $ascCallbackVect(mtxlist) $command] != -1} {
            set data {See the output of 'mtxhelp long' on the console.}
            mtxhelp long
         }
       }
       
       $ascCallbackVect(comtext) insert end $data
    }
  }
  $ascCallbackVect(comtext) configure -state disabled
}

proc Callback_Update_Groups {} {
  global ascCallbackVect
  # $ascCallbackVect(grouplist) configure -state normal 
  $ascCallbackVect(grouplist) delete 0 end
  switch $ascCallbackVect(mode) {
  a {
      eval $ascCallbackVect(grouplist) insert end \
        [lsort -dictionary [info globals asc*Vect]]
      set index [lsearch [info vars asc*Vect] $ascCallbackVect(curgroup)]
    }
  c -
  default {
      eval $ascCallbackVect(grouplist) insert end $ascCallbackVect(helpgroups)
      set index \
        [lsearch $ascCallbackVect(helpgroups) $ascCallbackVect(curgroup)]
    }
  }
  $ascCallbackVect(grouplist) see $index
  $ascCallbackVect(grouplist) selection set $index $index
  Callback_Update_group_expl $ascCallbackVect(curgroup)
}

proc Callback_Update_Commands {} {
  global ascCallbackVect
  $ascCallbackVect(comlist) delete 0 end
  set clist {}
  switch $ascCallbackVect(mode) {
  a {
      global $ascCallbackVect(curgroup)
      catch {set clist [lsort -dictionary \
                         [array names $ascCallbackVect(curgroup)]]} err
      # may leave elements list empty
    } 
  c -
  default {
      switch {$ascCallbackVect(allorgroup)} {
      all { set clist [help all] }
      default {
          catch {set clist \
                  [lrange [help $ascCallbackVect(curgroup)] 1 end]} err
          # may leave commands list empty
        }
      }
    }
  }

  eval $ascCallbackVect(comlist) insert end $clist
  set index [lsearch $clist $ascCallbackVect(curcommand)]
  $ascCallbackVect(comlist) see $index
  $ascCallbackVect(comlist) selection set $index $index
}

proc Callback_Update {} {
  Callback_Update_Groups
  Callback_Update_Commands
}

proc Callback_do_Print {} {
  error "Callback_do_Print not implemented"
}
proc Callback_do_Close {} {
  DestroyWindow.callback
}
proc Callback_do_wordsearch {} {
}
proc Callback_do_usagesearch {} {
}
proc Callback_do_textsearch {} {
}
proc Callback_do_command {} {
}
proc Callback_do_All {} {
}

proc Callback_do_Groups {} {
  global ascCallbackVect
  set ascCallbackVect(mode) c
  set ascCallbackVect(curcommand) none
  set ascCallbackVect(curgroup) none
  Callback_UpdateMode ;# change titles
  Callback_Update ;#change data
}

proc Callback_do_Font {} {
}
proc Callback_do_About {} {
}
proc Callback_do_syntax {} {
}
proc Callback_do_TOC {} {
}

proc Callback_do_Arrays {} {
  global ascCallbackVect
  set ascCallbackVect(mode) a
  set ascCallbackVect(curgroup) none
  set ascCallbackVect(curcommand) none
  Callback_UpdateMode ;# change titles
  Callback_Update ;#change data
}
