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
#include <ascend/compiler/extfunc.h>

/* include error reporting API as well, so we can send messages to user */
#include <ascend/utilities/error.h>

/* for accessing the DATA instance */
#include <ascend/compiler/child.h>
#include <ascend/general/list.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/childinfo.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/slist.h>
#include <ascend/compiler/type_desc.h>
#include <ascend/compiler/packages.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/instmacro.h>
#include <ascend/compiler/instance_types.h>

/* the code that we're wrapping... */
#include "helmholtz.h"
#include "sat.h"
#include "solve_ph.h"

/* for the moment, species data are defined in C code, we'll implement something
better later on, hopefully. */
#include "ammonia.h"
#include "nitrogen.h"
#include "hydrogen.h"
#include "water.h"
#include "carbondioxide.h"

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
ExtBBoxFunc helmholtz_g_calc;
ExtBBoxFunc helmholtz_phsx_vT_calc;
ExtBBoxFunc helmholtz_Tvsx_ph_calc;

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
static const char *helmholtz_g_help = "Calculate specific Gibbs energy from temperature and density, using Helmholtz fundamental correlation";

static const char *helmholtz_phsx_vT_help = "Calculate p, h, s, x from temperature and density, using FPROPS/Helmholtz eqn";

static const char *helmholtz_Tvsx_ph_help = "Calculate T, v, s, x from pressure and enthalpy, using FPROPS/Helmholtz eqn";


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
	CALCFN(helmholtz_g,2,1);
	CALCFN(helmholtz_phsx_vT,2,4);
	CALCFN(helmholtz_Tvsx_ph,2,4);

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
	}else if(strcmp(comp,"carbondioxide")==0){
		bbox->user_data = (void*)&helmholtz_data_carbondioxide;
	}else{
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Component name was not recognised. Check the source-code for for the supported species.");
	}

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Prepared component '%s' OK.\n",comp);
	return 0;
}

/*------------------------------------------------------------------------------
  EVALULATION ROUTINES
*/

#define CALCPREPARE(NIN,NOUT) \
	/* a few checks about the input requirements */ \
	if(ninputs != NIN)return -1; \
	if(noutputs != NOUT)return -2; \
	if(inputs==NULL)return -3; \
	if(outputs==NULL)return -4; \
	if(bbox==NULL)return -5; \
	\
	/* the 'user_data' in the black box object will contain the */\
	/* coefficients required for this fluid; cast it to the required form: */\
	const HelmholtzData *helmholtz_data = (const HelmholtzData *)bbox->user_data

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
	CALCPREPARE(2,1);

	/* first input is temperature, second is density */
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
	CALCPREPARE(2,1);

	/* first input is temperature, second is density */
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
	Evaluation function for 'helmholtz_s'
	@param jacobian ignored
	@return 0 on success
*/
int helmholtz_s_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);

	/* first input is temperature, second is density */
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
	CALCPREPARE(2,1);

	/* first input is temperature, second is density */
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
	Evaluation function for 'helmholtz_a'
	@param jacobian ignored
	@return 0 on success
*/
int helmholtz_a_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);

	/* first input is temperature, second is density */
	outputs[0] = helmholtz_a(inputs[0], inputs[1], helmholtz_data);

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'helmholtz_g'
	@param jacobian ignored
	@return 0 on success
*/
int helmholtz_g_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);

	/* first input is temperature, second is density */
	outputs[0] = helmholtz_g(inputs[0], inputs[1], helmholtz_data);

	/* no need to worry about error states etc. */
	return 0;
}



/**
	Evaluation function for 'helmholtz_phsx_vT'
	@return 0 on success
*/
int helmholtz_phsx_vT_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,4);

	double rho = 1./inputs[0];
	double T = inputs[1];
	double p_sat, rho_f, rho_g;

	if(T < helmholtz_data->T_c){
		int res = fprops_sat_T(T, &p_sat, &rho_f, &rho_g, helmholtz_data);

		if(rho < rho_f && rho > rho_g){
			/* saturated */
			double vf = 1./rho_f;
			double vg = 1./rho_g;
			double x = (inputs[0] - vf)  /(vg - vf);
			double sf = helmholtz_s(T,rho_f, helmholtz_data);
			double hf = helmholtz_h(T,rho_f, helmholtz_data);
			double sg = helmholtz_s(T,rho_g, helmholtz_data);
			double hg = helmholtz_h(T,rho_g, helmholtz_data);
			outputs[0] = p_sat;
			outputs[1] = hf + x * (hg-hf);
			outputs[2] = sf + x * (sg-sf);
			outputs[3] = x;
			/* maybe there was an error solving the saturation state? */
			return res;
		}
	}

	/* non-saturated */
	outputs[0] = helmholtz_p(T,rho, helmholtz_data);
	outputs[1] = helmholtz_h(T,rho, helmholtz_data);
	outputs[2] = helmholtz_s(T,rho, helmholtz_data);
	outputs[3] = rho < helmholtz_data->rho_c ? 1 : 0;
	return 0;
}


/**
	Evaluation function for 'helmholtz_Tvsx_ph'
	@return 0 on success
*/
int helmholtz_Tvsx_ph_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,4);

	static const HelmholtzData *last = NULL;
	static double p,h,T,v,s,x;
	int res;
	if(last == helmholtz_data && p == inputs[0] && h == inputs[1]){
		outputs[0] = T;
		outputs[1] = v;
		outputs[2] = s;
		outputs[3] = x;
		return 0;
	}
	
	p = inputs[0];
	h = inputs[1];

	if(p < fprops_pc(helmholtz_data)){
		double T_sat, rho_f, rho_g;
		res = fprops_sat_p(p, &T_sat, &rho_f, &rho_g, helmholtz_data);
		if(res)return 1;
		double hf = helmholtz_h(T_sat,rho_f, helmholtz_data);
		double hg = helmholtz_h(T_sat,rho_g, helmholtz_data);

		if(hf < h && h < hg){
			/* saturated */
			double vf = 1./rho_f;
			double vg = 1./rho_g;
			double sf = helmholtz_s(T_sat,rho_f, helmholtz_data);
			double sg = helmholtz_s(T_sat,rho_g, helmholtz_data);
			T = T_sat;
			x = (h - hf)  /(hg - hf);
			v = vf + x * (vg-vf);
			s = sf + x * (sg-sf);
			last = helmholtz_data;
			outputs[0] = T;
			outputs[1] = v;
			outputs[2] = s;
			outputs[3] = x;
			return 0;
		}
	}

	double rho;
	res = fprops_solve_ph(p,h, &T, &rho, 0, helmholtz_data);
	/* non-saturated */
	v = 1./rho;
	s = helmholtz_s(T,rho, helmholtz_data);
	x = (v > 1./helmholtz_data->rho_c) ? 1 : 0;
	last = helmholtz_data;
	outputs[0] = T;
	outputs[1] = v;
	outputs[2] = s;
	outputs[3] = x;
	return 0;
}



