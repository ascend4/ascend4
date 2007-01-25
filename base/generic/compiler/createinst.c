/*
 *  Ascend Instance Allocations Functions
 *  by Tom Epperly & Ben Allan
 *  9/3/89
 *  Version: $Revision: 1.19 $
 *  Version control file: $RCSfile: createinst.c,v $
 *  Date last modified: $Date: 1998/03/26 20:39:41 $
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
#include <utilities/error.h>
#include <general/list.h>
#include <general/dstring.h>

#include "bit.h"
#include "symtab.h"
#include "functype.h"
#include "expr_types.h"
#include "stattypes.h"
#include "slist.h"
#include "setinstval.h"
#include "instance_name.h"
#include "instance_io.h"
#include "check.h"
#include "dump.h"
#include "child.h"
#include "childinfo.h"
#include "type_desc.h"
#include "prototype.h"
#include "pending.h"
#include "find.h"
#include "extfunc.h"
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
/* new */
#include "atomsize.h"
#include "atomvalue.h"
#include "cmpfunc.h"
#include "copyinst.h"
#include "createinst.h"
#include "destroyinst.h"
#include "extinst.h"
#include "linkinst.h"
#include "instmacro.h"
#include "instquery.h"

#ifndef lint
static CONST char CreateInstModuleID[] = "$Id: createinst.c,v 1.19 1998/03/26 20:39:41 ballan Exp $";
#endif

void ZeroNewChildrenEntries(register struct Instance **child_ary,
			    register unsigned long int num)
{
  /* initialize all the children pointers to NULL */
  while(num--) {
    AssertMemory(child_ary);
    *(child_ary++) = NULL;
  }
}

/*
 * Create a new model instance. The semantics at this time is
 * as follows: We lookup for a prototype of the model in the
 * prototype library. If found we do a deepcopy of the instance.
 * We do this deep copy using copy-by-reference semantics on
 * relations. THIS may change.
 */
struct Instance *CreateModelInstance(struct TypeDescription *type)
{
  register struct ModelInstance *result, *proto;
  register unsigned long num_children;
  register CONST struct StatementList *stats;

  proto = MOD_INST(LookupPrototype(GetName(type)));
  if (proto==NULL) {
    CopyTypeDesc(type);
    num_children = ChildListLen(GetChildList(type));
    stats = GetStatementList(type);
    result = MOD_INST(ascmalloc((unsigned)sizeof(struct ModelInstance)+
				(unsigned)num_children*
				(unsigned)sizeof(struct Instance *)));
    result->t = MODEL_INST;
    result->pending_entry = NULL;
    result->interface_ptr = NULL;
    result->parents = gl_create(AVG_PARENTS);
    result->whens = NULL;
    result->desc = type;
    result->alike_ptr = INST(result);
    result->visited = 0;
    result->tmp_num = 0;
    result->anon_flags = 0x0;
#if (LONGCHILDREN == 1)
    result->padding = INT_MAX;
#endif

    result->executed = CreateFBList(gl_length(GetList(stats)));
    ZeroNewChildrenEntries(MOD_CHILD(result,0),num_children);
    AssertMemory(result);
    if (GetUniversalFlag(type)) {
      AddUniversalInstance(GetUniversalTable(),type,INST(result));
    }
    return INST(result);
  } else {	/* instance has a prototype which may be copied */
    result = MOD_INST(CopyInstance(INST(proto)));
    return INST(result);
  }
}

struct Instance *CreateDummyInstance(struct TypeDescription *gdt)
{
  struct GlobalDummyInstance *result;
  assert(GetBaseType(gdt)==dummy_type);
  result = ASC_NEW(struct GlobalDummyInstance);
  assert(result!=NULL);
  CopyTypeDesc(gdt);
  result->t = DUMMY_INST;
  result->interface_ptr = NULL;
  result->desc = gdt;
  result->ref_count = 1L;
  result->visited = 0L;
  result->tmp_num = 0L;
  result->anon_flags = 0;
  AddUniversalInstance(GetUniversalTable(),gdt,INST(result));
  return INST(result);
}

/*********************************************************************\
  Simulation stuff.
\*********************************************************************/

struct Instance *CreateSimulationInstance(struct TypeDescription *type,
					  symchar *name)
{
  register struct SimulationInstance *result;
  int num_children;
  unsigned int size;

  num_children = 1;
  CopyTypeDesc(type);
  size = (unsigned)sizeof(struct SimulationInstance) +
    (unsigned)num_children*sizeof(struct Instance *);
  result = SIM_INST(ascmalloc((unsigned)sizeof(struct SimulationInstance) +
			      (unsigned)num_children*
			      sizeof(struct Instance *)));
  result->t = SIM_INST;
  result->interface_ptr = NULL;
  result->desc = type;
  result->name = name;
  result->extvars = NULL;
  return INST(result);
}

/*
 * the following initialize (but do not allocate)
 * fundamental instances. (children of atoms)
 */
static void CreateReal(struct RealInstance *i, struct Instance *parent)
{
  i->t = REAL_INST;
  i->parent_offset = INST((unsigned long)i-(unsigned long)parent);
  i->value = UNDEFAULTEDREAL;
  i->dimen = WildDimension();
  i->assigned = 0;
  i->depth = UINT_MAX;
  i->padding = INT_MAX;
  AssertContainedMemory(i,sizeof(struct RealInstance));
}

static void CreateInteger(struct IntegerInstance *i, struct Instance *parent)
{
  i->t = INTEGER_INST;
  i->parent_offset = INST((unsigned long)i-(unsigned long)parent);
  i->value = 0;
  i->assigned = 0;
  i->depth = 0;
  i->padding = INT_MAX;
  AssertContainedMemory(i,sizeof(struct IntegerInstance));
}

static void CreateBoolean(struct BooleanInstance *i, struct Instance *parent)
{
  i->t = BOOLEAN_INST;
  i->parent_offset = INST((unsigned long)i-(unsigned long)parent);
  i->value = 0;
  i->assigned = 0;
  i->depth = UINT_MAX;
  AssertContainedMemory(i,sizeof(struct BooleanInstance));
#if (LONGCHILDREN == 0)
  i->padding = INT_MAX;
#endif

}

static void CreateSet(struct SetInstance *i,
			struct Instance *parent, int intset)
{
  i->t = SET_INST;
  i->parent_offset = INST((unsigned long)i-(unsigned long)parent);
  i->int_set = intset ? 1 : 0;
  i->list = NULL;
  AssertContainedMemory(i,sizeof(struct SetInstance));
}

static void CreateSymbol(struct SymbolInstance *i, struct Instance *parent)
{
  i->t = SYMBOL_INST;
  i->parent_offset = INST((unsigned long)i-(unsigned long)parent);
  i->value = NULL;
  AssertContainedMemory(i,sizeof(struct SymbolInstance));
}

static void MakeAtomChildren(unsigned long int nc, /* number of children */
		      struct Instance *parent, /* parent's address */
		      struct Instance *base, /* starting address of */
					     /* children instances */
		      struct Instance **iptrs, /* child pointer list */
		      CONST struct ChildDesc *childd) /* children */
						      /* descriptions */
{
  register unsigned long c=1;
  struct ChildDesc cd;
  while(c<=nc) {
    AssertContainedIn(parent,base);
    AssertContainedIn(parent,iptrs);
    *iptrs = base;		/* set pointer to child's address */
    cd = GetChildArrayElement(childd,c);
    switch(ChildDescType(cd)) {
    case real_child:
      CreateReal(R_INST(base),parent);
      if (ValueAssigned(cd)) {
	SetRealAtomValue(base,RealDefaultValue(cd),UINT_MAX);
	SetRealAtomDims(base,RealDimensions(cd));
      }
      /* prepare for next child */
      base = INST((unsigned long)base+sizeof(struct RealInstance));
      break;
    case integer_child:
      CreateInteger(I_INST(base),parent);
      if (ValueAssigned(cd))
	SetIntegerAtomValue(base,IntegerDefault(cd),UINT_MAX);
      /* prepare for next child */
      base = INST((unsigned long)base+sizeof(struct IntegerInstance));
      break;
    case boolean_child:
      CreateBoolean(B_INST(base),parent);
      if (ValueAssigned(cd))
	SetBooleanAtomValue(base,BooleanDefault(cd),UINT_MAX);
      /* prepare for next child */
      base = INST((unsigned long)base+sizeof(struct BooleanInstance));
      break;
    case set_child:
      CreateSet(S_INST(base),parent,(int)SetIsIntegerSet(cd));
      if (ValueAssigned(cd))
	AssignSetAtomList(base,CopySet(SetDefault(cd)));
      /* prepare for next child */
      base = INST((unsigned long)base+sizeof(struct SetInstance));
      break;
    case symbol_child:
      CreateSymbol(SYM_INST(base),parent);
      if (ValueAssigned(cd))
	SetSymbolAtomValue(base,SymbolDefault(cd));
      /* prepare for next child */
      base = INST((unsigned long)base+sizeof(struct SymbolInstance));
      break;
    case bad_child:
      ASC_PANIC("MakeAtomChildren called with bad_child\n");
      break;
    }
    /* set pointer and counter for next child */
    iptrs++;
    c++;
  }
  /* FIX BUG fix me bug !
   * ifndef NDEBUG, here we should be making sure there are no
   * extra trailing child ptrs (due to next-highest-even) and
   * there ARE some, initing them to NULL.
   */
}


/**
	This function, redone by kirk, no longer finds universals and
	returns them. if that were appropriate, it should have been
	checked and done before hand. By ShortCutMakeUniversalInstance
*/
struct Instance *CreateRealInstance(struct TypeDescription *type){

  if (BaseTypeIsAtomic(type)) {
    register struct RealAtomInstance *result;
    register unsigned long num_children;

    if ((result=RA_INST(LookupPrototype(GetName(type))))==NULL) {
      CopyTypeDesc(type);
      num_children = ChildListLen(GetChildList(type));
      result = RA_INST(ascmalloc(GetByteSize(type)));
      result->t = REAL_ATOM_INST;
      result->interface_ptr = NULL;
      result->parents = gl_create(AVG_PARENTS);
      result->alike_ptr = INST(result);
      result->desc = type;
      result->visited = 0;
      result->tmp_num = 0;
      result->anon_flags = 0x0;
      result->dimen = GetRealDimens(type);
      result->relations = NULL;
      result->depth = UINT_MAX;

      if(AtomDefaulted(type)){
        result->value = GetRealDefault(type);
        result->assigned = 1;
      }
      else {
        result->value = UNDEFAULTEDREAL;
        result->assigned = 0;
      }

      MakeAtomChildren(num_children,
			INST(result),
			BASE_ADDR(result,num_children,struct RealAtomInstance),
			CLIST(result,struct RealAtomInstance),
			GetChildDesc(type)
      );
      AssertMemory(result);

	  /*
      CONSOLE_DEBUG("CREATED REAL ATOM INSTANCE of type '%s' at %p"
		,SCP(GetName(type)),result
      );
      */

      if (GetUniversalFlag(type)){
        AddUniversalInstance(GetUniversalTable(),type,INST(result));
        return INST(result);
      }
      AddPrototype(CopyInstance(INST(result)));
      return INST(result);
    }else{ /* instance type has a prototype which can be copied */
      result = CopyInstance(INST(result));
      /*
      CONSOLE_DEBUG("CREATED (COPIED PROTOTYPE) REAL ATOM INSTANCE "
			"of type '%s' at %p" ,SCP(GetName(type)),result
	  );
	  */
	  return result;
    }
  } else {
    /* create constant */
    register struct RealConstantInstance *result;
    if((result=RC_INST(LookupPrototype(GetName(type))))==NULL){
      CopyTypeDesc(type);
      result = RC_INST(ascmalloc(GetByteSize(type)));
      result->t = REAL_CONSTANT_INST;
      result->parents = gl_create(AVG_CONSTANT_PARENTS);
      result->alike_ptr = INST(result);
      result->desc = type;
      result->visited = 0L;
      result->tmp_num = 0L;
      result->interface_ptr = NULL;
      result->dimen = GetConstantDimens(type);
      if (ConstantDefaulted(type)) {
        result->value = GetConstantDefReal(type);
        result->vflag = ci_ASSIGNED;
      }
      else {
        result->value = UNDEFAULTEDREAL;
        result->vflag = 0;
      }
      AssertMemory(result);
      if (GetUniversalFlag(type)){
        AddUniversalInstance(GetUniversalTable(),type,INST(result));
        return INST(result);
      }
      AddPrototype(CopyInstance(INST(result)));
      return INST(result);
    } else { /* instance type has a prototype which can be copied */
      return CopyInstance(INST(result));
    }
  }
}

struct Instance *CreateIntegerInstance(struct TypeDescription *type)
{
  if (BaseTypeIsAtomic(type)) {
    register struct IntegerAtomInstance *result;
    register unsigned long num_children;

    if ((result=IA_INST(LookupPrototype(GetName(type))))==NULL) {
      CopyTypeDesc(type);
      num_children = ChildListLen(GetChildList(type));
      result = IA_INST(ascmalloc(GetByteSize(type)));
      result->t = INTEGER_ATOM_INST;
      result->interface_ptr = NULL;
      result->parents = gl_create(AVG_PARENTS);
      result->alike_ptr = INST(result);
      result->desc = type;
      result->whens = NULL;
      result->visited = 0;
      result->tmp_num = 0;
      result->anon_flags = 0x0;
      result->depth = UINT_MAX;
      if (AtomDefaulted(type)) {
        result->value = GetIntDefault(type);
        result->assigned = 1;
      } else {
        result->value = 0L;
        result->assigned = 0;
      }
      MakeAtomChildren(num_children,
		       INST(result),
		       BASE_ADDR(result,num_children,
				struct IntegerAtomInstance),
		       CLIST(result,struct IntegerAtomInstance),
		       GetChildDesc(type));
      AssertMemory(result);
      if (GetUniversalFlag(type)){
        AddUniversalInstance(GetUniversalTable(),type,INST(result));
        return INST(result);
      }
      AddPrototype(CopyInstance(INST(result)));
      return INST(result);
    }
    else {/* a prototype exists which can be copied */
      return CopyInstance(INST(result));
    }
  } else {
    register struct IntegerConstantInstance *result;

    if ((result=IC_INST(LookupPrototype(GetName(type))))==NULL) {
      CopyTypeDesc(type);
      result = IC_INST(ascmalloc(GetByteSize(type)));
      result->t = INTEGER_CONSTANT_INST;
      result->parents = gl_create(AVG_ICONSTANT_PARENTS);
      result->alike_ptr = INST(result);
      result->whens = NULL;
      result->interface_ptr = NULL;
      result->desc = type;
      result->visited = 0;
      result->tmp_num = 0;
      result->interface_ptr = NULL;
      if (ConstantDefaulted(type)) {
        result->value = GetConstantDefInteger(type);
        result->vflag = ci_ASSIGNED;
      } else {
        result->value = 0L;
        result->vflag = 0;
      }
      if (GetUniversalFlag(type)){
        AddUniversalInstance(GetUniversalTable(),type,INST(result));
        return INST(result);
      }
      AddPrototype(CopyInstance(INST(result)));
      return INST(result);
    }
    else {/* a prototype exists which can be copied */
      return CopyInstance(INST(result));
    }
  }
}

struct Instance *CreateBooleanInstance(struct TypeDescription *type)
{
  if (BaseTypeIsAtomic(type)) {
    register struct BooleanAtomInstance *result;
    register unsigned long num_children;

    if ((result=BA_INST(LookupPrototype(GetName(type))))==NULL) {
      CopyTypeDesc(type);
      num_children = ChildListLen(GetChildList(type));
      result = BA_INST(ascmalloc(GetByteSize(type)));
      result->t = BOOLEAN_ATOM_INST;
      result->interface_ptr = NULL;
      result->parents = gl_create(AVG_PARENTS);
      result->alike_ptr = INST(result);
      result->desc = type;
      result->visited = 0;
      result->tmp_num = 0;
      result->anon_flags = 0x0;
      result->depth = UINT_MAX;
      result->padding = INT_MAX;
      result->logrelations = NULL;
      result->whens = NULL;
      if (AtomDefaulted(type)) {
        result->value = GetBoolDefault(type);
        result->assigned = 1;
      } else {
        result->value = 0;
        result->assigned = 0;
      }
      MakeAtomChildren(num_children,
		       INST(result),
		       BASE_ADDR(result,num_children,
				struct BooleanAtomInstance),
		       CLIST(result,struct BooleanAtomInstance),
		       GetChildDesc(type));
      AssertMemory(result);
      if (GetUniversalFlag(type)){
        AddUniversalInstance(GetUniversalTable(),type,INST(result));
        return INST(result);
      }
      AddPrototype(CopyInstance(INST(result)));
      return INST(result);
    }
    else {/* a prototype exists which can be copied */
      return CopyInstance(INST(result));
    }
  } else {
    register struct BooleanConstantInstance *result;

    if ((result=BC_INST(LookupPrototype(GetName(type))))==NULL) {
      CopyTypeDesc(type);
      result = BC_INST(ascmalloc(GetByteSize(type)));
      result->t = BOOLEAN_CONSTANT_INST;
      result->parents = gl_create(AVG_ICONSTANT_PARENTS);
      result->alike_ptr = INST(result);
      result->whens = NULL;
      result->interface_ptr = NULL;
      result->desc = type;
      result->visited = 0;
      result->tmp_num = 0;
      if (ConstantDefaulted(type)) {
        result->vflag = 1;
        result->vflag |= GetConstantDefBoolean(type);
      } else {
        result->vflag = 0;
      }
      AssertMemory(result);
      if (GetUniversalFlag(type)){
        AddUniversalInstance(GetUniversalTable(),type,INST(result));
        return INST(result);
      }
      AddPrototype(CopyInstance(INST(result)));
      return INST(result);
    }
    else {/* a prototype exists which can be copied */
      return CopyInstance(INST(result));
    }
  }
}

struct Instance *CreateSetInstance(struct TypeDescription *type, int intset)
{
  if (BaseTypeIsAtomic(type)) {
    register struct SetAtomInstance *result;
    register unsigned long num_children;

    if ((result=SA_INST(LookupPrototype(GetName(type))))==NULL) {
      CopyTypeDesc(type);
      num_children = ChildListLen(GetChildList(type));
      result = SA_INST(ascmalloc(GetByteSize(type)));
      result->t =  SET_ATOM_INST;
      result->interface_ptr = NULL;
      result->parents = gl_create(AVG_PARENTS);
      result->alike_ptr = INST(result);
      result->desc = type;
      result->visited = 0;
      result->tmp_num = 0;
      result->anon_flags = 0x0;
      result->list = NULL;
      result->int_set = intset ? 1 : 0;
      MakeAtomChildren(num_children,
		       INST(result),
		       BASE_ADDR(result,num_children,struct SetAtomInstance),
		       CLIST(result,struct SetAtomInstance),
		       GetChildDesc(type));
      AssertMemory(result);
      if (GetUniversalFlag(type)){
        AddUniversalInstance(GetUniversalTable(),type,INST(result));
        return INST(result);
      }
      AddPrototype(CopyInstance(INST(result)));
    }
    else{ /* a prototype exists which may be copied */
      result = SA_INST(CopyInstance(INST(result)));
      result->int_set = intset ? 1 : 0;
    }
    return INST(result);
  } else {
    FPRINTF(ASCERR,"set_constants not yet instantiable\n");
    return NULL;
  }
}

struct Instance *CreateSymbolInstance(struct TypeDescription *type)
{
  if (BaseTypeIsAtomic(type)) {
    register struct SymbolAtomInstance *result;
    register unsigned long num_children;

    if ((result=SYMA_INST(LookupPrototype(GetName(type))))==NULL){
      CopyTypeDesc(type);
      num_children = ChildListLen(GetChildList(type));
      result = SYMA_INST(ascmalloc(GetByteSize(type)));
      result->t = SYMBOL_ATOM_INST;
      result->interface_ptr = NULL;
      result->parents = gl_create(AVG_PARENTS);
      result->alike_ptr = INST(result);
      result->desc = type;
      result->whens = NULL;
      result->visited = 0;
      result->tmp_num = 0;
      result->anon_flags = 0x0;
      /* atom value stuff */
      if (AtomDefaulted(type)) {
        result->value = GetSymDefault(type);
      } else {
        result->value = NULL;
      }
      MakeAtomChildren(num_children,
		       INST(result),
		       BASE_ADDR(result,num_children,struct SymbolAtomInstance),
		       CLIST(result,struct SymbolAtomInstance),
		       GetChildDesc(type));
      AssertMemory(result);
      if (GetUniversalFlag(type)){
        AddUniversalInstance(GetUniversalTable(),type,INST(result));
        return INST(result);
      }
      AddPrototype(CopyInstance(INST(result)));
      return INST(result);
    }
    else { /* a prototype exists which may be copied */
      return CopyInstance(INST(result));
    }
  } else {
    register struct SymbolConstantInstance *result;

    if ((result=SYMC_INST(LookupPrototype(GetName(type))))==NULL){
      CopyTypeDesc(type);
      result = SYMC_INST(ascmalloc(GetByteSize(type)));
      result->t = SYMBOL_CONSTANT_INST;
      result->parents = gl_create(AVG_ICONSTANT_PARENTS);
      result->alike_ptr = INST(result);
      result->whens = NULL;
      result->interface_ptr = NULL;
      result->desc = type;
      result->visited = 0;
      result->tmp_num = 0;
      if (ConstantDefaulted(type)) {
        result->value = GetConstantDefSymbol(type);
        result->vflag = ci_ASSIGNED;
      } else {
        result->value = NULL;
        result->vflag = 0;
      }
      AssertMemory(result);
      if (GetUniversalFlag(type)){
        AddUniversalInstance(GetUniversalTable(),type,INST(result));
        return INST(result);
      }
      AddPrototype(CopyInstance(INST(result)));
      return INST(result);
    }
    else { /* a prototype exists which may be copied */
      return CopyInstance(INST(result));
    }
  }
}

struct Instance *CreateRelationInstance(struct TypeDescription *type,
					enum Expr_enum reltype)
{
  register struct RelationInstance *result;
  register unsigned long num_children;
  /* ERROR_REPORTER_DEBUG("Entered CreateRelationInstance\n"); */

  if ((result=RELN_INST(LookupPrototype(GetName(type))))==NULL){
    CopyTypeDesc(type);
    num_children = ChildListLen(GetChildList(type));
    result = RELN_INST(ascmalloc(GetByteSize(type)));
    result->t = REL_INST;
    result->interface_ptr = NULL;
    result->parent[0] = NULL;	/* relations can have only two parents */
    result->parent[1] = NULL;	/* initially it has none */
    result->desc = type;
    result->visited = 0;
    result->tmp_num = 0;
    result->anon_flags = 0;
    /* relation stuff */
    CONSOLE_DEBUG("Creating relation instance at %p with NULL rel ptr",result);
    result->ptr = NULL;
    result->whens = NULL;
    result->logrels = NULL;
    result->type = reltype;
    /* Not required anymore :
     * #if (LONGCHILDREN == 0)
     * result->padding = INT_MAX;
     * #endif
     */
    MakeAtomChildren(num_children,
		     INST(result),
		     BASE_ADDR(result,num_children,struct RelationInstance),
		     CLIST(result,struct RelationInstance),
		     GetChildDesc(type));
    AddPrototype(CopyInstance(INST(result)));
    AssertMemory(result);
    return INST(result);
  } else{			/* a prototype exists which may be copied */
    result = RELN_INST(CopyInstance(INST(result)));
    result->type = reltype;	/* now set up the reltype */
    return INST(result);
  }
}


struct Instance *CreateLogRelInstance(struct TypeDescription *type)
{
  register struct LogRelInstance *result;
  register unsigned long num_children;
  if ((result=LRELN_INST(LookupPrototype(GetName(type))))==NULL){
    CopyTypeDesc(type);
    num_children = ChildListLen(GetChildList(type));
    result = LRELN_INST(ascmalloc(GetByteSize(type)));
    result->t = LREL_INST;
    result->interface_ptr = NULL;
    result->parent[0] = NULL;	/*logical relations can have only two parents*/
    result->parent[1] = NULL;	/* initially it has none */
    result->desc = type;
    result->visited = 0;
    result->tmp_num = 0;
    result->anon_flags = 0x0;
    result->whens = NULL;
    result->logrels = NULL;
    result->padding = INT_MAX;
    /* relation stuff */
    result->ptr = NULL;
    MakeAtomChildren(num_children,
		     INST(result),
		     BASE_ADDR(result,num_children,struct LogRelInstance),
		     CLIST(result,struct LogRelInstance),
		     GetChildDesc(type));
    AddPrototype(CopyInstance(INST(result)));
    AssertMemory(result);
    return INST(result);
  }
  else{				/* a prototype exists which may be copied */
    result = LRELN_INST(CopyInstance(INST(result)));
    return INST(result);
  }
}


struct Instance *CreateWhenInstance(struct TypeDescription *type)
{
  register struct WhenInstance *result;
  if ((result=W_INST(LookupPrototype(GetName(type))))==NULL){
    CopyTypeDesc(type);
    result = W_INST(ascmalloc((unsigned)sizeof(struct WhenInstance)));
    result->t = WHEN_INST;
    result->interface_ptr = NULL;
    result->parent[0] = NULL;	/* relations can have only two parents */
    result->parent[1] = NULL;	/* initially it has none */
    result->whens = NULL;	/* initially it has none */
    result->cases = NULL;
    result->bvar = NULL;
    result->desc = type;
    result->visited = 0;
    result->tmp_num = 0;
    result->anon_flags = 0x0;

    AddPrototype(CopyInstance(INST(result)));
    AssertMemory(result);
    return INST(result);
  }
  else{				/* a prototype exists which may be copied */
    result = W_INST(CopyInstance(INST(result)));
    return INST(result);
  }
}



/*
 * Note: CopyTypeDesc(type) is not done here.; This is
 * handled by CreateArrayTypeDesc().
 */
struct Instance *CreateArrayInstance(struct TypeDescription *type,
				     unsigned long int pos)
{
  struct ArrayInstance *result;
  struct gl_list_t *list;
  struct IndexType *ptr;
  assert(type!=NULL);

  result = ARY_INST(ascmalloc((unsigned)sizeof(struct ArrayInstance)));
  list = GetArrayIndexList(type);
  if ((list==NULL)||(gl_length(list)==0)) {
    ASC_PANIC("An array without any indicies!\n");
  }
  ptr = (struct IndexType *)gl_fetch(list,pos);
  if (GetIndexType(ptr)) {
    result->t = ARRAY_INT_INST;
  } else {
    result->t = ARRAY_ENUM_INST;
  }
  result->pending_entry = NULL;
  result->interface_ptr = NULL;
  result->desc = type;
  result->parents = gl_create(AVG_PARENTS);
  result->children = NULL;
  result->indirected = pos-1;
  result->visited = 0;
  result->tmp_num = 0;
  result->anon_flags = 0x0;
  AssertMemory(result);
  return INST(result);
}
