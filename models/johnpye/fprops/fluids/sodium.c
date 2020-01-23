/* This file is created by John Pye 18 Jan 2020 */
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

static const IncompressibleData incomp_data_sodium = {
	.M = SODIUM_M
//	,.R = SODIUM_R
	,.cp0={

		// FIXME this is still the wrong data...?

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
	//,.ref={FPROPS_REF_TPHS,.data={.tphs={298.15,101.325e3,0,0}}}
	,.ref={FPROPS_REF_UNDEFINED}
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

const EosData eos_sodium = {
	"sodium"
	,SODIUM_SOURCE
	,SODIUM_SOURCEURL
	,100
	,FPROPS_INCOMP
	,.data = {.incomp = &incomp_data_sodium}
	,.thcond = &thcond_data_sodium
};

#else
# include "../test.h"
# include "../refstate.h"
# include "../fprops.h"
# include "../incomp.h"
# include "../rundata.h"
# include "../thcond.h"

double incomp_rho(FluidStateUnion vals, const FluidData *data, FpropsError *err);
PropEvalFn2 incomp_rho;

extern const EosData eos_sodium;

/* Some test data from Fink & Liebowitz */

typedef struct TestDataTrho_struct{
	double T, rho, lam;
} TestDataTrho;

/*
A small set of data points calculated using REFPROP 8.0, for validation.
*/

static const TestDataTrho td[] = {
    /* Temperature, Density, Conductivity */
    /* (K), (kg/m3), (W/m/K) */
    {400.,   919., 87.22 }
	,{600.,  874., 73.70 }
	,{800.,  828., 62.90 }
	,{1000., 781., 54.24 }
	,{1200., 732., 47.16 }
	,{1400., 680., 41.08 }
	,{1800., 568., 29.68 }
	,{2503.7,219., 0.05  }
};
static const unsigned ntd = sizeof(td)/sizeof(TestDataTrho);

void test_fluid_sodium(void){
	double maxerr = 0;

	PureFluid *P = incomp_prepare(&eos_sodium,NULL);

	FpropsError err;
	thcond_prepare(P, eos_sodium.thcond, &err);
	if(err){
		TEST_MSG("Failed to prepare thermal conductivity");
	}

	// refprop test test is evaluated with NBP refstate.
	ASSERT(NULL != P);
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
		double lam = thcond1_lam(S,&err);
		ASSERT_TOL_VAL(lam,td[i].lam,0.008);
	}

}

#endif
