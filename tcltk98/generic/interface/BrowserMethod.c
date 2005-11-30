/*
 *  BrowserMethod.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.24 $
 *  Version control file: $RCSfile: BrowserMethod.c,v $
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


#include "tcl.h"
#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "general/list.h"
#include "compiler/compiler.h"
#include "compiler/instance_enum.h"
#include "compiler/symtab.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/types.h"
#include "compiler/stattypes.h"
#include "compiler/slist.h"
#include "compiler/statio.h"
#include "compiler/proc.h"
#include "compiler/watchpt.h"
#include "compiler/watchptio.h"
#include "compiler/name.h"
#include "compiler/instquery.h"
#include "compiler/atomvalue.h"
#include "compiler/initialize.h"
#include "compiler/child.h"
#include "compiler/parentchild.h"
#include "compiler/type_desc.h"
#include "compiler/units.h"
#include "compiler/qlfdid.h"
#include "solver/slv_types.h"
#include "interface/HelpProc.h"
#include "interface/BrowserProc.h"
#include "interface/BrowserMethod.h"
#include "interface/UnitsProc.h"
#include "interface/Qlfdid.h"

#ifndef lint
static CONST char BrowserMethodID[] = "$Id: BrowserMethod.c,v 1.24 2003/08/23 18:43:04 ballan Exp $";
#endif


#define MAXID 256

#ifndef MAXIMUM_STRING_LENGTH
#define MAXIMUM_STRING_LENGTH 2048
#endif
#define BRSTRINGMALLOC \
(char *)ascmalloc(MAXIMUM_STRING_LENGTH * sizeof(char))


/*
 */
STDHLF(Asc_BrowInitializeCmd,(Asc_BrowInitializeCmdHL,HLFSTOP));
int Asc_BrowInitializeCmd(ClientData cdata, Tcl_Interp *interp,
                         int argc, CONST84 char *argv[])
{
  int status;
  struct Name *name=NULL;
  enum Proc_enum runstat;
  int options=0;
  CONST84 char *qlfdid=NULL;
  struct Instance *i = NULL;
  CONST84 char *stoponerr = NULL;
  CONST84 char *btuifstop = NULL;
  FILE *output=NULL;
  CONST84 char *method=NULL;
  CONST84 char *filename=NULL;
  int k,tmp=0;

  (void)cdata;    /* stop gcc whine about unused parameter */

  ASCUSE;

  if (argc < 2) {
    /* put help message here */
    Tcl_SetResult(interp, "wrong # args: Usage: " Asc_BrowInitializeCmdHU,
                  TCL_STATIC);
    return TCL_ERROR;
  }
  output = ASCERR;
  /* this is safe because argv[argc]==NULL by convention */
  for (k=1; k < argc;) {
    if (strcmp(argv[k],"-stopOnErr")==0) {
      stoponerr = argv[k+1];
      k += 2;
      continue;
    }
    if (strcmp(argv[k],"-backtrace")==0) {
      btuifstop = argv[k+1];
      k += 2;
      continue;
    }
    if (strcmp(argv[k],"-method")==0) {
      method = argv[k+1];
      k += 2;
      continue;
    }
    if (strcmp(argv[k],"-qlfdid")==0) {
      qlfdid = argv[k+1];
      k += 2;
      continue;
    }
    if (strcmp(argv[k],"-output")==0) {
      filename = argv[k+1];
      k += 2;
      continue;
    }
    Tcl_AppendResult(interp,"Unknown option '",argv[k],"' to ",
                     Asc_BrowInitializeCmdHN,(char *)NULL);
    return TCL_ERROR;
  }
  status = Asc_QlfdidSearch3(qlfdid,0); /* does check for NULL, yes? */
  if (status==0) { 		/* catch inst ptr */
    i = g_search_inst;
  } else { 				/* failed. bail out. */
    Tcl_AppendResult(interp,Asc_BrowInitializeCmdHN,
                     "Could not find instance ",qlfdid,(char *)NULL);
    return TCL_ERROR;
  }
  if (btuifstop != NULL) {
    status = Tcl_GetInt(interp,btuifstop,&tmp);
    if (status != TCL_OK || tmp < 0 || tmp > 1) {
      Tcl_AppendResult(interp,"Non-boolean value (",btuifstop,") given for ",
                       argv[0]," ","-backtrace",(char *)NULL);
      return TCL_ERROR;
    }
    if (tmp) {
      options |= WP_BTUIFSTOP;
    }
  }
  if (stoponerr != NULL) {
    status = Tcl_GetInt(interp,stoponerr,&tmp);
    if (status != TCL_OK || tmp < 0 || tmp > 1) {
      Tcl_AppendResult(interp,"Non-boolean value (",stoponerr,") given for ",
                       argv[0]," ","-stopOnErr",(char *)NULL);
      return TCL_ERROR;
    }
    if (tmp) {
      options |= WP_STOPONERR;
    }
  }
  if (method != NULL) {
    name = CreateIdName(AddSymbol(method));
  } else {
    Tcl_AppendResult(interp,Asc_BrowInitializeCmdHN,
                     "-method <method name> not given",(char *)NULL);
    return TCL_ERROR;
  } 
  if (filename != NULL) {
    output = fopen(filename,"w+");
    if (output == NULL) {
      Tcl_AppendResult(interp,Asc_BrowInitializeCmdHN,
                       "-output ",filename,
                       " cannot open/write",(char *)NULL);
      DestroyName(name);
      return TCL_ERROR;
    }
  }
  runstat = Initialize(i,name,QUIET(qlfdid),output,options,NULL,NULL);
  if (filename != NULL && output != ASCERR) {
    fclose(output);
  }
  DestroyName(name);
  if (runstat != Proc_all_ok) {
    Tcl_AppendResult(interp, "Error executing method ",method,
                     " in ",qlfdid,(char *)NULL);
    return TCL_ERROR;
  } 
  return TCL_OK;
}

static void lowerstring(register char *str)
{
  while (*str != '\0') {
    if ((*str >= 'A')&&(*str <= 'Z')) {
      *str = *str + ('a' - 'A');
    }
    str++;
  }
}


static
int BrowDoAssignment(Tcl_Interp *interp,struct Instance *i,
                     char *value_str, char *unit_str)
{
  char buffer[MAXID], *tmps;
  symchar *sym;
  int code = 0;
  switch(InstanceKind(i)) {
  case REAL_ATOM_INST:
  case REAL_INST:
  case REAL_CONSTANT_INST:
    code = Asc_UnitSetRealAtomValue(i,value_str,unit_str,0);
    switch (code) {
    case 0:
      break;
    case 1:
      Tcl_SetResult(interp, "Unparseable units given - Not assigned.",
                    TCL_STATIC);
      return TCL_ERROR;
    case 2:
      Tcl_SetResult(interp, "Dimensionally incompatible units - Not assigned.",
                    TCL_STATIC);
      return TCL_ERROR;
    case 3:
      Tcl_SetResult(interp,"Overflow in converting to SI value--Not assigned.",
                    TCL_STATIC);
      return TCL_ERROR;
    case 5:
      Tcl_SetResult(interp, "Unparseable value given - Not assigned.",
                    TCL_STATIC);
      return TCL_ERROR;
    default:
      return TCL_ERROR;
    }
    break;
  case BOOLEAN_ATOM_INST:
  case BOOLEAN_INST:
  case BOOLEAN_CONSTANT_INST:
    tmps = strcpy(buffer,value_str);
    lowerstring(tmps);
    if(strcmp(tmps,"true")==0 || strcmp(tmps,"1")==0 || strcmp(tmps,"yes")==0){
      SetBooleanAtomValue(i,1,0);
    } else if (strcmp(tmps,"false")==0
               || strcmp(tmps,"0")==0
               || strcmp(tmps,"no")==0) {
      SetBooleanAtomValue(i,0,0);
    } else {
      Tcl_SetResult(interp, "Incorrect boolean value", TCL_STATIC);
      return TCL_ERROR;
    }
    break;
  case INTEGER_ATOM_INST:
  case INTEGER_INST:
  case INTEGER_CONSTANT_INST:
    if (AtomMutable(i) || !AtomAssigned(i)) {
      SetIntegerAtomValue(i,atol(value_str),0);
    } else {
      Tcl_SetResult(interp, "Attempting to assign to an immutable integer",
                    TCL_STATIC);
      return TCL_ERROR;
    }
    break;
  case SYMBOL_ATOM_INST:
  case SYMBOL_INST:
    /* the symtab in symtab.c owns the string.
     * an instance only refers to it.
     */
    sym = AddSymbol(value_str); /* this will copy the string */
    SetSymbolAtomValue(i,sym);
    break;
  case SYMBOL_CONSTANT_INST:
    /* the symtab in symtab.c owns the string.
     * an instance only refers to it.
     */
    if (!AtomAssigned(i)) {
      sym = AddSymbol(value_str); /* this will copy the string */
      SetSymbolAtomValue(i,sym);
    }
    break;
  case SET_ATOM_INST:
  case SET_INST:
    /* not yet supported */
    break;
  default:
    Tcl_SetResult(interp, "The argument to assign is not a atom", TCL_STATIC);
    return TCL_ERROR;
  }
  return TCL_OK;
}

/*
 * This function should probably go away. !!.
 * We will at the next iteration.
 */
int Asc_BrowRunAssignQlfdidCmd2(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[])
{
  struct Instance *i;
  CONST84 char *value_str = NULL;
  CONST84 char *unit_str = NULL;
  int nok;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if (( argc < 3 ) || ( argc > 4 )) {
    Tcl_AppendResult(interp,"wrong # args: ",
                     "Usage: \"qassgn2\" qlfdid value [units]",(char *)NULL);
    return TCL_ERROR;
  }
  nok = Asc_QlfdidSearch2(QUIET(argv[1]));
  if (nok) { /* failed. bail out. */
    Tcl_AppendResult(interp," : Error -- Name not found",(char *)NULL);
    return TCL_ERROR;
  }
  i = g_search_inst;	/* catch inst ptr found in QlfdidSearch */
  value_str = argv[2];
  if ( argc == 4 ) {
    unit_str = argv[3];
  }
  if (strcmp("UNDEFINED",value_str)==0) {
    return TCL_OK;
  }
  nok = BrowDoAssignment(interp,i,QUIET(value_str),QUIET(unit_str));
  return nok;	/* whatever code returned by BrowDoAssignment */
}


int Asc_BrowRunAssignQlfdidCmd3(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[])
{
  struct Instance *i;
  char *value_str = NULL;
  char *unit_str = NULL;
  int nok;
  int relative = 0;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if (( argc < 3 ) || ( argc > 5 )) {
    Tcl_AppendResult(interp,"wrong # args: ",
		     "Usage: \"qassgn3\" qlfdid value [units] [-relative]",
		     (char *)NULL);
    return TCL_ERROR;
  }
/* reading args out of order to get relative flag sorted out */
  if ( argc == 4 ) {
    if (strcmp("-relative",argv[3])==0) {
      relative = 1;
    } else {
      unit_str = QUIET(argv[3]);
    }
  }
  if ( argc == 5 ) {
    relative = 1;
  }

  nok = Asc_QlfdidSearch3(QUIET(argv[1]),relative);
  if (nok) { /* failed. bail out. */
    Tcl_AppendResult(interp," : Error -- Name not found",(char *)NULL);
    return TCL_ERROR;
  }
  i = g_search_inst;	/* catch inst ptr found in QlfdidSearch */
  value_str = QUIET(argv[2]);

  if (strcmp("UNDEFINED",value_str)==0) {
    return TCL_OK;
  }
  nok = BrowDoAssignment(interp,i,value_str,unit_str);
  return nok;	/* whatever code returned by BrowDoAssignment */
}


int Asc_BrowRunAssignmentCmd(ClientData cdata, Tcl_Interp *interp,
                         int argc, CONST84 char *argv[])
{
  struct Instance *i;
  char *unit_str = NULL;
  char *value_str = NULL;
  int argstart=1;
  int nok;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if (argc<2 || argc>4) {
    Tcl_AppendResult(interp, "Usage: \"", argv[0],
                     "\" [-search] value [units]",(char *)NULL);
    return TCL_ERROR;
  }
  if (argv[1][0] == '-') {
    if (strncmp("-search",argv[1],3)!=0) {
      Tcl_AppendResult(interp,"Error: ",argv[0]," Unknown option ",argv[1],
                       " want \"-search\"", (char *)NULL);
      return TCL_ERROR;
    } else {
      argstart++;
      i = g_search_inst;
    }
  } else {
    i = g_curinst;	/* use the current instance as the context */
  }
  if (!i) {
    Tcl_SetResult(interp, "Given instance is NULL", TCL_STATIC);
    return TCL_ERROR;
  }
  value_str = QUIET(argv[argstart]);
  if ( argc == 3 && argstart == 1) {
    unit_str = QUIET(argv[2]);
  }
  if ( argc == 4) {
    unit_str = QUIET(argv[3]);
  }
  if (strcmp("UNDEFINED",value_str)==0) {
    return TCL_OK;
  }
  nok = BrowDoAssignment(interp,i,value_str,unit_str);
  return nok;	/* whatever code returned by BrowDoAssignment */
}


int Asc_BrowWriteProcedure(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  struct InitProcedure *proc;
  struct Instance *i;
  FILE *fp=NULL;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc < 3 || argc >4) {
    Tcl_SetResult(interp,"Usage bgetproc <methodname> <filepathname> [search]",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (argc==4) {
    i = g_search_inst;
  } else {
    i = g_curinst;
  }
  if (i==NULL) {
    Tcl_SetResult(interp, "no instance sent to bgetproc", TCL_STATIC);
    return TCL_ERROR;
  }
  proc = FindProcedure(i,AddSymbol(argv[1]));
  if (proc==NULL) {
    Tcl_SetResult(interp, "method named not found", TCL_STATIC);
    return TCL_ERROR;
  }
  fp=fopen(argv[2],"w");
  if (fp==NULL) {
     Tcl_SetResult(interp, "unable to open scratch file.", TCL_STATIC);
     return TCL_ERROR;
  }
  WriteProcedure(fp,proc);
  fclose(fp);

  return TCL_OK;
}


int Asc_BrowSetAtomAttribute(Tcl_Interp *interp, struct Instance *i,
                             symchar *attr, enum inst_t kind, void *value)
{
  struct Instance *ch;
  if (interp==NULL) {
    return TCL_ERROR;
  }
  if (i==NULL || attr == NULL || value == NULL) {
    Tcl_SetResult(interp, "Bad input to C Asc_BrowSetAtomAttribute",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  assert(AscFindSymbol(attr) != NULL);
  ch = ChildByChar(i,attr); /* symchar safe. no array child of atoms */
  if (ch == NULL || InstanceKind(ch) != kind) {
    Tcl_SetResult(interp, "Mismatched input to C Asc_BrowSetAtomAttribute",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  switch (InstanceKind(ch)) {
  case REAL_INST:
    SetRealAtomValue(ch,*(double *)value,0);
    break;
  case INTEGER_INST:
    SetIntegerAtomValue(ch,*(long *)value,0);
    break;
  case BOOLEAN_INST:
    SetBooleanAtomValue(ch,(*(int *)value != 0),0);
    break;
  case SYMBOL_INST:
    SetSymbolAtomValue(ch,AddSymbol(*(char **)value));
    break;
  default:
    Tcl_SetResult(interp, "Incorrect child type to C Asc_BrowSetAtomAttribute",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  return TCL_OK;
}
/*
 * status = Asc_BrowSetAtomAttribute(interp,atominstance,
 *                                   childname,childtype,dataptr);
 * Sets the value of an attribute of the ATOM/REL instance given.
 * Childname must be from the compiler symbol table via AddSymbol or
 * AddSymbolL. Childtype determines what dataptr contains.
 * Childtype must be REAL_INST, INTEGER_INST, BOOLEAN_INST, SYMBOL_INST.
 * SET_INST is not supported at this time. dataptr must point to
 * an appropriate value object for each of the INST types above:
 * double, long, int, symchar *, respectively. Note that a symbol
 * value MUST come from the symbol table.
 * Return a value and message other than TCL_OK if these conditions
 * are not met. Except that if the childname or symbol value given
 * are not in the symbol table, then does not return.
*/




