/*	ASCEND modelling environment 
	Copyright (C) Carnegie Mellon University 

	This program is free software; you can redistribute it and/or modify 
	it under the terms of the GNU General Public License as published by 
	the Free Software Foundation; either version 2, or (at your option) 
	any later version.

	This program is distributed in the hope that it will be useful, but 
	WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
	General Public License for more details.

	You should have received a copy of the GNU General Public License 
	along with this program; if not, write to the Free Software 
	Foundation --

	Free Software Foundation, Inc.
	59 Temple Place - Suite 330
	Boston, MA 02111-1307, USA.
*//*
	by Jacob Shealy, July 30-,2015

	Headers for functions that calculate mixture properties.
 */

#ifndef MIX_PROPERTIES_HEADER
#define MIX_PROPERTIES_HEADER

#include "mixture_struct.h"
#include "../helmholtz.h"
#include "../fluids.h"
#include "../fprops.h"
#include "../refstate.h"
#include "../sat.h"

#include <stdio.h>
#include <math.h>

typedef double MixPropertyFunc(PhaseMixState *PM, double *p_phases, FpropsError *err);

int mixture_rhos_sat(PhaseSpec *PS, double T, double P, FpropsError *err);
double mixture_rho(PhaseMixState *PM, double *rhos);
/* MixPropertyFunc old_mixture_u;
MixPropertyFunc old_mixture_h;
MixPropertyFunc old_mixture_cp; */

MixPropertyFunc mixture_u;
MixPropertyFunc mixture_h;
MixPropertyFunc mixture_cp;
MixPropertyFunc mixture_cv;
MixPropertyFunc mixture_s;
MixPropertyFunc mixture_g;
MixPropertyFunc mixture_a;

#endif
