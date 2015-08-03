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

#include "mixture_struct.h"
#include "mixture_phases.h"
#include "mixture_properties.h"
#include "../helmholtz.h"
#include "../pengrob.h"
#include "../fluids.h"
#include "../fprops.h"
#include "../refstate.h"
#include "../sat.h"
#include "../color.h"

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
	double rp_d = 0.0;

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
	double p_b = 0.0;

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
	MSG("Entered the function...");
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
	}
	p_d[0] = 1./rp_d;
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
		/* MSG("Adding substance %s, at temperature %.2f K, with error %i"
				, PF[i]->name, T, *err); */
		fprops_sat_T(T, (p_sat+i), (rho_l+i), (rho_v+i), PF[i], err);
		p_b[0] += xs[i] * p_sat[i];
	}
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

	secant_solve(&bubble_p_error, &dbd, p_b, tol);

	return p_b[0];
#undef XS
#undef PF
#undef NPURE
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

/*
	Find whether a mixture flashes (is in vapor-liquid equilibrium), and if so, 
	what are the characteristics of the flash (phases present, mass/mole 
	fraction in the vapor & liquid, mass/mole fraction of each component in each 
	phase, etc.)
 */
void mixture_flash(PhaseSpec *PS, MixtureSpec *MS, double T, double P, FpropsError *err){
#define NPURE MS->pures
#define MXS MS->Xs
#define MPF MS->PF
#define D MPF[i]->data
#define NPHASE PS->phases
#define PTYPE PS->ph_type
#define PFRAC PS->ph_frac
#define PPH PS->PH
	MSG("Entered the function");
	unsigned i, j
		, i_v = 0          /* indices of vapor, liquid, supercritical phases in PS */
		, i_l = 0
		, i_sc = 0
		;
	int flag_sc = 0        /* encountered any supercritical components? */
		, flag_vle = 0     /* encountered any subcritical components? */
		;
	double p_b, p_d, p_sat /* bubble, dew, and (throwaway) saturation pressures */
		, xs_ph[2], mm[2]  /* mole fractions & molar masses of super/subcritical phases */
		, rho_d[2]         /* density used in finding the saturation pressure */
		, tol = MIX_XTOL
		;

	PTYPE = ASC_NEW_ARRAY(PhaseName,3);
	PFRAC = ASC_NEW_ARRAY(double,3);
	PPH = ASC_NEW_ARRAY(Phase *,3);
	for(i=0;i<3;i++){
		PPH[i] = ASC_NEW(Phase);
		PPH[i]->pures = 0;
		PPH[i]->c  = ASC_NEW_ARRAY(unsigned,NPURE);
		PPH[i]->Xs = ASC_NEW_ARRAY(double,NPURE);
		PPH[i]->xs = ASC_NEW_ARRAY(double,NPURE);
		PPH[i]->PF = ASC_NEW_ARRAY(PureFluid *,NPURE);
		PPH[i]->rhos = ASC_NEW_ARRAY(double,NPURE);
	}
	NPHASE = 0;

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

			PTYPE[i_v] = VAPOR;
			PPH[i_v]->c[PPH[i_v]->pures] = i;
			PPH[i_v]->Xs[PPH[i_v]->pures] = MXS[i];
			PPH[i_v]->PF[PPH[i_v]->pures] = MPF[i];

			PPH[i_v]->pures ++;

			flag_vle = 1;
		}else{
			/*
				Current component is supercritical.  Operations are analogous to 
				subcritical condition discussed above.
			 */
			i_sc += (flag_sc) ? 0 : NPHASE;
			NPHASE += (flag_sc) ? 0 : 1;

			PTYPE[i_sc] = SUPERCRIT;
			PPH[i_sc]->c[PPH[i_sc]->pures] = i;
			PPH[i_sc]->Xs[PPH[i_sc]->pures] = MXS[i];
			PPH[i_sc]->PF[PPH[i_sc]->pures] = MPF[i];
			PPH[i_sc]->pures ++;

			flag_sc = 1;
		}
	}

	/*
		Rearrange order of supercritical/subcritical phases, and determine mass 
		fractions of the mixture that are in any supercritical phase.
	 */
	if(NPHASE==2){
		if(PTYPE[0]==VAPOR){
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
			ERRMSG("Two phases, but types are %i and %i, not VAPOR and SUPERCRIT.\n"
					, PTYPE[0], PTYPE[1]);
		}

		/*
			Determine values of 'PFRAC', the fraction of system mass in each phase
		 */
		/* PFRAC = ASC_NEW_ARRAY(double,2); */
		for(i=0;i<PPH[i_sc]->pures;i++){
			PFRAC[i_sc] += PPH[i_sc]->Xs[i]; /* supercritical phase */
		}
		PFRAC[i_v] = 1.0 - PFRAC[i_sc];       /* subcritical phase(s) */
#if 1
		MSG("For the current mixture");
		MSG("\tSupercritical mass fraction is %.6g,\tsubcritical mass fraction is %.6g"
				, PFRAC[i_sc], PFRAC[i_v]);
#endif
		/*
			Scale mass fractions to sum to one within each phase, and calculate 
			average molar mass for each phase.  Then calculate true mole 
			fraction of each phase within the overall mixture.
		 */
		for(i=0;i<NPHASE;i++){ 
			mixture_x_props(PPH[i]->pures, PPH[i]->Xs, PPH[i]->Xs);
		}
		mm[0] = mixture_M_avg(PPH[0]->pures, PPH[0]->Xs, PPH[0]->PF);
		mm[1] = mixture_M_avg(PPH[1]->pures, PPH[1]->Xs, PPH[1]->PF);
		xs_ph[0] = PFRAC[0] / mm[0] / ((PFRAC[0] / mm[0]) + (PFRAC[1] / mm[1]));
		xs_ph[1] = PFRAC[1] / mm[1] / ((PFRAC[0] / mm[0]) + (PFRAC[1] / mm[1]));

		PFRAC[0] = xs_ph[0];
		PFRAC[1] = xs_ph[1];

		/*
			Obtain mole fractions for the supercritical and subcritical phases 
			(subcritical phase is represented by the vapor).
		 */
		mole_fractions(PPH[i_v]->pures, PPH[i_v]->xs, PPH[i_v]->Xs, PPH[i_v]->PF);
		mole_fractions(PPH[i_sc]->pures, PPH[i_sc]->xs, PPH[i_sc]->Xs, PPH[i_sc]->PF);
#if 1
		MSG("\tSupercritical molar mass is %.6g,   \tsubcritical molar mass is %.6g"
				, mm[i_sc], mm[i_v]);
		MSG("\tSupercritical mole fraction is %.6g, subcritical mole fraction is %.6g"
				, PFRAC[i_sc], PFRAC[i_v]);
		for(i=0;i<NPHASE;i++){ 
			MSG("For %s phase", (PTYPE[i]==SUPERCRIT) ? "supercritical" : "subcritical");
			for(j=0;j<PPH[i]->pures;j++){
				MSG("\tmass fraction of %s is %.6g"
						, PPH[i]->PF[j]->name, PPH[i]->Xs[j]);
				MSG("\t mole fraction of %s is %.6g"
						, PPH[i]->PF[j]->name, PPH[i]->xs[j]);
			}
		}
#endif
	}else if(NPHASE>=3){
		ERRMSG("More than two phases occurred (there should be only up to two, "
				"a supercritical and subcritical).");
	}else{
		PFRAC[0] = 1.0;
	}

	/*
		Determine bubble pressure and dew pressure of any subcritical mixture
	 */
	if(NPHASE==2 || PTYPE[0]==VAPOR){
		/*
			Declare arrays sized to the subcritical phase(s)
		 */
		double phi_v[PPH[i_v]->pures]
			, p_sat[PPH[i_v]->pures]
			, K[PPH[i_v]->pures]
			;

		/*
			Create a new array of MixtureSpec structures to hold PureFluid 
			structures for supercritical and subcritical phases.
		 */
		MixtureSpec MS_temp = {
			PPH[i_v]->pures
			, PPH[i_v]->xs
			, PPH[i_v]->PF
		};
		p_b = bubble_pressure(&MS_temp, T, err);
		p_d = dew_pressure(&MS_temp, T, err);

		/*
			If the system pressure is above the bubble pressure, subcritical 
			components are all in the liquid; if pressure is below dew pressure, 
			system components are all in the vapor; and if pressure is between 
			bubble and dew pressures, they are in vapor-liquid equilibrium.
		 */
		if(P > p_b){
			i_l = i_v;
			PTYPE[i_l] = LIQUID;
		}else if(P < p_d){
			;
		}else{
			/* Read each attribute of the vapor phase into the liquid phase. */
			NPHASE = 3;
			i_l = i_v + 1;
			PTYPE[i_l] = LIQUID;

			PPH[i_l]->pures = PPH[i_v]->pures;
			PPH[i_l]->c = PPH[i_v]->c;
			PPH[i_l]->PF = PPH[i_v]->PF;

			/*
				Calculate K-factor for each component; create Rachford-Rice data 
				structure and array of vapor fractions.  Use these to calculate 
				flash conditions.
			 */
			for(i=0;i<PPH[i_v]->pures;i++){
				fprops_sat_T(T, (p_sat+i), (rho_d), (rho_d+1), PPH[i_v]->PF[i], err);
				phi_v[i] = pengrob_phi_pure(PPH[i_v]->PF[i], T, p_b, VAPOR, err);
				K[i] = p_sat[i] / (phi_v[i] * P);
			}
			RRData RR = {PPH[i_v]->pures, PPH[i_v]->xs, K};
			double V[] = {0.5, 0.51};

			secant_solve(&rachford_rice, &RR, V, tol);

			PFRAC[i_l] = PFRAC[i_v] * (1 - V[0]);
			PFRAC[i_v] *= V[0];

			for(i=0;i<PPH[i_v]->pures;i++){
				PPH[i_l]->xs[i] = PPH[i_v]->xs[i] / (1 + V[0]*(K[i] - 1));
				PPH[i_v]->xs[i] *= K[i] / (1 + V[0]*(K[i] - 1));
			}

			/*
				Find mass fractions of components in liquid and vapor phases; 
				supercritical phase already has correct mass fractions
			 */
			/* double xm_sum[3] = {0.0};
			for(i=0;i<PPH[i_v]->pures;i++){
				xm_sum[i_v] += PPH[i_v]->xs[i] * PPH[i_v]->PF[i]->data->M;
				xm_sum[i_l] += PPH[i_l]->xs[i] * PPH[i_l]->PF[i]->data->M;
			}
			for(i=0;i<PPH[i_v]->pures;i++){
				PPH[i_v]->Xs[i] = PPH[i_v]->xs[i] * PPH[i_v]->PF[i]->data->M / xm_sum[i_v];
				PPH[i_l]->Xs[i] = PPH[i_l]->xs[i] * PPH[i_l]->PF[i]->data->M / xm_sum[i_l];
			} */
			mass_fractions(PPH[i_v]->pures, PPH[i_v]->Xs, PPH[i_v]->xs, PPH[i_v]->PF);
			mass_fractions(PPH[i_l]->pures, PPH[i_l]->Xs, PPH[i_l]->xs, PPH[i_l]->PF);
		}
	}
	if(PTYPE[0]==SUPERCRIT){
		;
	}
	/* mixture_rhos_sat(PS, T, P, err); */

#if 1
	MSG("At temperature %.2f K and pressure %.0f Pa, there are %u phases;"
			, T, P, NPHASE);
	for(i=0;i<NPHASE;i++){
		MSG("\t%.6g of total moles are in %s phase"
				, PFRAC[i], (PTYPE[i]==SUPERCRIT) ? "supercritical" :
				(PTYPE[i]==VAPOR) ? "vapor" : "liquid");
		for(j=0;j<PPH[i]->pures;j++){
			MSG("\t  mole fraction of %s in this phase is  %.6g"
					, PPH[i]->PF[j]->name, PPH[i]->xs[j]);
			MSG("\t   mass fraction of %s in this phase is %.6g"
					, PPH[i]->PF[j]->name, PPH[i]->Xs[j]);
		}
	}
	if(NPHASE>=2){
		MSG("\tbubble pressure is %.1f Pa", p_b);
		MSG("\tdew pressure is %.1f Pa", p_d);
	}
	puts("");
#endif

#undef PPH
#undef PFRAC
#undef PTYPE
#undef NPHASE
#undef D
#undef MXS
#undef NPURE
}

