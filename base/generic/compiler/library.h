/*
 *  Type Definition Library
 *  by Tom Epperly
 *  Created: 1/12/90
 *  Version: $Revision: 1.17 $
 *  Version control file: $RCSfile: library.h,v $
 *  Date last modified: $Date: 1998/04/16 00:43:24 $
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */

#ifndef __LIBRARY_H_SEEN__
#define __LIBRARY_H_SEEN__


/*
 *  The type library will only maintain one definition for a given type
 *  name.  The library will complain if you try to add two types with the
 *  same name unless it happens when reloading a module.
 */

struct HierarchyNode {
  struct TypeDescription *desc;  /* the type at this node */
  struct gl_list_t *descendents; /* the list of refinements of desc */
};
/* for use in constructing hierarchies */

extern void InitializeLibrary(void);

extern struct TypeDescription *FindType(symchar *);
/*
 *  struct TypeDescription *FindType(name)
 *  const char *name;
 *  Find the type description associated with name.  Returns NULL if unable
 *  to locate the type. Handles NULL input gracefully.
 */

extern struct TypeDescription *FindRelationType(void);
/*
 *  struct TypeDescription *FindRelationType()
 *  Find the type description associated with real relations.
 *  Returns NULL if never defined, which means someone forgot to
 *  load a system.lib equivalent.
 */

extern struct TypeDescription *FindLogRelType(void);
/*
 *  struct TypeDescription *FindLogRelnType()
 *  Find the type description associated with logical relations.
 *  Returns NULL if never defined, which means someone forgot to
 *  load a system.lib equivalent.
 */

extern struct TypeDescription *FindSetType(void);
/*
 *  struct TypeDescription *FindWhenType()
 *  Find the type description associated with set statements.
 *  Returns NULL if never defined, which is an extreme error.
 */

extern struct TypeDescription *FindDummyType(void);
/*
 *  struct TypeDescription *FindDummyType()
 *  Find the type description associated with unselected statements.
 *  Returns NULL if never defined, which is an extreme error.
 */

extern struct TypeDescription *FindWhenType(void);
/*
 *  struct TypeDescription *FindWhenType()
 *  Find the type description associated with WHEN statements.
 *  Returns NULL if never defined, which is an extreme error.
 */

extern struct TypeDescription *FindExternalType(void);
/*
 *  struct TypeDescription *FindExternalType()
 *  Find the type description associated with external statements.
 *  Returns NULL if never defined, which is an extreme error.
 */

extern int AddType(struct TypeDescription *);
/*
 *  void AddType(desc)
 *  struct TypeDescription *desc;
 *  Add a type to the library.
 *  Returns 1 if kept the new type, 0 if already had it and kept old.
 */

extern void DestroyLibrary(void);

extern struct gl_list_t *FindFundamentalTypes(void);
/*
 *  struct gl_list_t *FindFundamentalTypes
 *  return a gl_list containing pointers to the fundamental types
 */

extern struct gl_list_t *Asc_TypeByModule(CONST struct module_t *);
/*
 *  struct gl_list_t *Asc_TypeByModule(module);
 *      const struct module_t *module;
 *
 *  Comments:
 *  Accepts a module and searches the main type library defined in
 *  library.c for the all types that were found in that module.
 *  Builds a list of the types found and returns it.
 */

extern struct gl_list_t *TypesThatRefineMe (symchar *);
/*
 *  struct gl_list_t *TypesThatRefineMe(name);
 *  symchar *name;
 *  Comments :
 *  Accepts the name of a type and searches the main library defined in
 *  library.c for all the types that refine the given type. Builds a list
 *  of all the types found and returns it. This should be an expensive
 *  function and could be made more efficient for certain atomic types.
 *  The list return is the list of immediate refinements only, not
 *  all of the refinements in a chain such as a<-b<-c<-d. Given a, only
 *  b is returned.
 */

extern struct gl_list_t *AllTypesThatRefineMe_Flat (symchar *);
/*
 *  struct gl_list_t *AllTypesThatRefineMe_Flat(name);
 *  symchar *name;
 *  Comments :
 *  Accepts the name of a type and searches the main library defined in
 *  library.c for all the types that refine the given type. Builds a list
 *  of all the types found and returns it. This is only slightly more
 *  expensive than TypesThatRefineMe.
 *  For efficiency atoms and models are handled differently.
 *  The list returned is the list of all refinements in a chain
 *  such as a<-b<-c<-d. if a is input, b,c,d are returned.
 */

extern struct HierarchyNode *AllTypesThatRefineMe_Tree (symchar *);
/*
 *  struct HierarchyNode *AllTypesThatRefineMe_Tree(name);
 *  const char *name;
 *  Comments :
 *  Accepts the name of a type and searches the main library defined in
 *  library.c for all the types that refine the given type. Builds a tree
 *  of HierarchyNodes of all the types found and returns it. This is somewhat
 *  pricey in terms of the number of pointer comparisons.
 *  For efficiency atoms and models are handled separately.
 *  The first node returned is the node of the type given. If it is null,
 *  the type given was not found.
 */

extern void DestroyHierarchyNode(struct HierarchyNode *);
/*
 *  struct HierarchyNode *hier;
 *  Comments:
 *  Deallocates (recursively) all the memory associated with a HierarchyNode.
 */

extern int IsTypeRefined(CONST struct TypeDescription *);
/*
 *  int IsTypeRefined(desc);
 *  CONST struct TypeDescription *desc;
 *  Comments :
 *  Searches the main library defined in library.c to see if the type desc,
 *  is refined by any other type, i.e., if the type is the root of a
 *  hierarchy. Returns 1 if TRUE, 0 otherwise.
 */

extern struct gl_list_t *DefinitionList(void);
/*
 *  Make a sorted list of all the definitions.  In the case of there being
 *  two versions of a given type, the latest version is used.  This doesn't
 *  include array type definitions.
 *  The user is responsible for destroying the list (but not its content).
 */

extern unsigned int CheckFundamental(symchar *f);
/*
 *  Return true if string is a fundamental type name.
 *  f must be from the symbol table.
 */
#endif /* __LIBRARY_H_SEEN__ */
