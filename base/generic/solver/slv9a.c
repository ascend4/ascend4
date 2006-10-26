/*
 *  Logical Relation Solver
 *  by Vicente Rico-Ramirez
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: slv9a.c,v $
 *  Date last modified: $Date: 2000/01/25 02:28:03 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the SLV solver.
 *  The SLV solver is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The SLV solver is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 *
 */

#include <math.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/tm_time.h>
#include <general/mathmacros.h>
#include <utilities/mem.h>
#include <compiler/compiler.h>
#include <compiler/instance_enum.h>
#include <general/list.h>
#include <compiler/fractions.h>
#include <compiler/dimen.h>
#include <compiler/functype.h>
#include <compiler/func.h>
#include "mtx.h"
#include "linsol.h"
#include "linsolqr.h"
#include "slv_types.h"
#include "var.h"
#include "rel.h"
#include "discrete.h"
#include "conditional.h"
#include "logrel.h"
#include "bnd.h"
#include "calc.h"
#include "relman.h"
#include "logrelman.h"
#include "bndman.h"
#include "slv_common.h"
#include "slv_client.h"
#include "cond_config.h"
#include "slv9a.h"
#include "slv_stdcalls.h"


#if !defined(STATIC_LRSLV) || defined(DYNAMIC_LRSLV)
int slv9a_register(SlvFunctionsT *f)
{
  (void)f;  /* stop gcc whine about unused parameter */

  FPRINTF(ASCERR,"LRSlv not compiled in this ASCEND IV.\n");
  return 1;
}
#else /* either STATIC_LRSLV or DYNAMIC_LRSLV is defined */
#ifdef DYNAMIC_LRSLV
/* do dynamic loading stuff.   yeah, right */
#else /* following is used if STATIC_LRSLV is defined */

#define SLV9A(s) ((slv9a_system_t)(s))
#define SERVER (sys->slv)
#define slv9a_PA_SIZE 6 /* MUST INCREMENT WHEN ADDING PARAMETERS */
#define SHOW_MORE_IMPT_PTR (sys->parm_array[0])
#define SHOW_MORE_IMPT     ((*(int32 *)SHOW_MORE_IMPT_PTR))
#define SHOW_LESS_IMPT_PTR (sys->parm_array[1])
#define SHOW_LESS_IMPT     ((*(int32 *)SHOW_LESS_IMPT_PTR))
#define AUTO_RESOLVE_PTR (sys->parm_array[2])
#define AUTO_RESOLVE     ((*(int32 *)AUTO_RESOLVE_PTR))
#define TIME_LIMIT_PTR (sys->parm_array[3])
#define TIME_LIMIT     ((*(int32 *)TIME_LIMIT_PTR))
#define ITER_LIMIT_PTR (sys->parm_array[4])
#define ITER_LIMIT     ((*(int32 *)ITER_LIMIT_PTR))
#define PERTURB_BOUNDARY_PTR (sys->parm_array[5])
#define PERTURB_BOUNDARY     ((*(int32 *)PERTURB_BOUNDARY_PTR))

/*
 * auxiliar structures
 */
struct structural_data {
  mtx_matrix_t           mtx;          /* For use in structural analysis */
  unsigned               *subregions;  /* Set of subregion indeces */
  dof_t                  *dofdata;     /* dof data pointer from server */
  mtx_region_t           reg;          /* Current block region */
  int32                  rank;         /* Rank of the matrix */
};


/*
 * solver structure
 */
struct slv9a_system_structure {

  /*
   *  Problem definition
   */
  slv_system_t   	        slv;   /* slv_system_t back-link */
  struct dis_discrete        **vlist; /* Dis vars list (NULL terminated) */
  struct logrel_relation     **rlist; /* Logrelations list(NULL terminated) */
  struct bnd_boundary        **blist; /* Boundaries. Maybe NULL */

  /*
   *  Solver information
   */
  int                    integrity;    /* ? Has the system been created */
  int32                  presolved;    /* ? Has the system been presolved */
  slv_parameters_t       p;            /* parameters */
  slv_status_t           s;            /* Status (as of iteration end) */
  int32                  cap;          /* Order of matrix/vectors */
  int32                  rank;         /* Symbolic rank of problem */
  int32                  vused;        /* Free and incident variables */
  int32                  vtot;         /* length of varlist */
  int32                  rused;        /* Included relations */
  int32                  rtot;         /* length of rellist */
  double                 clock;        /* CPU time */

  void *parm_array[slv9a_PA_SIZE];
  struct slv_parameter pa[slv9a_PA_SIZE];
  /*
   *  Calculated data
   */
  struct structural_data S;            /* structural information */
};


/*
 *  Integrity checks
 *  ----------------
 *     check_system(sys)
 */

#define OK        ((int32)813029392)
#define DESTROYED ((int32)103289182)

static int check_system(slv9a_system_t sys)
/*
 *  Checks sys for NULL and for integrity.
 */
{
  if( sys == NULL ) {
    FPRINTF(ASCERR,"ERROR:  (slv9a) check_system\n");
    FPRINTF(ASCERR,"        NULL system handle.\n");
    return 1;
  }

  switch( sys->integrity ) {
  case OK:
    return 0;
  case DESTROYED:
    FPRINTF(ASCERR,"ERROR:  (slv9a) check_system\n");
    FPRINTF(ASCERR,"        System was recently destroyed.\n");
    return 1;
  default:
    FPRINTF(ASCERR,"ERROR:  (slv9a) check_system\n");
    FPRINTF(ASCERR,"        System reused or never allocated.\n");
    return 1;
  }
}

/*
 *  General input/output routines (discrete vars and log rels)
 *  ----------------------------------------------------------
 *     print_dis_name(out,sys,dvar)
 *     print_logrel_name(out,sys,lrel)
 */

#define print_dis_name(a,b,c) slv_print_dis_name((a),(b)->slv,(c))
#define print_logrel_name(a,b,c) slv_print_logrel_name((a),(b)->slv,(c))

/*
 *  Debug output routines
 *  ---------------------
 *     debug_delimiter(fp)
 *     debug_out_dvar_values(fp,sys)
 *     debug_out_logrel_residuals(fp,sys)
 *     debug_out_structural_mtx(fp,sys)
 */

static void debug_delimiter( FILE *fp)
/*
 *  Outputs a hyphenated line.
 */
{
  int i;
  for( i=0; i<60; i++ ) PUTC('-',fp);
  PUTC('\n',fp);
}

#if DEBUG

/*
 *  Outputs all variable values in current block.
 */
static void debug_out_dvar_values( FILE *fp, slv9a_system_t sys)
{
  int32 col;
  struct dis_discrete *dvar;

  FPRINTF(fp,"Discrete var values --> \n");
  for( col = sys->S.reg.col.low; col <= sys->S.reg.col.high ; col++ ) {
    dvar = sys->vlist[mtx_col_to_org(sys->S.mtx,col)];
    print_dis_name(fp,sys,dvar);
    FPRINTF(fp, "\nI	Value	Col \n");
    FPRINTF(fp,"%d\t%d\t%d\n",dis_sindex(dvar),dis_value(dvar),col);
  }
}


/*
 *  Outputs all logical relation residuals in current block.
 */
static void debug_out_logrel_residuals( FILE *fp, slv9a_system_t sys)
{
  int32 row;

  FPRINTF(fp,"Logical rel residuals --> \n");
  for( row = sys->S.reg.row.low; row <= sys->S.reg.row.high ; row++ ) {
    struct logrel_relation *lrel;
    lrel = sys->rlist[mtx_row_to_org(sys->S.mtx,row)];
    FPRINTF(fp,"  %d : ",logrel_residual(lrel));
    print_logrel_name(fp,sys,lrel);
    PUTC('\n',fp);
  }
  PUTC('\n',fp);
}


/*
 *  Outputs permutation of the elements in the structural matrix.
 */
static void debug_out_structural_mtx( FILE *fp, slv9a_system_t sys)
{
  mtx_coord_t nz;

  nz.row = sys->S.reg.row.low;
  for( ; nz.row <= sys->S.reg.row.high; ++(nz.row) ) {
    FPRINTF(fp,"   Row %d (lrel %d)\n", nz.row,
            mtx_row_to_org(sys->S.mtx,nz.row));
  }
}

#endif  /* DEBUG */

/*
 *  Array operations
 *  -----------------
 *     destroy_array(p)
 *     create_array(len,type)
 *     create_zero_array(len,type)
 */
#define destroy_array(p)  \
   if( (p) != NULL ) ascfree((p))
#define create_array(len,type)  \
   ((len) > 0 ? (type *)ascmalloc((len)*sizeof(type)) : NULL)
#define create_zero_array(len,type)  \
   ((len) > 0 ? (type *)asccalloc((len),sizeof(type)) : NULL)


/*
 *  Block routines
 *  --------------
 *     block_feasible(sys)
 *     move_to_next_block(sys)
 *     find_next_unconverged_block(sys)
 */

/*
 *  Returns TRUE if the current block is feasible, FALSE otherwise.
 *  It is assumed that the residuals have been computed.
 */
static boolean block_feasible( slv9a_system_t sys)
{
   int32 row;
   struct logrel_relation *rel;

   if( !sys->s.calc_ok )
      return(FALSE);

   for( row = sys->S.reg.row.low; row <= sys->S.reg.row.high; row++ ) {
     rel = sys->rlist[mtx_row_to_org(sys->S.mtx,row)];
     if( !logrel_satisfied(rel) ) return FALSE;
   }
   return TRUE;
}


/*
 *  Moves on to the next block, updating all of the solver information.
 *  To move to the first block, set sys->s.block.current_block to -1 before
 *  calling.  If already at the last block, then sys->s.block.current_block
 *  will equal the number of blocks and the system will be declared
 *  converged.
 */
static void move_to_next_block( slv9a_system_t sys)
{
  struct dis_discrete *dvar;
  struct logrel_relation *lrel;
  int32 row;
  int32 col;
  int32 ci;

  if( sys->s.block.current_block >= 0 ) {

    /* Record cost accounting info here. */
    ci=sys->s.block.current_block;
    sys->s.cost[ci].size	=	sys->s.block.current_size;
    sys->s.cost[ci].iterations	=	sys->s.block.iteration;
    sys->s.cost[ci].funcs	=	sys->s.block.funcs;
    sys->s.cost[ci].jacs	=	sys->s.block.jacs;
    sys->s.cost[ci].functime	=	sys->s.block.functime;
    sys->s.cost[ci].jactime	=	sys->s.block.jactime;
    sys->s.cost[ci].time	=	sys->s.block.cpu_elapsed;
    sys->s.cost[ci].resid	=	sys->s.block.residual;

    /* De-initialize previous block */
    if (SHOW_LESS_IMPT && (sys->s.block.current_size >1)) {
      FPRINTF(LIF(sys),"Block %d converged.\n",
              sys->s.block.current_block);
    }

    for( col=sys->S.reg.col.low; col <= sys->S.reg.col.high; col++ ) {
      dvar = sys->vlist[mtx_col_to_org(sys->S.mtx,col)];
      dis_set_in_block(dvar,FALSE);
    }

    for( row=sys->S.reg.row.low; row <= sys->S.reg.row.high; row++ ) {
      lrel = sys->rlist[mtx_row_to_org(sys->S.mtx,row)];
      logrel_set_in_block(lrel,FALSE);
    }
    sys->s.block.previous_total_size += sys->s.block.current_size;

  }

  sys->s.block.current_block++;
  if( sys->s.block.current_block < sys->s.block.number_of ) {

    /* Initialize next block */
    sys->S.reg =
      (slv_get_solvers_log_blocks(SERVER))->block[sys->s.block.current_block];

    row = sys->S.reg.row.high - sys->S.reg.row.low + 1;
    col = sys->S.reg.col.high - sys->S.reg.col.low + 1;
    sys->s.block.current_size = MAX(row,col);

    sys->s.block.iteration = 0;
    sys->s.block.cpu_elapsed = 0.0;
    sys->s.block.functime = 0.0;
    sys->s.block.jactime = 0.0;
    sys->s.block.funcs = 0;
    sys->s.block.jacs = 0;

    if(SHOW_LESS_IMPT && ( sys->s.block.current_size > 1)) {
      debug_delimiter(LIF(sys));
    }
    if(SHOW_LESS_IMPT) {
      FPRINTF(LIF(sys),"\n%-40s ---> %d in [%d..%d]\n",
              "Current block number", sys->s.block.current_block,
              0, sys->s.block.number_of-1);
      FPRINTF(LIF(sys),"%-40s ---> %d\n", "Current block size",
        sys->s.block.current_size);
    }
    sys->s.calc_ok = TRUE;

  } else {
    /*
     *  Before we claim convergence, we must check if we left behind
     *  some unassigned logrelations.  If and only if they happen to be
     *  satisfied at the current point, convergence has been obtained.
     */
    if( sys->s.struct_singular ) {
      sys->s.block.current_size = sys->rused - sys->rank;
      if(SHOW_LESS_IMPT) {
        debug_delimiter(LIF(sys));
        FPRINTF(LIF(sys),"%-40s ---> %d\n", "Unassigned Log Rels",
                sys->s.block.current_size);
      }
      if( block_feasible(sys) ) {
        if(SHOW_LESS_IMPT) {
          FPRINTF(LIF(sys),"\nUnassigned logrels ok. You lucked out.\n");
        }
        sys->s.converged = TRUE;
      } else {
        if(SHOW_LESS_IMPT) {
          FPRINTF(LIF(sys),"\nProblem inconsistent:  %s.\n",
                  "Unassigned logrels not satisfied");
        }
        sys->s.inconsistent = TRUE;
      }
      if(SHOW_LESS_IMPT) {
        debug_delimiter(LIF(sys));
      }
    } else {
      sys->s.converged = TRUE;
    }
  }
}


/*
 *  Moves to next unconverged block, assuming that the current block has
 *  converged (or is -1, to start).
 */
static void find_next_unconverged_block( slv9a_system_t sys)
{
   do {
     move_to_next_block(sys);
#if DEBUG
     debug_out_dvar_values(ASCERR,sys);
     debug_out_logrel_residuals(ASCERR,sys);
#endif /* DEBUG */
   } while( !sys->s.converged && block_feasible(sys) );
}


/*
 *  Iteration begin/end routines
 *  ----------------------------
 *     iteration_begins(sys)
 *     iteration_ends(sys)
 */

/*
 *  Prepares sys for entering an iteration, increasing the iteration counts
 *  and starting the clock.
 */
static void iteration_begins( slv9a_system_t sys)
{
   sys->clock = tm_cpu_time();
   ++(sys->s.block.iteration);
   ++(sys->s.iteration);
   if(SHOW_LESS_IMPT && (sys->s.block.current_size >1 )) {
     FPRINTF(LIF(sys),"\n%-40s ---> %d\n",
             "Iteration", sys->s.block.iteration);
     FPRINTF(LIF(sys),"%-40s ---> %d\n",
             "Total iteration", sys->s.iteration);
   }
}


/*
 *  Prepares sys for exiting an iteration, stopping the clock and recording
 *  the cpu time.
 */
static void iteration_ends( slv9a_system_t sys)
{
   double cpu_elapsed;   /* elapsed this iteration */

   cpu_elapsed = (double)(tm_cpu_time() - sys->clock);
   sys->s.block.cpu_elapsed += cpu_elapsed;
   sys->s.cpu_elapsed += cpu_elapsed;
   if(SHOW_LESS_IMPT && (sys->s.block.current_size >1 )) {
     FPRINTF(LIF(sys),"%-40s ---> %g\n",
            "Elapsed time", sys->s.block.cpu_elapsed);
     FPRINTF(LIF(sys),"%-40s ---> %g\n",
            "Total elapsed time", sys->s.cpu_elapsed);
   }
}


/*
 *  Updates the solver status.
 */
static void update_status( slv9a_system_t sys)
{
   boolean unsuccessful;

   if( !sys->s.converged ) {
      sys->s.time_limit_exceeded =
         (sys->s.block.cpu_elapsed >= TIME_LIMIT);
      sys->s.iteration_limit_exceeded =
         (sys->s.block.iteration >= ITER_LIMIT);
   }

   unsuccessful = sys->s.diverged || sys->s.inconsistent ||
      sys->s.iteration_limit_exceeded || sys->s.time_limit_exceeded
     || (sys->s.block.current_size >1 );

   sys->s.ready_to_solve = !unsuccessful && !sys->s.converged;
   sys->s.ok = !unsuccessful && sys->s.calc_ok && !sys->s.struct_singular;
}


static int32 slv9a_get_default_parameters(slv_system_t server,
					  SlvClientToken asys,
					  slv_parameters_t *parameters)
{
  slv9a_system_t sys;
  union parm_arg lo,hi,val;
  struct slv_parameter *new_parms = NULL;
  int32 make_macros = 0;

  if (server != NULL && asys != NULL) {
    sys = SLV9A(asys);
    make_macros = 1;
  }

  if (parameters->parms == NULL) {
   /* an external client wants our parameter list.
     * an instance of slv9a_system_structure has this pointer
     * already set in slv9a_create
     */
    new_parms = (struct slv_parameter *)
    ascmalloc((slv9a_PA_SIZE)*sizeof(struct slv_parameter));
    if (new_parms == NULL) {
      return -1;
    }
    parameters->parms = new_parms;
    parameters->dynamic_parms = 1;
  }
  parameters->num_parms = 0;

  /* begin defining parameters */

  slv_define_parm(parameters, bool_parm,
	       "showmoreimportant", "showmoreimportant", "showmoreimportant",
	       U_p_bool(val,1),U_p_bool(lo,0),U_p_bool(hi,1),-1);
  SLV_BPARM_MACRO(SHOW_MORE_IMPT_PTR,parameters);

  slv_define_parm(parameters, bool_parm,
	       "showlessimportant", "detailed solving info",
               "detailed solving info",
	       U_p_bool(val, 0),U_p_bool(lo,0),U_p_bool(hi,1), 2);
  SLV_BPARM_MACRO(SHOW_LESS_IMPT_PTR,parameters);

  slv_define_parm(parameters, bool_parm,
	       "autoresolve", "auto-resolve", "auto-resolve",
	       U_p_bool(val,1),U_p_bool(lo,0),U_p_bool(hi,1), 2);
  SLV_BPARM_MACRO(AUTO_RESOLVE_PTR,parameters);

  slv_define_parm(parameters, int_parm,
	       "timelimit", "time limit (CPU sec/block)",
               "time limit (CPU sec/block)",
	       U_p_int(val,1500),U_p_int(lo, 1),U_p_int(hi,20000),1);
  SLV_IPARM_MACRO(TIME_LIMIT_PTR,parameters);

  slv_define_parm(parameters, int_parm,
	       "iterationlimit", "max iterations/block",
               "max iterations/block",
	       U_p_int(val, 30),U_p_int(lo, 1),U_p_int(hi,20000),1);
  SLV_IPARM_MACRO(ITER_LIMIT_PTR,parameters);

  slv_define_parm(parameters, bool_parm,
	       "perturbboundaries", "perturb boundaries",
               "perturb boundaries",
	       U_p_bool(val, 0),U_p_bool(lo,0),U_p_bool(hi,1), -1);
  SLV_BPARM_MACRO(PERTURB_BOUNDARY_PTR,parameters);

  return 1;
}



/*
 *  External routines
 *  -----------------
 *     See slv_client.h
 */

static SlvClientToken slv9a_create(slv_system_t server, int *statusindex)
{
  slv9a_system_t sys;

  sys = (slv9a_system_t)asccalloc(1, sizeof(struct slv9a_system_structure) );
  if (sys==NULL) {
   *statusindex = 1;
    return sys;
  }
  SERVER = server;
  sys->p.parms = sys->pa;
  sys->p.dynamic_parms = 0;
  slv9a_get_default_parameters(server,(SlvClientToken)sys,&(sys->p));
  sys->integrity = OK;
  sys->presolved = 0;
  sys->p.output.more_important = stdout;
  sys->p.output.less_important = stdout;
  sys->p.whose = (*statusindex);

  sys->s.ok = TRUE;
  sys->s.calc_ok = TRUE;
  sys->s.costsize = 0;
  sys->s.cost = NULL; /*redundant, but sanity preserving */
  sys->vlist = slv_get_solvers_dvar_list(server);
  sys->rlist = slv_get_solvers_logrel_list(server);
  sys->blist = slv_get_solvers_bnd_list(server);
  if (sys->vlist == NULL) {
    ascfree(sys);
    FPRINTF(ASCERR,"LRSlv called with no discrete variables.\n");
    *statusindex = -2;
    return NULL;
  }
  if (sys->rlist == NULL) {
    ascfree(sys);
    FPRINTF(ASCERR,"LRSlv called with no logical relations.\n");
    *statusindex = -1;
    return NULL;
  }
  slv_check_dvar_initialization(server);
  *statusindex = 0;
  return((SlvClientToken)sys);
}


static void destroy_matrices( slv9a_system_t sys)
{
  if (sys->S.mtx) {
    mtx_destroy(sys->S.mtx);
    sys->S.mtx = NULL;
  }
  return;
}


static int slv9a_eligible_solver(slv_system_t server)
{
  logrel_filter_t lrfilter;

  lrfilter.matchbits = (LOGREL_INCLUDED | LOGREL_ACTIVE);
  lrfilter.matchvalue = (LOGREL_INCLUDED | LOGREL_ACTIVE);
  if (slv_count_solvers_logrels(server,&lrfilter)) {
    return(TRUE);
  } else {
    return(FALSE);
  }
}


static void slv9a_get_parameters(slv_system_t server, SlvClientToken asys,
				 slv_parameters_t *parameters)
{
  slv9a_system_t sys;
  sys = SLV9A(asys);
  if (server == NULL || sys==NULL) return;
  if (check_system(sys)) return;
  mem_copy_cast(&(sys->p),parameters,sizeof(slv_parameters_t));
}


static void slv9a_set_parameters(slv_system_t server, SlvClientToken asys,
				 slv_parameters_t *parameters)
{
  slv9a_system_t sys;
  sys = SLV9A(asys);
  if (server == NULL || sys==NULL) return;
  if (check_system(sys)) return;
  mem_copy_cast(parameters,&(sys->p),sizeof(slv_parameters_t));
}



static void slv9a_get_status(slv_system_t server, SlvClientToken asys,
                            slv_status_t *status)
{
  slv9a_system_t sys;
  (void) server;
  sys = SLV9A(asys);
  if (check_system(sys)) return;
  mem_copy_cast(&(sys->s),status,sizeof(slv_status_t));
}


static linsolqr_system_t slv9a_get_linsolqr_sys(slv_system_t server,
                                                SlvClientToken asys)
{
  slv9a_system_t sys;
  sys = SLV9A(asys);
  if (server == NULL || sys==NULL) return NULL;;
  FPRINTF(ASCERR,"ERROR:  (slv9a) slv9a_get_linsolqr_sys\n");
  FPRINTF(ASCERR,"         slv9a has no linsolqr sys\n");
  return ( NULL );
}


static linsol_system_t slv9a_get_linsol_sys(slv_system_t server,
                                            SlvClientToken asys)
{
  slv9a_system_t sys;
  sys = SLV9A(asys);
  if (server == NULL || sys==NULL) return NULL;;
  FPRINTF(ASCERR,"ERROR:  (slv9a) slv9a_get_linsol_sys\n");
  FPRINTF(ASCERR,"         slv9a has no linsol sys\n");
  return( NULL );
}


/*
 *  Performs structural analysis on the system, setting the flags in
 *  status.  The problem must be set up, the logrelation/dis_var list
 *  must be non-NULL.  The structural matrix  must be created and have
 *  the correct order (stored in sys->cap).
 *  Everything else will be determined here.
 *  On entry there isn't yet a correspondence between dis_sindex and
 *  structural matrix column. Here we establish that.
 */
static void structural_analysis(slv_system_t server, slv9a_system_t sys)
{
  dis_filter_t dvfilter;
  logrel_filter_t lrfilter;

  /*
   * The server has marked incidence flags already.
   */
  /* count included equalities */
  lrfilter.matchbits = (LOGREL_INCLUDED | LOGREL_ACTIVE);
  lrfilter.matchvalue = (LOGREL_INCLUDED | LOGREL_ACTIVE);
  sys->rused = slv_count_solvers_logrels(server,&lrfilter);

  /* count free and incident vars */
  dvfilter.matchbits = (DIS_FIXED | DIS_INCIDENT | DIS_BVAR | DIS_ACTIVE);
  dvfilter.matchvalue = (DIS_INCIDENT | DIS_BVAR | DIS_ACTIVE);
  sys->vused = slv_count_solvers_dvars(server,&dvfilter);

  /* Symbolic analysis */
  sys->rtot = slv_get_num_solvers_logrels(server);
  sys->vtot = slv_get_num_solvers_dvars(server);
  if (sys->rtot) {
    if (slv_log_block_partition(server)) {
      FPRINTF(ASCERR,
             "Structural Analysis:Error in slv_log_block_partition\n");
      return;
    }
  }
  sys->S.dofdata = slv_get_log_dofdata(server);
  sys->rank = sys->S.dofdata->structural_rank;

  /* Initialize Status */
  sys->s.over_defined = (sys->rused > sys->vused);
  sys->s.under_defined = (sys->rused < sys->vused);
  sys->s.struct_singular = (sys->rank < sys->rused);
  sys->s.block.number_of = (slv_get_solvers_log_blocks(SERVER))->nblocks;
}


/*
 * Create matrix to perform symbolic analysis
 */
static void create_matrices(slv_system_t server, slv9a_system_t sys)
{
  sys->S.mtx = mtx_create();
  mtx_set_order(sys->S.mtx,sys->cap);
  structural_analysis(server,sys);
}


/*
 * Here we will check if any fixed or included flags have
 * changed since the last presolve.
 */
static int32 slv9a_dof_changed(slv9a_system_t sys)
{
  int32 ind, result = 0;
  /*
   *  Currently we have two copies of the fixed and included flags
   *  which must be kept in sync.  The dis_fixed and logrel_included
   *  functions perform the syncronization and hence must be called
   *  over the whole dvar list and logrel list respectively.  When we move
   *  to using only one set of flags (bit flags) this function can
   *  be changed to return 1 at the first indication of a change
   *  in the dof.
   */

  /* search for dvars that were fixed and are now free */
  for( ind = sys->vused; ind < sys->vtot; ++ind ) {
    if( !dis_fixed(sys->vlist[ind]) && dis_active(sys->vlist[ind]) ) {
      ++result;
    }
  }
  /* search for logrels that were unincluded and are now included */
  for( ind = sys->rused; ind < sys->rtot; ++ind ) {
    if( logrel_included(sys->rlist[ind]) && logrel_active(sys->rlist[ind])) {
      ++result;
    }
  }
  /* search for dvars that were free and are now fixed */
  for( ind = sys->vused -1; ind >= 0; --ind ) {
    if( dis_fixed(sys->vlist[ind]) ||  !dis_active(sys->vlist[ind])) {
      ++result;
    }
  }
  /* search for logrels that were included and are now unincluded */
  for( ind = sys->rused -1; ind >= 0; --ind ) {
    if(!logrel_included(sys->rlist[ind]) || !logrel_active(sys->rlist[ind])){
      ++result;
    }
  }
  return result;
}


static void reset_cost(struct slv_block_cost *cost,int32 costsize)
{
  int32 ind;
  for( ind = 0; ind < costsize; ++ind ) {
    cost[ind].size = 0;
    cost[ind].iterations = 0;
    cost[ind].funcs = 0;
    cost[ind].jacs = 0;
    cost[ind].functime = 0;
    cost[ind].jactime = 0;
    cost[ind].time = 0;
    cost[ind].resid = 0;
  }
}


static void slv9a_presolve(slv_system_t server, SlvClientToken asys)
{
  struct dis_discrete **dvp;
  struct logrel_relation **lrp;
  int32 cap, ind;
  int32 matrix_creation_needed = 1;
  slv9a_system_t sys;

  sys = SLV9A(asys);
  iteration_begins(sys);
  check_system(sys);
  if( sys->vlist == NULL ) {
    FPRINTF(ASCERR,"ERROR:  (slv9a) slv9a_presolve\n");
    FPRINTF(ASCERR,"        Discrete Variable list was never set.\n");
    return;
  }
  if( sys->rlist == NULL ) {
    FPRINTF(ASCERR,"ERROR:  (slv9a) slv9a_presolve\n");
    FPRINTF(ASCERR,"        Logical Relation list was never set.\n");
    return;
  }

  if(sys->presolved > 0) { /* system has been presolved before */
    if(!slv9a_dof_changed(sys) ) { /* no changes in fixed or included flags */
#if DEBUG
      FPRINTF(ASCERR,"Avoiding matrix destruction/creation\n");
#endif /* DEBUG */
      matrix_creation_needed = 0;
     }
  }

  lrp=sys->rlist;
  for( ind = 0; ind < sys->rtot; ++ind ) {
    logrel_set_satisfied(lrp[ind],FALSE);
  }
  if( matrix_creation_needed ) {
    cap = slv_get_num_solvers_logrels(SERVER);
    sys->cap = slv_get_num_solvers_dvars(SERVER);
    sys->cap = MAX(sys->cap,cap);
    dvp=sys->vlist;
    for( ind = 0; ind < sys->vtot; ++ind ) {
      dis_set_in_block(dvp[ind],FALSE);
    }
    lrp=sys->rlist;
    for( ind = 0; ind < sys->rtot; ++ind ) {
      logrel_set_in_block(lrp[ind],FALSE);
      logrel_set_satisfied(lrp[ind],FALSE);
    }
    sys->presolved = 1; /* full presolve recognized here */
    destroy_matrices(sys);
    create_matrices(server,sys);
    sys->s.block.current_reordered_block = -2;
  }

  /* Reset status */
  sys->s.iteration = 0;
  sys->s.cpu_elapsed = 0.0;
  sys->s.converged = sys->s.diverged = sys->s.inconsistent = FALSE;
  sys->s.block.previous_total_size = 0;
  sys->s.costsize = 1+sys->s.block.number_of;

  if( matrix_creation_needed ) {
    destroy_array(sys->s.cost);
    sys->s.cost = create_zero_array(sys->s.costsize,struct slv_block_cost);
    for( ind = 0; ind < sys->s.costsize; ++ind ) {
      sys->s.cost[ind].reorder_method = -1;
    }
  } else {
    reset_cost(sys->s.cost,sys->s.costsize);
  }

  /* set to go to first unconverged block */
  sys->s.block.current_block = -1;
  sys->s.block.current_size = 0;
  sys->s.calc_ok = TRUE;
  sys->s.block.iteration = 0;

  update_status(sys);
  iteration_ends(sys);
  sys->s.cost[sys->s.block.number_of].time=sys->s.cpu_elapsed;
}


static void slv9a_resolve(slv_system_t server, SlvClientToken asys)
{
  struct dis_discrete **dvp;
  struct logrel_relation **lrp;
  slv9a_system_t sys;

  sys = SLV9A(asys);
  (void) server;
  check_system(sys);

  for( dvp = sys->vlist ; *dvp != NULL ; ++dvp ) {
    dis_set_in_block(*dvp,FALSE);
  }
  for( lrp = sys->rlist ; *lrp != NULL ; ++lrp ) {
    logrel_set_in_block(*lrp,FALSE);
    logrel_set_satisfied(*lrp,FALSE);
  }

  /* Reset status */
  sys->s.iteration = 0;
  sys->s.cpu_elapsed = 0.0;
  sys->s.converged = sys->s.diverged = sys->s.inconsistent = FALSE;
  sys->s.block.previous_total_size = 0;

  /* go to first unconverged block */
  sys->s.block.current_block = -1;
  sys->s.block.current_size = 0;
  sys->s.calc_ok = TRUE;
  sys->s.block.iteration = 0;

  update_status(sys);
}


/*
 * The boundary, relation and logrelation structures in this function
 * are used to perform the solution of logical relations when changing
 * the value of truth of the boundary expressions. This is sometimes
 * required for conditional analysis. The use of the structure instance
 * in this function is an insanity, but we stick with it by now.
 */
static void slv9a_iterate(slv_system_t server, SlvClientToken asys)
{
  slv9a_system_t sys;
  struct bnd_boundary **blist;
  struct bnd_boundary *cur_bnd;
  struct rel_relation *rel;
  struct logrel_relation *logrel;
  struct gl_list_t *per_insts;
  struct Instance *i;
  bnd_filter_t bfilter;
  int32 numbnds,numper,nb;

  FILE              *mif;
  FILE              *lif;
  int               ds_status=0;
  double            time0;

  sys = SLV9A(asys);
  mif = MIF(sys);
  lif = LIF(sys);
  if (server == NULL || sys==NULL) return;
  if (check_system(SLV9A(sys))) return;
  if( !sys->s.ready_to_solve ) {
    FPRINTF(ASCERR,"ERROR:  (slv9a) slv9a_iterate\n");
    FPRINTF(ASCERR,"        Not ready to solve.\n");
    return;
  }

  /*
   * To change truth values of some boundaries
   */
  blist = sys->blist;
  if (blist == NULL && PERTURB_BOUNDARY) {
    FPRINTF(lif,"No boundaries in the problem. The solver cannot\n");
    FPRINTF(lif,"work in perturbation mode \n");
    sys->s.ready_to_solve = FALSE;
    iteration_ends(sys);
    return;
  }

  /*
   * Solution process begins
   */
  if (sys->s.block.current_block==-1) {
    find_next_unconverged_block(sys);
    update_status(sys);
    return;
  }

  /*
   * finding the list of boundaries to be perturbed
   */
  per_insts = NULL;
  if (PERTURB_BOUNDARY) {
    numbnds = slv_get_num_solvers_bnds(server);
    bfilter.matchbits = (BND_PERTURB);
    bfilter.matchvalue = (BND_PERTURB);
    numper = slv_count_solvers_bnds(server,&bfilter);
    if (numper != 0) {
      per_insts = gl_create(numper);
      for (nb=0; nb <numbnds; nb++){
	cur_bnd = blist[nb];
        if(bnd_perturb(cur_bnd)) {
	  if(bnd_kind(cur_bnd) == e_bnd_rel) {
            rel = bnd_rel(bnd_real_cond(cur_bnd));
            i = (struct Instance *)rel_instance(rel);
	    gl_append_ptr(per_insts,i);
	  } else {
	    if (bnd_kind(cur_bnd) == e_bnd_logrel) {
              logrel = bnd_logrel(bnd_log_cond(cur_bnd));
              i = (struct Instance *)logrel_instance(logrel);
	      gl_append_ptr(per_insts,i);
	    }
	  }
	}
      }
    }
  }


  iteration_begins(sys);

  /*
   *  Attempt direct solve if appropriate
   */
  if( sys->s.block.current_size == 1 ) {
    struct dis_discrete *dvar;
    struct logrel_relation *lrel;
    dvar = sys->vlist[mtx_col_to_org(sys->S.mtx,sys->S.reg.col.low)];
    lrel = sys->rlist[mtx_row_to_org(sys->S.mtx,sys->S.reg.row.low)];
    if (SHOW_LESS_IMPT) {
      FPRINTF(lif,"%-40s ---> (%d)", "Singleton relation",
              mtx_row_to_org(sys->S.mtx,sys->S.reg.row.low));
      print_logrel_name(lif,sys,lrel); PUTC('\n',lif);
      FPRINTF(lif,"%-40s ---> (%d)", "Singleton variable",
              mtx_col_to_org(sys->S.mtx,sys->S.reg.col.low));
      print_dis_name(lif,sys,dvar); PUTC('\n',lif);
    }

    /* Attempt direct solve */
    time0=tm_cpu_time();
    if (PERTURB_BOUNDARY && per_insts != NULL) {
      ds_status=slv_direct_log_solve(SERVER,lrel,dvar,mif,1,per_insts);
      gl_destroy(per_insts);
      per_insts = NULL;
    } else {
      ds_status=slv_direct_log_solve(SERVER,lrel,dvar,mif,0,NULL);
    }
    sys->s.block.functime += (tm_cpu_time()-time0);

    switch( ds_status ) {
    case 0:
      if (SHOW_LESS_IMPT) {
        FPRINTF(lif,"Unable to directly solve a logical relation.\n");
      }
      FPRINTF(lif,"Bad discrete variable or logrel\n");
      return;
    case 1:
      if (SHOW_LESS_IMPT) {
        FPRINTF(lif,"Directly solved.\n");
      }
      iteration_ends(sys);
      find_next_unconverged_block(sys);
      update_status(sys);
      return;
    case 2:
      if (SHOW_LESS_IMPT) {
        FPRINTF(lif,"Directly solved.\n");
      }
      sys->s.inconsistent = TRUE;
      FPRINTF(mif,"Multiple solution exists for the discrete variable:\n");
      print_dis_name(mif,sys,dvar); PUTC('\n',mif);
      FPRINTF(mif,"when solving the logical relation:\n");
      print_logrel_name(mif,sys,lrel); PUTC('\n',mif);
      iteration_ends(sys);
      update_status(sys);
      return;
    case -1:
      sys->s.inconsistent = TRUE;
      FPRINTF(mif,"No solution exists for the discrete variable:\n");
      print_dis_name(mif,sys,dvar); PUTC('\n',mif);
      FPRINTF(mif,"when solving the logical relation:\n");
      print_logrel_name(mif,sys,lrel); PUTC('\n',mif);
      iteration_ends(sys);
      update_status(sys);
      return;
    }
  } else {
    FPRINTF(lif,"block number = %d \n",sys->s.block.current_block);
    FPRINTF(lif,"block size = %d \n",sys->s.block.current_size );
    FPRINTF(lif,"block iteration = %d \n",sys->s.block.iteration);
    FPRINTF(lif,"Multiple logical relations in block.\n");
    FPRINTF(lif,"Not supported in this solver.\n");
    iteration_ends(sys);
    update_status(sys);
  }

#if DEBUG
      FPRINTF(ASCERR,"***********end of iteration*****************\n");
      debug_out_dvar_values(LIF(sys), sys);
      debug_out_logrel_residuals(LIF(sys), sys);
      FPRINTF(ASCERR,"********************************************\n");
#endif /* DEBUG */
}


static void slv9a_solve(slv_system_t server, SlvClientToken asys)
{
  slv9a_system_t sys;
  sys = SLV9A(asys);
  if (server == NULL || sys==NULL) return;
  if (check_system(sys)) return;
  while( sys->s.ready_to_solve ) slv9a_iterate(server,sys);
}


static mtx_matrix_t slv9a_get_structural_matrix(slv_system_t server,
                                                SlvClientToken sys)
{
  if (server == NULL || sys==NULL) return NULL;
  if (check_system(SLV9A(sys))) return NULL;
  return SLV9A(sys)->S.mtx;
}

static int slv9a_destroy(slv_system_t server, SlvClientToken asys)
{
  slv9a_system_t sys;
  sys = SLV9A(asys);
  if (server == NULL || sys==NULL) return 1;
  if (check_system(sys)) return 1;
  slv_destroy_parms(&(sys->p));
  destroy_matrices(sys);
  sys->integrity = DESTROYED;
  if (sys->s.cost) ascfree(sys->s.cost);
  ascfree( (POINTER)asys );
  return 0;
}

static void slv9a_dump_internals(slv_system_t server,
				 SlvClientToken sys,int level)
{
  check_system(sys);
  (void) server;
  if (level > 0) {
    FPRINTF(ASCERR,"ERROR:  (slv9a) slv9a_dump_internals\n");
    FPRINTF(ASCERR,"         slv9a does not dump its internals.\n");
  }
}


int slv9a_register(SlvFunctionsT *sft)
{
  if (sft==NULL)  {
    FPRINTF(ASCERR,"slv9a_register called with NULL pointer\n");
    return 1;
  }

  sft->name = "LRSlv";
  sft->ccreate = slv9a_create;
  sft->cdestroy = slv9a_destroy;
  sft->celigible = slv9a_eligible_solver;
  sft->getdefparam = slv9a_get_default_parameters;
  sft->getparam = slv9a_get_parameters;
  sft->setparam = slv9a_set_parameters;
  sft->getstatus = slv9a_get_status;
  sft->solve = slv9a_solve;
  sft->presolve = slv9a_presolve;
  sft->iterate = slv9a_iterate;
  sft->resolve = slv9a_resolve;
  sft->getlinsol = slv9a_get_linsol_sys;
  sft->getlinsys = slv9a_get_linsolqr_sys;
  sft->getsysmtx = slv9a_get_structural_matrix;
  sft->dumpinternals = slv9a_dump_internals;
  return 0;
}

#endif /* #else clause of DYNAMIC_LRSLV */
#endif /* #else clause of !STATIC_LRSLV && !DYNAMIC_LRSLV */

