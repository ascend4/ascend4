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
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*//** @file
The following are the functions

	 ⎰ ∂z ⎱   ___\   VTn
	 ⎱ ∂v ⎰T     /

etc., for saturation and non-saturation regions.
*/

#include "derivs.h"
#include "helmholtz.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define SQ(X) ((X)*(X))

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
double fprops_deriv(FPROPS_CHAR z, FPROPS_CHAR x, FPROPS_CHAR y, double T, double rho, const HelmholtzData *D){
	StateData S;
	S.rho = rho; S.T = T;
	char sat = 0;

	if(T < D->T_c){
		int res = fprops_sat_T(T,&S.psat, &S.rhof, &S.rhog);\
		if(res){
			fprintf(stderr,"Failed to calculation saturation props\n");
			exit(1);
		}
		if(rho < S.rhof && rho > S.rhog){
			/* we're in the saturation region */
			sat = 1;
		}
	}

	double ZTV, ZVT, XTV, XVT, YTV, YVT;
	if(sat){
		/* saturated. first use clapeyron equation for (∂p/∂T)v */ 
		double hf, hg;
		hf = helmholtz_h(S.T, S.rhof,D);
		hg = helmholtz_h(S.T, S.rhog,D);
		S.dpdT_sat = (hg - hf) / T / (1./S.rhog - 1./S.rhof);

		fprintf(stderr,"Saturation region derivatives not yet implemented.\n");
		exit(1);

		/* ...then call the partial deriv routines, which use dpdT_sat */
#define TVSAT(X) fprops_sat_dZdT_v(X,&S)
#define VTSAT(X) fprops_sat_dZdv_T(X,&S)
		ZTV = TVSAT(z); ZVT = VTSAT(z);
		XTV = TVSAT(x); XVT = VTSAT(x);
		YTV = TVSAT(y); YVT = VTSAT(y);
	}else{
		/* non-saturated */
#define TVNON(X) fprops_non_dZdT_v(X,T,rho,D)
#define VTNON(X) fprops_non_dZdv_T(X,T,rho,D)
		ZTV = TVNON(z); ZVT = VTNON(z);
		XTV = TVNON(x); XVT = VTNON(x);
		YTV = TVNON(y); YVT = VTNON(y);
	}
#undef TVNON
#undef VTNON

	double deriv = ((ZTV*YVT-ZVT*YTV)/(XTV*YVT-XVT*YTV));
	//fprintf(stderr,"Calculated (∂%c/∂%c)%c = %g\n",z,x,y,deriv);
	return deriv;
}

/*------------------------------------------------------------------------------
  Non-saturation derivatives... easy.
*/

/*
	FIXME the following macros avoid calculating unneeded results eg within VT3 
	but at the level of freesteam_deriv, there is wasted effort, because eg 'p' 
	will be calculated several times in different calls to VT3.
*/

#define p helmholtz_p(T,rho,D)
#define cv helmholtz_cv(T,rho,D)
#define v (1./rho)
#define s helmholtz_s(T,rho,D)
#define alphap helmholtz_alphap(T,rho,D)
#define betap helmholtz_betap(T,rho,D)

double fprops_non_dZdv_T(FPROPS_CHAR x, double T, double rho, const HelmholtzData *D){
	double res;
	switch(x){
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
			fprintf(stderr,"%s (%s:%d): Invalid variable '%c'\n", __func__,__FILE__,__LINE__,x);
			exit(1);
	}
	//fprintf(stderr,"(∂%c/∂v)T = %f\n",x,res);
	return res;
}

double fprops_non_dZdT_v(FPROPS_CHAR x, double T, double rho, const HelmholtzData *D){
	double res;
	switch(x){
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
			fprintf(stderr,"%s (%s:%d): Invalid variable '%c'\n", __func__,__FILE__,__LINE__,x);
			exit(1);
	}
	//fprintf(stderr,"(∂%c/∂T)v = %f\n",x,res);
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
	⎰ ∂z ⎱   =  ⎰∂z_f⎱ (1 - x) + ⎰∂z_g⎱ x
	⎱ ∂T ⎰v     ⎱ ∂T ⎰           ⎱ ∂T ⎰

	Need to review the theory of this section, might not be correct.
*/
double fprops_sat_dZdT_v(FPROPS_CHAR z, const StateData *S){
	double res;
	switch(z){
		/* a couple of easy cases, first: */
		case 'p': return S->dpdT_sat;
		case 'T': return 1.;
	}

	double drhofdT = fprops_drhofdT(S);
	double drhogdT = fprops_drhogdT(S);

	double dzfdT, dzgdT;

	assert(S->rhof!=0);
	assert(S->rhog!=0);
	double dvfdT = -1./SQ(S->rhof) * drhofdT;
	assert(!isnan(dvfdT));
	double dvgdT = -1./SQ(S->rhog) * drhogdT;
	assert(!isnan(dvgdT));

#define TVNON(X,RHO) fprops_non_dZdT_v(X,S->T,RHO,S->D)
#define VTNON(X,RHO) fprops_non_dZdv_T(X,S->T,RHO,S->D)
	dzfdT = VTNON(z,S->rhof)*dvfdT + TVNON(z,S->rhof);
	dzgdT = VTNON(z,S->rhog)*dvgdT + TVNON(z,S->rhog);

	assert(!isnan(dzfdT));
	assert(!isnan(dzgdT));
	double x;
	res = dzfdT*(1-x) + dzgdT*x;
	//fprintf(stderr,"(∂%c/∂T)x = %g\n",z,res);
	return res;
}

/*
	These derivatives are simply the gradient within the two-phase region,
	and is very simply calculated as

	⎰ ∂z   ⎱   =  (z_g - z_f) / (rho_g - rho_f)  where z in {v,u,h,z,s,g,a}.
	⎱ ∂v ⎰T

	or, otherwise,

	⎰ ∂T ⎱  , ⎰ ∂p ⎱    = 0
	⎱ ∂x ⎰T   ⎱ ∂x ⎰T


	Need to double-check theory in this section.
*/
double fprops_sat_dZdv_T(FPROPS_CHAR z, const StateData *S){
	switch(z){
		case 'p': return 0;
		case 'T': return 0;
	}
	double zf, zg;
#define ZFG(Z,P,T) \
	zf = helmholtz_##Z(S->T,S->rhof,S->D);\
	zg = helmholtz_##Z(S->T,S->rhog,S->D)
	switch(z){
		case 'v': zf = 1./S->rhof; zg = 1./S->rhog; break;
		case 'u': ZFG(u,p,T); break;
		case 'h': ZFG(h,p,T); break;
		case 's': ZFG(s,p,T); break;
		case 'g': ZFG(g,p,T); break;
		case 'a': case 'f': ZFG(a,p,T); break;
		default:
			fprintf(stderr,"%s (%s:%d): Invalid character x = '%c'\n", __func__,__FILE__,__LINE__,z);
			exit(1);
	}
#undef ZFG
	//fprintf(stderr,"(∂%c/∂x)T = %g\n",z,zg-zf);
	return zg - zf;
}

/*------------------------------------------------------------------------------
  DERIVATIVES OF rhof and rhog with temperature
*/

double fprops_drhofdT(const StateData *S){
	double dpdT = TVNON('p',S->rhof);
	double dpdrho = -1./SQ(S->rhof) * VTNON('p',S->rhof);
	return (S->dpdT_sat - dpdT)/dpdrho;
}

double fprops_drhogdT(const StateData *S){
	double dpdT = TVNON('p',S->rhog);
	double dpdrho = -1./SQ(S->rhog) * VTNON('p',S->rhog);
	return (S->dpdT_sat - dpdT)/dpdrho;
}



