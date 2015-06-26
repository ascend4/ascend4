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
	by Jacob Shealy, June 5-, 2015

	Function headers for initial model of ideal-solution mixing.  Removed these 
	from the test files init_mix1, init_mix2, etc. to de-clutter them.
 */

#ifndef INIT_MIXTURE_HEADER
#define INIT_MIXTURE_HEADER

#include "mixture_struct.h"
#include "../helmholtz.h"
#include "../fluids.h"
#include "../fprops.h"
#include "../refstate.h"
#include "../sat.h"

#include <stdio.h>
#include <math.h>

#define MIX_ERROR "  ERROR: "
#define MIX_XSUM_ERROR MIX_ERROR "the sum over all mass fractions, which should be exactly 1.00, is %.10f\n"

#define PREPARE_TABLE(ROWS,COLS,T_HEAD,T_SIDE,T_VALS,T_FORM,T_CONT) \
	for(i2=0;i2<COLS-1;i2++){ \
		T_CONT[0][i2+1] = T_HEAD[i2]; \
	} \
	for(i1=0;i1<ROWS;i1++){ \
		T_CONT[i1][0] = T_SIDE[i1]; \
	} \
	for(i1=0;i1<ROWS-1;i1++){ \
		for(i2=0;i2<COLS-1;i2++){ \
			T_CONT[i1+1][i2+1] = (char *)malloc(20); \
			snprintf(T_CONT[i1+1][i2+1], 100, T_FORM[i1], T_VALS[i1][i2]); \
		} \
	}

#define PRINT_STR_TABLE(ROWS,COLS,CWIDTH,CELLS) \
	for(i1=0;i1<ROWS;i1++){ \
		for(i2=0;i2<COLS;i2++){ \
			if(strlen(CELLS[i1][i2])>=CWIDTH[i2]){ \
				CWIDTH[i2] = strlen(CELLS[i1][i2]); \
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

typedef double SecantSubjectFunction(double, void *user_data);

/* Function prototypes */
void mixture_x_props(unsigned nPure, double *Xs, double *props);
double mixture_x_fill_in(unsigned nPure, double *Xs);
void ig_rhos(MixtureState *M, double P, char **Names);
void initial_rhos(MixtureState *M, double P, char **names, FpropsError *err);
void pressure_rhos(MixtureState *M, double P, double tol, /* char **Names, */ FpropsError *err);
void densities_to_mixture(MixtureState *M, double tol, char **Names, FpropsError *err);

MixtureSpec *mixture_specify(unsigned npure, double *Xs, void **fluids, char *type, char *source);

double mixture_rho(MixtureState *M);
double mixture_u(MixtureState *M, FpropsError *err);
double mixture_h(MixtureState *M, FpropsError *err);
double mixture_cp(MixtureState *M, FpropsError *err);
double mixture_cv(MixtureState *M, FpropsError *err);
double mixture_x_ln_x(unsigned nPure, double *Xs, PureFluid **PFs);
double mixture_M_avg(unsigned nPure, double *Xs, PureFluid **PFs);
double mixture_s(MixtureState *M, FpropsError *err);
double mixture_g(MixtureState *M, FpropsError *err);
double mixture_a(MixtureState *M, FpropsError *err);

void print_mixture_properties(char *how_calc, double rho, double u, double h, double cp, double cv, double s, double g, double a);
void print_substances_properties(unsigned subst, char **headers, double *Xs, double *rhos, double *ps, double *us, double *hs, double *cps, double *cvs, double *ss, double *gs, double *as);
void print_cases_properties(unsigned cases, char **headers, double *rhos, double *ps, double *us, double *hs, double *cps, double *cvs, double *ss, double *gs, double *as);

#endif

