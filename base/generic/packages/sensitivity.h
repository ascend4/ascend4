/*	ASCEND modelling environment
	Copyright (C) 1990-2006 Carnegie-Mellon University

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
	This file is part of the SLV solver.
*//** @file
	Sensitivity add-on for ASCEND
	by Tom Epperly, Kirk Abbot

	-- notes here have been moved to models/sensitivity/sensitivity.c --
*/

#ifndef ASC_SENSITIVITY_H
#define ASC_SENSITIVITY_H

#include <utilities/ascConfig.h>
#include <general/list.h>
#include <compiler/extfunc.h>

#include <compiler/instance_enum.h>
#include <compiler/compiler.h>

#include <linear/mtx.h>
#include <linear/mtx_basic.h>
#include <linear/mtx_perms.h>
#include <linear/mtx_query.h>
#include <linear/linsol.h>
#include <linear/linsolqr.h>
#include <linear/densemtx.h>

#include <solver/calc.h>
#include <solver/relman.h>
#include <solver/slv_stdcalls.h>
#include <solver/slv_client.h>

/*--------------------------------------------------
	The following functions are provided only for use by LSODE. We want to
	phase this out and replace with a new solver like IDA.
*/
ASC_DLLSPEC int Compute_J(slv_system_t sys);
ASC_DLLSPEC int NumberFreeVars(slv_system_t sys);
int NumberIncludedRels(slv_system_t sys);
int LUFactorJacobian(slv_system_t sys);
ASC_DLLSPEC int Compute_dy_dx_smart(slv_system_t sys, real64 *rhs, DenseMatrix dy_dx,
		int *inputs, int ninputs, int *outputs, int noutputs
);

/*--------------------------------------------------
	The following are provided to the external module 'sensitivity'. We want
	to move all of these out into the external shared object, but can't until
	the LSODE dependency goes away.
*/

ASC_DLLSPEC int finite_difference(struct gl_list_t *arglist);
ASC_DLLSPEC struct Instance *FetchElement(struct gl_list_t *arglist,
				     unsigned long whichbranch,
				     unsigned long whichelement);
ASC_DLLSPEC int NumberRels(slv_system_t sys);
ASC_DLLSPEC int LUFactorJacobian1(slv_system_t sys,int rank);
ASC_DLLSPEC struct Instance *FetchElement(struct gl_list_t *arglist,
				     unsigned long whichbranch,
				     unsigned long whichelement);

/* I *think* that we can probably replace these with 'densemtx.h' functions. */

ASC_DLLSPEC real64 **make_matrix(int nrows, int ncols);
ASC_DLLSPEC void free_matrix(real64 **matrix, int nrows);

#endif  /* ASC_SENSITIVITY_H */

