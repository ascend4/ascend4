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
 *
 */

/*
 *  Abstract
 *
 *    The purpose of this module is to provide a kind of flexible array.
 *  The flexible array has two interesting characteristics.  It allows
 *  contant time(O(1)) retrieval of list items and it is almost infinitely
 *  extendable(i.e. has no preset limit on the number of items in the list.
 *  One disadvanatage is that the list only stores pointers to items.
 *  It does not use much extra memory while providing these services.
 *  This module will also provide some routines which I consider standard
 *  list type operations.  In the comment for each procedure I will make
 *  some predictions about the efficiency of that operation.  Any
 *  modification of these procedures should live up to those claims.
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
 *  2/26/88	added gl_copy, gl_concat
 *  3/31/88	added additional commenting
 *  2/18/96	added defines when -DNDEBUG is active. We can't
 *  		afford the calls in a production compiler. (Ben Allan)
 *  2/23/96	Added recycling feature to reuse gl_lists. (TGWE)
 *  3/25/96	Improved recycling feature. (Ben Allan)
 *  3/30/96	Took dispose flag off gl_destroy and added a mirror
 *		function gl_free_and_destroy to take its place.
 *                Added pooled list heads (optional) which depends on
 *                pool.[ch] and improves performance substantially.
 *		Tuned to large applications. (Ben Allan)
 *  9/9/96	Changed flags from struct to int.  (Ben Allan)
 *  10/2/96	Added switch over -DMOD_REALLOC to use ascreallocPURE.
 *		If this file is compiled -DMOD_REALLOC, purify leaks of
 *		list->data are real, OTHERWISE it may be noise.
 *              Skipping the call to gl_init may also help dianosis.
 *  9/20/97     Added gl_compare_ptrs.
 */

/*
 *  When #including list.h, make sure these files are #included first:
 *         #include "compiler.h"
 */


#ifndef __LIST_H_SEEN__
#define __LIST_H_SEEN__
/* requires
# #include <stdio.h>
# #include"compiler.h"
*/

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#define LISTIMPLEMENTED 0 		/* BAA_DEBUG changes thatneed work */

/*
 * The following bit fields are defined for the gllist flags
 */
#define gsf_SORTED 0x1
#define gsf_EXPANDABLE 0x2

struct gl_list_t {
  VOIDPTR		*data;		/* The data*/
  unsigned long		length;		/* Number of items used*/
  unsigned long		capacity;	/* Capacity of list*/
  unsigned int flags;			/* Status flags*/
};

/* generic comparator for sorts and searches */
typedef int (*CmpFunc)(CONST VOIDPTR, CONST VOIDPTR);

/* generic destroyer for iterations */
typedef void (*DestroyFunc)(VOIDPTR);

extern void gl_init(void);
/*
 *  Until this function is called, no recycling will take place.
 *  This function initializes a list recycler control table.
 *  This recycler control table should be tuned to your application.
 *  The list recycler is independent of the pool implementation.
 */

#ifdef ASC_NO_POOL
#define LISTUSESPOOL FALSE
#else
#define LISTUSESPOOL TRUE
#endif
/*
 *  LISTUSESPOOL == TRUE allows the list module to use pool.[ch] to
 *  manage list memory overhead. Performance is enhanced this way.
 *
 *  LISTUSESPOOL == FALSE removes the pool dependency completely, at
 *  a performance penalty.
 *
 *  The following 3 functions work for either value of LISTUSESPOOL
 *  in some appropriate fashion.
 */

extern void gl_init_pool(void);
/*
 *  gl_init_pool();
 *  Sets up list overhead structure data management
 *  before anything can be built, ideally at startup time.
 *  Do not call it again unless gl_destroy_pool is called first.
 *  If insufficient memory to compile anything at all, does exit(2).
 *  Do not call gl_create before this is called if LISTUSESPOOL == TRUE.
 */

extern void gl_destroy_pool(void);
/*
 *  gl_destroy_pool();
 *  Destroy list overhead structure data management. This must be called to
 *  clean up before shutting down you application if gl_init_pool was called.
 *  Do not call this function while there are ANY lists in existence.
 *  Do not attempt to create any lists after you call this unless you
 *  have recalled gl_init_pool.
 *  Do not call this if gl_init_pool has not been called.
 */

extern void gl_report_pool(FILE *);
/*
 *  gl_report_pool(f);
 *  FILE *f;
 *  Reports on the recycle pool to f.
 */

extern	struct gl_list_t *gl_create(unsigned long);
/*
 *  FUNCTION gl_create(capacity)
 *  unsigned long capacity;
 *
 *  This function takes one argument which is the anticipated size of the
 *  list.  This size simply sets the initial capacity of the list.  If the
 *  number of items execeeds this size, the capacity is expand.  It is no
 *  crucial, but a good guess will increase the performance of this module.
 *
 *  Complexity: worst case O(capacity)
 *  Because memory proportional to capacity is allocated, depending on
 *  your memory management system, this could be proportional to the
 *  initial capacity.
 */

extern	void gl_free_and_destroy(struct gl_list_t *);
/*
 *  PROCEDURE gl_free_and_destroy(list);
 *  struct gl_list_t *list;
 *
 *  This procedure takes ONE argument, a list that is
 *  to be destroyed(whose memory is to be returned to free memory).
 *  The items in the list WILL BE DEALLOCATED.
 *
 *  Complexity: worst case O(n)
 *  where n is the size of the list.  I say this because memory proportional
 *  to n must be deallocated.  If that time is not proportional to the
 *  size then it should be O(1)
 */

extern	void gl_destroy(struct gl_list_t *);
/*
 *  PROCEDURE gl_destroy(list);
 *  struct gl_list_t *list;
 *
 *  This procedure takes ONE argument, a list that is
 *  to be destroyed(whose memory is to be returned to free memory).
 *  The items in the list WILL NOT be deallocated.
 *
 *  If you want the items to be freed (a la the old gl_destroy(list,1))
 *  call gl_free_and_destroy(list) instead.
 *
 *  Complexity: worst case O(n)
 *  where n is the size of the list.  I say this because memory proportional
 *  to n must be deallocated.  If that time is not proportional to the
 *  size then it should be O(1)
 */

#ifndef NDEBUG
#define gl_fetch(l,p) gl_fetchF((l),(p))
#else
#define gl_fetch(l,p) ((l)->data[((p)-1)])
#endif
extern VOIDPTR gl_fetchF(CONST struct gl_list_t *,unsigned long);
/*
 *  FUNCTION gl_fetch(list,pos);
 *  const struct gl_list_t *list;
 *  unsigned long pos;
 *
 *  This function is used to access the list elements.  It returns the
 *  pointer indexed by pos.
 *  pos must be in the range [1..gl_length(list)].  Otherwise it causes
 *  an error.
 *
 *  Even though the declared type of this function is void pointer,
 *  it will return a pointer to your data structure.  You must coerce
 *  the type to be correct as shown below.  This is similar with what
 *  you have to do to malloc calls which return pointers to chars.
 *
 *  Example:
 *	struct data_t *item;
 *	item = (struct data_t *)gl_fetch(datalist,4);
 *
 *  Complexity: O(1)
 */

extern	void gl_store(struct gl_list_t *,unsigned long, VOIDPTR);
/*
 *  PROCEDURE gl_store(list,pos,ptr);
 *  struct gl_list_t *list;
 *  unsigned long pos;
 *  VOIDPTR ptr;
 *
 *  This procedure is used to store an item in the list in position
 *  pos.  This procedure can only modify existing list items.  It cannot
 *  expand the list length or capacity.  You must use gl_append_ptr to
 *  do that(see below).
 *
 *  Even though the declared type of ptr is a void pointer, you
 *  pass in a pointer to your data structure.  It store whatever pointer
 *  you give it.
 *
 *  Example:
 *	struct data_t *item;
 *	item = (struct data_t *)malloc(sizeof(struct data_t));
 *  various assignments to item->
 *  gl_store(list,4,item); * This stores item
 *
 *  INPUT
 *	pos	in the range [1..gl_length(list)]
 *
 *  COMPLEXITY: O(1)
 */

extern	void gl_append_ptr(struct gl_list_t *,VOIDPTR);
/*
 *  PROCEDURE gl_append_ptr(list,ptr);
 *  struct gl_list_t *list;
 *  VOIDPTR ptr;
 *
 *  This procedure will append ptr to the end of the list.  If the
 *  addition ptr exceeds the list capacity, the list capacity is increased.
 *  This and gl_append_list are the only procedures that will expand
 *  the length and/or the capacity of a list.
 *  ptr is always store in position gl_length(list)+1.
 *
 *  Even though the declared type of ptr is a void pointer, you
 *  pass in a pointer to your data structure.  It store whatever pointer
 *  you give it.
 *
 *  Example:
 *	struct data_t *item;
 *	item = (struct data_t *)malloc(sizeof(struct data_t));
 *  various assignments to item->
 *  gl_append_ptr(list,item); * This stores item
 *
 *  Complexity: worst case O(n)
 *  where n is the capacity of the list.  Normally it is O(1).  It is
 *  only when the list capacity is increased that it takes O(n).  This
 *  may also be an over estimate.
 */

extern	void gl_fast_append_ptr(struct gl_list_t *,VOIDPTR);
/*
 *  PROCEDURE gl_fast_append_ptr(list,ptr);
 *  struct gl_list_t *list;
 *  VOIDPTR ptr;
 *
 *  This procedure will append ptr to the end of the list.  If the
 *  addition ptr exceeds the list capacity, the memory adjacent will be
 *  corrupted. This does not expand the length or check it.
 *  ptr is always store in position * gl_length(list)+1.
 *  Only use this in situations where you are absolutely sure the
 *  appended pointer will not cause list to grow.
 *
 *  Even though the declared type of ptr is a void pointer, you
 *  pass in a pointer to your data structure.  It store whatever pointer
 *  you give it.
 *
 *  Example: See gl_append_ptr.
 *  Intended use is that you create a list of the size you know you
 *  need (but may want to expand at a later time) and then call this.
 *  This is faster than gl_store.
 *
 *  Complexity: O(1)
 */

extern	void gl_append_list(struct gl_list_t *,struct gl_list_t *);
/*
 *  PROCEDURE gl_append_list(extendlist,list);
 *  struct gl_list_t *list,*extendlist;
 *
 *  This procedure will append the pointers of the list to the
 *  extendlist.  If the addition exceeds the extendlist capacity,
 *  the extendlist capacity is increased.
 *  This and gl_append_ptr are the only procedures that will expand
 *  the length and/or the capacity of a list.
 *
 *  No new list is created.
 *  extendlist is changed if there is data in list.
 *  list is never changed in any case.
 *
 *  Example:
 *	gl_append_list(oldlist,addlist);
 *  This stores contents of addlist at the end of oldlist
 *
 *  Complexity: worst case O(gl_length(list)+gl_length(extendlist))
 *  Normally it is O(gl_length(list)).  It is
 *  only when the extendlist capacity is increased that it is worst.  This
 *  may also be an over estimate depending on your allocator.
 */

#ifndef NDEBUG
#define gl_length(l) gl_lengthF(l)
#else
#define gl_length(l) ((l)->length)
#endif
extern	unsigned long gl_lengthF(CONST struct gl_list_t *);
/*
 *  FUNCTION gl_lengthF(list)
 *  const struct gl_list_t *list;
 *
 *  This function returns the length of list, the number of items it
 *  is currently holding.
 *
 *  Complexity: O(1)
 */

extern unsigned long gl_safe_length(CONST struct gl_list_t *list);
/*
 *  FUNCTION gl_safe_length(list)
 *  const struct gl_list_t *list;
 *
 *  This function returns the length of list, the number of items it
 *  is currently holding.
 *  Tolerates NULL input, for which the return value is 0.
 *
 *  Complexity: O(1)
 */


extern	unsigned long gl_capacity(CONST struct gl_list_t *);
/*
 *  FUNCTION gl_length(list)
 *  const struct gl_list_t *list;
 *
 *  This function returns the *potential* capacity of the list,
 *  at the current time. To find out the number of items in the list
 *  use gl_length(list).
 *
 *  Complexity: O(1)
 */

extern	int gl_sorted(CONST struct gl_list_t *);
/*
 *  FUNCTION gl_sorted(list);
 *  const struct gl_list_t *list;
 *
 *  Returns 1 if sorted and 0 otherwise.
 *  Complexity: O(1)
 */

#if LISTIMPLEMENTED
extern void gl_ptr_sort(struct gl_list_t *, int);
/*
 *  PROCEDURE gl_ptr_sort(list,inc);
 *  struct gl_list_t *list;
 *  int inc;
 *
 *  This will sort the list data using Quick Sort.  It
 *  uses a somewhat intelligent pivot choice, so it is unlikely that the
 *  worst case of O(n^2) will arise.  I however will not guarantee that.
 *  We will sort the list into order of decreasing order of address if
 *  inc == 0 and increasing order if increasing !=0.
 *  Which value of inc you use can drastically affect performance of
 *  other functions; consider your application carefully.
 *
 *  Complexity: worst case O(n^2) average case O(n*log(n))
 *  The multiplier is considerably smaller than for gl_sort, however.
 */
#endif

extern void gl_sort(struct gl_list_t *, CmpFunc);
/*
 *  PROCEDURE gl_sort(list,func);
 *  struct gl_list_t *list;
 *  CmpFunc func;
 *
 *  This will sort the list in increasing order using Quick Sort.  It
 *  uses a somewhat inteligent pivot choice, so it is unlikely that the
 *  worst case of O(n^2) will arise.  I however will not guarantee that.
 *  You must furnish a procedure "func" that compares two list items
 *  and returns > 0 if item1 > item2 and otherwise <= 0.
 *
 *  int func(item1,item2);
 *  const struct itemtype *item1,*item2;
 *
 *  itemtype is whatever type you are storing in the list.
 *  NOTE: YOUR COMPARISON PROCEDURE SHOULD BE ABLE TO HANDLE NULL POINTERS
 *  GRACEFULLY.
 *
 *  Complexity: worst case O(n^2) average case O(n*log(n))
 */

#if LISTIMPLEMENTED
extern	void gl_insert_ptr_sorted(struct gl_list_t *,VOIDPTR,int);
/*
 *  PROCEDURE gl_insert_ptr_sorted(list,ptr,inc);
 *  struct gl_list_t *list;
 *  VOIDPTR ptr;
 *  int inc;
 *
 *  This procedure will insert an item into the list in the position
 *  where it belongs to keep the list sorted. If inc != 0,
 *  sort will be in increasing order of address, else it will be
 *  in decreasing order of address.
 *  Which value of inc you use can drastically affect performance of
 *  other functions; consider your application carefully.
 *
 *  Example:
 *	struct data_t *item;
 *	item = (struct data_t *)malloc(sizeof(struct data_t));
 *  various assignments to item->
 *  gl_insert_ptr_sorted(list,item,0); * This stores item
 *  Complexity: O(length)
 */
#endif

extern	void gl_insert_sorted(struct gl_list_t *,VOIDPTR, CmpFunc);
/*
 *  PROCEDURE gl_insert_sorted(list,ptr,func);
 *  struct gl_list_t *list;
 *  VOIDPTR ptr;
 *  CmpFunc func;
 *
 *  This procedure will insert an item into the list in the position
 *  where it belongs to keep the list sorted.  You must pass a function
 *  that will compare to list items in the following way.  If the list is
 *  not sorted the item will be added to the end.
 *	item1 > item2	returns > 0
 *	item1 = item2	returns = 0
 *	item1 < item2	returns < 0
 *  This function could also be used in gl_sort above.
 *
 *  Even though the declared type of ptr is a pointer to a char, you
 *  pass in a pointer to your data structure.  It store whatever pointer
 *  you give it.
 *
 *  Example:
 *	struct data_t *item;
 *	item = (struct data_t *)malloc(sizeof(struct data_t));
 *  various assignments to item->
 *  gl_insert_sorted(list,item,sortfunc); * This stores item
 *  Complexity: O(length)
 */

extern	void gl_iterate(struct gl_list_t *,void (*)(VOIDPTR) );
/*
 *  PROCEDURE gl_iterate(list,func);
 *  struct gl_list_t *list;
 *  void (*func)(VOIDPTR);
 *
 *  This procedure will execute the function func on all the members of
 *  list.  It will always execute the function on the items in the order
 *  they appear in the list, 1,2,3..gl_length(list).  The function
 *  should handle nil pointers as input gracefully.
 *  Complexity: O(n*O(func))
 */

extern unsigned long gl_ptr_search(CONST struct gl_list_t *,CONST VOIDPTR,int);
/*
 *  FUNCTION gl_ptr_search(list,match,inc);
 *  const struct gl_list_t *list;
 *  const VOIDPTR match;
 *  int inc;
 *
 *  This procedure will search a list for a item that matches match and
 *  return the index where it is stored.  If a match is not found it
 *  returns 0.
 *  The definition of match is as follows:
 *  It will return item i iff (gl_fetch(list,i) == match)
 *
 *  If the list is sorted this function will use a binary search,
 *  otherwise it will search linearly. The binary search will proceed
 *  based on inc. If you have the list sorted in increasing ptr order,
 *  give this inc=1, else inc=0. If you give us an incorrect inc, we
 *  may erroneously return 0.
 *
 *  Complexity: if   gl_sorted(list) O(log gl_length(list))
 *              else O(gl_length(list))
 *  Multiplier better than for gl_search.
 */

extern	unsigned long gl_search(CONST struct gl_list_t *,
    CONST VOIDPTR, CmpFunc);
/*
 *  FUNCTION gl_search(list,match,func);
 *  const struct gl_list_t *list;
 *  const VOIDPTR match;
 *  CmpFunc func;
 *
 *  This procedure will search a list for a item that matches match and
 *  return the index where it is stored.  If a match is not found it
 *  returns 0.
 *  The definition of match is as follows:
 *  It will return item i iff (*func)(gl_fetch(list,i),match) = 0
 *
 *  If the list is sorted this function will use a binary search, otherwise
 *  it will search linearly.  func should be defined like below(it is
 *  identical to the insert sorted func).  If it cannot find the item
 *  you are searching for it returns 0.
 *
 *  int (*func)(item1,item2);
 *  struct data_t *item1,*item2;
 *
 *  item1 > item2 ==> 1	(actually it need only be > 0)
 *  item1 = item2 ==> 0
 *  item1 < item2 ==> -1  (actually it need only be < 0)
 *
 *  Complexity: if   gl_sorted(list) O(log gl_length(list))
 *              else O(gl_length(list))
 */

extern unsigned long gl_search_reverse(CONST struct gl_list_t *,
                                       CONST VOIDPTR, CmpFunc);
/*
 *  FUNCTION gl_search(list,match,func);
 *  const struct gl_list_t *list;
 *  const VOIDPTR match;
 *  CmpFunc func;
 *
 *  This function similar to gl_search() except that if the list is NOT
 *  sorted, it does a linear search starting from the last element in
 *  the list and working toward the first element.  For sorted lists,
 *  this function calls gl_search() to do a binary search.
 *
 *  See the documentation under gl_search() for more information.
 */

extern	int gl_empty(CONST struct gl_list_t *);
/*
 *  FUNCTION gl_empty(list)
 *  const struct gl_list_t *list;
 *
 *  If the list is empty it returns TRUE otherwise it returns FALSE.
 *  Complexity: O(1)
 */

extern	int gl_unique_list(CONST struct gl_list_t *);
/*
 *  FUNCTION gl_unique_list(list)
 *  const struct gl_list_t *list;
 *
 *  If the list is of unique pointers it returns TRUE
 *  otherwise it returns FALSE.
 *  Complexity: O(nlogn)
 */

extern	void gl_delete(struct gl_list_t *,unsigned long,int);
/*
 *  PROCEDURE gl_delete(list,pos,dispose);
 *  struct gl_list_t *list;
 *  unsigned long pos;
 *  int dispose;
 *
 *  This procedure will delete the item in position pos from the list.
 *  pos must be in the range [1..gl_length(list)].
 *  If dispose is true then this will deallocate the memory associate
 *  with the item being deleted.
 *  Complexity: O(gl_length(list))
 *  This is because all the list items to the right of the deleted
 *  item must be shuffled left one space.
 */

extern	void gl_reverse(struct gl_list_t *);
/*
 *  PROCEDURE gl_reverse(list);
 *  struct gl_list_t *list;
 *  unsigned long pos;
 *  int dispose;
 *
 *  This procedure will reverse the ordering of the list given.
 *  If the list given is marked as sorted, this function will
 *  leave it marked as sorted, though an inverse CmpFunc is now
 *  required for search/insertion.
 *  Complexity: O(gl_length(list))
 */

extern void gl_reset(struct gl_list_t *);
/*
 *  PROCEDURE gl_reset(list);
 *  struct gl_list_t list;
 *
 *  This procedure will reset the list to a *clean* state as if it had
 *  just been created. As such this list will be considered as expandle
 *  and sorted but with a length of zero.
 *  Complexity 1.
 *  This is useful for a list that is being used as a scratch list,
 *  and needs to be *reset* between operations.
 */

extern	struct gl_list_t *gl_copy(CONST struct gl_list_t *);
/*
 *  FUNCTION gl_copy(list);
 *  const struct gl_list_t *list;
 *
 *  This function will copy a list.  The copy of the list will have its
 *  own memory associated with it, but the items themselves are shared
 *  between the lists.  What I mean by this is that the i'th item in
 *  each list both point to the same piece of memory which holds the
 *  i'th item.  List is not changed in any way.
 *
 *  Complexity O(gl_length(list))
 */

extern	struct gl_list_t *gl_concat(CONST struct gl_list_t *,
                                    CONST struct gl_list_t *);
/*
 *  FUNCTION gl_concat(list1,list2);
 *  const struct gl_list_t *list1,*list2;
 *
 *  This function will concatenate two lists neither of which is changed
 *  in any way, shape or form.  The list that is returns will contain
 *  the items in list1 followed by the items in list2.
 *
 *  Complexity O(gl_length(list1)+gl_length(list2))
 */

extern	int gl_compare_ptrs(CONST struct gl_list_t *, CONST struct gl_list_t *);
/*
 *  FUNCTION gl_compare_ptrs(list1,list2);
 *  const struct gl_list_t *list1,*list2;
 *
 *  This function will compare the data (ptrs or whatever) in two lists)
 *  returning which ever list has greater data on an item-by-item basis.
 *  Returns -1,0,1. Longer lists are considered > than shorter lists
 *  which are identical up the the length of the shorter.
 *  For pointers this is a weird function. But we sometimes store ints
 *  in lists and want to sort lists of lists.
 *  Complexity O(SHORTER_OF(list1,list2))
 *  This function does NOT tolerate NULL inputs.
 */

extern void gl_set_sorted(struct gl_list_t *);
/*
 *  Set the sorted flag.  This should be done only when you are sure that
 *  the list is sorted.
 */

extern void gl_set_notexpandable(struct gl_list_t *);
/*
 *  Set the expandable flag.  This should be done only when you are sure
 *  that you dont want the list to be expandable. All lists are expandable
 *  until this flag is explicitly set. It sometimes useful to have a list
 *  not be expandable, as in the case when someone is depending on the
 *  address of a list member.
 */

extern VOIDPTR *gl_fetchaddr(CONST struct gl_list_t *list, unsigned long);
/*
 *  Return the address of the pointer to the information stored at
 *  position pos. This is sometimes useful for ptr to ptr manipulation.
 */

extern void gl_emptyrecycler();
/*
 *  To improve runtime performance, this list module trys to reuse destroyed
 *  lists.  However, it may be beneficial to empty recycling bin from time
 *  to time.  The most appropriate time for this is before shutdown and
 *  perhaps after an instantiation.
 */

extern void gl_reportrecycler(FILE *);
/*
 *  To improve runtime performance, this list module trys to reuse destroyed
 *  lists. This function reports the recycler status.
 */
#endif /* __LIST_H_SEEN__ */






