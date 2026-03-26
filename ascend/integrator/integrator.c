/*	ASCEND modelling environment
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
*//**
	@file
	Integrator API for ASCEND, for solving systems of ODEs and/or DAEs.
*//*
	by John Pye, May 2006
	based on parts of Integrators.c in the Tcl/Tk interface directory, heavily
	modified to provide a non-GUI-specific API and modularised for multiple
	integration engines.
*/
#include <time.h>
#include <string.h>

#include <ascend/general/panic.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/compiler/link.h>
#include <ascend/compiler/derivinst.h>
#include <ascend/compiler/evaluate.h>
#include <ascend/compiler/find.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/packages.h>
#include <ascend/compiler/relerr.h>
#include <ascend/compiler/value_type.h>
#include <ascend/compiler/visitinst.h>
#include <ascend/compiler/when_util.h>


#include <ascend/system/slv_common.h>
#include <ascend/system/conditional.h>
#include <ascend/system/slv_stdcalls.h>
#include <ascend/system/slv_client.h>
#include <ascend/system/block.h>
#include <ascend/system/discrete.h>
#include <ascend/system/diffvars.h>

#include <ascend/solver/solver.h>

#include "integrator.h"
#include "samplelist.h"

#define ANALYSE_DEBUG
/* #define SOLVE_DEBUG */
/* #define CLASSIFY_DEBUG */
/* #define DESTROY_DEBUG */
/* #define ATOL_DEBUG */

#ifdef INTEGRATOR_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(...)
#endif

#ifdef INTEGRATOR_DEBUG
static void integ_debug_list(const char *label, struct gl_list_t *list){
	if(list == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"%s list is NULL", label);
		return;
	}
	CONSOLE_DEBUG("%s list=%p len=%lu cap=%lu expandable=%d"
		, label
		, (void *)list
		, (unsigned long)gl_length(list)
		, (unsigned long)gl_capacity(list)
		, gl_expandable(list)
	);
}
#endif

static int integrator_report_initial_status_failure(const slv_status_t *status, int presolve);

/*------------------------------------------------------------------------------
   The following names are of solver_var children or attributes
 * we support (at least temporarily) to determine who is a state and
 * who matching derivative.
 * These should be supported directly in a future solveratominst.
 */

static symchar *g_symbols[3];

#define STATEFLAG g_symbols[0]
/*
	Integer child. 0= algebraic, 1 = state, 2 = derivative, 3 = 2nd deriv etc
	independent variable is -1.
*/
#define INTEG_OTHER_VAR -1L
#define INTEG_ALGEBRAIC_VAR 0L
#define INTEG_STATE_VAR 1L

#define STATEINDEX g_symbols[1]
/* Integer child. all variables with the same STATEINDEX value are taken to
 * be derivatives of the same state variable. We really need a compiler
 * that maintains this info by backpointers, but oh well until then.
 */
#define OBSINDEX g_symbols[2]
/* Integer child. All variables with OBSINDEX !=0 will be sent to the
	IntegratorOutputWriteObsFn allowing output to a file, graph, console, etc.
 */
/** Temporary catcher of dynamic variable and observation variable data */
struct Integ_var_t {
  long index;
  long type;
  struct Integ_var_t *derivative;
  struct Integ_var_t *derivative_of;
  struct var_variable *i;
  int varindx; /**< index into slv_get_master_vars_list, or -1 if not there */
  int isstate;
};

/*------------------------------------------------------------------------------
  forward declarations
*/

/* abstractions of setup/teardown procedures for the specific solvers */
void integrator_create_engine(IntegratorSystem *sys);
void integrator_free_engine(IntegratorSystem *sys);

IntegratorAnalyseFn integrator_analyse_ode;

typedef void (IntegratorVarVisitorFn)(IntegratorSystem *sys, struct var_variable *var, const int *varindx);
void integrator_visit_system_vars(IntegratorSystem *sys,IntegratorVarVisitorFn *visitor);
IntegratorVarVisitorFn integrator_ode_classify_var;
IntegratorVarVisitorFn integrator_dae_classify_var;
IntegratorVarVisitorFn integrator_classify_indep_var;
IntegratorVarVisitorFn integrator_dae_show_var;

static int integrator_sort_obs_vars(IntegratorSystem *sys);
static void integrator_print_var_stats(IntegratorSystem *sys);
static int integrator_check_indep_var(IntegratorSystem *sys);
static void integrator_clear_analysis(IntegratorSystem *sys);
static void integrator_fix_ode_states(IntegratorSystem *sys);

static int Integ_CmpDynVars(struct Integ_var_t *v1, struct Integ_var_t *v2);
static int Integ_CmpObs(struct Integ_var_t *v1, struct Integ_var_t *v2);
static void Integ_SetObsId(struct var_variable *v, long oindex);

static long DynamicVarInfo(struct var_variable *v,long *vindex, IntegratorSystem *sys);
static struct var_variable *ObservationVar(struct var_variable *v, long *oindex);
static void IntegInitSymbols(void);

/*------------------------------------------------------------------------------
  INSTANTIATION AND DESTRUCTION
*/

/**
	Create a new IntegratorSystem and assign a slv_system_t to it.
*/
IntegratorSystem *integrator_new(slv_system_t slvsys, struct Instance *inst){
	IntegratorSystem *sys;

	if (slvsys == NULL) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"slvsys is NULL!");
		return NULL;
	}

	sys = ASC_NEW_CLEAR(IntegratorSystem);
	sys->system = slvsys;
	sys->instance = inst;

	sys->engine = INTEG_UNKNOWN;
	sys->internals = NULL;

	sys->states = NULL; sys->derivs = NULL;
	sys->dynvars = NULL; sys->obslist = NULL; sys->indepvars = NULL;

	sys->y_id = NULL;
	sys->obs_id = NULL;
	sys->y = NULL;
	sys->ydot = NULL;
	sys->obs = NULL;
	sys->n_y = 0;
	sys->initial_mode_prepared = 0;
	return sys;
}

/**
	Carefully trash any data in the IntegratorSystem that we own,
	then destroy the IntegratorSystem struct.

	Note that the integrator doesn't own the samplelist.

	@param sys will be destroyed and set to NULL.
*/
void integrator_free(IntegratorSystem *sys){
	if(sys==NULL)return;

	integrator_free_engine(sys);

	/* CONSOLE_DEBUG("Engine freed, destroying internal data"); */

	if(sys->states != NULL)gl_destroy(sys->states);
	if(sys->derivs != NULL)gl_destroy(sys->derivs);

	if(sys->dynvars != NULL)gl_free_and_destroy(sys->dynvars);    /* we own the objects in dynvars */
	if(sys->obslist != NULL)gl_free_and_destroy(sys->obslist);    /* and obslist */
	if (sys->indepvars != NULL)gl_free_and_destroy(sys->indepvars);  /* and indepvars */

	if(sys->y_id != NULL)ASC_FREE(sys->y_id);
	if(sys->obs_id != NULL)ASC_FREE(sys->obs_id);
	if(sys->y != NULL)ASC_FREE(sys->y);
	if(sys->ydot != NULL)ASC_FREE(sys->ydot);
	if(sys->obs != NULL)ASC_FREE(sys->obs);

	slv_destroy_parms(&(sys->params));

	ASC_FREE(sys);
#ifdef DESTROY_DEBUG
	CONSOLE_DEBUG("Destroyed IntegratorSystem");
#endif
	sys=NULL;
}

/**
	Utility function to retreive pointers to the symbols we'll be looking for
	in the instance hierarchy.
*/
static void IntegInitSymbols(void){
	STATEFLAG = AddSymbol("ode_type");
	STATEINDEX = AddSymbol("ode_id");
	OBSINDEX = AddSymbol("obs_id");
}

typedef struct IntegratorPreValueEntry{
	const struct Instance *inst;
	double value;
	const dim_type *dims;
} IntegratorPreValueEntry;

typedef struct IntegratorPreSnapshot{
	IntegratorSystem *sys;
	IntegratorPreValueEntry *entries;
	unsigned long nentries;
} IntegratorPreSnapshot;

static void integrator_pre_snapshot_destroy(IntegratorPreSnapshot *snapshot){
	if(snapshot->entries != NULL){
		ASC_FREE(snapshot->entries);
		snapshot->entries = NULL;
	}
	snapshot->nentries = 0;
}

static int integrator_pre_snapshot_contains(const IntegratorPreSnapshot *snapshot, const struct Instance *inst){
	unsigned long i;
	for(i = 0; i < snapshot->nentries; ++i){
		if(snapshot->entries[i].inst == inst){
			return 1;
		}
	}
	return 0;
}

static int integrator_pre_snapshot_build(IntegratorPreSnapshot *snapshot, IntegratorSystem *sys){
	struct var_variable **vars;
	unsigned long i, nvars, capacity;
	const struct Instance *xinst = NULL;

	snapshot->sys = sys;
	snapshot->entries = NULL;
	snapshot->nentries = 0;

	nvars = (unsigned long)slv_get_num_master_vars(sys->system);
	capacity = nvars + 1;
	snapshot->entries = ASC_NEW_ARRAY(IntegratorPreValueEntry, capacity);
	vars = slv_get_master_var_list(sys->system);
	for(i = 0; i < nvars; ++i){
		const struct Instance *inst = (const struct Instance *)var_instance(vars[i]);
		snapshot->entries[snapshot->nentries].inst = inst;
		snapshot->entries[snapshot->nentries].value = var_value(vars[i]);
		snapshot->entries[snapshot->nentries].dims = RealAtomDims((struct Instance *)inst);
		++snapshot->nentries;
	}
	if(sys->x != NULL){
		xinst = (const struct Instance *)var_instance(sys->x);
		if(xinst != NULL && !integrator_pre_snapshot_contains(snapshot, xinst)){
			snapshot->entries[snapshot->nentries].inst = xinst;
			snapshot->entries[snapshot->nentries].value = var_value(sys->x);
			snapshot->entries[snapshot->nentries].dims = RealAtomDims((struct Instance *)xinst);
			++snapshot->nentries;
		}
	}
	return 0;
}

static const IntegratorPreValueEntry *integrator_pre_snapshot_lookup(const IntegratorPreSnapshot *snapshot, const struct Instance *inst){
	unsigned long i;
	for(i = 0; i < snapshot->nentries; ++i){
		if(snapshot->entries[i].inst == inst){
			return snapshot->entries + i;
		}
	}
	return NULL;
}

static struct value_t integrator_evaluate_pre_name(const struct Name *nptr, void *userdata){
	IntegratorPreSnapshot *snapshot = (IntegratorPreSnapshot *)userdata;
	struct gl_list_t *instances;
	struct Instance *inst;
	struct Instance *context;
	REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
	const IntegratorPreValueEntry *entry;

	if(GetEvaluationContext() == NULL){
		return CreateErrorValue(incorrect_name);
	}

	context = GetEvaluationContext();
	SetEvaluationContext(NULL);
	instances = FindInstances(context, nptr, &err);
	SetEvaluationContext(context);
	if(instances == NULL || gl_length(instances) != 1){
		if(instances != NULL)gl_destroy(instances);
		return CreateErrorValue(incorrect_name);
	}

	inst = (struct Instance *)gl_fetch(instances, 1);
	gl_destroy(instances);

	switch(InstanceKind(inst)){
	case REAL_ATOM_INST:
	case REAL_CONSTANT_INST:
		entry = integrator_pre_snapshot_lookup(snapshot, inst);
		if(entry != NULL){
			return CreateRealValue(entry->value, entry->dims, 0);
		}
		return CreateRealValue(RealAtomValue(inst), RealAtomDims(inst), 0);
	default:
		ERROR_REPORTER_HERE(ASC_USER_ERROR,
			"pre(...) currently requires a real-valued variable reference");
		return CreateErrorValue(type_conflict);
	}
}

static struct var_variable *integrator_find_real_target(const IntegratorSystem *sys, const struct Instance *inst){
	struct var_variable **vars;
	struct var_variable **unas;
	int32 nvars, i;
	int32 nunas;

	if(sys == NULL || sys->system == NULL || inst == NULL){
		return NULL;
	}

	vars = slv_get_solvers_var_list(sys->system);
	nvars = slv_get_num_solvers_vars(sys->system);
	for(i = 0; i < nvars; ++i){
		struct var_variable *var = vars[i];
		if(var != NULL && (const struct Instance *)var_instance(var) == inst){
			return var;
		}
	}

	unas = slv_get_solvers_unattached_list(sys->system);
	nunas = slv_get_num_solvers_unattached(sys->system);
	for(i = 0; i < nunas; ++i){
		struct var_variable *var = unas[i];
		if(var != NULL && (const struct Instance *)var_instance(var) == inst){
			return var;
		}
	}
	return NULL;
}

static int integrator_reinit_target_is_diff_state(const IntegratorSystem *sys, const struct Instance *inst){
	int i;

	if(sys == NULL || inst == NULL){
		return 0;
	}
	for(i = 0; i < sys->n_y; ++i){
		if(sys->y[i] != NULL
			&& (const struct Instance *)var_instance(sys->y[i]) == inst
			&& sys->ydot[i] != NULL){
			return 1;
		}
	}
	return 0;
}

static struct dis_discrete *integrator_find_discrete_target(const IntegratorSystem *sys, const struct Instance *inst){
	struct dis_discrete **dvars;
	int32 ndvars, i;

	if(sys == NULL || sys->system == NULL || inst == NULL){
		return NULL;
	}

	dvars = slv_get_solvers_dvar_list(sys->system);
	ndvars = slv_get_num_solvers_dvars(sys->system);
	for(i = 0; i < ndvars; ++i){
		struct dis_discrete *dvar = dvars[i];
		if(dvar != NULL && (const struct Instance *)dis_instance(dvar) == inst){
			return dvar;
		}
	}

	return NULL;
}

static int integrator_reinit_already_applied(const struct gl_list_t *applied_reinits,
		const struct when_reinit *wr){
	unsigned long i, len;

	if(applied_reinits == NULL || wr == NULL){
		return 0;
	}

	len = gl_length((struct gl_list_t *)applied_reinits);
	for(i = 1; i <= len; ++i){
		if((const struct when_reinit *)gl_fetch((struct gl_list_t *)applied_reinits, i) == wr){
			return 1;
		}
	}

	return 0;
}

static int integrator_apply_case_reinits(IntegratorSystem *sys, struct Instance *context,
		struct gl_list_t *reinit_list, IntegratorPreSnapshot *snapshot,
		struct gl_list_t *applied_reinits, int *applied_count){
	unsigned long i, len;

	if(reinit_list == NULL){
		return 0;
	}

	len = gl_length(reinit_list);
	for(i = 1; i <= len; ++i){
		struct when_reinit *wr = (struct when_reinit *)gl_fetch(reinit_list, i);
		struct Instance *target;
		struct dis_discrete *dtarget;
		struct var_variable *rtarget;
		const struct Expr *guard;
		struct value_t value;
		int target_is_diff_state;
		int target_is_discrete_real;
		int target_is_discrete_nonreal;

		if(wr == NULL){
			continue;
		}
		if(integrator_reinit_already_applied(applied_reinits, wr)){
			continue;
		}
		guard = when_reinit_guard(wr);
		if(guard != NULL){
			struct value_t guard_value;
			int guard_true;
			asc_assert(GetEvaluationContext() == NULL);
			SetEvaluationContext(context);
			SetEvaluationPreNameFn(integrator_evaluate_pre_name, snapshot);
			guard_value = EvaluateExpr((struct Expr *)guard, NULL, InstanceEvaluateName);
			SetEvaluationPreNameFn(NULL, NULL);
			SetEvaluationContext(NULL);
			switch(guard_value.t){
			case boolean_value:
				guard_true = BooleanValue(guard_value) ? 1 : 0;
				break;
			case integer_value:
				guard_true = IntegerValue(guard_value) ? 1 : 0;
				break;
			default:
				DestroyValue(&guard_value);
				ERROR_REPORTER_HERE(ASC_USER_ERROR,
					"SWITCH TO guard requires a boolean-valued expression");
				return 1;
			}
			DestroyValue(&guard_value);
			if(!guard_true){
				continue;
			}
		}

		target = (struct Instance *)when_reinit_target(wr);
		if(target == NULL){
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"Unable to resolve REINIT target");
			return 1;
		}
		dtarget = integrator_find_discrete_target(sys, target);
		rtarget = integrator_find_real_target(sys, target);
		target_is_diff_state = integrator_reinit_target_is_diff_state(sys, target);
		target_is_discrete_real = (rtarget != NULL && var_discrete(rtarget));
		target_is_discrete_nonreal = (dtarget != NULL
			&& (dis_kind(dtarget) == e_dis_boolean_t
				|| dis_kind(dtarget) == e_dis_integer_t
				|| dis_kind(dtarget) == e_dis_symbol_t));
		if(!(target_is_diff_state || target_is_discrete_real)
			&& !target_is_discrete_nonreal){
			ERROR_REPORTER_HERE(ASC_USER_ERROR,
				"REINIT target must be a differential state, inferred discrete real event-memory variable, or discrete boolean/integer/symbol variable");
			return 1;
		}

		asc_assert(GetEvaluationContext() == NULL);
		SetEvaluationContext(context);
		SetEvaluationPreNameFn(integrator_evaluate_pre_name, snapshot);
		value = EvaluateExpr((struct Expr *)when_reinit_rhs(wr), NULL, InstanceEvaluateName);
		SetEvaluationPreNameFn(NULL, NULL);
		SetEvaluationContext(NULL);

		if(target_is_diff_state || target_is_discrete_real){
			switch(value.t){
			case real_value:
				SetRealAtomValue(target, RealValue(value), 0);
				break;
			case integer_value:
				SetRealAtomValue(target, (double)IntegerValue(value), 0);
				break;
			default:
				DestroyValue(&value);
				ERROR_REPORTER_HERE(ASC_USER_ERROR,
					"Unable to evaluate REINIT expression");
				return 1;
			}
		}else{
			switch(dis_kind(dtarget)){
			case e_dis_boolean_t:
				switch(value.t){
				case boolean_value:
					dis_set_boolean_value(dtarget, BooleanValue(value));
					break;
				case integer_value:
					dis_set_boolean_value(dtarget, IntegerValue(value) ? 1 : 0);
					break;
				default:
					DestroyValue(&value);
					ERROR_REPORTER_HERE(ASC_USER_ERROR,
						"Boolean REINIT target requires a boolean-valued expression");
					return 1;
				}
				break;
			case e_dis_integer_t:
				switch(value.t){
				case integer_value:
					dis_set_inst_and_field_value(dtarget, IntegerValue(value));
					break;
				case boolean_value:
					dis_set_inst_and_field_value(dtarget, BooleanValue(value) ? 1 : 0);
					break;
				default:
					DestroyValue(&value);
					ERROR_REPORTER_HERE(ASC_USER_ERROR,
						"Integer REINIT target requires an integer-valued expression");
					return 1;
				}
				break;
			case e_dis_symbol_t:
				switch(value.t){
				case symbol_value:
					SetSymbolAtomValue(target, SymbolValue(value));
					dis_set_value_from_inst(dtarget, slv_get_symbol_list(sys->system));
					break;
				default:
					DestroyValue(&value);
					ERROR_REPORTER_HERE(ASC_USER_ERROR,
						"Symbol REINIT target requires a symbol-valued expression");
					return 1;
				}
				break;
			default:
				DestroyValue(&value);
				ERROR_REPORTER_HERE(ASC_PROG_ERR,
					"Unsupported discrete REINIT target kind");
				return 1;
			}
		}
		DestroyValue(&value);
		if(applied_reinits != NULL){
			gl_append_ptr(applied_reinits, wr);
		}
		if(applied_count != NULL){
			++(*applied_count);
		}
	}

	return 0;
}

/*------------------------------------------------------------------------------
  INTEGRATOR ENGINE
*/

/**
	Local function that holds the list of available integrators. The value
	returned is NOT owned by the called.

	@param free_space if 0, call as normal. if 1, free the list and maybe do some
	cleaning up etc. Should be called whenever a simulation is destroyed.
*/
static struct gl_list_t *integrator_get_list(int free_space){
	static int init = 0;
	static struct gl_list_t *L;
	int i, error;
	/* standard integrators that we will register */
	static char *defaultintegrators[] = {
		"lsode"
		,"ida"
		,NULL
	};

	if(free_space){
		if(init && L){
			gl_destroy(L);
			L = NULL;
		}
		init = 0;
		return NULL;
	}
	if(!init){
		L = gl_create(10);
		init = 1; /* set init to 1 here now, since this will be called
			recursively from the LoadArchiveLibrary call. */

		/* CONSOLE_DEBUG("REGISTERING STANDARD SOLVER ENGINES"); */
		for(i=0; defaultintegrators[i]!=NULL;++i){
			error = package_load(defaultintegrators[i],NULL);
			if(error){
				MSG("Integrator '%s' is not available (error %d)."
					,defaultintegrators[i],error);
			}else{
				MSG("Integrator '%s' registered OK",defaultintegrators[i]);
			}
		}
	}
	return L;
}

/**
	Return gl_list of IntegratorInternals. C++ will use this to produce a
	nice little list of integrator names that can be used in Python :-/
*/
const struct gl_list_t *integrator_get_engines(){
	return integrator_get_list(0);
}

struct gl_list_t *integrator_get_engines_growable(){
	return integrator_get_list(0);
}

void integrator_free_engines(){
	integrator_get_list(1);
}

/* return 0 on success */
int integrator_set_engine(IntegratorSystem *sys, const char *name){
	struct gl_list_t *L = integrator_get_engines_growable();
	int i;
	const IntegratorInternals *I, *Ifound=NULL;
	for(i=1; i <= gl_length(L); ++i){
		I = gl_fetch(L,i);
		if(strcmp(I->name,name)==0){
			Ifound = I;
			break;
		}
	}
	if(Ifound){
		/** @TODO tests for applicability of this engine... */

		MSG("Setting engine...");
		if(Ifound->engine == sys->engine){
			// already set...
			return 0;
		}

		// clean up if old engine is there
		if(sys->engine!=INTEG_UNKNOWN){
#ifdef DESTROY_DEBUG
			CONSOLE_DEBUG("Freeing memory used by old integrator engine");
#endif
			integrator_free_engine(sys);
#ifdef DESTROY_DEBUG
			CONSOLE_DEBUG("done");
#endif
		}

		sys->engine = Ifound->engine;

		sys->internals = Ifound;
	}else{
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid engine name '%s'",name);
		return 1;
	}

	asc_assert(sys->internals);
	integrator_create_engine(sys);
	return 0;
}

/**
	@TODO rename this
*/
const IntegratorInternals *integrator_get_engine(const IntegratorSystem *sys){
	return sys->internals;
}

/**
	Free any engine-specific  data that was required for the solution of
	this system. Note that this data is pointed to by sys->enginedata.

	@TODO rename this, bad choice/confusing name.
*/
void integrator_free_engine(IntegratorSystem *sys){
	if(sys->engine==INTEG_UNKNOWN)return;
	if(sys->enginedata){
		if(sys->internals){
#ifdef DESTROY_DEBUG
			CONSOLE_DEBUG("Running engine's freefn");
#endif
			(sys->internals->freefn)(sys->enginedata);
#ifdef DESTROY_DEBUG
			CONSOLE_DEBUG("Done with freefn");
#endif
			sys->enginedata=NULL;
		}else{
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to free engine data: no sys->internals");
		}
	}
#ifdef DESTROY_DEBUG
	CONSOLE_DEBUG("Done with integrator_free_engine");
#endif
}

/**
	Create enginedata memory if required for this solver. This doesn't include
	allocating computation space, since we assume that this stage all we know
	is that we want to use a specified integrator engine, not the full details
	of the problem at hand. Allocating space inside enginedata should be done
	during the solve stage (and freed inside integrator_free_engine)

	@TODO rename this
*/
void integrator_create_engine(IntegratorSystem *sys){
	asc_assert(sys);
	asc_assert(sys->engine!=INTEG_UNKNOWN);
	asc_assert(sys->internals);
	asc_assert(sys->internals->createfn);
	asc_assert(sys->enginedata==NULL);
	(sys->internals->createfn)(sys);
}

int integrator_register(const IntegratorInternals *integ){
	/* get the current list of registered engines */
	struct gl_list_t *L;
	L = integrator_get_engines_growable();
#ifdef INTEGRATOR_DEBUG
	integ_debug_list("integrator_register", L);
#endif
	if(L == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Integrator list is NULL; cannot register '%s'", integ->name);
		return 1;
	}
	if(!gl_expandable(L)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR
			,"Integrator list is not expandable; cannot register '%s' (len=%lu cap=%lu)"
			, integ->name
			, (unsigned long)gl_length(L)
			, (unsigned long)gl_capacity(L)
		);
		return 1;
	}

	MSG("REGISTERING INTEGRATOR");
	MSG("There were %lu registered integrators", gl_length(integrator_get_list(0)));

	int i;
	IntegratorInternals *I;
	for(i=1; i <= gl_length(L); ++i){
		I = (IntegratorInternals *)gl_fetch(L,i);
		if(strcmp(integ->name,I->name)==0){
			ERROR_REPORTER_HERE(ASC_USER_WARNING,"Integrator with name '%s' is already registered",integ->name);
			return 0;
		}
		if(integ->engine == I->engine){
			ERROR_REPORTER_HERE(ASC_USER_WARNING,"Integrator with ID '%d' is already registered",integ->engine);
			return 0;
		}
	}

	MSG("Adding engine '%s'",integ->name);

	gl_append_ptr(L,(void *)integ);

	MSG("There are now %lu registered integrators", gl_length(integrator_get_list(0)));
	return 0;
}

/*------------------------------------------------------------------------------
  PARAMETERS
*/

/**
	Reset the parameters in this IntegratorSystem to the default ones for this
	Integrator.

	@return 0 on success, 1 on error
*/
int integrator_params_default(IntegratorSystem *sys){
	asc_assert(sys->engine!=INTEG_UNKNOWN);
	asc_assert(sys->internals);
	return (sys->internals->paramsdefaultfn)(sys);
}

int integrator_params_get(const IntegratorSystem *sys, slv_parameters_t *parameters){
	asc_assert(sys!=NULL);
	asc_assert(sys->params.num_parms > 0);
	memcpy(parameters,&(sys->params),sizeof(slv_parameters_t));
	return 0;
}

int integrator_params_set(IntegratorSystem *sys, const slv_parameters_t *parameters){
	asc_assert(sys!=NULL);
	asc_assert(parameters!=NULL);
	memcpy(&(sys->params),parameters,sizeof(slv_parameters_t));
	return 0;
}

/*------------------------------------------------------------------------------
  ANALYSIS

  Provide two modes in order to provide analysis suitable for solution of both
  ODEs (the previous technique) and DAEs (new code). These share a common
  "visit" method that needs to eventually be integrated with the code in
  <solver/analyze.c>. For the moment, we're just hacking in to the compiler.
*/

/**
	Locate the independent variable.

	@NOTE For the purpose of GUI design, this needs	to work independent of the
	integration engine being used. @ENDNOTE
*/
int integrator_find_indep_var(IntegratorSystem *sys){
	int result = 0;
#ifdef ANALYSE_DEBUG
	char *varname;
#endif

	/* if the indep var has been found, we don't look again (we assume the user won't fiddle with ode_type!) */
	if(sys->x != NULL){
		MSG("sys->x already set");
		return 0; /* success */
	}

	/* create a clear indepvars list */
	if(sys->indepvars!=NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"indepvars should be NULL at this point");
		return 1; /* error */
	}
	sys->indepvars = gl_create(10L);

	IntegInitSymbols();

	MSG("Looking for independent var...");
	integrator_visit_system_vars(sys,&integrator_classify_indep_var);
#ifdef ANALYSE_DEBUG
	if(gl_length(sys->indepvars)){
		MSG("Found %lu indepvars",gl_length(sys->indepvars));
	}else{
		MSG("NO INDEP VARS FOUND");
	}
#endif

	/* after visiting the instance tree, look at the candidates and return 0 on success */
	result = integrator_check_indep_var(sys);

	/* whatever happens, we clean up afterwards */
	gl_free_and_destroy(sys->indepvars);
	sys->indepvars = NULL;

#ifdef ANALYSE_DEBUG
	asc_assert(sys->system);
	if(!result){
		asc_assert(sys->x);
		varname = var_make_name(sys->system, sys->x);
		MSG("Indep var is '%s'",varname);
		ASC_FREE(varname);
	}else{
		MSG("No indep var was found");
	}
#endif

	/* ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Returning result %d",result); */

	return result;
}

/**
	Perform whatever additional problem is required so that the system can be
	integrated as a dynamical system with the IntegrationEngine chosen.

	We can always assume that sys->system has had analyse_make_problem called on
	it, so all the variable lists (etc) will be there already.

	@return 0 on success
*/
int integrator_analyse(IntegratorSystem *sys){
	int res;

#ifdef ANALYSE_DEBUG
	MSG("Analysing integration system...");
#endif
	asc_assert(sys);
	if(sys->engine==INTEG_UNKNOWN){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"No engine selected: can't analyse");
		return 1;
	}
	asc_assert(sys->engine!=INTEG_UNKNOWN);
	asc_assert(sys->internals);

	if(!sys->indepvars || !gl_length(sys->indepvars)){
		if(integrator_find_indep_var(sys)){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Independent variable problem: abandoning integration");
			return 2;
		}
#ifdef ANALYSE_DEBUG
		else{
			MSG("got 0 from  integrator_find_indep_var");
		}
#endif
	}

	res = (sys->internals->analysefn)(sys);
#ifdef ANALYSE_DEBUG
	MSG("integrator_analyse returning %d",res);
#endif
	return res;
}

struct IntegratorInitialWalk{
	int found;
};

static void integrator_find_initial_relations(struct Instance *inst, VOIDPTR userdata){
	struct IntegratorInitialWalk *walk = (struct IntegratorInitialWalk *)userdata;
	if(walk == NULL || walk->found || inst == NULL){
		return;
	}
	switch(InstanceKind(inst)){
	case REL_INST:
		walk->found = relinst_initial(inst) ? 1 : 0;
		break;
	case LREL_INST:
		walk->found = logrelinst_initial(inst) ? 1 : 0;
		break;
	default:
		break;
	}
}

static struct Instance *integrator_root_instance(const IntegratorSystem *sys){
	struct Instance *inst;
	if(sys == NULL){
		return NULL;
	}
	inst = sys->instance;
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

static int integrator_root_has_initial_relations(struct Instance *root){
	struct IntegratorInitialWalk walk;
	walk.found = 0;
	if(root == NULL){
		return 0;
	}
	VisitInstanceTreeTwo(root, (VisitTwoProc)integrator_find_initial_relations, 0, 0, &walk);
	return walk.found;
}

int integrator_has_initial_relations(IntegratorSystem *sys){
	struct Instance *root;
	root = integrator_root_instance(sys);
	return integrator_root_has_initial_relations(root);
}

int integrator_initialise_with_solver(IntegratorSystem *sys, int solver_index){
	struct Instance *root;
	slv_system_t init_sys = NULL;
	slv_system_t normal_sys = NULL;
	unsigned long res;
	struct var_variable **vlist = NULL;
	unsigned long nvars = 0, i = 0;
	struct Instance **defaults = NULL;
	unsigned long ndefaults = 0;

	if(sys == NULL){
		return 1;
	}
	root = integrator_root_instance(sys);
	if(root == NULL || !integrator_root_has_initial_relations(root)){
		return 0;
	}
	if(solver_index < 0){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"No algebraic solver selected for initialization");
		return 2;
	}
	if(sys->system != NULL){
		system_destroy(sys->system);
		sys->system = NULL;
	}
	integrator_clear_analysis(sys);

	init_sys = system_build_with_mode(root, SYSTEM_BUILD_INITIAL);
	if(init_sys == NULL){
		system_set_build_mode(root, SYSTEM_BUILD_NORMAL);
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to build initialization-mode system");
		return 3;
	}
	if(slv_select_solver(init_sys, solver_index) == -1){
		system_destroy(init_sys);
		system_set_build_mode(root, SYSTEM_BUILD_NORMAL);
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to select algebraic solver for initialization solve");
		return 4;
	}

	vlist = slv_get_solvers_var_list(init_sys);
	nvars = (unsigned long)slv_get_num_solvers_vars(init_sys);
	if(nvars > 0){
		defaults = ASC_NEW_ARRAY_CLEAR(struct Instance *, nvars);
	}
	for(i = 0; i < nvars; ++i){
		struct var_variable *var = vlist[i];
		struct Instance *inst;
		if(var == NULL){
			continue;
		}
		inst = (struct Instance *)var_instance(var);
		if(inst != NULL && IsDerivativeInstance(inst) && DerivativeInstanceUsesAlgebraicDefault(inst)){
			DerivativeInstanceSetAlgebraicDefault(inst, FALSE);
			defaults[ndefaults++] = inst;
		}
	}
	slv_block_set_dof_messages_enabled(FALSE);
	if(slv_presolve(init_sys)){
		slv_status_t status;
		slv_get_status(init_sys, &status);
		slv_block_set_dof_messages_enabled(TRUE);
		for(i = 0; i < ndefaults; ++i){
			if(defaults[i] != NULL){
				DerivativeInstanceSetAlgebraicDefault(defaults[i], TRUE);
			}
		}
		if(defaults != NULL){
			ASC_FREE(defaults);
		}
		system_destroy(init_sys);
		system_set_build_mode(root, SYSTEM_BUILD_NORMAL);
		integrator_report_initial_status_failure(&status, 1);
		return 5;
	}
	{
		slv_status_t status;
		slv_get_status(init_sys, &status);
		slv_block_set_dof_messages_enabled(TRUE);
		if(status.over_defined || status.under_defined || status.struct_singular || status.inconsistent){
			for(i = 0; i < ndefaults; ++i){
				if(defaults[i] != NULL){
					DerivativeInstanceSetAlgebraicDefault(defaults[i], TRUE);
				}
			}
			if(defaults != NULL){
				ASC_FREE(defaults);
			}
			system_destroy(init_sys);
			system_set_build_mode(root, SYSTEM_BUILD_NORMAL);
			integrator_report_initial_status_failure(&status, 1);
			return 5;
		}
	}
	res = slv_solve(init_sys);
	{
		slv_status_t status;
		slv_get_status(init_sys, &status);
	for(i = 0; i < ndefaults; ++i){
		if(defaults[i] != NULL){
			DerivativeInstanceSetAlgebraicDefault(defaults[i], TRUE);
		}
	}
	if(defaults != NULL){
		ASC_FREE(defaults);
	}
	system_destroy(init_sys);
	if(res || !status.ok || !status.converged){
		system_set_build_mode(root, SYSTEM_BUILD_NORMAL);
		integrator_report_initial_status_failure(&status, 0);
		return 6;
	}
	}
	normal_sys = system_build_with_mode(root, SYSTEM_BUILD_NORMAL);
	if(normal_sys == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to rebuild normal-mode system after initialization solve");
		return 7;
	}
	if(solver_index >= 0 && slv_select_solver(normal_sys, solver_index) == -1){
		system_destroy(normal_sys);
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to restore solver after initialization solve");
		return 8;
	}
	if(sys->system != NULL){
		system_destroy(sys->system);
	}
	sys->system = normal_sys;
	integrator_clear_analysis(sys);
	if(integrator_analyse(sys)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to reanalyse normal-mode system after initialization solve");
		return 9;
	}
	return 0;
}

int integrator_initialise_ode(IntegratorSystem *sys){
	if(sys == NULL || sys->system == NULL){
		return 1;
	}
	if(integrator_initialise_with_solver(sys, slv_get_selected_solver(sys->system))){
		return 1;
	}
	integrator_fix_ode_states(sys);
	return 0;
}

static int integrator_report_initial_status_failure(const slv_status_t *status, int presolve){
	const char *phase = presolve ? "presolve" : "solve";
	if(status == NULL){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,
			"Initialization %s failed. Check INITIAL equations and startup FIX/FREE settings.", phase);
		return 1;
	}
	if(status->over_defined || status->under_defined || status->struct_singular){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,
			"Initialization problem is not square. Check INITIAL equations and startup FIX/FREE settings.");
		return 1;
	}
	if(status->inconsistent){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,
			"Initialization problem is inconsistent. Check INITIAL equations and startup values.");
		return 1;
	}
	if(status->diverged || status->iteration_limit_exceeded || status->time_limit_exceeded || !status->calc_ok){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,
			"Initialization solve did not converge. Check INITIAL equations, startup values, and solver guesses.");
		return 1;
	}
	ERROR_REPORTER_HERE(ASC_USER_ERROR,
		"Initialization %s failed. Check INITIAL equations and startup FIX/FREE settings.", phase);
	return 1;
}


void integrator_visit_system_vars(IntegratorSystem *sys,IntegratorVarVisitorFn *visitfn){
  struct var_variable **vlist;
  int i, vlen;

  /* visit all the slv_system_t master var lists to collect vars */
  /* find the vars mostly in this one */
  vlist = slv_get_master_var_list(sys->system);
  vlen = slv_get_num_master_vars(sys->system);
  for (i=0;i<vlen;i++) {
    (*visitfn)(sys, vlist[i], &i);
  }

  /*
  CONSOLE_DEBUG("Checked %d vars",vlen);
  integrator_print_var_stats(sys);
  */

  /* probably nothing here, but gotta check. */
  vlist = slv_get_master_par_list(sys->system);
  vlen = slv_get_num_master_pars(sys->system);
  for (i=0;i<vlen;i++) {
    (*visitfn)(sys, vlist[i], NULL);
  }

  /*
  CONSOLE_DEBUG("Checked %d pars",vlen);
  integrator_print_var_stats(sys);
  */

  /* might find t here */
  vlist = slv_get_master_unattached_list(sys->system);
  vlen = slv_get_num_master_unattached(sys->system);
  for (i=0;i<vlen;i++) {
    (*visitfn)(sys, vlist[i], NULL);
  }

  /* CONSOLE_DEBUG("Checked %d unattached",vlen); */
}
/**
	Analyse the ODE structure. We can assume that the independent variable was
	already found.

	@return 0 on success
*/
int integrator_analyse_ode(IntegratorSystem *sys){
  struct Integ_var_t *v1,*v2;
  long half,i,len;
  int happy=1;
  int solver_index;
  char *varname1, *varname2;

  asc_assert(sys->system!=NULL);

  solver_index = slv_get_selected_solver(sys->system);
  if(solver_index < 0 || strcmp(slv_solver_name(solver_index),"QRSlv")!=0){
    int qrslv_index;
    if(package_load("qrslv", NULL) != 0){
      ERROR_REPORTER_HERE(ASC_PROG_ERR,
        "Unable to load QRSlv for ODE integration analysis");
	  return 2;
    }
    qrslv_index = slv_lookup_client("QRSlv");
    if(qrslv_index < 0 || slv_select_solver(sys->system, qrslv_index) == -1){
      ERROR_REPORTER_HERE(ASC_PROG_ERR,
        "QRSlv is unavailable for ODE integration analysis");
	  return 2;
    }
    solver_index = qrslv_index;
  }
  MSG("Checked that NLA solver is set to '%s'",slv_solver_name(solver_index));

  if(slv_get_num_solvers_bnds(sys->system) > 0 || slv_get_num_solvers_whens(sys->system) > 0){
    ERROR_REPORTER_NOLINE(ASC_USER_ERROR,
      "LSODE does not support CONDITIONAL/WHEN event handling or REINIT."
      " Use IDA for hybrid/evented models."
    );
    return 2;
  }

  MSG("Starting ODE analysis");
  IntegInitSymbols();

  /* collect potential states and derivatives */
  sys->indepvars = gl_create(10L);  /* t var info */
  sys->dynvars = gl_create(200L);  /* y ydot var info */
  sys->obslist = gl_create(100L);  /* obs info */
  if (sys->dynvars == NULL
    || sys->obslist == NULL
    || sys->indepvars == NULL
  ){
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory.");
    return 1;
  }

  sys->nstates = sys->nderivs = 0;

  integrator_visit_system_vars(sys,&integrator_ode_classify_var);

  integrator_print_var_stats(sys);

  /* check sanity of state and var lists */

  len = gl_length(sys->dynvars);
  half = len/2;
  MSG("NUMBER OF DYNAMIC VARIABLES = %ld",half);

  if (len == 0L) {
    ERROR_REPORTER_NOLINE(ASC_USER_ERROR,
      "No dynamic variables were found for ODE integration."
      " This model is not in first-order ODE form."
    );
    return 3;
  }
  if (len % 2 || sys->nstates != sys->nderivs ) {
    /* list length must be even for vars to pair off */
    ERROR_REPORTER_NOLINE(ASC_USER_ERROR,
      "Model is not in first-order ODE form (states=%ld, derivatives=%ld)."
      " This may be a high-index DAE or constrained system;"
      " LSODE requires a first-order ODE formulation or prior index reduction."
      , sys->nstates, sys->nderivs
    );
    return 3;
  }
  gl_sort(sys->dynvars,(CmpFunc)Integ_CmpDynVars);
  if (gl_fetch(sys->dynvars,len)==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Mysterious NULL found!");
    return 4;
  }
  sys->states = gl_create(half);   /* state vars Integ_var_t references */
  sys->derivs = gl_create(half);   /* derivative var atoms */
  for (i=1;i < len; i+=2) {
    v1 = (struct Integ_var_t *)gl_fetch(sys->dynvars,i);
    v2 = (struct Integ_var_t *)gl_fetch(sys->dynvars,i+1);
    if (v1->type!=1  || v2 ->type !=2 || v1->index != v2->index) {
      varname1 = var_make_name(sys->system,v1->i);
	  varname2 = var_make_name(sys->system,v2->i);

      ERROR_REPORTER_HERE(ASC_USER_ERROR,"Mistyped or misindexed dynamic variables: %s (%s = %ld,%s = %ld) and %s (%s = %ld,%s = %ld).",
             varname1, SCP(STATEFLAG),v1->type,SCP(STATEINDEX),v1->index,
             varname2, SCP(STATEFLAG),v2->type,SCP(STATEINDEX),v2->index
		);
      ASC_FREE(varname1);
      ASC_FREE(varname2);
      happy=0;
      break;
    } else {
      gl_append_ptr(sys->states,(POINTER)v1);
      gl_append_ptr(sys->derivs,(POINTER)v2->i);
    }
  }
  if (!happy) {
	ERROR_REPORTER_HERE(ASC_USER_ERROR,
		"Problem with legacy ODE indexing metadata (ode_id/ode_type values)."
	);
    return 5;
  }
  sys->n_y = half;
  sys->y = ASC_NEW_ARRAY(struct var_variable *, half);
  sys->ydot = ASC_NEW_ARRAY(struct var_variable *, half);
  sys->y_id = ASC_NEW_ARRAY(int, half);
  if (sys->y==NULL || sys->ydot==NULL || sys->y_id==NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory.");
    return 6;
  }
  for (i = 1; i <= half; i++) {
    v1 = (struct Integ_var_t *)gl_fetch(sys->states,i);
    sys->y[i-1] = v1->i;
    sys->y_id[i-1] = v1->index;
    sys->ydot[i-1] = (struct var_variable *)gl_fetch(sys->derivs,i);
  }

  if(integrator_sort_obs_vars(sys)){
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error sorting observation variables");
	return 7;
  }

  /* don't need the gl_lists now that we have arrays for everyone */
  gl_destroy(sys->states);
  gl_destroy(sys->derivs);
  gl_free_and_destroy(sys->indepvars);  /* we own the objects in indepvars */
  gl_free_and_destroy(sys->dynvars);    /* we own the objects in dynvars */
  gl_free_and_destroy(sys->obslist);    /* and obslist */
  sys->states = NULL;
  sys->derivs = NULL;
  sys->indepvars = NULL;
  sys->dynvars = NULL;
  sys->obslist = NULL;

  /* analysis completed OK */
  return 0;
}

/**
	Reindex observations. Sort if the user mostly numbered. Take natural order
	if user just booleaned.

	@return 0 on success
*/
static int integrator_sort_obs_vars(IntegratorSystem *sys){
  int half, i, len = 0;
  struct Integ_var_t *v2;

  half = sys->n_y;
  len = gl_length(sys->obslist);
  /* we shouldn't be seeing NULL here ever except if malloc fail. */
  if (len > 1L) {
    half = ((struct Integ_var_t *)gl_fetch(sys->obslist,1))->index;
    /* half != 0 now because we didn't collect 0 indexed vars */
    for (i=2; i <= len; i++) {
      if (half != ((struct Integ_var_t *)gl_fetch(sys->obslist,i))->index) {
        /* change seen. sort and go on */
        gl_sort(sys->obslist,(CmpFunc)Integ_CmpObs);
        break;
      }
    }
  }
  for (i = half = 1; i <= len; i++) {
    v2 = (struct Integ_var_t *)gl_fetch(sys->obslist,i);
    if (v2==NULL) {
      /* we shouldn't be seeing NULL here ever except if malloc fail. */
      gl_delete(sys->obslist,i,0); /* should not be gl_delete(so,i,1) */
    } else {
      Integ_SetObsId(v2->i,half);
      v2->index = half++;
    }
  }

  /* obslist now uniquely indexed, no nulls */
  /* make into arrays */
  half = gl_length(sys->obslist);
  sys->obs = ASC_NEW_ARRAY(struct var_variable *,half);
  sys->obs_id = ASC_NEW_ARRAY(int, half);
  if ( sys->obs==NULL || sys->obs_id==NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory.");
    return 1;
  }
  sys->n_obs = half;
  for (i = 1; i <= half; i++) {
    v2 = (struct Integ_var_t *)gl_fetch(sys->obslist,i);
    sys->obs[i-1] = v2->i;
    sys->obs_id[i-1] = v2->index;
  }

  return 0;
}

static void integrator_clear_analysis(IntegratorSystem *sys){
  if(sys == NULL){
    return;
  }
  if(sys->states != NULL){
    gl_destroy(sys->states);
    sys->states = NULL;
  }
  if(sys->derivs != NULL){
    gl_destroy(sys->derivs);
    sys->derivs = NULL;
  }
  if(sys->dynvars != NULL){
    gl_free_and_destroy(sys->dynvars);
    sys->dynvars = NULL;
  }
  if(sys->obslist != NULL){
    gl_free_and_destroy(sys->obslist);
    sys->obslist = NULL;
  }
  if(sys->indepvars != NULL){
    gl_free_and_destroy(sys->indepvars);
    sys->indepvars = NULL;
  }
  if(sys->y_id != NULL){
    ASC_FREE(sys->y_id);
    sys->y_id = NULL;
  }
  if(sys->obs_id != NULL){
    ASC_FREE(sys->obs_id);
    sys->obs_id = NULL;
  }
  if(sys->y != NULL){
    ASC_FREE(sys->y);
    sys->y = NULL;
  }
  if(sys->ydot != NULL){
    ASC_FREE(sys->ydot);
    sys->ydot = NULL;
  }
  if(sys->obs != NULL){
    ASC_FREE(sys->obs);
    sys->obs = NULL;
  }
  sys->x = NULL;
  sys->n_y = 0;
  sys->n_obs = 0;
  sys->nstates = 0;
  sys->nderivs = 0;
}

static void integrator_fix_ode_states(IntegratorSystem *sys){
  int i;
  if(sys == NULL || sys->y == NULL){
    return;
  }
  for(i = 0; i < sys->n_y; ++i){
    if(!var_fixed(sys->y[i])){
      ERROR_REPORTER_HERE(ASC_USER_WARNING,"Fixing state %d",i);
      var_set_fixed(sys->y[i], TRUE);
    }
  }
}

static void integrator_print_var_stats(IntegratorSystem *sys){
	MSG("Currently %lu vars, %lu indep"
		, (unsigned long)gl_length(sys->dynvars)
		, (unsigned long)gl_length(sys->indepvars)
	);
}

/**
	Check sanity of the independent variable.

	@return 0 on success
*/
static int integrator_check_indep_var(IntegratorSystem *sys){
  int len, i;
  struct Integ_var_t *info;
  char *varname;

  if(sys->x){
	MSG("Indep var already assigned");
	return 1;
  }

  /* check the sanity of the independent variable */
  len = gl_length(sys->indepvars);
  if (!len) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"No independent variable found.");
    return 2;
  }
  if (len > 1) {
	ERROR_REPORTER_START_HERE(ASC_USER_ERROR);
    FPRINTF(ASCERR,"Excess %ld independent variables found:",
      len);
    for(i=1; i <=len;i++) {
      info = (struct Integ_var_t *)gl_fetch(sys->indepvars,i);
      if(info==NULL)continue;

      varname = var_make_name(sys->system,info->i);
      FPRINTF(ASCERR," %s",varname);
      ASC_FREE(varname);
    }
    FPRINTF(ASCERR , "\nSet the '%s' flag on all but one of these to %s >= 0.\n"
        , SCP(STATEFLAG),SCP(STATEFLAG)
	);
	error_reporter_end_flush();
    return 3;
  }else{
    info = (struct Integ_var_t *)gl_fetch(sys->indepvars,1);
    sys->x = info->i;
  }
  asc_assert(gl_length(sys->indepvars)==1);
  asc_assert(sys->x);
  return 0;
}

/*------------------------------------------------------------------------------
  CLASSIFICATION OF VARIABLES (for ANALYSIS step)
*/

#define INTEG_ADD_TO_LIST(info,TYPE,INDEX,VAR,VARINDX,LIST) \
	if((LIST)==NULL){ \
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"INTEG_ADD_TO_LIST called with NULL list"); \
		return; \
	} \
	if(!gl_expandable((LIST))){ \
		ERROR_REPORTER_HERE(ASC_PROG_ERR \
			,"INTEG_ADD_TO_LIST list not expandable (len=%lu cap=%lu)" \
			, (unsigned long)gl_length((LIST)) \
			, (unsigned long)gl_capacity((LIST)) \
		); \
		return; \
	} \
	info = ASC_NEW(struct Integ_var_t); \
	if(info==NULL){ \
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory (INTEG_VAR_NEW)"); \
		return; \
	} \
	info->type=TYPE; \
	info->index=INDEX; \
	info->i=VAR; \
	info->derivative=NULL; \
	info->derivative_of=NULL; \
    if(VARINDX==NULL){ \
		info->varindx = -1; \
	}else{ \
		info->varindx = *VARINDX; \
	} \
	gl_append_ptr(LIST,(void *)info); \
	info = NULL

/**
	In a DAE, it's either the (single) independent variable, or it's a
	variable in the model.

	I'm not sure what we should be doing with variables that are already
	present as derivatives of other variables, I guess those ones need to be
	removed from the list in a second pass?
*/
void integrator_dae_classify_var(IntegratorSystem *sys
		, struct var_variable *var, const int *varindx
){
	struct Integ_var_t *info;
	long type,index;

	/* filter for recognition of solver_vars */
	var_filter_t vfilt;
	vfilt.matchbits = VAR_SVAR;
	vfilt.matchvalue = VAR_SVAR;

	asc_assert(var != NULL && var_instance(var)!=NULL );

  if( var_apply_filter(var,&vfilt) ) {
		if(var_discrete(var)){
			if(ObservationVar(var,&index) != NULL && index > 0L) {
				INTEG_ADD_TO_LIST(info,0L,index,var,varindx,sys->obslist);
			}
			return;
		}
		if(!var_active(var)){
			MSG("VARIABLE IS NOT ACTIVE");
			return;
		}

		/* only non-fixed variables are accepted */
		if(!var_fixed(var)){
			/* get the ode_type and ode_id of this solver_var */
			type = DynamicVarInfo(var,&index,sys);

			if(type==INTEG_OTHER_VAR){
				/* if the var's type is -1, it's independent */
				INTEG_ADD_TO_LIST(info,INTEG_OTHER_VAR,0,var,varindx,sys->indepvars);
			}else{
				if(type < 0)type=0;
				/* any other type of var is in the DAE system, at least for now */
				INTEG_ADD_TO_LIST(info,type,index,var,varindx,sys->dynvars);
			}
		}
#if 1
		else{
			/* fixed variable, only include it if ode_type == 1 */
			type = DynamicVarInfo(var,&index,sys);
			if(type==INTEG_STATE_VAR){
				INTEG_ADD_TO_LIST(info,type,index,var,varindx,sys->dynvars);
			}
		}
#endif

		/* if the var's obs_id > 0, add it to the observation list */
		if(ObservationVar(var,&index) != NULL && index > 0L) {
			INTEG_ADD_TO_LIST(info,type,index,var,varindx,sys->obslist);
		}
	}
}

/**
	Inspect a specific variable and work out what type it is (what 'ode_type' it
	has) and what other variable(s) it corresponds to (ie dydt corresponds to
	y as a derivative).

	@TODO add ability to create new variables for 'missing' derivative vars?
*/
void integrator_ode_classify_var(IntegratorSystem *sys, struct var_variable *var
		, const int *varindx
){
  struct Integ_var_t *info;
  long type,index;

  var_filter_t vfilt;
  vfilt.matchbits = VAR_SVAR;
  vfilt.matchvalue = VAR_SVAR;

  asc_assert(var != NULL && var_instance(var)!=NULL );

  if( var_apply_filter(var,&vfilt) ) {
	if(var_discrete(var)){
		if(ObservationVar(var,&index) != NULL && index > 0L) {
			INTEG_ADD_TO_LIST(info,0L,index,var,varindx,sys->obslist);
		}
		return;
	}
	/* it's a solver var: what type of variable? */
    type = DynamicVarInfo(var,&index,sys);

    if(type==INTEG_ALGEBRAIC_VAR){
		/* no action required */
	}else if(type==INTEG_OTHER_VAR){
		/* i.e. independent var */
        INTEG_ADD_TO_LIST(info,type,index,var,varindx,sys->indepvars);
	}else if(type>=INTEG_STATE_VAR){
        INTEG_ADD_TO_LIST(info,type,index,var,varindx,sys->dynvars);
        if(type == 1){
          sys->nstates++;
        }else if(type == 2){ /* what about higher-order derivatives? -- JP */
          sys->nderivs++;
        }else{
		  ERROR_REPORTER_HERE(ASC_USER_WARNING,"Higher-order (>=2) derivatives are not supported in ODEs.");
		}	}

    if(ObservationVar(var,&index) != NULL && index > 0L) {
		INTEG_ADD_TO_LIST(info,0L,index,var,varindx,sys->obslist);
    }
  }
}

/**
	Look at a variable and determine if it's the independent variable or not.
	This is just for the purpose of the integrator_find_indep_var function,
	which is a utility function provided for use by the GUI.
*/
void integrator_classify_indep_var(IntegratorSystem *sys
		, struct var_variable *var, const int *varindx
){
	struct Integ_var_t *info;
	long type,index;
	var_filter_t vfilt;
#ifdef CLASSIFY_DEBUG
	char *varname;
#endif

	asc_assert(var != NULL && var_instance(var)!=NULL );
	vfilt.matchbits = VAR_SVAR;
	vfilt.matchvalue = VAR_SVAR;

#ifdef CLASSIFY_DEBUG
	varname = var_make_name(sys->system,var);
#endif

	if( var_apply_filter(var,&vfilt) ) {
		if(var_discrete(var)){
			return;
		}
		type = DynamicVarInfo(var,&index,sys);

		if(type==INTEG_OTHER_VAR){
			/* i.e. independent var */
#ifdef CLASSIFY_DEBUG
			CONSOLE_DEBUG("Var '%s' added to indepvars",varname);
#endif
			INTEG_ADD_TO_LIST(info,type,index,var,varindx,sys->indepvars);
#ifdef CLASSIFY_DEBUG
		}else{
			CONSOLE_DEBUG("Var '%s' not correct ode_type",varname);
#endif
		}
#ifdef CLASSIFY_DEBUG
	}else{
		CONSOLE_DEBUG("Var '%s' failed filter",varname);
#endif
	}

#ifdef CLASSIFY_DEBUG
	ASC_FREE(varname);
#endif
}


/**
	Look at a variable, and if it is an 'ODE variable' (it has a child instance
	named 'ode_type') return its type, which will be either:
		- INTEG_OTHER_VAR (if 'ode_type' is -1)
		- INTEG_ALGEBRAIC_VAR (if 'ode_type' is zero or any negative value < -1)
		- INTEG_STATE_VAR (if 'ode_type' is 1)
		- values 2, 3 or up, indicating derivatives (1st deriv=2, 2nd deriv=3, etc)

	If the parameter 'index' is not null, the value of 'ode_id' will be stuffed
	there.
*/
static long DynamicVarInfo(struct var_variable *v,long *index, IntegratorSystem *sys){
  struct Instance *c, *d, *i;
	int type;
	long dtype;

  i = var_instance(v);

  asc_assert(i!=NULL);
  asc_assert(STATEFLAG!=NULL);
  asc_assert(STATEINDEX!=NULL);
  c = ChildByChar(i,STATEFLAG);
  d = ChildByChar(i,STATEINDEX);



  dtype = system_diffvars_var_role(sys->system, v, index);
  if(dtype != INTEG_ALGEBRAIC_VAR){
    return dtype;
  }

  /* lazy evaluation is important in the following if */
  if(c == NULL
      || d == NULL
      || InstanceKind(c) != INTEGER_INST
      || InstanceKind(d) != INTEGER_INST
      || !AtomAssigned(c)
      || (!AtomAssigned(d) && GetIntegerAtomValue(c) != INTEG_OTHER_VAR)
  ){
		type = getOdeType(sys->instance,i);
		if(type != 0) {
			if(index !=NULL){
				*index = getOdeId(sys->instance,i);
			}
 			return type;
		}

    return INTEG_ALGEBRAIC_VAR;
  }
  if (index != NULL) {
    *index = GetIntegerAtomValue(d);
  }
  return GetIntegerAtomValue(c);
}

/**
	Looks at the given variable checks if it is an 'observation variable'. This
	means that it has its 'obs_id' child instance set to a non-zero value.

	If the variable is an observation variable, its index value ('obs_id') is
	stuff into *index (provided index!=NULL), and the pointer to the original
	instance is rtruend.

	If it's not an observation variable, we return NULL and *index is untouched.
 */
static struct var_variable *ObservationVar(struct var_variable *v, long *index){
  struct Instance *c,*i;
  i = var_instance(v);
  asc_assert(i!=NULL);
  c = ChildByChar(i,OBSINDEX);
  if( c == NULL || InstanceKind(c) != INTEGER_INST || !AtomAssigned(c)) {
    return NULL;
  }
  if (index != NULL) {
    *index = GetIntegerAtomValue(c);
  }
  return v;
}

/*------------------------------------------------------------------------------
  RUNNING THE SOLVER
*/

/*
	Make the call to the actual integrator we've selected, for the range of
	time values specified. The sys contains all the specifics.

	Return 0 on success
*/
int integrator_solve(IntegratorSystem *sys, long i0, long i1){

	long nstep;
	unsigned long start_index=0, finish_index=0;
	asc_assert(sys!=NULL);

	asc_assert(sys->internals);
	asc_assert(sys->engine!=INTEG_UNKNOWN);

	nstep = integrator_getnsamples(sys)-1;
	/* check for at least 2 steps and dimensionality of x vs steps here */

	if (i0<0 || i1 <0) {
		/* removed completely inappropriate interactive code here */
		ERROR_REPORTER_HERE(ASC_PROG_ERROR,"Console input of integration limits has been disabled!");
		return -1;
	} else {
		start_index=i0;
		finish_index =i1;
		if (start_index >= (unsigned long)nstep) {
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Start point (=%lu) must be an index in the range [0,%li]."
				,start_index,nstep
			);
			return -2;
		}
		if (finish_index > (unsigned long)nstep) {
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"End point (=%lu) must be an index in the range [0,%li]."
				,finish_index,nstep
			);
			return -3;
		}
    }

	if(finish_index <= start_index) {
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"End point comes before start point! (start=%lu, end=%lu)"
			,start_index,finish_index
		);
		return -4;
	}

	MSG("RUNNING INTEGRATION...");

	if(!sys->initial_mode_prepared && sys->internals->initialisefn != NULL){
		if((sys->internals->initialisefn)(sys)){
			return -5;
		}
		sys->initial_mode_prepared = 1;
	}

	return (sys->internals->solvefn)(sys,start_index,finish_index);
}

/*---------------------------------------------------------------
  HANDLING THE LIST OF TIMESTEMPS
*/

#define GETTER_AND_SETTER(TYPE,NAME) \
	void integrator_set_##NAME(IntegratorSystem *sys, TYPE val){ \
		sys->NAME=val; \
	} \
	TYPE integrator_get_##NAME(IntegratorSystem *sys){ \
		return sys->NAME; \
	}

GETTER_AND_SETTER(SampleList *,samples) /*;*/
GETTER_AND_SETTER(double,maxstep) /*;*/
GETTER_AND_SETTER(double,minstep) /*;*/
GETTER_AND_SETTER(double,stepzero) /*;*/
GETTER_AND_SETTER(int,maxsubsteps) /*;*/
#undef GETTER_AND_SETTER

long integrator_getnsamples(IntegratorSystem *sys){
	asc_assert(sys!=NULL);
	asc_assert(sys->samples!=NULL);
	return samplelist_length(sys->samples);
}

double integrator_getsample(IntegratorSystem *sys, long i){
	asc_assert(sys!=NULL);
	asc_assert(sys->samples!=NULL);
	return samplelist_get(sys->samples,i);
}

void integrator_setsample(IntegratorSystem *sys, long i,double xi){
	asc_assert(sys!=NULL);
	asc_assert(sys->samples!=NULL);
	samplelist_set(sys->samples,i,xi);
}

const dim_type *integrator_getsampledim(IntegratorSystem *sys){
	asc_assert(sys!=NULL);
	asc_assert(sys->samples!=NULL);
	return samplelist_dim(sys->samples);
}

ASC_DLLSPEC long integrator_getcurrentstep(IntegratorSystem *sys){
	return sys->currentstep;
}

/*------------------------------------------------------------------------------
  GET/SET VALUE OF THE INDEP VARIABLE
*/

/**
	Retrieve the value of the independent variable (time) from ASCEND
	and return it as a double.
*/
double integrator_get_t(IntegratorSystem *sys){
	asc_assert(sys->x!=NULL);
	return var_value(sys->x);
}

/**
	Set the value of the independent variable (time) in ASCEND.
*/
void integrator_set_t(IntegratorSystem *sys, double value){
  var_set_value(sys->x, value);
  /* CONSOLE_DEBUG("set_t = %g", value); */
}

/*------------------------------------------------------------------------------
  PASSING DIFFERENTIAL VARIABLES AND THEIR DERIVATIVES TO/FROM THE SOLVER
*/
/**
	Retrieve the current values of the derivatives of the y-variables
	and stick them in the/an array that the integrator will use.

	If the pointer 'y' is NULL, the necessary space is allocated (and
	must be freed somewhere else).
*/
double *integrator_get_y(IntegratorSystem *sys, double *y) {
  long i;

  if (y==NULL) {
    y = ASC_NEW_ARRAY_CLEAR(double, sys->n_y+1);
    /* C y[0]  <==> ascend d.y[1]  <==>  f77 y(1) */
  }

  for (i=0; i< sys->n_y; i++) {
	asc_assert(sys->y[i]!=NULL);
    y[i] = var_value(sys->y[i]);
    /* CONSOLE_DEBUG("ASCEND --> y[%ld] = %g", i+1, y[i]); */
  }
  return y;
}

/**
	Take the values of the differential variables from the array that the
	integrator uses, and use them to update the values of the corresponding
	variables in ASCEND.
*/
void integrator_set_y(IntegratorSystem *sys, double *y) {
  long i;
#ifdef SOLVE_DEBUG
  char *varname;
#endif

  for (i=0; i < sys->n_y; i++) {
	asc_assert(sys->y[i]!=NULL);
    var_set_value(sys->y[i],y[i]);
#ifdef SOLVE_DEBUG
	varname = var_make_name(sys->system, sys->y[i]);
	CONSOLE_DEBUG("y[%ld] = %g --> '%s'", i+1, y[i], varname);
	ASC_FREE(varname);
#endif
  }
}

/**
	Send the values of the derivatives of the 'y' variables to the solver.
	Allocate space for an array if necessary.

	Any element in sys->ydot that is NULL will be passed over (the value
	won't be modified in dydx).
*/
double *integrator_get_ydot(IntegratorSystem *sys, double *dydx) {
  long i;

  if (dydx==NULL) {
    dydx = ASC_NEW_ARRAY_CLEAR(double, sys->n_y+1);
    /* C dydx[0]  <==> ascend d.dydx[1]  <==>  f77 ydot(1) */
  }

  for (i=0; i < sys->n_y; i++) {
    if(sys->ydot[i]!=NULL){
		dydx[i] = var_value(sys->ydot[i]);
	}
    /* CONSOLE_DEBUG("ASCEND --> ydot[%ld] = %g", i+1, dydx[i]); */
  }
  return dydx;
}

void integrator_set_ydot(IntegratorSystem *sys, double *dydx) {
	long i;
#ifdef SOLVE_DEBUG
	char *varname;
#endif
	for (i=0; i < sys->n_y; i++) {
		if(sys->ydot[i]!=NULL){
    		var_set_value(sys->ydot[i],dydx[i]);
#ifdef SOLVE_DEBUG
			varname = var_make_name(sys->system, sys->ydot[i]);
			CONSOLE_DEBUG("ydot[%ld] = \"%s\" = %g --> ASCEND", i+1, varname, dydx[i]);
			ASC_FREE(varname);
		}else{
			CONSOLE_DEBUG("ydot[%ld] = %g (internal)", i+1, dydx[i]);
#endif
		}
	}
}

/**
	Retrieve the values of 'ode_atol' properties of each of y-variables,
	for use in setting absolute error tolerances for the Integrator.

	If the pointer 'atol' is NULL, the necessary space is allocated (and
	must be freed somewhere else).
*/
double *integrator_get_atol(IntegratorSystem *sys, double *atol){
	long i;
#ifdef ATOL_DEBUG
	char *varname;
#endif

	if (atol==NULL) {
		atol = ASC_NEW_ARRAY_CLEAR(double, sys->n_y);
	}

	for (i=0; i< sys->n_y; i++) {
		asc_assert(sys->y[i]!=NULL);
		atol[i] = var_odeatol(sys->y[i]);
		asc_assert(atol[i]!=-1);
#ifdef ATOL_DEBUG
		varname = var_make_name(sys->system,sys->y[i]);
		CONSOLE_DEBUG("%s.ode_atol = %8.2e",varname,atol[i]);
		ASC_FREE(varname);
#endif
	}
	return atol;
}

/*-------------------------------------------------------------
  RETRIEVING OBSERVATION DATA
*/

/**
   This function takes the inst in the solver and returns the vector of
   observation variables that are located in the submodel d.obs array.
*/
double *integrator_get_observations(IntegratorSystem *sys, double *obsi) {
  long i;

  if (obsi==NULL) {
    obsi = ASC_NEW_ARRAY_CLEAR(double, sys->n_obs+1);
  }

  /* C obsi[0]  <==> ascend d.obs[1] */

  for (i=0; i < sys->n_obs; i++) {
    obsi[i] = var_value(sys->obs[i]);
    /* CONSOLE_DEBUG("*get_d_obs[%ld] = %g\n", i+1, obsi[i]); */
  }
  return obsi;
}

struct var_variable *integrator_get_observed_var(IntegratorSystem *sys, const long i){
	asc_assert(i>=0);
	asc_assert(i<sys->n_obs);
	return sys->obs[i];
}

/**
	@NOTE Although this shouldn't be required for implementation of solver
	engines, this is useful for GUI reporting of integration results.
*/
struct var_variable *integrator_get_independent_var(IntegratorSystem *sys){
	return sys->x;
}

int integrator_write_matrix(const IntegratorSystem *sys, FILE *fp,const char *type){
	asc_assert(sys);
	asc_assert(sys->enginedata);
	asc_assert(sys->internals);
	asc_assert(sys->internals->name);
	if(sys->internals->writematrixfn){
		if(type!=NULL && strcmp(type,"")==0)type = NULL;
		return (sys->internals->writematrixfn)(sys,fp,type);
	}else{
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Integrator '%s' defines no write_matrix function.",sys->internals->name);
		return -1;
	}
}

int integrator_debug(const IntegratorSystem *sys, FILE *fp){
	asc_assert(sys);
	asc_assert(sys->enginedata);
	asc_assert(sys->internals);
	asc_assert(sys->internals->name);
	if(sys->internals->debugfn){
		return (sys->internals->debugfn)(sys,fp);
	}else{
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Integrator '%s' defines no write_matrix function.",sys->internals->name);
		return -1;
	}
}

int integrator_apply_reinits_tracked(IntegratorSystem *sys, struct gl_list_t *applied_reinits){
	struct w_when **whens;
	int32 nwhens, w;
	IntegratorPreSnapshot snapshot;
	int applied = 0;

	if(sys == NULL || sys->system == NULL){
		return 0;
	}

	integrator_pre_snapshot_build(&snapshot, sys);
	whens = slv_get_solvers_when_list(sys->system);
	nwhens = slv_get_num_solvers_whens(sys->system);

	for(w = 0; w < nwhens; ++w){
		struct w_when *when = whens[w];
		struct Instance *wheninst;
		struct Instance *context;
		struct gl_list_t *solver_cases;
		unsigned long c, ncases;

		if(when == NULL){
			continue;
		}

		wheninst = (struct Instance *)when_instance(when);
		if(wheninst == NULL){
			continue;
		}

		context = InstanceParent(wheninst, 1);
		if(context == NULL){
			context = wheninst;
		}

		solver_cases = when_cases_list(when);
		if(solver_cases == NULL){
			continue;
		}

		ncases = gl_length(solver_cases);
		for(c = 1; c <= ncases; ++c){
			struct when_case *solver_case = (struct when_case *)gl_fetch(solver_cases, c);
			if(solver_case != NULL && when_case_active(solver_case)){
				struct gl_list_t *reinit_list = when_case_reinits_list(solver_case);
				if(integrator_apply_case_reinits(sys, context,
						reinit_list, &snapshot, applied_reinits, &applied) != 0){
					integrator_pre_snapshot_destroy(&snapshot);
					return -1;
				}
			}
		}
	}

	integrator_pre_snapshot_destroy(&snapshot);
	return applied;
}

int integrator_apply_reinits(IntegratorSystem *sys){
	return integrator_apply_reinits_tracked(sys, NULL);
}

/*----------------------------------------------------
	Build an analytic jacobian for solving the state system

	This necessarily ugly piece of code attempts to create a unique
	list of relations that explicitly contain the variables in the
	given input list. The utility of this information is that we know
	exactly which relations must be differentiated, to fill in the
	df/dy matrix. If the problem has very few derivative terms, this will
	be of great savings. If the problem arose from the discretization of
	a pde, then this will be not so useful. The decision wether to use
	this function or to simply differentiate the entire relations list
	must be done before calling this function.

	Final Note: the callee owns the array, but not the array elements.
 */
#define AVG_NUM_INCIDENT 4


/**
	This function helps to arrange the observation variables in a sensible order.
	The 'obs_id' child instance of v, if present, is assigned the value of the
	given parameter 'index'.
*/
static void Integ_SetObsId(struct var_variable *v, long index){
  struct Instance *c, *i;
  i = var_instance(v);
  asc_assert(i!=NULL);
  c = ChildByChar(i,OBSINDEX);
  if( c == NULL || InstanceKind(c) != INTEGER_INST || !AtomAssigned(c)) {
    return;
  }
  SetIntegerAtomValue(c,index,0);
}

/**
	Compares observation structs. NULLs should end up at far end.
*/
static int Integ_CmpObs(struct Integ_var_t *v1, struct Integ_var_t *v2){
  if(v1 == NULL)return 1;
  if(v2 == NULL)return -1;
  if(v1->index > v2->index)return 1;
  if(v1->index == v2->index)return 0;
  return -1;
}

/**
	Compares dynamic vars structs. NULLs should end up at far end.
	List should be sorted primarily by index and then by type, in order
	of increasing value of both.
*/
static int Integ_CmpDynVars(struct Integ_var_t *v1, struct Integ_var_t *v2){
  if(v1 == NULL)return 1;
  if(v2 == NULL)return -1;
  if(v1->index > v2->index)return 1;
  if(v1->index != v2->index)return -1;
  if(v1->type > v2->type)return 1;
  return -1;
}
/*----------------------------
  Output handling to the GUI/interface.
*/

int integrator_set_reporter(IntegratorSystem *sys
	, IntegratorReporter *reporter
){
	asc_assert(sys!=NULL);
	sys->reporter = reporter;
	/* ERROR_REPORTER_HERE(ASC_PROG_NOTE,"INTEGRATOR REPORTER HOOKS HAVE BEEN SET\n"); */
	return 1;
}

int integrator_output_init(IntegratorSystem *sys){
	asc_assert(sys!=NULL);
	asc_assert(sys->reporter!=NULL);
	if(sys->reporter->init!=NULL){
		/* call the specified output function */
		return (*(sys->reporter->init))(sys);
	}
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"No integrator reporter init method");
	return 1;
}

int integrator_output_write(IntegratorSystem *sys){
	static int reported_already=0;
	asc_assert(sys!=NULL);
	if(sys->reporter->write!=NULL){
		return (*(sys->reporter->write))(sys);
	}
	if(!reported_already){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"No integrator reporter write method (this message only shown once)");
		reported_already=1;
	}
	return 1;
}

int integrator_output_write_obs(IntegratorSystem *sys){
	static int reported_already=0;
	asc_assert(sys!=NULL);
	if(sys->reporter->write_obs!=NULL){
		return (*(sys->reporter->write_obs))(sys);
	}
	if(!reported_already){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"No integrator reporter write_obs method (this message only shown once)");
		reported_already=1;
	}
	return 1;
}

int integrator_output_close(IntegratorSystem *sys){
	asc_assert(sys!=NULL);
	if(sys->reporter->close!=NULL){
		return (*(sys->reporter->close))(sys);
	}
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"No integrator reporter close method");
	return 1;
}

/**
	Decode status codes from the integrator, and output them via FPRINTF.

	@return 0 on status ok (converged), <0 on unrecognised state, >0 on recognised error state.
*/
int integrator_checkstatus(slv_status_t status) {
	if(status.converged){
		return 0;
	}

	if(status.diverged){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"The derivative system did not converge."
			" Integration will terminate."); return 1;
	}else if(status.inconsistent){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"A numerically inconsistent state was discovered while"
			" calculating derivatives. Integration will terminate."); return 1;
	}else if(status.time_limit_exceeded){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"The time limit was exceeded while calculating"
			" derivatives. Integration will terminate."); return 1;
	}else if(status.iteration_limit_exceeded){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"The iteration limit was exceeded while calculating"
			" derivatives. Integration will terminate."); return 1;
	}else if(status.panic){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Integration cancelled by user."); return 1;
	}

	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unrecognised solver state (non converged)");
	return -1;
}
