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
#ifndef FPROPS_INCOMPRESSIBLE_H
#define FPROPS_INCOMPRESSIBLE_H

#include "rundata.h"

/**
	Prepare a PureFluid object for evaluation of ideal gas fluid properties,
	from an appropriate EosData input, using the specified ReferenceState for
	enthalpy and entropy scales.
*/
PureFluid *incompressible_prepare(const EosData *E, const ReferenceState *ref);

PropEvalFn incompressible_p;
PropEvalFn incompressible_u;
PropEvalFn incompressible_h;
PropEvalFn incompressible_s;
PropEvalFn incompressible_a;
PropEvalFn incompressible_g;
PropEvalFn incompressible_cp;
PropEvalFn incompressible_cv;
PropEvalFn incompressible_w;
PropEvalFn incompressible_alphap;
PropEvalFn incompressible_betap;
PropEvalFn incompressible_dpdrho_T;
SatEvalFn incompressible_sat;

#define HELM_INCOMPRESSIBLE_DELTAU(TAU, DELTA, DATA, ERROR) (0)

#endif

