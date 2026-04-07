#ifndef FPROPS_THERMO_MIX_IDEAL_H
#define FPROPS_THERMO_MIX_IDEAL_H

#include "thermo.h"
#include "mixture.h"

const ThermoModel *thermo_model_mix_ideal(void);

static inline ThermoState thermo_state_mix_ideal(const FpropsMix *mix, double T, double p){
	ThermoState S;
	S.model = thermo_model_mix_ideal();
	S.model_data = mix;
	S.phase = FPROPS_PHASE_GAS;
	S.T = T;
	S.p = p;
	S.x = mix ? mix->x : NULL;
	S.n = mix ? mix->nspecies : 0;
	return S;
}

static inline ThermoState thermo_state_pure_as_mix(const PureFluid *P, double T, double p, FpropsError *err){
	ThermoState S;
	FpropsMix *mix = fprops_mix_from_pure(P, err);
	S.model = thermo_model_mix_ideal();
	S.model_data = mix;
	S.phase = FPROPS_PHASE_GAS;
	S.T = T;
	S.p = p;
	S.x = mix ? mix->x : NULL;
	S.n = mix ? mix->nspecies : 0;
	return S;
}

#endif /* FPROPS_THERMO_MIX_IDEAL_H */
