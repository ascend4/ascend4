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

	#ifndef PT
	#define PT P->data->table
	PureFluid *P,*Ph;
	FpropsError err;
	const char *helmfluids[] = { "water"};
	//const int n = sizeof(helmfluids)/sizeof(char *);
	int i;

	MSG("Which Fluid? -->  %s",helmfluids[0]);

	P = (PureFluid *)fprops_fluid(helmfluids[0],"ttse",NULL);
	Ph = (PureFluid *)fprops_fluid(helmfluids[0],"helmholtz",NULL);

	MSG("Triple Point Tt %f  & Crit Temp  %f",P->data->T_t,P->data->T_c);

	MSG("Comparing Helmholtz vs TTSE");
	double rho = 1000;

	//Plot in Mathematica Saturation dome
	double avgprf=0,avgprg=0;

	int npoints =NSAT*4.7;

	for(i=0;i<npoints;i++){

		double Tt = P->data->T_t;
		double Tc = P->data->T_c;
		double dt2p = (Tc - Tt)/NSAT;
		double T = Tt + (i+0.5)*dt2p;
		// MSG("%f",T);
		double psat, rhof,rhog;
		fprops_sat_T(T,&psat,&rhof,&rhog,Ph,&err);
		int j = (int)round(  ((T - Tt)/(Tc - Tt))*(NSAT)  );
		assert(j>=0 && j<NSAT);
		//double delt = T - ( Tt + j*dt2p);
		double rhofT,rhogT;
		psat = ttse_sat(T,&rhofT,&rhogT,P->data,&err);

		//  rhofT =   P->table->satFRho[j] + delt*P->table->satFdRhodt[j]+ 0.5*delt*delt*P->table->satFd2RhodT2[j];
		//  rhogT =   P->table->satGRho[j] + delt*P->table->satGdRhodt[j]+ 0.5*delt*delt*P->table->satGd2RhodT2[j];

		avgprf += fabs(100*(rhof-rhofT)/rhof);
		avgprg += fabs(100*(rhog-rhogT)/rhog);
		// MSG("%f  %f  %f  %f  %f  %f", rhof,rhofT,rhog,rhogT, 100*(rhof-rhofT)/rhof,100*(rhog-rhogT)/rhog );

#if 0
		// For mathematica print out.
		if(i==0)
		    printf("{{%f, %f, %f},\n",T, rhofT,rhogT);
		else if(i==npoints-1)
		    printf("{%f, %f, %f}};\n",T, rhofT,rhogT);
		else
		    printf("{%f, %f, %f},\n",T, rhofT,rhogT);
#endif

	}
	avgprf /= (npoints);
	avgprg /= (npoints);
	MSG("Average percent error in  rhof and rhog evaluations respectively --> %f  %f",avgprf,avgprg);

	/*****************************************Single Phase Table Testing*****************************************/
	#define NPOINTS 100000
	double temp_s = 650;
	double temp_f = 1650;
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
		//FluidState S = fprops_set_Trho(T,rho,P,&err);
		//pressH[i] = fprops_p(S,&err) ;
		pressH[i] = Ph->p_fn(T, rho, Ph->data,&err) ;
		enthalpyH[i] = Ph->h_fn(T, rho, Ph->data,&err) ;
		entH[i] = Ph->s_fn(T, rho, Ph->data,&err) ;
		intuH[i] = Ph->u_fn(T, rho, Ph->data,&err) ;
		gibbsgH[i] = Ph->g_fn(T, rho, Ph->data,&err) ;
	}

	clock_t end = clock();
	double msecH = (double)(end - start) / (CLOCKS_PER_SEC/1000);

	start = clock();
	for(i=0; i<nT; ++i){
		err = FPROPS_NO_ERROR;

		double T = temp_s + i*dT;

		pressT[i] = ttse_p(T, rho,P->data, &err) ;
		enthalpyT[i] =  ttse_h(T, rho,P->data, &err)  ;
		entT[i] = ttse_s(T, rho,P->data, &err)  ;
		intuT[i] =  ttse_u(T, rho,P->data, &err) ;
		gibbsgT[i] = ttse_g(T, rho,P->data, &err) ;
	}

	end = clock();
	double msecT = (double)(end - start) / (CLOCKS_PER_SEC/1000);


	//  MSG("Percentage Errors");
	//  MSG("Temp     \tPressure \tEnthalpy ");
	double pererrp,pererrh;
	for(i=0; i<nT; ++i){
		//double T = temp_s + i*dT;
		pererrp = 100*((pressT[i]-pressH[i])/pressH[i]);
		pererrh = 100*((enthalpyT[i]-enthalpyH[i])/enthalpyH[i]);

	  //  MSG("%3.6f\t%3.6f\t%3.6f",T, pererrp,pererrh );
	}

	// MSG("Percentage Errors");
	//  MSG("Temp     \t\tEntropy  \t\tU        \t\tG        ");
	double pererrs,pererru,pererrg;

	for(i=0; i<nT; ++i){
	  //  double T = temp_s + i*dT;
		pererrs = 100*((entT[i]-entH[i])/entH[i]);
		pererru = 100*((intuT[i]-intuH[i])/intuH[i]);
		pererrg = 100*((gibbsgT[i]-gibbsgH[i])/gibbsgH[i]);

	 //   MSG("%3.6f\t%3.6f\t%3.6f\t%3.6f",T, pererrs,pererru,pererrg );
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

	MSG("AVERAGE percentage errors for 5 variables p h s u and g respectively -->");
	MSG("%3.6f\t%3.6f\t%3.6f\t%3.6f\t%3.6f",av[0]/nT,av[1]/nT,av[2]/nT,av[3]/nT,av[4]/nT);

	MSG("Helmholtz did %d calculations in %e seconds", nT*5,msecH/1000);
	MSG("TTSE did %d calculations in %e seconds", nT*5,msecT/1000);

	return 1;



	#endif // PT
}










