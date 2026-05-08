/*
 * A4SQP diagnostics.
 */

#define ASC_BUILDING_INTERFACE

#include "a4sqp_diag.h"

#include "a4sqp.h"
#include "a4sqp_params.h"

#include <ascend/general/platform.h>
#include <ascend/utilities/error.h>
#include <ascend/solver/solver.h>

void a4sqp_report_progress(slv_parameters_t *params, const char *message){
	if(message == NULL){
		return;
	}
	if(params != NULL && SLV_PARAM_BOOL(params,A4SQP_PARAM_PROGRESS_LOG)){
		ERROR_REPORTER_NOLINE(ASC_PROG_NOTE,"(A4SQP progress) %s",message);
	}
	if(params == NULL || SLV_PARAM_BOOL(params,A4SQP_PARAM_PROGRESS_CALLBACKS)){
		(void)slv_report_progress(A4SQP_SOLVER_NAME,message);
	}
}
