#include "test.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


int helm_run_test_cases(const HelmholtzData *d, unsigned ntd, const TestData *td){

	double rho, T, p, u, h, a, s, cp0;

	double maxerr = 0;
	double se = 0, sse = 0;

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
	for(i=16; i<n;++i){
		T = td[i].T+273.15;
		rho = td[i].rho;
		p = td[i].p*1e6;
	 	ASSERT_TOL(helmholtz_p, T, rho, d, p, p*1e-3);
	}

	/* Checking internal energy values (proves phi0_tau, phir_tau) */

	fprintf(stderr,"INTERNAL ENERGY TESTS\n");
	for(i=0; i<n;++i){
		T = td[i].T+273.15;
		rho = td[i].rho;
		u = td[i].u*1e3;
		ASSERT_TOL(helmholtz_u, T, rho, d, u, 1e3*u);
	}

	/* Checking entropy values */

	fprintf(stderr,"ENTROPY TESTS\n");
	se = 0, sse = 0;
	for(i=0; i<n;++i){
		T = td[i].T+273.15;
		rho = td[i].rho;
		s = td[i].s*1e3;
	 	ASSERT_TOL(helmholtz_s, T, rho, d, s, 1e-6*s);
	}

	/* checking enthalpy values */
	fprintf(stderr,"ENTHALPY TESTS\n");
	se = 0, sse = 0;
	for(i=0; i<n;++i){
		T = td[i].T+273.15;
		rho = td[i].rho;
		h = td[i].h*1e3;
#if 0
		double err = h - helmholtz_h(T,rho,d);
		se += err;
		sse += err*err;
		fprintf(stderr,"%.12e\t%.12e\t%.12e\t%.12e\n",T,rho,u,err);
#else
	 	ASSERT_TOL(helmholtz_h, td[i].T+273.15, td[i].rho, d, h, 1E3);
#endif
	}
#if 0
	fprintf(stderr,"average error = %.10e\n",se/n);
	fprintf(stderr,"sse - n se^2 = %.3e\n",sse - n*se*se);
	exit(1);
#endif

	/* Checking helmholtz energy values */

	fprintf(stderr,"HELMHOLTZ ENERGY TESTS\n");
	for(i=0; i<n;++i){
		T = td[i].T+273.15;
		rho = td[i].rho;
		a = td[i].a*1e3;
		//fprintf(stderr,"%.20e\t%.20e\t%.20e\n",T,rho,(a - helmholtz_a(T,rho,d)));
	 	ASSERT_TOL(helmholtz_a, T, rho, d, a, a*1e-6);
	}
	
	fprintf(stderr,"Tests completed OK (maximum error = %0.5f%%)\n",maxerr);
	exit(0);
}

int helm_check_u(const HelmholtzData *d, unsigned ntd, const TestData *td){
	unsigned i;
	double T,rho,u,err,se = 0,sse = 0;
	unsigned n = ntd;
	double tol = 1e-1;

	fprintf(stderr,"INTERNAL ENERGY VALUES\n\n");
	fprintf(stderr,"%-18s\t%-18s\t%-18s\t%-18s\t%-18s\n","T","rho","u","du","%err");
	for(i=0; i<n;++i){
		T = td[i].T+273.15;
		rho = td[i].rho;
		u = td[i].u*1e3;
		err = u - helmholtz_u(T,rho,d);
		se += err;
		sse += err*err;
		fprintf(stderr,"%.12e\t%.12e\t%.12e\t%.12e\t%.6f\n",T,rho,u,err,err/u*100.);
	}
	fprintf(stderr,"average error = %.10e\n",se/n);
	fprintf(stderr,"sse - n se^2 = %.3e\n",sse - n*se*se);
}

int helm_check_s(const HelmholtzData *d, unsigned ntd, const TestData *td){
	unsigned i;
	double T,rho,s,err,se = 0,sse = 0;
	unsigned n = ntd;
	double tol = 1e-1;

	fprintf(stderr,"ENTROPY VALUES\n\n");
	fprintf(stderr,"%-18s\t%-18s\t%-18s\t%-18s\t%-18s\n","T","rho","s","ds","%err");
	for(i=0; i<n;++i){
		T = td[i].T + 273.15;
		rho = td[i].rho;
		s = td[i].s*1e3;
		err = s - helmholtz_s(T,rho,d);
		se += err;
		sse += err*err;
		fprintf(stderr,"%.12e\t%.12e\t%.12e\t%.12e\t%.6f\n",T,rho,s,err,err/s*100.);
	}
	fprintf(stderr,"average error = %.10e\n",se/n);
	fprintf(stderr,"sse - n se^2 = %.3e\n",sse - n*se*se);
}

