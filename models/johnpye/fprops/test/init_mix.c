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
typedef double SecantSubjectFunction(double, void *user_data);

unsigned index_of_min(unsigned nelems, double *nums);
unsigned index_of_max(unsigned nelems, double *nums);
void secant_solve(SecantSubjectFunction *func, void *user_data, double x[2], double tol);
double rachford_rice(double V, void *user_data);
void mixture_flash(MixturePhaseState *M, double P, FpropsError *err);
void solve_mixture_conditions(unsigned n_sims, double *Ts, double *Ps, MixtureSpec *M, char **Names, FpropsError *err);
void densities_Ts_to_mixture(MixtureState *MS, double *P_out, double *T_in, double tol, char **Names, FpropsError *err);


/*
	Find index of the minimum value in the array `nums', with maximum index 
	`nelems'
 */
unsigned index_of_min(unsigned nelems, double *nums){
	unsigned i;
	unsigned min_ix=0;  /* the index of the minimum element */
	double min=nums[0]; /* the minimum element */

	for(i=1;i<nelems;i++){
		if(nums[i]<min){
			min_ix = i;    /* update both `min' and `min_ix' */
			min = nums[i];
		}
	}
	return min_ix;
}

/*
	Find index of the maximum value in the array `nums', with maximum index 
	`nelems'
 */
unsigned index_of_max(unsigned nelems, double *nums){
	unsigned i;
	unsigned max_ix=0;  /* the index of the minimum element */
	double max=nums[0]; /* the minimum element */

	for(i=1;i<nelems;i++){
		if(nums[i]>max){
			max_ix = i;    /* update both `min' and `min_ix' */
			max = nums[i];
		}
	}
	return max_ix;
}

/*
	Generic root-finding function that uses the secant method, starting from 
	the positions in `x' and setting the first element of `x' to the position 
	at which `func' equals zero within the given tolerance `tol'
 */
void secant_solve(SecantSubjectFunction *func, void *user_data, double x[2], double tol){
#define MAX_ITER 30
	unsigned i;
	double y[2];
	double delta_x;

	y[1] = (*func)(x[1], user_data);

	for(i=0;i<MAX_ITER;i++){
		y[0] = (*func)(x[0], user_data);
		if(fabs(y[0])<tol){
			printf("\n\n\tRoot-finding SUCCEEDED after %u iterations;"
					"\n\t  zeroed function has value %.6g at postion %.6g", i, y[0], x[0]);
			break;
		}
		if(x[0]==x[1]){
			printf("\n\n\tRoot-finding FAILED after %u iterations;"
					"\n\t  independent variables equal at %.6g,"
					"\n\t  function is not zero, but %.6g",
					i, x[0], y[0]);
			break;
		}

		/* update independent variable x[0] */
		delta_x = -y[0] * (x[0] - x[1])/(y[0] - y[1]);
		x[1] = x[0];     /* reassign second position to first position */
		y[1] = y[0];
		x[0] += delta_x; /* shift first position to more accurate value */
	}
#undef MAX_ITER
}

/*
	Structure to pass constant parameters into the Rachford-Rice function
 */
typedef struct{
	unsigned pures;
	double *zs;
	double *Ks;
} RRData;

/*
	Calculate the Rachford-Rice function for a given set of parameters and value 
	of the vapor fraction for a mixture in phase equilibrium
 */
double rachford_rice(double V, void *user_data){
	RRData *RR = (RRData *)user_data;
	double F=0.0;
	unsigned i;

	/* Add each term of the Rachford-Rice function to the output F */
	for(i=0;i<RR->pures;i++){
		F += (RR->zs[i] * (RR->Ks[i] - 1)) / (1 + V*(RR->Ks[i] - 1));
	}
	return F;
}

/*
	Determine whether the mixture given is in a one-phase (vapor or liquid) or 
	two-phase state, and if it is in two phases (VLE), what the mass fractions 
	within each phase are, and what fraction of the mass is in each phase.
 */
void mixture_flash(MixturePhaseState *M, double P, FpropsError *err){
#define NPURE M->X->pures
#define ZS M->X->Xs
#define XS M->Xs
#define PF M->X->PF
#define D PF[i]->data
#define RHOS M->rhos
	unsigned i;
	double p_sat,     /* temporary saturation pressure */
		   p_b=0.0,   /* bubble-point and dew-point pressures */
		   p_d,
		   rp_d=0.0;  /* reciprocal dew-point pressure */
	double Ks[NPURE]; /* K-factor: ratio (vapor mole fraction)/(liquid mole fraction) */
	double V_frac[]={0.50, 0.52}; /* vapor mass fraction */
	double xs[NPURE],
		   ys[NPURE],
		   zs[NPURE];
	double rho_l[NPURE],
		   rho_v[NPURE];
	double XM_sum=0.0,   /* sum of (X_i * MolarMass_i) */
		   YM_sum=0.0,
		   ZM_sum=0.0;   /* sum of (Z_i / MolarMass_i) */

	for(i=0;i<NPURE;i++){
		zs[i] = ZS[i] / D->M; /* first step in calculating overall mole fractions */
		ZM_sum += zs[i];
	}
	for(i=0;i<NPURE;i++){
		zs[i] /= ZM_sum;      /* divide by reciprocal average molar mass to obtain mole fractions */
		fprops_sat_T(M->T, &p_sat, (rho_l+i), (rho_v+i), PF[i], err);
		p_b += zs[i] * p_sat;
		rp_d += zs[i] / p_sat;
		Ks[i] = p_sat / P;    /* value of K-factor under Raoult's law */
	}
	p_d = 1./rp_d;

	if(P<=p_d){                /* system will be all-gas */
		M->ph = GAS_PHASE;
	}else if(P>=p_b){          /* system will be all-liquid */
		M->ph = LIQ_PHASE;
	}else{                     /* system may be in vapor-liquid equilibrium */
		RRData RR = {NPURE, zs, Ks};
		double tol = 1.e-3;

		secant_solve(&rachford_rice, &RR, V_frac, tol);

		for(i=0;i<NPURE;i++){
			ys[i] = (zs[i] * Ks[i]) / ((1 - V_frac[0]) + V_frac[0]*Ks[i]);
			xs[i] = zs[i] / (1 + (V_frac[0] * (Ks[i] - 1)));
			printf("\n\tThe vapor-phase mole fraction is %.5f,"
					" and the liquid-phase mole fraction is %.5f", ys[i], xs[i]);

			XS[0][i] = ys[i] * D->M; /* first step in calculating mass fractions */
			XS[1][i] = xs[i] * D->M;
			XM_sum += xs[i] * D->M;
			YM_sum += ys[i] * D->M;
		}
		for(i=0;i<NPURE;i++){
			XS[0][i] /= YM_sum;
			XS[1][i] /= XM_sum;
			RHOS[0][i] = rho_v[i];
			RHOS[1][i] = rho_l[i];
		}
		M->phase_splits[0] = V_frac[0];
		M->phase_splits[1] = 1 - V_frac[0];
	}

#undef RHOS
#undef D
#undef PF
#undef XS
#undef ZS
#undef NPURE
}

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
		MixtureState MS = {
			.T=Ts[i],
			.rhos=rhos[i],
			.X=M,
		};
		initial_rhos(&MS, Ps[i], Names, err);
		/* ig_rhos(&MS, Ps[i], Names); */
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
#define NPURE M->X->pures
#define RHOS M->rhos
#define XS M->X->Xs
#define PF M->X->PF
#define N_VERT 3 /* number of simplex vertices */
#define MAX_LOOP 20

	unsigned i1,i2; /* counter variable */
	double u_avg=0.0, h_avg=0.0;    /* average internal energy and enthalpy; calculate specially, since each has a different temperature */
	double T_avg=0.0, T_min, T_max, /* maximum and average temperatures */
		   P_avg=0.0, P_min, P_max, /* average pressure */
		   P_now;                   /* pressure at each T,rho pair in turn */

	for(i1=0;i1<NPURE;i1++){
		/* finding average internal energy and enthalpy */
		u_avg += M->X->Xs[i1] * fprops_u((FluidState){T_in[i1], RHOS[i1], PF[i1]}, err);
		h_avg += M->X->Xs[i1] * fprops_h((FluidState){T_in[i1], RHOS[i1], PF[i1]}, err);

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
	T_min = my_min(NPURE, T_in);
	T_max = my_max(NPURE, T_in);
	T_avg /= NPURE;
	P_avg /= NPURE;

	/*
		Arrays to hold pressure, temperature, internal energy, enthalpy, and 
		sum of internal energy with enthalpy for a solution.
	 */
	double p_sol[]={
		P_avg,
		P_avg,
		P_avg + 0.25*(P_min - P_avg)
	};
	double T_sol[]={
		T_avg,
		T_avg + 0.25*(T_min - T_avg),
		T_avg
	};
	double u[N_VERT],
		   h[N_VERT],
		   uh_err[N_VERT];
	double delta_p, delta_T;

	/*
		Essential values for simplex root-solver.
		
		Coefficients A, B, C, K control reflection, expansion, contraction, and 
		scaling of simplex as it seeks the minimum.
		
		Indices i_max and i_min locate the vertices where the maximum and 
		minimum values (of error in u and h, here) occur.

		Temperature and pressure T_centr and p_centr are the central point 
		between the two vertices other than the vertex with the highest error

		Temperature and pressure T_rf, T_ex, T_cn, and p_rf, p_ex, p_cn hold 
		potential locations for the new vertex (replacing the one with the 
		highest error).
	 */
	double A=1.05,
		   B=2.01,
		   C=0.49,
		   K=0.76;  /* used to re-scale vertices if contraction does not decrease chi-squared */
	unsigned i_max,
			 i_min;
	double T_centr,
		   p_centr;
	double T_rf, T_ex, T_cn,
		   p_rf, p_ex, p_cn;
	double uh_new[3];

	/*
		Find initial values of internal energy, enthalpy, and error (sum of 
		absolute values of difference between current and average u and h).
	 */
	for(i1=0;i1<N_VERT;i1++){
		M->T = T_sol[i1];
		pressure_rhos(M, p_sol[i1], tol, Names, err);
		u[i1] = mixture_u(M, err);
		h[i1] = mixture_h(M, err);
		uh_err[i1] = fabs(u_avg - u[i1]) + fabs(h_avg - h[i1]);
	}

	/*
		Seek minimum at which change in internal energy on mixing is zero, and 
		change in enthalpy on mixing is zero.

		I find the absolute value of the error in u and h and add these errors 
		together, so that only when both are zero will the result be zero.
	 */
	for(i1=0;i1<MAX_LOOP;i1++){
		printf("\n  Iteration %u of %u; current conditions are:\n", i1+1, MAX_LOOP);
		for(i2=0;i2<N_VERT;i2++){
			printf("\t    T_sol[%u]=%.1f, p_sol[%u]=%.0f\n", i2, T_sol[i2], i2, p_sol[i2]);
		}

		i_max = index_of_max(N_VERT, uh_err);
		i_min = index_of_min(N_VERT, uh_err);

		if(uh_err[i_min]<tol){
			printf("\n\n\tRoot-finding SUCCEEDED after %u iterations;"
					"\n\t  at overall internal energy u[0]=%.6g kg/m3,"
					"\n\t  overall enthalpy h[0]=%.6g kg/m3,"
					"\n\t  temperature T=%.1f K and pressure p=%.5f Pa.\n",
					i1, u[i_min], h[i_min], T_sol[i_min], p_sol[i_min]);
			M->T = T_sol[i_min];
			*P_out = p_sol[i_min];
			break;
		}
		/* if(fabs(p_sol[0] - p_sol[2])<tol){
			printf("\n\n\tRoot-finding FAILED after %u iterations;"
					"\n\t  internal energy/enthalpy conditions not satisfied, but difference"
					"\n\t  between pressures is too small:"
					"\n\t  p_sol[0]=%.0f Pa, p_sol[1]=%.0f Pa, p_sol[2]=%.0f Pa.\n",
					i1, p_sol[0], p_sol[1], p_sol[2]);
			break;
		}
		if(fabs(T_sol[0] - T_sol[1])<tol){
			printf("\n\n\tRoot-finding FAILED after %u iterations;"
					"\n\t  internal energy/enthalpy conditions not satisfied, but difference"
					"\n\t  between temperatures too small:"
					"\n\t  T_sol[0]=%.1f K, T_sol[1]=%.1f K, T_sol[2]=%.1f K.\n",
					i1, T_sol[0], T_sol[1], T_sol[2]);
			break;
		} */

		T_centr = 0.0; /* zero the center-points between vertices not having the highest error */
		p_centr = 0.0;
		for(i2=0;i2<N_VERT;i2++){
			if(i2!=i_max){
				T_centr += T_sol[i2];
				p_centr += p_sol[i2];
			}
		}
		
		T_rf = (1 + A)*T_centr - A*T_sol[i_max];
		T_ex = (1 - B)*T_centr + B*T_sol[i_max];
		T_cn = (1 - C)*T_centr + C*T_sol[i_max];

		p_rf = (1 + A)*p_centr - A*p_sol[i_max];
		p_ex = (1 - B)*p_centr + B*p_sol[i_max];
		p_cn = (1 - C)*p_centr + C*p_sol[i_max];

		/*	
			Update temperature and pressure for next iteration:
		 */
		/* delta_p = (u_avg - u[0]) * (p_sol[0] - p_sol[2]) / (u[0] - u[2]);
		delta_T = (h_avg - h[0]) * (T_sol[0] - T_sol[1]) / (h[0] - h[1]);

		while((p_sol[0]+delta_p)<P_min || (p_sol[0]+delta_p)>P_max){
			delta_p *= 0.51;
			if(fabs(delta_p) < tol){
				puts("\n\n\tChange in pressure has become too small!");
				break;
			}
		}
		while((T_sol[0]+delta_T)<T_min || (T_sol[0]+delta_T)>T_max){
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

		p_sol[2] = p_sol[0];
		p_sol[0] += delta_p;
		p_sol[1] = p_sol[0];

		T_sol[1] = T_sol[0];
		T_sol[0] += delta_T;
		T_sol[2] = T_sol[0]; */

		if(i1==MAX_LOOP-1){
			puts("\n\n  Reached maximum number of iterations; terminating now...");
		}
	}
#undef NPURE
#undef RHOS
#undef XS
#undef PF
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
	int i; /* counter variable */
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

	double rho1[]={
		2, 3, 2.5, 1.7, 1.9
	};
	double rho2[]={
		1.1, 2, 1.5, 1.7, 1.9
	};

	/*
		choose which model to use by commenting and uncommenting the array names 
		(only one name uncommented at a time)
	 */
	MixtureSpec MX = {
		.pures=NFLUIDS,
		.Xs=x,
		.PF=Helms
		/* .PF=Ideals */
		/* .PF=Pengs */
	};

	solve_mixture_conditions(NSIMS, T, P, &MX, fluid_names, &err);

	return 0;
} /* end of `main' */

