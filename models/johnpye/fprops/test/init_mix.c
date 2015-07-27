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
	by Jacob Shealy, June 4-July 26, 2015
	
	Initial model of a simple mixture, to get the procedure right.  This is in 
	preparation for more general algorithms to find mixing conditions in the 
	ideal-mixture case.
 */

#include "../mixtures/init_mixfuncs.h"
#include "../mixtures/mixture_prepare.h"
#include "../mixtures/mixture_phases.h"
#include "../helmholtz.h"
#include "../pengrob.h"
#include "../fluids.h"
#include "../fprops.h"
#include "../refstate.h"
#include "../sat.h"
#include "../color.h"

#include <stdio.h>
#include <math.h>

extern const EosData eos_rpp_nitrogen;
extern const EosData eos_rpp_ammonia;
extern const EosData eos_rpp_carbon_dioxide;
extern const EosData eos_rpp_methane;
extern const EosData eos_rpp_water;

/* Function prototypes */

void mixture_flash(PhaseSpec *PS, MixtureSpec *MS, double T, double P, FpropsError *err);

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

/* Finding phase-equilibrium conditions */
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
	MSG("Entered the function");
	unsigned i, j
		, i_v = 0         /* indices of vapor, liquid, supercritical phases in PS */
		, i_l = 0
		, i_sc = 0
		;
	int flag_sc = 0       /* encountered any supercritical components? */
		, flag_vle = 0    /* encountered any subcritical components? */
		;
	double p_b, p_d       /* bubble and dew pressures */
		, xs_ph[2], mm[2] /* mole fractions & molar masses of super/subcritical phases */
		, rho_d[2]        /* density used in finding the saturation pressure */
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
			PPH[i_v]->c[PPH[i_v]->ncomps] = i;
			PPH[i_v]->Xs[PPH[i_v]->ncomps] = MXS[i];
			PPH[i_v]->PF[PPH[i_v]->ncomps] = MPF[i];

			PPH[i_v]->ncomps ++;

			flag_vle = 1;
		}else{
			/*
				Current component is supercritical.  Operations are analogous to 
				subcritical condition discussed above.
			 */
			i_sc += (flag_sc) ? 0 : NPHASE;
			NPHASE += (flag_sc) ? 0 : 1;

			PTYPE[i_sc] = SUPERCRIT;
			PPH[i_sc]->c[PPH[i_sc]->ncomps] = i;
			PPH[i_sc]->Xs[PPH[i_sc]->ncomps] = MXS[i];
			PPH[i_sc]->PF[PPH[i_sc]->ncomps] = MPF[i];
			PPH[i_sc]->ncomps ++;

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
		PFRAC = ASC_NEW_ARRAY(double,2);
		for(i=0;i<PPH[i_sc]->ncomps;i++){
			PFRAC[i_sc] += PPH[i_sc]->Xs[i]; /* supercritical phase */
		}
		PFRAC[i_v] = 1.0 - PFRAC[i_sc];       /* subcritical phase(s) */

		for(i=0;i<NPHASE;i++){ 
			mixture_x_props(PPH[i]->ncomps, PPH[i]->Xs, PPH[i]->Xs);
		}
		mm[0] = mixture_M_avg(PPH[0]->ncomps, PPH[0]->Xs, PPH[0]->PF);
		mm[1] = mixture_M_avg(PPH[1]->ncomps, PPH[1]->Xs, PPH[1]->PF);

		xs_ph[0] = PFRAC[0] / mm[0] / ((PFRAC[0] / mm[0]) + (PFRAC[1] / mm[1]));
		xs_ph[1] = PFRAC[1] / mm[1] / ((PFRAC[0] / mm[0]) + (PFRAC[1] / mm[1]));

		PFRAC[0] = xs_ph[0];
		PFRAC[1] = xs_ph[1];

		MSG("For the current mixture");
		MSG("\tSupercritical mass fraction is %.6g,\tsubcritical mass fraction is %.6g"
				, PFRAC[i_sc], PFRAC[i_v]);
		MSG("\tSupercritical molar mass is %.6g,   \tsubcritical molar mass is %.6g"
				, mm[i_sc], mm[i_v]);
		MSG("\tSupercritical mole fraction is %.6g, subcritical mole fraction is %.6g"
				, PFRAC[i_sc], PFRAC[i_v]);
		for(i=0;i<NPHASE;i++){ 
			MSG("For %s phase", (PTYPE[i]==SUPERCRIT) ? "supercritical" : "subcritical");
			for(j=0;j<PPH[i]->ncomps;j++){
				MSG("\tmass fraction of %s is %.6g"
						, PPH[i]->PF[j]->name, PPH[i]->Xs[j]);
				MSG("\tmole fraction of %s is %.6g"
						, PPH[i]->PF[j]->name, PPH[i]->xs[j]);
			}
		}

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
		double phi_v[PPH[i_v]->ncomps]
			, p_sat[PPH[i_v]->ncomps]
			, K[PPH[i_v]->ncomps]
			;
		/*
			Obtain mole fractions for vapor phase -- these represent the overall 
			mole fractions in all subcritical phases, since vapor phase is being 
			used as an alias for all subcritical phases considered together.
		 */
		mole_fractions(PPH[i_v]->ncomps, PPH[i_v]->xs, PPH[i_v]->Xs, PPH[i_v]->PF);

		/*
			Create a new array of MixtureSpec structures to hold PureFluid 
			structures for supercritical and subcritical phases.
		 */
		MixtureSpec MS_temp = {
			PPH[i_v]->ncomps
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
			NPHASE = 3;
			i_l = i_v + 1;
			PTYPE[i_l] = LIQUID;

			/*
				Read each component in the vapor phase into the liquid phase as 
				well.  Find the saturation temperature, vapor-phase fugacity 
				coefficient, and K-factor to use in calculating liquid and vapor 
				mole fractions.
			 */
			MSG("Calculating K-factors...");
			for(i=0;i<PPH[i_v]->ncomps;i++){
				fprops_sat_T(T, (p_sat+i), (rho_d), (rho_d+1), PPH[i_v]->PF[i], err);
				phi_v[i] = pengrob_phi_pure(PPH[i_v]->PF[i], T, p_b, VAPOR, err);
				K[i] = p_sat[i] / (phi_v[i] * P);
				MSG("K-factor for %s is %.6g"
						, PPH[i_v]->PF[i]->name, K[i]);
			}
			RRData RR = {PPH[i_v]->ncomps, PPH[i_v]->xs, K};
			double V[] = {0.5, 0.51};

			/* secant_solve(&flash_error, &FD, V, tol); */
			secant_solve(&rachford_rice, &RR, V, tol);

			PPH[i_l]->ncomps = PPH[i_v]->ncomps;
			PPH[i_l]->c = PPH[i_v]->c;
			PPH[i_l]->PF = PPH[i_v]->PF;

			MSG("Subcritical mole fraction is %.6g, and vapor mole fraction "
					"within this is %.6g", PFRAC[i_v], V[0]);

			PFRAC[i_l] = PFRAC[i_v] * (1 - V[0]);
			PFRAC[i_v] *= V[0];

			MSG("Vapor mole fraction in mixture is %.6g, and liquid mole fraction "
					"is %.6g", PFRAC[i_v], PFRAC[i_l]);

			for(i=0;i<PPH[i_v]->ncomps;i++){
				PPH[i_l]->xs[i] = PPH[i_v]->xs[i] / (1 + V[0]*(K[i] - 1));
				PPH[i_v]->xs[i] *= K[i] / (1 + V[0]*(K[i] - 1));
			}
		}
	}
	if(PTYPE[0]==SUPERCRIT){
		mole_fractions(PPH[i_sc]->ncomps, PPH[i_sc]->xs, PPH[i_sc]->Xs, PPH[i_sc]->PF);
	}

#if 1
	MSG("At temperature %.2f K and pressure %.0f Pa, there are %u phases;"
			, T, P, NPHASE);
	for(i=0;i<NPHASE;i++){
		MSG("\t%.6g of total moles are in %s phase"
				, PFRAC[i], (PTYPE[i]==SUPERCRIT) ? "supercritical" :
				(PTYPE[i]==VAPOR) ? "vapor" : "liquid");
		for(j=0;j<PPH[i]->ncomps;j++){
			MSG("\t  mole fraction of %s in this phase is %.6g"
					, PPH[i]->PF[j]->name, PPH[i]->xs[j]);
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
#undef TITLE
}

/* Functions to test/demonstrate other functionality */
void test_five(double T, double P){
#define NPURE 4
#define D MS->PF[i]->data
#define TRIALS 5
	unsigned i;
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
		MSG("%s T_c = %.2f K, P_c = %.0f Pa", MS->PF[i]->name, D->T_c, D->p_c);
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
			MSG("Adding substance %s to vapor-liquid equilibrium", fluid_names[i]);
			VPF[VPURE] = MS->PF[i]; /* add new component and mass fraction, in next place */
			VXS[VPURE] = MS->Xs[i];
			VPURE ++;               /* increment number of pures in VLE */
		}else{
			MSG("Adding substance %s to supercritical phase", fluid_names[i]);
			SPF[SPURE] = MS->PF[i]; /* add new component and mass fraction, in next place */
            SXS[SPURE] = MS->Xs[i];
			SPURE ++;               /* increment number of pures in supercritical condition */
		}
		MSG("VPURE = %u, and SPURE = %u; i = %u", VPURE, SPURE, i);
	}
	for(i=0;i<VPURE;i++){
		fprops_sat_T(T, (p_sat+i), (rho_l+i), (rho_v+i), VPF[i], &err);
		fprops_sat_p(P, (T_sat+i), &rho_dummy, (rho_v+i), VPF[i], &err);

		x_vle[i] /= x_sum;
	}

	/*
	for(i=0;i<NPURE;i++){
		if(rho_v[i]!=0 && rho_l[i]!=0){
			MSG("Saturation pressure for %s at T=%.2f K is %.0f Pa;"
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
			MSG("Supercritical density for %s is %.6g kg/m^3;"
					"\n\tthe calculated pressure from this density is %.0f Pa\n"
					, fluid_names[i], rho_sc[i]
					, fprops_p((FluidState){T, rho_sc[i], MS->PF[i]}, &err));
		}
	}
	MSG("Bubble pressure is %.0f Pa,"
			"\n  Reciprocal dew pressure is %.6g Pa^-1, and dew pressure is %.0f Pa\n"
			, p_b, rp_d, p_d);
	*/
	for(i=0;i<VPURE;i++){
		MSG("Saturation pressure for %s at T=%.2f K is %.0f Pa;"
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
	MSG("Dew pressure is %.0f Pa", p_d);
	MSG("\n  Bubble pressure is %.0f Pa", p_b);
	puts(""); */
	for(i=0;i<TRIALS;i++){
		MSG("At temperature %.2f K, dew pressure is %.0f Pa, bubble pressure is %.0f Pa"
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
		MSG("Modeling substance %s at density %.6g kg/m^3", MS->PF[C_PURE]->name, rho);
		if((temp_len = asprintf((sides+i1+1), "rho=%.2f", rho)) < 1 || temp_len > 100){
			sides[i1] = "";
		}
		MSG("Length of output string was %u", temp_len);
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
		for(i2=0;i2<NCOLS;i2++){
			MSG("%s\t%s\t%.6g", sides[i1+1], heads[i2], P[i1][i2]);
		}
	}
	puts("");
	/* PRINT_STR_TABLE(NROWS+1,NCOLS+1,widths,conts); */

#undef D
#undef NPURE
}

void test_seven(void){
#define NPURE 4
#define D MS->PF[i]->data
#define TEMPS 4
#define PRESSURES 3
	unsigned j, i;
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
		, Ps[] = {1.5e5, 2.5e5, 4e5, 6e5}
		;

	MSG("Declared all variables...");
#if 0
	for(j=0;j<TEMPS;j++){
		MSG("Preparing all variables for temperture %.2f K...", Ts[j]);

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

		MSG("Prepared all variables");

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
			MSG("Calculating K-factors...");
			for(i=0;i<VPURE;i++){
				K[i] = p_sat[j][i] / (phi_v[j][i] * P);
				MSG("K-factor for %s is %.6g"
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
		MSG("Found mixture conditions (mass and mole fractions, bubble and dew "
				"pressures) at temperature %.2f K"
				, Ts[j]);
	}
	for(j=0;j<TEMPS;j++){
		MSG("At temperature %.2f K"
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
			MSG("At the given pressure %.0f Pa, mixture is in VLE; mole splits are:"
					"\n\t  %.6f of total moles are in the vapor, %.6f in the liquid"
					, P, PFRAC[1], PFRAC[2]);
			for(i=0;i<VPURE;i++){
				printf("\n\t  %.6f of vapor is %s, %.6f of liquid is %s"
						, PXS(1)[i], VPF[i]->name, PXS(2)[i], VPF[i]->name);
			}
		}else{
			MSG("At the given pressure %.0f Pa, mixture of %s ", P, VPF[0]->name);
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
	/* MSG_MARK("1"); */
	unsigned i1, i2;
	for(i1=0;i1<TEMPS;i1++){ 
		for(i2=0;i2<PRESSURES;i2++){ 
			printf("\n  At temperature %.2f K and pressure %.0f Pa, there are "
					"%u phases;"
					, Ts[i1], Ps[i2], NPHASE);
			for(i=0;i<NPHASE;i++){
				printf("\n\t%.6g of total moles are in %s phase"
						, PFRAC[i], (PTYPE[i]==SUPERCRIT) ? "supercritical" :
						(PTYPE[i]==VAPOR) ? "vapor" : "liquid");
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
}
