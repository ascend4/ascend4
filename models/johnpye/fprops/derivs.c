/*
ASCEND modelling environment
Copyright (C) 2004-2010 John Pye

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @file
FPROPS derivatives modules. Calculates thermodynamic property derivatives
using the method given in IAPWS, 2008, Revised Advisory Note No. 3 
Thermodynamic Derivatives from IAPWS Formulation, http://www.iapws.org.

TODO we still need to figure out how this works in general for simpler non-
Helmholtz correlations such as pengrob and potentially others. It seems
plausible that this will 'just work' if each of those correlations provides 
functions for p, cv, v, s, alphap and betap as functions of FluidState (T,rho).
But we need to check that, and find out if there's an easier or more efficient
approach as well.
*/

#include "derivs.h"
#include "fprops.h"
#include "sat.h"
#include "rundata.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//#define FPE_DEBUG
#ifdef FPE_DEBUG
# include <assert.h>
#else
# define assert(ARGS...)
#endif

struct SatStateData_struct{
	FluidState state;
	double psat;
	double rhof;
	double rhog;
	double dpdT_sat;
};

#define SQ(X) ((X)*(X))

//#define DERIV_DEBUG
#define DERIV_ERRORS

#ifdef DERIV_DEBUG
# include "color.h"
# define MSG(FMT, ...) \
	color_on(stderr,ASC_FG_BRIGHTRED);\
	fprintf(stderr,"%s:%d: ",__FILE__,__LINE__);\
	color_on(stderr,ASC_FG_BRIGHTBLUE);\
	fprintf(stderr,"%s: ",__func__);\
	color_off(stderr);\
	fprintf(stderr,FMT "\n",##__VA_ARGS__)
#else
# define MSG(ARGS...) ((void)0)
#endif

/* TODO centralise declaration of our error-reporting function somehow...? */
#ifdef DERIV_ERRORS
# include "color.h"
# define ERRMSG(STR,...) \
	color_on(stderr,ASC_FG_BRIGHTRED);\
	fprintf(stderr,"ERROR:");\
	color_off(stderr);\
	fprintf(stderr," %s:%d:" STR "\n", __func__, __LINE__ ,##__VA_ARGS__)
#else
# define ERRMSG(ARGS...) ((void)0)
#endif

/*------------------------------------------------------------------------------
  EXPORTED FUNCTION(S)
*/

/**
	Calculates the derivative 

	 ⎰ ∂z ⎱
	 ⎱ ∂x ⎰y

	@param S steam state, already calculated using steam_ph, etc.
	@param x in above equation, character one of 'pTvuhsgaf'.
	@param y in above equation, character one of pTvuhsgaf.
	@param z in above equation, character one of pTvuhsgaf.
	Note that Helmholtz free energy can be signified by either 'a' or 'f'.

	@NOTE that the variables ARE NOT IN ALPHABETICAL ORDER.

	@return the numerical value of the derivative (∂z/∂x)y.
*/
double fprops_deriv(FluidState state, char *vars, FpropsError *err){
	SatStateData ssd;
	ssd.state = state;
	int i, sat = 0;

	if(!vars || strlen(vars)!=3){
		ERRMSG("Invalid 'vars' string");
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	const char *valid = "pTvuhsgaf";
	for(i=0; i<3; ++i){
		if(!strchr(valid,vars[i])){
			ERRMSG("Invalid '%c' found in vars '%s' (valid are: %s)",vars[i],vars,valid);
			*err = FPROPS_INVALID_REQUEST;
			return 0;
		}
	}

#define D fluid->data
	if(state.T < state.fluid->data->T_c
		&& state.T > state.fluid->data->T_t
	){
		fprops_sat_T(state.T, &ssd.psat, &ssd.rhof, &ssd.rhog, state.fluid, err);
		if(*err){
			ERRMSG("Failed to calculate saturation props");
			*err = FPROPS_SAT_CVGC_ERROR;
			return 0;
		}
		if(state.rho < ssd.rhof && state.rho > ssd.rhog){
			/* we're in the saturation region */
			sat = 1;
		}
	}

	/* vars to hold various partial derivs, nomenclature as follows:
	  ⎰ ∂z ⎱   →   ZVT
	  ⎱ ∂v ⎰T
	*/
	double ZTV, ZVT, XTV, XVT, YTV, YVT;

#define TVSAT(X) fprops_sat_dZdT_v(X,&ssd,err)
#define VTSAT(X) fprops_sat_dZdv_T(X,&ssd,err)
#define TVNON(X) fprops_non_dZdT_v(X,state.T,state.rho,state.fluid,err)
#define VTNON(X) fprops_non_dZdv_T(X,state.T,state.rho,state.fluid,err)
#define z vars[0]
#define x vars[1]
#define y vars[2]
	
	if(!sat){
		/* non-saturated */
		ZTV = TVNON(z); ZVT = VTNON(z);
		XTV = TVNON(x); XVT = VTNON(x);
		YTV = TVNON(y); YVT = VTNON(y);
	}else{
		/* saturated. first use Clapeyron equation for (∂p/∂T)v */
		double hf, hg;
		/* TODO add checks for error state */
		hf = fprops_h(fprops_set_Trho(state.T, ssd.rhof, state.fluid, err),err);
		hg = fprops_h(fprops_set_Trho(state.T, ssd.rhog, state.fluid, err),err);
		ssd.dpdT_sat = (hg - hf) / state.T / (1./ssd.rhog - 1./ssd.rhof);

		fprintf(stderr,"Saturation region derivatives not yet implemented.\n");
		*err = FPROPS_NOT_IMPLEMENTED;
		return 0;

		/* ...then call the partial deriv routines, which use dpdT_sat */

		ZTV = TVSAT(z); ZVT = VTSAT(z);
		XTV = TVSAT(x); XVT = VTSAT(x);
		YTV = TVSAT(y); YVT = VTSAT(y);
	}

#undef TVSAT
#undef VTSAT
#undef TVNON
#undef VTNON
#undef z
#undef x
#undef y

	double deriv = ((ZTV*YVT-ZVT*YTV)/(XTV*YVT-XVT*YTV));
	//fprintf(stderr,"Calculated (∂%c/∂%c)%c = %g\n",z,x,y,deriv);
	return deriv;
}

/*------------------------------------------------------------------------------
  Non-saturation derivatives... easy.
*/

/*
	FIXME: the following macros avoid calculating unneeded results eg within VT3 
	but at the level of freesteam_deriv, there is wasted effort, because eg 'p' 
	will be calculated several times in different calls to VT3.
*/

#define p (fluid->p_fn(T,rho,fluid->data,err))
#define cv (fluid->cv_fn(T,rho,fluid->data,err))
#define v (1./rho)
#define s (fluid->s_fn(T,rho,fluid->data,err))
#define alphap (fluid->alphap_fn(T,rho,fluid->data,err))
#define betap (fluid->betap_fn(T,rho,fluid->data,err))

double fprops_non_dZdv_T(FPROPS_CHAR z, double T, double rho, const PureFluid *fluid, FpropsError *err){
	MSG("Calculating (d%c/dv)_T in non-saturated region",z);
    double res;
	switch(z){
		case 'p': res = -p*betap; break;
		case 'T': res = 0; break;
		case 'v': res = 1; break;
		case 'u': res = p*(T*alphap-1.); break;
		case 'h': res = p*(T*alphap-v*betap); break;
		case 's': res = p*alphap; break;
		case 'g': res = -p*v*betap; break;
		case 'a':
		case 'f': res = -p; break;
		default:
			fprintf(stderr,"%s (%s:%d): Invalid variable '%c'\n", __func__,__FILE__,__LINE__,z);
			*err = FPROPS_INVALID_REQUEST;
			return 0;
	}
#if 1
	if(isnan(res)){
		fprintf(stderr,"NAN when calculating '%c'\n",z);
	}
#endif
	assert(!__isnan(res));
	MSG("(∂%c/∂v)T = %f\n",z,res);
	return res;
}

double fprops_non_dZdT_v(FPROPS_CHAR z, double T, double rho, const PureFluid *fluid, FpropsError *err){
    double res;
	switch(z){
		case 'p': res = p*alphap; break;
		case 'T': res = 1; break;
		case 'v': res = 0; break;
		case 'u': res = cv; break;
		case 'h': res = cv + p*v*alphap; break;
		case 's': res = cv/T; break;
		case 'g': res = p*v*alphap - s; break;
		case 'a':
		case 'f': res = -s; break;
		default:
			fprintf(stderr,"%s (%s:%d): Invalid variable '%c'\n", __func__,__FILE__,__LINE__,z);
			*err = FPROPS_INVALID_REQUEST;
			return 0;
	}
#if 0
	if(__isnan(res)){
		fprintf(stderr,"calculating '%c'\n",z);
	}
#endif
	assert(!__isnan(res));
	//fprintf(stderr,"(∂%c/∂T)v = %f\n",z,res);
	return res;
}
#undef p
#undef cv
#undef v
#undef s
#undef alphap
#undef betap


/*------------------------------------------------------------------------------
  Saturation region derivatives... a bit harder.

FPROPS uses T,rho as coordinates, so we need to work out what these
derivatives are going to be within the saturation region.
*/

/*
	⎰ ∂z ⎱  = ⎰∂z_f⎱ (1 - x) + ⎰∂z_g⎱ x
	⎱ ∂T ⎰v   ⎱ ∂T ⎰          ⎱ ∂T ⎰

	Need to review the theory of this section, might not be correct.
*/
double fprops_sat_dZdT_v(FPROPS_CHAR z, const SatStateData *ssd, FpropsError *err){
	double res;
	switch(z){
		/* a couple of easy cases, first: */
		case 'p': return ssd->dpdT_sat;
		case 'T': return 1.;
	}

	double drhofdT = fprops_drhofdT(ssd,err);
	double drhogdT = fprops_drhogdT(ssd,err);

	double dzfdT, dzgdT;

	assert(ssd->rhof!=0);
	assert(ssd->rhog!=0);
	double dvfdT = -1./SQ(ssd->rhof) * drhofdT;
	assert(!isnan(dvfdT));
	double dvgdT = -1./SQ(ssd->rhog) * drhogdT;
	assert(!isnan(dvgdT));

#define TVNON(X,RHO) fprops_non_dZdT_v(X,ssd->state.T,RHO,ssd->state.fluid,err)
#define VTNON(X,RHO) fprops_non_dZdv_T(X,ssd->state.T,RHO,ssd->state.fluid,err)
	dzfdT = VTNON(z,ssd->rhof)*dvfdT + TVNON(z,ssd->rhof);
	dzgdT = VTNON(z,ssd->rhog)*dvgdT + TVNON(z,ssd->rhog);

	assert(!isnan(dzfdT));
	assert(!isnan(dzgdT));

	/* FIXME: this is not the correct solution, this is for x const, not rho const. */

	double x = (1./ssd->state.rho - 1./ssd->rhof) / (1./ssd->rhog - 1./ssd->rhof);
	res = dzfdT*(1-x) + dzgdT*x;
	//fprintf(stderr,"(∂%c/∂T)x = %g\n",z,res);
	return res;
}

/*
	These derivatives are simply the gradient within the two-phase region,
	and is very simply calculated as

	⎰ ∂z ⎱   =  (z_g - z_f) / (rho_g - rho_f)  where z in {v,u,h,z,s,g,a}.
	⎱ ∂v ⎰T

	or, otherwise,

	⎰ ∂T ⎱  , ⎰ ∂p ⎱    = 0
	⎱ ∂x ⎰T   ⎱ ∂x ⎰T


	Need to double-check theory in this section.
*/
double fprops_sat_dZdv_T(FPROPS_CHAR z, const SatStateData *ssd, FpropsError *err){
	switch(z){
		case 'p': return 0;
		case 'T': return 0;
	}
	double zf, zg;
#define ZFG(Z,P,T) \
	zf = ssd->state.fluid->Z##_fn(ssd->state.T,ssd->rhof,ssd->state.fluid->data,err);\
	zg = ssd->state.fluid->Z##_fn(ssd->state.T,ssd->rhog,ssd->state.fluid->data,err)
	switch(z){
		case 'v': zf = 1./ssd->rhof; zg = 1./ssd->rhog; break;
		case 'u': ZFG(u,p,T); break;
		case 'h': ZFG(h,p,T); break;
		case 's': ZFG(s,p,T); break;
		case 'g': ZFG(g,p,T); break;
		case 'a': case 'f': ZFG(a,p,T); break;
		default:
			fprintf(stderr,"%s (%s:%d): Invalid character x = '%c'\n", __func__,__FILE__,__LINE__,z);
			*err = FPROPS_INVALID_REQUEST;
			return 0;
	}
#undef ZFG
	//fprintf(stderr,"(∂%c/∂x)T = %g\n",z,zg-zf);
	return zg - zf;
}

/*------------------------------------------------------------------------------
  DERIVATIVES OF rhof and rhog with temperature
*/

double fprops_drhofdT(const SatStateData *ssd, FpropsError *err){
	double dpdT = TVNON('p',ssd->rhof);
	double dpdrho = -1./SQ(ssd->rhof) * VTNON('p',ssd->rhof);
	return (ssd->dpdT_sat - dpdT)/dpdrho;
}

double fprops_drhogdT(const SatStateData *ssd, FpropsError *err){
	double dpdT = TVNON('p',ssd->rhog);
	double dpdrho = -1./SQ(ssd->rhog) * VTNON('p',ssd->rhog);
	return (ssd->dpdT_sat - dpdT)/dpdrho;
}



