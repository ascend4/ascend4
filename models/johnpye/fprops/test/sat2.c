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

//#define TESTHELM
//#define TESTRPP

#define FNAME(F) #F
#define COMMA ,
	const char *helmfluids[] = { FLUIDS(FNAME,COMMA) COMMA RPPFLUIDS(FNAME,COMMA) };
#undef FNAME
	const char *corrtypes[] = {"pengrob","helmholtz"};
	const char *corrinitial[] = {"P","H"};
	enum corrtypes_enum {CORRTYPE_PENGROB,CORRTYPE_HELMHOLTZ,CORRTYPE_N};
#define FHELM(F) CORRTYPE_HELMHOLTZ
#define FPR(F) CORRTYPE_PENGROB
	const enum corrtypes_enum corrfluids[] = {FLUIDS(FHELM,COMMA) COMMA RPPFLUIDS(FPR,COMMA) };
#undef FHELM
#undef FPR
#define FRPP(F) "RPP"
#define FUN(F) NULL
	const char *srcfluids[] = {FLUIDS(FUN,COMMA) COMMA RPPFLUIDS(FRPP,COMMA) };
#undef FUN
#undef FRPP
#undef COMMA

	char nloggederrors = 0;
#define MAXNLOGGEDERRORS 25
#define MAXERRORLOG (MAXNLOGGEDERRORS*80)
	char errorlog[MAXERRORLOG] = "";

#define ERRLOG(FMT,...) \
	if(nloggederrors < MAXNLOGGEDERRORS){\
		int l = strlen(errorlog);\
		char *s = errorlog + l;\
		snprintf(s,MAXERRORLOG-l,FMT "\n",##__VA_ARGS__);\
	}\
	nloggederrors++;

	const int n = sizeof(helmfluids)/sizeof(char *);
	int i,j;
	int nerrfluids = 0;
	const char *errfluids[n];	
	MSG("Testing convergence of saturation curves for all fluids...");
	for(i=0; i<n; ++i){
		int nerr = 0;
		P = fprops_fluid(helmfluids[i],corrtypes[corrfluids[i]],srcfluids[i]);
		if(!P){
			MSG("Error initialising fluid '%s' type '%s'",helmfluids[i],corrtypes[corrfluids[i]]);
			nerr = 1;
			color_on(stdout,ASC_FG_BRIGHTRED);
			fprintf(stdout,"%s",corrinitial[corrfluids[i]]);
			color_off(stdout);
		}else{
			double Tt = P->data->T_t;
			double Tc = P->data->T_c;
			if(P->data->T_min > Tt){ // or if Tt is zero
				Tt= P->data->T_min;
			}
			if(Tt == 0 && P->data->T_f != 0){
				Tt = 0.9 * P->data->T_f + 0.1 * Tc ;
				// use the freezing point as a proxy for the triple point
			}
			
			if(Tt == 0){
				Tt = 273.15 - 20;
				if(Tt > Tc){
					Tt = 0.4 * Tc;
					color_on(stdout,ASC_FG_GREEN);
					fprintf(stdout,"%s",corrinitial[corrfluids[i]]);
					color_off(stdout);
				}else{
					color_on(stdout,ASC_FG_YELLOW);
					fprintf(stdout,"%s",corrinitial[corrfluids[i]]);
					color_off(stdout);
				}
			}else{
				color_on(stdout,ASC_FG_BRIGHTGREEN);
				fprintf(stdout,"%s",corrinitial[corrfluids[i]]);
				color_off(stdout);
			}
			double nT = 150;
			double rT = 1/Tt;
			double drT = (1/Tc - 1/Tt) / nT;
			if(P->data->T_t == 0 && P->data->T_min == 0){
				color_on(stdout,ASC_FG_BRIGHTRED);
			}
			fprintf(stdout,"%6.1f",Tt);
			color_off(stdout);
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
					ERRLOG("sat_T(%f) for '%s', omega=%f",T,P->name,P->data->omega);
				}else{
					fprintf(stdout,".");
				}
				rT += drT;
			}
			fprintf(stdout,"%6.1f",Tc);
		}
		fprintf(stdout,":%s",helmfluids[i]);
		fprintf(stdout,"\n");
		if(nerr)errfluids[nerrfluids++] = helmfluids[i];
		fprops_fluid_destroy(P); P = NULL;
	}

	if(nerrfluids){
		MSG("There were %d fluids with saturation curve errors.",nerrfluids);
#if 1
		for(i=0; i<nerrfluids; ++i){
			fprintf(stderr,"  %s",errfluids[i]);
		}
		fprintf(stderr,"\n");
#endif
	}
#if 0
	if(nloggederrors){
		fprintf(stderr,"\n");
		MSG("First %d of the %d errors logged:",MAXNLOGGEDERRORS,nloggederrors);
		fprintf(stderr,"%s",errorlog);
	}
#endif

	if(nerrfluids)return nerrfluids;

	fprintf(stderr,"\n");
	color_on(stderr,ASC_FG_BRIGHTGREEN);
	fprintf(stderr,"SUCCESS (%s)",__FILE__);
	color_off(stderr);
	fprintf(stderr,"\n");
	return 0;
}

