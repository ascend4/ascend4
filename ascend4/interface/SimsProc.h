/*
 *  SimsProc.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.12 $
 *  Version control file: $RCSfile: SimsProc.h,v $
 *  Date last modified: $Date: 2003/08/23 18:43:08 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the ASCEND Tcl/Tk interface
 *
 *  Copyright 1997, Carnegie Mellon University
 *
 *  The ASCEND Tcl/Tk interface is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The ASCEND Tcl/Tk interface is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

#ifndef SimsProc_module_loaded
#define SimsProc_module_loaded

/*
 *  To include this header, you must include the following:
 *      #include "tcl.h"
 *      #include "compiler/compiler.h"
 *      #include "compiler/instance_enum.h"
 *      #include "interface/SimsProc.h"
 */

extern struct Instance *Asc_FindSimulationTop(symchar *str);
/*
 *  return the sim pointer if there is a sim named *str
 */

extern struct Instance *Asc_FindSimulationRoot(symchar *str);
/*
 *  return the top inst pointer if there is a sim named *str
 */

extern symchar *Asc_SimsFindSimulationName(CONST struct Instance *);
/*
 *  return the name string of the instance given if it is in the
 *  global sim list. return null if not.
 */

extern void Asc_SetCurrentSim(struct Instance *);
/*
 *  void Asc_SetCurrentSim;
 *  struct Instance *sim;
 *  Comments :
 *  Sets the current working simulation to the simulation given.
 */

extern struct Instance *Asc_GetCurrentSim(void);
/*
 *  struct Instance *Asc_GetCurrentSim;
 *  Comments :
 *  Returns the current working simulation. Makes no checks on the state
 *  of the simulation.
 */

extern int Asc_SimsUniqueName(symchar *);
/*
 *  int Asc_SimsUniqueName;
 *  symchar *str;
 *  Comments :
 *  Searches the simulation list for the name of a simulation.
 *  Returns 0 if the name was found else returns 1;
 */

extern int Asc_SimsCmpSim(struct Instance *, struct Instance *);
/*
 *  int Asc_SimsCmpSim;
 *  struct Instance *sim1;
 *  struct Instance *sim2;
 *  Comments :
 *  Compares two simulations, based on their name. Returns 0 if the same.
 *  Returns non-zero if different.
 */

extern int Asc_SimsQueryCmd(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[]);
/*
 *  int Asc_SimsQueryCmd
 *  Registered as : \"sims arg ?args?\";
 *  Comments :
 *  1) sims getcurrent --
 *  Returns the name of the current simualation.
 *  If the simulation is not found then, returns "1" or ""; else it
 *  returns the name of the simulation.
 *  2) sims setcurrent simname --
 *  Searches for the simulation that matches the specified name. If found
 *  sets the current simulation and returns "0". Otherwise it returns "1".
 */

extern int Asc_SimsUniqueNameCmd(ClientData dummy, Tcl_Interp *interp,
                                 int argc, CONST84 char *argv[]);
/*
 *  int Asc_SimsUniqueNameCmd
 *  Comments :
 *  Registered as : unique \"name\";
 *  Returns 1 if name is not a simulation instance or if called with
 *  wrong number of args.  Returns 0 if simulation name exists.
 */

extern int Asc_SimsCreateInstanceCmd(ClientData cdata, Tcl_Interp *interp,
                                     int argc, CONST84 char *argv[]);
/*
 *  int CreateInstance_Callback;
 *  Comments :
 *  A blatant ripofff of Tom Epperlys CreateInstance.
 *  Registered as : screate \"simname\" \"type\".
 *  Creates a simulation instance and adds to the global simlist.
 *  Returns 0 if failed, 1 if ok;
 */

extern int Asc_SimsResumeInstantiateCmd(ClientData cdata, Tcl_Interp *interp,
                                        int argc, CONST84 char *argv[]);
/*
 *  int Asc_SimsResumeInstantiateCmd;
 *  Comments :
 *  Registered as : sresume \"simname\";
 *  Returns "0" if sim not found, 1 if found
 */

extern int Asc_BrowResumeInstantiateCmd(ClientData cdata, Tcl_Interp *interp,
                                        int argc, CONST84 char *argv[]);
/*
 *  int Asc_BrowResumeInstantiateCmd;
 *  Comments :
 *  Registered as : bresume;
 *  browser callback to resume compilation.
 *  assumes instance to be dealt with is in g_root. Returns ""
 */


extern int Asc_SimsCopyInstanceCmd(ClientData cdata, Tcl_Interp *interp,
                                   int argc, CONST84 char *argv[]);
/*
 *  int Asc_SimsCopyInstanceCmd;		KAA_DEBUG
 *  Comments :
 *  Registered as : __sims_copy qlfdid;
 *  Will attempt to take the given qualified id id and 'copy' it.
 *  It will destroy the information immediately after. This is some
 *  experimental code.
 */

extern int Asc_SimsProtoTypeInstanceCmd(ClientData cdata, Tcl_Interp *interp,
                                        int argc, CONST84 char *argv[]);
/*
 *  int Asc_SimsProtoTypeInstanceCmd;		KAA_DEBUG
 *  Comments :
 *  Registered as : __sims_proto qlfdid;
 *  Will attempt to take the given qualified id id and prototype it.
 *  It will add the instance to the prototype library. It will copy
 */

extern int Asc_SimsSaveInstanceCmd(ClientData cdata, Tcl_Interp *interp,
                                   int argc, CONST84 char *argv[]);
/*
 *  inst Asc_SimsSaveInstanceCmd:		KAA_DEBUG
 *  Comments :
 *  Registered as : __sims_saveinst qlfdid file
 *  Will attemptlt to take the given qualified id and 'save' it to
 *  the specified file. This is still experimental code but hopefully
 *  not for long. The details of the save format may be found in
 *  instance_io.[ch]; If all goes well there will be a matching function
 *  called SimsRestoreInstanceCmd();
 */

extern int Asc_SimsUpdateInstanceCmd(ClientData cdata, Tcl_Interp *interp,
     int argc, CONST84 char *argv[]);
/*
 *  int Asc_SimsUpdateInstanceCmd;		KAA_DEBUG
 *  Comments :
 *  Registered as : __sims_update qlfdid <file,type>;
 *  Will attempt to take the given qualified id and 'update' it
 *  using the information found in the file 'file', or using the given
 *  type information. At the moment it reads the instructions from a
 *  file. The primitives used may be found in compiler/instantiate.c
 */

extern void Asc_DeAllocSim(struct Instance *);
/*
 *  void Asc_DeAllocSim(sim);
 *  NOTE:
 *  sim is the *Top* of the simulation and *not* the root. As such it
 *  works on instances of kind SIM_INST.
 *  Deallocate a simulation instance, destroying the instnace and its
 *  associated root instance, by  calling DestroyInstance (in instance.h).
 *  It also frees the name string, which it owns, and cleans up any external
 *  vars associated with the simulation.
 */

extern int Asc_SimsDestroySimulationCmd(ClientData cdata, Tcl_Interp *interp,
                                        int argc, CONST84 char *argv[]);
/*
 *  int Asc_SimsDestroySimulationCmd;
 *  Comments :
 *  Registered as : sdestroy \"simname\";
 *  Returns 1 if successful, returns 0 if not.
 */

extern int Asc_BrowShowPendings(ClientData cdata, Tcl_Interp *interp,
                                int argc, CONST84 char *argv[]);
/*
 *  int Asc_BrowShowPendings;
 *  Comments :
 *  Registered as : \"bnumpendings\" ?simulation?instance? ?sim?search?current?
 *  Call is "bnumpendings simulation sim_name". -- checks given sim_name;
 *        or "bnumpendings instance current". -- checks current instance.
 *        or "bnumpendings instance search".  -- checks search instance.
 *  for the number of pending statements. Will return > 1 if there are
 *  any pending statements.
 *  Returns a BIG number if no instance.
 *  For more details, use BrowWritePendings.
 */

extern int Asc_BrowWritePendingsSTDOUT(ClientData cdata, Tcl_Interp *interp,
                                       int argc, CONST84 char *argv[]);
/*
 *  int Asc_BrowWritePendingsSTDOUT;
 *  Comments : Usage \"bwritependings\" simname.
 *  Will write to stdout, the list of pending statements for the given
 *  simulation. Will return an error if the simulation does not exist or
 *  the sim->root instance is NULL.
 */

extern int Asc_SimListPending(ClientData cdata, Tcl_Interp *interp,
                              int argc, CONST84 char *argv[]);
/*
 *  int Asc_SimListPending ()a;
 *  Tcl registration simlistpending
 *  syntax:  simlistpending simname [optional filename]";
 *  Will print all the unassigned constants and undefined structures and
 *  unexecuted statements in a simulation. simname is the name of an
 *  instance in the global simulation list.
 *  Examples:
 *  simlistpendings tf
 *     writes to stdout the pending messages of simulation tf
 *  simlistpendings tf /tmp/asctfpend1374.1
 *     writes to scratch file /tmp/asctfpend1374. the pending messages of
 *     simulation tf.
 */

STDHLF_H(Asc_SimBinTokenSetOptions);
extern int Asc_SimBinTokenSetOptions(ClientData, Tcl_Interp*, int, CONST84 char**);
/* Registered as: */
#define Asc_SimBinTokenSetOptionsHN "sim_BinTokenSetOptions"
/*  Usage:  */
#define Asc_SimBinTokenSetOptionsHU \
  Asc_SimBinTokenSetOptionsHN " <src obj lib build delete" \
  " maxrels verbose housekeep>" 
#define Asc_SimBinTokenSetOptionsHS \
  "Defines the next set of file names and other misc to build/load with"
#define Asc_SimBinTokenSetOptionsHL "\
 *  Specifies full path names for source, object, and shared object files\n\
 *  to be used in the next compilation of binary token relations.\n\
 *  Names must be consistent and the name of the shared object must\n\
 *  be unique, e.g. where N is a number changing between instantiations:\n\
 *  /tmp/fooN.c /tmp/fooN.o /tmp/fooN.so <build command> /bin/rm 0 1 1000.\n\
 *  verbose is a code generation option 1 makes more readable code which\n\
 *  may be slower for cc to digest. housekeep if 1 causes intermediate\n\
 *  files to be aggressively deleted. maxrels is the most equations to\n\
 *  be allowed in one generated file. If more are needed, binary will not\n\
 *  be generated. If maxrels = 0, we ignore C generation completely.\n"
#define Asc_SimBinTokenSetOptionsHL2 "\
 *  Bugs: needs to have additional arguments for coping with F77, Java, etc.\n\
 *  Note: This function is called indirectly from sim_create, because\n\
 *  we need consistently updated version values and don't trust the tcl\n\
 *  programmer to remember that. Tinkering, if needed, should be done via\n\
 *  Sim_SetupBinTokenCC in LibraryProc.tcl\n\
"


#endif /*SimsProc_module_loaded*/
