/*	ASCEND modelling environment
	Copyright (C) 2020 John Pye

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
*//* @file
	Property data for high-temperature chloride salt (data from NREL),
	with test cases. This file was created by John Pye 4 Feb 2020.
*/
#define CLSALT_SOURCE \
	"Pers. comm. from Devon Kesseli, NREL, 21 June 2019. SaltParams-6-21-19.csv"
#define CLSALT_SOURCEURL ""

#include "../incomp.h"
#ifndef CUNIT_TEST

#define CLSALT_M NAN /* UNKNOWN kg/kmol */

static const IncompressibleData incomp_data_chloridesalt = {
	.M = CLSALT_M
	,.cp0={
		1000. /* cp0star */
		, 1. /* Tstar */
		, 2 /* power terms */
		, (const Cp0PowTerm[]){
			{1.5388204685, 0}
			,{-0.00052799,  1}
		}
		, 0 /* exponential terms */
	}
	,.rho={
		.Tstar = 1.
		,.rhostar = 1.
		,.type = FPROPS_DENS_T
		,.np = 2
		,.pt = (const DensityTerm[]){
			{ 1992.99890, 0. }
			,{ -0.406000, 1. }
		}
	}
	,.ref={FPROPS_REF_TPHS,.data={.tphs={400+273.15,101.325e3,18989.2875,0}}}
};

static const ThermalConductivityData thcond_data_chloridesalt = {
	.source = CLSALT_SOURCE " " CLSALT_SOURCEURL
	,.type = FPROPS_THCOND_POLY
	,.data = {.poly={
		.np = 2
		,.pt = (const ThCondPolyTerm[]){
			{5.355150e-1, 0}
			,{-1e-4, 1}
		}
		,.Tstar = 1
		,.kstar = 1
	}}
};

static const ViscosityData visc_data_chloridesalt = {
	.source = CLSALT_SOURCE " " CLSALT_SOURCEURL
	,.type = FPROPS_VISC_EPT
	,.data = {.ept={
		.mu_star = 1
		,.np = 3
		,.pt = (const ViscPowTerm[]){
			{-8.09998306264701, 0}
			,{2.13731951E+03,  -1}
			,{3.39389663E-02, -2}
		}
		,.b = 2.36796774274476E-05
		,.is_ln = 1
	}}
};

const EosData eos_chloridesalt = {
	"chloridesalt"
	,CLSALT_SOURCE
	,CLSALT_SOURCEURL
	,100
	,FPROPS_INCOMP
	,.data = {.incomp = &incomp_data_chloridesalt}
	,.thcond = &thcond_data_chloridesalt
	,.visc  = &visc_data_chloridesalt
};

#else

//#define TEST_VERBOSE

# include "../test.h"
# include "../refstate.h"
# include "../fprops.h"
# include "../incomp.h"
# include "../rundata.h"
# include "../thcond.h"
# include "../solve_ph.h"

extern const EosData eos_chloridesalt;

/* Some test data from Fink & Liebowitz */

typedef struct TestDataTrho_struct{
	double T, rho, cp, lam, h, mu;
} TestDataTrho;

/*
Data points for T, rho, k, h are from Fink & Liebowitz.
Values of 's' were calculated with models/johnpye/liquidsodium.a4c.
*/

static const TestDataTrho td[] = {
    /* T,           rho,      cp,          lam,          h,           mu */
    /* (K),         (kg/m3),  J/kg/K       (W/m/K),      J/kg,        Pa.s */
    {400.+273.15,   1719.7,   1.183404e3,  0.4682,       18989.2875,  7.264709E-03 }
    ,{500.+273.15,   1679.1,  1.130605e3,  0.4582,       134689.7375, 4.818011E-03 }
    ,{600.+273.15,   1638.5,  1.077806e3,  0.4482,       245110.2875, 3.510504E-03 }
    ,{700.+273.15,   1597.9,  1.025007e3,  0.4382,       350250.9375, 2.729793E-03 }
    ,{780.+273.15,   1565.42, 0.9827678e3, 0.4302,       430561.9295, 2.310332E-03 }
};

static const unsigned ntd = sizeof(td)/sizeof(TestDataTrho);

void test_fluid_chloridesalt(void){
	double maxerr = 0;
	FpropsError err;

	PureFluid *P = fprops_prepare(&eos_chloridesalt,NULL);

	ASSERT(NULL != P->thcond);

	double p = 1e5;
#define TEST_PROP(PROP,TOL) \
	for(int i=0; i<ntd; ++i){ \
		double T = td[i].T; \
		FluidState2 S = fprops_set_Tp(T,p,P,&err); \
		double PROP = fprops_##PROP(S,&err);\
		ASSERT_TOL_VAL(PROP,td[i].PROP,TOL);\
	}

	TEST_PROP(rho,0.05);
	TEST_PROP(cp,0.0000005e3);
	TEST_PROP(h,0.003);
	TEST_PROP(lam,0.00005);
	TEST_PROP(mu,0.005e-3);

#if 0
	for(int i=0; i<ntd; ++i){
		double p = 1;
		double T = td[i].T;
		FluidState2 S = fprops_set_Tp(T,p,P,&err);

		double rho = incomp_rho(S.vals,S.fluid->data,&err);
		ASSERT_TOL_VAL(rho,td[i].rho,0.5);
	}

	for(int i=0; i<ntd; ++i){
		double p = 1;
		double T = td[i].T;
		FluidState2 S = fprops_set_Tp(T,p,P,&err);
		//TEST_MSG("T = %f",T);
		double h = incomp_h(S.vals,S.fluid->data,&err);
		ASSERT_TOL_VAL(h,td[i].h,0.5e3);
	}

	for(int i=0; i<ntd; ++i){
		double p = 1;
		double T = td[i].T;
		FluidState2 S = fprops_set_Tp(T,p,P,&err);
		//TEST_MSG("T = %f",T);
		double lam = thcond1_lam_poly(S.vals.Tp.T,&(S.fluid->thcond->data.poly),&err);
		ASSERT_TOL_VAL(lam,td[i].lam,0.008);
	}



/**
	// check the high-level interface through 'fprops_T' etc.
	FluidState2 S = fprops_set_Tp(400.,1e5,P,&err);
	ASSERT_PROP(T,S,&err,400.,1e-20);
	ASSERT_PROP(p,S,&err,1e5,1e-20);
	ASSERT_PROP(h,S,&err,247e3,0.5e3);
	ASSERT_PROP(rho,S,&err,919.,0.5);
	ASSERT_PROP(lam,S,&err,87.22,0.005);
	ASSERT_PROP(v,S,&err,1/919.,0.03e-2);
*/
	// check viscosity
	for(int i=0; i<ntd; ++i){
		double p = 1;
		double T = td[i].T;
		FluidState2 S = fprops_set_Tp(T,p,P,&err);
		//TEST_MSG("T = %f",T);
		double mu = fprops_mu(S,&err);
		ASSERT_TOL_VAL(mu,td[i].mu,0.005e-4);
	}
#endif

	// check solve_ph...
	p = 1e5; // works equally with 5e5,10e5... shouldn't have any effect.
	for(int i=0; i<ntd; ++i){
		double T = td[i].T;
		FluidState2 S = fprops_set_Tp(T,p,P,&err);
		double h = fprops_h(S,&err);
		if(err)TEST_MSG("failed to evaluate 'h': %s",fprops_error(err));
		FluidState2 S1 = fprops_solve_ph(p,h,P,&err);
		if(err)TEST_MSG("failed to solve (p,h): %s",fprops_error(err));
		double T1 = fprops_T(S1,&err);
		ASSERT_TOL_VAL(T1,T,1e-12);
	}

}

#endif
