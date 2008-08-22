#include "ammonia.h"

/* Property data for Nitrogen

From Span, Lemmon, Jacobsen & Wagner
A Reference Quality Equation of State for Nitrogen
Int J Thermophysics, Vol 18, No 4, 1998.

This is the nitrogren property correlation recommended
by NIST in its program REFPROP 7.0. */

const HelmholtzData helmholtz_data_ammonia = {
	/* R */ 8.31451 / 28.01348e-3 /* J/molK * kg/mol = J/kg/K */
	, /* M */ 28.01348 /* kg/kmol */
	, /* rho_star */ 11.1839 * 28.01348 /* mol/L * kg/kmol = kg/kL = kg/m³ = rho_c for this model*/
	, /* T_star */ 126.192 /* K = T_c for this model */
	, 5 /* ni */
	, (const double[]){ /* a0 values 1..5 */
		 /* need to work out how to implement these still */
	}, 21 /* nr */
	, (const HelmholtzATDL[]){
		/* a_i, t_i, d_i, l_i */
		{ 0.924803575275,      0.25,    1,  0}
		,{-0.492448489428,     0.875,   1,  0}
		,{ 0.661883336938,     0.5,     2,  0}
		,{-0.192902649201E1,   0.875,   2,  0}
		,{-0.622469309629E-1,  0.375,   3,  0}
		,{ 0.349943957581,     0.75,    3,  0}
		,{ 0.564857472498,     0.5,     1,  1}
		,{-0.161720005987E1,   0.75,    1,  1}
		,{-0.481395031883,     2.,      1,  1}
		,{ 0.421150636384,     1.25,    3,  1}
		,{-0.161962230825E-1,  3.5,     3,  1}
		,{ 0.172100994165,     1.,      4,  1}
		,{ 0.735448924933E-2,  0.5,     6,  1}
		,{ 0.168077305479E-1,  3.,      6,  1}
		,{-0.107626664179E-2,  0.,      7,  1}
		,{-0.137318088513E-1,  2.75,    7,  1}
		,{ 0.635466899859E-3,  0.75,    8,  1}
		,{ 0.304432279419E-2,  2.5,     8,  1}
		,{-0.435762336045E-1,  4.,      1,  2}
		,{-0.723174889316E-1,  6. ,     2,  2}
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
		/* more terms here still to come */
	}
};


/*
	Test suite. These tests attempt to validate the current code using 
	a few sample figures output by REFPROP 7.0.

	To run the test, compile and run as follows:

	gcc helmholtz.c ammonia.c -DTEST -o ammonia -lm && ./ammonia 

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
#define ASSERT_TOL(EXPR,VAL,TOL) {\
		double cval; cval = (EXPR);\
		double err; err = cval - (double)(VAL);\
		double relerrpc = (cval-(VAL))/(VAL)*100;\
		if(fabs(relerrpc)>maxerr)maxerr=fabs(relerrpc);\
		if(fabs(err)>TOL){\
			fprintf(stderr,"ERROR in line %d: value of '%s' = %f, should be %f, error is %f (%.2f%%)!\n"\
				, __LINE__, #EXPR, cval, VAL,cval-(VAL),relerrpc);\
			exit(1);\
		}else{\
			fprintf(stderr,"    OK, %s = %8.2e with %.2f%% err.\n",#EXPR,VAL,relerrpc);\
			/*fprintf(stderr,"        (err = %8.2e, tol = %8.2e, calc = %8.2e)\n",fabs(err),TOL,cval);*/\
		}\
	}

int main(void){
	double rho, T, p, h, u;
	const HelmholtzData *d;

	d = &helmholtz_data_ammonia;
	double maxerr = 0;

	fprintf(stderr,"ENTROPY TESTS\n");

	/* offset required to attain agreement with REFPROP */
	double Y = -471.596704;

	fprintf(stderr,"s(T,rho) at p = 0.1 MPa\n");
	ASSERT_TOL(helmholtz_s(273.15+-60, 713.65,d), Y+0.36737e3, 0.5);
	ASSERT_TOL(helmholtz_s(273.15+  0,0.76124,d), Y+6.8900e3, 0.5);
	ASSERT_TOL(helmholtz_s(273.15+ 50,0.63869,d), Y+7.2544e3, 0.5);
	ASSERT_TOL(helmholtz_s(273.15+200,0.43370,d), Y+8.1232e3, 0.5);
	ASSERT_TOL(helmholtz_s(273.15+300,0.35769,d), Y+8.6084e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+420,0.29562,d), Y+9.1365e3, 1);

	fprintf(stderr,"s(T,rho) at p = 1 MPa\n");
	ASSERT_TOL(helmholtz_s(273.15+-50,702.49,d), Y+0.56381e3, 0.5);
	ASSERT_TOL(helmholtz_s(273.15+150,4.9817,d), Y+6.7008e3, 0.5);
	ASSERT_TOL(helmholtz_s(273.15+200,4.4115,d), Y+6.9770e3, 0.5);
	ASSERT_TOL(helmholtz_s(273.15+350,3.3082,d), Y+7.7012e3, 0.5);
	ASSERT_TOL(helmholtz_s(273.15+420,2.9670,d), Y+8.0059e3, 0.5);

	fprintf(stderr,"s(T,rho) at p = 10 MPa\n");
	ASSERT_TOL(helmholtz_s(273.15+-70,728.11,d), Y+0.14196e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+-50,706.21,d), Y+0.54289e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+-20,670.55,d), Y+1.0975e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+  0,645.04,d), Y+1.4403e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+125.17,356.70,d), Y+3.5463e3, 1);

	ASSERT_TOL(helmholtz_s(273.15+125.17,121.58,d), Y+4.5150e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+200,54.389,d), Y+5.5906e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+350,35.072,d), Y+6.4850e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+420,30.731,d), Y+6.8171e3, 1);

	fprintf(stderr,"s(T,rho) at p = 20 MPa\n");
	ASSERT_TOL(helmholtz_s(273.15+-50,710.19,d), Y+0.52061e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+ 30,612.22,d), Y+1.8844e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+150,359.41,d), Y+3.7164e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+200,152.83,d), Y+4.8376e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+350,74.590,d), Y+6.0407e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+420,63.602,d), Y+6.4066e3, 1);

	fprintf(stderr,"s(T,rho) at p = 100 MPa\n");
	ASSERT_TOL(helmholtz_s(273.15+  0,690.41,d), Y+1.2158e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+100,591.07,d), Y+2.5499e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+250,437.69,d), Y+4.0264e3, 1);
	ASSERT_TOL(helmholtz_s(273.15+420,298.79,d), Y+5.2620e3, 1);

	/* successful entropy tests means that helm_ideal_tau, helm_real_tau, helm_ideal and helm_resid are all OK */

	fprintf(stderr,"PRESSURE TESTS\n");

	fprintf(stderr,"p(T,rho) = 0.1 MPa\n");	
	ASSERT_TOL(helmholtz_p(273.15 -70,724.74783,d), 0.1E6,  1E3);
	ASSERT_TOL(helmholtz_p(273.15 -60,713.64815,d), 0.1E6,  1E3);
	ASSERT_TOL(helmholtz_p(273.15 -50,702.11130,d), 0.1E6,  1E3);
	ASSERT_TOL(helmholtz_p(273.15 -40,690.16351,d), 0.1E6,   1E3);
	ASSERT_TOL(helmholtz_p(273.15 -33.588341,682.29489,d), 0.1E6,1E3);
	ASSERT_TOL(helmholtz_p(273.15+  0,0.76123983,d), 0.1E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+100,0.55135,d), 0.1E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+250,0.39203,d), 0.1E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+420,0.29562,d), 0.1E6, 1E3);

	fprintf(stderr,"p(T,rho) = 1 MPa\n");
	ASSERT_TOL(helmholtz_p(273.15 -70,725.05815,d), 1E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+  0,638.97275,d), 1E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+ 30,7.5736465,d), 1E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+150,4.9816537,d), 1E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+200,4.4115,d), 1E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+350,3.3082,d), 1E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+420,2.9670,d), 1E6, 1E3);

	fprintf(stderr,"p(T,rho) = 10 MPa\n");
	ASSERT_TOL(helmholtz_p(273.15+-40.,694.67407,d), 10E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+-20.,670.54741,d), 10E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+50,573.07306,d), 10E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+110,441.76869,d), 10E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+150,74.732,d), 10E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+200,54.389,d), 10E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+350,35.072,d), 10E6, 1E3);
	ASSERT_TOL(helmholtz_p(273.15+420,30.731,d), 10E6, 1E3);

	fprintf(stderr,"p(T,rho) = 20 MPa\n");
	ASSERT_TOL(helmholtz_p(273.15+150,359.40683,d), 20E6, 1E4);
	ASSERT_TOL(helmholtz_p(273.15+200,152.83430,d), 20E6, 1E4);
	ASSERT_TOL(helmholtz_p(273.15+350,74.590236,d), 20E6, 1E4);
	ASSERT_TOL(helmholtz_p(273.15+420,63.601873,d), 20E6, 1E4);

	//fprintf(stderr,"IDEAL HELMHOLTZ COMPONENT\n");
	//ASSERT_TOL(helm_ideal(273.15, 0) 

	fprintf(stderr,"ENTHALPY TESTS\n");

	/* this offset is required to attain agreement with values from REFPROP */
	double Z = -1635.7e3 + 1492.411e3;

	fprintf(stderr,"h(T,rho) at p = 0.1 MPa\n");
	ASSERT_TOL(helmholtz_h(273.15+-60, 713.65,d), Z+75.166e3, 0.2e3);
	ASSERT_TOL(helmholtz_h(273.15+  0,0.76124,d), Z+1635.7e3, 0.2e3);
	ASSERT_TOL(helmholtz_h(273.15+ 50,0.63869,d), Z+1744.0e3, 0.2e3);
	ASSERT_TOL(helmholtz_h(273.15+200,0.43370,d), Z+2087.0e3, 0.2e3);
	ASSERT_TOL(helmholtz_h(273.15+300,0.35769,d), Z+2340.0e3, 1e3);
	ASSERT_TOL(helmholtz_h(273.15+420,0.29562,d), Z+2674.3e3, 1e3);

	fprintf(stderr,"h(T,rho) at p = 1 MPa\n");
	ASSERT_TOL(helmholtz_h(273.15+150,4.9817,d), Z+1949.1e3, 1e3);
	ASSERT_TOL(helmholtz_h(273.15+200,4.4115,d), Z+2072.7e3, 1e3);
	ASSERT_TOL(helmholtz_h(273.15+350,3.3082,d), Z+2468.2e3, 1e3);
	ASSERT_TOL(helmholtz_h(273.15+420,2.9670,d), Z+2668.6e3, 1e3);

	fprintf(stderr,"h(T,rho) at p = 10 MPa\n");
	ASSERT_TOL(helmholtz_h(273.15+-50,706.21,d), Z+127.39e3, 2e3);
	ASSERT_TOL(helmholtz_h(273.15+-0,645.04,d), Z+349.53e3, 2e3);
	
	ASSERT_TOL(helmholtz_h(273.15+150,74.732,d), Z+1688.5e3, 1e3);
	ASSERT_TOL(helmholtz_h(273.15+200,54.389,d), Z+1908.0e3, 1e3);
	ASSERT_TOL(helmholtz_h(273.15+350,35.072,d), Z+2393.4e3, 1e3);
	ASSERT_TOL(helmholtz_h(273.15+420,30.731,d), Z+2611.8e3, 1e3);

	fprintf(stderr,"h(T,rho) at p = 20 MPa\n");
	/* note rather larger errors in the following few lines -- why? */
	ASSERT_TOL(helmholtz_h(273.15 -70,731.41,d), Z+51.734e3, 0.5e3);
	ASSERT_TOL(helmholtz_h(273.15 -60,721.00318,d), Z+93.871419e3, 0.5e3);
	ASSERT_TOL(helmholtz_h(273.15 -50,710.19289,d), Z+136.54351e3, 1e3);
	ASSERT_TOL(helmholtz_h(273.15 -40,699.02472,d), Z+179.72030e3, 0.5e3);
	ASSERT_TOL(helmholtz_h(273.15+ 30,612.22,d), Z+493.28e3, 0.5e3);
	ASSERT_TOL(helmholtz_h(273.15+150,359.40683,d), Z+1162.5e3, 0.5e3);
	ASSERT_TOL(helmholtz_h(273.15+200,152.83430,d), Z+1662.9e3, 0.5e3);
	ASSERT_TOL(helmholtz_h(273.15+250,106.31299,d), Z+1928.6499e3, 0.5e3);
	ASSERT_TOL(helmholtz_h(273.15+300,86.516941,d), Z+2128.9031e3, 0.5e3);
	ASSERT_TOL(helmholtz_h(273.15+330,78.784703,d), Z+2238.2416e3, 0.5e3);
	ASSERT_TOL(helmholtz_h(273.15+350,74.590236,d), Z+2308.8516e3, 10e3);
	ASSERT_TOL(helmholtz_h(273.15+420,63.601873,d), Z+2549.2872e3, 10e3);

	fprintf(stderr,"h(T,rho) at p = 100 MPa\n");
	ASSERT_TOL(helmholtz_h(273.15+  0,690.41,d), Z+422.69e3, 0.5e3);
	ASSERT_TOL(helmholtz_h(273.15+100,591.07,d), Z+850.44e3, 0.1e3);
	ASSERT_TOL(helmholtz_h(273.15+250,437.69,d), Z+1506.6e3, 1e3);
	ASSERT_TOL(helmholtz_h(273.15+420,298.79,d), Z+2252.3e3, 1e3);

	fprintf(stderr,"Tests completed OK (maximum error = %0.2f%%)\n",maxerr);
	exit(0);
}
#endif
