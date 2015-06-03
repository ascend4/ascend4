/*	ASCEND modelling environment 
	Copyright (C) Carnegie Mellon University 

	This program is free software; you can redistribute it and/or modify 
	it under the terms of the GNU General Public License as published by 
	the Free Software Foundation; either version 2, or (at your option) 
	any later version.

	This program is distributed in the hope that it will be useful, but 
	WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
	General Public License for more details.

	You should have received a copy of the GNU General Public License 
	along with this program; if not, write to the Free Software 
	Foundation --

	Free Software Foundation, Inc.
	59 Temple Place - Suite 330
	Boston, MA 02111-1307, USA.
*//*
	by Jacob Shealy, June 3, 2015-
	
	Initial model of a simple mixture, to get the procedure right.  This 
	is in preparation for a general algorithm to find mixing conditions 
	in the ideal-mixture case.

	TODO -
		add error handling in functions:
			check that sum of mass fractions is really one
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

/** 
	The fluids I want available for the mixture: nitrogen, ammonia, carbon 
	dioxide, methane, water
 */
extern const EosData eos_rpp_nitrogen;
extern const EosData eos_rpp_ammonia;
extern const EosData eos_rpp_carbon_dioxide;
extern const EosData eos_rpp_methane;
extern const EosData eos_rpp_water;

/* Function prototypes */
// void ig_rhos(double *rho_out, unsigned nPure, double T, double P, PureFluid **I, char **Names);
void pressure_rhos(double *rho_out, unsigned nPure, double T, double P, double tol, PureFluid **PF, char **Names, FpropsError *err);
double mixture_rho(unsigned nPure, double *x, double *rhos);
double mixture_u(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err);
double mixture_h(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err);
double mixture_cp(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err);
double mixture_cv(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err);


/* 
	Function Definitions
 */

/* 
	Calculate ideal-gas densities `rho_out' from the temperature and pressure 
	(used as starting densities for other routines)

	Specifically, these densities may be useful if T > T_c (critical 
	temperature), and P < P_c (critical pressure).
 */
void ig_rhos(double *rho_out, unsigned nPure, double T, double P, PureFluid **I, char **Names){
	unsigned i; /* counter variable */

	for(i=0;i<nPure;i++){
		rho_out[i] = P / I[i]->data->R /T;
		printf("\n\t%s%s is :  %.4f kg/m3", "The ideal-gas mass density of ",
				Names[i], rho_out[i]);
	} puts("");
}

/* 
	Calculate realistic densities `rho_out' in ideal-solution, such that 
	densities are consistent with the given temperature and pressure
 */
void pressure_rhos(double *rho_out, unsigned nPure, double T, double P, double tol, PureFluid **PF, char **Names, FpropsError *err){
	unsigned i1, i2;   /* counter variables */
	double p1, p2,     /* pressures */
		   rho1, rho2, /* densities */
		   delta_rho;  /* change in density for one step */

	for(i1=0;i1<nPure;i1++){
		rho1 = rho_out[i1];
		rho2 = 1.01 * rho1;

		for(i2=0;i2<20;i2++){
			p1 = fprops_p((FluidState){T, rho1, PF[i1]}, err);
			p2 = fprops_p((FluidState){T, rho2, PF[i1]}, err);

			if(fabs(P - p1) < tol){ /* Success! */
				rho_out[i1] = rho1;
				printf("\nRoot-finding succeeded at rho1 = %.6e kg/m3, p1 = %.6e Pa "
						"(and P = %.6e Pa), after %d iterations.", 
						rho1, p1, P, i2);
				break;
			}
			if(p1==p2){
				printf("\nWith %s, got p1 = p2 = %.6e Pa, but P = %.6e Pa!",
						Names[i1], p1, P);
			}

			/*
				unlike in sat.c, here delta_rho does not need to check bounds, 
				since we do not need to stay in the saturation region
			 */
			delta_rho = (P - p1) * (rho1 - rho2) / (p1 - p2);
			rho2 = rho1;
			p2 = p1;
			rho1 += delta_rho;
		}
	} puts("");
}

/*
	Calculate overall mass density of a mixture of components

	@param nPure number of pure components
	@param x mass fractions of components
	@param rhos mass density of each component

	@return mass density of mixture
 */
double mixture_rho(unsigned nPure, double *xs, double *rhos){
	unsigned i;
	double vol_mix=0.0;

	for(i=0;i<nPure;i++){
		vol_mix += xs[i] / rhos[i]; /* mixture volume per unit mass is the sum of each mass 
									  fraction divided by the corresponding mass density */
	}
	return 1 / vol_mix;
}

/* 
	Calculate overall ideal-solution internal energy per unit mass in a mixture 
	of pure components.

	@param nPure number of pure components
	@param xs array with mass fraction of each component
	@param rhos array with mass density of each component
	@param T temperature of mixture
	@param PFs array of pointers to PureFluid structures representing components
	@param err error argument
 */
double mixture_u(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err){
	double u_mix=0.0; /* internal energy of the mixture, iteratively summed */
	double u_n;       /* internal energy of the n[Pure]th component */

	for(;nPure>0;nPure--){ /* count *down* through `nPure' */
		u_n = fprops_u((FluidState){T,rhos[nPure],PFs[nPure]}, err);
		u_mix += xs[nPure] * u_n;
	}
	return u_mix;
}

/*
	Calculate overall ideal-solution enthalpy per unit mass in a mixture of pure 
	components.

	@param nPure number of pure components
	@param xs array with mass fraction of each component
	@param rhos array with mass density of each component
	@param T temperature of mixture
	@param PFs array of pointers to PureFluid structures representing components
	@param err error argument
 */
double mixture_h(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err){
	double h_mix=0.0; /* enthalpy of mixture, to be iteratively summed */
	double h_n;       /* the enthalpy of the n[Pure]th component */

	for(;nPure>0;nPure--){ /* count *down* through `nPure' */
		h_n = fprops_h((FluidState){T,rhos[nPure],PFs[nPure]}, err);
		h_mix += xs[nPure] * h_n;
	}
	return h_mix;
}

/* 
	Calculate overall ideal-solution constant-pressure heat capacity (per unit 
	of mass), in a mixture of pure components

	@param nPure number of pure components
	@param xs array with mass fraction of each component
	@param rhos array with mass density of each component
	@param T temperature of mixture
	@param PFs array of pointers to PureFluid structures representing components
	@param err error argument

	@return mixture heat capacity (constant-pressure)
 */
double mixture_cp(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err){
	unsigned i;
	double cp_mix=0.0;

	for(i=0;i<nPure;i++){
		cp_mix += xs[i] * fprops_cp((FluidState){T,rhos[nPure],PFs[nPure]}, err);
	}
	return cp_mix;
}

/* 
	Calculate overall ideal-solution constant-volume heat capacity (per unit of 
	mass), in a mixture of pure components

	@param nPure number of pure components
	@param xs array with mass fraction of each component
	@param rhos array with mass density of each component
	@param T temperature of mixture
	@param PFs array of pointers to PureFluid structures representing components
	@param err error argument

	@return mixture heat capacity (constant-volume)
 */
double mixture_cv(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err){
	unsigned i;
	double cv_mix=0.0;

	for(i=0;i<nPure;i++){
		cv_mix += xs[i] * fprops_cv((FluidState){T,rhos[nPure],PFs[nPure]}, err);
	}
	return cv_mix;
}


/*
	Establish mixing conditions (T,P), find individual densities, and use those 
	along with the temperature to find first-law properties for individual 
	components and the whole mixture.
	
	As a check, find pressure corresponding to temperature and component density 
	for each component -- this should equal the overall pressure
 */
int main(){
	int i; /* loop counter used throughout `main' */
	enum FluidAbbrevs {N2,NH3,CO2,/*CH4,H2O,*/NFLUIDS}; /* fluids that will be used */

	const EosData *IdealEos[]={
		&eos_rpp_nitrogen, 
		&eos_rpp_ammonia, 
		&eos_rpp_carbon_dioxide, 
		&eos_rpp_methane, 
		&eos_rpp_water
	};
	char *FluidNames[]={
		"nitrogen", "ammonia", "carbondioxide", "methane", "water"
	};
	PureFluid *Ideals[NFLUIDS];
	PureFluid *Helms[NFLUIDS];

	ReferenceState ref = {FPROPS_REF_REF0}; /* reference and error */
	FpropsError err = FPROPS_NO_ERROR;

	/**	Fill the `Ideals' and `Helms' PureFluid arrays with data for ideal and 
		helmholtz equations of state.  Ideal-gas equation of state is used for 
		comparison, although eventually I will try to set conditions far from 
		the ideal-gas case */
	for(i=N2;i<NFLUIDS;i++){
		Ideals[i] = ideal_prepare(IdealEos[i],&ref);
		Helms[i] = fprops_fluid(FluidNames[i],"helmholtz",NULL);
	}

	/* Mixing conditions (temperature,pressure), and mass fractions */
	double T=300; /* K */
	double P=1e5; /* Pa */
	double x[NFLUIDS]={0.3,0.5,0.2}; /* mass fractions */
	double rho[NFLUIDS];             /* individual densities */
		   // rho_id[NFLUIDS];          /* ideal-gas densities */

	/* Find ideal-gas density, to use as a starting point */
	ig_rhos(rho, NFLUIDS, T, P, Ideals, FluidNames);
	/*for(i=N2;i<NFLUIDS;i++){
		rho_id[i] = P / Ideals[i]->data->R / T;
		printf("\n\t%s%s is :  %.4f kg/m3", "The ideal-gas mass density of ",
				FluidNames[i], rho_id[i]);
	} puts("");*/ /* extra line for formatting purposes */

	/*
		Find actual density by searching for the individual densities which 
		each satisfy the equation P_i(T, \rho_i) = P, starting from ideal-gas 
		density.

		In production code, it might be advantageous to find several different 
		starting points for this search; e.g. if P > critical pressure, start 
		from critical density, etc.

		I take inspiration from the structure of `zeroin_solve' in how it takes 
		inputs and outputs

		Compare with:
			zeroin_solve               in  zeroin.c
			helmholtz_sat              in  helmholtz.c
			sat_p_resid, fprops_sat_p  in  sat.c (example of using zeroin_solve)
			fprops_sat_hf              in  sat.c
			fprops_solve_ph            in  solve_ph.c (uses fprops_sat_p from sat.c)
		This is partly for my own reference.
	 */
	// int ii; /* dedicated counter for iterative search */
	// double tol;
	// tol=1e-9;

	// double p1, p2,     /* pressures */
	// 	   rho1, rho2, /* densities */
	// 	   delta_rho;  /* change in density for one step */

	// for(i=N2;i<NFLUIDS;i++){
	// 	rho1 = rho_id[i];
	// 	rho2 = 1.1 * rho1;
	// 	PZeroData PZ = {P,T,Helms[i],&err};

	// 	for(ii=0; ii<20; ii++){
	// 		p1 = fprops_p((FluidState){T, rho1, Helms[i]}, &err);
	// 		p2 = fprops_p((FluidState){T, rho2, Helms[i]}, &err);

	// 		if(fabs(P - p1) < tol){ /* Success! */
	// 			rho[i] = rho1;
	// 			printf("\nRoot-finding succeeded at rho1 = %.6e kg/m3, p1 = %.6e Pa "
	// 					"(and P = %.6e Pa), after %d iterations.", 
	// 					rho1, p1, P, ii);
	// 			break;
	// 		}
	// 		if(p1==p2){
	// 			printf("\nWith %s, got p1 = p2 = %.6e Pa, but P = %.6e Pa!",
	// 					FluidNames[i], p1, P);
	// 		}

	// 		/*
	// 			unlike in sat.c, here delta_rho does not need to check bounds, 
	// 			since we do not need to stay in the saturation region
	// 		 */
	// 		delta_rho = (P - p1) * (rho1 - rho2) / (p1 - p2);
	// 		rho2 = rho1;
	// 		p2 = p1;
	// 		rho1 += delta_rho;
	// 	}
	// }
	double tol = 1e-9;
	pressure_rhos(rho, NFLUIDS, T, P, tol, Helms, FluidNames, &err);

	double rho_mx1 = mixture_rho(NFLUIDS, x, rho), /* mixture properties */
		   u_mx1   = mixture_u(NFLUIDS, x, rho, T, Helms, &err),
		   h_mx1   = mixture_h(NFLUIDS, x, rho, T, Helms, &err),
		   cp_mx1  = mixture_cp(NFLUIDS, x, rho, T, Helms, &err),
		   cv_mx1  = mixture_cv(NFLUIDS, x, rho, T, Helms, &err);
	double u_mx2, /* alternate mixture properties */
		   h_mx2,
		   cp_mx2,
		   cv_mx2;
	double p_i, /* component properties */
		   u_i,
		   h_i,
		   cp_i,
		   cv_i;
	FluidState fs_i;
	/* Find mixture properties in a loop */
	for(i=0;i<NFLUIDS;i++){
		fs_i = (FluidState){T,rho[i],Helms[i]};
		p_i  = fprops_p(fs_i, &err);
		u_i  = fprops_u(fs_i, &err);
		h_i  = fprops_h(fs_i, &err);
		cp_i = fprops_cp(fs_i, &err);
		cv_i = fprops_cv(fs_i, &err);
		printf("\n\t%s %s\n\t\t%s  %g Pa;\n\t\t%s  %g J/kg.\n",
				"For the substance", FluidNames[i],
				"the pressure is  :", p_i,
				"the enthalpy is  :", h_i);
		u_mx2  += x[i] * u_i;
		h_mx2  += x[i] * h_i;
		cp_mx2 += x[i] * cp_i;
		cv_mx2 += x[i] * cv_i;
		/* the following is used in calculating second-law properties */
		/* x_ln_x_mx += */
	}
	// rho_mx = mixture_rho(NFLUIDS, x, rho);
	printf("\n%s\n\t%s\t\t:\t  %f kg/m3"
			"\n\t%s\t\t:\t  %g J/kg"
			"\n\t%s\t:\t  %g J/kg"
			"\n\t%s\t:\t  %g J/kg/K"
			"\n\t%s\t:\t  %g J/kg/K\n",
			"  For the mixture properties calculated with functions",
			"The density of the mixture is", rho_mx1,
			"The enthalpy of the mixture is", h_mx1,
			"The internal energy of the mixture is", u_mx1,
			"The constant-pressure heat capacity is", cp_mx1,
			"The constant-volume heat capacity is", cv_mx1);
	printf("\n%s\n\t%s\t\t:\t  %g J/kg"
			"\n\t%s\t:\t  %g J/kg"
			"\n\t%s\t:\t  %g J/kg/K"
			"\n\t%s\t:\t  %g J/kg/K\n",
			"  For the mixture properties calculated directly",
			"The enthalpy of the mixture is", h_mx2,
			"The internal energy of the mixture is", u_mx2,
			"The constant-pressure heat capacity is", cp_mx2,
			"The constant-volume heat capacity is", cv_mx2);

	return 0;
}
