/*	ASCEND modelling environment
	Copyright 1997, Carnegie Mellon University
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
	Lsode Integrator.

	@NOTE
	The macro DOTIME is defined inside Lsode.c. If its value is
	TRUE, we spew all sorts of time junk. If FALSE we are quiet.
	Default for this macro is FALSE.

	@NOTE the analytic jacobian routines (state matrix) depend on the
	assumption that struct var_variable *<--> struct Instance *.
	Requires:
	#include <utilities/ascConfig.h>
	#include <solver/slv_client.h>
	#include <solver/integrator.h>
*//*
	by Kirk Abbott and Ben Allan
	Created: 1/94
	Version: $Revision: 1.6 $
	Version control file: $RCSfile: Lsode.h,v $
	Date last modified: $Date: 1997/07/18 12:23:18 $
	Last modified by: $Author: mthomas $
*/

#ifndef ASC_LSODE_H
#define ASC_LSODE_H

#include "linsol.h"
#include "linsolqr.h"
#include "slv_client.h"

/// Perform LSODE integration of a specified 'Integ_system'.
/**
	Takes the start and finish index as defined by the user and carries
	out the integration using repeated calls to the function lsode.
	Assumes sys corresponds to g_solvinst_cur.
	works off instances of type blsode taken from blsys.
 */
extern int integrator_lsode_solve(IntegratorSystem *blsys
		,unsigned long i1, unsigned long i2
);

void integrator_lsode_create(IntegratorSystem *blsys);
void integrator_lsode_init(IntegratorSystem *blsys);
void integrator_lsode_free(void *enginedata);

#endif  /* ASC_LSODE_H */

