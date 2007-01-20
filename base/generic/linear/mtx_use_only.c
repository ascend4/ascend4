/*
 *  mtx: Ascend Sparse Matrix Package
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: mtx_use_only.c,v $
 *  Date last modified: $Date: 2000/01/25 02:27:12 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
 *  Copyright (C) 1995 Kirk Andre Abbott, Benjamin Andrew Allan
 *  Copyright (C) 1996 Benjamin Andrew Allan
 *
 *  The SLV solver is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The SLV solver is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 *  COPYING is found in ../compiler.
 */
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/mem.h>
#include "mtx.h"
/* grab our private parts */
#define __MTX_C_SEEN__
#include "mtx_use_only.h"

#define free_unless_null(ptr) if( NOTNULL(ptr) ) ascfree(ptr)

FILE *g_mtxerr;

struct element_t *mtx_find_element(mtx_matrix_t mtx, int32 org_row,
                                   int32 org_col)
{
   register struct element_t *elt;

/* old way.
   for( elt = mtx->hdr.row[org_row] ; NOTNULL(elt) ; elt = elt->next.col )
      if( elt->col == org_col )
         break;
*/
   for( elt = mtx->hdr.row[org_row] ;
        NOTNULL(elt) && elt->col!=org_col ;
        elt = elt->next.col );
   return(elt);
}

struct element_t *mtx_create_element(mtx_matrix_t mtx,
                                     int32 org_row, int32 org_col)
/**
 ***  Creates the given element and returns a pointer to it.  The value is
 ***  initially zero.
 **/
{
  register struct element_t *elt;

#if MTX_DEBUG
  if( NOTNULL(mtx_find_element(mtx,org_row,org_col)) ) {
    FPRINTF(g_mtxerr,"ERROR:  (mtx) create_element\n");
    FPRINTF(g_mtxerr,"        Element (%d,%d) already exists.\n",
            org_row,org_col);
  }
#endif
  elt = (struct element_t *)mem_get_element(mtx->ms);
  /* guess who didn't check for the alloc return. */
  elt->value = 0.0;
  elt->row = org_row;
  elt->col = org_col;
  elt->next.col = mtx->hdr.row[org_row];
  elt->next.row = mtx->hdr.col[org_col];
  mtx->hdr.row[org_row] = mtx->hdr.col[org_col] = elt;
  return(elt);
}

struct element_t *mtx_create_element_value(mtx_matrix_t mtx,
                                           int32 org_row,
                                           int32 org_col,
                                           real64 val)
{
  register struct element_t *elt;

#if MTX_DEBUG
  if( NOTNULL(mtx_find_element(mtx,org_row,org_col)) ) {
    FPRINTF(g_mtxerr,"ERROR:  (mtx) create_element_value\n");
    FPRINTF(g_mtxerr,"        Element (%d,%d) already exists.\n",
            org_row,org_col);
  }
#endif
  elt = (struct element_t *)mem_get_element(mtx->ms);
  /* guess who didn't check for the alloc return. not needed here. */
  elt->value = val;
  elt->row = org_row;
  elt->col = org_col;
  elt->next.col = mtx->hdr.row[org_row];
  elt->next.row = mtx->hdr.col[org_col];
  mtx->hdr.row[org_row] = mtx->hdr.col[org_col] = elt;
  return(elt);
}

/* new version */
struct element_t *mtx_next_col(register struct element_t *elt,
                               mtx_range_t *rng, int32 *tocur)
{
   if( NOTNULL(elt) ) {
     elt = elt->next.col;
   } else {
     return elt;
   }
   /* use of not_in_range in the following is a loser on alphas */
   /* due to the price of assigning the range limits */
   if( rng != mtx_ALL_COLS ) {
      if( rng->high < rng->low ) return NULL;
      while( NOTNULL(elt) && !in_range(rng,tocur[elt->col]) )
         elt = elt->next.col;
   }
   return( elt );
}

/* original version */
struct element_t *mtx_next_row(register struct element_t *elt,
                               mtx_range_t *rng, int32 *tocur)
{
   if( NOTNULL(elt) ) elt = elt->next.row;
   if( rng != mtx_ALL_ROWS ) {
      if( rng->high < rng->low ) return NULL;
      while( NOTNULL(elt) && !in_range(rng,tocur[elt->row]) )
         elt = elt->next.row;
   }
   return( elt );
}


int32 *mtx_alloc_perm(int32 cap)
{
  int32 *perm;
  perm = (int32 *)ascmalloc( (cap+1)*sizeof(int32) );
  *perm = -1;
  return( perm+1 );
}

void mtx_copy_perm( int32 *tarperm, int32 *srcperm, int32 cap)
/**
 ***  Copies srcperm to tarperm given the capacity of srcperm.
 ***  If tarperm was obtained from
 ***  alloc_perm(), the -1 has already been copied
 **/
{
  mem_copy_cast(srcperm,tarperm,cap*sizeof(int32));
}

#define free_perm(perm) ascfree( (POINTER)((perm)-1) )
/**
 ***  Frees a permutation vector.
 **/

void mtx_free_perm(int32 *perm)
{
  free_perm(perm);
}

/* some local scope globals to keep memory so we aren't constantly
   reallocating */
struct reusable_data_vector
  g_mtx_null_index_data = {NULL,0,sizeof(int32),0},
  g_mtx_null_sum_data = {NULL,0,sizeof(real64),0},
  g_mtx_null_mark_data = {NULL,0,sizeof(char),0},
  g_mtx_null_vector_data = {NULL,0,sizeof(struct element_t *),0},
  g_mtx_null_col_vector_data = {NULL,0,sizeof(struct element_t *),0},
  g_mtx_null_row_vector_data = {NULL,0,sizeof(struct element_t *),0};

/* the psychologist function. resets data to zero after some checking */
static void mtx_sanify_reuseable(struct reusable_data_vector *r)
{
  if (r->last_line) {
    r->last_line = ZERO;
    if (r->capacity > 0 && r->arr != NULL) {
      /* rezero the buffer */
      switch (r->entry_size) {
      case sizeof(int32):
        mtx_zero_int32(r->arr,r->capacity);
        break;
      case sizeof(real64):
        mtx_zero_real64(r->arr,r->capacity);
        break;
      default:
        mtx_zero_char(r->arr, r->capacity*r->entry_size);
        break;
      }
    }
  }
  if (r->capacity && r->arr == NULL) {
    /* forget it exists (capacity && !arr --> insanity) */
    r->capacity = 0;
    r->arr = NULL;
  }
}

/* a function to insure the sanity of all the reuseable vectors */
void mtx_reset_null_vectors(void) {
  mtx_sanify_reuseable( &g_mtx_null_index_data);
  mtx_sanify_reuseable( &g_mtx_null_sum_data);
  mtx_sanify_reuseable( &g_mtx_null_mark_data);
  mtx_sanify_reuseable( &g_mtx_null_row_vector_data);
  mtx_sanify_reuseable( &g_mtx_null_col_vector_data);
  mtx_sanify_reuseable( &g_mtx_null_vector_data);
}

/* get a chunk of memory (probably already allocated) and return
   to user who will reset it to 0s before releasing it. */
void *mtx_null_vector_f(int32 cap, int line, CONST char *file,
                        struct reusable_data_vector *ptr, char *fn)
/**
 ***  Call only via the macros.
 ***  Bugs: does not check for null return.
 ***  but then if this returns null, a crash from some other cause
 ***  is imminent anyway.
 **/
{
  if (ptr->last_line) {
    FPRINTF(g_mtxerr,"Warning: (%s) mtx_%s called while data in use\n",file,fn);
    FPRINTF(g_mtxerr,"         Last called line: %d, this call line %d\n",
            ptr->last_line,line);
    if (cap > 0 && cap <= ptr->capacity) {
      switch (ptr->entry_size) {
      case sizeof(int32):
	mtx_zero_int32(ptr->arr,ptr->capacity);
	break;
      case sizeof(real64):
	mtx_zero_real64(ptr->arr,ptr->capacity);
	break;
      default:
        mtx_zero_char(ptr->arr, ptr->capacity*ptr->entry_size);
        break;
      }
    }
  }
  if (!cap) {
    free_unless_null( (POINTER)ptr->arr );
    ptr->capacity = ZERO;
    ptr->arr = NULL;
    ptr->last_line = ZERO;
    return( ptr->arr );
  }

  if( cap > ptr->capacity ) {
    free_unless_null( (POINTER)ptr->arr );
    ptr->arr = asccalloc( cap, ptr->entry_size );
    if (ISNULL(ptr->arr)) {
      ptr->capacity = ZERO;
      ptr->last_line = ZERO;
      FPRINTF(g_mtxerr,"ERROR: memory allocation failed in mtx_%s.\n",fn);
      return ptr->arr;
    }
    ptr->capacity = cap;
  }
  ptr->last_line = line;
  return( ptr->arr );
}

/**
 ***  Call only via the macros.
 **/
void mtx_null_vector_release_f(int line, CONST char *file,
                               struct reusable_data_vector *ptr, char *fn)
{
  if (ptr->last_line == 0) {
    FPRINTF(g_mtxerr,
      "Warning:  (%s) mtx_%s_release called (line %d) on vector not in use\n",
       file, fn, line);
  }
  ptr->last_line = 0;
}

/********************************************************************/

struct element_t **mtx_expand_row(mtx_matrix_t mtx, int32 org)
{
   struct element_t **arr;
   struct element_t *elt;

   arr = mtx_null_vector(mtx->order);
   for( elt=mtx->hdr.row[org]; NOTNULL(elt); elt=elt->next.col )
      arr[elt->col] = elt;
   return(arr);
}

struct element_t **mtx_expand_col(mtx_matrix_t mtx, int32 org)
{
   struct element_t **arr;
   struct element_t *elt;

   arr = mtx_null_vector(mtx->order);
   for( elt = mtx->hdr.col[org] ; NOTNULL(elt) ; elt = elt->next.row )
      arr[elt->row] = elt;
   return(arr);
}

void mtx_renull_using_row(mtx_matrix_t mtx, int32 org,
                          struct element_t **arr)
{
   struct element_t *elt;

   for( elt=mtx->hdr.row[org]; NOTNULL(elt); elt=elt->next.col )
      arr[elt->col] = NULL;
}

void mtx_renull_using_col(mtx_matrix_t mtx, int32 org,
                          struct element_t **arr)
{
   struct element_t *elt;

   for( elt=mtx->hdr.col[org]; NOTNULL(elt); elt=elt->next.row )
      arr[elt->row] = NULL;
}

void mtx_renull_all(mtx_matrix_t mtx, struct element_t **arr)
/**
 ***  Makes arr NULLed again, assuming it is size mtx->order.
 **/
{
  if (mtx && arr)
    zero(arr,mtx->order,struct element_t *);
}

#undef __MTX_C_SEEN__
