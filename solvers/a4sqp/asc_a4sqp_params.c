/*
 * A4SQP solver parameters.
 */

#define ASC_BUILDING_INTERFACE

#include "asc_a4sqp_params.h"

#include <ascend/general/ascMalloc.h>

#define ASC_A4SQP_DEFAULT_ELASTIC_PENALTY 100.0

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
		}, FALSE}
	);

	slv_param_char(parameters,A4SQP_PARAM_SCALEOPT,
		(SlvParameterInitChar){{"scaleopt",
			"Scaling option",1,
			"QRSlv-style scaling option for the A4SQP problem view."
		}, "ROW_2NORM"}, (char *[]){"NONE","ROW_2NORM","RELNOM",NULL}
	);

	slv_param_char(parameters,A4SQP_PARAM_HESS_MODE,
		(SlvParameterInitChar){{"hessian",
			"Hessian model",1,
			"Step Hessian model for the SQP QP: BFGS, EXACT_OBJ, EXACT_LAGRANGIAN, or AUTO."
		}, "BFGS"}, (char *[]){"AUTO","BFGS","EXACT_OBJ","EXACT_LAGRANGIAN",NULL}
	);

	slv_param_real(parameters,A4SQP_PARAM_HESS_REG,
		(SlvParameterInitReal){{"hess_reg",
			"Hessian regularization",2,
			"Minimum diagonal margin enforced when regularizing the step Hessian to a convex QP model."
		}, 1e-8, 0.0, 1e12}
	);

	slv_param_real(parameters,A4SQP_PARAM_HESS_FALLBACK_RATIO,
		(SlvParameterInitReal){{"hess_fallback_ratio",
			"Exact Hessian fallback ratio",2,
			"Use the BFGS fallback model for an exact-Hessian step when PSD regularization divided by the Hessian infinity norm exceeds this ratio; zero disables fallback."
		}, 1.0, 0.0, 1e12}
	);

	slv_param_real(parameters,A4SQP_PARAM_BOUND_PUSH,
		(SlvParameterInitReal){{"bound_push",
			"Bound push",2,
			"Distance used when projecting an out-of-bounds initial value just inside finite variable bounds."
		}, 1e-8, 0.0, 1e3}
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

	slv_param_real(parameters,A4SQP_PARAM_ACCEPTABLE_TOL,
		(SlvParameterInitReal){{"acceptable_tol",
			"Acceptable tolerance",1,
			"Relaxed feasibility/stationarity tolerance for acceptable solves."
		}, 1e-5, 1e-14, 1e3}
	);

	slv_param_int(parameters,A4SQP_PARAM_ACCEPTABLE_ITER,
		(SlvParameterInitInt){{"acceptable_iter",
			"Acceptable iterations",1,
			"Number of consecutive acceptable iterations required before accepting a relaxed solve."
		}, 0, 0, 10000}
	);

	slv_param_bool(parameters,A4SQP_PARAM_KKT_CONVERGENCE,
		(SlvParameterInitBool){{"kkt_convergence",
			"KKT convergence",2,
			"Use the core KKT residual, rather than constrained small-step acceptance, for objective stationarity convergence."
		}, 0}
	);

	slv_param_real(parameters,A4SQP_PARAM_MERIT_TOL,
		(SlvParameterInitReal){{"merit_tol",
			"Merit tolerance",2,
			"Minimum merit decrease regarded as meaningful by the line search."
		}, 1e-14, 0.0, 1e3}
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
		}, ASC_A4SQP_DEFAULT_ELASTIC_PENALTY, 1e-12, 1e12}
	);

	slv_param_real(parameters,A4SQP_PARAM_ELASTIC_PENALTY_GROWTH,
		(SlvParameterInitReal){{"elastic_penalty_growth",
			"Elastic penalty growth",2,
			"Multiplier applied when the core detects saturated elastic-QP duals while infeasibility remains."
		}, 1.0, 1.0, 1e6}
	);

	slv_param_real(parameters,A4SQP_PARAM_ELASTIC_PENALTY_MAX,
		(SlvParameterInitReal){{"elastic_penalty_max",
			"Maximum elastic penalty",2,
			"Upper bound for automatic elastic/merit penalty increases."
		}, 1e8, 1e-12, 1e16}
	);

	slv_param_bool(parameters,A4SQP_PARAM_FILTER_ACCEPT,
		(SlvParameterInitBool){{"filter_accept",
			"Filter-lite acceptance",2,
			"Permit feasibility-improving constrained steps that reduce merit but fail the predicted-reduction ratio."
		}, 0}
	);

	slv_param_real(parameters,A4SQP_PARAM_FILTER_MARGIN,
		(SlvParameterInitReal){{"filter_margin",
			"Filter-lite margin",2,
			"Required fractional constraint-violation reduction for filter-lite acceptance."
		}, 1e-4, 0.0, 0.999999}
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
		}, 0.0, -1e6, 1e6}
	);

		slv_param_real(parameters,A4SQP_PARAM_TRUST_GOOD,
			(SlvParameterInitReal){{"trust_good",
				"Trust good ratio",2,
				"Actual-to-predicted merit reduction ratio that triggers trust-region growth when the step is trust-active."
			}, 0.75, -1e6, 1e6}
		);

		slv_param_real(parameters,A4SQP_PARAM_TRUST_TINY_ALPHA,
			(SlvParameterInitReal){{"trust_tiny_alpha",
				"Tiny-alpha trust shrink",2,
				"Accepted regular steps with alpha at or below this value shrink the trust radius toward the accepted step size; zero disables this update."
			}, 0.0, 0.0, 1.0}
		);

		slv_param_real(parameters,A4SQP_PARAM_TRUST_TINY_RADIUS_FACTOR,
			(SlvParameterInitReal){{"trust_tiny_radius_factor",
				"Tiny-alpha trust factor",2,
				"Multiplier on accepted scaled step size used as the new trust-radius target after a tiny-alpha accepted step."
			}, 2.0, 1.0, 100.0}
		);

		slv_param_int(parameters,A4SQP_PARAM_TRUST_QP_RETRIES,
		(SlvParameterInitInt){{"trust_qp_retries",
			"Trust-region retries",2,
			"Maximum number of trust-radius reductions and QP rebuild retries per SQP iteration."
		}, 5, 0, 100}
	);

	slv_param_bool(parameters,A4SQP_PARAM_TRUST_UNCONSTRAINED,
		(SlvParameterInitBool){{"trust_unconstrained",
			"Trust unconstrained",2,
			"Apply the scaled trust-region radius and trust retries to unconstrained objective-only problems."
		}, 0}
	);

	slv_param_bool(parameters,A4SQP_PARAM_RESTORATION,
		(SlvParameterInitBool){{"restoration",
			"Restoration phase",2,
			"Enable feasibility restoration steps after repeated lack of constraint-violation progress."
		}, 0}
	);

	slv_param_int(parameters,A4SQP_PARAM_RESTORATION_TRIGGER_ITER,
		(SlvParameterInitInt){{"restoration_trigger_iter",
			"Restoration trigger",2,
			"Number of consecutive non-improving infeasible iterations before restoration steps are requested."
		}, 3, 0, 10000}
	);

	slv_param_int(parameters,A4SQP_PARAM_RESTORATION_MAX_ITER,
		(SlvParameterInitInt){{"restoration_max_iter",
			"Restoration handoff",2,
			"Maximum consecutive restoration iterations before handing back to the regular SQP objective model; zero disables the cap."
		}, 0, 0, 10000}
	);

	slv_param_real(parameters,A4SQP_PARAM_RESTORATION_IMPROVE,
		(SlvParameterInitReal){{"restoration_improve",
			"Restoration progress",2,
			"Required fractional maximum-violation improvement to reset the restoration stall counter."
		}, 1e-3, 0.0, 0.999999}
	);

	slv_param_real(parameters,A4SQP_PARAM_RESTORATION_MARGIN,
		(SlvParameterInitReal){{"restoration_margin",
			"Restoration acceptance",2,
			"Required fractional constraint-violation reduction for restoration line-search acceptance."
		}, 1e-4, 0.0, 0.999999}
	);

		slv_param_real(parameters,A4SQP_PARAM_RESTORATION_HANDOFF_REDUCTION,
			(SlvParameterInitReal){{"restoration_handoff_reduction",
				"Restoration handoff reduction",2,
				"Fractional constraint-violation reduction from restoration entry required before trying the regular SQP objective model again; zero disables improvement-based handoff."
			}, 0.5, 0.0, 0.999999}
		);

		slv_param_real(parameters,A4SQP_PARAM_RESTORATION_REENTRY_FACTOR,
			(SlvParameterInitReal){{"restoration_reentry_factor",
				"Restoration re-entry factor",2,
				"Multiplier on the restoration exit tolerance before immediate restoration re-entry is allowed after a handoff."
			}, 1.0, 1.0, 1e6}
		);

		return 0;
	}
