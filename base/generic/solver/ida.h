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
	IDA integrator
*//*
	by John Pye, May 2006.
*/

#ifndef ASC_IDA_H
#define ASC_IDA_H

/**	@addtogroup integrator Integrator
	@{
*/

#include <utilities/config.h>
#include "integrator.h"

#ifdef ASC_WITH_IDA

IntegratorCreateFn integrator_ida_create;
IntegratorParamsDefaultFn integrator_ida_params_default;
IntegratorSolveFn integrator_ida_solve;
IntegratorFreeFn integrator_ida_free;
IntegratorAnalyseFn integrator_ida_analyse; /* for new approach -- JP Jan 2007 */

const IntegratorInternals integrator_ida_internals;

#endif

/* @} */

#endif  /* ASC_IDA_H */
