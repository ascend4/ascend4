/* Moran & Shapiro ideal-gas cp for H2 (300-1000 K). */
#include "../filedata.h"

#ifndef CUNIT_TEST

#define HYDROGEN_MS_M 2.016
#define HYDROGEN_MS_R (R_UNIVERSAL / HYDROGEN_MS_M)

static const IdealData ideal_data_hydrogen_ms = {
	IDEAL_CP0
	,.data = {.cp0 = {
		.cp0star = HYDROGEN_MS_R
		,.Tstar = 1
		,.np = 5
		,.pt = (const Cp0PowTerm[]){
			{3.057, 0}
			,{2.677e-3, 1}
			,{-5.810e-6, 2}
			,{5.521e-9, 3}
			,{-1.812e-12, 4}
		}
	}}
};

static const CubicData cubic_data_hydrogen_ms = {
	.M = HYDROGEN_MS_M
	,.T_c = 1.0
	,.p_c = 0
	,.rho_c = 1.0
	,.T_t = 0
	,.omega = 0
	/* M&S G9e Table A-25, H2(g) */
	,.ref0 = {FPROPS_REF_TPHG,{.tphg={298.0, 101325, (0.0 / HYDROGEN_MS_M), (0.0 / HYDROGEN_MS_M)}}}
	,.ref = {FPROPS_REF_REF0}
	,.ideal = &ideal_data_hydrogen_ms
};

static const ElementComp elements_hydrogen_ms[] = {
	{"H", 2}
};

const EosData eos_hydrogen_ms = {
	"hydrogen"
	,"Moran and Shapiro"
	,NULL
	,100
	,FPROPS_CUBIC
	,.data = {.cubic = &cubic_data_hydrogen_ms}
	,.elements = elements_hydrogen_ms
	,.nelements = 1
};

#else
# include "../test.h"
extern const EosData eos_hydrogen_ms;
void test_fluid_hydrogen_ms(void){
	TEST_MSG("No test data for hydrogen_ms");
}
#endif
