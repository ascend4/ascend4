/*
 *  BrowWhen_io.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: BrowWhen_io.c,v $
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

#include <tcl.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/dstring.h>

#include <compiler/fractions.h>
#include <compiler/dimen.h>
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
#include <compiler/stattypes.h>
#include <compiler/when.h>
#include <compiler/when_util.h>
#include <compiler/when_io.h>
#include <compiler/instance_name.h>
#include <compiler/qlfdid.h>
#include <solver/slv_types.h>
#include "HelpProc.h"
#include "BrowWhen_io.h"
#include "Qlfdid.h"
#include "BrowserQuery.h"
#include "BrowserProc.h"


#ifndef lint
static CONST char BrowWhenIOID[] = "$Id: BrowWhen_io.c,v 1.9 2003/08/23 18:43:04 ballan Exp $";
#endif


/**************************************************************************/

static struct gl_list_t *g_brow_whenlist = NULL;

/**************************************************************************/


/*
 * To generate a gl_list of the when instances
 * existing in a model, an array or a WHEN itself.
 */
static
void BrowGetWhens(struct Instance *i)
{
  if (i) {
    switch(InstanceKind(i)) {
    case WHEN_INST:
      gl_append_ptr(g_brow_whenlist,i);
      break;
    default:
      break;
    }
  }
}


/*
 * Write a list of when statements to the Tcl interpreter
 */
int Asc_BrowWriteWhenListCmd(ClientData cdata, Tcl_Interp *interp,
                          int argc, CONST84 char *argv[])
{
  struct Instance *i, *when_inst;
  unsigned long len, c;
  int save=0;
  unsigned long nwhens;

  UNUSED_PARAMETER(cdata);

  if (( argc < 2 )||( argc > 3 )) {
    Tcl_AppendResult(interp,"wrong # args : ",
                     "Usage \"bgetwhens\" ?cur?search? save",(char *)NULL);
    return TCL_ERROR;
  }

  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else {
    if (strncmp(argv[1],"search",3)==0) {
      i = g_search_inst;
    } else {
      Tcl_SetResult(interp, "invalid args to \"bgetwhens\"", TCL_STATIC);
      return TCL_ERROR;
    }
  }

  if ( argc == 3 ) {
    if (strncmp(argv[2],"save",4)==0) {
      save = 1;
    }
  }

  if (!i) {
    return TCL_ERROR;
  }

  if (!g_brow_whenlist) {
    g_brow_whenlist = gl_create(40L);
  }

  VisitInstanceTree(i,BrowGetWhens,0,0);
  len = gl_length(g_brow_whenlist);

  /* Writing the WHENs in the model, array or WHEN itself */
  if (len) {
    Tcl_AppendResult(interp,
                     "{WHENs in this Instance: } ",
                     (char *)NULL);
  }
  for (c=1;c<=len;c++) {
    char *tmp;
    when_inst = (struct Instance *)gl_fetch(g_brow_whenlist,c);
    Tcl_AppendResult(interp,"{",(char *)NULL);
    tmp = WriteWhenString(when_inst,NULL);
    Tcl_AppendResult(interp,tmp,(char *)NULL);
    ascfree(tmp);
    Tcl_AppendResult(interp,"} ",(char *)NULL);
  }

  /* Writing the WHEN which include such MODEL or WHEN(nesting) */

  switch (InstanceKind(i)) {
    case MODEL_INST:
    case WHEN_INST:
      nwhens = WhensCount(i);
      if (nwhens) {
        Tcl_AppendResult(interp,
                         "{This Instance is used in CASEs OF: } ",
                         (char *)NULL);
        for (c=1;c<=nwhens;c++) {
          char *tmp;
          when_inst = WhensForInstance(i,c);
          Tcl_AppendResult(interp,"{",(char *)NULL);
          tmp = WriteWhenString(when_inst,NULL);
          Tcl_AppendResult(interp,tmp,(char *)NULL);
          ascfree(tmp);
          Tcl_AppendResult(interp,"} ",(char *)NULL);
        }
      }
      break;
    case ARRAY_INT_INST:
    case ARRAY_ENUM_INST:
      break;
    default:
      Tcl_AppendResult(interp,"Inappropriate instance called",
                              "in BrowWriteWhenList",(char *)NULL);
      return TCL_ERROR;
  }

  if (!save) {
    gl_destroy(g_brow_whenlist);
    g_brow_whenlist=NULL;
  }

  return TCL_OK;
}


int Asc_BrowWriteWhensForInstanceCmd(ClientData cdata, Tcl_Interp *interp,
                                 int argc, CONST84 char *argv[])
{
  struct Instance *i, *when_inst;
  unsigned long nwhens, c;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    Tcl_AppendResult(interp,"wrong # args : ",
             "Usage :__brow_whensforinstance ?cur?search?",(char *)NULL);
    return TCL_ERROR;
  }

  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else {
    if (strncmp(argv[1],"search",3)==0) {
      i = g_search_inst;
    } else {
      Tcl_SetResult(interp, "invalid args to \"__brow_whensforinstance\"",
                    TCL_STATIC);
      return TCL_ERROR;
    }
  }


  if (!i) {
    return TCL_ERROR;
  }

  switch (InstanceKind(i)) {
    case BOOLEAN_ATOM_INST:
    case BOOLEAN_CONSTANT_INST:
    case INTEGER_ATOM_INST:
    case INTEGER_CONSTANT_INST:
    case SYMBOL_ATOM_INST:
    case SYMBOL_CONSTANT_INST:
    case REL_INST:
      break;
    default:
      Tcl_AppendResult(interp,"Inappropriate instance called",
                              "in BrowWriteWhensForInstance",(char *)NULL);
      return TCL_ERROR;
  }

  nwhens = WhensCount(i);
  if (nwhens) {
    Tcl_AppendResult(interp,"{WHENs including this instance are: } ",
                           (char *)NULL);

    for (c=1;c<=nwhens;c++) {
      char *tmp;
      when_inst = WhensForInstance(i,c);
      Tcl_AppendResult(interp,"{",(char *)NULL);
      tmp = WriteWhenString(when_inst,NULL);
      Tcl_AppendResult(interp,tmp,(char *)NULL);
      ascfree(tmp);
      Tcl_AppendResult(interp,"} ",(char *)NULL);
    }
  }

  return TCL_OK;
}
