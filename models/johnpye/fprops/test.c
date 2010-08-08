#include "test.h"
#include "sat.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


int helm_run_test_cases(const HelmholtzData *d, unsigned ntd, const TestData *td, int temp_unit){

	double rho, T, p, u, h, a, s, cp0, T_adj = 0;

	if(temp_unit=='C')T_adj = 273.15;


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
	 	ASSERT_TOL(CP0_TEMP, td[i].T+T_adj, td[i].rho, d, cp0, cp0*1e-6);
	}
#undef CP0_TEMP

	/* Checking pressure values (proves phir_delta) */
	fprintf(stderr,"PRESSURE TESTS\n");
	for(i=16; i<n;++i){
		T = td[i].T+T_adj;
		rho = td[i].rho;
		p = td[i].p*1e6;
	 	ASSERT_TOL(helmholtz_p_raw, T, rho, d, p, p*1e-3);
	}

	/* checking enthalpy values */
	fprintf(stderr,"ENTHALPY TESTS\n");
	se = 0, sse = 0;
	for(i=0; i<n;++i){
		T = td[i].T+T_adj;
		rho = td[i].rho;
		h = td[i].h*1e3;
	 	ASSERT_TOL(helmholtz_h, td[i].T+T_adj, td[i].rho, d, h, 1e-3*h);
	}

	/* Checking internal energy values (proves phi0_tau, phir_tau) */
	fprintf(stderr,"INTERNAL ENERGY TESTS\n");
	for(i=0; i<n;++i){
		T = td[i].T+T_adj;
		rho = td[i].rho;
		u = td[i].u*1e3;
		ASSERT_TOL(helmholtz_u, T, rho, d, u, 1e3*u);
	}

	/* Checking entropy values */

	fprintf(stderr,"ENTROPY TESTS\n");
	se = 0, sse = 0;
	for(i=0; i<n;++i){
		T = td[i].T+T_adj;
		rho = td[i].rho;
		s = td[i].s*1e3;
	 	ASSERT_TOL(helmholtz_s, T, rho, d, s, 1e-5*s);
	}


	/* Checking helmholtz energy values */

	fprintf(stderr,"HELMHOLTZ ENERGY TESTS\n");
	for(i=0; i<n;++i){
		T = td[i].T+T_adj;
		rho = td[i].rho;
		a = td[i].a*1e3;
		//fprintf(stderr,"%.20e\t%.20e\t%.20e\n",T,rho,(a - helmholtz_a(T,rho,d)));
	 	ASSERT_TOL(helmholtz_a, T, rho, d, a, a*1e-3	);
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

/* check derivatives of p */

int helm_check_dpdT_rho(const HelmholtzData *d, unsigned ntd, const TestData *td){
	unsigned i;
	double T,rho,p,T1,p1,dpdT,dpdT_est,err,se = 0,sse = 0;
	unsigned n = ntd;
	double tol = 1e-1;

	double dT = 0.0001 /* finite difference in temperature, in K */;

	fprintf(stderr,"(dp/dT)_rho RESULTS\n\n");
	fprintf(stderr,"%-18s\t%-18s\t%-18s\t%-18s\t%-18s\t%12s\t%12s\n","T","rho","p","dp/dT","dp/dT est","err","%err");
	for(i=0; i<n;++i){
		T = td[i].T + 273.15;
		rho = td[i].rho;
		p = helmholtz_p(T,rho,d);
		dpdT = helmholtz_dpdT_rho(T,rho,d);
		assert(!isinf(dpdT));
		T1 = T + dT;
		p1 = helmholtz_p(T1, rho, d);
		dpdT_est = (p1 - p)/dT;
		err = (dpdT_est - dpdT);
		se += err;
		sse += err*err;
		fprintf(stderr,"%.12e\t%.12e\t%.12e\t%.12e\t%.12e\t%12.4e\t%12.2e\n",T,rho,p,dpdT,dpdT_est,err,err/dpdT*100	);
	}
	fprintf(stderr,"average error = %.10e\n",se/n);
	fprintf(stderr,"sse - n se^2 = %.3e\n",sse - n*se*se);
}

int helm_check_dpdrho_T(const HelmholtzData *d, unsigned ntd, const TestData *td){
	unsigned i;
	double T,rho,p,rho1,p1,dpdrho,dpdrho_est,err,se = 0,sse = 0;
	unsigned n = ntd;
	double tol = 1e-1;

	double drho = 0.0001 /* finite difference in temperature, in K */;

	fprintf(stderr,"(dp/drho)_T RESULTS\n\n");
	fprintf(stderr,"%-18s\t%-18s\t%-18s\t%-18s\t%-18s\t%12s\t%12s\n","T","rho","p","dp/drho","dp/drho est","err","%err");
	for(i=0; i<n;++i){
		T = td[i].T + 273.15;
		rho = td[i].rho;
		p = helmholtz_p(T,rho,d);
		dpdrho = helmholtz_dpdrho_T(T,rho,d);
		assert(!isinf(dpdrho));
		rho1 = rho + drho;
		p1 = helmholtz_p(T, rho1, d);
		dpdrho_est = (p1 - p)/drho;
		err = (dpdrho_est - dpdrho);
		se += err;
		sse += err*err;
		fprintf(stderr,"%.12e\t%.12e\t%.12e\t%.12e\t%.12e\t%12.4e\t%12.6f\n",T,rho,p,dpdrho,dpdrho_est,err,err/dpdrho*100);
	}
	fprintf(stderr,"average error = %.10e\n",se/n);
	fprintf(stderr,"sse - n se^2 = %.3e\n",sse - n*se*se);
}


int helm_check_d2pdrho2_T(const HelmholtzData *d, unsigned ntd, const TestData *td){
	unsigned i;
	double T,rho,p,dpdrho,rho1,dpdrho1,d2pdrho2,d2pdrho2_est,err,se = 0,sse = 0;
	unsigned n = ntd;
	double tol = 1e-1;

	double drho = 0.0001 /* finite difference in temperature, in K */;

	fprintf(stderr,"\n(d2p/drho2)_T RESULTS\n\n");
	fprintf(stderr,"%-18s\t%-18s\t%-18s\t%-18s\t%-18s\t%12s\t%12s\t%12s\n","T","rho","p","dp/drho","d2p/drho2 est","d2p/drho2 calc","err","rel err");
	for(i=0; i<n;++i){
		T = td[i].T + 273.15;
		rho = td[i].rho;
		p = helmholtz_p(T,rho,d);
		dpdrho = helmholtz_dpdrho_T(T,rho,d);
		d2pdrho2 = helmholtz_d2pdrho2_T(T, rho, d);
		assert(!isinf(d2pdrho2));
		rho1 = rho + drho;
		dpdrho1 = helmholtz_dpdrho_T(T, rho1, d);
		d2pdrho2_est = (dpdrho1 - dpdrho)/drho;
		err = (d2pdrho2_est - d2pdrho2);
		se += err;
		sse += err*err;
		fprintf(stderr,"%.12e\t%.12e\t%.12e\t%.12e\t%.12e\t%12.4e\t%12.2e\t%12.3e\n",T,rho,p,dpdrho,d2pdrho2_est,d2pdrho2,err,err/d2pdrho2_est);
	}
	fprintf(stderr,"average error = %.10e\n",se/n);
	fprintf(stderr,"sse - n se^2 = %.3e\n",sse - n*se*se);
}

/* check derivatives of h */

int helm_check_dhdT_rho(const HelmholtzData *d, unsigned ntd, const TestData *td){
	unsigned i;
	double T,rho,h,T1,h1,dhdT,dhdT_est,err,se = 0,sse = 0;
	unsigned n = ntd;
	double tol = 1e-1;

	double dT = 1e-6 /* finite difference in temperature, in K */;

	fprintf(stderr,"(dh/dT)_rho RESULTS\n\n");
	fprintf(stderr,"%-18s\t%-18s\t%-18s\t%-18s\t%-18s\t%12s\t%12s\n","T","rho","h","dh/dT","dh/dT est","err","%err");
	for(i=0; i<n;++i){
		T = td[i].T + 273.15;
		rho = td[i].rho;
		h = helmholtz_h(T,rho,d);
		dhdT = helmholtz_dhdT_rho(T,rho,d);
		assert(!isinf(dhdT));
		T1 = T + dT;
		h1 = helmholtz_h(T1, rho, d);
		dhdT_est = (h1 - h)/dT;
		err = (dhdT_est - dhdT);
		se += err;
		sse += err*err;
		fprintf(stderr,"%.12e\t%.12e\t%.12e\t%.12e\t%.12e\t%12.4e\t%12.2e\n",T,rho,h,dhdT,dhdT_est,err,err/dhdT*100	);
	}
	fprintf(stderr,"average error = %.10e\n",se/n);
	fprintf(stderr,"sse - n se^2 = %.3e\n",sse - n*se*se);
}


int helm_check_dhdrho_T(const HelmholtzData *d, unsigned ntd, const TestData *td){
	unsigned i;
	double T,rho,h,rho1,h1,dhdrho,dhdrho_est,err,se = 0,sse = 0;
	unsigned n = ntd;

	double drho = 1e-6 /* finite difference in temperature, in K */;

	fprintf(stderr,"(dh/drho)_T RESULTS\n\n");
	fprintf(stderr,"%-18s\t%-18s\t%-18s\t%-18s\t%-18s\t%12s\t%12s\n","T","rho","h","dh/drho","dh/drho est","err","%err");
	for(i=0; i<n;++i){
		T = td[i].T + 273.15;
		rho = td[i].rho;
		h = helmholtz_h(T,rho,d);
		dhdrho = helmholtz_dhdrho_T(T,rho,d);
		assert(!isinf(dhdrho));
		rho1 = rho + drho;
		h1 = helmholtz_h(T, rho1, d);
		dhdrho_est = (h1 - h)/drho;
		err = (dhdrho_est - dhdrho);
		se += err;
		sse += err*err;
		fprintf(stderr,"%.12e\t%.12e\t%.12e\t%.12e\t%.12e\t%12.4e\t%12.2e\n",T,rho,h,dhdrho,dhdrho_est,err,err/dhdrho*100	);
	}
	fprintf(stderr,"average error = %.10e\n",se/n);
	fprintf(stderr,"sse - n se^2 = %.3e\n",sse - n*se*se);
}

/* check derivatives of u */

int helm_check_dudT_rho(const HelmholtzData *d, unsigned ntd, const TestData *td){
	unsigned i;
	double T,rho,u,T1,u1,dudT,dudT_est,err,se = 0,sse = 0;
	unsigned n = ntd;

	double dT = 1e-3 /* finite difference in temperature, in K */;

	fprintf(stderr,"(du/dT)_rho RESULTS\n\n");
	fprintf(stderr,"%-18s\t%-18s\t%-18s\t%-18s\t%-18s\t%12s\t%12s\n","T","rho","h","du/dT","du/dT est","err","%err");
	for(i=0; i<n;++i){
		T = td[i].T + 273.15;
		rho = td[i].rho;
		u = helmholtz_u(T,rho,d);
		dudT = helmholtz_dudT_rho(T,rho,d);
		assert(!isinf(dudT));
		T1 = T + dT;
		u1 = helmholtz_u(T1, rho, d);
		dudT_est = (u1 - u)/dT;
		err = (dudT_est - dudT);
		se += err;
		sse += err*err;
		fprintf(stderr,"%.12e\t%.12e\t%.12e\t%.12e\t%.12e\t%12.4e\t%12.2e\n",T,rho,u,dudT,dudT_est,err,err/dudT*100	);
	}
	fprintf(stderr,"average error = %.10e\n",se/n);
	fprintf(stderr,"sse - n se^2 = %.3e\n",sse - n*se*se);
}


int helm_check_dudrho_T(const HelmholtzData *d, unsigned ntd, const TestData *td){
	unsigned i;
	double T,rho,u,rho1,u1,dudrho,dudrho_est,err,se = 0,sse = 0;
	unsigned n = ntd;
	double tol = 1e-1;

	double drho = 1e-6 /* finite difference in temperature, in K */;

	fprintf(stderr,"(du/drho)_T RESULTS\n\n");
	fprintf(stderr,"%-18s\t%-18s\t%-18s\t%-18s\t%-18s\t%12s\t%12s\n","T","rho","h","du/drho","du/drho est","err","%err");
	for(i=0; i<n;++i){
		T = td[i].T + 273.15;
		rho = td[i].rho;
		u = helmholtz_u(T,rho,d);
		dudrho = helmholtz_dudrho_T(T,rho,d);
		assert(!isinf(dudrho));
		rho1 = rho + drho;
		u1 = helmholtz_u(T, rho1, d);
		dudrho_est = (u1 - u)/drho;
		err = (dudrho_est - dudrho);
		se += err;
		sse += err*err;
		fprintf(stderr,"%.12e\t%.12e\t%.12e\t%.12e\t%.12e\t%12.4e\t%12.2e\n",T,rho,u,dudrho,dudrho_est,err,err/dudrho*100	);
	}
	fprintf(stderr,"average error = %.10e\n",se/n);
	fprintf(stderr,"sse - n se^2 = %.3e\n",sse - n*se*se);
}







