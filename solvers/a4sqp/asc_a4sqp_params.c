/*
 * A4SQP solver parameters.
 */

#define ASC_BUILDING_INTERFACE

#include "asc_a4sqp_params.h"
#include "a4sqp_c.h"

#include <ascend/general/ascMalloc.h>

static int asc_a4sqp_define_core_parameter(
	slv_parameters_t *parameters,
	int index,
	const struct A4SqpOptionInfo *info
){
	if(parameters == NULL || info == NULL || info->keyword == NULL){
		return -1;
	}
	switch(info->type){
	case A4SqpOptionInteger:
		return slv_param_int(parameters,index,
			(SlvParameterInitInt){{info->keyword,
				info->label,
				info->display,
				info->description
			}, (int)info->default_number, (int)info->lower, (int)info->upper}
		);
	case A4SqpOptionBool:
		return slv_param_bool(parameters,index,
			(SlvParameterInitBool){{info->keyword,
				info->label,
				info->display,
				info->description
			}, info->default_number != 0.0}
		);
	case A4SqpOptionNumber:
		return slv_param_real(parameters,index,
			(SlvParameterInitReal){{info->keyword,
				info->label,
				info->display,
				info->description
			}, info->default_number, info->lower, info->upper}
		);
	case A4SqpOptionString:
		return slv_param_char(parameters,index,
			(SlvParameterInitChar){{info->keyword,
				info->label,
				info->display,
				info->description
			}, info->default_string}, (char **)info->choices
		);
	default:
		return -1;
	}
}

int a4sqp_get_default_parameters(
	slv_system_t server,
	SlvClientToken asys,
	slv_parameters_t *parameters
){
	struct slv_parameter *new_parms = NULL;
	A4SqpIndex core_count;
	A4SqpIndex i;

	(void)server;
	(void)asys;

	if(parameters == NULL){
		return -1;
	}

	if(parameters->parms == NULL){
		new_parms = ASC_NEW_ARRAY_OR_NULL(struct slv_parameter,A4SQP_PARAM_COUNT);
		if(new_parms == NULL){
			return -1;
		}
		parameters->parms = new_parms;
		parameters->dynamic_parms = 1;
	}

	parameters->num_parms = 0;

	/* ASCEND-adapter controls: these do not exist in liba4sqp.so because they
	 * select ASCEND evaluation/reporting behaviour rather than SQP behaviour.
	 */
	slv_param_bool(parameters,A4SQP_PARAM_SAFE_CALC,
		(SlvParameterInitBool){{"safeeval",
			"Use safe evaluation?",1,
			"Use ASCEND safe function evaluation routines."
		}, FALSE}
	);

	slv_param_bool(parameters,A4SQP_PARAM_PROGRESS_CALLBACKS,
		(SlvParameterInitBool){{"progress_callbacks",
			"Enable progress callbacks?",2,
			"Emit in-run A4SQP progress messages through slv_report_progress."
		}, TRUE}
	);

	slv_param_bool(parameters,A4SQP_PARAM_PROGRESS_LOG,
		(SlvParameterInitBool){{"progress_log",
			"Log progress?",2,
			"Emit progress messages through the ASCEND error reporter as notes."
		}, FALSE}
	);

	slv_param_bool(parameters,A4SQP_PARAM_DUMP_VIEW,
		(SlvParameterInitBool){{"dump_view",
			"Dump problem view?",3,
			"Emit a developer diagnostic dump of the constructed A4SQP problem view."
		}, FALSE}
	);

	core_count = GetA4SqpOptionCount();
	if(A4SQP_PARAM_CORE_BASE + core_count != A4SQP_PARAM_COUNT){
		return -1;
	}
	for(i = 0; i < core_count; ++i){
		struct A4SqpOptionInfo info;
		if(!GetA4SqpOptionInfo(i,&info)){
			return -1;
		}
		if(asc_a4sqp_define_core_parameter(parameters,A4SQP_PARAM_CORE_BASE + i,&info) < 0){
			return -1;
		}
	}

	return 0;
}
