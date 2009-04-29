#  HubProc.tcl: This is the ASCEND Hub -- what others call an event broker.
#  by Benjamin A. Allan and Kirk A. Abbott
#  Created: January 1994
#  Part of ASCEND
#  Revision: $Revision: 1.15 $
#  Last modified on: $Date: 1998/06/18 15:54:46 $
#  Last modified by: $Author: mthomas $
#  Revision control file: $RCSfile: HubProc.tcl,v $
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
# proc set_HUB_Defaults {}
#------------------------------------------------------------------------
# This is the ascend HUB. It acts as the event broker for disptching
# update info for other windows and variables. Any information of interest
# should be registered with the HUB. New events and their respective actions
# should be registered in the HUB_Table. Care should be taken that cycles
# are not created, such that unnecessary updating is done. Every function
# registered MUST be able to handle 1 argument which is a list which might
# be empty:
#
# proc myfunc {{list ""}} {
#   foreach item $list {
#     puts "$item"
#   }
# }
#------------------------------------------------------------------------
proc set_HUB_Defaults {} {

  global HUB_Table

#------  EVENT ------------------------ INTERESTED functions ------------

  set HUB_Table(SYSFLUSH) [list \
    Script_Record_Flush \
    ]

  set HUB_Table(SIMDELETEDONE) [list \
    Probe_HandleNewInstances \
    ]

  set HUB_Table(SIMDELETED) [list \
    Probe_HandleSimsDelete \
    Probe_UserData_HandleSimsDelete \
    Brow_HandleSimsDelete \
    Solve_HandleSimsDelete \
    Script_Record_Delete \
    Announce_sims_Deleted]

  set HUB_Table(LIBDESTROYED) [list \
    Script_Record_DeleteTypes \
    Solve_HandleTypesDelete \
    Sims_HandleTypesDelete \
    Type_Handle_Sourceread \
    Units_HandleLibDestroyed]

  set HUB_Table(SOLVINGDONE) [list \
    Probe_HandleVariableUpdated \
    Brow_HandleVariableUpdated]

  set HUB_Table(VARIABLEUPDATED) [list \
    Probe_HandleVariableUpdated \
    Brow_HandleVariableUpdated]

  set HUB_Table(INSTANCEMOVED) [list  \
    Probe_HandleInstanceMoved \
    Probe_UserData_HandleInstanceMoved \
    Solve_HandleInstanceMoved \
    Sims_HandleInstanceMoved]

  set HUB_Table(INSTANCERESUMED) [list \
    Probe_HandleNewInstances \
    Script_Record_Resume]

  set HUB_Table(BOOLEANUPDATED) [list \
    Solve_HandleBooleanUpdated]

  set HUB_Table(WHENVARUPDATED) [list \
    Solve_HandleWhenVarUpdated]

  set HUB_Table(TYPEHIDDEN) [list \
    Brow_HandleTypeHidden]

  set HUB_Table(SOURCEREAD) [list \
    Units_HandleSourceRead \
    Type_Handle_Sourceread \
    Notes_Handle_Sourceread \
    Script_Record_Read]

  set HUB_Table(UNITSUPDATED) [list \
    Solve_HandlePrecisionUpdated \
    Probe_HandleVariableUpdated \
    Brow_HandleVariableUpdated]

  set HUB_Table(SIMCREATED) [list \
    Script_Record_Compile \
    Library_Handle_Configure \
    Probe_HandleNewInstances]
  set HUB_Table(SOLVINGSTARTED) \
    [list Script_Record_Solve ]
  set HUB_Table(DATAREAD) \
    [list Script_Record_ValueRead ]
  set HUB_Table(DATAWRITE) \
    [list Script_Record_ValueWrite ]
  set HUB_Table(CLEARVARS) [list \
    Script_Record_ClearVars \
    Probe_HandleVariableUpdated \
    Brow_HandleVariableUpdated]

  set HUB_Table(INSTREFINED) [list \
    Brow_HandleInstRefined \
    Probe_HandleNewInstances \
    Script_Record_Refine]

  set HUB_Table(INSTMERGED) [list \
    Brow_HandleInstMerged \
    Probe_HandleNewInstances \
    Script_Record_Merge]

  set HUB_Table(INSTBROWSED) \
    [list Script_Record_Browse ]
  set HUB_Table(VALUESET) \
    [list Script_Record_Assign ]
  set HUB_Table(PLOTMADE) \
    [list Script_Record_Plot ]
  set HUB_Table(PROCRUN) \
    [list Script_Record_Run ]
  set HUB_Table(INSTPROBED) \
    [list Script_Record_Probe ]
  set HUB_Table(DISPLAYFILLED) \
    [list Script_Record_Display ]
  set HUB_Table(WINDOWPRINTED) \
    [list Script_Record_Print ]

}


#Document your event semantics here:

#SIMDELETEDONE
# meaning: one or more simulations are now gone

#SIMDELETED simname
# meaning: if you are looking at sim, forget him as he is about to die.

#SYSFLUSH
# meaning: clear the thing in the solver window.
# should have a number added to support multiple systems.

#INSTANCEMOVED qlfdid
# meaning: everybody repoint to or forget about any and all parts of the
# simulaton corresponding to the given qualified id. There is a particluar
# Case with the Browser and Script as they might have originated the message
# themselves. As a result they are not registered under this event.

#INSTREFINED instancename type
#INSTMERGED instancename1 instancename2
#INSTANCERESUMED instancename
# these are 'informational' messages only. The INSTANCEMOVED message
# is still the one for C housekeeping. These messages should be sent
# AFTER a merge or refine so that windows may be updated to reflect the
# new state of the world after a merge or refine.

#LIBDESTROYED
# meaning: All types in the interface library have been deleted.

#SOLVINGDONE instancename
# meaning: solver has finished some execution step and updated real vars
# probably throughout the sim that instancename is a part of

#VARIABLEUPDATED instancename
# meaning: internal value of a real/integer may have changed/been set
# Redisplay if you are interested in instancename or another of its
# aliases.

#BOOLEANUPDATED instancename
# meaning: boolean, which generally means model structure change or reset
# This forces a solver flush if instancename is not given.

#WHENVARUPDATED instancename
# meaning: boolean, symbol or integer member of a list of whenvars have
# changed which means model structure change
# This forces a solver flush

#TYPEHIDDEN
# meaning: The TYPESHOW bit of a type description has changed.
# It call for a Update_ChildBox in the browser
#

#SOURCEREAD filename
# meaning: new file just successfully read into library
# type hierarchy may have changed.

#UNITSUPDATED
# meaning: display value precision or display units changed.
# redisplay if you want to stay current
#
# The following are principally of script interest, though others may
# also want to look at them if we start accumulating instance history
# data, for example, as Art has proposed.

#SIMCREATED simname
# something has been compiled

#DATAREAD filename
# a values file has been read. the fact that this may have changed something
# is handled distinctly by VARIABLEUPDATED/BOOLEANUPDATED.

#DATAWRITE instancename filename
# a values file has been written for instancename

#CLEARVARS instancename
# all fixed flags in instance have been set to FALSE

#INSTPROBED instancename style
# instance exported to probe in the style indicated
# styles are at present ONE (just the item) ALL (probeall meaning)

#INSTBROWSED instancename
# instance exported to browser

#SOLVINGSTARTED instname solvername
# Execute.Solve button pushed

#VALUESET instancename value [units]
# A variable assignment has been done as indicated
# The fact that this may have changed something
# is handled distinctly by VARIABLEUPDATED/BOOLEANUPDATED.

#PLOTMADE instancename filename
# A plot has been made using the currently set plot type

#PROCRUN qlfpid
# an initialization procedure qlfpid has been executed.

#DISPLAYFILLED
# something sent to display window

#WINDOWPRINTED window
# window dumped to printer

#
# proc HUB_Message_to_HUB {event args} {
#------------------------------------------------------------------------
# Call this function with the event name and any other arguments you wish.
# All arguments other than the first will be put into the args list.
# The event will be looked up in global array HUB_Table and the
# args list passed on to each routine in HUB_Table(event). Routines
# will be called in the order they appear within the list HUB_Table($event).
# If the Hub cannot successfully run all routines, it will return 1, else
# will return 0. Failure of one routine will not stop others from being
# called.
# Initiating an event without the arguments indicated above will not be
# fatal, but may leave the system in an undesirable state or a state more
# drastically disturbed than is necessary.
#------------------------------------------------------------------------
proc HUB_Message_to_HUB {event args} {
  global HUB_Table

  if {[catch {set activitylist $HUB_Table($event)} ]} {
    puts stderr "Interface broker called with unknown event $event"
    return 1
  }
  set error 0
  set nok ""
  foreach activity $activitylist {
    if {[catch "$activity $args" nok]} {
      #_don't put {} around the the catch body -- it messes the recording.
      puts "Interface broker error in call:\n $activity $args \n$nok"
      set error 1
    }
  }
  return $error
}

proc Announce_sims_Deleted {sims} {
  puts "$sims has been deleted."
}
