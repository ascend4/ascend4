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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//** @file
	Wrapper for helmholtz.c to allow access to the routine from ASCEND.
*/

/* include the external function API from libascend... */
#include <compiler/extfunc.h>

/* include error reporting API as well, so we can send messages to user */
#include <utilities/error.h>

/* for accessing the DATA instance */
#include <compiler/child.h>
#include <general/list.h>
#include <compiler/module.h>
#include <compiler/childinfo.h>
#include <compiler/parentchild.h>
#include <compiler/slist.h>
#include <compiler/type_desc.h>
#include <compiler/packages.h>
#include <compiler/symtab.h>
#include <compiler/instquery.h>
#include <compiler/instmacro.h>
#include <compiler/instance_types.h>

/* the code that we're wrapping... */
#include "helmholtz.h"

/* for the moment, species data are defined in C code, we'll implement something
better later on, hopefully. */
#include "ammonia.h"
#include "nitrogen.h"
#include "hydrogen.h"
#include "water.h"

#ifndef ASC_EXPORT
# error "Where is ASC_EXPORT?"
#endif


/*------------------------------------------------------------------------------
  FORWARD DECLARATIONS
*/

ExtBBoxInitFunc helmholtz_prepare;
ExtBBoxFunc helmholtz_p_calc;
ExtBBoxFunc helmholtz_u_calc;
ExtBBoxFunc helmholtz_s_calc;
ExtBBoxFunc helmholtz_h_calc;
ExtBBoxFunc helmholtz_a_calc;

/*------------------------------------------------------------------------------
  GLOBALS
*/

/* place to store symbols needed for accessing ASCEND's instance tree */
static symchar *helmholtz_symbols[1];
#define COMPONENT_SYM helmholtz_symbols[0]

static const char *helmholtz_p_help = "Calculate pressure from temperature and density, using Helmholtz fundamental correlation";
static const char *helmholtz_u_help = "Calculate specific internal energy from temperature and density, using Helmholtz fundamental correlation";
static const char *helmholtz_s_help = "Calculate specific entropy from temperature and density, using Helmholtz fundamental correlation";
static const char *helmholtz_h_help = "Calculate specific enthalpy from temperature and density, using Helmholtz fundamental correlation";
static const char *helmholtz_a_help = "Calculate specific Helmholtz energy from temperature and density, using Helmholtz fundamental correlation";

/*------------------------------------------------------------------------------
  REGISTRATION FUNCTION
*/

/**
	This is the function called from "IMPORT helmholtz"

	It sets up the functions contained in this external library
*/
extern
ASC_EXPORT int helmholtz_register(){
	int result = 0;

	ERROR_REPORTER_HERE(ASC_USER_WARNING,"FPROPS is still EXPERIMENTAL. Use with caution.\n");

#define CALCFN(NAME,INPUTS,OUTPUTS) \
	result += CreateUserFunctionBlackBox(#NAME \
		, helmholtz_prepare \
		, NAME##_calc /* value */ \
		, (ExtBBoxFunc*)NULL /* derivatives not provided yet*/ \
		, (ExtBBoxFunc*)NULL /* hessian not provided yet */ \
		, (ExtBBoxFinalFunc*)NULL /* finalisation not implemented */ \
		, INPUTS,OUTPUTS /* inputs, outputs */ \
		, NAME##_help /* help text */ \
		, 0.0 \
	) /* returns 0 on success */

#define CALCFNDERIV(NAME,INPUTS,OUTPUTS) \
	result += CreateUserFunctionBlackBox(#NAME \
		, helmholtz_prepare \
		, NAME##_calc /* value */ \
		, NAME##_calc /* derivatives */ \
		, (ExtBBoxFunc*)NULL /* hessian not provided yet */ \
		, (ExtBBoxFinalFunc*)NULL /* finalisation not implemented */ \
		, INPUTS,OUTPUTS /* inputs, outputs */ \
		, NAME##_help /* help text */ \
		, 0.0 \
	) /* returns 0 on success */

	CALCFNDERIV(helmholtz_p,2,1);
	CALCFN(helmholtz_u,2,1);
	CALCFN(helmholtz_s,2,1);
	CALCFN(helmholtz_h,2,1);
	CALCFN(helmholtz_a,2,1);

#undef CALCFN

	if(result){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"CreateUserFunction result = %d\n",result);
	}
	return result;
}

/**
   'helmholtz_prepare' just gets the data member and checks that it's
	valid, and stores it in the blackbox data field.
*/
int helmholtz_prepare(struct BBoxInterp *bbox,
	   struct Instance *data,
	   struct gl_list_t *arglist
){
	struct Instance *compinst;
	const char *comp;

	helmholtz_symbols[0] = AddSymbol("component");

	/* get the data file name (we will look for this file in the ASCENDLIBRARY path) */
	compinst = ChildByChar(data,COMPONENT_SYM);
	if(!compinst){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			,"Couldn't locate 'component' in DATA, please check usage of HELMHOLTZ."
		);
		return 1;
	}
	if(InstanceKind(compinst)!=SYMBOL_CONSTANT_INST){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"DATA member 'component' must be a symbol_constant");
		return 1;
	}
	comp = SCP(SYMC_INST(compinst)->value);
	CONSOLE_DEBUG("COMPONENT: %s",comp);
	if(comp==NULL || strlen(comp)==0){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"'component' is NULL or empty");
		return 1;
	}

	if(strcmp(comp,"ammonia")==0){
		bbox->user_data = (void*)&helmholtz_data_ammonia;
	}else if(strcmp(comp,"nitrogen")==0){
		bbox->user_data = (void*)&helmholtz_data_nitrogen;
	}else if(strcmp(comp,"hydrogen")==0){
		bbox->user_data = (void*)&helmholtz_data_hydrogen;
	}else if(strcmp(comp,"water")==0){
		bbox->user_data = (void*)&helmholtz_data_water;
	}else{
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Component name was not recognised. Check the source-code for for the supported species.");
	}

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Prepared component '%s' OK.\n",comp);
	return 0;
}

/*------------------------------------------------------------------------------
  EVALULATION ROUTINES
*/

#define CALCPREPARE \
	/* a few checks about the input requirements */ \
	if(ninputs != 2)return -1; \
	if(noutputs != 1)return -2; \
	if(inputs==NULL)return -3; \
	if(outputs==NULL)return -4; \
	if(bbox==NULL)return -5; \
	\
	/* the 'user_data' in the black box object will contain the */\
	/* coefficients required for this fluid; cast it to the required form: */\
	HelmholtzData *helmholtz_data = (HelmholtzData *)bbox->user_data

/**
	Evaluation function for 'helmholtz_p'.
	@param inputs array with values of inputs T and rho.
	@param outputs array with just value of p
	@param jacobian, the partial derivative df/dx, where
		each row is df[i]/dx[j] over each j for the y_out[i] of
		matching index. The jacobian array is 1-D, row major, i.e.
		df[i]/dx[j] -> jacobian[i*ninputs+j].
	@return 0 on success
*/
int helmholtz_p_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE;

	/* first input is temperature, second is molar density */
	if(bbox->task == bb_func_eval){
		outputs[0] = helmholtz_p(inputs[0], inputs[1], helmholtz_data);
	}else{
		//ERROR_REPORTER_HERE(ASC_USER_NOTE,"JACOBIAN CALCULATION FOR P!\n");
		jacobian[0*1+0] = helmholtz_dpdT_rho(inputs[0], inputs[1], helmholtz_data);
		jacobian[0*1+1] = helmholtz_dpdrho_T(inputs[0], inputs[1], helmholtz_data);
	}

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'helmholtz_u'
	@param jacobian ignored
	@return 0 on success
*/
int helmholtz_u_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE;

	/* first input is temperature, second is molar density */
	if(bbox->task == bb_func_eval){
		outputs[0] = helmholtz_u(inputs[0], inputs[1], helmholtz_data);
	}else{
		jacobian[0*1+0] = helmholtz_dudT_rho(inputs[0], inputs[1], helmholtz_data);
		jacobian[0*1+1] = helmholtz_dudrho_T(inputs[0], inputs[1], helmholtz_data);
	}

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'helmholtz_h'
	@param jacobian ignored
	@return 0 on success
*/
int helmholtz_s_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE;

	/* first input is temperature, second is molar density */
	outputs[0] = helmholtz_s(inputs[0], inputs[1], helmholtz_data);

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'helmholtz_h'
	@param jacobian ignored
	@return 0 on success
*/
int helmholtz_h_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE;

	/* first input is temperature, second is molar density */
	if(bbox->task == bb_func_eval){
		outputs[0] = helmholtz_h(inputs[0], inputs[1], helmholtz_data);
	}else{
		//ERROR_REPORTER_HERE(ASC_USER_NOTE,"JACOBIAN CALCULATION FOR P!\n");
		jacobian[0*1+0] = helmholtz_dhdT_rho(inputs[0], inputs[1], helmholtz_data);
		jacobian[0*1+1] = helmholtz_dhdrho_T(inputs[0], inputs[1], helmholtz_data);
	}

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'helmholtz_h'
	@param jacobian ignored
	@return 0 on success
*/
int helmholtz_a_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE;

	/* first input is temperature, second is molar density */
	outputs[0] = helmholtz_a(inputs[0], inputs[1], helmholtz_data);

	/* no need to worry about error states etc. */
	return 0;
}



