/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University

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
*//**
	@file
	Sample list for ASCEND integrator. Keeps a list of sample times
	for which model data will be computed during the integration.

	Will include functions to initialise various kinds of sample
	vectors, from linear-spaced to log-spaced, etc.

	This is a nicely self-contained 'class' that can be wrapped up and
	interfaced with SWIG/C++ for the PyGTK interface, and interfaced via
	the current functions in Integrators.c for the needs of the Tcl/Tk GUI.
*//*
	by John Pye, May 2006.
*/

#ifndef ASC_SAMPLELIST_H
#define ASC_SAMPLELIST_H

/**	@addtogroup integrator Integrator
	@{
*/

#include <utilities/ascConfig.h>
#include <compiler/fractions.h>
#include <compiler/compiler.h>
#include <compiler/dimen.h>

/** An array of sample 'times' for reference during integration */
struct SampleList_struct {
  long ns;
  double *x;
  const dim_type *d;
};

typedef struct SampleList_struct SampleList;

ASC_DLLSPEC void samplelist_free(SampleList *);

ASC_DLLSPEC SampleList *samplelist_new(unsigned long n, const dim_type *d);

/* assign to a SampleList, freeing stuff if there's some there. */

ASC_DLLSPEC int samplelist_assign(SampleList *l, unsigned long n, double *values, const dim_type *d);

ASC_DLLSPEC long samplelist_length(CONST SampleList *l);
ASC_DLLSPEC const dim_type *samplelist_dim(CONST SampleList *l);

/**
	Get an element from the samplelist. Value of the index i should be in [0,ns).
*/
ASC_DLLSPEC double samplelist_get(CONST SampleList *l, CONST long i);
ASC_DLLSPEC void samplelist_set(CONST SampleList *l, CONST long i, CONST double x);

void samplelist_debug(CONST SampleList *l);

/* @} */

#endif
