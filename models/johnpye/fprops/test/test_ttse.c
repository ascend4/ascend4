

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
	//const int n = sizeof(helmfluids)/sizeof(char *);
	int i;


    MSG("Which Fluid? -->  %s",helmfluids[0]);

    P = (PureFluid *)fprops_fluid(helmfluids[0],"helmholtz",NULL);

    MSG("Comparing Helmholtz vs TTSE");


    double rho = 1200;



    #define NPOINTS 500
    double temp_s = 400;
    double temp_f = 500;
    int nT = NPOINTS;
    double dT = (temp_f-temp_s)/nT;



    double pressH[NPOINTS],enthalpyH[NPOINTS];
    double entH[NPOINTS], intuH[NPOINTS], gibbsgH[NPOINTS];

    double pressT[NPOINTS],enthalpyT[NPOINTS];
    double entT[NPOINTS], intuT[NPOINTS], gibbsgT[NPOINTS];


    clock_t start = clock();
    for(i=0; i<nT; ++i){
        err = FPROPS_NO_ERROR;

        double T = temp_s + i*dT;

        pressH[i] = P->p_fn(T, rho, P->data,&err) ;
        enthalpyH[i] = P->h_fn(T, rho, P->data,&err) ;
        entH[i] = P->s_fn(T, rho, P->data,&err) ;
        intuH[i] = P->u_fn(T, rho, P->data,&err) ;
        gibbsgH[i] = P->g_fn(T, rho, P->data,&err) ;
    }

    clock_t end = clock();
    double msecH = (double)(end - start) / (CLOCKS_PER_SEC/1000);

    start = clock();
    for(i=0; i<nT; ++i){
        err = FPROPS_NO_ERROR;

        double T = temp_s + i*dT;

        pressT[i] = evaluate_ttse_p(P,T, rho) ;
        enthalpyT[i] =  evaluate_ttse_h(P,T, rho)  ;
        entT[i] = evaluate_ttse_s(P,T, rho)  ;
        intuT[i] =  evaluate_ttse_u(P,T, rho) ;
        gibbsgT[i] = evaluate_ttse_g(P,T, rho) ;
    }

    end = clock();
    double msecT = (double)(end - start) / (CLOCKS_PER_SEC/1000);


    MSG("Percentage Errors");
    MSG("Temp     \tPressure \tEnthalpy ");
    double pererrp,pererrh;
    for(i=0; i<nT; ++i){
        double T = temp_s + i*dT;
        pererrp = 100*((pressT[i]-pressH[i])/pressH[i]);
        pererrh = 100*((enthalpyT[i]-enthalpyH[i])/enthalpyH[i]);

        MSG("%3.6f\t%3.6f\t%3.6f",T, pererrp,pererrh );
    }


    MSG("Percentage Errors");
    MSG("Temp     \t\tEntropy  \t\tU        \t\tG        ");
    double pererrs,pererru,pererrg;

    for(i=0; i<nT; ++i){
        double T = temp_s + i*dT;
        pererrs = 100*((entT[i]-entH[i])/entH[i]);
        pererru = 100*((intuT[i]-intuH[i])/intuH[i]);
        pererrg = 100*((gibbsgT[i]-gibbsgH[i])/gibbsgH[i]);

        MSG("%3.6f\t%3.6f\t%3.6f\t%3.6f",T, pererrs,pererru,pererrg );
    }

    double av[5]={0,0,0,0,0};
    i=0;
    while(i<nT){
        pererrp = 100*((pressT[i]-pressH[i])/pressH[i]);
        pererrh = 100*((enthalpyT[i]-enthalpyH[i])/enthalpyH[i]);
        pererrs = 100*((entT[i]-entH[i])/entH[i]);
        pererru = 100*((intuT[i]-intuH[i])/intuH[i]);
        pererrg = 100*((gibbsgT[i]-gibbsgH[i])/gibbsgH[i]);
        av[0] += fabs(pererrp);
        av[1] += fabs(pererrh);
        av[2] += fabs(pererrs);
        av[3] += fabs(pererru);
        av[4] += fabs(pererrg);
        ++i;
    }

    MSG("AVERAGE percentage errors for 5 variables p h s u and g");
    MSG("%3.6f\t%3.6f\t%3.6f\t%3.6f\t%3.6f\n",av[0]/nT,av[1]/nT,av[2]/nT,av[3]/nT,av[4]/nT);

    MSG("Helmholtz did %d calculations in %e seconds", nT*5,msecH/1000);
    MSG("TTSE did %d calculations in %e seconds", nT*5,msecT/1000);

    return 1;
}










