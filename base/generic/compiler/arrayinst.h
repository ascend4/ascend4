/*
 *  Ascend Instance Array Functions
 *  by Tom Epperly & Ben Allan
 *  8/16/89
 *  Version: $Revision: 1.15 $
 *  Version control file: $RCSfile: arrayinst.h,v $
 *  Date last modified: $Date: 1998/04/07 19:52:46 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1996, 1997 Benjamin Andrew Allan
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
 */

/** @file
 *  Ascend Instance Array Functions.
 *  <pre>
 *  When #including arrayinst.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "instance_enum.h"
 *         #include "compiler.h"
 *         #include "setvalinst.h"
 *         #include "pool.h"
 *         #include "list.h"
 *
 *  Notes on the structure implemented for ASCEND arrays.
 *
 *  ASCEND arrays are 'associative arrays.' That is they are
 *  not sequential in memory, rather they are accessed by
 *  names of the elements. So there really isn't a difference
 *  between a dense rectangular array and a sparse array except
 *  it is algorithmically easier to construct the dense array.
 *
 *  For example:
 *
 *       a[1..2]['a','f'] IS_A foo;
 *
 *  yields an internal data structure (a, uai1 and uai2
 *  are ArrayInstances as described in instance_types.h) like so:
 *
 *  a -------------------|-----------------------|    <=== gl_list_t
 *                       V                       V         *childlist.
 *                    ArrayChild{             ArrayChild{
 *                    name.int = 1            name.int = 2
 *              unnamed array inst|}    unnamed array inst|}
 *     /--------------------------/    /------------------/
 *    V                               V
 *    uai1 ------|-------|            uai2 -----|-----|  <=== gl_list_t's
 *               V       V                      V     V
 *             AC       AC                     AC     AC
 *       name.str='a'  name.str='f'      name.str='a'  name.str='f'
 *       inst --\      inst --\          inst --\      inst --\
 *              |             |                 |             |
 *              V             V                 V             V
 *        fooinst       fooinst            fooinst       fooinst
 *
 *  Unnamed array instances actually DO have a compiler generated
 *  internal name, but it is not useful for anything except avoiding
 *  core dumps in routines that assume all insts have names.
 *
 *  Navigating these structures during assembly is
 *  terribly dangerous so all that is handled by this file.
 *
 *  All the indirection in these structures makes interesting tricks
 *  for sparse and dense arrays possible.
 *
 *  One trick in particular, however, is NOT to be attempted because
 *  it plays havoc with the semantics of the ASCEND language:
 *  thou shalt not declare an array over one set and then later expand
 *  it to have more elements.  This would be trivial to implement
 *  because the elements exist in gl_lists, but so far every potential
 *  application proposed for it has been the result of sloppy and/or
 *  lazy thinking.  In the end we may find a need for a genuine
 *  multidimensional ListInstance that has much in common with arrays,
 *  but such a creature should be implemented as it's own creature and
 *  not a sloppy graft on top of arrays.
 *  </pre>
 */

#ifndef __ARRAYINST_H_SEEN__
#define __ARRAYINST_H_SEEN__

/* Array child memory management */
#define CAC(acp) ((struct ArrayChild *)(acp))

extern pool_store_t g_array_child_pool;
/**< 
 *  Pool of array children.
 *  Never ever dereference this except with MALLOCPOOLAC or FREEPOOLAC(). 
 */

#ifdef ASC_NO_POOL

/* slow version for debugging */
#define MALLOCPOOLAC CAC( ascmalloc(sizeof(struct ArrayChild)) )
/**< 
 *  Get an element from the pool (slow, unpooled version).
 *  Only call after InitInstanceNanny(). 
 */
#define FREEPOOLAC(ac) ascfree(ac);
/**< 
 *  Return element ac to the pool (slow, unpooled version). 
 *  Only call after InitInstanceNanny(). 
 */

#else

#define MALLOCPOOLAC CAC(pool_get_element(g_array_child_pool))
/**< Get an element from the pool. Only call after InitInstanceNanny(). */
#define FREEPOOLAC(ac) pool_free_element(g_array_child_pool,(ac))
/**< Return element ac to the pool. Only call after InitInstanceNanny(). */

#endif /* ASC_NO_POOL */

extern void InitInstanceNanny(void);
/**<
 *  <!--  InitInstanceNanny();                                         -->
 *  Sets up array child instantiation gizmos. This must be called once
 *  before any arrays can be built, ideally at startup time.
 *  Do not call it again unless DestroyInstanceNanny is called first.
 *  If insufficient memory to compile anything at all, does exit(2). <br><br>
 *
 *  Under no circumstances should you (an external client) be freeing
 *  a struct ArrayChild.
 */

extern void DestroyInstanceNanny(void);
/**<
 *  <!--  DestroyInstanceNanny();                                      -->
 *  Destroy array child instantiation gizmos. This must be called to
 *  clean up before shutting down ASCEND.
 *  Do attempt to instantiate anything after you call this unless you
 *  have recalled InitInstanceNanny().
 */

extern void ReportInstanceNanny(FILE *f);
/**<
 *  <!--  ReportInstanceNanny(f);                                      -->
 *  <!--  FILE *f;                                                     -->
 *  Reports on the array child instantiator to f.
 */

/* Array management */

extern struct gl_list_t
*CollectArrayInstances(CONST struct Instance *i,
                       struct gl_list_t *list);
/**<
 *  <!--  list = CollectArrayInstances(i,NULL);                        -->
 *  <!--  const struct Instance *i;                                    -->
 *  <!--  struct gl_list_t *list;                                      -->
 *  Appends pointers of the set/MODEL/ATOM/constant instances found in
 *  the leaves of an array instance, i, sparse or dense.
 *  If list given by user is NULL, a list to be returned is made if
 *  necessary.
 *  If i is not an array, list returned will be NULL if list given is NULL.
 *  If i is an array, list returned may be empty, but not NULL.
 *  This function recurses through all the subscripts of the array.
 */

typedef void (*AVProc)(struct Instance *);
/**< A function taking an Instance* and having no return value. */

extern void ArrayVisitLocalLeaves(struct Instance *mch, AVProc func);
/**<
 *  <!--  ArrayVisitLocalLeaves(mch,func)                              -->
 *  <!--  struct Instance *mch;                                        -->
 *  <!--  AVProc func;                                                 -->
 *  This function visits the instances indicated by the name
 *  given in the definition statement of mch.
 *  func is as described in visitinst.h for VisitProc.
 *  mch is an array instance that is the child of a MODEL.
 */

/* Dense array procedures. (non-relations) */

extern int RectangleArrayExpanded(CONST struct Instance *i);
/**<
 *  <!--  int RectangleArrayExpanded(i)                                -->
 *  <!--  const struct Instance *i;                                    -->
 *  Test if the array is fully expanded
 *  (i.e. all the sets for all the derefencing have been specified).<br><br>
 *
 *  On sparse arrays, this operator might return a FALSE positive
 *  because it checks down the leading member of each defined
 *  subscript range. This error is precluded only by the fact that when
 *  instantiating, we do sparse arrays completely in one pass, therefore
 *  the leading members check is a sufficient test.
 *  In general, however, this should not be used on sparse arrays.
 */

extern int RectangleSubscriptsMatch(CONST struct Instance *context,
                                    CONST struct Instance *ary,
                                    CONST struct Name *subscripts);
/**<
 *  <!--  int RectangleSubscriptsMatch(context,ary,subscripts)         -->
 *  <!--  const struct Instance *context, *ary;                        -->
 *  <!--  const struct Name *subscripts;                               -->
 *  Test if the ary children expected from evaluating the
 *  nodes of subscripts (all set nodes) are all compatible
 *  with the children of the array instance given. The set
 *  expressions in Name elements are evaluated in the context given.
 *  Assumes the array has been fully expanded. <br><br>
 *
 *  On array subscripts not yet resolvable, returns -2; try later. <br>
 *  On array shape mismatch, returns -1. <br>
 *  On subscripts mismatch, returns 0. <br>
 *  On match, returns 1. <br><br>
 *
 *  On sparse arrays, this operator should NOT be used.
 *  A reasonably intelligent person could rewrite this to handle sparse
 *  arrays, with the addition of a for_table argument.
 */

extern unsigned long NextToExpand(CONST struct Instance *i);
/**<
 *  <!--  unsigned long NextToExpand(i)                                -->
 *  <!--  const struct Instance *i;                                    -->
 *  Return the number of the dereferencing that needs to be expanded.  This
 *  returns 0 if none are needed; 1 is the first dereference.
 */

extern unsigned long NumberofDereferences(CONST struct Instance *i);
/**<
 *  <!--  unsigned long NumberofDereferences(i)                        -->
 *  <!--  const struct Instance *i;                                    -->
 *  This returns the number of dereferences that this array instance has
 *  before reaching what the array contains.
 */

extern CONST struct Set *IndexSet(CONST struct Instance *i, unsigned long num);
/**<
 *  <!--  struct Set *IndexSet(i,num)                                  -->
 *  <!--  struct Instance *i;                                          -->
 *  <!--  unsigned long num;                                           -->
 *  Return the set that the num'th index is defined over.  Don't make any
 *  changes to the structure that is returned!
 *  1 <= num <= NumberofDereferences(i)
 *  Will return NULL on the final subscript of an ALIASES/IS_A
 *  inside a FOR loop.
 */

extern void ExpandArray(struct Instance *i,
                        unsigned long num,
                        struct set_t *set,
                        struct Instance *rhsinst,
                        struct Instance *arginst,
                        struct gl_list_t *rhslist);
/**<
 *  <!--  void ExpandArray(i,num,set,rhsinst,arginst,rhslist)          -->
 *  <!--  struct Instance *i;                                          -->
 *  <!--  unsigned long num;                                           -->
 *  <!--  struct set_t *set;                                           -->
 *  <!--  struct Instance *arginst, *rhsinst;                          -->
 *  <!--  struct gl_list_t *rhslist;                                   -->
 *
 *  This will expand the num'th index over the set of index values given by
 *  set.  set is returned unchanged.<br><br>
 *
 *  If the array is being expanded by an IS_A, this may effect the pending
 *  instance list.  All the instances it adds will be added below the top.<br><br>
 *
 *  If the array is being expanded by an alias, rhsinst is not NULL and
 *  rhsinst is used as the array element and pending list is not affected.<br><br>
 *
 *  If the array is being expanded by an aliases-IS_A, rhsinst is NULL,
 *  rhslist is used as the array elements source for copying,
 *  and the pending list is not affected.
 *  The contents of the list is a bunch of struct ArrayChild * which should
 *  have indices matching the last subscript of the array being expanded.
 *  Deallocating the contents of the rhs ist is the caller's responsibility,
 *  as is creating it -- it is copied as needed internally.<br><br>
 *
 *  If the array being expanded is of a parametric type, the arginst
 *  will be used to construct the elements and pending list may be affected.
 */

/* Sparse arrays stuff. */

extern struct Instance *FindOrAddIntChild(struct Instance *i,
                                          long v,
                                          struct Instance *rhsinst,
                                          struct Instance *arginst);
/**<
 *  <!--  struct Instance *FindOrAddIntChild(i,v,rhsinst,arginst)      -->
 *  <!--  struct Instance *i,*rhsinst, *arginst;                       -->
 *  <!--  long v;                                                      -->
 *  Add sparse array child at location defined by current ForTable
 *  after instantiating child if rhsinst is NULL (an ISA).
 *  If instantiating, uses arginst if not NULL.
 *  Uses rhsinst if it is not NULL (an array element defined by alias).
 */

extern struct Instance *FindOrAddStrChild(struct Instance *i,
                                          symchar *sym,
                                          struct Instance *rhsinst,
                                          struct Instance *arginst);
/**<
 *  <!--  struct Instance *FindOrAddStrChild(i,sym,rhsinst,arginst)    -->
 *  <!--  struct Instance *i, *rhsinst, *arginst;                      -->
 *  <!--  symchar *sym;                                                -->
 *  Add sparse array child at location defined by current ForTable
 *  after instantiating child if rhsinst is NULL (an ISA).
 *  If instantiating, uses arginst if not NULL.
 *  Uses rhsinst if it is not NULL (an array element defined by alias).
 */

extern int CmpArrayInsts(struct Instance *i1, struct Instance *i2);
/**<
 *  <!--  int CmpArrayInsts(i1,i2)                                     -->
 *  <!--  struct Instance *i1, *i2;                                    -->
 *  Returns 0 if the arrays i1 and i2 are defined over equivalent subscript
 *  ranges and have leaf parts of shallowly equivalent types.
 *  (hint: relations and models are not deeply checked.)
 *  Returns 1 for any non-equivalency.
 *  Doesn't return if called with something other than arrays.
 *  NULL input --> 1 + warning, rather than exit.
 */

#endif  /* __ARRAYINST_H_SEEN__ */

