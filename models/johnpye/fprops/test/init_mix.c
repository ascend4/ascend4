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
void pengrob_phi_pure(double *phi_out, MixtureSpec *M, double T, double *rhos, FpropsError *err);
void pengrob_phi_mix(double *phi_out, MixtureState *M, double P, FpropsError *err);
void mole_fractions(unsigned n_pure, double *x_mole, double *X_mass, PureFluid **PF);
void test_one(double *Ts, double *Ps);
void test_two(double T, double P);
void test_three(double T, double *rhos);
void test_four(double T, double P);

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
		M->ph_frac[0] = (V[0] * YM_sum) / ((V[0] * YM_sum) + ((1 - V[0]) * XM_sum));
		M->ph_frac[1] = 1 - M->ph_frac[0];

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

	/* Mixing conditions (temperature,pressure), and mass fractions */
	double T[]={250, 300, 300, 350, 350, 400, 900};               /* K */
	double P[]={1.5e5, 1.5e5, 1.9e5, 1.9e5, 2.1e5, 2.1e5, 3.2e6}; /* Pa */

	double rho1[]={
		2, 3, 2.5, 1.7, 1.9
	};
	double rho2[]={
		1.1, 2, 1.5, 1.7, 1.9
	};

	/* test_one(T, P); */
	test_two(T[1], P[1]);
	test_three(T[1], rho1);
	test_four(T[5], P[6]);

	return 0;
} /* end of `main' */

void pengrob_phi_pure(double *phi_out, MixtureSpec *M, double T, double *rhos, FpropsError *err){
#define NPURE M->pures
#define D M->PF[i]->data
#define RR D->R
#define PR D->corr.pengrob
	unsigned i;
	double P,     /* pressure */
		   aT,    /* parameter `a' at the temperature */
		   alpha, /* intermediate variable in calculating aT */
		   Tr,    /* reduced temperature */
		   A,     /* two variables */
		   B,
		   Z;     /* compressibility */
	double cpx_ln; /* rather involved part of the fugacity coefficient */
	
	for(i=0;i<NPURE;i++){ /* find fugacity coefficient for each component */
		P = fprops_p((FluidState){T, rhos[i], M->PF[i]}, err);
		printf("\n  Substance number %u\n\tPressure is now %.0f Pa", i, P);

		Tr = T/D->T_c;
		alpha = (1 + PR->kappa * (1 - sqrt(Tr)));
		aT = PR->aTc * alpha * alpha;

		A = aT * P / (RR * RR * T * T);
		B = PR->b * P / (RR * T);
		Z = P / (rhos[i] * RR * T);
		printf("\n\tCompressibility is %.6g;\n\t`A' is %.6g\n\t`B' is %.6g", Z, A, B);

		cpx_ln = log( (Z + (1 + M_SQRT2)*B)/(Z + (1 - M_SQRT2)*B) );
		phi_out[i] = exp( Z - 1 - log(Z - B) - (A / 2 / M_SQRT2) * cpx_ln );
		printf("\n\tLog of fugacity coefficient is %.6g", Z - 1 - log(Z - B) - (A / (2*M_SQRT2))*cpx_ln);
		printf("\n\tFugacity coefficient is %.6g", phi_out[i]);
	}

#undef PR
#undef RR
#undef D
#undef NPURE
}

void pengrob_phi_mix(double *phi_out, MixtureState *M, double P, FpropsError *err){
#define NPURE M->X->pures
#define XS M->X->Xs
#define TT M->T
#define D M->X->PF[i]->data
#define RMOL D->R * D->M
#define PR D->corr.pengrob
	unsigned i, j;
	double as[NPURE], /* parameters a,b for each species */
		   bs[NPURE];
	double a_mix=0.0, /* parameters a,b for the mixture */
		   b_mix=0.0,
		   M_mix,     /* average molar mass of mixture */
		   v_mix,     /* molar volume of mixture */
		   P_calc,    /* pressure */
		   alpha,     /* intermediate variable in calculating aT */
		   Tr,        /* reduced temperature */
		   A,         /* two variables */
		   B,
		   Z;         /* compressibility */
	double cpx_term,
		   a_sum=0.0;
	double x_mole[NPURE];
	mole_fractions(NPURE, x_mole, XS, M->X->PF); /* mole fractions */

	puts("\n  Mixture fugacity coefficient calculations:");
	for(i=0;i<NPURE;i++){
		Tr = TT/D->T_c;
		alpha = (1 + PR->kappa * (1 - sqrt(Tr)));
		as[i] = PR->aTc * alpha * alpha * D->M * D->M; /* individual parameters converted to use molar units for proper combining */
		bs[i] = PR->b * D->M;
		printf("\n\tSubstance number %u\n\t  Parameter `a' %.6g\n\t  Parameter `b' %.6g",
				i, as[i], bs[i]);
		b_mix += bs[i] * XS[i];
	}
	for(i=0;i<NPURE;i++){
		for(j=0;j<NPURE;j++){
			a_mix += sqrt(as[i]*as[j]) * XS[i] * XS[j];
		}
	}
	printf("\n\tMixture-wide parameter `a' %.6g\n\tMixture-wide parameter `b' %.6g", a_mix, b_mix);

	i = 0;
	M_mix = mixture_M_avg(NPURE, XS, M->X->PF);

	v_mix = M_mix / mixture_rho(M);
	printf("\n\tMixture average molar mass %.2f kg/kmol\n\tMixture mass density %.4f kg/m3"
			"\n\tMixture molar volume %.6g m3/kmol", M_mix, mixture_rho(M), v_mix);
	P_calc = (RMOL * TT / (v_mix - b_mix)) - (a_mix / (v_mix * (v_mix + b_mix) + b_mix * (v_mix - b_mix)));
	A = a_mix * P_calc / (RMOL * RMOL * TT * TT);
	B = b_mix * P_calc / (RMOL * TT);
	Z = v_mix * P_calc / (RMOL * TT);
	printf("\n  Mixture fugacity coefficient:\n\tPressure is %.0f Pa;"
			"\n\tCompressibility is %.5f;\n\t`A' is %.6g\n\t`B' is %.6g",
			P_calc, Z, A, B);

	for(i=0;i<NPURE;i++){
		for(j=0;j<NPURE;j++){
			a_sum += x_mole[j] * sqrt(as[i]*as[j]);
		}
		cpx_term = (2*a_sum/a_mix - bs[i]/b_mix) * log( (Z + (1 + M_SQRT2)*B) / (Z + (1 - M_SQRT2)*B) );
		phi_out[i] = bs[i]*(Z-1)/b_mix - log(Z - B) - (A/(2*M_SQRT2*B) * cpx_term);
		printf("\n\tFugacity coefficient is %.6g", phi_out[i]);
	}

#undef PR
#undef RMOL
#undef D
#undef TT
#undef XS
#undef NPURE
}

void mole_fractions(unsigned n_pure, double *x_mole, double *X_mass, PureFluid **PF){
#define D PF[i]->data
	unsigned i;
	double XM_sum=0.0; /* sum of mass fraction over molar mass terms */

	for(i=0;i<n_pure;i++){
		XM_sum += X_mass[i] / D->M;
	}
	for(i=0;i<n_pure;i++){
		x_mole[i] = X_mass[i] / D->M / XM_sum;
	}
#undef D
}

/*
	Calculate and print mixture properties, given several temperatures and 
	densities
 */
void test_one(double *Ts, double *Ps){
	int i; /* counter variable */
	enum FluidAbbrevs {N2,NH3,CO2,CH4,/* H2O, */NFLUIDS}; /* fluids that will be used */

	char *fluids[]={
		"nitrogen", "ammonia", "carbondioxide", "methane", "water"
	};
	char *fluid_names[]={
		"Nitrogen", "Ammonia", "Carbon Dioxide", "Methane", "Water"
	};
	/* const EosData *IdealEos[]={
		&eos_rpp_nitrogen, 
		&eos_rpp_ammonia, 
		&eos_rpp_carbon_dioxide, 
		&eos_rpp_methane, 
		&eos_rpp_water
	}; */

	PureFluid *Helms[NFLUIDS];
	/* PureFluid *Pengs[NFLUIDS]; */
	/* PureFluid *Ideals[NFLUIDS]; */
	/* ReferenceState ref = {FPROPS_REF_REF0}; */
	FpropsError err = FPROPS_NO_ERROR;

	/*	
		Fill the `Helms' PureFluid array with data from the helmholtz equation 
		of state.
	 */
	for(i=0;i<NFLUIDS;i++){
		Helms[i] = fprops_fluid(fluids[i],"helmholtz",NULL);
		/* Pengs[i] = fprops_fluid(fluids[i],"pengrob",NULL); */
		/* Ideals[i] = ideal_prepare(IdealEos[i],&ref); */
	}

	double x[NFLUIDS];                    /* mass fractions */

	double props[] = {1, 3, 2, 1.5, 2.5}; /* proportions of mass fractions */
	mixture_x_props(NFLUIDS, x, props);   /* mass fractions from proportions */

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

	char **Names = fluid_names;
	MixtureSpec *M = &MX;
	unsigned n_sims = 5;

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
#define MIXTURE_CALC(PROP) mixture_##PROP(&MS, &err)
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
		initial_rhos(&MS, Ps[i], Names, &err);
		pressure_rhos(&MS, Ps[i], tol, /* Names, */ &err);

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
	Perform vapor-liquid equilibrium on a mixture of water and ammonia, to test 
	function for modeling flash processes
 */
void test_two(double T, double P){
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
	/* PureFluid *Pengs[NFLUIDS]; */
	/* ReferenceState ref = {FPROPS_REF_REF0}; */
	FpropsError err = FPROPS_NO_ERROR;

	/*	
		Fill the `Helms' PureFluid array with data from the helmholtz equation 
		of state.
	 */
	for(i=0;i<NFLUIDS;i++){
		Helms[i] = fprops_fluid(fluids[i],"helmholtz",NULL);
		/* Pengs[i] = fprops_fluid(fluids[i],"pengrob",NULL); */
	}

	double x[NFLUIDS];                    /* mass fractions */

	double props[] = {1, 3, 2, 1.5, 2.5}; /* proportions of mass fractions */
	mixture_x_props(NFLUIDS, x, props);   /* mass fractions from proportions */

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
		.T=T
		, .X=&MX
		, .ph_frac = mp_ps
		, .Xs = mp_xs
		, .rhos = mp_rhos
	};
	MP.Xs[0] = (double *)malloc(2*sizeof(double));
	MP.Xs[1] = (double *)malloc(2*sizeof(double));
	MP.rhos[0] = (double *)malloc(2*sizeof(double));
	MP.rhos[1] = (double *)malloc(2*sizeof(double));

	mixture_flash(&MP, P, fluid_names, &err);
	printf("\n\tAt %.1f K and %.0f Pa, the mixture is in vapor-liquid equilibrium:"
			"\n\t  %.5f of the mass is in the vapor"
			"\n\t    %s mass fraction in the vapor is %.5f"
			"\n\t    %s mass fraction in the vapor is %.5f"
			"\n\t  %.5f of the mass is in the liquid"
			"\n\t    %s mass fraction in the liquid is %.5f"
			"\n\t    %s mass fraction in the liquid is %.5f"
			"\n",
			MP.T, P, MP.ph_frac[0], fluid_names[0], MP.Xs[0][0], 
			fluid_names[1], MP.Xs[0][1], MP.ph_frac[1], fluid_names[0], 
			MP.Xs[1][0], fluid_names[1], MP.Xs[1][1]);
	printf("\n\n\tCross-check on mass fractions:");
	for(i=0;i<NFLUIDS;i++){
		printf("\n\t  Total %s mass fraction is (%.4f x %.4f) + (%.4f x %.4f) = %.5f",
				fluid_names[i], MP.ph_frac[0], MP.Xs[0][i], MP.ph_frac[1], MP.Xs[1][i],
				((MP.ph_frac[0] * MP.Xs[0][i]) + (MP.ph_frac[1] * MP.Xs[1][i])));
	} puts("");
	for(i=0;i<NFLUIDS;i++){
		printf("\n\t  Total %s mass fraction should be %.5f", fluid_names[i], MP.X->Xs[i]);
	} puts("");
}

void test_three(double T, double *rhos){
	unsigned i;
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
	/* PureFluid *Pengs[NFLUIDS]; */
	/* PureFluid *Ideals[NFLUIDS]; */
	/* ReferenceState ref = {FPROPS_REF_REF0}; */
	FpropsError err = FPROPS_NO_ERROR;

	/*	
		Fill the `Helms' PureFluid array with data from the helmholtz equation 
		of state.
	 */
	for(i=0;i<NFLUIDS;i++){
		Helms[i] = fprops_fluid(fluids[i],"helmholtz",NULL);
		/* Pengs[i] = fprops_fluid(fluids[i],"pengrob",NULL); */
		/* Ideals[i] = ideal_prepare(IdealEos[i],&ref); */
	}

	double x[NFLUIDS];                    /* mass fractions */

	double props[] = {1, 3, 2, 1.5, 2.5}; /* proportions of mass fractions */
	mixture_x_props(NFLUIDS, x, props);   /* mass fractions from proportions */

	/*
		choose which model to use by commenting and uncommenting the array names 
		(only one name uncommented at a time)
	 */
	MixtureSpec MX = {
		.pures=NFLUIDS
		, .Xs=x
		, .PF=Helms
		/* , .PF=Ideals */
		/* , .PF=Pengs */
	};

	MixtureState MS = {
		.T=T
		, .rhos=rhos
		, .X = &MX
	};

	double tol=1.e-6;

	densities_to_mixture(&MS, tol, fluid_names, &err);
}

void test_four(double T, double P){
	unsigned i;
	enum FluidAbbrevs {N2,NH3,CO2,CH4,/* H2O, */NFLUIDS}; /* fluids that will be used */

	char *fluids[]={
		"nitrogen", "ammonia", "carbondioxide", "methane", "water"
	};
	char *fluid_names[]={
		"Nitrogen", "Ammonia", "Carbon Dioxide", "Methane", "Water"
	};

	PureFluid *Helms[NFLUIDS];
	PureFluid *Pengs[NFLUIDS];
	FpropsError err = FPROPS_NO_ERROR;

	for(i=0;i<NFLUIDS;i++){
		Helms[i] = fprops_fluid(fluids[i],"helmholtz",NULL);
		Pengs[i] = fprops_fluid(fluids[i],"pengrob",NULL);
	}

	double rhos[NFLUIDS] = {0.0};
	double x[NFLUIDS];                    /* mass fractions */

	double props[] = {1, 3, 2, 1.5, 2.5}; /* proportions of mass fractions */
	mixture_x_props(NFLUIDS, x, props);   /* mass fractions from proportions */

	MixtureSpec MX = {
		.pures=NFLUIDS
		, .Xs=x
		, .PF=Pengs
	};
	MixtureState MS = {
		.T=T
		, .rhos=rhos
		, .X = &MX
	};

	double phi_1[NFLUIDS],
		   phi_2[NFLUIDS];
	double tol = 1.e-6;

	initial_rhos(&MS, P, fluid_names, &err);
	pressure_rhos(&MS, P, tol, /* fluid_names, */ &err);

	pengrob_phi_pure(phi_1, &MX, T, MS.rhos, &err);
	pengrob_phi_mix(phi_2, &MS, P, &err);

	puts("\n  Pure-component fugacity coefficients:");
	for(i=0;i<NFLUIDS;i++){
		printf("\t%s %.6g\n", fluid_names[i], phi_1[i]);
	}
	puts("\n  Ideal-solution fugacity:");
	for(i=0;i<NFLUIDS;i++){
		printf("\t%s %.6g Pa\n", fluid_names[i], phi_1[i] * MS.X->Xs[i] * P);
	}
	puts("\n  Mixture fugacity coefficients:");
	for(i=0;i<NFLUIDS;i++){
		printf("\t%s %.6g\n", fluid_names[i], phi_2[i]);
	}
	puts("\n  Mixture fugacity:");
	for(i=0;i<NFLUIDS;i++){
		printf("\t%s %.6g\n", fluid_names[i], phi_2[i] * P);
	}

/* 	double tau, delta;
	double B_mix, B_centi, B_milli,
		   d_helm;
	puts("\n  Attempt to find virial coefficients from Helmholtz EOS:");
#define CH Helms[i]->data->corr.helm
	for(i=0;i<NFLUIDS;i++){
		tau = CH->T_star / T;
		delta = rhos[i] / CH->rho_star;
		printf("\tFor substance %s\n\t  Tau is %.6g\n\t  Delta is %.6g\n",
				fluid_names[i], tau, delta);

		B_mix = helm_resid_del(tau, delta, CH) / Helms[i]->data->rho_c;
		B_centi = helm_resid_del(tau, delta / 100., CH) / Helms[i]->data->rho_c;
		B_milli = helm_resid_del(tau, delta / 1000., CH) / Helms[i]->data->rho_c;
		d_helm = helm_resid_del(tau, 0, CH);

		printf("\n\t  V.C. from derivative at mixture conditions: %.6g"
				"\n\t  V.C. from derivative at tau = 0.01 of mixture conditions: %.6g"
				"\n\t  V.C. from derivative at tau = 0.001 of mixture conditions: %.6g"
				"\n\t  derivative at tau = 0 : %.6g" "\n\n" ,B_mix ,B_centi ,B_milli
				,d_helm);
	} */
#undef CH
}
