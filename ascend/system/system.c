/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
	Copyright (C) 2006 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//*
	by Karl Michael Westerberg
	Created: 2/6/90
	Last in CVS: $Revision: 1.29 $ $Date: 2003/01/19 02:16:05 $ $Author: ballan $
*/

#include "system.h"

#include <ascend/general/ascMalloc.h>
#include <ascend/general/list.h>
#include <ascend/general/tm_time.h>
#include <string.h>

#include <ascend/compiler/instance_enum.h>
#include <ascend/compiler/check.h>
#include <ascend/compiler/link.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/instance_io.h>
#include <ascend/compiler/mathinst.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/relation.h>
#include <ascend/compiler/relation_io.h>
#include <ascend/compiler/vlist.h>
#include <ascend/compiler/cmpfunc.h>
#include <ascend/compiler/derivinst.h>
#include <ascend/compiler/visitinst.h>
#include <ascend/compiler/when_util.h>

#include <ascend/linear/mtx.h>

#include "slv_client.h"
#include "diffvars.h"
#include "diffvars_impl.h"

#include "relman.h"
#include "slv_server.h"
#include "analyze.h"
#include "slv_common.h"

//#define ASC_SYSTEM_DEBUG
#ifdef ASC_SYSTEM_DEBUG
# define DOTIME 1
# define MSG CONSOLE_DEBUG
#else
# define MSG(...) 
# define DOTIME 0
#endif

#define IPTR(i) ((struct Instance *) (i))

static struct Instance *system_root_instance(struct Instance *inst){
	if(inst == NULL){
		return NULL;
	}
	if(InstanceKind(inst) == SIM_INST){
		return GetSimulationRoot(inst);
	}
	{
		struct Instance *sim = FindSimulationInstance(inst);
		if(sim != NULL){
			return GetSimulationRoot(sim);
		}
	}
	return inst;
}

static symchar *system_link_entry_key(struct link_entry_t *entry){
	if(entry == NULL){
		return NULL;
	}
	if(entry->key_cache != NULL){
		return entry->key_cache;
	}
	if(entry->u.statptr != NULL){
		return LINKStatKey(entry->u.statptr);
	}
	return NULL;
}

static int system_instance_matches(struct Instance *a, struct Instance *b){
	if(a == NULL || b == NULL){
		return 0;
	}
	return a == b;
}

static void count_link_key(struct gl_list_t *table, symchar *key, int *count){
	unsigned long i, len;
	if(!table) return;
	len = gl_length(table);
	for(i=1; i<=len; ++i){
		struct link_entry_t *entry = (struct link_entry_t *)gl_fetch(table, i);
		symchar *entry_key = system_link_entry_key(entry);
		if(entry_key && CmpSymchar(entry_key, key) == 0){
			(*count)++;
		}
	}
}

struct der_usage_walk {
	int found;
};

static int relation_side_has_der(union RelationTermUnion *side, unsigned long len){
	unsigned long i;
	struct relation_term *term;
	if(side == NULL){
		return 0;
	}
	for(i = 0; i < len; ++i){
		term = A_TERM(&(side[i]));
		if(term != NULL && term->t == e_der){
			return 1;
		}
	}
	return 0;
}

static void find_der_terms_in_relation(struct Instance *inst, VOIDPTR userdata){
	struct der_usage_walk *walk = (struct der_usage_walk *)userdata;
	struct relation *rel;

	if(walk == NULL || walk->found || InstanceKind(inst) != REL_INST){
		return;
	}
	if(GetInstanceRelationType(inst) != e_token){
		return;
	}
	rel = (struct relation *)GetInstanceRelationOnly(inst);
	if(rel == NULL){
		return;
	}

	if(relation_side_has_der(RTOKEN(rel).lhs, RTOKEN(rel).lhs_len)
	    || relation_side_has_der(RTOKEN(rel).rhs, RTOKEN(rel).rhs_len)){
		walk->found = 1;
	}
}

static int system_has_der_terms(struct Instance *inst){
	struct der_usage_walk walk;
	walk.found = 0;
	VisitInstanceTreeTwo(inst, (VisitTwoProc)find_der_terms_in_relation, 0, 0, &walk);
	return walk.found;
}

static int check_ode_independent_links(struct Instance *inst){
	symchar *ode_key = AddSymbol("ode");
	symchar *indep_key = AddSymbol("independent");
	struct gl_list_t *decl = getLinkTableDeclarative(inst);
	struct gl_list_t *proc = getLinkTableProcedural(inst);
	int ode_count = 0;
	int indep_count = 0;
	int has_der_terms = 0;
	struct gl_list_t *indep_instances;
	unsigned long i;

	count_link_key(decl, ode_key, &ode_count);
	count_link_key(proc, ode_key, &ode_count);
	count_link_key(decl, indep_key, &indep_count);
	count_link_key(proc, indep_key, &indep_count);
	has_der_terms = system_has_der_terms(inst);

	if((ode_count > 0 || has_der_terms) && indep_count != 1){
		ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
		FPRINTF(ASCERR,"ODE model requires exactly one INDEPENDENT variable; found %d.\n", indep_count);
		error_reporter_end_flush();
		return 1;
	}
	/* verify that no DER entries reference the independent variable */
	indep_instances = gl_create(4);
	if(decl){
		unsigned long len = gl_length(decl);
		for(i=1;i<=len;i++){
			struct link_entry_t *entry = (struct link_entry_t *)gl_fetch(decl,i);
			symchar *entry_key = system_link_entry_key(entry);
			if(entry_key && CmpSymchar(entry_key, indep_key) == 0){
				CONST struct gl_list_t *instances = getLinkInstances(inst, entry, 0);
				if(instances){
					unsigned long j, n = gl_length((struct gl_list_t *)instances);
					for(j=1;j<=n;j++){
						gl_append_ptr(indep_instances, gl_fetch((struct gl_list_t *)instances, j));
					}
				}
			}
		}
	}
	if(proc){
		unsigned long len = gl_length(proc);
		for(i=1;i<=len;i++){
			struct link_entry_t *entry = (struct link_entry_t *)gl_fetch(proc,i);
			symchar *entry_key = system_link_entry_key(entry);
			if(entry_key && CmpSymchar(entry_key, indep_key) == 0){
				CONST struct gl_list_t *instances = getLinkInstances(inst, entry, 0);
				if(instances){
					unsigned long j, n = gl_length((struct gl_list_t *)instances);
					for(j=1;j<=n;j++){
						gl_append_ptr(indep_instances, gl_fetch((struct gl_list_t *)instances, j));
					}
				}
			}
		}
	}

	if(gl_length(indep_instances) > 0){
		struct gl_list_t *tables[2] = {decl, proc};
		for(int t=0;t<2;t++){
			struct gl_list_t *table = tables[t];
			if(!table) continue;
			unsigned long len = gl_length(table);
			for(i=1;i<=len;i++){
				struct link_entry_t *entry = (struct link_entry_t *)gl_fetch(table,i);
				symchar *entry_key = system_link_entry_key(entry);
				if(entry_key && CmpSymchar(entry_key, ode_key) == 0){
					CONST struct gl_list_t *instances = getLinkInstances(inst, entry, 0);
					if(instances){
						unsigned long j, k, n = gl_length((struct gl_list_t *)instances);
						for(j=1;j<=n;j++){
							struct Instance *odeinst = (struct Instance *)gl_fetch((struct gl_list_t *)instances, j);
							for(k=1;k<=gl_length(indep_instances);k++){
								struct Instance *indepinst = (struct Instance *)gl_fetch(indep_instances, k);
								if(system_instance_matches(indepinst, odeinst)){
									ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
									FPRINTF(ASCERR,"DER uses independent variable.\n");
									error_reporter_end_flush();
									gl_destroy(indep_instances);
									return 1;
								}
							}
						}
					}
				}
			}
		}
	}
	gl_destroy(indep_instances);
	return 0;
}

void system_set_build_mode(SlvBackendToken inst, SystemBuildMode mode){
	struct Instance *root = system_root_instance(IPTR(inst));
	if(root == NULL){
		return;
	}
	SetInitialRelationInclusion(root, mode == SYSTEM_BUILD_INITIAL ? TRUE : FALSE);
}

static slv_system_t system_build_internal(SlvBackendToken inst){
  slv_system_t sys;
  int stat;

#if DOTIME
  double comptime;
  comptime = tm_cpu_time();
#endif

  sys = slv_create();

  if (set_solver_types()) {
    system_destroy(sys);
    sys = NULL;
    return sys;
  }
  /* THIS MEANS I NEED THE boolean_var DEFINITION IN system.c  */
  if (set_boolean_types()){
    system_destroy(sys);
    sys = NULL;
    return sys;
  }
  stat = analyze_make_problem(sys,IPTR(inst));
  if(stat){
    system_destroy(sys);
    sys = NULL;
    if(stat==2) {
      ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
      FPRINTF(ASCERR,"Models sent to solver: \n");
      FPRINTF(ASCERR,"1 cannot have any pending parts\n");
      FPRINTF(ASCERR,"2 cannot have NULL or unfinished relations.\n");
      FPRINTF(ASCERR,"3 must have at least one variable.\n");
      FPRINTF(ASCERR,"4 must have at least one objective or relation.\n");
      FPRINTF(ASCERR,"5 must have at all WHEN-controlling values initialized.\n");
      FPRINTF(ASCERR,"Check pendings and problem structure.\n");
      error_reporter_end_flush();
    }
    return sys;
  }

  slv_set_instance(sys,inst);

#if DOTIME
  comptime = tm_cpu_time() - comptime;
  if(comptime >= 2){
    MSG("System built (time %5.3f s)",comptime);
  }else if(comptime > 10e-3){
    MSG("System built (time %5.3f ms)",comptime*1e3);
  }else{
    MSG("System built (time %0.0f us)",comptime*1e6);
  }
#endif
  if(check_ode_independent_links(IPTR(inst))){
    system_destroy(sys);
    sys = NULL;
    return sys;
  }
  return(sys);
}

static char *system_debug_instance_name(CONST struct Instance *inst){
	char *name;
	if(inst == NULL){
		name = ASC_NEW_ARRAY(char, 7);
		strcpy(name, "<null>");
		return name;
	}
	if(IsDerivativeInstance(inst)){
		struct Instance *base = DerivativeInstanceBase(inst);
		char *basename = system_debug_instance_name(base);
		size_t len = strlen(basename) + 6;
		name = ASC_NEW_ARRAY(char, len);
		SNPRINTF(name, len, "der(%s)", basename);
		ASC_FREE(basename);
		return name;
	}
	name = WriteInstanceNameString(inst, NULL);
	if(name == NULL || name[0] == '\0'){
		if(name != NULL)ASC_FREE(name);
		name = ASC_NEW_ARRAY(char, 10);
		strcpy(name, "<unnamed>");
	}
	return name;
}

static void system_write_precheck_dae_report(FILE *fp, slv_system_t sys){
	const SolverDiffVarCollection *diffvars;
	struct var_variable **vars;
	struct rel_relation **rels;
	long i;
	int nvars, nrels;

	diffvars = system_get_diffvars(sys);
	FPRINTF(fp, "PRECHECK DAE REPORT\n\n");
	if(diffvars == NULL){
		FPRINTF(fp, "NO DIFFVARS AVAILABLE\n");
		return;
	}

	FPRINTF(fp, "DIFFVAR COUNTS\n");
	FPRINTF(fp, "  sequences: %ld\n", diffvars->nseqs);
	FPRINTF(fp, "  states: %ld\n", diffvars->ndiff);
	FPRINTF(fp, "  algebraics: %ld\n", diffvars->nalg);
	FPRINTF(fp, "  independents: %ld\n", diffvars->nindep);
	FPRINTF(fp, "  observations: %ld\n", diffvars->nobs);
	FPRINTF(fp, "  maxorder: %ld\n", diffvars->maxorder);

	FPRINTF(fp, "\nINDEPENDENT VARIABLES (%ld)\n", diffvars->nindep);
	for(i = 0; i < diffvars->nindep; ++i){
		char *name = system_debug_instance_name((struct Instance *)var_instance(diffvars->indep[i]));
		FPRINTF(fp, "  %ld: %s\n", i, name);
		ASC_FREE(name);
	}

	nvars = slv_get_num_solvers_vars(sys);
	vars = slv_get_solvers_var_list(sys);
	FPRINTF(fp, "\nSOLVER VARIABLES (%d)\n", nvars);
	for(i = 0; i < nvars; ++i){
		long ode_id = 0;
		long role = system_diffvars_var_role(sys, vars[i], &ode_id);
		const char *role_name = "algebraic";
		char *name = system_debug_instance_name((struct Instance *)var_instance(vars[i]));
		if(role < 0){
			role_name = "independent";
		}else if(role == 1){
			role_name = "state";
		}else if(role > 1){
			role_name = "derivative";
		}
		FPRINTF(fp, "  %ld: %s [role=%s", i, name, role_name);
		if(role > 0){
			FPRINTF(fp, ", order=%ld, ode_id=%ld", role, ode_id);
		}
		FPRINTF(fp, ", sindex=%d", var_sindex(vars[i]));
		if(var_deriv(vars[i]))FPRINTF(fp, ", deriv");
		if(var_fixed(vars[i]))FPRINTF(fp, ", fixed");
		if(!var_active(vars[i]))FPRINTF(fp, ", inactive");
		if(!var_incident(vars[i]))FPRINTF(fp, ", not-incident");
		FPRINTF(fp, "]\n");
		ASC_FREE(name);
	}

	FPRINTF(fp, "\nDERIVATIVE CHAINS\n");
	FPRINTF(fp, "Derivative chains in slv_system...\n");
	for(i = 0; i < diffvars->nseqs; ++i){
		int j;
		SolverDiffVarSequence seq = diffvars->seqs[i];
		FPRINTF(fp, "%ld: ", i);
		for(j = 0; j < seq.n; ++j){
			char *name = system_debug_instance_name((struct Instance *)var_instance(seq.vars[j]));
			if(j)FPRINTF(fp, " <-- ");
			FPRINTF(fp, "%d: (%p)'%s'", var_sindex(seq.vars[j]), (void *)seq.vars[j], name);
			ASC_FREE(name);
		}
		FPRINTF(fp, "\n");
	}

	nrels = slv_get_num_solvers_rels(sys);
	rels = slv_get_solvers_rel_list(sys);
	FPRINTF(fp, "\nSOLVER RELATIONS (%d)\n", nrels);
	for(i = 0; i < nrels; ++i){
		char *name = system_debug_instance_name((struct Instance *)rel_instance(rels[i]));
		char *expr = WriteRelationString(rel_instance(rels[i]), NULL, NULL, NULL, relio_ascend, NULL);
		FPRINTF(fp, "  %ld: [%s] %s", i, rel_classify_differential(rels[i]) ? "differential" : "algebraic", name);
		if(expr != NULL){
			FPRINTF(fp, " :: %s", expr);
			ASC_FREE(expr);
		}
		FPRINTF(fp, "\n");
		ASC_FREE(name);
	}
}

int system_debug_precheck_dae(SlvBackendToken inst, FILE *fp){
	slv_system_t sys;
	int stat;

	if(inst == NULL || fp == NULL){
		return 1;
	}

	sys = slv_create();
	if(sys == NULL){
		return 1;
	}
	if(set_solver_types() || set_boolean_types()){
		system_destroy(sys);
		return 1;
	}

	stat = analyze_make_problem(sys, IPTR(inst));
	if(stat){
		system_destroy(sys);
		return stat;
	}

	slv_set_instance(sys, inst);
	system_write_precheck_dae_report(fp, sys);
	system_destroy(sys);
	return 0;
}

slv_system_t system_build_with_mode(SlvBackendToken inst, SystemBuildMode mode){
	system_set_build_mode(inst, mode);
	return system_build_internal(inst);
}

slv_system_t system_build(SlvBackendToken inst){
	return system_build_with_mode(inst, SYSTEM_BUILD_NORMAL);
}

void system_destroy(slv_system_t sys){
	struct gl_list_t *symbollist;
	void *l;

#define FN(FUNCNAME) \
		l=(void*)FUNCNAME(sys); if(l!=NULL)ASC_FREE(l);
#define F(N) FN(slv_get_master_##N##_list)
	F(var); F(par); F(unattached); F(dvar); F(disunatt); F(rel);
	F(condrel); F(obj); F(logrel); F(condlogrel); F(when); F(bnd);
#undef F

#define F(N) FN(slv_get_solvers_##N##_list)
	F(var); F(par); F(unattached); F(dvar); F(disunatt); F(rel);
	F(condrel); F(obj); F(logrel); F(condlogrel); F(when); F(bnd);
#undef F
#undef FN

	system_diffvars_destroy(sys);

	symbollist=slv_get_symbol_list(sys);
	if(symbollist != NULL)DestroySymbolValuesList(symbollist);
	slv_destroy_classifier_artifacts(sys);

	slv_set_solvers_blocks(sys,0,NULL);
	slv_set_solvers_log_blocks(sys,0,NULL);	/* free blocks lists */
	slv_destroy(sys); /* frees buf data */
}

void system_free_reused_mem(){
  mtx_free_reused_mem();
  linsolqr_free_reused_mem();
  analyze_free_reused_mem();
  relman_free_reused_mem();
}
