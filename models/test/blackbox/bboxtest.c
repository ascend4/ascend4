/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University

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
*//**
	@file
	black box semantics test.
*//*
	by Ben Allan
	Created: July 4, 2006
	Version: $Revision: 1.5 $
	Date last modified: $Date: 1997/07/18 12:20:07 $
*/

#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <compiler/compiler.h>
#include <compiler/packages.h>
#include <compiler/instance_enum.h>
#include <utilities/ascPanic.h>
/* next 4 needed only because we use RealAtomValue on the DATA instance. */
#include <compiler/fractions.h>
#include <compiler/dimen.h>
#include <compiler/atomvalue.h>
#include <compiler/instquery.h>

#include <compiler/extcall.h>

/* #define BBOXTEST_DEBUG */

ExtBBoxInitFunc bboxtest_preslv;
ExtBBoxFunc bboxtest_fex;
ExtBBoxFunc bboxtest_jex;
ExtBBoxFinalFunc bboxtest_final;
#define N_INPUT_ARGS 1 /* formal arg count */
#define N_OUTPUT_ARGS 1 /* formal arg count */

extern 
ASC_EXPORT int bboxtest_register(void){
	double epsilon = 1.0e-14;

	char bboxtest_help[] = "This tests a simple black box y=k*x."
		" The value 'k' is provided to the blackbox as a data argument.";

	return CreateUserFunctionBlackBox("bboxtest"
		,&bboxtest_preslv
		,&bboxtest_fex, &bboxtest_jex
		,NULL, &bboxtest_final
		,N_INPUT_ARGS, N_OUTPUT_ARGS
		,bboxtest_help
		,epsilon
	);
}

/*------------------------------------------------------------------------------
  forward decls
*/

struct BBOXTEST_problem {
	double coef; /* coef in y=coef*x*/
	int n; /* number of equations. */
};

static int GetCoef( struct Instance *data, struct BBOXTEST_problem *problem);

static int CheckArgsOK(struct Instance *data
	,struct gl_list_t *arglist, struct BBOXTEST_problem *problem
);

static int DoCalculation(struct BBoxInterp *interp
	,int ninputs, int noutputs
	,double *inputs, double *outputs
);

int DoDeriv(struct BBoxInterp *interp, int ninputs, double *jacobian);

#ifndef EXTERNAL_EPSILON
#define EXTERNAL_EPSILON 1.0e-12
#endif

/*----------------------------------------------------------------------------*/

/**
	This function is one of the registered functions. It operates in
	mode 'last_call'.

	In 'last_call' mode, the memory associated with the problem is
	released.
*/
void bboxtest_final(struct BBoxInterp *interp){
	struct BBOXTEST_problem *problem;

	if(interp->task != bb_last_call){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unexpected call to last_call fn");
		return;
	}

	if(interp->user_data != NULL) {
		problem = (struct BBOXTEST_problem *)interp->user_data;
		problem->coef *= -1;
		problem->n *= -1;
		ASC_FREE(problem);
		interp->user_data = NULL;
	}
}
/**
	This function is one of the registered functions. It operates in
	 mode first_call.
	It creates a BBOXTEST_problem and calls a number of routines to check
	the arguments (data and arglist) and to cache the information
	processed away in the BBOXTEST_problem structure.

	In last_call mode, the memory associated with the problem is
	released.

	@return 0 on success
*/
int bboxtest_preslv(struct BBoxInterp *interp,
			struct Instance *data,
			struct gl_list_t *arglist

){
	struct BBOXTEST_problem *problem;

	if(interp->task != bb_first_call){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unexpected call to first_call fn");
		return -1;
	}

	if(interp->user_data!=NULL){
#ifdef BBOXTEST_DEBUG
		CONSOLE_DEBUG("user_data has already been allocated, no need to reallocate");
#endif
		return 0;
	}

	problem = ASC_NEW(struct BBOXTEST_problem);
	if(CheckArgsOK(data,arglist,problem)){
		CONSOLE_DEBUG("Problem with arguments");
		ASC_FREE(problem);
		return -2;
	}

	/* store the BBTEST_problem in the user_data pointer */
	interp->user_data = (void *)problem;
	return 0; /* success */
}

/*----------------------------------------------------------------------------*/

/** 
	Evaluate residuals

	@return 0 on success 
*/
int bboxtest_fex(struct BBoxInterp *interp,
		int ninputs,
		int noutputs,
		double *inputs,
		double *outputs,
		double *jacobian
){
	(void)jacobian;
	return DoCalculation(interp, ninputs, noutputs, inputs, outputs);
}

/*
	Evaluate jacobian
	@return 0 on success
*/
int bboxtest_jex(struct BBoxInterp *interp,
		int ninputs,
		int noutputs,
		double *inputs,
		double *outputs,
		double *jacobian
){
	(void)noutputs; 
	(void)outputs;
	(void)inputs;
	
	return DoDeriv(interp, ninputs, jacobian);
}

/*------------------------------------------------------------------------------
  utility routines
*/

static int GetCoef( struct Instance *data, struct BBOXTEST_problem *problem){

	if(!data){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"expecting a data instance to be provided");
		return 5;
	}
	if(InstanceKind(data)!=REAL_CONSTANT_INST) {
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"expecting a real constant instance.");
		return 6;
	}

	problem->coef = RealAtomValue(data);
	return 0;
}

static int CheckArgsOK(struct Instance *data,
			struct gl_list_t *arglist,
			struct BBOXTEST_problem *problem
){
	unsigned long len,ninputs,noutputs;

	if (!arglist) {
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"External function argument list does not exist.");
		return 1;
	}
	len = gl_length(arglist);
	if (!len) {
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"No arguments to external function statement.");
		return 2;
	}
	if ((len!=(N_INPUT_ARGS+N_OUTPUT_ARGS))) {
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Number of arguments does not match"
			" the external function"
			" prototype(array_of_realatom[set],array_of_realatom[set],real_constant"
		);
		return 3;
	}

	ninputs = CountNumberOfArgs(arglist,1,N_INPUT_ARGS);
	noutputs = CountNumberOfArgs(arglist,N_INPUT_ARGS+1,
					N_INPUT_ARGS+N_OUTPUT_ARGS);
	if (ninputs != noutputs) {
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Length of input, output arguments mismatched.");
		return 4;
	}

	problem->n = (int)ninputs;

	return GetCoef(data,problem); /* get the coef, return 0 on success means all was ok */
}

/*
	This function provides support to bboxtest_fex which is one of the
	registered functions. The input variables are x[set]
	The output variables are y[set]. We do our loop
	based on the ascend standard that sets are arbitrarily but
	consistently ordered if they contain the same values.
*/

static int DoCalculation(struct BBoxInterp *interp,
			 int ninputs, int noutputs,
			 double *inputs,
			 double *outputs
){
	struct BBOXTEST_problem *problem;
	int c;
	double coef;

	if(ninputs != noutputs){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"ninputs != noutputs");
		return -1;
	}
	if(interp->user_data == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"user_data not supplied");
		return -2;
	}
	problem = (struct BBOXTEST_problem *)interp->user_data;
	coef = problem->coef;

	for (c=0; c < ninputs; c++) {
		outputs[c] = coef * inputs[c];
	}

#ifdef BBOXTEST_DEBUG
	CONSOLE_DEBUG("instance = %p",interp->user_data);
	for(c=0;c<ninputs;c++) {
		CONSOLE_DEBUG("x[%d]	= %12.8g",c,inputs[c]);
	}
	for (c=0;c<noutputs;c++) {
		CONSOLE_DEBUG("y[%d]	= %20.8g",c,outputs[c]);
	}
#endif /* BBOXTEST_DEBUG */

	interp->status = calc_all_ok;
	return 0;
}

int DoDeriv(struct BBoxInterp *interp, int ninputs, double *jacobian){
	int i; int len;
	double coef;
	if(interp==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"interp==NULL");
		return -1;
	}
	if(interp->user_data==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"interp->user_data==NULL");
		return -2;
	}
	coef = ((struct BBOXTEST_problem *)interp->user_data)->coef;
	len = ninputs*ninputs;

#ifdef BBOXTEST_DEBUG
	CONSOLE_DEBUG("instance = %p",interp->user_data);
#endif
	for (i = 0; i< len; i++) {
		jacobian[i] = 0;
	}
	
	for (i = 0; i< ninputs; i++) {
		jacobian[i*ninputs+i] = coef;
	}
#ifdef BBOXTEST_DEBUG
	for(i=0; i<len; i++) {
		CONSOLE_DEBUG("J[%d]	= %12.8g", i, jacobian[i]);
	}
#endif
	return 0;
}
