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

typedef struct FluidState_struct{
	double T;
	double rho;
	const PureFluid *fluid; ///< pointer to fluid description and associated functions
} FluidState;

FluidState2 fprops_set_Trho(double T, double rho, const PureFluid *fluid, FpropsError *err);

/**< Use this function if you want to set the state of a fluid that uses (T,rho) for
its internal state, such as Helmholtz, Ideal, Pengrob.
*/
	
FluidState2 fprops_set_Tp(double T, double p, const PureFluid *fluid, FpropsError *err);
/**< Use this function if you want to set the state of a fluid that uses (T,p) for
	its internal state, such as Helmholtz, Ideal, Pengrob.
*/

/* TODO we need to add a way to specify what fluid correlation is desired
and also what reference state, as another option. */

//FpropsError* fprops_get_err_pointer(void);

/*The following take the fluid data and use the function pointers
  to call the correct function (e.g. fprops_p -> helmholtz_p)*/

double fprops_T(FluidState2 state, FpropsError *err);   ///< Temperature / [K]
double fprops_rho(FluidState2 state, FpropsError *err); ///< Density / [kg/m3]
double fprops_v(FluidState2 state, FpropsError *err); ///< Density / [kg/m3]
double fprops_p(FluidState2 state, FpropsError *err);   ///< Pressure / [Pa]
double fprops_u(FluidState2 state, FpropsError *err);   ///< Specific internal energy / [J/kg]
double fprops_h(FluidState2 state, FpropsError *err);   ///< Specific enthalpy / [J/kg]
double fprops_s(FluidState2 state, FpropsError *err);   ///< Specific entropy / [J/kg/K]
double fprops_a(FluidState2 state, FpropsError *err);   ///< Specific helmholtz energy / [J/kg]
double fprops_cv(FluidState2 state, FpropsError *err);  ///< Specific isochoric heat capacity / [J/kg/K]
double fprops_cp(FluidState2 state, FpropsError *err);  ///< Specific isobaric heat capacity / [J/kg/K]
double fprops_w(FluidState2 state, FpropsError *err);   ///< Speed of sound / [m/s]
double fprops_g(FluidState2 state, FpropsError *err);   ///< Specific Gibbs energy / [J/kg]

double fprops_alphap(FluidState2 state, FpropsError *err);
double fprops_betap(FluidState2 state, FpropsError *err);

double fprops_cp0(FluidState2 state, FpropsError *err); ///< Specific isobaric heat capacity at zero pressure / [J/kg/K] (ideal gas limit)

/**	\brief Partial derivative of pressure wrt temperature at density constant.
	\f$\left(\frac{\partial p}{\partial T}\right)_{\rho}\f$
*/
double fprops_dpdT_rho(FluidState2 state, FpropsError *err);

/// return the fluid quality; 0 if subcooled, 1 if superheated, error if both T>T_c and p>p_c
double fprops_x(FluidState2 state, FpropsError *err);

#if 1
double fprops_dpdrho_T(const FluidState2 state, FpropsError *err);
double fprops_d2pdrho2_T(const FluidState2 state, FpropsError *err);

double fprops_dhdT_rho(const FluidState2 state, FpropsError *err);
double fprops_dhdrho_T(const FluidState2 state, FpropsError *err);

double fprops_dudT_rho(const FluidState2 state, FpropsError *err);
double fprops_dudrho_T(const FluidState2 state, FpropsError *err);
#endif


double fprops_mu(FluidState2 state, FpropsError *err); ///< Dynamic viscosity / [Pa*s]
double fprops_lam(FluidState2 state, FpropsError *err); ///< Thermal conductivity / [W/m/K]

/**
	Convert file data E into a PureFluid object, doing any necessary pre-calculation
	along the way. The PureFluid should implement the named correlation type.
	@return NULL on failure.
*/
PureFluid *fprops_prepare(const EosData *E, const char *corrtype);

/* TODO what about a function to destroy the PureFluid structure? */

/**
	Check if the file data E is suitable for preparing a PureFluid
	of the named correlation type.
	If corrtype is NULL, return the 'best' available correlation in the data.
	@return 0 if not available, else the corresponding EosType value if available.
*/
int fprops_corr_avail(const EosData *E, const char *corrtype);

const char *fprops_corr_type(EosType type);

const char *fprops_refstate_type(ReferenceStateType type);

char *fprops_error(FpropsError err);
#endif
