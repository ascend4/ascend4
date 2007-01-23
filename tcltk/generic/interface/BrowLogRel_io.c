/*
 *  BrowLogRel_io.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.11 $
 *  Version control file: $RCSfile: BrowLogRel_io.c,v $
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


/*
 *                 Browser Logical Relation Output Routines
 */

#define ASC_BUILDING_INTERFACE

#include <tcl.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/dstring.h>



#include <compiler/expr_types.h>
#include <compiler/exprs.h>
#include <compiler/instance_enum.h>
#include <compiler/symtab.h>
#include <compiler/instance_io.h>
#include <compiler/instquery.h>
#include <compiler/visitinst.h>
#include <compiler/mathinst.h>
#include <compiler/extfunc.h>
#include <compiler/find.h>
#include <compiler/functype.h>
#include <compiler/safe.h>
#include <compiler/func.h>
#include <compiler/extcall.h>
#include <compiler/logical_relation.h>
#include <compiler/logrelation.h>
#include <compiler/logrel_util.h>
#include <compiler/logrel_io.h>
#include <compiler/instance_name.h>
#include <compiler/qlfdid.h>
#include <solver/slv_types.h>
#include "HelpProc.h"
#include "BrowserProc.h"
#include "BrowLogRel_io.h"
#include "Qlfdid.h"
#include "BrowserQuery.h"


#ifndef lint
static CONST char BrowLogRelIOID[] = "$Id: BrowLogRel_io.c,v 1.11 2003/08/23 18:43:04 ballan Exp $";
#endif


/**************************************************************************/

static struct gl_list_t *g_brow_lrellist = NULL;
static struct gl_list_t *g_brow_condlrellist = NULL;

/**************************************************************************/

/* Get a list of pointers to logical relations and
 * a list of pointer to conditional logical relations
 */

static
void BrowGetLogRelations(struct Instance *i)
{
  CONST struct logrelation *lrel;

  if (i) {
    switch(InstanceKind(i)) {
    case LREL_INST:
      lrel = GetInstanceLogRelOnly(i);
      if (!LogRelIsCond(lrel)) {
        gl_append_ptr(g_brow_lrellist,i);
      } else { /* conditional */
        gl_append_ptr(g_brow_condlrellist,i);
      }
      break;
    default:
      break;
    }
  }
}


/* This functions sends to the interpreter  the list of logical relations
 * and then the list of conditional ligcal relations if required
 */

int Asc_BrowWriteLogRelListCmd(ClientData cdata,Tcl_Interp *interp,
                            int argc, CONST84 char *argv[])
{
  struct Instance *i, *lrel_inst;
  unsigned long len, c;
  int save=0;

  UNUSED_PARAMETER(cdata);

  if (( argc < 2 ) || ( argc > 3 )) {
    Tcl_AppendResult(interp,"wrong # args : ",
                     "Usage \"bgetlogrels\" ?cur?search? save",(char *)NULL);
    return TCL_ERROR;
  }
  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else {
    if (strncmp(argv[1],"search",3)==0) {
      i = g_search_inst;
    } else {
      Tcl_SetResult(interp, "invalid args to  \"bgetlogrels\"", TCL_STATIC);
      return TCL_ERROR;
    }
  }

  if (argc==3) {
    if (strncmp(argv[2],"save",4)==0) {
      save = 1;
    }
  }

  if (!i) {
    return TCL_ERROR;
  }

  if (!g_brow_lrellist) {
    g_brow_lrellist = gl_create(40L);
  }
  if (!g_brow_condlrellist) {
    g_brow_condlrellist = gl_create(40L);
  }

    VisitInstanceTree(i,BrowGetLogRelations,0,0);

    /* Logical Relation */
    len = gl_length(g_brow_lrellist);
    for (c=1;c<=len;c++) {
      char *tmp;
      lrel_inst = (struct Instance *)gl_fetch(g_brow_lrellist,c);
      Tcl_AppendResult(interp,"{",(char *)NULL);
      tmp = WriteLogRelToString(lrel_inst,NULL);
      Tcl_AppendResult(interp,tmp,(char *)NULL);
      ascfree(tmp);
      Tcl_AppendResult(interp,"} ",(char *)NULL);
    }

    /* Conditional Logical Relation if the case */
    len = gl_length(g_brow_condlrellist);
    if (len) {
      Tcl_AppendResult(interp,"{The following LogRels are Conditional: } ",
                     (char *)NULL);
      for (c=1;c<=len;c++) {
        char *tmp;
        lrel_inst = (struct Instance *)gl_fetch(g_brow_condlrellist,c);
        Tcl_AppendResult(interp,"{",(char *)NULL);
        tmp = WriteLogRelToString(lrel_inst,NULL);
        Tcl_AppendResult(interp,tmp,(char *)NULL);
        ascfree(tmp);
        Tcl_AppendResult(interp,"} ",(char *)NULL);
      }
    }
  if (!save) {
    gl_destroy(g_brow_lrellist);
    g_brow_lrellist=NULL;
    gl_destroy(g_brow_condlrellist);
    g_brow_condlrellist=NULL;
  }
  return TCL_OK;
}


/* This function is particular for conditional logical relations */

int Asc_BrowWriteCondLogRelListCmd(ClientData cdata,Tcl_Interp *interp,
                               int argc, CONST84 char *argv[])
{
  struct Instance *i, *lrel_inst;
  unsigned long len, c;
  int save=0;

  UNUSED_PARAMETER(cdata);

  if (( argc < 2 ) || ( argc > 3 )) {
    Tcl_AppendResult(interp,"wrong # args : ",
                  "Usage \"bgetcondlogrels\" ?cur?search? save",(char *)NULL);
    return TCL_ERROR;
  }

  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else {
    if (strncmp(argv[1],"search",3)==0) {
      i = g_search_inst;
    } else {
      Tcl_SetResult(interp, "invalid args to \"bgetcondlogrels\"", TCL_STATIC);
      return TCL_ERROR;
    }
  }

  if (argc==3) {
    if (strncmp(argv[2],"save",4)==0) {
      save = 1;
    }
  }
  if (!i) {
    return TCL_ERROR;
  }

  if (!g_brow_lrellist) {
    g_brow_lrellist = gl_create(40L);
  }
  if (!g_brow_condlrellist) {
    g_brow_condlrellist = gl_create(40L);
  }

  VisitInstanceTree(i,BrowGetLogRelations,0,0);

  len = gl_length(g_brow_condlrellist);
  if (len) {
    for (c=1;c<=len;c++) {
      char *tmp;
      lrel_inst = (struct Instance *)gl_fetch(g_brow_condlrellist,c);
      Tcl_AppendResult(interp,"{",(char *)NULL);
      tmp = WriteLogRelToString(lrel_inst,NULL);
      Tcl_AppendResult(interp,tmp,(char *)NULL);
      ascfree(tmp);
      Tcl_AppendResult(interp,"} ",(char *)NULL);
    }
  }
  if (!save) {
    gl_destroy(g_brow_lrellist);
    g_brow_lrellist=NULL;
    gl_destroy(g_brow_condlrellist);
    g_brow_condlrellist=NULL;
  }
  return TCL_OK;
}




/* For postfix representation */

int Asc_BrowWriteLogRelListPostfixCmd(ClientData cdata,Tcl_Interp *interp,
                                     int argc, CONST84 char *argv[])
{
  struct Instance *i, *lrel_inst;
  unsigned long len, c;
  int save=0;

  UNUSED_PARAMETER(cdata);

  if (( argc < 2 ) || ( argc > 3 )) {
    Tcl_AppendResult(interp,"wrong # args : ",
                     "Usage \"bgetlogrelspf\" ?cur?search? save",(char *)NULL);
    return TCL_ERROR;
  }

  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else {
      if (strncmp(argv[1],"search",3)==0) {
        i = g_search_inst;
      } else {
        Tcl_SetResult(interp, "invalid args to \"bgetlogrelspf\"", TCL_STATIC);
        return TCL_ERROR;
      }
  }

  if (argc==3) {
    if (strncmp(argv[2],"save",4)==0) {
      save = 1;
    }
  }

  if (!i) {
    return TCL_ERROR;
  }


  if (!g_brow_lrellist) {
    g_brow_lrellist = gl_create(40L);
  }
  if (!g_brow_condlrellist) {
    g_brow_condlrellist = gl_create(40L);
  }

  VisitInstanceTree(i,BrowGetLogRelations,0,0);

  len = gl_length(g_brow_lrellist);
  for (c=1;c<=len;c++) {
    char *tmp;
    lrel_inst = (struct Instance *)gl_fetch(g_brow_lrellist,c);
    Tcl_AppendResult(interp,"{",(char *)NULL);
    tmp = WriteLogRelPostfixToString(lrel_inst,NULL);
    Tcl_AppendResult(interp,tmp,(char *)NULL);
    ascfree(tmp);
    Tcl_AppendResult(interp,"} ",(char *)NULL);
  }
  if (!save) {
    gl_destroy(g_brow_lrellist);
    g_brow_lrellist=NULL;
    gl_destroy(g_brow_condlrellist);
    g_brow_condlrellist=NULL;
  }
  return TCL_OK;
}


/* Get a list of logical relations involving the current
 * boolean instance
 */

int Asc_BrowWriteLogRelsForAtomCmd(ClientData cdata,Tcl_Interp *interp,
                               int argc, CONST84 char *argv[])
{
  CONST struct logrelation *lrel;
  struct Instance *i, *lrel_inst;
  unsigned long nlrels, c;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    Tcl_AppendResult(interp,"wrong # args : ",
                     "Usage :__brow_lrelsforatom ?cur?search?",(char *)NULL);
    return TCL_ERROR;
  }

  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else {
      if (strncmp(argv[1],"search",3)==0) {
        i = g_search_inst;
      } else {
       Tcl_SetResult(interp, "invalid args to \"__brow_lrelsforatom\"",
                     TCL_STATIC);
       return TCL_ERROR;
     }
  }

  if (!i) {
    return TCL_ERROR;
  }

  if ( (InstanceKind(i)!= BOOLEAN_ATOM_INST ) &&
       (InstanceKind(i)!= BOOLEAN_CONSTANT_INST ) ) {
    Tcl_AppendResult(interp,"Only boolean atoms are allowed",
                     "in logical relations",(char *)NULL);
    return TCL_ERROR;
  }

  nlrels = LogRelationsCount(i);
  for (c=1;c<=nlrels;c++) {
    char *tmp;
    lrel_inst = LogRelationsForInstance(i,c);
    lrel = GetInstanceLogRelOnly(lrel_inst);
    Tcl_AppendResult(interp,"{",(char *)NULL);
    tmp = WriteLogRelToString(lrel_inst,NULL);
    Tcl_AppendResult(interp,tmp,(char *)NULL);
    ascfree(tmp);
    if (LogRelIsCond(lrel)) {
      Tcl_AppendResult(interp,"    Conditional Logical Relation",(char *)NULL);
    }
    Tcl_AppendResult(interp,"} ",(char *)NULL);
  }
  return TCL_OK;
}


