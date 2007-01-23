/*
 *  BrowserProc.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.36 $
 *  Version control file: $RCSfile: BrowserProc.c,v $
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

#define ASC_BUILDING_INTERFACE

#include <stdarg.h>
#include <time.h>
#include <tcl.h>
#include <utilities/ascConfig.h>
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <general/tm_time.h>
#include <general/list.h>
#include <general/dstring.h>

#include <compiler/instance_enum.h>
#include <compiler/cmpfunc.h>
#include <compiler/check.h>
#include <compiler/fractions.h>
#include <compiler/dimen.h>
#include <compiler/expr_types.h>
#include <compiler/relation_type.h>
#include <compiler/setinstval.h>
#include <compiler/extfunc.h>
#include <compiler/find.h>
#include <compiler/functype.h>
#include <compiler/safe.h>
#include <compiler/rel_blackbox.h>
#include <compiler/vlist.h>
#include <compiler/relation.h>
#include <compiler/relation_util.h>
#include <compiler/logical_relation.h>
#include <compiler/logrelation.h>
#include <compiler/logrel_util.h>
#include <compiler/symtab.h>
#include <compiler/instance_io.h>
#include <compiler/instance_name.h>
#include <compiler/instquery.h>
#include <compiler/parentchild.h>
#include <compiler/atomvalue.h>
#include <compiler/mathinst.h>
#include <compiler/mergeinst.h>
#include <compiler/child.h>
#include <compiler/type_desc.h>
#include <compiler/refineinst.h>
#include <compiler/stattypes.h>
#include <compiler/instantiate.h>
#include <compiler/module.h>
#include <compiler/library.h>
#include <compiler/simlist.h>
#include <compiler/anontype.h>
#include <compiler/qlfdid.h>
#include <solver/slv_types.h>
#include "HelpProc.h"
#include "BrowserProc.h"
#include "Qlfdid.h"
#include "UnitsProc.h"
#include "SimsProc.h"
#include "Commands.h"
#include "Driver.h"

#ifndef lint
static CONST char BrowserProcID[] = "$Id: BrowserProc.c,v 1.36 2003/08/23 18:43:04 ballan Exp $";
#endif


#ifndef MAXIMUM_STRING_LENGTH
#define MAXIMUM_STRING_LENGTH 1024
#endif
#define MAXIMUM_INST_DEPTH 40
/* #define MAXIMUM_ID_LENGTH 40 // defined in compiler/qlfdid.h now */


unsigned long g_depth = 0;      /* depth of the instance query list */
struct Instance *g_instlist[MAXIMUM_INST_DEPTH];
struct Instance *g_root = NULL;         /* root instance */
struct Instance *g_curinst = NULL;      /* the current instance */


static
void InitInstList(void)
{
  unsigned long c;
  for (c=0; c<MAXIMUM_INST_DEPTH; c++) {
    g_instlist[c] = NULL;
  }
  g_depth = 0;
  FPRINTF(ASCERR,"g_instlist initialized\n");
  FFLUSH(ASCERR);
}

static
unsigned long ChildNumberbyChar(struct Instance *i, char *name)
{
  struct InstanceName rec;
  symchar *sym;
  unsigned long c = 0;
  unsigned long nch = 0;
  long iindex;

  if((!i)||(!name)) {
    FPRINTF(ASCERR,"Null Instance or name in ChildbyNameChar\n");
    FFLUSH(ASCERR);
    return 0;
  }
  nch = NumberChildren(i);
  sym = AddSymbol(name);
  if(!nch) {
    return 0;
  }
  do {
    c++;
    rec = ChildName(i,c);
    switch (InstanceNameType(rec)) {
    case StrName:
      if (CmpSymchar(InstanceNameStr(rec), sym)==0) {
        return c;
      }
      break;
    case IntArrayIndex:
      iindex = atol(name); /* fixme strtod */
      if (iindex==InstanceIntIndex(rec)) {
        return c;
      }
      break;
    case StrArrayIndex:
      if (CmpSymchar(InstanceStrIndex(rec), sym)==0) {
        return c;
      }
      break;
    }
  } while(c < nch);
  return 0; /*NOTREACHED*/
}

static
int BrowRootInit(char *sim_name)
{
  struct Instance *ptr;
  if (!sim_name) {
    return 1;
  }
  InitInstList();    /* initialize the g_instlist */
  ptr = Asc_FindSimulationRoot(AddSymbol(sim_name));
  if (ptr) {
    g_root = g_curinst = ptr;
    g_depth = 1;
    g_instlist[g_depth] = g_root; /* we only use positions 1 forward */
    Asc_SetCurrentSim(Asc_FindSimulationTop(AddSymbol(sim_name)));
    return 0;
  } else {
    g_root = g_curinst = NULL;
    InitInstList();
    g_depth = 0;
    Asc_SetCurrentSim(NULL);
    return 1;
  }
}

int Asc_BrowRootInitCmd(ClientData cdata, Tcl_Interp *interp,
                    int argc, CONST84 char *argv[])
{
  /*  Initializes the browser root and search instances.
   *  If called with one arg, it must be the name of a simulation.
   *  Format -- rootinit $arg$
   */
  int nok;

  UNUSED_PARAMETER(cdata);

  if( argc == 1 ) {
    g_root = g_curinst = NULL;
    g_depth = 0;
    Tcl_SetResult(interp, "g_instlist initialized\n", TCL_STATIC);
    return TCL_OK;
  } else if ( argc == 2 ) {
    nok = BrowRootInit(QUIET(argv[1]));
    if (nok) {
      Tcl_SetResult(interp, "simulation not found in \"rootinit\"",TCL_STATIC);
      return TCL_ERROR;
    }
  } else {
    Tcl_SetResult(interp, "wrong # args: Usage \"rootinit $sim$\"",TCL_STATIC);
    return TCL_ERROR;
  }
  return TCL_OK;
}

int Asc_BrowRootCmd(ClientData cdata, Tcl_Interp *interp,
                int argc, CONST84 char *argv[])
{
/* This command takes the form : root $arg1$.
   This will set the current search positions.
*/
  unsigned long nch,c;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "wrong # args to root", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_curinst==NULL) {
    Tcl_SetResult(interp, "Call exp_b $sim$  first!!", TCL_STATIC);
    return TCL_ERROR;
  }
  nch = NumberChildren(g_curinst);
  if (nch) {
    c = ChildNumberbyChar(g_curinst,QUIET(argv[1]));
    if (c) {
      g_curinst = InstanceChild(g_curinst,c);
      g_depth++;
      g_instlist[g_depth] = g_curinst;
      return TCL_OK;
    } else {
      Tcl_SetResult(interp, "Child not found - check your root", TCL_STATIC);
      return TCL_ERROR;
    }
  } else {
    Tcl_SetResult(interp, "At leaves of the Instance Tree", TCL_STATIC);
    return TCL_ERROR;  /* maybe not an error */
  }
}

int Asc_BrowRootNCmd(ClientData cdata, Tcl_Interp *interp,
                 int argc, CONST84 char *argv[])
{
/* This command takes the form : rootn $arg1$. where arg is numeric.
   This will set the current search positions.
*/
  unsigned long iindex;
  struct Instance *i;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "wrong # args to \"rootn\"", TCL_STATIC);
    return TCL_ERROR;
  }
  iindex = atol(argv[1]);
  if((iindex >= MAXIMUM_INST_DEPTH) || (iindex < 1)) {
    Tcl_SetResult(interp, "Invalid args to \"rootn\"", TCL_STATIC);
    return TCL_ERROR;
  }
  /* Three cases to consider.
   *  1) index < g_depth; -- already exists, so just point; adjust g_depth.
   *  2) index = g_depth; -- do nothing -- we should already be looking here.
   *  3) index > g_depth; -- invalid , we MUST have a name, so use root.
   */
  if (iindex < g_depth) {
    i = g_instlist[iindex]; /* should maybe check for iindex = 1*/
    if(i) {
      g_depth = iindex;
      g_curinst = g_instlist[g_depth];
      return TCL_OK;
    } else {
      Tcl_SetResult(interp, "Instance for this index, is NULL or not found",
                    TCL_STATIC);
      return TCL_ERROR;
    }
  }
  if (iindex==g_depth) {
    return TCL_OK;
  }
  if (iindex > (g_depth)) {
    Tcl_SetResult(interp, "Invalid index to \"rootn\" use \"root\" instead",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  return TCL_ERROR;  /* not reached */
}

int Asc_BrowRootBackupCmd(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  struct Instance *newi;

  UNUSED_PARAMETER(cdata);
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    Tcl_SetResult(interp, "wrong # args to oldinst", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_depth==1) {
    Tcl_SetResult(interp, "Already at root; Cant backup", TCL_STATIC);
    return TCL_OK;
  }
  g_curinst = g_instlist[g_depth];
  if( ! g_curinst ) {
    Tcl_SetResult(interp, "Current Instance is NULL; not backing up!!",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  g_depth--;
  newi = g_instlist[g_depth];
  if (newi) {
    g_curinst = newi;
    return TCL_OK;
  } else {
    Tcl_SetResult(interp, "Something is wrong -- previous inst NULL",
                  TCL_STATIC);
    return TCL_ERROR;
  }
}


static
int BrowTransfer(struct gl_list_t *search_list)
{
  struct SearchEntry *se;
  unsigned long c,len;
  char *sim_name;
  int nok;
  len = gl_length(search_list);
  if (!len) {
    return 1;
  }
  se = (struct SearchEntry *)gl_fetch(search_list,1); /*  1st is sim name */
  sim_name = Asc_SearchEntryName(se);
  nok = BrowRootInit(sim_name);                       /* sets current etc */
  if (nok) {
    return (nok);
  }
  for(c=1;c<=len;c++) {
    se = (struct SearchEntry *)gl_fetch(search_list,c);
    g_instlist[c] = Asc_SearchEntryInstance(se);
  }
  g_depth = len;
  g_curinst = g_instlist[len];
  return 0;
}

int Asc_BrowTransferCmd(ClientData cdata, Tcl_Interp *interp,
                    int argc, CONST84 char *argv[])
{
  /* Format : \"transfer name\" */
  char temp[MAXIMUM_ID_LENGTH];
  struct gl_list_t *search_list;
  int nok;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "wrong # args : Usage is \"transfer name\"",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  search_list = Asc_BrowQlfdidSearch(QUIET(argv[1]),temp);
  if ((g_search_inst==NULL) || (search_list==NULL)) {
    Tcl_AppendResult(interp,"Search instance not found\n",temp,(char *)NULL);
    return TCL_ERROR;
  }
  nok = BrowTransfer(search_list);
  if (nok) {
    Tcl_SetResult(interp, "Major Error in BrowTransfer - contact abbott@globe",
                  TCL_STATIC);
  }
  Asc_SearchListDestroy(search_list);
  return TCL_OK;
}

int Asc_BrowSimListCmd(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{
  struct Instance *sptr;
  struct gl_list_t *sl;
  unsigned long len, c;

  UNUSED_PARAMETER(cdata);
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    Tcl_SetResult(interp, "wrong # args to \"slist\"", TCL_STATIC);
    return TCL_ERROR;
  }
  sl = g_simulation_list;
  if (sl==NULL) {
    Tcl_SetResult(interp, "Simulation list is NULL", TCL_STATIC);
  } else {
    len = gl_length(sl);
    for(c=1;c<=len;c++) {
      sptr = (struct Instance *)gl_fetch(sl,c);
      Tcl_AppendElement(interp,(char *)SCP(GetSimulationName(sptr)));
    }
  }
  return TCL_OK;
}

int Asc_BrowSimTypeCmd(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{
  struct Instance *sptr;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    Tcl_SetResult(interp,"wrong # args: Usage \"simtype\" simname",TCL_STATIC);
    return TCL_ERROR;
  }
  sptr = Asc_FindSimulationRoot(AddSymbol(argv[1]));
  if (sptr) {
    Tcl_AppendResult(interp,(char *)SCP(InstanceType(sptr)),(char *)NULL);
    return TCL_OK;
  }
  Tcl_SetResult(interp, "Simulation name not found", TCL_STATIC);
  return TCL_ERROR;
}

int Asc_BrowInstStatCmd(ClientData cdata, Tcl_Interp *interp,
                    int argc, CONST84 char *argv[])
{
  UNUSED_PARAMETER(cdata);
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    Tcl_SetResult(interp, "wrong # args to \"bstatistics\"", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_depth<1) {
    Tcl_SetResult(interp, "No instances to profile", TCL_STATIC);
    return TCL_OK;
  }
  if (!g_curinst) {
    Tcl_SetResult(interp, "Null current instance", TCL_STATIC);
    return TCL_ERROR;
  }
  InstanceStatistics(stdout,g_curinst);
  return TCL_OK;
}

int Asc_BrowInstListCmd(ClientData cdata, Tcl_Interp *interp,
                    int argc, CONST84 char *argv[])
{
  struct Instance *p, *c;
  struct InstanceName name;
  unsigned long cc, cindex;

  UNUSED_PARAMETER(cdata);
  UNUSED_PARAMETER(argv);

  if ( argc != 1 ) {
    Tcl_SetResult(interp, "wrong # args to \"instlist\"", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_depth<1) {
    Tcl_SetResult(interp, "No instances to list", TCL_STATIC);
    return TCL_OK;
  }
  for(cc=1;cc<g_depth;cc++) {
    p = g_instlist[cc];
    c = g_instlist[cc+1];
    cindex = ChildIndex(p,c);
    if(cindex) {
      name = ChildName(p,cindex);
      switch(InstanceNameType(name)) {
      case IntArrayIndex:
        PRINTF("[%ld]\n",InstanceIntIndex(name)); break;
      case StrArrayIndex:
        PRINTF("['%s']\n",SCP(InstanceStrIndex(name)));
        break;
      case StrName:
        PRINTF("%s\n",SCP(InstanceNameStr(name)));
        break;
      }
    }
  }
  return TCL_OK;
}

int Asc_BrowPrintCmd(ClientData cdata, Tcl_Interp *interp,
                 int argc, CONST84 char *argv[])
{
  UNUSED_PARAMETER(cdata);
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc > 2 ) {
    Tcl_SetResult(interp, "wrong #args to bprint", TCL_STATIC);
    return TCL_ERROR;
  }
  WriteInstance(stdout,g_instlist[g_depth]);
  return TCL_OK;
}


/* Start of some general instance query rotuines */

static
int BrowInstKind(struct Instance *i, char **rstring)
{
  char *tmps;

  if (!i) {
    return 1;
  }
  tmps = Asc_MakeInitString(MAXIMUM_ID_LENGTH);
  switch(InstanceKind(i)) {
  case DUMMY_INST:
    strcpy(tmps,"DUMMY_INST"); break;
  case REL_INST:
    strcpy(tmps,"REL_INST"); break;
  case WHEN_INST:
    strcpy(tmps,"WHEN_INST"); break;
  case LREL_INST:
    strcpy(tmps,"LREL_INST"); break;
  case MODEL_INST:
    strcpy(tmps,"MODEL_INST"); break;
  case REAL_INST:
    strcpy(tmps,"REAL_INST"); break;
  case REAL_ATOM_INST:
    strcpy(tmps,"REAL_ATOM_INST"); break;
  case REAL_CONSTANT_INST:
    strcpy(tmps,"REAL_CONSTANT_INST"); break;
  case BOOLEAN_INST:
    strcpy(tmps,"BOOLEAN_INST"); break;
  case BOOLEAN_ATOM_INST:
    strcpy(tmps,"BOOLEAN_ATOM_INST"); break;
  case BOOLEAN_CONSTANT_INST:
    strcpy(tmps,"BOOLEAN_CONSTANT_INST"); break;
  case INTEGER_INST:
    strcpy(tmps,"INTEGER_INST"); break;
  case INTEGER_ATOM_INST:
    strcpy(tmps,"INTEGER_ATOM_INST"); break;
  case INTEGER_CONSTANT_INST:
    strcpy(tmps,"INTEGER_CONSTANT_INST"); break;
  case SET_INST:
    strcpy(tmps,"SET_INST"); break;
  case SET_ATOM_INST:
    strcpy(tmps,"SET_ATOM_INST"); break;
  case SYMBOL_INST:
    strcpy(tmps,"SYMBOL_INST"); break;
  case SYMBOL_ATOM_INST:
    strcpy(tmps,"SYMBOL_ATOM_INST"); break;
  case SYMBOL_CONSTANT_INST:
    strcpy(tmps,"SYMBOL_CONSTANT_INST"); break;
  case ARRAY_INT_INST:
    strcpy(tmps,"ARRAY_INT_INST"); break;
  case ARRAY_ENUM_INST:
    strcpy(tmps,"ARRAY_ENUM_INST"); break;
  case ERROR_INST:
    strcpy(tmps,"ERROR_INST"); break;
  default:
    FPRINTF(ASCERR,"Unrecognized instance kind ?maybe simulation?\n");
    FFLUSH(ASCERR);
    return 1;
  }
  *rstring = tmps;
  return 0;
}

static
int BrowInstIsAssignable(struct Instance *i)
{
  if (!i) {
    return 0;                      /* Using 0 - False; 1- True */
  }
  switch(InstanceKind(i)) {
  case REAL_ATOM_INST:
  case REAL_INST:
  case BOOLEAN_ATOM_INST:
  case BOOLEAN_INST:
    return 1;
  case INTEGER_INST:
  case INTEGER_ATOM_INST:
    if (AtomMutable(i)) { /* similar treatment to be done for symbols later !*/
      return 1;
    }
  case SYMBOL_INST:
      return 1;
  case SYMBOL_ATOM_INST:
  case REAL_CONSTANT_INST:
  case INTEGER_CONSTANT_INST:
  case BOOLEAN_CONSTANT_INST:
  case SYMBOL_CONSTANT_INST:
    if (AtomAssigned(i)) { /* if already assigned then is unassignable */
      return 0;
    } else {
      return 1;
    }
  default:
    return 0;
  }
}

int Asc_BrowInstIsMutable(struct Instance *i)
{
  if (!i) {
    return 0;                      /* Using 0 - False; 1- True */
  }
  switch(InstanceKind(i)) {
  case REAL_ATOM_INST:
  case REAL_INST:
  case REAL_CONSTANT_INST:
  case BOOLEAN_ATOM_INST:
  case BOOLEAN_INST:
  case BOOLEAN_CONSTANT_INST:
  case INTEGER_INST:
  case INTEGER_ATOM_INST:
  case INTEGER_CONSTANT_INST:
  case SYMBOL_INST:
  case SYMBOL_ATOM_INST:
  case SYMBOL_CONSTANT_INST:
    if (AtomMutable(i)) {
      return 1;
    } else {
      return 0;
    }
  default:
    return 0;
  }
}

int Asc_BrowInstIsAtomic(struct Instance *i)
{
  if (!i) {
    return 0;                      /* Using 0 - False; 1- True */
  }
  switch(InstanceKind(i)) {
  case REAL_ATOM_INST: case BOOLEAN_ATOM_INST:
  case INTEGER_ATOM_INST: case SYMBOL_ATOM_INST:
  case SET_ATOM_INST:
    return 1;
  default:
    return 0;
  }
}

int Asc_BrowInstIsSubAtomic(struct Instance *i)
{
  if (!i) {
    return 0;                      /* Using 0 - False; 1- True */
  }
  switch(InstanceKind(i)) {
  case REAL_INST: case BOOLEAN_INST:
  case INTEGER_INST: case SYMBOL_INST:
  case SET_INST:
    return 1;
  default:
    return 0;
  }
}

int Asc_BrowInstIsConstant(struct Instance *i)
{
  if (!i) {
    return 0;                 /* Using 0 - False; 1- True */
  }
  switch (InstanceKind(i)) {
  case REAL_CONSTANT_INST:
  case BOOLEAN_CONSTANT_INST:
  case INTEGER_CONSTANT_INST:
  case SYMBOL_CONSTANT_INST:
    return 1;
  default:
    return 0;
  }
}

static
int BrowInstIsWhenVar(struct Instance *i)
{
  if (!i) {
    return 0;                      /* Using 0 - False; 1- True */
  }
  switch(InstanceKind(i)) {
  case BOOLEAN_ATOM_INST:
  case SYMBOL_ATOM_INST:
  case INTEGER_ATOM_INST:
    if ( WhensCount(i) ) {
      return 1;
    } else {
      return 0;
    }
  default:
    return 0;
  }
}
static
void BrowWriteUnformattedSet(Tcl_Interp *interp,struct Instance *i)
{
  CONST struct set_t *s;
  unsigned long len,c;
  char value[80];
  s = SetAtomList(i);
  switch(SetKind(s)) {
  case empty_set:
    break;
  case integer_set:
  case string_set:
    len = Cardinality(s);
    for(c=1;c<=len;c++) {
      if (SetKind(s)==integer_set) {
        sprintf(value,"%ld", FetchIntMember(s,c));
        Tcl_AppendResult(interp,value," ",(char *)NULL);
      } else {
        Tcl_AppendResult(interp,"'",SCP(FetchStrMember(s,c)),"' ",
                         (char *)NULL);
      }
    }
    return;
  default:
    return;
  }
}

static
int Asc_BrowInstAtomValue(Tcl_Interp *interp, struct Instance *i)
{
  enum inst_t kind;
  char value[256];
  char *ustr;

  switch(kind = InstanceKind(i)) {
  case DUMMY_INST:
    break;
  case REAL_INST:
  case REAL_ATOM_INST:
  case REAL_CONSTANT_INST:
    ustr = Asc_UnitValue(i);
    Tcl_AppendResult(interp,ustr,(char *)NULL);
    break;
  case INTEGER_INST:
  case INTEGER_ATOM_INST:
  case INTEGER_CONSTANT_INST:
    sprintf(value,"%ld",GetIntegerAtomValue(i));
    Tcl_AppendResult(interp,value,(char *)NULL);
    break;
  case BOOLEAN_INST:
  case BOOLEAN_ATOM_INST:
  case BOOLEAN_CONSTANT_INST:
    sprintf(value,GetBooleanAtomValue(i)?"TRUE":"FALSE");
    Tcl_AppendResult(interp,value,(char *)NULL);
    break;
  case SYMBOL_INST:
  case SYMBOL_ATOM_INST:
  case SYMBOL_CONSTANT_INST:
    Tcl_AppendResult(interp,(char *)SCP(GetSymbolAtomValue(i)),(char *)NULL);
    break;
  case SET_INST:
  case SET_ATOM_INST:
    BrowWriteUnformattedSet(interp,i);
    break;
  case REL_INST:
    ustr = Asc_UnitValue(i);
    Tcl_AppendResult(interp,ustr,(char *)NULL);
    break;
  case LREL_INST:
    if (GetInstanceLogRel(i)!=NULL) {
      Tcl_AppendResult(interp,(LogRelResidual(GetInstanceLogRel(i)) ?
                                   "TRUE" : "FALSE"), (char *)NULL);
    } else {
      Tcl_AppendResult(interp,"UNDEFINED",(char *)NULL);
    }
    break;
  default:
    Asc_Panic(2, "Asc_BrowInstAtomValue",
              "Unrecognized atom type in Asc_BrowInstAtomValue\n");
  }
  return 0;
}

/*
 * At best this is a colloquialism and refers to any instance that is
 * 1) Is a solver_var or refinement of.
 * This is unclean !!! I am only checking if the type is a boolean
 * and return 1 if it is. When we get true solver_reals this will
 * be ok.
 */
static
int BrowInstIsFixable(struct Instance *i)
{
  enum inst_t kind;
  if (!i) {
    return 0;
  }
  kind = InstanceKind(i);
  if ((kind==BOOLEAN_ATOM_INST)||(kind==BOOLEAN_INST)) {
    return 1;
  } else {
    return 0;
  }
}

static
int BrowInstName(struct Instance *i, char **rstring)
{
  /* Now only goes to stdout */

  UNUSED_PARAMETER(rstring);

  if (i) {
    WriteInstanceName(stdout,i,NULL);
    PRINTF("\n");
    return 0;
  } else {
    return 1;
  }
}

static
int BrowInstNChild(struct Instance *i, unsigned long *l)
{
  unsigned long nch;
  nch = NumberChildren(i);
  if (nch) {
    *l = nch;
    return 0;
  } else {
    return 1;
  }
}

static
int BrowInstIsAtomChild(struct Instance *i)
{
  if (!i) {
    return 0;
  }
  switch (InstanceKind(i)) {
  case REAL_INST:
  case BOOLEAN_INST:
  case INTEGER_INST:
  case SET_INST:
  case SYMBOL_INST:
  case ERROR_INST:
    return 1;
  default:
    return 0;
  }
}


/* Something seems a little screwy with this one !!! */
static
int BrowInstNParents(struct Instance *i, unsigned long *l)
{
/* Form : inst nparents arg
*/
  unsigned long nch;
  nch = NumberParents(i);
  if (nch) {
    *l = nch;
    return 0;
  } else {
    return 1;
  }
}

static
struct Instance *FirstModelUpward(struct Instance *i)
{
  while (1) {
    if (i == NULL || NumberParents(i) == 0) {
      return NULL;
    }
    i = InstanceParent(i,1);
    if (InstanceKind(i) == MODEL_INST) {
      return i;
    }
  }
}

int BrowOperands(Tcl_Interp *interp, struct Instance *i)
{
  struct gl_list_t *ol;
  struct Instance *p;
  unsigned long c,len;
  char *name;

  if (i == NULL) {
    return TCL_OK;
  }
  ol = GetInstanceOperands(i);
  if (ol == NULL) {
    return TCL_OK;
  }
  len = gl_length(ol);
  p = FirstModelUpward(i);
  for (c=1;c <= len; c++) {
    i = gl_fetch(ol,c);
    if (i == NULL) {
      continue;
    }
    name = WriteInstanceNameString(i,p);
    if (name == NULL) {
      continue;
    }
    Tcl_AppendElement(interp,name);
    ascfree(name);
  }
  gl_destroy(ol);

  return TCL_OK;
}

int Asc_BrowInstQueryCmd(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  struct Instance *i;
  struct Instance *p;
  struct InstanceName in;
  char buf[MAXIMUM_NUMERIC_LENGTH];     /* string to hold long integer */
  char *rstring = NULL;
  char *tmps = NULL;
  unsigned long n;
  unsigned long c;
  unsigned long nch;
  unsigned long npa;
  int result;

  UNUSED_PARAMETER(cdata);

  if ( argc == 1 ) {
    Tcl_AppendResult(interp,"Usage : inst <",
        "name, type, kind, old, nchild, nparents, child, parent",
        ", atomchild, isassignable, isfixable, "
        "ismutable, isconstant, iswhenvar, operands",
        ", atomvalue> [current,search]",(char *)NULL);
    return TCL_ERROR;
  }
  i = g_curinst;
  if ( argc == 3 ) {
    if(strncmp(argv[2],"current",3)==0) {
      i = g_curinst;
    } else if(strncmp(argv[2],"search",3)==0) {
      i = g_search_inst;
    } else {
      i = g_curinst;
    }
  }

  if (!i) {
    Tcl_SetResult(interp, "NULL_INSTANCE", TCL_STATIC);
    return TCL_ERROR;
  }
  if (strncmp(argv[1],"name",3)==0) {
    result = BrowInstName(i,&rstring);
    WriteInstanceName(stdout,i,NULL); PRINTF("\n");
    return TCL_OK;
  }

  if (strncmp(argv[1],"operands",3)==0) {
    return BrowOperands(interp,i);
  }
  if (strncmp(argv[1],"type",3)==0) {
    Tcl_AppendResult(interp,SCP(InstanceType(i)),(char *)NULL);
    return TCL_OK;
  }

  if (strncmp(argv[1],"kind",3)==0) {
    result = BrowInstKind(i,&rstring);
    if (result==0) {
      Tcl_ResetResult(interp);
      Tcl_AppendResult(interp,rstring,(char *)NULL);
      ascfree(rstring);
      return TCL_OK;
    } else {
      if (rstring) {
        ascfree(rstring);
      }
      Tcl_ResetResult(interp);
      return TCL_ERROR;
    }
  }

  /* always uses current instance */
  if (strncmp(argv[1],"atomchild",5)==0) {
    result = BrowInstIsAtomChild(i);
    if (result) {
      Tcl_SetResult(interp, "1", TCL_STATIC);
    } else {
      Tcl_SetResult(interp, "0", TCL_STATIC);
    }
    return TCL_OK;
  }

  if (strncmp(argv[1],"isassignable",4)==0) {
    result = BrowInstIsAssignable(i);
    if (result) {
      Tcl_SetResult(interp, "1", TCL_STATIC);
    } else {
      Tcl_SetResult(interp, "0", TCL_STATIC);
    }
    return TCL_OK;
  }

  if (strncmp(argv[1],"isfixable",3)==0) {
    result = BrowInstIsFixable(i);
    if (result) {
      Tcl_SetResult(interp, "1", TCL_STATIC);
    } else {
      Tcl_SetResult(interp, "0", TCL_STATIC);
    }
    return TCL_OK;
  }

  if (strncmp(argv[1],"ismutable",3)==0) {
    result = Asc_BrowInstIsMutable(i);
    if (result) {
      Tcl_SetResult(interp, "1", TCL_STATIC);
   } else {
      Tcl_SetResult(interp, "0", TCL_STATIC);
    }
    return TCL_OK;
  }

  if (strncmp(argv[1],"isconstant",3)==0) {
    result = Asc_BrowInstIsConstant(i);
    if (result) {
      Tcl_SetResult(interp, "1", TCL_STATIC);
    } else {
      Tcl_SetResult(interp, "0", TCL_STATIC);
    }
    return TCL_OK;
  }

  if (strncmp(argv[1],"iswhenvar",3)==0) {
    result = BrowInstIsWhenVar(i);
    if (result) {
      Tcl_SetResult(interp, "1", TCL_STATIC);
    } else {
      Tcl_SetResult(interp, "0", TCL_STATIC);
    }
    return TCL_OK;
  }

  if (strncmp(argv[1],"atomvalue",5)==0) {
    if(Asc_BrowInstIsAtomic(i)
       || BrowInstIsAtomChild(i) || Asc_BrowInstIsConstant (i)) {
      if (AtomAssigned(i)) {
        result = Asc_BrowInstAtomValue(interp,i);
      } else {
        Tcl_AppendResult(interp,"UNDEFINED",(char *)NULL);
        return TCL_OK;
      }
    } else if(InstanceKind(i)== REL_INST || InstanceKind(i)==LREL_INST ) {
      result = Asc_BrowInstAtomValue(interp,i);
    } else {
      Tcl_AppendResult(interp,
                       "Only atomic instances, constants or relations"
                       " have the notion of value",
                       (char *)NULL);
      return TCL_ERROR;
    }
    if (result==0) {
      return TCL_OK;
    } else {
      Tcl_ResetResult(interp);
      return TCL_OK;
    }
  }

  if (strncmp(argv[1],"child",3)==0) {
    nch = NumberChildren(i);
    if (nch) {
      tmps = Asc_MakeInitString(256);
      for(c=1;c<=nch;c++) {
        in = ChildName(i,c);
        switch(InstanceNameType(in)) {
        case StrName:
          Tcl_AppendElement(interp,(char *)InstanceNameStr(in));
          break;
        case IntArrayIndex:
          sprintf(tmps,"[%ld]",InstanceIntIndex(in));
          Tcl_AppendElement(interp,tmps);
          break;
        case StrArrayIndex:
          sprintf(tmps,"[\'%s\']",SCP(InstanceStrIndex(in)));
          Tcl_AppendElement(interp,tmps);
          break;
        }
      }
      ascfree(tmps);
      return TCL_OK;
    } else {
      Tcl_SetResult(interp, "0", TCL_STATIC);
      return TCL_OK;
    }
  }

  if (strncmp(argv[1],"parents",3)==0) {
    npa = NumberParents(i);
    if (npa) {
      tmps = Asc_MakeInitString(256); /* fixme size assumed */
      for(c=1;c<=npa;c++) {
        p = InstanceParent(i,c);
        in = ParentsName(p,i);
        switch(InstanceNameType(in)) {
        case StrName:
          Tcl_AppendElement(interp,(char *)SCP(InstanceNameStr(in)));
          break;
        case IntArrayIndex:
          sprintf(tmps,"[%ld]",InstanceIntIndex(in));
          Tcl_AppendElement(interp,tmps);
          break;
        case StrArrayIndex:
          sprintf(tmps,"[\'%s\']",SCP(InstanceStrIndex(in)));
          Tcl_AppendElement(interp,tmps);
          break;
        }
      }
      ascfree(tmps);
      return TCL_OK;
    }
  }

  if (strncmp(argv[1],"nchild",3)==0) {
    result = BrowInstNChild(i,&n);
    if (result==0) {
      sprintf(buf, "%lu", n);
      Tcl_SetResult(interp, buf, TCL_VOLATILE);
      return TCL_OK;
    } else {
      Tcl_SetResult(interp, "0", TCL_STATIC);
      return TCL_OK;
    }
  }

  if (strncmp(argv[1],"nparents",3)==0) {
    result = BrowInstNParents(i,&n);
    if (result==0) {
      sprintf(buf, "%lu", n);
      Tcl_SetResult(interp, buf, TCL_VOLATILE);
    }
    return TCL_OK;
  }

  Tcl_SetResult(interp, "unrecognized command to inst", TCL_STATIC);
  return TCL_ERROR;/*UNREACHED*/
}

static struct Instance *BrowInstanceMerge(struct Instance *i1,
                                          struct Instance *i2)
{
  struct Instance *result;
  if (i1 && i2) {
    result = MergeInstances(i1,i2);
    PostMergeCheck(result);
    if (result) {
      return result;
    } else {
      return NULL;
    }
  }
  return NULL;
}


/*
 * This call should be preceeded by a call to BrowQlfidSeachCmd.
 * That call will leave g_search_inst looking at the appropriate
 * inst. This function will merge g_curinst and g_search_inst.
 * If a client other than these two ptrs are looking at the instances
 * that are about to be merged, they need to be *notified* accordingly.
 * The instance that was kept from the merge will the corresponding,
 * non-NULL pointer.
 */
int Asc_BrowInstanceMergeCmd(ClientData cdata, Tcl_Interp *interp,
                         int argc, CONST84 char *argv[])
{
  struct Instance *result;
  struct Instance *i1, *i2;

  UNUSED_PARAMETER(cdata);
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    Tcl_SetResult(interp, "wrong # args: Usage \"bmerge\"", TCL_STATIC);
    return TCL_ERROR;
  }
  i1 = g_curinst;
  i2 = g_search_inst;
  switch(InstanceKind(i1)) {         /* I really should do this for i2 also */
  case REAL_INST: case BOOLEAN_INST:
  case INTEGER_INST: case SYMBOL_INST:
  case SET_INST: case REL_INST: case LREL_INST:
    Tcl_AppendResult(interp,"AscendIV does not allow\nmerging ",
                     "of \nchildren of Atoms.",(char *)NULL);
    return TCL_ERROR;
  default:
    break;
  }
  switch(InstanceKind(i2)) {         /* yes you should */
  case REAL_INST: case BOOLEAN_INST:
  case INTEGER_INST: case SYMBOL_INST:
  case SET_INST: case REL_INST: case LREL_INST:
    Tcl_AppendResult(interp,"AscendIV does not allow\n merging ",
                     "of \nchildren of Atoms.",(char *)NULL);
    return TCL_ERROR;
  default:
    break;
  }
  result = BrowInstanceMerge(i1,i2);
  if (result==NULL) {
    Tcl_AppendResult(interp,"Error in merging instances",(char *)NULL);
    return TCL_ERROR;
  }
  if (result==g_curinst) {	/* attempt to patch the pointers */
    g_search_inst = NULL;	/* rather than have someone dangling */
  } else {
    g_curinst = NULL;
  }
  return TCL_OK;
}


/*
 * This function accepts the name of a type and will refine the given
 * instance to be of that type. The orginal refine code for the interface
 * has a bug in it. The code below now calls ReInstantiate for each
 * member of the clique being refined as suggested by Tom.
 * NOTE:
 * The process of refining an instance, may cause that instance to be
 * moved in memory. Any body with their hands on that instance need
 * to be *notified*. It is always safest to search for the instance
 * using a qualified name after a refine has been done. It is possible
 * fix the global instance pointers such as g_curinst and g_search_inst,
 * and perhaps g_cursim->root, but not much else with the current setup.
 * Handles would help.
 */
int Asc_BrowInstanceRefineCmd(ClientData cdata, Tcl_Interp *interp,
                          int argc, CONST84 char *argv[])
{
  struct TypeDescription *desc, *desc1, *desc2;
  struct Instance *i, *top, *inst;
  double start_time =0.0;

  UNUSED_PARAMETER(cdata);

  if ( argc != 3 ) {
    Tcl_SetResult(interp, "wrong # args : Usage \"brefine\" type ?cur?search?",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (strncmp(argv[2],"current",3)==0) {
    i = g_curinst;
  } else if (strncmp(argv[2],"search",3)==0) {
    i = g_search_inst;
  } else {
    Tcl_SetResult(interp, "Invalid args to brefine", TCL_STATIC);
    return TCL_ERROR;
  }
  if (!i) {
    Tcl_SetResult(interp, "Cannot refine a NULL instance", TCL_STATIC);
    return TCL_ERROR;
  }
  switch(InstanceKind(i)) {
  case REAL_INST:
  case BOOLEAN_INST:
  case INTEGER_INST:
  case SYMBOL_INST:
  case SET_INST:
  case REL_INST:
  case LREL_INST:
    Tcl_AppendResult(interp,
       "AscendIV does not allow\nrefinement of\nchildren of ATOMs",
       (char *)NULL);
    return TCL_ERROR;
  default:
    break;
  }

  desc1 = InstanceTypeDesc(i);
  desc2 = FindType(AddSymbol(argv[1]));
  if (!desc2) {
    Tcl_SetResult(interp, "Type not found", TCL_STATIC);
    return  TCL_ERROR;
  }
  if (desc1==desc2) {
    return TCL_OK;
  }
  if(0 != (desc = MoreRefined(desc1,desc2))) {
    if (desc == desc1) {             /* desc1 more refined than desc2 */
      return TCL_OK;                /* hence nothing to do */
    } else {
      inst = i;                     /* just in case refine moves i*/
      top = inst = RefineClique(inst,desc,NULL);
      do {                          /* Reinstatiate the entire clique */
        if (g_compiler_timing) {
          start_time = tm_cpu_time();
        }
        ReInstantiate(inst);
        if (g_compiler_timing) {
          PRINTF("Reinstantiation CPU time = %g seconds\n",
                 tm_cpu_time() - start_time);
        }
        inst = NextCliqueMember(inst);
      } while (inst != top);
      /*
       * prepare for exit; fix up the pointer that we were called with.
       * when we start other symbolic pointers, those will have to
       * patched here as well. Ideally the entire simulation list,
       * and possibly the universal table should be fixed.
       */
      if (strncmp(argv[2],"current",3)==0) {
        g_curinst = inst;
      } else if (strncmp(argv[2],"search",3)==0) {
        g_search_inst = inst;
      }
      return TCL_OK;
    }
  } else {
    Tcl_AppendResult(interp,"Types are not conformable\n",
                     "or the Library is inconsistent",(char *)NULL);
    return TCL_ERROR;
  }
}

/*
 * This function suffers from the same ills as the refine command
 * above. We will fix the g_curinst and the g_search_inst pointers
 * but nothing else. Any one else looking at those instance had
 * better be notified otherwise.
 */
int Asc_BrowMakeAlikeCmd(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  struct TypeDescription *desc,*desc1,*desc2;
  struct Instance *i1, *i2;

  UNUSED_PARAMETER(cdata);
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 3 ) {
    Tcl_SetResult(interp, "wrong # args : Usage \"bmakealike\" current search",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  i1 = g_curinst;
  i2 = g_search_inst;
  if (i1 && i2) {
    desc1 = InstanceTypeDesc(i1);
    desc2 = InstanceTypeDesc(i2);
    if (desc1==desc2) {
      MergeCliques(i1,i2);
    } else {
      if (0 != (desc = MoreRefined(desc1,desc2))) {
        if (desc == desc1) {		/* desc1 more refined than desc2*/
         i2 = RefineClique(i2,desc,NULL);
        } else {
          i1 = RefineClique(i1,desc,NULL);
        }
        MergeCliques(i1,i2);
        g_curinst = i1;
        g_search_inst = i2;		/* patch the pointers */
      } else {
        g_curinst = i1;
        g_search_inst = i2;		/* patch the pointers */
        Tcl_SetResult(interp, "Instances are unconformable", TCL_STATIC);
        return TCL_ERROR;
      }
    }
  } else {
    Tcl_SetResult(interp, "Invalid instances in b_makealike", TCL_STATIC);
    return TCL_ERROR;
  }
  return TCL_OK;
}


static
void DumpAT(FILE *fp,struct Instance *root)
{
  int start,start1,start2;
  struct gl_list_t *atl;
  start = clock();
  atl = Asc_DeriveAnonList(root);
  start1 = clock()-start;
  start = clock();
  Asc_WriteAnonList(fp,atl,root,0);
  start2 = clock()-start;
  PRINTF("time to classify = %d\n",start1);
  PRINTF("time to write list = %d\n",start2);
  start = clock();
  Asc_DestroyAnonList(atl);
  PRINTF("time to destroy list = %lu\n",(unsigned long)(clock() - start));
}

STDHLF(Asc_BrowAnonTypesCmd,(Asc_BrowAnonTypesCmdHL,HLFSTOP));
int Asc_BrowAnonTypesCmd(ClientData cdata, Tcl_Interp *interp,
                         int argc, CONST84 char **argv)
{
  struct Instance *i;
  ASCUSE;  /* see if first arg is -help */
  if ( argc != 2 ) {
    Tcl_AppendResult(interp, "Usage: ",Asc_BrowAnonTypesCmdHN,
        " <-current,-search>", (char *)NULL);
    return TCL_ERROR;
  }
  if (strncmp(argv[1],"-current",3)==0) {
    i = g_curinst;
  } else if (strncmp(argv[1],"-search",3)==0) {
    i = g_search_inst;
  } else {
    Tcl_AppendResult(interp, "Usage: ",Asc_BrowAnonTypesCmdHN,
        " <-current,-search>", (char *)NULL);
    return TCL_ERROR;
  }
  if (!i) {
    Tcl_SetResult(interp, "0", TCL_STATIC);
    return TCL_OK;
  }
  DumpAT(stdout,i);
  return TCL_OK;
}


