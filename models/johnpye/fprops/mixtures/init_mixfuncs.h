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

#include "../helmholtz.h"
#include "../fluids.h"
#include "../fprops.h"
#include "../refstate.h"
#include "../sat.h"

#include <stdio.h>
/* #include <assert.h> */
#include <math.h>

#define MIX_XTOL 1e-6
#define MIX_ERROR "  ERROR: "
#define MIX_XSUM_ERROR MIX_ERROR "the sum over all mass fractions, which should be exactly 1.00, is %.10f\n"

/* Function prototypes */
void mixture_x_props(unsigned nPure, double *xs, double *props);
double mixture_x_fill_in(unsigned nPure, double *xs);
void ig_rhos(double *rho_out, unsigned nPure, double T, double P, PureFluid **I, char **Names);
void initial_rhos(double *rho_out, unsigned nPure, double T, double P, PureFluid **PFs, char **names, FpropsError *err);
void pressure_rhos(double *rho_out, unsigned nPure, double T, double P, double tol, PureFluid **PF, char **Names, FpropsError *err);
double mixture_rho(unsigned nPure, double *x, double *rhos);
double mixture_u(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err);
double mixture_h(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err);
double mixture_cp(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err);
double mixture_cv(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err);
double mixture_x_ln_x(unsigned nPure, double *xs, PureFluid **PFs);
double mixture_M_avg(unsigned nPure, double *xs, PureFluid **PFs);
double mixture_s(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err);
double mixture_g(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err);
double mixture_a(unsigned nPure, double *xs, double *rhos, double T, PureFluid **PFs, FpropsError *err);

#endif

