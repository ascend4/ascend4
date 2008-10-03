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
	Implementation of the reduced molar Helmholtz free energy equation of state.

	For nomenclature see Tillner-Roth, Harms-Watzenberg and Baehr, Eine neue
	Fundamentalgleichung für Ammoniak.

	John Pye, 29 Jul 2008.
*/

#include <math.h>

#include "helmholtz.h"
#include "ideal_impl.h"

#ifdef TEST
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#endif

#define SQ(X) ((X)*(X))

/* forward decls */

static double helm_resid(double tau, double delta, const HelmholtzData *data);
static double helm_resid_del(double tau, double delta, const HelmholtzData *data);
static double helm_resid_tau(double tau, double delta, const HelmholtzData *data);
static double helm_resid_deltau(double tau, double delta, const HelmholtzData *data);
static double helm_resid_deldel(double tau, double delta, const HelmholtzData *data);
static double helm_resid_tautau(double tau, double delta, const HelmholtzData *data);

/**
	Function to calculate pressure from Helmholtz free energy EOS, given temperature
	and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return pressure in Pa???
*/
double helmholtz_p(double T, double rho, const HelmholtzData *data){
	
	double tau = data->T_star / T;
	double delta = rho / data->rho_star;

#ifdef TEST
	assert(data->rho_star!=0);
	assert(T!=0);
	assert(!isnan(tau));
	assert(!isnan(delta));
	assert(!isnan(data->R));

	//fprintf(stderr,"p calc: T = %f\n",T);
	//fprintf(stderr,"p calc: tau = %f\n",tau);
	//fprintf(stderr,"p calc: rho = %f\n",rho);
	//fprintf(stderr,"p calc: delta = %f\n",delta);
	//fprintf(stderr,"p calc: R*T*rho = %f\n",data->R * T * rho);

	//fprintf(stderr,"T = %f\n", T);
	//fprintf(stderr,"rhob = %f, rhob* = %f, delta = %f\n", rho/data->M, data->rho_star/data->M, delta);
#endif
	
	return data->R * T * rho * (1 + delta * helm_resid_del(tau,delta,data));
}

/**
	Function to calculate internal energy from Helmholtz free energy EOS, given
	temperature	and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return internal energy in ???
*/
double helmholtz_u(double T, double rho, const HelmholtzData *data){
	
	double tau = data->T_star / T;
	double delta = rho / data->rho_star;

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
double helmholtz_h(double T, double rho, const HelmholtzData *data){
	
	double tau = data->T_star / T;
	double delta = rho / data->rho_star;

#ifdef TEST
	assert(data->rho_star!=0);
	assert(T!=0);
	assert(!isnan(tau));
	assert(!isnan(delta));
	assert(!isnan(data->R));
#endif

	return data->R * T * (1 + tau * (helm_ideal_tau(tau,delta,data->ideal) + helm_resid_tau(tau,delta,data)) + delta*helm_resid_del(tau,delta,data));
}

/**
	Function to calculate entropy from Helmholtz free energy EOS, given
	temperature	and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return entropy in J/kgK
*/
double helmholtz_s(double T, double rho, const HelmholtzData *data){
	
	double tau = data->T_star / T;
	double delta = rho / data->rho_star;

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

	double tau = data->T_star / T;
	double delta = rho / data->rho_star;

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
	Calculate partial derivative of p with respect to T, with rho constant
*/
double helmholtz_dpdT_rho(double T, double rho, const HelmholtzData *data){
	double tau = data->T_star / T;
	double delta = rho / data->rho_star;

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
	double tau = data->T_star / T;
	double delta = rho / data->rho_star;

	double phir_del = helm_resid_del(tau,delta,data);
	double phir_deldel = helm_resid_deldel(tau,delta,data);
#ifdef TEST
	assert(!isinf(phir_del));
	assert(!isinf(phir_deldel));
#endif	
	return data->R * T * (1 + 2*delta*phir_del + delta*delta* phir_deldel);
}

/**
	Calculate partial derivative of h with respect to T, with rho constant
*/
double helmholtz_dhdT_rho(double T, double rho, const HelmholtzData *data){
	double tau = data->T_star / T;
	double delta = rho / data->rho_star;

	double phir_del = helm_resid_del(tau,delta,data);
	double phir_deltau = helm_resid_deltau(tau,delta,data);
	double phir_tautau = helm_resid_tautau(tau,delta,data);
	double phi0_tautau = helm_ideal_tautau(tau,data->ideal);

	//fprintf(stderr,"phir_del = %f, phir_deltau = %f, phir_tautau = %f, phi0_tautau = %f\n",phir_del,phir_deltau,phir_tautau,phi0_tautau);

	//return (helmholtz_h(T+0.01,rho,data) - helmholtz_h(T,rho,data)) / 0.01;
	return data->R * (1. + delta*phir_del - tau*tau*(phi0_tautau + phir_tautau) - delta*tau*phir_deltau);
}

/**
	Calculate partial derivative of h with respect to rho, with T constant
*/
double helmholtz_dhdrho_T(double T, double rho, const HelmholtzData *data){
	double tau = data->T_star / T;
	double delta = rho / data->rho_star;

	double phir_del = helm_resid_del(tau,delta,data);
	double phir_deltau = helm_resid_deltau(tau,delta,data);
	double phir_deldel = helm_resid_deldel(tau,delta,data);
	
	return data->R * T / rho * (tau*delta*(0 + phir_deltau) + delta * phir_del + SQ(delta)*phir_deldel);
}


/**
	Calculate partial derivative of u with respect to T, with rho constant
*/
double helmholtz_dudT_rho(double T, double rho, const HelmholtzData *data){
	double tau = data->T_star / T;
	double delta = rho / data->rho_star;

	double phir_tautau = helm_resid_tautau(tau,delta,data);
	double phi0_tautau = helm_ideal_tautau(tau,data->ideal);

	return -data->R * SQ(tau) * (phi0_tautau + phir_tautau);
}

/**
	Calculate partial derivative of u with respect to rho, with T constant
*/
double helmholtz_dudrho_T(double T, double rho, const HelmholtzData *data){
	double tau = data->T_star / T;
	double delta = rho / data->rho_star;

	double phir_deltau = helm_resid_deltau(tau,delta,data);
	
	return data->R * T * tau * phir_deltau;
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

//#define RESID_DEBUG

/**
	Residual part of helmholtz function.
*/
double helm_resid(double tau, double delta, const HelmholtzData *data){
	double dell,ldell, term, sum, res = 0;
	unsigned n, i;
	const HelmholtzPowTerm *pt;
	const HelmholtzGausTerm *gt;

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
		term = pt->a * pow(tau, pt->t) * ipow(delta, pt->d);
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
			dell = ipow(delta,pt->l);
			ldell = pt->l*dell;
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
		double e1 = -gt->alpha*d1*d1 - gt->beta*t1*t1;
		sum = gt->n * pow(tau,gt->t) * pow(delta,gt->d) * exp(e1);
		//fprintf(stderr,"sum = %f\n",sum);
		res += sum;
		++gt;
	}

#ifdef RESID_DEBUG
	fprintf(stderr,"phir = %f\n",res);
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
			dell = ipow(delta,pt->l);
			ldell = pt->l*dell;
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
		double val2;
		val2 = - gt->n * pow(tau,gt->t) * pow(delta, -1. + gt->d)
			* (2. * gt->alpha * delta * (delta - gt->epsilon) - gt->d)
			* exp(-(gt->alpha * SQ(delta-gt->epsilon) + gt->beta*SQ(tau-gt->gamma)));
		res += val2;
#ifdef RESID_DEBUG
		fprintf(stderr,"val2 = %f --> res = %f\n",val2,res);
#endif
		++gt;
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

	return res;
}	


/*=================== SECOND DERIVATIVES =======================*/

/**
	Mixed derivative of the helmholtz residual function with respect to
	delta and tau.
*/
double helm_resid_deltau(double tau,double delta,const HelmholtzData *data){
	double dell,ldell, term, sum = 0, res = 0;
	unsigned n, i;
	const HelmholtzPowTerm *pt;
	const HelmholtzGausTerm *gt;

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
			dell = ipow(delta,pt->l);
			ldell = pt->l*dell;
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

	FIXME this function is WRONG.
*/
double helm_resid_deldel(double tau,double delta,const HelmholtzData *data){
	double sum = 0, res = 0;
	double dell, ldell;
	unsigned n, i;
	const HelmholtzPowTerm *pt;
	const HelmholtzGausTerm *gt;


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
			dell = ipow(delta,pt->l);
			ldell = pt->l*dell;
		}
	}

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
			dell = ipow(delta,pt->l);
			ldell = pt->l*dell;
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

#ifdef RESID_DEBUG
	fprintf(stderr,"phir_tautau = %f\n",res);
#endif
	return res;
}

