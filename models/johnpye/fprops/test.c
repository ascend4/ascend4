#include "test.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

int helm_run_test_cases(const HelmholtzData *d, unsigned ntd, const TestData *td){

	double rho, T, p, u, h, a, s, cp0;

	double maxerr = 0;

	unsigned i;
	const unsigned n = ntd;

	fprintf(stderr,"Running through %d test points...\n",n);

	/* Checking CP0 values */

#define CP0_TEMP(T,RHO,DATA) helmholtz_cp0(T,DATA)
	fprintf(stderr,"CP0 TESTS\n");
	for(i=0; i<n;++i){
		cp0 = td[i].cp0*1e3;
	 	ASSERT_TOL(CP0_TEMP, td[i].T+273.15, td[i].rho, d, cp0, cp0*1e-6);
	}
#undef CP0_TEMP

	/* Checking pressure values (proves phir_delta) */

	fprintf(stderr,"PRESSURE TESTS\n");
	for(i=0; i<n;++i){
		T = td[i].T+273.15;
		rho = td[i].rho;
		p = td[i].p*1e6;
	 	ASSERT_TOL(helmholtz_p, T, rho, d, p, p*1e-6);
	}

	/* Checking internal energy values (proves phi0_tau, phir_tau) */

	fprintf(stderr,"INTERNAL ENERGY TESTS\n");
	for(i=0; i<n;++i){
		T = td[i].T+273.15;
		rho = td[i].rho;
		u = td[i].u*1e3;
		//fprintf(stderr,"%.20e\t%.20e\t%.20e\n",T,rho,(u - helmholtz_u(T,rho,d)));
	 	ASSERT_TOL(helmholtz_u, td[i].T+273.15, td[i].rho, d, u, u*1e-6);
	}

	/* Checking entropy values */

	fprintf(stderr,"ENTROPY TESTS\n");
	double se = 0, sse = 0;
	for(i=0; i<n;++i){
		T = td[i].T+273.15;
		rho = td[i].rho;
		s = td[i].s*1e3;
		double err = s - helmholtz_s(T,rho,d);
		se += err;
		sse += err*err;
		//fprintf(stderr,"%.20e\t%.20e\t%.20e\n",T,rho,(s - helmholtz_s(T,rho,d)));
	 	ASSERT_TOL(helmholtz_s, T, rho, d, s, 1e-6*s);
	}
	//fprintf(stderr,"average error = %.10e\n",se/n);
	//fprintf(stderr,"sse - n se^2 = %.3e\n",sse - n*se*se);
	//exit(1);

	/* Checking helmholtz energy values */

	fprintf(stderr,"HELMHOLTZ ENERGY TESTS\n");
	for(i=0; i<n;++i){
		T = td[i].T+273.15;
		rho = td[i].rho;
		a = td[i].a*1e3;
		//fprintf(stderr,"%.20e\t%.20e\t%.20e\n",T,rho,(a - helmholtz_a(T,rho,d)));
	 	ASSERT_TOL(helmholtz_a, T, rho, d, a, a*1e-6);
	}
	//exit(1);

	fprintf(stderr,"ENTHALPY TESTS\n");
	for(i=0; i<n;++i){
		T = td[i].T+273.15;
		rho = td[i].rho;
		h = td[i].h*1e3;
		//fprintf(stderr,"%.20e\n",(h - helmholtz_h(T,rho,d)) );
	 	ASSERT_TOL(helmholtz_h, td[i].T+273.15, td[i].rho, d, h, 1E3);
	}

	fprintf(stderr,"Tests completed OK (maximum error = %0.5f%%)\n",maxerr);
	exit(0);
}

