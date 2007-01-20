/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1990 Karl Michael Westerberg

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
	Relation manipulator module for the SLV solver.

	This module will provide supplemental operations for
	relations such as simplification, evaluation, and
	differentiation.

	Dates: 06/90 - original version
	       06/93 - separated out exprman
	       11/93 - added relman_calc_satisfied

*//*
	by Karl Michael Westerberg and Joseph Zaher, 2/6/90
	Last in CVS: $Revision: 1.29 $ $Date: 1998/04/23 23:56:24 $ $Author: ballan $
*/

#ifndef ASC_RELMAN_H
#define ASC_RELMAN_H

#include <utilities/ascConfig.h>

#include <linear/mtx.h>

#include "var.h"
#include "rel.h"

/**	@addtogroup solver Solver
	@{
*/

#define relman_is_linear(a,b) (FALSE)
/**<
 *  Determines whether or not the given relation is linear in
 *  all of the variables which pass through the variable filter, treating
 *  those variables which fail to pass as constants.<br><br>
 *
 *  Example:
 *  x1 + x2 >= 3 is a linear relation.
 */

extern real64 relman_linear_coef(struct rel_relation *rel,
                                 struct var_variable *var,
                                 var_filter_t *filter);
/**<
 *  Computes the coefficient of the given variable in a linear
 *  relation.  If var=NULL, then the "RHS" is returned instead.  More
 *  nprecisely, "a[var]" is returned where:<br><br>
 *
 *  rel : sum[variables v](a[v] * v) COMPARISON a[NULL]<br><br>
 *
 *  It is assumed that the relation is linear, otherwise,
 *  something will be returned, but it won't be very meaningful.
 */

extern void relman_simplify(struct rel_relation *rel, int opt_level);
/**<  NOT IMPLEMENTED
 *  The left and right hand sides of the given relation are simplified
 *  to the extent given by opt_level.  The effect of varying values of
 *  opt_level is given in the description for exprman_simplify().
 */

extern void relman_dissolve_vars(struct rel_relation *rel, var_filter_t *filter);
/**<   NOT IMPLEMENTED
 *  Variables which pass through the filter are replaced in the
 *  relation by their current values.
 */

extern void relman_decide_incidence(struct rel_relation *rel);
/**<
 *  Sets the incidence field of each variable which is found to be
 *  incident in the relation rel to TRUE.  If these variables make
 *  up a subset of some larger variable list, it is important to first
 *  set the incidence field of all of the variables to FALSE before
 *  using this function in order to determine the unattached variables.
 */

extern void relman_get_incidence(struct rel_relation *rel,
                                 var_filter_t *filter,
                                 mtx_matrix_t matrix);
/**<
 *  Upon return, coefficient (rel_n,var_n) (using original row and column
 *  numbers) is non-zero if and only if the relation rel with index rel_n
 *  depends on a variable with index var_n.
 */

ASC_DLLSPEC real64 relman_eval(struct rel_relation *rel, int32 *calc_okp, int safe);
/**<
	The residual of the relation is calculated and returned.  In addition
	to returning the residual, the residual field of the relation is
	updated. The residual field of the relation is not updated when an error
	occurs.

	@param safe If set nonzero, "safe" functions are used for evaluation (means
		that overflows, divide by zero, etc are avoided)
	@param calc_ok (returned) status of the calculation. 0=error, else ok.
	@return residual (= LHS - RHS, regardless of comparison)

	@NOTE
	This function should be surrounded by Asc_SignalHandlerPush/Pop both
	with arguments (SIGFPE,SIG_IGN). If it is being called in a loop,
	the push/pop should be _outside_ the loop.
*/

extern int32 relman_obj_direction(struct rel_relation *rel);
/**<
 *  Returns:
 *    - direction = -1 if objective is minimization
 *    - direction =  1 if objective is maximization
 *    - direction =  0 otherwise. (ie. if not an objective)
 */

extern real64 relman_scale(struct rel_relation *rel);
/**<
 *  Calculates relation nominal scaling factor for
 *  current values stored in the relations variables.
 *  Fills the relations nominal field and also returns
 *  the relations nominal.
 */

#define relman_diff(a,b,c,d) (abort(),1)
/**<
	Calculates the derivative of the relation residual with respect to
	the specified variable and stuffs it in pd. if problem with
	calculation, returns 1, else 0.
	If the value of safe is nonzero, "safe" functions will be used to
	calculate the residual.

	@TODO relman_diff() needs to be reimplemented - needs compiler-side work.
*/

extern int relman_diff2(struct rel_relation *rel,
                        var_filter_t *filter,
                        real64 *derivatives,
                        int32 *variables,
                        int32 *count,
                        int32 safe);
/**<
	Calculates the row of the jacobian matrix (the transpose gradient of
	the relation residual, $ \grad^{T}(f) $) corresponding to the relation
	rel. The filter determines which variables actually contribute to the
	jacobian.

	derivatives[i] will contain the derivative of the relation with
	respect to the variable whose solver index is stored in
	variables[i].

	@param rel         Relation being differentiated
	@param filter	   Filter for variables for which derivs are desired
	@param safe        If nonzero, "safe" functions are used to for the calculations
	@param derivatives output vector (allocated by the calling function)
	@param variables   output vector (allocated by the calling function)
	@param count       output value, will be set to the number of elements assigned upon exit.

	@return 0 on success, non-zero if an error is encountered in the calculation
*/

extern int relman_diff_grad(struct rel_relation *rel,
                            var_filter_t *filter,
                            real64 *derivatives,
                            int32 *variables_master,
                            int32 *variables_solver,
                            int32 *count,
                            real64 *resid,
                            int32 safe);
/**<
 *  Calculates the row of the jacobian matrix (the transpose gradient of
 *  the relation residual grad^T(f) ) corresponding to the relation
 *  rel.  The filter determines which variables actually contribute to the
 *  jacobian. The residual of the relation is also computed and returned.
 *  If an error is encountered in the calculation, the status returned is
 *  1. Status = 0 is OK.
 *  If the value of safe is nonzero, "safe" functions are used to for
 *  the calculations.<br><br>
 *  The calling function should allocate the output vectors 'derivatives',
 *  'variables_master' and 'variables_solver'.  'count' will be set to
 *  the number of elements assigned upon exit.
 *  derivative(i) will contain the derivative of the relation with
 *  respect to the variable whose master index is stored in
 *  variables_master(i). The solver index of each variable is stored in
 *  variables_solver(i).
 *
 *  There are two differences wrt to relman_diff2:
 *    - the master index (solver independent) is obtained
 *    - the residual is evaluated
 */

ASC_DLLSPEC int relman_diffs(struct rel_relation *rel,
		var_filter_t *filter, mtx_matrix_t mtx,
		real64 *resid, int safe);
/**<
	Calculates the row of the jacobian matrix (the transpose gradient of
	the relation residual grad^T(f) ) corresponding to the relation
	rel.  The filter determines which variables actually contribute to the
	jacobian.  The residual of the relation is also computed and returned.
	If an error is encountered in the calculation, the status returned is
	1 and the residual is set to some number we managed to calculate,
	while the gradient is discarded. status = 0 is OK.

	@param rel  relation for which jacobian entries are required
	@param filter  filter for which variables should actually contribute to the jacobian
	@param mtx  matrix into which the row (corresponding to rel) is written
	@param safe  if non-zero, "safe" functions are used to for the calucaltions.

	It doesn't matter how you have permuted the columns and rows:
	for the vars which pass the filter you send we
	fill the org row determined by rel_sindex and the org cols
	determined by var_sindex.

	@return 0 on success, 1 on calculation error (residual will be returned, grad discarded)

	@NOTE The row of the mtx corresponding to rel should be cleared
	before calling this function, since this FILLS with the gradient.<br><br>

	@NOTE *changed* -- This operator used to just ADD on top of any incidence
	already in the row. This is not TRUE now.

	@TODO This operator really needs to be redesigned so it can deal with
	harwellian matrices, glassbox rels and blackbox.
*/

extern int32 relman_diff_harwell(struct rel_relation **rlist,
		var_filter_t *vfilter, rel_filter_t *rfilter,
		int32 rlen, int32 bias, int32 mors,
		real64 *avec, int32 *ivec, int32 *jvec);
/**<
 *  This fills an "a-i-j" sparse matrix in the avec/ivec/jvec given.
 *  @param rlist   struct rel_relation **, list of relations rlen long.
 *  @param vfilter var_filter_t *, stuffs gradient for matching variables only.
 *  @param rlen    int32, length of list of relations.
 *  @param bias    int32, 0 = row grouped together, 1 = column grouped together.
 *                 There is a substantial penalty for bias = 1. we MODEL by row.
 *  @param mors    int32, 0 = master var index of columns, master rel index of rows
 *                 1 = solver var index of columns, master rel index of rows
 *                 2 = master var index of columns, solver rel index of rows
 *                 3 = solver var index of columns, solver rel index of rows
 *  Size of avec,ivec,jvec given is assumed big enough.
 *  big_enough = relman_jacobian_count(rlist,rlen,vfilter,rfilter,&dummy);
 *  If ivec or jvec given is NULL, then neither is stuffed, though avec is.
	@return 0 on success, <0 on floating point errors, 1 on unrecoverable error.

	err = 1 --> unrecoverable error/bad input. caller should probably punt.
	err < 0 --> -(number of floating point errors in evaluation).
	            The matrix will contain an approximation only.

	@todo relman_diff_harwell() bias == 1 is not yet implemented.
*/

extern int32 relman_jacobian_count(struct rel_relation **rlist,
                                   int32 rlen,
                                   var_filter_t *vfilter,
                                   rel_filter_t *rfilter,
                                   int32 *rhomax);
/**<
 *  Return the number of nonzero gradient entries in the equations
 *  given. Only equations passing rfilter and entries passing vfilter
 *  are counted. rlen is the length of the relation list.
 *  *rhomax is the largest row count on return.
 */

extern boolean relman_calc_satisfied_scaled(struct rel_relation *rel,
                                            real64 tolerance);
/**<
 *  This definition of satisfaction includes the notion
 *  of scaling by the relation nominal before comparison.
 *  @see relman_calc_satisfied.
 */
extern boolean relman_calc_satisfied(struct rel_relation *rel,
                                     real64 tolerance);
/**<
 *  Returns TRUE or FALSE depending on whether the relation whose residual
 *  has been previously calculated is satisfied based on the value stored
 *  in the residual field.  The satisfied field of the relation is also
 *  updated.  A tolerance specification allows equalities to be declared
 *  satisfied as long as their residuals are close to zero.
 *	@see relman_calc_satisfied_scaled.
 */

#define relman_directly_solve(r,s,a,n) \
  relman_directly_solve_new(r,s,a,n,1.0e-8)
  /**< @see relman_directly_solve_new(). */
extern real64 *relman_directly_solve_new(struct rel_relation *rel,
                                         struct var_variable *solvefor,
                                         int *able,
                                         int *nsolns,
                                         real64 tol);
/**<
 *  Attempts to solve the given equation for the given variable.  If this
 *  function is able to determine the solution set, then *able is set to
 *  1 and a newly allocated solution list is returned: *nsolns will be
 *  set to the length of this array.  Otherwise *able is 0 and NULL
 *  is returned.  NULL *may* also be returned if the solution set is empty.
 *  A return of able == 1, solution_list != NULL, and nsolns == 0 is
 *  possible for certain classes of floating point exceptions.
 *  It is assumed that the relation is a condition of equality.<br><br>
 *
 *  relman_directly_solve_new() handles passing in a tolerance for glassbox
 *  relations so a rootfinder can do the work rather than leaving it to
 *  someone else. The rootfinder is based on Brent's algorithm. Old clients of
 *  relman_directly solve are grandfathered at a default tolerance of 1e-8.
 *  Once all these clients are gone, go back to the old name space for
 *  the new function.<br><br>
 *
 *  Do NOT free or keep a persistent pointer to the solution_list this
 *  function returns.
 */

#define relman_make_string_infix(sys,rel)    \
      relman_make_vstring_infix((sys),(rel),TRUE)
/**< @see relman_make_xstring_infix() */
#define relman_make_string_postfix(sys,rel)  \
      relman_make_vstring_postfix((sys),(rel),TRUE)
/**< @see relman_make_xstring_infix() */
#define relman_make_xstring_infix(sys,rel)   \
      relman_make_vstring_infix((sys),(rel),FALSE)
/**<
	@return string
	@param sys
	@param rel struct rel_relation *rel

	Creates a sufficiently large string (you must free it when you're
	done with it) into which it converts a relation.  The string will be
	terminated with a '\0' character.

	The xstring versions of this call make strings where all the variables
	are written as x<varindex> (e.g. x23) rather than as object names.
	The MASTER index (var_mindex) is used, not the solver's sindex.
	Currently the compiler does not support xstring postfix format,
	but could easily do so if needed.

	The name of a var is context dependent, so you have to provide the
	slv_system_t from which you got the relation.
	
	@see relman_make_string_postfix
	@see relman_make_string_infix
*/
#if 0 /* needs compiler-side work */
#define relman_make_xstring_postfix(sys,rel) \
      relman_make_vstring_postfix((sys),(rel),FALSE)
#else
#define relman_make_xstring_postfix(sys,rel) \
      dummy_rel_string((sys),(rel),0)
#endif
/**<
 *  Not suppported.
 *  @TODO Consider adding support for xstring postfix format.
 */

ASC_DLLSPEC char *relman_make_vstring_infix(slv_system_t sys,
                                       struct rel_relation *rel,
                                       int style);
/**<
 *  Implementation function for relman_make_string_infix() and
 *  relman_make_xstring_infix().  Do not call this function
 *  directly - use one of the macros instead.
 */
extern char *relman_make_vstring_postfix(slv_system_t sys,
                                         struct rel_relation *rel,
                                         int style);
/**<
 *  Implementation function for relman_make_string_postfix(). and
 *  Do not call this function directly - use the macro instead.
 */

extern char *dummyrelstring(slv_system_t sys,
                            struct rel_relation *rel,
                            int style);
/**<  Temporary no-op function to placehold unimplemented io functions. */

extern void relman_free_reused_mem(void);
/**<
 * Call when desired to free memory cached internally.
 */

/* @} */

#endif  /* ASC_RELMAN_H */

