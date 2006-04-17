/*
 *  List Module
 *  by Tom Epperly
 *  Version: $Revision: 1.3 $
 *  Version control file: $RCSfile: list.h,v $
 *  Date last modified: $Date: 1998/02/19 13:03:22 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *
 *  The Ascend Language Interpreter is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 *  Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */

/** @file
 *  List Module.
 *
 *  The purpose of this module is to provide a kind of flexible array.
 *  The flexible array has two interesting characteristics.  It allows
 *  contant time(O(1)) retrieval of list items and it is almost infinitely
 *  extendable (i.e. has no preset limit on the number of items in the list).
 *  It does not use much extra memory while providing these services.<br><br>
 *
 *  The list only stores pointers to items as VOIDPTR (void *).  This is
 *  an advantage, in that the user has the flexibility to store pointers to
 *  any data type in the list.  It is also a disadvanatage, since the data
 *  structure is not type safe and the user must carefully keep track of
 *  what is stored in the list.<br><br>
 *
 *  This module provides a standard set of list type operations.  Each includes
 *  some predictions about the efficiency of that operation.  Any  modification
 *  of these procedures should live up to those claims.
 *  <pre>
 *  When #including list.h, make sure these files are #included first:
 *         #include <stdio.h>
 *         #include "utilities/ascConfig.h"
 *         #include "compiler/compiler.h"
 *  </pre>
 */

/*
 *  Any bugs or suggestions can be sent to:
 *
 *  te07@edrc.cmu.edu or te07@andrew.cmu.edu or epperly@osnome.che.wisc.edu
 *  Tom Epperly
 *  314 South Orchard Street
 *  Madison, WI 53715-1542
 *
 *  Also please copy any bugs or suggestions to ascend+developers@cs.cmu.edu
 *
 *  This utility depends on ascmalloc.[ch] and (optionally) pool.[ch]
 *
 *  Change Log
 *  2/26/88   added gl_copy, gl_concat
 *  3/31/88   added additional commenting
 *  2/18/96   added defines when -DNDEBUG is active. We can't
 *              afford the calls in a production compiler. (Ben Allan)
 *  2/23/96   Added recycling feature to reuse gl_lists. (TGWE)
 *  3/25/96   Improved recycling feature. (Ben Allan)
 *  3/30/96   Took dispose flag off gl_destroy and added a mirror
 *              function gl_free_and_destroy to take its place.
 *              Added pooled list heads (optional) which depends on
 *              pool.[ch] and improves performance substantially.
 *              Tuned to large applications. (Ben Allan)
 *  9/9/96    Changed flags from struct to int.  (Ben Allan)
 *  10/2/96   Added switch over -DMOD_REALLOC to use ascreallocPURE.
 *              If this file is compiled -DMOD_REALLOC, purify leaks of
 *              list->data are real, OTHERWISE it may be noise.
 *              Skipping the call to gl_init may also help dianosis.
 *  9/20/97   Added gl_compare_ptrs.
 */

#ifndef ASC_LIST_H
#define ASC_LIST_H

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#define LISTIMPLEMENTED 0 		/**< BAA_DEBUG changes that need work */

/*
 * The following bit fields are defined for the gl_list flags
 */
#define gsf_SORTED     0x1    /**< gl_list_t flag:  list is sorted. */
#define gsf_EXPANDABLE 0x2    /**< gl_list_t flag:  list is expandable. */

/** List data structure. */
struct gl_list_t {
  VOIDPTR	*data;          /**< The data. */
  unsigned long	length;   /**< Number of items used. */
  unsigned long	capacity; /**< Capacity of list. */
  unsigned int flags;     /**< Status flags. */
};

/** Generic comparator for sorts and searches. */
typedef int (*CmpFunc)(CONST VOIDPTR, CONST VOIDPTR);

/** Generic destroyer for iterations. */
typedef void (*DestroyFunc)(VOIDPTR);

/** Generic function called during iterations. */
typedef void (*IterateFunc)(VOIDPTR);

extern void gl_init(void);
/**<
 *  Initializes the list recycler control table.
 *  Until this function is called, no recycling will take place.
 *  This recycler control table should be tuned to your application.
 *  The list recycler is independent of the pool implementation.
 *  This function may be called more than once, although in general
 *  there is no reason to do so.
 */

#ifdef ASC_NO_POOL
#define LISTUSESPOOL FALSE
#else
#define LISTUSESPOOL TRUE
#endif
/**<
 *  Flag to select list management strategy.
 *  LISTUSESPOOL == TRUE allows the list module to use pool.[ch] to
 *  manage list memory overhead. Performance is enhanced this way.<br><br>
 *
 *  LISTUSESPOOL == FALSE removes the pool dependency completely, at
 *  a performance penalty.<br><br>
 *
 *  The following 3 functions work for either value of LISTUSESPOOL
 *  in some appropriate fashion: gl_init_pool(),  gl_destroy_pool(),
 *  gl_report_pool().
 */

extern void gl_init_pool(void);
/**<
 *  Sets up list overhead structure data management.
 *  This function should be called before anything can be built,
 *  ideally at startup time.  Do not call it again unless
 *  gl_destroy_pool() is called first.  If there is insufficient
 *  memory to compile anything at all, it does exit(2).   Do not
 *  call gl_create() before this is called if LISTUSESPOOL == TRUE.
 */

extern void gl_destroy_pool(void);
/**<
 *  Destroys list overhead structure data management.  This must be
 *  called to clean up before shutting down your application if
 *  gl_init_pool() was called.  Do not call this function while
 *  there are ANY lists in existence.  Do not attempt to create
 *  any lists after you call this unless you have recalled
 *  gl_init_pool().  Do not call this if gl_init_pool() has not
 *  been called.
 */

extern int gl_pool_initialized(void);
/**<
 *  Query whether the list pool has been initialized.
 *  Always returns TRUE if the list does not use pooling.
 *
 *  @return Returns TRUE if the pool has already been initialized,
 *          FALSE otherwise.
 */

extern void gl_report_pool(FILE *f);
/**<
 *  Prints a report on the recycle pool to f.
 *
 *  @param f Open file stream to which to print report.
 */

extern struct gl_list_t *gl_create(unsigned long capacity);
/**<
 *  Creates a new empty list having the specified initial capacity.
 *  If the number of list items later exceeds this size, the
 *  capacity will be expanded.  It is not crucial, but a good
 *  guess will increase the performance of this module.  There
 *  is an implementation-defined minimum list size, so the actual
 *  initial capacity can be larger than the requested capacity.<br><br>
 *
 *  Note that newly-created lists are both sorted and expandable.
 *  Destruction of the returned list is the responsibility of the
 *  caller.  Use gl_free_and_destroy() or gl_destroy() to do this.<br><br>
 *
 *  Do not call this function unless gl_init_pool() has been called
 *  if LISTUSESPOOL == TRUE.
 *
 *  Complexity: worst case O(capacity)   <br>
 *  Because memory proportional to capacity is allocated, depending on
 *  your memory management system, this could be proportional to the
 *  initial capacity.
 *
 *  @param capacity The desired initial list capacity.
 *  @return Returns a pointer to the new empty gl_list_t.
 */

extern void gl_free_and_destroy(struct gl_list_t *list);
/**<
 *  Destroys a list and deallocates all list items.
 *  The specified list will be destroyed (its memory is to be
 *  returned to free memory or the list pool).  Unlike with
 *  gl_destroy(), the items in the list WILL BE DEALLOCATED.
 *  This is appropriate when the list is considered the owner
 *  of the list items.  The specified list can be NULL.<br><br>
 *
 *  DO NOT CALL gl_free_and_destroy() ON BOTH A LIST AND IT'S
 *  gl_copy().  Since the copy is shallow, this would result in
 *  double deletion of the pointers and a likely crash.<br><br>
 *
 *  Complexity: worst case O(n)  <br>
 *  where n is the size of the list.  This is because memory proportional
 *  to n must be deallocated.  If that time is not proportional to the
 *  size then it should be O(1)
 *
 *  @param list A pointer to the gl_list_t to destroy.
 */

extern void gl_destroy(struct gl_list_t *list);
/**<
 *  Destroys a list without deallocating the list items.
 *  The specified list will be destroyed (its memory is to be
 *  returned to free memory or the list pool).  Unlike with
 *  gl_free_and_destroy(), the items in the list WILL NOT be
 *  deallocated.  This is appropriate when the list is considered
 *  to be storing pointers to data owned by someone else.  Note that
 *  the stored pointers are no longer available after calling this
 *  function, so copies of the pointers must exist somewhere to
 *  allow deallocation of the data.  The specified list can be NULL.<br><br>
 *
 *  If you want the items to be freed (a la the old gl_destroy(list,1))
 *  call gl_free_and_destroy() instead.<br><br>
 *
 *  Complexity: worst case O(n)  <br>
 *  where n is the size of the list.  I say this because memory proportional
 *  to n must be deallocated.  If that time is not proportional to the
 *  size then it should be O(1)
 *
 *  @param list A pointer to the gl_list_t to destroy.
 */

#ifndef NDEBUG
#define gl_fetch(list,pos) gl_fetchF((list),(pos))
#else
#define gl_fetch(list,pos) ((list)->data[((pos)-1)])
#endif
/**<
 *  Retrieves the data item stored at position pos in the list.
 *  This function is used to access the list elements.  It returns the
 *  pointer indexed by pos, which must be in the range [1..gl_length(list)]
 *  (checked by assertion).  The specified list may not be NULL
 *  (checked by assertion).<br><br>
 *
 *  The returned pointer is of type VOIDPTR (void *).  It will usually be
 *  necessary to cast the pointer to the correct type before use.  This is
 *  similar to what needs to be done with the pointer returned be malloc().<br><br>
 *
 *  Example:                                        <pre>
 *	     struct data_t *item;
 *	     item = (struct data_t *)gl_fetch(datalist,4);   </pre>
 *
 *  Complexity: O(1)
 *
 *  @param list CONST struct gl_list_t*, the list to query (non-NULL).
 *  @param pos  unsigned long, index to fetch [1..gl_length(list)].
 *  @return The list element at index pos as a VOIDPTR.
 *  @see gl_fetchF()
 */

extern VOIDPTR ASC_DLLSPEC gl_fetchF(CONST struct gl_list_t *list, unsigned long pos);
/**<
 *  Implementation function for gl_fetch() (debug mode).
 *  Do not call this function directly - use gl_fetch() instead.
 */

extern void gl_store(struct gl_list_t *list, unsigned long pos, VOIDPTR ptr);
/**<
 *  Stores an item in the list in position pos.  This procedure can only
 *  modify existing list items (i.e. 1 <= pos <= gl_length(list)).  It cannot
 *  expand the list length or capacity.  You must use gl_append_ptr() to
 *  do that.  The specified list may not be NULL (checked by assertion).<br><br>
 *
 *  Because any pointer type is convertible to a void pointer, you
 *  pass in a pointer to your data structure.  It stores whatever pointer
 *  you give it.<br><br>
 *
 *  Example:                                                      <pre>
 *      struct data_t *item;
 *      item = (struct data_t *)malloc(sizeof(struct data_t));
 *      various assignments to item->
 *      gl_store(list,4,item);   * This stores item               </pre>
 *
 *  COMPLEXITY: O(1)
 *
 *  @param list  The list to modify (non-NULL).
 *  @param pos   Index of position to modify, [1..gl_length(list)].
 *  @param ptr   Pointer to data to store at index pos.
 */

extern void gl_append_ptr(struct gl_list_t *list, VOIDPTR ptr);
/**<
 *  <!--  PROCEDURE gl_append_ptr(list,ptr);                           -->
 *  <!--  struct gl_list_t *list;                                      -->
 *  <!--  VOIDPTR ptr;                                                 -->
 *
 *  Appends ptr to the end of the list.  If the addition of ptr exceeds
 *  the list capacity, the list capacity is increased.  This and
 *  gl_append_list() are the only procedures that will expand the
 *  length and/or the capacity of a list.  ptr is always stored in
 *  position gl_length(list)+1.  The specified list may not be
 *  NULL, and the list must be expandable (checked by assertion).<br><br>
 *
 *  Because any pointer type is convertible to a void pointer, you
 *  pass in a pointer to your data structure.  It stores whatever pointer
 *  you give it.<br><br>
 *
 *  Example:                                                       <pre>
 *      struct data_t *item;
 *      item = (struct data_t *)malloc(sizeof(struct data_t));
 *      various assignments to item->
 *      gl_append_ptr(list,item);   * This stores item             </pre>
 *
 *  Complexity: worst case O(n)  <br>
 *  where n is the capacity of the list.  Normally it is O(1).  It is
 *  only when the list capacity is increased that it takes O(n).  This
 *  may also be an over estimate.
 *
 *  @param list  The list to modify (non-NULL).
 *  @param ptr   Pointer to data to append to the list.
 */

extern void gl_fast_append_ptr(struct gl_list_t *list, VOIDPTR ptr);
/**<
 *  Appends ptr to the end of the list without checking for adequate
 *  capacity.  If the addition of ptr exceeds the list capacity, the
 *  memory adjacent will be corrupted.  This function does not expand
 *  the list length or check it.  ptr is always stored in position
 *  gl_length(list)+1.   Only use this function in situations where
 *  you are absolutely sure the gl_length(list) < list->capacity.
 *  The specified list may not be NULL (checked by assertion).<br><br>
 *
 *  Because any pointer type is convertible to a void pointer, you
 *  pass in a pointer to your data structure.  It stores whatever pointer
 *  you give it.<br><br>
 *
 *  Example: See gl_append_ptr.<br>
 *  Intended use is that you create a list of the size you know you
 *  need (but may want to expand at a later time) and then call this.
 *  This is faster than gl_store().<br><br>
 *
 *  Complexity: O(1)
 *
 *  @param list  The list to modify (non-NULL).
 *  @param ptr   Pointer to data to append to the list.
 */

extern void gl_append_list(struct gl_list_t *extendlist,
                           struct gl_list_t *list);
/**<
 *  Appends the pointers stored in list to the end of extendlist.
 *  If the addition exceeds the capacity of extendlist, the extendlist
 *  capacity is increased.  This function and gl_append_ptr() are the
 *  only procedures that will expand the length and/or the capacity of
 *  a list.  Neither extendlist nor list may be NULL, and expandlist
 *  must be expandable (checked by assertion).<br><br>
 *
 *  No new list is created.  extendlist is changed if there is data in list.
 *  list is never changed in any case.<br><br>
 *
 *  Example:                                                    <pre>
 *      gl_append_list(oldlist,addlist);
 *      This stores contents of addlist at the end of oldlist   </pre>
 *
 *  Complexity: worst case O(gl_length(list)+gl_length(extendlist))  <br>
 *  Normally it is O(gl_length(list)).  It is
 *  only when the extendlist capacity is increased that it is worst.  This
 *  may also be an over estimate depending on your allocator.
 *
 *  @param extendlist  The list to modify (non-NULL).
 *  @param list        The list to append to extendlist (non-NULL).
 */

#ifndef NDEBUG
#define gl_length(list) gl_lengthF(list)
#else
#define gl_length(list) ((list)->length)
#endif
/**<
 *  Returns the length of list.  This is the number of items it
 *  is currently holding.  There is no checking for whether list
 *  is a valid pointer.  The specified list may not be NULL
 *  (checked by assertion).  Use gl_safe_length() if the
 *  list pointer might be NULL.<br><br>
 *
 *  Complexity: O(1)
 *
 *  @param list  CONST struct gl_list_t*. the list to query (non-NULL).
 *  @return The length as an unsigned long.
 *  @see gl_lengthF()
 */
extern unsigned long ASC_DLLSPEC gl_lengthF(CONST struct gl_list_t *list);
/**<
 *  Implementation function for gl_length() (debug mode).
 *  Do not call this function directly - use gl_length() instead.
 *
 *  @param list  CONST struct gl_list_t*. the list to query (non-NULL).
 */

extern unsigned long gl_safe_length(CONST struct gl_list_t *list);
/**<
 *  Returns the length of list.  This is the number of items it
 *  is currently holding.  Unlike gl_length() this function
 *  tolerates NULL input, for which the return value is 0.<br><br>
 *
 *  Complexity: O(1)
 *
 *  @param list The list to query.
 *  @return The number of items stored in list.
 */

extern unsigned long gl_capacity(CONST struct gl_list_t *list);
/**<
 *  Returns the *potential* capacity of the list at the current time.
 *  This is the number of items the list can store without having to
 *  expand its capacity.  To find out the number of items in the list
 *  use gl_length().  The specified list may be NULL, in which case
 *  0 is returned.<br><br>
 *
 *  Complexity: O(1)
 *
 *  @param list The list to query.
 *  @return The number of items that can be stored in list without expanding.
 */

extern int gl_sorted(CONST struct gl_list_t *list);
/**<
 *  Query whether the specified list is sorted.  A list having 0 or 1
 *  element is always sorted.  The specified list may not be NULL
 *  (checked by assertion).
 *
 *  Complexity: O(1)
 *
 *  @param list The list to query (non-NULL).
 *  @return Non-zero if the list is sorted and 0 otherwise.
 */

#if LISTIMPLEMENTED
extern void gl_ptr_sort(struct gl_list_t *list, int inc);
/**<
 *  <!--  PROCEDURE gl_ptr_sort(list,inc);                             -->
 *  <!--  struct gl_list_t *list;                                      -->
 *  <!--  int inc;                                                     -->
 *
 *  This will sort the list data using Quick Sort.  It
 *  uses a somewhat intelligent pivot choice, so it is unlikely that the
 *  worst case of O(n^2) will arise.  I however will not guarantee that.
 *  We will sort the list into order of decreasing order of address if
 *  inc == 0 and increasing order if increasing !=0.
 *  Which value of inc you use can drastically affect performance of
 *  other functions; consider your application carefully.<br><br>
 *
 *  Complexity: worst case O(n^2) average case O(n*log(n))  <br><br>
 *  The multiplier is considerably smaller than for gl_sort, however.
 */
#endif

extern void gl_sort(struct gl_list_t *list, CmpFunc func);
/**<
 *  Sorts the list in increasing order using Quick Sort.  It
 *  uses a somewhat intelligent pivot choice, so it is unlikely that the
 *  worst case of O(n^2) will arise.  This is not, however, guaranteed.
 *  The caller must furnish a procedure "func" that compares two list items
 *  and returns an integer > 0 if item1 > item2 and otherwise <= 0.
 *  NOTE: THE COMPARISON PROCEDURE SHOULD BE ABLE TO HANDLE NULL POINTERS
 *  GRACEFULLY.  Neither the specified list nor func may not be NULL
 *  (checked by assertion).<br><br>
 *
 *  Example:                                                    <pre>
 *    const struct itemtype *item1, *item2;  // whatever type being stored in the list.
 *    int func(item1,item2);
 *    gl_sort(my_list, func);                                   </pre>
 *
 *  Complexity: worst case O(n^2) average case O(n*log(n))
 *
 *  @param list The list to sort (non-NULL).
 *  @param func The comparison function to call during the sort.
 */

#if LISTIMPLEMENTED
extern void gl_insert_ptr_sorted(struct gl_list_t *list, VOIDPTR ptr, int inc);
/**<
 *  <!--  PROCEDURE gl_insert_ptr_sorted(list,ptr,inc);                -->
 *  <!--  struct gl_list_t *list;                                      -->
 *  <!--  VOIDPTR ptr;                                                 -->
 *  <!--  int inc;                                                     -->
 *
 *  This procedure will insert an item into the list in the position
 *  where it belongs to keep the list sorted. If inc != 0,
 *  sort will be in increasing order of address, else it will be
 *  in decreasing order of address.
 *  Which value of inc you use can drastically affect performance of
 *  other functions; consider your application carefully.<br><br>
 *
 *  Example:                                                      <pre>
 *      struct data_t *item;
 *      item = (struct data_t *)malloc(sizeof(struct data_t));
 *      various assignments to item->
 *      gl_insert_ptr_sorted(list,item,0); * This stores item     </pre>
 *
 *  Complexity: O(length)
 */
#endif

extern void gl_insert_sorted(struct gl_list_t *list, VOIDPTR ptr, CmpFunc func);
/**<
 *  Inserts an item into a sorted list in the position where it belongs to
 *  keep the list sorted.  The specified func is used to compare to list items
 *  in the following way:
 *	- item1 > item2  returns > 0
 *	- item1 = item2  returns = 0
 *	- item1 < item2  returns < 0
 *  This function should be the same as used to gl_sort() the list
 *  If the list is not sorted, it will be sorted and the item added
 *  in the appropriate location.  Neither the specified list nor
 *  func may be NULL, and the list must be expandable (checked
 *  by assertion).<br><br>
 *
 *  Because any pointer type is convertible to a void pointer, you
 *  pass in a pointer to your data structure.  It stores whatever pointer
 *  you give it.<br><br>
 *
 *  Example:                                                          <pre>
 *      struct data_t *item;
 *      item = (struct data_t *)malloc(sizeof(struct data_t));
 *      various assignments to item->
 *      gl_insert_sorted(list,item,sortfunc); * This stores item      </pre>
 *
 *  Complexity: O(length)
 *
 *  @param list The list to modify (non-NULL).
 *  @param ptr  Pointer to data to append to the list.
 *  @param func The comparison function to call during the sort.
 */

extern void ASC_DLLSPEC gl_iterate(struct gl_list_t *list, IterateFunc func);
/**<
 *  Executes the function func on all the members of the list.
 *  It will always execute the function on the items in the order
 *  they appear in the list, 1,2,3..gl_length(list).  The function
 *  should handle NULL pointers as input gracefully.  Neither the
 *  specified list nor the func may be NULL (checked by
 *  assertion).<br><br>
 *
 *  Complexity: O(n*O(func))
 *
 *  @param list The list to iterate through (non-NULL).
 *  @param func The function to execute for each list item.
 */

extern unsigned long gl_ptr_search(CONST struct gl_list_t *list,
                                   CONST VOIDPTR match,
                                   int increasing);
/**<
 *  Searches the list for a specific item and returns the position
 *  where it is stored.  If match is not found, the function returns 0.
 *  The definition of match is as follows:
 *  It will return item i iff (gl_fetch(list,i) == match).
 *  The specified list may not be NULL (checked by assertion).<br><br>
 *
 *  If the list is sorted this function will use a binary search,
 *  otherwise it will search linearly. The binary search will proceed
 *  based on increasing. If you have the list sorted in increasing ptr
 *  order, give this increasing=1, else increasing=0.<br><br>
 *
 *  CAUTION!!  If the list is sorted by some other criteria than
 *  pointer address order, this function may erroneously return 0.
 *  If you give us an incorrect increasing, we may erroneously
 *  return 0.  <br><br>
 *
 *  Complexity: if   gl_sorted(list) O(log gl_length(list))
 *              else O(gl_length(list))<br>
 *  Multiplier is better than for gl_search().
 *
 *  @param list       The list to search (non-NULL).
 *  @param match      The pointer to search for in list.
 *  @param increasing TRUE if the list is sorted in increasing
 *                    pointer order, FALSE otherwise.
 *  @return The position of match in the list [1..gl_length(list)],
 *          0 if match not found.
 */

extern unsigned long gl_search(CONST struct gl_list_t *list,
                               CONST VOIDPTR match,
                               CmpFunc func);
/**<
 *  Searches a list for a specified item and returns the position where
 *  it is stored.  If match is not found, the function returns 0.
 *  The definition of match is as follows:
 *  It will return item i iff (*func)(gl_fetch(list,i),match) = 0.
 *  Neither the specified list nor func may be NULL (checked by
 *  assertion).<br><br>
 *
 *  If the list is sorted this function will use a binary search, otherwise
 *  it will search linearly.  The user must provide func, a comparison
 *  function returning:
 *  - item1 > item2 ==> 1   (actually it need only be > 0)
 *  - item1 = item2 ==> 0
 *  - item1 < item2 ==> -1  (actually it need only be < 0)
 *
 *  Complexity: if   gl_sorted(list) O(log gl_length(list))
 *              else O(gl_length(list))
 *
 *  @param list  The list to search (non-NULL).
 *  @param match The pointer to search for in list.
 *  @param func  The comparison function to call during the search.
 *  @return The position of match in the list[1..gl_length(list)],
 *          0 if match not found.
 */

extern unsigned long gl_search_reverse(CONST struct gl_list_t *list,
                                       CONST VOIDPTR match,
                                       CmpFunc func);
/**<
 *  Searches a list for a specified item and returns the position where
 *  it is stored.  This is similar to gl_search(), except that if the list
 *  is NOT sorted, it does a linear search starting from the last element in
 *  the list and working toward the first element.  For sorted lists,
 *  this function calls gl_search() to do a binary search.  Neither the
 *  specified list nor func may be NULL (checked by assertion).
 *
 *  @param list  The list to search (non-NULL).
 *  @param match The pointer to search for in list.
 *  @param func  The comparison function to call during the search.
 *  @return The position of match in the list[1..gl_length(list)],
 *          0 if match not found.
 */

extern int gl_empty(CONST struct gl_list_t *list);
/**<
 *  Query whether the list is empty.  An empty list has no items stored.
 *  The specified list may not be NULL (checked by assertion).
 *
 *  Complexity: O(1)
 *
 *  @param list The list to query (non-NULL).
 *  @return TRUE if the list is empty, FALSE otherwise.
 */

extern int gl_unique_list(CONST struct gl_list_t *list);
/**<
 *  Query whether the pointers stored in the list are all unique.
 *  The specified list may be NULL, in which case TRUE is returned.
 *
 *  Complexity: O(nlogn)
 *
 *  @param list The list to query.
 *  @return TRUE if the pointers in the list are unique, FALSE otherwise.
 */

extern void gl_delete(struct gl_list_t *list,
                      unsigned long pos,
                      int dispose);
/**<
 *  Deletes the item in position pos from the list.
 *  pos must be in the range [1..gl_length(list)], although pos = 0 is
 *  tolerated (and has no effect).  The upper bound is checked by assertion.
 *  If dispose is true then the memory associated with the item will also
 *  be deallocated.  The specified list may not be NULL (checked by
 *  assertion).<br><br>
 *
 *  Complexity: O(gl_length(list))  <br>
 *  This is because all the list items to the right of the deleted
 *  item must be shuffled left one space.
 *
 *  @param list    The list to modify (non-NULL).
 *  @param pos     The position of the item to remove [1..gl_length(list)].
 *  @param dispose If non-zero the item will be free'd;
 *                 if 0 the item will not be deallocated.
 *  @todo Why does general/list:gl_delete() check for pos == 0?  Should
 *        it be an ASSERTRANGE like other functions, or is there some
 *        reason to expect this lone function to be called with pos == 0?
 */

extern void gl_reverse(struct gl_list_t *list);
/**<
 *  Reverses the ordering of the list.
 *  If the list given is marked as sorted, this function will
 *  leave it marked as sorted, though an inverse CmpFunc is now
 *  required for search/insertion.  The specified list may be
 *  NULL, in which case no action is taken.<br><br>
 *
 *  Complexity: O(gl_length(list))
 *
 *  @param list The list to modify.
 */

extern void gl_reset(struct gl_list_t *list);
/**<
 *  Resets the list to a *clean* state as if it had just been created.
 *  As such, the list will be considered as expandle and sorted but
 *  with a length of zero.  The items in the list are NOT deallocated.
 *  The specified list may not be NULL (checked by assertion).<br><br>
 *
 *  Complexity 1.   <br>
 *  This is useful for a list that is being used as a scratch list,
 *  and needs to be *reset* between operations.
 *
 *  @param list The list to reset (non-NULL).
 */

extern struct gl_list_t *gl_copy(CONST struct gl_list_t *list);
/**<
 *  Copies a list.  The copy of the list will have its own memory
 *  associated with it, but the items themselves are shared between the
 *  lists.  That is, the copy of the items is shallow and the 2 lists will
 *  share pointers to the same memory locations.  The starting list is not
 *  changed in any way.  The specified list may not be NULL (checked by
 *  assertion).  Destruction of the returned list is the responsibility
 *  of the caller, but be careful not to call gl_free_and_destroy() on both
 *  the original and copy.  This will result in double deletion of the
 *  pointers and likely a crash.<br><br>
 *
 *  Complexity O(gl_length(list))
 *
 *  @param list The list to copy (non-NULL).
 *  @return A shallow copy of the list.
 */

extern struct gl_list_t *gl_concat(CONST struct gl_list_t *list1,
                                   CONST struct gl_list_t *list2);
/**<
 *  Concatenates two lists into a new list.  Neither of the original
 *  lists is changed in any way.  The list that is returned will contain
 *  the items in list1 followed by the items in list2.  Neither list1
 *  list 2 may be NULL (checked by assertion).  Destruction of the
 *  returned list is the responsibility of the caller.<br><br>
 *
 *  Complexity O(gl_length(list1)+gl_length(list2))
 *
 *  @param list1 The 1st list to append to the new list (non-NULL).
 *  @param list2 The 2nd list to append to the new list (non-NULL).
 *  @return A new list containing the items of list1 and list2 appended.
 */

extern int gl_compare_ptrs(CONST struct gl_list_t *list1,
                           CONST struct gl_list_t *list2);
/**<
 *  Compares the data (ptrs or whatever) in two lists.  Returns a code
 *  as follows:
 *    - -1 list1 < list2
 *    -  0 list1 == list2
 *    -  1 list1 > list2
 *  The comparison is performed on an item-by-item basis, with the relative
 *  values of the first different items found determining the "greater" list.
 *  If all items up to the length of the shorter list have equal values,
 *  the longer list is considered "greater".  For pointers this is a weird
 *  function. But, we sometimes store ints in lists and want to sort lists
 *  of lists.  This function does NOT tolerate NULL inputs.  Calling it with
 *  a NULL pointer will abort the program.<br><br>
 *
 *  Complexity O(SHORTER_OF(list1,list2))
 *
 *  @param list1 The 1st list to compare (non-NULL).
 *  @param list2 The 2st list to compare (non-NULL).
 *  @return Returns -1 if list1 < list2, 1 if list1 > list 2, and 0 if the
 *          lists are equal.
 *  @todo  Why does general/list:gl_compare_ptrs() use Asc_Panic()
 *         on a NULL pointer?  Assertions used in rest of module.
 */

extern void gl_set_sorted(struct gl_list_t *list, int TRUE_or_FALSE);
/**<
 *  Sets the sorted flag for the list based on TRUE_or_FALSE.
 *  Setting a list to sorted should be done only if you are sure
 *  that the list is sorted.  The list may not be NULL (checked
 *  by assertion).
 *
 *  @param list The list to modify (non-NULL).
 */

extern int gl_expandable(struct gl_list_t *list);
/**<
 *  Query whether the specified list is expandable.  The specified list
 *  may not be NULL (checked by assertion).
 *
 *  Complexity: O(1)
 *
 *  @param list The list to query (non-NULL).
 *  @return Non-zero if the list is expandable and 0 otherwise.
 */

extern void gl_set_expandable(struct gl_list_t *list, int TRUE_or_FALSE);
/**<
 *  Sets the expandable flag for the list based on TRUE_or_FALSE.
 *  All lists are expandable until this flag is explicitly cleared.
 *  Setting a list to non-expandable should be done only when you are
 *  quite sure that you want this. It is sometimes useful to have a
 *  list not be expandable, as in the case when someone is depending
 *  on the address of a list member.  However, list append and insert
 *  functions will fail if the list is not expandable.  The list may
 *  not be NULL (checked by assertion).
 *
 *  @param list The list to modify (non-NULL).
 */

extern VOIDPTR *gl_fetchaddr(CONST struct gl_list_t *list,
                             unsigned long pos);
/**<
 *  Returns the address of the pointer stored at position pos.  This
 *  is sometimes useful for ptr to ptr manipulation.  pos must be in
 *  the range [1..gl_length(list).  The specified list may not be NULL.
 *  Both of these conditions are checked by assertion.
 *
 *  @param list The list to query (non-NULL).
 *  @param pos  The position of the data to fetch, [1..gl_length(list).
 *  @return The address of the pointer as position pos.
 */

extern void gl_emptyrecycler(void);
/**<
 *  Empties the list recycler.
 *  To improve runtime performance, this list module trys to reuse destroyed
 *  lists.  However, it may be beneficial to empty recycling bin from time
 *  to time.  The most appropriate time for this is before shutdown and
 *  perhaps after an instantiation.  If LISTRECYCLERDEBUG is defined, a
 *  summary of the recycler status is also reported on stdout.
 */

extern void gl_reportrecycler(FILE *fp);
/**<
 *  Prints a report of the recycler status to fp.
 *  To improve runtime performance, this list module trys to reuse destroyed
 *  lists. This function reports the recycler status.  Note that the report is
 *  only printed if LISTRECYCLERDEBUG is defined.  The specified fp may not be
 *  NULL (checked by assertion).
 *
 *  @param fp  Pointer to file stream to receive report.
 */

#endif /* ASC_LIST_H */

