/*
 * FPROPS fluid property calculation library
 * Copyright (C) 2011 - John Pye
 *
 * ASCEND is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ASCEND is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASCEND; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

/*
 * This file should contain general functions which are common to all equations of state.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rundata.h"
#include "filedata.h"
#include "fprops.h"
#include "helmholtz.h"
#include "ideal.h"
#include "incomp.h"
#include "sat.h"
//#include "redkw.h"
#include "pengrob.h"
#include "visc.h"
#include "thcond.h"
//#include "mbwr.h"

//#define FPR_DEBUG
#ifdef FPR_DEBUG
# include "color.h"
# define MSG FPROPS_MSG
# define ERRMSG FPROPS_ERRMSG
#else
# define MSG(ARGS...) ((void)0)
# define ERRMSG(ARGS...) ((void)0)
#endif

#include <stdio.h>

// FIXME convert to use of ideal + departure function here in this file (not at the correlation level)
// ... or doesn't that work? get further with PR EOS first?


int fprops_corr_avail(const EosData *E, const char *corrtype){
	if(corrtype==NULL){
		/* if none specify, return the 'best' we can manage (not yet very civilised) */
		switch(E->type){
		case FPROPS_HELMHOLTZ:
		case FPROPS_IDEAL:
			return E->type;
		case FPROPS_CUBIC:
			return FPROPS_PENGROB;
		case FPROPS_INCOMP:
			return FPROPS_INCOMP;
		default:
			return 0;
		}
	}else if(strcmp(corrtype,"helmholtz")==0){
		switch(E->type){
		case FPROPS_HELMHOLTZ:
			return FPROPS_HELMHOLTZ;
		default:
			return 0;
		}
	}else if(strcmp(corrtype,"pengrob")==0){
		switch(E->type){
		case FPROPS_HELMHOLTZ:
		case FPROPS_CUBIC:
			return FPROPS_PENGROB;
		default:
			return 0;
		}
	}else if(strcmp(corrtype,"ideal")==0){
		switch(E->type){
		case FPROPS_IDEAL:
		case FPROPS_HELMHOLTZ:
		case FPROPS_CUBIC:
			return FPROPS_IDEAL;
		default:
			return 0;
		}
	}else if(strcmp(corrtype,"incomp")==0){
		switch(E->type){
		case FPROPS_INCOMP:
			return FPROPS_INCOMP;
		default:
			return 0;
		/* TODO in future we could 'downsample' here from eg Helmholtz? */
		}
	}
	return 0;
}


PureFluid *fprops_prepare(const EosData *E,const char *corrtype){
	PureFluid *P = NULL;
	FpropsError err = FPROPS_NO_ERROR;
	MSG("Working with EosData name '%s', source '%s", E->name, E->source);
	MSG("Chosen correlation: %d (requested %s)", fprops_corr_avail(E,corrtype),corrtype);
	switch(fprops_corr_avail(E,corrtype)){
	case FPROPS_HELMHOLTZ:
		P = helmholtz_prepare(E,NULL);
		break;
	case FPROPS_PENGROB:
		P = pengrob_prepare(E,NULL);
		break;
	case FPROPS_IDEAL:
		P = ideal_prepare(E,NULL);
		break;
	case FPROPS_INCOMP:
		P= incomp_prepare(E,NULL);
		break;
	default:
		ERRMSG("Invalid EOS data, unimplemented correlation type requested");
		return NULL;
	}
	if(!P){
		ERRMSG("Unable to prepare fluid data with type '%s'",corrtype);
		return NULL;
	}

	/* next: add preparation of viscosity, thermal conductivity, surface tension, ... */

	MSG("Preparing viscosity data...");
	P->visc = visc_prepare(E,P,&err);
	if(err){
		ERRMSG("Invalid viscosity data for '%s",P->name);
		/* visc_prepare should return NULL if there was an error, so result is
		same as when there is no viscosity data at all */
	}

	MSG("Preparing thermal conductivity data...");
	err = FPROPS_NO_ERROR;
	if(E->thcond){
		thcond_prepare(P,E->thcond,&err);
		if(err){
			ERRMSG("Invalid viscosity data for '%s",P->name);
			/* visc_prepare should return NULL if there was an error, so result is
			same as when there is no viscosity data at all */
		}else{
			MSG("Thermal conductivity data initialised");
		}
	}else{
		MSG("No thermal conductivity data provided");
	}
	return P;
}

FluidState2 fprops_set_Trho(double T, double rho, const PureFluid *fluid, FpropsError *err){
	switch(fluid->type){
	case FPROPS_HELMHOLTZ:
	case FPROPS_IDEAL:
	case FPROPS_PENGROB:
		return (FluidState2){.vals={.Trho={T,rho}},.fluid=fluid};
	case FPROPS_INCOMP:
		ERRMSG("Unable to set incompressible state using (T,rho).");
	default:
		*err = FPROPS_INVALID_REQUEST;
		return (FluidState2){.vals={.Tp={NAN,NAN}},.fluid=fluid};
	}
}

FluidState2 fprops_set_Tp(double T, double p, const PureFluid *fluid, FpropsError *err){
	switch(fluid->type){
	case FPROPS_INCOMP:
		return (FluidState2){.vals={.Tp={T,p}},.fluid=fluid};
	case FPROPS_HELMHOLTZ:
		ERRMSG("Unable to set helmholtz state using (T,p).");
		break;
	case FPROPS_IDEAL:
		ERRMSG("Unable to set ideal state using (T,p).");
		break;
	case FPROPS_PENGROB:
		ERRMSG("Unable to set helmholtz state using (T,p).");
		break;
	default:
		break;
	}
	*err = FPROPS_INVALID_REQUEST;
	return (FluidState2){.vals={.Tp={NAN,NAN}},.fluid=fluid};
}

// FIXME XXX not all properties should be mass-weighted in the saturation region...

/*
	TODO need some kind of support for freezing line, especially for water
	since for temperatures 0.degC up to 0.01 degC liquid phase is possible
	for ambient pressure, but this is BELOW triple line. Hmmm.

	Also sublimation curve needs to be added.
*/

/* FIXME these evalfns need to check whether the fluid type in question supports VLE,
and if not, just redirect immediately to the EOS evaluation function. */

#define EVALFN(VAR) \
	double fprops_##VAR(FluidState2 state, FpropsError *err){\
		switch(state.fluid->type){\
		case FPROPS_HELMHOLTZ:\
		case FPROPS_PENGROB:\
			if(state.vals.Trho.T >= state.fluid->data->T_t && state.vals.Trho.T < state.fluid->data->T_c){\
		double p, rho_f, rho_g;\
				fprops_sat_T(state.vals.Trho.T, &p, &rho_f, &rho_g, state.fluid, err);\
			if(*err){\
				MSG("Got error %d from saturation calc in %s\n",*err,__func__);\
				/*return state.fluid->VAR##_fn(state.fluid->data->T_c,state.fluid->data->rho_c,state.fluid->data,err);*/\
				return 0;\
			}\
				if(rho_g < state.vals.Trho.rho && state.vals.Trho.rho < rho_f){\
					double x = rho_g*(rho_f/state.vals.Trho.rho - 1)/(rho_f - rho_g);\
					double Qf = state.fluid->VAR##_fn((FluidStateUnion){.Trho={state.vals.Trho.T,rho_f}},state.fluid->data,err);\
					double Qg = state.fluid->VAR##_fn((FluidStateUnion){.Trho={state.vals.Trho.T,rho_g}},state.fluid->data,err);\
				return x*Qg + (1-x)*Qf;\
			}\
		}\
			break;\
		case FPROPS_INCOMP:\
		case FPROPS_IDEAL:\
			MSG("Evaluating %s at T=%f K, p = %f bar",#VAR,state.vals.Tp.T,state.vals.Tp.p/1e5);\
			/* no phase change modelled for these fluids */\
			break;\
		default:\
			*err = FPROPS_INVALID_REQUEST;\
			return NAN;\
		}\
		return state.fluid->VAR##_fn(state.vals,state.fluid->data,err);\
	}

/// For fluids properties that are undefined in the saturation region:
#define EVALFN_SATUNDEFINED(VAR) \
	double fprops_##VAR(FluidState2 state, FpropsError *err){\
		switch(state.fluid->type){\
		case FPROPS_HELMHOLTZ:\
		case FPROPS_PENGROB:\
			{\
		double p, rho_f, rho_g;\
				if(state.vals.Trho.T >= state.fluid->data->T_t && state.vals.Trho.T < state.fluid->data->T_c){\
					fprops_sat_T(state.vals.Trho.T, &p, &rho_f, &rho_g, state.fluid, err);\
			if(*err){\
				MSG("Got error %d from sat calc in %s\n",*err,__func__);\
				return state.fluid->data->rho_c;\
			}\
					if(rho_g < state.vals.Trho.rho && state.vals.Trho.rho < rho_f){\
				*err = FPROPS_VALUE_UNDEFINED;\
			}\
		}\
			}\
			break;\
		case FPROPS_INCOMP:\
		case FPROPS_IDEAL:\
			/* no phase change modelled for these fluids */\
			break;\
		default:\
			*err = FPROPS_INVALID_REQUEST;\
			return NAN;\
		}\
		return state.fluid->VAR##_fn(state.vals,state.fluid->data,err);\
	}\

EVALFN(u); EVALFN(h); EVALFN(s); EVALFN(a); EVALFN(g);
EVALFN_SATUNDEFINED(cp); EVALFN_SATUNDEFINED(cv);
EVALFN_SATUNDEFINED(w);
EVALFN(dpdrho_T);
double fprops_v(FluidState2 state, FpropsError *err){
	return 1/fprops_rho(state,err);
}

EVALFN(alphap); EVALFN(betap);
// EVALFN(dpdT_rho);
//EVALFN(dpdrho_T); EVALFN(d2pdrho2_T); EVALFN(dhdT_rho); EVALFN(dhdrho_T);
//EVALFN(dudT_rho); EVALFN(dudrho)T);
/* The following functions need specific handling, because they are variables that
are sometimes/always stored within the FluidState2, depending on the EOS */

double fprops_T(FluidState2 state, FpropsError *err){
	switch(state.fluid->type){
	 	case FPROPS_HELMHOLTZ:
		case FPROPS_PENGROB:
		case FPROPS_IDEAL:
			return state.vals.Trho.T;
		case FPROPS_INCOMP:
			return state.vals.Tp.T;
		default:
			*err = FPROPS_INVALID_REQUEST;
			return NAN;
	}
}

double fprops_rho(FluidState2 state, FpropsError *err){
	switch(state.fluid->type){
	 	case FPROPS_HELMHOLTZ:
		case FPROPS_PENGROB:
		case FPROPS_IDEAL:
			return state.vals.Trho.rho;
		case FPROPS_INCOMP:
			return state.fluid->rho_fn(state.vals, state.fluid->data, err);
		default:
			*err = FPROPS_INVALID_REQUEST;
			return NAN;
	}
}

double fprops_p(FluidState2 state, FpropsError *err){
	double p, rho_f, rho_g;
	switch(state.fluid->type){
	case FPROPS_INCOMP:
		return state.vals.Tp.p;
	case FPROPS_HELMHOLTZ:
	case FPROPS_PENGROB:
		// fluids with a (T,rho) state and VLE
		if(state.vals.Trho.T >= state.fluid->data->T_t && state.vals.Trho.T < state.fluid->data->T_c){
			fprops_sat_T(state.vals.Trho.T, &p, &rho_f, &rho_g, state.fluid, err);
			if(*err){
				MSG("Got error %d from saturation calc in %s\n",*err,__func__);
				/*return state.fluid->VAR##_fn(state.fluid->data->T_c,state.fluid->data->rho_c,state.fluid->data,err);*/
				return 0;
			}
			if(rho_g < state.vals.Trho.rho && state.vals.Trho.rho < rho_f){
				// Note, previously we calculated p(T,rhof) and p(T,rhog) and interpolated... but that obviously shouldn't be necessary
				return p;
			}
		}
		break;
	case FPROPS_IDEAL:
		break;
	default:
		*err = FPROPS_INVALID_REQUEST;
		return NAN;
	}
	return state.fluid->p_fn(state.vals,state.fluid->data,err);
}

#if 0
double fprops_alphap(FluidState state, FpropsError *err){
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}
double fprops_betap(FluidState state, FpropsError *err){
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}
#endif

/// TODO reimplement with function pointer?
double fprops_mu(FluidState2 state, FpropsError *err){
	if(NULL!=state.fluid->visc){
		switch(state.fluid->visc->type){
		case FPROPS_VISC_1:
			return visc1_mu(state,err);
		case FPROPS_VISC_EPT:
			return visc2_mu(fprops_T(state,err),&(state.fluid->visc->data.ept),err);
		default:
			break;
		}	
	}
	*err = FPROPS_NOT_IMPLEMENTED;
	return NAN;
}

/// TODO reimplement with function pointer?
double fprops_lam(FluidState2 state, FpropsError *err){
	if(NULL == state.fluid->thcond){
		MSG("thcond data is NULL");
		*err = FPROPS_NOT_IMPLEMENTED;
		return NAN;
	}
	switch(state.fluid->thcond->type){
	case FPROPS_THCOND_1:
			return thcond1_lam(state,err);
	case FPROPS_THCOND_POLY:
		return thcond1_lam_poly(fprops_T(state,err),&(state.fluid->thcond->data.poly),err);
	default:
		ERRMSG("Thermal conductivity calculation not yet supported for this fluid");
		*err = FPROPS_INVALID_REQUEST;
		return NAN;
	}
}


double fprops_cp0(FluidState2 state, FpropsError *err){
	return ideal_cp((FluidStateUnion){.Trho={state.vals.Trho.T,0}},state.fluid->data,err);
}

double fprops_x(FluidState2 state, FpropsError *err){
	double p, rho_f, rho_g;
	if(state.vals.Trho.T >= state.fluid->data->T_t && state.vals.Trho.T < state.fluid->data->T_c){
		fprops_sat_T(state.vals.Trho.T, &p, &rho_f, &rho_g, state.fluid, err);
		if(*err)return 0;
		if(state.vals.Trho.rho > rho_f)return 0;
		if(state.vals.Trho.rho < rho_g)return 1;
		return rho_g*(rho_f/state.vals.Trho.rho - 1)/(rho_f - rho_g);
	}
	fprintf(stderr,"Temperature is <T_t or >T_c\n");
	*err = FPROPS_VALUE_UNDEFINED;
	return 0;
}

double fprops_dpdT_rho(FluidState2 state, FpropsError *err){
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}

double fprops_d2pdrho2_T(FluidState2 state, FpropsError *err){
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}

double fprops_dhdT_rho(FluidState2 state, FpropsError *err){
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}

double fprops_dhdrho_T(FluidState2 state, FpropsError *err){
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}

double fprops_dudT_rho(FluidState2 state, FpropsError *err){
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}
double fprops_dudrho_T(FluidState2 state, FpropsError *err){
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}

char *fprops_error(FpropsError err){
	switch (err) {
	case FPROPS_NO_ERROR:return NULL;
	case FPROPS_NUMERIC_ERROR:return "FPROPS encountered a numerical error.";
	case FPROPS_SAT_CVGC_ERROR:return "FPROPS unable to converge solution in saturation region.";
	case FPROPS_RANGE_ERROR:return "FPROPS had a range error on one of its inputs.";
	case FPROPS_DATA_ERROR:return "FPROPS encountered a data error.";
	case FPROPS_NOT_IMPLEMENTED:return "FPROPS feature not yet implemented.";
	case FPROPS_INVALID_REQUEST:return "FPROPS encountered an invalid request.";
	case FPROPS_VALUE_UNDEFINED:return "FPROPS reports the request value is locally undefined.";
	default:return "Unrecognised error";
	}
}

const char *fprops_corr_type(EosType type){
	switch(type){
	case FPROPS_IDEAL:
		return "ideal";
	case FPROPS_INCOMP:
		return "incomp";
	case FPROPS_CUBIC:
		return "cubic";
	case FPROPS_PENGROB:
		return "pengrob";
	case FPROPS_REDKW:
		return "redkw";
	case FPROPS_SOAVE:
		return "soave";
	case FPROPS_HELMHOLTZ:
		return "helmholtz";
	case FPROPS_MBWR:
		return "mbwr";
	}
	return NULL;
}

const char *fprops_refstate_type(ReferenceStateType type){
	switch(type){ // refer to filedata.h
	case FPROPS_REF_UNDEFINED: return "undefined";
	case FPROPS_REF_PHI0:      return "PHI0";
	case FPROPS_REF_IIR:       return "IIR";
	case FPROPS_REF_NBP:       return "NBP";
	case FPROPS_REF_TRHS:      return "TRHS";
	case FPROPS_REF_TPUS:      return "TPUS";
	case FPROPS_REF_TPHS:      return "TPHS";
	case FPROPS_REF_TPF:       return "TPF";
	case FPROPS_REF_TPFU:      return "TPFU";
	case FPROPS_REF_TPHG:      return "TPHG";
	case FPROPS_REF_TPHS0:     return "TPHS0";
	case FPROPS_REF_REF0:      return "REF0";
	}
	return NULL;
}

