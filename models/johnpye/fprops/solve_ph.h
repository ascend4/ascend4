/*
ASCEND modelling environment
Copyright (C) 2004-2010 John Pye

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
#ifndef FPROPS_SOLVE_PH_H
#define FPROPS_SOLVE_PH_H

#include "rundata.h"

int fprops_region_ph(double p, double h, const PureFluid *fluid, FpropsError *err);

/**
	Function to solve fluid state for given (p,h). Check the performance of this function
	for your fluid of interest using the script python/solve_ph_array.py.
	NOTE: a previous version of this function had a 'use_guess' flag, which had not
	been fully implemented.
*/
FluidState2 fprops_solve_ph(double p, double h, const PureFluid *fluid, FpropsError *err);

#if 0
/* functions for reporting steps back to python */
typedef struct{
	double x,y;
} StepData;

stepdata_reset();
int stepdata_record(double x, double y);
stepdata_count();
StepData stepdata_get(int i);
#endif

#endif
