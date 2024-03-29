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
	Wrapper for FPROPS to allow access from ASCEND.
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
#include "fprops.h"
#include "sat.h"
#include "solve_ph.h"
#include "thcond.h"
#include "visc.h"

/* for the moment, species data are defined in C code, we'll implement something
better later on, hopefully. */
#include "fluids.h"

#ifndef ASC_EXPORT
# error "Where is ASC_EXPORT?"
#endif

//#define ASCFPROPS_DEBUG
#ifdef ASCFPROPS_DEBUG
# define MSG(MSG,ARGS...) ERROR_REPORTER_HERE(ASC_PROG_NOTE,MSG "\n",##ARGS);
#else
# define MSG(ARGS...) ((void)0)
#endif

#define ERRMSG(MSG,ARGS...) ERROR_REPORTER_HERE(ASC_USER_ERROR,MSG,##ARGS);
#define ERRMSGP(MSG,ARGS...) ERROR_REPORTER_HERE(ASC_PROG_ERR,MSG,##ARGS);

/*------------------------------------------------------------------------------
  FORWARD DECLARATIONS
*/

ExtBBoxInitFunc asc_fprops_prepare;
ExtBBoxFunc fprops_p_Trho_calc;
ExtBBoxFunc fprops_u_Trho_calc;
ExtBBoxFunc fprops_s_Trho_calc;
ExtBBoxFunc fprops_a_Trho_calc;
ExtBBoxFunc fprops_h_Trho_calc;
ExtBBoxFunc fprops_g_Trho_calc;
ExtBBoxFunc fprops_cp_Trho_calc;
ExtBBoxFunc fprops_cv_Trho_calc;
ExtBBoxFunc fprops_w_Trho_calc;
ExtBBoxFunc fprops_mu_Trho_calc;
ExtBBoxFunc fprops_lam_Trho_calc;
ExtBBoxFunc fprops_rho_Tp_calc;
ExtBBoxFunc fprops_cp_Tp_calc;
ExtBBoxFunc fprops_h_Tp_calc;
ExtBBoxFunc fprops_s_Tp_calc;
ExtBBoxFunc fprops_mu_T_incomp_calc;
ExtBBoxFunc fprops_lam_T_incomp_calc;
ExtBBoxFunc fprops_cp_T_incomp_calc;
ExtBBoxFunc fprops_phsx_vT_calc;
ExtBBoxFunc fprops_Tvsx_ph_calc;
ExtBBoxFunc fprops_Tvsx_h_incomp_calc;

/* FIXME need incompressible fluid functions that depend only on T or h, to 
	avoid unpivoted external relations...
*/

#define TCRIT(FLUID) (FLUID->data->T_c)
#define TTRIP(FLUID) (FLUID->data->T_t)
#define RHOCRIT(FLUID) (FLUID->data->rho_c)
#define PCRIT(FLUID) (FLUID->data->p_c)
#define FSU_TRHO(T,RHO) (FluidStateUnion){.Trho={T,RHO}}

/*------------------------------------------------------------------------------
  GLOBALS
*/

/* place to store symbols needed for accessing ASCEND's instance tree */
static symchar *fprops_symbols[3];
#define COMPONENT_SYM fprops_symbols[0]
#define TYPE_SYM fprops_symbols[1]
#define SOURCE_SYM fprops_symbols[2]

static const char *fprops_p_Trho_help = "Calculate pressure from temperature and density, using FPROPS";
static const char *fprops_u_Trho_help = "Calculate specific internal energy from temperature and density, using FPROPS";
static const char *fprops_s_Trho_help = "Calculate specific entropy from temperature and density, using FPROPS";
static const char *fprops_h_Trho_help = "Calculate specific enthalpy from temperature and density, using FPROPS";
static const char *fprops_a_Trho_help = "Calculate specific Helmholtz energy from temperature and density, using FPROPS";
static const char *fprops_g_Trho_help = "Calculate specific Gibbs energy from temperature and density, using FPROPS";
static const char *fprops_cp_Trho_help = "Calculate isobaric specific heat from temperature and density, using FPROPS";
static const char *fprops_cv_Trho_help = "Calculate isochoric specific heat from temperature and density, using FPROPS";
static const char *fprops_w_Trho_help = "Calculate speed of sound from temperature and density, using FPROPS";
static const char *fprops_mu_Trho_help = "Calculate viscosity from temperature and density, using FPROPS";
static const char *fprops_lam_Trho_help = "Calculate thermal conductivity sound from temperature and density, using FPROPS";

static const char *fprops_rho_Tp_help = "rho(T,p) esp. for incompressible substances";
static const char *fprops_cp_Tp_help = "h(T,p) esp. for incompressible substances";
static const char *fprops_h_Tp_help = "h(T,p) esp. for incompressible substances";
static const char *fprops_s_Tp_help = "s(T,p) esp. for incompressible substances";
static const char *fprops_mu_T_incomp_help = "mu(T,p) (dynamic viscosity) esp. for incompressible substances";
static const char *fprops_lam_T_incomp_help = "lam(T) (thermal conductivity) esp. for incompressible substances";
static const char *fprops_cp_T_incomp_help = "cp(T) (specific heat capacity) esp. for incompressible substances";

static const char *fprops_phsx_vT_help = "Calculate p, h, s, x from specific volume and temperature, using FPROPS";

static const char *fprops_Tvsx_ph_help = "Calculate T, v, s, x from pressure and enthalpy, using FPROPS";
static const char *fprops_Tvsx_h_incomp_help = "Calculate T, v, s, x for incompressible fluid from enthalpy, using FPROPS";
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

	ERROR_REPORTER_HERE(ASC_USER_WARNING,"FPROPS is still EXPERIMENTAL. Use with caution.");

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

	CALCFNDERIV(fprops_p_Trho,2,1);
	CALCFN(fprops_u_Trho,2,1);
	CALCFN(fprops_s_Trho,2,1);
	CALCFN(fprops_h_Trho,2,1);
	CALCFN(fprops_a_Trho,2,1);
	CALCFN(fprops_g_Trho,2,1);
	CALCFN(fprops_cp_Trho,2,1);
	CALCFN(fprops_cv_Trho,2,1);
	CALCFN(fprops_w_Trho,2,1);
	CALCFN(fprops_mu_Trho,2,1);
	CALCFN(fprops_lam_Trho,2,1);

	CALCFN(fprops_rho_Tp,2,1);
	CALCFN(fprops_cp_Tp,2,1);
	CALCFN(fprops_h_Tp,2,1);
	CALCFN(fprops_s_Tp,2,1);
	CALCFN(fprops_mu_T_incomp,1,1);
	CALCFN(fprops_lam_T_incomp,1,1);
	CALCFN(fprops_cp_T_incomp,1,1);
	CALCFN(fprops_phsx_vT,2,4);
	CALCFN(fprops_Tvsx_ph,2,4);
	CALCFN(fprops_Tvsx_h_incomp,2,4);

#undef CALCFN

	if(result){
		MSG("CreateUserFunction result = %d.",result);
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
	struct Instance *compinst, *typeinst, *srcinst;
	const char *comp, *type = NULL, *src = NULL;

	fprops_symbols[0] = AddSymbol("component");
	fprops_symbols[1] = AddSymbol("type");
	fprops_symbols[2] = AddSymbol("source");

	/* get the component name */
	compinst = ChildByChar(data,COMPONENT_SYM);
	if(!compinst){
		ERRMSG("Couldn't locate 'component' in DATA, please check usage of FPROPS.")
		return 1;
	}
	if(InstanceKind(compinst)!=SYMBOL_CONSTANT_INST){
		ERRMSG("DATA member 'component' must be a symbol_constant");
		return 1;
	}
	comp = SCP(SYMC_INST(compinst)->value);
	if(comp==NULL || strlen(comp)==0){
		ERRMSG("'component' is NULL or empty");
		return 1;
	}

	/* get the component correlation type (FPROPS doesn't mind if none given) */
	typeinst = ChildByChar(data,TYPE_SYM);
	if(typeinst){
		if(InstanceKind(typeinst)!=SYMBOL_CONSTANT_INST){
			ERRMSG("DATA member 'type' must be a symbol_constant");
			return 1;
		}
		type = SCP(SYMC_INST(typeinst)->value);
		//CONSOLE_DEBUG("TYPE: %s",type?type:"(null)");
		if(type && strlen(type)==0)type = NULL;
	}

	/* get the source data string (FPROPS doesn't mind if none given) */
	srcinst = ChildByChar(data,SOURCE_SYM);
	if(srcinst){
		if(InstanceKind(srcinst)!=SYMBOL_CONSTANT_INST){
			ERRMSG("DATA member 'source' must be a symbol_constant");
			return 1;
		}
		src = SCP(SYMC_INST(srcinst)->value);
		CONSOLE_DEBUG("SOURCE: %s",src?src:"(null)");
		if(src && strlen(src)==0)src = NULL;
	}

	bbox->user_data = (void *)fprops_fluid(comp,type,src);
	if(bbox->user_data == NULL){
		ERRMSG("Unsupported component requested (name='%s',type='%s'). Check source-code for supported species.",comp,type);
		return 1;
	}

	MSG("Prepared component '%s'%s%s%s OK.",comp, type?" type '":"", type?type:"" ,type?"'":""
	);
	return 0;
}

/*------------------------------------------------------------------------------
  EVALULATION ROUTINES
*/

static const char *ninputs_msg = "Incorrect call: %u inputs received, but expected %u";
static const char *noutputs_msg = "Incorrect call: %u outputs received, but expected %u";
#define CALCPREPARE(NIN,NOUT) \
	/* a few checks about the input requirements */ \
	if(ninputs != NIN){ERRMSG(ninputs_msg,ninputs,NIN);return -1;} \
	if(noutputs != NOUT){ERRMSG(noutputs_msg,noutputs,NOUT)return -2;} \
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
int fprops_p_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);

	/* first input is temperature, second is density */
	if(bbox->task == bb_func_eval){
		FluidState2 S = fprops_set_Trho(inputs[0],inputs[1], FLUID, &err);
		outputs[0] = fprops_p(S, &err);
	}else{
		//MSG("JACOBIAN CALCULATION FOR P!\n");
		FluidState2 S = fprops_set_Trho(inputs[0],inputs[1], FLUID, &err);
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
int fprops_u_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState2 S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

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
int fprops_s_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState2 S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

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
int fprops_h_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState2 S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	if(bbox->task == bb_func_eval){
		outputs[0] = fprops_h(S, &err);
	}else{
		//MSG("JACOBIAN CALCULATION FOR P!\n");
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
int fprops_a_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState2 S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

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
int fprops_g_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState2 S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

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
int fprops_cp_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState2 S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

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
int fprops_cv_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState2 S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

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
int fprops_w_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState2 S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	outputs[0] = fprops_w(S, &err);

	/* no need to worry about error states etc. */
	return 0;
}

/**
	Evaluation function for 'fprops_mu'
	@param jacobian ignored
	@return 0 on success
*/
int fprops_mu_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState2 S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	outputs[0] = fprops_mu(S, &err);

	/* no need to worry about error states etc. */
	return 0;
}

/**
	Evaluation function for 'fprops_lam'
	@param jacobian ignored
	@return 0 on success
*/
int fprops_lam_Trho_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(2,1);
	FluidState2 S = fprops_set_Trho(inputs[0], inputs[1], FLUID, &err);

	/* first input is temperature, second is density */
	outputs[0] = fprops_lam(S, &err);

	/* no need to worry about error states etc. */
	return 0;
}

#define	CALC_TP_BODY(fn) \
	CALCPREPARE(2,1);\
	FluidState2 S = fprops_set_Tp(inputs[0],inputs[1],FLUID,&err);\
	outputs[0] = fprops_##fn(S,&err);\
	if(err){\
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to solve %s(T,rho) for '%s' (err '%s'."\
			,#fn,FLUID->name,fprops_error(err));\
		return 1;\
	}\
	return 0;

#define	CALC_T_BODY(fn) \
	CALCPREPARE(1,1); /* the pressure value is completely arbitrary, set to 999 */\
	FluidState2 S = fprops_set_Tp(inputs[0],999,FLUID,&err);\
	outputs[0] = fprops_##fn(S,&err);\
	if(err){\
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to solve %s(T,rho) for '%s' (err '%s'."\
			,#fn,FLUID->name,fprops_error(err));\
		return 1;\
	}\
	return 0;

int fprops_rho_Tp_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALC_TP_BODY(lam);
}
int fprops_cp_Tp_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALC_TP_BODY(cp);
}
int fprops_h_Tp_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALC_TP_BODY(h);
}
int fprops_s_Tp_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALC_TP_BODY(s);
}
int fprops_mu_T_incomp_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALC_T_BODY(mu);
}
int fprops_lam_T_incomp_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALC_T_BODY(lam);
}

int fprops_cp_T_incomp_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALC_T_BODY(cp);
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

	/*
		TODO unclear why we don't put this code into the main fprops.c routines...?
	 	there is an attempt here to minimise calls to the slow function fprops_sat_T,
		by making direct calls to s_fn and h_fn. FIXME A better solution would be to move 
		all of this code into solve_ph.c or similar.
	*/
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
			double sf = FLUID->s_fn(FSU_TRHO(T,rho_f), FLUID->data, &err);
			double hf = FLUID->h_fn(FSU_TRHO(T,rho_f), FLUID->data, &err);
			double sg = FLUID->s_fn(FSU_TRHO(T,rho_g), FLUID->data, &err);
			double hg = FLUID->h_fn(FSU_TRHO(T,rho_g), FLUID->data, &err);
			outputs[0] = p_sat;
			outputs[1] = hf + x * (hg-hf);
			outputs[2] = sf + x * (sg-sf);
			outputs[3] = x;
			/* maybe there was an error solving the saturation state? */
			return err;
		}
	}

	/* non-saturated */
	outputs[0] = FLUID->p_fn(FSU_TRHO(T,rho), FLUID->data, &err);
	outputs[1] = FLUID->h_fn(FSU_TRHO(T,rho), FLUID->data, &err);
	outputs[2] = FLUID->s_fn(FSU_TRHO(T,rho), FLUID->data, &err);
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
	switch(FLUID->type){
	case FPROPS_HELMHOLTZ:
	case FPROPS_PENGROB:
		{
			// the code below aims to avoid calls to fprops_sat_T, which are slow.

	double hft, pt, rhoft,rhogt;
	fprops_triple_point(&pt,&rhoft,&rhogt,FLUID,&err);
	if(err){
				ERRMSGP("Failed to solve triple point for %s.",FLUID->name);
		return 5;
	}
			hft = FLUID->h_fn(FSU_TRHO(TTRIP(FLUID),rhoft),FLUID->data,&err);
	if(h < hft){
				ERRMSGP("Input enthalpy %f kJ/kg is below triple point liquid enthalpy %f kJ/kg"
			,h/1e3,hft/1e3
		);
		return 6;
	}
	
	if(p < pt){
				ERRMSGP("Input pressure %f bar is below triple point pressure %f bar"
			,p/1e5,pt/1e5
		);
		outputs[0] = TTRIP(FLUID);
		outputs[1] = 1./ rhoft;
				outputs[2] = FLUID->s_fn(FSU_TRHO(TTRIP(FLUID),rhoft),FLUID->data, &err);
		outputs[3] = 0;
		return 7;
	}

	if(p < PCRIT(FLUID)){
		double T_sat, rho_f, rho_g;
		
		fprops_sat_p(p, &T_sat, &rho_f, &rho_g, FLUID, &err);
		if(err){
					ERRMSGP("Failed to solve saturation state of %s for p = %f bar < pc (= %f bar)"
				, FLUID->name, p/1e5,PCRIT(FLUID)/1e5
			);
			outputs[0] = TTRIP(FLUID);
			outputs[1] = 1./rhoft;
					outputs[2] = FLUID->s_fn(FSU_TRHO(TTRIP(FLUID), rhoft), FLUID->data, &err);
			outputs[3] = 0;
			return 8;
		}
		
				double hf = FLUID->h_fn(FSU_TRHO(T_sat, rho_f),FLUID->data,&err);
				double hg = FLUID->h_fn(FSU_TRHO(T_sat, rho_g),FLUID->data,&err);

		if(hf < h && h < hg){
			/* saturated */
			double vf = 1./rho_f;
			double vg = 1./rho_g;
					double sf = FLUID->s_fn(FSU_TRHO(T_sat, rho_f),FLUID->data,&err);
					double sg = FLUID->s_fn(FSU_TRHO(T_sat, rho_g),FLUID->data,&err);
			T = T_sat;
			x = (h - hf)  /(hg - hf);
			v = vf + x * (vg-vf);
			s = sf + x * (sg-sf);
			last = FLUID;
			outputs[0] = T;
			outputs[1] = v;
			outputs[2] = s;
			outputs[3] = x;
					MSG("Saturated state, p=%f bar, h = %f kJ/kg.",p/1e5,h/1e3);
			return 0;
		}
	}

			FluidState2 S = fprops_solve_ph(p,h, FLUID, &err); // prev code was use_guess=0
			double rho = S.vals.Trho.rho;
			T = S.vals.Trho.T;
	if(err){
				ERRMSGP("Failed to solve for (p,h): %s",fprops_error(err));
		return 9;
	}
	/* non-saturated */
	v = 1./rho;
			s = FLUID->s_fn(S.vals, FLUID->data, &err); // straight to EOS, no sat test req.
	x = (v > 1./RHOCRIT(FLUID)) ? 1 : 0;
	last = FLUID;
	outputs[0] = T;
	outputs[1] = v;
	outputs[2] = s;
	outputs[3] = x;
			MSG("Non-saturated state, p = %f bar, h = %f kJ/kg.",p/1e5,h/1e3);
	return 0;
}
	case FPROPS_INCOMP:
		{
			MSG("Solving for p=%f bar, h=%f kJ/kg.",p/1e5, h/1e3);
			FluidState2 S;
			S = fprops_solve_ph(p,h,FLUID,&err);
			double rho = fprops_rho(S,&err);
			T = fprops_T(S,&err);
			s = fprops_s(S,&err);
			v = 1./rho;
			x = 0;
			MSG("Got T = %f, rho = %f, s = %f",T,rho,s);
			if(err){
				ERRMSGP("Failed to solve (p,h): %s (fluid '%s')",fprops_error(err),FLUID->name);
				return 9;
			}
			last = FLUID;
			outputs[0] = T;
			outputs[1] = v;
			outputs[2] = s;
			outputs[3] = x;
			return 0;
		}
	default:
		ERRMSGP("Invalid fluid type (type %u)",FLUID->type);
		return 10;
	}
}




/**
	Evaluation function for 'fprops_Tvsx_h_incomp'
	@return 0 on success
*/
int fprops_Tvsx_h_incomp_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(1,4);

	static const PureFluid *last = NULL;
	double p = 1e5; // arbitrary!
	static double h,T,v,s,x;
	if(last == FLUID && h == inputs[1]){
		outputs[0] = T;
		outputs[1] = v;
		outputs[2] = s;
		outputs[3] = x;
		return 0;
	}

	h = inputs[0];

	MSG("hello!");

	switch(FLUID->type){
	case FPROPS_INCOMP:
		{
			MSG("Solving for p=%f bar, h=%f kJ/kg.",p/1e5, h/1e3);
			FluidState2 S;
			S = fprops_solve_ph(p,h,FLUID,&err);
			double rho = fprops_rho(S,&err);
			T = fprops_T(S,&err);
			s = fprops_s(S,&err);
			v = 1./rho;
			x = 0;
			if(err){
				ERRMSGP("Failed to solve (p,h): %s (fluid '%s')",fprops_error(err),FLUID->name);
				return 9;
			}
			last = FLUID;
			outputs[0] = T;
			outputs[1] = v;
			outputs[2] = s;
			outputs[3] = x;
			MSG("...returning T = %f",T);
			return 0;
		}
	default:
		ERRMSGP("Invalid fluid type (type %u)",FLUID->type);
		return 10;
	}
}



