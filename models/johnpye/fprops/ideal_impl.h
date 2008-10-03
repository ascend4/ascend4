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

#ifndef FPROPS_IDEAL_IMPL_H
#define FPROPS_IDEAL_IMPL_H

#include "ideal.h"

/*
	This file contains the headers for the private code definedin 'ideal.c'.
	You shouldn't include this file in your programs, because the implementation
	of the ideal gas curves is 'secret business' of the fprops code.
*/

double helm_ideal(double tau, double delta, const IdealData *data);
double helm_ideal_tau(double tau, double delta, const IdealData *data);
double helm_ideal_tautau(double tau, const IdealData *data);

double helm_cp0(double T, const IdealData *data);

/*
	Note: the cross partial derivative with respect to delta and tau is
	identically zero
*/
#define HELM_IDEAL_DELTAU(TAU, DELTA, DATA) (0)


#endif

