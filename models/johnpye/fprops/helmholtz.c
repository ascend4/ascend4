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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//** @file
	Implementation of the reduced molar Helmholtz free energy equation of state.

	For nomenclature see Tillner-Roth, Harms-Watzenberg and Baehr, Eine neue
	Fundamentalgleichung für Ammoniak.

	John Pye, 29 Jul 2008.
*/

#include <math.h>

#include "helmholtz.h"
#include "ideal_impl.h"
#include "sat.h"

#if 0
# include <assert.h>
#else
# define assert(ARGS...)
#endif

#include <stdlib.h>
#include <stdio.h>

//#define RESID_DEBUG

#ifdef RESID_DEBUG
# define MSG(STR,...) fprintf(stderr,"%s:%d: " STR "\n", __func__, __LINE__ ,##__VA_ARGS__)
#else
# define MSG(ARGS...)
#endif

#define INCLUDE_THIRD_DERIV_CODE

/* macros and forward decls */

#define SQ(X) ((X)*(X))

#include "helmholtz_impl.h"

/* calculate tau and delta using a macro -- is used in most functions */
#define DEFINE_TD \
	double tau = data->T_star / T; \
	double delta = rho / data->rho_star

double helmholtz_p(double T, double rho, const HelmholtzData *d){
	double p, rho_f, rho_g;
#if 0
	if(T < d->T_t){
		fprintf(stderr,"%s: Unable to calculate pressure, T = %e is below triple point.\n", __func__, T);
		return d->p_t;
	}
	/* but what if we're in the sublimation region?? */
#endif
	if(T < d->T_c){
		int res = fprops_sat_T(T, &p, &rho_f, &rho_g, d);
		if(res){
			//fprintf(stderr,"ERROR: got error % from saturation calc in %s",res,__func__);
			return p;
		}
		if(rho < rho_f && rho > rho_g){
			return p;
		}
	}
	return helmholtz_p_raw(T,rho,d);
}

double helmholtz_h(double T, double rho, const HelmholtzData *d){
	double p, rho_f, rho_g;
	if(T < d->T_c){
		int res = fprops_sat_T(T, &p, &rho_f, &rho_g, d);
		if(res){
			//fprintf(stderr,"ERROR: got error % from saturation calc in %s",res,__func__);
			return d->rho_c;
		}
		if(rho < rho_f && rho > rho_g){
			double x = rho_g*(rho_f/rho - 1)/(rho_f - rho_g);
			return x*helmholtz_h_raw(T,rho_g,d) + (1.-x)*helmholtz_h_raw(T,rho_f,d);
		}
	}
	return helmholtz_h_raw(T,rho,d);
}

double helmholtz_s(double T, double rho, const HelmholtzData *d){
	double p, rho_f, rho_g;
	if(T < d->T_c){
		int res = fprops_sat_T(T, &p, &rho_f, &rho_g, d);
		if(res){
			//fprintf(stderr,"ERROR: got error % from saturation calc in %s",res,__func__);
			return d->rho_c;
		}
		if(rho < rho_f && rho > rho_g){
			double x = rho_g*(rho_f/rho - 1)/(rho_f - rho_g);
			return x*helmholtz_s_raw(T,rho_g,d) + (1.-x)*helmholtz_s_raw(T,rho_f,d);
		}
	}
	return helmholtz_s_raw(T,rho,d);
}

/**
	Function to calculate pressure from Helmholtz free energy EOS, given temperature
	and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return pressure in Pa???
*/
double helmholtz_p_raw(double T, double rho, const HelmholtzData *data){
	DEFINE_TD;

	assert(data->rho_star!=0);
	assert(T!=0);
	assert(!__isnan(T));
	assert(!__isnan(rho));
	assert(!__isnan(data->R));

	//fprintf(stderr,"p calc: T = %f\n",T);
	//fprintf(stderr,"p calc: tau = %f\n",tau);
	//fprintf(stderr,"p calc: rho = %f\n",rho);
	//fprintf(stderr,"p calc: delta = %f\n",delta);
	//fprintf(stderr,"p calc: R*T*rho = %f\n",data->R * T * rho);

	//fprintf(stderr,"T = %f\n", T);
	//fprintf(stderr,"rhob = %f, rhob* = %f, delta = %f\n", rho/data->M, data->rho_star/data->M, delta);
	
	double p = data->R * T * rho * (1 + delta * helm_resid_del(tau,delta,data));
	if(isnan(p)){
		fprintf(stderr,"T = %.12e, rho = %.12e\n",T,rho);
	}
	assert(!__isnan(p));
	return p;
}

/**
	Function to calculate internal energy from Helmholtz free energy EOS, given
	temperature	and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return internal energy in ???
*/
double helmholtz_u(double T, double rho, const HelmholtzData *data){
	DEFINE_TD;

#ifdef TEST
	assert(data->rho_star!=0);
	assert(T!=0);
	assert(!isnan(tau));
	assert(!isnan(delta));
	assert(!isnan(data->R));
#endif

#if 0
	fprintf(stderr,"ideal_tau = %f\n",helm_ideal_tau(tau,delta,data->ideal));
	fprintf(stderr,"resid_tau = %f\n",helm_resid_tau(tau,delta,data));
	fprintf(stderr,"R T = %f\n",data->R * data->T_star);
#endif

	return data->R * data->T_star * (helm_ideal_tau(tau,delta,data->ideal) + helm_resid_tau(tau,delta,data));
}

/**
	Function to calculate enthalpy from Helmholtz free energy EOS, given
	temperature	and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return enthalpy in J/kg
*/
double helmholtz_h_raw(double T, double rho, const HelmholtzData *data){
	DEFINE_TD;

//#ifdef TEST
	assert(data->rho_star!=0);
	assert(T!=0);
	assert(!isnan(tau));
	assert(!isnan(delta));
	assert(!isnan(data->R));
//#endif
	double h = data->R * T * (1 + tau * (helm_ideal_tau(tau,delta,data->ideal) + helm_resid_tau(tau,delta,data)) + delta*helm_resid_del(tau,delta,data));
	assert(!__isnan(h));
	return h;
}

/**
	Function to calculate entropy from Helmholtz free energy EOS, given
	temperature	and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return entropy in J/kgK
*/
double helmholtz_s_raw(double T, double rho, const HelmholtzData *data){
	DEFINE_TD;

#ifdef ENTROPY_DEBUG
	assert(data->rho_star!=0);
	assert(T!=0);
	assert(!isnan(tau));
	assert(!isnan(delta));
	assert(!isnan(data->R));

	fprintf(stderr,"helm_ideal_tau = %f\n",helm_ideal_tau(tau,delta,data->ideal));
	fprintf(stderr,"helm_resid_tau = %f\n",helm_resid_tau(tau,delta,data));
	fprintf(stderr,"helm_ideal = %f\n",helm_ideal(tau,delta,data->ideal));
	fprintf(stderr,"helm_resid = %f\n",helm_resid(tau,delta,data));
#endif
	return data->R * (
		tau * (helm_ideal_tau(tau,delta,data->ideal) + helm_resid_tau(tau,delta,data))
		- (helm_ideal(tau,delta,data->ideal) + helm_resid(tau,delta,data))
	);
}

/**
	Function to calculate Helmholtz energy from the Helmholtz free energy EOS,
	given temperature and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return Helmholtz energy 'a', in J/kg
*/
double helmholtz_a(double T, double rho, const HelmholtzData *data){
	DEFINE_TD;

#ifdef TEST
	assert(data->rho_star!=0);
	assert(T!=0);
	assert(!isnan(tau));
	assert(!isnan(delta));
	assert(!isnan(data->R));
#endif

#ifdef HELMHOLTZ_DEBUG
	fprintf(stderr,"helmholtz_a: T = %f, rho = %f\n",T,rho);
	fprintf(stderr,"multiplying by RT = %f\n",data->R*T);
#endif

	return data->R * T * (helm_ideal(tau,delta,data->ideal) + helm_resid(tau,delta,data));
}

/**
	Function to calculate isochoric heat capacity from the Helmholtz free energy
	EOS given temperature and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return Isochoric specific heat capacity in J/kg/K.
*/
double helmholtz_cv(double T, double rho, const HelmholtzData *data){
	DEFINE_TD;

	return - data->R * SQ(tau) * (helm_ideal_tautau(tau,data->ideal) + helm_resid_tautau(tau,delta,data));
}

/**
	Function to calculate isobaric heat capacity from the Helmholtz free energy
	EOS given temperature and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return Isobaric specific heat capacity in J/kg/K.
*/
double helmholtz_cp(double T, double rho, const HelmholtzData *data){
	DEFINE_TD;

	double phir_d = helm_resid_del(tau,delta,data);
	double phir_dd = helm_resid_deldel(tau,delta,data);
	double phir_dt = helm_resid_deltau(tau,delta,data);
	
	/* note similarities with helmholtz_w */
	double temp1 = 1 + 2*delta*phir_d + SQ(delta)*phir_dd;
	double temp2 = 1 + delta*phir_d - delta*tau*phir_dt;
	double temp3 = -SQ(tau)*(helm_ideal_tautau(tau,data->ideal) + helm_resid_tautau(tau,delta,data));

	return data->R * (temp3 + SQ(temp2)/temp1);
}


/**
	Function to calculate the speed of sound in a fluid from the Helmholtz free
	energy EOS, given temperature and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return Speed of sound in m/s.
*/
double helmholtz_w(double T, double rho, const HelmholtzData *data){
	DEFINE_TD;

	double phir_d = helm_resid_del(tau,delta,data);
	double phir_dd = helm_resid_deldel(tau,delta,data);
	double phir_dt = helm_resid_deltau(tau,delta,data);
	
	/* note similarities with helmholtz_cp */
	double temp1 = 1. + 2.*delta*phir_d + SQ(delta)*phir_dd;
	double temp2 = 1. + delta*phir_d - delta*tau*phir_dt;
	double temp3 = -SQ(tau)*(helm_ideal_tautau(tau,data->ideal) + helm_resid_tautau(tau,delta,data));

	return sqrt(data->R * T * (temp1 + SQ(temp2)/temp3));

}

/**
	Function to calculate the Gibbs energy fluid from the Helmholtz free
	energy EOS, given temperature and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return Gibbs energy, in J/kg.
*/
double helmholtz_g(double T, double rho, const HelmholtzData *data){
	DEFINE_TD;

	double phir_d = helm_resid_del(tau,delta,data);
	double phir = helm_resid(tau,delta,data);
	double phi0 = helm_ideal(tau,delta,data->ideal);
	
	return data->R * T * (phi0 + phir + 1. + delta * phir_d);
}

/**
	Calculation zero-pressure specific heat capacity
*/
double helmholtz_cp0(double T, const HelmholtzData *data){
	double val = helm_cp0(T,data->ideal);
#if 0
	fprintf(stderr,"val = %f\n",val);
#endif
	return val;
}

/** 
	alpha_p function from IAPWS Advisory Note 3, used in calculation of
	partial property derivatives.
*/
double helmholtz_alphap(double T, double rho, const HelmholtzData *data){
	DEFINE_TD;
	double phir_d = helm_resid_del(tau,delta,data);
	double phir_dt = helm_resid_deltau(tau,delta,data);
	return 1./T * (1. - delta*tau*phir_dt/(1 + delta*phir_d));
}

/**
	beta_p function from IAPWS Advisory Note 3 , used in calculation of partial
	property derivatives.
*/
double helmholtz_betap(double T, double rho, const HelmholtzData *data){
	DEFINE_TD;
	double phir_d = helm_resid_del(tau,delta,data);
	double phir_dd = helm_resid_deldel(tau,delta,data);
	return rho*(1. + (delta*phir_d + SQ(delta)*phir_dd)/(1+delta*phir_d));
}

/*----------------------------------------------------------------------------
  PARTIAL DERIVATIVES
*/

/**
	Calculate partial derivative of p with respect to T, with rho constant
*/
double helmholtz_dpdT_rho(double T, double rho, const HelmholtzData *data){
	DEFINE_TD;

	double phir_del = helm_resid_del(tau,delta,data);
	double phir_deltau = helm_resid_deltau(tau,delta,data);
#ifdef TEST
	assert(!isinf(phir_del));
	assert(!isinf(phir_deltau));
	assert(!isnan(phir_del));
	assert(!isnan(phir_deltau));
	assert(!isnan(data->R));
	assert(!isnan(rho));
	assert(!isnan(tau));
#endif

	double res =  data->R * rho * (1 + delta*phir_del - delta*tau*phir_deltau);

#ifdef TEST
	assert(!isnan(res));
	assert(!isinf(res));
#endif
	return res;
}

/**
	Calculate partial derivative of p with respect to rho, with T constant
*/
double helmholtz_dpdrho_T(double T, double rho, const HelmholtzData *data){
	DEFINE_TD;

	double phir_del = helm_resid_del(tau,delta,data);
	double phir_deldel = helm_resid_deldel(tau,delta,data);
#ifdef TEST
	assert(!isinf(phir_del));
	assert(!isinf(phir_deldel));
#endif	
	return data->R * T * (1 + 2*delta*phir_del + SQ(delta)*phir_deldel);
}


double helmholtz_d2pdrho2_T(double T, double rho, const HelmholtzData *data){
	DEFINE_TD;

	double phir_del = helm_resid_del(tau,delta,data);
	double phir_deldel = helm_resid_deldel(tau,delta,data);
	double phir_deldeldel = helm_resid_deldeldel(tau,delta,data);
#ifdef TEST
	assert(!isinf(phir_del));
	assert(!isinf(phir_deldel));
	assert(!isinf(phir_deldeldel));
#endif	

	return data->R * T / rho * delta * (2*phir_del + delta*(4*phir_deldel + delta*phir_deldeldel));
}

/**
	Calculate partial derivative of h with respect to T, with rho constant
*/
double helmholtz_dhdT_rho(double T, double rho, const HelmholtzData *data){
	DEFINE_TD;

	double phir_del = helm_resid_del(tau,delta,data);
	double phir_deltau = helm_resid_deltau(tau,delta,data);
	double phir_tautau = helm_resid_tautau(tau,delta,data);
	double phi0_tautau = helm_ideal_tautau(tau,data->ideal);

	//fprintf(stderr,"phir_del = %f, phir_deltau = %f, phir_tautau = %f, phi0_tautau = %f\n",phir_del,phir_deltau,phir_tautau,phi0_tautau);

	//return (helmholtz_h(T+0.01,rho,data) - helmholtz_h(T,rho,data)) / 0.01;
	return data->R * (1. + delta*phir_del - SQ(tau)*(phi0_tautau + phir_tautau) - delta*tau*phir_deltau);
}

/**
	Calculate partial derivative of h with respect to rho, with T constant
*/
double helmholtz_dhdrho_T(double T, double rho, const HelmholtzData *data){
	DEFINE_TD;

	double phir_del = helm_resid_del(tau,delta,data);
	double phir_deltau = helm_resid_deltau(tau,delta,data);
	double phir_deldel = helm_resid_deldel(tau,delta,data);
	
	return data->R * T / rho * (tau*delta*(0 + phir_deltau) + delta * phir_del + SQ(delta)*phir_deldel);
}


/**
	Calculate partial derivative of u with respect to T, with rho constant
*/
double helmholtz_dudT_rho(double T, double rho, const HelmholtzData *data){
	DEFINE_TD;

	double phir_tautau = helm_resid_tautau(tau,delta,data);
	double phi0_tautau = helm_ideal_tautau(tau,data->ideal);

	return -data->R * SQ(tau) * (phi0_tautau + phir_tautau);
}

/**
	Calculate partial derivative of u with respect to rho, with T constant
*/
double helmholtz_dudrho_T(double T, double rho, const HelmholtzData *data){
	DEFINE_TD;

	double phir_deltau = helm_resid_deltau(tau,delta,data);
	
	return data->R * T / rho * (tau * delta * phir_deltau);
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
double helm_resid(double tau, double delta, const HelmholtzData *data){
	double dell,ldell, term, sum, res = 0;
	unsigned n, i;
	const HelmholtzPowTerm *pt;
	const HelmholtzGausTerm *gt;
	const HelmholtzCritTerm *ct;

	n = data->np;
	pt = &(data->pt[0]);

	MSG("tau=%f, del=%f",tau,delta);

	/* power terms */
	sum = 0;
	dell = ipow(delta,pt->l);
	ldell = pt->l * dell;
	unsigned oldl;
	for(i=0; i<n; ++i){
		term = pt->a * pow(tau, pt->t) * ipow(delta, pt->d);
		sum += term;
#ifdef RESID_DEBUG
		fprintf(stderr,"i = %d,               a=%e, t=%f, d=%d, l=%u, term = %f, sum = %f",i,pt->a,pt->t,pt->d,pt->l,term,sum);
		if(pt->l==0){
			fprintf(stderr,",row=%e\n",term);
		}else{
			fprintf(stderr,",row=%e\n",term*exp(-dell));
		}
#endif
		oldl = pt->l;
		++pt;
		if(i+1==n || oldl != pt->l){
			if(oldl == 0){
#ifdef RESID_DEBUG
				fprintf(stderr,"                      linear ");
#endif
				res += sum;
			}else{
#ifdef RESID_DEBUG
				fprintf(stderr,"                      %sEXP dell=%f, exp(-dell)=%f sum=%f: ",(i+1==n?"LAST ":""),dell,exp(-dell),sum);
#endif
				res += sum * exp(-dell);
			}
#ifdef RESID_DEBUG
			fprintf(stderr,"i = %d, res = %f\n",i,res);
#endif
			sum = 0;
			if(i+1<n){
#ifdef RESID_DEBUG
				fprintf(stderr,"                      next delta = %.12e, l = %u\n",delta, pt->l);
#endif
				dell = (delta==0 ? 0 : ipow(delta,pt->l));
				ldell = pt->l*dell;
			}
		}
	}
	assert(!__isnan(res));

	/* gaussian terms */
	n = data->ng;
	//fprintf(stderr,"THERE ARE %d GAUSSIAN TERMS\n",n);
	gt = &(data->gt[0]);
	for(i=0; i<n; ++i){
#ifdef RESID_DEBUG
		fprintf(stderr,"i = %d, GAUSSIAN, n = %e, t = %f, d = %f, alpha = %f, beta = %f, gamma = %f, epsilon = %f\n",i+1, gt->n, gt->t, gt->d, gt->alpha, gt->beta, gt->gamma, gt->epsilon);
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
	n = data->nc;
	ct = &(data->ct[0]);
	for(i=0; i<n; ++i){
#ifdef RESID_DEBUG
		fprintf(stderr,"i = %d, CRITICAL, n = %e, a = %f, b = %f, beta = %f, A = %f, B = %f, C = %f, D = %f\n",i+1, ct->n, ct->a, ct->b, ct->beta, ct->A, ct->B, ct->C, ct->D);
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
double helm_resid_del(double tau,double delta, const HelmholtzData *data){
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
	n = data->np;
	pt = &(data->pt[0]);
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
	n = data->ng;
	gt = &(data->gt[0]);
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
	n = data->nc;
	ct = &(data->ct[0]);
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
double helm_resid_tau(double tau,double delta,const HelmholtzData *data){
	
	double sum;
	double res = 0;
	double delX;
	unsigned l;
	unsigned n, i;
	const HelmholtzPowTerm *pt;
	const HelmholtzGausTerm *gt;
	const HelmholtzCritTerm *ct;

	n = data->np;
	pt = &(data->pt[0]);

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
	n = data->ng;
	gt = &(data->gt[0]);
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
	n = data->nc;
	ct = &(data->ct[0]);
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
double helm_resid_deltau(double tau,double delta,const HelmholtzData *data){
	double dell,ldell, sum = 0, res = 0;
	unsigned n, i;
	const HelmholtzPowTerm *pt;
	const HelmholtzGausTerm *gt;
	const HelmholtzCritTerm *ct;

	/* power terms */
	n = data->np;
	pt = &(data->pt[0]);
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
	n = data->ng;
	gt = &(data->gt[0]);
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
	n = data->nc;
	ct = &(data->ct[0]);
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
double helm_resid_deldel(double tau,double delta,const HelmholtzData *data){
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
	n = data->np;
	pt = &(data->pt[0]);
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
	if(__isnan(res)){
		fprintf(stderr,"tau = %.12e, del = %.12e\n",tau,delta);
	}
	assert(!__isnan(res));

	/* gaussian terms */
	n = data->ng;
	//fprintf(stderr,"THERE ARE %d GAUSSIAN TERMS\n",n);
	gt = &(data->gt[0]);
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
	n = data->nc;
	ct = &(data->ct[0]);
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
double helm_resid_tautau(double tau, double delta, const HelmholtzData *data){
	double dell,ldell, term, sum, res = 0;
	unsigned n, i;
	const HelmholtzPowTerm *pt;
	const HelmholtzGausTerm *gt;
	const HelmholtzCritTerm *ct;

	n = data->np;
	pt = &(data->pt[0]);

#ifdef RESID_DEBUG
		fprintf(stderr,"tau=%f, del=%f\n",tau,delta);
#endif

	/* power terms */
	sum = 0;
	dell = ipow(delta,pt->l);
	ldell = pt->l * dell;
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
				ldell = pt->l*dell;
			}
		}
	}

	/* gaussian terms */
	n = data->ng;
	//fprintf(stderr,"THERE ARE %d GAUSSIAN TERMS\n",n);
	gt = &(data->gt[0]);
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
	n = data->nc;
	ct = &(data->ct[0]);
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
double helm_resid_deldeldel(double tau,double delta,const HelmholtzData *data){
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
	return (helm_resid_deldel(tau,delta+ddel,data) - helm_resid_deldel(tau,delta,data))/ddel;
#endif

	/* seem to be errors in the following, still haven't tracked them all down. */

#if 1
	/* wxmaxima code:
		a*delta^d*%e^(-delta^l)*tau^t
		diff(%,delta,3);
	*/
	/* power terms */
	n = data->np;
	pt = &(data->pt[0]);
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
	n = data->ng;
	//fprintf(stderr,"THERE ARE %d GAUSSIAN TERMS\n",n);
	gt = &(data->gt[0]);
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
	n = data->nc;
	ct = &(data->ct[0]);
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


