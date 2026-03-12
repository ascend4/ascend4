#include "fluids.h"
#include "fluids/fluids_list.h"
#include "fprops.h"
#include "helmholtz.h"
#include "ideal.h"
#include "incomp.h"
#include "pengrob.h"
#include "gibbs_species.h"
#include "constcp_data.h"
#include "shomate_data.h"
#include "solution_data.h"
#include "spinel_data.h"

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

static int source_filter_matches(const EosData *E, const char *source){
	if(source == NULL){
		return 1;
	}
	return E->source != NULL && NULL != strstr(E->source, source);
}

static int default_corr_rank(int corr){
	switch(corr){
	case FPROPS_HELMHOLTZ:
		return 4;
	case FPROPS_PENGROB:
		return 3;
	case FPROPS_INCOMP:
		return 2;
	case FPROPS_IDEAL:
		return 1;
	default:
		return 0;
	}
}

static const EosData *find_first_matching_eos(const char *name, const char *corrtype, const char *source){
	int i;
	for(i = 0; i < nfluids; ++i){
		if(0 != strcmp(name, fluids[i]->name)){
			continue;
		}
		MSG("Got '%s' (type %d, source '%s')", name, fluids[i]->type, fluids[i]->source);
		if(!source_filter_matches(fluids[i], source)){
			MSG("Source '%s' not matched", source);
			continue;
		}
		if(fprops_corr_avail(fluids[i], corrtype)){
			MSG("Match! %d", i);
			return fluids[i];
		}
		MSG("No match");
	}
	return NULL;
}

static const EosData *find_best_default_eos(const char *name, const char *source){
	int i;
	int best_rank = 0;
	const EosData *best = NULL;
	for(i = 0; i < nfluids; ++i){
		int corr;
		int rank;
		if(0 != strcmp(name, fluids[i]->name)){
			continue;
		}
		MSG("Got '%s' (type %d, source '%s')", name, fluids[i]->type, fluids[i]->source);
		if(!source_filter_matches(fluids[i], source)){
			MSG("Source '%s' not matched", source);
			continue;
		}
		corr = fprops_corr_avail(fluids[i], NULL);
		rank = default_corr_rank(corr);
		if(rank > best_rank){
			best_rank = rank;
			best = fluids[i];
		}
	}
	return best;
}

const PureFluid *fprops_fluid(const char *name, const char *corrtype, const char *source){
	const EosData *E;
	MSG("Looking for fluid '%s' of type '%s', with source text '%s'",name,corrtype,source);
	if(corrtype == NULL){
		E = find_best_default_eos(name, source);
	}else{
		E = find_first_matching_eos(name, corrtype, source);
	}
	if(E != NULL){
		return fprops_prepare(E, corrtype);
	}
	ERRMSG("No fluid found matching name '%s', type '%s' and source '%s'",name,corrtype,source);
	return NULL;
}

const EosData *fprops_eos(const char *name, const char *corrtype, const char *source){
	const EosData *E;
	MSG("Looking for EOS '%s' of type '%s', with source text '%s'",name,corrtype,source);
	if(corrtype == NULL){
		E = find_best_default_eos(name, source);
	}else{
		E = find_first_matching_eos(name, corrtype, source);
	}
	if(E != NULL){
		return E;
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
		const GibbsSpecies *G = gibbs_species_lookup(names[i], source_i);
		const BinarySolutionPhaseDef *phase = NULL;
		unsigned member_index = 0;
		if(!G){
			G = gibbs_species_lookup(names[i], NULL);
		}
		if(G){
			unsigned k2;
			if(!G->elements || !G->stoich || G->nelem == 0){
				ERRMSG("Missing Gibbs-species element composition for '%s'", names[i]);
				return 0;
			}
			for(k2 = 0; k2 < G->nelem; ++k2){
				for(e = 0; e < ne; ++e){
					if(0 == strcmp(G->elements[k2], elements[e])){
						A_out[e * ns + i] += G->stoich[k2];
					}
				}
			}
			continue;
		}
		if(solution_phase_lookup_member(names[i], source_i, &phase, &member_index)
				|| solution_phase_lookup_member(names[i], NULL, &phase, &member_index)){
			unsigned k2;
			const char **phase_elements = (member_index == 0) ? phase->elements_a : phase->elements_b;
			const double *phase_stoich = (member_index == 0) ? phase->stoich_a : phase->stoich_b;
			unsigned phase_nelem = (member_index == 0) ? phase->nelem_a : phase->nelem_b;
			if(!phase_elements || !phase_stoich || phase_nelem == 0){
				ERRMSG("Missing solution-phase element composition for '%s'", names[i]);
				return 0;
			}
			for(k2 = 0; k2 < phase_nelem; ++k2){
				for(e = 0; e < ne; ++e){
					if(0 == strcmp(phase_elements[k2], elements[e])){
						A_out[e * ns + i] += phase_stoich[k2];
					}
				}
			}
			continue;
		}
		{
			const FeSpinelPhaseDef *spinel = NULL;
			if(spinel_phase_lookup_member(names[i], source_i, &spinel, &member_index)
					|| spinel_phase_lookup_member(names[i], NULL, &spinel, &member_index)){
				unsigned k2;
				const char **phase_elements = spinel_phase_member_elements(spinel, member_index);
				const double *phase_stoich = spinel_phase_member_stoich(spinel, member_index);
				unsigned phase_nelem = spinel_phase_member_nelem(spinel, member_index);
				if((phase_nelem > 0) && (!phase_elements || !phase_stoich)){
					ERRMSG("Missing spinel-phase element composition for '%s'", names[i]);
					return 0;
				}
				for(k2 = 0; k2 < phase_nelem; ++k2){
					for(e = 0; e < ne; ++e){
						if(0 == strcmp(phase_elements[k2], elements[e])){
							A_out[e * ns + i] += phase_stoich[k2];
						}
					}
				}
				continue;
			}
		}
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

static int fprops_add_unique_element(char ***elements, int *ne, int *cap, const char *sym){
	int i;
	char **tmp;
	char *copy;
	int newcap;

	if(!elements || !ne || !cap || !sym || !sym[0]){
		return 0;
	}
	for(i = 0; i < *ne; ++i){
		if(0 == strcmp((*elements)[i], sym)){
			return 1;
		}
	}
	if(*ne >= *cap){
		newcap = (*cap > 0) ? (2 * (*cap)) : 8;
		tmp = (char **)realloc(*elements, (size_t)newcap * sizeof(char *));
		if(!tmp){
			return 0;
		}
		*elements = tmp;
		*cap = newcap;
	}
	copy = strdup(sym);
	if(!copy){
		return 0;
	}
	(*elements)[*ne] = copy;
	++(*ne);
	return 1;
}

void fprops_free_elements(char ***elements, int *ne){
	int i;
	if(!elements || !*elements){
		if(ne){
			*ne = 0;
		}
		return;
	}
	for(i = 0; ne && i < *ne; ++i){
		free((*elements)[i]);
	}
	free(*elements);
	*elements = NULL;
	if(ne){
		*ne = 0;
	}
}

int fprops_collect_elements_source(const char **names, int ns, const char *source,
		char ***elements_out, int *ne_out){
	char **elements = NULL;
	int ne = 0;
	int cap = 0;
	int i;

	if(!names || ns <= 0 || !elements_out || !ne_out){
		return 0;
	}

	for(i = 0; i < ns; ++i){
		char source_buf[512];
		const char *source_i = fprops_resolve_species_source(source, names[i], source_buf,
			(unsigned)sizeof(source_buf));
		const GibbsSpecies *G = gibbs_species_lookup(names[i], source_i);
		const BinarySolutionPhaseDef *phase = NULL;
		unsigned member_index = 0;
		if(!G){
			G = gibbs_species_lookup(names[i], NULL);
		}
		if(G){
			unsigned k2;
			if(!G->elements || !G->stoich || G->nelem == 0){
				ERRMSG("Missing Gibbs-species element composition for '%s'", names[i]);
				fprops_free_elements(&elements, &ne);
				return 0;
			}
			for(k2 = 0; k2 < G->nelem; ++k2){
				if(!fprops_add_unique_element(&elements, &ne, &cap, G->elements[k2])){
					fprops_free_elements(&elements, &ne);
					return 0;
				}
			}
			continue;
		}
		if(solution_phase_lookup_member(names[i], source_i, &phase, &member_index)
				|| solution_phase_lookup_member(names[i], NULL, &phase, &member_index)){
			unsigned k2;
			const char **phase_elements = (member_index == 0) ? phase->elements_a : phase->elements_b;
			unsigned phase_nelem = (member_index == 0) ? phase->nelem_a : phase->nelem_b;
			if(!phase_elements || phase_nelem == 0){
				ERRMSG("Missing solution-phase element composition for '%s'", names[i]);
				fprops_free_elements(&elements, &ne);
				return 0;
			}
			for(k2 = 0; k2 < phase_nelem; ++k2){
				if(!fprops_add_unique_element(&elements, &ne, &cap, phase_elements[k2])){
					fprops_free_elements(&elements, &ne);
					return 0;
				}
			}
			continue;
		}
		{
			const FeSpinelPhaseDef *spinel = NULL;
			if(spinel_phase_lookup_member(names[i], source_i, &spinel, &member_index)
					|| spinel_phase_lookup_member(names[i], NULL, &spinel, &member_index)){
				unsigned k2;
				const char **phase_elements = spinel_phase_member_elements(spinel, member_index);
				unsigned phase_nelem = spinel_phase_member_nelem(spinel, member_index);
				if((phase_nelem > 0) && !phase_elements){
					ERRMSG("Missing spinel-phase element composition for '%s'", names[i]);
					fprops_free_elements(&elements, &ne);
					return 0;
				}
				for(k2 = 0; k2 < phase_nelem; ++k2){
					if(!fprops_add_unique_element(&elements, &ne, &cap, phase_elements[k2])){
						fprops_free_elements(&elements, &ne);
						return 0;
					}
				}
				continue;
			}
		}
		{
			const EosData *E = fprops_eos(names[i], NULL, source_i);
			int k;
			if(!E){
				E = fprops_eos(names[i], NULL, NULL);
			}
			if(!E){
				const ConstCpSpecies *S = constcp_data_lookup(names[i], source_i);
				const ShomateSpecies *Sh = NULL;
				int k;
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
					fprops_free_elements(&elements, &ne);
					return 0;
				}
				if(S){
					if(!S->elements || !S->stoich || S->nelem == 0){
						ERRMSG("Missing element composition for '%s'", S->name);
						fprops_free_elements(&elements, &ne);
						return 0;
					}
					for(k = 0; k < (int)S->nelem; ++k){
						if(!fprops_add_unique_element(&elements, &ne, &cap, S->elements[k])){
							fprops_free_elements(&elements, &ne);
							return 0;
						}
					}
				}else{
					if(!Sh->elements || !Sh->stoich || Sh->nelem == 0){
						ERRMSG("Missing element composition for '%s'", Sh->name);
						fprops_free_elements(&elements, &ne);
						return 0;
					}
					for(k = 0; k < (int)Sh->nelem; ++k){
						if(!fprops_add_unique_element(&elements, &ne, &cap, Sh->elements[k])){
							fprops_free_elements(&elements, &ne);
							return 0;
						}
					}
				}
				continue;
			}
			if(!E->elements || E->nelements <= 0){
				const EosData *Erpp = fprops_eos(names[i], NULL, "RPP");
				if(Erpp && Erpp->elements && Erpp->nelements > 0){
					E = Erpp;
				}
			}
			if(!E->elements || E->nelements <= 0){
				const ConstCpSpecies *S = constcp_data_lookup(names[i], NULL);
				int k;
				if(S && S->elements && S->stoich && S->nelem > 0){
					for(k = 0; k < (int)S->nelem; ++k){
						if(!fprops_add_unique_element(&elements, &ne, &cap, S->elements[k])){
							fprops_free_elements(&elements, &ne);
							return 0;
						}
					}
					continue;
				}
				ERRMSG("Missing element composition for '%s'", E->name);
				fprops_free_elements(&elements, &ne);
				return 0;
			}
			for(k = 0; k < E->nelements; ++k){
				if(!fprops_add_unique_element(&elements, &ne, &cap, E->elements[k].symbol)){
					fprops_free_elements(&elements, &ne);
					return 0;
				}
			}
		}
	}

	if(ne <= 0){
		fprops_free_elements(&elements, &ne);
		return 0;
	}

	*elements_out = elements;
	*ne_out = ne;
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
		ideal_destroy(P);
		break;
	case FPROPS_INCOMP:
		incomp_destroy(P);
		break;
	case FPROPS_REDKW:
	case FPROPS_SOAVE:
	case FPROPS_MBWR:
		ERRMSG("Not implemented: fluid '%s' of type %d to be destroyed",P->name,P->type);
		break;
	}
}
