#ifndef FPROPS_THERMO_SOLUTION_BINARY_H
#define FPROPS_THERMO_SOLUTION_BINARY_H

#include "solution.h"
#include "thermo.h"

const ThermoModel *thermo_model_solution_binary(void);

static inline ThermoState thermo_state_solution_binary(const BinarySolutionModel *M, double T,
		double p, const double *x){
	ThermoState S;
	S.model = thermo_model_solution_binary();
	S.model_data = M;
	S.phase = FPROPS_PHASE_SOLID;
	S.T = T;
	S.p = p;
	S.x = x;
	S.n = 2;
	return S;
}

#endif /* FPROPS_THERMO_SOLUTION_BINARY_H */
