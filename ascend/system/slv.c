/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
	Copyright (C) 1996 Benjamin Andrew Allan
	Copyright (C) 2005-2006 Carnegie-Mellon University

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <ascend/utilities/config.h>
#include <ascend/system/slv_client.h>
#include <ascend/system/slv_server.h>
#include <ascend/solver/solver.h>

#include <math.h>
#include <stdarg.h>

/** @TODO should not be ANY compiler includes here, right? */

#include <ascend/utilities/ascMalloc.h>
#include <ascend/utilities/ascPanic.h>

#include <ascend/compiler/atomvalue.h>

#include <ascend/linear/mtx.h>

#include <ascend/system/bndman.h>
#include <ascend/system/analyze.h>
#include <ascend/system/system_impl.h>

/* #define EMPTY_DEBUG */

#define NEEDSTOBEDONE 0

/**
 global variable used to communicate information between solvers and
 an interface, whether a calculation should be halted or not.
 0 means go on. any other value may contain additional information
 content.
*/
int Solv_C_CheckHalt_Flag = 0;

#if 0
/** making ANSI assumption that RegisteredClients is init to 0/NULLs */
static SlvFunctionsT SlvClientsData[SLVMAXCLIENTS];
#endif

/*-----------------------------------------------------------------*/
/**
	Note about g_number_of_whens, g_number_of_dvars and g_number_of_bnds:
	These numbers are as the same as those given in the solver and master
	lists, however, these lists are destroyed before the buffers are destroyed,
	so the information is gone before I can use it.
*/
/*
	These have been REMOVED and added to the 'sys' type.
*/

/*-------------------------------------------------------------------
	Convenience macros
*/

/** Return the solver index for a given slv_system_t */
#define SNUM(sys) ((sys)->solver)

/** Return the pointer to a registered SLV client's data space. @see SF, related.
	@param i registered solver ID
*/
//#define SCD(i) SlvClientsData[(i)]

/**	Get the solver index for a system and return TRUE if the solver
	index is in the range [0,NORC). 'sys' should not be null
	@param sys system, slv_system_t.

	== 'is a valid solver assigned?'
 */
//#define LS(sys) (sys->internals!=NULL))

/** Boolean test that i is in the range [0,NORC) 
	== 'is *i* a value solver?
*/
//#define LSI(i) (solver_engine(i)!=NULL)

/** Check and return a function pointer. See @SF 
	== get ptr to solver method 'ptr' on system
*/
//#define CF(sys,ptr) (sys->internals && sys-> LS(sys) ? (solver_engine((sys)->solver))->ptr : NULL )

/** Return the pointer to the client-supplied function or char if
	the client supplied one, else NULL. This should only be called
	with nonNULL sys after CF is happy. @see CF
*/
//#define SF(sys,ptr) ( (solver_engine((sys)->solver))->ptr )

/** Free a pointer provided it's not NULL */
#define SFUN(p) if ((p) != NULL)ASC_FREE(p)

/*-------------------------------------------------------------------------
  SERVER STUFF
*/
slv_system_t slv_create(void){
  slv_system_t sys;
  static unsigned nextid = 1;
  sys = (slv_system_t)asccalloc(1,sizeof(struct system_structure) );
  /* all lists, sizes, pointers DEFAULT to 0/NULL */
  sys->solver = -1; /* a nonregistration */
  sys->serial_id = nextid++;
  return(sys);
}

unsigned slv_serial_id(slv_system_t sys){
  return sys->serial_id;
}

/*---------------------------------------------------------------
	Macros to define
		slv_set_incidence
		slv_set_var_incidence
		slv_set_logincidence
*/

/* define but with error on null */
#define DEFINE_SET_INCIDENCE(NAME,PROP,TYPE,SIZE) \
	void slv_set_##NAME(slv_system_t sys, struct TYPE **inc, long s){ \
		if(sys->data.PROP != NULL){ \
			Asc_Panic(2,"slv_set_" #NAME,"bad call: sys->data." #PROP " is already defined!"); \
		}else if(inc == NULL){ \
			ERROR_REPORTER_HERE(ASC_PROG_ERROR,"bad call: 'inc' parameter is NULL"); \
			/*Asc_Panic(2,"slv_set_" #NAME,"bad call: 'inc' parameter is NULL!");*/ \
		}else{ \
			sys->data.PROP = inc; \
			sys->data.SIZE = s; \
		} \
	}

/* define, no error on null */
#define DEFINE_SET_INCIDENCE_NONULLERROR(NAME,PROP,TYPE,SIZE) \
	void slv_set_##NAME(slv_system_t sys, struct TYPE **inc, long s){ \
		if(sys->data.PROP != NULL){ \
			Asc_Panic(2,"slv_set_" #NAME,"bad call: sys->data." #PROP " is already defined!"); \
		}else{ \
			sys->data.PROP = inc; \
			sys->data.SIZE = s; \
		} \
	}


#define DEFINE_SET_INCIDENCES(D,D1) \
	D(incidence, incidence, var_variable, incsize) \
	D(var_incidence, varincidence, rel_relation, varincsize) \
	D1(logincidence, logincidence, dis_discrete, incsize)

DEFINE_SET_INCIDENCES(DEFINE_SET_INCIDENCE, DEFINE_SET_INCIDENCE_NONULLERROR)

/* see below for the use of this one */
#define SLV_FREE_INCIDENCE(NAME,PROP,TYPE,SIZE) \
    if (sys->data.PROP != NULL) ascfree(sys->data.PROP); \
    sys->data.PROP = NULL;

/*----------------------------------------------------
	destructors
*/

#define DEFINE_DESTROY_BUFFER(NAME,PROP,TYPE,DESTROY) \
	static void slv_destroy_##NAME##_buffer(slv_system_t sys){ \
		int c; struct TYPE *cur; \
		struct TYPE *buf; \
		buf = sys->PROP.buf; \
		for(c = 0; c < sys->PROP.bufnum; c++){ \
			cur = &(buf[c]); \
			DESTROY(cur); \
		} \
		ascfree(buf); \
		sys->PROP.buf = NULL; \
		sys->PROP.bufnum = 0; \
	}

#define DEFINE_DESTROY_BUFFERS(D) \
	D(dvar, dvars, dis_discrete, dis_destroy) \
	D(when, whens, w_when, when_destroy) \
	D(bnd, bnds, bnd_boundary, bnd_destroy)

DEFINE_DESTROY_BUFFERS(DEFINE_DESTROY_BUFFER)

#define SLV_FREE_BUF(PROP) \
	if(sys->PROP.buf !=NULL) ascfree(sys->PROP.buf); \
	sys->PROP.buf = NULL;

#define SLV_FREE_BUF_GLOBAL(NAME, PROP) \
	if (sys->PROP.buf != NULL) { \
		slv_destroy_##NAME##_buffer(sys); \
	}

#define SLV_FREE_BUFS(D,D_GLOBAL) \
	D(vars) \
	D(rels) \
	D(objs) \
	D(condrels) \
	D(logrels) \
	D(condlogrels) \
	D(pars) \
	D(unattached) \
	D(disunatt) \
	D_GLOBAL(dvar, dvars) \
	D_GLOBAL(when, whens) \
	D_GLOBAL(bnd, bnds)

int slv_destroy(slv_system_t sys)
{
  int ret = 0;
  if(sys->ct != NULL){
	asc_assert(sys->internals);
    if(sys->internals->cdestroy == NULL ) {
	  ERROR_REPORTER_HERE(ASC_PROG_FATAL,"slv_destroy: SlvClientToken 0x%p not freed by %s",
        sys->ct,sys->internals->name);
    } else {
      if((sys->internals->cdestroy)(sys,sys->ct)){
        ret++;
      }
    }
  }
  if (ret) {
	ERROR_REPORTER_HERE(ASC_PROG_FATAL,"slv_destroy: slv_system_t 0x%p not freed.",sys);
  } else {

	SLV_FREE_BUFS(SLV_FREE_BUF, SLV_FREE_BUF_GLOBAL)

	DEFINE_SET_INCIDENCES(SLV_FREE_INCIDENCE,SLV_FREE_INCIDENCE)

    ascfree( (POINTER)sys );
  }
  return ret;
}

/*---------------------------------------------------------------*/

void slv_destroy_client(slv_system_t sys)
{

  if (sys->ct != NULL) {
	asc_assert(sys->internals);
    if(sys->internals->cdestroy == NULL){
      ERROR_REPORTER_HERE(ASC_PROG_ERR,
		"SlvClientToken 0x%p not freed in slv_destroy_client",sys->ct);
    }else{
      if((sys->internals->cdestroy)(sys,sys->ct) ) {
        ERROR_REPORTER_HERE(ASC_PROG_ERR,"slv_destroy_client: SlvClientToken not freed");
      }else{
	sys->ct = NULL;
      }
    }
  }
}

/*---------------------------------------------------------
	get/set instance
*/

SlvBackendToken slv_instance(slv_system_t sys)
{
  if (sys == NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"slv_instance: called with NULL system.");
    return NULL;
  } else {
    return sys->instance;
  }
}

void slv_set_instance(slv_system_t sys,SlvBackendToken instance)
{
  if (sys == NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"slv_set_instance: called with NULL system.");
    return;
  } else {
    sys->instance = instance;
  }
}

dof_t *slv_get_dofdata(slv_system_t sys)
{
  return &(sys->dof);
}

dof_t *slv_get_log_dofdata(slv_system_t sys)
{
  return &(sys->logdof);
}

int32 slv_get_num_models(slv_system_t sys)
{
  if (sys == NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"slv_get_num_models: called with NULL system.");
    return 0;
  } else {
    return sys->nmodels;
  }
}
void slv_set_num_models(slv_system_t sys, int32 nmod)
{
  if (sys == NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"slv_set_num_models: called with NULL system.");
  } else {
    sys->nmodels = nmod;
  }
}


void slv_set_symbol_list(slv_system_t sys,
			 struct gl_list_t *sv)
{
  if (sys->symbollist != NULL) {
    DestroySymbolValuesList(sys->symbollist);
  }
  sys->symbollist = sv;
}

/*--------------------------------------------------------]
	Macros to declare

	slv_set_master_*_list(slv_system_t sys, string var_variable **list, int size)
	slv_set_*_buf(slv_system_t sys, string var_variable **list, int size)
*/

#define DEFINE_SET_MASTER_LIST_METHOD(NAME,PROP,TYPE) \
	void slv_set_master_##NAME##_list(slv_system_t sys, struct TYPE **vlist, int size){ \
		SFUN(sys->PROP.master); \
		sys->PROP.mnum = size; \
		sys->PROP.master = vlist; \
	}



#define DEFINE_SET_MASTER_LIST_METHODS(D) \
	D(var,vars,var_variable) \
	D(par,pars,var_variable) \
	D(unattached,unattached,var_variable) \
	D(dvar,dvars,dis_discrete) \
	D(disunatt,disunatt,dis_discrete) \
	D(rel,rels,rel_relation) \
	D(condrel,condrels,rel_relation) \
	D(obj,objs,rel_relation) \
	D(logrel,logrels,logrel_relation) \
	D(condlogrel,condlogrels,logrel_relation) \
	D(when,whens,w_when) \
	D(bnd,bnds,bnd_boundary)

DEFINE_SET_MASTER_LIST_METHODS(DEFINE_SET_MASTER_LIST_METHOD)

/*------------------------------------------------------------
	Macros to declare

	slv_set_NAME_buf(slv_system_t sts, struct TYPE *PROP)
*/

#define DEFINE_SET_BUF_METHOD(NAME,PROP,TYPE) \
	void slv_set_##NAME##_buf(slv_system_t sys, struct TYPE *PROP){ \
		if(sys->PROP.buf !=NULL ){ \
			Asc_Panic(2,"slv_set_" #NAME "_buf","bad call."); \
		}else{ \
			sys->PROP.buf = PROP; \
		} \
	}

#define DEFINE_SET_BUF_METHOD_GLOBAL(NAME,PROP,TYPE) \
	void slv_set_##NAME##_buf(slv_system_t sys, struct TYPE *buf, int len){ \
		if(sys->PROP.buf != NULL){ \
			Asc_Panic(2,"slv_set_" #NAME "_buf","bad call."); \
		}else{ \
			sys->PROP.buf = buf; \
			sys->PROP.bufnum = len; \
		} \
	}

#define DEFINE_SET_BUF_METHODS(D, D_GLOBAL) \
	D(var,vars,var_variable) \
	D(par,pars,var_variable) \
	D(unattached,unattached,var_variable) \
	D(disunatt,disunatt,dis_discrete) \
	D(rel,rels,rel_relation) \
	D(condrel,condrels,rel_relation) \
	D(obj,objs,rel_relation) \
	D(logrel,logrels,logrel_relation) \
	D(condlogrel,condlogrels,logrel_relation) \
	D_GLOBAL(dvar, dvars, dis_discrete) \
	D_GLOBAL(when, whens, w_when) \
	D_GLOBAL(bnd,bnds,bnd_boundary)


DEFINE_SET_BUF_METHODS(DEFINE_SET_BUF_METHOD, DEFINE_SET_BUF_METHOD_GLOBAL)

void slv_set_extrel_list(slv_system_t sys,struct ExtRelCache **erlist,
                         int size)
{
  if (sys->extrels.erlist !=NULL ) {
    Asc_Panic(2,"slv_set_extrel_list",
              "bad call.");
  }
  sys->extrels.num_extrels = size;
  sys->extrels.erlist = erlist;
}

struct ExtRelCache **slv_get_extrel_list(slv_system_t sys)
{
  return sys->extrels.erlist;
}

int slv_get_num_extrels(slv_system_t sys)
{
  return sys->extrels.num_extrels;
}


/*********************************************************************\
  client functions.
\*********************************************************************/
int Solv_C_CheckHalt()
{
  if (Solv_C_CheckHalt_Flag)
    return 1;
  else
    return 0;
}

const mtx_block_t *slv_get_solvers_blocks(slv_system_t sys)
{
  if (sys == NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_get_solvers_blocks called with NULL system");
    return NULL;
  } else {
    return &(sys->dof.blocks);
  }
}

const mtx_block_t *slv_get_solvers_log_blocks(slv_system_t sys)
{
  if (sys == NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_get_solvers_log_blocks called with NULL system");
    return NULL;
  } else {
    return &(sys->logdof.blocks);
  }
}

void slv_set_solvers_blocks(slv_system_t sys,int len, mtx_region_t *data)
{
  if (sys == NULL || len < 0) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_set_solvers_blocks called with NULL system or bad len.");
  } else {
    if (len && data==NULL) {
      ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_set_solvers_blocks called with bad data.");
    } else {
      if (sys->dof.blocks.nblocks && sys->dof.blocks.block != NULL) {
        ascfree(sys->dof.blocks.block);
      }
      sys->dof.blocks.block = data;
      sys->dof.blocks.nblocks = len;
    }
  }
}

void slv_set_solvers_log_blocks(slv_system_t sys,int len, mtx_region_t *data)
{
  if (sys == NULL || len < 0) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_set_solvers_log_blocks called with NULL system or bad len.");
  } else {
    if (len && data==NULL) {
      ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_set_solvers_log_blocks called with bad data.");
    } else {
      if (sys->logdof.blocks.nblocks && sys->logdof.blocks.block != NULL) {
        ascfree(sys->logdof.blocks.block);
      }
      sys->logdof.blocks.block = data;
      sys->logdof.blocks.nblocks = len;
    }
  }
}

void slv_check_var_initialization(slv_system_t sys){
  struct var_variable **vp;
  for (vp = slv_get_solvers_var_list(sys); *vp != NULL; vp++) {
    if (!AtomAssigned((struct Instance *)var_instance(*vp))) {
      var_set_value(*vp,var_nominal(*vp));
    }
  }
}

void slv_check_dvar_initialization(slv_system_t sys)
{
  struct dis_discrete **vp;

  for (vp = slv_get_solvers_dvar_list(sys); *vp != NULL; vp++) {
    if (!AtomAssigned((struct Instance *)dis_instance(*vp))) {
      dis_set_boolean_value(*vp,1);
    }
  }
}


void slv_bnd_initialization(slv_system_t sys)
{
  struct bnd_boundary **bp;
  int32 value;

  for (bp = slv_get_solvers_bnd_list(sys); *bp != NULL; bp++) {
    value =  bndman_calc_satisfied(*bp);
    bnd_set_cur_status(*bp,value);
    bnd_set_pre_status(*bp,value);
    bnd_set_crossed(*bp,FALSE);
    if (bnd_kind(*bp) == e_bnd_rel) {
      value = bndman_calc_at_zero(*bp);
      bnd_set_at_zero(*bp,value);
    } else {
      bnd_set_at_zero(*bp,FALSE);
    }
  }
}

struct gl_list_t *slv_get_symbol_list(slv_system_t sys)
{
  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_get_symbol_list called with NULL system.");
    return NULL;
  }
  return sys->symbollist;
}

/*---------------------------------------------------------
	Macros to define

		slv_set_solvers_*_list
		slv_get_solvers_*_list
		slv_get_master_*_list
*/
#define DEFINE_SET_SOLVERS_LIST_METHOD(NAME,PROP,TYPE) \
	void slv_set_solvers_##NAME##_list(slv_system_t sys, struct TYPE **vlist, int size){ \
		if(sys->PROP.master==NULL){ \
			ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"slv_set_solvers_" #NAME "_list: called before slv_set_master_" #NAME "_list."); \
			/* might be ok, no return */ \
		} \
		sys->PROP.snum = size; \
		sys->PROP.solver = vlist; \
	}

#define DEFINE_SET_SOLVERS_LIST_METHOD_RETURN(NAME,PROP,TYPE) \
	void slv_set_solvers_##NAME##_list(slv_system_t sys, struct TYPE **vlist, int size){ \
		if(sys->PROP.master==NULL){ \
			ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"slv_set_solvers_" #NAME "_list: called before slv_set_master_" #NAME "_list."); \
			return; /* can't be OK, so return now */ \
		} \
		sys->PROP.snum = size; \
		sys->PROP.solver = vlist; \
	}

#ifdef EMPTY_DEBUG
/* EW(SECT,NAME) makes an empty-list warning whenever a slv_get_*_*_list() method is called */
# define EW(SECT,NAME) \
		if (sys->PROP.solver == NULL) { \
			ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_get_" #SECT "_" #NAME "_list: returning NULL (?)."); \
		}
#else
/* ... unless we've silenced it */
# define EW(SECT,NAME)
#endif
#define DEFINE_GET_SOLVERS_LIST_METHOD(NAME,PROP,TYPE) \
	struct TYPE **slv_get_solvers_##NAME##_list(slv_system_t sys){ \
		EW(solvers,NAME) \
		return sys->PROP.solver; \
	}

#define DEFINE_GETSET_LIST_METHODS(D,D_RETURN) \
	D_RETURN(var,vars,var_variable) \
	D(par,pars,var_variable) \
	D(unattached,unattached,var_variable) \
	D_RETURN(dvar,dvars,dis_discrete) \
	D(disunatt,disunatt,dis_discrete) \
	D_RETURN(rel,rels,rel_relation) \
	D_RETURN(obj,objs,rel_relation) \
	D_RETURN(condrel,condrels,rel_relation) \
	D_RETURN(logrel,logrels,logrel_relation) \
	D_RETURN(condlogrel,condlogrels,logrel_relation) \
	D_RETURN(when,whens,w_when) \
	D_RETURN(bnd,bnds,bnd_boundary)

/* the slv_set_solvers_*_list methods: some have a 'return' when sys->PROP.master==NULL; others do not: */
DEFINE_GETSET_LIST_METHODS(DEFINE_SET_SOLVERS_LIST_METHOD, DEFINE_SET_SOLVERS_LIST_METHOD_RETURN) /*;*/

/* the slv_get_solvers_*_list methods: all have the same form so it's DEFINE...(D,D) in this case: */
DEFINE_GETSET_LIST_METHODS(DEFINE_GET_SOLVERS_LIST_METHOD, DEFINE_GET_SOLVERS_LIST_METHOD) /*;*/

#define DEFINE_GET_MASTER_LIST_METHOD(NAME,PROP,TYPE) \
	struct TYPE **slv_get_master_##NAME##_list(slv_system_t sys){ \
		EW(master,NAME) \
		return sys->PROP.master; \
	}

/* the slv_get_master_*_list are also all of the same form, so DEFINE...(D,D) */
DEFINE_GETSET_LIST_METHODS(DEFINE_GET_MASTER_LIST_METHOD,DEFINE_GET_MASTER_LIST_METHOD) /*;*/

#undef EW

/*----------------------------------------------------------------------
	Macros to define:

		slv_get_num_solvers_TYPE
		slv_get_num_master_TYPE
*/

#define DEFINE_SOLVERS_GET_NUM_METHOD(TYPE) \
	int slv_get_num_solvers_##TYPE(slv_system_t sys){ \
		if(sys==NULL){ \
			ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"slv_get_num_solvers_" #TYPE " called with NULL system."); \
			return 0; \
		} \
		return sys->TYPE.snum; \
	}

#define DEFINE_MASTER_GET_NUM_METHOD(TYPE) \
	int slv_get_num_master_##TYPE(slv_system_t sys){ \
		if(sys==NULL){ \
			ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"slv_get_num_master_" #TYPE " called with NULL system."); \
			return 0; \
		} \
		return sys->TYPE.mnum; \
	}

#define DEFINE_SLV_METHODS(D) \
	D(vars) \
	D(pars) \
	D(unattached) \
	D(dvars) \
	D(disunatt) \
	D(rels) \
	D(condrels) \
	D(objs) \
	D(logrels) \
	D(condlogrels) \
	D(whens) \
	D(bnds)

DEFINE_SLV_METHODS(DEFINE_SOLVERS_GET_NUM_METHOD) /*;*/
DEFINE_SLV_METHODS(DEFINE_MASTER_GET_NUM_METHOD) /*;*/

void slv_set_obj_relation(slv_system_t sys,struct rel_relation *obj)
{
  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_set_obj_relation called with NULL system (?).");
    return;
  }
  sys->obj = obj;
}

struct rel_relation *slv_get_obj_relation(slv_system_t sys)
{
  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_get_obj_relation called with NULL system (?)");
    return NULL;
  }
  return sys->obj;
}

void slv_set_obj_variable(slv_system_t sys,struct var_variable *objvar,
                          unsigned maximize)
{
  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_set_obj_variable called with NULL system.");
    return;
  }
  sys->objvar = objvar;
  if (objvar!=NULL) {
    if (maximize) {
      sys->objvargrad = -1;
    } else {
      sys->objvargrad = 1;
    }
  } else {
    sys->objvargrad = 0;
  }
}

struct var_variable *slv_get_obj_variable(slv_system_t sys)
{
  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_get_obj_variable called with NULL system.");
    return NULL;
  }
  return sys->objvar;
}

real64 slv_get_obj_variable_gradient(slv_system_t sys)
{
  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_get_obj_variable_gradient called with NULL system.");
    return 0.0;
  }
  return sys->objvargrad;
}


void slv_set_need_consistency(slv_system_t sys, int32 need_consistency)
{
  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_set_need_consistency called with NULL system.");
    return;
  }

  sys->need_consistency = need_consistency;
}


int32 slv_need_consistency(slv_system_t sys)
{
  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_need_consistency called with NULL system.");
    return 0;
  }
  return sys->need_consistency;
}

/*----------------------------------------------------------------
	Macros to define

		slv_count_vars
		          rels
		          dvars
		          logrels
		          whens
		          bnds
*/

#define DEFINE_SLV_COUNT_METHOD(NAME,FILTER,TYPE) \
	static int slv_count_##NAME(const FILTER##_filter_t *filter, struct TYPE **list){ \
		int ret=0; \
		asc_assert(list!=NULL); \
		/* CONSOLE_DEBUG("COUNTING " #NAME " WITH FILTER 0x%x -> 0x%x",filter->matchbits,filter->matchvalue); */ \
		while(*list!=NULL){ \
			ret += FILTER##_apply_filter(*list,filter); \
			list++; \
		} \
		/* CONSOLE_DEBUG("Returning %d",ret); */ \
		return ret; \
	}

#define DEFINE_SLV_COUNT_METHODS(D) \
	D(vars,var,var_variable) \
	D(rels,rel,rel_relation) \
	D(dvars,dis,dis_discrete) \
	D(logrels,logrel,logrel_relation) \
	D(whens,when,w_when) \
	D(bnds,bnd,bnd_boundary)

DEFINE_SLV_COUNT_METHODS(DEFINE_SLV_COUNT_METHOD) /*;*/

/*--------------------------------------------------------------
	Methods to define
		slv_count_solvers_*
		slv_count_master_*
*/

/** This macro automates the declaration of the slv_count_solvers_* methods */
#define DEFINE_SLV_COUNT_SOLVER_METHOD(NAME,PROP,TYPE,COUNT) \
	int slv_count_solvers_ ## NAME ( slv_system_t sys, const TYPE ##_filter_t *xxx){ \
		if(sys==NULL || sys->PROP.solver == NULL || xxx==NULL){ \
			ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_count_solvers_" #NAME " called with NULL"); \
			return 0; \
		} \
		/* CONSOLE_DEBUG("slv_count" #COUNT "(filter,sys->" #PROP ".solver)");*/ \
		return slv_count_##COUNT(xxx,sys->PROP.solver); \
	}

/** This macro automates the declaration of the slv_count_master_* methods */
#define DEFINE_SLV_COUNT_MASTER_METHOD(NAME,PROP,TYPE,COUNT) \
	int slv_count_master_ ## NAME ( slv_system_t sys, const TYPE ##_filter_t *xxx){ \
		if(sys==NULL || sys->PROP.master == NULL || xxx==NULL){ \
			ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_count_master_" #NAME " called with NULL"); \
			return 0; \
		} \
		return slv_count_##COUNT(xxx,sys->PROP.master); \
	}

/** The macro makes all the various * declarations of the methods of type D (master or solvers) */
#define DEFINE_COUNT_METHODS(D) \
	D(vars,vars,var,vars) \
	D(pars,pars,var,vars) \
	D(unattached,unattached,var,vars) \
	D(dvars,dvars,dis,dvars) \
	D(disunatt,disunatt,dis,dvars) \
	D(rels,rels,rel,rels) \
	D(condrels,condrels,rel,rels) \
	D(objs,objs,rel,rels) \
	D(logrels,logrels,logrel,logrels) \
	D(condlogrels,condlogrels,logrel,logrels) \
	D(whens,whens,when,whens) \
	D(bnds,bnds,bnd,bnds)

/** Invoke the DEFINE_COUNT_METHODS macro for SOLVERS methods */
DEFINE_COUNT_METHODS(DEFINE_SLV_COUNT_SOLVER_METHOD) /*;*/
/** Invoke the DEFINE_COUNT_METHODS macro for MASTER methods */
DEFINE_COUNT_METHODS(DEFINE_SLV_COUNT_MASTER_METHOD) /*;*/


/*********************************************************************\
  unregistered client functions that need to go elsewhere(other files).
  hereunder are utility calls which are unstandardized
\*********************************************************************/

boolean slv_change_basis(slv_system_t sys, int32 var, mtx_range_t *rng)
{
  (void)sys;
  (void)var;
  (void)rng;
  ASC_PANIC("fix me");
  return 0;
}

/*
 * This routine is provided as the start of some report generation
 * capabilities. It operates off the main solve system and
 * writes out the relation residuals and variable values for
 * the entire problem to the named file.
 * Isn't very bright.
 */

void slv_print_output(FILE *out, slv_system_t sys)
{
  struct rel_relation **rp;
  struct var_variable **vp;
  int nrels, nvars,c;

  vp = slv_get_master_var_list(sys);
  nvars = slv_get_num_master_vars(sys);
  FPRINTF(out,"%-6s %-12s\n",
	  "INDEX","LEVEL");
  for (c=0; c<nvars; c++) {
    FPRINTF(out,"  % -6d  % -12.8e\n",c, var_value(vp[c]));
  }
  PUTC('\n',out);

  rp = slv_get_master_rel_list(sys);
  nrels = slv_get_num_master_rels(sys);
  FPRINTF(out,"%-6s %-12s\n",
	  "INDEX","RESDUAL");
  for (c=0; c<nrels; c++) {
    FPRINTF(out,"  % -6d  % -12.8e\n",c, rel_residual(rp[c]));
  }
}

int32 slv_obj_select_list(slv_system_t sys,int32 **rip)
{
  int32 len,count,i, *ra;
  static rel_filter_t rfilter;
  struct rel_relation **rlist=NULL;
  len = slv_get_num_solvers_objs(sys);
  ra = *rip = (int32 *)ascmalloc((len+1)*sizeof(int32 *));
  rfilter.matchbits = (REL_INCLUDED);
  rfilter.matchvalue =(REL_INCLUDED);
  rlist = slv_get_solvers_obj_list(sys);
  count = 0;
  for (i = 0; i < len; i++) {
    if (rel_apply_filter(rlist[i],&rfilter)) {
      ra[count] = i;
      count++;
    }
  }
  ra[count] = -1;
  return count;
}

int32 slv_get_obj_num(slv_system_t sys)
{
  int32 len,i;
  struct rel_relation *obj;
  struct rel_relation **rlist=NULL;
  len = slv_get_num_solvers_objs(sys);
  rlist = slv_get_solvers_obj_list(sys);
  obj = slv_get_obj_relation(sys);
  if (obj != NULL) {
    for (i = 0; i < len; i++) {
      if (rlist[i] == obj) {
	return i;
      }
    }
  }
  return -1;
}

int32 slv_near_bounds(slv_system_t sys,real64 epsilon,
		       int32 **vip)
{
  int32 len,i, *va, vindex;
  real64 comp;
  static var_filter_t vfilter;
  struct var_variable **vlist=NULL;
  len = slv_get_num_solvers_vars(sys);
  va = *vip = (int32 *)ascmalloc((2*len+2)*sizeof(int32 *));
  vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
  vlist = slv_get_solvers_var_list(sys);
  va[0] = va[1] = 0;
  vindex = 2;
  for (i = 0; i < len; i++) {
    if (var_apply_filter(vlist[i],&vfilter)) {
      comp = (var_value(vlist[i]) - var_lower_bound(vlist[i]))
	/ var_nominal(vlist[i]);
      if (comp < epsilon) {
	va[vindex] = i;
	vindex++;
	va[0]++;
      }
    }
  }
  for (i = 0; i < len; i++) {
    if (var_apply_filter(vlist[i],&vfilter)) {
      comp = (var_upper_bound(vlist[i]) - var_value(vlist[i]))
	/ var_nominal(vlist[i]);
      if (comp < epsilon) {
	va[vindex] = i;
	vindex++;
	va[1]++;
      }
    }
  }
  return vindex - 2;
}

int32 slv_far_from_nominals(slv_system_t sys,real64 bignum,
		       int32 **vip)
{
  int32 len,i, *va, vindex;
  real64 comp;
  static var_filter_t vfilter;
  struct var_variable **vlist=NULL;
  len = slv_get_num_solvers_vars(sys);
  va = *vip = (int32 *)ascmalloc((len+1)*sizeof(int32 *));
  vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
  vlist = slv_get_solvers_var_list(sys);
  vindex = 0;
  for (i = 0; i < len; i++) {
    if (var_apply_filter(vlist[i],&vfilter)) {
      comp = fabs(var_value(vlist[i]) - var_nominal(vlist[i]))
	/ var_nominal(vlist[i]);
      if (comp > bignum) {
	va[vindex] = i;
	vindex++;
      }
    }
  }
  return vindex;
}

const void *slv_get_diffvars(slv_system_t sys){
	return sys->diffvars;
}

int slv_set_diffvars(slv_system_t sys,void *diffvars){
	sys->diffvars = diffvars;
	return 0;
}

