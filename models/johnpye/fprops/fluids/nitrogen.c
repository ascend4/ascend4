/*	ASCEND modelling environment
	Copyright (C) 2008-2009 Carnegie Mellon University

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
Property data for Nitrogen

From Span, Lemmon, Jacobsen & Wagner
A Reference Quality Equation of State for Nitrogen
Int J Thermophysics, Vol 18, No 4, 1998.

This is the nitrogren property correlation recommended
by NIST in its program REFPROP 7.0.
*/

#include "../fprops.h"
#include "../helmholtz.h"

#define NITROGEN_M 28.01348
#define NITROGEN_R (8.31451e3/NITROGEN_M)
#define NITROGEN_TC 126.192
#define NITROGEN_RHOC (11.1839 * NITROGEN_M)



static const IdealData ideal_data_nitrogen = {
	IDEAL_CP0
	,.data = {.cp0 = {
		.cp0star = NITROGEN_R
		, .Tstar = 1.
		, .np = 4 /* power terms */
		, .pt = (const Cp0PowTerm[]){
			{3.5,            0.}
			,{3.066469e-6,   1.}
			,{4.701240e-9,   2.}
			,{-3.987984e-13, 3.}
		}
		, .ne = 1 /* exponential terms */
		, .et = (const Cp0ExpTerm[]){
			{1.012941, 3364.011}
		}
	}}
};

/*
	this correlation is given in molar terms. we convert to mass
	basis by changing the scaling density rho_star, and the ideal gas constant.
*/
static const HelmholtzData helmholtz_data_nitrogen = {
	.R = NITROGEN_R /* 1000 * kJ/kmolK / kg/kmol = J/kgK */
	, .M = NITROGEN_M /* kg/kmol */
	, .rho_star = NITROGEN_RHOC /* kmol/m3 * kg/kmol = kg/m³ (= rho_c for this model) */
	, .T_star = NITROGEN_TC /* K (= T_c for this model) */

	, .T_c = NITROGEN_TC
	, .rho_c = NITROGEN_RHOC
	, .T_t = 63.151

#if 0
	// values from the Span paper, REFPROP agreement isn't perfect with this values
	, .ref = {
		FPROPS_REF_PHI0
		, .data = {.phi0 = {
			.c = -12.76953
			, .m = -0.0007841630
		}}
	}
#else
	, .ref = {FPROPS_REF_TRHS, {.trhs={
		323.15, 1.042637745E+0, 3.353055943E+5, 6.923064311E+3
	}}}
#endif

	, .omega = 0.039 /* acentric factor, from Reid, Prausnitz & Polling */
	, .ideal = &ideal_data_nitrogen
	, .np = 32
	, .pt = (const HelmholtzPowTerm[]){
		/* a_i, t_i, d_i, l_i */
		{ 0.924803575275,      0.25,    1,  0}/* 1 */
		,{-0.492448489428,     0.875,   1,  0}
		,{ 0.661883336938,     0.5,     2,  0}
		,{-0.192902649201E1,   0.875,   2,  0}
		,{-0.622469309629E-1,  0.375,   3,  0}
		,{ 0.349943957581,     0.75,    3,  0}
		,{ 0.564857472498,     0.5,     1,  1}
		,{-0.161720005987E1,   0.75,    1,  1}
		,{-0.481395031883,     2.,      1,  1}
		,{ 0.421150636384,     1.25,    3,  1}/* 10 */
		,{-0.161962230825E-1,  3.5,     3,  1}
		,{ 0.172100994165,     1.,      4,  1}
		,{ 0.735448924933E-2,  0.5,     6,  1}
		,{ 0.168077305479E-1,  3.,      6,  1}
		,{-0.107626664179E-2,  0.,      7,  1}
		,{-0.137318088513E-1,  2.75,    7,  1}
		,{ 0.635466899859E-3,  0.75,    8,  1}
		,{ 0.304432279419E-2,  2.5,     8,  1}
		,{-0.435762336045E-1,  4.,      1,  2}
		,{-0.723174889316E-1,  6. ,     2,  2}/* 20 */
		,{ 0.389644315272E-1,  6.,      3,  2}
		,{-0.212201363910E-1,  3.,      4,  2}
		,{ 0.408822981509E-2,  3.,      5,  2}
		,{-0.551990017984E-4,  6.,      8,  2}
		,{-0.462016716479E-1, 16.,      4,  3}
		,{-0.300311716011E-2, 11.,      5,  3}
		,{ 0.368825891208E-1, 15.,      5,  3}
		,{-0.255856846220E-2, 12.,      8,  3}
		,{ 0.896915264558E-2, 12.,      3,  4}
		,{-0.441513370350E-2,  7.,      5,  4}/* 30 */
		,{ 0.133722924858E-2,  4.,      6,  4}
		,{ 0.264832491957E-3, 16.,      9,  4}
	}
	, .ng = 4 /* gaussian terms */
	, .gt = (const HelmholtzGausTerm[]){
		/* n, t, d, alpha, beta, gamma, epsilon */
		{0.196688194015e2,    0., 1, 20, 325, 1.16, 1}
		, {-0.209115600730e2, 1., 1, 20, 325, 1.16, 1}
		, {0.167788306989e-1, 2., 3, 15, 300, 1.13, 1}
		, {0.262767566274e4,  3., 2, 25, 275, 1.25, 1}
	}
};

const ViscosityData visc_nitrogen = {
	.source="E W Lemmon and R T Jacobsen, 2004. 'Viscosity and Thermal Conductivity Equations for Nitrogen, Oxygen, Argon, and Air', Int J Thermophys 25(1), pp. 21-69."
	,.type=FPROPS_VISC_1
	,.data={.v1={
		.mu_star = 1e-6
		,.T_star = NITROGEN_TC
		,.rho_star = NITROGEN_RHOC
		,.sigma = 0.3656
		,.M = 28.01348
		,.eps_over_k = 98.94
		,.ci={
			FPROPS_CI_1
			,.data={.ci1={
				.nt=5
				,.t=(const ViscCI1Term[]){
					{0, 0.431}
					,{1, -0.4623}
					,{2, 0.08406}
					,{3, 0.005341}
					,{4, -0.00331}
				}
			}}
		}
		,.nt=5
		,.t=(const ViscData1Term[]){
			{10.72, 0.1, 2, 0}
			,{0.03989, 0.25, 10, 1}
			,{0.001208, 3.2, 12, 1}
			,{-7.402, 0.9, 2, 2}
			,{4.620, 0.3, 1, 3}
		}
	}}
};

const ThermalConductivityData thcond_nitrogen = {
	.source = "Lemmon and Jacobsen, 2003. Int J Thermophys 25(1)."
	,.type=FPROPS_THCOND_1
	,.data={.k1={
		.k_star = 1e-3
		,.T_star = 126.192
		,.rho_star = 11.1839
		,.v1=&(visc_carbondioxide.data.v1)
		,.eps_over_k = 98.94
		,.nc = 6
		,.ct=(const ThCondCSTerm[]){
			{0, 0.4226159}
			,{1, 0.6280115}
			,{2, -0.5387661}
			,{3, 0.6735941}
			,{6, -0.4362677}
			,{7, 0.2255388}
		}
		,.nr=6
		,.rt=(const ThCondData1Term[]){
			{8.862,        0.,   1, 0}
			,{31.11,       0.03, 2, 0}
			,{-73.13,      0.2,  3, 1}
			,{20.03,       0.8,  4, 2}
			,{-0.7096,     0.6,  8, 2}
			,{0.2672,      1.9, 10, 2}
		}
		,.crit = NULL
	}}
};


EosData eos_nitrogen = {
	"nitrogen"
	,"Span, Lemmon, Jacobsen & Wagner, A Reference Quality Equation of State "
	"for Nitrogen, Int J Thermophysics, Vol 18, No 4, 1998."
	,NULL
	,100
	,FPROPS_HELMHOLTZ
	,.data = {.helm = &helmholtz_data_nitrogen}
	,.visc = &visc_nitrogen
	,.thcond = &thcond_nitrogen;
};


/*
	Test suite. These tests attempt to validate the current code using
	a few sample figures output by REFPROP 7.0.

	To run the test, compile and run as follows:

	gcc ideal.c helmholtz.c nitrogen.c -DTEST -o nitrogen -lm && ./nitrogen
*/
#ifdef TEST

#include "../ideal_impl.h"
#include "../visc.h"
#include "../test.h"
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

const TestData td[]; const unsigned ntd;

int main(void){
	test_init();
	PureFluid *P = helmholtz_prepare(&eos_nitrogen,NULL);
	//PureFluid *PI = ideal_prepare(&eos_nitrogen,NULL);
	ASSERT(P);

	FpropsError err=FPROPS_NO_ERROR;
#define D P->data
	double rho, T, p, u, h, a, s, cp0, w, mu;

	double maxerr = 0;

	unsigned i;
	const unsigned n = ntd;

	//cp0 = fprops_cp(273.15,0,PI,&err);
	//fprintf(stderr,"cp0(2731.5) = %f\n",cp0);

	fprintf(stderr,"CP0 TESTS\n");
	for(i=0; i<n;++i){
		cp0 = td[i].cp0*1e3;
#define CP0_TEMP(T,RHO,DATA, ERROR) ideal_cp(T,RHO,DATA, ERROR)
	 	ASSERT_TOL(CP0_TEMP, td[i].T+273.15, 0., P->data, &err, cp0, cp0*1e-6);
#undef CP0_TEMP
	}

	fprintf(stderr,"\nTesting sample values from the Span paper...\n");

	//ReferenceState ref = {FPROPS_REF_IIR};
	//fprops_set_reference_state(P, &ref);

	rho = 11 * D->M; T = 270.;
	FluidState S = fprops_set_Trho(T,rho, P, &err);
	ASSERT(!err);
	p = fprops_p(S, &err);
	fprintf(stderr,"p = %f\n", p);
	ASSERT(fabs(p - 27.0621e6) < 0.00005e6);
	h = fprops_h(S, &err);
	fprintf(stderr,"h = %f (should = %f)\n", h, 6517.95*1e3/D->M);
	//ASSERT(fabs(h - 6517.95 *1e3/ D->M) < 5e-3*1e3 / D->M);
	w = fprops_w(S, &err);
	fprintf(stderr,"w = %f\n", w);
	ASSERT(fabs(w - 459.222) < 0.0005);
	fprintf(stderr,"OK 1\n");

	rho = 11.2 * D->M; T = 126.2;
	S = fprops_set_Trho(T,rho, P, &err);
	p = fprops_p(S, &err);
	fprintf(stderr,"p = %f\n", p);
	ASSERT(fabs(p - 3.39712e6) < 50e3);
	h = fprops_h(S, &err);
	fprintf(stderr,"h = %f (should be = %f)\n", h,816.780*1e3/D->M);
	//ASSERT(fabs(h - 816.780*1e3/D->M) < 5e-4*1e3/D->M);
	w = fprops_w(S, &err);
	fprintf(stderr,"w = %f\n", w);
	ASSERT(fabs(w - 135.571) < 0.0005);
	fprintf(stderr,"OK 2\n");

	fprintf(stderr,"Testing viscosity values from Int J Thermophys 25(1) Jan 2004... ");
	const ViscosityData *V = visc_prepare(&eos_nitrogen, P, &err);
	ASSERT(FPROPS_NO_ERROR==err);
	ASSERT(V != NULL);
	P->visc = V;

	// test data gives densities in mol/dm³
#define VISC_TEST(T__1,RHO__1,MU__1,TOL__1) \
	S = fprops_set_Trho(T__1, RHO__1*P->visc->data.v1.M, P, &err); \
	mu = fprops_mu(S,&err); \
fprintf(stderr,"mu(T=%f, rho=%f) = %e (target: %e)\n",S.T,S.rho,mu,MU__1); \
	ASSERT(FPROPS_NO_ERROR==err); \
	ASSERT(fabs(mu - MU__1)<TOL__1);

	VISC_TEST(100,0,6.90349e-6, 0.000005e-6);
	VISC_TEST(300,0,17.8771e-6, 0.00005e-6);
	VISC_TEST(100,25,79.7418e-6, 0.00005e-6);
	VISC_TEST(200,10,21.0810e-6, 0.00005e-6);
	VISC_TEST(300,5,20.7430e-6, 0.00005e-6);
	VISC_TEST(126.195,11.18,18.2978e-6, 0.00005e-6);

	fprintf(stderr,"done\n");

	fprintf(stderr,"CONSISTENCY TESTS (of test data): u, T, s, a... ");
	for(i=0; i<n; ++i){
		u = td[i].u*1e3;
		T = td[i].T+273.15;
		s = td[i].s*1e3;
		a = td[i].a*1e3;
		//fprintf(stderr,"u - T s = %f, a = %f\n", u-T*s, a);
		double resid = (u - T*s) - a;
		assert(fabs(resid) < fabs(a)*1e-6);
	}
	fprintf(stderr,"done\n");

	fprintf(stderr,"CONSISTENCY TESTS (of test data): p, rho, h, u... ");
	for(i=0; i<n; ++i){
		h = td[i].h*1e3;
		p = td[i].p*1e6;
		rho = td[i].rho;
		u = td[i].u*1e3;
		//fprintf(stderr,"h - p/rho - T s = %f, a = %f\n", h - p/rho-T*s, a);
		double resid = (u + p/rho) - h;
		assert(fabs(resid) < fabs(h)*1e-6);
	}
	fprintf(stderr,"done\n");

#if 0
	//return helm_check_dpdrho_T(D, ntd, td);
	//return helm_check_dhdT_rho(D, ntd, td);
	//return helm_check_dhdrho_T(d, ntd, td);

	helm_check_dhdT_rho(D, ntd, td);
	helm_check_dhdrho_T(D, ntd, td);
	helm_check_dudT_rho(D, ntd, td);
	helm_check_dudrho_T(D, ntd, td);
#endif

	fprintf(stderr,"Running through %d test points...\n",n);
	return helm_run_test_cases(P, ntd, td, 'C');
}

const TestData td[] = {
/*{T/C,   , p/MPa  , rho/(kg/m3)   , u/(kJ/kg)     , h/(kJ/kg)     , s/(kJ/kgK)    , cv/(kJ/kgK)   , cp(kJ/kgK)    , cp0/(kJ/kgK)    a/(kJ/kg)}*/
/*
	samples point from Span et al, as calculated by REFPROP 7.0
*/
#if 0
	{-3.15E+0, 2.706209546E+1, 3.0814828E+2, 1.448502895E+2, 2.326719559E+2}
	, {-1.4695E+2, 3.397116495E+0, 3.13750976E+2, 1.832925675E+1, 2.9156686E+1}
	,
#endif
/*
	A small set of data points calculated using REFPROP 7.0, for validation
*/
	{-2.00E+2, 1.E-1, 8.249414592E+2, -1.306857633E+2, -1.305645426E+2, 2.720604877E+0, 1.109565257E+0, 2.023829833E+0, 1.038887632E+0, -3.296980101E+2}
	, {-1.959064992E+2, 1.E-1, 8.065903588E+2, -1.223708156E+2, -1.22246837E+2, 2.83124055E+0, 1.084716563E+0, 2.040959335E+0, 1.038892209E+0, -3.410657474E+2}
	, {-1.959064992E+2, 1.E-1, 4.556481103E+0, 5.512608739E+1, 7.7072848E+1, 5.41164774E+0, 7.710362489E-1, 1.123116531E+0, 1.038892209E+0, -3.628885293E+2}
	, {-1.50E+2, 1.E-1, 2.765107908E+0, 9.033141131E+1, 1.264963652E+2, 5.914940099E+0, 7.462163482E-1, 1.055667199E+0, 1.03894666E+0, -6.380934619E+2}
	, {-1.00E+2, 1.E-1, 1.952885853E+0, 1.277566181E+2, 1.789628879E+2, 6.272601137E+0, 7.434283808E-1, 1.045406635E+0, 1.03901286E+0, -9.583442688E+2}
	, {-5.0E+1, 1.E-1, 1.511923126E+0, 1.650053791E+2, 2.31146308E+2, 6.537380868E+0, 7.428543667E-1, 1.042430606E+0, 1.039104289E+0, -1.293811162E+3}
	, {0.E+0, 1.E-1, 1.234027837E+0, 2.022010932E+2, 2.832365424E+2, 6.74801325E+0, 7.428806326E-1, 1.041392204E+0, 1.039368282E+0, -1.641018726E+3}
	, {5.0E+1, 1.E-1, 1.042637745E+0, 2.393950056E+2, 3.353055943E+2, 6.923064311E+0, 7.436352617E-1, 1.041585124E+0, 1.040231109E+0, -1.997793226E+3}
	, {1.00E+2, 1.E-1, 9.027313243E-1, 2.7664507E+2, 3.874200009E+2, 7.073009026E+0, 7.456586574E-1, 1.043277497E+0, 1.042312159E+0, -2.362648248E+3}
	, {1.50E+2, 1.E-1, 7.959713424E-1, 3.140324207E+2, 4.396650845E+2, 7.204396773E+0, 7.49458015E-1, 1.046866197E+0, 1.046145853E+0, -2.734508074E+3}
	, {2.00E+2, 1.E-1, 7.118143393E-1, 3.516554257E+2, 4.921414969E+2, 7.321608221E+0, 7.552713429E-1, 1.052537659E+0, 1.051981392E+0, -3.112563504E+3}
	, {2.50E+2, 1.E-1, 6.437636895E-1, 3.896156385E+2, 5.44952141E+2, 7.427704455E+0, 7.630329612E-1, 1.060199497E+0, 1.059758319E+0, -3.496187947E+3}
	, {3.00E+2, 1.E-1, 5.875965268E-1, 4.280048515E+2, 5.98189656E+2, 7.524887552E+0, 7.724566308E-1, 1.069550528E+0, 1.06919308E+0, -3.884884449E+3}
	, {3.50E+2, 1.E-1, 5.404484175E-1, 4.66897069E+2, 6.519286035E+2, 7.614775644E+0, 7.831461911E-1, 1.080185712E+0, 1.079891005E+0, -4.278250373E+3}
	, {4.00E+2, 1.E-1, 5.003075032E-1, 5.063454996E+2, 7.062225739E+2, 7.698579114E+0, 7.946852037E-1, 1.091683066E+0, 1.091436533E+0, -4.675953031E+3}
	, {-2.00E+2, 1.E+0, 8.269722658E+2, -1.311062353E+2, -1.298970049E+2, 2.714834497E+0, 1.111903759E+0, 2.015980797E+0, 1.038887632E+0, -3.296963787E+2}
	, {-1.694030878E+2, 1.E+0, 6.658282477E+2, -6.582866611E+1, -6.432677729E+1, 3.460052635E+0, 9.738696893E-1, 2.43143904E+0, 1.038922946E+0, -4.247984431E+2}
	, {-1.694030878E+2, 1.E+0, 4.133110398E+1, 6.353915362E+1, 8.773400698E+1, 4.92574237E+0, 8.785897919E-1, 1.651759413E+0, 1.038922946E+0, -4.474914077E+2}
	, {-1.50E+2, 1.E+0, 3.089966568E+1, 8.261922852E+1, 1.149820382E+2, 5.167381911E+0, 7.914292017E-1, 1.268580393E+0, 1.03894666E+0, -5.537438538E+2}
	, {-1.00E+2, 1.E+0, 2.018992668E+1, 1.236354905E+2, 1.731651404E+2, 5.565357304E+0, 7.547063826E-1, 1.108351859E+0, 1.03901286E+0, -8.400061267E+2}
	, {-5.0E+1, 1.E+0, 1.530392626E+1, 1.622053684E+2, 2.275480773E+2, 5.841465649E+0, 7.47812625E-1, 1.07327346E+0, 1.039104289E+0, -1.141317691E+3}
	, {0.E+0, 1.E+0, 1.238752228E+1, 2.001017336E+2, 2.808281272E+2, 6.056951203E+0, 7.45695244E-1, 1.059759072E+0, 1.039368282E+0, -1.454354488E+3}
	, {5.0E+1, 1.E+0, 1.042615666E+1, 2.377322774E+2, 3.336448972E+2, 6.23453053E+0, 7.454880323E-1, 1.053757882E+0, 1.040231109E+0, -1.776956263E+3}
	, {1.00E+2, 1.E+0, 9.009743348E+0, 2.752812148E+2, 3.862721678E+2, 6.385956969E+0, 7.470005161E-1, 1.051916948E+0, 1.042312159E+0, -2.107638628E+3}
	, {1.50E+2, 1.E+0, 7.936266563E+0, 3.128862409E+2, 4.38890073E+2, 6.518285235E+0, 7.504954318E-1, 1.0532985E+0, 1.046145853E+0, -2.445326156E+3}
	, {2.00E+2, 1.E+0, 7.093503539E+0, 3.506749593E+2, 4.916490203E+2, 6.636129311E+0, 7.561114932E-1, 1.057499314E+0, 1.051981392E+0, -2.789209624E+3}
	, {2.50E+2, 1.E+0, 6.413750916E+0, 3.88765591E+2, 5.44680595E+2, 6.742670284E+0, 7.637368818E-1, 1.064132645E+0, 1.059758319E+0, -3.138662368E+3}
	, {3.00E+2, 1.E+0, 5.853600946E+0, 4.272601376E+2, 5.980951517E+2, 6.840177103E+0, 7.730616677E-1, 1.072736728E+0, 1.06919308E+0, -3.493187369E+3}
	, {3.50E+2, 1.E+0, 5.383882193E+0, 4.662392116E+2, 6.519787885E+2, 6.930307557E+0, 7.836765419E-1, 1.082812769E+0, 1.079891005E+0, -3.852381942E+3}
	, {4.00E+2, 1.E+0, 4.984238815E+0, 5.057604909E+2, 7.063929319E+2, 7.014296741E+0, 7.951572985E-1, 1.093881037E+0, 1.091436533E+0, -4.215913361E+3}
	, {-2.00E+2, 1.E+1, 8.452673172E+2, -1.348156535E+2, -1.229850766E+2, 2.662217387E+0, 1.134215722E+0, 1.955220874E+0, 1.038887632E+0, -3.295568554E+2}
	, {-1.50E+2, 1.E+1, 6.146317008E+2, -3.70249487E+1, -2.075504269E+1, 3.721059545E+0, 9.337581355E-1, 2.270056086E+0, 1.03894666E+0, -4.952734317E+2}
	, {-1.00E+2, 1.0E+1, 2.740868419E+2, 7.185239674E+1, 1.083371836E+2, 4.599930836E+0, 8.546941465E-1, 2.189924967E+0, 1.03901286E+0, -7.246256275E+2}
	, {-5.0E+1, 1.0E+1, 1.656973184E+2, 1.330827215E+2, 1.934337283E+2, 5.035617646E+0, 7.905131572E-1, 1.424466596E+0, 1.039104289E+0, -9.906153563E+2}
	, {0.E+0, 1.0E+1, 1.252477529E+2, 1.794301367E+2, 2.592718885E+2, 5.302448665E+0, 7.701053502E-1, 1.241224784E+0, 1.039368282E+0, -1.268933716E+3}
	, {5.0E+1, 1.E+1, 1.024973378E+2, 2.216986464E+2, 3.19262156E+2, 5.504302349E+0, 7.619790883E-1, 1.167616763E+0, 1.040231109E+0, -1.557016658E+3}
	, {1.00E+2, 1.E+1, 8.742785901E+1, 2.622473109E+2, 3.766273279E+2, 5.66942089E+0, 7.59252946E-1, 1.130895007E+0, 1.042312159E+0, -1.853297094E+3}
	, {1.50E+2, 1.0E+1, 7.653447744E+1, 3.019757762E+2, 4.326358439E+2, 5.8103035E+0, 7.601658341E-1, 1.111555336E+0, 1.046145853E+0, -2.15665415E+3}
	, {2.00E+2, 1.0E+1, 6.821629197E+1, 3.413566944E+2, 4.879492416E+2, 5.933867436E+0, 7.640672667E-1, 1.102307946E+0, 1.051981392E+0, -2.466252683E+3}
	, {2.50E+2, 1.0E+1, 6.16196599E+1, 3.806901303E+2, 5.429759984E+2, 6.044424746E+0, 7.704812929E-1, 1.099665148E+0, 1.059758319E+0, -2.781450676E+3}
	, {3.00E+2, 1.0E+1, 5.624066686E+1, 4.201840709E+2, 5.979913509E+2, 6.144858594E+0, 7.789093121E-1, 1.101578426E+0, 1.06919308E+0, -3.101741632E+3}
	, {3.50E+2, 1.0E+1, 5.175958526E+1, 4.59985516E+2, 6.531864458E+2, 6.237185757E+0, 7.88835609E-1, 1.106659944E+0, 1.079891005E+0, -3.426716789E+3}
	, {4.00E+2, 1.0E+1, 4.796230162E+1, 5.001960475E+2, 7.086931308E+2, 6.32286331E+0, 7.997719319E-1, 1.113896879E+0, 1.091436533E+0, -3.756039389E+3}
	, {-1.50E+2, 1.00E+2, 8.420363237E+2, -7.952232043E+1, 3.92374019E+1, 3.242503691E+0, 1.061794503E+0, 1.651045202E+0, 1.03894666E+0, -4.7883665E+2}
	, {-1.00E+2, 1.00E+2, 7.480840694E+2, -1.477293041E+1, 1.189018852E+2, 3.7864991E+0, 9.612166888E-1, 1.540186543E+0, 1.03901286E+0, -6.704052495E+2}
	, {-5.0E+1, 1.00E+2, 6.686218021E+2, 4.412242706E+1, 1.936838079E+2, 4.166379675E+0, 9.045605306E-1, 1.454706847E+0, 1.039104289E+0, -8.856051973E+2}
	, {0.E+0, 1.00E+2, 6.020019153E+2, 9.855920741E+1, 2.646716357E+2, 4.453651157E+0, 8.694630248E-1, 1.387312334E+0, 1.039368282E+0, -1.117955606E+3}
	, {5.0E+1, 1.00E+2, 5.463770513E+2, 1.49612758E+2, 3.326365504E+2, 4.682270323E+0, 8.466982693E-1, 1.333296991E+0, 1.040231109E+0, -1.363462897E+3}
	, {1.00E+2, 1.00E+2, 4.998628485E+2, 1.981348717E+2, 3.981897474E+2, 4.870958715E+0, 8.321215371E-1, 1.290606288E+0, 1.042312159E+0, -1.619463373E+3}
	, {1.50E+2, 1.00E+2, 4.607196175E+2, 2.448127085E+2, 4.618644601E+2, 5.031138876E+0, 8.236965038E-1, 1.257948834E+0, 1.046145853E+0, -1.884113707E+3}
	, {2.00E+2, 1.00E+2, 4.274764375E+2, 2.901989649E+2, 5.241299871E+2, 5.170247187E+0, 8.201904384E-1, 1.234009444E+0, 1.051981392E+0, -2.156103492E+3}
	, {2.50E+2, 1.00E+2, 3.989550654E+2, 3.347325069E+2, 5.853873016E+2, 5.293334174E+0, 8.206409326E-1, 1.217384623E+0, 1.059758319E+0, -2.434475266E+3}
	, {3.00E+2, 1.00E+2, 3.742353905E+2, 3.787557173E+2, 6.459672171E+2, 5.40393536E+0, 8.241897795E-1, 1.206689759E+0, 1.06919308E+0, -2.718509834E+3}
	, {3.50E+2, 1.00E+2, 3.526046985E+2, 4.225300983E+2, 7.061338065E+2, 5.504585302E+0, 8.300621222E-1, 1.200652388E+0, 1.079891005E+0, -3.007652233E+3}
	, {4.00E+2, 1.00E+2, 3.335102049E+2, 4.662506686E+2, 7.660915686E+2, 5.597138445E+0, 8.375827178E-1, 1.19816815E+0, 1.091436533E+0, -3.301463075E+3}
};

const unsigned ntd = sizeof(td)/sizeof(TestData);

#endif

