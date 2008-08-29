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

#ifdef TEST
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#endif

/* forward decls */

static double helm_ideal(double tau, double delta, const IdealData *data);
static double helm_ideal_tau(double tau, double delta, const IdealData *data);
static double helm_resid(double tau, double delta, const HelmholtzData *data);
static double helm_resid_del(double tau, double delta, const HelmholtzData *data);
static double helm_resid_tau(double tau, double delta, const HelmholtzData *data);
static double helm_resid_deltau(double tau, double delta, const HelmholtzData *data);
static double helm_resid_deldel(double tau, double delta, const HelmholtzData *data);

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
#endif
	
	return data->R * T * rho * (1. + delta * helm_resid_del(tau,delta,data));
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

#ifdef TEST
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

#ifdef TEST
	assert(data->rho_star!=0);
	assert(T!=0);
	assert(!isnan(tau));
	assert(!isnan(delta));
	assert(!isnan(data->R));
#endif

	return data->R * (
		tau * (helm_ideal_tau(tau,delta,data->ideal) + helm_resid_tau(tau,delta,data))
		- helm_ideal(tau,delta,data->ideal) - helm_resid(tau,delta,data)
	);
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

/*---------------------------------------------
  IDEAL COMPONENT RELATIONS
*/

/**
	Ideal component of helmholtz function
*/	
double helm_ideal(double tau, double delta, const IdealData *data){

	const IdealPowTerm *pt;
	const IdealExpTerm *et;

	unsigned i;
	double sum = log(delta) + data->c + data->m * tau - log(tau);
	double term;

	//fprintf(stderr,"constant = %f, linear = %f", data->c, data->m);
	//fprintf(stderr,"initial terms = %f\n",sum);
	pt = &(data->pt[0]);
	for(i = 0; i<data->np; ++i, ++pt){
		term = pt->a0 * pow(tau, pt->t0);
		//fprintf(stderr,"i = %d: a0 = %f, t0 = %f, term = %f\n",i,pt->a0, pt->t0, term);
		sum += pt->a0 * pow(tau, pt->t0);
	}

#if 0
	et = &(data->et[0]);
	for(i=0; i<data->ne; ++i, ++et){
		sum += et->b * log( 1 - exp(- et->B * tau));
	}
#endif

	return sum;
}

/**
	Partial dervivative of ideal component of helmholtz residual function with 
	respect to tau.
*/	
double helm_ideal_tau(double tau, double delta, const IdealData *data){
	const IdealPowTerm *pt;
	const IdealExpTerm *et;

	unsigned i;
	double sum = -1./tau + data->m;

	pt = &(data->pt[0]);
	for(i = 0; i<data->np; ++i, ++pt){
		sum += pt->a0 * pt->t0 * pow(tau, pt->t0 - 1);
	}

#if 0
	et = &(data->et[0]);
	for(i=0; i<data->ne; ++i, ++et){
		sum += et->b * log( 1 - exp(- et->B * tau));
	}
#endif
	return sum;
}	

/**
	Residual part of helmholtz function. Note: we have NOT prematurely
	optimised here ;-)
*/
double helm_resid(double tau, double delta, const HelmholtzData *data){
	double sum;
	double res = 0;
	double delX;
	unsigned l;
	unsigned np, i;
	const HelmholtzPowTerm *pt;

	np = data->np;
	pt = &(data->pt[0]);

	delX = 1;

	l = 0;
	sum = 0;
	for(i=0; i<np; ++i){
		//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d, l = %d\n",i+1, pt->a, pt->t, pt->d, pt->l);
		sum += pt->a * pow(tau, pt->t) * ipow(delta, pt->d);
		++pt;
		//fprintf(stderr,"l = %d\n",l);
		if(i+1==np || l != pt->l){
			if(l==0){
				//fprintf(stderr,"Adding non-exp term\n");
				res += sum;
			}else{
				//fprintf(stderr,"Adding exp term with l = %d, delX = %e\n",l,delX);
				res += sum * exp(-delX);
			}
			/* set l to new value */
			if(i+1!=np){
				l = pt->l;
				//fprintf(stderr,"New l = %d\n",l);
				delX = ipow(delta,l);
				sum = 0;
			}
		}
	}

	return res;
}

/**
	Derivative of the helmholtz residual function with respect to
	delta.

	NOTE: POSSIBLY STILL AN ERROR IN THIS FUNCTION.
*/	
double helm_resid_del(double tau,double delta, const HelmholtzData *data){
	double sum;
	double res = 0;
	double delX, XdelX;
	unsigned l;
	unsigned n, i;
	const HelmholtzPowTerm *pt;
	const HelmholtzExpTerm *et;

	n = data->np;
	pt = &(data->pt[0]);

	delX = 1;

	l = 0;
	sum = 0;
	XdelX = 0;
	for(i=0; i<n; ++i){
		//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d, l = %d\n",i+1, pt->a, pt->t, pt->d, pt->l);
		sum += pt->a * pow(tau, pt->t) * pow(delta, pt->d - 1) * (pt->d - XdelX);
		++pt;
		//fprintf(stderr,"l = %d\n",l);
		if(i+1==n || l != pt->l){
			if(l==0){
				//fprintf(stderr,"Adding non-exp term\n");
				//fprintf(stderr,"sum = %f\n",sum);
				res += sum;
			}else{
				//fprintf(stderr,"Adding exp term with l = %d, delX = %e\n",l,delX);
				//fprintf(stderr,"sum = %f\n",sum);
				res += sum * exp(-delX);
			}
			/* set l to new value */
			if(i+1!=n){
				l = pt->l;
				delX = ipow(delta,l);
				XdelX = l * delX;
				//fprintf(stderr,"New l = %d, XdelX = %f\n",l,XdelX);
				sum = 0;
			}
		}
	}

#if 0
	/* now the exponential terms */
	n = data->ne;
	et = &(data->et[0]);
	for(i=0; i< n; ++i){
		fprintf(stderr,"i = %d, a = %e, t = %f, d = %d, phi = %d, beta = %d, gamma = %f\n",i+1, et->a, et->t, et->d, et->phi, et->beta, et->gamma);
		
		double del2 = delta*delta;
		double tau2 = tau*tau;
		double gam2 = et->gamma * et->gamma;
		sum = -et->a * pow(tau,et->t) * ipow(delta,et->d-1)
			* (2 * et->phi * del2 - 2 * et->phi * delta - et->d)
			* exp(-et->phi * del2
					 + 2 * et->phi * delta
					 - et->beta * tau2
					 + 2 * et->beta * et->gamma * tau
					 - et->phi 
					 - et->beta * gam2
			   );
		fprintf(stderr,"sum = %f\n",sum);
		res += sum;
		++et;
	}
#endif

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
	unsigned np, i;
	const HelmholtzPowTerm *pt;

	np = data->np;
	pt = &(data->pt[0]);

	delX = 1;

	l = 0;
	sum = 0;
	for(i=0; i<np; ++i){
		if(pt->t){
			//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d, l = %d\n",i+1, pt->a, pt->t, pt->d, pt->l);
			sum += pt->a * pow(tau, pt->t - 1) * ipow(delta, pt->d) * pt->t;
		}
		++pt;
		//fprintf(stderr,"l = %d\n",l);
		if(i+1==np || l != pt->l){
			if(l==0){
				//fprintf(stderr,"Adding non-exp term\n");
				res += sum;
			}else{
				//fprintf(stderr,"Adding exp term with l = %d, delX = %e\n",l,delX);
				res += sum * exp(-delX);
			}
			/* set l to new value */
			if(i+1!=np){
				l = pt->l;
				//fprintf(stderr,"New l = %d\n",l);
				delX = ipow(delta,l);
				sum = 0;
			}
		}
	}

	return res;
}	



/**
	Mixed derivative of the helmholtz residual function with respect to
	delta and tau
*/	
double helm_resid_deltau(double tau,double delta,const HelmholtzData *data){
	
	double sum;
	double phir = 0;
	unsigned i;
	double XdelX;

	const HelmholtzPowTerm *pt = &(data->pt[0]);
	
	for(i=0; i<5; ++i){
		phir += pt->a * pow(tau, pt->t - 1) * ipow(delta, pt->d - 1) * pt->d * pt->t;
		++pt;
	}

	sum = 0;
	XdelX = delta;
	for(i=5; i<10; ++i){
		sum += pt->a * pow(tau, pt->t - 1) * ipow(delta, pt->d - 1) * pt->t *(pt->d - XdelX);
		++pt;
	}
	phir += exp(-delta) * sum;

	sum = 0; 
	XdelX = 2*delta*delta;
	for(i=10; i<17; ++i){
		sum += pt->a * pow(tau, pt->t - 1) * ipow(delta, pt->d - 1) * pt->t *(pt->d - XdelX);
		++pt;
	}
	phir += exp(-delta*delta) * sum;

	sum = 0;
	XdelX = 3*delta*delta*delta;
	for(i=17; i<21; ++i){
		sum += pt->a * pow(tau, pt->t - 1) * ipow(delta, pt->d - 1) * pt->t *(pt->d - XdelX);
		++pt;
	}
	phir += exp(-delta*delta*delta) * sum;

	return phir;
}

#define SQ(X) ((X)*(X))

/**
	Second derivative of helmholtz residual function with respect to
	delta (twice).
*/
double helm_resid_deldel(double tau,double delta,const HelmholtzData *data){
	
	double sum;
	double phir = 0;
	unsigned i;
	unsigned X;
	double XdelX;

	const HelmholtzPowTerm *pt = &(data->pt[0]);
	
	for(i=0; i<5; ++i){
		phir += pt->a * pow(tau, pt->t) * ipow(delta, pt->d - 2) * (SQ(pt->d) - X);
		++pt;
	}

	sum = 0;
	X = 1;
	XdelX = delta;
	for(i=5; i<10; ++i){
		sum += pt->a * pow(tau, pt->t) * ipow(delta, pt->d - 2) * (SQ(XdelX) - X*XdelX - 2*pt->d*XdelX + XdelX + SQ(pt->d) - pt->d);
		++pt;
	}
	phir += exp(-delta) * sum;

	sum = 0; 
	X = 2;
	XdelX = 2*delta*delta;
	for(i=10; i<17; ++i){
		sum += pt->a * pow(tau, pt->t) * ipow(delta, pt->d - 2) * (SQ(XdelX) - X*XdelX - 2*pt->d*XdelX + XdelX + SQ(pt->d) - pt->d);
		++pt;
	}
	phir += exp(-delta*delta) * sum;

	sum = 0;
	X = 3;
	XdelX = 3*delta*delta*delta;
	for(i=17; i<21; ++i){
		sum += pt->a * pow(tau, pt->t) * ipow(delta, pt->d - 2) * (SQ(XdelX) - X*XdelX - 2*pt->d*XdelX + XdelX + SQ(pt->d) - pt->d);
		++pt;
	}
	phir += exp(-delta*delta*delta) * sum;

	return phir;
}

