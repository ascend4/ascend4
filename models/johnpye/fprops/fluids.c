#include "fluids.h"
#include "fluids/fluids_list.h"
#include "fprops.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
static const EosData *fluids[] ={ FLUIDS(F,X)
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

static int fluids_sorted = 0;

static int cmp_fluid_dataquality(const void *A, const void *B){
	/* sort in order of decreasing quality -- ie best first */
	if(((EosData *)A)->quality > ((EosData *)B)->quality)return -1;
	if(((EosData *)A)->quality < ((EosData *)B)->quality)return 1;
	return 0;
}

static void sort_fluids(){
	if(!fluids_sorted){
		qsort(fluids,nfluids,sizeof(EosData *),&cmp_fluid_dataquality);
		fluids_sorted = 1;
	}
}

const PureFluid *fprops_fluid(const char *name, const char *corrtype, const char *source){
	int i;
	sort_fluids();
	for(i=0; i < nfluids; ++i){
		if(0==strcmp(name, fluids[i]->name)){
			if(
				fprops_corr_avail(fluids[i],corrtype) 
				&& fprops_source_match(fluids[i],source)
			)return fprops_prepare(fluids[i],corrtype);
		}
	}
	return NULL;
}


int fprops_num_fluids(){
	return nfluids;
}

const PureFluid *fprops_get_fluid(int i){
	sort_fluids();
	if(i >= 0 && i < nfluids){
		PureFluid *P = fprops_prepare(fluids[i],NULL);
		MSG("Fluid '%s' with T_t = %f",P->name,P->data->T_t);
		return P;
	}
	return NULL;
}



