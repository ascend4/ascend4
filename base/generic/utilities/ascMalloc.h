 /*
 *  Ascend Memory Allocation Routines
 *  by Tom Epperly
 *  Created: 2/6/90
 *  Version: $Revision: 1.2 $
 *  Version control file: $RCSfile: ascMalloc.h,v $
 *  Date last modified: $Date: 1997/07/18 11:44:50 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
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
 *
 */

#ifndef __ASCMALLOC_H_SEEN__
#define __ASCMALLOC_H_SEEN__

/** @file
 *  Ascend Memory Allocation Routines.
 *  <pre>
 *  The following macros affect the compilation and run-time behavior
 *  of Ascend memory management.
 *
 *    MOD_ASCMALLOC     Forces the memory allocation functions to always
 *                      allocate at least 1 byte and return a non-NULL
 *                      pointer.  This macro should be defined if your
 *                      allocator returns NULL when a zero-sized block is
 *                      requested.  It need not be defined if your
 *                      allocator returns a non-NULL pointer to a zero-
 *                      length block, saving you a few bytes of memory.
 *
 *    MALLOC_DEBUG      Causes all calls to memory allocation routines
 *                      to be tracked and logged to an output file.
 *                      This really slows down the memory management
 *                      system, and so should be used for debugging only.
 *
 *    ALLOCATED_TESTS   Forces a lot of extra assertions when defined
 *                      along with MALLOC_DEBUG.
 *
 *    MOD_REALLOC       If defined, ascreallocPURE() uses a homegrown
 *                      realloc() that behaves properly with purify.
 *                      This is more expensive, and should be used
 *                      for debugging only.
 *
 *  Requires:
 *         #include "utilities/ascConfig.h"
 *  </pre>
 */

extern char *ascstrdup(char *s);
/**< as strdup. */

extern char *asc_memcpy(char *s1, char *s2, unsigned int n);
/**<
 *  <!--  char *memcpy(s1, s2, n)                                      -->
 *  <!--  char *s1, *s2;                                               -->
 *  <!--  int n;                                                       -->
 *  asc_memcpy() copies n characters from memory area s2 to s1.   It
 *  returns s1. This version of memcpy handles overlapping memory
 *  ranges ok. It could be more efficient internally. As it is, it
 *  moves data a char at a time.
 */

extern char *ascreallocPUREF(char *ptr, size_t oldbytes, size_t newbytes);
/**<
 *  <!--  char *ascreallocPUREF(ptr,oldbytes,newbytes)                 -->
 *  <!--  char *ptr;                                                   -->
 *  <!--  size_t oldbytes,newbytes;                                    -->
 *  Behaves as realloc, except you must tell us what the old size was
 *  and the OS realloc doesn't get used.
 *  For debugging only.
 */

extern void ascstatus(CONST char *msg);
/**<
 *  <!--  void ascstatus(msg)                                          -->
 *  <!--  const char *msg;                                             -->
 *  If MALLOC_DEBUG is not defined, this does nothing.
 *  Otherwise, it prints the message followed by a newline to the log and
 *  to the screen.  It then prints various statistics about virtual memory
 *  usuage to the screen and to the log.
 */

extern void ascshutdown(CONST char *msg);
/**<
 *  <!--  void ascshutdown(CONST char *)                               -->
 *  <!--  const char *msg;                                             -->
 *  If MALLOC_DEBUG is not defined, this does nothing.
 *  Otherwise, it does what ascstatus does plus, it lists blocks of memory
 *  that are still allocated.
 */

extern unsigned long ascmeminuse(void);
/**< 
 *  The memory total reporting function is ascmeminuse().
 *  The following macros have the listed effects if they are defined at
 *  C compile time.
 *
 *  - MOD_ASCMALLOC     patches around a bug in OSF, AIX allocators.
 *  - MALLOC_DEBUG      runs a tracking file of all ascmalloc functions.
 *  - ALLOCATED_TESTS   forces extra assertions when added with MALLOC_DEBUG.
 *
 *  If MALLOC_DEBUG is undefined, we will report 0 memory allocated.
 */

/*
 *  all the bcopy/bzero functions have been replaced with their mem
 *  counterparts for ansi compliance
 */

/**
 * The following define is for debugging purposes only.
 * In some OS, realloc fools purify into thinking there
 * is a memory leak.
 * If MOD_REALLOC is defined at compile time for all
 * software referencing this header, then all occurences
 * of ascrealloc will use a homegrown realloc (which is
 * somewhat more expensive than most system supplied ones)
 * that does not fool purify.
 * Leaks of memory reported around realloc when MOD_REALLOC
 * is defined should be real leaks and not OS noise.
 * MALLOC_DEBUG overrides MOD_REALLOC, but we never use
 * MALLOC_DEBUG and purify together.
 * The homegrown realloc needs more information (the oldsize).
 * ascreallocPURE should not be used on a regular basis.
 */
#ifndef MOD_REALLOC
#define ascreallocPURE(p,nold,nnew) ascrealloc((p),(nnew))
#else
#define ascreallocPURE(p,nold,nnew) ascreallocPUREF((p),(nold),(nnew))
#endif

/*
 * The next line switches between the normal ascmalloc.h and the
 * alpha and rs6000 ascmalloc declarations.
 */
#ifndef MOD_ASCMALLOC
/* here's the normal version: */
#ifdef MALLOC_DEBUG
#define asccalloc(p,q) \
  asccallocf(p,q,__FILE__,__LINE__)
extern VOIDPTR asccallocf(unsigned nelem, unsigned elsize,
                          CONST char *file, int line);

#define ascmalloc(p) \
  ascmallocf(p,__FILE__,__LINE__)
extern VOIDPTR ascmallocf(unsigned size, CONST char * file, int line);

#define ascrealloc(p,q) \
  ascreallocf(p,q,__FILE__,__LINE__)
extern VOIDPTR ascreallocf(VOIDPTR ptr, unsigned size,
                           CONST char *file, int line);

#define ascfree(p) \
  ascfreef(p,__FILE__,__LINE__)
extern void ascfreef(VOIDPTR ptr, CONST char *file, int line);

#define ascbcopy(p,q,r) \
  ascbcopyf(p,q,r,__FILE__,__LINE__)
extern void ascbcopyf(CONST VOIDPTR b1, VOIDPTR b2, int size,
                      CONST char *file, int line);

#define ascbzero(p,q) \
  ascbzerof(p,q,__FILE__,__LINE__)
extern void ascbzerof(VOIDPTR b1, int length, CONST char *file, int line);

#define ascbfill(p,q) memset((char *)p,255,q)

extern int AllocatedMemory(CONST VOIDPTR ptr, unsigned size);
/**<
 *  Evaluate a memory block for allocation status.
 *  Return values:
 *    -  0       no memory is used
 *    -  1       the memory block is wholly contained in an allocated block
 *    -  2       the memory block equals a element of the memory list
 *    - -1       the memory block is partially contained in an allocated block
 */

extern int InMemoryBlock(CONST VOIDPTR ptr1, CONST VOIDPTR ptr2);
/**<
 *  <!--  extern int InMemoryBlock(ptr1,ptr2)                          -->
 *  Return true if ptr2 is in the memory block headed by ptr1, otherwise
 *  return false.
 */

#ifdef ALLOCATED_TESTS
#define AssertAllocatedMemory(p,q) \
  assert(AllocatedMemory((VOIDPTR)(p),(unsigned)(q))==2)

#define AssertMemory(p) \
  assert(AllocatedMemory((VOIDPTR)(p),0))

#define AssertContainedMemory(p,q) \
  assert(AllocatedMemory((VOIDPTR)(p),(unsigned)(q))>0)

#define AssertContainedIn(p,q) \
  assert(InMemoryBlock((VOIDPTR)(p),(VOIDPTR)(q)))
#else /* ALLOCATED_TESTS */
#define AssertAllocatedMemory(p,q)

#define AssertMemory(p)

#define AssertContainedMemory(p,q)

#define AssertContainedIn(p,q)
#endif /* ALLOCATED_TESTS */
#else /* MALLOC_DEBUG */

#define ascmalloc(x) malloc(x)

#if (defined(sun) && !defined(__SVR4))
#define ascrealloc(x,y) ((x)!=NULL ? realloc(x,y) : malloc(y))
#else
#define ascrealloc(x,y) realloc(x,y)
#endif

#define ascfree(x) free(x)

#define asccalloc(p,q) calloc(p,q)

#define ascbcopy(p,q,r) memcpy((void *)q,(void *)p,r)

#define ascbzero(p,q) memset((char *)p,0,q)

#define ascbfill(p,q) memset((char *)p,255,q)

#define AllocatedMemory(p,q) (1)

#define InMemoryBlock(p,q) (1)

#define AssertAllocatedMemory(p,q)

#define AssertMemory(p)

#define AssertContainedMemory(p,q)

#define AssertContainedIn(p,q)
#endif /* MALLOC_DEBUG */

#else /* MOD_ASCMALLOC */
/*  here starts the modified version of ascmalloc headers */
#ifndef MAX
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#endif

#ifdef MALLOC_DEBUG
#define asccalloc(p,q) \
  asccallocf(MAX(p,1),q,__FILE__,__LINE__)
extern VOIDPTR asccallocf(unsigned nelem, unsigned elsize,
                          CONST char *file, int line);

#define ascmalloc(p) \
  ascmallocf(MAX(p,1),__FILE__,__LINE__)
extern VOIDPTR ascmallocf(unsigned size, CONST char * file, int line);

#define ascrealloc(p,q) \
  ascreallocf(p,MAX(q,1),__FILE__,__LINE__)
extern VOIDPTR ascreallocf(VOIDPTR ptr, unsigned size,
                           CONST char *file, int line);

#define ascfree(p) \
  ascfreef(p,__FILE__,__LINE__)
extern void ascfreef(VOIDPTR ptr, CONST char *file, int line);

#define ascbcopy(p,q,r) \
  ascbcopyf(p,q,r,__FILE__,__LINE__)
extern void ascbcopyf(CONST VOIDPTR b1, VOIDPTR b2, int size,
                      CONST char *file, int line);

#define ascbzero(p,q) \
  ascbzerof(p,q,__FILE__,__LINE__)
extern void ascbzerof(VOIDPTR b1, int length, CONST char *file, int line);

#define ascbfill(p,q) memset((char *)p,255,q)

extern int AllocatedMemory(CONST VOIDPTR ptr, unsigned size);
/**<
 *  Evaluate a memory block for allocation status.
 *  Return values:
 *    -  0       no memory is used
 *    -  1       the memory block is wholly contained in an allocated block
 *    -  2       the memory block equals a element of the memory list
 *    - -1       the memory block is partially contained in an allocated block
 */

extern int InMemoryBlock(CONST VOIDPTR ptr1, CONST VOIDPTR ptr2);
/**<
 *  <!--  extern int InMemoryBlock(ptr1,ptr2)                          -->
 *  Return true if ptr2 is in the memory block headed by ptr1, otherwise
 *  return false.
 */

#ifdef ALLOCATED_TESTS
#define AssertAllocatedMemory(p,q) \
  assert(AllocatedMemory((VOIDPTR)(p),(unsigned)(MAX(q,1)))==2)

#define AssertMemory(p) \
  assert(AllocatedMemory((VOIDPTR)(p),0))

#define AssertContainedMemory(p,q) \
  assert(AllocatedMemory((VOIDPTR)(p),(unsigned)(q))>0)

#define AssertContainedIn(p,q) \
  assert(InMemoryBlock((VOIDPTR)(p),(VOIDPTR)(q)))
#else /* ALLOCATED_TESTS */
#define AssertAllocatedMemory(p,q)

#define AssertMemory(p)

#define AssertContainedMemory(p,q)

#define AssertContainedIn(p,q)
#endif /* ALLOCATED_TESTS */
#else /* MALLOC_DEBUG */

#define ascmalloc(x) malloc(MAX(x,1))

#define ascrealloc(x,y) realloc(x,MAX(y,1))

#define ascfree(x) free(x)

#define asccalloc(p,q) calloc(MAX(p,1),q)

#define ascbcopy(p,q,r) memcpy((void *)q,(void *)p,r)

#define ascbzero(p,q) memset((char *)p,0,q)

#define ascbfill(p,q) memset((char *)p,255,q)

#define AllocatedMemory(p,q) (1)

#define InMemoryBlock(p,q) (1)

#define AssertAllocatedMemory(p,q)

#define AssertMemory(p)

#define AssertContainedMemory(p,q)

#define AssertContainedIn(p,q)

#endif /* MALLOC_DEBUG */
#endif /* MOD_ASCMALLOC */
#endif /* __ASCMALLOC_H_SEEN__ */

