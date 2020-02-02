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
	Property data for sodium, with test cases.
	This file was created by John Pye 18 Jan 2020
*/
#define SODIUM_SOURCE \
	"J. Fink and L. Leibowitz (1995). \"Thermodynamic and transport " \
	"properties of sodium liquid and vapor', technical report ANL/RE-95/2, " \
	"Reactor Engineering Division, Argonne National Laboratories, " \
	"http://www.ne.anl.gov/eda/ANL-RE-95-2.pdf"
#define SODIUM_SOURCEURL \
	"http://www.ne.anl.gov/eda/ANL-RE-95-2.pdf"

#include "../incomp.h"
#ifndef CUNIT_TEST

#define SODIUM_M 22.98977 /* kg/kmol */
#define SODIUM_R (8314.472/SODIUM_M) /* J/kg/K */
#define SODIUM_TC 2503.7 /* K */
#define SODIUM_RHOC  219. /* kg/m3 */

// 1.6582 - 8.4790e-4 * T + 4.4541e-7 * T**2 - 2992.6 * T**-2;
static const IncompressibleData incomp_data_sodium = {
	.M = SODIUM_M
	,.cp0={
		1000. /* cp0star */
		, 1. /* Tstar */
		, 4 /* power terms */
		, (const Cp0PowTerm[]){
			{1.6582,      0}
			,{-8.4790e-4, 1}
			,{4.4541e-7,  2}
			,{-2992.6,    -2}
		}
		, 0 /* exponential terms */
	}
	,.rho={
		.Tstar = SODIUM_TC
		,.rhostar = 1.
		,.type = FPROPS_DENS_1MT
		,.np = 3
		,.pt = (const DensityTerm[]){
			{ SODIUM_RHOC, 0. }
			,{ 275.32, 1. }
			,{ 511.58, 0.5 }
		}
	}
	,.ref={FPROPS_REF_TPHS,.data={.tphs={371.,101.325e3,206.7e3,0}}}
};

static const ThermalConductivityData thcond_data_sodium = {
	.source = SODIUM_SOURCE " " SODIUM_SOURCEURL
	,.type = FPROPS_THCOND_POLY
	,.data = {.poly=(ThCondPoly){
		.np = 4
		,.pt = (const ThCondPolyTerm[]){
			{124.67, 0}
			,{-0.11381, 1}
			,{5.5226e-5, 2}
			,{-1.1842e-8, 3}
		}
		,.Tstar = 1
		,.kstar = 1
	}}
};

static const ViscosityData visc_data_sodium = {
	.source = SODIUM_SOURCE " " SODIUM_SOURCEURL
	,.type = FPROPS_VISC_EPT
	,.data = {.ept=(ViscDataEpt){
		.mu_star = 1
		,.np = 2
		,.pt = (const ViscPowTerm[]){
			{-6.4406, 0}
			,{556.835, -1}
		}
		,.b = -0.3958
		,.is_ln = 1
	}}
};

const EosData eos_sodium = {
	"sodium"
	,SODIUM_SOURCE
	,SODIUM_SOURCEURL
	,100
	,FPROPS_INCOMP
	,.data = {.incomp = &incomp_data_sodium}
	,.thcond = &thcond_data_sodium
	,.visc  = &visc_data_sodium
};

#else
//#define TEST_VERBOSE

# include "../test.h"
# include "../refstate.h"
# include "../fprops.h"
# include "../incomp.h"
# include "../rundata.h"
# include "../thcond.h"

extern const EosData eos_sodium;

/* Some test data from Fink & Liebowitz */

typedef struct TestDataTrho_struct{
	double T, rho, lam, h, s, mu;
} TestDataTrho;

/*
Data points for T, rho, k, h are from Fink & Liebowitz.
Values of 's' were calculated with models/johnpye/liquidsodium.a4c.
*/

static const TestDataTrho td[] = {
    /* Temperature, Density, Conductivity, Enthalpy, Entropy, Viscosity */
    /* (K),         (kg/m3), (W/m/K),      J/kg,     J/kgK    Pa.s */
    {400.,   919., 87.22, 247e3,  0.53326716242e3, 5.99e-4 }
	,{600.,  874., 73.70, 514e3,  1.07537491829e3, 3.21e-4 }
	,{800.,  828., 62.90, 769e3,  1.44336831041e3, 2.27e-4 }
	,{1000., 781., 54.24, 1020e3, 1.72313707816e3, 1.81e-4 }
	,{1200., 732., 47.16, 1273e3, 1.95341568058e3, 1.53e-4 }
	,{1400., 680., 41.08, 1534e3, 2.15497925873e3, 1.35e-4 }
	,{1800., 568., 29.68, 2113e3, 2.51730964576e3, 1.12e-4 }
	//,{2503.7,219., 0.05,  4294e3 } // enthalpy will not agree at >2000K
};

static const unsigned ntd = sizeof(td)/sizeof(TestDataTrho);

void test_fluid_sodium(void){
	double maxerr = 0;
	FpropsError err;

	PureFluid *P = fprops_prepare(&eos_sodium,NULL);
#if 0
	ASSERT(NULL != P);

	thcond_prepare(P, eos_sodium.thcond, &err);
	if(err){
		TEST_MSG("Failed to prepare thermal conductivity");
	}

	visc_prepare(
#endif

	// refprop test test is evaluated with NBP refstate.
	ASSERT(NULL != P->thcond);

	//TEST_MSG("P->data->corr.incomp = %p",P->data->corr.incomp);
	ASSERT(3 == P->data->corr.incomp->rho.np);
	//ASSERT(NULL != P->data->corr.incomp->rho.pt);

	//ReferenceState R = {FPROPS_REF_IIR};
	//int res = fprops_set_reference_state(P, &R);
	//ASSERT(res == 0);

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
		double lam = thcond1_lam_poly(S.vals.Tp.T,&(S.fluid->thcond->data.poly),&err);
		ASSERT_TOL_VAL(lam,td[i].lam,0.008);
	}

	for(int i=0; i<ntd; ++i){
		double p = 1;
		double T = td[i].T;
		FluidState2 S = fprops_set_Tp(T,p,P,&err);
		//TEST_MSG("T = %f",T);
		double h = incomp_h(S.vals,S.fluid->data,&err);
		ASSERT_TOL_VAL(h,td[i].h,0.5e3);
	}

	// check the high-level interface through 'fprops_T' etc.
	FluidState2 S = fprops_set_Tp(400.,1e5,P,&err);
	ASSERT_PROP(T,S,&err,400.,1e-20);
	ASSERT_PROP(p,S,&err,1e5,1e-20);
	ASSERT_PROP(h,S,&err,247e3,0.5e3);
	ASSERT_PROP(rho,S,&err,919.,0.5);
	ASSERT_PROP(lam,S,&err,87.22,0.005);
	ASSERT_PROP(v,S,&err,1/919.,0.03e-2);

	// check entropy (versus data from our own ASCEND model, only)
	ReferenceState ref1 = {FPROPS_REF_TPHS,{.tphs={273.15,1e5,0,0}}};
	fprops_set_reference_state(P,  &ref1);
	for(int i=0; i<ntd; ++i){
		double p = 1;
		double T = td[i].T;
		FluidState2 S = fprops_set_Tp(T,p,P,&err);
		//TEST_MSG("T = %f",T);
		double s = incomp_s(S.vals,S.fluid->data,&err);
		ASSERT_TOL_VAL(s,td[i].s,1e-5);
	}

	// check viscosity
	for(int i=0; i<ntd; ++i){
		double p = 1;
		double T = td[i].T;
		FluidState2 S = fprops_set_Tp(T,p,P,&err);
		//TEST_MSG("T = %f",T);
		double mu = fprops_mu(S,&err);
		ASSERT_TOL_VAL(mu,td[i].mu,0.005e-4);
	}

}

#endif
