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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef FPROPS_DERIV_H
#define FPROPS_DERIV_H

#include "fprops.h"

#define FPROPS_CHAR int

/* a locally-used struct for passing state together with some saturation properties */
typedef struct SatStateData_struct SatStateData;

double fprops_deriv(FluidState state, char *vars, FpropsError *err);

double fprops_non_dZdv_T(FPROPS_CHAR z, double T, double rho, const PureFluid *fluid, FpropsError *err);
double fprops_non_dZdT_v(FPROPS_CHAR z, double T, double rho, const PureFluid *fluid, FpropsError *err);

/* the StateData object must be filled in/calculated external to the following
functions */

double fprops_sat_dZdT_v(FPROPS_CHAR z, const SatStateData *ssd, FpropsError *err);
double fprops_sat_dZdv_T(FPROPS_CHAR z, const SatStateData *ssd, FpropsError *err);

double fprops_drhofdT(const SatStateData *ssd, FpropsError *err);
double fprops_drhogdT(const SatStateData *ssd, FpropsError *err);

#endif

