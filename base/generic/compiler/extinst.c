/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1995 Kirk Andre Abbott

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//*
	Ascend Instance External Vars Functions
	by Kirk Abbott
	Last in CVS $Revision: 1.7 $ $Date: 1997/07/18 12:29:34 $ $Author: mthomas $
*/

#include <stdarg.h>
#include <utilities/ascConfig.h>
#include "compiler.h"
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include "instance_enum.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "expr_types.h"
#include "instquery.h"
#include "instance_types.h"
#include "instmacro.h"
#include "cmpfunc.h"
#include "extinst.h"

struct Instance **g_ExtVariablesTable = NULL;

static
void InitExtVariablesTable(void)
{
  int c;
  if (g_ExtVariablesTable==NULL) {
    g_ExtVariablesTable = ASC_NEW_ARRAY(struct Instance *,MAX_EXTRELATIONS);
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
  Asc_Panic(2, __FUNCTION__, "MAX_EXTRELATIONS limit exceeded.");
  
}


void FixExternalVars(struct Instance *old,struct Instance *new){
  struct Instance **hndl;
  struct gl_list_t *exists;
  unsigned long len,c;
  if (g_ExtVariablesTable==NULL)
    return;
  if (old==new)
    return;
  if ((old==NULL) || (new==NULL)) {
    Asc_Panic(2, __FUNCTION__,"Cannot handle NULL instances.");
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

