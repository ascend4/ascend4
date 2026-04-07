#include "solution_data.h"

#include <string.h>

#include "bcc_iron_hidayat.h"
#include "fcc_iron_hidayat.h"
#include "wustite_hidayat.h"

static int source_match(const char *entry_source, const char *source){
	if(!source){
		return 1;
	}
	if(!entry_source){
		return 0;
	}
	return NULL != strstr(entry_source, source);
}

const BinarySolutionPhaseDef *solution_phase_lookup(const char *name, const char *source){
	const BinarySolutionPhaseDef *phases[] = {
		bcc_iron_hidayat_phase(),
		fcc_iron_hidayat_phase(),
		wustite_hidayat_phase()
	};
	size_t i;
	if(!name){
		return NULL;
	}
	for(i = 0; i < sizeof(phases) / sizeof(phases[0]); ++i){
		const BinarySolutionPhaseDef *P = phases[i];
		if(0 == strcmp(P->name, name) && source_match(P->source, source)){
			return P;
		}
	}
	return NULL;
}

int solution_phase_lookup_member(const char *name, const char *source,
		const BinarySolutionPhaseDef **phase, unsigned *member_index){
	const BinarySolutionPhaseDef *phases[] = {
		bcc_iron_hidayat_phase(),
		fcc_iron_hidayat_phase(),
		wustite_hidayat_phase()
	};
	size_t i;
	if(!name || !phase || !member_index){
		return 0;
	}
	for(i = 0; i < sizeof(phases) / sizeof(phases[0]); ++i){
		const BinarySolutionPhaseDef *P = phases[i];
		if(!source_match(P->source, source)){
			continue;
		}
		if(0 == strcmp(P->member_a_name, name)){
			*phase = P;
			*member_index = 0;
			return 1;
		}
		if(0 == strcmp(P->member_b_name, name)){
			*phase = P;
			*member_index = 1;
			return 1;
		}
	}
	return 0;
}
