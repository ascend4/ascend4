/*
 * HiGHS QP backend experiments for A4SQP.
 */

#ifndef ASC_A4SQP_QP_HIGHS_H
#define ASC_A4SQP_QP_HIGHS_H

#include <ascend/general/platform.h>

struct A4SqpQpSpikeResult {
	int highs_status;
	int highs_model_status;
	double objective_value;
	double col_value[3];
	double col_dual[3];
	double row_value[2];
	double row_dual[2];
};

ASC_EXPORT int a4sqp_qp_highs_spike(struct A4SqpQpSpikeResult *result);

#endif
