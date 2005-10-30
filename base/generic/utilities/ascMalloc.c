/*
 *  Ascend Memory Allocation Routines
 *  by Tom Epperly
 *  Created: 2/6/90
 *  Version: $Revision: 1.1 $
 *  Version control file: $RCSfile: ascMalloc.c,v $
 *  Date last modified: $Date: 1997/07/18 11:44:49 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2 of the
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

#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <malloc.h>
#include <stdlib.h>
#include "utilities/ascConfig.h"
#include "compiler/compiler.h"
#include "utilities/ascPanic.h"
#include "utilities/ascMalloc.h"

#define LOGFILE "memlg"
#define MAXPOINTERS 1000000

#ifndef lint
static CONST char AscendMemoryAllocRCSid[]="$Id: ascMalloc.c,v 1.1 1997/07/18 11:44:49 mthomas Exp $";
#endif

/*
 *  The "non-debug" version of ascstrdupf -
 *  all memory calls should be to the system versions.
 */
char *ascstrdupf(char *s)
{
	char *result;

	if (NULL == s) {
    return NULL;
  }
	result = (char *)malloc(strlen(s) + 1);
  if (NULL != result) {
    strcpy(result, s);
  }
	return result;
}


/*
 *  Here's the debug version of ascstrdup -
 *  all memory calls should be to the local debug versions.
 */
char *ascstrdupf_dbg(char *s)
{
	char *result;

	if (NULL == s) {
    return NULL;
  }
	result = (char *)ascmallocf(strlen(s) + 1, __FILE__, __LINE__);
  if (NULL != result) {
    strcpy(result, s);
  }
	return result;
}

char *asc_memcpy(char *to, char *from, size_t size)
{
/* We really should be moving stuff a long at a time, but that mean
 * handling the leading and trailing unaligned bytes separately.
 * We also really should just be passing on to the vendor memcpy on
 * those systems which do overlapping ranges right.
 */
  char *fill;
  asc_assert((NULL != to) && (NULL != from));
  if  (from < to) {
    fill = (to + size) - 1;
    from += (size - 1);
    while (size-- > 0) {
      *fill-- = *from--;
    }
  } else {
    if (from > to) {
      fill = to;
      while (size-- > 0) {
        *fill++ = *from++;
      }
    } /* else from == to, do nothing */
  }
  return to;
}

/*
 *  The "non-debug" version of ascreallocPURE -
 *  all memory calls should be to the system versions.
 */
char *ascreallocPUREF(char *ptr, size_t oldbytes, size_t newbytes)
{
  /* shrink */
  if (newbytes > 0  && newbytes <= oldbytes) return ptr;
  /* release */
  if (!newbytes) {
    free(ptr);
    return NULL;
  }
  /* create */
  if (!oldbytes) {
    if (ptr!=NULL) free(ptr); /* some OS allocate 0 bytes, god help us */
    return (char *)malloc(newbytes);
  } else {
    /* expand */
    char *ret;
    ret = malloc(newbytes);
    if (ret==NULL) return NULL;
    if (oldbytes%sizeof(long) == 0 && ((unsigned long)ptr)%sizeof(long) == 0) {
      register unsigned long c,len, *src, *trg;
      src = (unsigned long *)ptr;
      trg = (unsigned long *)ret;
      len = oldbytes/sizeof(long);
      for (c=0;c < len;c++) trg[c]=src[c]; /* copy data as longs */
    } else {
      memcpy(ret,ptr,oldbytes);
    }
    free(ptr);
    return ret;
  }
}

/*
 *  Here's the debug version of ascreallocPURE -
 *  all memory calls should be to the local debug versions.
 */
char *ascreallocPUREF_dbg(char *ptr, size_t oldbytes, size_t newbytes)
{
  /* shrink */
  if (newbytes > 0  && newbytes <= oldbytes) return ptr;
  /* release */
  if (!newbytes) {
    ascfreef(ptr, __FILE__, __LINE__);
    return NULL;
  }
  /* create */
  if (!oldbytes) {
    if (ptr!=NULL) ascfreef(ptr, __FILE__, __LINE__); /* some OS allocate 0 bytes, god help us */
    return (char *)ascmallocf(newbytes, __FILE__, __LINE__);
  } else {
    /* expand */
    char *ret;
    ret = ascmallocf(newbytes, __FILE__, __LINE__);
    if (ret==NULL) return NULL;
    if (oldbytes%sizeof(long) == 0 && ((unsigned long)ptr)%sizeof(long) == 0) {
      register unsigned long c,len, *src, *trg;
      src = (unsigned long *)ptr;
      trg = (unsigned long *)ret;
      len = oldbytes/sizeof(long);
      for (c=0;c < len;c++) trg[c]=src[c]; /* copy data as longs */
    } else {
      memcpy(ret,ptr,oldbytes);
    }
    ascfreef(ptr, __FILE__, __LINE__);
    return ret;
  }
}

struct memory_rec{
  CONST VOIDPTR ptr;
  size_t size;
};

FILE *g_memory_log_file = NULL;
int g_memory_length = 0;
unsigned long g_memory_allocated = 0L;
unsigned long g_peak_memory_usage = 0L;
struct memory_rec g_mem_rec[MAXPOINTERS];
char *g_memlog_filename = NULL;

unsigned long ascmeminusef(void)
{
  return g_memory_allocated;
}

#ifdef THIS_IS_AN_UNUSED_FUNCTION
/*@unused@*/
static int SortedList(void)
{
  int c;
  for(c=1;c<g_memory_length;c++)
    if (g_mem_rec[c-1].ptr>g_mem_rec[c].ptr) return 0;
  return 1;
}
#endif  /* THIS_IS_AN_UNUSED_FUNCTION */

static CONST VOIDPTR MinMemory(void)
{
  int c;
  CONST VOIDPTR minm=(VOIDPTR)ULONG_MAX;
  for(c=0;c<g_memory_length;c++)
    if (g_mem_rec[c].ptr < minm) minm = g_mem_rec[c].ptr;
  return minm;
}

static CONST VOIDPTR MaxMemory(void)
{
  int c;
  CONST VOIDPTR maxm=0;
  for(c=0;c<g_memory_length;c++)
    if (((CONST char *)g_mem_rec[c].ptr + g_mem_rec[c].size - 1)
                > (CONST char *)maxm)
      maxm = (CONST VOIDPTR)((CONST char *)g_mem_rec[c].ptr + g_mem_rec[c].size - 1);
  return maxm;
}

#define NONZERO(x) ((0 != (x)) ? (x) : 1)

static CONST VOIDPTR MemoryMean(void)
{
  int c;
  size_t size;
  double sum=0.0, bytes=0.0;
  for(c=0 ; c<g_memory_length ; c++){
    size = g_mem_rec[c].size;
    if (0 != size){
      bytes += (double)size;
      sum += (double)((size * (size-1))/2)
          + ((double)size) * ((double)(unsigned long)g_mem_rec[c].ptr);
    }
  }
  return (VOIDPTR)(unsigned long)(sum/NONZERO(bytes));
}

/*
 *  Creates a file for logging memory events.
 *  The file is opened and header information is written to it.
 *  It is then closed.  Thereafter, the file name is available to
 *  other routines in g_memlog_filename, which should append to the
 *  file as needed.  This function may be called more than once -
 *  the header info will only be written the first time it is called,
 *  or after ascshutdown() has been called.
 */
static void OpenLogFile(void)
{
  if (NULL == g_memlog_filename){
    time_t t;
    g_memlog_filename = tempnam(NULL,LOGFILE);
    if (g_memlog_filename &&
       (NULL == (g_memory_log_file = fopen(g_memlog_filename,"w")))) {
      Asc_Panic(2, NULL, "Unable to open memory log file.\n");
    }
    t = time((time_t *)NULL);
    FPRINTF(g_memory_log_file,"Ascend memory log file opened %s",
                              asctime(localtime(&t)));
    FPRINTF(g_memory_log_file,"%16s %13s %16s %13s %6s %s",
                              "Alloc Range",
                              "Size",
                              "Dealloc Range",
                              "Size",
                              "Line#",
                              "Source File\n");
    fclose(g_memory_log_file);
    g_memory_log_file = NULL;
  }
}

static void WriteMemoryStatus(FILE *f, CONST char *msg)
{
  CONST VOIDPTR minm;
  CONST VOIDPTR maxm;
  minm = MinMemory();
  maxm = MaxMemory();
  FPRINTF(f,"%s\n"
            "Current memory usage(byte allocated): %lu\n"
            "Current number of blocks: %d\n"
            "Peak memory usage(bytes allocated): %lu\n"
            "Lowest address: %lx\n"
            "Highest address: %lx\n"
            "Memory density: %g\n"
            "Memory mean: %lx\n",
            msg,
            g_memory_allocated,
            g_memory_length,
            g_peak_memory_usage,
            (unsigned long)minm,
            (unsigned long)maxm,
            ((double)g_memory_allocated/(double)
            NONZERO((CONST char *)maxm - (CONST char *)minm)),
            (unsigned long)MemoryMean());
}

static void WriteMemoryRecords(FILE *f, CONST char *msg)
{
  int c;
  FPRINTF(f,"%s\nAllocation record count:  %d\n", msg, g_memory_length);
  for (c=0 ; c<g_memory_length ; ++c) {
    FPRINTF(f,"%5d %9x->%9x %9u\n",
              c,
              g_mem_rec[c].ptr,
              (CONST VOIDPTR)((CONST char *)g_mem_rec[c].ptr + g_mem_rec[c].size),
              g_mem_rec[c].size);
  }
}

void ascstatusf(CONST char *msg)
{
  OpenLogFile();
  if (NULL != (g_memory_log_file = fopen(g_memlog_filename,"a"))){
    WriteMemoryStatus(g_memory_log_file, msg);
    fclose(g_memory_log_file);
    g_memory_log_file = NULL;
  }
  else
    FPRINTF(stderr,"Can't open memory log file.\n");
  WriteMemoryStatus(stdout, msg);
}

void ascstatus_detailf(CONST char *msg)
{
  OpenLogFile();
  if (NULL != (g_memory_log_file = fopen(g_memlog_filename,"a"))){
    WriteMemoryStatus(g_memory_log_file, msg);
    WriteMemoryRecords(g_memory_log_file, "\nDetail Report of Currently Allocated Blocks");
    fclose(g_memory_log_file);
    g_memory_log_file = NULL;
  }
  else
    FPRINTF(stderr,"Can't open memory log file.\n");
  WriteMemoryStatus(stdout, msg);
  WriteMemoryRecords(stdout, "\nDetail Report of Currently Allocated Blocks");
}

void ascshutdownf(CONST char *msg)
{
  OpenLogFile();
  if (NULL != (g_memory_log_file = fopen(g_memlog_filename,"a"))){
    WriteMemoryStatus(g_memory_log_file,msg);
    if (g_memory_length) {
      WriteMemoryRecords(g_memory_log_file, "\n!!! SHUTDOWN ALERT -- POINTERS STILL ALLOCATED !!!");
      FPRINTF(g_memory_log_file, "!!! END OF SHUTDOWN MESSAGE !!!\n");
    } else {
      FPRINTF(g_memory_log_file, "NO POINTERS STILL ALLOCATED :-)\n");
    }
    fclose(g_memory_log_file);
    g_memory_log_file = NULL;
    FPRINTF(stderr, "Memory log filename: %s\n", g_memlog_filename);
    free(g_memlog_filename);	/* should not be ascfree() */
    g_memlog_filename = NULL;
  }
  else
    FPRINTF(stderr,"Can't open memory log file.\n");
  WriteMemoryStatus(stdout, msg);
  if (g_memory_length) {
    WriteMemoryRecords(stdout, "\n!!! SHUTDOWN ALERT -- POINTERS STILL ALLOCATED !!!");
    FPRINTF(stdout, "!!! END OF SHUTDOWN MESSAGE !!!\n");
  } else {
    FPRINTF(stdout, "NO POINTERS STILL ALLOCATED :-)\n");
  }
}

static void WriteAllocation(CONST VOIDPTR adr, size_t size,
                            CONST char *file, int line)
{
  if (NULL != (g_memory_log_file = fopen(g_memlog_filename,"a"))){
    FPRINTF(g_memory_log_file,"%9lx->%9x %9u %31s%6d %s\n",
                              (unsigned long)adr,
                              (unsigned long)adr + size - 1,
                              size,
                              "",
                              line,
                              file);
    fclose(g_memory_log_file);
    g_memory_log_file = NULL;
  }
  else{
    FPRINTF(stderr,"Unable to append to memory log file.\n");
    FPRINTF(stderr,"%9lx->%9x %9u %31s%6d %s\n",
                   (unsigned long)adr,
                   (unsigned long)adr + size - 1,
                   size,
                   "",
                   line,
                   file);
  }
}

static void WriteReAllocation(CONST VOIDPTR adr1, size_t size1,
                              CONST VOIDPTR adr2, size_t size2,
                              CONST char *file, int line)
{
  if (NULL != (g_memory_log_file = fopen(g_memlog_filename,"a"))){
    FPRINTF(g_memory_log_file,"%9lx->%9x %9u %9lx->%9x %9u %6d %s\n",
                              (unsigned long)adr2,
                              (unsigned long)adr2 + size2 - 1,
                              size2,
                              (unsigned long)adr1,
                              (unsigned long)adr1 + size1 - 1,
                              size1, line, file);
    fclose(g_memory_log_file);
    g_memory_log_file = NULL;
  }
  else{
    FPRINTF(stderr,"Unable to append to memory log file.\n");
    FPRINTF(stderr,"%9lx->%9x %9u %9lx->%9x %9u %6d %s\n",
                   (unsigned long)adr2,
                   (unsigned long)adr2 + size2 - 1,
                   size2,
                   (unsigned long)adr1,
                   (unsigned long)adr1 + size1 - 1,
                   size1, line, file);
  }
}

static void WriteDeallocation(CONST VOIDPTR adr, size_t size,
                              CONST char *file, int line)
{
  if (NULL != (g_memory_log_file = fopen(g_memlog_filename,"a"))){
    FPRINTF(g_memory_log_file,"%31s%9x->%9x %9u %6d %s\n","",
                              (unsigned long)adr,
                              (unsigned long)adr + size - 1,
                              size, line, file);
    fclose(g_memory_log_file);
    g_memory_log_file = NULL;
  }
  else{
    FPRINTF(stderr,"Unable to append to memory log file.\n");
    FPRINTF(stderr,"%31s%9x->%9x %9u %6d %s\n","",
                   (unsigned long)adr,
                   (unsigned long)adr + size - 1,
                   size, line, file);
  }
}

static void WriteError(CONST char *msg, CONST char *file, int line)
{
  FPRINTF(stderr,"%s\nCalled from file: %s on line %d.\n", msg, file, line);
  if (NULL != (g_memory_log_file = fopen(g_memlog_filename,"a"))){
    FPRINTF(g_memory_log_file,"%s\nCalled from file: %s on line %d.\n",
                              msg, file, line);
    fclose(g_memory_log_file);
    g_memory_log_file = NULL;
  }
}

/*
 *  Searches the array of allocation records for ptr.
 *  Returns -1 if no memory records have been recorded.
 *  Otherwise, returns the index of the memory record for
 *  which the stored pointer is greater than or equal to
 *  ptr.  If ptr is higher than any of the stored pointers,
 *  then g_memory_length is returned.
 *
 *  This routine assumes that the memory record array is sorted
 *  by pointer address ascending.
 */
static int SearchForMemory(CONST VOIDPTR ptr)
{
  int c, lower, upper;
  lower = 0;
  upper = g_memory_length-1;
  if (upper<lower) return -1;
  while(lower<upper){
    c = (lower+upper)/2;
    if (g_mem_rec[c].ptr == ptr) return c;
    if (g_mem_rec[c].ptr > ptr)
      upper = c-1;
    else
      lower = c+1;
  }
  if (upper<lower) return lower;
  if (lower>upper) return upper;
  while(( lower < g_memory_length ) && ( g_mem_rec[lower].ptr < ptr )) lower++;
  return lower;
}

int AllocatedMemoryF(CONST VOIDPTR ptr, size_t size)
{
  int pos;

  if (NULL == ptr)                    /* NULL ptr - by definition not allocated */
    return 0;

  pos = SearchForMemory(ptr);

  if (pos < 0)                        /* no allocation records have been stored */
    return 0;

  /* if a matching pointer was found... */
  if (( pos < g_memory_length ) &&
      ( g_mem_rec[pos].ptr == ptr )) {
    if ( g_mem_rec[pos].size == size )
      return 2;                       /* the block matches an allocated block */
    if ( g_mem_rec[pos].size > size )
      return 1;                       /* the block is contained in an allocated block */
    return -1;                        /* the block spans multiple allocated blocks */
  }

  /* if ptr block extends into the block above... */
  else if (( pos < g_memory_length ) &&
           ((CONST VOIDPTR)((CONST char *)ptr + size - 1) >= g_mem_rec[pos].ptr)) {
    return -1;                        /* the block extends into the block above */
  }

  else if (pos > 0) {
    if (ptr > ((CONST VOIDPTR)((CONST char *)g_mem_rec[pos-1].ptr + g_mem_rec[pos-1].size - 1)))
      return 0;                       /* ptr not contained within the found block */

    if ((CONST VOIDPTR)((CONST char *)ptr + size)
            <= (CONST VOIDPTR)((CONST char *)g_mem_rec[pos-1].ptr + g_mem_rec[pos-1].size))
      return 1;                       /* the block is contained in an allocated block */
    else
      return -1;                      /* the block spans multiple allocated blocks */
  }

  return 0;
}

static void AddAllocatedMemory(CONST VOIDPTR ptr, size_t size,
                               CONST char *file, int line)
{
  int pos,c;
  pos = SearchForMemory(ptr);
  if (( pos < 0 ) ||
      ( pos == g_memory_length) ||
      ( g_mem_rec[pos].ptr != ptr )) {
    if ( pos < 0 ) pos = 0;
    if ( (g_memory_length + 1) < MAXPOINTERS ) {
      if ( pos == g_memory_length ) {
        g_memory_length++;
        g_mem_rec[pos].ptr = ptr;
        g_mem_rec[pos].size = size;
      }
      else {
        /* make room for the new addition */
        for(c=g_memory_length ; c>pos ; c--){
          g_mem_rec[c] = g_mem_rec[c-1];
        }
        g_memory_length++;
        g_mem_rec[pos].ptr = ptr;
        g_mem_rec[pos].size = size;
      }
    }
    else {
      FPRINTF(stderr, "Pointer list filled up.  Error messages may be unreliable.\n");
    }
  }
  WriteAllocation(ptr,size,file,line);
}

static void DeallocateMemory(CONST VOIDPTR ptr, size_t size,
                             CONST char *file, int line)
{
  int pos,c;
  pos = SearchForMemory(ptr);
  if (( pos >= 0 ) && 
      ( pos < g_memory_length ) &&
      ( g_mem_rec[pos].ptr == ptr )) {
    /* a matching pointer was found */
    asc_assert(g_mem_rec[pos].size == size);
    /* copy all allocation records to the previous index, overwriting the current record */
    for(c=pos+1 ; c<g_memory_length ; c++)
      g_mem_rec[c-1] = g_mem_rec[c];
    g_memory_length--;
  }
  WriteDeallocation(ptr,size,file,line);
}

static void ReallocateMemory(CONST VOIDPTR ptr1, size_t size1,
                             CONST VOIDPTR ptr2, size_t size2,
                             CONST char *file, int line)
{
  int pos,c;
  /* handle the deallocation first */
  pos = SearchForMemory(ptr1);
  if (( pos >= 0 ) &&
      ( pos < g_memory_length ) &&
      ( g_mem_rec[pos].ptr == ptr1 )) {
    /* a matching pointer was found */
    asc_assert(g_mem_rec[pos].size == size1);
    /* copy all allocation records to the previous index, overwriting the current record */
    for(c=pos+1 ; c<g_memory_length ; c++)
      g_mem_rec[c-1] = g_mem_rec[c];
    g_memory_length--;
  }
  /* then, handle the allocation if ptr2 is non-NULL */
  pos = SearchForMemory(ptr2);
  if (( NULL != ptr2 ) &&
      (( pos < 0 ) ||
      ( pos == g_memory_length) ||
      ( g_mem_rec[pos].ptr != ptr2 ))) {
    if ( pos < 0 ) pos = 0;
    if ( (g_memory_length + 1) < MAXPOINTERS ) {
      if ( pos == g_memory_length ) {
        g_memory_length++;
        g_mem_rec[pos].ptr = ptr2;
        g_mem_rec[pos].size = size2;
      }
      else {
        /* make room for the new addition */
        for(c=g_memory_length ; c>pos ; c--){
          g_mem_rec[c] = g_mem_rec[c-1];
        }
        g_memory_length++;
        g_mem_rec[pos].ptr = ptr2;
        g_mem_rec[pos].size = size2;
      }
    }
    else {
      FPRINTF(stderr, "Pointer list filled up.  Error messages may be unreliable.\n");
    }
  }
  WriteReAllocation(ptr1,size1,ptr2,size2,file,line);
}

VOIDPTR asccallocf(size_t nelem, size_t elsize,
                   CONST char *file, int line)
{
  VOIDPTR result;
  OpenLogFile();
  result = calloc(nelem,elsize);
  if (NULL != result) {
    /* adjust statistical variables */
    g_memory_allocated += nelem*elsize;
    if (g_memory_allocated > g_peak_memory_usage)
      g_peak_memory_usage = g_memory_allocated;
    if (AllocatedMemory(result,nelem*elsize)){
      WriteError("calloc returned a piece of memory that is already being used.",
  	       file,line);
    }
    AddAllocatedMemory(result,nelem*elsize,file,line);
  }
  return result;
}

VOIDPTR ascmallocf(size_t size, CONST char *file, int line)
{
  VOIDPTR result;
  OpenLogFile();
  result = malloc(size);
  if (NULL != result) {
    g_memory_allocated += size;
    if (g_memory_allocated > g_peak_memory_usage)
      g_peak_memory_usage = g_memory_allocated;
    if (AllocatedMemory(result,size)){
      WriteError("malloc returned a piece of memory that is already being used.",
  	       file,line);
    }
    AddAllocatedMemory(result,size,file,line);
  }
  return result;
}

static size_t FindMemorySize(CONST VOIDPTR ptr, int * CONST found)
{
  int pos;
  pos = SearchForMemory(ptr);
  if (( pos >= 0 ) &&
      ( pos < g_memory_length)) {
    *found = 1;
    if (g_mem_rec[pos].ptr==ptr) return g_mem_rec[pos].size;
  }
  *found = 0;
  return 0;
}

VOIDPTR ascreallocf(VOIDPTR ptr, size_t size, CONST char *file, int line)
{
  size_t old_size;
  int found;
  VOIDPTR result;
  OpenLogFile();
  if (AllocatedMemory(ptr,0)){
    old_size = FindMemorySize(ptr,&found);
    if (!found){
      FPRINTF(stderr,"realloc'ing a piece of an allocated block.\n");
    }
    result = realloc(ptr,size);
  }
  else{
    old_size = 0;
    if (ptr == NULL) {
      result = malloc(size);
    } else {
      WriteError("ascreallocf called on a deallocated piece of memory.",
                 file, line);
      result = realloc(ptr,size);
    }
  }
  if (NULL == result)
    size = 0;
  ReallocateMemory(ptr,old_size,result,size,file,line);
  if (size >= old_size)
    g_memory_allocated += (size-old_size);
  else
    g_memory_allocated -= (old_size-size);
  if (g_memory_allocated > g_peak_memory_usage)
    g_peak_memory_usage = g_memory_allocated;
  return result;
}

void ascfreef(VOIDPTR ptr, CONST char *file, int line)
{
  size_t size;
  int found;

  if (NULL == ptr)
    return;

  OpenLogFile();
  if (0 != AllocatedMemory(ptr,0)) {
    size = FindMemorySize(ptr,&found);
    if (!found){		/* indicates a problem */
      WriteError("Deallocating a piece of an allocated block.", file, line);
    }
    else
      memset((char *)ptr, 0, size);	/* clear the memory */
  }
  else {
    WriteError("ascfreef called on a deallocated piece of memory.", file, line);
    size = 0;
  }
  DeallocateMemory(ptr,size,file,line);
  g_memory_allocated -= size;
  free(ptr);
}

VOIDPTR ascbcopyf(CONST VOIDPTR src, VOIDPTR dest, size_t length,
               CONST char *file, int line)
{
  UNUSED_PARAMETER(file);
  UNUSED_PARAMETER(line);
  OpenLogFile();
  return memcpy(dest, src, length);
}

VOIDPTR ascbzerof(VOIDPTR dest, size_t length, CONST char *file, int line)
{
  UNUSED_PARAMETER(file);
  UNUSED_PARAMETER(line);
  OpenLogFile();
  return memset((char *)dest, 0, length);
}

int InMemoryBlockF(CONST VOIDPTR ptr1, CONST VOIDPTR ptr2)
{
  int pos;
  pos = SearchForMemory(ptr1);
  if (( pos >= 0 ) &&
      ( pos < g_memory_length ) &&
      ( g_mem_rec[pos].ptr == ptr1 ))
    return (( ptr2 >= ptr1 ) &&
            ( ptr2 < (CONST VOIDPTR)((CONST char *)ptr1 + g_mem_rec[pos].size)));
  else
    return 0;
}

