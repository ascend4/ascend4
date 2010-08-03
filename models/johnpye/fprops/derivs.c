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

/* forward decls */

#define TV3 fprops_deriv_nonsat_dAdTv
#define VT3 fprops_deriv_nonsat_dAdvT
#define TV4 fprops_deriv_sat_dAdTv
#define VT4 fprops_deriv_sat_dAdvT

typedef struct{
	double T;
	double rho;
	double psat;
	double rhof;
	double rhog;
	double dpdT_sat;
	const HelmholtzData *D;
} StateData;

typedef double PartialDerivFn(FPROPS_CHAR Z, StateData *S);

#define TVSAT fprops_sat_dZdT_v
#define VTSAT fprops_sat_dZdv_T
#define TVNON fprops_non_dZdT_v
#define VTNON fprops_non_dZdv_T

PartialDerivFn TVSAT, VTSAT, TVNON, VTNON;

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
		ZTV = TVSAT(z,&S); ZVT = VTSAT(z,&S);
		XTV = TVSAT(x,&S); XVT = VTSAT(x,&S);
		YTV = TVSAT(y,&S); YVT = VTSAT(y,&S);
	}else{
		/* non-saturated */
		ZTV = TVNON(z,&S); ZVT = VTNON(z,&S);
		XTV = TVNON(x,&S); XVT = VTNON(x,&S);
		YTV = TVNON(y,&S); YVT = VTNON(y,&S);
	}

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

#define p helmholtz_p(S->T,S->rho,S->D)
#define cv helmholtz_cv(S->T,S->rho,S->D)
#define v (1./S->rho)
#define s helmholtz_s(S->T,S->rho,S->D)
#define alphap helmholtz_alphap(S->T,S->rho,S->D)
#define betap helmholtz_betap(S->T,S->rho,S->D)

double fprops_non_dZdv_T(FPROPS_CHAR x, StateData *S){
	double res;
	switch(x){
		case 'p': res = -p*betap; break;
		case 'T': res = 0; break;
		case 'v': res = 1; break;
		case 'u': res = p*(S->T*alphap-1.); break;
		case 'h': res = p*(S->T*alphap-v*betap); break;
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

double fprops_non_dZdT_v(FPROPS_CHAR x, StateData *S){
	double res;
	switch(x){
		case 'p': res = p*alphap; break;
		case 'T': res = 1; break;
		case 'v': res = 0; break;
		case 'u': res = cv; break;
		case 'h': res = cv + p*v*alphap; break;
		case 's': res = cv/S->T; break;
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

#if 0
/*------------------------------------------------------------------------------
  Saturation region derivatives... a bit harder.

FPROPS uses T,rho as coordinates, so we need to work out what these
derivatives are going to be within the saturation region.
*/

/*
	⎰ ∂z ⎱   =  ⎰∂z_f⎱ (1 - x) + ⎰∂z_g⎱ x
	⎱ ∂T ⎰v     ⎱ ∂T ⎰           ⎱ ∂T ⎰
*/
double fprops_deriv_sat_dZdT_v(FPROPS_CHAR z, StateData *S){
	double res;
	switch(z){
		/* a couple of easy cases, first: */
		case 'p': return S->dpdT_sat;
		case 'T': return 1.;
	}

	double drhofdT = fprops_drhofdT_T(S);
	double drhogdT = fprops_drhogdT_T(S);


	double dzfdT, dzgdT;

	assert(S->rhof!=0);
	assert(S->rhog!=0);
	double dvfdT = -1./SQ(rhof) * drhofdT_T;
	assert(!isnan(dvfdT));
	double dvgdT = -1./SQ(rhog) * drhogdT_T;
	assert(!isnan(dvgdT));

	dzfdT = VT3(z,T,rhof,D)*dvfdT + TV3(z,T,rhof,D);
	dzgdT = VT3(z,T,rhog,D)*dvgdT + TV3(z,T,rhog,D);

	assert(!isnan(dzfdT));
	assert(!isnan(dzgdT));
#define x S.R4.x
	res = dzfdT*(1-x) + dzgdT*x;
	//fprintf(stderr,"(∂%c/∂T)x = %g\n",z,res);
	return res;
#undef T
#undef x
}

/*
	These derivatives are simply the gradient within the two-phase region,
	and is very simply calculated as

	⎰ ∂z   ⎱   =  (z_g - z_f) / (rho_g - rho_f)  where z in {v,u,h,z,s,g,a}.
	⎱ ∂v ⎰T

	or, otherwise,

	⎰ ∂T ⎱  , ⎰ ∂p ⎱    = 0
	⎱ ∂x ⎰T   ⎱ ∂x ⎰T

*/	
double fprops_deriv_sat_dZdv_T(FPROPS_CHAR z, double T, double rho, const HelmholtzData *D){
	switch(z){
		case 'p': return 0;
		case 'T': return 0;
	}
	double p,rhof,rhog;
	int res = fprops_sat_T(T, &p, &rhof, &rhog, d);
	double zf, zg;
#define ZFG(Z,P,T) \
	zf = helmholtz_##Z##(T,rhof,d);\
	zg = helmholtz_##Z##(T,rhog,d)	switch(z){
		case 'v': ZFG(v,p,T); break;
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

double freesteam_drhofdT(StateData *S){
	double rho1 = S->rho;
	S->rho = S->rhof; /* set StateData to the saturated gas line */
	double dpdT = TVNON('p',S);
	double dpdrho = -1./SQ(rhof) * VTNON('p',S);
	S->rho = rho1; /* restore StateData */
	return (dpsatdT - dpdT)/dpdrho;
}

double freesteam_drhogdT(StateData *S){
	double rho1 = S->rho;
	S->rho = S->rhog; /* set StateData to the saturated gas line */
	double dpdT = TVNON('p',S);
	double dpdrho = -1./SQ(rhog) * VTNON('p',S);
	S->rho = rho1; /* restore StateData */
	return (dpsatdT - dpdT)/dpdrho;
}
#endif



