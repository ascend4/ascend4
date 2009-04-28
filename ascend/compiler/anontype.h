/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright 1997, Carnegie Mellon University.

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
*//** @file
	'Anonymous type' classification functions.

	The idea of this module is to find and report groups of
	isomorphic instances (instances with the same Anonymous Type).
	We do not actually produce type definitions, or anything like them,
	but instead use various sorting techniques. Note that anonymous
	type is a dynamic quantity in the ascend language definition which
	allows deferred binding.
	This should be named something like isomorph.c, but half the compiler
	files start with i already.
	These functions are thread-safe, if the instance data
	structures are, as this module creates no global variables.
	At present, instance structures are not thread-safe unless the
	entire global simulations list is locked into one thread.

	Ultimately the user will be returned a gl_list of AnonType pointers,
	where each pointer knows how many occurrences of some identical AT
	there are and where those occurences are. This list will become
	invalid if any compiler actions anywhere in the simulation universe
	change any of the instances in such a way that any instance is
	MOVED (e.g. by realloc) or modified independently of all other
	instances in the same anonymous type group.

	The Instances are NOT left with a record of their anonymous type.
	Each AT has a unique index number, which is its position in the
	gl_list returned. There is no useful ordering of the gl_list returned.
	Each AT knows its formal family as a doubly linked list, and each
	AT has a counter, visited, that the user can do arbitrary things
	with. The doubly linked list is ordered in some way that is an
	artifact of the classification algorithm. The head and tail of
	the linked list are NULL.

	Special cases:

	-# Relations, logical relations, and when statements are always
	   anonymous in the following sense:

	   They are more like atomic types. Their true 'formal type',
	   if you like, is the statement defining them since their
	   basic DEFINITIONS are universal. If you accept this, then
	   you can easily see also that the 'anonymous type' (the
	   expanded token form) of a relation is entirely dependent
	   on the anonymous type of the relation's context. This also
	   holds for logical relations and when structures.

	   So, in the collected 'anonymous types', there will be only as
	   many AT's devoted to relations as there are formal definitions
	   present in the system for relation (normally just 1).
	   The AT for each formal relation definition present will have
	   an instance list of all relations from that definition.

	   To find cliques of relations, just look at cliques of models.
	   If this handling of relations is insufficient, then the
	   statement of the relation could be used in a secondary slot
	   of the AT. To find exact relation groups requires a two pass
	   algorithm.

	-# Arrays. ASCEND arrays just suck. The approach here is
	   conservative; there may be fewer array cliques than we can
	   compute with a single pass algorithm.

	-# Undefined constants. Constant reals/integers which are undefined
	   are sorted as if they have DBL_MAX/LONG_MAX for a value. We are
	   assuming there are no meaningful constants for modeling
	   which take on the maximum value of the machine representation.

	Requires
	#include <stdio.h>
	       #include "utilities/ascConfig.h"
	       #include "general/list.h"
	       #include "compiler/instance_enum.h"
	
*//*
	By Benjamin Andrew Allan
	Created August 30, 1997.
	Last in CVS: $Revision: 1.5 $ $Date: 1998/02/05 22:23:20 $ $Author: ballan $
*/

#ifndef ASC_ANONTYPE_H
#define ASC_ANONTYPE_H

/**	@addtogroup compiler Compiler
	@{
*/

/** For debugging set =1, for performance set = 0 */
#define ATDEBUG 0

/** abbreviated AT in comments */
struct AnonType {
   unsigned long index;           /**< index in the gllist of ATs
                                      0 -> not in list. -1 == freed. */
   struct gl_list_t *instances;   /**< list of this AT's instances */
   struct AnonType *prev, *next;  /**< other ATs related by
                                       all being the same formally. */
   int exactfamily;               /**< group id of anonymous types which are
                                       identical except for merged substructure. */
   int visited;                   /**< counter for arbitrary use. */
};

/**
 * The initial number of instances to expect for each AT.
 * Tuning this may be hard. E.g. relations and some atoms
 * will grow to a huge number, but many constant types will
 * only have 1-3 instances shared universally.
 * In a distillation MODEL there are ~400 anon types
 * and most of these fall in the constant types.
 */
#define INSTANCES_PER_AT 2

/**
 * Define the expected maximum number of anonymous types.
 * This should not be too low or the code will do memory
 * reallocation of gl_list, atl.
 */
#define ANONEXPECTED 200

/**
 * Define the size for the hash tables keyed by the
 * formal type name and secondarily keyed by type ptr.
 * The secondary key is required because mixed simulations
 * may be created by pathological users and we must not
 * get confused when classifying.
 * One such hash table is used while classifying an instance tree
 * into AT groups. The user never sees this table, however.
 * This size must be even power of 2 and shouldn't be messed with.
 */
#define ANONTABLESIZE 1024

/**
 * Return the nth element of anon type list n.
 * The return type is <code>AnonType*</code>.
 */
#define Asc_GetAnonType(atl,n) \
  ((struct AnonType *)gl_fetch((atl),(n)))

/**
 * Return the number of instances associated with the nth position
 * in anonymous type list n.  The return value is type
 * <code>unsigned long</code>.
 */
#define Asc_GetAnonCount(atl,n) \
  gl_length(Asc_GetAnonType((atl),(n))->instances)

/**
 * Returns the cth member of an anonymous type clique
 * represented by at.  The return value is type <code>Instance*</code>.
 * @param at  an AnonType*, such as returned by: at = Asc_GetAnonType(atl,n);
 * @param  c  index:  1 <= c <= Asc_GetAnonCount(atl,n)
 */
#define Asc_GetAnonTypeInstance(at,c) \
  (struct Instance *)gl_fetch((at)->instances,(c))

/**
 * Returns the first element of the instance list associated with the
 * AT given. Does not normally return unless there is such a creature.
 */
extern struct Instance *Asc_GetAnonPrototype(struct AnonType *at);

/**
 * Destroys the anonlist we returned from a call to classify.
 * Do not destroy this list any other way.
 */
ASC_DLLSPEC void Asc_DestroyAnonList(struct gl_list_t *atl);

/**
 * This function classifies an instance tree
 * and returns the list described above.
 * root may be at any place in an instance tree.
 * The list should be destroyed with Asc_DestroyAnonList.
 */
ASC_DLLSPEC struct gl_list_t *Asc_DeriveAnonList(struct Instance *root);

/**
 * Writes a list of anon type info in atl, mainly for debugging purposes,
 * to the file given. root is the instance that Asc_DeriveAnonList
 * was called with, for the proper writing of names.
 * If root is NULL, arbitrary names will be made up.
 * If mlists != 0 and merge info exists, merge info will be written
 * with the AT info.
 */
ASC_DLLSPEC void Asc_WriteAnonList(FILE *fp,
                              struct gl_list_t *atl,
                              struct Instance *root,
                              int mlists);

/* @} */

#endif  /* ASC_ANONTYPE_H */

