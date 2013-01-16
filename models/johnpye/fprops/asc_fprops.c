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
	Wrapper for FPROPS to allow access from ASCEND.
*/

//#define ASC_FPROPS_DEBUG

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
#include "fprops.h"
#include "sat.h"
#include "solve_ph.h"

/* for the moment, species data are defined in C code, we'll implement something
better later on, hopefully. */
#include "fluids.h"

#ifndef ASC_EXPORT
# error "Where is ASC_EXPORT?"
#endif


/*------------------------------------------------------------------------------
  FORWARD DECLARATIONS
*/

ExtBBoxInitFunc asc_fprops_prepare;
ExtBBoxFunc fprops_p_calc;
ExtBBoxFunc fprops_u_calc;
ExtBBoxFunc fprops_s_calc;
ExtBBoxFunc fprops_h_calc;
ExtBBoxFunc fprops_a_calc;
ExtBBoxFunc fprops_g_calc;
ExtBBoxFunc fprops_cp_calc;
ExtBBoxFunc fprops_cv_calc;
ExtBBoxFunc fprops_w_calc;
ExtBBoxFunc fprops_phsx_vT_calc;
ExtBBoxFunc fprops_Tvsx_ph_calc;

#define TCRIT(FLUID) (FLUID->data->T_c)
#define TTRIP(FLUID) (FLUID->data->T_t)
#define RHOCRIT(FLUID) (FLUID->data->rho_c)
#define PCRIT(FLUID) (FLUID->data->p_c)

/*------------------------------------------------------------------------------
  GLOBALS
*/

/* place to store symbols needed for accessing ASCEND's instance tree */
static symchar *fprops_symbols[2];
#define COMPONENT_SYM fprops_symbols[0]
#define TYPE_SYM fprops_symbols[1]

static const char *fprops_p_help = "Calculate pressure from temperature and density, using FPROPS";
static const char *fprops_u_help = "Calculate specific internal energy from temperature and density, using FPROPS";
static const char *fprops_s_help = "Calculate specific entropy from temperature and density, using FPROPS";
static const char *fprops_h_help = "Calculate specific enthalpy from temperature and density, using FPROPS";
static const char *fprops_a_help = "Calculate specific Helmholtz energy from temperature and density, using FPROPS";
static const char *fprops_g_help = "Calculate specific Gibbs energy from temperature and density, using FPROPS";
static const char *fprops_cp_help = "Calculate isobaric specific heat from temperature and density, using FPROPS";
static const char *fprops_cv_help = "Calculate isochoric specific heat from temperature and density, using FPROPS";
static const char *fprops_w_help = "Calculate speed of sound from temperature and density, using FPROPS";

static const char *fprops_phsx_vT_help = "Calculate p, h, s, x from temperature and density, using FPROPS/Helmholtz eqn";

static const char *fprops_Tvsx_ph_help = "Calculate T, v, s, x from pressure and enthalpy, using FPROPS/Helmholtz eqn";

/*------------------------------------------------------------------------------
  REGISTRATION FUNCTION
*/

/**
	This is the function called from "IMPORT fprops"

	It sets up the functions contained in this external library
*/
extern
ASC_EXPORT int fprops_register(){
	int result = 0;

	ERROR_REPORTER_HERE(ASC_USER_WARNING,"FPROPS is still EXPERIMENTAL. Use with caution.\n");

#define CALCFN(NAME,INPUTS,OUTPUTS) \
	result += CreateUserFunctionBlackBox(#NAME \
		, asc_fprops_prepare \
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
		, asc_fprops_prepare \
		, NAME##_calc /* value */ \
		, NAME##_calc /* derivatives */ \
		, (ExtBBoxFunc*)NULL /* hessian not provided yet */ \
		, (ExtBBoxFinalFunc*)NULL /* finalisation not implemented */ \
		, INPUTS,OUTPUTS /* inputs, outputs */ \
		, NAME##_help /* help text */ \
		, 0.0 \
	) /* returns 0 on success */

	CALCFNDERIV(fprops_p,2,1);
	CALCFN(fprops_u,2,1);
	CALCFN(fprops_s,2,1);
	CALCFN(fprops_h,2,1);
	CALCFN(fprops_a,2,1);
	CALCFN(fprops_g,2,1);
	CALCFN(fprops_cp,2,1);
	CALCFN(fprops_cv,2,1);
	CALCFN(fprops_w,2,1);
	CALCFN(fprops_phsx_vT,2,4);
	CALCFN(fprops_Tvsx_ph,2,4);

#undef CALCFN

	if(result){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"CreateUserFunction result = %d\n",result);
	}
	return result;
}

/**
   'fprops_prepare' just gets the data member and checks that it's
	valid, and stores it in the blackbox data field.
*/
int asc_fprops_prepare(struct BBoxInterp *bbox,
	   struct Instance *data,
	   struct gl_list_t *arglist
){
	struct Instance *compinst, *typeinst;
	const char *comp, *type = NULL;

	fprops_symbols[0] = AddSymbol("component");
	fprops_symbols[1] = AddSymbol("type");

	/* get the component name */
	compinst = ChildByChar(data,COMPONENT_SYM);
	if(!compinst){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			,"Couldn't locate 'component' in DATA, please check usage of FPROPS."
		);
		return 1;
	}
	if(InstanceKind(compinst)!=SYMBOL_CONSTANT_INST){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"DATA member 'component' must be a symbol_constant");
		return 1;
	}
	comp = SCP(SYMC_INST(compinst)->value);
	if(comp==NULL || strlen(comp)==0){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"'component' is NULL or empty");
		return 1;
	}

	/* get the component correlation type (FPROPS doesn't mind if none given) */
	typeinst = ChildByChar(data,TYPE_SYM);
	if(typeinst){
		if(InstanceKind(typeinst)!=SYMBOL_CONSTANT_INST){
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"DATA member 'type' must be a symbol_constant");
			return 1;
		}
		type = SCP(SYMC_INST(typeinst)->value);
		CONSOLE_DEBUG("TYPE: %s",type);
		if(strlen(type)==0)type = NULL;
	}

	bbox->user_data = (void *)fprops_fluid(comp,type);
	if(bbox->user_data == NULL){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Component name/type was not recognised. Check the source-code for for the supported species.");
		return 1;
	}

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Prepared component '%s'%s%s%s OK.\n"
		,comp
		,type?" type '":""
		,type?type:""
		,type?"'":""
	);
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
	const PureFluid *FLUID = (const PureFluid *)bbox->user_data;\
    FpropsError err=FPROPS_NO_ERROR;

/**
	Evaluation function for 'fprops_p'.
	@param inputs array with values of inputs T and rho.
	@param outputs array with just value of p
	@param jacobian, the partial derivative df/dx, where
		each row is df[i]/dx[j] over each j for the y_out[i] of
		matching index. The jacobian array is 1-D, row major, i.e.
		df[i]/dx[j] -> jacobian[i*ninputs+j].
	@return 0 on success
*/
int fprops_p_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);

	/* first input is temperature, second is density */
	if(bbox->task == bb_func_eval){
		FluidState S = fprops_set_Trho(inputs[0],inputs[1], FLUID, &err);
		outputs[0] = fprops_p(S, &err);
	}else{
		//ERROR_REPORTER_HERE(ASC_USER_NOTE,"JACOBIAN CALCULATION FOR P!\n");
		FluidState S = fprops_set_Trho(inputs[0],inputs[1], FLUID, &err);
		jacobian[0*1+0] = fprops_dpdT_rho(S, &err);
		jacobian[0*1+1] = fprops_dpdrho_T(S, &err);
	}

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'fprops_u'
	@param jacobian ignored
	@return 0 on success
*/
int fprops_u_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	if(bbox->task == bb_func_eval){
		outputs[0] = fprops_u(S, &err);
	}else{
		jacobian[0*1+0] = fprops_dudT_rho(S, &err);
		jacobian[0*1+1] = fprops_dudrho_T(S, &err);
	}

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'fprops_s'
	@param jacobian ignored
	@return 0 on success
*/
int fprops_s_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	outputs[0] = fprops_s(S, &err);

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'fprops_h'
	@param jacobian ignored
	@return 0 on success
*/
int fprops_h_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	if(bbox->task == bb_func_eval){
		outputs[0] = fprops_h(S, &err);
	}else{
		//ERROR_REPORTER_HERE(ASC_USER_NOTE,"JACOBIAN CALCULATION FOR P!\n");
		jacobian[0*1+0] = fprops_dhdT_rho(S, &err);
		jacobian[0*1+1] = fprops_dhdrho_T(S, &err);
	}

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'fprops_a'
	@param jacobian ignored
	@return 0 on success
*/
int fprops_a_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	outputs[0] = fprops_a(S, &err);

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'fprops_g'
	@param jacobian ignored
	@return 0 on success
*/
int fprops_g_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	outputs[0] = fprops_g(S, &err);

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'fprops_cp'
	@param jacobian ignored
	@return 0 on success
*/
int fprops_cp_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	outputs[0] = fprops_cp(S, &err);

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'fprops_cv'
	@param jacobian ignored
	@return 0 on success
*/
int fprops_cv_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	outputs[0] = fprops_cv(S, &err);

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'fprops_w'
	@param jacobian ignored
	@return 0 on success
*/
int fprops_w_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	outputs[0] = fprops_w(S, &err);

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'fprops_phsx_vT'
	@return 0 on success
*/
int fprops_phsx_vT_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,4);

	double rho = 1./inputs[0];
	double T = inputs[1];
	double p_sat, rho_f, rho_g;

	if(T < TCRIT(FLUID)){
		fprops_sat_T(T, &p_sat, &rho_f, &rho_g, FLUID, &err);

		if(rho < rho_f && rho > rho_g){
			/* saturated */
			double vf = 1./rho_f;
			double vg = 1./rho_g;
			double x = (inputs[0] - vf)  /(vg - vf);
			FluidState Sf = fprops_set_Trho(T, rho_f, FLUID, &err);
			double sf = fprops_s(Sf, &err);
			double hf = fprops_h(Sf, &err);
			FluidState Sg = fprops_set_Trho(T, rho_g, FLUID, &err);
			double sg = fprops_s(Sg, &err);
			double hg = fprops_h(Sg, &err);
			outputs[0] = p_sat;
			outputs[1] = hf + x * (hg-hf);
			outputs[2] = sf + x * (sg-sf);
			outputs[3] = x;
			/* maybe there was an error solving the saturation state? */
			return err;
		}
	}

	/* non-saturated */
	FluidState S = fprops_set_Trho(T, rho, FLUID, &err);
	outputs[0] = fprops_p(S, &err);
	outputs[1] = fprops_h(S, &err);
	outputs[2] = fprops_s(S, &err);
	outputs[3] = rho < RHOCRIT(FLUID) ? 1 : 0;
	return 0;
}


/**
	Evaluation function for 'fprops_Tvsx_ph'
	@return 0 on success
*/
int fprops_Tvsx_ph_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,4);

	static const PureFluid *last = NULL;
	static double p,h,T,v,s,x;
	if(last == FLUID && p == inputs[0] && h == inputs[1]){
		outputs[0] = T;
		outputs[1] = v;
		outputs[2] = s;
		outputs[3] = x;
		return 0;
	}

	p = inputs[0];
	h = inputs[1];

	double hft, pt, rhoft,rhogt;
	fprops_triple_point(&pt,&rhoft,&rhogt,FLUID,&err);
	if(err){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to solve triple point for %s.",FLUID->name);
		return 5;
	}
	FluidState Sft = fprops_set_Trho(TTRIP(FLUID),rhoft,FLUID,&err);
	hft = fprops_h(Sft, &err);
	if(h < hft){
		ERROR_REPORTER_HERE(ASC_PROG_ERR
			,"Input enthalpy %f kJ/kg is below triple point liquid enthalpy %f kJ/kg"
			,h/1e3,hft/1e3
		);
		return 6;
	}
	
	if(p < pt){
		ERROR_REPORTER_HERE(ASC_PROG_ERR
			,"Input pressure %f bar is below triple point pressure %f bar"
			,p/1e5,pt/1e5
		);
		outputs[0] = TTRIP(FLUID);
		outputs[1] = 1./ rhoft;
		outputs[2] = FLUID->s_fn(TTRIP(FLUID), rhoft, FLUID->data, &err);
		outputs[3] = 0;
		return 6;
	}

	if(p < PCRIT(FLUID)){
		double T_sat, rho_f, rho_g;
		fprops_sat_p(p, &T_sat, &rho_f, &rho_g, FLUID, &err);
		if(err){
			ERROR_REPORTER_HERE(ASC_PROG_ERR
				, "Failed to solve saturation state of %s for p = %f bar < pc (= %f bar)"
				, FLUID->name, p/1e5,PCRIT(FLUID)/1e5
			);
			outputs[0] = TTRIP(FLUID);
			outputs[1] = 1./rhoft;
			outputs[2] = FLUID->s_fn(TTRIP(FLUID), rhoft, FLUID->data, &err);
			outputs[3] = 0;
			return 1;
		}
		
		FluidState Sf = fprops_set_Trho(T_sat,rho_f,FLUID,&err);
		FluidState Sg = fprops_set_Trho(T_sat,rho_g,FLUID,&err);
		double hf = fprops_h(Sf, &err);
		double hg = fprops_h(Sg, &err);

		if(hf < h && h < hg){
			/* saturated */
			double vf = 1./rho_f;
			double vg = 1./rho_g;
			double sf = fprops_s(Sf, &err);
			double sg = fprops_s(Sg, &err);
			T = T_sat;
			x = (h - hf)  /(hg - hf);
			v = vf + x * (vg-vf);
			s = sf + x * (sg-sf);
			last = FLUID;
			outputs[0] = T;
			outputs[1] = v;
			outputs[2] = s;
			outputs[3] = x;
#ifdef ASC_FPROPS_DEBUG
			ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Saturated state, p=%f bar, h = %f kJ/kg",p/1e5,h/1e3);
#endif
			return 0;
		}
	}

	double rho;
	fprops_solve_ph(p,h, &T, &rho, 0, FLUID, &err);
	/* non-saturated */
	v = 1./rho;
	FluidState S = fprops_set_Trho(T,rho,FLUID,&err);
	s = fprops_s(S, &err);
	x = (v > 1./RHOCRIT(FLUID)) ? 1 : 0;
	last = FLUID;
	outputs[0] = T;
	outputs[1] = v;
	outputs[2] = s;
	outputs[3] = x;
#ifdef ASC_FPROPS_DEBUG
	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Non-saturated state, p = %f bar, h = %f kJ/kg",p/1e5,h/1e3);
#endif
	return err;
}



