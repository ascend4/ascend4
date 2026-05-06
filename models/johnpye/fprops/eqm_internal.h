#ifndef FPROPS_EQM_INTERNAL_H
#define FPROPS_EQM_INTERNAL_H

#include "eqm.h"
#include "solution_data.h"
#include "spinel_data.h"

typedef enum EqmPhaseMetaKind{
	EQM_PHASE_BINARY_SOLUTION = 1,
	EQM_PHASE_FE_SPINEL = 2
} EqmPhaseMetaKind;

typedef struct EqmBinaryPhaseMeta{
	EqmPhaseMetaKind kind;
	const BinarySolutionPhaseDef *phase;
	const FeSpinelPhaseDef *spinel;
	int ia;
	int ib;
	int members[5];
} EqmBinaryPhaseMeta;

typedef struct EqmData{
	int ns;
	int ne;
	double T;
	double P;
	double P0;
	const double *A;
	const double *b;
	double *mu0;
	int *is_condensed;
	int *solution_phase_id;
	int *solution_member_index;
	EqmBinaryPhaseMeta *binary_phases;
	int nbinary_phases;
	double *n_scale;
	double *b_scale;
	double *n_est;
} EqmData;

typedef struct EqmN{
	int ns;
	int ne;
	double T;
	double P;
	double P0;
	double obj_scale;
	double n_min;
	const double *A;
	const double *b;
	double *mu0;
	int *is_condensed;
	int *solution_phase_id;
	int *solution_member_index;
	EqmBinaryPhaseMeta *binary_phases;
	int nbinary_phases;
	double *b_scale;
	double *n_est;
} EqmN;

typedef struct EqmLogN{
	int ns;
	int ne;
	double T;
	double P;
	double P0;
	double obj_scale;
	const double *A;
	const double *b;
	double *mu0;
	int *is_condensed;
	int *solution_phase_id;
	int *solution_member_index;
	EqmBinaryPhaseMeta *binary_phases;
	int nbinary_phases;
	double *b_scale;
	double *n_est;
} EqmLogN;

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
	double *A;
	double *b;
	double *mu0;
	int *is_condensed;
	int *solution_phase_id;
	int *solution_member_index;
	EqmBinaryPhaseMeta *binary_phases;
	int nbinary_phases;
	double *n0;
	double *N;
} EqmNullspace;

int eqm_solve(const char **names, int ns, int ne, const double *A, const double *b,
		const char *source, double T, double P, const char *algorithm, const double *n_init,
		double *n_out);

int eqm_solve_elements(const char **names, int ns, const char **elements, int ne,
		const double *b, const char *source, double T, double P, const char *algorithm,
		const double *n_init, double *n_out);

int eqm_mu0_ideal_source(const char *name, const char *source, double T, double P0,
		double *mu0);

double gas_R(void);

void eqm_apply_bscale(EqmData *D);
void eqm_apply_bscale_n(EqmN *D);
void eqm_apply_bscale_logn(EqmLogN *D);
void eqm_apply_nscale(EqmData *D, const double *n_init);

int eqm_compute_mu0(const char **names, int ns, const char *source, double T, double P0,
		double *mu0);
int eqm_compute_is_condensed(const char **names, int ns, const char *source,
		int *is_condensed);
int eqm_compute_solution_phases(const char **names, int ns, const char *source,
		int **solution_phase_id_out, int **solution_member_index_out,
		EqmBinaryPhaseMeta **binary_phases_out, int *nbinary_phases_out);
void eqm_free_solution_phases(int **solution_phase_id, int **solution_member_index,
		EqmBinaryPhaseMeta **binary_phases);
int eqm_has_solution_phases(const char **names, int ns, const char *source);

int eqm_eval_obj_mu(const double *n, const double *mu0, const int *is_condensed,
		const int *solution_phase_id, const EqmBinaryPhaseMeta *binary_phases,
		int nbinary_phases, int ns, double T, double P, double P0,
		double *G_out, double *mu_out, double *H_out);

int eqm_rref(double *A, int m, int n, int *pivots, int *rank);
void eqm_fill_nullspace(const double *A_rref, int m, int n,
		const int *pivots, int rank, double *N, int r);
int eqm_solve_particular(const double *A_in, const double *b_in, int m, int n,
		double *n0_out);
void eqm_fill_n_est(const double *A, const double *b, int ne, int ns,
		const double *n_init, double *n_est);
double eqm_logsumexp(const double *logv, int n);
int eqm_seed_from_nullspace_r1(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, double *n_out);

#endif /* FPROPS_EQM_INTERNAL_H */
