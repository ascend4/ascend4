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
double rachford_rice(double V, void *user_data);
void mixture_flash(MixturePhaseState *M, double P, char **Names, FpropsError *err);
void solve_mixture_conditions(unsigned n_sims, double *Ts, double *Ps, MixtureSpec *M, char **Names, FpropsError *err);
void densities_Ts_to_mixture(MixtureState *MS, double *P_out, double *T_in, double tol, char **Names, FpropsError *err);

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
void mixture_flash(MixturePhaseState *M, double P, char **Names, FpropsError *err){
#define NPURE M->X->pures
#define ZS M->X->Xs
#define XS M->Xs
#define PFL M->X->PF
#define D PFL[i]->data
#define RHOS M->rhos
	unsigned i;
	double p_sat,     /* temporary saturation pressure */
		   p_b=0.0,   /* bubble-point and dew-point pressures */
		   p_d,
		   rp_d=0.0;  /* reciprocal dew-point pressure */
	double Ks[NPURE]; /* K-factor: ratio (vapor mole fraction)/(liquid mole fraction) */
	double V[]={0.50, 0.52}; /* vapor mass fraction */
	double xs[NPURE],
		   ys[NPURE],
		   zs[NPURE];
	double XM_sum=0.0,   /* sum of (X_i * MolarMass_i) */
		   YM_sum=0.0,
		   ZM_sum=0.0;   /* sum of (Z_i / MolarMass_i) */

	for(i=0;i<NPURE;i++){
		zs[i] = ZS[i] / D->M; /* first step in calculating overall mole fractions */
		ZM_sum += zs[i];
	}
	for(i=0;i<NPURE;i++){
		zs[i] /= ZM_sum;      /* divide by reciprocal average molar mass to obtain mole fractions */
		fprops_sat_T(M->T, &p_sat, (RHOS[1]+i), (RHOS[0]+i), PFL[i], err);
		p_b += zs[i] * p_sat;
		rp_d += zs[i] / p_sat;
		Ks[i] = p_sat / P;    /* value of K-factor under Raoult's law */
	}
	p_d = 1./rp_d;

	if(P<=p_d){                /* system will be all-gas */
		M->ph = GAS_PHASE;
		puts("\n\tMixture is all gas/vapor.");
	}else if(P>=p_b){          /* system will be all-liquid */
		M->ph = LIQ_PHASE;
		puts("\n\tMixture is all liquid.");
	}else{                     /* system may be in vapor-liquid equilibrium */
		RRData RR = {NPURE, zs, Ks};
		double tol = 1.e-6;

		secant_solve(&rachford_rice, &RR, V, tol);

		for(i=0;i<NPURE-1;i++){
			ys[i] = (zs[i] * Ks[i]) / ((1 - V[0]) + V[0]*Ks[i]);
			xs[i] = zs[i] / (1 + (V[0] * (Ks[i] - 1)));
		}
		ys[NPURE-1] = 1 - my_sum(NPURE-1, ys); /* ys[0] starts as 0.0, so the fractions should sum correctly */
		xs[NPURE-1] = 1 - my_sum(NPURE-1, xs);

		for(i=0;i<NPURE;i++){
			XS[0][i] = ys[i] * D->M; /* first step in calculating mass fractions */
			XS[1][i] = xs[i] * D->M;

			XM_sum += xs[i] * D->M;
			YM_sum += ys[i] * D->M;
		}
		for(i=0;i<NPURE;i++){
			XS[0][i] /= YM_sum;
			XS[1][i] /= XM_sum;
		}
		M->phase_splits[0] = (V[0] * YM_sum) / ((V[0] * YM_sum) + ((1 - V[0]) * XM_sum));
		M->phase_splits[1] = 1 - M->phase_splits[0];

		for(i=0;i<NPURE;i++){
			printf("\n\tMolar mass of %s is %.3f kg/kmol;", Names[i], D->M);
		}
	}

#undef RHOS
#undef D
#undef PFL
#undef XS
#undef ZS
#undef NPURE
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
	enum FluidAbbrevs {/* N2, */NH3,/* CO2,CH4, */H2O,NFLUIDS}; /* fluids that will be used */

	char *fluids[]={
		/* "nitrogen", */ "ammonia", /* "carbondioxide", "methane", */ "water"
	};
	char *fluid_names[]={
		/* "Nitrogen", */ "Ammonia", /* "Carbon Dioxide", "Methane", */ "Water"
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
	for(i=0;i<NFLUIDS;i++){
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
		.pures=NFLUIDS
		, .Xs=x
		, .PF=Helms
		/* .PF=Ideals */
		/* .PF=Pengs */
	};

	double mp_ps[] = {0.0, 0.0};
	double *mp_xs[] = {NULL, NULL};
	double *mp_rhos[] = {NULL, NULL};
	MixturePhaseState MP = {
		.T=T[2]
		, .X=&MX
		, .phase_splits = mp_ps
		, .Xs = mp_xs
		, .rhos = mp_rhos
	};
	MP.Xs[0] = (double *)malloc(2*sizeof(double));
	MP.Xs[1] = (double *)malloc(2*sizeof(double));
	MP.rhos[0] = (double *)malloc(2*sizeof(double));
	MP.rhos[1] = (double *)malloc(2*sizeof(double));
	
	/* printf("\n\tAt %.1f K and %.0f Pa, the mixture is in vapor-liquid equilibrium:"
			"\n\t  %.5f of the mass is in the vapor"
			"\n\t    %s mass fraction in the vapor is %.5f"
			"\n\t    %s mass fraction in the vapor is %.5f"
			"\n\t  %.5f of the mass is in the liquid"
			"\n\t    %s mass fraction in the liquid is %.5f"
			"\n\t    %s mass fraction in the liquid is %.5f"
			"\n",
			MP.T, P[2], MP.phase_splits[0], fluid_names[0], MP.Xs[0][0], 
			fluid_names[1], MP.Xs[0][1], MP.phase_splits[1], fluid_names[0], 
			MP.Xs[1][0], fluid_names[1], MP.Xs[1][1]); */
	mixture_flash(&MP, P[2], fluid_names, &err);
	printf("\n\tAt %.1f K and %.0f Pa, the mixture is in vapor-liquid equilibrium:"
			"\n\t  %.5f of the mass is in the vapor"
			"\n\t    %s mass fraction in the vapor is %.5f"
			"\n\t    %s mass fraction in the vapor is %.5f"
			"\n\t  %.5f of the mass is in the liquid"
			"\n\t    %s mass fraction in the liquid is %.5f"
			"\n\t    %s mass fraction in the liquid is %.5f"
			"\n",
			MP.T, P[2], MP.phase_splits[0], fluid_names[0], MP.Xs[0][0], 
			fluid_names[1], MP.Xs[0][1], MP.phase_splits[1], fluid_names[0], 
			MP.Xs[1][0], fluid_names[1], MP.Xs[1][1]);

	return 0;
} /* end of `main' */

