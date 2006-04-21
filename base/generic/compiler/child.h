/*	ASCEND modelling environment
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
	Copyright (C) 1996 Benjamin Allan
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
	This is a package of routines to process child lists.

	Requires
	#include "utilities/ascConfig.h"
	#include "fractions.h"
	#include "compiler.h"
	#include "dimen.h"
	#include "list.h"
*//*
	by Tom Epperly
	Version: $Revision: 1.21 $
	Version control file: $RCSfile: child.h,v $
	Date last modified: $Date: 1998/03/17 22:08:26 $
	Last modified by: $Author: ballan $
*/

#ifndef ASC_CHILD_H
#define ASC_CHILD_H

#include <utilities/ascConfig.h>

/**
 *  The ChildListStructure is a private implementation detail.
 *  All the public interface needs is a pointer thereto.
 */
typedef CONST struct ChildListStructure *ChildListPtr;

/**
 * The ChildListEntry is the interface container for creating
 * childlists. Storage scheme and usage after that is all
 * black magic which can only be done by following this header.<br><br>
 *
 * As of 9/96 we've added a number of important features to
 * ChildList, and it's nobody's business how we implement it.
 */
struct ChildListEntry {
  symchar *strptr;
      /**< the symbol table name of a child, eg "a" of a[i][j] */
  CONST struct TypeDescription *typeptr;
      /**< the most refined type for the child that can be determined
       * at parse time. Corresponding instances at run time may be this or
       * a more refined type. This allows several kinds of sanity checking.
       * This pointer must not be NULL.
       */
  CONST struct Statement *statement;
      /**< statement where child is initially defined. */
  unsigned bflags;
      /**< boolean flags, as defined below with CBF_* */
  short isarray;
      /**< isarray should be number of subscripts if you firmly
       * believe the named child is an array. Nobody could possibly
       * have more than a shorts worth of subscripts.
       */
  short origin;
      /**< tells how child created (ALIASES, ARR, WILL_BE, IS_A,
       *                            P-ALIASES, P-ARR, P-IS_A, P-WILL_BE)
       * P-IS_A and P-WILL_BE indicate that it came through a parameter list.
       * P-ALIASES indicates an alias of something that came through a
       * parameter list WILL_BE directly or indirectly (as a part of a parameter).
       *
       * The set in the ALIASES-IS_A statement gets
       * listed as an IS_A origin. The array gets listed as an ARR origin.
       */

#define origin_ERR  0     /**< Origin code - invalid origin. */
/* set 1 */
#define origin_ALI  1     /**< Origin code - ALIASES. */
#define origin_ARR  2     /**< Origin code - ARR. */
#define origin_ISA  3     /**< Origin code - IS_A. */
#define origin_WB   4     /**< Origin code - WILL_BE. */
/* set 2 should match set 1, except having P and being > by offset */
#define origin_PALI 5     /**< Origin code - P-ALIASES. */
#define origin_PARR 6     /**< Origin code - P-ARR. */
#define origin_PISA 7     /**< Origin code - P-IS_A. */
#define origin_PWB  8     /**< Origin code - P-WILL_BE. */
#define origin_EXT  9     /**< Origin code - EXT statement */
#define origin_PARAMETER_OFFSET (origin_PALI - origin_ALI)
/**<
 * Distance between corresponding origin and the parametric version.
 * If you mess with these origin defines, fix the macro LegalOrigin
 * in child.c
 */

/* child boolean flag bit definitions */
#define CBF_VISIBLE	  0x1 /**< child to be shown in UI among lists */
#define CBF_SUPPORTED	0x2 /**< child is a '$upported' attribute */
#define CBF_PASSED    0x4 /**< child is argument to another child.
                               Note that because of arrays, CBF_PASSED
                               is approximate.  If a[1] is passed, a gets
                               marked passed, but if a.b is passed, a is not. */
/* other CBF as required */
};  /* struct ChildListEntry */

#define AliasingOrigin(ori) \
  ((ori) == origin_PALI || (ori) == origin_ALI || \
   (ori) == origin_PARR || (ori) == origin_ARR)
/**< Returns 1 if the value given is an alias sort. */

#define ParametricOrigin(ori) ((ori) >= origin_PALI && (ori) <= origin_PWB)
/**< Returns 1 if the value given is a parametric sort or 0 if not. */

extern int CmpChildListEntries(CONST struct ChildListEntry *e1,
                               CONST struct ChildListEntry *e2);
/**<
 *  Returns the result of an alphabetical order comparison
 *  on the names in the two pointers. Not necessarily implemented
 *  with strcmp, however, so you should use this function.
 */

extern ChildListPtr CreateChildList(struct gl_list_t *l);
/**<
 *  This takes a list of struct ChildListEntry * from a gl_list_t to a
 *  ChildList type.
 *  l must be sorted and should not contain any duplicate entries,
 *  nor any entries with a NULL typeptr.
 *  There are no other preconditions on l.  Children lists are always
 *  stored in alphabetic order (strcmp).
 *  If the list isn't sorted, we will sort it at rather an expense.
 *  You still own the list l AND its entries and should act accordingly.
 *  We will return a ChildListPtr. You don't know what is in that.
 *  We own the memory to it and only we know how to destroy it.
 *  Creating a ChildListPtr causes the type
 *  descriptions of the children to be copied.<br><br>
 *
 *  This function should never return a NULL pointer except in the
 *  case where you have specified input with a NULL typeptr.
 */

extern void DestroyChildList(ChildListPtr cl);
/**<
 *  Deallocate the memory associated with the list but not the symchars in
 *  the list. The fact that you're passing a CONST pointer object into
 *  this function is somewhat odd, but it suffices to know that after
 *  this call, any data that might have been in the pointer you gave us
 *  really has been disposed of.
 */

extern ChildListPtr AppendChildList(ChildListPtr cl, struct gl_list_t *l);
/**<
 *  Create and return a new child list which contains all the information
 *  contained in cl or l.  l must be sorted, and it is assumed that there
 *  are no duplicate entries. The new list returned is sorted.
 *  If l is not sorted, we will sort it at rather an expense.
 *  The same conditions for l in CreateChildList apply here.
 */

ASC_DLLSPEC(unsigned long ) ChildListLen(ChildListPtr cl);
/**<
 *  Return the length of the child list.
 */

ASC_DLLSPEC(symchar *) ChildStrPtr(ChildListPtr cl, unsigned long n);
/**<
 *  Return child number n name element 1 string.
 *  Children are numbered 1..ChildListLen(cl).
 */

extern unsigned int ChildIsArray(ChildListPtr cl, unsigned long n);
/**<
 *  Return >= 1 if child number n is determined array type at parse time.
 *  The return value the number of subscripts of child n
 *  needed to reach a single array element of ChildBaseTypePtr type.
 *  Returns 0 if type is not array.
 */

extern unsigned int ChildOrigin(ChildListPtr cl, unsigned long n);
/**<
 *  Return the origin code of the child.
 */

extern unsigned int ChildAliasing(ChildListPtr cl, unsigned long n);
/**<
 *  Return the Aliasness of a child, meaning if the child ALIASES one
 *  passed into the type definition or a part of the definition.
 */

extern unsigned int ChildParametric(ChildListPtr cl, unsigned long n);
/**<
 *  Return the parametricness of a child.  That is, if the child is one
 *  passed into the type definition or one aliasing something or a part of
 *  something passed into the definition.
 */

extern CONST struct Statement *ChildStatement(ChildListPtr cl, unsigned long n);
/**<
 *  Return child number n initial declaration statement.
 *  Children are numbered 1..ChildListLen(cl).
 */

ASC_DLLSPEC(unsigned) ChildGetBooleans(ChildListPtr cl, unsigned long n);
/**<
 *  Return child number n current boolean flags.
 *  Children are numbered 1..ChildListLen(cl).
 *  If an improperly large or small n is given, result is 0.
 */

#define ChildVisible(cl,n) ((ChildGetBooleans((cl),(n)) & CBF_VISIBLE)!=0)
/**<
 *  Returns 1 if child has visibility bit turned on.
 */

#define ChildSupported(cl,n) ((ChildGetBooleans((cl),(n)) & CBF_SUPPORTED)!=0)
/**<
 *  Returns 1 if child has supported bit turned on.
 */

#define ChildPassed(cl,n) ((ChildGetBooleans((cl),(n)) & CBF_PASSED) !=0)
/**<
 *  Returns 1 if child has PASSED bit turned on.
 */

ASC_DLLSPEC(void ) ChildSetBoolean(ChildListPtr cl, unsigned long n,
                            unsigned cbfname, unsigned val);
/**<
 *  Set child number n current boolean flag bit cbfname to val.
 *  Children are numbered 1..ChildListLen(cl).
 *  cbfname is a child boolean flag CBF_ defined above.
 *  val is 0 or 1 only.
 */

#define ChildHide(cl,n) ChildSetBoolean((cl),(n),CBF_VISIBLE,0)
/**<
 *  Hide (presumably for display purposes) the nth child in cl.
 *  @param cl  CONST ChildListPtr*
 *  @param n   unsigned long
 */

#define ChildShow(cl,n) ChildSetBoolean((cl),(n),CBF_VISIBLE,1)
/**<
 *  Unhide (presumably for display purposes) the nth child in cl.
 *  @param cl CONST ChildListPtr
 *  @param n  unsigned long
 */

extern CONST struct TypeDescription *ChildBaseTypePtr(ChildListPtr cl,
                                                      unsigned long n);
/**<
 *  Return child number n type determinable at parse time.
 *  If type was not determinable, returns NULL, but this will never be
 *  the case as Something is always determinable.
 *  NOTE: an array child does not return an array type description but
 *  instead the base type for the array. Note that this requires we
 *  ignore refinement statements on specific array children when dealing
 *  with array base type info.
 *  If this pointer is not NULL, then any corresponding instance will
 *  be of at least the type returned.
 *  Children are numbered 1..ChildListLen(cl).<br><br>
 *
 *  Note: The children of atoms always return NULL type since they do
 *  not have type descriptions in the system because they are not full
 *  instances.
 */

ASC_DLLSPEC(unsigned long ) ChildPos(ChildListPtr cl, symchar *s);
/**<
 *  Search for the string s in child list cl.  If it is not found,
 *  it will return 0; otherwise, it returns the index of the child which
 *  matches s.
 *  s must be in the symbol table. If s is NOT in the symbol table,
 *  then it is impossible a priori for this function to return nonzero
 *  and so it will not return at all because you have asked a stupid
 *  question.
 *  If you need to check s yourself, AscFindSymbol(s) will return
 *  s if s is in the symbol table -- but there should never be a
 *  programming context where you need to check.
 *  If you need to guarantee s is in the table, call AddSymbol first.
 */

extern int CompareChildLists(ChildListPtr cl, ChildListPtr c2, unsigned long *diff);
/**<
 * Returns -1/0/1 as d1 <,==,> d2 (0). If cmp != 0, diff = position
 * in child list d2 of first difference, i.e. if the lists are m and n
 * long (m > n) and OTHERWISE equivalent, diff = n + 1.
 */

extern void WriteChildList(FILE *fp, ChildListPtr cl);
/**<
 *  Write what is known at parse time about the children in the child list
 *  given.  What is known may be surprising. It may be only mildly
 *  accurate.
 */

#endif  /* ASC_CHILD_H */
