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
	Name external definitions.

	@todo WHAT'S IN A NAME??? That which we call a rose
	By any other name would smell as sweet.

	A name is a linked list of symchars, integers, potentially
	unexpanded set definitions (array subscripts) and variable lists
	that gives the route another instance starting at some root
	instance i.  So if you have something's name and context instance, we
	know how to find the child (grandchild, etc) with the given name.
	A name without a context (either an instance or a type definition)
	means nothing.
*//*
	by Tom Epperly
	July 31, 1989
	Last in CVS: $Revision: 1.13 $ $Date: 1998/06/16 16:36:28 $ $Author: mthomas $
*/

#ifndef ASC_NAME_H
#define ASC_NAME_H

#include <ascend/general/platform.h>
#include "compiler.h"
#include "expr_types.h"

/**	@addtogroup compiler_stmt Compiler Statements
	@{
*/

#include <ascend/general/platform.h>

#define CreateIdName(s) CreateIdNameF((s),NAMEBIT_IDTY)
/**<
	Create name with Id flag.
	@see CreateIdNameF()
*/
#define CreateSystemIdName(s) CreateIdNameF((s),NAMEBIT_IDTY|NAMEBIT_AUTO)
/**<
	Create system name with Id+Auto flags.
	@see CreateIdNameF()
*/
ASC_DLLSPEC struct Name*CreateIdNameF(symchar *s, int bits);
/**<
	Create a name node with the identifier s
	and flag bits associated with it.  Implementation
	function for CreateIdName() and CreateSystemIdName().
*/

extern struct Name *CreateDerivName(struct DerName *der);
/**<
	Create a name node of type derivative with the derivative structure associated with it.
*/

extern struct DerName *CreateDeriv(struct VariableList *vlist);
/**<
	Create a derivative structure with the varlist vlist associated with it.
*/

extern struct Name *CreateSetName(struct Set *s);
/**<
	Create a name node of type set with the set s associated with it.
*/

extern struct Name *CreateEnumElementName(symchar *s);
/**<
	Create a name node of type set with the subscript s associated with it.
*/

extern struct Name *CreateIntegerElementName(long i);
/**<
	Create a name node of type set with the subscript i associated with it.
*/

extern struct Name *CreateReservedIndexName(symchar *reserved);
/**<
 * Make subscript index from the reserved identifier given, 
 * which in most uses will
 * contain an illegal character for an ascend ID reserving the
 * set index to internal compiler use only.
 */

extern void LinkNames(struct Name *cur, struct Name *next);
/**<
	Link "next" to cur so that NextName(cur) = next
*/

#ifdef NDEBUG
#define NextName(n) ((n)->next)
#else
#define NextName(n) NextNameF(n)
#endif
/**<
	Return the next attribute of n.
	@param n CONST struct Name*, Name to query.
	@return The next attribute as a struct Name*.
	@see NextNameF()
*/
extern struct Name *NextNameF(CONST struct Name *n);
/**<
	Return the next attribute of n.
	Implementation function for NextName().  Do not call this
	function directly - use NextName() instead.
*/

#ifdef NDEBUG
#define NameId(n) ((n)->bits & NAMEBIT_IDTY)
#else
#define NameId(n) NameIdF(n)
#endif
/**<
	Test whether a Name element is an identifier.
	We should have analogous functions for CHAT and ATTR, but since no
	clients yet use them, they aren't implemented.
	@param n CONST struct Name*, Name to query.
	@return An int:  NAMEBIT_IDTY if n is an identifier type Name or 0 otherwise.
	@see NameIdF()

	@note This answers for just the *first* link in the name.
		
*/

extern int NameIdF(CONST struct Name *n);
/**<
	Return NAMEBIT_IDTY if n is an identifier type Name or 0 otherwise.
	We should have analogous functions for CHAT and ATTR, but since no
	clients yet use them, they aren't implemented.
	Implementation function for NameId().  Do not call this
	function directly - use NameId() instead.
*/

#ifdef NDEBUG
#define NameDeriv(n) ((n)->bits & NAMEBIT_DERIV)
#else
#define NameDeriv(n) NameDerivF(n)
#endif
/**<
	Test whether a Name element is a derivative.
	We should have analogous functions for CHAT and ATTR, but since no
	clients yet use them, they aren't implemented.
	@param n CONST struct Name*, Name to query.
	@return An int:  NAMEBIT_DERIV if n is a derivative type Name or 0 otherwise.
	@see NameDerivF()

	@note This answers for just the *first* link in the name.
		
*/

extern int NameDerivF(CONST struct Name *n);
/**<
	Return NAMEBIT_DERIV if n is a derivative type Name or 0 otherwise.
	We should have analogous functions for CHAT and ATTR, but since no
	clients yet use them, they aren't implemented.
	Implementation function for NameDeriv().  Do not call this
	function directly - use NameDeriv() instead.
*/

#ifdef NDEBUG
#define NameHasder(n) ((n)->bits & NAMEBIT_HASDER)
#else
#define NameHasder(n) NameHasderF(n)
#endif
/**<
	Test whether a Name contains a derivative in the list of name elements.
	@param n CONST struct Name*, Name to query.
	@return An int:  NAMEBIT_HASDER if n contains a derivative name or 0 otherwise.
	@see NameHasderF()

	@note This answers for all links in the name.
		
*/

extern int NameHasderF(CONST struct Name *n);
/**<
	Return NAMEBIT_HASDER if n contains a derivative in the list of
        name elements or 0 otherwise.
	Implementation function for NameHasder().  Do not call this
	function directly - use NameHasder() instead.
*/

#ifdef NDEBUG
#define NameAuto(n) ((n)->bits & (NAMEBIT_AUTO|NAMEBIT_IDTY))
#else
#define NameAuto(n) NameAutoF(n)
#endif
/**<
	Test whether a Name is a system generated identifier.
	@param n CONST struct Name*, Name to query.
	@return An int:  NAMEBIT_AUTO if n is an system generated identifier
			type Name, or 0 otherwise.
	@see NameAutoF()
*/

extern int NameAutoF(CONST struct Name *n);
/**<
	Return NAMEBIT_AUTO if n is an system generated identifier
	type Name, or 0 otherwise.
	Implementation function for NameAuto().  Do not call this
	function directly - use NameAuto() instead.
*/

#ifdef NDEBUG
#define NameIdPtr(n) (NameId(n) ? (n)->val.id : (n)->val.der->strptr)
#else
#define NameIdPtr(n) NameIdPtrF(n)
#endif
/**<
	Returns the id pointer for identifier type name node n.
	@param n CONST struct Name*, Name to query.
	@return The id pointer as a symchar*.
	@see NameIdPtrF()
*/
extern symchar *NameIdPtrF(CONST struct Name *n);
/**<
	Assumes that n is a identifier type name node.
	@return the id pointer.
	Implementation function for NameIdPtr().  Do not call this
	function directly - use NameIdPtr() instead.
*/

extern symchar *SimpleNameIdPtr(CONST struct Name *n);
/**<
	Return NULL if n is not an NameId or if it has a next field.  Otherwise,
	it returns the char pointer.
*/

extern unsigned int NameLength(CONST struct Name *n);
/**<
	Returns the number of links in a name.

	@note May be used in a similar manner to SimpleNameIdPtr,
		to determine if a name is a simple name

	@example
		<tt>my_var</tt> has length 1.
		<tt>my_var.your_var[1..3]</tt> has length 3.
*/

#ifdef NDEBUG
#define NameSetPtr(n) ((n)->val.s)
#else
#define NameSetPtr(n) NameSetPtrF(n)
#endif
/**<
	Returns the set pointer for set type name node n.
	@param n CONST struct Name*, Name to query.
	@return The set pointer as a CONST struct Set*.
	@see NameSetPtrF()
*/
extern CONST struct Set *NameSetPtrF(CONST struct Name *n);
/**<
	Assumes that n is a set type name node.
	Returns the set pointer.
	Implementation function for NameSetPtr().  Do not call this
	function directly - use NameSetPtr() instead.
*/

#ifdef NDEBUG
#define NameDerPtr(n) ((n)->val.der)
#else
#define NameDerPtr(n) NameDerPtrF(n)
#endif
/**<
	Returns the derivative pointer for derivative type name node n.
	@param n CONST struct Name*, Name to query.
	@return The derivative pointer as a CONST struct DerName*.
	@see NameDerPtrF()
*/
extern CONST struct DerName *NameDerPtrF(CONST struct Name *n);
/**<
	Assumes that n is a derivative type name node.
	Returns the derivative pointer.
	Implementation function for NameDerPtr().  Do not call this
	function directly - use NameDerPtr() instead.
*/

#ifdef NDEBUG
#define DerStrPtr(n) ((n)->strname)
#else
#define DerStrPtr(n) DerStrPtrF(n)
#endif
/**<
	Returns the symchar pointer for derivative structure n.
	@param n CONST struct DerName*.
	@return The symchar pointer as a symchar*.
	@see DerStrPtrF()
*/
extern symchar *DerStrPtrF(CONST struct DerName *n);
/**<
	Returns the symchar pointer.
	Implementation function for DerStrPtr().  Do not call this
	function directly - use DerStrPtr() instead.
*/

#ifdef NDEBUG
#define DerVlist(n) ((n)->vlist)
#else
#define DerVlist(n) DerVlistF(n)
#endif
/**<
	Returns the variable list pointer for derivative structure n.
	@param n CONST struct DerName*.
	@return The variable list pointer as a struct VariableList*.
	@see DerStrPtrF()
*/
extern struct VariableList *DerVlistF(CONST struct DerName *n);
/**<
	Returns the variable list pointer.
	Implementation function for DerVlist().  Do not call this
	function directly - use DerVlist() instead.
*/

extern void SetHasder(struct Name *n);
/**<
	Sets NAMEBIT_HASDER of all nodes of the name to 1.
*/

extern struct DerName *CopyDerName(CONST struct DerName *n);
/**<
	Make and return a copy of the derivative structure.
*/

extern struct Name *CopyName(CONST struct Name *n);
/**<
	Make and return a copy of the whole name.
*/

extern struct Name *CopyAppendNameNode(CONST struct Name *n, CONST struct Name *node);
/**<
	Make a copy of n and append a copy of the node (which may be just the
	head of a longer name). The result is totally disjoint from the inputs.
*/

extern struct Name *AppendNameNode(struct Name *n1, CONST struct Name *n2);
/**<
	Append a copy of the node (which may be just the head of a longer name).
*/

ASC_DLLSPEC void DestroyName(struct Name *n);
/**<
	Deallocate the whole name linked list
	Handles NULL input gracefully.
*/

extern void DestroyDerName(struct DerName *der);
/**<
	Deallocate this derivative structure.
	Handles NULL input gracefully.
 */

extern void DestroyNamePtr(struct Name *n);
/**<
	Deallocate this name node, and don't change the next node.
	Handles NULL input gracefully.
 */

extern struct Name *JoinNames(struct Name *n1, struct Name *n2);
/**<
	Appends n2 to the end of n1.  This will return n1, unless n1 is NULL in
	which case it will return n2.
 */

extern struct Name *ReverseName(struct Name *n);
/**<
	Returns the reverse of n.
	Normally only done to unreverse the order that yacc collects
	identifiers.
*/

extern CONST struct Name *NextIdName(CONST struct Name *n);
/**<
	Returns the first NameId element in the name after the current element
	which is expected to be a NameId. If there is none, returns NULL.
 */

extern int NameCompound(CONST struct Name *n);
/**<
	Test whether name is compound (i.e. crosses a MODEL/ATOM boundary).
	If so this returns 1, OTHERWISE this returns 0.  So array names
	will return 0.

	The following return 0:
	 - a
	 - a[i]
	 - [i][j]  -- though this isn't a proper name, generally.

	The following return 1:
	 - a.b
	 - a[i].b
	 - [i].b

	So basically, if the name is printed with a '.' this will return 1.
*/

extern int NamesEqual(CONST struct Name *n1, CONST struct Name *n2);
/**<
	Return TRUE if and only if n1 and n2 are structurally equivalent.
*/

extern int CompareNames(CONST struct Name *n1, CONST struct Name *n2);
/**<
	Returns -1 0 1 as n1 < = > n2.
	Will need fixing when we have supported attributes.
*/

extern int CompareDers(CONST struct DerName *n1, CONST struct DerName *n2);
/**<
	Returns -1 0 1 as n1 < = > n2.
*/

extern void name_init_pool(void);
/**<
	Starts memory recycle. Do not call twice before stopping recycle.
*/

extern void name_destroy_pool(void);
/**<
	Stops memory recycle. Do not call while ANY names are outstanding.
*/

extern void name_report_pool(void);
/**<
	Write the pool report to ASCERR for the name pool.
*/

/* @} */

#endif  /* ASC_NAME_H */
