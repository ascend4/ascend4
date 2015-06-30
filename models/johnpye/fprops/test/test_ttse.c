

#include "../fluids.h"
#include "../fprops.h"
#include "../solve_ph.h"
#include "../refstate.h"
#include "../sat.h"
#include "../visc.h"
#include "../ttse.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
	 PureFluid *P;
	FpropsError err;
    const char *helmfluids[] = { "water"};
	const int n = sizeof(helmfluids)/sizeof(char *);
	int i;


    printf("%d %s\n",n,helmfluids[0]);

    P = (PureFluid *)fprops_fluid(helmfluids[0],"helmholtz",NULL);

    MSG("Comparing Helmholtz vs TTSE");


    double rho = 1350;



    #define NPOINTS 500
    double temp_s = 280;
    double temp_f = 350;
    int nT = NPOINTS;
    double dT = (temp_f-temp_s)/nT;



    double pressH[NPOINTS],entH[NPOINTS],enthalpyH[NPOINTS];
    double pressT[NPOINTS],entT[NPOINTS],enthalpyT[NPOINTS];


    clock_t start = clock();
    for(i=0; i<nT; ++i){
        err = FPROPS_NO_ERROR;

        double T = temp_s + i*dT;

        pressH[i] = P->p_fn(T, rho, P->data,&err) ;
        entH[i] = P->s_fn(T, rho, P->data,&err) ;
        enthalpyH[i] = P->h_fn(T, rho, P->data,&err) ;
    }

    clock_t end = clock();
    double msecH = (double)(end - start) / (CLOCKS_PER_SEC/1000);

    start = clock();
    for(i=0; i<nT; ++i){
        err = FPROPS_NO_ERROR;

        double T = temp_s + i*dT;

        pressT[i] = evaluate_ttse_p(P,T, rho) ;
        entT[i] = evaluate_ttse_s(P,T, rho)  ;
        enthalpyT[i] =  evaluate_ttse_h(P,T, rho)  ;
    }

    end = clock();
    double msecT = (double)(end - start) / (CLOCKS_PER_SEC/1000);


    MSG("Percentage Errors");
    MSG("Temp        Pressure   Entropy     Enthalpy");



    for(i=0; i<nT; ++i){
        double T = temp_s + i*dT;
        double pererrp = 100*((pressT[i]-pressH[i])/pressH[i]);
        double pererrs = 100*((entT[i]-entH[i])/entH[i]);
        double pererrh = 100*((enthalpyT[i]-enthalpyH[i])/enthalpyH[i]);

        MSG("%f   %f   %f     %f",T, pererrp,pererrs,pererrh );
    }

    MSG("Helmholtz did %d calculations in %e seconds", nT*3,msecH/1000);
    MSG("TTSE did %d calculations in %e seconds", nT*3,msecT/1000);

    return 1;
}










