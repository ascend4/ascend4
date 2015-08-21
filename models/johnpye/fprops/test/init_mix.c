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
#include "../mixtures/mixture_struct.h"
#include "../mixtures/mixture_properties.h"
#include "../mixtures/mixture_prepare.h"
#include "../mixtures/mixture_phases.h"
#include "../mixtures/init_mixfuncs.h"
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
void extract_rhos(PhaseSpec *PS, unsigned npure, double *rhos);
void test_one(double *T, double *P);
void test_seven(void);
void test_eight(double *p, double *h);

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

#if 0
	int usr_cont=1;
	char temp_str[100];

	/* Mixing conditions (temperature,pressure), and mass fractions */
	double T[]={250, 300, 300, 350, 350, 400, 900};               /* K */
	double P[]={1.5e5, 1.5e5, 1.9e5, 1.9e5, 2.1e5, 2.1e5, 3.2e6}; /* Pa */

	test_one(T, P);

	printf("\n  %s\n\t%s\n\t%s", "Continue on to the next simulation?",
			"0 - No", "1 - Yes");
	do{
		printf("\n    %s ", "Choice?");
		fgets(temp_str, 100, stdin);
	}while((1 != sscanf(temp_str, "%i", &usr_cont) || (0>usr_cont || 1<usr_cont)) 
			&& printf("\n  %s", "You must enter either 0 or 1"));
	if(1!=usr_cont){
		return 0;
	}
#endif
	double p[]={1.5e5, 1.5e5, 1.9e5, 1.9e5, 2.1e5, 2.1e5, 3.2e6}; /* Pa */
	double h[]={5.3e5, 5.2e5, 7.1e5, 7.3e5, 1.1e6, 1.05e6, 1.1e6}; /* J/kg */

	/* test_seven(); */
	test_eight(p, h);

	return 0;
} /* end of `main' */

/* Extract densities for each fluid from a PhaseSpec (for testing purposes) */
void extract_rhos(PhaseSpec *PS, unsigned npure, double *rhos){
#define PPH PS->PH
#define NPHASE PS->phases
#define NPURE PPH[i]->pures
	unsigned i, j;

	for(i=0;i<npure;i++){
		rhos[i] = -1; /* set all densities to minus one for later check */
	}
	for(i=0;i<2 && i<NPHASE;i++){
		for(j=0;j<NPURE;j++){
			rhos[PPH[i]->c[j]] = PPH[i]->rhos[j];
		}
	}
	for(i=0;i<npure;i++){
		if(rhos[i]==-1){
			ERRMSG("Density number %u was not assigned", i);
			rhos[i] = 1;
		}
	}
#undef NPURE
#undef NPHASE
#undef PPH
}

/* Functions to test/demonstrate other functionality */
void test_one(double *T, double *P){
#define NFLUIDS 4
#define NSIMS 6
	unsigned i, j; /* counter variable */
	int flash[NSIMS]   /* whether flash (i.e. root-finding in flash) succeeds */
		, rsat[NSIMS]; /* whether calculation of densities (i.e. root-finding) succeeds */
	enum FluidAbbrevs {N2,NH3,CO2,CH4,H2O}; /* fluids that will be used */

	char *fluids[]={
		"nitrogen", "ammonia", "carbondioxide", "methane", "water"
	};

	/* MixtureSpec *MS = new_MixtureSpec(NFLUIDS); */
	PhaseSpec *PS = new_PhaseSpec(NFLUIDS,3);
	FpropsError err = FPROPS_NO_ERROR;
	MixtureError merr = MIXTURE_NO_ERROR;

	char *source[NFLUIDS] = {NULL};
	double xs[NFLUIDS];                  /* mass fractions */
	double props[] = {2, 6, 4, 3, 5};    /* proportions of mass fractions */
	double tol = MIX_XTOL;               /* tolerance to which to find solutions */
	mixture_x_props(NFLUIDS, xs, props); /* mass fractions from proportions */

    MixtureSpec *MS = build_MixtureSpec(NFLUIDS, xs, (void **) fluids, "pengrob", source, &merr);

	double rho_mix[NSIMS] = {0}
		/* , u_mix[NSIMS] = {0}
		, h_mix[NSIMS] = {0}
		, cp_mix[NSIMS]
		, cv_mix[NSIMS]
		, s_mix[NSIMS]
		, g_mix[NSIMS]
		, a_mix[NSIMS] */
		;
	double rho_amix[NSIMS] = {0}
		, u_amix[NSIMS] = {0}
		, h_amix[NSIMS] = {0}
		, cp_amix[NSIMS] = {0}
		, cv_amix[NSIMS] = {0}
		, s_amix[NSIMS] = {0}
		, g_amix[NSIMS] = {0}
		, a_amix[NSIMS] = {0}
		, rhos_single[NFLUIDS] = {0}
		;
	double u_bmix[NSIMS] = {0}
		, h_bmix[NSIMS] = {0}
		, cp_bmix[NSIMS] = {0}
		, cv_bmix[NSIMS] = {0}
		, s_bmix[NSIMS] = {0}
		, g_bmix[NSIMS] = {0}
		, a_bmix[NSIMS] = {0}
		;
	double rhos[NSIMS][3] = {{0}}
		/* , u_ph[NSIMS][3] = {{0}}
		, h_ph[NSIMS][3] = {{0}}
		, cp_ph[NSIMS][3] = {{0}}
		, cv_ph[NSIMS][3] = {{0}}
		, s_ph[NSIMS][3] = {{0}}
		, g_ph[NSIMS][3] = {{0}}
		, a_ph[NSIMS][3] = {{0}} */
		;
	double u_bph[NSIMS][3] = {{0}}
		, h_bph[NSIMS][3] = {{0}}
		, cp_bph[NSIMS][3] = {{0}}
		, cv_bph[NSIMS][3] = {{0}}
		, s_bph[NSIMS][3] = {{0}}
		, g_bph[NSIMS][3] = {{0}}
		, a_bph[NSIMS][3] = {{0}}
		;
	unsigned nphase[NSIMS] = {0};

	int usr_cont=1;
	char temp_str[100];
	char *headers[NSIMS];

#define MIXTURE_CALC(PROP) old_mixture_##PROP(&PM, PROP##_ph[i], &err)
#define AMIXTURE_CALC(PROP) mixture_##PROP(&MT, &err)
#define BMIXTURE_CALC(PROP) mixture_##PROP(&PM, PROP##_bph[i], &err)
	for(i=0;i<NSIMS;i++){
		flash[i] = mixture_flash(PS, MS, T[i], P[i], tol, &err); /* flash mixture, obtaining phases */
		rsat[i]  = mixture_rhos_sat(PS, T[i], P[i], tol, &err);  /* find densities in each phase */
		if(flash[i] || rsat[i]){
			MSG("The flash-calculation function returned %i", flash[i]);
			MSG("The density-calculation function returned %i", rsat[i]);
			return;
		}
		if(err!=FPROPS_NO_ERROR){
			MSG("Error is %i; resetting it to zero", err);
			err = FPROPS_NO_ERROR;
		}
		extract_rhos(PS, MS->pures, rhos_single); /* find densities for one phase */
		nphase[i] = PS->phases;

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
		MixtureState MT = {
			.T = T[i]
			, .rhos = rhos_single
			, .X = MS
		};
		rho_mix[i] = mixture_rho(&PM, rhos[i]);
		/* u_mix[i] = MIXTURE_CALC(u);
		h_mix[i] = MIXTURE_CALC(h);
		cp_mix[i] = MIXTURE_CALC(cp); */

		rho_amix[i] = mixture_rho(&MT);
		u_amix[i] = AMIXTURE_CALC(u);
		h_amix[i] = AMIXTURE_CALC(h);
		cp_amix[i] = AMIXTURE_CALC(cp);
		cv_amix[i] = AMIXTURE_CALC(cv);
		s_amix[i] = AMIXTURE_CALC(s);
		g_amix[i] = AMIXTURE_CALC(g);
		a_amix[i] = AMIXTURE_CALC(a);

		u_bmix[i] = BMIXTURE_CALC(u);
		h_bmix[i] = BMIXTURE_CALC(h);
		cp_bmix[i] = BMIXTURE_CALC(cp);
		cv_bmix[i] = BMIXTURE_CALC(cv);
		s_bmix[i] = BMIXTURE_CALC(s);
		g_bmix[i] = BMIXTURE_CALC(g);
		a_bmix[i] = BMIXTURE_CALC(a);

		/* Fill in header */
		/* headers[i] = ASC_NEW_ARRAY(char,40); */
		/* snprintf(headers[i], 40, "[T=%.1f K, P=%.2f bar]", T[i], P[i]/1.e5); */
	}
#undef MIXTURE_CALC
#undef AMIXTURE_CALC

	/* for(i=0;i<NSIMS;i++){ 
		MSG("At T=%.1f K, P=%.0f Pa, the overall density is %g kg/m^3"
				, T[i], P[i], rho_mix[i]);
		MSG("  The overall internal energy is %g J/kg, and enthalpy is %g J/kg"
				, u_mix[i], h_mix[i]);
		MSG("  The overall constant-pressure heat capacity is %g J/kg", cp_mix[i]);
		for(j=0;j<nphase[i];j++){
			MSG("\tFor phase number %u, the phase density is %g kg/m^3", j, rhos[i][j]);
			MSG("\t  The phase internal energy is %g J/kg, and enthalpy is %g J/kg"
					, u_ph[i][j], h_ph[i][j]);
		}
	}
	puts(""); */
	for(i=0;i<NSIMS;i++){ 
		printf("\nAt T=%.1f K, P=%.0f Pa, the overall density is %g kg/m^3"
				, T[i], P[i], rho_mix[i]);
		printf("\n\tThe overall internal energy from macros is "
				"%g J/kg and the enthalpy from macros is %g J/kg"
				, u_bmix[i], h_bmix[i]);
		printf("\n\tThe overall constant-pressure heat capacity is %g J/kg", cp_bmix[i]);
		printf("\n\tThe overall constant-volume heat capacity is %g J/kg", cv_bmix[i]);
		printf("\n\tThe overall entropy is %g J/(kg K)", s_bmix[i]);
		printf("\n\tThe overall Gibbs energy is %g J/(kg K)", g_bmix[i]);
		printf("\n\tThe overall Helmholtz energy is %g J/(kg K)", a_bmix[i]);
		for(j=0;j<nphase[i];j++){
			printf("\n\tFor phase number %u, the phase density is %g kg/m^3", j, rhos[i][j]);
			printf("\n\t  The phase internal energy is %g J/kg, and enthalpy is %g J/kg"
					, u_bph[i][j], h_bph[i][j]);
			printf("\n\t  The phase constant-pressure heat capacity is %g J/kg", cp_bph[i][j]);
			printf("\n\t  The phase constant-volume heat capacity is %g J/kg", cv_bph[i][j]);
			printf("\n\t  The phase entropy is %g J/(kg K)", s_bph[i][j]);
			printf("\n\t  The phase Gibbs energy is %g J/kg, and Helmholtz energy is %g J/kg"
					, g_bph[i][j], a_bph[i][j]);
		}
		puts("");
	}
	puts("");
	for(i=0;i<NSIMS;i++){ 
		printf("\nAt T=%.1f K, P=%.0f Pa, the overall density from the old functions is %g kg/m^3"
				, T[i], P[i], rho_amix[i]);
		printf("\n\tOverall internal energy is %g J/kg", u_amix[i]);
		printf("\n\tOverall enthalpy is %g J/kg", h_amix[i]);
		printf("\n\tOverall constant-pressure heat capacity is %g J/kg", cp_amix[i]);
		printf("\n\tOverall constant-volume heat capacity is %g J/kg", cv_amix[i]);
		printf("\n\tOverall entropy is %g J/kg", s_amix[i]);
		printf("\n\tOverall Gibbs energy is %g J/kg", g_amix[i]);
		printf("\n\tOverall Helmholtz energy is %g J/kg", a_amix[i]);
		puts("");
	}
#undef NSIMS
#undef NFLUIDS
}

void test_seven(void){
#define NPURE 5
#define D MS->PF[i]->data
#define TEMPS 4
#define PRESSURES 3
	unsigned j, i;
	int flash, pdew, pbubl, Tdew, Tbubl;
	char *fluids[] = {
		"isohexane", "krypton", "carbonmonoxide", "ammonia", "water"
	};
	double props[] = {11, 4, 2, 3, 2};
	double Xs[NPURE]
		, p_d[TEMPS][PRESSURES] = {{0}} /* dew pressure */
		, p_b[TEMPS][PRESSURES] = {{0}} /* bubble pressure */
		, T_d[TEMPS][PRESSURES] = {{0}} /* dew temperature */
		, T_b[TEMPS][PRESSURES] = {{0}} /* bubble temperature */
		, tol = MIX_XTOL
		;

	mixture_x_props(NPURE, Xs, props);
	char *src[NPURE] = {NULL};
	FpropsError err = FPROPS_NO_ERROR;
	MixtureError merr = MIXTURE_NO_ERROR;

    MixtureSpec *MS = build_MixtureSpec(NPURE, Xs, (void **) fluids, "pengrob", src, &merr);

	double Ts[] = {270, 310, 350, 390, 430, 470}
		, Ps[] = {1.5e5, 2.5e5, 4e5, 6e5}
		;

	MSG("Declared all variables...");
    MSG("The mixture specification is at %p ; it holds %u components", MS, MS->pures);
    for(i=0;i<NPURE;i++){
        MSG("  Component number %u is %s, and has mass fraction %g"
                , i, MS->PF[i]->name, MS->Xs[i]);
    }

	PhaseSpec ***PS_2 = ASC_NEW_ARRAY(PhaseSpec **,TEMPS);
	for(i=0;i<TEMPS;i++){
		PS_2[i] = ASC_NEW_ARRAY(PhaseSpec *,PRESSURES);
		for(j=0;j<PRESSURES;j++){
			PS_2[i][j] = ASC_NEW(PhaseSpec);

			flash = mixture_flash(PS_2[i][j], MS, Ts[i], Ps[j], tol, &err);

			pdew  = mixture_dew_pressure((p_d[i]+j), MS, Ts[i], tol, &err);
			pbubl = mixture_bubble_pressure((p_b[i]+j), MS, Ts[i], tol, &err);

			Tdew  = mixture_dew_temperature((T_d[i]+j), MS, Ps[j], tol, &err);
			Tbubl = mixture_bubble_temperature((T_b[i]+j), MS, Ps[j], tol, &err);

			if(flash || pdew || pbubl || Tdew || Tbubl){
				MSG("The flash-calculation function returned %i", flash);
				MSG("The dew-pressure function returned %i", pdew);
				MSG("The bubble-pressure function returned %i", pbubl);
				MSG("The dew-temperature function returned %i", Tdew);
				MSG("The bubble-temperature function returned %i", Tbubl);
				return;
			}
		}
	}

#define NPHASE PS_2[i1][i2]->phases
#define PFRAC PS_2[i1][i2]->ph_frac
#define PTYPE PS_2[i1][i2]->ph_type
#define PPH PS_2[i1][i2]->PH

#if 1
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
			printf("\n\tThe dew pressure is %.1f Pa", p_d[i1][i2]);
			printf("\n\tThe bubble pressure is %.1f Pa", p_b[i1][i2]);
            printf("\n\tThe dew temperature is %.2f K", T_d[i1][i2]);
            printf("\n\tThe bubble temperature is %.2f K", T_b[i1][i2]);
			puts("");
		}
	}
#endif

#undef PPH
#undef PTYPE
#undef PFRAC
#undef NPHASE

#undef PRESSURES
#undef TEMPS
#undef D
#undef NPURE
}

void test_eight(double *p, double *h){
#define NFLUIDS 4
#define NSIMS 3
	unsigned i, j; /* counter variable */
	int phT[NSIMS]; /* whether temperature seeking succeeds */

	enum FluidAbbrevs {N2,NH3,CO2,CH4,H2O}; /* fluids that will be used */
	char *fluids[]={
		"nitrogen", "ammonia", "carbondioxide", "methane", "water"
	};

	/* MixtureSpec *MS = new_MixtureSpec(NFLUIDS); */
	FpropsError err = FPROPS_NO_ERROR;
	MixtureError merr = MIXTURE_NO_ERROR;

	char *source[NFLUIDS] = {NULL};
	double xs[NFLUIDS];                  /* mass fractions */
	double props[] = {2, 6, 4, 3, 5};    /* proportions of mass fractions */
	double tol = MIX_XTOL;               /* tolerance to which to find solutions */
	mixture_x_props(NFLUIDS, xs, props); /* mass fractions from proportions */

    MixtureSpec *MS = build_MixtureSpec(NFLUIDS, xs, (void **) fluids, "pengrob", source, &merr);
	PhaseSpec **PS = ASC_NEW_ARRAY(PhaseSpec *,NSIMS);
	PhaseMixState **PM = ASC_NEW_ARRAY(PhaseMixState *,NSIMS);

	double T[NSIMS] = {0};
	double h_ph[3];

	for(i=0;i<NSIMS;i++){
		phT[i] = mixture_T_ph((T+i), MS, p[i], h[i], tol, &err);
		MSG("SUCCESSFULLY FOUND TEMPERATURE number %u to be %.2f K", i, T[i])
		PS[i] = new_PhaseSpec(NFLUIDS,3);

		mixture_flash(PS[i], MS, T[i], p[i], tol, &err);
		mixture_rhos_sat(PS[i], T[i], p[i], tol, &err);
		PM[i] = fill_PhaseMixState(T[i], p[i], PS[i], MS);
		MSG("ENTHALPY at temperature %.2f K, pressure %.2f Pa, (original h= %g J/kg):"
				, T[i], p[i], h[i]);
		MSG("\tEnthalpy is %g J/kg", mixture_h(PM[i], h_ph, &err));
	}
	for(i=0;i<NSIMS;i++){
		MSG("At P=%.2f Pa, h=%.2f J/kg, the temperature is %.2f K", p[i], h[i], T[i]);
		MSG("  This gives an enthalpy of %g J/kg", mixture_h(PM[i], h_ph, &err));
	}
#undef NSIMS
#undef NFLUIDS
}
