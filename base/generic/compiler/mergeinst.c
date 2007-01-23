/*
 *  Ascend Instance Merge Implementation
 *  by Tom Epperly
 *  9/3/89
 *  Version: $Revision: 1.16 $
 *  Version control file: $RCSfile: mergeinst.c,v $
 *  Date last modified: $Date: 1998/03/17 22:09:08 $
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
#include "visitinst.h"
#include "extinst.h"
#include "parentchild.h"
#include "instance_types.h"
#include "instquery.h"
#include "linkinst.h"
#include "destroyinst.h"
#include "createinst.h"
#include "refineinst.h"
#include "atomvalue.h"
#include "atomsize.h"
#include "check.h"
#include "dump.h"
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
#include "cmpfunc.h"
#include <general/pool.h>
#include "tmpnum.h"
#include "setinstval.h"
#include "mergeinst.h"

#ifndef lint
static CONST char MergeInstModuleID[] = "$Id: mergeinst.c,v 1.16 1998/03/17 22:09:08 ballan Exp $";
#endif

/* forward declarations */
static
struct Instance *RecursiveMergeInstance(struct Instance *, struct Instance *);

/*
 * Issue merge illegal message.
 */
void BadMerge(FILE *fp, char *m1, 
              CONST struct Instance *i1, CONST struct Instance *i2, char *m2)
{
  FPRINTF(fp,"%sInstance 1 IS_A %s (ptr=%p)\nInstance 2 IS_A %s (ptr=%p)\n%s",
	  m1, SCP(InstanceType(i1)), (void *)InstanceTypeDesc(i1),
          SCP(InstanceType(i2)), (void *)InstanceTypeDesc(i2),m2);
}

/*
 * Issue merge really illegal message.
 */
void ReallyBadMerge(FILE *fp, char *m1, 
                    CONST struct Instance *i1, CONST struct Instance *i2,
                    char *m2)
{
  FPRINTF(fp, "%sInstance 1 IS_A %s(%d)\nInstance 2 IS_A %s(%d)\n%s",
          m1, SCP(InstanceType(i1)), InstanceKind(i1),
          SCP(InstanceType(i2)), InstanceKind(i2),m2);
}

/*
 * this function tries to guess which of i1,i2 to keep, based on
 * memory if d1,d2 ==, or on MoreRefined if d1,d2 !=.
 * Returns 1 or 2 if one should be kept.
 * Returns 0 if unconformable.
 * This function needs to change to deal with parameterized
 * types correctly, and then perhaps MergeInstances needs
 * to be revisited. Basuckly, we need to have a function that
 * returns EqualAsFarAsBuilt result about parametric i's being merged.
 */
static
int KeepWhichInstance(struct TypeDescription *d1,
		      struct TypeDescription *d2,
		      struct Instance *i1, struct Instance *i2)
{
  if (d1==d2){			/* heuristic based on memory usage */
#ifdef MEM_DECREASE
    if (i1>i2) {
      return 1;
    } else {
      return 2;
    }
#else
    if (i1<i2){
      return 1;
    } else {
      return 2;
    }
#endif
  } else {
    register struct TypeDescription *morerefined;
    morerefined = MoreRefined(d1,d2);
    if (morerefined==d1) return 1;
    if (morerefined==d2) return 2;
    assert(morerefined==NULL);
    return 0;
  }
}

static
int MergeValues(struct Instance *i1, struct Instance *i2)
/*********************************************************************\
Check to make sure the values of i1 and i2 are conformable.  The
instance i1 will be set to the most refined value.  Parts of i2 may
be cannibalized. Merging value on constants here has a clique side effect.
Returns 0 if happy, 1 or other if not.
\*********************************************************************/
{
  register CONST dim_type *dim;
  assert(i1&&i2&&i1->t==i2->t);
  AssertMemory(i1);
  AssertMemory(i2);
  switch(i1->t){
  case REAL_ATOM_INST:
    dim = CheckDimensionsMatch(RA_INST(i1)->dimen,RA_INST(i2)->dimen);
    if (dim==NULL) {
      FPRINTF(ASCERR,"Dimensional mismatch.\n");
      return 1;
    }
    RA_INST(i1)->dimen = dim;
    if ((RA_INST(i1)->depth < RA_INST(i2)->depth)||
	((RA_INST(i1)->depth == RA_INST(i2)->depth)&&
	 (RA_INST(i1)->assigned < RA_INST(i2)->assigned))){
      RA_INST(i1)->depth = RA_INST(i2)->depth;
      RA_INST(i1)->assigned = RA_INST(i2)->assigned;
      RA_INST(i1)->value = RA_INST(i2)->value;
    }
    return 0;
  case REAL_CONSTANT_INST:
    dim = CheckDimensionsMatch(RC_INST(i1)->dimen,RC_INST(i2)->dimen);
    if (dim==NULL) {
      FPRINTF(ASCERR,"Constant dimensional mismatch.\n");
      return 1;
    }
    /* dimens compatible, but must check that values are before changing */
    if ( CIASS(i1) && CIASS(i2) &&
         (RC_INST(i1)->value != RC_INST(i2)->value) ) {
      FPRINTF(ASCERR,"Real constant value mismatch.\n");
      return 1;
    }
    /* now known compatible somehow */
    if (!CIASS(i1) && CIASS(i2)) {
      /* set i1val if needed */
      SetRealAtomValue(INST(i1),RC_INST(i2)->value,0);
    }
    /* nonwild one, if such, goes. Note use of Set and not ->dimen. */
    if (!IsWild(dim) && dim != RC_INST(i1)->dimen) {
      SetRealAtomDims(INST(i1),dim);  /* nonwild i2 dim -> i1 */
    }
    return 0;
  case BOOLEAN_ATOM_INST:
    if ((BA_INST(i1)->depth < BA_INST(i2)->depth)||
	((BA_INST(i1)->depth == BA_INST(i2)->depth)&&
	 (BA_INST(i1)->assigned < BA_INST(i2)->assigned))){
      BA_INST(i1)->value = BA_INST(i2)->value;
      BA_INST(i1)->assigned = BA_INST(i2)->assigned;
      BA_INST(i1)->depth = BA_INST(i2)->depth;
    }
    return 0;
  case BOOLEAN_CONSTANT_INST:
    if ( CIASS(i1) && CIASS(i2) && (BCV(i1) != BCV(i2)) ) {
      FPRINTF(ASCERR,"Boolean constant value mismatch.\n");
      return 1;
    } /* else compatible */
    if (!CIASS(i1) && CIASS(i2)) {
      SetBooleanAtomValue(INST(i1),BCV(i2),0);
    }
    return 0;
  case INTEGER_ATOM_INST:
    if ((IA_INST(i1)->depth < IA_INST(i2)->depth)||
	((IA_INST(i1)->depth == IA_INST(i2)->depth)&&
	 (IA_INST(i1)->assigned < IA_INST(i2)->assigned))){
      IA_INST(i1)->value = IA_INST(i2)->value;
      IA_INST(i1)->assigned = IA_INST(i2)->assigned;
      IA_INST(i1)->depth = IA_INST(i2)->depth;
    }
    return 0;
  case INTEGER_CONSTANT_INST:
    if (CIASS(i1) && CIASS(i2)
         && IC_INST(i1)->value != IC_INST(i2)->value ){
      FPRINTF(ASCERR,"Integer constant value mismatch.\n");
      return 1;
    } /* else compatible */
    if (!CIASS(i1) && CIASS(i2)) {
      SetIntegerAtomValue(INST(i1),IC_INST(i2)->value,0);
    }
    return 0;
  case SET_ATOM_INST:
    if (SA_INST(i2)->list!=NULL){
      if (SA_INST(i1)->list!=NULL){
	if (SetsEqual(SA_INST(i1)->list,SA_INST(i2)->list)){
	  /* heuristic to save the one with the lower address */
#ifdef MEM_DECREASE
	  if (SA_INST(i2)->list > SA_INST(i1)->list){
	    register struct set_t *temp;
	    temp = SA_INST(i2)->list;
	    SA_INST(i2)->list = SA_INST(i1)->list;
	    SA_INST(i1)->list = temp;
	  }
#else
	  if (SA_INST(i2)->list < SA_INST(i1)->list){
	    register struct set_t *temp;
	    temp = SA_INST(i2)->list;
	    SA_INST(i2)->list = SA_INST(i1)->list;
	    SA_INST(i1)->list = temp;
	  }
#endif
	  return 0;
	} else {
          FPRINTF(ASCERR,"Set value mismatch.\n  ");
	  WriteInstanceName(ASCERR,i1,NULL);
	  FPRINTF(ASCERR," = \n  ");
	  WriteAtomValue(ASCERR,i1);
	  FPRINTF(ASCERR,"\n  ");
	  WriteInstanceName(ASCERR,i2,NULL);
	  FPRINTF(ASCERR," = \n  ");
	  WriteAtomValue(ASCERR,i2);
	  FPRINTF(ASCERR,"\n");
          return 1;
	}
      } else {
	SA_INST(i1)->list = SA_INST(i2)->list;
	SA_INST(i2)->list = NULL;
      }
    }
    return 0;
  case SYMBOL_ATOM_INST:
    if (SYMA_INST(i2)->value!=NULL){
      if (SYMA_INST(i1)->value!=NULL)
	return (SYMA_INST(i1)->value != SYMA_INST(i2)->value);
      else{
	SYMA_INST(i1)->value = SYMA_INST(i2)->value;
      }
    }
    return 0;
  case SYMBOL_CONSTANT_INST:
    if (SYMC_INST(i2)->value!=NULL){
      if (SYMC_INST(i1)->value!=NULL) {
	return ( SYMC_INST(i1)->value != SYMC_INST(i2)->value);
      } else{
	SetSymbolAtomValue(INST(i1),SYMC_INST(i2)->value);
      }
    }
    return 0;
  case REAL_INST:
    dim = CheckDimensionsMatch(R_INST(i1)->dimen,R_INST(i2)->dimen);
    if (dim==NULL) {
      FPRINTF(ASCERR,"Dimensional mismatch.\n");
      return 1;
    }
    R_INST(i1)->dimen = dim;
    if ((R_INST(i1)->depth < R_INST(i2)->depth)||
	((R_INST(i1)->depth == R_INST(i2)->depth)&&
	 (R_INST(i1)->assigned < R_INST(i2)->assigned))){
      R_INST(i1)->depth = R_INST(i2)->depth;
      R_INST(i1)->assigned = R_INST(i2)->assigned;
      R_INST(i1)->value = R_INST(i2)->value;
    }
    return 0;
  case INTEGER_INST:
    if ((I_INST(i1)->depth < I_INST(i2)->depth)||
	((I_INST(i1)->depth == I_INST(i2)->depth)&&
	 (I_INST(i1)->assigned < I_INST(i2)->assigned))){
      I_INST(i1)->value = I_INST(i2)->value;
      I_INST(i1)->assigned = I_INST(i2)->assigned;
      I_INST(i1)->depth = I_INST(i2)->depth;
    }
    return 0;
  case BOOLEAN_INST:
    if ((B_INST(i1)->depth < B_INST(i2)->depth)||
	((B_INST(i1)->depth == B_INST(i2)->depth)&&
	 (B_INST(i1)->assigned < B_INST(i2)->assigned))){
      B_INST(i1)->value = B_INST(i2)->value;
      B_INST(i1)->assigned = B_INST(i2)->assigned;
      B_INST(i1)->depth = B_INST(i2)->depth;
    }
    return 0;
  case SET_INST:
    if (S_INST(i2)->list!=NULL){
      if (S_INST(i1)->list!=NULL){
	if (SetsEqual(S_INST(i1)->list,S_INST(i2)->list)){
	  /* heuristic to save the one with the lower address */
#ifdef MEM_DECREASE
	  if (S_INST(i2)->list > S_INST(i1)->list){
	    register struct set_t *temp;
	    temp = S_INST(i2)->list;
	    S_INST(i2)->list = S_INST(i1)->list;
	    S_INST(i1)->list = temp;
	  }
#else
	  if (S_INST(i2)->list < S_INST(i1)->list){
	    register struct set_t *temp;
	    temp = S_INST(i2)->list;
	    S_INST(i2)->list = S_INST(i1)->list;
	    S_INST(i1)->list = temp;
	  }
#endif
	  return 0;
	}
	else return 1;
      }
      else{
	S_INST(i1)->list = S_INST(i2)->list;
	S_INST(i2)->list = NULL;
      }
    }
    return 0;
  case SYMBOL_INST:
    if (SYM_INST(i2)->value!=NULL){
      if (SYM_INST(i1)->value!=NULL)
	return (SYM_INST(i1)->value != SYM_INST(i2)->value);
      else{
	SYM_INST(i1)->value = SYM_INST(i2)->value;
      }
    }
    return 0;
  default:
    ASC_PANIC("Error in MergeValues in instance.c. Unknown type.\n");
    
  }
}

/* called on only things w/ atom children */
static
void MergeCommonChildren(struct Instance **i1,
			 struct Instance **i2,
			 unsigned long int num)
{
  while (num--){
    (void)MergeValues(*(i1++),*(i2++));
  }
}

/* also called only on things with atom children */
static
void MergeUncommonChildren(struct Instance *i1, struct Instance *i2)
/*********************************************************************\
This procedure assumes that i1 is more refined that i2 and that i1
has more children that i2.
\*********************************************************************/
{
  unsigned long c,len;
  struct InstanceName name;
  struct Instance *child1,*child2;
  len = NumberChildren(i2);
  for(c=1;c<=len;c++){
    child2 = InstanceChild(i2,c);
    name = ChildName(i2,c);
    child1 = InstanceChild(i1,ChildSearch(i1,&name));
    (void)MergeValues(child1,child2);
  }
}

/* merge ATOM children */
static
void MergeChildrenValues(struct Instance *i1, struct Instance *i2)
/*********************************************************************\
This procedure will merge the values of the children of atoms i1 and
i2.  It is assumed that i1 is conformable with i2 and that i1 is
as refined or more refined that i2.
\*********************************************************************/
{
  assert(i1&&i2&&InstanceKind(i1)==InstanceKind(i2));
  if (NumberChildren(i1)==NumberChildren(i2)){
    switch(InstanceKind(i1)) {
    case REL_INST:
      MergeCommonChildren(REL_CHILD(i1,0),REL_CHILD(i2,0),NumberChildren(i1));
      break;
    case LREL_INST:
      MergeCommonChildren(LREL_CHILD(i1,0),LREL_CHILD(i2,0),
                          NumberChildren(i1));
      break;
    case REAL_ATOM_INST:
      MergeCommonChildren(RA_CHILD(i1,0),RA_CHILD(i2,0),NumberChildren(i1));
      break;
    case BOOLEAN_ATOM_INST:
      MergeCommonChildren(BA_CHILD(i1,0),BA_CHILD(i2,0),NumberChildren(i1));
      break;
    case INTEGER_ATOM_INST:
      MergeCommonChildren(IA_CHILD(i1,0),IA_CHILD(i2,0),NumberChildren(i1));
      break;
    case SET_ATOM_INST:
      MergeCommonChildren(SA_CHILD(i1,0),SA_CHILD(i2,0),NumberChildren(i1));
      break;
    case SYMBOL_ATOM_INST:
      MergeCommonChildren(SYMA_CHILD(i1,0),SYMA_CHILD(i2,0),
			  NumberChildren(i1));
      break;
    default:
      ASC_PANIC("Wrong type passed to MergeChildrenValues.\n");
    }
  } else {
    MergeUncommonChildren(i1,i2);
  }
}

/* this innocent little thing is killing us. */
static
void MergeParentLists(struct gl_list_t *l1,
		      struct gl_list_t *l2,
		      struct Instance *old,
		      struct Instance *new)
{
  register unsigned long c,len;
  register struct Instance *parent;
  len = gl_length(l2);
  for(c=1;c<=len;c++){
    parent = INST(gl_fetch(l2,c));
    if (gl_search(l1,(char *)parent,(CmpFunc)CmpParents)==0)
      gl_insert_sorted(l1,(char *)parent,(CmpFunc)CmpParents);
    ChangeParent(parent,old,new);
  }
  gl_destroy(l2);
}

static
void MergeParents(struct Instance *i1, struct Instance *i2)
{
/*
 * Change i2's parents to i1's parents.
 */
  assert(i1&&i2&&InstanceKind(i1)==InstanceKind(i2));
  switch(InstanceKind(i1)) {
  case MODEL_INST:
    MergeParentLists(MOD_INST(i1)->parents,MOD_INST(i2)->parents,i2,i1);
    MOD_INST(i2)->parents = gl_create(0L);
    break;
  case REAL_ATOM_INST:
    MergeParentLists(RA_INST(i1)->parents,RA_INST(i2)->parents,i2,i1);
    RA_INST(i2)->parents = gl_create(0L);
    break;
  case BOOLEAN_ATOM_INST:
    MergeParentLists(BA_INST(i1)->parents,BA_INST(i2)->parents,i2,i1);
    BA_INST(i2)->parents = gl_create(0L);
    break;
  case INTEGER_ATOM_INST:
    MergeParentLists(IA_INST(i1)->parents,IA_INST(i2)->parents,i2,i1);
    IA_INST(i2)->parents = gl_create(0L);
    break;
  case SET_ATOM_INST:
    MergeParentLists(SA_INST(i1)->parents,SA_INST(i2)->parents,i2,i1);
    SA_INST(i2)->parents = gl_create(0L);
    break;
  case SYMBOL_ATOM_INST:
    MergeParentLists(SYMA_INST(i1)->parents,SYMA_INST(i2)->parents,i2,i1);
    SYMA_INST(i2)->parents = gl_create(0L);
    break;
  case REAL_CONSTANT_INST:
    MergeParentLists(RC_INST(i1)->parents,RC_INST(i2)->parents,i2,i1);
    RC_INST(i2)->parents = gl_create(0L);
    break;
  case BOOLEAN_CONSTANT_INST:
    MergeParentLists(BC_INST(i1)->parents,BC_INST(i2)->parents,i2,i1);
    BC_INST(i2)->parents = gl_create(0L);
    break;
  case INTEGER_CONSTANT_INST:
    MergeParentLists(IC_INST(i1)->parents,IC_INST(i2)->parents,i2,i1);
    IC_INST(i2)->parents = gl_create(0L);
    break;
  case SYMBOL_CONSTANT_INST:
    MergeParentLists(SYMC_INST(i1)->parents,SYMC_INST(i2)->parents,i2,i1);
    SYMC_INST(i2)->parents = gl_create(0L);
    break;
  case REL_INST:
    assert((NumberParents(i1)==1)&&(NumberParents(i2)==1));
    ChangeParent(RELN_INST(i2)->parent[0],i2,i1);
    AddParent(i1,RELN_INST(i2)->parent[0]);
    RELN_INST(i2)->parent[0] = NULL;
    break;
  case LREL_INST:
    assert((NumberParents(i1)==1)&&(NumberParents(i2)==1));
    ChangeParent(LRELN_INST(i2)->parent[0],i2,i1);
    AddParent(i1,LRELN_INST(i2)->parent[0]);
    LRELN_INST(i2)->parent[0] = NULL;
    break;
  case WHEN_INST:
    assert((NumberParents(i1)==1)&&(NumberParents(i2)==1));
    ChangeParent(W_INST(i2)->parent[0],i2,i1);
    AddParent(i1,W_INST(i2)->parent[0]);
    W_INST(i2)->parent[0] = NULL;
    break;
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    MergeParentLists(ARY_INST(i1)->parents,ARY_INST(i2)->parents,i2,i1);
    ARY_INST(i2)->parents = gl_create(0L);
    break;
  case REAL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SET_INST:
  case SYMBOL_INST:
  case DUMMY_INST:
    ASC_PANIC("Shouldn't be called on fundamental/dummy type.\n");
  default:
    ASC_PANIC("Unknown instance type passed to MergeParents.\n");
  }
}

static
int InClique(struct Instance *i1, struct Instance *i2)
{
  register struct Instance *ptr;
  ptr = i1;
  do{
    if (ptr==i2) return 1;
    ptr = NextCliqueMember(ptr);
  } while(ptr!=i1);
  return 0;
}

/* wire together cliques, but does not mess with refinements */
void MergeCliques(struct Instance *i1, struct Instance *i2)
{
  register struct Instance *tmp;
  if (!InClique(i1,i2)){
    tmp = NextCliqueMember(i1);
    SetNextCliqueMember(i1,NextCliqueMember(i2));
    SetNextCliqueMember(i2,tmp);
  }
}

static
struct Instance *MergeConstants(struct Instance *i1, struct Instance *i2)
{
  struct TypeDescription *desc;
  switch(KeepWhichInstance(InstanceTypeDesc(i1),
			   InstanceTypeDesc(i2),
			   INST(i1),INST(i2))){
  case 1:			/* keep instance 1 */
    if (MergeValues(i1,i2)) return NULL; /* check instance values */
    /* no interface pointers, no children */
    MergeParents(i1,i2);
    MergeCliques(i1,i2);
    if((i2->t==BOOLEAN_CONSTANT_INST)||(i2->t==INTEGER_CONSTANT_INST)||
       (i2->t==SYMBOL_CONSTANT_INST)) {
      FixWhens(i2,i1);
    }
    desc = InstanceTypeDesc(i2);
    if (GetUniversalFlag(desc)) {
      ChangeUniversalInstance(GetUniversalTable(),INST(i2),INST(i1));
    }
    DestroyInstance(i2,NULL);
    return i1;
  case 2:			/* keep instance 2 */
    if (MergeValues(i2,i1)) return NULL; /* check instance values */
    MergeParents(i2,i1);
    MergeCliques(i2,i1);
    if((i1->t==BOOLEAN_CONSTANT_INST)||
       (i1->t==INTEGER_CONSTANT_INST)||
       (i1->t==SYMBOL_CONSTANT_INST)) {
      FixWhens(i1,i2);
    }
    desc = InstanceTypeDesc(i1);
    if (GetUniversalFlag(desc)) {
      ChangeUniversalInstance(GetUniversalTable(),INST(i1),INST(i2));
    }
    DestroyInstance(i1,NULL);
    return i2;
  default:			/* unconformable types or something wrong */
    assert(MoreRefined(InstanceTypeDesc(i1),InstanceTypeDesc(i2))==NULL);
    BadMerge(ASCERR,"Unconformable ARE_THE_SAME of constants.\n",
               INST(i1), INST(i2),"");
    DifferentVersionCheck(InstanceTypeDesc(i1),InstanceTypeDesc(i2));
    return NULL;
  }
}

static
struct Instance *MergeAtoms(struct Instance *i1, struct Instance *i2)
{
  struct TypeDescription *desc;
  switch(KeepWhichInstance(InstanceTypeDesc(i1),
			   InstanceTypeDesc(i2),
			   INST(i1),INST(i2))){
  case 1:			/* keep instance 1 */
    if (MergeValues(i1,i2)) return NULL; /* check instance values */
    if (InterfacePtrATS!=NULL) {
      (*InterfacePtrATS)(i1,i2);
    }
    MergeChildrenValues(i1,i2);
    MergeParents(i1,i2);
    MergeCliques(i1,i2);
    switch (i2->t) {
    case REAL_ATOM_INST:
      FixRelations(RA_INST(i2),RA_INST(i1));
      break;
    case BOOLEAN_ATOM_INST:
      FixLogRelations(i2,i1);
      FixWhens(i2,i1);
      break;
    case INTEGER_ATOM_INST:
    case SYMBOL_ATOM_INST:
      FixWhens(i2,i1);
      break;
    default:
      break;
    }
    desc = InstanceTypeDesc(i2);
    if (GetUniversalFlag(desc)) {
      ChangeUniversalInstance(GetUniversalTable(),INST(i2),INST(i1));
    }
    DestroyInstance(i2,NULL);
    return i1;
  case 2:			/* keep instance 2 */
    if (MergeValues(i2,i1)) return NULL; /* check instance values */
    if (InterfacePtrATS!=NULL) {
      (*InterfacePtrATS)(i2,i1);
    }
    MergeChildrenValues(i2,i1);
    MergeParents(i2,i1);
    MergeCliques(i2,i1);
    switch (i1->t) {
    case REAL_ATOM_INST:
      FixRelations(RA_INST(i1),RA_INST(i2));
      break;
    case BOOLEAN_ATOM_INST:
      FixLogRelations(i1,i2);
      FixWhens(i1,i2);
      break;
    case INTEGER_ATOM_INST:
    case SYMBOL_ATOM_INST:
      FixWhens(i1,i2);
      break;
    default:
      break;
    }
    desc = InstanceTypeDesc(i1);
    if (GetUniversalFlag(desc)) {
      ChangeUniversalInstance(GetUniversalTable(),INST(i1),INST(i2));
    }
    DestroyInstance(i1,NULL);
    return i2;
  default:			/* unconformable types or something wrong */
    assert(MoreRefined(InstanceTypeDesc(i1),InstanceTypeDesc(i2))==NULL);
    BadMerge(ASCERR,"Unconformable ARE_THE_SAME.\n",
               INST(i1), INST(i2),"");
    DifferentVersionCheck(InstanceTypeDesc(i1),InstanceTypeDesc(i2));
    return NULL;
  }
}

static
void MergeModelValues(struct ModelInstance *i1, struct ModelInstance *i2)
{
/*
 * assumes that i1 is more refined.  Checks the bitlists.
 */
  register unsigned long c,len;
  len = BLength(i2->executed);
  for(c=0;c<len;c++)
    if (!ReadBit(i2->executed,c)) ClearBit(i1->executed,c);
}

static
void RemoveParent(struct Instance *child, struct Instance *parent)
{
  unsigned long pos;
  if((pos = SearchForParent(child,parent))!=0)
    DeleteParent(child,pos);
}

static
void MergeCommonModelChildren(struct Instance **i1,
			      struct Instance **i2,
			      unsigned long int num,
			      struct Instance *newparent,
			      struct Instance *oldparent)
{
  while (num--){
    if ((*i1 != NULL)&&(*i2 != NULL)){
      (void)RecursiveMergeInstance(*i1,*i2);
    }
    else if (*i2 != NULL){
      RemoveParent(*i2,oldparent);
      if (SearchForParent(*i2,newparent)==0)
	AddParent(*i2,newparent);
      *i1 = *i2;
      *i2 = NULL;
    }
    i1++;
    i2++;
  }
}

static
void MergeUncommonModelChildren(struct Instance *i1, struct Instance *i2)
{
  unsigned long c,len,pos;
  struct Instance *child1,*child2;
  struct InstanceName name;
  len = NumberChildren(i2);
  for(c=1;c<=len;c++){
    child2 = InstanceChild(i2,c);
    name = ChildName(i2,c);
    child1 = InstanceChild(i1,pos=ChildSearch(i1,&name));
    if ((child1 != NULL)&&(child2 != NULL))
      (void)RecursiveMergeInstance(child1,child2);
    else if (child2 != NULL){
      StoreChildPtr(i1,pos,child2);
      StoreChildPtr(i2,c,NULL);
      RemoveParent(child2,i2);
      if (SearchForParent(child2,i1)==0) AddParent(child2,i1);
    }
  }
}

static
void MergeModelChildren(struct ModelInstance *i1, struct ModelInstance *i2)
{
/*
 * assumes that i1 is more refined that i2.
 */
  if (NumberChildren(INST(i1))==NumberChildren(INST(i2))) {
    MergeCommonModelChildren(MOD_CHILD(i1,0),MOD_CHILD(i2,0),
			     NumberChildren(INST(i1)),INST(i1),INST(i2));
  } else {
    MergeUncommonModelChildren(INST(i1),INST(i2));
  }
}

static
struct Instance *MergeModels(struct ModelInstance *i1,
			     struct ModelInstance *i2)
{
  switch(KeepWhichInstance(i1->desc,i2->desc,INST(i1),INST(i2))){
  case 1:
    if (InterfacePtrATS!=NULL) {
      (*InterfacePtrATS)(INST(i1),INST(i2));
    }
    FixExternalVars(INST(i1),INST(i2));
    FixWhens(INST(i1),INST(i2));
    MergeModelValues(i1,i2);
    MergeModelChildren(i1,i2);
    MergeParents(INST(i1),INST(i2));
    MergeCliques(INST(i1),INST(i2));
    if (GetUniversalFlag(i2->desc))
      ChangeUniversalInstance(GetUniversalTable(),INST(i2),INST(i1));
    DestroyInstance(INST(i2),NULL);
    return INST(i1);
  case 2:
    if (InterfacePtrATS!=NULL) {
      (*InterfacePtrATS)(INST(i2),INST(i1));
    }
    FixExternalVars(INST(i2),INST(i1));
    FixWhens(INST(i2),INST(i1));
    MergeModelValues(i2,i1);
    MergeModelChildren(i2,i1);
    MergeParents(INST(i2),INST(i1));
    MergeCliques(INST(i2),INST(i1));
    if (GetUniversalFlag(i1->desc))
      ChangeUniversalInstance(GetUniversalTable(),INST(i1),INST(i2));
    DestroyInstance(INST(i1),NULL);
    return INST(i2);
  default:
    assert(MoreRefined(i1->desc,i2->desc)==NULL);
    BadMerge(ASCERR,"Unconformable ARE_THE_SAME.\n",INST(i1),INST(i2),"");
    DifferentVersionCheck(i1->desc,i2->desc);
    return NULL;
  }
}

static
struct Instance *MergeRelations(struct RelationInstance *i1,
				struct RelationInstance *i2)
{
  if (i1->desc==i2->desc){
    switch(KeepWhichInstance(i1->desc,i2->desc,INST(i1),INST(i2))){
    case 1:
      if (InterfacePtrATS!=NULL) {
	(*InterfacePtrATS)(INST(i1),INST(i2));
      }
      /* add check to make sure equations are equal.
       * A sufficient check is that they have the same
       * union *share in the struct relation. For
       * token relations, a sufficient check is that
       * token arrays be the same size and varlists
       * be the same size. This is uncheckable at
       * present because the merge of relations and
       * whens possibly occurs before the merge of 
       * locally defined variables and sets.
       * To ensure correctness, we must merge all the
       * local variables and sets successfully, then
       * relations may be merged without even checking
       * because if two relations had an incompatibility
       * it would have showed up in the vars/sets checking.
       */
      MergeChildrenValues(INST(i1),INST(i2));
      MergeParents(INST(i1),INST(i2));
      FixLogRelations(INST(i1),INST(i2));
      FixWhens(INST(i1),INST(i2));
      DestroyInstance(INST(i2),NULL);
      return INST(i1);
    case 2:
      if (InterfacePtrATS!=NULL) {
	(*InterfacePtrATS)(INST(i2),INST(i1));
      }
      /* add check to make sure equations are equal */
      MergeChildrenValues(INST(i2),INST(i1));
      MergeParents(INST(i2),INST(i1));
      FixLogRelations(INST(i2),INST(i1));
      FixWhens(INST(i2),INST(i1));
      DestroyInstance(INST(i1),NULL);
      return INST(i2);
    default:
      BadMerge(ASCERR,"Unconformable ARE_THE_SAME of relations!\n",
               INST(i1), INST(i2),"");
      DifferentVersionCheck(i1->desc,i2->desc);
      return NULL;
    }
  }
  else{
    BadMerge(ASCERR,"Unconformable ARE_THE_SAME of relations!\n",
               INST(i1), INST(i2),"");
    DifferentVersionCheck(i1->desc,i2->desc);
    return NULL;
  }
}

static
struct Instance *MergeLogRelations(struct LogRelInstance *i1,
				   struct LogRelInstance *i2)
{
  if (i1->desc==i2->desc){
    switch(KeepWhichInstance(i1->desc,i2->desc,INST(i1),INST(i2))){
    case 1:
      if (InterfacePtrATS!=NULL) {
	(*InterfacePtrATS)(INST(i1),INST(i2));
      }
      /* add check to make sure logical equations are equal */
      MergeChildrenValues(INST(i1),INST(i2));
      MergeParents(INST(i1),INST(i2));
      FixLogRelations(INST(i1),INST(i2));
      FixWhens(INST(i1),INST(i2));
      DestroyInstance(INST(i2),NULL);
      return INST(i1);
    case 2:
      if (InterfacePtrATS!=NULL) {
	(*InterfacePtrATS)(INST(i2),INST(i1));
      }
      /* add check to make sure logical equations are equal */
      MergeChildrenValues(INST(i2),INST(i1));
      MergeParents(INST(i2),INST(i1));
      FixLogRelations(INST(i2),INST(i1));
      FixWhens(INST(i2),INST(i1));
      DestroyInstance(INST(i1),NULL);
      return INST(i2);
    default:
      BadMerge(ASCERR,"Unconformable ARE_THE_SAME of logical relations!\n",
               INST(i1), INST(i2),"");
      DifferentVersionCheck(i1->desc,i2->desc);
      return NULL;
    }
  } else {
    BadMerge(ASCERR,"Unconformable ARE_THE_SAME of logical relations!\n",
               INST(i1), INST(i2),"");
    DifferentVersionCheck(i1->desc,i2->desc);
    return NULL;
  }
}

static
struct Instance *MergeWhens(struct WhenInstance *i1,
			    struct WhenInstance *i2)
{
  if (i1->desc==i2->desc){
    switch(KeepWhichInstance(i1->desc,i2->desc,INST(i1),INST(i2))){
    case 1:
      if (InterfacePtrATS!=NULL) {
	(*InterfacePtrATS)(INST(i1),INST(i2));
      }
      MergeParents(INST(i1),INST(i2));
      FixWhens(INST(i1),INST(i2));
      DestroyInstance(INST(i2),NULL);
      return INST(i1);
    case 2:
      if (InterfacePtrATS!=NULL) {
	(*InterfacePtrATS)(INST(i2),INST(i1));
      }
      MergeParents(INST(i2),INST(i1));
      FixWhens(INST(i2),INST(i1));
      DestroyInstance(INST(i1),NULL);
      return INST(i2);
    default:
      BadMerge(ASCERR,"Unconformable ARE_THE_SAME of whens!\n",
               INST(i1),INST(i2),"");
      DifferentVersionCheck(i1->desc,i2->desc);
      return NULL;
    }
  } else {
    BadMerge(ASCERR,"Unconformable ARE_THE_SAME of whens!\n",
             INST(i1),INST(i2),"");
    DifferentVersionCheck(i1->desc,i2->desc);
    return NULL;
  }
}


static
int CheckArrayChildren(struct gl_list_t *l1,
		       struct gl_list_t *l2,
		       enum inst_t t)
{
  register unsigned long c,len;
  register struct ArrayChild *child1,*child2;
  if ((l1==NULL)||(l2==NULL)) return 0;
  len = gl_length(l1);
  if (gl_length(l2)!=len) return 1;
  for(c=1;c<=len;c++){
    child1 = (struct ArrayChild *)gl_fetch(l1,c);
    child2 = (struct ArrayChild *)gl_fetch(l2,c);
    if (t == ARRAY_INT_INST){
      if (CmpIntIndex(child1,child2)!=0) {
        return 1;
      }
    } else {
      if (CmpStrIndex(child1,child2)!=0) {
        return 1;
      }
    }
  }
  return 0;
}

static
void MergeArrayChildren(struct ArrayInstance *i1, struct ArrayInstance *i2)
{
  unsigned long c,len;
  struct ArrayChild *ptr1,*ptr2;
  if ((i1->children != NULL)&&(i2->children != NULL)){
    len = gl_length(i1->children);
    for(c=1;c<=len;c++){
      ptr1 = (struct ArrayChild *)gl_fetch(i1->children,c);
      ptr2 = (struct ArrayChild *)gl_fetch(i2->children,c);
      (void)RecursiveMergeInstance(ptr1->inst,ptr2->inst);
    }
  } else {
    if (i2->children != NULL){
      i1->children = i2->children;
      i2->children = NULL;
      len = gl_length(i1->children);
      for(c=1;c<=len;c++){
        ptr1 = (struct ArrayChild *)gl_fetch(i1->children,c);
        RemoveParent(ptr1->inst,INST(i2));
        if (SearchForParent(ptr1->inst,INST(i1))==0) {
	  AddParent(ptr1->inst,INST(i1));
        }
      }
    }
  }
}

static
struct Instance *MergeArrays(struct ArrayInstance *i1,
			     struct ArrayInstance *i2)
{
  assert(i1&&i2&&i1->t==i2->t);
  if ((i1->desc == i2->desc)&&(i1->indirected == i2->indirected)){
    if (CheckArrayChildren(i1->children,i2->children,i1->t)){
      ASC_PANIC("Arrays have different children.\n");/*NOTREACHED*/
    }
    switch (KeepWhichInstance(i1->desc,i2->desc,INST(i1),INST(i2))){
    case 1:
      MergeArrayChildren(i1,i2);
      MergeParents(INST(i1),INST(i2));
      DestroyInstance(INST(i2),NULL);
      return INST(i1);
    case 2:
      MergeArrayChildren(i2,i1);
      MergeParents(INST(i2),INST(i1));
      DestroyInstance(INST(i1),NULL);
      return INST(i2);
    default:
      Asc_Panic(2, NULL,
                "Bizarre error that should never occur.\n");/*NOTREACHED*/
    }
  } else {
    ASC_PANIC("Unconformable arrays.\n");/*NOTREACHED*/
  }
  exit(2);/* NOT REACHED.  Needed to keep gcc from whining */
}

static
void CheckClique(struct Instance *i)
{
  struct TypeDescription *type;
  struct Instance *ptr;
  if (i!=NULL){
    ptr = i;
    type = InstanceTypeDesc(i);
    while((ptr=NextCliqueMember(ptr)) != i){
      if (InstanceTypeDesc(ptr)!=type) {
        /* NULL is correct arginst because parameterized
         * instances are never in cliques with other than
         * themselves.
         */
	ptr = RefineInstance(ptr,type,NULL);
      }
    }
  }
}

/* basically checks arealikes recursively after merge */
void PostMergeCheck(struct Instance *i)
{
  /* This can't use VisitInstanceTree because it could be called recursively
   * by RefineInstance when it refines a universal type
   */
  unsigned long nc,c;
  struct Instance *child;
  if (i==NULL) return;
  AssertMemory(i);
  CheckClique(i);
  if (NotAtom(i)){ /* wrong -- atoms and constants can have cliques. fix me */
    nc = NumberChildren(i);
    for(c=1;c<=nc;c++) {
      child = InstanceChild(i,c);
      if (child != NULL) {
	PostMergeCheck(child);
      }
    }
  }
}

static
struct Instance *RecursiveMergeInstance(struct Instance *i1,
					struct Instance *i2)
{
  struct Instance *result;
  assert(i1&&i2);
  if (i1==i2) {
    return i1;
  }
  if ( i1->t == i2->t ) {
    switch( i1->t ) {
    case MODEL_INST:
      result = MergeModels(MOD_INST(i1),MOD_INST(i2));
      break;
    case REAL_CONSTANT_INST:
    case BOOLEAN_CONSTANT_INST:
    case INTEGER_CONSTANT_INST:
    case SYMBOL_CONSTANT_INST:
      result = MergeConstants(i1,i2);
      break;
    case REAL_ATOM_INST:
    case BOOLEAN_ATOM_INST:
    case INTEGER_ATOM_INST:
    case SET_ATOM_INST:
    case SYMBOL_ATOM_INST:
      result = MergeAtoms(i1,i2);
      break;
    case REL_INST:
      result = MergeRelations(RELN_INST(i1),RELN_INST(i2));
      break;
    case LREL_INST:
      result = MergeLogRelations(LRELN_INST(i1),LRELN_INST(i2));
      break;
    case WHEN_INST:
      result = MergeWhens(W_INST(i1),W_INST(i2));
      break;
    case ARRAY_INT_INST:
    case ARRAY_ENUM_INST:
      result = MergeArrays(ARY_INST(i1),ARY_INST(i2));
      break;
    case REAL_INST:
    case INTEGER_INST:
    case BOOLEAN_INST:
    case SET_INST:
    case SYMBOL_INST:
      FPRINTF(ASCERR,"Attempt to merge fundamental instance type.\n");
      FPRINTF(ASCERR,"This operation is not allowed.\n");
      result = NULL;
      break;
    default:
      Asc_Panic(2, NULL,
                "Unknown instance type passed to RecursiveMergeInstance.\n");
	  result = NULL;
      break;
    }
    /*  CheckClique(result); moved to a PostMergeCheck */
    return result;
  } else {
    ASC_PANIC("Attempt to merge unconformable types in children.\n");
    
  }
}

struct Instance *MergeInstances(struct Instance *i1, struct Instance *i2)
{
  assert(i1&&i2);
  if (i1==i2) return i1;
  AssertMemory(i1);
  AssertMemory(i2);
  if (InstanceKind(i1)==InstanceKind(i2)){
    if (InstanceKind(i1)==MODEL_INST) {
      if (GetModelParameterCount(InstanceTypeDesc(i1)) != 0 ||
          GetModelParameterCount(InstanceTypeDesc(i2)) != 0 ) {
        /* We need to relax this for == types where i1,i2 have
         * exactly equal arguments, typewise.
         */
        BadMerge(ASCERR,"Attempt to merge parameterized types.\n",
	        INST(i1), INST(i2), "Both instances remain unchanged.\n");
        return NULL;
      }
    }
    if (MoreRefined(InstanceTypeDesc(i1),InstanceTypeDesc(i2))!=NULL) {
      return RecursiveMergeInstance(i1,i2);
    } else {
      BadMerge(ASCERR,"Attempt to merge unconformable types.\n",
               INST(i1),INST(i2), "Both instances remain unchanged.\n");
      DifferentVersionCheck(InstanceTypeDesc(i1),
			    InstanceTypeDesc(i2));
      return NULL;
    }
  } else{
    ReallyBadMerge(ASCERR,"Attempt to merge very unconformable types.\n",
             INST(i1),INST(i2), "Both instances remain unchanged.\n");
    return NULL;
  }
}

