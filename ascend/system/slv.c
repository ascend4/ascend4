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

#include <ascend/general/ascMalloc.h>
#include <ascend/general/panic.h>

#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/createinst.h>
#include <ascend/compiler/derivinst.h>
#include <ascend/compiler/destroyinst.h>
#include <ascend/compiler/exprs.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/logrelation.h>
#include <ascend/compiler/logrel_util.h>
#include <ascend/compiler/mathinst.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/relation.h>
#include <ascend/compiler/relation_util.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/when_util.h>

#include <ascend/linear/mtx.h>

#include <ascend/system/bndman.h>
#include <ascend/system/analyze.h>
#include <ascend/system/cond_config.h>
#include <ascend/system/system_impl.h>
#include <ascend/system/lsq.h>

/* #define EMPTY_DEBUG */

#define NEEDSTOBEDONE 0

#ifndef IPTR
#define IPTR(i) ((struct Instance *)(i))
#endif

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

	slv_destroy_classifier_artifacts(sys);

	if(sys->hidden_instances != NULL){
		unsigned long i, len = gl_length(sys->hidden_instances);
		for(i = 1; i <= len; ++i){
			struct Instance *inst = (struct Instance *)gl_fetch(sys->hidden_instances, i);
			if(inst != NULL){
				SetInterfacePtr(inst,NULL);
			}
		}
		gl_destroy(sys->hidden_instances);
		sys->hidden_instances = NULL;
	}

	system_clear_lsq_view(sys);
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

void slv_set_hidden_instance_list(slv_system_t sys,
			 struct gl_list_t *sv)
{
  if (sys->hidden_instances != NULL) {
    Asc_Panic(2,"slv_set_hidden_instance_list","bad call: sys->hidden_instances is already defined!");
  }
  sys->hidden_instances = sv;
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
	ASC_DLLSPEC void slv_set_solvers_##NAME##_list(slv_system_t sys, struct TYPE **vlist, int size){ \
		if(sys->PROP.master==NULL){ \
			ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"slv_set_solvers_" #NAME "_list: called before slv_set_master_" #NAME "_list."); \
			/* might be ok, no return */ \
		} \
		sys->PROP.snum = size; \
		sys->PROP.solver = vlist; \
	}

#define DEFINE_SET_SOLVERS_LIST_METHOD_RETURN(NAME,PROP,TYPE) \
	ASC_DLLSPEC void slv_set_solvers_##NAME##_list(slv_system_t sys, struct TYPE **vlist, int size){ \
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

enum classifier_artifact_kind {
  CLASSIFIER_ARTIFACT_REL,
  CLASSIFIER_ARTIFACT_LOGREL
};

struct classifier_artifact {
  enum classifier_artifact_kind kind;
  symchar *name;
  struct Instance *inst;
  struct rel_relation *rel;
  struct logrel_relation *logrel;
  struct bnd_boundary *bnd;
};

struct classifier_name_resolver {
  slv_system_t sys;
};

struct expr_fragment {
  const struct Expr *source_start;
  const struct Expr *source_end;
  struct Expr *head;
  struct Expr *tail;
  struct classifier_artifact *generated_boundary;
};

static void classifier_artifact_free(struct classifier_artifact *artifact)
{
  if (artifact == NULL) {
    return;
  }
  if (artifact->bnd != NULL) {
    bnd_destroy(artifact->bnd);
    ascfree(artifact->bnd);
    artifact->bnd = NULL;
  }
  if (artifact->logrel != NULL) {
    logrel_destroy(artifact->logrel);
    artifact->logrel = NULL;
  }
  if (artifact->rel != NULL) {
    rel_destroy(artifact->rel);
    artifact->rel = NULL;
  }
  if (artifact->inst != NULL) {
    DestroyInstance(artifact->inst,NULL);
    artifact->inst = NULL;
  }
  ascfree(artifact);
}

void slv_destroy_classifier_artifacts(slv_system_t sys)
{
  unsigned long len;

  if (sys == NULL || sys->classifier_artifacts == NULL) {
    return;
  }

  len = gl_length(sys->classifier_artifacts);
  while (len > 0) {
    classifier_artifact_free(
      (struct classifier_artifact *)gl_fetch(sys->classifier_artifacts,len)
    );
    --len;
  }
  gl_destroy(sys->classifier_artifacts);
  sys->classifier_artifacts = NULL;
  sys->classifier_artifacts_installed = 0;
}

static struct Instance *classifier_resolve_name(CONST struct Name *name,
    void *userdata)
{
  struct classifier_name_resolver *resolver;
  symchar *id;
  unsigned long i, len;

  resolver = (struct classifier_name_resolver *)userdata;
  id = SimpleNameIdPtr(name);
  if (resolver == NULL || resolver->sys == NULL
      || resolver->sys->classifier_artifacts == NULL || id == NULL) {
    return NULL;
  }

  len = gl_length(resolver->sys->classifier_artifacts);
  for (i = 1; i <= len; ++i) {
    struct classifier_artifact *artifact =
      (struct classifier_artifact *)gl_fetch(
          resolver->sys->classifier_artifacts,i);
    if (artifact != NULL && artifact->name == id) {
      return artifact->inst;
    }
  }
  return NULL;
}

static struct classifier_artifact *classifier_artifact_fetch_kind(
    slv_system_t sys, enum classifier_artifact_kind kind, int32 index)
{
  unsigned long i, len;
  int32 count = 0;

  if (sys == NULL || sys->classifier_artifacts == NULL || index < 0) {
    return NULL;
  }

  len = gl_length(sys->classifier_artifacts);
  for (i = 1; i <= len; ++i) {
    struct classifier_artifact *artifact =
      (struct classifier_artifact *)gl_fetch(sys->classifier_artifacts,i);
    if (artifact != NULL && artifact->kind == kind) {
      if (count == index) {
        return artifact;
      }
      ++count;
    }
  }
  return NULL;
}

static int32 classifier_artifact_count_kind(slv_system_t sys,
    enum classifier_artifact_kind kind)
{
  unsigned long i, len;
  int32 count = 0;

  if (sys == NULL || sys->classifier_artifacts == NULL) {
    return 0;
  }

  len = gl_length(sys->classifier_artifacts);
  for (i = 1; i <= len; ++i) {
    struct classifier_artifact *artifact =
      (struct classifier_artifact *)gl_fetch(sys->classifier_artifacts,i);
    if (artifact != NULL && artifact->kind == kind) {
      ++count;
    }
  }
  return count;
}

int32 slv_get_num_classifier_rels(slv_system_t sys)
{
  return classifier_artifact_count_kind(sys,CLASSIFIER_ARTIFACT_REL);
}

int32 slv_get_num_classifier_logrels(slv_system_t sys)
{
  return classifier_artifact_count_kind(sys,CLASSIFIER_ARTIFACT_LOGREL);
}

int32 slv_get_num_classifier_bnds(slv_system_t sys)
{
  if (sys == NULL || sys->classifier_artifacts == NULL) {
    return 0;
  }
  return (int32)gl_length(sys->classifier_artifacts);
}

struct rel_relation *slv_get_classifier_rel(slv_system_t sys, int32 index)
{
  struct classifier_artifact *artifact =
    classifier_artifact_fetch_kind(sys,CLASSIFIER_ARTIFACT_REL,index);
  return artifact != NULL ? artifact->rel : NULL;
}

struct logrel_relation *slv_get_classifier_logrel(slv_system_t sys,
    int32 index)
{
  struct classifier_artifact *artifact =
    classifier_artifact_fetch_kind(sys,CLASSIFIER_ARTIFACT_LOGREL,index);
  return artifact != NULL ? artifact->logrel : NULL;
}

struct bnd_boundary *slv_get_classifier_bnd(slv_system_t sys, int32 index)
{
  struct classifier_artifact *artifact;

  if (sys == NULL || sys->classifier_artifacts == NULL || index < 0
      || index >= (int32)gl_length(sys->classifier_artifacts)) {
    return NULL;
  }

  artifact = (struct classifier_artifact *)gl_fetch(
      sys->classifier_artifacts,(unsigned long)index + 1);
  return artifact != NULL ? artifact->bnd : NULL;
}

static struct classifier_artifact *classifier_artifact_find_inst(
    slv_system_t sys, struct Instance *inst)
{
  unsigned long i, len;

  if (sys == NULL || sys->classifier_artifacts == NULL || inst == NULL) {
    return NULL;
  }

  len = gl_length(sys->classifier_artifacts);
  for (i = 1; i <= len; ++i) {
    struct classifier_artifact *artifact =
      (struct classifier_artifact *)gl_fetch(sys->classifier_artifacts,i);
    if (artifact != NULL && artifact->inst == inst) {
      return artifact;
    }
  }
  return NULL;
}

static struct var_variable *classifier_find_var(slv_system_t sys,
    struct Instance *inst)
{
  struct var_variable **lists[3];
  int l;

  if (sys == NULL || inst == NULL) {
    return NULL;
  }

  lists[0] = slv_get_master_var_list(sys);
  lists[1] = slv_get_master_par_list(sys);
  lists[2] = slv_get_master_unattached_list(sys);

  for (l = 0; l < 3; ++l) {
    struct var_variable **vp = lists[l];
    if (vp == NULL) {
      continue;
    }
    while (*vp != NULL) {
      if (var_instance(*vp) == inst) {
        return *vp;
      }
      ++vp;
    }
  }
  return NULL;
}

static struct dis_discrete *classifier_find_discrete(slv_system_t sys,
    struct Instance *inst)
{
  struct dis_discrete **lists[2];
  int l;

  if (sys == NULL || inst == NULL) {
    return NULL;
  }

  lists[0] = slv_get_master_dvar_list(sys);
  lists[1] = slv_get_master_disunatt_list(sys);

  for (l = 0; l < 2; ++l) {
    struct dis_discrete **dp = lists[l];
    if (dp == NULL) {
      continue;
    }
    while (*dp != NULL) {
      if (dis_instance(*dp) == inst) {
        return *dp;
      }
      ++dp;
    }
  }
  return NULL;
}

static int classifier_set_relation_incidence(slv_system_t sys,
    struct classifier_artifact *artifact)
{
  CONST struct relation *reln;
  struct var_variable **incidence;
  unsigned long i, len;

  if (sys == NULL || artifact == NULL || artifact->rel == NULL
      || artifact->inst == NULL) {
    return 1;
  }

  reln = GetInstanceRelationOnly(artifact->inst);
  if (reln == NULL) {
    return 1;
  }

  len = NumberVariables(reln);
  incidence = len > 0 ? ASC_NEW_ARRAY(struct var_variable *,len) : NULL;
  if (len > 0 && incidence == NULL) {
    return 1;
  }

  for (i = 0; i < len; ++i) {
    struct Instance *varinst = RelationVariable(reln,i + 1);
    struct var_variable *var = classifier_find_var(sys,varinst);
    if (var == NULL) {
      if (incidence != NULL) {
        ascfree(incidence);
      }
      return 1;
    }
    incidence[i] = var;
  }

  rel_set_incidences(artifact->rel,(int32)len,incidence);
  return 0;
}

static int classifier_set_logrel_incidence(slv_system_t sys,
    struct classifier_artifact *artifact)
{
  CONST struct logrelation *lreln;
  struct dis_discrete **incidence;
  unsigned long i, len;

  if (sys == NULL || artifact == NULL || artifact->logrel == NULL
      || artifact->inst == NULL) {
    return 1;
  }

  lreln = GetInstanceLogRelOnly(artifact->inst);
  if (lreln == NULL) {
    return 1;
  }

  len = NumberBoolVars(lreln);
  incidence = len > 0 ? ASC_NEW_ARRAY(struct dis_discrete *,len) : NULL;
  if (len > 0 && incidence == NULL) {
    return 1;
  }

  for (i = 0; i < len; ++i) {
    struct Instance *disinst = LogRelBoolVar(lreln,i + 1);
    struct dis_discrete *dvar = classifier_find_discrete(sys,disinst);
    if (dvar == NULL) {
      if (incidence != NULL) {
        ascfree(incidence);
      }
      return 1;
    }
    incidence[i] = dvar;
  }

  logrel_set_incidences(artifact->logrel,(int32)len,incidence);
  return 0;
}

static int classifier_link_satisfied_boundary(slv_system_t sys,
    struct classifier_artifact *target, struct classifier_artifact *user)
{
  struct gl_list_t *logrels;
  unsigned long i, len;

  if (target == NULL || target->bnd == NULL || user == NULL
      || user->kind != CLASSIFIER_ARTIFACT_LOGREL || user->logrel == NULL) {
    return 0;
  }

  logrels = bnd_logrels(target->bnd);
  if (logrels == NULL) {
    logrels = gl_create(1);
    if (logrels == NULL) {
      return 1;
    }
    bnd_set_logrels(target->bnd,logrels);
  }

  len = gl_length(logrels);
  for (i = 1; i <= len; ++i) {
    if ((struct logrel_relation *)gl_fetch(logrels,i) == user->logrel) {
      bnd_set_flags(target->bnd,bnd_flags(target->bnd) | BND_IN_LOGREL);
      (void)sys;
      return 0;
    }
  }
  gl_append_ptr(logrels,user->logrel);
  bnd_set_flags(target->bnd,bnd_flags(target->bnd) | BND_IN_LOGREL);
  (void)sys;
  return 0;
}

static int classifier_set_boundary_logrels(slv_system_t sys,
    struct classifier_artifact *artifact)
{
  CONST struct logrelation *lreln;
  unsigned long i, len;

  if (sys == NULL || artifact == NULL
      || artifact->kind != CLASSIFIER_ARTIFACT_LOGREL
      || artifact->inst == NULL) {
    return 0;
  }

  lreln = GetInstanceLogRelOnly(artifact->inst);
  if (lreln == NULL) {
    return 1;
  }

  len = NumberRelations(lreln);
  for (i = 1; i <= len; ++i) {
    struct classifier_artifact *target =
      classifier_artifact_find_inst(sys,LogRelRelation(lreln,i));
    if (target != NULL && classifier_link_satisfied_boundary(sys,target,artifact)) {
      return 1;
    }
  }
  return 0;
}

static int classifier_finalize_artifacts(slv_system_t sys)
{
  unsigned long i, len;

  if (sys == NULL || sys->classifier_artifacts == NULL) {
    return 0;
  }

  len = gl_length(sys->classifier_artifacts);
  for (i = 1; i <= len; ++i) {
    struct classifier_artifact *artifact =
      (struct classifier_artifact *)gl_fetch(sys->classifier_artifacts,i);
    if (artifact == NULL) {
      return 1;
    }
    switch (artifact->kind) {
    case CLASSIFIER_ARTIFACT_REL:
      if (classifier_set_relation_incidence(sys,artifact)) {
        return 1;
      }
      break;
    case CLASSIFIER_ARTIFACT_LOGREL:
      if (classifier_set_logrel_incidence(sys,artifact)) {
        return 1;
      }
      break;
    default:
      return 1;
    }
  }

  for (i = 1; i <= len; ++i) {
    struct classifier_artifact *artifact =
      (struct classifier_artifact *)gl_fetch(sys->classifier_artifacts,i);
    if (classifier_set_boundary_logrels(sys,artifact)) {
      return 1;
    }
  }

  return 0;
}

static int classifier_alloc_rel_lists(struct rel_relation ***master_new,
    struct rel_relation ***solver_new, int32 nmaster, int32 nsolver,
    int32 nadd)
{
  *master_new = NULL;
  *solver_new = NULL;
  if (nadd <= 0) {
    return 0;
  }
  *master_new = ASC_NEW_ARRAY(struct rel_relation *,nmaster + nadd + 1);
  *solver_new = ASC_NEW_ARRAY(struct rel_relation *,nsolver + nadd + 1);
  if (*master_new == NULL || *solver_new == NULL) {
    if (*master_new != NULL) {
      ascfree(*master_new);
      *master_new = NULL;
    }
    if (*solver_new != NULL) {
      ascfree(*solver_new);
      *solver_new = NULL;
    }
    return 1;
  }
  return 0;
}

static int classifier_alloc_logrel_lists(struct logrel_relation ***master_new,
    struct logrel_relation ***solver_new, int32 nmaster, int32 nsolver,
    int32 nadd)
{
  *master_new = NULL;
  *solver_new = NULL;
  if (nadd <= 0) {
    return 0;
  }
  *master_new = ASC_NEW_ARRAY(struct logrel_relation *,nmaster + nadd + 1);
  *solver_new = ASC_NEW_ARRAY(struct logrel_relation *,nsolver + nadd + 1);
  if (*master_new == NULL || *solver_new == NULL) {
    if (*master_new != NULL) {
      ascfree(*master_new);
      *master_new = NULL;
    }
    if (*solver_new != NULL) {
      ascfree(*solver_new);
      *solver_new = NULL;
    }
    return 1;
  }
  return 0;
}

static int classifier_alloc_bnd_lists(struct bnd_boundary ***master_new,
    struct bnd_boundary ***solver_new, int32 nmaster, int32 nsolver,
    int32 nadd)
{
  *master_new = NULL;
  *solver_new = NULL;
  if (nadd <= 0) {
    return 0;
  }
  *master_new = ASC_NEW_ARRAY(struct bnd_boundary *,nmaster + nadd + 1);
  *solver_new = ASC_NEW_ARRAY(struct bnd_boundary *,nsolver + nadd + 1);
  if (*master_new == NULL || *solver_new == NULL) {
    if (*master_new != NULL) {
      ascfree(*master_new);
      *master_new = NULL;
    }
    if (*solver_new != NULL) {
      ascfree(*solver_new);
      *solver_new = NULL;
    }
    return 1;
  }
  return 0;
}

static int classifier_install_artifact_lists(slv_system_t sys)
{
  struct rel_relation **old_master_condrels, **old_solver_condrels;
  struct logrel_relation **old_master_logrels, **old_solver_logrels;
  struct bnd_boundary **old_master_bnds, **old_solver_bnds;
  struct rel_relation **new_master_condrels = NULL;
  struct rel_relation **new_solver_condrels = NULL;
  struct logrel_relation **new_master_logrels = NULL;
  struct logrel_relation **new_solver_logrels = NULL;
  struct bnd_boundary **new_master_bnds = NULL;
  struct bnd_boundary **new_solver_bnds = NULL;
  int32 nmaster_condrels, nsolver_condrels;
  int32 nmaster_logrels, nsolver_logrels;
  int32 nmaster_bnds, nsolver_bnds;
  int32 nrels, nlogrels, nbnds;
  unsigned long i, len;
  int32 cr = 0, clr = 0, cb = 0;

  if (sys == NULL || sys->classifier_artifacts == NULL) {
    return 0;
  }
  if (sys->classifier_artifacts_installed) {
    return 0;
  }

  nrels = slv_get_num_classifier_rels(sys);
  nlogrels = slv_get_num_classifier_logrels(sys);
  nbnds = slv_get_num_classifier_bnds(sys);
  if (nrels == 0 && nlogrels == 0 && nbnds == 0) {
    sys->classifier_artifacts_installed = 1;
    return 0;
  }

  old_master_condrels = slv_get_master_condrel_list(sys);
  old_solver_condrels = slv_get_solvers_condrel_list(sys);
  old_master_logrels = slv_get_master_logrel_list(sys);
  old_solver_logrels = slv_get_solvers_logrel_list(sys);
  old_master_bnds = slv_get_master_bnd_list(sys);
  old_solver_bnds = slv_get_solvers_bnd_list(sys);
  nmaster_condrels = slv_get_num_master_condrels(sys);
  nsolver_condrels = slv_get_num_solvers_condrels(sys);
  nmaster_logrels = slv_get_num_master_logrels(sys);
  nsolver_logrels = slv_get_num_solvers_logrels(sys);
  nmaster_bnds = slv_get_num_master_bnds(sys);
  nsolver_bnds = slv_get_num_solvers_bnds(sys);

  if (classifier_alloc_rel_lists(&new_master_condrels,&new_solver_condrels,
      nmaster_condrels,nsolver_condrels,nrels)
      || classifier_alloc_logrel_lists(&new_master_logrels,
        &new_solver_logrels,nmaster_logrels,nsolver_logrels,
        nlogrels)
      || classifier_alloc_bnd_lists(&new_master_bnds,&new_solver_bnds,
        nmaster_bnds,nsolver_bnds,nbnds)) {
    if (new_master_condrels != NULL) ascfree(new_master_condrels);
    if (new_solver_condrels != NULL) ascfree(new_solver_condrels);
    if (new_master_logrels != NULL) ascfree(new_master_logrels);
    if (new_solver_logrels != NULL) ascfree(new_solver_logrels);
    if (new_master_bnds != NULL) ascfree(new_master_bnds);
    if (new_solver_bnds != NULL) ascfree(new_solver_bnds);
    return 1;
  }

  if (new_master_condrels != NULL) {
    for (i = 0; i < (unsigned long)nmaster_condrels; ++i) {
      new_master_condrels[i] = old_master_condrels[i];
    }
    for (i = 0; i < (unsigned long)nsolver_condrels; ++i) {
      new_solver_condrels[i] = old_solver_condrels[i];
    }
  }
  if (new_master_logrels != NULL) {
    for (i = 0; i < (unsigned long)nmaster_logrels; ++i) {
      new_master_logrels[i] = old_master_logrels[i];
    }
    for (i = 0; i < (unsigned long)nsolver_logrels; ++i) {
      new_solver_logrels[i] = old_solver_logrels[i];
    }
  }
  if (new_master_bnds != NULL) {
    for (i = 0; i < (unsigned long)nmaster_bnds; ++i) {
      new_master_bnds[i] = old_master_bnds[i];
    }
    for (i = 0; i < (unsigned long)nsolver_bnds; ++i) {
      new_solver_bnds[i] = old_solver_bnds[i];
    }
  }

  len = gl_length(sys->classifier_artifacts);
  for (i = 1; i <= len; ++i) {
    struct classifier_artifact *artifact =
      (struct classifier_artifact *)gl_fetch(sys->classifier_artifacts,i);
    if (artifact == NULL) {
      continue;
    }
    if (artifact->rel != NULL) {
      rel_set_mindex(artifact->rel,nmaster_condrels + cr);
      rel_set_sindex(artifact->rel,nsolver_condrels + cr);
      rel_set_model(artifact->rel,0);
      new_master_condrels[nmaster_condrels + cr] = artifact->rel;
      new_solver_condrels[nsolver_condrels + cr] = artifact->rel;
      ++cr;
    }
    if (artifact->logrel != NULL) {
      logrel_set_mindex(artifact->logrel,nmaster_logrels + clr);
      logrel_set_sindex(artifact->logrel,nsolver_logrels + clr);
      logrel_set_model(artifact->logrel,0);
      new_master_logrels[nmaster_logrels + clr] = artifact->logrel;
      new_solver_logrels[nsolver_logrels + clr] = artifact->logrel;
      ++clr;
    }
    if (artifact->bnd != NULL) {
      bnd_set_mindex(artifact->bnd,nmaster_bnds + cb);
      bnd_set_sindex(artifact->bnd,nsolver_bnds + cb);
      bnd_set_model(artifact->bnd,0);
      new_master_bnds[nmaster_bnds + cb] = artifact->bnd;
      new_solver_bnds[nsolver_bnds + cb] = artifact->bnd;
      ++cb;
    }
  }

  if (new_master_condrels != NULL) {
    new_master_condrels[nmaster_condrels + nrels] = NULL;
    new_solver_condrels[nsolver_condrels + nrels] = NULL;
    slv_set_master_condrel_list(sys,new_master_condrels,
        nmaster_condrels + nrels);
    if (old_solver_condrels != NULL
        && old_solver_condrels != old_master_condrels) {
      ascfree(old_solver_condrels);
    }
    slv_set_solvers_condrel_list(sys,new_solver_condrels,
        nsolver_condrels + nrels);
  }
  if (new_master_logrels != NULL) {
    new_master_logrels[nmaster_logrels + nlogrels] = NULL;
    new_solver_logrels[nsolver_logrels + nlogrels] = NULL;
    slv_set_master_logrel_list(sys,new_master_logrels,
        nmaster_logrels + nlogrels);
    if (old_solver_logrels != NULL
        && old_solver_logrels != old_master_logrels) {
      ascfree(old_solver_logrels);
    }
    slv_set_solvers_logrel_list(sys,new_solver_logrels,
        nsolver_logrels + nlogrels);
  }
  if (new_master_bnds != NULL) {
    new_master_bnds[nmaster_bnds + nbnds] = NULL;
    new_solver_bnds[nsolver_bnds + nbnds] = NULL;
    slv_set_master_bnd_list(sys,new_master_bnds,nmaster_bnds + nbnds);
    if (old_solver_bnds != NULL && old_solver_bnds != old_master_bnds) {
      ascfree(old_solver_bnds);
    }
    slv_set_solvers_bnd_list(sys,new_solver_bnds,nsolver_bnds + nbnds);
  }

  sys->classifier_artifacts_installed = 1;
  return 0;
}

static struct Expr *copy_expr_range(const struct Expr *start,
    const struct Expr *end)
{
  struct Expr *copy;
  struct Expr *saved_next;

  if (start == NULL || end == NULL) {
    return NULL;
  }

  saved_next = ((struct Expr *)end)->next;
  ((struct Expr *)end)->next = NULL;
  copy = CopyExprList(start);
  ((struct Expr *)end)->next = saved_next;
  return copy;
}

static void expr_fragment_destroy(struct expr_fragment *fragment)
{
  if (fragment != NULL && fragment->head != NULL) {
    DestroyExprList(fragment->head);
    fragment->head = NULL;
    fragment->tail = NULL;
  }
}

static int expr_fragment_append(struct expr_fragment *fragment,
    struct Expr *expr)
{
  struct Expr *tail;

  if (fragment == NULL || expr == NULL) {
    return 1;
  }

  tail = expr;
  while (NextExpr(tail) != NULL) {
    tail = NextExpr(tail);
  }

  if (fragment->head == NULL) {
    fragment->head = expr;
  } else {
    LinkExprs(fragment->tail,expr);
  }
  fragment->tail = tail;
  return 0;
}

static int expr_fragment_ensure_bool_copy(struct expr_fragment *fragment)
{
  if (fragment == NULL) {
    return 1;
  }
  if (fragment->head != NULL) {
    return 0;
  }
  fragment->head = copy_expr_range(fragment->source_start,
                                   fragment->source_end);
  if (fragment->head == NULL) {
    return 1;
  }
  fragment->tail = fragment->head;
  while (NextExpr(fragment->tail) != NULL) {
    fragment->tail = NextExpr(fragment->tail);
  }
  return 0;
}

static int expr_fragment_push(struct expr_fragment *stack, int stack_size,
    int *sp, struct expr_fragment fragment)
{
  if (stack == NULL || sp == NULL || *sp >= stack_size) {
    expr_fragment_destroy(&fragment);
    return 1;
  }
  stack[(*sp)++] = fragment;
  return 0;
}

static int expr_fragment_pop(struct expr_fragment *stack, int *sp,
    struct expr_fragment *fragment)
{
  if (stack == NULL || sp == NULL || fragment == NULL || *sp <= 0) {
    return 1;
  }
  *fragment = stack[--(*sp)];
  return 0;
}

static int classifier_name(char *buf, size_t buflen, const char *prefix,
    int32 when_index, int32 guard_index, int32 artifact_index)
{
  if (buf == NULL || buflen == 0) {
    return 1;
  }
  if (snprintf(buf,buflen,"__ascend_when_%d_guard_%d_%s_%d",
      (int)when_index,(int)guard_index,prefix,(int)artifact_index)
      >= (int)buflen) {
    return 1;
  }
  return 0;
}

static struct classifier_artifact *classifier_artifact_new(
    slv_system_t sys, enum classifier_artifact_kind kind, symchar *name,
    struct Instance *inst)
{
  struct classifier_artifact *artifact;

  if (sys->classifier_artifacts == NULL) {
    sys->classifier_artifacts = gl_create(4);
    if (sys->classifier_artifacts == NULL) {
      return NULL;
    }
  }

  artifact = ASC_NEW_CLEAR(struct classifier_artifact);
  if (artifact == NULL) {
    return NULL;
  }
  artifact->kind = kind;
  artifact->name = name;
  artifact->inst = inst;
  gl_append_ptr(sys->classifier_artifacts,artifact);
  return artifact;
}

static struct Instance *classifier_context_instance(struct w_when *when)
{
  struct Instance *context;

  if (when == NULL || when_instance(when) == NULL) {
    return NULL;
  }
  context = InstanceParent(IPTR(when_instance(when)),1);
  if (context == NULL) {
    context = IPTR(when_instance(when));
  }
  return context;
}

static int classifier_create_relation_artifact(slv_system_t sys,
    struct Instance *context, struct Expr *expr, symchar *name,
    struct classifier_artifact **artifact_out)
{
  struct Instance *relinst;
  struct relation *reln;
  struct classifier_artifact *artifact;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;

  if (artifact_out != NULL) {
    *artifact_out = NULL;
  }
  if (sys == NULL || context == NULL || expr == NULL || name == NULL) {
    return 1;
  }

  relinst = CreateRelationInstance(FindRelationType(),e_token);
  if (relinst == NULL) {
    return 1;
  }
  reln = CreateTokenRelation(context,relinst,expr,&err);
  if (reln == NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,
        "Could not create generated WHEN guard relation artifact.");
    DestroyInstance(relinst,NULL);
    return 1;
  }
  SetRelationIsCond(reln);
  relinst_set_conditional(relinst,TRUE);
  SetInstanceRelation(relinst,reln,e_token);

  artifact = classifier_artifact_new(sys,CLASSIFIER_ARTIFACT_REL,
                                     name,relinst);
  if (artifact == NULL) {
    DestroyInstance(relinst,NULL);
    return 1;
  }
  artifact->rel = rel_create(relinst,NULL);
  artifact->bnd = bnd_create(NULL);
  if (artifact->rel == NULL || artifact->bnd == NULL) {
    return 1;
  }
  rel_set_flags(artifact->rel,REL_INCLUDED | REL_ACTIVE | REL_CONDITIONAL);
  bnd_set_kind(artifact->bnd,e_bnd_rel);
  bnd_real_cond(artifact->bnd) = bnd_rel(artifact->rel);
  bnd_set_flags(artifact->bnd,BND_REAL | BND_IN_LOGREL);
  if (artifact_out != NULL) {
    *artifact_out = artifact;
  }
  return 0;
}

static int classifier_create_logrel_artifact(slv_system_t sys,
    struct Instance *context, struct Expr *expr, symchar *name,
    struct classifier_name_resolver *resolver)
{
  struct Instance *lrelinst;
  struct logrelation *lreln;
  struct classifier_artifact *artifact;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;

  if (sys == NULL || context == NULL || expr == NULL || name == NULL) {
    return 1;
  }

  lrelinst = CreateLogRelInstance(FindLogRelType());
  if (lrelinst == NULL) {
    return 1;
  }
  lreln = CreateLogicalRelationWithSatisfiedResolver(context,lrelinst,expr,
      &err,classifier_resolve_name,resolver);
  if (lreln == NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,
        "Could not create generated WHEN guard logical artifact.");
    DestroyInstance(lrelinst,NULL);
    return 1;
  }
  SetLogRelIsCond(lreln);
  logrelinst_set_conditional(lrelinst,TRUE);
  SetInstanceLogRel(lrelinst,lreln);

  artifact = classifier_artifact_new(sys,CLASSIFIER_ARTIFACT_LOGREL,
                                     name,lrelinst);
  if (artifact == NULL) {
    DestroyInstance(lrelinst,NULL);
    return 1;
  }
  artifact->logrel = logrel_create(lrelinst,NULL);
  artifact->bnd = bnd_create(NULL);
  if (artifact->logrel == NULL || artifact->bnd == NULL) {
    return 1;
  }
  logrel_set_flags(artifact->logrel,
      LOGREL_INCLUDED | LOGREL_ACTIVE | LOGREL_CONDITIONAL);
  bnd_set_kind(artifact->bnd,e_bnd_logrel);
  bnd_log_cond(artifact->bnd) = bnd_logrel(artifact->logrel);
  bnd_set_flags(artifact->bnd,BND_IN_LOGREL);
  return 0;
}

static struct Expr *classifier_satisfied_expr_tol(symchar *name,
    double tolerance, CONST dim_type *dims)
{
  return CreateSatisfiedExpr(CreateSystemIdName(name),tolerance,dims);
}

static struct Expr *classifier_satisfied_expr(symchar *name)
{
  return classifier_satisfied_expr_tol(name,0.0,Dimensionless());
}

static int classifier_materialize_guard(slv_system_t sys,
    struct w_when *when, const struct Expr *guard, int32 guard_index,
    struct classifier_name_resolver *resolver)
{
  struct expr_fragment *stack;
  struct expr_fragment fragment, left, right;
  struct Instance *context;
  const struct Expr *expr;
  int stack_size;
  int sp = 0;
  int artifact_index = 0;
  char namebuf[128];
  symchar *name;
  int result = 1;

  if (sys == NULL || when == NULL || guard == NULL || resolver == NULL) {
    return 1;
  }
  context = classifier_context_instance(when);
  if (context == NULL) {
    return 1;
  }

  stack_size = (int)ExprListLength(guard) + 1;
  stack = ASC_NEW_ARRAY_CLEAR(struct expr_fragment,stack_size);
  if (stack == NULL) {
    return 1;
  }

  for (expr = guard; expr != NULL; expr = NextExpr(expr)) {
    memset(&fragment,0,sizeof(fragment));
    fragment.source_start = expr;
    fragment.source_end = expr;

    switch (ExprType(expr)) {
    case e_zero:
    case e_int:
    case e_real:
    case e_var:
    case e_der:
    case e_pre:
      if (expr_fragment_push(stack,stack_size,&sp,fragment)) {
        goto cleanup;
      }
      break;
    case e_boolean:
      fragment.head = copy_expr_range(expr,expr);
      fragment.tail = fragment.head;
      if (fragment.head == NULL
          || expr_fragment_push(stack,stack_size,&sp,fragment)) {
        goto cleanup;
      }
      break;
    case e_satisfied:
      if (SatisfiedExprName(expr) != NULL) {
        fragment.head = copy_expr_range(expr,expr);
        fragment.tail = fragment.head;
        if (fragment.head == NULL
            || expr_fragment_push(stack,stack_size,&sp,fragment)) {
          goto cleanup;
        }
        break;
      }
      if (expr_fragment_pop(stack,&sp,&right)) {
        goto cleanup;
      }
      if (right.generated_boundary == NULL
          || right.generated_boundary->bnd == NULL
          || bnd_kind(right.generated_boundary->bnd) != e_bnd_rel) {
        expr_fragment_destroy(&right);
        goto cleanup;
      }
      bnd_set_tolerance(
        right.generated_boundary->bnd,SatisfiedExprRValue(expr)
      );
      if (right.head != NULL && ExprType(right.head) == e_satisfied
          && NextExpr(right.head) == NULL) {
        DestroyExprList(right.head);
        right.head = classifier_satisfied_expr_tol(
          right.generated_boundary->name,SatisfiedExprRValue(expr),
          SatisfiedExprRDimensions(expr)
        );
        right.tail = right.head;
        if (right.head == NULL) {
          goto cleanup;
        }
      }
      if (expr_fragment_push(stack,stack_size,&sp,right)) {
        goto cleanup;
      }
      break;
    case e_func:
    case e_uminus:
      if (expr_fragment_pop(stack,&sp,&right)) {
        goto cleanup;
      }
      right.source_end = expr;
      expr_fragment_destroy(&right);
      right.head = NULL;
      right.tail = NULL;
      if (expr_fragment_push(stack,stack_size,&sp,right)) {
        goto cleanup;
      }
      break;
    case e_plus:
    case e_minus:
    case e_times:
    case e_divide:
    case e_power:
    case e_ipower:
      if (expr_fragment_pop(stack,&sp,&right)
          || expr_fragment_pop(stack,&sp,&left)) {
        goto cleanup;
      }
      expr_fragment_destroy(&left);
      expr_fragment_destroy(&right);
      fragment.source_start = left.source_start;
      fragment.source_end = expr;
      if (expr_fragment_push(stack,stack_size,&sp,fragment)) {
        goto cleanup;
      }
      break;
    case e_less:
    case e_greater:
    case e_lesseq:
    case e_greatereq:
    case e_equal:
    case e_notequal:
      if (expr_fragment_pop(stack,&sp,&right)
          || expr_fragment_pop(stack,&sp,&left)) {
        goto cleanup;
      }
      expr_fragment_destroy(&left);
      expr_fragment_destroy(&right);
      if (classifier_name(namebuf,sizeof(namebuf),"rel",
          when_mindex(when),guard_index,++artifact_index)) {
        goto cleanup;
      }
      name = AddSymbol(namebuf);
      fragment.head = classifier_satisfied_expr(name);
      fragment.tail = fragment.head;
      {
        struct Expr *rel_expr = copy_expr_range(left.source_start,expr);
        struct classifier_artifact *rel_artifact = NULL;
        if (rel_expr == NULL
            || classifier_create_relation_artifact(sys,context,rel_expr,name,
                &rel_artifact)) {
          if (rel_expr != NULL) {
            DestroyExprList(rel_expr);
          }
          goto cleanup;
        }
        fragment.generated_boundary = rel_artifact;
        DestroyExprList(rel_expr);
      }
      if (expr_fragment_push(stack,stack_size,&sp,fragment)) {
        goto cleanup;
      }
      break;
    case e_boolean_eq:
    case e_boolean_neq:
      if (expr_fragment_pop(stack,&sp,&right)
          || expr_fragment_pop(stack,&sp,&left)
          || expr_fragment_ensure_bool_copy(&left)
          || expr_fragment_ensure_bool_copy(&right)) {
        expr_fragment_destroy(&left);
        expr_fragment_destroy(&right);
        goto cleanup;
      }
      if (classifier_name(namebuf,sizeof(namebuf),"logrel",
          when_mindex(when),guard_index,++artifact_index)) {
        expr_fragment_destroy(&left);
        expr_fragment_destroy(&right);
        goto cleanup;
      }
      name = AddSymbol(namebuf);
      fragment.source_start = left.source_start;
      fragment.source_end = expr;
      fragment.head = left.head;
      fragment.tail = left.tail;
      if (expr_fragment_append(&fragment,right.head)
          || expr_fragment_append(&fragment,CreateOpExpr(ExprType(expr)))) {
        right.head = NULL;
        expr_fragment_destroy(&fragment);
        goto cleanup;
      }
      right.head = NULL;
      if (classifier_create_logrel_artifact(sys,context,fragment.head,
          name,resolver)) {
        expr_fragment_destroy(&fragment);
        goto cleanup;
      }
      expr_fragment_destroy(&fragment);
      fragment.head = classifier_satisfied_expr(name);
      fragment.tail = fragment.head;
      if (fragment.head == NULL
          || expr_fragment_push(stack,stack_size,&sp,fragment)) {
        goto cleanup;
      }
      break;
    case e_and:
    case e_or:
      if (expr_fragment_pop(stack,&sp,&right)
          || expr_fragment_pop(stack,&sp,&left)
          || expr_fragment_ensure_bool_copy(&left)
          || expr_fragment_ensure_bool_copy(&right)) {
        expr_fragment_destroy(&left);
        expr_fragment_destroy(&right);
        goto cleanup;
      }
      fragment.source_start = left.source_start;
      fragment.source_end = expr;
      fragment.head = left.head;
      fragment.tail = left.tail;
      if (expr_fragment_append(&fragment,right.head)
          || expr_fragment_append(&fragment,CreateOpExpr(ExprType(expr)))) {
        right.head = NULL;
        expr_fragment_destroy(&fragment);
        goto cleanup;
      }
      right.head = NULL;
      if (expr_fragment_push(stack,stack_size,&sp,fragment)) {
        goto cleanup;
      }
      break;
    case e_not:
      if (expr_fragment_pop(stack,&sp,&right)
          || expr_fragment_ensure_bool_copy(&right)) {
        expr_fragment_destroy(&right);
        goto cleanup;
      }
      right.source_end = expr;
      if (expr_fragment_append(&right,CreateOpExpr(e_not))) {
        expr_fragment_destroy(&right);
        goto cleanup;
      }
      if (expr_fragment_push(stack,stack_size,&sp,right)) {
        goto cleanup;
      }
      break;
    default:
      goto cleanup;
    }
  }

  if (sp != 1 || expr_fragment_pop(stack,&sp,&fragment)
      || expr_fragment_ensure_bool_copy(&fragment)) {
    goto cleanup;
  }
  if (expr_fragment_append(&fragment,CreateTrueExpr())
      || expr_fragment_append(&fragment,CreateOpExpr(e_boolean_eq))) {
    expr_fragment_destroy(&fragment);
    goto cleanup;
  }
  if (classifier_name(namebuf,sizeof(namebuf),"guard",
      when_mindex(when),guard_index,0)) {
    expr_fragment_destroy(&fragment);
    goto cleanup;
  }
  name = AddSymbol(namebuf);
  result = classifier_create_logrel_artifact(sys,context,fragment.head,
      name,resolver);
  fragment.head = NULL;
  expr_fragment_destroy(&fragment);

cleanup:
  while (sp > 0) {
    expr_fragment_destroy(&stack[--sp]);
  }
  ascfree(stack);
  return result;
}

static int slv_materialize_classifier_whens(slv_system_t sys,
    enum when_region_request request)
{
  struct w_when **whens;
  int32 nwhens, w;

  if (request != WHEN_REGION_STEADY) {
    return 0;
  }
  if (sys->classifier_artifacts != NULL) {
    return 0;
  }

  whens = slv_get_master_when_list(sys);
  nwhens = slv_get_num_master_whens(sys);
  if (whens == NULL) {
    return 0;
  }

  for (w = 0; w < nwhens; ++w) {
    int32 nguards, g;
    unsigned long c, clen;
    struct gl_list_t *cases;
    int saw_applies;
    struct classifier_name_resolver resolver;

    if (when_inwhen(whens[w])) {
      continue;
    }
    resolver.sys = sys;

    if (!when_case_if_guard_count(whens[w],&nguards)) {
      for (g = 0; g < nguards; ++g) {
        const struct Expr *guard = when_case_if_guard(whens[w],g);
        if (guard != NULL) {
          if (classifier_materialize_guard(sys,whens[w],guard,g,&resolver)) {
            slv_destroy_classifier_artifacts(sys);
            return 1;
          }
        }
      }
      continue;
    }

    /*
     * APPLIES IF predicates are already complete region predicates, but they
     * can still contain natural real boundaries. Materialize them here so
     * compact SATISFIED(real_relation,tol) syntax gets the same generated
     * relation/boundary treatment as CASE IF guards.
     */
    cases = when_cases_list(whens[w]);
    clen = cases != NULL ? gl_length(cases) : 0;
    saw_applies = 0;
    for (c = 1; c <= clen; ++c) {
      const struct when_case *wc =
        (const struct when_case *)gl_fetch(cases,c);
      const struct Expr *applies = when_case_applies(wc);
      saw_applies = saw_applies || applies != NULL;
    }
    if (saw_applies) {
      for (c = 1, g = 0; c <= clen; ++c) {
        const struct when_case *wc =
          (const struct when_case *)gl_fetch(cases,c);
        const struct Expr *applies = when_case_applies(wc);
        if (applies == NULL) {
          slv_destroy_classifier_artifacts(sys);
          return 1;
        }
        if (classifier_materialize_guard(sys,whens[w],applies,g,&resolver)) {
          slv_destroy_classifier_artifacts(sys);
          return 1;
        }
        ++g;
      }
    }
  }
  return classifier_finalize_artifacts(sys);
}

int32 slv_has_classifier_whens(slv_system_t sys)
{
  struct w_when **whens;
  int32 nwhens, w;

  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_has_classifier_whens called with NULL system.");
    return 0;
  }

  whens = slv_get_master_when_list(sys);
  nwhens = slv_get_num_master_whens(sys);
  if (whens == NULL) {
    return 0;
  }

  for (w = 0; w < nwhens; ++w) {
    if (when_has_classifier_predicates(whens[w])) {
      return 1;
    }
  }
  return 0;
}

int32 slv_classifier_regions_lowered(slv_system_t sys,
    enum when_region_request request)
{
  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_classifier_regions_lowered called with NULL system.");
    return 0;
  }

  switch (request) {
  case WHEN_REGION_STEADY:
    return sys->when_regions_lowered_steady;
  case WHEN_REGION_DYNAMIC_CLASSIFIER:
    return sys->when_regions_lowered_dynamic;
  default:
    return 0;
  }
}

int slv_lower_classifier_whens(slv_system_t sys, enum when_region_request request)
{
  struct w_when **whens;
  int32 nwhens, w;

  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_lower_classifier_whens called with NULL system.");
    return 1;
  }

  whens = slv_get_master_when_list(sys);
  nwhens = slv_get_num_master_whens(sys);
  if (whens == NULL) {
    return 0;
  }

  for (w = 0; w < nwhens; ++w) {
    if (!when_inwhen(whens[w])) {
      if (when_lower_classifier_regions(whens[w],request)) {
        return 1;
      }
    }
  }

  switch (request) {
  case WHEN_REGION_STEADY:
    sys->when_regions_lowered_steady = 1;
    break;
  case WHEN_REGION_DYNAMIC_CLASSIFIER:
    sys->when_regions_lowered_dynamic = 1;
    break;
  default:
    return 1;
  }
  return 0;
}

int slv_prepare_classifier_whens(slv_system_t sys,
    enum when_region_request request)
{
  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_prepare_classifier_whens called with NULL system.");
    return 1;
  }

  if (!slv_has_classifier_whens(sys)) {
    return 0;
  }

  if (!slv_classifier_regions_lowered(sys,request)) {
    if (slv_lower_classifier_whens(sys,request)) {
      return 1;
    }
  }
  if (slv_materialize_classifier_whens(sys,request)) {
    return 1;
  }
  if (classifier_install_artifact_lists(sys)) {
    return 1;
  }

  switch (request) {
  case WHEN_REGION_STEADY:
    return reanalyze_solver_lists_with_lowered_whens(sys);
  case WHEN_REGION_DYNAMIC_CLASSIFIER:
    return 0;
  default:
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,
      "slv_prepare_classifier_whens called with unknown WHEN region request.");
    return 1;
  }
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
