/*	ASCEND modelling environment
	Copyright (C) 2008-2013 John Pye

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
#ifndef FPROPS_INCOMP_H
#define FPROPS_INCOMP_H

#include "rundata.h"

/**
	Prepare a PureFluid object for evaluation of incompressible liquid/solid properties,
	from an appropriate EosData input, using the specified ReferenceState for
	enthalpy and entropy scales.
*/
PureFluid *incomp_prepare(const EosData *E, const ReferenceState *ref);

void incomp_destroy(PureFluid *data);

// these functions should be private, but are currently used in refstate.c and in testing.
double incomp_h(FluidStateUnion vals, const FluidData *data, FpropsError *err);
double incomp_s(FluidStateUnion vals, const FluidData *data, FpropsError *err);

# ifdef CUNIT_TEST
double incomp_rho(FluidStateUnion vals, const FluidData *data, FpropsError *err);
# endif

#endif // FPROPS_INCOMP_H
