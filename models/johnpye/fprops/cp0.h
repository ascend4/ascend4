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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*/
#ifndef FPROPS_CP0_H
#define FPROPS_CP0_H

#include "rundata.h"

Phi0RunData *cp0_prepare(const IdealData *I, double R, double Tstar);

double ideal_phi(double tau, double delta, const Phi0RunData *data);
double ideal_phi_tau(double tau, double delta, const Phi0RunData *data);
double ideal_phi_tautau(double tau, const Phi0RunData *data);

#endif

