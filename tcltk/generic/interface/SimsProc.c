/*
 *  SimsProc.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.31 $
 *  Version control file: $RCSfile: SimsProc.c,v $
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


#include <ctype.h>
#include <tcl.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/tm_time.h>
#include <general/list.h>
#include <general/dstring.h>
#include <compiler/compiler.h>
#include <compiler/instance_enum.h>
#include <compiler/cmpfunc.h>
#include <compiler/fractions.h>
#include <compiler/dimen.h>
#include <compiler/types.h>
#include <compiler/stattypes.h>
#include <compiler/statement.h>
#include <compiler/slist.h>
#include <compiler/syntax.h>
#include <compiler/prototype.h>
#include <compiler/symtab.h>
#include <compiler/instance_io.h>
#include <compiler/instance_name.h>
#include <compiler/parentchild.h>
#include <compiler/instquery.h>
#include <compiler/extinst.h>
#include <compiler/child.h>
#include <compiler/type_desc.h>
#include <compiler/copyinst.h>
#include <compiler/destroyinst.h>
#include <compiler/module.h>
#include <compiler/library.h>
#include <compiler/name.h>
#include <compiler/pending.h>
#include <compiler/check.h>
#include <compiler/stattypes.h>
#include <compiler/relation_type.h>
#include <compiler/bintoken.h>
#include <compiler/instantiate.h>
#include <compiler/value_type.h>
#include <compiler/statio.h>
#include <compiler/bit.h>
#include <compiler/simlist.h>
#include <compiler/qlfdid.h>
#include <solver/slv_types.h>
#include "HelpProc.h"
#include "Qlfdid.h"
#include "Driver.h"
#include "BrowserProc.h"
#include "HelpProc.h"
#include "SimsProc.h"

#ifndef lint
static CONST char SimsProcID[] = "$Id: SimsProc.c,v 1.31 2003/08/23 18:43:08 ballan Exp $";
#endif

int Asc_SimsQueryCmd(ClientData cdata, Tcl_Interp *interp,
                 int argc, CONST84 char *argv[])
{
  struct Instance *sim;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if (( argc == 2 ) && ( (strncmp(argv[1],"getcurrent",4)) == 0 )) {
    sim = Asc_GetCurrentSim();
    if (sim) {
      Tcl_AppendResult(interp,(char *)SCP(GetSimulationName(sim)),
                       (char *)NULL);
      return TCL_OK;
    } else {
      Tcl_SetResult(interp, "1", TCL_STATIC);
      return TCL_OK;
    }
  }

  if (( argc == 3 ) && ( (strncmp(argv[1],"setcurrent",4)) == 0 )) {
    sim = Asc_FindSimulationTop(AddSymbol(argv[2]));
    if (sim) {
      Asc_SetCurrentSim(sim);
      Tcl_SetResult(interp, "0", TCL_STATIC);
      return TCL_OK;
    } else {
      Tcl_SetResult(interp, "1", TCL_STATIC);
      return TCL_OK;
    }
  }

  Tcl_AppendResult(interp,"Invalid args: Usage ", argv[0],
                   " <setcurrent,getcurrent>", (char *)NULL);
  return TCL_ERROR;
}

int Asc_SimsUniqueNameCmd(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
     Tcl_SetResult(interp, "call is: unique \"name\"", TCL_STATIC);
     return TCL_ERROR;
  }
  if (Asc_SimsUniqueName(AddSymbol(argv[1]))) {
     Tcl_SetResult(interp, "1", TCL_STATIC);
  } else {
     Tcl_SetResult(interp, "0", TCL_STATIC);
  }
  return TCL_OK;
}

int Asc_SimsCreateInstanceCmd(ClientData cdata, Tcl_Interp *interp,
                              int argc, CONST84 char *argv[])
{
  struct TypeDescription *type;
  struct Instance *new;
  symchar *name, *defmethod;
  int len, format, k;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if (( argc < 3 ) || ( argc > 6 )) {
    Tcl_AppendResult(interp,"wrong # args:",
                     "Usage sim_create <instance_name> <instance_type>",
                     "[-m defaultMethod] [-p,-r]",(char *)NULL);
    return TCL_ERROR;
  }

  name = AddSymbol(argv[1]);
  len = SCLEN(name);
  if ((len<1) || (len > 70)) {
    Tcl_SetResult(interp, "Simulation name too long (> 70)", TCL_STATIC);
    return TCL_ERROR;
  }
  if (isdigit(SCP(name)[0])) {
    Tcl_SetResult(interp, "Instance names cannot begin with a digit",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  type = FindType(AddSymbol(argv[2]));
  if (type==NULL) {
    Tcl_AppendResult(interp,"CreateInstance called with unknown type",
                     (char *)NULL);
    return TCL_ERROR;
  }

  /*
   * Check for additional compilation flags.
   */
  format = 0;			/* do standard compilation */
  k = 3;
  defmethod = AddSymbol("default_self"); /* when no input with -m */
  while (k < argc) {
    if (strcmp(argv[k],"-r")==0) {
      format = 1;			/* do no relations */
      k++;
      continue;
    }
    if (strcmp(argv[k],"-p")==0) {
      format = 2;			/* do patch compilation */
      k++;
      continue;
    }
    if (strcmp(argv[k],"-m")==0) {
      if ( (k+1)== argc) {
        k++;
        defmethod = NULL; /* -m <noargs> --> no defaults */
      } else {
        defmethod = AddSymbol(argv[k+1]);
        k += 2;
      }
      continue;
    }
    Tcl_AppendResult(interp,"Unknown compilation flag",(char *)NULL);
    return TCL_ERROR;
  }

  if (Asc_SimsUniqueName(name)) {
    Tcl_GlobalEval(interp,"Sims_SetupBinTokenCC");
    new = SimsCreateInstance(GetName(type),name,format,defmethod);
    if (new != NULL) {
      gl_insert_sorted(g_simulation_list,new,(CmpFunc)Asc_SimsCmpSim);
      Tcl_SetResult(interp, "0", TCL_STATIC);
      return TCL_OK;
    } else { /* Instantiate returned null */
      Tcl_AppendResult(interp,"Simulation of type ",argv[2]," with name ",
                       (char *)SCP(name),
                       " cannot be instantiated.", (char *)NULL);
      return TCL_ERROR;
    }
  } else {				/* sim already exists */
    Tcl_AppendResult(interp,"A simulation of the name ",(char *)SCP(name),
                     " already exists", (char *)NULL);
    return TCL_ERROR;
  }
#ifndef NDEBUG
  return TCL_OK; /* NOT reached */
#endif
}

/*
 *********************************************************************
 * Resume Instantiation functions
 *
 * An experimental UpdateInstanceCmd is included here.
 *
 * Query ? Is it possible for Reinstantiate to move the root instance ?
 * Not sure. At the moment it does not return an instance pointer.
 *********************************************************************
 */

int Asc_SimsResumeInstantiateCmd(ClientData cdata, Tcl_Interp *interp,
                             int argc, CONST84 char *argv[])
{
  double start_time =0;
  struct Instance *xisting;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "call is: sresume <instancename> ", TCL_STATIC);
    return TCL_ERROR;
  }
  xisting = Asc_FindSimulationRoot(AddSymbol(argv[1]));
  if (xisting) {
    if (g_compiler_timing) {
      start_time = tm_cpu_time();
    }
    ReInstantiate(xisting);
    if (g_compiler_timing) {
      FPRINTF(stdout,"Reinstantiation CPU time = %g seconds\n",
              tm_cpu_time() - start_time);
    }
    Tcl_SetResult(interp, "0", TCL_STATIC);
    return TCL_OK;
  } else {
    Tcl_AppendResult(interp,"Instance ", argv[1], "not found",(char *)NULL);
    return TCL_ERROR;
  }
}

int Asc_SimsUpdateInstanceCmd(ClientData cdata, Tcl_Interp *interp,
                          int argc, CONST84 char *argv[])
{
  struct Instance *target;
  struct TypeDescription *desc, *patchdef;
  CONST struct StatementList *slist;
  double start_time;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 3 ) {
    Tcl_SetResult(interp, "call is: __sims_update qlfdid type", TCL_STATIC);
    return TCL_ERROR;
  }
  /*
   * Find the target instance, and leave g_search_inst
   * looking at it.
   */
  if (Asc_QlfdidSearch3(argv[1],0)) {
    Tcl_SetResult(interp, "target instance not found", TCL_STATIC);
    return TCL_ERROR;
  }
  target = g_search_inst;

  patchdef = FindType(AddSymbol(argv[2]));
  if (!patchdef) {
    Tcl_SetResult(interp, "Cannot find patch type", TCL_STATIC);
    return TCL_ERROR;
  }
  desc = InstanceTypeDesc(target);
  if (GetPatchOriginal(patchdef)!=desc) {
    Tcl_SetResult(interp, "Inconsistent types in patch", TCL_STATIC);
    return TCL_ERROR;
  }

  start_time = tm_cpu_time();		/* do the update */
  slist = GetStatementList(patchdef);
  UpdateInstance(g_root,target,slist);
  if (g_compiler_timing) {
    PRINTF("Reinstantiation CPU time = %g seconds\n",
             tm_cpu_time() - start_time);
  }

  return TCL_OK;
}


int Asc_SimsCopyInstanceCmd(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  struct Instance *target;
  double start_time;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "call is: __sims_copy qlfdid", TCL_STATIC);
    return TCL_ERROR;
  }

  /*
   * Find the target instance, and leave g_search_inst
   * looking at it.
   */
  if (Asc_QlfdidSearch3(argv[1],0)) {
    Tcl_SetResult(interp, "target instance not found", TCL_STATIC);
    return TCL_ERROR;
  }

  start_time = tm_cpu_time();
  target = g_search_inst;
  target = CopyInstance(target);
  start_time = tm_cpu_time() - start_time;
  FPRINTF(stderr,"Time to copy instance = %g\n",start_time);
  DestroyInstance(target,NULL);

  return TCL_OK;
}


int Asc_SimsProtoTypeInstanceCmd(ClientData cdata, Tcl_Interp *interp,
                             int argc, CONST84 char *argv[])
{
  struct Instance *target, *result;
  CONST struct TypeDescription *desc;
  double start_time;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "call is: __sims_proto qlfdid", TCL_STATIC);
    return TCL_ERROR;
  }

  if (Asc_QlfdidSearch3(argv[1],0)) {
    Tcl_SetResult(interp, "target instance not found", TCL_STATIC);
    return TCL_ERROR;
  }

  start_time = tm_cpu_time();
  target = g_search_inst;
  switch (InstanceKind(target)) {
  case ARRAY_ENUM_INST:
  case ARRAY_INT_INST:
  case SIM_INST:
    Tcl_SetResult(interp, "Cannot prototype this type of instance",TCL_STATIC);
    return TCL_ERROR;
  default:
    break;
  }

  desc = InstanceTypeDesc(target);
  if (LookupPrototype(GetName(desc))) {
    Tcl_SetResult(interp, "a prototype already exists", TCL_STATIC);
    return TCL_OK;
  }
  result = CopyInstance(target);
  start_time = tm_cpu_time() - start_time;
  if (result) {
    AddPrototype(result);
    FPRINTF(stderr,"Time to prototype instance = %g\n",start_time);
    return TCL_OK;
  } else {
    Tcl_SetResult(interp, "Error in prototyping instance", TCL_STATIC);
    return TCL_ERROR;
  }
}


int Asc_SimsSaveInstanceCmd(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  FILE *fp = NULL;
  struct Instance *target;
  double start_time;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 3 ) {
    Tcl_SetResult(interp, "call is: __sims_saveinst qlfdid file", TCL_STATIC);
    return TCL_ERROR;
  }

  /*
   * Find the target instance, and leave g_search_inst
   * looking at it.
   */
  if (Asc_QlfdidSearch3(argv[1],0)) {
    Tcl_SetResult(interp, "target instance not found", TCL_STATIC);
    return TCL_ERROR;
  }
  /*
   * Set up the file pointer.
   */
  fp = fopen(argv[2],"w");
  if (!fp) {
    Tcl_SetResult(interp, "error in opening file", TCL_STATIC);
    return TCL_ERROR;
  }

  start_time = tm_cpu_time();
  target = g_search_inst;
  SaveInstance(fp,target,0);
  start_time = tm_cpu_time() - start_time;
  FPRINTF(stderr,"Time to save instance = %g\n",start_time);

  if (fp) {
    fclose(fp);
  }
  return TCL_OK;
}



/*
 *********************************************************************
 * Destroy Simulation functions
 *
 * The DestroyInstance code correctly deals with *all*
 * instances. As such SIM_INST are correctly handled as well
 * DO NOT refer to it after this. Parts of the sim instance that
 * are shared by other instances, (as in the case of universals)
 * are properly handled, and will not be deallocated unless this
 * simulation is the only parent.
 *********************************************************************
 */

int Asc_SimsDestroySimulationCmd(ClientData cdata, Tcl_Interp *interp,
                               int argc, CONST84 char *argv[])
{
  struct Instance *xisting, *current;
  unsigned long ndx;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "wrong # args: Usage: sim_destroy <simname>",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  xisting = Asc_FindSimulationTop(AddSymbol(argv[1]));
  if (xisting) {
    /* the next line grabs the index in the list */
    ndx = gl_search(g_simulation_list,xisting,(CmpFunc)Asc_SimsCmpSim);
    current = Asc_GetCurrentSim();
    if (current) {
      if (Asc_SimsCmpSim(xisting,current)==0) {
        Asc_SetCurrentSim(NULL);
        Asc_DeAllocSim(xisting);
        gl_delete(g_simulation_list,ndx,0);
        Tcl_AppendResult(interp,"Simulation ",argv[1], " deleted",
                         (char *)NULL);
        return TCL_OK;
      }
    }
    Asc_DeAllocSim(xisting);		/* prepares for cleanup */
    gl_delete(g_simulation_list,ndx,0);
    Tcl_AppendResult(interp,"Simulation ",argv[1], " deleted", (char *)NULL);
    return TCL_OK;
  } else {
    Tcl_AppendResult(interp,"Simulation ",argv[1]," not found",(char *)NULL);
    return TCL_ERROR;
  }
}

int Asc_BrowShowPendings(ClientData cdata, Tcl_Interp *interp,
                    int argc, CONST84 char *argv[])
{
  struct Instance *i = NULL;
  char buf[MAXIMUM_NUMERIC_LENGTH];       /* string to hold long */
  unsigned long p=0L;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 3 ) {
    Tcl_AppendResult(interp,"wrong # args: Usage: ",
                     "\"bnumpendings\" ?sim?inst ?simname?current?search?",
                     (char *)NULL);
    return TCL_ERROR;
  }
  if ( (strncmp(argv[1],"simulation",3)) == 0 ) {
    i = Asc_FindSimulationRoot(AddSymbol(argv[2]));
  } else if ( (strncmp(argv[1],"instance",3)) == 0 ) {
    if ( (strncmp(argv[2],"current",3)) == 0 ) {
      i = g_curinst;
    } else {
      i = g_search_inst;
    }
  }
  if (!i) {
    Tcl_SetResult(interp, "10000", TCL_STATIC); /* Just some big number */
    return TCL_OK;
  }
  p = NumberPendingInstances(i);
  if (p>0L) {
    sprintf(buf,"%lu",p);
    Tcl_SetResult(interp, buf, TCL_VOLATILE);
  }
  return TCL_OK;
}

static
void BrowWritePendingStatements(FILE *f, CONST struct Instance *i)
{
  CONST struct BitList *blist;
  CONST struct TypeDescription *desc;
  CONST struct StatementList *slist;
  CONST struct Statement *stat;
  CONST struct gl_list_t *list;
  unsigned long c,len;
  blist = InstanceBitList(i);
  if ((blist!=NULL)&&(!BitListEmpty(blist))) {
    FPRINTF(stderr,"PENDING STATEMENTS\n");
    desc = InstanceTypeDesc(i);
    slist = GetStatementList(desc);
    list = GetList(slist);
    len = gl_length(list);
    for(c=1;c<=len;c++) {
      if (ReadBit(blist,c-1)) {
        stat = (struct Statement *)gl_fetch(list,c);
        WriteStatement(f,stat,4);
        if (StatementType(stat)== SELECT) {
          c = c + SelectStatNumberStats(stat);
        }
      }
    }
  }
}

int Asc_BrowWritePendingsSTDOUT(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[])
{
  struct Instance *i;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "wrong # args : Usage \"bwritependings\" simname",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  i = Asc_FindSimulationRoot(AddSymbol(argv[1]));
  if (!i) {
    Tcl_SetResult(interp, "given simulation nonexistent !", TCL_STATIC);
    return TCL_ERROR;
  }
  BrowWritePendingStatements(stdout,i);
  return TCL_OK;
}

int Asc_SimListPending(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{
  struct Instance *i;
  FILE *fp = stdout;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if (( argc != 2 ) && ( argc != 3 )) {
    Tcl_SetResult(interp,
                  "simlistpending: simlistpending simname [optional filename]",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  i = Asc_FindSimulationRoot(AddSymbol(argv[1]));
  if (!i) {
    Tcl_SetResult(interp, "simlistpending: given simulation not found!",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if ( argc == 3 ) {
    fp = fopen(argv[2],"w+");
    if (fp==NULL) {
      Tcl_SetResult(interp, "simlistpending: given bad filename", TCL_STATIC);
      return TCL_ERROR;
    }
  }
  FPRINTF(fp,"\n-------------------------------------\n");
  FPRINTF(fp," Pendings statements for simulation %s\n\n",argv[1]);
  CheckInstance(fp,i);
  FPRINTF(fp,"\n-------------------------------------\n");
  if ( argc == 3 ) {
    fclose(fp);
  }
  return TCL_OK;
}


STDHLF(Asc_SimBinTokenSetOptions, (Asc_SimBinTokenSetOptionsHL,Asc_SimBinTokenSetOptionsHL2,HLFSTOP));
int Asc_SimBinTokenSetOptions(ClientData cdata, Tcl_Interp *interp,
                              int argc, CONST84 char **argv)
{
  char *srcname, *objname, *libname, *buildcommand, *unlinkcommand;
  long maxrels;
  int verbose, housekeep;
  int s1,s2,s3;

  ASCUSE;  /* see if first arg is -help */
  if (argc != 9 ) {
    Asc_HelpGetUsage(interp,Asc_SimBinTokenSetOptionsHN);
    return TCL_ERROR;
  };
  srcname = ascstrdup(argv[1]);
  objname = ascstrdup(argv[2]);
  libname = ascstrdup(argv[3]);
  buildcommand = ascstrdup(argv[4]);
  unlinkcommand = ascstrdup(argv[5]);
  s1 = Tcl_ExprLong(interp,argv[6],&maxrels);
  Tcl_ResetResult(interp);
  s2 = Tcl_GetInt(interp,argv[7],&verbose);
  Tcl_ResetResult(interp);
  s3 = Tcl_GetInt(interp,argv[8],&housekeep);
  Tcl_ResetResult(interp);
  if (srcname == NULL || objname == NULL || libname == NULL ||
      buildcommand == NULL || unlinkcommand == NULL ||
      s1 != TCL_OK || s2 != TCL_OK || s3 != TCL_OK) {
    Tcl_AppendResult(interp,argv[0],": Error converting input",(char *)NULL);
    return TCL_ERROR;
  }
  BinTokenSetOptions(srcname,objname,libname,buildcommand,unlinkcommand,
                     (unsigned long)maxrels,verbose,housekeep);
  return TCL_OK;
}
