/*
 * Internal A4SQP structures.
 */

#ifndef ASC_A4SQP_INTERNAL_H
#define ASC_A4SQP_INTERNAL_H

#include "a4sqp_core.h"
#include "a4sqp_view.h"
#include "asc_a4sqp_params.h"

#include <ascend/system/slv_client.h>
#include <ascend/system/slv_common.h>

struct A4SqpSystem {
	slv_system_t server;
	slv_parameters_t params;
	struct slv_parameter param_data[A4SQP_PARAM_COUNT];
		slv_status_t status;
		struct A4SqpView view;
		real64 *x;
	int32 x_n;
	struct A4SqpCoreBoundStats bound_stats;
	real64 last_merit_before;
	real64 last_merit_after;
	real64 last_model_merit_after;
	real64 last_predicted_reduction;
	real64 last_linearized_violation;
	real64 last_violation_sum;
	real64 last_violation_max;
	real64 last_dual_infeasibility;
	real64 last_complementarity;
	real64 last_kkt_error;
	real64 last_regularization_size;
	real64 last_alpha;
	real64 last_step_norm;
	real64 trust_radius;
	real64 last_trust_ratio;
	int32 worst_violation_rel;
	int line_search_failed;
	int acceptable_count;
	int solved_acceptable;
	struct A4SqpCoreRestorationState restoration_state;
	enum A4SqpCorePhase last_phase;
	int phase_switches;
	int regular_iterations;
	int restoration_iterations;
	int restoration_entries;
	int restoration_exits;
	int restoration_handoffs;
};

#endif
