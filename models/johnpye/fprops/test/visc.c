
#include "../fluids.h"
#include "../fprops.h"
#include "../solve_ph.h"
#include "../refstate.h"
#include "../sat.h"
#include "../visc.h"

#include <assert.h>
#include <math.h>

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

#define TOL_T 1e-8
#define TOL_RHO 1e-3

int main(void){
	const PureFluid *P;
	const char *fluids[] = {"carbondioxide",NULL};
	const char **fi = fluids;
	int nfluiderrors = 0;
	while(*fi){
		MSG("Testing fluid '%s'",*fi);
		FpropsError err = FPROPS_NO_ERROR;
		P = fprops_fluid(*fi,NULL,"Span");
		assert(P);
		int testedpoints;
		int nerr;

		double T = 300.;
		double rho = 1300.;
		FluidState S = fprops_set_Trho(T, rho, P, &err);
		MSG("State: S.T = %f, S.rho = %f", S.T, S.rho);
		MSG("err = %d",err);

		double mu = fprops_mu(S,&err);
		MSG("mu = %f",mu);

		if(err)nfluiderrors++;
		++fi;
	}
	
	if(nfluiderrors){
		ERRMSG("There were %d fluids with viscosity errors",nfluiderrors);
		return 1;
	}


	/* all done? report success */
	fprintf(stderr,"\n");
	color_on(stderr,ASC_FG_BRIGHTGREEN);
	fprintf(stderr,"SUCCESS (%s)",__FILE__);
	color_off(stderr);	
	fprintf(stderr,"\n");
	return 0;
}

