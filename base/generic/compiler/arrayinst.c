/*
 *  Ascend Instance Array Implementation
 *  by Tom Epperly & Ben Allan
 *  9/3/89
 *  Version: $Revision: 1.23 $
 *  Version control file: $RCSfile: arrayinst.c,v $
 *  Date last modified: $Date: 1998/04/07 19:52:39 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Carnegie Mellon University
 *  Copyright (C) 1996 Benjamin Andrew Allan
 *  based on instance.c
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
#include <general/list.h>
#include <general/dstring.h>

#include "bit.h"
#include "symtab.h"


#include "functype.h"
#include "expr_types.h"
#include "instance_name.h"
#include "instance_io.h"
#include "check.h"
#include "dump.h"
#include "child.h"
#include "type_desc.h"
#include "prototype.h"
#include "vlist.h"
#include "pending.h"
#include "find.h"
#include "rel_blackbox.h"
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
#include "instance_types.h"
#include "instantiate.h"
/* new */
#include "arrayinst.h"
#include "atomvalue.h"
#include "atomsize.h"
#include "cmpfunc.h"
#include "copyinst.h"
#include "createinst.h"
#include "destroyinst.h"
#include "extinst.h"
#include "instmacro.h"
#include "instquery.h"
#include "linkinst.h"
#include "parentchild.h"
#include "refineinst.h"
#include "tmpnum.h"
/* needed, but didn't know it */
#include "forvars.h"
#include "setinstval.h"
#include "stattypes.h"
#include "statement.h"
#include "module.h"
#include "library.h"
#include "evaluate.h"
#include "statio.h"
#include "name.h"
#include "value_type.h"

#ifndef lint
static CONST char ArrayInstModuleID[] = "$Id: arrayinst.c,v 1.23 1998/04/07 19:52:39 ballan Exp $";
#endif

pool_store_t g_array_child_pool=NULL;
/*
 * A pool_store for all the array children ever simultaneously in use.
 */
#define IN_LEN 2
#if (SIZEOF_VOID_P == 8)
#define IN_WID 127
#else
#define IN_WID 255
#endif
/* retune if the size of ArrayChild changes dramatically */
#define IN_ELT_SIZE (sizeof(struct ArrayChild))
#define IN_MORE_ELTS 1
/*  Number of slots filled if more elements needed.
 *  So if the pool grows, it grows by IN_MORE_ELTS*IN_WID elements at a time.
 */
#define IN_MORE_BARS 200
/*  This is the number of pool bar slots to add during expansion.
 *  not all the slots will be filled immediately.
 */

void InitInstanceNanny(void)
{
  if (g_array_child_pool != NULL ) {
    ASC_PANIC("ERROR: InitInstanceNanny called twice.\n");
  }
  g_array_child_pool =
    pool_create_store(IN_LEN, IN_WID, IN_ELT_SIZE, IN_MORE_ELTS, IN_MORE_BARS);
  if (g_array_child_pool == NULL) {
    ASC_PANIC("ERROR: InitInstanceNanny unable to allocate pool.\n");
  }
}

void DestroyInstanceNanny(void)
{
  assert(g_array_child_pool!=NULL);
  pool_destroy_store(g_array_child_pool);
  g_array_child_pool = NULL;
}

void ReportInstanceNanny(FILE *f)
{
  assert(g_array_child_pool!=NULL);
  FPRINTF(f,"InstanceNanny ");
  pool_print_store(f,g_array_child_pool,0);
}

static
void ApplyToLeaves(struct Instance *i, AVProc func, int depth)
{
  unsigned long c,len;
  struct Instance *ch;
  if (i == NULL)  {
    return;
  }
  len = NumberChildren(i);
  if (depth == 1) {
    for (c = 1; c <= len; c++) {
      ch = InstanceChild(i,c);
      if (ch != NULL) {
        (*func)(ch);
      }
    }
  } else {
    depth--;
    for (c = 1; c <= len; c++) {
      ch = InstanceChild(i,c);
      if (ch != NULL) {
        ApplyToLeaves(ch,func,depth);
      }
    }
  }
}
/*
 * It may happen that there are no leaves if the array is
 * defined over a NULL set in some portion.  So for example if 
 * c[8][9] IS_A real; b[1..2][4..5] ALIASES c;
 * then the instance b has 4 subscripts to the end user,
 * but the 'leaves' of b which we don't want to
 * count as parents of elements c[i] are b[i][j].
 * Go NumberofDereferences(ch) and delete that contribution to
 * the number of parents counts.
 */
void ArrayVisitLocalLeaves(struct Instance *i, AVProc func)
{
  register struct ArrayInstance *ary;
  int depth;
  AssertMemory(i);
  ary = ARY_INST(i);
  if ((ary->t==ARRAY_INT_INST)||(ary->t==ARRAY_ENUM_INST)) {
    depth = (int)gl_length(GetArrayIndexList(ary->desc))-ary->indirected;
  } else {
    Asc_Panic(2, "ArrayVisitLocalLeaves",
              "Incorrect instance type (nonarray) given.\n");
    exit(2);/* NOT REACHED: Panic Exits--Needed to keep gcc from whining */
  }
  ApplyToLeaves(i,func,depth);
}

struct gl_list_t *CollectArrayInstances(CONST struct Instance *i,
                                        struct gl_list_t *result)
{
  unsigned long c, len;
  struct gl_list_t *clist;
  struct ArrayChild *ch;

  assert(i != NULL && IsArrayInstance(i) != 0);
  if (result == NULL) {
    result = gl_create(10L);
  }
  assert(result != NULL);
  clist = ARY_INST(i)->children;
  if (clist==NULL) {
    return result;
  }
  len = gl_length(clist);
  for (c = 1; c <= len; c++) {
    ch = (struct ArrayChild *)gl_fetch(clist,c);
    if (ch != NULL) {
      if (IsArrayInstance(ch->inst)==0) {
        /* found something not an array */
        gl_append_ptr(result,ch->inst);
      } else {
        /* found another array layer */
        CollectArrayInstances(ch->inst,result);
      }
    }
  }
  return result;
}

int RectangleArrayExpanded(CONST struct Instance *i)
{
  register struct ArrayInstance *ary;
  register struct ArrayChild *ptr;
  register unsigned long number;
  AssertMemory(i);
  ary = ARY_INST(i);
  if ((ary->t==ARRAY_INT_INST)||(ary->t==ARRAY_ENUM_INST)) {
    number = gl_length(GetArrayIndexList(ary->desc)) - ary->indirected;
    while (number-- > 0) {
      if (ary->children==NULL) return 0;
      if (gl_length(ary->children)==0) return 1;
      ptr = (struct ArrayChild *)gl_fetch(ary->children,1);
      AssertContainedMemory(ptr,sizeof(struct ArrayChild));
      if ((ptr==NULL)||(ptr->inst==NULL)){
	ASC_PANIC("Illegal array data structure.\n");
      }
      ary = ARY_INST(ptr->inst);
    }
    return 1;
  } else {
    Asc_Panic(2, NULL,
              "Incorrect instance type passed to RectangleArrayExpanded.\n");
    
  }
}

/* returns NULL for bad input OTHERWISE a set of indices from the array
 * ai's immediate child list.
 * if array is not expanded, returns NULL.
 */
static
struct set_t *ExtractIndices(CONST struct Instance *ai)
{
  struct set_t *result;
  unsigned long c,len;
  struct ArrayChild *ptr;
  struct ArrayInstance *ary;

  result = CreateEmptySet();
  ary = ARY_INST(ai);
  switch(InstanceKind(ai)) {
  case ARRAY_INT_INST:
    if (ary->children==NULL) {
      DestroySet(result);
      return NULL;
    }
    len = gl_length(ary->children);
    for (c = 1; c <= len; c++) {
      ptr = (struct ArrayChild *)gl_fetch(ary->children,c);
      if ((ptr==NULL)||(ptr->inst==NULL)){
	FPRINTF(ASCERR,"Illegal array data structure in ExtractIndices.\n");
        DestroySet(result);
        return NULL;
      }
      InsertInteger(result,ptr->name.index);
    }
    return result;
  case ARRAY_ENUM_INST:
    if (ary->children==NULL) {
      DestroySet(result);
      return NULL;
    }
    len = gl_length(ary->children);
    for (c = 1; c <= len; c++) {
      ptr = (struct ArrayChild *)gl_fetch(ary->children,c);
      if ((ptr==NULL)||(ptr->inst==NULL)){
	FPRINTF(ASCERR,"Illegal array data structure in ExtractIndices.\n");
        DestroySet(result);
        return NULL;
      }
      InsertString(result,ptr->name.str);
    }
    return result;
  default:
    DestroySet(result);
    return NULL;
  }
}

/* return 1 if match,
 * 0 if not match,
 * and -1 if horribly mangled 
 * and -2 if can't tell yet.
 */
int RectangleSubscriptsMatch(CONST struct Instance *parent,
                             CONST struct Instance *ary,
                             CONST struct Name *setname)
{
  struct ArrayChild *ptr;
  struct value_t value,   /* computed from of the expanded name element */
                 setval,  /* value converted to set value for comparison */
                 indices, /* subscripts found in array, as a setvalue */
                 cmp;     /* boolean comparison result */
  CONST struct Set *setp;
  struct set_t *sptr;
  unsigned int really_ok = 0; /* reached matching empty array leaf */

  /* outside the while loop, all value_t should be empty */
  while (ary != NULL && setname != NULL) {
    if ( (ary->t!=ARRAY_INT_INST) && (ary->t!=ARRAY_ENUM_INST) ) {
      ary = NULL;
      break;
    }

    /* calculate set expected */
    assert(NameId(setname)==0);
    assert(GetEvaluationContext()==NULL);
    SetEvaluationContext(parent);
    setp = NameSetPtr(setname);
    value = EvaluateSet(setp,InstanceEvaluateName);
    SetEvaluationContext(NULL);

    /* check for sanity of expected set */
    switch(ValueKind(value)) {
    case list_value:
      setval = CreateSetFromList(value); /* make unique,sorted,and monotypic */
      DestroyValue(&value);
      switch(ValueKind(setval)) {
      case set_value:
        break;
      default:
        DestroyValue(&setval);
        return -1;
      }
      break;
    case error_value:
      switch(ErrorValue(value)){
      case name_unfound:
      case undefined_value:
        DestroyValue(&value);
        return -2; /* possibly pending name piece, or unexecuted assgn */
      default:
        FPRINTF(ASCERR,"Array index definition has incorrect type:\n");
        DestroyValue(&value);
        return -1;
      }
    default:
      FPRINTF(ASCERR,"Array instance has incorrect index value type.\n");
      break;
    }

    /* at this point value is cleared, and setval is ok */
    /* extract array indices and compare. */
    sptr = ExtractIndices(ary);
    if (sptr==NULL) {
      DestroyValue(&setval);
      return -1;
    }
    indices = CreateSetValue(sptr);
    cmp = EqualValues(indices,setval);
    DestroyValue(&indices); /* also cleans up sptr */
    DestroyValue(&setval);
    switch(ValueKind(cmp)) {
    case boolean_value:
      if (BooleanValue(cmp)!=0) {
        DestroyValue(&cmp);
        break; /* move on to next subscript */
      } else {
        DestroyValue(&cmp);
        return 0; /* value mismatch */
      }
    default: /* error in comparison value */
      DestroyValue(&cmp);
      return -1;
    }

    setname = NextName(setname);
    if (gl_length(ARY_INST(ary)->children)==0) {
      if (setname != NULL) {
        /* more subscripts than children to fill them. */
        return -1;
      } 
      /* else the including while will kick out because setname NULL,
       * then the if will pass because we force it to. it will
       * OTHERWISE fail because the IsArrayInstance would return 1.
       */
      /* apply force */
      ary = NULL;
      really_ok =1;
    } else {
      /* here's where the rectangle-ism comes in. needs recursion
       * to handle the sparse case.
       */
      ptr = (struct ArrayChild *)gl_fetch(ARY_INST(ary)->children,1);
      ary = ptr->inst;
    }
  }
  if ((ary!=NULL && IsArrayInstance(ary) == 0 && setname == NULL) ||
      really_ok) {
    /* only TRUE if at array leaf or emptyset defined array. */
    return 1;
  } else {
    /* either set or ary terminated early/late. */
    return -1;
  }
}


unsigned long NextToExpand(CONST struct Instance *i)
{
  register struct ArrayInstance *ary;
  register struct ArrayChild *ptr;
  register unsigned long number,c;
  AssertMemory(i);
  ary = ARY_INST(i);
  if ((ary->t==ARRAY_INT_INST)||(ary->t==ARRAY_ENUM_INST)) {
    c = number = gl_length(GetArrayIndexList(ary->desc))-ary->indirected;
    while (c-->0) {
      assert((ary->t==ARRAY_INT_INST)||(ary->t==ARRAY_ENUM_INST));
      if (ary->children==NULL) return number-c;
      if (gl_length(ary->children)==0) return 0;
      ptr = (struct ArrayChild *)gl_fetch(ary->children,1);
      AssertContainedMemory(ptr,sizeof(struct ArrayChild));
      if ((ptr==NULL)||(ptr->inst==NULL)){
	ASC_PANIC("Illegal array data structure.\n");
      }
      if (c) {
        ary = ARY_INST(ptr->inst);
      }
    }
    return 0;
  } else {
    ASC_PANIC("Incorrect instance type passed to NextToExpand.\n");
    
  }
}

unsigned long NumberofDereferences(CONST struct Instance *i)
{
  register struct ArrayInstance *ary;
  AssertMemory(i);
  ary = ARY_INST(i);
  if ((ary->t==ARRAY_INT_INST)||(ary->t==ARRAY_ENUM_INST)) {
    return gl_length(GetArrayIndexList(ary->desc))-ary->indirected;
  } else {
    Asc_Panic(2, "NumberofDereferences",
              "Incorrect instance type passed to NumberofDereferences.\n");
    exit(2);/* NOT REACHED: Panic Exits--Needed to keep gcc from whining */
  }
}

CONST struct Set *IndexSet(CONST struct Instance *i, unsigned long int num)
{
  struct IndexType *ptr;
  AssertMemory(i);
  if ((i->t==ARRAY_INT_INST)||(i->t==ARRAY_ENUM_INST)){
    ptr = (struct IndexType *)gl_fetch(GetArrayIndexList(ARY_INST(i)->desc),
				       num);
    AssertAllocatedMemory(ptr,sizeof(struct IndexType));
    return GetIndexSet(ptr);
  } else {
    ASC_PANIC("Incorrect instance type passed to IndexSet.\n");
    
  }
}

/*
 * We have made this function longer by now explicitly
 * requiring it to call ShortCutMakeUniversalInstance. This is
 * so that the Create* routines will *always* provide
 * a new copy of an instance. For models we do not add
 * the instance to the pending list if it is a universal
 * instance or if we copied a prototype,
 * OTHERWISE we do add to pending.
 *
 * This function makes the final object that is the element
 * of an array, whatever its type. Relation arrays are handled
 * elsewhere, however. If called with a multisubscript array,
 * it returns the next subscript layer of a rectangular array.
 *
 * We call this even in the case of aliases, because this is where
 * we get the indirection of dense arrays done right.
 * If rhsinst is not null, we assume we are in alias processing.
 */
static
struct Instance *CreateArrayChildInst(struct Instance *i,
                                      struct Instance *rhsinst,
                                      struct Instance *arginst)
{
  struct TypeDescription *def,*desc;
  struct Instance *inst;
  int isintset;
  struct gl_list_t *list;

  AssertMemory(i);
  desc = ARY_INST(i)->desc;
  AssertMemory(desc);
  list = GetArrayIndexList(desc);
  if ((ARY_INST(i)->indirected+1)>=gl_length(list)){
  /* make or copy new object */
    if (rhsinst==NULL) {
      /* create new instance */
      def = GetArrayBaseType(desc);
      isintset = GetArrayBaseIsInt(desc);
      switch(GetBaseType(def)){
      case model_type:
        inst = ShortCutMakeUniversalInstance(def);
        if (inst==NULL) {
	  inst = ShortCutProtoInstance(def);	/* check if prototype exists */
        }
        if (inst==NULL) {				/* have to make one */
	  inst = CreateModelInstance(def);
	  if (!GetUniversalFlag(def)||!InstanceInList(inst)) {
            /* see notes in instantiate.c */
            ConfigureInstFromArgs(inst,arginst);
            AddBelow(NULL,inst);
            /* add PENDING model */
          }
        }
        return inst;
      case real_type:
      case real_constant_type:
        inst = ShortCutMakeUniversalInstance(def);
        if (inst!=NULL)	return inst;
        return CreateRealInstance(def);
      case boolean_type:
      case boolean_constant_type:
        inst = ShortCutMakeUniversalInstance(def);
        if (inst!=NULL)	return inst;
        return CreateBooleanInstance(def);
      case integer_type:
      case integer_constant_type:
        inst = ShortCutMakeUniversalInstance(def);
        if (inst!=NULL)	return inst;
        return CreateIntegerInstance(def);
      case set_type:
        inst = ShortCutMakeUniversalInstance(def);
        if (inst!=NULL)	return inst;
        return CreateSetInstance(def,isintset);
      case symbol_type:
      case symbol_constant_type:
        inst = ShortCutMakeUniversalInstance(def);
        if (inst!=NULL)	return inst;
        return CreateSymbolInstance(def);
      case relation_type:
        /*
         * we dont know the reltype at this stage so
         * set to undefined. we also dont call shortcut
         * as relations cannot be universal by themselves.
         */
        if (GetArrayBaseIsRelation(desc)) {
		  return CreateRelationInstance(def,e_undefined);
        }
        /* fallthrough */
      case logrel_type:
        if (GetArrayBaseIsLogRel(desc)) {
          return CreateLogRelInstance(def);
        }
        /* fallthrough */
      case when_type:
        if (GetArrayBaseIsWhen(desc)) {
          return CreateWhenInstance(def);
        }
        /* fallthrough */
      default:
        FPRINTF(ASCERR,"Uggggh! CreateArrayChildInst.\n");
        break; /* oh boy did it... */
      }
      return NULL;
    } else {
      /* take the single rhs pointer of the ALIASES statement. */
      return rhsinst;
    }
  } else {
    CopyTypeDesc(desc);
    return CreateArrayInstance(desc,ARY_INST(i)->indirected+2);
  }
}

static
struct ArrayChild *FindRHSByInt(struct gl_list_t *rhslist, long aindex)
{
  unsigned long c,len;
  for (c=1, len = gl_length(rhslist); c <= len; c++) {
    if (CAC(gl_fetch(rhslist,c))->name.index == aindex) {
      return CAC(gl_fetch(rhslist,c));
    }
  }
  FPRINTF(ASCERR,"ALIASES-IS_A integer child has skipped town.\n");
  return NULL;
}

static
struct ArrayChild *FindRHSByString(struct gl_list_t *rhslist, symchar *s)
{
  unsigned long c,len;
  for (c=1, len = gl_length(rhslist); c <= len; c++) {
    if (CAC(gl_fetch(rhslist,c))->name.str == s ||
         CmpSymchar(CAC(gl_fetch(rhslist,c))->name.str,s)==0) {
      return CAC(gl_fetch(rhslist,c));
    }
  }
  FPRINTF(ASCERR,"ALIASES-IS_A symbol child has skipped town.\n");
  return NULL;
}

static
void ExpandIntegerSet(struct ArrayInstance *i, struct set_t *set,
                      struct Instance *rhsinst, struct Instance *arginst,
                      struct gl_list_t *rhslist)
{
  register unsigned long c,len;
  register struct ArrayChild *ptr, *rptr;
  AssertMemory(i);
  AssertMemory(set);
  assert(rhslist==NULL||rhsinst==NULL); /* one type of alias or other */
  if (i->t==ARRAY_INT_INST){
    len = Cardinality(set);
    i->children = gl_create(len);
    AssertMemory(i->children);
    for(c=1;c<=len;c++){
      ptr = MALLOCPOOLAC;
      ptr->name.index = FetchIntMember(set,c);
      if (rhslist != NULL) {
        rptr = FindRHSByInt(rhslist,ptr->name.index);
        assert(rptr != NULL);
        rhsinst = rptr->inst;
        assert(rhsinst != NULL);
      }
      ptr->inst = CreateArrayChildInst(INST(i),rhsinst,arginst);
      /* will return rhsinst or the next array layer in case of alias */
      /* will return new instance or the next array layer in case of IS_A */
      AssertContainedMemory(ptr,sizeof(struct ArrayChild));
      if (rhsinst==NULL || /* regular case, but what about UNIVERSAL? */
          (rhsinst!=NULL && /* alii */
           SearchForParent(ptr->inst,INST(i))==0)
         ) {
        AddParent(ptr->inst,INST(i));
      }
      gl_append_ptr(i->children,(VOIDPTR)ptr);
    }
    gl_sort(i->children,(CmpFunc)CmpIntIndex);
  } else {
    Asc_Panic(2, NULL,
              "Attempt to expand alias array with incorrect set type.\n");
  }
}

static
void ExpandStringSet(struct ArrayInstance *i, struct set_t *set,
                     struct Instance *rhsinst, struct Instance *arginst,
                     struct gl_list_t *rhslist)
{
  register unsigned long c,len;
  register struct ArrayChild *ptr, *rptr;
  AssertMemory(i);
  AssertMemory(set);
  assert(rhslist==NULL||rhsinst==NULL); /* one type of alias or other */
  if (i->t==ARRAY_ENUM_INST){
    len = Cardinality(set);
    i->children = gl_create(len);
    AssertMemory(i->children);
    for(c=1;c<=len;c++){
      ptr = MALLOCPOOLAC;
      ptr->name.str = FetchStrMember(set,c);
      if (rhslist != NULL) {
        rptr = FindRHSByString(rhslist,ptr->name.str);
        assert(rptr != NULL);
        rhsinst = rptr->inst;
        assert(rhsinst != NULL);
      }
      ptr->inst = CreateArrayChildInst(INST(i),rhsinst,arginst);
      AssertContainedMemory(ptr,sizeof(struct ArrayChild));
      if (rhsinst==NULL || /* regular case */
          (rhsinst!=NULL && /* alii */
           SearchForParent(ptr->inst,INST(i))==0)
         ) {
        AddParent(ptr->inst,INST(i));
      }
      gl_append_ptr(i->children,(VOIDPTR)ptr);
    }
    gl_sort(i->children,(CmpFunc)CmpStrIndex);
  } else {
    ASC_PANIC("Attempt to expand array with incorrect set type.\n");
  }
}

/*
 * The recursion is over the already expanded intermediate
 * array nodes until we reach one that is not yet expanded or
 * we reach the terminal subscript of the subscripts written
 * explicitly in the IS_A/ALIASES/ALIASES-IS_A.
 *
 * Does not recurse down into the subscripts of the alias
 * rhs instance in the case where those subscripts are incomplete
 * because the typedesc of the instance being expanded does
 * not include those.
 */
static
void RecursiveExpand(struct Instance *i, unsigned long int num,
		     struct set_t *set,struct Instance *rhsinst,
                     struct Instance *arginst, struct gl_list_t *rhslist)
{
  AssertMemory(i);
  AssertMemory(set);
  if ((i->t!=ARRAY_INT_INST)&&(i->t!=ARRAY_ENUM_INST)){
    ASC_PANIC("Incorrect array structure in RecursiveExpand.\n");
  }
  if ((--num)==0){		/* we're here -- start creating instances */
    if (ARY_INST(i)->children==NULL){
      switch(SetKind(set)){
      case empty_set: ARY_INST(i)->children = gl_create(0); break;
      case integer_set:
	ExpandIntegerSet(ARY_INST(i),set,rhsinst,arginst,rhslist);
	break;
      case string_set:
	ExpandStringSet(ARY_INST(i),set,rhsinst,arginst,rhslist);
	break;
      }
    } else {
      ASC_PANIC("Attempt to expand previously expanded array.\n");
    }
  } else {			/* not there yet recurse on each child */
    register unsigned long c,len;
    register struct ArrayChild *child;
    register struct ArrayInstance *ptr;
    ptr = ARY_INST(i);
    if (ptr->children==NULL){
      Asc_Panic(2, NULL,
                "Incorrect call to ExpandArray died in RecursiveExpand.\n");
    }
    AssertMemory(ptr->children);
    len = gl_length(ptr->children);
    for(c=1;c<=len;c++){
      child = (struct ArrayChild *)gl_fetch(ptr->children,c);
      AssertContainedMemory(child,sizeof(struct ArrayChild));
      RecursiveExpand(child->inst,num,set,rhsinst,arginst,rhslist);
    }
  }
}

void ExpandArray(struct Instance *i, unsigned long int num,
                 struct set_t *set, struct Instance *rhsinst,
                 struct Instance *arginst, struct gl_list_t *rhslist)
{
  if ((i->t==ARRAY_INT_INST)||(i->t==ARRAY_ENUM_INST)){
    assert((num >= 1)&&(num <= NumberofDereferences(i)));
    AssertMemory(i);
    AssertMemory(set);
    RecursiveExpand(i,num,set,rhsinst,arginst,rhslist);
  } else {
    ASC_PANIC("Incorrect instance type passed to ExpandArray.\n");
  }
}


static
struct ArrayChild *MakeNextInst(struct Instance *ary, long int v,
				symchar *sym, struct Instance *rhsinst,
                                struct Instance *arginst)
{
  struct ArrayChild *ptr;
  ptr = MALLOCPOOLAC;
  ptr->inst = CreateArrayChildInst(ary,rhsinst,arginst);
  if (rhsinst==NULL || /* regular case */
      (rhsinst != NULL && /* alii */
       SearchForParent(ptr->inst,INST(ary))==0)
     ) {
    AddParent(ptr->inst,INST(ary));
  }
  if(sym==NULL) {
    ptr->name.index = v;
  } else {
    ptr->name.str = sym;
  }
  return ptr;
}

struct Instance *FindOrAddIntChild(struct Instance *i, long int v,
                                   struct Instance *rhsinst,
                                   struct Instance *arginst)
{
  struct ArrayChild rec,*ptr;
  unsigned long pos;
  switch(i->t) {
  case ARRAY_INT_INST:
    if (ARY_INST(i)->children !=NULL){
      rec.name.index = v;
      rec.inst = NULL;
      pos = gl_search(ARY_INST(i)->children,(char *)&rec,(CmpFunc)CmpIntIndex);
      if (pos) {
	ptr = (struct ArrayChild *)gl_fetch(ARY_INST(i)->children,pos);
	return ptr->inst;
      }
    } else {
      ARY_INST(i)->children = gl_create(AVG_ARY_CHILDREN);
    }
    ptr = MakeNextInst(i,v,NULL,rhsinst,arginst);
    gl_insert_sorted(ARY_INST(i)->children,(char *)ptr,(CmpFunc)CmpIntIndex);
    return ptr->inst;
  case ARRAY_ENUM_INST:
    return NULL;
  default:
    ASC_PANIC("Wrong type passed to ForOrAddIntChild.\n");
    
  }
}

struct Instance *FindOrAddStrChild(struct Instance *i, symchar *sym,
                                   struct Instance *rhsinst,
                                   struct Instance *arginst)
{
  struct ArrayChild rec,*ptr;
  unsigned long pos;
  switch(i->t) {
  case ARRAY_ENUM_INST:
    if (ARY_INST(i)->children !=NULL){
      rec.name.str = sym;
      rec.inst = NULL;
      pos = gl_search(ARY_INST(i)->children,(char *)&rec,(CmpFunc)CmpStrIndex);
      if (pos){
	ptr = (struct ArrayChild *)gl_fetch(ARY_INST(i)->children,pos);
	return ptr->inst;
      }
    } else {
      ARY_INST(i)->children = gl_create(AVG_ARY_CHILDREN);
    }
    ptr = MakeNextInst(i,0,sym,rhsinst,arginst);
    gl_insert_sorted(ARY_INST(i)->children,ptr,(CmpFunc)CmpStrIndex);
    return ptr->inst;
  case ARRAY_INT_INST:
    return NULL;
  default:
    ASC_PANIC("Wrong type passed to ForOrAddStrChild.\n");
    
  }
}

static
int RealCmpArrayInsts(struct ArrayInstance *, struct ArrayInstance *);

/* This is not exported because it is not a strict comparator.
 * It's more or less a shallow type compatibility check only.
 * It expects a little sanity.
 */
static
int CmpArrayInstances(struct Instance *i1, struct Instance *i2)
{
  if (i1==i2) {
    return 0;
  }
  if (i1==NULL) {
    return -1;
  }
  if (i2==NULL) {
    return 1;
  }
  if (InstanceKind(INST(i1))!=InstanceKind(INST(i2))) {
    return 1;
  }
  if (IsArrayInstance(i1)) {
    return RealCmpArrayInsts(ARY_INST(i1),ARY_INST(i2));
  }
  if (IsConstantInstance(i1)) {
    return CmpAtomValues(i1,i2);
  }
  if (InstanceKind(i1)==MODEL_INST || IsAtomicInstance(i1)) {
    return ((InstanceTypeDesc(i1)!=InstanceTypeDesc(i2)) ? 1 : 0);
  }
  /* relations/whens/logrels have no decent comparisons */
  return 0;
}
/* recursive part, ultimately depth first, top down. */
static
int RealCmpArrayInsts(struct ArrayInstance *a1, struct ArrayInstance *a2)
{
  unsigned long c,len;
  struct gl_list_t *cl1, *cl2;
  register struct ArrayChild *ac1, *ac2;
  int cmp;

  if (a1==a2) {
    return 0;
  }
  if (a1==NULL) {
    return -1;
  }
  if (a2==NULL) {
    return 1;
  }
  if (InstanceKind(INST(a1))!=InstanceKind(INST(a2))) {
    return 1;
  }
  cl1 = a1->children;
  cl2 = a2->children;
  if (gl_length(cl1) != gl_length(cl2)) {
    return ((gl_length(cl1) > gl_length(cl2)) ? -1 : 1);
  }
  len = gl_length(cl1);
  if (a1->t == ARRAY_ENUM_INST) {
    for (c=1; c <= len; c++) {
      ac1 = CAC(gl_fetch(cl1,c));
      ac2 = CAC(gl_fetch(cl2,c));
      cmp = CmpStrIndex(ac1,ac2);
      if (cmp != 0) {
        return cmp;
      }
      cmp = CmpArrayInstances(ac1->inst, ac2->inst);
      if (cmp != 0) {
        return cmp;
      }
    }
  } else {
    for (c=1; c <= len; c++) {
      ac1 = CAC(gl_fetch(cl1,c));
      ac2 = CAC(gl_fetch(cl2,c));
      cmp = CmpIntIndex(ac1,ac2);
      if (cmp != 0) {
        return cmp;
      }
      cmp = CmpArrayInstances(ac1->inst, ac2->inst);
      if (cmp != 0) {
        return cmp;
      }
    }
  }
  return 0;
}

int CmpArrayInsts(struct Instance *a1, struct Instance *a2)
{
  if (a1==a2) {
    return 0;
  }
  if (a1==NULL) {
    return -1;
  }
  if (a2==NULL) {
    return 1;
  }
  if (GetArrayBaseType(ARY_INST(a1)->desc) !=
      GetArrayBaseType(ARY_INST(a2)->desc)) {
    return 1;
  }
  return RealCmpArrayInsts(ARY_INST(a1),ARY_INST(a2));
}
