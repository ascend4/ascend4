/*	ASCEND modelling environment
	Copyright (C) 2008-2009 Carnegie Mellon University

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
*//** @file
	Routines to calculate saturation properties using Helmholtz correlation
	data.
	Using the iterative algorithm of Akasaka, see FPROPS wiki page for details.
*/
#ifndef FPROPS_SAT3_H
#define FRPOPS_SAT3_H

#include "helmholtz.h"
#include "sat.h"

int fprops_sat_T_akasaka(double T, double *psat_out, double *rhof_out, double * rhog_out, const HelmholtzData *d);

#endif

