#ifndef ASC_LP_UTILS_H
#define ASC_LP_UTILS_H

/**
 * @file
 * Shared LP/MIP utility helpers used by optional solvers that build
 * matrix-based problem data from ASCEND relations/variables (currently
 * MakeMPS and HiGHS).
 *
 * The routines in this header operate on solver lists and on the shared
 * `mps_data_t` structure in `lp_data.h`, so both solvers can reuse the same
 * incidence filtering, matrix assembly, and bounds/type extraction logic.
 */

#include <stdio.h>

#include "lp_data.h"
#include "slv_common.h"
#include "slv_client.h"
#include "var.h"
#include "rel.h"

/** True for non-fixed active variables in solver lists. */
ASC_DLLSPEC boolean lp_free_inc_var_filter(struct var_variable *var);
/** True for active included relations in solver lists. */
ASC_DLLSPEC boolean lp_inc_rel_filter(struct rel_relation *rel);

/** Release and NULL all dynamic storage in an `mps_data_t` container. */
ASC_DLLSPEC void lp_nuke_pointers(mps_data_t *mps);

/** Fill objective-gradient coefficients into the objective row of `mtx`. */
ASC_DLLSPEC boolean lp_calc_c(
	mtx_matrix_t mtx,
	int32 org_row,
	struct rel_relation *obj
);

/** Extract lower/upper bounds in original-column order. */
ASC_DLLSPEC real64 *lp_calc_bounds(
	struct var_variable **vlist,
	int32 vused,
	boolean upper
);

/** Convert relation operators to token array used by export/solver backends. */
ASC_DLLSPEC char *lp_calc_reloplist(
	struct rel_relation **rlist,
	int32 rused
);

/** Classify each variable into shared MPS/LP type codes and count categories. */
ASC_DLLSPEC char *lp_calc_svtlist(
	struct var_variable **vlist,
	int32 vused,
	int *solver_var_used,
	int *solver_relaxed_used,
	int *solver_int_used,
	int *solver_binary_used,
	int *solver_semi_used,
	int *solver_other_used,
	int *solver_fixed
);

/** Build Jacobian-style matrix plus rhs/objective arrays for LP-style solvers. */
ASC_DLLSPEC mtx_matrix_t lp_calc_matrix(
	int32 cap,
	int32 rused,
	int32 vused,
	struct rel_relation **rlist,
	struct rel_relation *obj,
	int32 crow,
	slv_status_t *s,
	int32 *rank,
	real64 **rhs_orig
);

/** Recompute RHS values at current variable values for non-equality rows. */
ASC_DLLSPEC void lp_real_rhs(
	mtx_matrix_t Ac_mtx,
	char relopcol[],
	struct var_variable **vlist,
	int32 rused,
	int32 vused,
	real64 rhs[]
);

/** Normalize/repair variable bounds and initial value consistency. */
ASC_DLLSPEC void lp_ensure_bounds(
	FILE *mif,
	slv_system_t slv,
	struct var_variable *var
);

/**
 * Apply nominal-based scaling to LP/MIP data:
 * - variable scaling by var_nominal (continuous vars only)
 * - relation scaling by relman_scale for incident constraints
 *
 * The transformed problem uses scaled variables x_s such that
 * x = col_scale * x_s.
 */
ASC_DLLSPEC boolean lp_apply_nominal_scaling(
	mtx_matrix_t Ac_mtx,
	real64 lbrow[],
	real64 ubrow[],
	real64 bcol[],
	char typerow[],
	char relopcol[],
	int32 cap,
	int32 rused,
	int32 vused,
	int32 crow,
	struct var_variable **vlist,
	struct rel_relation **rlist,
	struct rel_relation *obj,
	boolean varnom_scale,
	boolean relnom_scale,
	real64 **col_scale_out,
	real64 **row_scale_out
);

#endif
