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

/** @file
 *  Memory module.
 *  <pre>
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
 *                usage pattern and tune the mem_create_store() parameters
 *                accordingly.  See mem_create_store() for more information.
 *
 *  Detailed Description of Memory Manager:
 *
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
 *  Even More Details
 *
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
 *
 *  Requires:     #include <stdio.h>
 *                #include "utilities/ascConfig.h"
 *  </pre>
 *  @todo Do we need to maintain the seemingly duplicate memory utilities and
 *        pooled allocators of utilites/ascMalloc.c and utilities/mem.c?  These
 *        look like they could be consolidated into a single module.
 *
 */

#ifndef mem_NULL

#define mem_NULL      NULL
#define mem_code_NULL NULL
#define mem_address(ptr)      ((long)(ptr))
#define mem_code_address(ptr) ((long)(ptr))

#define mem_move_cast(from,too,nbytes) \
   mem_move((POINTER)(from),(POINTER)(too),(size_t)(nbytes))
/**<
 *  Copies nbytes of data from memory location from to memory location too.
 *  The memory regions can be overlapping.
 *
 *  @param from   Pointer to memory to copy from.
 *  @param too    Pointer to memory to receive copy.
 *  @param nbytes The number of bytes to copy (size_t).
 *  @return No return value.
 *  @see mem_move()
 */

extern void mem_move(POINTER from, POINTER too, size_t nbytes);
/**<
 *  Implementation function for mem_move_cast().  Do not call this
 *  function directly - use mem_move_cast() instead.
 */

#define mem_copy_cast(from,too,nbytes) \
   mem_move_disjoint((POINTER)(from),(POINTER)(too),(size_t)(nbytes))
/**<
 *  Copies nbytes of data from memory location from to memory location too.
 *  The memory regions can NOT be overlapping.
 *
 *  @param from   Pointer to memory to copy from.
 *  @param too    Pointer to memory to receive copy.
 *  @param nbytes The number of bytes to copy (size_t).
 *  @return No return value.
 *  @see mem_move_disjoint()
 */

extern void mem_move_disjoint(POINTER from, POINTER too, size_t nbytes);
/**<
 *  Implementation function for mem_copy_cast().  Do not call this
 *  function directly - use mem_copy_cast() instead.
 */

#define mem_repl_byte_cast(too,byte,nbytes) \
   mem_repl_byte((POINTER)(too),(unsigned)(byte),(size_t)(nbytes))
/**<
 *  Replaces nbytes of data at memory location too with byte.
 *
 *  @param too    Pointer to start of block to be modified.
 *  @param byte   The character to write (unsigned int).
 *  @param nbytes The number of bytes to modify (size_t).
 *  @return No return value.
 *  @see mem_repl_byte()
 */

extern void mem_repl_byte(POINTER too, unsigned byte, size_t nbytes);
/**<
 *  Implementation function for mem_repl_byte_cast().  Do not call this
 *  function directly - use mem_repl_byte_cast() instead.
 */

#define mem_zero_byte_cast(too,byte,nbytes) \
   mem_zero_byte((POINTER)(too),(unsigned)(byte),(size_t)(nbytes))
/**<
 *  Zeroes nbytes of data at memory location too.
 *  byte is ignored - it is a placeholder for mem_repl_byte
 *  substitutability.
 *
 *  @param too    Pointer to start of block to be modified.
 *  @param byte   Ignored (unsigned).
 *  @param nbytes The number of bytes to zero (size_t).
 *  @return No return value.
 *  @see mem_zero_byte()
 */

extern void mem_zero_byte(POINTER too, unsigned byte, size_t nbytes);
/**<
 *  Implementation function for mem_zero_byte_cast().  Do not call this
 *  function directly - use mem_zero_byte_cast() instead.
 */

#define mem_repl_word_cast(too,word,nwords) \
   mem_repl_word((POINTER)(too),(unsigned)(word),(size_t)(nwords))
/**<
 *  Replaces nwords of data at memory location too with word.
 *
 *  @param too    Pointer to start of block to be modified.
 *  @param word   The word to write (unsigned).
 *  @param nbytes The number of bytes to modify (size_t).
 *  @return No return value.
 *  @see mem_repl_word()
 */

extern void mem_repl_word(POINTER too, unsigned word, size_t nwords);
/**<
 *  Implementation function for mem_repl_word_cast().  Do not call this
 *  function directly - use mem_repl_word_cast() instead.                    
 */

/* the following are pretty much a monument to Karl. */
#if 0
extern int mem_get_byte(long from);               /**< Returns the byte located at from. */
#endif
extern unsigned char mem_get_byte(long from);     /**< Returns the byte located at from. */
extern int mem_get_int(long from);                /**< Returns the int located at from. */
extern long mem_get_long(long from);              /**< Returns the long located at from. */
extern double mem_get_float(long from);           /**< Returns the float located at from. */
extern double mem_get_double(long from);          /**< Returns the double located at from. */
extern void mem_set_byte(long from, int b);       /**< Sets the byte located at from. */
extern void mem_set_int(long from, int i);        /**< Sets the int located at from. */
extern void mem_set_long(long from, long l);      /**< Sets the long located at from. */
extern void mem_set_float(long from, double f);   /**< Sets the float located at from. */
extern void mem_set_double(long from, double d);  /**< Sets the double located at from. */

#define	mem_get_unsigned(from)	((unsigned)mem_get_int(from))
/**< Returns the unsigned located at from. */
#define	mem_set_unsigned(from,u) mem_set_int(from,(int)u)
/**< Sets the unsigned located at from. */

/*---------------------------------------------------------------------------
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
---------------------------------------------------------------------------*/

typedef struct mem_store_header *mem_store_t;
/**<
 *  The token for this memory system.  Internal details of the
 *  implementation are private.  Do not dereference or free this 
 *  pointer.
 */

/**
 *  Memory statistics data structure.
 *  This is the reporting structure for a pool_store_header query.
 */
struct mem_statistics {
  double m_eff;       /**< bytes in use / bytes allocated */
  double m_recycle;   /**< avg reuses per element */
  int elt_total;      /**< current elements existing in store*/
  int elt_taken;      /**< fresh elements handed out */
  int elt_inuse;      /**< elements the user currently has */
  int elt_onlist;     /**< elements awaiting reuse */
  int elt_size;       /**< bytes/element, as mem sees it */
  int str_len;        /**< length of active pool. */
  int str_wid;        /**< elements/pointer in pool. */
};

extern void mem_get_stats(struct mem_statistics *m_stats, mem_store_t ms);
/**<
 *  Get statistics about a memory store.
 *  Stuffs the user's interface structure, m_stats, with info
 *  derived from ms given.
 *  If mem_LIGHTENING (see below) is TRUE, no statistics
 *  except elt_total, elt_taken, elt_onlist, and elt_size are
 *  available.
 *
 *  @param m_stats Pointer to a mem_statistics struct to receive
 *                 the info.  If m_stats is NULL, an error message
 *                 is printed and the function returns.
 *  @param ms      Pointer to the memory store to query.
 */

extern mem_store_t mem_create_store(int length, int width, size_t eltsize,
                                    int deltalen, int deltapool);
/**<
 *  Creates and returns a new memory store. The returned mem_store_t
 *  contains all the necessary accounting information, but in particular
 *  the eltsize is fixed at creation. All elements requested from ms
 *  will be pointers to eltsize bytes of memory.
 *  Returns NULL if a store of the requested length*width*eltsize
 *  cannot be initially allocated.<br><br>
 *
 *  The user may request more than length*width elements from the store:
 *  this will cause it to grow. It will grow (internally) in chunks of
 *  deltalen*width elements. The pool vector above grows in chunks of
 *  deltapool, the extra pointers in it being NULL until needed.<br><br>
 *
 *  @param length    The initial number of width*eltsize blocks that the
 *                   new pool store will contain.  If length < 1, an error
 *                   message is printed and NULL is returned.
 *
 *  @param width     Number of elements in each block.
 *                   Width should (for some architectures) be such that
 *                   width*eltsize = 2^n - 32 for some n fairly large
 *                   (heuristic: n= 9..13).  Widths that are too large may 
 *                   be prone to causing excess page faults, though the 
 *                   process cpu time reported by the clock() can be much
 *                   better for extremely large sizes.  If width < 1, an
 *                   error message is printed and NULL is returned.<br><br>
 *                   Widths that are too small will result in an excessive
 *                   number of pool expansions, which may severely limit
 *                   performance on some VM systems.  See deltapool below
 *                   about pool expansions.<br><br>
 *                   If you know something about the page size of your
 *                   architecture, fiddling with width may help you reduce 
 *                   your page fault or cache miss count in some uses.
 *
 *  @param eltsize   Element size maintained by the pool.
 *                   For maximum efficiency, eltsize should be an integer
 *                   multiple of sizeof(void *). If it is not, elts will be
 *                   padded so that this is the case. This is to avoid
 *                   pointer data misalignment.  This restriction may or
 *                   may not help avoid alignment problems with items inside
 *                   the user's element structure.
 *
 *  @param deltalen  Number of additional pointers in the pool that will be
 *                   allocated when more elements are needed than are
 *                   available internally.  deltalen must be at least 1 or
 *                   creation of the new pool will fail.
 *
 *  @param deltapool Size change of the pool array when expanded.  It should
 *                   be as large as you are willing to tolerate.  The pool 
 *                   array starts out completely filled (all pointers allocated). 
 *                   When the pool needs more pointers it gets them in chunks of 
 *                   at least deltapool.  These additional pointers will not 
 *                   automatically have elements allocated to them; rather,
 *                   they will be initialized to NULL and filled in only as the 
 *                   chunks of deltalen*width elements are required.
 *
 *  @return A pointer to the newly created pool store, NULL if an error occurred.
 */

extern void *mem_get_element(mem_store_t ms);
/**<
 *  Get a usable element from the pool.
 *  Returns a void pointer to a blob of memory of the eltsize
 *  set when ms was created.  You must cast it appropriately.
 *  The blob data is not initialized in any particular way.
 *
 *  @param ms The pool store from which to retrieve an element.
 *            If ms is NULL, then an error message is printed
 *            and NULL is returned.
 *  @return A pointer to the usable element, or NULL iff ms is NULL or
 *          store growth is required and the operating system is unable 
 *          to allocate the required memory.
 */

extern void mem_get_element_list(mem_store_t ms, int len, void **ellist);
/**<
 *  NOT IMPLEMENTED.
 *
 *  Takes the pointer array, ellist, of length len provided by the user
 *  and fills it with pointers to elements from the store.
 *  There is not necessarily any relation (memory map wise) between the
 *  locations pointed to by successive entries in the ellist returned.
 *  Ellist should point to an array with enough space for len pointers.
 *  Returns NULL in ellist[0] iff store growth is required and the operating
 *  system is unable to allocate the required memory.<br><br>
 *
 *  The user is reminded that if he knows how many elements he needs
 *  ahead of time, he is probably better off mallocing the array himself.
 *
 *  @todo Implement utilities/mem.c:mem_get_element_list() of remove
 *        it from pool.h.
 */

#define mem_DEBUG FALSE
/**<
 *  Flag controlling extra checking of the pool management routines.
 *  Setting mem_DEBUG to TRUE causes the mem_store routines to do
 *  some RATHER expensive checking. It should be set to FALSE.
 */
#define mem_LIGHTENING FALSE
/**<
 *  Flag controlling extent of internal sanity checking.
 *  Setting mem_LIGHTENING to TRUE causes mem_store routines to assume
 *  the user is perfect: i.e. no sanity checks are at all necessary and most
 *  internal accounting can be disabled.  No one with an ounce of sanity
 *  would ever set this flag to TRUE unless the code using the
 *  mem module was proven bug free. It makes the allocator smaller
 *  and faster, though, by ~15%.  <br><br>
 *
 *  This flag exists to make it easy to test the theory that the
 *  accounting overhead in this code is not of significant cost.
 *  Below 1e5 elements it really isn't bad enough to justify the
 *  assumption that the user is perfect.
 */

extern void mem_free_element(mem_store_t ms, void *eltpointer);
/**<
 *  Releases an element back to the store.
 *  If you return the same pointer twice, we will have
 *  no qualms about returning it to you twice. We won't necessarily
 *  return it to you twice, though.<br><br>
 *
 *  If mem_DEBUG is TRUE, eltpointer will be checked for belonging
 *  to ms. If you call mem_free_element() with a pointer the ms does
 *  not recognize, it will not be freed and a message will be
 *  sent to ASCERR.<br><br>
 *
 *  If mem_DEBUG is FALSE, eltpointer will be assumed to belong
 *  with the ms in question.  The implications of handing
 *  mem_free_element() an element of the wrong size or from the
 *  wrong ms (bearing in mind the LIFO reuse of elements) should be
 *  obvious. If they are not, stop using these routines.<br><br>
 *
 *  If at any time the number of elements freed exceeds the number
 *  handed out, we will whine (unless mem_LIGHTENING).  If ms is
 *  NULL, and error message is printed and the function returns.
 *  If eltpointer is NULL, we will ignore it completely.
 *
 *  @param ms          The memory store to modify.
 *  @param eltpointer  The element to return to the pool.
 *  @return No return value.
 */

extern void mem_clear_store(mem_store_t ms);
/**<
 *  Clears the books in ms. That is, we reset the ms to think
 *  that __all__ elements are freshly available and have never
 *  been handed out.  If ms is NULL, an error message is printed
 *  and the function returns.<br><br>
 *
 *  If mem_DEBUG is TRUE, it first verifies that all elements have
 *  been mem_freed first and whines if not.
 *  Get and free calls will be balanced to see if spurious elements
 *  have been handed in. (This is a heuristic check).
 *  The clear process will cause any spurious pointers that were
 *  turned in via mem_free_element() to be forgotten about.<br><br>
 *
 *  Clearing a store is not necessary for mem_destroy_store().
 *  Recycling is faster from the recycle list than from a cleared store, ~2%.
 *  Clear is provided for users who want to obtain elements with a higher
 *  probability of successive elements being near each other.
 *
 *  @param ms The memory store to clear.
 *  @return No return value.
 */

extern void mem_destroy_store(mem_store_t ms);
/**<
 *  Deallocates everything associated with the ms.
 *  If mem_DEBUG is TRUE, it first verifies that all elements
 *  have been mem_freed first and whines if not.
 *  If pmem_DEBUG is FALSE, just nukes everything unconditionally.
 *  If ms is NULL, an error message is printed and the function
 *  returns.
 *
 *  @paramms The memory store to destroy.
 */

extern void mem_print_store(FILE *fp, mem_store_t ms, unsigned detail);
/**<
 *  Prints statistics about a mem_store_t to the file stream given.
 *  Which stats get printed depends on detail.
 *  - If detail 0, displays just summary statistics.
 *  - If detail 1, just internal statistics.
 *  - If detail >1, displays both.
 *
 *  @param fp     The open file stream on which to print the report.
 *  @param ms     The memory store on which to report.
 *  @param detail The level of detail to print:
 *                0 = summary, 1 = internal stats, >1 = both.
 */

extern size_t mem_sizeof_store(mem_store_t ms);
/**<
 *  Retrieves the current total byte usage of the store.
 *  Returns 0 if an invalid pool store is specified.
 *
 *  @param ms The memory store to query.
 *  @return The total bytes currently used by the memory store.
 */

#endif  /* mem_NULL */

