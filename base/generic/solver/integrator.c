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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
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
#include "integrator.h"
#include "lsode.h"
#include "ida.h"

#include "samplelist.h"

/**
	Define as TRUE to enable debug message printing.
	@TODO this needs to go away.
*/
#define INTEG_DEBUG TRUE

/**
	Print a debug message with value if INTEG_DEBUG is TRUE.
*/
#define print_debug(msg, value) \
	if(INTEG_DEBUG){ CONSOLE_DEBUG(msg, value); }

/**
	Print a debug message string if INTEG_DEBUG is TRUE.
*/
#define print_debugstring(msg) \
	if(INTEG_DEBUG){ CONSOLE_DEBUG(msg); }

/*------------------------------------------------------------------------------
   The following names are of solver_var children or attributes
 * we support (at least temporarily) to determine who is a state and
 * who matching derivative.
 * These should be supported directly in a future solveratominst.
 */

static symchar *g_symbols[4];

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
/* Integer child. All variables with OBSINDEX !=0 will be recorded in
 * the blsode output file. Tis someone else's job to grok this output.
 */

#define FIXEDSYMBOL g_symbols[3]

/** Temporary catcher of dynamic variable and observation variable data */
struct Integ_var_t {
  long index;
  long type;
  struct var_variable *i;
  struct Integ_var_t *derivative_of;
  struct var_variable *derivative;
  int isstate;
};

/*------------------------------------------------------------------------------
  forward declarations
*/

/* abstractions of setup/teardown procedures for the specific solvers */
void integrator_create_engine(IntegratorSystem *blsys);
void integrator_free_engine(IntegratorSystem *blsys);

static int integrator_analyse_ode(IntegratorSystem *blsys);
static int integrator_analyse_dae(IntegratorSystem *blsys);

typedef void (IntegratorVarVisitorFn)(IntegratorSystem *blsys, struct var_variable *var);
static void integrator_visit_system_vars(IntegratorSystem *blsys,IntegratorVarVisitorFn *visitor);
IntegratorVarVisitorFn integrator_ode_classify_var;
IntegratorVarVisitorFn integrator_dae_classify_var;
IntegratorVarVisitorFn integrator_classify_indep_var;

static int integrator_sort_obs_vars(IntegratorSystem *blsys);
static void integrator_print_var_stats(IntegratorSystem *blsys);
static int integrator_check_indep_var(IntegratorSystem *blsys);

static int Integ_CmpDynVars(struct Integ_var_t *v1, struct Integ_var_t *v2);
static int Integ_CmpObs(struct Integ_var_t *v1, struct Integ_var_t *v2);
static void Integ_SetObsId(struct var_variable *v, long index);

static long DynamicVarInfo(struct var_variable *v,long *index);
static struct var_variable *ObservationVar(struct var_variable *v, long *index);
static void IntegInitSymbols(void);

/*------------------------------------------------------------------------------
  INSTANTIATION AND DESTRUCTION
*/

/**
	Create a new IntegratorSystem and assign a slv_system_t to it.
*/
IntegratorSystem *integrator_new(slv_system_t sys, struct Instance *inst){
	IntegratorSystem *blsys;

	if (sys == NULL) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"sys is NULL!");
		return NULL;
	}

	blsys = ASC_NEW_CLEAR(IntegratorSystem);
	blsys->system = sys;
	blsys->instance = inst;
	return blsys;
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

	if(sys->states != NULL)gl_destroy(sys->states);
	if(sys->derivs != NULL)gl_destroy(sys->derivs);

	if(sys->dynvars != NULL)gl_free_and_destroy(sys->dynvars);    /* we own the objects in dynvars */
	if(sys->obslist != NULL)gl_free_and_destroy(sys->obslist);    /* and obslist */
	if (sys->indepvars != NULL)gl_free_and_destroy(sys->indepvars);  /* and indepvars */

	if(sys->y_id != NULL)ascfree(sys->y_id);
	if(sys->obs_id != NULL)ascfree(sys->obs_id);

	if(sys->y != NULL && !sys->ycount)ascfree(sys->y);
	if(sys->ydot != NULL && !sys->ydotcount)ascfree(sys->ydot);
	if(sys->obs != NULL && !sys->obscount)ascfree(sys->obs);

	ascfree(sys);
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
  FIXEDSYMBOL = AddSymbol("fixed");
}

/*------------------------------------------------------------------------------
  INTEGRATOR ENGINE
*/

/* return 1 on success */
int integrator_set_engine(IntegratorSystem *blsys, IntegratorEngine engine){

	/* verify integrator type ok. always passes for nonNULL inst. */
	if(engine==INTEG_UNKNOWN){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR
			,"Integrator has not been specified (or is unknown)."
		);
		return 0;
	}

	if(engine==blsys->engine){
		return 1;
	}
	if(blsys->engine!=INTEG_UNKNOWN){
		integrator_free_engine(blsys);
	}
	blsys->engine = engine;
	integrator_create_engine(blsys);

	return 1;
}

IntegratorEngine integrator_get_engine(const IntegratorSystem *blsys){
	return blsys->engine;
}

/**
	Free any engine-specific  data that was required for the solution of
	this system. Note that this data is pointed to by blsys->enginedata.
*/
void integrator_free_engine(IntegratorSystem *blsys){
	switch(blsys->engine){
		case INTEG_LSODE: integrator_lsode_free(blsys->enginedata); break;
#ifdef ASC_WITH_IDA
		case INTEG_IDA: integrator_ida_free(blsys->enginedata); break;
#endif
		default: break;
	}
	blsys->enginedata=NULL;
}

/**
	Create enginedata memory if required for this solver. This doesn't include
	allocating computation space, since we assume that this stage all we know
	is that we want to use a specified integrator engine, not the full details
	of the problem at hand. Allocating space inside enginedata should be done
	during the solve stage (and freed inside integrator_free_engine)
*/
void integrator_create_engine(IntegratorSystem *blsys){
	if(blsys->enginedata!=NULL)return;
	switch(blsys->engine){
		case INTEG_LSODE: integrator_lsode_create(blsys); break;
#ifdef ASC_WITH_IDA
		case INTEG_IDA: integrator_ida_create(blsys); break;
#endif
		default: break;
	}
}

/*------------------------------------------------------------------------------
  ANALYSIS

  Provide two modes in order to provide analysis suitable for solution of both
  ODEs (the previous technique) and DAEs (new code). These share a common
  "visit" method that needs to eventually be integrated with the code in
  <solver/analyze.c>. For the moment, we're just hacking in to the compiler.
*/

/**
	Locate the independent variable. For the purpose of GUI design, this needs
	to work independent of the integration engine being used.
*/
int integrator_find_indep_var(IntegratorSystem *blsys){
	int result = 0;

	if(blsys->x != NULL){
		CONSOLE_DEBUG("blsys->x already set");
		return 1;
	}
	assert(blsys->indepvars==NULL);
	blsys->indepvars = gl_create(10L);

	IntegInitSymbols();

	/* CONSOLE_DEBUG("About to visit..."); */
	integrator_visit_system_vars(blsys,&integrator_classify_indep_var);

	/* CONSOLE_DEBUG("..."); */

	result = integrator_check_indep_var(blsys);
	gl_free_and_destroy(blsys->indepvars);
	blsys->indepvars = NULL;

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Returning result %d",result);

	return result;
}

/**
	Analyse the system, either as DAE or as an ODE system, depending on the
	solver engine selected.

	@return 1 on success
*/
int integrator_analyse(IntegratorSystem *blsys){
	switch(blsys->engine){
		case INTEG_LSODE: return integrator_analyse_ode(blsys);
#ifdef ASC_WITH_IDA
		case INTEG_IDA: return integrator_analyse_dae(blsys);
#endif
		case INTEG_UNKNOWN:
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"No engine selected: can't analyse");
		default:
			ERROR_REPORTER_HERE(ASC_PROG_ERR
				,"The selected integration engine (%d) is not available"
				,blsys->engine
			);
	}
	return 0;
}

/**
	To analyse a DAE we need to identify *ALL* variables in the system
	Except for the highest-level derivatives of any present?
	We also need to identify the independent variable (just one).

	@TODO implement Pantelides algorithm in here?
	@TODO prevent re-analysis without clearing out the data structures?
	@return 1 on success
*/
int integrator_analyse_dae(IntegratorSystem *blsys){
	struct Integ_var_t *info, *prev;
	char *varname, *derivname;
	int i, j;
	int numstates;
	int numy, nrels;
	int maxderiv;

	CONSOLE_DEBUG("Starting DAE analysis");
	IntegInitSymbols();

	assert(blsys->indepvars==NULL);

	blsys->indepvars = gl_create(10L);  /* t var info */
	blsys->dynvars = gl_create(200L);  /* y ydot var info */
	blsys->obslist = gl_create(100L);  /* obs info */

	if(blsys->indepvars==NULL
		|| blsys->dynvars==NULL
		|| blsys->obslist==NULL
	){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory");
		return 0;
	}

	integrator_visit_system_vars(blsys,&integrator_dae_classify_var);

	CONSOLE_DEBUG("Found %lu observation variables:",gl_length(blsys->obslist));
	for(i=1; i<=gl_length(blsys->obslist); ++i){
		info = (struct Integ_var_t *)gl_fetch(blsys->obslist, i);
		varname = var_make_name(blsys->system,info->i);
		CONSOLE_DEBUG("observation[%d] = \"%s\"",i,varname);
		ASC_FREE(varname);
	}

	CONSOLE_DEBUG("Checking found vars...");
	if(gl_length(blsys->dynvars)==0){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"No solver_var vars found to integrate (check 'ode_type'?).");
		return 0;
	}

	CONSOLE_DEBUG("Found %lu vars.", gl_length(blsys->dynvars));

	maxderiv = 0;
	numstates = 0;
	for(i=1; i<=gl_length(blsys->dynvars); ++i){
		info = (struct Integ_var_t *)gl_fetch(blsys->dynvars, i);
		if(info->index==1 || info->index==0)numstates++;
		if(maxderiv < info->type - 1)maxderiv = info->type - 1;
		varname = var_make_name(blsys->system,info->i);
		CONSOLE_DEBUG("var[%d] = \"%s\": order = %ld",i,varname,info->type-1);
		ASC_FREE(varname);
	}
	if(maxderiv == 0){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"No derivatives found (check 'ode_type' values in your vars).");
		return 0;
	}
	if(numstates == 0){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"No states found (check 'odetype' values in your vars).");
		return 0;
	}


	if(!integrator_check_indep_var(blsys))return 0;

	gl_sort(blsys->dynvars,(CmpFunc)Integ_CmpDynVars);

	/* sort into state-groups with ascending derivs */

	for(i=1; i<=gl_length(blsys->dynvars); ++i){
		info = (struct Integ_var_t *)gl_fetch(blsys->dynvars, i);
		varname = var_make_name(blsys->system,info->i);
		CONSOLE_DEBUG("var[%d] = \"%s\": order = %ld",i,varname,info->type-1);
		ASC_FREE(varname);
	}

	/* link up derivative chains */

	prev = NULL;
	for(i=1; i<=gl_length(blsys->dynvars); ++i){ /* why does gl_list index with base 1??? */
		info = (struct Integ_var_t *)gl_fetch(blsys->dynvars, i);
		info->derivative = NULL;

		derivname = var_make_name(blsys->system,info->i);
		if(prev!=NULL){
			varname = var_make_name(blsys->system,prev->i);
		}else{
			varname = NULL;
		}
		CONSOLE_DEBUG("current = \"%s\", previous = \"%s\"",derivname,varname);
		ASC_FREE(derivname);
		if(varname)ASC_FREE(varname);

		if(info->type == INTEG_STATE_VAR || info->type == INTEG_ALGEBRAIC_VAR){
			varname = var_make_name(blsys->system,info->i);
			CONSOLE_DEBUG("Var \"%s\" is not a derivative",varname);
			ASC_FREE(varname);
			info->derivative_of = NULL;
			info->type = INTEG_STATE_VAR;
		}else{
			if(prev==NULL || info->index != prev->index){
				CONSOLE_DEBUG("current current type = %ld",info->type);
				if(prev!=NULL){
					CONSOLE_DEBUG("current index = %ld, previous = %ld",info->index,prev->index);
				}else{
					CONSOLE_DEBUG("current index = %ld, current type = %ld",info->index,info->type);
				}
				varname = var_make_name(blsys->system,info->i);
				ERROR_REPORTER_HERE(ASC_USER_ERROR,"Derivative %d of \"%s\" is present without its un-differentiated equivalent"
					, info->type-1
					, varname
				);
				ASC_FREE(varname);
				return 0;
			}else if(info->type != prev->type + 1){
				derivname = var_make_name(blsys->system,info->i);
				varname = var_make_name(blsys->system,prev->i);
				ERROR_REPORTER_HERE(ASC_USER_ERROR
					,"Looking at \"%s\", expected a derivative (order %d) of \"%s\"."
					,varname
					,prev->type+1
					,derivname
				);
				ASC_FREE(varname);
				ASC_FREE(derivname);
				return 0;
			}else{
				varname = var_make_name(blsys->system,prev->i);
				derivname = var_make_name(blsys->system,info->i);
				CONSOLE_DEBUG("Var \"%s\" is the derivative of \"%s\"",derivname,varname);
				ASC_FREE(varname);
				ASC_FREE(derivname);
				info->derivative_of = prev;
				numy++;
			}
		}
		prev = info;
	}

	/* record which vars have derivatives and which don't */
	for(i=1; i<=gl_length(blsys->dynvars); ++i){
		info = (struct Integ_var_t *)gl_fetch(blsys->dynvars, i);
		if(info->derivative_of){
			info->derivative_of->derivative = info->i;
		}
	}

	CONSOLE_DEBUG("Indentifying states...");

	/* count numy: either it's a state, or it has a higher-order derivative */
	numy = 0;
	for(i=1; i<=gl_length(blsys->dynvars); ++i){
		info = (struct Integ_var_t *)gl_fetch(blsys->dynvars, i);
		if(info->type == INTEG_STATE_VAR || info->type == INTEG_ALGEBRAIC_VAR || info->derivative != NULL){
			varname = var_make_name(blsys->system,info->i);
			CONSOLE_DEBUG("Var \"%s\" is a state variable",varname);
			ASC_FREE(varname);
			info->isstate = 1;
			numy++;
		}else{
			info->isstate = 0;
		}
	}

	/*
		create lists 'y' and 'ydot'. some elements of ydot don't correspond
		to variables in our model: these are the algebraic vars.
	*/

	CONSOLE_DEBUG("Identified %d state variables", numy);

	blsys->y = ASC_NEW_ARRAY(struct var_variable *,numy);
	blsys->ydot = ASC_NEW_ARRAY(struct var_variable *,numy);

	/*
		at this point we know there are no missing derivatives etc, so we
		can use (i-1) as the index into y and ydot. any variable with
		'derivative_of' set to null is a state variable... but it might already
		be getting added
	*/
	for(j=0, i=1; i<=gl_length(blsys->dynvars); ++i){
		info = (struct Integ_var_t *)gl_fetch(blsys->dynvars, i);
		if(!info->isstate)continue;
		if(info->derivative == NULL){
			blsys->y[j] = info->i;
			blsys->ydot[j] = NULL;
		}else{
			blsys->y[j] = info->i;
			blsys->ydot[j] = info->derivative;
		}
		++j;
	}

	nrels = slv_get_num_solvers_rels(blsys->system);
	if(numy != nrels){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			,"System is not square: solver has %d rels, found %d system states"
			,nrels, numy
		);
		return 0;
	}

	blsys->n_y = numy;

	if(!integrator_sort_obs_vars(blsys))return 0;

	return 1;
}

void integrator_visit_system_vars(IntegratorSystem *blsys,IntegratorVarVisitorFn *visitfn){
  struct var_variable **vlist;
  int i, vlen;

  /* visit all the slv_system_t master var lists to collect vars */
  /* find the vars mostly in this one */
  vlist = slv_get_master_var_list(blsys->system);
  vlen = slv_get_num_master_vars(blsys->system);
  for (i=0;i<vlen;i++) {
    (*visitfn)(blsys, vlist[i]);
  }

  /*
  CONSOLE_DEBUG("Checked %d vars",vlen);
  integrator_print_var_stats(blsys);
  */

  /* probably nothing here, but gotta check. */
  vlist = slv_get_master_par_list(blsys->system);
  vlen = slv_get_num_master_pars(blsys->system);
  for (i=0;i<vlen;i++) {
    (*visitfn)(blsys, vlist[i]);
  }

  /*
  CONSOLE_DEBUG("Checked %d pars",vlen);
  integrator_print_var_stats(blsys);
  */

  /* might find t here */
  vlist = slv_get_master_unattached_list(blsys->system);
  vlen = slv_get_num_master_unattached(blsys->system);
  for (i=0;i<vlen;i++) {
    (*visitfn)(blsys, vlist[i]);
  }

  /* CONSOLE_DEBUG("Checked %d unattached",vlen); */
}
/**
	@return 1 on success
*/
int integrator_analyse_ode(IntegratorSystem *blsys){
  struct Integ_var_t *v1,*v2;
  long half,i,len;
  int happy=1;

  CONSOLE_DEBUG("Starting ODE analysis");
  IntegInitSymbols();

  /* collect potential states and derivatives */
  blsys->indepvars = gl_create(10L);  /* t var info */
  blsys->dynvars = gl_create(200L);  /* y ydot var info */
  blsys->obslist = gl_create(100L);  /* obs info */
  if (blsys->dynvars == NULL
    || blsys->obslist == NULL
    || blsys->indepvars == NULL
  ){
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory.");
    return 0;
  }

  blsys->nstates = blsys->nderivs = 0;

  integrator_visit_system_vars(blsys,&integrator_ode_classify_var);

  integrator_print_var_stats(blsys);

  if(!integrator_check_indep_var(blsys))return 0;

  /* check sanity of state and var lists */

  len = gl_length(blsys->dynvars);
  half = len/2;
  CONSOLE_DEBUG("NUMBER OF DYNAMIC VARIABLES = %ld",half);

  if (len % 2 || len == 0L || blsys->nstates != blsys->nderivs ) {
    /* list length must be even for vars to pair off */
    ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"n_y != n_ydot, or no dynamic vars found. Fix your indexing.");
    return 0;
  }
  gl_sort(blsys->dynvars,(CmpFunc)Integ_CmpDynVars);
  if (gl_fetch(blsys->dynvars,len)==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Mysterious NULL found!");
    return 0;
  }
  blsys->states = gl_create(half);   /* state vars Integ_var_t references */
  blsys->derivs = gl_create(half);   /* derivative var atoms */
  for (i=1;i < len; i+=2) {
    v1 = (struct Integ_var_t *)gl_fetch(blsys->dynvars,i);
    v2 = (struct Integ_var_t *)gl_fetch(blsys->dynvars,i+1);
    if (v1->type!=1  || v2 ->type !=2 || v1->index != v2->index) {
      ERROR_REPORTER_HERE(ASC_USER_ERROR,"Mistyped or misindexed dynamic variables: (%s = %ld,%s = %ld),(%s = %ld,%s = %ld).",
             SCP(STATEFLAG),v1->type,SCP(STATEINDEX),v1->index,
             SCP(STATEFLAG),v2->type,SCP(STATEINDEX),v2->index
		);
      happy=0;
      break;
    } else {
      gl_append_ptr(blsys->states,(POINTER)v1);
      gl_append_ptr(blsys->derivs,(POINTER)v2->i);
    }
  }
  if (!happy) {
    return 0;
  }
  blsys->n_y = half;
  blsys->y = ASC_NEW_ARRAY(struct var_variable *, half);
  blsys->y_id = ASC_NEW_ARRAY(long, half);
  blsys->ydot = ASC_NEW_ARRAY(struct var_variable *, half);
  if (blsys->y==NULL || blsys->ydot==NULL || blsys->y_id==NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory.");
    return 0;
  }
  for (i = 1; i <= half; i++) {
    v1 = (struct Integ_var_t *)gl_fetch(blsys->states,i);
    blsys->y[i-1] = v1->i;
    blsys->y_id[i-1] = v1->index;
    blsys->ydot[i-1] = (struct var_variable *)gl_fetch(blsys->derivs,i);
  }

  if(!integrator_sort_obs_vars(blsys))return 0;

  /* don't need the gl_lists now that we have arrays for everyone */
  gl_destroy(blsys->states);
  gl_destroy(blsys->derivs);
  gl_free_and_destroy(blsys->indepvars);  /* we own the objects in indepvars */
  gl_free_and_destroy(blsys->dynvars);    /* we own the objects in dynvars */
  gl_free_and_destroy(blsys->obslist);    /* and obslist */
  blsys->states = NULL;
  blsys->derivs = NULL;
  blsys->indepvars = NULL;
  blsys->dynvars = NULL;
  blsys->obslist = NULL;

  /* analysis completed OK */
  return 1;
}

/**
	Reindex observations. Sort if the user mostly numbered. Take natural order
	if user just booleaned.

	@return 1 on success
*/
static int integrator_sort_obs_vars(IntegratorSystem *blsys){
  int half, len, i;
  struct Integ_var_t *v2;

  half = blsys->n_y;
  len = gl_length(blsys->obslist);
  /* we shouldn't be seeing NULL here ever except if malloc fail. */
  if (len > 1L) {
    half = ((struct Integ_var_t *)gl_fetch(blsys->obslist,1))->index;
    /* half != 0 now because we didn't collect 0 indexed vars */
    for (i=2; i <= len; i++) {
      if (half != ((struct Integ_var_t *)gl_fetch(blsys->obslist,i))->index) {
        /* change seen. sort and go on */
        gl_sort(blsys->obslist,(CmpFunc)Integ_CmpObs);
        break;
      }
    }
  }
  for (i = half = 1; i <= len; i++) {
    v2 = (struct Integ_var_t *)gl_fetch(blsys->obslist,i);
    if (v2==NULL) {
      /* we shouldn't be seeing NULL here ever except if malloc fail. */
      gl_delete(blsys->obslist,i,0); /* should not be gl_delete(so,i,1) */
    } else {
      Integ_SetObsId(v2->i,half);
      v2->index = half++;
    }
  }

  /* obslist now uniquely indexed, no nulls */
  /* make into arrays */
  half = gl_length(blsys->obslist);
  blsys->obs = ASC_NEW_ARRAY(struct var_variable *,half);
  blsys->obs_id = ASC_NEW_ARRAY(long, half);
  if ( blsys->obs==NULL || blsys->obs_id==NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory.");
    return 0;
  }
  blsys->n_obs = half;
  for (i = 1; i <= half; i++) {
    v2 = (struct Integ_var_t *)gl_fetch(blsys->obslist,i);
    blsys->obs[i-1] = v2->i;
    blsys->obs_id[i-1] = v2->index;
  }

  return 1;
}

static void integrator_print_var_stats(IntegratorSystem *blsys){
	int v = gl_length(blsys->dynvars);
	int i = gl_length(blsys->indepvars);
	CONSOLE_DEBUG("Currently %d vars, %d indep",v,i);
}

/**
	@return 1 on success
*/
static int integrator_check_indep_var(IntegratorSystem *blsys){
  int len, i;
  struct Integ_var_t *info;
  char *varname;

  /* check the sanity of the independent variable */
  len = gl_length(blsys->indepvars);
  if (!len) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"No independent variable found.");
    return 0;
  }
  if (len > 1) {
	ERROR_REPORTER_START_HERE(ASC_USER_ERROR);
    FPRINTF(ASCERR,"Excess %ld independent variables found:",
      len);
    for(i=1; i <=len;i++) {
      info = (struct Integ_var_t *)gl_fetch(blsys->indepvars,i);
      if(info==NULL)continue;

      varname = var_make_name(blsys->system,info->i);
      FPRINTF(ASCERR," %s",i,varname);
      ASC_FREE(varname);
    }
    FPRINTF(ASCERR , "\nSet the \"%s\" flag on all but one of these to %s >= 0.\n"
        , SCP(STATEFLAG),SCP(STATEFLAG)
	);
	error_reporter_end_flush();
    return 0;
  }else{
    info = (struct Integ_var_t *)gl_fetch(blsys->indepvars,1);
    blsys->x = info->i;
  }
  return 1;
}

/*------------------------------------------------------------------------------
  CLASSIFICATION OF VARIABLES (for ANALYSIS step)
*/

#define INTEG_ADD_TO_LIST(info,TYPE,INDEX,VAR,LIST) \
	info = ASC_NEW(struct Integ_var_t); \
	if(info==NULL){ \
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory (INTEG_VAR_NEW)"); \
		return; \
	} \
	info->type=TYPE; \
	info->index=INDEX; \
	info->i=VAR; \
	gl_append_ptr(LIST,(void *)info); \
	info = NULL

/**
	In a DAE, it's either the (single) independent variable, or it's a
	variable in the model.

	I'm not sure what we should be doing with variables that are already
	present as derivatives of other variables, I guess those ones need to be
	removed from the list in a second pass?
*/
void integrator_dae_classify_var(IntegratorSystem *blsys, struct var_variable *var){
	struct Integ_var_t *info;
	long type,index;

	/* filter for recognition of solver_vars */
	var_filter_t vfilt;
	vfilt.matchbits = VAR_SVAR;
	vfilt.matchvalue = VAR_SVAR;

	assert(var != NULL && var_instance(var)!=NULL );

	if( var_apply_filter(var,&vfilt) ) {

		if(!var_fixed(var)){
			/* get the ode_type and ode_id of this solver_var */
			type = DynamicVarInfo(var,&index);

			if(type==INTEG_OTHER_VAR){
				/* if the var's type is -1, it's independent */
				INTEG_ADD_TO_LIST(info,INTEG_OTHER_VAR,0,var,blsys->indepvars);
			}else{
				if(type < 0)type=0;
				/* any other type of var is in the DAE system, at least for now */
				INTEG_ADD_TO_LIST(info,type,index,var,blsys->dynvars);
			}
		}

		/* if the var's obs_id > 0, add it to the observation list */
		if(ObservationVar(var,&index) != NULL && index > 0L) {
			INTEG_ADD_TO_LIST(info,type,index,var,blsys->obslist);
		}
	}
}

/**
	Inspect a specific variable and work out what type it is (what 'ode_type' it
	has) and what other variable(s) it corresponds to (ie dydt corresponds to
	y as a derivative).

	@TODO add ability to create new variables for 'missing' derivative vars?
*/
void integrator_ode_classify_var(IntegratorSystem *blsys, struct var_variable *var){
  struct Integ_var_t *info;
  long type,index;

  var_filter_t vfilt;
  vfilt.matchbits = VAR_SVAR;
  vfilt.matchvalue = VAR_SVAR;

  assert(var != NULL && var_instance(var)!=NULL );

  if( var_apply_filter(var,&vfilt) ) {
	/* it's a solver var: what type of variable? */
    type = DynamicVarInfo(var,&index);

    if(type==INTEG_ALGEBRAIC_VAR){
		/* no action required */
	}else if(type==INTEG_OTHER_VAR){
		/* i.e. independent var */
        INTEG_ADD_TO_LIST(info,type,index,var,blsys->indepvars);
	}else if(type>=INTEG_STATE_VAR){
        INTEG_ADD_TO_LIST(info,type,index,var,blsys->dynvars);
        if(type == 1){
          blsys->nstates++;
        }else if(type == 2){ /* what about higher-order derivatives? -- JP */
          blsys->nderivs++;
        }else{
		  ERROR_REPORTER_HERE(ASC_USER_WARNING,"Higher-order (>=2) derivatives are not supported in ODEs.");
		}	}

    if(ObservationVar(var,&index) != NULL && index > 0L) {
		INTEG_ADD_TO_LIST(info,0L,index,var,blsys->obslist);
    }
  }
}

/**
	Look at a variable and determine if it's the independent variable or not.
	This is just for the purpose of the integrator_find_indep_var function,
	which is a utility function provided for use by the GUI.
*/
void integrator_classify_indep_var(IntegratorSystem *blsys, struct var_variable *var){
	struct Integ_var_t *info;
	long type,index;

	var_filter_t vfilt;
	vfilt.matchbits = VAR_SVAR;
	vfilt.matchvalue = VAR_SVAR;

	/* CONSOLE_DEBUG("..."); */

	assert(var != NULL && var_instance(var)!=NULL );

	if( var_apply_filter(var,&vfilt) ) {
		type = DynamicVarInfo(var,&index);

		if(type==INTEG_OTHER_VAR){
			/* i.e. independent var */
			INTEG_ADD_TO_LIST(info,type,index,var,blsys->indepvars);
		}
	}
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
static long DynamicVarInfo(struct var_variable *v,long *index){
  struct Instance *c, *d, *i;
  i = var_instance(v);
  assert(i!=NULL);
  assert(STATEFLAG!=NULL);
  assert(STATEINDEX!=NULL);
  c = ChildByChar(i,STATEFLAG);
  d = ChildByChar(i,STATEINDEX);
  /* lazy evaluation is important in the following if */
  if(c == NULL
      || d == NULL
      || InstanceKind(c) != INTEGER_INST
      || InstanceKind(d) != INTEGER_INST
      || !AtomAssigned(c)
      || (!AtomAssigned(d) && GetIntegerAtomValue(c) != INTEG_OTHER_VAR)
  ){
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
  assert(i!=NULL);
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
	time values specified. The blsys contains all the specifics.

	Return 1 on success
*/
int integrator_solve(IntegratorSystem *blsys, long i0, long i1){

	long nstep;
	unsigned long start_index=0, finish_index=0;

	assert(blsys!=NULL);

	nstep = integrator_getnsamples(blsys)-1;
	/* check for at least 2 steps and dimensionality of x vs steps here */

	if (i0<0 || i1 <0) {
		/* dude, there's no way we're writing interactive stuff here... */
		ERROR_REPORTER_HERE(ASC_PROG_ERROR,"Console input of integration limits has been disabled!");
		return 0;
	}else{
		start_index=i0;
		finish_index =i1;
		if (start_index >= (unsigned long)nstep) {
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Start point (=%lu) must be an index in the range [0,%li]."
				,start_index,nstep
			);
			return 0;
		}
		if (finish_index > (unsigned long)nstep) {
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"End point (=%lu) must be an index in the range [0,%li]."
				,finish_index,nstep
			);
			return 0;
		}
    }

	if(finish_index <= start_index) {
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"End point comes before start point! (start=%lu, end=%lu)"
			,start_index,finish_index
		);
		return 0;
	}

	/* now go and run the integrator */
	switch (blsys->engine) {
		case INTEG_LSODE: return integrator_lsode_solve(blsys, start_index, finish_index); break;
#ifdef ASC_WITH_IDA
		case INTEG_IDA: return integrator_ida_solve(blsys,start_index, finish_index); break;
#endif
		default:
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unknown integrator (invalid, or not implemented yet)");
			return 0;
	}
}

/*---------------------------------------------------------------
  HANDLING THE LIST OF TIMESTEMPS
*/

#define GETTER_AND_SETTER(TYPE,NAME) \
	void integrator_set_##NAME(IntegratorSystem *blsys, TYPE val){ \
		blsys->NAME=val; \
	} \
	TYPE integrator_get_##NAME(IntegratorSystem *blsys){ \
		return blsys->NAME; \
	}

GETTER_AND_SETTER(SampleList *,samples);
GETTER_AND_SETTER(double,maxstep);
GETTER_AND_SETTER(double,minstep);
GETTER_AND_SETTER(double,stepzero);
GETTER_AND_SETTER(int,maxsubsteps);
#undef GETTER_AND_SETTER

long integrator_getnsamples(IntegratorSystem *blsys){
	assert(blsys!=NULL);
	assert(blsys->samples!=NULL);
	return samplelist_length(blsys->samples);
}

double integrator_getsample(IntegratorSystem *blsys, long i){
	assert(blsys!=NULL);
	assert(blsys->samples!=NULL);
	return samplelist_get(blsys->samples,i);
}

void integrator_setsample(IntegratorSystem *blsys, long i,double xi){
	assert(blsys!=NULL);
	assert(blsys->samples!=NULL);
	samplelist_set(blsys->samples,i,xi);
}

const dim_type *integrator_getsampledim(IntegratorSystem *blsys){
	assert(blsys!=NULL);
	assert(blsys->samples!=NULL);
	return samplelist_dim(blsys->samples);
}

ASC_DLLSPEC(long) integrator_getcurrentstep(IntegratorSystem *blsys){
	return blsys->currentstep;
}

/*------------------------------------------------------------------------------
  GET/SET VALUE OF THE INDEP VARIABLE
*/

/**
	Retrieve the value of the independent variable (time) from ASCEND
	and return it as a double.
*/
double integrator_get_t(IntegratorSystem *blsys){
	assert(blsys->x!=NULL);
	return var_value(blsys->x);
}

/**
	Set the value of the independent variable (time) in ASCEND.
*/
void integrator_set_t(IntegratorSystem *blsys, double value){
  var_set_value(blsys->x, value);
  CONSOLE_DEBUG("set_t = %g", value);
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
double *integrator_get_y(IntegratorSystem *blsys, double *y) {
  long i;

  if (y==NULL) {
    y = ASC_NEW_ARRAY_CLEAR(double, blsys->n_y+1);
    /* C y[0]  <==> ascend d.y[1]  <==>  f77 y(1) */
  }

  for (i=0; i< blsys->n_y; i++) {
	assert(blsys->y[i]!=NULL);
    y[i] = var_value(blsys->y[i]);
    CONSOLE_DEBUG("ASCEND --> y[%ld] = %g", i+1, y[i]);
  }
  return y;
}

/**
	Take the values of the derivatives from the array that the integrator
	uses, and use them to update the values of the corresponding variables
	in ASCEND.
*/
void integrator_set_y(IntegratorSystem *blsys, double *y) {
  long i;
#ifndef NDEBUG
  char *varname;
#endif

  for (i=0; i < blsys->n_y; i++) {
	assert(blsys->y[i]!=NULL);
    var_set_value(blsys->y[i],y[i]);
#ifndef NDEBUG
	varname = var_make_name(blsys->system, blsys->y[i]);
    CONSOLE_DEBUG("y[%ld] = \"%s\" = %g --> ASCEND", i+1, varname, y[i]);
	ASC_FREE(varname);
#endif
  }
}

/**
	Send the values of the derivatives of the 'y' variables to the solver.
	Allocate space for an array if necessary.

	Any element in blsys->ydot that is NULL will be passed over (the value
	won't be modified in dydx).
*/
double *integrator_get_ydot(IntegratorSystem *blsys, double *dydx) {
  long i;

  if (dydx==NULL) {
    dydx = ASC_NEW_ARRAY_CLEAR(double, blsys->n_y+1);
    /* C dydx[0]  <==> ascend d.dydx[1]  <==>  f77 ydot(1) */
  }

  for (i=0; i < blsys->n_y; i++) {
    if(blsys->ydot[i]!=NULL){
		dydx[i] = var_value(blsys->ydot[i]);
	}
    CONSOLE_DEBUG("ASCEND --> ydot[%ld] = %g", i+1, dydx[i]);
  }
  return dydx;
}

void integrator_set_ydot(IntegratorSystem *blsys, double *dydx) {
	long i;
#ifndef NDEBUG
	char *varname;
#endif
	for (i=0; i < blsys->n_y; i++) {
		if(blsys->ydot[i]!=NULL){
    		var_set_value(blsys->ydot[i],dydx[i]);
#ifndef NDEBUG
			varname = var_make_name(blsys->system, blsys->ydot[i]);
			CONSOLE_DEBUG("ydot[%ld] = \"%s\" = %g --> ASCEND", i+1, varname, dydx[i]);
			ASC_FREE(varname);
#endif
		}
#ifndef NDEBUG
		else{
			CONSOLE_DEBUG("ydot[%ld] = %g (internal)", i+1, dydx[i]);
		}
#endif
	}
}

/*-------------------------------------------------------------
  RETRIEVING OBSERVATION DATA

   This function takes the inst in the solver and returns the vector of
   observation variables that are located in the submodel d.obs array.
*/
double *integrator_get_observations(IntegratorSystem *blsys, double *obsi) {
  long i;

  if (obsi==NULL) {
    obsi = ASC_NEW_ARRAY_CLEAR(double, blsys->n_obs+1);
  }

  /* C obsi[0]  <==> ascend d.obs[1] */

  for (i=0; i < blsys->n_obs; i++) {
    obsi[i] = var_value(blsys->obs[i]);
    /* CONSOLE_DEBUG("*get_d_obs[%ld] = %g\n", i+1, obsi[i]); */
  }
  return obsi;
}

struct var_variable *integrator_get_observed_var(IntegratorSystem *blsys, const long i){
	assert(i>=0);
	assert(i<blsys->n_obs);
	return blsys->obs[i];
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
  assert(i!=NULL);
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

int integrator_set_reporter(IntegratorSystem *blsys
	, IntegratorReporter *reporter
){
	assert(blsys!=NULL);
	blsys->reporter = reporter;
	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"INTEGRATOR REPORTER HOOKS HAVE BEEN SET\n");
	return 1;
}

int integrator_output_init(IntegratorSystem *blsys){
	CONSOLE_DEBUG("...");
	assert(blsys!=NULL);
	assert(blsys->reporter!=NULL);
	if(blsys->reporter->init!=NULL){
		/* call the specified output function */
		return (*(blsys->reporter->init))(blsys);
	}
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"No integrator reporter init method");
	return 1;
}

int integrator_output_write(IntegratorSystem *blsys){
	static int reported_already=0;
	assert(blsys!=NULL);
	if(blsys->reporter->write!=NULL){
		return (*(blsys->reporter->write))(blsys);
	}
	if(!reported_already){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"No integrator reporter write method (this message only shown once)");
		reported_already=1;
	}
	return 1;
}

int integrator_output_write_obs(IntegratorSystem *blsys){
	static int reported_already=0;
	assert(blsys!=NULL);
	if(blsys->reporter->write_obs!=NULL){
		return (*(blsys->reporter->write_obs))(blsys);
	}
	if(!reported_already){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"No integrator reporter write_obs method (this message only shown once)");
		reported_already=1;
	}
	return 1;
}

int integrator_output_close(IntegratorSystem *blsys){
	assert(blsys!=NULL);
	if(blsys->reporter->close!=NULL){
		return (*(blsys->reporter->close))(blsys);
	}
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"No integrator reporter close method");
	return 1;
}

/**
	Decode status codes from the integrator, and output them via FPRINTF.
*/
int integrator_checkstatus(slv_status_t status) {
  if (status.converged) {
    return 1;
  }
  if (status.diverged) {
    FPRINTF(stderr, "The derivative system did not converge.\n");
    FPRINTF(stderr, "Integration will be terminated ");
    FPRINTF(stderr, "at the end of the current step.\n");
    return 0;
  }
  if (status.inconsistent) {
    FPRINTF(stderr, "A numerical inconsistency was discovered ");
    FPRINTF(stderr, "while calculating derivatives.");
    FPRINTF(stderr, "Integration will be terminated at the end of ");
    FPRINTF(stderr, "the current step.\n");
    return 0;
  }
  if (status.time_limit_exceeded) {
    FPRINTF(stderr, "The time limit was ");
    FPRINTF(stderr, "exceeded while calculating derivatives.\n");
    FPRINTF(stderr, "Integration will be terminated at ");
    FPRINTF(stderr, "the end of the current step.\n");
    return 0;
  }
  if (status.iteration_limit_exceeded) {
    FPRINTF(stderr, "The iteration limit was ");
    FPRINTF(stderr, "exceeded while calculating derivatives.\n");
    FPRINTF(stderr, "Integration will be terminated at ");
    FPRINTF(stderr, "the end of the current step.\n");
    return 0;
  }
  if (status.panic) {
    FPRINTF(stderr, "The user patience limit was ");
    FPRINTF(stderr, "exceeded while calculating derivatives.\n");
    FPRINTF(stderr, "Integration will be terminated at ");
    FPRINTF(stderr, "the end of the current step.\n");
    return 0;
  }
  return 0;
}
