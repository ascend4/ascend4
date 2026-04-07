#ifndef FPROPS_THERMO_PURE_H
#define FPROPS_THERMO_PURE_H

#include "thermo.h"
#include "rundata.h"

const ThermoModel *thermo_model_pure(void);

static inline ThermoState thermo_state_pure(const PureFluid *P, double T, double p){
	ThermoState S;
	S.model = thermo_model_pure();
	S.model_data = P;
	S.phase = FPROPS_PHASE_GAS;
	S.T = T;
	S.p = p;
	S.x = NULL;
	S.n = 1;
	return S;
}

#endif /* FPROPS_THERMO_PURE_H */
