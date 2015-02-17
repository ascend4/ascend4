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
	Implementation of the reduced molar Helmholtz free energy equation of state.

	For nomenclature see Tillner-Roth, Harms-Watzenberg and Baehr, Eine neue
	Fundamentalgleichung für Ammoniak.

	John Pye, 29 Jul 2008.
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "helmholtz.h"
#include "ideal_impl.h"
#include "sat.h"
#include "cp0.h"
#include "refstate.h"



/* these are the 'raw' functions, they don't do phase equilibrium. */
PropEvalFn helmholtz_p;
PropEvalFn helmholtz_u;
PropEvalFn helmholtz_h;
PropEvalFn helmholtz_s;
PropEvalFn helmholtz_a;
PropEvalFn helmholtz_g;
PropEvalFn helmholtz_cp;
PropEvalFn helmholtz_cv;
PropEvalFn helmholtz_w;
PropEvalFn helmholtz_dpdrho_T;
PropEvalFn helmholtz_alphap;
PropEvalFn helmholtz_betap;
SatEvalFn helmholtz_sat;

double helmholtz_dpdT_rho(double T, double rho, const FluidData *data, FpropsError *err);
double helmholtz_d2pdrho2_T(double T, double rho, const FluidData *data, FpropsError *err);

double helmholtz_dhdT_rho(double T, double rho, const FluidData *data, FpropsError *err);
double helmholtz_dhdrho_T(double T, double rho, const FluidData *data, FpropsError *err);

double helmholtz_dudT_rho(double T, double rho, const FluidData *data, FpropsError *err);
double helmholtz_dudrho_T(double T, double rho, const FluidData *data, FpropsError *err);


//#define HELM_DEBUG
#define HELM_ERRORS
//#define SAT_DEBUG

#ifdef HELM_DEBUG
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
#ifdef HELM_ERRORS
# include "color.h"
# define ERRMSG(STR,...) \
	color_on(stderr,ASC_FG_BRIGHTRED);\
	fprintf(stderr,"ERROR:");\
	color_off(stderr);\
	fprintf(stderr," %s:%d:" STR "\n", __func__, __LINE__ ,##__VA_ARGS__)
#else
# define ERRMSG(ARGS...) ((void)0)
#endif


//#define ASSERT_DEBUG
#ifdef ASSERT_DEBUG
# include <assert.h>
#else
# define assert(ARGS...)
#endif

//#define RESID_DEBUG

#define INCLUDE_THIRD_DERIV_CODE

/* macros and forward decls */

#define SQ(X) ((X)*(X))

#include "helmholtz_impl.h"

/* shortcut take us straight into the correct data structure for Helmholtz correlation calculations */
#define HD data->corr.helm
#define HD_R data->R
#define HD_CP0 data->cp0

/* calculate tau and delta using a macro -- is used in most functions */
#define DEFINE_TD \
	double tau = data->corr.helm->T_star / T; \
	double delta = rho / data->corr.helm->rho_star


PureFluid *helmholtz_prepare(const EosData *E, const ReferenceState *ref){
	PureFluid *P = FPROPS_NEW(PureFluid);

	if(E->type != FPROPS_HELMHOLTZ){
		ERRMSG("invalid EOS data, wrong type");
		return NULL;
	}

	MSG("Fluid '%s' with T_t = %f", E->name, E->data.helm->T_t);

	P->data = FPROPS_NEW(FluidData);
	P->data->corr.helm = FPROPS_NEW(HelmholtzRunData);

	/* metadata */
	/* FIXME strings should be copied, not just referenced */
	P->name = E->name;
	P->source = E->source;
	P->type = E->type;
	MSG("name = %s",P->name);

	/* common data across all correlation types */
#define I E->data.helm
	P->data->M = I->M;
	if(I->R == 0){
		P->data->R = R_UNIVERSAL / I->M;
	}else{
		P->data->R = I->R;
	}
	P->data->T_t = I->T_t;
	P->data->T_c = I->T_c;
	P->data->p_c = 0; // we calculate this later...
	P->data->rho_c = I->rho_c;
	P->data->omega = I->omega;
	P->data->cp0 = cp0_prepare(E->data.helm->ideal, P->data->R, P->data->T_c);

	/* data specific to helmholtz correlations */
#define H P->data->corr.helm
	H->rho_star = I->rho_star;
	H->T_star = I->T_star;
	H->np = I->np;
	// FIXME copy et, ct, pt to runtime struct?, FIXME see helmholtz_destroy below.
	H->pt = I->pt;
	H->ng = I->ng;
	H->gt = I->gt;
	H->nc = I->nc;
	H->ct = I->ct;
	MSG("np = %d, ng = %d, nc = %d, T_t = %f",H->np,H->ng,H->nc,I->T_t);
#undef H

	/* function pointers... more to come still? */
#define FN(VAR) P->VAR##_fn = &helmholtz_##VAR
	FN(p); FN(u); FN(h); FN(s); FN(a); FN(g); FN(cp); FN(cv); FN(w);
	FN(alphap); FN(betap); FN(dpdrho_T);
	FN(sat);
#undef FN

	FpropsError err = 0;

	/* calculate critical pressure (doesn't require h0, s0) */
	MSG("Calculating critical pressure at T_c = %f K, rho_c = %f kg/m3",P->data->T_c, P->data->rho_c);
	P->data->p_c = helmholtz_p(P->data->T_c, P->data->rho_c, P->data, &err);
	if(err){
		ERRMSG("Failed to calculate critical pressure.");
		FPROPS_FREE(P->data);
		FPROPS_FREE(P->data->corr.helm);
		return NULL;
	}
	if(P->data->p_c <= 0){
		ERRMSG("Calculated a critical pressure <= 0! (value = %f)",P->data->p_c);
		//return NULL;
	}

	// fix up the reference point now...
	if(ref == NULL){
		// use the provided ReferenceState, or the default one otherwise.
		ref = &(I->ref);
	}
	int res = fprops_set_reference_state(P,ref);
	if(res){
		ERRMSG("Unable to apply reference state (type %d, err %d)",ref->type,res);
		return NULL;
	}

#undef I
	return P;
}

void helmholtz_destroy(PureFluid *P){
	assert(FPROPS_HELMHOLTZ == P->data);
	cp0_destroy(P->data->cp0);
	FPROPS_FREE(P->data->corr.helm);
	FPROPS_FREE(P->data);
	FPROPS_FREE(P);
}

/**
	Function to calculate pressure from Helmholtz free energy EOS, given temperature
	and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return pressure in Pa
*/
double helmholtz_p(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TD;

	assert(HD->rho_star!=0);
	assert(T!=0);
	assert(!isnan(T));
	assert(!isnan(rho));
	assert(!isnan(HD_R));

	//fprintf(stderr,"p calc: T = %f\n",T);
	//fprintf(stderr,"p calc: tau = %f\n",tau);
	//fprintf(stderr,"p calc: rho = %f\n",rho);
	//fprintf(stderr,"p calc: delta = %f\n",delta);
	//fprintf(stderr,"p calc: R*T*rho = %f\n",HD_R * T * rho);

	//fprintf(stderr,"T = %f\n", T);
	//fprintf(stderr,"rhob = %f, rhob* = %f, delta = %f\n", rho/HD->M, HD->rho_star/HD->M, delta);

	double p = HD_R * T * rho * (1 + delta * helm_resid_del(tau,delta,HD));
#if 0
	if(isnan(p)){
		fprintf(stderr,"T = %.12e, rho = %.12e\n",T,rho);
	}
#endif
	//abort();
	if(isnan(p))*err = FPROPS_NUMERIC_ERROR;
	return p;
}

/**
	Function to calculate internal energy from Helmholtz free energy EOS, given
	temperature	and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return internal energy in ???
*/
double helmholtz_u(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TD;

#ifdef TEST
	assert(HD->rho_star!=0);
	assert(T!=0);
	assert(!isnan(tau));
	assert(!isnan(delta));
	assert(!isnan(HD_R));
#endif

#if 0
	fprintf(stderr,"ideal_tau = %f\n",ideal_phi_tau(tau,delta,HD_CP0));
	fprintf(stderr,"resid_tau = %f\n",helm_resid_tau(tau,delta,HD));
	fprintf(stderr,"R T = %f\n",HD_R * HD->T_star);
#endif

	return HD_R * HD->T_star * (ideal_phi_tau(tau,delta,HD_CP0) + helm_resid_tau(tau,delta,HD));
}

/**
	Function to calculate enthalpy from Helmholtz free energy EOS, given
	temperature	and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return enthalpy in J/kg
*/
double helmholtz_h(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TD;

//#ifdef TEST
	assert(HD->rho_star!=0);
	assert(T!=0);
	assert(!isnan(tau));
	assert(!isnan(delta));
	assert(!isnan(HD_R));
//#endif
	double h = HD_R * T * (1 + tau * (ideal_phi_tau(tau,delta,HD_CP0) + helm_resid_tau(tau,delta,HD)) \
		+ delta*helm_resid_del(tau,delta,HD));
	assert(!isnan(h));
	return h;
}

/**
	Function to calculate entropy from Helmholtz free energy EOS, given
	temperature	and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return entropy in J/kgK
*/
double helmholtz_s(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TD;

#ifdef ENTROPY_DEBUG
	assert(HD->rho_star!=0);
	assert(T!=0);
	assert(!isnan(tau));
	assert(!isnan(delta));
	assert(!isnan(HD_R));

	fprintf(stderr,"ideal_phi_tau = %f\n",ideal_phi_tau(tau,delta,HD_CP0));
	fprintf(stderr,"helm_resid_tau = %f\n",helm_resid_tau(tau,delta,HD));
	fprintf(stderr,"ideal_phi = %f\n",ideal_phi(tau,delta,HD_CP0));
	fprintf(stderr,"helm_resid = %f\n",helm_resid(tau,delta,HD));
#endif
	return HD_R * (
		tau * (ideal_phi_tau(tau,delta,HD_CP0) + helm_resid_tau(tau,delta,HD))
		- (ideal_phi(tau,delta,HD_CP0) + helm_resid(tau,delta,HD))
	);
}

/**
	Function to calculate Helmholtz energy from the Helmholtz free energy EOS,
	given temperature and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return Helmholtz energy 'a', in J/kg
*/
double helmholtz_a(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TD;

#ifdef TEST
	assert(HD->rho_star!=0);
	assert(T!=0);
	assert(!isnan(tau));
	assert(!isnan(delta));
	assert(!isnan(HD_R));
#endif

#ifdef HELMHOLTZ_DEBUG
	fprintf(stderr,"helmholtz_a: T = %f, rho = %f\n",T,rho);
	fprintf(stderr,"multiplying by RT = %f\n",HD_R*T);
#endif

	return HD_R * T * (ideal_phi(tau,delta,HD_CP0) + helm_resid(tau,delta,HD));
}

/**
	Function to calculate isochoric heat capacity from the Helmholtz free energy
	EOS given temperature and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return Isochoric specific heat capacity in J/kg/K.
*/
double helmholtz_cv(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TD;

	return - HD_R * SQ(tau) * (ideal_phi_tautau(tau,HD_CP0) + helm_resid_tautau(tau,delta,HD));
}

/**
	Function to calculate isobaric heat capacity from the Helmholtz free energy
	EOS given temperature and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return Isobaric specific heat capacity in J/kg/K.
*/
double helmholtz_cp(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TD;

	double phir_d = helm_resid_del(tau,delta,HD);
	double phir_dd = helm_resid_deldel(tau,delta,HD);
	double phir_dt = helm_resid_deltau(tau,delta,HD);

	/* note similarities with helmholtz_w */
	double temp1 = 1 + 2*delta*phir_d + SQ(delta)*phir_dd;
	double temp2 = 1 + delta*phir_d - delta*tau*phir_dt;
	double temp3 = -SQ(tau)*(ideal_phi_tautau(tau,HD_CP0) + helm_resid_tautau(tau,delta,HD));

	return HD_R * (temp3 + SQ(temp2)/temp1);
}


/**
	Function to calculate the speed of sound in a fluid from the Helmholtz free
	energy EOS, given temperature and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return Speed of sound in m/s.
*/
double helmholtz_w(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TD;

	double phir_d = helm_resid_del(tau,delta,HD);
	double phir_dd = helm_resid_deldel(tau,delta,HD);
	double phir_dt = helm_resid_deltau(tau,delta,HD);

	/* note similarities with helmholtz_cp */
	double temp1 = 1. + 2.*delta*phir_d + SQ(delta)*phir_dd;
	double temp2 = 1. + delta*phir_d - delta*tau*phir_dt;
	double temp3 = -SQ(tau)*(ideal_phi_tautau(tau,HD_CP0) + helm_resid_tautau(tau,delta,HD));

	return sqrt(HD_R * T * (temp1 + SQ(temp2)/temp3));

}

/**
	Function to calculate the Gibbs energy fluid from the Helmholtz free
	energy EOS, given temperature and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return Gibbs energy, in J/kg.
*/
double helmholtz_g(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TD;

	double phir_d = helm_resid_del(tau,delta,HD);
	double phir = helm_resid(tau,delta,HD);
	double phi0 = ideal_phi(tau,delta,HD_CP0);

	return HD_R * T * (phi0 + phir + 1. + delta * phir_d);
}

/**
	alpha_p function from IAPWS Advisory Note 3, used in calculation of
	partial property derivatives.
*/
double helmholtz_alphap(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TD;
	double phir_d = helm_resid_del(tau,delta,HD);
	double phir_dt = helm_resid_deltau(tau,delta,HD);
	return 1./T * (1. - delta*tau*phir_dt/(1 + delta*phir_d));
}

/**
	beta_p function from IAPWS Advisory Note 3 , used in calculation of partial
	property derivatives.
*/
double helmholtz_betap(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TD;
	double phir_d = helm_resid_del(tau,delta,HD);
	double phir_dd = helm_resid_deldel(tau,delta,HD);
	return rho*(1. + (delta*phir_d + SQ(delta)*phir_dd)/(1+delta*phir_d));
}

/*----------------------------------------------------------------------------
  PARTIAL DERIVATIVES
*/

/**
	Calculate partial derivative of p with respect to T, with rho constant
*/
double helmholtz_dpdT_rho(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TD;

	double phir_del = helm_resid_del(tau,delta,HD);
	double phir_deltau = helm_resid_deltau(tau,delta,HD);
#ifdef TEST
	assert(!isinf(phir_del));
	assert(!isinf(phir_deltau));
	assert(!isnan(phir_del));
	assert(!isnan(phir_deltau));
	assert(!isnan(HD_R));
	assert(!isnan(rho));
	assert(!isnan(tau));
#endif

	double res = HD_R * rho * (1 + delta*phir_del - delta*tau*phir_deltau);

#ifdef TEST
	assert(!isnan(res));
	assert(!isinf(res));
#endif
	return res;
}

/**
	Calculate partial derivative of p with respect to rho, with T constant
*/
double helmholtz_dpdrho_T(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TD;
	//MSG("...");
	double phir_del = helm_resid_del(tau,delta,HD);
	double phir_deldel = helm_resid_deldel(tau,delta,HD);
#ifdef TEST
	assert(!isinf(phir_del));
	assert(!isinf(phir_deldel));
#endif
	return HD_R * T * (1 + 2*delta*phir_del + SQ(delta)*phir_deldel);
}


double helmholtz_d2pdrho2_T(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TD;

	double phir_del = helm_resid_del(tau,delta,HD);
	double phir_deldel = helm_resid_deldel(tau,delta,HD);
	double phir_deldeldel = helm_resid_deldeldel(tau,delta,HD);
#ifdef TEST
	assert(!isinf(phir_del));
	assert(!isinf(phir_deldel));
	assert(!isinf(phir_deldeldel));
#endif

	return HD_R * T / rho * delta * (2*phir_del + delta*(4*phir_deldel + delta*phir_deldeldel));
}

/**
	Calculate partial derivative of h with respect to T, with rho constant
*/
double helmholtz_dhdT_rho(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TD;

	double phir_del = helm_resid_del(tau,delta,HD);
	double phir_deltau = helm_resid_deltau(tau,delta,HD);
	double phir_tautau = helm_resid_tautau(tau,delta,HD);
	double phi0_tautau = ideal_phi_tautau(tau,HD_CP0);

	//fprintf(stderr,"phir_del = %f, phir_deltau = %f, phir_tautau = %f, phi0_tautau = %f\n",phir_del,phir_deltau,phir_tautau,phi0_tautau);

	//return (helmholtz_h(T+0.01,rho,data) - helmholtz_h(T,rho,data)) / 0.01;
	return HD_R * (1. + delta*phir_del - SQ(tau)*(phi0_tautau + phir_tautau) - delta*tau*phir_deltau);
}

/**
	Calculate partial derivative of h with respect to rho, with T constant
*/
double helmholtz_dhdrho_T(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TD;

	double phir_del = helm_resid_del(tau,delta,HD);
	double phir_deltau = helm_resid_deltau(tau,delta,HD);
	double phir_deldel = helm_resid_deldel(tau,delta,HD);

	return HD_R * T / rho * (tau*delta*(0 + phir_deltau) + delta * phir_del + SQ(delta)*phir_deldel);
}


/**
	Calculate partial derivative of u with respect to T, with rho constant
*/
double helmholtz_dudT_rho(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TD;

	double phir_tautau = helm_resid_tautau(tau,delta,HD);
	double phi0_tautau = ideal_phi_tautau(tau,HD_CP0);

	return -HD_R * SQ(tau) * (phi0_tautau + phir_tautau);
}


/**
	Calculate partial derivative of u with respect to rho, with T constant
*/
double helmholtz_dudrho_T(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TD;

	double phir_deltau = helm_resid_deltau(tau,delta,HD);

	return HD_R * T / rho * (tau * delta * phir_deltau);
}


/**
	Solve saturation condition for a specified temperature using approach of
	Akasaka, but adapted for general use to non-helmholtz property correlations.
	@param T temperature [K]
	@param psat_out output, saturation pressure [Pa]
	@param rhof_out output, saturated liquid density [kg/m^3]
	@param rhog_out output, saturated vapour density [kg/m^3]
	@param d helmholtz data object for the fluid in question.
	@return 0 on success, non-zero on error (eg algorithm failed to converge, T out of range, etc.)
*/
double helmholtz_sat(double T, double *rhof_out, double * rhog_out, const FluidData *data, FpropsError *err){
	if(T < data->T_t - 1e-8){
		ERRMSG("Input temperature %f K is below triple-point temperature %f K",T,data->T_t);
		return FPROPS_RANGE_ERROR;
	}

	if(T > data->T_c + 1e-8){
		ERRMSG("Input temperature is above critical point temperature");
		*err = FPROPS_RANGE_ERROR;
	}

	// we're at the critical point
	if(fabs(T - data->T_c) < 1e-9){
		*rhof_out = data->rho_c;
		*rhog_out = data->rho_c;
		return data->p_c;
	}

	// FIXME at present step-length multiplier is set to 0.4 just because of 
	// ONE FLUID, ethanol. Probably our initial guess data isn't good enough,
	// or maybe there's a problem with the acentric factor or something like
	// that. This factor 0.4 will be slowing down the whole system, so it's not
	// good. TODO XXX.

	// initial guesses for liquid and vapour density
	double rhof = 1.1 * fprops_rhof_T_rackett(T,data);
	double rhog= 0.9 * fprops_rhog_T_chouaieb(T,data);
	double R = data->R;
	double pc = data->p_c;

#ifdef SAT_DEBUG
	MSG("initial guess rho_f = %f, rho_g = %f",rhof,rhog);
	MSG("calculating at T = %.12e",T);
#endif

	int i = 0;
	while(i++ < 200){
		assert(!isnan(rhog));
		assert(!isnan(rhof));
#ifdef SAT_DEBUG
		MSG("iter %d: T = %f, rhof = %f, rhog = %f",i,T, rhof, rhog);
#endif

		double pf = helmholtz_p(T,rhof,data,err);
		double pg = helmholtz_p(T,rhog,data,err);
		double gf = helmholtz_a(T,rhof,data,err) + pf/rhof;
		double gg = helmholtz_a(T,rhog,data,err) + pg/rhog;
		double dpdrf = helmholtz_dpdrho_T(T,rhof,data,err);
		double dpdrg = helmholtz_dpdrho_T(T,rhog,data,err);

		// jacobian for [F;G](rhof, rhog) --- derivatives wrt rhof and rhog
		double F = (pf - pg)/pc;
		double G = (gf - gg)/R/T;

		if(fabs(F) + fabs(G) < 1e-12){
			//fprintf(stderr,"%s: CONVERGED\n",__func__);
			*rhof_out = rhof;
			*rhog_out = rhog;
			return helmholtz_p(T, *rhog_out, data, err);
			/* SUCCESS */
		}

		double Ff = dpdrf/pc;
		double Fg = -dpdrg/pc;
		//MSG("Ff = %e, Fg = %e",Ff,Fg);

		double Gf = dpdrf/rhof/R/T;
		double Gg = -dpdrg/rhog/R/T;
		//MSG("Gf = %e, Gg = %e",Gf,Gg);

		double DET = Ff*Gg - Fg*Gf;
		//MSG("DET = %f",DET);

		// 'gamma' needs to be increased to 0.5 for water to solve correctly (see 'test/sat.c')
		// 'gamma' needs to be not more than 0.4 for ethanol to solve correctly (see 'test/sat.c')
#define gamma 0.40
		rhof += gamma/DET * (Fg*G - Gg*F);
		rhog += gamma/DET * ( Gf*F - Ff*G);
#undef gamma

		assert(!isnan(rhof));
		assert(!isnan(rhog));

		if(rhog < 0)rhog = -0.5*rhog;
		if(rhof < 0)rhof = -0.5*rhof;
	}
	*rhof_out = rhof;
	*rhog_out = rhog;
	*err = FPROPS_SAT_CVGC_ERROR;
	ERRMSG("Not converged: with T = %e (rhof=%f, rhog=%f).",T,*rhof_out,*rhog_out);
	return helmholtz_p(T, rhog, data, err);
}




/*---------------------------------------------
  UTILITY FUNCTION(S)
*/

/* ipow:  public domain by Mark Stephen with suggestions by Keiichi Nakasato */
static double ipow(double x, int n){
	double t = 1.0;

	if(!n)return 1.0;    /* At the top. x^0 = 1 */

	if (n < 0){
		n = -n;
		x = 1.0/x;  /* error if x == 0. Good                        */
	}                 /* ZTC/SC returns inf, which is even better     */

	if (x == 0.0)return 0.0;

	do{
		if(n & 1)t *= x;
		n /= 2;     /* KN prefers if (n/=2) x*=x; This avoids an    */
		x *= x;     /* unnecessary but benign multiplication on     */
	}while(n);      /* the last pass, but the comparison is always
					   true _except_ on the last pass. */

	return t;
}

/* maxima expressions:
	Psi(delta) := exp(-C*(delta-1)^2 -D*(tau-1)^2);
	theta(delta) := (1-tau) + A*((delta-1)^2)^(1/(2*beta));
	Delta(delta):= theta(delta)^2 + B*((delta-1)^2)^a;
	n*Delta(delta)^b*delta*Psi(delta);
	diff(%,delta,3);
	yikes, that's scary! break down into steps.
*/

#undef HD

/*
	We avoid duplication by using the following #defines for common code in
	calculation of critical terms.
*/
#define DEFINE_DELTA \
		double d1 = delta - 1.; \
		double t1 = tau - 1.; \
		double d12 = SQ(d1); \
		double theta = (1. - tau) + ct->A * pow(d12, 0.5/ct->beta); \
		double PSI = exp(-ct->C*d12 - ct->D*SQ(t1)); \
		double DELTA = SQ(theta) + ct->B* pow(d12, ct->a)

#define DEFINE_DELB \
		double DELB = pow(DELTA,ct->b)

#define DEFINE_DPSIDDELTA \
		double dPSIddelta = -2. * ct->C * d1 * PSI

#define DEFINE_DDELDDELTA \
		double dDELddelta = d1 * (ct->A * theta * 2./ct->beta * pow(d12, 0.5/ct->beta - 1) + 2* ct->B * ct->a * pow(d12, ct->a - 1))

#define DEFINE_DDELBDTAU \
		double dDELbdtau = (DELTA == 0) ? 0 : -2. * theta * ct->b * (DELB/DELTA);\
		assert(!__isnan(dDELbdtau))

#define DEFINE_DPSIDTAU \
		double dPSIdtau = -2. * ct->D * t1 * PSI

#define DEFINE_DDELBDDELTA \
		double dDELbddelta = (DELTA==0?0:ct->b * (DELB/DELTA) * dDELddelta)

#define DEFINE_D2DELDDELTA2 \
		double powd12bm1 = pow(d12,0.5/ct->beta-1.); \
		double d2DELddelta2 = 1./d1*dDELddelta + d12*( \
			4.*ct->B*ct->a*(ct->a-1.)*pow(d12,ct->a-2.) \
			+ 2.*SQ(ct->A)*SQ(1./ct->beta)*SQ(powd12bm1) \
			+ ct->A*theta*4./ct->beta*(0.5/ct->beta-1.)*powd12bm1/d12 \
		)

#define DEFINE_D2DELBDDELTA2 \
		double d2DELbddelta2 = ct->b * ( (DELB/DELTA)*d2DELddelta2 + (ct->b-1.)*(DELB/SQ(DELTA)*SQ(dDELddelta)))

#define DEFINE_D2PSIDDELTA2 \
		double d2PSIddelta2 = (2.*ct->C*d12 - 1.)*2.*ct->C * PSI

#define DEFINE_D3PSIDDELTA3 \
	double d3PSIddelta3 = -4. * d1 * SQ(ct->C) * (2.*d12*ct->C - 3.) * PSI

#define DEFINE_D3DELDDELTA3 \
	double d3DELddelta3 = 1./(d1*d12*ct->beta*SQ(ct->beta)) * (\
		4*ct->B*ct->a*(1.+ct->a*(2*ct->a-3))*SQ(ct->beta)*pow(d12,ct->a)\
		+ ct->A * (1.+ct->beta*(2*ct->beta-3))*pow(d12,0.5/ct->beta)\
	)

#define DEFINE_D3DELBDDELTA3 \
	double d3DELbddelta3 = ct->b / (DELTA*SQ(DELTA)) * ( \
		(2+ct->b*(ct->b-3))*dDELddelta*SQ(dDELddelta)*DELB \
		+ DELB*SQ(DELTA)*d3DELddelta3 \
		+ 3*(ct->b-1) * DELB * DELTA * dDELddelta * d2DELddelta2 \
	)

/**
	Residual part of helmholtz function.
*/
double helm_resid(double tau, double delta, const HelmholtzRunData *HD){
	double dell, term, sum, res = 0;
	unsigned n, i;
	const HelmholtzPowTerm *pt;
	const HelmholtzGausTerm *gt;
	const HelmholtzCritTerm *ct;

	n = HD->np;
	pt = &(HD->pt[0]);

	//MSG("tau=%f, del=%f",tau,delta);
	//if(isinf(tau))abort();

	/* power terms */
	sum = 0;
	dell = ipow(delta,pt->l);
	//ldell = pt->l * dell;
	unsigned oldl;
	for(i=0; i<n; ++i){
		term = pt->a * pow(tau, pt->t) * ipow(delta, pt->d);
		sum += term;
#ifdef RESID_DEBUG
		MSG("i = %d,               a=%e, t=%f, d=%d, l=%u, term = %f, sum = %f",i,pt->a,pt->t,pt->d,pt->l,term,sum);
		if(pt->l==0){
			MSG(",row=%e\n",term);
		}else{
			MSG(",row=%e\n",term*exp(-dell));
		}
#endif
		oldl = pt->l;
		++pt;
		if(i+1==n || oldl != pt->l){
			if(oldl == 0){
#ifdef RESID_DEBUG
				MSG("                      linear ");
#endif
				res += sum;
			}else{
#ifdef RESID_DEBUG
				MSG("                      %sEXP dell=%f, exp(-dell)=%f sum=%f: ",(i+1==n?"LAST ":""),dell,exp(-dell),sum);
#endif
				res += sum * exp(-dell);
			}
#ifdef RESID_DEBUG
			MSG("i = %d, res = %f\n",i,res);
#endif
			sum = 0;
			if(i+1<n){
#ifdef RESID_DEBUG
				MSG("                      next delta = %.12e, l = %u\n",delta, pt->l);
#endif
				dell = (delta==0 ? 0 : ipow(delta,pt->l));
				//ldell = pt->l*dell;
			}
		}
	}
	assert(!__isnan(res));

	/* gaussian terms */
	n = HD->ng;
	//fprintf(stderr,"THERE ARE %d GAUSSIAN TERMS\n",n);
	gt = &(HD->gt[0]);
	for(i=0; i<n; ++i){
#ifdef RESID_DEBUG
		MSG("i = %d, GAUSSIAN, n = %e, t = %f, d = %f, alpha = %f, beta = %f, gamma = %f, epsilon = %f",i+1, gt->n, gt->t, gt->d, gt->alpha, gt->beta, gt->gamma, gt->epsilon);
#endif
		double d1 = delta - gt->epsilon;
		double t1 = tau - gt->gamma;
		double e1 = -gt->alpha*SQ(d1) - gt->beta*SQ(t1);
		sum = gt->n * pow(tau,gt->t) * pow(delta,gt->d) * exp(e1);
		//fprintf(stderr,"sum = %f\n",sum);
		res += sum;
		++gt;
	}
	assert(!__isnan(res));

	/* critical terms */
	n = HD->nc;
	ct = &(HD->ct[0]);
	for(i=0; i<n; ++i){
#ifdef RESID_DEBUG
		MSG("i = %d, CRITICAL, n = %e, a = %f, b = %f, beta = %f, A = %f, B = %f, C = %f, D = %f\n",i+1, ct->n, ct->a, ct->b, ct->beta, ct->A, ct->B, ct->C, ct->D);
#endif

		DEFINE_DELTA;
		DEFINE_DELB;

		sum = ct->n * DELB * delta * PSI;
		res += sum;
		++ct;
	}
	assert(!__isnan(res));

#ifdef RESID_DEBUG
	fprintf(stderr,"CALCULATED RESULT FOR phir = %f\n",res);
#endif
	return res;
}

/*=================== FIRST DERIVATIVES =======================*/

/**
	Derivative of the helmholtz residual function with respect to
	delta.
*/
double helm_resid_del(double tau,double delta, const HelmholtzRunData *HD){
	double sum = 0, res = 0;
	double dell, ldell;
	unsigned n, i;
	const HelmholtzPowTerm *pt;
	const HelmholtzGausTerm *gt;
	const HelmholtzCritTerm *ct;

#ifdef RESID_DEBUG
		fprintf(stderr,"tau=%f, del=%f\n",tau,delta);
#endif

	/* power terms */
	n = HD->np;
	pt = &(HD->pt[0]);
	dell = ipow(delta,pt->l);
	ldell = pt->l * dell;
	unsigned oldl;
	for(i=0; i<n; ++i){
		sum += pt->a * pow(tau, pt->t) * ipow(delta, pt->d - 1) * (pt->d - ldell);
		oldl = pt->l;
		++pt;
		if(i+1==n || oldl != pt->l){
			if(oldl == 0){
				res += sum;
			}else{
				res += sum * exp(-dell);
			}
			sum = 0;
			if(i+1<n){
				dell = (delta==0 ? 0 : ipow(delta,pt->l));
				ldell = pt->l*dell;
			}
		}
	}

	/* gaussian terms */
	n = HD->ng;
	gt = &(HD->gt[0]);
	for(i=0; i<n; ++i){
#ifdef RESID_DEBUG
		fprintf(stderr,"i = %d, GAUSSIAN, n = %e, t = %f, d = %f, alpha = %f, beta = %f, gamma = %f, epsilon = %f\n",i+1, gt->n, gt->t, gt->d, gt->alpha, gt->beta, gt->gamma, gt->epsilon);
#endif
		sum = - gt->n * pow(tau,gt->t) * pow(delta, -1. + gt->d)
			* (2. * gt->alpha * delta * (delta - gt->epsilon) - gt->d)
			* exp(-(gt->alpha * SQ(delta-gt->epsilon) + gt->beta*SQ(tau-gt->gamma)));
		res += sum;
#ifdef RESID_DEBUG
		fprintf(stderr,"sum = %f --> res = %f\n",sum,res);
#endif
		++gt;
	}

	/* critical terms */
	n = HD->nc;
	ct = &(HD->ct[0]);
	for(i=0; i<n; ++i){
#ifdef RESID_DEBUG
		fprintf(stderr,"i = %d, CRITICAL, n = %e, a = %f, b = %f, beta = %f, A = %f, B = %f, C = %f, D = %f\n",i+1, ct->n, ct->a, ct->b, ct->beta, ct->A, ct->B, ct->C, ct->D);
#endif
		DEFINE_DELTA;
		DEFINE_DELB;
		DEFINE_DPSIDDELTA;
		DEFINE_DDELDDELTA;
		DEFINE_DDELBDDELTA;

#if 0
		if(fabs(dpsiddelta) ==0)fprintf(stderr,"WARNING: dpsiddelta == 0\n");
		if(fabs(dpsiddelta) ==0)fprintf(stderr,"WARNING: dpsiddelta == 0\n");
		fprintf(stderr,"psi = %f\n",psi);
		fprintf(stderr,"DELTA = %f\n",DELTA);

		fprintf(stderr,"dDELddelta = %f\n",dDELddelta);
		fprintf(stderr,"ct->b - 1. = %f\n",ct->b - 1.);
		fprintf(stderr,"pow(DELTA,ct->b - 1.) = %f\n",pow(DELTA,ct->b - 1.));
		assert(!isnan(pow(DELTA,ct->b - 1.)));
		assert(!isnan(dDELddelta));
		assert(!isnan(dDELbddelta));
//double dDELbddelta = ct->b * pow(DELTA,ct->b - 1.) * dDELddelta
		fprintf(stderr,"sum = %f\n",sum);
		if(isnan(sum))fprintf(stderr,"ERROR: sum isnan with i=%d at %d\n",i,__LINE__);
#endif
		sum = ct->n * (DELB * (PSI + delta * dPSIddelta) + dDELbddelta * delta * PSI);
		res += sum;

		++ct;
	}

	return res;
}

/**
	Derivative of the helmholtz residual function with respect to
	tau.
*/
double helm_resid_tau(double tau,double delta,const HelmholtzRunData *HD){

	double sum;
	double res = 0;
	double delX;
	unsigned l;
	unsigned n, i;
	const HelmholtzPowTerm *pt;
	const HelmholtzGausTerm *gt;
	const HelmholtzCritTerm *ct;

	n = HD->np;
	pt = &(HD->pt[0]);

	delX = 1;

	l = 0;
	sum = 0;
	for(i=0; i<n; ++i){
		if(pt->t){
			//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d, l = %d\n",i+1, pt->a, pt->t, pt->d, pt->l);
			sum += pt->a * pow(tau, pt->t - 1) * ipow(delta, pt->d) * pt->t;
		}
		++pt;
		//fprintf(stderr,"l = %d\n",l);
		if(i+1==n || l != pt->l){
			if(l==0){
				//fprintf(stderr,"Adding non-exp term\n");
				res += sum;
			}else{
				//fprintf(stderr,"Adding exp term with l = %d, delX = %e\n",l,delX);
				res += sum * exp(-delX);
			}
			/* set l to new value */
			if(i+1!=n){
				l = pt->l;
				//fprintf(stderr,"New l = %d\n",l);
				delX = ipow(delta,l);
				sum = 0;
			}
		}
	}
	assert(!__isnan(res));

//#define RESID_DEBUG
	/* gaussian terms */
	n = HD->ng;
	gt = &(HD->gt[0]);
	for(i=0; i<n; ++i){
#ifdef RESID_DEBUG
		fprintf(stderr,"i = %d, GAUSSIAN, n = %e, t = %f, d = %f, alpha = %f, beta = %f, gamma = %f, epsilon = %f\n",i+1, gt->n, gt->t, gt->d, gt->alpha, gt->beta, gt->gamma, gt->epsilon);
#endif

		double val2;
		val2 = -gt->n * pow(tau,gt->t - 1.) * pow(delta, gt->d)
			* (2. * gt->beta * tau * (tau - gt->gamma) - gt->t)
			* exp(-(gt->alpha * SQ(delta-gt->epsilon) + gt->beta*SQ(tau-gt->gamma)));
		res += val2;
#ifdef RESID_DEBUG
		fprintf(stderr,"res = %f\n",res);
#endif

		++gt;
	}
	assert(!__isnan(res));

	/* critical terms */
	n = HD->nc;
	ct = &(HD->ct[0]);
	for(i=0; i<n; ++i){
#ifdef RESID_DEBUG
		fprintf(stderr,"i = %d, CRITICAL, n = %e, a = %f, b = %f, beta = %f, A = %f, B = %f, C = %f, D = %f\n",i+1, ct->n, ct->a, ct->b, ct->beta, ct->A, ct->B, ct->C, ct->D);
#endif
		DEFINE_DELTA;
		DEFINE_DELB;
		DEFINE_DDELBDTAU;
		DEFINE_DPSIDTAU;

		sum = ct->n * delta * (dDELbdtau * PSI + DELB * dPSIdtau);
		res += sum;
		++ct;
	}
	assert(!__isnan(res));

	return res;
}


/*=================== SECOND DERIVATIVES =======================*/

/**
	Mixed derivative of the helmholtz residual function with respect to
	delta and tau.
*/
double helm_resid_deltau(double tau,double delta,const HelmholtzRunData *HD){
	double dell,ldell, sum = 0, res = 0;
	unsigned n, i;
	const HelmholtzPowTerm *pt;
	const HelmholtzGausTerm *gt;
	const HelmholtzCritTerm *ct;

	/* power terms */
	n = HD->np;
	pt = &(HD->pt[0]);
	dell = ipow(delta,pt->l);
	ldell = pt->l * dell;
	unsigned oldl;
	for(i=0; i<n; ++i){
		sum += pt->a * pt->t * pow(tau, pt->t - 1) * ipow(delta, pt->d - 1) * (pt->d - ldell);
		oldl = pt->l;
		++pt;
		if(i+1==n || oldl != pt->l){
			if(oldl == 0){
				res += sum;
			}else{
				res += sum * exp(-dell);
			}
			sum = 0;
			if(i+1<n){
				dell = ipow(delta,pt->l);
				ldell = pt->l*dell;
			}
		}
	}

#ifdef TEST
	assert(!isinf(res));
#endif

	/* gaussian terms */
	n = HD->ng;
	gt = &(HD->gt[0]);
	for(i=0; i<n; ++i){
#ifdef RESID_DEBUG
		fprintf(stderr,"i = %d, GAUSSIAN, n = %e, t = %f, d = %f, alpha = %f, beta = %f, gamma = %f, epsilon = %f\n",i+1, gt->n, gt->t, gt->d, gt->alpha, gt->beta, gt->gamma, gt->epsilon);
#endif
		double d1 = delta - gt->epsilon;
		double t1 = tau - gt->gamma;
		double e1 = -gt->alpha*SQ(d1) - gt->beta*SQ(t1);

		double f1 = gt->t - 2*gt->beta*tau*(tau - gt->gamma);
		double g1 = gt->d - 2*gt->alpha*delta*(delta - gt->epsilon);

		sum = gt->n * f1 * pow(tau,gt->t-1) * g1 * pow(delta,gt->d-1) * exp(e1);

		//fprintf(stderr,"sum = %f\n",sum);
		res += sum;
#ifdef TEST
		assert(!isinf(res));
#endif
		++gt;
	}

	/* critical terms */
	n = HD->nc;
	ct = &(HD->ct[0]);
	for(i=0; i<n; ++i){
#ifdef RESID_DEBUG
		fprintf(stderr,"i = %d, CRITICAL, n = %e, a = %f, b = %f, beta = %f, A = %f, B = %f, C = %f, D = %f\n",i+1, ct->n, ct->a, ct->b, ct->beta, ct->A, ct->B, ct->C, ct->D);
#endif
		DEFINE_DELTA;
		DEFINE_DELB;
		DEFINE_DPSIDDELTA;
		DEFINE_DDELBDTAU;
		DEFINE_DDELDDELTA;

		double d2DELbddeldtau = -ct->A * ct->b * 2./ct->beta * (DELB/DELTA)*d1*pow(d12,0.5/ct->beta-1) \
			- 2. * theta * ct->b * (ct->b - 1) * (DELB/SQ(DELTA)) * dDELddelta;

		double d2PSIddeldtau = 4. * ct->C*ct->D*d1*t1*PSI;

		DEFINE_DPSIDTAU;

		sum = ct->n * (DELB * (dPSIdtau + delta * d2PSIddeldtau) \
			+ delta *dDELbdtau*dPSIdtau \
			+ dDELbdtau*(PSI+delta*dPSIddelta) \
			+ d2DELbddeldtau*delta*PSI
		);
		res += sum;
		++ct;
	}

#ifdef RESID_DEBUG
	fprintf(stderr,"phir = %f\n",res);
#endif

#ifdef TEST
	assert(!isnan(res));
	assert(!isinf(res));
#endif
	return res;
}

/**
	Second derivative of helmholtz residual function with respect to
	delta (twice).
*/
double helm_resid_deldel(double tau,double delta,const HelmholtzRunData *HD){
	double sum = 0, res = 0;
	double dell, ldell;
	unsigned n, i;
	const HelmholtzPowTerm *pt;
	const HelmholtzGausTerm *gt;
	const HelmholtzCritTerm *ct;

#ifdef RESID_DEBUG
		fprintf(stderr,"tau=%f, del=%f\n",tau,delta);
#endif

	/* power terms */
	n = HD->np;
	pt = &(HD->pt[0]);
	dell = ipow(delta,pt->l);
	ldell = pt->l * dell;
	unsigned oldl;
	for(i=0; i<n; ++i){
		double lpart = pt->l ? SQ(ldell) + ldell*(1. - 2*pt->d - pt->l) : 0;
		sum += pt->a * pow(tau, pt->t) * ipow(delta, pt->d - 2) * (pt->d*(pt->d - 1) + lpart);
		oldl = pt->l;
		++pt;
		if(i+1==n || oldl != pt->l){
			if(oldl == 0){
				res += sum;
			}else{
				res += sum * exp(-dell);
			}
			sum = 0;
			if(i+1<n){
				dell = ipow(delta,pt->l);
				ldell = pt->l*dell;
			}
		}
	}
#if RESID_DEBUG
	if(isnan(res)){
		fprintf(stderr,"got NAN in %s: tau = %.12e, del = %.12e\n",__func__,tau,delta);
	}
	assert(!__isnan(res));
#endif

	/* gaussian terms */
	n = HD->ng;
	//fprintf(stderr,"THERE ARE %d GAUSSIAN TERMS\n",n);
	gt = &(HD->gt[0]);
	for(i=0; i<n; ++i){
		double s1 = SQ(delta - gt->epsilon);
		double f1 = gt->d*(gt->d - 1)
			+ 2.*gt->alpha*delta * (
				delta * (2. * gt->alpha * s1 - 1)
				- 2. * gt->d * (delta - gt->epsilon)
			);
		res += gt->n * pow(tau,gt->t) * pow(delta, gt->d - 2.)
			* f1
			* exp(-(gt->alpha * s1 + gt->beta*SQ(tau-gt->gamma)));
		++gt;
	}
	assert(!__isnan(res));

	/* critical terms */
	n = HD->nc;
	ct = &(HD->ct[0]);
	for(i=0; i<n; ++i){
#ifdef RESID_DEBUG
		fprintf(stderr,"i = %d, CRITICAL, n = %e, a = %f, b = %f, beta = %f, A = %f, B = %f, C = %f, D = %f\n",i+1, ct->n, ct->a, ct->b, ct->beta, ct->A, ct->B, ct->C, ct->D);
#endif

		DEFINE_DELTA;
		DEFINE_DELB;
		DEFINE_DPSIDDELTA;
		DEFINE_DDELDDELTA;
		DEFINE_DDELBDDELTA;

		DEFINE_D2DELDDELTA2;
		DEFINE_D2DELBDDELTA2;

		DEFINE_D2PSIDDELTA2;

		sum = ct->n * (DELB*(2.*dPSIddelta + delta*d2PSIddelta2) + 2.*dDELbddelta*(PSI+delta*dPSIddelta) + d2DELbddelta2*delta*PSI);

		res += sum;
		++ct;
	}
	assert(!__isnan(res));

	return res;
}



/**
	Residual part of helmholtz function.
*/
double helm_resid_tautau(double tau, double delta, const HelmholtzRunData *HD){
	double dell, term, sum, res = 0;
	unsigned n, i;
	const HelmholtzPowTerm *pt;
	const HelmholtzGausTerm *gt;
	const HelmholtzCritTerm *ct;

	n = HD->np;
	pt = &(HD->pt[0]);

#ifdef RESID_DEBUG
		fprintf(stderr,"tau=%f, del=%f\n",tau,delta);
#endif

	/* power terms */
	sum = 0;
	dell = ipow(delta,pt->l);
	//ldell = pt->l * dell;
	unsigned oldl;
	for(i=0; i<n; ++i){
		term = pt->a * pt->t * (pt->t - 1) * pow(tau, pt->t - 2) * ipow(delta, pt->d);
		sum += term;
#ifdef RESID_DEBUG
		fprintf(stderr,"i = %d,               a=%e, t=%f, d=%d, term = %f, sum = %f",i,pt->a,pt->t,pt->d,term,sum);
		if(pt->l==0){
			fprintf(stderr,",row=%e\n",term);
		}else{
			fprintf(stderr,",row=%e\n,",term*exp(-dell));
		}
#endif
		oldl = pt->l;
		++pt;
		if(i+1==n || oldl != pt->l){
			if(oldl == 0){
#ifdef RESID_DEBUG
				fprintf(stderr,"linear ");
#endif
				res += sum;
			}else{
#ifdef RESID_DEBUG
				fprintf(stderr,"exp dell=%f, exp(-dell)=%f sum=%f: ",dell,exp(-dell),sum);
#endif
				res += sum * exp(-dell);
			}
#ifdef RESID_DEBUG
			fprintf(stderr,"i = %d, res = %f\n",i,res);
#endif
			sum = 0;
			if(i+1<n){
				dell = ipow(delta,pt->l);
				//ldell = pt->l*dell;
			}
		}
	}

	/* gaussian terms */
	n = HD->ng;
	//fprintf(stderr,"THERE ARE %d GAUSSIAN TERMS\n",n);
	gt = &(HD->gt[0]);
	for(i=0; i<n; ++i){
#ifdef RESID_DEBUG
		fprintf(stderr,"i = %d, GAUSSIAN, n = %e, t = %f, d = %f, alpha = %f, beta = %f, gamma = %f, epsilon = %f\n",i+1, gt->n, gt->t, gt->d, gt->alpha, gt->beta, gt->gamma, gt->epsilon);
#endif
		double d1 = delta - gt->epsilon;
		double t1 = tau - gt->gamma;
		double f1 = gt->t*(gt->t - 1) + 4. * gt->beta * tau * (tau * (gt->beta*SQ(t1) - 0.5) - t1*gt->t);
		double e1 = -gt->alpha*SQ(d1) - gt->beta*SQ(t1);
		sum = gt->n * f1 * pow(tau,gt->t - 2) * pow(delta,gt->d) * exp(e1);
		//fprintf(stderr,"sum = %f\n",sum);
		res += sum;
		++gt;
	}

	/* critical terms */
	n = HD->nc;
	ct = &(HD->ct[0]);
	for(i=0; i<n; ++i){
#ifdef RESID_DEBUG
		fprintf(stderr,"i = %d, CRITICAL, n = %e, a = %f, b = %f, beta = %f, A = %f, B = %f, C = %f, D = %f\n",i+1, ct->n, ct->a, ct->b, ct->beta, ct->A, ct->B, ct->C, ct->D);
#endif
		DEFINE_DELTA;
		DEFINE_DELB;
		DEFINE_DDELBDTAU;
		DEFINE_DPSIDTAU;

		double d2DELbdtau2 = 2. * ct->b * (DELB/DELTA) + 4. * SQ(theta) * ct->b * (ct->b - 1) * (DELB/SQ(DELTA));

		double d2PSIdtau2 = 2. * ct->D * PSI * (2. * ct->D * SQ(t1) -1.);

		sum = ct->n * delta * (d2DELbdtau2 * PSI + 2 * dDELbdtau*dPSIdtau + DELB * d2PSIdtau2);
		res += sum;
		++ct;
	}

#ifdef RESID_DEBUG
	fprintf(stderr,"phir_tautau = %f\n",res);
#endif
	return res;
}

/* === THIRD DERIVATIVES (this is getting boring now) === */

#ifdef INCLUDE_THIRD_DERIV_CODE
/**
	Third derivative of helmholtz residual function, with respect to
	delta (thrice).
*/
double helm_resid_deldeldel(double tau,double delta,const HelmholtzRunData *HD){
	double sum = 0, res = 0;
	double D;
	unsigned n, i;
	const HelmholtzPowTerm *pt;
	const HelmholtzGausTerm *gt;
	const HelmholtzCritTerm *ct;

#ifdef RESID_DEBUG
		fprintf(stderr,"tau=%f, del=%f\n",tau,delta);
#endif

#if 1
	/* major shortcut, but not efficient */
	double ddel = 0.0000000001;
	return (helm_resid_deldel(tau,delta+ddel,HD) - helm_resid_deldel(tau,delta,HD))/ddel;
#endif

	/* seem to be errors in the following, still haven't tracked them all down. */

#if 1
	/* wxmaxima code:
		a*delta^d*%e^(-delta^l)*tau^t
		diff(%,delta,3);
	*/
	/* power terms */
	n = HD->np;
	pt = &(HD->pt[0]);
	D = ipow(delta,pt->l);
	unsigned oldl;
	for(i=0; i<n; ++i){
		double d = pt->d;
		double l = pt->l;
		double lpart = pt->l
			? D*((D-1)*(D-2)-1)   * l*SQ(l)
				+ 3*D*(1-d)*(D-1) * SQ(l)
				+ D*(3*SQ(d-1)-1) * l
			: 0;
		sum += pt->a * pow(tau,pt->t) * ipow(delta, d-3) * (d*(d-1)*(d-2) + lpart);
		oldl = pt->l;
		++pt;
		if(i+1==n || oldl != pt->l){
			if(oldl == 0){
				res += sum; // note special meaning of l==0 case: no exponential
			}else{
				res += sum * exp(-D);
			}
			sum = 0;
			D = ipow(delta,pt->l);
		}
	}

	//fprintf(stderr,"DELDELDEL fiff = %f, sum = %f  ",fdiff, res);
#endif

#if 1
	/* gaussian terms */
	n = HD->ng;
	//fprintf(stderr,"THERE ARE %d GAUSSIAN TERMS\n",n);
	gt = &(HD->gt[0]);
	for(i=0; i<n; ++i){
		double D = delta - gt->epsilon;
		double D2 = SQ(D);
		double T2 = SQ(tau - gt->gamma);
		double A = gt->alpha * delta;
		double A2 = SQ(A);
		double d = gt->d;
		double d2 = SQ(d);

		// this expression calculated from wxMaxima using subsitutions for
		// D=delta-epsilon and A=alpha*delta.
		double f1 =
			- (8*A*A2) * D*D2
			+ (12*d * A2) * D2
			+ (12 * delta * A2 + (6*d - 6*d2)*A) * D
			- (6 * d * delta * A + d*d2 - 3*d2 + 2*d);

		res += gt->n * pow(tau,gt->t) * pow(delta, d - 3.)
			* f1
			* exp(-(gt->alpha * D2 + gt->beta * T2));
		++gt;
	}
#endif

#if 1
	/* critical terms */
	n = HD->nc;
	ct = &(HD->ct[0]);
	for(i=0; i<n; ++i){
#ifdef RESID_DEBUG
		fprintf(stderr,"i = %d, CRITICAL, n = %e, a = %f, b = %f, beta = %f, A = %f, B = %f, C = %f, D = %f\n",i+1, ct->n, ct->a, ct->b, ct->beta, ct->A, ct->B, ct->C, ct->D);
#endif

		DEFINE_DELTA;
		DEFINE_DELB;
		DEFINE_DPSIDDELTA;
		DEFINE_DDELDDELTA;
		DEFINE_DDELBDDELTA;

		DEFINE_D2PSIDDELTA2;
		DEFINE_D2DELDDELTA2;
		DEFINE_D2DELBDDELTA2;

		DEFINE_D3PSIDDELTA3;
		DEFINE_D3DELDDELTA3;
		DEFINE_D3DELBDDELTA3;

		sum = ct->n * (
			delta * (DELB*d3PSIddelta3 + 3 * dDELbddelta * d2PSIddelta2 + 3 * d2DELbddelta2 * dPSIddelta + PSI * d3DELbddelta3)
			+ 3 * (DELB*d2PSIddelta2 +  2 *  dDELbddelta * dPSIddelta + d2DELbddelta2 * PSI)
		);

		res += sum;
		++ct;
	}
#endif

	return res;
}

#endif


