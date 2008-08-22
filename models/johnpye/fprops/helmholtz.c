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
Baehr, 'Eine neue Fundamentalgleichung für Ammoniak', DKV-Tagungsbericht,
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

	THERE IS AN ERROR IN THIS FUNCTION.
*/	
double helm_resid_del(double tau,double delta, const HelmholtzData *data){
	
	double sum;
	double phir = 0;
	unsigned i;
	double X;
	double delX;
	double XdelX;

	const HelmholtzATD *atd = &(data->atd[0]);
	
	for(i=0; i<5; ++i){
		//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d\n",i+1, atd->a, atd->t, atd->d);
		phir += atd->a * pow(tau, atd->t) * pow(delta, atd->d - 1) * atd->d;
		++atd;
	}

	sum = 0;
	X = 1;
	delX = ipow(delta,X);
	XdelX = X*delX;
	for(i=5; i<10; ++i){
		//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d\n",i+1, atd->a, atd->t, atd->d);
		sum -= atd->a * pow(tau, atd->t) * pow(delta, atd->d - 1) * (XdelX - atd->d);
		++atd;
	}
	//fprintf(stderr,"sum = %f\n",sum);
	phir += exp(-delX) * sum;

	sum = 0; 
	X = 2;
	delX = ipow(delta,X);
	XdelX = X*delX;
	for(i=10; i<17; ++i){
		//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d\n",i+1, atd->a, atd->t, atd->d);
		sum -= atd->a * pow(tau, atd->t) * pow(delta, atd->d - 1) * (XdelX - atd->d);
		++atd;
	}
	//fprintf(stderr,"sum = %f\n",sum);
	phir += exp(-delX) * sum;

	sum = 0;
	X = 3;
	delX = ipow(delta,X);
	XdelX = X*delX;
	for(i=17; i<21; ++i){
		//fprintf(stderr,"i = %d, a = %e, t = %f, d = %d\n",i+1, atd->a, atd->t, atd->d);
		sum -= atd->a * pow(tau, atd->t) * pow(delta, atd->d - 1) * (XdelX - atd->d);
		++atd;
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
#define ASSERT_TOL(EXPR,VAL,TOL) {\
		double cval; cval = (EXPR);\
		double err; err = cval - (double)(VAL);\
		double relerrpc = (cval-(VAL))/(VAL)*100;\
		if(fabs(relerrpc)>maxerr)maxerr=fabs(relerrpc);\
		if(fabs(err)>TOL){\
			fprintf(stderr,"ERROR in line %d: value of '%s' = %f, should be %f, error is %f (%.2f%%)!\n"\
				, __LINE__, #EXPR, cval, VAL,cval-(VAL),relerrpc);\
			exit(1);\
		}else{\
			fprintf(stderr,"    OK, %s = %8.2e with %.2f%% err.\n",#EXPR,VAL,relerrpc);\
			/*fprintf(stderr,"        (err = %8.2e, tol = %8.2e, calc = %8.2e)\n",fabs(err),TOL,cval);*/\
		}\
	}

int main(void){
	double rho, T, p, h, u;
	const HelmholtzData *d;

	d = &helmholtz_data_ammonia;
	double maxerr = 0;

	fprintf(stderr,"ENTROPY TESTS\n");

	/* offset required to attain agreement with REFPROP */
	double Y = -471.596704;

	fprintf(stderr,"s(T,rho) at p = 0.1 MPa\n");
	ASSERT_TOL(helmholtz_s(273.15+-60, 713.65,d), Y+0.36737e3, 0.5);
	ASSERT_TOL(helmholtz_s(273.15+  0,0.76124,d), Y+6.8900e3, 0.5);
	ASSERT_TOL(helmholtz_s(273.15+ 50,0.63869,d), Y+7.2544e3, 0.5);
	ASSERT_TOL(helmholtz_s(273.15+200,0.43370,d), Y+8.1232e3, 0.5);
	ASSERT_TOL(helmholtz_s(273.15+300,0.35769,d), Y+8.6084e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+420,0.29562,d), Y+9.1365e3, 1);

	fprintf(stderr,"s(T,rho) at p = 1 MPa\n");
	ASSERT_TOL(helmholtz_s(273.15+-50,702.49,d), Y+0.56381e3, 0.5);
	ASSERT_TOL(helmholtz_s(273.15+150,4.9817,d), Y+6.7008e3, 0.5);
	ASSERT_TOL(helmholtz_s(273.15+200,4.4115,d), Y+6.9770e3, 0.5);
	ASSERT_TOL(helmholtz_s(273.15+350,3.3082,d), Y+7.7012e3, 0.5);
	ASSERT_TOL(helmholtz_s(273.15+420,2.9670,d), Y+8.0059e3, 0.5);

	fprintf(stderr,"s(T,rho) at p = 10 MPa\n");
	ASSERT_TOL(helmholtz_s(273.15+-70,728.11,d), Y+0.14196e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+-50,706.21,d), Y+0.54289e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+-20,670.55,d), Y+1.0975e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+  0,645.04,d), Y+1.4403e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+125.17,356.70,d), Y+3.5463e3, 1);

	ASSERT_TOL(helmholtz_s(273.15+125.17,121.58,d), Y+4.5150e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+200,54.389,d), Y+5.5906e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+350,35.072,d), Y+6.4850e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+420,30.731,d), Y+6.8171e3, 1);

	fprintf(stderr,"s(T,rho) at p = 20 MPa\n");
	ASSERT_TOL(helmholtz_s(273.15+-50,710.19,d), Y+0.52061e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+ 30,612.22,d), Y+1.8844e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+150,359.41,d), Y+3.7164e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+200,152.83,d), Y+4.8376e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+350,74.590,d), Y+6.0407e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+420,63.602,d), Y+6.4066e3, 1);

	fprintf(stderr,"s(T,rho) at p = 100 MPa\n");
	ASSERT_TOL(helmholtz_s(273.15+  0,690.41,d), Y+1.2158e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+100,591.07,d), Y+2.5499e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+250,437.69,d), Y+4.0264e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+420,298.79,d), Y+5.2620e3, 1);

	/* successful entropy tests means that helm_ideal_tau, helm_real_tau, helm_ideal and helm_resid are all OK */

	fprintf(stderr,"PRESSURE TESTS\n");

	fprintf(stderr,"p(T,rho) = 0.1 MPa\n");	
	ASSERT_TOL(helmholtz_p(273.15 -70,724.74783,d), 0.1E6,  1E3);
	ASSERT_TOL(helmholtz_p(273.15 -60,713.64815,d), 0.1E6,  1E3);
	ASSERT_TOL(helmholtz_p(273.15 -50,702.11130,d), 0.1E6,  1E3);
	ASSERT_TOL(helmholtz_p(273.15 -40,690.16351,d), 0.1E6,   1E3);
	ASSERT_TOL(helmholtz_p(273.15 -33.588341,682.29489,d), 0.1E6,1E3);
	ASSERT_TOL(helmholtz_p(273.15+  0,0.76123983,d), 0.1E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+100,0.55135,d), 0.1E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+250,0.39203,d), 0.1E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+420,0.29562,d), 0.1E6, 1E3);

	fprintf(stderr,"p(T,rho) = 1 MPa\n");
	ASSERT_TOL(helmholtz_p(273.15 -70,725.05815,d), 1E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+  0,638.97275,d), 1E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+ 30,7.5736465,d), 1E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+150,4.9816537,d), 1E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+200,4.4115,d), 1E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+350,3.3082,d), 1E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+420,2.9670,d), 1E6, 1E3);

	fprintf(stderr,"p(T,rho) = 10 MPa\n");
	ASSERT_TOL(helmholtz_p(273.15+-40.,694.67407,d), 10E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+-20.,670.54741,d), 10E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+50,573.07306,d), 10E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+110,441.76869,d), 10E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+150,74.732,d), 10E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+200,54.389,d), 10E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+350,35.072,d), 10E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+420,30.731,d), 10E6, 1E3);

	fprintf(stderr,"p(T,rho) = 20 MPa\n");
	ASSERT_TOL(helmholtz_p(273.15+150,359.40683,d), 20E6, 1E4);
	ASSERT_TOL(helmholtz_p(273.15+200,152.83430,d), 20E6, 1E4);
	ASSERT_TOL(helmholtz_p(273.15+350,74.590236,d), 20E6, 1E4);
	ASSERT_TOL(helmholtz_p(273.15+420,63.601873,d), 20E6, 1E4);

	//fprintf(stderr,"IDEAL HELMHOLTZ COMPONENT\n");
	//ASSERT_TOL(helm_ideal(273.15, 0) 

	fprintf(stderr,"ENTHALPY TESTS\n");

	/* this offset is required to attain agreement with values from REFPROP */
	double Z = -1635.7e3 + 1492.411e3;

	fprintf(stderr,"h(T,rho) at p = 0.1 MPa\n");
	ASSERT_TOL(helmholtz_h(273.15+-60, 713.65,d), Z+75.166e3, 0.2e3);
	ASSERT_TOL(helmholtz_h(273.15+  0,0.76124,d), Z+1635.7e3, 0.2e3);
	ASSERT_TOL(helmholtz_h(273.15+ 50,0.63869,d), Z+1744.0e3, 0.2e3);
	ASSERT_TOL(helmholtz_h(273.15+200,0.43370,d), Z+2087.0e3, 0.2e3);
	ASSERT_TOL(helmholtz_h(273.15+300,0.35769,d), Z+2340.0e3, 1e3);
	ASSERT_TOL(helmholtz_h(273.15+420,0.29562,d), Z+2674.3e3, 1e3);

	fprintf(stderr,"h(T,rho) at p = 1 MPa\n");
	ASSERT_TOL(helmholtz_h(273.15+150,4.9817,d), Z+1949.1e3, 1e3);
	ASSERT_TOL(helmholtz_h(273.15+200,4.4115,d), Z+2072.7e3, 1e3);
	ASSERT_TOL(helmholtz_h(273.15+350,3.3082,d), Z+2468.2e3, 1e3);
	ASSERT_TOL(helmholtz_h(273.15+420,2.9670,d), Z+2668.6e3, 1e3);

	fprintf(stderr,"h(T,rho) at p = 10 MPa\n");
	ASSERT_TOL(helmholtz_h(273.15+-50,706.21,d), Z+127.39e3, 2e3);
	ASSERT_TOL(helmholtz_h(273.15+-0,645.04,d), Z+349.53e3, 2e3);
	
	ASSERT_TOL(helmholtz_h(273.15+150,74.732,d), Z+1688.5e3, 1e3);
	ASSERT_TOL(helmholtz_h(273.15+200,54.389,d), Z+1908.0e3, 1e3);
	ASSERT_TOL(helmholtz_h(273.15+350,35.072,d), Z+2393.4e3, 1e3);
	ASSERT_TOL(helmholtz_h(273.15+420,30.731,d), Z+2611.8e3, 1e3);

	fprintf(stderr,"h(T,rho) at p = 20 MPa\n");
	ASSERT_TOL(helmholtz_h(273.15 -70,731.41,d), Z+51.734e3, 0.5e3);
	ASSERT_TOL(helmholtz_h(273.15 -60,721.00318,d), Z+93.871419e3, 0.5e3);
	ASSERT_TOL(helmholtz_h(273.15 -50,710.19289,d), Z+136.54351e3, 1e3);
	ASSERT_TOL(helmholtz_h(273.15 -40,699.02472,d), Z+179.72030e3, 0.5e3);
	ASSERT_TOL(helmholtz_h(273.15+ 30,612.22,d), Z+493.28e3, 0.5e3);
	ASSERT_TOL(helmholtz_h(273.15+150,359.40683,d), Z+1162.5e3, 0.5e3);
	ASSERT_TOL(helmholtz_h(273.15+200,152.83430,d), Z+1662.9e3, 0.5e3);
	ASSERT_TOL(helmholtz_h(273.15+250,106.31299,d), Z+1928.6499e3, 0.5e3);
	ASSERT_TOL(helmholtz_h(273.15+300,86.516941,d), Z+2128.9031e3, 0.5e3);
	ASSERT_TOL(helmholtz_h(273.15+330,78.784703,d), Z+2238.2416e3, 0.5e3);
	ASSERT_TOL(helmholtz_h(273.15+350,74.590236,d), Z+2308.8516e3, 10e3);
	ASSERT_TOL(helmholtz_h(273.15+420,63.601873,d), Z+2549.2872e3, 10e3);

	fprintf(stderr,"h(T,rho) at p = 100 MPa\n");
	ASSERT_TOL(helmholtz_h(273.15+  0,690.41,d), Z+422.69e3, 0.5e3);
	ASSERT_TOL(helmholtz_h(273.15+100,591.07,d), Z+850.44e3, 0.1e3);
	ASSERT_TOL(helmholtz_h(273.15+250,437.69,d), Z+1506.6e3, 1e3);
	ASSERT_TOL(helmholtz_h(273.15+420,298.79,d), Z+2252.3e3, 1e3);

	fprintf(stderr,"Tests completed OK (maximum error = %0.2f%%)\n",maxerr);
	exit(0);
}
#endif
