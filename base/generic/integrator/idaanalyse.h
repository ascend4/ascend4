#ifndef ASC_IDAANALYSE_H
#define ASC_IDAANALYSE_H

#include "integrator.h"

IntegratorAnalyseFn integrator_ida_analyse; /* for new approach -- JP Jan 2007 */

/**
	Given a derivative variable, return the index of its corresponding differential
	variable in the y vector (and equivalently the var_sindex of the diff var)
*/
int integrator_ida_diffindex(const IntegratorSystem *sys, const struct var_variable *deriv);

int integrator_ida_analyse_debug(const IntegratorSystem *sys,FILE *fp);

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
