#include "pantelides.h"

#include <stdio.h>
#include <string.h>

#include <ascend/general/ascMalloc.h>
#include <ascend/general/pairlist.h>
#include <ascend/compiler/instance_enum.h>
#include <ascend/compiler/relation_io.h>
#include <ascend/compiler/instance_io.h>
#include <ascend/system/slv_server.h>
#include <ascend/system/rel.h>
#include <ascend/system/var.h>
#include <ascend/system/diffvars.h>
#include <ascend/system/diffvars_impl.h>

typedef struct PantelidesVarStruct PantelidesVar;
typedef struct PantelidesEqStruct PantelidesEq;

struct PantelidesVarStruct{
	struct var_variable *var;
	char *name;
	PantelidesVar *derivative;
	PantelidesVar *base;
	PantelidesEq *assigned;
	long role;
	long chain_id;
	unsigned generated:1;
	unsigned deleted:1;
	unsigned colored:1;
};

struct PantelidesEqStruct{
	struct rel_relation *rel;
	char *name;
	char *detail;
	struct gl_list_t *edges;
	PantelidesEq *differentiated;
	PantelidesEq *source;
	unsigned order;
	unsigned generated:1;
	unsigned colored:1;
};

typedef struct PantelidesContextStruct{
	slv_system_t sys;
	struct gl_list_t *vars;
	struct gl_list_t *eqs;
	struct pairlist_t *varmap;
	struct pairlist_t *eqmap;
	struct gl_list_t *generated_vars;
	struct gl_list_t *generated_eqs;
	struct gl_list_t *differentiation_order;
} PantelidesContext;

static PantelidesVar *pantelides_var_from_real(PantelidesContext *ctx, const struct var_variable *var){
	unsigned long pos;
	if(ctx == NULL || var == NULL || ctx->varmap == NULL){
		return NULL;
	}
	pos = pairlist_contains(ctx->varmap, (void *)var);
	if(pos == 0){
		return NULL;
	}
	return (PantelidesVar *)pairlist_valueAt(ctx->varmap, pos);
}

static int pantelides_edge_contains(struct gl_list_t *edges, PantelidesVar *var){
	unsigned long i, len;
	if(edges == NULL || var == NULL){
		return 0;
	}
	len = gl_length(edges);
	for(i = 1; i <= len; ++i){
		if((PantelidesVar *)gl_fetch(edges, i) == var){
			return 1;
		}
	}
	return 0;
}

static void pantelides_edge_append_unique(struct gl_list_t *edges, PantelidesVar *var){
	if(edges == NULL || var == NULL){
		return;
	}
	if(!pantelides_edge_contains(edges, var)){
		gl_append_ptr(edges, var);
	}
}

static void pantelides_reset_colours(PantelidesContext *ctx){
	unsigned long i;
	for(i = 1; i <= gl_length(ctx->vars); ++i){
		PantelidesVar *var = (PantelidesVar *)gl_fetch(ctx->vars, i);
		if(var != NULL){
			var->colored = 0;
		}
	}
	for(i = 1; i <= gl_length(ctx->eqs); ++i){
		PantelidesEq *eq = (PantelidesEq *)gl_fetch(ctx->eqs, i);
		if(eq != NULL){
			eq->colored = 0;
		}
	}
}

static int pantelides_augment(PantelidesEq *eq){
	unsigned long i, len;
	if(eq == NULL){
		return 0;
	}
	eq->colored = 1;

	len = gl_length(eq->edges);
	for(i = 1; i <= len; ++i){
		PantelidesVar *var = (PantelidesVar *)gl_fetch(eq->edges, i);
		if(var == NULL || var->deleted){
			continue;
		}
		if(var->assigned == NULL){
			var->assigned = eq;
			return 1;
		}
	}

	for(i = 1; i <= len; ++i){
		PantelidesVar *var = (PantelidesVar *)gl_fetch(eq->edges, i);
		if(var == NULL || var->deleted || var->colored){
			continue;
		}
		var->colored = 1;
		if(var->assigned != NULL && pantelides_augment(var->assigned)){
			var->assigned = eq;
			return 1;
		}
	}
	return 0;
}

static char *pantelides_make_derivative_name(const char *name){
	size_t len;
	char *buf;
	if(name == NULL){
		return NULL;
	}
	len = strlen(name);
	buf = ASC_NEW_ARRAY(char, len + 6);
	sprintf(buf, "der(%s)", name);
	return buf;
}

static PantelidesVar *pantelides_create_generated_derivative(PantelidesContext *ctx, PantelidesVar *base){
	PantelidesVar *var;
	if(ctx == NULL || base == NULL){
		return NULL;
	}
	if(base->derivative != NULL){
		return base->derivative;
	}
	var = ASC_NEW_CLEAR(PantelidesVar);
	if(var == NULL){
		return NULL;
	}
	var->name = pantelides_make_derivative_name(base->name);
	if(var->name == NULL){
		ASC_FREE(var);
		return NULL;
	}
	var->generated = 1;
	var->base = base;
	var->role = (base->role > 0) ? base->role + 1 : 0;
	var->chain_id = base->chain_id;
	base->derivative = var;
	gl_append_ptr(ctx->vars, var);
	gl_append_ptr(ctx->generated_vars, var);
	return var;
}

static char *pantelides_make_eq_name(const PantelidesEq *eq){
	size_t len;
	char *buf;
	if(eq == NULL || eq->name == NULL){
		return NULL;
	}
	len = strlen(eq->name);
	buf = ASC_NEW_ARRAY(char, len + 16);
	sprintf(buf, "d/dt(%s)", eq->name);
	return buf;
}

static char *pantelides_make_eq_detail(const PantelidesEq *eq){
	size_t len;
	char *buf;
	if(eq == NULL || eq->detail == NULL){
		return NULL;
	}
	len = strlen(eq->detail);
	buf = ASC_NEW_ARRAY(char, len + 16);
	sprintf(buf, "d/dt(%s)", eq->detail);
	return buf;
}

static PantelidesEq *pantelides_create_generated_eq(PantelidesContext *ctx, PantelidesEq *source){
	unsigned long i, len;
	PantelidesEq *eq;
	if(ctx == NULL || source == NULL){
		return NULL;
	}
	if(source->differentiated != NULL){
		return source->differentiated;
	}
	eq = ASC_NEW_CLEAR(PantelidesEq);
	if(eq == NULL){
		return NULL;
	}
	eq->name = pantelides_make_eq_name(source);
	eq->detail = pantelides_make_eq_detail(source);
	eq->edges = gl_create(gl_length(source->edges) + 4);
	if(eq->name == NULL || eq->detail == NULL || eq->edges == NULL){
		if(eq->name != NULL)ASC_FREE(eq->name);
		if(eq->detail != NULL)ASC_FREE(eq->detail);
		if(eq->edges != NULL)gl_destroy(eq->edges);
		ASC_FREE(eq);
		return NULL;
	}
	eq->generated = 1;
	eq->source = source;
	eq->order = source->order + 1;
	source->differentiated = eq;

	len = gl_length(source->edges);
	for(i = 1; i <= len; ++i){
		PantelidesVar *var = (PantelidesVar *)gl_fetch(source->edges, i);
		if(var == NULL){
			continue;
		}
		pantelides_edge_append_unique(eq->edges, var);
		if(var->derivative != NULL){
			pantelides_edge_append_unique(eq->edges, var->derivative);
		}
	}

	gl_append_ptr(ctx->eqs, eq);
	gl_append_ptr(ctx->generated_eqs, eq);
	gl_append_ptr(ctx->differentiation_order, source);
	return eq;
}

static int pantelides_build_context(PantelidesContext *ctx, slv_system_t sys){
	struct var_variable **vars;
	struct rel_relation **rels;
	SolverDiffVarCollection *diffvars;
	unsigned long i, nvars, nrels;

	memset(ctx, 0, sizeof(*ctx));
	ctx->sys = sys;
	ctx->vars = gl_create(64);
	ctx->eqs = gl_create(64);
	ctx->varmap = pairlist_create(64);
	ctx->eqmap = pairlist_create(64);
	ctx->generated_vars = gl_create(8);
	ctx->generated_eqs = gl_create(8);
	ctx->differentiation_order = gl_create(8);
	if(ctx->vars == NULL || ctx->eqs == NULL || ctx->varmap == NULL || ctx->eqmap == NULL
			|| ctx->generated_vars == NULL || ctx->generated_eqs == NULL || ctx->differentiation_order == NULL){
		return 1;
	}

	vars = slv_get_solvers_var_list(sys);
	nvars = (unsigned long)slv_get_num_solvers_vars(sys);
	for(i = 0; i < nvars; ++i){
		PantelidesVar *pvar;
		long chain_id = 0;
		if(vars[i] == NULL){
			continue;
		}
		pvar = ASC_NEW_CLEAR(PantelidesVar);
		if(pvar == NULL){
			return 1;
		}
		pvar->var = vars[i];
		pvar->name = var_make_name(sys, vars[i]);
		pvar->role = system_diffvars_var_role(sys, vars[i], &chain_id);
		pvar->chain_id = chain_id;
		if(pvar->name == NULL){
			ASC_FREE(pvar);
			return 1;
		}
		gl_append_ptr(ctx->vars, pvar);
		pairlist_append(ctx->varmap, vars[i], pvar);
	}

	rels = slv_get_solvers_rel_list(sys);
	nrels = (unsigned long)slv_get_num_solvers_rels(sys);
	for(i = 0; i < nrels; ++i){
		PantelidesEq *peq;
		const struct var_variable **inc;
		int j, ninc;
		int len = -1;
		if(rels[i] == NULL || !rel_equal(rels[i])){
			continue;
		}
		peq = ASC_NEW_CLEAR(PantelidesEq);
		if(peq == NULL){
			return 1;
		}
		peq->rel = rels[i];
		peq->name = rel_make_name(sys, rels[i]);
		peq->detail = WriteRelationString(rel_instance(rels[i]), slv_instance(sys), NULL, NULL, relio_ascend, &len);
		peq->edges = gl_create((unsigned long)rel_n_incidences(rels[i]) + 2);
		if(peq->name == NULL || peq->detail == NULL || peq->edges == NULL){
			if(peq->name != NULL)ASC_FREE(peq->name);
			if(peq->detail != NULL)ASC_FREE(peq->detail);
			if(peq->edges != NULL)gl_destroy(peq->edges);
			ASC_FREE(peq);
			return 1;
		}
		inc = rel_incidence_list(rels[i]);
		ninc = rel_n_incidences(rels[i]);
		for(j = 0; j < ninc; ++j){
			PantelidesVar *pvar = pantelides_var_from_real(ctx, inc[j]);
			if(pvar != NULL){
				pantelides_edge_append_unique(peq->edges, pvar);
			}
		}
		gl_append_ptr(ctx->eqs, peq);
		pairlist_append(ctx->eqmap, rels[i], peq);
	}

	diffvars = system_get_diffvars(sys);
	if(diffvars != NULL){
		for(i = 0; i < (unsigned long)diffvars->nseqs; ++i){
			SolverDiffVarSequence *seq = &diffvars->seqs[i];
			int j;
			for(j = 0; j < seq->n - 1; ++j){
				PantelidesVar *base = pantelides_var_from_real(ctx, seq->vars[j]);
				PantelidesVar *deriv = pantelides_var_from_real(ctx, seq->vars[j + 1]);
				if(base != NULL && deriv != NULL){
					base->derivative = deriv;
					deriv->base = base;
				}
			}
		}
	}

	return 0;
}

static void pantelides_free_context(PantelidesContext *ctx){
	unsigned long i;
	if(ctx == NULL){
		return;
	}
	if(ctx->vars != NULL){
		for(i = 1; i <= gl_length(ctx->vars); ++i){
			PantelidesVar *var = (PantelidesVar *)gl_fetch(ctx->vars, i);
			if(var != NULL){
				if(var->name != NULL)ASC_FREE(var->name);
				ASC_FREE(var);
			}
		}
		gl_destroy(ctx->vars);
	}
	if(ctx->eqs != NULL){
		for(i = 1; i <= gl_length(ctx->eqs); ++i){
			PantelidesEq *eq = (PantelidesEq *)gl_fetch(ctx->eqs, i);
			if(eq != NULL){
				if(eq->name != NULL)ASC_FREE(eq->name);
				if(eq->detail != NULL)ASC_FREE(eq->detail);
				if(eq->edges != NULL)gl_destroy(eq->edges);
				ASC_FREE(eq);
			}
		}
		gl_destroy(ctx->eqs);
	}
	if(ctx->varmap != NULL)pairlist_destroy(ctx->varmap);
	if(ctx->eqmap != NULL)pairlist_destroy(ctx->eqmap);
	if(ctx->generated_vars != NULL)gl_destroy(ctx->generated_vars);
	if(ctx->generated_eqs != NULL)gl_destroy(ctx->generated_eqs);
	if(ctx->differentiation_order != NULL)gl_destroy(ctx->differentiation_order);
	memset(ctx, 0, sizeof(*ctx));
}

static int pantelides_run(PantelidesContext *ctx){
	unsigned long initial_eqs, eqi;
	initial_eqs = gl_length(ctx->eqs);
	for(eqi = 1; eqi <= initial_eqs; ++eqi){
		PantelidesEq *current = (PantelidesEq *)gl_fetch(ctx->eqs, eqi);
		int path_found = 0;
		while(!path_found){
			unsigned long i;
			for(i = 1; i <= gl_length(ctx->vars); ++i){
				PantelidesVar *var = (PantelidesVar *)gl_fetch(ctx->vars, i);
				if(var != NULL && !var->deleted && var->derivative != NULL){
					var->deleted = 1;
				}
			}

			pantelides_reset_colours(ctx);
			path_found = pantelides_augment(current);
			if(path_found){
				break;
			}

			for(i = 1; i <= gl_length(ctx->vars); ++i){
				PantelidesVar *var = (PantelidesVar *)gl_fetch(ctx->vars, i);
				if(var != NULL && !var->deleted && var->colored && var->derivative == NULL){
					if(pantelides_create_generated_derivative(ctx, var) == NULL){
						return 1;
					}
				}
			}

			for(i = 1; i <= gl_length(ctx->eqs); ++i){
				PantelidesEq *eq = (PantelidesEq *)gl_fetch(ctx->eqs, i);
				if(eq != NULL && eq->colored && eq->differentiated == NULL){
					if(pantelides_create_generated_eq(ctx, eq) == NULL){
						return 1;
					}
				}
			}

			for(i = 1; i <= gl_length(ctx->vars); ++i){
				PantelidesVar *var = (PantelidesVar *)gl_fetch(ctx->vars, i);
				if(var != NULL && !var->deleted && var->colored && var->assigned != NULL && var->derivative != NULL){
					if(var->assigned->differentiated == NULL){
						return 1;
					}
					var->derivative->assigned = var->assigned->differentiated;
				}
			}

			if(current->differentiated == NULL){
				return 1;
			}
			current = current->differentiated;
		}
	}
	return 0;
}

static void pantelides_write_report(PantelidesContext *ctx, FILE *fp){
	unsigned long i;
	fprintf(fp, "Pantelides advisory report\n");
	fprintf(fp, "==========================\n");
	fprintf(fp, "Variables: %lu\n", (unsigned long)gl_length(ctx->vars));
	fprintf(fp, "Equations: %lu\n", (unsigned long)gl_length(ctx->eqs));
	fprintf(fp, "\n");

	fprintf(fp, "Current derivative chains:\n");
	system_diffvars_debug(ctx->sys, fp);
	fprintf(fp, "\n\n");

	fprintf(fp, "Active equations:\n");
	for(i = 1; i <= gl_length(ctx->eqs); ++i){
		PantelidesEq *eq = (PantelidesEq *)gl_fetch(ctx->eqs, i);
		if(eq == NULL || eq->generated){
			continue;
		}
		fprintf(fp, "  %s: %s\n", eq->name, eq->detail);
	}
	fprintf(fp, "\n");

	fprintf(fp, "Differentiation steps:\n");
	if(gl_length(ctx->differentiation_order) == 0){
		fprintf(fp,
			"  none\n"
			"  No differentiations were suggested from the derivative chains\n"
			"  currently recognised in this system.\n"
		);
	}else{
		for(i = 1; i <= gl_length(ctx->differentiation_order); ++i){
			PantelidesEq *eq = (PantelidesEq *)gl_fetch(ctx->differentiation_order, i);
			if(eq == NULL || eq->differentiated == NULL){
				continue;
			}
			fprintf(fp, "  %lu. Differentiate %s: %s\n", i, eq->name, eq->detail);
			fprintf(fp, "     add %s\n", eq->differentiated->name);
		}
	}
	fprintf(fp, "\n");

	fprintf(fp, "Generated derivative quantities:\n");
	if(gl_length(ctx->generated_vars) == 0){
		fprintf(fp, "  none\n");
	}else{
		for(i = 1; i <= gl_length(ctx->generated_vars); ++i){
			PantelidesVar *var = (PantelidesVar *)gl_fetch(ctx->generated_vars, i);
			if(var == NULL){
				continue;
			}
			fprintf(fp, "  %s", var->name);
			if(var->base != NULL){
				fprintf(fp, "   [from %s]", var->base->name);
			}
			fprintf(fp, "\n");
		}
	}
	fprintf(fp, "\n");

	fprintf(fp, "Generated equations:\n");
	if(gl_length(ctx->generated_eqs) == 0){
		fprintf(fp, "  none\n");
	}else{
		for(i = 1; i <= gl_length(ctx->generated_eqs); ++i){
			PantelidesEq *eq = (PantelidesEq *)gl_fetch(ctx->generated_eqs, i);
			if(eq == NULL){
				continue;
			}
			fprintf(fp, "  %s: %s\n", eq->name, eq->detail);
		}
	}
	fprintf(fp, "\n");
	fprintf(fp,
		"Note: this first-pass report uses the current explicit der(...) chains\n"
		"already present in the system. It does not yet infer additional chain\n"
		"links from ordinary equations such as v = der(x), and it does not yet\n"
		"build symbolic differentiated relations.\n"
	);
}

int integrator_pantelides_advisory(slv_system_t sys, FILE *fp){
	PantelidesContext ctx;
	if(sys == NULL || fp == NULL){
		return 1;
	}
	if(pantelides_build_context(&ctx, sys)){
		pantelides_free_context(&ctx);
		return 1;
	}
	if(pantelides_run(&ctx)){
		pantelides_free_context(&ctx);
		return 1;
	}
	pantelides_write_report(&ctx, fp);
	pantelides_free_context(&ctx);
	return 0;
}
