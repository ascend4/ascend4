/*
 *  External Relations Cache for solvers.
 *  by Kirk Andre Abbott
 *  Created: 08/10/94
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: extrel.c,v $
 *  Date last modified: $Date: 1997/07/18 12:14:14 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1994 Kirk Abbott
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
#include "utilities/ascConfig.h"
#include "compiler/packages.h"
/* #include "solver/exprman.h" */
#include "solver/rel.h"
#include "solver/extrel.h"
#include "compiler/extcall.h"

double g_external_tolerance = 1.0e-12;

struct ExtRelCache *CreateExtRelCache(struct ExtCallNode *ext)
{
  struct ExtRelCache *result;
  unsigned long n_input_args, n_output_args;
  int ninputs, noutputs;

  assert(ext!=NULL);
  result = (struct ExtRelCache *)ascmalloc(sizeof(struct ExtRelCache));
  result->nodestamp = ExternalCallNodeStamp(ext);
  result->efunc = ExternalCallExtFunc(ext);
  result->data = ExternalCallDataInstance(ext);
  result->arglist = ExternalCallArgList(ext);
  result->user_data = NULL;		/* absolutely important to be
					 * initialized to NULL ! */

  n_input_args = NumberInputArgs(result->efunc);
  n_output_args = NumberOutputArgs(result->efunc);

  /*
   * We own the result of the LinearizeArgList call.
   */
  result->inputlist = LinearizeArgList(result->arglist,1,n_input_args);
  ninputs = (int)gl_length(result->inputlist);
  noutputs = (int)CountNumberOfArgs(result->arglist,n_input_args+1,
				    n_input_args+n_output_args);
  result->ninputs = ninputs;
  result->noutputs = noutputs;

  result->inputs = (double *)asccalloc(ninputs,sizeof(double));
  result->outputs = (double *)asccalloc(noutputs,sizeof(double));
  result->jacobian = (double *)asccalloc(ninputs*noutputs,sizeof(double));
  /*
   * Setup default flags for controlling calculations.
   */
  result->newcalc_done = (unsigned)1;
  return result;
}


struct ExtRelCache *CreateCacheFromInstance(struct Instance *relinst)
{
  struct ExtCallNode *ext;
  struct ExtRelCache *cache;
  CONST struct relation *reln;
  enum Expr_enum type;

  reln = GetInstanceRelation(relinst,&type);
  if (type!=e_blackbox) {
    FPRINTF(stderr,"Invalid relation type in (CreateCacheFromInstance)\n");
    return NULL;
  }
  ext = BlackBoxExtCall(reln);
  cache = CreateExtRelCache(ext);
  return cache;
}

void ExtRel_DestroyCache(struct ExtRelCache *cache)
{
  if (cache) {
    cache->nodestamp=-1;
    cache->efunc = NULL;
    cache->data = NULL;
    gl_destroy(cache->inputlist);	/* we own the list */
    ascfree(cache->inputs);
    ascfree(cache->outputs);
    ascfree(cache->jacobian);
    cache->inputlist = NULL;
    cache->inputs = NULL;
    cache->outputs = NULL;
    cache->jacobian = NULL;
    ascfree(cache);
  }
}


/*
 *********************************************************************
 * ExtRel_PreSolve:
 *
 * To deal with the first time we also want to do arguement
 * checking, and then turn off the first_func_eval flag.
 * Turn on the newcalc_done flag. The rationale behind this is
 * as follows:
 * The solver at the moment does not treat an external relation
 * specially, i.e., as a block. It also calls for its functions
 * a relation at a time. However the external relations compute
 * all their outputs at once. So as not to do unnecessary
 * recalculations we use these flag bits. We set newcalc_done
 * initially to true, so as to force *at least* one calculation
 * of the external relations. By similar reasoning first_func_eval (done)
 * is set to false.
 *********************************************************************
 */
int ExtRel_PreSolve(struct ExtRelCache *cache, int setup)
{
  struct ExternalFunc *efunc;
  struct Slv_Interp slv_interp;
  int ninputs,noutputs;
  double *inputs, *outputs, *jacobian;
  int (*init_func)(struct Slv_Interp *,
		   struct Instance *,struct gl_list_t *);
  int nok = 0;

  if (!cache) return 1;
  efunc = cache->efunc;
  init_func = GetInitFunc(efunc);
  Init_Slv_Interp(&slv_interp);
  slv_interp.nodestamp = cache->nodestamp;
  slv_interp.user_data = cache->user_data;
  if (setup) {
    slv_interp.first_call = (unsigned)1;
    slv_interp.last_call = (unsigned)0;
    slv_interp.check_args = (unsigned)1;
  }
  else{
    slv_interp.first_call = (unsigned)0;
    slv_interp.last_call = (unsigned)1;
    slv_interp.check_args = (unsigned)0;
  }
  nok = (*init_func)(&slv_interp,cache->data,cache->arglist);
  if (nok)
    return 1;

  /*
   * Save the user's data and update our status.
   */
  cache->user_data = slv_interp.user_data;
  cache->newcalc_done = (unsigned)1;	/* force at least one calculation */
  cache->first_func_eval = (unsigned)0;
  return 0;
}


static int ArgsDifferent(double new, double old)
{
  if (fabs(new - old) >= g_external_tolerance)
    return 1;
  else
    return 0;
}

real64 ExtRel_Evaluate_RHS(struct rel_relation *rel)
{
  struct Slv_Interp slv_interp;
  struct ExtRelCache *cache;
  struct ExternalFunc *efunc;
  struct Instance *arg;
  struct gl_list_t *inputlist;
  double value;
  int c,ninputs;
  int nok;
  unsigned long whichvar;
  int newcalc_reqd=0;

  int (*eval_func)(struct Slv_Interp *,
		   int ninputs, int noutputs,
		   double *inputs, double *outputs, double *jacobian);

  assert(rel_extnodeinfo(rel));
  cache = rel_extcache(rel);
  efunc = cache->efunc;
  eval_func = GetValueFunc(efunc);
  inputlist = cache->inputlist;
  ninputs = cache->ninputs;
  whichvar = (unsigned long)rel_extwhichvar(rel);

  /*
   * The determination of whether a new calculation is required needs
   * some more thought. One thing we should insist upon is that all
   * the relations for an external relation are forced into the same
   * block.
   */
  for (c=0;c<ninputs;c++) {
    arg = (struct Instance *)gl_fetch(inputlist,c+1);
    value = RealAtomValue(arg);
    if (ArgsDifferent(value,cache->inputs[c])) {
      newcalc_reqd = 1;
      cache->inputs[c] = value;
    }
  }
  value = 0;
  /*
   * Do the calculations if necessary. Note that we have to *ensure*
   * that we send the user the information that he provided to us.
   * We have to update our user_data after each call of the user's function
   * as he might move information around (not smart but possible), on us.
   * If a function call is made, mark a new calculation as having been,
   * done, otherwise dont.
   */
  if (newcalc_reqd) {
    Init_Slv_Interp(&slv_interp);
    slv_interp.nodestamp = cache->nodestamp;
    slv_interp.user_data = cache->user_data;
    slv_interp.func_eval = (unsigned)1;

    nok = (*eval_func)(&slv_interp, ninputs, cache->noutputs,
		       cache->inputs, cache->outputs, cache->jacobian);
    value = cache->outputs[whichvar - ninputs - 1];
    cache->newcalc_done = (unsigned)1;			/* newcalc done */
    cache->user_data = slv_interp.user_data;		/* update user_data */
  }
  else{
    value = cache->outputs[whichvar - ninputs - 1];
    cache->newcalc_done = (unsigned)0; /* a result was simply returned */
  }

#ifdef KAA_DEBUG
  FPRINTF(stderr,"Finsished calling ExtRel_Evaluate_RHS result ->%g\n",
	  result);
#endif

  return value;
}

	/******* FIX FIX FIX **********/
real64 ExtRel_Evaluate_LHS(rel)
struct rel_relation *rel;
{
  real64 res;
  res = 1.0;		/******* FIX FIX FIX **********/
  FPRINTF(stderr,"Finsished calling ExtRel_Evaluate_LHS result ->%g\n",
	  res);
  return res;
}


/*
 *********************************************************************
 * ExtRel_Gradient evaluation routines.
 *
 * The following code implements gradient evaluation routines for
 * external relations. The routines here will prepare the arguements
 * and call a user supplied derivative routine, is same is non-NULL.
 * If it is the user supplied function evaluation routine will be
 * used to compute the gradients via finite differencing.
 * The current solver will not necessarily call for the derivative
 * all at once. This makes it necessary to do the gradient
 * computations (user supplied or finite difference), and to cache
 * away the results. Based on calculation flags, the appropriate
 * *row* of this cached jacobian will be extracted and mapped to the
 * main solve matrix.
 *
 * The cached jacobian is a contiguous vector ninputs*noutputs long
 * and is loaded row wise. Indexing starts from 0. Each row corresponds
 * to the partial derivatives of the output variable (associated with
 * that row, wrt to all its incident input variables.
 *
 * Careful attention needs to be paid to the way this jacobian is
 * loaded/unloaded, because of the multiple indexing schemes in use.
 * i.e, arglist's and inputlists index 1..nvars, whereas all numeric
 * vectors number from 0.
 *
 *********************************************************************
 */

struct deriv_data {
  var_filter_t *filter;
  mtx_matrix_t mtx;
  mtx_coord_t nz;
};


/*
 *********************************************************************
 * ExtRel_MapDataToMtx
 *
 * This function attempts to map the information from the contiguous
 * jacobian back into the main matrix, based on the current row <=>
 * whichvar. The jacobian assumed to have been calculated.
 * Since we are operating a relation at a time, we have to find
 * out where to index into our jacobian. This index is computed as
 * follows:
 *
 * index = (whichvar - ninputs - 1) * ninputs
 *
 * Example: a problem with 4 inputs, 3 outputs and whichvar = 6.
 * with the counting for vars 1..nvars, but the jacobian indexing
 * starting from 0 (c-wise).
 *
 *          V-------- first output variable
 *  1 2 3 4 5 6 7
 *            ^---------------- whichvar
 *				    ------------------- grads for whichvar = 6
 *				    |    |    |    |
 *				    v    v    v    v
 *  index    =	0    1    2    3    4    5    6    7   8    9   10    11
 *  jacobian = 2.0  9.0  4.0  6.0  0.5  1.3  0.0  9.7  80  7.0  1.0  2.5
 *
 * Hence jacobian index = (6 - 4 - 1) * 4 = 4
 *********************************************************************
 */

static void ExtRel_MapDataToMtx(struct gl_list_t *inputlist,
				unsigned long whichvar,
				int ninputs,
				double *jacobian,
				struct deriv_data *d)
{
  struct Instance *inst;
  struct var_variable *var;
  double value, *ptr;
  boolean used;
  unsigned long c;
  int index;

  index = ((int)whichvar - ninputs - 1) * ninputs;
  ptr = &jacobian[index];

/* this is totally broken, thanks to kirk making the var=instance assumption */
  Asc_Panic(2, "ExtRel_MapDataToMtx",
            "ExtRel_MapDataToMtx is totally broken\n"
            "Makes a var=instance assumption");
  for (c=0;c<ninputs;c++) {
    inst = (struct Instance *)gl_fetch(inputlist,c+1);
    var = var_instance(inst);
    used = var_apply_filter(var,d->filter);
    if (used) {
      d->nz.col = mtx_org_to_col(d->mtx,var_sindex(var));
      value = ptr[c] + mtx_value(d->mtx,&(d->nz));
      mtx_set_value(d->mtx,&(d->nz), value);
    }
  }
}


/*
 *********************************************************************
 * ExtRel Finite Differencing.
 *
 * This routine actually does the finite differencing.
 * The jacobian is a single contiguous vector. We load information
 * in it *row* wise. If we have noutputs x ninputs = 3 x 4, variables,
 * then jacobian entry 4,
 * would correspond to jacobian[1][0], i.e., = 0.5 for this eg.
 *
 *	  2.0  9.0   4.0  6.0
 *	  0.5  1.3   0.0  9.7
 *	  80   7.0   1.0  2.5
 *
 *  2.0  9.0  4.0  6.0  0.5  1.3  0.0  9.7  80  7.0  1.0  2.5
 * [0][0]              [1][0]		      [2][1]
 *
 * When we are finite differencing variable c, we will be loading
 * jacobian positions c, c+ninputs, c+2*ninputs ....
 *********************************************************************
 */

static double CalculateInterval(double var_value)
{
  return (1.0e-05);
}

static int ExtRel__FDiff(struct Slv_Interp *slv_interp,
			 int (*eval_func) (/* ARGS */),
			 int ninputs, int noutputs,
			 double *inputs, double *outputs,
			 double *jacobian)
{
  int c1,c2, nok = 0;
  double *tmp_vector;
  double *ptr;
  double old_x,interval,value;

  tmp_vector = (double *)asccalloc(noutputs,sizeof(double));
  for (c1=0;c1<ninputs;c1++) {
    old_x = inputs[c1];					/* perturb x */
    interval = CalculateInterval(old_x);
    inputs[c1] = old_x + interval;
    nok = (*eval_func)(slv_interp, ninputs, noutputs,	/* call routine */
		       inputs, tmp_vector, jacobian);
    if (nok) break;
    ptr = &jacobian[c1];
    for (c2=0;c2<noutputs;c2++) {			/* load jacobian */
      value = (tmp_vector[c2] - outputs[c2])/interval;
      *ptr = value;
      ptr += ninputs;
    }
    inputs[c1] = old_x;
  }
  ascfree((char *)tmp_vector);				/* cleanup */
  return nok;
}


int ExtRel_CalcDeriv(struct rel_relation *rel, struct deriv_data *d)
{
  int nok = 0;
  struct Slv_Interp slv_interp;
  struct ExtRelCache *cache;
  struct ExternalFunc *efunc;
  unsigned long whichvar;
  double *deriv_vector;
  int (*eval_func)();
  int (*deriv_func)();

  assert(rel_extnodeinfo(rel));
  cache = rel_extcache(rel);
  whichvar = rel_extwhichvar(rel);
  efunc = cache->efunc;

  /*
   * Check and deal with the special case of the first
   * computation.
   */
  if (cache->first_deriv_eval) {
    cache->newcalc_done = (unsigned)1;
    cache->first_deriv_eval = (unsigned)0;
  }

  /*
   * If a function evaluation was not recently done, then we
   * can return the results from the cached jacobian.
   */
  if (!cache->newcalc_done) {
    ExtRel_MapDataToMtx(cache->inputlist, whichvar,
			cache->ninputs, cache->jacobian, d);
    return 0;
  }

  /*
   * If we are here, we have to do the derivative calculation.
   * The only thing to determine is whether we do analytical
   * derivatives (deriv_func != NULL) or finite differencing.
   * In any case init the interpreter.
   */
  Init_Slv_Interp(&slv_interp);
  slv_interp.deriv_eval = (unsigned)1;
  slv_interp.user_data = cache->user_data;
  deriv_func = GetDerivFunc(efunc);
  if (deriv_func) {
    nok = (*deriv_func)(&slv_interp, cache->ninputs, cache->noutputs,
			cache->inputs, cache->outputs, cache->jacobian);
    if (nok) return nok;
  }
  else{
    eval_func = GetValueFunc(efunc);
    nok = ExtRel__FDiff(&slv_interp, eval_func,
			cache->ninputs, cache->noutputs,
			cache->inputs, cache->outputs, cache->jacobian);
    if (nok) return nok;
  }

  /*
   * Cleanup. Ensure that we update the users data, and load
   * the main matrix with the derivative information.
   */
  cache->user_data = slv_interp.user_data;	/* save user info */
  ExtRel_MapDataToMtx(cache->inputlist, whichvar,
		      cache->ninputs, cache->jacobian, d);
  return 0;
}


/*
 *********************************************************************
 * ExtRel Deriv routines.
 *
 * This is the entry point for most cases. ExtRel_CalcDeriv depends
 * on ExtRel_Evaluate being called  immediately before it.
 *
 *********************************************************************
 */

real64 ExtRel_Diffs_RHS(struct rel_relation *rel,
			      var_filter_t *filter,
			      int32 row,
			      mtx_matrix_t mtx)
{
  int nok;
  real64 res;
  struct deriv_data data;

  data.filter = filter;
  data.mtx = mtx;
  data.nz.row = row;

  res = ExtRel_Evaluate_RHS(rel);
  nok = ExtRel_CalcDeriv(rel,&data);
  if (nok)
    return 0.0;
  else
    return res;
}



	/******* FIX FIX FIX **********/

real64 ExtRel_Diffs_LHS(struct rel_relation *rel,
			      var_filter_t *filter,
			      int32 row,
			      mtx_matrix_t mtx)
{
  real64 res=0.0;
  return 1.0;	/******* FIX FIX FIX **********/
}










