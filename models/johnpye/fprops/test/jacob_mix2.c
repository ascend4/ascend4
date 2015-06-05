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
 */

#include "../helmholtz.h"
#include "../fluids.h"
#include "../fprops.h"
#include "../refstate.h"
#include "../sat.h"
/* #include "../zeroin.h" */

#include <stdio.h>
#include <assert.h>
#include <math.h>

/* Macro/Preprocessor definitions; prefixed with `MIX_' */
#define MIX_XTOL 1e-6
#define MIX_ERROR "  ERROR: "
#define MIX_XSUM_ERROR MIX_ERROR "the sum over all mass fractions, which should be exactly 1.00, is %.10f\n"

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
void mixture_x_props(unsigned nPure, double *xs, double *props);
double mixture_x_fill_in(unsigned nPure, double *xs);
void ig_rhos(double *rho_out, unsigned nPure, double T, double P, PureFluid **I, char **Names);
void initial_rhos(double *rho_out, unsigned nPure, double T, double P, PureFluid **PFs, char **names, FpropsError *err);
void pressure_rhos(double *rho_out, unsigned nPure, double T, double P, double tol, PureFluid **PF, char **Names, FpropsError *err);
double mixture_rho(unsigned nPure, double *x, double *rhos);
double mixture_u(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err);
double mixture_h(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err);
double mixture_cp(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err);
double mixture_cv(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err);
double mixture_x_ln_x(unsigned nPure, double *xs, PureFluid **PFs);
double mixture_M_avg(unsigned nPure, double *xs, PureFluid **PFs);
double mixture_s(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err);
void solve_mixture_conditions(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, char **Names, FpropsError *err);


/* 
	Function Definitions
 */

/*	
	Calculate mass fractions from an array of numbers, with each mass fraction 
	sized proportionally to its corresponding number
 */
void mixture_x_props(unsigned nPure, double *xs, double *props){
	unsigned i;
	double x_total=0.0; /* sum of proportions */

	for(i=0;i<nPure;i++){
		x_total += props[i]; /* find sum of proportions */
	}
	/*	
		Each mass fraction is its corresponding proportion, divided by the sum 
		over all proportions.
	 */
	for(i=0;i<nPure;i++){
		xs[i] = props[i] / x_total; 
	}
}

/*
	Calculate last of (n) mass fractions given an array of (n-1) mass 
	fractions, such that the sum over all mass fractions will equal one.
 */
double mixture_x_fill_in(unsigned nPure, double *xs){
	unsigned i;
	double x_total;

	for(i=0;i<(nPure-1);i++){ /* sum only for nPure-1 loops */
		x_total += xs[i];
	}
	if(x_total>0){
		printf(MIX_ERROR "%.6f.", x_total);
	}
	return 1-x_total;
}

/* 
	Calculate ideal-gas densities `rho_out' from the temperature and pressure 
	(used as starting densities for other routines)

	Specifically, these densities may be useful if T > T_c (critical 
	temperature), and P < P_c (sub-critical pressure).
 */
void ig_rhos(double *rho_out, unsigned nPure, double T, double P, PureFluid **I, char **Names){
	unsigned i; /* counter variable */

	for(i=0;i<nPure;i++){
		rho_out[i] = P / I[i]->data->R / T;
		printf("\n\t%s%s is :  %.4f kg/m3", "The ideal-gas mass density of ",
				Names[i], rho_out[i]);
	} puts("");
}

/*
	Set initial densities for use in `pressure_rhos', given an initial 
	temperature and pressure.  Considers whether a substance is in critical or 
	saturation regions.
 */
void initial_rhos(double *rho_out, unsigned nPure, double T, double P, PureFluid **PFs, char **Names, FpropsError *err){
	unsigned i;
	enum Region_Enum {SUPERCRIT, GASEOUS, LIQUID, VAPOR, SAT_VLE} Region;
	char *region_names[] = {
		"super-critical",
		"gaseous",
		"liquid",
		"vapor",
		"vapor-liquid equilibrium (saturation)"
	};

#define D PFs[i]->data
	for(i=0;i<nPure;i++){
		if(T >= D->T_c){ /* temperature >= critical temperature */
			if(P >= D->p_c){       /* pressure >= critical pressure */
				rho_out[i] = D->rho_c;     /* super-critical fluid */
				Region = SUPERCRIT;
			}else{                 /* true gas (as opposed to sub-critical vapor) */
				rho_out[i] = P / D->R / T; /* density from ideal gas */
				Region = GASEOUS;
			}
		}else{
			if(P >= D->p_c){ /* pressure >= critical pressure -- liquid */
				rho_out[i] = fprops_rhof_T_rackett(T, D); /* density from saturation liquid */
				Region = LIQUID;
			}else{ /* now we're getting into hard conditions */
				double T_sat,   /* find saturation temperature, densities */
					   rho_liq,
					   rho_vap;
				fprops_sat_p(P, &T_sat, &rho_liq, &rho_vap, PFs[i], err);

				if(T > T_sat){ /* this is the last condition that we can precisely find */
					rho_out[i] = rho_vap; /* vapor phase */
					Region = VAPOR;
				}else{
					rho_out[i] = (rho_liq + rho_vap) / 2; /* both liquid and vapor */
					/*	
						We are in vapor-liquid equilibrium, and the density 
						cannot be determined without knowing how much of the 
						substance is in the vapor and how much in the liquid.
					 */
					/*printf("\n\tThe substance %s is in vapor-liquid equilibrium; "
							"it has been assigned a provisional density %.5f kg/m3.",
							Names[i], rho_out[i]);*/
					Region = SAT_VLE;
				}
			}
		}
		printf("\n\tThe substance %s was assigned a density of %.5f kg/m3;\n"
				"\t  it is in the %s region, since\n"
				"\t\tCritical temperature T_c=%.2f K \tand current temperature T=%.2f K;\n"
				"\t\tCritical pressure    P_c=%.0f Pa\tand current pressure    P=%.0f Pa.",
				Names[i], rho_out[i], region_names[Region], D->T_c, T, D->p_c, P);
	}
#undef D

}

/* 
	Calculate realistic densities `rho_out' in ideal-solution, such that 
	densities are consistent with the given temperature and pressure
 */
void pressure_rhos(double *rho_out, unsigned nPure, double T, double P, double tol, PureFluid **PF, char **Names, FpropsError *err){
	/*
		Find actual density by searching for the individual densities which 
		each satisfy the equation P_i(T, \rho_i) = P, starting from ideal-gas 
		density.

		In production code, it might be advantageous to find several different 
		starting points for this search; e.g. if P > critical pressure, start 
		from critical density, etc.

		Compare with:
			zeroin_solve               in  zeroin.c
			helmholtz_sat              in  helmholtz.c
			sat_p_resid, fprops_sat_p  in  sat.c (example of using zeroin_solve)
			fprops_sat_hf              in  sat.c
			fprops_solve_ph            in  solve_ph.c (uses fprops_sat_p from sat.c)
	 */
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
				printf("\n\tRoot-finding succeeded for substance %s after %d iterations;\n"
						"\t  at rho1=%.5f kg/m3, p1=%.0f Pa (and P=%.0f Pa).", 
						Names[i1], i2, rho1, p1, P);
				break;
			}
			if(p1==p2){
				printf("\n\tRoot-finding FAILED for substance %s after %d iterations;\n"
						"\t  at rho1=%.5f kg/m3, rho2=%.5f, got p1=p2=%.6e Pa, but P = %.6e Pa!",
						Names[i1], i2, rho1, rho2, p1, P);
				break;
			}

			/*
				Unlike in sat.c, here delta_rho does not need to check bounds, 
				since we do not need to stay in the saturation region.

				However, I will eventually need to provide for correct handling 
				of saturation conditions.
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
	double x_total=0.0; /* sum over all mass fractions -- to check consistency */
	double vol_mix=0.0; /* volume per unit mass of the mixture, iteratively summed */

	for(i=0;i<nPure;i++){
		vol_mix += xs[i] / rhos[i];
		x_total += xs[i];
	}
	if(fabs(x_total - 1) > MIX_XTOL){
		printf(MIX_XSUM_ERROR, x_total);
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
	unsigned i;
	double x_total=0.0; /* sum over all mass fractions -- to check consistency */
	double u_mix=0.0;   /* internal energy of the mixture, iteratively summed */

	for(i=0;i<nPure;i++){
		u_mix += xs[i] * fprops_u((FluidState){T,rhos[i],PFs[i]}, err);
		x_total += xs[i];
	}
	if(fabs(x_total - 1) > MIX_XTOL){
		printf(MIX_XSUM_ERROR, x_total);
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
	unsigned i;
	double x_total=0.0; /* sum over all mass fractions -- to check consistency */
	double h_mix=0.0;   /* enthalpy of mixture, iteratively summed */

	for(i=0;i<nPure;i++){
		h_mix += xs[i] * fprops_h((FluidState){T,rhos[i],PFs[i]}, err);
		x_total += xs[i];
	}
	if(fabs(x_total - 1) > MIX_XTOL){
		printf(MIX_XSUM_ERROR, x_total);
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
	double x_total=0.0; /* sum over all mass fractions -- to check consistency */
	double cp_mix=0.0;  /* constant-pressure heat capacity of mixture, iteratively summed */

	for(i=0;i<nPure;i++){
		cp_mix += xs[i] * fprops_cp((FluidState){T,rhos[i],PFs[i]}, err);
		x_total += xs[i];
	}
	if(fabs(x_total - 1) > MIX_XTOL){
		printf(MIX_XSUM_ERROR, x_total);
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
	double x_total=0.0; /* sum over all mass fractions -- to check consistency */
	double cv_mix=0.0;  /* constant-volume heat capacity of mixture, iteratively summed */

	for(i=0;i<nPure;i++){
		cv_mix += xs[i] * fprops_cv((FluidState){T,rhos[i],PFs[i]}, err);
		x_total += xs[i];
	}
	if(fabs(x_total - 1) > MIX_XTOL){
		printf(MIX_XSUM_ERROR, x_total);
	}
	return cv_mix;
}

/*
	Calculate the value of the sum over all *mole* fractions x_i, of the mole 
	fraction times the natural logarithm of the mole fraction:
		\sum\limits_i x_i \ln(x_i)

	This quantity is used in calculating second-law mixture properties for ideal 
	solutions

	@param nPure number of pure components
	@param xs array with mass fraction of each component
	@param PFs array of pointers to PureFluid structures representing components

	@return sum over all components of mole fraction times natural logarithm of 
	mole fraction.
 */
double mixture_x_ln_x(unsigned nPure, double *x_mass, PureFluid **PFs){
	unsigned i;
	double x_total=0.0, /* sum over all mass fractions -- to check consistency */
		   x_mole,      /* mole fraction of current component in the loop */
		   rM_avg=0.0,  /* reciprocal average molar mass */
		   x_ln_x=0.0;  /* sum of (x_i * ln(x_i)) over all `i' */

	for(i=0;i<nPure;i++){ /* find the reciprocal average molar mass */
		/* add mass fraction over molar mass */
		rM_avg += x_mass[i] / PFs[i]->data->M;
		x_total += x_mass[i];
	}
	/* return error if sum of mole fractions is not 1.00 */
	if(fabs(x_total - 1) > MIX_XTOL){
		printf(MIX_XSUM_ERROR, x_total);
	}

	for(i=0;i<nPure;i++){ /* Find the summation we came for */
		x_mole  = (x_mass[i] / PFs[i]->data->M) * rM_avg;
		x_ln_x += x_mass[i] / PFs[i]->data->M * log(x_mole);
	}
	return x_ln_x;
}

/* 
	Calculate the average molar mass of the solution.  This is useful in 
	converting mass-specific quantities (e.g. enthalpy in J/kg) into molar 
	quantities (e.g. enthalpy in J/kmol).  The molar masses provided by 
	PureFluid structs in FPROPS have units of kg/kmol, so this molar mass will 
	have the same units.

	@param nPure number of pure components
	@param xs array with mass fraction of each component
	@param PFs array of pointers to PureFluid structures representing components

	@return average molar mass of the solution
 */
double mixture_M_avg(unsigned nPure, double *x_mass, PureFluid **PFs){
	unsigned i;
	double x_total=0.0; /* sum over all mass fractions -- to check consistency */
	double rM_avg=0.0;  /* reciprocal average molar mass */

	for(i=0;i<nPure;i++){
		rM_avg += x_mass[i] / PFs[i]->data->M;
		x_total += x_mass[i];
	}
	if(fabs(x_total - 1) > MIX_XTOL){
		printf(MIX_XSUM_ERROR, x_total);
	}
	return 1. / rM_avg;
} /* end of `mixture_M_avg' */

/*
	Calculate the overall ideal-solution entropy per unit mass in a mixture of 
	pure components
 */

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

