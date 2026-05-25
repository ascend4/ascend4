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
#include <float.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/** @TODO should not be ANY compiler includes here, right? */

#include <ascend/general/ascMalloc.h>
#include <ascend/general/panic.h>

#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/createinst.h>
#include <ascend/compiler/derivinst.h>
#include <ascend/compiler/destroyinst.h>
#include <ascend/compiler/exprio.h>
#include <ascend/compiler/exprs.h>
#include <ascend/compiler/find.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/logrelation.h>
#include <ascend/compiler/logrel_util.h>
#include <ascend/compiler/mathinst.h>
#include <ascend/compiler/module.h>
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

static void slv_destroy_classifier_encodings(slv_system_t sys);

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

	slv_destroy_classifier_encodings(sys);
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
  CLASSIFIER_ARTIFACT_DVAR,
  CLASSIFIER_ARTIFACT_REL,
  CLASSIFIER_ARTIFACT_LOGREL
};

struct classifier_artifact {
  enum classifier_artifact_kind kind;
  symchar *name;
  struct Instance *inst;
  struct dis_discrete *dvar;
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
  if (artifact->dvar != NULL) {
    dis_destroy(artifact->dvar);
    ascfree(artifact->dvar);
    artifact->dvar = NULL;
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

static void classifier_encoding_free(
    struct slv_classifier_when_encoding *encoding)
{
  if (encoding == NULL) {
    return;
  }
  if (encoding->cases != NULL) {
    ascfree(encoding->cases);
    encoding->cases = NULL;
  }
  if (encoding->guard_implications != NULL) {
    ascfree(encoding->guard_implications);
    encoding->guard_implications = NULL;
  }
  if (encoding->guard_mutexes != NULL) {
    ascfree(encoding->guard_mutexes);
    encoding->guard_mutexes = NULL;
  }
  if (encoding->guard_dvars != NULL) {
    ascfree(encoding->guard_dvars);
    encoding->guard_dvars = NULL;
  }
  if (encoding->guard_boundaries != NULL) {
    ascfree(encoding->guard_boundaries);
    encoding->guard_boundaries = NULL;
  }
  ascfree(encoding);
}

static void slv_destroy_classifier_encodings(slv_system_t sys)
{
  unsigned long len;

  if (sys == NULL || sys->classifier_encodings == NULL) {
    return;
  }

  len = gl_length(sys->classifier_encodings);
  while (len > 0) {
    classifier_encoding_free(
      (struct slv_classifier_when_encoding *)gl_fetch(
        sys->classifier_encodings,len
      )
    );
    --len;
  }
  gl_destroy(sys->classifier_encodings);
  sys->classifier_encodings = NULL;
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

int32 slv_get_num_classifier_when_encodings(slv_system_t sys)
{
  if (sys == NULL || sys->classifier_encodings == NULL) {
    return 0;
  }
  return (int32)gl_length(sys->classifier_encodings);
}

struct slv_classifier_when_encoding *slv_get_classifier_when_encoding(
    slv_system_t sys, int32 index)
{
  if (sys == NULL || sys->classifier_encodings == NULL || index < 0
      || index >= (int32)gl_length(sys->classifier_encodings)) {
    return NULL;
  }
  return (struct slv_classifier_when_encoding *)gl_fetch(
    sys->classifier_encodings,(unsigned long)index + 1
  );
}

static int classifier_encoding_case_matches(
    const struct slv_classifier_case_encoding *ce,
    const int32 *values, int32 nvalues)
{
  int32 i;

  if (ce == NULL || values == NULL || ce->nvalues != nvalues) {
    return 0;
  }

  for (i = 0; i < nvalues; ++i) {
    if (values[i] != FALSE && values[i] != TRUE) {
      return 0;
    }
    if (ce->values[i] == -2) {
      continue;
    }
    if (ce->values[i] != values[i]) {
      return 0;
    }
  }
  return 1;
}

struct classifier_guard_atom {
  int valid;
  CONST struct Name *name;
  enum Expr_enum op;
  double bound;
  CONST dim_type *dims;
};

struct classifier_guard_expr_item {
  int is_var;
  CONST struct Name *name;
  int is_const;
  double value;
  CONST dim_type *dims;
  int is_bound;
  struct classifier_guard_atom bound;
};

static struct Instance *classifier_context_instance(struct w_when *when);

static enum Expr_enum classifier_reverse_relop(enum Expr_enum op)
{
  switch (op) {
  case e_less:
    return e_greater;
  case e_lesseq:
    return e_greatereq;
  case e_greater:
    return e_less;
  case e_greatereq:
    return e_lesseq;
  default:
    return op;
  }
}

static int classifier_guard_stack_push(struct classifier_guard_expr_item *stack,
    int32 stack_size, int32 *sp, const struct classifier_guard_expr_item *item)
{
  if (stack == NULL || sp == NULL || item == NULL || *sp >= stack_size) {
    return 1;
  }
  stack[(*sp)++] = *item;
  return 0;
}

static int classifier_guard_stack_pop(struct classifier_guard_expr_item *stack,
    int32 *sp, struct classifier_guard_expr_item *item)
{
  if (stack == NULL || sp == NULL || item == NULL || *sp <= 0) {
    return 1;
  }
  *item = stack[--(*sp)];
  return 0;
}

static int classifier_instance_real_fixed_or_constant(struct Instance *inst)
{
  struct Instance *fixed_child;

  if (inst == NULL || !AtomAssigned(inst)) {
    return 0;
  }
  switch (InstanceKind(inst)) {
  case REAL_INST:
  case REAL_ATOM_INST:
  case REAL_CONSTANT_INST:
    break;
  default:
    return 0;
  }
  if (IsConstantInstance(inst)) {
    return 1;
  }
  fixed_child = ChildByChar(inst,AddSymbol("fixed"));
  return fixed_child != NULL
    && AtomAssigned(fixed_child)
    && GetBooleanAtomValue(fixed_child);
}

static int classifier_guard_named_real_threshold(
    struct Instance *context, CONST struct Name *name,
    struct classifier_guard_expr_item *item)
{
  struct gl_list_t *instances;
  struct Instance *inst;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;

  if (context == NULL || name == NULL || item == NULL) {
    return 0;
  }

  instances = FindInstances(context,name,&err);
  if (instances == NULL) {
    return 0;
  }
  if (gl_length(instances) != 1) {
    gl_destroy(instances);
    return 0;
  }

  inst = (struct Instance *)gl_fetch(instances,1);
  if (!classifier_instance_real_fixed_or_constant(inst)) {
    gl_destroy(instances);
    return 0;
  }

  item->is_const = 1;
  item->value = RealAtomValue(inst);
  item->dims = RealAtomDims(inst);
  gl_destroy(instances);
  return 1;
}

static int classifier_guard_expr_simple_bound(const struct Expr *expr,
    struct w_when *when, struct classifier_guard_atom *atom)
{
  struct classifier_guard_expr_item *stack;
  struct Instance *context;
  int32 stack_size, sp = 0;
  int failed = 0;

  if (atom == NULL) {
    return 0;
  }
  atom->valid = 0;
  if (expr == NULL) {
    return 0;
  }
  context = classifier_context_instance(when);

  stack_size = (int32)ExprListLength(expr);
  stack = ASC_NEW_ARRAY_CLEAR(struct classifier_guard_expr_item,stack_size + 1);
  if (stack == NULL) {
    return 0;
  }

  while (expr != NULL && !failed) {
    struct classifier_guard_expr_item item, left, right;
    enum Expr_enum t = ExprType(expr);
    memset(&item,0,sizeof(item));

    switch (t) {
    case e_var:
      item.is_var = 1;
      item.name = ExprName(expr);
      (void)classifier_guard_named_real_threshold(context,item.name,&item);
      failed = classifier_guard_stack_push(stack,stack_size + 1,&sp,&item);
      break;
    case e_zero:
      item.is_const = 1;
      item.value = 0.0;
      item.dims = WildDimension();
      failed = classifier_guard_stack_push(stack,stack_size + 1,&sp,&item);
      break;
    case e_int:
      item.is_const = 1;
      item.value = (double)ExprIValue(expr);
      item.dims = Dimensionless();
      failed = classifier_guard_stack_push(stack,stack_size + 1,&sp,&item);
      break;
    case e_real:
      item.is_const = 1;
      item.value = ExprRValue(expr);
      item.dims = ExprRDimensions(expr);
      failed = classifier_guard_stack_push(stack,stack_size + 1,&sp,&item);
      break;
    case e_satisfied:
      if (SatisfiedExprName(expr) != NULL) {
        failed = 1;
        break;
      }
      if (classifier_guard_stack_pop(stack,&sp,&right) || !right.is_bound) {
        failed = 1;
        break;
      }
      item = right;
      failed = classifier_guard_stack_push(stack,stack_size + 1,&sp,&item);
      break;
    case e_less:
    case e_lesseq:
    case e_greater:
    case e_greatereq:
      if (classifier_guard_stack_pop(stack,&sp,&right)
          || classifier_guard_stack_pop(stack,&sp,&left)) {
        failed = 1;
        break;
      }
      item.is_bound = 1;
      if (left.is_var && !left.is_const && right.is_const) {
        item.bound.valid = 1;
        item.bound.name = left.name;
        item.bound.op = t;
        item.bound.bound = right.value;
        item.bound.dims = right.dims;
      } else if (left.is_const && right.is_var && !right.is_const) {
        item.bound.valid = 1;
        item.bound.name = right.name;
        item.bound.op = classifier_reverse_relop(t);
        item.bound.bound = left.value;
        item.bound.dims = left.dims;
      } else {
        failed = 1;
        break;
      }
      failed = classifier_guard_stack_push(stack,stack_size + 1,&sp,&item);
      break;
    default:
      failed = 1;
      break;
    }
    expr = NextExpr(expr);
  }

  if (!failed && sp == 1 && stack[0].is_bound && stack[0].bound.valid) {
    *atom = stack[0].bound;
  }
  ascfree(stack);
  return atom->valid;
}

static int classifier_guard_atoms_comparable(
    const struct classifier_guard_atom *a,
    const struct classifier_guard_atom *b)
{
  if (a == NULL || b == NULL || !a->valid || !b->valid) {
    return 0;
  }
  if (CompareNames(a->name,b->name) != 0) {
    return 0;
  }
  if (a->dims == NULL || b->dims == NULL) {
    return 0;
  }
  return SameDimen(a->dims,b->dims);
}

static int classifier_bound_is_upper(enum Expr_enum op)
{
  return op == e_less || op == e_lesseq;
}

static int classifier_bound_is_lower(enum Expr_enum op)
{
  return op == e_greater || op == e_greatereq;
}

static int classifier_bound_is_strict(enum Expr_enum op)
{
  return op == e_less || op == e_greater;
}

static int classifier_guard_bound_implies(
    const struct classifier_guard_atom *a,
    const struct classifier_guard_atom *b)
{
  if (!classifier_guard_atoms_comparable(a,b)) {
    return 0;
  }

  if (classifier_bound_is_upper(a->op) && classifier_bound_is_upper(b->op)) {
    if (a->bound < b->bound) {
      return 1;
    }
    if (a->bound == b->bound) {
      return b->op == e_lesseq || classifier_bound_is_strict(a->op);
    }
  }

  if (classifier_bound_is_lower(a->op) && classifier_bound_is_lower(b->op)) {
    if (a->bound > b->bound) {
      return 1;
    }
    if (a->bound == b->bound) {
      return b->op == e_greatereq || classifier_bound_is_strict(a->op);
    }
  }

  return 0;
}

static int classifier_guard_bounds_mutex_ordered(
    const struct classifier_guard_atom *upper,
    const struct classifier_guard_atom *lower)
{
  if (!classifier_guard_atoms_comparable(upper,lower)
      || !classifier_bound_is_upper(upper->op)
      || !classifier_bound_is_lower(lower->op)) {
    return 0;
  }
  if (upper->bound < lower->bound) {
    return 1;
  }
  if (upper->bound == lower->bound
      && (classifier_bound_is_strict(upper->op)
          || classifier_bound_is_strict(lower->op))) {
    return 1;
  }
  return 0;
}

static int classifier_guard_bounds_mutex(
    const struct classifier_guard_atom *a,
    const struct classifier_guard_atom *b)
{
  return classifier_guard_bounds_mutex_ordered(a,b)
    || classifier_guard_bounds_mutex_ordered(b,a);
}

static void classifier_encoding_analyze_guard_logic(
    struct slv_classifier_when_encoding *encoding)
{
  struct classifier_guard_atom atoms[MAX_VAR_IN_LIST];
  int32 i, j;

  if (encoding == NULL || encoding->when == NULL
      || encoding->nguards <= 0 || encoding->nguards > MAX_VAR_IN_LIST) {
    return;
  }

  encoding->guard_implications = ASC_NEW_ARRAY_CLEAR(
    int32,encoding->nguards * encoding->nguards
  );
  encoding->guard_mutexes = ASC_NEW_ARRAY_CLEAR(
    int32,encoding->nguards * encoding->nguards
  );
  if (encoding->guard_implications == NULL
      || encoding->guard_mutexes == NULL) {
    return;
  }

  memset(atoms,0,sizeof(atoms));
  for (i = 0; i < encoding->nguards; ++i) {
    const struct Expr *guard = when_case_if_guard(encoding->when,i);
    (void)classifier_guard_expr_simple_bound(
      guard,encoding->when,&atoms[i]
    );
  }

  for (i = 0; i < encoding->nguards; ++i) {
    for (j = 0; j < encoding->nguards; ++j) {
      if (i == j) {
        continue;
      }
      if (classifier_guard_bound_implies(&atoms[i],&atoms[j])) {
        encoding->guard_implications[i * encoding->nguards + j] = 1;
      }
      if (i < j && classifier_guard_bounds_mutex(&atoms[i],&atoms[j])) {
        encoding->guard_mutexes[i * encoding->nguards + j] = 1;
        encoding->guard_mutexes[j * encoding->nguards + i] = 1;
      }
    }
  }
}

int32 slv_classifier_encoding_match_case(
    const struct slv_classifier_when_encoding *encoding,
    const int32 *values, int32 nvalues)
{
  int32 c;

  if (encoding == NULL || values == NULL || nvalues != encoding->nguards) {
    return -1;
  }

  for (c = 0; c < encoding->ncases; ++c) {
    if (classifier_encoding_case_matches(&(encoding->cases[c]),values,nvalues)) {
      return c;
    }
  }
  return -1;
}

int32 slv_classifier_encoding_uses_discrete(
    const struct slv_classifier_when_encoding *encoding,
    struct dis_discrete *dvar)
{
  struct gl_list_t *dvars;
  unsigned long d, dlen;

  if (encoding == NULL || encoding->when == NULL || dvar == NULL) {
    return 0;
  }
  dvars = when_dvars_list(encoding->when);
  if (dvars == NULL) {
    return 0;
  }
  dlen = gl_length(dvars);
  for (d = 1; d <= dlen; ++d) {
    if ((struct dis_discrete *)gl_fetch(dvars,d) == dvar) {
      return 1;
    }
  }
  return 0;
}

int32 slv_classifier_encoding_case_active(
    const struct slv_classifier_when_encoding *encoding, int32 case_index)
{
  if (encoding == NULL || case_index < 0 || case_index >= encoding->ncases
      || encoding->cases[case_index].wc == NULL) {
    return 0;
  }
  return when_case_active(encoding->cases[case_index].wc) ? 1 : 0;
}

int32 slv_classifier_encoding_current_case(
    const struct slv_classifier_when_encoding *encoding)
{
  struct gl_list_t *dvars;
  int32 values[MAX_VAR_IN_LIST];
  int32 i;

  if (encoding == NULL || encoding->when == NULL
      || encoding->nguards < 0 || encoding->nguards > MAX_VAR_IN_LIST) {
    return -1;
  }
  if (!when_flagbit(encoding->when,WHEN_CLASSIFIER_GUARD_DVARS)) {
    return -1;
  }

  dvars = when_dvars_list(encoding->when);
  if (dvars == NULL || (int32)gl_length(dvars) != encoding->nguards) {
    return -1;
  }

  for (i = 0; i < encoding->nguards; ++i) {
    struct dis_discrete *dvar =
      (struct dis_discrete *)gl_fetch(dvars,(unsigned long)i + 1);
    if (dvar == NULL) {
      return -1;
    }
    values[i] = dis_value(dvar) ? TRUE : FALSE;
  }

  return slv_classifier_encoding_match_case(
    encoding,values,encoding->nguards
  );
}

static int classifier_guard_matrix_value(const int32 *matrix,
    const struct slv_classifier_when_encoding *encoding, int32 row, int32 col)
{
  if (matrix == NULL || encoding == NULL || row < 0 || col < 0
      || row >= encoding->nguards || col >= encoding->nguards) {
    return 0;
  }
  return matrix[row * encoding->nguards + col] ? 1 : 0;
}

int32 slv_classifier_encoding_guard_implies(
    const struct slv_classifier_when_encoding *encoding,
    int32 guard_index, int32 implied_guard_index)
{
  return classifier_guard_matrix_value(
    encoding != NULL ? encoding->guard_implications : NULL,
    encoding,guard_index,implied_guard_index
  );
}

int32 slv_classifier_encoding_guards_mutex(
    const struct slv_classifier_when_encoding *encoding,
    int32 guard_index, int32 other_guard_index)
{
  return classifier_guard_matrix_value(
    encoding != NULL ? encoding->guard_mutexes : NULL,
    encoding,guard_index,other_guard_index
  );
}

int32 slv_classifier_encoding_tuple_admissible(
    const struct slv_classifier_when_encoding *encoding,
    const int32 *values, int32 nvalues)
{
  int32 i, j;

  if (encoding == NULL || values == NULL || nvalues != encoding->nguards) {
    return 0;
  }
  for (i = 0; i < nvalues; ++i) {
    if (values[i] != FALSE && values[i] != TRUE) {
      return 0;
    }
  }
  for (i = 0; i < nvalues; ++i) {
    for (j = 0; j < nvalues; ++j) {
      if (i == j) {
        continue;
      }
      if (values[i] == TRUE && values[j] == FALSE
          && slv_classifier_encoding_guard_implies(encoding,i,j)) {
        return 0;
      }
      if (i < j && values[i] == TRUE && values[j] == TRUE
          && slv_classifier_encoding_guards_mutex(encoding,i,j)) {
        return 0;
      }
    }
  }
  return 1;
}

static int classifier_encoding_case_concrete_values(
    const struct slv_classifier_when_encoding *encoding,
    int32 case_index, int32 *values, int32 nvalues)
{
  int32 nguards, ntuples, mask;

  if (encoding == NULL || values == NULL || case_index < 0
      || case_index >= encoding->ncases || nvalues != encoding->nguards) {
    return 1;
  }
  nguards = encoding->nguards;
  if (nguards < 0 || nguards > MAX_VAR_IN_LIST || nguards > 20) {
    return 1;
  }

  ntuples = 1 << nguards;
  for (mask = 0; mask < ntuples; ++mask) {
    int32 i;
    for (i = 0; i < nguards; ++i) {
      values[i] = (mask & (1 << i)) ? TRUE : FALSE;
    }
    if (slv_classifier_encoding_match_case(encoding,values,nguards)
        == case_index
        && slv_classifier_encoding_tuple_admissible(
          encoding,values,nguards
        )) {
      return 0;
    }
  }
  return 1;
}

int32 slv_classifier_encoding_current_tuple_admissible(
    const struct slv_classifier_when_encoding *encoding)
{
  struct gl_list_t *dvars;
  int32 values[MAX_VAR_IN_LIST];
  int32 i;

  if (encoding == NULL || encoding->when == NULL
      || encoding->nguards < 0 || encoding->nguards > MAX_VAR_IN_LIST) {
    return 0;
  }
  if (!when_flagbit(encoding->when,WHEN_CLASSIFIER_GUARD_DVARS)) {
    return 1;
  }

  dvars = when_dvars_list(encoding->when);
  if (dvars == NULL || (int32)gl_length(dvars) != encoding->nguards) {
    return 0;
  }

  for (i = 0; i < encoding->nguards; ++i) {
    struct dis_discrete *dvar =
      (struct dis_discrete *)gl_fetch(dvars,(unsigned long)i + 1);
    if (dvar == NULL) {
      return 0;
    }
    values[i] = dis_value(dvar) ? TRUE : FALSE;
  }

  return slv_classifier_encoding_tuple_admissible(
    encoding,values,encoding->nguards
  );
}

static int classifier_encoding_set_guard_values(
    const struct slv_classifier_when_encoding *encoding,
    const int32 *values, int32 nvalues)
{
  struct gl_list_t *dvars;
  int32 i;

  if (encoding == NULL || values == NULL || nvalues != encoding->nguards
      || encoding->when == NULL) {
    return 1;
  }
  if (!when_flagbit(encoding->when,WHEN_CLASSIFIER_GUARD_DVARS)) {
    return 0;
  }

  dvars = when_dvars_list(encoding->when);
  if (dvars == NULL || (int32)gl_length(dvars) != nvalues) {
    return 1;
  }

  for (i = 0; i < nvalues; ++i) {
    struct dis_discrete *dvar =
      (struct dis_discrete *)gl_fetch(dvars,(unsigned long)i + 1);
    if (dvar == NULL) {
      return 1;
    }
    if (values[i] == TRUE || values[i] == FALSE) {
      dis_set_inst_and_field_value(dvar,values[i]);
    }
  }
  return 0;
}

int slv_reanalyze_with_classifier_encoding_values(slv_system_t sys,
    const struct slv_classifier_when_encoding *encoding,
    const int32 *values, int32 nvalues, int32 *case_index)
{
  int32 match;

  if (case_index != NULL) {
    *case_index = -1;
  }
  if (sys == NULL || encoding == NULL || values == NULL) {
    return 1;
  }

  match = slv_classifier_encoding_match_case(encoding,values,nvalues);
  if (match < 0 || match >= encoding->ncases) {
    return 1;
  }
  if (!slv_classifier_encoding_tuple_admissible(encoding,values,nvalues)) {
    return 1;
  }
  if (!when_flagbit(encoding->when,WHEN_CLASSIFIER_GUARD_DVARS)) {
    return 1;
  }

  if (classifier_encoding_set_guard_values(encoding,values,nvalues)) {
    return 1;
  }
  if (reanalyze_solver_lists_with_classifier_guard_values(sys)) {
    return 1;
  }
  if (case_index != NULL) {
    *case_index = match;
  }
  return 0;
}

int slv_sync_classifier_guards_from_boundaries(slv_system_t sys)
{
  unsigned long e, elen;

  if (sys == NULL || sys->classifier_encodings == NULL) {
    return 0;
  }

  elen = gl_length(sys->classifier_encodings);
  for (e = 1; e <= elen; ++e) {
    struct slv_classifier_when_encoding *encoding =
      (struct slv_classifier_when_encoding *)gl_fetch(
        sys->classifier_encodings,e
      );
    int32 g;

    if (encoding == NULL || encoding->nguards < 0
        || encoding->nguards > MAX_VAR_IN_LIST) {
      return 1;
    }
    if (encoding->nguards == 0) {
      continue;
    }
    if (encoding->guard_dvars == NULL || encoding->guard_boundaries == NULL) {
      continue;
    }

    for (g = 0; g < encoding->nguards; ++g) {
      struct dis_discrete *dvar = encoding->guard_dvars[g];
      struct bnd_boundary *bnd = encoding->guard_boundaries[g];
      int32 cur_value;
      int32 pre_value;

      if (dvar == NULL || bnd == NULL) {
        continue;
      }
      cur_value = bnd_cur_status(bnd) ? TRUE : FALSE;
      pre_value = bnd_pre_status(bnd) ? TRUE : FALSE;
      dis_set_inst_and_field_value(dvar,cur_value);
      dis_set_previous_value(dvar,pre_value);
    }
  }
  return 0;
}

int slv_reanalyze_classifier_whens_from_guard_values(slv_system_t sys)
{
  return reanalyze_solver_lists_with_classifier_guard_values(sys);
}

int slv_sync_classifier_guard_values_for_case(
    slv_system_t sys, struct w_when *when, struct when_case *wc)
{
  unsigned long i, len;

  if (sys == NULL || when == NULL || wc == NULL
      || sys->classifier_encodings == NULL) {
    return 0;
  }

  len = gl_length(sys->classifier_encodings);
  for (i = 1; i <= len; ++i) {
    struct slv_classifier_when_encoding *encoding =
      (struct slv_classifier_when_encoding *)gl_fetch(
        sys->classifier_encodings,i
      );
    int32 c;

    if (encoding == NULL || encoding->when != when) {
      continue;
    }
    for (c = 0; c < encoding->ncases; ++c) {
      if (encoding->cases[c].wc == wc) {
        int32 values[MAX_VAR_IN_LIST];
        if (classifier_encoding_case_concrete_values(
            encoding,c,values,encoding->nguards
            )) {
          return 1;
        }
        return classifier_encoding_set_guard_values(
          encoding,values,encoding->nguards
        );
      }
    }
  }

  return 0;
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
  unsigned long i, len;
  int32 count = 0;

  if (sys == NULL || sys->classifier_artifacts == NULL) {
    return 0;
  }

  len = gl_length(sys->classifier_artifacts);
  for (i = 1; i <= len; ++i) {
    struct classifier_artifact *artifact =
      (struct classifier_artifact *)gl_fetch(sys->classifier_artifacts,i);
    if (artifact != NULL && artifact->bnd != NULL) {
      ++count;
    }
  }
  return count;
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
  unsigned long i, len;
  int32 count = 0;

  if (sys == NULL || sys->classifier_artifacts == NULL || index < 0
      || index >= (int32)gl_length(sys->classifier_artifacts)) {
    return NULL;
  }

  len = gl_length(sys->classifier_artifacts);
  for (i = 1; i <= len; ++i) {
    struct classifier_artifact *artifact =
      (struct classifier_artifact *)gl_fetch(sys->classifier_artifacts,i);
    if (artifact != NULL && artifact->bnd != NULL) {
      if (count == index) {
        return artifact->bnd;
      }
      ++count;
    }
  }
  return NULL;
}

static const char *classifier_artifact_kind_label(
    enum classifier_artifact_kind kind)
{
  switch (kind) {
  case CLASSIFIER_ARTIFACT_DVAR:
    return "dvar";
  case CLASSIFIER_ARTIFACT_REL:
    return "rel";
  case CLASSIFIER_ARTIFACT_LOGREL:
    return "logrel";
  default:
    return "unknown";
  }
}

static const char *classifier_bool_label(int32 value)
{
  if (value == TRUE) {
    return "TRUE";
  }
  if (value == FALSE) {
    return "FALSE";
  }
  if (value == -2) {
    return "*";
  }
  if (value == -1) {
    return "OTHERWISE";
  }
  return "?";
}

static const char *classifier_region_source_label(int32 source)
{
  switch (source) {
  case WHEN_REGION_NONE:
    return "none";
  case WHEN_REGION_APPLIES:
    return "APPLIES IF";
  case WHEN_REGION_CASE_IF:
    return "CASE IF";
  case WHEN_REGION_CASE_IF_OTHERWISE:
    return "OTHERWISE";
  default:
    return "unknown";
  }
}

static const char *classifier_symbol_name_for_value(slv_system_t sys,
    int32 value)
{
  unsigned long i, len;

  if (sys == NULL || sys->symbollist == NULL || value <= 0) {
    return NULL;
  }

  len = gl_length(sys->symbollist);
  for (i = 1; i <= len; ++i) {
    struct SymbolValues *entry =
      (struct SymbolValues *)gl_fetch(sys->symbollist,i);
    if (entry != NULL && entry->value == value) {
      return entry->name;
    }
  }
  return NULL;
}

static const char *classifier_case_source_label(slv_system_t sys,
    struct when_case *wc)
{
  symchar *otherwise_label;
  int32 *values;

  if (wc == NULL) {
    return NULL;
  }

  otherwise_label = when_case_otherwise_label(wc);
  if (otherwise_label != NULL) {
    return SCP(otherwise_label);
  }

  values = when_case_values_list(wc);
  if (values == NULL) {
    return NULL;
  }
  return classifier_symbol_name_for_value(sys,values[0]);
}

static void classifier_write_expr(FILE *fp, const struct Expr *expr)
{
  if (expr == NULL) {
    FPRINTF(fp,"<none>");
    return;
  }
  WriteExprInfix(fp,expr);
}

static void classifier_write_case_pattern(FILE *fp, const int32 *values,
    int32 nvalues)
{
  int32 i;

  FPRINTF(fp,"[");
  for (i = 0; i < nvalues; ++i) {
    if (i > 0) {
      FPRINTF(fp,",");
    }
    FPRINTF(fp,"%s",classifier_bool_label(values[i]));
  }
  FPRINTF(fp,"]");
}

static void classifier_encoding_write_tuple(FILE *fp,
    const int32 *values, int32 nvalues)
{
  classifier_write_case_pattern(fp,values,nvalues);
}

static void classifier_write_case_expansion_summary(FILE *fp,
    const struct slv_classifier_when_encoding *encoding, int32 case_index)
{
  int32 nguards, ntuples, mask;
  int32 admissible = 0;
  int32 rejected = 0;
  int32 examples = 0;

  if (encoding == NULL || case_index < 0 || case_index >= encoding->ncases) {
    return;
  }
  nguards = encoding->nguards;
  if (nguards < 0 || nguards > 12) {
    FPRINTF(fp,"      expanded_tuples=<not enumerated: %d guards>\n",
        (int)nguards);
    return;
  }

  ntuples = 1 << nguards;
  for (mask = 0; mask < ntuples; ++mask) {
    int32 values[MAX_VAR_IN_LIST];
    int32 i;
    int32 matched;
    int32 tuple_ok;

    for (i = 0; i < nguards; ++i) {
      values[i] = (mask & (1 << i)) ? TRUE : FALSE;
    }
    matched = slv_classifier_encoding_match_case(
      encoding,values,nguards
    );
    if (matched != case_index) {
      continue;
    }
    tuple_ok = slv_classifier_encoding_tuple_admissible(
      encoding,values,nguards
    );
    if (tuple_ok) {
      ++admissible;
    } else {
      ++rejected;
    }
  }

  FPRINTF(fp,"      expanded_tuples admissible=%d rejected_by_guard_logic=%d",
      admissible,rejected);
  if (rejected > 0) {
    FPRINTF(fp," rejected_examples=");
    for (mask = 0; mask < ntuples && examples < 3; ++mask) {
      int32 values[MAX_VAR_IN_LIST];
      int32 i;
      for (i = 0; i < nguards; ++i) {
        values[i] = (mask & (1 << i)) ? TRUE : FALSE;
      }
      if (slv_classifier_encoding_match_case(encoding,values,nguards)
          == case_index
          && !slv_classifier_encoding_tuple_admissible(
            encoding,values,nguards
          )) {
        if (examples > 0) {
          FPRINTF(fp,",");
        }
        classifier_encoding_write_tuple(fp,values,nguards);
        ++examples;
      }
    }
  }
  FPRINTF(fp,"\n");
}

static struct classifier_artifact *classifier_artifact_find_dvar_ptr(
    slv_system_t sys, struct dis_discrete *dvar)
{
  unsigned long i, len;

  if (sys == NULL || sys->classifier_artifacts == NULL || dvar == NULL) {
    return NULL;
  }
  len = gl_length(sys->classifier_artifacts);
  for (i = 1; i <= len; ++i) {
    struct classifier_artifact *artifact =
      (struct classifier_artifact *)gl_fetch(sys->classifier_artifacts,i);
    if (artifact != NULL && artifact->dvar == dvar) {
      return artifact;
    }
  }
  return NULL;
}

static void classifier_write_discrete_name(slv_system_t sys,
    struct dis_discrete *dvar, FILE *fp)
{
  struct classifier_artifact *artifact;

  if (dvar == NULL) {
    FPRINTF(fp,"<none>");
    return;
  }
  artifact = classifier_artifact_find_dvar_ptr(sys,dvar);
  if (artifact != NULL && artifact->name != NULL) {
    FPRINTF(fp,"%s",SCP(artifact->name));
    return;
  }
  dis_write_name(sys,dvar,fp);
}

static void classifier_write_when_header(slv_system_t sys,
    struct w_when *when, FILE *fp)
{
  FPRINTF(fp,"WHEN ");
  when_write_name(sys,when,fp);
  FPRINTF(fp," m=%d s=%d flags=0x%x classifier=%d guard_dvars=%d\n",
      when_mindex(when),when_sindex(when),when_flags(when),
      when_has_classifier_predicates(when) ? 1 : 0,
      when_flagbit(when,WHEN_CLASSIFIER_GUARD_DVARS) ? 1 : 0);
}

static void classifier_write_when_selectors(slv_system_t sys,
    struct w_when *when, FILE *fp)
{
  struct gl_list_t *dvars;
  unsigned long d, dlen;

  dvars = when_dvars_list(when);
  dlen = dvars != NULL ? gl_length(dvars) : 0;
  FPRINTF(fp,"  selectors %lu\n",dlen);
  for (d = 1; d <= dlen; ++d) {
    struct dis_discrete *dvar = (struct dis_discrete *)gl_fetch(dvars,d);
    FPRINTF(fp,"    %lu: ",d);
    classifier_write_discrete_name(sys,dvar,fp);
    FPRINTF(fp," value=%d previous=%d m=%d s=%d flags=0x%x\n",
        dis_value(dvar),dis_previous_value(dvar),dis_mindex(dvar),
        dis_sindex(dvar),dis_flags(dvar));
  }
}

static void classifier_write_case_source(struct when_case *wc, FILE *fp)
{
  struct module_t *module;
  unsigned long line;

  module = when_case_source_module(wc);
  line = when_case_source_line(wc);
  if (module != NULL || line != 0) {
    FPRINTF(fp," source=");
    if (module != NULL) {
      FPRINTF(fp,"%s",Asc_ModuleBestName(module));
    } else {
      FPRINTF(fp,"<unknown>");
    }
    if (line != 0) {
      FPRINTF(fp,":%lu",line);
    }
  }
}

static void classifier_write_when_cases(struct w_when *when,
    const struct slv_classifier_when_encoding *encoding, FILE *fp)
{
  struct gl_list_t *cases;
  unsigned long c, clen;

  cases = when_cases_list(when);
  clen = cases != NULL ? gl_length(cases) : 0;
  FPRINTF(fp,"  cases %lu\n",clen);
  for (c = 1; c <= clen; ++c) {
    struct when_case *wc = (struct when_case *)gl_fetch(cases,c);
    const struct Expr *condition = when_case_condition(wc);
    const struct Expr *applies = when_case_applies(wc);
    const struct Expr *region = when_case_region_predicate(wc);
    symchar *otherwise_label = when_case_otherwise_label(wc);
    int32 values[MAX_VAR_IN_LIST];
    int32 nvalues = 0;

    FPRINTF(fp,"    case %lu number=%d active=%d",
        c,when_case_case_number(wc),when_case_active(wc) ? 1 : 0);
    classifier_write_case_source(wc,fp);
    if (otherwise_label != NULL) {
      FPRINTF(fp," label='%s'",SCP(otherwise_label));
    }
    FPRINTF(fp,"\n");

    if (encoding != NULL && c <= (unsigned long)encoding->ncases) {
      struct slv_classifier_case_encoding *ce = &(encoding->cases[c - 1]);
      FPRINTF(fp,"      encoded_pattern=");
      classifier_write_case_pattern(fp,ce->values,ce->nvalues);
      FPRINTF(fp,"\n");
      classifier_write_case_expansion_summary(fp,encoding,(int32)c - 1);
    } else if (!when_case_if_pattern(when,wc,values,&nvalues)) {
      FPRINTF(fp,"      case_if_pattern=");
      classifier_write_case_pattern(fp,values,nvalues);
      FPRINTF(fp,"\n");
    }

    FPRINTF(fp,"      condition=");
    classifier_write_expr(fp,condition);
    FPRINTF(fp,"\n      applies=");
    classifier_write_expr(fp,applies);
    FPRINTF(fp,"\n      lowered_region(%s)=",
        classifier_region_source_label(when_case_region_source(wc)));
    classifier_write_expr(fp,region);
    FPRINTF(fp,"\n");
  }
}

static const struct slv_classifier_when_encoding *
classifier_encoding_for_when(slv_system_t sys, struct w_when *when)
{
  unsigned long i, len;

  if (sys == NULL || sys->classifier_encodings == NULL || when == NULL) {
    return NULL;
  }
  len = gl_length(sys->classifier_encodings);
  for (i = 1; i <= len; ++i) {
    struct slv_classifier_when_encoding *encoding =
      (struct slv_classifier_when_encoding *)gl_fetch(
          sys->classifier_encodings,i);
    if (encoding != NULL && encoding->when == when) {
      return encoding;
    }
  }
  return NULL;
}

static void classifier_write_encoding(FILE *fp,
    const struct slv_classifier_when_encoding *encoding)
{
  int32 c;

  if (encoding == NULL) {
    return;
  }
  FPRINTF(fp,"  encoding guards=%d cases=%d\n",
      encoding->nguards,encoding->ncases);
  if (encoding->guard_implications != NULL || encoding->guard_mutexes != NULL) {
    int32 i, j, first = 1;
    FPRINTF(fp,"    guard_logic=");
    for (i = 0; i < encoding->nguards; ++i) {
      for (j = 0; j < encoding->nguards; ++j) {
        if (i == j) {
          continue;
        }
        if (slv_classifier_encoding_guard_implies(encoding,i,j)) {
          FPRINTF(fp,"%s%d=>%d",first ? "" : ";",(int)i,(int)j);
          first = 0;
        }
        if (i < j && slv_classifier_encoding_guards_mutex(encoding,i,j)) {
          FPRINTF(fp,"%s%d!%d",first ? "" : ";",(int)i,(int)j);
          first = 0;
        }
      }
    }
    if (first) {
      FPRINTF(fp,"<none>");
    }
    FPRINTF(fp,"\n");
  }
  for (c = 0; c < encoding->ncases; ++c) {
    FPRINTF(fp,"    encoded case %d active=%d pattern=",c,
        slv_classifier_encoding_case_active(encoding,c) ? 1 : 0);
    classifier_write_case_pattern(fp,encoding->cases[c].values,
        encoding->cases[c].nvalues);
    FPRINTF(fp,"\n");
  }
}

static void classifier_write_artifact_name(slv_system_t sys,
    struct classifier_artifact *artifact, FILE *fp)
{
  (void)sys;
  if (artifact == NULL) {
    FPRINTF(fp,"<none>");
    return;
  }
  FPRINTF(fp,"%s",artifact->name != NULL ? SCP(artifact->name) : "<unnamed>");
}

static void classifier_write_artifact(slv_system_t sys,
    struct classifier_artifact *artifact, FILE *fp)
{
  if (artifact == NULL) {
    return;
  }

  FPRINTF(fp,"  %s %s name=",classifier_artifact_kind_label(artifact->kind),
      artifact->name != NULL ? SCP(artifact->name) : "<unnamed>");
  classifier_write_artifact_name(sys,artifact,fp);

  if (artifact->dvar != NULL) {
    FPRINTF(fp," value=%d previous=%d m=%d s=%d flags=0x%x",
        dis_value(artifact->dvar),dis_previous_value(artifact->dvar),
        dis_mindex(artifact->dvar),dis_sindex(artifact->dvar),
        dis_flags(artifact->dvar));
  }
  if (artifact->rel != NULL) {
    FPRINTF(fp," rel_m=%d rel_s=%d active=%d included=%d conditional=%d"
        " equality=%d residual=%g flags=0x%x incidences=%d",
        rel_mindex(artifact->rel),rel_sindex(artifact->rel),
        rel_active(artifact->rel) ? 1 : 0,
        rel_included(artifact->rel) ? 1 : 0,
        rel_conditional(artifact->rel) ? 1 : 0,
        rel_equality(artifact->rel) ? 1 : 0,
        rel_residual(artifact->rel),rel_flags(artifact->rel),
        rel_n_incidences(artifact->rel));
  }
  if (artifact->logrel != NULL) {
    FPRINTF(fp," logrel_m=%d logrel_s=%d active=%d included=%d"
        " conditional=%d equality=%d residual=%d flags=0x%x incidences=%d",
        logrel_mindex(artifact->logrel),logrel_sindex(artifact->logrel),
        logrel_active(artifact->logrel) ? 1 : 0,
        logrel_included(artifact->logrel) ? 1 : 0,
        logrel_conditional(artifact->logrel) ? 1 : 0,
        logrel_equality(artifact->logrel) ? 1 : 0,
        logrel_residual(artifact->logrel),logrel_flags(artifact->logrel),
        logrel_n_incidences(artifact->logrel));
  }
  if (artifact->bnd != NULL) {
    FPRINTF(fp," bnd_m=%d bnd_s=%d kind=%d tol=%g cur=%d pre=%d"
        " at_zero=%d crossed=%d flags=0x%x",
        bnd_mindex(artifact->bnd),bnd_sindex(artifact->bnd),
        bnd_kind(artifact->bnd),bnd_tolerance(artifact->bnd),
        bnd_status_cur(artifact->bnd),bnd_status_pre(artifact->bnd),
        bnd_at_zero(artifact->bnd) ? 1 : 0,
        bnd_crossed(artifact->bnd) ? 1 : 0,
        bnd_flags(artifact->bnd));
  }
  FPRINTF(fp,"\n");
}

int slv_write_classifier_lowered_view(slv_system_t sys, FILE *fp)
{
  struct w_when **whens;
  int32 nwhens, w;
  unsigned long i, len;

  if (sys == NULL || fp == NULL) {
    return 1;
  }

  FPRINTF(fp,"ASCEND classifier WHEN lowered view\n");
  FPRINTF(fp,"  steady_lowered=%d dynamic_lowered=%d artifacts_installed=%d\n",
      sys->when_regions_lowered_steady,sys->when_regions_lowered_dynamic,
      sys->classifier_artifacts_installed);
  FPRINTF(fp,"  encodings=%d artifacts=%lu rel_artifacts=%d"
      " logrel_artifacts=%d bnd_artifacts=%d\n",
      slv_get_num_classifier_when_encodings(sys),
      sys->classifier_artifacts != NULL
        ? gl_length(sys->classifier_artifacts) : 0,
      slv_get_num_classifier_rels(sys),
      slv_get_num_classifier_logrels(sys),
      slv_get_num_classifier_bnds(sys));

  whens = slv_get_master_when_list(sys);
  nwhens = slv_get_num_master_whens(sys);
  FPRINTF(fp,"\nClassifier WHENs\n");
  if (whens == NULL || nwhens == 0) {
    FPRINTF(fp,"  <none>\n");
  } else {
    for (w = 0; w < nwhens; ++w) {
      const struct slv_classifier_when_encoding *encoding;
      if (whens[w] == NULL || !when_has_classifier_predicates(whens[w])) {
        continue;
      }
      encoding = classifier_encoding_for_when(sys,whens[w]);
      classifier_write_when_header(sys,whens[w],fp);
      classifier_write_when_selectors(sys,whens[w],fp);
      classifier_write_encoding(fp,encoding);
      classifier_write_when_cases(whens[w],encoding,fp);
    }
  }

  FPRINTF(fp,"\nGenerated classifier artifacts\n");
  if (sys->classifier_artifacts == NULL) {
    FPRINTF(fp,"  <none>\n");
  } else {
    len = gl_length(sys->classifier_artifacts);
    for (i = 1; i <= len; ++i) {
      classifier_write_artifact(sys,
        (struct classifier_artifact *)gl_fetch(sys->classifier_artifacts,i),
        fp);
    }
  }
  FPRINTF(fp,"END ASCEND classifier WHEN lowered view\n");
  return 0;
}

static int32 classifier_active_case_index(
    const struct slv_classifier_when_encoding *encoding)
{
  int32 c;

  if (encoding == NULL) {
    return -1;
  }
  for (c = 0; c < encoding->ncases; ++c) {
    if (slv_classifier_encoding_case_active(encoding,c)) {
      return c;
    }
  }
  return -1;
}

static void classifier_write_compact_guard_tuple(slv_system_t sys,
    const struct slv_classifier_when_encoding *encoding, FILE *fp)
{
  struct gl_list_t *dvars;
  unsigned long d, dlen;

  if (encoding == NULL || encoding->when == NULL) {
    FPRINTF(fp,"[]");
    return;
  }

  dvars = when_dvars_list(encoding->when);
  dlen = dvars != NULL ? gl_length(dvars) : 0;
  FPRINTF(fp,"[");
  for (d = 1; d <= dlen; ++d) {
    struct dis_discrete *dvar = (struct dis_discrete *)gl_fetch(dvars,d);
    if (d > 1) {
      FPRINTF(fp,",");
    }
    if (dvar == NULL) {
      FPRINTF(fp,"?");
    } else {
      FPRINTF(fp,"%d",dis_value(dvar));
    }
  }
  FPRINTF(fp,"]");
  (void)sys;
}

static void classifier_write_compact_case_label(
    const struct slv_classifier_when_encoding *encoding, int32 case_index,
    FILE *fp)
{
  const struct slv_classifier_case_encoding *ce;

  if (encoding == NULL || case_index < 0 || case_index >= encoding->ncases) {
    FPRINTF(fp,"<none>");
    return;
  }

  ce = &(encoding->cases[case_index]);
  if (ce->label != NULL) {
    FPRINTF(fp,"%s",ce->label);
  } else {
    FPRINTF(fp,"case%d",(int)case_index);
  }
}

static void classifier_write_compact_encoding_summary(slv_system_t sys,
    const struct slv_classifier_when_encoding *encoding, int32 index, FILE *fp)
{
  int32 active_case;
  int32 c;

  active_case = classifier_active_case_index(encoding);
  FPRINTF(fp,"  encoding[%d] guards=",index);
  classifier_write_compact_guard_tuple(sys,encoding,fp);
  FPRINTF(fp," active=%d label=",active_case);
  classifier_write_compact_case_label(encoding,active_case,fp);
  FPRINTF(fp," cases=");
  if (encoding == NULL) {
    FPRINTF(fp,"<none>\n");
    return;
  }
  FPRINTF(fp,"{");
  for (c = 0; c < encoding->ncases; ++c) {
    if (c > 0) {
      FPRINTF(fp,";");
    }
    FPRINTF(fp,"%d:",(int)c);
    classifier_write_compact_case_label(encoding,c,fp);
    FPRINTF(fp,"=");
    classifier_write_case_pattern(fp,encoding->cases[c].values,
        encoding->cases[c].nvalues);
  }
  FPRINTF(fp,"}\n");
}

static void classifier_write_compact_artifact_summary(slv_system_t sys,
    FILE *fp)
{
  unsigned long i, len;
  int32 bcount = 0;

  if (sys == NULL || sys->classifier_artifacts == NULL) {
    FPRINTF(fp,"  boundaries=<none>\n");
    return;
  }

  FPRINTF(fp,"  boundaries=");
  len = gl_length(sys->classifier_artifacts);
  for (i = 1; i <= len; ++i) {
    struct classifier_artifact *artifact =
      (struct classifier_artifact *)gl_fetch(sys->classifier_artifacts,i);
    if (artifact == NULL || artifact->bnd == NULL) {
      continue;
    }
    if (bcount > 0) {
      FPRINTF(fp,";");
    }
    FPRINTF(fp,"%d:",bcount);
    classifier_write_artifact_name(sys,artifact,fp);
    FPRINTF(fp," cur=%d pre=%d zero=%d cross=%d",
        bnd_status_cur(artifact->bnd),bnd_status_pre(artifact->bnd),
        bnd_at_zero(artifact->bnd) ? 1 : 0,
        bnd_crossed(artifact->bnd) ? 1 : 0);
    if (artifact->rel != NULL) {
      FPRINTF(fp," res=%g",rel_residual(artifact->rel));
    } else if (artifact->logrel != NULL) {
      FPRINTF(fp," lres=%d",logrel_residual(artifact->logrel));
    }
    FPRINTF(fp," tol=%g",bnd_tolerance(artifact->bnd));
    ++bcount;
  }
  if (bcount == 0) {
    FPRINTF(fp,"<none>");
  }
  FPRINTF(fp,"\n");
}

int slv_write_classifier_summary_view(slv_system_t sys, FILE *fp)
{
  int32 i, nencodings;

  if (sys == NULL || fp == NULL) {
    return 1;
  }

  nencodings = slv_get_num_classifier_when_encodings(sys);
  FPRINTF(fp,"ASCEND classifier WHEN summary");
  FPRINTF(fp," encodings=%d artifacts=%lu rels=%d logrels=%d bnds=%d\n",
      nencodings,
      sys->classifier_artifacts != NULL
        ? gl_length(sys->classifier_artifacts) : 0,
      slv_get_num_classifier_rels(sys),
      slv_get_num_classifier_logrels(sys),
      slv_get_num_classifier_bnds(sys));
  FPRINTF(fp,"  guard_vectors=");
  for (i = 0; i < nencodings; ++i) {
    struct slv_classifier_when_encoding *encoding =
      slv_get_classifier_when_encoding(sys,i);
    if (i > 0) {
      FPRINTF(fp,";");
    }
    FPRINTF(fp,"%d:",(int)i);
    classifier_write_compact_guard_tuple(sys,encoding,fp);
  }
  FPRINTF(fp,"\n");
  FPRINTF(fp,"  active_cases=");
  for (i = 0; i < nencodings; ++i) {
    struct slv_classifier_when_encoding *encoding =
      slv_get_classifier_when_encoding(sys,i);
    int32 active_case = classifier_active_case_index(encoding);
    if (i > 0) {
      FPRINTF(fp,";");
    }
    FPRINTF(fp,"%d:%d:",(int)i,(int)active_case);
    classifier_write_compact_case_label(encoding,active_case,fp);
  }
  FPRINTF(fp,"\n");
  for (i = 0; i < nencodings; ++i) {
    classifier_write_compact_encoding_summary(
      sys,slv_get_classifier_when_encoding(sys,i),i,fp
    );
  }
  classifier_write_compact_artifact_summary(sys,fp);
  FPRINTF(fp,"END ASCEND classifier WHEN summary\n");
  return 0;
}

static void slv_maybe_write_classifier_lowered_view(slv_system_t sys)
{
  const char *target;
  FILE *fp;

  target = getenv("ASCEND_DEBUG_CLASSIFIER_WHEN");
  if (target == NULL || target[0] == '\0') {
    return;
  }

  fp = ASCERR;
  if (!strcmp(target,"stdout")) {
    fp = stdout;
  } else if (strcmp(target,"1") && strcmp(target,"stderr")) {
    fp = fopen(target,"w");
    if (fp == NULL) {
      FPRINTF(ASCERR,
        "Unable to open ASCEND_DEBUG_CLASSIFIER_WHEN target '%s'; writing to stderr instead.\n",
        target);
      fp = ASCERR;
    }
  }

  (void)slv_write_classifier_lowered_view(sys,fp);
  if (fp != ASCERR && fp != stdout) {
    fclose(fp);
  }
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
  struct classifier_artifact *artifact;
  int l;

  if (sys == NULL || inst == NULL) {
    return NULL;
  }

  artifact = classifier_artifact_find_inst(sys,inst);
  if (artifact != NULL && artifact->dvar != NULL) {
    return artifact->dvar;
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
    case CLASSIFIER_ARTIFACT_DVAR:
      break;
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

static int classifier_alloc_dvar_lists(struct dis_discrete ***master_new,
    struct dis_discrete ***solver_new, int32 nmaster, int32 nsolver,
    int32 nadd)
{
  *master_new = NULL;
  *solver_new = NULL;
  if (nadd <= 0) {
    return 0;
  }
  *master_new = ASC_NEW_ARRAY(struct dis_discrete *,nmaster + nadd + 1);
  *solver_new = ASC_NEW_ARRAY(struct dis_discrete *,nsolver + nadd + 1);
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
  struct dis_discrete **old_master_dvars, **old_solver_dvars;
  struct rel_relation **old_master_condrels, **old_solver_condrels;
  struct logrel_relation **old_master_logrels, **old_solver_logrels;
  struct bnd_boundary **old_master_bnds, **old_solver_bnds;
  struct dis_discrete **new_master_dvars = NULL;
  struct dis_discrete **new_solver_dvars = NULL;
  struct rel_relation **new_master_condrels = NULL;
  struct rel_relation **new_solver_condrels = NULL;
  struct logrel_relation **new_master_logrels = NULL;
  struct logrel_relation **new_solver_logrels = NULL;
  struct bnd_boundary **new_master_bnds = NULL;
  struct bnd_boundary **new_solver_bnds = NULL;
  int32 nmaster_dvars, nsolver_dvars;
  int32 nmaster_condrels, nsolver_condrels;
  int32 nmaster_logrels, nsolver_logrels;
  int32 nmaster_bnds, nsolver_bnds;
  int32 ndvars, nrels, nlogrels, nbnds;
  unsigned long i, len;
  int32 cd = 0, cr = 0, clr = 0, cb = 0;

  if (sys == NULL || sys->classifier_artifacts == NULL) {
    return 0;
  }
  if (sys->classifier_artifacts_installed) {
    return 0;
  }

  ndvars = classifier_artifact_count_kind(sys,CLASSIFIER_ARTIFACT_DVAR);
  nrels = slv_get_num_classifier_rels(sys);
  nlogrels = slv_get_num_classifier_logrels(sys);
  nbnds = slv_get_num_classifier_bnds(sys);
  if (ndvars == 0 && nrels == 0 && nlogrels == 0 && nbnds == 0) {
    sys->classifier_artifacts_installed = 1;
    return 0;
  }

  old_master_dvars = slv_get_master_dvar_list(sys);
  old_solver_dvars = slv_get_solvers_dvar_list(sys);
  old_master_condrels = slv_get_master_condrel_list(sys);
  old_solver_condrels = slv_get_solvers_condrel_list(sys);
  old_master_logrels = slv_get_master_logrel_list(sys);
  old_solver_logrels = slv_get_solvers_logrel_list(sys);
  old_master_bnds = slv_get_master_bnd_list(sys);
  old_solver_bnds = slv_get_solvers_bnd_list(sys);
  nmaster_dvars = slv_get_num_master_dvars(sys);
  nsolver_dvars = slv_get_num_solvers_dvars(sys);
  nmaster_condrels = slv_get_num_master_condrels(sys);
  nsolver_condrels = slv_get_num_solvers_condrels(sys);
  nmaster_logrels = slv_get_num_master_logrels(sys);
  nsolver_logrels = slv_get_num_solvers_logrels(sys);
  nmaster_bnds = slv_get_num_master_bnds(sys);
  nsolver_bnds = slv_get_num_solvers_bnds(sys);

  if (classifier_alloc_dvar_lists(&new_master_dvars,&new_solver_dvars,
      nmaster_dvars,nsolver_dvars,ndvars)
      || classifier_alloc_rel_lists(&new_master_condrels,&new_solver_condrels,
      nmaster_condrels,nsolver_condrels,nrels)
      || classifier_alloc_logrel_lists(&new_master_logrels,
        &new_solver_logrels,nmaster_logrels,nsolver_logrels,
        nlogrels)
      || classifier_alloc_bnd_lists(&new_master_bnds,&new_solver_bnds,
        nmaster_bnds,nsolver_bnds,nbnds)) {
    if (new_master_dvars != NULL) ascfree(new_master_dvars);
    if (new_solver_dvars != NULL) ascfree(new_solver_dvars);
    if (new_master_condrels != NULL) ascfree(new_master_condrels);
    if (new_solver_condrels != NULL) ascfree(new_solver_condrels);
    if (new_master_logrels != NULL) ascfree(new_master_logrels);
    if (new_solver_logrels != NULL) ascfree(new_solver_logrels);
    if (new_master_bnds != NULL) ascfree(new_master_bnds);
    if (new_solver_bnds != NULL) ascfree(new_solver_bnds);
    return 1;
  }

  if (new_master_dvars != NULL) {
    for (i = 0; i < (unsigned long)nmaster_dvars; ++i) {
      new_master_dvars[i] = old_master_dvars[i];
    }
    for (i = 0; i < (unsigned long)nsolver_dvars; ++i) {
      new_solver_dvars[i] = old_solver_dvars[i];
    }
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
    if (artifact->dvar != NULL) {
      dis_set_mindex(artifact->dvar,nmaster_dvars + cd);
      dis_set_sindex(artifact->dvar,nsolver_dvars + cd);
      new_master_dvars[nmaster_dvars + cd] = artifact->dvar;
      new_solver_dvars[nsolver_dvars + cd] = artifact->dvar;
      ++cd;
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

  if (new_master_dvars != NULL) {
    new_master_dvars[nmaster_dvars + ndvars] = NULL;
    new_solver_dvars[nsolver_dvars + ndvars] = NULL;
    slv_set_master_dvar_list(sys,new_master_dvars,nmaster_dvars + ndvars);
    if (old_solver_dvars != NULL && old_solver_dvars != old_master_dvars) {
      ascfree(old_solver_dvars);
    }
    slv_set_solvers_dvar_list(sys,new_solver_dvars,nsolver_dvars + ndvars);
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
    struct classifier_name_resolver *resolver, int boundary_artifact)
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
  if (boundary_artifact) {
    SetLogRelIsCond(lreln);
    logrelinst_set_conditional(lrelinst,TRUE);
  }
  SetInstanceLogRel(lrelinst,lreln);

  artifact = classifier_artifact_new(sys,CLASSIFIER_ARTIFACT_LOGREL,
                                     name,lrelinst);
  if (artifact == NULL) {
    DestroyInstance(lrelinst,NULL);
    return 1;
  }
  artifact->logrel = logrel_create(lrelinst,NULL);
  artifact->bnd = boundary_artifact ? bnd_create(NULL) : NULL;
  if (artifact->logrel == NULL || (boundary_artifact && artifact->bnd == NULL)) {
    return 1;
  }
  logrel_set_flags(artifact->logrel,
      LOGREL_INCLUDED | LOGREL_ACTIVE
      | (boundary_artifact ? LOGREL_CONDITIONAL : 0)
      | (LogRelRelop(lreln) == e_boolean_eq ? LOGREL_EQUALITY : 0));
  if (boundary_artifact) {
    bnd_set_kind(artifact->bnd,e_bnd_logrel);
    bnd_log_cond(artifact->bnd) = bnd_logrel(artifact->logrel);
    bnd_set_flags(artifact->bnd,BND_IN_LOGREL);
  }
  return 0;
}

static int classifier_create_boolean_artifact(slv_system_t sys,
    symchar *name, int32 initial_value,
    struct classifier_artifact **artifact_out)
{
  struct TypeDescription *type;
  struct Instance *inst;
  struct classifier_artifact *artifact;

  if (artifact_out != NULL) {
    *artifact_out = NULL;
  }
  if (sys == NULL || name == NULL) {
    return 1;
  }

  type = FindType(AddSymbol(BOOLEAN_VAR_STR));
  if (type == NULL) {
    return 1;
  }
  inst = CreateBooleanInstance(type);
  if (inst == NULL) {
    return 1;
  }
  SetBooleanAtomValue(inst,initial_value ? TRUE : FALSE,(unsigned)0);

  artifact = classifier_artifact_new(sys,CLASSIFIER_ARTIFACT_DVAR,name,inst);
  if (artifact == NULL) {
    DestroyInstance(inst,NULL);
    return 1;
  }
  artifact->dvar = dis_create(inst,NULL);
  if (artifact->dvar == NULL) {
    return 1;
  }
  dis_set_value(artifact->dvar,initial_value ? TRUE : FALSE);
  dis_set_previous_value(artifact->dvar,initial_value ? TRUE : FALSE);
  dis_set_kind(artifact->dvar,e_dis_boolean_t);
  dis_set_flags(artifact->dvar,
      DIS_INWHEN | DIS_BVAR | DIS_BOOLEAN | DIS_INCIDENT);

  if (artifact_out != NULL) {
    *artifact_out = artifact;
  }
  return 0;
}

static struct Expr *classifier_satisfied_expr_tol(symchar *name,
    double tolerance, CONST dim_type *dims)
{
  return CreateSatisfiedExpr(CreateSystemIdName(name),tolerance,dims);
}

static struct Expr *classifier_existing_dvar_expr(struct w_when *when,
    struct dis_discrete *dvar)
{
  struct Instance *context;
  struct Instance *inst;
  struct InstanceName name;

  if (when == NULL || dvar == NULL) {
    return NULL;
  }
  context = classifier_context_instance(when);
  inst = IPTR(dis_instance(dvar));
  if (context == NULL || inst == NULL) {
    return NULL;
  }
  if (!SearchForParent(inst,context)) {
    return NULL;
  }
  name = ParentsName(context,inst);
  if (InstanceNameType(name) != StrName || InstanceNameStr(name) == NULL) {
    return NULL;
  }
  return CreateVarExpr(CreateIdName(InstanceNameStr(name)));
}

static struct Expr *classifier_satisfied_expr(symchar *name)
{
  return classifier_satisfied_expr_tol(name,0.0,Dimensionless());
}

static int classifier_materialize_guard(slv_system_t sys,
    struct w_when *when, const struct Expr *guard, int32 guard_index,
    struct classifier_name_resolver *resolver,
    struct dis_discrete *existing_guard_dvar,
    struct classifier_artifact **guard_dvar_out,
    struct bnd_boundary **guard_boundary_out)
{
  struct expr_fragment *stack;
  struct expr_fragment fragment, left, right;
  struct Instance *context;
  const struct Expr *expr;
  int stack_size;
  int sp = 0;
  int artifact_index = 0;
  int uses_named_satisfied = 0;
  char namebuf[128];
  symchar *name;
  int result = 1;

  if (guard_dvar_out != NULL) {
    *guard_dvar_out = NULL;
  }
  if (guard_boundary_out != NULL) {
    *guard_boundary_out = NULL;
  }
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
    case e_sum:
    case e_prod:
    case e_card:
    case e_choice:
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
        uses_named_satisfied = 1;
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
      if (SatisfiedExprRValue(expr) != DBL_MAX
          && fabs(SatisfiedExprRValue(expr))
             > fabs(bnd_tolerance(right.generated_boundary->bnd))) {
        bnd_set_tolerance(
          right.generated_boundary->bnd,SatisfiedExprRValue(expr)
        );
      }
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
          name,resolver,TRUE)) {
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
  if (artifact_index == 0 && !uses_named_satisfied
      && existing_guard_dvar == NULL && guard_dvar_out == NULL) {
    expr_fragment_destroy(&fragment);
    result = 0;
    goto cleanup;
  }
  if (classifier_name(namebuf,sizeof(namebuf),"guard",
      when_mindex(when),guard_index,0)) {
    expr_fragment_destroy(&fragment);
    goto cleanup;
  }
  name = AddSymbol(namebuf);
  if (existing_guard_dvar != NULL || guard_dvar_out != NULL) {
    struct classifier_artifact *dvar_artifact = NULL;
    struct Expr *dvar_expr;

    if (existing_guard_dvar != NULL) {
      dvar_expr = classifier_existing_dvar_expr(when,existing_guard_dvar);
    } else {
      if (classifier_name(namebuf,sizeof(namebuf),"dvar",
          when_mindex(when),guard_index,0)) {
        expr_fragment_destroy(&fragment);
        goto cleanup;
      }
      if (classifier_create_boolean_artifact(sys,AddSymbol(namebuf),FALSE,
          &dvar_artifact)) {
        expr_fragment_destroy(&fragment);
        goto cleanup;
      }
      dvar_expr = CreateVarExpr(CreateSystemIdName(dvar_artifact->name));
    }
    if (dvar_expr == NULL
        || expr_fragment_append(&fragment,CreateOpExpr(e_boolean_eq))) {
      if (dvar_expr != NULL) {
        DestroyExprList(dvar_expr);
      }
      expr_fragment_destroy(&fragment);
      goto cleanup;
    }
    LinkExprs(dvar_expr,fragment.head);
    fragment.head = dvar_expr;
    result = classifier_create_logrel_artifact(sys,context,fragment.head,
        name,resolver,FALSE);
    fragment.head = NULL;
    if (result == 0 && guard_dvar_out != NULL) {
      *guard_dvar_out = dvar_artifact;
    }
    if (result == 0 && guard_boundary_out != NULL
        && fragment.generated_boundary != NULL) {
      *guard_boundary_out = fragment.generated_boundary->bnd;
    }
  } else {
    if (expr_fragment_append(&fragment,CreateTrueExpr())
        || expr_fragment_append(&fragment,CreateOpExpr(e_boolean_eq))) {
      expr_fragment_destroy(&fragment);
      goto cleanup;
    }
    result = classifier_create_logrel_artifact(sys,context,fragment.head,
        name,resolver,TRUE);
    fragment.head = NULL;
    if (result == 0 && guard_boundary_out != NULL
        && fragment.generated_boundary != NULL) {
      *guard_boundary_out = fragment.generated_boundary->bnd;
    }
  }
  expr_fragment_destroy(&fragment);

cleanup:
  while (sp > 0) {
    expr_fragment_destroy(&stack[--sp]);
  }
  ascfree(stack);
  return result;
}

static int classifier_case_if_can_reuse_boolean_selectors(
    struct w_when *when, int32 nguards)
{
  struct gl_list_t *dvars;
  struct gl_list_t *cases;
  unsigned long c, clen;
  int32 g;

  if (when == NULL || nguards <= 0) {
    return 0;
  }
  dvars = when_dvars_list(when);
  if (dvars == NULL || gl_length(dvars) != (unsigned long)nguards) {
    return 0;
  }
  for (g = 0; g < nguards; ++g) {
    struct dis_discrete *dvar =
      (struct dis_discrete *)gl_fetch(dvars,(unsigned long)g + 1);
    struct Instance *context = classifier_context_instance(when);
    struct Instance *inst = dvar != NULL ? IPTR(dis_instance(dvar)) : NULL;
    if (dvar == NULL || dis_kind(dvar) != e_dis_boolean_t) {
      return 0;
    }
    if (context == NULL || inst == NULL || !SearchForParent(inst,context)) {
      return 0;
    }
  }

  cases = when_cases_list(when);
  clen = cases != NULL ? gl_length(cases) : 0;
  for (c = 1, g = 0; c <= clen; ++c) {
    struct when_case *wc = (struct when_case *)gl_fetch(cases,c);
    int32 *values;
    int32 i, true_count = 0;

    if (wc == NULL || when_case_values_list(wc)[0] == -1) {
      break;
    }
    if (when_case_condition(wc) == NULL) {
      continue;
    }
    if (g >= nguards) {
      return 0;
    }
    values = when_case_values_list(wc);
    if (values == NULL) {
      return 0;
    }
    for (i = 0; i < nguards; ++i) {
      if (values[i] != FALSE && values[i] != TRUE) {
        return 0;
      }
      if (values[i] == TRUE) {
        ++true_count;
      }
    }
    if (values[g] != TRUE || true_count != 1) {
      return 0;
    }
    ++g;
  }
  return g == nguards;
}

static int classifier_bind_case_if_guard_dvars(struct w_when *when,
    struct dis_discrete **guard_dvars, int32 nguards,
    int use_explicit_case_values)
{
  struct gl_list_t *old_dvars;
  struct gl_list_t *new_dvars;
  struct gl_list_t *cases;
  unsigned long c, clen;
  int32 g;
  int copy_old_values = 0;

  if (when == NULL || guard_dvars == NULL || nguards <= 0) {
    return 0;
  }
  for (g = 0; g < nguards; ++g) {
    if (guard_dvars[g] == NULL) {
      return 0;
    }
  }

  old_dvars = when_dvars_list(when);
  if (old_dvars != NULL && gl_length(old_dvars) == (unsigned long)nguards) {
    copy_old_values = 1;
    for (g = 0; g < nguards; ++g) {
      struct dis_discrete *old_dvar =
        (struct dis_discrete *)gl_fetch(old_dvars,(unsigned long)g + 1);
      if (old_dvar == NULL || dis_kind(old_dvar) != e_dis_boolean_t) {
        copy_old_values = 0;
        break;
      }
      dis_set_value_from_inst(old_dvar,NULL);
    }
  }

  new_dvars = gl_create((unsigned long)nguards);
  if (new_dvars == NULL) {
    return 1;
  }
  for (g = 0; g < nguards; ++g) {
    struct gl_list_t *whens;
    struct dis_discrete *dvar = guard_dvars[g];
    if (copy_old_values) {
      struct dis_discrete *old_dvar =
        (struct dis_discrete *)gl_fetch(old_dvars,(unsigned long)g + 1);
      int32 value = dis_value(old_dvar) ? TRUE : FALSE;
      dis_set_inst_and_field_value(dvar,value);
      dis_set_previous_value(dvar,value);
    }
    dis_set_flags(dvar,dis_flags(dvar)
      | DIS_INWHEN | DIS_BVAR | DIS_BOOLEAN | DIS_INCIDENT);
    gl_append_ptr(new_dvars,dvar);
    whens = dis_whens_list(dvar);
    if (whens == NULL) {
      whens = gl_create(1);
      if (whens == NULL) {
        gl_destroy(new_dvars);
        return 1;
      }
      dis_set_whens_list(dvar,whens);
    }
    gl_append_ptr(whens,when);
  }

  cases = when_cases_list(when);
  clen = cases != NULL ? gl_length(cases) : 0;
  for (c = 1; c <= clen; ++c) {
    struct when_case *wc = (struct when_case *)gl_fetch(cases,c);
    int32 values[MAX_VAR_IN_LIST];
    int32 nvalues = 0;
    if (use_explicit_case_values && when_case_values_list(wc)[0] != -1) {
      int32 *case_values = when_case_values_list(wc);
      int32 i;
      if (case_values == NULL) {
        gl_destroy(new_dvars);
        return 1;
      }
      for (i = 0; i < nguards; ++i) {
        if (case_values[i] != FALSE && case_values[i] != TRUE) {
          gl_destroy(new_dvars);
          return 1;
        }
        values[i] = case_values[i];
      }
      nvalues = nguards;
    } else {
      if (when_case_if_pattern(when,wc,values,&nvalues)
          || nvalues != nguards) {
        gl_destroy(new_dvars);
        return 1;
      }
    }
    when_case_set_values_list(wc,values);
  }

  when_set_dvars_list(when,new_dvars);
  when_set_flagbit(when,WHEN_CLASSIFIER_GUARD_DVARS,TRUE);
  if (old_dvars != NULL) {
    gl_destroy(old_dvars);
  }
  return 0;
}

static int classifier_when_has_constant_selectors(struct w_when *when)
{
  struct gl_list_t *dvars;
  unsigned long d, dlen;

  if (when == NULL) {
    return 0;
  }
  dvars = when_dvars_list(when);
  if (dvars == NULL) {
    return 0;
  }
  dlen = gl_length(dvars);
  if (dlen == 0) {
    return 0;
  }
  for (d = 1; d <= dlen; ++d) {
    struct dis_discrete *dvar = (struct dis_discrete *)gl_fetch(dvars,d);
    if (dvar == NULL || !dis_const(dvar)) {
      return 0;
    }
  }
  return 1;
}

static int classifier_refresh_encoding_case_values(slv_system_t sys,
    struct w_when *when)
{
  unsigned long i, len;

  if (sys == NULL || when == NULL || sys->classifier_encodings == NULL) {
    return 0;
  }
  len = gl_length(sys->classifier_encodings);
  for (i = 1; i <= len; ++i) {
    struct slv_classifier_when_encoding *encoding =
      (struct slv_classifier_when_encoding *)gl_fetch(
        sys->classifier_encodings,i
      );
    int32 c;
    if (encoding == NULL || encoding->when != when) {
      continue;
    }
    for (c = 0; c < encoding->ncases; ++c) {
      struct when_case *wc = encoding->cases[c].wc;
      int32 *values = wc != NULL ? when_case_values_list(wc) : NULL;
      int32 g;
      if (values == NULL || encoding->nguards > MAX_VAR_IN_LIST) {
        return 1;
      }
      for (g = 0; g < encoding->nguards; ++g) {
        encoding->cases[c].values[g] = values[g];
      }
      encoding->cases[c].nvalues = encoding->nguards;
    }
    return 0;
  }
  return 0;
}

static int classifier_refresh_encoding_guard_bindings(slv_system_t sys,
    struct w_when *when, struct dis_discrete **guard_dvars,
    struct bnd_boundary **guard_boundaries, int32 nguards)
{
  unsigned long i, len;

  if (sys == NULL || when == NULL || sys->classifier_encodings == NULL
      || nguards < 0) {
    return 0;
  }
  len = gl_length(sys->classifier_encodings);
  for (i = 1; i <= len; ++i) {
    struct slv_classifier_when_encoding *encoding =
      (struct slv_classifier_when_encoding *)gl_fetch(
        sys->classifier_encodings,i
      );
    int32 g;

    if (encoding == NULL || encoding->when != when) {
      continue;
    }
    if (encoding->nguards != nguards) {
      return 1;
    }
    if (encoding->guard_dvars != NULL) {
      ascfree(encoding->guard_dvars);
      encoding->guard_dvars = NULL;
    }
    if (encoding->guard_boundaries != NULL) {
      ascfree(encoding->guard_boundaries);
      encoding->guard_boundaries = NULL;
    }
    if (nguards == 0) {
      return 0;
    }
    encoding->guard_dvars = ASC_NEW_ARRAY_CLEAR(
      struct dis_discrete *,nguards
    );
    encoding->guard_boundaries = ASC_NEW_ARRAY_CLEAR(
      struct bnd_boundary *,nguards
    );
    if (encoding->guard_dvars == NULL
        || encoding->guard_boundaries == NULL) {
      return 1;
    }
    for (g = 0; g < nguards; ++g) {
      encoding->guard_dvars[g] =
        guard_dvars != NULL ? guard_dvars[g] : NULL;
      encoding->guard_boundaries[g] =
        guard_boundaries != NULL ? guard_boundaries[g] : NULL;
    }
    return 0;
  }
  return 0;
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
      int bind_guard_dvars = !classifier_when_has_constant_selectors(whens[w]);
      int reuse_boolean_selectors =
        bind_guard_dvars
        && classifier_case_if_can_reuse_boolean_selectors(whens[w],nguards);
      struct gl_list_t *old_dvars = when_dvars_list(whens[w]);
      struct classifier_artifact **guard_artifacts =
        ASC_NEW_ARRAY_CLEAR(struct classifier_artifact *,nguards);
      struct dis_discrete **guard_dvars =
        ASC_NEW_ARRAY_CLEAR(struct dis_discrete *,nguards);
      struct bnd_boundary **guard_boundaries =
        ASC_NEW_ARRAY_CLEAR(struct bnd_boundary *,nguards);
      if ((guard_artifacts == NULL || guard_dvars == NULL
          || guard_boundaries == NULL) && nguards > 0) {
        if (guard_artifacts != NULL) {
          ascfree(guard_artifacts);
        }
        if (guard_dvars != NULL) {
          ascfree(guard_dvars);
        }
        if (guard_boundaries != NULL) {
          ascfree(guard_boundaries);
        }
        slv_destroy_classifier_artifacts(sys);
        return 1;
      }
      for (g = 0; g < nguards; ++g) {
        const struct Expr *guard = when_case_if_guard(whens[w],g);
        if (guard != NULL) {
          struct dis_discrete *existing_guard_dvar =
            reuse_boolean_selectors && old_dvars != NULL
              ? (struct dis_discrete *)gl_fetch(old_dvars,(unsigned long)g + 1)
              : NULL;
          if (classifier_materialize_guard(sys,whens[w],guard,g,&resolver,
              existing_guard_dvar,
              bind_guard_dvars && existing_guard_dvar == NULL
                ? &(guard_artifacts[g]) : NULL,
              &(guard_boundaries[g]))) {
            ascfree(guard_boundaries);
            ascfree(guard_dvars);
            ascfree(guard_artifacts);
            slv_destroy_classifier_artifacts(sys);
            return 1;
          }
          guard_dvars[g] = existing_guard_dvar != NULL
            ? existing_guard_dvar
            : (guard_artifacts[g] != NULL ? guard_artifacts[g]->dvar : NULL);
        }
      }
      if (bind_guard_dvars) {
        if (classifier_bind_case_if_guard_dvars(
            whens[w],guard_dvars,nguards,reuse_boolean_selectors
        )) {
          ascfree(guard_boundaries);
          ascfree(guard_dvars);
          ascfree(guard_artifacts);
          slv_destroy_classifier_artifacts(sys);
          return 1;
        }
        if (classifier_refresh_encoding_case_values(sys,whens[w])) {
          ascfree(guard_boundaries);
          ascfree(guard_dvars);
          ascfree(guard_artifacts);
          slv_destroy_classifier_artifacts(sys);
          return 1;
        }
      }
      if (classifier_refresh_encoding_guard_bindings(
          sys,whens[w],guard_dvars,guard_boundaries,nguards
      )) {
        ascfree(guard_boundaries);
        ascfree(guard_dvars);
        ascfree(guard_artifacts);
        slv_destroy_classifier_artifacts(sys);
        return 1;
      }
      ascfree(guard_boundaries);
      ascfree(guard_dvars);
      ascfree(guard_artifacts);
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
        if (classifier_materialize_guard(sys,whens[w],applies,g,&resolver,
            NULL,NULL,NULL)) {
          slv_destroy_classifier_artifacts(sys);
          return 1;
        }
        ++g;
      }
    }
  }
  return classifier_finalize_artifacts(sys);
}

static int slv_encode_classifier_when(slv_system_t sys, struct w_when *when)
{
  struct slv_classifier_when_encoding *encoding;
  struct gl_list_t *cases;
  unsigned long c, clen;
  int32 nguards;

  if (sys == NULL || when == NULL) {
    return 1;
  }
  if (when_case_if_guard_count(when,&nguards)) {
    return 0;
  }
  cases = when_cases_list(when);
  clen = cases != NULL ? gl_length(cases) : 0;
  if (clen == 0) {
    return 0;
  }

  if (sys->classifier_encodings == NULL) {
    sys->classifier_encodings = gl_create(4);
    if (sys->classifier_encodings == NULL) {
      return 1;
    }
  }

  encoding = ASC_NEW_CLEAR(struct slv_classifier_when_encoding);
  if (encoding == NULL) {
    return 1;
  }
  encoding->when = when;
  encoding->nguards = nguards;
  encoding->ncases = (int32)clen;
  encoding->cases = ASC_NEW_ARRAY_CLEAR(
    struct slv_classifier_case_encoding,encoding->ncases
  );
  if (encoding->cases == NULL) {
    classifier_encoding_free(encoding);
    return 1;
  }

  for (c = 1; c <= clen; ++c) {
    struct when_case *wc = (struct when_case *)gl_fetch(cases,c);
    struct slv_classifier_case_encoding *ce = &(encoding->cases[c - 1]);
    int32 nvalues = 0;
    ce->wc = wc;
    ce->label = classifier_case_source_label(sys,wc);
    if (when_case_if_pattern(when,wc,ce->values,&nvalues)) {
      classifier_encoding_free(encoding);
      return 1;
    }
    ce->nvalues = nvalues;
  }

  classifier_encoding_analyze_guard_logic(encoding);
  gl_append_ptr(sys->classifier_encodings,encoding);
  return 0;
}

static int slv_encode_classifier_whens(slv_system_t sys,
    enum when_region_request request)
{
  struct w_when **whens;
  int32 nwhens, w;

  if (request != WHEN_REGION_STEADY) {
    return 0;
  }
  if (sys == NULL) {
    return 1;
  }
  if (sys->classifier_encodings != NULL) {
    return 0;
  }

  whens = slv_get_master_when_list(sys);
  nwhens = slv_get_num_master_whens(sys);
  if (whens == NULL) {
    return 0;
  }

  for (w = 0; w < nwhens; ++w) {
    if (when_inwhen(whens[w])) {
      continue;
    }
    if (slv_encode_classifier_when(sys,whens[w])) {
      slv_destroy_classifier_encodings(sys);
      return 1;
    }
  }
  return 0;
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
  if (slv_encode_classifier_whens(sys,request)) {
    return 1;
  }
  if (slv_materialize_classifier_whens(sys,request)) {
    slv_destroy_classifier_encodings(sys);
    return 1;
  }
  if (classifier_install_artifact_lists(sys)) {
    slv_destroy_classifier_encodings(sys);
    return 1;
  }

  switch (request) {
  case WHEN_REGION_STEADY:
    {
      int status = reanalyze_solver_lists_with_lowered_whens(sys);
      slv_maybe_write_classifier_lowered_view(sys);
      return status;
    }
  case WHEN_REGION_DYNAMIC_CLASSIFIER:
    slv_maybe_write_classifier_lowered_view(sys);
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
