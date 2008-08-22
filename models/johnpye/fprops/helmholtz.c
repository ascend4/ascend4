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

static double helm_ideal(double tau, double delta, const HelmholtzData *data);
static double helm_ideal_tau(double tau, double delta, const HelmholtzData *data);
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
	fprintf(stderr,"ideal_tau = %f\n",helm_ideal_tau(tau,delta,data));
	fprintf(stderr,"resid_tau = %f\n",helm_resid_tau(tau,delta,data));
	fprintf(stderr,"R T = %f\n",data->R * data->T_star);
#endif

	return data->R * data->T_star * (helm_ideal_tau(tau,delta,data) + helm_resid_tau(tau,delta,data));
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

	return data->R * T * (1 + tau * (helm_ideal_tau(tau,delta,data) + helm_resid_tau(tau,delta,data)) + delta*helm_resid_del(tau,delta,data));
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

	return data->R * (
		tau * (helm_ideal_tau(tau,delta,data) + helm_resid_tau(tau,delta,data))
		- helm_ideal(tau,delta,data) - helm_resid(tau,delta,data)
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
double helm_ideal(double tau, double delta, const HelmholtzData *data){
	const double *a0;

	double tau13 = pow(tau,1./3.);
	double taum32 = pow(tau,-3./2.);
	double taum74 = pow(tau,-7./4.);

	a0 = &(data->a0[0]);

	return log(delta) + a0[0] + a0[1] * tau - log(tau) + a0[2] * tau13 + a0[3] * taum32 + a0[4] * taum74;
}

/**
	Partial dervivative of ideal component of helmholtz residual function with 
	respect to tau.
*/	
double helm_ideal_tau(double tau, double delta, const HelmholtzData *data){
	const double *a0;

	double taum114 = pow(tau,-11./4.);
	double taum52 = pow(tau,-5./2.);
	double taum23 = pow(tau,-2./3.);

	//fprintf(stderr,"tau = %f, taum23 = %f\n",tau,taum23);
	//fprintf(stderr,"tau = %f, taum52 = %f\n",tau,taum52);
	//fprintf(stderr,"tau = %f, taum114 = %f\n",tau,taum114);

	a0 = &(data->a0[0]);

	//unsigned i;	for(i=0;i<5;++i)fprintf(stderr,"a0[%u] = %f\n",i,a0[i]);

	double res;
	res = a0[2]/3.*taum23 - 1./tau - 3./2.*a0[3]*taum52 - 7./4.*a0[4]*taum114 + a0[1];
	//fprintf(stderr,"res = %f\n",res);
	return res;
}	

/**
	Residual part of helmholtz function. Note: we have NOT prematurely
	optimised here ;-)
*/
double helm_resid(double tau, double delta, const HelmholtzData *data){
	
	double sum;
	double phir = 0;
	unsigned i;

	const HelmholtzATDL *atdl = &(data->atdl[0]);
	
	for(i=0; i<5; ++i){
		phir += atdl->a * pow(tau, atdl->t) * ipow(delta, atdl->d);
		++atdl;
	}

	sum = 0;
	for(i=5; i<10; ++i){
		sum += atdl->a * pow(tau, atdl->t) * ipow(delta, atdl->d);
		++atdl;
	}
	phir += exp(-delta) * sum;

	sum = 0; 
	for(i=10; i<17; ++i){
		sum += atdl->a * pow(tau, atdl->t) * ipow(delta, atdl->d);
		++atdl;
	}
	phir += exp(-delta*delta) * sum;

	sum = 0;
	for(i=17; i<21; ++i){
		sum += atdl->a * pow(tau, atdl->t) * ipow(delta, atdl->d);
		++atdl;
	}
	phir += exp(-delta*delta*delta) * sum;

	return phir;
}

/**
	Derivative of the helmholtz residual function with respect to
	delta.

	THERE IS AN ERROR IN THIS FUNCTION.
*/	
double helm_resid_del(double tau,double delta, const HelmholtzData *data){
	
	double sum;
	double phir = 0;
	unsigned i;
	double X;
	double delX;
	double XdelX;

	const HelmholtzATDL *atdl = &(data->atdl[0]);
	
	for(i=0; i<5; ++i){
		//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d\n",i+1, atdl->a, atdl->t, atdl->d);
		phir += atdl->a * pow(tau, atdl->t) * pow(delta, atdl->d - 1) * atdl->d;
		++atdl;
	}

	sum = 0;
	X = 1;
	delX = ipow(delta,X);
	XdelX = X*delX;
	for(i=5; i<10; ++i){
		//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d\n",i+1, atdl->a, atdl->t, atdl->d);
		sum -= atdl->a * pow(tau, atdl->t) * pow(delta, atdl->d - 1) * (XdelX - atdl->d);
		++atdl;
	}
	//fprintf(stderr,"sum = %f\n",sum);
	phir += exp(-delX) * sum;

	sum = 0; 
	X = 2;
	delX = ipow(delta,X);
	XdelX = X*delX;
	for(i=10; i<17; ++i){
		//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d\n",i+1, atdl->a, atdl->t, atdl->d);
		sum -= atdl->a * pow(tau, atdl->t) * pow(delta, atdl->d - 1) * (XdelX - atdl->d);
		++atdl;
	}
	//fprintf(stderr,"sum = %f\n",sum);
	phir += exp(-delX) * sum;

	sum = 0;
	X = 3;
	delX = ipow(delta,X);
	XdelX = X*delX;
	for(i=17; i<21; ++i){
		//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d\n",i+1, atdl->a, atdl->t, atdl->d);
		sum -= atdl->a * pow(tau, atdl->t) * pow(delta, atdl->d - 1) * (XdelX - atdl->d);
		++atdl;
	}
	//fprintf(stderr,"sum = %f\n",sum);
	phir += exp(-delX) * sum;

	return phir;
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
	unsigned nr, i;
	const HelmholtzATDL *atdl;

	nr = data->nr;
	atdl = &(data->atdl[0]);

	delX = 1;

#if 1
	l = 0;
	sum = 0;
	for(i=0; i<nr; ++i){
		if(atdl->t){
			//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d, l = %d\n",i+1, atdl->a, atdl->t, atdl->d, atdl->l);
			sum += atdl->a * pow(tau, atdl->t - 1) * ipow(delta, atdl->d) * atdl->t;
		}
		++atdl;
		//fprintf(stderr,"l = %d\n",l);
		if(i+1==nr || l != atdl->l){
			if(l==0){
				//fprintf(stderr,"Adding non-exp term\n");
				res += sum;
			}else{
				//fprintf(stderr,"Adding exp term with l = %d, delX = %e\n",l,delX);
				res += sum * exp(-delX);
			}
			/* set l to new value */
			if(i+1!=nr){
				l = atdl->l;
				//fprintf(stderr,"New l = %d\n",l);
				delX = ipow(delta,l);
				sum = 0;
			}
		}
	}

#else
	/* old code, not so flexible */

	delX = 1;

	for(i=0; i<5; ++i){
		if(atdl->t){
			//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d\n",i+1, atdl->a, atdl->t, atdl->d);
			res += atdl->a * pow(tau, atdl->t - 1) * ipow(delta, atdl->d) * atdl->t;
		}
		++atdl;
	}

	sum = 0;
	for(i=5; i<10; ++i){
		if(atdl->t){
			//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d\n",i+1, atdl->a, atdl->t, atdl->d);
			sum += atdl->a * pow(tau, atdl->t - 1) * ipow(delta, atdl->d) * atdl->t;
		}
		++atdl;
	}
	res += exp(-delta) * sum;

	sum = 0; 
	for(i=10; i<17; ++i){
		if(atdl->t){
			//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d\n",i+1, atdl->a, atdl->t, atdl->d);
			sum += atdl->a * pow(tau, atdl->t - 1) * ipow(delta, atdl->d) * atdl->t;
		}
		++atdl;
	}
	res += exp(-delta*delta) * sum;

	sum = 0;
	for(i=17; i<21; ++i){
		if(atdl->t){
			//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d\n",i+1, atdl->a, atdl->t, atdl->d);
			sum += atdl->a * pow(tau, atdl->t - 1) * ipow(delta, atdl->d) * atdl->t;
		}
		++atdl;
	}
	res += exp(-delta*delta*delta) * sum;
#endif

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

	const HelmholtzATDL *atdl = &(data->atdl[0]);
	
	for(i=0; i<5; ++i){
		phir += atdl->a * pow(tau, atdl->t - 1) * ipow(delta, atdl->d - 1) * atdl->d * atdl->t;
		++atdl;
	}

	sum = 0;
	XdelX = delta;
	for(i=5; i<10; ++i){
		sum += atdl->a * pow(tau, atdl->t - 1) * ipow(delta, atdl->d - 1) * atdl->t *(atdl->d - XdelX);
		++atdl;
	}
	phir += exp(-delta) * sum;

	sum = 0; 
	XdelX = 2*delta*delta;
	for(i=10; i<17; ++i){
		sum += atdl->a * pow(tau, atdl->t - 1) * ipow(delta, atdl->d - 1) * atdl->t *(atdl->d - XdelX);
		++atdl;
	}
	phir += exp(-delta*delta) * sum;

	sum = 0;
	XdelX = 3*delta*delta*delta;
	for(i=17; i<21; ++i){
		sum += atdl->a * pow(tau, atdl->t - 1) * ipow(delta, atdl->d - 1) * atdl->t *(atdl->d - XdelX);
		++atdl;
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

	const HelmholtzATDL *atdl = &(data->atdl[0]);
	
	for(i=0; i<5; ++i){
		phir += atdl->a * pow(tau, atdl->t) * ipow(delta, atdl->d - 2) * (SQ(atdl->d) - X);
		++atdl;
	}

	sum = 0;
	X = 1;
	XdelX = delta;
	for(i=5; i<10; ++i){
		sum += atdl->a * pow(tau, atdl->t) * ipow(delta, atdl->d - 2) * (SQ(XdelX) - X*XdelX - 2*atdl->d*XdelX + XdelX + SQ(atdl->d) - atdl->d);
		++atdl;
	}
	phir += exp(-delta) * sum;

	sum = 0; 
	X = 2;
	XdelX = 2*delta*delta;
	for(i=10; i<17; ++i){
		sum += atdl->a * pow(tau, atdl->t) * ipow(delta, atdl->d - 2) * (SQ(XdelX) - X*XdelX - 2*atdl->d*XdelX + XdelX + SQ(atdl->d) - atdl->d);
		++atdl;
	}
	phir += exp(-delta*delta) * sum;

	sum = 0;
	X = 3;
	XdelX = 3*delta*delta*delta;
	for(i=17; i<21; ++i){
		sum += atdl->a * pow(tau, atdl->t) * ipow(delta, atdl->d - 2) * (SQ(XdelX) - X*XdelX - 2*atdl->d*XdelX + XdelX + SQ(atdl->d) - atdl->d);
		++atdl;
	}
	phir += exp(-delta*delta*delta) * sum;

	return phir;
}

