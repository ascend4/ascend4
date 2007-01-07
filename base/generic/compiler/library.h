/*	ASCEND modelling environment
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
	Copyright (C) 2006 Carnegie Mellon University

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
	Type Definition Library.

	Implements a type definition library for use by the ASCEND compiler.
	Each type encountered during parsing is converted to a TypeDescription
	and stored in the main type library maintained by this module.
	The type library will only maintain one definition for a given type
	name.  The library will complain if you try to add two types with the
	same name unless it happens when reloading a module.

	Requires:
	#include "utilities/ascConfig.h"
	#include "compiler/compiler.h"
	#include "compiler/type_desc.h"
	#include "compiler/module.h"
	#include "general/list.h"
*//*
	by Tom Epperly
	Created: 1/12/90
	Version: $Revision: 1.17 $
	Version control file: $RCSfile: library.h,v $
	Date last modified: $Date: 1998/04/16 00:43:24 $
	Last modified by: $Author: ballan $
*/

#ifndef ASC_LIBRARY_H
#define ASC_LIBRARY_H

/**	@addtogroup compiler Compiler
	@{
*/

#include <utilities/ascConfig.h>

/** For use in constructing hierarchies. */
struct HierarchyNode {
  struct TypeDescription *desc;   /**< The type at this node. */
  struct gl_list_t *descendents;  /**< The list of refinements of desc. */
};

extern void InitializeLibrary(void);
/**<
 *  Initializes the library.
 *  This should be called before using any other functions
 *  or globals in this module.  Call DestroyLibrary() when
 *  finished with the library.
 */

ASC_DLLSPEC void DestroyLibrary(void);
/**<
 *  Cleans up after library use.
 *  Do not use any other functions or globals in this module after
 *  using this function until InitializeLibrary() is called.
 */

ASC_DLLSPEC struct TypeDescription*FindType(symchar *name);
/**<
 *  Finds the type description associated with name.
 *  Returns NULL if unable to locate the type.
 *  Handles NULL input gracefully.
 *
 *  @param name The type name to look up.
 *  @return The type description having the specified name,
 *          or NULL if none have that name.
 */

extern struct TypeDescription *FindRelationType(void);
/**<
 *  Finds the type description associated with real relations.
 *  Returns NULL if never defined, which means someone forgot to
 *  load a system.lib equivalent.
 */

extern struct TypeDescription *FindLogRelType(void);
/**<
 *  Finds the type description associated with logical relations.
 *  Returns NULL if never defined, which means someone forgot to
 *  load a system.lib equivalent.
 */

extern struct TypeDescription *FindSetType(void);
/**<
 *  Finds the type description associated with set statements.
 *  Returns NULL if never defined, which is an extreme error.
 */

extern struct TypeDescription *FindDummyType(void);
/**<
 *  Finds the type description associated with unselected statements.
 *  Returns NULL if never defined, which is an extreme error.
 */

extern struct TypeDescription *FindWhenType(void);
/**<
 *  Finds the type description associated with WHEN statements.
 *  Returns NULL if never defined, which is an extreme error.
 */

extern struct TypeDescription *FindExternalType(void);
/**<
 *  Finds the type description associated with external statements.
 *  Returns NULL if never defined, which is an extreme error.
 */

extern int AddType(struct TypeDescription *desc);
/**<
 *  Adds a type to the library.
 *  The type is not added if it is already present in the
 *  type library.  Otherwise, the library takes ownership of
 *  desc and adds it to the library.
 *
 *  @param desc The type to add to the library.
 *  @return Returns 1 if the new type is added and kept,
 *          0 if it was already present.
 */

ASC_DLLSPEC struct gl_list_t *FindFundamentalTypes(void);
/**<
 *  Creates a gl_list_t containing pointers to the fundamental
 *  types.  Destruction of the returned list (but not it's
 *  contents) is the responsibility of the caller.
 *
 *  @return A gl_list_t of (struct TypeDescription *) to the
 *          fundamental types.
 */

ASC_DLLSPEC struct gl_list_t*Asc_TypeByModule(CONST struct module_t *module);
/**<
 *  Builds a list of type names defined in module that are already
 *  present in the main type library.  Destruction of the returned
 *  list (but not its contents) is the responsibility of the caller.
 *
 *  @param module The module to parse for defined type names.
 *  @return A gl_list_t of (symchar *) to the names of types found
 *          both in module and in the main type library.
 */

ASC_DLLSPEC struct gl_list_t *TypesThatRefineMe (symchar *name);
/**<
 *  Builds a list of type names in the main type library which refine
 *  the type having the specified name.  The returned list includes
 *  immediate refinements only, not all of the refinements in a chain
 *  such as a<-b<-c<-d.  Given a, only b is included in the list.
 *  This should be an expensive function and could be made more
 *  efficient for certain atomic types.  Destruction of the returned
 *  list (but not its contents) is the responsibility of the caller.
 *
 *  @param name The type name to check for registered refinements.
 *  @return A gl_list_t of (symchar *) to the names of types in the main
 *          type library which refine the specified type.
 */

ASC_DLLSPEC struct gl_list_t *AllTypesThatRefineMe_Flat (symchar *name);
/**<
 *  Builds a list of all type names in the main type library which
 *  refine the type having the specified name.  The returned list
 *  includes all refinements in a chain such as a<-b<-c<-d.  Given a,
 *  b, c, and d are all included in the list.  This is only slightly
 *  more expensive than TypesThatRefineMe().  For efficiency atoms
 *  and models are handled differently.  Destruction of the returned
 *  list (but not its contents) is the responsibility of the caller.
 *
 *  @param name The type name to check for registered refinements.
 *  @return A gl_list_t of (symchar *) to the names of types in the main
 *          type library which refine the specified type.
 */

ASC_DLLSPEC struct HierarchyNode *AllTypesThatRefineMe_Tree (symchar *name);
/**<
 *  Builds a tree of HierarchyNodes all types in the main type library
 *  which refine the type having the specified name.  This is similar to
 *  AllTypesThatRefineMe_Flat() except that the results are returned in
 *  a HierarchyNodes tree.  This is somewhat pricey in terms of the number
 *  of pointer comparisons.   For efficiency atoms and models are handled
 *  separately.  The first node returned is the node of the type given.
 *  If it is null, the type given was not found.  Destruction of the
 *  returned tree is the responsibility of the caller.  Use
 *  DestroyHierarchyNode() for this purpose.
 *
 *  @param name The type name to check for registered refinements.
 *  @return A tree of HierarchyNodes containing the types in the main
 *          type library which refine the specified type.
 */

ASC_DLLSPEC void DestroyHierarchyNode(struct HierarchyNode *heir);
/**<
 *  Deallocates (recursively) all the memory associated with a
 *  HierarchyNode.  Use this function to destroy the tree returned
 *  by AllTypesThatRefineMe_Tree().
 *
 *  @param heir Head of the HeirarchyNode tree to destroy.
 */

ASC_DLLSPEC int IsTypeRefined(CONST struct TypeDescription *desc);
/**<
 *  Check whether the specified type is refined by any other type in
 *  the main type library.
 *
 *  @param desc The type to check for refinements.
 *  @return Returns 1 if desc is refined, 0 otherwise.
 */

ASC_DLLSPEC struct gl_list_t *DefinitionList(void);
/**<
 *  Makes a sorted list of all registered definitions.
 *  In the case of there being two versions of a given type,
 *  the latest version is used.  This doesn't include array type
 *  definitions.  The user is responsible for destroying the
 *  returned list (but not its content).
 *
 *  @return A gl_list_t of (struct TypeDescription *) containing
 *          the sorted types.
 */

ASC_DLLSPEC unsigned int CheckFundamental(symchar *f);
/**<
 *  Checks whether string f is a fundamental type name.
 *  f must be from the symbol table (i.e. not an externally-defined
 *  character string).
 *
 *  @param f The string to check.
 *  @return Returns 1 if f is a fundamental type name, 0 if not.
 */

/* @} */

#endif /* ASC_LIBRARY_H */
