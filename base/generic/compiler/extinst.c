/*
 *  Ascend Instance External Vars Functions
 *  by Kirk Abbott
 *  1995
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: extinst.c,v $
 *  Date last modified: $Date: 1997/07/18 12:29:34 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1995 Kirk Andre' Abbott
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 *
 */
#include <stdarg.h>
#include "utilities/ascConfig.h"
#include "compiler/compiler.h"
#include "utilities/ascPanic.h"
#include "utilities/ascMalloc.h"
#include "general/list.h"
#include "compiler/instance_enum.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/functype.h"
#include "compiler/types.h"
#include "compiler/instquery.h"
#include "compiler/instance_types.h"
#include "compiler/instmacro.h"
#include "compiler/cmpfunc.h"
#include "compiler/extinst.h"

#ifndef lint
static CONST char ExtInstModuleID[] = "$Id: extinst.c,v 1.7 1997/07/18 12:29:34 mthomas Exp $";
#endif

/*********************************************************************\
 External Relations Table
\*********************************************************************/

struct Instance **g_ExtVariablesTable = NULL;

static
void InitExtVariablesTable(void)
{
  int c;
  if (g_ExtVariablesTable==NULL) {
    g_ExtVariablesTable = (struct Instance **)
      ascmalloc(MAX_EXTRELATIONS*sizeof(struct Instance *));
    for (c=0;c<MAX_EXTRELATIONS;c++) {
      g_ExtVariablesTable[c] = NULL;
    }
  }
}

static
struct gl_list_t *LookupVarInTable(struct Instance *inst)
{
  int c;
  struct Instance **hndl = g_ExtVariablesTable;
  struct gl_list_t *hndl_list = NULL;
  if (!inst)
    return NULL;
  if (g_ExtVariablesTable) {
    hndl_list = gl_create(7L);
    for(c=0;c<MAX_EXTRELATIONS;c++) {
      if (hndl[c] == NULL) {
	break;
      }
      if (hndl[c] == inst) {
	gl_append_ptr(hndl_list,(VOIDPTR)&hndl[c]);
      }
    }
    if (gl_length(hndl_list))
      return hndl_list;
    else{
      gl_destroy(hndl_list);
      return NULL;
    }
  }
  return NULL;
}

struct Instance **AddVarToTable(struct Instance *inst, int *added)
{
  struct Instance **hndl;
  int c=0;

  assert(inst!=NULL);
  *added = 0;
  if (g_ExtVariablesTable==NULL) {
    InitExtVariablesTable();
    hndl = g_ExtVariablesTable;
    *hndl = inst;
    *added = 1;
    return (&hndl[0]);
  }
  hndl = g_ExtVariablesTable;
  while (c<MAX_EXTRELATIONS) {
    if (hndl[c]==inst) {
      *added = 0;
      return &hndl[c];
    }
    if (hndl[c]==NULL) { /* store in the first NULL position */
      *added = 1;
      hndl[c] = inst;
      return &hndl[c];
    }
    c++;
  }
  Asc_Panic(2, NULL,
            "MAX_EXTRELATIONS internal limit has just been exceeded.\n"
            "Please report failure in AddVarToTable to :\n"
            "%s\n", ASC_BIG_BUGMAIL);
  exit(2);/* Needed to keep gcc from whining */
}

/*
 * This will be called only for models.
 */
void FixExternalVars(struct Instance *old,struct Instance *new)
{
  struct Instance **hndl;
  struct gl_list_t *exists;
  unsigned long len,c;
  if (g_ExtVariablesTable==NULL)
    return;
  if (old==new)
    return;
  if ((old==NULL) || (new==NULL)) {
    Asc_Panic(2, NULL,
              "Internal error in FixExternalVars\n"
              "Cannot handle NULL instances... exiting.\n");
  }
  exists = LookupVarInTable(old);
  if (exists) {
    len = gl_length(exists);
    for (c=1;c<=len;c++) {
      hndl = (struct Instance **)gl_fetch(exists,c);
      *hndl = new;
    }
    gl_destroy(exists);
    return;
  }
  return;
}

void SetSimulationExtVars(struct Instance *i,struct Instance **extvars)
{
  assert (i&&InstanceKind(i)==SIM_INST);
  SIM_INST(i)->extvars = extvars;
}


