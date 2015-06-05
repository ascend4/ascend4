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
	by Jacob Shealy, June 4, 2015
	
	Initial model of a simple mixture, to get the procedure right.  This 
	is in preparation for a general algorithm to find mixing conditions 
	in the ideal-mixture case.

	TODO -
		add error handling in functions:
		[x]	check that sum of mass fractions is really one
			we will need to add different values to the FpropsError enum to represent errors
		how do we want inconsistent mass fractions to be handled?  Strategies:
			1. for an n-component system, only specify (n-1) mass fractions; 
			   remaining mass fraction is (1 - (sum over other mass fractions))
			2. specify all mass fractions, but check that they sum to 1 (within 
			   some tolerance), and return error otherwise.
			3. specify all mass fractions, but check that they sum to 1 (within 
			   some tolerance), and adjust value of last mass fraction if 
			   incorrect sum occurs only because of that fraction.  If incorrect 
			   sum occurs before the last mass fraction, return an error.  This 
			   strategy is very dangerous, since it involves changing 
			   user-provided data.
		Maybe add colors from `color.h' later.  Too involved for now.
 */

#include "../mixtures/init_mixfuncs.h"
#include "../helmholtz.h"
#include "../fluids.h"
#include "../fprops.h"
#include "../refstate.h"
#include "../sat.h"

#include <stdio.h>
/* #include <assert.h> */
#include <math.h>

/* Macro/Preprocessor definitions; prefixed with `MIX_' */
#define MIX_XTOL 1e-6
#define MIX_ERROR "  ERROR: "
#define MIX_XSUM_ERROR MIX_ERROR "the sum over all mass fractions, which should be exactly 1.00, is %.10f\n"

extern const EosData eos_rpp_nitrogen;
extern const EosData eos_rpp_ammonia;
extern const EosData eos_rpp_carbon_dioxide;
extern const EosData eos_rpp_methane;
extern const EosData eos_rpp_water;

/* single function header */
void solve_mixture_conditions(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, char **Names, FpropsError *err);

/*
	Calculate and print mixture properties by various means
 */
void solve_mixture_conditions(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, char **Names, FpropsError *err){
	int i; /* loop counter */
	double rho_mx1 = mixture_rho(nPure, xs, rhos), /* mixture properties */
		   u_mx1   = mixture_u(nPure, xs, rhos, T, PFs, err),
		   h_mx1   = mixture_h(nPure, xs, rhos, T, PFs, err),
		   cp_mx1  = mixture_cp(nPure, xs, rhos, T, PFs, err),
		   cv_mx1  = mixture_cv(nPure, xs, rhos, T, PFs, err);

	double x_ln_x_mx1 = mixture_x_ln_x(nPure, xs, PFs),
		   M_avg_mx1  = mixture_M_avg(nPure, xs, PFs);

	double u_mx2=0.0,  /* alternate mixture properties */
		   h_mx2=0.0,
		   cp_mx2=0.0,
		   cv_mx2=0.0;
	double p_i=0.0,    /* component properties */
		   u_i=0.0,
		   h_i=0.0,
		   cp_i=0.0,
		   cv_i=0.0;
	FluidState fs_i;
	/* Find mixture properties in a loop */
	for(i=0;i<nPure;i++){
		fs_i = (FluidState){T,rhos[i],PFs[i]};
		p_i  = fprops_p(fs_i, err);
		u_i  = fprops_u(fs_i, err);
		h_i  = fprops_h(fs_i, err);
		cp_i = fprops_cp(fs_i, err);
		cv_i = fprops_cv(fs_i, err);
		printf("\n\t%s %s"
				"\n\t\t%s\t\t:  %.3f;"
				"\n\t\t%s\t\t\t:  %.0f Pa;"
				"\n\t\t%s\t\t:  %g J/kg;"
				"\n\t\t%s\t\t\t:  %g J/kg;"
				"\n\t\t%s\t:  %g J/kg/K;"
				"\n\t\t%s\t:  %g J/kg/K.\n",
				"For the substance", Names[i],
				"the mass fraction is", xs[i],
				"the pressure is", p_i,
				"the internal energy is", u_i,
				"the enthalpy is", h_i,
				"the isobaric heat capacity is", cp_i,
				"the isometric heat capacity is", cv_i);
		u_mx2  += xs[i] * u_i;
		h_mx2  += xs[i] * h_i;
		cp_mx2 += xs[i] * cp_i;
		cv_mx2 += xs[i] * cv_i;
	}
	printf("\n  %s\n\t%s\t\t:  %f kg/m3"
			"\n\t%s\t:  %g J/kg \n\t%s\t\t:  %g J/kg"
			"\n\t%s\t:  %g J/kg/K \n\t%s\t:  %g J/kg/K\n",
			"For the mixture properties calculated with functions",
			"The density of the mixture is", rho_mx1,
			"The internal energy of the mixture is", u_mx1,
			"The enthalpy of the mixture is", h_mx1,
			"The constant-pressure heat capacity is", cp_mx1,
			"The constant-volume heat capacity is", cv_mx1);
	printf("\n  %s\n\t%s\t:  %g J/kg \n\t%s\t\t:  %g J/kg"
			"\n\t%s\t:  %g J/kg/K \n\t%s\t:  %g J/kg/K\n",
			"For the mixture properties calculated directly",
			"The internal energy of the mixture is", u_mx2,
			"The enthalpy of the mixture is", h_mx2,
			"The constant-pressure heat capacity is", cp_mx2,
			"The constant-volume heat capacity is", cv_mx2);
	printf("\n  %s:\n\t%s %s %s,\n\t%s %s %s,\n\t%s %s %s,\n\t%s %s %s.\n",
			"For the mixture properties calculated in different ways",
			"The internal energies", (u_mx1==u_mx2 ? "are" : "are NOT"), "equal",
			"The enthalpies", (h_mx1==h_mx2 ? "are" : "are NOT"), "equal",
			"The constant-pressure heat capacities",
			(cp_mx1==cp_mx2 ? "are" : "are NOT"), "equal",
			"The constant-volume heat capacities",
			(cv_mx1==cv_mx2 ? "are" : "are NOT"), "equal");
	printf("\n  %s:\n\t%s\t:  %.5f\n\t%s\t:  %.5f kg/kmol.\n",
			"The second-law properties of the solution are",
			"sum of (x ln(x))", x_ln_x_mx1, "average molar mass", M_avg_mx1);
} /* end of `solve_mixture_conditions' */

/*
	Establish mixing conditions (T,P), find individual densities, and use those 
	along with the temperature to find first-law properties for individual 
	components and the whole mixture.
	
	As a check, find pressure corresponding to temperature and component density 
	for each component -- this should equal the overall pressure

	The fluids I use in the mixture are nitrogen, ammonia, carbon dioxide, and 
	methane.

	I may add other substances later, such as methyl chloride, carbon monoxide, 
	nitrous oxide, and hydrogen sulfide.
 */
int main(){
	enum FluidAbbrevs {N2,NH3,CO2,CH4,/* H2O, */NFLUIDS}; /* fluids that will be used */

	char *FluidNames[]={
		"nitrogen", "ammonia", "carbondioxide", "methane", "water"
	};
	PureFluid *Helms[NFLUIDS];
	FpropsError err = FPROPS_NO_ERROR;

	/*	
		Fill the `Helms' PureFluid array with data from the helmholtz equation 
		of state.
	 */
	int i;
	for(i=N2;i<NFLUIDS;i++){
		Helms[i] = fprops_fluid(FluidNames[i],"helmholtz",NULL);
	}

	/* Mixing conditions (temperature,pressure), and mass fractions */
	double T=250;        /* K */
	double P=1.5e5;      /* Pa */
	double rho[NFLUIDS]; /* individual densities */
	double x[NFLUIDS];   /* mass fractions */

	double props[] = {1, 3, 2, 1.5, 2.5}; /* proportions of mass fractions */
	mixture_x_props(NFLUIDS, x, props);

	/* Find ideal-gas density, to use as a starting point */
	initial_rhos(rho, NFLUIDS, T, P, Helms, FluidNames, &err);

	double tol = 1e-9;
	pressure_rhos(rho, NFLUIDS, T, P, tol, Helms, FluidNames, &err);

	int usr_cont;
	char temp_str[100];
	printf("\n  %s\n\t%s\n\t%s",
			"Continue to calculate and print solution properties?",
			"0 - No", "1 - Yes");
	do{
		printf("\n    %s ", "Choice?");
		fgets(temp_str, 100, stdin);
	}while((1 != sscanf(temp_str, "%i", &usr_cont) || (0>usr_cont || 1<usr_cont)) 
			&& printf("\n  %s", "You must enter either 0 or 1"));
	if(1==usr_cont){
		solve_mixture_conditions(NFLUIDS, x, rho, T, Helms, FluidNames, &err);
	}

	return 0;
} /* end of `main' */

