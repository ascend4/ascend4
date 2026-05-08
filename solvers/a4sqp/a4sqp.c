/*
 * A4SQP solver registration and minimal lifecycle.
 */

#define ASC_BUILDING_INTERFACE

#include "a4sqp.h"

#include "a4sqp_ascend.h"
#include "a4sqp_diag.h"
#include "a4sqp_internal.h"
#include "a4sqp_scale.h"

#include <stdio.h>
#include <string.h>

#include <ascend/general/ascMalloc.h>
#include <ascend/general/mem.h>
#include <ascend/system/slv_common.h>
#include <ascend/system/slv_param.h>
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
		(long)sys->view.calc_errors,
		(long)sys->view.derivative_errors,
		(long)sys->view.unsupported_rels
	);
	a4sqp_report_progress(&sys->params,message);

	if(SLV_PARAM_BOOL(&sys->params,A4SQP_PARAM_DUMP_VIEW)){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"%s",message);
	}
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

	++sys->status.iteration;
	a4sqp_report_view(sys);
	return 0;
}

static int a4sqp_solve(slv_system_t server, SlvClientToken asys){
	int status;
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;

	status = a4sqp_iterate(server,asys);
	if(status == 0 && sys != NULL){
		sys->status.converged = FALSE;
		sys->status.diverged = FALSE;
		sys->status.iteration_limit_exceeded = TRUE;
		ERROR_REPORTER_HERE(ASC_PROG_WARNING,
			"A4SQP Phase 1 skeleton built the problem view but does not yet solve NLPs."
		);
	}
	return status;
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
