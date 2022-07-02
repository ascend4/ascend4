/*	ASCEND modelling environment
	Copyright (C) 2011 Carnegie Mellon University

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
#ifndef PENGROB_H
#define PENGROB_H

#include "rundata.h"

PureFluid *pengrob_prepare(const EosData *data, const ReferenceState *ref);

void pengrob_destroy(PureFluid *P);

/**
	Solve pengrob cubic EOS for pressure and temperature. Maybe we will
	generalise this at some point.
	
	Returns `rho`, or sets `err` if unable to solve.
	FIXME document if use_guess used?
*/
void pengrob_solve_pT(double p,double T, double *rho
	,FluidData *data, FpropsError *err
);

#endif //PENGROB_H
