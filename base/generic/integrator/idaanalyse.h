#ifndef ASC_IDAANALYSE_H
#define ASC_IDAANALYSE_H

#include "integrator.h"

IntegratorAnalyseFn integrator_ida_analyse; /* for new approach -- JP Jan 2007 */

int integrator_ida_diffindex(const IntegratorSystem *sys, const struct var_variable *deriv);

int integrator_ida_analyse_debug(const IntegratorSystem *sys,FILE *fp);

#endif
