/*	ASCEND modelling environment
	Copyright (C) 2015 Sidharth, John Pye

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
*//** @file
Tabulated Taylor Series Expansion (TTSE) property evaluation
in FPROPS. For more details see http://ascend4.org/User:Sidharth
*/

#define NSAT 500

void alloc_tables();
PureFluid * ttse_prepare(const EosData *E, const ReferenceState *ref);
void ttse_destroy(PureFluid *P);

double ttse_sat(double T, double *rhof_out, double * rhog_out, const FluidData *data, FpropsError *err);


PropEvalFn ttse_p;
PropEvalFn ttse_u;
PropEvalFn ttse_h;
PropEvalFn ttse_s;
PropEvalFn ttse_a;
PropEvalFn ttse_g;
PropEvalFn ttse_cp;
PropEvalFn ttse_cv;
PropEvalFn ttse_w;
PropEvalFn ttse_dpdrho_T;
PropEvalFn ttse_alphap;
PropEvalFn ttse_betap;
SatEvalFn ttse_sat;
