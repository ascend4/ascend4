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
	by Jacob Shealy, June 3, 2015
	
	Initial model of a simple mixture, to get the procedure right.  This 
	is in preparation for a general algorithm to find mixing conditions 
	in the ideal-mixture case.

	TODO -
		add error handling in functions:
			check that sum of mass fractions is really one
 */

#include "../mixtures/init_mixfuncs.h"
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

	/*	
		Fill the `Ideals' and `Helms' PureFluid arrays with data for ideal and 
		helmholtz equations of state.  Ideal-gas equation of state is used for 
		comparison, although eventually I will try to set conditions far from 
		the ideal-gas case
	 */
	for(i=N2;i<NFLUIDS;i++){
		Ideals[i] = ideal_prepare(IdealEos[i],&ref);
		Helms[i] = fprops_fluid(FluidNames[i],"helmholtz",NULL);
	}

	/* Mixing conditions (temperature,pressure), and mass fractions */
	double T=300; /* K */
	double P=1e5; /* Pa */
	double x[NFLUIDS]={0.3,0.5,0.2}; /* mass fractions */
	double rho[NFLUIDS];             /* individual densities */

	/* Find ideal-gas density, to use as a starting point */
	ig_rhos(rho, NFLUIDS, T, P, Ideals, FluidNames);

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
	double tol = 1e-9;
	pressure_rhos(rho, NFLUIDS, T, P, tol, Helms, FluidNames, &err);

	double rho_mx1 = mixture_rho(NFLUIDS, x, rho), /* mixture properties */
		   u_mx1   = mixture_u(NFLUIDS, x, rho, T, Helms, &err),
		   h_mx1   = mixture_h(NFLUIDS, x, rho, T, Helms, &err),
		   cp_mx1  = mixture_cp(NFLUIDS, x, rho, T, Helms, &err),
		   cv_mx1  = mixture_cv(NFLUIDS, x, rho, T, Helms, &err);

	double x_ln_x_mx1 = mixture_x_ln_x(NFLUIDS, x, Helms),
		   M_avg_mx1  = mixture_M_avg(NFLUIDS, x, Helms);

	double u_mx2=0.0, /* alternate mixture properties */
		   h_mx2=0.0,
		   cp_mx2=0.0,
		   cv_mx2=0.0;
	double p_i=0.0, /* component properties */
		   u_i=0.0,
		   h_i=0.0,
		   cp_i=0.0,
		   cv_i=0.0;
	FluidState fs_i;
	/* Find mixture properties in a loop */
	for(i=0;i<NFLUIDS;i++){
		fs_i = (FluidState){T,rho[i],Helms[i]};
		p_i  = fprops_p(fs_i, &err);
		u_i  = fprops_u(fs_i, &err);
		h_i  = fprops_h(fs_i, &err);
		cp_i = fprops_cp(fs_i, &err);
		cv_i = fprops_cv(fs_i, &err);
		printf("\n\t%s %s"
				"\n\t\t%s\t\t:  %.3f;"
				"\n\t\t%s\t\t\t:  %.0f Pa;"
				"\n\t\t%s\t\t:  %g J/kg;"
				"\n\t\t%s\t\t\t:  %g J/kg;"
				"\n\t\t%s\t:  %g J/kg/K;"
				"\n\t\t%s\t:  %g J/kg/K.\n",
				"For the substance", FluidNames[i],
				"the mass fraction is", x[i],
				"the pressure is", p_i,
				"the internal energy is", u_i,
				"the enthalpy is", h_i,
				"the isobaric heat capacity is", cp_i,
				"the isometric heat capacity is", cv_i);
		u_mx2  += x[i] * u_i;
		h_mx2  += x[i] * h_i;
		cp_mx2 += x[i] * cp_i;
		cv_mx2 += x[i] * cv_i;
	}
	printf("\n  %s\n\t%s\t\t:\t  %f kg/m3"
			"\n\t%s\t\t:\t  %g J/kg"
			"\n\t%s\t:\t  %g J/kg"
			"\n\t%s\t:\t  %g J/kg/K"
			"\n\t%s\t:\t  %g J/kg/K\n",
			"For the mixture properties calculated with functions",
			"The density of the mixture is", rho_mx1,
			"The internal energy of the mixture is", u_mx1,
			"The enthalpy of the mixture is", h_mx1,
			"The constant-pressure heat capacity is", cp_mx1,
			"The constant-volume heat capacity is", cv_mx1);
	printf("\n  %s\n\t%s\t\t:\t  %g J/kg"
			"\n\t%s\t:\t  %g J/kg"
			"\n\t%s\t:\t  %g J/kg/K"
			"\n\t%s\t:\t  %g J/kg/K\n",
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

	return 0;
}
