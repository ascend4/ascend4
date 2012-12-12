/*
 *  BrowserRel_io.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.20 $
 *  Version control file: $RCSfile: BrowserRel_io.c,v $
 *  Date last modified: $Date: 2003/08/23 18:43:05 $
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define ASC_BUILDING_INTERFACE

#include <tcl.h>
#include "config.h"
#include <ascend/general/ascMalloc.h>
#include <ascend/general/list.h>
#include <ascend/general/dstring.h>

#include <ascend/compiler/instance_enum.h>
#include <ascend/compiler/expr_types.h>
#include <ascend/compiler/relation_io.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/instance_io.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/visitinst.h>
#include <ascend/compiler/mathinst.h>
#include <ascend/compiler/find.h>
#include <ascend/compiler/functype.h>
#include <ascend/compiler/safe.h>
#include <ascend/compiler/rel_blackbox.h>
#include <ascend/compiler/vlist.h>
#include <ascend/compiler/relation.h>
#include <ascend/compiler/relation_util.h>
#include <ascend/compiler/func.h>
#include <ascend/compiler/extcall.h>
#include <ascend/compiler/instance_name.h>
#include <ascend/compiler/qlfdid.h>
#include <ascend/system/slv_types.h>
#include "HelpProc.h"
#include "BrowserProc.h"
#include "BrowserRel_io.h"
#include "Qlfdid.h"
#include "BrowserProc.h"
#include "BrowserQuery.h"

#ifndef lint
static CONST char RelationOutputRoutinesRCS[]="$Id: BrowserRel_io.c,v 1.20 2003/08/23 18:43:05 ballan Exp $";
#endif

/* a horde of redundant code deleted. */
/**************************************************************************/
static struct gl_list_t *g_brow_rellist = NULL;
static struct gl_list_t *g_brow_condrellist = NULL;
/**************************************************************************/


/* Get a list of pointers to normal relations
 * and a list of pointers to conditional relations
 */

static
void BrowGetRelations(struct Instance *i)
{
  CONST struct relation *rel;
  if (i) {
    switch(InstanceKind(i)) {
    case REL_INST:
      rel = GetInstanceRelationOnly(i);
      if (!RelationIsCond(rel)) {
        gl_append_ptr(g_brow_rellist,i);
      } else { /* conditional relations */
        gl_append_ptr(g_brow_condrellist,i);
      }
      break;
    default:
      break;
    }
  }
}

/* This functions sends to the interpreter  the list of  relations
 * and then the list of conditional relations if required
 */
int Asc_BrowWriteRelListCmd(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  struct Instance *i, *rel_inst;
  unsigned long len, c;
  int save=0;

  UNUSED_PARAMETER(cdata);

  if (( argc < 2 ) || ( argc > 3 )) {
    Tcl_AppendResult(interp,"wrong # args : ",
                     "Usage \"bgetrels\" ?cur?search? save",(char *)NULL);
    return TCL_ERROR;
  }

  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else if (strncmp(argv[1],"search",3)==0) {
    i = g_search_inst;
  } else {
    Tcl_SetResult(interp, "invalid args to  \"bgetrels\"", TCL_STATIC);
    return TCL_ERROR;
  }

  if (argc==3) {
    if (strncmp(argv[2],"save",4)==0) {
      save = 1;
    }
  }

  if (!i) {
    return TCL_ERROR;
  }
  if (!g_brow_rellist) {
    g_brow_rellist = gl_create(40L);
  }
  if (!g_brow_condrellist) {
    g_brow_condrellist = gl_create(40L);
  }

  VisitInstanceTree(i,BrowGetRelations,0,0);

  /* relations */
  len = gl_length(g_brow_rellist);
  for (c=1;c<=len;c++) { /* the "{ }" is for making proper list elems */
    char *tmp;
    rel_inst = (struct Instance *)gl_fetch(g_brow_rellist,c);
    Tcl_AppendResult(interp,"{",(char *)NULL);
    tmp = WriteRelationString(rel_inst,NULL,NULL,NULL,relio_ascend,NULL);
    Tcl_AppendResult(interp,tmp,(char *)NULL);
    ascfree(tmp);
    Tcl_AppendResult(interp,"} ",(char *)NULL);
  }

  /* conditional relations. Only if required */
  len = gl_length(g_brow_condrellist);
  if (len) {
    Tcl_AppendResult(interp,"{The following Relations are Conditional: } ",
                     (char *)NULL);
    for (c=1;c<=len;c++) { /* the "{ }" is for making proper list elems */
      char *tmp;
      rel_inst = (struct Instance *)gl_fetch(g_brow_condrellist,c);
      Tcl_AppendResult(interp,"{",(char *)NULL);
      tmp = WriteRelationString(rel_inst,NULL,NULL,NULL,relio_ascend,NULL);
      Tcl_AppendResult(interp,tmp,(char *)NULL);
      ascfree(tmp);
      Tcl_AppendResult(interp,"} ",(char *)NULL);
    }
  }
  if (!save) {
    gl_destroy(g_brow_rellist);
    g_brow_rellist=NULL;
    gl_destroy(g_brow_condrellist);
    g_brow_condrellist=NULL;
  }
  return TCL_OK;
}


/* This function is particular for conditional relations */

int Asc_BrowWriteCondRelListCmd(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[])
{
  struct Instance *i, *rel_inst;
  unsigned long len, c;
  int save=0;

  UNUSED_PARAMETER(cdata);

  if (( argc < 2 ) || ( argc > 3 )) {
    Tcl_AppendResult(interp,"wrong # args : ",
                     "Usage \"bgetcondrels\" ?cur?search? save",(char *)NULL);
    return TCL_ERROR;
  }

  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else if (strncmp(argv[1],"search",3)==0) {
    i = g_search_inst;
  } else {
    Tcl_SetResult(interp, "invalid args to  \"bgetcondrels\"", TCL_STATIC);
    return TCL_ERROR;
  }

  if (argc==3) {
    if (strncmp(argv[2],"save",4)==0) {
      save = 1;
    }
  }
  if (!i) {
    return TCL_ERROR;
  }

  if (!g_brow_rellist) {
    g_brow_rellist = gl_create(40L);
  }
  if (!g_brow_condrellist) {
    g_brow_condrellist = gl_create(40L);
  }

  VisitInstanceTree(i,BrowGetRelations,0,0);

  len = gl_length(g_brow_condrellist);
  if (len) {
    for (c=1;c<=len;c++) {
      char *tmp;
      rel_inst = (struct Instance *)gl_fetch(g_brow_condrellist,c);
      Tcl_AppendResult(interp,"{",(char *)NULL);
      tmp = WriteRelationString(rel_inst,NULL,NULL,NULL,relio_ascend,NULL);
      Tcl_AppendResult(interp,tmp,(char *)NULL);
      ascfree(tmp);
      Tcl_AppendResult(interp,"} ",(char *)NULL);
    }
  }
  if (!save) {
    gl_destroy(g_brow_rellist);
    g_brow_rellist=NULL;
    gl_destroy(g_brow_condrellist);
    g_brow_condrellist=NULL;
  }
  return TCL_OK;
}


int Asc_BrowWriteRelListPostfixCmd(ClientData cdata, Tcl_Interp *interp,
                               int argc, CONST84 char *argv[])
{
  struct Instance *i, *rel_inst;
  enum Expr_enum type;
  unsigned long len, c;
  int save=0;

  UNUSED_PARAMETER(cdata);

  if (( argc < 2 ) || ( argc > 3 )) {
    Tcl_AppendResult(interp,"wrong # args : ",
                     "Usage \"bmake_rels\" ?cur?search? save",(char *)NULL);
    return TCL_ERROR;
  }

  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else if (strncmp(argv[1],"search",3)==0) {
    i = g_search_inst;
  } else {
    Tcl_SetResult(interp, "invalid args to  \"bmake_rels\"", TCL_STATIC);
    return TCL_ERROR;
  }

  if (argc==3) {
    if (strncmp(argv[2],"save",4)==0) {
      save = 1;
    }
  }

  if (!i) {
    return TCL_ERROR;
  }


  if (!g_brow_rellist) {
    g_brow_rellist = gl_create(40L);
  }
  if (!g_brow_condrellist) {
    g_brow_condrellist = gl_create(40L);
  }

  VisitInstanceTree(i,BrowGetRelations,0,0);

  len = gl_length(g_brow_rellist);
  for (c=1;c<=len;c++) { /* the "{ }" is for making proper list elems */
    char *tmp;
    rel_inst = (struct Instance *)gl_fetch(g_brow_rellist,c);
    type = GetInstanceRelationType(rel_inst);
    if (type!=e_token) {	/* FIX FIX FIX */
      FPRINTF(stderr,"relation type not yet supported\n");
      continue;
    }
    Tcl_AppendResult(interp,"{",(char *)NULL);
    tmp = WriteRelationPostfixString(rel_inst,NULL);
    Tcl_AppendResult(interp,tmp,(char *)NULL);
    ascfree(tmp);
    Tcl_AppendResult(interp,"} ",(char *)NULL);
  }
  if (!save) {
    gl_destroy(g_brow_rellist);
    g_brow_rellist=NULL;
    gl_destroy(g_brow_condrellist);
    g_brow_condrellist=NULL;
  }
  return TCL_OK;
}



int Asc_BrowWriteRelsForAtomCmd(ClientData cdata,Tcl_Interp *interp,
                            int argc, CONST84 char *argv[])
{
  CONST struct relation *rel;
  struct Instance *i, *rel_inst;
  unsigned long nrels, c;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    Tcl_AppendResult(interp,"wrong # args : ",
                     "Usage :__brow_relsforatom ?cur?search?",(char *)NULL);
    return TCL_ERROR;
  }
  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else if (strncmp(argv[1],"search",3)==0) {
    i = g_search_inst;
  } else {
    Tcl_SetResult(interp, "invalid args to \"__brow_relsforatom\"",TCL_STATIC);
    return TCL_ERROR;
  }
  if (!i) {
    return TCL_ERROR;
  }
  if ( (InstanceKind(i) != REAL_ATOM_INST)
       && (InstanceKind(i)!= REAL_CONSTANT_INST) ) {
    /* We may soon do booleans also */
    Tcl_AppendResult(interp,"At the moment only real atoms ",
                     "are allowed in relations",(char *)NULL);
    return TCL_ERROR;
  }
  nrels = RelationsCount(i);
  for (c=1;c<=nrels;c++) { /* the "{ }" is for making proper list elems */
    char *tmp;
    rel_inst = RelationsForAtom(i,c);
    rel = GetInstanceRelationOnly(rel_inst);
    Tcl_AppendResult(interp,"{",(char *)NULL);
    tmp = WriteRelationString(rel_inst,NULL,NULL,NULL,relio_ascend,NULL);
    Tcl_AppendResult(interp,tmp,(char *)NULL);
    ascfree(tmp);
    if (RelationIsCond(rel)) {
      Tcl_AppendResult(interp,"    Conditional Relation",(char *)NULL);
    }
    Tcl_AppendResult(interp,"} ",(char *)NULL);
  }
  return TCL_OK;
}
