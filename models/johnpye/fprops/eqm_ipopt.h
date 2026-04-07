#ifndef FPROPS_EQM_IPOPT_H
#define FPROPS_EQM_IPOPT_H

#include "eqm.h"
#include "eqm_internal.h"

#ifdef HAVE_IPOPT

int eqm_ipopt_solve(const char **names, int ns, int ne, const double *A, const double *b,
		double T, double P, double *n_out);
int eqm_ipopt_solve_source(const char **names, int ns, int ne, const double *A, const double *b,
		const char *source, double T, double P, double *n_out);
int eqm_ipopt_solve_source_init(const char **names, int ns, int ne, const double *A, const double *b,
		const char *source, double T, double P, const double *n_init, double *n_out);
int eqm_ipopt_solve_elements(const char **names, int ns, const char **elements, int ne,
		const double *b, double T, double P, double *n_out);
int eqm_ipopt_solve_elements_source(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, double *n_out);
int eqm_ipopt_solve_elements_source_init(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, const double *n_init, double *n_out);

int eqm_ipopt_solve_logn_source_init(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, const double *n_init,
		double *n_out);
int eqm_ipopt_solve_logn_source(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, double *n_out);
int eqm_ipopt_solve_logn(const char **names, int ns, int ne, const double *A, const double *b,
		double T, double P, double *n_out);
int eqm_ipopt_solve_elements_logn_source_init(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, const double *n_init,
		double *n_out);
int eqm_ipopt_solve_elements_logn_source(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, double *n_out);
int eqm_ipopt_solve_elements_logn(const char **names, int ns, const char **elements, int ne,
		const double *b, double T, double P, double *n_out);

int eqm_ipopt_solve_n_source_init(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, const double *n_init,
		double *n_out);
int eqm_ipopt_solve_n_source(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, double *n_out);
int eqm_ipopt_solve_n(const char **names, int ns, int ne, const double *A, const double *b,
		double T, double P, double *n_out);
int eqm_ipopt_solve_elements_n_source_init(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, const double *n_init,
		double *n_out);
int eqm_ipopt_solve_elements_n_source(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, double *n_out);
int eqm_ipopt_solve_elements_n(const char **names, int ns, const char **elements, int ne,
		const double *b, double T, double P, double *n_out);

int eqm_ipopt_nullspace_1d(const char **names, const char **elements, const char *source,
		const double *b, double T, double P, double *n_out);

int eqm_ipopt_nullspace_create(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, EqmNullspace **out);
int eqm_ipopt_nullspace_solve(EqmNullspace *M, const double *z_init, double *n_out);
void eqm_ipopt_nullspace_destroy(EqmNullspace *M);
int eqm_ipopt_nullspace_solve_source(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, double *n_out);

int eqm_ipopt_nullspace_seed_r1(const EqmNullspace *M, double *n_seed);
int eqm_ipopt_nullspace_logK_solution_r1(const EqmNullspace *M, double *n_out);

#endif

#endif
