/*
 *  Ascend Pooled Memory Manager
 *  by Benjamin Andrew Allan
 *  Created: 2/96
 *  Version: $Revision: 1.1 $
 *  Version control file: $RCSfile: pool.h,v $
 *  Date last modified: $Date: 1997/07/18 11:37:02 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1996 Benjamin Andrew Allan
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
 */
/*
 *  Contents:     Pool Memory module
 *
 *  Authors:
 *                Benjamin Andrew Allan
 *
 *  Dates:        05/95 - original version: utilities/mem.[ch]
 *                        nearly generic element allocation routines.
 *                02/96 - copied to compiler and swapped pool for mem in
 *                        names. deleted all the Karl foo of mem.c.
 *                03/96 - retuned some of the parameters for compiler.
 *
 *  Description:
 *                The generic element memory manager is yet another
 *                implementation of a fixed byte size malloc system
 *                that is more efficient than most standard mallocs. This
 *                is possible because we make the fixed size assumption.
 *                The idea is clearly not original.
 *                For best results, you must understand your memory
 *                usage pattern and tune the pool_create_store parameters
 *                accordingly.
 *                This has similar functionality to free_store.[ch] but
 *		    is a rather more efficient implementation with a better
 *		    interface. The mem_ implementation of this code
 *		    remains in utilities/ because the solvers (which may
 *		    also stand alone) need to be able to link without the
 *		    compiler goop.
 */

/*
 *  When #including pool.h, make sure these files are #included first:
 *         #include "compiler.h"
 */


#ifndef __POOL_H_SEEN__
#define __POOL_H_SEEN__
/* requires
 * #include <stdio.h>
 * #include "compiler.h"
 */

/*
 *  The following definitions provide a generic and reasonably efficient memory
 *  allocation system for situations where many many objects of the same size
 *  need to be "allocated and deallocated" rapidly and stored efficiently:
 *  that is to say when normal malloc is too slow and expensive.
 *  This scheme does not play reference count games. Instead any elements that
 *  have been created and then used and subsequently "freed" go on a list
 *  and are handed out again at the next opportunity. The list is run LIFO.
 *  The list is associated with the pool_store_t.
 *
 *  There is one restriction on the elements: they will be a multiple of the
 *  size of a pointer, even if you specify otherwise. If this is too large,
 *  write your own allocator. Think about how your elements align. On many
 *  architectures doubles should only be stored at addresses that are
 *  multiples of 8 bytes.
 *  There is the implicit restriction on a store that it must contain
 *  no more than MAXINT elements. If this is too small, write your own
 *  allocator.
 *
 *  Note for the intelligent:
 *  If you specify an element size that is not a nice multiple of your machine
 *  word length, you are *very* likely to get data alignment (bus) errors.
 *  E.g., if your element is not a convenient multiple of
 *  sizeof(double) in size, double data are likely to die.
 *
 *  The allocation scheme looks like so:
 *  [struct pool_store_header | ]
 *          __________________|
 *         |
 *         V
 *   pool  _____________________________________________________
 *         | b | b | b | b | b | b | b | b | b | b | b | b | b |
 *         -----------------------------------------------------
 *  where each b is a pointer to an array of size 'width' of elements.
 *
 *  The size characteristics of this scheme are tunable at run time so that
 *  it can be scaled well when the number of elements and likely amount
 *  of expansion required are known.
 */

typedef struct pool_store_header *pool_store_t;
/*
 *  The token for this memory system. malloc doesn't tell you much
 *  about its internals, and we aren't telling you about ours.
 *  You can't dereference or free this pointer yourself, so there's
 *  no need to even know that it IS a pointer, now is there?
 */

struct pool_statistics {
  double p_eff;		/* bytes in use / bytes allocated */
  double p_recycle;	/* avg reuses per element */
  int elt_total;	/* current elements existing in store*/
  int elt_taken;	/* fresh elements handed out */
  int elt_inuse;	/* elements the user currently has */
  int elt_onlist;	/* elements awaiting reuse */
  int elt_size;		/* bytes/element, as mem sees it */
  int str_len;		/* length of active pool. */
  int str_wid;          /* elements/pointer in pool. */
};
/* The reporting structure for a pool_store_header query. */

extern void pool_get_stats(struct pool_statistics *, pool_store_t);
/*
 *  pool_get_stats(p_stats,ps);
 *  struct pool_statistics *p_stats;
 *  pool_store_t ps;
 *
 *  Stuffs the user interface structure, p_stats, with info
 *  derived from ps given.
 *  If pool_LIGHTENING (see below) is TRUE, no statistics
 *  except elt_total, elt_taken, elt_onlist, and elt_size are
 *  available.
 */

extern pool_store_t pool_create_store(int, int, size_t, int, int);
/*
 *  ps = pool_create_store(length, width, eltsize, deltalen, deltapool);
 *  pool_store_t ps;
 *  int length,width,deltalen,deltapool;
 *  size_t eltsize;
 *
 *  Returns a pool_store_t which can be used in future. The pool_store_t
 *  contains all the necessary accounting information, but in particular
 *  the eltsize is fixed at creation. All elements requested from ps will be
 *  pointers to eltsize bytes of memory.
 *  Returns NULL if a store of the requested length*width*eltsize
 *  cannot be initially allocated.
 *
 *  The user may request more than length*width elements from the store:
 *  this will cause it to grow. It will grow (internally) in chunks of
 *  deltalen*width elements. The pool vector above grows in chunks of
 *  deltapool, the extra pointers in it being NULL until needed.
 *
 *  Info for tuning purposes:
 *
 *  For maximum efficiency, eltsize should be an integer
 *  multiple of sizeof(void *). If it is not, elts will be padded
 *  so that this is the case. This is to avoid pointer data misalignment.
 *  This restriction may or may not help avoid alignment problems with
 *  items inside the user's element structure.
 *
 *  Width should (for some architectures) be such that
 *  width*eltsize = 2^n - 32 for some n fairly large (heuristic: n= 9..13).
 *  Widths that are too large may be prone to causing excess page faults,
 *  though the process cpu time reported by the clock() can be much
 *  better for extremely large sizes.
 *  Widths that are too small will result in an excessive number of pool
 *  expansions which may severely limit performance on some VM systems.
 *  See deltapool below about pool expansions.
 *  If you know something about the page size of your architecture, fiddling
 *  with width may reduce help you reduce page fault or cache miss count in
 *  some uses.
 *
 *  Deltalen is the number of additional pointers in the pool that will be
 *  allocated when more elements are needed than are available internally,
 *  as already noted.
 *
 *  Deltapool is the size change of the pool array described above: it should
 *  be as large as you are willing to tolerate. The pool array starts out
 *  completely filled (all pointers allocated). When the pool needs more
 *  pointers it gets them in chunks of at least deltapool. These additional
 *  pointers will not automatically have elements allocated to them; rather,
 *  they will be initialized to NULL and filled in only as the chunks of
 *  deltalen*width elements are required.
 */

extern void *pool_get_element(pool_store_t);
/*
 *  eltpointer = (elt_type *)pool_get_element(ps);
 *  pool_store_t ps;
 *  <the elt_type you want is your business> *eltpointer;
 *
 *  Returns a void pointer to a blob of memory of the eltsize
 *  set when ps was created. You must cast it appropriately.
 *  The blob data is not initialized in any particular way.
 *  Returns NULL iff store growth is required and the operating
 *  system is unable to allocate the required memory.
 */

extern void pool_get_element_list(pool_store_t, int, void **);
/*
 *  pool_get_element_list(ps, len, ellist);
 *  int len;
 *  pool_store_t ps;
 *
 *  NOT IMPLEMENTED.
 *
 *  Takes the pointer array, ellist, of length len provided by the user
 *  and fills it with pointers to elements from the store.
 *  There is not necessarily any relation (memory map wise) between the
 *  locations pointed to by successive entries in the ellist returned.
 *  Ellist should point to an array with enough space for len pointers.
 *  Returns NULL in ellist[0] iff store growth is required and the operating
 *  system is unable to allocate the required memory.
 *
 *  The user is reminded that if he knows how many elements he needs
 *  ahead of time, he is probably better off mallocing the array himself.
 */

#define pool_DEBUG FALSE
/*  pool_DEBUG set TRUE causes the pool_store routines to do
 *  some RATHER expensive checking. It should be set to
 *   FALSE.
 */
#define pool_LIGHTENING FALSE
/*  pool_LIGHTENING set TRUE causes pool_store routines to assume the
 *  user is perfect: no sanity checks are at all necessary and most
 *  internal accounting can be disabled. Noone with an ounce of sanity
 *  would ever set this flag to TRUE unless the code using the
 *  pool module was proven bug free. It makes the allocator smaller
 *  and faster, though, by ~15%.
 *  ;-)
 *  This flag exists to make it easy to test the theory that the
 *  accounting overhead in this code is not of significant cost.
 *  Below 1e5 elements it really isn't bad enough to justify the
 *  assumption that the user is perfect.
 */

#if pool_DEBUG
#define pool_free_element(a,b) pool_free_elementF((a),(b),__FILE__)
#else
#define pool_free_element(a,b) pool_free_elementF((a),(b))
#endif
extern void pool_free_elementF(pool_store_t, void *
#if pool_DEBUG
,CONST char *
#endif
);
/*
 *  pool_free_element(ps,(void *)eltpointer[,__FILE__]);
 *  pool_store_t ps;
 *  <your elttype> *eltpointer;
 *
 *  Returns an element to the store.
 *  If you return the same pointer twice, we will have
 *  no qualms about returning it to you twice. We won't necessarily
 *  return it to you twice, though.
 *  If pool_DEBUG is TRUE, eltpointer will be checked for belonging
 *  to ps. If you call pool_free_element with a pointer the ps does
 *  not recognize, it will not be freed and a message will be
 *  sent to ASCERR.
 *  If pool_DEBUG is FALSE, eltpointer will be assumed to belong
 *  with the ps in question. The implications of handing pool_free_element
 *  an element of the wrong size or from the wrong ps (bearing in
 *  mind the LIFO reuse of elements) should be obvious. If they are
 *  not, stop using these routines.
 *  If at any time the number of elements freed exceeds the number
 *  handed out, we will whine (unless pool_LIGHTENING).
 *  If you send us a NULL pointer, we will ignore it completely.
 */

#if pool_DEBUG
#define pool_clear_store(ps) pool_clear_storeF((ps),__FILE__)
#else
#define pool_clear_store(ps) pool_clear_storeF(ps)
#endif
extern void pool_clear_storeF(pool_store_t
#if pool_DEBUG
, CONST char *
#endif
);
/*
 *  pool_clear_store(ps[,__FILE__]);
 *  pool_store_t ps;
 *
 *  Clears the books in ps. That is, we reset the ps to think
 *  that __all__ elements are freshly available and have never
 *  been handed out.
 *  If pool_DEBUG TRUE, verifies that all elements have been pool_freed
 *  first and whines if not.
 *  Get and free calls will be balanced to see if spurious elements
 *  have been handed in. (This is a heuristic check).
 *  The clear process will cause any spurious pointers that were
 *  turned in via pool_free_element to be forgotten about.
 *
 *  Clearing a store is not necessary for pool_destroy_store.
 *  Recycling is faster from the recycle list than from a cleared store, ~2%.
 *  Clear is provided for users who want to obtain elements with a higher
 *  probability of successive elements being near each other.
 */

extern void pool_destroy_store(pool_store_t);
/*
 *  pool_destroy_store(ps);
 *  pool_store_t ps;
 *
 *  Deallocates everything associated with the ps.
 *  If pool_DEBUG TRUE, verifies that all elements have been pool_freed
 *  first and whines if not.
 *  If pool_DEBUG FALSE, just nukes everything unconditionally.
 */

extern void pool_print_store(FILE *, pool_store_t,unsigned);
/*
 *  pool_print_store(fp,ps,detail);
 *  FILE *fp;
 *  pool_store_t ps;
 *  unsigned detail;
 *  Displays a bunch of statistics about a pool_store_t on the file
 *  given. Which ones depends on detail.
 *  If detail 0, displays just summary statistics.
 *  If detail 1, just internal statistics.
 *  If detail >1, displays both.
 */

extern size_t pool_sizeof_store(pool_store_t);
/*
 *  pool_sizeof_store(ps);
 *  Returns the current total byte usage of the store.
 */

#endif
