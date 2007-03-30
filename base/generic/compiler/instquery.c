/*
 *  Ascend Instance Tree Type Implementation
 *  by Tom Epperly
 *  9/3/89
 *  Version: $Revision: 1.19 $
 *  Version control file: $RCSfile: instquery.c,v $
 *  Date last modified: $Date: 1998/06/23 13:44:33 $
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
#include <general/list.h>
#include <general/pool.h>
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
#include "type_descio.h"
#include "prototype.h"
#include "pending.h"
#include "find.h"
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
#include "instance_types.h"
/* new */
#include "atomsize.h"
#include "atomvalue.h"
#include "cmpfunc.h"
#include "instmacro.h"
#include "parentchild.h"
#include "module.h"
#include "library.h"
#include "visitinst.h"
#include "instquery.h"

/**** GLOBAL VARIABLE SECTION ****/

void (*InterfacePtrATS)() = NULL;
void (*InterfacePtrDelete)() = NULL;
void (*InterfaceNotify)() = NULL;

/**** END GLOBAL VARIABLE SECTION ****/

enum inst_t InstanceKindF(CONST struct Instance *i)
{
  if (i==NULL || (i->t & IERRINST))
    return (ERROR_INST);
  return i->t;
}


unsigned long InstanceDepth(CONST struct Instance *i)
{
  register unsigned long c,result=0,d;
  register CONST struct Instance *parent;
  if (i==NULL) return 0;
  AssertMemory(i);
  for(c=NumberParents(i);c>0;c--){
    parent = InstanceParent(i,c);
    AssertMemory(parent);
    if ((d=InstanceDepth(parent))>result) result = d;
  }
  return result+1;
}

unsigned long InstanceShortDepth(CONST struct Instance *i)
{
  register unsigned long c,result=UINT_MAX,d;
  register CONST struct Instance *parent;
  if (i==NULL) return 0;
  AssertMemory(i);
  if (NumberParents(i)==0) return 1;
  for(c=NumberParents(i);c>0;c--){
    parent = InstanceParent(i,c);
    AssertMemory(parent);
    if ((d=InstanceShortDepth(parent))<result) result = d;
  }
  return result+1;
}

/*********************************************************************\
 SetNextCliqueMember(i,ptr)
 sets i->alike_ptr to ptr for types that have alike_ptrs.
 exits on types that don't or bad input.
 Instantiator use only!
\*********************************************************************/

void SetNextCliqueMember(struct Instance *i, struct Instance *next)
{
  AssertMemory(i);
  switch(i->t) {
  case MODEL_INST:
    MOD_INST(i)->alike_ptr = next;
    break;
  /* constants */
  case REAL_CONSTANT_INST:
    RC_INST(i)->alike_ptr = next;
    break;
  case BOOLEAN_CONSTANT_INST:
    BC_INST(i)->alike_ptr = next;
    break;
  case INTEGER_CONSTANT_INST:
    IC_INST(i)->alike_ptr = next;
    break;
  case SET_ATOM_INST:			/* should be a set constant */
    SA_INST(i)->alike_ptr = next;
    break;
  case SYMBOL_CONSTANT_INST:
    SYMC_INST(i)->alike_ptr = next;
    break;
  /* atoms */
  case REAL_ATOM_INST:
    RA_INST(i)->alike_ptr = next;
    break;
  case BOOLEAN_ATOM_INST:
    BA_INST(i)->alike_ptr = next;
    break;
  case INTEGER_ATOM_INST:
    IA_INST(i)->alike_ptr = next;
    break;
/* 					should be a list atom inst
  case SET_ATOM_INST:
    SA_INST(i)->alike_ptr = next;
    break;
*/
  case SYMBOL_ATOM_INST:
    SYMA_INST(i)->alike_ptr = next;
    break;
  case REL_INST:
    ASC_PANIC("Relations can only be ALIKE with themselves.\n");
  case LREL_INST:
    Asc_Panic(2, NULL,
              "Logical Relations can only be ALIKE with themselves.\n");
  case WHEN_INST:
    ASC_PANIC("WHEN's can only be ALIKE with themselves.\n");
  case SIM_INST:
    ASC_PANIC("Simulations can only be ALIKE with themselves.\n");
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    ASC_PANIC("Arrays can only be ALIKE with themselves.\n");
  /* fundamentals */
  case REAL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SET_INST:
  case SYMBOL_INST:
    ASC_PANIC("Fundamental reals cannot be made ARE_ALIKE.\n");
  case DUMMY_INST:
    ASC_PANIC("UNSELECTED instances cannot be made ARE_ALIKE.\n");
  default:
    ASC_PANIC("Illegal atom passed to SetNextCliqueMember.\n");
  }
}

struct Instance *NextCliqueMember(CONST struct Instance *i){
  AssertMemory(i);
  switch(i->t) {
  case MODEL_INST:
    return MOD_INST(i)->alike_ptr;
  /* constants */
  case REAL_CONSTANT_INST:
    return RC_INST(i)->alike_ptr;
  case BOOLEAN_CONSTANT_INST:
    return BC_INST(i)->alike_ptr;
  case INTEGER_CONSTANT_INST:
    return IC_INST(i)->alike_ptr;
  case SET_ATOM_INST:
    return SA_INST(i)->alike_ptr;
  case SYMBOL_CONSTANT_INST:
    return SYMC_INST(i)->alike_ptr;
  /* atoms */
  case REAL_ATOM_INST:
    return RA_INST(i)->alike_ptr;
  case BOOLEAN_ATOM_INST:
    return BA_INST(i)->alike_ptr;
  case INTEGER_ATOM_INST:
    return IA_INST(i)->alike_ptr;
/* 						should be list atom
  case SET_ATOM_INST:
    return SA_INST(i)->alike_ptr;
*/
  case SYMBOL_ATOM_INST:
    return SYMA_INST(i)->alike_ptr;
  case REL_INST:
  case LREL_INST:
  case WHEN_INST:
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
  /* fundies */
  case REAL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SET_INST:
  case SYMBOL_INST:
  case SIM_INST:
  case DUMMY_INST:
    return (struct Instance *)i;
  default:
    ASC_PANIC("Invalid arguments to NextCliqueMember.\n");
    
  }
}

VOIDPTR GetInterfacePtr(CONST struct Instance *i)
{
  if (i == NULL) {
    ASC_PANIC("NULL instance passed to GetInterfacePtr.\n");
  }
  AssertMemory(i);
  switch(i->t) {
  case DUMMY_INST: return D_INST(i)->interface_ptr;
  case MODEL_INST: return MOD_INST(i)->interface_ptr;
  case ARRAY_INT_INST:		/* fall through */
  case ARRAY_ENUM_INST:
    return ARY_INST(i)->interface_ptr;
  case REAL_ATOM_INST: return RA_INST(i)->interface_ptr;
  case BOOLEAN_ATOM_INST: return BA_INST(i)->interface_ptr;
  case INTEGER_ATOM_INST: return IA_INST(i)->interface_ptr;
  case SET_ATOM_INST: return SA_INST(i)->interface_ptr;
  case SYMBOL_ATOM_INST: return SYMA_INST(i)->interface_ptr;
  case REL_INST: return RELN_INST(i)->interface_ptr;
  case LREL_INST: return LRELN_INST(i)->interface_ptr;
  case WHEN_INST: return W_INST(i)->interface_ptr;
  case SIM_INST: return SIM_INST(i)->interface_ptr;
  /* constants */
  case INTEGER_CONSTANT_INST: 	/* fall through */
  case BOOLEAN_CONSTANT_INST: 	/* fall through */
  case SYMBOL_CONSTANT_INST:	/* fall through */
  case REAL_CONSTANT_INST:    return CI_INST(i)->interface_ptr;
  /* fundies */
  case REAL_INST:		/* fall through */
  case INTEGER_INST:		/* fall through */
  case BOOLEAN_INST:		/* fall through */
  case SET_INST:		/* fall through */
  case SYMBOL_INST:		/* fall through */
    /* FPRINTF(ASCERR,"Instance type does not possess an GetInterfacePtr."); */
	break;
  default:
    ASC_PANIC("Undefined instance type %d passed to GetInterfacePtr",i->t);
  } 
  return NULL;
}

void SetInterfacePtr(struct Instance *i, VOIDPTR c)
{
  AssertMemory(i);
  switch(i->t) {
  case DUMMY_INST:
    D_INST(i)->interface_ptr=c;
    break;
  case MODEL_INST:
    MOD_INST(i)->interface_ptr=c;
    break;
  case ARRAY_INT_INST: /* FALL THROUGH */
  case ARRAY_ENUM_INST:
    ARY_INST(i)->interface_ptr=c;
    break;
  case REAL_ATOM_INST:
    RA_INST(i)->interface_ptr=c;
    break;
  case BOOLEAN_ATOM_INST:
    BA_INST(i)->interface_ptr=c;
    break;
  case INTEGER_ATOM_INST:
    IA_INST(i)->interface_ptr=c;
    break;
  case SET_ATOM_INST:
    SA_INST(i)->interface_ptr=c;
    break;
  case SYMBOL_ATOM_INST:
    SYMA_INST(i)->interface_ptr=c;
    break;
  case REL_INST:
    RELN_INST(i)->interface_ptr=c;
    break;
  case LREL_INST:
    LRELN_INST(i)->interface_ptr=c;
    break;
  case WHEN_INST:
    W_INST(i)->interface_ptr=c;
    break;
  case SIM_INST:
    SIM_INST(i)->interface_ptr=c;
    break;
  /* constants */
  case INTEGER_CONSTANT_INST:	/* fall through */
  case BOOLEAN_CONSTANT_INST:	/* fall through */
  case SYMBOL_CONSTANT_INST:	/* fall through */
  case REAL_CONSTANT_INST:
    CI_INST(i)->interface_ptr=c;
    break;
  case REAL_INST:		/* fall through */
  case INTEGER_INST:		/* fall through */
  case BOOLEAN_INST:		/* fall through */
  case SET_INST:		/* fall through */
  case SYMBOL_INST:		/* fall through */
    FPRINTF(ASCERR,"Instance type does not possess an InterfacePtr.");
	return;
  default:
    ASC_PANIC("Undefined instance type passed to InterfacePtr.\n");
  }
}

unsigned int GetAnonFlags(CONST struct Instance *i)
{
  AssertMemory(i);
  switch(i->t) {
  case SIM_INST:
    return SIM_INST(i)->anon_flags;
  case DUMMY_INST:
    return D_INST(i)->anon_flags;
  case MODEL_INST:
    return MOD_INST(i)->anon_flags;
  case REAL_ATOM_INST: /* fall through */
  case BOOLEAN_ATOM_INST: /* fall through */
  case INTEGER_ATOM_INST: /* fall through */
  case SET_ATOM_INST: /* fall through */
  case SYMBOL_ATOM_INST:
    return CA_INST(i)->anon_flags;
  case REL_INST:
    return RELN_INST(i)->anon_flags;
  case LREL_INST:
    return LRELN_INST(i)->anon_flags;
  case WHEN_INST:
    return W_INST(i)->anon_flags;
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    return ARY_INST(i)->anon_flags;
  /* constants */
  case REAL_CONSTANT_INST:	/* fall through */
  case INTEGER_CONSTANT_INST:	/* fall through */
  case BOOLEAN_CONSTANT_INST:	/* fall through */
  case SYMBOL_CONSTANT_INST:	/* fall through */
    return CI_INST(i)->anon_flags;
  /* fundies */
  case REAL_INST:		/* fall through */
  case INTEGER_INST:		/* fall through */
  case BOOLEAN_INST:		/* fall through */
  case SET_INST:		/* fall through */
  case SYMBOL_INST:		/* fall through */
    return 0;
  default:
    ASC_PANIC("Incorrect type in GetAnonFlags\n"); /*NOTREACHED*/
    
  }
}

void SetAnonFlags(struct Instance *i,unsigned int flags)
{
  AssertMemory(i);
  switch (i->t) {
  case SIM_INST:
    SIM_INST(i)->anon_flags = flags;
  case DUMMY_INST:
    D_INST(i)->anon_flags = flags;
    break;
  case MODEL_INST:
    MOD_INST(i)->anon_flags = flags;
    break;
  case REAL_ATOM_INST:		/* fall through */
  case BOOLEAN_ATOM_INST:	/* fall through */
  case INTEGER_ATOM_INST:	/* fall through */
  case SET_ATOM_INST:		/* fall through */
  case SYMBOL_ATOM_INST:
    CA_INST(i)->anon_flags = flags;
    break;
  case REL_INST:
    RELN_INST(i)->anon_flags = flags;
    break;
  case LREL_INST:
    LRELN_INST(i)->anon_flags = flags;
    break;
  case WHEN_INST:
    W_INST(i)->anon_flags = flags;
    break;
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    ARY_INST(i)->anon_flags = flags;
    break;
  /* constants */
  case REAL_CONSTANT_INST:	/* fall through */
  case INTEGER_CONSTANT_INST:	/* fall through */
  case BOOLEAN_CONSTANT_INST:	/* fall through */
  case SYMBOL_CONSTANT_INST:
    CI_INST(i)->anon_flags = flags;
  /* fundies */
  case REAL_INST:		/* fall through */
  case INTEGER_INST:		/* fall through */
  case BOOLEAN_INST:		/* fall through */
  case SET_INST:		/* fall through */
  case SYMBOL_INST:		/* fall through */
    break;
  default:
    ASC_PANIC("Incorrect atom type in SetAnonFlags.\n");
  }
}

symchar *GetSimulationName(struct Instance *i)
{
  assert(i&&InstanceKind(i)==SIM_INST);
  if (!(i&&InstanceKind(i)==SIM_INST)) {
    FPRINTF(ASCERR,"GetSimulationName called on non-simulation.\n");
  }
  return SIM_INST(i)->name;
}

static
void CollectSimulationInstances(struct Instance *i,struct gl_list_t *sims)
{
  unsigned len;
  assert(i!=NULL && sims != NULL);
  if (InstanceKind(i) == MODEL_INST) {
    for (len = NumberParents(i); len > 0 ; len--) {
      if (InstanceKind(InstanceParent(i,len)) == SIM_INST) {
        gl_append_ptr(sims,(VOIDPTR)InstanceParent(i,len));
      }
    }
  }
}

struct gl_list_t *FindSimulationAncestors(struct Instance *i)
{
  struct gl_list_t *sims;
  if (i==NULL) {
    return NULL;
  }
  sims = gl_create(7);
  SilentVisitInstanceRootsTwo(i,(VisitTwoProc)CollectSimulationInstances,
                              0,(VOIDPTR)sims);
  return sims;
}

struct Instance *GetSimulationRoot(struct Instance *i)
{
  struct Instance **child_adr;
  assert(i&&InstanceKind(i)==SIM_INST);
  child_adr = SIM_CHILD(i,0);
  return *child_adr;
}


/* Returns the number of bytes spent on a particular instance
 * Does not count relation guts, dimens, strings, or set_t *
 * as these are all potentially shared.
 */
unsigned long InstanceSize(CONST struct Instance *i)
{
  unsigned long len;

  assert(i!=NULL);
  AssertMemory(i);
  len = 0;
  if (i== NULL ||IsFundamentalInstance(i)) {
    return len;
  }
  /* no bytes for fundamentals, nulls */
  if (IsConstantInstance(i)) {
    len += ConstantByteLength(InstanceKind(i));
  }
  switch(i->t) {
  case SIM_INST:	/* list is a pointer in the structure */
    len += sizeof(struct SimulationInstance);
    return len;
  case DUMMY_INST:
    len += sizeof(struct GlobalDummyInstance);
    return len;
  case MODEL_INST:
    len += 2*sizeof(struct gl_list_t);
    len += sizeof(struct ModelInstance);
    len += BitListBytes(MOD_INST(i)->executed);
    len += gl_capacity(MOD_INST(i)->whens) * sizeof(void *);
    len += gl_capacity(MOD_INST(i)->parents)*sizeof(void *);
    len += ChildListLen(GetChildList(MOD_INST(i)->desc))*
           sizeof(struct Instance *);
    return len;
  case REAL_CONSTANT_INST:
    len += sizeof(struct gl_list_t);
    len += gl_capacity(RC_INST(i)->parents) * sizeof(void *);
    return len;
  case BOOLEAN_CONSTANT_INST:
    len += 2*sizeof(struct gl_list_t);
    len += gl_capacity(BC_INST(i)->parents) * sizeof(void *);
    len += gl_capacity(BC_INST(i)->whens) * sizeof(void *);
    return len;
  case INTEGER_CONSTANT_INST:
    len += 2*sizeof(struct gl_list_t);
    len += gl_capacity(IC_INST(i)->parents) * sizeof(void *);
    len += gl_capacity(IC_INST(i)->whens) * sizeof(void *);
    return len;
  case SYMBOL_CONSTANT_INST:
    len += 2*sizeof(struct gl_list_t);
    len += gl_capacity(SYMC_INST(i)->parents) * sizeof(void *);
    len += gl_capacity(SYMC_INST(i)->whens) * sizeof(void *);
    return len;
  case REAL_ATOM_INST:
    len += GetByteSize(InstanceTypeDesc(i));
    len += 2*sizeof(struct gl_list_t);
    len += (gl_capacity(RA_INST(i)->parents) +
            gl_capacity(RA_INST(i)->relations)) * sizeof(void *);
    return len;
  case BOOLEAN_ATOM_INST:
    len += 3 * sizeof(struct gl_list_t);
    len += gl_capacity(BA_INST(i)->logrelations) * sizeof(void *);
    len += gl_capacity(BA_INST(i)->whens) * sizeof(void *);
    len += GetByteSize(InstanceTypeDesc(i));
    len += gl_capacity(((struct CommonAtomInstance *)(i))->parents)
           * sizeof(void *);
    return len;
  case INTEGER_ATOM_INST:
    len += 2 * sizeof(struct gl_list_t);
    len += gl_capacity(IA_INST(i)->whens) * sizeof(void *);
    len += GetByteSize(InstanceTypeDesc(i));
    len += gl_capacity(((struct CommonAtomInstance *)(i))->parents)
           * sizeof(void *);
    return len;
  case SYMBOL_ATOM_INST:
    len += 2 *sizeof(struct gl_list_t);
    len += GetByteSize(InstanceTypeDesc(i));
    len += gl_capacity(SYMA_INST(i)->whens) * sizeof(void *);
    len += gl_capacity(((struct CommonAtomInstance *)(i))->parents)
           * sizeof(void *);
    return len;
  case SET_ATOM_INST:
    len += sizeof(struct gl_list_t);
    len += GetByteSize(InstanceTypeDesc(i));
    len += gl_capacity(((struct CommonAtomInstance *)(i))->parents)
           * sizeof(void *);
    return len;
  case REL_INST:
    len += 2*sizeof(struct gl_list_t);
    len += gl_capacity(RELN_INST(i)->whens) * sizeof(void *);
    len += gl_capacity(RELN_INST(i)->logrels) * sizeof(void *);
    len += GetByteSize(InstanceTypeDesc(i));
    return len;
  case LREL_INST:
    len += 2*sizeof(struct gl_list_t);
    len += gl_capacity(LRELN_INST(i)->whens) * sizeof(void *);
    len += gl_capacity(LRELN_INST(i)->logrels) * sizeof(void *);
    len += GetByteSize(InstanceTypeDesc(i));
    return len;
  case WHEN_INST:
    len += 3*sizeof(struct gl_list_t);
    len += gl_capacity(W_INST(i)->bvar) * sizeof(void *);
    len += gl_capacity(W_INST(i)->cases) * sizeof(void *);
    len += gl_capacity(W_INST(i)->whens) * sizeof(void *);
    len += GetByteSize(InstanceTypeDesc(i));
    return len;
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    len += sizeof(struct ArrayInstance);
    len += 2*sizeof(struct gl_list_t);
    len += gl_capacity(ARY_INST(i)->children);
    len += gl_capacity(ARY_INST(i)->parents);
    if (ARY_INST(i)->children!= NULL) {
      len += gl_length(ARY_INST(i)->children)*sizeof(struct ArrayChild);
    }
    return len;
  default:
    ASC_PANIC("Invalid argument to InstanceSize.\n");
    
  }
}

int IntegerSetInstance(CONST struct Instance *i)
{
  AssertMemory(i);
  switch(i->t) {
  case SET_ATOM_INST:
    return SA_INST(i)->int_set;
  case SET_INST:
    return S_INST(i)->int_set;
  default:
    ASC_PANIC("IntegerSetInstance called with non-set argument.\n");
    
  }
}


struct TypeDescription *InstanceTypeDesc(CONST struct Instance *i)
{
  assert(i!=NULL);
  AssertMemory(i);
  switch(i->t) {
  case SIM_INST:
    return SIM_INST(i)->desc;	/* SIM_INST -- same as root for the time */
  case MODEL_INST:
    return MOD_INST(i)->desc;
  case DUMMY_INST:
    return D_INST(i)->desc;
  case REAL_ATOM_INST:
    return RA_INST(i)->desc;
  case BOOLEAN_ATOM_INST:
    return BA_INST(i)->desc;
  case INTEGER_ATOM_INST:
    return IA_INST(i)->desc;
  case SET_ATOM_INST:
    return SA_INST(i)->desc;
  case SYMBOL_ATOM_INST:
    return SYMA_INST(i)->desc;
  case REAL_CONSTANT_INST:
    return RC_INST(i)->desc;
  case BOOLEAN_CONSTANT_INST:
    return BC_INST(i)->desc;
  case INTEGER_CONSTANT_INST:
    return IC_INST(i)->desc;
  case SYMBOL_CONSTANT_INST:
    return SYMC_INST(i)->desc;
  case REL_INST:
    return RELN_INST(i)->desc;
  case LREL_INST:
    return LRELN_INST(i)->desc;
  case WHEN_INST:
    return W_INST(i)->desc;
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    return ARY_INST(i)->desc;
  case REAL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SET_INST:
  case SYMBOL_INST:
    return NULL;
  default:
    ASC_PANIC("Unknown instance type passed to InstanceTypeDesc.\n");
    
  }
}

unsigned long InstanceIndirected(CONST struct Instance *i)
{
  assert(i!=NULL);
  if (InstanceKind(i) & IARR) {
    return ARY_INST(i)->indirected;
  }
  return LONG_MAX;
}

symchar *InstanceType(register CONST struct Instance *i)
{
  AssertMemory(i);
  switch(i->t) {
  case SIM_INST:	/* Not sure -- return the type of root */
    return GetName(SIM_INST(i)->desc);
  case MODEL_INST:
    return GetName(MOD_INST(i)->desc);
  case DUMMY_INST:
    return GetName(D_INST(i)->desc);
  case REAL_CONSTANT_INST:
    return GetName(RC_INST(i)->desc);
  case BOOLEAN_CONSTANT_INST:
    return GetName(BC_INST(i)->desc);
  case INTEGER_CONSTANT_INST:
    return GetName(IC_INST(i)->desc);
  case SYMBOL_CONSTANT_INST:
    return GetName(SYMC_INST(i)->desc);
  case REAL_ATOM_INST:
    return GetName(RA_INST(i)->desc);
  case BOOLEAN_ATOM_INST:
    return GetName(BA_INST(i)->desc);
  case INTEGER_ATOM_INST:
    return GetName(IA_INST(i)->desc);
  case SET_ATOM_INST:
    return GetName(SA_INST(i)->desc);
  case SYMBOL_ATOM_INST:
    return GetName(SYMA_INST(i)->desc);
  case REL_INST:
    return GetName(FindRelationType());
  case LREL_INST:
    return GetName(FindLogRelType());
  case WHEN_INST:
    return GetName(FindWhenType());
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:			/* the following addsymbol calls suck */
    return AddSymbol("");
  case REAL_INST:
    return GetBaseTypeName(real_type);
  case INTEGER_INST:
    return GetBaseTypeName(integer_type);
  case BOOLEAN_INST:
    return GetBaseTypeName(boolean_type);
  case SET_INST:
    return GetBaseTypeName(set_type);
  case SYMBOL_INST:
    return GetBaseTypeName(symbol_type);
  default:
    ASC_PANIC("InstanceType called on invalid instance.\n");
    
  }
}


struct BitList *InstanceBitList(CONST struct Instance *i)
{
  AssertMemory(i);
  switch(i->t) {
  case MODEL_INST:
    return MOD_INST(i)->executed;
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
    return NULL;
  default:
    ASC_PANIC("Unknown instance type passed to InstanceBitList.\n");
    
  }
}

