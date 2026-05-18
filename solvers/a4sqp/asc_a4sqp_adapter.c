/*
 * A4SQP slv_system_t adapter.
 */

#define ASC_BUILDING_INTERFACE

#include "asc_a4sqp_adapter.h"

#include "asc_a4sqp_internal.h"

int asc_a4sqp_build_view(struct A4SqpSystem *sys, slv_system_t server){
	if(sys == NULL){
		return 1;
	}
	return a4sqp_view_build(
		&sys->view,
		server,
		SLV_PARAM_BOOL(&sys->params,A4SQP_PARAM_SAFE_CALC),
		SLV_PARAM_CHAR(&sys->params,A4SQP_PARAM_SCALEOPT)
	);
}
