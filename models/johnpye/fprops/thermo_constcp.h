#ifndef FPROPS_THERMO_CONSTCP_H
#define FPROPS_THERMO_CONSTCP_H

#include "thermo.h"
#include "constcp.h"

const ThermoModel *thermo_model_constcp(void);

static inline ThermoState thermo_state_constcp(const ConstCpData *D, double T, double p){
	ThermoState S;
	S.model = thermo_model_constcp();
	S.model_data = D;
	S.phase = (D && D->phase == FPROPS_CONSTCP_LIQUID) ? FPROPS_PHASE_LIQUID : FPROPS_PHASE_SOLID;
	S.T = T;
	S.p = p;
	S.x = NULL;
	S.n = 1;
	return S;
}

#endif /* FPROPS_THERMO_CONSTCP_H */
