#include "fluids.h"
#include "fluids/fluids_list.h"

#include <string.h>

#define X
#define F(NAME) extern const HelmholtzData helmholtz_data_##NAME;
FLUIDS(F,X)
#undef X
#undef F

/* create a list of all the fluids from fluids.h */
static const HelmholtzData *fluids[] ={
#define X ,
#define F(NAME) &helmholtz_data_##NAME
	FLUIDS(F,X)
#undef X
#undef F
};

/* count them */
static int nfluids = 0 +
#define X +
#define F(NAME) 1
	FLUIDS(F,X);
#undef F
#undef X

const HelmholtzData *fprops_fluid(const char *name){
	int i;
	for(i = 0; i < nfluids; ++i){
		if(0==strcmp(name, fluids[i]->name)){
			return fluids[i];
		}
	}
	return NULL;
}
