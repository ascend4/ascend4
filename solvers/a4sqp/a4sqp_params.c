/*
 * A4SQP solver parameters.
 */

#define ASC_BUILDING_INTERFACE

#include "a4sqp_params.h"

#include "a4sqp_qp_highs.h"

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

	slv_param_int(parameters,A4SQP_PARAM_MAX_ITER,
		(SlvParameterInitInt){{"max_iter",
			"Maximum iterations",1,
			"Maximum number of A4SQP major SQP iterations."
		}, 20, 1, 10000}
	);

	slv_param_int(parameters,A4SQP_PARAM_MAX_BACKTRACK,
		(SlvParameterInitInt){{"max_backtrack",
			"Maximum backtracking steps",2,
			"Maximum number of merit line-search backtracking steps per SQP iteration."
		}, 20, 1, 100}
	);

	slv_param_real(parameters,A4SQP_PARAM_FEAS_TOL,
		(SlvParameterInitReal){{"feas_tol",
			"Feasibility tolerance",1,
			"Convergence tolerance for maximum scaled relation violation."
		}, 1e-7, 1e-14, 1e3}
	);

	slv_param_real(parameters,A4SQP_PARAM_STEP_TOL,
		(SlvParameterInitReal){{"step_tol",
			"Step tolerance",1,
			"Convergence tolerance for physical accepted step norm."
		}, 1e-4, 1e-14, 1e3}
	);

	slv_param_real(parameters,A4SQP_PARAM_MERIT_TOL,
		(SlvParameterInitReal){{"merit_tol",
			"Merit tolerance",2,
			"Minimum merit decrease regarded as meaningful by the line search."
		}, 1e-10, 0.0, 1e3}
	);

	slv_param_real(parameters,A4SQP_PARAM_ARMIJO_COEFF,
		(SlvParameterInitReal){{"armijo_coeff",
			"Armijo coefficient",2,
			"Fraction of predicted merit reduction required by the line search."
		}, 1e-4, 1e-12, 0.5}
	);

	slv_param_real(parameters,A4SQP_PARAM_ELASTIC_PENALTY,
		(SlvParameterInitReal){{"elastic_penalty",
			"Elastic penalty",1,
			"Linear penalty used for lower and upper elastic QP slacks and the merit function."
		}, A4SQP_QP_DEFAULT_ELASTIC_PENALTY, 1e-12, 1e12}
	);

	slv_param_real(parameters,A4SQP_PARAM_TRUST_RADIUS_INIT,
		(SlvParameterInitReal){{"trust_radius_init",
			"Initial trust radius",2,
			"Initial scaled infinity-norm trust-region radius for the primal SQP step."
		}, 1.0, 1e-12, 1e12}
	);

	slv_param_real(parameters,A4SQP_PARAM_TRUST_RADIUS_MIN,
		(SlvParameterInitReal){{"trust_radius_min",
			"Minimum trust radius",2,
			"Minimum scaled trust-region radius before A4SQP gives up shrinking the QP step."
		}, 1e-6, 1e-12, 1e12}
	);

	slv_param_real(parameters,A4SQP_PARAM_TRUST_RADIUS_MAX,
		(SlvParameterInitReal){{"trust_radius_max",
			"Maximum trust radius",2,
			"Maximum scaled trust-region radius for the primal SQP step."
		}, 100.0, 1e-12, 1e12}
	);

	slv_param_real(parameters,A4SQP_PARAM_TRUST_SHRINK,
		(SlvParameterInitReal){{"trust_shrink",
			"Trust shrink factor",2,
			"Factor applied to the trust radius after QP or agreement failure."
		}, 0.25, 1e-6, 0.999999}
	);

	slv_param_real(parameters,A4SQP_PARAM_TRUST_GROW,
		(SlvParameterInitReal){{"trust_grow",
			"Trust grow factor",2,
			"Factor applied to the trust radius after a good boundary-active step."
		}, 2.0, 1.000001, 1e6}
	);

	slv_param_real(parameters,A4SQP_PARAM_TRUST_ACCEPT,
		(SlvParameterInitReal){{"trust_accept",
			"Trust acceptance ratio",2,
			"Minimum actual-to-predicted merit reduction ratio required for line-search acceptance."
		}, 0.1, -1e6, 1e6}
	);

	slv_param_real(parameters,A4SQP_PARAM_TRUST_GOOD,
		(SlvParameterInitReal){{"trust_good",
			"Trust good ratio",2,
			"Actual-to-predicted merit reduction ratio that triggers trust-region growth when the step is trust-active."
		}, 0.75, -1e6, 1e6}
	);

	slv_param_int(parameters,A4SQP_PARAM_TRUST_QP_RETRIES,
		(SlvParameterInitInt){{"trust_qp_retries",
			"Trust-region retries",2,
			"Maximum number of trust-radius reductions and QP rebuild retries per SQP iteration."
		}, 5, 0, 100}
	);

	return 0;
}
