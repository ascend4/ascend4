#ifndef FPROPS_EQM_PHASE_INTERNAL_H
#define FPROPS_EQM_PHASE_INTERNAL_H

#include "eqm_phase.h"

int fprops_eqm_phase_resolve_package(const char **specs, const char **sources,
		int nphase, FpropsEqmPhaseModel *phases);

int fprops_eqm_phase_total_members(const FpropsEqmPhaseModel *phases, int nphase);

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

int fprops_eqm_phase_solve_active_set(const FpropsEqmPhaseModel *phases, int nphase,
		const char **elements, int ne, const double *b, double T, double P,
		const char *algorithm, const int *phase_active_init,
		double *phase_amounts_out, double *phase_y_out,
		int *phase_active_out, double *member_amounts_out, int *nmember_out);

int fprops_eqm_phase_solve_active_set_result(const FpropsEqmPhaseModel *phases, int nphase,
		const char **elements, int ne, const double *b, double T, double P,
		const char *algorithm, const int *phase_active_init, FpropsEqmPhaseResult *result);

int fprops_eqm_phase_reconstruct_lambda(const FpropsEqmPhaseModel *phases, int nphase,
		const char **elements, int ne, double T, double P,
		const double *phase_amounts, const double *phase_y, const int *phase_active,
		double *lambda_out, double *stationarity_rms_out);

int fprops_eqm_phase_validate_entry_residuals(const FpropsEqmPhaseModel *phases, int nphase,
		const char **elements, int ne, double T, double P,
		const double *phase_amounts, const double *phase_y, const int *phase_active,
		double *lambda_out, double *entry_residuals_out);

#endif /* FPROPS_EQM_PHASE_INTERNAL_H */
