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

/* TODO we need to add a way to specify what fluid correlation is desired
and also what reference state, as another option. */

//FpropsError* fprops_get_err_pointer(void);

/*The following take the fluid data and use the function pointers
  to call the correct function (e.g. fprops_p -> helmholtz_p)*/
double fprops_p(double T, double rho, const PureFluid *data, FpropsError *err);
double fprops_u(double T, double rho, const PureFluid *data, FpropsError *err);
double fprops_h(double T, double rho, const PureFluid *data, FpropsError *err);
double fprops_s(double T, double rho, const PureFluid *data, FpropsError *err);
double fprops_a(double T, double rho, const PureFluid *data, FpropsError *err);
double fprops_cv(double T, double rho, const PureFluid *data, FpropsError *err);
double fprops_cp(double T, double rho, const PureFluid *data, FpropsError *err);
double fprops_w(double T, double rho, const PureFluid *data, FpropsError *err);
double fprops_g(double T, double rho, const PureFluid *data, FpropsError *err);

double fprops_alphap(double T, double rho, const PureFluid *data, FpropsError *err);
double fprops_betap(double T, double rho, const PureFluid *data, FpropsError *err);

double fprops_cp0(double T, const PureFluid *data, FpropsError *err);

double fprops_dpdT_rho(double T, double rho, const PureFluid *data, FpropsError *err);


double fprops_dpdrho_T(double T, double rho, const PureFluid *data, FpropsError *err);
#if 0
double fprops_d2pdrho2_T(double T, double rho, const PureFluid *data, FpropsError *err);

double fprops_dhdT_rho(double T, double rho, const PureFluid *data, FpropsError *err);
double fprops_dhdrho_T(double T, double rho, const PureFluid *data, FpropsError *err);

double fprops_dudT_rho(double T, double rho, const PureFluid *data, FpropsError *err);
double fprops_dudrho_T(double T, double rho, const PureFluid *data, FpropsError *err);
#endif

PureFluid *fprops_prepare(const EosData *E, const char *corrtype);
/**<
	Convert file data E into a PureFluid object, doing any necessary pre-calculation
	along the way. The PureFluid should implement the named correlation type.
	@return NULL on failure.
*/

int fprops_source_match(const EosData *E, const char *source);
/**<
	Check that fluid contains source as a substring of the E->source field.
	This mechanism can be used to force lower-quality data to be used if
	normally it is overruled by higher-quality source data.
*/

int fprops_corr_avail(const EosData *E, const char *corrtype);
/**<
	Check if the file data E is suitable for preparing a PureFluid
	of the named correlation type.
	If corrtype is NULL, return the 'best' available correlation in the data.
	@return 0 if not available, else the corresponding EosType value if available.
*/


char *fprops_error(FpropsError err);
extern double tnbpref,vref;//hack to get liquid density and pressure at IIR reference state
#endif
