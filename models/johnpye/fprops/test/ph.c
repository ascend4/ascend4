
#include "../fluids.h"
#include "../fprops.h"
#include "../solve_ph.h"
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

#define TOL_T 1e-3
#define TOL_RHO 1e-3


int main(void){
	const PureFluid *P = fprops_fluid("water","helmholtz",NULL);
	assert(P);
	FpropsError err = FPROPS_NO_ERROR;
	FluidState S;

	double T0, rho0, p, h, T, rho;

#define TEST_PH(T1,RHO1) \
	MSG("TEST_PH(T1=%f, RHO1=%f)",T1,RHO1); \
	T0 = T1; \
	rho0 = RHO1; \
	S = fprops_set_Trho(T0,rho0,P,&err); \
	assert(!err); \
	p = fprops_p(S,&err); \
	assert(!err); \
	h = fprops_h(S,&err); \
	assert(!err); \
	fprops_solve_ph(p,h,&T,&rho,0,P,&err); \
	assert(!err); \
	assert(fabs(T - T0) < TOL_T); \
	assert(fabs(rho - rho0) < TOL_RHO);

	// low-density saturation cases (I think)
	TEST_PH(4.278618181818e+02, 3.591421624513e-03);
	TEST_PH(3.453541818182e+02, 6.899880592960e-03);
	TEST_PH(7.166385454545e+02, 6.899880592960e-03);
	TEST_PH(7.372654545455e+02, 4.092431601778e-03);

	TEST_PH(314.4054538268115, 999.7897902747587);

	TEST_PH(304.10372142680086, 999.7863801065582);

	TEST_PH(283.4886584572104, 999.7900620473787);

	TEST_PH(293.8028752316878, 999.7858245521049);


	fprintf(stderr,"\n");
	color_on(stderr,ASC_FG_BRIGHTGREEN);
	fprintf(stderr,"SUCCESS (%s)",__FILE__);
	color_off(stderr);	
	fprintf(stderr,"\n");
	return 0;
}

