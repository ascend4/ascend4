/*
 *  When Utility functions
 *  by Vicente Rico-Ramirez
 *  Version: $Revision: 1.11 $
 *  Version control file: $RCSfile: when_util.c,v $
 *  Date last modified: $Date: 1997/07/29 15:52:59 $
 *  Last modified by: $Author: rv2a $
 *
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997  Carnegie Mellon Univeristy
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
#include "general/list.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/functype.h"
#include "compiler/types.h"
#include "compiler/instance_enum.h"
#include "compiler/case.h"
#include "compiler/extfunc.h"
#include "compiler/find.h"
#include "compiler/relation_type.h"
#include "compiler/relation.h"
#include "compiler/relation_type.h"
#include "compiler/instance_types.h"
#include "compiler/instquery.h"
#include "compiler/mathinst.h"
#include "compiler/rel_common.h"
#include "compiler/instmacro.h"
#include "compiler/when_util.h"


/********************************************************************\
                   WHEN instance utility functions
\********************************************************************/

unsigned long NumberWhenVariables(CONST struct Instance *when)
{
  struct gl_list_t *vars;
  unsigned long n;
  assert(when->t==WHEN_INST);
  vars = GetInstanceWhenVars(when);
  n = (vars!=NULL) ? gl_length(vars) : 0;
  return n;
}

struct Instance *WhenVariable(CONST struct Instance *when,
			      unsigned long int varnum)
{
  struct gl_list_t *vars;
  assert(when->t==WHEN_INST);
  vars = GetInstanceWhenVars(when);
  return (struct Instance *)gl_fetch(vars,varnum);
}


int VarFoundInWhen(CONST struct Instance *var, CONST struct Instance *when)
{
  int n,c;

  n = NumberWhenVariables(when);
  for (c=1;c<=n;c++) {
    if (var == WhenVariable(when,c)) {
      return 1;
    }
  }
  return 0;
}

unsigned long NumberWhenCases(CONST struct Instance *when)
{
  struct gl_list_t *cases;
  unsigned long n;
  assert(when->t==WHEN_INST);
  cases = GetInstanceWhenCases(when);
  n = (cases!=NULL) ? gl_length(cases) : 0;
  return n;
}

struct Case *WhenCase(CONST struct Instance *when,
		      unsigned long int casenum)
{
  struct gl_list_t *cases;
  assert(when->t==WHEN_INST);
  cases = GetInstanceWhenCases(when);
  return (struct Case *)gl_fetch(cases,casenum);
}


void ModifyWhenPointers(struct gl_list_t *reforvar,
			CONST struct Instance *old,
			CONST struct Instance *new)
{
  unsigned long pos,other;
  assert(reforvar!=NULL);

  if (old==new) return;
  if (new){
    if (0 != (pos = gl_search(reforvar,old,(CmpFunc)CmpP))) {
      if (0 != (other = gl_search(reforvar,new,(CmpFunc)CmpP))){
	gl_store(reforvar,pos,(VOIDPTR)new);	       /* case 3 */
        if (pos < other) Swap(&pos,&other);
        /* pos > other now */
        gl_delete(reforvar,pos,0);
      }
      else
	gl_store(reforvar,pos,(char *)new);            /* case 2 */
    }
    else{					       /* case 1 */
      FPRINTF(ASCERR,"Warning ModifiyWhenPointers not found.\n");
      FPRINTF(ASCERR,"This shouldn't effect your usage at all.\n");
    }
  }
  else						       /* case 4 */
    if (0 != (pos = gl_search(reforvar,old,(CmpFunc)CmpP)))
      gl_store(reforvar,pos,(VOIDPTR)new);
}


struct gl_list_t *CopyWhenBVarList(struct Instance *dest_inst,
				   struct gl_list_t *copylist)
{
  struct Instance *bvar;
  struct gl_list_t *newbvarlist = NULL;
  unsigned long len,c,pos;

  if (copylist) {
    len = gl_length(copylist);
    newbvarlist = gl_create(len);
    for (c=1;c<=len;c++) {
      bvar = (struct Instance *)gl_fetch(copylist,c);
      pos = gl_search(newbvarlist,bvar,(CmpFunc)CmpP);
      if (pos) {
	Asc_Panic(2, NULL, "Corrupted variable list in CopyWhenBVarList\n");
      }
      gl_append_ptr(newbvarlist,(VOIDPTR)bvar);
      switch(bvar->t){
        case BOOLEAN_ATOM_INST:
        case INTEGER_ATOM_INST:
        case SYMBOL_ATOM_INST:
        case BOOLEAN_CONSTANT_INST:
        case INTEGER_CONSTANT_INST:
        case SYMBOL_CONSTANT_INST:
          AddWhen(bvar,dest_inst);
          break;
        default:
          FPRINTF(ASCERR,"Incorrect instance in When Variable list\n");
      }
    }
  }
  else{	/* we will always return a varlist, even if empty */
    newbvarlist = gl_create(1L);
  }
  return newbvarlist;
}

struct gl_list_t *CopyWhenCaseRefList(struct Instance *dest_inst,
				      struct gl_list_t *copylist)
{
  struct Instance *ref;
  struct gl_list_t *newreflist = NULL;
  unsigned long len,c,pos;

  if (copylist) {
    len = gl_length(copylist);
    newreflist = gl_create(len);
    for (c=1;c<=len;c++) {
      ref = (struct Instance *)gl_fetch(copylist,c);
      pos = gl_search(newreflist,ref,(CmpFunc)CmpP);
      if (pos) {
	Asc_Panic(2, NULL, "Corrupted reference list in CopyWhenCaseRefList\n");
      }
      gl_append_ptr(newreflist,(VOIDPTR)ref);
      switch(ref->t){
        case MODEL_INST:
        case REL_INST:
        case LREL_INST:
        case WHEN_INST:
          AddWhen(ref,dest_inst);
          break;
        default:
          FPRINTF(ASCERR,"Incorrect instance in Case Reference list\n");
      }
    }
  }
  else{	/* we will always return a varlist, even if empty */
    newreflist = gl_create(1L);
  }
  return newreflist;
}


void DestroyWhenVarList(struct gl_list_t *l, struct Instance *inst)
{
  register struct Instance *ptr;
  register unsigned long c;
  for(c=gl_length(l);c>=1;c--){
    if (NULL != (ptr = (struct Instance *)gl_fetch(l,c))){
      switch(ptr->t) {
      case BOOLEAN_ATOM_INST:
      case INTEGER_ATOM_INST:
      case SYMBOL_ATOM_INST:
      case BOOLEAN_CONSTANT_INST:
      case INTEGER_CONSTANT_INST:
      case SYMBOL_CONSTANT_INST:
        RemoveWhen(ptr,inst);
        break;
      default:
        FPRINTF(ASCERR,"Incorrect instance in When Variable list\n");
      }
    }
  }
  gl_destroy(l);
}


static
void DestroyWhenCases(struct Case *cur_case, struct Instance *inst)
{
  register struct Instance *ptr;
  register unsigned long c,len;
  struct gl_list_t *reflist;

  reflist = GetCaseReferences(cur_case);
  assert(reflist!=NULL);
  len = gl_length(reflist);

  for(c=len;c>=1;c--){
     if (NULL != (ptr = (struct Instance *)gl_fetch(reflist,c))){
      switch(ptr->t) {
      case MODEL_INST:
      case REL_INST:
      case LREL_INST:
      case WHEN_INST:
        RemoveWhen(ptr,inst);
        break;
      default:
        FPRINTF(ASCERR,"Incorrect instance in DestroyWhenCases\n");
      }
    }
  }
  DestroyCase(cur_case);
}


void DestroyWhenCaseList(struct gl_list_t *l, struct Instance *inst)
{
  struct Case *cur_case;
  register unsigned long c,len;
  len = gl_length(l);

  for(c=len;c>=1;c--){
    cur_case = (struct Case *)gl_fetch(l,c);
    DestroyWhenCases(cur_case,inst);
  }
  gl_destroy(l);
}


/***********************************************************************\
  Model Filtration functions. This file will provide a mean to "turn
  off/on" all the models included in a WHEN statement.
\***********************************************************************/

unsigned int model_flags( struct Instance *mod)
{
  assert( mod!=NULL && InstanceKind(mod)==MODEL_INST);
  return MOD_INST(mod)->anon_flags;
}

void model_set_flags(struct Instance *mod, unsigned int flags)
{
  assert( mod!=NULL && InstanceKind(mod)==MODEL_INST);
  MOD_INST(mod)->anon_flags = flags;
}

unsigned int model_flagbit(CONST struct Instance *mod, unsigned int one)
{
  if (mod==NULL) {
    FPRINTF(stderr,"ERROR: model_flagbit called with bad model.\n");
    return 0;
  }
  assert(InstanceKind(mod)==MODEL_INST);
  return (MOD_INST(mod)->anon_flags & one);
}

void model_set_flagbit(struct Instance *mod, unsigned int field,
                       unsigned int one)
{
  assert( mod!=NULL && InstanceKind(mod)==MODEL_INST);
  if (one) {
    MOD_INST(mod)->anon_flags |= field;
  } else {
    MOD_INST(mod)->anon_flags &= ~field;
  }
}


/***********************************************************************\
  Relation Filtration functions. This file will provide a mean to "turn
  off/on" all the relations/logrelations  included in a WHEN statement.
\***********************************************************************/

unsigned int relinst_flags( struct Instance *rel)
{
  assert( rel!=NULL && InstanceKind(rel)==REL_INST);
  return RELN_INST(rel)->anon_flags;
}

void relinst_set_flags(struct Instance *rel, unsigned int flags)
{
  assert( rel!=NULL && InstanceKind(rel)==REL_INST);
  RELN_INST(rel)->anon_flags = flags;
}

unsigned int relinst_flagbit(CONST struct Instance *rel, unsigned int one)
{
  if (rel==NULL) {
    FPRINTF(stderr,"ERROR: relinst_flagbit called with bad relation.\n");
    return 0;
  }
  assert( rel!=NULL && InstanceKind(rel)==REL_INST);
  return (RELN_INST(rel)->anon_flags & one);
}

void relinst_set_flagbit(struct Instance *rel, unsigned int field,
                         unsigned int one)
{
  assert( rel!=NULL && InstanceKind(rel)==REL_INST);
  if (one) {
    RELN_INST(rel)->anon_flags |= field;
  } else {
    RELN_INST(rel)->anon_flags &= ~field;
  }
}

unsigned int logrelinst_flags( struct Instance *lrel)
{
  assert( lrel!=NULL && InstanceKind(lrel)==LREL_INST);
  return LRELN_INST(lrel)->anon_flags;
}

void logrelinst_set_flags(struct Instance *lrel, unsigned int flags)
{
  assert( lrel!=NULL && InstanceKind(lrel)==LREL_INST);
  LRELN_INST(lrel)->anon_flags = flags;
}

unsigned int logrelinst_flagbit(CONST struct Instance *lrel, unsigned int one)
{
  if (lrel==NULL) {
    FPRINTF(stderr,"ERROR: logrelinst_flagbit called with bad logrelation.\n");
    return 0;
  }
  assert( lrel!=NULL && InstanceKind(lrel)==LREL_INST);
  return (LRELN_INST(lrel)->anon_flags & one);
}

void logrelinst_set_flagbit(struct Instance *lrel, unsigned int field,
                         unsigned int one)
{
  assert( lrel!=NULL && InstanceKind(lrel)==LREL_INST);
  if (one) {
    LRELN_INST(lrel)->anon_flags |= field;
  } else {
    LRELN_INST(lrel)->anon_flags &= ~field;
  }
}



