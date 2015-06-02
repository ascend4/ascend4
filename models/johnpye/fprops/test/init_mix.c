/* 
	Initial model of a simple mixture, to get the procedure right.  This 
	is in preparation for a general algorithm to find mixing conditions 
	in the ideal-mixture case.

	Started May 28, 2015
 */

#include "../helmholtz.h"
#include "../fluids.h"
#include "../fprops.h"
#include "../refstate.h"
#include "../sat.h"

#include <stdio.h>
#include <assert.h>
#include <math.h>

/* TODO: maybe add nice colors from `color.h', later.
   Too much of a distraction to figure it out now. */

extern const EosData eos_rpp_nitrogen;
extern const EosData eos_rpp_ammonia;
extern const EosData eos_rpp_carbon_dioxide;
extern const EosData eos_rpp_methane;
extern const EosData eos_rpp_water;

/** 
	Calculate overall mass density of a mixture of components, given mass 
	density of components.

	@param nPure number of pure components
	@param x mass fractions of components
	@param rhos mass density of each component
	@return mass density of mixture
 */
double mixture_rho(unsigned nPure, double *x, double *rhos){
	double rho_mix;

	int i;
	for(i=0,rho_mix=0.0;i<nPure;i++){
		rho_mix += x[i]*rhos[i]; /* mixture mass density is simply sum of each mass density weighted by the mass fraction */
	}
	return rho_mix;
}

/**
	Get data for the fluids I want available for the mixture:
		nitrogen
		ammonia
		carbon dioxide
		methane
		water
		
	Then, establish mixture conditions, and find individual densities
 */
int main(void){
	enum FluidNames {N2,NH3,CO2,/*CH4,H2O,*/NFLUIDS};
	ReferenceState ref = {FPROPS_REF_REF0};

	const EosData *IdealEos[NFLUIDS]={
		&eos_rpp_nitrogen
			, &eos_rpp_ammonia
			, &eos_rpp_carbon_dioxide
			// , &eos_rpp_methane
			// , &eos_rpp_water
	};
	
	char *FluidNames[NFLUIDS]={
		"nitrogen", "ammonia", "carbondioxide"/* , "methane", "water" */
	};

	PureFluid *Ideals[NFLUIDS];
	PureFluid *Helms[NFLUIDS];

	FpropsError err = FPROPS_NO_ERROR;
	int i;
	for(i=N2;i<NFLUIDS;i++){
		Ideals[i] = ideal_prepare(IdealEos[i],&ref);
		Helms[i] = fprops_fluid(FluidNames[i],"helmholtz",NULL);
	}

	/* 	Mixing conditions, in temperature and pressure */
	double T=300; /* K */
	double P=1e5; /* Pa */
	double x[NFLUIDS] = {0.5, 0.3, 0.2}; /* mass fraction */
	/* Density is found as ideal-gas density for now */
	double rho[NFLUIDS];
	for(i=N2;i<NFLUIDS;i++){
		rho[i] = P / Ideals[i]->data->R / T;
		printf("\n\t%s%s is :  %.4f kg/m3", "The mass density of ", FluidNames[i], rho[i]);
	}

	/* mixture properties */
	double rho_mx = mixture_rho(NFLUIDS, x, rho);
	double p_mx=0.0, /* pressure and enthalpy, calculated from mixture mass densities */
		   h_mx=0.0;

	/* Calculate pressures and enthalpies with the Ideal model (ideal-gas mixture) */
	double p_i, h_i; /* these will hold individual pressures and enthalpies */
	printf("\n The ideal-gas case:");
	for(i=N2;i<NFLUIDS;i++){
		p_i = ideal_p(T, rho[i], Ideals[i]->data, &err);
		h_i = ideal_h(T, rho[i], Ideals[i]->data, &err);
		printf("\n\t%s %s\n\t\t%s  %g Pa;\n\t\t%s  %g J/kg.\n",
				"For the substance", FluidNames[i],
				"the pressure is  :", p_i,
				"the enthalpy is  :", h_i);
		p_mx += x[i] * p_i;
		h_mx += x[i] * h_i;
	}
	printf("\n\t%s\t\t:\t  %f kg/m3\n\t%s\t:\t  %g Pa\n\t%s\t\t:\t  %g J/kg\n",
			"The density of the mixture is", rho_mx,
			"The average pressure of the mixture is", p_mx,
			"The enthalpy of the mixture is", h_mx);

	/* Calculate pressures and enthalpies from Helmholtz model */
	printf("\n The non-ideal-gas case:");
	p_mx=0.0; /* reset mixture pressure and enthalpy to zero */
	h_mx=0.0;
	for(i=N2;i<NFLUIDS;i++){
		p_i = fprops_p((FluidState){T,rho[i],Helms[i]}, &err);
		h_i = fprops_h((FluidState){T,rho[i],Helms[i]}, &err);
		printf("\n\t%s %s\n\t\t%s  %g Pa;\n\t\t%s  %g J/kg.\n",
				"For the substance", FluidNames[i],
				"the pressure is  :", p_i,
				"the enthalpy is  :", h_i);
		p_mx += x[i] * p_i;
		h_mx += x[i] * h_i;
	}
	printf("\n\t%s\t\t:\t  %f kg/m3\n\t%s\t:\t  %g Pa\n\t%s\t\t:\t  %g J/kg\n",
			"The density of the mixture is", rho_mx,
			"The average pressure of the mixture is", p_mx,
			"The enthalpy of the mixture is", h_mx);

	/** 
		Now I drop the assumption that densities can be calculated from the 
		ideal-gas model, and use a root-finding method to find the densities 
		that each component must have to be at the pressure P.

		That is, since ideal-solution mixing is isobaric (constant pressure), the 
		density of each component is found by assuming it is at pressure P before 
		mixing, and solving for the density that will satisfy that condition.
	 */
	return 0;
}
