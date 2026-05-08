/*
 * Internal A4SQP structures.
 */

#ifndef ASC_A4SQP_INTERNAL_H
#define ASC_A4SQP_INTERNAL_H

#include "a4sqp_view.h"
#include "a4sqp_params.h"
#include "a4sqp_qp_highs.h"

#include <ascend/system/slv_client.h>
#include <ascend/system/slv_common.h>

struct A4SqpSystem {
	slv_system_t server;
	slv_parameters_t params;
	struct slv_parameter param_data[A4SQP_PARAM_COUNT];
	slv_status_t status;
	struct A4SqpView view;
	struct A4SqpQp qp;
	real64 last_merit_before;
	real64 last_merit_after;
	real64 last_violation_sum;
	real64 last_violation_max;
	real64 last_alpha;
	real64 last_step_norm;
	int32 worst_violation_rel;
	int line_search_failed;
};

#endif
