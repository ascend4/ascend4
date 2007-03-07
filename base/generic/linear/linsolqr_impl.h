/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Boyd Safrit, Joseph Zaher, Benjamin Andrew Allan
	Copyright (C) 1995 Benjamin Andrew Allan, Kirk Andre Abbott
	Copyright (C) 2006-2007 Carnegie Mellon University

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
	Implementation for hidden parts of linsolqr (these things can be opaque
	to the end user
*/

#ifndef ASC_LINSOLQR_IMPL_H
#define ASC_LINSOLQR_IMPL_H

#include "linsolqr.h"

#define D_ZERO (real64)0.0
#define ZERO (int32)0
#define D_ONE (real64)1.0

#define OK        ((int)439828676)
#define DESTROYED ((int)839276847)
#define CHECK_SYSTEM(a) check_system((a),__FILE__,__LINE__)

int check_system(linsolqr_system_t sys, char *file, int line);

void square_region(linsolqr_system_t sys,mtx_region_t *region);

void determine_pivot_range(linsolqr_system_t sys);

void ensure_lu_capacity(linsolqr_system_t sys);
void ensure_qr_capacity(linsolqr_system_t sys);
void ensure_capacity(linsolqr_system_t sys);

void zero_diagonal_elements(mtx_matrix_t mtx,
                                   int32 low, int32 high);

void zero_unpivoted_vars(linsolqr_system_t sys,
				real64 *varvalues,
				boolean transpose
);

int32 find_pivot_number(const real64 *vec,
                                     const int32 len,
                                     const real64 tol,
                                     const real64 eps,
                                     int32 * const ivec,
                                     real64 * const rvec,
                                     int32 * const maxi
);

/*-----------------------------------------------------------------------------
  DATA STRUCTURES

  Any time you modify these, please verify linsolqr_size is still correct.
*/
struct rhs_list {
   real64 *rhs;            /* Vector of rhs values */
   real64 *varvalue;       /* Solution of the linear system */
   boolean solved;         /* ? has the rhs been solved */
   boolean transpose;      /* ? transpose the coef matrix */
   struct rhs_list *next;
};

struct qr_fill_t {
  int32 cnt;     /* number of real nonzeros in a column */
  int32 col;     /* column number prior to being sorted */
  int32 fill;    /* fill if this (or if col?) were the pivot column  */
  int32 overlap; /* scratch for fillable calculation. */
};
/* fill counting structure for sparse qr */

struct lu_auxdata { /* all indexed by cur row or cur col number */
  real64 *pivlist;  /* vector of pivots (diagonal of L for ranki) */
  real64 *tmp;      /* row elimination, dependency buffer */
  int32 cap;        /* current capacity of the vectors if not null */
};
/* structure for lu algorithms */

struct qr_auxdata { /* all indexed by cur row or cur col number */
  mtx_matrix_t hhvects;   /* slave matrix of sys->factors for holding u's */
  mtx_sparse_t sp;  /* sparse data space */
  real64 *alpha;    /* column norms for the Q\R factor matrix */
  real64 *sigma;    /* column norms for the S (R inverse) matrix */
  real64 *tau;      /* column tau of the Householder transforms */
  real64 *hhcol;    /* Householder transform scratch vector */
  real64 *hhrow;    /* Householder transform scratch vector */
  real64 nu;        /* current condition number of inverse */
  real64 anu;       /* condition number of coef */
  real64 asubnu;    /* condition number of active region in coef */
  struct qr_fill_t *fill; /* fill counting structure */
  mtx_region_t facreg;    /* region over which qr is done if nonsquare */
  int32 cap;        /* current capacity of these vectors if not null */
};
/* structure for CondQR algorithms */

struct linsolqr_header {
   int integrity;
   enum factor_class fclass;     /* Type of factoring expected */
   enum factor_method fmethod;   /* Method factoring expected */
   enum reorder_method rmethod;  /* Type of most recent reordering */
   int32 capacity;               /* Capacity of arrays */
   mtx_matrix_t coef;            /* Coefficient matrix */
   struct rhs_list *rl;          /* List of rhs vectors */
   int rlength;                  /* Length of rhs list */
   real64 pivot_zero;            /* Smallest acceptable pivot */
   real64 ptol;                  /* Pivot selection tolerance */
   real64 ctol;                  /* Condition selection tolerance */
   real64 dtol;                  /* Matrix entry drop tolerance */
   mtx_matrix_t factors;         /* Matrix with UL and dependence info (ranki),
                                    or L and dependence info  (ranki2),
                                    or R and dependence info (qr methods)  */
   mtx_matrix_t inverse;         /* Matrix with U multipliers (ranki2) or R^-1
                                    (condqr), in both cases slave of factors */
   boolean factored;             /* ? has the matrix been factored */
   boolean rowdeps;              /* ? have row dependencies been calc'ed */
   boolean coldeps;              /* ? have col dependencies been calc'ed */
   mtx_range_t rng;              /* Pivot range */
   mtx_region_t reg;             /* Bounding region given */
   int32 rank;                   /* Rank of the matrix */
   real64 smallest_pivot;        /* Smallest pivot accepted */
   struct qr_auxdata *qrdata;    /* Data vectors for qr methods */
   struct lu_auxdata *ludata;    /* Data vectors for lu methods */
};
/* linsol main structure */

#endif
