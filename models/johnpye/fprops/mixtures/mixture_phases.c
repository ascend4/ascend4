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
	by Jacob Shealy, July 26-, 2015 (GSOC 2015)

	Phase-equilibrium routines for ideal solution mixtures; generally these 
	handle only supercritical (gas/fluid), vapor, and liquid phases.
 */

#include "mixture_phases.h"

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
	Find fugacity coefficient for vapor or liquid
 */
double pengrob_phi_pure(PureFluid *PF, double T, double P, PhaseName type, FpropsError *err){
#define RR PF->data->R
#define PR PF->data->corr.pengrob
	unsigned i
		, z_num
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
	
	/* MSG("Pressure is now %.0f Pa", P); */
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
				ERRMSG(MIX_COMPR_ERROR " when calculating"
						"\n\tfugacity coefficient for %s phase; compressibility is %.6g"
						, (type<=VAPOR) ? "vapor/gas" : "liquid", Z);
				/* err = FPROPS_RANGE_ERROR; */
			}
		}
	}
	MSG("The available compressibilities are:");
	for(i=0;i<z_num;i++){ 
		MSG("%.6g", Z_rt[0]);
	}
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
	/* MSG("Entered the function"); */
	unsigned i;
	double p_d_term,
		   rp_d = 0.0;

	DBData *dbd = (DBData *)user_data;
	/* MSG("Unpacked the user_data struct for mixture "
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
	/* MSG("Entered the function"); */
	unsigned i;
	double p_b_term,
		   p_b = 0.0;

	DBData *dbd = (DBData *)user_data;
	/* MSG("Unpacked the user_data struct for mixture "
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
		/* MSG("Calculating saturation pressure for substance %s (at index %u)"
				"\n\tfrom temperature T=%.2f K...", PF[i]->name, i, T); */
		fprops_sat_T(T, (p_sat+i), (rho_l+i), (rho_v+i), PF[i], err);
		rp_d += xs[i] / p_sat[i];
		/* printf("\n\tsaturation pressure was P_sat=%.0f Pa", p_sat[i]); */
	}
	p_d[0] = 1./rp_d;
	/* MSG("Ideal-gas dew pressure is P_D = %.0f Pa", p_d[0]); */
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
	/* MSG("Created DBData struct"); */
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
	MSG("Entered the function...");
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
		MSG("Adding substance %s, at temperature %.2f K, with error %i"
				, PF[i]->name, T, *err);
		fprops_sat_T(T, (p_sat+i), (rho_l+i), (rho_v+i), PF[i], err);
		p_b[0] += xs[i] * p_sat[i];
	}
	/* MSG("Ideal-gas bubble pressure is P_B = %.0f Pa", p_b[0]); */
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
	/* MSG("Created DBData struct\n"); */

	secant_solve(&bubble_p_error, &dbd, p_b, tol);

	return p_b[0];
#undef XS
#undef PF
#undef NPURE
}

