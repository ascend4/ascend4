#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <string.h>
#include <ctype.h>

#include "fprops.h"
#include "ideal.h"
#include "helmholtz.h"
#include "pengrob.h"
#include "fluids.h"
#include "gibbs_species.h"
#include "constcp_species.h"
#include "shomate_species.h"
#include "solution.h"
#include "eqm.h"
#include "eqm_internal.h"
#include "name_resolve.h"

#ifdef HAVE_IPOPT
#include "eqm_ipopt.h"
#endif
#ifdef HAVE_NLOPT
#include "eqm_slsqp.h"
#endif

#ifdef EQM_DEBUG
# define MSG FPROPS_MSG
# define ERR FPROPS_ERRMSG
#else
# define MSG(ARGS...) ((void)0)
# define ERR(ARGS...) ((void)0)
#endif

double gas_R(void){
	return 8.31446261815324;
}

/* Bound-KKT acceptance settings for boundary-active equilibrium solutions. */
static const double EQM_BOUND_KKT_FREE_TOL = 2e-2;
static const double EQM_BOUND_KKT_DUAL_TOL = 2e-2;
static const double EQM_BOUND_ACTIVE_CUTOFF_FRAC = 1e-22;
static const double EQM_BOUND_ACTIVE_SMALL_FRAC = 2e-10;
static const double EQM_BOUND_COMPLEMENTARITY_TOL = 1e-9;

typedef enum {
	EQM_MODEL_AUTO = 0,
	EQM_MODEL_IDEAL,
	EQM_MODEL_CONSTCP,
	EQM_MODEL_SHOMATE,
	EQM_MODEL_HELMHOLTZ,
	EQM_MODEL_PENGROB
} EqmMuModel;

typedef enum{
	FPROPS_RXN_ENTRY_PURE = 0,
	FPROPS_RXN_ENTRY_BINARY_SOLUTION_MEMBER,
	FPROPS_RXN_ENTRY_SPINEL_MEMBER
} FpropsRxnEntryKind;

typedef enum{
	FPROPS_RXN_COMPILED_NONE = 0,
	FPROPS_RXN_COMPILED_CONSTCP,
	FPROPS_RXN_COMPILED_SHOMATE,
	FPROPS_RXN_COMPILED_GIBBS,
	FPROPS_RXN_COMPILED_FLUID
} FpropsRxnCompiledKind;

typedef struct{
	FpropsRxnCompiledKind mu_kind;
	FpropsRxnCompiledKind h_kind;
	FpropsRxnCompiledKind v_kind;
	EqmMuModel selector_model;
	int use_ref0;
	PureFluid *fluid;
	const ConstCpSpecies *constcp;
	const ShomateSpecies *shomate;
	const GibbsSpecies *gibbs;
} FpropsRxnThermoRef;

typedef struct{
	char *name;
	char *source_resolved;
	FpropsRxnEntryKind entry_kind;
	int phase_id;
	int member_index;
	FpropsRxnThermoRef thermo;
} FpropsRxnSpeciesCache;

struct FpropsRxnPackage_struct{
	int ns;
	char **names;
	char *source;
	char **elements;
	int ne;
	double *A;
	int *is_condensed;
	int *solution_phase_id;
	int *solution_member_index;
	EqmBinaryPhaseMeta *binary_phases;
	int nbinary_phases;
	FpropsRxnSpeciesCache *species;
};

static const FpropsRxnPackage *eqm_current_package = NULL;

static int eqm_parse_selector(const char *spec, EqmMuModel *model_out, int *use_ref0_out,
		const char **source_out);

static const char *eqm_resolve_rxn_name(const char *name, const char *source,
		char *buf, unsigned buflen, const char **resolved_source){
	FpropsResolvedName resolved;
	FpropsNameResolveStatus status;
	unsigned domains = FPROPS_NAME_DOMAIN_PURE_FLUID | FPROPS_NAME_DOMAIN_EQM_SPECIES;
	char source_buf[512];
	EqmMuModel selector_model = EQM_MODEL_AUTO;
	int use_ref0 = 0;
	const char *selector_source = NULL;
	const char *source_i = fprops_resolve_species_source(source, name, source_buf,
		(unsigned)sizeof(source_buf));
	const char *name_source = NULL;
	(void)use_ref0;

	if(resolved_source){
		*resolved_source = source_i;
	}
	if(!name || !name[0] || !buf || buflen == 0){
		return name;
	}
	eqm_parse_selector(source_i, &selector_model, &use_ref0, &selector_source);
	if(selector_source && selector_source[0]){
		name_source = selector_source;
	}
	status = fprops_name_resolve(name, domains, name_source, &resolved);
	if(status != FPROPS_NAME_RESOLVE_OK || !resolved.canonical || !resolved.canonical->canonical){
		return name;
	}
	snprintf(buf, buflen, "%s", resolved.canonical->canonical);
	buf[buflen - 1] = '\0';
	if(resolved_source && (!source_i || !source_i[0]) && resolved.canonical->source && resolved.canonical->source[0]){
		*resolved_source = resolved.canonical->source;
	}
	return buf;
}

static int eqm_active_trace_enabled(void){
	static int enabled = -1;
	if(enabled < 0){
		const char *v = getenv("FPROPS_EQM_ACTIVESET_TRACE");
		enabled = (v && v[0] && strcmp(v, "0") != 0) ? 1 : 0;
	}
	return enabled;
}

static int eqm_alg_trace_enabled(void){
	static int enabled = -1;
	if(enabled < 0){
		const char *v = getenv("FPROPS_EQM_ALG_TRACE");
		enabled = (v && v[0] && strcmp(v, "0") != 0) ? 1 : 0;
	}
	return enabled;
}

static int eqm_package_trace_enabled(void){
	static int enabled = -1;
	if(enabled < 0){
		const char *v = getenv("FPROPS_EQM_PACKAGE_TRACE");
		enabled = (v && v[0] && strcmp(v, "0") != 0) ? 1 : 0;
	}
	return enabled;
}

static int eqm_basis_trace_enabled(void){
	static int enabled = -1;
	if(enabled < 0){
		const char *v = getenv("FPROPS_EQM_BASIS_TRACE");
		enabled = (v && v[0] && strcmp(v, "0") != 0) ? 1 : 0;
	}
	return enabled;
}

static int eqm_reduced_basis_trace_enabled(void){
	static int enabled = -1;
	if(enabled < 0){
		const char *v = getenv("FPROPS_EQM_REDUCED_BASIS_TRACE");
		enabled = (v && v[0] && strcmp(v, "0") != 0) ? 1 : 0;
	}
	return enabled;
}

static void eqm_package_trace(const char *event, const FpropsRxnPackage *pkg,
		const FpropsRxnPackage *other, const char **names, int ns){
	static long seq = 0;
	const char *first = NULL;
	if(!eqm_package_trace_enabled()){
		return;
	}
	if(pkg && pkg->ns > 0 && pkg->species && pkg->species[0].name){
		first = pkg->species[0].name;
	}else if(names && ns > 0 && names[0]){
		first = names[0];
	}else{
		first = "(none)";
	}
	++seq;
	fprintf(stderr,
		"FPROPS_EQM_PACKAGE_TRACE seq=%ld event=%s pkg=%p other=%p current=%p ns=%d ne=%d first=%s\n",
		seq, event ? event : "(null)", (const void *)pkg, (const void *)other,
		(const void *)eqm_current_package, pkg ? pkg->ns : ns, pkg ? pkg->ne : -1, first);
	fflush(stderr);
}

static void eqm_basis_trace_dump(const char *tag, const char **names, int ns,
		const char **elements, int ne, const double *A, const double *b,
		const double *n_in, const double *n_init, const char *source,
		const char *algorithm, double T, double P, const void *pkg){
	int i, e;
	if(!eqm_basis_trace_enabled()){
		return;
	}
	fprintf(stderr,
		"FPROPS_EQM_BASIS_TRACE tag=%s pkg=%p current=%p ns=%d ne=%d T=%.17g P=%.17g source=%s algorithm=%s\n",
		tag ? tag : "(null)", pkg, (const void *)eqm_current_package, ns, ne, T, P,
		source ? source : "(null)", algorithm ? algorithm : "(null)");
	if(elements){
		fprintf(stderr, "FPROPS_EQM_BASIS_TRACE elements=");
		for(e = 0; e < ne; ++e){
			fprintf(stderr, "%s%s", e ? "," : "", elements[e] ? elements[e] : "(null)");
		}
		fputc('\n', stderr);
	}
	for(i = 0; i < ns; ++i){
		fprintf(stderr, "FPROPS_EQM_BASIS_TRACE species[%d]=%s", i,
			(names && names[i]) ? names[i] : "(null)");
		if(n_in){
			fprintf(stderr, " n_in=%.17g", n_in[i]);
		}
		if(n_init){
			fprintf(stderr, " n_init=%.17g", n_init[i]);
		}
		if(A && ne > 0){
			fprintf(stderr, " A=[");
			for(e = 0; e < ne; ++e){
				fprintf(stderr, "%s%.17g", e ? "," : "", A[e * ns + i]);
			}
			fputc(']', stderr);
		}
		fputc('\n', stderr);
	}
	if(b){
		fprintf(stderr, "FPROPS_EQM_BASIS_TRACE b=[");
		for(e = 0; e < ne; ++e){
			fprintf(stderr, "%s%.17g", e ? "," : "", b[e]);
		}
		fprintf(stderr, "]\n");
	}
	fflush(stderr);
}

static void eqm_reduced_basis_trace_dump(const char *tag, const char **names, int ns, int ne,
		const double *A, const double *b, const int *pivots, int rank, const double *n0,
		const double *N, int r, const double *n_target, const double *z, const double *n,
		double T, double P){
	if(!eqm_reduced_basis_trace_enabled()){
		return;
	}
	fprintf(stderr,
		"FPROPS_EQM_REDUCED_BASIS_TRACE tag=%s T=%.17g P=%.17g ns=%d ne=%d rank=%d r=%d\n",
		tag ? tag : "(null)", T, P, ns, ne, rank, r);
	if(pivots){
		fprintf(stderr, "FPROPS_EQM_REDUCED_BASIS_TRACE pivots=[");
		for(int i = 0; i < rank; ++i){
			fprintf(stderr, "%s%d", i ? "," : "", pivots[i]);
		}
		fprintf(stderr, "]\n");
	}
	if(b){
		fprintf(stderr, "FPROPS_EQM_REDUCED_BASIS_TRACE b=[");
		for(int e = 0; e < ne; ++e){
			fprintf(stderr, "%s%.17g", e ? "," : "", b[e]);
		}
		fprintf(stderr, "]\n");
	}
	if(z){
		fprintf(stderr, "FPROPS_EQM_REDUCED_BASIS_TRACE z=[");
		for(int j = 0; j < r; ++j){
			fprintf(stderr, "%s%.17g", j ? "," : "", z[j]);
		}
		fprintf(stderr, "]\n");
	}
	for(int i = 0; i < ns; ++i){
		fprintf(stderr, "FPROPS_EQM_REDUCED_BASIS_TRACE species[%d]=%s",
			i, (names && names[i]) ? names[i] : "(null)");
		if(n0){
			fprintf(stderr, " n0=%.17g", n0[i]);
		}
		if(n_target){
			fprintf(stderr, " n_target=%.17g", n_target[i]);
		}
		if(n){
			fprintf(stderr, " n=%.17g", n[i]);
		}
		if(A){
			fprintf(stderr, " A=[");
			for(int e = 0; e < ne; ++e){
				fprintf(stderr, "%s%.17g", e ? "," : "", A[e * ns + i]);
			}
			fputc(']', stderr);
		}
		if(N){
			fprintf(stderr, " N=[");
			for(int j = 0; j < r; ++j){
				fprintf(stderr, "%s%.17g", j ? "," : "", N[i * r + j]);
			}
			fputc(']', stderr);
		}
		fputc('\n', stderr);
	}
	fflush(stderr);
}

static int eqm_mu0_constcp_source(const char *name, const char *source, double T, double P0,
		double *mu0);
static int eqm_mu0_shomate_source(const char *name, const char *source, double T, double P0,
		double *mu0);
static int eqm_mu0_gibbs_species_source(const char *name, const char *source, double T, double P0,
		double *mu0);
static int eqm_mu0_model_source(const char *name, EqmMuModel model, const char *source, double T,
		double P0, int use_ref0, double *mu0);
static int eqm_parse_selector(const char *spec, EqmMuModel *model_out, int *use_ref0_out,
		const char **source_out);
int eqm_mu0_source(const char *name, const char *source, double T, double P0, double *mu0);
static int eqm_h_constcp_source(const char *name, const char *source, double T, double P,
		double *h);
static int eqm_h_shomate_source(const char *name, const char *source, double T, double P,
		double *h);
static int eqm_h_fluid_model_source(const char *name, const char *corrtype, const char *source,
		double T, double P, int use_ref0, double *h);
static int eqm_h_model_source(const char *name, EqmMuModel model, const char *source, double T,
		double P, int use_ref0, double *h);
static int eqm_h_source(const char *name, const char *source, double T, double P, double *h);
static PureFluid *eqm_prepare_fluid_for_mu0(const EosData *E, const char *corrtype, int use_ref0);
static int eqm_fluid_state_from_pT(const PureFluid *F, double T, double P, FluidState2 *S_out);
static int eqm_validate_solution_bounds(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, const double *n_out);
static int eqm_dense_solve(double *A, double *b, int n);
void fprops_rxn_package_free(FpropsRxnPackage *pkg);

static void eqm_sort_columns_by_target(const double *n_target, int ns, int *perm, int *inv_perm){
	for(int i = 0; i < ns; ++i){
		perm[i] = i;
	}
	for(int i = 1; i < ns; ++i){
		int key = perm[i];
		double keyv = n_target ? n_target[key] : 0.0;
		int j = i - 1;
		while(j >= 0){
			double curv = n_target ? n_target[perm[j]] : 0.0;
			if(curv > keyv || (curv == keyv && perm[j] < key)){
				break;
			}
			perm[j + 1] = perm[j];
			--j;
		}
		perm[j + 1] = key;
	}
	if(inv_perm){
		for(int i = 0; i < ns; ++i){
			inv_perm[perm[i]] = i;
		}
	}
}

static int eqm_solve_particular_from_pivots(const double *A, const double *b, int ne, int ns,
		const int *pivots, int rank, double *n0_out){
	double *B = NULL;
	double *x = NULL;
	int ok = 0;

	if(!A || !b || !pivots || !n0_out || ne <= 0 || ns <= 0 || rank != ne){
		return 0;
	}
	B = (double *)calloc((size_t)(ne * ne), sizeof(double));
	x = (double *)calloc((size_t)ne, sizeof(double));
	if(!B || !x){
		goto cleanup;
	}
	for(int e = 0; e < ne; ++e){
		x[e] = b[e];
		for(int j = 0; j < ne; ++j){
			B[e * ne + j] = A[e * ns + pivots[j]];
		}
	}
	if(!eqm_dense_solve(B, x, ne)){
		goto cleanup;
	}
	for(int i = 0; i < ns; ++i){
		n0_out[i] = 0.0;
	}
	for(int j = 0; j < ne; ++j){
		n0_out[pivots[j]] = x[j];
	}
	ok = 1;

cleanup:
	free(x);
	free(B);
	return ok;
}

static int eqm_lookup_solution_member(const char *name, const char *source,
		const BinarySolutionPhaseDef **phase_out, unsigned *member_index_out){
	char source_buf[512];
	const char *source_i;
	EqmMuModel selector_model = EQM_MODEL_AUTO;
	int use_ref0 = 0;
	const char *selector_source = NULL;
	(void)selector_model;
	if(!name){
		return 0;
	}
	source_i = fprops_resolve_species_source(source, name, source_buf, (unsigned)sizeof(source_buf));
	eqm_parse_selector(source_i, &selector_model, &use_ref0, &selector_source);
	if(solution_phase_lookup_member(name, selector_source ? selector_source : source_i,
			phase_out, member_index_out)){
		return 1;
	}
	return solution_phase_lookup_member(name, NULL, phase_out, member_index_out);
}

static int eqm_lookup_spinel_member(const char *name, const char *source,
		const FeSpinelPhaseDef **phase_out, unsigned *member_index_out){
	char source_buf[512];
	const char *source_i;
	EqmMuModel selector_model = EQM_MODEL_AUTO;
	int use_ref0 = 0;
	const char *selector_source = NULL;
	(void)selector_model;
	if(!name){
		return 0;
	}
	source_i = fprops_resolve_species_source(source, name, source_buf, (unsigned)sizeof(source_buf));
	eqm_parse_selector(source_i, &selector_model, &use_ref0, &selector_source);
	if(spinel_phase_lookup_member(name, selector_source ? selector_source : source_i,
			phase_out, member_index_out)){
		return 1;
	}
	return spinel_phase_lookup_member(name, NULL, phase_out, member_index_out);
}

static int eqm_has_explicit_source(const char *source){
	return (source && source[0]) ? 1 : 0;
}

static const FpropsRxnPackage *eqm_package_scope_push(const FpropsRxnPackage *pkg){
	const FpropsRxnPackage *old = eqm_current_package;
	eqm_package_trace("scope_push", pkg, old, NULL, 0);
	eqm_current_package = pkg;
	return old;
}

static void eqm_package_scope_pop(const FpropsRxnPackage *old){
	eqm_package_trace("scope_pop", eqm_current_package, old, NULL, 0);
	eqm_current_package = old;
}

static int eqm_pkg_find_species(const FpropsRxnPackage *pkg, const char *name){
	int i;
	if(!pkg || !name){
		return -1;
	}
	for(i = 0; i < pkg->ns; ++i){
		if(pkg->species[i].name && 0 == strcmp(pkg->species[i].name, name)){
			return i;
		}
	}
	return -1;
}

static int eqm_pkg_matches_basis_exact(const FpropsRxnPackage *pkg, const char **names, int ns){
	int i;
	if(!pkg || !names || ns != pkg->ns){
		return 0;
	}
	for(i = 0; i < ns; ++i){
		if(!names[i] || !pkg->species[i].name || 0 != strcmp(names[i], pkg->species[i].name)){
			return 0;
		}
	}
	return 1;
}

static char *eqm_strdup_local(const char *s){
	size_t n;
	char *out;
	if(!s){
		return NULL;
	}
	n = strlen(s);
	out = (char *)malloc(n + 1);
	if(!out){
		return NULL;
	}
	memcpy(out, s, n + 1);
	return out;
}

static PureFluid *eqm_prepare_fluid_cached(const char *name, const char *corrtype,
		const char *source, int use_ref0){
	const char *cands[3];
	int ncands = 0;
	int c;
	int explicit_source;

	if(!name || !corrtype){
		return NULL;
	}
	explicit_source = eqm_has_explicit_source(source);
	cands[ncands++] = source;
	if(!explicit_source){
		cands[ncands++] = NULL;
	}
	if(!explicit_source && strcmp(corrtype, "ideal") == 0){
		cands[ncands++] = "RPP";
	}
	for(c = 0; c < ncands; ++c){
		const EosData *E = fprops_eos(name, corrtype, cands[c]);
		PureFluid *P = eqm_prepare_fluid_for_mu0(E, corrtype, use_ref0);
		if(P){
			return P;
		}
	}
	return NULL;
}

static int eqm_species_compile_thermo(const char *name, const char *source_resolved,
		EqmMuModel selector_model, int use_ref0, FpropsRxnThermoRef *thermo){
	int allow_unsourced_fallback;
	if(!name || !thermo){
		ERR("eqm species compile thermo: invalid args name=%p thermo=%p", (void *)name, (void *)thermo);
		return 0;
	}
	memset(thermo, 0, sizeof(*thermo));
	thermo->selector_model = selector_model;
	thermo->use_ref0 = use_ref0;
	allow_unsourced_fallback = !eqm_has_explicit_source(source_resolved);

	switch(selector_model){
	case EQM_MODEL_AUTO:
		thermo->fluid = eqm_prepare_fluid_cached(name, "ideal", source_resolved, 1);
		if(thermo->fluid){
			thermo->mu_kind = FPROPS_RXN_COMPILED_FLUID;
			thermo->h_kind = FPROPS_RXN_COMPILED_FLUID;
			thermo->v_kind = FPROPS_RXN_COMPILED_FLUID;
			return 1;
		}
		thermo->gibbs = gibbs_species_lookup(name, source_resolved);
		if(!thermo->gibbs && allow_unsourced_fallback){
			thermo->gibbs = gibbs_species_lookup(name, NULL);
		}
		if(thermo->gibbs){
			thermo->mu_kind = FPROPS_RXN_COMPILED_GIBBS;
		}
		thermo->shomate = shomate_species_lookup(name, source_resolved);
		if(!thermo->shomate && allow_unsourced_fallback){
			thermo->shomate = shomate_species_lookup(name, NULL);
		}
		if(thermo->shomate){
			if(thermo->mu_kind == FPROPS_RXN_COMPILED_NONE){
				thermo->mu_kind = FPROPS_RXN_COMPILED_SHOMATE;
			}
			thermo->h_kind = FPROPS_RXN_COMPILED_SHOMATE;
			thermo->v_kind = FPROPS_RXN_COMPILED_SHOMATE;
			return thermo->mu_kind != FPROPS_RXN_COMPILED_NONE;
		}
		thermo->constcp = constcp_species_lookup(name, source_resolved);
		if(!thermo->constcp && allow_unsourced_fallback){
			thermo->constcp = constcp_species_lookup(name, NULL);
		}
		if(thermo->constcp){
			if(thermo->mu_kind == FPROPS_RXN_COMPILED_NONE){
				thermo->mu_kind = FPROPS_RXN_COMPILED_CONSTCP;
			}
			thermo->h_kind = FPROPS_RXN_COMPILED_CONSTCP;
			thermo->v_kind = FPROPS_RXN_COMPILED_CONSTCP;
		}
		if(thermo->mu_kind == FPROPS_RXN_COMPILED_GIBBS
				&& thermo->h_kind == FPROPS_RXN_COMPILED_NONE){
			thermo->h_kind = FPROPS_RXN_COMPILED_GIBBS;
		}
		return thermo->mu_kind != FPROPS_RXN_COMPILED_NONE;
	case EQM_MODEL_IDEAL:
		thermo->fluid = eqm_prepare_fluid_cached(name, "ideal", source_resolved, 1);
		if(!thermo->fluid){
			ERR("eqm species compile thermo: no ideal fluid handle for '%s' (source='%s')",
				name, source_resolved ? source_resolved : "");
			return 0;
		}
		thermo->mu_kind = FPROPS_RXN_COMPILED_FLUID;
		thermo->h_kind = FPROPS_RXN_COMPILED_FLUID;
		return 1;
	case EQM_MODEL_CONSTCP:
		thermo->constcp = constcp_species_lookup(name, source_resolved);
		if(!thermo->constcp){
			thermo->constcp = constcp_species_lookup(name, NULL);
		}
		if(!thermo->constcp){
			ERR("eqm species compile thermo: no constcp species for '%s' (source='%s')",
				name, source_resolved ? source_resolved : "");
			return 0;
		}
		thermo->mu_kind = FPROPS_RXN_COMPILED_CONSTCP;
		thermo->h_kind = FPROPS_RXN_COMPILED_CONSTCP;
		thermo->v_kind = FPROPS_RXN_COMPILED_CONSTCP;
		return 1;
	case EQM_MODEL_SHOMATE:
		thermo->shomate = shomate_species_lookup(name, source_resolved);
		if(!thermo->shomate){
			thermo->shomate = shomate_species_lookup(name, NULL);
		}
		if(!thermo->shomate){
			ERR("eqm species compile thermo: no shomate species for '%s' (source='%s')",
				name, source_resolved ? source_resolved : "");
			return 0;
		}
		thermo->mu_kind = FPROPS_RXN_COMPILED_SHOMATE;
		thermo->h_kind = FPROPS_RXN_COMPILED_SHOMATE;
		thermo->v_kind = FPROPS_RXN_COMPILED_SHOMATE;
		return 1;
	case EQM_MODEL_HELMHOLTZ:
		thermo->fluid = eqm_prepare_fluid_cached(name, "helmholtz", source_resolved, use_ref0);
		if(!thermo->fluid){
			ERR("eqm species compile thermo: no helmholtz fluid handle for '%s' (source='%s')",
				name, source_resolved ? source_resolved : "");
			return 0;
		}
		thermo->mu_kind = FPROPS_RXN_COMPILED_FLUID;
		thermo->h_kind = FPROPS_RXN_COMPILED_FLUID;
		thermo->v_kind = FPROPS_RXN_COMPILED_FLUID;
		return 1;
	case EQM_MODEL_PENGROB:
		thermo->fluid = eqm_prepare_fluid_cached(name, "pengrob", source_resolved, use_ref0);
		if(!thermo->fluid){
			ERR("eqm species compile thermo: no pengrob fluid handle for '%s' (source='%s')",
				name, source_resolved ? source_resolved : "");
			return 0;
		}
		thermo->mu_kind = FPROPS_RXN_COMPILED_FLUID;
		thermo->h_kind = FPROPS_RXN_COMPILED_FLUID;
		thermo->v_kind = FPROPS_RXN_COMPILED_FLUID;
		return 1;
	}
	ERR("eqm species compile thermo: no compiled thermo path for '%s' (model=%d, source='%s')",
		name, (int)selector_model, source_resolved ? source_resolved : "");
	return 0;
}

static void eqm_species_free_thermo(FpropsRxnThermoRef *thermo){
	if(!thermo){
		return;
	}
	if(thermo->fluid){
		fprops_fluid_destroy(thermo->fluid);
	}
	memset(thermo, 0, sizeof(*thermo));
}

static int eqm_mu0_from_compiled(const FpropsRxnSpeciesCache *spec, double T, double P0, double *mu0){
	FpropsError err = FPROPS_NO_ERROR;
	if(!spec || !mu0 || !(T > 0.0) || !(P0 > 0.0)){
		return 0;
	}
	switch(spec->thermo.mu_kind){
	case FPROPS_RXN_COMPILED_GIBBS:
		if(!spec->thermo.gibbs){
			return 0;
		}
		return gibbs_species_g_molar(spec->thermo.gibbs, T, 1e5, mu0);
	case FPROPS_RXN_COMPILED_SHOMATE:
		if(!spec->thermo.shomate){
			return 0;
		}
		*mu0 = shomate_species_g_molar(spec->thermo.shomate, T, P0, &err);
		return (!err && isfinite(*mu0)) ? 1 : 0;
	case FPROPS_RXN_COMPILED_CONSTCP:
		if(!spec->thermo.constcp){
			return 0;
		}
		*mu0 = constcp_species_g_molar(spec->thermo.constcp, T, P0, NULL, &err);
		return (!err && isfinite(*mu0)) ? 1 : 0;
	case FPROPS_RXN_COMPILED_FLUID:
		if(spec->thermo.fluid && spec->thermo.fluid->g_fn && spec->thermo.fluid->data){
			FluidState2 S;
			double g_mass;
			double molar_mass = spec->thermo.fluid->data->M * 1e-3;
			if(!(molar_mass > 0.0) || !eqm_fluid_state_from_pT(spec->thermo.fluid, T, P0, &S)){
				return 0;
			}
			err = FPROPS_NO_ERROR;
			g_mass = fprops_g(S, &err);
			if(err || !isfinite(g_mass)){
				return 0;
			}
			*mu0 = g_mass * molar_mass;
			return 1;
		}
		return 0;
	case FPROPS_RXN_COMPILED_NONE:
	default:
		return 0;
	}
}

static int eqm_h_from_compiled(const FpropsRxnSpeciesCache *spec, double T, double P, double *h){
	FpropsError err = FPROPS_NO_ERROR;
	if(!spec || !h || !(T > 0.0) || !(P > 0.0)){
		return 0;
	}
	switch(spec->thermo.h_kind){
	case FPROPS_RXN_COMPILED_GIBBS:
		if(!spec->thermo.gibbs){
			return 0;
		}
		return gibbs_species_h_molar(spec->thermo.gibbs, T, 1e5, h);
	case FPROPS_RXN_COMPILED_SHOMATE:
		if(!spec->thermo.shomate){
			return 0;
		}
		*h = shomate_species_h_molar(spec->thermo.shomate, T, &err);
		return (!err && isfinite(*h)) ? 1 : 0;
	case FPROPS_RXN_COMPILED_CONSTCP:
		if(spec->thermo.constcp){
			const ConstCpData *phase = constcp_species_select_phase(spec->thermo.constcp, T, P, &err);
			double h_mass;
			if(err || !phase || !(spec->thermo.constcp->M > 0.0)){
				return 0;
			}
			err = FPROPS_NO_ERROR;
			h_mass = constcp_h(T, phase, &err);
			if(err || !isfinite(h_mass)){
				return 0;
			}
			*h = h_mass * (spec->thermo.constcp->M * 1e-3);
			return 1;
		}
			return 0;
	case FPROPS_RXN_COMPILED_FLUID:
		if(spec->thermo.fluid && spec->thermo.fluid->data){
			FluidState2 S;
			double h_mass;
			double molar_mass = spec->thermo.fluid->data->M * 1e-3;
			if(!(molar_mass > 0.0) || !eqm_fluid_state_from_pT(spec->thermo.fluid, T, P, &S)){
				return 0;
			}
			err = FPROPS_NO_ERROR;
			h_mass = fprops_h(S, &err);
			if(err || !isfinite(h_mass)){
				return 0;
			}
			*h = h_mass * molar_mass;
			return 1;
		}
		return 0;
	case FPROPS_RXN_COMPILED_NONE:
	default:
		return 0;
	}
}

static int eqm_v_from_compiled(const FpropsRxnSpeciesCache *spec, double T, double P, double *v){
	FpropsError err = FPROPS_NO_ERROR;
	if(!spec || !v || !(T > 0.0) || !(P > 0.0)){
		return 0;
	}
	switch(spec->thermo.v_kind){
	case FPROPS_RXN_COMPILED_SHOMATE:
		if(spec->thermo.shomate){
			double molar_mass;
			if(!(spec->thermo.shomate->rho_ref > 0.0) || !(spec->thermo.shomate->M > 0.0)){
				return 0;
			}
			molar_mass = spec->thermo.shomate->M * 1e-3;
			*v = molar_mass / spec->thermo.shomate->rho_ref;
			return isfinite(*v) && (*v > 0.0);
		}
		return 0;
	case FPROPS_RXN_COMPILED_CONSTCP:
		if(spec->thermo.constcp){
			const ConstCpData *phase = constcp_species_select_phase(spec->thermo.constcp, T, P, &err);
			double molar_mass;
			if(err || !phase || !(phase->rho > 0.0) || !(spec->thermo.constcp->M > 0.0)){
				return 0;
			}
			molar_mass = spec->thermo.constcp->M * 1e-3;
			*v = molar_mass / phase->rho;
			return isfinite(*v) && (*v > 0.0);
		}
		return 0;
	case FPROPS_RXN_COMPILED_FLUID:
		if(spec->thermo.fluid && spec->thermo.fluid->data){
			FluidState2 S;
			double v_mass;
			double molar_mass = spec->thermo.fluid->data->M * 1e-3;
			if(!(molar_mass > 0.0) || !eqm_fluid_state_from_pT(spec->thermo.fluid, T, P, &S)){
				return 0;
			}
			err = FPROPS_NO_ERROR;
			v_mass = fprops_v(S, &err);
			if(err || !isfinite(v_mass) || !(v_mass > 0.0)){
				return 0;
			}
			*v = v_mass * molar_mass;
			return isfinite(*v) && (*v > 0.0);
		}
		return 0;
	case FPROPS_RXN_COMPILED_GIBBS:
	case FPROPS_RXN_COMPILED_NONE:
	default:
		return 0;
	}
}

void eqm_apply_bscale(EqmData *D){
	int e;
	D->b_scale = (double *)calloc((size_t)D->ne, sizeof(double));
	for(e = 0; e < D->ne; ++e){
		double denom = fabs(D->b[e]);
		if(denom < 1.0){
			denom = 1.0;
		}
		D->b_scale[e] = 1.0 / denom;
	}
}

void eqm_apply_bscale_logn(EqmLogN *D){
	int e;
	D->b_scale = (double *)calloc((size_t)D->ne, sizeof(double));
	for(e = 0; e < D->ne; ++e){
		double denom = fabs(D->b[e]);
		if(denom < 1.0){
			denom = 1.0;
		}
		D->b_scale[e] = 1.0 / denom;
	}
}

void eqm_apply_bscale_n(EqmN *D){
	int e;
	D->b_scale = (double *)calloc((size_t)D->ne, sizeof(double));
	for(e = 0; e < D->ne; ++e){
		double denom = fabs(D->b[e]);
		if(denom < 1.0){
			denom = 1.0;
		}
		D->b_scale[e] = 1.0 / denom;
	}
}

void eqm_apply_nscale(EqmData *D, const double *n_init){
	int i;
	D->n_scale = (double *)calloc((size_t)D->ns, sizeof(double));
	for(i = 0; i < D->ns; ++i){
		double denom = 1.0;
		if(n_init && n_init[i] > 0.0){
			denom = n_init[i];
		}
		if(denom < 1e-30){
			denom = 1e-30;
		}
		D->n_scale[i] = 1.0 / denom;
	}
}

int eqm_compute_mu0(const char **names, int ns, const char *source, double T, double P0, double *mu0){
	int i;
	if(eqm_current_package){
		for(i = 0; i < ns; ++i){
			int idx = eqm_pkg_find_species(eqm_current_package, names[i]);
			if(idx >= 0){
				if(eqm_current_package->species[idx].entry_kind != FPROPS_RXN_ENTRY_PURE){
					mu0[i] = 0.0;
					continue;
				}
				if(eqm_mu0_from_compiled(&eqm_current_package->species[idx], T, P0, &mu0[i])){
					continue;
				}
			}
			break;
		}
		if(i == ns){
			eqm_package_trace("mu0_cache_hit", eqm_current_package, NULL, names, ns);
			MSG("eqm compute mu0: using cached package thermo for %d species", ns);
			return 1;
		}
		eqm_package_trace("mu0_cache_miss", eqm_current_package, NULL, names, ns);
		MSG("eqm compute mu0: package cache miss at species %d ('%s'), falling back",
			i, (i >= 0 && i < ns && names && names[i]) ? names[i] : "(null)");
	}
	for(i = 0; i < ns; ++i){
		const BinarySolutionPhaseDef *phase = NULL;
		const FeSpinelPhaseDef *spinel = NULL;
		unsigned member_index = 0;
		if(eqm_lookup_solution_member(names[i], source, &phase, &member_index)){
			(void)phase;
			(void)member_index;
			mu0[i] = 0.0;
			continue;
		}
		if(eqm_lookup_spinel_member(names[i], source, &spinel, &member_index)){
			(void)spinel;
			(void)member_index;
			mu0[i] = 0.0;
			continue;
		}
		if(!eqm_mu0_source(names[i], source, T, P0, &mu0[i])){
			ERR("eqm compute mu0 failed: no thermo data for '%s' (source='%s', T=%.17g, P0=%.17g)",
				names[i], source ? source : "", T, P0);
			return 0;
		}
	}
	return 1;
}

int eqm_compute_is_condensed(const char **names, int ns, const char *source, int *is_condensed){
	int i;
	if(!names || !is_condensed || ns <= 0){
		ERR("eqm compute is_condensed: invalid args names=%p is_condensed=%p ns=%d",
			(void *)names, (void *)is_condensed, ns);
		return 0;
	}
	if(eqm_current_package){
		for(i = 0; i < ns; ++i){
			int idx = eqm_pkg_find_species(eqm_current_package, names[i]);
			if(idx < 0){
				break;
			}
			is_condensed[i] = eqm_current_package->is_condensed[idx];
		}
		if(i == ns){
			eqm_package_trace("condensed_cache_hit", eqm_current_package, NULL, names, ns);
			MSG("eqm compute is_condensed: using cached package classification for %d species", ns);
			return 1;
		}
		eqm_package_trace("condensed_cache_miss", eqm_current_package, NULL, names, ns);
		MSG("eqm compute is_condensed: package cache miss at species %d ('%s'), falling back",
			i, (i >= 0 && i < ns && names && names[i]) ? names[i] : "(null)");
	}
	for(i = 0; i < ns; ++i){
		const BinarySolutionPhaseDef *phase = NULL;
		const FeSpinelPhaseDef *spinel = NULL;
		unsigned member_index = 0;
		const GibbsSpecies *G = NULL;
		const ConstCpSpecies *S = NULL;
		const ShomateSpecies *Sh = NULL;
		char source_buf[512];
		const char *selector_source = NULL;
		EqmMuModel selector_model = EQM_MODEL_AUTO;
		int use_ref0 = 0;
		const char *source_i;
		if(!names[i]){
			ERR("eqm compute is_condensed: null species name at index %d", i);
			return 0;
		}
		if(eqm_lookup_solution_member(names[i], source, &phase, &member_index)){
			(void)phase;
			(void)member_index;
			is_condensed[i] = 1;
			continue;
		}
		if(eqm_lookup_spinel_member(names[i], source, &spinel, &member_index)){
			(void)spinel;
			(void)member_index;
			is_condensed[i] = 1;
			continue;
		}
		source_i = fprops_resolve_species_source(source, names[i], source_buf,
			(unsigned)sizeof(source_buf));
		eqm_parse_selector(source_i, &selector_model, &use_ref0, &selector_source);
		G = gibbs_species_lookup(names[i], selector_source ? selector_source : source_i);
		if(!G){
			G = gibbs_species_lookup(names[i], NULL);
		}
		S = constcp_species_lookup(names[i], selector_source ? selector_source : source_i);
		if(!S){
			S = constcp_species_lookup(names[i], NULL);
		}
		Sh = shomate_species_lookup(names[i], selector_source);
		if(!Sh){
			Sh = shomate_species_lookup(names[i], NULL);
		}
		if(selector_model == EQM_MODEL_SHOMATE){
			is_condensed[i] = (Sh && Sh->phase != FPROPS_PHASE_GAS) ? 1 : 0;
		}else if(selector_model == EQM_MODEL_CONSTCP){
			is_condensed[i] = S ? 1 : 0;
		}else{
			int is_cond_constcp = S ? 1 : 0;
			int is_cond_shomate = (Sh && Sh->phase != FPROPS_PHASE_GAS) ? 1 : 0;
			int is_cond_gibbs = G ? 1 : 0;
			is_condensed[i] = (is_cond_constcp || is_cond_shomate || is_cond_gibbs) ? 1 : 0;
		}
	}
	return 1;
}

int eqm_compute_solution_phases(const char **names, int ns, const char *source,
		int **solution_phase_id_out, int **solution_member_index_out,
		EqmBinaryPhaseMeta **binary_phases_out, int *nbinary_phases_out){
	int *solution_phase_id = NULL;
	int *solution_member_index = NULL;
	EqmBinaryPhaseMeta *binary_phases = NULL;
	int nbinary_phases = 0;
	int i;

	if(!names || ns <= 0 || !solution_phase_id_out || !solution_member_index_out
			|| !binary_phases_out || !nbinary_phases_out){
		ERR("eqm compute solution phases: invalid args names=%p ns=%d", (void *)names, ns);
		return 0;
	}
	if(eqm_current_package && eqm_pkg_matches_basis_exact(eqm_current_package, names, ns)){
		solution_phase_id = (int *)calloc((size_t)ns, sizeof(int));
		solution_member_index = (int *)calloc((size_t)ns, sizeof(int));
		if(!solution_phase_id || !solution_member_index){
			ERR("eqm compute solution phases: allocation failed for cached copy ns=%d", ns);
			free(solution_phase_id);
			free(solution_member_index);
			return 0;
		}
		memcpy(solution_phase_id, eqm_current_package->solution_phase_id, sizeof(int) * (size_t)ns);
		memcpy(solution_member_index, eqm_current_package->solution_member_index, sizeof(int) * (size_t)ns);
		if(eqm_current_package->nbinary_phases > 0){
			binary_phases = (EqmBinaryPhaseMeta *)calloc((size_t)eqm_current_package->nbinary_phases,
				sizeof(EqmBinaryPhaseMeta));
			if(!binary_phases){
				ERR("eqm compute solution phases: allocation failed for %d cached phases",
					eqm_current_package->nbinary_phases);
				free(solution_phase_id);
				free(solution_member_index);
				return 0;
			}
			memcpy(binary_phases, eqm_current_package->binary_phases,
				sizeof(EqmBinaryPhaseMeta) * (size_t)eqm_current_package->nbinary_phases);
		}
		*solution_phase_id_out = solution_phase_id;
		*solution_member_index_out = solution_member_index;
		*binary_phases_out = binary_phases;
		*nbinary_phases_out = eqm_current_package->nbinary_phases;
		eqm_package_trace("solution_phase_cache_hit", eqm_current_package, NULL, names, ns);
		MSG("eqm compute solution phases: using cached package phase map (%d phases)",
			eqm_current_package->nbinary_phases);
		return 1;
	}
	if(eqm_current_package){
		eqm_package_trace("solution_phase_cache_miss", eqm_current_package, NULL, names, ns);
		MSG("eqm compute solution phases: package basis mismatch, rebuilding phase map");
	}

	solution_phase_id = (int *)calloc((size_t)ns, sizeof(int));
	solution_member_index = (int *)calloc((size_t)ns, sizeof(int));
	binary_phases = (EqmBinaryPhaseMeta *)calloc((size_t)ns, sizeof(EqmBinaryPhaseMeta));
	if(!solution_phase_id || !solution_member_index || !binary_phases){
		ERR("eqm compute solution phases: allocation failed ns=%d", ns);
		free(solution_phase_id);
		free(solution_member_index);
		free(binary_phases);
		return 0;
	}
	for(i = 0; i < ns; ++i){
		solution_phase_id[i] = -1;
		solution_member_index[i] = -1;
	}

	for(i = 0; i < ns; ++i){
		const BinarySolutionPhaseDef *phase = NULL;
		const FeSpinelPhaseDef *spinel = NULL;
		unsigned member_index = 0;
		int p;
		if(eqm_lookup_solution_member(names[i], source, &phase, &member_index)){
			for(p = 0; p < nbinary_phases; ++p){
				if(binary_phases[p].kind == EQM_PHASE_BINARY_SOLUTION
						&& binary_phases[p].phase == phase){
					break;
				}
			}
			if(p == nbinary_phases){
				binary_phases[p].kind = EQM_PHASE_BINARY_SOLUTION;
				binary_phases[p].ia = -1;
				binary_phases[p].ib = -1;
				binary_phases[p].phase = phase;
				binary_phases[p].spinel = NULL;
				for(int j = 0; j < 5; ++j){
					binary_phases[p].members[j] = -1;
				}
				++nbinary_phases;
			}
			if(member_index == 0){
				if(binary_phases[p].ia >= 0){
					ERR("eqm compute solution phases: duplicate binary solution member A for '%s'",
						names[i]);
					free(solution_phase_id);
					free(solution_member_index);
					free(binary_phases);
					return 0;
				}
				binary_phases[p].ia = i;
			}else if(member_index == 1){
				if(binary_phases[p].ib >= 0){
					ERR("eqm compute solution phases: duplicate binary solution member B for '%s'",
						names[i]);
					free(solution_phase_id);
					free(solution_member_index);
					free(binary_phases);
					return 0;
				}
				binary_phases[p].ib = i;
			}else{
				ERR("eqm compute solution phases: invalid binary member index %u for '%s'",
					member_index, names[i]);
				free(solution_phase_id);
				free(solution_member_index);
				free(binary_phases);
				return 0;
			}
			solution_phase_id[i] = p;
			solution_member_index[i] = (int)member_index;
			continue;
		}
		if(!eqm_lookup_spinel_member(names[i], source, &spinel, &member_index)){
			continue;
		}
		for(p = 0; p < nbinary_phases; ++p){
			if(binary_phases[p].kind == EQM_PHASE_FE_SPINEL
					&& binary_phases[p].spinel == spinel){
				break;
			}
		}
		if(p == nbinary_phases){
			binary_phases[p].kind = EQM_PHASE_FE_SPINEL;
			binary_phases[p].ia = -1;
			binary_phases[p].ib = -1;
			binary_phases[p].phase = NULL;
			binary_phases[p].spinel = spinel;
			for(int j = 0; j < 5; ++j){
				binary_phases[p].members[j] = -1;
			}
			++nbinary_phases;
		}
		if(member_index >= 5){
			ERR("eqm compute solution phases: invalid spinel member index %u for '%s'",
				member_index, names[i]);
			free(solution_phase_id);
			free(solution_member_index);
			free(binary_phases);
			return 0;
		}
		if(binary_phases[p].members[member_index] >= 0){
			ERR("eqm compute solution phases: duplicate spinel member %u for '%s'",
				member_index, names[i]);
			free(solution_phase_id);
			free(solution_member_index);
			free(binary_phases);
			return 0;
		}
		binary_phases[p].members[member_index] = i;
		solution_phase_id[i] = p;
		solution_member_index[i] = (int)member_index;
	}

	for(i = 0; i < nbinary_phases; ++i){
		if(binary_phases[i].kind == EQM_PHASE_BINARY_SOLUTION){
			if(binary_phases[i].ia < 0 || binary_phases[i].ib < 0){
				ERR("eqm compute solution phases: incomplete binary solution phase at %d", i);
				free(solution_phase_id);
				free(solution_member_index);
				free(binary_phases);
				return 0;
			}
		}else if(binary_phases[i].kind == EQM_PHASE_FE_SPINEL){
			for(int j = 0; j < 5; ++j){
				if(binary_phases[i].members[j] < 0){
					ERR("eqm compute solution phases: incomplete spinel phase %d member %d", i, j);
					free(solution_phase_id);
					free(solution_member_index);
					free(binary_phases);
					return 0;
				}
			}
		}
	}

	*solution_phase_id_out = solution_phase_id;
	*solution_member_index_out = solution_member_index;
	*binary_phases_out = (nbinary_phases > 0) ? binary_phases : NULL;
	*nbinary_phases_out = nbinary_phases;
	if(nbinary_phases == 0){
		free(binary_phases);
	}
	MSG("eqm compute solution phases: built phase map with %d phases", nbinary_phases);
	return 1;
}

void eqm_free_solution_phases(int **solution_phase_id, int **solution_member_index,
		EqmBinaryPhaseMeta **binary_phases){
	if(solution_phase_id && *solution_phase_id){
		free(*solution_phase_id);
		*solution_phase_id = NULL;
	}
	if(solution_member_index && *solution_member_index){
		free(*solution_member_index);
		*solution_member_index = NULL;
	}
	if(binary_phases && *binary_phases){
		free(*binary_phases);
		*binary_phases = NULL;
	}
}

int eqm_has_solution_phases(const char **names, int ns, const char *source){
	int i;
	if(eqm_current_package && eqm_pkg_matches_basis_exact(eqm_current_package, names, ns)){
		return eqm_current_package->nbinary_phases > 0;
	}
	for(i = 0; i < ns; ++i){
		const BinarySolutionPhaseDef *phase = NULL;
		const FeSpinelPhaseDef *spinel = NULL;
		unsigned member_index = 0;
		if(eqm_lookup_solution_member(names[i], source, &phase, &member_index)){
			(void)phase;
			(void)member_index;
			return 1;
		}
		if(eqm_lookup_spinel_member(names[i], source, &spinel, &member_index)){
			(void)spinel;
			(void)member_index;
			return 1;
		}
	}
	return 0;
}

static int eqm_augment_special_phase_constraints(const char **names, int ns, const char *source,
		int ne_in, const double *A_in, const double *b_in, int *ne_out,
		double **A_out, double **b_out){
	int *phase_id = NULL;
	int *member_index = NULL;
	EqmBinaryPhaseMeta *phases = NULL;
	int nphases = 0;
	int extra = 0;
	double *A_aug = NULL;
	double *b_aug = NULL;
	int e;
	int p;
	if(!names || !A_in || !b_in || !ne_out || !A_out || !b_out){
		return 0;
	}
	if(!eqm_compute_solution_phases(names, ns, source, &phase_id, &member_index, &phases, &nphases)){
		return 0;
	}
	for(p = 0; p < nphases; ++p){
		if(phases[p].kind == EQM_PHASE_FE_SPINEL){
			extra += 2;
		}
	}
	if(extra == 0){
		*ne_out = ne_in;
		*A_out = (double *)A_in;
		*b_out = (double *)b_in;
		eqm_free_solution_phases(&phase_id, &member_index, &phases);
		return 1;
	}
	A_aug = (double *)calloc((size_t)((ne_in + extra) * ns), sizeof(double));
	b_aug = (double *)calloc((size_t)(ne_in + extra), sizeof(double));
	if(!A_aug || !b_aug){
		free(A_aug);
		free(b_aug);
		eqm_free_solution_phases(&phase_id, &member_index, &phases);
		return 0;
	}
	memcpy(A_aug, A_in, sizeof(double) * (size_t)(ne_in * ns));
	memcpy(b_aug, b_in, sizeof(double) * (size_t)ne_in);
	e = ne_in;
	for(p = 0; p < nphases; ++p){
		if(phases[p].kind != EQM_PHASE_FE_SPINEL){
			continue;
		}
		/* 2(n_tet,total) - n_oct,total = 0 */
		A_aug[e * ns + phases[p].members[0]] = 2.0;
		A_aug[e * ns + phases[p].members[1]] = 2.0;
		A_aug[e * ns + phases[p].members[2]] = -1.0;
		A_aug[e * ns + phases[p].members[3]] = -1.0;
		A_aug[e * ns + phases[p].members[4]] = -1.0;
		b_aug[e] = 0.0;
		++e;
		/* Charge neutrality: 6 n_tFe2 + 5 n_tFe3 - 2 n_oFe2 - 3 n_oFe3 = 0 */
		A_aug[e * ns + phases[p].members[0]] = 6.0;
		A_aug[e * ns + phases[p].members[1]] = 5.0;
		A_aug[e * ns + phases[p].members[2]] = -2.0;
		A_aug[e * ns + phases[p].members[3]] = -3.0;
		b_aug[e] = 0.0;
		++e;
	}
	*ne_out = ne_in + extra;
	*A_out = A_aug;
	*b_out = b_aug;
	eqm_free_solution_phases(&phase_id, &member_index, &phases);
	return 1;
}

static int eqm_augment_special_phase_constraints_meta(int ns,
		const EqmBinaryPhaseMeta *phases, int nphases,
		int ne_in, const double *A_in, const double *b_in, int *ne_out,
		double **A_out, double **b_out){
	int extra = 0;
	double *A_aug = NULL;
	double *b_aug = NULL;
	int e;
	int p;

	if(!A_in || !b_in || !ne_out || !A_out || !b_out || ns <= 0 || ne_in <= 0){
		ERR("eqm augment special phase constraints: invalid args ns=%d ne_in=%d", ns, ne_in);
		return 0;
	}
	for(p = 0; p < nphases; ++p){
		if(phases[p].kind == EQM_PHASE_FE_SPINEL){
			extra += 2;
		}
	}
	if(extra == 0){
		*ne_out = ne_in;
		*A_out = (double *)A_in;
		*b_out = (double *)b_in;
		return 1;
	}
	A_aug = (double *)calloc((size_t)((ne_in + extra) * ns), sizeof(double));
	b_aug = (double *)calloc((size_t)(ne_in + extra), sizeof(double));
	if(!A_aug || !b_aug){
		ERR("eqm augment special phase constraints: allocation failed for ne=%d extra=%d ns=%d",
			ne_in, extra, ns);
		free(A_aug);
		free(b_aug);
		return 0;
	}
	memcpy(A_aug, A_in, sizeof(double) * (size_t)(ne_in * ns));
	memcpy(b_aug, b_in, sizeof(double) * (size_t)ne_in);
	e = ne_in;
	for(p = 0; p < nphases; ++p){
		if(phases[p].kind != EQM_PHASE_FE_SPINEL){
			continue;
		}
		A_aug[e * ns + phases[p].members[0]] = 2.0;
		A_aug[e * ns + phases[p].members[1]] = 2.0;
		A_aug[e * ns + phases[p].members[2]] = -1.0;
		A_aug[e * ns + phases[p].members[3]] = -1.0;
		A_aug[e * ns + phases[p].members[4]] = -1.0;
		b_aug[e] = 0.0;
		++e;
		A_aug[e * ns + phases[p].members[0]] = 6.0;
		A_aug[e * ns + phases[p].members[1]] = 5.0;
		A_aug[e * ns + phases[p].members[2]] = -2.0;
		A_aug[e * ns + phases[p].members[3]] = -3.0;
		b_aug[e] = 0.0;
		++e;
	}
	*ne_out = ne_in + extra;
	*A_out = A_aug;
	*b_out = b_aug;
	return 1;
}

int eqm_eval_obj_mu(const double *n, const double *mu0, const int *is_condensed,
		const int *solution_phase_id, const EqmBinaryPhaseMeta *binary_phases, int nbinary_phases,
		int ns, double T, double P, double P0, double *obj, double *mu, double *n_gas_out){
	const double RT = gas_R() * T;
	const double logPP0 = log(P / P0);
	double ngas = 0.0;
	int has_gas = 0;
	double f = 0.0;
	int i;

	if(!n || !mu0 || ns <= 0 || !(T > 0.0) || !(P > 0.0) || !(P0 > 0.0)){
		return 0;
	}
	for(i = 0; i < ns; ++i){
		if(!(n[i] > 0.0) || !isfinite(n[i])){
			return 0;
		}
		if(!is_condensed || !is_condensed[i]){
			ngas += n[i];
			has_gas = 1;
		}
	}
	if(has_gas && (!(ngas > 0.0) || !isfinite(ngas))){
		return 0;
	}
	if(n_gas_out){
		*n_gas_out = ngas;
	}
	for(i = 0; i < nbinary_phases; ++i){
		const EqmBinaryPhaseMeta *phase = &binary_phases[i];
		if(phase->kind == EQM_PHASE_BINARY_SOLUTION){
			const BinarySolutionModel *M;
			double n_a;
			double n_b;
			double n_tot;
			double x;
			double g_phase;
			double mu_a;
			double mu_b;
			FpropsError err = FPROPS_NO_ERROR;
			if(!phase->phase || !phase->phase->model){
				return 0;
			}
			M = phase->phase->model;
			n_a = n[phase->ia];
			n_b = n[phase->ib];
			n_tot = n_a + n_b;
			if(!(n_tot > 0.0) || !isfinite(n_tot)){
				return 0;
			}
			x = n_b / n_tot;
			g_phase = solution_binary_g_molar(M, T, P, x, &err);
			if(err || !isfinite(g_phase)){
				return 0;
			}
			mu_a = solution_binary_mu_a(M, T, P, x, &err);
			if(err || !isfinite(mu_a)){
				return 0;
			}
			mu_b = solution_binary_mu_b(M, T, P, x, &err);
			if(err || !isfinite(mu_b)){
				return 0;
			}
			if(mu){
				mu[phase->ia] = mu_a;
				mu[phase->ib] = mu_b;
			}
			if(obj){
				f += n_tot * g_phase;
			}
		}else if(phase->kind == EQM_PHASE_FE_SPINEL){
			double n_members[5];
			double mu_members[5];
			double g_phase = 0.0;
			int j;
			if(!phase->spinel){
				return 0;
			}
			for(j = 0; j < 5; ++j){
				int idx = phase->members[j];
				if(idx < 0){
					return 0;
				}
				n_members[j] = n[idx];
			}
			if(!spinel_phase_eval(phase->spinel, n_members, T, P, &g_phase, mu ? mu_members : NULL)){
				return 0;
			}
			if(obj){
				f += g_phase;
			}
			if(mu){
				for(j = 0; j < 5; ++j){
					mu[phase->members[j]] = mu_members[j];
				}
			}
		}else{
			return 0;
		}
	}
	for(i = 0; i < ns; ++i){
		if(solution_phase_id && solution_phase_id[i] >= 0){
			continue;
		}
		double mui = mu0[i];
		if(!is_condensed || !is_condensed[i]){
			mui += RT * (log(n[i]) - log(ngas) + logPP0);
		}
		if(!isfinite(mui)){
			return 0;
		}
		if(mu){
			mu[i] = mui;
		}
		if(obj){
			f += n[i] * mui;
		}
	}
	if(obj){
		*obj = f;
	}
	return 1;
}

static int eqm_mu0_constcp_source(const char *name, const char *source, double T, double P0,
		double *mu0){
	const ConstCpSpecies *S;
	const ConstCpData *phase = NULL;
	FpropsError err = FPROPS_NO_ERROR;
	double g_molar;

	if(!name || !mu0){
		return 0;
	}

	S = constcp_species_lookup(name, source);
	if(!S){
		S = constcp_species_lookup(name, NULL);
	}
	if(!S){
		return 0;
	}

	g_molar = constcp_species_g_molar(S, T, P0, &phase, &err);
	if(err || !isfinite(g_molar)){
		return 0;
	}
	*mu0 = g_molar;
	return 1;
}

static int eqm_mu0_shomate_source(const char *name, const char *source, double T, double P0,
		double *mu0){
	const ShomateSpecies *S;
	FpropsError err = FPROPS_NO_ERROR;
	double g_molar;

	if(!name || !mu0){
		return 0;
	}
	S = shomate_species_lookup(name, source);
	if(!S){
		S = shomate_species_lookup(name, NULL);
	}
	if(!S){
		return 0;
	}
	g_molar = shomate_species_g_molar(S, T, P0, &err);
	if(err || !isfinite(g_molar)){
		return 0;
	}
	*mu0 = g_molar;
	return 1;
}

static int eqm_mu0_gibbs_species_source(const char *name, const char *source, double T, double P0,
		double *mu0){
	const GibbsSpecies *S;
	double g_molar;
	(void)P0;

	if(!name || !mu0){
		return 0;
	}
	S = gibbs_species_lookup(name, source);
	if(!S){
		S = gibbs_species_lookup(name, NULL);
	}
	if(!S){
		return 0;
	}
	if(!gibbs_species_g_molar(S, T, 1e5, &g_molar)){
		return 0;
	}
	*mu0 = g_molar;
	return 1;
}

static PureFluid *eqm_prepare_fluid_for_mu0(const EosData *E, const char *corrtype, int use_ref0){
	ReferenceState ref0 = {FPROPS_REF_REF0};
	if(!E || !corrtype){
		return NULL;
	}
	if(strcmp(corrtype, "helmholtz") == 0){
		return helmholtz_prepare(E, use_ref0 ? &ref0 : NULL);
	}
	if(strcmp(corrtype, "pengrob") == 0){
		return pengrob_prepare(E, use_ref0 ? &ref0 : NULL);
	}
	if(strcmp(corrtype, "ideal") == 0){
		return ideal_prepare(E, use_ref0 ? &ref0 : NULL);
	}
	return NULL;
}

static int eqm_fluid_state_from_pT(const PureFluid *F, double T, double P, FluidState2 *S_out){
	FpropsError err = FPROPS_NO_ERROR;
	double rho;
	double p_calc = NAN;
	int it;

	if(!F || !S_out || !(T > 0.0) || !(P > 0.0) || !F->data || !(F->data->R > 0.0)){
		return 0;
	}

	rho = P / (F->data->R * T);
	if(!(rho > 0.0) || !isfinite(rho)){
		return 0;
	}

	if(F->type != FPROPS_IDEAL){
		if(!F->p_fn){
			return 0;
		}
		for(it = 0; it < 12; ++it){
			double rel;
			double rho_new;
			err = FPROPS_NO_ERROR;
			p_calc = F->p_fn((FluidStateUnion){.Trho={T, rho}}, F->data, &err);
			if(err || !isfinite(p_calc) || !(p_calc > 0.0)){
				return 0;
			}
			rel = fabs(p_calc - P) / P;
			if(rel < 1e-10){
				break;
			}
			rho_new = rho * (P / p_calc);
			if(!isfinite(rho_new) || !(rho_new > 0.0)){
				return 0;
			}
			if(rho_new > 1e5){
				rho_new = 1e5;
			}
			rho = rho_new;
		}
	}

	err = FPROPS_NO_ERROR;
	*S_out = fprops_set_Trho(T, rho, F, &err);
	if(err){
		return 0;
	}
	return 1;
}

static int eqm_mu0_fluid_model_source(const char *name, const char *corrtype, const char *source,
		double T, double P0, int use_ref0, double *mu0){
	PureFluid *P;
	const char *cands[3];
	int ncands = 0;
	int c;
	int explicit_source;
	if(!name || !corrtype || !mu0 || !(T > 0.0) || !(P0 > 0.0)){
		return 0;
	}
	explicit_source = eqm_has_explicit_source(source);
	cands[ncands++] = source;
	if(!explicit_source){
		cands[ncands++] = NULL;
	}
	if(!explicit_source && strcmp(corrtype, "ideal") == 0){
		cands[ncands++] = "RPP";
	}
	for(c = 0; c < ncands; ++c){
		double rho;
		double p_calc = NAN;
		double g;
		double molar_mass;
		FpropsError err = FPROPS_NO_ERROR;
		int it;
		const char *src = cands[c];
		const EosData *E = fprops_eos(name, corrtype, src);
		P = eqm_prepare_fluid_for_mu0(E, corrtype, use_ref0);
		if(!P){
			continue;
		}
		if(!P->data || !(P->data->R > 0.0) || !P->p_fn || !P->g_fn){
			fprops_fluid_destroy(P);
			continue;
		}
		rho = P0 / (P->data->R * T);
		if(!(rho > 0.0) || !isfinite(rho)){
			fprops_fluid_destroy(P);
			continue;
		}
		if(P->type != FPROPS_IDEAL){
			for(it = 0; it < 12; ++it){
				double rel;
				double rho_new;
				err = FPROPS_NO_ERROR;
				p_calc = P->p_fn((FluidStateUnion){.Trho={T, rho}}, P->data, &err);
				if(err || !isfinite(p_calc) || !(p_calc > 0.0)){
					break;
				}
				rel = fabs(p_calc - P0) / P0;
				if(rel < 1e-10){
					break;
				}
				rho_new = rho * (P0 / p_calc);
				if(!isfinite(rho_new) || !(rho_new > 0.0)){
					break;
				}
				if(rho_new > 1e5){
					rho_new = 1e5;
				}
				rho = rho_new;
			}
		}
		err = FPROPS_NO_ERROR;
		g = P->g_fn((FluidStateUnion){.Trho={T, rho}}, P->data, &err);
		molar_mass = P->data->M * 1e-3;
		fprops_fluid_destroy(P);
		if(!err && isfinite(g) && isfinite(molar_mass) && molar_mass > 0.0){
			*mu0 = g * molar_mass;
			return 1;
		}
	}
	return 0;
}

static int eqm_mu0_model_source(const char *name, EqmMuModel model, const char *source, double T,
		double P0, int use_ref0, double *mu0){
	if(model == EQM_MODEL_AUTO){
		if(eqm_mu0_ideal_source(name, source, T, P0, mu0)){
			return 1;
		}
		if(eqm_mu0_gibbs_species_source(name, source, T, P0, mu0)){
			return 1;
		}
		if(eqm_mu0_shomate_source(name, source, T, P0, mu0)){
			return 1;
		}
		return eqm_mu0_constcp_source(name, source, T, P0, mu0);
	}
	if(model == EQM_MODEL_IDEAL){
		return eqm_mu0_ideal_source(name, source, T, P0, mu0);
	}
	if(model == EQM_MODEL_CONSTCP){
		return eqm_mu0_constcp_source(name, source, T, P0, mu0);
	}
	if(model == EQM_MODEL_SHOMATE){
		return eqm_mu0_shomate_source(name, source, T, P0, mu0);
	}
	if(model == EQM_MODEL_HELMHOLTZ){
		return eqm_mu0_fluid_model_source(name, "helmholtz", source, T, P0, use_ref0, mu0);
	}
	if(model == EQM_MODEL_PENGROB){
		return eqm_mu0_fluid_model_source(name, "pengrob", source, T, P0, use_ref0, mu0);
	}
	return 0;
}

static int eqm_parse_selector(const char *spec, EqmMuModel *model_out, int *use_ref0_out,
		const char **source_out){
	char model_buf[32];
	const char *colon = NULL;
	size_t n = 0;
	if(model_out){
		*model_out = EQM_MODEL_AUTO;
	}
	if(use_ref0_out){
		*use_ref0_out = 0;
	}
	if(source_out){
		*source_out = spec;
	}
	if(!spec || !spec[0]){
		return 1;
	}
	colon = strchr(spec, ':');
	if(!colon){
		return 1;
	}
	while(spec[n] && &spec[n] < colon && n < sizeof(model_buf) - 1){
		model_buf[n] = (char)tolower((unsigned char)spec[n]);
		++n;
	}
	model_buf[n] = '\0';
	if(n == 0){
		return 1;
	}
	if(n > 5 && strcmp(model_buf + n - 5, "+ref0") == 0){
		if(use_ref0_out){
			*use_ref0_out = 1;
		}
		model_buf[n - 5] = '\0';
	}else if(n > 5 && strcmp(model_buf + n - 5, "_ref0") == 0){
		if(use_ref0_out){
			*use_ref0_out = 1;
		}
		model_buf[n - 5] = '\0';
	}
	if(model_out){
		if(strcmp(model_buf, "auto") == 0){
			*model_out = EQM_MODEL_AUTO;
		}else if(strcmp(model_buf, "ideal") == 0){
			*model_out = EQM_MODEL_IDEAL;
		}else if(strcmp(model_buf, "constcp") == 0){
			*model_out = EQM_MODEL_CONSTCP;
		}else if(strcmp(model_buf, "shomate") == 0){
			*model_out = EQM_MODEL_SHOMATE;
		}else if(strcmp(model_buf, "helmholtz") == 0){
			*model_out = EQM_MODEL_HELMHOLTZ;
		}else if(strcmp(model_buf, "pengrob") == 0){
			*model_out = EQM_MODEL_PENGROB;
		}else{
			*model_out = EQM_MODEL_AUTO;
			return 1;
		}
	}
	if(source_out){
		const char *src = colon + 1;
		while(*src && isspace((unsigned char)*src)){
			++src;
		}
		*source_out = (*src) ? src : NULL;
	}
	return 1;
}

int eqm_mu0_source(const char *name, const char *source, double T, double P0, double *mu0){
	char source_buf[512];
	const char *source_i = fprops_resolve_species_source(source, name, source_buf,
		(unsigned)sizeof(source_buf));
	EqmMuModel selector_model = EQM_MODEL_AUTO;
	int use_ref0 = 0;
	const char *selector_source = NULL;
	eqm_parse_selector(source_i, &selector_model, &use_ref0, &selector_source);
	if(eqm_mu0_model_source(name, selector_model, selector_source, T, P0, use_ref0, mu0)){
		return 1;
	}
	return 0;
}

static int eqm_h_constcp_source(const char *name, const char *source, double T, double P,
		double *h){
	const ConstCpSpecies *S;
	const ConstCpData *phase = NULL;
	FpropsError err = FPROPS_NO_ERROR;
	double h_mass;

	if(!name || !h){
		return 0;
	}
	S = constcp_species_lookup(name, source);
	if(!S){
		S = constcp_species_lookup(name, NULL);
	}
	if(!S){
		return 0;
	}
	phase = constcp_species_select_phase(S, T, P, &err);
	if(err || !phase){
		return 0;
	}
	h_mass = constcp_h(T, phase, &err);
	if(err || !isfinite(h_mass) || !(S->M > 0.0)){
		return 0;
	}
	*h = h_mass * (S->M * 1e-3);
	return 1;
}

static int eqm_h_shomate_source(const char *name, const char *source, double T, double P,
		double *h){
	const ShomateSpecies *S;
	FpropsError err = FPROPS_NO_ERROR;
	(void)P;

	if(!name || !h){
		return 0;
	}
	S = shomate_species_lookup(name, source);
	if(!S){
		S = shomate_species_lookup(name, NULL);
	}
	if(!S){
		return 0;
	}
	*h = shomate_species_h_molar(S, T, &err);
	if(err || !isfinite(*h)){
		return 0;
	}
	return 1;
}

static int eqm_h_fluid_model_source(const char *name, const char *corrtype, const char *source,
		double T, double P, int use_ref0, double *h){
	PureFluid *F;
	const char *cands[3];
	int ncands = 0;
	int c;
	int explicit_source;

	if(!name || !corrtype || !h || !(T > 0.0) || !(P > 0.0)){
		return 0;
	}
	explicit_source = eqm_has_explicit_source(source);
	cands[ncands++] = source;
	if(!explicit_source){
		cands[ncands++] = NULL;
	}
	if(!explicit_source && strcmp(corrtype, "ideal") == 0){
		cands[ncands++] = "RPP";
	}
	for(c = 0; c < ncands; ++c){
		const char *src = cands[c];
		const EosData *E = fprops_eos(name, corrtype, src);
		FpropsError err = FPROPS_NO_ERROR;
		FluidState2 S;
		double h_mass;
		double molar_mass;

		F = eqm_prepare_fluid_for_mu0(E, corrtype, use_ref0);
		if(!F){
			continue;
		}
		if(!eqm_fluid_state_from_pT(F, T, P, &S)){
			fprops_fluid_destroy(F);
			continue;
		}
		err = FPROPS_NO_ERROR;
		h_mass = fprops_h(S, &err);
		molar_mass = F->data ? (F->data->M * 1e-3) : NAN;
		fprops_fluid_destroy(F);
		if(!err && isfinite(h_mass) && isfinite(molar_mass) && molar_mass > 0.0){
			*h = h_mass * molar_mass;
			return 1;
		}
	}
	return 0;
}

static int eqm_h_model_source(const char *name, EqmMuModel model, const char *source, double T,
		double P, int use_ref0, double *h){
	if(model == EQM_MODEL_AUTO){
		if(eqm_h_fluid_model_source(name, "ideal", source, T, P, 1, h)){
			return 1;
		}
		if(eqm_h_shomate_source(name, source, T, P, h)){
			return 1;
		}
		if(eqm_h_constcp_source(name, source, T, P, h)){
			return 1;
		}
		return 0;
	}
	if(model == EQM_MODEL_IDEAL){
		return eqm_h_fluid_model_source(name, "ideal", source, T, P, 1, h);
	}
	if(model == EQM_MODEL_CONSTCP){
		return eqm_h_constcp_source(name, source, T, P, h);
	}
	if(model == EQM_MODEL_SHOMATE){
		return eqm_h_shomate_source(name, source, T, P, h);
	}
	if(model == EQM_MODEL_HELMHOLTZ){
		return eqm_h_fluid_model_source(name, "helmholtz", source, T, P, use_ref0, h);
	}
	if(model == EQM_MODEL_PENGROB){
		return eqm_h_fluid_model_source(name, "pengrob", source, T, P, use_ref0, h);
	}
	return 0;
}

static int eqm_h_source(const char *name, const char *source, double T, double P, double *h){
	char source_buf[512];
	const char *source_i = fprops_resolve_species_source(source, name, source_buf,
		(unsigned)sizeof(source_buf));
	EqmMuModel selector_model = EQM_MODEL_AUTO;
	int use_ref0 = 0;
	const char *selector_source = NULL;
	eqm_parse_selector(source_i, &selector_model, &use_ref0, &selector_source);
	return eqm_h_model_source(name, selector_model, selector_source, T, P, use_ref0, h);
}

int eqm_mu0_ideal_source(const char *name, const char *source, double T, double P0,
		double *mu0){
	FpropsError err = FPROPS_NO_ERROR;
	ReferenceState ref0 = {FPROPS_REF_REF0};
	PureFluid *Pideal;
	double rho;
	double g;
	double molar_mass;
	const EosData *cands[5];
	int ncands = 0;
	int c;
	int explicit_source = eqm_has_explicit_source(source);
	int prefer_shomate_source = 0;

	if(!name || !mu0){
		return 0;
	}
	if(explicit_source && shomate_species_lookup(name, source)){
		prefer_shomate_source = 1;
	}
	cands[ncands++] = fprops_eos(name, "ideal", source);
	cands[ncands++] = fprops_eos(name, NULL, source);
	if(!prefer_shomate_source){
		cands[ncands++] = fprops_eos(name, "ideal", NULL);
		cands[ncands++] = fprops_eos(name, NULL, NULL);
		cands[ncands++] = fprops_eos(name, NULL, "RPP");
	}
	for(c = 0; c < ncands; ++c){
		const EosData *E = cands[c];
		if(!E){
			continue;
		}
		Pideal = ideal_prepare(E, &ref0);
		if(!Pideal){
			continue;
		}
		err = FPROPS_NO_ERROR;
		rho = P0 / (Pideal->data->R * T);
		g = Pideal->g_fn((FluidStateUnion){.Trho={T, rho}}, Pideal->data, &err);
		molar_mass = Pideal->data->M * 1e-3;
		fprops_fluid_destroy(Pideal);
		if(!err){
			*mu0 = g * molar_mass;
			return 1;
		}
	}

	/* For explicit shomate-backed sources, do not ideal-fallback. */
	if(prefer_shomate_source){
		return 0;
	}

	Pideal = (PureFluid *)fprops_fluid(name, "ideal", NULL);
	if(Pideal){
		rho = P0 / (Pideal->data->R * T);
		err = FPROPS_NO_ERROR;
		g = Pideal->g_fn((FluidStateUnion){.Trho={T, rho}}, Pideal->data, &err);
		molar_mass = Pideal->data->M * 1e-3;
		fprops_fluid_destroy(Pideal);
		if(!err){
			*mu0 = g * molar_mass;
			return 1;
		}
	}
	Pideal = (PureFluid *)fprops_fluid(name, "ideal", "RPP");
	if(!Pideal){
		return 0;
	}
	err = FPROPS_NO_ERROR;
	rho = P0 / (Pideal->data->R * T);
	g = Pideal->g_fn((FluidStateUnion){.Trho={T, rho}}, Pideal->data, &err);
	molar_mass = Pideal->data->M * 1e-3;
	fprops_fluid_destroy(Pideal);
	if(err){
		return 0;
	}
	*mu0 = g * molar_mass;
	return 1;
}

int eqm_rref(double *A, int m, int n, int *pivots, int *rank){
	const double tol = 1e-12;
	int row = 0;
	int col;
	*rank = 0;
	for(col = 0; col < n && row < m; ++col){
		int piv = -1;
		double maxabs = 0.0;
		int r;
		for(r = row; r < m; ++r){
			double val = fabs(A[r * n + col]);
			if(val > maxabs){
				maxabs = val;
				piv = r;
			}
		}
		if(piv < 0 || maxabs < tol){
			continue;
		}
		if(piv != row){
			for(int c = col; c < n; ++c){
				double tmp = A[row * n + c];
				A[row * n + c] = A[piv * n + c];
				A[piv * n + c] = tmp;
			}
		}
		{
			double diag = A[row * n + col];
			for(int c = col; c < n; ++c){
				A[row * n + c] /= diag;
			}
		}
		for(r = 0; r < m; ++r){
			if(r == row){
				continue;
			}
			double factor = A[r * n + col];
			if(fabs(factor) < tol){
				continue;
			}
			for(int c = col; c < n; ++c){
				A[r * n + c] -= factor * A[row * n + c];
			}
		}
		pivots[*rank] = col;
		(*rank)++;
		row++;
	}
	return *rank;
}

static int eqm_is_pivot(int col, const int *pivots, int rank){
	for(int i = 0; i < rank; ++i){
		if(pivots[i] == col){
			return 1;
		}
	}
	return 0;
}

void eqm_fill_nullspace(const double *A_rref, int m, int n,
		const int *pivots, int rank, double *N_out, int r){
	int fidx = 0;
	(void)m;
	for(int col = 0; col < n; ++col){
		if(eqm_is_pivot(col, pivots, rank)){
			continue;
		}
		for(int i = 0; i < n; ++i){
			N_out[i * r + fidx] = 0.0;
		}
		N_out[col * r + fidx] = 1.0;
		for(int prow = 0; prow < rank; ++prow){
			int pcol = pivots[prow];
			N_out[pcol * r + fidx] = -A_rref[prow * n + col];
		}
		++fidx;
	}
}

int eqm_solve_particular(const double *A_in, const double *b_in, int m, int n, double *n0_out){
	const double tol = 1e-12;
	int ncols = n + 1;
	double *A = (double *)calloc((size_t)(m * ncols), sizeof(double));
	int *pivots = (int *)calloc((size_t)m, sizeof(int));
	int rank = 0;

	if(!A || !pivots){
		free(A);
		free(pivots);
		return 0;
	}
	for(int i = 0; i < m; ++i){
		for(int j = 0; j < n; ++j){
			A[i * ncols + j] = A_in[i * n + j];
		}
		A[i * ncols + n] = b_in[i];
	}
	eqm_rref(A, m, ncols, pivots, &rank);
	for(int i = 0; i < m; ++i){
		double row_norm = 0.0;
		for(int j = 0; j < n; ++j){
			row_norm += fabs(A[i * ncols + j]);
		}
		if(row_norm < tol && fabs(A[i * ncols + n]) > tol){
			free(A);
			free(pivots);
			return 0;
		}
	}
	for(int i = 0; i < n; ++i){
		n0_out[i] = 0.0;
	}
	for(int prow = 0; prow < rank; ++prow){
		int pcol = pivots[prow];
		n0_out[pcol] = A[prow * ncols + n];
	}
	free(A);
	free(pivots);
	return 1;
}

void eqm_fill_n_est(const double *A, const double *b, int ne, int ns,
		const double *n_init, double *n_est){
	size_t ns_count;
	if(ns <= 0){
		return;
	}
	ns_count = (size_t)ns;
	for(int i = 0; i < ns; ++i){
		n_est[i] = 1.0;
	}
	if(n_init){
		int ok_init = 1;
		for(int i = 0; i < ns; ++i){
			if(n_init[i] < 0.0){
				ok_init = 0;
				break;
			}
		}
		if(ok_init){
			for(int i = 0; i < ns; ++i){
				n_est[i] = n_init[i];
			}
			return;
		}
	}
	{
		double *n0 = (double *)calloc(ns_count, sizeof(double));
		int ok_n0 = 1;
		if(n0 && eqm_solve_particular(A, b, ne, ns, n0)){
			for(int i = 0; i < ns; ++i){
				if(n0[i] <= 0.0){
					ok_n0 = 0;
					break;
				}
			}
			if(ok_n0){
				for(int i = 0; i < ns; ++i){
					n_est[i] = n0[i];
				}
				free(n0);
				goto clamp;
			}
		}
		free(n0);
	}
	for(int i = 0; i < ns; ++i){
		double bound = HUGE_VAL;
		for(int e = 0; e < ne; ++e){
			double aei = A[e * ns + i];
			if(aei > 0.0){
				double val = b[e] / aei;
				if(val < bound){
					bound = val;
				}
			}
		}
		if(!isfinite(bound) || bound <= 0.0){
			bound = 1.0;
		}
		n_est[i] = bound;
	}
clamp:
	for(int i = 0; i < ns; ++i){
		if(n_est[i] < 1e-30){
			n_est[i] = 1e-30;
		}
		if(n_est[i] > 1e30){
			n_est[i] = 1e30;
		}
	}
}

double eqm_logsumexp(const double *logv, int n){
	double maxv = -HUGE_VAL;
	double sum = 0.0;
	for(int i = 0; i < n; ++i){
		if(logv[i] > maxv){
			maxv = logv[i];
		}
	}
	if(!isfinite(maxv)){
		return -HUGE_VAL;
	}
	for(int i = 0; i < n; ++i){
		sum += exp(logv[i] - maxv);
	}
	if(sum <= 0.0){
		return -HUGE_VAL;
	}
	return maxv + log(sum);
}

static int eqm_dense_solve(double *A, double *b, int n){
	const double piv_tol = 1e-14;
	for(int k = 0; k < n; ++k){
		int piv = k;
		double maxabs = fabs(A[k * n + k]);
		for(int i = k + 1; i < n; ++i){
			double v = fabs(A[i * n + k]);
			if(v > maxabs){
				maxabs = v;
				piv = i;
			}
		}
		if(!(maxabs > piv_tol)){
			return 0;
		}
		if(piv != k){
			for(int j = k; j < n; ++j){
				double tmp = A[k * n + j];
				A[k * n + j] = A[piv * n + j];
				A[piv * n + j] = tmp;
			}
			{
				double tmp = b[k];
				b[k] = b[piv];
				b[piv] = tmp;
			}
		}
		{
			double diag = A[k * n + k];
			for(int i = k + 1; i < n; ++i){
				double f = A[i * n + k] / diag;
				A[i * n + k] = 0.0;
				for(int j = k + 1; j < n; ++j){
					A[i * n + j] -= f * A[k * n + j];
				}
				b[i] -= f * b[k];
			}
		}
	}
	for(int i = n - 1; i >= 0; --i){
		double s = b[i];
		double diag = A[i * n + i];
		if(!(fabs(diag) > piv_tol)){
			return 0;
		}
		for(int j = i + 1; j < n; ++j){
			s -= A[i * n + j] * b[j];
		}
		b[i] = s / diag;
	}
	return 1;
}

static void eqm_reduced_compute_n(const double *n0, const double *N, int ns, int r,
		const double *z, double *n){
	for(int i = 0; i < ns; ++i){
		double v = n0[i];
		for(int j = 0; j < r; ++j){
			v += N[i * r + j] * z[j];
		}
		n[i] = v;
	}
}

static int eqm_reduced_bounds_r1_interval(const double *n0, const double *N, int ns,
		double n_floor, double *z_low, double *z_high){
	double zl = -HUGE_VAL;
	double zh = HUGE_VAL;
	for(int i = 0; i < ns; ++i){
		double Ni = N[i];
		double rhs = n_floor - n0[i];
		if(Ni > 0.0){
			double v = rhs / Ni;
			if(v > zl){
				zl = v;
			}
		}else if(Ni < 0.0){
			double v = rhs / Ni;
			if(v < zh){
				zh = v;
			}
		}else if(n0[i] <= n_floor){
			return 0;
		}
	}
	if(!(zl < zh)){
		return 0;
	}
	*z_low = zl;
	*z_high = zh;
	return 1;
}

static int eqm_reduced_bounds_r1_interval_ld(const double *n0, const double *N, int ns,
		long double n_floor, long double *z_low, long double *z_high){
	long double zl = -HUGE_VALL;
	long double zh = HUGE_VALL;
	for(int i = 0; i < ns; ++i){
		long double Ni = (long double)N[i];
		long double rhs = n_floor - (long double)n0[i];
		if(Ni > 0.0L){
			long double v = rhs / Ni;
			if(v > zl){
				zl = v;
			}
		}else if(Ni < 0.0L){
			long double v = rhs / Ni;
			if(v < zh){
				zh = v;
			}
		}else if((long double)n0[i] <= n_floor){
			return 0;
		}
	}
	if(!(zl < zh)){
		return 0;
	}
	*z_low = zl;
	*z_high = zh;
	return 1;
}

static int eqm_reduced_bounds_r1(const double *n0, const double *N, int ns, double n_floor,
		double *z_mid){
	double zl = 0.0;
	double zh = 0.0;
	if(!eqm_reduced_bounds_r1_interval(n0, N, ns, n_floor, &zl, &zh)){
		return 0;
	}
	*z_mid = 0.5 * (zl + zh);
	return 1;
}

static int eqm_reduced_project_ls(const double *n0, const double *N, int ns, int r,
		const double *n_target, double *z_out){
	double *G = NULL;
	double *rhs = NULL;
	int ok = 0;
	G = (double *)calloc((size_t)(r * r), sizeof(double));
	rhs = (double *)calloc((size_t)r, sizeof(double));
	if(!G || !rhs){
		free(G);
		free(rhs);
		return 0;
	}
	for(int j = 0; j < r; ++j){
		double s = 0.0;
		for(int i = 0; i < ns; ++i){
			s += N[i * r + j] * (n_target[i] - n0[i]);
		}
		rhs[j] = s;
		for(int k = 0; k < r; ++k){
			double g = 0.0;
			for(int i = 0; i < ns; ++i){
				g += N[i * r + j] * N[i * r + k];
			}
			G[j * r + k] = g;
		}
	}
	ok = eqm_dense_solve(G, rhs, r);
	if(ok){
		for(int j = 0; j < r; ++j){
			z_out[j] = rhs[j];
		}
	}
	free(G);
	free(rhs);
	return ok;
}

static int eqm_reduced_make_interior(const double *n0, const double *N, int ns, int r,
		double n_floor, double *z){
	double *n = NULL;
	double *p = NULL;
	const int trace = eqm_alg_trace_enabled();
	int pass;
	int iter;
	int ok = 0;

	if(r == 1){
		double zmid = 0.0;
		if(eqm_reduced_bounds_r1(n0, N, ns, n_floor, &zmid)){
			z[0] = zmid;
			return 1;
		}
		return 0;
	}

	n = (double *)calloc((size_t)ns, sizeof(double));
	p = (double *)calloc((size_t)r, sizeof(double));
	if(!n || !p){
		free(n);
		free(p);
		return 0;
	}

	for(pass = 0; pass < 3 && !ok; ++pass){
		if(pass == 1){
			for(int j = 0; j < r; ++j){
				z[j] = 0.0;
			}
		}else if(pass == 2){
			for(int j = 0; j < r; ++j){
				z[j] = (j % 2 == 0) ? 1.0 : -1.0;
			}
		}
		if(trace){
			fprintf(stderr, "FPROPS_EQM_MAKE_INTERIOR pass=%d z0=", pass);
			for(int j = 0; j < r; ++j){
				fprintf(stderr, "%s%.17g", j ? "," : "", z[j]);
			}
			fprintf(stderr, "\n");
		}
		for(iter = 0; iter < 600; ++iter){
			int imin = 0;
			double nmin;
			double norm2 = 0.0;
			double alpha_max = HUGE_VAL;
			double alpha;
			eqm_reduced_compute_n(n0, N, ns, r, z, n);
			nmin = n[0];
			for(int i = 1; i < ns; ++i){
				if(n[i] < nmin){
					nmin = n[i];
					imin = i;
				}
			}
			if(nmin > n_floor){
				if(trace){
					fprintf(stderr,
						"FPROPS_EQM_MAKE_INTERIOR success pass=%d iter=%d nmin=%.17g n_floor=%.17g\n",
						pass, iter, nmin, n_floor);
				}
				ok = 1;
				break;
			}
			for(int j = 0; j < r; ++j){
				p[j] = N[imin * r + j];
				norm2 += p[j] * p[j];
			}
			if(trace && iter < 8){
				fprintf(stderr,
					"FPROPS_EQM_MAKE_INTERIOR iter=%d pass=%d imin=%d nmin=%.17g norm2=%.17g n[imin]=%.17g n0[imin]=%.17g p=",
					iter, pass, imin, nmin, norm2, n[imin], n0[imin]);
				for(int j = 0; j < r; ++j){
					fprintf(stderr, "%s%.17g", j ? "," : "", p[j]);
				}
				fprintf(stderr, "\n");
			}
			if(!(norm2 > 1e-24)){
				if(trace){
					fprintf(stderr,
						"FPROPS_EQM_MAKE_INTERIOR break pass=%d iter=%d reason=small-norm2 imin=%d\n",
						pass, iter, imin);
				}
				break;
			}
			for(int i = 0; i < ns; ++i){
				double slope = 0.0;
				for(int j = 0; j < r; ++j){
					slope += N[i * r + j] * p[j];
				}
				if(slope < 0.0){
					double a = 0.9 * (n[i] - n_floor) / (-slope);
					if(a < alpha_max){
						alpha_max = a;
					}
				}
			}
			if(!(alpha_max > 0.0) || !isfinite(alpha_max)){
				if(trace){
					fprintf(stderr,
						"FPROPS_EQM_MAKE_INTERIOR break pass=%d iter=%d reason=alpha-max alpha_max=%.17g\n",
						pass, iter, alpha_max);
				}
				break;
			}
			alpha = (10.0 * n_floor - nmin) / norm2;
			if(alpha < 1e-8){
				alpha = 1e-8;
			}
			if(alpha > 0.5 * alpha_max){
				alpha = 0.5 * alpha_max;
			}
			if(!(alpha > 1e-16)){
				if(trace){
					fprintf(stderr,
						"FPROPS_EQM_MAKE_INTERIOR break pass=%d iter=%d reason=alpha-small alpha=%.17g alpha_max=%.17g\n",
						pass, iter, alpha, alpha_max);
				}
				break;
			}
			if(trace && iter < 8){
				fprintf(stderr,
					"FPROPS_EQM_MAKE_INTERIOR step pass=%d iter=%d alpha=%.17g alpha_max=%.17g\n",
					pass, iter, alpha, alpha_max);
			}
			for(int j = 0; j < r; ++j){
				z[j] += alpha * p[j];
			}
		}
	}

	if(trace && !ok){
		fprintf(stderr, "FPROPS_EQM_MAKE_INTERIOR failed final_z=");
		for(int j = 0; j < r; ++j){
			fprintf(stderr, "%s%.17g", j ? "," : "", z[j]);
		}
		fprintf(stderr, "\n");
	}

	free(n);
	free(p);
	return ok;
}

static int eqm_reduced_eval_obj_mu(const double *n, const double *mu0, const int *is_condensed,
		int ns, double T, double P, double P0, double *obj, double *mu, double *n_gas_tot){
	return eqm_eval_obj_mu(n, mu0, is_condensed, NULL, NULL, 0, ns, T, P, P0, obj, mu, n_gas_tot);
}

static void eqm_reduced_eval_grad_hess(const double *n, const double *N, int ns, int r,
		double T, const int *is_condensed, double *H){
	const double RT = gas_R() * T;
	const double n_curv_floor = 1e-80;
	double n_gas = 0.0;
	double *c = (double *)calloc((size_t)r, sizeof(double));
	if(!c){
		return;
	}
	for(int i = 0; i < ns; ++i){
		if(!is_condensed || !is_condensed[i]){
			n_gas += n[i];
		}
	}
	for(int j = 0; j < r; ++j){
		double cj = 0.0;
		for(int i = 0; i < ns; ++i){
			if(is_condensed && is_condensed[i]){
				continue;
			}
			double Nij = N[i * r + j];
			cj += Nij;
		}
		c[j] = cj;
	}
	if(!(n_gas > 0.0) || !isfinite(n_gas)){
		for(int j = 0; j < r; ++j){
			for(int k = 0; k < r; ++k){
				H[j * r + k] = 0.0;
			}
		}
		free(c);
		return;
	}
	for(int j = 0; j < r; ++j){
		for(int k = 0; k < r; ++k){
			double s = 0.0;
			for(int i = 0; i < ns; ++i){
				if(is_condensed && is_condensed[i]){
					continue;
				}
				double ni = n[i];
				if(ni < n_curv_floor){
					ni = n_curv_floor;
				}
				s += N[i * r + j] * N[i * r + k] / ni;
			}
			H[j * r + k] = RT * (s - (c[j] * c[k] / n_gas));
		}
	}
	free(c);
}

static int eqm_reduced_eval_phi_r1(const double *n0, const double *v, int ns,
		const double *mu0, const int *is_condensed, double T, double P, double P0, long double z,
		long double *phi, long double *obj){
	const long double RT = (long double)gas_R() * (long double)T;
	const long double logPP0 = logl((long double)P / (long double)P0);
	long double ngas = 0.0L;
	long double f = 0.0L;
	long double p = 0.0L;
	for(int i = 0; i < ns; ++i){
		long double ni = (long double)n0[i] + (long double)v[i] * z;
		if(!(ni > 0.0L) || !isfinite((double)ni)){
			return 0;
		}
		if(!is_condensed || !is_condensed[i]){
			ngas += ni;
		}
	}
	if(ngas < 0.0L || !isfinite((double)ngas)){
		return 0;
	}
	for(int i = 0; i < ns; ++i){
		long double ni = (long double)n0[i] + (long double)v[i] * z;
		long double mui = (long double)mu0[i];
		if(!is_condensed || !is_condensed[i]){
			if(!(ngas > 0.0L)){
				return 0;
			}
			mui += RT * (logl(ni) - logl(ngas) + logPP0);
		}
		if(!isfinite((double)mui)){
			return 0;
		}
		f += ni * mui;
		p += (long double)v[i] * mui;
	}
	*obj = f;
	*phi = p;
	return 1;
}

static int eqm_reduced_eval_phi_edge(const long double *n_edge, const double *dir, int ns,
		const double *mu0, const int *is_condensed, double T, double P, double P0, long double u,
		long double *phi, long double *obj){
	const long double RT = (long double)gas_R() * (long double)T;
	const long double logPP0 = logl((long double)P / (long double)P0);
	long double ngas = 0.0L;
	long double f = 0.0L;
	long double p = 0.0L;
	for(int i = 0; i < ns; ++i){
		long double ni = n_edge[i] + (long double)dir[i] * u;
		if(!(ni > 0.0L) || !isfinite((double)ni)){
			return 0;
		}
		if(!is_condensed || !is_condensed[i]){
			ngas += ni;
		}
	}
	if(ngas < 0.0L || !isfinite((double)ngas)){
		return 0;
	}
	for(int i = 0; i < ns; ++i){
		long double ni = n_edge[i] + (long double)dir[i] * u;
		long double mui = (long double)mu0[i];
		if(!is_condensed || !is_condensed[i]){
			if(!(ngas > 0.0L)){
				return 0;
			}
			mui += RT * (logl(ni) - logl(ngas) + logPP0);
		}
		if(!isfinite((double)mui)){
			return 0;
		}
		f += ni * mui;
		p += (long double)(-dir[i]) * mui; /* dir = +/-v, phi uses v */
	}
	*obj = f;
	*phi = p;
	return 1;
}

static int eqm_reduced_try_edge_root(const long double *n_edge, const double *dir, int ns,
		const double *mu0, const int *is_condensed, double T, double P, double P0, long double u_max,
		long double *u_best){
	const long double u_min = 1e-300L;
	long double ulo = u_min;
	long double uhi = u_min;
	long double flo = 0.0L;
	long double fhi = 0.0L;
	long double tmp = 0.0L;
	int bracketed = 0;

	if(!(u_max > u_min)){
		return 0;
	}
	if(!eqm_reduced_eval_phi_edge(n_edge, dir, ns, mu0, is_condensed, T, P, P0, ulo, &flo, &tmp)){
		return 0;
	}
	for(int it = 0; it < 600; ++it){
		uhi *= 10.0L;
		if(uhi > u_max){
			uhi = u_max;
		}
		if(!eqm_reduced_eval_phi_edge(n_edge, dir, ns, mu0, is_condensed, T, P, P0, uhi, &fhi, &tmp)){
			return 0;
		}
		if((flo == 0.0L) || (fhi == 0.0L) || (flo * fhi < 0.0L)){
			bracketed = 1;
			break;
		}
		ulo = uhi;
		flo = fhi;
		if(uhi >= u_max){
			break;
		}
	}
	if(!bracketed){
		return 0;
	}
	if(flo == 0.0L){
		*u_best = ulo;
		return 1;
	}
	if(fhi == 0.0L){
		*u_best = uhi;
		return 1;
	}
	for(int it = 0; it < 300; ++it){
		long double um = sqrtl(ulo * uhi);
		long double fm = 0.0L;
		if(!eqm_reduced_eval_phi_edge(n_edge, dir, ns, mu0, is_condensed, T, P, P0, um, &fm, &tmp)){
			return 0;
		}
		if(fabsl(fm) / ((long double)gas_R() * (long double)T) < 1e-10L
				|| (uhi / ulo) < (1.0L + 1e-14L)){
			*u_best = um;
			return 1;
		}
		if(flo * fm <= 0.0L){
			uhi = um;
			fhi = fm;
		}else{
			ulo = um;
			flo = fm;
		}
	}
	*u_best = sqrtl(ulo * uhi);
	return 1;
}

static int eqm_reduced_solve_r1(const double *n0, const double *v, int ns, const double *mu0,
		const int *is_condensed, double T, double P, double P0, double n_floor, double *n_out){
	long double zl;
	long double zh;
	long double span;
	long double eps;
	long double za = 0.0L;
	long double zb = 0.0L;
	long double phi_a = 0.0L;
	long double phi_b = 0.0L;
	long double obj_a = 0.0L;
	long double obj_b = 0.0L;
	long double z_best = 0.0L;
	long double u_edge = 0.0L;
	long double *n_low = NULL;
	long double *n_high = NULL;
	double *dir_low = NULL;
	double *dir_high = NULL;
	int used_edge = 0;
	int edge_high = 0;

	if(!eqm_reduced_bounds_r1_interval_ld(n0, v, ns, (long double)n_floor, &zl, &zh)){
		return 0;
	}
	n_low = (long double *)calloc((size_t)ns, sizeof(long double));
	n_high = (long double *)calloc((size_t)ns, sizeof(long double));
	dir_low = (double *)calloc((size_t)ns, sizeof(double));
	dir_high = (double *)calloc((size_t)ns, sizeof(double));
	if(!n_low || !n_high || !dir_low || !dir_high){
		free(n_low);
		free(n_high);
		free(dir_low);
		free(dir_high);
		return 0;
	}
	for(int i = 0; i < ns; ++i){
		n_low[i] = (long double)n0[i] + (long double)v[i] * zl;
		n_high[i] = (long double)n0[i] + (long double)v[i] * zh;
		dir_low[i] = v[i];
		dir_high[i] = -v[i];
	}

	span = zh - zl;
	eps = 1e-18L * (1.0L + fabsl(zl) + fabsl(zh));
	if(eps > 0.49L * span){
		eps = 0.49L * span;
	}
	za = nextafterl(zl, zh);
	zb = nextafterl(zh, zl);
	if(!(za < zb)){
		za = zl + eps;
		zb = zh - eps;
	}
	if(!(za < zb)){
		za = zl;
		zb = zh;
	}
	if(!eqm_reduced_eval_phi_r1(n0, v, ns, mu0, is_condensed, T, P, P0, za, &phi_a, &obj_a)){
		free(n_low);
		free(n_high);
		free(dir_low);
		free(dir_high);
		return 0;
	}
	if(!eqm_reduced_eval_phi_r1(n0, v, ns, mu0, is_condensed, T, P, P0, zb, &phi_b, &obj_b)){
		free(n_low);
		free(n_high);
		free(dir_low);
		free(dir_high);
		return 0;
	}
	if(phi_a == 0.0L){
		z_best = za;
	}else if(phi_b == 0.0L){
		z_best = zb;
	}else if(phi_a * phi_b < 0.0L){
		long double left = za;
		long double right = zb;
		long double fl = phi_a;
		for(int iter = 0; iter < 300; ++iter){
			long double mid = 0.5L * (left + right);
			long double fm = 0.0L;
			long double om = 0.0L;
			if(!eqm_reduced_eval_phi_r1(n0, v, ns, mu0, is_condensed, T, P, P0, mid, &fm, &om)){
				return 0;
			}
			if(fabsl(fm) / ((long double)gas_R() * (long double)T) < 1e-10L
					|| fabsl(right - left) < 1e-21L * (1.0L + fabsl(mid))){
				z_best = mid;
				break;
			}
			if(fl * fm <= 0.0L){
				right = mid;
			}else{
				left = mid;
				fl = fm;
			}
			z_best = 0.5L * (left + right);
		}
	}else{
		long double ubest = 0.0L;
		int found = 0;
		if(eqm_reduced_try_edge_root(n_high, dir_high, ns, mu0, is_condensed, T, P, P0, span, &ubest)){
			z_best = zh - ubest;
			u_edge = ubest;
			used_edge = 1;
			edge_high = 1;
			found = 1;
		}
		if(!found && eqm_reduced_try_edge_root(n_low, dir_low, ns, mu0, is_condensed, T, P, P0, span, &ubest)){
			z_best = zl + ubest;
			u_edge = ubest;
			used_edge = 1;
			edge_high = 0;
			found = 1;
		}
		if(!found){
			z_best = (obj_a <= obj_b) ? za : zb;
		}
	}
	for(int i = 0; i < ns; ++i){
		long double ni;
		if(used_edge){
			if(edge_high){
				ni = n_high[i] + (long double)dir_high[i] * u_edge;
			}else{
				ni = n_low[i] + (long double)dir_low[i] * u_edge;
			}
		}else{
			ni = (long double)n0[i] + (long double)v[i] * z_best;
		}
		n_out[i] = (double)ni;
		if(!(n_out[i] > 0.0) || !isfinite(n_out[i])){
			free(n_low);
			free(n_high);
			free(dir_low);
			free(dir_high);
			return 0;
		}
	}
	free(n_low);
	free(n_high);
	free(dir_low);
	free(dir_high);
	return 1;
}

static int eqm_reduced_solve_source_init_once(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, const double *n_init,
		double n_floor, double *n_out){
	const double P0 = 1e5;
	const double grad_tol = 1e-8;
	const int max_iter = 2000;
	const int trace = eqm_alg_trace_enabled();
	double *mu0 = NULL;
	int *is_condensed = NULL;
	double *Awork = NULL;
	int *pivots = NULL;
	int *pivots_perm = NULL;
	int *perm = NULL;
	int *inv_perm = NULL;
	double *N = NULL;
	double *Nperm = NULL;
	double *n0 = NULL;
	double *n = NULL;
	double *n_target = NULL;
	double *z = NULL;
	double *mu = NULL;
	double *grad = NULL;
	double *H = NULL;
	double *Hsys = NULL;
	double *rhs = NULL;
	double *dz = NULL;
	double *dn = NULL;
	int rank = 0;
	int r = 0;
	int status = -13;
	const char *reason = "uninitialized";

	if(!names || !A || !b || !n_out || ns <= 0 || ne <= 0 || !(T > 0.0) || !(P > 0.0)){
		return -11;
	}
	if(trace){
		fprintf(stderr,
			"FPROPS_EQM_REDUCED_TRACE enter T=%.17g P=%.17g ns=%d ne=%d n_floor=%.3e n_init=%s\n",
			T, P, ns, ne, n_floor, n_init ? "yes" : "no");
	}

	mu0 = (double *)calloc((size_t)ns, sizeof(double));
	is_condensed = (int *)calloc((size_t)ns, sizeof(int));
	Awork = (double *)calloc((size_t)(ne * ns), sizeof(double));
	pivots = (int *)calloc((size_t)ne, sizeof(int));
	pivots_perm = (int *)calloc((size_t)ne, sizeof(int));
	perm = (int *)calloc((size_t)ns, sizeof(int));
	inv_perm = (int *)calloc((size_t)ns, sizeof(int));
	n_target = (double *)calloc((size_t)ns, sizeof(double));
	if(!mu0 || !is_condensed || !Awork || !pivots || !pivots_perm || !perm || !inv_perm
			|| !n_target){
		status = -11;
		reason = "alloc-front";
		goto cleanup;
	}
	if(!eqm_compute_mu0(names, ns, source, T, P0, mu0)){
		status = -11;
		reason = "compute-mu0";
		goto cleanup;
	}
	if(!eqm_compute_is_condensed(names, ns, source, is_condensed)){
		status = -11;
		reason = "compute-is-condensed";
		goto cleanup;
	}
	eqm_fill_n_est(A, b, ne, ns, n_init, n_target);
	eqm_sort_columns_by_target(n_target, ns, perm, inv_perm);
	for(int e = 0; e < ne; ++e){
		for(int j = 0; j < ns; ++j){
			Awork[e * ns + j] = A[e * ns + perm[j]];
		}
	}
	eqm_rref(Awork, ne, ns, pivots_perm, &rank);
	r = ns - rank;
	for(int i = 0; i < rank; ++i){
		pivots[i] = perm[pivots_perm[i]];
	}
	if(trace){
		fprintf(stderr, "FPROPS_EQM_REDUCED_TRACE rank=%d r=%d\n", rank, r);
	}
	if(r <= 0){
		if(!eqm_solve_particular(A, b, ne, ns, n_out)){
			status = -13;
			reason = "solve-particular-r0";
			goto cleanup;
		}
		for(int i = 0; i < ns; ++i){
			if(!(n_out[i] > 0.0) || !isfinite(n_out[i])){
				status = -13;
				reason = "nonpositive-r0";
				goto cleanup;
			}
		}
		status = 0;
		reason = "success-r0";
		goto cleanup;
	}

	N = (double *)calloc((size_t)(ns * r), sizeof(double));
	Nperm = (double *)calloc((size_t)(ns * r), sizeof(double));
	n0 = (double *)calloc((size_t)ns, sizeof(double));
	n = (double *)calloc((size_t)ns, sizeof(double));
	z = (double *)calloc((size_t)r, sizeof(double));
	mu = (double *)calloc((size_t)ns, sizeof(double));
	grad = (double *)calloc((size_t)r, sizeof(double));
	H = (double *)calloc((size_t)(r * r), sizeof(double));
	Hsys = (double *)calloc((size_t)(r * r), sizeof(double));
	rhs = (double *)calloc((size_t)r, sizeof(double));
	dz = (double *)calloc((size_t)r, sizeof(double));
	dn = (double *)calloc((size_t)ns, sizeof(double));
	if(!N || !Nperm || !n0 || !n || !z || !mu || !grad || !H || !Hsys || !rhs || !dz || !dn){
		status = -11;
		reason = "alloc-main";
		goto cleanup;
	}

	eqm_fill_nullspace(Awork, ne, ns, pivots_perm, rank, Nperm, r);
	for(int j = 0; j < r; ++j){
		for(int i = 0; i < ns; ++i){
			N[i * r + j] = Nperm[inv_perm[i] * r + j];
		}
	}
	if(!eqm_solve_particular_from_pivots(A, b, ne, ns, pivots, rank, n0)
			&& !eqm_solve_particular(A, b, ne, ns, n0)){
		status = -13;
		reason = "solve-particular";
		goto cleanup;
	}
	if(r == 1){
		if(eqm_reduced_solve_r1(n0, N, ns, mu0, is_condensed, T, P, P0, n_floor, n_out)){
			status = 0;
			reason = "success-r1";
			goto cleanup;
		}
		if(trace){
			fprintf(stderr, "FPROPS_EQM_REDUCED_TRACE r1 closed-form path failed, continuing full solve\n");
		}
	}
	if(!eqm_reduced_project_ls(n0, N, ns, r, n_target, z)){
		if(trace){
			fprintf(stderr, "FPROPS_EQM_REDUCED_TRACE project-ls failed, using z=0\n");
		}
		for(int j = 0; j < r; ++j){
			z[j] = 0.0;
		}
	}
	eqm_reduced_compute_n(n0, N, ns, r, z, n);
	eqm_reduced_basis_trace_dump("post-project", names, ns, ne, A, b, pivots, rank,
		n0, N, r, n_target, z, n, T, P);
	{
		double nmin = n[0];
		for(int i = 1; i < ns; ++i){
			if(n[i] < nmin){
				nmin = n[i];
			}
		}
			if(nmin <= n_floor){
				if(!eqm_reduced_make_interior(n0, N, ns, r, n_floor, z)){
					eqm_reduced_basis_trace_dump("make-interior-failed", names, ns, ne, A, b,
						pivots, rank, n0, N, r, n_target, z, n, T, P);
					status = -13;
					reason = "make-interior";
					goto cleanup;
				}
				eqm_reduced_compute_n(n0, N, ns, r, z, n);
				eqm_reduced_basis_trace_dump("post-make-interior", names, ns, ne, A, b, pivots,
					rank, n0, N, r, n_target, z, n, T, P);
			}
			if(trace){
			fprintf(stderr, "FPROPS_EQM_REDUCED_TRACE initial nmin=%.17g\n", nmin);
		}
	}

	for(int iter = 0; iter < max_iter; ++iter){
		double obj = 0.0;
		double grad_inf = 0.0;
		double gdotdz;
		double alpha_max = HUGE_VAL;
		double alpha;
		int accepted = 0;

		if(!eqm_reduced_eval_obj_mu(n, mu0, is_condensed, ns, T, P, P0, &obj, mu, NULL)){
			status = -13;
			reason = "eval-obj-mu";
			goto cleanup;
		}
		for(int j = 0; j < r; ++j){
			double gj = 0.0;
			for(int i = 0; i < ns; ++i){
				gj += N[i * r + j] * mu[i];
			}
			grad[j] = gj;
		}
		eqm_reduced_eval_grad_hess(n, N, ns, r, T, is_condensed, H);
		for(int j = 0; j < r; ++j){
			double g = fabs(grad[j]) / (gas_R() * T);
			if(g > grad_inf){
				grad_inf = g;
			}
		}
		if(grad_inf < grad_tol){
			for(int i = 0; i < ns; ++i){
				n_out[i] = n[i];
			}
			status = 0;
			reason = "success-gradtol";
			goto cleanup;
		}
		if(trace && (iter < 5 || iter == max_iter - 1)){
			fprintf(stderr, "FPROPS_EQM_REDUCED_TRACE iter=%d grad_inf=%.17g obj=%.17g\n",
				iter, grad_inf, obj);
		}

		{
			double lambda = 0.0;
			int solved = 0;
			for(int damp = 0; damp < 8; ++damp){
				for(int j = 0; j < r; ++j){
					rhs[j] = -grad[j];
					for(int k = 0; k < r; ++k){
						Hsys[j * r + k] = H[j * r + k];
					}
					Hsys[j * r + j] += lambda;
				}
				if(eqm_dense_solve(Hsys, rhs, r)){
					solved = 1;
					for(int j = 0; j < r; ++j){
						dz[j] = rhs[j];
					}
					break;
				}
				lambda = (lambda == 0.0) ? 1e-12 : (lambda * 100.0);
			}
			if(!solved){
				for(int j = 0; j < r; ++j){
					dz[j] = -grad[j];
				}
			}
		}

		gdotdz = 0.0;
		for(int j = 0; j < r; ++j){
			gdotdz += grad[j] * dz[j];
		}
		if(!(gdotdz < 0.0)){
			gdotdz = 0.0;
			for(int j = 0; j < r; ++j){
				dz[j] = -grad[j];
				gdotdz += grad[j] * dz[j];
			}
			if(!(gdotdz < 0.0)){
				status = -13;
				reason = "non-descent";
				goto cleanup;
			}
		}

		for(int i = 0; i < ns; ++i){
			double s = 0.0;
			for(int j = 0; j < r; ++j){
				s += N[i * r + j] * dz[j];
			}
			dn[i] = s;
			if(s < 0.0){
				double a = 0.995 * (n[i] - n_floor) / (-s);
				if(a < alpha_max){
					alpha_max = a;
				}
			}
		}
		if(!(alpha_max > 0.0) || !isfinite(alpha_max)){
			status = -13;
			reason = "alpha-max";
			goto cleanup;
		}
		alpha = 1.0;
		if(alpha > alpha_max){
			alpha = alpha_max;
		}
		for(int bt = 0; bt < 80; ++bt){
			double obj_trial = 0.0;
			int valid = 1;
			for(int j = 0; j < r; ++j){
				rhs[j] = z[j] + alpha * dz[j];
			}
			eqm_reduced_compute_n(n0, N, ns, r, rhs, dn);
			for(int i = 0; i < ns; ++i){
				if(!(dn[i] > n_floor) || !isfinite(dn[i])){
					valid = 0;
					break;
				}
			}
			if(valid && eqm_reduced_eval_obj_mu(dn, mu0, is_condensed, ns, T, P, P0,
					&obj_trial, mu, NULL)){
				if(obj_trial <= obj + 1e-4 * alpha * gdotdz || obj_trial < obj){
					accepted = 1;
					for(int j = 0; j < r; ++j){
						z[j] = rhs[j];
					}
					for(int i = 0; i < ns; ++i){
						n[i] = dn[i];
					}
					break;
				}
			}
			alpha *= 0.5;
			if(alpha < 1e-24){
				break;
			}
		}
		if(!accepted){
			status = -13;
			reason = "line-search";
			goto cleanup;
		}
	}

	status = -13;
	reason = "max-iter";

cleanup:
	if(trace){
		fprintf(stderr, "FPROPS_EQM_REDUCED_TRACE exit status=%d reason=%s T=%.17g P=%.17g\n",
			status, reason, T, P);
	}
	free(dn);
	free(dz);
	free(rhs);
	free(Hsys);
	free(H);
	free(grad);
	free(mu);
	free(z);
	free(n_target);
	free(n);
	free(n0);
	free(N);
	free(Nperm);
	free(inv_perm);
	free(perm);
	free(pivots_perm);
	free(pivots);
	free(Awork);
	free(is_condensed);
	free(mu0);
	return status;
}

static int eqm_reduced_eval_reduced_gradients(const double *mu, const double *A, int ns, int ne,
		const int *is_active, double T, double *red){
	double *M = NULL;
	double *Msys = NULL;
	double *rhs = NULL;
	double *lambda = NULL;
	int nfree = 0;

	if(!mu || !A || !is_active || !red || ns <= 0 || ne <= 0 || !(T > 0.0)){
		return 0;
	}
	for(int i = 0; i < ns; ++i){
		if(!is_active[i]){
			++nfree;
		}
	}
	if(nfree <= 0){
		return 0;
	}
	M = (double *)calloc((size_t)(ne * ne), sizeof(double));
	Msys = (double *)calloc((size_t)(ne * ne), sizeof(double));
	rhs = (double *)calloc((size_t)ne, sizeof(double));
	lambda = (double *)calloc((size_t)ne, sizeof(double));
	if(!M || !Msys || !rhs || !lambda){
		free(M);
		free(Msys);
		free(rhs);
		free(lambda);
		return 0;
	}
	for(int p = 0; p < ne; ++p){
		double bp = 0.0;
		for(int i = 0; i < ns; ++i){
			if(is_active[i]){
				continue;
			}
			bp += A[p * ns + i] * mu[i];
		}
		rhs[p] = -bp;
		for(int q = 0; q < ne; ++q){
			double s = 0.0;
			for(int i = 0; i < ns; ++i){
				if(is_active[i]){
					continue;
				}
				s += A[p * ns + i] * A[q * ns + i];
			}
			M[p * ne + q] = s;
		}
	}
	{
		double reg = 0.0;
		int solved = 0;
		for(int damp = 0; damp < 8; ++damp){
			for(int p = 0; p < ne; ++p){
				lambda[p] = rhs[p];
				for(int q = 0; q < ne; ++q){
					Msys[p * ne + q] = M[p * ne + q];
				}
				Msys[p * ne + p] += reg;
			}
			if(eqm_dense_solve(Msys, lambda, ne)){
				solved = 1;
				break;
			}
			reg = (reg == 0.0) ? 1e-18 : (reg * 100.0);
		}
		if(!solved){
			free(M);
			free(Msys);
			free(rhs);
			free(lambda);
			return 0;
		}
	}
	for(int i = 0; i < ns; ++i){
		double r = mu[i];
		for(int p = 0; p < ne; ++p){
			r += A[p * ns + i] * lambda[p];
		}
		red[i] = r / (gas_R() * T);
	}
	free(M);
	free(Msys);
	free(rhs);
	free(lambda);
	return 1;
}

static void eqm_bound_classify_active_free(const double *n, const double *red, int ns,
		double dual_tol, int *is_active, int *nactive_out, int *nfree_out,
		double *n_active_cutoff_out, double *n_small_cutoff_out){
	double n_tot = 0.0;
	double n_active_cutoff;
	double n_small_cutoff;
	int nactive = 0;
	int nfree = 0;

	if(!n || !is_active || ns <= 0){
		if(nactive_out){
			*nactive_out = 0;
		}
		if(nfree_out){
			*nfree_out = 0;
		}
		if(n_active_cutoff_out){
			*n_active_cutoff_out = 0.0;
		}
		if(n_small_cutoff_out){
			*n_small_cutoff_out = 0.0;
		}
		return;
	}

	for(int i = 0; i < ns; ++i){
		if(n[i] > 0.0 && isfinite(n[i])){
			n_tot += n[i];
		}
	}
	n_active_cutoff = fmax(1e-60, EQM_BOUND_ACTIVE_CUTOFF_FRAC * n_tot);
	n_small_cutoff = fmax(n_active_cutoff, EQM_BOUND_ACTIVE_SMALL_FRAC * n_tot);

	for(int i = 0; i < ns; ++i){
		int active = 0;
		if(n[i] <= n_active_cutoff){
			active = 1;
		}else if(red && n[i] <= n_small_cutoff){
			double comp = n[i] * fmax(red[i], 0.0);
			if(red[i] >= -dual_tol
					&& comp <= EQM_BOUND_COMPLEMENTARITY_TOL * n_tot){
				active = 1;
			}
		}
		is_active[i] = active;
		if(active){
			++nactive;
		}else{
			++nfree;
		}
	}

	if(nactive_out){
		*nactive_out = nactive;
	}
	if(nfree_out){
		*nfree_out = nfree;
	}
	if(n_active_cutoff_out){
		*n_active_cutoff_out = n_active_cutoff;
	}
	if(n_small_cutoff_out){
		*n_small_cutoff_out = n_small_cutoff;
	}
}

static void eqm_active_set_push_candidate(int idx, double score, int maxcand,
		int *ncand, int *cand_idx, double *cand_score){
	int pos;
	if(!ncand || !cand_idx || !cand_score || maxcand <= 0 || idx < 0 || !(score > 0.0)){
		return;
	}
	for(int i = 0; i < *ncand; ++i){
		if(cand_idx[i] == idx){
			if(score <= cand_score[i]){
				return;
			}
			for(int j = i; j + 1 < *ncand; ++j){
				cand_idx[j] = cand_idx[j + 1];
				cand_score[j] = cand_score[j + 1];
			}
			--(*ncand);
			break;
		}
	}
	if(*ncand < maxcand){
		pos = (*ncand)++;
	}else{
		if(score <= cand_score[maxcand - 1]){
			return;
		}
		pos = maxcand - 1;
	}
	while(pos > 0 && score > cand_score[pos - 1]){
		cand_idx[pos] = cand_idx[pos - 1];
		cand_score[pos] = cand_score[pos - 1];
		--pos;
	}
	cand_idx[pos] = idx;
	cand_score[pos] = score;
}

static int eqm_reduced_active_set_trial(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, const double *mu0,
		const int *is_condensed, const int *is_active, const double *n_hint, double n_floor,
		double *n_trial, double *mu, double *red, double *obj_out, double *max_free_resid_out,
		int *kkt_ok_out, int *free_idx, const char **names_f, double *A_f, double *b_f,
		double *n_f, double *init_f){
	const double pin = n_floor;
	const double P0 = 1e5;
	double obj = 0.0;
	double max_free_resid = 0.0;
	int nf = 0;
	int status;

	if(!names || !A || !b || !mu0 || !is_condensed || !is_active || !n_hint || !n_trial
			|| !mu || !red || !free_idx || !names_f || !A_f || !b_f || !n_f || !init_f){
		return 0;
	}
	for(int i = 0; i < ns; ++i){
		if(!is_active[i]){
			free_idx[nf++] = i;
		}
	}
	if(nf <= 0){
		return 0;
	}
	for(int e = 0; e < ne; ++e){
		double rhs = b[e];
		for(int i = 0; i < ns; ++i){
			if(is_active[i]){
				rhs -= A[e * ns + i] * pin;
			}
		}
		b_f[e] = rhs;
	}
	for(int j = 0; j < nf; ++j){
		int i = free_idx[j];
		double ni = n_hint[i];
		if(!(ni > n_floor) || !isfinite(ni)){
			ni = fmax(10.0 * n_floor, 1e-30);
		}
		names_f[j] = names[i];
		init_f[j] = ni;
		for(int e = 0; e < ne; ++e){
			A_f[e * nf + j] = A[e * ns + i];
		}
	}
	status = eqm_reduced_solve_source_init_once(names_f, nf, ne, A_f, b_f, source, T, P,
		init_f, n_floor, n_f);
	if(status != 0){
		status = eqm_reduced_solve_source_init_once(names_f, nf, ne, A_f, b_f, source, T, P,
			NULL, n_floor, n_f);
	}
	if(status != 0){
		return 0;
	}
	for(int i = 0; i < ns; ++i){
		n_trial[i] = is_active[i] ? pin : 0.0;
	}
	for(int j = 0; j < nf; ++j){
		n_trial[free_idx[j]] = n_f[j];
	}
	if(!eqm_reduced_eval_obj_mu(n_trial, mu0, is_condensed, ns, T, P, P0, &obj, mu, NULL)){
		return 0;
	}
	if(!eqm_reduced_eval_reduced_gradients(mu, A, ns, ne, is_active, T, red)){
		return 0;
	}
	for(int i = 0; i < ns; ++i){
		if(!is_active[i]){
			double ar = fabs(red[i]);
			if(ar > max_free_resid){
				max_free_resid = ar;
			}
		}
	}
	if(obj_out){
		*obj_out = obj;
	}
	if(max_free_resid_out){
		*max_free_resid_out = max_free_resid;
	}
	if(kkt_ok_out){
		*kkt_ok_out = eqm_validate_solution_bounds(names, ns, ne, A, b, source, T, P, n_trial);
	}
	return 1;
}

static int eqm_active_set_trial_better(int have_best, int kkt_ok, double obj,
		double max_free_resid, int best_kkt_ok, double best_obj, double best_max_free_resid){
	double obj_tol;
	if(!have_best){
		return 1;
	}
	if(kkt_ok != best_kkt_ok){
		return kkt_ok > best_kkt_ok;
	}
	obj_tol = 1e-12 * fmax(1.0, fmax(fabs(obj), fabs(best_obj)));
	if(obj < best_obj - obj_tol){
		return 1;
	}
	if(obj > best_obj + obj_tol){
		return 0;
	}
	return max_free_resid < best_max_free_resid;
}

static int eqm_reduced_active_set_seed(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, const double *n_hint,
		double n_floor, double *n_seed_out){
	const int max_iter = 24;
	const int max_candidates = 4;
	const double pin = n_floor;
	const double active_n_cut = fmax(1e-30, 1e3 * n_floor);
	const double free_tol = EQM_BOUND_KKT_FREE_TOL;
	const double dual_tol = EQM_BOUND_KKT_DUAL_TOL;
	const double P0 = 1e5;
	const int trace = eqm_active_trace_enabled();
	int nactive = 0;
	int nfree = 0;
	int *is_active = NULL;
	int *free_idx = NULL;
	const char **names_f = NULL;
	double *A_f = NULL;
	double *b_f = NULL;
	double *n_f = NULL;
	double *init_f = NULL;
	double *n_work = NULL;
	double *n_trial = NULL;
	double *n_trial_best = NULL;
	double *mu0 = NULL;
	int *is_condensed = NULL;
	double *mu = NULL;
	double *red = NULL;
	double *mu_cand = NULL;
	double *red_cand = NULL;
	int *is_active_best = NULL;
	double obj_dummy = 0.0;
	int ok = 0;
	int status = -13;

	if(!names || !A || !b || !n_hint || !n_seed_out || ns <= 1 || ne <= 0){
		return 0;
	}
	is_active = (int *)calloc((size_t)ns, sizeof(int));
	free_idx = (int *)calloc((size_t)ns, sizeof(int));
	names_f = (const char **)calloc((size_t)ns, sizeof(const char *));
	A_f = (double *)calloc((size_t)(ne * ns), sizeof(double));
	b_f = (double *)calloc((size_t)ne, sizeof(double));
	n_f = (double *)calloc((size_t)ns, sizeof(double));
	init_f = (double *)calloc((size_t)ns, sizeof(double));
	n_work = (double *)calloc((size_t)ns, sizeof(double));
	n_trial = (double *)calloc((size_t)ns, sizeof(double));
	n_trial_best = (double *)calloc((size_t)ns, sizeof(double));
	mu0 = (double *)calloc((size_t)ns, sizeof(double));
	is_condensed = (int *)calloc((size_t)ns, sizeof(int));
	mu = (double *)calloc((size_t)ns, sizeof(double));
	red = (double *)calloc((size_t)ns, sizeof(double));
	mu_cand = (double *)calloc((size_t)ns, sizeof(double));
	red_cand = (double *)calloc((size_t)ns, sizeof(double));
	is_active_best = (int *)calloc((size_t)ns, sizeof(int));
	if(!is_active || !free_idx || !names_f || !A_f || !b_f || !n_f || !init_f
			|| !n_work || !n_trial || !n_trial_best || !mu0 || !is_condensed || !mu || !red
			|| !mu_cand || !red_cand || !is_active_best){
		goto cleanup;
	}
	if(!eqm_compute_mu0(names, ns, source, T, P0, mu0)){
		goto cleanup;
	}
	if(!eqm_compute_is_condensed(names, ns, source, is_condensed)){
		goto cleanup;
	}
	for(int i = 0; i < ns; ++i){
		double ni = n_hint[i];
		if(!(ni > 0.0) || !isfinite(ni)){
			ni = fmax(10.0 * n_floor, 1e-30);
		}
		n_work[i] = ni;
	}
	eqm_bound_classify_active_free(n_work, NULL, ns, dual_tol, is_active, &nactive, &nfree,
		NULL, NULL);
	if(nfree > 0
			&& eqm_reduced_eval_obj_mu(n_work, mu0, is_condensed, ns, T, P, P0, &obj_dummy, mu, NULL)
			&& eqm_reduced_eval_reduced_gradients(mu, A, ns, ne, is_active, T, red)){
		eqm_bound_classify_active_free(n_work, red, ns, dual_tol, is_active, &nactive, &nfree,
			NULL, NULL);
	}
	if(nactive <= 0){
		int imin = -1;
		double nmin = HUGE_VAL;
		for(int i = 0; i < ns; ++i){
			if(n_work[i] < nmin){
				nmin = n_work[i];
				imin = i;
			}
			is_active[i] = 0;
		}
		if(imin >= 0){
			is_active[imin] = 1;
			nactive = 1;
			nfree = ns - 1;
		}
	}
	if(nfree <= 0){
		int imax = -1;
		double nmax = -HUGE_VAL;
		for(int i = 0; i < ns; ++i){
			if(n_work[i] > nmax){
				nmax = n_work[i];
				imax = i;
			}
			is_active[i] = 1;
		}
		if(imax >= 0){
			is_active[imax] = 0;
			nactive = ns - 1;
			nfree = 1;
		}
	}
	if(trace){
		fprintf(stderr, "eqm active-set seed start: T=%.6g P=%.6g n_floor=%.3e nactive=%d nfree=%d\n",
			T, P, n_floor, nactive, nfree);
	}

	for(int it = 0; it < max_iter; ++it){
		int nf = 0;
		int add_idx = -1;
		int drop_idx = -1;
		double add_score = 0.0;
		double drop_score = 0.0;
		double max_free_resid = 0.0;
		int add_cands[4] = {-1, -1, -1, -1};
		double add_cand_scores[4] = {0.0, 0.0, 0.0, 0.0};
		int drop_cands[4] = {-1, -1, -1, -1};
		double drop_cand_scores[4] = {0.0, 0.0, 0.0, 0.0};
		int nadd_cands = 0;
		int ndrop_cands = 0;

		for(int i = 0; i < ns; ++i){
			if(!is_active[i]){
				free_idx[nf++] = i;
			}
		}
		if(nf <= 0){
			goto cleanup;
		}
		if(trace){
			fprintf(stderr, "  it=%d nf=%d active={", it, nf);
			{
				int first = 1;
				for(int i = 0; i < ns; ++i){
					if(is_active[i]){
						fprintf(stderr, "%s%d", first ? "" : ",", i);
						first = 0;
					}
				}
			}
			fprintf(stderr, "}\n");
		}
		for(int e = 0; e < ne; ++e){
			double rhs = b[e];
			for(int i = 0; i < ns; ++i){
				if(is_active[i]){
					rhs -= A[e * ns + i] * pin;
				}
			}
			b_f[e] = rhs;
		}
		for(int j = 0; j < nf; ++j){
			int i = free_idx[j];
			double ni = n_work[i];
			if(!(ni > n_floor) || !isfinite(ni)){
				ni = fmax(10.0 * n_floor, 1e-30);
			}
			names_f[j] = names[i];
			init_f[j] = ni;
			for(int e = 0; e < ne; ++e){
				A_f[e * nf + j] = A[e * ns + i];
			}
		}
		status = eqm_reduced_solve_source_init_once(names_f, nf, ne, A_f, b_f, source, T, P,
			init_f, n_floor, n_f);
		if(status != 0){
			status = eqm_reduced_solve_source_init_once(names_f, nf, ne, A_f, b_f, source, T, P,
				NULL, n_floor, n_f);
		}
		if(status != 0){
			int have_best = 0;
			int best_idx = -1;
			int best_make_active = 0;
			int best_kkt_ok = 0;
			double best_obj = HUGE_VAL;
			double best_max_free_resid = HUGE_VAL;
			if(eqm_reduced_eval_obj_mu(n_work, mu0, is_condensed, ns, T, P, P0, &obj_dummy, mu, NULL)
					&& eqm_reduced_eval_reduced_gradients(mu, A, ns, ne, is_active, T, red)){
				double worst_red = 0.0;
				for(int i = 0; i < ns; ++i){
					if(is_active[i] && red[i] < -dual_tol && red[i] < worst_red){
						worst_red = red[i];
						drop_idx = i;
					}
					if(is_active[i] && red[i] < -dual_tol){
						eqm_active_set_push_candidate(i, -red[i], max_candidates,
							&ndrop_cands, drop_cands, drop_cand_scores);
					}
				}
			}
			if(drop_idx < 0){
				double nmax = -HUGE_VAL;
				for(int i = 0; i < ns; ++i){
					if(is_active[i] && n_work[i] > nmax){
						nmax = n_work[i];
						drop_idx = i;
					}
				}
			}
			eqm_active_set_push_candidate(drop_idx, drop_idx >= 0 ? HUGE_VAL : 0.0, max_candidates,
				&ndrop_cands, drop_cands, drop_cand_scores);
			for(int ci = 0; ci < ndrop_cands; ++ci){
				int idx = drop_cands[ci];
				double cand_obj;
				double cand_max_free_resid;
				int cand_kkt_ok = 0;
				if(idx < 0){
					continue;
				}
				is_active[idx] = 0;
				if(eqm_reduced_active_set_trial(names, ns, ne, A, b, source, T, P, mu0,
						is_condensed, is_active, n_work, n_floor, n_trial_best, mu_cand, red_cand,
						&cand_obj, &cand_max_free_resid, &cand_kkt_ok, free_idx, names_f, A_f, b_f,
						n_f, init_f)
						&& eqm_active_set_trial_better(have_best, cand_kkt_ok, cand_obj,
							cand_max_free_resid, best_kkt_ok, best_obj, best_max_free_resid)){
					have_best = 1;
					best_idx = idx;
					best_make_active = 0;
					best_kkt_ok = cand_kkt_ok;
					best_obj = cand_obj;
					best_max_free_resid = cand_max_free_resid;
					for(int i = 0; i < ns; ++i){
						n_trial[i] = n_trial_best[i];
						is_active_best[i] = is_active[i];
					}
				}
				is_active[idx] = 1;
			}
			if(have_best){
				for(int i = 0; i < ns; ++i){
					n_work[i] = n_trial[i];
					is_active[i] = is_active_best[i];
				}
				if(trace){
					fprintf(stderr,
						"    scored pivot: %s species %d (kkt=%d obj=%.12g max_free=%.3e)\n",
						best_make_active ? "add" : "drop", best_idx, best_kkt_ok, best_obj,
						best_max_free_resid);
				}
				continue;
			}
			if(drop_idx < 0){
				goto cleanup;
			}
			if(trace){
				fprintf(stderr, "    free solve failed -> drop active species %d\n", drop_idx);
			}
			is_active[drop_idx] = 0;
			continue;
		}
		for(int i = 0; i < ns; ++i){
			n_trial[i] = is_active[i] ? pin : 0.0;
		}
		for(int j = 0; j < nf; ++j){
			n_trial[free_idx[j]] = n_f[j];
		}
		if(!eqm_reduced_eval_obj_mu(n_trial, mu0, is_condensed, ns, T, P, P0,
				&obj_dummy, mu, NULL)){
			goto cleanup;
		}
		if(!eqm_reduced_eval_reduced_gradients(mu, A, ns, ne, is_active, T, red)){
			goto cleanup;
		}

		for(int i = 0; i < ns; ++i){
			if(is_active[i]){
				if(red[i] < -dual_tol && -red[i] > drop_score){
					drop_score = -red[i];
					drop_idx = i;
				}
				if(red[i] < -dual_tol){
					eqm_active_set_push_candidate(i, -red[i], max_candidates,
						&ndrop_cands, drop_cands, drop_cand_scores);
				}
			}else{
				double ar = fabs(red[i]);
				if(ar > max_free_resid){
					max_free_resid = ar;
				}
				if(n_trial[i] <= active_n_cut && red[i] > dual_tol && red[i] > add_score){
					add_score = red[i];
					add_idx = i;
				}
				if(n_trial[i] <= active_n_cut && red[i] > dual_tol){
					eqm_active_set_push_candidate(i, red[i], max_candidates,
						&nadd_cands, add_cands, add_cand_scores);
				}
			}
		}
		if(trace){
			fprintf(stderr,
				"    resid: max_free=%.3e add=%d(%.3e) drop=%d(%.3e)\n",
				max_free_resid, add_idx, add_score, drop_idx, drop_score);
		}
		for(int i = 0; i < ns; ++i){
			n_work[i] = n_trial[i];
		}
		if(add_idx < 0 && drop_idx < 0 && max_free_resid <= free_tol){
			for(int i = 0; i < ns; ++i){
				n_seed_out[i] = n_work[i];
			}
			if(trace){
				fprintf(stderr, "  active-set converged (KKT) in %d iter\n", it + 1);
			}
			ok = 1;
			goto cleanup;
		}
		if(nadd_cands > 0 || ndrop_cands > 0){
			int have_best = 0;
			int best_idx = -1;
			int best_make_active = 0;
			int best_kkt_ok = 0;
			double best_obj = HUGE_VAL;
			double best_max_free_resid = HUGE_VAL;
			for(int ci = 0; ci < nadd_cands; ++ci){
				int idx = add_cands[ci];
				double cand_obj;
				double cand_max_free_resid;
				int cand_kkt_ok = 0;
				if(idx < 0){
					continue;
				}
				is_active[idx] = 1;
				if(eqm_reduced_active_set_trial(names, ns, ne, A, b, source, T, P, mu0,
						is_condensed, is_active, n_trial, n_floor, n_trial_best, mu_cand, red_cand,
						&cand_obj, &cand_max_free_resid, &cand_kkt_ok, free_idx, names_f, A_f, b_f,
						n_f, init_f)
						&& eqm_active_set_trial_better(have_best, cand_kkt_ok, cand_obj,
							cand_max_free_resid, best_kkt_ok, best_obj, best_max_free_resid)){
					have_best = 1;
					best_idx = idx;
					best_make_active = 1;
					best_kkt_ok = cand_kkt_ok;
					best_obj = cand_obj;
					best_max_free_resid = cand_max_free_resid;
					for(int i = 0; i < ns; ++i){
						n_work[i] = n_trial_best[i];
						is_active_best[i] = is_active[i];
					}
				}
				is_active[idx] = 0;
			}
			for(int ci = 0; ci < ndrop_cands; ++ci){
				int idx = drop_cands[ci];
				double cand_obj;
				double cand_max_free_resid;
				int cand_kkt_ok = 0;
				if(idx < 0){
					continue;
				}
				is_active[idx] = 0;
				if(eqm_reduced_active_set_trial(names, ns, ne, A, b, source, T, P, mu0,
						is_condensed, is_active, n_trial, n_floor, n_trial_best, mu_cand, red_cand,
						&cand_obj, &cand_max_free_resid, &cand_kkt_ok, free_idx, names_f, A_f, b_f,
						n_f, init_f)
						&& eqm_active_set_trial_better(have_best, cand_kkt_ok, cand_obj,
							cand_max_free_resid, best_kkt_ok, best_obj, best_max_free_resid)){
					have_best = 1;
					best_idx = idx;
					best_make_active = 0;
					best_kkt_ok = cand_kkt_ok;
					best_obj = cand_obj;
					best_max_free_resid = cand_max_free_resid;
					for(int i = 0; i < ns; ++i){
						n_work[i] = n_trial_best[i];
						is_active_best[i] = is_active[i];
					}
				}
				is_active[idx] = 1;
			}
			if(have_best){
				for(int i = 0; i < ns; ++i){
					is_active[i] = is_active_best[i];
				}
				if(best_kkt_ok && best_max_free_resid <= free_tol){
					for(int i = 0; i < ns; ++i){
						n_seed_out[i] = n_work[i];
					}
					if(trace){
						fprintf(stderr,
							"  active-set converged via scored pivot: %s species %d\n",
							best_make_active ? "add" : "drop", best_idx);
					}
					ok = 1;
					goto cleanup;
				}
				if(trace){
					fprintf(stderr,
						"    scored pivot: %s species %d (kkt=%d obj=%.12g max_free=%.3e)\n",
						best_make_active ? "add" : "drop", best_idx, best_kkt_ok, best_obj,
						best_max_free_resid);
				}
				for(int i = 0; i < ns; ++i){
					is_active[i] = is_active_best[i];
				}
				continue;
			}
		}
		if(add_idx >= 0 && drop_idx >= 0){
			if(add_score >= drop_score){
				drop_idx = -1;
			}else{
				add_idx = -1;
			}
		}
		if(add_idx >= 0){
			if(trace){
				fprintf(stderr, "    pivot: add species %d to active\n", add_idx);
			}
			is_active[add_idx] = 1;
		}else if(drop_idx >= 0){
			if(trace){
				fprintf(stderr, "    pivot: drop species %d from active\n", drop_idx);
			}
			is_active[drop_idx] = 0;
		}else{
			for(int i = 0; i < ns; ++i){
				n_seed_out[i] = n_work[i];
			}
			if(trace){
				fprintf(stderr, "  active-set accepted without pivot in %d iter\n", it + 1);
			}
			ok = 1;
			goto cleanup;
		}
	}
	if(trace){
		fprintf(stderr, "  active-set seed exhausted max_iter=%d\n", max_iter);
	}

cleanup:
	free(red);
	free(red_cand);
	free(mu);
	free(mu_cand);
	free(mu0);
	free(n_trial_best);
	free(n_trial);
	free(n_work);
	free(init_f);
	free(n_f);
	free(b_f);
	free(A_f);
	free(names_f);
	free(free_idx);
	free(is_active_best);
	free(is_active);
	free(is_condensed);
	return ok;
}

static int eqm_validate_solution_bounds(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, const double *n_out){
	const double elem_tol = 1e-6;
	const double free_tol = EQM_BOUND_KKT_FREE_TOL;
	const double dual_tol = EQM_BOUND_KKT_DUAL_TOL;
	const double P0 = 1e5;
	const int trace = eqm_active_trace_enabled();
	double n_tot = 0.0;
	double n_active_cutoff;
	double n_small_cutoff;
	double *mu0 = NULL;
	int *is_condensed = NULL;
	int *solution_phase_id = NULL;
	int *solution_member_index = NULL;
	EqmBinaryPhaseMeta *binary_phases = NULL;
	int nbinary_phases = 0;
	double *mu = NULL;
	double *red = NULL;
	int *is_active = NULL;
	int nfree = 0;
	int nactive = 0;

	if(!names || !A || !b || !n_out || ns <= 0 || ne <= 0 || !(T > 0.0) || !(P > 0.0)){
		return 0;
	}

	for(int i = 0; i < ns; ++i){
		if(!isfinite(n_out[i]) || n_out[i] <= 0.0){
			return 0;
		}
		n_tot += n_out[i];
	}
	if(!isfinite(n_tot) || n_tot <= 0.0){
		return 0;
	}

	for(int e = 0; e < ne; ++e){
		double lhs = 0.0;
		double resid;
		double denom = fabs(b[e]);
		if(denom < 1.0){
			denom = 1.0;
		}
		for(int i = 0; i < ns; ++i){
			lhs += A[e * ns + i] * n_out[i];
		}
		resid = lhs - b[e];
		if(!isfinite(resid) || fabs(resid) > elem_tol * denom){
			return 0;
		}
	}

	mu0 = (double *)calloc((size_t)ns, sizeof(double));
	is_condensed = (int *)calloc((size_t)ns, sizeof(int));
	mu = (double *)calloc((size_t)ns, sizeof(double));
	red = (double *)calloc((size_t)ns, sizeof(double));
	is_active = (int *)calloc((size_t)ns, sizeof(int));
	if(!mu0 || !is_condensed || !mu || !red || !is_active){
		free(mu0);
		free(is_condensed);
		free(mu);
		free(red);
		free(is_active);
		return 0;
	}
	if(!eqm_compute_solution_phases(names, ns, source, &solution_phase_id, &solution_member_index,
			&binary_phases, &nbinary_phases)){
		free(mu0);
		free(is_condensed);
		free(mu);
		free(red);
		free(is_active);
		return 0;
	}
	if(!eqm_compute_mu0(names, ns, source, T, P0, mu0)){
		free(mu0);
		free(is_condensed);
		free(mu);
		free(red);
		free(is_active);
		eqm_free_solution_phases(&solution_phase_id, &solution_member_index, &binary_phases);
		return 0;
	}
	if(!eqm_compute_is_condensed(names, ns, source, is_condensed)){
		free(mu0);
		free(is_condensed);
		free(mu);
		free(red);
		free(is_active);
		eqm_free_solution_phases(&solution_phase_id, &solution_member_index, &binary_phases);
		return 0;
	}
	if(!eqm_eval_obj_mu(n_out, mu0, is_condensed, solution_phase_id, binary_phases,
			nbinary_phases, ns, T, P, P0, NULL, mu, NULL)){
		free(mu0);
		free(is_condensed);
		free(mu);
		free(red);
		free(is_active);
		eqm_free_solution_phases(&solution_phase_id, &solution_member_index, &binary_phases);
		return 0;
	}

	n_active_cutoff = fmax(1e-60, EQM_BOUND_ACTIVE_CUTOFF_FRAC * n_tot);
	n_small_cutoff = fmax(n_active_cutoff, EQM_BOUND_ACTIVE_SMALL_FRAC * n_tot);
	if(!eqm_reduced_eval_reduced_gradients(mu, A, ns, ne, is_active, T, red)){
		free(mu0);
		free(is_condensed);
		free(mu);
		free(red);
		free(is_active);
			eqm_free_solution_phases(&solution_phase_id, &solution_member_index, &binary_phases);
			return 0;
	}
	eqm_bound_classify_active_free(n_out, red, ns, dual_tol, is_active, &nactive, &nfree,
		&n_active_cutoff, &n_small_cutoff);
	if(trace){
		fprintf(stderr, "eqm validate bounds preclass: T=%.6g P=%.6g nsmall=%.3e nactive=%d nfree=%d\n",
			T, P, n_small_cutoff, nactive, nfree);
		for(int i = 0; i < ns; ++i){
			fprintf(stderr, "  %s n=%.3e red0=%.3e active=%d\n",
				names[i] ? names[i] : "?", n_out[i], red[i], is_active[i]);
		}
	}
	if(nactive == 0 || nfree <= 0){
		free(mu0);
		free(is_condensed);
		free(mu);
		free(red);
		free(is_active);
		eqm_free_solution_phases(&solution_phase_id, &solution_member_index, &binary_phases);
		return 0;
	}
	if(!eqm_reduced_eval_reduced_gradients(mu, A, ns, ne, is_active, T, red)){
		free(mu0);
		free(is_condensed);
		free(mu);
		free(red);
		free(is_active);
		eqm_free_solution_phases(&solution_phase_id, &solution_member_index, &binary_phases);
		return 0;
	}

	for(int i = 0; i < ns; ++i){
		if(is_active[i]){
			if(red[i] < -dual_tol){
				if(trace){
					fprintf(stderr, "eqm validate bounds fail active %s red=%.3e\n",
						names[i] ? names[i] : "?", red[i]);
				}
				free(mu0);
				free(is_condensed);
				free(mu);
				free(red);
				free(is_active);
				eqm_free_solution_phases(&solution_phase_id, &solution_member_index, &binary_phases);
				return 0;
			}
		}else{
			if(fabs(red[i]) > free_tol){
				if(trace){
					fprintf(stderr, "eqm validate bounds fail free %s red=%.3e\n",
						names[i] ? names[i] : "?", red[i]);
				}
				free(mu0);
				free(is_condensed);
				free(mu);
				free(red);
				free(is_active);
				eqm_free_solution_phases(&solution_phase_id, &solution_member_index, &binary_phases);
				return 0;
			}
		}
	}
	if(trace){
		fprintf(stderr, "eqm validate bounds accepted\n");
	}

	free(mu0);
	free(is_condensed);
	free(mu);
	free(red);
	free(is_active);
	eqm_free_solution_phases(&solution_phase_id, &solution_member_index, &binary_phases);
	return 1;
}

static int eqm_reduced_condensed_candidate_seed(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, const double *n_hint,
		double n_floor, double *n_seed_out, int *kkt_ok_out){
	const int max_condensed_enum = 12;
	const double pin = n_floor;
	const double P0 = 1e5;
	const int trace = eqm_active_trace_enabled();
	int *is_condensed = NULL;
	double *mu0 = NULL;
	int *cond_idx = NULL;
	int *is_active = NULL;
	int *free_idx = NULL;
	const char **names_f = NULL;
	double *A_f = NULL;
	double *b_f = NULL;
	double *n_f = NULL;
	double *init_f = NULL;
	double *n_trial = NULL;
	double *n_best = NULL;
	int nc = 0;
	int ok = 0;
	int best_kkt = 0;
	double best_obj = 0.0;
	unsigned long long nmask = 0ULL;

	if(kkt_ok_out){
		*kkt_ok_out = 0;
	}
	if(!names || !A || !b || !n_seed_out || ns <= 1 || ne <= 0 || !(T > 0.0) || !(P > 0.0)){
		return 0;
	}

	is_condensed = (int *)calloc((size_t)ns, sizeof(int));
	mu0 = (double *)calloc((size_t)ns, sizeof(double));
	cond_idx = (int *)calloc((size_t)ns, sizeof(int));
	is_active = (int *)calloc((size_t)ns, sizeof(int));
	free_idx = (int *)calloc((size_t)ns, sizeof(int));
	names_f = (const char **)calloc((size_t)ns, sizeof(const char *));
	A_f = (double *)calloc((size_t)(ne * ns), sizeof(double));
	b_f = (double *)calloc((size_t)ne, sizeof(double));
	n_f = (double *)calloc((size_t)ns, sizeof(double));
	init_f = (double *)calloc((size_t)ns, sizeof(double));
	n_trial = (double *)calloc((size_t)ns, sizeof(double));
	n_best = (double *)calloc((size_t)ns, sizeof(double));
	if(!is_condensed || !mu0 || !cond_idx || !is_active || !free_idx || !names_f
			|| !A_f || !b_f || !n_f || !init_f || !n_trial || !n_best){
		goto cleanup;
	}

	if(!eqm_compute_mu0(names, ns, source, T, P0, mu0)){
		goto cleanup;
	}
	if(!eqm_compute_is_condensed(names, ns, source, is_condensed)){
		goto cleanup;
	}
	for(int i = 0; i < ns; ++i){
		if(is_condensed[i]){
			cond_idx[nc++] = i;
		}
	}
	if(nc <= 0){
		goto cleanup;
	}
	if(nc > max_condensed_enum){
		if(trace){
			fprintf(stderr, "eqm condensed-candidate: skip nc=%d > %d\n", nc, max_condensed_enum);
		}
		goto cleanup;
	}

	nmask = 1ULL << nc;
	if(trace){
		fprintf(stderr, "eqm condensed-candidate: trying %llu masks at T=%.6g P=%.6g\n",
			nmask, T, P);
	}
	for(unsigned long long mask = 0ULL; mask < nmask; ++mask){
		int nf = 0;
		int status;
		int kkt_ok = 0;
		double obj = 0.0;

		for(int i = 0; i < ns; ++i){
			is_active[i] = 0;
		}
		for(int c = 0; c < nc; ++c){
			int i = cond_idx[c];
			int present = ((mask >> c) & 1ULL) ? 1 : 0;
			if(!present){
				is_active[i] = 1;
			}
		}
		for(int i = 0; i < ns; ++i){
			if(!is_active[i]){
				free_idx[nf++] = i;
			}
		}
		if(nf <= 0){
			continue;
		}

		for(int e = 0; e < ne; ++e){
			double rhs = b[e];
			for(int i = 0; i < ns; ++i){
				if(is_active[i]){
					rhs -= A[e * ns + i] * pin;
				}
			}
			b_f[e] = rhs;
		}
		for(int j = 0; j < nf; ++j){
			int i = free_idx[j];
			double ni = (n_hint && n_hint[i] > n_floor && isfinite(n_hint[i]))
				? n_hint[i] : fmax(10.0 * n_floor, 1e-30);
			names_f[j] = names[i];
			init_f[j] = ni;
			for(int e = 0; e < ne; ++e){
				A_f[e * nf + j] = A[e * ns + i];
			}
		}

		status = eqm_reduced_solve_source_init_once(names_f, nf, ne, A_f, b_f, source, T, P,
			init_f, n_floor, n_f);
		if(status != 0){
			status = eqm_reduced_solve_source_init_once(names_f, nf, ne, A_f, b_f, source, T, P,
				NULL, n_floor, n_f);
		}
		if(status != 0){
			continue;
		}

		for(int i = 0; i < ns; ++i){
			n_trial[i] = is_active[i] ? pin : 0.0;
		}
		for(int j = 0; j < nf; ++j){
			n_trial[free_idx[j]] = n_f[j];
		}
		for(int i = 0; i < ns; ++i){
			if(!(n_trial[i] > 0.0) || !isfinite(n_trial[i])){
				status = -13;
				break;
			}
		}
		if(status != 0){
			continue;
		}
		if(!eqm_reduced_eval_obj_mu(n_trial, mu0, is_condensed, ns, T, P, P0, &obj, NULL, NULL)){
			continue;
		}
		kkt_ok = eqm_validate_solution_bounds(names, ns, ne, A, b, source, T, P, n_trial);

		if(!ok
				|| (kkt_ok && !best_kkt)
				|| (kkt_ok == best_kkt && obj < best_obj)){
			for(int i = 0; i < ns; ++i){
				n_best[i] = n_trial[i];
			}
			best_obj = obj;
			best_kkt = kkt_ok;
			ok = 1;
		}
	}
	if(ok){
		for(int i = 0; i < ns; ++i){
			n_seed_out[i] = n_best[i];
		}
		if(kkt_ok_out){
			*kkt_ok_out = best_kkt;
		}
		if(trace){
			fprintf(stderr, "eqm condensed-candidate: selected %s candidate\n",
				best_kkt ? "KKT-valid" : "non-KKT");
		}
	}

cleanup:
	free(n_best);
	free(n_trial);
	free(init_f);
	free(n_f);
	free(b_f);
	free(A_f);
	free(names_f);
	free(free_idx);
	free(is_active);
	free(cond_idx);
	free(mu0);
	free(is_condensed);
	return ok;
}

static int eqm_reduced_solve_source_init(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, const double *n_init,
		double *n_out){
	static const double floor_knots[] = {
		1e-12, 1e-18, 1e-24, 1e-30, 1e-40, 1e-50, 1e-60, 1e-80, 1e-100, 1e-120
	};
	double schedule[16];
	int nsteps = 0;
	double *n_seed = NULL;
	double *n_work = NULL;
	double *n_polish = NULL;
	double *n_bootstrap = NULL;
	const double *init = n_init;
	const double n_floor_target = 1e-120;
	int status = -13;

	if(!names || !A || !b || !n_out || ns <= 0 || ne <= 0 || !(T > 0.0) || !(P > 0.0)){
		return -11;
	}
	status = eqm_reduced_solve_source_init_once(names, ns, ne, A, b, source, T, P, n_init,
		n_floor_target, n_out);
	if(status == 0){
		return 0;
	}

	{
		/* Relative continuation ladder, avoids hardwired absolute temperature knots. */
		double Tk = 2.5 * T;
		int guard = 0;
		while(Tk > T + 1e-9 && nsteps < (int)(sizeof(schedule) / sizeof(schedule[0])) - 1){
			schedule[nsteps++] = Tk;
			Tk *= 0.82;
			if(++guard > 64){
				break;
			}
		}
	}
	schedule[nsteps++] = T;

	n_seed = (double *)calloc((size_t)ns, sizeof(double));
	n_work = (double *)calloc((size_t)ns, sizeof(double));
	n_polish = (double *)calloc((size_t)ns, sizeof(double));
	n_bootstrap = (double *)calloc((size_t)ns, sizeof(double));
	if(!n_seed || !n_work || !n_polish || !n_bootstrap){
		free(n_seed);
		free(n_work);
		free(n_polish);
		free(n_bootstrap);
		return status;
	}
	eqm_fill_n_est(A, b, ne, ns, n_init, n_bootstrap);

	for(int s = 0; s < nsteps; ++s){
		double Tk = schedule[s];
		int any_ok = 0;
		for(size_t fk = 0; fk < sizeof(floor_knots) / sizeof(floor_knots[0]); ++fk){
			double nf = floor_knots[fk];
			const double *step_init = init ? init : n_bootstrap;
			if(nf < n_floor_target){
				nf = n_floor_target;
			}
			status = eqm_reduced_solve_source_init_once(names, ns, ne, A, b, source,
				Tk, P, step_init, nf, n_work);
			if(status != 0 && step_init != NULL && step_init != init){
				status = eqm_reduced_solve_source_init_once(names, ns, ne, A, b, source,
					Tk, P, NULL, nf, n_work);
			}
			if(status != 0 && init != NULL){
				status = eqm_reduced_solve_source_init_once(names, ns, ne, A, b, source,
					Tk, P, NULL, nf, n_work);
			}
			if(status != 0){
				const double *seed_hint = step_init ? step_init : n_init;
				if(seed_hint != NULL
						&& eqm_reduced_active_set_seed(names, ns, ne, A, b, source, Tk, P,
							seed_hint, nf, n_work)){
					status = eqm_reduced_solve_source_init_once(names, ns, ne, A, b, source,
						Tk, P, n_work, nf, n_polish);
					if(status == 0){
						for(int i = 0; i < ns; ++i){
							n_work[i] = n_polish[i];
						}
					}else if(eqm_validate_solution_bounds(names, ns, ne, A, b, source, Tk, P, n_work)){
						status = 0;
					}
				}
				if(status != 0){
					int cand_kkt_ok = 0;
					const double *hint = step_init ? step_init : n_init;
					if(eqm_reduced_condensed_candidate_seed(names, ns, ne, A, b, source, Tk, P,
								hint, nf, n_work, &cand_kkt_ok)){
						status = eqm_reduced_solve_source_init_once(names, ns, ne, A, b, source,
							Tk, P, n_work, nf, n_polish);
						if(status == 0){
							for(int i = 0; i < ns; ++i){
								n_work[i] = n_polish[i];
							}
						}else if(cand_kkt_ok
								|| eqm_validate_solution_bounds(names, ns, ne, A, b, source, Tk, P, n_work)){
							status = 0;
						}
					}
				}
			}
			if(status != 0){
				/* Small-floor continuation can fail near boundary optima; keep last good floor. */
				if(any_ok){
					continue;
				}
				continue;
			}
			any_ok = 1;
			for(int i = 0; i < ns; ++i){
				n_seed[i] = n_work[i];
			}
			init = n_seed;
		}
		if(!any_ok){
			free(n_seed);
			free(n_work);
			free(n_polish);
			free(n_bootstrap);
			return status;
		}
		for(int i = 0; i < ns; ++i){
			n_seed[i] = n_work[i];
		}
		init = n_seed;
	}
	for(int i = 0; i < ns; ++i){
		n_out[i] = n_work[i];
	}
	free(n_seed);
	free(n_work);
	free(n_polish);
	free(n_bootstrap);
	return 0;
}

#ifndef HAVE_IPOPT
int eqm_seed_from_nullspace_r1(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, double *n_seed){
	(void)names;
	(void)ns;
	(void)elements;
	(void)ne;
	(void)source;
	(void)b;
	(void)T;
	(void)P;
	(void)n_seed;
	return 0;
}
#endif

static int eqm_alg_auto(const char *algorithm){
	return !algorithm || !algorithm[0] || strcmp(algorithm, "auto") == 0;
}

static int eqm_alg_prefix(const char *algorithm, const char *prefix){
	size_t len = strlen(prefix);
	return algorithm && strncmp(algorithm, prefix, len) == 0;
}

static int eqm_alg_exact(const char *algorithm, const char *name){
	return algorithm && strcmp(algorithm, name) == 0;
}

static int eqm_alg_reduced(const char *algorithm){
	return algorithm && eqm_alg_prefix(algorithm, "reduced");
}

static int eqm_alg_auto_reduced(const char *algorithm){
	return eqm_alg_exact(algorithm, "auto_reduced");
}

static int eqm_alg_use_nullspace(const char *algorithm){
	if(eqm_alg_exact(algorithm, "auto_no_nullspace")
			|| eqm_alg_exact(algorithm, "no_nullspace")
			|| eqm_alg_auto_reduced(algorithm)
			|| eqm_alg_reduced(algorithm)){
		return 0;
	}
	if(eqm_alg_prefix(algorithm, "ipopt")){
		return 1;
	}
	if(eqm_alg_auto(algorithm)
			|| eqm_alg_exact(algorithm, "auto_nullspace")
			|| eqm_alg_exact(algorithm, "nullspace")
			|| eqm_alg_exact(algorithm, "ipopt_nullspace")){
		return 1;
	}
	return 0;
}

static int eqm_alg_nullspace_only(const char *algorithm){
	return eqm_alg_exact(algorithm, "nullspace")
		|| eqm_alg_exact(algorithm, "ipopt_nullspace");
}

static const char *eqm_alg_fallback(const char *algorithm){
	if(eqm_alg_exact(algorithm, "auto_nullspace")
			|| eqm_alg_exact(algorithm, "auto_no_nullspace")
			|| eqm_alg_exact(algorithm, "no_nullspace")){
		return "auto";
	}
	return algorithm;
}

#ifdef HAVE_IPOPT
static int eqm_status_ok_ipopt(int status){
	return status == 0 || status == 1 || status == 6;
}
#endif

#ifdef HAVE_NLOPT
static int eqm_status_ok_slsqp(int status){
	return status == 0;
}
#endif

#if defined(HAVE_IPOPT) || defined(HAVE_NLOPT)
static int eqm_validate_solution(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, const double *n_out){
	const double elem_tol = 1e-6;
	const double stat_tol = 1e-1;
	const double P0 = 1e5;
	double *mu0 = NULL;
	int *is_condensed = NULL;
	int *solution_phase_id = NULL;
	int *solution_member_index = NULL;
	EqmBinaryPhaseMeta *binary_phases = NULL;
	int nbinary_phases = 0;
	double *mu = NULL;
	double *Awork = NULL;
	double *N = NULL;
	int *pivots = NULL;
	int rank = 0;
	int r = 0;

	if(!names || !A || !b || !n_out || ns <= 0 || ne <= 0 || !(T > 0.0) || !(P > 0.0)){
		return 0;
	}

	for(int i = 0; i < ns; ++i){
		if(!isfinite(n_out[i]) || n_out[i] <= 0.0){
			MSG("eqm validate failed: invalid n[%d]=%.17g", i, n_out[i]);
			return 0;
		}
	}

	for(int e = 0; e < ne; ++e){
		double lhs = 0.0;
		double resid;
		double denom = fabs(b[e]);
		if(denom < 1.0){
			denom = 1.0;
		}
		for(int i = 0; i < ns; ++i){
			lhs += A[e * ns + i] * n_out[i];
		}
		resid = lhs - b[e];
		if(!isfinite(resid) || fabs(resid) > elem_tol * denom){
			MSG(
				"eqm validate failed: element residual e=%d lhs=%.17g rhs=%.17g resid=%.17g",
				e, lhs, b[e], resid);
			return 0;
		}
	}

	mu0 = (double *)calloc((size_t)ns, sizeof(double));
	is_condensed = (int *)calloc((size_t)ns, sizeof(int));
	mu = (double *)calloc((size_t)ns, sizeof(double));
	Awork = (double *)calloc((size_t)(ne * ns), sizeof(double));
	pivots = (int *)calloc((size_t)ne, sizeof(int));
	if(!mu0 || !is_condensed || !mu || !Awork || !pivots){
		free(mu0);
		free(is_condensed);
		free(mu);
		free(Awork);
		free(pivots);
		return 0;
	}
	if(!eqm_compute_solution_phases(names, ns, source, &solution_phase_id, &solution_member_index,
			&binary_phases, &nbinary_phases)){
		free(mu0);
		free(is_condensed);
		free(mu);
		free(Awork);
		free(pivots);
		return 0;
	}

	if(!eqm_compute_mu0(names, ns, source, T, P0, mu0)){
		free(mu0);
		free(is_condensed);
		free(mu);
		free(Awork);
		free(pivots);
		eqm_free_solution_phases(&solution_phase_id, &solution_member_index, &binary_phases);
		return 0;
	}
	if(!eqm_compute_is_condensed(names, ns, source, is_condensed)){
		free(mu0);
		free(is_condensed);
		free(mu);
		free(Awork);
		free(pivots);
		eqm_free_solution_phases(&solution_phase_id, &solution_member_index, &binary_phases);
		return 0;
	}
	if(!eqm_eval_obj_mu(n_out, mu0, is_condensed, solution_phase_id, binary_phases,
			nbinary_phases, ns, T, P, P0, NULL, mu, NULL)){
		MSG("eqm validate failed: invalid activity/mu state");
		free(mu0);
		free(is_condensed);
		free(mu);
		free(Awork);
		free(pivots);
		eqm_free_solution_phases(&solution_phase_id, &solution_member_index, &binary_phases);
		return 0;
	}

	for(int i = 0; i < ne * ns; ++i){
		Awork[i] = A[i];
	}
	eqm_rref(Awork, ne, ns, pivots, &rank);
	r = ns - rank;
	if(r > 0){
		N = (double *)calloc((size_t)(ns * r), sizeof(double));
		if(!N){
			free(mu0);
			free(is_condensed);
			free(mu);
			free(Awork);
			free(pivots);
			eqm_free_solution_phases(&solution_phase_id, &solution_member_index, &binary_phases);
			return 0;
		}
		eqm_fill_nullspace(Awork, ne, ns, pivots, rank, N, r);
		for(int j = 0; j < r; ++j){
			double normv = 0.0;
			double dot = 0.0;
			for(int i = 0; i < ns; ++i){
				double v = N[i * r + j];
				double av = fabs(v);
				if(av > normv){
					normv = av;
				}
				dot += v * mu[i];
			}
			if(normv > 0.0){
					double scaled = fabs(dot / normv) / (gas_R() * T);
				if(!isfinite(scaled) || scaled > stat_tol){
						MSG(
						"eqm validate failed: stationarity col=%d scaled=%.17g",
						j, scaled);
						free(N);
						free(mu0);
						free(is_condensed);
						free(mu);
						free(Awork);
						free(pivots);
						eqm_free_solution_phases(&solution_phase_id, &solution_member_index,
							&binary_phases);
						return 0;
				}
			}
		}
	}

	free(N);
	free(mu0);
	free(is_condensed);
	free(mu);
	free(Awork);
	free(pivots);
	eqm_free_solution_phases(&solution_phase_id, &solution_member_index, &binary_phases);
	return 1;
}
#endif

static int eqm_solution_valid(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, const double *n_out){
#if defined(HAVE_IPOPT) || defined(HAVE_NLOPT)
	if(eqm_validate_solution_bounds(names, ns, ne, A, b, source, T, P, n_out)){
		return 1;
	}
	return eqm_validate_solution(names, ns, ne, A, b, source, T, P, n_out);
#else
	(void)names;
	(void)ns;
	(void)ne;
	(void)A;
	(void)b;
	(void)source;
	(void)T;
	(void)P;
	(void)n_out;
	return 1;
#endif
}

#ifdef HAVE_IPOPT
static int eqm_try_ipopt(const char **names, int ns, int ne, const double *A, const double *b,
		const char *source, double T, double P, const char *algorithm, const double *n_init,
		double *n_out){
	int status;
	if(algorithm && strstr(algorithm, "logn")){
		status = eqm_ipopt_solve_logn_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
		if(eqm_status_ok_ipopt(status)
				&& eqm_solution_valid(names, ns, ne, A, b, source, T, P, n_out)){
			return status;
		}
		if(eqm_status_ok_ipopt(status)){
			return -13;
		}
		return status;
	}
	if(algorithm && strstr(algorithm, "_n")){
		status = eqm_ipopt_solve_n_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
		if(eqm_status_ok_ipopt(status)
				&& eqm_solution_valid(names, ns, ne, A, b, source, T, P, n_out)){
			return status;
		}
		if(eqm_status_ok_ipopt(status)){
			return -13;
		}
		return status;
	}
	status = eqm_ipopt_solve_logn_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
	if(eqm_status_ok_ipopt(status)
			&& eqm_solution_valid(names, ns, ne, A, b, source, T, P, n_out)){
		return status;
	}
	status = eqm_ipopt_solve_n_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
	if(eqm_status_ok_ipopt(status)
			&& eqm_solution_valid(names, ns, ne, A, b, source, T, P, n_out)){
		return status;
	}
	status = eqm_ipopt_solve_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
	if(eqm_status_ok_ipopt(status)
			&& eqm_solution_valid(names, ns, ne, A, b, source, T, P, n_out)){
		return status;
	}
	if(eqm_status_ok_ipopt(status)){
		return -13;
	}
	return status;
}
#endif

int eqm_solve(const char **names, int ns, int ne, const double *A, const double *b,
		const char *source, double T, double P, const char *algorithm, const double *n_init,
		double *n_out){
	int status = -99;
	int has_solution_phases;

	if(!names || !A || !b || !n_out || ns <= 0 || ne <= 0){
		return -11;
	}
	has_solution_phases = eqm_has_solution_phases(names, ns, source);
	if(eqm_alg_trace_enabled()){
		fprintf(stderr,
			"FPROPS_EQM_ALG_TRACE enter alg=%s T=%.17g P=%.17g has_solution_phases=%d\n",
			algorithm ? algorithm : "(null)", T, P, has_solution_phases);
	}
	if(has_solution_phases && eqm_alg_auto_reduced(algorithm)){
		if(eqm_alg_trace_enabled()){
			fprintf(stderr, "FPROPS_EQM_ALG_TRACE auto_reduced redirected to auto due to solution phases\n");
		}
		algorithm = "auto";
	}
	if(has_solution_phases && eqm_alg_reduced(algorithm)){
		return -12;
	}
	if(eqm_alg_auto_reduced(algorithm)){
		status = eqm_reduced_solve_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
		if(eqm_alg_trace_enabled()){
			fprintf(stderr, "FPROPS_EQM_ALG_TRACE auto_reduced reduced_status=%d\n", status);
		}
		if(status == 0 && eqm_solution_valid(names, ns, ne, A, b, source, T, P, n_out)){
			if(eqm_alg_trace_enabled()){
				fprintf(stderr, "FPROPS_EQM_ALG_TRACE auto_reduced accepted reduced solution\n");
			}
			return 0;
		}
		if(status == 0){
			if(eqm_alg_trace_enabled()){
				fprintf(stderr, "FPROPS_EQM_ALG_TRACE auto_reduced reduced solution rejected by validation\n");
			}
			status = -13;
		}
		if(eqm_alg_trace_enabled()){
			fprintf(stderr, "FPROPS_EQM_ALG_TRACE auto_reduced falling back to auto\n");
		}
		algorithm = "auto";
	}
	if(eqm_alg_reduced(algorithm)){
		status = eqm_reduced_solve_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
		if(status == 0 && eqm_solution_valid(names, ns, ne, A, b, source, T, P, n_out)){
			return 0;
		}
		if(status == 0){
			return -13;
		}
		return status;
	}
	if(eqm_alg_auto(algorithm)){
#ifdef HAVE_IPOPT
		status = eqm_try_ipopt(names, ns, ne, A, b, source, T, P, NULL, n_init, n_out);
		if(eqm_status_ok_ipopt(status)){
			return status;
		}
#endif
#ifdef HAVE_NLOPT
		status = eqm_slsqp_solve_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
		if(eqm_status_ok_slsqp(status)
				&& eqm_solution_valid(names, ns, ne, A, b, source, T, P, n_out)){
			return status;
		}
		if(eqm_status_ok_slsqp(status)){
			status = -13;
		}
#endif
		return status;
	}
#ifdef HAVE_IPOPT
	if(eqm_alg_prefix(algorithm, "ipopt")){
		status = eqm_try_ipopt(names, ns, ne, A, b, source, T, P, algorithm, n_init, n_out);
		if(eqm_status_ok_ipopt(status)){
			return status;
		}
		return status;
	}
#endif
#ifdef HAVE_NLOPT
	if(eqm_alg_prefix(algorithm, "slsqp")){
		status = eqm_slsqp_solve_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
		if(eqm_status_ok_slsqp(status)
				&& eqm_solution_valid(names, ns, ne, A, b, source, T, P, n_out)){
			return status;
		}
		if(eqm_status_ok_slsqp(status)){
			return -13;
		}
		return status;
	}
#endif
	return -12;
}

int eqm_solve_elements(const char **names, int ns, const char **elements, int ne,
		const double *b, const char *source, double T, double P, const char *algorithm,
		const double *n_init, double *n_out){
	double *A = NULL;
	double *A_use = NULL;
	double *b_use = NULL;
	int ne_use = 0;
	int status = -11;
	int has_solution_phases;

	if(!names || !elements || !b || !n_out || ns <= 0 || ne <= 0){
		return -11;
	}
	has_solution_phases = eqm_has_solution_phases(names, ns, source);
	A = (double *)calloc((size_t)(ne * ns), sizeof(double));
	if(!A){
		return -11;
	}
	if(!fprops_build_element_matrix_source(names, ns, elements, ne, source, A)){
		free(A);
		return -11;
	}
	if(!eqm_augment_special_phase_constraints(names, ns, source, ne, A, b, &ne_use, &A_use, &b_use)){
		free(A);
		return -11;
	}
#ifdef HAVE_IPOPT
	if(!has_solution_phases && eqm_alg_use_nullspace(algorithm)){
		status = eqm_ipopt_nullspace_solve_source(names, ns, elements, ne, source, b, T, P, n_out);
		if(eqm_status_ok_ipopt(status)
				&& eqm_solution_valid(names, ns, ne, A, b, source, T, P, n_out)){
			free(A);
			return status;
		}
		if(eqm_alg_nullspace_only(algorithm)){
			free(A);
			if(eqm_status_ok_ipopt(status)){
				return -13;
			}
			return status;
		}
	}
	if(has_solution_phases && eqm_alg_nullspace_only(algorithm)){
		if(A_use != A){
			free(A_use);
		}
		if(b_use != b){
			free(b_use);
		}
		free(A);
		return -12;
	}
#endif
	status = eqm_solve(names, ns, ne_use, A_use, b_use, source, T, P,
		eqm_alg_fallback(algorithm), n_init, n_out);
	if(A_use != A){
		free(A_use);
	}
	if(b_use != b){
		free(b_use);
	}
	free(A);
	return status;
}

static int eqm_total_h_tpn(const char **names, int ns, const double *n, const char *source,
		double T, double P, double *H_out){
	double H_total = 0.0;
	int i;

	if(!names || !n || !H_out || ns <= 0 || !(T > 0.0) || !(P > 0.0)){
		return -11;
	}
	if(eqm_has_solution_phases(names, ns, source)){
		return -15;
	}
	for(i = 0; i < ns; ++i){
		double hi = 0.0;
		if(!(n[i] >= 0.0) || !isfinite(n[i])){
			return -13;
		}
		if(n[i] == 0.0){
			continue;
		}
		if(!eqm_h_source(names[i], source, T, P, &hi)){
			return -14;
		}
		H_total += n[i] * hi;
	}
	*H_out = H_total;
	return 0;
}

FpropsRxnPackage *fprops_rxn_package_build(const char **names, int ns, const char *source){
	FpropsRxnPackage *pkg = NULL;
	int i;

	if(!names || ns <= 0){
		ERR("rxn package build: invalid args names=%p ns=%d", (void *)names, ns);
		return NULL;
	}
	pkg = (FpropsRxnPackage *)calloc(1, sizeof(*pkg));
	if(!pkg){
		ERR("rxn package build: package allocation failed ns=%d", ns);
		return NULL;
	}
	pkg->ns = ns;
	pkg->names = (char **)calloc((size_t)ns, sizeof(char *));
	pkg->species = (FpropsRxnSpeciesCache *)calloc((size_t)ns, sizeof(FpropsRxnSpeciesCache));
	pkg->source = eqm_strdup_local(source ? source : "");
	if(!pkg->names || !pkg->species || (source && !pkg->source)){
		ERR("rxn package build: initial allocation failed ns=%d", ns);
		fprops_rxn_package_free(pkg);
		return NULL;
	}
	for(i = 0; i < ns; ++i){
		char resolved_name_buf[256];
		char source_buf[512];
		const char *name_i;
		const char *source_pref = source;
		const char *source_i;
		const BinarySolutionPhaseDef *phase = NULL;
		const FeSpinelPhaseDef *spinel = NULL;
		unsigned member_index = 0;
		EqmMuModel selector_model = EQM_MODEL_AUTO;
		int use_ref0 = 0;
		const char *selector_source = NULL;

		if(!names[i]){
			ERR("rxn package build: null species name at index %d", i);
			fprops_rxn_package_free(pkg);
			return NULL;
		}
		name_i = eqm_resolve_rxn_name(names[i], source, resolved_name_buf,
			(unsigned)sizeof(resolved_name_buf), &source_pref);
		pkg->names[i] = eqm_strdup_local(name_i);
		pkg->species[i].name = eqm_strdup_local(name_i);
		if(!pkg->names[i] || !pkg->species[i].name){
			fprops_rxn_package_free(pkg);
			return NULL;
		}

		source_i = fprops_resolve_species_source(source_pref, name_i, source_buf, (unsigned)sizeof(source_buf));
		pkg->species[i].source_resolved = eqm_strdup_local(source_i ? source_i : "");
		if(source_i && !pkg->species[i].source_resolved){
			ERR("rxn package build: failed to copy resolved source for '%s'", name_i);
			fprops_rxn_package_free(pkg);
			return NULL;
		}

		if(eqm_lookup_solution_member(name_i, source_pref, &phase, &member_index)){
			pkg->species[i].entry_kind = FPROPS_RXN_ENTRY_BINARY_SOLUTION_MEMBER;
			pkg->species[i].member_index = (int)member_index;
			continue;
		}
		if(eqm_lookup_spinel_member(name_i, source_pref, &spinel, &member_index)){
			pkg->species[i].entry_kind = FPROPS_RXN_ENTRY_SPINEL_MEMBER;
			pkg->species[i].member_index = (int)member_index;
			continue;
		}

		eqm_parse_selector(source_i, &selector_model, &use_ref0, &selector_source);
		if(!eqm_species_compile_thermo(name_i, selector_source,
				selector_model, use_ref0, &pkg->species[i].thermo)){
			ERR("rxn package build failed: no thermo model for '%s' (source='%s')",
				name_i, selector_source ? selector_source : "");
			fprops_rxn_package_free(pkg);
			return NULL;
		}
	}

	if(!fprops_collect_elements_source((const char **)pkg->names, ns, source, &pkg->elements, &pkg->ne) || pkg->ne <= 0){
		ERR("rxn package build: failed collecting elements for %d species", ns);
		fprops_rxn_package_free(pkg);
		return NULL;
	}
	pkg->A = (double *)calloc((size_t)(pkg->ne * ns), sizeof(double));
	pkg->is_condensed = (int *)calloc((size_t)ns, sizeof(int));
	if(!pkg->A || !pkg->is_condensed){
		ERR("rxn package build: allocation failed for A/is_condensed (ns=%d ne=%d)", ns, pkg->ne);
		fprops_rxn_package_free(pkg);
		return NULL;
	}
	if(!fprops_build_element_matrix_source((const char **)pkg->names, ns,
			(const char **)pkg->elements, pkg->ne, source, pkg->A)){
		ERR("rxn package build: failed building element matrix (ns=%d ne=%d)", ns, pkg->ne);
		fprops_rxn_package_free(pkg);
		return NULL;
	}
	if(!eqm_compute_is_condensed((const char **)pkg->names, ns, source, pkg->is_condensed)){
		ERR("rxn package build: failed classifying condensed species");
		fprops_rxn_package_free(pkg);
		return NULL;
	}
	if(!eqm_compute_solution_phases((const char **)pkg->names, ns, source,
			&pkg->solution_phase_id, &pkg->solution_member_index,
			&pkg->binary_phases, &pkg->nbinary_phases)){
		ERR("rxn package build: failed computing solution phase metadata");
		fprops_rxn_package_free(pkg);
		return NULL;
	}
	for(i = 0; i < ns; ++i){
		pkg->species[i].phase_id = pkg->solution_phase_id ? pkg->solution_phase_id[i] : -1;
		if(pkg->solution_member_index){
			pkg->species[i].member_index = pkg->solution_member_index[i];
		}
	}
	MSG("rxn package build: built package ns=%d ne=%d nbinary=%d", pkg->ns, pkg->ne, pkg->nbinary_phases);
	eqm_package_trace("package_build", pkg, NULL, (const char **)pkg->names, pkg->ns);
	return pkg;
}

int fprops_rxn_package_num_species(const FpropsRxnPackage *pkg){
	return pkg ? pkg->ns : 0;
}

int fprops_rxn_package_num_elements(const FpropsRxnPackage *pkg){
	return pkg ? pkg->ne : 0;
}

const double *fprops_rxn_package_element_matrix(const FpropsRxnPackage *pkg){
	return pkg ? pkg->A : NULL;
}

void fprops_rxn_package_free(FpropsRxnPackage *pkg){
	int i;
	if(!pkg){
		return;
	}
	eqm_package_trace("package_free", pkg, NULL, (const char **)pkg->names, pkg->ns);
	if(pkg->species){
		for(i = 0; i < pkg->ns; ++i){
			free(pkg->species[i].name);
			free(pkg->species[i].source_resolved);
			eqm_species_free_thermo(&pkg->species[i].thermo);
		}
		free(pkg->species);
	}
	if(pkg->names){
		for(i = 0; i < pkg->ns; ++i){
			free(pkg->names[i]);
		}
		free(pkg->names);
	}
	free(pkg->source);
	free(pkg->A);
	free(pkg->is_condensed);
	eqm_free_solution_phases(&pkg->solution_phase_id, &pkg->solution_member_index, &pkg->binary_phases);
	fprops_free_elements(&pkg->elements, &pkg->ne);
	free(pkg);
}

static int eqm_binary_solution_total_g(const EqmBinaryPhaseMeta *phase, const double *n,
		double T, double P, double *g_out){
	const BinarySolutionModel *M;
	double n_a;
	double n_b;
	double n_tot;
	double x;
	double g_molar;
	FpropsError err = FPROPS_NO_ERROR;

	if(!phase || !n || !g_out || !phase->phase || !phase->phase->model){
		return 0;
	}
	M = phase->phase->model;
	n_a = n[phase->ia];
	n_b = n[phase->ib];
	n_tot = n_a + n_b;
	if(!(n_tot >= 0.0) || !isfinite(n_tot)){
		return 0;
	}
	if(n_tot == 0.0){
		*g_out = 0.0;
		return 1;
	}
	x = n_b / n_tot;
	g_molar = solution_binary_g_molar(M, T, P, x, &err);
	if(err || !isfinite(g_molar)){
		return 0;
	}
	*g_out = n_tot * g_molar;
	return isfinite(*g_out);
}

static int eqm_spinel_total_g(const EqmBinaryPhaseMeta *phase, const double *n,
		double T, double P, double *g_out){
	double n_members[5];
	int j;

	if(!phase || !n || !g_out || !phase->spinel){
		return 0;
	}
	for(j = 0; j < 5; ++j){
		int idx = phase->members[j];
		if(idx < 0){
			return 0;
		}
		n_members[j] = n[idx];
		if(!(n_members[j] >= 0.0) || !isfinite(n_members[j])){
			return 0;
		}
	}
	return spinel_phase_eval(phase->spinel, n_members, T, P, g_out, NULL);
}

static int eqm_phase_total_h_fd(const EqmBinaryPhaseMeta *phase, const double *n,
		double T, double P, double *h_out){
	double dT;
	double g0 = 0.0;
	double gp = 0.0;
	double gm = 0.0;
	double dgdt;
	int ok0 = 0;
	int okp = 0;
	int okm = 0;

	if(!phase || !n || !h_out || !(T > 0.0) || !(P > 0.0)){
		return 0;
	}

	dT = fmax(1e-3, 1e-5 * T);
	if(T - dT <= 0.0){
		dT = 0.5 * T;
	}
	if(!(dT > 0.0)){
		return 0;
	}

	if(phase->kind == EQM_PHASE_BINARY_SOLUTION){
		ok0 = eqm_binary_solution_total_g(phase, n, T, P, &g0);
		okp = eqm_binary_solution_total_g(phase, n, T + dT, P, &gp);
		if(T - dT > 0.0){
			okm = eqm_binary_solution_total_g(phase, n, T - dT, P, &gm);
		}
	}else if(phase->kind == EQM_PHASE_FE_SPINEL){
		ok0 = eqm_spinel_total_g(phase, n, T, P, &g0);
		okp = eqm_spinel_total_g(phase, n, T + dT, P, &gp);
		if(T - dT > 0.0){
			okm = eqm_spinel_total_g(phase, n, T - dT, P, &gm);
		}
	}else{
		return 0;
	}

	if(!ok0 || !okp){
		return 0;
	}
	if(okm){
		dgdt = (gp - gm) / (2.0 * dT);
	}else{
		dgdt = (gp - g0) / dT;
	}
	if(!isfinite(dgdt)){
		return 0;
	}
	*h_out = g0 - T * dgdt;
	return isfinite(*h_out);
}

int fprops_rxn_mix_h(const FpropsRxnPackage *pkg, const FpropsRxnTPN *state, double *H_out){
	double H_total = 0.0;
	double h_phase = 0.0;
	int p;
	int i;

	if(!pkg || !state || !state->n || !H_out || pkg->ns <= 0 || !(state->T > 0.0) || !(state->P > 0.0)){
		ERR("rxn mix h: invalid args pkg=%p state=%p n=%p H_out=%p ns=%d T=%.17g P=%.17g",
			(void *)pkg, (void *)state, state ? (void *)state->n : NULL, (void *)H_out,
			pkg ? pkg->ns : -1, state ? state->T : NAN, state ? state->P : NAN);
		return -11;
	}
	for(i = 0; i < pkg->ns; ++i){
		if(!(state->n[i] >= 0.0) || !isfinite(state->n[i])){
			ERR("rxn mix h: invalid amount n[%d]=%.17g for '%s'", i, state->n[i],
				pkg->species && pkg->species[i].name ? pkg->species[i].name : "(null)");
			return -13;
		}
	}
	for(p = 0; p < pkg->nbinary_phases; ++p){
		if(!eqm_phase_total_h_fd(&pkg->binary_phases[p], state->n, state->T, state->P, &h_phase)){
			ERR("rxn mix h: solution/spinel enthalpy evaluation failed for phase %d at T=%.17g P=%.17g",
				p, state->T, state->P);
			return -15;
		}
		H_total += h_phase;
	}
	for(i = 0; i < pkg->ns; ++i){
		double hi = 0.0;
		if(state->n[i] == 0.0){
			continue;
		}
		if(pkg->solution_phase_id && pkg->solution_phase_id[i] >= 0){
			continue;
		}
		if(!eqm_h_from_compiled(&pkg->species[i], state->T, state->P, &hi)){
			ERR("rxn mix h: enthalpy evaluation failed for '%s' at T=%.17g P=%.17g",
				pkg->species && pkg->species[i].name ? pkg->species[i].name : "(null)",
				state->T, state->P);
			return -14;
		}
		H_total += state->n[i] * hi;
	}
	*H_out = H_total;
	return 0;
}

int fprops_rxn_mix_v(const FpropsRxnPackage *pkg, const FpropsRxnTPN *state, double *V_out){
	double V_total = 0.0;
	int i;

	if(!pkg || !state || !state->n || !V_out || pkg->ns <= 0 || !(state->T > 0.0) || !(state->P > 0.0)){
		ERR("rxn mix v: invalid args pkg=%p state=%p n=%p V_out=%p ns=%d T=%.17g P=%.17g",
			(void *)pkg, (void *)state, state ? (void *)state->n : NULL, (void *)V_out,
			pkg ? pkg->ns : -1, state ? state->T : NAN, state ? state->P : NAN);
		return -11;
	}
	for(i = 0; i < pkg->ns; ++i){
		if(!(state->n[i] >= 0.0) || !isfinite(state->n[i])){
			ERR("rxn mix v: invalid amount n[%d]=%.17g for '%s'", i, state->n[i],
				pkg->species && pkg->species[i].name ? pkg->species[i].name : "(null)");
			return -13;
		}
	}
	for(i = 0; i < pkg->ns; ++i){
		double vi = 0.0;
		if(state->n[i] == 0.0){
			continue;
		}
		if(pkg->solution_phase_id && pkg->solution_phase_id[i] >= 0){
			ERR("rxn mix v: solution-phase volume evaluation not implemented for '%s'",
				pkg->species && pkg->species[i].name ? pkg->species[i].name : "(null)");
			return -15;
		}
		if(!eqm_v_from_compiled(&pkg->species[i], state->T, state->P, &vi)){
			ERR("rxn mix v: volume evaluation failed for '%s' at T=%.17g P=%.17g",
				pkg->species && pkg->species[i].name ? pkg->species[i].name : "(null)",
				state->T, state->P);
			return -14;
		}
		V_total += state->n[i] * vi;
	}
	*V_out = V_total;
	return 0;
}

int fprops_rxn_eqm_tpy(const FpropsRxnPackage *pkg, const FpropsRxnTPN *state,
		const char *algorithm, const double *n_init, FpropsRxnResult *out){
	double *b = NULL;
	int e, i, status;

	if(!pkg || !state || !state->n || !out || !out->n_out || pkg->ns <= 0 || pkg->ne <= 0
			|| !(state->T > 0.0) || !(state->P > 0.0)){
		ERR("rxn eqm tpy: invalid args pkg=%p state=%p n=%p out=%p n_out=%p ns=%d ne=%d T=%.17g P=%.17g",
			(void *)pkg, (void *)state, state ? (void *)state->n : NULL,
			(void *)out, out ? (void *)out->n_out : NULL,
			pkg ? pkg->ns : -1, pkg ? pkg->ne : -1,
			state ? state->T : NAN, state ? state->P : NAN);
		return -11;
	}
	b = (double *)calloc((size_t)pkg->ne, sizeof(double));
	if(!b){
		ERR("rxn eqm tpy: unable to allocate element totals vector (ne=%d)", pkg->ne);
		return -12;
	}
	for(i = 0; i < pkg->ns; ++i){
		if(!(state->n[i] >= 0.0) || !isfinite(state->n[i])){
			ERR("rxn eqm tpy: invalid amount n[%d]=%.17g for '%s'", i, state->n[i],
				pkg->species && pkg->species[i].name ? pkg->species[i].name : "(null)");
			free(b);
			return -13;
		}
		for(e = 0; e < pkg->ne; ++e){
			b[e] += pkg->A[e * pkg->ns + i] * state->n[i];
		}
	}
	eqm_basis_trace_dump("rxn_tpy", (const char **)pkg->names, pkg->ns,
		(const char **)pkg->elements, pkg->ne, pkg->A, b, state->n, n_init,
		pkg->source, algorithm, state->T, state->P, (const void *)pkg);
	status = fprops_rxn_eqm_tpb(pkg, state, b, algorithm, n_init, out);
	free(b);
	return status;
}

int fprops_rxn_eqm_sensitivities(const FpropsRxnPackage *pkg, const FpropsRxnTPN *state,
		const double *n_eq, double *dn_dT, double *dn_dP, double *dn_db){
	const double P0 = 1e5;
	const double n_floor = 1e-120;
	const double R = gas_R();
	const int m = pkg ? pkg->ns + pkg->ne : 0;
	double *mu0 = NULL;
	double *h0 = NULL;
	double *K = NULL;
	double *Kwork = NULL;
	double *rhs = NULL;
	double ngas = 0.0;
	double RT;
	double logPP0;
	int i, j, e;
	int status = -11;

	if(!pkg || !state || !state->n || !n_eq || pkg->ns <= 0 || pkg->ne <= 0
			|| !(state->T > 0.0) || !(state->P > 0.0)){
		return -11;
	}
	if(pkg->nbinary_phases > 0){
		return -15;
	}
	for(i = 0; i < pkg->ns; ++i){
		if(!(n_eq[i] > 0.0) || !isfinite(n_eq[i])){
			return -13;
		}
		if(pkg->solution_phase_id && pkg->solution_phase_id[i] >= 0){
			return -15;
		}
		if(pkg->is_condensed && pkg->is_condensed[i]){
			return -15;
		}
		if(pkg->species[i].entry_kind != FPROPS_RXN_ENTRY_PURE){
			return -15;
		}
		ngas += n_eq[i];
	}
	if(!(ngas > 0.0) || !isfinite(ngas)){
		return -13;
	}

	mu0 = (double *)calloc((size_t)pkg->ns, sizeof(double));
	h0 = (double *)calloc((size_t)pkg->ns, sizeof(double));
	K = (double *)calloc((size_t)(m * m), sizeof(double));
	Kwork = (double *)calloc((size_t)(m * m), sizeof(double));
	rhs = (double *)calloc((size_t)m, sizeof(double));
	if(!mu0 || !h0 || !K || !Kwork || !rhs){
		status = -12;
		goto cleanup;
	}

	for(i = 0; i < pkg->ns; ++i){
		if(!eqm_mu0_from_compiled(&pkg->species[i], state->T, P0, &mu0[i])){
			status = -14;
			goto cleanup;
		}
		if(!eqm_h_from_compiled(&pkg->species[i], state->T, P0, &h0[i])){
			status = -14;
			goto cleanup;
		}
	}

	RT = R * state->T;
	logPP0 = log(state->P / P0);
	for(i = 0; i < pkg->ns; ++i){
		for(j = 0; j < pkg->ns; ++j){
			double nij = (i == j) ? 1.0 / fmax(n_eq[i], n_floor) : 0.0;
			K[i * m + j] = RT * (nij - (1.0 / ngas));
		}
		for(e = 0; e < pkg->ne; ++e){
			double aei = pkg->A[e * pkg->ns + i];
			K[i * m + (pkg->ns + e)] = aei;
			K[(pkg->ns + e) * m + i] = aei;
		}
	}

	if(dn_dT){
		memset(rhs, 0, sizeof(double) * (size_t)m);
		for(i = 0; i < pkg->ns; ++i){
			double logterm = log(fmax(n_eq[i], n_floor)) - log(ngas) + logPP0;
			double dmu_dT = (mu0[i] - h0[i]) / state->T + R * logterm;
			rhs[i] = -dmu_dT;
		}
		memcpy(Kwork, K, sizeof(double) * (size_t)(m * m));
		if(!eqm_dense_solve(Kwork, rhs, m)){
			status = -13;
			goto cleanup;
		}
		for(i = 0; i < pkg->ns; ++i){
			dn_dT[i] = rhs[i];
		}
	}

	if(dn_dP){
		memset(rhs, 0, sizeof(double) * (size_t)m);
		for(i = 0; i < pkg->ns; ++i){
			rhs[i] = -(RT / state->P);
		}
		memcpy(Kwork, K, sizeof(double) * (size_t)(m * m));
		if(!eqm_dense_solve(Kwork, rhs, m)){
			status = -13;
			goto cleanup;
		}
		for(i = 0; i < pkg->ns; ++i){
			dn_dP[i] = rhs[i];
		}
	}

	if(dn_db){
		for(e = 0; e < pkg->ne; ++e){
			memset(rhs, 0, sizeof(double) * (size_t)m);
			rhs[pkg->ns + e] = 1.0;
			memcpy(Kwork, K, sizeof(double) * (size_t)(m * m));
			if(!eqm_dense_solve(Kwork, rhs, m)){
				status = -13;
				goto cleanup;
			}
			for(i = 0; i < pkg->ns; ++i){
				dn_db[i * pkg->ne + e] = rhs[i];
			}
		}
	}

	status = 0;

cleanup:
	free(mu0);
	free(h0);
	free(K);
	free(Kwork);
	free(rhs);
	return status;
}

int fprops_rxn_eqm_tpb(const FpropsRxnPackage *pkg, const FpropsRxnTPN *state,
		const double *b, const char *algorithm, const double *n_init, FpropsRxnResult *out){
	int status;
	int ne_use = 0;
	double *A_use = NULL;
	double *b_use = NULL;
	const FpropsRxnPackage *old_pkg;
	if(!pkg || !state || !b || !out || !out->n_out || pkg->ns <= 0 || pkg->ne <= 0
			|| !(state->T > 0.0) || !(state->P > 0.0)){
		ERR("rxn eqm tpb: invalid args pkg=%p state=%p b=%p out=%p n_out=%p ns=%d ne=%d T=%.17g P=%.17g",
			(void *)pkg, (void *)state, (void *)b, (void *)out, out ? (void *)out->n_out : NULL,
			pkg ? pkg->ns : -1, pkg ? pkg->ne : -1, state ? state->T : NAN, state ? state->P : NAN);
		return -11;
	}
	old_pkg = eqm_package_scope_push(pkg);
	if(!eqm_augment_special_phase_constraints_meta(pkg->ns, pkg->binary_phases, pkg->nbinary_phases,
			pkg->ne, pkg->A, b, &ne_use, &A_use, &b_use)){
		ERR("rxn eqm tpb: failed augmenting special phase constraints (ns=%d ne=%d nbinary=%d)",
			pkg->ns, pkg->ne, pkg->nbinary_phases);
		eqm_package_scope_pop(old_pkg);
		return -11;
	}
	MSG("rxn eqm tpb: solving ns=%d ne=%d T=%.17g P=%.17g algorithm='%s'",
		pkg->ns, ne_use, state->T, state->P, algorithm ? algorithm : "");
	eqm_package_trace("eqm_tpb_enter", pkg, old_pkg, (const char **)pkg->names, pkg->ns);
	status = eqm_solve((const char **)pkg->names, pkg->ns, ne_use, A_use, b_use,
		pkg->source, state->T, state->P, eqm_alg_fallback(algorithm), n_init, out->n_out);
	if(A_use != pkg->A){
		free(A_use);
	}
	if(b_use != b){
		free(b_use);
	}
	eqm_package_trace("eqm_tpb_exit", pkg, old_pkg, (const char **)pkg->names, pkg->ns);
	eqm_package_scope_pop(old_pkg);
	out->status = status;
	out->H = NAN;
	out->G = NAN;
	if(status != 0 && status != 1 && status != 6){
		ERR("rxn eqm tpb: solver returned status %d", status);
	}
	if(status == 0 || status == 1 || status == 6){
		FpropsRxnTPN out_state = {state->T, state->P, out->n_out};
		if(0 == fprops_rxn_mix_h(pkg, &out_state, &out->H)){
			return status;
		}
		ERR("rxn eqm tpb: post-equilibrium enthalpy evaluation failed");
	}
	return status;
}

int fprops_eqm_tpb(const char **names, int ns, const char **elements, int ne,
		const double *b, const char *source, double T, double P, const char *algorithm,
		const double *n_init, double *n_out, double *H_out){
	int status;

	if(!names || !elements || !b || !n_out || ns <= 0 || ne <= 0 || !(T > 0.0) || !(P > 0.0)){
		return -11;
	}
	status = eqm_solve_elements(names, ns, elements, ne, b, source, T, P, algorithm, n_init, n_out);
	if(status != 0 && status != 1 && status != 6){
		return status;
	}
	if(H_out){
		int h_status = eqm_total_h_tpn(names, ns, n_out, source, T, P, H_out);
		if(h_status != 0){
			return h_status;
		}
	}
	return status;
}

int fprops_eqm_tpy(const char **names, int ns, const double *y_in, const char *source,
		double T, double P, const char *algorithm, const double *n_init, double *n_out){
	char **elements = NULL;
	int ne = 0;
	double *A = NULL;
	double *b = NULL;
	double ysum = 0.0;
	int i;
	int e;
	int status = -11;

	if(!names || !y_in || !n_out || ns <= 0 || !(T > 0.0) || !(P > 0.0)){
		return -11;
	}

	for(i = 0; i < ns; ++i){
		if(!isfinite(y_in[i]) || y_in[i] < 0.0){
			return -11;
		}
		ysum += y_in[i];
	}
	if(!(ysum > 0.0) || !isfinite(ysum)){
		return -11;
	}

	if(!fprops_collect_elements_source(names, ns, source, &elements, &ne) || ne <= 0){
		return -11;
	}

	A = (double *)calloc((size_t)(ne * ns), sizeof(double));
	b = (double *)calloc((size_t)ne, sizeof(double));
	if(!A || !b){
		goto cleanup;
	}
	if(!fprops_build_element_matrix_source(names, ns, (const char **)elements, ne, source, A)){
		goto cleanup;
	}

	for(e = 0; e < ne; ++e){
		double be = 0.0;
		for(i = 0; i < ns; ++i){
			double yi = y_in[i] / ysum;
			be += A[e * ns + i] * yi;
		}
		b[e] = be;
	}

	eqm_basis_trace_dump("legacy_tpy", names, ns, (const char **)elements, ne, A, b,
		y_in, n_init, source, algorithm, T, P, NULL);

	status = fprops_eqm_tpb(names, ns, (const char **)elements, ne, b, source, T, P,
		algorithm, n_init, n_out, NULL);

cleanup:
	free(A);
	free(b);
	fprops_free_elements(&elements, &ne);
	return status;
}

int fprops_mix_h_tpn(const char **names, int ns, const double *n, const char *source,
		double T, double P, double *H_out){
	return eqm_total_h_tpn(names, ns, n, source, T, P, H_out);
}
