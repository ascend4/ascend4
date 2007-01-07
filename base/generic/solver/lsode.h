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

/**	@addtogroup integrator Integrator
	@{
*/

#include "integrator.h"

IntegratorCreateFn integrator_lsode_create;
IntegratorParamsDefaultFn integrator_lsode_params_default;
IntegratorSolveFn integrator_lsode_solve;
IntegratorFreeFn integrator_lsode_free;

const IntegratorInternals integrator_lsode_internals;

/* @} */

#endif  /* ASC_LSODE_H */
