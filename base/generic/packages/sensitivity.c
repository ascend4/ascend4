/*********************************************************************\
  Sensititvity analysis code. Kirk Abbott.
\*********************************************************************/

#include <string.h>
#include <math.h>
#include "utilities/ascConfig.h"
#include "compiler/instance_enum.h"
#include "compiler/compiler.h"
#include "general/list.h"
#include "utilities/ascMalloc.h"
#include "compiler/extfunc.h"
#include "compiler/packages.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/atomvalue.h"
#include "compiler/instquery.h"
#include "solver/mtx.h"
#include "solver/mtx_basic.h"
#include "solver/mtx_perms.h"
#include "solver/mtx_query.h"
#include "solver/linsol.h"
#include "solver/linsolqr.h"
#include "solver/slv_types.h"
#include "solver/var.h"
#include "solver/rel.h"
#include "solver/discrete.h"
#include "solver/conditional.h"
#include "solver/logrel.h"
#include "solver/bnd.h"
#include "solver/calc.h"
#include "solver/relman.h"
#include "solver/slv_common.h"
#include "solver/slv_stdcalls.h"
#include "solver/system.h"
#include "solver/slv_client.h"

#define DEBUG 1

/*
 * This file attempts to implement the extraction of dy_dx from
 * a system of equations. If one considers a black-box where x are
 * the input variables, u are inuut parameters, y are the output
 * variables, f(x,y,u) is the system of equations that will be solved
 * for given x and u, then one can extract the sensitivity information
 * of y wrt x.
 * One crude but simple way of doing this is to finite-difference the
 * given black box, i.e, perturb x, n\subx times by delta x, resolve
 * the system of equations at the new value of x, and compute
 * dy/dx = (f(x\sub1) - f(x\sub2))/(x\sub1 - x\sub2).
 * This is known to be very expensive.
 *
 * The solution that will be adopted here is to formulate the Jacobian J of
 * the system, (or the system is already solved, to grab the jacobian at
 * the solution. Develop the sensitivity matrix df/dx by exact numnerical
 * differentiation; this will be a n x n\subx matrix. Compute the LU factors
 * for J, and to solve column by column to : LU dz/dx = df/dx. Here
 * z, represents all the internal variables to the system and the strictly
 * output variables y. In other words J = df/dz.
 *
 * Given the solution of the above problem we then simply extract the rows
 * of dz/dx, that pertain to the y variables that we are interested in;
 * this will give us dy/dx.
 */

#if 0
static real64 *zero_vector(real64 *vec, int size)
{
  int c;
  for (c=0;c<size;c++) {
    vec[c] = 0.0;
  }
  return vec;
}
#endif

static real64 **make_matrix(int nrows, int ncols)
{
  real64 **result;
  int i;
  result = (real64 **)calloc(nrows,sizeof(real64*));
  for (i=0;i<nrows;i++) {
    result[i] = (real64 *)calloc(ncols,sizeof(real64));
  }
  return result;
}

static void free_matrix(real64 **matrix, int nrows)
{
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

static struct Instance *FetchElement(struct gl_list_t *arglist,
				     unsigned long whichbranch,
				     unsigned long whichelement)
{
  struct gl_list_t *branch;
  struct Instance *element;

  if (!arglist) return NULL;
  branch = (struct gl_list_t *)gl_fetch(arglist,whichbranch);
  element = (struct Instance *)gl_fetch(branch,whichelement);
  return element;
}

static slv_system_t PreSolve(struct Instance *inst)
{
  slv_system_t sys;
  slv_parameters_t parameters;
  struct var_variable **vp;
  struct rel_relation **rp;
  int ind,len;
  char *tmp=NULL;

  sys = system_build(inst);
  if (sys==NULL) {
    FPRINTF(stdout,
      "(sensitivity.c): Something radically wrong in creating solver\n");
    return NULL;
  }
  if (g_SlvNumberOfRegisteredClients == 0) {
    return NULL;
  }
  ind = 0;
  while (strcmp(slv_solver_name(ind),"QRSlv")) {
    if (ind >= g_SlvNumberOfRegisteredClients) {
      FPRINTF(stderr,"(sensitivity.c): QRSlv must be registered client\n");
      return NULL;
    }
    ++ind;
  }
  slv_select_solver(sys,ind);

  slv_get_parameters(sys,&parameters);
  parameters.partition = 0;
  slv_set_parameters(sys,&parameters);
  slv_presolve(sys);

#if DEBUG
  vp = slv_get_solvers_var_list(sys);
  len = slv_get_num_solvers_vars(sys);
  for (ind=0 ; ind<len; ind++) {
    tmp = var_make_name(sys,vp[ind]);
    FPRINTF(stderr,"%s  %d\n",tmp,var_sindex(vp[ind]));
    if (tmp!=NULL) ascfree(tmp);
  }
  rp = slv_get_solvers_rel_list(sys);
  len = slv_get_num_solvers_rels(sys);
  for (ind=0 ; ind<len ; ind++) {
    tmp = rel_make_name(sys,rp[ind]);
    FPRINTF(stderr,"%s  %d\n",tmp,rel_sindex(rp[ind]));
    if (tmp) ascfree(tmp);
  }
#endif
  return sys;
}

#if 0
static int ReSolve(slv_system_t sys)
{
  if (!sys)
    return 1;
  slv_solve(sys);
  return 0;
}
#endif 

static int DoSolve(struct Instance *inst)
{
  slv_system_t sys;

  sys = system_build(inst);
  if (!sys) {
    FPRINTF(stdout,"Something radically wrong in creating solver\n");
    return 1;
  }
  (void)slv_select_solver(sys,0);
  slv_presolve(sys);
  slv_solve(sys);
  system_destroy(sys);
  return 0;
}

int do_solve_eval(struct Slv_Interp *slv_interp,
		  struct Instance *i,
		  struct gl_list_t *arglist,
		  unsigned long whichvar)
{
  unsigned long len;
  int result;
  struct Instance *inst;
  len = gl_length(arglist);

  /* Ignore unused params */
  (void)slv_interp; (void)i; (void)whichvar;

  if (len!=2) {
    FPRINTF(stdout,"Wrong number of args to do_solve_eval\n");
    return 1;
  }
  inst = FetchElement(arglist,1,1);
  if (!inst)
    return 1;
  result = DoSolve(inst);
  return result;
}

static int FiniteDiffCheckArgs(struct gl_list_t *arglist)
{
  struct Instance *inst;
  unsigned long len;
  unsigned long ninputs, noutputs;

  /*
   * arg1 - model inst to be solved.
   * arg2 - array of input instances.
   * arg3 - array of output instances.
   * matrix of partials to be written to.
   */
  len = gl_length(arglist);
  if (len != 4) {
    FPRINTF(stderr,"wrong number of args -- 4 expected\n");
    return 1;
  }
  inst = FetchElement(arglist,1,1);
  if (InstanceKind(inst)!=MODEL_INST) {
    FPRINTF(stderr,"Arg #1 is not a model instance\n");
    return 1;
  }
  ninputs = gl_length((struct gl_list_t *)gl_fetch(arglist,2));
    /* input args */
  noutputs = gl_length((struct gl_list_t *)gl_fetch(arglist,3));
    /* output args */
  len = gl_length((struct gl_list_t *)gl_fetch(arglist,4));
    /* partials matrix */
  if (len != (ninputs*noutputs)) {
    FPRINTF(stderr,
	    "The array of partials is inconsistent with the args given\n");
    return 1;
  }
  return 0;
}

static int finite_difference(struct gl_list_t *arglist)
{
  struct Instance *model_inst, *xinst, *inst;
  slv_system_t sys;
  int ninputs,noutputs;
  int i,j,offset;
  real64 **partials;
  real64 *y_old, *y_new;
  real64 x;
  real64 interval = 1e-6;
  int result=0;

  model_inst = FetchElement(arglist,1,1);
  sys = system_build(model_inst);
  if (!sys) {
    FPRINTF(stdout,"Something radically wrong in creating solver\n");
    return 1;
  }
  (void)slv_select_solver(sys,0);
  slv_presolve(sys);
  slv_solve(sys);
  /*
   * Make the necessary vectors.
   */
  ninputs = (int)gl_length((struct gl_list_t *)gl_fetch(arglist,2));
  noutputs = (int)gl_length((struct gl_list_t *)gl_fetch(arglist,3));
  y_old = (real64 *)calloc(noutputs,sizeof(real64));
  y_new = (real64 *)calloc(noutputs,sizeof(real64));
  partials = make_matrix(noutputs,ninputs);
  for (i=0;i<noutputs;i++) {      	/* get old yvalues */
    inst = FetchElement(arglist,3,i+1);
    y_old[i] = RealAtomValue(inst);
  }
  for (j=0;j<ninputs;j++) {
    xinst = FetchElement(arglist,2,j+1);
    x = RealAtomValue(xinst);
    SetRealAtomValue(xinst,x+interval,(unsigned)0); /* perturb system */
    slv_presolve(sys);
    slv_solve(sys);

    for (i=0;i<noutputs;i++) { 		/* get new yvalues */
      inst = FetchElement(arglist,3,i+1);
      y_new[i] = RealAtomValue(inst);
      partials[i][j] = -1.0 * (y_old[i] - y_new[i])/interval;
      PRINTF("y_old = %20.12g  y_new = %20.12g\n", y_old[i],y_new[i]);
    }
    SetRealAtomValue(xinst,x,(unsigned)0); /* unperturb system */
  }
  offset = 0;
  for (i=0;i<noutputs;i++) {
    for (j=0;j<ninputs;j++) {
      inst = FetchElement(arglist,4,offset+j+1);
      SetRealAtomValue(inst,partials[i][j],(unsigned)0);
      PRINTF("%12.6f %s",partials[i][j], (j==(ninputs-1)) ? "\n" : "    ");
    }
    offset += ninputs;
  }
  /* error: */
  free(y_old);
  free(y_new);
  free_matrix(partials,noutputs);
  system_destroy(sys);
  return result;
}

int do_finite_diff_eval(struct Slv_Interp *slv_interp,
			 struct Instance *i,
			 struct gl_list_t *arglist,
			 unsigned long whichvar)
{
  int result;

  /* Ignore unused params */
  (void)slv_interp; (void)i; (void)whichvar;

  if (FiniteDiffCheckArgs(arglist))
    return 1;
  result = finite_difference(arglist);
  return result;
}

/*********************************************************************\
  Sensititvity analysis code.
\*********************************************************************/

int SensitivityCheckArgs(struct gl_list_t *arglist)
{
  struct Instance *inst;
  unsigned long len;
  unsigned long ninputs, noutputs;

  /*
   * arg1 - model inst for which the sensitivity analysis is to be done.
   * arg2 - array of input instances.
   * arg3 - array of output instances.
   * arg4 matrix of partials to be written to.
   */

  len = gl_length(arglist);
  if (len != 4) {
    FPRINTF(stderr,"wrong number of args -- 4 expected\n");
    return 1;
  }
  inst = FetchElement(arglist,1,1);
  if (InstanceKind(inst)!=MODEL_INST) {
    FPRINTF(stderr,"Arg #1 is not a model instance\n");
    return 1;
  }
  ninputs = gl_length((struct gl_list_t *)gl_fetch(arglist,2));
    /* input args */
  noutputs = gl_length((struct gl_list_t *)gl_fetch(arglist,3));
   /* output args */
  len = gl_length((struct gl_list_t *)gl_fetch(arglist,4));
        /* partials matrix */
  if (len != (ninputs*noutputs)) {
    FPRINTF(stderr,
	    "The array of partials is inconsistent with the args given\n");
    return 1;
  }
  return 0;
}

int SensitivityAllCheckArgs(struct gl_list_t *arglist, double *step_length)
{
  struct Instance *inst;
  unsigned long len;

  /*
   * arg1 - model inst for which the sensitivity analysis is to be done.
   * arg2 - array of input instances.
   * arg3 - new_input instances, for variable projection.
   * arg4 - instance representing the step_length for projection.
   * The result will be written to standard out.
   */

  len = gl_length(arglist);
  if (len != 4) {
    FPRINTF(stderr,"wrong number of args -- 4 expected\n");
    return 1;
  }
  inst = FetchElement(arglist,1,1);
  if (InstanceKind(inst)!=MODEL_INST) {
    FPRINTF(stderr,"Arg #1 is not a model instance\n");
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


static int NumberRels(slv_system_t sys)
{
  static int nrels = -1;
  rel_filter_t rfilter;
  int tmp;

  if (!sys) { /* a NULL system may be used to reinitialise the count */
    nrels = -1;
    return -1;
  }
  if (nrels < 0) {
    /*get used (included) relation count -- equalities only !*/
    rfilter.matchbits = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
    rfilter.matchvalue = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
    tmp = slv_count_solvers_rels(sys,&rfilter);
    nrels = tmp;
    return tmp;
  }
  else return nrels;
}

static int NumberFreeVars(slv_system_t sys)
{
  static int nvars = -1;
  var_filter_t vfilter;
  int tmp;

  if (!sys) {
    nvars = -1;
    return -1;
  }
  if (nvars < 0) {
    /*get used (free and incident) variable count */
    vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT  | VAR_SVAR | VAR_ACTIVE);
    vfilter.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
    tmp = slv_count_solvers_vars(sys,&vfilter);
    nvars = tmp;
    return tmp;
  }
  else return nvars;
}


/***************************************************************/
/***************************************************************/
/*
 * The following bit of code does the computation of the matrix
 * dz/dx. It accepts a slv_system_t and a list of 'input' variables.
 * The matrix df/dx now exists and sits to the right of the main
 * square region of the Jacobian. We will do the following in turn
 * for each variable in this list:
 *
 * 1) 	Get the variable index, and use it to extract the required column
 *    	from the main gradient matrix, this will be stored in a temporary
 *    	vector.
 * 2) 	We will then clear the column in the original matrix, as we want to
 *	store the caluclated results back in place.
 * 3)	Add the vector extracted in 1) as rhs to the main matrix.
 * 4)	Call linsol solve on this rhs to yield a solution which represents
 * 	a column of dx/dx.
 * 6)	Store this vector back in the location cleared out in 2).
 * 7)	Goto 1.
 *
 * At the end of this procedure we would have calculated all the columns of
 * dz/dx, and left them back in the main matrix.
 */

/*******************************************************************/
/*******************************************************************/

/*
 * At this point we should have an empty jacobian. We now
 * need to call relman_diff over the *entire* matrix.
 * fixed and free.
 *
 *  Calculates the entire jacobian.
 *  It is initially unscaled.
 */
static int Compute_J(slv_system_t sys)
{
  int32 row;
  var_filter_t vfilter;
  linsolqr_system_t lqr_sys;
  mtx_matrix_t mtx;
  struct rel_relation **rlist;
  int nrows;
  real64 resid;

  /*
   * Get the linear system from the solve system.
   * Get the matrix from the linear system.
   * Get the relations list from the solve system.
   */
  lqr_sys = slv_get_linsolqr_sys(sys);
  mtx = linsolqr_get_matrix(lqr_sys);
  rlist = slv_get_solvers_rel_list(sys);
  nrows = NumberRels(sys);

  calc_ok = TRUE;
  vfilter.matchbits =  (VAR_SVAR | VAR_ACTIVE) ;
  vfilter.matchvalue =  vfilter.matchbits ;

  /*
   * Clear the entire matrix and then compute
   * the gradients at the current point.
   */
  mtx_clear_region(mtx,mtx_ENTIRE_MATRIX);
  for(row=0; row<nrows; row++) {
    struct rel_relation *rel;
    rel = rlist[mtx_row_to_org(mtx,row)];
    (void)relman_diffs(rel,&vfilter,mtx,&resid,1);
  }
  linsolqr_matrix_was_changed(lqr_sys);

  return(!calc_ok);
}

/*
 * Note a rhs would have been previously added
 * to keep the system happy.
 */

static int LUFactorJacobian(slv_system_t sys,int rank)
{
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


/*
 * At this point the rhs would have already been added.
 *
 */


static int Compute_dy_dx_smart(slv_system_t sys,
			       real64 *rhs,
			       real64 **dy_dx,
			       int *inputs, int ninputs,
			       int *outputs, int noutputs)
{
  linsolqr_system_t lqr_sys;
  mtx_matrix_t mtx;
  int col,current_col;
  int row;
  int capacity;
  real64 *solution = NULL;
  int i,j,k;

  lqr_sys = slv_get_linsolqr_sys(sys); /* get the linear system */
  mtx = linsolqr_get_matrix(lqr_sys); /* get the matrix */

  capacity = mtx_capacity(mtx);
  solution = (real64 *)asccalloc(capacity,sizeof(real64));

  /*
   * The array inputs is a list of original indexes, of the variables
   * that we are trying to obtain the sensitivity to. We have to
   * get the *current* column from the matrix based on those indices.
   * Hence we use mtx_org_to_col. This little manipulation is not
   * necessary for the computed solution as the solve routine returns
   * the results in the *original* order rather than the *current* order.
   */
  for (j=0;j<ninputs;j++) {
    col = inputs[j];
    current_col = mtx_org_to_col(mtx,col);
    mtx_org_col_vec(mtx,current_col,rhs,mtx_ALL_ROWS);
    /* get the column in org row indexed order */

    linsolqr_rhs_was_changed(lqr_sys,rhs);
    linsolqr_solve(lqr_sys,rhs);			/* solve */
    linsolqr_copy_solution(lqr_sys,rhs,solution);	/* get the solution */

#if DEBUG
    FPRINTF(stderr,"This is the rhs and solution for rhs #%d orgcol%d\n",j,col);
    for (k=0;k<capacity;k++) {
      FPRINTF(stderr,"%12.8g  %12.8g\n",rhs[k],solution[k]);
    }
#endif

    for (i=0;i<noutputs;i++) {	/* copy the solution to dy_dx */
      row = outputs[i];
      dy_dx[i][j] = -1.0*solution[row]; /* the -1 comes from the lin algebra */
    }
    /*
     * zero the vector using the incidence pattern of our column.
     * This is faster than the naiive approach of zeroing
     * everything especially if the vector is large.
     */
    mtx_zr_org_vec_using_col(mtx,current_col,rhs,mtx_ALL_ROWS);
  }

  if (solution) ascfree((char *)solution);
  return 0;
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
  solution = (real64 *)asccalloc(capacity,sizeof(real64));

  order = mtx_order(mtx);
  for (j=0;j<order;j++) {
    rhs[j] = 1.0;
    linsolqr_rhs_was_changed(lqr_sys,rhs);
    linsolqr_solve(lqr_sys,rhs);			/* solve */
    linsolqr_copy_solution(lqr_sys,rhs,solution);	/* get the solution */

    FPRINTF(stderr,"This is the rhs and solution for rhs #%d\n",j);
    for (k=0;k<order;k++) {
      FPRINTF(stderr,"%12.8g  %12.8g\n",rhs[k],solution[k]);
    }
    rhs[j] = 0.0;
  }
  if (solution) ascfree((char *)solution);
  return 0;
}
#endif

int sensitivity_anal(struct Slv_Interp *slv_interp,
		     struct Instance *inst, /* not used but will be */
		     struct gl_list_t *arglist)
{
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
  (void)slv_interp; (void) inst;

  (void)NumberFreeVars(NULL);		/* used to re-init the system */
  (void)NumberRels(NULL);		/* used to re-init the system */
  which_instance = FetchElement(arglist,1,1);
  sys = PreSolve(which_instance);
  if (!sys) {
    FPRINTF(stderr,"Early termination due to failure in Presolve\n");
    result = 1;
    goto error;
  }

  dof = slv_get_dofdata(sys);
  if (!(dof->n_rows == dof->n_cols &&
	dof->n_rows == dof->structural_rank)) {
    FPRINTF(stderr,"Early termination: non square system\n");
    result = 1;
    goto error;
  }
  /*
   * prepare the inputs list
   */
  vlist = slv_get_solvers_var_list(sys);

  branch = gl_fetch(arglist,2); /* input args */
  ninputs = gl_length(branch);
  inputs_ndx_list = (int *)ascmalloc(ninputs*sizeof(int));

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
    if (!found) {
      FPRINTF(stderr,"Sensitivity input variable not found\n");
      result = 1;
      goto error;
    }
  }

  /*
   * prepare the outputs list
   */
  branch = gl_fetch(arglist,3); /* output args */
  noutputs = gl_length(branch);
  outputs_ndx_list = (int *)ascmalloc(noutputs*sizeof(int));
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
      FPRINTF(stderr,"Sensitivity ouput variable not found\n");
      result = 1;
      goto error;
    }
  }

  /*
   * prepare the results dy_dx.
   */
  dy_dx = make_matrix(noutputs,ninputs);


  result = Compute_J(sys);
  if (result) {
    FPRINTF(stderr,"Early termination due to failure in calc Jacobian\n");
    goto error;
  }

  /*
   * Note: the RHS *has* to added here. We will construct the vector
   * of size matrix capacity and add it. It will be removed after
   * we finish computing dy_dx.
   */
  lqr_sys = slv_get_linsolqr_sys(sys);	/* get the linear system */
  mtx = linsolqr_get_matrix(lqr_sys);	/* get the matrix */
  capacity = mtx_capacity(mtx);
  scratch_vector = (real64 *)asccalloc(capacity,sizeof(real64));
  linsolqr_add_rhs(lqr_sys,scratch_vector,FALSE);
  result = LUFactorJacobian(sys,dof->structural_rank);
  if (result) {
    FPRINTF(stderr,"Early termination due to failure in LUFactorJacobian\n");
    goto error;
  }
  result = Compute_dy_dx_smart(sys, scratch_vector, dy_dx,
			       inputs_ndx_list, ninputs,
			       outputs_ndx_list, noutputs);

  linsolqr_remove_rhs(lqr_sys,scratch_vector);
  if (result) {
    FPRINTF(stderr,"Early termination due to failure in Compute_dy_dx\n");
    goto error;
  }

  /*
   * Write the results back to the partials array in the
   * instance tree
   */
  offset = 0;
  for (i=0;i<noutputs;i++) {
    for (j=0;j<ninputs;j++) {
      tmp_inst = FetchElement(arglist,4,offset+j+1);
      SetRealAtomValue(tmp_inst,dy_dx[i][j],(unsigned)0);
#if DEBUG
      FPRINTF(stderr,"%12.8f   i%d j%d",dy_dx[i][j],i,j);
#endif
    }
#if DEBUG
    FPRINTF(stderr,"\n");
#endif
    offset += ninputs;
  }
#if DEBUG
  FPRINTF(stderr,"\n");
#endif

error:
  if (inputs_ndx_list) ascfree((char *)inputs_ndx_list);
  if (outputs_ndx_list) ascfree((char *)outputs_ndx_list);
  if (dy_dx) free_matrix(dy_dx,noutputs);
  if (scratch_vector) ascfree((char *)scratch_vector);
  if (sys) system_destroy(sys);
  return result;
}


static int DoDataAnalysis(struct var_variable **inputs,
			  struct var_variable **outputs,
			  int ninputs, int noutputs,
			  real64 **dy_dx)
{
  FILE *fp;
  double *norm_2, *norm_1;
  double input_nominal,maxvalue,sum;
  int i,j;

  norm_1 = (double *)asccalloc(ninputs,sizeof(double));
  norm_2 = (double *)asccalloc(ninputs,sizeof(double));

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
    FPRINTF(fp,"%8d    ",var_mindex(inputs[j]));
  }
  FPRINTF(fp,"\n");

  for (j=0;j<ninputs;j++) {		/* print the 1 norms */
    FPRINTF(fp,"%-#18.8f    ",norm_1[j]);
  }
  FPRINTF(fp,"\n");

  for (j=0;j<ninputs;j++) {		/* print the 2 norms */
    FPRINTF(fp,"%-#18.8f    ",norm_2[j]);
  }
  FPRINTF(fp,"\n\n");
  ascfree((char *)norm_1);
  ascfree((char *)norm_2);

  for (i=0;i<noutputs;i++) {		/* print the scaled data */
    for (j=0;j<ninputs;j++) {
      FPRINTF(fp,"%-#18.8f   %-4d",dy_dx[i][j],i);
    }
    if (var_fixed(outputs[i]))
      FPRINTF(fp,"    **fixed*** \n");
    else
      PUTC('\n',fp);
  }
  FPRINTF(fp,"\n");
  fclose(fp);
  return 0;
}

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

  delta_x = (real64 *)asccalloc(ninputs,sizeof(real64));

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
# if  DEBUG
    FPRINTF(stderr,"Old_y = %12.8g; Nex_y = %12.8g\n",old_y,new_y);
# endif
  }
  ascfree((char *)delta_x);
  return 0;
}
#endif


/*
 * This function is very similar to sensitivity_anal, execept,
 * that it perform the analysis on the entire system, based on
 * the given inputs. Nothing is returned. As such the call from
 * ASCEND is :
 *
 *	EXTERN sensitivity_anal_all(
 *		this_instance,
 *		u_old[1..n_inputs],
 *		u_new[1..n_inputs],
 *		step_length
 *	);
 *
 * The results will be witten to standard out.
 * This function is more expensive from a a memory point of view,
 * as we keep aroung a dense matrix n_outputs * n_inputs, but here
 * n_outputs may be *much* larger depending on problem size.
 */

int sensitivity_anal_all(struct Slv_Interp *slv_interp,
			 struct Instance *inst,  /* not used but will be */
			 struct gl_list_t *arglist,
			 real64 step_length)
{
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
  (void)slv_interp; (void)inst; (void)step_length;

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
  sys = PreSolve(which_instance);
  if (!sys) {
    FPRINTF(stderr,"Early termination due to failure in Presolve\n");
    result = 1;
    goto error;
  }
  dof = slv_get_dofdata(sys);

  /*
   * prepare the inputs list. We dont need the index of the new_inputs
   * list. We will grab them later if necessary.
   */
  branch2 = gl_fetch(arglist,2); /* input args -- old u_values */
  branch3 = gl_fetch(arglist,3); /* input args -- new u_values */
  ninputs = gl_length(branch2);
  inputs = (struct var_variable **)ascmalloc((ninputs+1)*sizeof(struct var_variable *));
  new_inputs = (struct var_variable **)ascmalloc((ninputs+1)*sizeof(struct var_variable *));

  inputs_ndx_list = (int *)ascmalloc(ninputs*sizeof(int));
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
  outputs = (struct var_variable **)ascmalloc((noutputs+1)*sizeof(struct var_variable *));
  outputs_ndx_list = (int *)ascmalloc(noutputs*sizeof(int));
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
    FPRINTF(stderr,"Early termination due to failure in calc Jacobian\n");
    goto error;
  }

  /*
   * Note: the RHS *has* to added here. We will construct the vector
   * of size matrix capacity and add it. It will be removed after
   * we finish computing dy_dx.
   */
  lqr_sys = slv_get_linsolqr_sys(sys);	/* get the linear system */
  mtx = linsolqr_get_matrix(lqr_sys);	/* get the matrix */
  capacity = mtx_capacity(mtx);
  scratch_vector = (real64 *)asccalloc(capacity,sizeof(real64));
  linsolqr_add_rhs(lqr_sys,scratch_vector,FALSE);
  result = LUFactorJacobian(sys,dof->structural_rank);
  if (result) {
    FPRINTF(stderr,"Early termination due to failure in LUFactorJacobian\n");
    goto error;
  }
  result = Compute_dy_dx_smart(sys, scratch_vector, dy_dx,
			       inputs_ndx_list, ninputs,
			       outputs_ndx_list, noutputs);

  linsolqr_remove_rhs(lqr_sys,scratch_vector);
  if (result) {
    FPRINTF(stderr,"Early termination due to failure in Compute_dy_dx\n");
    goto error;
  }

  /*
   * Do some analysis on the results, inclusive of
   * writing them to someplace useful.
   */
  if (DoDataAnalysis(inputs, outputs, ninputs, noutputs, dy_dx))
    result = 1;

  /*
   * Some experimental projection stuff -- not used now.
   * if (DoProject_X(inputs, new_inputs, step_length,
   *     outputs, ninputs, noutputs, dy_dx))
   * result = 1;
   */

 error:
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


int do_sensitivity_eval(struct Slv_Interp *slv_interp,
			 struct Instance *i,
			 struct gl_list_t *arglist)
{
  int result;
  if (SensitivityCheckArgs(arglist)) {
    return 1;
  }
  result = sensitivity_anal(slv_interp,i,arglist);
  return result;
}

int do_sensitivity_eval_all(struct Slv_Interp *slv_interp,
			    struct Instance *i,
			    struct gl_list_t *arglist)
{
  int result;
  double step_length = 0.0;
  if (SensitivityAllCheckArgs(arglist,&step_length)) {
    return 1;
  }
  result = sensitivity_anal_all(slv_interp,i,arglist,step_length);
  return result;
}

char sensitivity_help[] =
"This function does sensitivity analysis dy/dx. It requires 4 args.\n\
The first arg is the name of a reference instance or SELF.\n\
The second arg is x, where x is an array of > solver_var\n.\
The third arg y, where y is an array of > solver_var\n. \
The fourth arg is dy/dx which dy_dx[1..n_y][1..n_x].\n";

#undef DEBUG
