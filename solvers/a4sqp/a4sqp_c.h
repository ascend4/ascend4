/*
 * IPOPT-like C interface for A4SQP.
 */

#ifndef ASC_A4SQP_C_H
#define ASC_A4SQP_C_H

#ifdef _MSC_VER
# ifdef A4SQP_DLL
#  define A4SQP_EXPORT(type) __declspec(dllexport) type __cdecl
# else
#  define A4SQP_EXPORT(type) type __cdecl
# endif
#elif defined(__GNUC__) || defined(__clang__)
# define A4SQP_EXPORT(type) __attribute__((visibility("default"))) type
#else
# define A4SQP_EXPORT(type) type
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef double A4SqpNumber;
typedef int A4SqpIndex;
typedef int A4SqpInt;
typedef int A4SqpBool;
typedef void *A4SqpUserDataPtr;

#ifndef A4SQP_TRUE
# define A4SQP_TRUE 1
#endif
#ifndef A4SQP_FALSE
# define A4SQP_FALSE 0
#endif

struct A4SqpProblemInfo;
typedef struct A4SqpProblemInfo *A4SqpProblem;

typedef A4SqpBool (*A4SqpEvalFCB)(
	A4SqpIndex n,
	A4SqpNumber *x,
	A4SqpBool new_x,
	A4SqpNumber *obj_value,
	A4SqpUserDataPtr user_data
);

typedef A4SqpBool (*A4SqpEvalGradFCB)(
	A4SqpIndex n,
	A4SqpNumber *x,
	A4SqpBool new_x,
	A4SqpNumber *grad_f,
	A4SqpUserDataPtr user_data
);

typedef A4SqpBool (*A4SqpEvalGCB)(
	A4SqpIndex n,
	A4SqpNumber *x,
	A4SqpBool new_x,
	A4SqpIndex m,
	A4SqpNumber *g,
	A4SqpUserDataPtr user_data
);

typedef A4SqpBool (*A4SqpEvalJacGCB)(
	A4SqpIndex n,
	A4SqpNumber *x,
	A4SqpBool new_x,
	A4SqpIndex m,
	A4SqpIndex nele_jac,
	A4SqpIndex *iRow,
	A4SqpIndex *jCol,
	A4SqpNumber *values,
	A4SqpUserDataPtr user_data
);

typedef A4SqpBool (*A4SqpEvalHCB)(
	A4SqpIndex n,
	A4SqpNumber *x,
	A4SqpBool new_x,
	A4SqpNumber obj_factor,
	A4SqpIndex m,
	A4SqpNumber *lambda,
	A4SqpBool new_lambda,
	A4SqpIndex nele_hess,
	A4SqpIndex *iRow,
	A4SqpIndex *jCol,
	A4SqpNumber *values,
	A4SqpUserDataPtr user_data
);

typedef A4SqpBool (*A4SqpIntermediateCB)(
	A4SqpIndex alg_mod,
	A4SqpIndex iter_count,
	A4SqpNumber obj_value,
	A4SqpNumber inf_pr,
	A4SqpNumber inf_du,
	A4SqpNumber mu,
	A4SqpNumber d_norm,
	A4SqpNumber regularization_size,
	A4SqpNumber alpha_du,
	A4SqpNumber alpha_pr,
	A4SqpIndex ls_trials,
	A4SqpUserDataPtr user_data
);

enum A4SqpOptionType {
	A4SqpOptionNumber = 0,
	A4SqpOptionInteger = 1,
	A4SqpOptionBool = 2,
	A4SqpOptionString = 3
};

struct A4SqpOptionInfo {
	const char *keyword;
	const char *label;
	const char *description;
	A4SqpInt display;
	enum A4SqpOptionType type;
	A4SqpNumber default_number;
	A4SqpNumber lower;
	A4SqpNumber upper;
	const char *default_string;
	const char *const *choices;
	A4SqpBool problem_option;
};

enum A4SqpApplicationReturnStatus {
	A4SqpSolveSucceeded = 0,
	A4SqpSolvedToAcceptableLevel = 1,
	A4SqpInfeasibleProblemDetected = 2,
	A4SqpSearchDirectionBecomesTooSmall = 3,
	A4SqpDivergingIterates = 4,
	A4SqpUserRequestedStop = 5,
	A4SqpFeasiblePointFound = 6,

	A4SqpMaximumIterationsExceeded = -1,
	A4SqpRestorationFailed = -2,
	A4SqpErrorInStepComputation = -3,
	A4SqpMaximumCpuTimeExceeded = -4,
	A4SqpNotEnoughDegreesOfFreedom = -10,
	A4SqpInvalidProblemDefinition = -11,
	A4SqpInvalidOption = -12,
	A4SqpInvalidNumberDetected = -13,

	A4SqpUnrecoverableException = -100,
	A4SqpInsufficientMemory = -102,
	A4SqpInternalError = -199
};

enum A4SqpAlgorithmMode {
	A4SqpRegularMode = 0,
	A4SqpRestorationPhaseMode = 1
};

struct A4SqpSolveStats {
	A4SqpInt iterations;
	A4SqpInt qp_solves;
	A4SqpInt qp_failures;
	A4SqpInt line_search_failures;
	A4SqpInt algorithm_mode;
	A4SqpInt mode_switches;
	A4SqpInt regular_iterations;
	A4SqpInt restoration_iterations;
	A4SqpInt restoration_entries;
	A4SqpInt restoration_exits;
	A4SqpInt restoration_handoffs;
	A4SqpNumber objective;
	A4SqpNumber max_constraint_violation;
	A4SqpNumber projected_gradient_inf;
	A4SqpNumber kkt_error;
	A4SqpNumber dual_infeasibility_inf;
	A4SqpNumber complementarity_inf;
	A4SqpInt kkt_lambda_sign;
	A4SqpInt bound_lower_active;
	A4SqpInt bound_upper_active;
	A4SqpInt bound_fixed_active;
	A4SqpInt bound_worst_index;
	A4SqpNumber bound_stationarity_inf;
	A4SqpNumber bound_worst_lagrangian_gradient;
	A4SqpNumber final_step_norm;
	A4SqpNumber final_trust_radius;
	A4SqpNumber final_elastic_max;
	A4SqpNumber regularization_size;
	A4SqpInt reduced_gradient_polish_mode;
	A4SqpInt reduced_gradient_polish_attempts;
	A4SqpInt reduced_gradient_polish_accepts;
};

A4SQP_EXPORT(A4SqpProblem) CreateA4SqpProblem(
	A4SqpIndex n,
	A4SqpNumber *x_L,
	A4SqpNumber *x_U,
	A4SqpIndex m,
	A4SqpNumber *g_L,
	A4SqpNumber *g_U,
	A4SqpIndex nele_jac,
	A4SqpIndex nele_hess,
	A4SqpIndex index_style,
	A4SqpEvalFCB eval_f,
	A4SqpEvalGCB eval_g,
	A4SqpEvalGradFCB eval_grad_f,
	A4SqpEvalJacGCB eval_jac_g,
	A4SqpEvalHCB eval_h
);

A4SQP_EXPORT(void) FreeA4SqpProblem(A4SqpProblem problem);

A4SQP_EXPORT(A4SqpBool) AddA4SqpStrOption(A4SqpProblem problem, char *keyword, char *val);
A4SQP_EXPORT(A4SqpBool) AddA4SqpNumOption(A4SqpProblem problem, char *keyword, A4SqpNumber val);
A4SQP_EXPORT(A4SqpBool) AddA4SqpIntOption(A4SqpProblem problem, char *keyword, A4SqpInt val);
A4SQP_EXPORT(A4SqpBool) OpenA4SqpOutputFile(A4SqpProblem problem, char *file_name, A4SqpInt print_level);
A4SQP_EXPORT(A4SqpBool) SetA4SqpProblemScaling(
	A4SqpProblem problem,
	A4SqpNumber obj_scaling,
	A4SqpNumber *x_scaling,
	A4SqpNumber *g_scaling
);
A4SQP_EXPORT(A4SqpBool) SetA4SqpIntermediateCallback(
	A4SqpProblem problem,
	A4SqpIntermediateCB intermediate_cb
);
A4SQP_EXPORT(A4SqpIndex) GetA4SqpOptionCount(void);
A4SQP_EXPORT(A4SqpBool) GetA4SqpOptionInfo(A4SqpIndex index, struct A4SqpOptionInfo *info);
A4SQP_EXPORT(A4SqpBool) GetA4SqpOptionInfoByName(char *keyword, struct A4SqpOptionInfo *info);

A4SQP_EXPORT(enum A4SqpApplicationReturnStatus) A4SqpSolve(
	A4SqpProblem problem,
	A4SqpNumber *x,
	A4SqpNumber *g,
	A4SqpNumber *obj_val,
	A4SqpNumber *mult_g,
	A4SqpNumber *mult_x_L,
	A4SqpNumber *mult_x_U,
	A4SqpUserDataPtr user_data
);

A4SQP_EXPORT(A4SqpBool) GetA4SqpSolveStatistics(
	A4SqpProblem problem,
	struct A4SqpSolveStats *stats
);

#ifdef __cplusplus
}
#endif

#endif
