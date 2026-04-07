/* Moran & Shapiro ideal-gas cp for O2 (300-1000 K). */
#include "../filedata.h"

#ifndef CUNIT_TEST

#define OXYGEN_MS_M 31.999
#define OXYGEN_MS_R (R_UNIVERSAL / OXYGEN_MS_M)

static const IdealData ideal_data_oxygen_ms = {
	IDEAL_CP0
	,.data = {.cp0 = {
		.cp0star = OXYGEN_MS_R
		,.Tstar = 1
		,.np = 5
		,.pt = (const Cp0PowTerm[]){
			{3.626, 0}
			,{-1.878e-3, 1}
			,{7.055e-6, 2}
			,{-6.764e-9, 3}
			,{2.156e-12, 4}
		}
	}}
};

static const CubicData cubic_data_oxygen_ms = {
	.M = OXYGEN_MS_M
	,.T_c = 1.0
	,.p_c = 0
	,.rho_c = 1.0
	,.T_t = 0
	,.omega = 0
	/* M&S G9e Table A-25, O2(g) */
	,.ref0 = {FPROPS_REF_TPHS0, .data = {.tphs = {
		.T0 = 298.15
		, .p0 = 101325.0
		, .h0 = (0.0 / OXYGEN_MS_M)
		, .s0 = (205.03e3 / OXYGEN_MS_M)
	}}}
	,.ref = {FPROPS_REF_REF0}
	,.ideal = &ideal_data_oxygen_ms
};

static const ElementComp elements_oxygen_ms[] = {
	{"O", 2}
};

const EosData eos_oxygen_ms = {
	"oxygen"
	,"Moran and Shapiro"
	,NULL
	,100
	,FPROPS_CUBIC
	,.data = {.cubic = &cubic_data_oxygen_ms}
	,.elements = elements_oxygen_ms
	,.nelements = 1
};

#else
# include "../test.h"
extern const EosData eos_oxygen_ms;
void test_fluid_oxygen_ms(void){
	TEST_MSG("No test data for oxygen_ms");
}
#endif
