#include "thermo_solution_binary.h"

#include <math.h>

static int solution_binary_extract_x(const double *x, double *xb, FpropsError *err){
	if(!err){
		return 0;
	}
	*err = FPROPS_NO_ERROR;
	if(!x || !xb){
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	if(!(x[0] > 0.0) || !(x[1] > 0.0)){
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	if(fabs((x[0] + x[1]) - 1.0) > 1e-9){
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	*xb = x[1];
	return 1;
}

static double solution_binary_g_cb(const void *ctx, double T, double p, const double *x,
		FpropsError *err){
	double xb;
	if(!solution_binary_extract_x(x, &xb, err)){
		return NAN;
	}
	return solution_binary_g_molar((const BinarySolutionModel *)ctx, T, p, xb, err);
}

static double solution_binary_mu_cb(const void *ctx, unsigned i, double T, double p,
		const double *x, FpropsError *err){
	double xb;
	if(!solution_binary_extract_x(x, &xb, err)){
		return NAN;
	}
	if(i == 0){
		return solution_binary_mu_a((const BinarySolutionModel *)ctx, T, p, xb, err);
	}
	if(i == 1){
		return solution_binary_mu_b((const BinarySolutionModel *)ctx, T, p, xb, err);
	}
	*err = FPROPS_INVALID_REQUEST;
	return NAN;
}

static const ThermoModel solution_binary_model = {
	.h = NULL,
	.s = NULL,
	.g = solution_binary_g_cb,
	.mu = solution_binary_mu_cb,
	.sat_T = NULL
};

const ThermoModel *thermo_model_solution_binary(void){
	return &solution_binary_model;
}
