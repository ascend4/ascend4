#include "ida.h"
#include "idalinear.h"
#include "idaanalyse.h"
#include "idatypes.h"
#include "idaprec.h"
#include "idacalc.h"
#include "idaio.h"
#include "idaboundary.h"
#include <stdio.h>
#include <stdlib.h>

#include <ascend/general/platform.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/general/list.h>
#include <ascend/general/panic.h>

#include <ascend/solver/solver.h>

#include <ascend/system/conditional.h>
#include <ascend/system/slv_client.h>
#include <ascend/system/cond_config.h>
#include <ascend/system/discrete.h>
#include <ascend/system/slv_types.h>
#include <ascend/system/slv_common.h>
#include <ascend/system/logrel.h>
#include <ascend/system/rel.h>
#include <ascend/system/system_impl.h>

#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/packages.h>

static int ida_collect_active_guardroots_in_when(struct w_when *when, struct gl_list_t *roots, struct gl_list_t *contexts){
	struct gl_list_t *cases;
	struct Instance *wheninst;
	struct Instance *context;
	unsigned long c, ncases;

	if(when == NULL || roots == NULL || contexts == NULL){
		return 1;
	}

	wheninst = (struct Instance *)when->instance;
	context = wheninst != NULL ? InstanceParent(wheninst, 1) : NULL;
	if(context == NULL){
		context = wheninst;
	}

	cases = when->cases;
	if(cases == NULL){
		return 0;
	}

	ncases = gl_length(cases);
	for(c = 1; c <= ncases; ++c){
		struct when_case *solver_case = (struct when_case *)gl_fetch(cases, c);
		struct gl_list_t *reinit_list;
		unsigned long r, nr;

		if(solver_case == NULL || !(solver_case->flags & WHEN_CASE_ACTIVE)){
			continue;
		}

		reinit_list = solver_case->reinits;
		if(reinit_list != NULL){
			nr = gl_length(reinit_list);
			for(r = 1; r <= nr; ++r){
				struct when_reinit *wr = (struct when_reinit *)gl_fetch(reinit_list, r);
				const struct Expr *guard = wr != NULL ? wr->guard : NULL;
				if(guard != NULL && integrator_direct_guard_rootable(guard)){
					gl_append_ptr(roots, wr);
					gl_append_ptr(contexts, context);
				}
			}
		}

		if(solver_case->whens != NULL){
			unsigned long w, nw = gl_length(solver_case->whens);
			for(w = 1; w <= nw; ++w){
				struct w_when *nested = (struct w_when *)gl_fetch(solver_case->whens, w);
				if(ida_collect_active_guardroots_in_when(nested, roots, contexts) != 0){
					return 1;
				}
			}
		}
	}

	return 0;
}

int ida_refresh_event_roots(IntegratorSystem *integ){
	IntegratorIdaData *enginedata;
	struct gl_list_t *roots = NULL, *contexts = NULL;
	struct w_when **whenlist;
	int nwhens, i;

	if(integ == NULL){
		return 1;
	}

	enginedata = integrator_ida_enginedata(integ);
	enginedata->bndlist = slv_get_solvers_bnd_list(integ->system);
	enginedata->nbnds = slv_get_num_solvers_bnds(integ->system);

	if(enginedata->guardroots != NULL){
		ASC_FREE(enginedata->guardroots);
		enginedata->guardroots = NULL;
	}
	if(enginedata->guardcontexts != NULL){
		ASC_FREE(enginedata->guardcontexts);
		enginedata->guardcontexts = NULL;
	}
	enginedata->nguardroots = 0;
	enginedata->nroots = enginedata->nbnds;

	roots = gl_create(4);
	contexts = gl_create(4);
	if(roots == NULL || contexts == NULL){
		if(roots != NULL)gl_destroy(roots);
		if(contexts != NULL)gl_destroy(contexts);
		return 1;
	}

	whenlist = integ->system->whens.solver;
	nwhens = integ->system->whens.snum;
	for(i = 0; i < nwhens; ++i){
		if(whenlist[i] != NULL && ida_collect_active_guardroots_in_when(whenlist[i], roots, contexts) != 0){
			gl_destroy(roots);
			gl_destroy(contexts);
			return 1;
		}
	}

	enginedata->nguardroots = (int)gl_length(roots);
	if(enginedata->nguardroots > 0){
		unsigned long n = (unsigned long)enginedata->nguardroots;
		unsigned long k;
		enginedata->guardroots = ASC_NEW_ARRAY(struct when_reinit *, n);
		enginedata->guardcontexts = ASC_NEW_ARRAY(struct Instance *, n);
		if(enginedata->guardroots == NULL || enginedata->guardcontexts == NULL){
			if(enginedata->guardroots != NULL){
				ASC_FREE(enginedata->guardroots);
				enginedata->guardroots = NULL;
			}
			if(enginedata->guardcontexts != NULL){
				ASC_FREE(enginedata->guardcontexts);
				enginedata->guardcontexts = NULL;
			}
			gl_destroy(roots);
			gl_destroy(contexts);
			return 1;
		}
		for(k = 0; k < n; ++k){
			enginedata->guardroots[k] = (struct when_reinit *)gl_fetch(roots, k + 1);
			enginedata->guardcontexts[k] = (struct Instance *)gl_fetch(contexts, k + 1);
		}
	}

	enginedata->nroots = enginedata->nbnds + enginedata->nguardroots;

	gl_destroy(roots);
	gl_destroy(contexts);
	return 0;
}

int ida_reinit_integrator(IntegratorSystem *integ, void *ida_mem, realtype tout1);

#ifndef IDA_BND_DEBUG
# define IDA_BND_DEBUG 0
#endif

static void ida_sync_discretes_to_instances(slv_system_t sys){
	struct dis_discrete **dvars;
	int i, ndvars;

	if(sys == NULL){
		return;
	}

	dvars = slv_get_solvers_dvar_list(sys);
	ndvars = slv_get_num_solvers_dvars(sys);

	for(i = 0; i < ndvars; ++i){
		struct dis_discrete *dvar = dvars[i];
		struct Instance *inst;

		if(dvar == NULL || dis_const(dvar)){
			continue;
		}

		inst = (struct Instance *)dvar->datom;
		if(inst == NULL){
			continue;
		}

		switch(dis_kind(dvar)){
		case e_dis_boolean_t:
			SetBooleanAtomValue(inst, dis_value(dvar) ? 1 : 0, 0);
			break;
		case e_dis_integer_t:
			SetIntegerAtomValue(inst, dis_value(dvar), 0);
			break;
		case e_dis_symbol_t:
		default:
			break;
		}
	}
}

static int ida_event_has_logical_solver(slv_system_t sys){
	int selected;

	if(sys == NULL){
		return 0;
	}

	selected = slv_get_selected_solver(sys);
	if(selected < 0){
		return 0;
	}

	return strcmp(slv_solver_name(selected), "LRSlv") == 0;
}

int ida_hybrid_trace_enabled(void){
	const char *env = getenv("ASCEND_HYBRID_TRACE");
	return env != NULL && env[0] != '\0' && strcmp(env, "0") != 0;
}

void ida_hybrid_trace(IntegratorSystem *integ, const char *label, realtype t){
	slv_system_t sys;
	struct dis_discrete **dvars;
	int ndvars, nwhens, i;
	int selected;

	if(!ida_hybrid_trace_enabled() || integ == NULL || integ->system == NULL){
		return;
	}

	sys = integ->system;
	selected = slv_get_selected_solver(sys);
	FPRINTF(ASCERR, "[HYBRID] %s t=%.17g solver=%s\n",
		label != NULL ? label : "(null)",
		t,
		selected >= 0 ? slv_solver_name(selected) : "(none)");

	dvars = slv_get_solvers_dvar_list(sys);
	ndvars = slv_get_num_solvers_dvars(sys);
	for(i = 0; i < ndvars; ++i){
		struct dis_discrete *dvar = dvars[i];
		char *name;

		if(dvar == NULL || !dis_inwhen(dvar)){
			continue;
		}
		name = dis_make_name(sys, dvar);
		FPRINTF(ASCERR, "[HYBRID]   dvar %s kind=%d value=%ld prev=%ld active=%d\n",
			name != NULL ? name : "(unnamed)",
			(int)dis_kind(dvar),
			(long)dis_value(dvar),
			(long)dis_previous_value(dvar),
			(int)dis_active(dvar));
		if(name != NULL){
			ASC_FREE(name);
		}
	}

	nwhens = sys->whens.snum;
	for(i = 0; i < nwhens; ++i){
		struct w_when *when = sys->whens.solver[i];
		struct gl_list_t *cases;
		unsigned long c, clen;
		char *name;

		if(when == NULL){
			continue;
		}
		name = when_make_name(sys, when);
		FPRINTF(ASCERR, "[HYBRID]   when %s\n", name != NULL ? name : "(unnamed)");
		if(name != NULL){
			ASC_FREE(name);
		}
		cases = when->cases;
		clen = cases != NULL ? gl_length(cases) : 0;
		for(c = 1; c <= clen; ++c){
			struct when_case *wc = (struct when_case *)gl_fetch(cases, c);
			if(wc != NULL && (wc->flags & WHEN_CASE_ACTIVE)){
				FPRINTF(ASCERR, "[HYBRID]     active case #%ld local_case=%ld\n",
					(long)c, (long)wc->case_number);
			}
		}
	}
}

typedef struct IdaDiscreteSnapshotEntry{
	struct dis_discrete *dvar;
	int32 value;
} IdaDiscreteSnapshotEntry;

typedef struct IdaDiscreteSnapshot{
	IdaDiscreteSnapshotEntry *entries;
	int count;
} IdaDiscreteSnapshot;

static void ida_discrete_snapshot_clear(IdaDiscreteSnapshot *snapshot){
	if(snapshot == NULL){
		return;
	}
	if(snapshot->entries != NULL){
		ASC_FREE(snapshot->entries);
		snapshot->entries = NULL;
	}
	snapshot->count = 0;
}

static int ida_discrete_snapshot_capture(slv_system_t sys, IdaDiscreteSnapshot *snapshot){
	struct dis_discrete **dvars;
	int ndvars, i, count;

	if(sys == NULL || snapshot == NULL){
		return 1;
	}

	ida_discrete_snapshot_clear(snapshot);
	dvars = slv_get_solvers_dvar_list(sys);
	ndvars = slv_get_num_solvers_dvars(sys);

	count = 0;
	for(i = 0; i < ndvars; ++i){
		if(dvars[i] != NULL && dis_inwhen(dvars[i])){
			++count;
		}
	}

	if(count == 0){
		return 0;
	}

	snapshot->entries = ASC_NEW_ARRAY(IdaDiscreteSnapshotEntry, count);
	if(snapshot->entries == NULL){
		return 1;
	}

	snapshot->count = count;
	count = 0;
	for(i = 0; i < ndvars; ++i){
		if(dvars[i] != NULL && dis_inwhen(dvars[i])){
			snapshot->entries[count].dvar = dvars[i];
			snapshot->entries[count].value = dis_value(dvars[i]);
			++count;
		}
	}

	return 0;
}

static int ida_discrete_snapshot_changed(const IdaDiscreteSnapshot *snapshot){
	int i;

	if(snapshot == NULL){
		return 0;
	}

	for(i = 0; i < snapshot->count; ++i){
		if(snapshot->entries[i].dvar != NULL
			&& dis_value(snapshot->entries[i].dvar) != snapshot->entries[i].value){
			return 1;
		}
	}

	return 0;
}

#if IDA_BND_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(...)
#endif
/*
 *
 *
 * Check to see if and of the system discrete variables have changed.
 *
 * @return 1 if any of the values have changed (does not necessarily mean
 * 			 the system needs to be reconfigured)
 */
int some_dis_vars_changed(slv_system_t sys) {
	struct dis_discrete **dvlist, *cur_dis;
	int numDVs, i, ret;
	char *dis_name;

	dvlist = slv_get_solvers_dvar_list(sys);
	numDVs = slv_get_num_solvers_dvars(sys);

	ret = 0;
	for (i = 0; i < numDVs; i++) {
		cur_dis = dvlist[i];


#ifdef IDA_BND_DEBUG
		dis_name = dis_make_name(sys, cur_dis);
		MSG("Boundary %s index, current, prev = %d, %d, %d ", dis_name,
				i, dis_value(cur_dis), dis_previous_value(cur_dis));
		ASC_FREE(dis_name);
#endif

		if (dis_inwhen(cur_dis)) {
			if (dis_value(cur_dis) != dis_previous_value(cur_dis)) {
				ret = 1;
			}
		}
	}

	return ret;

}

int ida_setup_lrslv(IntegratorSystem *integ) {
	slv_parameters_t parameters;
	slv_status_t status;
	int i, num_params, slv_index;
	char *pname;

	/* Setup the logical solver */
	if(package_load("lrslv", NULL) != 0){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to load LRSlv for IDA boundary handling");
		return 1;
	}
	slv_index = slv_lookup_client("LRSlv");
	if (slv_index < 0) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"LRSlv is unavailable for IDA boundary handling");
		return 2;
	}
	if (slv_select_solver(integ->system, slv_index) == -1) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error attempting to load LRSlv");
		return 3;
	}

#ifdef IDA_BND_DEBUG
	MSG("Solver selected is '%s'",slv_solver_name
			(slv_get_selected_solver(integ->system)));
#endif

	/* Flip LRSlv into ida mode */
	slv_get_parameters(integ->system, &parameters);
	num_params = parameters.num_parms;
	for (i = 0; i<num_params; i++) {
		pname = parameters.parms[i].name;
		if(strcmp(pname, "withida") == 0) {
			parameters.parms[i].info.b.value = 1;
		}
	}

		/* solve the initial logical states */
		slv_presolve(integ->system);
		slv_solve(integ->system);
		ida_sync_discretes_to_instances(integ->system);

		/* Check for convergence */
		slv_get_status(integ->system, &status);
		if (!status.converged) {
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Non-convergence in logical solver at"
					"intialisation");
			return 4;
		}

		if(some_dis_vars_changed(integ->system)) {
			return ida_bnd_reanalyse(integ);
		}

	return 0;
}

int ida_bnd_reanalyse(IntegratorSystem *integ){
	IntegratorIdaData *enginedata;

	if (integ->y_id != NULL) {
		ASC_FREE(integ->y_id);
		integ->y_id = NULL;
	}

	if (integ->obs_id != NULL){
		ASC_FREE(integ->obs_id);
		integ->obs_id = NULL;
	}
	if (integ->y != NULL) {
		ASC_FREE(integ->y);
		integ->y = NULL;
	}
	if (integ->ydot != NULL) {
		ASC_FREE(integ->ydot);
		integ->ydot = NULL;
	}
	if (integ->obs != NULL) {
		ASC_FREE(integ->obs);
		integ->obs = NULL;
	}

	integ->n_y = 0;


	integrator_ida_analyse(integ);
	enginedata = integrator_ida_enginedata(integ);
	(void)enginedata;
	return ida_refresh_event_roots(integ);
}

int ida_bnd_event_iterate(IntegratorSystem *integ, void *ida_mem, realtype tout1){
	slv_status_t status;
	int iter;
	const int max_iter = 20;
	struct gl_list_t *applied_reinits;
	int need_consistency = 1;
	int need_logical_solve = 0;
	IdaDiscreteSnapshot dshot = {NULL, 0};

	applied_reinits = gl_create(8);
	if(applied_reinits == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,
			"Unable to allocate REINIT tracking state for event iteration");
		return 1;
	}

	for(iter = 0; iter < max_iter; ++iter){
		int nreinits;

		if(need_logical_solve){
			int already_solved = 0;

			if(ida_discrete_snapshot_capture(integ->system, &dshot) != 0){
				gl_destroy(applied_reinits);
				return 1;
			}
			if(!ida_event_has_logical_solver(integ->system)){
				if(ida_setup_lrslv(integ) != 0){
					ida_discrete_snapshot_clear(&dshot);
					gl_destroy(applied_reinits);
					return 1;
				}
				already_solved = 1;
			}
			if(!already_solved){
				slv_presolve(integ->system);
				slv_solve(integ->system);
				ida_sync_discretes_to_instances(integ->system);
				slv_get_status(integ->system, &status);
				if(!status.converged){
					ida_discrete_snapshot_clear(&dshot);
					ERROR_REPORTER_HERE(ASC_PROG_ERR,
						"Non-convergence in logical solver during event iteration");
					gl_destroy(applied_reinits);
					return 1;
				}
			}
			if(ida_discrete_snapshot_changed(&dshot)){
				ida_discrete_snapshot_clear(&dshot);
				if(ida_bnd_reanalyse(integ) != 0){
					gl_destroy(applied_reinits);
					return 1;
				}
				need_consistency = 1;
			}else{
				ida_discrete_snapshot_clear(&dshot);
			}
			need_logical_solve = 0;
		}

		nreinits = integrator_apply_reinits_tracked(integ, applied_reinits);
		if(nreinits < 0){
			gl_destroy(applied_reinits);
			return 1;
		}
		if(nreinits > 0){
			need_consistency = 1;
			if(ida_bnd_reanalyse(integ) != 0){
				gl_destroy(applied_reinits);
				return 1;
			}
		}
		if(!need_consistency){
			gl_destroy(applied_reinits);
			return 0;
		}

		{
			realtype reinit_tout = tout1;
			realtype t0 = integrator_get_t(integ);
			if(reinit_tout <= t0 + 1e-4){
				reinit_tout = t0 + 1e-4;
			}
			if(ida_bnd_update_relist(integ) != 0){
				gl_destroy(applied_reinits);
				return 1;
			}
			if(ida_reinit_integrator(integ, ida_mem, reinit_tout) != 0){
				gl_destroy(applied_reinits);
				return 1;
			}
		}
		need_consistency = 0;
		need_logical_solve = ida_event_has_logical_solver(integ->system);
	}

	ERROR_REPORTER_HERE(ASC_PROG_ERR,
		"Event iteration did not converge after %d iterations (possible state cycle)",
		max_iter);
	ida_discrete_snapshot_clear(&dshot);
	gl_destroy(applied_reinits);
	return 1;
}

int ida_bnd_update_relist(IntegratorSystem *integ){
	IntegratorIdaData *enginedata;
	struct rel_relation **rels;
	char *relname;
	int i,j,n_solverrels,n_active_rels;

	enginedata = integrator_ida_enginedata(integ);

	n_solverrels = slv_get_num_solvers_rels(integ->system);
	n_active_rels = slv_count_solvers_rels(integ->system, &integrator_ida_rel);
	rels = slv_get_solvers_rel_list(integ->system);

	if(enginedata->rellist != NULL){
		ASC_FREE(enginedata->rellist);
		enginedata->rellist = NULL;
		enginedata->rellist = ASC_NEW_ARRAY(struct rel_relation *, n_active_rels);
	}

	j = 0;
	for (i = 0; i < n_solverrels; ++i) {
		if (rel_apply_filter(rels[i], &integrator_ida_rel)) {
#ifdef IDA_BND_DEBUG
			relname = rel_make_name(integ->system, rels[i]);
			MSG("rel '%s': 0x%x", relname, rel_flags(rels[i]));
			ASC_FREE(relname);
#endif
			enginedata->rellist[j++] = rels[i];
		}
	}
	asc_assert(j == n_active_rels);
	enginedata->nrels = n_active_rels;

	if (enginedata->nrels != integ->n_y) {
		ERROR_REPORTER_HERE(ASC_USER_ERROR
				,"Integration problem is not square (%d active rels, %d vars)"
				,n_active_rels, integ->n_y
		);
		return 1; /* failure */
	}

	return 0;
}

N_Vector ida_bnd_new_zero_NV(IntegratorSystem *integ, long int vec_length){
	int i;
#if SUNDIALS_VERSION_MAJOR >= 6
	IntegratorIdaData *enginedata = integrator_ida_enginedata(integ);
	N_Vector nv = N_VNew_Serial(vec_length, enginedata->sunctx);
#else
	N_Vector nv = N_VNew_Serial(vec_length);
#endif
	for(i= 0; i< vec_length; i++) {
		NV_Ith_S(nv,i) = 0.0;
	}

	return nv;
}


void ida_bnd_update_IC(IntegratorSystem *integ, realtype t0, N_Vector y0, N_Vector yp0) {
	/* First destroy since n_y may have changed */
	N_VDestroy_Serial(y0);
	N_VDestroy_Serial(yp0);
	/* retrieve new initial values from the system */
	t0 = integrator_get_t(integ);
	y0 = ida_bnd_new_zero_NV(integ, integ->n_y);
	integrator_get_y(integ, NV_DATA_S(y0));

	yp0 = ida_bnd_new_zero_NV(integ, integ->n_y);
	integrator_get_ydot(integ, NV_DATA_S(yp0));

#ifdef IDA_BND_DEBUG
	MSG("BEFORE IC SOLVING:");
	MSG("TIME: %f", t0);
	MSG("Y");
	N_VPrint_Serial(y0);
	MSG("Yp");
	N_VPrint_Serial(yp0);
	MSG("Press any to continue...");
	getchar();
#endif

}

/*
 * Uses LRSlv to check if any of the logical conditions in the model need
 * updating after a boundary crossing. If so, the model is reconfigured and
 * new initial conditions are determined at the point of the crossing.
 *
 * @return 1 if the crossing causes a change in the system
 */

int ida_cross_boundary(IntegratorSystem *integ, int *rootsfound,
		int *bnd_cond_states) {

	IntegratorIdaData *enginedata;
	slv_status_t status;

	int i, num_bnds;
	int any_crossed = 0;
	IdaDiscreteSnapshot dshot = {NULL, 0};

	/* Flag the crossed boundary and update bnd_cond_states */
	enginedata = integ->enginedata;
	num_bnds = enginedata->nbnds;
	for (i = 0; i < num_bnds; i++) {
		if (rootsfound[i]) {
			struct bnd_boundary *bnd;
			if(!any_crossed){
				integrator_output_write(integ);
				any_crossed = 1;
			}
			bnd = enginedata->bndlist[i];
			bnd_set_ida_crossed(bnd, 1);

			/* Flag boundary for change, update bnd_cond_state */
			if (bnd_cond_states[i] == 0) {
				bnd_set_ida_value(bnd, 1);
				bnd_cond_states[i] = 1;
			} else {
				bnd_set_ida_value(bnd, 0);
				bnd_cond_states[i] = 0;
			}
		}
	}

	/* solve the logical relations in the model, if possible */
	if(ida_discrete_snapshot_capture(integ->system, &dshot) != 0){
		return -1;
	}
	if(num_bnds > 0 && !ida_event_has_logical_solver(integ->system)){
		if(ida_setup_lrslv(integ) != 0){
			ida_discrete_snapshot_clear(&dshot);
			return -1;
		}
	}else{
		slv_presolve(integ->system);
		slv_solve(integ->system);
		ida_sync_discretes_to_instances(integ->system);

		/* Check for convergence */
		slv_get_status(integ->system, &status);
		if (!status.converged) {
			ida_discrete_snapshot_clear(&dshot);
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Non-convergence in logical solver.");
			return -1;
		}
	}

	/* Reset the boundary flag */
	for (i = 0; i < num_bnds; i++) {
		if (rootsfound[i]) {
			struct bnd_boundary *bnd = enginedata->bndlist[i];
			bnd_set_ida_crossed(bnd, 0);
		}
	}

	/* update the main system if required */
	if (ida_discrete_snapshot_changed(&dshot)) {
		ida_discrete_snapshot_clear(&dshot);
		ida_bnd_reanalyse(integ);

		return 1;
	} else {
		ida_discrete_snapshot_clear(&dshot);
		/* Boundary crossing that has no effect on system */
		return 0;
	}

}
