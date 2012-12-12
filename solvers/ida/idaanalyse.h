/*	ASCEND modelling environment
	Copyright (C) 2006-2011 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @file
	Analysis routines for the ASCEND wrapper of the IDA integrator.
	These functions perform sorting of variables and relations and create
	additional lists of variables as required for use by our ida.c code.
*/

#ifndef ASC_IDAANALYSE_H
#define ASC_IDAANALYSE_H

#include <ascend/integrator/integrator.h>
#include <ascend/system/var.h>
#include <ascend/system/rel.h>

/**
	The top-level analysis function, called by the Integrator API.
*/
IntegratorAnalyseFn integrator_ida_analyse;

/**
	Given a derivative variable, return the index of its corresponding differential
	variable in the y vector (and equivalently the var_sindex of the diff var)
*/
int integrator_ida_diffindex(const IntegratorSystem *sys, const struct var_variable *deriv);

/**
	Same as integrator_ida_diffindex but returns -1 instead of aborting
*/
int integrator_ida_diffindex1(const IntegratorSystem *sys, const struct var_variable *deriv);

/**
	Filter that will match all our 'y' variables (and only those)
*/
const var_filter_t integrator_ida_filter_nonderiv;

/*
	Filter that will match all our 'ydot' variables (and only those)
*/
const var_filter_t integrator_ida_filter_deriv;

/**
	Some filters that will generally be useful for IDA systems.

	A var can be non-incident. If it *is* non incident and we're going to
	keep it, it will have to have derivative that *is* incident, and that
	meets the following filter.

	If it doesn't have a valid derivative (eg the derivative is fixed, or
	the variable doesn't HAVE a derivative), we will mark the non-deriv
	var non-ACTIVE, so anyway it will end up meeting this filter after we've
	run	integrator_ida_check_vars.
*/
extern const var_filter_t integrator_ida_nonderiv;
extern const var_filter_t integrator_ida_deriv;
extern const rel_filter_t integrator_ida_rel;

#endif
