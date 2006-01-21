/*
	SLV: Ascend Nonlinear Solver
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
	This file is part of the SLV solver.
*/

#include <math.h>
#include <stdarg.h>
#include "utilities/ascConfig.h"
#include "compiler/instance_enum.h"
#include "compiler/fractions.h"
#include "compiler/compiler.h"
#include "utilities/ascMalloc.h"
#include "utilities/ascPanic.h"
#include "compiler/dimen.h"
#include "compiler/atomvalue.h"
#include "solver/mtx.h"
#include "solver/linsol.h"
#include "solver/linsolqr.h"
#include "solver/slv_types.h"
#include "solver/var.h"
#include "solver/rel.h"
#include "solver/logrel.h"
#include "solver/discrete.h"
#include "solver/conditional.h"
#include "solver/bnd.h"
#include "solver/bndman.h"
#include "solver/system.h"
#include "solver/slv_server.h"
#include "solver/slv_common.h"
#include "solver/slv_client.h"
#include "solver/analyze.h"

#define NEEDSTOBEDONE 0

/**
 ***  Include all of the solvers involved,
 ***  even if they are not linked later
 ***  Defines are to take care of the unlinked ones.
 **/
#if 0
# include "solver/slv0.h"
# include "solver/slv1.h"
# include "solver/slv2.h"
# include "solver/slv3.h"
# include "solver/slv4.h"
# include "solver/slv5.h"
# include "solver/slv6.h"
# include "solver/slv7.h"
# include "solver/slv8.h"
# include "solver/slv9.h"
#endif


struct slv_system_structure {
  int solver;

  int serial_id;
	/**< Through time, two systems may have the same pointer but never
		simultaneously. The serial_id provides a unique tag that will
		never repeat. Clients concerned with identity but not capable
		of tracking time must use the serial_id for checks. */

  SlvBackendToken instance;	/* should be void * in the most generic case */

  /* All solver handles.  sysI can't be dereferenced outside slvI.c
   * should be an array of pointers to arrays of the functions provided
   * by dynamically loaded clients, or at least by the client which this
   * system is currently supporting.
   */

  SlvClientToken ct;
  /* This is a pointer that the client returns on registration.
   * If it is not null, the registration was successful.
   * This token will be handed back to the client code on all calls
   * originating from here.
   */

  dof_t dof;                    /* non linear blocks */
  dof_t logdof;                 /* logical blocks */

  /* In the following NULL terminated lists, note that snum and mnum
   * are the lengths of the arrays WITHOUT the NULL pointer at the end.
   * Note objs is a list of relations that are objectives
   * (e_maximize,e_minimize). this list will include the first included obj.
   */
  struct {
    int snum;			/* length of the solver list */ 
    int mnum;			/* length of the master list */
    struct var_variable **solver;
    struct var_variable **master;
	struct var_variable *buf;
  } vars;

  struct {
    int snum;		        	/* length of the solver list */
    int mnum;			       /* length of the master list */
    struct dis_discrete **solver;
    struct dis_discrete **master;
	struct dis_discrete *buf;
  } dvars;

  struct {
    int snum;			/* length of the solver list */
    int mnum;			/* length of the master list */
    struct rel_relation **solver;
    struct rel_relation **master;
    struct rel_relation *buf;
  } rels;

  struct {
    int snum;
    int mnum;
    struct rel_relation **solver;
    struct rel_relation **master;
    struct rel_relation *buf;
  } objs;

  struct {
    int snum;			/* length of the solver list */
    int mnum;			/* length of the master list */
    struct rel_relation **solver;
    struct rel_relation **master;
	struct rel_relation *buf;
  } condrels;

  struct {
    int snum;			/* length of the solver list */
    int mnum;			/* length of the master list */
    struct logrel_relation **solver;
    struct logrel_relation **master;
	struct logrel_relation *buf;
  } logrels;

  struct {
    int snum;			/* length of the solver list */
    int mnum;			/* length of the master list */
    struct logrel_relation **solver;
    struct logrel_relation **master;
	struct logrel_relation *buf;
  } condlogrels;

  struct {
    int snum;			/* length of the solver list */
    int mnum;			/* length of the master list */
    struct w_when **solver;
    struct w_when **master;
	struct w_when *buf;
  } whens;

  struct {
    int snum;			/* length of the solver list */
    int mnum;			/* length of the master list */
    struct bnd_boundary **solver;
    struct bnd_boundary **master;
	struct bnd_boundary *buf;
  } bnds;

  struct {
    int snum;
    int mnum;
    struct var_variable **solver;
    struct var_variable **master;
	struct var_variable *buf;
  } pars;

  struct {
    int snum;
    int mnum;
    struct var_variable **solver;
    struct var_variable **master;
	struct var_variable *buf;
  } unattached;

  struct {
    int snum;
    int mnum;
    struct dis_discrete **solver;
    struct dis_discrete **master;
	struct dis_discrete *buf;
  } disunatt;

  /* the data that follows is for internal consumption only. */
  struct {
    int num_extrels;
    struct ExtRelCache **erlist;
  } extrels;

  struct rel_relation *obj; /* selected for optimization from list */
  struct var_variable *objvar; /* selected for optimization from list */
  struct gl_list_t *symbollist; /* list of symbol values struct used to */
                                /* assign an integer value to a symbol value */
  struct {
    struct var_variable **incidence; /* all relation incidence list memory */
    struct rel_relation **varincidence; /* all variable incidence list memory */
    struct dis_discrete **logincidence; /* all logrel incidence list memory */
    long incsize;     /* size of incidence array */
    long varincsize;  /* size of varincidence array */
    long logincsize;  /* size of discrete incidence array */
#if NEEDSTOBEDONE
/* we should be group allocating this data, but aren't */
    struct ExtRelCache *ebuf; /* data space for all extrel caches */
#endif
  } data;

  int32 nmodels;
  int32 need_consistency; /* consistency analysis required for conditional model ? */
  real64 objvargrad; /* maximize -1 minimize 1 noobjvar 0 */
};

/**
 global variable used to communicate information between solvers and
 an interface, whether a calculation should be halted or not.
 0 means go on. any other value may contain additional information
 content.
*/
int Solv_C_CheckHalt_Flag = 0;

int g_SlvNumberOfRegisteredClients; /* see header */

/** making ANSI assumption that RegisteredClients is init to 0/NULLs */
static SlvFunctionsT SlvClientsData[SLVMAXCLIENTS];

/*-----------------------------------------------------------------*/
/**
	Note about g_number_of_whens, g_number_of_dvars and g_number_of_bnds:
	These numbers are as the same as those given in the solver and master
	lists, however, these lists are destroyed before the buffers are destroyed,
	so the information is gone before I can use it.
*/

/** Global var used to destroy the cases and the gl_list inside each WHEN */
static int g_number_of_whens;

/** Global var used to destroy the list of whens in each discrete variable */
static int g_number_of_dvars;

/** Global var used to destroy the list of logical relations in each boundary */
static int g_number_of_bnds;

/*-------------------------------------------------------------------
  A bunch of annoyingly unintuitive macros that probably will save
  you from developing RSI :-)
*/

/** Return the solver index for a given slv_system_t */
#define SNUM(sys) ((sys)->solver)

/** Number of registered clients */
#define NORC g_SlvNumberOfRegisteredClients

/** Return the pointer to a registered SLV client's data space. @see SF, related.
	@param i registered solver ID 
*/
#define SCD(i) SlvClientsData[(i)]

/**	Get the solver index for a system and return TRUE if the solver
	index is in the range [0,NORC). 'sys' should not be null
	@param sys system, slv_system_t.
 */
#define LS(sys) ( (sys)->solver >= 0 && (sys)->solver < g_SlvNumberOfRegisteredClients )

/** Boolean test that i is in the range [0,NORC) */
#define LSI(i) ( (i) >= 0 && (i) < g_SlvNumberOfRegisteredClients )

/** Check and return a function pointer. See @SF */
#define CF(sys,ptr) ( LS(sys) ?  SlvClientsData[(sys)->solver].ptr : NULL )

/** Return the pointer to the client-supplied function or char if
	the client supplied one, else NULL. This should only be called
	with nonNULL sys after CF is happy. @see CF
*/
#define SF(sys,ptr) ( SlvClientsData[(sys)->solver].ptr )

/** Free a pointer provided it's not NULL */
#define SFUN(p) if ((p) != NULL) ascfree(p)

/*-----------------------------------------------------------------
	SERVER FUNCTIONS
*/

int slv_lookup_client( const char *solverName )
{
  int i;
  if (solverName == NULL) { return -1; }
  for (i = 0; i < NORC; i++) {
    if ( strcmp( SCD(i).name, solverName)==0) {
      return i;
    }
  }
  return -1;
}

/** Register a new solver.
	@TODO This needs work still, particularly of the dynamic loading
	sort. it would be good if here we farmed out the dynamic loading
	to another file so we don't have to crap this one all up.
*/
int slv_register_client(SlvRegistration registerfunc, CONST char *func
		,CONST char *file, int *new_client_id)
{
  int status;

  (void)func;  /*  stop gcc whine about unused parameter */
  (void)file;  /*  stop gcc whine about unused parameter */

  status = registerfunc(&( SlvClientsData[NORC]));
  if (!status) { /* ok */
    SlvClientsData[NORC].number = NORC;
	*new_client_id = NORC;
    NORC++;
  } else {
  	*new_client_id = -2;
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Client %d registration failure (%d)!\n",NORC,status);
  }
  return status;
}

slv_system_t slv_create(void)
{
  slv_system_t sys;
  static unsigned nextid = 1;
  sys = (slv_system_t)asccalloc(1,sizeof(struct slv_system_structure) );
  /* all lists, sizes, pointers DEFAULT to 0/NULL */
  sys->solver = -1; /* a nonregistration */
  sys->serial_id = nextid++;
  return(sys);
}

unsigned slv_serial_id(slv_system_t sys)
{
  return sys->serial_id;
}

/*----------------------------------------------------
	destructors
*/

#define DEFINE_DESTROY_BUFFER(NAME,PROP,TYPE,DESTROY) \
	static void slv_destroy_##NAME##_buffer(struct TYPE *buf){ \
		int c; struct TYPE *cur; \
		for(c=0;c<g_number_of_##PROP;c++){ \
			cur = &(buf[c]); \
			DESTROY(cur); \
		} \
		ascfree(buf); \
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
		slv_destroy_##NAME##_buffer(sys->PROP.buf); \
		sys->PROP.buf = NULL; \
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
  if (sys->ct != NULL) {
    if ( CF(sys,cdestroy) == NULL ) {
	  ERROR_REPORTER_HERE(ASC_PROG_FATAL,"slv_destroy: SlvClientToken 0x%p not freed by %s",
        sys->ct,SF(sys,name));
    } else {
      if ( SF(sys,cdestroy)(sys,sys->ct) ) {
        ret++;
      }
    }
  }
  if (ret) {
	ERROR_REPORTER_HERE(ASC_PROG_FATAL,"slv_destroy: slv_system_t 0x%p not freed.",sys);
  } else {

	SLV_FREE_BUFS(SLV_FREE_BUF, SLV_FREE_BUF_GLOBAL)

    if (sys->data.incidence != NULL) ascfree(sys->data.incidence);
    sys->data.incidence = NULL;
    if (sys->data.varincidence != NULL) ascfree(sys->data.varincidence);
    sys->data.varincidence = NULL;
    if (sys->data.logincidence != NULL) ascfree(sys->data.logincidence);
    sys->data.incidence = NULL;
    ascfree( (POINTER)sys );
  }
  return ret;
}

void slv_destroy_client(slv_system_t sys)
{

  if (sys->ct != NULL) {
    if ( CF(sys,cdestroy) == NULL ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,
		"SlvClientToken 0x%p not freed in slv_destroy_client",sys->ct);
    } else {
      if ( SF(sys,cdestroy)(sys,sys->ct) ) {
        ERROR_REPORTER_HERE(ASC_PROG_ERR,"slv_destroy_client: SlvClientToken not freed");
      } else {
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
	D(unattached,unattached,var_variable); \
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
			g_number_of_##PROP = len; \
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

/*---------------------------------------------------------------
	Macros to define
		slv_set_incidence
		slv_set_var_incidence
		slv_set_logincidence
*/

#define DEFINE_SET_INCIDENCE(NAME,PROP,TYPE,SIZE) \
	void slv_set_##NAME(slv_system_t sys, struct TYPE **inc, long s){ \
		if(sys->data.PROP != NULL || inc == NULL){ \
			Asc_Panic(2,"slv_set_" #NAME,"bad call"); \
		}else{ \
			sys->data.PROP = inc; \
			sys->data.SIZE = s; \
		} \
	}

#define DEFINE_SET_INCIDENCES(D) \
	D(incidence, incidence, var_variable, incsize) \
	D(var_incidence, varincidence, rel_relation, varincsize) \
	D(logincidence, logincidence, dis_discrete, incsize)

DEFINE_SET_INCIDENCES(DEFINE_SET_INCIDENCE)

/*-------------------------------------------------------------*/

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

const char *slv_solver_name(int index)
{
  static char errname[] = "ErrorSolver";
  if (index >= 0 && index < NORC) {
    if ( SlvClientsData[index].name == NULL ) {
      ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"slv_solver_name: unnamed solver: index='%d'",index);
      return errname;
    } else {
	  return SlvClientsData[index].name;
    }
  } else {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_solver_name: invalid solver index '%d'", index);
    return errname;
  }
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
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_set_solvers_blocks called with NULL system or bad len.\n");
  } else {
    if (len && data==NULL) {
      ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_set_solvers_blocks called with bad data.\n");
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
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_set_solvers_log_blocks called with NULL system or bad len\n");
  } else {
    if (len && data==NULL) {
      ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_set_solvers_log_blocks called with bad data.\n");
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
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_get_symbol_list called with NULL system.\n");
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

#define DEFINE_GET_SOLVERS_LIST_METHOD(NAME,PROP,TYPE) \
	struct TYPE **slv_get_solvers_##NAME##_list(slv_system_t sys){ \
		if (sys->PROP.solver == NULL) { \
			ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_get_solvers_" #NAME "_list: returning NULL (?)."); \
		} \
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
DEFINE_GETSET_LIST_METHODS(DEFINE_SET_SOLVERS_LIST_METHOD, DEFINE_SET_SOLVERS_LIST_METHOD_RETURN)

/* the slv_get_solvers_*_list methods: all have the same form so it's DEFINE...(D,D) in this case: */
DEFINE_GETSET_LIST_METHODS(DEFINE_GET_SOLVERS_LIST_METHOD, DEFINE_GET_SOLVERS_LIST_METHOD)

#define DEFINE_GET_MASTER_LIST_METHOD(NAME,PROP,TYPE) \
	struct TYPE **slv_get_master_##NAME##_list(slv_system_t sys){ \
		if (sys->PROP.master == NULL) { \
			ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_get_master_" #NAME "_list returning NULL?\n"); \
		} \
		return sys->PROP.master; \
	}

/* the slv_get_master_*_list are also all of the same form, so DEFINE...(D,D) */
DEFINE_GETSET_LIST_METHODS(DEFINE_GET_MASTER_LIST_METHOD,DEFINE_GET_MASTER_LIST_METHOD)

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

DEFINE_SLV_METHODS(DEFINE_SOLVERS_GET_NUM_METHOD)
DEFINE_SLV_METHODS(DEFINE_MASTER_GET_NUM_METHOD)

void slv_set_obj_relation(slv_system_t sys,struct rel_relation *obj)
{
  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_set_obj_relation called with NULL system.\n");
    return;
  }
  sys->obj = obj;
}

struct rel_relation *slv_get_obj_relation(slv_system_t sys)
{
  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_get_obj_relation called with NULL system.\n");
    return NULL;
  }
  return sys->obj;
}

void slv_set_obj_variable(slv_system_t sys,struct var_variable *objvar,
                          unsigned maximize)
{
  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_set_obj_variable called with NULL system.\n");
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
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_get_obj_variable called with NULL system.\n");
    return NULL;
  }
  return sys->objvar;
}

real64 slv_get_obj_variable_gradient(slv_system_t sys)
{
  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_get_obj_variable_gradient called with NULL system.\n");
    return 0.0;
  }
  return sys->objvargrad;
}


void slv_set_need_consistency(slv_system_t sys, int32 need_consistency)
{
  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_set_need_consistency called with NULL system.\n");
    return;
  }

  sys->need_consistency = need_consistency;
}


int32 slv_need_consistency(slv_system_t sys)
{
  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_need_consistency called with NULL system.\n");
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
	static int slv_count_##NAME(FILTER##_filter_t *filter, struct TYPE **list){ \
		int ret=0; \
		assert(list!=NULL); \
		while(*list!=NULL){ \
			ret += FILTER##_apply_filter(*list,filter); \
			list++; \
		} \
		return ret; \
	}

#define DEFINE_SLV_COUNT_METHODS(D) \
	D(vars,var,var_variable) \
	D(rels,rel,rel_relation) \
	D(dvars,dis,dis_discrete) \
	D(logrels,logrel,logrel_relation) \
	D(whens,when,w_when) \
	D(bnds,bnd,bnd_boundary)

DEFINE_SLV_COUNT_METHODS(DEFINE_SLV_COUNT_METHOD)

/*--------------------------------------------------------------
	Methods to define 
		slv_count_solvers_*
		slv_count_master_*
*/

/** This macro automates the declaration of the slv_count_solvers_* methods */
#define DEFINE_SLV_COUNT_SOLVER_METHOD(NAME,PROP,TYPE,COUNT) \
	int slv_count_solvers_ ## NAME ( slv_system_t sys, TYPE ##_filter_t *xxx){ \
		if(sys==NULL || sys->PROP.solver == NULL || xxx==NULL){ \
			ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_cound_solvers_" #NAME " called with NULL"); \
			return 0; \
		} \
		return slv_count_##COUNT(xxx,sys->PROP.solver); \
	}

/** This macro automates the declaration of the slv_count_master_* methods */
#define DEFINE_SLV_COUNT_MASTER_METHOD(NAME,PROP,TYPE,COUNT) \
	int slv_count_master_ ## NAME ( slv_system_t sys, TYPE ##_filter_t *xxx){ \
		if(sys==NULL || sys->PROP.master == NULL || xxx==NULL){ \
			ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_cound_master_" #NAME " called with NULL"); \
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
DEFINE_COUNT_METHODS(DEFINE_SLV_COUNT_SOLVER_METHOD)
/** Invoke the DEFINE_COUNT_METHODS macro for MASTER methods */
DEFINE_COUNT_METHODS(DEFINE_SLV_COUNT_MASTER_METHOD)

/*------------------------------------------------------*/

static void printwarning(const char * fname, slv_system_t sys)
{
  ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,
    "%s called with bad registered client (%s).",fname,
    slv_solver_name(sys->solver));
}

static void printinfo(slv_system_t sys, const char *rname)
{
  if (CF(sys,name) == NULL ) {
    ERROR_REPORTER_NOLINE(ASC_PROG_NOTE,
      "Client %s does not support function %s\n",
      slv_solver_name(sys->solver),rname);
  }
}

int slv_eligible_solver(slv_system_t sys)
{
  if ( CF(sys,celigible) == NULL ) {
    printwarning("slv_eligible_solver",sys);
    return 0;
  }
  return SF(sys,celigible)(sys);
}

int slv_select_solver(slv_system_t sys,int solver){

  int status_index;
  SlvClientDestroyF *destroy;

  if (sys ==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"slv_select_solver called with NULL system\n");
    return -1;
  }
  if ( solver >= 0 && solver < NORC ) {
    if (sys->ct != NULL && solver != sys->solver) {
      destroy = SlvClientsData[sys->solver].cdestroy;
      if(destroy!=NULL) {
        (destroy)(sys,sys->ct);
        sys->ct = NULL;
      } else {
        ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"slv_select_solver: 'cdestroy' is undefined on solver '%s' (index %d).",
          slv_solver_name(sys->solver), sys->solver);
      }
    }

    if (sys->ct != NULL) {
      return sys->solver;
    }

    status_index = solver;
    sys->solver = solver;
    if ( CF(sys,ccreate) != NULL) {
      sys->ct = SF(sys,ccreate)(sys,&status_index);
    } else {
      ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_select_solver create failed due to bad client %s\n",
        slv_solver_name(sys->solver));
      return sys->solver;
    }
    if (sys->ct==NULL) {
      ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"SlvClientCreate failed in slv_select_solver\n");
      sys->solver = -1;
    } else {
      if (status_index) {
        ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"SlvClientCreate succeeded with warning %d %s\n",
          status_index," in slv_select_solver");
      }
      /* we could do a better job explaining the client warnings... */
      sys->solver = solver;
    }
  } else {
    ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"slv_select_solver: invalid solver index '%d'.",
      solver);
    return -1;
  }
  return sys->solver;
}


int slv_switch_solver(slv_system_t sys,int solver)
{
  int status_index;

  if (sys ==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"slv_switch_solver called with NULL system\n");
    return -1;
  }
  if( solver >= 0 && solver < g_SlvNumberOfRegisteredClients ){
    status_index = solver;
    sys->solver = solver;
    if ( CF(sys,ccreate) != NULL) {
      sys->ct = SF(sys,ccreate)(sys,&status_index);
    } else {
      ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"slv_switch_solver create failed due to bad client %s\n",
         slv_solver_name(sys->solver));
      return sys->solver;
    }
    if (sys->ct==NULL) {
      ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"SlvClientCreate failed in slv_switch_solver\n");
      sys->solver = -1;
    } else {
      if (status_index) {
        ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"SlvClientCreate succeeded with warning %d %s\n",
           status_index," in slv_switch_solver");
      }
      sys->solver = solver;
    }
  } else {
    ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"slv_switch_solver called with unknown client (%d)\n",solver);
    return -1;
  }
  return sys->solver;
}

void slv_set_char_parameter(char **cp, char *newvalue)
{
  if (cp != NULL) {
    if (*cp != NULL) {
      ascfree(*cp);
    }
    *cp = ascstrdup(newvalue);
  }
}

void slv_destroy_parms(slv_parameters_t *p) {
  int32 i,j;
  for (i = 0; i < p->num_parms; i++) {
    switch(p->parms[i].type) {
    case char_parm:
      ascfree(p->parms[i].info.c.value);
      for (j = 0; j < p->parms[i].info.c.high; j++) {
	ascfree(p->parms[i].info.c.argv[j]);
      }
      ascfree(p->parms[i].info.c.argv);
      /* FALL THROUGH */
    case int_parm:
    case bool_parm:
    case real_parm:
      ascfree(p->parms[i].name);
      ascfree(p->parms[i].interface_label);
      ascfree(p->parms[i].description);
      break;
    default:
      ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"Unrecognized parameter type in slv_destroy_parms\n");
    }
  }
  if (p->parms && p->dynamic_parms) {
    ascfree(p->parms);
  }
}

int32 slv_define_parm(slv_parameters_t *p,
		   enum parm_type type,
		   char *name,
		   char *interface_label,
		   char *description,
		   union parm_arg value,
		   union parm_arg low,
		   union parm_arg high,
		   int32 display)
{
  int32 len,length,i, err=1;
  if (p == NULL) {
    return -1;
  }
  length = p->num_parms;

  switch (type) {
  case int_parm:
    err = 0;
    p->parms[length].info.i.value = value.argi;
    p->parms[length].info.i.low = low.argi;
    p->parms[length].info.i.high = high.argi;
    break;

  case bool_parm:
    err = 0;
    p->parms[length].info.b.value = value.argb;
    p->parms[length].info.b.low = low.argb;
    p->parms[length].info.b.high = high.argb;
    break;

  case real_parm:
    err = 0;
    p->parms[length].info.r.value = value.argr;
    p->parms[length].info.r.low = low.argr;
    p->parms[length].info.r.high = high.argr;
    break;

  case char_parm:
    err = 0;
    p->parms[length].info.c.argv =
      (char **)ascmalloc(high.argi*sizeof(char *));
    for (i = 0; i < high.argi; i++) {
      len = strlen(low.argv[i]);
      p->parms[length].info.c.argv[i] =(char *)ascmalloc(len+1*sizeof(char));
      strcpy(p->parms[length].info.c.argv[i],low.argv[i]);
    }

    p->parms[length].info.c.value =
      (char *)ascmalloc(strlen(value.argc)+1*sizeof(char));
    strcpy(p->parms[length].info.c.value,value.argc);

    p->parms[length].info.c.high = high.argi;
    break;

  default:
    return -1;
  }
  if (!err) {
    p->parms[length].type = type;
    p->parms[length].number = length;

    len = strlen(name);
    p->parms[length].name = (char *)ascmalloc(len+1*sizeof(char));
    strcpy(p->parms[length].name,name);

    len = strlen(interface_label);
    p->parms[length].interface_label = (char *)ascmalloc(len+1*sizeof(char));
    strcpy(p->parms[length].interface_label,interface_label);

    len = strlen(description);
    p->parms[length].description = (char *)ascmalloc(len+1*sizeof(char));
    strcpy(p->parms[length].description,description);

    p->parms[length].display = display;
  } else {
    p->parms[length].type = -1;
  }
  p->num_parms++;
  return p->num_parms;
}

int slv_get_selected_solver(slv_system_t sys)
{
  if (sys!=NULL) return sys->solver;
  return -1;
}

int32 slv_get_default_parameters(int index,
				slv_parameters_t *parameters)
{
  if (index >= 0 && index < NORC) {
    if ( SlvClientsData[index].getdefparam == NULL ) {
      ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_get_default_parameters called with parameterless index\n");
      return 0;
    } else {
      /* send NULL system when setting up interface */
      (SlvClientsData[index].getdefparam)(NULL,NULL,parameters);
      return 1;
    }
  } else {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_get_default_parameters called with unregistered index\n");
    return 0;
  }
}

/*-----------------------------------------------------------
	These macros do some more elimination of repetition. Here we're
	trying to replace some more complex 'method-like' calls on
	slv_system_t:

	These macros use macro-argument-concatenation and macro stringification.
	Verified that the former works with Visual C++:
		getlinso://www.codeproject.com/macro/metamacros.asp
*/

/** Define a method like 'void slv_METHODNAME(sys)' */
#define DEFINE_SLV_PROXY_METHOD_VOID(METHOD) \
	void slv_ ## METHOD (slv_system_t sys){ \
		if(CF(sys,METHOD)==NULL){ \
			printwarning(#METHOD,sys); \
			return; \
		} \
		SF(sys,METHOD)(sys,sys->ct); \
	}

/** Define a method like 'RETURNTYPE slv_METHOD(sys)'; */
#define DEFINE_SLV_PROXY_METHOD(METHOD,PROP,RETTYPE) \
	RETTYPE slv_ ## METHOD (slv_system_t sys){ \
		if(CF(sys,PROP)==NULL){ \
			printwarning(#METHOD,sys); \
			return; \
		} \
		return SF(sys,PROP)(sys,sys->ct); \
	}

/** Define a method like 'void slv_METHOD(sys,TYPE PARAMNAME)'; */
#define DEFINE_SLV_PROXY_METHOD_PARAM(METHOD,PROP,PARAMTYPE,PARAMNAME) \
	void slv_ ## METHOD (slv_system_t sys, PARAMTYPE PARAMNAME){ \
		if(CF(sys,PROP)==NULL){ \
			printwarning(#METHOD,sys); \
			return; \
		} \
		SF(sys,PROP)(sys,sys->ct, PARAMNAME); \
	}

DEFINE_SLV_PROXY_METHOD_PARAM(get_parameters,getparam,slv_parameters_t*,parameters);

void slv_set_parameters(slv_system_t sys,slv_parameters_t *parameters)
{
  if ( CF(sys,setparam) == NULL ) {
    printwarning("slv_set_parameters",sys);
    return;
  }
  if (parameters->whose != sys->solver) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,
		"slv_set_parameters can give parameters from one client to a different client.");
    return;
  }
  SF(sys,setparam)(sys,sys->ct,parameters);
}

DEFINE_SLV_PROXY_METHOD_PARAM(get_status,getstatus,slv_status_t*,status);
DEFINE_SLV_PROXY_METHOD(get_linsolv_sys, getlinsol, linsol_system_t);
DEFINE_SLV_PROXY_METHOD(get_sys_mtx, getsysmtx, mtx_matrix_t); 
DEFINE_SLV_PROXY_METHOD(get_linsolqr_sys, getlinsys, linsolqr_system_t); 
DEFINE_SLV_PROXY_METHOD_PARAM(dump_internals,dumpinternals,int,level);
DEFINE_SLV_PROXY_METHOD_VOID(presolve);
DEFINE_SLV_PROXY_METHOD_VOID(resolve);
DEFINE_SLV_PROXY_METHOD_VOID(iterate);
DEFINE_SLV_PROXY_METHOD_VOID(solve);

/*-----------------------------------------------------------*/

SlvClientToken slv_get_client_token(slv_system_t sys)
{
  if (sys==NULL) {
    FPRINTF(stderr,"slv_get_client_token called with NULL system.\n");
    return NULL;
  }
  return sys->ct;
}


void slv_set_client_token(slv_system_t sys, SlvClientToken ct)
{
  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_set_client_token called with NULL system.\n");
    return;
  }
  sys->ct = ct;
}

void slv_set_solver_index(slv_system_t sys, int solver)
{
  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_set_solver_index called with NULL system.\n");
    return;
  }
  sys->solver = solver;
}

/*********************************************************************\
  unregistered client functions that need to go elsewhere(other files).
  hereunder are utility calls which are unstandardized
\*********************************************************************/

boolean slv_change_basis(slv_system_t sys, int32 var, mtx_range_t *rng)
{
  (void)sys;
  (void)var;
  (void)rng;
  Asc_Panic(2, "slv_change_basis", "fix me");
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
  int32 len,i, *va, index;
  real64 comp;
  static var_filter_t vfilter;
  struct var_variable **vlist=NULL;
  len = slv_get_num_solvers_vars(sys);
  va = *vip = (int32 *)ascmalloc((2*len+2)*sizeof(int32 *));
  vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
  vlist = slv_get_solvers_var_list(sys);
  va[0] = va[1] = 0;
  index = 2;
  for (i = 0; i < len; i++) {
    if (var_apply_filter(vlist[i],&vfilter)) {
      comp = (var_value(vlist[i]) - var_lower_bound(vlist[i]))
	/ var_nominal(vlist[i]);
      if (comp < epsilon) {
	va[index] = i;
	index++;
	va[0]++;
      }
    }
  }
  for (i = 0; i < len; i++) {
    if (var_apply_filter(vlist[i],&vfilter)) {
      comp = (var_upper_bound(vlist[i]) - var_value(vlist[i]))
	/ var_nominal(vlist[i]);
      if (comp < epsilon) {
	va[index] = i;
	index++;
	va[1]++;
      }
    }
  }
  return index - 2;
}

int32 slv_far_from_nominals(slv_system_t sys,real64 bignum,
		       int32 **vip)
{
  int32 len,i, *va, index;
  real64 comp;
  static var_filter_t vfilter;
  struct var_variable **vlist=NULL;
  len = slv_get_num_solvers_vars(sys);
  va = *vip = (int32 *)ascmalloc((len+1)*sizeof(int32 *));
  vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
  vlist = slv_get_solvers_var_list(sys);
  index = 0;
  for (i = 0; i < len; i++) {
    if (var_apply_filter(vlist[i],&vfilter)) {
      comp = fabs(var_value(vlist[i]) - var_nominal(vlist[i]))
	/ var_nominal(vlist[i]);
      if (comp > bignum) {
	va[index] = i;
	index++;
      }
    }
  }
  return index;
}

