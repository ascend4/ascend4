/*	ASCEND modelling environment
	Copyright (C) 2008 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*/

#ifndef FPROPS_HELMHOLTZ_H
#define FPROPS_HELMHOLTZ_H

#define FPROPS_CHAR int

#include "rundata.h"
#include "ideal.h"

PureFluid *helmholtz_prepare(const EosData *data, const ReferenceState *ref);

/* these are the 'raw' functions, they don't do phase equilibrium. */
PropEvalFn helmholtz_p;
PropEvalFn helmholtz_u;
PropEvalFn helmholtz_h;
PropEvalFn helmholtz_s;
PropEvalFn helmholtz_a;
PropEvalFn helmholtz_g;
PropEvalFn helmholtz_cp;
PropEvalFn helmholtz_cv;
PropEvalFn helmholtz_w;
PropEvalFn helmholtz_dpdrho_T;
PropEvalFn helmholtz_alphap;
PropEvalFn helmholtz_betap;

double helmholtz_dpdT_rho(double T, double rho, const FluidData *data, FpropsError *err);
double helmholtz_d2pdrho2_T(double T, double rho, const FluidData *data, FpropsError *err);

double helmholtz_dhdT_rho(double T, double rho, const FluidData *data, FpropsError *err);
double helmholtz_dhdrho_T(double T, double rho, const FluidData *data, FpropsError *err);

double helmholtz_dudT_rho(double T, double rho, const FluidData *data, FpropsError *err);
double helmholtz_dudrho_T(double T, double rho, const FluidData *data, FpropsError *err);

#endif

