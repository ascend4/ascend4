/*
 * A4SQP solver registration and minimal lifecycle.
 */

#define ASC_BUILDING_INTERFACE

#include "a4sqp.h"

#include "a4sqp_ascend.h"
#include "a4sqp_diag.h"
#include "a4sqp_internal.h"
#include "a4sqp_qp_highs.h"
#include "a4sqp_scale.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <ascend/general/ascMalloc.h>
#include <ascend/general/mem.h>
#include <ascend/system/slv_common.h>
#include <ascend/system/slv_param.h>
#include <ascend/system/var.h>
#include <ascend/utilities/error.h>

static void a4sqp_init_status(struct A4SqpSystem *sys){
	if(sys == NULL){
		return;
	}
	memset(&sys->status,0,sizeof(sys->status));
	sys->status.kind = SLV_STATUS_NLP;
	sys->status.ok = TRUE;
	sys->status.calc_ok = TRUE;
	sys->status.ready_to_solve = FALSE;
	sys->status.converged = FALSE;
	sys->status.iteration = 0;
	sys->status.cpu_elapsed = 0.0;
	sys->last_merit_before = 0.0;
	sys->last_merit_after = 0.0;
	sys->last_violation_sum = 0.0;
	sys->last_violation_max = 0.0;
	sys->last_alpha = 0.0;
	sys->last_step_norm = 0.0;
	sys->worst_violation_rel = -1;
	sys->line_search_failed = 0;
}

static SlvClientToken a4sqp_create(slv_system_t server, int *statusindex){
	struct A4SqpSystem *sys;

	sys = ASC_NEW_CLEAR(struct A4SqpSystem);
	if(sys == NULL){
		if(statusindex != NULL){
			*statusindex = 1;
		}
		return NULL;
	}

	sys->server = server;
	sys->params.parms = sys->param_data;
	sys->params.dynamic_parms = 0;
	a4sqp_get_default_parameters(server,(SlvClientToken)sys,&sys->params);
	if(statusindex != NULL){
		sys->params.whose = *statusindex;
	}

	a4sqp_view_init(&sys->view);
	a4sqp_qp_init(&sys->qp);
	a4sqp_init_status(sys);

	if(statusindex != NULL){
		*statusindex = 0;
	}
	return (SlvClientToken)sys;
}

static int a4sqp_destroy(slv_system_t server, SlvClientToken asys){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;
	(void)server;

	if(sys == NULL){
		return 0;
	}

	a4sqp_view_destroy(&sys->view);
	a4sqp_qp_destroy(&sys->qp);
	slv_destroy_parms(&sys->params);
	ascfree(sys);
	return 0;
}

static int a4sqp_eligible(slv_system_t server){
	(void)server;
	return 1;
}

static void a4sqp_get_parameters(slv_system_t server, SlvClientToken asys, slv_parameters_t *parameters){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;
	(void)server;
	if(sys == NULL || parameters == NULL){
		return;
	}
	mem_copy_cast(&sys->params,parameters,sizeof(slv_parameters_t));
}

static void a4sqp_set_parameters(slv_system_t server, SlvClientToken asys, slv_parameters_t *parameters){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;
	(void)server;
	if(sys == NULL || parameters == NULL){
		return;
	}
	if(parameters->whose == A4SQP_SOLVER_NUMBER){
		mem_copy_cast(parameters,&sys->params,sizeof(slv_parameters_t));
	}
}

static int a4sqp_get_status(slv_system_t server, SlvClientToken asys, slv_status_t *status){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;
	(void)server;
	if(sys == NULL || status == NULL){
		return 1;
	}
	mem_copy_cast(&sys->status,status,sizeof(slv_status_t));
	return 0;
}

static void a4sqp_report_view(struct A4SqpSystem *sys){
	char message[256];
	const char *scaleopt;

	if(sys == NULL){
		return;
	}

	scaleopt = a4sqp_scale_mode_name(SLV_PARAM_CHAR(&sys->params,A4SQP_PARAM_SCALEOPT));
	snprintf(message,sizeof(message),
		"view: vars=%ld, rels=%ld, jac_nnz=%ld, objective=%s, scaleopt=%s, calc_errors=%ld, derivative_errors=%ld, unsupported_rels=%ld",
		(long)sys->view.n_var,
		(long)sys->view.n_rel,
		(long)sys->view.jac_nnz,
		sys->view.obj != NULL ? "yes" : "no",
		scaleopt,
		(long)(sys->view.calc_errors + sys->view.obj_calc_errors),
		(long)(sys->view.derivative_errors + sys->view.obj_derivative_errors),
		(long)sys->view.unsupported_rels
	);
	a4sqp_report_progress(&sys->params,message);

	if(SLV_PARAM_BOOL(&sys->params,A4SQP_PARAM_DUMP_VIEW)){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"%s",message);
	}
}

static void a4sqp_report_qp(struct A4SqpSystem *sys){
	char message[256];

	if(sys == NULL){
		return;
	}
	snprintf(
		message,
		sizeof(message),
		"qp: cols=%ld rows=%ld nnz=%ld q_nnz=%ld status=%d model_status=%d objective=%g",
		(long)sys->qp.num_col,
		(long)sys->qp.num_row,
		(long)sys->qp.num_nz,
		(long)sys->qp.q_num_nz,
		sys->qp.highs_status,
		sys->qp.highs_model_status,
		sys->qp.objective_value
	);
	a4sqp_report_progress(&sys->params,message);
}

static void a4sqp_report_iteration(struct A4SqpSystem *sys){
	char message[256];

	if(sys == NULL){
		return;
	}
	snprintf(
		message,
		sizeof(message),
		"iter=%ld obj=%g merit=%g viol_sum=%g viol_max=%g alpha=%g step=%g worst_rel=%ld",
		(long)sys->status.iteration,
		sys->view.obj != NULL ? sys->view.obj_value : 0.0,
		sys->last_merit_after,
		sys->last_violation_sum,
		sys->last_violation_max,
		sys->last_alpha,
		sys->last_step_norm,
		(long)sys->worst_violation_rel
	);
	a4sqp_report_progress(&sys->params,message);
}

static int a4sqp_is_lower_inf(real64 value){
	return value <= var_NO_LOWER_BOUND / 10.0;
}

static int a4sqp_is_upper_inf(real64 value){
	return value >= var_NO_UPPER_BOUND / 10.0;
}

static real64 a4sqp_view_violation(
	const struct A4SqpView *view,
	real64 *max_violation,
	int32 *worst_rel
){
	int32 i;
	real64 violation = 0.0;
	real64 vmax = 0.0;
	int32 worst = -1;
	if(view == NULL){
		if(max_violation != NULL){
			*max_violation = 0.0;
		}
		if(worst_rel != NULL){
			*worst_rel = -1;
		}
		return 0.0;
	}
	for(i = 0; i < view->n_rel; ++i){
		real64 row_violation = 0.0;
		if(!a4sqp_is_lower_inf(view->scaled_rel_lower[i])
			&& view->scaled_rel_residual[i] < view->scaled_rel_lower[i]
		){
			row_violation += view->scaled_rel_lower[i] - view->scaled_rel_residual[i];
		}
		if(!a4sqp_is_upper_inf(view->scaled_rel_upper[i])
			&& view->scaled_rel_residual[i] > view->scaled_rel_upper[i]
		){
			row_violation += view->scaled_rel_residual[i] - view->scaled_rel_upper[i];
		}
		violation += row_violation;
		if(row_violation > vmax){
			vmax = row_violation;
			worst = i;
		}
	}
	if(max_violation != NULL){
		*max_violation = vmax;
	}
	if(worst_rel != NULL){
		*worst_rel = worst;
	}
	return violation;
}

static real64 a4sqp_view_merit(const struct A4SqpView *view, real64 penalty){
	real64 violation = a4sqp_view_violation(view,NULL,NULL);
	if(view == NULL || view->obj == NULL){
		return violation;
	}
	return view->obj_value + penalty * violation;
}

static void a4sqp_update_metrics(struct A4SqpSystem *sys){
	if(sys == NULL){
		return;
	}
	sys->last_violation_sum = a4sqp_view_violation(
		&sys->view,
		&sys->last_violation_max,
		&sys->worst_violation_rel
	);
	sys->status.block.residual = sys->last_violation_max;
}

static int a4sqp_has_converged(struct A4SqpSystem *sys){
	real64 feas_tol;
	real64 step_tol;
	if(sys == NULL){
		return 0;
	}
	feas_tol = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_FEAS_TOL);
	step_tol = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_STEP_TOL);
	a4sqp_update_metrics(sys);
	return sys->last_violation_max <= feas_tol && sys->last_step_norm <= step_tol;
}

static real64 a4sqp_apply_step(
	struct var_variable **vars,
	const real64 *old_values,
	const real64 *physical_step,
	int32 n_var,
	real64 alpha
){
	int32 i;
	real64 norm2 = 0.0;
	for(i = 0; i < n_var; ++i){
		real64 step = alpha * physical_step[i];
		var_set_value(vars[i],old_values[i] + step);
		norm2 += step * step;
	}
	return sqrt(norm2);
}

static int a4sqp_line_search(slv_system_t server, struct A4SqpSystem *sys){
	int32 i;
	int accepted = 0;
	int max_backtrack;
	real64 alpha = 1.0;
	real64 penalty;
	real64 merit_tol;
	real64 *old_values;
	real64 *physical_step;
	struct var_variable **vars;

	if(sys == NULL || sys->view.n_var <= 0){
		return 1;
	}

	old_values = ASC_NEW_ARRAY_OR_NULL(real64,sys->view.n_var);
	physical_step = ASC_NEW_ARRAY_OR_NULL(real64,sys->view.n_var);
	if(old_values == NULL || physical_step == NULL){
		ASC_FREE(old_values);
		ASC_FREE(physical_step);
		return 1;
	}

	vars = sys->view.vars;
	max_backtrack = SLV_PARAM_INT(&sys->params,A4SQP_PARAM_MAX_BACKTRACK);
	penalty = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_ELASTIC_PENALTY);
	merit_tol = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_MERIT_TOL);
	sys->last_merit_before = a4sqp_view_merit(&sys->view,penalty);
	sys->last_merit_after = sys->last_merit_before;
	sys->last_alpha = 0.0;
	sys->last_step_norm = 0.0;
	sys->line_search_failed = 0;

	for(i = 0; i < sys->view.n_var; ++i){
		old_values[i] = sys->view.var_value[i];
		physical_step[i] = sys->view.var_scale[i] * sys->qp.col_value[i];
		sys->last_step_norm += physical_step[i] * physical_step[i];
	}
	sys->last_step_norm = sqrt(sys->last_step_norm);

	if(sys->last_step_norm <= SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_STEP_TOL)){
		a4sqp_update_metrics(sys);
		ASC_FREE(old_values);
		ASC_FREE(physical_step);
		return 0;
	}

	for(i = 0; i < max_backtrack; ++i){
		real64 step_norm;
		step_norm = a4sqp_apply_step(vars,old_values,physical_step,sys->view.n_var,alpha);
		if(a4sqp_ascend_build_view(sys,server)){
			alpha *= 0.5;
			continue;
		}
		sys->last_merit_after = a4sqp_view_merit(&sys->view,penalty);
		a4sqp_update_metrics(sys);
		if(sys->last_merit_after < sys->last_merit_before - merit_tol){
			sys->last_alpha = alpha;
			sys->last_step_norm = step_norm;
			accepted = 1;
			break;
		}
		alpha *= 0.5;
	}

	if(!accepted){
		a4sqp_apply_step(vars,old_values,physical_step,sys->view.n_var,0.0);
		(void)a4sqp_ascend_build_view(sys,server);
		a4sqp_update_metrics(sys);
		sys->line_search_failed = 1;
	}

	ASC_FREE(old_values);
	ASC_FREE(physical_step);
	return accepted ? 0 : 1;
}

static int a4sqp_presolve(slv_system_t server, SlvClientToken asys){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;

	if(sys == NULL){
		return 1;
	}

	sys->server = server;
	a4sqp_init_status(sys);

	if(a4sqp_ascend_build_view(sys,server)){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		sys->status.ready_to_solve = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"A4SQP failed to build the ASCEND problem view.");
		return 1;
	}

	if(sys->view.unsupported_rels > 0){
		sys->status.ok = FALSE;
		sys->status.ready_to_solve = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,
			"A4SQP Phase 1 does not support %ld relation(s) in the selected solver list.",
			(long)sys->view.unsupported_rels
		);
		a4sqp_report_view(sys);
		return 1;
	}

	if(sys->view.calc_errors > 0){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		sys->status.ready_to_solve = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,
			"A4SQP encountered residual evaluation errors in %ld relation(s).",
			(long)sys->view.calc_errors
		);
		a4sqp_report_view(sys);
		return 1;
	}

	if(sys->view.derivative_errors > 0){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		sys->status.ready_to_solve = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,
			"A4SQP encountered derivative evaluation errors in %ld relation(s).",
			(long)sys->view.derivative_errors
		);
		a4sqp_report_view(sys);
		return 1;
	}

	sys->status.ready_to_solve = TRUE;
	a4sqp_update_metrics(sys);
	a4sqp_report_view(sys);
	return 0;
}

static int a4sqp_iterate(slv_system_t server, SlvClientToken asys){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;

	if(sys == NULL){
		return 1;
	}
	if(!sys->status.ready_to_solve){
		if(a4sqp_presolve(server,asys)){
			return 1;
		}
	}

	if(a4sqp_qp_build_from_view(
		&sys->qp,
		&sys->view,
		SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_ELASTIC_PENALTY)
	)){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"A4SQP failed to assemble the HiGHS QP subproblem.");
		return 1;
	}
	if(a4sqp_qp_solve_highs(&sys->qp)){
		sys->status.converged = FALSE;
		sys->status.diverged = TRUE;
		a4sqp_report_qp(sys);
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"A4SQP HiGHS QP subproblem did not solve to optimality.");
		return 1;
	}

	if(a4sqp_line_search(server,sys)){
		sys->status.converged = FALSE;
		sys->status.diverged = TRUE;
		a4sqp_report_qp(sys);
		ERROR_REPORTER_HERE(ASC_PROG_ERR,
			"A4SQP line search failed to find a merit-improving step (merit_before=%g, merit_after=%g, step=%g, viol_max=%g).",
			sys->last_merit_before,
			sys->last_merit_after,
			sys->last_step_norm,
			sys->last_violation_max
		);
		return 1;
	}

	++sys->status.iteration;
	sys->status.block.iteration = sys->status.iteration;
	a4sqp_report_qp(sys);
	a4sqp_report_iteration(sys);
	a4sqp_report_view(sys);
	return 0;
}

static int a4sqp_solve(slv_system_t server, SlvClientToken asys){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;
	int max_iter;

	if(sys == NULL){
		return 1;
	}
	if(!sys->status.ready_to_solve){
		if(a4sqp_presolve(server,asys)){
			return 1;
		}
	}

	max_iter = SLV_PARAM_INT(&sys->params,A4SQP_PARAM_MAX_ITER);
	while(sys->status.iteration < max_iter){
		if(a4sqp_iterate(server,asys)){
			sys->status.ready_to_solve = FALSE;
			return 1;
		}
		if(a4sqp_has_converged(sys)){
			sys->status.converged = TRUE;
			sys->status.diverged = FALSE;
			sys->status.iteration_limit_exceeded = FALSE;
			sys->status.ready_to_solve = FALSE;
			return 0;
		}
	}

	sys->status.converged = FALSE;
	sys->status.diverged = FALSE;
	sys->status.iteration_limit_exceeded = TRUE;
	sys->status.ready_to_solve = FALSE;
	ERROR_REPORTER_HERE(ASC_PROG_WARNING,
		"A4SQP reached the maximum iteration count before satisfying convergence tests."
	);
	return 0;
}

static int a4sqp_resolve(slv_system_t server, SlvClientToken asys){
	return a4sqp_solve(server,asys);
}

static void a4sqp_dumpinternals(slv_system_t server, SlvClientToken asys, int level){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;
	(void)server;
	if(sys == NULL || level <= 0){
		return;
	}
	a4sqp_report_view(sys);
}

static const SlvFunctionsT a4sqp_internals = {
	A4SQP_SOLVER_NUMBER,
	A4SQP_SOLVER_NAME,
	a4sqp_create,
	a4sqp_destroy,
	a4sqp_eligible,
	a4sqp_get_default_parameters,
	a4sqp_get_parameters,
	a4sqp_set_parameters,
	a4sqp_get_status,
	a4sqp_solve,
	a4sqp_presolve,
	a4sqp_iterate,
	a4sqp_resolve,
	NULL,
	NULL,
	a4sqp_dumpinternals
};

ASC_EXPORT int a4sqp_register(void){
	return solver_register(&a4sqp_internals);
}
