/*
 *  BrowserProc.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: BrowserProc.h,v $
 *  Date last modified: $Date: 2003/08/23 18:43:04 $
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

#ifndef BrowserProc_module_loaded
#define BrowserProc_module_loaded

/*
 *  To include this header, you must include the following:
 *      #include "tcl.h"
 *      #include "compiler/instance_enum.h"
 *      #include "interface/BrowserProc.h"
 */


/*
 *  The root instance and the current instance in the Browser
 */
extern struct Instance *g_root;
extern struct Instance *g_curinst;



extern int Asc_BrowRootInitCmd(ClientData cdata, Tcl_Interp *interp,
                               int argc, CONST84 char *argv[]);
/*
 *  Asc_BrowRootInitCmd
 *  Comments : rootinit;
 *  This should set initialize the root.
 */

extern int Asc_BrowRootCmd(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char *argv[]);
/*
 *  This should set the root.
 *  Comments : root $arg$
 */

extern int Asc_BrowRootBackupCmd(ClientData cdata, Tcl_Interp *interp,
                                 int argc, CONST84 char *argv[]);
/*
 *  This should backup to the old root.
 *  Comments : oldinst -- wil be supseded by the general inst routines.
 */

extern int Asc_BrowRootNCmd(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[]);
/*
 *  This should set the root. Uses direct addressing. The instance must
 *  already exist in the instance query list. For this to be used.
 *  Comments : rootn $arg$
 */

extern int Asc_BrowTransferCmd(ClientData cdata, Tcl_Interp *interp,
                               int argc, CONST84 char *argv[]);
/*
 *  Used to transfer from a search instance to the maim browser. One of
 *  the main steps in exportin to the browser from any window. The sims
 *  export to browser is a easier case and can be handled through rootinit.
 */

extern int Asc_BrowSimListCmd(ClientData cdata, Tcl_Interp *interp,
                              int argc, CONST84 char *argv[]);
/*
 *  Should list all the active simulations. -- registered as slist.
 */

extern int Asc_BrowSimTypeCmd(ClientData cdata, Tcl_Interp *interp,
                              int argc, CONST84 char *argv[]);
/*
 *  Returns the type of a simulation given the name of the simulation.
 *  Will return an error message if the simlist is null or the name not
 *  found.
 */

extern int Asc_BrowInstStatCmd(ClientData cdata, Tcl_Interp *interp,
                               int argc, CONST84 char *argv[]);
/*
 *  print instance tree stats to stdout, a la Tom.
 */

extern int Asc_BrowInstListCmd(ClientData cdata, Tcl_Interp *interp,
                               int argc, CONST84 char *argv[]);
/*
 *  Should list all the working instances  -- registered as ilist.
 */

extern int Asc_BrowPrintCmd(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[]);
/*
 *  Should print the current instance or the simulation list if no
 *  arguements are given. -- registered as bprint.
 */

extern int Asc_BrowInstQueryCmd(ClientData cdata, Tcl_Interp *interp,
                                int argc, CONST84 char *argv[]);
/*
 *  Asc_BrowInstQueryCmd
 *  Comments : inst querykind [cur,search];
 *  arg : name, type, old, nchild, nparents, child, parent,
 *  isassignable, isfixable, ismutable, isconstant, iswhenvar,
 *  atomchild;
 *  Returns the name, type, previous instance, # of children, # of parents,
 *  child list, parents list, and  if parent atomic, respectively.
 *  It also returns the appropriate value (TRUE or FALSE) fo the queries:
 *  is the instance assignable ? fixable ? mutable ? constant ?
 *  is it in the list of variables of a WHEN statement ?
 *  If current instance is null, returns NULL_INSTANCE and TCL_ERROR.
 *  Takes the search instance also. if no third argument given, uses
 *  current.
 */

extern int Asc_BrowRunInitializeCmd(ClientData cdata,Tcl_Interp *interp,
                                    int argc, CONST84 char *argv[]);
/*
 *  Asc_BrowRunInitializeCmd
 *  Comments : \"runproc\" $name$
 *  Will take the current instance and run the named procedure associated
 *  with it. Returns an TCL_OK or TCL_ERROR status flag.
 */

extern int Asc_BrowInstanceMergeCmd(ClientData cdata,Tcl_Interp *interp,
                                    int argc, CONST84 char *argv[]);
/*
 *  Asc_BrowInstanceMergeCmd
 *  Comments : \"bmerge\" <noargs>
 *  NOTE WELL -- this function will attempt to merge the current instance
 *  g_curinst and the search instance g_search_inst. Hence to set up
 *  g_search_inst appropriately, call Asc_BrowQlfdidSeachCmd FIRST.
 */

extern int Asc_BrowInstanceRefineCmd(ClientData cdata,Tcl_Interp *interp,
                                     int argc, CONST84 char *argv[]);
/*
 *  Asc_BrowInstanceRefineCmd
 *  Comments : \"brefine\" ?current?search? type;
 *  Will take the current or search instance as specified and will try
 *  to refine it to the specified type. Will return TCL_OK if all works
 *  and a spew of TCL_ERRORs if things are not perfect. Note this function
 *  makes use of RefineClique in instance.c and hence refines all the
 *  members of the clique.
 */

extern int Asc_BrowMakeAlikeCmd(ClientData cdata,Tcl_Interp *interp,
                                int argc, CONST84 char *argv[]);
/*
 *  int Asc_BrowMakeAlikeCmd;
 *  Comments : \"bmakealike\" current search.
 *  Will make two instances ARE_ALIKE.
 *  Operates on the current and the search instances.
 */

/*
 *  Other useful queries exported for general use.
 */

extern int Asc_BrowInstIsAtomic(struct Instance *);
/*
 *  int Asc_BrowInstIsAtomic(i);
 *  Returns true if the instance is one of the atom types. It will fiil
 *  for relations, as at the moment they are not strictly considered
 *  as being atomic.
 */

extern int Asc_BrowInstIsSubAtomic(struct Instance *);
/*
 *  int Asc_BrowInstIsSubAtomic(i);
 *  Returns true if the instance is a child of one of the atom types.
 *  i.e, sub-atomic :-)
 */

extern int Asc_BrowInstIsConstant(struct Instance *);
/*
 *  int Asc_BrowInstIsConstant(i);
 *  Returns true if the instance is a constant.
 */

extern int Asc_BrowInstIsMutable(struct Instance *);
/*
 *  int Asc_BrowInstIsMutable(i);
 *  Returns true if the instance is mutable. The instance must have the
 *  notion of a *value*. Otherwise returns false.
 */

STDHLF_H(Asc_BrowAnonTypesCmd);
extern int Asc_BrowAnonTypesCmd(ClientData, Tcl_Interp*, int, CONST84 char**);
/* Registered as: */
#define Asc_BrowAnonTypesCmdHN "__brow_anontypes"
/*  Usage:  */
#define Asc_BrowAnonTypesCmdHU \
  "libr_anontypes <-current,-search>"
#define Asc_BrowAnonTypesCmdHS \
  "Times the execution of anonymous type classification and prints results"
#define Asc_BrowAnonTypesCmdHL \
"\
 * This is a testing interface for timing anonymous type classification.\n\
 * It has no other particular use since no result is obtainable from the\n\
 * TCL interpreter on return.\n\
"
/* */


#endif /* BrowserProc_module_loaded */
