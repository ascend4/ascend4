/**< 
 *  Model Child list routines
 *  by Tom Epperly
 *  Version: $Revision: 1.21 $
 *  Version control file: $RCSfile: child.h,v $
 *  Date last modified: $Date: 1998/03/17 22:08:26 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *  Copyright (C) 1996 Benjamin Allan
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
 *  This is a package of routines to process child lists.
 */

/**< 
 *  When #including child.h, make sure these files are #included first:
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 */


#ifndef __CHILD_H_SEEN__
#define __CHILD_H_SEEN__
/**< requires
 *# #include"compiler.h"
 *# #include"list.h"
 */

typedef CONST struct ChildListStructure *ChildListPtr;

struct ChildListEntry {
  symchar *strptr;
  /**< 
   * this is the symbol table name of a child, eg "a" of a[i][j]
   */
  CONST struct TypeDescription *typeptr;
  /**< 
   * this is the most refined type for the child that can be determined
   * at parse time. Corresponding instances at run time may be this or
   * a more refined type. This allows several kinds of sanity checking.
   * This pointer must not be NULL.
   */
  CONST struct Statement *statement;
  /**< 
   * statement where child is initially defined.
   */
  unsigned bflags;
  /**< 
   * boolean flags, as defined below with CBF_*
   */
  short isarray;
  /**< 
   * isarray should be number of subscripts if you firmly
   * believe the named child is an array. Nobody could possibly
   * have more than a shorts worth of subscripts.
   */
  short origin;
  /**< 
   * tells how child created (ALIASES, ARR, WILL_BE, IS_A,
   *                          P-ALIASES, P-ARR, P-IS_A, P-WILL_BE)
   * P-IS_A and P-WILL_BE indicate that it came through a parameter list.
   * P-ALIASES indicates an alias of something that came through a
   * parameter list WILL_BE directly or indirectly (as a part of a parameter).
   *
   * The set in the ALIASES-IS_A statement gets
   * listed as an IS_A origin. The array gets listed as an ARR origin.
   */
#define origin_ERR	0
/**< set 1 */
#define origin_ALI	1
#define origin_ARR      2
#define origin_ISA	3
#define origin_WB	4
/**< set 2 should match set 1, except having P and being > by offset */
#define origin_PALI	5
#define origin_PARR     6
#define origin_PISA	7
#define origin_PWB	8
#define origin_PARAMETER_OFFSET (origin_PALI - origin_ALI)
/**< 
 * Distance between corresponding origin and the parametric version.
 * If you mess with these origin defines, fix the macro LegalOrigin
 * in child.c
 */
/**< child boolean flag bit definitions */
#define CBF_VISIBLE	0x1	/**< child to be shown in UI among lists */
#define CBF_SUPPORTED	0x2	/**< child is a '$upported' attribute */
#define CBF_PASSED	0x4	/**< child is argument to another child */
/**< Note that because of arrays, CBF_PASSED is approximate. 
 * If a[1] is passed, a gets marked passed, but if a.b is passed, a is not.
 */
/**< other CBF as required */
};
/**< 
 * As of 9/96 we've added a number of important features to
 * ChildList, and it's nobody's business how we implement it.
 * The ChildListEntry is the interface container for creating
 * childlists. Storage scheme and usage after that is all
 * black magic which can only be done by following this header.
 */

#define AliasingOrigin(ori) \
  ((ori) == origin_PALI || (ori) == origin_ALI || \
   (ori) == origin_PARR || (ori) == origin_ARR)
/**< 
 *  Returns 1 if the value given is an alias sort.
 */

#define ParametricOrigin(ori) ((ori) >= origin_PALI && (ori) <= origin_PWB)
/**< 
 *  Returns 1 if the value given is a parametric sort or 0 if not.
 */

extern int CmpChildListEntries(CONST struct ChildListEntry *,
                               CONST struct ChildListEntry *);
/**< 
 *  CmpChildListEntries(e1,e2)
 *  CONST struct ChildListEntry *e1, *e2;
 *  Returns the result of an alphabetical order comparison
 *  on the names in the two pointers. Not necessarily implemented
 *  with strcmp, however, so you should use this function.
 */

extern ChildListPtr CreateChildList(struct gl_list_t *);
/**< 
 *  ChildListPtr CreateChildList(l)
 *  struct gl_list_t *l;
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
 *  descriptions of the children to be copied.
 *
 *  This function should never return a NULL pointer except in the
 *  case where you have specified input with a NULL typeptr.
 */

extern void DestroyChildList(ChildListPtr);
/**< 
 *  void DestroyChildList(cl)
 *  ChildListPtr cl;
 *  Deallocate the memory associated with the list but not the symchars in
 *  the list. The fact that you're passing a CONST pointer object into
 *  this function is somewhat odd, but it suffices to know that after
 *  this call, any data that might have been in the pointer you gave us
 *  really has been disposed of.
 */

extern ChildListPtr AppendChildList(ChildListPtr,
                                    struct gl_list_t *);
/**< 
 *  ChildListPtr AppendChildList(cl,l)
 *  ChildListPtr cl;
 *  struct gl_list_t *l;
 *  Create and return a new child list which contains all the information
 *  contained in cl or l.  l must be sorted, and it is assumed that there
 *  are no duplicate entries. The new list returned is sorted.
 *  If l is not sorted, we will sort it at rather an expense.
 *  The same conditions for l in CreateChildList apply here.
 */

extern unsigned long ChildListLen(ChildListPtr);
/**< 
 *  unsigned long ChildListLen(cl)
 *  CONST ChildListPtr cl;
 *  Return the length of the child list.
 */

extern symchar *ChildStrPtr(ChildListPtr ,unsigned long);
/**< 
 *  symchar *ChildStrPtr(cl,n)
 *  CONST ChildListPtr *cl;
 *  unsigned long n;
 *  Return child number n name element 1 string.
 *  Children are numbered 1..ChildListLen(cl).
 */

extern unsigned int ChildIsArray(ChildListPtr ,unsigned long);
/**< 
 *  unsigned int ChildIsArray(cl,n)
 *  CONST ChildListPtr *cl;
 *  unsigned long n;
 *  Return >= 1 if child number n is determined array type at parse time.
 *  The return value the number of subscripts of child n
 *  needed to reach a single array element of ChildBaseTypePtr type.
 *  Returns 0 if type is not array.
 */

extern unsigned int ChildOrigin(ChildListPtr ,unsigned long);
/**< 
 *  unsigned int ChildOrigin(cl,n)
 *  CONST ChildListPtr *cl;
 *  unsigned long n;
 *  Return the origin code of the child.
 */

extern unsigned int ChildAliasing(ChildListPtr ,unsigned long);
/**< 
 *  unsigned int ChildAliasing(cl,n)
 *  CONST ChildListPtr *cl;
 *  unsigned long n;
 *  Return the Aliasness of a child, meaning if the child ALIASES one
 *  passed into the type definition or a part of the definition.
 */

extern unsigned int ChildParametric(ChildListPtr ,unsigned long);
/**< 
 *  unsigned int ChildParametric(cl,n)
 *  CONST ChildListPtr *cl;
 *  unsigned long n;
 *  Return the parametricness of a child, meaning if the child is one
 *  passed into the type definition or one aliasing something or a part of
 *  something passed into the definition.
 */

extern CONST struct Statement *ChildStatement(ChildListPtr,unsigned long);
/**< 
 *  CONST struct Statement *ChildStatement(cl,n)
 *  CONST ChildListPtr *cl;
 *  unsigned long n;
 *  Return child number n initial declaration statement.
 *  Children are numbered 1..ChildListLen(cl).
 */

extern unsigned ChildGetBooleans(ChildListPtr,unsigned long);
/**< 
 *  unsigned int ChildGetBooleans(cl,n)
 *  CONST ChildListPtr *cl;
 *  unsigned long n;
 *  Return child number n current boolean flags.
 *  Children are numbered 1..ChildListLen(cl).
 *  If an improperly large or small n is given, result is 0.
 */

#define ChildVisible(cl,n) ((ChildGetBooleans((cl),(n)) & CBF_VISIBLE)!=0)
/**< 
 *  macro ChildVisible(clist,childnumber)
 *  Returns 1 if child has visibility bit turned on.
 */

#define ChildSupported(cl,n) ((ChildGetBooleans((cl),(n)) & CBF_SUPPORTED)!=0)
/**< 
 *  macro ChildSupported(clist,childnumber)
 *  Returns 1 if child has supported bit turned on.
 */

#define ChildPassed(cl,n) ((ChildGetBooleans((cl),(n)) & CBF_PASSED) !=0)
/**< 
 *  macro ChildSupported(clist,childnumber)
 *  Returns 1 if child has PASSED bit turned on.
 */

extern void ChildSetBoolean(ChildListPtr,unsigned long,unsigned,unsigned);
/**< 
 *  void ChildSetBoolean(cl,n,cbfname,val)
 *  CONST ChildListPtr *cl;
 *  unsigned long n;
 *  unsigned int cbfname; as defined above by CBF_
 *  unsigned int val;   0 or 1 only.
 *  Set child number n current boolean flag bit cbfname to val.
 *  Children are numbered 1..ChildListLen(cl).
 */

#define ChildHide(cl,n) ChildSetBoolean((cl),(n),CBF_VISIBLE,0)
/**< 
 *  macro ChildHide(cl,n)
 *  CONST ChildListPtr *cl;
 *  unsigned long n;
 *  Hide (presumably for display purposes) the nth child in cl.
 */

#define ChildShow(cl,n) ChildSetBoolean((cl),(n),CBF_VISIBLE,1)
/**< 
 *  macro ChildShow(cl,n)
 *  CONST ChildListPtr *cl;
 *  unsigned long n;
 *  Unhide (presumably for display purposes) the nth child in cl.
 */

extern CONST struct TypeDescription *ChildBaseTypePtr(ChildListPtr,
                                                      unsigned long);
/**< 
 *  CONST struct TypeDescription *ChildBaseTypePtr(cl,n)
 *  CONST ChildListPtr cl;
 *  unsigned long n;
 *  Return child number n type determinable at parse time.
 *  If type was not determinable, returns NULL, but this will never be
 *  the case as Something is always determinable.
 *  NOTE: an array child does not return an array type description but
 *  instead the base type for the array. Note that this requires we
 *  ignore refinement statements on specific array children when dealing
 *  with array base type info.
 *  If this pointer is not NULL, then any corresponding instance will
 *  be of at least the type returned.
 *  Children are numbered 1..ChildListLen(cl).
 *
 *  Note: The children of atoms always return NULL type since they do
 *  not have type descriptions in the system because they are not full
 *  instances.
 */

extern unsigned long ChildPos(ChildListPtr ,symchar *);
/**< 
 *  unsigned long ChildPos(cl,s)
 *  CONST ChildListPtr cl;
 *  symchar *s;
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

extern int CompareChildLists(ChildListPtr,ChildListPtr,unsigned long *);
/**< 
 * cmp = CompareChildLists(GetChildList(d1),GetChildList(d2),&diff);
 * struct TypeDescription *d1,*d2;
 * unsigned long int diff;
 * int cmp;
 * Returns -1/0/1 as d1 <,==,> d2 (0). If cmp != 0, diff = position
 * in child list d2 of first difference, i.e. if the lists are m and n
 * long (m > n) and OTHERWISE equivalent, diff = n + 1.
 */

extern void WriteChildList(FILE *,ChildListPtr);
/**< 
 *  WriteChildList(fp,cl)
 *  Write what is known at parse time about the children in the child list
 *  given.  What is known may be surprising. It may be only mildly
 *  accurate.
 */
#endif /**< __CHILD_H_SEEN__ */
