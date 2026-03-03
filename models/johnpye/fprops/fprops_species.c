#include "fprops_species.h"

#include <string.h>
#include <stdlib.h>

#include "fluids.h"
#include "fprops.h"
#include "thermo_pure.h"
#include "constcp_data.h"

static int modeltype_is(const char *modeltype, const char *tag){
	return modeltype && 0 == strcmp(modeltype, tag);
}

static FpropsSpecies *fprops_species_create_base(const char *name, const char *source,
		const char *modeltype, const char *corrtype){
	FpropsSpecies *S = (FpropsSpecies *)calloc(1, sizeof(FpropsSpecies));
	if(!S){
		return NULL;
	}
	S->name = name;
	S->source = source;
	S->modeltype = modeltype;
	S->corrtype = corrtype;
	return S;
}

const char *fprops_species_kind_name(FpropsSpeciesKind kind){
	switch(kind){
	case FPROPS_SPECIES_EOS:
		return "eos";
	case FPROPS_SPECIES_CONSTCP:
		return "constcp";
	case FPROPS_SPECIES_DERIVED_CONSTCP:
		return "derived_constcp";
	default:
		return "unknown";
	}
}

FpropsSpecies *fprops_species_lookup(const char *name, const char *source,
		const char *modeltype, const char *corrtype, FpropsError *err){
	const char *mt = modeltype ? modeltype : "auto";

	if(!err){
		return NULL;
	}
	*err = FPROPS_NO_ERROR;
	if(!name){
		*err = FPROPS_INVALID_REQUEST;
		return NULL;
	}

	if(modeltype_is(mt, "auto") || modeltype_is(mt, "constcp")){
		const ConstCpSpecies *C = constcp_data_lookup(name, source);
		if(C){
			FpropsSpecies *S = fprops_species_create_base(name, source, mt, NULL);
			if(!S){
				*err = FPROPS_NUMERIC_ERROR;
				return NULL;
			}
			S->kind = FPROPS_SPECIES_CONSTCP;
			S->constcp = C;
			S->model = thermo_model_constcp_species();
			S->model_data = (void *)C;
			return S;
		}
		if(modeltype_is(mt, "constcp")){
			*err = FPROPS_DATA_ERROR;
			return NULL;
		}
	}

	if(modeltype_is(mt, "derived_constcp")){
		*err = FPROPS_NOT_IMPLEMENTED;
		return NULL;
	}

	{
		const char *corr = corrtype;
		const EosData *E;
		PureFluid *P;
		if(modeltype_is(mt, "helmholtz") || modeltype_is(mt, "pengrob")
				|| modeltype_is(mt, "ideal")){
			corr = mt;
		}else if(modeltype_is(mt, "eos")){
			/* keep corr as passed, or NULL to allow auto selection */
		}else if(!modeltype_is(mt, "auto")){
			*err = FPROPS_INVALID_REQUEST;
			return NULL;
		}

		E = fprops_eos(name, corr, source);
		if(!E){
			*err = FPROPS_DATA_ERROR;
			return NULL;
		}
		P = fprops_prepare(E, corr);
		if(!P){
			*err = FPROPS_DATA_ERROR;
			return NULL;
		}
		{
			const char *resolved_corr = corr ? corr : fprops_corr_type(P->type);
			FpropsSpecies *S = fprops_species_create_base(name, source, mt, resolved_corr);
			if(!S){
				fprops_fluid_destroy(P);
				*err = FPROPS_NUMERIC_ERROR;
				return NULL;
			}
			S->kind = FPROPS_SPECIES_EOS;
			S->model = thermo_model_pure();
			S->model_data = P;
			S->eos = E;
			return S;
		}
	}
}

void fprops_species_destroy(FpropsSpecies *S){
	if(!S){
		return;
	}
	if(S->kind == FPROPS_SPECIES_EOS && S->model_data){
		fprops_fluid_destroy((PureFluid *)S->model_data);
	}
	free(S);
}
