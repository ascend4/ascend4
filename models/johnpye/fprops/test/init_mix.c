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
	by Jacob Shealy, June 4-July 20, 2015
	
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
#include "../mixtures/mixture_prepare.h"
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
#define FUNC_REPT(REPORT) printf("\n  " TITLE "%s", REPORT)
#define FUNC_MARK(REPORT) printf("\n  " TITLE "mark %s", REPORT)

extern const EosData eos_rpp_nitrogen;
extern const EosData eos_rpp_ammonia;
extern const EosData eos_rpp_carbon_dioxide;
extern const EosData eos_rpp_methane;
extern const EosData eos_rpp_water;

/* Function prototypes */
SecantSubjectFunction rachford_rice;
SecantSubjectFunction dew_p_error;
SecantSubjectFunction bubble_p_error;
SecantSubjectFunction flash_error;
double dew_pressure(MixtureSpec *MS, double T, FpropsError *err);
double bubble_pressure(MixtureSpec *MS, double T, FpropsError *err);

PhaseMixState *mixture_phase_prepare(MixtureSpec *MX, double T, double P, FpropsError *err);
void read_in_MixtureSpec(MixtureSpec **MS_array, MixtureSpec *MS_src, PhaseSpec *PS);
void mixture_flash(PhaseSpec *PS, MixtureSpec *MS, double T, double P, FpropsError *err);
void solve_mixture_conditions(unsigned n_sims, double *Ts, double *Ps, MixtureSpec *M, char **Names, FpropsError *err);
void densities_Ts_to_mixture(MixtureState *MS, double *P_out, double *T_in, double tol, char **Names, FpropsError *err);

double pengrob_phi_pure(PureFluid *PF, double T, double P, PhaseName type, FpropsError *err);
double poynting_factor(PureFluid *PF, double T, double P, FpropsError *err);

// void test_one(double *Ts, double *Ps);
// void test_two(double T, double P);
// void test_three(double T, double *rhos);
// void test_four(double T, double P);
void test_five(double T, double P);
void test_six(void);
void test_seven(void);

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
	// double T[]={250, 300, 300, 350, 350, 400, 900};               /* K */
	// double P[]={1.5e5, 1.5e5, 1.9e5, 1.9e5, 2.1e5, 2.1e5, 3.2e6}; /* Pa */

	/* double rho1[]={
		2, 3, 2.5, 1.7, 1.9
	};
	double rho2[]={
		1.1, 2, 1.5, 1.7, 1.9
	}; */

	/* test_one(T, P); */
	/* test_two(T[1], P[1]); */
	/* test_three(T[1], rho1); */
	/* test_four(T[5], P[6]); */
	/* test_five(270, 1.5e5); */
	/* test_six(); */
	test_seven();

	return 0;
} /* end of `main' */

/*
	Structure to pass constant parameters into the Rachford-Rice function
 */
typedef struct RachRiceData_Struct {
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
#if 0
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
		M->phases = 1;
		M->ph_name[0] = VAPOR;
		puts("\n\tMixture is all gas/vapor.");
	}else if(P>=p_b){          /* system will be all-liquid */
		M->phases = 1;
		M->ph_name[0] = LIQUID;
		puts("\n\tMixture is all liquid.");
	}else{                     /* system may be in vapor-liquid equilibrium */
		M->phases = 2;
		M->ph_name[0] = VAPOR;
		M->ph_name[1] = LIQUID;
		M->ph_name[2] = SUPERCRIT;

		RRData RR = {NPURE, zs, Ks};
		double tol = 1.e-6;

		secant_solve(&rachford_rice, &RR, V, tol);

		for(i=0;i<NPURE-1;i++){
			ys[i] = (zs[i] * Ks[i]) / ((1 - V[0]) + V[0]*Ks[i]);
			xs[i] = zs[i] / (1 + (V[0] * (Ks[i] - 1)));
		}
		ys[NPURE-1] = 1 - sum_elements(NPURE-1, ys); /* fractions should sum correctly */
		xs[NPURE-1] = 1 - sum_elements(NPURE-1, xs);

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
#endif

int cubic_solution(double coef[4], double *roots){
	double p, q, r,
		   a, b,
		   cond;

	p = coef[1]/coef[0];
	q = coef[2]/coef[0];
	r = coef[3]/coef[0];
	a = (3*q - pow(p, 2)) / 3;
	b = (2*pow(p, 3) - (9*p*q) + (27*r)) / 27;
	cond = (pow(b, 2) / 4) + (pow(a, 3) / 27);

#if 0
	printf("\n  <cubic_solution>: variables in this cubic-formula solution are:"
			"\n\tp = %.6g"
			"\n\tq = %.6g"
			"\n\tr = %.6g"
			"\n\ta = %.6g"
			"\n\tb = %.6g"
			"\n\tcond = %.6g"
			, p, q, r, a, b, cond);
#endif

	if(cond>0){
		roots[0] = cbrt(-b/2 + sqrt(cond)) + cbrt(-b/2 - sqrt(cond)) - p/3;
		return 1;
	}else if(cond<0){
		double R, phi;

		R = pow(b, 2) / (4 * (-pow(a, 3)/27));
		phi = acos(sqrt(R));

#if 0
		printf("\n  <cubic_solution>: there will be three roots"
				"\n\tR = %.6g"
				"\n\tphi = %.6g"
				, R, phi);
#endif

		int i;
		for(i=0;i<3;i++){
			if(b>0){
				roots[i] = -2 * sqrt(-a/3) * cos(phi/3 + (MIX_PI*2*i)/3) - p/3;
			}else{
				roots[i] = 2 * sqrt(-a/3) * cos(phi/3 + (MIX_PI*2*i)/3) - p/3;
			}
#if 0
			printf("\n\troot %u is"
					"\n\t  %.1g * sqrt(%.9g) * cos(%.9g + %.9g) ="
					"\n\t  %.1g * %.9g * %.9g = %.9g"
					, i, (b>0) ? -2.0 : 2.0, -a/3, phi/3, (MIX_PI * 2 * i)/3
					, (b>0) ? -2.0 : 2.0, sqrt(-a/3), cos(phi/3 + (MIX_PI*2*i)/3)
					, roots[i]);
#endif
		}
		return 3;
	}else{ /* cond equals zero */
		double A;

		A = sqrt(a/3) * ((b>0) ? -1 : 1);
		roots[0] = 2*A - p/3;
		roots[1] = -A - p/3;
		/* roots[2] = -A - p/3; */

		return 2;
	}
	return 0; /* error -- no roots returned */
}

double pengrob_phi_pure(PureFluid *PF, double T, double P, PhaseName type, FpropsError *err){
#define RR PF->data->R
#define PR PF->data->corr.pengrob
	unsigned z_num
#if 0
		, z_cons1 /* tests whether the expressions for Z have been derived properly... */
		, z_cons2 /* tests whether roots of cubic-form equations have been solved properly... */
#endif
		;
	double Tr       /* reduced temperature */
		, aT        /* parameter `a' at the temperature */
		, alpha     /* intermediate variable in calculating aT */
		, A         /* two variables */
		, B
		, z_coef[4] /* coefficients of cubic expression for compressibility */
		, Z_rt[3]   /* roots of cubic expression for compressibility */
		, Z
		, cpx_ln    /* rather involved part of the fugacity coefficient */
		/* , tol = 1.e-6 */
		;
	
	/* printf("\n  <pengrob_phi_pure>: Pressure is now %.0f Pa", P); */
	Tr = T / PF->data->T_c;
	alpha = (1 + PR->kappa * (1 - sqrt(Tr)));
	aT = PR->aTc * alpha * alpha;

	A = aT * P / (RR * RR * T * T);
	B = PR->b * P / (RR * T);
	/* if(type<=VAPOR){ */
		z_coef[0] = 1;
		z_coef[1] = B - 1;
		z_coef[2] = A - 2*B - 3*pow(B, 2);
		z_coef[3] = pow(B, 3) + pow(B, 2) - A*B;
	/* }else{ */
#if 0
		z_coef[0] = - 1./A;
		z_coef[1] = (1 - B)/A;
		z_coef[2] = (2*B + 3*pow(B, 2))/A;
		z_coef[3] = -(pow(B, 2) + pow(B, 3))/A + B;
/* #else */
		z_coef[0] = 1;
		z_coef[1] = 
		z_coef[2] = 
		z_coef[3] = 
#endif
	/* } */
	z_num = cubic_solution(z_coef, Z_rt);
	if(z_num==1){
		Z = Z_rt[0];
	}else{
		if(type<=VAPOR){
			Z = max_element(z_num, Z_rt);
		}else{
			if(min_positive_elem(&Z, z_num, Z_rt)){
				printf("\n  <pengrob_phi_pure>:" MIX_COMPR_ERROR " when calculating"
						"\n\tfugacity coefficient for %s phase; compressibility is %.6g"
						, (type<=VAPOR) ? "vapor/gas" : "liquid", Z);
				/* err = FPROPS_RANGE_ERROR; */
			}
		}
	}
#if 0
	printf("\n\t<pengrob_phi_pure>: There are %u roots of the equation"
			"\n\t  %.6g x^3 + %.6g x^2 + %.6g x + %.6g = 0 :"
			, z_num, z_coef[0], z_coef[1], z_coef[2], z_coef[3]);
	for(i=0;i<z_num;i++){
		printf("\n\t  %.6g", Z_rt[i]);
	}

	/* if un-commenting this portion, un-comment declaration of `tol' above, too */
	if(type<=VAPOR){
		z_cons1 = (fabs(Z - (1 + B - A * (Z - B)/(pow(Z, 2) + (2*Z*B) - pow(B, 2)))) < tol);
	}else{
		z_cons1 = (fabs(Z - (B + (pow(Z, 2) + (2*Z*B) - pow(B, 2))*(1 + B - Z)/A)) < tol);
	}
	z_cons2 = (fabs(z_coef[0]*pow(Z, 3) + z_coef[1]*pow(Z, 2) + z_coef[2]*Z + z_coef[3]) < tol);

	printf("\n\t<pengrob_phi_pure>: Compressibility for %s of %s is %.6g;"
			"\n\t  `A' is %.6g"
			"\n\t  `B' is %.6g"
			/* "\n\t  Derivation of Z %s consistent with original equations." */
			/* "\n\t  Value of Z %s consistent with cubic-form equation." */
			, (type<=VAPOR) ? "vapor/gas" : "liquid", PF->name, Z, A, B
			/* , (z_cons1) ? "is" : "is not", (z_cons2) ? "is" : "is not" */);
#endif
	cpx_ln = log( (Z + (1 + M_SQRT2)*B)/(Z + (1 - M_SQRT2)*B) );
	return exp( Z - 1 - log(Z - B) - (A / 2 / M_SQRT2) * cpx_ln );
#undef PR
#undef RR
}

/*
	Calculate the Poynting Factor.  This approximately accounts for the volume 
	deviation of a liquid in phase equilibrium that results from the pressure 
	effects due to not being at the saturation pressure.
 */
double poynting_factor(PureFluid *PF, double T, double P, FpropsError *err){
	double /* p, */
		   p_sat,
		   rho_liq, rho_vap,
		   ln_pf; /* natural logarithm of Poynting Factor */

	/* p = fprops_p((FluidState){T, rho, PF}, err); */

	fprops_sat_T(T, &p_sat, &rho_liq, &rho_vap, PF, err);

	ln_pf = (P - p_sat) / (rho_liq * PF->data->R * T);
	return exp(ln_pf);
}

/* Finding phase-equilibrium conditions */
/*
	Apportion components into either critical/supercritical or saturation/VLE 
	regions, and find saturation pressure, liquid and vapor densities for 
	species in the saturation region.  Return this data 
 */
#if 0
PhaseMixState *mixture_phase_prepare(MixtureSpec *MX, double T, double P, FpropsError *err){
	;
}
#endif

typedef struct DewBubbleData_Struct {
	MixtureSpec *MS;  /* components and mass fractions */
	double T;         /* temperature */
	double *p_sat;    /* saturation pressures for all components */
	// double *rho_v; /* vapor densities at dew pressure for all components */
	// double *rho_l; /* liquid densities at dew pressure for all components */
	double *rhos;     /* vapor-phase densities at dew pressure */
	double *sat_rhos; /* liquid saturation densities for all components */
	double tol;       /* tolerance to which to solve */
	FpropsError *err; /* error enumeration */
} DBData;

double dew_p_error(double P_D, void *user_data){
#define NPURE dbd->MS->pures
#define YS dbd->MS->Xs
#define PF dbd->MS->PF
#define TT dbd->T
#define PSAT dbd->p_sat
#define RHOS dbd->rhos
/* #define RHOV dbd->rho_v */
/* #define RHOL dbd->rho_l */
#define SATRHO dbd->sat_rhos
#define TOL dbd->tol
#define ERR dbd->err
	/* printf("\n  <dew_p_error>: Entered the function"); */
	unsigned i;
	double p_d_term,
		   rp_d = 0.0;

	DBData *dbd = (DBData *)user_data;
	/* printf("\n\t<dew_p_error>: Unpacked the user_data struct for mixture "
			"at %.2f K, %.0f Pa"
			, TT, P_D); */

	for(i=0;i<NPURE;i++){
#if 0
		p_d_term = YS[i] * pengrob_phi_pure(PF[i], TT, P_D, VAPOR, ERR);
		p_d_term /= pengrob_phi_pure(PF[i], TT, PSAT[i], LIQUID, ERR) * PSAT[i] * poynting_factor(PF[i], TT, P_D, ERR);
		rp_d += p_d_term;
#else
		rp_d += YS[i] * pengrob_phi_pure(PF[i], TT, P_D, VAPOR, ERR) / PSAT[i];
#endif
	}
	return P_D - 1./rp_d;
#if 0
#undef ERR
#undef TOL
#undef SATRHO
#undef RHOV
#undef RHOL
#undef PSAT
#undef TT
#undef PF
#undef YS
#undef NPURE
#else
#undef YS
#endif
}

double bubble_p_error(double P_B, void *user_data){
#define XS dbd->MS->Xs
	/* printf("\n  <bubble_p_error>: Entered the function"); */
	unsigned i;
	double p_b_term,
		   p_b = 0.0;

	DBData *dbd = (DBData *)user_data;
	/* printf("\n\t<bubble_p_error>: Unpacked the user_data struct for mixture "
			"at %.2f K, %.0f Pa"
			, TT, P_B); */

	for(i=0;i<NPURE;i++){
#if 0
		p_b_term = XS[i] * pengrob_phi_pure(PF[i], TT, PSAT[i], LIQUID, ERR) * PSAT[i] * poynting_factor(PF[i], TT, P_B, ERR);
		p_b_term /= pengrob_phi_pure(PF[i], TT, P_B, VAPOR, ERR);
		p_b += p_b_term;
#else
		p_b += XS[i] * PSAT[i] / pengrob_phi_pure(PF[i], TT, P_B, VAPOR, ERR);
#endif
	}
	return P_B - p_b;
#undef ERR
#undef TOL
#undef SATRHO
#undef RHOV
#undef RHOL
#undef PSAT
#undef TT
#undef PF
#undef XS
#undef NPURE
}

/*
	Find the dew-point pressure for a mixture at a given temperature
 */
double dew_pressure(MixtureSpec *MS, double T, FpropsError *err){
#define NPURE MS->pures
#define PF MS->PF
#define XS MS->Xs
	unsigned i;
	double p_d[2],
		   rp_d = 0.0;
	double p_sat[NPURE]
		, rho_l[NPURE]
		, rho_v[NPURE]
		, xs[NPURE];
	double tol = 1.e-5;
	mole_fractions(NPURE, xs, XS, PF);

	/* Find ideal-gas dew pressure as a starting point */
	for(i=0;i<NPURE;i++){
		/* printf("\n  <dew_pressure>: Calculating saturation pressure for "
				"substance %s (at index %u)"
				"\n\tfrom temperature T=%.2f K...", PF[i]->name, i, T); */
		fprops_sat_T(T, (p_sat+i), (rho_l+i), (rho_v+i), PF[i], err);
		rp_d += xs[i] / p_sat[i];
		/* printf("\n\tsaturation pressure was P_sat=%.0f Pa", p_sat[i]); */
	}
	p_d[0] = 1./rp_d;
	/* printf("\n  <dew_pressure>: Ideal-gas dew pressure is P_D = %.0f Pa", p_d[0]); */
	p_d[1] = 1.01/rp_d;

	DBData dbd = {
		.MS = MS
		, .T = T
		, .p_sat = p_sat
		, .rhos = rho_v
		, .sat_rhos = rho_l
		, .tol = tol
		, .err = err
	};
	/* printf("\n  <dew_pressure>: Created DBData struct"); */
	/* puts(""); */

	secant_solve(&dew_p_error, &dbd, p_d, tol);

	return p_d[0];
#undef XS
#undef PF
#undef NPURE
}

/*
	Find the bubble-point pressure for a mixture at a given temperature
 */
double bubble_pressure(MixtureSpec *MS, double T, FpropsError *err){
#define NPURE MS->pures
#define PF MS->PF
#define XS MS->Xs
	unsigned i;
	double p_b[] = {0.0, 0.0}
		, p_sat[NPURE]
		, rho_l[NPURE]
		, rho_v[NPURE]
		, xs[NPURE]
		, tol = 1.e-5;
	mole_fractions(NPURE, xs, XS, PF);

	/* Find ideal-gas bubble pressure as a starting point */
	for(i=0;i<NPURE;i++){
		/* printf("\n  <bubble_pressure>: Calculating saturation pressure for substance "
				"%s (at index %u)\n\tfrom temperature T=%.2f K...", PF[i]->name, i, T); */
		fprops_sat_T(T, (p_sat+i), (rho_l+i), (rho_v+i), PF[i], err);
		p_b[0] += xs[i] * p_sat[i];
		/* printf("\n\tsaturation pressure was P_sat=%.0f Pa;"
				"\n\t  since mole fraction is %.6f, bubble pressure is now %.0f Pa"
				, p_sat[i], xs[i], p_b[0]); */
	}
	/* printf("\n  <bubble_pressure>: Ideal-gas bubble pressure is P_B = %.0f Pa", p_b[0]); */
	p_b[1] = 1.01*p_b[0];

	DBData dbd = {
		.MS = MS
		, .T = T
		, .p_sat = p_sat
		, .rhos = rho_v
		, .sat_rhos = rho_l
		, .tol = tol
		, .err = err
	};
	/* printf("\n  <bubble_pressure>: Created DBData struct\n"); */

	secant_solve(&bubble_p_error, &dbd, p_b, tol);

	return p_b[0];
#undef XS
#undef PF
#undef NPURE
}

typedef struct FlashData_Struct {
	unsigned npure; /* number of components */
	double *zs;     /* overall mole fractions of each component */
	double *Ks;     /* K-factors for each component */
} FlashData;

double flash_error(double v_frac, void *user_data){
#define NPURE fd->npure
#define Z fd->zs[i]
#define K fd->Ks[i]
	unsigned i;
	double F_sum = 0.0;

	FlashData *fd = (FlashData *)user_data;

	for(i=0;i<NPURE;i++){
		F_sum += (Z * (K - 1)) / (1 + v_frac*(K - 1));
	}
	printf("\n  <flash_error>: When vapor mole fraction is %.6g, then sum for Flash is %.6g"
			, v_frac, F_sum);
	return F_sum;
#undef K
#undef Z
#undef NPURE
}

#if 0
void read_in_MixtureSpec(MixtureSpec **MS_array, MixtureSpec *MS_src, PhaseSpec *PS){
#define TITLE "<read_in_MixtureSpec>: "
#define NPHASE PS->phases
#define NCOMP PS->PH[i]->ncomps
#define PH_C PS->PH[i]->c
	unsigned i, j;
	FUNC_REPT("Entered the function");

	for(i=0;i<NPHASE;i++){
		MS_array[i] = ASC_NEW(MixtureSpec);

		MS_array[i]->pures = NCOMP;
		MS_array[i]->PF = ASC_NEW_ARRAY(PureFluid *,NCOMP);
		MS_array[i]->Xs = ASC_NEW_ARRAY(double,NCOMP);

		for(j=0;j<NCOMP;j++){
			MS_array[i]->PF[j] = MS_src->PF[PH_C[j]];
			MS_array[i]->Xs[j] = MS_src->Xs[PH_C[j]];
		}
	}
#undef PH_C
#undef NCOMP
#undef NPHASE
#undef TITLE
}
#endif

/*
	Find whether a mixture flashes (is in vapor-liquid equilibrium), and if so, 
	what are the characteristics of the flash (phases present, mass/mole 
	fraction in the vapor & liquid, mass/mole fraction of each component in each 
	phase, etc.)
 */
void mixture_flash(PhaseSpec *PS, MixtureSpec *MS, double T, double P, FpropsError *err){
#define TITLE "<mixture_flash>: "
#define NPURE MS->pures
#define MXS MS->Xs
#define MPF MS->PF
#define D MPF[i]->data
#define NPHASE PS->phases
#define PTYPE PS->ph_type
#define PFRAC PS->ph_frac
#define PPH PS->PH
/* #define PH_C(IX,IY) PPH[ IX ]->c[ IY ] */
/* #define PH_X(IX,IY) PPH[ IX ]->Xs[ IY ] */
	FUNC_REPT("Entered the function");
	unsigned i, j
		, i_v = 0   /* indices of vapor, liquid, supercritical phases in PS */
		, i_l = 0
		, i_sc = 0
		;
	int flag_sc = 0 /* whether encountered any supercritical or subcritical components */
		, flag_vle = 0
		;
	double p_b
		, p_d
		, xs_vle[NPURE]
		, xs_sc[NPURE]
		, xs_phase[2]
		, mm[2]
		/* , phi_l[NPURE] */
		, phi_v[NPURE]
		, p_sat[NPURE]
		/* , pf_l[NPURE] */
		, rho_d[2] /* a `dummy' density used in finding the saturation pressure */
		, K[NPURE]
		, tol = MIX_XTOL
		;

	PTYPE = ASC_NEW_ARRAY(PhaseName,3);
	PFRAC = ASC_NEW_ARRAY(double,3);
	PPH = ASC_NEW_ARRAY(Phase *,3);
	for(i=0;i<3;i++){
		PPH[i] = ASC_NEW(Phase);
		PPH[i]->ncomps = 0;
		PPH[i]->c  = ASC_NEW_ARRAY(unsigned,NPURE);
		PPH[i]->Xs = ASC_NEW_ARRAY(double,NPURE);
		PPH[i]->xs = ASC_NEW_ARRAY(double,NPURE);
		PPH[i]->PF = ASC_NEW_ARRAY(PureFluid *,NPURE);
	}
	NPHASE = 0;

	/* FUNC_REPT("Declared all variables..."); */

	for(i=0;i<NPURE;i++){
		if(T<D->T_c){
			/*
				Current component is subcritical.  If no subcritical component 
				was encountered previously, set the index of the subcritical 
				phase equal to the current number of phases (in last place) and 
				increment the number of phases by one.
				
				Set the type of the phase to VAPOR, set the next component index 
				for the subcritical phase to the current index 'i', copy the 
				mass fraction in the subcritical phase from the current 
				component in the mixture specification 'MS', and increment the 
				number of components in the subcritical phase.  Confirm that a 
				subcritical phase has been encountered, by setting 'flag_vle'.
			 */
			i_v += (flag_vle) ? 0 : NPHASE;
			NPHASE += (flag_vle) ? 0 : 1;

			/* printf("\n  <mixture_flash>: index is %u, substance is %s, flag_vle is %i"
					, i_v, MPF[i]->name, flag_vle); */

			PTYPE[i_v] = VAPOR;
			PPH[i_v]->c[PPH[i_v]->ncomps] = i;
			PPH[i_v]->Xs[PPH[i_v]->ncomps] = MXS[i];
			PPH[i_v]->PF[PPH[i_v]->ncomps] = MPF[i];

			PPH[i_v]->ncomps ++;

			flag_vle = 1;
			/* printf("\n\tnow index is %u, flag_vle is %i, NPHASE is %u"
					, i_v, flag_vle, NPHASE); */
		}else{
			/*
				Current component is supercritical.  Operations are analogous to 
				subcritical condition discussed above.
			 */
			i_sc += (flag_sc) ? 0 : NPHASE;
			NPHASE += (flag_sc) ? 0 : 1;

			/* printf("\n  <mixture_flash>: index is %u, substance is %s, flag_sc is %i"
					, i_sc, MPF[i]->name, flag_sc); */

			PTYPE[i_sc] = SUPERCRIT;
			PPH[i_sc]->c[PPH[i_sc]->ncomps] = i;
			PPH[i_sc]->Xs[PPH[i_sc]->ncomps] = MXS[i];
			PPH[i_sc]->PF[PPH[i_sc]->ncomps] = MPF[i];
			PPH[i_sc]->ncomps ++;

			flag_sc = 1;
			/* printf("\n\tnow index is %u, flag_sc is %i, NPHASE is %u"
					, i_sc, flag_sc, NPHASE); */
		}
	}
#if 0
	printf("\n  <mixture_flash>: There are %u phases", NPHASE);
	for(i=0;i<NPHASE;i++){
		printf("\n\tPhase number %u is %s"
				, i, (PTYPE[i]==SUPERCRIT) ? "supercritical" : "subcritical" );
	}
	puts("");
#endif

	/*
		Rearrange order of supercritical/subcritical phases, and determine mass 
		fractions of the mixture that are in any supercritical phase.
	 */
	if(NPHASE==2){
		if(PTYPE[0]==VAPOR){
			/* FUNC_REPT("Switching phase structs"); */
			PTYPE[0] = SUPERCRIT;
			PTYPE[1] = VAPOR;
			/*	Rearrange indices that refer to the number of the supercritical 
				and vapor phases */
			i_sc = 0;
			i_v = 1;
			Phase *A = PPH[0]; /* holds reference to phase[0] */
			PPH[0] = PPH[1];
			PPH[1] = A;
		}
		if(PTYPE[0]!=SUPERCRIT || PTYPE[1]!=VAPOR){
			printf("\n  <mixture_flash>: ERROR -- two phases, but types are "
					"%i and %i, not VAPOR and SUPERCRIT.\n"
					, PTYPE[0], PTYPE[1]);
		}

		/*
			Determine values of 'PFRAC', the fraction of system mass in each phase
		 */
		PFRAC = ASC_NEW_ARRAY(double,2);
		for(i=0;i<PPH[i_sc]->ncomps;i++){
			PFRAC[i_sc] += PPH[i_sc]->Xs[i]; /* supercritical phase */
		}
		PFRAC[i_v] = 1.0 - PFRAC[i_sc];       /* subcritical phase(s) */
		FUNC_REPT("For the current mixture");

		/* mixture_x_props(PPH[i_v]->ncomps, PPH[i_v]->Xs, PPH[i_v]->Xs); */
		for(i=0;i<NPHASE;i++){ 
			mixture_x_props(PPH[i]->ncomps, PPH[i]->Xs, PPH[i]->Xs);
			mole_fractions(PPH[i]->ncomps, PPH[i]->xs, PPH[i]->Xs, PPH[i]->PF);
		}
		mm[0] = mixture_M_avg(PPH[0]->ncomps, PPH[0]->Xs, PPH[0]->PF);
		mm[1] = mixture_M_avg(PPH[1]->ncomps, PPH[1]->Xs, PPH[1]->PF);

		printf("\n\tsupercritical mass fraction is %.6g"
				"\n\tsubcritical mass fraction is %.6g"
				"\n\tsupercritical molar mass is %.6g"
				"\n\tsubcritical molar mass is %.6g"
				, PFRAC[i_sc], PFRAC[i_v], mm[i_sc], mm[i_v]);

		xs_phase[0] = PFRAC[0] / mm[0] / ((PFRAC[0] / mm[0]) + (PFRAC[1] / mm[1]));
		xs_phase[1] = PFRAC[1] / mm[1] / ((PFRAC[0] / mm[0]) + (PFRAC[1] / mm[1]));

		PFRAC[0] = xs_phase[0];
		PFRAC[1] = xs_phase[1];

		printf("\n\tsupercritical mole fraction is %.6g"
				"\n\tsubcritical mole fraction is %.6g"
				, PFRAC[i_sc], PFRAC[i_v]);
	}else if(NPHASE>=3){
		FUNC_REPT("ERROR -- more than two phases occurred (there should be only up to"
				"\n\ttwo, a supercritical and subcritical).");
	}else{
		PFRAC[0] = 1.0;
	}

#if 0
	printf("\n  <mixture_flash>: There are %u phases", NPHASE);
	for(i=0;i<NPHASE;i++){
		printf("\n\tPhase number %u is %s"
				"\n\t  this phase has components"
				, i, (PTYPE[i]==SUPERCRIT) ? "supercritical" : "subcritical" );
		for(j=0;j<PPH[i]->ncomps;j++){
			printf("\n\t\t%s", PPH[i]->PF[j]->name);
		}
	}
	puts("");
#endif

	/*
		Determine bubble pressure and dew pressure of any subcritical mixture
	 */
	if(NPHASE==2 || PTYPE[0]==VAPOR){
		/*
			Obtain mole fractions for vapor phase -- these represent the overall 
			mole fractions in all subcritical phases, since vapor phase is being 
			used as an alias for all subcritical phases considered together.
		 */
		/* mole_fractions(PPH[i_v]->ncomps, xs_vle, PPH[i_v]->Xs, PPH[i_v]->PF); */

		/*
			Create a new array of MixtureSpec structures to hold PureFluid 
			structures for supercritical and subcritical phases.
		 */
		MixtureSpec MS_temp = {
			PPH[i_v]->ncomps
			/* , xs_vle */
			, PPH[i_v]->xs
			, PPH[i_v]->PF
		};
		p_b = bubble_pressure(&MS_temp, T, err);
		p_d = dew_pressure(&MS_temp, T, err);

#if 0
		printf("\n  <mixture_flash>: bubble pressure is %.1f Pa"
				"\n\tdew pressure is %.1f Pa"
				, p_b, p_d);
		for(i=0;i<PPH[i_v]->ncomps;i++){
			printf("\n\tmole fraction of subcritical component %s is %.6f"
					, /* MS->PF[PH_C(i_v,i)]->name */ PPH[i_v]->PF[i]->name, xs_vle[i]);
		}
		puts("");
#endif

		/*
			If the system pressure is above the bubble pressure, subcritical 
			components are all in the liquid; if pressure is below dew pressure, 
			system components are all in the vapor; and if pressure is between 
			bubble and dew pressures, they are in vapor-liquid equilibrium.
		 */
		if(P > p_b){
			i_l = i_v;
			PTYPE[i_l] = LIQUID;
			/* PPH[i_l]->xs = xs_vle; */
		}else if(P < p_d){
			/* PPH[i_v]->xs = xs_vle; */
			;
		}else{
			NPHASE = 3;
			i_l = i_v + 1;
			PTYPE[i_l] = LIQUID;

			/*
				Read each component in the vapor phase into the liquid phase as 
				well.  Find the saturation temperature, vapor-phase fugacity 
				coefficient, and K-factor to use in calculating liquid and vapor 
				mole fractions.
			 */
			/* FUNC_REPT("Calculating K-factors..."); */
			for(i=0;i<PPH[i_v]->ncomps;i++){
				fprops_sat_T(T, (p_sat+i), (rho_d), (rho_d+1), PPH[i_v]->PF[i], err);
				phi_v[i] = pengrob_phi_pure(PPH[i_v]->PF[i], T, p_b, VAPOR, err);
				K[i] = p_sat[i] / (phi_v[i] * P);
				/* printf("\n\tK-factor for %s is %.6g"
						, PPH[i_v]->PF[i]->name, K[i]); */
			}
			FlashData FD = {PPH[i_v]->ncomps, PPH[i_v]->xs, K};
			double V[] = {0.5, 0.51};

			secant_solve(&flash_error, &FD, V, tol);

			/* PPH[i_v]->xs = ASC_NEW_ARRAY(double,PPH[i_v]->ncomps);
			PPH[i_l]->xs = ASC_NEW_ARRAY(double,PPH[i_v]->ncomps); */
			PPH[i_l]->ncomps = PPH[i_v]->ncomps;
			PPH[i_l]->c = PPH[i_v]->c;
			PPH[i_l]->PF = PPH[i_v]->PF;

			PFRAC[i_l] = PFRAC[i_v] * (1 - V[0]);
			PFRAC[i_v] *= V[0];

			for(i=0;i<PPH[i_v]->ncomps;i++){
				/* PPH[i_v]->xs[i] = xs_vle[i] * K[i] / (1 + V[0]*(K[i] - 1)); */
				PPH[i_l]->xs[i] = PPH[i_v]->xs[i] / (1 + V[0]*(K[i] - 1));
				PPH[i_v]->xs[i] *= K[i] / (1 + V[0]*(K[i] - 1));
			}
		}
	}
	if(PTYPE[0]==SUPERCRIT){
		mole_fractions(PPH[i_sc]->ncomps, PPH[i_sc]->xs, PPH[i_sc]->Xs, PPH[i_sc]->PF);
	}

#if 1
	printf("\n  <mixture_flash>: At temperature %.2f K and pressure %.0f Pa, there are %u phases;"
			, T, P, NPHASE);
	for(i=0;i<NPHASE;i++){
		printf("\n\t%.6g of total moles are in %s phase"
				, PFRAC[i], (PTYPE[i]==SUPERCRIT) ? "supercritical" :
				(PTYPE[i]==VAPOR) ? "vapor" : "liquid");
		for(j=0;j<PPH[i]->ncomps;j++){
			printf("\n\t  mole fraction of %s in this phase is %.6g"
					, PPH[i]->PF[j]->name, PPH[i]->xs[j]);
		}
	}
	if(NPHASE>=2){
		printf("\n\tbubble pressure is %.1f Pa"
				"\n\tdew pressure is %.1f Pa"
				, p_b, p_d);
	}
	puts("");
#endif

/* #undef PH_X */
/* #undef PH_C */
#undef PPH
#undef PFRAC
#undef PTYPE
#undef NPHASE
#undef D
#undef MXS
#undef NPURE
#undef TITLE
}

/* Functions to test/demonstrate other functionality */
void test_five(double T, double P){
	unsigned i;
#define NPURE 4
#define D MS->PF[i]->data
#define TRIALS 5
	char *fluids[] = {
		"isohexane", "krypton", "carbonmonoxide", "ammonia", "water"
	};
	char *fluid_names[] = {
		"isohexane", "krypton", "carbon monoxide", "ammonia", "water"
	};
	double Xs[] = {0.55, 0.20, 0.10, 0.15, 0.10};
	char *src[] = {
		NULL, NULL, NULL, NULL, NULL
	};
	FpropsError err = FPROPS_NO_ERROR;
	MixtureError merr = MIXTURE_NO_ERROR;

	MixtureSpec *MS = ASC_NEW(MixtureSpec);
	MS->pures = NPURE;
	MS->Xs = Xs;
	MS->PF = ASC_NEW_ARRAY(PureFluid *,NPURE);
	mixture_fluid_spec(MS, NPURE, (void *)fluids, "pengrob", src, &merr);

	/* for(i=0;i<NPURE;i++){ 
		printf("\n%s T_c = %.2f K, P_c = %.0f Pa", MS->PF[i]->name, D->T_c, D->p_c);
	} */

	/* declare variables used when finding phase equilibrium */
	/* double tol = 1.e-5; */
	double p_sat[NPURE] = {0.0},
		   T_sat[NPURE] = {0.0};
	double rho_l[NPURE] = {0.0},  /* liquid-phase densities */
		   rho_v[NPURE] = {0.0},  /* vapor-phase densities */
		   /* rho_sc[NPURE] = {0.0}, */ /* supercritical densities */
		   rho_dummy = 0.0;
	double p_b[TRIALS] = {0.0},   /* bubble pressure */
		   p_d[TRIALS] = {0.0};   /* dew pressure */
	double x_vle[NPURE]           /* overall mole fractions in vapor-liquid equilibrium */
		   /* , xs[4][NPURE] */;        /* per-phase mole fractions */
	double x_sum=0.0;             /* sum of mole fractions */
	double Ts[] = {
		270
		, 310
		, 350
		, 390
		, 430
		, 470
	};

	MixtureSpec *MS_vle = ASC_NEW(MixtureSpec);
#define VPURE MS_vle->pures
#define VXS MS_vle->Xs
#define VPF MS_vle->PF
	VPURE = 0;
	VXS = ASC_NEW_ARRAY(double,NPURE);
	VPF = ASC_NEW_ARRAY(PureFluid *,NPURE);

	MixtureSpec *MS_sc = ASC_NEW(MixtureSpec);
#define SPURE MS_sc->pures
#define SXS MS_sc->Xs
#define SPF MS_sc->PF
	SPURE = 0;
	SXS = ASC_NEW_ARRAY(double,NPURE);
	SPF = ASC_NEW_ARRAY(PureFluid *,NPURE);

	for(i=0;i<NPURE;i++){
		if(T < D->T_c){
			/*
				We want to find saturation pressure, and densities to use in 
				calculating compressibilities later on.  Liquid density is at 
				(T, p_sat), and vapor density is at (T, P).  So the liquid 
				density that comes out of fprops_sat_T is correct, but the vapor 
				density may be too high.  We find a more reasonable vapor 
				density from fprops_sat_p, and can search from there.
			 */
			printf("\n  Adding substance %s to vapor-liquid equilibrium", fluid_names[i]);
			VPF[VPURE] = MS->PF[i]; /* add new component and mass fraction, in next place */
			VXS[VPURE] = MS->Xs[i];
			VPURE ++;               /* increment number of pures in VLE */
		}else{
			printf("\n  Adding substance %s to supercritical phase", fluid_names[i]);
			SPF[SPURE] = MS->PF[i]; /* add new component and mass fraction, in next place */
            SXS[SPURE] = MS->Xs[i];
			SPURE ++;               /* increment number of pures in supercritical condition */
		}
		printf("\n  VPURE = %u, and SPURE = %u; i = %u", VPURE, SPURE, i);
	}
	for(i=0;i<VPURE;i++){
		fprops_sat_T(T, (p_sat+i), (rho_l+i), (rho_v+i), VPF[i], &err);
		fprops_sat_p(P, (T_sat+i), &rho_dummy, (rho_v+i), VPF[i], &err);

		x_vle[i] /= x_sum;
	}

	/*
	for(i=0;i<NPURE;i++){
		if(rho_v[i]!=0 && rho_l[i]!=0){
			printf("\n  Saturation pressure for %s at T=%.2f K is %.0f Pa;"
					"\n\tliquid density is %.6g kg/m^3"
					"\n  Saturation temperature at P=%.0f Pa is %.2f K"
					"\n\tvapor density is %.6g kg/m^3"
					"\n  Poynting Factor is %.6g"
					"\n\tliquid-phase fugacity coefficient is %.6g"
					"\n\tvapor-phase fugacity coefficient is %.6g\n"
					"\n\tmole fraction is %.6g\n"
					, fluid_names[i] , T, p_sat[i], rho_l[i]
					, P, T_sat[i], rho_v[i]
					, pf_l[i], phi_l[i], phi_v[i], x_vle[i]);
		}else{
			printf("\n  Supercritical density for %s is %.6g kg/m^3;"
					"\n\tthe calculated pressure from this density is %.0f Pa\n"
					, fluid_names[i], rho_sc[i]
					, fprops_p((FluidState){T, rho_sc[i], MS->PF[i]}, &err));
		}
	}
	printf("\n  Bubble pressure is %.0f Pa,"
			"\n  Reciprocal dew pressure is %.6g Pa^-1, and dew pressure is %.0f Pa\n"
			, p_b, rp_d, p_d);
	*/
	for(i=0;i<VPURE;i++){
		printf("\n  Saturation pressure for %s at T=%.2f K is %.0f Pa;"
				"\n\tliquid density is %.6g kg/m^3"
				"\n  Saturation temperature at P=%.0f Pa is %.2f K"
				"\n\tvapor density is %.6g kg/m^3"
				"\n"
				, fluid_names[i], T, p_sat[i], rho_l[i]
				, P, T_sat[i], rho_v[i]);
	}

	for(i=0;i<TRIALS;i++){
		p_d[i] = dew_pressure(MS_vle, Ts[i], &err);
		p_b[i] = bubble_pressure(MS_vle, Ts[i], &err);
	}
	/* p_d = dew_pressure(MS_vle, T, &err);
	p_b = bubble_pressure(MS_vle, T, &err);
	printf("\n  Dew pressure is %.0f Pa"
			"\n  Bubble pressure is %.0f Pa"
			, p_d, p_b);
	puts(""); */
	for(i=0;i<TRIALS;i++){
		printf("\n  At temperature %.2f K"
				"\n\tDew pressure is %.0f Pa"
				"\n\tBubble pressure is %.0f Pa"
				, Ts[i], p_d[i], p_b[i]);
	}
	puts("");
#undef SPF
#undef SXS
#undef SPURE
#undef VPF
#undef VXS
#undef VPURE

#undef D
#undef NPURE
}

void test_six(void){
#define NPURE 2
#define C_PURE 0
#define NROWS 14
#define NCOLS 9
#define D MS->PF[i]->data
	unsigned i1=0,i2=0,i3=0;
	double T,rho;
	char *fluids[] = {
		"isohexane", "ammonia"
	};
	/* char *fluid_names[] = {
		"isohexane", "ammonia"
	}; */
	double Xs[] = {0.55, 0.20, 0.10, 0.15, 0.10};
	char *src[] = {
		NULL, NULL, NULL, NULL, NULL
	};
	FpropsError err = FPROPS_NO_ERROR;
	MixtureError merr = MIXTURE_NO_ERROR;

	MixtureSpec *MS = ASC_NEW(MixtureSpec);
	MS->pures = NPURE;
	MS->Xs = Xs;
	MS->PF = ASC_NEW_ARRAY(PureFluid *,NPURE);
	mixture_fluid_spec(MS, NPURE, (void *)fluids, "pengrob", src, &merr);

	double P[NROWS][NCOLS];
	char *heads[NCOLS]
		, *sides[NROWS+1]
#if 0
		, *forms[NROWS] = {"%.6g"}
		, *conts[NROWS+1][NCOLS+1]
		;
	unsigned widths[NCOLS]
		, temp_len /* temporary string length */
#endif
		;
	sides[0] = "";

	for(rho=1.0,i1=0;rho<5200.0;rho*=1.9,i1++){
#if 0
		printf("\n  Modeling substance %s at density %.6g kg/m^3", MS->PF[C_PURE]->name, rho);
		if((temp_len = asprintf((sides+i1+1), "rho=%.2f", rho)) < 1 || temp_len > 100){
			sides[i1] = "";
		}
		printf("\n  length of output string was %u", temp_len);
#endif
		asprintf((sides+i1+1), "rho=%.2f", rho);

		for(T=100.0,i2=0;T<=500.0;T+=50.0,i2++){
			/* printf("\n\tat temperature %.2f K", T); */
			if(i1==0){
#if 0
				if((temp_len = asprintf((heads+i2), "T=%.6g", T)) < 1 || temp_len > 100){
					heads[i2] = "";
				}
				printf("\n\t  length of output string was %u", temp_len);
#endif
				asprintf((heads+i2), "T=%.6g", T);
			}

			P[i1][i2] = fprops_p((FluidState){T, rho, MS->PF[C_PURE]}, &err);
			/* printf("\n\t  pressure is %.6g at i1=%u, i2=%u, T=%.2f, rho=%.6g, err=%i"
					, P[i1][i2], i1, i2, T, rho, err); */
			if(err==FPROPS_RANGE_ERROR){
				/* printf("\n  Error in range"); */
			}
			err = FPROPS_NO_ERROR;
		}
	}

	/* PREPARE_TABLE(NROWS+1,NCOLS+1,heads,sides,P,forms,conts); */
	for(i1=0;i1<NROWS;i1++){
		/* printf("\n %s\t%s\t", sides[i1+1]); */
		for(i2=0;i2<NCOLS;i2++){
			printf("\n%s\t%s\t%.6g", sides[i1+1], heads[i2], P[i1][i2]);
		}
	}
	puts("");
	/* PRINT_STR_TABLE(NROWS+1,NCOLS+1,widths,conts); */

#undef D
#undef NPURE
}

void test_seven(void){
	unsigned j, i;
#define TITLE "<test_seven>: "
#define NPURE 4
#define D MS->PF[i]->data
#define TEMPS 5
#define PRESSURES 1
	char *fluids[] = {
		"isohexane", "krypton", "carbonmonoxide", "ammonia", "water"
	};
	char *fluid_names[] = {
		"isohexane", "krypton", "carbon monoxide", "ammonia", "water"
	};
	double props[] = {11, 4, 2, 3, 2};
	double Xs[NPURE];
	mixture_x_props(NPURE, Xs, props);
	char *src[] = {
		NULL, NULL, NULL, NULL, NULL
	};
	FpropsError err = FPROPS_NO_ERROR;
	MixtureError merr = MIXTURE_NO_ERROR;

	MixtureSpec *MS = ASC_NEW(MixtureSpec);

#if 0
	MixtureSpec **MS_vle = ASC_NEW_ARRAY(MixtureSpec *,TEMPS);
	MixtureSpec **MS_sc = ASC_NEW_ARRAY(MixtureSpec *,TEMPS);
#define VPURE MS_vle[j]->pures
#define VXS   MS_vle[j]->Xs
#define VPF   MS_vle[j]->PF
#define SPURE MS_sc[j]->pures
#define SXS   MS_sc[j]->Xs
#define SPF   MS_sc[j]->PF

	PhaseSpec **PS = ASC_NEW_ARRAY(PhaseSpec *,TEMPS);
#define NPHASE PS[j]->phases
#define PTYPE PS[j]->ph_type
#define PFRAC PS[j]->ph_frac
#define PPH PS[j]->PH
#define PXS(IX) PPH[ IX ]->xs
#endif

	MS->pures = NPURE;
	MS->Xs = Xs;
	MS->PF = ASC_NEW_ARRAY(PureFluid *,NPURE);
	mixture_fluid_spec(MS, NPURE, (void *)fluids, "pengrob", src, &merr);

#if 0
	double tol = 1.e-7
		, p_b[TEMPS] = {0.0}
		, p_d[TEMPS] = {0.0}
		, xs_vle[NPURE]
		, xs_sc[NPURE]
		, phi_l[TEMPS][NPURE]
		, phi_v[TEMPS][NPURE]
		, rho_d[] = {0.0, 0.0}
		, p_sat[TEMPS][NPURE]
		, pf_l[TEMPS][NPURE]
		, K[NPURE] = {0.0}
		;
#endif
	double Ts[] = {270, 310, 350, 390, 430, 470}
		/* , P = 1.5e5 */
		, Ps[] = {2.5e5}
		;

	FUNC_REPT("Declared all variables...");
#if 0
	for(j=0;j<TEMPS;j++){
		printf("\n  <test_seven>: %s %.2f %s", 
				"Preparing all variables for temperture", Ts[j], "K...\n");

		MS_vle[j] = ASC_NEW(MixtureSpec);
		VPURE = 0;
		VXS   = ASC_NEW_ARRAY(double,NPURE);
		VPF   = ASC_NEW_ARRAY(PureFluid *,NPURE);

		MS_sc[j] = ASC_NEW(MixtureSpec);
		SPURE = 0;
		SXS   = ASC_NEW_ARRAY(double,NPURE);
		SPF   = ASC_NEW_ARRAY(PureFluid *,NPURE);

		PS[j] = ASC_NEW(PhaseSpec);
		NPHASE = 0;
		PTYPE = ASC_NEW_ARRAY(PhaseName,3);
		PFRAC = ASC_NEW_ARRAY(double,3);
		PPH = ASC_NEW_ARRAY(Phase *,3);
		for(i=0;i<3;i++){
			PPH[i] = ASC_NEW(Phase);
		}

		FUNC_REPT("Prepared all variables");

		for(i=0;i<NPURE;i++){
			if(Ts[j]<D->T_c){
				VPF[VPURE] = MS->PF[i];
				VXS[VPURE] = MS->Xs[i];
				VPURE ++;
			}else{
				NPHASE = 1;
				PTYPE[0] = SUPERCRIT;
				SPF[SPURE] = MS->PF[i];
				SXS[SPURE] = MS->Xs[i];
				SPURE ++;
			}
		}
		mixture_x_props(VPURE, VXS, VXS);
		mixture_x_props(SPURE, SXS, SXS);
		mole_fractions(VPURE, xs_vle, VXS, VPF);
		mole_fractions(SPURE, xs_sc, SXS, SPF);
		p_b[j] = bubble_pressure(MS_vle[j], Ts[j], &err);
		p_d[j] = dew_pressure(MS_vle[j], Ts[j], &err);

		if(P > p_b[j]){
			NPHASE = 2;
			PTYPE[1] = LIQUID;
		}else if(P < p_d[j]){
			NPHASE = 2;
			PTYPE[1] = VAPOR;
		}else{
			NPHASE = 3;
			PTYPE[1] = VAPOR;
			PTYPE[2] = LIQUID;
		}

		for(i=0;i<VPURE;i++){
			fprops_sat_T(Ts[j], (p_sat[j]+i), (rho_d), (rho_d+1), VPF[i], &err);

			/* phi_l[j][i] = pengrob_phi_pure(VPF[i], Ts[j], p_sat[j][i], LIQUID, &err); */
			phi_l[j][i] = pengrob_phi_pure(VPF[i], Ts[j], p_b[j], LIQUID, &err);
			pf_l[j][i] = poynting_factor(VPF[i], Ts[j], p_b[j], &err);

			phi_v[j][i] = pengrob_phi_pure(VPF[i], Ts[j], p_b[j], VAPOR, &err);
		}

		if(NPHASE==3){
			FUNC_REPT("Calculating K-factors...");
			for(i=0;i<VPURE;i++){
				K[i] = p_sat[j][i] / (phi_v[j][i] * P);
				printf("\n\tK-factor for %s is %.6g"
						, VPF[i]->name, K[i]);
			}
			FlashData FD = {
				VPURE
				, xs_vle
				, K
			};
			double V[] = {0.5, 0.51};
			PXS(1) = ASC_NEW_ARRAY(double, VPURE);
			PXS(2) = ASC_NEW_ARRAY(double, VPURE);

			secant_solve(&flash_error, &FD, V, tol);

			PFRAC[1] = V[0];
			PFRAC[2] = 1 - V[0];
			for(i=0;i<VPURE;i++){
				PXS(1)[i] = xs_vle[i] * K[i] / (1 + V[0]*(K[i] - 1));
				PXS(2)[i] = xs_vle[i] / (1 + V[0]*(K[i] - 1));
			}
		}
		printf("\n  <test_seven>: %s %.2f K",
				"Found mixture conditions (mass and mole fractions, bubble and "
				"dew pressures) at temperature", Ts[j]);
	}
	for(j=0;j<TEMPS;j++){
		printf("\n  At temperature %.2f K"
				"\n\tDew pressure is %.0f Pa"
				"\n\tBubble pressure is %.0f Pa"
				"\n\tAt bubble pressure --"
				, Ts[j], p_d[j], p_b[j]);
		for(i=0;i<VPURE;i++){
			printf("\n\t  liquid-phase fugacity coefficient for substance %s is %.6g"
					"\n\t  liquid-phase Poynting Factor for %s is %.6g"
					"\n\t  vapor-phase fugacity coefficient for %s is %.6g"
					"\n\t  overall vapor/liquid mass fraction for %s is %.6g"
					/* "\n\t  overall vapor/liquid mole fraction for %s is %.6g" */
					"\n"
					, VPF[i]->name, phi_l[j][i]
					, VPF[i]->name, pf_l[j][i]
					, VPF[i]->name, phi_v[j][i]
					, VPF[i]->name, VXS[i]
					/* , VPF[i]->name, pf_l[j][i] */
					);
		}
		if(NPHASE==3){
			printf("\n\tAt the given pressure %.0f Pa, mixture is in VLE; mole splits are:"
					"\n\t  %.6f of total moles are in the vapor, %.6f in the liquid"
					, P, PFRAC[1], PFRAC[2]);
			for(i=0;i<VPURE;i++){
				printf("\n\t  %.6f of vapor is %s, %.6f of liquid is %s"
						, PXS(1)[i], VPF[i]->name, PXS(2)[i], VPF[i]->name);
			}
		}else{
			printf("\n\tAt the given pressure %.0f Pa, mixture of %s ", P, VPF[0]->name);
			for(i=1;i<VPURE;i++){
				printf("and %s ", VPF[i]->name);
			}
			printf("is entirely in the %s phase"
					, (PTYPE[1]==VAPOR) ? "vapor" : "liquid");
		}
	} puts("");
#endif

	PhaseSpec ***PS_2 = ASC_NEW_ARRAY(PhaseSpec **,TEMPS);
	for(i=0;i<TEMPS;i++){
		PS_2[i] = ASC_NEW_ARRAY(PhaseSpec *,PRESSURES);
		for(j=0;j<PRESSURES;j++){
			PS_2[i][j] = ASC_NEW(PhaseSpec);
			mixture_flash(PS_2[i][j], MS, Ts[i], Ps[j], &err);
		}
	}

#undef PPH
#undef PFRAC
#undef PTYPE
#undef NPHASE
#define NPHASE PS_2[i1][i2]->phases
#define PFRAC PS_2[i1][i2]->ph_frac
#define PTYPE PS_2[i1][i2]->ph_type
#define PPH PS_2[i1][i2]->PH
	/* FUNC_REPT("blip 1"); */
	unsigned i1, i2;
	for(i1=0;i1<TEMPS;i1++){ 
		for(i2=0;i2<PRESSURES;i2++){ 
			printf("\n  " TITLE "At temperature %.2f K and pressure %.0f Pa, "
					"there are %u phases;"
					, Ts[i1], Ps[i2], NPHASE);
			/* FUNC_REPT("blip 2"); */
			for(i=0;i<NPHASE;i++){
				/* FUNC_REPT("blip 3"); */
				/* FUNC_REPT("blip 4"); */
				printf("\n\t%.6g of total moles are in %s phase"
						, PFRAC[i], (PTYPE[i]==SUPERCRIT) ? "supercritical" :
						(PTYPE[i]==VAPOR) ? "vapor" : "liquid");
				/* FUNC_REPT("blip 5"); */
				for(j=0;j<PPH[i]->ncomps;j++){
					printf("\n\t  mole fraction of %s in this phase is %.6g"
							, PPH[i]->PF[j]->name, PPH[i]->xs[j]);
				}
			}
#if 0
			if(NPHASE>=2){
				printf("\n\tbubble pressure is %.1f Pa"
						"\n\tdew pressure is %.1f Pa"
						, p_b, p_d);
			}
#endif
			puts("");
		}
	}
#undef PPH
#undef PTYPE
#undef PFRAC
#undef NPHASE

#undef PXS
#undef SPF
#undef SXS
#undef SPURE
#undef VPF
#undef VXS
#undef VPURE
#undef PRESSURES
#undef TEMPS
#undef D
#undef NPURE
#undef TITLE
}
