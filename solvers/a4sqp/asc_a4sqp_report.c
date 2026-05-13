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

#include <ascend/system/slv_param.h>
#include <ascend/utilities/error.h>

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

void asc_a4sqp_report_iteration(struct A4SqpSystem *sys){
	char message[256];

	if(sys == NULL){
		return;
	}
	snprintf(
		message,
		sizeof(message),
		"iter=%ld obj=%g merit=%g pred=%g rho=%g delta=%g viol_sum=%g viol_max=%g alpha=%g step=%g worst_rel=%ld",
		(long)sys->status.iteration,
		sys->view.obj != NULL ? sys->view.obj_value : 0.0,
		sys->last_merit_after,
		sys->last_predicted_reduction,
		sys->last_trust_ratio,
		sys->trust_radius,
		sys->last_violation_sum,
		sys->last_violation_max,
		sys->last_alpha,
		sys->last_step_norm,
		(long)sys->worst_violation_rel
	);
	a4sqp_report_progress(&sys->params,message);
}
