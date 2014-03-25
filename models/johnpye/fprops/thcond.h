/*	ASCEND modelling environment
	Copyright (C) 2014 John Pye

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
#include "rundata.h"
#include "fprops.h"

void thcond_prepare(PureFluid *P, const ThermalConductivityData *K, FpropsError *err);

double thcond1_lam0(FluidState state, FpropsError *err);
/**< zero-density component of thermal conductivity [W/m/K] */

double thcond1_lamr(FluidState state, FpropsError *err);
/**< residual component of thermal conductivity [W/m/K] */

double thcond1_k(FluidState state, FpropsError *err);
/**< thermal conductivity [W/m/K] */

