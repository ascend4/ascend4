/*
 * ASCEND-facing A4SQP reporting helpers.
 */

#define ASC_BUILDING_INTERFACE

#include "asc_a4sqp_report.h"

#include "asc_a4sqp_diag.h"
#include "asc_a4sqp_internal.h"
#include "asc_a4sqp_params.h"
#include "a4sqp_scale.h"

#include <stdio.h>

#include <ascend/general/ascMalloc.h>
#include <ascend/system/rel.h>
#include <ascend/system/slv_param.h>
#include <ascend/system/var.h>
#include <ascend/utilities/error.h>

static const char *asc_a4sqp_phase_name(enum A4SqpCorePhase phase){
	switch(phase){
	case A4SQP_CORE_PHASE_RESTORATION:
		return "restoration";
	case A4SQP_CORE_PHASE_RESTORATION_EXIT:
		return "restoration_exit";
	case A4SQP_CORE_PHASE_REGULAR:
	default:
		return "regular";
	}
}

static void asc_a4sqp_report_view_dump(struct A4SqpSystem *sys){
	int32 i;
	int32 k;

	if(sys == NULL){
		return;
	}
	ERROR_REPORTER_HERE(ASC_PROG_NOTE,
		"A4SQP_VIEW_DUMP: vars=%ld rels=%ld jac_nnz=%ld obj=%g",
		(long)sys->view.n_var,
		(long)sys->view.n_rel,
		(long)sys->view.jac_nnz,
		sys->view.obj != NULL ? sys->view.obj_value : 0.0
	);
	for(i = 0; i < sys->view.n_var; ++i){
		char *name = NULL;
		if(sys->server != NULL && sys->view.vars != NULL && sys->view.vars[i] != NULL){
			name = var_make_name(sys->server,(struct var_variable *)sys->view.vars[i]);
		}
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,
			"A4SQP_VIEW_VAR: col=%ld sindex=%ld mindex=%ld name=%s x=%.17g lb=%.17g ub=%.17g grad=%.17g scale=%.17g",
			(long)i,
			sys->view.var_sindex != NULL ? (long)sys->view.var_sindex[i] : -1L,
			sys->view.var_mindex != NULL ? (long)sys->view.var_mindex[i] : -1L,
			name != NULL ? name : "?",
			sys->view.var_value != NULL ? sys->view.var_value[i] : 0.0,
			sys->view.var_lower != NULL ? sys->view.var_lower[i] : 0.0,
			sys->view.var_upper != NULL ? sys->view.var_upper[i] : 0.0,
			sys->view.obj_gradient != NULL ? sys->view.obj_gradient[i] : 0.0,
			sys->view.var_scale != NULL ? sys->view.var_scale[i] : 0.0
		);
		ASC_FREE(name);
	}
	for(i = 0; i < sys->view.n_rel; ++i){
		char *name = NULL;
		if(sys->server != NULL && sys->view.rels != NULL && sys->view.rels[i] != NULL){
			name = rel_make_name(sys->server,(struct rel_relation *)sys->view.rels[i]);
		}
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,
			"A4SQP_VIEW_REL: row=%ld sindex=%ld name=%s residual=%.17g lower=%.17g upper=%.17g start=%ld end=%ld",
			(long)i,
			sys->view.rel_sindex != NULL ? (long)sys->view.rel_sindex[i] : -1L,
			name != NULL ? name : "?",
			sys->view.rel_residual != NULL ? sys->view.rel_residual[i] : 0.0,
			sys->view.rel_lower != NULL ? sys->view.rel_lower[i] : 0.0,
			sys->view.rel_upper != NULL ? sys->view.rel_upper[i] : 0.0,
			sys->view.jac_row_start != NULL ? (long)sys->view.jac_row_start[i] : -1L,
			sys->view.jac_row_start != NULL ? (long)sys->view.jac_row_start[i + 1] : -1L
		);
		ASC_FREE(name);
		if(sys->view.jac_row_start == NULL){
			continue;
		}
		for(k = sys->view.jac_row_start[i]; k < sys->view.jac_row_start[i + 1]; ++k){
			ERROR_REPORTER_HERE(ASC_PROG_NOTE,
				"A4SQP_VIEW_JAC: row=%ld col=%ld col_sindex=%ld value=%.17g scaled=%.17g",
				(long)i,
				sys->view.jac_col_index != NULL ? (long)sys->view.jac_col_index[k] : -1L,
				sys->view.jac_col_sindex != NULL ? (long)sys->view.jac_col_sindex[k] : -1L,
				sys->view.jac_value != NULL ? sys->view.jac_value[k] : 0.0,
				sys->view.scaled_jac_value != NULL ? sys->view.scaled_jac_value[k] : 0.0
			);
		}
	}
}

void asc_a4sqp_report_view(struct A4SqpSystem *sys){
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
		asc_a4sqp_report_view_dump(sys);
	}
}

void asc_a4sqp_report_qp(struct A4SqpSystem *sys){
	char message[256];

	if(sys == NULL){
		return;
	}
	snprintf(
		message,
		sizeof(message),
		"qp: details owned by liba4sqp core"
	);
	a4sqp_report_progress(&sys->params,message);
}

void asc_a4sqp_report_iteration(struct A4SqpSystem *sys){
	char message[768];

	if(sys == NULL){
		return;
	}
	snprintf(
		message,
		sizeof(message),
		"iter=%ld mode=%s mode_switches=%d regular_iter=%d restoration_iter=%d restoration_entries=%d restoration_exits=%d restoration_handoffs=%d obj=%g merit=%g pred=%g rho=%g delta=%g viol_sum=%g viol_max=%g kkt=%g dual=%g compl=%g hreg=%g alpha=%g step=%g worst_rel=%ld rel_eq=%ld rel_active=%ld rel_near=%ld rel_inactive=%ld bound_lower=%ld bound_upper=%ld bound_fixed=%ld bound_stat=%g bound_worst=%ld",
		(long)sys->status.iteration,
		asc_a4sqp_phase_name(sys->last_phase),
		sys->phase_switches,
		sys->regular_iterations,
		sys->restoration_iterations,
		sys->restoration_entries,
		sys->restoration_exits,
		sys->restoration_handoffs,
		sys->view.obj != NULL ? sys->view.obj_value : 0.0,
		sys->last_merit_after,
		sys->last_predicted_reduction,
		sys->last_trust_ratio,
		sys->trust_radius,
		sys->last_violation_sum,
		sys->last_violation_max,
		sys->last_kkt_error,
		sys->last_dual_infeasibility,
		sys->last_complementarity,
		sys->last_regularization_size,
		sys->last_alpha,
		sys->last_step_norm,
		(long)sys->worst_violation_rel,
		(long)sys->rel_stats.equality,
		(long)sys->rel_stats.active,
		(long)sys->rel_stats.near_active,
		(long)sys->rel_stats.inactive,
		(long)sys->bound_stats.lower_active,
		(long)sys->bound_stats.upper_active,
		(long)sys->bound_stats.fixed_active,
		sys->bound_stats.stationarity_inf,
		(long)sys->bound_stats.worst_index
	);
	a4sqp_report_progress(&sys->params,message);
}
