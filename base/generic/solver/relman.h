/**< 
 *  SLV: Ascend Nonlinear Solver
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.29 $
 *  Version control file: $RCSfile: relman.h,v $
 *  Date last modified: $Date: 1998/04/23 23:56:24 $
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

/*************************
 *  Contents:     Relation manipulator module
 *
 *  Authors:      Karl Westerberg
 *                Joseph Zaher
 *
 *  Dates:        06/90 - original version
 *                06/93 - separated out exprman
 *                11/93 - added relman_calc_satisfied
 * 
 *  Description:  This module will provide supplemental operations for
 *                relations such as simplification, evaluation, and
 *                differentiation.
 *************************/
#ifndef relman__already_included
#define relman__already_included

/**< requires #include "mtx.h" */
/**< requires #include "var.h" */
/**< requires #include "rel.h" */


#define relman_is_linear(a,b) (FALSE)
/**< 
extern boolean relman_is_linear(struct rel_relation *,var_filter_t *);
 *  islinear = relman_is_linear(rel,filter)
 *  boolean islinear;
 *  struct rel_relation *rel;
 *  var_filter_t *filter;
 *  
 *  Determines whether or not the given relation is linear in
 *  all of the variables which pass through the variable filter, treating
 *  those variables which fail to pass as constants.
 *  
 *  Example:
 *  x1 + x2 >= 3 is a linear relation.
 **/

extern real64 relman_linear_coef(struct rel_relation *,
  struct var_variable *,var_filter_t *);
/**< 
 *  coef = relman_linear_coef(rel,var,filter)
 *  real64 coef;
 *  struct rel_relation *rel;
 *  struct var_variable *var;
 *  var_filter_t *filter;
 *  
 *  Computes the coefficient of the given variable in a linear
 *  relation.  If var=NULL, then the "RHS" is returned instead.  More
 *  nprecisely, "a[var]" is returned where:
 *
 *  rel : sum[variables v](a[v] * v) COMPARISON a[NULL]
 *
 *  It is assumed that the relation is linear, otherwise,
 *  something will be returned, but it won't be very meaningful.
 **/

extern relman_simplify(struct rel_relation *,int);
/**<  NOT IMPLEMENTED
 *  relman_simplify(rel,opt_level)
 *  struct rel_relation *rel;
 *  int opt_level;
 *
 *  The left and right hand sides of the given relation are simplified
 *  to the extent given by opt_level.  The effect of varying values of
 *  opt_level is given in the description for exprman_simplify().
 **/

extern relman_dissolve_vars(struct rel_relation *,var_filter_t *);
/**<   NOT IMPLEMENTED
 *  relman_dissolve_vars(rel,filter)
 *  struct rel_relation *rel;
 *  var_filter_t *filter;
 *
 *  Variables which pass through the filter are replaced in the
 *  relation by their current values.
 **/

extern void relman_decide_incidence(struct rel_relation *);
/**< 
 *  relman_decide_incidence(rel)
 *  struct rel_relation *rel;
 *
 *  Sets the incidence field of each variable which is found to be
 *  incident in the relation rel to TRUE.  If these variables make
 *  up a subset of some larger variable list, it is important to first
 *  set the incidence field of all of the variables to FALSE before
 *  using this function in order to determine the unattached variables.
 **/

extern void relman_get_incidence(struct rel_relation *,
                                 var_filter_t *,mtx_matrix_t);
/**< 
 *  relman_get_incidence(rel,filter,matrix)
 *  struct rel_relation *rel;
 *  var_filter_t *filter;
 *  mtx_matrix_t matrix;
 *
 *  Upon return, coefficient (rel_n,var_n) (using original row and column
 *  numbers) is non-zero if and only if the relation rel with index rel_n
 *  depends on a variable with index var_n.
 **/

extern real64 relman_eval(struct rel_relation *, int32 *, int);
/**< 
 *  residual = relman_eval(rel,status,safe)
 *  struct rel_relation *rel;
 *  int32 *status;
 *  int safe;
 *  real64 residual;
 *
 *  The residual of the relation is calculated and returned.  In addition
 *  to returning the residual, the residual field of the relation is
 *  updated.  Residual := LHS - RHS regardless of comparison.  The status
 *  value can be monitored to
 *  determine if any calculation errors were encountered.  It will be set
 *  0 if a calculation results in an error.
 *  If the value of safe is nonzero, "safe" functions will be used to
 *  calculate the residual.
 *  The residual field of the relation is not updated when an error occurs.
 *
 * This function should be surrounded by Asc_SignalHandlerPush/Pop both
 * with arguments (SIGFPE,SIG_IGN). If it is being called in a loop,
 * the push/pop should be _outside_ the loop.
 **/

extern int32 relman_obj_direction(struct rel_relation *);
/**< 
 *  direction = relman_eval_obj(rel,status,safe)
 *  struct rel_relation *rel;
 *
 *  Returns:
 *  direction = -1 if objective is minimization
 *  direction =  1 if objective is maximization
 *  direction =  0 otherwise. (ie. if not an objective)
 **/

extern real64 relman_scale(struct rel_relation *);
/**< 
 *  residual = relman_scale(rel)
 *  struct rel_relation *rel;
 *  real64 residual;
 *
 *  Calculates relation nominal scaling factor for
 *  current values stored in the relations variables.
 *  Fills the relations nominal field and also returns
 *  the relations nominal.
 **/

#define relman_diff(a,b,c,d) (abort(),1)
/**< needs to be reimplemented.
ExTERn int relman_diff(struct rel_relation *,struct var_variable*,real64*,int);
 *!  status = relman_diff(rel,var,pd,safe);
 *!  struct rel_relation *rel;
 *!  struct var_variable *var;
 *!  real64 *pd;
 *!  int safe;
 *!  int status;
 *!
 *!  Calculates the derivative of the relation residual with respect to
 *!  the specified variable and stuffs it in pd. if problem with
 *!  calculation, returns 1, else 0.
 *!  If the value of safe is nonzero, "safe" functions will be used to
 *!  calculate the residual.
 *!  Needs compiler side work.
 **/

extern int relman_diff2(struct rel_relation *, var_filter_t *,
			real64 *, int32 *, int32 *, int32);
/**< 
 *  status = relman_diff2(rel,filter,derivatives,variables,count,safe)
 *  struct rel_relation *rel;
 *  var_filter_t *filter;
 *  real64 *derivatives;
 *  int32 *variables;
 *  int32 *count;
 *  int32 status;
 *
 *  Calculates the row of the jacobian matrix (the transpose gradient of
 *  the relation residual grad^T(f) ) corresponding to the relation
 *  rel.  The filter determines which variables actually contribute to the
 *  jacobian.
 *  If an error is encountered in the calculation, the status returned is
 *  1. Status = 0 is OK.
 *  If the value of safe is nonzero, "safe" functions are used to for
 *  the calucaltions.
 *  The calling function should allocate the output vectors 'derivatives'
 *  and 'variables'.  'count' will be set to the number of elements
 *  assigned upon exit.
 *  derivative(I) will contain the derivative of the relation with
 *  respect to the variable whose solver index is stored in
 *  variables(I).
 **/

extern int relman_diff_grad(struct rel_relation *, var_filter_t *,
			    real64 *, int32 *, int32 *, int32 *, real64 *,
			    int32);
/**< 
 ***  status = relman_diff_grad(rel,filter,derivatives,variables_master,
 ***                            variables_solver,count,resid,safe)
 ***  struct rel_relation *rel;
 ***  var_filter_t *filter;
 ***  real64 *derivatives;
 ***  real64 *resid;
 ***  int32 *variables_master;
 ***  int32 *variables_solver;
 ***  int32 *count;
 ***  int32 status;
 ***
 ***  Calculates the row of the jacobian matrix (the transpose gradient of
 ***  the relation residual grad^T(f) ) corresponding to the relation
 ***  rel.  The filter determines which variables actually contribute to the
 ***  jacobian. The residual of the relation is also computed and returned.
 ***  If an error is encountered in the calculation, the status returned is
 ***  1. Status = 0 is OK.
 ***  If the value of safe is nonzero, "safe" functions are used to for
 ***  the calculations.
 ***  The calling function should allocate the output vectors 'derivatives',
 ***  'variables_master' and 'variables_solver'.  'count' will be set to 
 ***  the number of elements assigned upon exit.
 ***  derivative(i) will contain the derivative of the relation with
 ***  respect to the variable whose master index is stored in
 ***  variables_master(i). The solver index of each variable is stored in
 ***  variables_solver(i).
 ***  
 ***  Ther are two differences wrt to relman_diff2:
 ***  1) the master index (solver independent) is obtained
 ***  2) the residual is evaluated
 **/


extern int relman_diffs(struct rel_relation *, var_filter_t *,
                        mtx_matrix_t, real64 *, int);
/**< 
 *  status = relman_diffs(rel,filter,mtx,resid,safe)
 *  struct rel_relation *rel;
 *  var_filter_t *filter;
 *  real64 *resid;
 *  mtx_matrix_t mtx;
 *  int safe;
 *  int status;
 *
 *  Calculates the row of the jacobian matrix (the transpose gradient of
 *  the relation residual grad^T(f) ) corresponding to the relation
 *  rel.  The filter determines which variables actually contribute to the
 *  jacobian.  The residual of the relation is also computed and returned.
 *  If an error is encountered in the calculation, the status returned is
 *  1 and the residual is set to some number we managed to calculate,
 *  while the gradient is discarded. status = 0 is OK.
 *  If the value of safe is nonzero, "safe" functions are used to for
 *  the calucaltions.
 *  It doesn't matter how you have permuted the columns and rows:
 *  for the vars which pass the filter you send we
 *  fill the org row determined by rel_sindex and the org cols
 *  determined by var_sindex.
 *
 *  NOTE: The row of the mtx corresponding to rel should be cleared
 *  before calling this function, since this FILLS with the gradient.
 *
 *  CHANGE: This operator used to just ADD on top of any incidence already
 *          in the row. This is not TRUE now.
 *
 *  Bugs: none known except this operator really needs to be redesigned
 *  so it can deal with harwellian matrices, glassbox rels and blackbox.
 **/

extern int32 relman_diff_harwell(struct rel_relation **,
                                 var_filter_t *, rel_filter_t *,
                                 int32, int32, int32,
                                 real64 *, int32 *, int32 *);
/**< 
 * err = relman_diff_harwell(rlist,vfilter,rfilter,rlen,bias,mors,
 *                           avec,ivec,jvec);
 * This fills an "a-i-j" sparse matrix in the avec/ivec/jvec given.
 * struct rel_relation **rlist; list of relations rlen long.
 * var_filter_t *vfilter; stuffs gradient for matching variables only.
 * int32 rlen; length of list of relations.
 * int32 bias; 0 = row grouped together, 1 = column grouped together.
 *             There is a substantial penalty for bias = 1. we MODEL by row.
 * int32 mors; 0 = master var index of columns, master rel index of rows
 *             1 = solver var index of columns, master rel index of rows
 *             2 = master var index of columns, solver rel index of rows
 *             3 = solver var index of columns, solver rel index of rows
 * Size of avec,ivec,jvec given is assumed big enough.
 * big_enough = relman_jacobian_count(rlist,rlen,vfilter,rfilter,&dummy);
 * If ivec or jvec given is NULL, then neither is stuffed, though avec is.
 * int32 err;
 * err = 1 --> unrecoverable error/bad input. caller should probably punt.
 * err = 0 --> ok;
 * err < 0 --> -(number of floating point errors in evaluation).
 *             The matrix will contain an approximation only.
 *
 * Bugs:
 * bias == 1 is not yet implemented.
 */

extern int32 relman_jacobian_count(struct rel_relation **, int32,
                                   var_filter_t *, rel_filter_t *, int32 *);
/**< 
 * nnz = relman_jacobian_count(rlist, rlen, vfilter, rfilter, rhomax);
 * Return the number of nonzero gradient entries in the equations
 * given. Only equations passing rfilter and entries passing vfilter
 * are counted. rlen is the length of the relation list.
 * *rhomax is the largest row count on return.
 */

extern boolean relman_calc_satisfied(struct rel_relation *,real64);
extern boolean relman_calc_satisfied_scaled(struct rel_relation *,real64);
/**< 
 *  satisfied = relman_calc_satisfied(rel,tolerance)
 *  satisfied = relman_calc_satisfied_scaled(rel,tolerance)
 *  boolean satisfied;
 *  real64 tolerance;
 *
 *  relman_calc_satisfied:
 *  Returns TRUE or FALSE depending on whether the relation whose residual
 *  has been previously calculated is satisfied based on the value stored
 *  in the residual field.  The satisfied field of the relation is also
 *  updated.  A tolerance specification allows equalities to be declared
 *  satisfied as long as their residuals are close to zero.
 *
 *  relman_calc_satisfied_scaled:
 *  This definition of satisfaction includes the notion
 *  of scaling by the relation nominal before comparison.
 **/

#define relman_directly_solve(r,s,a,n) \
  relman_directly_solve_new(r,s,a,n,1.0e-8)
extern real64 *relman_directly_solve_new(struct rel_relation *,struct var_variable *,
  int *,int *,real64);
/**< 
 *  solution_list = relman_directly_solve(rel,solvefor,able,nsolns)
 *  solution_list = relman_directly_solve_new(rel,solvefor,able,nsolns,tol)
 *  real64 *solution_list;
 *  struct rel_relation *rel;
 *  struct var_variable *solvefor;
 *  int *able;
 *  int *nsolns;
 *  real64 tol;
 *  
 *  Attempts to solve the given equation for the given variable.  If this
 *  function is able to determine the solution set, then *able is set to
 *  1 and a newly allocated solution list is returned: *nsolns will be
 *  set to the length of this array.  Otherwise *able is 0 and NULL
 *  is returned.  NULL *may* also be returned if the solution set is empty.
 *  A return of able == 1, solution_list != NULL, and nsolns == 0 is
 *  possible for certain classes of floating point exceptions.
 *  It is assumed that the relation is a condition of equality.
 *
 *  relman_directly_solve_new handles passing in a tolerance for glassbox
 *  relations so a rootfinder can do the work rather than leaving it to
 *  someone else. The rootfinder is based on Brent's algorithm. Old clients of
 *  relman_directly solve are grandfathered at a default tolerance of 1e-8.
 *  Once all these clients are gone, go back to the old name space for
 *  the new function.
 *
 *  Do NOT free or keep a persistent pointer to the solution_list this
 *  function returns.
 **/

#define relman_make_string_infix(s,r)    \
  relman_make_vstring_infix((s),(r),TRUE)
#define relman_make_string_postfix(s,r)  \
  relman_make_vstring_postfix((s),(r),TRUE)
#define relman_make_xstring_infix(s,r)   \
  relman_make_vstring_infix((s),(r),FALSE)
#if 0 /**< needs compiler side work */
#define relman_make_xstring_postfix(s,r) \
  relman_make_vstring_postfix((s),(r),FALSE)
#else
#define relman_make_xstring_postfix(s,r) \
  dummy_rel_string(s,r,0)
#endif
/**< 
 *  string = relman_make_string_infix(sys,rel)
 *  string = relman_make_string_postfix(sys,rel)
 *  string = relman_make_xstring_infix(sys,rel)
 *  string = relman_make_xstring_postfix(sys,rel) // not working
 *  char *string;
 *  struct rel_relation *rel;
 *
 *  Creates a sufficiently large string (you must free it when you're
 *  done with it) into which it converts a relation.  The string will be
 *  terminated with a '\0' character. 
 *  
 *  The xstring versions of this call make strings where all the variables
 *  are written as x<varindex> (e.g. x23) rather than as object names.
 *  The MASTER index (var_mindex) is used, not the solver's sindex.
 *  Currently the compiler does not support xstring postfix format,
 *  but could easily do so if needed.
 *  
 *  The name of a var is context dependent, so you have to provide the
 *  slv_system_t from which you got the relation.
 **/
extern char *relman_make_vstring_infix(slv_system_t,
                                       struct rel_relation *,int);
extern char *relman_make_vstring_postfix(slv_system_t,
                                         struct rel_relation *,int);
/**< 
 * tmp function to placehold unimplemented io functions.
 */
extern char *dummyrelstring(slv_system_t, struct rel_relation *,int);

/**< 
 * relman_free_reused_mem(void); 
 * Call when desired to free memory cached internally.
 */
extern void relman_free_reused_mem(void); 

#endif
