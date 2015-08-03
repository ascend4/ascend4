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

/* #include "../mixtures/init_mixfuncs.h" */
#include "../mixtures/mixture_struct.h"
#include "../mixtures/mixture_properties.h"
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

#define PREPARE_TABLE(ROWS,COLS,T_HEAD,T_SIDE,T_VALS,T_FORM,T_CONT) \
	for(i2=0;i2<COLS-1;i2++){ \
		T_CONT[0][i2+1] = T_HEAD[i2]; \
	} \
	for(i1=0;i1<ROWS;i1++){ \
		T_CONT[i1][0] = T_SIDE[i1]; \
	} \
	for(i1=0;i1<ROWS-1;i1++){ \
		for(i2=0;i2<COLS-1;i2++){ \
			/* T_CONT[i1+1][i2+1] = ASC_NEW_ARRAY(char,40); */ \
			asprintf((T_CONT[i1+1]+i2+1), T_FORM[i1], T_VALS[i1][i2]); \
			/* snprintf(T_CONT[i1+1][i2+1], 39, T_FORM[i1], T_VALS[i1][i2]); */ \
			printf("\n%s", T_CONT[i1+1][i2+1]); \
		} \
	}

#define PRINT_STR_TABLE(ROWS,COLS,CWIDTH,CELLS) \
	for(i1=0;i1<COLS;i1++){ \
		CWIDTH[i1] = 0; \
	} \
	for(i1=0;i1<ROWS;i1++){ \
		for(i2=0;i2<COLS;i2++){ \
			if(strlen(CELLS[i1][i2])>CWIDTH[i2]){ \
				CWIDTH[i2] = strlen(CELLS[i1][i2]); \
				MSG("The length of the string %s is %u", CWIDTH[i2]); \
			} \
		} \
	} \
	printf("\n"); \
	for(i1=0;i1<ROWS;i1++){ \
		for(i2=0;i2<COLS;i2++){ \
			printf(" %s ", CELLS[i1][i2]); \
			for(i3=0;i3<(CWIDTH[i2] - strlen(CELLS[i1][i2]));i3++){ \
				printf("%c", ' '); \
			} \
		} \
		printf("\n"); \
	}

extern const EosData eos_rpp_nitrogen;
extern const EosData eos_rpp_ammonia;
extern const EosData eos_rpp_carbon_dioxide;
extern const EosData eos_rpp_methane;
extern const EosData eos_rpp_water;

/* Function prototypes */
void test_one(double *T, double *P);
// void test_five(double T, double P);
// void test_six(void);
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
	double T[]={250, 300, 300, 350, 350, 400, 900};               /* K */
	double P[]={1.5e5, 1.5e5, 1.9e5, 1.9e5, 2.1e5, 2.1e5, 3.2e6}; /* Pa */

	test_one(T, P);
	/* test_seven(); */

	return 0;
} /* end of `main' */

/* Functions to test/demonstrate other functionality */
void test_one(double *T, double *P){
#define NFLUIDS 4
#define NSIMS 1
	unsigned i, j; /* counter variable */
	enum FluidAbbrevs {N2,NH3,CO2,CH4,H2O}; /* fluids that will be used */

	char *fluids[]={
		"nitrogen", "ammonia", "carbondioxide", "methane", "water"
	};

	MixtureSpec *MS = new_MixtureSpec(NFLUIDS);
	PhaseSpec *PS = new_PhaseSpec(NFLUIDS,3);
	FpropsError err = FPROPS_NO_ERROR;
	MixtureError merr = MIXTURE_NO_ERROR;

	const char *source[NFLUIDS] = {NULL};
	double xs[NFLUIDS];                  /* mass fractions */
	double props[] = {2, 6, 4, 3, 5};    /* proportions of mass fractions */
	mixture_x_props(NFLUIDS, xs, props); /* mass fractions from proportions */

	mixture_specify(MS, NFLUIDS, xs, (const void **) fluids, "pengrob", source, &merr);

	double rho_mix[NSIMS] = {0}
		, u_mix[NSIMS] = {0}
		, h_mix[NSIMS] = {0}
		/* , cp_mix[NSIMS]
		, cv_mix[NSIMS]
		, s_mix[NSIMS]
		, g_mix[NSIMS]
		, a_mix[NSIMS] */
		;
	double rhos[NSIMS][3] = {{0}}
		, u_ph[NSIMS][3] = {{0}}
		, h_ph[NSIMS][3] = {{0}}
		;

	int usr_cont=1;
	double tol = 1e-9;
	char temp_str[100];
	char *headers[NSIMS];

#define MIXTURE_CALC(PROP) mixture_##PROP(&PM, PROP##_ph[i], &err)
	for(i=0;i<NSIMS;i++){
		mixture_flash(PS, MS, T[i], P[i], &err); /* flash mixture, obtaining phases */
		mixture_rhos_sat(PS, T[i], P[i], &err);  /* find densities in each phase */

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
		PhaseMixState PM = {
			.T = T[i]
			, .p = P[i]
			, .PS = PS
			, .MS = MS
		};
		rho_mix[i] = mixture_rho(&PM, rhos[i]);
		u_mix[i] = MIXTURE_CALC(u);
		h_mix[i] = MIXTURE_CALC(h);
		/* cp_mix[i] = MIXTURE_CALC(cp);
		cv_mix[i] = MIXTURE_CALC(cv);
		s_mix[i] = MIXTURE_CALC(s);
		g_mix[i] = MIXTURE_CALC(g);
		a_mix[i] = MIXTURE_CALC(a); */

		/* Fill in header */
		/* headers[i] = ASC_NEW_ARRAY(char,40); */
		/* snprintf(headers[i], 40, "[T=%.1f K, P=%.2f bar]", T[i], P[i]/1.e5); */
	}
#undef MIXTURE_CALC

	for(i=0;i<NSIMS;i++){ 
		MSG("At T=%.1f K, P=%.0f Pa, the overall density is %g kg/m^3"
				, T[i], P[i], rho_mix[i]);
		MSG("  The overall internal energy is %g J/kg, and enthalpy is %g J/kg"
				, u_mix[i], h_mix[i]);
		for(j=0;j<3;j++){
			MSG("\tFor phase number %u, the phase density is %g kg/m^3", j, rhos[i][j]);
			MSG("\t  The phase internal energy is %g J/kg, and enthalpy is %g J/kg"
					, u_ph[i][j], h_ph[i][j]);
		}
	}
	/* print_cases_properties(NSIMS, headers, rho_mix, Ps, u_mix, h_mix, cp_mix, cv_mix, s_mix, g_mix, a_mix); */
}

void test_seven(void){
#define NPURE 5
#define D MS->PF[i]->data
#define TEMPS 4
#define PRESSURES 3
	unsigned j, i;
	char *fluids[] = {
		"isohexane", "krypton", "carbonmonoxide", "ammonia", "water"
	};
	double props[] = {11, 4, 2, 3, 2};
	double Xs[NPURE];
	mixture_x_props(NPURE, Xs, props);
	const char *src[NPURE] = {NULL};
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

	/* MS->pures = NPURE;
	MS->Xs = Xs;
	MS->PF = ASC_NEW_ARRAY(PureFluid *,NPURE);
	mixture_fluid_spec(MS, NPURE, (void *)fluids, "pengrob", src, &merr); */
	mixture_specify(MS, NPURE, Xs, (void *)fluids, "pengrob", src, &merr);

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
				for(j=0;j<PPH[i]->pures;j++){
					printf("\n\t  mass fraction of %s in this phase is  %.6g"
							, PPH[i]->PF[j]->name, PPH[i]->Xs[j]);
					printf("\n\t   mole fraction of %s in this phase is %.6g"
							, PPH[i]->PF[j]->name, PPH[i]->xs[j]);
				}
			}
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
