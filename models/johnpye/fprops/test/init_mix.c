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
void solve_mixture_conditions(unsigned n_pure, double *xs, double *rhos, double T, PureFluid **PFs, char **Names, FpropsError *err);
void print_mixture_properties(char *how_calc, double rho, double u, double h, double cp, double cv, double s, double g, double a);
void print_row(unsigned columns, char *label, char *format, double *contents);
void print_substances_properties(unsigned subst, char **headers, double *xs, double *rhos, double *ps, double *us, double *hs, double *cps, double *cvs, double *ss, double *gs, double *as);
void print_cases_properties(unsigned cases, char **headers, double *rhos, double *ps, double *us, double *hs, double *cps, double *cvs, double *ss, double *gs, double *as);

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
	Print a row of a table
 */
void print_row(unsigned columns, char *label, char *format, double *contents){
	unsigned i;

	printf("\n %s  ", label);
	for(i=0;i<columns;i++){
		printf(format, contents[i]);
	}
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
	/* unsigned i;

	printf("\n %s\t\t  ", "CASES");
	for(i=0;i<cases;i++){
		printf("%s\t  ", headers[i]);
	}
	print_row(cases, "DENSITY (kg/m3)", "%6f\t  ", rhos);
	print_row(cases,"PRESSURE (Pa)\t"                               ,"%9.1f\t  "   ,ps);
	print_row(cases,"INTERNAL\n ENERGY (J/kg)\t"                    ,"% .6g  \t  " ,us);
	print_row(cases,"ENTHALPY (J/kg)"                               ,"% .6g  \t  " ,hs);
	print_row(cases,"CONSTANT-PRESSURE HEAT CAPACITY\n (J/kg/K)\t\t","% .6g  \t  " ,cps);
	print_row(cases,"CONSTANT-VOLUME HEAT CAPACITY"                 ,"% .6g  \t  " ,cvs);
	print_row(cases,"ENTROPY (J/kg/K)"                              ,"% .6g  \t   ",ss);
	print_row(cases,"GIBBS\n ENERGY (J/kg)"                         ,"% .6g  \t  " ,gs);
	print_row(cases,"HELMHOLTZ\n ENERGY (J/kg)"                     ,"% .6g  \t  " ,as); */
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

	/* printf("\n  prepared all contributing arrays for cases"); */
	PREPARE_TABLE(TBL_ROWS,cases+1,headers,sides,vals,forms,cont);
	/* printf("\n  prepared string array for cases"); */
	PRINT_STR_TABLE(TBL_ROWS,cases+1,col_width,cont);

#undef TBL_ROWS
}

/*
	Calculate and print mixture properties by various means
 */
void solve_mixture_conditions(unsigned n_pure, double *xs, double *rhos, double T, PureFluid **PFs, char **Names, FpropsError *err){
	int i; /* loop counter */
	double rho_mx[] = {mixture_rho(n_pure, xs, rhos), 0.0}, /* mixture properties */
		   p_mx[] = {
			   fprops_p((FluidState){T,rhos[0],PFs[0]}, err),
			   fprops_p((FluidState){T,rhos[1],PFs[1]}, err)
		   },
		   u_mx[]  = {mixture_u(n_pure, xs, rhos, T, PFs, err), 0.0},
		   h_mx[]  = {mixture_h(n_pure, xs, rhos, T, PFs, err), 0.0},
		   cp_mx[] = {mixture_cp(n_pure, xs, rhos, T, PFs, err), 0.0},
		   cv_mx[] = {mixture_cv(n_pure, xs, rhos, T, PFs, err), 0.0},
		   s_mx[]  = {
			   mixture_s(n_pure, xs, rhos, T, PFs, err),
			   -(PFs[0]->data->R * PFs[0]->data->M) * mixture_x_ln_x(n_pure, xs, PFs)
		   },
		   g_mx[] = {
			   mixture_g(n_pure, xs, rhos, T, PFs, err),
			   (PFs[0]->data->R * T * PFs[0]->data->M) * mixture_x_ln_x(n_pure, xs, PFs)
		   },
		   a_mx[] = {
			   mixture_a(n_pure, xs, rhos, T, PFs, err),
			   (PFs[0]->data->R * T * PFs[0]->data->M) * mixture_x_ln_x(n_pure, xs, PFs)
		   };
	char *calc_types[]={
		"Calculate by Functions", "Calculate Directly"
	};

	double ps[n_pure],  /* component properties */
		   us[n_pure],
		   hs[n_pure],
		   cps[n_pure],
		   cvs[n_pure],
		   ss[n_pure],
		   gs[n_pure],
		   as[n_pure];
	FluidState fs_i;
	/* Find mixture properties in a loop */
	for(i=0;i<n_pure;i++){
		fs_i   = (FluidState){T,rhos[i],PFs[i]};
		ps[i]  = fprops_p(fs_i, err);
		us[i]  = fprops_u(fs_i, err);
		hs[i]  = fprops_h(fs_i, err);
		cps[i] = fprops_cp(fs_i, err);
		cvs[i] = fprops_cv(fs_i, err);
		ss[i]  = fprops_s(fs_i, err);
		gs[i]  = fprops_g(fs_i, err);
		as[i]  = fprops_a(fs_i, err);
		u_mx[1]  += xs[i] * us[i];
		h_mx[1]  += xs[i] * hs[i];
		cp_mx[1] += xs[i] * cps[i];
		cv_mx[1] += xs[i] * cvs[i];
		s_mx[1]  += xs[i] * ss[i];
		g_mx[1]  += xs[i] * gs[i];
		a_mx[1]  += xs[i] * as[i];
	}
	print_substances_properties(n_pure, Names, xs, rhos, ps, us, hs, cps, cvs, ss, gs, as);
	print_cases_properties(2, calc_types, rho_mx, p_mx, u_mx, h_mx, cp_mx, cv_mx, s_mx, g_mx, a_mx);
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

