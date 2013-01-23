#include "test.h"
#include "fprops.h"
#include "sat.h"
#include "rundata.h"
#include "ideal.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifdef __WIN32__
# include <windows.h>
#endif

void test_init(){
#ifdef __WIN32__
	SetErrorMode(SEM_NOGPFAULTERRORBOX);
#endif
}

int helm_run_test_cases(const PureFluid *P, unsigned ntd, const TestData *td, int temp_unit){
	FpropsError fprops_err = FPROPS_NO_ERROR;

	double rho, T, p, u, h, a, s, cp0, T_adj = 0;

	if(temp_unit=='C')T_adj = 273.15;

	double maxerr = 0;
	//double se = 0, sse = 0;

	unsigned i;
	const unsigned n = ntd;

	fprintf(stderr,"Running through %d test points...\n",n);

	//PureFluid *P = ideal_prepare(eos,NULL);
	/* Checking CP0 values */

#define CP0_TEMP(T,RHO,DATA, ERROR) ideal_cp(T,RHO,DATA, ERROR)
	fprintf(stderr,"CP0 TESTS\n");
	for(i=0; i<n;++i){
		cp0 = td[i].cp0*1e3;
	 	ASSERT_TOL(CP0_TEMP, td[i].T+T_adj, td[i].rho, P->data, &fprops_err, cp0, cp0*1e-6);
	}
#undef CP0_TEMP

	/* Checking pressure values (proves phir_delta) */
	fprintf(stderr,"PRESSURE TESTS\n");
	for(i=0; i<n;++i){
		T = td[i].T+T_adj;
		rho = td[i].rho;
		FluidState S1 = fprops_set_Trho(T,rho,P,&fprops_err);
		p = td[i].p*1e6;
	 	ASSERT_PROP(p, S1, &fprops_err, p, p*1e-3);
	}

	/* checking enthalpy values */
	fprintf(stderr,"ENTHALPY TESTS\n");
	//se = 0, sse = 0;
	for(i=0; i<n;++i){
		T = td[i].T+T_adj;
		rho = td[i].rho;
		FluidState S1 = fprops_set_Trho(T,rho,P,&fprops_err);
		h = td[i].h*1e3;
	 	ASSERT_PROP(h, S1, &fprops_err, h, 1e-4*h);
	}

	/* Checking internal energy values (proves phi0_tau, phir_tau) */
	fprintf(stderr,"INTERNAL ENERGY TESTS\n");
	for(i=0; i<n;++i){
		T = td[i].T+T_adj;
		rho = td[i].rho;
		FluidState S1 = fprops_set_Trho(T,rho,P,&fprops_err);
		u = td[i].u*1e3;
		ASSERT_PROP(u, S1, &fprops_err, u, 1e3*u);
	}

	/* Checking entropy values */

	fprintf(stderr,"ENTROPY TESTS\n");
	//se = 0, sse = 0;
	for(i=0; i<n;++i){
		T = td[i].T+T_adj;
		rho = td[i].rho;
		FluidState S1 = fprops_set_Trho(T,rho,P,&fprops_err);
		s = td[i].s*1e3;
	 	ASSERT_PROP(s, S1, &fprops_err, s, 1e-4*s);
	}


	/* Checking helmholtz energy values */

	fprintf(stderr,"HELMHOLTZ ENERGY TESTS\n");
	for(i=0; i<n;++i){
		T = td[i].T+T_adj;
		rho = td[i].rho;
		FluidState S1 = fprops_set_Trho(T,rho,P,&fprops_err);
		a = td[i].a*1e3;
		//fprintf(stderr,"%.20e\t%.20e\t%.20e\n",T,rho,(a - fprops_a(T,rho,fluid)));
	 	ASSERT_PROP(a, S1, &fprops_err, a, a*1e-3);
	}

	/* TRIPLE POINT PROPERTY CALCULATION */		
	
	fprintf(stderr,"\nTRIPLE POINT PROPERTIES\n\n");
	assert(P->data->T_t != 0);
	fprintf(stderr,"T_t = %f K\n", P->data->T_t);
	double pt,rhoft,rhogt;
	fprops_triple_point(&pt, &rhoft, &rhogt, P, &fprops_err);
	assert(fprops_err == FPROPS_NO_ERROR);
	fprintf(stderr,"p_t = %.12e Pa\n", pt);
	fprintf(stderr,"rhof_t = %.12e kg/m^3\n", rhoft);
	fprintf(stderr,"rhog_t = %.12e kg/m^3\n\n", rhogt);

	/* Check convergence along saturation curve */

	fprintf(stderr,"SATURATION CURVE CONVERGENCE TEST\n");
	if(P->data->T_t != 0){	
		double n1 = 200;
		/* space point linearly in 1/T, cf Sandler 5e fig 7.7-1. */
		double rT = 1./P->data->T_t;
		double drT = (1./P->data->T_c - 1./P->data->T_t) / (n1 - 1);
		fprintf(stderr,"\t%10s\t%10s\t%10s\t%10s\n","T","psat", "rhof", "rhog");
		for(i = 0; i < n1; i++){
			T = 1/rT;
			double psat1, rhof1, rhog1;
			fprops_sat_T(T, &psat1, &rhof1, &rhog1, P, &fprops_err);
			assert(fprops_err == FPROPS_NO_ERROR);
			fprintf(stderr,"\t%10.3f\t%10.3f\t%10.5f\t%10.5f\n",T,psat1, rhof1, rhog1);
			rT += drT;
		}
	}else{
		fprintf(stderr,"NOT POSSIBLE due to missing T_t\n");
	}
	fprintf(stderr,"\n");

	fprintf(stderr,"Tests completed OK (maximum error = %0.5f%%)\n",maxerr);
	return 0;
}

int helm_check_u(const PureFluid *fluid, unsigned ntd, const TestData *td){
    //TODO: Use this enum:
    FpropsError fprops_err = FPROPS_NO_ERROR;
	unsigned i;
	double T,rho,u,err,se = 0,sse = 0;
	unsigned n = ntd;
	//double tol = 1e-1;

	fprintf(stderr,"INTERNAL ENERGY VALUES\n\n");
	fprintf(stderr,"%-18s\t%-18s\t%-18s\t%-18s\t%-18s\n","T","rho","u","du","%err");
	for(i=0; i<n;++i){
		T = td[i].T+273.15;
		rho = td[i].rho;
		u = td[i].u*1e3;
		FluidState S1 = fprops_set_Trho(T,rho,fluid,&fprops_err);
		err = u - fprops_u(S1,&fprops_err);
		se += err;
		sse += err*err;
		fprintf(stderr,"%.12e\t%.12e\t%.12e\t%.12e\t%.6f\n",T,rho,u,err,err/u*100.);
	}
	fprintf(stderr,"average error = %.10e\n",se/n);
	fprintf(stderr,"sse - n se^2 = %.3e\n",sse - n*se*se);
    return 0;
}

int helm_check_s(const PureFluid *fluid, unsigned ntd, const TestData *td){
    //TODO: Use this enum:
    FpropsError fprops_err=FPROPS_NO_ERROR;
	unsigned i;
	double T,rho,s,err,se = 0,sse = 0;
	unsigned n = ntd;
	//double tol = 1e-1;

	fprintf(stderr,"ENTROPY VALUES\n\n");
	fprintf(stderr,"%-18s\t%-18s\t%-18s\t%-18s\t%-18s\n","T","rho","s","ds","%err");
	for(i=0; i<n;++i){
		T = td[i].T + 273.15;
		rho = td[i].rho;
		s = td[i].s*1e3;
		FluidState S1 = fprops_set_Trho(T,rho,fluid,&fprops_err);
		err = s - fprops_s(S1,&fprops_err);
		se += err;
		sse += err*err;
		fprintf(stderr,"%.12e\t%.12e\t%.12e\t%.12e\t%.6f\n",T,rho,s,err,err/s*100.);
	}
	fprintf(stderr,"average error = %.10e\n",se/n);
	fprintf(stderr,"sse - n se^2 = %.3e\n",sse - n*se*se);
    return 0;
}

/* check derivatives of p */

#if 0
int helm_check_dpdT_rho(const PureFluid *fluid, unsigned ntd, const TestData *td){
    //TODO: Use this enum:
    FpropsError fprops_err=FPROPS_NO_ERROR;
	unsigned i;
	double T,rho,p,T1,p1,dpdT,dpdT_est,err,se = 0,sse = 0;
	unsigned n = ntd;
	//double tol = 1e-1;

	double dT = 0.0001 /* finite difference in temperature, in K */;

	fprintf(stderr,"(dp/dT)_rho RESULTS\n\n");
	fprintf(stderr,"%-18s\t%-18s\t%-18s\t%-18s\t%-18s\t%12s\t%12s\n","T","rho","p","dp/dT","dp/dT est","err","%err");
	for(i=0; i<n;++i){
		T = td[i].T + 273.15;
		rho = td[i].rho;
		p = fprops_p(T,rho,fluid,&fprops_err);
		dpdT = fprops_dpdT_rho(T,rho,fluid,&fprops_err);
		assert(!isinf(dpdT));
		T1 = T + dT;
		p1 = fprops_p(T1, rho, fluid,&fprops_err);
		dpdT_est = (p1 - p)/dT;
		err = (dpdT_est - dpdT);
		se += err;
		sse += err*err;
		fprintf(stderr,"%.12e\t%.12e\t%.12e\t%.12e\t%.12e\t%12.4e\t%12.2e\n",T,rho,p,dpdT,dpdT_est,err,err/dpdT*100	);
	}
	fprintf(stderr,"average error = %.10e\n",se/n);
	fprintf(stderr,"sse - n se^2 = %.3e\n",sse - n*se*se);
    return 0;
}

int helm_check_dpdrho_T(const PureFluid *fluid, unsigned ntd, const TestData *td){
    //TODO: Use this enum:
    FpropsError fprops_err=FPROPS_NO_ERROR;
	unsigned i;
	double T,rho,p,rho1,p1,dpdrho,dpdrho_est,err,se = 0,sse = 0;
	unsigned n = ntd;
    //TODO: tol unused. Delete?
//	double tol = 1e-1;

	double drho = 0.0001 /* finite difference in temperature, in K */;

	fprintf(stderr,"(dp/drho)_T RESULTS\n\n");
	fprintf(stderr,"%-18s\t%-18s\t%-18s\t%-18s\t%-18s\t%12s\t%12s\n","T","rho","p","dp/drho","dp/drho est","err","%err");
	for(i=0; i<n;++i){
		T = td[i].T + 273.15;
		rho = td[i].rho;
		p = fprops_p(T,rho,fluid,&fprops_err);
		dpdrho = fprops_dpdrho_T(T,rho,fluid,&fprops_err);
		assert(!isinf(dpdrho));
		rho1 = rho + drho;
		p1 = fprops_p(T, rho1, fluid,&fprops_err);
		dpdrho_est = (p1 - p)/drho;
		err = (dpdrho_est - dpdrho);
		se += err;
		sse += err*err;
		fprintf(stderr,"%.12e\t%.12e\t%.12e\t%.12e\t%.12e\t%12.4e\t%12.6f\n",T,rho,p,dpdrho,dpdrho_est,err,err/dpdrho*100);
	}
	fprintf(stderr,"average error = %.10e\n",se/n);
	fprintf(stderr,"sse - n se^2 = %.3e\n",sse - n*se*se);
    return 0;
}


int helm_check_d2pdrho2_T(const PureFluid *fluid, unsigned ntd, const TestData *td){
    //TODO: Use this enum:
    FpropsError fprops_err=FPROPS_NO_ERROR;
	unsigned i;
	double T,rho,p,dpdrho,rho1,dpdrho1,d2pdrho2,d2pdrho2_est,err,se = 0,sse = 0;
	unsigned n = ntd;
	//double tol = 1e-1;

	double drho = 0.0001 /* finite difference in temperature, in K */;

	fprintf(stderr,"\n(d2p/drho2)_T RESULTS\n\n");
	fprintf(stderr,"%-18s\t%-18s\t%-18s\t%-18s\t%-18s\t%12s\t%12s\t%12s\n","T","rho","p","dp/drho","d2p/drho2 est","d2p/drho2 calc","err","rel err");
	for(i=0; i<n;++i){
		T = td[i].T + 273.15;
		rho = td[i].rho;
		p = fprops_p(T,rho,fluid,&fprops_err);
		dpdrho = fprops_dpdrho_T(T,rho,fluid,&fprops_err);
		d2pdrho2 = fprops_d2pdrho2_T(T, rho, fluid,&fprops_err);
		assert(!isinf(d2pdrho2));
		rho1 = rho + drho;
		dpdrho1 = fprops_dpdrho_T(T, rho1, fluid,&fprops_err);
		d2pdrho2_est = (dpdrho1 - dpdrho)/drho;
		err = (d2pdrho2_est - d2pdrho2);
		se += err;
		sse += err*err;
		fprintf(stderr,"%.12e\t%.12e\t%.12e\t%.12e\t%.12e\t%12.4e\t%12.2e\t%12.3e\n",T,rho,p,dpdrho,d2pdrho2_est,d2pdrho2,err,err/d2pdrho2_est);
	}
	fprintf(stderr,"average error = %.10e\n",se/n);
	fprintf(stderr,"sse - n se^2 = %.3e\n",sse - n*se*se);
    return 0;
}

/* check derivatives of h */

int helm_check_dhdT_rho(const PureFluid *fluid, unsigned ntd, const TestData *td){
    //TODO: Use this enum:
    FpropsError fprops_err=FPROPS_NO_ERROR;
	unsigned i;
	double T,rho,h,T1,h1,dhdT,dhdT_est,err,se = 0,sse = 0;
	unsigned n = ntd;
	//double tol = 1e-1;
	double dT = 1e-6 /* finite difference in temperature, in K */;

	fprintf(stderr,"(dh/dT)_rho RESULTS\n\n");
	fprintf(stderr,"%-18s\t%-18s\t%-18s\t%-18s\t%-18s\t%12s\t%12s\n","T","rho","h","dh/dT","dh/dT est","err","%err");
	for(i=0; i<n;++i){
		T = td[i].T + 273.15;
		rho = td[i].rho;
		h = fprops_h(T,rho,fluid,&fprops_err);
		dhdT = fprops_dhdT_rho(T,rho,fluid,&fprops_err);
		assert(!isinf(dhdT));
		T1 = T + dT;
		h1 = fprops_h(T1, rho, fluid,&fprops_err);
		dhdT_est = (h1 - h)/dT;
		err = (dhdT_est - dhdT);
		se += err;
		sse += err*err;
		fprintf(stderr,"%.12e\t%.12e\t%.12e\t%.12e\t%.12e\t%12.4e\t%12.2e\n",T,rho,h,dhdT,dhdT_est,err,err/dhdT*100	);
	}
	fprintf(stderr,"average error = %.10e\n",se/n);
	fprintf(stderr,"sse - n se^2 = %.3e\n",sse - n*se*se);
    return 0;
}


int helm_check_dhdrho_T(const PureFluid *fluid, unsigned ntd, const TestData *td){
    //TODO: Use this enum:
    FpropsError fprops_err=FPROPS_NO_ERROR;
	unsigned i;
	double T,rho,h,rho1,h1,dhdrho,dhdrho_est,err,se = 0,sse = 0;
	unsigned n = ntd;

	double drho = 1e-6 /* finite difference in temperature, in K */;

	fprintf(stderr,"(dh/drho)_T RESULTS\n\n");
	fprintf(stderr,"%-18s\t%-18s\t%-18s\t%-18s\t%-18s\t%12s\t%12s\n","T","rho","h","dh/drho","dh/drho est","err","%err");
	for(i=0; i<n;++i){
		T = td[i].T + 273.15;
		rho = td[i].rho;
		h = fprops_h(T,rho,fluid,&fprops_err);
		dhdrho = fprops_dhdrho_T(T,rho,fluid,&fprops_err);
		assert(!isinf(dhdrho));
		rho1 = rho + drho;
		h1 = fprops_h(T, rho1, fluid,&fprops_err);
		dhdrho_est = (h1 - h)/drho;
		err = (dhdrho_est - dhdrho);
		se += err;
		sse += err*err;
		fprintf(stderr,"%.12e\t%.12e\t%.12e\t%.12e\t%.12e\t%12.4e\t%12.2e\n",T,rho,h,dhdrho,dhdrho_est,err,err/dhdrho*100	);
	}
	fprintf(stderr,"average error = %.10e\n",se/n);
	fprintf(stderr,"sse - n se^2 = %.3e\n",sse - n*se*se);
    return 0;
}

/* check derivatives of u */

int helm_check_dudT_rho(const PureFluid *fluid, unsigned ntd, const TestData *td){
    //TODO: Use this enum:
    FpropsError fprops_err=FPROPS_NO_ERROR;
	unsigned i;
	double T,rho,u,T1,u1,dudT,dudT_est,err,se = 0,sse = 0;
	unsigned n = ntd;

	double dT = 1e-3 /* finite difference in temperature, in K */;

	fprintf(stderr,"(du/dT)_rho RESULTS\n\n");
	fprintf(stderr,"%-18s\t%-18s\t%-18s\t%-18s\t%-18s\t%12s\t%12s\n","T","rho","h","du/dT","du/dT est","err","%err");
	for(i=0; i<n;++i){
		T = td[i].T + 273.15;
		rho = td[i].rho;
		u = fprops_u(T,rho,fluid,&fprops_err);
		dudT = fprops_dudT_rho(T,rho,fluid,&fprops_err);
		assert(!isinf(dudT));
		T1 = T + dT;
		u1 = fprops_u(T1, rho, fluid,&fprops_err);
		dudT_est = (u1 - u)/dT;
		err = (dudT_est - dudT);
		se += err;
		sse += err*err;
		fprintf(stderr,"%.12e\t%.12e\t%.12e\t%.12e\t%.12e\t%12.4e\t%12.2e\n",T,rho,u,dudT,dudT_est,err,err/dudT*100	);
	}
	fprintf(stderr,"average error = %.10e\n",se/n);
	fprintf(stderr,"sse - n se^2 = %.3e\n",sse - n*se*se);
    return 0;
}


int helm_check_dudrho_T(const PureFluid *fluid, unsigned ntd, const TestData *td){
    //TODO: Use this enum:
    FpropsError fprops_err=FPROPS_NO_ERROR;

	unsigned i;
	double T,rho,u,rho1,u1,dudrho,dudrho_est,err,se = 0,sse = 0;
	unsigned n = ntd;
	//double tol = 1e-1;

	double drho = 1e-6 /* finite difference in temperature, in K */;

	fprintf(stderr,"(du/drho)_T RESULTS\n\n");
	fprintf(stderr,"%-18s\t%-18s\t%-18s\t%-18s\t%-18s\t%12s\t%12s\n","T","rho","h","du/drho","du/drho est","err","%err");
	for(i=0; i<n;++i){
		T = td[i].T + 273.15;
		rho = td[i].rho;
		u = fprops_u(T,rho,fluid,&fprops_err);
		dudrho = fprops_dudrho_T(T,rho,fluid,&fprops_err);
		assert(!isinf(dudrho));
		rho1 = rho + drho;
		u1 = fprops_u(T, rho1, fluid,&fprops_err);
		dudrho_est = (u1 - u)/drho;
		err = (dudrho_est - dudrho);
		se += err;
		sse += err*err;
		fprintf(stderr,"%.12e\t%.12e\t%.12e\t%.12e\t%.12e\t%12.4e\t%12.2e\n",T,rho,u,dudrho,dudrho_est,err,err/dudrho*100	);
	}
	fprintf(stderr,"average error = %.10e\n",se/n);
	fprintf(stderr,"sse - n se^2 = %.3e\n",sse - n*se*se);
    return 0;
}

#endif

int helm_calc_offsets(double Tref, double rhoref, double href, double sref, const PureFluid *fluid){
    //TODO: Use this enum:
    FpropsError fprops_err=FPROPS_NO_ERROR;
	FluidState S1 = fprops_set_Trho(Tref,rhoref,fluid,&fprops_err);
	double h = fprops_h(S1,&fprops_err);
	double s = fprops_s(S1,&fprops_err);

	fprintf(stderr,"\nH,S REFERENCE STATE CALCULATION\n\n");

	fprintf(stderr,"Tref = %f\n",Tref);
	fprintf(stderr,"rhoref = %f\n",rhoref);
	fprintf(stderr,"--> h(Tref,rhoref) = %.14e  (wanted %.14e)\n",h, href);
	fprintf(stderr,"--> s(Tref,rhoref) = %.14e  (wanted %.14e)\n",s, sref);

	double m_new = fluid->data->cp0->m + (href - h)/fluid->data->R/fluid->data->corr.helm->T_star;

	double c_new = fluid->data->cp0->c - (sref - s)/fluid->data->R;

	fprintf(stderr,"c_new = %.20e\n",c_new);
	fprintf(stderr,"m_new = %.20e\n\n",m_new);

	return 0;
}



int helm_run_saturation_tests(const PureFluid *fluid, unsigned nsd, const TestDataSat *td, int temp_unit){
    //TODO: Use this enum:
    FpropsError fprops_err=FPROPS_NO_ERROR;
	double T, p, rhof, rhog, hf, hg, sf, sg, T_adj = 0;
	if(temp_unit=='C')T_adj = 273.15;

	double maxerr = 0;

	unsigned i;
	const unsigned n = nsd;

	fprintf(stderr,"\nSATURATION TESTS\n\n");

	fprintf(stderr,"Running through %d saturation-region points...\n",n);
	double tol = 1e5;
	for(i=0; i<n;++i){
		T = td[i].T+T_adj;
        //TODO: res unused. Delete?
		fprops_sat_T(T, &p, &rhof, &rhog, fluid, &fprops_err);
		assert(FPROPS_NO_ERROR == fprops_err);
		double p_tab = td[i].p*1e6;
		ASSERT_TOL_VAL(p,p_tab,p_tab*tol);
		double rhof_tab = td[i].rhof;
		ASSERT_TOL_VAL(rhof,rhof_tab,rhof_tab*tol);
		double rhog_tab = td[i].rhog;
		ASSERT_TOL_VAL(rhog,rhog_tab,rhog_tab*tol);
		hf = td[i].hf*1e3;
		FluidState Sf = fprops_set_Trho(T,rhof,fluid,&fprops_err);
		FluidState Sg = fprops_set_Trho(T,rhog,fluid,&fprops_err);
	 	ASSERT_PROP(h,Sf, &fprops_err, hf, hf*tol);
		hg = td[i].hg*1e3;
	 	ASSERT_PROP(h,Sg, &fprops_err, hg, hg*tol);
		sf = td[i].sf*1e3;
	 	ASSERT_PROP(s,Sf, &fprops_err, sf, sf*tol);
		sg = td[i].sg*1e3;
	 	ASSERT_PROP(s,Sg, &fprops_err, sg, sg*tol);
	}

	fprintf(stderr,"Tests completed OK (maximum error = %0.5f%%)\n",maxerr);
	return 0;
}

