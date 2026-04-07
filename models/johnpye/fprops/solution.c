#include "solution.h"

#include <math.h>

static double solution_gas_R(void){
	return 8.31446261815324;
}

int solution_binary_validate_x(const BinarySolutionModel *M, double x, FpropsError *err){
	if(!err){
		return 0;
	}
	*err = FPROPS_NO_ERROR;
	if(!M || !(x > 0.0) || !(x < 1.0)){
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	if(x < M->xmin || x > M->xmax){
		*err = FPROPS_RANGE_ERROR;
		return 0;
	}
	return 1;
}

static int solution_binary_require(const BinarySolutionModel *M, double x, FpropsError *err){
	if(!solution_binary_validate_x(M, x, err)){
		return 0;
	}
	if(!M->g0_a || !M->g0_b || !M->gex || !M->dgex_dx){
		*err = FPROPS_NOT_IMPLEMENTED;
		return 0;
	}
	return 1;
}

double solution_binary_g_molar(const BinarySolutionModel *M, double T, double p, double x,
		FpropsError *err){
	double one_minus_x;
	double g0a;
	double g0b;
	double gex;
	double mix;
	const double RT = solution_gas_R() * T;

	if(!solution_binary_require(M, x, err)){
		return NAN;
	}
	if(!(T > 0.0) || !(p > 0.0)){
		*err = FPROPS_INVALID_REQUEST;
		return NAN;
	}

	one_minus_x = 1.0 - x;
	g0a = M->g0_a(T, p, err);
	if(*err){
		return NAN;
	}
	g0b = M->g0_b(T, p, err);
	if(*err){
		return NAN;
	}
	gex = M->gex(T, x, err);
	if(*err){
		return NAN;
	}
	mix = RT * (one_minus_x * log(one_minus_x) + x * log(x));
	return one_minus_x * g0a + x * g0b + mix + gex;
}

double solution_binary_mu_a(const BinarySolutionModel *M, double T, double p, double x,
		FpropsError *err){
	double one_minus_x;
	double g0a;
	double gex;
	double dgdx;
	const double RT = solution_gas_R() * T;

	if(!solution_binary_require(M, x, err)){
		return NAN;
	}
	if(!(T > 0.0) || !(p > 0.0)){
		*err = FPROPS_INVALID_REQUEST;
		return NAN;
	}

	one_minus_x = 1.0 - x;
	g0a = M->g0_a(T, p, err);
	if(*err){
		return NAN;
	}
	gex = M->gex(T, x, err);
	if(*err){
		return NAN;
	}
	dgdx = M->dgex_dx(T, x, err);
	if(*err){
		return NAN;
	}
	return g0a + RT * log(one_minus_x) + gex - x * dgdx;
}

double solution_binary_mu_b(const BinarySolutionModel *M, double T, double p, double x,
		FpropsError *err){
	double g0b;
	double gex;
	double dgdx;
	const double RT = solution_gas_R() * T;

	if(!solution_binary_require(M, x, err)){
		return NAN;
	}
	if(!(T > 0.0) || !(p > 0.0)){
		*err = FPROPS_INVALID_REQUEST;
		return NAN;
	}

	g0b = M->g0_b(T, p, err);
	if(*err){
		return NAN;
	}
	gex = M->gex(T, x, err);
	if(*err){
		return NAN;
	}
	dgdx = M->dgex_dx(T, x, err);
	if(*err){
		return NAN;
	}
	return g0b + RT * log(x) + gex + (1.0 - x) * dgdx;
}
