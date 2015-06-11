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
	by Jacob Shealy, June 4-9, 2015
	
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
#include "../pengrob.h"
#include "../fluids.h"
#include "../fprops.h"
#include "../refstate.h"
#include "../sat.h"

#include <stdio.h>
#include <math.h>

/* Macro/Preprocessor definitions; prefixed with `MIX_' */
#define NSIMS 5

extern const EosData eos_rpp_nitrogen;
extern const EosData eos_rpp_ammonia;
extern const EosData eos_rpp_carbon_dioxide;
extern const EosData eos_rpp_methane;
extern const EosData eos_rpp_water;

/* Function prototypes */
void solve_mixture_conditions(unsigned n_sims, double *Ts, double *Ps, MixtureSpec *M, char **Names, FpropsError *err);
void densities_Ts_to_mixture(MixtureState *MS, double *P_out, double *T_in, double tol, char **Names, FpropsError *err);


/*
	Calculate and print mixture properties, given several temperatures and 
	densities
 */
void solve_mixture_conditions(unsigned n_sims, double *Ts, double *Ps, MixtureSpec *M, char **Names, FpropsError *err){
	int i;                       /* loop counter */
	double rhos[n_sims][M->pures]; /* individual densities */
	double rho_mix[n_sims],
		   u_mix[n_sims],
		   h_mix[n_sims],
		   cp_mix[n_sims],
		   cv_mix[n_sims],
		   s_mix[n_sims],
		   g_mix[n_sims],
		   a_mix[n_sims];

	int usr_cont=1;
	double tol = 1e-9;
	char temp_str[100];
	char *headers[n_sims];

#if 0
#define MIXTURE_CALC(PROP) mixture_##PROP(n_pure, xs, rhos[i], Ts[i], PFs, err)
#define MIXTURE_CALC(PROP) mixture_##PROP(M, Ts[i] err)
#else
#define MIXTURE_CALC(PROP) mixture_##PROP(&MS, err)
#endif
	for(i=0;i<n_sims;i++){
		/*	
			For each set of conditions simulated, find initial densities to use as a 
			starting point, and then find more exact densities to use
		 */
		/* initial_rhos(rhos[i], n_pure, Ts[i], Ps[i], PFs, Names, err); */
		MixtureState MS = {
			Ts[i],
			rhos[i],
			M
		};
		initial_rhos(&MS, Ps[i], Names, err);
		/* ig_rhos(&MS, Ps[i], Names); */
		/* pressure_rhos(rhos[i], n_pure, Ts[i], Ps[i], tol, PFs, Names, err); */
		/* pressure_rhos(MS.rhos, MS.X, MS.T, Ps[i], tol, Names, err); */
		pressure_rhos(&MS, Ps[i], tol, Names, err);

		/*
			Check that user wants to continue (user can interrupt simulation if 
			bad results were encountered)
		 */
		printf("\n  %s\n\t%s\n\t%s",
				"Continue to calculate and print solution properties?",
				"0 - No", "1 - Yes");
		do{
			printf("\n    %s ", "Choice?");
			fgets(temp_str, 100, stdin);
		}while((1 != sscanf(temp_str, "%i", &usr_cont) || (0>usr_cont || 1<usr_cont)) 
				&& printf("\n  %s", "You must enter either 0 or 1"));
		if(1!=usr_cont){
			break;
		}

		/* Calculate solution properties */
		rho_mix[i] = mixture_rho(&MS);
		u_mix[i] = MIXTURE_CALC(u);
		h_mix[i] = MIXTURE_CALC(h);
		cp_mix[i] = MIXTURE_CALC(cp);
		cv_mix[i] = MIXTURE_CALC(cv);
		s_mix[i] = MIXTURE_CALC(s);
		g_mix[i] = MIXTURE_CALC(g);
		a_mix[i] = MIXTURE_CALC(a);

		/* Fill in header */
		headers[i] = (char *)malloc(40);
		snprintf(headers[i], 40, "[T=%.1f K, P=%.2f bar]", Ts[i], Ps[i]/1.e5);
	}
#undef MIXTURE_CALC

	print_cases_properties(n_sims, headers, rho_mix, Ps, u_mix, h_mix, cp_mix, cv_mix, s_mix, g_mix, a_mix);
}

/*
	Calculate realistic densities in ideal-solution, such that the internal 
	energy and enthalpy remain the same from pre-mixing to post-mixing 
	conditions, starting with disparate temperatures and densities.

	See introduction of density_to_rhos for further explaination of what these 
	conditions mean.

	For now, I am using a simplex method to find the pressure P and temperature 
	T to satisfy the conditions, simultaneously.  The function finds the uniform 
	temperature and pressure of the mixture, as well as component densities 
	under mixture conditions.
 */
void densities_Ts_to_mixture(MixtureState *M, double *P_out, double *T_in, double tol, char **Names, FpropsError *err){
#define N_PURE M->X->pures
#define RHOS M->rhos
#define XS M->X->xs
#define PF M->X->PF
#define N_VERT 3 /* number of simplex vertices */
#define MAX_LOOP 20

	unsigned i1,i2; /* counter variable */
	double u_avg=0.0, h_avg=0.0; /* average internal energy and enthalpy; calculate specially, since each has a different temperature */
	double T_avg=0.0, T_min, T_max, /* maximum and average temperatures */
		   P_avg=0.0, P_min, P_max, /* average pressure */
		   P_now; /* pressure at each T,rho pair in turn */

	for(i1=0;i1<N_PURE;i1++){
		/* finding average internal energy and enthalpy */
		u_avg += M->X->xs[i1] * fprops_u((FluidState){T_in[i1], RHOS[i1], PF[i1]}, err);
		h_avg += M->X->xs[i1] * fprops_h((FluidState){T_in[i1], RHOS[i1], PF[i1]}, err);

		/* finding maximum and average temperature and pressure */
		P_now = fprops_p((FluidState){T_in[i1], RHOS[i1], PF[i1]}, err);
		
		if(i1==0 || P_now < P_min){
			P_min = P_now;
		}
		if(i1==0 || P_now > P_max){
			P_max = P_now;
		}
		T_avg += T_in[i1];
		P_avg += P_now;
	}
	T_min = my_min(N_PURE, T_in);
	T_max = my_max(N_PURE, T_in);
	T_avg /= N_PURE;
	P_avg /= N_PURE;

	double p[]={
		P_avg,
		P_avg,
		P_avg + 0.25*(P_min - P_avg)
	};
	double T[]={
		T_avg,
		T_avg + 0.25*(T_min - T_avg),
		T_avg
	};
	double u[N_VERT],
		   h[N_VERT];
	double delta_p, delta_T;

	for(i1=0;i1<MAX_LOOP;i1++){
		printf("\n  Iteration %u of %u; current conditions are:\n", i1+1, MAX_LOOP);
		for(i2=0;i2<N_VERT;i2++){
			printf("\t    T[%u]=%.1f, p[%u]=%.0f\n", i2, T[i2], i2, p[i2]);
		}

		for(i2=0;i2<N_VERT;i2++){
			M->T = T[i2];
			pressure_rhos(M, p[i2], tol, Names, err);
			u[i1] = mixture_u(M, err);
			h[i1] = mixture_h(M, err);
		}

		if(fabs(u_avg - u[0]) < tol && fabs(h_avg - h[0]) < tol){
			printf("\n\n\tRoot-finding SUCCEEDED after %u iterations;"
					"\n\t  at overall internal energy u[0]=%.6g kg/m3,"
					"\n\t  overall enthalpy h[0]=%.6g kg/m3,"
					"\n\t  temperature T[0]=%.1f K and pressure p[0]=%.5f Pa.\n",
					i1, u[0], h[0], T[0], p[0]);
			M->T = T[0];
			*P_out = p[0];
			break;
		}
		if(fabs(p[0] - p[2])<tol){
			printf("\n\n\tRoot-finding FAILED after %u iterations;"
					"\n\t  internal energy/enthalpy conditions not satisfied, but difference"
					"\n\t  between pressures is too small:"
					"\n\t  p[0]=%.0f Pa, p[1]=%.0f Pa, p[2]=%.0f Pa.\n",
					i1, p[0], p[1], p[2]);
			break;
		}
		if(fabs(T[0] - T[1])<tol){
			printf("\n\n\tRoot-finding FAILED after %u iterations;"
					"\n\t  internal energy/enthalpy conditions not satisfied, but difference"
					"\n\t  between temperatures too small:"
					"\n\t  T[0]=%.1f K, T[1]=%.1f K, T[2]=%.1f K.\n",
					i1, T[0], T[1], T[2]);
			break;
		}

		/*	
			Update temperature and pressure for next iteration:
		 */
		delta_p = (u_avg - u[0]) * (p[0] - p[2]) / (u[0] - u[2]);
		delta_T = (h_avg - h[0]) * (T[0] - T[1]) / (h[0] - h[1]);

		while((p[0]+delta_p)<P_min || (p[0]+delta_p)>P_max){
			delta_p *= 0.51;
			if(fabs(delta_p) < tol){
				puts("\n\n\tChange in pressure has become too small!");
				break;
			}
		}
		while((T[0]+delta_T)<T_min || (T[0]+delta_T)>T_max){
			delta_T *= 0.53;
			if(fabs(delta_T) < tol){
				puts("\n\n\tChange in temperature has become too small!");
				break;
			}
		}
		if(fabs(delta_p)<tol || fabs(delta_T)<tol){
			printf("\n\n\tChange in pressure or temperature has become too small!"
					"\n\t  delta_T=%.5g K, delta_p=%.5g Pa.", delta_T, delta_p);
			break;
		}

		p[2] = p[0];
		p[0] += delta_p;
		p[1] = p[0];

		T[1] = T[0];
		T[0] += delta_T;
		T[2] = T[0];

		if(i1==MAX_LOOP-1){
			puts("\n\n  Reached maximum number of iterations; terminating now...");
		}
	}
#undef N_PURE
#undef N_VERT
#undef MAX_LOOP
}

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

	char *fluids[]={
		"nitrogen", "ammonia", "carbondioxide", "methane", "water"
	};
	char *fluid_names[]={
		"Nitrogen", "Ammonia", "Carbon Dioxide", "Methane", "Water"
	};
	const EosData *IdealEos[]={
		&eos_rpp_nitrogen, 
		&eos_rpp_ammonia, 
		&eos_rpp_carbon_dioxide, 
		&eos_rpp_methane, 
		&eos_rpp_water
	};

	PureFluid *Helms[NFLUIDS];
	PureFluid *Pengs[NFLUIDS];
	PureFluid *Ideals[NFLUIDS];
	ReferenceState ref = {FPROPS_REF_REF0};
	FpropsError err = FPROPS_NO_ERROR;

	/*	
		Fill the `Helms' PureFluid array with data from the helmholtz equation 
		of state.
	 */
	int i;
	for(i=N2;i<NFLUIDS;i++){
		Helms[i] = fprops_fluid(fluids[i],"helmholtz",NULL);
		Pengs[i] = fprops_fluid(fluids[i],"pengrob",NULL);
		Ideals[i] = ideal_prepare(IdealEos[i],&ref);
	}

	/* Mixing conditions (temperature,pressure), and mass fractions */
	double T[]={250, 300, 300, 350, 350, 400};             /* K */
	double P[]={1.5e5, 1.5e5, 1.9e5, 1.9e5, 2.1e5, 2.1e5}; /* Pa */
	double x[NFLUIDS];                    /* mass fractions */

	double props[] = {1, 3, 2, 1.5, 2.5}; /* proportions of mass fractions */
	mixture_x_props(NFLUIDS, x, props);   /* mass fractions from proportions */

	/* solve_mixture_conditions(NFLUIDS, NSIMS, x, T, P, Helms, fluid_names, &err); */
	double rho1[]={
		2, 3, 2.5, 1.7, 1.9
	};
	double rho2[]={
		1.1, 2, 1.5, 1.7, 1.9
	};

	double tol=1.e-9;
	MixtureSpec MX = {
		NFLUIDS, x,
		/* Helms */
		/* Ideals */
		Pengs
	};
	/* MS = {
		T[3], rho1, &MX
	}
	densities_to_mixture(&MS, tol, fluid_names, &err); */

	/* double T_i[]={300, 350, 320, 410, 390}; */
	/* double P_i; */
	/* MixtureState MS = {
		.T = T[2],
		.rhos = &rho2[0],
		.X = &MX
	}; */
	/* densities_Ts_to_mixture(&MS, &P_i, T_i, tol, fluid_names, &err); */
	solve_mixture_conditions(NSIMS, T, P, &MX, fluid_names, &err);

	return 0;
} /* end of `main' */

