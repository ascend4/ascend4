/*
 * Optional least-squares system view.
 */

#ifndef ASC_SYSTEM_LSQ_H
#define ASC_SYSTEM_LSQ_H

#include <ascend/general/platform.h>
#include <ascend/compiler/relation_util.h>
#include "slv_types.h"

struct rel_relation;
struct relation_term;

enum system_lsq_analyse_flags {
	SYSTEM_LSQ_ANALYSE_CLASSIFY = 0,
	SYSTEM_LSQ_ANALYSE_BUILD_VIEW = 1,
	SYSTEM_LSQ_ANALYSE_BUILD_PROJECTION = 2
};

struct system_lsq_residual {
	struct rel_relation *objective;
	CONST struct relation_term *residual_term;
	double weight;
};

struct system_lsq_view {
	int valid;
	struct rel_relation *objective;
	struct RelationLeastSquaresAnalysis analysis;
	unsigned long nresiduals;
	struct system_lsq_residual *residuals;
	unsigned long nprojected;
	int *projected_sindex;
};

ASC_DLLSPEC int system_analyse_lsq_objective(
	slv_system_t sys,
	unsigned flags,
	struct RelationLeastSquaresAnalysis *analysis
);
/**<
 *  Analyse the current system objective for a least-squares objective form.
 *
 *  If flags includes SYSTEM_LSQ_ANALYSE_BUILD_VIEW, a retained view is stored
 *  on sys and can be retrieved using system_get_lsq_view(). Without that flag,
 *  the function only classifies the objective and fills analysis when non-NULL.
 */

ASC_DLLSPEC const struct system_lsq_view *system_get_lsq_view(slv_system_t sys);
/**< Returns the retained least-squares view, or NULL if no valid view exists. */

ASC_DLLSPEC void system_clear_lsq_view(slv_system_t sys);
/**< Clears and frees any retained least-squares view stored on sys. */

ASC_DLLSPEC int system_lsq_eval_residual(
	slv_system_t sys,
	unsigned long index,
	double *residual
);
/**<
 *  Evaluate one raw least-squares residual by zero-based residual index.
 *
 *  The retained LS view must already exist. The returned value is r_i, not
 *  sqrt(weight_i)*r_i.
 */

ASC_DLLSPEC int system_lsq_eval_residuals(slv_system_t sys, double *residuals);
/**<
 *  Evaluate all raw least-squares residuals into residuals[0..nresiduals-1].
 */

ASC_DLLSPEC int system_lsq_eval_jacobian_row(
	slv_system_t sys,
	unsigned long index,
	int *columns,
	double *values,
	unsigned long capacity,
	unsigned long *nnz
);
/**<
 *  Evaluate one sparse residual-Jacobian row by zero-based residual index.
 *
 *  columns[k] is var_sindex(var), suitable for solver-column use. values[k]
 *  is d r_i / d x_columns[k]. If capacity is zero, only nnz is reported.
 */

#endif
