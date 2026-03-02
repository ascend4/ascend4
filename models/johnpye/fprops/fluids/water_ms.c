/* Moran & Shapiro ideal-gas cp for H2O (300-1000 K). */
#include "../filedata.h"

#ifndef CUNIT_TEST

#define WATER_MS_M 18.015
#define WATER_MS_R (R_UNIVERSAL / WATER_MS_M)

static const IdealData ideal_data_water_ms = {
	IDEAL_CP0
	,.data = {.cp0 = {
		.cp0star = WATER_MS_R
		,.Tstar = 1
		,.np = 5
		,.pt = (const Cp0PowTerm[]){
			{4.070, 0}
			,{-1.108e-3, 1}
			,{4.152e-6, 2}
			,{-2.964e-9, 3}
			,{0.807e-12, 4}
		}
	}}
};

static const CubicData cubic_data_water_ms = {
	.M = WATER_MS_M
	,.T_c = 1.0
	,.p_c = 0
	,.rho_c = 1.0
	,.T_t = 0
	,.omega = 0
	/* M&S G9e Table A-25, H2O(g) */
	,.ref0 = {FPROPS_REF_TPHG,{.tphg={298.0, 101325, (-241820.0 / WATER_MS_M), (-228590.0 / WATER_MS_M)}}}
	,.ref = {FPROPS_REF_REF0}
	,.ideal = &ideal_data_water_ms
};

static const ElementComp elements_water_ms[] = {
	{"H", 2}
	,{"O", 1}
};

const EosData eos_water_ms = {
	"water"
	,"Moran and Shapiro"
	,NULL
	,100
	,FPROPS_CUBIC
	,.data = {.cubic = &cubic_data_water_ms}
	,.elements = elements_water_ms
	,.nelements = 2
};

#else
# include "../test.h"
extern const EosData eos_water_ms;
void test_fluid_water_ms(void){
	TEST_MSG("No test data for water_ms");
}
#endif
