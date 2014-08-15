/*	ASCEND modelling environment
	Copyright (C) 2008 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @file
	Wrapper for plnkin.c to allow access to the calculation from ASCEND.
*/

/* include the external function API from libascend... */
#include <ascend/compiler/extfunc.h>

/* include error reporting API as well, so we can send messages to user */
#include <ascend/utilities/error.h>

/* the code that we're wrapping... */
#include "plnkin.h"

static ExtBBoxInitFunc planck_prepare;
static ExtBBoxFunc planck_calc;

static const char *planck_help = "\
Calculate the normalised Planck integral. Inputs are wavelength and \
absolute tempearture. Result is the fraction of the black body radiation energy \
at temperature T that is present in the wavelength range 0--lambda. See \
http://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/19680008986.pdf \
for details.";

/*------------------------------------------------------------------------------
  REGISTRATION FUNCTION
*/

/**
	This is the function called from 'IMPORT "johnpye/planck/planck";'

	It sets up the functions contained in this external library
*/
extern
ASC_EXPORT int planck_register(){
	int result = 0;

#define CALCFN(NAME,INPUTS,OUTPUTS) \
	result += CreateUserFunctionBlackBox(#NAME \
		, NAME##_prepare \
		, NAME##_calc /* value */ \
		, (ExtBBoxFunc*)NULL /* derivatives not provided yet*/ \
		, (ExtBBoxFunc*)NULL /* hessian not provided yet */ \
		, (ExtBBoxFinalFunc*)NULL /* finalisation not implemented */ \
		, INPUTS,OUTPUTS /* inputs, outputs */ \
		, NAME##_help /* help text */ \
		, 0.0 \
	) /* returns 0 on success */

	CALCFN(planck,2,1);

#undef CALCFN

	if(result){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"CreateUserFunction result = %d\n",result);
	}
	return result;
}

/**
	This function is called when the black-box relation is being instantiated.
*/
static int planck_prepare(struct BBoxInterp *bbox,
	   struct Instance *data,
	   struct gl_list_t *arglist
){
	bbox->user_data = NULL;
	return 0;
}

#define CALCPREPARE(NIN,NOUT) \
	/* a few checks about the input requirements */ \
	if(ninputs != NIN)return -1; \
	if(noutputs != NOUT)return -2; \
	if(inputs==NULL)return -3; \
	if(outputs==NULL)return -4; \
	if(bbox==NULL)return -5;

/**
	Evaluation function for 'planck'
	@return 0 on success
*/
static int planck_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);

	double lam, T;

	lam = inputs[0];
	T = inputs[1];

	double res =  plnkin(lam,T);

	outputs[0] = res;
	return 0;
}

