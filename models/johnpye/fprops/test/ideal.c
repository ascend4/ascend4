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

int main(void){
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

	int i;
	MSG("%-20s\t%s\t%s\t%s","comp","h(850 K)","h(1520 K)","Dh (kJ/kmol)");
	for(i=0;i<NFLUIDS;++i){
		double h0 = ideal_h(298.2, 0, P[i]->data, &err);
		MSG("%-20s: hbar(298.2) = %f J/kmol",P[i]->name,h0*P[CH4]->data->M);
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

