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
#ifndef FPROPS_DERIV_H
#define FPROPS_DERIV_H

#include "helmholtz.h"

#define FPROPS_CHAR int

typedef struct{
	double T;
	double rho;
	double psat;
	double rhof;
	double rhog;
	double dpdT_sat;
	const HelmholtzData *D;
} StateData;

double fprops_deriv(FPROPS_CHAR z, FPROPS_CHAR x, FPROPS_CHAR y, double T, double rho, const HelmholtzData *D);

double fprops_non_dZdv_T(FPROPS_CHAR x, double T, double rho, const HelmholtzData *D);
double fprops_non_dZdT_v(FPROPS_CHAR x, double T, double rho, const HelmholtzData *D);

/* the StateData object must be filled in/calculated external to the following
functions */

double fprops_sat_dZdT_v(FPROPS_CHAR z, const StateData *S);
double fprops_sat_dZdv_T(FPROPS_CHAR z, const StateData *S);

double fprops_drhofdT(const StateData *S);
double fprops_drhogdT(const StateData *S);

#endif

