/*
 *  Ascend Instance Tree Link Implementation
 *  by Tom Epperly & Ben Allan
 *  9/3/89
 *  Version: $Revision: 1.15 $
 *  Version control file: $RCSfile: parentchild.c,v $
 *  Date last modified: $Date: 1998/06/11 15:28:33 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1996 Ben Allan
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
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <general/pool.h>
#include <general/list.h>
#include <general/dstring.h>

#include "bit.h"
#include "symtab.h"
#include "functype.h"
#include "expr_types.h"
#include "child.h"
#include "childinfo.h"
#include "type_desc.h"
#include "instance_name.h"
#include "instance_io.h"
#include "instmacro.h"
#include "instquery.h"
#include "atomvalue.h"
#include "atomsize.h"
#include "check.h"
#include "dump.h"
#include "prototype.h"
#include "pending.h"
#include "find.h"
#include "logical_relation.h"
#include "rel_blackbox.h"
#include "vlist.h"
#include "relation.h"
#include "logrelation.h"
#include "relation_util.h"
#include "logrel_util.h"
#include "rel_common.h"
#include "case.h"
#include "when_util.h"
#include "universal.h"
#include "instance_types.h"
#include "cmpfunc.h"
#include "childio.h"
#include "parentchild.h"

unsigned long NumberParents(CONST struct Instance *i)
{
  AssertMemory(i);
  assert(i != NULL);
  switch(i->t) {
    /* relations have only zero, one or two parents */
  case REL_INST:
    if (RELN_INST(i)->parent[1]) return 2;
    if (RELN_INST(i)->parent[0]) return 1;
    else return 0;
  case LREL_INST:
    if (LRELN_INST(i)->parent[1]) return 2;
    if (LRELN_INST(i)->parent[0]) return 1;
    else return 0;
  case WHEN_INST:
    if (W_INST(i)->parent[1]) return 2;
    if (W_INST(i)->parent[0]) return 1;
    else return 0;
  case SIM_INST:
    return 0;
    /* fundamental instances have only zero or one parent */
  case REAL_INST:
    return R_INST(i)->parent_offset ? 1 : 0;
  case INTEGER_INST:
    return I_INST(i)->parent_offset ? 1 : 0;
  case BOOLEAN_INST:
    return B_INST(i)->parent_offset ? 1 : 0;
  case SET_INST:
    return S_INST(i)->parent_offset ? 1 : 0;
  case SYMBOL_INST:
    return SYM_INST(i)->parent_offset ? 1 : 0;
  case MODEL_INST:
    return gl_length(MOD_INST(i)->parents);
  case REAL_CONSTANT_INST:
    return gl_length(RC_INST(i)->parents);
  case BOOLEAN_CONSTANT_INST:
    return gl_length(BC_INST(i)->parents);
  case INTEGER_CONSTANT_INST:
    return gl_length(IC_INST(i)->parents);
  case SYMBOL_CONSTANT_INST:
    return gl_length(SYMC_INST(i)->parents);
  case REAL_ATOM_INST:
    return gl_length(RA_INST(i)->parents);
  case BOOLEAN_ATOM_INST:
    return gl_length(BA_INST(i)->parents);
  case INTEGER_ATOM_INST:
    return gl_length(IA_INST(i)->parents);
  case SET_ATOM_INST:
    return gl_length(SA_INST(i)->parents);
  case SYMBOL_ATOM_INST:
    return gl_length(SYMA_INST(i)->parents);
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    return gl_length(ARY_INST(i)->parents);
  case DUMMY_INST:
    return 0;
  default:
    Asc_Panic(2, __FUNCTION__, "Invalid instance");
    
  }
}

struct Instance *InstanceParent(CONST struct Instance *i, unsigned long int n)
{
  AssertMemory(i);
  assert((i!=NULL) && (n>0) && (n<=NumberParents(i)));
  switch(i->t) {
  case MODEL_INST:
    return INST(gl_fetch(MOD_INST(i)->parents,n));
  case REAL_CONSTANT_INST:
    return INST(gl_fetch(RC_INST(i)->parents,n));
  case BOOLEAN_CONSTANT_INST:
    return INST(gl_fetch(BC_INST(i)->parents,n));
  case INTEGER_CONSTANT_INST:
    return INST(gl_fetch(IC_INST(i)->parents,n));
  case SYMBOL_CONSTANT_INST:
    return INST(gl_fetch(SYMC_INST(i)->parents,n));
  case REAL_ATOM_INST:
    return INST(gl_fetch(RA_INST(i)->parents,n));
  case BOOLEAN_ATOM_INST:
    return INST(gl_fetch(BA_INST(i)->parents,n));
  case INTEGER_ATOM_INST:
    return INST(gl_fetch(IA_INST(i)->parents,n));
  case SET_ATOM_INST:
    return INST(gl_fetch(SA_INST(i)->parents,n));
  case SYMBOL_ATOM_INST:
    return INST(gl_fetch(SYMA_INST(i)->parents,n));
  case REL_INST:
    return RELN_INST(i)->parent[n - 1];
  case LREL_INST:
    return LRELN_INST(i)->parent[n - 1];
  case WHEN_INST:
    return W_INST(i)->parent[n - 1];
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    return INST(gl_fetch(ARY_INST(i)->parents,n));
  case REAL_INST:
    return R_PARENT(i);
  case INTEGER_INST:
    return I_PARENT(i);
  case BOOLEAN_INST:
    return B_PARENT(i);
  case SET_INST:
    return S_PARENT(i);
  case SYMBOL_INST:
    return SYM_PARENT(i);
  case SIM_INST:
  case DUMMY_INST:
  default:
    Asc_Panic(2, __FUNCTION__, "Invalid instance");
    
  }
}

#define ParentSearch(list,parent)\
  gl_search(list,(char *)parent,(CmpFunc)CmpParents)

void DeleteParent(struct Instance *i, unsigned long int pos)
{
  assert((i != NULL)
         && (i->t==DUMMY_INST || ( pos>0 && pos<=NumberParents(i) )));
  AssertMemory(i);
  switch(i->t) {
  case MODEL_INST:
    gl_delete(MOD_INST(i)->parents,pos,0);
    break;
  case REAL_CONSTANT_INST:
    gl_delete(RC_INST(i)->parents,pos,0);
    break;
  case BOOLEAN_CONSTANT_INST:
    gl_delete(BC_INST(i)->parents,pos,0);
    break;
  case INTEGER_CONSTANT_INST:
    gl_delete(IC_INST(i)->parents,pos,0);
    break;
  case SYMBOL_CONSTANT_INST:
    gl_delete(SYMC_INST(i)->parents,pos,0);
    break;
  case REAL_ATOM_INST:
    gl_delete(RA_INST(i)->parents,pos,0);
    break;
  case BOOLEAN_ATOM_INST:
    gl_delete(BA_INST(i)->parents,pos,0);
    break;
  case INTEGER_ATOM_INST:
    gl_delete(IA_INST(i)->parents,pos,0);
    break;
  case SET_ATOM_INST:
    gl_delete(SA_INST(i)->parents,pos,0);
    break;
  case SYMBOL_ATOM_INST:
    gl_delete(SYMA_INST(i)->parents,pos,0);
    break;
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    gl_delete(ARY_INST(i)->parents,pos,0);
    break;
  case REL_INST:
    if (pos == 1){
      RELN_INST(i)->parent[0] = RELN_INST(i)->parent[1];
      RELN_INST(i)->parent[1] = NULL;
    }
    else{
      assert(pos==2);
      RELN_INST(i)->parent[1] = NULL;
    }
    break;
  case LREL_INST:
    if (pos == 1){
      LRELN_INST(i)->parent[0] = LRELN_INST(i)->parent[1];
      LRELN_INST(i)->parent[1] = NULL;
    }
    else{
      assert(pos==2);
      LRELN_INST(i)->parent[1] = NULL;
    }
    break;
  case WHEN_INST:
    if (pos == 1){
      W_INST(i)->parent[0] = W_INST(i)->parent[1];
      W_INST(i)->parent[1] = NULL;
    }
    else{
      assert(pos==2);
      W_INST(i)->parent[1] = NULL;
    }
    break;
  case REAL_INST:
    assert(pos==1);
    R_INST(i)->parent_offset = NULL;
    break;
  case INTEGER_INST:
    assert(pos==1);
    I_INST(i)->parent_offset = NULL;
    break;
  case BOOLEAN_INST:
    assert(pos==1);
    B_INST(i)->parent_offset = NULL;
    break;
  case SET_INST:
    assert(pos==1);
    S_INST(i)->parent_offset = NULL;
    break;
  case SYMBOL_INST:
    assert(pos==1);
    SYM_INST(i)->parent_offset = NULL;
  case DUMMY_INST:
    D_INST(i)->ref_count--;
    break;
  default:
    Asc_Panic(2, __FUNCTION__, "Invalid instance");
  }
}

unsigned long SearchForParent(CONST struct Instance *i,
			      CONST struct Instance *p)
{
  AssertMemory(i);
  switch(i->t) {
  case MODEL_INST:
    return ParentSearch(MOD_INST(i)->parents,p);
  case REAL_CONSTANT_INST:
    return ParentSearch(RC_INST(i)->parents,p);
  case BOOLEAN_CONSTANT_INST:
    return ParentSearch(BC_INST(i)->parents,p);
  case INTEGER_CONSTANT_INST:
    return ParentSearch(IC_INST(i)->parents,p);
  case SYMBOL_CONSTANT_INST:
    return ParentSearch(SYMC_INST(i)->parents,p);
  case REAL_ATOM_INST:
    return ParentSearch(RA_INST(i)->parents,p);
  case BOOLEAN_ATOM_INST:
    return ParentSearch(BA_INST(i)->parents,p);
  case INTEGER_ATOM_INST:
    return ParentSearch(IA_INST(i)->parents,p);
  case SET_ATOM_INST:
    return ParentSearch(SA_INST(i)->parents,p);
  case SYMBOL_ATOM_INST:
    return ParentSearch(SYMA_INST(i)->parents,p);
  case REL_INST:
    if (RELN_INST(i)->parent[0]==p) return 1;
    if (RELN_INST(i)->parent[1]==p) return 2;
    return 0;
  case LREL_INST:
    if (LRELN_INST(i)->parent[0]==p) return 1;
    if (LRELN_INST(i)->parent[1]==p) return 2;
    return 0;
  case WHEN_INST:
    if (W_INST(i)->parent[0]==p) return 1;
    if (W_INST(i)->parent[1]==p) return 2;
    return 0;
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    return ParentSearch(ARY_INST(i)->parents,p);
  case REAL_INST:
    return (R_PARENT(i)==p) ? 1 : 0;
  case INTEGER_INST:
    return (I_PARENT(i)==p) ? 1 : 0;
  case BOOLEAN_INST:
    return (B_PARENT(i)==p) ? 1 : 0;
  case SET_INST:
    return (S_PARENT(i)==p) ? 1 : 0;
  case SYMBOL_INST:
    return (SYM_PARENT(i)==p) ? 1 : 0;
  case DUMMY_INST:
    return 0;
  default:
    Asc_Panic(2, __FUNCTION__, "Invalid instance");
    
  }
}

/*********************************************************************\
  This function has a funny name. It means, what does my parent call
  me. Instances dont know their names and have to ask their parents.
\*********************************************************************/

struct InstanceName ParentsName(CONST struct Instance *p,
				CONST struct Instance *c)
{
  register unsigned long count=1, num_children;
  register struct Instance **child;
  register ChildListPtr clist;
  struct InstanceName result;
  register struct gl_list_t *arylist;
  register struct ArrayChild *arychild;
  AssertMemory(p);
  AssertMemory(c);
  assert((p != NULL)&&(c != NULL));
  switch(p->t) {
  case SIM_INST:	/* sims hold the name of their only child */
    SetInstanceNameType(result,StrName);
    SetInstanceNameStrPtr(result,SIM_INST(p)->name);
    return result;
  case MODEL_INST:
    child = MOD_CHILD(p,0);
    num_children = ChildListLen(clist = GetChildList(MOD_INST(p)->desc));
    break;
  case REAL_ATOM_INST:
    child = RA_CHILD(p,0);
    num_children = ChildListLen(clist = GetChildList(RA_INST(p)->desc));
    break;
  case BOOLEAN_ATOM_INST:
    child = BA_CHILD(p,0);
    num_children = ChildListLen(clist = GetChildList(BA_INST(p)->desc));
    break;
  case INTEGER_ATOM_INST:
    child = IA_CHILD(p,0);
    num_children = ChildListLen(clist = GetChildList(IA_INST(p)->desc));
    break;
  case SET_ATOM_INST:
    child = SA_CHILD(p,0);
    num_children = ChildListLen(clist = GetChildList(SA_INST(p)->desc));
    break;
  case SYMBOL_ATOM_INST:
    child = SYMA_CHILD(p,0);
    num_children = ChildListLen(clist = GetChildList(SYMA_INST(p)->desc));
    break;
  case REL_INST:
    child = REL_CHILD(p,0);
    num_children = ChildListLen(clist = GetChildList(RELN_INST(p)->desc));
    break;
  case LREL_INST:
    child = LREL_CHILD(p,0);
    num_children = ChildListLen(clist = GetChildList(LRELN_INST(p)->desc));
    break;
  case WHEN_INST:    /*  WHEN instances have no children */
    ASC_PANIC("ParentsName cannot be called on a WHEN instance.\n");
  case ARRAY_INT_INST:
    if (ARY_INST(p)->children!=NULL){
      SetInstanceNameType(result,IntArrayIndex);
      for (count=gl_length(arylist = ARY_INST(p)->children);count>0;count--) {
	arychild = (struct ArrayChild *)gl_fetch(arylist,count);
	AssertContainedMemory(arychild,sizeof(struct ArrayChild));
	if (arychild->inst == c) {
	  SetInstanceNameIntIndex(result,arychild->name.index);
	  return result;
	}
      }
    }
    else{
      SetInstanceNameType(result,StrName);
      SetInstanceNameStrPtr(result,AddSymbol(""));
    }
    return result;
  case ARRAY_ENUM_INST:
    if (ARY_INST(p)->children!=NULL){
      SetInstanceNameType(result,StrArrayIndex);
      for (count=gl_length(arylist = ARY_INST(p)->children);count>0;count--) {
	arychild = (struct ArrayChild *)gl_fetch(arylist,count);
	AssertContainedMemory(arychild,sizeof(struct ArrayChild));
	if (arychild->inst == c) {
	  SetInstanceNameStrIndex(result,arychild->name.str);
	  return result;
	}
      }
    }
    else{
      SetInstanceNameType(result,StrName);
      SetInstanceNameStrPtr(result,AddSymbol(""));
    }
    return result;
  case REAL_CONSTANT_INST:
  case INTEGER_CONSTANT_INST:
  case BOOLEAN_CONSTANT_INST:
  case SYMBOL_CONSTANT_INST:
  case REAL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SET_INST:
  case SYMBOL_INST:
  case DUMMY_INST:
    Asc_Panic(2, __FUNCTION__, "p is a fundamental or constant (invalid)");
    child = 0;
    clist = 0;
    num_children = 0;
  default:
    Asc_Panic(2, __FUNCTION__, "Invalid instance passed as parent");
    child = 0;
    clist = 0;
    num_children = 0;
  }
  SetInstanceNameType(result,StrName);
  while(num_children--) {
    if (*(child++) == c) {
      SetInstanceNameStrPtr(result,ChildStrPtr(clist,count));
      return result;
    }
    count++;
  }
  SetInstanceNameStrPtr(result,AddSymbol(""));
  return result;
}

void AddParent(struct Instance *i, struct Instance *p)
{
  assert((i!=NULL) && (p!=NULL));
  AssertMemory(i);
  AssertMemory(p);
  switch(i->t) {
  case MODEL_INST:
    gl_insert_sorted(MOD_INST(i)->parents,(char *)p,(CmpFunc)CmpParents);
    break;
  case REAL_CONSTANT_INST:
    gl_insert_sorted(RC_INST(i)->parents,(char *)p,(CmpFunc)CmpParents);
    break;
  case BOOLEAN_CONSTANT_INST:
    gl_insert_sorted(BC_INST(i)->parents,(char *)p,(CmpFunc)CmpParents);
    break;
  case INTEGER_CONSTANT_INST:
    gl_insert_sorted(IC_INST(i)->parents,(char *)p,(CmpFunc)CmpParents);
    break;
  case SYMBOL_CONSTANT_INST:
    gl_insert_sorted(SYMC_INST(i)->parents,(char *)p,(CmpFunc)CmpParents);
    break;
  case REAL_ATOM_INST:
    gl_insert_sorted(RA_INST(i)->parents,(char *)p,(CmpFunc)CmpParents);
    break;
  case BOOLEAN_ATOM_INST:
    gl_insert_sorted(BA_INST(i)->parents,(char *)p,(CmpFunc)CmpParents);
    break;
  case INTEGER_ATOM_INST:
    gl_insert_sorted(IA_INST(i)->parents,(char *)p,(CmpFunc)CmpParents);
    break;
  case SET_ATOM_INST:
    gl_insert_sorted(SA_INST(i)->parents,(char *)p,(CmpFunc)CmpParents);
    break;
  case SYMBOL_ATOM_INST:
    gl_insert_sorted(SYMA_INST(i)->parents,(char *)p,(CmpFunc)CmpParents);
    break;
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    gl_insert_sorted(ARY_INST(i)->parents,(char *)p,(CmpFunc)CmpParents);
    break;
  /* relations can only have one or two parents */
  case REL_INST:
    if (RELN_INST(i)->parent[0]==NULL)
      RELN_INST(i)->parent[0] = p;
    else if (RELN_INST(i)->parent[1]==NULL){
      if (p > RELN_INST(i)->parent[0])
	RELN_INST(i)->parent[1] = p;
      else{
	RELN_INST(i)->parent[1] = RELN_INST(i)->parent[0];
	RELN_INST(i)->parent[0] = p;
      }
    }
    else{
      Asc_Panic(2, NULL,
                "Relations can only have two parents.\n");/*NOTREACHED*/
    }
    break;
  case LREL_INST:
    if (LRELN_INST(i)->parent[0]==NULL)
      LRELN_INST(i)->parent[0] = p;
    else if (LRELN_INST(i)->parent[1]==NULL){
      if (p > LRELN_INST(i)->parent[0])
	LRELN_INST(i)->parent[1] = p;
      else{
	LRELN_INST(i)->parent[1] = LRELN_INST(i)->parent[0];
	LRELN_INST(i)->parent[0] = p;
      }
    }
    else{
      ASC_PANIC("Logical relations can only have two parents.\n");
    }
    break;
  /* WHENs can only have one or two parents */
  case WHEN_INST:
    if (W_INST(i)->parent[0]==NULL)
      W_INST(i)->parent[0] = p;
    else if (W_INST(i)->parent[1]==NULL){
      if (p > W_INST(i)->parent[0])
	W_INST(i)->parent[1] = p;
      else{
	W_INST(i)->parent[1] = W_INST(i)->parent[0];
	W_INST(i)->parent[0] = p;
      }
    }
    else{
      ASC_PANIC("WHENs can only have two parents.\n");/*NOTREACHED*/
    }
    break;
  /* fundamental atom instances can only have one parent */
  case REAL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SET_INST:
  case SYMBOL_INST:
    Asc_Panic(2, NULL,
              "It is illegal to call AddParent"
              " on a relation or funamental instance.\n");
  case DUMMY_INST:
    D_INST(i)->ref_count++;
    break;
  case SIM_INST:
  default:
    Asc_Panic(2, NULL,
              "Illegal argument passed as passed as i to AddParent.\n");
  }
}

unsigned long NumberChildren(CONST struct Instance *i)
{
  assert(i!=NULL);
  AssertMemory(i);
  if (IsChildlessInstance(i)) {
    return 0; /*no children of constants,when,funds*/
  }
  switch(i->t) {
  case SIM_INST:	/* only and always, 1 child */
    return 1;
  case MODEL_INST:
    return ChildListLen(GetChildList(MOD_INST(i)->desc));
  case REAL_ATOM_INST:
    return ChildListLen(GetChildList(RA_INST(i)->desc));
  case BOOLEAN_ATOM_INST:
    return ChildListLen(GetChildList(BA_INST(i)->desc));
  case INTEGER_ATOM_INST:
    return ChildListLen(GetChildList(IA_INST(i)->desc));
  case SET_ATOM_INST:
    return ChildListLen(GetChildList(SA_INST(i)->desc));
  case SYMBOL_ATOM_INST:
    return ChildListLen(GetChildList(SYMA_INST(i)->desc));
  case REL_INST:
    return ChildListLen(GetChildList(RELN_INST(i)->desc));
  case LREL_INST:
    return ChildListLen(GetChildList(LRELN_INST(i)->desc));
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    if (ARY_INST(i)->children!=NULL)
      return gl_length(ARY_INST(i)->children);
    break;
  case DUMMY_INST:
    return 0;
  default:
    ASC_PANIC("Invalid argument to NumberChildren.\n");
    /*NOTREACHED*/
  }
  return 0;
}

#define OLDICHILD 0
#if OLDICHILD
struct Instance *InstanceChild(CONST struct Instance *i,
				   unsigned long int n)
{
  register struct ArrayChild *ptr;
  register struct Instance **child;
  assert((i!=NULL) && (n>0) && (n<=NumberChildren(i)));
  AssertMemory(i);
  switch(i->t) {
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    if (ARY_INST(i)->children!=NULL){
      ptr = (struct ArrayChild *)gl_fetch(ARY_INST(i)->children,n);
      AssertContainedMemory(ptr,sizeof(struct ArrayChild));
      return ptr->inst;
    }
    else return NULL;
  case SIM_INST:
    child = SIM_CHILD(i,n-1);
    AssertContainedIn(i,child);
    return *child;
  case MODEL_INST:
    child = MOD_CHILD(i,n-1);
    AssertContainedIn(i,child);
    return *child;
  case REAL_ATOM_INST:
    child = RA_CHILD(i,n-1);
    AssertContainedIn(i,child);
    return *child;
  case BOOLEAN_ATOM_INST:
    child = BA_CHILD(i,n-1);
    AssertContainedIn(i,child);
    return *child;
  case INTEGER_ATOM_INST:
    child = IA_CHILD(i,n-1);
    AssertContainedIn(i,child);
    return *child;
  case SET_ATOM_INST:
    child = SA_CHILD(i,n-1);
    AssertContainedIn(i,child);
    return *child;
  case SYMBOL_ATOM_INST:
    child = SYMA_CHILD(i,n-1);
    AssertContainedIn(i,child);
    return *child;
  case REL_INST:
    child = REL_CHILD(i,n-1);
    AssertContainedIn(i,child);
   return *child;
  case LREL_INST:
    child = LREL_CHILD(i,n-1);
    AssertContainedIn(i,child);
    return *child;
  /* WHEN instances don't have any children */
  case WHEN_INST:
    Asc_Panic(2, "InstanceChild",
              "InstanceChild shouldn't be called on WHEN instances\n");
  /* fundamental atoms don't have any children */
  case REAL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SET_INST:
  case SYMBOL_INST:
  case REAL_CONSTANT_INST:
  case INTEGER_CONSTANT_INST:
  case BOOLEAN_CONSTANT_INST:
  case SYMBOL_CONSTANT_INST:
  case DUMMY_INST:
    Asc_Panic(2, NULL,
              "InstanceChild shouldn't be called"
              " on fundamental atoms, constants.\n");
  default:
    ASC_PANIC("Invalid argument to InstanceChild.\n");
  }
  return NULL;
}
#else /* OLDICHILD */
/*
 * we want to handle models, arrays, reals and rels very fast
 * the rest we will be slow about.
 */
struct Instance *InstanceChild(CONST struct Instance *i,
				   unsigned long int n)
{
#ifndef NDEBUG
  register struct ArrayChild *ptr;
  register struct Instance **child;
  assert((i!=NULL) && (n>0) && (n<=NumberChildren(i)));
  AssertMemory(i);
  if (i->t & (IERRINST | ICHILDLESS)) {
    Asc_Panic(2, NULL,
              "InstanceChild called with corrupt/childless instance.\n");
  }
#endif
  if (i->t & (IERRINST | ICHILDLESS)) return NULL;
  if (IsCompoundInstance(i)) { /* ary, model, sim */
    if (IsArrayInstance(i)) {
      if (ARY_INST(i)->children!=NULL){
#ifdef NDEBUG
        return ((struct ArrayChild *)gl_fetch(ARY_INST(i)->children,n))->inst;
#else
      ptr = (struct ArrayChild *)gl_fetch(ARY_INST(i)->children,n);
      AssertContainedMemory(ptr,sizeof(struct ArrayChild));
      return ptr->inst;
#endif
      } else {
        return NULL;
      }
    } else { /* model, sim */
      if ( i->t == MODEL_INST) {
#ifdef NDEBUG
        return *MOD_CHILD(i,n-1);
#else
        child = MOD_CHILD(i,n-1);
        AssertContainedIn(i,child);
        return *child;
#endif
      } else { /* sim */
#ifdef NDEBUG
        return *SIM_CHILD(i,n-1);
#else
        child = SIM_CHILD(i,n-1);
        AssertContainedIn(i,child);
        return *child;
#endif
      }
    }
#ifndef NDEBUG
    ASC_PANIC("InstanceChild found unexpected compound instance.\n");
#endif
  } else { /* not compound */
    if (i->t == REAL_ATOM_INST) {
#ifdef NDEBUG
      return *RA_CHILD(i,n-1);
#else
      child = RA_CHILD(i,n-1);
      AssertContainedIn(i,child);
      return *child;
#endif
    }
    if (i->t == REL_INST) {
#ifdef NDEBUG
      return *REL_CHILD(i,n-1);
#else
      child = REL_CHILD(i,n-1);
      AssertContainedIn(i,child);
      return *child;
#endif
    }
    if (i->t == LREL_INST) {
#ifdef NDEBUG
      return *LREL_CHILD(i,n-1);
#else
      child = LREL_CHILD(i,n-1);
      AssertContainedIn(i,child);
      return *child;
#endif
    }
    if (i->t == INTEGER_ATOM_INST) {
#ifdef NDEBUG
      return *IA_CHILD(i,n-1);
#else
      child = IA_CHILD(i,n-1);
      AssertContainedIn(i,child);
      return *child;
#endif
    }
    if (i->t == BOOLEAN_ATOM_INST) {
#ifdef NDEBUG
      return *BA_CHILD(i,n-1);
#else
      child = BA_CHILD(i,n-1);
      AssertContainedIn(i,child);
      return *child;
#endif
    }
    if (i->t == SET_ATOM_INST) {
#ifdef NDEBUG
      return *SA_CHILD(i,n-1);
#else
      child = SA_CHILD(i,n-1);
      AssertContainedIn(i,child);
      return *child;
#endif
    }
    if (i->t == SYMBOL_ATOM_INST) {
#ifdef NDEBUG
      return *SYMA_CHILD(i,n-1);
#else
      child = SYMA_CHILD(i,n-1);
      AssertContainedIn(i,child);
      return *child;
#endif
    }
  }
  /* constants, whens, fundamental atoms don't have any children */
  /* NOTREACHED */
  return NULL;
}
#endif /* OLDICHILD */

struct InstanceName ChildName(CONST struct Instance *i, unsigned long int n)
{
  register struct ArrayChild *arychild;
  struct InstanceName result;
  assert((i!=NULL)&&(n>0)&&(n<=NumberChildren(i)));
  AssertMemory(i);
  switch(i->t) {
  case SIM_INST:
    SetInstanceNameType(result,StrName);
    SetInstanceNameStrPtr(result,SIM_INST(i)->name);
    break;
  case MODEL_INST:
    SetInstanceNameType(result,StrName);
    SetInstanceNameStrPtr(result,
			  ChildStrPtr(GetChildList(MOD_INST(i)->desc),n));
    break;
  case REAL_ATOM_INST:
    SetInstanceNameType(result,StrName);
    SetInstanceNameStrPtr(result,
			  ChildStrPtr(GetChildList(RA_INST(i)->desc),n));
    break;
  case BOOLEAN_ATOM_INST:
    SetInstanceNameType(result,StrName);
    SetInstanceNameStrPtr(result,
			  ChildStrPtr(GetChildList(BA_INST(i)->desc),n));
    break;
  case INTEGER_ATOM_INST:
    SetInstanceNameType(result,StrName);
    SetInstanceNameStrPtr(result,
			  ChildStrPtr(GetChildList(IA_INST(i)->desc),n));
    break;
  case SET_ATOM_INST:
    SetInstanceNameType(result,StrName);
    SetInstanceNameStrPtr(result,
			  ChildStrPtr(GetChildList(SA_INST(i)->desc),n));
    break;
  case SYMBOL_ATOM_INST:
    SetInstanceNameType(result,StrName);
    SetInstanceNameStrPtr(result,
			  ChildStrPtr(GetChildList(SYMA_INST(i)->desc),n));
    break;
  case REL_INST:
    SetInstanceNameType(result,StrName);
    SetInstanceNameStrPtr(result,
			  ChildStrPtr(GetChildList(RELN_INST(i)->desc),n));
    break;
 case LREL_INST:
    SetInstanceNameType(result,StrName);
    SetInstanceNameStrPtr(result,
			  ChildStrPtr(GetChildList(LRELN_INST(i)->desc),n));
    break;
  case ARRAY_INT_INST:
    SetInstanceNameType(result,IntArrayIndex);
    arychild = (struct ArrayChild *)gl_fetch(ARY_INST(i)->children,n);
    SetInstanceNameIntIndex(result,arychild->name.index);
    break;
  case ARRAY_ENUM_INST:
    SetInstanceNameType(result,StrArrayIndex);
    arychild = (struct ArrayChild *)gl_fetch(ARY_INST(i)->children,n);
    SetInstanceNameStrIndex(result,arychild->name.str);
    break;
  /* WHEN instances don't have any children */
  case WHEN_INST:
    ASC_PANIC("ChildName shouldn't be called on WHEN instances\n");
  /* fundamental, constant instances don't have children */
  case REAL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SET_INST:
  case SYMBOL_INST:
  case REAL_CONSTANT_INST:
  case INTEGER_CONSTANT_INST:
  case BOOLEAN_CONSTANT_INST:
  case SYMBOL_CONSTANT_INST:
  case DUMMY_INST:
    Asc_Panic(2, NULL,
              "ChildName should not be called"
              " on fundamentaa/const instances.\n");
    exit(2);  /*NOTREACHED*/
  default:
    ASC_PANIC("Invalid argument passed to ChildName.\n");
    exit(2);  /*NOTREACHED*/
  }
  return result;
}

CONST struct TypeDescription *ChildRefines(CONST struct Instance *i,
                                           unsigned long n)
{
  struct TypeDescription *d;
  ChildListPtr cl;
  if (i==NULL) return NULL;
  if (n==0||n>NumberChildren(i)) return NULL;
  d = InstanceTypeDesc(i);
  cl = GetChildList(d);
  return ChildBaseTypePtr(cl,n);
}

CONST struct Statement *ChildDeclaration(CONST struct Instance *i,
                                         unsigned long n)
{
  struct TypeDescription *d;
  ChildListPtr cl;
  if (i==NULL) return NULL;
  if (n==0||n>NumberChildren(i)) return NULL;
  d = InstanceTypeDesc(i);
  cl = GetChildList(d);
  return ChildStatement(cl,n);
}

unsigned long ChildSearch(CONST struct Instance *i,
			  CONST struct InstanceName *name)
{
  struct ArrayChild arychild;
  assert((i!=NULL)&&(name!=NULL));
  AssertMemory(i);
  switch(i->t) {
  case SIM_INST:
    if (InstanceNameType(*name)==StrName)
      if (SIM_INST(i)->name == InstanceNameStr(*name))
	return 1;
    break;
  case MODEL_INST:
    if (InstanceNameType(*name)==StrName)
      return ChildPos(GetChildList(MOD_INST(i)->desc),InstanceNameStr(*name));
    break;
  case REAL_ATOM_INST:
    if (InstanceNameType(*name)==StrName)
      return ChildPos(GetChildList(RA_INST(i)->desc),InstanceNameStr(*name));
    break;
  case BOOLEAN_ATOM_INST:
    if (InstanceNameType(*name)==StrName)
      return ChildPos(GetChildList(BA_INST(i)->desc),InstanceNameStr(*name));
    break;
  case INTEGER_ATOM_INST:
    if (InstanceNameType(*name)==StrName)
      return ChildPos(GetChildList(IA_INST(i)->desc),InstanceNameStr(*name));
    break;
  case SET_ATOM_INST:
    if (InstanceNameType(*name)==StrName)
      return ChildPos(GetChildList(SA_INST(i)->desc),InstanceNameStr(*name));
    break;
  case SYMBOL_ATOM_INST:
    if (InstanceNameType(*name)==StrName)
      return ChildPos(GetChildList(SYMA_INST(i)->desc),InstanceNameStr(*name));
  case REL_INST:
    if (InstanceNameType(*name)==StrName)
      return ChildPos(GetChildList(RELN_INST(i)->desc),InstanceNameStr(*name));
    break;
  case LREL_INST:
    if (InstanceNameType(*name)==StrName)
      return ChildPos(GetChildList(LRELN_INST(i)->desc),
                      InstanceNameStr(*name));
    break;
  case ARRAY_INT_INST:
    if (ARY_INST(i)->children!=NULL){
      if (InstanceNameType(*name)==IntArrayIndex) {
	arychild.inst = NULL;
	arychild.name.index = InstanceIntIndex(*name);
	return gl_search(ARY_INST(i)->children,(char *)&arychild,
			 (CmpFunc)CmpIntIndex);
      }
    }
    break;
  case ARRAY_ENUM_INST:
    if (ARY_INST(i)->children!=NULL){
      if (InstanceNameType(*name)==StrArrayIndex) {
	arychild.inst = NULL;
	arychild.name.str = InstanceStrIndex(*name);
	return gl_search(ARY_INST(i)->children,(char *)&arychild,
			 (CmpFunc)CmpStrIndex);
      }
    }
    break;
  /* WHEN instances don't have any children */
  case WHEN_INST:
    FPRINTF(ASCERR,
      "ChildSearch shouldn't be called on WHEN instances\n");
    break;
  case REAL_CONSTANT_INST:
  case INTEGER_CONSTANT_INST:
  case BOOLEAN_CONSTANT_INST:
  case SYMBOL_CONSTANT_INST:
  case REAL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SET_INST:
  case SYMBOL_INST:
  case DUMMY_INST:
    break;
  default:
    ASC_PANIC("Invalid argument passed to ChildSearch.\n");
  }
  return 0;
}

unsigned long ChildIndex(CONST struct Instance *i,
			 CONST struct Instance *child)
{
  register unsigned long count=1, num_children=0;
  register struct Instance **childlist = NULL;
  register ChildListPtr clist;
  register struct gl_list_t *arylist;
  register struct ArrayChild *arychild;

  assert((i!=NULL) && (child!=NULL));
  AssertMemory(i);
  if (IsChildlessInstance(i)) return 0; /* no kids of fundies, whens, consts */
  AssertMemory(child);
  switch(i->t) {
  case SIM_INST:
    childlist = SIM_CHILD(i,0);
    if (*childlist==child) return 1;
    return 0;
  case MODEL_INST:
    childlist = MOD_CHILD(i,0);
    num_children = ChildListLen(clist = GetChildList(MOD_INST(i)->desc));
    break;
  case REAL_ATOM_INST:
    childlist = RA_CHILD(i,0);
    num_children = ChildListLen(clist = GetChildList(RA_INST(i)->desc));
    break;
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    if (ARY_INST(i)->children!=NULL){
      for(count=gl_length(arylist = ARY_INST(i)->children);count>0;count--) {
        arychild = (struct ArrayChild *)gl_fetch(arylist,count);
        if (arychild->inst == child) return count;
      }
    }
    break;
  case BOOLEAN_ATOM_INST:
    childlist = BA_CHILD(i,0);
    num_children = ChildListLen(clist = GetChildList(BA_INST(i)->desc));
    break;
  case INTEGER_ATOM_INST:
    childlist = IA_CHILD(i,0);
    num_children = ChildListLen(clist = GetChildList(IA_INST(i)->desc));
    break;
  case SET_ATOM_INST:
    childlist = SA_CHILD(i,0);
    num_children = ChildListLen(clist = GetChildList(SA_INST(i)->desc));
    break;
  case SYMBOL_ATOM_INST:
    childlist = SYMA_CHILD(i,0);
    num_children = ChildListLen(clist = GetChildList(SYMA_INST(i)->desc));
    break;
  case REL_INST:
    childlist = REL_CHILD(i,0);
    num_children = ChildListLen(clist = GetChildList(RELN_INST(i)->desc));
    break;
  case LREL_INST:
    childlist = LREL_CHILD(i,0);
    num_children = ChildListLen(clist = GetChildList(LRELN_INST(i)->desc));
    break;
  default:
    ASC_PANIC("Invalid argument to ChildIndex.\n");
    childlist = 0;
  }
  while(num_children--) {
    if (*(childlist++) == child) return count;
    count++;
  }
  return 0;
}

struct Instance *ChildByChar(CONST struct Instance *inst,symchar *name)
{
  unsigned long ndx;
  struct InstanceName childname;
  assert(name!=NULL);
  assert(AscFindSymbol(name)!=NULL);

  if (inst==NULL || name == NULL) {
    return NULL;
  }
  childname.t = StrName;
  childname.u.name = name;
  ndx = ChildSearch(inst,&childname);
  if( ndx )  {
    return InstanceChild(inst,ndx);
  } else {
    WriteChildMissing(ASCERR,"ChildByChar",name);
    return NULL;
  }
}

void StoreChildPtr(struct Instance *i, unsigned long int n,
		   struct Instance *child)
{
  register struct Instance **childptr;
  register struct ArrayChild *ptr;
  assert((i!=NULL)&&(n>0)&&(n<=NumberChildren(i)));
  AssertMemory(i);
  switch(i->t) {
  case SIM_INST:
    childptr = SIM_CHILD(i,0);		/* only one child at pos 0 */
    AssertContainedIn(i,childptr);
    *childptr = child;
    break;
  case MODEL_INST:
    childptr = MOD_CHILD(i,n-1);
    AssertContainedIn(i,childptr);
    *childptr = child;
    break;
  case REAL_ATOM_INST:
    childptr = RA_CHILD(i,n-1);
    AssertContainedIn(i,childptr);
    *childptr = child;
    break;
  case BOOLEAN_ATOM_INST:
    childptr = BA_CHILD(i,n-1);
    AssertContainedIn(i,childptr);
    *childptr = child;
    break;
  case INTEGER_ATOM_INST:
    childptr = IA_CHILD(i,n-1);
    AssertContainedIn(i,childptr);
    *childptr = child;
    break;
  case SET_ATOM_INST:
    childptr = SA_CHILD(i,n-1);
    AssertContainedIn(i,childptr);
    *childptr = child;
    break;
  case SYMBOL_ATOM_INST:
    childptr = SYMA_CHILD(i,n-1);
    AssertContainedIn(i,childptr);
    *childptr = child;
    break;
  case REL_INST:
    childptr = REL_CHILD(i,n-1);
    AssertContainedIn(i,childptr);
    *childptr = child;
    break;
  case LREL_INST:
    childptr = LREL_CHILD(i,n-1);
    AssertContainedIn(i,childptr);
    *childptr = child;
    break;
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    ptr = (struct ArrayChild *)gl_fetch(ARY_INST(i)->children,n);
    AssertContainedMemory(ptr,sizeof(struct ArrayChild));
    ptr->inst = child;
    break;
  /* WHEN instances don't have any children */
  case WHEN_INST:
    Asc_Panic(2, "StoreChildPtr",
              "StoreChildPtr shouldn't be called on WHEN instances\n");
  case REAL_CONSTANT_INST:
  case INTEGER_CONSTANT_INST:
  case BOOLEAN_CONSTANT_INST:
  case SYMBOL_CONSTANT_INST:
  case REAL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SET_INST:
  case SYMBOL_INST:
  case DUMMY_INST:
    Asc_Panic(2, NULL,
              "StoreChildPtr shouldn't be called"
              " on fundamental/constant atoms.\n");
  default:
    ASC_PANIC("Invalid argument to StoreChildPtr.\n");
  }
}

