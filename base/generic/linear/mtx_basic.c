/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
	Copyright (C) 1995 Benjamin Andrew Allan, Kirk Andre' Abbott
	Copyright (C) 1996 Benjamin Andrew Allan
	Copyright (C) 2007 Carnegie Mellon University

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
	Basic operations with mtx matrix types, including file i/o.
*//*
	mtx: Ascend Sparse Matrix Package
	by Karl Michael Westerberg, Created: 2/6/90
	last in CVS: $Revision: 1.20 $ $Date: 2000/01/25 02:27:07 $ $Author: ballan $
*/

#include <math.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/mem.h>
#include "mtx.h"
#ifdef ASC_WITH_MMIO
# include <mmio.h>
#endif

/* grab our private parts */
#define __MTX_C_SEEN__
#include "mtx_use_only.h"
#include <general/mathmacros.h>

/**
	*Really* check the matrix.
	@return an error count
*/
int super_check_matrix( mtx_matrix_t mtx){
  int32 ndx,errcnt=0;
  int32 rowc,colc;

  /* Test consistency of permutation arrays */
  for( ndx=ZERO ; ndx < mtx->capacity ; ++ndx ) {
    if( mtx->perm.row.org_to_cur[mtx->perm.row.cur_to_org[ndx]] != ndx ) {
      
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Permutation violation in row %d.\n", ndx);
        errcnt++;
    }
    if( mtx->perm.col.org_to_cur[mtx->perm.col.cur_to_org[ndx]] != ndx ) {
      
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Permutation violation in col %d.\n",ndx);
      errcnt++;
    }
  }

  if( mtx->order > mtx->capacity ) {
    
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Capacity %d is less than order %d\n",
      mtx->capacity,mtx->order);
    errcnt++;
  }

  /* Make sure rows and columns which don't exist are blank */
  for( ndx = mtx->order ; ndx < mtx->capacity ; ++ndx ) {
    int32 org_row,org_col;
    org_row = mtx->perm.row.cur_to_org[ndx];
    org_col = mtx->perm.col.cur_to_org[ndx];
    if( NOTNULL(mtx->hdr.row[org_row]) ) {
      
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Non-zeros found in non-existent row %d.\n",ndx);
      errcnt++;
    }
    if( NOTNULL(mtx->hdr.col[org_col]) ) {
      
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Non-zeros found in non-existent col %d.\n",ndx);
      errcnt++;
    }
  }

  /* Verify fishnet structure */
  for( ndx=rowc=colc=ZERO ; ndx < mtx->capacity ; ++ndx ) {
    struct element_t *elt;
    int32 org_row,org_col;
    org_row = mtx->perm.row.cur_to_org[ndx];
    org_col = mtx->perm.col.cur_to_org[ndx];
    for( elt = mtx->hdr.row[org_row] ; NOTNULL(elt) ; elt = elt->next.col ) {
      if( elt->row != mtx_NONE ) {
        ++rowc;
        if( elt->row != org_row ) {
          
          ERROR_REPORTER_HERE(ASC_PROG_ERR,"Element mismatch in row %d.\n", ndx);
          errcnt++;
        }
      } else {
        
        ERROR_REPORTER_HERE(ASC_PROG_ERR,"Disclaimed element in row %d.\n", ndx);
        errcnt++;
      }
    }
    for( elt = mtx->hdr.col[org_col] ; NOTNULL(elt) ; elt = elt->next.row ) {
      if( elt->col != mtx_NONE ) {
        ++colc;
        if( elt->col != org_col ) {
          
          ERROR_REPORTER_HERE(ASC_PROG_ERR,"Element mismatch in col %d.\n", ndx);
          errcnt++;
        }
      } else {
        
        ERROR_REPORTER_HERE(ASC_PROG_ERR,"Disclaimed element in col %d.\n", ndx);
        errcnt++;
      }
    }
  }
  if( rowc != colc ) {
    
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Non-zero discrepancy, %d by row, %d by col.\n",
      rowc, colc);
    errcnt++;
  }
  return errcnt;
}

/**
	Checks the integrity flag of the matrix.
*/
boolean check_matrix( mtx_matrix_t mtx, char *file, int line){

   if( ISNULL(mtx) ) {
      
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"NULL matrix in %s line %d.\n",file,line);
      return 0;
   }

   switch( mtx->integrity ) {
      case OK:
         break;
      case DESTROYED:
         
         FPRINTF(g_mtxerr,
           "        Matrix deceased found in %s line %d.\n",file,line);
         return 0;
      default:
         
         ERROR_REPORTER_HERE(ASC_PROG_ERR,"Matrix garbage found in %s line %d.\n",
           file,line);
         return 0;
   }

#if MTX_DEBUG
   super_check_matrix(mtx);
#endif

   if (ISSLAVE(mtx)) {
     return (check_matrix(mtx->master,file,line));
   }
   return 1;
}

/* 
	this function checks the consistency of a sparse as best it can.
	@return FALSE if something wierd
*/
boolean check_sparse(const mtx_sparse_t * const sp, char *file, int line)
{
  if ( ISNULL(sp) ||
       sp->len > sp->cap ||
       ( sp->cap > 0 && ( ISNULL(sp->idata) || ISNULL(sp->data) ) )
     ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Inconsistent or NULL sparse in %s line %d.\n",file,line);
    return 0;
  }
  return 1;
}

#define free_unless_null(ptr) if( NOTNULL(ptr) ) ascfree(ptr)
/*
static void free_unless_null(POINTER ptr)
{
   if( NOTNULL(ptr) )
      ascfree(ptr);
}
*/

void mtx_zero_int32(int32 *data, int len){
  int i;
  if (data==NULL) return;
  for (i=0; i<len; i++) data[i]=0;
}

void mtx_zero_real64(real64 *data, int len){
  int i;
  if (data==NULL) return;
  for (i=0; i<len; i++) data[i]=0.0;
}

void mtx_zero_ptr(void **data, int len){
  int i;
  if (data==NULL) return;
  for (i=0; i<len; i++) data[i]=NULL;
}

/*
	Allocates a matrix header and returns a pointer to it.
*/
static mtx_matrix_t alloc_header(void){
   mtx_matrix_t mtx;
   mtx = (mtx_matrix_t)ascmalloc( sizeof(struct mtx_header) );
   mtx->integrity = OK;
   return(mtx);
}

/**
	Frees a matrix header.
*/
static void free_header(mtx_matrix_t mtx){
   mtx->integrity = DESTROYED;
   ascfree(mtx);
}

/**
	Allocates a zeroed row or column header.
*/
static struct element_t **calloc_nz_header(int32 cap){
   return(cap > 0 ? ASC_NEW_ARRAY_CLEAR(struct element_t *,cap) : NULL);
}

/**
	Copies srchdr to tarhdr given the capacity of srchdr.
*/
static void copy_nz_header(struct element_t **tarhdr,
			 struct element_t **srchdr, int32 cap
){
   mem_copy_cast(srchdr,tarhdr,cap*sizeof(struct element_t *));
}

#define free_nz_header(hdr) free_unless_null( (POINTER)(hdr) )
/**
	Frees a row or column header
 **/


/********************************************************************/
/*
Any method of deallocating elements that doesn't use
delete_from_row/col or nuke_fishnet is DOOMED to die.
On ANY entry to delete_from_row or delete_from_col,
last_value_matrix should be already set to the mtx being deleted from.
What this really means is that before any of the following list of functions
is called, the last_value_matrix should be set:

disclaim element
delete_from_row/col
blast_row/col
clean_row/col

Note that the above functions call each other, with blast_/clean_ being
the top housekeeping/garbage collection pair. Users of delete, blast or
clean should set last_value_matrix at the scope where the matrix is known.

Excuses for this kluge:
1) mtx_value/set_value need it.
2) the mem_store scheme REQUIRES it.
3) it is faster than passing the mtx
up and down through the functions listed above, and we do a LOT of
these operations.
*/
static mtx_matrix_t last_value_matrix = NULL;

/**
	Indicates that one of the row or column (it doesn't matter) in which
	this element appears will no longer point to this element (it has
	already been removed from the list).  Elements disclaimed once have
	their row,col indices set to mtx_NONE.  Elements disclaimed twice are
	discarded.
	NOTE WELL: last_value_mtx must be set before this function is called.
*/
static void disclaim_element(struct element_t *element){
  if( element->row == mtx_NONE ) {
    mem_free_element(last_value_matrix->ms,(void *)element);
  } else {
    element->row = element->col = mtx_NONE;
  }
}

/**
	Deletes the element from the row it is in, given a pointer to
	the row link which leads to this element.
	On return the pointer pointed to by link points to the new next element.
	NOTE WELL: last_value_mtx must be set before this function is called.
*/
static void delete_from_row(struct element_t **link){
   struct element_t *p;
   p = *link;
   *link = p->next.col;
   disclaim_element(p);
   /* conservatively cause mtx_set_value to forget */
   last_value_matrix->last_value = NULL;
}

/**
	Deletes the element from the col it is in, given a pointer to
	the col link which leads to this element.
	NOTE WELL: last_value_mtx must be set before this function is called.
*/
static void delete_from_col(struct element_t **link){
   struct element_t *p;
   p = *link;
   *link = p->next.row;
   disclaim_element(p);
   /* conservatively cause mtx_set_value to forget */
   last_value_matrix->last_value = NULL;
}

/**
	Deletes every element even remotely associated with a matrix
	and nulls the headers. Also nulls the headers of slaves
	and cleans out their elements.
*/
static void nuke_fishnet(mtx_matrix_t mtx){
  int32 i;

  if ( ISNULL(mtx) || ISNULL(mtx->hdr.row) ||
       ISNULL(mtx->hdr.col) || ISSLAVE(mtx)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called with bad matrix.");
    return;
  }
  if (mtx->capacity<1) return;
  mtx->last_value = NULL;
  mem_clear_store(mtx->ms);
  zero(mtx->hdr.row,mtx->capacity,struct element_t *);
  zero(mtx->hdr.col,mtx->capacity,struct element_t *);
  for (i = 0; i < mtx->nslaves; i++) {
    zero(mtx->slaves[i]->hdr.row,mtx->capacity,struct element_t *);
    zero(mtx->slaves[i]->hdr.col,mtx->capacity,struct element_t *);
  }
}

/**
	Destroys all elements in the given row with (current)
	col index in the given col range: if rng == mtx_ALL_COLS,
	entire row is destroyed.
	NOTE WELL: last_value_mtx must be set before this function is called.
*/
static void blast_row( mtx_matrix_t mtx, int32 org, mtx_range_t *rng){
  struct element_t **link;

  link = &(mtx->hdr.row[org]);
  if( rng == mtx_ALL_COLS ) {
    while( NOTNULL(*link) )
      delete_from_row(link);
  } else {
    int32 *tocur = mtx->perm.col.org_to_cur;
    int32 col;
    while( NOTNULL(*link) ) {
      col = (*link)->col;
      if( col == mtx_NONE || in_range(rng,tocur[col]) )
        delete_from_row(link);
      else
        link = &((*link)->next.col);
    }
  }
}

/**
	Destroys all elements in the given col with (current)
	row index in the given row range: if rng == mtx_ALL_ROWS,
	entire col is destroyed.
	NOTE WELL: last_value_mtx must be set before this function is called.
*/
static void blast_col( mtx_matrix_t mtx, int32 org, mtx_range_t *rng){
  struct element_t **link;

  link = &(mtx->hdr.col[org]);
  if( rng == mtx_ALL_ROWS ) {
    while( NOTNULL(*link) )
      delete_from_col(link);
  } else {
    int32 *tocur = mtx->perm.row.org_to_cur, row;

    while( NOTNULL(*link) ) {
      row = (*link)->row;
      if( row == mtx_NONE || in_range(rng,tocur[row]) )
        delete_from_col(link);
      else
        link = &((*link)->next.row);
    }
  }
}

/**
	Disclaims all elements in the given row which have already been
	disclaimed once.
	NOTE WELL: last_value_mtx must be set before this function is called.
*/
static void clean_row(mtx_matrix_t mtx, int32 org){
  struct element_t **link;

  link = &(mtx->hdr.row[org]);
  while( NOTNULL(*link) )
    if( (*link)->row == mtx_NONE )
      delete_from_row(link);
    else
      link = &((*link)->next.col);
}

/**
	Disclaims all elements in the given col which have already been
	disclaimed once.
	NOTE WELL: last_value_mtx must be set before this function is called.
*/
static void clean_col(mtx_matrix_t mtx, int32 org){
  struct element_t **link;

  link = &(mtx->hdr.col[org]);
  while( NOTNULL(*link) )
    if( (*link)->col == mtx_NONE )
      delete_from_col(link);
    else
      link = &((*link)->next.row);
}
/********************************************************************/

mtx_coord_t *mtx_coord(mtx_coord_t *coordp, int32 row, int32 col){
   coordp->row = row;
   coordp->col = col;
   return(coordp);
}

mtx_range_t *mtx_range(mtx_range_t *rangep, int32 low, int32 high){
   rangep->low = low;
   rangep->high = high;
   return(rangep);
}

mtx_region_t *mtx_region( mtx_region_t *regionp, int32 rowlow, int32 rowhigh,
			int32 collow, int32 colhigh
){
   mtx_range(&(regionp->row),rowlow,rowhigh);
   mtx_range(&(regionp->col),collow,colhigh);
   return(regionp);
}

static int g_mtx_debug_redirect = 0;

void mtx_debug_redirect_freeze(){
  g_mtx_debug_redirect = 1;
}

static void mtx_redirectErrors(FILE *f){
  if (!g_mtx_debug_redirect) {
    assert(f != NULL);
    g_mtxerr = f;
  }
}

mtx_matrix_t mtx_create(void){
  mtx_matrix_t mtx;

  mtx_redirectErrors(stderr); /* call mtx_debug_redirect_freeze() to bypass */

  mtx = alloc_header();
  mtx->order = ZERO;
  mtx->capacity = ZERO;
  mtx->hdr.row = mtx->hdr.col = NULL;
  mtx->perm.row.org_to_cur = mtx_alloc_perm(ZERO);
  mtx->perm.row.cur_to_org = mtx_alloc_perm(ZERO);
  mtx->perm.row.parity = EVEN;
  mtx->perm.col.org_to_cur = mtx_alloc_perm(ZERO);
  mtx->perm.col.cur_to_org = mtx_alloc_perm(ZERO);
  mtx->perm.col.parity = EVEN;
  mtx->perm.transpose = mtx_NONE;
  mtx->data =
    (struct structural_data_t *)ascmalloc(sizeof(struct structural_data_t));
  mtx->data->symbolic_rank = -1;
  mtx->data->nblocks = -1;
  mtx->data->block = NULL;
  mtx->master = NULL;
  mtx->nslaves = ZERO;
  mtx->slaves = NULL;
  mtx->last_value = NULL;
  mtx->ms = mem_create_store(LENMAGIC, WIDTHMAGIC/sizeof(struct element_t) - 1,
              sizeof(struct element_t),10,2048);
  return(mtx);
}

/**
 ** Slave headers share the master's:
 ** perm.anything, data->anything, ms->anything, order and capacity.
 ** Unique to the slave are:
 ** hdr.anything
 ** last_value.
 ** All slaves of a master appear as pointers in the masters slaves list
 ** which is nslaves long. slaves do not have slaves.
 **/
mtx_matrix_t mtx_create_slave(mtx_matrix_t master){
  mtx_matrix_t mtx, *mlist;
  int32 newcnt;
  mtx_redirectErrors(stderr); /* call mtx_debug_redirect_freeze() to bypass */
  if(!mtx_check_matrix(master) || ISSLAVE(master)) return NULL;

  mtx = alloc_header();
  if (ISNULL(mtx)) return mtx;
  mtx->order = master->order;
  mtx->capacity = master->capacity;
  mtx->nslaves = ZERO;
  mtx->hdr.row = calloc_nz_header(master->capacity);
  if (ISNULL(mtx->hdr.row)) {
    ascfree(mtx);
    FPRINTF(g_mtxerr,"mtx_create_slave: Insufficient memory.\n");
    return NULL;
  }
  mtx->hdr.col = calloc_nz_header(master->capacity);
  if (ISNULL(mtx->hdr.col)) {
    ascfree(mtx->hdr.row);
    ascfree(mtx);
    FPRINTF(g_mtxerr,"mtx_create_slave: Insufficient memory.\n");
    return NULL;
  }
  mtx->perm.row.org_to_cur = master->perm.row.org_to_cur;
  mtx->perm.row.cur_to_org = master->perm.row.cur_to_org;
  mtx->perm.row.parity = EVEN; /* dont look at this again*/
  mtx->perm.col.org_to_cur = master->perm.col.org_to_cur;
  mtx->perm.col.cur_to_org = master->perm.col.cur_to_org;
  mtx->perm.col.parity = EVEN; /* dont look at this again*/
  mtx->perm.transpose = mtx_NONE; /* dont look at this again*/
  mtx->slaves = NULL;
  mtx->data = master->data;
  mtx->master = master;
  mtx->last_value = NULL;
  mtx->ms = master->ms;
  newcnt = master->nslaves + 1;
  if (newcnt == 1) {
    mlist = (mtx_matrix_t *)ascmalloc(sizeof(mtx_matrix_t));
  } else {
    mlist =
      (mtx_matrix_t *)ascrealloc(master->slaves,newcnt*sizeof(mtx_matrix_t));
  }
  if (ISNULL(mlist)) {
    ascfree(mtx->hdr.row);
    ascfree(mtx->hdr.col);
    ascfree(mtx);
    FPRINTF(g_mtxerr,"mtx_create_slave: Insufficient memory.\n");
    return NULL;
  } else {
    master->slaves = mlist;
    master->slaves[master->nslaves] = mtx;
    master->nslaves = newcnt;
  }
  return(mtx);
}

static void destroy_slave(mtx_matrix_t master, mtx_matrix_t mtx){
  int32 sid,i;

  if(!mtx_check_matrix(mtx) || !mtx_check_matrix(master)) {
    FPRINTF(g_mtxerr,
      "destroy_slave(mtx.c) called with corrupt slave or master\n");
    return;
  }
  if (ISSLAVE(master) || !ISSLAVE(mtx)) {
    FPRINTF(g_mtxerr,
      "destroy_slave(mtx.c) called with mismatched slave/master pair\n");
    return;
  }
  /* get slave index */
  sid = -1;
  for (i=0; i<master->nslaves; i++) {
    if (master->slaves[i] == mtx) {
      sid = i;
    }
  }
  if (sid < 0) {
    FPRINTF(g_mtxerr,
      "destroy_slave(mtx.c) called with mismatched slave/master pair\n");
    return;
  }
  /* move the rest of the slaves up the list */
/* old implementation - possible undefined operation on i - new code follows - JDS
  for (i = sid; i < master->nslaves -1;) {
    master->slaves[i] = master->slaves[++i];
  }
*/
  for (i = sid; i < master->nslaves -1; ++i) {
    master->slaves[i] = master->slaves[i+1];
  }
  (master->nslaves)--;
  /* we will not realloc smaller. */
  if (master->nslaves == 0 && NOTNULL(master->slaves)) {
     ascfree(master->slaves);
     master->slaves = NULL;
  }
  mtx_clear_region(mtx,mtx_ENTIRE_MATRIX);
  free_nz_header(mtx->hdr.row);
  free_nz_header(mtx->hdr.col);
  free_header(mtx);
}

void mtx_destroy(mtx_matrix_t mtx){
  int32 i;
  if(!mtx_check_matrix(mtx)) return;
  if (ISSLAVE(mtx)) {
    destroy_slave(mtx->master,mtx);
    return;
  }

  for (i=0; i<mtx->nslaves; i++) {
    if (mtx_check_matrix(mtx->slaves[i])) {
      free_nz_header(mtx->slaves[i]->hdr.row);
      free_nz_header(mtx->slaves[i]->hdr.col);
      free_header(mtx->slaves[i]);
      mtx->slaves[i] = NULL;
    } else {
      FPRINTF(g_mtxerr,
        "mtx_destroy: Corrupt slave found while destroying master.\n");
      FPRINTF(g_mtxerr,
        "             Slave %d being abandoned.\n",i);
    }
  }
  mtx->nslaves = 0;

  mtx_clear_region(mtx,mtx_ENTIRE_MATRIX);
  /* mtx_check_matrix is called again if MTX_DEBUG*/
  /**
  	The fishnet structure is
  	destroyed, just leaving
  	the headers and maybe ms.
   **/
  mem_destroy_store(mtx->ms);

  free_nz_header(mtx->hdr.row);
  free_nz_header(mtx->hdr.col);

  mtx_free_perm(mtx->perm.row.org_to_cur);
  mtx_free_perm(mtx->perm.row.cur_to_org);
  mtx_free_perm(mtx->perm.col.org_to_cur);
  mtx_free_perm(mtx->perm.col.cur_to_org);

  if( NOTNULL(mtx->data->block) ) {
    ascfree(mtx->data->block);
  }
  ascfree(mtx->data);

  free_header(mtx);
}

mtx_sparse_t *mtx_create_sparse(int32 cap){
  mtx_sparse_t *ret;
  ret = (mtx_sparse_t *)ascmalloc(sizeof(mtx_sparse_t));
  if (ISNULL(ret)) {
    FPRINTF(g_mtxerr,"ERROR: (mtx_create_sparse) Insufficient memory.\n");
    return ret;
  }
  ret->data = ASC_NEW_ARRAY(real64,cap);
  if (ISNULL(ret->data)) {
    FPRINTF(g_mtxerr,"ERROR: (mtx_create_sparse) Insufficient memory.\n");
    ascfree(ret);
    return NULL;
  }
  ret->idata = ASC_NEW_ARRAY(int32,cap);
  if (ISNULL(ret->idata)) {
    FPRINTF(g_mtxerr,"ERROR: (mtx_create_sparse) Insufficient memory.\n");
    ascfree(ret->data);
    ascfree(ret);
    return NULL;
  }
  ret->cap = cap;
  ret->len = 0;
  return ret;
}

void mtx_destroy_sparse(mtx_sparse_t *ret){
  if (ISNULL(ret)) return;
  if (NOTNULL(ret->idata)) ascfree(ret->idata);
  if (NOTNULL(ret->data)) ascfree(ret->data);
  ascfree(ret);
}

void mtx_destroy_blocklist(mtx_block_t *bl){
  if (ISNULL(bl)) return;
  if (NOTNULL(bl->block) && bl->nblocks>0) ascfree(bl->block);
  ascfree(bl);
}

mtx_matrix_t mtx_duplicate_region(mtx_matrix_t mtx
		, mtx_region_t *reg
		, real64 drop
){
  mtx_matrix_t new;
  int32 org_row;
  struct element_t *elt;

  if (!mtx_check_matrix(mtx) ||
      (mtx->master != NULL && !mtx_check_matrix(mtx->master))) {
    return NULL;
  }
  drop = fabs(drop);
  if (ISSLAVE(mtx)) {
    new = mtx_create_slave(mtx->master);
  } else {
    new = mtx_create_slave(mtx);
  }
  if (new == NULL) {
    FPRINTF(g_mtxerr,"ERROR: (mtx_duplicate_region) Insufficient memory.\n");
    return NULL;
  }
  if (reg==mtx_ENTIRE_MATRIX){
    if (drop >0.0) { /* copy with drop tolerance */
      for( org_row=0 ; org_row < mtx->order ; ++org_row ) {
        for( elt = mtx->hdr.row[org_row] ;
             NOTNULL(elt) ; elt = elt->next.col ) {
          if( elt->row != mtx_NONE  && fabs(elt->value) > drop) {
            mtx_create_element_value(new,elt->row,elt->col,elt->value);
          }
        }
      }
    } else {
      for( org_row=0 ; org_row < mtx->order ; ++org_row ) {
        for( elt = mtx->hdr.row[org_row] ;
             NOTNULL(elt) ; elt = elt->next.col ) {
          if( elt->row != mtx_NONE ) {
            mtx_create_element_value(new,elt->row,elt->col,elt->value);
          }
        }
      }
    }
  } else {
    int32 *rowcur, *colcur, row,col;
    mtx_range_t *colrng,*rowrng;
    rowcur=mtx->perm.row.org_to_cur;
    colcur=mtx->perm.col.org_to_cur;
    rowrng=&(reg->row);
    colrng=&(reg->col);
    if (drop >0.0) { /* copy with drop tolerance */
      for( org_row=0 ; org_row < mtx->order ; ++org_row ) {
        row=rowcur[org_row];
        if (in_range(rowrng,row)) {
          for( elt = mtx->hdr.row[org_row] ;
               NOTNULL(elt) ; elt = elt->next.col ) {
            col=colcur[elt->col];
            if( in_range(colrng,col) && elt->row != mtx_NONE &&
                fabs(elt->value) > drop ) {
                                        /* don't copy^bad^elements */
              mtx_create_element_value(new,elt->row,elt->col,elt->value);
            }
          }
        }
      }
    } else {
      for( org_row=0 ; org_row < mtx->order ; ++org_row ) {
        row=rowcur[org_row];
        if (in_range(rowrng,row)) {
          for( elt = mtx->hdr.row[org_row] ;
               NOTNULL(elt) ; elt = elt->next.col ) {
            col=colcur[elt->col];
            if( in_range(colrng,col) && elt->row != mtx_NONE ) {
                                        /* don't copy^bad^elements */
              mtx_create_element_value(new,elt->row,elt->col,elt->value);
            }
          }
        }
      }
    }
  }

  return new;
}

/* WARNING: this function assumes sufficient memory is available.
 * it needs to be check for null returns in allocs and doesn't.
 */
mtx_matrix_t mtx_copy_options(mtx_matrix_t mtx
		, boolean blocks
		, boolean incidence, mtx_region_t *reg
		, real64 drop
){
  mtx_matrix_t copy;
  int32 org_row;
  struct element_t *elt;
  int32 bnum;

  if (!mtx_check_matrix(mtx)) return NULL;
  drop = fabs(drop);

  /* create same size matrix */
  copy = alloc_header();
  copy->order = mtx->order;
  copy->capacity = mtx->capacity;
  copy->master = NULL;
  copy->nslaves = ZERO;
  copy->slaves = NULL;
  copy->last_value = NULL;

  /* copy headers and fishnet */
  copy->hdr.row = calloc_nz_header(copy->capacity);
  copy->hdr.col = calloc_nz_header(copy->capacity);
  if (incidence) {
    struct mem_statistics stat;
    int s_expool;
    mem_get_stats(&stat,mtx->ms);
    s_expool = MAX(2048,stat.elt_total/stat.str_wid);
    copy->ms = mem_create_store(LENMAGIC,
                 WIDTHMAGIC/sizeof(struct element_t) - 1,
                 sizeof(struct element_t),
                 10,s_expool-LENMAGIC);
    /* copy of a slave or master matrix will end up with an
     * initial mem_store that is the size cumulative of the
     * master and all its slaves since they share the ms.
     */
    if (reg==mtx_ENTIRE_MATRIX){
      if (drop >0.0) { /* copy with drop tolerance */
        for( org_row=0 ; org_row < mtx->order ; ++org_row ) {
          for( elt = mtx->hdr.row[org_row] ;
               NOTNULL(elt) ; elt = elt->next.col ) {
            if( elt->row != mtx_NONE  && fabs(elt->value) > drop) {
              mtx_create_element_value(copy,elt->row,elt->col,elt->value);
            }
          }
        }
      } else {
        for( org_row=0 ; org_row < mtx->order ; ++org_row ) {
          for( elt = mtx->hdr.row[org_row] ;
               NOTNULL(elt) ; elt = elt->next.col ) {
            if( elt->row != mtx_NONE ) {
              mtx_create_element_value(copy,elt->row,elt->col,elt->value);
            }
          }
        }
      }
    } else {
      int32 *rowcur, *colcur, row,col;
      mtx_range_t *colrng,*rowrng;
      rowcur=mtx->perm.row.org_to_cur;
      colcur=mtx->perm.col.org_to_cur;
      rowrng=&(reg->row);
      colrng=&(reg->col);
      if (drop >0.0) { /* copy with drop tolerance */
        for( org_row=0 ; org_row < mtx->order ; ++org_row ) {
          row=rowcur[org_row];
          if (in_range(rowrng,row)) {
            for( elt = mtx->hdr.row[org_row] ;
                 NOTNULL(elt) ; elt = elt->next.col ) {
              col=colcur[elt->col];
              if( in_range(colrng,col) && elt->row != mtx_NONE &&
                  fabs(elt->value) > drop ) {
                                          /* don't copy^bad^elements */
                mtx_create_element_value(copy,elt->row,elt->col,elt->value);
              }
            }
          }
        }
      } else {
        for( org_row=0 ; org_row < mtx->order ; ++org_row ) {
          row=rowcur[org_row];
          if (in_range(rowrng,row)) {
            for( elt = mtx->hdr.row[org_row] ;
                 NOTNULL(elt) ; elt = elt->next.col ) {
              col=colcur[elt->col];
              if( in_range(colrng,col) && elt->row != mtx_NONE ) {
                                          /* don't copy^bad^elements */
                mtx_create_element_value(copy,elt->row,elt->col,elt->value);
              }
            }
          }
        }
      }
    }
  } else {
    copy->ms = mem_create_store(LENMAGIC,
                 WIDTHMAGIC/sizeof(struct element_t) - 1,
                 sizeof(struct element_t),10,2048);
    /* copy of a slave or master matrix will end up with an
       initial mem_store that is the default size */
  }

  /* copy permutation information */
  copy->perm.row.org_to_cur = mtx_alloc_perm(copy->capacity);
  mtx_copy_perm(copy->perm.row.org_to_cur,
            mtx->perm.row.org_to_cur,copy->capacity);
  copy->perm.row.cur_to_org = mtx_alloc_perm(copy->capacity);
  mtx_copy_perm(copy->perm.row.cur_to_org,
            mtx->perm.row.cur_to_org,copy->capacity);
  copy->perm.row.parity = mtx_row_parity(mtx);

  copy->perm.col.org_to_cur = mtx_alloc_perm(copy->capacity);
  mtx_copy_perm(copy->perm.col.org_to_cur,
            mtx->perm.col.org_to_cur,copy->capacity);
  copy->perm.col.cur_to_org = mtx_alloc_perm(copy->capacity);
  mtx_copy_perm(copy->perm.col.cur_to_org,
            mtx->perm.col.cur_to_org,copy->capacity);
  copy->perm.col.parity = mtx_col_parity(mtx);

  /* copy (or not) block data */
  copy->data =
    (struct structural_data_t *)ascmalloc(sizeof(struct structural_data_t));
  if (blocks) {
    copy->data->symbolic_rank = mtx->data->symbolic_rank;
    copy->data->nblocks = mtx->data->nblocks;
    copy->data->block = mtx->data->nblocks > 0 ? (mtx_region_t *)
      ascmalloc( mtx->data->nblocks*sizeof(mtx_region_t) ) : NULL;
    for( bnum=0; bnum < mtx->data->nblocks; bnum++ ) {
      copy->data->block[bnum].row.low = mtx->data->block[bnum].row.low;
      copy->data->block[bnum].row.high = mtx->data->block[bnum].row.high;
      copy->data->block[bnum].col.low = mtx->data->block[bnum].col.low;
      copy->data->block[bnum].col.high = mtx->data->block[bnum].col.high;
    }
  } else {
    copy->data->symbolic_rank=0;
    copy->data->nblocks=0;
    copy->data->block =NULL;
  }

  return(copy);
}

int32 mtx_order( mtx_matrix_t mtx){
   if (!mtx_check_matrix(mtx)) return -1;
   return(mtx->order);
}

int32 mtx_capacity( mtx_matrix_t mtx){
   if (!mtx_check_matrix(mtx)) return -1;
   return(mtx->capacity);
}

/* this is only to be called from in mtx_set_order */
static void trim_incidence(mtx_matrix_t mtx, int32 order){
   int32 ndx;
   int32 *toorg;

   last_value_matrix = mtx;
   toorg = mtx->perm.col.cur_to_org;
   for( ndx = order ; ndx < mtx->order ; ++ndx )
     blast_col(mtx,toorg[ndx],mtx_ALL_ROWS);
   for( ndx = ZERO ; ndx < order ; ++ndx )
     clean_row(mtx,toorg[ndx]);

   toorg = mtx->perm.row.cur_to_org;
   for( ndx = order ; ndx < mtx->order ; ++ndx )
     blast_row(mtx,toorg[ndx],mtx_ALL_COLS);
   for( ndx = ZERO ; ndx < order ; ++ndx )
     clean_col(mtx,toorg[ndx]);
}

/* this is only to be called from in mtx_set_order */
static void enlarge_nzheaders(mtx_matrix_t mtx, int32 order){
  struct element_t **newhdr;

  /* realloc does not initialize, so calloc is the best we can do here */

  newhdr = calloc_nz_header(order);
  copy_nz_header(newhdr,mtx->hdr.row,mtx->capacity);
  free_nz_header(mtx->hdr.row);
  mtx->hdr.row = newhdr;

  newhdr = calloc_nz_header(order);
  copy_nz_header(newhdr,mtx->hdr.col,mtx->capacity);
  free_nz_header(mtx->hdr.col);
  mtx->hdr.col = newhdr;
}

/**
	This function will preserve the fact that all of the arrays are
	"correct" out to the capacity of the arrays, not just out to the order
	of the matrix.  In other words, extensions to orders which still do
	not exceed the capacity of the arrays are trivial.
*/
void mtx_set_order( mtx_matrix_t mtx, int32 order){
  int32 i;
  if(!mtx_check_matrix(mtx)) return;
  if (ISSLAVE(mtx)) {
    mtx_set_order(mtx->master,order);
    return;
  }
  /* we now have a master matrix */
  if( order < mtx->order ) {   /* Truncate */
    trim_incidence(mtx,order); /* clean master */
    for (i = 0; i < mtx->nslaves; i++) {
      trim_incidence(mtx->slaves[i],order);
    }
  }
  if (mtx->perm.transpose == mtx_NONE) {
    mtx->perm.transpose = 0;
  }

  if( order > mtx->capacity ) {
    int32 *newperm;
    int32 ndx;

    enlarge_nzheaders(mtx,order);
    for (i = 0; i < mtx->nslaves; i++) {
      enlarge_nzheaders(mtx->slaves[i],order);
    }

    /* realloc not in order here.  Happens only on the master. */
    newperm = mtx_alloc_perm(order);
    mtx_copy_perm(newperm,mtx->perm.row.org_to_cur,mtx->capacity);
    for( ndx=mtx->capacity ; ndx < order ; ++ndx )
       newperm[ndx] = ndx;
    mtx_free_perm(mtx->perm.row.org_to_cur);
    mtx->perm.row.org_to_cur = newperm;

    newperm = mtx_alloc_perm(order);
    mtx_copy_perm(newperm,mtx->perm.row.cur_to_org,mtx->capacity);
    for( ndx=mtx->capacity ; ndx < order ; ++ndx )
       newperm[ndx] = ndx;
    mtx_free_perm(mtx->perm.row.cur_to_org);
    mtx->perm.row.cur_to_org = newperm;

    newperm = mtx_alloc_perm(order);
    mtx_copy_perm(newperm,mtx->perm.col.org_to_cur,mtx->capacity);
    for( ndx=mtx->capacity ; ndx < order ; ++ndx )
       newperm[ndx] = ndx;
    mtx_free_perm(mtx->perm.col.org_to_cur);
    mtx->perm.col.org_to_cur = newperm;

    newperm = mtx_alloc_perm(order);
    mtx_copy_perm(newperm,mtx->perm.col.cur_to_org,mtx->capacity);
    for( ndx=mtx->capacity ; ndx < order ; ++ndx )
       newperm[ndx] = ndx;
    mtx_free_perm(mtx->perm.col.cur_to_org);
    mtx->perm.col.cur_to_org = newperm;

    mtx->capacity = order;
    for (i = 0; i < mtx->nslaves; i++) {
      mtx->slaves[i]->capacity = order;
      /* point slaves at master perm again in case anything has changed */
      mtx->slaves[i]->perm.row.org_to_cur = mtx->perm.row.org_to_cur;
      mtx->slaves[i]->perm.row.cur_to_org = mtx->perm.row.cur_to_org;
      mtx->slaves[i]->perm.col.org_to_cur = mtx->perm.col.org_to_cur;
      mtx->slaves[i]->perm.col.cur_to_org = mtx->perm.col.cur_to_org;
    }
  }
  mtx->order = order;
  for (i = 0; i < mtx->nslaves; i++) {
    mtx->slaves[i]->order = order;
  }
}

void mtx_clear_coord(mtx_matrix_t mtx, int32 row, int32 col){
   struct element_t **rlink, **clink;
   int32 org_row, org_col;

#if MTX_DEBUG
   if( !mtx_check_matrix(mtx) ) return; /*ben*/
#endif

   last_value_matrix = mtx;
   org_row = mtx->perm.row.cur_to_org[row];
   org_col = mtx->perm.col.cur_to_org[col];
   rlink = &(mtx->hdr.row[org_row]);
   for( ; NOTNULL(*rlink) && (*rlink)->col != org_col ; )
      rlink = &((*rlink)->next.col);
   if( ISNULL(*rlink) ) return;
   clink = &(mtx->hdr.col[org_col]);
   for( ; NOTNULL(*clink)  && (*clink)->row != org_row ; )
      clink = &((*clink)->next.row);
   delete_from_row(rlink);
   delete_from_col(clink);
}

void mtx_clear_row( mtx_matrix_t mtx, int32 row, mtx_range_t *rng){
   struct element_t **rlink, **clink;
   int32 org_row;

#if MTX_DEBUG
   if( !mtx_check_matrix(mtx) ) return; /*ben*/
#endif

   last_value_matrix = mtx;
   org_row = mtx->perm.row.cur_to_org[row];
   rlink = &(mtx->hdr.row[org_row]);
   if( rng == mtx_ALL_COLS ) {
      while( NOTNULL(*rlink)  ) {
         clink = &(mtx->hdr.col[(*rlink)->col]);
         for( ; NOTNULL(*clink) && (*clink)->row != org_row ; )
            clink = &((*clink)->next.row);
         delete_from_row(rlink);
         delete_from_col(clink);
      }
   } else if( rng->high >= rng->low ) {
      while( NOTNULL(*rlink) ) {
         if( in_range(rng,mtx->perm.col.org_to_cur[(*rlink)->col]) ) {
            clink = &(mtx->hdr.col[(*rlink)->col]);
            for( ; NOTNULL(*clink) && (*clink)->row != org_row ; )
               clink = &((*clink)->next.row);
            delete_from_row(rlink);
            delete_from_col(clink);
         } else
            rlink = &((*rlink)->next.col);
      }
   }
}

void mtx_clear_col( mtx_matrix_t mtx, int32 col, mtx_range_t *rng){
   struct element_t **clink, **rlink;
   int32 org_col;

#if MTX_DEBUG
   if( !mtx_check_matrix(mtx) ) return;
#endif

   last_value_matrix = mtx;
   org_col = mtx->perm.col.cur_to_org[col];
   clink = &(mtx->hdr.col[org_col]);
   if( rng == mtx_ALL_ROWS ) {
      while( NOTNULL(*clink) ) {
         rlink = &(mtx->hdr.row[(*clink)->row]);
         for( ; NOTNULL(*rlink) && (*rlink)->col != org_col ; )
            rlink = &((*rlink)->next.col);
         delete_from_col(clink);
         delete_from_row(rlink);
      }
   } else if( rng->high >= rng->low ) {
      while( NOTNULL(*clink) ) {
         if( in_range(rng,mtx->perm.row.org_to_cur[(*clink)->row]) ) {
            rlink = &(mtx->hdr.row[(*clink)->row]);
            for( ; NOTNULL(*rlink) && (*rlink)->col != org_col ; )
               rlink = &((*rlink)->next.col);
            delete_from_col(clink);
            delete_from_row(rlink);
         } else
            clink = &((*clink)->next.row);
      }
   }
}

void mtx_clear_region(mtx_matrix_t mtx, mtx_region_t *region){
#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif

  if( region == mtx_ENTIRE_MATRIX && !ISSLAVE(mtx)) {
    /* damn the torpedos, wipe that sucker and slaves fast */
    nuke_fishnet(mtx);
  } else {
    int32 ndx;
    int32 *toorg;
    mtx_region_t reg;

    if (region == mtx_ENTIRE_MATRIX) {
      reg.row.low = reg.col.low = 0;
      reg.row.high = reg.col.high = mtx->order-1;
    } else {
      reg = *region;
    }
    last_value_matrix = mtx;
    toorg = mtx->perm.row.cur_to_org;
    for( ndx = reg.row.low ; ndx <= reg.row.high ; ++ndx )
      blast_row(mtx,toorg[ndx],&(reg.col));
    toorg = mtx->perm.col.cur_to_org;
    for( ndx = reg.col.low ; ndx <= reg.col.high ; ++ndx )
      clean_col(mtx,toorg[ndx]);
  }
}


void mtx_reset_perm(mtx_matrix_t mtx){
  int32 ndx;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif
  if (ISSLAVE(mtx)) {
    mtx_reset_perm(mtx->master);
    return;
  }

  for( ndx=ZERO ; ndx < mtx->capacity ; ++ndx ) {
    mtx->perm.row.org_to_cur[ndx] = ndx;
    mtx->perm.row.cur_to_org[ndx] = ndx;
    mtx->perm.col.org_to_cur[ndx] = ndx;
    mtx->perm.col.cur_to_org[ndx] = ndx;
  }
  mtx->perm.row.parity = mtx->perm.col.parity = EVEN;

  mtx->data->symbolic_rank = -1;
  mtx->data->nblocks = -1;
  if( NOTNULL(mtx->data->block) ) {
    ascfree(mtx->data->block);
    mtx->data->block = NULL;
  }
}

void mtx_clear(mtx_matrix_t mtx){
  if (ISSLAVE(mtx)) {
    mtx_clear_region(mtx->master,mtx_ENTIRE_MATRIX);
  }
  mtx_clear_region(mtx,mtx_ENTIRE_MATRIX);
  mtx_reset_perm(mtx);
}

real64 mtx_value(mtx_matrix_t mtx, mtx_coord_t *coord){
   struct element_t *elt;
   register int32 org_row,org_col;

#if MTX_DEBUG
   if(!mtx_check_matrix(mtx)) return D_ZERO;
#endif
   org_row = mtx->perm.row.cur_to_org[coord->row];
   org_col = mtx->perm.col.cur_to_org[coord->col];
   elt = mtx_find_element(mtx,org_row,org_col);
   mtx->last_value = elt;
   return( ISNULL(elt) ? D_ZERO : elt->value );
}

void mtx_set_value( mtx_matrix_t mtx, mtx_coord_t *coord, real64 value){
  register int32 org_row,org_col;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif
  org_row = mtx->perm.row.cur_to_org[coord->row];
  org_col = mtx->perm.col.cur_to_org[coord->col];
  if ( NOTNULL(mtx->last_value) &&
      mtx->last_value->row==org_row &&
      mtx->last_value->col==org_col ) {
      mtx->last_value->value = value;
  } else {
    struct element_t *elt;
    if( ISNULL(elt = mtx_find_element(mtx,org_row,org_col)) ) {
      if (value != D_ZERO ) {
        elt = mtx_create_element_value(mtx,org_row,org_col,value);
      }
    } else {
      elt->value = value;
    }
  }
}

void mtx_fill_value(mtx_matrix_t mtx, mtx_coord_t *coord, real64 value){
   register int32 org_row,org_col;

#if MTX_DEBUG
   if(!mtx_check_matrix(mtx)) return;
#endif
   org_row = mtx->perm.row.cur_to_org[coord->row];
   org_col = mtx->perm.col.cur_to_org[coord->col];
   mtx_create_element_value(mtx,org_row,org_col,value);
}

void mtx_fill_org_value(mtx_matrix_t mtx
		, const mtx_coord_t *coord
		, real64 value
){
#if MTX_DEBUG
   if(!mtx_check_matrix(mtx)) return;
#endif
   mtx_create_element_value(mtx,coord->row,coord->col,value);
}

/* sparse matrix assembly of potentially duplicate fills */
/* takes a matrix, assumed to have redundant and otherwise insane incidences
  created by 'misusing mtx_fill_value' and sums all like entries, eliminating
  the duplicates and the zeroes. Returns -# of elements removed, or 1 if bad.
  returns 1 if fails for some reason, 0 otherwise.
  Could stand to have the error messages it emits improved.
  Could stand to take a rowrange or a rowlist, a colrange or a collist,droptol.
  algorithm: O(3tau)
    establish a vector mv of columns needing cleaning markers
    set lowmark=-1, highmark= -2
    for rows
      if row empty continue
      establish a vector ev of elt pointers set null
      prevlink = &header 1st elt pointer
      for each elt in row
        if elt is not marked in ev
          mark ev with elt, update prevlink
        else
          add value to elt pointed at by ev
          mark col in mv as needing cleaning, updating lowmark,highmark
          delete elt
        endif
      endfor
      for each elt in row
        unmark ev with elt
        if elt is 0, delete and mark col in mv.
      endfor
    endfor
    for i = lowmark to highmark
      clean col
    endfor
   My god, the tricks we play on a linked list.
*/
int32 mtx_assemble(mtx_matrix_t mtx){
  char *real_mv=NULL, *mv;
  struct element_t **real_ev=NULL, **ev, **prevlink, *elt;
  int32 orgrow, orgcol, lowmark,highmark,dup;
  /* note elt and prevlink could be combined, but the code is
     unreadable utterly if you do so. */

  if (ISNULL(mtx)) return 1;
  if (mtx->order <1) return 0;
  real_mv = mtx_null_mark(mtx->order+1);
  mv = real_mv+1;
  if (ISNULL(real_mv))  return 1;
  real_ev = mtx_null_vector(mtx->order+1);
  ev = real_ev+1;
  if (ISNULL(real_ev)) {
    mtx_null_mark_release();
    return 1;
  }
  /* we have allocated arrays which include a -1 element to buy
     ourselves an awful convenient lot of safety. */
  lowmark=-1;
  highmark=-2;
  dup = 0;
  last_value_matrix = mtx;
  for (orgrow=0; orgrow < mtx->order; orgrow++) {
    elt = mtx->hdr.row[orgrow];
    prevlink = &(mtx->hdr.row[orgrow]);
    while (NOTNULL(elt)) {
      if (ISNULL(ev[elt->col])) {
        ev[elt->col] = elt;         /* mark first elt found for this col */
        prevlink= &(elt->next.col); /* collect pointer to where we go next */
        elt = elt->next.col;        /* go there */
      } else {
        /* elt is duplicate and must die */
        dup++;
        ev[elt->col]->value += elt->value; /* accumulate value */
        /* update lowmark, highmark . this is a debatable implementation. */
        /* for small mods on large matrices, this is sane */
        if (lowmark > -1) { /* not first mark */
          if (elt->col < lowmark && elt->col > -1) {
            lowmark = elt->col;
          }
          if (elt->col > highmark && elt->col < mtx->order) {
            highmark = elt->col;
          }
        } else { /* very first mark */
          if (elt->col > -1 && elt->col < mtx->order) {
            lowmark = highmark = elt->col;
          }
        }
        mv[elt->col] = 1; /* mark column as to be cleaned */
        delete_from_row(prevlink);
        elt = *prevlink;
      }
    }
    elt = mtx->hdr.row[orgrow];
    prevlink = &(mtx->hdr.row[orgrow]);
    while (NOTNULL(elt)) {  /* renull the accumulator and trash 0s */
      ev[elt->col] = NULL; /* regardless, reset accum */
      if (elt->value != D_ZERO) {
        prevlink= &(elt->next.col); /* collect pointer to where we go next */
        elt = elt->next.col;        /* go there */
      } else {
        /* this is still a debatable implementation. */
        if (lowmark > -1) { /* not first mark */
          if (elt->col < lowmark && elt->col > -1) {
            lowmark = elt->col;
          }
          if (elt->col > highmark && elt->col < mtx->order) {
            highmark = elt->col;
          }
        } else { /* very first mark */
          if (elt->col > -1 && elt->col < mtx->order) {
            lowmark = highmark = elt->col;
          }
        }
        mv[elt->col] = 1; /* mark column as to be cleaned */
        delete_from_row(prevlink);
        elt = *prevlink;
      }
    }
  }
  for (orgcol = lowmark; orgcol <= highmark; orgcol++) {
    if (mv[orgcol]) {
      clean_col(mtx,orgcol); /* scrap dups and 0s */
    }
  }
  mtx_null_mark_release();
  mtx_null_vector_release();
  return -dup;
}

void mtx_del_zr_in_row(mtx_matrix_t mtx, int32 row){
   register int32 org;
   struct element_t **rlink, **clink;

#if MTX_DEBUG
   if(!mtx_check_matrix(mtx)) return;
#endif
   org = mtx->perm.row.cur_to_org[row];
   rlink = &(mtx->hdr.row[org]);

   last_value_matrix = mtx;
   while( NOTNULL(*rlink) )
      if( (*rlink)->value == D_ZERO ) {
         clink = &(mtx->hdr.col[(*rlink)->col]);
         for( ; NOTNULL(*clink) && (*clink)->row != org ; )
            clink = &((*clink)->next.row);
         delete_from_row(rlink);
         delete_from_col(clink);
      } else
         rlink = &((*rlink)->next.col);
}

void mtx_del_zr_in_col(mtx_matrix_t mtx, int32 col){
   register int32 org;
   struct element_t **clink, **rlink;

#if MTX_DEBUG
   if(!mtx_check_matrix(mtx)) return;
#endif
   org = mtx->perm.col.cur_to_org[col];
   clink = &(mtx->hdr.col[org]);

   last_value_matrix = mtx;
   while( NOTNULL(*clink) )
      if( (*clink)->value == D_ZERO ) {
         rlink = &(mtx->hdr.row[(*clink)->row]);
         for( ; NOTNULL(*rlink) && (*rlink)->col != org ; )
            rlink = &((*rlink)->next.col);
         delete_from_col(clink);
         delete_from_row(rlink);
      } else
         clink = &((*clink)->next.row);
}

void mtx_del_zr_in_rowrange(mtx_matrix_t mtx, mtx_range_t *rng){
  register int32 org,row,rowhi, *toorg;
  struct element_t **rlink, **clink;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif
  rowhi=rng->high;
  toorg= mtx->perm.row.cur_to_org;
  last_value_matrix = mtx;
  for (row=rng->low; row <=rowhi; row++) {
    org = toorg[row];
    rlink = &(mtx->hdr.row[org]);

    while( NOTNULL(*rlink) ) {
      if( (*rlink)->value == D_ZERO ) {
        clink = &(mtx->hdr.col[(*rlink)->col]);
        for( ; NOTNULL(*clink) && (*clink)->row != org ; )
          clink = &((*clink)->next.row);
        delete_from_row(rlink);
        delete_from_col(clink);
      } else {
        rlink = &((*rlink)->next.col);
      }
    }

  }
}

void mtx_del_zr_in_colrange(mtx_matrix_t mtx, mtx_range_t *rng){
  register int32 org,col,colhi, *toorg;
  struct element_t **clink, **rlink;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif
  colhi=rng->high;
  toorg= mtx->perm.col.cur_to_org;
  last_value_matrix = mtx;
  for (col=rng->low; col <=colhi; col++) {
    org = toorg[col];
    clink = &(mtx->hdr.col[org]);

    while( NOTNULL(*clink) ) {
      if( (*clink)->value == D_ZERO ) {
        rlink = &(mtx->hdr.row[(*clink)->row]);
        for( ; NOTNULL(*rlink) && (*rlink)->col != org ; )
          rlink = &((*rlink)->next.col);
        delete_from_col(clink);
        delete_from_row(rlink);
      } else {
        clink = &((*clink)->next.row);
      }
    }

  }
}

void mtx_steal_org_row_vec(mtx_matrix_t mtx, int32 row
		, real64 *vec, mtx_range_t *rng
){
  struct element_t **rlink, **clink;
  int32 org_row;

#if MTX_DEBUG
  if( !mtx_check_matrix(mtx) ) return;
#endif

  last_value_matrix = mtx;
  org_row = mtx->perm.row.cur_to_org[row];
  rlink = &(mtx->hdr.row[org_row]);
  if( rng == mtx_ALL_COLS ) {
    while( NOTNULL(*rlink)  ) {
      vec[(*rlink)->col] = (*rlink)->value;
      clink = &(mtx->hdr.col[(*rlink)->col]);
      for( ; NOTNULL(*clink) && (*clink)->row != org_row ; )
        clink = &((*clink)->next.row);
      delete_from_row(rlink);
      delete_from_col(clink);
    }
  } else if( rng->high >= rng->low ) {
    int32 *tocur;
    tocur = mtx->perm.col.org_to_cur;
    while( NOTNULL(*rlink) ) {
      if( in_range(rng,tocur[(*rlink)->col]) ) {
        vec[(*rlink)->col] = (*rlink)->value;
        clink = &(mtx->hdr.col[(*rlink)->col]);
        for( ; NOTNULL(*clink) && (*clink)->row != org_row ; )
          clink = &((*clink)->next.row);
        delete_from_row(rlink);
        delete_from_col(clink);
      } else {
         rlink = &((*rlink)->next.col);
      }
    }
  }
}

void mtx_steal_org_col_vec(mtx_matrix_t mtx, int32 col
		, real64 *vec, mtx_range_t *rng
){
  struct element_t **clink, **rlink;
  int32 org_col;

#if MTX_DEBUG
  if( !mtx_check_matrix(mtx) ) return;
#endif

  last_value_matrix = mtx;
  org_col = mtx->perm.col.cur_to_org[col];
  clink = &(mtx->hdr.col[org_col]);
  if( rng == mtx_ALL_ROWS ) {
    while( NOTNULL(*clink) ) {
      vec[(*clink)->row] = (*clink)->value;
      rlink = &(mtx->hdr.row[(*clink)->row]);
      for( ; NOTNULL(*rlink) && (*rlink)->col != org_col ; )
        rlink = &((*rlink)->next.col);
      delete_from_col(clink);
      delete_from_row(rlink);
    }
  } else if( rng->high >= rng->low ) {
    int32 *tocur;
    tocur = mtx->perm.row.org_to_cur;
    while( NOTNULL(*clink) ) {
      if( in_range(rng,tocur[(*clink)->row]) ) {
        vec[(*clink)->row] = (*clink)->value;
        rlink = &(mtx->hdr.row[(*clink)->row]);
        for( ; NOTNULL(*rlink) && (*rlink)->col != org_col ; )
          rlink = &((*rlink)->next.col);
        delete_from_col(clink);
        delete_from_row(rlink);
      } else {
        clink = &((*clink)->next.row);
      }
    }
  }
}

void mtx_steal_cur_row_vec(mtx_matrix_t mtx, int32 row
		, real64 *vec, mtx_range_t *rng
){
  struct element_t **rlink, **clink;
  int32 org_row, *tocur;

#if MTX_DEBUG
  if( !mtx_check_matrix(mtx) ) return;
#endif

  tocur = mtx->perm.col.org_to_cur;
  last_value_matrix = mtx;
  org_row = mtx->perm.row.cur_to_org[row];
  rlink = &(mtx->hdr.row[org_row]);
  if( rng == mtx_ALL_COLS ) {
    while( NOTNULL(*rlink)  ) {
      vec[tocur[(*rlink)->col]] = (*rlink)->value;
      clink = &(mtx->hdr.col[(*rlink)->col]);
      for( ; NOTNULL(*clink) && (*clink)->row != org_row ; )
        clink = &((*clink)->next.row);
      delete_from_row(rlink);
      delete_from_col(clink);
    }
  } else if( rng->high >= rng->low ) {
    while( NOTNULL(*rlink) ) {
      if( in_range(rng,tocur[(*rlink)->col]) ) {
        vec[tocur[(*rlink)->col]] = (*rlink)->value;
        clink = &(mtx->hdr.col[(*rlink)->col]);
        for( ; NOTNULL(*clink) && (*clink)->row != org_row ; )
          clink = &((*clink)->next.row);
        delete_from_row(rlink);
        delete_from_col(clink);
      } else {
         rlink = &((*rlink)->next.col);
      }
    }
  }
}

void mtx_steal_cur_col_vec(mtx_matrix_t mtx, int32 col
		, real64 *vec, mtx_range_t *rng
){
  struct element_t **clink, **rlink;
  int32 org_col, *tocur;

#if MTX_DEBUG
  if( !mtx_check_matrix(mtx) ) return;
#endif

  tocur = mtx->perm.row.org_to_cur;
  last_value_matrix = mtx;
  org_col = mtx->perm.col.cur_to_org[col];
  clink = &(mtx->hdr.col[org_col]);
  if( rng == mtx_ALL_ROWS ) {
    while( NOTNULL(*clink) ) {
      vec[tocur[(*clink)->row]] = (*clink)->value;
      rlink = &(mtx->hdr.row[(*clink)->row]);
      for( ; NOTNULL(*rlink) && (*rlink)->col != org_col ; )
        rlink = &((*rlink)->next.col);
      delete_from_col(clink);
      delete_from_row(rlink);
    }
  } else if( rng->high >= rng->low ) {
    while( NOTNULL(*clink) ) {
      if( in_range(rng,tocur[(*clink)->row]) ) {
        vec[tocur[(*clink)->row]] = (*clink)->value;
        rlink = &(mtx->hdr.row[(*clink)->row]);
        for( ; NOTNULL(*rlink) && (*rlink)->col != org_col ; )
          rlink = &((*rlink)->next.col);
        delete_from_col(clink);
        delete_from_row(rlink);
      } else {
        clink = &((*clink)->next.row);
      }
    }
  }
}

#ifdef DELETE_THIS_UNUSED_FUNCTION
/* Little function to enlarge the capacity of a sparse to the len given.
 * New memory allocated, if any, is not initialized in any way.
 * The pointer given is not itself changed, only its data.
 * This function in no way shrinks the data in a sparse.
 * (exception: buggy realloc implementations may shrink it, ack!)
 * Data/idata values up to the old capacity (ret->cap) are preserved.
 */
static int enlarge_sparse(mtx_sparse_t *ret, int32 len){
  int32 *inew;
  real64 *dnew;
  if (ISNULL(ret)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called with NULL.\n");
    return 1;
  }
  if (len <= ret->cap || len <1) return 0; /* already big enough */

  if (ret->idata == NULL) {
    inew = ASC_NEW_ARRAY(int32,len);
  } else {
    inew = (int32 *)ascrealloc(ret->idata,sizeof(int32)*len);
  }
  if (ISNULL(inew)) {
    
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"          Insufficient memory.\n");
    return 1;
  }
  ret->idata = inew; /* dnew can still fail without losing inew memory. */

  if (ret->data == NULL) {
    dnew = ASC_NEW_ARRAY(real64,len);
  } else {
    dnew = (real64 *)ascrealloc(ret->data,sizeof(real64)*len);
  }
  if (ISNULL(dnew)) {
    
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"          Insufficient memory.\n");
    return 1;
  }
  ret->data = dnew; /* we succeeded */
  ret->cap = len;
  return 0;
}
#endif /* DELETE_THIS_UNUSED_FUNCTION */


/* going to try to make steal also handle sparse creation ...*/
/* don't you dare, whoever you are! */
boolean mtx_steal_org_row_sparse(mtx_matrix_t mtx, int32 row
		, mtx_sparse_t *sp, mtx_range_t *rng
){
  struct element_t **rlink, **clink;
  int32 org_row;
  int32 len,k;

#if MTX_DEBUG
  if( !mtx_check_matrix(mtx) ) return TRUE;
#endif
  if (sp == mtx_CREATE_SPARSE) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called with mtx_CREATE_SPARSE. Not supported.\n");
    return TRUE;
  }
  if (rng == mtx_ALL_COLS) {
    len = mtx->order;
  } else {
    len = rng->high - rng->low +1;
  }
  if (sp->cap < len) {
    sp->len = 0;
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called with sparse of insufficient capacity.\n");
    return TRUE;
  }

  last_value_matrix = mtx;
  org_row = mtx->perm.row.cur_to_org[row];
  rlink = &(mtx->hdr.row[org_row]);
  k = 0;

  if( rng == mtx_ALL_COLS ) {
    while( NOTNULL(*rlink)  ) {
      sp->idata[k] = (*rlink)->col;
      sp->data[k] = (*rlink)->value;
      k++;
      clink = &(mtx->hdr.col[(*rlink)->col]);
      for( ; NOTNULL(*clink) && (*clink)->row != org_row ; )
        clink = &((*clink)->next.row);
      delete_from_row(rlink);
      delete_from_col(clink);
    }
  } else if( rng->high >= rng->low ) {
    int32 *tocur;
    tocur = mtx->perm.col.org_to_cur;
    while( NOTNULL(*rlink) ) {
      if( in_range(rng,tocur[(*rlink)->col]) ) {
        sp->idata[k] = (*rlink)->col;
        sp->data[k] = (*rlink)->value;
        k++;
        clink = &(mtx->hdr.col[(*rlink)->col]);
        for( ; NOTNULL(*clink) && (*clink)->row != org_row ; )
          clink = &((*clink)->next.row);
        delete_from_row(rlink);
        delete_from_col(clink);
      } else {
         rlink = &((*rlink)->next.col);
      }
    }
  }
  sp->len = k;
  return FALSE;
}

boolean mtx_steal_org_col_sparse(mtx_matrix_t mtx, int32 col
		, mtx_sparse_t *sp, mtx_range_t *rng
){
  struct element_t **clink, **rlink;
  int32 org_col;
  int32 len,k;

#if MTX_DEBUG
  if( !mtx_check_matrix(mtx) ) return TRUE;
#endif
  if (sp == mtx_CREATE_SPARSE) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called with mtx_CREATE_SPARSE. Not supported.\n");
    return TRUE;
  }
  if (rng == mtx_ALL_ROWS) {
    len = mtx->order;
  } else {
    len = rng->high - rng->low +1;
  }
  if (sp->cap < len) {
    sp->len = 0;
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called with sparse of insufficient capacity.\n");
    return TRUE;
  }

  last_value_matrix = mtx;
  org_col = mtx->perm.col.cur_to_org[col];
  clink = &(mtx->hdr.col[org_col]);
  k = 0;

  if( rng == mtx_ALL_ROWS ) {
    while( NOTNULL(*clink) ) {
      sp->idata[k] = (*clink)->row;
      sp->data[k] = (*clink)->value;
      k++;
      rlink = &(mtx->hdr.row[(*clink)->row]);
      for( ; NOTNULL(*rlink) && (*rlink)->col != org_col ; )
        rlink = &((*rlink)->next.col);
      delete_from_col(clink);
      delete_from_row(rlink);
    }
  } else if( rng->high >= rng->low ) {
    int32 *tocur;
    tocur = mtx->perm.row.org_to_cur;
    while( NOTNULL(*clink) ) {
      if( in_range(rng,tocur[(*clink)->row]) ) {
        sp->idata[k] = (*clink)->row;
        sp->data[k] = (*clink)->value;
        k++;
        rlink = &(mtx->hdr.row[(*clink)->row]);
        for( ; NOTNULL(*rlink) && (*rlink)->col != org_col ; )
          rlink = &((*rlink)->next.col);
        delete_from_col(clink);
        delete_from_row(rlink);
      } else {
        clink = &((*clink)->next.row);
      }
    }
  }
  sp->len = k;
  return FALSE;
}

boolean mtx_steal_cur_row_sparse(mtx_matrix_t mtx, int32 row
		, mtx_sparse_t *sp, mtx_range_t *rng
){
  struct element_t **rlink, **clink;
  int32 org_row, *tocur;
  int32 len,k;

#if MTX_DEBUG
  if( !mtx_check_matrix(mtx) ) return TRUE;
#endif
  if (sp == mtx_CREATE_SPARSE) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called with mtx_CREATE_SPARSE. Not supported.\n");
    return TRUE;
  }
  if (rng == mtx_ALL_COLS) {
    len = mtx->order;
  } else {
    len = rng->high - rng->low +1;
  }
  if (sp->cap < len) {
    sp->len = 0;
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called with sparse of insufficient capacity.\n");
    return TRUE;
  }

  tocur = mtx->perm.col.org_to_cur;
  last_value_matrix = mtx;
  org_row = mtx->perm.row.cur_to_org[row];
  rlink = &(mtx->hdr.row[org_row]);
  k = 0;

  if( rng == mtx_ALL_COLS ) {
    while( NOTNULL(*rlink)  ) {
      sp->idata[k] = tocur[(*rlink)->col];
      sp->data[k] = (*rlink)->value;
      k++;
      clink = &(mtx->hdr.col[(*rlink)->col]);
      for( ; NOTNULL(*clink) && (*clink)->row != org_row ; )
        clink = &((*clink)->next.row);
      delete_from_row(rlink);
      delete_from_col(clink);
    }
  } else if( rng->high >= rng->low ) {
    while( NOTNULL(*rlink) ) {
      if( in_range(rng,tocur[(*rlink)->col]) ) {
        sp->idata[k] = tocur[(*rlink)->col];
        sp->data[k] = (*rlink)->value;
        k++;
        clink = &(mtx->hdr.col[(*rlink)->col]);
        for( ; NOTNULL(*clink) && (*clink)->row != org_row ; )
          clink = &((*clink)->next.row);
        delete_from_row(rlink);
        delete_from_col(clink);
      } else {
         rlink = &((*rlink)->next.col);
      }
    }
  }
  sp->len = k;
  return FALSE;
}

boolean mtx_steal_cur_col_sparse(mtx_matrix_t mtx, int32 col
		, mtx_sparse_t *sp, mtx_range_t *rng
){
  struct element_t **clink, **rlink;
  int32 org_col, *tocur;
  int32 len,k;

#if MTX_DEBUG
  if( !mtx_check_matrix(mtx) ) return TRUE;
#endif
  if (sp == mtx_CREATE_SPARSE) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called with mtx_CREATE_SPARSE. Not supported.\n");
    return TRUE;
  }
  if (rng == mtx_ALL_ROWS) {
    len = mtx->order;
  } else {
    len = rng->high - rng->low +1;
  }
  if (sp->cap < len) {
    sp->len = 0;
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called with sparse of insufficient capacity.\n");
    return TRUE;
  }

  tocur = mtx->perm.row.org_to_cur;
  last_value_matrix = mtx;
  org_col = mtx->perm.col.cur_to_org[col];
  clink = &(mtx->hdr.col[org_col]);
  k = 0;

  if( rng == mtx_ALL_ROWS ) {
    while( NOTNULL(*clink) ) {
      sp->idata[k] = tocur[(*clink)->row];
      sp->data[k] = (*clink)->value;
      k++;
      rlink = &(mtx->hdr.row[(*clink)->row]);
      for( ; NOTNULL(*rlink) && (*rlink)->col != org_col ; )
        rlink = &((*rlink)->next.col);
      delete_from_col(clink);
      delete_from_row(rlink);
    }
  } else if( rng->high >= rng->low ) {
    while( NOTNULL(*clink) ) {
      if( in_range(rng,tocur[(*clink)->row]) ) {
        sp->idata[k] = tocur[(*clink)->row];
        sp->data[k] = (*clink)->value;
        k++;
        rlink = &(mtx->hdr.row[(*clink)->row]);
        for( ; NOTNULL(*rlink) && (*rlink)->col != org_col ; )
          rlink = &((*rlink)->next.col);
        delete_from_col(clink);
        delete_from_row(rlink);
      } else {
        clink = &((*clink)->next.row);
      }
    }
  }
  sp->len = k;
  return FALSE;
}

void mtx_fill_org_row_vec(mtx_matrix_t mtx, int32 row
		, real64 *vec, mtx_range_t *rng
){
  int32 org_row,highcol, *toorg;
  register int32 org_col;
  register real64 value;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif

  org_row = mtx->perm.row.cur_to_org[row];
  if( rng == mtx_ALL_COLS ) {
    highcol=mtx->order;
    for( org_col=0 ; org_col <highcol ; org_col++ ) {
      if ((value=vec[org_col]) != D_ZERO)
        mtx_create_element_value(mtx,org_row,org_col,value);
    }
  } else {
    register int32 cur_col;

    toorg = mtx->perm.col.cur_to_org;
    highcol= rng->high;
    for ( cur_col=rng->low; cur_col<=highcol; cur_col++) {
      if ((value=vec[(org_col=toorg[cur_col])]) != D_ZERO)
        mtx_create_element_value(mtx,org_row,org_col,value);
    }
  }
}

void mtx_fill_org_col_vec(mtx_matrix_t mtx
		, int32 col, real64 *vec, mtx_range_t *rng
){
  int32 org_col,highrow, *toorg;
  register int32 org_row;
  register real64 value;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif

  org_col = mtx->perm.col.cur_to_org[col];
  if( rng == mtx_ALL_ROWS ) {
    highrow=mtx->order;
    for( org_row=0 ; org_row <highrow ; org_row++ ) {
      if ((value=vec[org_row]) != D_ZERO)
        mtx_create_element_value(mtx,org_row,org_col,value);
    }
  } else {
    register int32 cur_row;

    toorg = mtx->perm.row.cur_to_org;
    highrow= rng->high;
    for ( cur_row=rng->low; cur_row<=highrow; cur_row++) {
      if ((value=vec[(org_row=toorg[cur_row])]) != D_ZERO)
        mtx_create_element_value(mtx,org_row,org_col,value);
    }
  }
}

void mtx_fill_cur_row_vec(mtx_matrix_t mtx
		, int32 row, real64 *vec, mtx_range_t *rng
){
  int32 org_row,highcol,lowcol, *toorg;
  register int32 cur_col;
  register real64 value;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif

  org_row = mtx->perm.row.cur_to_org[row];
  toorg=mtx->perm.col.cur_to_org;
  if( rng == mtx_ALL_COLS ) {
    highcol = mtx->order-1;
    lowcol = 0;
  } else {
    highcol= rng->high;
    lowcol=rng->low;
  }
  for( cur_col=lowcol ; cur_col <= highcol ; cur_col++ ) {
    if ((value=vec[cur_col]) != D_ZERO)
      mtx_create_element_value(mtx,org_row,toorg[cur_col],value);
  }
}

void mtx_fill_cur_col_vec(mtx_matrix_t mtx
		, int32 col, real64 *vec, mtx_range_t *rng
){
  int32 org_col,highrow,lowrow, *toorg;
  register int32 cur_row;
  register real64 value;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif

  org_col = mtx->perm.col.cur_to_org[col];
  toorg=mtx->perm.row.cur_to_org;
  if( rng == mtx_ALL_ROWS ) {
    highrow=mtx->order-1;
    lowrow=0;
  } else {
    highrow= rng->high;
    lowrow=rng->low;
  }
  for( cur_row=lowrow ; cur_row <= highrow ; cur_row++ ) {
    if ((value=vec[cur_row]) != D_ZERO)
      mtx_create_element_value(mtx,toorg[cur_row],org_col,value);
  }
}

void mtx_dropfill_cur_col_vec(mtx_matrix_t mtx, int32 col,
                              real64 *vec, mtx_range_t *rng,
                              real64 tol
){
  int32 org_col,highrow,lowrow, *toorg;
  register int32 cur_row;
  register real64 value;

  if (tol==0.0) {
    mtx_fill_cur_col_vec(mtx,col,vec,rng);
    return;
  }

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif

  org_col = mtx->perm.col.cur_to_org[col];
  toorg=mtx->perm.row.cur_to_org;
  if( rng == mtx_ALL_ROWS ) {
    highrow=mtx->order-1;
    lowrow=0;
  } else {
    highrow= rng->high;
    lowrow=rng->low;
  }
  for( cur_row=lowrow ; cur_row <= highrow ; cur_row++ ) {
    if (fabs(value=vec[cur_row]) > tol)
      mtx_create_element_value(mtx,toorg[cur_row],org_col,value);
  }
}

void mtx_dropfill_cur_row_vec(mtx_matrix_t mtx, int32 row,
                              real64 *vec, mtx_range_t *rng,
                              real64 tol
){
  int32 org_row,highcol,lowcol, *toorg;
  register int32 cur_col;
  register real64 value;

  if (tol==0.0) {
    mtx_fill_cur_row_vec(mtx,row,vec,rng);
    return;
  }

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif

  org_row = mtx->perm.row.cur_to_org[row];
  toorg=mtx->perm.col.cur_to_org;
  if( rng == mtx_ALL_COLS ) {
    highcol = mtx->order-1;
    lowcol = 0;
  } else {
    highcol= rng->high;
    lowcol=rng->low;
  }
  for( cur_col=lowcol ; cur_col <= highcol ; cur_col++ ) {
    if (fabs(value=vec[cur_col]) > tol)
      mtx_create_element_value(mtx,org_row,toorg[cur_col],value);
  }
}

void mtx_fill_org_row_sparse(mtx_matrix_t mtx, int32 row,
                             const mtx_sparse_t *sp
){
  int32 orgrow,i;
#if MTX_DEBUG
  if(!mtx_check_matrix(mtx) || !mtx_check_sparse(sp)) return;
#endif
  orgrow = mtx->perm.row.cur_to_org[row];
  for (i=0; i < sp->len; i++) {
    if (sp->data[i] != D_ZERO
#if MTX_DEBUG
        && sp->idata[i] >=0 && sp->idata[i] < mtx->order
#endif
       ) {
      mtx_create_element_value(mtx,orgrow,sp->idata[i],sp->data[i]);
    }
  }
}

void mtx_fill_org_col_sparse(mtx_matrix_t mtx, int32 col,
                             const mtx_sparse_t *sp
){
  int32 orgcol,i;
#if MTX_DEBUG
  if(!mtx_check_matrix(mtx) || !mtx_check_sparse(sp)) return;
#endif
  orgcol = mtx->perm.col.cur_to_org[col];
  for (i=0; i < sp->len; i++) {
    if (sp->data[i] != D_ZERO
#if MTX_DEBUG
        && sp->idata[i] >=0 && sp->idata[i] < mtx->order
#endif
       ) {
      mtx_create_element_value(mtx,sp->idata[i],orgcol,sp->data[i]);
    }
  }
}

void mtx_fill_cur_row_sparse(mtx_matrix_t mtx, int32 row,
                             const mtx_sparse_t *sp
){
  int32 orgrow,i;
#if MTX_DEBUG
  if(!mtx_check_matrix(mtx) || !mtx_check_sparse(sp)) return;
#endif
  orgrow = mtx->perm.row.cur_to_org[row];
  for (i=0; i < sp->len; i++) {
    if (
#if MTX_DEBUG
        sp->idata[i] >=0 && sp->idata[i] < mtx->order &&
#endif
        sp->data[i] != D_ZERO) {
      mtx_create_element_value(mtx,orgrow,
                           mtx->perm.col.cur_to_org[sp->idata[i]],
                           sp->data[i]);
    }
  }
}

void mtx_fill_cur_col_sparse(mtx_matrix_t mtx, int32 col,
                             const mtx_sparse_t *sp
){
  int32 orgcol,i;
#if MTX_DEBUG
  if(!mtx_check_matrix(mtx) || !mtx_check_sparse(sp)) return;
#endif
  orgcol = mtx->perm.col.cur_to_org[col];
  for (i=0; i < sp->len; i++) {
    if (
#if MTX_DEBUG
        sp->idata[i] >=0 && sp->idata[i] < mtx->order &&
#endif
        sp->data[i] != D_ZERO) {
      mtx_create_element_value(mtx,
                           mtx->perm.col.cur_to_org[sp->idata[i]],
                           orgcol,
                           sp->data[i]);
    }
  }
}

void mtx_mult_row(mtx_matrix_t mtx, int32 row, real64 factor, mtx_range_t *rng){
   struct element_t *elt;
   int32 *tocur;

   if( factor == D_ZERO ) {
      mtx_clear_row(mtx,row,rng);
      return;
   }

   if (factor == D_ONE) return;
#if MTX_DEBUG
   if(!mtx_check_matrix(mtx)) return;
#endif

   tocur = mtx->perm.col.org_to_cur;
   elt = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];
   if( rng == mtx_ALL_COLS )
      for( ; NOTNULL(elt); elt = elt->next.col ) elt->value *= factor;
   else if( rng->high >= rng->low )
      for( ; NOTNULL(elt); elt = elt->next.col )
         if( in_range(rng,tocur[elt->col]) ) elt->value *= factor;
}

void mtx_mult_col(mtx_matrix_t mtx, int32 col, real64 factor, mtx_range_t *rng){
   struct element_t *elt;
   int32 *tocur;

   if( factor == D_ZERO ) {
      mtx_clear_col(mtx,col,rng);
      return;
   }
   if (factor == D_ONE) return;

#if MTX_DEBUG
   if(!mtx_check_matrix(mtx)) return;
#endif

   tocur = mtx->perm.row.org_to_cur;
   elt = mtx->hdr.col[mtx->perm.col.cur_to_org[col]];
   if( rng == mtx_ALL_ROWS )
      for( ; NOTNULL(elt); elt = elt->next.row ) elt->value *= factor;
   else if( rng->high >= rng->low )
      for( ; NOTNULL(elt); elt = elt->next.row )
         if( in_range(rng,tocur[elt->row]) ) elt->value *= factor;
}

void mtx_mult_row_zero(mtx_matrix_t mtx, int32 row, mtx_range_t *rng){
   struct element_t *elt;
   int32 *tocur;

#if MTX_DEBUG
   if(!mtx_check_matrix(mtx)) return;
#endif

   tocur = mtx->perm.col.org_to_cur;
   elt = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];
   if( rng == mtx_ALL_COLS )
      for( ; NOTNULL(elt); elt = elt->next.col ) elt->value = D_ZERO;
   else if( rng->high >= rng->low )
      for( ; NOTNULL(elt); elt = elt->next.col )
         if( in_range(rng,tocur[elt->col]) ) elt->value = D_ZERO;
}

void mtx_mult_col_zero(mtx_matrix_t mtx, int32 col, mtx_range_t *rng){
   struct element_t *elt;
   int32 *tocur;

#if MTX_DEBUG
   if(!mtx_check_matrix(mtx)) return;
#endif

   tocur = mtx->perm.row.org_to_cur;
   elt = mtx->hdr.col[mtx->perm.col.cur_to_org[col]];
   if( rng == mtx_ALL_ROWS )
      for( ; NOTNULL(elt); elt = elt->next.row ) elt->value = D_ZERO;
   else if( rng->high >= rng->low )
      for( ; NOTNULL(elt); elt = elt->next.row )
         if( in_range(rng,tocur[elt->row]) ) elt->value = D_ZERO;
}

void mtx_add_row(mtx_matrix_t mtx, int32 s_cur, int32 t_cur, real64 factor,
		mtx_range_t *rng
){
   register int32 org_col;
   int32 t_org,*tocur;
   struct element_t **arr,*elt;

#if MTX_DEBUG
   if(!mtx_check_matrix(mtx)) return;
#endif

   t_org = mtx->perm.row.cur_to_org[t_cur];
   if( rng == mtx_ALL_COLS ) {
      arr = mtx_expand_row(mtx,t_org);   /* Expand the target row */
      elt = mtx->hdr.row[mtx->perm.row.cur_to_org[s_cur]];
      for( ; NOTNULL(elt); elt = elt->next.col ) {
         if( ISNULL(arr[(org_col=elt->col)]) ) {
            arr[org_col] =
             mtx_create_element_value(mtx,t_org,org_col,(factor * elt->value));
         } else {
            arr[org_col]->value += factor * elt->value;
         }
      }
      mtx_renull_using_row(mtx,t_org,arr);
   } else if( rng->high >= rng->low ) {
      register int32 cur_col;

      tocur = mtx->perm.col.org_to_cur;
      arr = mtx_expand_row(mtx,t_org);   /* Expand the target row */
      elt = mtx->hdr.row[mtx->perm.row.cur_to_org[s_cur]];
      for( ; NOTNULL(elt); elt = elt->next.col ) {
         cur_col=tocur[(org_col=elt->col)];
         if( in_range(rng,cur_col) ) {
            if( NOTNULL(arr[org_col]) ) {
               arr[org_col]->value += factor * elt->value;
            } else {
              arr[org_col] =
                mtx_create_element_value(mtx,t_org,org_col,
                                         (factor * elt->value));
               /* hit rate usually lower */
            }
         }
      }
      mtx_renull_using_row(mtx,t_org,arr);
   }
   mtx_null_vector_release();
}

void mtx_add_col(mtx_matrix_t mtx, int32 s_cur, int32 t_cur, real64 factor,
		 mtx_range_t *rng
){
   int32 t_org,*tocur;
   struct element_t **arr,*elt;

#if MTX_DEBUG
   if(!mtx_check_matrix(mtx)) return;
#endif

   if( rng == mtx_ALL_ROWS ) {
      t_org = mtx->perm.col.cur_to_org[t_cur];
      arr = mtx_expand_col(mtx,t_org);   /* Expand the target col */
      elt = mtx->hdr.col[mtx->perm.col.cur_to_org[s_cur]];
      for( ; NOTNULL(elt); elt = elt->next.row )
         if( ISNULL(arr[elt->row]) ) {
            arr[elt->row] =
               mtx_create_element_value(mtx,elt->row,t_org,
                                        (factor * elt->value));
         } else arr[elt->row]->value += factor * elt->value;
      mtx_renull_using_col(mtx,t_org,arr);
   } else if( rng->high >= rng->low ) {
      tocur = mtx->perm.row.org_to_cur;
      t_org = mtx->perm.col.cur_to_org[t_cur];
      arr = mtx_expand_col(mtx,t_org);   /* Expand the target col */
      elt = mtx->hdr.col[mtx->perm.col.cur_to_org[s_cur]];
      for( ; NOTNULL(elt); elt = elt->next.row ) {
         if( in_range(rng,tocur[elt->row]) ) {
            if( NOTNULL(arr[elt->row]) ) {
               arr[elt->row]->value += factor * elt->value;
            } else {
               arr[elt->row] =
                 mtx_create_element_value(mtx,elt->row,t_org,
                                          (factor * elt->value));
            }
         }
      }
      mtx_renull_using_col(mtx,t_org,arr);
   }
   mtx_null_vector_release();
}

/**
	Expands the given row into an array of pointers, indexed on original
	col number.  The array is obtained from mtx_null_row_vector().
	Be sure to call mtx_null_row_vector_release() when done with the vector and
	you have rezeroed it.
*/
static struct element_t **mtx_expand_row_series( mtx_matrix_t mtx, int32 org){
   struct element_t **arr;
   struct element_t *elt;

   arr = mtx_null_row_vector(mtx->order);
   for( elt=mtx->hdr.row[org]; NOTNULL(elt); elt=elt->next.col )
      arr[elt->col] = elt;
   return(arr);
}

/**
	Expands the given col into an array of pointers, indexed on original
	row number.  The array is obtained from mtx_null_col_vector().
	Be sure to call mtx_null_col_vector_release() when done with the vector and
	you have rezeroed it.
*/
static struct element_t **mtx_expand_col_series(mtx_matrix_t mtx, int32 org){
   struct element_t **arr;
   struct element_t *elt;

   arr = mtx_null_col_vector(mtx->order);
   for( elt = mtx->hdr.col[org] ; NOTNULL(elt) ; elt = elt->next.row )
      arr[elt->row] = elt;
   return(arr);
}

struct add_series_data {
  mtx_matrix_t mtx;        /* matrix we're operating on */
  struct element_t **arr;  /* target row/col expansion array */
  int32 *tocur;      /* col/row permutation vector */
  int32 t_org;       /* org row/col which is expanded */
};

static struct add_series_data
  rsdata={NULL,NULL,NULL,mtx_NONE},
  csdata={NULL,NULL,NULL,mtx_NONE};

static void add_series_data_release(void){
  /* if apparently sane, and have array release the arr */
  if (NOTNULL(rsdata.mtx) && NOTNULL(rsdata.tocur)
      && rsdata.t_org >= 0 && NOTNULL(rsdata.arr)) {
    mtx_null_row_vector_release();
  }
  if (NOTNULL(csdata.mtx) && NOTNULL(csdata.tocur)
      && csdata.t_org >= 0 && NOTNULL(csdata.arr)) {
    mtx_null_col_vector_release();
  }
  /* reinit data */

  rsdata.mtx = NULL;
  rsdata.arr = NULL;
  rsdata.tocur = NULL;
  rsdata.t_org = mtx_NONE;

  csdata.mtx = NULL;
  csdata.arr = NULL;
  csdata.tocur = NULL;
  csdata.t_org = mtx_NONE;
}

void mtx_add_row_series_init(mtx_matrix_t mtx,int32 t_cur,boolean use){

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif

  if ( !(rsdata.mtx) ) {
    /* this is a grabbing call due to memory reuse */
    if ( mtx && (t_cur >= 0) && (t_cur < mtx->order) ) {
      rsdata.mtx   = mtx;
      rsdata.tocur = mtx->perm.col.org_to_cur;
      rsdata.t_org = mtx->perm.row.cur_to_org[t_cur];
      rsdata.arr   = mtx_expand_row_series(mtx,rsdata.t_org);
    } else {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"called for grab with invalid column or mtx. col number %d.\n",t_cur);
    }
    return;
  } else {
    /* this is supposed to be a releasing call */
    if (t_cur != mtx_NONE) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"called for release without mtx_NONE.\n");
      return;
    }
    if (mtx != rsdata.mtx) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"called for release with ungrabbed matrix.\n");
      return;
    }
    if (use) {
      mtx_renull_using_row(rsdata.mtx, rsdata.t_org, rsdata.arr);
    } else {
      mtx_renull_all(rsdata.mtx, rsdata.arr);
    }
    mtx_null_row_vector_release();
    rsdata.mtx   = NULL;
    rsdata.arr   = NULL;
    rsdata.tocur = NULL;
    rsdata.t_org = mtx_NONE;
  }
}

void mtx_add_row_series(int32 s_cur,real64 factor,mtx_range_t *rng){
  register int32 org_col;
  int32 t_org,*tocur;
  struct element_t **arr,*elt;
  mtx_matrix_t mtx;

  if ( !(rsdata.mtx) ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called without grabbing target row first.\n");
    return;
  }
  mtx   = rsdata.mtx;
  arr   = rsdata.arr;
  tocur = rsdata.tocur;
  t_org = rsdata.t_org;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif

  elt = mtx->hdr.row[mtx->perm.row.cur_to_org[s_cur]];
  if( rng == mtx_ALL_COLS ) {
    for( ; NOTNULL(elt); elt = elt->next.col ) {
      if( ISNULL(arr[(org_col=elt->col)]) ) {
        arr[org_col] =
          mtx_create_element_value(mtx,t_org,org_col,(factor * elt->value));
      } else {
        arr[org_col]->value += factor * elt->value;
      }
    }
    return;
  }
  /* fast_in_range is a 10% winner on the alpha, and should be even more
     on the other platforms. */
  if( rng->high >= rng->low ) {
    register int32 cur_col, lo,hi;
    lo=rng->low; hi= rng->high;
    for( ; NOTNULL(elt); elt = elt->next.col ) {
      cur_col=tocur[(org_col=elt->col)];
      if( fast_in_range(lo,hi,cur_col) ) {
        if( NOTNULL(arr[org_col]) ) {
          arr[org_col]->value += factor * elt->value;
        } else {
          arr[org_col] =
            mtx_create_element_value(mtx,t_org,org_col,(factor*elt->value));
        }
      }
    }
  }
}

void mtx_add_col_series_init(mtx_matrix_t mtx,int32 t_cur,boolean use){

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif

  if ( !(csdata.mtx) ) {
    /* this is a grabbing call */
    if ( mtx && (t_cur >= 0) && (t_cur < mtx->order) ) {
      csdata.mtx   = mtx;
      csdata.tocur = mtx->perm.row.org_to_cur;
      csdata.t_org = mtx->perm.col.cur_to_org[t_cur];
      csdata.arr   = mtx_expand_col_series(mtx,csdata.t_org);
    } else {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"called for grab with invalid row or mtx. row number %d.\n",t_cur);
    }
    return;
  } else {
    /* this is supposed to be a releasing call */
    if (t_cur != mtx_NONE) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"called for release without mtx_NONE.\n");
      return;
    }
    if (mtx != csdata.mtx) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"called for release with ungrabbed matrix.\n");
      return;
    }
    if (use) {
      mtx_renull_using_col(csdata.mtx, csdata.t_org, csdata.arr);
    } else {
      mtx_renull_all(csdata.mtx, csdata.arr);
    }
    mtx_null_col_vector_release();
    csdata.mtx   = NULL;
    csdata.arr   = NULL;
    csdata.tocur = NULL;
    csdata.t_org = mtx_NONE;
  }
}

void mtx_add_col_series(int32 s_cur,real64 factor,mtx_range_t *rng){
  register int32 org_row;
  int32 t_org,*tocur;
  struct element_t **arr,*elt;
  mtx_matrix_t mtx;

  if ( !(csdata.mtx) ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called for without grabbing target col first.\n");
    return;
  }
  mtx   = csdata.mtx;
  arr   = csdata.arr;
  tocur = csdata.tocur;
  t_org = csdata.t_org;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif

  elt = mtx->hdr.col[mtx->perm.col.cur_to_org[s_cur]];
  if( rng == mtx_ALL_ROWS ) {
    for( ; NOTNULL(elt); elt = elt->next.row ) {
      if( ISNULL(arr[(org_row=elt->row)]) ) {
        arr[org_row] =
          mtx_create_element_value(mtx,org_row,t_org,(factor * elt->value));
      } else {
        arr[org_row]->value += factor * elt->value;
      }
    }
    return;
  }
  if( rng->high >= rng->low ) {
    register int32 cur_row;

    for( ; NOTNULL(elt); elt = elt->next.row ) {
      cur_row=tocur[(org_row=elt->row)];
      if( in_range(rng,cur_row) ) {
        if( NOTNULL(arr[org_row]) ) {
          arr[org_row]->value += factor * elt->value;
        } else {
          arr[org_row] =
            mtx_create_element_value(mtx,org_row,t_org,(factor*elt->value));
        }
      }
    }
  }
}

void mtx_old_add_row_sparse(mtx_matrix_t mtx,
		int32 t_cur,   /* cur index of target row */
		real64 *s_cur, /* dense source row, curindexed */
		real64 factor, /* coefficient */
		mtx_range_t *rng,
		int32 *ilist  /* list to add over */
){
  int32 t_org,*toorg,cindex,orgcindex,hilim;
  struct element_t **arr;
  real64 value;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif

  if (factor==D_ZERO) return; /* adding 0 rather silly */

  if( rng == mtx_ALL_COLS ) {
    t_org = mtx->perm.row.cur_to_org[t_cur]; /* org of target row */
    arr = mtx_expand_row(mtx,t_org);             /* Expand the target row */
    toorg = mtx->perm.col.cur_to_org;        /* current col perm */
    hilim=mtx->order;

    if (!ilist) { /* FULL ROW CASE no ilist */
      for (cindex=0; cindex< hilim; cindex++) {
        if ((value=s_cur[cindex])!=D_ZERO) {
          if ( ISNULL(arr[(orgcindex=toorg[cindex])]) ) { /* add element */
            arr[orgcindex] =
              mtx_create_element_value(mtx,t_org,orgcindex,(factor * value));
          } else {  /* increment element */
            arr[orgcindex]->value += factor * value;
          }
        }
      }
    } else { /* SPARSE ROW CASE with ilist */
      int32 i;
      i=0;
      while ((cindex=ilist[i])>=0) {
        value=s_cur[cindex];
        if ( ISNULL(arr[(orgcindex=toorg[cindex])]) ) { /* add element */
          arr[orgcindex] =
            mtx_create_element_value(mtx,t_org,orgcindex,(factor * value));
        } else {  /* increment element */
          arr[orgcindex]->value += factor * value;
        }
        i++;
      }
    }
    mtx_renull_using_row(mtx,t_org,arr);
  } else if( rng->high >= rng->low ) { /* DENSE RANGE CASE */
    t_org = mtx->perm.row.cur_to_org[t_cur]; /* org of target row */
    arr = mtx_expand_row(mtx,t_org);             /* Expand the target row */
    toorg = mtx->perm.col.cur_to_org;        /* current col perm */
    hilim = rng->high;

    for (cindex=rng->low; cindex<= hilim; cindex++) {
      if ((value=s_cur[cindex])!=D_ZERO) {
        if ( ISNULL(arr[(orgcindex=toorg[cindex])]) ) { /* add element */
          arr[orgcindex] =
            mtx_create_element_value(mtx,t_org,orgcindex,(factor * value));
        } else {  /* increment element */
          arr[orgcindex]->value += factor * value;
        }
      }
    }
    mtx_renull_using_row(mtx,t_org,arr);
  }
  mtx_null_vector_release();
}

void mtx_old_add_col_sparse(mtx_matrix_t mtx,
                       int32 t_cur,   /* cur index of target col */
                       real64 *s_cur, /* dense source col, curindexed */
                       real64 factor, /* coefficient */
                       mtx_range_t *rng,    /* range to add over or */
                       int32 *ilist  /* list to add over */
){
  int32 t_org,*toorg,rowindex,orgrowindex,hilim;
  struct element_t **arr;
  real64 value;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif

  if (factor==D_ZERO) return;

  if( rng == mtx_ALL_ROWS ) {
    t_org = mtx->perm.col.cur_to_org[t_cur]; /* org of target col */
    arr = mtx_expand_col(mtx,t_org);             /* Expand the target col */
    toorg = mtx->perm.row.cur_to_org;        /* current row perm */
    hilim=mtx->order;

    if (!ilist) { /* FULL COL CASE no ilist */
      for (rowindex=0; rowindex< hilim; rowindex++) {
        if ((value=s_cur[rowindex])!=D_ZERO) {
          if ( ISNULL(arr[(orgrowindex=toorg[rowindex])]) ) { /* add element */
            arr[orgrowindex] =
              mtx_create_element_value(mtx,orgrowindex,t_org,(factor * value));
          } else { /* increment element */
            arr[orgrowindex]->value += factor * value;
          }
        }
      }
    } else { /* SPARSE COL CASE with ilist */
      int32 i;
      i=0;
      while ((rowindex=ilist[i])>=0) {
        value=s_cur[rowindex];
        if ( ISNULL(arr[(orgrowindex=toorg[rowindex])])  ) { /* add element */
          arr[orgrowindex] =
            mtx_create_element_value(mtx,orgrowindex,t_org,(factor * value));
        } else { /* increment element */
          arr[orgrowindex]->value += factor * value;
        }
        i++;
      }
    }
    mtx_renull_using_col(mtx,t_org,arr);

  } else if( rng->high >= rng->low ) { /* DENSE RANGE CASE */
    t_org = mtx->perm.col.cur_to_org[t_cur]; /* org of target col */
    arr = mtx_expand_col(mtx,t_org);             /* Expand the target col */
    toorg = mtx->perm.row.cur_to_org;        /* current row perm */
    hilim = rng->high;

    for (rowindex=rng->low; rowindex<= hilim; rowindex++) {
      if ((value=s_cur[rowindex])!=D_ZERO) {
        if ( ISNULL(arr[(orgrowindex=toorg[rowindex])]) ) { /* add element */
          arr[orgrowindex] =
            mtx_create_element_value(mtx,orgrowindex,t_org,(factor * value));
        } else { /* increment element */
          arr[orgrowindex]->value += factor * value;
        }
      }
    }
    mtx_renull_using_col(mtx,t_org,arr);
  }
  mtx_null_vector_release();
}

size_t mtx_size(mtx_matrix_t mtx) {
  size_t size=0;
  if (ISNULL(mtx)) return size;
  size += (1+mtx->nslaves)*sizeof(struct mtx_header);
  if (ISSLAVE(mtx)) {
    return 2*sizeof(struct element_t *)*(size_t)mtx->capacity +size;
  }
  size += mem_sizeof_store(mtx->ms);
  /* headers */
  size += (1 + mtx->nslaves) * (size_t)mtx->capacity *
    (size_t)2 * sizeof(struct element_t *);
  /* block data */
  if (mtx->data->nblocks >0)
    size += sizeof(mtx_region_t)*(size_t)(mtx->data->nblocks-1);
  /* permutations */
  size += (size_t)4*(sizeof(int32)*(size_t)mtx->capacity);
  return size;
}

size_t mtx_chattel_size(mtx_matrix_t mtx) {
  size_t size=0;
  int32 i;
  if (ISNULL(mtx) || ISSLAVE(mtx)) return size;
  /* headers */
  size += (mtx->nslaves)*sizeof(struct mtx_header);
  /* incidence */
  for (i=0; i <mtx->nslaves; i++) {
    size += (size_t)mtx_nonzeros_in_region(mtx->slaves[i],mtx_ENTIRE_MATRIX);
  }
  /*nz headers */
  size += mtx->nslaves * (size_t)mtx->capacity *
    (size_t)2 * sizeof(struct element_t *);
  /* block data */
  /* permutations */
  return size;
}

void mtx_free_reused_mem(void){
  (void)mtx_null_mark((int32)0);
  (void)mtx_null_vector((int32)0);
  (void)mtx_null_row_vector((int32)0);
  (void)mtx_null_col_vector((int32)0);
}


void mtx_write_sparse(FILE *fp,mtx_sparse_t *v){
  int32 k;
  if (NOTNULL(v) && NOTNULL(fp)) {
    FPRINTF(fp,"len %d, cap %d\n",v->len,v->cap);
  } else {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called with NULL.\n");
    return;
  }
  if ( NOTNULL(v->data) && NOTNULL(v->idata) ){
    FPRINTF(fp,"valid data:\n");
    for (k=0; k < v->len; k++) {
      FPRINTF(fp,"(%d) %d %.18g\n",k,v->idata[k],v->data[k]);
    }
  } else {
    FPRINTF(fp,"Invalid data pointers found.\n");
  }
}

void mtx_write_region_human_f(FILE *fp,mtx_matrix_t mtx,
                              mtx_region_t *greg,int colwise,int orgmajor
){
  mtx_coord_t nz,org;
  real64 value;
  mtx_region_t reg;

  if (greg!=mtx_ENTIRE_MATRIX) {
    reg=(*greg);
  } else {
    mtx_region(&reg,0,mtx->order-1,0,mtx->order-1);
  }
  /* current col perm driven */
  if (colwise && !orgmajor) {
    for(nz.col = reg.col.low; nz.col <= reg.col.high; ++(nz.col) ) {
      nz.row = mtx_FIRST;
      /* if col not empty, print header and first element */
      if( value = mtx_next_in_col(mtx,&nz,&(reg.row)), nz.row != mtx_LAST ) {
        FPRINTF(fp,"   Col %d (org %d)\n", nz.col, mtx_col_to_org(mtx,nz.col));
        FPRINTF(fp,"      Row %d (org %d) has value %g\n",
                nz.row, mtx_row_to_org(mtx,nz.row), value);
      }
      while( value = mtx_next_in_col(mtx,&nz,&(reg.row)),
             nz.row != mtx_LAST ) {
        FPRINTF(fp,"      Row %d (org %d) has value %g\n",
                nz.row, mtx_row_to_org(mtx,nz.row), value);
      }
    }
    return;
  }
  /* current row perm driven */
  if (!colwise && !orgmajor) {
    for(nz.row = reg.row.low; nz.row <= reg.row.high; ++(nz.row) ) {
      nz.col = mtx_FIRST;
      if ( value = mtx_next_in_row(mtx,&nz,&(reg.col)), nz.col != mtx_LAST ) {
        FPRINTF(fp,"   Row %d (org %d)\n", nz.row, mtx_row_to_org(mtx,nz.row));
        FPRINTF(fp,"      Col %d (org %d) has value %g\n",
                nz.col, mtx_col_to_org(mtx,nz.col), value);
      }
      while( value = mtx_next_in_row(mtx,&nz,&(reg.col)),
             nz.col != mtx_LAST ) {
        FPRINTF(fp,"      Col %d (org %d) has value %g\n",
                nz.col, mtx_col_to_org(mtx,nz.col), value);
      }
    }
    return;
  }
  /* org row driven */
  if (!colwise && orgmajor) {

    for(org.row = 0; org.row < mtx->order; ++(org.row) ) {
      nz.row = mtx_org_to_row(mtx,org.row);
      /* skip outside block rows */
      if (nz.row < reg.row.low || nz.row > reg.row.high) {
        continue;
      }
      nz.col = mtx_FIRST;
      if ( value = mtx_next_in_row(mtx,&nz,&(reg.col)), nz.col != mtx_LAST ) {
        FPRINTF(fp,"OrgRow %d\n", org.row);
        FPRINTF(fp,"   OrgCol %d has value %g\n",
                mtx_col_to_org(mtx,nz.col), value);
      }
      while( value = mtx_next_in_row(mtx,&nz,&(reg.col)),
             nz.col != mtx_LAST ) {
        FPRINTF(fp,"   OrgCol %d has value %g\n",
                mtx_col_to_org(mtx,nz.col), value);
      }
    }
    return;
  }
  /* org col driven */
  if (colwise && orgmajor) {

    for(org.col = 0; org.col < mtx->order; ++(org.col) ) {
      nz.col = mtx_org_to_col(mtx,org.col);
      /* skip outside block cols */
      if (nz.col < reg.col.low || nz.col > reg.col.high) {
        continue;
      }
      nz.row = mtx_FIRST;
      if ( value = mtx_next_in_col(mtx,&nz,&(reg.row)), nz.row != mtx_LAST ) {
        FPRINTF(fp,"OrgCol %d\n", org.col);
        FPRINTF(fp,"   OrgRow %d has value %g\n",
                mtx_row_to_org(mtx,nz.row), value);
      }
      while( value = mtx_next_in_col(mtx,&nz,&(reg.row)),
             nz.row != mtx_LAST ) {
        FPRINTF(fp,"   OrgRow %d has value %g\n",
                mtx_row_to_org(mtx,nz.row), value);
      }
    }
    return;
  }
}

static void mtx_write_perm(FILE *fp,int32 ord,int32 *arr){
  int32 i;
  for (i=0; i<ord;i++)
    FPRINTF(fp,"%d\n",arr[i]);
}

static void mtx_read_perm(FILE *fp,int32 ord,int32 *arr){
  int32 i;
  for (i=0; i<ord;i++)
    fscanf(fp,"%d",&(arr[i]));
}

static char mtxmagic[]="mtx_matrix_data.\0";
static char permmagic[]="#endperm\0";
static char blockmagic[]="#endperm_beginblock\0";
static char blockend[]="#endblock\0";

void mtx_write_region(FILE *fp,mtx_matrix_t mtx,mtx_region_t *greg){
  mtx_coord_t nz;
  mtx_region_t reg;
  mtx_range_t *rng;
  int32 cur_col,org_col, *tocur,block;
  struct element_t *elt;

  if (greg!=mtx_ENTIRE_MATRIX)
    reg=(*greg);
   else
    mtx_region(&reg,0,mtx->order-1,0,mtx->order-1);
  rng= &(reg.col);
  FPRINTF(fp,"%s\n",mtxmagic);
  /* write order */
  FPRINTF(fp,"%d\n",mtx->order);
  /* write region generated from */
  FPRINTF(fp,"%d\n",reg.row.low);
  FPRINTF(fp,"%d\n",reg.row.high);
  FPRINTF(fp,"%d\n",reg.col.low);
  FPRINTF(fp,"%d\n",reg.col.high);
  /* write permutations of rows/cols */
  mtx_write_perm(fp,mtx->order,mtx->perm.row.cur_to_org);
  mtx_write_perm(fp,mtx->order,mtx->perm.row.org_to_cur);
  mtx_write_perm(fp,mtx->order,mtx->perm.col.cur_to_org);
  mtx_write_perm(fp,mtx->order,mtx->perm.col.org_to_cur);
  if(greg != mtx_ENTIRE_MATRIX) {
    FPRINTF(fp,"%s\n",permmagic);
  } else {
    FPRINTF(fp,"%s\n",blockmagic);
    FPRINTF(fp,"%d\n",mtx->data->nblocks);
    FPRINTF(fp,"%d\n",mtx->data->symbolic_rank);
    for (block= 0; block < mtx->data->nblocks; block++) {
      FPRINTF(fp,"%d\n",mtx->data->block[block].row.low);
      FPRINTF(fp,"%d\n",mtx->data->block[block].row.high);
      FPRINTF(fp,"%d\n",mtx->data->block[block].col.low);
      FPRINTF(fp,"%d\n",mtx->data->block[block].col.high);
    }
    FPRINTF(fp,"%s\n",blockend);
  }

  nz.row = reg.row.low;
  tocur = mtx->perm.col.org_to_cur;
  for( ; nz.row <= reg.row.high; ++(nz.row) ) {
    elt = mtx->hdr.row[mtx->perm.row.cur_to_org[nz.row]];
    for( ; NOTNULL(elt); elt = elt->next.col ) {
      org_col=elt->col;
      cur_col=tocur[org_col];
      if( in_range(rng,cur_col) )
        FPRINTF(fp,"%d %d %.20g\n",elt->row,org_col,elt->value);
    }
  }
}

static int getregion(FILE* fp, mtx_region_t *reg) {
  if( fscanf(fp,"%d",&(reg->row.low)) == EOF) return 0;
  if( fscanf(fp,"%d",&(reg->row.high)) == EOF) return 0;
  if( fscanf(fp,"%d",&(reg->col.low)) == EOF) return 0;
  if( fscanf(fp,"%d",&(reg->col.high)) == EOF) return 0;
#if MTX_DEBUG
  FPRINTF(g_mtxerr,"%d\n",reg->row.low);
  FPRINTF(g_mtxerr,"%d\n",reg->row.high);
  FPRINTF(g_mtxerr,"%d\n",reg->col.low);
  FPRINTF(g_mtxerr,"%d\n",reg->col.high);
#endif
  return 1;
}

static int getcoef(FILE* fp, int *row, int *col, double *val){
  char buf[80];
  if( fscanf(fp,"%d %d",row,col)==EOF) {
    return 0;
  } else {
    fscanf(fp,"%s",buf);
    *val=strtod(buf,NULL);
    return 1;
  }
}

mtx_matrix_t mtx_read_region(FILE *fp,mtx_matrix_t mtx,int transpose){
  real64 value;
  mtx_region_t reg;
  int32 orgrow,orgcol, ord,inc,nblocks;
  boolean readblocks;
  char buf[81];

  readblocks = FALSE;
  fscanf(fp,"%80s",buf);
  if (strcmp(buf,mtxmagic)) {
    FPRINTF(g_mtxerr,
	    "mtx_read_region: mtx data file has bad magic number\n%s\n",buf);
    return mtx;
  }
  /* order */
  fscanf(fp,"%d",&ord);
  if (ord<0) {
    FPRINTF(g_mtxerr, "mtx_read_region: mtx data has illegal order%d\n",ord);
    return mtx;
  }
#if MTX_DEBUG
  FPRINTF(g_mtxerr,"order found: %d\n",ord);
#endif
  /*region generated from */
  if( !getregion(fp,&reg)) {
    FPRINTF(g_mtxerr, "mtx_read_region: mtx data has illegal region.\n");
    return mtx;
  }
  if (ISNULL(mtx)) {
    mtx=mtx_create();
    mtx_set_order(mtx,ord);
    if ( reg.row.low==0 && reg.row.high == (ord-1) &&
         reg.col.low==0 && reg.col.high == (ord-1) ) {
      readblocks = TRUE;
    }
  } else {
    if (ord > mtx->order) mtx_set_order(mtx,ord);
    mtx_clear_region(mtx,&reg);
  }
  /* permutations of rows/cols */
  mtx_read_perm(fp,ord,mtx->perm.row.cur_to_org);
  mtx_read_perm(fp,ord,mtx->perm.row.org_to_cur);
  mtx_read_perm(fp,ord,mtx->perm.col.cur_to_org);
  mtx_read_perm(fp,ord,mtx->perm.col.org_to_cur);
  fscanf(fp,"%80s",buf);
  if (strcmp(buf,permmagic)) {
     /* could be new format with block data, then read, or bogusness */
    if (strcmp(buf,blockmagic)) {
      FPRINTF(g_mtxerr,
	      "mtx_read_region: mtx data file has bad perm magic\n%s\n",buf);
      return mtx;
    } else {
      /* read the block data */
      fscanf(fp,"%d",&nblocks);
      fscanf(fp,"%d",&(mtx->data->symbolic_rank));
      if (!readblocks) {
        mtx_region_t fake;
        for (inc=0; inc < nblocks; inc++) {
          /* must eat up data */
          if( !getregion(fp,&fake)) {
            FPRINTF(g_mtxerr, "mtx_read_region: mtx block data bogus.\n");
            return mtx;
          }
        }
        nblocks = 0;
      } else {
        /* ensure block capacity of mtx */
        if (nblocks > mtx->data->nblocks && nblocks > 0) {
          if (NOTNULL(mtx->data->block)) ascfree(mtx->data->block);
          mtx->data->nblocks = nblocks;
          mtx->data->block = (mtx_region_t *)
            ascmalloc( mtx->data->nblocks*sizeof(mtx_region_t) );
        }
      }
      for (inc=0; inc < nblocks; inc++) {
        if( !getregion(fp,&(mtx->data->block[inc])) ) {
          FPRINTF(g_mtxerr, "mtx_read_region: mtx data has illegal region.\n");
          mtx->data->nblocks = inc;
          mtx->data->symbolic_rank = 0;
          return mtx;
        }
      }
      fscanf(fp,"%80s",buf);
      if (strcmp(buf,blockend)) {
        FPRINTF(g_mtxerr, "mtx_read_region: mtx data has bad block magic.\n");
      }
    }
  }

  inc=0;
  if (!transpose) {
    while ( getcoef(fp,&orgrow,&orgcol,&value)) {
#if MTX_DEBUG
      FPRINTF(g_mtxerr,"%d %d %.20g\n",orgrow,orgcol,value);
#endif
      if (orgrow>=0 && orgcol>=0 && orgrow < ord && orgcol <ord) {
	mtx_create_element_value(mtx,orgrow,orgcol,value);
	inc++;
      }
    }
  }
  else{
    while ( getcoef(fp,&orgcol,&orgrow,&value)) {
#if MTX_DEBUG
      FPRINTF(g_mtxerr,"%d %d %.20g\n",orgrow,orgcol,value);
#endif
      if (orgrow>=0 && orgcol>=0 && orgrow < ord && orgcol <ord) {
	mtx_create_element_value(mtx,orgrow,orgcol,value);
	inc++;
      }
    }
  }
#if MTX_DEBUG
  FPRINTF(g_mtxerr,"Number of incidences created: %d\n",inc);
#endif

  return mtx;
}

void mtx_write_region_matlab(FILE *fp,mtx_matrix_t mtx,mtx_region_t *region){
  struct element_t Rewind, *elt;
  int32 *perm;
  mtx_coord_t nz;
  int32 nrows, nnz;

  if( !mtx_check_matrix(mtx) ) return;

  if( region != mtx_ENTIRE_MATRIX ) {
    nrows = region->row.high - region->row.low + 1;
  } else {
    nrows = mtx->order-1;
  }
  nnz = mtx_nonzeros_in_region(mtx,region);
  FPRINTF(stderr,"Generating plot file with %d rows, %d nonzeros\n",
	  nrows, nnz);

  perm = mtx->perm.col.org_to_cur;
  if( region == mtx_ENTIRE_MATRIX ) {
    for( nz.row = 0; nz.row < mtx->order; nz.row++ ) {
      Rewind.next.col = mtx->hdr.row[mtx->perm.row.cur_to_org[nz.row]];
      elt = &Rewind;
      for( ; NULL != (elt = mtx_next_col(elt,mtx_ALL_COLS,perm)) ; ) {
        FPRINTF(fp,"%d %d %.20g\n",nz.row,perm[elt->col],elt->value);
      }
    }
  } else {
    for( nz.row = region->row.low; nz.row <= region->row.high; nz.row++ ) {
      Rewind.next.col = mtx->hdr.row[mtx->perm.row.cur_to_org[nz.row]];
      elt = &Rewind;
      for( ; NULL != (elt = mtx_next_col(elt,&(region->col),perm)) ; ) {
        FPRINTF(fp,"%d %d %.20g\n",nz.row,perm[elt->col],elt->value);
      }
    }
  }
}

#ifdef ASC_WITH_MMIO
int mtx_write_region_mmio(FILE *fp,mtx_matrix_t mtx,mtx_region_t *region){
    MM_typecode matcode;
    int nrows, ncols, nnz, *perm;
	struct element_t Rewind, *elt;
	mtx_coord_t nz;

	if(!mtx_check_matrix(mtx))return 1;

    mm_initialize_typecode(&matcode);
    mm_set_matrix(&matcode);
    mm_set_coordinate(&matcode);
    mm_set_real(&matcode);

    mm_write_banner(fp, matcode);

	if(region == mtx_ENTIRE_MATRIX)nrows = mtx->order;
	else nrows = region->row.high - region->row.low + 1;

	if(region == mtx_ENTIRE_MATRIX)ncols = mtx->order;
	else ncols = region->col.high - region->col.low + 1;

	nnz = mtx_nonzeros_in_region(mtx,region);

	fprintf(fp,"%% Matrix Market file format\n");
	fprintf(fp,"%% see http://math.nist.gov/MatrixMarket/\n");
	fprintf(fp,"%% RANGE: rows = %d, cols = %d, num_of_non_zeros =%d\n",nrows,ncols,nnz);
	fprintf(fp,"%% MATRIX: rows = %d, cols = %d\n",mtx->order,mtx->order);

    mm_write_mtx_crd_size(fp, mtx->order, mtx->order, nnz);

    /*
		NOTE: matrix market files use 1-based indices, i.e. first element
		of a vector has index 1, not 0.
	*/

	fprintf(fp,"%% sparse value data:\n");
	fprintf(fp,"%% row#, col#, value\n");

	if(!nnz){
		fprintf(fp,"%%\n%%\n%% note: as exported, there were no non-zeros in the matrix\n%%\n%%\n");
	}

	perm = mtx->perm.col.org_to_cur;
	if(region == mtx_ENTIRE_MATRIX){
		fprintf(fp,"%% whole matrix:\n");
		for( nz.row = 0; nz.row < mtx->order; nz.row++ ) {
			Rewind.next.col = mtx->hdr.row[mtx->perm.row.cur_to_org[nz.row]];
			elt = &Rewind;
			for( ; NULL != (elt = mtx_next_col(elt,mtx_ALL_COLS,perm)) ; ) {
				fprintf(fp,"%d %d %.20g\n",nz.row + 1,perm[elt->col] + 1,elt->value);
			}
		}
	} else {
		fprintf(fp,"%% matrix range:\n");
		fprintf(fp,"%% row low = %d, row high = %d\n",region->row.low,region->row.high);
		fprintf(fp,"%% col low = %d, col high = %d\n",region->col.low,region->col.high);
		for( nz.row = region->row.low; nz.row <= region->row.high; nz.row++ ) {
			Rewind.next.col = mtx->hdr.row[mtx->perm.row.cur_to_org[nz.row]];
			elt = &Rewind;
			for( ; NULL != (elt = mtx_next_col(elt,&(region->col),perm)) ; ) {
				fprintf(fp,"%d %d %.20g\n",nz.row + 1,perm[elt->col] + 1,elt->value);
			}
		}
	}

	fflush(fp);
	CONSOLE_DEBUG("Wrote matrix range (%d x %d) to file",nrows,ncols);

	return 0;
}
#endif


void mtx_write_region_plot(FILE *fp,mtx_matrix_t mtx,mtx_region_t *region){
  struct element_t Rewind, *elt;
  int32 *perm;
  mtx_coord_t nz;
  int32 nrows, nnz;

  if( !mtx_check_matrix(mtx) ) return;

  if( region != mtx_ENTIRE_MATRIX ) {
    nrows = region->row.high - region->row.low + 1;
  } else {
    nrows = mtx->order-1;
  }
  nnz = mtx_nonzeros_in_region(mtx,region);
  FPRINTF(stderr,"Generating plot file with %d rows, %d nonzeros\n",
	  nrows, nnz);

  FPRINTF(fp,"TitleText:   Structural Analysis\n");
  FPRINTF(fp,"XUnitText:   Variables\n");
  FPRINTF(fp,"YUnitText:   Relations\n\n");
  FPRINTF(fp,"\"incidence\"\n");

  perm = mtx->perm.col.org_to_cur;
  if( region == mtx_ENTIRE_MATRIX ) {
    for( nz.row = 0; nz.row < mtx->order; nz.row++ ) {
      Rewind.next.col = mtx->hdr.row[mtx->perm.row.cur_to_org[nz.row]];
      elt = &Rewind;
      for( ; NULL != (elt = mtx_next_col(elt,mtx_ALL_COLS,perm)) ; )
        FPRINTF(fp,"%d  %d\n",perm[elt->col],-(nz.row));
    }
  } else {
    for( nz.row = region->row.low; nz.row <= region->row.high; nz.row++ ) {
      Rewind.next.col = mtx->hdr.row[mtx->perm.row.cur_to_org[nz.row]];
      elt = &Rewind;
      for( ; NULL != (elt = mtx_next_col(elt,&(region->col),perm)); )
        FPRINTF(fp,"%d  %d\n",perm[elt->col],-(nz.row));
    }
  }
}

/*
 * This code writes out a matrix in CSR or compressed row format.
 * The first thing that is written out is n and nnz; the number of
 * rows and the the total number of nonzeros.
 * Next it writes out A, (nnz long). It then writes an array of
 * row of column indices (nnz long). Finally it writes out an array
 * of row pointers (n) long. Offset refers to whether indexing starts
 * from 0 or 1.
 *
 *	EXAMPLE:  FOR THE MATRIX (using offset = 1)
 *
 *                                         X  0  X  0
 *                                         0  X  X  0
 *                                         0  0  X  X
 *                                         X  X  X  X
 *
 *               WHERE X INDICATES ANY NONZERO ELEMENT,
 *
 *               THE ARRAYS ARE:
 *               ICI(I) = 1,3,2,3,3,4,1,2,3,4
 *               IRP(I) = 1,3,5,7,11
 */

void mtx_write_region_csr(FILE *fp,mtx_matrix_t mtx,
			  mtx_region_t *region, int offset
){
  mtx_region_t reg;
  mtx_coord_t nz;
  real64 value;
  int32 *ici = NULL, *colptr;
  int32 *irp = NULL, *rowptr;
  int32 nnz, nrows;
  int32 count=0, i,j;
  int32 written=0;
  int lowcol, lowrow;
  int old=0;

  if (region!=mtx_ENTIRE_MATRIX)
    reg = *region;
  else
    mtx_region(&reg,0,mtx->order-1,0,mtx->order-1);

  /*
   * Ascend allows empty rows and columns. Most people dont
   * like this. We hence check to find the first non empty
   * column and row in the given region. THIS NEEDS TO BE
   * FIXED PROPERLY.
   */
  lowcol = reg.col.low;
  lowrow = reg.row.low;

  nrows = reg.row.high - reg.row.low + 1;
  nnz = mtx_nonzeros_in_region(mtx,&reg);

  FPRINTF(stderr,"Generating csr file with %d rows, %d nonzeros\n",
	  nrows, nnz);

  /*
   * I malloc a little bit more than is necessary. These are
   * temporary vectors. I prefer to be conservative to deal with
   * all the odd cases of empty rows, cols and matrices. The
   * irp vector however must be at least nrows + 1.
   */
  colptr = ici = ASC_NEW_ARRAY_CLEAR(int32,nnz+8);
  irp = ASC_NEW_ARRAY_CLEAR(int32,nrows+8);
  irp[0] = offset;
  rowptr = &irp[1];

  FPRINTF(fp,"%d %d\n",nrows, nnz);			/* nrows nnz */

  /*
   * Everyone else in the world, seems to think that life starts
   * at 1 and is continuous and contiguous up to order. We try
   * to satisfy these people by adjusting our start index to 0 or 1
   * depending on the value of offset.
   */
  for (nz.row = lowrow; nz.row <= reg.row.high; nz.row++) {
    old = count;
    nz.col = -1;
    while (value = mtx_next_in_row(mtx,&nz,&(reg.col)),
	   nz.col!= mtx_LAST) {
      FPRINTF(fp,"%20.8e\n",value);
      j = nz.col - lowcol + offset;
      *colptr++ = j;				/* compute col index */
      count++;
    }
    *rowptr++ = count + offset;			/* the row pointer */
    if (old==count) {
      FPRINTF(stderr,"Warning: Empty row found at incidence %d, row %d\n",
	      count, nz.row);
    }
  }
  FPRINTF(fp,"\n\n");

  /*
   * Write out col indices i.e, ici, or ja if you prefer.
   */
  written = 0;
  for (j=0; j<nnz; j++) {
    FPRINTF(fp,"%8d ",ici[j]);
    written++;
    if (written >= 8) {		/* just to break lines */
      FPRINTF(fp,"\n");
      written = 0;
    }
  }
  FPRINTF(fp,"\n\n");

  /*
   * Write out row pointers, i.e. irp or ka.
   * The <= is intentional, as we write out n+1 indices.
   */
  written = 0;
  for(i=0; i<=nrows; i++) {
    FPRINTF(fp,"%8d ",irp[i]);
    written++;
    if (written >= 8) {		/* just to break lines */
      FPRINTF(fp,"\n");
      written = 0;
    }
  }
  FPRINTF(fp,"\n\n");

  if (ici) ascfree((char *)ici);
  if (irp) ascfree((char *)irp);
}

/*
 * The so called smms format, or Sparse Matrix Manipulation System
 * format (Alavarado, F.L., 1993), writes out the number of rows and
 * columns and the coordinates of the element and its value. It
 * terminates with 0 0 0.0.
 * Offset here determines whether we start indexing at 0 or 1.
 * Most people cant deal with matrices that dont have contiguous
 * rows and columns, so we always write from offset to order.
 *
 *	EXAMPLE:  FOR THE MATRIX (using offset = 1)
 *
 *                                         5  0  9  0
 *                                         0  4  7  0
 *                                         0  0 19 -1
 *                                        -3  1  8  2
 *
 *				4 4
 *				1 1  5.0
 *				1 3  9.0
 *				2 2  4.0
 *				2 3  7.0
 *				[...]
 *				3 3 19.0
 *				[...]
 *				4 3  8.0
 *				4 4  2.0
 *
 */
void mtx_write_region_smms(FILE *fp, mtx_matrix_t mtx,
			   mtx_region_t *region, int offset
){
  mtx_coord_t nz;
  real64 value;
  mtx_region_t reg;
  int i,j,lowcol,lowrow;

  if (region!=mtx_ENTIRE_MATRIX)
    reg = *region;
  else
    mtx_region(&reg,0,mtx->order-1,0,mtx->order-1);

  /*
   * Ascend allows empty rows and columns. Most people dont
   * like this. We hence check to find the first non empty
   * column and row in the given region. THIS NEEDS TO BE
   * DONE PROPERLY.
   */
  lowcol = reg.col.low;
  lowrow = reg.row.low;

  FPRINTF(stderr,"Generating smms file with %d rows, %d nonzeros\n",
	  reg.row.high - reg.row.low + 1,
	  mtx_nonzeros_in_region(mtx,&reg));
  FPRINTF(fp,"%d %d\n",				/* nrows ncols */
	  reg.row.high - reg.row.low + 1,
	  reg.col.high - reg.col.low + 1);

  for (nz.row = lowrow; nz.row <= reg.row.high; nz.row++) {/* i j value */
    nz.col = -1;
    i = nz.row - lowrow + offset;
    while (value = mtx_next_in_row(mtx,&nz,&(reg.col)),
	   nz.col != mtx_LAST) {
      j = nz.col - lowcol + offset;
      FPRINTF(fp,"%d %d %20.8e\n", i, j, value);
    }
  }
  FPRINTF(fp,"0 0 0.0\n");			/* terminate */
}

mtx_matrix_t mtx_read_smms(FILE *fp,mtx_matrix_t mtx,int transpose){
  real64 value;
  int32 orgrow,orgcol, ord, count;
  int32 nrows=0, ncols=0;
  char buffer[256];

  /*
   * Get the order of the matrix. smms requires the first line contain
   * 	nrows <ncols>
   * The ncols is optional if the matrix is square. We check for
   * that here.
   */
  if (fgets(buffer,80,fp)==NULL) {
    FPRINTF(g_mtxerr,"mtx_read_smms: EOF while reading order\n");
    return mtx;
  }
  count = sscanf(buffer,"%d %d",&nrows, &ncols);
  if (count==1) ncols = nrows;			/* ncols is optional */
  if (nrows<=0 || ncols<=0) {
    FPRINTF(g_mtxerr,"mtx_read_smms: Illegal order found\n");
    return mtx;
  }
  /*
   * We have to create a mtx with order, at least 1 greater than
   * number of rows found. We intend to maintain the indexing
   * as found in the file, and smms matices always start at 1 1
   */
  ord = MAX(nrows,ncols) + 1;
  FPRINTF(g_mtxerr,
          "Reading matrix with %d rows and %d columns\n", nrows, ncols);

  if (ISNULL(mtx)) {
    mtx = mtx_create();
    mtx_set_order(mtx,ord);
  } else {
    if (ord > mtx->order)
      mtx_set_order(mtx,ord);
    mtx_clear_region(mtx,mtx_ALL_ROWS);
  }

  /*
   * Now get the elements of the matrix. SMMS uses either
   * the EOF or 0 0 0.0 as terminator. Its valid values
   * starts at 1 1.
   */
  count = 0;
  if (!transpose) {
    while (getcoef(fp,&orgrow,&orgcol,&value)) {
      if (orgrow>0 && orgcol>0 && orgrow<ord && orgcol<ord) {
	if (fabs(value)<=1.0e-16) {
	  continue;
	}
	else{
#undef MTX_DEBUG
#ifdef MTX_DEBUG
	  FPRINTF(g_mtxerr,"%d %d %.20g\n",orgrow,orgcol,value);
#endif /* MTX_DEBUG */
	  mtx_create_element_value(mtx,orgrow,orgcol,value);
	  count++;
	}
      }
      else{	/* illegal data or the end. */
	break;
      }
    }
  }
  else{
    while (getcoef(fp,&orgcol,&orgrow,&value)) {
      if (orgrow>0 && orgcol>0 && orgrow<ord && orgcol<ord) {
	if (fabs(value)<=1.0e-16) {
	  continue;
	}
	else{
#undef MTX_DEBUG
#ifdef MTX_DEBUG
	  FPRINTF(g_mtxerr,"%d %d %.20g\n",orgrow,orgcol,value);
#endif /* MTX_DEBUG */
	  mtx_create_element_value(mtx,orgrow,orgcol,value);
	  count++;
	}
      }
      else{	/* illegal data or the end. */
	break;
      }
    }
  }
  FPRINTF(g_mtxerr,"Number of incidences created: %d\n",count);
  return mtx;
}

void mtx_exception_recover(void){
  add_series_data_release();
  mtx_reset_null_vectors();
}

void mtx__debug_output(FILE *fp,mtx_matrix_t mtx){
   if (fp)
     g_mtxerr=fp;
   else
     g_mtxerr=stderr;
   mtx_check_matrix(mtx);
#if !MTX_DEBUG
   super_check_matrix(mtx);
#endif
   g_mtxerr=stderr;
}

#undef __MTX_C_SEEN__
