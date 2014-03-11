#include "fluids.h"
#include "fluids/fluids_list.h"
#include "fprops.h"
#include "helmholtz.h"
#include "pengrob.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#define FLUIDS_DEBUG
#define FLUIDS_ERRORS

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

/* TODO centralise declaration of our error-reporting function somehow...? */
#ifdef FLUIDS_ERRORS
# include "color.h"
# define ERRMSG(STR,...) \
	color_on(stderr,ASC_FG_BRIGHTRED);\
	fprintf(stderr,"ERROR:");\
	color_off(stderr);\
	fprintf(stderr," %s:%d:" STR "\n", __func__, __LINE__ ,##__VA_ARGS__)
#else
# define ERRMSG(ARGS...) ((void)0)
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

const PureFluid *fprops_fluid(const char *name, const char *corrtype, const char *source){
	int i;
	MSG("Looking for fluid '%s' of type '%s', with source text '%s'",name,corrtype,source);
	for(i = 0; i < nfluids; ++i){
		if(0==strcmp(name, fluids[i]->name)){
			MSG("Got '%s' (type %d, source '%s')",name,fluids[i]->type,fluids[i]->source);
			if(source){
				if(fluids[i]->source && NULL != strstr(fluids[i]->source, source)){
					MSG("Source '%s' OK",source);
				}else{
					MSG("Source '%s' not matched",source);
					continue;
				}
			}
			if(fprops_corr_avail(fluids[i],corrtype)){
				MSG("Match! %d",i);
				return fprops_prepare(fluids[i],corrtype);
			}else{
				MSG("No match");
			}
		}
	}
	ERRMSG("No fluid found matching name '%s', type '%s' and source '%s'",name,corrtype,source);
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

void fprops_fluid_destroy(PureFluid *P){
	MSG("Freeing data for lfuid '%s'",P->name);
	switch(P->type){
	case FPROPS_HELMHOLTZ:
		helmholtz_destroy(P);
		break;
	case FPROPS_PENGROB:
		pengrob_destroy(P);
		break;
	case FPROPS_CUBIC:
		assert(FPROPS_CUBIC != P->type);
		break;
	case FPROPS_IDEAL:
	case FPROPS_REDKW:
	case FPROPS_SOAVE:
	case FPROPS_MBWR:
		ERRMSG("Not implemented: fluid '%s' of type %d to be destroyed",P->name,P->type);
		break;
	}
}

