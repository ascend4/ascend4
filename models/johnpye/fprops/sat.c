/*	ASCEND modelling environment
	Copyright (C) 2008-2009 Carnegie Mellon University

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
	Routines to calculate saturation properties using Helmholtz correlation
	data. We first include some 'generic' saturation equations that make use
	of the acentric factor and critical point properties to predict saturation
	properties (pressure, vapour density, liquid density). These correlations
	seem to be only very rough in some cases, but it is hoped that they will
	be suitable as first-guess values that can then be fed into an iterative
	solver to converge to an accurate result.
*/

#include "rundata.h"
#include "sat.h"
#include "fprops.h"
#include "zeroin.h"

// report lots of stuff
//#define SAT_DEBUG
#define SAT_ERRORS

// assertions for NANs etc
//#define SAT_ASSERT

#ifdef SAT_ASSERT
# include <assert.h>
#else
# define assert(ARGS...)
#endif

#include <math.h>
#include <stdio.h>

#define SQ(X) ((X)*(X))

#define TCRIT(DATA) (DATA->data->T_c)
#define PCRIT(DATA) (DATA->data->p_c)
#define RHOCRIT(DATA) (DATA->data->rho_c)
#define OMEGA(DATA) (DATA->data->omega)
#define RGAS(DATA) (DATA->data->R)
#define TTRIP(DATA) (DATA->data->T_t)

#define THROW_FPE

#ifdef THROW_FPE
#define _GNU_SOURCE
#include <fenv.h>
int feenableexcept (int excepts);
int fedisableexcept (int excepts);
int fegetexcept (void);
#endif

# include "color.h"

#ifdef SAT_DEBUG
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

#ifdef SAT_ERRORS
# define ERRMSG(STR,...) \
	color_on(stderr,ASC_FG_BRIGHTRED);\
	fprintf(stderr,"ERROR:");\
	color_off(stderr);\
	fprintf(stderr," %s:%d:" STR "\n", __func__, __LINE__ ,##__VA_ARGS__)
#else
# define ERRMSG(ARGS...) ((void)0)
#endif

/**
	Estimate of saturation pressure using H W Xiang ''The new simple extended
	corresponding-states principle: vapor pressure and second virial
	coefficient'', Chemical Engineering Science,
	57 (2002) pp 1439-1449.

	This seems to be hopeless, or buggy. Tried with water at 373 K, gives
	525 kPa...
*/
double fprops_psat_T_xiang(double T, const FluidData *data){
	double Zc = data->p_c / (8314. * data->rho_c * data->T_c);

#ifdef TEST
	fprintf(stderr,"Zc = %f\n",Zc);
#endif

	double theta = SQ(Zc - 0.29);

#ifdef TEST
	fprintf(stderr,"theta = %f\n",theta);
#endif

	double aa[] = {5.790206, 4.888195, 33.91196
	            , 6.251894, 15.08591, -315.0248
	            , 11.65859, 46.78273, -1672.179
	};

	double a0 = aa[0] + aa[1]*data->omega + aa[2]*theta;
	double a1 = aa[3] + aa[4]*data->omega + aa[5]*theta;
	double a2 = aa[6] + aa[7]*data->omega + aa[8]*theta;

	double Tr = T / data->T_c;
	double tau = 1 - Tr;

	double taupow = pow(tau, 1.89);
	double temp = a0 + a1 * taupow + a2 * taupow * taupow * taupow;

	double logpr = temp * log(Tr);
	double p_r = exp(logpr);

#ifdef TEST
	fprintf(stderr,"a0 = %f\n", a0);
	fprintf(stderr,"a1 = %f\n", a1);
	fprintf(stderr,"a2 = %f\n", a2);
	fprintf(stderr,"temp = %f\n", temp);
	fprintf(stderr,"T_r = %f\n", Tr);
	fprintf(stderr,"p_r = %f\n", p_r);
#endif

	return p_r * data->p_c;
}

/**
	Estimate saturation pressure using acentric factor. This algorithm
	is used for first estimates for later refinement in the program REFPROP.

	This is derived from the definition of the acentric factor,
	omega = -log10(psat(T1) - 1 where T1/Tc = Tr = 0.7
	
	together with the saturation curve obtained if h_fg(T) is assumed constant:
	ln(psat(T)) = A - B/(T + C)

	See Sandler 5e, p 320.	

s	Such a curve will pass through (pc,Tc) and (psat(Tr),Tr) where Tr = 0.7,
	but will probably be inaccurate at Tt. Given additional data, such as the
	normal boiling point, a better equation like the Antoine equation can be
	fitted. But we don't use that here.

*/
double fprops_psat_T_acentric(double T, const FluidData *data){
	/* first guess using acentric factor */
	double p = data->p_c * pow(10, -7./3 * (1.+data->omega) * (data->T_c / T - 1.));
	return p;
}


/**
	Saturated liquid density correlation of Rackett, Spencer & Danner (1972)
	see http://dx.doi.org/10.1002/aic.690250412
*/
double fprops_rhof_T_rackett(double T, const FluidData *data){
	//MSG("RHOCRIT=%f, RGAS=%f, TCRIT=%f, PCRIT=%f",data->rho_c,data->R,data->T_c,data->p_c);
	double Zc = data->rho_c * data->R * data->T_c / data->p_c;
	double Tau = 1. - T/data->T_c;
	//MSG("Zc = %f, Tau = %f",Zc,Tau);
	double vf = (data->R * data->T_c / data->p_c) * pow(Zc, -1 - pow(Tau, 2./7));
	//MSG("got vf(T=%f) = %f",T,vf);
	return 1./vf;
}

/*
	TODO add Yamada & Gunn sat rhof equation eg from RPP5 eq 4-11.4a, should
	be more accurate?

	^Sean? I think you are referring to http://dx.doi.org/10.1002/aic.690170613,
	is that correct? That equation requires extra data for V_SC according to
	the paper. I don't have RPP5, only RPP4 unfortunately -- JP.
*/

/**
	Inverse of fprops_rhof_T_rackett. TODO: this need checking.
*/
double fprops_T_rhof_rackett(double rhof, const FluidData *data){
	double Zc = data->rho_c * data->R * data->T_c / data->p_c;
	double f1 = data->p_c / data->R / data->T_c / rhof;
	double f2 = -log(f1)/log(Zc);
	return pow(f2 -1, 3./2);
}


/**
	Saturated vapour density correlation of Chouaieb, Ghazouani, Bellagi
	see http://dx.doi.org/10.1016/j.tca.2004.05.017
*/
double fprops_rhog_T_chouaieb(double T, const FluidData *data){
	double Tau = 1. - T/data->T_c;
#if 0
	double Zc = RHOCRIT(d) * RGAS(d) * TCRIT(d) / PCRIT(d);
# define N1 -0.1497547
# define N2 0.6006565
# define P1 -19.348354
# define P2 -41.060325
# define P3 1.1878726
	double MMM = 2.6; /* guess, reading from Chouaieb, Fig 8 */
	//MMM = 2.4686277;
	double PPP = Zc / (P1 + P2*Zc*log(Zc) + P3/Zc);
	fprintf(stderr,"PPP = %f\n",PPP);
	//PPP = -0.6240188;
	double NNN = PPP + 1./(N1*D->omega + N2);
#else
/* exact values from Chouaieb for CO2 */
# define MMM 2.4686277
# define NNN 1.1345838
# define PPP -0.6240188
#endif

	double alpha = exp(pow(Tau,1./3) + sqrt(Tau) + Tau + pow(Tau, MMM));
	return data->rho_c * exp(PPP * (pow(alpha,NNN) - exp(1-alpha)));
}

void fprops_sat_T(double T, double *psat, double *rhof, double *rhog, const PureFluid *d, FpropsError *err){
	*psat = d->sat_fn(T,rhof,rhog,d->data,err);
}

/**
	Calculate the triple point pressure and densities using T_t from the FluidData.
*/
void fprops_triple_point(double *p_t_out, double *rhof_t_out, double *rhog_t_out, const PureFluid *d, FpropsError *err){
	static const PureFluid *d_last = NULL;
	static double p_t, rhof_t, rhog_t;
	if(d == d_last){
		*p_t_out = p_t;
		*rhof_t_out = rhof_t;
		*rhog_t_out = rhog_t;
		return;
	}

	if(d->data->T_t == 0){
		ERRMSG("Note: data for '%s' does not include a valid triple point temperature.",d->name);
	}

	MSG("Calculating for '%s' (type %d, T_t = %f, T_c = %f, p_c = %f)",d->name, d->type, d->data->T_t, d->data->T_c, d->data->p_c);
	fprops_sat_T(d->data->T_t, &p_t, &rhof_t, &rhog_t,d,err);
	if(*err)return;
	d_last = d;
	*p_t_out = p_t;
	*rhof_t_out = rhof_t;
	*rhog_t_out = rhog_t;
	MSG("p_t = %f, rhof_t = %f, rhog_t = %f", p_t, rhof_t, rhog_t);
}


typedef struct{
	const PureFluid *P;
	double p;
	FpropsError *err;
	double Terr;
} SatPResidData;

static ZeroInSubjectFunction sat_p_resid;
static double sat_p_resid(double T, void *user_data){
#define D ((SatPResidData *)user_data)
	double p, rhof, rhog;
	fprops_sat_T(T, &p, &rhof, &rhog, D->P, D->err);
	if(*(D->err))D->Terr = T;
	MSG("T = %f --> p = %f, rhof = %f, rhog = %f, RESID %f", T, p, rhof, rhog, (p - D->p));
	//if(*(D->err))MSG("Error: %s",fprops_error(*(D->err)));
	//if(*(D->err))return -1;
	return p - D->p;
#undef D
}


/**
	Solve saturation conditions as a function of pressure. 

	TODO Currently, we will just attempt a Brent solver (zeroin) but hopefully 
	we can do better later. In particular with cubic EOS this approach seems
	very inefficient. At the very least we should be able to manage a Newton
	solver...
*/	
void fprops_sat_p(double p, double *T_sat, double *rho_f, double *rho_g, const PureFluid *P, FpropsError *err){
	if(*err){
		MSG("ERROR FLAG ALREADY SET");
	}
	if(p == P->data->p_c){
		MSG("Requested pressure is critical point pressure, returning CP conditions");
		*T_sat = P->data->T_c;
		*rho_f = P->data->rho_c;
		*rho_g = P->data->rho_c;
		return;
	}
	/* FIXME what about checking triple point pressure? */
	

	SatPResidData D = {P, p, err, 0};
	MSG("Solving saturation conditions at p = %f", p);
	double p1, T, resid;
	int errn;
	double Tt = P->data->T_t;
	if(Tt == 0)Tt = 0.2* P->data->T_c;
	errn = zeroin_solve(&sat_p_resid, &D, Tt, P->data->T_c, 1e-5, &T, &resid);
	if(*err){
		MSG("FPROPS error within zeroin_solve iteration ('%s', p = %f, p_c = %f): %s"
			, P->name, p, P->data->p_c, fprops_error(*err)
		);
	}
	if(errn){
		ERRMSG("Failed to solve saturation at p = %f.",p);
		*err = FPROPS_SAT_CVGC_ERROR;
		return;
	}else{
		if(*err){
			ERRMSG("Ignoring error inside zeroin_solve iteration at T = %f",D.Terr);
		}
		*err = FPROPS_NO_ERROR;
	}
	fprops_sat_T(T, &p1, rho_f, rho_g, P, err);
	if(!*err)*T_sat = T;
	MSG("Got p1 = %f, p = %f", p1, p);
}


/**
	Calculate Tsat based on a value of hf. This value is useful in setting
	first guess Temperatures when solving for the coordinates (p,h).
	This function uses the secant method for the iterative solution.
*/
void fprops_sat_hf(double hf, double *Tsat_out, double *psat_out, double *rhof_out, double *rhog_out, const PureFluid *P, FpropsError *err){
	double T1 = 0.4 * P->data->T_t + 0.6 * P->data->T_c;
	double T2 = P->data->T_t;
	double h1, h2, p, rhof, rhog;
	fprops_sat_T(T2, &p, &rhof, &rhog, P, err);
	if(*err){
		ERRMSG("Failed to solve psat(T_t = %.12e) for %s",T2,P->name);
		return;
	}
	double tol = 1e-6;
	h2 = P->h_fn(T2,rhof,P->data, err);
	if(*err){
		ERRMSG("Unable to calculate h(T=%f K,rhof=%f kg/m3",T2,rhof);
	}
	if(hf < h2){
		ERRMSG("Value given for hf = %.12e is below that calculated for triple point liquid hf_t = %.12e",hf,h2);
		*err = FPROPS_RANGE_ERROR;
		return;
	}

	int i = 0;
	while(i++ < 60){
		assert(T1 >= P->data->T_t - 1e-4);
		assert(T1 <= P->data->T_c);
		MSG("T1 = %f\n",T1);
		fprops_sat_T(T1, &p, &rhof, &rhog, P, err);
		if(*err){
			ERRMSG("Failed to solve psat(T = %.12e) for %s",T1,P->name);
			return;
		}
		h1 = P->h_fn(T1,rhof, P->data, err);
		if(*err){
			ERRMSG("Unable to calculate h");
			return;
		}
		if(fabs(h1 - hf) < tol){
			*Tsat_out = T1;
			*psat_out = p;
			*rhof_out = rhof;
			*rhog_out = rhog;
			return; /* SUCCESS */
		}
		if(h1 == h2){
			MSG("With %s, got h1 = h2 = %.12e, but hf = %.12e!",P->name,h1,hf);
			*err = FPROPS_SAT_CVGC_ERROR;
			return;
		}

		double delta_T = -(h1 - hf) * (T1 - T2) / (h1 - h2);
		T2 = T1;
		h2 = h1;
		while(T1 + delta_T > P->data->T_c)delta_T *= 0.5;
		T1 += delta_T;
		if(T1 < P->data->T_t)T1 = P->data->T_t;
		if(i==20 || i==30)tol*=100;
	}
	fprintf(stderr,"Failed to solve Tsat for hf = %f (got to T = %f)\n",hf,T1);
	*Tsat_out = T1;
	*psat_out = p;
	*rhof_out = rhof;
	*rhog_out = rhog;
	*err = FPROPS_SAT_CVGC_ERROR;
}


