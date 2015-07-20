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
	by Jacob Shealy, June 25-, 2015

	Function headers for generic functions used in modeling mixtures
 */

#ifndef MIX_GENERICS_HEADER
#define MIX_GENERICS_HEADER

#if 0
#include <ascend/general/ascMalloc.h>
#else
#define ASC_NEW(TYPE) (TYPE*)malloc(sizeof(TYPE))
#define ASC_NEW_ARRAY(TYPE,COUNT) (TYPE*)malloc(sizeof(TYPE)*(COUNT))
#endif

#include "mixture_struct.h"
#include <stdio.h>

double min_element(unsigned nelems, double *nums);
int min_positive_elem(double *min, unsigned nelems, double *nums);
double max_element(unsigned nelems, double *nums);
double sum_elements(unsigned nelems, double *nums);
unsigned index_of_min(unsigned nelems, double *nums);
unsigned index_of_max(unsigned nelems, double *nums);
void secant_solve(SecantSubjectFunction *func, void *user_data, double x[2], double tol);
void mole_fractions(unsigned n_pure, double *x_mole, double *X_mass, PureFluid **PF);

#endif
