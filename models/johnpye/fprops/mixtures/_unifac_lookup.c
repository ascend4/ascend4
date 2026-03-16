#include <string.h>
#include "unifac_data.h"

extern const FpropsUNIFACSubgroupSource fprops_unifac_orig_2003_subgroups[];
extern const FpropsUNIFACComponentSource fprops_unifac_orig_2003_components[];
extern const FpropsUNIFACInteractionSource fprops_unifac_orig_2003_interactions;

const FpropsUNIFACSourceData fprops_unifac_orig_2003 = {
	"UNIFAC-orig-2003",
	fprops_unifac_orig_2003_subgroups,
	90,
	fprops_unifac_orig_2003_components,
	35,
	&fprops_unifac_orig_2003_interactions,
	NULL,
	0
};

const FpropsUNIFACSourceData *fprops_unifac_source(const char *name){
	if(!name || strcmp(name, "UNIFAC-orig-2003") == 0 || strcmp(name, "orig-2003") == 0){
		return &fprops_unifac_orig_2003;
	}
	return NULL;
}

const FpropsUNIFACComponentSource *fprops_unifac_component(
		const FpropsUNIFACSourceData *src, const char *name){
	int i;
	if(!src || !name){
		return NULL;
	}
	for(i = 0; i < src->ncomponents; ++i){
		if(strcmp(src->components[i].name, name) == 0){
			return &src->components[i];
		}
	}
	return NULL;
}

const FpropsUNIFACSubgroupSource *fprops_unifac_subgroup(
		const FpropsUNIFACSourceData *src, const char *name){
	int i;
	if(!src || !name){
		return NULL;
	}
	for(i = 0; i < src->nsubgroups; ++i){
		if(strcmp(src->subgroups[i].name, name) == 0){
			return &src->subgroups[i];
		}
	}
	return NULL;
}
