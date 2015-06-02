/**
	Initial model of a simple mixture, to get the procedure right.  This 
	is in preparation for a general algorithm to find mixing conditions 
	in the ideal-mixture case.

	Started May 28, 2015
	Real work on this started June 2, "
 */

#include "../helmholtz.h"
#include "../fluids.h"
#include "../fprops.h"
#include "../refstate.h"
#include "../sat.h"
#include "../zeroin.h"

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
	double rho[NFLUIDS]; /* individual densities, kg/m3 */

	/** 
		Density is found as ideal-gas density for now, as I check that the 
		property functions yield reasonable results...
	 */
	for(i=N2;i<NFLUIDS;i++){
		rho[i] = P / Ideals[i]->data->R / T;
		printf("\n\t%s%s is :  %.4f kg/m3", "The mass density of ", 
				FluidNames[i], rho[i]);
	}
	puts("");

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

		That is, since ideal-solution mixing is isobaric (constant pressure), 
		the density of each component is found by assuming it is at pressure P 
		before mixing, and solving for the density that will satisfy that 
		condition.

		For now, I use `zeroin_solve' to find the densities that satisfy 
		P(T,rho,fluid) = P

		This is not the best function to use; in fact, it is relatively unsuited 
		to this sort of problems.  We KNOW that as pressure increases density 
		does as well; any other result is physically impossible.  Therefore the 
		trend of pressure vs. density is monotonic and a single starting point 
		should suffice.
	 */
	printf("\n\n Solve for individual densities that match the given pressure:");
	typedef struct{ /* structure to provide extra data for `zeroin_solve' */
		double P;         /* the pressure, Pa */
		double T;         /* the temperature, K */
		PureFluid *Fluid; /* the fluid */
		FpropsError *err;
	} PZeroData;

	double delta_P(double rho, void *user_data){ /* function to be zeroed by `zeroin_solve' */
		PZeroData *PZ = (PZeroData *)user_data;
		return PZ->P - fprops_p((FluidState){PZ->T,rho,PZ->Fluid}, PZ->err);
	}

	double lower,upper,toler,error;
	char succeed;
	for(i=N2;i<NFLUIDS;i++){
		PZeroData PZ = {P,T,Helms[i],&err};
		lower=0.2;
		upper=2.5;
		toler=1e-9;
		
		/* succeed = */ zeroin_solve(&delta_P, &PZ, lower, upper, toler, (rho+i), &error);
		printf("\n\t%s%s is :  %.4f kg/m3,  %s :  %.2e Pa",
				"The mass density of ", FluidNames[i], rho[i], 
				"with error in pressure of", error);
	}

	/*	Now, find the individual pressures and entropies, and the average of each. 
		This is the same as  */
	p_mx=0.0; /* reset mixture pressure and enthalpy to zero */
	h_mx=0.0;
	double cp_mx=0.0, /* might as well calculate the heat capacities, etc. also */
		   cv_mx=0.0,
		   u_mx=0.0;
	double cp_i, /* single-component heat capacities, etc. */
		   cv_i,
		   u_i;
	FluidState fs_i; /* single-component FluidState; fill this only once per loop */
	for(i=N2;i<NFLUIDS;i++){
		fs_i = (FluidState){T,rho[i],Helms[i]};
		p_i  = fprops_p(fs_i, &err);
		h_i  = fprops_h(fs_i, &err);
		cp_i = fprops_cp(fs_i, &err);
		cv_i = fprops_cv(fs_i, &err);
		u_i  = fprops_u(fs_i, &err);
		printf("\n\t%s %s\n\t\t%s  %g Pa;\n\t\t%s  %g J/kg.\n",
				"For the substance", FluidNames[i],
				"the pressure is  :", p_i,
				"the enthalpy is  :", h_i);
		p_mx  += x[i] * p_i;
		h_mx  += x[i] * h_i;
		cp_mx += x[i] * cp_i;
		cv_mx += x[i] * cv_i;
		u_mx  += x[i] * u_i;
	}
	printf("\n\t%s\t\t:\t  %f kg/m3"
			"\n\t%s\t:\t  %g Pa"
			"\n\t%s\t\t:\t  %g J/kg"
			"\n\t%s\t:\t  %g J/kg"
			"\n\t%s\t:\t  %g J/kg/K"
			"\n\t%s\t:\t  %g J/kg/K\n",
			"The density of the mixture is", rho_mx,
			"The average pressure of the mixture is", p_mx,
			"The enthalpy of the mixture is", h_mx,
			"The internal energy of the mixture is", u_mx,
			"The constant-pressure heat capacity is", cp_mx,
			"The constant-volume heat capacity is", cv_mx);
	/**	
		Notice that the results from even the last simulation of this system give 
		results very close to the ideal-gas case.  We are still in the area of 
		ideal-gas behavior.
		
		The most challenging initial condition occurs when the overall solution 
		density, or individual initial densities, are provided instead of an 
		overall pressure.  (The overall density can be found from the individual 
		densities, as their sum when weighted by mass fractions.)
		
		Now we have to solve for the set of individual densities that (1) sum, 
		when weighted by their mass fractions, to the overall density; (2) all 
		yield the same pressure at the given temperature T.

		Other initial conditions (e.g. a list of individual enthalpies, 
		individual entropies, etc.) may involve similar root-finding problems

		However, the algorithm for these cases can wait until I write root-finding 
		methods for given (T,P), and test them under conditions not so nearly 
		identical to ideal-gas conditions!
	 */
	return 0;
}
