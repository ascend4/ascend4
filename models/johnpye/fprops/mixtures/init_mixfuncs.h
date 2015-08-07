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

/* Function prototypes */
/* void ig_rhos(MixtureState *M, double P); */
/* void initial_rhos(MixtureState *M, double P, FpropsError *err); */
/* void pressure_rhos(MixtureState *M, double P, double tol, FpropsError *err); */
/* void densities_to_mixture(MixtureState *M, double tol, FpropsError *err); */

double amixture_rho(MixtureState *M);
double amixture_u(MixtureState *M, FpropsError *err);
double amixture_h(MixtureState *M, FpropsError *err);
double amixture_cp(MixtureState *M, FpropsError *err);
double amixture_cv(MixtureState *M, FpropsError *err);
double mixture_x_ln_x(unsigned nPure, double *Xs, PureFluid **PFs);
double amixture_s(MixtureState *M, FpropsError *err);
double amixture_g(MixtureState *M, FpropsError *err);
double amixture_a(MixtureState *M, FpropsError *err);

/* void print_mixture_properties(char *how_calc, double rho, double u, double h, double cp, double cv, double s, double g, double a); */
/* void print_substances_properties(unsigned subst, char **headers, double *Xs, double *rhos, double *ps, double *us, double *hs, double *cps, double *cvs, double *ss, double *gs, double *as); */
/* void print_cases_properties(unsigned cases, char **headers, double *rhos, double *ps, double *us, double *hs, double *cps, double *cvs, double *ss, double *gs, double *as); */

#endif
