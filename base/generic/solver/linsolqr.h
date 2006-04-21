/*
 *  linsol II: Ascend Linear Equation Solver
 *  by Karl Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.13 $
 *  Version control file: $RCSfile: linsolqr.h,v $
 *  Date last modified: $Date: 1997/07/25 17:23:49 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
 *  Copyright (C) 1995 Benjamin Andrew Allan, Kirk Andre' Abbott
 *
 *  QR options by Ben Allan
 *  based on sqr.pas v1.5: Copyright (C) 1994 Boyd Safrit
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

/** @file
 *  linsol II:  Ascend Linear Equation Solver.
 *  <pre>
 *  Contents:     ASCEND generalized linear equation solver module
 *
 *  Authors:      Numerous. See copyrights above and methods below.
 *
 *  Dates:        06/90 - original version (KW)
 *                04/91 - removed output assignment and partitioning
 *                        (belong in structural analysis) (JZ)
 *                08/92 - added transpose ability (JZ)
 *                01/94 - broke out linsol_invert() and linsol_solve() (JZ)
 *                10/94 - reorganized for multiple methods and QR (BA)
 *                        moved drag operator to mtx (BA,JZ)
 *                11/94 - added linsolqr_get_factors/inverse for debugging.BA
 *                09/95 - added factor_class to better manage families. BA
 *                09/95 - added ranki_*2 methods (BA)
 *                09/95 - added gauss (broken) (KA)
 *                11/96 - added ranki_ba2 method (BA)
 *
 *  Description:  A linear system consists of a coefficient matrix (A)
 *                and possibly several right-hand-sides (rhs).  The
 *                solution vector x sought can be that of either
 *
 *                                         T
 *                   A x = rhs      or    A x = rhs
 *
 *                depending on a specification inherent with rhs which
 *                dictates whether or not A should be transposed.  If a
 *                rhs specifies transpose, then the vector itself is
 *                expected to be indexed by the original column numbers
 *                of the coefficient matrix and the solution vector shall
 *                be indexed by original row numbers.  Otherwise, rhs
 *                is expected to be indexed by original row numbers while
 *                the solution can be referenced using original column
 *                numbers.  The coefficient matrix and each rhs will be
 *                preserved throughout solving, except that the
 *                coefficient matrix may be permuted during reordering
 *                or (depending on the method) solving.
 *
 *                The method of linear solution is an option new to
 *                linsolqr. Also, some of the data structures have been
 *                expanded as required. The factoring methods are
 *                enumerated and summarized below.
 *
 *                Linsol is a general sparse solver. It uses mtx, though any
 *                proper sparse matrix package can be substituted. By
 *                proper, of course, we mean one with at least the
 *                functionality of mtx. If you find something with all
 *                the features of mtx that is faster than mtx, let us
 *                know, please. If you do any comparisons of linsol with
 *                other sparse codes (on sequential processors) also
 *                please let us know how you find its performance.
 *                You probably won't find algorithms for banded or
 *                symmetric matrices here. Why?
 *                The efficiency of most banded algorithms comes from
 *                the way they access memory with three assumptions:
 *                that all possible coefficient locations are known before
 *                a solution is attempted, that they are accessible
 *                in some ORDERED fashion, and that the matrix will not
 *                be reordered (except perhaps in very limited ways)
 *                during solution. Linsol is predicated on just
 *                the opposite assumptions at the moment.
 *                These assumptions would relieve the solver of
 *                memory allocation and a great deal of range checking.
 *                While we would do our flops efficiently with linsol/mtx
 *                on a banded matrix, the cost of accessing linear
 *                coefficients in an UNORDERED fashion is such that a good
 *                band, variable band, or symmetric skyline code making the
 *                ORDERED assumption will always beat linsol in speed,
 *                though perhaps not in robustness.
 *
 *  Requires:     #include <string.h>
 *                #include "utilities/ascConfig.h"
 *                #include "mtx.h"
 *
 *  Each method is described here, including the unimplemented ones,
 *  however vaguely. By convention all methods assign unpivotable variables
 *  the value of 0.0 in the linear solution vector. This makes sense because
 *  the linear system usually arises from a nonlinear scheme wherein
 *  stepping in the unpivoted variables is left for a future iteration.
 *
 *  The currently recommended methods are ranki_*2, in particular ba2.
 *
 *  ranki_kw:
 *    Reordering method: SPK1, Stadtherr & Wood (1984), and like.
 *      The region used is confined to the main diagonal.
 *      The square region is reordered to lower triangular with spikes.
 *      Each spike is higher than or equal to all spikes to the left of it
 *      if the matrix is partitioned.
 *    Solution method: RANKI, Stadtherr & Wood
 *      Ref: Computers and Chemical Engineering (8)1 pp. 9-33  (1984)
 *      The square region is U/L factored with spikes dragged in to replace
 *      unacceptable pivots. Fill is in spike columns and
 *      dragged rows where it usually does take place.
 *      Numerically dependent rows and columns do not have dependencies
 *      automatically calculated. This is a change from linsol, so be
 *      careful in making comparisons.
 *    Authors: Karl Westerberg, Joe Zaher
 *      with some clean ups and restructuring by Ben Allan
 *
 *  ranki_jz:
 *    As ranki_kw, except where ranki_kw looks only across the current row
 *    for pivots if the diagonal element is too small, ranki_jz also looks
 *    down the current column. This introduces additional spikes to the
 *    current block, but sometimes the overall effect is reduced fill. The
 *    main motivation is to broaden pivot choices on those problems
 *    where restricting the choice to the current row results in immediate
 *    explosions of fill due to a full subdiagonal and a moderately small
 *    pivot.
 *
 *  ranki_kw2:
 *  ranki_jz2:
 *    As the ranki_XX factorizations, except uses drop tolerance and
 *    uses much faster bilevel matrices (master + slave matrix of mtx)
 *    to segregate the contents of U from L. Also, dependency information
 *    is not calculated automatically.
 *    Author: Ben Allan
 *
 *  ranki_ba2:
 *    As the ranki_kw2 factorization, except pivot and row being eliminated
 *    are stored as linked lists/full org vectors (dual access) during
 *    elimination which removes the last significant quadratic effect from
 *    the ranki factorization.
 *
 *  plain_qr:
 *    Reordering method: Transpose SPK1 (tspk1), LORA
 *      The region used may be rectangular, but will be modified
 *      if needed so that the upper left corner is on the diagonal.
 *      The region in sys->coef is not permuted in the solution process.
 *    Solution method:
 *      The rectangular region is QR factored with standard column pivoting
 *      modified by the pivot_tolerance. That is, at each step the column
 *      to factor in next is the leftmost col with norm alpha that satisfies
 *      alpha >= ptol * maxalpha.
 *      ptol = 1 --> Stewart's QRDC style pivoting.
 *      ptol = 0 --> no pivoting (Sometimes a dumb idea).
 *      plain_qr is to QRDC as ranki_kw is to LU factorization with strict
 *      partial pivoting.
 *      This implementation is best used on matrices where nrows about = ncols
 *      and data are not highly correlated.
 *      There are better QR methods (not available here) for vector processors,
 *      distributed processors, data fitting and so forth.
 *      Some care has been taken that the implementation here will scale up
 *      reasonably to larger matrices.
 *      Detailed references are given in the plain_qr section of the .c file.
 *  Author: Ben Allan
 *
 *  cond_qr: (broken)
 *    Reordering method: Transpose SPK1 (tspk1)
 *      The region used is confined to the main diagonal.
 *      The square region is permuted in the solution process.
 *    Solution method:
 *      The square region is QR factored with pivoting by a local minimum
 *      fill criteria balanced against choosing pivots based on the
 *      incremental inverse R condition number.
 *      Ref:Incremental Condition Calculation and Column Selection,
 *          UMIACS TR 90-87, G.W.Stewart, July 1990)
 *      (Allan, Safrit, Westerberg, 1995)
 *  Authors: Ben Allan
 *
 *  opt_qr:
 *    As cond_qr, except region may be underspecified (MxN, M < N)
 *    The region's upper left corner should be (or will be forced to be)
 *    on the main diagonal. The columns pivoted are expected to be a
 *    good basis. Solution is not a least squares solution of the
 *    transpose NxM problem; for this we need rowwise Householder
 *    transforms and spk1 reordering.
 *  Authors: Ben Allan
 *
 *  ls_qr:
 *    As cond_qr, except region may be overspecified (MxN, M > N)
 *    Solves the system in a linear least squares sense.
 *  Authors: Ben Allan
 *
 *  band_lu:
 *  symmetric_lu:
 *  (cholesky?)
 *    The best codes we can find already implemented to complement and/or
 *    verify our own. More detailed semantics unknown at this time.
 *
 *  The parameters pivot_tolerance, condition_tolerance and drop_tolerance
 *  have semantics varying with the method.
 *  See the header of linsolqr_set_pivot_tolerance below for details.
 *  </pre>
 */

#ifndef ASC_LINSOLQR_H
#define ASC_LINSOLQR_H

#define LINSOLMTX_DEBUG FALSE
/**< Debug mode. */
#define LINQR_DROP_TOLERANCE 1.0e-16
/**< This is the default for drop tolerances in methods which use a drop tol */

typedef struct linsolqr_header *linsolqr_system_t;
/**<  linsolqr_system_t is the linear system handle. */

enum factor_class {
  unknown_c,      /**< error handling class */
  ranki = 100,    /**< all ranki (and gauss until broken out) methods */
  s_qr = 200      /**< all sparse qr methods */
};

enum reorder_method {
  unknown_r,       /**< error handling method */
  natural = 1000,  /**< do nothing reorder */
  spk1 = 2000,     /**< Stadtherr's SPK1 reordering */
  tspk1 = 3000     /**< transpose of Stadtherr's SPK1 reordering good for gauss */
  /* future work:
  invspk1,      spk1 then diagonally inverted
  invtspk1,     tspk1 then diagonally inverted
  widespk1,     spk1 of an MxN region, N > M
  */
};

enum factor_method {
  unknown_f = 0,    /**< error handling method */
  ranki_kw = 1,     /**< original linsol method */
  ranki_jz = 2,     /**< original linsol method with pseudo-complete pivoting */
  ranki_kw2 = 3,    /**< better data structure version of ranki_kw, w/drop tol */ 
  ranki_jz2 = 4,    /**< better data structure version of ranki_jz, w/drop tol */ 
  ranki_ka = 12,    /**< kirks hacked verion of the basic method. */
  plain_qr = 5,     /**< rectangular col pivoted qr variants */
  cond_qr = 6,      /**< stewart-based sparse QR method new Coke */
  ranki_ba2 = 7,    /**< proper linked list implementation of ranki (dragfree) */
  opt_qr = 8,       /**< coming soon */
  ls_qr = 9,        /**< anticipated */
  gauss_ba2 = 10,   /**< anticipated */
  symmetric_lu = 11 /**< anticipated */
};

extern int g_linsolqr_timing;
/**< 
 * If nonzero and certain internal defines are set, factorization
 * generates a fill and timing message.
 */

/* KAA_DEBUG */
/* unimplemented function
extern boolean linsolqr_col_is_a_spike (mtx_matrix_t mtx, int32 col);
*/
/* Returns true if the column in question is a spike. */


/* Functions for managing interfaces */

extern char *linsolqr_rmethods(void);
/**<
 * <!--  s=linsolqr_rmethods();                                        -->
 *
 * Returns a , separated list of the names of reordering methods
 * implemented. If you implement a new method, update this
 * function. Do not free the pointer returned.<br><br>
 *
 * Not all reorderings are appropriate to all factorizations.
 */

ASC_DLLSPEC(char *) linsolqr_fmethods(void);
/**<
 * <!--  s=linsolqr_fmethods();                                        -->
 *
 * Returns a , separated list of the names of factorization methods
 * implemented. If you implement a new method, update this
 * function. Do not free the pointer returned. The string returned
 * will contain no whitespace of any sort, tabs, blanks, or \n.
 */

extern enum reorder_method linsolqr_rmethod_to_enum(char *s);
/**<
 * <!--  meth=linsolqr_rmethod_to_enum(s);                             -->
 *
 * Returns the enum of a reorder method with the name s.
 * If you implement a new method, update this function.
 */

extern enum factor_method linsolqr_fmethod_to_enum(char *s);
/**<
 * <!--  meth=linsolqr_fmethod_to_enum(s);                             -->
 *
 * Returns the enum of a factor method with the name s.
 * If you implement a new method, update this function.
 */

extern enum factor_class linsolqr_fmethod_to_fclass(enum factor_method fm);
/**<
 * <!--  class=linsolqr_fmethod_to_fclass(fm);                         -->
 *
 * Returns the enum of the factor class containing the method given.
 * If you implement a new method or class, update this function.
 */

extern char *linsolqr_enum_to_rmethod(enum reorder_method m);
/**<
 * <!--  s=linsolqr_enum_to_rmethod(m);                                -->
 *
 * Returns the name of a reorder method with the enum m.
 * If you implement a new method, update this function.
 * Do not free the name.
 */

extern char *linsolqr_enum_to_fmethod(enum factor_method m);
/**<  
 * <!--  s=linsolqr_enum_to_fmethod(m);                                -->
 *
 * Returns the name of a factor method with the enum m.
 * If you implement a new method, update this function.
 * Do not free the name.
 */

extern char *linsolqr_rmethod_description(enum reorder_method meth);
/**<  
 * <!--  description=linsolqr_rmethod_description(meth);               -->
 *
 * Returns a string describing the method inquired on. Do not mess
 * with the string: linsolqr owns it.
 * If you implement a new method, update this function.
 */

extern char *linsolqr_fmethod_description(enum factor_method meth);
/**<  
 * <!--  description=linsolqr_fmethod_description(meth);               -->
 *
 * Returns a string describing the method inquired on. Do not mess
 * with the string: linsolqr owns it.
 * If you implement a new method, update this function.
 */

/* Functions for specifying problems and controlling them */

extern linsolqr_system_t linsolqr_create(void);
/**< 
 *  <!--  sys = linsolqr_create()                                      -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *
 *  Creates a linear system and returns a pointer to it.  Initially the
 *  system has no coefficient matrix and no rhs.
 */

extern void linsolqr_destroy(linsolqr_system_t sys);
/**< 
 *  <!--  linsolqr_destroy(sys)                                        -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *
 *  Destroys the linear system.  The coefficient matrix and each rhs are
 *  not destroyed by this call.
 */

extern void linsolqr_set_matrix(linsolqr_system_t sys, mtx_matrix_t mtx);
/**<
 *  <!--  linsolqr_set_matrix(sys,mtx)                                 -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *  <!--  mtx_matrix_t mtx;                                            -->
 *
 *  Sets the coefficient matrix to mtx.
 */

extern void linsolqr_set_region(linsolqr_system_t sys, mtx_region_t region);
/**<
 *  <!--  linsolqr_set_region(sys,region)                              -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *  <!--  mtx_region_t region;                                         -->
 *
 *  Sets the reg to region.
 */

extern mtx_matrix_t linsolqr_get_matrix(linsolqr_system_t sys);
/**< 
 *  <!--  mtx = linsolqr_get_matrix(sys)                               -->
 *  <!--  mtx_matrix_t mtx;                                            -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *
 *  Returns the coefficient matrix.
 */

extern void linsolqr_add_rhs(linsolqr_system_t sys, 
                             real64 *rhs,
                             boolean transpose);
/**<
 *  <!--  linsolqr_add_rhs(sys,rhs,transpose)                          -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *  <!--  real64 *rhs;                                                 -->
 *  <!--  boolean transpose;                                           -->
 *
 *  Adds the given rhs to the collection of rhs's already part of the
 *  system.
 *
 *  You continue to be responsible for deallocating rhs. We do not take
 *  over management of that memory. Do not free the rhs you have given
 *  us without calling linsolqr_remove_rhs first.<br><br>
 *
 *  Rhs should point to an array of reals indexed by original
 *  column number if the linear system is to be solved using the transpose
 *  of the matrix or by original row number if the matrix is not to be
 *  transposed.  This is determined using the boolean transpose.  The
 *  rhs should be refered to in the future by its pointer.
 *  <pre>
 *  For the mathematically impaired:
 *  M' denotes Transpose(M).
 *
 *  transpose==FALSE --> rhs will be solved for x in A x = b.
 *                   --> rhs b is an org row indexed column vector.
 *                   --> x is an org col indexed column vector.
 *                   --> yields dx = Newton direction in J.dx = -f
 *                       if the b given is -f.
 *
 *  transpose==TRUE  --> rhs will be solved for y in A' y = d.
 *                   --> rhs d is an org column indexed column vector.
 *                   --> y is an org row indexed column vector.
 *                   --> yields dx = Newton direction in J'.dx = -f.
 *                       if the d given is -f and f,dx are properly indexed.
 *                       This may be useful if A was created using
 *                       mtx_transpose in order to improve factorization.
 *
 *  Useful matrix identity: (M')^-1 == (M^-1)' for invertible M.
 *  </pre>
 */

extern void linsolqr_remove_rhs(linsolqr_system_t sys, real64 *rhs);
/**< 
 *  <!--  linsolqr_remove_rhs(sys,rhs)                                 -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *  <!--  real64 *rhs;                                                 -->
 *
 *  Removes the given rhs from the system.  The rhs is not destroyed, just
 *  removed from the system.
 */

extern int32 linsolqr_number_of_rhs(linsolqr_system_t sys);
/**< 
 *  <!--  nrhs = linsolqr_number_of_rhs(sys)                           -->
 *  <!--  int32 nrhs;                                                  -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *
 *  Returns the number of rhs's currently part of the system.
 */

extern real64 *linsolqr_get_rhs(linsolqr_system_t sys, int n);
/**< 
 *  <!--  rhs = linsolqr_get_rhs(sys,n)                                -->
 *  <!--  real64 *rhs;                                                 -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *  <!--  int n;                                                       -->
 *
 *  Returns the n-th rhs, where rhs's are indexed in the order they were
 *  added using linsolqr_add_rhs() from 0 to (# rhs's)-1.  NULL is returned
 *  if the index is out of range.
 */

extern void linsolqr_matrix_was_changed(linsolqr_system_t sys);
/**< 
 *  <!--  linsolqr_matrix_was_changed(sys)                             -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *
 *  Informs the solver that a numerical value of a non-zero was changed.
 *  This must be called whenever any numerical changes to the matrix are
 *  made.
 */

extern void linsolqr_rhs_was_changed(linsolqr_system_t sys,
                                     real64 *rhs);
/**<
 *  <!--  linsolqr_rhs_was_changed(sys,rhs)                            -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *  <!--  real64 *rhs;                                                 -->
 *
 *  Informs the solver that the given rhs has been modified.  This must be
 *  called whenever the rhs is modified.
 */

extern void linsolqr_set_pivot_zero(linsolqr_system_t sys, 
                                    real64 pivot_zero);
/**<
 *  Sets the pivot zero for the system.  Pivots less than or equal to
 *  this value are regarded as zero.  linsolqr_set_pivot_zero() will
 *  automatically call linsolqr_matrix_was_changed().
 */
extern real64 linsolqr_pivot_zero(linsolqr_system_t sys);
/**<
 *  <!--  linsolqr_set_pivot_zero(sys,pivot_zero)                      -->
 *  <!--  pivot_zero = linsolqr_pivot_zero(sys)                        -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *  <!--  real64 pivot_zero;                                           -->
 *
 *  <!--  Sets/gets the pivot zero for the system.  Pivots less than or equal to  -->
 *  <!--  this value are regarded as zero.  linsolqr_set_pivot_zero() will        -->
 *  <!--  automatically call linsolqr_matrix_was_changed().            -->
 *  Gets the pivot zero for the system.  Pivots less than or equal to
 *  this value are regarded as zero.
 */

extern void linsolqr_set_pivot_tolerance(linsolqr_system_t sys, real64 ptol);
/**< See discussion under linsolqr_drop_tolerance(). */
extern real64 linsolqr_pivot_tolerance(linsolqr_system_t sys);
/**< See discussion under linsolqr_drop_tolerance(). */
extern void linsolqr_set_condition_tolerance(linsolqr_system_t sys, real64 ctol);
/**< See discussion under linsolqr_drop_tolerance(). */
extern real64 linsolqr_condition_tolerance(linsolqr_system_t sys);
/**< See discussion under linsolqr_drop_tolerance(). */
extern void linsolqr_set_drop_tolerance(linsolqr_system_t sys, real64 dtol);
/**< See discussion under linsolqr_drop_tolerance(). */
extern real64 linsolqr_drop_tolerance(linsolqr_system_t sys);
/**< 
 *  <pre>
 *  linsolqr_set_pivot_tolerance(sys,ptol)
 *  ptol = linsolqr_pivot_tolerance(sys)
 *
 *  linsolqr_set_condition_tolerance(sys,ctol)
 *  ctol = linsolqr_condition_tolerance(sys)
 *
 *  linsolqr_set_drop_tolerance(sys,dtol)
 *  dtol = linsolqr_drop_tolerance(sys)
 *
 *  linsolqr_system_t sys;
 *  real64 ptol, ctol,dtol;
 *
 *  Sets/gets the pivot/condition/drop tolerance for the system. Semantics of
 *  tolerances vary with method. Not to be confused with pivot zero, epsilon.
 *
 *  pivot_tolerance: Pivots less than this fraction of the maximum pivot
 *  value in the same row or column (depending on method) are disregarded.
 *  
 *  ranki_kw: pivot_tolerance applies to row. condition_tolerance ignored. 
 *  ranki_jz: pivot_tolerance applies to row and col. condition ignored.
 *  ranki_ba2:
 *  ranki_kw2:
 *  ranki_jz2: as ranki_kw/ranki_jz except that matrix entries below
 *             dtol in magnitude are dropped.
 *  plain_qr: pivot_tolerance applies to cols. condition_tolerance used
 *            with a condition heuristic rather than actual condition.
 * 
 *  cond_qr and variants:
 *  Unpivoted columns are ordered by fill potential and then the first
 *  of these to meet the criterion CI*condition_tolerance <= min_CI
 *  is chosen as the next column to pivot with.
 *  Pivot_tolerance is applied to choose the pivot within the
 *  selected column.
 *
 *  linsolqr_set_pivot/condition/drop_tolerance() will automatically call
 *  linsolqr_matrix_was_changed().
 *  </pre>
 *  @todo Separate documentation for these linsolqr functions?
 */

/* Functions for analyzing and querying linear systems. */

extern enum factor_class linsolqr_fclass(linsolqr_system_t sys);
/**< 
 *  <!--  meth = linsolqr_fclass(sys)                                  -->
 *  <!--  enum factor_class;                                           -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *
 *  Returns the most recently set factorization class of the system.
 *  The system should be previously prepped.
 */

extern enum factor_method linsolqr_fmethod(linsolqr_system_t sys);
/**< 
 *  <!--  meth = linsolqr_fmethod(sys)                                 -->
 *  <!--  enum factor_method;                                          -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *
 *  Returns the most recently used factorization method of the system.
 *  The system should be previously prepped.
 */

extern enum reorder_method linsolqr_rmethod(linsolqr_system_t sys);
/**< 
 *  <!--  meth = linsolqr_rmethod(sys)                                 -->
 *  <!--  enum reorder_method;                                         -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *
 *  Returns the most recently set reorder method of the system.
 *  The system should be previously prepped.
 */

extern int32 linsolqr_rank(linsolqr_system_t sys);
/**<
 *  <!--  rank = linsolqr_rank(sys)                                    -->
 *  <!--  int32 rank;                                                  -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *
 *  Returns the rank of the system.  The system must be previously
 *  factored.
 */

extern real64 linsolqr_smallest_pivot(linsolqr_system_t sys);
/**< 
 *  <!--  smallest_pivot = linsolqr_smallest_pivot(sys)                -->
 *  <!--  real64 smallest_pivot;                                       -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *
 *  Returns the smallest pivot accepted in solving the system.  The
 *  system must be previously factored.  If no pivoting was performed,
 *  MAXDOUBLE is returned.
 */

extern int linsolqr_prep(linsolqr_system_t sys, 
                         enum factor_class fclass);
/**<
 *  <!--  linsolqr_prep(sys,fclass)                                    -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *  <!--  enum factor_class;                                           -->
 *
 *  This function is analogous to slv_select_solver. It
 *  takes care of allocations. The prep call should be done (once) at
 *  the beginning of any series of linear solutions being performed on
 *  on the same linsolqr system with the same factorization class.<br><br>
 *
 *  You must prep before doing a reordering, factorization or solution
 *  as prep sets up the appropriate internal settings. You should set
 *  the matrix before you call prep.
 *  Prep (or subsequent preps) do not affect the right hand sides of
 *  the system.<br><br>
 *
 *  If you wish to change from 1 factoring method to another and they
 *  are not part of the class of compatible methods, you should call
 *  prep again with the new class.
 *  After prep is called, reorder should be called before factorization.
 *  As most of the sparse routines depend for performance on the
 *  prior reordering.<br><br>
 *
 *  Return 0 if ok, or 1 otherwise.
 */

extern int linsolqr_reorder(linsolqr_system_t sys,
                            mtx_region_t *region,
                            enum reorder_method rmeth);
/**<
 *  <!--  linsolqr_reorder(sys,region,rmeth)                           -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *  <!--  mtx_region_t *region;                                        -->
 *  <!--  enum reorder_method;                                         -->
 *
 *  The specified region of the coefficient matrix is reordered.  This
 *  must be called before factoring the matrix.  The specified region
 *  is assumed to contain only nonempty rows and columns and have a full
 *  diagonal.
 *  If the coefficient matrix in use is from a nonlinear system, the
 *  pattern in the coefficient matrix should be the structural one (as
 *  opposed to the numerically derived incidence which may be less.)<br><br>
 *
 *  If you use the numerically derived incidence, you will need to reorder
 *  before every factorization. This is generally not cost effective.
 *  If region given is mtx_ENTIRE_MATRIX, a search will be done to find
 *  an appropriate bounding region in the coefficient mtx. This is
 *  not a particularly cheap search.<br><br>
 *
 *  This function is analogous to slv_presolve. It
 *  takes care of any structural analysis necessary
 *  to the linear solution. The reorder call should be done (once) at
 *  the beginning of any series of linear solutions being performed on
 *  on structurally identical matrices.<br><br>
 *
 *  The reordering done must be appropriate to the factorization class.
 *  You must reorder before doing a factorization or solution as reorder
 *  sets up the appropriate internal settings. Even were the internals
 *  method independent, the reordering is critical to the performance of
 *  these methods.<br><br>
 *  Return 0 if ok, 1 if not.
 *  <pre>
 *  Brief notes on the reorderings available.
 *     SPK1:     The region you give becomes the region for the problem,
 *     TSPK1:    but for factorization purposes rows and columns that
 *               do not intersect the main diagonal within the region
 *               are considered structurally (therefore numerically) dependent.
 *     Natural:  Blesses the system and does nothing.
 *               Again, the rows/cols not in the diagonal are dependent.
 *  </pre>
 */

extern int linsolqr_factor(linsolqr_system_t sys,
                           enum factor_method fmethod);
/**<
 *  <!--  linsolqr_factor(sys,fmethod)                                 -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *  <!--  enum factor_method fmethod;                                  -->
 *
 *  Decompose the reordered region of a copy of the coefficient matrix
 *  into upper and lower triangular factors (if necessary) which can be
 *  inverted easily when applied to any rhs.  Matrix must be factored in
 *  order to perform any of the operations below.  The numerical rank and
 *  the smallest pivot encountered during pivoting are computed in the
 *  process. Factorization method used is that given if it is compatible
 *  with the class set when the prep was done, otherwise the call fails.<br><br>
 *
 *  If you are handed a linsolqr system and want to refactor it using the
 *  usual method, but don't know what that method is, call like:
 *  status = linsolqr_factor(sys,linsolqr_fmethod(sys));<br><br>
 *
 *  Return 0 if ok, 1 if not.
 */

extern int linsolqr_get_pivot_sets(linsolqr_system_t sys, 
                                   unsigned *org_rowpivots,
                                   unsigned *org_colpivots);
/**<
 *  <!--  status=linsolqr_get_pivot_sets(sys,org_rowpivots,org_colpivots) -->
 *  <!--  linsolqr_system_t sys;                                          -->
 *  <!--  unsigned *org_rowpivots,*org_colpivots;  (see the "set" module) -->
 *
 *  Returns the set of original row numbers / original column numbers which
 *  have been pivoted.  org_rowpivots and org_colpivots are assumed to be
 *  sets created by (or at least for) the set module with sufficient size
 *  before calling this function.  They must also be previously NULLed.
 *  The system must be previously factored.
 *  The sets input should be the result of set_create(neqn),set_create(nvar).
 *  There is no association of rows with columns here.<br><br>
 *
 *  Status is 0 if not factored, 1 if factored. If 0, sets will not be
 *  changed.
 *  This bizarre piece of functionality should be done away with as soon
 *  as the equivalents below have been implemented.
 */

extern mtx_sparse_t *linsolqr_unpivoted_rows(linsolqr_system_t sys);
/**< See discussion under linsolqr_unpivoted_cols(). */
extern mtx_sparse_t *linsolqr_unpivoted_cols(linsolqr_system_t sys);
/**< 
 *  <pre>
 *  singrows = linsolqr_unpivoted_rows(sys);
 *  singcols = linsolqr_unpivoted_cols(sys);
 *  linsolqr_system_t sys;
 *  mtx_sparse_t *singrows, *singcols;
 *
 *  Returns the set of original row numbers / original column numbers which
 *  have NOT been pivoted and the rejected pivots in an mtx_sparse_t.
 *  Return is NULL if sys not factored or if no unpivoted rows/cols exist.
 *  E.g. singrows->idata[i] are the original rows that were not pivoted,
 *       singrows->data[i] is the pivot that was rejected,
 *       for i = 0 to singrows->len-1.
 *  If len is 0, the data and idata pointers may be NULL.
 *
 *  The CALLER is responsible for deallocating the mtx_sparse_t returned;
 *  linsolqr wants nothing further to do with it.
 *  </pre>
 *  @bug Only deals with ranki based square systems at the moment.
 *       Then again, that's all we have at the moment (10/95).
 *       Returns NULL from non-ranki systems.
 *  @todo Separate documentation for these linsolqr functions?
 */

extern mtx_sparse_t *linsolqr_pivoted_rows(linsolqr_system_t sys);
/**< See discussion under linsolqr_pivoted_cols(). */
extern mtx_sparse_t *linsolqr_pivoted_cols(linsolqr_system_t sys);
/**<
 *  <pre>
 *  pivrows = linsolqr_pivoted_rows(sys);
 *  pivcols = linsolqr_pivoted_cols(sys);
 *  linsolqr_system_t sys;
 *  mtx_sparse_t *pivrows, *pivcols;
 *
 *  Returns the set of original row numbers / original column numbers which
 *  have been pivoted and the pivots in an mtx_sparse_t.
 *  Return is NULL if sys not factored or if no pivoted rows/cols exist.
 *  E.g. pivrows->idata[i] are the original rows that were pivoted,
 *       pivrows->data[i] is the pivot,
 *       for i = 0 to pivrows->len-1.
 *  If len is 0, the data and idata pointers may be NULL.
 *
 *  The CALLER is responsible for deallocating the mtx_sparse_t returned;
 *  linsolqr wants nothing further to do with it.
 *  </pre>
 *  @bug  Only deals with ranki based square systems at the moment.
 *        Then again, that's all we have at the moment (10/95).
 *        Returns NULL from non-ranki systems.
 *  @todo Separate documentation for these linsolqr functions?
 */

extern int32 linsolqr_org_row_to_org_col(linsolqr_system_t sys,
                                         int32 org_row);
/**< See discussion under linsolqr_org_col_to_org_row(). */
extern int32 linsolqr_org_col_to_org_row(linsolqr_system_t sys,
                                         int32 org_col);
/**<
 *  <pre>
 *  org_col = linsolqr_org_row_to_org_col(sys,org_row)
 *  org_row = linsolqr_org_col_to_org_row(sys,org_col)
 *  linsolqr_system_t sys;
 *  int32 org_col,org_row;
 *
 *  Pivoted original columns and pivoted original rows can be associated
 *  with one another via the pivot sequence.  These functions returned the
 *  org_col/org_row associated with the given org_row/org_col.  If the given
 *  org_row/org_col is not pivoted, a meaningless value is returned.  The
 *  system must be previously factored. If not factored, these functions
 *  will return a value, but linsolqr may reorder making the value wrong.
 *  </pre>
 *  @todo Separate documentation for these linsolqr functions?
 */

ASC_DLLSPEC(void) linsolqr_calc_row_dependencies(linsolqr_system_t sys);
/**< See discussion under linsolqr_calc_col_dependencies(). */

ASC_DLLSPEC(void) linsolqr_calc_col_dependencies(linsolqr_system_t sys);
/**<
 *  <pre>
 *  linsolqr_calc_row_dependencies(sys);
 *  linsolqr_calc_col_dependencies(sys);
 *  linsolqr_system_t sys;
 *
 *  Given a factored system for which dependencies are not yet
 *  calculated, calculates row/column dependencies. This must be
 *  called before either linsolqr_org_row/col_dependency
 *  or linsolqr_row/col_dependence_coefs can be called.
 *  All rows/columns in the region specified to reorder
 *  and not part of the final factorization have their dependencies
 *  calculated.
 *
 *  The calculation method is fairly standard.
 *  We will give the version for LU column dependency. Transpose it
 *  for row dependency. Similar things can be done for QR.
 *  Given a matrix region A, factor it and arrive at
 *      A11 | A12  where A11 = L U and columns of A12 are dependent,
 *      ---------  that is A12 = A11 . Y. (The row A2 need not exist,
 *      A21 | eps  but if it does, A22 must be small or we could have
 *  factored further.) Solve for Y. The Yi are the coefficients of the
 *  linear combination of columns in A11 which sum to A12.
 *  And, note that since the columns of A12 were treated during
 *  factoring as if they were ultimately going to be pivoted in,
 *  we only need to substitute on the other triangle factor, half the
 *  work of a regular solve.
 *  </pre>
 *  @todo Separate documentation for these linsolqr functions?
 */

extern mtx_sparse_t
*linsolqr_row_dependence_coefs(linsolqr_system_t sys, int32 orgrow);
/**< See discussion under linsolqr_col_dependence_coefs(). */
extern mtx_sparse_t
*linsolqr_col_dependence_coefs(linsolqr_system_t sys, int32 orgcol);
/**<
 *  <pre>
 *  rowcoefs = linsolqr_row_dependence_coefs(sys,orgrow);
 *  colcoefs = linsolqr_col_dependence_coefs(sys,orgcol);
 *
 *  linsolqr_system_t sys;
 *  int32 orgrow, orgcol;
 *  mtx_sparse_t *rowcoefs, *colcoefs;
 *
 *  Given an org row/col and a factored system, returns a mtx_sparse_t
 *  containing the org row/col indices and linear combination coefficients
 *  contributing to the given dependent row/col. If the orgrow/orgcol
 *  given is not dependent or dependencies have not been calculated,
 *  return is NULL.
 *    E.g. rowcoefs->idata[i] is the org row index of a contributing row,
 *         rowcoefs->data[i] is the dependency coefficient,
 *           for i = 0 to rowcoefs->len-1.
 *  Numeric dependency calculation is a numeric process with lots of room
 *  for interpretation of the results. Not everything that claims to
 *  contribute to a singularity really does so. We leave this interpre-
 *  tation to the the caller.
 *
 *  The CALLER is responsible for deallocating the mtx_sparse_t returned;
 *  linsol wants nothing further to do with it.
 *  </pre>
 *  @todo Separate documentation for these linsolqr functions?
 */

extern real64 linsolqr_org_row_dependency(linsolqr_system_t sys,
                                          int32 dep, int32 ind);
/**< See discussion under linsolqr_org_col_dependency(). */
extern real64 linsolqr_org_col_dependency(linsolqr_system_t sys,
                                          int32 dep, int32 ind);
/**<
 *  <pre>
 *  coef = linsolqr_org_row_dependency(sys,dep,ind)
 *  coef = linsolqr_org_col_dependency(sys,dep,ind)
 *  real64 coef;
 *  linsolqr_system_t sys;
 *  int32 dep,ind;
 *
 *  Any original row / column of the coefficient matrix which when submitted
 *  to the linear solver is not pivoted, is called dependent and can be
 *  written as a linear combination of the independent (pivoted) original
 *  rows / columns.  These functions return the previously computed
 *  coefficients of the linear combination.  The system must be previously
 *  factored and the independent row / column must be a member of the
 *  set of row / column pivots obtained by linsolqr_get_pivot_sets.
 *  This is a slow and clunky way to retrieve dependency info.
 *  This ought to be done away with when the above function is done.
 *  </pre>
 *  @todo Separate documentation for these linsolqr functions?
 */

extern int linsolqr_solve(linsolqr_system_t sys, real64 *rhs);
/**<
 *  <!--  linsolqr_solve(sys,rhs)                                      -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *  <!--  real64 *rhs;                                                 -->
 *
 *  Solves the system of linear equations (if necessary) utilizing the
 *  specified rhs along with the previously factored matrix.  The rhs
 *  is automatically checked if the matrix factors need to be transposed
 *  or not (see linsolqr_add_rhs.)
 *  Solution method will be appropriate to the factorization method used
 *  in linsolqr_factor.
 *  Return 0 if ok, 1 if not.
 */

extern real64 linsolqr_var_value(linsolqr_system_t sys,
                                 real64 *rhs, int32 var);
/**<
 *  <!--  value = linsolqr_var_value(sys,rhs,var)                      -->
 *  <!--  real64 value;                                                -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *  <!--  real64 *rhs;                                                 -->
 *  <!--  int32 var;                                                   -->
 *
 *  Returns the value of the variable in the solution vector associated
 *  with the given rhs and either the matrix or its transpose.  The rhs
 *  must be solved for first.  If rhs specifies transpose, then var is
 *  expected to be an original row number, otherwise it should be an
 *  original column number.
 *  If sys, rhs, and var are not proper, the value returned is 0.0
 *  and a warning is issued to stderr.
 */

extern boolean linsolqr_copy_solution(linsolqr_system_t sys, 
                                      real64 *rhs, real64 *vector);
/**<
 *  <!--  not_ok = linsolqr_copy_solution(sys,rhs,vector)              -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *  <!--  real64 *rhs, *vector;                                        -->
 *
 *  Once a sys has been factored and rhs solved, this
 *  fills in a copy of the solution vector associated with rhs. Caller
 *  must provide vector and vector must be of length at least as long
 *  as the order of the matrix that was solved. The vector entries
 *  will be in org_col order if the rhs is normal or in org_row order
 *  if the rhs is a transpose.
 *  
 *  The return value is TRUE if something is amiss, FALSE otherwise.
 *  
 */

extern real64 linsolqr_eqn_residual(linsolqr_system_t sys,
                                   real64 *rhs, int32 eqn);
/**<
 *  <!--  residual = linsolqr_eqn_residual(sys,rhs,eqn)                -->
 *  <!--  real64 residual;                                             -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *  <!--  real64 *rhs;                                                 -->
 *  <!--  int32 eqn;                                                   -->
 *
 *  Returns the equation residual using the solution vector associated
 *  with the given rhs and either the matrix or its transpose.
 *  <pre>
 *                                           T
 *     residual = A x - b   or   residual = A x - b
 *  </pre>
 *  The rhs must be solved for first.  If rhs specifies transpose, then
 *  eqn is expected to be an original column number, otherwise it should
 *  be an original row number.
 *  If the system and rhs and eqn are not properly solved, the return
 *  value is MAXDOUBLE.
 */

extern boolean linsolqr_calc_residual(linsolqr_system_t sys,
                                      real64 *rhs, real64 *vector);
/**<
 *  <!--  not_ok = linsolqr_calc_residual(sys,rhs,vector)              -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *  <!--  real64 *rhs, *vector;                                        -->
 *
 *  Returns the residuals using the solution associated
 *  with the given rhs and either the matrix or its transpose.
 *  <pre>
 *                                           T
 *     residual = A x - b   or   residual = A x - b
 *  </pre>
 *  The rhs must be solved for first. Caller
 *  must provide vector and vector must be of length at least as long
 *  as the order of the matrix that was solved. The vector entries
 *  will be in org_row order if the rhs is normal or in org_col order
 *  if the rhs is a transpose. Entries of the vector which do not
 *  correspond to rows/cols of factored system solved will not be modified.<br><br>
 *
 *  If the system and rhs are not properly solved, or other is amiss
 *  return value is TRUE, else FALSE. 
 */

/* miscellaneous functions for C programmers wanting to know things. */

extern size_t linsolqr_size(linsolqr_system_t sys);
/**< 
 * <!--  size = linsolqr_size(sys)                                     -->
 * <!--  size_t size;                                                  -->
 * <!--  linsolqr_system_t sys;                                        -->
 *
 *  Returns the amount of memory in use by a system and all its
 *  bits and pieces. User supplied RHS vectors and coefficient
 *  matrix are NOT included in the size calculation. The user
 *  must do accounting for those.
 */

extern void linsolqr_free_reused_mem(void);
/**< 
 *  <!--  linsolqr_free_reused_mem()                                   -->
 *
 *  Deallocates any memory that linsolqr may be squirrelling away for
 *  internal reuse. Calling this while any slv_system_t using linsolqr exists
 *  is likely to be fatal: handle with care.
 *  There isn't a way to query how many bytes this is.
 */

/*
 *  The following calls exist to facilitate debugging of the linear
 *  solver when it is being tested on large systems. Do not use them
 *  in routine coding. If you need access to the factor/inverse matrices
 *  for computational purpose, you should probably consider adding that
 *  functionality to linsol.
 */
 
extern mtx_matrix_t linsolqr_get_factors(linsolqr_system_t sys);
/**< See discussion under linsolqr_get_inverse(). */
extern mtx_matrix_t linsolqr_get_inverse(linsolqr_system_t sys);
/**<
 *  <pre>
 *  <!--  linsolqr_get_factors(sys)                                    -->
 *  <!--  linsolqr_get_inverse(sys)                                    -->
 *
 *  Returns the handle of the factor (L\U, Q\R) or inverse matrix.
 *  The handle may be NULL, and should be checked before use.
 *  The matrix should not be tampered (other than to look at it)
 *  with if any other mathematical operations will take place with
 *  sys. Linsol expects to deallocate both factors and inverse; do
 *  not do so yourself.
 *  Note: All the factorization methods use some sort of dense vectors
 *  for storing pivots and whatnot. As with a FORTRAN factorization,
 *  there's not much you can do with a factorization without
 *  also understanding in detail the factorization routine and getting
 *  your hands on the dense vectors.
 *  </pre>
 *  @todo Separate documentation of linsolqr_get_factors() 
 *        and linsolqr_get_inverse()?
 */

extern mtx_region_t *linsolqr_get_region(linsolqr_system_t sys);
/**< 
 *  <!--  reg = linsolqr_get_region(sys)                               -->
 *  <!--  mtx_region_t *reg;                                           -->
 *  <!--  linsolqr_system_t sys;                                       -->
 *
 *  Returns a pointer to the current linsolqr region.
 *  This is being created for use in the check numeric dependency
 *  routine and may be removed once the new "problem manager" window
 *  is created to take over this functionality (and others).
 */

extern int linsolqr_setup_ngslv(linsolqr_system_t sys, 
                                real64 *rhs,
                                mtx_range_t *un_p_rng,
                                real64 *tmpvec);
/**< Sets up the NGSlv solver. */
extern real64 *linsolqr_get_varvalue(linsolqr_system_t sys, int n);
/**<  Returns the value of the nth variable in sys. */

#endif  /* ASC_LINSOLQR_H */

