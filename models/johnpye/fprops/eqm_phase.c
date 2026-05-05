#include "eqm_phase.h"

#include "eqm.h"
#include "fluids.h"
#include "gibbs_species.h"
#include "solution.h"
#include "solution_data.h"
#include "spinel_data.h"
#include "eqm_linalg.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static double eqm_phase_R(void){
	return 8.31446261815324;
}

static int eqm_phase_status_ok(int status){
	return status == 0 || status == 1 || status == 6;
}

static int eqm_phase_trace_enabled(void){
	static int inited = 0;
	static int enabled = 0;
	if(!inited){
		const char *v = getenv("FPROPS_EQM_PHASE_TRACE");
		enabled = v && v[0] && 0 != strcmp(v, "0");
		inited = 1;
	}
	return enabled;
}

static void eqm_phase_copy(char *dst, size_t n, const char *src){
	size_t i;
	if(!dst || n == 0){
		return;
	}
	if(!src){
		dst[0] = '\0';
		return;
	}
	for(i = 0; i + 1 < n && src[i]; ++i){
		dst[i] = src[i];
	}
	dst[i] = '\0';
}

static void eqm_phase_copy_range(char *dst, size_t n, const char *a, const char *b){
	const char *s = a;
	const char *e = b;
	size_t len;
	if(!dst || n == 0){
		return;
	}
	while(s < e && isspace((unsigned char)*s)){
		++s;
	}
	while(e > s && isspace((unsigned char)*(e - 1))){
		--e;
	}
	len = (size_t)(e - s);
	if(len >= n){
		len = n - 1;
	}
	if(len > 0){
		memcpy(dst, s, len);
	}
	dst[len] = '\0';
}

static int eqm_phase_add_element(FpropsEqmPhaseModel *phase, const char *name){
	int i;
	if(!phase || !name || !name[0]){
		return 0;
	}
	for(i = 0; i < phase->nelem; ++i){
		if(0 == strcmp(phase->elements[i], name)){
			return 1;
		}
	}
	if(phase->nelem >= FPROPS_EQM_PHASE_MAX_ELEMS){
		return 0;
	}
	eqm_phase_copy(phase->element_storage[phase->nelem],
		sizeof(phase->element_storage[phase->nelem]), name);
	phase->elements[phase->nelem] = phase->element_storage[phase->nelem];
	++phase->nelem;
	return 1;
}

static int eqm_phase_element_index(const FpropsEqmPhaseModel *phase, const char *name){
	int i;
	if(!phase || !name){
		return -1;
	}
	for(i = 0; i < phase->nelem; ++i){
		if(0 == strcmp(phase->elements[i], name)){
			return i;
		}
	}
	return -1;
}

static int eqm_phase_global_element_index(const char **elements, int ne, const char *name){
	int i;
	if(!elements || !name){
		return -1;
	}
	for(i = 0; i < ne; ++i){
		if(0 == strcmp(elements[i], name)){
			return i;
		}
	}
	return -1;
}

static int eqm_phase_set_member(FpropsEqmPhaseModel *phase, int i, const char *name){
	if(!phase || i < 0 || i >= FPROPS_EQM_PHASE_MAX_MEMBERS || !name){
		return 0;
	}
	eqm_phase_copy(phase->member_storage[i], sizeof(phase->member_storage[i]), name);
	phase->members[i] = phase->member_storage[i];
	if(i >= phase->nmember){
		phase->nmember = i + 1;
	}
	return 1;
}

static int eqm_phase_set_var(FpropsEqmPhaseModel *phase, int i, const char *name,
		double lo, double hi){
	if(!phase || i < 0 || i >= FPROPS_EQM_PHASE_MAX_VARS || !name){
		return 0;
	}
	eqm_phase_copy(phase->var_storage[i], sizeof(phase->var_storage[i]), name);
	phase->var_names[i] = phase->var_storage[i];
	phase->lower[i] = lo;
	phase->upper[i] = hi;
	if(i >= phase->nvar){
		phase->nvar = i + 1;
	}
	return 1;
}

static void eqm_phase_init(FpropsEqmPhaseModel *phase){
	memset(phase, 0, sizeof(*phase));
	phase->name = phase->name_storage;
	phase->source = phase->source_storage;
	phase->basis = phase->basis_storage;
}

const char *fprops_eqm_phase_kind_name(FpropsEqmPhaseKind kind){
	switch(kind){
	case FPROPS_EQM_PHASE_STOICHIOMETRIC:
		return "stoichiometric";
	case FPROPS_EQM_PHASE_IDEAL_GAS:
		return "ideal_gas";
	case FPROPS_EQM_PHASE_BINARY_SOLUTION:
		return "binary_solution";
	case FPROPS_EQM_PHASE_SITE_SOLUTION:
		return "site_solution";
	case FPROPS_EQM_PHASE_GENERIC:
		return "generic";
	default:
		return "unknown";
	}
}

static void eqm_phase_parse_spec(const char *spec, char *name, size_t nname,
		char *source, size_t nsource){
	const char *eq;
	const char *lb;
	const char *rb;
	name[0] = '\0';
	source[0] = '\0';
	if(!spec){
		return;
	}
	eq = strchr(spec, '=');
	if(eq){
		eqm_phase_copy_range(source, nsource, eq + 1, spec + strlen(spec));
	}else{
		eq = spec + strlen(spec);
	}
	eqm_phase_copy_range(name, nname, spec, eq);
	if(0 == strncmp(name, "phase:", 6)){
		memmove(name, name + 6, strlen(name + 6) + 1);
	}
	lb = strchr(name, '[');
	rb = lb ? strchr(lb + 1, ']') : NULL;
	if(lb && rb){
		if(source[0] == '\0'){
			eqm_phase_copy_range(source, nsource, lb + 1, rb);
		}
		eqm_phase_copy_range(name, nname, name, lb);
	}
}

static const char *eqm_phase_source_for(const char *name, const char *fallback,
		char *buf, unsigned nbuf){
	const char *s;
	if(!fallback || !fallback[0]){
		return NULL;
	}
	s = fprops_resolve_species_source(fallback, name, buf, nbuf);
	return s ? s : fallback;
}

static int eqm_phase_resolve_gas(const char *spec_name, const char *source,
		FpropsEqmPhaseModel *phase){
	const char *a;
	const char *b;
	const char *p;
	int i = 0;
	if(0 != strncmp(spec_name, "gas:ideal(", 10)){
		return 0;
	}
	a = spec_name + 10;
	b = strrchr(a, ')');
	if(!b || b <= a){
		return 0;
	}
	phase->kind = FPROPS_EQM_PHASE_IDEAL_GAS;
	eqm_phase_copy(phase->name_storage, sizeof(phase->name_storage), "gas:ideal");
	eqm_phase_copy(phase->source_storage, sizeof(phase->source_storage), source);
	eqm_phase_copy(phase->basis_storage, sizeof(phase->basis_storage), "mol gas mixture");
	p = a;
	while(p < b){
		const char *comma = p;
		while(comma < b && *comma != ','){
			++comma;
		}
		if(i >= FPROPS_EQM_PHASE_MAX_MEMBERS){
			return 0;
		}
		eqm_phase_copy_range(phase->member_storage[i], sizeof(phase->member_storage[i]), p, comma);
		phase->members[i] = phase->member_storage[i];
		eqm_phase_set_var(phase, i, phase->member_storage[i], 0.0, 1.0);
		++i;
		p = (comma < b) ? comma + 1 : b;
	}
	phase->nmember = i;
	if(phase->nmember <= 0){
		return 0;
	}
	{
		char **elements = NULL;
		int ne = 0;
		if(!fprops_collect_elements_source(phase->members, phase->nmember, source, &elements, &ne)){
			return 0;
		}
		for(i = 0; i < ne; ++i){
			if(!eqm_phase_add_element(phase, elements[i])){
				fprops_free_elements(&elements, &ne);
				return 0;
			}
		}
		fprops_free_elements(&elements, &ne);
	}
	return 1;
}

static int eqm_phase_resolve_binary(const char *name, const char *source,
		FpropsEqmPhaseModel *phase){
	const BinarySolutionPhaseDef *B = solution_phase_lookup(name, source);
	unsigned i;
	if(!B && (!source || !source[0])){
		B = solution_phase_lookup(name, NULL);
	}
	if(!B){
		return 0;
	}
	phase->kind = FPROPS_EQM_PHASE_BINARY_SOLUTION;
	phase->data = B;
	eqm_phase_copy(phase->name_storage, sizeof(phase->name_storage), B->name);
	eqm_phase_copy(phase->source_storage, sizeof(phase->source_storage), B->source);
	eqm_phase_copy(phase->basis_storage, sizeof(phase->basis_storage), "mol binary solution members");
	eqm_phase_set_member(phase, 0, B->member_a_name);
	eqm_phase_set_member(phase, 1, B->member_b_name);
	eqm_phase_set_var(phase, 0, "x_member_b", B->model->xmin, B->model->xmax);
	for(i = 0; i < B->nelem_a; ++i){
		if(!eqm_phase_add_element(phase, B->elements_a[i])){
			return 0;
		}
	}
	for(i = 0; i < B->nelem_b; ++i){
		if(!eqm_phase_add_element(phase, B->elements_b[i])){
			return 0;
		}
	}
	return 1;
}

static int eqm_phase_resolve_spinel(const char *name, const char *source,
		FpropsEqmPhaseModel *phase){
	const FeSpinelPhaseDef *S = spinel_phase_lookup(name, source);
	int j;
	if(!S && 0 == strcmp(name, "spinel")){
		S = spinel_phase_lookup("spinel_fe", source);
	}
	if(!S && (!source || !source[0])){
		S = spinel_phase_lookup(name, NULL);
	}
	if(!S){
		return 0;
	}
	phase->kind = FPROPS_EQM_PHASE_SITE_SOLUTION;
	phase->data = S;
	eqm_phase_copy(phase->name_storage, sizeof(phase->name_storage), "spinel");
	eqm_phase_copy(phase->source_storage, sizeof(phase->source_storage), S->source);
	eqm_phase_copy(phase->basis_storage, sizeof(phase->basis_storage), "mol spinel formula unit");
	for(j = 0; j < 5; ++j){
		eqm_phase_set_member(phase, j, S->member_names[j]);
	}
	eqm_phase_set_var(phase, 0, "y_tet_fe2", 0.0, 1.0);
	eqm_phase_set_var(phase, 1, "y_oct_fe2", 0.0, 1.0);
	eqm_phase_add_element(phase, "Fe");
	eqm_phase_add_element(phase, "O");
	return 1;
}

static int eqm_phase_resolve_stoich(const char *name, const char *source,
		FpropsEqmPhaseModel *phase){
	const GibbsSpecies *G = gibbs_species_lookup(name, source);
	unsigned i;
	if(!G && (!source || !source[0])){
		G = gibbs_species_lookup(name, NULL);
	}
	if(!G){
		return 0;
	}
	phase->kind = FPROPS_EQM_PHASE_STOICHIOMETRIC;
	phase->data = G;
	eqm_phase_copy(phase->name_storage, sizeof(phase->name_storage), G->name);
	eqm_phase_copy(phase->source_storage, sizeof(phase->source_storage), G->source);
	eqm_phase_copy(phase->basis_storage, sizeof(phase->basis_storage), "mol species");
	eqm_phase_set_member(phase, 0, G->name);
	for(i = 0; i < G->nelem; ++i){
		if(!eqm_phase_add_element(phase, G->elements[i])){
			return 0;
		}
	}
	return 1;
}

int fprops_eqm_phase_resolve(const char *spec, const char *source,
		FpropsEqmPhaseModel *phase){
	char name[256];
	char spec_source[256];
	char source_buf[256];
	const char *use_source;
	if(!spec || !phase){
		return 0;
	}
	eqm_phase_init(phase);
	eqm_phase_parse_spec(spec, name, sizeof(name), spec_source, sizeof(spec_source));
	use_source = spec_source[0] ? spec_source : source;
	if(0 != strncmp(name, "gas:", 4)){
		use_source = eqm_phase_source_for(name, use_source, source_buf, (unsigned)sizeof(source_buf));
	}
	if(eqm_phase_resolve_gas(name, use_source, phase)){
		return 1;
	}
	if(eqm_phase_resolve_binary(name, use_source, phase)){
		return 1;
	}
	if(eqm_phase_resolve_spinel(name, use_source, phase)){
		return 1;
	}
	if(eqm_phase_resolve_stoich(name, use_source, phase)){
		return 1;
	}
	return 0;
}

static int eqm_phase_binary_elements(const FpropsEqmPhaseModel *phase,
		const double *y, double *a_out){
	const BinarySolutionPhaseDef *B = (const BinarySolutionPhaseDef *)phase->data;
	double x;
	int e;
	unsigned i;
	if(!B || !y || !a_out){
		return 0;
	}
	x = y[0];
	if(!(x >= B->model->xmin && x <= B->model->xmax)){
		return 0;
	}
	for(e = 0; e < phase->nelem; ++e){
		a_out[e] = 0.0;
	}
	for(i = 0; i < B->nelem_a; ++i){
		e = eqm_phase_element_index(phase, B->elements_a[i]);
		if(e >= 0){
			a_out[e] += (1.0 - x) * B->stoich_a[i];
		}
	}
	for(i = 0; i < B->nelem_b; ++i){
		e = eqm_phase_element_index(phase, B->elements_b[i]);
		if(e >= 0){
			a_out[e] += x * B->stoich_b[i];
		}
	}
	return 1;
}

static int eqm_phase_spinel_state(const double *y, double *n_members,
		double *n_fe, double *n_o){
	double a;
	double b;
	double c;
	double v;
	if(!y || !n_members){
		return 0;
	}
	a = y[0];
	b = y[1];
	c = (a + 5.0 - 4.0 * b) / 6.0;
	v = (1.0 - a - 2.0 * b) / 6.0;
	if(!(a >= 0.0 && a <= 1.0 && b >= 0.0 && b <= 1.0
			&& c >= 0.0 && c <= 1.0 && v >= 0.0 && v <= 1.0)){
		return 0;
	}
	n_members[0] = a;
	n_members[1] = 1.0 - a;
	n_members[2] = 2.0 * b;
	n_members[3] = 2.0 * c;
	n_members[4] = 2.0 * v;
	if(n_fe){
		*n_fe = 1.0 + 2.0 * (b + c);
	}
	if(n_o){
		*n_o = 4.0;
	}
	return 1;
}

int fprops_eqm_phase_elements(const FpropsEqmPhaseModel *phase,
		const double *y, double *a_out){
	int e;
	if(!phase || !a_out){
		return 0;
	}
	for(e = 0; e < phase->nelem; ++e){
		a_out[e] = 0.0;
	}
	if(phase->kind == FPROPS_EQM_PHASE_STOICHIOMETRIC){
		const GibbsSpecies *G = (const GibbsSpecies *)phase->data;
		unsigned i;
		if(!G){
			return 0;
		}
		for(i = 0; i < G->nelem; ++i){
			e = eqm_phase_element_index(phase, G->elements[i]);
			if(e >= 0){
				a_out[e] += G->stoich[i];
			}
		}
		return 1;
	}
	if(phase->kind == FPROPS_EQM_PHASE_BINARY_SOLUTION){
		return eqm_phase_binary_elements(phase, y, a_out);
	}
	if(phase->kind == FPROPS_EQM_PHASE_SITE_SOLUTION){
		double n_members[5];
		double n_fe;
		double n_o;
		if(!eqm_phase_spinel_state(y, n_members, &n_fe, &n_o)){
			return 0;
		}
		e = eqm_phase_element_index(phase, "Fe");
		if(e >= 0){
			a_out[e] = n_fe;
		}
		e = eqm_phase_element_index(phase, "O");
		if(e >= 0){
			a_out[e] = n_o;
		}
		return 1;
	}
	if(phase->kind == FPROPS_EQM_PHASE_IDEAL_GAS){
		double A[FPROPS_EQM_PHASE_MAX_ELEMS * FPROPS_EQM_PHASE_MAX_MEMBERS];
		int i;
		if(!y || !fprops_build_element_matrix_source((const char **)phase->members, phase->nmember,
				(const char **)phase->elements, phase->nelem, phase->source, A)){
			return 0;
		}
		for(i = 0; i < phase->nmember; ++i){
			for(e = 0; e < phase->nelem; ++e){
				a_out[e] += y[i] * A[e * phase->nmember + i];
			}
		}
		return 1;
	}
	return 0;
}

int fprops_eqm_phase_gibbs(const FpropsEqmPhaseModel *phase,
		double T, double P, const double *y, double *g_out){
	if(!phase || !g_out || !(T > 0.0) || !(P > 0.0)){
		return 0;
	}
	if(phase->kind == FPROPS_EQM_PHASE_STOICHIOMETRIC){
		return gibbs_species_g_molar((const GibbsSpecies *)phase->data, T, P, g_out);
	}
	if(phase->kind == FPROPS_EQM_PHASE_BINARY_SOLUTION){
		const BinarySolutionPhaseDef *B = (const BinarySolutionPhaseDef *)phase->data;
		FpropsError err = FPROPS_NO_ERROR;
		if(!B || !B->model || !y){
			return 0;
		}
		*g_out = solution_binary_g_molar(B->model, T, P, y[0], &err);
		return !err && isfinite(*g_out);
	}
	if(phase->kind == FPROPS_EQM_PHASE_SITE_SOLUTION){
		const FeSpinelPhaseDef *S = (const FeSpinelPhaseDef *)phase->data;
		double n_members[5];
		if(!S || !eqm_phase_spinel_state(y, n_members, NULL, NULL)){
			return 0;
		}
		return spinel_phase_eval(S, n_members, T, P, g_out, NULL);
	}
	if(phase->kind == FPROPS_EQM_PHASE_IDEAL_GAS){
		const double RT = eqm_phase_R() * T;
		const double logPP0 = log(P / 1e5);
		double sum = 0.0;
		double ysum = 0.0;
		int i;
		if(!y){
			return 0;
		}
		for(i = 0; i < phase->nmember; ++i){
			double mu0;
			if(y[i] < 0.0 || !isfinite(y[i])){
				return 0;
			}
			ysum += y[i];
			if(!eqm_mu0_source(phase->members[i], phase->source, T, 1e5, &mu0)){
				return 0;
			}
			if(y[i] > 0.0){
				sum += y[i] * (mu0 + RT * (log(y[i]) + logPP0));
			}
		}
		if(fabs(ysum - 1.0) > 1e-8){
			return 0;
		}
		*g_out = sum;
		return isfinite(*g_out);
	}
	return 0;
}

static int eqm_phase_entry_stoich(const FpropsEqmPhaseModel *phase,
		double T, double P, const double *lambda, double *phi_out){
	double g;
	double a[FPROPS_EQM_PHASE_MAX_ELEMS];
	int e;
	if(!fprops_eqm_phase_gibbs(phase, T, P, NULL, &g)
			|| !fprops_eqm_phase_elements(phase, NULL, a)){
		return 0;
	}
	for(e = 0; e < phase->nelem; ++e){
		g += lambda[e] * a[e];
	}
	*phi_out = g / (eqm_phase_R() * T);
	return isfinite(*phi_out);
}

static int eqm_phase_entry_binary(const FpropsEqmPhaseModel *phase,
		double T, double P, const double *lambda, double *phi_out, double *y_out){
	const BinarySolutionPhaseDef *B = (const BinarySolutionPhaseDef *)phase->data;
	double lo;
	double hi;
	double best_x = 0.0;
	double best = HUGE_VAL;
	int round;
	if(!B || !B->model){
		return 0;
	}
	lo = B->model->xmin;
	hi = B->model->xmax;
	for(round = 0; round < 6; ++round){
		int i;
		double span = hi - lo;
		for(i = 0; i <= 80; ++i){
			double x = lo + span * (double)i / 80.0;
			double y[1] = {x};
			double g;
			double a[FPROPS_EQM_PHASE_MAX_ELEMS];
			double r;
			int e;
			if(!fprops_eqm_phase_gibbs(phase, T, P, y, &g)
					|| !fprops_eqm_phase_elements(phase, y, a)){
				continue;
			}
			r = g;
			for(e = 0; e < phase->nelem; ++e){
				r += lambda[e] * a[e];
			}
			if(r < best){
				best = r;
				best_x = x;
			}
		}
		lo = fmax(B->model->xmin, best_x - span / 20.0);
		hi = fmin(B->model->xmax, best_x + span / 20.0);
	}
	if(!isfinite(best)){
		return 0;
	}
	*phi_out = best / (eqm_phase_R() * T);
	if(y_out){
		y_out[0] = best_x;
	}
	return 1;
}

static int eqm_phase_entry_spinel(const FpropsEqmPhaseModel *phase,
		double T, double P, const double *lambda, double *phi_out, double *y_out){
	double alo = 0.0;
	double ahi = 1.0;
	double blo = 0.0;
	double bhi = 0.5;
	double best_a = 0.0;
	double best_b = 0.0;
	double best = HUGE_VAL;
	int round;
	for(round = 0; round < 5; ++round){
		int ia;
		double aspan = ahi - alo;
		double bspan = bhi - blo;
		for(ia = 0; ia <= 50; ++ia){
			int ib;
			double a = alo + aspan * (double)ia / 50.0;
			for(ib = 0; ib <= 50; ++ib){
				double b = blo + bspan * (double)ib / 50.0;
				double y[2] = {a, b};
				double g;
				double elems[FPROPS_EQM_PHASE_MAX_ELEMS];
				double r;
				int e;
				if(!fprops_eqm_phase_gibbs(phase, T, P, y, &g)
						|| !fprops_eqm_phase_elements(phase, y, elems)){
					continue;
				}
				r = g;
				for(e = 0; e < phase->nelem; ++e){
					r += lambda[e] * elems[e];
				}
				if(r < best){
					best = r;
					best_a = a;
					best_b = b;
				}
			}
		}
		alo = fmax(0.0, best_a - aspan / 12.0);
		ahi = fmin(1.0, best_a + aspan / 12.0);
		blo = fmax(0.0, best_b - bspan / 12.0);
		bhi = fmin(0.5, best_b + bspan / 12.0);
	}
	if(!isfinite(best)){
		return 0;
	}
	*phi_out = best / (eqm_phase_R() * T);
	if(y_out){
		y_out[0] = best_a;
		y_out[1] = best_b;
	}
	return 1;
}

static int eqm_phase_entry_gas(const FpropsEqmPhaseModel *phase,
		double T, double P, const double *lambda, double *phi_out, double *y_out){
	const double RT = eqm_phase_R() * T;
	const double logPP0 = log(P / 1e5);
	double weights[FPROPS_EQM_PHASE_MAX_MEMBERS];
	double maxz = -HUGE_VAL;
	double sum = 0.0;
	double A[FPROPS_EQM_PHASE_MAX_ELEMS * FPROPS_EQM_PHASE_MAX_MEMBERS];
	int i;
	if(!fprops_build_element_matrix_source((const char **)phase->members, phase->nmember,
			(const char **)phase->elements, phase->nelem, phase->source, A)){
		return 0;
	}
	for(i = 0; i < phase->nmember; ++i){
		double mu0;
		double q;
		double z;
		int e;
		if(!eqm_mu0_source(phase->members[i], phase->source, T, 1e5, &mu0)){
			return 0;
		}
		q = mu0 + RT * logPP0;
		for(e = 0; e < phase->nelem; ++e){
			q += lambda[e] * A[e * phase->nmember + i];
		}
		z = -q / RT;
		weights[i] = z;
		if(z > maxz){
			maxz = z;
		}
	}
	for(i = 0; i < phase->nmember; ++i){
		weights[i] = exp(weights[i] - maxz);
		sum += weights[i];
	}
	if(!(sum > 0.0) || !isfinite(sum)){
		return 0;
	}
	*phi_out = -(maxz + log(sum));
	if(y_out){
		for(i = 0; i < phase->nmember; ++i){
			y_out[i] = weights[i] / sum;
		}
	}
	return isfinite(*phi_out);
}

int fprops_eqm_phase_entry_residual(const FpropsEqmPhaseModel *phase,
		double T, double P, const double *lambda, double *phi_out, double *y_out){
	if(!phase || !lambda || !phi_out || !(T > 0.0) || !(P > 0.0)){
		return 0;
	}
	if(phase->kind == FPROPS_EQM_PHASE_STOICHIOMETRIC){
		return eqm_phase_entry_stoich(phase, T, P, lambda, phi_out);
	}
	if(phase->kind == FPROPS_EQM_PHASE_BINARY_SOLUTION){
		return eqm_phase_entry_binary(phase, T, P, lambda, phi_out, y_out);
	}
	if(phase->kind == FPROPS_EQM_PHASE_SITE_SOLUTION){
		return eqm_phase_entry_spinel(phase, T, P, lambda, phi_out, y_out);
	}
	if(phase->kind == FPROPS_EQM_PHASE_IDEAL_GAS){
		return eqm_phase_entry_gas(phase, T, P, lambda, phi_out, y_out);
	}
	return 0;
}

int fprops_eqm_phase_solve_fixed_linear(const FpropsEqmPhaseModel *phases, int nphase,
		const char **elements, int ne, const double *b, double T, double P,
		double *phase_amounts_out, double *member_amounts_out){
	double M[FPROPS_EQM_PHASE_MAX_ELEMS * FPROPS_EQM_PHASE_MAX_MEMBERS * FPROPS_EQM_PHASE_MAX_MEMBERS];
	double rhs[FPROPS_EQM_PHASE_MAX_ELEMS];
	double member_work[FPROPS_EQM_PHASE_MAX_ELEMS * FPROPS_EQM_PHASE_MAX_MEMBERS];
	int nvar = 0;
	int col = 0;
	int off = 0;
	int p;
	int e;
	(void)T;
	(void)P;
	if(!phases || nphase <= 0 || !elements || ne <= 0 || !b
			|| !phase_amounts_out || !member_amounts_out){
		return -11;
	}
	if(ne > FPROPS_EQM_PHASE_MAX_ELEMS){
		return -11;
	}
	for(p = 0; p < nphase; ++p){
		if(phases[p].kind == FPROPS_EQM_PHASE_STOICHIOMETRIC){
			nvar += 1;
		}else if(phases[p].kind == FPROPS_EQM_PHASE_IDEAL_GAS){
			nvar += phases[p].nmember;
		}else{
			return -12;
		}
	}
	if(nvar != ne || nvar > FPROPS_EQM_PHASE_MAX_ELEMS * FPROPS_EQM_PHASE_MAX_MEMBERS){
		return -12;
	}
	for(e = 0; e < ne; ++e){
		int j;
		rhs[e] = b[e];
		for(j = 0; j < nvar; ++j){
			M[e * nvar + j] = 0.0;
		}
	}
	for(p = 0; p < nphase; ++p){
		if(phases[p].kind == FPROPS_EQM_PHASE_STOICHIOMETRIC){
			double a[FPROPS_EQM_PHASE_MAX_ELEMS];
			if(!fprops_eqm_phase_elements(&phases[p], NULL, a)){
				return -13;
			}
			for(e = 0; e < ne; ++e){
				int pe = eqm_phase_element_index(&phases[p], elements[e]);
				M[e * nvar + col] = (pe >= 0) ? a[pe] : 0.0;
			}
			++col;
		}else if(phases[p].kind == FPROPS_EQM_PHASE_IDEAL_GAS){
			if(!fprops_build_element_matrix_source((const char **)phases[p].members,
					phases[p].nmember, elements, ne, phases[p].source, member_work)){
				return -13;
			}
			for(int j = 0; j < phases[p].nmember; ++j){
				for(e = 0; e < ne; ++e){
					M[e * nvar + col] = member_work[e * phases[p].nmember + j];
				}
				++col;
			}
		}
	}
	if(!eqm_linalg_dense_solve(M, rhs, nvar)){
		return -13;
	}
	col = 0;
	off = 0;
	for(p = 0; p < nphase; ++p){
		int j;
		double amount = 0.0;
		if(phases[p].kind == FPROPS_EQM_PHASE_STOICHIOMETRIC){
			amount = rhs[col++];
			member_amounts_out[off++] = amount;
		}else if(phases[p].kind == FPROPS_EQM_PHASE_IDEAL_GAS){
			for(j = 0; j < phases[p].nmember; ++j){
				double n = rhs[col++];
				member_amounts_out[off++] = n;
				amount += n;
			}
		}
		if(amount < -1e-9 || !isfinite(amount)){
			return -22;
		}
		phase_amounts_out[p] = amount < 0.0 ? 0.0 : amount;
	}
	for(p = 0; p < off; ++p){
		if(member_amounts_out[p] < -1e-9 || !isfinite(member_amounts_out[p])){
			return -22;
		}
		if(member_amounts_out[p] < 0.0){
			member_amounts_out[p] = 0.0;
		}
	}
	return 0;
}

static int eqm_phase_append_source(char *buf, size_t nbuf, size_t *pos,
		const char *member, const char *source){
	int nw;
	if(!buf || !pos || !member || !source){
		return 0;
	}
	if(!source[0]){
		return 1;
	}
	nw = snprintf(buf + *pos, nbuf - *pos, "%s%s=%s",
		(*pos > 0) ? ";" : "", member, source);
	if(nw < 0 || (size_t)nw >= nbuf - *pos){
		return 0;
	}
	*pos += (size_t)nw;
	return 1;
}

static int eqm_phase_expand_members(const FpropsEqmPhaseModel *phases, int nphase,
		const char **names, char *source_map, size_t source_map_len, int *nmember_out){
	int p;
	int ns = 0;
	size_t source_pos = 0;
	if(!phases || nphase <= 0 || !names || !source_map || !nmember_out){
		return 0;
	}
	source_map[0] = '\0';
	for(p = 0; p < nphase; ++p){
		int j;
		if(phases[p].nmember <= 0 || phases[p].nmember > FPROPS_EQM_PHASE_MAX_MEMBERS){
			return 0;
		}
		if(phases[p].kind != FPROPS_EQM_PHASE_STOICHIOMETRIC
				&& phases[p].kind != FPROPS_EQM_PHASE_IDEAL_GAS
				&& phases[p].kind != FPROPS_EQM_PHASE_BINARY_SOLUTION
				&& phases[p].kind != FPROPS_EQM_PHASE_SITE_SOLUTION){
			return 0;
		}
		for(j = 0; j < phases[p].nmember; ++j){
			if(ns >= FPROPS_EQM_PHASE_MAX_MEMBERS * FPROPS_EQM_PHASE_MAX_MEMBERS){
				return 0;
			}
			names[ns++] = phases[p].members[j];
			if(!eqm_phase_append_source(source_map, source_map_len, &source_pos,
					phases[p].members[j], phases[p].source)){
				return 0;
			}
		}
	}
	*nmember_out = ns;
	return 1;
}

static void eqm_phase_fill_outputs(const FpropsEqmPhaseModel *phases, int nphase,
		const double *n_members, double *phase_amounts_out, double *phase_y_out,
		double *member_amounts_out){
	int off = 0;
	int p;
	for(p = 0; p < nphase; ++p){
		int j;
		double amount = 0.0;
		if(phases[p].kind == FPROPS_EQM_PHASE_STOICHIOMETRIC){
			amount = n_members[off];
			if(phase_y_out){
				for(j = 0; j < FPROPS_EQM_PHASE_MAX_VARS; ++j){
					phase_y_out[p * FPROPS_EQM_PHASE_MAX_VARS + j] = NAN;
				}
			}
			++off;
		}else if(phases[p].kind == FPROPS_EQM_PHASE_IDEAL_GAS){
			for(j = 0; j < phases[p].nmember; ++j){
				amount += n_members[off + j];
			}
			if(phase_y_out){
				for(j = 0; j < phases[p].nmember; ++j){
					phase_y_out[p * FPROPS_EQM_PHASE_MAX_VARS + j] =
						(amount > 0.0) ? n_members[off + j] / amount : NAN;
				}
				for(; j < FPROPS_EQM_PHASE_MAX_VARS; ++j){
					phase_y_out[p * FPROPS_EQM_PHASE_MAX_VARS + j] = NAN;
				}
			}
			off += phases[p].nmember;
		}else if(phases[p].kind == FPROPS_EQM_PHASE_BINARY_SOLUTION){
			double na = n_members[off];
			double nb = n_members[off + 1];
			amount = na + nb;
			if(phase_y_out){
				phase_y_out[p * FPROPS_EQM_PHASE_MAX_VARS] =
					(amount > 0.0) ? nb / amount : NAN;
				for(j = 1; j < FPROPS_EQM_PHASE_MAX_VARS; ++j){
					phase_y_out[p * FPROPS_EQM_PHASE_MAX_VARS + j] = NAN;
				}
			}
			off += 2;
		}else if(phases[p].kind == FPROPS_EQM_PHASE_SITE_SOLUTION){
			double nt = n_members[off] + n_members[off + 1];
			double no = n_members[off + 2] + n_members[off + 3] + n_members[off + 4];
			amount = nt;
			if(phase_y_out){
				phase_y_out[p * FPROPS_EQM_PHASE_MAX_VARS] =
					(nt > 0.0) ? n_members[off] / nt : NAN;
				phase_y_out[p * FPROPS_EQM_PHASE_MAX_VARS + 1] =
					(no > 0.0) ? n_members[off + 2] / no : NAN;
				for(j = 2; j < FPROPS_EQM_PHASE_MAX_VARS; ++j){
					phase_y_out[p * FPROPS_EQM_PHASE_MAX_VARS + j] = NAN;
				}
			}
			off += 5;
		}
		if(phase_amounts_out){
			phase_amounts_out[p] = amount;
		}
	}
	if(member_amounts_out){
		int n = off;
		for(p = 0; p < n; ++p){
			member_amounts_out[p] = n_members[p];
		}
	}
}

int fprops_eqm_phase_solve_fixed_expanded(const FpropsEqmPhaseModel *phases, int nphase,
		const char **elements, int ne, const double *b, double T, double P,
		const char *algorithm, const double *member_init,
		double *phase_amounts_out, double *phase_y_out, double *member_amounts_out,
		int *nmember_out){
	const char *names[FPROPS_EQM_PHASE_MAX_MEMBERS * FPROPS_EQM_PHASE_MAX_MEMBERS];
	double n_out[FPROPS_EQM_PHASE_MAX_MEMBERS * FPROPS_EQM_PHASE_MAX_MEMBERS];
	char source_map[4096];
	int ns = 0;
	int status;
	if(!phases || nphase <= 0 || !elements || ne <= 0 || !b || !(T > 0.0) || !(P > 0.0)){
		return -11;
	}
	if(!eqm_phase_expand_members(phases, nphase, names, source_map, sizeof(source_map), &ns)){
		return -11;
	}
	status = eqm_solve_elements(names, ns, elements, ne, b, source_map, T, P,
		algorithm ? algorithm : "auto", member_init, n_out);
	if(eqm_phase_status_ok(status)){
		eqm_phase_fill_outputs(phases, nphase, n_out, phase_amounts_out, phase_y_out,
			member_amounts_out);
		if(nmember_out){
			*nmember_out = ns;
		}
	}
	return status;
}

static int eqm_phase_total_members(const FpropsEqmPhaseModel *phases, int nphase){
	int p;
	int n = 0;
	if(!phases || nphase < 0){
		return -1;
	}
	for(p = 0; p < nphase; ++p){
		if(phases[p].nmember <= 0 || phases[p].nmember > FPROPS_EQM_PHASE_MAX_MEMBERS){
			return -1;
		}
		n += phases[p].nmember;
	}
	return n;
}

static int eqm_phase_member_offset(const FpropsEqmPhaseModel *phases, int iphase){
	int p;
	int off = 0;
	if(!phases || iphase < 0){
		return -1;
	}
	for(p = 0; p < iphase; ++p){
		off += phases[p].nmember;
	}
	return off;
}

static void eqm_phase_zero_full_outputs(const FpropsEqmPhaseModel *phases, int nphase,
		double *phase_amounts, double *phase_y, int *phase_active,
		double *member_amounts){
	int p;
	int total_members = eqm_phase_total_members(phases, nphase);
	if(phase_amounts){
		for(p = 0; p < nphase; ++p){
			phase_amounts[p] = 0.0;
		}
	}
	if(phase_active){
		for(p = 0; p < nphase; ++p){
			phase_active[p] = 0;
		}
	}
	if(phase_y){
		for(p = 0; p < nphase * FPROPS_EQM_PHASE_MAX_VARS; ++p){
			phase_y[p] = NAN;
		}
	}
	if(member_amounts && total_members >= 0){
		for(p = 0; p < total_members; ++p){
			member_amounts[p] = 0.0;
		}
	}
}

static int eqm_phase_output_objective(const FpropsEqmPhaseModel *phases, int nphase,
		const double *phase_amounts, const double *phase_y, double T, double P,
		double *g_total_out, int *active_count_out){
	double gtot = 0.0;
	int active = 0;
	int p;
	if(!phases || !phase_amounts || !phase_y || !g_total_out){
		return 0;
	}
	for(p = 0; p < nphase; ++p){
		double amount = phase_amounts[p];
		double g = 0.0;
		const double *y = NULL;
		if(amount < -1e-8 || !isfinite(amount)){
			return 0;
		}
		if(amount <= 1e-12){
			continue;
		}
		if(phases[p].kind != FPROPS_EQM_PHASE_STOICHIOMETRIC){
			y = phase_y + p * FPROPS_EQM_PHASE_MAX_VARS;
		}
		if(!fprops_eqm_phase_gibbs(&phases[p], T, P, y, &g)){
			return 0;
		}
		gtot += amount * g;
		++active;
	}
	*g_total_out = gtot;
	if(active_count_out){
		*active_count_out = active;
	}
	return isfinite(gtot);
}

static int eqm_phase_validate_full_balance(const FpropsEqmPhaseModel *phases, int nphase,
		const char **elements, int ne, const double *b, const double *phase_amounts,
		const double *phase_y){
	int e;
	if(!phases || !elements || !b || !phase_amounts || !phase_y || ne <= 0){
		return 0;
	}
	for(e = 0; e < ne; ++e){
		double sum = 0.0;
		double scale = fabs(b[e]);
		int p;
		for(p = 0; p < nphase; ++p){
			double a[FPROPS_EQM_PHASE_MAX_ELEMS];
			int ie;
			if(phase_amounts[p] <= 1e-12){
				continue;
			}
			if(!fprops_eqm_phase_elements(&phases[p],
					phases[p].kind == FPROPS_EQM_PHASE_STOICHIOMETRIC
						? NULL : phase_y + p * FPROPS_EQM_PHASE_MAX_VARS,
					a)){
				return 0;
			}
			ie = eqm_phase_element_index(&phases[p], elements[e]);
			if(ie >= 0){
				sum += phase_amounts[p] * a[ie];
			}
		}
		if(fabs(sum - b[e]) > 1e-6 * fmax(1.0, scale)){
			return 0;
		}
	}
	return 1;
}

static int eqm_phase_append_stationarity_row(double *row_A, double *row_mu, int maxrows,
		int ne, int *nrow, const double *a, double mu){
	if(!row_A || !row_mu || !nrow || !a || ne <= 0 || *nrow >= maxrows || !isfinite(mu)){
		return 0;
	}
	for(int e = 0; e < ne; ++e){
		if(!isfinite(a[e])){
			return 0;
		}
		row_A[*nrow * ne + e] = a[e];
	}
	row_mu[*nrow] = mu;
	++(*nrow);
	return 1;
}

static int eqm_phase_append_phase_unit_row(const FpropsEqmPhaseModel *phase,
		const char **elements, int ne, double T, double P, const double *y,
		double *row_A, double *row_mu, int maxrows, int *nrow){
	double g;
	double local_a[FPROPS_EQM_PHASE_MAX_ELEMS];
	double global_a[FPROPS_EQM_PHASE_MAX_ELEMS];
	if(!fprops_eqm_phase_gibbs(phase, T, P, y, &g)
			|| !fprops_eqm_phase_elements(phase, y, local_a)){
		return 0;
	}
	for(int e = 0; e < ne; ++e){
		global_a[e] = 0.0;
	}
	for(int e = 0; e < phase->nelem; ++e){
		int ge = eqm_phase_global_element_index(elements, ne, phase->elements[e]);
		if(ge < 0){
			return 0;
		}
		global_a[ge] += local_a[e];
	}
	return eqm_phase_append_stationarity_row(row_A, row_mu, maxrows, ne, nrow, global_a, g);
}

static int eqm_phase_append_gas_member_rows(const FpropsEqmPhaseModel *phase,
		const char **elements, int ne, double T, double P, const double *y,
		double *row_A, double *row_mu, int maxrows, int *nrow){
	const double RT = eqm_phase_R() * T;
	const double logPP0 = log(P / 1e5);
	double A[FPROPS_EQM_PHASE_MAX_ELEMS * FPROPS_EQM_PHASE_MAX_MEMBERS];
	if(!y || !fprops_build_element_matrix_source((const char **)phase->members,
			phase->nmember, elements, ne, phase->source, A)){
		return 0;
	}
	for(int j = 0; j < phase->nmember; ++j){
		double mu0;
		double mu;
		double row[FPROPS_EQM_PHASE_MAX_ELEMS];
		if(y[j] <= 1e-12){
			continue;
		}
		if(!eqm_mu0_source(phase->members[j], phase->source, T, 1e5, &mu0)){
			return 0;
		}
		mu = mu0 + RT * (log(y[j]) + logPP0);
		for(int e = 0; e < ne; ++e){
			row[e] = A[e * phase->nmember + j];
		}
		if(!eqm_phase_append_stationarity_row(row_A, row_mu, maxrows, ne, nrow, row, mu)){
			return 0;
		}
	}
	return 1;
}

int fprops_eqm_phase_reconstruct_lambda(const FpropsEqmPhaseModel *phases, int nphase,
		const char **elements, int ne, double T, double P,
		const double *phase_amounts, const double *phase_y, const int *phase_active,
		double *lambda_out, double *stationarity_rms_out){
	double *row_A = NULL;
	double *row_mu = NULL;
	double normal[FPROPS_EQM_PHASE_MAX_ELEMS * FPROPS_EQM_PHASE_MAX_ELEMS];
	double work[FPROPS_EQM_PHASE_MAX_ELEMS * FPROPS_EQM_PHASE_MAX_ELEMS];
	double rhs[FPROPS_EQM_PHASE_MAX_ELEMS];
	int maxrows;
	int nrow = 0;
	int solved = 0;
	if(!phases || nphase <= 0 || !elements || ne <= 0 || ne > FPROPS_EQM_PHASE_MAX_ELEMS
			|| !(T > 0.0) || !(P > 0.0) || !phase_amounts || !phase_y || !lambda_out){
		return 0;
	}
	maxrows = nphase * FPROPS_EQM_PHASE_MAX_MEMBERS;
	row_A = (double *)calloc((size_t)(maxrows * ne), sizeof(double));
	row_mu = (double *)calloc((size_t)maxrows, sizeof(double));
	if(!row_A || !row_mu){
		free(row_A);
		free(row_mu);
		return 0;
	}
	for(int p = 0; p < nphase; ++p){
		const int active = phase_active ? phase_active[p] : (phase_amounts[p] > 1e-10);
		const double *y = (phases[p].kind == FPROPS_EQM_PHASE_STOICHIOMETRIC)
			? NULL : phase_y + p * FPROPS_EQM_PHASE_MAX_VARS;
		if(!active || !(phase_amounts[p] > 1e-12)){
			continue;
		}
		if(phases[p].kind == FPROPS_EQM_PHASE_IDEAL_GAS){
			if(!eqm_phase_append_gas_member_rows(&phases[p], elements, ne, T, P, y,
					row_A, row_mu, maxrows, &nrow)){
				goto cleanup;
			}
		}else if(!eqm_phase_append_phase_unit_row(&phases[p], elements, ne, T, P, y,
				row_A, row_mu, maxrows, &nrow)){
			goto cleanup;
		}
	}
	if(nrow < ne){
		goto cleanup;
	}
	for(int e = 0; e < ne; ++e){
		rhs[e] = 0.0;
		for(int q = 0; q < ne; ++q){
			normal[e * ne + q] = 0.0;
		}
	}
	for(int r = 0; r < nrow; ++r){
		for(int e = 0; e < ne; ++e){
			double ae = row_A[r * ne + e];
			rhs[e] -= ae * row_mu[r];
			for(int q = 0; q < ne; ++q){
				normal[e * ne + q] += ae * row_A[r * ne + q];
			}
		}
	}
	for(int e = 0; e < ne; ++e){
		lambda_out[e] = rhs[e];
		for(int q = 0; q < ne; ++q){
			work[e * ne + q] = normal[e * ne + q];
		}
	}
	solved = eqm_linalg_dense_solve(work, lambda_out, ne);
	if(!solved){
		goto cleanup;
	}
	if(stationarity_rms_out){
		double ss = 0.0;
		double RT = eqm_phase_R() * T;
		for(int r = 0; r < nrow; ++r){
			double resid = row_mu[r];
			for(int e = 0; e < ne; ++e){
				resid += row_A[r * ne + e] * lambda_out[e];
			}
			resid /= RT;
			ss += resid * resid;
		}
		*stationarity_rms_out = sqrt(ss / (double)nrow);
	}
cleanup:
	free(row_A);
	free(row_mu);
	return solved;
}

int fprops_eqm_phase_validate_entry_residuals(const FpropsEqmPhaseModel *phases, int nphase,
		const char **elements, int ne, double T, double P,
		const double *phase_amounts, const double *phase_y, const int *phase_active,
		double *lambda_out, double *entry_residuals_out){
	double lambda_local[FPROPS_EQM_PHASE_MAX_ELEMS];
	double lambda_work[FPROPS_EQM_PHASE_MAX_ELEMS];
	double rms = HUGE_VAL;
	int ok = 1;
	if(!phases || nphase <= 0 || !elements || ne <= 0 || ne > FPROPS_EQM_PHASE_MAX_ELEMS
			|| !phase_amounts || !phase_y || !entry_residuals_out){
		return 0;
	}
	if(!fprops_eqm_phase_reconstruct_lambda(phases, nphase, elements, ne, T, P,
			phase_amounts, phase_y, phase_active, lambda_work, &rms)){
		return 0;
	}
	if(!(rms <= 1e-3)){
		return 0;
	}
	for(int e = 0; e < ne; ++e){
		if(lambda_out){
			lambda_out[e] = lambda_work[e];
		}
	}
	for(int p = 0; p < nphase; ++p){
		double phi = NAN;
		double y_min[FPROPS_EQM_PHASE_MAX_VARS];
		const int active = phase_active ? phase_active[p] : (phase_amounts[p] > 1e-10);
		for(int e = 0; e < phases[p].nelem; ++e){
			int ge = eqm_phase_global_element_index(elements, ne, phases[p].elements[e]);
			if(ge < 0){
				return 0;
			}
			lambda_local[e] = lambda_work[ge];
		}
		if(!fprops_eqm_phase_entry_residual(&phases[p], T, P, lambda_local, &phi, y_min)){
			return 0;
		}
		entry_residuals_out[p] = phi;
		if(active){
			if(fabs(phi) > 5e-2){
				ok = 0;
			}
		}else if(phi < -5e-2){
			ok = 0;
		}
	}
	return ok;
}

static int eqm_phase_active_mask_count(unsigned long long mask, int nphase){
	int n = 0;
	for(int p = 0; p < nphase; ++p){
		if(mask & (1ULL << p)){
			++n;
		}
	}
	return n;
}

static int eqm_phase_solve_mask_full(const FpropsEqmPhaseModel *phases, int nphase,
		const char **elements, int ne, const double *b, double T, double P,
		const char *algorithm, unsigned long long mask, const double *member_seed,
		double *full_amounts, double *full_y, int *full_active, double *full_members,
		int *nmember_out){
	enum {MAX_ACTIVE_PHASES = 20};
	FpropsEqmPhaseModel selected[MAX_ACTIVE_PHASES];
	int selected_index[MAX_ACTIVE_PHASES];
	double cand_amounts[MAX_ACTIVE_PHASES];
	double cand_y[MAX_ACTIVE_PHASES * FPROPS_EQM_PHASE_MAX_VARS];
	double cand_members[MAX_ACTIVE_PHASES * FPROPS_EQM_PHASE_MAX_MEMBERS];
	double cand_init[MAX_ACTIVE_PHASES * FPROPS_EQM_PHASE_MAX_MEMBERS];
	int nsel = 0;
	int nmember_sel = 0;
	int cand_init_off = 0;
	int status;
	if(!phases || nphase <= 0 || nphase > MAX_ACTIVE_PHASES || !full_amounts
			|| !full_y || !full_active || !full_members || mask == 0){
		return -11;
	}
	for(int p = 0; p < nphase; ++p){
		if(mask & (1ULL << p)){
			int full_off = eqm_phase_member_offset(phases, p);
			selected[nsel] = phases[p];
			selected_index[nsel] = p;
			if(member_seed){
				for(int j = 0; j < phases[p].nmember; ++j){
					double seed = member_seed[full_off + j];
					cand_init[cand_init_off + j] =
						(seed > 1e-30 && isfinite(seed)) ? seed : 1e-12;
				}
			}
			cand_init_off += phases[p].nmember;
			++nsel;
		}
	}
	status = fprops_eqm_phase_solve_fixed_expanded(selected, nsel, elements, ne, b, T, P,
		algorithm ? algorithm : "auto", member_seed ? cand_init : NULL,
		cand_amounts, cand_y, cand_members, &nmember_sel);
	if(!eqm_phase_status_ok(status)){
		return status;
	}
	eqm_phase_zero_full_outputs(phases, nphase, full_amounts, full_y, full_active,
		full_members);
	{
		int cand_member_off = 0;
		for(int s = 0; s < nsel; ++s){
			int idx = selected_index[s];
			int full_off = eqm_phase_member_offset(phases, idx);
			full_amounts[idx] = cand_amounts[s];
			full_active[idx] = cand_amounts[s] > 1e-10 ? 1 : 0;
			for(int j = 0; j < FPROPS_EQM_PHASE_MAX_VARS; ++j){
				full_y[idx * FPROPS_EQM_PHASE_MAX_VARS + j] =
					cand_y[s * FPROPS_EQM_PHASE_MAX_VARS + j];
			}
			for(int j = 0; j < phases[idx].nmember; ++j){
				full_members[full_off + j] = cand_members[cand_member_off + j];
			}
			cand_member_off += phases[idx].nmember;
		}
	}
	if(nmember_out){
		*nmember_out = nmember_sel;
	}
	return status;
}

int fprops_eqm_phase_solve_active_set(const FpropsEqmPhaseModel *phases, int nphase,
		const char **elements, int ne, const double *b, double T, double P,
		const char *algorithm, const int *phase_active_init,
		double *phase_amounts_out, double *phase_y_out,
		int *phase_active_out, double *member_amounts_out, int *nmember_out){
	enum {MAX_ACTIVE_PHASES = 20};
	const double drop_tol = 1e-10;
	const double add_tol = -5e-2;
	double full_amounts[MAX_ACTIVE_PHASES];
	double full_y[MAX_ACTIVE_PHASES * FPROPS_EQM_PHASE_MAX_VARS];
	double full_members[MAX_ACTIVE_PHASES * FPROPS_EQM_PHASE_MAX_MEMBERS];
	double seed_members[MAX_ACTIVE_PHASES * FPROPS_EQM_PHASE_MAX_MEMBERS];
	double residuals[MAX_ACTIVE_PHASES];
	double lambda[FPROPS_EQM_PHASE_MAX_ELEMS];
	int full_active[MAX_ACTIVE_PHASES];
	int total_members;
	int status = -99;
	int converged = 0;
	unsigned long long mask = 0;
	unsigned long long blocked_add = 0;
	int have_seed = 0;
	if(!phases || nphase <= 0 || nphase > MAX_ACTIVE_PHASES || !elements || ne <= 0
			|| !b || !(T > 0.0) || !(P > 0.0)){
		return -11;
	}
	total_members = eqm_phase_total_members(phases, nphase);
	if(total_members < 0 || total_members > MAX_ACTIVE_PHASES * FPROPS_EQM_PHASE_MAX_MEMBERS){
		return -11;
	}
	for(int i = 0; i < total_members; ++i){
		seed_members[i] = 0.0;
	}
	if(phase_active_init){
		for(int p = 0; p < nphase; ++p){
			if(phase_active_init[p]){
				mask |= 1ULL << p;
			}
		}
	}else{
		mask = (1ULL << nphase) - 1ULL;
	}
	if(mask == 0){
		mask = (1ULL << nphase) - 1ULL;
	}
	for(int iter = 0; iter < 3 * nphase + 8; ++iter){
		int changed = 0;
		int nmember_sel = 0;
		status = eqm_phase_solve_mask_full(phases, nphase, elements, ne, b, T, P,
			algorithm, mask, have_seed ? seed_members : NULL, full_amounts, full_y,
			full_active, full_members, &nmember_sel);
		if(!eqm_phase_status_ok(status)){
			if(iter == 0 && !phase_active_init){
				unsigned long long nmask = 1ULL << nphase;
				int found_seed = 0;
				for(int nactive = 1; nactive <= nphase && !found_seed; ++nactive){
					for(unsigned long long trial = 1ULL; trial < nmask; ++trial){
						if(eqm_phase_active_mask_count(trial, nphase) != nactive){
							continue;
						}
						status = eqm_phase_solve_mask_full(phases, nphase, elements, ne, b, T, P,
							algorithm, trial, NULL, full_amounts, full_y, full_active,
							full_members, &nmember_sel);
						if(eqm_phase_status_ok(status)
								&& eqm_phase_validate_full_balance(phases, nphase, elements, ne,
									b, full_amounts, full_y)){
							mask = trial;
							found_seed = 1;
							if(eqm_phase_trace_enabled()){
								fprintf(stderr,
									"FPROPS_EQM_PHASE_TRACE active_set seed mask=%llu status=%d\n",
									(unsigned long long)mask, status);
							}
							break;
						}
					}
				}
				if(found_seed){
					for(int i = 0; i < total_members; ++i){
						seed_members[i] = full_members[i];
					}
					have_seed = 1;
					goto active_set_have_solution;
				}
			}
			if(eqm_phase_active_mask_count(mask, nphase) > 1){
				int found_recovery = 0;
				for(int p = 0; p < nphase; ++p){
					unsigned long long trial;
					if(!(mask & (1ULL << p))){
						continue;
					}
					trial = mask & ~(1ULL << p);
					status = eqm_phase_solve_mask_full(phases, nphase, elements, ne, b, T, P,
						algorithm, trial, NULL, full_amounts, full_y, full_active, full_members,
						&nmember_sel);
					if(eqm_phase_status_ok(status)
							&& eqm_phase_validate_full_balance(phases, nphase, elements, ne,
								b, full_amounts, full_y)){
						mask = trial;
						found_recovery = 1;
						if(eqm_phase_trace_enabled()){
							fprintf(stderr,
								"FPROPS_EQM_PHASE_TRACE active_set recover mask=%llu status=%d\n",
								(unsigned long long)mask, status);
						}
						break;
					}
				}
				if(found_recovery){
					for(int i = 0; i < total_members; ++i){
						seed_members[i] = full_members[i];
					}
					have_seed = 1;
					goto active_set_have_solution;
				}
			}
			if(eqm_phase_trace_enabled()){
				fprintf(stderr, "FPROPS_EQM_PHASE_TRACE active_set iter=%d mask=%llu status=%d failed\n",
					iter, (unsigned long long)mask, status);
			}
			break;
		}
active_set_have_solution:
		for(int i = 0; i < total_members; ++i){
			seed_members[i] = full_members[i];
		}
		have_seed = 1;
		if(!eqm_phase_validate_full_balance(phases, nphase, elements, ne, b,
				full_amounts, full_y)){
			status = -22;
			break;
		}
		for(int p = 0; p < nphase; ++p){
			if((mask & (1ULL << p)) && full_amounts[p] <= drop_tol
					&& eqm_phase_active_mask_count(mask, nphase) > 1){
				mask &= ~(1ULL << p);
				blocked_add |= 1ULL << p;
				changed = 1;
				if(eqm_phase_trace_enabled()){
					fprintf(stderr, "FPROPS_EQM_PHASE_TRACE active_set iter=%d drop phase=%d amount=%.17g\n",
						iter, p, full_amounts[p]);
				}
			}
		}
		if(changed){
			continue;
		}
		for(int p = 0; p < nphase; ++p){
			full_active[p] = (mask & (1ULL << p)) ? 1 : 0;
			residuals[p] = NAN;
		}
		if(fprops_eqm_phase_validate_entry_residuals(phases, nphase, elements, ne, T, P,
				full_amounts, full_y, full_active, lambda, residuals)){
			if(eqm_phase_trace_enabled()){
				fprintf(stderr, "FPROPS_EQM_PHASE_TRACE active_set iter=%d mask=%llu validated\n",
					iter, (unsigned long long)mask);
			}
			converged = 1;
			break;
		}
		{
			int best_add = -1;
			double best_phi = 0.0;
			for(int p = 0; p < nphase; ++p){
				if(mask & (1ULL << p)){
					continue;
				}
				if(blocked_add & (1ULL << p)){
					continue;
				}
				if(!isfinite(residuals[p])){
					continue;
				}
				if(best_add < 0 || residuals[p] < best_phi){
					best_add = p;
					best_phi = residuals[p];
				}
			}
			if(best_add >= 0 && best_phi < add_tol){
				mask |= 1ULL << best_add;
				if(eqm_phase_trace_enabled()){
					fprintf(stderr, "FPROPS_EQM_PHASE_TRACE active_set iter=%d add phase=%d phi=%.17g\n",
						iter, best_add, best_phi);
				}
				continue;
			}
		}
		status = -23;
		break;
	}
	if(!converged && eqm_phase_status_ok(status)){
		status = -23;
	}
	if(eqm_phase_status_ok(status)){
		if(phase_amounts_out){
			for(int p = 0; p < nphase; ++p){
				phase_amounts_out[p] = full_amounts[p];
			}
		}
		if(phase_active_out){
			for(int p = 0; p < nphase; ++p){
				phase_active_out[p] = (mask & (1ULL << p)) ? 1 : 0;
			}
		}
		if(phase_y_out){
			for(int i = 0; i < nphase * FPROPS_EQM_PHASE_MAX_VARS; ++i){
				phase_y_out[i] = full_y[i];
			}
		}
		if(member_amounts_out){
			for(int i = 0; i < total_members; ++i){
				member_amounts_out[i] = full_members[i];
			}
		}
	}
	if(nmember_out){
		*nmember_out = total_members;
	}
	return status;
}

int fprops_eqm_phase_solve_auto(const FpropsEqmPhaseModel *phases, int nphase,
		const char **elements, int ne, const double *b, double T, double P,
		const char *algorithm, double *phase_amounts_out, double *phase_y_out,
		int *phase_active_out, double *member_amounts_out, int *nmember_out){
	enum {MAX_AUTO_PHASES = 20};
	FpropsEqmPhaseModel selected[MAX_AUTO_PHASES];
	int selected_index[MAX_AUTO_PHASES];
	double cand_amounts[MAX_AUTO_PHASES];
	double cand_y[MAX_AUTO_PHASES * FPROPS_EQM_PHASE_MAX_VARS];
	double cand_members[MAX_AUTO_PHASES * FPROPS_EQM_PHASE_MAX_MEMBERS];
	double full_amounts[MAX_AUTO_PHASES];
	double full_y[MAX_AUTO_PHASES * FPROPS_EQM_PHASE_MAX_VARS];
	double full_members[MAX_AUTO_PHASES * FPROPS_EQM_PHASE_MAX_MEMBERS];
	int full_active[MAX_AUTO_PHASES];
	double best_amounts[MAX_AUTO_PHASES];
	double best_y[MAX_AUTO_PHASES * FPROPS_EQM_PHASE_MAX_VARS];
	double best_members[MAX_AUTO_PHASES * FPROPS_EQM_PHASE_MAX_MEMBERS];
	int best_active[MAX_AUTO_PHASES];
	unsigned long long nmask;
	unsigned long long mask;
	double best_g = HUGE_VAL;
	int best_active_count = 0;
	int best_status = -99;
	int total_members;
	int found = 0;
	if(!phases || nphase <= 0 || nphase > MAX_AUTO_PHASES || !elements || ne <= 0
			|| !b || !(T > 0.0) || !(P > 0.0)){
		return -11;
	}
	total_members = eqm_phase_total_members(phases, nphase);
	if(total_members < 0 || total_members > MAX_AUTO_PHASES * FPROPS_EQM_PHASE_MAX_MEMBERS){
		return -11;
	}
	eqm_phase_zero_full_outputs(phases, nphase, best_amounts, best_y, best_active, best_members);
	nmask = 1ULL << nphase;
	for(mask = 1ULL; mask < nmask; ++mask){
		int nsel = 0;
		int p;
		int nmember_sel = 0;
		int status;
		int active_count = 0;
		double gtot = HUGE_VAL;
		for(p = 0; p < nphase; ++p){
			if(mask & (1ULL << p)){
				selected[nsel] = phases[p];
				selected_index[nsel] = p;
				++nsel;
			}
		}
		status = fprops_eqm_phase_solve_fixed_expanded(selected, nsel, elements, ne, b, T, P,
			algorithm ? algorithm : "auto", NULL, cand_amounts, cand_y, cand_members, &nmember_sel);
		if(!eqm_phase_status_ok(status)){
			if(eqm_phase_trace_enabled()){
				fprintf(stderr, "FPROPS_EQM_PHASE_TRACE mask=%llu status=%d rejected\n",
					(unsigned long long)mask, status);
			}
			continue;
		}
		eqm_phase_zero_full_outputs(phases, nphase, full_amounts, full_y, full_active,
			full_members);
		{
			int cand_member_off = 0;
			int s;
			for(s = 0; s < nsel; ++s){
				int idx = selected_index[s];
				int full_off = eqm_phase_member_offset(phases, idx);
				int j;
				full_amounts[idx] = cand_amounts[s];
				full_active[idx] = cand_amounts[s] > 1e-10 ? 1 : 0;
				for(j = 0; j < FPROPS_EQM_PHASE_MAX_VARS; ++j){
					full_y[idx * FPROPS_EQM_PHASE_MAX_VARS + j] =
						cand_y[s * FPROPS_EQM_PHASE_MAX_VARS + j];
				}
				for(j = 0; j < phases[idx].nmember; ++j){
					full_members[full_off + j] = cand_members[cand_member_off + j];
				}
				cand_member_off += phases[idx].nmember;
			}
		}
		if(!eqm_phase_validate_full_balance(phases, nphase, elements, ne, b,
				full_amounts, full_y)
				|| !eqm_phase_output_objective(phases, nphase, full_amounts, full_y,
					T, P, &gtot, &active_count)){
			if(eqm_phase_trace_enabled()){
				fprintf(stderr, "FPROPS_EQM_PHASE_TRACE mask=%llu status=%d validation rejected\n",
					(unsigned long long)mask, status);
			}
			continue;
		}
		if(eqm_phase_trace_enabled()){
			fprintf(stderr, "FPROPS_EQM_PHASE_TRACE mask=%llu status=%d active=%d G=%.17g\n",
				(unsigned long long)mask, status, active_count, gtot);
		}
		if(!found || gtot < best_g - 1e-7 * fmax(1.0, fabs(best_g))
				|| (fabs(gtot - best_g) <= 1e-7 * fmax(1.0, fabs(best_g))
					&& active_count < best_active_count)){
			int i;
			found = 1;
			best_g = gtot;
			best_active_count = active_count;
			best_status = status;
			for(i = 0; i < nphase; ++i){
				best_amounts[i] = full_amounts[i];
				best_active[i] = full_active[i];
			}
			for(i = 0; i < nphase * FPROPS_EQM_PHASE_MAX_VARS; ++i){
				best_y[i] = full_y[i];
			}
			for(i = 0; i < total_members; ++i){
				best_members[i] = full_members[i];
			}
		}
	}
	if(!found){
		eqm_phase_zero_full_outputs(phases, nphase, phase_amounts_out, phase_y_out,
			phase_active_out, member_amounts_out);
		if(nmember_out){
			*nmember_out = total_members;
		}
		return best_status;
	}
	if(phase_amounts_out){
		int i;
		for(i = 0; i < nphase; ++i){
			phase_amounts_out[i] = best_amounts[i];
		}
	}
	if(phase_active_out){
		int i;
		for(i = 0; i < nphase; ++i){
			phase_active_out[i] = best_active[i];
		}
	}
	if(phase_y_out){
		int i;
		for(i = 0; i < nphase * FPROPS_EQM_PHASE_MAX_VARS; ++i){
			phase_y_out[i] = best_y[i];
		}
	}
	if(member_amounts_out){
		int i;
		for(i = 0; i < total_members; ++i){
			member_amounts_out[i] = best_members[i];
		}
	}
	if(nmember_out){
		*nmember_out = total_members;
	}
	return best_status;
}
