#ifndef FPROPS_EQM_INTERNAL_H
#define FPROPS_EQM_INTERNAL_H

#include "eqm.h"

typedef struct EqmData{
	int ns;
	int ne;
	double T;
	double P;
	double P0;
	const double *A; /* [ne * ns] row-major */
	const double *b; /* [ne] */
	double *b_scale; /* [ne], scaling for constraints */
	double *n_scale; /* [ns], scaling for variable gradients */
	double *mu0;     /* [ns], J/mol */
} EqmData;

typedef struct EqmNullspace{
	int ns;
	int ne;
	int r;
	double T;
	double P;
	double P0;
	double n_min;
	double obj_scale;
	double barrier_tau;
	double *A;  /* [ne * ns] */
	double *b;  /* [ne] */
	double *mu0; /* [ns] */
	double *n0; /* [ns] */
	double *N;  /* [ns * r] */
} EqmNullspace;

typedef struct EqmLogN{
	int ns;
	int ne;
	double T;
	double P;
	double P0;
	double obj_scale;
	const double *A; /* [ne * ns] row-major */
	const double *b; /* [ne] */
	double *b_scale; /* [ne], scaling for constraints */
	double *mu0;     /* [ns], J/mol */
	double *n_est;   /* [ns], scaling for log-mole variables */
} EqmLogN;

typedef struct EqmN{
	int ns;
	int ne;
	double T;
	double P;
	double P0;
	double obj_scale;
	double n_min;
	const double *A; /* [ne * ns] row-major */
	const double *b; /* [ne] */
	double *b_scale; /* [ne], scaling for constraints */
	double *mu0;     /* [ns], J/mol */
} EqmN;

double gas_R(void);
int eqm_compute_mu0(const char **names, int ns, const char *source, double T, double P0, double *mu0);
void eqm_apply_bscale(EqmData *D);
void eqm_apply_bscale_logn(EqmLogN *D);
void eqm_apply_bscale_n(EqmN *D);
void eqm_apply_nscale(EqmData *D, const double *n_init);
int eqm_rref(double *A, int m, int n, int *pivots, int *rank);
void eqm_fill_nullspace(const double *A_rref, int m, int n,
		const int *pivots, int rank, double *N_out, int r);
int eqm_solve_particular(const double *A_in, const double *b_in, int m, int n, double *n0_out);
void eqm_fill_n_est(const double *A, const double *b, int ne, int ns, const double *n_init, double *n_est);
double eqm_logsumexp(const double *logv, int n);
int eqm_seed_from_nullspace_r1(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, double *n_seed);

#endif
