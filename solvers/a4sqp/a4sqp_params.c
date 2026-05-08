/*
 * A4SQP solver parameters.
 */

#define ASC_BUILDING_INTERFACE

#include "a4sqp_params.h"

#include <ascend/general/ascMalloc.h>

int a4sqp_get_default_parameters(
	slv_system_t server,
	SlvClientToken asys,
	slv_parameters_t *parameters
){
	struct slv_parameter *new_parms = NULL;

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

	slv_param_bool(parameters,A4SQP_PARAM_SAFE_CALC,
		(SlvParameterInitBool){{"safeeval",
			"Use safe evaluation?",1,
			"Use ASCEND safe function evaluation routines."
		}, TRUE}
	);

	slv_param_char(parameters,A4SQP_PARAM_SCALEOPT,
		(SlvParameterInitChar){{"scaleopt",
			"Scaling option",1,
			"QRSlv-style scaling option for the A4SQP problem view."
		}, "ROW_2NORM"}, (char *[]){"NONE","ROW_2NORM","RELNOM",NULL}
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

	slv_param_int(parameters,A4SQP_PARAM_VERBOSITY,
		(SlvParameterInitInt){{"verbosity",
			"Verbosity",2,
			"A4SQP diagnostic verbosity level."
		}, 1, 0, 5}
	);

	slv_param_bool(parameters,A4SQP_PARAM_DUMP_VIEW,
		(SlvParameterInitBool){{"dump_view",
			"Dump problem view?",3,
			"Emit a developer diagnostic dump of the constructed A4SQP problem view."
		}, FALSE}
	);

	return 0;
}
