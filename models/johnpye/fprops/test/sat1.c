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
	FpropsError err = FPROPS_NO_ERROR;
	FluidState S;

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
	MSG("At T = %f K (%f C), psat = %f, rhof = %f, rhog = %f ('%s')",T0,T0-273.15,psat1,rhof,rhog,P->name);

	// following tests came form sat.c failures, mostly at close to critical point temp?
	P = fprops_fluid("trans_difluorodiazine","pengrob","RPP"); assert(P);
	TEST_SAT(259.958622);

	P = fprops_fluid("silicon_tetrafluoride","pengrob","RPP"); assert(P);
	TEST_SAT(258.859249);
	TEST_SAT(258.894422);
	TEST_SAT(258.929605);
	TEST_SAT(258.964798);

	P = fprops_fluid("boron_trifluoride","pengrob","RPP"); assert(P);
	TEST_SAT(260.753648);
	
	P = fprops_fluid("ozone","pengrob","RPP"); assert(P);
	TEST_SAT(260.955380);
	TEST_SAT(261.003569);
	TEST_SAT(261.051776);

	P = fprops_fluid("n_eicosane","pengrob","RPP"); assert(P);
	TEST_SAT(258.238420);
	TEST_SAT(254.514089);
	TEST_SAT(257.637699);

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

