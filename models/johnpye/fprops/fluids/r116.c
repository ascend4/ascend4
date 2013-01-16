/* This file is created by Hongke Zhu, 7-23-2010. 
Chemical & Materials Engineering Department, 
University of Alabama in Huntsville, United States.

LITERATURE REFERENCE
Lemmon, E.W. and Span, R.,
"Short Fundamental Equations of State for 20 Industrial Fluids,"
J. Chem. Eng. Data, 51:785-850, 2006.
*/

#include "../helmholtz.h"

#define R116_M 138.01182 /* kg/kmol */
#define R116_R (8314.472/R116_M) /* J/kg/K */
#define R116_TC 293.03 /* K */

static const IdealData ideal_data_r116 = {
	IDEAL_CP0,{.cp0={
		R116_R /* cp0star */
		, 1. /* Tstar */
		, 1 /* power terms */
		, (const Cp0PowTerm[]){
			{4.0,	0.0}
		}
		, 3 /* exponential terms */
		, (const Cp0ExpTerm[]){
			{2.4818,    190.0}
			, {7.0622,    622.0}
			, {7.9951,   1470.0}
		}
	}}
};

static const HelmholtzData helmholtz_data_r116 = {
	/* R */ R116_R /* J/kg/K */
	, /* M */ R116_M /* kg/kmol */
	, /* rho_star */ 4.444*R116_M /* kg/m3(= rho_c for this model) */
	, /* T_star */ R116_TC /* K (= T_c for this model) */

	, /* T_c */ R116_TC
	, /* rho_c */ 4.444*R116_M /* kg/m3 */
	, /* T_t */ 173.1

	,{FPROPS_REF_PHI0,{.phi0={
		.c = -10.7088650331 /* constant */
		, .m = 8.9148979056 /* linear */
	}}}

	, 0.2566 /* acentric factor */
	, &ideal_data_r116
	, 12 /* power terms */
	, (const HelmholtzPowTerm[]){
		/* a_i, 	t_i, 	d_i, 	l_i */
		{1.1632,          0.25,    1.0,   0}
		, {-2.8123,         1.125,   1.0,   0}
		, {0.77202,         1.5,     1.0,   0}
		, {-0.14331,        1.375,   2.0,   0}
		, {0.10227,         0.25,    3.0,   0}
		, {0.00024629,      0.875,   7.0,   0}
		, {0.30893,         0.625,   2.0,   1}
		, {-0.028499,       1.75,    5.0,   1}
		, {-0.30343,        3.625,   1.0,   2}
		, {-0.068793,       3.625,   4.0,   2}
		, {-0.027218,      14.5,     3.0,   3}
		, {0.010665,       12.0,     4.0,   3}
	}
};

EosData eos_r116 = {
	"r116"
	,"Lemmon, E.W. and Span, R., Short Fundamental Equations of State for "
	" 20 Industrial Fluids, J. Chem. Eng. Data, 51:785-850, 2006."
	,NULL
	,100
	,FPROPS_HELMHOLTZ
	,.data = {.helm = &helmholtz_data_r116}
};

/*
    Test suite. These tests attempt to validate the current code using a few sample figures output by REFPROP 8.0. To compile and run the test:

    ./test.py r116
*/

#ifdef TEST

#include "../test.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>

const TestData td[]; const unsigned ntd;

int main(void){
	test_init();
	PureFluid *P = helmholtz_prepare(&eos_r116,NULL);
	return helm_run_test_cases(P, ntd, td, 'C');
}

/*
A small set of data points calculated using REFPROP 8.0, for validation. 
*/

const TestData td[] = {
    /* Temperature, Pressure, Density, Int. Energy, Enthalpy, Entropy, Cv, Cp, Cp0, Helmholtz */
    /* (C), (MPa), (kg/m3), (kJ/kg), (kJ/kg), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg-K), (kJ/kg) */
    {-1.00E+2, 1.E-1, 1.69815106958E+3, 9.44460681275E+1, 9.45049557034E+1, 5.31444232038E-1, 6.02245872415E-1, 8.9277375427E-1, 5.43355290597E-1, 2.42649935017E+0}
    , {-5.0E+1, 1.E-1, 7.65084923306E+0, 2.36332985808E+2, 2.49403430271E+2, 1.32643481862E+0, 5.87351713352E-1, 6.55968554511E-1, 6.41813261159E-1, -5.96609439672E+1}
    , {0.E+0, 1.E-1, 6.16295996921E+0, 2.67997710994E+2, 2.84223680414E+2, 1.46695626905E+0, 6.72383776938E-1, 7.36709196028E-1, 7.30367952819E-1, -1.32701393896E+2}
    , {5.0E+1, 1.E-1, 5.17715162634E+0, 3.03674859503E+2, 3.22990500074E+2, 1.59710775286E+0, 7.50052002738E-1, 8.12702353552E-1, 8.09232104364E-1, -2.12430510834E+2}
    , {1.00E+2, 1.E-1, 4.4690025561E+0, 3.42971356409E+2, 3.65347714164E+2, 1.71886505867E+0, 8.18180395825E-1, 8.80013154725E-1, 8.77851435699E-1, -2.98423140234E+2}
    , {1.50E+2, 1.E-1, 3.93362186995E+0, 3.85410910867E+2, 4.10832774814E+2, 1.83318037357E+0, 8.76451059477E-1, 9.37824111148E-1, 9.36351975155E-1, -3.90299364208E+2}
    , {-5.0E+1, 1.E+0, 1.47656993067E+3, 1.4161330564E+2, 1.42290550909E+2, 7.70372096458E-1, 6.66238991366E-1, 1.02056636017E+0, 6.41813261159E-1, -3.02952276846E+1}
    , {0.E+0, 1.E+0, 7.2263822065E+1, 2.62216052362E+2, 2.76054235445E+2, 1.30638124999E+0, 6.97794516828E-1, 8.29240293672E-1, 7.30367952819E-1, -9.46219860726E+1}
    , {5.0E+1, 1.E+0, 5.59584594346E+1, 2.99941380076E+2, 3.1781177912E+2, 1.44670773121E+0, 7.60519101931E-1, 8.51984324686E-1, 8.09232104364E-1, -1.67562223266E+2}
    , {1.00E+2, 1.E+0, 4.6658037051E+1, 3.40211164992E+2, 3.61643699732E+2, 1.5727372451E+0, 8.23603697222E-1, 9.02208438161E-1, 8.77851435699E-1, -2.46655738018E+2}
    , {1.50E+2, 1.E+0, 4.03250468437E+1, 3.83222123757E+2, 4.08020607039E+2, 1.68930574424E+0, 8.79642028909E-1, 9.52176540675E-1, 9.36351975155E-1, -3.31607601917E+2}
    , {-5.0E+1, 1.E+1, 1.53426527355E+3, 1.37663556062E+2, 1.44181333771E+2, 7.52080337631E-1, 6.69562890128E-1, 9.7380857819E-1, 6.41813261159E-1, -3.01631712804E+1}
    , {0.E+0, 1.0E+1, 1.30180980883E+3, 1.88220960355E+2, 1.95902574007E+2, 9.60784476685E-1, 7.37229554475E-1, 1.10153879856E+0, 7.30367952819E-1, -7.42173194512E+1}
    , {5.0E+1, 1.0E+1, 9.9499708006E+2, 2.4499009068E+2, 2.5504037143E+2, 1.15920744206E+0, 8.07114418735E-1, 1.265775989E+0, 8.09232104364E-1, -1.29607794222E+2}
    , {1.00E+2, 1.0E+1, 6.61938540708E+2, 3.04581217576E+2, 3.19688360286E+2, 1.34521786682E+0, 8.64643912731E-1, 1.26473345012E+0, 8.77851435699E-1, -1.97386829428E+2}
    , {1.50E+2, 1.E+1, 4.76012326025E+2, 3.58664837639E+2, 3.79672697002E+2, 1.49622854239E+0, 9.07063969374E-1, 1.14960834603E+0, 9.36351975155E-1, -2.74464270075E+2}
};

const unsigned ntd = sizeof(td)/sizeof(TestData);

#endif
