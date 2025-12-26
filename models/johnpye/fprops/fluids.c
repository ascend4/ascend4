#include "fluids.h"
#include "fluids/fluids_list.h"
#include "fprops.h"
#include "helmholtz.h"
#include "pengrob.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

//#define FLUIDS_DEBUG
#ifdef FLUIDS_DEBUG
# include "color.h"
# define MSG FPROPS_MSG
# define ERRMSG FPROPS_ERRMSG
#else
# define MSG(ARGS...) ((void)0)
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

const EosData *fprops_eos(const char *name, const char *corrtype, const char *source){
	int i;
	MSG("Looking for EOS '%s' of type '%s', with source text '%s'",name,corrtype,source);
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
				return fluids[i];
			}else{
				MSG("No match");
			}
		}
	}
	ERRMSG("No EOS found matching name '%s', type '%s' and source '%s'",name,corrtype,source);
	return NULL;
}

int fprops_build_element_matrix(const char **names, int ns, const char **elements, int ne, double *A_out){
	return fprops_build_element_matrix_source(names, ns, elements, ne, NULL, A_out);
}

int fprops_build_element_matrix_source(const char **names, int ns, const char **elements, int ne,
		const char *source, double *A_out){
	int i;
	int e;
	int k;

	if(!names || !elements || !A_out || ns <= 0 || ne <= 0){
		return 0;
	}

	for(e = 0; e < ne; ++e){
		for(i = 0; i < ns; ++i){
			A_out[e * ns + i] = 0.0;
		}
	}

	for(i = 0; i < ns; ++i){
		const EosData *E = fprops_eos(names[i], NULL, source);
		if(!E){
			ERRMSG("Missing EOS data for '%s'", names[i]);
			return 0;
		}
		if(!E->elements || E->nelements <= 0){
			ERRMSG("Missing element composition for '%s'", E->name);
			return 0;
		}
		for(k = 0; k < E->nelements; ++k){
			for(e = 0; e < ne; ++e){
				if(0 == strcmp(E->elements[k].symbol, elements[e])){
					A_out[e * ns + i] += E->elements[k].count;
				}
			}
		}
	}

	return 1;
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
	case FPROPS_INCOMP:
	case FPROPS_REDKW:
	case FPROPS_SOAVE:
	case FPROPS_MBWR:
		ERRMSG("Not implemented: fluid '%s' of type %d to be destroyed",P->name,P->type);
		break;
	}
}
