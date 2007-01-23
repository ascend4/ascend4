/*
 *  Implementation of Free Store Module
 *  Kirk A. Abbott
 *  Created Dec 18, 1994
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: freestore.c,v $
 *  Date last modified: $Date: 1997/07/18 12:29:49 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1994, 1995 Kirk Andre Abbott.
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
#include <stdarg.h>
#include <utilities/ascConfig.h>

#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <general/stack.h>
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "expr_types.h"
#include "relation_type.h"
#include "freestore.h"
#include <general/mathmacros.h>

/*
 * This is an implementation of a *freestore* which is hoped to
 * reduce the memory management difficulties associated with
 * symbolic processing. This is algorithm 1
 * which does no *freeing* until a computation is done.
 * In this way it acts as a *buffer*. The difficulty is that
 * instantaneous size of the buffer may be large, depending upon
 * the number of terms generated, as we dont *free* or mark nodes
 * as unused until we are done.
 *
 * A second variation of this algorithm would be to call Copy
 * and Destroy, which would play a reference count game. We would
 * also maintain a stack of pointers to nodes, as they were requested
 * to be destroyed. This stack would allow us to find free terms quickly
 * as within the distribution of terms.
 *
 *
 * 		--------------------------------------------------------
 *		|	|	|	|	|	|	|	|
 *	------>	|   O	|   X	|   X	|   X	|   O	|   O	|   O	|
 *		|	|	|	|	|	|	|	|
 * 		--------------------------------------------------------
 *		    ^			           ^
 *		    |				   |
 *		    |	    |--------------->-------
 * 		--------------------------------------------------------
 *	------>	|	|	|	|	|	|	|	|
 * 		--------------------------------------------------------
 */

static struct FreeStore *g_free_store = NULL;
static long g_units_alloc = 0L;

struct FreeStore *FreeStore_Create(int n_buffers,int buffer_length)
{
  struct FreeStore *result;
  union RelationTermUnion **root;
  int i;

  result = (struct FreeStore *)malloc(sizeof(struct FreeStore));
  /*
   * Make and initialize the array of pointers to the buffers.
   */
  root = (union RelationTermUnion **)
    calloc(n_buffers,sizeof(union RelationTermUnion *));

  /*
   * Allocate all n_buffers of size buffer_length.
   * Allocate the return stack at 30% of the buffer_length;
   * The return stack is expanded by the stack module at a rate
   * orig_capacity *(1.5)^n-1, where n is the number of times that the
   * stack has to be reallocated.
   */
  for (i=0;i<n_buffers;i++) {
    root[i] =  (union RelationTermUnion *)
		malloc(buffer_length*sizeof(union RelationTermUnion));
  }
  result->returned = gs_stack_create(MAX(8,(long)0.3*buffer_length));

  /*
   * Set the structures created to the result; Add the other
   * information about the freestore.
   */
  result->root = root;
  result->next_free = root[0];	/* start of first buffer */
  result->n_buffers = n_buffers;
  result->buffer_length = buffer_length;
  result->row = 0;
  result->col = 0;
  return result;
}

void FreeStore_ReInit(struct FreeStore *store)
{
  if (store && store->root) {
    gs_stack_destroy(store->returned,0); /* faster to destroy and rebuild */
    store->returned = gs_stack_create(MAX(8,(long)0.3*store->buffer_length));
    store->row = 0;
    store->col = 0;
    store->next_free = store->root[0];
    g_units_alloc = 0;
  }
}

static union RelationTermUnion *FreeStore__GetMem(struct FreeStore *store)
{
  union RelationTermUnion *result;
  union RelationTermUnion *ptr;
  union RelationTermUnion **root;
  int i;

  root = store->root;
  if (!root) {
    FPRINTF(stderr,"FreeStore not initialized\n");
    return NULL;
  }

  /*
   * Try the returned list first.
   */
  ptr = (union RelationTermUnion *)gs_stack_pop(store->returned);
  if (ptr!=NULL) {
    return ptr;
  }
  else if (store->col == store->buffer_length) {
    /*
     * Nothing on the returned stack, and we are out of blocks
     * on the current buffer, i.e, need to make a buffer.
     */
    i = store->n_buffers++; /* update buffer count */
    root = (union RelationTermUnion **)
      realloc(root,store->n_buffers * sizeof(union RelationTermUnion*));
    root[i] = (union RelationTermUnion *)
      malloc(store->buffer_length * sizeof(union RelationTermUnion));
    store->root = root; 	/* critical !! -- realloc could moved root */
    result = &root[i][0]; 	/* memory to be returned */
    store->col = 1;	 	/* point to next free block */
    store->row = i;	 	/* update the row counter */
    store->next_free = &root[i][1];
    return result;
  }
  else{
    ptr = store->next_free;
    store->next_free++;
    store->col++;
    return ptr;
  }
}


static union RelationTermUnion *FreeStore__FindMem(struct FreeStore *store,
				union RelationTermUnion *term)
{
  union RelationTermUnion **root, *start, *end;
  int i;

  if (store==NULL || term==NULL)
    return NULL;
  root = store->root;
  for (i=0;i<store->n_buffers;i++) {
    start = root[i] - 1;			/* just before the start */
    end = root[i] + store->buffer_length;	/* just beyond the end */
    if ((term > start ) && (term < end)) {
      return term;
    }
  }
  return NULL;	/* term not found within range */
}

static
void FreeStore__FreeMem(struct FreeStore *store,
			union RelationTermUnion *term)
{
  union RelationTermUnion *ptr;

  ptr = FreeStore__FindMem(store,term); /* check if we own the memory */
  if (ptr!=NULL) {
    gs_stack_push(store->returned,ptr);
  }
  else{
    FPRINTF(stderr,"This is not one of our pointers;");
    FPRINTF(stderr,"free it yourself.\n");
  }
}

union RelationTermUnion *FreeStoreCheckMem(union RelationTermUnion *term)
{
  return FreeStore__FindMem(FreeStore_GetFreeStore(),term);
}

void FreeStore__Statistics(FILE *fp,
			   struct FreeStore *store)
{
  union RelationTermUnion *bufptr;
  int n_allocated=0;
  int n_maxused=0;
  int n_destroyed=0;
  int n_buffers, buffer_length;
  int i,j;

  if (store==NULL) {
    FPRINTF(stderr,"FreeStore is nonexistent\n");
    return;
  }
  n_buffers = store->n_buffers;
  buffer_length = store->buffer_length;
  n_allocated = n_buffers * buffer_length;
  n_maxused = (store->row)*buffer_length + store->col;
  for (i=0;i<n_buffers;i++) {
    bufptr = store->root[i];
    for (j=0;j<buffer_length;j++) {
      if (bufptr == store->next_free)
	break;
      bufptr++;
    }
  }
  n_destroyed = (int)gs_stack_size(store->returned);
  FPRINTF(fp,"Memory Allocated = %d or %ld bytes\n",
	  n_allocated,(long)n_allocated*sizeof(union RelationTermUnion));
  FPRINTF(fp,"Maximum Memory Used = %d or %ld bytes\n",
	  n_maxused,(long)n_maxused*sizeof(union RelationTermUnion));
  FPRINTF(fp,"Memory Destroyed = %d or %ld bytes\n",
	  n_destroyed,(long)n_destroyed*sizeof(union RelationTermUnion));
}

/*
 * This is to be used as a high water counter
 * over many invocations of the free strore.
 */
long FreeStore_UnitsAllocated()
{
  return g_units_alloc;
}

void FreeStore__BlastMem(struct FreeStore *store)
{
  int i,n_buffers;
  if (!store)
    return;
  n_buffers = store->n_buffers;
  for (i=0;i<n_buffers;i++) {
    ascfree((char *)store->root[i]);	/* freeup each buffer */
    store->root[i] = NULL;		/* unnecessary ... oh well*/
  }
  ascfree((char *)store->root);
  gs_stack_destroy(store->returned,0);
  store->row = store->col = 0;
  store->next_free = NULL;
  ascfree((char *)store);
  store = NULL;
  g_units_alloc = 0;
}

void FreeStore_SetFreeStore(struct FreeStore *store)
{
  g_free_store = store;
}

struct FreeStore *FreeStore_GetFreeStore(void)
{
  return g_free_store;
}

union RelationTermUnion *GetMem()
{
  g_units_alloc++;
  return FreeStore__GetMem(g_free_store);
}

void FreeMem(union RelationTermUnion *term)
{
  FreeStore__FreeMem(g_free_store,term);
}

#ifdef FREESTORE_TEST
main (int argc, char **argv)
{
  union RelationTermUnion *term = NULL;
  union RelationTermUnion **list = NULL;
  int n_buffers = 1;
  int buffer_length = 5;
  int len = 10;
  int i;

  g_free_store = FreeStore_Create(n_buffers,buffer_length);

  list = (union RelationTermUnion **)
		calloc(len,sizeof(union RelationTermUnion *));
  for (i=0;i<len;i++) {
    term = GetMem();
    if (term) {
      V_TERM(term)->varnum = i;
      term->t = e_var;
      list[i] = term;
    }
  }
  FreeMem(list[4]);
  FreeMem(list[5]);
  FreeMem(list[6]);

  term = (union RelationTermUnion *)malloc(sizeof(union RelationTermUnion));
  V_TERM(term)->varnum = 6;
  FreeMem(term);
  FreeStore__Statistics(stdout,g_free_store);
  exit(0);
}

/*
 * acc -o freestore_test freestore.c ascmalloc.o stack.o list.o
 */
#endif /* FREESTORE_TEST */
