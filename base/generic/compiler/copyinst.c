/*
 *  Ascend Instance Tree Type Implementation
 *  by Tom Epperly	    9/3/89
 *  Version: $Revision: 1.18 $
 *  Version control file: $RCSfile: copyinst.c,v $
 *  Date last modified: $Date: 1998/03/17 22:08:28 $
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
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <general/pool.h>
#include <general/list.h>
#include <general/dstring.h>

#include "bit.h"
#include "symtab.h"
#include "functype.h"
#include "expr_types.h"
#include "childinfo.h"
#include "child.h"
#include "type_desc.h"
#include "instance_name.h"
#include "instance_types.h"
#include "instance_io.h"
#include "instmacro.h"
#include "visitinst.h"
#include "parentchild.h"
#include "extinst.h"
#include "instquery.h"
#include "linkinst.h"
#include "destroyinst.h"
#include "arrayinst.h"
#include "mathinst.h"
#include "createinst.h"
#include "refineinst.h"
#include "atomvalue.h"
#include "atomsize.h"
#include "check.h"
#include "dump.h"
#include "prototype.h"
#include "pending.h"
#include "find.h"
#include "extfunc.h"
#include "rel_blackbox.h"
#include "vlist.h"
#include "relation.h"
#include "logical_relation.h"
#include "logrelation.h"
#include "relation_util.h"
#include "logrel_util.h"
#include "rel_common.h"
#include "case.h"
#include "when_util.h"
#include "universal.h"
#include <general/pool.h>
#include "tmpnum.h"
#include "cmpfunc.h"
#include "setinstval.h"
#include "copyinst.h"

#ifndef lint
static CONST char CopyInstModuleID[] = "$Id: copyinst.c,v 1.18 1998/03/17 22:08:28 ballan Exp $";
#endif


/*
 * This function simply makes a first pass at determining
 * whether the instance is a universal instance. It returns
 * non NULL, if TRUE.
 */
struct Instance *ShortCutMakeUniversalInstance(struct TypeDescription *type)
{
  struct Instance *result;
  if (GetUniversalFlag(type)&&
      (0 != (result=LookupInstance(GetUniversalTable(),type)))) {
    return result;
  } else {
    return NULL;
  }
}

/*
 * This function simply makes a first pass at determining
 * whether a prototype of the instance exists. It returns
 * non NULL, if TRUE.
 */
struct Instance *ShortCutProtoInstance(struct TypeDescription *type)
{
  struct Instance *result, *proto;

  result = LookupPrototype(GetName(type));
  if (result) {
    proto = CopyInstance(result);
    return proto;
  } else {
    return NULL;
  }
}

/*********************************************************************\
  Copy Instance code.
\*********************************************************************/

void CheckChildCopies(register unsigned long int num,
		      register struct Instance **clist)
{
  register unsigned long c;
  for(c=0;c<num;c++) {
    if ((clist[c]->t==SET_INST)&&(S_INST(clist[c])->list!=NULL)) {
      S_INST(clist[c])->list = CopySet(S_INST(clist[c])->list);
    }
  }
}

void RedoChildPointers(unsigned long int num,
		       struct Instance *newparent,
		       struct Instance **newchildptrs,
		       CONST struct Instance *oldparent,
		       struct Instance * CONST *oldchildptrs)
{
  while (num > 0) {
    *newchildptrs = INST((unsigned long)newparent+
			 (unsigned long)(*oldchildptrs)-
			 (unsigned long)oldparent);
    newchildptrs++;
    oldchildptrs++;
    num--;
  }
}

/*
 * mem copy of a REAL_ATOM_INSTANCE or REAL_CONSTANT_INSTANCE.
 */
static
struct Instance *CopyReal(CONST struct Instance *i)
{
  if (IsAtomicInstance(i)) {
    register struct RealAtomInstance *src,*result;
    register unsigned long size;
    AssertMemory(i);
    src = RA_INST(i);
    size = GetByteSize(src->desc);
    result = RA_INST(ascmalloc((unsigned)size));
    ascbcopy((char *)src,(char *)result,(int)size);
    result->parents = gl_create(AVG_PARENTS);
    result->alike_ptr = INST(result);
    result->relations = NULL;	/* initially the copy isn't in any relations */
    CopyTypeDesc(result->desc);
    RedoChildPointers(ChildListLen(GetChildList(result->desc)),
		      INST(result),RA_CHILD(result,0),
		      i,RA_CHILD(i,0));
    CheckChildCopies(ChildListLen(GetChildList(result->desc)),
		     RA_CHILD(result,0));
    AssertMemory(result);
    return INST(result);
  } else {
    register struct RealConstantInstance *src,*result;
    register unsigned long size;
    AssertMemory(i);
    src = RC_INST(i);
    size = GetByteSize(src->desc);
    result = RC_INST(ascmalloc((unsigned)size));
    ascbcopy((char *)src,(char *)result,(int)size);
    result->parents = gl_create(AVG_CONSTANT_PARENTS);
    result->alike_ptr = INST(result);
    CopyTypeDesc(result->desc);
    AssertMemory(result);
    return INST(result);
  }
}

/*
 * mem copy of a INTEGER_ATOM_INSTANCE or INTEGER_CONSTANT_INSTANCE.
 */
static
struct Instance *CopyInteger(CONST struct Instance *i)
{
  if (IsAtomicInstance(i)) {
    register struct IntegerAtomInstance *src,*result;
    register unsigned long size;

    AssertMemory(i);
    src = IA_INST(i);
    size = GetByteSize(src->desc);
    result = IA_INST(ascmalloc((unsigned)size));
    ascbcopy((char *)src,(char *)result,(int)size);
    result->parents = gl_create(AVG_PARENTS);
    result->alike_ptr = INST(result);
    result->whens = NULL;
    CopyTypeDesc(result->desc);
    RedoChildPointers(ChildListLen(GetChildList(result->desc)),
		      INST(result),IA_CHILD(result,0),
		      i,IA_CHILD(i,0));
    CheckChildCopies(ChildListLen(GetChildList(result->desc)),
		     IA_CHILD(result,0));
    AssertMemory(result);
    return INST(result);
  } else {
    register struct IntegerConstantInstance *src,*result;
    register unsigned long size;

    AssertMemory(i);
    src = IC_INST(i);
    size = GetByteSize(src->desc);
    result = IC_INST(ascmalloc((unsigned)size));
    ascbcopy((char *)src,(char *)result,(int)size);
    result->parents = gl_create(AVG_ICONSTANT_PARENTS);
    result->alike_ptr = INST(result);
    result->whens = NULL;
    CopyTypeDesc(result->desc);
    AssertMemory(result);
    return INST(result);
  }
}

/*
 * mem copy of a BOOLEAN_ATOM_INSTANCE or BOOLEAN_CONSTANT_INSTANCE.
 */
static
struct Instance *CopyBoolean(CONST struct Instance *i)
{
  if (IsAtomicInstance(i)) {
    register struct BooleanAtomInstance *src,*result;
    register unsigned long size;
    AssertMemory(i);
    src = BA_INST(i);
    size = GetByteSize(src->desc);
    result = BA_INST(ascmalloc((unsigned)size));
    ascbcopy((char *)src,(char *)result,(int)size);
    result->parents = gl_create(AVG_PARENTS);
    result->alike_ptr = INST(result);
    result->logrelations = NULL;
    result->whens = NULL;
    /* initially the copy isn't in any logical relation or when */
    CopyTypeDesc(result->desc);
    RedoChildPointers(ChildListLen(GetChildList(result->desc)),
		      INST(result),BA_CHILD(result,0),
		      i,BA_CHILD(i,0));
    CheckChildCopies(ChildListLen(GetChildList(result->desc)),
		     BA_CHILD(result,0));
    AssertMemory(result);
    return INST(result);
  } else {
    register struct BooleanConstantInstance *src,*result;
    register unsigned long size;
    AssertMemory(i);
    src = BC_INST(i);
    size = GetByteSize(src->desc);
    result = BC_INST(ascmalloc((unsigned)size));
    ascbcopy((char *)src,(char *)result,(int)size);
    result->parents = gl_create(AVG_PARENTS);
    result->alike_ptr = INST(result);
    result->whens = NULL;
    CopyTypeDesc(result->desc);
    AssertMemory(result);
    return INST(result);
  }
}

static
struct Instance *CopySetInst(CONST struct Instance *i)
{
  register struct SetAtomInstance *src, *result;
  register unsigned long size;
  AssertMemory(i);
  src = SA_INST(i);
  size = GetByteSize(src->desc);
  result = SA_INST(ascmalloc((unsigned)size));
  ascbcopy((char *)src,(char *)result,(int)size);
  if (src->list!=NULL)
    result->list = CopySet(src->list);
  result->parents = gl_create(AVG_PARENTS);
  result->alike_ptr = INST(result);
  CopyTypeDesc(result->desc);
  RedoChildPointers(ChildListLen(GetChildList(result->desc)),
		    INST(result),SA_CHILD(result,0),
		    i,SA_CHILD(i,0));
  CheckChildCopies(ChildListLen(GetChildList(result->desc)),
		   SA_CHILD(result,0));
  AssertMemory(result);
  return INST(result);
}

/*
 * mem copy of a SYMBOL_ATOM_INSTANCE or SYMBOL_CONSTANT_INSTANCE.
 */
static
struct Instance *CopySymbolInst(CONST struct Instance *i)
{
  if (IsAtomicInstance(i)) {
    register struct SymbolAtomInstance *src,*result;
    register unsigned long size;
    AssertMemory(i);
    src = SYMA_INST(i);
    size = GetByteSize(src->desc);
    result = SYMA_INST(ascmalloc((unsigned)size));
    ascbcopy((char *)src,(char *)result,(int)size);
    result->parents = gl_create(AVG_PARENTS);
    result->alike_ptr = INST(result);
    result->whens = NULL;
    CopyTypeDesc(result->desc);
    RedoChildPointers(ChildListLen(GetChildList(result->desc)),
		      INST(result),SYMA_CHILD(result,0),
		      i,SYMA_CHILD(i,0));
    CheckChildCopies(ChildListLen(GetChildList(result->desc)),
		     SYMA_CHILD(result,0));
    AssertMemory(result);
    return INST(result);
  } else {
    register struct SymbolConstantInstance *src,*result;
    register unsigned long size;
    AssertMemory(i);
    src = SYMC_INST(i);
    size = GetByteSize(src->desc);
    result = SYMC_INST(ascmalloc((unsigned)size));
    ascbcopy((char *)src,(char *)result,(int)size);
    result->parents = gl_create(AVG_ICONSTANT_PARENTS);
    result->alike_ptr = INST(result);
    result->whens = NULL;
    CopyTypeDesc(result->desc);
    AssertMemory(result);
    return INST(result);
  }
}

static
struct Instance *CopyRelationInst(CONST struct Instance *i)
{
  register struct RelationInstance *src,*result;
  register unsigned long size;
  AssertMemory(i);
  src = RELN_INST(i);
  size = GetByteSize(src->desc);
  result = RELN_INST(ascmalloc((unsigned)size));
  ascbcopy((char *)src,(char *)result,(int)size);
  result->parent[0] = NULL;
  result->parent[1] = NULL;
  result->whens = NULL;
  result->logrels = NULL;
  result->anon_flags = 0x0;
  CopyTypeDesc(result->desc);
  if(result->ptr){
    CONSOLE_DEBUG("Clearing rel ptr %p",result->ptr);
  }
  result->ptr = NULL;

  RedoChildPointers(ChildListLen(GetChildList(result->desc)),
		    INST(result),REL_CHILD(result,0),
		    i,REL_CHILD(i,0));
  CheckChildCopies(ChildListLen(GetChildList(result->desc)),
		   REL_CHILD(result,0));
  AssertMemory(result);
  return INST(result);
}

static
struct Instance *CopyLogRelInst(CONST struct Instance *i)
{
  register struct LogRelInstance *src,*result;
  register unsigned long size;
  AssertMemory(i);
  src = LRELN_INST(i);
  size = GetByteSize(src->desc);
  result = LRELN_INST(ascmalloc((unsigned)size));
  ascbcopy((char *)src,(char *)result,(int)size);
  result->parent[0] = NULL;
  result->parent[1] = NULL;
  result->whens = NULL;
  result->logrels = NULL;
  result->anon_flags = 0x0;
  CopyTypeDesc(result->desc);
  if(result->ptr!=NULL){
    CONSOLE_DEBUG("Clearing rel ptr %p",result->ptr);
  }
  result->ptr = NULL;
  RedoChildPointers(ChildListLen(GetChildList(result->desc)),
		    INST(result),LREL_CHILD(result,0),
		    i,LREL_CHILD(i,0));
  CheckChildCopies(ChildListLen(GetChildList(result->desc)),
		   LREL_CHILD(result,0));
  AssertMemory(result);
  return INST(result);
}


static
struct Instance *CopyWhenInst(CONST struct Instance *i)
{
  register struct WhenInstance *src,*result;
  register unsigned long size;
  AssertMemory(i);
  src = W_INST(i);
  size = sizeof(struct WhenInstance);
  result = W_INST(ascmalloc((unsigned)size));
  ascbcopy((char *)src,(char *)result,(int)size);
  result->parent[0] = NULL;
  result->parent[1] = NULL;
  CopyTypeDesc(result->desc);
  result->whens = NULL;
  result->cases = NULL;
  result->bvar = NULL;
  AssertMemory(result);
  return INST(result);
}

/*
 * We dont use bcopy here so that we have to make sure
 * that we individually copy the relevant fields from
 * the source instance.
 */
static
struct Instance *CopyModel(CONST struct Instance *i)
{
  register struct ModelInstance *mod,*result;
  register unsigned long num_children;
  register struct TypeDescription *type;
  AssertMemory(i);
  mod = MOD_INST(i);
  type = mod->desc;
  CopyTypeDesc(type);
  num_children = ChildListLen(GetChildList(type));
  result = MOD_INST(ascmalloc((unsigned)sizeof(struct ModelInstance)+
			      (unsigned)num_children*
			      (unsigned)sizeof(struct Instance *)));
  result->t = MODEL_INST;
  result->pending_entry = NULL;
  result->interface_ptr = mod->interface_ptr;
  result->parents = gl_create(gl_length(mod->parents));
  result->whens = NULL;
  result->desc = type;
  result->alike_ptr = INST(result);
  result->visited = 0;
  result->tmp_num = 0;
  result->anon_flags = 0x0;
  result->executed = CopyBList(mod->executed);
  ZeroNewChildrenEntries(MOD_CHILD(result,0),num_children);
  AssertMemory(result);
  return INST(result);
}

static struct gl_list_t *CopyArrayChildPtrs(struct gl_list_t *list)
{
  register struct gl_list_t *result;
  register unsigned long length,c;
  register struct ArrayChild *new,*src;
  if (list!=NULL){
    length = gl_length(list);
    if (length) {
      result = gl_create(length);
      for(c=1;c<=length;c++) {
	src = (struct ArrayChild *)gl_fetch(list,c);
	new = MALLOCPOOLAC;
	*new = *src;
	new->inst = NULL;
	gl_append_ptr(result,(VOIDPTR)new);
      }
      gl_set_sorted(result, TRUE); /* because the originals were, by name */
      return result;
    } else {
      return gl_create(AVG_ARY_CHILDREN);
    }
  } else  {
    return NULL;
  }
}

static struct Instance *CopyArray(CONST struct Instance *i)
{
  register struct ArrayInstance *ary,*result;
  AssertMemory(i);
  ary = ARY_INST(i);
  result = ARY_INST(ascmalloc(sizeof(struct ArrayInstance)));
  result->t = ary->t;
  result->pending_entry = NULL;
  result->desc = ary->desc;
  CopyTypeDesc(result->desc);
  result->indirected = ary->indirected;
  result->visited = 0;
  result->tmp_num = 0;
  result->parents = gl_create(AVG_PARENTS);
  result->children = CopyArrayChildPtrs(ary->children);
  AssertMemory(result);
  return INST(result);
}


/*
 * This is just a preprocessing function. The work of doing
 * sanity checking of the incidence lists, and for
 * making the variable aware of the RELINST is that of
 * CopyRelationVarList in relation.[ch]
 */
static
void BuildRelationVarList(CONST struct Instance *src,
			  struct gl_list_t *dest_list,
			  struct gl_list_t *varlist)
{
  struct Instance *i,*ptr;
  CONST struct gl_list_t *src_varlist;
  CONST struct relation *rel;
  unsigned long len,c,copynum;

  assert(src->t==REL_INST);

  rel = RELN_INST(src)->ptr;
  src_varlist = RelationVarList(rel);
  if (!src_varlist) return;

  len = gl_length(src_varlist);
  for (c=1;c<=len;c++) {
    i = (struct Instance *)gl_fetch(src_varlist,c);
    copynum = GetTmpNum(i);
    ptr = (struct Instance *)gl_fetch(dest_list,copynum);
    gl_append_ptr(varlist,(VOIDPTR)ptr);
  }
}

static
void BuildLogRelBVarList(CONST struct Instance *src,
			  struct gl_list_t *dest_list,
			  struct gl_list_t *bvarlist)
{
  struct Instance *i,*ptr;
  CONST struct gl_list_t *src_bvarlist;
  CONST struct logrelation *lrel;
  unsigned long len,c,copynum;

  assert(src->t==LREL_INST);

  lrel = LRELN_INST(src)->ptr;
  src_bvarlist = LogRelBoolVarList(lrel);
  if (!src_bvarlist) return;

  len = gl_length(src_bvarlist);
  for (c=1;c<=len;c++) {
    i = (struct Instance *)gl_fetch(src_bvarlist,c);
    copynum = GetTmpNum(i);
    ptr = (struct Instance *)gl_fetch(dest_list,copynum);
    gl_append_ptr(bvarlist,(VOIDPTR)ptr);
  }
}


static
void BuildLogRelSatRelList(CONST struct Instance *src,
			   struct gl_list_t *dest_list,
			   struct gl_list_t *rellist)
{
  struct Instance *i,*ptr;
  CONST struct gl_list_t *src_rellist;
  CONST struct logrelation *lrel;
  unsigned long len,c,copynum;

  assert(src->t==LREL_INST);

  lrel = LRELN_INST(src)->ptr;
  src_rellist = LogRelSatRelList(lrel);
  if (!src_rellist) return;

  len = gl_length(src_rellist);
  for (c=1;c<=len;c++) {
    i = (struct Instance *)gl_fetch(src_rellist,c);
    copynum = GetTmpNum(i);
    ptr = (struct Instance *)gl_fetch(dest_list,copynum);
    gl_append_ptr(rellist,(VOIDPTR)ptr);
  }
}


static void BuildWhenVarList(CONST struct Instance *src,
			     struct gl_list_t *dest_list,
			     struct gl_list_t *bvarlist)
{
  struct Instance *i,*ptr,*dest;
  struct gl_list_t *src_bvarlist;
  unsigned long len,c,copynum;

  assert(src->t==WHEN_INST);
  copynum = GetTmpNum(src);
  dest = (struct Instance *)gl_fetch(dest_list,copynum);

  src_bvarlist = GetInstanceWhenVars(src);
  if (!src_bvarlist) return;
  len = gl_length(src_bvarlist);

  for (c=1;c<=len;c++) {
    i = (struct Instance *)gl_fetch(src_bvarlist,c);
    copynum = GetTmpNum(i);
    ptr = (struct Instance *)gl_fetch(dest_list,copynum);
    gl_append_ptr(bvarlist,(VOIDPTR)ptr);
  }
  W_INST(dest)->bvar = CopyWhenBVarList(dest,bvarlist);
}


static
struct gl_list_t *BuildWhenCasesRefList(struct Instance *dest,
                                        struct gl_list_t *srcref_list,
			                struct gl_list_t *dest_list)
{
  struct Instance *i,*ptr;
  struct gl_list_t *destref_list;
  unsigned long ref,lref,copynum;

  lref = gl_length(srcref_list);
  destref_list = gl_create(lref);
  for (ref=1;ref<=lref;lref++) {
    i = (struct Instance *)gl_fetch(srcref_list,ref);
    copynum = GetTmpNum(i);
    ptr = (struct Instance *)gl_fetch(dest_list,copynum);
    gl_append_ptr(destref_list,(VOIDPTR)ptr);
    AddWhen(ptr,dest);
  }
  return destref_list;
}


static void BuildWhenCasesList(CONST struct Instance *src,
			      struct gl_list_t *dest_list)
{
  struct Instance *dest;
  struct gl_list_t *src_caselist,*srcref_list;
  struct gl_list_t *destref_list,*caselist;
  struct Case *src_case,*dest_case;
  unsigned long len,c,copynum;

  assert(src->t==WHEN_INST);
  copynum = GetTmpNum(src);
  dest = (struct Instance *)gl_fetch(dest_list,copynum);

  src_caselist = GetInstanceWhenCases(src);
  if (!src_caselist) return;
  len = gl_length(src_caselist);
  caselist = gl_create(len);

  for (c=1;c<=len;c++) {
    src_case = (struct Case *)gl_fetch(src_caselist,c);
    dest_case = CreateCase(GetCaseValues(src_case),NULL);
    srcref_list = GetCaseReferences(src_case);
    destref_list = BuildWhenCasesRefList(dest,srcref_list,dest_list);
    SetCaseReferences(dest_case,destref_list);
    gl_append_ptr(caselist,(VOIDPTR)dest_case);
  }
  W_INST(dest)->cases = caselist;
}


static
void CopyRelationStructures(struct gl_list_t *src_list,
			    struct gl_list_t *dest_list,
			    enum Copy_enum copy_relns)
{
  CONST struct Instance *src;
  struct Instance *dest;
  struct gl_list_t *scratch;
  struct relation *rel = NULL;
  enum Expr_enum type;
  unsigned long len,c,copynum;

  len = gl_length(src_list);
  scratch = gl_create(100L);

  for (c=1;c<=len;c++) {
    src = (CONST struct Instance *)gl_fetch(src_list,c);
    if (src->t!=REL_INST)
      continue;
    BuildRelationVarList(src,dest_list,scratch);
    copynum = GetTmpNum(src);
    dest = (struct Instance *)gl_fetch(dest_list,copynum);
    switch (copy_relns) {
    case c_reference:
      rel = CopyRelationByReference(src,dest,scratch);
      break;
    case c_tomodify:
      rel = CopyRelationToModify(src,dest,scratch);
      break;
    default:
      ASC_PANIC("Invalid type in CopyRelationStructures\n");
      exit(2);
    }
    type = GetInstanceRelationType(src);
    SetInstanceRelation(dest,rel,type);
    gl_reset(scratch);
  }
  gl_destroy(scratch);
}


static
void CopyLogRelStructures(struct gl_list_t *src_list,
			    struct gl_list_t *dest_list,
			    enum Copy_enum copy_lrelns)
{
  CONST struct Instance *src;
  struct Instance *dest;
  struct gl_list_t *varlist;
  struct gl_list_t *rellist;
  struct logrelation *lrel;
  unsigned long len,c,copynum;

  len = gl_length(src_list);
  varlist = gl_create(100L);
  rellist = gl_create(100L);

  for (c=1;c<=len;c++) {
    src = (CONST struct Instance *)gl_fetch(src_list,c);
    if (src->t!=LREL_INST)
      continue;
    BuildLogRelBVarList(src,dest_list,varlist);
    BuildLogRelSatRelList(src,dest_list,rellist);
    copynum = GetTmpNum(src);
    dest = (struct Instance *)gl_fetch(dest_list,copynum);
    switch (copy_lrelns) {
    case c_reference:
      lrel = CopyLogRelByReference(src,dest,varlist,rellist);
      break;
    case c_tomodify:
      lrel = CopyLogRelToModify(src,dest,varlist,rellist);
      break;
    default:
      ASC_PANIC("Invalid type in CopyLogRelStructures\n"); /* NOTREACHED */
      lrel = NULL;
    }
    SetInstanceLogRel(dest,lrel);
    gl_reset(varlist);
    gl_reset(rellist);
  }
  gl_destroy(varlist);
  gl_destroy(rellist);
}


static void CopyWhenContents(struct gl_list_t *src_list,
		             struct gl_list_t *dest_list)
{
  CONST struct Instance *src;
  struct gl_list_t *scratch;
  unsigned long len,c;

  len = gl_length(src_list);
  scratch = gl_create(100L);

  for (c=1;c<=len;c++) {
    src = (CONST struct Instance *)gl_fetch(src_list,c);
    if (src->t!=WHEN_INST)
      continue;
    BuildWhenVarList(src,dest_list,scratch);
    gl_reset(scratch);
    BuildWhenCasesList(src,dest_list);
  }
  gl_destroy(scratch);
}

static
struct Instance *CopyDummy(CONST struct Instance *i)
{
  (void) i; /* intentional */
  FPRINTF(ASCERR,
    "CopyDummy called in error -- UNIVERSAL's shouldn't be copied.\n");
  return NULL;
}

/*
 *********************************************************************
 * This copies a node and not it's children unless it is an atom in which
 * case it copies the whole atom(including its children).  This does not
 * worry about connectivity or anything like that.
 *********************************************************************
 */
static struct Instance *CopyNode(CONST struct Instance *i)
{
  AssertMemory(i);
  switch (i->t) {
  case MODEL_INST:
    return CopyModel(i);
  case DUMMY_INST:
    return CopyDummy(i);
  case REAL_ATOM_INST:
  case REAL_CONSTANT_INST:
    return CopyReal(i);
  case BOOLEAN_ATOM_INST:
  case BOOLEAN_CONSTANT_INST:
    return CopyBoolean(i);
  case INTEGER_ATOM_INST:
  case INTEGER_CONSTANT_INST:
    return CopyInteger(i);
  case SET_ATOM_INST:
    return CopySetInst(i);
  case SYMBOL_ATOM_INST:
  case SYMBOL_CONSTANT_INST:
    return CopySymbolInst(i);
  case REL_INST:
    return CopyRelationInst(i);
  case LREL_INST:
    return CopyLogRelInst(i);
  case WHEN_INST:
    return CopyWhenInst(i);
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    return CopyArray(i);
  case REAL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SET_INST:
  case SYMBOL_INST:
    return NULL;
  default:
    ASC_PANIC("Incorrect instance type passed to CopyNode.\n");
    
  }
}

/* this goddamned thing is externed in instanceio rather than being headered */
void CollectNodes(struct Instance *i, struct gl_list_t *data)
{
  struct gl_list_t *list;
  list = data;

  switch (i->t) {
  case SIM_INST:		/* double check this one !! */
  case MODEL_INST:
  case REAL_CONSTANT_INST:
  case INTEGER_CONSTANT_INST:
  case BOOLEAN_CONSTANT_INST:
  case SYMBOL_CONSTANT_INST:
  case REAL_ATOM_INST:
  case BOOLEAN_ATOM_INST:
  case INTEGER_ATOM_INST:
  case SET_ATOM_INST:
  case SYMBOL_ATOM_INST:
  case REL_INST:
  case LREL_INST:
  case WHEN_INST:
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    gl_append_ptr(list,(VOIDPTR)i);
    SetTmpNum(i,gl_length(list));
    break;
  case REAL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SET_INST:
  case SYMBOL_INST:
    break;
  default:
    ASC_PANIC("Incorrect instance type passed to CollectNodes.\n");
  }
}

static unsigned long g_copy_numnodes = 0L;

static void ResetNodeCount(void)
{
  g_copy_numnodes = 0L;
}

static unsigned long GetNodeCount(void)
{
  return g_copy_numnodes;
}

static void CountNodes(struct Instance *i)
{
  switch (i->t) {
  case SIM_INST:		/* double check this one !! */
  case MODEL_INST:
  case REAL_CONSTANT_INST:
  case INTEGER_CONSTANT_INST:
  case BOOLEAN_CONSTANT_INST:
  case SYMBOL_CONSTANT_INST:
  case REAL_ATOM_INST:
  case BOOLEAN_ATOM_INST:
  case INTEGER_ATOM_INST:
  case SET_ATOM_INST:
  case SYMBOL_ATOM_INST:
  case REL_INST:
  case LREL_INST:
  case WHEN_INST:
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    g_copy_numnodes++;
    break;
  case REAL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SET_INST:
  case SYMBOL_INST:
    break;
  default:
    ASC_PANIC("Incorrect instance type passed to CountNodes.\n");
  }
}


/*
 * This function run downs the list of 'copyable' instances,
 * looking for Universal instances. It then visits the subtree
 * rooted at the universal instance, resetting the copynums to 0.
 */
static
void UnMarkUniversals(struct gl_list_t *list)
{
  unsigned long len,c;
  struct Instance *ptr;
  struct TypeDescription *desc;

  len = gl_length(list);
  for (c=1;c<=len;c++) {
    ptr = (struct Instance *)gl_fetch(list,c);
    desc = InstanceTypeDesc(ptr);
    if (GetUniversalFlag(desc)) {
      ZeroTmpNums((struct Instance *)ptr,0);
    }
  }
}

/*
 * This function run downs the list of 'copyable' instances,
 * which has been prepocessed by UnMarkUniversals. If the
 * copynum of the instance is non-zero, a new node (without any
 * connectivity) is created and appended to the new list. Otherwise
 * the instance is simply appended to the new list and its
 * copynum is reset. NOTE copynum should always be the index
 * of the instance on the list, except for when we temporarily
 * unmark them.
 */
static
void CopyNodes_ListVersion(struct gl_list_t *original_list,
			   struct gl_list_t *new_list)
{
  unsigned long len,c,copynum;
  struct Instance *src, *dest;

  len = gl_length(original_list);
  for (c=1;c<=len;c++) {
    src = (struct Instance *)gl_fetch(original_list,c);
    copynum = GetTmpNum(src);
    if (copynum!=0) {
      dest = CopyNode(src);
      gl_append_ptr(new_list,(VOIDPTR)dest);
      SetTmpNum(dest,c);
    } else{
      gl_append_ptr(new_list,(VOIDPTR)src);
      SetTmpNum(src,c);
    }
  }
}

/*
 * This function does the linking up of the new nodes, and
 * correctly takes care up of universal instances.
 * It runs down the old list, and foreach instance, processes its
 * children. This is done by wiring up the copy and the copy's
 * children, in the same way that the original is wired up to
 * its children.
 * NOTE: If the original and and the copy are looking
 * at the same instance, then this instance is a universal
 * instance or is part of the subtree of universal instance and
 * hence there is nothing to wire up for the copy. Remember that
 * universals and their subtrees are shared between the original and
 * any copies.
 */
static
void LinkNodes_ListVersion(struct gl_list_t *original_list,
			   struct gl_list_t *new_list)
{
  struct Instance *original, *child;
  struct Instance *copy, *ptr;
  struct Instance *next;
  unsigned long len, c, cc;
  unsigned long nch, cindex;

  len = gl_length(original_list);
  for (c=1;c<=len;c++) {
    copy = INST(gl_fetch(new_list,c));
    original = INST(gl_fetch(original_list,c));
    if (copy==original)	{
      /* universal or its child */
      continue;
    }
    assert(GetTmpNum(original)!=0);	/* copynum = 0 is invalid here */
    nch = NumberChildren(original);
    if ( ( InstanceKind(original) &
           (ICONS | IATOM | IFUND | IRELN | ILRELN | IWHEN)
         ) ==0 /* bit test for no/only fundamental children */
       ) {
      for (cc=1;cc<=nch;cc++) {
        /* only arrays/models should get here */
        child = InstanceChild(original,cc);
        cindex = GetTmpNum(child);
        assert(cindex!=0);/* subatomic children dont have copynums */
        /* link from child to parent. this could be smarter.
         * our handling of UNIVERSAL is poor.
         */
        ptr = INST(gl_fetch(new_list,cindex));
        if (SearchForParent(ptr,copy)==0) {
	  AddParent(ptr,copy);
        }
        /* link from parent to child */
        StoreChildPtr(copy,cc,ptr);
      }
    }

    /*
     * link ARE_ALIKE cliques. ARE_ALIKE cliques can run a
     * thread through instances that are *not* in the subtree
     * that is being copied. If the *entire* tree rooted at a
     * simulation is properly initialized to 0, then this code
     * will correctly link in only those instances in the subtree.
     *  --kaa
     * Note this is a bad assumption -- the universe must have
     * tmpnums of 0 when we have the ability to connect parts of
     * simulations across objects. This will need revisiting.
     * of course with parameterized types, ARE_ALIKE should be
     * so much less common that we won't care.
     *  -baa.
     */
    ptr = original;
    if (NextCliqueMember(ptr)!=ptr) { /* i is in a clique */
      do {
	ptr = NextCliqueMember(ptr);
	cindex = GetTmpNum(ptr);
	if (cindex) {
	  next = INST(gl_fetch(new_list,cindex));
	  SetNextCliqueMember(copy,next);
	  copy = next;
	}
      } while(ptr!=original);
    }
  }
}


#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
void CheckNewCopy(char *filename, CONST struct Instance *i)
{
  FILE *fp;
  int opened = 0;

  if (strcmp(filename,"ASCERR")==0)
    fp = ASCERR;
  else  if (strcmp(filename,"stdin")==0)
    fp = stdin;
  else{
    fp = fopen(filename,"w");
    if (!fp) {
      FPRINTF(ASCERR,"Error in opening CheckNewCopy file\n");
      return;
    }
    opened = 1;
  }

  InstanceStatistics(fp,i);
  if (opened) fclose(fp);
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */

/*
 * The new instance returned is the last one on the
 * list.
 */
static
struct Instance *CopyTree(CONST struct Instance *i)
{
  struct gl_list_t *orig_list;
  struct gl_list_t *new_list;
  struct Instance *result = NULL;
  unsigned long len;

  AssertMemory(i);
  ResetNodeCount();
  VisitInstanceTree((struct Instance *)i,CountNodes,1,0);
  len = GetNodeCount();

  new_list = gl_create(len);
  orig_list = gl_create(len);

  VisitInstanceTreeTwo((struct Instance *)i,(VisitTwoProc)CollectNodes,
		       1,0,(void *)orig_list);
  UnMarkUniversals(orig_list);
  CopyNodes_ListVersion(orig_list,new_list);
  LinkNodes_ListVersion(orig_list,new_list);
  CopyRelationStructures(orig_list,new_list,c_reference);
  CopyLogRelStructures(orig_list,new_list,c_reference);
  CopyWhenContents(orig_list,new_list);
  /*
   * reset copy numbers to zero. This is crucial for
   * subsequent calls to this function.
   */
  ZeroTmpNums((struct Instance *)i,1);
  ResetNodeCount();

  len = gl_length(new_list);
  if (len) {
    result = INST(gl_fetch(new_list,len));
  }
  gl_destroy(new_list);
  gl_destroy(orig_list);

  /* Do some debug checking
  CheckNewCopy("new.inst",result);
  CheckNewCopy("old.inst",i);
  */
  return result;
}

#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
struct Instance *DeepCopyTree(CONST struct Instance *i,
                              enum Copy_enum copy_relns,
                              enum Copy_enum copy_lrelns)
{
  struct gl_list_t *orig_list;
  struct gl_list_t *new_list;
  struct Instance *result = NULL;
  unsigned long len;

  AssertMemory(i);
  new_list = gl_create(1000L);
  orig_list = gl_create(1000L);

  VisitInstanceTreeTwo((struct Instance *)i,(VisitTwoProc)CollectNodes,
		       1,0,(void *)orig_list);
  UnMarkUniversals(orig_list);
  CopyNodes_ListVersion(orig_list,new_list);
  LinkNodes_ListVersion(orig_list,new_list);
  if (copy_relns!=c_none)
    CopyRelationStructures(orig_list,new_list,copy_relns);
  if (copy_lrelns!=c_none)
    CopyLogRelStructures(orig_list,new_list,copy_lrelns);
  CopyWhenContents(orig_list,new_list);
  /*
   * reset copy numbers to zero. This is crucial for
   * subsequent calls to this function.
   */
  ZeroTmpNums((struct Instance *)i,1);

  len = gl_length(new_list);
  if (len) {
    result = INST(gl_fetch(new_list,len));
  }
  gl_destroy(new_list);
  gl_destroy(orig_list);

  return result;
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */


struct Instance *CopyInstance(CONST struct Instance *i)
{
  AssertMemory(i);
  switch(i->t) {
  case SIM_INST:
    /* We are being strict and copying the SIM_INST */
    return CopyTree(i);
  case MODEL_INST:
    return CopyTree(i);
  case REAL_ATOM_INST:
  case REAL_CONSTANT_INST:
    return CopyReal(i);
  case BOOLEAN_ATOM_INST:
  case BOOLEAN_CONSTANT_INST:
    return CopyBoolean(i);
  case INTEGER_ATOM_INST:
  case INTEGER_CONSTANT_INST:
    return CopyInteger(i);
  case SET_ATOM_INST:
    return CopySetInst(i);
  case SYMBOL_ATOM_INST:
  case SYMBOL_CONSTANT_INST:
    return CopySymbolInst(i);
  case REL_INST:
    return CopyRelationInst(i);
  case LREL_INST:
    return CopyLogRelInst(i);
  case WHEN_INST:
    return CopyWhenInst(i);
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    return CopyTree(i);
  case REAL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SET_INST:
  case SYMBOL_INST:
    Asc_Panic(2, NULL,
              "CopyInstance may not be called on"
              " fundamental atomic instances.\n");
    
  default:
    ASC_PANIC("Unknown instance type passed to CopyInstance.\n");
    
  }
}
/************ end of copy stuff ****************/

