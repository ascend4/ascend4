/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1996 Ben Allan
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*//**
	@file
	Ascend Atomic-Instance Value Functions
*//*
	by Tom Epperly & Ben Allan
	based on instance.c
	9/3/89
	Last in CVS: $Revision: 1.18 $ $Date: 1998/03/26 20:39:31 $ $Author: ballan $
*/

#include <stdarg.h>
#include <utilities/ascConfig.h>
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/dstring.h>

#include "symtab.h"


#include "functype.h"
#include "expr_types.h"
#include "child.h"
#include "type_desc.h"
#include "type_descio.h"
#include "instance_name.h"
#include "instance_io.h"
#include "instmacro.h"
#include "instquery.h"
#include "setinstval.h"
#include "setinst_io.h"
#include "instance_types.h"
#include "cmpfunc.h"
#include "atomvalue.h"

unsigned AtomAssigned(CONST struct Instance *i){
  assert(i!=NULL);
  AssertMemory(i);
  if (i->t & ICONS) {
    return CIASS(i);
  }
  switch(i->t) {
  /* int */
  case INTEGER_INST: return I_INST(i)->assigned;
  case INTEGER_ATOM_INST: return IA_INST(i)->assigned;
  /* real */
  case REAL_INST: return R_INST(i)->assigned;
  case REAL_ATOM_INST: return RA_INST(i)->assigned;
  /* set */
  case SET_INST: return (S_INST(i)->list!=NULL) ? 1 : 0;
  case SET_ATOM_INST: return (SA_INST(i)->list!=NULL) ? 1 : 0;
  /* symbol */
  case SYMBOL_INST: return (SYM_INST(i)->value!=NULL) ? 1 : 0;
  case SYMBOL_ATOM_INST: return (SYMA_INST(i)->value!=NULL) ? 1 : 0;
  /* boolean */
  case BOOLEAN_INST: return B_INST(i)->assigned;
  case BOOLEAN_ATOM_INST: return BA_INST(i)->assigned;
  default:
    ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"AtomAssigned called on non-atomic instance.");
    return 0; /* not atomic, so can't very well be assigned, eh? */
  }
}

unsigned AtomMutable(CONST struct Instance *i){
  assert(i!=NULL);
  AssertMemory(i);
  if (i->t & ICONS) {
    return !CIASS(i);
  }
  switch(i->t) {
  case INTEGER_INST: return 1; /* always */
  case INTEGER_ATOM_INST: return 1; /*always*/
  case REAL_INST: return 1; /* always */
  case REAL_ATOM_INST: return 1; /* always */
  case SET_INST: return (S_INST(i)->list==NULL) ? 1 : 0;
  case SET_ATOM_INST: return (SA_INST(i)->list==NULL) ? 1 : 0;
  case SYMBOL_INST: return (SYM_INST(i)->value==NULL) ? 1 : 0;
  case SYMBOL_ATOM_INST: return (SYMA_INST(i)->value==NULL) ? 1 : 0;
  case BOOLEAN_INST: return 1; /* always */
  case BOOLEAN_ATOM_INST: return 1; /* always */
  default:
    ERROR_REPORTER_HERE(ASC_PROG_ERROR,"AtomMutable called on non-atomic instance.\n");
    return 0; /* not atomic, so can't very well be mutable, eh? */
  }
}

unsigned DepthAssigned(CONST struct Instance *i){
  assert(i!=NULL);
  AssertMemory(i);
  switch(i->t){
  case REAL_ATOM_INST:
    return RA_INST(i)->depth;
  case REAL_INST:
    return R_INST(i)->depth;
  case BOOLEAN_ATOM_INST:
    return BA_INST(i)->depth;
  case BOOLEAN_INST:
    return B_INST(i)->depth;
  default:
    ASC_PANIC("Incorrect type passed");
    
  }
}

double RealAtomValue(CONST struct Instance *i){
  assert(i!=NULL);
  AssertMemory(i);
  switch(i->t) {
  case REAL_INST:
    return R_INST(i)->value;
  case REAL_CONSTANT_INST:
    return RC_INST(i)->value;
  case REAL_ATOM_INST:
    return RA_INST(i)->value;
  default:
    ASC_PANIC("called with non-real instance");
  }
}

void SetRealAtomValue(struct Instance *i, double d, unsigned int depth){
  assert(i!=NULL);
  AssertMemory(i);
  
  switch(i->t) {
  case REAL_CONSTANT_INST:
    if (AtomAssigned(i)) {
      ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"SetRealAtomValue called on a constant instance.");
    } else {
      struct Instance *ptr;

      RC_INST(i)->value = d;
      RC_INST(i)->vflag |= ci_ASSIGNED;
      /* assign rest of clique */
      ptr = i;
      while ((ptr=NextCliqueMember(ptr))!=i){
        assert(ptr!=NULL);
        RC_INST(i)->value = d;
        RC_INST(i)->vflag |= ci_ASSIGNED;
      }
    }
    break;
  case REAL_INST:
    R_INST(i)->assigned++;
    R_INST(i)->value = d;
    R_INST(i)->depth = depth;
    break;
  case REAL_ATOM_INST:
	/* CONSOLE_DEBUG("SETTING REAL ATOM INSTANCE %p TO VALUE %f, DEPTH %u (WAS %f)",i,d,depth,RA_INST(i)->value); */
    RA_INST(i)->assigned++;
    RA_INST(i)->value = d;
    RA_INST(i)->depth = depth;
    break;
  default:
    ASC_PANIC("called on non-real instance.\n");
  }
}

void SetRealAtomDims(struct Instance *i, CONST dim_type *dim){
  assert(i!=NULL);
  AssertMemory(i);
  switch(i->t) {
  case REAL_CONSTANT_INST:
    if ( IsWild(RC_INST(i)->dimen) ) {
      struct Instance *ptr;

      RC_INST(i)->dimen = dim;
      /* assign rest of clique */
      ptr = i;
      while ((ptr=NextCliqueMember(ptr))!=i){
	assert(ptr!=NULL);
	RC_INST(ptr)->dimen = dim;
      }
    } else {
      ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"SetRealAtomDims called on dimensioned constant.");
    }
    break;
  case REAL_INST:
    R_INST(i)->dimen = dim;
    break;
  case REAL_ATOM_INST:
    RA_INST(i)->dimen = dim;
    break;
  default:
    ASC_PANIC("called on non-real instance.");
  }
}

CONST dim_type *RealAtomDims(CONST struct Instance *i){
  assert(i!=NULL);
  AssertMemory(i);
  switch(i->t) {
  case REAL_INST:
    return R_INST(i)->dimen;
  case REAL_CONSTANT_INST:
    return RC_INST(i)->dimen;
  case REAL_ATOM_INST:
    return RA_INST(i)->dimen;
  default:
    ASC_PANIC("called on non-real instance."); 
  }
}

long GetIntegerAtomValue(CONST struct Instance *i){
  assert(i!=NULL);
  AssertMemory(i);
  switch(i->t) {
  case INTEGER_INST:
    return I_INST(i)->value;
  case INTEGER_ATOM_INST:
    return IA_INST(i)->value;
  case INTEGER_CONSTANT_INST:
    if (!CIASS(i)) {
      ERROR_REPORTER_HERE(ASC_PROG_WARNING,"GetIntegerAtomValue called on unassigned constant.");
    }
    return IC_INST(i)->value;
  default:
    ASC_PANIC("called on non-integer instance");
  }
}

void SetIntegerAtomValue(struct Instance *i, long int v,unsigned d){
  assert(i!=NULL);
  AssertMemory(i);
  switch(i->t) {
  case INTEGER_ATOM_INST:
    IA_INST(i)->value = v;
    IA_INST(i)->assigned++;
    IA_INST(i)->depth = d;
    break;
  case INTEGER_INST:
    I_INST(i)->value = v;
    I_INST(i)->assigned++;
    I_INST(i)->depth = d;
    break;
  case INTEGER_CONSTANT_INST:
    if (CIASS(i)) {
      ERROR_REPORTER_HERE(ASC_PROG_ERROR,"SetIntegerAtomValue called on Constant instance.");
    }else{
      struct Instance *ptr;

      IC_INST(i)->value = v;
      IC_INST(i)->vflag |= ci_ASSIGNED;
      /* assign rest of clique */
      ptr = i;
      while ((ptr=NextCliqueMember(ptr))!=i){
        assert(ptr!=NULL);
        IC_INST(i)->value = v;
        IC_INST(i)->vflag |= ci_ASSIGNED;
      }
    }
    break;
  default:
    ASC_PANIC("SetIntegerAtomValue called on non-integer.\n");
  }
}

int GetBooleanAtomValue(CONST struct Instance *i){
  assert(i!=NULL);
  AssertMemory(i);
  switch(i->t) {
  case BOOLEAN_INST:
    return B_INST(i)->value;
  case BOOLEAN_ATOM_INST:
    return BA_INST(i)->value;
  case BOOLEAN_CONSTANT_INST:
    if (!CIASS(i)) {
      ERROR_REPORTER_HERE(ASC_PROG_WARNING,"GetBooleanAtomValue called on unassigned constant.");
    }
    return BCV(i);
  default:
    ASC_PANIC("called on non-boolean instance."); 
  }
}

void SetBooleanAtomValue(struct Instance *i, int truth, unsigned int depth){
  assert(i!=NULL);
  AssertMemory(i);
  switch(i->t) {
  case BOOLEAN_INST:
	/* CONSOLE_DEBUG("SETTING BOOLEAN INSTANCE %p TO VALUE %d, DEPTH %u (WAS %d)",i,truth?1:0,depth,B_INST(i)->value); */
    B_INST(i)->value = truth ? 1 : 0;
    B_INST(i)->assigned++;
    B_INST(i)->depth = depth;
    break;
  case BOOLEAN_ATOM_INST:
    BA_INST(i)->value = truth ? 1 : 0;
    BA_INST(i)->assigned++;
    BA_INST(i)->depth = depth;
    break;
  case BOOLEAN_CONSTANT_INST:
    if (CIASS(i)) {
      ERROR_REPORTER_HERE(ASC_PROG_ERROR,"SetBooleanAtomValue called on Constant instance.");
    } else {
      struct Instance *ptr;

      if (truth)  {
        BC_INST(i)->vflag |= (ci_BVAL | ci_ASSIGNED);
      } else {
        /* never assigned, and inited to FALSE, so turn on assigned flag*/
        BC_INST(i)->vflag |= ci_ASSIGNED;
      }
      /* assign rest of clique */
      ptr = i;
      while ((ptr=NextCliqueMember(ptr))!=i){
        assert(ptr!=NULL);
        if (truth)  {
          BC_INST(i)->vflag |= (ci_BVAL | ci_ASSIGNED);
        } else {
          /* never assigned, and inited to FALSE, so turn on assigned flag*/
          BC_INST(i)->vflag |= ci_ASSIGNED;
        }
      }
    }
    break;
  default:
    ASC_PANIC("called on non-boolean instance.\n");
  }
}

int AssignSetAtomList(struct Instance *i, struct set_t *list){
  assert(i!=NULL);
  AssertMemory(i);
  switch(i->t) {
  case SET_ATOM_INST:
    if ((SetKind(list)!=empty_set)&&
	((SA_INST(i)->int_set==1)!=(SetKind(list)==integer_set))){
      FPRINTF(ASCERR,"AssignSetAtomList called with mismatching set:\n");
      WriteInstSet(ASCERR,list);
      FPRINTF(ASCERR,"for set OF %s\n",
        (SA_INST(i)->int_set==1) ?
        SCP(GetBaseTypeName(integer_constant_type)) :
        SCP(GetBaseTypeName(symbol_constant_type)) );
      return 0;
    }
    if (SA_INST(i)->list != NULL) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"AssignSetAtomList called on fixed set instance.");
      if (SetsEqual(list,SA_INST(i)->list)){ /* benign assignment */
        ERROR_REPORTER_HERE(ASC_PROG_NOTE,
			"The assignment is benign(assigns the same value) so the "
			" program can continue running."
		);
        DestroySet(SA_INST(i)->list);
      } else {
	return 0;
      }
    }
    SA_INST(i)->list = list;
    return 1;
  case SET_INST:
    if ((SetKind(list)!=empty_set)&&
	((S_INST(i)->int_set==1)!=(SetKind(list)==integer_set))){
      ERROR_REPORTER_START_HERE(ASC_PROG_ERR);
	  FPRINTF(ASCERR,"AssignSetAtomList called with mismatching set:\n");
      WriteInstSet(ASCERR,list);
      FPRINTF(ASCERR,"\n");
	  error_reporter_end_flush();
      return 0;
    }
    if (S_INST(i)->list != NULL) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"AssignSetAtomList called on fixed set instance.\n");
      if(SetsEqual(list,S_INST(i)->list)){
        /* benign assignment */
        ERROR_REPORTER_HERE(ASC_PROG_NOTE,
			"The assignment is benign(assigns the same value) so the "
			" program can continue running."
		);
	DestroySet(S_INST(i)->list);
      } else {
	return 0;
      }
    }
    S_INST(i)->list = list;
    return 1;
  default:
    ERROR_REPORTER_HERE(ASC_PROG_ERROR,"AssignSetAtomList called on non-set instance.");
    return 0;
  }
}

CONST struct set_t *SetAtomList(CONST struct Instance *i){
  assert(i!=NULL);
  AssertMemory(i);
  switch (i->t) {
  case SET_INST:
    return S_INST(i)->list;
  case SET_ATOM_INST:
    return SA_INST(i)->list;
  default:
    ASC_PANIC("called on non-set instance.");
  }
}

int GetSetAtomKind(CONST struct Instance *i){
  assert(i!=NULL);
  AssertMemory(i);
  switch (i->t) {
  case SET_INST:
    return (int)(S_INST(i)->int_set);
  case SET_ATOM_INST:
    return (int)(SA_INST(i)->int_set);
  default:
    ASC_PANIC("called on non-set instance.");
  }
}

symchar *GetSymbolAtomValue(CONST struct Instance *i){
  assert(i!=NULL);
  AssertMemory(i);
  switch(i->t){
  case SYMBOL_INST:
    return SYM_INST(i)->value;
  case SYMBOL_ATOM_INST:
    return SYMA_INST(i)->value;
  case SYMBOL_CONSTANT_INST:
    if (!CIASS(i)) {
      FPRINTF(ASCERR,
	      "Warning GetSymbolAtomValue called on unassigned constant.\n");
    }
    return SYMC_INST(i)->value;
  default:
    ASC_PANIC("called on non-symbol instance.");
  }
}

/*
 * 02/26/97.
 * Eliminating the restriction of assigning only once a SYMBOL_INST and
 * SYMBOL_ATOM_INST. Actually, it was only disabled with an if. VRR.
 */

void SetSymbolAtomValue(struct Instance *i, symchar *str){
  assert(i!=NULL);
  AssertMemory(i);
  assert(AscFindSymbol(str)!=NULL);
  switch(i->t){
  case SYMBOL_INST:
    SYM_INST(i)->value = str;
    break;
  case SYMBOL_ATOM_INST:
    SYMA_INST(i)->value = str;
    break;
  case SYMBOL_CONSTANT_INST:
    if (CIASS(i)) {
      FPRINTF(ASCERR,"SetSymbolAtomValue called on Constant instance.\n");
      FPRINTF(ASCERR,"Old symbol retained: %s.\n",SCP(SYMC_INST(i)->value));
      FPRINTF(ASCERR,"New symbol ignored: %s.\n",SCP(str));
    } else {
      struct Instance *ptr;

      SYMC_INST(i)->value = str;
      SYMC_INST(i)->vflag |= ci_ASSIGNED;
      /* assign rest of clique */
      ptr = i;
      while ((ptr=NextCliqueMember(ptr))!=i){
        assert(ptr!=NULL);
        SYMC_INST(i)->value = str;
      }
    }
    break;
  default:
    ASC_PANIC("called on non-symbol instance.");
  }
}

int CmpAtomValues(CONST struct Instance *i1, CONST struct Instance *i2){
  if (i1==i2) {
    return 0;
  }
  if (i1==NULL) {
    return -1;
  }
  if (i2 == NULL) {
    return 1;
  }
  if (InstanceKind(i1) != InstanceKind(i2)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERROR,"CmpAtomValues called with mismatched ATOM types");
    return (((int)InstanceKind(i1) - (int)InstanceKind(i2)) < 0) ? -1 : 1;
  }
  if (InstanceKind(i1) & ISET) {
    /* even unassigned sets have a kind that is effectively an instkind */
    if (GetSetAtomKind(i2) != GetSetAtomKind(i1)) {
      return (GetSetAtomKind(i2)-GetSetAtomKind(i1));
    }
  }
  if (AtomAssigned(i1) != AtomAssigned(i2)){
    if (AtomAssigned(i1)==0) {
      return -1;
    } else {
      return 1;
    }
  }
  switch (InstanceKind(i1)) {
  case SYMBOL_INST:
  case SYMBOL_ATOM_INST:
  case SYMBOL_CONSTANT_INST:
    return CmpSymchar(GetSymbolAtomValue(i1),GetSymbolAtomValue(i2));
  case REAL_INST:
  case REAL_ATOM_INST:
  case REAL_CONSTANT_INST:
    if (RealAtomValue(i1) != RealAtomValue(i2)) {
      return ( RealAtomValue(i1) < RealAtomValue(i2)) ? 1 : -1;
    } else {
      return CmpDimen(RealAtomDims(i1),RealAtomDims(i2));
    }
  case INTEGER_INST:
  case INTEGER_ATOM_INST:
  case INTEGER_CONSTANT_INST:
    if (GetIntegerAtomValue(i1) != GetIntegerAtomValue(i2)) {
      return ( GetIntegerAtomValue(i1) < GetIntegerAtomValue(i2)) ? 1 : -1;
    }
    return 0;
  case BOOLEAN_INST:
  case BOOLEAN_ATOM_INST:
  case BOOLEAN_CONSTANT_INST:
    if (GetBooleanAtomValue(i1) != GetBooleanAtomValue(i2)) {
      return ( GetBooleanAtomValue(i1) == 0) ? 1 : -1;
    }
    return 0;
  case SET_INST:
  case SET_ATOM_INST:
    return CmpSetInstVal(SetAtomList(i1),SetAtomList(i2));
  default:
    ASC_PANIC("Bad call!");
  }
}
