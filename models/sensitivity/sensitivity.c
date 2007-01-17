#include <math.h>

#include "sensitivity.h"
#include <packages/sensitivity.h>
#include <compiler/instquery.h>
#include <compiler/atomvalue.h>
#include <utilities/ascMalloc.h>


/**
	Build then presolve an instance
*/
slv_system_t asc_sens_presolve(struct Instance *inst){
  slv_system_t sys;
  slv_parameters_t parameters;
  struct var_variable **vp;
  struct rel_relation **rp;
  int ind,len;
  char *tmp=NULL;

  sys = system_build(inst);
  if (sys==NULL) {
	ERROR_REPORTER_HERE(ASC_PROG_ERR,
      "Something radically wrong in creating solver.");
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


int sensitivity_anal(
		     struct Instance *inst, /* not used but will be */
		     struct gl_list_t *arglist){
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
  sys = asc_sens_presolve(which_instance);
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
  scratch_vector = ASC_NEW_ARRAY_CLEAR(real64,capacity);
  linsolqr_add_rhs(lqr_sys,scratch_vector,FALSE);
  result = LUFactorJacobian1(sys,dof->structural_rank);
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

/**
	Finite-difference Check Arguments...?

	@param arglist list of arguments

	Argument list contains
	. arg1 - model inst to be solved
	. arg2 - array of input instances
	. arg3 - array of output instances
	. arg4 - matrix of partials to be written to
*/
static int FiniteDiffCheckArgs(struct gl_list_t *arglist)
{
  struct Instance *inst;
  unsigned long len;
  unsigned long ninputs, noutputs;

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



/*-------------------------------------------------
  SENSITIVITY ANALYSIS CODE
*/

/**
	Sensitivity Analysis

	@param arglist List of arguments

	Argument list contains the following:
	  . arg1 - model inst for which the sensitivity analysis is to be done.
	  . arg2 - array of input instances.
	  . arg3 - array of output instances.
	  . arg4 matrix of partials to be written to.
*/
int SensitivityCheckArgs(struct gl_list_t *arglist)
{
  struct Instance *inst;
  unsigned long len;
  unsigned long ninputs, noutputs;

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

  /*

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
	as we keep aroung a dense matrix n_outputs * n_inputs, but here
	n_outputs may be *much* larger depending on problem size.
*/
int sensitivity_anal_all( struct Instance *inst,  /* not used but will be */
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
  sys = asc_sens_presolve(which_instance);
  if (!sys) {
    FPRINTF(stderr,"Early termination due to failure in asc_sens_presolve\n");
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
  scratch_vector = ASC_NEW_ARRAY_CLEAR(real64,capacity);
  linsolqr_add_rhs(lqr_sys,scratch_vector,FALSE);
  result = LUFactorJacobian1(sys,dof->structural_rank);
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



/**
	Calls 'DoSolve'

	@see DoSolve
*/
int do_solve_eval( struct Instance *i,
		  struct gl_list_t *arglist, void *user_data)
{
  unsigned long len;
  int result;
  struct Instance *inst;
  len = gl_length(arglist);

  /* Ignore unused params */
  (void)i;

  if (len!=2) {
	ERROR_REPORTER_HERE(ASC_USER_ERROR,
      "Wrong number of args to do_solve_eval.");
    return 1;
  }
  inst = FetchElement(arglist,1,1);
  if (!inst)
    return 1;
  result = DoSolve(inst);
  return result;
}


/**
	Finite different evaluate...
*/
int do_finite_diff_eval( struct Instance *i,
			 struct gl_list_t *arglist, void *user_data)
{
  int result;

  /* Ignore unused params */
  (void)i;

  if (FiniteDiffCheckArgs(arglist))
    return 1;
  result = finite_difference(arglist);
  return result;
}


int do_sensitivity_eval( struct Instance *i,
			 struct gl_list_t *arglist, void *user_data)
{
  int result;
  if (SensitivityCheckArgs(arglist)) {
    return 1;
  }
  result = sensitivity_anal(i,arglist);
  return result;
}

int do_sensitivity_eval_all( struct Instance *i,
			    struct gl_list_t *arglist, void *user_data)
{
  int result;
  double step_length = 0.0;
  if (SensitivityAllCheckArgs(arglist,&step_length)) {
    return 1;
  }
  result = sensitivity_anal_all(i,arglist,step_length);
  return result;
}


char sensitivity_help[] =
	"This function does sensitivity analysis dy/dx. It requires 4 args:\n"
	"  1. name: name of a reference instance or SELF.\n"
	"  2. x: x, where x is an array of > solver_var.\n"
	"  3. y: where y is an array of > solver_var.\n"
	"  4. dy/dx: which dy_dx[1..n_y][1..n_x].";

int sensitivity_register(void){

  int result=0;


  result = CreateUserFunctionMethod("do_solve",
			      do_solve_eval,
			      2,NULL,NULL,NULL); /* was 2,0,null */
  result += CreateUserFunctionMethod("do_finite_difference",
			       do_finite_diff_eval,
			       4,NULL,NULL,NULL); /* 4,0,null */
  result += CreateUserFunctionMethod("do_sensitivity",
			       do_sensitivity_eval,
			       4,sensitivity_help,NULL,NULL);
  result += CreateUserFunctionMethod("do_sensitivity_all",
			       do_sensitivity_eval_all,
			       4,"See do_sensitivity_eval for details",NULL,NULL);

  return result;
}

