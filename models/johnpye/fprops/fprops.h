/*
 * FPROPS
 * Copyright (C) 2011 - Carnegie Mellon University
 *
 * ASCEND is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ASCEND is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASCEND; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef FPROPS_H
#define FPROPS_H

#include "rundata.h"

/**
	State object for FPROPS. This struct allows user-friendly API in a similar
	way to in freesteam, but supports different fluid types and correlations,
	and might be extensible to support fluid mixtures.

	TODO if we've got a saturated state then we almost certainly have calculated
	rhof, rhog, p. They're expensive, so we should save them in this state
	struct, too, or else find some other way to cache them.

	TODO perhaps eventually we can different different correlations using
	different independent variables, in which case this state could be 
*/
typedef struct FluidState_struct{
	double T; ///< temperature / K
	double rho; ///< density / kg/m3
	const PureFluid *fluid; ///< pointer to fluid description and associated functions
} FluidState;

FluidState fprops_set_Trho(double T, double rho, const PureFluid *fluid, FpropsError *err);

/* TODO we need to add a way to specify what fluid correlation is desired
and also what reference state, as another option. */

//FpropsError* fprops_get_err_pointer(void);

/*The following take the fluid data and use the function pointers
  to call the correct function (e.g. fprops_p -> helmholtz_p)*/
double fprops_p(FluidState state, FpropsError *err);
double fprops_u(FluidState state, FpropsError *err);
double fprops_h(FluidState state, FpropsError *err);
double fprops_s(FluidState state, FpropsError *err);
double fprops_a(FluidState state, FpropsError *err);
double fprops_cv(FluidState state, FpropsError *err);
double fprops_cp(FluidState state, FpropsError *err);
double fprops_w(FluidState state, FpropsError *err);
double fprops_g(FluidState state, FpropsError *err);

double fprops_alphap(FluidState state, FpropsError *err);
double fprops_betap(FluidState state, FpropsError *err);

double fprops_cp0(FluidState state, FpropsError *err);

double fprops_dpdT_rho(FluidState state, FpropsError *err);

/// return the fluid quality; 0 if subcooled, 1 if superheated, error if both T>T_c and p>p_c
double fprops_x(FluidState state, FpropsError *err);

#if 1
double fprops_dpdrho_T(const FluidState state, FpropsError *err);
double fprops_d2pdrho2_T(const FluidState state, FpropsError *err);

double fprops_dhdT_rho(const FluidState state, FpropsError *err);
double fprops_dhdrho_T(const FluidState state, FpropsError *err);

double fprops_dudT_rho(const FluidState state, FpropsError *err);
double fprops_dudrho_T(const FluidState state, FpropsError *err);
#endif

/**
	Convert file data E into a PureFluid object, doing any necessary pre-calculation
	along the way. The PureFluid should implement the named correlation type.
	@return NULL on failure.
*/
PureFluid *fprops_prepare(const EosData *E, const char *corrtype);

/**
	Check if the file data E is suitable for preparing a PureFluid
	of the named correlation type.
	If corrtype is NULL, return the 'best' available correlation in the data.
	@return 0 if not available, else the corresponding EosType value if available.
*/
int fprops_corr_avail(const EosData *E, const char *corrtype);

char *fprops_corr_type(EosType type);

char *fprops_error(FpropsError err);
#endif
