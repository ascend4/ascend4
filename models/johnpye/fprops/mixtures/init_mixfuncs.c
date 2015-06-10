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
	by Jacob Shealy, June 5-, 2015

	Function definitions for initial model of ideal-solution mixing.  Removed 
	these from the test files init_mix1, init_mix2, etc. to de-clutter them.
 */

#include "init_mixfuncs.h"
#include "../helmholtz.h"
#include "../fluids.h"
#include "../fprops.h"
#include "../refstate.h"
#include "../sat.h"

#include <stdio.h>
#include <math.h>

/* Mixture-Preparation Functions */
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
		from critical density, etc. [NOTE: this is done in initial_rhos, above]

		Compare with:
			zeroin_solve               in  zeroin.c
			helmholtz_sat              in  helmholtz.c
			sat_p_resid, fprops_sat_p  in  sat.c (example of using zeroin_solve)
			fprops_sat_hf              in  sat.c
			fprops_solve_ph            in  solve_ph.c (uses fprops_sat_p from sat.c)
			densities_to_mixture       in  init_mix.c or this file
			densities_Ts_to_mixture    in  init_mix.c or this file
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
				printf("\n\n\tRoot-finding for substance %s SUCCEEDED after %d iterations;\n"
						"\t  at rho1=%.5f kg/m3, p1=%.0f Pa (and P=%.0f Pa).", 
						Names[i1], i2, rho1, p1, P);
				break;
			}
			if(p1==p2){
				printf("\n\n\tRoot-finding for substance %s FAILED after %d iterations;\n"
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

/* Mixture-Property Functions */
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

	@return ideal-solution internal energy
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

	@return ideal-solution enthalpy
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

	@return ideal-solution heat capacity (constant-pressure)
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

	@return ideal-solution heat capacity (constant-volume)
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
double mixture_s(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err){
	unsigned i;
	double x_total=0.0, /* sum over all mass fractions -- to check consistency */
		   s_mix=0.0;   /* entropy of mixture, iteratively summed */
	double R = PFs[0]->data->R * PFs[0]->data->M; /* ideal gas constant */

	for(i=0;i<nPure;i++){
		s_mix += xs[i] * fprops_s((FluidState){T,rhos[i],PFs[i]}, err);
		x_total += xs[i];
	}
	if(fabs(x_total - 1) > MIX_XTOL){
		printf(MIX_XSUM_ERROR, x_total);
	}
	return s_mix - (R * mixture_x_ln_x(nPure,xs,PFs));
}

/*
	Calculate overall ideal-solution Gibbs energy per unit mass in a mixture
 */
double mixture_g(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err){
	unsigned i;
	double x_total=0.0, /* sum over all mass fractions -- to check consistency */
		   g_mix=0.0;   /* entropy of mixture, iteratively summed */
	double R = PFs[0]->data->R * PFs[0]->data->M; /* ideal gas constant */

	for(i=0;i<nPure;i++){
		g_mix += xs[i] * fprops_g((FluidState){T,rhos[i],PFs[i]}, err);
		x_total += xs[i];
	}
	if(fabs(x_total - 1) > MIX_XTOL){
		printf(MIX_XSUM_ERROR, x_total);
	}
	return g_mix + (R * T * mixture_x_ln_x(nPure,xs,PFs));
}

/*
	Calculate overall ideal-solution Helmholtz energy per unit mass in a mixture
 */
double mixture_a(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err){
	unsigned i;
	double x_total=0.0, /* sum over all mass fractions -- to check consistency */
		   a_mix=0.0;   /* entropy of mixture, iteratively summed */
	double R = PFs[0]->data->R * PFs[0]->data->M; /* ideal gas constant */

	for(i=0;i<nPure;i++){
		a_mix += xs[i] * fprops_a((FluidState){T,rhos[i],PFs[i]}, err);
		x_total += xs[i];
	}
	if(fabs(x_total - 1) > MIX_XTOL){
		printf(MIX_XSUM_ERROR, x_total);
	}
	return a_mix + (R * T * mixture_x_ln_x(nPure,xs,PFs));
}

/* Mixture-Display Functions */
/*
	Print properties of a mixture, with correct formatting
 */
void print_mixture_properties(char *how_calc, double rho, double u, double h, double cp, double cv, double s, double g, double a){
	printf("\n  %s %s"
			"\n\t%s is\t\t:  %.6f kg/m3"
			"\n\t%s is\t:  %g J/kg"
			"\n\t%s is\t\t:  %g J/kg"
			"\n\t%s is\t:  %g J/kg/K"
			"\n\t%s is\t:  %g J/kg/K"
			"\n\t%s is\t\t:  %g J/kg/K"
			"\n\t%s is\t:  %g J/kg"
			"\n\t%s is\t:  %g J/kg\n",
			"For the mixture properties calculated", how_calc,
			"The density of the mixture", rho,
			"The internal energy of the mixture", u,
			"The enthalpy of the mixture", h,
			"The constant-pressure heat capacity", cp,
			"The constant-volume heat capacity", cv,
			"The entropy of the mixture", s,
			"The Gibbs energy of the mixture", g,
			"The Helmholtz energy of the mixture", a);
}

/*
	Print table of properties for different substances
 */
void print_substances_properties(const unsigned subst, char **headers, double *xs, double *rhos, double *ps, double *us, double *hs, double *cps, double *cvs, double *ss, double *gs, double *as){
#define TBL_ROWS 11

	unsigned i1,i2,i3;
	unsigned col_width[20]={0};

	double *vals[TBL_ROWS-1]={
		xs, rhos, ps, us, hs, cps, cvs, ss, gs, as
	};

	char *forms[TBL_ROWS-1]={
		"% .6f", "% .6f", "%9.1f", "% .6g", "% .6g", "% .6g",
		"% .6g", "% .6g", "% .6g", "% .6g"
	};
	char *sides[TBL_ROWS]={
		"SUBSTANCES",
		"MASS FRACTION",
		"DENSITY (kg/m3)",
		"PRESSURE (Pa)",
		"INTERNAL ENERGY (J/kg)",
		"ENTHALPY (J/kg)",
		"C_P (J/kg/K)",
		"C_V (J/kg/K)",
		"ENTROPY (J/kg/K)",
		"GIBBS ENERGY (J/kg)",
		"HELMHOLTZ ENERGY (J/kg)"
	};
	char *cont[TBL_ROWS][subst+1];

	PREPARE_TABLE(TBL_ROWS,subst+1,headers,sides,vals,forms,cont);
	PRINT_STR_TABLE(TBL_ROWS,subst+1,col_width,cont);

#undef TBL_ROWS
}

/*
	Print table of properties for different cases
 */
void print_cases_properties(unsigned cases, char **headers, double *rhos, double *ps, double *us, double *hs, double *cps, double *cvs, double *ss, double *gs, double *as){
#define TBL_ROWS 10

	unsigned i1,i2,i3;
	unsigned col_width[20]={0};

	double *vals[TBL_ROWS-1]={
		rhos, ps, us, hs, cps, cvs, ss, gs, as
	};

	char *forms[TBL_ROWS-1]={
		"% .6f", "%9.1f", "% .6g", "% .6g", "% .6g", 
		"% .6g", "% .6g", "% .6g", "% .6g"
	};
	char *sides[TBL_ROWS]={
		"CASES",
		"DENSITY (kg/m3)",
		"PRESSURE (Pa)",
		"INTERNAL ENERGY (J/kg)",
		"ENTHALPY (J/kg)",
		"C_P (J/kg/K)",
		"C_V (J/kg/K)",
		"ENTROPY (J/kg/K)",
		"GIBBS ENERGY (J/kg)",
		"HELMHOLTZ ENERGY (J/kg)"                     
	};
	char *cont[TBL_ROWS][cases+1];

	PREPARE_TABLE(TBL_ROWS,cases+1,headers,sides,vals,forms,cont);
	PRINT_STR_TABLE(TBL_ROWS,cases+1,col_width,cont);

#undef TBL_ROWS
}

