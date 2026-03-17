/* Moran & Shapiro ideal-gas cp for CO2 (300-1000 K). */
#include "../filedata.h"

#ifndef CUNIT_TEST

#define CARBONDIOXIDE_MS_M 44.010
#define CARBONDIOXIDE_MS_R (R_UNIVERSAL / CARBONDIOXIDE_MS_M)

static const IdealData ideal_data_carbondioxide_ms = {
	IDEAL_CP0
	,.data = {.cp0 = {
		.cp0star = CARBONDIOXIDE_MS_R
		,.Tstar = 1
		,.np = 5
		,.pt = (const Cp0PowTerm[]){
			{2.401, 0}
			,{8.735e-3, 1}
			,{-6.607e-6, 2}
			,{2.002e-9, 3}
			,{0.0, 4}
		}
	}}
};

static const CubicData cubic_data_carbondioxide_ms = {
	.M = CARBONDIOXIDE_MS_M
	,.T_c = 1.0
	,.p_c = 0
	,.rho_c = 1.0
	,.T_t = 0
	,.omega = 0
	/* M&S G9e Table A-25, CO2(g) */
	,.ref0 = {FPROPS_REF_TPHS0, .data = {.tphs = {
		.T0 = 298.15
		, .p0 = 101325.0
		, .h0 = (-393520.0e3 / CARBONDIOXIDE_MS_M)
		, .s0 = (213.69e3 / CARBONDIOXIDE_MS_M)
	}}}
	,.ref = {FPROPS_REF_REF0}
	,.ideal = &ideal_data_carbondioxide_ms
};

static const ElementComp elements_carbondioxide_ms[] = {
	{"C", 1}
	,{"O", 2}
};

const EosData eos_carbondioxide_ms = {
	"carbondioxide"
	,"Moran and Shapiro"
	,NULL
	,100
	,FPROPS_CUBIC
	,.data = {.cubic = &cubic_data_carbondioxide_ms}
	,.elements = elements_carbondioxide_ms
	,.nelements = 2
};

#else
# include "../test.h"
extern const EosData eos_carbondioxide_ms;
void test_fluid_carbondioxide_ms(void){
	TEST_MSG("No test data for carbondioxide_ms");
}
#endif
