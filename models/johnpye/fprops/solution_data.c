#include "solution_data.h"

#include <string.h>

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
	const BinarySolutionPhaseDef *P = wustite_hidayat_phase();
	if(!name){
		return NULL;
	}
	if(0 == strcmp(P->name, name) && source_match(P->source, source)){
		return P;
	}
	return NULL;
}

int solution_phase_lookup_member(const char *name, const char *source,
		const BinarySolutionPhaseDef **phase, unsigned *member_index){
	const BinarySolutionPhaseDef *P = wustite_hidayat_phase();
	if(!name || !phase || !member_index){
		return 0;
	}
	if(!source_match(P->source, source)){
		return 0;
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
	return 0;
}
