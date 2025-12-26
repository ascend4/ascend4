#ifndef FPROPS_EQM_SLSQP_H
#define FPROPS_EQM_SLSQP_H

#include "eqm.h"

#ifdef HAVE_NLOPT

int eqm_slsqp_solve_source_init(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, const double *n_init,
		double *n_out);
int eqm_slsqp_solve_source(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, double *n_out);
int eqm_slsqp_solve(const char **names, int ns, int ne, const double *A, const double *b,
		double T, double P, double *n_out);
int eqm_slsqp_solve_elements_source_init(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, const double *n_init,
		double *n_out);
int eqm_slsqp_solve_elements_source(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, double *n_out);
int eqm_slsqp_solve_elements(const char **names, int ns, const char **elements, int ne,
		const double *b, double T, double P, double *n_out);

#endif

#endif
