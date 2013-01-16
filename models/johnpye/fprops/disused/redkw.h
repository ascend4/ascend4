/*
 * redkw.h
 * This file is part of ASCEND
 *
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
#ifndef REDKW_H
#define REDKW_H

double redkw_p  (double tau, double delta, const FluidData *data, const FpropsError *err);
#if 0
double redkw_Z  (double tau, double   p, const FluidData *data, const FpropsError *err);
double redkw_Vm (double tau, double   p, const FluidData *data, const FpropsError *err);
double redkw_rho(double tau, double   p, const FluidData *data, const FpropsError *err);
double redkw_T  (double p, double delta, const FluidData *data, const FpropsError *err);
#endif

double redkw_a_depart(double tau, double delta, const FluidData *data, const FpropsError *err);
double redkw_s_depart(double tau, double delta, const FluidData *data, const FpropsError *err);
double redkw_h_depart(double tau, double delta, const FluidData *data, const FpropsError *err);
double redkw_u_depart(double tau, double delta, const FluidData *data, const FpropsError *err);
double redkw_g_depart(double tau, double delta, const FluidData *data, const FpropsError *err);

//Haven't worked these out yet:
double redkw_cv(double tau, double delta, const FluidData *data, const FpropsError *err);
double redkw_cp(double tau, double delta, const FluidData *data, const FpropsError *err);
double redkw_w(double tau, double delta, const FluidData *data, const FpropsError *err);

//Helper functions
#if 0
void redkw_get_coeffs(double T, RedKwCoeffs *coeffs, const CubicData *data, const FpropsError *err);
void redkw_refresh_coeffs(double T, RedKwCoeffs *coeffs, const CubicData *data, const FpropsError *err);
#endif
#endif //REDKW_H
