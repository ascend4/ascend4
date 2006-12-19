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
*//** @file
	C-code implementation of Art Westerberg's DAE integrator
*//*
	by John Pye, Dec 2006.
*/

#ifndef ASC_INTEGRATOR_AWW_H
#define ASC_INTEGRATOR_AWW_H

#include <utilities/config.h>
#include "integrator.h"

/* Perform IDA integration of a specified 'Integ_system'.*/
ASC_DLLSPEC(int) integrator_aww_solve(IntegratorSystem *blsys
		, unsigned long start_index, unsigned long finish_index
);

ASC_DLLSPEC(void) integrator_aww_free(void *enginedata);

ASC_DLLSPEC(void) integrator_aww_create(IntegratorSystem *blsys);

ASC_DLLSPEC(int) integrator_aww_analyse(IntegratorSystem *blsys);

IntegratorParamsDefaultFn integrator_aww_params_default;

#endif  /* ASC_INTEGRATOR_AWW_H */
