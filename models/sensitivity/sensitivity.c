/*	ASCEND modelling environment
	Copyright (C) 1996-2007 Carnegie Mellon University

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

	The module permits sensitivity analysis in an ASCEND model. In your model
	file, you need to create some additional arrays like so:

	X[1..m] IS_A solver_var; (* outputs *)
	U[1..n] IS_A solver_var; (* inputs *)
	dx_du[1..m][1..n] IS_A solver_var; (* data space *)

	You must then 'ARE_THE_SAME' your problem variables into the spaces in the
	first two of the above arrays.

	Then, run EXTERNAL do_sensitivity(SELF,U[1..n],X[1..m],dx_du[1..m][1..n]);

	In the array dx_du[1..m][1..n], values of the corresponding sensitivity
	derivatives will be calculated.

	A test/example model is available in the models directory, called
	sensitivity_test.a4c.

	--- implementation notes ---

	This file attempts to implement the extraction of dy_dx from
	a system of equations. If one considers a black-box where x are
	the input variables, u are input parameters, y are the output
	variables, f(x,y,u) is the system of equations that will be solved
	for given x and u, then one can extract the sensitivity information
	of y wrt x.

	One crude but simple way of doing this is to finite-difference the
	given black box, i.e, perturb x, n_x times by delta x, resolve
	the system of equations at the new value of x, and compute

	    dy/dx = (f(x_1) - f(x_2))/(x_1 - x_2).

 	This is known to be very expensive.

	The solution that will be adopted here is to formulate the Jacobian J of
	the system, (or the system is already solved, to grab the jacobian at
	the solution. Develop the sensitivity matrix df/dx by exact numnerical
	differentiation; this will be a n x n_x matrix. Compute the LU factors
	for J, and to solve column by column to : LU dz/dx = df/dx. Here
	z, represents all the internal variables to the system and the strictly
	output variables y. In other words J = df/dz.

	Given the solution of the above problem we then simply extract the rows
	of dz/dx, that pertain to the y variables that we are interested in;
	this will give us dy/dx.

	@todo There are files in tcltk called Sensitivity.[ch]. Do we need them?
*/

#include <math.h>

#include <packages/sensitivity.h>
#include <compiler/instquery.h>
#include <compiler/atomvalue.h>
#include <utilities/ascMalloc.h>
#include <compiler/extfunc.h>
#include <general/mathmacros.h>

/* #define SENSITIVITY_DEBUG */

ASC_EXPORT int sensitivity_register(void);

ExtMethodRun do_sensitivity_eval;
ExtMethodRun do_sensitivity_eval_all;



/**
	Allocate memory for a matrix
	@param nrows Number of rows
	@param ncols Number of colums
	@return Pointer to the allocated matrix memory location
*/
real64 **make_matrix(int nrows, int ncols){
  real64 **result;
  int i;
  result = (real64 **)calloc(nrows,sizeof(real64*));
  for (i=0;i<nrows;i++) {
    result[i] = (real64 *)calloc(ncols,sizeof(real64));
  }
  return result;
}

/**
	Free a matrix from memory
	@param matrix Memory location for the matrix
	@param nrows Number of rows in the matrix
*/
void free_matrix(real64 **matrix, int nrows){
  int i;
  if (!matrix)
    return;
  for (i=0;i<nrows;i++) {
    if (matrix[i]) {
      free(matrix[i]);
      matrix[i] = NULL;
    }
  }
  free(matrix);
}


/**
	Build then presolve an instance
*/
slv_system_t sens_presolve(struct Instance *inst){
  slv_system_t sys;
  slv_parameters_t parameters;
  int ind;
#ifdef SENSITIVITY_DEBUG
  struct var_variable **vp;
  struct rel_relation **rp;
  int len;
  char *tmp=NULL;
#endif
  sys = system_build(inst);
  if (sys==NULL) {
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to build system.");
    return NULL;
  }
  if (g_SlvNumberOfRegisteredClients == 0) {
    return NULL;
  }
  ind = 0;
  while (strcmp(slv_solver_name(ind),"QRSlv")) {
    if (ind >= g_SlvNumberOfRegisteredClients) {
	  ERROR_REPORTER_HERE(ASC_PROG_ERR,
        "QRSlv must be registered client.");
      return NULL;
    }
    ++ind;
  }
  slv_select_solver(sys,ind);

  slv_get_parameters(sys,&parameters);
  parameters.partition = 0;
  slv_set_parameters(sys,&parameters);
  slv_presolve(sys);

#ifdef SENSITIVITY_DEBUG
  vp = slv_get_solvers_var_list(sys);
  len = slv_get_num_solvers_vars(sys);
  for (ind=0 ; ind<len; ind++) {
    tmp = var_make_name(sys,vp[ind]);
    CONSOLE_DEBUG("%s  %d\n",tmp,var_sindex(vp[ind]));
    if (tmp!=NULL) ascfree(tmp);
  }
  rp = slv_get_solvers_rel_list(sys);
  len = slv_get_num_solvers_rels(sys);
  for (ind=0 ; ind<len ; ind++) {
    tmp = rel_make_name(sys,rp[ind]);
    CONSOLE_DEBUG("%s  %d\n",tmp,rel_sindex(rp[ind]));
    if (tmp) ascfree(tmp);
  }
#endif
  return sys;
}

/*
	LU Factor Jacobian

	@note The RHS will have been  will already have been added before
		calling this function.

	@NOTE there is another version of this floating around in packages/senstivity.c. The
	other one uses dense matrices so probably this one's better?
*/
int LUFactorJacobian1(slv_system_t sys,int rank){
  linsolqr_system_t lqr_sys;
  mtx_region_t region;
  enum factor_method fm;

  mtx_region(&region,0,rank-1,0,rank-1);	/* set the region */
  lqr_sys = slv_get_linsolqr_sys(sys);	/* get the linear system */

  linsolqr_matrix_was_changed(lqr_sys);
  linsolqr_reorder(lqr_sys,&region,natural);

  fm = linsolqr_fmethod(lqr_sys);
  if (fm == unknown_f) fm = ranki_kw2; /* make sure somebody set it */
  linsolqr_factor(lqr_sys,fm);

  return 0;
}

int sensitivity_anal(
	     struct Instance *inst, /* not used but will be */
	     struct gl_list_t *arglist
){
	struct Instance *which_instance,*tmp_inst, *atominst;
	struct gl_list_t *branch;
	struct var_variable **vlist = NULL;
	int *inputs_ndx_list = NULL, *outputs_ndx_list = NULL;
	real64 **dy_dx = NULL;
	slv_system_t sys = NULL;
	int c;
	int noutputs = 0;
	int ninputs;
	int i,j;
	int offset;
	dof_t *dof;
	int num_vars,ind,found;

	linsolqr_system_t lqr_sys;	/* stuff for the linear system & matrix */
	mtx_matrix_t mtx;
	int32 capacity;
	real64 *scratch_vector = NULL;
	int result=0;

	/* Ignore unused params */
	(void) inst;

	(void)NumberFreeVars(NULL);		/* used to re-init the system */
	(void)NumberRels(NULL);		/* used to re-init the system */
	which_instance = FetchElement(arglist,1,1);
	sys = sens_presolve(which_instance);
	if (!sys) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to presolve");
		result = 1;
		goto finish;
	}

	dof = slv_get_dofdata(sys);
	if (!(dof->n_rows == dof->n_cols &&
		dof->n_rows == dof->structural_rank)) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"System is not square");
		result = 1;
		goto finish;
	}

	CONSOLE_DEBUG("Presolved, square");

	/*
		prepare the inputs list
	*/
	vlist = slv_get_solvers_var_list(sys);

	branch = gl_fetch(arglist,2); /* input args */
	ninputs = gl_length(branch);
	inputs_ndx_list = ASC_NEW_ARRAY(int,ninputs);

	num_vars = slv_get_num_solvers_vars(sys);
	for (c=0;c<ninputs;c++) {
		atominst = (struct Instance *)gl_fetch(branch,c+1);
		found = 0;
		ind = num_vars - 1;
		/* search backwards because fixed vars are at the end of the
	       var list */
	    while (!found && ind >= 0){
			if (var_instance(vlist[ind]) == atominst) {
				inputs_ndx_list[c] = var_sindex(vlist[ind]);
				found = 1;
			}
			--ind;
		}
		if(!found){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to find sensitivity input variable");
			result = 1;
			goto finish;
		}
	}

	CONSOLE_DEBUG("%d inputs",ninputs);

	/*
		prepare the outputs list
	*/
	branch = gl_fetch(arglist,3); /* output args */
	noutputs = gl_length(branch);
	outputs_ndx_list = ASC_NEW_ARRAY(int,noutputs);
	for (c=0;c<noutputs;c++) {
		atominst = (struct Instance *)gl_fetch(branch,c+1);
		found = 0;
		ind = 0;
		while (!found && ind < num_vars){
			if (var_instance(vlist[ind]) == atominst) {
				outputs_ndx_list[c] = var_sindex(vlist[ind]);
				found = 1;
			}
			++ind;
		}
		if (!found) {
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to find sensitivity ouput variable");
			result = 1;
			goto finish;
		}
	}
	
	CONSOLE_DEBUG("%d outputs",noutputs);

	/*
		prepare the results dy_dx.
	*/
	dy_dx = make_matrix(noutputs,ninputs);

	result = Compute_J(sys);
	if (result) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to calculate Jacobian failure in calc Jacobian\n");
		goto finish;
	}

	CONSOLE_DEBUG("Computed Jacobian");

	/*
		Note: the RHS *has* to added here. We will construct the vector
		of size matrix capacity and add it. It will be removed after
		we finish computing dy_dx.
	*/
	lqr_sys = slv_get_linsolqr_sys(sys);	/* get the linear system */
	mtx = linsolqr_get_matrix(lqr_sys);	/* get the matrix */
	capacity = mtx_capacity(mtx);
	scratch_vector = ASC_NEW_ARRAY_CLEAR(real64,capacity);
	linsolqr_add_rhs(lqr_sys,scratch_vector,FALSE);
	result = LUFactorJacobian1(sys,dof->structural_rank);
	if(result){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to calculate LUFactorJacobian");
		goto finish;
	}
	result = Compute_dy_dx_smart(sys, scratch_vector, dy_dx,
		inputs_ndx_list, ninputs,
		outputs_ndx_list, noutputs
	);

	linsolqr_remove_rhs(lqr_sys,scratch_vector);
	if (result) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed Compute_dy_dx");
		goto finish;
	}
	
	CONSOLE_DEBUG("Computed dy/dx");

	/*
		Write the results back to the partials array in the
		instance tree
	*/
	offset = 0;
	for (i=0;i<noutputs;i++) {
		for (j=0;j<ninputs;j++) {
			tmp_inst = FetchElement(arglist,4,offset+j+1);
			SetRealAtomValue(tmp_inst,dy_dx[i][j],(unsigned)0);
#ifdef SENSITIVITY_DEBUG
			CONSOLE_DEBUG("%12.8f   i%d j%d",dy_dx[i][j],i,j);
#endif
		}
#ifdef SENSITIVITY_DEBUG
		CONSOLE_DEBUG("\n");
#endif
		offset += ninputs;
	}
#ifdef SENSITIVITY_DEBUG
	CONSOLE_DEBUG("\n");
#endif

	ERROR_REPORTER_HERE(ASC_USER_SUCCESS
		,"Sensitivity results for %d vars were written back to the model"
		,noutputs
	);

finish:
	if (inputs_ndx_list) ascfree((char *)inputs_ndx_list);
	if (outputs_ndx_list) ascfree((char *)outputs_ndx_list);
	if (dy_dx) free_matrix(dy_dx,noutputs);
	if (scratch_vector) ascfree((char *)scratch_vector);
	if (sys) system_destroy(sys);
	return result;
}

/**
	Do Data Analysis. Used by sensitivity_anal_all.
*/
int DoDataAnalysis(struct var_variable **inputs,
			  struct var_variable **outputs,
			  int ninputs, int noutputs,
			  real64 **dy_dx)
{
  FILE *fp;
  double *norm_2, *norm_1;
  double input_nominal,maxvalue,sum;
  int i,j;

  norm_1 = ASC_NEW_ARRAY_CLEAR(double,ninputs);
  norm_2 = ASC_NEW_ARRAY_CLEAR(double,ninputs);

  fp = fopen("sensitivity.out","w+");
  if (!fp) return 0;

  /*
   * calculate the 1 and 2 norms; cache them away so that we
   * can pretty print them. Style is everything !.
   *
   */
  for (j=0;j<ninputs;j++) {
    input_nominal = var_nominal(inputs[j]);
    maxvalue = sum = 0;
    for (i=0;i<noutputs;i++) {
      dy_dx[i][j] *= input_nominal/var_nominal(outputs[i]);
      maxvalue = MAX(fabs(dy_dx[i][j]),maxvalue);
      sum += dy_dx[i][j]*dy_dx[i][j];
    }
    norm_1[j] = maxvalue;
    norm_2[j] = sum;
  }

  for (j=0;j<ninputs;j++) {		/* print the var_index */
    fprintf(fp,"%8d    ",var_mindex(inputs[j]));
  }
  fprintf(fp,"\n");

  for (j=0;j<ninputs;j++) {		/* print the 1 norms */
    fprintf(fp,"%-#18.8f    ",norm_1[j]);
  }
  fprintf(fp,"\n");

  for (j=0;j<ninputs;j++) {		/* print the 2 norms */
    fprintf(fp,"%-#18.8f    ",norm_2[j]);
  }
  fprintf(fp,"\n\n");
  ascfree((char *)norm_1);
  ascfree((char *)norm_2);

  for (i=0;i<noutputs;i++) {		/* print the scaled data */
    for (j=0;j<ninputs;j++) {
      fprintf(fp,"%-#18.8f   %-4d",dy_dx[i][j],i);
    }
    if (var_fixed(outputs[i]))
      fprintf(fp,"    **fixed*** \n");
    else
      PUTC('\n',fp);
  }
  fprintf(fp,"\n");
  fclose(fp);
  return 0;
}


/**
	This function is very similar to sensitivity_anal, execept,
	that it perform the analysis on the entire system, based on
	the given inputs. Nothing is returned. As such the call from
	ASCEND is:

<pre>
EXTERN sensitivity_anal_all(
	this_instance,
	u_old[1..n_inputs],
	u_new[1..n_inputs],
	step_length
);</pre>

	The results will be witten to standard out.
	This function is more expensive from a a memory point of view,
	as we keep around a dense matrix n_outputs * n_inputs, but here
	n_outputs may be *much* larger depending on problem size.
*/
int sensitivity_anal_all( struct Instance *inst,  /* not used but will be */
		struct gl_list_t *arglist,
		real64 step_length
){
	struct Instance *which_instance;
	struct gl_list_t *branch2, *branch3;
	dof_t *dof;
	struct var_variable **inputs = NULL, **outputs = NULL;
	int *inputs_ndx_list = NULL, *outputs_ndx_list = NULL;
	real64 **dy_dx = NULL;
	struct var_variable **vp,**ptr;
	slv_system_t sys = NULL;
	long c;
	int noutputs=0, ninputs;
	var_filter_t vfilter;

	struct var_variable **new_inputs = NULL; /* optional stuff for variable
				      * projection */

	linsolqr_system_t lqr_sys;	/* stuff for the linear system & matrix */
	mtx_matrix_t mtx;
	int32 capacity;
	real64 *scratch_vector = NULL;
	int result=0;

	/* Ignore unused params */
	(void)inst; (void)step_length;

	/*
	* Call the presolve for the system. This should number variables
	* and relations as well create and order the main Jacobian. The
	* only potential problem that I see here is that presolve using
	* the slv0 solver *only* recognizes solver vars. So that if one
	* wanted to see the sensitivity of a *parameter*, it would not
	* be possible. We will have to trap this in CheckArgs.
	*
	* Also the current version of ascend is fucked in how the var module
	* handles variables and their numbering through the interface ptr
	* crap.
	*/

	(void)NumberFreeVars(NULL);		/* used to re-init the system */
	(void)NumberRels(NULL);		/* used to re-init the system */
	which_instance = FetchElement(arglist,1,1);
	sys = sens_presolve(which_instance);
	if (!sys) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to presolve");
		result = 1;
		goto finish;
	}
	dof = slv_get_dofdata(sys);

	/*
	* prepare the inputs list. We dont need the index of the new_inputs
	* list. We will grab them later if necessary.
	*/
	branch2 = gl_fetch(arglist,2); /* input args -- old u_values */
	branch3 = gl_fetch(arglist,3); /* input args -- new u_values */
	ninputs = gl_length(branch2);
	inputs = ASC_NEW_ARRAY(struct var_variable *,ninputs+1);
	new_inputs = ASC_NEW_ARRAY(struct var_variable *,ninputs+1);

	inputs_ndx_list = ASC_NEW_ARRAY(int,ninputs);
	for (c=0;c<ninputs;c++) {
		inputs[c] = (struct var_variable *)gl_fetch(branch2,c+1);
		inputs_ndx_list[c] = var_mindex(inputs[c]);
		new_inputs[c] = (struct var_variable *)gl_fetch(branch3,c+1);
	}
	inputs[ninputs] = NULL; 	/* null terminate the list */
	new_inputs[ninputs] = NULL; 	/* null terminate the list */

	/*
	* prepare the outputs list. This is where we differ from
	* the other function. The noutputs, and the indexes of these
	* outputs is obtained from the entire solve system.
	*/
	vfilter.matchbits = 0;
	noutputs = slv_count_solvers_vars(sys,&vfilter);
	outputs = ASC_NEW_ARRAY(struct var_variable *,noutputs+1);
	outputs_ndx_list = ASC_NEW_ARRAY(int,noutputs);
	ptr = vp = slv_get_solvers_var_list(sys);
	for (c=0;c<noutputs;c++) {
		outputs[c] = *ptr;
		outputs_ndx_list[c] = var_sindex(*ptr);
		ptr++;
	}
	outputs[noutputs] = NULL; /* null terminate the list */

	/*
	* prepare the results dy_dx. This is the expensive part from a
	* memory point of view. However I would like to have the entire
	* noutputs * ninputs matrix even for a short while so that I
	* can compute a number of  different types of norms.
	*/
	dy_dx = make_matrix(noutputs,ninputs);

	result = Compute_J(sys);
	if (result) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to compute Jacobian");
		goto finish;
	}

	/*
		Note: the RHS *has* to added here. We will construct the vector
		of size matrix capacity and add it. It will be removed after
		we finish computing dy_dx.
	*/
	lqr_sys = slv_get_linsolqr_sys(sys);	/* get the linear system */
	mtx = linsolqr_get_matrix(lqr_sys);	/* get the matrix */
	capacity = mtx_capacity(mtx);
	scratch_vector = ASC_NEW_ARRAY_CLEAR(real64,capacity);
	linsolqr_add_rhs(lqr_sys,scratch_vector,FALSE);
	result = LUFactorJacobian1(sys,dof->structural_rank);

	if (result) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failure in LUFactorJacobian");
		goto finish;
	}

	result = Compute_dy_dx_smart(sys, scratch_vector, dy_dx,
		inputs_ndx_list, ninputs,
		outputs_ndx_list, noutputs
	);

	linsolqr_remove_rhs(lqr_sys,scratch_vector);
	if (result) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failure in Compute_dy_dx");
		goto finish;
	}

	/*
	* Do some analysis on the results, inclusive of
	* writing them to someplace useful.
	*/
	if(DoDataAnalysis(inputs, outputs, ninputs, noutputs, dy_dx)){
		result = 1;
	}
	/*
	* Some experimental projection stuff -- not used now.
	* if (DoProject_X(inputs, new_inputs, step_length,
	*     outputs, ninputs, noutputs, dy_dx))
	* result = 1;
	*/

finish:
	if (inputs) ascfree((char *)inputs);
	if (new_inputs) ascfree((char *)new_inputs);
	if (inputs_ndx_list) ascfree((char *)inputs_ndx_list);
	if (outputs) ascfree((char *)outputs);
	if (outputs_ndx_list) ascfree((char *)outputs_ndx_list);
	if (dy_dx) free_matrix(dy_dx,noutputs);
	if (scratch_vector) ascfree((char *)scratch_vector);
	if (sys) system_destroy(sys);
	return result;
}




#if 0
static int ComputeInverse(slv_system_t sys,
			  real64 *rhs)
{
  linsolqr_system_t lqr_sys;
  mtx_matrix_t mtx;
  int capacity,order;
  real64 *solution = NULL;
  int j,k;

  lqr_sys = slv_get_linsolqr_sys(sys); 	/* get the linear system */
  mtx = linsolqr_get_matrix(lqr_sys); 		/* get the matrix */

  capacity = mtx_capacity(mtx);
  zero_vector(rhs,capacity);		/* zero the rhs */
  solution = ASC_NEW_ARRAY_CLEAR(real64,capacity);

  order = mtx_order(mtx);
  for (j=0;j<order;j++) {
    rhs[j] = 1.0;
    linsolqr_rhs_was_changed(lqr_sys,rhs);
    linsolqr_solve(lqr_sys,rhs);			/* solve */
    linsolqr_copy_solution(lqr_sys,rhs,solution);	/* get the solution */

    CONSOLE_DEBUG("This is the rhs and solution for rhs #%d\n",j);
    for (k=0;k<order;k++) {
      CONSOLE_DEBUG("%12.8g  %12.8g\n",rhs[k],solution[k]);
    }
    rhs[j] = 0.0;
  }
  if (solution) ascfree((char *)solution);
  return 0;
}
#endif

#if 0
static int DoProject_X(struct var_variable **old_inputs,
		       struct var_variable **new_inputs, /* new values of u */
		       double step_length,
		       struct var_variable **outputs,
		       int ninputs, int noutputs,
		       real64 **dy_dx)
{
  struct var_variable *var;
  real64 old_y, new_y, tmp;
  real64 *delta_x;
  int i,j;

  delta_x = ASC_NEW_ARRAY_CLEAR(real64,ninputs);

  for (j=0;j<ninputs;j++) {
    delta_x[j] = var_value(new_inputs[j]) - var_value(old_inputs[j]);
    /*    delta_x[j] = RealAtomValue(new_inputs[j]) - RealAtomValue(old_inputs[j]); */
  }

  for (i=0;i<noutputs;i++) {
    var = outputs[i];
    if (var_fixed(var) || !var_active(var))    /* project only the free vars */
      continue;
    tmp = 0.0;
    for (j=0;j<ninputs;j++) {
      tmp += (dy_dx[i][j] * delta_x[j]);
    }
    /*    old_y = RealAtomValue(var); */
    old_y = var_value(var);
    new_y = old_y + step_length*tmp;
    /*    SetRealAtomValue(var,new_y,(unsigned)0);  */
    var_set_value(var,new_y);
# \if  DEBUG
    CONSOLE_DEBUG("Old_y = %12.8g; Nex_y = %12.8g\n",old_y,new_y);
# \endif
  }
  ascfree((char *)delta_x);
  return 0;
}
#endif


#if 0
/**
 * At this point we should have an empty jacobian. We now
 * need to call relman_diff over the *entire* matrix.
 * Calculates the entire jacobian. It is initially unscaled.
 *
 * Note: this assumes the sys given is using one of the ascend solvers
 * with either linsol or linsolqr. Both are now allowed. baa 7/95
 */
#define SAFE_FIX_ME 0
static int Compute_J_OLD(slv_system_t sys)
{
  int32 row;
  var_filter_t vfilter;
  linsol_system_t lin_sys = NULL;
  linsolqr_system_t lqr_sys = NULL;
  mtx_matrix_t mtx;
  struct rel_relation **rlist;
  int nrows;
  int qr=0;
#\if DOTIME
  double time1;
#\endif

#\if DOTIME
  time1 = tm_cpu_time();
#\endif
  /*
   * Get the linear system from the solve system.
   * Get the matrix from the linear system.
   * Get the relations list from the solve system.
   */
  lin_sys = slv_get_linsol_sys(sys);
  if (lin_sys==NULL) {
    qr=1;
    lqr_sys=slv_get_linsolqr_sys(sys);
  }
  mtx = slv_get_sys_mtx(sys);
  if (mtx==NULL || (lin_sys==NULL && lqr_sys==NULL)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Compute_dy_dx: error, found NULL.\n");
    return 1;
  }
  rlist = slv_get_solvers_rel_list(sys);
  nrows = NumberIncludedRels(sys);

  calc_ok = TRUE;
  vfilter.matchbits = VAR_SVAR;
  vfilter.matchvalue = VAR_SVAR;
  /*
   * Clear the entire matrix and then compute
   * the gradients at the current point.
   */
  mtx_clear_region(mtx,mtx_ENTIRE_MATRIX);
  for(row=0; row<nrows; row++) {
    struct rel_relation *rel;
    /* added  */
    double resid;

    rel = rlist[mtx_row_to_org(mtx,row)];
    (void)relman_diffs(rel,&vfilter,mtx,&resid,SAFE_FIX_ME);

    /* added  */
    rel_set_residual(rel,resid);

  }
  if (qr) {
    linsolqr_matrix_was_changed(lqr_sys);
  } else {
    linsol_matrix_was_changed(lin_sys);
  }
#\if DOTIME
  time1 = tm_cpu_time() - time1;
  ERROR_REPORTER_HERE(ASC_PROG_ERR,"Time taken for Compute_J = %g\n",time1);
#\endif
  return(!calc_ok);
}
#endif


/**
	Sensitivity Analysis

	@param arglist List of arguments

	Argument list contains the following:
	  . arg1 - model inst for which the sensitivity analysis is to be done.
	  . arg2 - array of input instances.
	  . arg3 - array of output instances.
	  . arg4 matrix of partials to be written to.
*/
int SensitivityCheckArgs(struct gl_list_t *arglist){
  struct Instance *inst;
  unsigned long len;
  unsigned long ninputs, noutputs;

  len = gl_length(arglist);
  if (len != 4) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"wrong number of args -- 4 expected\n");
    return 1;
  }
  inst = FetchElement(arglist,1,1);
  if (InstanceKind(inst)!=MODEL_INST) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Arg #1 is not a model instance\n");
    return 1;
  }
  ninputs = gl_length((struct gl_list_t *)gl_fetch(arglist,2));
    /* input args */
  noutputs = gl_length((struct gl_list_t *)gl_fetch(arglist,3));
   /* output args */
  len = gl_length((struct gl_list_t *)gl_fetch(arglist,4));
        /* partials matrix */
  if (len != (ninputs*noutputs)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,
	    "The array of partials is inconsistent with the args given\n");
    return 1;
  }
  return 0;
}


int do_sensitivity_eval( struct Instance *i,
		struct gl_list_t *arglist, void *user_data
){
	CONSOLE_DEBUG("Starting sensitivity analysis...");
	if(SensitivityCheckArgs(arglist))return 1;

	return sensitivity_anal(i,arglist);
}


/**
	@param arglist List of arguments
	@param step_length ...?

	The list of arguments should chontain

   	  . arg1 - model inst for which the sensitivity analysis is to be done.
	  . arg2 - array of input instances.
	  . arg3 - new_input instances, for variable projection.
	  . arg4 - instance representing the step_length for projection.

	The result will be written to standard out.
*/
int SensitivityAllCheckArgs(struct gl_list_t *arglist, double *step_length)
{
  struct Instance *inst;
  unsigned long len;

  len = gl_length(arglist);
  if (len != 4) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"wrong number of args -- 4 expected\n");
    return 1;
  }
  inst = FetchElement(arglist,1,1);
  if (InstanceKind(inst)!=MODEL_INST) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Arg #1 is not a model instance\n");
    return 1;
  }
  /*
   * we should be checking that arg2 list contains solver vars
   * and that they are fixed. The same applies to arglist 3... later.
   * We will check and return the steplength though. 0 means dont do
   * the variable projection.
   */
  inst = FetchElement(arglist,4,1);
  *step_length = RealAtomValue(inst);
  if (fabs(*step_length) < 1e-08)
    *step_length = 0.0;
  return 0;
}


int do_sensitivity_eval_all( struct Instance *i,
		struct gl_list_t *arglist, void *user_data
){
	int result;
	double step_length = 0.0;
	if (SensitivityAllCheckArgs(arglist,&step_length)) {
		return 1;
	}
	result = sensitivity_anal_all(i,arglist,step_length);
	return result;
}


const char sensitivity_help[] =
	"This function does sensitivity analysis dy/dx. It requires 4 args:\n\n"
	"  1. name: name of a reference instance or SELF.\n"
	"  2. x: x, where x is an array of > solver_var.\n"
	"  3. y: where y is an array of > solver_var.\n"
	"  4. dy/dx: which dy_dx[1..n_y][1..n_x].\n\n"
	"See also sensitivity_anal_all.";

/** @TODO document what 'u_new' is all about...? */

const char sensitivity_all_help[] =
	"Analyse the sensitivity of *all* variables in the system with respect\n"
	"to the specific set of inputs 'u'. Instead of returning values to a\n"
	"a special array inside the model, the results are written to the\n"
	"console. Usage example:\n\n"
	"EXTERN sensitivity_anal_all(\n"
	"	this_instance,\n"
	"	u_old[1..n_inputs],\n"
	"	u_new[1..n_inputs],\n"
	"	step_length\n"
	");\n\n"
	"See also sensitivity_anal.";

int sensitivity_register(void){
	int result=0;

	result += CreateUserFunctionMethod("do_sensitivity",
		do_sensitivity_eval,
		4,sensitivity_help,NULL,NULL
	);
	result += CreateUserFunctionMethod("do_sensitivity_all",
		do_sensitivity_eval_all,
		4,sensitivity_all_help,NULL,NULL
	);

	return result;
}

/* :ex: set ts=4: */
