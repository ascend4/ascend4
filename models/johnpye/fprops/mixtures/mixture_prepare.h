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
	by Jacob Shealy, June 26-, 2015

	Prototypes for functions to prepare structs to represent mixtures to model
 */

#ifndef MIX_PREPARE_HEADER
#define MIX_PREPARE_HEADER

#include "mixture_generics.h"
#include "mixture_struct.h"

void mixture_specify(MixtureSpec *MS, unsigned npure, double *Xs, void **fluids, char *type, char **source, MixtureError *merr);
void mixture_fluid_spec(MixtureSpec *MS, unsigned npure, void **fluids, char *type, char **source, MixtureError *merr);

#endif
