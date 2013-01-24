#include "../fluids.h"
#include "../fprops.h"
#include "../solve_Tx.h"
#include "../sat.h"
#include <assert.h>
#include <math.h>
#include "../fluids/fluids_list.h"
#include "../color.h"

#define MSG(FMT, ...) \
	color_on(stderr,ASC_FG_BRIGHTRED);\
	fprintf(stderr,"%s:%d: ",__FILE__,__LINE__);\
	color_on(stderr,ASC_FG_BRIGHTBLUE);\
	fprintf(stderr,"%s: ",__func__);\
	color_off(stderr);\
	fprintf(stderr,FMT "\n",##__VA_ARGS__)

#define ERRMSG(STR,...) \
	color_on(stderr,ASC_FG_BRIGHTRED);\
	fprintf(stderr,"ERROR:");\
	color_off(stderr);\
	fprintf(stderr," %s:%d:" STR "\n", __func__, __LINE__ ,##__VA_ARGS__)

#define TOL_T 1e-3
#define TOL_RHO 1e-3


int main(void){
	const PureFluid *P;
	FpropsError err;
	FluidState S;

#define FNAME(F) #F
#define COMMA ,
	const char *helmfluids[] = { FLUIDS(FNAME,COMMA) COMMA RPPFLUIDS(FNAME,COMMA) };
#undef FNAME
#undef COMMA
	const int n = sizeof(helmfluids)/sizeof(char *);
	int i,j;
	int nerrfluids = 0;
	const char *errfluids[n];
	MSG("Testing convergence of saturation curves for all helmholtz fluids...");
	for(i=0; i<n; ++i){
		int nerr = 0;
		P = fprops_fluid(helmfluids[i],NULL,NULL);
		if(!P){
			MSG("Error initialising fluid '%s'",helmfluids[i]);
			nerr = 1;
			color_on(stdout,ASC_FG_BRIGHTRED);
			fprintf(stdout,"I");
			color_off(stdout);
		}else{
			double Tt = P->data->T_t;
			double Tc = P->data->T_c;
			if(Tt == 0){
				color_on(stdout,ASC_FG_YELLOW);
				fprintf(stdout,"T");
				color_off(stdout);
				Tt = 273.15 - 20;
				if(Tt > Tc){
					Tt = 0.5 * Tc;
				}
			}else{
				color_on(stdout,ASC_FG_BRIGHTGREEN);
				fprintf(stdout,"I");
				color_off(stdout);
			}
			double nT = 170;
			double rT = 1/Tt;
			double drT = (1/Tc - 1/Tt) / nT;
			for(j=0; j<nT; ++j){
				double T = 1/rT;
				double psat,rhof,rhog;
				err = FPROPS_NO_ERROR;
				fprops_sat_T(T, &psat, &rhof, &rhog, P, &err);
				if(err){
					nerr++;
					color_on(stdout,ASC_FG_BRIGHTRED);
					fprintf(stdout,"C");
					color_off(stdout);
				}else{
					fprintf(stdout,".");
				}
				rT += drT;
			}
		}
		fprintf(stdout,":%s\n",helmfluids[i]);
		if(nerr)errfluids[nerrfluids++] = helmfluids[i];
	}

	MSG("There were %d fluids with saturation curve errors:",nerrfluids);
	for(i=0; i<nerrfluids; ++i){
		fprintf(stderr,"  %s",errfluids[i]);
	}
	fprintf(stderr,"\n");
	if(nerrfluids)return nerrfluids;

	double T0, rho, rhof, rhog, psat1, psat2;

#define TEST_SAT(T1) \
	T0 = T1; \
	fprops_solve_Tx(T0,0,&rho,P,&err); \
	assert(!err); \
	S = fprops_set_Trho(T0,rho,P,&err); \
	assert(!err); \
	rhof = S.rho; \
	psat1 = fprops_p(S,&err); \
	assert(!err); \
	fprops_solve_Tx(T0,1,&rho,P,&err); \
	S = fprops_set_Trho(T0,rho,P,&err); \
	assert(!err); \
	assert(!err); \
	psat2 = fprops_p(S,&err); \
	assert(!err); \
	rhog = S.rho; \
	assert(fabs(psat1 - psat2) < 1e-3); \
	/*MSG("At T = %f K (%f C), psat = %f, rhof = %f, rhog = %f",T0,T0-273.15,psat1,rhof,rhog);*/

	const char *fluids[] = {"water","toluene","ethanol","isohexane", NULL};
//	const char *fluids[] = {"toluene",NULL};
	const char **fi = fluids;
	while(*fi){
		MSG("TESTING %s",*fi);
		P = fprops_fluid(*fi,"pengrob",NULL);
		assert(P);
		err = FPROPS_NO_ERROR;

		double psat,rhof,rhog;
		fprops_triple_point(&psat, &rhof, &rhog, P, &err);
		assert(!err);
		++fi;
	}

	P = fprops_fluid("water","helmholtz",NULL);
	assert(P);
	err = FPROPS_NO_ERROR;

	// low-density saturation cases (I think)
	TEST_SAT(273.15+4.1);
	TEST_SAT(273.15+3.9);
	TEST_SAT(273.15+4);
	TEST_SAT(275.212471); 
	TEST_SAT(275.212471);
	TEST_SAT(2.732910e+02);
	TEST_SAT(2.731868e+02);
	TEST_SAT(2.844904e+02);

	psat1 = 709.144373;
	fprops_sat_p(psat1,&T0,&rhof,&rhog,P,&err);
	assert(!err);

	P = fprops_fluid("isohexane","helmholtz",NULL);
	assert(P);
	err = FPROPS_NO_ERROR;

	// low-density saturation cases (I think)
	TEST_SAT(P->data->T_t);

	/*MSG("At p = %f Pa, got T = %f K (%f C), rhof = %f, rhog = %f", psat1, T0, T0-273.15, rhof, rhog)*/;

	fprintf(stderr,"\n");
	color_on(stderr,ASC_FG_BRIGHTGREEN);
	fprintf(stderr,"SUCCESS (%s)",__FILE__);
	color_off(stderr);
	fprintf(stderr,"\n");
	return 0;
}

