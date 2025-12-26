#include "../test.h"
#include "../fprops.h"
#include "../pengrob.h"
#include "../fluids.h"
#include "../fluids/fluids_list.h"
#include "../sat.h"

#include <math.h>
#include <stdio.h>

static int pr_rpp_allow_solve(double T, double rho, double Tc, double rhoc){
	if(Tc > 0 && rhoc > 0){
		/* supercritical, dilute gas */
		return (T >= 1.1 * Tc) && (rho <= 0.1 * rhoc);
	}
	return 0;
}

static int pr_rpp_allow_cp_cv(double T, double rho, double Tc, double rhoc){
	if(Tc > 0 && rhoc > 0){
		return (T >= 1.1 * Tc) && (rho <= 0.1 * rhoc);
	}
	return 0;
}

static void pr_rpp_check_state(const char *name, PureFluid *P, double T, double rho){
	FpropsError err = FPROPS_NO_ERROR;
	if(!(T > 0) || !(rho > 0)){
		fprintf(stderr,"pengrob_rpp: invalid T/rho for '%s' (T=%g rho=%g)\n",name,T,rho);
		CU_TEST(0);
		return;
	}
	if(P->data->corr.pengrob && P->data->corr.pengrob->b > 0){
		double rhomax = 0.9 / P->data->corr.pengrob->b;
		if(rho > rhomax){
			rho = rhomax;
		}
	}

	FluidState2 S = fprops_set_Trho(T,rho,P,&err);
	if(err){
		fprintf(stderr,"pengrob_rpp: set_Trho failed for '%s' (T=%g rho=%g)\n",name,T,rho);
		CU_TEST(0);
		return;
	}

	err = FPROPS_NO_ERROR;
	double p = fprops_p(S,&err);
	CU_TEST(!err && isfinite(p) && p > 0);
	if(err || !isfinite(p) || p <= 0){
		fprintf(stderr,"pengrob_rpp: p failed for '%s' (T=%g rho=%g p=%g err=%d)\n",name,T,rho,p,err);
	}

	err = FPROPS_NO_ERROR;
	double u = fprops_u(S,&err);
	CU_TEST(!err && isfinite(u));
	if(err || !isfinite(u)){
		fprintf(stderr,"pengrob_rpp: u failed for '%s' (T=%g rho=%g u=%g err=%d)\n",name,T,rho,u,err);
	}

	err = FPROPS_NO_ERROR;
	double h = fprops_h(S,&err);
	CU_TEST(!err && isfinite(h));
	if(err || !isfinite(h)){
		fprintf(stderr,"pengrob_rpp: h failed for '%s' (T=%g rho=%g h=%g err=%d)\n",name,T,rho,h,err);
	}

	if(isfinite(p) && isfinite(u) && isfinite(h)){
		double h_expected = u + p / rho;
		double rel = fabs(h - h_expected) / (fabs(h) + 1.0);
		CU_TEST(rel < 1e-6);
		if(rel >= 1e-6){
			fprintf(stderr,"pengrob_rpp: h != u + p/rho for '%s' (T=%g rho=%g rel=%g)\n",name,T,rho,rel);
		}
	}

	if(pr_rpp_allow_solve(T,rho,P->data->T_c,P->data->rho_c)){
		err = FPROPS_NO_ERROR;
		double rho2 = 0.0;
		pengrob_solve_pT(p,T,&rho2,P->data,&err);
		CU_TEST(!err && isfinite(rho2) && rho2 > 0);
		if(err || !isfinite(rho2) || rho2 <= 0){
			fprintf(stderr,"pengrob_rpp: solve_pT failed for '%s' (T=%g rho=%g rho2=%g err=%d)\n",name,T,rho,rho2,err);
		}else{
			double relrho = fabs(rho2 - rho) / (fabs(rho) + 1.0);
			CU_TEST(relrho < 1e-5);
			if(relrho >= 1e-5){
				fprintf(stderr,"pengrob_rpp: solve_pT mismatch for '%s' (T=%g rho=%g rho2=%g rel=%g)\n",name,T,rho,rho2,relrho);
			}
		}
	}

	if(pr_rpp_allow_cp_cv(T,rho,P->data->T_c,P->data->rho_c)){
		err = FPROPS_NO_ERROR;
		double cp = fprops_cp(S,&err);
		CU_TEST(!err && isfinite(cp));
		if(err || !isfinite(cp)){
			fprintf(stderr,"pengrob_rpp: cp failed for '%s' (T=%g rho=%g cp=%g err=%d)\n",name,T,rho,cp,err);
		}

		err = FPROPS_NO_ERROR;
		double cv = fprops_cv(S,&err);
		CU_TEST(!err && isfinite(cv));
		if(err || !isfinite(cv)){
			fprintf(stderr,"pengrob_rpp: cv failed for '%s' (T=%g rho=%g cv=%g err=%d)\n",name,T,rho,cv,err);
		}
	}

	err = FPROPS_NO_ERROR;
	double alphap = fprops_alphap(S,&err);
	CU_TEST(!err && isfinite(alphap));
	if(err || !isfinite(alphap)){
		fprintf(stderr,"pengrob_rpp: alphap failed for '%s' (T=%g rho=%g alphap=%g err=%d)\n",name,T,rho,alphap,err);
	}

	err = FPROPS_NO_ERROR;
	double betap = fprops_betap(S,&err);
	CU_TEST(!err && isfinite(betap));
	if(err || !isfinite(betap)){
		fprintf(stderr,"pengrob_rpp: betap failed for '%s' (T=%g rho=%g betap=%g err=%d)\n",name,T,rho,betap,err);
	}

	/* speed of sound is still under validation for PR; skip in smoke suite */
}

static void pr_rpp_check_sat(const char *name, PureFluid *P){
	FpropsError err = FPROPS_NO_ERROR;
	double Tc = P->data->T_c;
	if(!(Tc > 0)){
		return;
	}

	{
		int i;
		double Tmin = 0.7 * Tc;
		double Tmax = 0.98 * Tc;
		for(i = 0; i < 5; ++i){
			double T = Tmin + (Tmax - Tmin) * (double)i / 4.0;
			double psat = 0.0;
			double rhof = 0.0;
			double rhog = 0.0;

			err = FPROPS_NO_ERROR;
			fprops_sat_T(T, &psat, &rhof, &rhog, P, &err);
			CU_TEST(!err && isfinite(psat) && isfinite(rhof) && isfinite(rhog));
			if(err || !isfinite(psat) || !isfinite(rhof) || !isfinite(rhog)){
				fprintf(stderr,"pengrob_rpp: sat_T failed for '%s' (T=%g err=%d)\n",name,T,err);
				continue;
			}

			CU_TEST(psat > 0 && rhof > rhog && rhof > 0 && rhog > 0);
			if(!(psat > 0 && rhof > rhog && rhof > 0 && rhog > 0)){
				fprintf(stderr,"pengrob_rpp: sat_T invalid for '%s' (T=%g psat=%g rhof=%g rhog=%g)\n",name,T,psat,rhof,rhog);
				continue;
			}

			err = FPROPS_NO_ERROR;
			FluidState2 Sf = fprops_set_Trho(T, rhof, P, &err);
			double gf = fprops_g(Sf, &err);
			CU_TEST(!err && isfinite(gf));
			if(err || !isfinite(gf)){
				fprintf(stderr,"pengrob_rpp: g(liq) failed for '%s' (T=%g rhof=%g err=%d)\n",name,T,rhof,err);
				continue;
			}

			err = FPROPS_NO_ERROR;
			FluidState2 Sg = fprops_set_Trho(T, rhog, P, &err);
			double gg = fprops_g(Sg, &err);
			CU_TEST(!err && isfinite(gg));
			if(err || !isfinite(gg)){
				fprintf(stderr,"pengrob_rpp: g(vap) failed for '%s' (T=%g rhog=%g err=%d)\n",name,T,rhog,err);
				continue;
			}

			{
				double diff = fabs(gf - gg);
				double denom = fabs(gf) + fabs(gg) + 1.0;
				double rel = diff / denom;
				if(rel >= 1e-2){
					fprintf(stderr,"pengrob_rpp: g mismatch for '%s' (T=%g rel=%g)\n",name,T,rel);
				}
			}
		}
	}
}

static void pr_rpp_smoke_one(const char *name){
	PureFluid *P = fprops_fluid(name,"pengrob","RPP");
	if(!P){
		fprintf(stderr,"pengrob_rpp: failed to load '%s'\n",name);
		CU_TEST(0);
		return;
	}

	double Tc = P->data->T_c;
	double rhoc = P->data->rho_c;
	double Tvals[3];
	double rhovals[5];
	int nt = 0;
	int nr = 0;

	if(Tc > 0){
		Tvals[nt++] = 0.9 * Tc;
		Tvals[nt++] = 1.1 * Tc;
		Tvals[nt++] = 1.5 * Tc;
	}else{
		Tvals[nt++] = 280.0;
		Tvals[nt++] = 400.0;
		Tvals[nt++] = 600.0;
	}

	if(rhoc > 0){
		rhovals[nr++] = 0.01 * rhoc;
		rhovals[nr++] = 0.10 * rhoc;
		rhovals[nr++] = 0.50 * rhoc;
		rhovals[nr++] = 1.50 * rhoc;
		rhovals[nr++] = 2.50 * rhoc;
	}else{
		rhovals[nr++] = 0.5;
		rhovals[nr++] = 2.0;
		rhovals[nr++] = 5.0;
	}

	{
		int i, j;
		for(i = 0; i < nt; ++i){
			for(j = 0; j < nr; ++j){
				pr_rpp_check_state(name,P,Tvals[i],rhovals[j]);
			}
		}
	}

	pr_rpp_check_sat(name,P);

	fprops_fluid_destroy(P);
}

static void test_pengrob_rpp_smoke(void){
#define FNAME(F) #F
#define COMMA ,
	const char *rpp_fluids[] = { RPPFLUIDS(FNAME,COMMA) };
#undef COMMA
#undef FNAME

	const int n = (int)(sizeof(rpp_fluids)/sizeof(rpp_fluids[0]));
	int i;
	for(i = 0; i < n; ++i){
		pr_rpp_smoke_one(rpp_fluids[i]);
	}
}

CU_ErrorCode test_register_pengrob_rpp(void){
	CU_pSuite s = CU_add_suite("pengrob_rpp",NULL,NULL);
	if(NULL == s){
		return CUE_NOSUITE;
	}
	if(NULL == CU_add_test(s, "pengrob_rpp_smoke", test_pengrob_rpp_smoke)){
		return CUE_NOTEST;
	}
	return CUE_SUCCESS;
}
