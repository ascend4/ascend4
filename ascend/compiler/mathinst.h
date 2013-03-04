/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//**
	@file
	Ascend Instance Math Object Functions.
	
	Relation, logical relation, and When querying and assignment routines

	Requires:
	#include "utilities/ascConfig.h"
	#include "fractions.h"
	#include "instance_enum.h"
	#include "compiler.h"
	#include "dimen.h"
	#include "expr_types.h"
*//*
	by Tom Epperly
	8/16/89
	Last in CVS: $Revision: 1.8 $ $Date: 1998/05/06 17:33:37 $ $Author: ballan $
*/

#ifndef ASC_MATHINST_H
#define ASC_MATHINST_H

#include "expr_types.h"
#include "instance_enum.h"

/**	@addtogroup compiler_inst Compiler Instance Hierarchy
	@{
*/

ASC_DLLSPEC enum Expr_enum GetInstanceRelationType(CONST struct Instance *i);
/**<
	This returns the type of the relation associated with this relations
	instance (REL_INST).
	This should be one of e_token, e_opcode, e_glassbox or e_blackbox.
*/

ASC_DLLSPEC CONST struct relation *GetInstanceRelationOnly(CONST struct Instance *i);
/**<
	This returns the guts of a relation instance (REL_INST).
	These should be treated as read only usually because you don't
	know who is sharing those guts.
*/

ASC_DLLSPEC CONST struct relation *GetInstanceRelation(CONST struct Instance *i,
                                                  enum Expr_enum *type);
/**<
	This returns the value of a relation inst(REL_INST). It will fill in
	the type of the relation associated with this relation instance.
	This will be one of e_token, e_opcode, e_glassbox or e_blackbox,
	or possibly e_undefined.

	@TODO Important! This appears not to be to inverse of the rel_instance
	function in the solver! Why is that? -- JP
*/

extern struct relation *GetInstanceRelToModify(struct Instance *i,
                                               enum Expr_enum *type);
/**<
	This returns the value of a relation inst(REL_INST). In this function
	we do not get a CONST structure and therefore we can modify it. This
	is useful while marking conditional relations.
*/

extern void SetInstanceRelation(struct Instance *i,
                                         struct relation *rel,
                                         enum Expr_enum type);
/**< 
	Set the relation to rel.  rel will be incorporated into the instance,
	so rel should not be modified or destroyed. The type of the relation
	*has* to be provided.

	rel and type may be the NULL/e_undefined combination.
*/

ASC_DLLSPEC CONST struct logrelation *GetInstanceLogRelOnly(CONST struct Instance*i);
/**< 
	This returns the guts of a logical relation instance (LREL_INST).
	These should be treated as read only usually because you don't
	know who is sharing those guts.
*/

ASC_DLLSPEC CONST struct logrelation *GetInstanceLogRel(CONST struct Instance *i);
/**< 
	This returns the value of a logrelation inst(LREL_INST).
*/

extern struct logrelation *GetInstanceLogRelToModify(struct Instance *i);
/**< 
	This returns the value of a logrelation inst(LREL_INST). In this function
	we do not get a CONST structure and therefore we can modify it. This
	is useful while marking conditional logrelations.
*/

extern void SetInstanceLogRel(struct Instance *i, struct logrelation *lrel);
/**< 
	Set the logical relation to lrel.  rel will be incorporated into the
	instance, so rel should not be modified or destroyed.
*/

ASC_DLLSPEC struct gl_list_t *GetInstanceOperands(CONST struct Instance *i);
/**< 
	Returns list of vars/models/equations in a mathematical relationship.
	Accepts all instance kinds. returns list only if the query makes
	sense, but list may be empty. List is yours to destroy, but its
	contents are not.
*/

extern struct gl_list_t *GetInstanceWhenVars(CONST struct Instance *i);
/**< 
	This returns the list of variables associated with this when
	instance (WHEN_INST)
*/

extern struct gl_list_t *GetInstanceWhenCases(CONST struct Instance *i);
/**< 
	This returns the list of cases associated with this when
	instance (WHEN_INST)
*/

extern struct gl_list_t *GetInstanceWhens(CONST struct Instance *i);
/**< 
	This returns the list of whens associated with this instance.
	Instance can be a model, relation,when,boolean, integer or symbol.
*/

extern void SetWhenVarList(struct Instance *i, struct gl_list_t *whenvars);
/**< 
	Set the list of variables of a when instance to whenvars
*/

extern void SetWhenCases(struct Instance *i, struct gl_list_t *whencases);
/**< 
	Set the list of cases of a when instance to whencases
*/

/*------------------------------------------------------------------------------
	SPECIAL REAL ATOM FUNCTIONS
	
	Each real instance knows what mathematical relations that it appears in.
	That information can be acessed by the following routines.
	Same for discrete variables.
*/

ASC_DLLSPEC unsigned long RelationsCount(CONST struct Instance *i);
/**< 
	This will return the number of relations that instance "i" appears in.
	"i" must be a REAL_ATOM_INST.  Otherwise, this routine will
	bomb.
*/

ASC_DLLSPEC struct Instance *RelationsForAtom(CONST struct Instance *i,
                                         unsigned long c);
/**<
 	This routine will return the c'th relation in instance i's relation list.
	"i" must be a REAL_ATOM_INST and 0 < c <= RelationsCount(i).
*/

extern void AddRelation(struct Instance *i, struct Instance *reln);
/**< 
	Add the relation instance reln to instance i's relation list.  "i" must
	be of type REAL_ATOM_INST and reln must be of type REL_INST.
*/

extern void RemoveRelation(struct Instance *i, struct Instance *reln);
/**< 
	Remove reln from the relation list of variable instance 'i'. If reln isn't
	found in i's relation list, execution continues with a warning message.
*/

ASC_DLLSPEC unsigned long LogRelationsCount(CONST struct Instance *i);
/**< 
	This will return the number of logical relations that instance "i"
	appears in. "i" must be a BOOLEAN_ATOM_INST, REL_INST or LREL_INST
*/

ASC_DLLSPEC struct Instance *LogRelationsForInstance(CONST struct Instance *i,
                                                unsigned long c);
/**<
	This routine will return the c'th logical relation in instance i's
	logical relation list. "i" must be  BOOLEAN_ATOM_INST, REL_INST or
	LREL_INST  and  0 < c <= LogRelationsCount(i).
*/

extern void AddLogRel(struct Instance *i, struct Instance *reln);
/**< 
	Add the logical relation instance lreln to instance i's logical relation
	list.  "i" must be of type BOOLEAN_ATOM_INST, REL_INST or LREL_INST
	and lreln must be of type LREL_INST. REL_INST and LREL_INST are
	allowed because of the SATISFIED logical term.
*/

extern void RemoveLogRel(struct Instance *i, struct Instance *lreln);
/**< 
	Remove lreln from i's logical relation list.  If 'lreln' isn't found in
	i's logical relation list, execution continues with a warning message.
*/

ASC_DLLSPEC unsigned long WhensCount(struct Instance *i);
/**< 
	This will return the number of whens that instance "i" is referenced in.
*/

ASC_DLLSPEC struct Instance *WhensForInstance(struct Instance *i,
                                         unsigned long c);
/**<
	This routine will return the c'th when in instance i's when list.
	0 < c <= WhensCount(i).
*/

extern void AddWhen(struct Instance *i, struct Instance *when);
/**<
	Add the when instance when to instance i's when list.  when must be of
	type WHEN_INST.
*/

extern void RemoveWhen(struct Instance *i, struct Instance *when);
/**<
	Remove when from i's when list.  If 'when' isn't found in i's when
	list, execution continues with a warning message.
*/

/* @} */

#endif  /* ASC_MATHINST_H */

