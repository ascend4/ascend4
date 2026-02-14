#ifndef ASC_LP_UTILS_H
#define ASC_LP_UTILS_H

#include <stdio.h>

#include "lp_data.h"
#include "slv_common.h"
#include "slv_client.h"
#include "var.h"
#include "rel.h"

ASC_DLLSPEC boolean lp_free_inc_var_filter(struct var_variable *var);
ASC_DLLSPEC boolean lp_inc_rel_filter(struct rel_relation *rel);

ASC_DLLSPEC void lp_nuke_pointers(mps_data_t *mps);

ASC_DLLSPEC boolean lp_calc_c(
	mtx_matrix_t mtx,
	int32 org_row,
	struct rel_relation *obj
);

ASC_DLLSPEC real64 *lp_calc_bounds(
	struct var_variable **vlist,
	int32 vused,
	boolean upper
);

ASC_DLLSPEC char *lp_calc_reloplist(
	struct rel_relation **rlist,
	int32 rused
);

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

ASC_DLLSPEC void lp_real_rhs(
	mtx_matrix_t Ac_mtx,
	char relopcol[],
	struct var_variable **vlist,
	int32 rused,
	int32 vused,
	real64 rhs[]
);

ASC_DLLSPEC void lp_ensure_bounds(
	FILE *mif,
	slv_system_t slv,
	struct var_variable *var
);

#endif
