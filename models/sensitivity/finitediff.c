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

	Finite difference computation as external ASCEND METHOD.
	No examples available.

	Sliced out of sensitivity.c because it's independent code.

	@TODO document this.
*/

#include <math.h>

#include <packages/sensitivity.h>
#include <compiler/instquery.h>
#include <compiler/atomvalue.h>
#include <utilities/ascMalloc.h>
#include <compiler/extfunc.h>
#include <general/mathmacros.h>
#include <solver/densemtx.h>

ExtMethodRun do_finite_diff_eval;

ASC_EXPORT int finitediff_register(void);


/**
	Finite difference...
*/
int finite_difference(struct gl_list_t *arglist){
  struct Instance *model_inst, *xinst, *inst;
  slv_system_t sys;
  int ninputs,noutputs;
  int i,j,offset;
  DenseMatrix partials;
  real64 *y_old, *y_new;
  real64 x;
  real64 interval = 1e-6;
  int result=0;

  model_inst = FetchElement(arglist,1,1);
  sys = system_build(model_inst);
  if (!sys) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to build system.");
    return 1;
  }
  (void)slv_select_solver(sys,0);
  slv_presolve(sys);
  slv_solve(sys);

  /* Make the necessary vectors */

  ninputs = (int)gl_length((struct gl_list_t *)gl_fetch(arglist,2));
  noutputs = (int)gl_length((struct gl_list_t *)gl_fetch(arglist,3));
  y_old = (real64 *)calloc(noutputs,sizeof(real64));
  y_new = (real64 *)calloc(noutputs,sizeof(real64));
  partials = densematrix_create(noutputs,ninputs);
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
      DENSEMATRIX_ELEM(partials,i,j) = -1.0 * (y_old[i] - y_new[i])/interval;
      PRINTF("y_old = %20.12g  y_new = %20.12g\n", y_old[i],y_new[i]);
    }
    SetRealAtomValue(xinst,x,(unsigned)0); /* unperturb system */
  }
  offset = 0;
  for (i=0;i<noutputs;i++) {
    for (j=0;j<ninputs;j++) {
      inst = FetchElement(arglist,4,offset+j+1);
      SetRealAtomValue(inst,DENSEMATRIX_ELEM(partials,i,j),(unsigned)0);
      PRINTF("%12.6f %s",DENSEMATRIX_ELEM(partials,i,j), (j==(ninputs-1)) ? "\n" : "    ");
    }
    offset += ninputs;
  }
  /* error: */
  free(y_old);
  free(y_new);
  densematrix_destroy(partials);
  system_destroy(sys);
  return result;
}



/**
	Finite-difference Check Arguments

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
	    "The array of partials is inconsistent with the args given."
	);
    return 1;
  }
  return 0;
}


/**
	Finite different evaluate...
*/
int do_finite_diff_eval( struct Instance *i,
		struct gl_list_t *arglist, void *user_data
){
	int result;
	(void)i; /* not used */

	if(FiniteDiffCheckArgs(arglist))
		return 1;
	result = finite_difference(arglist);
	return result;
}

/** Registration function */
int finitediff_register(void){
	int result=0;
	result += CreateUserFunctionMethod("do_finite_difference",
		do_finite_diff_eval,
		4,NULL,NULL,NULL
	);
	return result;
}
