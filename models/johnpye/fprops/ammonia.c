#include "ammonia.h"


/**
Ideal gas data for Ammonia, from Tillner-Roth, Harms-Watzenberg and
Baehr, 'Eine neue Fundamentalgleichung für Ammoniak', DKV-Tagungsbericht,
20:167-181, 1993. This is the ammmonia property correlation recommended
by NIST in its program REFPROP 7.0.
*/
const IdealData ideal_data_ammonia = {
	-15.815020 /* const */
	, 4.255726 /* linear */
	, 3 /* power terms */
	, (const IdealPowTerm[]){
		{11.474340,   1./3 }
		,{-1.296211,  -3./2.}
		,{0.5706757,  -7./4.}
	}
	, 0, (const IdealExpTerm *)0 /* no exponential terms */
};
			 

/**
Residual (non-ideal) property data for Ammonia, from Tillner-Roth, 
Harms-Watzenberg and Baehr, 'Eine neue Fundamentalgleichung für Ammoniak',
DKV-Tagungsbericht, 20:167-181, 1993. This is the ammmonia property correlation
recommended by NIST in its program REFPROP 7.0.
*/
const HelmholtzData helmholtz_data_ammonia = {
	/* R */ 488.189 /* J/kg/K */
	, /* M */ 17.03026 /* kg/kmol */
	, /* rho_star */225. /* kg/m³ */
	, /* T_star */ 405.40 /* K */
	, &ideal_data_ammonia
	, 21 /* nr */
	, (const HelmholtzATDL[]){
		/* a_i, t_i, d_i, l_i */
		{0.4554431E-1,  -0.5  ,  2,  0}/* 1 */
		,{0.7238548E+0,   0.5 ,   1, 0 }
		,{0.1229470E-1,     1 ,   4, 0 }
		,{-0.1858814E+1,  1.5 ,   1, 0 }
		,{0.2141882E-10,    3 ,  15, 0 }/* 5 */
		,{-0.1430020E-1,    0 ,   3, 1 }
		,{0.3441324E+0,     3 ,   3, 1 } 
		,{-0.2873571E+0,    4 ,   1, 1 }
		,{0.2352589E-4,     4 ,   8, 1 }
		,{-0.3497111E-1,   5  ,  2,  1}/* 10 */
		,{0.2397852E-1,    3  ,  1,  2}
		,{0.1831117E-2,    5 ,   8,  2}
		,{-0.4085375E-1,   6 ,   1,  2}
		,{0.2379275E+0,    8 ,   2,  2}
		,{-0.3548972E-1,   8 ,   3,  2}/* 15 */
		,{-0.1823729E+0,   10,   2,  2}
		,{0.2281556E-1,   10 ,   4,  2}
		,{-0.6663444E-2,   5 ,   3,  3}
		,{-0.8847486E-2,  7.5,   1,  3}
		,{0.2272635E-2 ,  15 ,   2,  3}/* 20 */
		,{-0.5588655E-3,  30,    4,  3}
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

	fprintf(stderr,"Tests completed OK (maximum error = %0.2f%%)\n",maxerr);
	exit(0);
}
#endif
