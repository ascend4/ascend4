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
 *
 */

#include <stdio.h>
#include <assert.h>
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

char *asc_memcpy(char *to, char *from, unsigned size)
{
/* We really should be moving stuff a long at a time, but that mean
 * handling the leading and trailing unaligned bytes separately.
 * We also really should just be passing on to the vendor memcpy on
 * those systems which do overlapping ranges right.
 */
  char *fill;
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

char *ascreallocPUREF(char *ptr,size_t oldbytes, size_t newbytes)
{
  /* shrink */
  if (newbytes > 0  && newbytes <= oldbytes) return ptr;
  /* release */
  if (!newbytes) {
    ascfree(ptr);
    return NULL;
  }
  /* create */
  if (!oldbytes) {
    if (ptr!=NULL) ascfree(ptr); /* some OS allocate 0 bytes, god help us */
    return (char *)ascmalloc(newbytes);
  } else {
    /* expand */
    char *ret;
    ret = ascmalloc(newbytes);
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
    ascfree(ptr);
    return ret;
  }
}
#ifdef MALLOC_DEBUG
struct memory_rec{
  CONST VOIDPTR ptr;
  unsigned size;
};

FILE *g_memory_log_file = NULL;
int g_memory_length =0;
unsigned long g_memory_allocated=0L;
unsigned long g_peak_memory_usage=0L;
struct memory_rec g_mem_rec[MAXPOINTERS];
char *g_memlog_filename = NULL;

unsigned long ascmeminuse(void)
{
#ifndef MALLOC_DEBUG
  return 0;
#else
  return g_memory_allocated;
#endif
}

int SortedList(void)
{
  int c;
  for(c=1;c<g_memory_length;c++)
    if (g_mem_rec[c-1].ptr>g_mem_rec[c].ptr) return 0;
  return 1;
}

CONST VOIDPTR MinMemory(void)
{
  int c;
  CONST VOIDPTR minm=(VOIDPTR)ULONG_MAX;
  for(c=0;c<g_memory_length;c++)
    if (g_mem_rec[c].ptr < minm) minm = g_mem_rec[c].ptr;
  return minm;
}

CONST VOIDPTR MaxMemory(void)
{
  int c;
  CONST VOIDPTR maxm=0;
  for(c=0;c<g_memory_length;c++)
    if (((CONST char *)g_mem_rec[c].ptr+g_mem_rec[c].size)
	> (CONST char *)maxm)
      maxm = (CONST VOIDPTR)((CONST char *)g_mem_rec[c].ptr+
			     g_mem_rec[c].size);
  return maxm;
}

#define NONZERO(x) ((x) ? (x) : 1)

CONST VOIDPTR MemoryMean(void)
{
  int c,size;
  double sum=0.0,bytes=0.0;
  for(c=0;c<g_memory_length;c++){
    size = g_mem_rec[c].size;
    if (size){
      bytes += (double)size;
      sum += (double)((size * (size-1))/2)
	+ ((double)size) * ((double)(unsigned long)g_mem_rec[c].ptr);
    }
  }
  return (VOIDPTR)(unsigned long)(sum/NONZERO(bytes));
}

void OpenLogFile(void)
{
  if (!g_memlog_filename){
    time_t t;
    g_memlog_filename = tempnam(NULL,LOGFILE);
    if (g_memlog_filename &&
	(g_memory_log_file = fopen(g_memlog_filename,"w"))==NULL){
      Asc_Panic(2, NULL, "Unable to open memory log file.\n");
    }
    t = time((time_t *)NULL);
    FPRINTF(g_memory_log_file,"Ascend memory log file opened %s",
	    asctime(localtime(&t)));
    fclose(g_memory_log_file);
    g_memory_log_file = NULL;
  }
}

void WriteMemoryStatus(FILE *f, CONST char *msg)
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
	  msg,g_memory_allocated,g_memory_length,g_peak_memory_usage,
	  (unsigned long)minm,(unsigned long)maxm,
	  ((double)g_memory_allocated/(double)
	   NONZERO((CONST char *)maxm-(CONST char *)minm)),
	  (unsigned long)MemoryMean());
}

void ascstatus(CONST char *msg)
{
  OpenLogFile();
  if ((g_memory_log_file=fopen(g_memlog_filename,"a"))){
    WriteMemoryStatus(g_memory_log_file,msg);
    fclose(g_memory_log_file);
  }
  else
    FPRINTF(stderr,"Can't open memory log file.\n");
  WriteMemoryStatus(stdout,msg);
}

void ascshutdown(CONST char *msg)
{
  unsigned long c;
  OpenLogFile();
  if ((g_memory_log_file=fopen(g_memlog_filename,"a"))){
    WriteMemoryStatus(g_memory_log_file,msg);
    if (g_memory_length) {
      FPRINTF(g_memory_log_file,
	      "!!!SHUTDOWN MESSAGE -- POINTERS STILL ALLOCATED\n");
      for(c=0;c < g_memory_length;c++){
        FPRINTF(g_memory_log_file,"%9lx %9u\n",g_mem_rec[c].ptr,
	        g_mem_rec[c].size);
      }
      FPRINTF(g_memory_log_file, "!!!END OF SHUTDOWN MESSAGE\n");
    } else {
      FPRINTF(g_memory_log_file, "NO POINTERS STILL ALLOCATED :-)\n");
    }
    fclose(g_memory_log_file);
    FPRINTF(stderr,"Memory log filename: %s\n",g_memlog_filename);
    free(g_memlog_filename);	/* should not be ascfree() */
    g_memlog_filename = NULL;
  }
  else
    FPRINTF(stderr,"Can't open memory log file.\n");
  WriteMemoryStatus(stdout,msg);
}

void WriteAllocation(CONST VOIDPTR adr, unsigned int size,
		     CONST char *file, int line)
{
  if ((g_memory_log_file=fopen(g_memlog_filename,"a"))){
    FPRINTF(g_memory_log_file,"%9lx %9u %20s%d %s\n",
	    (unsigned long)adr,size,"",line,file);
    fclose(g_memory_log_file);
  }
  else{
    FPRINTF(stderr,"Unable to append to memory log file.\n");
    FPRINTF(stderr,"%9lx %9u %20s%d %s\n",
	    (unsigned long)adr,size,"",line,file);
  }
}

void WriteReAllocation(CONST VOIDPTR adr1, unsigned int size1,
		       CONST VOIDPTR adr2, unsigned int size2,
		       CONST char *file, int line)
{
  if ((g_memory_log_file=fopen(g_memlog_filename,"a"))){
    FPRINTF(g_memory_log_file,"%9lx %9u %9lx %9u %d %s\n",
	    (unsigned long)adr1,size1,(unsigned long)adr2,size2,line,file);
    fclose(g_memory_log_file);
  }
  else{
    FPRINTF(stderr,"Unable to append to memory log file.\n");
    FPRINTF(stderr,"%9lx %9u %9lx %9u %d %s\n",
	    (unsigned long)adr1,size1,(unsigned long)adr2,size2,line,file);
  }
}

void WriteDeallocation(CONST VOIDPTR adr, unsigned int size,
		       CONST char *file, int line)
{
  if ((g_memory_log_file=fopen(g_memlog_filename,"a"))){
    FPRINTF(g_memory_log_file,"%20s%9lx %9u %d %s\n","",
	    (unsigned long)adr,size,line,file);
    fclose(g_memory_log_file);
  }
  else{
    FPRINTF(stderr,"Unable to append to memory log file.\n");
    FPRINTF(stderr,"%20s%9lx %9u %d %s\n","",
	    (unsigned long)adr,size,line,file);
  }
}

void WriteError(CONST char *msg, CONST char *file, int line)
{
  FPRINTF(stderr,"%s\nCalled from file: %s on line %d.\n",msg,file,line);
  if ((g_memory_log_file=fopen(g_memlog_filename,"a"))){
    FPRINTF(g_memory_log_file,"%s\nCalled from file: %s on line %d.\n",
	    msg,file,line);
    fclose(g_memory_log_file);
  }
}

int SearchForMemory(CONST VOIDPTR ptr)
{
  int c,l,u;
  l = 0;
  u = g_memory_length-1;
  if (u<l) return -1;
  while(l<u){
    c = (l+u)/2;
    if (g_mem_rec[c].ptr == ptr) return c;
    if (g_mem_rec[c].ptr > ptr)
      u = c-1;
    else
      l = c+1;
  }
  if (u<l) return l;
  if (l>u) return u;
  while((l<(g_memory_length-1))&&(g_mem_rec[l].ptr<ptr)) l++;
  return l;
}

int AllocatedMemory(CONST VOIDPTR ptr, unsigned int size)
{
  int pos,result=0;
  pos = SearchForMemory(ptr);
  if ((pos>=0)&&(g_mem_rec[pos].ptr==ptr)){
    if (g_mem_rec[pos].size==size) return 2;
    if (g_mem_rec[pos].size>size) return 1;
    result = -1;
  }
  for(pos=0;pos<g_memory_length;pos++){
    if ((ptr>=g_mem_rec[pos].ptr)&&
	((ptr<=(CONST VOIDPTR)((CONST char *)g_mem_rec[pos].ptr+
	       g_mem_rec[pos].size)))){
      if ((CONST VOIDPTR)((CONST char *)ptr+size)
	  <=(CONST VOIDPTR)((CONST char *)g_mem_rec[pos].ptr+
			    g_mem_rec[pos].size))
	return 1;
      else result = -1;
    }
    else if (((CONST VOIDPTR)((CONST char *)ptr+size)
	      >=g_mem_rec[pos].ptr)&&
	     ((CONST VOIDPTR)((CONST char *)ptr+size)
	      <=(CONST VOIDPTR)((CONST char *)g_mem_rec[pos].ptr+
	      g_mem_rec[pos].size)))
      result = -1;
  }
  return result;
}

void AddAllocatedMemory(CONST VOIDPTR ptr, unsigned int size,
			CONST char *file, int line)
{
  int pos,c;
  pos = SearchForMemory(ptr);
  if ((pos<0)||(g_mem_rec[pos].ptr != ptr)){
    if (pos<0) pos=0;
    if (g_memory_length+1<MAXPOINTERS){
      if ((pos==g_memory_length-1)&&
	  (g_mem_rec[pos].ptr < ptr)){
	pos++;
	g_memory_length++;
	g_mem_rec[pos].ptr = ptr;
	g_mem_rec[pos].size = size;
      }
      else{
	/* make room for the new addition */
	for(c=g_memory_length;c>pos;c--){
	  g_mem_rec[c] = g_mem_rec[c-1];
	}
	g_memory_length++;
	g_mem_rec[pos].ptr = ptr;
	g_mem_rec[pos].size = size;
      }
    }
    else{
      FPRINTF(stderr,
	      "Pointer list filled up.  Error messages may be unreliable.\n");
    }
  }
  WriteAllocation(ptr,size,file,line);
}

void DeallocateMemory(CONST VOIDPTR ptr, unsigned int size,
		      CONST char *file, int line)
{
  int pos,c;
  pos = SearchForMemory(ptr);
  if ((pos>=0)&&(g_mem_rec[pos].ptr==ptr)){
    assert(g_mem_rec[pos].size==size);
    for(c=pos+1;c<g_memory_length;c++)
      g_mem_rec[c-1] = g_mem_rec[c];
    g_memory_length--;
  }
  WriteDeallocation(ptr,size,file,line);
}

void ReallocateMemory(CONST VOIDPTR ptr1, unsigned int size1,
		      CONST VOIDPTR ptr2, unsigned int size2,
		      CONST char *file, int line)
{
  int pos,c;
  pos = SearchForMemory(ptr1);
  if ((pos>=0)&&(g_mem_rec[pos].ptr==ptr1)){
    assert(g_mem_rec[pos].size==size1);
    for(c=pos+1;c<g_memory_length;c++)
      g_mem_rec[c-1] = g_mem_rec[c];
    g_memory_length--;
  }
  pos = SearchForMemory(ptr2);
  if ((pos<0)||(g_mem_rec[pos].ptr!=ptr2)){
    if (pos<0) pos=0;
    if (g_memory_length+1<MAXPOINTERS){
      if ((pos==g_memory_length-1)&&
	  (g_mem_rec[pos].ptr<ptr2)){
	pos++;
	g_memory_length++;
	g_mem_rec[pos].ptr = ptr2;
	g_mem_rec[pos].size = size2;
      }
      else{
	/* make room for the new addition */
	for(c=g_memory_length;c>pos;c--){
	  g_mem_rec[c] = g_mem_rec[c-1];
	}
	g_memory_length++;
	g_mem_rec[pos].ptr = ptr2;
	g_mem_rec[pos].size = size2;
      }
    }
    else{
      FPRINTF(stderr,
	      "Pointer list filled up.  Error messages may be unreliable.\n");
    }
  }
  WriteReAllocation(ptr1,size1,ptr2,size2,file,line);
}

VOIDPTR asccallocf(unsigned int nelem, unsigned int elsize,
		   CONST char *file, int line)
{
  VOIDPTR result;
  OpenLogFile();
  result = calloc(nelem,elsize);
  /* adjust statistical variables */
  g_memory_allocated += nelem*elsize;
  if (g_memory_allocated > g_peak_memory_usage)
    g_peak_memory_usage = g_memory_allocated;
  if (AllocatedMemory(result,nelem*elsize)){
    WriteError("calloc returned a piece of memory that is already being used.",
	       file,line);
  }
  AddAllocatedMemory(result,nelem*elsize,file,line);
  return result;
}

VOIDPTR ascmallocf(unsigned int size, CONST char *file, int line)
{
  VOIDPTR result;
  OpenLogFile();
  result = malloc(size);
  g_memory_allocated += size;
  if (g_memory_allocated > g_peak_memory_usage)
    g_peak_memory_usage = g_memory_allocated;
  if (AllocatedMemory(result,size)){
    WriteError("malloc returned a piece of memory that is already being used.",
	       file,line);
  }
  AddAllocatedMemory(result,size,file,line);
  return result;
}

unsigned FindMemorySize(CONST VOIDPTR ptr, int * CONST found)
{
  int pos;
  pos = SearchForMemory(ptr);
  if (pos>=0){
    *found = 1;
    if (g_mem_rec[pos].ptr==ptr) return g_mem_rec[pos].size;
  }
  *found = 0;
  return 0;
}

VOIDPTR ascreallocf(VOIDPTR ptr, unsigned int size, CONST char *file, int line)
{
  unsigned old_size;
  int found;
  VOIDPTR result;
  OpenLogFile();
  if (AllocatedMemory(ptr,0)){
    old_size = FindMemorySize(ptr,&found);
    if (!found){
      FPRINTF(stderr,"realloc'ing a piece of an allocated block.\n");
    }
    result = realloc(ptr,size);
    ReallocateMemory(ptr,old_size,result,size,file,line);
  }
  else{
    WriteError("ascreallocf called on a deallocated piece of memory.",file,
	       line);
    old_size = 0;
    if (ptr==NULL) {
      result = malloc(size);
    } else {
      result = realloc(ptr,size);
    }
    ReallocateMemory(ptr,old_size,result,size,file,line);
  }
  if (size >= old_size) g_memory_allocated += (size-old_size);
  else g_memory_allocated -= (old_size-size);
  if (g_memory_allocated > g_peak_memory_usage)
    g_peak_memory_usage = g_memory_allocated;
  return result;
}

void ascfreef(VOIDPTR ptr, CONST char *file, int line)
{
  unsigned size;
  int found;
  OpenLogFile();
  if (AllocatedMemory(ptr,0)){
    size = FindMemorySize(ptr,&found);
    if (!found){		/* indicates a problem */
      WriteError("Deallocating a piece of an allocated block.",file,line);
    }
    else
      memset((char *)ptr,0,(int)size);	/* clear the memory */
  }
  else{
    WriteError("ascfreef called on a deallocated piece of memory.",file,line);
    size = 0;
  }
  DeallocateMemory(ptr,size,file,line);
  g_memory_allocated -= size;
  free(ptr);
}

void ascbcopyf(CONST VOIDPTR b1, VOIDPTR b2, int size,
	       CONST char *file, int line)
{
  OpenLogFile();
  memcpy(b2,b1,size);
}

void ascbzerof(VOIDPTR b1, int length, CONST char *file, int line)
{
  OpenLogFile();
  memset((char *)b1,0,length);
}

int InMemoryBlock(CONST VOIDPTR ptr1, CONST VOIDPTR ptr2)
{
  int pos;
  pos = SearchForMemory(ptr1);
  if (g_mem_rec[pos].ptr==ptr1)
    return (ptr2>=ptr1)&&(ptr2 <= (CONST VOIDPTR)((CONST char *)ptr1+
				  g_mem_rec[pos].size));
  else
    return 0;
}
#else
void ascstatus(CONST char *msg)
{
  (void) msg;
}

void ascshutdown(CONST char *msg)
{
  (void) msg;
}
#endif /* MALLOC_DEBUG */
