/*
 *  Memory module
 *  by Karl Westerberg, Ben Allan
 *  Created: 6/90
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: mem.h,v $
 *  Date last modified: $Date: 1997/07/18 12:04:22 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Carnegie Mellon University
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
 *  COPYING.  COPYING is in ../compiler.
 */


/*
 *  Contents:     Memory module
 *
 *  Authors:      Karl Westerberg
 *                Benjamin Andrew Allan
 *
 *  Dates:        06/90 - original version
 *                08/94 - cleanup with bzero, bcopy, and corrected casting.
 *                        BAA, JJZ
 *                05/95 - add nearly generic element allocation routines.
 *                        BAA
 *
 *  Description:  It is convenient for pointers and ints to be the same
 *                size, and most C compilers support this.  However for
 *                many machines (particularly ones with 16-bit words),
 *                there is more RAM available than can be accessed by a
 *                single int, so standard C pointers may only be able to
 *                point to a limited region of memory.  The matter is
 *                further complicated by the fact that the pointer to
 *                address conversion rules may be different for pointers
 *                to functions (code) than for pointers to other objects
 *                (data).  This module attempts to partially circumvent
 *                these obstacles, by allowing the user to address memory
 *                with long ints.
 *
 *                As you can see, the above was written in the bad old
 *                days pre-ANSI/STDC.
 *                This file now serves to isolate bzero, bcopy, etc
 *                from solver code should they become nonstandard.
 *
 *                The generic element memory manager is yet another
 *                implementation of a fixed byte size malloc system
 *                that is more efficient than most standard mallocs. This
 *                is possible because we make the fixed size assumption.
 *                The idea is clearly not original.
 *                For best results, you must understand your memory
 *                usage pattern and tune the mem_create_store parameters
 *                accordingly.
 */
#ifndef mem_NULL
/* requires
# #include <stdio.h>
*/

#define mem_NULL NULL
#define mem_code_NULL NULL
#define mem_address(ptr) ((long)(ptr))
#define mem_code_address(ptr) ((long)(ptr))

/* use the defines below based on these rather than these directly */
extern void mem_move(POINTER, POINTER, unsigned);
extern void mem_move_disjoint(POINTER, POINTER, int);
extern void mem_repl_byte(POINTER, unsigned, unsigned);
extern void mem_zero_byte(POINTER, unsigned, unsigned);
extern void mem_repl_word(POINTER, unsigned, unsigned);
/* here are the defines */
#define mem_move_cast(from,too,nbytes) \
   mem_move((POINTER)(from),(POINTER)(too),(unsigned)(nbytes))

#define mem_copy_cast(from,too,nbytes) \
   mem_move_disjoint((POINTER)(from),(POINTER)(too),(int)(nbytes))

#define mem_repl_byte_cast(too,byte,nbytes) \
   mem_repl_byte((POINTER)(too),(unsigned)(byte),(unsigned)(nbytes))

#define mem_zero_byte_cast(too,byte,nbytes) \
   mem_zero_byte((POINTER)(too),(unsigned)(byte),(unsigned)(nbytes))
/* byte is ignored by mem_zero_byte. It is a placeholder for mem_repl_byte
   substitutability.
*/

#define mem_repl_word_cast(too,word,nwords) \
   mem_repl_word((POINTER)(too),(unsigned)(word),(unsigned)(nwords))

/* the following are pretty much a monument to Karl. */
#if 0
extern int mem_get_byte();
#endif
extern int mem_get_int();
extern long mem_get_long();
extern double mem_get_float();
extern double mem_get_double();
extern void mem_set_byte();
extern void mem_set_int();
extern void mem_set_long();
extern void mem_set_float();
extern void mem_set_double();

#define	mem_get_unsigned(from)	((unsigned)mem_get_int(from))
#define	mem_set_unsigned(from,u) mem_set_int(from,(int)u)

/***************************************************************************\
 The following definitions provide a generic and reasonably efficient memory
 allocation system for situations where many many objects of the same size
 need to be "allocated and deallocated" rapidly and stored efficiently:
 that is to say when normal malloc is too slow and expensive.
 This scheme does not play reference count games. Instead any elements that
 have been created and then used and subsequently "freed" go on a list
 and are handed out again at the next opportunity. The list is run LIFO.
 The list is associated with the mem_store_t.
 
 There is one restriction on the elements: they will be a multiple of the
 size of a pointer, even if you specify otherwise. If this is too large,
 write your own allocator.
 There is the implicit restriction on a store that it must contain
 no more than MAXINT elements. If this is too small, write your own allocator.

 Note for the intelligent:
 If you specify an element size that is not a nice multiple of your machine
 word length, you are *very* likely to get data alignment (bus) errors.
 E.g., if your element is not a convenient multiple of sizeof(double) in size,
 double data are likely to die.

 The allocation scheme looks like so:
  [struct mem_store_header | ]
         __________________|
        |
        V
  pool  _____________________________________________________
        | b | b | b | b | b | b | b | b | b | b | b | b | b |
        -----------------------------------------------------
  where each b is a pointer to an array of size 'width' of elements.

 The size characteristics of this scheme are tunable at run time so that
 it can be scaled well when the number of elements and likely amount
 of expansion required are known.
\***************************************************************************/

typedef struct mem_store_header *mem_store_t;
/**
  The token for this memory system. malloc doesn't tell you much
  about its internals, and we aren't telling you about ours.
  You can't dereference or free this pointer yourself, so there's
  no need to even know that it IS a pointer, now is there?
**/

struct mem_statistics {
  double m_eff;		/* bytes in use / bytes allocated */
  double m_recycle;	/* avg reuses per element */
  int elt_total;	/* current elements existing in store*/
  int elt_taken;	/* fresh elements handed out */
  int elt_inuse;	/* elements the user currently has */
  int elt_onlist;	/* elements awaiting reuse */
  int elt_size;		/* bytes/element, as mem sees it */
  int str_len;		/* length of active pool. */
  int str_wid;          /* elements/pointer in pool. */
};
/* The reporting structure for a mem_store_header query. */

extern void mem_get_stats(struct mem_statistics *, mem_store_t);
/*
 *  mem_get_stats(m_stats,ms);
 *  struct mem_statistics *m_stats;
 *  mem_store_t ms;
 *
 *  Stuffs the user interface structure, m_stats, with info
 *  derived from ms given.
 *  If mem_LIGHTENING (see below) is TRUE, no statistics
 *  except elt_total, elt_taken, elt_onlist, and elt_size are
 *  available.
 */

extern mem_store_t mem_create_store(int, int, size_t, int, int);
/*
 *  ms = mem_create_store(length, width, eltsize, deltalen, deltapool);
 *  mem_store_t ms;
 *  int length,width,deltalen,deltapool;
 *  size_t eltsize;
 *
 *  Returns a mem_store_t which can be used in future. The mem_store_t
 *  contains all the necessary accounting information, but in particular
 *  the eltsize is fixed at creation. All elements requested from ms will be
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

extern void *mem_get_element(mem_store_t);
/*
 *  eltpointer = (elt_type *)mem_get_element(ms);
 *  mem_store_t ms;
 *  <the elt_type you want is your business> *eltpointer;
 *
 *  Returns a void pointer to a blob of memory of the eltsize
 *  set when ms was created. You must cast it appropriately.
 *  The blob data is not initialized in any particular way.
 *  Returns NULL iff store growth is required and the operating
 *  system is unable to allocate the required memory.
 */

extern void mem_get_element_list(mem_store_t, int, void **);
/*
 *  mem_get_element_list(ms, len, ellist);
 *  int len;
 *  mem_store_t ms;
 *  
 *** NOT IMPLEMENTED.
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

#define mem_DEBUG FALSE
/* mem_DEBUG set TRUE causes the mem_store routines to do
   some RATHER expensive checking. It should be set to
   FALSE.
*/
#define mem_LIGHTENING FALSE
/* mem_LIGHTENING set TRUE causes mem_store routines to assume the
   user is perfect: no sanity checks are at all necessary and most
   internal accounting can be disabled. Noone with an ounce of sanity
   would ever set this flag to TRUE unless the code using the
   mem module was proven bug free. It makes the allocator smaller
   and faster, though, by ~15%.
   ;-)
   This flag exists to make it easy to test the theory that the
   accounting overhead in this code is not of significant cost.
   Below 1e5 elements it really isn't bad enough to justify the 
   assumption that the user is perfect.
*/

extern void mem_free_element(mem_store_t, void *);
/*
 *  mem_free_element(ms,(void *)eltpointer);  
 *  mem_store_t ms;
 *  <your elttype> *eltpointer;
 *
 *  Returns an element to the store.
 *  If you return the same pointer twice, we will have
 *  no qualms about returning it to you twice. We won't necessarily
 *  return it to you twice, though.
 *  If mem_DEBUG is TRUE, eltpointer will be checked for belonging
 *  to ms. If you call mem_free_element with a pointer the ms does
 *  not recognize, it will not be freed and a message will be
 *  sent to stderr.
 *  If mem_DEBUG is FALSE, eltpointer will be assumed to belong
 *  with the ms in question. The implications of handing mem_free_element
 *  an element of the wrong size or from the wrong ms (bearing in
 *  mind the LIFO reuse of elements) should be obvious. If they are
 *  not, stop using these routines.
 *  If at any time the number of elements freed exceeds the number
 *  handed out, we will whine (unless mem_LIGHTENING).
 *  If you send us a NULL pointer, we will ignore it completely.
 */

extern void mem_clear_store(mem_store_t);
/*
 *  mem_clear_store(ms);
 *  mem_store_t ms;
 *
 *  Clears the books in ms. That is, we reset the ms to think
 *  that __all__ elements are freshly available and have never
 *  been handed out.
 *  If mem_DEBUG TRUE, verifies that all elements have been mem_freed
 *  first and whines if not.
 *  Get and free calls will be balanced to see if spurious elements
 *  have been handed in. (This is a heuristic check).
 *  The clear process will cause any spurious pointers that were
 *  turned in via mem_free_element to be forgotten about.
 *
 *  Clearing a store is not necessary for mem_destroy_store.
 *  Recycling is faster from the recycle list than from a cleared store, ~2%.
 *  Clear is provided for users who want to obtain elements with a higher
 *  probability of successive elements being near each other.
 */

extern void mem_destroy_store(mem_store_t);
/*
 *  mem_destroy_store(ms);
 *  mem_store_t ms;
 *
 *  Deallocates everything associated with the ms.
 *  If mem_DEBUG TRUE, verifies that all elements have been mem_freed
 *  first and whines if not.
 *  If mem_DEBUG FALSE, just nukes everything unconditionally.
 */

extern void mem_print_store(FILE *, mem_store_t,unsigned);
/*
 *  mem_print_store(fp,ms,detail);
 *  FILE *fp;
 *  mem_store_t ms;
 *  unsigned detail;
 *  Displays a bunch of statistics about a mem_store_t on the file
 *  given. Which ones depends on detail.
 *  If detail 0, displays just summary statistics.
 *  If detail 1, just internal statistics.
 *  If detail >1, displays both.
 */

extern size_t mem_sizeof_store(mem_store_t);
/*
 *  mem_sizeof_store(ms);
 *  Returns the current total byte usage of the store.
 */

#endif
