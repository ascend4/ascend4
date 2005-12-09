/*
 *  linsol: Ascend Linear Solver
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: linsol.c,v $
 *  Date last modified: $Date: 2000/01/25 02:26:54 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
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
#include "compiler/compiler.h"
#include "utilities/ascMalloc.h"
#include "utilities/mem.h"
#include "utilities/set.h"
#include "general/tm_time.h"
#include "solver/mtx.h"
#include "solver/linsol.h"


#define LINSOL_DEBUG FALSE

struct rhs_list {
   real64 *rhs;            /* Vector of rhs values */
   real64 *varvalue;       /* Solution of the linear system */
   boolean solved;               /* ? has the rhs been solved */
   boolean transpose;            /* ? transpose the coef matrix */
   struct rhs_list *next;
};

struct linsol_header {
   int integrity;
   int32 capacity;         /* Capacity of arrays */
   mtx_matrix_t coef;            /* Coefficient matrix */
   struct rhs_list *rl;          /* List of rhs vectors */
   int rlength;                  /* Length of rhs list */
   real64 pivot_zero;      /* Smallest exceptable pivot */
   real64 ptol;            /* Pivot selection tolerance */
   mtx_matrix_t inv;             /* Inverse matrix containing UL factors */
   boolean inverted;             /* ? has the matrix been inverted */
   mtx_range_t rng;              /* Pivot range */
   mtx_region_t reg;             /* Bounding region */
   int32 rank;             /* Rank of the matrix */
   real64 smallest_pivot;  /* Smallest pivot accepted */
};


#define OK        ((int)439828676)
#define DESTROYED ((int)839276847)
static void check_system(linsol_system_t sys)
/**
 ***  Checks the system handle
 **/
{
   if( sys == NULL ) {
      FPRINTF(stderr,"ERROR:  (linsol) check_system\n");
      FPRINTF(stderr,"        NULL system handle.\n");
      return;
   }

   switch( sys->integrity ) {
   case OK:
      break;
   case DESTROYED:
      FPRINTF(stderr,"ERROR:  (linsol) check_system\n");
      FPRINTF(stderr,"        System was recently destroyed.\n");
      break;
   default:
      FPRINTF(stderr,"ERROR:  (linsol) check_system\n");
      FPRINTF(stderr,"        System was reused or never created.\n");
      break;
   }
}

static void destroy_rhs_list(struct rhs_list *rl)
/**
 ***  Destroys rhs list.
 **/
{
   while( rl != NULL ) {
      struct rhs_list *p;
      p = rl;
      rl = rl->next;
      if( p->varvalue != NULL )
	 ascfree( (POINTER)(p->varvalue) );
      ascfree( (POINTER)p );
   }
}

static struct rhs_list *find_rhs(struct rhs_list *rl, real64 *rhs)
/**
 ***  Searches for rhs in rhs list, returning it or NULL if not found.
 **/
{
   for( ; rl != NULL ; rl = rl->next )
      if( rl->rhs == rhs )
         break;
   return(rl);
}


linsol_system_t linsol_create()
{
   linsol_system_t sys;

   sys = (linsol_system_t)ascmalloc( sizeof(struct linsol_header) );
   sys->integrity = OK;
   sys->capacity = 0;
   sys->coef = NULL;
   sys->rl = NULL;
   sys->rlength = 0;
   sys->pivot_zero = 1e-12;   /* default value */
   sys->ptol = 0.1;           /* default value */
   sys->inv = NULL;
   sys->inverted = FALSE;
   sys->rng.low = sys->rng.high = -1;
   sys->reg.row.low = sys->reg.row.high = -1;
   sys->reg.col.low = sys->reg.col.high = -1;
   sys->rank = -1;
   sys->smallest_pivot = 0.0;
   return(sys);
}

void linsol_destroy(linsol_system_t sys)
{
   check_system(sys);
   if( sys->inv != NULL ) {
     mtx_destroy(sys->inv);
   }
   destroy_rhs_list(sys->rl);
   sys->integrity = DESTROYED;
   ascfree( (POINTER)sys );
}

void linsol_set_matrix(linsol_system_t sys, mtx_matrix_t mtx)
{
   check_system(sys);
   sys->coef = mtx;
}

mtx_matrix_t linsol_get_matrix(linsol_system_t sys)
{
   check_system(sys);
   return( sys->coef );
}

mtx_matrix_t linsol_get_inverse(linsol_system_t sys)
{
  check_system(sys);
  return (sys->inv);
}

void linsol_add_rhs(linsol_system_t sys, real64 *rhs, boolean transpose)
{
  struct rhs_list *rl;
  int32 capacity;

  check_system(sys);
  rl = find_rhs(sys->rl,rhs);
  if( rl != NULL ) {
    return;
  } else if( rhs != NULL ) {
    rl = (struct rhs_list *)ascmalloc( sizeof(struct rhs_list) );
    rl->rhs = rhs;
    rl->varvalue = NULL;
    rl->solved = FALSE;
    rl->transpose = transpose;
    rl->next = sys->rl;
    sys->rl = rl;
    ++(sys->rlength);
    if (sys->coef) {
      capacity = mtx_capacity(sys->coef);
      if (capacity) {
	rl->varvalue =(real64 *)
	  ascmalloc(capacity*sizeof(real64));
      }
    }
  }
}

void linsol_remove_rhs(linsol_system_t sys, real64 *rhs)
{
   struct rhs_list **q;

   check_system(sys);
   for( q = &(sys->rl) ; *q != NULL ; q = &((*q)->next) )
      if( (*q)->rhs == rhs )
	 break;
   if( *q != NULL ) {
      struct rhs_list *p;
      p = *q;
      *q = p->next;
      if( p->varvalue != NULL )
	 ascfree( (POINTER)(p->varvalue) );
      ascfree( (POINTER)p );
      --(sys->rlength);
   } else if( rhs != NULL ) {
      FPRINTF(stderr,"ERROR:  (linsol) linsol_remove_rhs\n");
      FPRINTF(stderr,"        Rhs does not exist.\n");
   }
}

int linsol_number_of_rhs(linsol_system_t sys)
{
   check_system(sys);
   return( sys->rlength );
}

real64 *linsol_get_rhs(linsol_system_t sys, int n)
{
   struct rhs_list *rl;
   int count;

   check_system(sys);

   count = sys->rlength - 1 - n;
   if( count < 0 )  return(NULL);
   for( rl = sys->rl ; count > 0 && rl != NULL ; rl = rl->next )
      --count;
   return( rl == NULL ? NULL : rl->rhs );
}

void linsol_matrix_was_changed(linsol_system_t sys)
{
   check_system(sys);
   sys->inverted = FALSE;
}

void linsol_rhs_was_changed(linsol_system_t sys,real64 *rhs)
{
   struct rhs_list *rl;

   check_system(sys);
   rl = find_rhs(sys->rl,rhs);
   if( rl != NULL ) {
      rl->solved = FALSE;
   } else if( rhs != NULL ) {
      FPRINTF(stderr,"ERROR:  (linsol) linsol_rhs_was_modified\n");
      FPRINTF(stderr,"        Rhs does not exist.\n");
   }
}

void linsol_set_pivot_zero(linsol_system_t sys,real64 pivot_zero)
{
   check_system(sys);
   if( pivot_zero < 0.0 ) {
      FPRINTF(stderr,"ERROR:  (linsol) linsol_set_pivot_zero\n");
      FPRINTF(stderr,"        Invalid pivot zero of %Lf\n",pivot_zero);
   } else {
      sys->pivot_zero = pivot_zero;
      linsol_matrix_was_changed(sys);
   }
}

real64 linsol_pivot_zero(linsol_system_t sys)
{
   check_system(sys);
   return( sys->pivot_zero );
}

void linsol_set_pivot_tolerance(linsol_system_t sys,real64 ptol)
{
   check_system(sys);
   if( ptol < 0.0 || ptol >= 1.0 ) {
      FPRINTF(stderr,"ERROR:  (linsol) linsol_set_pivot_tolerance\n");
      FPRINTF(stderr,"        Invalid pivot tolerance of %Lf\n",ptol);
   } else {
      sys->ptol = ptol;
      linsol_matrix_was_changed(sys);
   }
}

real64 linsol_pivot_tolerance(linsol_system_t sys)
{
   check_system(sys);
   return( sys->ptol );
}


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
   real64 value;
   nz.col = removed_col;
   nz.row = mtx_FIRST;
   while( value = mtx_next_in_col(vars->mtx,&nz,&(vars->reg.row)),
	 nz.row != mtx_LAST )
      --(vars->rowcount[nz.row]);
}

static void assign_row_and_col(struct reorder_vars *vars,int32 row,int32 col)
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

   do {
      mtx_coord_t nz;
      real64 value;
      change = FALSE;
      for( nz.row = vars->reg.row.low ;
          nz.row <= vars->reg.row.high && vars->rowcount[nz.row] != 1;
          ++nz.row ) ;
      if( nz.row <= vars->reg.row.high ) {
         /* found singleton row */
         nz.col = mtx_FIRST;
         value = mtx_next_in_row(vars->mtx,&nz,&(vars->reg.col));
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
   int32 i, nties;   /* # elements currently defined in vars->tlist */

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
   while( i < nties ) {
      int32 sum;
      mtx_coord_t nz;
      real64 value;

      sum = 0;
      nz.row = vars->tlist[i].ndx;
      nz.col = mtx_FIRST;
      while( value = mtx_next_in_row(vars->mtx,&nz,&(vars->reg.col)),
	    nz.col != mtx_LAST )
         sum += mtx_nonzeros_in_col(vars->mtx,nz.col,&(vars->reg.row));
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

static void reorder(struct reorder_vars *vars)
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

   size = vars->reg.row.high - vars->reg.row.low + 1;
   size = MAX(size,vars->reg.col.high - vars->reg.col.low + 1);
   vars->tlist = size > 0 ? (struct reorder_list *)
      ascmalloc( size*sizeof(struct reorder_list) ) : NULL;
   rowcount_array_origin = size > 0 ? (int32 *)
      ascmalloc( size*sizeof(int32) ) : NULL;
   vars->rowcount = rowcount_array_origin != NULL ?
      rowcount_array_origin - vars->reg.row.low : NULL;

   vars->colhigh = vars->reg.col.high;
   /* Establish row counts */
   for( row = vars->reg.row.low ; row <= vars->reg.row.high ; ++row )
      vars->rowcount[row] =
	 mtx_nonzeros_in_row(vars->mtx,row,&(vars->reg.col));

   while(TRUE) {
      struct reorder_list *head;
      int32 nelts;   /* # elements "allocated" from vars->tlist */
      mtx_coord_t nz;
      real64 value;

      forward_triangularize(vars);
      assign_null_rows(vars);
      if( vars->reg.row.low>vars->reg.row.high ||
	 vars->reg.col.low>vars->reg.col.high ) {
	 /* Active region is now empty, done */
	 if( vars->tlist != NULL )
	    ascfree( vars->tlist );
	 if( rowcount_array_origin != NULL )
	    ascfree( rowcount_array_origin );
         return;
      }

      head = NULL;
      nelts = 0;
      nz.row = select_row(vars);
      nz.col = mtx_FIRST;
      while( value = mtx_next_in_row(vars->mtx,&nz,&(vars->reg.col)),
	    nz.col != mtx_LAST ) {
         struct reorder_list **q,*p;

         p = &(vars->tlist[nelts++]);
         p->ndx = mtx_col_to_org(vars->mtx,nz.col);
         p->count = mtx_nonzeros_in_col(vars->mtx,nz.col,&(vars->reg.row));
         for( q = &head; *q && (*q)->count>p->count; q = &((*q)->next) );
         p->next = *q;
         *q = p;
      }
      /**
       ***  We now have a list of columns which intersect the selected row.
       ***  The list is in order of decreasing column count.
       **/

      /* Push incident columns on stack */
      for( ; head != NULL ; head = head->next )
         push_column_on_stack(vars,mtx_org_to_col(vars->mtx,head->ndx));

      /* Pop column off stack and assign to selected row */
      assign_row_and_col(vars , nz.row , pop_column_from_stack(vars));
   }
   /*  Not reached. */
}

static boolean nonempty_row(mtx_matrix_t mtx,int32 row)
/**
 ***  ? row not empty in mtx.
 **/
{
   mtx_coord_t nz;
   real64 value;
   value = mtx_next_in_row(mtx, mtx_coord(&nz,row,mtx_FIRST), mtx_ALL_COLS);
   return( nz.col != mtx_LAST );
}

static boolean nonempty_col(mtx_matrix_t mtx,int32 col)
/**
 ***  ? column not empty in mtx.
 **/
{
   mtx_coord_t nz;
   real64 value;
   value = mtx_next_in_col(mtx, mtx_coord(&nz,mtx_FIRST,col), mtx_ALL_ROWS);
   return( nz.row != mtx_LAST );
}

static void determine_pivot_range(linsol_system_t sys)
/**
 ***  Calculates sys->rng from sys->coef.
 **/
{
   sys->reg.row.low = sys->reg.row.high = -1;
   sys->reg.col.low = sys->reg.col.high = -1;

   for( sys->rng.high=mtx_order(sys->coef) ; --(sys->rng.high) >= 0 ; ) {
      if( nonempty_row(sys->coef,sys->rng.high) &&
	 sys->reg.row.high < 0 )
	 sys->reg.row.high = sys->rng.high;
      if( nonempty_col(sys->coef,sys->rng.high) &&
	 sys->reg.col.high < 0 )
	 sys->reg.col.high = sys->rng.high;
      if( nonempty_row(sys->coef,sys->rng.high) &&
         nonempty_col(sys->coef,sys->rng.high) )
         break;
   }

   for( sys->rng.low=0 ; sys->rng.low <= sys->rng.high ; ++(sys->rng.low) ) {
      if( nonempty_row(sys->coef,sys->rng.low) &&
	 sys->reg.row.low < 0 )
	 sys->reg.row.low = sys->rng.low;
      if( nonempty_col(sys->coef,sys->rng.low) &&
	 sys->reg.col.low < 0 )
	 sys->reg.col.low = sys->rng.low;
      if( nonempty_row(sys->coef,sys->rng.low) &&
         nonempty_col(sys->coef,sys->rng.low) )
         break;
   }
}

void linsol_reorder(linsol_system_t sys,mtx_region_t *region)
/**
 ***  The region to reorder is first isolated by truncating the region
 ***  provided to the largest square region confined to the matrix diagonal.
 ***  It is presumed it will contain no empty rows or columns and will
 ***  provide the basis of candidate pivots when inverting.
 **/
{
   struct reorder_vars vars;
   check_system(sys);
   if( region == mtx_ENTIRE_MATRIX )
      determine_pivot_range(sys);
   else {
      sys->reg.row.low = region->row.low;
      sys->reg.row.high = region->row.high;
      sys->reg.col.low = region->col.low;
      sys->reg.col.high = region->col.high;
      sys->rng.low = MAX(region->row.low,region->col.low);
      sys->rng.high = MIN(region->row.high,region->col.high);
   }
   vars.mtx = sys->coef;
   vars.reg.row.low = vars.reg.col.low = sys->rng.low;
   vars.reg.row.high = vars.reg.col.high = sys->rng.high;
   reorder(&vars);
}


static void drag(mtx_matrix_t mtx,int32 n1,int32 n2)
/**
 ***  Drags row n1 to n2, moving row n1 to n2 and shifting all rows in
 ***  between back toward n1.  Ditto for columns.  This function works
 ***  regardless of whether n1 < n2 or n2 < n1.
 **/
{
   while( n1 < n2 ) {
      mtx_swap_rows(mtx,n1,n1+1);
      mtx_swap_cols(mtx,n1,n1+1);
      ++n1;
   }

   while( n1 > n2 ) {
      mtx_swap_rows(mtx,n1,n1-1);
      mtx_swap_cols(mtx,n1,n1-1);
      --n1;
   }
}

static void eliminate_row(mtx_matrix_t mtx,mtx_range_t *rng,int32 row,real64 *tmp)
/**
 ***  Eliminates the given row to the left of the diagonal element, assuming
 ***  valid pivots for all of the diagonal elements above it (the elements
 ***  above those diagonal elements, if any exist, are assumed to be U
 ***  elements and therefore ignored).  The temporary array is used by this
 ***  function to do its job.  tmp[k], where rng->low <= k <= rng->high must
 ***  be defined (allocated) but need not be initialized.
 **/
{
   mtx_coord_t nz;
   real64 value;
   mtx_range_t left_to_eliminate,high_cols;

   high_cols.low = row;
   high_cols.high = rng->high;
   left_to_eliminate.low = rng->low;
   left_to_eliminate.high = row - 1;

   /* Move non-zeros left of pivot from matrix to full array */
   mem_zero_byte_cast(tmp+rng->low,0.0,(row-rng->low)*sizeof(real64));
   nz.row = row;
   nz.col = mtx_FIRST;
   while( value = mtx_next_in_row(mtx,&nz,&left_to_eliminate),
	 nz.col != mtx_LAST )
      tmp[nz.col] = value;
   mtx_clear_row(mtx,row,&left_to_eliminate);

   /* eliminates nzs from pivot, one by one, filling tmp with multipliers */
   for( nz.row = row-1 ; nz.row >= rng->low ; --(nz.row) ) {
      if( tmp[nz.row] == 0.0 )
         continue;   /* Nothing to do for this row */

      nz.col = nz.row;
      tmp[nz.row] /= mtx_value(mtx,&nz);
      /* tmp[nz.row] now equals multiplier */

      /* Perform "mtx_add_row" for full array part of the row */
      left_to_eliminate.high = nz.row - 1;
      nz.col = mtx_FIRST;
      while( value = mtx_next_in_row(mtx,&nz,&left_to_eliminate),
	    nz.col != mtx_LAST )
         tmp[nz.col] -= tmp[nz.row] * value;

      /* Perform "mtx_add_row" on remaining part of row */
      mtx_add_row(mtx,nz.row,row,-tmp[nz.row],&high_cols);
   }

   nz.row = row;
   for( nz.col = rng->low ; nz.col < row ; ++(nz.col) )
      if( tmp[nz.col] != 0.0 ) mtx_add_value(mtx,&nz,tmp[nz.col]);
}

#ifdef THIS_IS_AN_UNUSED_FUNCTION
static int32 top_of_spike(linsol_system_t sys,int32 col)
/**
 ***  Determines the top row (row of lowest index) in a possible spike
 ***  above the diagonal element in the given column.  If there is no spike,
 ***  then (row = ) col is returned.
 **/
{
   mtx_range_t above_diagonal;
   mtx_coord_t nz;
   real64 value;
   int32 top_row;

   above_diagonal.low = sys->rng.low;
   above_diagonal.high = col-1;
   top_row = nz.col = col;
   nz.row = mtx_FIRST;
   while( value = mtx_next_in_col(sys->inv,&nz,&above_diagonal),
	 nz.row != mtx_LAST )
      if( nz.row < top_row )
         top_row = nz.row;
   return( top_row );
}
#endif  /* THIS_IS_AN_UNUSED_FUNCTION */

static boolean col_is_a_spike(linsol_system_t sys,int32 col)
/**
 ***  Determines if the col is a spike, characterized by having any
 ***  nonzeros above the diagonal.
 **/
{
   mtx_range_t above_diagonal;
   mtx_coord_t nz;
   real64 value;
   int32 top_row;

   above_diagonal.low = sys->rng.low;
   above_diagonal.high = col-1;
   top_row = nz.col = col;
   nz.row = mtx_FIRST;
   while( value = mtx_next_in_col(sys->inv,&nz,&above_diagonal),
	 nz.row != mtx_LAST )
      if( nz.row < col ) return TRUE;
   return( FALSE );
}

static void invert(linsol_system_t sys)
/**
 ***  This is the heart of the linear equation solver.  This function
 ***  factorizes the matrix into a lower (L) and upper (U) triangular
 ***  matrix.  sys->smallest_pivot and sys->rank are calculated.  The
 ***  RANKI method is utilized.  At the end of elimination, the matrix A
 ***  is factored into A = U L, where U and L are stored as follows:
 ***
 ***      <----- r ----> <- n-r ->
 ***     +--------------+---------+
 ***     |              |         |
 ***     |         U    |         |
 ***     |              |         |
 ***     |   L          |         | r
 ***     |              |         |
 ***     +--------------+---------+
 ***     |              |         |
 ***     |              |   0     | n-r
 ***     |              |         |
 ***     +--------------+---------+
 ***
 ***  The rows and columns have been permuted so that all of the pivoted
 ***  original rows and columns are found in the first r rows and columns
 ***  of the region.  The last n-r rows and columns are unpivoted.  U has
 ***  1's on its diagonal, whereas L's diagonal is stored in the matrix.
 ***
 ***  Everything above was written as though the entire matrix is
 ***  involved.  In reality, only the relevant square region is involved.
 **/
{
   mtx_coord_t nz;
   int32 last_row;
   mtx_range_t pivot_candidates;
   real64 *tmp,*tmp_array_origin;
   int32 length;

   length = sys->rng.high - sys->rng.low + 1;
   tmp_array_origin = length > 0 ?
      (real64 *)ascmalloc( length*sizeof(real64) ) : NULL;
   tmp = tmp_array_origin != NULL ?
      tmp_array_origin - sys->rng.low : NULL;

   sys->smallest_pivot = MAXDOUBLE;
   last_row = pivot_candidates.high = sys->rng.high;
   for( nz.row = sys->rng.low ; nz.row <= last_row ; ) {
      real64 pivot;

      pivot_candidates.low = nz.col = nz.row;
      pivot = mtx_value(sys->inv,&nz);
      pivot = fabs(pivot);
      if( pivot > sys->pivot_zero &&
	 pivot > sys->ptol * mtx_row_max(sys->inv,&nz,&pivot_candidates,NULL)
	 && !col_is_a_spike(sys,nz.row) ) {
	 /* Good pivot and not a spike: continue with next row */
         if( pivot < sys->smallest_pivot )
            sys->smallest_pivot = pivot;
         ++(nz.row);
         continue;
      }

      /**
       ***  Row is a spike row or will
       ***  be when a necessary column
       ***  exchange occurs.
       **/
      eliminate_row(sys->inv,&(sys->rng),nz.row,tmp);
      if( (pivot=mtx_row_max(sys->inv,&nz,&pivot_candidates,NULL)) <=
	 sys->pivot_zero ) {
         /* Dependent row, drag to the end */
         drag(sys->inv,nz.row,last_row);
         --last_row;
      } else {
         /* Independent row: nz contains best pivot */
         mtx_swap_cols(sys->inv,nz.row,nz.col);
	 /* Move pivot to diagonal */
	 drag( sys->inv , nz.row , sys->rng.low );
         if( pivot < sys->smallest_pivot )
            sys->smallest_pivot = pivot;
         ++(nz.row);
      }

   }
   if( tmp_array_origin != NULL )
      ascfree( (POINTER)tmp_array_origin );

   sys->rank = last_row - sys->rng.low + 1;
}

static real64 *raise_capacity(real64 *vec,int32 oldcap,int32 newcap)
/**
 ***  Raises the capacity of the array and returns a new array.
 ***  It is assumed that oldcap < newcap.  vec is destroyed or
 ***  returned as appropriate.
 **/
{
   real64 *newvec=NULL;
/*
   real64 *newvec;
   newvec = newcap > 0 ?
      (real64 *)ascmalloc( newcap * sizeof(real64) ) : NULL;
   if( vec != NULL ) {
      mem_move_cast(vec,newvec,oldcap*sizeof(real64));
      ascfree( (POINTER)vec );
   }
   return(newvec);
*/
  if (newcap < oldcap)
    return vec;
  if (vec!=NULL) /* don't call realloc on null with newcap 0 or it frees */
    newvec = (real64 *)ascrealloc(vec,(newcap * sizeof(real64)));
  else
    newvec=newcap > 0 ?
      (real64 *)ascmalloc( newcap * sizeof(real64) ) : NULL;
  return newvec;
}

static void insure_capacity(linsol_system_t sys)
/**
 ***  Insures that the capacity of all of the solution vectors
 ***  for each rhs is large enough.
 **/
{
   int32 req_cap = mtx_capacity(sys->coef);

   if( req_cap > sys->capacity ) {
      struct rhs_list *rl;

      for( rl = sys->rl ; rl != NULL ; rl = rl->next )
         rl->varvalue = raise_capacity(rl->varvalue,sys->capacity,req_cap);
      sys->capacity = req_cap;
   }
}

static void forward_substitute(linsol_system_t sys,real64 *arr,boolean transpose)
/**
 ***  Forward substitute.  It is assumed that the L (or U) part of
 ***  sys->inv is computed.  This function converts c to x in place.  The
 ***  values are stored in arr indexed by original row number (or original
 ***  column number).
 ***
 ***     transpose = FALSE:                  transpose = TRUE:
 ***                                          T
 ***     L x = c                             U x = c
 ***
 ***  The following formula holds:
 ***     0<=k<r ==> x(k) = [c(k) - L(k,0..k-1) dot x(0..k-1)] / L(k,k)
 ***  or
 ***     0<=k<r ==> x(k) = [c(k) - U(0..k-1,k) dot x(0..k-1)] / U(k,k)
 **/
{
   mtx_range_t dot_rng;
   mtx_coord_t nz;
   real64 value;

   dot_rng.low = sys->rng.low;
   if (transpose) {     /* arr is indexed by original column number */
     for( nz.col=dot_rng.low; nz.col < dot_rng.low+sys->rank; ++(nz.col) ) {
       real64 sum;
       register int32 org_col;

       dot_rng.high = nz.col - 1;
       sum = 0.0;
       nz.row = mtx_FIRST;
       while( value = mtx_next_in_col(sys->inv,&nz,&dot_rng),
	     nz.row != mtx_LAST )
	  sum += value*arr[mtx_col_to_org(sys->inv,nz.row)];

       nz.row = nz.col;
       org_col = mtx_col_to_org(sys->inv,nz.col);
       arr[org_col] = (arr[org_col] - sum) / 1.0;
     }

   } else {             /* arr is indexed by original row number */
     for( nz.row=dot_rng.low; nz.row < dot_rng.low+sys->rank; ++(nz.row) ) {
       real64 sum;
       register int32 org_row;

       dot_rng.high = nz.row - 1;
       sum = 0.0;
       nz.col = mtx_FIRST;
       while( value = mtx_next_in_row(sys->inv,&nz,&dot_rng),
	     nz.col != mtx_LAST )
	  sum += value*arr[mtx_row_to_org(sys->inv,nz.col)];

       nz.col = nz.row;
       org_row = mtx_row_to_org(sys->inv,nz.row);
       arr[org_row] = (arr[org_row] - sum) / mtx_value(sys->inv,&nz);
     }
   }
}

static void backward_substitute(linsol_system_t sys,real64 *arr,boolean transpose)
/**
 ***  Backward substitute.  It is assumed that the U (or L) part of
 ***  sys->inv is computed.  This function converts rhs to c in place.  The
 ***  values are stored in arr indexed by original row number (or original
 ***  column number).
 ***
 ***    transpose = FALSE:                  transpose = TRUE:
 ***                                         T
 ***    U c = rhs                           L c = rhs
 ***
 ***  The following formula holds:
 ***     0<=k<r ==> c(k) = [rhs(k) - U(k,k+1..r-1) dot c(k+1..r-1)] / U(k,k)
 ***  or
 ***     0<=k<r ==> c(k) = [rhs(k) - L(k+1..r-1,k) dot c(k+1..r-1)] / L(k,k)
 **/
{
   mtx_range_t dot_rng;
   mtx_coord_t nz;
   real64 value;

   dot_rng.high = sys->rng.low + sys->rank - 1;
   if (transpose) {     /* arr is indexed by original column number */
     for( nz.col = dot_rng.high ; nz.col >= sys->rng.low ; --(nz.col) ) {
       real64 sum;
       register int32 org_col;

       dot_rng.low = nz.col + 1;
       sum = 0.0;
       nz.row = mtx_FIRST;
       while( value = mtx_next_in_col(sys->inv,&nz,&dot_rng),
	     nz.row != mtx_LAST )
	  sum += value*arr[mtx_col_to_org(sys->inv,nz.row)];

       nz.row = nz.col;
       org_col = mtx_col_to_org(sys->inv,nz.col);
       arr[org_col] = (arr[org_col] - sum) / mtx_value(sys->inv,&nz);
     }

   } else {             /* arr is indexed by original row number */
     for( nz.row = dot_rng.high ; nz.row >= sys->rng.low ; --(nz.row) ) {
       real64 sum;
       register int32 org_row;

       dot_rng.low = nz.row + 1;
       sum = 0.0;
       nz.col = mtx_FIRST;
       while( value = mtx_next_in_row(sys->inv,&nz,&dot_rng),
	     nz.col != mtx_LAST )
	  sum += value*arr[mtx_row_to_org(sys->inv,nz.col)];

       nz.col = nz.row;
       org_row = mtx_row_to_org(sys->inv,nz.row);
       arr[org_row] = (arr[org_row] - sum) / 1.0;
     }
   }
}


static void calc_dependent_rows(linsol_system_t sys)
{
   mtx_coord_t nz;
   real64 value;
   mtx_range_t colrange;
   real64 *lc;

   if( (sys->reg.row.low == sys->rng.low) &&
      ( sys->reg.row.high == sys->rng.low+sys->rank-1 ) )
      return;
   if (sys->rank==0) return;

   lc = sys->capacity > 0 ? (real64 *)
      ascmalloc(sys->capacity*sizeof(real64)) : NULL;
   colrange.low = sys->rng.low;
   colrange.high = colrange.low + sys->rank - 1;

   nz.row = sys->reg.row.low;
   for( ; nz.row <= sys->reg.row.high; nz.row++ ) {
      if( nz.row == sys->rng.low ) {
	 nz.row = sys->rng.low+sys->rank-1;
	 continue;
      }
      mem_zero_byte_cast(lc,0.0,(sys->capacity)*sizeof(real64));
      nz.col = mtx_FIRST;
      while( value = mtx_next_in_row(sys->inv,&nz,&colrange),
	    nz.col != mtx_LAST )
	 lc[mtx_col_to_org(sys->inv,nz.col)] = value;
      if( nz.row < sys->rng.low+sys->rank || nz.row > sys->rng.high )
	 backward_substitute(sys,lc,TRUE);
      forward_substitute(sys,lc,TRUE);
      mtx_clear_row(sys->inv,nz.row,&colrange);
      for( nz.col=colrange.low; nz.col <= colrange.high; nz.col++ ) {
	 real64 value;
	 value = lc[mtx_col_to_org(sys->inv,nz.col)];
	 if( value != 0.0 ) mtx_add_value(sys->inv,&nz,value);
      }
   }

   if( lc ) ascfree(lc);
}


static void calc_dependent_cols(linsol_system_t sys)
{
   mtx_coord_t nz;
   real64 value;
   mtx_range_t rowrange;
   real64 *lc;

   if( (sys->reg.col.low == sys->rng.low) &&
      ( sys->reg.col.high == sys->rng.low+sys->rank-1 ) )
      return;
   if (sys->rank==0) return;

   lc = sys->capacity > 0 ? (real64 *)
      ascmalloc(sys->capacity*sizeof(real64)) : NULL;
   rowrange.low = sys->rng.low;
   rowrange.high = rowrange.low + sys->rank - 1;

   nz.col = sys->reg.col.low;
   for( ; nz.col <= sys->reg.col.high; nz.col++ ) {
      if( nz.col == sys->rng.low ) {
	 nz.col = sys->rng.low+sys->rank-1;
	 continue;
      }
      mem_zero_byte_cast(lc,0.0,sys->capacity*sizeof(real64));
      nz.row = mtx_FIRST;
      while( value = mtx_next_in_col(sys->inv,&nz,&rowrange),
	    nz.row != mtx_LAST )
	 lc[mtx_row_to_org(sys->inv,nz.row)] = value;
      if( nz.col < sys->rng.low+sys->rank || nz.col > sys->rng.high )
	 backward_substitute(sys,lc,FALSE);
      forward_substitute(sys,lc,FALSE);
      mtx_clear_col(sys->inv,nz.col,&rowrange);
      for( nz.row=rowrange.low; nz.row <= rowrange.high; nz.row++ ) {
	 real64 value;
	 value = lc[mtx_row_to_org(sys->inv,nz.row)];
	 if( value != 0.0 ) mtx_add_value(sys->inv,&nz,value);
      }
   }

   if( lc ) ascfree(lc);
}

#ifdef THIS_IS_AN_UNUSED_FUNCTION
static void debug_out_inverse(FILE *fp,linsol_system_t sys)
/**
 ***  Outputs permutation and values of the nonzero elements in the
 ***  inverse matrix.
 **/
{
   mtx_coord_t nz;
   real64 value;

   nz.row = sys->rng.low;
   for( ; nz.row <= sys->rng.high; ++(nz.row) ) {
      FPRINTF(fp,"   Row %d (org %d)\n",
	      nz.row,
	      mtx_row_to_org(sys->inv,nz.row));
      nz.col = mtx_FIRST;
      while( value = mtx_next_in_row(sys->inv,&nz,&(sys->rng)),
	    nz.col != mtx_LAST )
	 FPRINTF(fp,"      Col %d (org %d) has value %g\n",
		 nz.col, mtx_col_to_org(sys->inv,nz.col), value);
   }
}
#endif  /* THIS_IS_AN_UNUSED_FUNCTION */

#ifdef THIS_IS_AN_UNUSED_FUNCTION
/* from joe equivalent to ranki_jz*/
static void invert_alt(linsol_system_t sys)
/**
 ***  This is the heart of the linear equation solver.  This function
 ***  factorizes the matrix into a lower (L) and upper (U) triangular
 ***  matrix.  sys->smallest_pivot and sys->rank are calculated.  The
 ***  RANKI method is utilized.  At the end of elimination, the matrix A
 ***  is factored into A = U L, where U and L are stored as follows:
 ***
 ***      <----- r ----> <- n-r ->
 ***     +--------------+---------+
 ***     |              |         |
 ***     |         U    |         |
 ***     |              |         |
 ***     |   L          |         | r
 ***     |              |         |
 ***     +--------------+---------+
 ***     |              |         |
 ***     |              |   0     | n-r
 ***     |              |         |
 ***     +--------------+---------+
 ***
 ***  The rows and columns have been permuted so that all of the pivoted
 ***  original rows and columns are found in the first r rows and columns
 ***  of the region.  The last n-r rows and columns are unpivoted.  U has
 ***  1's on its diagonal, whereas L's diagonal is stored in the matrix.
 ***
 ***  Everything above was written as though the entire matrix is
 ***  involved.  In reality, only the relevant square region is involved.
 **/
{
   real64 pivot;
   real64 biggest;
   mtx_coord_t nz, best;
   mtx_region_t candidates;
   real64 *tmp;
   int32 length;

   length = sys->rng.high - sys->rng.low + 1;
   tmp = length > 0 ?
      (real64 *)ascmalloc( length*sizeof(real64) ) : NULL;

   sys->smallest_pivot = MAXDOUBLE;
   candidates.row.high = sys->rng.high;
   candidates.col.high = sys->rng.high;
   for( nz.row = sys->rng.low ; nz.row <= candidates.row.high ; ) {
      nz.col = nz.row;
      pivot = mtx_value(sys->inv,&nz);
      pivot = fabs(pivot);
      candidates.row.low = nz.row;
      candidates.col.low = nz.row;

      if( !col_is_a_spike(sys,nz.row) ) {
         best.col = nz.row;
         biggest = mtx_col_max(sys->inv,&best,&(candidates.row),NULL);
         if( biggest >= sys->pivot_zero ) {
            if( pivot < sys->pivot_zero || pivot < sys->ptol*biggest ) {
               mtx_swap_rows(sys->inv,nz.row,best.row);
               pivot = biggest;
            }
            if( pivot < sys->smallest_pivot ) sys->smallest_pivot = pivot;
            ++(nz.row);
            continue;
         }
      }

      /**
       ***  Row is a spike row or will
       ***  be when a necessary column
       ***  exchange occurs.
       **/
      eliminate_row(sys->inv,&(sys->rng),nz.row,tmp - sys->rng.low);
      pivot = mtx_row_max(sys->inv,&nz,&(candidates.col),NULL);
      if( pivot < sys->pivot_zero ) { /* pivot is an epsilon */
         /* Dependent row, drag nz to lower right */
         mtx_drag(sys->inv,nz.row,candidates.row.high);
         --(candidates.row.high);
      } else {
         /* Independent row, drag nz to upper left */
         mtx_swap_cols(sys->inv,nz.row,nz.col);
         mtx_drag(sys->inv,nz.row,sys->rng.low);
         if( pivot < sys->smallest_pivot )
            sys->smallest_pivot = pivot;
         ++(nz.row);
      }
   }
   if( tmp != NULL ) ascfree( (POINTER)tmp );
   sys->rank = candidates.row.high - sys->rng.low + 1;
}
#endif  /* THIS_IS_AN_UNUSED_FUNCTION */

#define KAA_DEBUG 0
void linsol_invert(linsol_system_t sys,mtx_region_t *region)
/**
 ***  The region to invert is first isolated by truncating the region
 ***  provided to the largest square region confined to the matrix diagonal.
 ***  It is presumed it will contain no empty rows or columns and that it has
 ***  been previously reordered using linsol_reorder.
 **/
{
   struct rhs_list *rl;
#if KAA_DEBUG
   double time;
#endif

   check_system(sys);
   if( sys->inverted )
      return;

#if KAA_DEBUG
   time = tm_cpu_time();
#endif
   if( sys->inv != NULL )
      mtx_destroy(sys->inv);
   sys->inv = mtx_copy(sys->coef);
   sys->rank = -1;
   sys->smallest_pivot = 0.0;
   for( rl = sys->rl ; rl != NULL ; rl = rl->next )
      rl->solved = FALSE;
   insure_capacity(sys);
   if( region == mtx_ENTIRE_MATRIX )
      determine_pivot_range(sys);
   else {
      sys->reg = *region;
/*
      sys->reg.row.low = region->row.low;
      sys->reg.row.high = region->row.high;
      sys->reg.col.low = region->col.low;
      sys->reg.col.high = region->col.high;
*/
      sys->rng.low = MAX(region->row.low,region->col.low);
      sys->rng.high = MIN(region->row.high,region->col.high);
   }
   invert(sys);
   calc_dependent_rows(sys);
   calc_dependent_cols(sys);
   sys->inverted = TRUE;
#if KAA_DEBUG
   time = tm_cpu_time() - time;
   FPRINTF(stderr,"Time for Inversion = %f\n",time);
#endif /* KAA_DEBUG */
}

int32 linsol_rank(linsol_system_t sys)
{
   check_system(sys);
   if( !sys->inverted ) {
      FPRINTF(stderr,"ERROR:  (linsol) linsol_rank\n");
      FPRINTF(stderr,"        System not inverted yet.\n");
   }
   return(sys->rank);
}

real64 linsol_smallest_pivot(sys)
linsol_system_t sys;
{
   check_system(sys);
#if LINSOL_DEBUG
   if( !sys->inverted ) {
      FPRINTF(stderr,"ERROR:  (linsol) linsol_smallest_pivot\n");
      FPRINTF(stderr,"        System not inverted yet.\n");
   }
#endif
   return(sys->smallest_pivot);
}

int linsol_get_pivot_sets(linsol_system_t sys,unsigned *org_rowpivots,unsigned *org_colpivots)
{
   int32 ndx;

   check_system(sys);
   if( !sys->inverted ) {
#if LINSOL_DEBUG
      FPRINTF(stderr,"ERROR:  (linsol) linsol_get_pivot_sets\n");
      FPRINTF(stderr,"        System not inverted yet.\n");
#endif
      return 0;
   }
   for( ndx = sys->rng.low ; ndx < sys->rng.low + sys->rank ; ++ndx ) {
      set_change_member(org_rowpivots,mtx_row_to_org(sys->inv,ndx),TRUE);
      set_change_member(org_colpivots,mtx_col_to_org(sys->inv,ndx),TRUE);
   }
   return 1;
}

#define org_row_to_org_col(sys,org_row) \
   mtx_col_to_org((sys)->inv,mtx_org_to_row((sys)->inv,org_row))
#define org_col_to_org_row(sys,org_col) \
   mtx_row_to_org((sys)->inv,mtx_org_to_col((sys)->inv,org_col))

int32 linsol_org_row_to_org_col(linsol_system_t sys,int32 org_row)
{
   check_system(sys);
   if( !sys->inverted ) {
      FPRINTF(stderr,"ERROR:  (linsol) linsol_org_row_to_org_col\n");
      FPRINTF(stderr,"        System not inverted yet.\n");
   }
   return( org_row_to_org_col(sys,org_row) );
}

int32 linsol_org_col_to_org_row(linsol_system_t sys,int32 org_col)
{
   check_system(sys);
   if( !sys->inverted ) {
      FPRINTF(stderr,"ERROR:  (linsol) linsol_org_col_to_org_row\n");
      FPRINTF(stderr,"        System not inverted yet.\n");
   }
   return( org_col_to_org_row(sys,org_col) );
}

real64 linsol_org_row_dependency(linsol_system_t sys,int32 dep,int32 ind)
{
   mtx_coord_t nz;

   check_system(sys);
   if( !sys->inverted ) {
#if LINSOL_DEBUG
      FPRINTF(stderr,"ERROR:  (linsol) linsol_org_row_dependency\n");
      FPRINTF(stderr,"        System not inverted yet.\n");
      FPRINTF(stderr,"        Returning 0.0.\n");
#endif
      return(0.0);
   }

   nz.col = mtx_org_to_row(sys->inv,ind);
   if( (sys->rng.low > nz.col) || (nz.col > sys->rng.low+sys->rank-1) ) {
      FPRINTF(stderr,"ERROR:  (linsol) linsol_org_row_dependency\n");
      FPRINTF(stderr,"        Original row %d is not independent.\n", ind);
      FPRINTF(stderr,"        Returning 0.0.\n");
      return(0.0);
   }

   nz.row = mtx_org_to_row(sys->inv,dep);
   if( (nz.row <= sys->rng.low+sys->rank-1) && (nz.row >= sys->rng.low) )
      return( ind == dep ? 1.0 : 0.0 );

   return(mtx_value(sys->inv,&nz));
}

real64 linsol_org_col_dependency(linsol_system_t sys,int32 dep,int32 ind)
{
   mtx_coord_t nz;

   check_system(sys);
   if( !sys->inverted ) {
#if LINSOL_DEBUG
      FPRINTF(stderr,"ERROR:  (linsol) linsol_org_col_dependency\n");
      FPRINTF(stderr,"        System not inverted yet.\n");
      FPRINTF(stderr,"        Returning 0.0.\n");
      return(0.0);
#endif
   }

   nz.row = mtx_org_to_col(sys->inv,ind);
   if( (sys->rng.low > nz.row) || (nz.row > sys->rng.low+sys->rank-1) ) {
      FPRINTF(stderr,"ERROR:  (linsol) linsol_org_col_dependency\n");
      FPRINTF(stderr,"        Original col %d is not independent.\n",ind);
      FPRINTF(stderr,"        Returning 0.0.\n");
      return(0.0);
   }

   nz.col = mtx_org_to_col(sys->inv,dep);
   if( (nz.col <= sys->rng.low+sys->rank-1) && (nz.col >= sys->rng.low) )
      return( ind == dep ? 1.0 : 0.0 );

   return(mtx_value(sys->inv,&nz));
}


static void zero_unpivoted_vars(linsol_system_t sys,real64 *varvalues,boolean transpose)
/**
 ***  Sets the values of unpivoted variables to zero.
 **/
{
   int32 ndx,order;

   order = mtx_order(sys->inv);
   for( ndx = 0 ; ndx < sys->rng.low ; ++ndx )
     if (transpose)
       varvalues[mtx_col_to_org(sys->inv,ndx)] = 0.0;
     else
       varvalues[mtx_row_to_org(sys->inv,ndx)] = 0.0;

   for( ndx = sys->rng.low + sys->rank ; ndx < order ; ++ndx )
     if (transpose)
       varvalues[mtx_col_to_org(sys->inv,ndx)] = 0.0;
     else
       varvalues[mtx_row_to_org(sys->inv,ndx)] = 0.0;
}

void linsol_solve(linsol_system_t sys,real64 *rhs)
/**
 ***  Assuming the bounding block region of the matrix has been previously
 ***  inverted, the specified rhs can then be applied.
 ***
 ***     A x = U L x = rhs.  Define c := L x, so that U c = rhs and L x = c.
 ***
 ***  or
 ***      T     T T                       T            T             T
 ***     A x = L U x = rhs.  Define c := U x, so that L c = rhs and U x = c.
 ***
 ***  The variables associated with any of the unpivoted original rows
 ***  and columns are assigned the value of zero by convention.
 **/
{
   struct rhs_list *rl;

   check_system(sys);
   if( !sys->inverted ) {
      FPRINTF(stderr,"ERROR:  (linsol) linsol_solve\n");
      FPRINTF(stderr,"        System not inverted yet.\n");
   }
   rl = find_rhs(sys->rl,rhs);
   if( rl != NULL ) {
      if( rl->solved )
	 return;
/* by my reading, these are disjoint
      mem_move_cast(rl->rhs,rl->varvalue,sys->capacity*sizeof(real64));
 */
      mem_copy_cast(rl->rhs,rl->varvalue,sys->capacity*sizeof(real64));

      backward_substitute(sys,rl->varvalue,rl->transpose);
      forward_substitute(sys,rl->varvalue,rl->transpose);

      zero_unpivoted_vars(sys,rl->varvalue,rl->transpose);
      rl->solved = TRUE;
   } else if( rhs != NULL ) {
      FPRINTF(stderr,"ERROR:  (linsol) linsol_solve\n");
      FPRINTF(stderr,"        Rhs does not exist.\n");
   }
}

real64 linsol_var_value(linsol_system_t sys, real64 *rhs, int32 ndx)
{
  struct rhs_list *rl;

  check_system(sys);
  if( !sys->inverted ) {
    FPRINTF(stderr,"ERROR:  (linsol) linsol_var_value\n");
    FPRINTF(stderr,"        System not inverted yet.\n");
  }
  rl = find_rhs(sys->rl,rhs);
  if( rl != NULL ) {
    if( !(rl->solved) ) {
      FPRINTF(stderr,"ERROR:  (linsol) linsol_var_value\n");
      FPRINTF(stderr,"        Rhs not solved yet.\n");
    }
    if( rl->transpose ) {
      /* ndx is an original row index */
      return( rl->varvalue[org_row_to_org_col(sys,ndx)] );
    } else {
      /* ndx is an original column index */
      return( rl->varvalue[org_col_to_org_row(sys,ndx)] );
    }
  } else if( rhs != NULL ) {
    FPRINTF(stderr,"ERROR:  (linsol) linsol_var_value\n");
    FPRINTF(stderr,"        Rhs does not exist.\n");
  }
  return 0.0; /* Function had no return value.  Added this line.  JDS */
}

boolean linsol_copy_solution(linsol_system_t sys,real64 *rhs,real64 *vector)
{
  struct rhs_list *rl;
  real64 *varvalue;
  int ndx,size;

  check_system(sys);
  if( !sys->inverted ) {
    FPRINTF(stderr,"ERROR:  (linsol) linsol_var_value\n");
    FPRINTF(stderr,"        System not inverted yet.\n");
    return TRUE;
  }
  rl = find_rhs(sys->rl,rhs);
  if( rl != NULL ) {
    if( !(rl->solved) ) {
      FPRINTF(stderr,"ERROR:  (linsol) linsol_var_value\n");
      FPRINTF(stderr,"        Rhs not solved yet.\n");
      return TRUE;
    }
    size = sys->capacity;
    varvalue = rl->varvalue;
    if( rl->transpose ) {	/* ndx is an original row index */
      for (ndx = 0;ndx < size;ndx++) {
	vector[ndx] = varvalue[org_row_to_org_col(sys,ndx)];
      }
    }
    else{	      /* ndx is an original column index */
      for (ndx = 0;ndx < size;ndx++) {
	vector[ndx] = varvalue[org_col_to_org_row(sys,ndx)];
      }
    }
  } else if( rhs != NULL ) {
    FPRINTF(stderr,"ERROR:  (linsol) linsol_copy_solution\n");
    FPRINTF(stderr,"        Rhs does not exist.\n");
    return TRUE;
  }
  return FALSE;
}

real64 linsol_eqn_residual(linsol_system_t sys, real64 *rhs, int32 ndx)
{
   struct rhs_list *rl;
   mtx_coord_t nz;
   real64 value;
   real64 lhs;

   check_system(sys);
   if( !sys->inverted ) {
      FPRINTF(stderr,"ERROR:  (linsol) linsol_eqn_residual\n");
      FPRINTF(stderr,"        System not inverted yet.\n");
   }
   rl = find_rhs(sys->rl,rhs);
   if( rl != NULL ) {
      if( !(rl->solved) ) {
	 FPRINTF(stderr,"ERROR:  (linsol) linsol_eqn_residual\n");
	 FPRINTF(stderr,"        Rhs not solved yet.\n");
      }
      if (rl->transpose) {
	 /* ndx is an original column index */
	 lhs = 0.0;
	 nz.col = mtx_org_to_col(sys->coef,ndx);
	 nz.row = mtx_FIRST;
	 while( value = mtx_next_in_col(sys->coef,&nz,&(sys->reg.row)),
	       nz.row != mtx_LAST )
	    lhs += value * rl->varvalue
	       [org_row_to_org_col(sys,mtx_row_to_org(sys->coef,nz.row))];
	 return( lhs - rl->rhs[ndx] );
      } else {
	 /* ndx is an original row index */
	 lhs = 0.0;
	 nz.row = mtx_org_to_row(sys->coef,ndx);
	 nz.col = mtx_FIRST;
	 while( value = mtx_next_in_row(sys->coef,&nz,&(sys->reg.col)),
	       nz.col != mtx_LAST )
	    lhs += value * rl->varvalue
	       [org_col_to_org_row(sys,mtx_col_to_org(sys->coef,nz.col))];
	 return( lhs - rl->rhs[ndx] );
      }
   } else if( rhs != NULL ) {
      FPRINTF(stderr,"ERROR:  (linsol) linsol_eqn_residual\n");
      FPRINTF(stderr,"        Rhs does not exist.\n");
   }
  return 0.0; /* Function had no return value.  Added this line.  JDS */
}
