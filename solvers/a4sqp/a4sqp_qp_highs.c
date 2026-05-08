/*
 * Small HiGHS QP exercises used while bringing up the A4SQP backend.
 */

#define ASC_BUILDING_INTERFACE

#include "a4sqp_qp_highs.h"

#include <string.h>

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include <interfaces/highs_c_api.h>

ASC_EXPORT int a4sqp_qp_highs_spike(struct A4SqpQpSpikeResult *result){
	void *highs;
	HighsInt status;
	HighsInt model_status;
	double inf;
	const HighsInt num_col = 3;
	const HighsInt num_row = 2;
	const HighsInt num_nz = 4;
	const HighsInt q_num_nz = 1;
	const double col_cost[3] = {0.0, 10.0, 10.0};
	double col_lower[3];
	double col_upper[3];
	double row_lower[2];
	double row_upper[2];
	const HighsInt a_start[4] = {0, 2, 3, 4};
	const HighsInt a_index[4] = {0, 1, 0, 1};
	const double a_value[4] = {1.0, 1.0, 1.0, -1.0};
	const HighsInt q_start[4] = {0, 1, 1, 1};
	const HighsInt q_index[1] = {0};
	const double q_value[1] = {1.0};
	double col_value[3] = {0.0, 0.0, 0.0};
	double col_dual[3] = {0.0, 0.0, 0.0};
	double row_value[2] = {0.0, 0.0};
	double row_dual[2] = {0.0, 0.0};

	if(result == NULL){
		return 1;
	}
	memset(result,0,sizeof(*result));
	result->highs_status = kHighsStatusError;
	result->highs_model_status = kHighsModelStatusNotset;

	highs = Highs_create();
	if(highs == NULL){
		return 1;
	}

	inf = Highs_getInfinity(highs);
	col_lower[0] = 0.0;
	col_lower[1] = 0.0;
	col_lower[2] = 0.0;
	col_upper[0] = 0.25;
	col_upper[1] = inf;
	col_upper[2] = inf;
	row_lower[0] = 1.0;
	row_upper[0] = inf;
	row_lower[1] = -inf;
	row_upper[1] = 2.0;

	(void)Highs_setBoolOptionValue(highs,"output_flag",0);

	status = Highs_passModel(
		highs,
		num_col,
		num_row,
		num_nz,
		q_num_nz,
		kHighsMatrixFormatColwise,
		kHighsHessianFormatTriangular,
		kHighsObjSenseMinimize,
		0.0,
		col_cost,
		col_lower,
		col_upper,
		row_lower,
		row_upper,
		a_start,
		a_index,
		a_value,
		q_start,
		q_index,
		q_value,
		NULL
	);
	if(status != kHighsStatusError){
		status = Highs_run(highs);
	}

	model_status = Highs_getModelStatus(highs);
	result->highs_status = (int)status;
	result->highs_model_status = (int)model_status;

	if(status != kHighsStatusError && model_status == kHighsModelStatusOptimal){
		if(Highs_getSolution(highs,col_value,col_dual,row_value,row_dual) == kHighsStatusError){
			Highs_destroy(highs);
			return 1;
		}
		memcpy(result->col_value,col_value,sizeof(col_value));
		memcpy(result->col_dual,col_dual,sizeof(col_dual));
		memcpy(result->row_value,row_value,sizeof(row_value));
		memcpy(result->row_dual,row_dual,sizeof(row_dual));
		result->objective_value =
			0.5 * col_value[0] * col_value[0]
			+ 10.0 * col_value[1]
			+ 10.0 * col_value[2];
		Highs_destroy(highs);
		return 0;
	}

	Highs_destroy(highs);
	return 1;
}
