/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University

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

/* Property data for Ammonia, from Tillner-Roth, Harms-Watzenberg and
Baehr, Eine neue Fundamentalgleichung für Ammoniak, DKV-Tagungsbericht,
20:167-181, 1993. This is the ammmonia property correlation recommended
by NIST in its program REFPROP 7.0. */
const HelmholtzData helmholtz_data_ammonia = {
	/* R */ 488.189 /* J/kg/K */
	, /* rho_star */225. /* kg/m³ */
	, /* T_star */ 405.40 /* K */

	, {
		/* a0_1 */ -15.815020
		,/* a0_2 */ 4.255726
		,/* a0_3 */ 11.474340
		,/* a0_4 */ -1.296211
		,/* a0_5 */ 0.5706757
	}

	, {
		/* a_i, t_i, d_i */
		/* 1 */{0.4554431E-1,  -0.5  ,  2}
		,{0.7238548E+0,   0.5 ,   1 }
		,{0.1229470E-1,     1 ,   4 }
		,{-0.1858814E+1,  1.5 ,   1 }
		/* 5 */,{0.2141882E-10,    3 ,  15 }
		,{-0.1430020E-1,    0 ,   3 }
		,{0.3441324E+0,     3 ,   3 } 
		,{-0.2873571E+0,    4 ,   1 }
		,{0.2352589E-4,     4 ,   8 }
		/* 10 */,{-0.3497111E-1,   5  ,  2}
		,{0.2397852E-1,    3  ,  1}
		,{0.1831117E-2,    5 ,   8}
		,{-0.4085375E-1,   6 ,   1}
		,{0.2379275E+0,    8 ,   2}
		/* 15 */,{-0.3548972E-1,   8 ,   3}
		,{-0.1823729E+0,   10,   2}
		,{0.2281556E-1,   10 ,   4}
		,{-0.6663444E-2,   5 ,   3}
		,{-0.8847486E-2,  7.5,   1}
		/* 20 */,{0.2272635E-2 ,  15 ,   2}
		,{-0.5588655E-3,  30,    4}
	}
};

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

	fprintf(stderr,"ideal_tau = %f\n",helm_ideal_tau(tau,delta,data));
	fprintf(stderr,"resid_tau = %f\n",helm_resid_tau(tau,delta,data));

	fprintf(stderr,"R T = %f\n",data->R * data->T_star);

	return data->R * data->T_star * (helm_ideal_tau(tau,delta,data) + helm_resid_tau(tau,delta,data));
}

/**
	Function to calculate enthalpy from Helmholtz free energy EOS, given
	temperature	and mass density.

	@param T temperature in K
	@param rho mass density in kg/m³
	@return enthalpy in ????
*/
double helmholtz_h(double T, double rho, const HelmholtzData *data){
	
	double tau = data->T_star / T;
	double delta = rho / data->rho_star;

	return data->R * T * (1 + tau * (helm_ideal_tau(tau,delta,data) + helm_resid_tau(tau,delta,data)) + delta*helm_resid_del(tau,delta,data));
}

/*---------------------------------------------
  UTILITY FUNCTION(S)
*/

/* ipow:  public domain by Mark Stephen with suggestions by Keiichi Nakasato */
static double ipow(double x, int n){
	double t = 1.0;

	if(!n)return t;    /* At the top. 0^0 = 1 */

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

	fprintf(stderr,"tau = %f, taum23 = %f\n",tau,taum23);
	fprintf(stderr,"tau = %f, taum52 = %f\n",tau,taum52);
	fprintf(stderr,"tau = %f, taum114 = %f\n",tau,taum114);

	a0 = &(data->a0[0]);

	//unsigned i;	for(i=0;i<5;++i)fprintf(stderr,"a0[%u] = %f\n",i,a0[i]);

	fprintf(stderr,"3 = %f\n", a0[2]/3.*taum23);

	double res;
	res = a0[2]/3.*taum23 - 1./tau - 3./2.*a0[3]*taum52 - 7./4.*a0[4]*taum114 + a0[1];
	fprintf(stderr,"res = %f\n",res);
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

	const HelmholtzATD *atd = &(data->atd[0]);
	
	for(i=0; i<5; ++i){
		phir += atd->a * pow(tau, atd->t) * ipow(delta, atd->d);
		++atd;
	}

	sum = 0;
	for(i=5; i<10; ++i){
		sum += atd->a * pow(tau, atd->t) * ipow(delta, atd->d);
		++atd;
	}
	phir += exp(-delta) * sum;

	sum = 0; 
	for(i=10; i<17; ++i){
		sum += atd->a * pow(tau, atd->t) * ipow(delta, atd->d);
		++atd;
	}
	phir += exp(-delta*delta) * sum;

	sum = 0;
	for(i=17; i<21; ++i){
		sum += atd->a * pow(tau, atd->t) * ipow(delta, atd->d);
		++atd;
	}
	phir += exp(-delta*delta*delta) * sum;

	return phir;
}

/**
	Derivative of the helmholtz residual function with respect to
	delta.
*/	
double helm_resid_del(double tau,double delta, const HelmholtzData *data){
	
	double sum;
	double phir = 0;
	unsigned i;
	double XdelX;

	const HelmholtzATD *atd = &(data->atd[0]);
	
	for(i=0; i<5; ++i){
		//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d\n",i+1, atd->a, atd->t, atd->d);
		phir += atd->a * pow(tau, atd->t) * ipow(delta, atd->d - 1) * atd->d;
		++atd;
	}

	sum = 0;
	XdelX = delta;
	for(i=5; i<10; ++i){
		//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d\n",i+1, atd->a, atd->t, atd->d);
		sum += atd->a * pow(tau, atd->t) * ipow(delta, atd->d - 1) * (atd->d - XdelX);
		++atd;
	}
	phir += exp(-delta) * sum;

	sum = 0; 
	XdelX = 2*delta*delta;
	for(i=10; i<17; ++i){
		sum += atd->a * pow(tau, atd->t) * ipow(delta, atd->d - 1) * (atd->d - XdelX);
		++atd;
	}
	phir += exp(-delta*delta) * sum;

	sum = 0;
	XdelX = 3*delta*delta*delta;
	for(i=17; i<21; ++i){
		sum += atd->a * pow(tau, atd->t) * ipow(delta, atd->d - 1) * (atd->d - XdelX);
		++atd;
	}
	phir += exp(-delta*delta*delta) * sum;

	return phir;
}

/**
	Derivative of the helmholtz residual function with respect to
	tau.
*/			
double helm_resid_tau(double tau,double delta,const HelmholtzData *data){
	
	double sum;
	double res = 0;
	unsigned i;

	const HelmholtzATD *atd = &(data->atd[0]);
	
	for(i=0; i<5; ++i){
		if(atd->t){
			//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d\n",i+1, atd->a, atd->t, atd->d);
			res += atd->a * pow(tau, atd->t - 1) * ipow(delta, atd->d) * atd->t;
		}
		++atd;
	}

	sum = 0;
	for(i=5; i<10; ++i){
		if(atd->t){
			//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d\n",i+1, atd->a, atd->t, atd->d);
			sum += atd->a * pow(tau, atd->t - 1) * ipow(delta, atd->d) * atd->t;
		}
		++atd;
	}
	res += exp(-delta) * sum;

	sum = 0; 
	for(i=10; i<17; ++i){
		if(atd->t){
			//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d\n",i+1, atd->a, atd->t, atd->d);
			sum += atd->a * pow(tau, atd->t - 1) * ipow(delta, atd->d) * atd->t;
		}
		++atd;
	}
	res += exp(-delta*delta) * sum;

	sum = 0;
	for(i=17; i<21; ++i){
		if(atd->t){
			//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d\n",i+1, atd->a, atd->t, atd->d);
			sum += atd->a * pow(tau, atd->t - 1) * ipow(delta, atd->d) * atd->t;
		}
		++atd;
	}
	res += exp(-delta*delta*delta) * sum;

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

	const HelmholtzATD *atd = &(data->atd[0]);
	
	for(i=0; i<5; ++i){
		phir += atd->a * pow(tau, atd->t - 1) * ipow(delta, atd->d - 1) * atd->d * atd->t;
		++atd;
	}

	sum = 0;
	XdelX = delta;
	for(i=5; i<10; ++i){
		sum += atd->a * pow(tau, atd->t - 1) * ipow(delta, atd->d - 1) * atd->t *(atd->d - XdelX);
		++atd;
	}
	phir += exp(-delta) * sum;

	sum = 0; 
	XdelX = 2*delta*delta;
	for(i=10; i<17; ++i){
		sum += atd->a * pow(tau, atd->t - 1) * ipow(delta, atd->d - 1) * atd->t *(atd->d - XdelX);
		++atd;
	}
	phir += exp(-delta*delta) * sum;

	sum = 0;
	XdelX = 3*delta*delta*delta;
	for(i=17; i<21; ++i){
		sum += atd->a * pow(tau, atd->t - 1) * ipow(delta, atd->d - 1) * atd->t *(atd->d - XdelX);
		++atd;
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

	const HelmholtzATD *atd = &(data->atd[0]);
	
	for(i=0; i<5; ++i){
		phir += atd->a * pow(tau, atd->t) * ipow(delta, atd->d - 2) * (SQ(atd->d) - X);
		++atd;
	}

	sum = 0;
	X = 1;
	XdelX = delta;
	for(i=5; i<10; ++i){
		sum += atd->a * pow(tau, atd->t) * ipow(delta, atd->d - 2) * (SQ(XdelX) - X*XdelX - 2*atd->d*XdelX + XdelX + SQ(atd->d) - atd->d);
		++atd;
	}
	phir += exp(-delta) * sum;

	sum = 0; 
	X = 2;
	XdelX = 2*delta*delta;
	for(i=10; i<17; ++i){
		sum += atd->a * pow(tau, atd->t) * ipow(delta, atd->d - 2) * (SQ(XdelX) - X*XdelX - 2*atd->d*XdelX + XdelX + SQ(atd->d) - atd->d);
		++atd;
	}
	phir += exp(-delta*delta) * sum;

	sum = 0;
	X = 3;
	XdelX = 3*delta*delta*delta;
	for(i=17; i<21; ++i){
		sum += atd->a * pow(tau, atd->t) * ipow(delta, atd->d - 2) * (SQ(XdelX) - X*XdelX - 2*atd->d*XdelX + XdelX + SQ(atd->d) - atd->d);
		++atd;
	}
	phir += exp(-delta*delta*delta) * sum;

	return phir;
}


/*
	Test suite. These tests attempt to validate the current code using 
	a few sample figures output by REFPROP 7.0.

	To run the test, compile and run as follows:

	gcc helmholtz.c -DTEST -o helmholtz -lm && ./helmholtz
*/
#ifdef TEST

/* a simple macro to actually do the testing */
#define ASSERT_TOL(EXPR,VAL,TOL) if(abs((EXPR)-(VAL))>TOL){\
		fprintf(stderr,"ERROR: value of '%s' = %f, should be %f, error is %f!\n", #EXPR, EXPR, VAL,(EXPR)-(VAL));exit(1);\
	}else{\
		fprintf(stderr,"    OK, %s = %8.2e within %.2f%% err\n",#EXPR,VAL,((EXPR)-(VAL))/(VAL)*100);\
	}

int main(void){
	double rho, T, p, h, u;
	const HelmholtzData *d;

	d = &helmholtz_data_ammonia;

	//ASSERT_TOL(helmholtz_p(273.15+-40.,694.67,d), 10E6, 1E3);
	//ASSERT_TOL(helmholtz_p(273.15+-20.,670.55,d), 10E6, 1E3);
	//ASSERT_TOL(helmholtz_p(273.15+50,573.07,d), 10E6, 1E3);
	//ASSERT_TOL(helmholtz_p(273.15+110,441.77,d), 10E6, 1E3);

	fprintf(stderr,"PRESSURE TESTS\n");

	fprintf(stderr,"p(T,rho) = 1 MPa\n");
	ASSERT_TOL(helmholtz_p(273.15+150,4.9817,d), 1E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+200,4.4115,d), 1E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+350,3.3082,d), 1E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+420,2.9670,d), 1E6, 1E3);

	fprintf(stderr,"p(T,rho) = 10 MPa\n");
	ASSERT_TOL(helmholtz_p(273.15+150,74.732,d), 10E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+200,54.389,d), 10E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+350,35.072,d), 10E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+420,30.731,d), 10E6, 1E3);

	fprintf(stderr,"p(T,rho) = 20 MPa\n");
	ASSERT_TOL(helmholtz_p(273.15+150,359.41,d), 20E6, 1E4);
	ASSERT_TOL(helmholtz_p(273.15+200,152.83,d), 20E6, 1E4);
	ASSERT_TOL(helmholtz_p(273.15+350,74.590,d), 20E6, 1E4);
	ASSERT_TOL(helmholtz_p(273.15+420,63.602,d), 20E6, 1E4);

	//fprintf(stderr,"IDEAL HELMHOLTZ COMPONENT\n");
	//ASSERT_TOL(helm_ideal(273.15, 0) 

	fprintf(stderr,"INTERNAL ENERGY TESTS\n");
	fprintf(stderr,"u(T,rho) at p = 1 MPa\n");
	ASSERT_TOL(helmholtz_u(273.15+  0,0.76124,d), 1635.7e3, 0.1e3);
	ASSERT_TOL(helmholtz_u(273.15+ 50,0.63869,d), 1744.0e3, 0.1e3);
	ASSERT_TOL(helmholtz_u(273.15+200,0.43370,d), 2087.0e3, 0.1e3);
	ASSERT_TOL(helmholtz_u(273.15+300,0.35769,d), 2340.0e3, 0.1e3);
	ASSERT_TOL(helmholtz_u(273.15+420,0.29562,d), 2674.3e3, 0.1e3);

	fprintf(stderr,"u(T,rho) at p = 20 MPa\n");
	ASSERT_TOL(helmholtz_u(273.15+150,359.41,d), 1162.5e3, 0.1e3);
	ASSERT_TOL(helmholtz_u(273.15+200,152.83,d), 1662.9e3, 0.1e3);
	ASSERT_TOL(helmholtz_u(273.15+350,74.590,d), 2393.4e3, 0.1e3);
	ASSERT_TOL(helmholtz_u(273.15+420,63.602,d), 2611.8e3, 0.1e3);

	fprintf(stderr,"u(T,rho) at p = 1 MPa\n");
	ASSERT_TOL(helmholtz_u(273.15+150,4.9817,d), 1949.1e3, 0.1e3);
	ASSERT_TOL(helmholtz_u(273.15+200,4.4115,d), 2072.7e3, 0.1e3);
	ASSERT_TOL(helmholtz_u(273.15+350,3.3082,d), 2468.2e3, 0.1e3);
	ASSERT_TOL(helmholtz_u(273.15+420,2.9670,d), 2668.6e3, 0.1e3);

	fprintf(stderr,"u(T,rho) at p = 10 MPa\n");
	ASSERT_TOL(helmholtz_u(273.15+150,74.732,d), 1688.5e3, 0.1e3);
	ASSERT_TOL(helmholtz_u(273.15+200,54.389,d), 1908.0e3, 0.1e3);
	ASSERT_TOL(helmholtz_u(273.15+350,35.072,d), 2393.4e3, 0.1e3);
	ASSERT_TOL(helmholtz_u(273.15+420,30.731,d), 2611.8e3, 0.1e3);
	

	fprintf(stderr,"Tests completed OK\n");
	exit(0);
}
#endif
