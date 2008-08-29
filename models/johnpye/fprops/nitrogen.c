#include "ammonia.h"

/* Property data for Nitrogen

From Span, Lemmon, Jacobsen & Wagner
A Reference Quality Equation of State for Nitrogen
Int J Thermophysics, Vol 18, No 4, 1998.

This is the nitrogren property correlation recommended
by NIST in its program REFPROP 7.0. */

const IdealData ideal_data_nitrogen = {
	-12.76953 /* constant */
	,-0.007841630 /* linear */
	, 3 /* power terms */
	, (const IdealPowTerm[]){
		{1.934819,     -1.}
		,{-1.247742e-5, -2.}
		,{6.678326e-8,  -3.}
	}
	, 1
	, (const IdealExpTerm[]){
		{1.012941, 26.65788}
	}
};

/*
	this correlation is given in molar terms. we convert to mass
	basis by changing the scaling density rho_star, and the ideal gas constant.
*/
#define M_NITROGEN 28.01348
const HelmholtzData helmholtz_data_nitrogen = {
	/* R */ 1e3 * 8.31451 / M_NITROGEN /* 1000 * kJ/kmolK / kg/kmol = J/kgK */
	, /* M */ M_NITROGEN /* kg/kmol */
	, /* rho_star */ 11.1839 * M_NITROGEN /* kmol/m3 * kg/kmol = kg/m³ (= rho_c for this model) */
	, /* T_star */ 126.192 /* K (= T_c for this model) */
	, &ideal_data_nitrogen
	, 32 /* np */
	, (const HelmholtzPowTerm[]){
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
		,{-0.441513370350E-2,  7.,      5,  4}
		,{ 0.133722924858E-2,  4.,      6,  4}
		,{ 0.264832491957E-3, 16.,      9,  4}
	}
	, 4 /* ne */
	, (const HelmholtzExpTerm[]){
		{0.196688194015e2, 0., 1, 20, 325, 1.16}
		, {-0.209115600730e2, 1., 1, 20, 325, 1.16}
		, {0.167788306989e-1, 2., 3, 15, 300, 1.13}
		, {0.262767566274e4, 3., 2, 25, 275, 1.25}
	}
};


/*
	Test suite. These tests attempt to validate the current code using 
	a few sample figures output by REFPROP 7.0.

	To run the test, compile and run as follows:

	gcc helmholtz.c nitrogen.c -DTEST -o nitrogen -lm && ./nitrogen

	These tests all currently pass with a maximum error of 2%. The error
	seems to arise in the enthalpy data for low temperatures. Haven't been
	able to determine where the problem comes from.
*/
#ifdef TEST

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* a simple macro to actually do the testing */
#define ASSERT_TOL(FN,PARAM1,PARAM2,PARAM3,VAL,TOL) {\
		double cval; cval = FN(PARAM1,PARAM2,PARAM3);\
		double err; err = cval - (double)(VAL);\
		double relerrpc = (cval-(VAL))/(VAL)*100;\
		if(fabs(relerrpc)>maxerr)maxerr=fabs(relerrpc);\
		if(fabs(err)>TOL){\
			fprintf(stderr,"ERROR in line %d: value of '%s(%f,%f,%s)' = %f,"\
				" should be %f, error is %f (%.2f%%)!\n"\
				, __LINE__, #FN,PARAM1,PARAM2,#PARAM3, cval, VAL,cval-(VAL)\
				,relerrpc\
			);\
			exit(1);\
		}else{\
			fprintf(stderr,"    OK, %s(%f,%f,%s) = %8.2e with %.2f%% err.\n"\
				,#FN,PARAM1,PARAM2,#PARAM3,VAL,relerrpc\
			);\
		}\
	}

typedef struct{double T,p,rho,h,s;} TestData;
const TestData td[]; const unsigned ntd;

int main(void){

	double rho, T, p, h, u;
	const HelmholtzData *d;

	d = &helmholtz_data_nitrogen;
	double maxerr = 0;

	unsigned i;
	const unsigned n = ntd;

#if 0
	fprintf(stderr,"Testing sample values from the Span paper...\n");

	rho = 10.993 * d->M; T = 270.;
	p = helmholtz_p(T, rho, d);
	fprintf(stderr,"p = %f\n", p);
	assert(fabs(p - 27.0621e6) < 5e3);
	fprintf(stderr,"OK 1\n");

	rho = 11.2 * d->M; T = 126.2;
	p = helmholtz_p(T, rho, d);
	fprintf(stderr,"p = %f\n", p);
	assert(fabs(p - 3.39712e6) < 50e3);
	fprintf(stderr,"OK 2\n");
#endif

	fprintf(stderr,"Running through %d test points...\n",n);

	fprintf(stderr,"PRESSURE TESTS\n");
	for(i=0; i<n;++i){
		T = td[i].T+273.15;
		p = td[i].p*1e6;
		rho = td[i].rho;
	 	ASSERT_TOL(helmholtz_p, T, rho, d, p, p*1e-6);
	}

	/* offset required to attain agreement with REFPROP */
	double Y = -471.596704;

	fprintf(stderr,"ENTROPY TESTS\n");
	for(i=0; i<n;++i){
	 	ASSERT_TOL(helmholtz_s, td[i].T+273.15, td[i].rho, d, td[i].s*1e3 + Y, 1E3);
	}

	/* this offset is required to attain agreement with values from REFPROP */
	double Z = -1635.7e3 + 1492.411e3;

	fprintf(stderr,"ENTHALPY TESTS\n");
	for(i=0; i<n;++i){
	 	ASSERT_TOL(helmholtz_h, td[i].T+273.15, td[i].rho, d, td[i].h*1e3 + Z, 1E3);
	}

	fprintf(stderr,"Tests completed OK (maximum error = %0.2f%%)\n",maxerr);
	exit(0);
}

const TestData td[] = {
/*
	samples point from Span et al, as calculated by REFPROP 7.0
*/
#if 1
	{-3.15E+0, 2.706209546E+1, 3.0814828E+2, 1.448502895E+2, 2.326719559E+2}
	, {-1.4695E+2, 3.397116495E+0, 3.13750976E+2, 1.832925675E+1, 2.9156686E+1}
/*
	A small set of data points calculated using REFPROP 7.0, for validation
*/
	,
#endif
    {-2.00E+2, 1.E-1, 8.249414592E+2, -1.305645426E+2, 2.720604877E+0}
	, {-1.959064992E+2, 1.E-1, 8.065903588E+2, -1.22246837E+2, 2.83124055E+0}
	, {-1.959064992E+2, 1.E-1, 4.556481103E+0, 7.7072848E+1, 5.41164774E+0}
	, {-1.50E+2, 1.E-1, 2.765107908E+0, 1.264963652E+2, 5.914940099E+0}
	, {-1.00E+2, 1.E-1, 1.952885853E+0, 1.789628879E+2, 6.272601137E+0}
	, {-5.0E+1, 1.E-1, 1.511923126E+0, 2.31146308E+2, 6.537380868E+0}
	, {0.E+0, 1.E-1, 1.234027837E+0, 2.832365424E+2, 6.74801325E+0}
	, {5.0E+1, 1.E-1, 1.042637745E+0, 3.353055943E+2, 6.923064311E+0}
	, {1.00E+2, 1.E-1, 9.027313243E-1, 3.874200009E+2, 7.073009026E+0}
	, {1.50E+2, 1.E-1, 7.959713424E-1, 4.396650845E+2, 7.204396773E+0}
	, {2.00E+2, 1.E-1, 7.118143393E-1, 4.921414969E+2, 7.321608221E+0}
	, {2.50E+2, 1.E-1, 6.437636895E-1, 5.44952141E+2, 7.427704455E+0}
	, {3.00E+2, 1.E-1, 5.875965268E-1, 5.98189656E+2, 7.524887552E+0}
	, {3.50E+2, 1.E-1, 5.404484175E-1, 6.519286035E+2, 7.614775644E+0}
	, {4.00E+2, 1.E-1, 5.003075032E-1, 7.062225739E+2, 7.698579114E+0}
	, {-2.00E+2, 1.E+0, 8.269722658E+2, -1.298970049E+2, 2.714834497E+0}
	, {-1.694030878E+2, 1.E+0, 6.658282477E+2, -6.432677729E+1, 3.460052635E+0}
	, {-1.694030878E+2, 1.E+0, 4.133110398E+1, 8.773400698E+1, 4.92574237E+0}
	, {-1.50E+2, 1.E+0, 3.089966568E+1, 1.149820382E+2, 5.167381911E+0}
	, {-1.00E+2, 1.E+0, 2.018992668E+1, 1.731651404E+2, 5.565357304E+0}
	, {-5.0E+1, 1.E+0, 1.530392626E+1, 2.275480773E+2, 5.841465649E+0}
	, {0.E+0, 1.E+0, 1.238752228E+1, 2.808281272E+2, 6.056951203E+0}
	, {5.0E+1, 1.E+0, 1.042615666E+1, 3.336448972E+2, 6.23453053E+0}
	, {1.00E+2, 1.E+0, 9.009743348E+0, 3.862721678E+2, 6.385956969E+0}
	, {1.50E+2, 1.E+0, 7.936266563E+0, 4.38890073E+2, 6.518285235E+0}
	, {2.00E+2, 1.E+0, 7.093503539E+0, 4.916490203E+2, 6.636129311E+0}
	, {2.50E+2, 1.E+0, 6.413750916E+0, 5.44680595E+2, 6.742670284E+0}
	, {3.00E+2, 1.E+0, 5.853600946E+0, 5.980951517E+2, 6.840177103E+0}
	, {3.50E+2, 1.E+0, 5.383882193E+0, 6.519787885E+2, 6.930307557E+0}
	, {4.00E+2, 1.E+0, 4.984238815E+0, 7.063929319E+2, 7.014296741E+0}
	, {-2.00E+2, 1.E+1, 8.452673172E+2, -1.229850766E+2, 2.662217387E+0}
	, {-1.50E+2, 1.E+1, 6.146317008E+2, -2.075504269E+1, 3.721059545E+0}
	, {-1.00E+2, 1.0E+1, 2.740868419E+2, 1.083371836E+2, 4.599930836E+0}
	, {-5.0E+1, 1.0E+1, 1.656973184E+2, 1.934337283E+2, 5.035617646E+0}
	, {0.E+0, 1.0E+1, 1.252477529E+2, 2.592718885E+2, 5.302448665E+0}
	, {5.0E+1, 1.E+1, 1.024973378E+2, 3.19262156E+2, 5.504302349E+0}
	, {1.00E+2, 1.E+1, 8.742785901E+1, 3.766273279E+2, 5.66942089E+0}
	, {1.50E+2, 1.0E+1, 7.653447744E+1, 4.326358439E+2, 5.8103035E+0}
	, {2.00E+2, 1.0E+1, 6.821629197E+1, 4.879492416E+2, 5.933867436E+0}
	, {2.50E+2, 1.0E+1, 6.16196599E+1, 5.429759984E+2, 6.044424746E+0}
	, {3.00E+2, 1.0E+1, 5.624066686E+1, 5.979913509E+2, 6.144858594E+0}
	, {3.50E+2, 1.0E+1, 5.175958526E+1, 6.531864458E+2, 6.237185757E+0}
	, {4.00E+2, 1.0E+1, 4.796230162E+1, 7.086931308E+2, 6.32286331E+0}
	, {-1.50E+2, 1.00E+2, 8.420363237E+2, 3.92374019E+1, 3.242503691E+0}
	, {-1.00E+2, 1.00E+2, 7.480840694E+2, 1.189018852E+2, 3.7864991E+0}
	, {-5.0E+1, 1.00E+2, 6.686218021E+2, 1.936838079E+2, 4.166379675E+0}
	, {0.E+0, 1.00E+2, 6.020019153E+2, 2.646716357E+2, 4.453651157E+0}
	, {5.0E+1, 1.00E+2, 5.463770513E+2, 3.326365504E+2, 4.682270323E+0}
	, {1.00E+2, 1.00E+2, 4.998628485E+2, 3.981897474E+2, 4.870958715E+0}
	, {1.50E+2, 1.00E+2, 4.607196175E+2, 4.618644601E+2, 5.031138876E+0}
	, {2.00E+2, 1.00E+2, 4.274764375E+2, 5.241299871E+2, 5.170247187E+0}
	, {2.50E+2, 1.00E+2, 3.989550654E+2, 5.853873016E+2, 5.293334174E+0}
	, {3.00E+2, 1.00E+2, 3.742353905E+2, 6.459672171E+2, 5.40393536E+0}
	, {3.50E+2, 1.00E+2, 3.526046985E+2, 7.061338065E+2, 5.504585302E+0}
	, {4.00E+2, 1.00E+2, 3.335102049E+2, 7.660915686E+2, 5.597138445E+0}
};

const unsigned ntd = sizeof(td)/sizeof(TestData);

#endif

