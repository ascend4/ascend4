#ifndef FPROPS_EQM_H
#define FPROPS_EQM_H

typedef enum EqmAlgorithm{
	EQM_ALG_AUTO = 0,
	EQM_ALG_IPOPT,
	EQM_ALG_IPOPT_N,
	EQM_ALG_IPOPT_LOGN,
	EQM_ALG_SLSQP
} EqmAlgorithm;

int eqm_solve(const char **names, int ns, int ne, const double *A, const double *b,
		const char *source, double T, double P, const char *algorithm, const double *n_init,
		double *n_out);

/* `algorithm` keywords for eqm_solve_elements:
   auto (default), ipopt*, slsqp, reduced, auto_reduced,
   auto_nullspace, auto_no_nullspace, nullspace
   Note: nullspace prepass is used for auto and ipopt* unless disabled. */
int eqm_solve_elements(const char **names, int ns, const char **elements, int ne,
		const double *b, const char *source, double T, double P, const char *algorithm,
		const double *n_init, double *n_out);

int eqm_mu0_ideal_source(const char *name, const char *source, double T, double P0,
		double *mu0);

#endif
