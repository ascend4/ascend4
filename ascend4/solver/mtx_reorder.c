/*
 *  mtx: Ascend Sparse Matrix Package Reordering
 *  by Ben Allan
 *  Created: 5/31/96
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: mtx_reorder.c,v $
 *  Date last modified: $Date: 1997/07/28 20:53:09 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the SLV solver.
 *
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

#include <math.h>
#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "utilities/mem.h"
#include "solver/mtx.h"
#define __MTX_C_SEEN__
#include "solver/mtx_use_only.h"
#define R_DEBUG FALSE

/*
 * All I know for sure is that reordering shouldn't be the
 * job of a linear factorization method.
 * Note: all of the following is still in the 'outside mtx'
 * idiom which makes some things terribly inefficient.
 * As a result, it doesn't need to include the mtx_use_only.h.
 * For efficiency, this needs to be converted over to mtx
 * internal idiom.
 * On top of all that, this stuff really belongs in mtx_perms.[ch]
 * and not out by itself, imho.
 * BAA 5/96.
 *
 * bugs:
 * spk1 and tspk1 use that stupid ptr backup game to get arrays that
 * look bigger than they actually are. This needs to be replaced with
 * use of mtx's reusable buffer space.
 */
/***************************************************************************\
  Reordering functions for SPK1, and possibly for other schemes to be
  implemented later.
  The stuff here is almost, but not quite, black magic. Don't tinker with it.
  Once it is translated into mtx idiom, it will be black magic.
\***************************************************************************/

/*********************************
 begin of spk1 stuff
*********************************/
struct reorder_list {            /* List of rows/columns and their counts. */
   int32 ndx;
   int32 count;
   struct reorder_list *next;
};

struct reorder_vars {
   mtx_matrix_t mtx;
   mtx_region_t reg;             /* Current active region */
   int32 colhigh;          /* Original value of reg.col.high */
   struct reorder_list *tlist;   /* Array of (enough) list elements */
   int32 *rowcount;        /* rowcount[reg.row.low .. reg.row.high] */
};

static void adjust_row_count(struct reorder_vars *vars,int32 removed_col)
/**
 ***  Adjusts the row counts to account for the (to be) removed column.
 **/
{
   mtx_coord_t nz;
   nz.col = removed_col;
   nz.row = mtx_FIRST;
   while( mtx_next_in_col(vars->mtx,&nz,&(vars->reg.row)),
	 nz.row != mtx_LAST )
      --(vars->rowcount[nz.row]);
}

static void assign_row_and_col(struct reorder_vars *vars,
                          int32 row,
                          int32 col)
/**
 ***  Assigns the given row to the given column, moving the row and column
 ***  to the beginning of the active region and removing them (readjusting
 ***  the region).  The row counts are NOT adjusted.  If col == mtx_NONE,
 ***  then no column is assigned and the row is moved to the end of the
 ***  active block instead.  Otherwise, it is assumed that the column
 ***  is active.
 **/
{
   if( col == mtx_NONE ) {
      mtx_swap_rows(vars->mtx,row,vars->reg.row.high);
      vars->rowcount[row] = vars->rowcount[vars->reg.row.high];
      --(vars->reg.row.high);
   } else {
      mtx_swap_rows(vars->mtx,row,vars->reg.row.low);
      vars->rowcount[row] = vars->rowcount[vars->reg.row.low];
      mtx_swap_cols(vars->mtx,col,vars->reg.col.low);
      ++(vars->reg.row.low);
      ++(vars->reg.col.low);
   }
}

static void push_column_on_stack(struct reorder_vars *vars,int32 col)
/**
 ***  Pushes the given column onto the stack.  It is assumed that the
 ***  column is active.  Row counts are adjusted.
 **/
{
   adjust_row_count(vars,col);
   mtx_swap_cols(vars->mtx,col,vars->reg.col.high);
   --(vars->reg.col.high);
}

static int32 pop_column_from_stack(struct reorder_vars *vars)
/**
 ***  Pops the column on the "top" of the stack off of the stack and
 ***  returns the column index, where it now lies in the active region.
 ***  If the stack is empty, mtx_NONE is returned.  Row counts are NOT
 ***  adjusted (this together with a subsequent assignment of this column
 ***  ==> no row count adjustment necessary).
 **/
{
   if( vars->reg.col.high < vars->colhigh )
      return(++(vars->reg.col.high));
   else
      return( mtx_NONE );
}

static void assign_null_rows(struct reorder_vars *vars)
/**
 ***  Assigns empty rows, moving them to the assigned region.  It is
 ***  assumed that row counts are correct.  Columns are assigned off the
 ***  stack.
 **/
{
   int32 row;

   for( row = vars->reg.row.low ; row <= vars->reg.row.high ; ++row )
      if( vars->rowcount[row] == 0 )
         assign_row_and_col(vars , row , pop_column_from_stack(vars));
}

static void forward_triangularize(struct reorder_vars *vars)
/**
 ***  Forward triangularizes the region, assigning singleton rows with their
 ***  one and only incident column until there are no more.  The row counts
 ***  must be correct, and they are updated.
 **/
{
   boolean change;
   mtx_coord_t nz;

   do {
      change = FALSE;
      for( nz.row = vars->reg.row.low ;
          nz.row <= vars->reg.row.high && vars->rowcount[nz.row] != 1;
          ++nz.row ) ;
      if( nz.row <= vars->reg.row.high ) {
         /* found singleton row */
         nz.col = mtx_FIRST; /* this is somehow coming back with nz.col -1 */
         mtx_next_in_row(vars->mtx,&nz,&(vars->reg.col));
         adjust_row_count(vars,nz.col);
         assign_row_and_col(vars,nz.row,nz.col);
         change = TRUE;
      }
   } while( change );
}

static int32 select_row(struct reorder_vars *vars)
/**
 ***  Selects a row and returns its index.  It is assumed that there is a
 ***  row.  Row counts must be correct.  vars->tlist will be used.
 **/
{
   int32 min_row_count;
   int32 max_col_count;
   int32 row;
   int32 i, nties=-2;  /* # elements currently defined in vars->tlist */
   int32 sum;
   mtx_coord_t nz;
   mtx_matrix_t mtx;
   mtx_range_t *colrng, *rowrng;

   /* Set to something > any possible value */
   min_row_count = vars->reg.col.high-vars->reg.col.low+2;
   for( row = vars->reg.row.low ; row <= vars->reg.row.high ; ++row )
      if( vars->rowcount[row] <= min_row_count ) {
         if( vars->rowcount[row] < min_row_count ) {
            min_row_count = vars->rowcount[row];
            nties = 0;
         }
         vars->tlist[nties++].ndx = row;
      }
   /**
    ***  vars->tlist[0..nties-1] is a list of row numbers which tie for
    ***   minimum row count.
    **/

   max_col_count = -1;   /* < any possible value */
   i = 0;
   mtx = vars->mtx;
   colrng=&(vars->reg.col);
   rowrng=&(vars->reg.row);
   while( i < nties ) {

      sum = 0;
      nz.row = vars->tlist[i].ndx;
      nz.col = mtx_FIRST;
      while(mtx_next_in_row(mtx,&nz,colrng),
	    nz.col != mtx_LAST )
         sum += mtx_nonzeros_in_col(mtx,nz.col,rowrng);
      if( sum > max_col_count ) {
         max_col_count = sum;
         row = nz.row;
      }
      i++;
   }
   /**
    ***  Now row contains the row with the minimum row count, which has the
    ***  greatest total column count of incident columns among all rows with
    ***  the same (minimum) row count.  Select it.
    **/
   return(row);
}

static void mtx_spk1_reorder(struct reorder_vars *vars)
/**
 ***  Reorders the assigned matrix vars->mtx within the specified bounding
 ***  block region vars->reg.  The region is split into 6 subregions during
 ***  reordering:  the rows are divided in two, and the columns divided in
 ***  three.  Initially everything is in the active subregion.  Ultimately,
 ***  everything will be assigned.
 ***
 ***          <-- assigned -->|<-- active-->|<-- on stack -->|
 ***     ----+----------------+-------------+----------------+
 ***       a |                |             |                |
 ***       s |                |             |                |
 ***       s |                |             |                |
 ***       i |    (SQUARE)    |             |                |
 ***       g |                |             |                |
 ***       n |                |             |                |
 ***       e |                |             |                |
 ***       d |                |             |                |
 ***     ----+----------------+-------------+----------------+
 ***       a |                |             |                |
 ***       c |                |    ACTIVE   |                |
 ***       t |                |    REGION   |                |
 ***       i |                |             |                |
 ***       v |                |             |                |
 ***       e |                |             |                |
 ***     ----+----------------+-------------+----------------+
 ***
 ***  The algorithm is roughly as follows:
 ***    (1) select a row (by some criterion).
 ***    (2) push columns incident on that row onto the stack in decreasing
 ***        order of their length.
 ***    (3) pop first column off the stack and assign it to the selected
 ***        row.
 ***    (4) forward-triangularize (assign singleton rows with their one
 ***        and only incident column, until no longer possible).
 ***
 ***    (1)-(4) should be repeated until the active subregion becomes empty.
 ***
 ***  Everything above was written as though the entire matrix is
 ***  involved.  In reality, only the relevant square region is involved.
 **/
{
   int32 row, size;
   int32 *rowcount_array_origin;
   mtx_matrix_t mtx;

   size = MAX(vars->reg.row.high,vars->reg.col.high) + 1;
   vars->tlist = size > 0 ? (struct reorder_list *)
      ascmalloc( size*sizeof(struct reorder_list) ) : NULL;
   vars->rowcount = rowcount_array_origin = size > 0 ? (int32 *)
      ascmalloc( size*sizeof(int32) ) : NULL;
   mtx = vars->mtx;

   vars->colhigh = vars->reg.col.high;
   /* Establish row counts */
   for( row = vars->reg.row.low ; row <= vars->reg.row.high ; ++row )
      vars->rowcount[row] =
	 mtx_nonzeros_in_row(mtx,row,&(vars->reg.col));

   while(TRUE) {
      struct reorder_list *head;
      int32 nelts;   /* # elements "allocated" from vars->tlist */
      mtx_coord_t nz;

      forward_triangularize(vars);
      assign_null_rows(vars);
      if( vars->reg.row.low>vars->reg.row.high ||
	 vars->reg.col.low>vars->reg.col.high ) {
	 /* Active region is now empty, done */
	 if( NOTNULL(vars->tlist) )
	    ascfree( vars->tlist );
	 if( NOTNULL(rowcount_array_origin) )
	    ascfree( rowcount_array_origin );
         return;
      }

      head = NULL;
      nelts = 0;
      nz.row = select_row(vars);
      nz.col = mtx_FIRST;
      while(mtx_next_in_row(mtx,&nz,&(vars->reg.col)),
	    nz.col != mtx_LAST ) {
         struct reorder_list **q,*p;

         p = &(vars->tlist[nelts++]);
         p->ndx = mtx_col_to_org(mtx,nz.col);
         p->count = mtx_nonzeros_in_col(mtx,nz.col,&(vars->reg.row));
         for( q = &head; *q && (*q)->count > p->count; q = &((*q)->next) );
         p->next = *q;
         *q = p;
      }
      /**
       ***  We now have a list of columns which intersect the selected row.
       ***  The list is in order of decreasing column count.
       **/

      /* Push incident columns on stack */
      for( ; NOTNULL(head) ; head = head->next )
         push_column_on_stack(vars,mtx_org_to_col(mtx,head->ndx));

      /* Pop column off stack and assign to selected row */
      assign_row_and_col(vars , nz.row , pop_column_from_stack(vars));
   }
   /*  Not reached. */
}

/*********************************
 end of spk1 stuff
*********************************/
/*********************************
 begin of tspk1 stuff
*********************************/
struct creorder_list {            /* List of columns/rows and their counts. */
   int32 ndx;
   int32 count;
   struct creorder_list *next;
};

struct creorder_vars {
   mtx_matrix_t mtx;
   mtx_region_t reg;             /* Current active region */
   int32 rowhigh;          /* Original value of reg.row.high */
   struct creorder_list *tlist;  /* Array of (enough) list elements */
   int32 *colcount;        /* colcount[reg.col.low .. reg.col.high] */
};

static void adjust_col_count(struct creorder_vars *vars,int32 removed_row)
/**
 ***  Adjusts the column counts to account for the (to be) removed row.
 **/
{
   mtx_coord_t nz;
   nz.row = removed_row;
   nz.col = mtx_FIRST;
   while( mtx_next_in_row(vars->mtx,&nz,&(vars->reg.col)),
	 nz.col != mtx_LAST )
      --(vars->colcount[nz.col]);
}

static void assign_col_and_row(struct creorder_vars *vars,
                          int32 col,
                          int32 row)
/**
 ***  Assigns the given row to the given column, moving the row and column
 ***  to the beginning of the active region and removing them (readjusting
 ***  the region).  The col counts are NOT adjusted.  If col == mtx_NONE,
 ***  then no column is assigned and the col is moved to the end of the
 ***  active block instead.  Otherwise, it is assumed that the row
 ***  is active.
 **/
{
   if( row == mtx_NONE ) {
      mtx_swap_cols(vars->mtx,col,vars->reg.col.high);
      vars->colcount[col] = vars->colcount[vars->reg.col.high];
      --(vars->reg.col.high);
   } else {
      mtx_swap_cols(vars->mtx,col,vars->reg.col.low);
      vars->colcount[col] = vars->colcount[vars->reg.col.low];
      mtx_swap_rows(vars->mtx,row,vars->reg.row.low);
      ++(vars->reg.col.low);
      ++(vars->reg.row.low);
   }
}

static void push_row_on_stack(struct creorder_vars *vars,int32 row)
/**
 ***  Pushes the given row onto the stack.  It is assumed that the
 ***  row is active.  Col counts are adjusted.
 **/
{
   adjust_col_count(vars,row);
   mtx_swap_rows(vars->mtx,row,vars->reg.row.high);
   --(vars->reg.row.high);
}

static int32 pop_row_from_stack(struct creorder_vars *vars)
/**
 ***  Pops the row on the "top" of the stack off of the stack and
 ***  returns the row index, where it now lies in the active region.
 ***  If the stack is empty, mtx_NONE is returned.  Col counts are NOT
 ***  adjusted (this together with a subsequent assignment of this row
 ***  ==> no col count adjustment necessary).
 **/
{
   if( vars->reg.row.high < vars->rowhigh )
      return(++(vars->reg.row.high));
   else
      return( mtx_NONE );
}

static void assign_null_cols(struct creorder_vars *vars)
/**
 ***  Assigns empty cols, moving them to the assigned region.  It is
 ***  assumed that col counts are correct.  Rows are assigned off the
 ***  stack.
 **/
{
   int32 col;

   for( col = vars->reg.col.low ; col <= vars->reg.col.high ; ++col )
      if( vars->colcount[col] == 0 )
         assign_col_and_row(vars , col , pop_row_from_stack(vars));
}

static void cforward_triangularize(struct creorder_vars *vars)
/**
 ***  Forward triangularizes the region, assigning singleton columns with their
 ***  one and only incident row until there are no more.  The column counts
 ***  must be correct, and they are updated.
 **/
{
   boolean change;

   do {
      mtx_coord_t nz;
      change = FALSE;
      for( nz.col = vars->reg.col.low ;
          nz.col <= vars->reg.col.high && vars->colcount[nz.col] != 1;
          ++nz.col ) ;
      if( nz.col <= vars->reg.col.high ) {
         /* found singleton col */
         nz.row = mtx_FIRST;
         mtx_next_in_col(vars->mtx,&nz,&(vars->reg.row));
         adjust_col_count(vars,nz.row);
         assign_col_and_row(vars,nz.col,nz.row);
         change = TRUE;
      }
   } while( change );
}

static int32 select_col(struct creorder_vars *vars)
/**
 ***  Selects a col and returns its index.  It is assumed that there is a
 ***  col.  Col counts must be correct.  vars->tlist will be used.
 **/
{
   int32 min_col_count;
   int32 max_row_count;
   int32 col;
   int32 i, nties=-2;  /* # elements currently defined in vars->tlist */
   int32 sum;
   mtx_coord_t nz;
   mtx_matrix_t mtx;
   mtx_range_t *colrng, *rowrng;

   /* Set to something > any possible value */
   min_col_count = vars->reg.row.high-vars->reg.row.low+2;
   for( col = vars->reg.col.low ; col <= vars->reg.col.high ; ++col )
      if( vars->colcount[col] <= min_col_count ) {
         if( vars->colcount[col] < min_col_count ) {
            min_col_count = vars->colcount[col];
            nties = 0;
         }
         vars->tlist[nties++].ndx = col;
      }
   /**
    ***  vars->tlist[0..nties-1] is a list of row numbers which tie for
    ***   minimum col count.
    **/

   max_row_count = -1;   /* < any possible value */
   i = 0;
   mtx = vars->mtx;
   rowrng=&(vars->reg.row);
   colrng=&(vars->reg.col);
   while( i < nties ) {

      sum = 0;
      nz.row = mtx_FIRST;
      nz.col = vars->tlist[i].ndx;
      while( mtx_next_in_col(mtx,&nz,rowrng),
	    nz.row != mtx_LAST )
         sum += mtx_nonzeros_in_row(mtx,nz.row,colrng);
      if( sum > max_row_count ) {
         max_row_count = sum;
         col = nz.col;
      }
      i++;
   }
   /**
    ***  Now col contains the col with the minimum col count, which has the
    ***  greatest total row count of incident rows among all cols with
    ***  the same (minimum) col count.  Select it.
    **/
   return(col);
}

static void mtx_tspk1_reorder(struct creorder_vars *vars)
/**
 ***  Transpose the picture and explanation that follows:
 ***  Reorders the assigned matrix vars->mtx within the specified bounding
 ***  block region vars->reg.  The region is split into 6 subregions during
 ***  reordering:  the rows are divided in two, and the columns divided in
 ***  three.  Initially everything is in the active subregion.  Ultimately,
 ***  everything will be assigned.
 ***
 ***          <-- assigned -->|<-- active-->|<-- on stack -->|
 ***     ----+----------------+-------------+----------------+
 ***       a |                |             |                |
 ***       s |                |             |                |
 ***       s |                |             |                |
 ***       i |    (SQUARE)    |             |                |
 ***       g |                |             |                |
 ***       n |                |             |                |
 ***       e |                |             |                |
 ***       d |                |             |                |
 ***     ----+----------------+-------------+----------------+
 ***       a |                |             |                |
 ***       c |                |    ACTIVE   |                |
 ***       t |                |    REGION   |                |
 ***       i |                |             |                |
 ***       v |                |             |                |
 ***       e |                |             |                |
 ***     ----+----------------+-------------+----------------+
 ***
 ***  The algorithm is roughly as follows:
 ***    (1) select a row (by some criterion).
 ***    (2) push columns incident on that row onto the stack in decreasing
 ***        order of their length.
 ***    (3) pop first column off the stack and assign it to the selected
 ***        row.
 ***    (4) forward-triangularize (assign singleton rows with their one
 ***        and only incident column, until no longer possible).
 ***
 ***    (1)-(4) should be repeated until the active subregion becomes empty.
 ***
 ***  Everything above was written as though the entire matrix is
 ***  involved.  In reality, only the relevant square region is involved.
 **/
{
   int32 col, size;
   int32 *colcount_array_origin;
   mtx_matrix_t mtx;

   size = vars->reg.col.high - vars->reg.col.low + 1;
   size = MAX(size,vars->reg.row.high - vars->reg.row.low + 1);
   vars->tlist = size > 0 ? (struct creorder_list *)
      ascmalloc( size*sizeof(struct creorder_list) ) : NULL;
   colcount_array_origin = size > 0 ? (int32 *)
      ascmalloc( size*sizeof(int32) ) : NULL;
   vars->colcount =
     NOTNULL(colcount_array_origin) ?
       colcount_array_origin - vars->reg.col.low : NULL;
   mtx = vars->mtx;

   vars->rowhigh = vars->reg.row.high;
   /* Establish col counts */
   for( col = vars->reg.col.low ; col <= vars->reg.col.high ; ++col )
      vars->colcount[col] =
	 mtx_nonzeros_in_col(mtx,col,&(vars->reg.row));

   while(TRUE) {
      struct creorder_list *head;
      int32 nelts;   /* # elements "allocated" from vars->tlist */
      mtx_coord_t nz;

      cforward_triangularize(vars);
      assign_null_cols(vars);
      if( vars->reg.col.low > vars->reg.col.high ||
	 vars->reg.row.low > vars->reg.row.high ) {
	 /* Active region is now empty, done */
	 if( NOTNULL(vars->tlist) )
	    ascfree( vars->tlist );
	 if( NOTNULL(colcount_array_origin) )
	    ascfree( colcount_array_origin );
         return;
      }

      head = NULL;
      nelts = 0;
      nz.row = mtx_FIRST;
      nz.col = select_col(vars);
      while( mtx_next_in_col(mtx,&nz,&(vars->reg.row)),
	    nz.row != mtx_LAST ) {
         struct creorder_list **q,*p;

         p = &(vars->tlist[nelts++]);
         p->ndx = mtx_row_to_org(mtx,nz.row);
         p->count = mtx_nonzeros_in_row(mtx,nz.row,&(vars->reg.col));
         for( q = &head; *q && (*q)->count > p->count; q = &((*q)->next) );
         p->next = *q;
         *q = p;
      }
      /**
       ***  We now have a list of columns which intersect the selected row.
       ***  The list is in order of decreasing column count.
       **/

      /* Push incident rows on stack */
      for( ; NOTNULL(head) ; head = head->next )
         push_row_on_stack(vars,mtx_org_to_row(mtx,head->ndx));

      /* Pop row off stack and assign to selected col */
      assign_col_and_row(vars , nz.col , pop_row_from_stack(vars));
   }
   /*  Not reached. */
}

/*********************************
 end of tspk1 stuff
*********************************/

static void square_region(mtx_region_t *region, mtx_range_t *rng)
/**
 *** Get the largest square confined to the diagonal within the region given
 *** and set rng accordingly.
 **/
{
      rng->low = MAX(region->row.low,region->col.low);
      rng->high = MIN(region->row.high,region->col.high);
}

static int ranki_reorder(mtx_matrix_t mtx,mtx_region_t *region)
/**
 ***  The region to reorder is first isolated by truncating the region
 ***  provided to the largest square region confined to the matrix diagonal.
 ***  It is presumed it will contain no empty rows or columns and will
 ***  provide the basis of candidate pivots when factoring.
 **/
{
   struct reorder_vars vars;
   mtx_range_t rng;

   square_region(region,&rng);
   vars.mtx = mtx;
   vars.reg.row.low = vars.reg.col.low = rng.low;
   vars.reg.row.high = vars.reg.col.high = rng.high;
   mtx_spk1_reorder(&vars);
   return 0;
}

static int tranki_reorder(mtx_matrix_t mtx,mtx_region_t *region)
/**
 ***  The region to reorder is first isolated by truncating the region
 ***  provided to the largest square region confined to the matrix diagonal.
 ***  It is presumed it will contain no empty rows or columns and will
 ***  provide the basis of candidate pivots when factoring.
 **/
{
   struct creorder_vars vars;
   mtx_range_t rng;

   square_region(region,&rng);
   vars.mtx = mtx;
   vars.reg.row.low = vars.reg.col.low = rng.low;
   vars.reg.row.high = vars.reg.col.high = rng.high;
   mtx_tspk1_reorder(&vars);
   return 0;
}

/***************************************************************************\
  End of reordering functions for SPK1.
\***************************************************************************/


int mtx_reorder(mtx_matrix_t mtx,mtx_region_t *region,
                enum mtx_reorder_method m)
{
  if (!mtx_check_matrix(mtx)){
    FPRINTF(g_mtxerr,"mtx_reorder called with bad matrix\n");
    return 1;
  }
  if (region==NULL){
    FPRINTF(g_mtxerr,"mtx_reorder called with mtx_ENTIRE_MATRIX\n");
    return 1;
  }
  switch (m) {
  case mtx_SPK1:
    return ranki_reorder(mtx,region);
  case mtx_TSPK1:
    return tranki_reorder(mtx,region);
  case mtx_NATURAL:
    return 0;
  default:
    FPRINTF(g_mtxerr,"mtx_reorder called with unknown reorder method\n");
    return 1;
  }
}
#undef __MTX_C_SEEN__
