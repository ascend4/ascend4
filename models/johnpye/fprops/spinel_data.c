#include "spinel_data.h"

#include <string.h>

#include "spinel_fe_degterov.h"

static int source_match(const char *entry_source, const char *source){
	if(!source){
		return 1;
	}
	if(!entry_source){
		return 0;
	}
	return NULL != strstr(entry_source, source);
}

const FeSpinelPhaseDef *spinel_phase_lookup(const char *name, const char *source){
	const FeSpinelPhaseDef *phases[] = {
		spinel_fe_degterov_phase(),
		spinel_feoxide_recon_phase(),
		spinel_fe_bg_tuned_phase(),
		spinel_fe_mmc1_guess_phase(),
		spinel_fe_hidayat_adj1_phase()
	};
	unsigned i;
	if(!name){
		return NULL;
	}
	for(i = 0; i < sizeof(phases) / sizeof(phases[0]); ++i){
		const FeSpinelPhaseDef *P = phases[i];
		if(0 == strcmp(P->name, name) && source_match(P->source, source)){
			return P;
		}
	}
	return NULL;
}

int spinel_phase_lookup_member(const char *name, const char *source,
		const FeSpinelPhaseDef **phase, unsigned *member_index){
	const FeSpinelPhaseDef *phases[] = {
		spinel_fe_degterov_phase(),
		spinel_feoxide_recon_phase(),
		spinel_fe_bg_tuned_phase(),
		spinel_fe_mmc1_guess_phase(),
		spinel_fe_hidayat_adj1_phase()
	};
	unsigned p;
	unsigned i;
	if(!name || !phase || !member_index){
		return 0;
	}
	for(p = 0; p < sizeof(phases) / sizeof(phases[0]); ++p){
		const FeSpinelPhaseDef *P = phases[p];
		if(!source_match(P->source, source)){
			continue;
		}
		for(i = 0; i < 5; ++i){
			if(0 == strcmp(P->member_names[i], name)){
				*phase = P;
				*member_index = i;
				return 1;
			}
		}
	}
	return 0;
}

unsigned spinel_phase_member_count(const FeSpinelPhaseDef *phase){
	(void)phase;
	return 5;
}

unsigned spinel_phase_member_nelem(const FeSpinelPhaseDef *phase, unsigned member_index){
	if(!phase || member_index >= 5){
		return 0;
	}
	return phase->member_nelem[member_index];
}

const char **spinel_phase_member_elements(const FeSpinelPhaseDef *phase, unsigned member_index){
	if(!phase || member_index >= 5){
		return NULL;
	}
	return (const char **)phase->member_elements[member_index];
}

const double *spinel_phase_member_stoich(const FeSpinelPhaseDef *phase, unsigned member_index){
	if(!phase || member_index >= 5){
		return NULL;
	}
	return phase->member_stoich[member_index];
}

int spinel_phase_eval(const FeSpinelPhaseDef *phase, const double *n_members,
		double T, double p, double *g_out, double *mu_out){
	if(!phase || !phase->eval){
		return 0;
	}
	return phase->eval(n_members, T, p, g_out, mu_out);
}
