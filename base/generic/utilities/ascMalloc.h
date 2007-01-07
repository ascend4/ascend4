/*	ASCEND modelling environment
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
*//** @file
	ASCEND memory allocation & reporting routines.

	@TODO
		An attempt is being made here to remove some of the complexity here
		and migrate to the external tool 'dmalloc' (http://dmalloc.com/)

	These functions provide tracking of memory events and assist
	finding and debugging memory errors.  Memory tracking options are
	selected using the macros MALLOC_DEBUG and ALLOCATED_TESTS discussed
	below.  This allows the enhanced functionality to be used or turned
	off as desired.  This functionality adds considerable run-time overhead,
	and so should generally be used for debugging purposes only.  There are
	also routines for reporting on the status of memory blocks as well as
	for the memory manager itself.<br><br>

	To use the memory tracking features, you need to use the provided
	allocation and deallocation functions (ascmalloc(), asccalloc(),
	ascrealloc(), ascfree()).  Direct calls to the corresponding system
	routines will by-pass the memory manager and not be tracked.  Memory
	allocations which return NULL are not tracked or recorded.<br><br>

	This module also standardizes critical implementation-specific features
	such as behavior when an allocation of zero bytes is requested
	(see MOD_ASCMALLOC), and the behavior of realloc() under purify
	(see MOD_REALLOC).<br><br>

	The following macros affect the compilation and run-time behavior
	of Ascend memory management.

	<pre>
	  MOD_ASCMALLOC     Forces the memory allocation functions to always
						allocate at least 1 byte and return a non-NULL
						pointer.  This macro should be defined if your
						allocator returns NULL when a zero-sized block is
						requested (e.g. alpha or rs6000; Borland and Watcom
						on Windows).  It need not be defined if your
						allocator returns a non-NULL pointer to a zero-length
						block, saving you a few bytes of memory.

	  MALLOC_DEBUG      Causes all calls to memory allocation routines
						to be tracked and logged to an output file.
						This really slows down the memory management
						system, and so should be used for debugging only.

	  ALLOCATED_TESTS   Forces a lot of extra assertions when defined
						along with MALLOC_DEBUG.

	  MOD_REALLOC       If defined, ascreallocPURE() uses a homegrown
						realloc() that behaves properly with purify.
						This is more expensive, and should be used
						for debugging only.

	Requires:
		   #include "utilities/ascConfig.h"
		   #include "utilities/ascPanic.h"
	</pre>
*//*
	by Tom Epperly
	Created: 2/6/90
	Last in CVS: $Revision: 1.2 $ $Date: 1997/07/18 11:44:50 $ $Author: mthomas $
*/

#ifndef ASC_ASCMALLOC_H
#define ASC_ASCMALLOC_H

/* MALLOC_DEBUG and ASC_WITH_DMALLOC will be defined in config.h... */
#include <utilities/config.h>
#include <utilities/ascConfig.h>

/*------------------------------------------------------------------------------
	C++-ish ASC_NEW* macros
*/
/**
	Shorthand for creating pointers to newly allocated data of a given type
*/
#define ASC_NEW(TYPE) (TYPE*)ascmalloc(sizeof(TYPE))
#define ASC_NEW_CLEAR(TYPE) (TYPE*)asccalloc(1,sizeof(TYPE))

/**
	Shorthand for creating pointer to an array of newly allocated data of a
	given type. 'ascmalloc' is used for the allocation.
*/
#define ASC_NEW_ARRAY(TYPE,COUNT) (TYPE*)ascmalloc(sizeof(TYPE)*(COUNT))

/**
	Shorthand for creating a pointer to allocated data, using asccalloc (to zero
	the allocated space).
*/
#define ASC_NEW_ARRAY_CLEAR(TYPE,COUNT) (TYPE*)asccalloc((COUNT),sizeof(TYPE))

/**
	Shorthand for creating a pointer to an array of allocated data. If the
	specified length is zero, no space is allocated, and a NULL is returned
	instead.
*/
#define ASC_NEW_ARRAY_OR_NULL(TYPE,COUNT) \
	((COUNT)>0 ? ASC_NEW_ARRAY(TYPE,COUNT) : NULL)

#define ASC_NEW_ARRAY_OR_NULL_CLEAR(TYPE,COUNT) \
	((COUNT)>0 ? ASC_NEW_ARRAY_CLEAR(TYPE,COUNT) : NULL)

/* regular expressions for fixing up mallocs (in regexxer)
	\(([^\*\)]+) ?\*\)\s*ascmalloc\(sizeof\(\1\)\s*\*\s*([a-zA-Z][a-zA-Z_0-9]*)\)
\(([^\*\)]+) ?\*\)\s*asccalloc\(([a-zA-Z][a-zA-Z_0-9]*)\s*,\s*sizeof\(\1\)\s*\)
*/

#define ASC_FREE(PTR) ascfree(PTR)

ASC_DLLSPEC char *ascstrdupf(CONST char *str);
/**<
 *  Implementation function for ascstrdup() if MALLOC_DEBUG
 *  is not defined.  Do not call this function directly - use
 *  ascstrdup() instead.
 */

/*------------------------------------------------------------------------------
	MACROS FOR THE CASE WHERE 'DMALLOC' IS AVAILABLE
*/

#ifdef ASC_WITH_DMALLOC

#include <malloc.h>
#include <dmalloc.h>

#define ascstrdup(str) ascstrdupf(str)
#define ascmalloc(SIZE) malloc(SIZE)
#define asccalloc(COUNT,SIZE) calloc((COUNT),(SIZE))
#define ascfree(ADDR) free(ADDR)
#define asc_memcpy(DEST,SRC,SIZE) memcpy((DEST),(SRC),(SIZE))
#define ascstatus(MSG) /* empty */
#define ascstatus_detail(msg) /* empty */
#define ascmeminuse() (0)
#define ascshutdown(ARG) /* empty */
#define ascrealloc(PTR,SIZE) realloc((PTR),(SIZE))
#define ascreallocPURE(PTR,OLD,NEWP) ascrealloc((PTR),(NEWP))
#define ascbcopy(SRC,DEST,SIZE) memcpy((void *)(DEST), (void *)(SRC), (SIZE))
#define ascbzero(DEST,LENGTH) memset((char *)(DEST), 0, (LENGTH))
#define ascbfill(DEST,LENGTH) memset((char *)(DEST), 255, (LENGTH))

/* some assertions, all ignored in this case */
#define AllocatedMemory(ptr,size) (1)
#define InMemoryBlock(ptr1,ptr2) (1)
#define AssertAllocatedMemory(ptr,size)
#define AssertMemory(ptr)
#define AssertContainedMemory(ptr,size)
#define AssertContainedIn(ptr,ptr2)

/*------------------------------------------------------------------------------
	ORIGINAL ASCEND 'MALLOC' IMPLEMENTATION
*/
#else /* ASC_WITH_DMALLOC */

#ifdef MALLOC_DEBUG
#  define ascstrdup(str) ascstrdupf_dbg(str)
#else
#  define ascstrdup(str) ascstrdupf(str)
#endif
/**<
 *  Returns a new copy of string str.
 *  This is the ASCEND rendition of strdup().  The caller is
 *  responsible for deallocating the new string when finished
 *  using ascfree().  Returns NULL if str is NULL or memory
 *  cannot be allocated for the new copy.  If MALLOC_DEBUG is
 *  defined, the allocation is tracked.
 *
 *  @param str The 0-terminated string to copy.
 *  @return A new copy of str as a char *, or NULL if an error occurs.
 */

ASC_DLLSPEC char *ascstrdupf_dbg(CONST char *str);
/**<
 *  Implementation function for ascstrdup() if MALLOC_DEBUG
 *  is defined.  Do not call this function directly - define
 *  MALLOC_DEBUG and use ascstrdup() instead.
 */

ASC_DLLSPEC char *asc_memcpy(char *dest, char *src, size_t n);
/**<
 *  Copies n bytes from memory address src to dest.
 *  This version of memcpy handles overlapping memory ranges
 *  properly. It could be more efficient internally. As it is,
 *  it moves data a char at a time.  Unless n is 0, neither dest
 *  nor src may be NULL (checked by asc_assertion).
 *
 *  @param dest Pointer to address to which to copy (non-NULL).
 *  @param src  Pointer to address from which to copy (non-NULL).
 *  @param n    Number of bytes to copy.
 *  @return Returns dest.
 */

#ifndef MOD_REALLOC
#  define ascreallocPURE(ptr,nold,nnew) ascrealloc((ptr),(nnew))
#else
#  ifdef MALLOC_DEBUG
#    define ascreallocPURE(ptr,nold,nnew) ascreallocPUREF_dbg((ptr),(nold),(nnew))
#  else
#    define ascreallocPURE(ptr,nold,nnew) ascreallocPUREF((ptr),(nold),(nnew))
#  endif
#endif
/**<
 *  purify realloc() function for debugging purposes.
 *  In some OS, realloc() fools purify into thinking there
 *  is a memory leak.  If MOD_REALLOC is defined at compile
 *  time for all software referencing this header, then all
 *  calls to ascreallocPURE() will use a homegrown realloc
 *  that does not fool purify.  Leaks of memory reported
 *  around realloc() when MOD_REALLOC is defined should be
 *  real leaks and not OS noise.<br><br>
 *
 *  The custom function is somewhat more expensive than most
 *  system-supplied realloc()'s, so should only be used for
 *  debugging.  Note that ascreallocPURE() will provide memory
 *  event tracking if MALLOC_DEBUG is also defined when this
 *  header is included.
 *
 *  @see ascreallocPUREF()
 *  @param ptr  Pointer to the memory block to reallocate.
 *  @param nold Old block size in bytes.
 *  @param nnew New block size in bytes.
 *  @return A pointer to the new memory block, or NULL if an error occurred.
 */

extern char *ascreallocPUREF(char *ptr, size_t oldbytes, size_t newbytes);
/**<
 *  Implementation function for release version of ascreallocPURE().
 *  Do not call this function directly - use ascreallocPURE()
 *  (by #defining MOD_REALLOC) instead.  This version does not have
 *  its memory tracked.  This is a custom realloc() which behaves
 *  properly with purify.  It bypasses the standard realloc() function.
 *  The caller must indicate the old size of the memory region.
 *
 *  @param ptr      Pointer to the memory block to reallocate.
 *  @param oldbytes Old block size in bytes.
 *  @param newbytes New block size in bytes.
 *  @return A pointer to the new memory block, or NULL if an error occurred.
 */

extern char *ascreallocPUREF_dbg(char *ptr, size_t oldbytes, size_t newbytes);
/**<
 *  Implementation function for debug version of ascreallocPURE().
 *  Do not call this function directly - use ascreallocPURE()
 *  (by #defining MOD_REALLOC and MALLOC_DEBUG) instead.  This version
 *  has it's allocations tracked by the memory manager.  This is
 *  a custom realloc() which behaves properly with purify.  It
 *  bypasses the standard realloc() function.  The caller must indicate
 *  the old size of the memory region.
 *
 *  @param ptr      Pointer to the memory block to reallocate.
 *  @param oldbytes Old block size in bytes.
 *  @param newbytes New block size in bytes.
 *  @return A pointer to the new memory block, or NULL if an error occurred.
 */

#ifdef MALLOC_DEBUG
#  define ascstatus(msg) ascstatusf(msg)
#else
#  define ascstatus(msg)
#endif
/**<
 *  Prints a summary of the memory manager status (iff MALLOC_DEBUG
 *  is defined)  The msg is first printed, followed by a newline and
 *  then the summary memory report.  These are printed both to the
 *  memory log file, as well as to the screen.  If MALLOC_DEBUG is
 *  not defined, then nothing is printed.
 *
 *  @see ascstatus_detail(), ascshutdown()
 *  @param msg The message to print before the summary report.
 */

extern void ascstatusf(CONST char *msg);
/**<
 *  Implementation function for ascstatus() if MALLOC_DEBUG
 *  is defined.  Do not call this function directly - define
 *  MALLOC_DEBUG and use ascstatus() instead.
 */

#ifdef MALLOC_DEBUG
#  define ascstatus_detail(msg) ascstatus_detailf(msg)
#else
#  define ascstatus_detail(msg)
#endif
/**<
 *  Prints a more detailed report of the memory manager status (iff
 *  MALLOC_DEBUG is defined)  The msg is first printed, followed by
 *  a newline and then the memory report.  This report includes both
 *  the summary information printed by ascstatus(), as well as a listing
 *  of memory blocks currently allocated.  These are printed both to the
 *  memory log file, as well as to the screen.  If MALLOC_DEBUG is
 *  not defined, then nothing is printed.
 *
 *  @see ascstatus(), ascshutdown()
 *  @param msg The message to print before the detail report.
 */

extern void ascstatus_detailf(CONST char *msg);
/**<
 *  Implementation function for ascstatus_detail() if MALLOC_DEBUG
 *  is defined.  Do not call this function directly - define
 *  MALLOC_DEBUG and use ascstatus_detail() instead.
 */

#ifdef MALLOC_DEBUG
#  define ascshutdown(msg) ascshutdownf(msg)
#else
#  define ascshutdown(msg)
#endif
/**<
 *  Prints a detailed report of the memory manager status and cleans
 *  up after memory logging (iff MALLOC_DEBUG is defined).  The
 *  reporting is the same as ascstatus_detail().  After this function
 *  is called, subsequent allocation/deallocation activity will be
 *  logged in a different log file than previously.  If MALLOC_DEBUG
 *  is not defined, then this function does nothing.
 *
 *  @see ascstatus(), ascstatus_detail()
 *  @param msg The message to print before the detail report.
 */

ASC_DLLSPEC void ascshutdownf(CONST char *msg);
/**<
 *  Implementation function for ascshutdown() if MALLOC_DEBUG
 *  is defined.  Do not call this function directly - define
 *  MALLOC_DEBUG and use ascshutdown() instead.
 */

#ifdef MALLOC_DEBUG
#  define ascmeminuse() ascmeminusef()
#else
#  define ascmeminuse() (0)
#endif

ASC_DLLSPEC unsigned long ascmeminusef(void);
/**<
 *  Returns the current total amount of allocated memory (iff
 *  MALLOC_DEBUG is #defined).  If MALLOC_DEBUG is not #defined,
 *  the return value is 0 regardless of the actual amount of
 *  allocated memory.
 */

#ifdef MOD_ASCMALLOC
#  define AT_LEAST_1(x) MAX((x),1)
#else
#  define AT_LEAST_1(x) (x)
#endif
/**<
 *  Macro to ensure at least 1 byte is requested when MOD_ASCMALLOC
 *  is defined.  Requires a define for MAX(), which is located in
 *  utilities/ascConfig.h.
 */

#ifdef MALLOC_DEBUG
#  define asccalloc(nelem, size) \
          asccallocf(AT_LEAST_1(nelem), (size), __FILE__, __LINE__)
#else
#  define asccalloc(nelem,size) calloc(AT_LEAST_1(nelem),(size))
#endif
/**<
 *  ASCEND calloc() function.
 *  If MALLOC_DEBUG is defined, the allocation is tracked.
 *  If not, then the standard system calloc() is used.
 *
 *  @see asccallocf()
 *  @param nelem size_t, number of elements to allocate.
 *  @param size size_t, size of each element.
 *  @return A (void *) to the newly-allocated block, or NULL on error.
 */

ASC_DLLSPEC VOIDPTR asccallocf(size_t nelem, size_t elsize,
                          CONST char *file, int line);
/**<
 *  Implementation function for asccalloc() when MALLOC_DEBUG
 *  is defined.  Do not call this function directly - use
 *  asccalloc() instead.
 */

#ifdef MALLOC_DEBUG
#  define ascmalloc(size) \
          ascmallocf(AT_LEAST_1(size), __FILE__, __LINE__)
#else
#  define ascmalloc(size) malloc(AT_LEAST_1(size))
#endif
/**<
 *  ASCEND malloc() function.
 *  If MALLOC_DEBUG is defined, the allocation is tracked.
 *  If not, then the standard system malloc() is used.
 *
 *  @see ascmallocf()
 *  @param size size_t, size of each element.
 *  @return A (void *) to the newly-allocated block, or NULL on error.
 */

ASC_DLLSPEC VOIDPTR ascmallocf(size_t size, CONST char * file, int line);
/**<
 *  Implementation function for ascmalloc() when MALLOC_DEBUG
 *  is defined.  Do not call this function directly - define
 *  MALLOC_DEBUG and use ascmalloc() instead.
 */

#ifdef MALLOC_DEBUG
#  define ascrealloc(ptr, size) \
          ascreallocf((ptr), AT_LEAST_1(size), __FILE__, __LINE__)
#else
#  if (defined(sun) && !defined(__SVR4))
#    define ascrealloc(ptr,size) \
            ((ptr)!=NULL ? realloc((ptr), AT_LEAST_1(size)) : malloc(AT_LEAST_1(size)))
#  else
#    define ascrealloc(ptr,size) realloc((ptr), AT_LEAST_1(size))
#  endif
#endif
/**<
 *  ASCEND realloc() function.
 *  If MALLOC_DEBUG is defined, the allocation is tracked.
 *  If not, then the standard system realloc() is used.
 *
 *  @see ascreallocf(), ascreallocPURE()
 *  @param ptr  Pointer to memory block to reallocate.
 *  @param size size_t, size of each element.
 *  @return A (void *) to the newly-allocated block, or NULL on error.
 */

ASC_DLLSPEC VOIDPTR ascreallocf(VOIDPTR ptr, size_t size,
                           CONST char *file, int line);
/**<
 *  Implementation function for ascrealloc() when MALLOC_DEBUG
 *  is defined.  Do not call this function directly - define
 *  MALLOC_DEBUG and use ascrealloc() instead.
 */

#ifdef MALLOC_DEBUG
#  define ascfree(ptr) ascfreef((ptr), __FILE__, __LINE__)
#else
#  define ascfree(ptr) free(ptr)
#endif
/**<
 *  ASCEND free() function.
 *  If MALLOC_DEBUG is defined, the deallocation is tracked.
 *  If not, then the standard system free() is used.
 *
 *  @see ascfreef()
 *  @param ptr Pointer to the memory block to free.
 */

ASC_DLLSPEC void ascfreef(VOIDPTR ptr, CONST char *file, int line);
/**<
 *  Implementation function for ascfree() when MALLOC_DEBUG
 *  is defined.  Do not call this function directly - define
 *  MALLOC_DEBUG and use ascfree() instead.
 */

#ifdef MALLOC_DEBUG
#  define ascbcopy(src,dest,size) \
          ascbcopyf((src), (dest), (size), __FILE__, __LINE__)
#else
#  define ascbcopy(src,dest,size) \
          memcpy((void *)(dest), (void *)(src), (size))
#endif
/**<
 *  ASCEND bcopy() function.
 *  Copies size bytes from src to dest.  If MALLOC_DEBUG is
 *  defined, this uses custom function ascbcopyf().  If not,
 *  then memcpy() is used.
 *
 *  @see asc_memcpy(), ascbcopyf(), ascbzero(), ascbfill()
 *  @param src  Pointer to memory block to copy.
 *  @param dest Pointer to memory block to receive copy.
 *  @param size size_t, number of bytes to copy.
 *  @return A (void *) to dest.
 */

ASC_DLLSPEC VOIDPTR ascbcopyf(CONST VOIDPTR src, VOIDPTR dest, size_t size,
                         CONST char *file, int line);
/**<
 *  Implementation function for ascbcopy() when MALLOC_DEBUG
 *  is defined.  Do not call this function directly - define
 *  MALLOC_DEBUG and use ascbcopy() instead.
 */

#ifdef MALLOC_DEBUG
#  define ascbzero(dest,length) \
          ascbzerof((dest), (length), __FILE__, __LINE__)
#else
#  define ascbzero(dest,length) memset((char *)(dest), 0, (length))
#endif
/**<
 *  ASCEND bzero() function.
 *  Sets length bytes of memory at dest to zero.  If MALLOC_DEBUG
 *  is defined, this uses custom function ascbzerof().  If not,
 *  then memset() is used.
 *
 *  @see ascbzerof(), ascbcopy(), ascbfill()
 *  @param dest   Pointer to memory block to be cleared.
 *  @param length size_t, number of bytes to set to zero.
 *  @return A (void *) to dest.
 */

ASC_DLLSPEC VOIDPTR ascbzerof(VOIDPTR dest, size_t length, CONST char *file, int line);
/**<
 *  Implementation function for ascbzero() when MALLOC_DEBUG
 *  is defined.  Do not call this function directly - define
 *  MALLOC_DEBUG and use ascbzero() instead.
 */

#ifdef MALLOC_DEBUG
#  define AllocatedMemory(ptr,size) AllocatedMemoryF((ptr), (size))
#else
#  define AllocatedMemory(ptr,size) (1)
#endif
/**<
 *  Evaluate a memory block for allocation status.
 *  Return values if MALLOC is defined:
 *    -  0       no memory in the block is currently allocated
 *    -  1       the memory block is wholly contained in an allocated block
 *    -  2       the memory block equals an element of the memory list
 *    - -1       the memory block is partially contained in an allocated block
 *  If MALLOC_DEBUG is not defined, always returns 1.
 *
 *  @param ptr  Pointer to memory block to look up
 *  @param size size_t, size of the memory block to look up.
 */

ASC_DLLSPEC int AllocatedMemoryF(CONST VOIDPTR ptr, size_t size);
/**<
 *  Implementation function for AllocatedMemory() when
 *  MALLOC_DEBUG is defined.  Do not call this function
 *  directly - define MALLOC_DEBUG and use
 *  AllocatedMemory() instead.
 */

#ifdef MALLOC_DEBUG
#  define InMemoryBlock(ptr1,ptr2) InMemoryBlockF((ptr1), (ptr2))
#else
#define InMemoryBlock(ptr1,ptr2) (1)
#endif
/**<
 *  Query whether one memory block is contained within another.
 *  Returns non-zero if ptr2 is in the memory block starting at
 *  address ptr1; otherwise returns 0.  If MALLOC_DEBUG is not
 *  defined, always returns 1.
 */

ASC_DLLSPEC int InMemoryBlockF(CONST VOIDPTR ptr1, CONST VOIDPTR ptr2);
/**<
 *  Implementation function for InMemoryBlock() when MALLOC_DEBUG
 *  is defined.  Do not call this function directly - define
 *  MALLOC_DEBUG and use InMemoryBlock() instead.
 */

#define ascbfill(dest,length) memset((char *)(dest), 255, (length))
/**<
 *  Sets length bytes of memory at dest to 255.  There is
 *  no difference in implementation whether MALLOC_DEBUG is
 *  defined or not.
 *
 *  @see ascbcopy(), ascbzero()
 *  @param dest   Pointer to memory block to be set.
 *  @param length size_t, number of bytes to set to 255.
 *  @return A (void *) to dest.
 */

#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
#  define AssertAllocatedMemory(ptr,size) \
          asc_assert(AllocatedMemoryF((VOIDPTR)(ptr), (size_t)(AT_LEAST_1(size)))==2)
#else
#  define AssertAllocatedMemory(ptr,size)
#endif
/**<
 *  Assertion that a memory block is allocated with specified size.
 *  This assertion is only active if both MALLOC_DEBUG
 *  and ALLOCATED_TESTS are defined.
 *
 *  @param ptr  Pointer to memory block to evaluate.
 *  @param size size_t, size of memory block to evaluate.
 */

#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
#  define AssertMemory(ptr) \
          asc_assert(AllocatedMemoryF((VOIDPTR)(ptr), 0))
#else
#  define AssertMemory(ptr)
#endif
/**<
 *  Assertion that ptr points to (or into) an allocated memory
 *  block.  This assertion is only active if both MALLOC_DEBUG
 *  and ALLOCATED_TESTS are defined.
 *
 *  @param ptr  Pointer to memory block to evaluate.
 */

#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
#  define AssertContainedMemory(ptr,size) \
          asc_assert(AllocatedMemoryF((VOIDPTR)(ptr),(size_t)(size))>0)
#else
#  define AssertContainedMemory(ptr,size)
#endif
/**<
 *  Assertion that a memory block is wholly or partially
 *  contained in an allocated block.
 *  This assertion is only active if both MALLOC_DEBUG
 *  and ALLOCATED_TESTS are defined.
 *
 *  @param ptr  Pointer to memory block to evaluate.
 *  @param size size_t, size of memory block to evaluate.
 */

#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
#  define AssertContainedIn(ptr1,ptr2) \
          asc_assert(InMemoryBlockF((VOIDPTR)(ptr1),(VOIDPTR)(ptr2)))
#else
#  define AssertContainedIn(ptr,ptr2)
#endif
/**<
 *  Assertion that memory block ptr2 is contained in the block
 *  starting at ptr1.  This assertion is only active if both
 *  MALLOC_DEBUG and ALLOCATED_TESTS are defined.
 */

#endif /* ASC_WITH_DMALLOC */

#endif /* ASC_ASCMALLOC_H */

