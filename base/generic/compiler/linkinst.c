/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1996 Ben Allan
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly

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
	@file
	Ascend Instance Tree Link Management
*//*
	based in instance.c by Tom Epperly
	9/3/89
	Last in CVS: $Revision: 1.13 $ $Date: 1998/02/05 16:36:59 $ $Author: ballan $
*/

#include <stdarg.h>
#include <utilities/ascConfig.h>
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/dstring.h>

#include "symtab.h"
#include "bit.h"


#include "functype.h"
#include "expr_types.h"
#include "instance_enum.h"
#include "instance_name.h"
#include "instance_io.h"
#include "check.h"
#include "dump.h"
#include "child.h"
#include "type_desc.h"
#include "prototype.h"
#include "pending.h"
#include "find.h"
#include "relation_type.h"
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
#include "instance_types.h"
/* new */
#include "atomsize.h"
#include "atomvalue.h"
#include "cmpfunc.h"
#include "destroyinst.h"
#include "instmacro.h"
#include "instquery.h"
#include "linkinst.h"
#include "mathinst.h"
#include "parentchild.h"

#define PANIC_ILLEGAL_INSTANCE Asc_Panic(2, __FUNCTION__, "invalid instance type")

/** this gets used in interactive merge/refinement and destruction. */
void ChangeRelationPointers(struct Instance *rel, struct Instance *old,
			    struct Instance *new
){
  assert(rel!=NULL);
  assert(rel->t==REL_INST);
  AssertMemory(rel);
  if (RELN_INST(rel)->ptr!=NULL) {
    /* FIXME: ChangeRelationPointers needs verification. all rel types have the rel->varlist that needs repair, then each rel type has specifics to fix up. */
    switch (RELN_INST(rel)->type) {
    case e_token:
      ModifyTokenRelationPointers(rel,RELN_INST(rel)->ptr,old,new);
      return;
    case e_glassbox:
      ModifyGlassBoxRelPointers(rel,RELN_INST(rel)->ptr,old,new);
      return;
    case e_blackbox:
      CONSOLE_DEBUG("MODIFY BLACK BOX POINTERS");
      ModifyBlackBoxRelPointers(rel,RELN_INST(rel)->ptr,old,new);
      return;
    case e_undefined:
    default:
      PANIC_ILLEGAL_INSTANCE;
    }
  }
}

void ChangeLogRelPointers(struct Instance *lrel, struct Instance *old,
			  struct Instance *new
){
  struct gl_list_t *varlist,*rellist;
  struct Instance *inst;
  struct logrelation *logrel;

  logrel = LRELN_INST(lrel)->ptr;
  varlist = logrel->bvars;
  rellist = logrel->satrels;

  if (new == NULL) {
    inst = old;
  }
  else {
    inst = new;
  }
  assert(lrel->t==LREL_INST);
  AssertMemory(lrel);
  switch(inst->t){
    case BOOLEAN_ATOM_INST:
      if (logrel!=NULL){
        ModifyLogRelPointers(varlist,logrel,old,new);
      return;
      }
      break;
    case REL_INST:
    case LREL_INST:
      if (logrel!=NULL){
        ModifyLogRelPointers(rellist,logrel,old,new);
      return;
      }
      break;
    default:
      PANIC_ILLEGAL_INSTANCE;
  }
}

void ChangeWhenPointers(struct Instance *when, struct Instance *old,
			struct Instance *new
){
  struct gl_list_t *varlist,*caselist,*reflist;
  struct Instance *scratch;
  struct Case *cur_case;
  unsigned long c,len,pos;

  varlist = W_INST(when)->bvar;
  caselist = W_INST(when)->cases;
  len = gl_length(caselist);

  if (new == NULL) {
    scratch = old;
  }
  else {
    scratch = new;
  }
  assert(when->t==WHEN_INST);
  AssertMemory(when);
  switch(scratch->t){
    case BOOLEAN_ATOM_INST:
    case INTEGER_ATOM_INST:
    case SYMBOL_ATOM_INST:
    case BOOLEAN_CONSTANT_INST:
    case INTEGER_CONSTANT_INST:
    case SYMBOL_CONSTANT_INST:
      if (varlist!=NULL){
        ModifyWhenPointers(varlist,old,new);
        return;
      }
    case MODEL_INST:
    case WHEN_INST:
    case REL_INST:
    case LREL_INST:
      for(c=1;c<=len;c++) {
        cur_case = (struct Case *)gl_fetch(caselist,c);
        reflist = GetCaseReferences(cur_case);
        if (reflist != NULL) {
          if (0 != (pos = gl_search(reflist,old,(CmpFunc)CmpP)))
            ModifyWhenPointers(reflist,old,new);
	}
      }
    return;
    default:
      PANIC_ILLEGAL_INSTANCE;
  }
}

/**
	Tell parent to change pointers to oldchild to pointers to newchild.
*/
void ChangeParent(struct Instance *parent, struct Instance *oldchild,
		  struct Instance *newchild
){
  register unsigned long c,length;
  AssertMemory(parent);
  length = NumberChildren(parent);
  for(c=1;c<=length;c++){
    if (InstanceChild(parent,c)==oldchild) {
      StoreChildPtr(parent,c,newchild);
    }
  }
}

void ReDirectParents(struct Instance *old, struct Instance *new){
  register struct Instance *parent;
  register unsigned long index1,length;
  length = NumberParents(new);
  for(index1=1;index1<=length;index1++) {
    parent = InstanceParent(new,index1);
    ChangeParent(parent,old,new);
  }
}

void ReDirectChildren(struct Instance *old, struct Instance *new){
  register struct Instance *child;
  register unsigned long c,length,pos;
  length = NumberChildren(new);
  for(c=1;c<=length;c++){
    if ((child = InstanceChild(new,c))!=NULL){
      if ((pos = SearchForParent(child,old))!=0){
	DeleteParent(child,pos);
	AddParent(child,new);
      }
    }
  }
}


void ReorderChildrenPtrs(register struct Instance **c,
		register CONST ChildListPtr old, register CONST ChildListPtr new,
		register unsigned long int olen, register unsigned long int nlen
){
  register unsigned nzero;
  if (olen==0) return;
  nzero = nlen-olen;
  while (nlen > 0) {
    if (ChildStrPtr(new,nlen) == ChildStrPtr(old,olen)) {
      /* move pointer at olen-1 to nlen-1 and put NULL in olen-1 */
      c[--nlen] = c[--olen];
      c[olen] = NULL;
      if (olen==0)return;
    } else {
      nlen--;
      if (--nzero==0) return;
    }
  }
}

/**
	Remove old from the clique put new in its place.
*/
void FixCliques(struct Instance *old, struct Instance *new){
  register struct Instance *ptr,*next;
  ptr = new;
  /*  SetNextCliqueMember(ptr,NextCliqueMember(old)); not needed */
  while((next=NextCliqueMember(ptr))!=old)
    ptr = next;
  SetNextCliqueMember(ptr,new);
  /*  SetNextCliqueMember(old,old); illegal since old is no longer allocated */
}


/* this is called to tell relations about a change in variable location
 * e.g. If two atoms are merged, point all the relations that know about
 * ATOM old to ATOM new. 
 */
void FixRelations(struct RealAtomInstance *old, struct RealAtomInstance *new)
{
  register unsigned long c,len;
  AssertMemory(old);
  AssertMemory(new);
  if ((new->relations==NULL)||(new->relations==old->relations)){
	/* new had no relations or new has the identical relation list */
    new->relations = old->relations;
    if ((len=RelationsCount(INST(new)))>0){
      for(c=1;c<=len;c++) {
	ChangeRelationPointers(RelationsForAtom(INST(new),c),
			       INST(old),INST(new));
      }
    }
  } else {
    len=RelationsCount(INST(old));
    if (len>0) {
      for(c=1;c<=len;c++){
	ChangeRelationPointers(RelationsForAtom(INST(old),c),
			       INST(old),INST(new));
	AddRelation(INST(new),RelationsForAtom(INST(old),c));
      }
    }
    if (old->relations) {
      gl_destroy(old->relations);
    }
  }
  old->relations=NULL;
}


static
void FixLogRelationsIf(struct Instance *old, struct Instance *new){
  register unsigned long c,len;
  if ((len=LogRelationsCount(new))>0){
    for(c=1;c<=len;c++) {
      ChangeLogRelPointers(LogRelationsForInstance(new,c),old,new);
    }
  }
}

static
void FixLogRelationsElse(struct Instance *old, struct Instance *new){
  register unsigned long c,len;
  if ((len=LogRelationsCount(INST(old)))>0){
    for(c=1;c<=len;c++){
      ChangeLogRelPointers(LogRelationsForInstance(old,c),old,new);
      AddLogRel(new,LogRelationsForInstance(old,c));
    }
  }
}


void FixLogRelations(struct Instance *old,
                     struct Instance *new
){
  switch(old->t){
    case BOOLEAN_ATOM_INST:
      AssertMemory(BA_INST(old));
      AssertMemory(BA_INST(new));
      if((BA_INST(new)->logrelations==NULL) ||
         (BA_INST(new)->logrelations==BA_INST(old)->logrelations))  {
        BA_INST(new)->logrelations = BA_INST(old)->logrelations;
        FixLogRelationsIf(old,new);
      }
      else {
        FixLogRelationsElse(old,new);
        if (BA_INST(old)->logrelations) gl_destroy(BA_INST(old)->logrelations);
      }
      BA_INST(old)->logrelations=NULL;
      break;
    case REL_INST:
      AssertMemory(RELN_INST(old));
      AssertMemory(RELN_INST(new));
      if((RELN_INST(new)->logrels==NULL) ||
         (RELN_INST(new)->logrels==RELN_INST(old)->logrels))  {
        RELN_INST(new)->logrels = RELN_INST(old)->logrels;
        FixLogRelationsIf(old,new);
      }
      else {
        FixLogRelationsElse(old,new);
        if (RELN_INST(old)->logrels) gl_destroy(RELN_INST(old)->logrels);
      }
      RELN_INST(old)->logrels=NULL;
      break;
    case LREL_INST:
      AssertMemory(LRELN_INST(old));
      AssertMemory(LRELN_INST(new));
      if((LRELN_INST(new)->logrels==NULL) ||
         (LRELN_INST(new)->logrels==LRELN_INST(old)->logrels))  {
        LRELN_INST(new)->logrels = LRELN_INST(old)->logrels;
        FixLogRelationsIf(old,new);
      }
      else {
        FixLogRelationsElse(old,new);
        if (LRELN_INST(old)->logrels) gl_destroy(LRELN_INST(old)->logrels);
      }
      LRELN_INST(old)->logrels=NULL;
      break;
    default:
      PANIC_ILLEGAL_INSTANCE;
  }
}

static
void FixWhensIf(struct Instance *old, struct Instance *new){
  register unsigned long c,len;
  if ((len=WhensCount(new))>0){
    for(c=1;c<=len;c++) {
      ChangeWhenPointers(WhensForInstance(new,c),old,new);
    }
  }
}

static
void FixWhensElse(struct Instance *old, struct Instance *new){
  register unsigned long c,len;
  if ((len=WhensCount(INST(old)))>0){
    for(c=1;c<=len;c++){
      ChangeWhenPointers(WhensForInstance(old,c),old,new);
      AddWhen(new,WhensForInstance(old,c));
    }
  }
}

void FixWhens(struct Instance *old, struct Instance *new){
  switch(old->t){
    case BOOLEAN_ATOM_INST:
      AssertMemory(BA_INST(old));
      AssertMemory(BA_INST(new));
      if((BA_INST(new)->whens==NULL) ||
        (BA_INST(new)->whens==BA_INST(old)->whens))  {
        BA_INST(new)->whens = BA_INST(old)->whens;
        FixWhensIf(old,new);
      }
      else{
        FixWhensElse(old,new);
        if (BA_INST(old)->whens) gl_destroy(BA_INST(old)->whens);
      }
      BA_INST(old)->whens=NULL;
      break;
    case INTEGER_ATOM_INST:
      AssertMemory(IA_INST(old));
      AssertMemory(IA_INST(new));
      if((IA_INST(new)->whens==NULL) ||
        (IA_INST(new)->whens==IA_INST(old)->whens))  {
        IA_INST(new)->whens = IA_INST(old)->whens;
        FixWhensIf(old,new);
      }
      else{
        FixWhensElse(old,new);
        if (IA_INST(old)->whens) gl_destroy(IA_INST(old)->whens);
      }
      IA_INST(old)->whens=NULL;
      break;
    case SYMBOL_ATOM_INST:
      AssertMemory(SYMA_INST(old));
      AssertMemory(SYMA_INST(new));
      if((SYMA_INST(new)->whens==NULL) ||
        (SYMA_INST(new)->whens==SYMA_INST(old)->whens))  {
        SYMA_INST(new)->whens = SYMA_INST(old)->whens;
        FixWhensIf(old,new);
      }
      else{
        FixWhensElse(old,new);
        if (SYMA_INST(old)->whens) gl_destroy(SYMA_INST(old)->whens);
      }
      SYMA_INST(old)->whens=NULL;
      break;
    case BOOLEAN_CONSTANT_INST:
      AssertMemory(BC_INST(old));
      AssertMemory(BC_INST(new));
      if((BC_INST(new)->whens==NULL) ||
        (BC_INST(new)->whens==BC_INST(old)->whens))  {
        BC_INST(new)->whens = BC_INST(old)->whens;
        FixWhensIf(old,new);
      }
      else{
        FixWhensElse(old,new);
        if (BC_INST(old)->whens) gl_destroy(BC_INST(old)->whens);
      }
      BC_INST(old)->whens=NULL;
      break;
    case INTEGER_CONSTANT_INST:
      AssertMemory(IC_INST(old));
      AssertMemory(IC_INST(new));
      if((IC_INST(new)->whens==NULL) ||
        (IC_INST(new)->whens==IC_INST(old)->whens))  {
        IC_INST(new)->whens = IC_INST(old)->whens;
        FixWhensIf(old,new);
      }
      else{
        FixWhensElse(old,new);
        if (IC_INST(old)->whens) gl_destroy(IC_INST(old)->whens);
      }
      IC_INST(old)->whens=NULL;
      break;
    case SYMBOL_CONSTANT_INST:
      AssertMemory(SYMC_INST(old));
      AssertMemory(SYMC_INST(new));
      if((SYMC_INST(new)->whens==NULL) ||
        (SYMC_INST(new)->whens==SYMC_INST(old)->whens))  {
        SYMC_INST(new)->whens = SYMC_INST(old)->whens;
        FixWhensIf(old,new);
      }
      else{
        FixWhensElse(old,new);
        if (SYMC_INST(old)->whens) gl_destroy(SYMC_INST(old)->whens);
      }
      SYMC_INST(old)->whens=NULL;
      break;
    case MODEL_INST:
      AssertMemory(MOD_INST(old));
      AssertMemory(MOD_INST(new));
      if((MOD_INST(new)->whens==NULL) ||
        (MOD_INST(new)->whens==MOD_INST(old)->whens))  {
        MOD_INST(new)->whens = MOD_INST(old)->whens;
        FixWhensIf(old,new);
      }
      else{
        FixWhensElse(old,new);
        if (MOD_INST(old)->whens) gl_destroy(MOD_INST(old)->whens);
      }
      MOD_INST(old)->whens=NULL;
      break;
    case REL_INST:
      AssertMemory(RELN_INST(old));
      AssertMemory(RELN_INST(new));
      if((RELN_INST(new)->whens==NULL) ||
        (RELN_INST(new)->whens==RELN_INST(old)->whens))  {
        RELN_INST(new)->whens = RELN_INST(old)->whens;
        FixWhensIf(old,new);
      }
      else{
        FixWhensElse(old,new);
        if (RELN_INST(old)->whens) gl_destroy(RELN_INST(old)->whens);
      }
      RELN_INST(old)->whens=NULL;
      break;
    case LREL_INST:
      AssertMemory(LRELN_INST(old));
      AssertMemory(LRELN_INST(new));
      if((LRELN_INST(new)->whens==NULL) ||
        (LRELN_INST(new)->whens==LRELN_INST(old)->whens))  {
        LRELN_INST(new)->whens = LRELN_INST(old)->whens;
        FixWhensIf(old,new);
      }
      else{
        FixWhensElse(old,new);
        if (LRELN_INST(old)->whens) gl_destroy(LRELN_INST(old)->whens);
      }
      LRELN_INST(old)->whens=NULL;
      break;
    case WHEN_INST:
      AssertMemory(W_INST(old));
      AssertMemory(W_INST(new));
      if((W_INST(new)->whens==NULL) ||
        (W_INST(new)->whens==W_INST(old)->whens))  {
        W_INST(new)->whens = W_INST(old)->whens;
        FixWhensIf(old,new);
      }
      else{
        FixWhensElse(old,new);
        if (W_INST(old)->whens) gl_destroy(W_INST(old)->whens);
      }
      W_INST(old)->whens=NULL;
      break;
    default:
      PANIC_ILLEGAL_INSTANCE;
  }
}


void FixWhensForRefinement(struct Instance *old, struct Instance *new){
  switch(new->t){
    case BOOLEAN_ATOM_INST:
      AssertMemory(BA_INST(new));
      if(BA_INST(new)->whens!=NULL) {
        FixWhensIf(old,new);
      }
      break;
    case INTEGER_ATOM_INST:
      AssertMemory(IA_INST(new));
      if(IA_INST(new)->whens!=NULL) {
        FixWhensIf(old,new);
      }
      break;
    case SYMBOL_ATOM_INST:
      AssertMemory(SYMA_INST(new));
      if(SYMA_INST(new)->whens!=NULL) {
        FixWhensIf(old,new);
      }
      break;
    case BOOLEAN_CONSTANT_INST:
      AssertMemory(BC_INST(new));
      if(BC_INST(new)->whens!=NULL) {
        FixWhensIf(old,new);
      }
      break;
    case INTEGER_CONSTANT_INST:
      AssertMemory(IC_INST(new));
      if(IC_INST(new)->whens!=NULL) {
        FixWhensIf(old,new);
      }
      break;
    case SYMBOL_CONSTANT_INST:
      AssertMemory(SYMC_INST(new));
      if(SYMC_INST(new)->whens!=NULL) {
        FixWhensIf(old,new);
      }
      break;
    case MODEL_INST:
      AssertMemory(MOD_INST(new));
      if(MOD_INST(new)->whens!=NULL) {
        FixWhensIf(old,new);
      }
      break;
    case REL_INST:
      AssertMemory(RELN_INST(new));
      if(RELN_INST(new)->whens!=NULL) {
        FixWhensIf(old,new);
      }
      break;
    case LREL_INST:
      AssertMemory(LRELN_INST(new));
      if(LRELN_INST(new)->whens!=NULL) {
        FixWhensIf(old,new);
      }
      break;
    case WHEN_INST:
      AssertMemory(W_INST(new));
      if(W_INST(new)->whens!=NULL) {
        FixWhensIf(old,new);
      }
      break;
    default:
      PANIC_ILLEGAL_INSTANCE;
  }
}

