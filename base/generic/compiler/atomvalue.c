/*
 *  Ascend Instance Value Functions
 *  by Tom Epperly & Ben Allan
 *  9/3/89
 *  Version: $Revision: 1.18 $
 *  Version control file: $RCSfile: atomvalue.c,v $
 *  Date last modified: $Date: 1998/03/26 20:39:31 $
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
#include <general/list.h>
#include <general/dstring.h>
#include "compiler.h"
#include "symtab.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "expr_types.h"
#include "child.h"
#include "type_desc.h"
#include "type_descio.h"
#include "instance_enum.h"
#include "instance_name.h"
#include "instance_io.h"
#include "instmacro.h"
#include "instquery.h"
#include "setinstval.h"
#include "setinst_io.h"
#include "instance_types.h"
#include "cmpfunc.h"
#include "atomvalue.h"

#ifndef lint
static CONST char AtomValueModuleID[] = "$Id: atomvalue.c,v 1.18 1998/03/26 20:39:31 ballan Exp $";
#endif

unsigned AtomAssigned(CONST struct Instance *i)
{
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
    /* used to be exit(2) */
    /*NOTREACHED*/
  }
}

unsigned AtomMutable(CONST struct Instance *i)
{
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
    FPRINTF(ASCERR,"AtomMutable called on non-atomic instance.\n");
    return 0; /* not atomic, so can't very well be mutable, eh? */
    /* used to be exit(2) */
    /*NOTREACHED*/
  }
}

unsigned DepthAssigned(CONST struct Instance *i)
{
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
    Asc_Panic(2, NULL, "Incorrect type passed to DepthAssigned.\n");
    exit(2);/* Needed to keep gcc from whining */
  }
}

double RealAtomValue(CONST struct Instance *i)
{
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
    Asc_Panic(2, NULL, "RealAtomValue called on non-real instance.\n");
    exit(2);/* Needed to keep gcc from whining */
  }
}

void SetRealAtomValue(struct Instance *i, double d, unsigned int depth)
{
  assert(i!=NULL);
  AssertMemory(i);
  switch(i->t) {
  case REAL_CONSTANT_INST:
    if (AtomAssigned(i)) {
      ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"SetRealAtomValue called on Constant instance.");
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
    RA_INST(i)->assigned++;
    RA_INST(i)->value = d;
    RA_INST(i)->depth = depth;
    break;
  default:
    Asc_Panic(2, NULL, "SetRealAtomValue called on non-real instance.\n");
  }
}

void SetRealAtomDims(struct Instance *i, CONST dim_type *dim)
{
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
    Asc_Panic(2, NULL, "SetRealAtomDims called on non-real instance.\n");
  }
}

CONST dim_type *RealAtomDims(CONST struct Instance *i)
{
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
    Asc_Panic(2, NULL, "RealAtomDims called on non-real instance.\n");
    exit(2);/* Needed to keep gcc from whining */
  }
}

long GetIntegerAtomValue(CONST struct Instance *i)
{
  assert(i!=NULL);
  AssertMemory(i);
  switch(i->t) {
  case INTEGER_INST:
    return I_INST(i)->value;
  case INTEGER_ATOM_INST:
    return IA_INST(i)->value;
  case INTEGER_CONSTANT_INST:
    if (!CIASS(i)) {
      FPRINTF(ASCERR,
	      "Warning GetIntegerAtomValue called on unassigned constant.\n");
    }
    return IC_INST(i)->value;
  default:
    Asc_Panic(2, "GetIntegerAtomValue",
              "GetIntegerAtomValue called on non-integer instance.\n");
    exit(2);/* Needed to keep gcc from whining */
  }
}

void SetIntegerAtomValue(struct Instance *i, long int v,unsigned d)
{
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
      FPRINTF(ASCERR,"SetIntegerAtomValue called on Constant instance.\n");
    } else {
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
    Asc_Panic(2, NULL, "SetIntegerAtomValue called on non-integer.\n");
  }
}

int GetBooleanAtomValue(CONST struct Instance *i)
{
  assert(i!=NULL);
  AssertMemory(i);
  switch(i->t) {
  case BOOLEAN_INST:
    return B_INST(i)->value;
  case BOOLEAN_ATOM_INST:
    return BA_INST(i)->value;
  case BOOLEAN_CONSTANT_INST:
    if (!CIASS(i)) {
      FPRINTF(ASCERR,
	      "Warning GetBooleanAtomValue called on unassigned constant.\n");
    }
    return BCV(i);
  default:
    Asc_Panic(2, "GetBooleanAtomValue",
              "GetBooleanAtomValue called on non-boolean instance.\n");
    exit(2);/* Needed to keep gcc from whining */
  }
}

void SetBooleanAtomValue(struct Instance *i, int truth, unsigned int depth)
{
  assert(i!=NULL);
  AssertMemory(i);
  switch(i->t) {
  case BOOLEAN_INST:
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
      FPRINTF(ASCERR,"SetBooleanAtomValue called on Constant instance.\n");
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
    Asc_Panic(2, NULL, "SetBooleanAtomValue",
              "SetBooleanAtomValue called on non-boolean instance.\n");
  }
}

int AssignSetAtomList(struct Instance *i, struct set_t *list)
{
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
      FPRINTF(ASCERR,"AssignSetAtomList called on fixed set instance.\n");
      if (SetsEqual(list,SA_INST(i)->list)){ /* benign assignment */
	FPRINTF(ASCERR,
          "The assignment is benign(assigns the same value), %s %s.\n",
          "so the program can continue\nrunning.  Report this message to",
          ASC_MILD_BUGMAIL);
        FPRINTF(ASCERR,"and or stop writing buggy models.\n");
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
      FPRINTF(ASCERR,"AssignSetAtomList called with mismatching set:\n");
      WriteInstSet(ASCERR,list);
      FPRINTF(ASCERR,"\n");
      return 0;
    }
    if (S_INST(i)->list != NULL) {
      FPRINTF(ASCERR,"AssignSetAtomList called on fixed set instance.\n");
      if (SetsEqual(list,S_INST(i)->list)){ /* benign assignment */
	FPRINTF(ASCERR,
          "The assignment is benign(assigns the same value), %s %s.\n",
          "so the program can continue\nrunning.  Report this message to",
          ASC_MILD_BUGMAIL);
        FPRINTF(ASCERR,"and or stop writing buggy models.\n");
	DestroySet(S_INST(i)->list);
      } else {
	return 0;
      }
    }
    S_INST(i)->list = list;
    return 1;
  default:
    FPRINTF(ASCERR,"AssignSetAtomList called on non-set instance.\n");
    return 0;
  }
}

CONST struct set_t *SetAtomList(CONST struct Instance *i)
{
  assert(i!=NULL);
  AssertMemory(i);
  switch (i->t) {
  case SET_INST:
    return S_INST(i)->list;
  case SET_ATOM_INST:
    return SA_INST(i)->list;
  default:
    Asc_Panic(2, NULL, "SetAtomList called on non-set instance.\n");
    exit(2);/* Needed to keep gcc from whining */
  }
}

int GetSetAtomKind(CONST struct Instance *i)
{
  assert(i!=NULL);
  AssertMemory(i);
  switch (i->t) {
  case SET_INST:
    return (int)(S_INST(i)->int_set);
  case SET_ATOM_INST:
    return (int)(SA_INST(i)->int_set);
  default:
    Asc_Panic(2, NULL, "GetSetAtomKind called on non-set instance.\n");
    exit(2);/* Needed to keep gcc from whining */
  }
}

symchar *GetSymbolAtomValue(CONST struct Instance *i)
{
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
    Asc_Panic(2, NULL, "GetSymbolAtomValue called on non-symbol instance.\n");
    exit(2);/* Needed to keep gcc from whining */
  }
}

/*
 * 02/26/97.
 * Eliminating the restriction of assigning only once a SYMBOL_INST and
 * SYMBOL_ATOM_INST. Actually, it was only disabled with an if. VRR.
 */

void SetSymbolAtomValue(struct Instance *i, symchar *str)
{
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
    Asc_Panic(2, NULL, "SetSymbolAtomValue called on non-symbol instance.\n");
  }
}

int CmpAtomValues(CONST struct Instance *i1, CONST struct Instance *i2)
{
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
    FPRINTF(ASCERR,"CmpAtomValues called with mismatched ATOM types\n");
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
    Asc_Panic(2, NULL, "Bad call to CmpAtomValues!\n");
    exit(2);/* Needed to keep gcc from whining */
    break;
  }
}
