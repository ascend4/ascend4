#include "../ideal.h"
#include "../fluids.h"
#include "../fprops.h"
#include "../refstate.h"
#include "../color.h"

extern const EosData eos_rpp_methane;
extern const EosData eos_rpp_nitrogen;
extern const EosData eos_rpp_oxygen;
extern const EosData eos_rpp_carbon_dioxide;
extern const EosData eos_rpp_water;

#include <assert.h>
#include <math.h>

#define MSG FPROPS_MSG
#define ERRMSG FPROPS_ERRMSG

#define TOL_T 1e-3
#define TOL_RHO 1e-3

typedef struct BejanIdealGasData_struct{
	double Hplus;
	double Splus;
	double a,b,c,d;
} BejanIdealGasData;

/**
	@return cp0 in kJ/kmolK
*/
double bejan_cp(double T, BejanIdealGasData D){
	double y = 1e-3*T;
	return 1000 * (D.a + y*(D.b + D.d*y) + D.c/(y*y));
}

double rpp_cp_N2(double T){
	double y = T / 1;
	double cp0bar = 3.115e1 + y * (-1.357e-2 + y * (2.680e-5 + y * -1.168e-8));
	return cp0bar / 28.013 * 1000;
}

int main(void){
	int i;
	FpropsError err = FPROPS_NO_ERROR;
	
	MSG("Testing ideal EOS	");

	ReferenceState ref = {FPROPS_REF_REF0};
	enum MyFluids{N2,O2,CO2,H2O,CH4,NFLUIDS};
	PureFluid *P[NFLUIDS];
	P[N2] = ideal_prepare(&eos_rpp_nitrogen, &ref);
	P[O2] = ideal_prepare(&eos_rpp_oxygen, &ref);
	P[CO2] = ideal_prepare(&eos_rpp_carbon_dioxide, &ref);
	P[H2O] = ideal_prepare(&eos_rpp_water, &ref);
	P[CH4] = ideal_prepare(&eos_rpp_methane, &ref);

	BejanIdealGasData D[NFLUIDS];
	D[N2] = (BejanIdealGasData){-7.069, 51.539, 24.229, 10.521, 0.180, -2.315};
	D[O2] = (BejanIdealGasData){-9.589, 36.116, 29.154, 6.477, -0.184, -1.017};
	D[CO2] = (BejanIdealGasData){-413.886, -87.078, 51.128, 4.368, -1.469, 0};
	D[H2O] = (BejanIdealGasData){-253.871, -11.750, 34.376, 7.841, -0.423, 0};
	D[CH4] = (BejanIdealGasData){-81.242, 96.731, 11.933, 77.647, 0.142, -18.414};

	MSG("preparing helmholtz");
	const PureFluid *H_N2 = fprops_fluid("nitrogen","helmholtz",NULL);
	MSG("preparing pengrob");
	const PureFluid *PR_N2 = fprops_fluid("nitrogen","pengrob","RPP");

	fprintf(stderr,"temp / [K]\tcp_ideal\tcp_bejan\tcp_helmh\tcp0_helm\tcp0_pengrob\tcp_rpp\n");
	for(i=0;i<20;++i){
		double T = 273.15 + 10 * i;
		double cpb = bejan_cp(T, D[N2]) / P[N2]->data->M;
		double p = 1e5;
		double rho = p / P[N2]->data->R / T;
		double cp0 = ideal_cp(T, rho, P[N2]->data, &err);
		double cph = fprops_cp((FluidState){T, rho, H_N2}, &err);
		double cph0 = fprops_cp0((FluidState){T, rho, H_N2}, &err);
		double cpp0 = fprops_cp((FluidState){T, rho, PR_N2}, &err);	
		double cpr = rpp_cp_N2(T);
		fprintf(stderr,"%f\t%f\t%f\t%f\t%f\t%f\t%f\n",T,cp0,cpb,cph,cph0,cpp0,cpr);
	}

	for(i=0;i<NFLUIDS;++i){
		double T = 298.2;
		double cpb = bejan_cp(T, D[i]);
		double cp0 = ideal_cp(T, 0, P[i]->data, &err);
		MSG("cp(%s): bejan = %f, fprops = %f", P[i]->name, cpb, cp0);
		//double h0 = ideal_h(298.2, 0, P[i]->data, &err);
		//MSG("%-20s: M = %f, R = %f, cp0(298.2) = %f, hbar(298.2) = %f J/kmol",P[i]->name,P[i]->data->M, P[i]->data->R, cp0, h0*P[CH4]->data->M);
	}

	MSG("%-20s\t%s\t%s\t%s","comp","h(850 K)","h(1520 K)","Dh (kJ/kmol)");
	for(i=0;i<NFLUIDS;++i){
		double h1 = ideal_h(850,1000,P[i]->data,&err);
		double h2 = ideal_h(1520,1000,P[i]->data,&err);
		double Dh = h2 - h1;
		MSG("%-20s\t%f\t%f\t%f",P[i]->name,h1,h2,Dh);
	}


	fprintf(stderr,"\n");
	color_on(stderr,ASC_FG_BRIGHTGREEN);
	fprintf(stderr,"SUCCESS (%s)",__FILE__);
	color_off(stderr);
	fprintf(stderr,"\n");
	return 0;
}

