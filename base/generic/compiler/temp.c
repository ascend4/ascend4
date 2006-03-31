/*
 *  Temporary Variable Implementation
 *  by Tom Epperly
 *  Created: 1/17/89
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: temp.c,v $
 *  Date last modified: $Date: 1998/04/19 18:45:34 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
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
#include <utilities/ascConfig.h>
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include "compiler.h"
#include "symtab.h"
#include "instance_enum.h"
#include "cmpfunc.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "value_type.h"
#include "temp.h"

#ifndef lint
static CONST char TemporaryRCSid[] = "$Id: temp.c,v 1.8 1998/04/19 18:45:34 ballan Exp $";
#endif

struct gl_list_t *g_temporary_var_list = NULL;
/* a list of active vars */
#define GTVL g_temporary_var_list
/* name is too damn long */

union temp_var_u{
  long i;
  symchar *sym;
};

struct temp_var_t {
  symchar *name;
  int isint;
  union temp_var_u value;
};

#define VMALLOC TempVarMalloc()

/* LIFO tempvar list.
 */
struct temp_var_t *g_temporary_var_recycle = NULL;

static struct temp_var_t *TempVarMalloc(void)
{
  struct temp_var_t *result;
  if (g_temporary_var_recycle!=NULL) {
    /* pop off list */
    result = g_temporary_var_recycle;
    g_temporary_var_recycle =
      (struct temp_var_t *)g_temporary_var_recycle->name;
    return result;
  } else {
    return (struct temp_var_t *)ascmalloc(sizeof(struct temp_var_t));
  }
}

static void TempVarRecycle(struct temp_var_t *tv)
{
  if (tv!=NULL){
    AssertAllocatedMemory(tv,sizeof(struct temp_var_t ));
    /* push on list */
    tv->name = (symchar *)g_temporary_var_recycle;
    g_temporary_var_recycle = tv;
  }
}

static
int CmpVars(CONST struct temp_var_t *v1, CONST struct temp_var_t *v2)
{
  assert(v1&&v2);
  return CmpSymchar(v1->name,v2->name);
}

void AddTemp(symchar *name)
{
  struct temp_var_t *ptr;
  assert(AscFindSymbol(name)!=NULL);
  if (GTVL==NULL) GTVL = gl_create(10);
  ptr = VMALLOC;
  ptr->name = name;
  ptr->isint = -1;
  gl_insert_sorted(GTVL,ptr,(CmpFunc)CmpVars);
}

void DestroyTemporaryList()
{
  struct temp_var_t *tmp;
  if (GTVL){
    if (gl_length(GTVL)){
      FPRINTF(ASCERR,
        "Warning temporary variable list still has entries at shutdown.\n");
    }
    gl_free_and_destroy(GTVL);
    GTVL = NULL;
  }
  while (g_temporary_var_recycle!=NULL) {
    tmp = g_temporary_var_recycle;
    g_temporary_var_recycle =
      (struct temp_var_t *)g_temporary_var_recycle->name;
    ascfree((char *)tmp);
  }
}

void SetTemp(symchar *name, struct value_t value)
{
  struct temp_var_t *ptr,rec;
  unsigned long pos;
  if (GTVL==NULL){
    Asc_Panic(2, NULL, "SetTemp called before AddTemp.\n");
  }
  rec.name = name;
  pos = gl_search(GTVL,&rec,(CmpFunc)CmpVars);
  if (pos==0){
    Asc_Panic(2, NULL, "Unable to locate temporary variable %s.\n",name);
  }
  ptr = (struct temp_var_t *)gl_fetch(GTVL,pos);
  switch(ValueKind(value)){
  case integer_value:
    ptr->isint = 1;
    ptr->value.i = IntegerValue(value);
    break;
  case symbol_value:
    ptr->isint = 0;
    ptr->value.sym = SymbolValue(value);
    assert(AscFindSymbol(ptr->value.sym)!=NULL);
    break;
  default:
    Asc_Panic(2, NULL,
              "This temporary variable module can only handle "
              "integer and symbol variables.\n");
  }
}

void RemoveTemp(symchar *name)
{
  struct temp_var_t rec;
  unsigned long pos;
  if (GTVL==NULL){
    Asc_Panic(2, NULL, "RemoveTemp called before AddTemp.\n");
  }
  assert(AscFindSymbol(name)!=NULL);
  rec.name = name;
  pos = gl_search(GTVL,&rec,(CmpFunc)CmpVars);
  if (pos==0) {
    FPRINTF(ASCERR,"Warning RemoveTemp called on non-existent variable %s.\n",
	    SCP(name));
  } else {
    TempVarRecycle(gl_fetch(GTVL,pos));
    gl_delete(GTVL,pos,0);
  }
}

int TempExists(symchar *name)
{
  struct temp_var_t rec;
  if (GTVL==NULL) return 0;
  assert(AscFindSymbol(name)!=NULL);
  rec.name = name;
  return (gl_search(GTVL,&rec,(CmpFunc)CmpVars)!=0);
}

struct value_t TempValue(symchar *name)
{
  struct temp_var_t rec,*ptr;
  unsigned long pos;
  assert(AscFindSymbol(name)!=NULL);
  if (GTVL==NULL) return CreateErrorValue(name_unfound);
  rec.name = name;
  pos = gl_search(GTVL,&rec,(CmpFunc)CmpVars);
  if (pos==0) return CreateErrorValue(name_unfound);
  ptr = (struct temp_var_t *)gl_fetch(GTVL,pos);
  switch(ptr->isint){
  case -1: return CreateErrorValue(undefined_value);
  case 0: return CreateSymbolValue(ptr->value.sym,0);
  case 1: return CreateIntegerValue(ptr->value.i,0);
  }
  /*NOTREACHED*/
  return CreateErrorValue(undefined_value);
}


