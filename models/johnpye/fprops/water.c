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
*/

#include "water.h"

#define WATER_R 461.51805 /* J/kg·K */
#define WATER_TSTAR 647.096 /* K */

/**
Ideal gas data for Water/Steam, from IAPWS-95.
http://www.iapws.org/relguide/IAPWS95.pdf
*/
const IdealData ideal_data_water = {
	-8.32044648201 /* constant */
	, 6.6832105268 /* linear */
	, WATER_TSTAR /* Tstar */
	, WATER_R /* cpstar J/kgK */
	, 1 /* power terms */	
	, (const IdealPowTerm[]){
		{1. + 3.00632,  1}
	}
	, 5 /* exponential terms */
	, (const IdealExpTerm []){
		/* b, beta */
		{0.012436, 1.28728967*WATER_TSTAR}
		,{0.97315,  3.53734222*WATER_TSTAR}
		,{1.27950, 7.74073708*WATER_TSTAR}
		,{0.96956, 9.24437796*WATER_TSTAR}
		,{0.24873, 27.5075105*WATER_TSTAR}
	}
};


/**
Residual (non-ideal) property data for Water/Steam, from IAPWS-95.
http://www.iapws.org/relguide/IAPWS95.pdf
*/
const HelmholtzData helmholtz_data_water = {
	/* R */ WATER_R /* J/kg/K */
	, /* M */ 0.00000000000000 /* kg/kmol  -- need to look up value cited by IAPWS */
	, /* rho_star */322. /* kg/m³ */
	, /* T_star */ WATER_TSTAR /* K */
	, &ideal_data_water
	, 51 /* np */
	, (const HelmholtzPowTerm[]){
		/* a_i, t_i, d_i, l_i */
		{0.12533547935523E-1, -0.5, 1, 0}
		,{0.78957634722828E1, 0.875, 1, 0}
		,{-0.87803203303561E1, 1, 1, 0}
		,{0.31802509345418, 0.5, 2, 0}
		,{-0.26145533859358, 0.75, 2, 0}
		,{-0.78199751687981E-2, 0.375, 3, 0}
		,{0.88089493102134E-2, 1, 4, 0}
		,{-0.66856572307965, 4, 1, 1}
		,{0.20433810950965, 6, 1, 1}
		,{-0.66212605039687E-4, 12, 1, 1}
		,{-0.19232721156002, 1, 2, 1}
		,{-0.25709043003438, 5, 2, 1}
		,{0.16074868486251, 4, 3, 1}
		,{-0.40092828925807E-1, 2, 4, 1}
		,{0.39343422603254E-6, 13, 4, 1}
		,{-0.75941377088144E-5, 9, 5, 1}
		,{0.56250979351888E-3, 3, 7, 1}
		,{-0.15608652257135E-4, 4, 9, 1}
		,{0.11537996422951E-8, 11, 10, 1}
		,{0.36582165144204E-6, 4, 11, 1}
		,{-0.13251180074668E-11, 13, 13, 1}
		,{-0.62639586912454E-9, 1, 15, 1}
		,{-0.10793600908932, 7, 1, 2}
		,{0.17611491008752E-1, 1, 2, 2}
		,{0.22132295167546, 9, 2, 2}
		,{-0.40247669763528, 10, 2, 2}
		,{0.58083399985759, 10, 3, 2}
		,{0.49969146990806E-2, 3, 4, 2}
		,{-0.31358700712549E-1, 7, 4, 2}
		,{-0.74315929710341, 10, 4, 2}
		,{0.47807329915480, 10, 5, 2}
		,{0.20527940895948E-1, 6, 6, 2}
		,{-0.13636435110343, 10, 6, 2}
		,{0.14180634400617E-1, 10, 7, 2}
		,{0.83326504880713E-2, 1, 9, 2}
		,{-0.29052336009585E-1, 2, 9, 2}
		,{0.38615085574206E-1, 3, 9, 2}
		,{-0.20393486513704E-1, 4, 9, 2}
		,{-0.16554050063734E-2, 8, 9, 2}
		,{0.19955571979541E-2, 6, 10, 2}
		,{0.15870308324157E-3, 9, 10, 2}
		,{-0.16388568342530E-4, 8, 12, 2}
		,{0.43613615723811E-1, 16, 3, 3}
		,{0.34994005463765E-1, 22, 4, 3}
		,{-0.76788197844621E-1, 23, 4, 3}
		,{0.22446277332006E-1, 23, 5, 3}
		,{-0.62689710414685E-4, 10, 14, 4}
		,{-0.55711118565645E-9, 50, 3, 6}
		,{-0.19905718354408, 44, 6, 6}
		,{0.31777497330738, 46, 6, 6}
		,{-0.11841182425981, 50, 6, 6}
	}
	, 0, 0 /* no exponential terms */
};


/*
	Test suite. These tests attempt to validate the current code using 
	a few sample figures output by REFPROP 7.0.

	To run the test, compile and run as follows:

	./test.py water
*/
#ifdef TEST

/* 
	some code from freesteam, http://freesteam.sf.net/, which has been thoroughly
	validated already.
*/

const double n0[] = {
    0.0 /* placeholder */,
    -8.32044648201, 6.6832105268, 3.00632 /* const, linear, ln(tau) coeffs */
	, 0.012436, 0.97315, 1.27950, 0.96956, 0.24873 /* exponential coeffs */
};

const double gamma0[] = {
    0.0, 0.0, 0.0, 0.0,
    1.28728967,
    3.53734222,
    7.74073708,
    9.24437796,
    27.5075105
};

enum Limits{
    eGamma1 = 4,
    eGamma2 = 9,
};

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "ideal_impl.h"

double phi0(const double delta, const double tau){
	int i;
    double sum = 0;
    for (i = eGamma1; i < eGamma2; i++)
    {
        sum += n0[i]*log(1-exp(-tau*gamma0[i]));
    }
    sum += log(delta) + n0[1] + n0[2]*tau + n0[3]*log(tau);
    return sum;
}

int main(void){
	double rho, T;
	const HelmholtzData *d;

	d = &helmholtz_data_water;
	double maxerr = 0;

/* a simple macro to actually do the testing */
#define ASSERT_TOL(FN,PARAM1,PARAM2,PARAM3,VAL,TOL) {\
		double cval; cval = FN(PARAM1,PARAM2,PARAM3);\
		double err; err = cval - (double)(VAL);\
		double relerrpc = (cval-(VAL))/(VAL)*100;\
		if(fabs(relerrpc)>maxerr)maxerr=fabs(relerrpc);\
		if(fabs(err)>fabs(TOL)){\
			fprintf(stderr,"ERROR in line %d: value of '%s(%f,%f,%s)' = %0.8f,"\
				" should be %f, error is %.10e (%.2f%%)!\n"\
				, __LINE__, #FN,PARAM1,PARAM2,#PARAM3, cval, VAL,cval-(VAL)\
				,relerrpc\
			);\
			exit(1);\
		}else{\
			fprintf(stderr,"    OK, %s(%f,%f,%s) = %8.2e with %.6f%% err.\n"\
				,#FN,PARAM1,PARAM2,#PARAM3,VAL,relerrpc\
			);\
		}\
	}


	fprintf(stderr,"phi0 TESTS\n");
	for(T = 300; T <= 900; T+= 100){
		for(rho = 900; rho >= 0.9; rho*=0.5){
			double delta = rho / d->rho_star;
			double tau = d->T_star / T;
			double p0 = phi0(delta,tau);
			
		 	ASSERT_TOL(helm_ideal, tau, delta, d->ideal, p0, p0*1e-5);
		}
	}

	fprintf(stderr,"Tests completed OK (maximum error = %0.2f%%)\n",maxerr);
	exit(0);
}

#endif

