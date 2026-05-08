/*
 * A4SQP solver parameters.
 */

#ifndef ASC_A4SQP_PARAMS_H
#define ASC_A4SQP_PARAMS_H

#include <ascend/system/slv_client.h>
#include <ascend/system/slv_param.h>

enum A4SqpParams {
	A4SQP_PARAM_SAFE_CALC = 0,
	A4SQP_PARAM_SCALEOPT,
	A4SQP_PARAM_PROGRESS_CALLBACKS,
	A4SQP_PARAM_PROGRESS_LOG,
	A4SQP_PARAM_VERBOSITY,
	A4SQP_PARAM_DUMP_VIEW,
	A4SQP_PARAM_MAX_ITER,
	A4SQP_PARAM_MAX_BACKTRACK,
	A4SQP_PARAM_FEAS_TOL,
	A4SQP_PARAM_STEP_TOL,
	A4SQP_PARAM_MERIT_TOL,
	A4SQP_PARAM_ELASTIC_PENALTY,
	A4SQP_PARAM_COUNT
};

int a4sqp_get_default_parameters(
	slv_system_t server,
	SlvClientToken asys,
	slv_parameters_t *parameters
);

#endif
