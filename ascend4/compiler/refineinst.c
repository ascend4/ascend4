/*
 *  Ascend Instance Refinement Functions
 *  by Tom Epperly & Ben Allan1
 *  9/3/89
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: refineinst.c,v $
 *  Date last modified: $Date: 1998/02/05 16:37:32 $
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
#include "utilities/ascConfig.h"
#include "utilities/ascPanic.h"
#include "utilities/ascMalloc.h"
#include "general/pool.h"
#include "general/list.h"
#include "general/dstring.h"
#include "compiler/compiler.h"
#include "compiler/bit.h"
#include "compiler/symtab.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/functype.h"
#include "compiler/types.h"
#include "compiler/instance_enum.h"
#include "compiler/stattypes.h"
#include "compiler/slist.h"
#include "compiler/instance_name.h"
#include "compiler/instance_io.h"
#include "compiler/check.h"
#include "compiler/dump.h"
#include "compiler/child.h"
#include "compiler/type_desc.h"
#include "compiler/prototype.h"
#include "compiler/pending.h"
#include "compiler/find.h"
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
#include "compiler/setinstval.h"
#include "compiler/instance_types.h"
/* new */
#include "compiler/atomsize.h"
#include "compiler/atomvalue.h"
#include "compiler/cmpfunc.h"
#include "compiler/copyinst.h"
#include "compiler/createinst.h"
#include "compiler/destroyinst.h"
#include "compiler/extinst.h"
#include "compiler/instmacro.h"
#include "compiler/instquery.h"
#include "compiler/linkinst.h"
#include "compiler/mergeinst.h"
#include "compiler/parentchild.h"
#include "compiler/instantiate.h"
#include "compiler/refineinst.h"

#ifndef lint
static CONST char RefineInstModuleID[] = "$Id: refineinst.c,v 1.10 1998/02/05 16:37:32 ballan Exp $";
#endif


/* checks children, and does some value copying in the process */
static void CheckChild(struct Instance *old, struct Instance *new)
{
  register CONST dim_type *dimp;
  assert(old->t==new->t);
  switch(old->t){
  case REAL_INST:
    dimp = CheckDimensionsMatch(R_INST(old)->dimen,R_INST(new)->dimen);
    if (dimp==NULL){
      FPRINTF(ASCERR,"Dimensional mismatch in CheckChild.\n");
      R_INST(new)->dimen = R_INST(old)->dimen;
    }
    else
      R_INST(new)->dimen = dimp;
    if (R_INST(old)->assigned > R_INST(new)->assigned){
      R_INST(new)->assigned = R_INST(old)->assigned;
      R_INST(new)->depth = R_INST(old)->depth;
      R_INST(new)->value = R_INST(old)->value;
    }
    break;
  case BOOLEAN_INST:
    if (B_INST(old)->assigned > B_INST(new)->assigned){
      B_INST(new)->assigned = B_INST(old)->assigned;
      B_INST(new)->depth = B_INST(old)->depth;
      B_INST(new)->value = B_INST(old)->value;
    }
    break;
  case INTEGER_INST:
    if ((I_INST(old)->assigned)&&!(I_INST(new)->assigned)){
      I_INST(new)->assigned = I_INST(old)->assigned;
      I_INST(new)->value = I_INST(old)->value;
    }
    break;
  case SET_INST:
#ifndef NDEBUG
    if ((S_INST(old)->list)&&(S_INST(new)->list))
      assert(SetsEqual(S_INST(old)->list,S_INST(new)->list));
#endif
    if ((S_INST(old)->list!=NULL)&&(S_INST(new)->list==NULL)){
      S_INST(new)->list = S_INST(old)->list;
      S_INST(old)->list = NULL;
    }
    break;
  case SYMBOL_INST:
#ifndef NDEBUG
    if ((SYM_INST(new)->value!=NULL)&&(SYM_INST(new)->value!=NULL)) {
      assert(SYM_INST(new)->value == SYM_INST(old)->value);
    }
#endif
    if ((SYM_INST(old)->value!=NULL)&&(SYM_INST(new)->value==NULL)) {
      SYM_INST(new)->value = SYM_INST(old)->value;
    }
    break;
  default:
    Asc_Panic(2, NULL, "Incorrect type passed to CheckChild.\n");/*NOTREACHED*/
  }
}

static
void CheckAtomValuesOne(register struct Instance **old,
			register struct Instance **new,
			register unsigned long int num)
{
  while(num--)
    CheckChild(*(old++),*(new++));
}

static
void CheckAtomValuesTwo(struct Instance *old, struct Instance *new)
{
  register unsigned long c,num_children,pos;
  struct InstanceName name;
  num_children = NumberChildren(old);
  for(c=1;c<=num_children;c++){
    name = ChildName(old,c);
    pos = ChildSearch(new,&name);
    CheckChild(InstanceChild(old,c),InstanceChild(new,pos));
  }
}


/* As constants never change size, this always returns the given i
   unless it becomes universal and is replaced by already existing.
   But note, the IRT universal with already existing universal inst case
   never reaches here because it == ATS and is trapped in RefineInstance.
   We do not handle that case for this reason. */
static struct Instance *RefineRealConstant(struct RealConstantInstance *i,
					struct TypeDescription *type)
{
  register CONST dim_type *dimp;
  unsigned what=0;
  /* bit game: bit 0 = update val,1 = update dim, bits 8,16= err in val/dim */

  assert(MoreRefined(type,i->desc)==type); /* refinement possible? */
  AssertMemory(i);
  if (i->desc==type) return INST(i); /* if same type already, no work */
  if (ShortCutMakeUniversalInstance(type)) {
    /* NOT REACHED. see RefineRealAtom for expl. */
    FPRINTF(ASCERR,"Program logic error in RefineRealConstant.\n");
    FPRINTF(ASCERR,"Please report this message to %s.\n",ASC_BIG_BUGMAIL);
    return INST(i);
  }
  /* Work: new type is more refined, at least in the library */
  /* check for value and dimensions compatibility */
  if (!CIASS(i)) {
    if (ConstantDefaulted(type) ) {
      what |= 1;
    } /* else not assigned and not defaulted is fine. just leave value as is */
  } else {
    if ( (ConstantDefaulted(type) && i->value != GetConstantDefReal(type))
	|| !(ConstantDefaulted(type)) ) {
      what |= 16;
      /* we're hosed, type, value incompatible.possibly ahead of type. */
    } /* else shut up and do not assign */
  }

  /* check dimensions */
  dimp = GetConstantDimens(type);
  if ( !(IsWild(dimp)) ) { /* do we get type's dims? */
    if (IsWild(i->dimen)) { /* we do */
      what |=2;
    } else {
      if( CheckDimensionsMatch(dimp,i->dimen)==NULL) {
        what |= 8; /* hosed, dims mismatch */
      } /* else dims already where they belong, shutup. */
    }
  } else { /* type is wild */
    if (!IsWild(i->dimen)) what |= 8;
    /* the instance can't be ahead of wild. funny implicit type case. */
  }
  if (what & 24) { /* something messed up */
    if (what & 16) {
      FPRINTF(ASCERR,"Value conflict in RefineRealConstant.\n");
    }
    if (what & 8) {
      FPRINTF(ASCERR,"Dimensional conflict in RefineRealConstant.\n");
    }
    FPRINTF(ASCERR,"Unable to refine instance.\n");
  } else { /* ok, fix it up */
    if (what & 1) {
      SetRealAtomValue(INST(i),GetConstantDefReal(type),0);
    }
    if (what & 2) {
      SetRealAtomDims(INST(i),dimp);
    }
    if (GetUniversalFlag(type) && !GetUniversalFlag(i->desc)) {
      /* don't have to check universal table cause that was already up top */
      AddUniversalInstance(GetUniversalTable(),type,INST(i));
    }
    /* else don't need to change universal table, cause we didn't move */
    /* no move, so no parent update either */
    i->desc = type; /* finally somebody make the refinement */
  }
  return INST(i); /* always returns i */
}

/*
 * When the type of i is < type given, i will _always_
 * end up being blown away, either by merging with a UNIVERSAL
 * instance or by the copy to the new type. This needs to be
 * rewritten so that atoms with the same size (number children)
 * are always recycled rather than malloc/free'd.
 */
static struct Instance *RefineRealAtom(struct RealAtomInstance *i,
			    struct TypeDescription *type)
{
  struct RealAtomInstance *new;
  register struct gl_list_t *tmp;
  register CONST dim_type *dimp;
  assert(MoreRefined(type,i->desc)==type);
  AssertMemory(i);
  if (i->desc==type) return INST(i);
  /* The following line is a bug. It should be
     new=SCMUI(); merge parents; destroy(i); return new;
     It is not provoked because any existing universal atoms to
     refine up to are filtered out at the RefineInstance function.
     Therefore the following if never passes.
     As is, this fails to do the refinement at all if SCMUI passes.

     Another bug is that if this function was ever called with
     an atom to be refined up to an existing universal, the parents
     lists would not be properly merged.
     If any place other than RefineInstance called from RefineClique
     ever calls this function, this stuff must be fixed.

     In fact, since it never passes, we're going to comment it out.
  */
/*
 *if (ShortCutMakeUniversalInstance(type)) {
 *  return INST(i);
 */  /* NOT REACHED */
/*}
 */
  new = RA_INST(CreateRealInstance(type));
  /* check value */
  if (i->assigned > new->assigned){ /* old value is been assigned */
    new->depth = i->depth;
    new->assigned = i->assigned;
    new->value = i->value;
  }
  /* check dimensions */
  if ((dimp = CheckDimensionsMatch(i->dimen,new->dimen))==NULL){
    FPRINTF(ASCERR,"Dimensional conflict in RefineRealAtom.\n");
    FPRINTF(ASCERR,"Unable to refine instance.\n");
    DestroyInstance(INST(new),NULL);
    return INST(i);
  }
  else {
    new->dimen = dimp;
  }
  /* move interface pointer */
  new->interface_ptr = i->interface_ptr;
  i->interface_ptr = NULL;
  if (InterfaceNotify!=NULL)
    (*InterfaceNotify)(new->interface_ptr,INST(i),INST(new));
  /* fix cliques */
  new->alike_ptr = i->alike_ptr;
  i->alike_ptr = INST(i);
  FixCliques(INST(i),INST(new));
  /* swap parent lists */
  tmp = new->parents;
  new->parents = i->parents;
  i->parents = tmp;
  /* fix any relations which point to this instance */
  FixRelations(i,new);
  /* check children values */
  if (NumberChildren(INST(i))==NumberChildren(INST(new))) {
    CheckAtomValuesOne(RA_CHILD(i,0),RA_CHILD(new,0),NumberChildren(INST(i)));
  } else {
    CheckAtomValuesTwo(INST(i),INST(new));
  }
  ReDirectParents(INST(i),INST(new));
  /* fix universal stuff */
  if (GetUniversalFlag(i->desc)) {
    ChangeUniversalInstance(GetUniversalTable(),INST(i),INST(new));
  }
  DestroyInstance(INST(i),NULL);
  AssertMemory(new);
  return INST(new);
}

static struct Instance *
RefineBooleanConstant(struct BooleanConstantInstance *i,
                      struct TypeDescription *type)
{
  assert(MoreRefined(type,i->desc)==type);
  AssertMemory(i);
  if (i->desc==type) return INST(i);
  if (ShortCutMakeUniversalInstance(type)) {
    /* NOT REACHED */
    return INST(i);
  }
  /* check value */
  if (!CIASS(i) ) {
    if (ConstantDefaulted(type) ) {
      SetBooleanAtomValue(INST(i),GetConstantDefBoolean(type),0);
    }
  } else {
    if (BCV(i) != GetConstantDefBoolean(type) ) {
      FPRINTF(ASCERR,"Value conflict in RefineBooleanConstant.\n");
      FPRINTF(ASCERR,"Unable to refine instance.\n");
      return INST(i);
    }
  }
  if (GetUniversalFlag(type) && !GetUniversalFlag(i->desc)) {
    /* don't have to check universal table cause that was already up top */
    AddUniversalInstance(GetUniversalTable(),type,INST(i));
  }
  /* else don't need to change universal table, cause we didn't move */
  /* no move, so no parent update either */
  i->desc = type; /* finally somebody make the refinement */
  return INST(i);
}

static struct Instance *RefineBooleanAtom(struct BooleanAtomInstance *i,
			       struct TypeDescription *type)
{
  register struct BooleanAtomInstance *new;
  register struct gl_list_t *tmp;
  assert(MoreRefined(type,i->desc)==type);
  AssertMemory(i);
  if (i->desc==type) return INST(i);
  if (ShortCutMakeUniversalInstance(type)) return INST(i);
  new =  BA_INST(CreateBooleanInstance(type));
  /* check value */
  if (i->assigned > new->assigned){ /* old value has been assigned */
    new->depth = i->depth;
    new->assigned = i->assigned;
    new->value = i->value;
  }
  /* move interface pointer */
  new->interface_ptr = i->interface_ptr;
  i->interface_ptr = NULL;
  if (InterfaceNotify!=NULL)
    (*InterfaceNotify)(new->interface_ptr,INST(i),INST(new));
  /* fix cliques */
  new->alike_ptr = i->alike_ptr;
  i->alike_ptr = INST(i);
  FixCliques(INST(i),INST(new));
  tmp = new->parents;
  new->parents = i->parents;
  i->parents = tmp;
  /* fix any logical relation which points to this instance */
  FixLogRelations(INST(i),INST(new));
  /* fix any when which points to this instance */
  FixWhens(INST(i),INST(new));
  /* check children values */
  if (NumberChildren(INST(i))==NumberChildren(INST(new)))
    CheckAtomValuesOne(BA_CHILD(i,0),BA_CHILD(new,0),NumberChildren(INST(i)));
  else
    CheckAtomValuesTwo(INST(i),INST(new));
  ReDirectParents(INST(i),INST(new));
  /* fix universal stuff */
  if (GetUniversalFlag(i->desc))
    ChangeUniversalInstance(GetUniversalTable(),INST(i),INST(new));
  DestroyInstance(INST(i),NULL);
  AssertMemory(new);
  return INST(new);
}

static struct Instance
*RefineIntegerConstant(struct IntegerConstantInstance *i,
		       struct TypeDescription *type)
{
  assert(MoreRefined(type,i->desc)==type);
  AssertMemory(i);
  if (i->desc==type) return INST(i);
  if (ShortCutMakeUniversalInstance(type)) {
    /* NOT REACHED */
    return INST(i);
  }
  /* check value */
  if (!CIASS(i)) {
    if (ConstantDefaulted(type) ) {
      SetIntegerAtomValue(INST(i),GetConstantDefInteger(type),0);
    }
  } else {
    if (i->value != GetConstantDefInteger(type) ) {
      FPRINTF(ASCERR,"Value conflict in RefineIntegerConstant.\n");
      FPRINTF(ASCERR,"Unable to refine instance.\n");
      return INST(i);
    }
  }
  if (GetUniversalFlag(type) && !GetUniversalFlag(i->desc)) {
    /* don't have to check universal table cause that was already up top */
    AddUniversalInstance(GetUniversalTable(),type,INST(i));
  }
  /* else don't need to change universal table, cause we didn't move */
  /* no move, so no parent update either */
  i->desc = type; /* finally somebody make the refinement */
  return INST(i);
}

static struct Instance *RefineIntegerAtom(struct IntegerAtomInstance *i,
			       struct TypeDescription *type)
{
  register struct gl_list_t *tmp;
  register struct IntegerAtomInstance *new;
  assert(MoreRefined(type,i->desc)==type);
  AssertMemory(i);
  if (i->desc==type) return INST(i);
  if (ShortCutMakeUniversalInstance(type)) return INST(i);
  new = IA_INST(CreateIntegerInstance(type));
  /* check value */
  if (i->assigned > new->assigned){ /* old value is been assigned */
    new->depth = i->depth;
    new->assigned = i->assigned;
    new->value = i->value;
  }
  /* move interface pointer */
  new->interface_ptr = i->interface_ptr;
  i->interface_ptr = NULL;
  if (InterfaceNotify!=NULL)
    (*InterfaceNotify)(new->interface_ptr,INST(i),INST(new));
  /* fix cliques */
  new->alike_ptr = i->alike_ptr;
  i->alike_ptr = INST(i);
  FixCliques(INST(i),INST(new));
  tmp = new->parents;
  new->parents = i->parents;
  i->parents = tmp;
  /* fix any when which points to this instance */
  FixWhens(INST(i),INST(new));
  /* check children values */
  if (NumberChildren(INST(i))==NumberChildren(INST(new)))
    CheckAtomValuesOne(IA_CHILD(i,0),IA_CHILD(new,0),NumberChildren(INST(i)));
  else
    CheckAtomValuesTwo(INST(i),INST(new));
  ReDirectParents(INST(i),INST(new));
  /* fix universal stuff */
  if (GetUniversalFlag(i->desc))
    ChangeUniversalInstance(GetUniversalTable(),INST(i),INST(new));
  DestroyInstance(INST(i),NULL);
  AssertMemory(new);
  return INST(new);
}

/* this function REASSIGNS a set value. since when is that legal? ! */
static struct Instance *RefineSet(struct SetAtomInstance *i,
			   struct TypeDescription *type)
{
  register struct gl_list_t *tmp;
  register struct SetAtomInstance *new;
  assert(MoreRefined(type,i->desc)==type);
  AssertMemory(i);
  if (i->desc==type) return INST(i);
  if (ShortCutMakeUniversalInstance(type)) return INST(i);
  new = SA_INST(CreateSetInstance(type,i->int_set));
  /* check value */
  if (i->list!=NULL){
    new->list = i->list;
    i->list = NULL;
  }
  /* move interface pointer */
  new->interface_ptr = i->interface_ptr;
  i->interface_ptr = NULL;
  if (InterfaceNotify!=NULL)
    (*InterfaceNotify)(new->interface_ptr,INST(i),INST(new));
  /* fix cliques */
  new->alike_ptr = i->alike_ptr;
  i->alike_ptr = INST(i);
  FixCliques(INST(i),INST(new));
  tmp = new->parents;
  new->parents = i->parents;
  i->parents = tmp;
  /* check children values */
  if (NumberChildren(INST(i))==NumberChildren(INST(new)))
    CheckAtomValuesOne(SA_CHILD(i,0),SA_CHILD(new,0),NumberChildren(INST(i)));
  else
    CheckAtomValuesTwo(INST(i),INST(new));
  ReDirectParents(INST(i),INST(new));
  /* fix universal stuff */
  if (GetUniversalFlag(i->desc))
    ChangeUniversalInstance(GetUniversalTable(),INST(i),INST(new));
  DestroyInstance(INST(i),NULL);
  AssertMemory(new);
  return INST(new);
}

static struct Instance *RefineSymbolConstant(struct SymbolConstantInstance *i,
			      struct TypeDescription *type)
{
  assert(MoreRefined(type,i->desc)==type);
  AssertMemory(i);
  if (i->desc==type) return INST(i);
  if (ShortCutMakeUniversalInstance(type)) {
    /* NOT REACHED*/
    return INST(i);
  }
  if (i->value==NULL ) {
    if (ConstantDefaulted(type) ) {
      SetSymbolAtomValue(INST(i),GetConstantDefSymbol(type));
    }
  } else {
    if ( SYMC_INST(i)->value != GetConstantDefSymbol(type) ) {
      FPRINTF(ASCERR,"Value conflict in RefineSymbolConstant.\n");
      FPRINTF(ASCERR,"Unable to refine instance.\n");
      return INST(i);
    }
  }
  if (GetUniversalFlag(type) && !GetUniversalFlag(i->desc)) {
    /* don't have to check universal table cause that was already up top */
    AddUniversalInstance(GetUniversalTable(),type,INST(i));
  }
  i->desc = type;
  return INST(i);
}

static struct Instance *RefineSymbolAtom(struct SymbolAtomInstance *i,
			      struct TypeDescription *type)
{
  register struct gl_list_t *tmp;
  register struct SymbolAtomInstance *new;
  assert(MoreRefined(type,i->desc)==type);
  AssertMemory(i);
  if (i->desc==type) return INST(i);
  if (ShortCutMakeUniversalInstance(type)) return INST(i);
  new = SYMA_INST(CreateSymbolInstance(type));
  /* check value */
  if (i->value!=NULL){
    new->value = i->value;
  }
  /* move interface pointer */
  new->interface_ptr = i->interface_ptr;
  i->interface_ptr = NULL;
  if (InterfaceNotify!=NULL)
    (*InterfaceNotify)(new->interface_ptr,INST(i),INST(new));
  /* fix cliques */
  new->alike_ptr = i->alike_ptr;
  i->alike_ptr = INST(i);
  FixCliques(INST(i),INST(new));
  tmp = new->parents;
  new->parents = i->parents;
  i->parents = tmp;
  /* fix any when which points to this instance */
  FixWhens(INST(i),INST(new));
  /* check children values */
  if (NumberChildren(INST(i))==NumberChildren(INST(new)))
    CheckAtomValuesOne(SYMA_CHILD(i,0),SYMA_CHILD(new,0),
		       NumberChildren(INST(i)));
  else
    CheckAtomValuesTwo(INST(i),INST(new));
  ReDirectParents(INST(i),INST(new));
  /* fix universal stuff */
  if (GetUniversalFlag(i->desc))
    ChangeUniversalInstance(GetUniversalTable(),INST(i),INST(new));
  DestroyInstance(INST(i),NULL);
  AssertMemory(new);
  return INST(new);
}

/*
 * This function morphs the type and expands the bit list if the
 * refined to type is a new UNIVERSAL or a non-UNIVERSAL type >
 * the type of i. If the new type requires more children,
 * i is realloc'd.
 * If needed, adds the inst to the pending list again.
 * Does not reinvoke the instantiator (which should be obvious).
 *
 * around UNIVERSAL an arginst !=NULL needs work. fix me.
 */
static
struct Instance *RefineModel(struct ModelInstance *i,
			     struct TypeDescription *type,
                             struct ModelInstance *arginst)
{
  struct TypeDescription *oldtype;
  register unsigned long new_length,old_length;
  register struct ModelInstance *result;
  assert(MoreRefined(type,i->desc)==type);
  AssertMemory(i);
  AssertMemory(type);
  if (i->desc==type) {
    return INST(i);
  }
  /* we assume at this point that the sanity of the refinement
   * move has been checked.
   */
  oldtype = i->desc;
  /* pop i into pendings if needed */
  new_length = gl_length(GetList(GetStatementList(type)));
  if (new_length  > BLength(i->executed)){
    i->executed = ExpandFBList(i->executed,new_length);
    if (!InstanceInList(INST(i))) {
      /* i is necessarily a model instance at this point */
      AddBelow(NULL,INST(i));
	/* add PENDING model */
    }
  }
  new_length = ChildListLen(GetChildList(type));
  old_length = ChildListLen(GetChildList(i->desc));
  if (new_length > old_length){
    /* resize the instance */
    result = MOD_INST(ascrealloc((char *)i,
				 (unsigned)sizeof(struct ModelInstance)+
				 (unsigned)new_length*
				 (unsigned)sizeof(struct Instance *)));
    if (result!=i) {
      /* if realloc moved the instance, need to update all connections to
       * the instance from before it was refined to point at the new memory.
       */
      PendingInstanceRealloced(INST(i),INST(result)); /* change pending list */
      if (InterfaceNotify!=NULL)
	(*InterfaceNotify)(result->interface_ptr,INST(i),INST(result));
      /* fix external relations variables */
      FixExternalVars(INST(i),INST(result));
      /* fix whens */
      FixWhensForRefinement(INST(i),INST(result));
      ReDirectParents(INST(i),INST(result));
      ReDirectChildren(INST(i),INST(result));
      /* fix cliques */
      FixCliques(INST(i),INST(result));
      /* fix universal stuff */
      if (GetUniversalFlag(result->desc)) {
	ChangeUniversalInstance(GetUniversalTable(),INST(i),INST(result));
      }
    }
    /* init spaces of expanded instance to NULL */
    ZeroNewChildrenEntries(MOD_CHILD(result,old_length),new_length-old_length);
    ReorderChildrenPtrs(MOD_CHILD(result,0),
			GetChildList(result->desc),
			GetChildList(type),
			old_length,
			new_length);
    result->desc = type;
    ReConfigureInstFromArgs(INST(result),INST(arginst));
  } else {
    result = i;
    result->desc = type;
  }
  /* add new universal type entry */
  if (GetUniversalFlag(type)) {
    AddUniversalInstance(GetUniversalTable(),type,INST(result));
  }
  /* fix the reference counts in the library */
  DeleteTypeDesc(oldtype);
  CopyTypeDesc(type);
  AssertMemory(result);
  return INST(result);
}


struct Instance *RefineInstance(struct Instance *i,
				struct TypeDescription *type,
                                struct Instance *arginst)
{
  struct TypeDescription *desc;
  assert((i!=NULL)&&(type!=NULL));
  AssertMemory(i);
  /* oy, arginst will need some fancy footwork here. fix me */
  if (GetUniversalFlag(type)&&
      LookupInstance(GetUniversalTable(),type)){
    desc = InstanceTypeDesc(i);
    if (GetUniversalFlag(desc)){
      ChangeUniversalInstance(GetUniversalTable(),
			      i,
			      LookupInstance(GetUniversalTable(),type));
    }
    i = MergeInstances(i,LookupInstance(GetUniversalTable(),type));
    PostMergeCheck(i);
    return i;
  }
  /* if the above if/Merge are not there, then bugs in the Refine<type>
   * functions switched below will destroy type and memory integrity.
   * See RefineRealAtom for details.
   * No universal instance matching type both can and does exist .
   */
  switch(i->t) {
  case MODEL_INST:
    return RefineModel(MOD_INST(i),type,MOD_INST(arginst));
  case REAL_ATOM_INST:
    return RefineRealAtom(RA_INST(i),type);
  case BOOLEAN_ATOM_INST:
    return RefineBooleanAtom(BA_INST(i),type);
  case INTEGER_ATOM_INST:
    return RefineIntegerAtom(IA_INST(i),type);
  case SET_ATOM_INST:
    return RefineSet(SA_INST(i),type);
  case SYMBOL_ATOM_INST:
    return RefineSymbolAtom(SYMA_INST(i),type);
  case REAL_CONSTANT_INST:
    return RefineRealConstant(RC_INST(i),type);
  case BOOLEAN_CONSTANT_INST:
    return RefineBooleanConstant(BC_INST(i),type);
  case INTEGER_CONSTANT_INST:
    return RefineIntegerConstant(IC_INST(i),type);
  case SYMBOL_CONSTANT_INST:
    return RefineSymbolConstant(SYMC_INST(i),type);
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
  case REL_INST:
  case LREL_INST:
  case WHEN_INST:
  case SIM_INST:
    /* at the current time these are meaningless */
    Asc_Panic(2, NULL,
              "At the current time these refining"
              " an array,when or relation is undefined");
  case REAL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SET_INST:
  case SYMBOL_INST:
  case DUMMY_INST:
    Asc_Panic(2, NULL,
              "It is illegal to call RefineInstance on a fundamental atom.");
  default:
    Asc_Panic(2, NULL, "RefineInstance called on unknown instance type.\n");
  }
  exit(2);/* NOT REACHED.  Needed to keep gcc from whining */
}


struct Instance *RefineClique(struct Instance *i,
                              struct TypeDescription *type,
                              struct Instance *arginst)
{
  struct Instance *ptr;
  AssertMemory(i);
  /* assert that it is conformable and that type is more refined */
  assert(MoreRefined(InstanceTypeDesc(i),type)==type);
  if (arginst != NULL) {
    assert(MoreRefined(InstanceTypeDesc(arginst),type)==type);
  }
  ptr = i = RefineInstance(i,type,arginst);
  while ( (ptr = NextCliqueMember(ptr))  !=  i){
    assert(ptr!=NULL);
    ptr = RefineInstance(ptr,type,arginst);
  }
  return i;
}

