/* This file is created by John Pye 18 Jan 2020

LITERATURE REFERENCE
Source data is J. Fink and L. Leibowitz (1995). "Thermodynamic and transport
properties of sodium liquid and vapor', technical report ANL/RE-95/2,
Reactor Engineering Division, Argonne National Laboratories,
http://www.ne.anl.gov/eda/ANL-RE-95-2.pdf
*/

#include "../incomp.h"
#ifndef CUNIT_TEST

#define SODIUM_M 22.98977 /* kg/kmol */
#define SODIUM_R (8314.472/SODIUM_M) /* J/kg/K */
#define SODIUM_TC 591.75 /* K */
#define SODIUM_RHOC  219. /* kg/m3 */

static const IncompressibleData incomp_data_sodium = {
	.M = SODIUM_M
//	,.R = SODIUM_R
	,.cp0={
		SODIUM_R /* cp0star */
		, 1. /* Tstar */
		, 1 /* power terms */
		, (const Cp0PowTerm[]){
			{4.0,	0.0}
		}
		, 5 /* exponential terms */
		, (const Cp0ExpTerm[]){
			{1.6994,	190.0}
			,{8.0577,	797.0}
			,{17.059,	1619.0}
			,{8.4567,	3072.0}
			,{8.6423,	7915.0}
		}
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
	,.ref={FPROPS_REF_TPHS,.data={.tphs={298.15,101.325e3,0,0}}}
};

const EosData eos_sodium = {
	"sodium"
	,"J. Fink and L. Leibowitz (1995). 'Thermodynamic and transport properties of sodium "
	"liquid and vapor', technical report ANL/RE-95/2, Reactor Engineering Division, "
	"Argonne National Laboratories"
	,"http://www.ne.anl.gov/eda/ANL-RE-95-2.pdf"
	,100
	,FPROPS_INCOMP
	,.data = {.incomp = &incomp_data_sodium}
};

#else
# include "../test.h"
# include "../refstate.h"
# include "../fprops.h"

extern const EosData eos_sodium;

/* Some test data from Fink & Liebowitz */

typedef struct TestDataTrho_struct{
	double T, rho;
} TestDataTrho;

/*
A small set of data points calculated using REFPROP 8.0, for validation.
*/

static const TestDataTrho td[] = {
    /* Temperature, Density */
    /* (K), (kg/m3) */
    {400., 919.}
	,{600., 874.}
	,{800., 828.}
	,{1000., 781.}
	,{1200., 732.}
	,{1400., 680.}
	,{1800., 568.}
	,{2503.7, 219.}
};
static const unsigned ntd = sizeof(td)/sizeof(TestDataTrho);

void test_fluid_sodium(void){
	double maxerr = 0;

	PureFluid *P = incomp_prepare(&eos_sodium, NULL);
	// refprop test test is evaluated with NBP refstate.
	ASSERT(P);

	//ReferenceState R = {FPROPS_REF_IIR};
	FpropsError err;
	//int res = fprops_set_reference_state(P, &R);
	//ASSERT(res == 0);

	for(int i=0; i<ntd; ++i){
		double p = 1;
		double T = td[i].T;
		FluidState2 S = fprops_set_Tp(T,p,P,&err);
		ASSERT(err == FPROPS_NO_ERROR);
		ASSERT_PROP(rho,S,&err,td[i].rho,1e-1);
	}
}

#endif
