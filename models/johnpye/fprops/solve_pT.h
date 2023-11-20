/*
ASCEND modelling environment
Copyright (C) 2020-2022 John Pye

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef FPROPS_SOLVE_PT_H
#define FPROPS_SOLVE_PT_H

#include "rundata.h"

int fprops_region_pT(double p, double h, const PureFluid *fluid, FpropsError *err);

/**
	Function to solve fluid state for given (p,T).
*/
FluidState2 fprops_solve_pT(double p, double h, const PureFluid *fluid, FpropsError *err);

#endif
