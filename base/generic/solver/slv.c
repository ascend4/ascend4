/*
	SLV: Ascend Nonlinear Solver
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
	Copyright (C) 1996 Benjamin Andrew Allan
	Copyright (C) 2005 The ASCEND developers

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
  } vars;

  struct {
    int snum;		        	/* length of the solver list */
    int mnum;			       /* length of the master list */
    struct dis_discrete **solver;
    struct dis_discrete **master;
  } dvars;

  struct {
    int snum;			/* length of the solver list */
    int mnum;			/* length of the master list */
    struct rel_relation **solver;
    struct rel_relation **master;
  } rels;

  struct {
    int snum;
    int mnum;
    struct rel_relation **solver;
    struct rel_relation **master;
  } objs;

  struct {
    int snum;			/* length of the solver list */
    int mnum;			/* length of the master list */
    struct rel_relation **solver;
    struct rel_relation **master;
  } condrels;

  struct {
    int snum;			/* length of the solver list */
    int mnum;			/* length of the master list */
    struct logrel_relation **solver;
    struct logrel_relation **master;
  } logrels;

  struct {
    int snum;			/* length of the solver list */
    int mnum;			/* length of the master list */
    struct logrel_relation **solver;
    struct logrel_relation **master;
  } condlogrels;

  struct {
    int snum;			/* length of the solver list */
    int mnum;			/* length of the master list */
    struct w_when **solver;
    struct w_when **master;
  } whens;

  struct {
    int snum;			/* length of the solver list */
    int mnum;			/* length of the master list */
    struct bnd_boundary **solver;
    struct bnd_boundary **master;
  } bnds;

  struct {
    int snum;
    int mnum;
    struct var_variable **solver;
    struct var_variable **master;
  } pars;

  struct {
    int snum;
    int mnum;
    struct var_variable **solver;
    struct var_variable **master;
  } unattached;

  struct {
    int snum;
    int mnum;
    struct dis_discrete **solver;
    struct dis_discrete **master;
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
    struct var_variable *ubuf; /* data space for unclassified real ATOMs */
    struct dis_discrete *udbuf; /* data space for unclassified discrete ATOM */
    struct var_variable *pbuf; /* data space for real ATOMs that are pars */
    struct var_variable *vbuf; /* data space for real ATOMs that are vars */
    struct dis_discrete *dbuf; /* data space for discrete ATOMs that are vars*/
    struct rel_relation *rbuf; /* data space for real rel constraints */
    struct rel_relation *cbuf; /* data space for conditional rel */
    struct rel_relation *obuf; /* data space for real relation objectives */
    struct logrel_relation *lbuf; /* data space for logical rel  */
    struct logrel_relation *clbuf; /* data space for conditional logical rel*/
    struct w_when *wbuf;          /* data space for whens */
    struct bnd_boundary *bbuf;    /* data space for boundaries */
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
  int32 need_consistency; /* 
			   * consistency analysis required for conditional
			   * model ?
			   */
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

/** leave it in here John. Unilateral 'housecleaning' of this sort is not appreciated. */
#define NORC g_SlvNumberOfRegisteredClients

/** leave it in here John. Unilateral 'housecleaning' of this sort is not appreciated. */
#define SCD(i) SlvClientsData[(i)]

/**	Get the solver index for a system and return TRUE if the solver 
	index is in the range [0,NORC). 'sys' should not be null
	@param sys system, slv_system_t.
 */
#define LS(sys) ( SNUM(sys) >= 0 && SNUM(sys) < g_SlvNumberOfRegisteredClients )

/** Boolean test that i is in the range [0,NORC) */
#define LSI(i) ( (i) >= 0 && (i) < g_SlvNumberOfRegisteredClients )

/** Check and return a function pointer. See @SF */
#define CF(sys,ptr) ( LS(sys) ?  SlvClientsData[SNUM(sys)].ptr : NULL )

/** Return the pointer to the client-supplied function or char if 
	the client supplied one, else NULL. This should only be called 
	with nonNULL sys after CF is happy. @see CF 
*/
#define SF(sys,ptr) ( SlvClientsData[SNUM(sys)].ptr )

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
	new_client_id = NORC;
    NORC++;
  } else {
    FPRINTF(stderr,"Client %d registration failure (%d)!\n",NORC,status);
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

static
void slv_destroy_dvar_buffer(struct dis_discrete *dbuf)
{
  int c;
  struct dis_discrete *cur_dis;
  for (c=0;c<g_number_of_dvars;c++){
    cur_dis = &(dbuf[c]);
    dis_destroy(cur_dis);
  }
  ascfree(dbuf);
}

static
void slv_destroy_when_buffer(struct w_when *wbuf)
{
  int c;
  struct w_when *cur_when;
  for (c=0;c<g_number_of_whens;c++){
    cur_when = &(wbuf[c]);
    when_destroy(cur_when);
  }
  ascfree(wbuf);
}

static
void slv_destroy_bnd_buffer(struct bnd_boundary *bbuf)
{
  int c;
  struct bnd_boundary *cur_bnd;
  for (c=0;c<g_number_of_bnds;c++){
    cur_bnd = &(bbuf[c]);
    bnd_destroy(cur_bnd);
  }
  ascfree(bbuf);
}

int slv_destroy(slv_system_t sys)
{
  int ret = 0;
  if (sys->ct != NULL) {
    if ( CF(sys,cdestroy) == NULL ) {
	  error_reporter(ASC_PROG_FATAL,__FILE__,__LINE__,"slv_destroy: SlvClientToken 0x%p not freed by %s",
        sys->ct,SF(sys,name));
    } else {
      if ( SF(sys,cdestroy)(sys,sys->ct) ) {
        ret++;
      }
    }
  }
  if (ret) {
	error_reporter(ASC_PROG_FATAL,__FILE__,__LINE__,"slv_destroy: slv_system_t 0x%p not freed.",sys);
  } else {
    if (sys->data.ubuf != NULL) ascfree(sys->data.ubuf);
    sys->data.ubuf = NULL;
    if (sys->data.udbuf != NULL) ascfree(sys->data.udbuf);
    sys->data.udbuf = NULL;
    if (sys->data.pbuf != NULL) ascfree(sys->data.pbuf);
    sys->data.pbuf = NULL;
    if (sys->data.vbuf != NULL) ascfree(sys->data.vbuf);
    sys->data.vbuf = NULL;
    if (sys->data.dbuf != NULL) {
      slv_destroy_dvar_buffer(sys->data.dbuf);
      sys->data.dbuf = NULL;
    }
    if (sys->data.rbuf != NULL) ascfree(sys->data.rbuf);
    sys->data.rbuf = NULL;
    if (sys->data.cbuf != NULL) ascfree(sys->data.cbuf);
    sys->data.cbuf = NULL;
    if (sys->data.obuf != NULL) ascfree(sys->data.obuf);
    sys->data.obuf = NULL;
    if (sys->data.lbuf != NULL) ascfree(sys->data.lbuf);
    sys->data.lbuf = NULL;
    if (sys->data.clbuf != NULL) ascfree(sys->data.clbuf);
    sys->data.clbuf = NULL;
    if (sys->data.wbuf != NULL) {
      slv_destroy_when_buffer(sys->data.wbuf);
      sys->data.wbuf = NULL;
    }
    if (sys->data.bbuf != NULL) {
      slv_destroy_bnd_buffer(sys->data.bbuf);
      sys->data.bbuf = NULL;
    }
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
      error_reporter(ASC_PROG_ERR,__FILE__,__LINE__,
		"SlvClientToken 0x%p not freed in slv_destroy_client",sys->ct);
    } else {
      if ( SF(sys,cdestroy)(sys,sys->ct) ) {
        error_reporter(ASC_PROG_ERR,__FILE__,__LINE__,"slv_destroy_client: SlvClientToken not freed");
      } else {
	sys->ct = NULL;
      }
    }
  }
}


SlvBackendToken slv_instance(slv_system_t sys)
{
  if (sys == NULL) {
    error_reporter(ASC_PROG_ERR,__FILE__,__LINE__,"slv_instance: called with NULL system.");
    return NULL;
  } else {
    return sys->instance;
  }
}

void slv_set_instance(slv_system_t sys,SlvBackendToken instance)
{
  if (sys == NULL) {
    error_reporter(ASC_PROG_ERR,__FILE__,__LINE__,"slv_set_instance: called with NULL system.");
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
    error_reporter(ASC_PROG_ERR,__FILE__,__LINE__,"slv_get_num_models: called with NULL system.");
    return 0;
  } else {
    return sys->nmodels;
  }
}
void slv_set_num_models(slv_system_t sys, int32 nmod)
{
  if (sys == NULL) {
    error_reporter(ASC_PROG_ERR,__FILE__,__LINE__,"slv_set_num_models: called with NULL system.");
  } else {
    sys->nmodels = nmod;
  }
}

void slv_set_master_var_list(slv_system_t sys,
                             struct var_variable **vlist, int size)
{
  SFUN(sys->vars.master);
  sys->vars.mnum = size;
  sys->vars.master = vlist;
}

void slv_set_master_par_list(slv_system_t sys,
                             struct var_variable **vlist, int size)
{
  SFUN(sys->pars.master);
  sys->pars.mnum = size;
  sys->pars.master = vlist;
}

void slv_set_master_unattached_list(slv_system_t sys,
                             struct var_variable **vlist, int size)
{
  SFUN(sys->unattached.master);
  sys->unattached.mnum = size;
  sys->unattached.master = vlist;
}

void slv_set_master_dvar_list(slv_system_t sys,
                              struct dis_discrete **dlist, int size)
{
  SFUN(sys->dvars.master);
  sys->dvars.mnum = size;
  sys->dvars.master = dlist;
}

void slv_set_master_disunatt_list(slv_system_t sys,
                                  struct dis_discrete **dlist, int size)
{
  SFUN(sys->disunatt.master);
  sys->disunatt.mnum = size;
  sys->disunatt.master = dlist;
}

void slv_set_master_rel_list(slv_system_t sys,struct rel_relation **rlist,
		      int size)
{
  SFUN(sys->rels.master);
  sys->rels.mnum = size;
  sys->rels.master = rlist;
}


void slv_set_master_condrel_list(slv_system_t sys,struct rel_relation **rlist,
		                 int size)
{
  SFUN(sys->condrels.master);
  sys->condrels.mnum = size;
  sys->condrels.master = rlist;
}

void slv_set_master_obj_list(slv_system_t sys,struct rel_relation **rlist,
		             int size)
{
  SFUN(sys->objs.master);
  sys->objs.mnum = size;
  sys->objs.master = rlist;
}

void slv_set_master_logrel_list(slv_system_t sys,
				struct logrel_relation **lrlist,
		                int size)
{
  SFUN(sys->logrels.master);
  sys->logrels.mnum = size;
  sys->logrels.master = lrlist;
}

void slv_set_master_condlogrel_list(slv_system_t sys,
				struct logrel_relation **lrlist,
		                int size)
{
  SFUN(sys->condlogrels.master);
  sys->condlogrels.mnum = size;
  sys->condlogrels.master = lrlist;
}

void slv_set_master_when_list(slv_system_t sys,
			      struct w_when **wlist,
		              int size)
{
  SFUN(sys->whens.master);
  sys->whens.mnum = size;
  sys->whens.master = wlist;
}

void slv_set_master_bnd_list(slv_system_t sys,
			     struct bnd_boundary **blist,
		             int size)
{
  SFUN(sys->bnds.master);
  sys->bnds.mnum = size;
  sys->bnds.master = blist;
}

void slv_set_symbol_list(slv_system_t sys,
			 struct gl_list_t *sv)
{
  if (sys->symbollist != NULL) {
    DestroySymbolValuesList(sys->symbollist);
  }
  sys->symbollist = sv;
}

void slv_set_var_buf(slv_system_t sys, struct var_variable *vbuf)
{
  if (sys->data.vbuf !=NULL ) {
    Asc_Panic(2,"slv_set_var_buf",
              "bad call.");
  } else {
    sys->data.vbuf = vbuf;
  }
}


void slv_set_par_buf(slv_system_t sys, struct var_variable *pbuf)
{
  if (sys->data.pbuf !=NULL ) {
    Asc_Panic(2,"slv_set_par_buf",
              "bad call.");
  } else {
    sys->data.pbuf = pbuf;
  }
}

void slv_set_unattached_buf(slv_system_t sys, struct var_variable *ubuf)
{
  if (sys->data.ubuf !=NULL ) {
    Asc_Panic(2,"slv_set_unattached_buf",
              "bad call.");
  } else {
    sys->data.ubuf = ubuf;
  }
}

void slv_set_dvar_buf(slv_system_t sys, struct dis_discrete *dbuf, int len)
{
  if (sys->data.dbuf !=NULL ) {
    Asc_Panic(2,"slv_set_dvar_buf",
              "bad call.");
  } else {
    sys->data.dbuf = dbuf;
    g_number_of_dvars = len;
  }
}


void slv_set_disunatt_buf(slv_system_t sys, struct dis_discrete *udbuf)
{
  if (sys->data.udbuf !=NULL ) {
    Asc_Panic(2,"slv_set_disunatt_buf",
              "bad call.");
  } else {
    sys->data.udbuf = udbuf;
  }
}

void slv_set_rel_buf(slv_system_t sys, struct rel_relation *rbuf)
{
  if (sys->data.rbuf !=NULL ) {
    Asc_Panic(2,"slv_set_rel_buf",
              "bad call.");
  } else {
    sys->data.rbuf = rbuf;
  }
}


void slv_set_condrel_buf(slv_system_t sys, struct rel_relation *cbuf)
{
  if (sys->data.cbuf !=NULL ) {
    Asc_Panic(2,"slv_set_condrel_buf",
              "bad call.");
  } else {
    sys->data.cbuf = cbuf;
  }
}

void slv_set_obj_buf(slv_system_t sys, struct rel_relation *obuf)
{
  if (sys->data.obuf !=NULL ) {
    Asc_Panic(2,"slv_set_obj_buf",
              "bad call.");
  } else {
    sys->data.obuf = obuf;
  }
}

void slv_set_logrel_buf(slv_system_t sys, struct logrel_relation *lbuf)
{
  if (sys->data.lbuf !=NULL ) {
    Asc_Panic(2,"slv_set_logrel_buf",
              "bad call.");
  } else {
    sys->data.lbuf = lbuf;
  }
}


void slv_set_condlogrel_buf(slv_system_t sys, struct logrel_relation *clbuf)
{
  if (sys->data.clbuf !=NULL ) {
    Asc_Panic(2,"slv_set_condlogrel_buf",
              "bad call.");
  } else {
    sys->data.clbuf = clbuf;
  }
}

void slv_set_when_buf(slv_system_t sys, struct w_when *wbuf, int len)
{
  if (sys->data.wbuf !=NULL ) {
    Asc_Panic(2,"slv_set_when_buf","bad call.");
  } else {
    sys->data.wbuf = wbuf;
    g_number_of_whens = len;
  }
}

void slv_set_bnd_buf(slv_system_t sys, struct bnd_boundary *bbuf, int len)
{
  if (sys->data.bbuf !=NULL ) {
    Asc_Panic(2,"slv_set_bnd_buf",
              "bad call.");
  } else {
    sys->data.bbuf = bbuf;
    g_number_of_bnds = len;
  }
}

void slv_set_incidence(slv_system_t sys, struct var_variable **incidence,long s)
{
  if (sys->data.incidence !=NULL || incidence == NULL) {
    Asc_Panic(2,"slv_set_incidence",
              "bad call.");
  } else {
    sys->data.incidence = incidence;
    sys->data.incsize = s;
  }
}

void slv_set_var_incidence(slv_system_t sys, struct rel_relation **varincidence,long s)
{
  if (sys->data.varincidence !=NULL || varincidence == NULL) {
    Asc_Panic(2,"slv_set_varincidence",
              "bad call.");
  } else {
    sys->data.varincidence = varincidence;
    sys->data.varincsize = s;
  }
}

void slv_set_logincidence(slv_system_t sys, struct dis_discrete **logincidence,
			  long s)
{
  if (sys->data.logincidence !=NULL) {
    Asc_Panic(2,"slv_set_logincidence","bad call.");
  } else {
    sys->data.logincidence = logincidence;
    sys->data.incsize = s;
  }
}

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
      error_reporter(ASC_PROG_WARNING,NULL,0,"slv_solver_name: unnamed solver: index='%d'",index);
      return errname;
    } else {
	  return SlvClientsData[index].name;
    }
  } else {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_solver_name: invalid solver index '%d'", index);
    return errname;
  }
}

const mtx_block_t *slv_get_solvers_blocks(slv_system_t sys)
{
  if (sys == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_solvers_blocks called with NULL system");
    return NULL;
  } else {
    return &(sys->dof.blocks);
  }
}

const mtx_block_t *slv_get_solvers_log_blocks(slv_system_t sys)
{
  if (sys == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_solvers_log_blocks called with NULL system");
    return NULL;
  } else {
    return &(sys->logdof.blocks);
  }
}

void slv_set_solvers_blocks(slv_system_t sys,int len, mtx_region_t *data)
{
  if (sys == NULL || len < 0) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_set_solvers_blocks called with NULL system or bad len.\n");
  } else {
    if (len && data==NULL) {
      error_reporter(ASC_PROG_ERROR,NULL,0,"slv_set_solvers_blocks called with bad data.\n");
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
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_set_solvers_log_blocks called with NULL system or bad len\n");
  } else {
    if (len && data==NULL) {
      error_reporter(ASC_PROG_ERROR,NULL,0,"slv_set_solvers_log_blocks called with bad data.\n");
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


void slv_set_solvers_var_list(slv_system_t sys,
                              struct var_variable **vlist, int size)
{
  if (sys->vars.master == NULL) {
	error_reporter(ASC_PROG_ERR,NULL,0,"slv_set_solvers_var_list: called before slv_set_master_var_list.");
    return; /* must be error */
  }
  sys->vars.snum = size;
  sys->vars.solver = vlist;
}


void slv_set_solvers_par_list(slv_system_t sys,
                              struct var_variable **vlist, int size)
{
  if (sys->pars.master == NULL ) {
    error_reporter(ASC_PROG_WARNING,NULL,0,"slv_set_solvers_par_list: called before slv_set_master_par_list.");
  } /* might be ok */
  sys->pars.snum = size;
  sys->pars.solver = vlist;
}

void slv_set_solvers_unattached_list(slv_system_t sys,
                                     struct var_variable **vlist, int size)
{
  if (sys->unattached.master == NULL) {
    error_reporter(ASC_PROG_WARNING,NULL,0,"slv_set_solvers_unattached_list: called before slv_set_master_unattached_list.");
  } /* might be ok */
  sys->unattached.snum = size;
  sys->unattached.solver = vlist;
}

void slv_set_solvers_dvar_list(slv_system_t sys,
                              struct dis_discrete **dlist, int size)
{
  if (sys->dvars.master == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_set_solvers_dvar_list: called before slv_set_master_dvar_list.");
    return; /* must be error */
  }
  sys->dvars.snum = size;
  sys->dvars.solver = dlist;
}

void slv_set_solvers_disunatt_list(slv_system_t sys,
                                   struct dis_discrete **dlist, int size)
{
  if (sys->disunatt.master == NULL) {
    error_reporter(ASC_PROG_WARNING,NULL,0,"slv_set_solvers_disunatt_list: called before slv_set_master_disunatt_list.");
  } /* might be ok */
  sys->disunatt.snum = size;
  sys->disunatt.solver = dlist;
}

void slv_set_solvers_rel_list(slv_system_t sys,
                              struct rel_relation **rlist, int size)
{
  /* Give relation list to the system itself. */
  if (sys->rels.master == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_set_solvers_rel_list: called before slv_set_master_rel_list.");
    return; /* can't be right */
  }
  sys->rels.snum = size;
  sys->rels.solver = rlist;
}


void slv_set_solvers_obj_list(slv_system_t sys,
                              struct rel_relation **rlist, int size)
{
  /* Give relation list to the system itself. */
  if (sys->objs.master == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_set_solvers_obj_list: called before slv_set_master_rel_list.");
    return;
  }
  sys->objs.snum = size;
  sys->objs.solver = rlist;
}

void slv_set_solvers_condrel_list(slv_system_t sys,
                              struct rel_relation **rlist, int size)
{
  /* Give relation list to the system itself. */
  if (sys->condrels.master == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_set_solvers_condrel_list: called before slv_set_master_condrel_list");
    return;
  }
  sys->condrels.snum = size;
  sys->condrels.solver = rlist;
}


void slv_set_solvers_logrel_list(slv_system_t sys,
                                 struct logrel_relation **lrlist, int size)
{
  /* Give logrelation list to the system itself. */
  if (sys->logrels.master == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_set_solvers_logrel_list: called before slv_set_master_logrel_list.");
    return; /* can't be right */
  }
  sys->logrels.snum = size;
  sys->logrels.solver = lrlist;
}

void slv_set_solvers_condlogrel_list(slv_system_t sys,
                                     struct logrel_relation **lrlist, int size)
{
  /* Give logrelation list to the system itself. */
  if (sys->condlogrels.master == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,
		"slv_set_solvers_condlogrel_list: called before slv_set_master_logrel_list.");
    return; /* can't be right */
  }
  sys->condlogrels.snum = size;
  sys->condlogrels.solver = lrlist;
}

void slv_set_solvers_when_list(slv_system_t sys,
                               struct w_when **wlist, int size)
{
  if (sys->whens.master == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_set_solvers_when_list: called before slv_set_master_when_list.");
    return;
  }
  sys->whens.snum = size;
  sys->whens.solver = wlist;
}

void slv_set_solvers_bnd_list(slv_system_t sys,
                              struct bnd_boundary **blist, int size)
{
  if (sys->bnds.master == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_set_solvers_bnd_list: called before slv_set_master_bnd_list.");
    return;
  }
  sys->bnds.snum = size;
  sys->bnds.solver = blist;
}

struct var_variable **slv_get_solvers_var_list(slv_system_t sys)
{
  if (sys->vars.solver == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_solvers_var_list: returning NULL (?).");
  }
  return sys->vars.solver;
}

struct var_variable **slv_get_solvers_par_list(slv_system_t sys)
{
  if (sys->pars.solver == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_solvers_par_list: returning NULL (?).");
  }
  return sys->pars.solver;
}

struct var_variable **slv_get_solvers_unattached_list(slv_system_t sys)
{
  if (sys->unattached.solver == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_solvers_unattached_list: returning NULL?\n");
  }
  return sys->unattached.solver;
}

struct dis_discrete **slv_get_solvers_dvar_list(slv_system_t sys)
{
  if (sys->dvars.solver == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"dvar_list is NULL\n");
  }
  return sys->dvars.solver;
}

struct dis_discrete **slv_get_solvers_disunatt_list(slv_system_t sys)
{
  if (sys->disunatt.solver == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_solvers_disunatt_list returning NULL?\n");
  }
  return sys->disunatt.solver;
}

struct var_variable **slv_get_master_var_list(slv_system_t sys)
{
  if (sys->vars.master == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_master_var_list returning NULL?\n");
  }
  return sys->vars.master;
}


struct var_variable **slv_get_master_par_list(slv_system_t sys)
{
  if (sys->pars.master == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_master_par_list returning NULL?\n");
  }
  return sys->pars.master;
}

struct var_variable **slv_get_master_unattached_list(slv_system_t sys)
{
  if (sys->unattached.master == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_solvers_unattached_list returning NULL?\n");
  }
  return sys->unattached.master;
}

struct dis_discrete **slv_get_master_dvar_list(slv_system_t sys)
{
  if (sys->dvars.master == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"dvar_list is NULL\n");
  }
  return sys->dvars.master;
}

struct dis_discrete **slv_get_master_disunatt_list(slv_system_t sys)
{
  if (sys->disunatt.master == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_solvers_disunatt_list returning NULL?\n");
  }
  return sys->disunatt.master;
}

struct rel_relation **slv_get_solvers_rel_list(slv_system_t sys)
{
  if (sys->rels.solver == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_solvers_rel_list returning NULL?\n");
  }
  return sys->rels.solver;
}

struct rel_relation **slv_get_solvers_condrel_list(slv_system_t sys)
{
  if (sys->condrels.solver == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"condrel_list is NULL?\n");
  }
  return sys->condrels.solver;
}

struct rel_relation **slv_get_solvers_obj_list(slv_system_t sys)
{
  if (sys->objs.solver == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_solvers_obj_list returning NULL?\n");
  }
  return sys->objs.solver;
}

struct logrel_relation **slv_get_solvers_logrel_list(slv_system_t sys)
{
  if (sys->logrels.solver == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"logrel_list is NULL\n");
  }
  return sys->logrels.solver;
}

struct logrel_relation **slv_get_solvers_condlogrel_list(slv_system_t sys)
{
  if (sys->condlogrels.solver == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"logrel_list is NULL\n");
  }
  return sys->condlogrels.solver;
}

struct w_when **slv_get_solvers_when_list(slv_system_t sys)
{
  if (sys->whens.solver == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"when_list is NULL\n");
  }
  return sys->whens.solver;
}

struct bnd_boundary **slv_get_solvers_bnd_list(slv_system_t sys)
{
  if (sys->bnds.solver == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"bnd_list is NULL\n");
  }
  return sys->bnds.solver;
}

struct rel_relation **slv_get_master_rel_list(slv_system_t sys)
{
  if (sys->rels.master == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_master_rel_list returning NULL?\n");
  }
  return sys->rels.master;
}


struct rel_relation **slv_get_master_condrel_list(slv_system_t sys)
{
  if (sys->condrels.master == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"condrel_list is NULL\n");
  }
  return sys->condrels.master;
}

struct rel_relation **slv_get_master_obj_list(slv_system_t sys)
{
  if (sys->objs.master == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_master_obj_list returning NULL?\n");
  }
  return sys->objs.master;
}


struct logrel_relation **slv_get_master_logrel_list(slv_system_t sys)
{
  if (sys->logrels.master == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"logrel_list is NULL\n");
  }
  return sys->logrels.master;
}

struct logrel_relation **slv_get_master_condlogrel_list(slv_system_t sys)
{
  if (sys->condlogrels.master == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"logrel_list is NULL\n");
  }
  return sys->condlogrels.master;
}


struct w_when **slv_get_master_when_list(slv_system_t sys)
{
  if (sys->whens.master == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"when_list is NULL\n");
  }
  return sys->whens.master;
}

struct bnd_boundary **slv_get_master_bnd_list(slv_system_t sys)
{
  if (sys->bnds.master == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"bnd_list is NULL\n");
  }
  return sys->bnds.master;
}

struct gl_list_t *slv_get_symbol_list(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_symbol_list called with NULL system.\n");
    return NULL;
  }
  return sys->symbollist;
}


int slv_get_num_solvers_vars(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_solvers_vars called with NULL system.\n");
    return 0;
  }
  return sys->vars.snum;
}


int slv_get_num_solvers_pars(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_solvers_pars called with NULL system.\n");
    return 0;
  }
  return sys->pars.snum;
}

int slv_get_num_solvers_unattached(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_solvers_unattached called with NULL system.\n");
    return 0;
  }
  return sys->unattached.snum;
}

int slv_get_num_solvers_dvars(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_solvers_dvars called with NULL system.\n");
    return 0;
  }
  return sys->dvars.snum;
}

int slv_get_num_solvers_disunatt(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_solvers_disunatt called with NULL system.\n");
    return 0;
  }
  return sys->disunatt.snum;
}


int slv_get_num_solvers_rels(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_solvers_rels called with NULL system.\n");
    return 0;
  }
  return sys->rels.snum;
}


int slv_get_num_solvers_condrels(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_solvers_condrels called with NULL system.\n");
    return 0;
  }
  return sys->condrels.snum;
}

int slv_get_num_solvers_objs(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_solvers_objs called with NULL system.\n");
    return 0;
  }
  return sys->objs.snum;
}

int slv_get_num_solvers_logrels(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_solvers_logrels called with NULL system.\n");
    return 0;
  }
  return sys->logrels.snum;
}

int slv_get_num_solvers_condlogrels(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_solvers_condlogrels called with NULL system.\n");
    return 0;
  }
  return sys->condlogrels.snum;
}

int slv_get_num_solvers_whens(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_solvers_whens called with NULL system.\n");
    return 0;
  }
  return sys->whens.snum;
}

int slv_get_num_solvers_bnds(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_solvers_bnds called with NULL system.\n");
    return 0;
  }
  return sys->bnds.snum;
}

int slv_get_num_master_vars(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_master_vars called with NULL system.\n");
    return 0;
  }
  return sys->vars.mnum;
}


int slv_get_num_master_pars(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_master_pars called with NULL system.\n");
    return 0;
  }
  return sys->pars.mnum;
}
int slv_get_num_master_unattached(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_master_unattached called with NULL system.\n");
    return 0;
  }
  return sys->unattached.mnum;
}

int slv_get_num_master_dvars(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_master_dvars called with NULL system.\n");
    return 0;
  }
  return sys->dvars.mnum;
}

int slv_get_num_master_disunatt(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_master_disunatt called with NULL system.\n");
    return 0;
  }
  return sys->disunatt.mnum;
}

int slv_get_num_master_rels(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_master_rels called with NULL system.\n");
    return 0;
  }
  return sys->rels.mnum;
}


int slv_get_num_master_condrels(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_master_condrels called with NULL system.\n");
    return 0;
  }
  return sys->condrels.mnum;
}

int slv_get_num_master_objs(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_master_objs called with NULL system.\n");
    return 0;
  }
  return sys->objs.mnum;
}

int slv_get_num_master_logrels(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_master_logrels called with NULL system.\n");
    return 0;
  }
  return sys->logrels.mnum;
}

int slv_get_num_master_condlogrels(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_master_logrels called with NULL system.\n");
    return 0;
  }
  return sys->condlogrels.mnum;
}

int slv_get_num_master_whens(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_master_whens called with NULL system.\n");
    return 0;
  }
  return sys->whens.mnum;
}

int slv_get_num_master_bnds(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_num_master_bnds called with NULL system.\n");
    return 0;
  }
  return sys->bnds.mnum;
}

void slv_set_obj_relation(slv_system_t sys,struct rel_relation *obj)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_set_obj_relation called with NULL system.\n");
    return;
  }
  sys->obj = obj;
}

struct rel_relation *slv_get_obj_relation(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_obj_relation called with NULL system.\n");
    return NULL;
  }
  return sys->obj;
}

void slv_set_obj_variable(slv_system_t sys,struct var_variable *objvar,
                          unsigned maximize)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_set_obj_variable called with NULL system.\n");
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
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_obj_variable called with NULL system.\n");
    return NULL;
  }
  return sys->objvar;
}

real64 slv_get_obj_variable_gradient(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_obj_variable_gradient called with NULL system.\n");
    return 0.0;
  }
  return sys->objvargrad;
}


void slv_set_need_consistency(slv_system_t sys, int32 need_consistency)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_set_need_consistency called with NULL system.\n");
    return;
  }

  sys->need_consistency = need_consistency;
}


int32 slv_need_consistency(slv_system_t sys)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_need_consistency called with NULL system.\n");
    return 0;
  }
  return sys->need_consistency;
}

/* dont call this with null! */
static int slv_count_vars(var_filter_t *vfilter, struct var_variable **vlist)
{
  int ret = 0;
  assert(vlist!=NULL);
  while(*vlist!=NULL) {
    ret += var_apply_filter(*vlist,vfilter);
    vlist++;
  }
  return ret;
}

/* dont call this with null! */
static int slv_count_rels(rel_filter_t *rfilter, struct rel_relation **rlist)
{
  int ret = 0;
  assert(rlist!=NULL);
  while(*rlist!=NULL) {
    ret += rel_apply_filter(*rlist,rfilter);
    rlist++;
  }
  return ret;
}

/* dont call this with null! */
static int slv_count_dvars(dis_filter_t *disfilter,
			   struct dis_discrete **dlist)
{
  int ret = 0;
  assert(dlist!=NULL);
  while(*dlist!=NULL) {
    ret += dis_apply_filter(*dlist,disfilter);
    dlist++;
  }
  return ret;
}

/* dont call this with null! */
static int slv_count_logrels(logrel_filter_t *lrfilter,
			     struct logrel_relation **lrlist)
{
  int ret = 0;
  assert(lrlist!=NULL);
  while(*lrlist!=NULL) {
    ret += logrel_apply_filter(*lrlist,lrfilter);
    lrlist++;
  }
  return ret;
}

/* dont call this with null! */
static int slv_count_whens(when_filter_t *wfilter,struct w_when **wlist)
{
  int ret = 0;
  assert(wlist!=NULL);
  while(*wlist!=NULL) {
    ret += when_apply_filter(*wlist,wfilter);
    wlist++;
  }
  return ret;
}

/* dont call this with null! */
static int slv_count_bnds(bnd_filter_t *bfilter,struct bnd_boundary **blist)
{
  int ret = 0;
  assert(blist!=NULL);
  while(*blist!=NULL) {
    ret += bnd_apply_filter(*blist,bfilter);
    blist++;
  }
  return ret;
}

int slv_count_solvers_vars(slv_system_t sys, var_filter_t *vf)
{
  if (sys==NULL || sys->vars.solver == NULL || vf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_solvers_vars called with NULL\n");
    return 0;
  }
  return slv_count_vars(vf,sys->vars.solver);
}


int slv_count_solvers_pars(slv_system_t sys, var_filter_t *vf)
{
  if (sys==NULL || sys->pars.solver == NULL || vf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_solvers_pars called with NULL\n");
    return 0;
  }
  return slv_count_vars(vf,sys->pars.solver);
}

int slv_count_solvers_unattached(slv_system_t sys, var_filter_t *vf)
{
  if (sys==NULL || sys->unattached.solver == NULL || vf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_solvers_unattached called with NULL\n");
    return 0;
  }
  return slv_count_vars(vf,sys->unattached.solver);
}

int slv_count_solvers_dvars(slv_system_t sys, dis_filter_t *dvf)
{
  if (sys==NULL || sys->dvars.solver == NULL || dvf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_solvers_dvars called with NULL\n");
    return 0;
  }
  return slv_count_dvars(dvf,sys->dvars.solver);
}

int slv_count_solvers_disunatt(slv_system_t sys, dis_filter_t *dvf)
{
  if (sys==NULL || sys->disunatt.solver == NULL || dvf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_solvers_disunatt called with NULL\n");
    return 0;
  }
  return slv_count_dvars(dvf,sys->disunatt.solver);
}

int slv_count_solvers_rels(slv_system_t sys, rel_filter_t *rf)
{
  if (sys==NULL || sys->rels.solver == NULL || rf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_solvers_rels called with NULL\n");
    return 0;
  }
  return slv_count_rels(rf,sys->rels.solver);
}


int slv_count_solvers_condrels(slv_system_t sys, rel_filter_t *rf)
{
  if (sys==NULL || sys->condrels.solver == NULL || rf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_solvers_condrels called with NULL\n");
    return 0;
  }
  return slv_count_rels(rf,sys->condrels.solver);
}

int slv_count_solvers_objs(slv_system_t sys, rel_filter_t *rf)
{
  if (sys==NULL || sys->objs.solver == NULL || rf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_solvers_objs called with NULL\n");
    return 0;
  }
  return slv_count_rels(rf,sys->objs.solver);
}

int slv_count_solvers_logrels(slv_system_t sys, logrel_filter_t *lrf)
{
  if (sys==NULL || sys->logrels.solver == NULL || lrf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_solvers_logrels called with NULL\n");
    return 0;
  }
  return slv_count_logrels(lrf,sys->logrels.solver);
}


int slv_count_solvers_condlogrels(slv_system_t sys, logrel_filter_t *lrf)
{
  if (sys==NULL || sys->condlogrels.solver == NULL || lrf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_solvers_condlogrels called with NULL\n");
    return 0;
  }
  return slv_count_logrels(lrf,sys->condlogrels.solver);
}

int slv_count_solvers_whens(slv_system_t sys, when_filter_t *wf)
{
  if (sys==NULL || sys->whens.solver == NULL || wf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_solvers_whens called with NULL\n");
    return 0;
  }
  return slv_count_whens(wf,sys->whens.solver);
}

int slv_count_solvers_bnds(slv_system_t sys, bnd_filter_t *bf)
{
  if (sys==NULL || sys->bnds.solver == NULL || bf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_solvers_bnds called with NULL\n");
    return 0;
  }
  return slv_count_bnds(bf,sys->bnds.solver);
}

int slv_count_master_vars(slv_system_t sys, var_filter_t *vf)
{
  if (sys==NULL || sys->vars.master == NULL || vf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_master_vars called with NULL\n");
    return 0;
  }
  return slv_count_vars(vf,sys->vars.master);
}


int slv_count_master_pars(slv_system_t sys, var_filter_t *vf)
{
  if (sys==NULL || sys->pars.master == NULL || vf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_master_pars called with NULL\n");
    return 0;
  }
  return slv_count_vars(vf,sys->pars.master);
}

int slv_count_master_unattached(slv_system_t sys, var_filter_t *vf)
{
  if (sys==NULL || sys->unattached.master == NULL || vf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_master_unattached called with NULL\n");
    return 0;
  }
  return slv_count_vars(vf,sys->unattached.master);
}

int slv_count_master_dvars(slv_system_t sys, dis_filter_t *dvf)
{
  if (sys==NULL || sys->dvars.master == NULL || dvf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_master_dvars called with NULL\n");
    return 0;
  }
  return slv_count_dvars(dvf,sys->dvars.master);
}

int slv_count_master_disunatt(slv_system_t sys, dis_filter_t *dvf)
{
  if (sys==NULL || sys->disunatt.master == NULL || dvf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_master_disunatt called with NULL\n");
    return 0;
  }
  return slv_count_dvars(dvf,sys->disunatt.master);
}

int slv_count_master_rels(slv_system_t sys, rel_filter_t *rf)
{
  if (sys==NULL || sys->rels.master == NULL || rf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_master_rels called with NULL\n");
    return 0;
  }
  return slv_count_rels(rf,sys->rels.master);
}

int slv_count_master_condrels(slv_system_t sys, rel_filter_t *rf)
{
  if (sys==NULL || sys->condrels.master == NULL || rf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_master_rels called with NULL\n");
    return 0;
  }
  return slv_count_rels(rf,sys->condrels.master);
}

int slv_count_master_objs(slv_system_t sys, rel_filter_t *rf)
{
  if (sys==NULL || sys->objs.master == NULL || rf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_master_objs called with NULL\n");
    return 0;
  }
  return slv_count_rels(rf,sys->objs.master);
}

int slv_count_master_logrels(slv_system_t sys, logrel_filter_t *lrf)
{
  if (sys==NULL || sys->logrels.master == NULL || lrf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_master_logrels called with NULL\n");
    return 0;
  }
  return slv_count_logrels(lrf,sys->logrels.master);
}

int slv_count_master_condlogrels(slv_system_t sys, logrel_filter_t *lrf)
{
  if (sys==NULL || sys->condlogrels.master == NULL || lrf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_master_condlogrels called with NULL\n");
    return 0;
  }
  return slv_count_logrels(lrf,sys->condlogrels.master);
}

int slv_count_master_whens(slv_system_t sys, when_filter_t *wf)
{
  if (sys==NULL || sys->whens.master == NULL || wf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_master_whens called with NULL\n");
    return 0;
  }
  return slv_count_whens(wf,sys->whens.master);
}

int slv_count_master_bnds(slv_system_t sys, bnd_filter_t *bf)
{
  if (sys==NULL || sys->bnds.master == NULL || bf == NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_count_master_bnds called with NULL\n");
    return 0;
  }
  return slv_count_bnds(bf,sys->bnds.master);
}

static void printwarning(const char * fname, slv_system_t sys)
{
  error_reporter(ASC_PROG_WARNING,NULL,0,
    "%s called with bad registered client (%s).",fname,
    slv_solver_name(sys->solver));
}

static void printinfo(slv_system_t sys, const char *rname)
{
  if (CF(sys,name) == NULL ) {
    error_reporter(ASC_PROG_NOTE,NULL,0,
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
    error_reporter(ASC_PROG_WARNING,NULL,0,"slv_select_solver called with NULL system\n");
    return -1;
  }
  if ( solver >= 0 && solver < NORC ) {
    if (sys->ct != NULL && solver != sys->solver) {
	  CONSOLE_DEBUG("Solver has changed, destroy old data...");
      destroy = SlvClientsData[SNUM(sys)].cdestroy;
      if(destroy!=NULL) {
	    CONSOLE_DEBUG("About to destroy data...");
        (destroy)(sys,sys->ct);
	    CONSOLE_DEBUG("Done destroying data.");
        sys->ct = NULL;
      } else {
        error_reporter(ASC_PROG_WARNING,NULL,0,"slv_select_solver: 'cdestroy' is undefined on solver '%s' (index %d).",
          slv_solver_name(sys->solver), sys->solver);
        /* return sys->solver; */
/** @TODO FIXME HACK this is probably very dodgy... */
        //CONSOLE_DEBUG("No 'cdestroy' method, so just killing sys->ct...");
		//sys->ct = NULL;
      }
    }

    if (sys->ct != NULL) {
      CONSOLE_DEBUG("sys->ct not-null, so returning current solver...");
      return sys->solver;
    }

    CONSOLE_DEBUG("Updating current solver...");
    status_index = solver;
    sys->solver = solver;
    if ( CF(sys,ccreate) != NULL) {
      CONSOLE_DEBUG("Running ccreate method for new solver...");
      sys->ct = SF(sys,ccreate)(sys,&status_index);
      CONSOLE_DEBUG("Done running ccreate");
    } else {
      error_reporter(ASC_PROG_ERROR,NULL,0,"slv_select_solver create failed due to bad client %s\n",
        slv_solver_name(sys->solver));
      return sys->solver;
    }
    if (sys->ct==NULL) {
      error_reporter(ASC_PROG_WARNING,NULL,0,"SlvClientCreate failed in slv_select_solver\n");
      sys->solver = -1;
    } else {
      if (status_index) {
        error_reporter(ASC_PROG_WARNING,NULL,0,"SlvClientCreate succeeded with warning %d %s\n",
          status_index," in slv_select_solver");
      }
      /* we could do a better job explaining the client warnings... */
      sys->solver = solver;
    }
  } else {
    error_reporter(ASC_PROG_WARNING,NULL,0,"slv_select_solver: invalid solver index '%d'.",
      solver);
    return -1;
  }
  return sys->solver;
}


int slv_switch_solver(slv_system_t sys,int solver)
{
  int status_index;

  if (sys ==NULL) {
    error_reporter(ASC_PROG_WARNING,NULL,0,"slv_switch_solver called with NULL system\n");
    return -1;
  }
  if (LSI(solver)) {
    status_index = solver;
    sys->solver = solver;
    if ( CF(sys,ccreate) != NULL) {
      sys->ct = SF(sys,ccreate)(sys,&status_index);
    } else {
      error_reporter(ASC_PROG_WARNING,NULL,0,"slv_switch_solver create failed due to bad client %s\n",
         slv_solver_name(sys->solver));
      return sys->solver;
    }
    if (sys->ct==NULL) {
      error_reporter(ASC_PROG_ERROR,NULL,0,"SlvClientCreate failed in slv_switch_solver\n");
      sys->solver = -1;
    } else {
      if (status_index) {
        error_reporter(ASC_PROG_WARNING,NULL,0,"SlvClientCreate succeeded with warning %d %s\n",
           status_index," in slv_switch_solver");
      }
      sys->solver = solver;
    }
  } else {
    error_reporter(ASC_PROG_WARNING,NULL,0,"slv_switch_solver called with unknown client (%d)\n",solver);
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
      error_reporter(ASC_PROG_WARNING,NULL,0,"Unrecognized parameter type in slv_destroy_parms\n");
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
      error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_default_parameters called with parameterless index\n");
      return 0;
    } else {
      /* send NULL system when setting up interface */
      (SlvClientsData[index].getdefparam)(NULL,NULL,parameters);
      return 1;
    }
  } else {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_get_default_parameters called with unregistered index\n");
    return 0;
  }
}

void slv_get_parameters(slv_system_t sys,slv_parameters_t *parameters)
{
  if ( CF(sys,getparam) == NULL ) {
    printwarning("slv_get_parameters",sys);
    return;
  }
  SF(sys,getparam)(sys,sys->ct,parameters);
}


void slv_set_parameters(slv_system_t sys,slv_parameters_t *parameters)
{
  if ( CF(sys,setparam) == NULL ) {
    printwarning("slv_set_parameters",sys);
    return;
  }
  if (parameters->whose != sys->solver) {
    error_reporter(ASC_PROG_ERROR,NULL,0,
		"slv_set_parameters can give parameters from one client to a different client.");
    return;
  }
  SF(sys,setparam)(sys,sys->ct,parameters);
}

void slv_get_status(slv_system_t sys, slv_status_t *status)
{
  if ( CF(sys,getstatus) == NULL ) {
    printwarning("slv_get_status",sys);
    return;
  }
  SF(sys,getstatus)(sys,sys->ct,status);
}

linsol_system_t slv_get_linsol_sys(slv_system_t sys)
{
  if (CF(sys,getlinsol) == NULL ) {
    printinfo(sys,"slv_get_linsol_sys");
    return NULL;
  }
  return SF(sys,getlinsol)(sys,sys->ct);
}

mtx_matrix_t slv_get_sys_mtx(slv_system_t sys)
{
  if (CF(sys,getsysmtx) == NULL ) {
    printinfo(sys,"slv_get_sys_mtx");
    return NULL;
  }
  return SF(sys,getsysmtx)(sys,sys->ct);
}

linsolqr_system_t slv_get_linsolqr_sys(slv_system_t sys)
{
  if (CF(sys,getlinsys) == NULL ) {
    printinfo(sys,"slv_get_linsolqr_sys");
    return NULL;
  }
  return SF(sys,getlinsys)(sys,sys->ct);
}

void slv_dump_internals(slv_system_t sys,int level)
{
  if (CF(sys,dumpinternals) == NULL ) {
    printinfo(sys,"slv_dump_internals");
    return;
  }
  SF(sys,dumpinternals)(sys,sys->ct,level);
}

void slv_presolve(slv_system_t sys)
{
  if ( CF(sys,presolve) == NULL ) {
    printwarning("slv_presolve",sys);
    return;
  }
  SF(sys,presolve)(sys,sys->ct);
}

void slv_resolve(slv_system_t sys)
{
  if ( CF(sys,resolve) == NULL ) {
    printwarning("slv_resolve",sys);
    return;
  }
  SF(sys,resolve)(sys,sys->ct);
}

void slv_iterate(slv_system_t sys)
{
  if ( CF(sys,iterate) == NULL ) {
    printwarning("slv_iterate",sys);
    return;
  }
  SF(sys,iterate)(sys,sys->ct);
}

void slv_solve(slv_system_t sys)
{
  fprintf(stderr,"STARTING SLV_SOLVE\n");
  /*ERROR_REPORTER_DEBUG("started");*/
  if ( CF(sys,solve) == NULL ) {
    printwarning("slv_solve",sys);
    return;
  }
  SF(sys,solve)(sys,sys->ct);
}


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
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_set_client_token called with NULL system.\n");
    return;
  }
  sys->ct = ct;
}

void slv_set_solver_index(slv_system_t sys, int solver)
{
  if (sys==NULL) {
    error_reporter(ASC_PROG_ERROR,NULL,0,"slv_set_solver_index called with NULL system.\n");
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

