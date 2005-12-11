/*
 *  FOR Loop Index Variable Table
 *  by Tom Epperly
 *  Created: 1/14/89
 *  Version: $Revision: 1.11 $
 *  Version control file: $RCSfile: forvars.c,v $
 *  Date last modified: $Date: 1998/02/05 16:36:06 $
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

#include "utilities/ascConfig.h"
#include "compiler/compiler.h"
#include "utilities/ascPanic.h"
#include "utilities/ascMalloc.h"
#include "general/list.h"
#include "compiler/symtab.h"
#include "compiler/instance_enum.h"
#include "compiler/cmpfunc.h"
#include "compiler/forvars.h"
#include "compiler/setinstval.h"
#include "compiler/setinst_io.h"

#ifndef lint
static CONST char ForVarRCSid[] = "$Id: forvars.c,v 1.11 1998/02/05 16:36:06 ballan Exp $";
#endif
#define FVMALLOC ForVarMalloc()

/* LIFO forvar list. contents of forvar recycle list are only the heads,
we do not save the contents of the var when it is destroyed. */
struct for_var_t *g_forvar_recycle_list = NULL;

static struct for_var_t *ForVarMalloc(void)
{
  struct for_var_t *result;
  if (g_forvar_recycle_list!=NULL) {
    /* pop off list */
    result = g_forvar_recycle_list;
    g_forvar_recycle_list = (struct for_var_t *)g_forvar_recycle_list->name;
    return result;
  } else {
    return (struct for_var_t *)ascmalloc(sizeof(struct for_var_t));
  }
}

struct for_table_t *CreateForTable(void)
{
  register struct for_table_t *result;
  result =gl_create(4L);
  AssertMemory(result);
  return result;
}

static FILE *g_forvarfile = NULL;
/* writes ' name = value,' for all cases */
static void WriteForVar(FILE *fp, struct for_var_t *fv)
{
  if (fv!=NULL){
    AssertAllocatedMemory(fv,sizeof(struct for_var_t ));
    switch (fv->t) {
    case f_set:
      FPRINTF(fp," %s = [",SCP(fv->name));
      WriteInstSet(fp,fv->value.sptr);
      FPRINTF(fp,"],");
      break;
    case f_integer:
      FPRINTF(fp," %s = %lu,",SCP(fv->name),fv->value.ivalue);
      break;
    case f_symbol:
      FPRINTF(fp," %s = %s,",SCP(fv->name),SCP(fv->value.sym_ptr));
      break;
    default:
      FPRINTF(fp," %s = UNDEFINED,",SCP(fv->name));
    }
  } else {
    FPRINTF(fp,"NULL FOR indices!\n");
  }
}

static
void WriteForVarIterate(struct for_var_t *ft)
{
  WriteForVar(g_forvarfile,ft);
}

void WriteForTable(FILE *fp,struct gl_list_t *ft)
{
  if(ft!=NULL && fp!=NULL){
    g_forvarfile = fp;
    AssertMemory(ft);
    gl_iterate(ft,(void (*)(VOIDPTR))WriteForVarIterate);
  }
}

void DestroyForTable(struct gl_list_t *ft)
{
  if(ft!=NULL){
    AssertMemory(ft);
    gl_iterate(ft,(void (*)(VOIDPTR))DestroyForVar);
    gl_destroy(ft);
  }
}

unsigned long ActiveForLoops(CONST struct gl_list_t *ft)
{
  assert(ft!=NULL);
  AssertMemory(ft);
  return gl_length(ft);
}

void AddLoopVariable(struct gl_list_t *ft, struct for_var_t *var)
{
  assert(ft && var);
  AssertMemory(ft);
  AssertMemory(var);
  gl_append_ptr(ft,(VOIDPTR)var);
}

struct for_var_t *LoopIndex(CONST struct gl_list_t *ft, unsigned long int num)
{
  assert(ft!=NULL);
  AssertMemory(ft);
  return (struct for_var_t *)gl_fetch(ft,num);
}

/*
 * This function is used in FindForVar only. It returns 0 for
 * an exact match and 1 OTHERWISE, so it is not appropriate
 * for sorting lists. Forvar names are assumed to come from
 * the symbol table, as the symchar implies.
 */
static
int CmpForVars(struct for_var_t *fv1, struct for_var_t *fv2)
{
  assert(fv1 && fv2);
  /* return CmpSymchar(fv1->name,fv2->name); */
  return (fv1->name != fv2->name);
}

struct for_var_t *FindForVar(CONST struct gl_list_t *ft, symchar *name)
{
  unsigned long pos;
  struct for_var_t match;
  assert(ft && name);
  AssertMemory(ft);
  match.name = name;
  pos = gl_search(ft,(char *)&match,(CmpFunc)CmpForVars); /* linear */
  if (pos != 0) {
    return (struct for_var_t *)gl_fetch(ft,pos);
  }
  return NULL;
}

void RemoveForVariable(struct gl_list_t *ft)
{
  unsigned long length;
  struct for_var_t *fv;
  length = gl_length(ft);
  if (length>0){
    fv = (struct for_var_t *)gl_fetch(ft,length);
    DestroyForVar(fv);
    gl_delete(ft,length,0);
  }
}

/*********************************************************************\
Routines to process for_var_t types.
\*********************************************************************/

struct for_var_t *CreateForVar(symchar *name)
{
  register struct for_var_t *result;
  assert(AscFindSymbol(name)!=NULL);
  result = FVMALLOC;
  result->t = f_untyped;
  result->name = name;
  AssertAllocatedMemory(result,sizeof(struct for_var_t));
  return result;
}

void SetForVarType(struct for_var_t *fv, enum for_kind t)
{
  assert(fv && (fv->t==f_untyped));
  AssertAllocatedMemory(fv,sizeof(struct for_var_t ));
  fv->t = t;
}

void SetForInteger(struct for_var_t *fv, long int ivalue)
{
  assert(fv && (fv->t==f_integer));
  AssertAllocatedMemory(fv,sizeof(struct for_var_t ));
  fv->value.ivalue = ivalue;
}

void SetForSymbol(struct for_var_t *fv, symchar *sym_ptr)
{
  assert(fv && (fv->t==f_symbol));
  AssertAllocatedMemory(fv,sizeof(struct for_var_t ));
  assert(AscFindSymbol(sym_ptr)!=NULL);
  fv->value.sym_ptr = sym_ptr;
}

void SetForSet(struct for_var_t *fv, struct set_t *sptr)
{
  assert(fv && (fv->t == f_set));
  AssertAllocatedMemory(fv,sizeof(struct for_var_t ));
  fv->value.sptr = sptr;
}

enum for_kind GetForKind(CONST struct for_var_t *fv)
{
  assert(fv!=NULL);
  AssertAllocatedMemory(fv,sizeof(struct for_var_t ));
  return fv->t;
}

symchar *GetForName(CONST struct for_var_t *fv)
{
  assert(fv!=NULL);
  AssertAllocatedMemory(fv,sizeof(struct for_var_t ));
  return fv->name;
}

long GetForInteger(CONST struct for_var_t *fv)
{
  assert(fv && (fv->t == f_integer));
  AssertAllocatedMemory(fv,sizeof(struct for_var_t ));
  return fv->value.ivalue;
}

symchar *GetForSymbol(CONST struct for_var_t *fv)
{
  assert(fv && (fv->t == f_symbol));
  AssertAllocatedMemory(fv,sizeof(struct for_var_t ));
  return fv->value.sym_ptr;
}

CONST struct set_t *GetForSet(CONST struct for_var_t *fv)
{
  assert(fv && (fv->t == f_set));
  AssertAllocatedMemory(fv,sizeof(struct for_var_t ));
  return fv->value.sptr;
}

void DestroyForVar(struct for_var_t *fv)
{
  if (fv!=NULL){
    AssertAllocatedMemory(fv,sizeof(struct for_var_t ));
    if (fv->t == f_set) DestroySet(fv->value.sptr);
    /* push on list */
    fv->name = (symchar *)g_forvar_recycle_list; /* push on stack */
    g_forvar_recycle_list = fv;
  }
}

int ClearForVarRecycle(void)
{
  struct for_var_t *tmp;
  int len=0;
  while (g_forvar_recycle_list!=NULL) {
    tmp = g_forvar_recycle_list;
    g_forvar_recycle_list = (struct for_var_t *)g_forvar_recycle_list->name;
    ascfree((char *)tmp);
    len++;
  }
  return len;
}
