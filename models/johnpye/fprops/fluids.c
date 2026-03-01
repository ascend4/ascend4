#include "fluids.h"
#include "fluids/fluids_list.h"
#include "fprops.h"
#include "helmholtz.h"
#include "pengrob.h"
#include "constcp_data.h"
#include "shomate_data.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>

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

static void copy_trimmed_range(const char *a, const char *b, char *out, unsigned out_len){
	const char *s = a;
	const char *e = b;
	unsigned n;
	if(!out || out_len == 0){
		return;
	}
	while(s < e && isspace((unsigned char)*s)){
		++s;
	}
	while(e > s && isspace((unsigned char)*(e - 1))){
		--e;
	}
	n = (unsigned)(e - s);
	if(n >= out_len){
		n = out_len - 1;
	}
	if(n > 0){
		memcpy(out, s, n);
	}
	out[n] = '\0';
}

const char *fprops_resolve_species_source(const char *source_spec, const char *species_name,
		char *out, unsigned out_len){
	const char *p;
	char default_source[256];
	int have_default = 0;
	if(!source_spec || !source_spec[0]){
		return NULL;
	}
	if(!species_name || !species_name[0]){
		return source_spec;
	}
	if(!strchr(source_spec, '=')){
		return source_spec;
	}
	if(!out || out_len == 0){
		return NULL;
	}
	default_source[0] = '\0';
	p = source_spec;
	while(*p){
		const char *q = p;
		const char *eq = NULL;
		char key[128];
		char val[512];
		while(*q && *q != ';'){
			if(!eq && *q == '='){
				eq = q;
			}
			++q;
		}
		if(eq){
			copy_trimmed_range(p, eq, key, (unsigned)sizeof(key));
			copy_trimmed_range(eq + 1, q, val, (unsigned)sizeof(val));
			if(key[0] && val[0]){
				if(0 == strcmp(key, species_name)){
					copy_trimmed_range(val, val + strlen(val), out, out_len);
					return out[0] ? out : NULL;
				}
				if(0 == strcmp(key, "*") || 0 == strcmp(key, "default")){
					copy_trimmed_range(val, val + strlen(val), default_source, (unsigned)sizeof(default_source));
					have_default = default_source[0] ? 1 : 0;
				}
			}
		}
		p = (*q == ';') ? q + 1 : q;
	}
	if(have_default){
		copy_trimmed_range(default_source, default_source + strlen(default_source), out, out_len);
		return out[0] ? out : NULL;
	}
	return NULL;
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
		char source_buf[512];
		const char *source_i = fprops_resolve_species_source(source, names[i], source_buf,
			(unsigned)sizeof(source_buf));
		const EosData *E = fprops_eos(names[i], NULL, source_i);
		if(!E){
			E = fprops_eos(names[i], NULL, NULL);
		}
		if(!E){
			const ConstCpSpecies *S = constcp_data_lookup(names[i], source_i);
			const ShomateSpecies *Sh = NULL;
			if(!S){
				S = constcp_data_lookup(names[i], NULL);
			}
			if(!S){
				Sh = shomate_data_lookup(names[i], source_i);
			}
			if(!Sh){
				Sh = shomate_data_lookup(names[i], NULL);
			}
			if(!S && !Sh){
				ERRMSG("Missing EOS/constcp data for '%s'", names[i]);
				return 0;
			}
			if(S){
				if(!S->elements || !S->stoich || S->nelem == 0){
					ERRMSG("Missing element composition for '%s'", S->name);
					return 0;
				}
				for(k = 0; k < (int)S->nelem; ++k){
					for(e = 0; e < ne; ++e){
						if(0 == strcmp(S->elements[k], elements[e])){
							A_out[e * ns + i] += S->stoich[k];
						}
					}
				}
			}else{
				if(!Sh->elements || !Sh->stoich || Sh->nelem == 0){
					ERRMSG("Missing element composition for '%s'", Sh->name);
					return 0;
				}
				for(k = 0; k < (int)Sh->nelem; ++k){
					for(e = 0; e < ne; ++e){
						if(0 == strcmp(Sh->elements[k], elements[e])){
							A_out[e * ns + i] += Sh->stoich[k];
						}
					}
				}
			}
			continue;
		}
		if(!E->elements || E->nelements <= 0){
			/* Elemental composition is source-independent: if a selected EOS
			   lacks composition metadata, try RPP entry for the same species. */
			const EosData *Erpp = fprops_eos(names[i], NULL, "RPP");
			if(Erpp && Erpp->elements && Erpp->nelements > 0){
				E = Erpp;
			}
		}
		if(!E->elements || E->nelements <= 0){
			const ConstCpSpecies *S = constcp_data_lookup(names[i], NULL);
			if(S && S->elements && S->stoich && S->nelem > 0){
				for(k = 0; k < (int)S->nelem; ++k){
					for(e = 0; e < ne; ++e){
						if(0 == strcmp(S->elements[k], elements[e])){
							A_out[e * ns + i] += S->stoich[k];
						}
					}
				}
				continue;
			}
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

const ConstCpSpecies *fprops_constcp_species(const char *name, const char *source){
	return constcp_data_lookup(name, source);
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
