#include "fluids.h"
#include "fluids/fluids_list.h"
#include "fprops.h"

#include <string.h>
#include <stdio.h>

//#define FLUIDS_DEBUG
#ifdef FLUIDS_DEBUG
# include "color.h"
# define MSG(FMT, ...) \
	color_on(stderr,ASC_FG_BRIGHTRED);\
	fprintf(stderr,"%s:%d: ",__FILE__,__LINE__);\
	color_on(stderr,ASC_FG_BRIGHTBLUE);\
	fprintf(stderr,"%s: ",__func__);\
	color_off(stderr);\
	fprintf(stderr,FMT "\n",##__VA_ARGS__)
#else
# define MSG(ARGS...) ((void)0)
#endif

/* declare the external EosData objects */
#define X
#define F(NAME) extern const EosData eos_##NAME;
FLUIDS(F,X)
#undef F
#define F(NAME) extern const EosData eos_rpp_##NAME;
RPPFLUIDS(F,X)
#undef F
#undef X

/* create a list of all the fluids from fluids/fluids_list.h */
#define X ,
#define F(NAME) &eos_##NAME
const EosData *fluids[] ={ FLUIDS(F,X)
#undef F
#define F(NAME) &eos_rpp_##NAME
	, RPPFLUIDS(F,X)
};
#undef X
#undef F

/* count them */
#define X +
#define F(NAME) 1
static int nfluids = 0 + FLUIDS(F,X) + RPPFLUIDS(F,X);
#undef F
#undef X

const PureFluid *fprops_fluid(const char *name, const char *corrtype){
	int i;
	MSG("Looking for fluid '%s' of type '%s'",name,corrtype);
	for(i = 0; i < nfluids; ++i){
		if(0==strcmp(name, fluids[i]->name)){
			MSG("Got '%s' (type %d)",name,fluids[i]->type);
			if(fprops_corr_avail(fluids[i],corrtype)){
				MSG("Match!");
				return fprops_prepare(fluids[i],corrtype);
			}else{
				MSG("No match");
			}
		}
	}
	MSG("Returning null");
	return NULL;
}


int fprops_num_fluids(){
	return nfluids;
}

const PureFluid *fprops_get_fluid(int i){
	if(i >= 0 && i < nfluids){
		PureFluid *P = fprops_prepare(fluids[i],NULL);
		MSG("Fluid '%s' with T_t = %f",P->name,P->data->T_t);
		return P;
	}
	return NULL;
}



