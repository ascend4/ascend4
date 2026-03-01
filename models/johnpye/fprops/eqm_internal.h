#ifndef FPROPS_EQM_INTERNAL_H
#define FPROPS_EQM_INTERNAL_H

#include "eqm.h"
#include "solution_data.h"
#include "spinel_data.h"

typedef enum {
	EQM_PHASE_PURE = 0,
	EQM_PHASE_BINARY_SOLUTION,
	EQM_PHASE_FE_SPINEL
} EqmPhaseKind;

typedef enum {
	EQM_SPECIES_ROLE_STANDALONE = 0,
	EQM_SPECIES_ROLE_SOLUTION_MEMBER
} EqmSpeciesRole;

typedef struct {
	int species_index;
	int phase_index;
	unsigned member_index;
} EqmSolutionMember;

typedef struct {
	EqmPhaseKind kind;
	int ia;
	int ib;
	const BinarySolutionPhaseDef *phase;
	const FeSpinelPhaseDef *spinel;
	int members[5];
} EqmBinaryPhaseMeta;

typedef struct {
	int ns;
	double *c;
	double rhs;
} EqmLinearIneqRow;

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
	int *is_condensed; /* [ns], 1 for condensed (a~1), 0 for gas species */
	int *solution_phase_id; /* [ns], -1 for standalone species */
	int *solution_member_index; /* [ns], -1 for standalone species */
	int nbinary_phases;
	EqmBinaryPhaseMeta *binary_phases; /* [nbinary_phases] */
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
	int *is_condensed; /* [ns], 1 for condensed (a~1), 0 for gas species */
	int *solution_phase_id; /* [ns], -1 for standalone species */
	int *solution_member_index; /* [ns], -1 for standalone species */
	int nbinary_phases;
	EqmBinaryPhaseMeta *binary_phases; /* [nbinary_phases] */
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
	int *is_condensed; /* [ns], 1 for condensed (a~1), 0 for gas species */
	int *solution_phase_id; /* [ns], -1 for standalone species */
	int *solution_member_index; /* [ns], -1 for standalone species */
	int nbinary_phases;
	EqmBinaryPhaseMeta *binary_phases; /* [nbinary_phases] */
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
	int *is_condensed; /* [ns], 1 for condensed (a~1), 0 for gas species */
	int *solution_phase_id; /* [ns], -1 for standalone species */
	int *solution_member_index; /* [ns], -1 for standalone species */
	int nbinary_phases;
	EqmBinaryPhaseMeta *binary_phases; /* [nbinary_phases] */
} EqmN;

double gas_R(void);
int eqm_compute_mu0(const char **names, int ns, const char *source, double T, double P0, double *mu0);
int eqm_compute_is_condensed(const char **names, int ns, const char *source, int *is_condensed);
int eqm_compute_solution_phases(const char **names, int ns, const char *source,
		int **solution_phase_id_out, int **solution_member_index_out,
		EqmBinaryPhaseMeta **binary_phases_out, int *nbinary_phases_out);
void eqm_free_solution_phases(int **solution_phase_id, int **solution_member_index,
		EqmBinaryPhaseMeta **binary_phases);
int eqm_has_solution_phases(const char **names, int ns, const char *source);
int eqm_eval_obj_mu(const double *n, const double *mu0, const int *is_condensed,
		const int *solution_phase_id, const EqmBinaryPhaseMeta *binary_phases, int nbinary_phases,
		int ns, double T, double P, double P0, double *obj, double *mu, double *n_gas_out);
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
