#ifndef FPROPS_EQM_PHASE_H
#define FPROPS_EQM_PHASE_H

#define FPROPS_EQM_PHASE_MAX_MEMBERS 8
#define FPROPS_EQM_PHASE_MAX_ELEMS 8
#define FPROPS_EQM_PHASE_MAX_VARS 8

typedef enum FpropsEqmPhaseKind{
	FPROPS_EQM_PHASE_STOICHIOMETRIC = 0,
	FPROPS_EQM_PHASE_IDEAL_GAS,
	FPROPS_EQM_PHASE_BINARY_SOLUTION,
	FPROPS_EQM_PHASE_SITE_SOLUTION,
	FPROPS_EQM_PHASE_GENERIC
} FpropsEqmPhaseKind;

typedef struct FpropsEqmPhaseModel{
	FpropsEqmPhaseKind kind;
	const char *name;
	const char *source;
	const char *basis;
	int nmember;
	int nelem;
	int nvar;
	const char *members[FPROPS_EQM_PHASE_MAX_MEMBERS];
	const char *elements[FPROPS_EQM_PHASE_MAX_ELEMS];
	const char *var_names[FPROPS_EQM_PHASE_MAX_VARS];
	double lower[FPROPS_EQM_PHASE_MAX_VARS];
	double upper[FPROPS_EQM_PHASE_MAX_VARS];
	const void *data;
	char name_storage[128];
	char source_storage[256];
	char basis_storage[64];
	char member_storage[FPROPS_EQM_PHASE_MAX_MEMBERS][64];
	char element_storage[FPROPS_EQM_PHASE_MAX_ELEMS][16];
	char var_storage[FPROPS_EQM_PHASE_MAX_VARS][64];
} FpropsEqmPhaseModel;

const char *fprops_eqm_phase_kind_name(FpropsEqmPhaseKind kind);

int fprops_eqm_phase_resolve(const char *spec, const char *source,
		FpropsEqmPhaseModel *phase);

int fprops_eqm_phase_elements(const FpropsEqmPhaseModel *phase,
		const double *y, double *a_out);

int fprops_eqm_phase_gibbs(const FpropsEqmPhaseModel *phase,
		double T, double P, const double *y, double *g_out);

int fprops_eqm_phase_entry_residual(const FpropsEqmPhaseModel *phase,
		double T, double P, const double *lambda, double *phi_out, double *y_out);

int fprops_eqm_phase_solve_fixed_linear(const FpropsEqmPhaseModel *phases, int nphase,
		const char **elements, int ne, const double *b, double T, double P,
		double *phase_amounts_out, double *member_amounts_out);

int fprops_eqm_phase_solve_fixed_expanded(const FpropsEqmPhaseModel *phases, int nphase,
		const char **elements, int ne, const double *b, double T, double P,
		const char *algorithm, const double *member_init,
		double *phase_amounts_out, double *phase_y_out, double *member_amounts_out,
		int *nmember_out);

int fprops_eqm_phase_solve_auto(const FpropsEqmPhaseModel *phases, int nphase,
		const char **elements, int ne, const double *b, double T, double P,
		const char *algorithm, double *phase_amounts_out, double *phase_y_out,
		int *phase_active_out, double *member_amounts_out, int *nmember_out);

#endif /* FPROPS_EQM_PHASE_H */
