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

extern const EosData eos_rpp_nitrogen;
extern const EosData eos_rpp_ammonia;
extern const EosData eos_rpp_carbon_dioxide;
extern const EosData eos_rpp_methane;
extern const EosData eos_rpp_water;

/* Function prototypes */
SecantSubjectFunction rachford_rice;
SecantSubjectFunction pressure_rho_error;
SecantSubjectFunction dew_p_error;
SecantSubjectFunction bubble_p_error;
void transit_non_physical(double T, double *rhos, const PureFluid *PF, FpropsError *err, int from_vap);
double rho_from_pressure(double T, double p, double rho_start, double tol, const PureFluid *PF, FpropsError *err);
double dew_pressure(MixtureSpec *MS, double T, FpropsError *err);
double bubble_pressure(MixtureSpec *MS, double T, FpropsError *err);

PhaseMixState *mixture_phase_prepare(MixtureSpec *MX, double T, double P, FpropsError *err);
void mixture_flash(MixturePhaseState *M, double P, char **Names, FpropsError *err);
void solve_mixture_conditions(unsigned n_sims, double *Ts, double *Ps, MixtureSpec *M, char **Names, FpropsError *err);
void densities_Ts_to_mixture(MixtureState *MS, double *P_out, double *T_in, double tol, char **Names, FpropsError *err);

void pengrob_phi_all(double *phi_out, MixtureSpec *M, double T, double *rhos, FpropsError *err);
double pengrob_phi_pure(PureFluid *PF, double T, double rho, FpropsError *err);
double poynting_factor(PureFluid *PF, double T, double rho, FpropsError *err);

void test_one(double *Ts, double *Ps);
void test_two(double T, double P);
void test_three(double T, double *rhos);
void test_four(double T, double P);
void test_five(double T, double P);
void test_six(void);

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
	test_five(270, 1.5e5);
	/* test_six(); */

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

void pengrob_phi_all(double *phi_out, MixtureSpec *M, double T, double *rhos, FpropsError *err){
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
		/* printf("\n  Substance number %u\n\tPressure is now %.0f Pa", i, P); */

		Tr = T/D->T_c;
		alpha = (1 + PR->kappa * (1 - sqrt(Tr)));
		aT = PR->aTc * alpha * alpha;

		A = aT * P / (RR * RR * T * T);
		B = PR->b * P / (RR * T);
		Z = P / (rhos[i] * RR * T);
		/* printf("\n\tCompressibility is %.6g;\n\t`A' is %.6g\n\t`B' is %.6g", Z, A, B); */

		cpx_ln = log( (Z + (1 + M_SQRT2)*B)/(Z + (1 - M_SQRT2)*B) );
		phi_out[i] = exp( Z - 1 - log(Z - B) - (A / 2 / M_SQRT2) * cpx_ln );
		/* printf("\n\tLog of fugacity coefficient is %.6g", Z - 1 - log(Z - B) - (A / (2*M_SQRT2))*cpx_ln); */
		/* printf("\n\tFugacity coefficient is %.6g", phi_out[i]); */
	}

#undef PR
#undef RR
#undef D
#undef NPURE
}

double pengrob_phi_pure(PureFluid *PF, double T, double rho, FpropsError *err){
#define RR PF->data->R
#define PR PF->data->corr.pengrob
	double P,     /* pressure */
		   Tr,    /* reduced temperature */
		   aT,    /* parameter `a' at the temperature */
		   alpha, /* intermediate variable in calculating aT */
		   A,     /* two variables */
		   B,
		   Z,     /* compressibility */
		   cpx_ln; /* rather involved part of the fugacity coefficient */
	
	P = fprops_p((FluidState){T, rho, PF}, err);
	printf("\n\tPressure is now %.0f Pa", P);
	Tr = T / PF->data->T_c;
	alpha = (1 + PR->kappa * (1 - sqrt(Tr)));
	aT = PR->aTc * alpha * alpha;

	A = aT * P / (RR * RR * T * T);
	B = PR->b * P / (RR * T);
	Z = P / (rho * RR * T);
	printf("\n\tCompressibility is %.6g;\n\t`A' is %.6g\n\t`B' is %.6g", Z, A, B);

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
double poynting_factor(PureFluid *PF, double T, double rho, FpropsError *err){
	double p,
		   p_sat,
		   rho_liq, rho_vap,
		   ln_pf; /* natural logarithm of Poynting Factor */

	p = fprops_p((FluidState){T, rho, PF}, err);

	fprops_sat_T(T, &p_sat, &rho_liq, &rho_vap, PF, err);

	ln_pf = (p - p_sat) / (rho_liq * PF->data->R * T);
	return exp(ln_pf);
}

/* Finding density from pressure */
/*
	Structure to hold auxiliary data for function to find the error in pressure 
	at a given density
 */
typedef struct PressureRhoData_Struct {
	double T;
	double P;
	PureFluid *pfl;
	FpropsError *err;
} PRData;

/*
	Return error in pressure given a certain density
 */
/* double pressure_rho_error(double rho, void *user_data){
	PRData *prd = (PRData *)user_data;
	FluidState fst = {prd->T, rho, prd->pfl};
	
	return fabs(prd->P - fprops_p(fst, prd->err));
} */

/*
	Return reasonable value of density at a given temperature and pressure, and 
	from a given starting density.
 */
double rho_from_pressure(double T, double P, double rho_start, double tol, const PureFluid *PF, FpropsError *err){
#define MAX_ITER 20
#define ALPHA 2.35
	unsigned i, j,
			 ix_low,  /* index of point (density, pressure) with lower error in P */
			 ix_high; /* index of point with higher error in P */
	int transit=0;    /* records whether the search has passed through the region of non-physical (dP/d(rho)) behavior */
	double ps[2],
		   /* delta_rho, */
		   rhos[] = {rho_start, 1.02*rho_start};
	double rho_new[3],
		   p_new[3];
	double A = 1.05,
		   B = 2.35,
		   C = 0.43;
	PRData prd = {T, P, PF, err};

	ps[1] = pressure_rho_error(rhos[1], &prd);

	for(i=0;i<MAX_ITER;i++){
		ps[0] = pressure_rho_error(rhos[0], &prd);
		ix_low = (fabs(ps[1]) < fabs(ps[0])) ? 1 : 0; /* which position has smallest error? */
		ix_high = 1 - ix_low;

		if(fabs(ps[0])<tol){
			printf("\n\t<rho_from_pressure>: Search for correct density SUCCEEDED after %u iterations "
					"at rho=%.6g kg/m^3"
					, i, rhos[0]);
			return rhos[0];
		}
		if(fabs(rhos[0]-rhos[1])<tol){
			/*
				Algorithm has converged on an extremum that does not satisfy the 
				pressure given.  Transition through non-physical region if it 
				has not already done so; otherwise, return an error.
			 */
			if(!transit){
				transit = 1; /* mark that algorithm has transitioned */
				transit_non_physical(T, rhos, PF, err, (ps[0]<P) ? 1 : 0);
				printf("\n\t<rho_from_pressure>: Transitioning across non-physical region");
			}else{
				printf("\n\t<rho_from_pressure>: Search for correct density FAILED after %u iterations,"
						"\n\t  densities equal at rho[0]=rho[1]=%.6g"
						, i, rhos[0]);
				return rhos[0];
			}
		}
		if(rhos[0]==INFINITY || rhos[1]==INFINITY 
				|| rhos[0]!=rhos[0] || rhos[1]!=rhos[1]){
			printf("\n\t<rho_from_pressure>: Search for correct density FAILED after %u iterations,"
					"\n\t  densities infinite or not-a-number at rho[0]=%.6g, rho[1]=%.6g"
					, i, rhos[0], rhos[1]);
			return rhos[0];
		}

		/*
			
		 */
		/* if((ps[0]-ps[1])/(rhos[0]-rhos[1]) > 0 || ){
			delta_rho = -ps[0] * (rhos[0] - rhos[1]) / (ps[0] - ps[1]);
			/ *
				If magnitude of change in density is greater than a certain 
				amount, reduce it to that magnitude.  This prevents the density 
				changes from growing too large, and is necessary for a 
				non-monotonic function like this one.
			 * /
			if(fabs(delta_rho) > ALPHA * fabs(rhos[0]-rhos[1])){
				delta_rho *= ALPHA / fabs(delta_rho);
			}
			rhos[1] = rhos[ix_low]; / * reassign second to lowest-error position * /
			ps[1] = ps[ix_low];
			rho[0] += delta_rho; 
		} */
		rho_new[0] = (1 + A)*rhos[ix_low] - A*rhos[ix_high];
		rho_new[1] = (1 - B)*rhos[ix_low] + B*rho_new[0];
		rho_new[2] = (1 - C)*rhos[ix_low] + C*rhos[ix_high];

		for(j=0;j<3;j++){
			p_new[j] = fabs(pressure_rho_error(rho_new[j], &prd));
		}

		rhos[1] = rhos[ix_low];
		ps[1] = ps[ix_low];
		rhos[0] = rho_new[index_of_min(3, p_new)];

		if(*err!=FPROPS_NO_ERROR){
			*err = FPROPS_NO_ERROR;
		}
	}

	printf("\n\t<rho_from_pressure>: Reached maximum number of iterations (%u) without converging on density;"
			"\n\trhos[0]=%.8g, rhos[1]=%.8g, P[0]=%.8g, P[1]=%.8g, tolerance=%g"
			, MAX_ITER, rhos[0], rhos[1], ps[0], ps[1], tol);
	return rhos[0];
#undef MAX_ITER
}

/*
	Transition across the non-physical portion of the pressure-density curve, 
	where dP/d(rho) < 0, that is, where density decreases with increasing 
	pressure.

	For this function, it is essential that the starting point be at or very 
	close to the peak of the P/rho curve, so that the first step takes the 
	density solidly into the non-physical region.
 */
void transit_non_physical(double T, double *rhos, const PureFluid *PF, FpropsError *err, int from_vap){
	unsigned i=0;
	double delta_rho;

	if(from_vap){
		rhos[1] = 1.05 * rhos[0];
		printf("\n  <transit_non_physical>: rhos[0]=%.6g, rhos[1]=%.6g, P[0]=%.1f Pa, P[1]=%.1f"
				, rhos[0], rhos[1]
				, fprops_p((FluidState){T, rhos[0], PF}, err)
				, fprops_p((FluidState){T, rhos[1], PF}, err));
		while(fprops_p((FluidState){T, rhos[0], PF}, err) > fprops_p((FluidState){T, rhos[1], PF}, err)){
			delta_rho = 1.5 * (rhos[1] - rhos[0]);
			rhos[0] = rhos[1];    /* rho[0], the trailing edge of the line segment, is shifted first */
			rhos[1] += delta_rho; /* then rho[1], the leading edge, is shifted */
			i++;
		}
		rhos[0] = 1.05 * rhos[1];
	}else{
		rhos[0] = 0.95 * rhos[1];
		printf("\n  <transit_non_physical>: rhos[0]=%.6g, rhos[1]=%.6g, P[0]=%.1f Pa, P[1]=%.1f"
				, rhos[0], rhos[1]
				, fprops_p((FluidState){T, rhos[0], PF}, err)
				, fprops_p((FluidState){T, rhos[1], PF}, err));
		while(fprops_p((FluidState){T, rhos[0], PF}, err) > fprops_p((FluidState){T, rhos[1], PF}, err)){
			rhos[1] = rhos[0]; /* rho[1] is now the trailing edge of the line segment, and is shifted first */
			rhos[0] *= 0.7;    /* then rho[0], the leading edge, is shifted */
			i++;
		}
		rhos[1] = 0.95 * rhos[0];
	}
	printf("\n\t<transit_non_physical>: Shifted to rho[0]=%.8g, after %u iterations", rhos[0], i);
}

/* Finding phase-equilibrium conditions */
/*
	Apportion components into either critical/supercritical or saturation/VLE 
	regions, and find saturation pressure, liquid and vapor densities for 
	species in the saturation region.  Return this data 
 */
PhaseMixState *mixture_phase_prepare(MixtureSpec *MX, double T, double P, FpropsError *err){
	/* unsigned i; */
}

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
	printf("\n  <dew_p_error>: entered the function");
	unsigned i;
	double p_d_term,
		   rp_d = 0.0;

	DBData *dbd = (DBData *)user_data;
	printf("\n\t%s", "<dew_p_error>: unpacked the user_data struct");

	for(i=0;i<NPURE;i++){
		RHOS[i] = rho_from_pressure(TT, P_D, RHOS[i], TOL, PF[i], ERR);
		/* RHOV[i] = rho_from_pressure(TT, P_D, RHOV[i], TOL, PF[i], ERR); */
		/* RHOL[i] = rho_from_pressure(TT, P_D, RHOL[i], TOL, PF[i], ERR); */

		/* p_d_term = YS[i] * pengrob_phi_pure(PF[i], TT, RHOS[i], ERR) / PSAT[i]; */

		rp_d += YS[i] * pengrob_phi_pure(PF[i], TT, RHOS[i], ERR) / PSAT[i];
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
	unsigned i;
	double p_b_term,
		   p_b = 0.0;

	DBData *dbd = (DBData *)user_data;

	for(i=0;i<NPURE;i++){
		RHOS[i] = rho_from_pressure(TT, P_B, RHOS[i], TOL, PF[i], ERR);
		/* RHOV[i] = rho_from_pressure(TT, P_B, RHOV[i], TOL, PF[i], ERR); */
		/* RHOL[i] = rho_from_pressure(TT, P_B, RHOL[i], TOL, PF[i], ERR); */

		/* p_b_term = XS[i] * pengrob_phi_pure(PF[i], TT, SATRHO[i], ERR) 
			* PSAT[i] * poynting_factor(PF[i], TT, RHOL[i], ERR);
		p_b_term /= pengrob_phi_pure(PF[i], TT, RHOV[i], ERR); */

		p_b += XS[i] * PSAT[i] / pengrob_phi_pure(PF[i], TT, RHOS[i], ERR);
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
	double p_sat[NPURE],
		   rho_l[NPURE],
		   rho_v[NPURE];
	double tol = 1.e-5;

	/* find ideal-gas dew pressure as a starting point */
	for(i=0;i<NPURE;i++){
		printf("\n  <dew_pressure>: Calculating saturation pressure for substance %s (at index %u)"
				"\n\tfrom temperature T=%.2f K...", PF[i]->name, i, T);
		fprops_sat_T(T, (p_sat+i), (rho_l+i), (rho_v+i), PF[i], err);
		rp_d += XS[i] / p_sat[i];
		printf("\n\tsaturation pressure was P_sat=%.0f Pa", p_sat[i]);
	}
	p_d[0] = 1./rp_d;
	printf("\n  <dew_pressure>: Ideal-gas dew pressure is P_D = %.0f Pa", p_d[0]);
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
	printf("\n  <dew_pressure>: Created DBData struct");
	puts("");

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
	return 0.0;
}

/* Functions to test/demonstrate other functionality */
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
	/* const EosData *IdealEos[]={
		&eos_rpp_nitrogen, 
		&eos_rpp_ammonia, 
		&eos_rpp_carbon_dioxide, 
		&eos_rpp_methane, 
		&eos_rpp_water
	}; */

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

	double mp_ps[] = {0.0, 0.0, 0.0};
	unsigned mp_pn[] = {0, 0, 0};
	double *mp_xs[] = {NULL, NULL, NULL};
	double *mp_rhos[] = {NULL, NULL, NULL};
	MixturePhaseState MP = {
		.T=T
		, .X=&MX
		, .phases = 0
		, .ph_name = mp_pn
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

	pengrob_phi_all(phi_1, &MX, T, MS.rhos, &err);
	/* pengrob_phi_mix(phi_2, &MS, P, &err); */

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

void test_five(double T, double P){
	unsigned i;
#define NPURE 4
#define D MS->PF[i]->data
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
	/* {
		.pures = NPURE
		, .Xs = Xs
		, .PF = (PureFluid *)pf;
	} */
	mixture_fluid_spec(MS, NPURE, (void *)fluids, "pengrob", src, &merr);

	/* for(i=0;i<NPURE;i++){ 
		printf("\n%s T_c = %.2f K, P_c = %.0f Pa", MS->PF[i]->name, D->T_c, D->p_c);
	} */

	/* declare variables used when finding phase equilibrium */
	double tol = 1.e-5;
	double p_sat[NPURE] = {0.0},
		   T_sat[NPURE] = {0.0};
	double rho_l[NPURE] = {0.0}, /* liquid-phase densities */
		   rho_v[NPURE] = {0.0}, /* vapor-phase densities */
		   rho_sc[NPURE] = {0.0}, /* supercritical densities */
		   rho_dummy = 0.0;
	double p_rho,
		   p_b = 0.0,    /* bubble pressure */
		   rp_d = 0.0,   /* reciprocal dew pressure */
		   p_d;          /* dew pressure */
	double phi_l[NPURE],
		   phi_v[NPURE],
		   pf_l[NPURE];
	double x_vle[NPURE], /* overall mole fractions in vapor-liquid equilibrium */
		   xs[4][NPURE]; /* per-phase mole fractions */
	double x_sum=0.0;    /* sum of mole fractions */

	MixtureSpec *MS_vle = ASC_NEW(MixtureSpec);
#define VPURE MS_vle->pures
#define VXS MS_vle->Xs
#define VPF MS_vle->PF
	VPURE = 0;
	// VXS = ASC_NEW_ARRAY(double,NPURE);
	VXS = (double *)malloc(sizeof(double)*NPURE);
	// VPF = ASC_NEW_ARRAY(PureFluid *,NPURE);
	VPF = (PureFluid **)malloc(sizeof(PureFluid *)*NPURE);

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
			/*
			fprops_sat_T(T, (p_sat+i), (rho_l+i), (rho_v+i), MS->PF[i], &err);
			fprops_sat_p(P, (T_sat+i), &rho_dummy, (rho_v+i), MS->PF[i], &err);

			x_vle[i] = Xs[i] / D->M;
			x_sum += x_vle[i];
			*/
		}else{
			printf("\n  Adding substance %s to supercritical phase", fluid_names[i]);
			SPF[SPURE] = MS->PF[i]; /* add new component and mass fraction, in next place */
            SXS[VPURE] = MS->Xs[i];
			SPURE ++;               /* increment number of pures in supercritical condition */
			/* rho_sc[i] = D->rho_c; */
		}
		/*
		if(rho_v[i]!=0 && rho_l[i]!=0){
			p_rho = fprops_p((FluidState){T,rho_v[i],MS->PF[i]}, &err);
			if(p_rho<P){
				rho_v[i] = rho_from_pressure(T, P, rho_v[i]*P/p_rho, tol, MS->PF[i], &err);
			}else{
				rho_v[i] = rho_from_pressure(T, P, rho_v[i], tol, MS->PF[i], &err);
			}
		}else{
			p_rho = fprops_p((FluidState){T,rho_sc[i],MS->PF[i]}, &err);
			if(p_rho>P){
				rho_sc[i] = rho_from_pressure(T, P, rho_sc[i]*P/p_rho, tol, MS->PF[i], &err);
			}else{
				rho_sc[i] = rho_from_pressure(T, P, rho_sc[i], tol, MS->PF[i], &err);
			}
		}
		*/
		printf("\n  VPURE = %u, and SPURE = %u; i = %u", VPURE, SPURE, i);
	}
	for(i=0;i<VPURE;i++){
		fprops_sat_T(T, (p_sat+i), (rho_l+i), (rho_v+i), VPF[i], &err);
		fprops_sat_p(P, (T_sat+i), &rho_dummy, (rho_v+i), VPF[i], &err);

		x_vle[i] /= x_sum;

		// phi_l[i] = pengrob_phi_pure(MS->PF[i], T, rho_l[i], &err);
		// phi_v[i] = pengrob_phi_pure(MS->PF[i], T, rho_v[i], &err);
	}
	/*
	for(i=0;i<NPURE;i++){
		if(rho_v[i]!=0 && rho_l[i]!=0){
		}
	}
	*/

	/* p_d = 1./rp_d; */
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

	p_d = dew_pressure(MS_vle, T, &err);
	printf("\n  Dew pressure is %.0f Pa\n", p_d);
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
	char *fluid_names[] = {
		"isohexane", "ammonia"
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

	double P[NROWS][NCOLS];
	char *heads[NCOLS],
		 *sides[NROWS+1],
		 *forms[NROWS] = {"%.6g"},
		 *conts[NROWS+1][NCOLS+1];
	unsigned widths[NCOLS],
			 temp_len; /* temporary string length */
	sides[0] = "";

	for(rho=1.0,i1=0;rho<5200.0;rho*=1.9,i1++){
		/* printf("\n  Modeling substance %s at density %.6g kg/m^3", MS->PF[C_PURE]->name, rho); */
		/* if((temp_len = asprintf((sides+i1+1), "rho=%.2f", rho)) < 1 || temp_len > 100){
			sides[i1] = "";
		} */
		/* printf("\n  length of output string was %u", temp_len); */
		asprintf((sides+i1+1), "rho=%.2f", rho);

		for(T=100.0,i2=0;T<=500.0;T+=50.0,i2++){
			/* printf("\n\tat temperature %.2f K", T); */
			if(i1==0){
				/* if((temp_len = asprintf((heads+i2), "T=%.6g", T)) < 1 || temp_len > 100){
					heads[i2] = "";
				} */
				/* printf("\n\t  length of output string was %u", temp_len); */
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
	/* puts("");
	for(i1=0;i1<NROWS+1;i1++){
		printf("\n  -%s", sides[i1]);
	}
	puts("");
	for(i1=0;i1<NCOLS;i1++){
		printf("\n  %s", heads[i1]);
	}
	puts(""); */

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
