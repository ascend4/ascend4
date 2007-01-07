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
*//**
	@file
	Ascend Instance Tree Link Management

	Requires:
	#include "utilities/ascConfig.h"
	#include "instance_enum.h"
*//*
	by Ben Allan
	based on instance.c by Tom Epperly
	8/16/89
	Last in CVS: $Revision: 1.7 $ $Date: 1997/07/18 12:31:17 $ $Author: mthomas $
*/

#ifndef ASC_LINKINST_H
#define ASC_LINKINST_H

/**	addtogroup compiler Compiler
	@{
*/

extern void ChangeRelationPointers(struct Instance *rel,
                                   struct Instance *old,
                                   struct Instance *new);
/**<
	This procedure changes all references of "old" in relation
	instance rel to "new".
*/

extern void ChangeLogRelPointers(struct Instance *lrel,
                                 struct Instance *old,
                                 struct Instance *new);
/**<
	This procedure changes all references of "old" in logical relation
	instance lrel to "new".
*/

extern void ChangeWhenPointers(struct Instance *when,
                               struct Instance *old,
                               struct Instance *new);
/**<
	This procedure changes all references of "old" in when
	instance when to "new".
*/

extern void ChangeParent(struct Instance *parent,
                         struct Instance *oldchild,
                         struct Instance *newchild);
/**<
	Tell a parent to point to newchild instead of pointing to oldchild
*/

extern void ReDirectParents(struct Instance *oldinst, struct Instance *newinst);
/**< 
	Tell to all the parent of the oldchild, to point to newchild rather
	than to point to oldchild.
*/

extern void ReDirectChildren(struct Instance *oldinst, struct Instance *newinst);
/**< 
	Look at all of the children of the instance new; if old is one of
	their parents, delete the parent old and add the parent new.
*/

extern void ReorderChildrenPtrs(struct Instance **c,
                                CONST ChildListPtr oldinst,
                                CONST ChildListPtr newinst,
                                unsigned long int oldlen,
                                unsigned long int newlen);
/**<
	This expands the old child pointers packed into a MODEL
	struct into the new list, starting at the tail of the instance
	and copying until blanks are needed to accomodate new
	children, filling those blanks with NULL, and then continuing
	on up toward the top of the child list which comes right
	after the end of the struct ModelInstance defined in the header
	instance_types.h
*/

extern void FixCliques(struct Instance *old, struct Instance *new);
/**< 
	Substitute the old instance in a clique by the new instance. It is
	required after merging or refinement an instance, task which implies
	some instance is going to be destroyed.
*/

extern void FixRelations(struct RealAtomInstance *old, struct RealAtomInstance *new);
/**< 
	This is called to tell relations about a change in variable location
	e.g. If two atoms are merged, point all the relations that know about
	ATOM old to ATOM new.<br><br>
	
	A RealAtomInstance contains a gl_list of relations. Such a gl_list tells
	us in which relations this real atom appears. Basically it is a list
	of pointers. When merging two MODELS, for example, the atoms
	contained in one of the models are going to be destroyed. In such a
	case (and in many other involving merging and refinement) the relation
	using such a variable must be notified about the change. This is the
	goal of this function. It will vist the list of relations of a real atom
	instance and it will tell them to use the  the "new" instance instead of
	the "old" instance. For an explanation about how to handle different
	cases, see the function FixWhens below, thinking of course in list
	of relations instances rather than a list of when instances.
*/

extern void FixLogRelations(struct Instance *old, struct Instance *new);
/**<
	This is called to tell logrelations about a change in the location of a
	variable or a relation referenced in the logrelation. For example:

	A BOOLEAN_ATOM_INST contains a gl_list of logrelations. Such a gl_list
	tells us in which logrelations this boolean atom appears. Basically
	it is a list of pointers. When merging two MODELS, for example, the atoms
	contained in one of the models are going to be destroyed. In such a
	case (and in many other involving merging and refinement) the logrelation
	using such a variable must be notified about the change. This is the
	goal of this function. Similar situation applies for REL_INST and
	LREL_INST.<br><br>

	It will vist the list of logrelations of the instance and it will tell
	them to use the  the "new" instance instead of the "old" instance.<br><br>

	For an explanation about how to handle different
	cases, see the function FixWhens below, thinking of course in list
	of logrelations instances rather than a list of when instances.
*/

extern void FixWhens(struct Instance *old, struct Instance *new);
/**<
	A WHEN instance contains a list of variables and a list of CASEs. Also,
	each CASE contains a list of model and relation instances to be
	used if such a CASE applies. In the list of variables, instances
	allowed are
	        - BOOLEAN_ATOM_INST,
	        - INTEGER_ATOM_INST,
	        - SYMBOL_ATOM_INST,
	        - BOOLEAN_CONSTANT_INST,
	        - INTEGER_CONSTANT_INST, and
	        - SYMBOL_CONSTANT_INST.

	In the list of instances of each CASE, we allow
	        - MODEL_INST,
	        - REL_INST,
	        - LREL_INST,
	        - WHEN_INST.

	For purposes of Merging and refining, an instance must know if it
	appears in some of the lists of instances of a WHEN instance. That is,
	all the instances listed above contain a when field, which is a
	gl_list. This gl_list tells us which and how many WHEN statements use
	the instance.<br><br>

	So, in the process of merging two instances , for example,  we keep
	only one of the instances (the most refined) and destroy the other.
	If the instance that we are going to destroy is used in some
	WHEN statement, then we should tell those WHEN about the change.
	That is what this function does. It takes two instance structures
	as argument, old and new. Then,
	-# It goes through the list of whens of the NEW instance and tells
	    the WHENs contained in such a list that they must point to the instance
	    new instead of the instance old.
	-# Actually there are some modifications to the previous case.
	    The function can also go through the list of whens of the
	    OLD instance and tell the WHENs contained in such a list that
	    they must point to the instance new instead of the instance old.

	So, which list of when we are going to visit depends in what we want.
	What is always the same however, is the fact that the WHEN has to
	point the instance new rather than the instance old at the end of
	the process, since the instance old is, in general, going to be
	destroyed.<br><br>

	The following cases were anticipated:
	-# If the list of whens of the instance new is NULL, this list will
	    be set equal to the list of whens of the old instance and then
	    the list of whens of the instance new should be visited.
	    This may happen when we are refining ( booleans, integer, symbols
	    or relations, but not models)

	-# If the list of when of new and old is the same, it does not
	    matter which list of when we have to visit, we visit the list
	    of whens of the new instance. This may happen when we are
	    merging instances of the same type.

	-# If the list of whens of the new instance is not null and is
	    different to the list of whens of the old instance, we will
	    visit the list of whens of the old instance, perform the
	change of the pointers (new instead of old), and then enlarge
	    the list of whens of the new instance with the WHENs of the
	old instance. Example
	<pre>
	old->whens contains   when1, when2  (also when1 and when2 point to old)
	new->whens contains   when3, when4  (also when3 and when4 point to new)

	old is going to be destroyed, it is going to be subtituted by new.
	Then, changing pointers

	when1 and when2 will point to  new

	Therefore, new must know that when1 and when2 point to it now

	new->whens  will contain  when1,when2,when3,when4

	This may happen when we are merging two instances, one more
	refined than another. Remeber again, old is going to be destroyed.
	</pre>
*/

extern void FixWhensForRefinement(struct Instance *old, struct Instance *new);
/**<
	This function is almost equal to the previous function, the difference
	is that it is specific for the refinement of a MODEL. In such a case,
	the instance new is the result of a reallocation of memory of the
	instance old. Therefore, the list of whens of the old instance is NULL,
	and the list of WHEN of the new instance is not NULL. This would fall
	in case 3 of the previous function, which involves visiting the list
	of whens of the old instance (which is NULL).That is precisely
	the reason for which we wrote this other function, we want to visit
	the list of whens of the new instance instead.
*/

/* @} */

#endif  /* ASC_LINKINST_H */

