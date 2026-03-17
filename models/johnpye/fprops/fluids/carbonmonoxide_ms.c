/* Moran & Shapiro ideal-gas cp for CO (300-1000 K). */
#include "../filedata.h"

#ifndef CUNIT_TEST

#define CARBONMONOXIDE_MS_M 28.010
#define CARBONMONOXIDE_MS_R (R_UNIVERSAL / CARBONMONOXIDE_MS_M)

static const IdealData ideal_data_carbonmonoxide_ms = {
	IDEAL_CP0
	,.data = {.cp0 = {
		.cp0star = CARBONMONOXIDE_MS_R
		,.Tstar = 1
		,.np = 5
		,.pt = (const Cp0PowTerm[]){
			{3.710, 0}
			,{-1.619e-3, 1}
			,{3.692e-6, 2}
			,{-2.032e-9, 3}
			,{0.240e-12, 4}
		}
	}}
};

static const CubicData cubic_data_carbonmonoxide_ms = {
	.M = CARBONMONOXIDE_MS_M
	,.T_c = 1.0
	,.p_c = 0
	,.rho_c = 1.0
	,.T_t = 0
	,.omega = 0
	/* M&S G9e Table A-25, CO(g) */
	,.ref0 = {FPROPS_REF_TPHS0, .data = {.tphs = {
		.T0 = 298.15
		, .p0 = 101325.0
		, .h0 = (-110530.0e3 / CARBONMONOXIDE_MS_M)
		, .s0 = (197.54e3 / CARBONMONOXIDE_MS_M)
	}}}
	,.ref = {FPROPS_REF_REF0}
	,.ideal = &ideal_data_carbonmonoxide_ms
};

static const ElementComp elements_carbonmonoxide_ms[] = {
	{"C", 1}
	,{"O", 1}
};

const EosData eos_carbonmonoxide_ms = {
	"carbonmonoxide"
	,"Moran and Shapiro"
	,NULL
	,100
	,FPROPS_CUBIC
	,.data = {.cubic = &cubic_data_carbonmonoxide_ms}
	,.elements = elements_carbonmonoxide_ms
	,.nelements = 2
};

#else
# include "../test.h"
extern const EosData eos_carbonmonoxide_ms;
void test_fluid_carbonmonoxide_ms(void){
	TEST_MSG("No test data for carbonmonoxide_ms");
}
#endif
