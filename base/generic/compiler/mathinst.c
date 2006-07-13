/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
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
	Ascend Instance Tree Type Implementation
*//*
	by Tom Epperly
	9/3/89
	Last in CVS: $Revision: 1.10 $ $Date: 1998/05/06 17:33:36 $ $Author: ballan $
*/

#include <stdarg.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <general/pool.h>
#include <general/list.h>
#include <general/dstring.h>
#include "compiler.h"
#include "bit.h"
#include "symtab.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "expr_types.h"
#include "child.h"
#include "type_desc.h"
#include "instance_enum.h"
#include "instance_name.h"
#include "instance_io.h"
#include "instmacro.h"
#include "extinst.h"
#include "instquery.h"
#include "instance_types.h"
#include "linkinst.h"
#include "destroyinst.h"
#include "createinst.h"
#include "refineinst.h"
#include "atomvalue.h"
#include "atomsize.h"
#include "check.h"
#include "dump.h"
#include "childinfo.h"
#include "prototype.h"
#include "pending.h"
#include "find.h"
#include "relation_type.h"
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
#include "tmpnum.h"
#include "mathinst.h"

static const char panic_msg[]="Incorrect type passed";
#define PANIC_INCORRECT_TYPE Asc_Panic(2,__FUNCTION__,panic_msg);

enum Expr_enum GetInstanceRelationType(CONST struct Instance *i)
{
  AssertMemory(i);
  if (i->t == REL_INST) {
    return RELN_INST(i)->type; /* the implementation kind */
  }else{
    PANIC_INCORRECT_TYPE;
  }
}

CONST struct relation *GetInstanceRelationOnly(CONST struct Instance *i)
{
  AssertMemory(i);
  if (i->t == REL_INST) {
    return RELN_INST(i)->ptr;
  }else{
    PANIC_INCORRECT_TYPE;
  }
}

CONST struct relation *GetInstanceRelation(CONST struct Instance *i,
					   enum Expr_enum *type)
{
  AssertMemory(i);
  if (i->t == REL_INST) {
    *type = RELN_INST(i)->type;
    return RELN_INST(i)->ptr;
  }else{
    PANIC_INCORRECT_TYPE;
  }
}

struct relation *GetInstanceRelToModify(struct Instance *i,
					enum Expr_enum *type)
{
  AssertMemory(i);
  if (i->t == REL_INST) {
    *type = RELN_INST(i)->type;
    return RELN_INST(i)->ptr;
  }else{
    PANIC_INCORRECT_TYPE;
    
  }
}


CONST struct logrelation *GetInstanceLogRel(CONST struct Instance *i)
{
  AssertMemory(i);
  if (i->t == LREL_INST) {
    return LRELN_INST(i)->ptr;
  }else{
    PANIC_INCORRECT_TYPE;
    
  }
}

struct logrelation *GetInstanceLogRelToModify(struct Instance *i)
{
  AssertMemory(i);
  if (i->t == LREL_INST) {
    return LRELN_INST(i)->ptr;
  }else{
    PANIC_INCORRECT_TYPE;
    
  }
}

CONST struct logrelation *GetInstanceLogRelOnly(CONST struct Instance *i)
{
  AssertMemory(i);
  if (InstanceKind(i) == LREL_INST) {
    return LRELN_INST(i)->ptr;
  }else{
    PANIC_INCORRECT_TYPE;
    
  }
}

/**
	Returns list of vars/models/equations in a mathematical relationship.
	May want to include the models/relations from the cases of a when.
*/
struct gl_list_t *GetInstanceOperands(CONST struct Instance *i)
{
  CONST struct gl_list_t *list = NULL;
  CONST struct gl_list_t *list2 = NULL;
  struct gl_list_t *result = NULL;
  unsigned long c,len;
  CONST struct relation *rel;
  CONST struct logrelation *lrel;
  void *p;

  if (i == NULL) {
    return NULL;
  }
  switch (InstanceKind(i)) {
  case REL_INST:
    rel = GetInstanceRelationOnly(i);
    if (rel != NULL) {
      list = RelationVarList(rel);
    }
    break; 
  case LREL_INST:
    lrel = GetInstanceLogRel(i);
    if (lrel != NULL) {
      list = LogRelBoolVarList(lrel);
      list2 = LogRelSatRelList(lrel);
    }
    break; 
  case WHEN_INST:
    list = GetInstanceWhenVars(i);
    break; 
  default:
    return NULL;
  }
  len = 0;
  len += (list != NULL) ? gl_length(list) : 0;
  len += (list2 != NULL) ? gl_length(list2) : 0;
  result = gl_create(len);
  if (list != NULL) {
    for (c=1; c <= len; c++) {
      p = gl_fetch(list,c);
      if (p!=NULL) {
        gl_append_ptr(result,p);
      }
    }
  }
  if (list2 != NULL) {
    for (c=1; c <= len; c++) {
      p = gl_fetch(list2,c);
      if (p!=NULL) {
        gl_append_ptr(result,p);
      }
    }
  }
  return result;
}

struct gl_list_t *GetInstanceWhenVars(CONST struct Instance *i)
{
  AssertMemory(i);
  if (i->t == WHEN_INST) {
    return W_INST(i)->bvar;
  }else{
    PANIC_INCORRECT_TYPE;
    
  }
}

struct gl_list_t *GetInstanceWhenCases(CONST struct Instance *i)
{
  AssertMemory(i);
  if (i->t == WHEN_INST) {
    return W_INST(i)->cases;
  }else{
    PANIC_INCORRECT_TYPE;
    
  }
}

struct gl_list_t *GetInstanceWhens(CONST struct Instance *i)
{
  AssertMemory(i);
  switch(i->t) {
    case BOOLEAN_ATOM_INST:
      return BA_INST(i)->whens;
    case INTEGER_ATOM_INST:
      return IA_INST(i)->whens;
    case SYMBOL_ATOM_INST:
      return SYMA_INST(i)->whens;
    case BOOLEAN_CONSTANT_INST:
      return BC_INST(i)->whens;
    case INTEGER_CONSTANT_INST:
      return IC_INST(i)->whens;
    case SYMBOL_CONSTANT_INST:
      return SYMC_INST(i)->whens;
    case MODEL_INST:
      return MOD_INST(i)->whens;
    case REL_INST:
      return RELN_INST(i)->whens;
    case LREL_INST:
      return LRELN_INST(i)->whens;
    case WHEN_INST:
      return W_INST(i)->whens;
    default:
    PANIC_INCORRECT_TYPE;
  }
}

void SetWhenVarList(struct Instance *i,struct gl_list_t *whenvars)
{
  AssertMemory(i);
  if (i->t == WHEN_INST) {
    W_INST(i)->bvar = whenvars;
  }else{
    PANIC_INCORRECT_TYPE;
  }
}

void SetWhenCases(struct Instance *i,struct gl_list_t *whencases)
{
  AssertMemory(i);
  if (i->t == WHEN_INST) {
    W_INST(i)->cases = whencases;
  }else{
     PANIC_INCORRECT_TYPE;
  }
}

/**
	This is a tricky function. If we are attempting to set
	a new relation, and if the i->ptr is not NULL, i.e. a
	relation structure was already assigned, we must *not* force
	the assignment, and destroy the old relation. Unfortunately
	at this stage, we would have to back up todo undo the
	AddRelations etc. So we punt.
*/
void SetInstanceRelation(struct Instance *i, struct relation *rel,
			 enum Expr_enum type)
{
  AssertMemory(i);
  if(i->t==REL_INST){
    if(RELN_INST(i)->ptr==NULL){
      /* CONSOLE_DEBUG("RelationInstance %p points to relation at %p",i,rel); */
      RELN_INST(i)->ptr = rel;
      RELN_INST(i)->type = type;
    }else{
      Asc_Panic(2, __FUNCTION__, "Attempt to reassign RelationPointer.");
    }
  }else{
    PANIC_INCORRECT_TYPE;
  }
}

void SetInstanceLogRel(struct Instance *i, struct logrelation *lrel){
  AssertMemory(i);
  if(i->t==LREL_INST){
    if(LRELN_INST(i)->ptr==NULL){
      LRELN_INST(i)->ptr = lrel;
    }else{
      Asc_Panic(2, __FUNCTION__, "Attempted reassignment to logrel ptr");
    }
  }else{
    PANIC_INCORRECT_TYPE;
  }
}

/*------------------------------------------------------------------------------
  RELATION LIST STUFF
*/

unsigned long RelationsCount(CONST struct Instance *i)
{
  assert(i!=NULL);
  AssertMemory(i);
  switch(i->t) {
  case REAL_ATOM_INST:
    if (RA_INST(i)->relations!=NULL) {
      return gl_length(RA_INST(i)->relations);
    }else{
      return 0;
    }
  default:
    Asc_Panic(2, NULL, "RelationsCount called with inappropriate argument.");
    
  }
}

struct Instance *RelationsForAtom(CONST struct Instance *i,
				  unsigned long int c)
{
  assert((i!=NULL)&&(c>0)&&(c<=RelationsCount(i)));
  AssertMemory(i);
  switch(i->t){
  case REAL_ATOM_INST:
    if (RA_INST(i)->relations!=NULL) {
      return INST(gl_fetch(RA_INST(i)->relations,c));
    }else{
      Asc_Panic(2, NULL, "c out of bounds in RelationsForAtom.");
    }
    break;
  default:
    Asc_Panic(2, NULL,
              "RelationsForAtom called with inappropriate argument.");
    break;
  }
  exit(2);/* NOT REACHED.  Needed to keep gcc from whining */
}

void AddRelation(struct Instance *i, struct Instance *reln){
  unsigned long len;
  assert(i&&reln&&(reln->t==REL_INST));
  AssertMemory(i);
  switch(i->t){
  case REAL_ATOM_INST:
	/* CONSOLE_DEBUG("ADD RelationInstance %p to RealAtomInstance %p",reln,i); */
    if (RA_INST(i)->relations==NULL) {
      RA_INST(i)->relations = gl_create(AVG_RELATIONS);
    }
    len = gl_length(RA_INST(i)->relations);
    if (gl_search(RA_INST(i)->relations,(char*)reln,(CmpFunc)CmpRelations)==0){
      gl_append_ptr(RA_INST(i)->relations,(VOIDPTR)reln);
    }
    break;
  default:
    PANIC_INCORRECT_TYPE;
  }
}

void RemoveRelation(struct Instance *i, struct Instance *reln)
{
  register unsigned long c;
  assert(i&&reln&&(reln->t==REL_INST));
  AssertMemory(i);
  switch(i->t) {
  case REAL_ATOM_INST:
    if (RA_INST(i)->relations==NULL) {
      return;
    }
    c = gl_search(RA_INST(i)->relations,(char *)reln,(CmpFunc)CmpRelations);
    if (c>0) {
      gl_delete(RA_INST(i)->relations,c,0);
    }
    break;
  default:
    PANIC_INCORRECT_TYPE;
  }
}

/*------------------------------------------------------------------------------
  LOGICAL RELATION LIST STUFF
*/

unsigned long LogRelationsCount(CONST struct Instance *i)
{
  assert(i!=NULL);
  AssertMemory(i);
  switch(i->t) {
  case BOOLEAN_ATOM_INST:
    if (BA_INST(i)->logrelations!=NULL) {
      return gl_length(BA_INST(i)->logrelations);
    }else{
      return 0;
    }
  case REL_INST:
    if (RELN_INST(i)->logrels!=NULL) {
      return gl_length(RELN_INST(i)->logrels);
    }else{
      return 0;
    }
  case LREL_INST:
    if (LRELN_INST(i)->logrels!=NULL) {
      return gl_length(LRELN_INST(i)->logrels);
    }else{
      return 0;
    }
  default:
    PANIC_INCORRECT_TYPE;
  }
}

struct Instance *LogRelationsForInstance(CONST struct Instance *i,
			                 unsigned long int c)
{
  assert((i!=NULL)&&(c>0)&&(c<=LogRelationsCount(i)));
  AssertMemory(i);
  switch(i->t){
  case BOOLEAN_ATOM_INST:
    if (BA_INST(i)->logrelations!=NULL) {
      return INST(gl_fetch(BA_INST(i)->logrelations,c));
    }else{
      Asc_Panic(2, NULL, "c out of bounds in LogRelationsForInstance.\n");
    }
  case REL_INST:
    if (RELN_INST(i)->logrels!=NULL) {
      return INST(gl_fetch(RELN_INST(i)->logrels,c));
    }else{
      Asc_Panic(2, NULL, "c out of bounds in LogRelationsForInstance.\n");
    }
  case LREL_INST:
    if (LRELN_INST(i)->logrels!=NULL) {
      return INST(gl_fetch(LRELN_INST(i)->logrels,c));
    }else{
      Asc_Panic(2, NULL, "c out of bounds in LogRelationsForInstance.\n");
    }
  default:
    PANIC_INCORRECT_TYPE;
  }
  exit(2);/* NOT REACHED.  Needed to keep gcc from whining */
}

void AddLogRel(struct Instance *i, struct Instance *lreln)
{
  unsigned long len;
  assert(i&&lreln&&(lreln->t==LREL_INST));
  AssertMemory(i);
  switch(i->t){
  case BOOLEAN_ATOM_INST:
    if (BA_INST(i)->logrelations==NULL) {
      BA_INST(i)->logrelations = gl_create(AVG_LOGRELS);
    }
    len = gl_length(BA_INST(i)->logrelations);
    if (gl_search(BA_INST(i)->logrelations,
                  (char *)lreln,(CmpFunc)CmpLogRelations)==0) {
      gl_append_ptr(BA_INST(i)->logrelations,(VOIDPTR)lreln);
    }
    break;
  case REL_INST:
    if (RELN_INST(i)->logrels==NULL) {
      RELN_INST(i)->logrels = gl_create(AVG_LOGRELS);
    }
    len = gl_length(RELN_INST(i)->logrels);
    if (gl_search(RELN_INST(i)->logrels,
                  (char *)lreln,(CmpFunc)CmpLogRelations)==0) {
      gl_append_ptr(RELN_INST(i)->logrels,(VOIDPTR)lreln);
    }
    break;
  case LREL_INST:
    if (LRELN_INST(i)->logrels==NULL) {
      LRELN_INST(i)->logrels = gl_create(AVG_LOGRELS);
    }
    len = gl_length(LRELN_INST(i)->logrels);
    if (gl_search(LRELN_INST(i)->logrels,
                  (char *)lreln,(CmpFunc)CmpLogRelations)==0) {
      gl_append_ptr(LRELN_INST(i)->logrels,(VOIDPTR)lreln);
    }
    break;
  default:
    PANIC_INCORRECT_TYPE;
  }
}

void RemoveLogRel(struct Instance *i, struct Instance *lreln)
{
  register unsigned long c;
  assert(i&&lreln&&(lreln->t==LREL_INST));
  AssertMemory(i);
  switch(i->t) {
  case BOOLEAN_ATOM_INST:
    if (BA_INST(i)->logrelations==NULL) {
      return;
    }
    c = gl_search(BA_INST(i)->logrelations,
                 (char *)lreln,(CmpFunc)CmpLogRelations);
    if (c>0) {
      gl_delete(BA_INST(i)->logrelations,c,0);
    }
    break;
  case REL_INST:
    if (RELN_INST(i)->logrels==NULL) {
      return;
    }
    c = gl_search(RELN_INST(i)->logrels,
                 (char *)lreln,(CmpFunc)CmpLogRelations);
    if (c>0) {
      gl_delete(RELN_INST(i)->logrels,c,0);
    }
    break;
  case LREL_INST:
    if (LRELN_INST(i)->logrels==NULL) {
      return;
    }
    c = gl_search(LRELN_INST(i)->logrels,
                 (char *)lreln,(CmpFunc)CmpLogRelations);
    if (c>0) {
      gl_delete(LRELN_INST(i)->logrels,c,0);
    }
    break;
  default:
    PANIC_INCORRECT_TYPE;
  }
}

/*------------------------------------------------------------------------------
  'WHEN' LIST STUFF
*/

unsigned long WhensCount(struct Instance *i)
{
  assert(i!=NULL);
  AssertMemory(i);
  switch(i->t) {
  case BOOLEAN_ATOM_INST:
    if (BA_INST(i)->whens!=NULL) {
      return gl_length(BA_INST(i)->whens);
    }else{
      return 0;
    }
  case INTEGER_ATOM_INST:
    if (IA_INST(i)->whens!=NULL) {
      return gl_length(IA_INST(i)->whens);
    }else{
      return 0;
    }
  case SYMBOL_ATOM_INST:
    if (SYMA_INST(i)->whens!=NULL) {
      return gl_length(SYMA_INST(i)->whens);
    }else{
      return 0;
    }
  case BOOLEAN_CONSTANT_INST:
    if (BC_INST(i)->whens!=NULL) {
      return gl_length(BC_INST(i)->whens);
    }else{
      return 0;
    }
  case INTEGER_CONSTANT_INST:
    if (IC_INST(i)->whens!=NULL) {
      return gl_length(IC_INST(i)->whens);
    }else{
      return 0;
    }
  case SYMBOL_CONSTANT_INST:
    if (SYMC_INST(i)->whens!=NULL) {
      return gl_length(SYMC_INST(i)->whens);
    }else{
      return 0;
    }
  case MODEL_INST:
    if (MOD_INST(i)->whens!=NULL) {
      return gl_length(MOD_INST(i)->whens);
    }else{
      return 0;
    }
  case REL_INST:
    if (RELN_INST(i)->whens!=NULL) {
      return gl_length(RELN_INST(i)->whens);
    }else{
      return 0;
    }
  case LREL_INST:
    if (LRELN_INST(i)->whens!=NULL) {
      return gl_length(LRELN_INST(i)->whens);
    }else{
      return 0;
    }
  case WHEN_INST:
    if (W_INST(i)->whens!=NULL) {
      return gl_length(W_INST(i)->whens);
    }else{
      return 0;
    }
  default:
    PANIC_INCORRECT_TYPE;
  }
}


struct Instance *WhensForInstance(struct Instance *i,
			      unsigned long int c)
{
  assert((i!=NULL)&&(c>0)&&(c<=WhensCount(i)));
  AssertMemory(i);
  switch(i->t) {
  case BOOLEAN_ATOM_INST:
    if (BA_INST(i)->whens!=NULL)
      return INST(gl_fetch(BA_INST(i)->whens,c));
    else{
      Asc_Panic(2, NULL, "c out of bounds in WhensForInstance.\n");
    }
  case INTEGER_ATOM_INST:
    if (IA_INST(i)->whens!=NULL) {
      return INST(gl_fetch(IA_INST(i)->whens,c));
    }else{
      Asc_Panic(2, NULL, "c out of bounds in WhensForInstance.\n");
    }
  case SYMBOL_ATOM_INST:
    if (SYMA_INST(i)->whens!=NULL) {
      return INST(gl_fetch(SYMA_INST(i)->whens,c));
    }else{
      Asc_Panic(2, NULL, "c out of bounds in WhensForInstance.\n");
    }
  case BOOLEAN_CONSTANT_INST:
    if (BC_INST(i)->whens!=NULL) {
      return INST(gl_fetch(BC_INST(i)->whens,c));
    }else{
      Asc_Panic(2, NULL, "c out of bounds in WhensForInstance.\n");
    }
  case INTEGER_CONSTANT_INST:
    if (IC_INST(i)->whens!=NULL) {
      return INST(gl_fetch(IC_INST(i)->whens,c));
    }else{
      Asc_Panic(2, NULL, "c out of bounds in WhensForInstance.\n");
    }
  case SYMBOL_CONSTANT_INST:
    if (SYMC_INST(i)->whens!=NULL) {
      return INST(gl_fetch(SYMC_INST(i)->whens,c));
    }else{
      Asc_Panic(2, NULL, "c out of bounds in WhensForInstance.\n");
    }
  case MODEL_INST:
    if (MOD_INST(i)->whens!=NULL) {
      return INST(gl_fetch(MOD_INST(i)->whens,c));
    }else{
      Asc_Panic(2, NULL, "c out of bounds in WhensForInstance.\n");
    }
  case REL_INST:
    if (RELN_INST(i)->whens!=NULL) {
      return INST(gl_fetch(RELN_INST(i)->whens,c));
    }else{
      Asc_Panic(2, NULL, "c out of bounds in WhensForInstance.\n");
    }
  case LREL_INST:
    if (LRELN_INST(i)->whens!=NULL) {
      return INST(gl_fetch(LRELN_INST(i)->whens,c));
    }else{
      Asc_Panic(2, NULL, "c out of bounds in WhensForInstance.\n");
    }
  case WHEN_INST:
    if (W_INST(i)->whens!=NULL) {
      return INST(gl_fetch(W_INST(i)->whens,c));
    }else{
      Asc_Panic(2, NULL, "c out of bounds in WhensForInstance.\n");
    }
  default:
    PANIC_INCORRECT_TYPE;
  }
  exit(2);/* NOT REACHED.  Needed to keep gcc from whining */
}


void AddWhen(struct Instance *i, struct Instance *when)
{
  unsigned long len;
  assert(i&&when&&(when->t==WHEN_INST));
  AssertMemory(i);
  switch(i->t){
  case BOOLEAN_ATOM_INST:
    if (BA_INST(i)->whens==NULL)
      BA_INST(i)->whens = gl_create(AVG_WHEN);
    len = gl_length(BA_INST(i)->whens);
    if (gl_search(BA_INST(i)->whens,
                 (char *)when,(CmpFunc)CmpWhens)==0)
      gl_append_ptr(BA_INST(i)->whens,(VOIDPTR)when);
    break;
  case INTEGER_ATOM_INST:
    if (IA_INST(i)->whens==NULL)
      IA_INST(i)->whens = gl_create(AVG_WHEN);
    len = gl_length(IA_INST(i)->whens);
    if (gl_search(IA_INST(i)->whens,
                 (char *)when,(CmpFunc)CmpWhens)==0)
      gl_append_ptr(IA_INST(i)->whens,(VOIDPTR)when);
    break;
  case SYMBOL_ATOM_INST:
    if (SYMA_INST(i)->whens==NULL)
      SYMA_INST(i)->whens = gl_create(AVG_WHEN);
    len = gl_length(SYMA_INST(i)->whens);
    if (gl_search(SYMA_INST(i)->whens,
                 (char *)when,(CmpFunc)CmpWhens)==0)
      gl_append_ptr(SYMA_INST(i)->whens,(VOIDPTR)when);
    break;
  case BOOLEAN_CONSTANT_INST:
    if (BC_INST(i)->whens==NULL)
      BC_INST(i)->whens = gl_create(AVG_WHEN);
    len = gl_length(BC_INST(i)->whens);
    if (gl_search(BC_INST(i)->whens,
                 (char *)when,(CmpFunc)CmpWhens)==0)
      gl_append_ptr(BC_INST(i)->whens,(VOIDPTR)when);
    break;
  case INTEGER_CONSTANT_INST:
    if (IC_INST(i)->whens==NULL)
      IC_INST(i)->whens = gl_create(AVG_WHEN);
    len = gl_length(IC_INST(i)->whens);
    if (gl_search(IC_INST(i)->whens,
                 (char *)when,(CmpFunc)CmpWhens)==0)
      gl_append_ptr(IC_INST(i)->whens,(VOIDPTR)when);
    break;
  case SYMBOL_CONSTANT_INST:
    if (SYMC_INST(i)->whens==NULL)
      SYMC_INST(i)->whens = gl_create(AVG_WHEN);
    len = gl_length(SYMC_INST(i)->whens);
    if (gl_search(SYMC_INST(i)->whens,
                 (char *)when,(CmpFunc)CmpWhens)==0)
      gl_append_ptr(SYMC_INST(i)->whens,(VOIDPTR)when);
    break;
  case MODEL_INST:
    if (MOD_INST(i)->whens==NULL)
      MOD_INST(i)->whens = gl_create(AVG_WHEN);
    len = gl_length(MOD_INST(i)->whens);
    if (gl_search(MOD_INST(i)->whens,
                 (char *)when,(CmpFunc)CmpWhens)==0)
      gl_append_ptr(MOD_INST(i)->whens,(VOIDPTR)when);
    break;
  case REL_INST:
    if (RELN_INST(i)->whens==NULL)
      RELN_INST(i)->whens = gl_create(AVG_WHEN);
    len = gl_length(RELN_INST(i)->whens);
    if (gl_search(RELN_INST(i)->whens,
                 (char *)when,(CmpFunc)CmpWhens)==0)
      gl_append_ptr(RELN_INST(i)->whens,(VOIDPTR)when);
    break;
  case LREL_INST:
    if (LRELN_INST(i)->whens==NULL)
      LRELN_INST(i)->whens = gl_create(AVG_WHEN);
    len = gl_length(LRELN_INST(i)->whens);
    if (gl_search(LRELN_INST(i)->whens,
                 (char *)when,(CmpFunc)CmpWhens)==0)
      gl_append_ptr(LRELN_INST(i)->whens,(VOIDPTR)when);
    break;
  case WHEN_INST:
    if (W_INST(i)->whens==NULL)
      W_INST(i)->whens = gl_create(AVG_WHEN);
    len = gl_length(W_INST(i)->whens);
    if (gl_search(W_INST(i)->whens,
                 (char *)when,(CmpFunc)CmpWhens)==0)
      gl_append_ptr(W_INST(i)->whens,(VOIDPTR)when);
    break;
  default:
    PANIC_INCORRECT_TYPE;
  }
}

void RemoveWhen(struct Instance *i, struct Instance *when)
{
  register unsigned long c;
  assert(i&&when&&(when->t==WHEN_INST));
  AssertMemory(i);
  switch(i->t) {
  case BOOLEAN_ATOM_INST:
    if (BA_INST(i)->whens==NULL) return;
    c = gl_search(BA_INST(i)->whens,
                 (char *)when,(CmpFunc)CmpWhens);
    if (c>0) gl_delete(BA_INST(i)->whens,c,0);
    break;
  case INTEGER_ATOM_INST:
    if (IA_INST(i)->whens==NULL) return;
    c = gl_search(IA_INST(i)->whens,
                 (char *)when,(CmpFunc)CmpWhens);
    if (c>0) gl_delete(IA_INST(i)->whens,c,0);
    break;
  case SYMBOL_ATOM_INST:
    if (SYMA_INST(i)->whens==NULL) return;
    c = gl_search(SYMA_INST(i)->whens,
                 (char *)when,(CmpFunc)CmpWhens);
    if (c>0) gl_delete(SYMA_INST(i)->whens,c,0);
    break;
  case BOOLEAN_CONSTANT_INST:
    if (BC_INST(i)->whens==NULL) return;
    c = gl_search(BC_INST(i)->whens,
                 (char *)when,(CmpFunc)CmpWhens);
    if (c>0) gl_delete(BC_INST(i)->whens,c,0);
    break;
  case INTEGER_CONSTANT_INST:
    if (IC_INST(i)->whens==NULL) return;
    c = gl_search(IC_INST(i)->whens,
                 (char *)when,(CmpFunc)CmpWhens);
    if (c>0) gl_delete(IC_INST(i)->whens,c,0);
    break;
  case SYMBOL_CONSTANT_INST:
    if (SYMC_INST(i)->whens==NULL) return;
    c = gl_search(SYMC_INST(i)->whens,
                 (char *)when,(CmpFunc)CmpWhens);
    if (c>0) gl_delete(SYMC_INST(i)->whens,c,0);
    break;
  case MODEL_INST:
    if (MOD_INST(i)->whens==NULL) return;
    c = gl_search(MOD_INST(i)->whens,
                 (char *)when,(CmpFunc)CmpWhens);
    if (c>0) gl_delete(MOD_INST(i)->whens,c,0);
    break;
  case REL_INST:
    if (RELN_INST(i)->whens==NULL) return;
    c = gl_search(RELN_INST(i)->whens,
                 (char *)when,(CmpFunc)CmpWhens);
    if (c>0) gl_delete(RELN_INST(i)->whens,c,0);
    break;
  case LREL_INST:
    if (LRELN_INST(i)->whens==NULL) return;
    c = gl_search(LRELN_INST(i)->whens,
                 (char *)when,(CmpFunc)CmpWhens);
    if (c>0) gl_delete(LRELN_INST(i)->whens,c,0);
    break;
  case WHEN_INST:
    if (W_INST(i)->whens==NULL) return;
    c = gl_search(W_INST(i)->whens,
                 (char *)when,(CmpFunc)CmpWhens);
    if (c>0) gl_delete(W_INST(i)->whens,c,0);
    break;
  default:
    PANIC_INCORRECT_TYPE;
  }
}


