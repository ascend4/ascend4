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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*/

#include "ammonia.h"

#define AMMONIA_OFFSET_H -1.4311891570e+05
#define AMMONIA_R 488.189
#define AMMONIA_TSTAR 405.40
/**
Ideal gas data for Ammonia, from Tillner-Roth, Harms-Watzenberg and
Baehr, 'Eine neue Fundamentalgleichung für Ammoniak', DKV-Tagungsbericht,
20:167-181, 1993. This is the ammmonia property correlation recommended
by NIST in its program REFPROP 7.0.
*/

#ifdef PRECALC
const IdealPhi0Data precalc_data = {
	AMMONIA_TSTAR
	, 3 /* power terms */
	, (const IdealPhi0PowTerm[]){
		{11.47340, 1./3.}
		,{-1.296211, -3./2}
		,{0.5706757, -7./4}
	}
	, 0
};
#else
const IdealData ideal_data_ammonia = {
	-15.815020 - 4.715240698841839e+02/AMMONIA_R /* constant */
	, 4.255726 + (1.432837032793666e+05)/AMMONIA_R/AMMONIA_TSTAR /* linear */
	, AMMONIA_TSTAR /* Tstar */
	, AMMONIA_R /* cpstar J/kgK */

	/* cp0 POWER TERMS AUTOMATICALLY PRE-CALCULATED FROM phi0 */
	, 3
	, (const IdealPowTerm[]){
		{1.887010003371195e+01, -3.333333333333333e-01}
		, {5.954994351355028e-04, 1.500000000000000e+00}
		, {-7.498313086309935e-05, 1.750000000000000e+00}
	}
	, 0, (const IdealExpTerm *)0 /* no exponential terms */
	/* END OF PRE-CALCULATED VALUES */

};

/**
Residual (non-ideal) property data for Ammonia, from Tillner-Roth, 
Harms-Watzenberg and Baehr, 'Eine neue Fundamentalgleichung für Ammoniak',
DKV-Tagungsbericht, 20:167-181, 1993. This is the ammmonia property correlation
recommended by NIST in its program REFPROP 7.0.
*/
const HelmholtzData helmholtz_data_ammonia = {
	/* R */ AMMONIA_R /* J/kg/K */
	, /* M */ 17.03026 /* kg/kmol */
	, /* rho_star */225. /* kg/m³ */
	, /* T_star */ AMMONIA_TSTAR /* K */

	, /* T_c */ AMMONIA_TSTAR
	, /* p_c */ 0
	, /* rho_c */ 225.
	, /* p_t */ 0

	, 0.250 /* acentric factor, from Reid, Prausnitz & Polling */
	, &ideal_data_ammonia
	, 21 /* np */
	, (const HelmholtzPowTerm[]){
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
	, 0, 0 /* no gaussian terms */
	, 0, 0 /* no critical terms */
};
#endif

/*
	Test suite. These tests attempt to validate the current code using 
	a few sample figures output by REFPROP 7.0.

	To run the test, compile and run as follows:

	gcc ideal.c helmholtz.c ammonia.c -DTEST -o ammonia -lm && ./ammonia 

	These tests all currently pass with a maximum error of 0.09%.
*/
#ifdef TEST

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

typedef struct{double T,p,rho,u,h,s,cv,cp,cp0,a;} TestData;
const TestData td[]; const unsigned ntd;

int main(void){
	unsigned n, i;
	double rho, T, cp0, p, u, h, a, s;
	const HelmholtzData *d;

	d = &helmholtz_data_ammonia;
	double maxerr = 0;

	n = ntd;
	fprintf(stderr,"Running through %d test points...\n",n);

/* a simple macro to actually do the testing */
#define ASSERT_TOL(FN,PARAM1,PARAM2,PARAM3,VAL,TOL) {\
		double cval; cval = FN(PARAM1,PARAM2,PARAM3);\
		double err; err = cval - (double)(VAL);\
		double relerrpc = (cval-(VAL))/(VAL)*100;\
		if(fabs(relerrpc)>maxerr)maxerr=fabs(relerrpc);\
		if(fabs(err)>fabs(TOL)){\
			fprintf(stderr,"ERROR in line %d: value of '%s(%f,%f,%s)' = %0.8f,"\
				" should be %f, error is %.10e (%.2f%%)!\n"\
				, __LINE__, #FN,PARAM1,PARAM2,#PARAM3, cval, VAL,cval-(VAL)\
				,relerrpc\
			);\
			exit(1);\
		}else{\
			fprintf(stderr,"    OK, %s(%f,%f,%s) = %8.2e with %.6f%% err.\n"\
				,#FN,PARAM1,PARAM2,#PARAM3,VAL,relerrpc\
			);\
		}\
	}

#define CP0(T,RHO,DATA) helmholtz_cp0(T,DATA)

	fprintf(stderr,"CP0 TESTS\n");
	for(i=0; i<n;++i){
		cp0 = td[i].cp0*1e3;
	 	ASSERT_TOL(CP0, td[i].T+273.15, 0., d, cp0, cp0*1e-1);
	}
#undef CP0

	fprintf(stderr,"PRESSURE TESTS\n");
	for(i=0; i<n;++i){
		p = td[i].p*1e6;
	 	ASSERT_TOL(helmholtz_p, td[i].T+273.15, td[i].rho, d, p, p*1e-3);
	}

	double se=0, ss=0;
	fprintf(stderr,"INTERNAL ENERGY TESTS\n");
	for(i=0; i<n;++i){
		T = td[i].T+273.15;
		rho = td[i].rho;
		u = td[i].u*1e3;
		//double err = u - helmholtz_u(T,rho,d);
		//se += err; ss += err*err;
	 	ASSERT_TOL(helmholtz_u, T, rho, d, u, u*1e-2);
		//fprintf(stderr,"%.20e\t%.20e\t%.20e\n",T,rho,err);
	}
	//fprintf(stderr,"average u error = %.15e\n",se/n);
	//fprintf(stderr,"sse = %.3e\n",ss - n*se*se);
	//exit(1);

	fprintf(stderr,"ENTHALPY TESTS\n");
	for(i=0; i<n;++i){
		T = td[i].T+273.15;
		rho = td[i].rho;
		h = td[i].h*1e3;
		//fprintf(stderr,"%.20e\n",(h - helmholtz_h(T,rho,d)) );
	 	ASSERT_TOL(helmholtz_h, td[i].T+273.15, td[i].rho, d, h, 1E3);
	}
	//exit(1);

	/* entropy offset required to attain agreement with REFPROP */
	fprintf(stderr,"ENTROPY TESTS\n");
	for(i=0; i<n;++i){
		T = td[i].T+273.15;
		rho = td[i].rho;
		s = td[i].s*1e3;
		//double err = s - helmholtz_s(T,rho,d);
		//se += err; ss += err*err;
	 	ASSERT_TOL(helmholtz_s, td[i].T+273.15, td[i].rho, d, s, 1e-1*s);
		//fprintf(stderr,"%.20e\t%.20e\t%.20e\n",T,rho,err);
	}
	//fprintf(stderr,"average s error = %.15e\n",se/n);
	//fprintf(stderr,"sse = %.3e\n",ss - n*se*se);
	//exit(1);

	fprintf(stderr,"HELMHOLTZ ENERGY TESTS\n");
	for(i=0; i<n;++i){
		T = td[i].T+273.15;
		rho = td[i].rho;
		a = td[i].a*1e3;
		//fprintf(stderr,"%.10e\t%.10e\t%.20e\n",T,rho,(a - helmholtz_a(T,rho,d)) );
	 	ASSERT_TOL(helmholtz_a, T, rho, d, a, a*1);
	}

	helm_check_dhdT_rho(d, ntd, td);
	helm_check_dhdrho_T(d, ntd, td);
	helm_check_dudT_rho(d, ntd, td);
	helm_check_dudrho_T(d, ntd, td);

	fprintf(stderr,"Tests completed OK (maximum error = %0.2f%%)\n",maxerr);
	exit(0);
}

/*
	Some test data generated by REFPROP 7.0 for p=0.1, 1, 10, 20, 100 MPa.
*/
const TestData td[] = {
/*{T/C,   , p/MPa  , rho/(kg/m3)   , u/(kJ/kg)     , h/(kJ/kg)     , s/(kJ/kgK)    , cv/(kJ/kgK)   , cp(kJ/kgK)    , cp0/(kJ/kgK)    a/(kJ/kg)}*/
{-7.0E+1, 1.E-1, 7.247478262E+2, 3.229029237E+1, 3.24282714E+1, 1.620191045E-1, 2.920877051E+0, 4.244712246E+0, 2.008935269E+0, -6.238887074E-1}
, {-3.358834071E+1, 1.E-1, 6.822948922E+2, 1.906067908E+2, 1.90753355E+2, 8.784304761E-1, 2.855271402E+0, 4.446549354E+0, 2.027472493E+0, -1.983147159E+1}
, {-3.358834071E+1, 1.E-1, 8.786678914E-1, 1.447205955E+3, 1.561014597E+3, 6.5982992E+0, 1.703163454E+0, 2.294671598E+0, 2.027472493E+0, -1.3349355E+2}
, {-2.0E+1, 1.E-1, 8.263382177E-1, 1.470672235E+3, 1.591688059E+3, 6.722856799E+0, 1.664164155E+0, 2.226407096E+0, 2.039317099E+0, -2.312189638E+2}
, {3.0E+1, 1.E-1, 6.82304266E-1, 1.554101898E+3, 1.700664077E+3, 7.115863679E+0, 1.647471243E+0, 2.164362521E+0, 2.099810352E+0, -6.030721764E+2}
, {8.0E+1, 1.E-1, 5.831571413E-1, 1.638302054E+3, 1.80978242E+3, 7.448946867E+0, 1.706378596E+0, 2.209153449E+0, 2.179739223E+0, -9.922935323E+2}
, {1.30E+2, 1.E-1, 5.097610128E-1, 1.725968834E+3, 1.922139191E+3, 7.746386561E+0, 1.791930531E+0, 2.28877615E+0, 2.272754856E+0, -1.396986908E+3}
, {1.80E+2, 1.E-1, 4.5299046E-1, 1.818169337E+3, 2.038924538E+3, 8.019355389E+0, 1.89071839E+0, 2.384574213E+0, 2.374740669E+0, -1.815801558E+3}
, {2.30E+2, 1.E-1, 4.076912096E-1, 1.915461928E+3, 2.160745608E+3, 8.274268056E+0, 1.997259617E+0, 2.489413305E+0, 2.482853093E+0, -2.247736044E+3}
, {2.80E+2, 1.E-1, 3.706742442E-1, 2.018176941E+3, 2.287955598E+3, 8.515225293E+0, 2.108580854E+0, 2.599678959E+0, 2.595030777E+0, -2.69201993E+3}
, {3.30E+2, 1.E-1, 3.398445366E-1, 2.126514644E+3, 2.420766836E+3, 8.745015419E+0, 2.222765313E+0, 2.713166279E+0, 2.709721662E+0, -3.148041406E+3}
, {3.80E+2, 1.E-1, 3.137636628E-1, 2.240589284E+3, 2.559300504E+3, 8.965613422E+0, 2.338444972E+0, 2.828362941E+0, 2.825720964E+0, -3.615301123E+3}
, {-7.0E+1, 1.E+0, 7.250581532E+2, 3.191292802E+1, 3.329212774E+1, 1.601599414E-1, 2.923511889E+0, 4.241967721E+0, 2.008935269E+0, -6.235640687E-1}
, {-2.0E+1, 1.E+0, 6.655956817E+2, 2.50822066E+2, 2.523244796E+2, 1.122985212E+0, 2.83379452E+0, 4.509597946E+0, 2.039317099E+0, -3.346164026E+1}
, {2.489509207E+1, 1.E+0, 6.029210949E+2, 4.586595787E+2, 4.603181705E+2, 1.878768996E+0, 2.764789412E+0, 4.783527097E+0, 2.092608239E+0, -1.012982998E+2}
, {2.489509207E+1, 1.E+0, 7.782283811E+0, 1.498025198E+3, 1.626522182E+3, 5.791613203E+0, 2.129812148E+0, 3.133024821E+0, 2.092608239E+0, -2.281366922E+2}
, {3.0E+1, 1.E+0, 7.573646544E+0, 1.510151287E+3, 1.64218808E+3, 5.843733285E+0, 2.071473976E+0, 3.009079611E+0, 2.099810352E+0, -2.613764583E+2}
, {8.0E+1, 1.E+0, 6.146734558E+0, 1.614152301E+3, 1.776840309E+3, 6.255779773E+0, 1.85362895E+0, 2.516587992E+0, 2.179739223E+0, -5.950763259E+2}
, {1.30E+2, 1.E+0, 5.259177246E+0, 1.709991264E+3, 1.900135075E+3, 6.582405217E+0, 1.860827189E+0, 2.443994372E+0, 2.272754856E+0, -9.437053987E+2}
, {1.80E+2, 1.E+0, 4.621587996E+0, 1.806536743E+3, 2.022912586E+3, 6.869456377E+0, 1.928949229E+0, 2.476922143E+0, 2.374740669E+0, -1.306357415E+3}
, {2.30E+2, 1.E+0, 4.132311779E+0, 1.906483833E+3, 2.148479127E+3, 7.132238182E+0, 2.020971903E+0, 2.550089921E+0, 2.482853093E+0, -1.682101808E+3}
, {2.80E+2, 1.E+0, 3.741648311E+0, 2.010971322E+3, 2.278233212E+3, 7.378029463E+0, 2.124500592E+0, 2.642300555E+0, 2.595030777E+0, -2.070185676E+3}
, {3.30E+2, 1.E+0, 3.421067224E+0, 2.120568346E+3, 2.412874791E+3, 7.610994396E+0, 2.234107913E+0, 2.744579111E+0, 2.709721662E+0, -2.470002925E+3}
, {3.80E+2, 1.E+0, 3.152561727E+0, 2.235579603E+3, 2.552781957E+3, 7.833784037E+0, 2.346907298E+0, 2.85236643E+0, 2.825720964E+0, -2.881056441E+3}
, {-7.0E+1, 1.E+1, 7.281133559E+2, 2.82467347E+1, 4.198085991E+1, 1.41957312E-1, 2.948363488E+0, 4.215226705E+0, 2.008935269E+0, -5.918932355E-1}
, {-2.0E+1, 1.E+1, 6.705474103E+2, 2.444309266E+2, 2.593441153E+2, 1.097500056E+0, 2.847709968E+0, 4.466308713E+0, 2.039317099E+0, -3.34012126E+1}
, {3.0E+1, 1.0E+1, 6.036532714E+2, 4.718115329E+2, 4.883773339E+2, 1.922486043E+0, 2.76482419E+0, 4.716399214E+0, 2.099810352E+0, -1.109901112E+2}
, {8.0E+1, 1.E+1, 5.191766069E+2, 7.182881257E+2, 7.375493941E+2, 2.682019021E+0, 2.725818451E+0, 5.40010006E+0, 2.179739223E+0, -2.288668917E+2}
, {1.251668991E+2, 1.0E+1, 3.566957919E+2, 1.036686156E+3, 1.064721249E+3, 3.546261425E+0, 3.120852518E+0, 1.799912569E+1, 2.263318196E+0, -3.758496986E+2}
, {1.251668991E+2, 1.0E+1, 1.215794756E+2, 1.368344633E+3, 1.450595358E+3, 4.515023005E+0, 3.549403632E+0, 2.75480558E+1, 2.263318196E+0, -4.300653297E+2}
, {1.30E+2, 1.0E+1, 1.006607688E+2, 1.433765286E+3, 1.533108854E+3, 4.721093813E+0, 3.179462337E+0, 1.218949302E+1, 2.272754856E+0, -4.69543685E+2}
, {1.80E+2, 1.0E+1, 6.013676382E+1, 1.664224216E+3, 1.830511847E+3, 5.423234397E+0, 2.370777153E+0, 4.100436931E+0, 2.374740669E+0, -7.93314451E+2}
, {2.30E+2, 1.E+1, 4.825185325E+1, 1.806069112E+3, 2.013315037E+3, 5.806541293E+0, 2.265129959E+0, 3.370696776E+0, 2.482853093E+0, -1.11549214E+3}
, {2.80E+2, 1.0E+1, 4.136336686E+1, 1.933702328E+3, 2.175462144E+3, 6.11393854E+0, 2.282610344E+0, 3.155805732E+0, 2.595030777E+0, -1.448222776E+3}
, {3.30E+2, 1.0E+1, 3.661170108E+1, 2.058308433E+3, 2.331445154E+3, 6.383938033E+0, 2.345404767E+0, 3.099738472E+0, 2.709721662E+0, -1.792163792E+3}
, {3.80E+2, 1.0E+1, 3.303984101E+1, 2.183905783E+3, 2.486570677E+3, 6.631017385E+0, 2.429601299E+0, 3.113246758E+0, 2.825720964E+0, -2.147143222E+3}
, {-7.0E+1, 2.E+1, 7.314110284E+2, 2.438989319E+1, 5.173429903E+1, 1.225158918E-1, 2.97295095E+0, 4.186833561E+0, 2.008935269E+0, -4.992102225E-1}
, {-2.0E+1, 2.E+1, 6.757461174E+2, 2.378074747E+2, 2.674043891E+2, 1.070658825E+0, 2.862424736E+0, 4.424294578E+0, 2.039317099E+0, -3.322980699E+1}
, {3.0E+1, 2.0E+1, 6.122156933E+2, 4.606156644E+2, 4.932838894E+2, 1.884416733E+0, 2.77478908E+0, 4.620255644E+0, 2.099810352E+0, -1.106452683E+2}
, {8.0E+1, 2.E+1, 5.368145605E+2, 6.956794002E+2, 7.329362138E+2, 2.615362829E+0, 2.713627362E+0, 5.036284223E+0, 2.179739223E+0, -2.279359829E+2}
, {1.30E+2, 2.0E+1, 4.294999502E+2, 9.663897317E+2, 1.012955511E+3, 3.354873661E+0, 2.741137614E+0, 6.544104233E+0, 2.272754856E+0, -3.861275849E+2}
, {1.80E+2, 2.0E+1, 2.039788488E+2, 1.395383189E+3, 1.493432571E+3, 4.471185097E+0, 2.869863104E+0, 1.043542616E+1, 2.374740669E+0, -6.307343372E+2}
, {2.30E+2, 2.0E+1, 1.191595281E+2, 1.667907891E+3, 1.835750113E+3, 5.192497178E+0, 2.520294197E+0, 4.955556508E+0, 2.482853093E+0, -9.446970641E+2}
, {2.80E+2, 2.0E+1, 9.305602458E+1, 1.837529361E+3, 2.052453651E+3, 5.603863917E+0, 2.442250471E+0, 3.919373117E+0, 2.595030777E+0, -1.262247965E+3}
, {3.30E+2, 2.E+1, 7.87847032E+1, 1.984385217E+3, 2.238241603E+3, 5.925629929E+0, 2.457876404E+0, 3.567673051E+0, 2.709721662E+0, -1.589658474E+3}
, {3.80E+2, 2.0E+1, 6.930562563E+1, 2.124157386E+3, 2.412734248E+3, 6.20363454E+0, 2.513826097E+0, 3.43455809E+0, 2.825720964E+0, -1.927746514E+3}
, {-2.0E+1, 1.E+2, 7.097079501E+2, 1.971623979E+2, 3.380654271E+2, 8.941118487E-1, 2.948865706E+0, 4.209874651E+0, 2.039317099E+0, -2.918201662E+1}
, {3.0E+1, 1.00E+2, 6.609898909E+2, 3.99293994E+2, 5.505822381E+2, 1.660051479E+0, 2.865882923E+0, 4.272619565E+0, 2.099810352E+0, -1.039506117E+2}
, {8.0E+1, 1.00E+2, 6.112193023E+2, 6.009802061E+2, 7.645876046E+2, 2.313447447E+0, 2.790639905E+0, 4.287734917E+0, 2.179739223E+0, -2.160137598E+2}
, {1.30E+2, 1.E+2, 5.605676047E+2, 8.013221537E+2, 9.797127692E+2, 2.88311668E+0, 2.735688771E+0, 4.322269403E+0, 2.272754856E+0, -3.610063358E+2}
, {1.80E+2, 1.00E+2, 5.091016945E+2, 1.000810051E+3, 1.197234461E+3, 3.391678155E+0, 2.70641506E+0, 4.380908617E+0, 2.374740669E+0, -5.361289048E+2}
, {2.30E+2, 1.00E+2, 4.577648376E+2, 1.19928899E+3, 1.417741767E+3, 3.853216148E+0, 2.701336718E+0, 4.435601393E+0, 2.482853093E+0, -7.394567146E+2}
, {2.80E+2, 1.00E+2, 4.085537409E+2, 1.39536859E+3, 1.640134431E+3, 4.274597549E+0, 2.715900681E+0, 4.45131851E+0, 2.595030777E+0, -9.691250446E+2}
, {3.30E+2, 1.00E+2, 3.638085408E+2, 1.587052868E+3, 1.86192272E+3, 4.658479239E+0, 2.745812238E+0, 4.411199889E+0, 2.709721662E+0, -1.222708885E+3}
, {3.80E+2, 1.00E+2, 3.251311703E+2, 1.772997776E+3, 2.080565949E+3, 5.006781049E+0, 2.788309263E+0, 4.330393148E+0, 2.825720964E+0, -1.497181267E+3}
};

const unsigned ntd = sizeof(td)/sizeof(TestData);

#endif
