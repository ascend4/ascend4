/*
 *  Ascend Instance Tree Killing
 *  by Tom Epperly & Ben Allan
 *  9/3/89
 *  Version: $Revision: 1.12 $
 *  Version control file: $RCSfile: destroyinst.c,v $
 *  Date last modified: $Date: 1997/07/18 12:28:50 $
 *  Last modified by: $Author: mthomas $
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
#include "utilities/ascConfig.h"
#include "utilities/ascPanic.h"
#include "utilities/ascMalloc.h"
#include "general/pool.h"
#include "general/list.h"
#include "general/dstring.h"
#include "compiler/compiler.h"
#include "compiler/symtab.h"
#include "compiler/bit.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/functype.h"
#include "compiler/types.h"
#include "compiler/instance_enum.h"
#include "compiler/instance_name.h"
#include "compiler/instance_io.h"
#include "compiler/child.h"
#include "compiler/type_desc.h"
#include "compiler/instmacro.h"
#include "compiler/instquery.h"
#include "compiler/instance_types.h"
#include "compiler/setinstval.h"
#include "compiler/linkinst.h"
#include "compiler/destroyinst.h"
#include "compiler/arrayinst.h"
#include "compiler/parentchild.h"
#include "compiler/atomvalue.h"
#include "compiler/atomsize.h"
#include "compiler/check.h"
#include "compiler/dump.h"
#include "compiler/find.h"
#include "compiler/type_desc.h"
#include "compiler/prototype.h"
#include "compiler/pending.h"
#include "compiler/extfunc.h"
#include "compiler/relation_type.h"
#include "compiler/relation.h"
#include "compiler/logical_relation.h"
#include "compiler/logrelation.h"
#include "compiler/relation_util.h"
#include "compiler/logrel_util.h"
#include "compiler/rel_common.h"
#include "compiler/case.h"
#include "compiler/when_util.h"
#include "compiler/universal.h"
#include "compiler/instance_types.h"
#include "compiler/cmpfunc.h"

#ifndef lint
static CONST char DestroyInstModuleID[] = "$Id: destroyinst.c,v 1.12 1997/07/18 12:28:50 mthomas Exp $";
#endif


static
void DeleteIPtr(struct Instance *i)
{
  assert(i&&InterfacePtrDelete);
  AssertMemory(i);
  switch(i->t) {
  case SIM_INST:
    (*InterfacePtrDelete)(i,SIM_INST(i)->interface_ptr);
    SIM_INST(i)->interface_ptr = NULL;
    return;
  case MODEL_INST:
    (*InterfacePtrDelete)(i,MOD_INST(i)->interface_ptr);
    MOD_INST(i)->interface_ptr = NULL;
    break;
  case REAL_ATOM_INST:
    (*InterfacePtrDelete)(i,RA_INST(i)->interface_ptr);
    RA_INST(i)->interface_ptr = NULL;
    break;
  case BOOLEAN_ATOM_INST:
    (*InterfacePtrDelete)(i,BA_INST(i)->interface_ptr);
    BA_INST(i)->interface_ptr = NULL;
    break;
  case INTEGER_ATOM_INST:
    (*InterfacePtrDelete)(i,IA_INST(i)->interface_ptr);
    IA_INST(i)->interface_ptr = NULL;
    break;
  case SET_ATOM_INST:
    (*InterfacePtrDelete)(i,SA_INST(i)->interface_ptr);
    SA_INST(i)->interface_ptr = NULL;
    break;
  case SYMBOL_ATOM_INST:
    (*InterfacePtrDelete)(i,SYMA_INST(i)->interface_ptr);
    SYMA_INST(i)->interface_ptr = NULL;
    break;
  case REL_INST:
    (*InterfacePtrDelete)(i,RELN_INST(i)->interface_ptr);
    RELN_INST(i)->interface_ptr = NULL;
    break;
  case LREL_INST:
    (*InterfacePtrDelete)(i,LRELN_INST(i)->interface_ptr);
    LRELN_INST(i)->interface_ptr = NULL;
    break;
  case WHEN_INST:
    (*InterfacePtrDelete)(i,W_INST(i)->interface_ptr);
    W_INST(i)->interface_ptr = NULL;
    break;
  /* constants */
  case INTEGER_CONSTANT_INST:
    (*InterfacePtrDelete)(i,IC_INST(i)->interface_ptr);
    IC_INST(i)->interface_ptr = NULL;
    break;
  case BOOLEAN_CONSTANT_INST:
    (*InterfacePtrDelete)(i,BC_INST(i)->interface_ptr);
    BC_INST(i)->interface_ptr = NULL;
    break;
  case SYMBOL_CONSTANT_INST:
    (*InterfacePtrDelete)(i,SYMC_INST(i)->interface_ptr);
    SYMC_INST(i)->interface_ptr = NULL;
    break;
  case DUMMY_INST:
    (*InterfacePtrDelete)(i,D_INST(i)->interface_ptr);
    D_INST(i)->interface_ptr = NULL;
    break;
  case REAL_CONSTANT_INST:	/* fall through */
  /* arrays */
  case ARRAY_INT_INST:         /* fall through */
  case ARRAY_ENUM_INST:        /* fall through */
  /* fundies */
  case REAL_INST:		/* fall through */
  case INTEGER_INST:		/* fall through */
  case BOOLEAN_INST:		/* fall through */
  case SET_INST:		/* fall through */
  case SYMBOL_INST:		/* fall through */
    break;
  default:
    Asc_Panic(2, NULL, "Unknown instance type passed to DeleteIPtr.\n");
  }
}

/*********************************************************************\
int RemoveParentReference(inst,parent)

Return a true value if inst should be deleted; otherwise, return 0.
This never returns anything but 1 for DUMMY_INSTs.
\*********************************************************************/
static int RemoveParentReferences(struct Instance *inst,
                                  struct Instance *parent)
{
  register unsigned long c,pos,length;
  AssertMemory(inst);
  if (parent!=NULL) {
    AssertMemory(parent);
    /* destroy link from inst to parent */
    pos = SearchForParent(inst,parent);
    if (pos != 0 || inst->t == DUMMY_INST) {
      /* Because the dummy always 'adds' a parent,
       * it must always delete it to keep the ref_count happy.
       * Dummy knows of no parents, but knows
       * exactly how many it doesn't have.
       */
      DeleteParent(inst,pos);
    }
    /* destroy link(s) from parent to inst */
    while ((pos = ChildIndex(parent,inst))) {
      StoreChildPtr(parent,pos,NULL);
    }
    return (NumberParents(inst) == 0);
  } else {
    length = NumberParents(inst);
    if (inst->t == DUMMY_INST) {
      FPRINTF(ASCERR,
 "The global dummy instance cannot be destroyed w/out parental consent\n");
      FPRINTF(ASCERR, "You should not be seeing this message.\n");
      return 0;
    }
    for(c=1;c<=length;c++) {
      parent = InstanceParent(inst,c);
      while ((pos = ChildIndex(parent,inst))) {
	StoreChildPtr(parent,pos,NULL);
      }
    }
    return  1;
  }
}

static
void RemoveFromClique(struct Instance *inst)
{
  register struct Instance *i,*hold;
  AssertMemory(inst);
  if ((hold=i=NextCliqueMember(inst))==inst)
    return;
  while (NextCliqueMember(i)!=inst)
    i = NextCliqueMember(i);
  switch(i->t) {
  case MODEL_INST:
    MOD_INST(i)->alike_ptr = hold;
    break;
  case REAL_CONSTANT_INST:
    RC_INST(i)->alike_ptr = hold;
    break;
  case BOOLEAN_CONSTANT_INST:
    BC_INST(i)->alike_ptr = hold;
    break;
  case INTEGER_CONSTANT_INST:
    IC_INST(i)->alike_ptr = hold;
    break;
  case SYMBOL_CONSTANT_INST:
    SYMC_INST(i)->alike_ptr = hold;
    break;
  case REAL_ATOM_INST:
    RA_INST(i)->alike_ptr = hold;
    break;
  case BOOLEAN_ATOM_INST:
    BA_INST(i)->alike_ptr = hold;
    break;
  case INTEGER_ATOM_INST:
    IA_INST(i)->alike_ptr = hold;
    break;
  case SET_ATOM_INST:
    SA_INST(i)->alike_ptr = hold;
    break;
  case SYMBOL_ATOM_INST:
    SYMA_INST(i)->alike_ptr = hold;
    break;
  default:
    Asc_Panic(2, "RemoveFromClique",
              "Invalid instance in clique found in RemoveFromClique.\n");
  }
}

static
void DeleteArrayChild(struct ArrayChild *acp, struct Instance *parent)
{
  if ((acp!=NULL)&&(acp->inst!=NULL)) {
    AssertContainedMemory(acp,sizeof(struct ArrayChild));
    DestroyInstance(acp->inst,parent);
  }
}

static
void RemoveRelationLinks(struct Instance *i, struct gl_list_t *list)
/*********************************************************************\
Take an ATOM and tell all the relations that know about it to
forget it, then destroy the list that identifies those relations.
After this, a real ATOM can be safely deleted if there are no models
refering to it.
\*********************************************************************/
{
  register unsigned long c,length;
  assert(list!=NULL);
  length = gl_length(list);
  for(c=1;c<=length;c++) {
    ChangeRelationPointers(INST(gl_fetch(list,c)),i,INST(NULL));
  }
  gl_destroy(list);
}


/*********************************************************************\
Take an BOOLEAN ATOM, REL or LOGREL and tell all the logrelations that
know about them to forget them, then destroy the list that identifies
those logrelations.
After this, the ATOM, REL or LOGREL  can be deleted if there are no
models or whens refering to it.
\*********************************************************************/
static
void RemoveLogRelLinks(struct Instance *i, struct gl_list_t *list)
{
  register unsigned long c,length;
  assert(list!=NULL);
  length = gl_length(list);
  for(c=1;c<=length;c++) {
    ChangeLogRelPointers(INST(gl_fetch(list,c)),i,INST(NULL));
  }
  gl_destroy(list);
}


static
void RemoveWhenLinks(struct Instance *i, struct gl_list_t *list)
{
  register unsigned long c,length;
  assert(list!=NULL);
  length = gl_length(list);
  for(c=1;c<=length;c++) {
    ChangeWhenPointers(INST(gl_fetch(list,c)),i,INST(NULL));
  }
  gl_destroy(list);
}

static
void DestroyAtomChildren(register struct Instance **i,
			 register unsigned long int nc)
{
  while(nc-- > 0){
    AssertMemory(i);
    AssertMemory(*i);
    if ((*i)->t==SET_INST) {
      if (S_INST(*i)->list!=NULL) {
	DestroySet(S_INST(*i)->list);
      }
    }
    i++;			/* position pointer for next child */
  }
}

/*
 * should only be called when there areno more references
 * to the object.
 */
static void DestroyInstanceParts(struct Instance *i)
{
  register unsigned long c,length;
  register struct gl_list_t *l;
  struct Instance *child;
  AssertMemory(i);
  switch(i->t) {
  case SIM_INST:
    child = InstanceChild(i,1); /* one child only */
    DestroyInstance(child,i);
    SIM_INST(i)->name = NULL;	/* main symbol table owns the string */
    SIM_INST(i)->extvars = NULL;
    i->t = ERROR_INST;
    DeleteTypeDesc(SIM_INST(i)->desc);
    SIM_INST(i)->desc = NULL;
    ascfree((char *)i);
    return;
  case MODEL_INST:
    gl_destroy(MOD_INST(i)->parents);
    MOD_INST(i)->parents = NULL;
    MOD_INST(i)->alike_ptr = NULL;
    if (MOD_INST(i)->whens!=NULL) {
      RemoveWhenLinks(i,MOD_INST(i)->whens);
      MOD_INST(i)->whens=NULL;
    }
    DestroyBList(MOD_INST(i)->executed);
    MOD_INST(i)->executed = NULL;
    /* destroy reference to children */
    length = NumberChildren(i);
    for(c=1;c<=length;c++) {
      child = InstanceChild(i,c);
      DestroyInstance(child,i);
    }
    i->t = ERROR_INST;
    DeleteTypeDesc(MOD_INST(i)->desc);
    MOD_INST(i)->desc = NULL;
    ascfree((char *)i);
    return;
  case REAL_CONSTANT_INST:
    /* continue delete the atom */
    gl_destroy(RC_INST(i)->parents);
    RC_INST(i)->parents = NULL;
    DeleteTypeDesc(RC_INST(i)->desc);
    RC_INST(i)->desc = NULL;
    RC_INST(i)->alike_ptr = NULL;
    /* children are automatically deleted by the following */
    i->t = ERROR_INST;
    ascfree((char *)i);
    return;
  case BOOLEAN_CONSTANT_INST:
    gl_destroy(BC_INST(i)->parents);
    BC_INST(i)->parents = NULL;
    DeleteTypeDesc(BC_INST(i)->desc);
    BC_INST(i)->desc = NULL;
    BC_INST(i)->alike_ptr = NULL;
    if (BC_INST(i)->whens!=NULL) {
      RemoveWhenLinks(i,BC_INST(i)->whens);
      BC_INST(i)->whens=NULL;
    }
    i->t = ERROR_INST;
    ascfree((char *)i);
    return;
  case INTEGER_CONSTANT_INST:
    gl_destroy(IC_INST(i)->parents);
    IC_INST(i)->parents = NULL;
    DeleteTypeDesc(IC_INST(i)->desc);
    IC_INST(i)->desc = NULL;
    IC_INST(i)->alike_ptr = NULL;
    if (IC_INST(i)->whens!=NULL) {
      RemoveWhenLinks(i,IC_INST(i)->whens);
      IC_INST(i)->whens=NULL;
    }
    i->t = ERROR_INST;
    ascfree((char *)i);
    return;
  case SYMBOL_CONSTANT_INST:
    gl_destroy(SYMC_INST(i)->parents);
    SYMC_INST(i)->parents = NULL;
    DeleteTypeDesc(SYMC_INST(i)->desc);
    SYMC_INST(i)->desc = NULL;
    SYMC_INST(i)->alike_ptr = NULL;
    SYMC_INST(i)->value = NULL;
    if (SYMC_INST(i)->whens!=NULL) {
      RemoveWhenLinks(i,SYMC_INST(i)->whens);
      SYMC_INST(i)->whens=NULL;
    }
    i->t = ERROR_INST;
    ascfree((char *)i);
    return;
  case REAL_ATOM_INST:
    /* deallocate dynamic memory used by children */
    DestroyAtomChildren(RA_CHILD(i,0),
			ChildListLen(GetChildList(RA_INST(i)->desc)));
    /* continue delete the atom */
    gl_destroy(RA_INST(i)->parents);
    RA_INST(i)->parents = NULL;
    DeleteTypeDesc(RA_INST(i)->desc);
    RA_INST(i)->desc = NULL;
    RA_INST(i)->alike_ptr = NULL;
    if (RA_INST(i)->relations!=NULL) {
      RemoveRelationLinks(i,RA_INST(i)->relations);
      RA_INST(i)->relations=NULL;
    }
    /* children are automatically deleted by the following */
    i->t = ERROR_INST;
    ascfree((char *)i);
    return;
  case BOOLEAN_ATOM_INST:
    /* deallocate dynamic memory used by children */
    DestroyAtomChildren(BA_CHILD(i,0),
			ChildListLen(GetChildList(BA_INST(i)->desc)));
    /* continue delete the atom */
    gl_destroy(BA_INST(i)->parents);
    BA_INST(i)->parents = NULL;
    DeleteTypeDesc(BA_INST(i)->desc);
    BA_INST(i)->desc = NULL;
    BA_INST(i)->alike_ptr = NULL;
    if (BA_INST(i)->logrelations!=NULL)
      RemoveLogRelLinks(i,BA_INST(i)->logrelations);
    if (BA_INST(i)->whens!=NULL) {
      RemoveWhenLinks(i,BA_INST(i)->whens);
      BA_INST(i)->whens=NULL;
    }
    i->t = ERROR_INST;
    /* children are automatically deleted by the following */
    ascfree((char *)i);
    return;
  case INTEGER_ATOM_INST:
    /* deallocate dynamic memory used by children */
    DestroyAtomChildren(IA_CHILD(i,0),
			ChildListLen(GetChildList(IA_INST(i)->desc)));
    /* continue delete the atom */
    gl_destroy(IA_INST(i)->parents);
    IA_INST(i)->parents = NULL;
    DeleteTypeDesc(IA_INST(i)->desc);
    IA_INST(i)->desc = NULL;
    IA_INST(i)->alike_ptr = NULL;
    if (IA_INST(i)->whens!=NULL) {
      RemoveWhenLinks(i,IA_INST(i)->whens);
      IA_INST(i)->whens=NULL;
    }
    i->t = ERROR_INST;
    /* children are automatically deleted by the following */
    ascfree((char *)i);
    return;
  case SET_ATOM_INST:
    /* deallocate dynamic memory used by children */
    DestroyAtomChildren(SA_CHILD(i,0),
			ChildListLen(GetChildList(SA_INST(i)->desc)));
    /* continue delete the atom */
    gl_destroy(SA_INST(i)->parents);
    SA_INST(i)->parents = NULL;
    DeleteTypeDesc(SA_INST(i)->desc);
    SA_INST(i)->desc = NULL;
    SA_INST(i)->alike_ptr = NULL;
    if (SA_INST(i)->list != NULL)
      DestroySet(SA_INST(i)->list);
    i->t = ERROR_INST;
    ascfree((char *)i);
    return;
  case SYMBOL_ATOM_INST:
    /* deallocate dynamic memory used by children */
    DestroyAtomChildren(SYMA_CHILD(i,0),
			ChildListLen(GetChildList(SYMA_INST(i)->desc)));
    /* continue to delete the atom */
    gl_destroy(SYMA_INST(i)->parents);
    SYMA_INST(i)->parents = NULL;
    DeleteTypeDesc(SYMA_INST(i)->desc);
    SYMA_INST(i)->desc = NULL;
    SYMA_INST(i)->alike_ptr = NULL;
    if (SYMA_INST(i)->whens!=NULL) {
      RemoveWhenLinks(i,SYMA_INST(i)->whens);
      SYMA_INST(i)->whens = NULL;
    }
    SYMA_INST(i)->value = NULL;
    i->t = ERROR_INST;
    ascfree((char *)i);
    return;
  case REL_INST:
    /* deallocate dynamic memory used by children */
    DestroyAtomChildren(REL_CHILD(i,0),
			ChildListLen(GetChildList(RELN_INST(i)->desc)));
    /* continue deleting the relation */
    DeleteTypeDesc(RELN_INST(i)->desc);
    RELN_INST(i)->desc = NULL;
    RELN_INST(i)->parent[0] = NULL;
    RELN_INST(i)->parent[1] = NULL;
    if (RELN_INST(i)->logrels!=NULL) {
      RemoveLogRelLinks(i,RELN_INST(i)->logrels);
      RELN_INST(i)->logrels=NULL;
    }
    if (RELN_INST(i)->whens!=NULL) {
      RemoveWhenLinks(i,RELN_INST(i)->whens);
      RELN_INST(i)->whens=NULL;
    }
    /* delete references of reals to this expression */
    if (RELN_INST(i)->ptr != NULL){
      DestroyRelation(RELN_INST(i)->ptr,i);
      RELN_INST(i)->ptr = NULL;
    }
    /* after relation has been destroyed */
    RELN_INST(i)->ptr = NULL;
    i->t = ERROR_INST;
    ascfree((char *)i);
    return;
  case LREL_INST:
    DestroyAtomChildren(LREL_CHILD(i,0),
			ChildListLen(GetChildList(LRELN_INST(i)->desc)));
    DeleteTypeDesc(LRELN_INST(i)->desc);
    LRELN_INST(i)->desc = NULL;
    LRELN_INST(i)->parent[0] = NULL;
    LRELN_INST(i)->parent[1] = NULL;
    if (LRELN_INST(i)->logrels!=NULL) {
      RemoveLogRelLinks(i,LRELN_INST(i)->logrels);
      LRELN_INST(i)->logrels=NULL;
    }
    if (LRELN_INST(i)->whens!=NULL) {
      RemoveWhenLinks(i,LRELN_INST(i)->whens);
      LRELN_INST(i)->whens=NULL;
    }
    if (LRELN_INST(i)->ptr != NULL){
      DestroyLogRelation(LRELN_INST(i)->ptr,i);
      LRELN_INST(i)->ptr = NULL;
    }
    LRELN_INST(i)->ptr = NULL;
    i->t = ERROR_INST;
    ascfree((char *)i);
    return;
  case WHEN_INST:
    DeleteTypeDesc(W_INST(i)->desc);
    W_INST(i)->desc = NULL;
    W_INST(i)->parent[0] = NULL;
    W_INST(i)->parent[1] = NULL;
    if (W_INST(i)->whens!=NULL) {
      RemoveWhenLinks(i,W_INST(i)->whens);
      W_INST(i)->whens=NULL;
    }
    if (W_INST(i)->bvar!=NULL) {
      DestroyWhenVarList(W_INST(i)->bvar,i);
      W_INST(i)->bvar = NULL;
    }
    if (W_INST(i)->cases!=NULL) {
      DestroyWhenCaseList(W_INST(i)->cases,i);
      W_INST(i)->cases = NULL;
    }
    i->t = ERROR_INST;
    ascfree((char *)i);
    return;
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    DeleteTypeDesc(ARY_INST(i)->desc);
    gl_destroy(ARY_INST(i)->parents);
    ARY_INST(i)->parents = NULL;
    l = ARY_INST(i)->children;
    if (l!=NULL){
      length = gl_length(l);
      for (c=1; c <= length; c++) {
        DeleteArrayChild((struct ArrayChild *)gl_fetch(l,c),i);
        FREEPOOLAC(gl_fetch(l,c));
      }
      gl_destroy(l);
    }
    ARY_INST(i)->children = NULL;
    i->t = ERROR_INST;
    ascfree((char *)i);
    return;
  case REAL_INST:
    i->t = ERROR_INST;
    return;
  case INTEGER_INST:
    i->t = ERROR_INST;
    return;
  case BOOLEAN_INST:
    i->t = ERROR_INST;
    return;
  case SET_INST:
    if (S_INST(i)->list!=NULL)
      DestroySet(S_INST(i)->list);
    i->t = ERROR_INST;
    return;
  case SYMBOL_INST:
    i->t = ERROR_INST;
    return;
  case DUMMY_INST:
    /* no parts */
    DeleteTypeDesc(D_INST(i)->desc);
    i->t = ERROR_INST;
    ascfree((char *)i);
    return;
  default:
    Asc_Panic(2, "DeleteInstance",
              "Unknown type of instance passed to DeleteInstance.\n");
  }
}

void DestroyInstance(struct Instance *inst, struct Instance *parent)
{
  struct TypeDescription *desc;
  int delete;
  if (inst==NULL) return;
  if (InterfacePtrDelete!=NULL) {
    DeleteIPtr(inst);
  }
  delete = RemoveParentReferences(inst,parent);
  if (delete) {
    if (inst->t != DUMMY_INST) {
      desc = InstanceTypeDesc(inst);
      if (GetUniversalFlag(desc)){ /* universal is being deleted */
        RemoveUniversalInstance(GetUniversalTable(),inst);
      }
      if (IsCompoundInstance(inst) &&
          InstanceKind(inst) != SIM_INST &&
          ((struct PendInstance *)(inst))->p != NULL
         ) {
        RemoveInstance(inst);
      }
      /* remove PENDING or maybe not pending instance in destroy process. */
      RemoveFromClique(inst);
      DestroyInstanceParts(inst);
    } else {
      if (D_INST(inst)->ref_count<2) {
        desc = InstanceTypeDesc(inst);
        if (GetUniversalFlag(desc)){ /* universal is being deleted */
          RemoveUniversalInstance(GetUniversalTable(),inst);
        }
        /* dummy is never in cliques or pending */
        DestroyInstanceParts(inst);
      }
    }
  }
}

