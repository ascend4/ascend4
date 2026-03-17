#ifndef FPROPS_SOLUTION_H
#define FPROPS_SOLUTION_H

#include "rundata.h"

typedef struct BinarySolutionModel_struct BinarySolutionModel;

typedef double BinarySolutionG0Fn(double T, double p, FpropsError *err);
typedef double BinarySolutionExcessFn(double T, double x, FpropsError *err);
typedef double BinarySolutionExcessDerivFn(double T, double x, FpropsError *err);

struct BinarySolutionModel_struct{
	const char *name;
	BinarySolutionG0Fn *g0_a;
	BinarySolutionG0Fn *g0_b;
	BinarySolutionExcessFn *gex;
	BinarySolutionExcessDerivFn *dgex_dx;
	BinarySolutionExcessDerivFn *d2gex_dx2;
	double xmin;
	double xmax;
};

int solution_binary_validate_x(const BinarySolutionModel *M, double x, FpropsError *err);
double solution_binary_g_molar(const BinarySolutionModel *M, double T, double p, double x,
		FpropsError *err);
double solution_binary_mu_a(const BinarySolutionModel *M, double T, double p, double x,
		FpropsError *err);
double solution_binary_mu_b(const BinarySolutionModel *M, double T, double p, double x,
		FpropsError *err);

#endif /* FPROPS_SOLUTION_H */
