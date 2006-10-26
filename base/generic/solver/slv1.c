/*
 *  SLV: Ascend Numeric Solver
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.30 $
 *  Version control file: $RCSfile: slv1.c,v $
 *  Date last modified: $Date: 2000/01/25 02:27:21 $
 *  Last modified by: $Author: ballan $
 *
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
 *
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

/*  known bugs:
 *  still uses pl_ functions.
 *  assumes old solver protocol.
 *  doesn't follow relman_eval signal trapping protocol.
 */
#include <stdarg.h>
#include <utilities/ascConfig.h>
#include <utilities/ascSignal.h>
#include <utilities/ascMalloc.h>
#include <general/tm_time.h>
#include <utilities/set.h>
#include <utilities/mem.h>
#include <compiler/compiler.h>
#include <compiler/instance_enum.h>
#include <compiler/fractions.h>
#include <utilities/ascPanic.h>
#include <general/list.h>
#include <compiler/extfunc.h>
#include <compiler/dimen.h>
#include <compiler/expr_types.h>
#include <compiler/find.h>
#include <compiler/relation_type.h>
#include <compiler/rel_blackbox.h>  /* relation enum */
#include <compiler/vlist.h>
#include <compiler/relation.h>  /* relation enum */
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
#include "slv_common.h"
#include "slv_client.h"
#include "slv1.h"
#if !defined(STATIC_MINOS) && !defined(DYNAMIC_MINOS)
/* do nothing */
int slv1_register(SlvFunctionsT *f)
{
  (void)f;  /* stop gcc whine about unused parameter */

  FPRINTF(stderr,"MINOS not compiled in this ASCEND IV.\n");
  return 1;
}
#else /* either STATIC_MINOS or DYNAMIC_MINOS is defined */
#ifdef DYNAMIC_MINOS
/* do dynamic loading stuff.   yeah, right */
#else /* following is used if STATIC_MINOS is defined */

/********************************************************************\
* minos C interface
* ASCEND
* (C) Ben Allan, March 28, 1994
* $Revision: 1.30 $
* $Date: 2000/01/25 02:27:21 $
*
* MINOS 5.4 is proprietary software sitelicensed to Carnegie Mellon.
* Others who wish to use minos with ASCEND must get their own license
* and MINOS 5.4 sources. We provide only interface code to feed problems
* to MINOS 5.4.
*
* Notes: MINOS gets a problem scaled by the ascend nominals, not the
* straight equations. If you change nominals you must (p?)resolve again;
* minos will get very confused if you change nominals on the fly.
*
* minoss call assumptions:
* -nnobj=0 (obj linear) or nnobj=v.nonlinear.
* -all variables in a nonlinear rel/obj are nonlinear.( need better
*  rel_linear primitive.)
*
\********************************************************************/

/*********************************************************************
 Ben Allan 5-8-94
 The MINOS interface parameters, and it's relation to the slv0 shoebox:
 (compare slv.h and MINOS 5.4 User's Guide Chapter 3, and App. A
  Technical Report SOL 83-20R)

 OUTPUT.MORE_IMPORTANT:
 OUTPUT.LESS_IMPORTANT: These will be used by the C interface and the
   jacobian and function calls. The control of MINOS noise is done via
   sub_parameter.
 TOLERANCE.PIVOT:        minos has no semantic equivalent.
 TOLERANCE.SINGULAR:     as minos PIVOT TOLERANCE
 TOLERANCE.FEASIBLE:     as minos ROW TOLERANCE & FEASIBILITY TOLERANCE
 TOLERANCE.STATIONARY:   as minos OPTIMALITY TOLERANCE
 TOLERANCE.TERMINATION:  no semantic equivalent.
 TIME_LIMIT:             use in C same as for slv0
 ITERATION_LIMIT:        as minos MAJOR ITERATIONS
 PARTITION:              minos has no semantic equivalent.
 IGNORE_BOUNDS:          minos disallows this entirely.
 RHO:                    as minos PENALTY PARAMETER

 Subparameters implemented:   (value/meaning)
 [Except as noted, real/integer parameters given a value of 0 by the user will
  default to MINOS values or last legal value set, since 0 is not generally
  legal.]
 sp.ia[SP1_COMPLETION]  0=>PARTIAL, 1=>FULL
 sp.ia[SP1_MINITS]      as minos MINOR ITERATIONS
 sp.ia[SP1_CRASH]       as minos CRASH OPTION (range 0-3)
 sp.ia[SP1_DERIV]       as minos DERIVATIVE LEVEL (range 0-3)
 sp.ia[SP1_CFREQ]       as minos CHECK FREQUENCY
 sp.ia[SP1_FFREQ]       as minos FACTORIZATION FREQUENCY
 sp.ia[SP1_USELG]       as minos LAGRANGIAN (0=>no, 1=> yes)
 sp.ia[SP1_LFREQ]       as minos LOG FREQUENCY
 sp.ia[SP1_MULPR]       as minos MULTIPLE PRICE
 sp.ia[SP1_PARPR]       as minos PARTIAL PRICE
 sp.ia[SP1_JFLXB]       as minos PRINT LEVEL (5 on/off bits, 0 legal)
 sp.ia[SP1_SCALE]       let minos SCALE (0 no, 1 yes, we handle scaling type)
 sp.ia[SP1_SOLN]        as minos SOLUTION except only (0=>no, 1=>yes)
 sp.ia[SP1_PARAM]       as minos SUPPRESS PARAMETERS (0 => no, 1=>yes)
 sp.ia[SP1_VERIFY]      as minos VERIFY LEVEL (range -1 - 3)
 sp.ia[SP1_EFREQ]       as minos EXPAND FREQUENCY
 sp.ia[SP1_SUMMY]       turn on unit 6, if not on, no print options will work.
 sp.ia[SP1_FSUMY]       turn on unit 9 for summary. minos.summary appears in pwd
 sp.ia[SP1_LCONS]       0 => check for linearity, 1 => assume nonlinearity

 sp.ra[SP1_DAMP]        as minos MAJOR DAMPING PARAMETER
 sp.ra[SP1_FDIFF]       as minos DIFFERENCE INTERVAL
 sp.ra[SP1_CDIFF]       as minos CENTRAL DIFFERENCE INTERVAL
 sp.ra[SP1_FPREC]       as minos FUNCTION PRECISION
 sp.ra[SP1_LSTOL]       as minos LINE SEARCH TOLERANCE
 sp.ra[SP1_LUFTO]       as minos LU FACTOR TOLERANCE (>=1.0)
 sp.ra[SP1_LUUTO]       as minos LU UPDATE TOLERANCE (>=1.0)
 sp.ra[SP1_RADIUS]      as minos RADIUS OF CONVERGENCE
 sp.ra[SP1_SUBSP]       as minos SUBSPACE TOLERANCE (range 0.0 - 1.0)
 sp.ra[SP1_OBJLIM]      as minos UNBOUNDED OBJECTIVE VALUE
 sp.ra[SP1_STEPLM]      as minos UNBOUNDED STEP SIZE
 sp.ra[SP1_LOBJWT]      as minos WEIGHT ON LINEAR OBJECTIVE
 sp.ra[SP1_LUDTO]       as minos LU DENSITY TOLERANCE (range 0.0 - 1.0)
 sp.ra[SP1_LUSTO]       as minos LU SINGULARITY TOLERANCE
 sp.ra[SP1_LUWTO]       as minos LU SWAP TOLERANCE
 sp.ra[SP1_MINDAMP]     as minos MINOR DAMPING PARAMETER

 Notes:
 minos ITERATIONS will be set to ITERATION_LIMIT * sp.ia[SP1_MINITS]

  Status flags not implemented:
  sys->s.:
  over_defined
  under_defined
  struct_singular
  Since minos adds variables to the system (slacks) to handle inequalities,
  the ASCEND dof analysis is inaccurate. This could be fixed by anyone who
  is good at counting beans in C.
*********************************************************************/

#define KILL 0
#define slv1_solver_name "MINOS"   /* Solver's name */
#define slv1_solver_number 1   /* Solver's number */

#define SLV1(s) ((slv1_system_t)(s))

#define slv1_IA_SIZE 19
#define slv1_RA_SIZE 16
#define slv1_CA_SIZE 0
#define slv1_VA_SIZE 0
/* subscripts for ia */
#define SP1_COMPLETION 0
#define SP1_MINITS 1
#define SP1_CRASH 2
#define SP1_DERIV 3
#define SP1_CFREQ 4
#define SP1_FFREQ 5
#define SP1_USELG 6
#define SP1_LFREQ 7
#define SP1_MULPR 8
#define SP1_PARPR 9
#define SP1_JFLXB 10
#define SP1_SCALE 11
#define SP1_SOLN 12
#define SP1_PARAM 13
#define SP1_VERIFY 14
#define SP1_EFREQ 15
#define SP1_SUMMY 16
#define SP1_FSUMY 17
#define SP1_LCONS 18
/* subscripts for ra. */
#define SP1_DAMP  0
#define SP1_FDIFF 1
#define SP1_CDIFF 2
#define SP1_FPREC 3
#define SP1_LSTOL 4
#define SP1_LUFTO 5
#define SP1_LUUTO 6
#define SP1_RADIUS 7
#define SP1_SUBSP 8
#define SP1_OBJLIM 9
#define SP1_STEPLM 10
#define SP1_LOBJWT 11
#define SP1_MINDAMP 12
#define SP1_LUDTO 13
#define SP1_LUSTO 14
#define SP1_LUWTO 15
/* subscripts for ca */
/* subscripts for va */

/**********************************\
* NOUNDERBARS --> FORTRAN compiler *
* naming convention for subroutine *
* is wierd.                        *
* CRAY is treated as special case  *
\**********************************/
#ifdef APOLLO
#define NOUNDERBARS TRUE
#endif
#ifdef _HPUX_SOURCE
#define NOUNDERBARS TRUE
#endif


#ifdef NOUNDERBARS
#define FUNCON_SUB_NAME funcon
#define FUNOBJ_SUB_NAME funobj
#define MATMOD_SUB_NAME matmod
#define MISPEC mispec
#define MIOPT miopt
#define MIOPTR mioptr
#define MIOPTI miopti
#define MINOSS minoss
#define GETCOMMON get_minos_common
#else
#define FUNCON_SUB_NAME funcon_
#define FUNOBJ_SUB_NAME funobj_
#define MATMOD_SUB_NAME matmod_
#define MISPEC mispec_
#define MIOPT miopt_
#define MIOPTR mioptr_
#define MIOPTI miopti_
#define MINOSS minoss_
#define GETCOMMON get_minos_common_
#endif

/*
 * Linux sources are typically f2c'ed. The
 * under_bar in get_minos_common causes f2c to add
 * another !!.
 */
#ifdef linux
#undef GETCOMMON
#define GETCOMMON get_minos_common__
#endif

/* CRAY compiler are a warped puppy: */
#ifdef CRAY
#define FUNCON_SUB_NAME FUNCON
#define FUNOBJ_SUB_NAME FUNOBJ
#define MATMOD_SUB_NAME MATMOD
#define MISPEC MISPEC
#define MIOPT MIOPT
#define MIOPTR MIOPTR
#define MIOPTI MIOPTI
#define MINOSS MINOSS
#define GETCOMMON GET_MINOS_COMMON
#endif

/* ALL MINOS ints below are expected to be 32 bits (INTEGER*4) */
/* len on char * are length expected not counting null         */
/* see minos docs and sources for details.                     */
extern MISPEC( int *,    /* ispecs */
               int *,    /* iprint */
               int *,    /* isumm */
               int *,    /* nwcore */
               int *     /* inform */
             );
extern MIOPT ( char *,   /* buffer len 72 */
              int *,     /* iprint */
              int *,     /* isumm */
              int *      /* inform */
             );
extern MIOPTR( char *,   /* buffer len 56 */
               double *, /* rvalue */
               int *,    /* iprint */
               int *,    /* isumm */
               int *     /* inform */
             );
extern MIOPTI( char *,   /* buffer len 56 */
               int *,    /* ivalue */
               int *,    /* iprint */
               int *,    /* isumm */
               int *     /* inform */
             );
extern MINOSS( char *,   /* start len 10 */
               int *,    /* m */
               int *,    /* n */
               int *,    /* nb */
               int *,    /* ne */
               int *,    /* nname */
               int *,    /* nncon */
               int *,    /* nnobj */
               int *,    /* nnjac */
               int *,    /* iobj */
               double *, /* objadd */
               char *,   /* names len 40 */
               double *, /* a */
               int *,    /* ha */
               int *,    /* ka */
               double *, /* bl */
               double *, /* bu */
               int *,    /* name1 */
               int *,    /* name2 */
               int *,    /* hs */
               double *, /* xn */
               double *, /* pi */
               double *, /* rc */
               int *,    /* inform */
               int *,    /* mincor */
               int *,    /* ns */
               int *,    /* ninf */
               double *, /* sinf */
               double *, /* obj  */
               double *, /* z  */
               int *     /* nwcore */
             );

extern GETCOMMON(int * /* major iterations */, int * /* major iterations */);

#define MINOS_DEBUG FALSE
/* if MINOS_DEBUG then some sanity checking is done in calls from f77 to C */
#define D_ZERO (double)0.0
#define D_ONE (double)1.0

struct count_data {
  int32 used;       /* Number of solver vars/included rels */
  int32 nonlinear;  /* Number of non-linear (used) vars/rels
                              non-linear vars are those nonlinear
                              in either rels or objective */
};

struct jacobian_data {
  linsol_system_t sys;   /* Jacobian linear system */
  mtx_matrix_t mtx;      /* Matrix from funcon jacobian system */
  mtx_matrix_t objmtx;   /* Matrix from funobj */
  real64 *rhs;     /* RHS from jacobian system */
};
/* NULL ==> not created.  sys, mtxs, and rhs created and destroyed together */


struct slv1_system_structure {
  int integrity;
  slv_parameters_t p;    /* the standard slv parameters and status */
  int iarray[slv1_IA_SIZE];
  double rarray[slv1_RA_SIZE];
  slv_status_t s;
  int panic;   /* bailout called noninteractively in FUNCON/OBJ */
  double clock;  /* cumulative cpu */

/* Problem definition */
  slv_system_t slv;		     /* slv_system_t back-link */
  struct rel_relation *obj;                        /* Objective function: NULL = none */
  struct var_variable **vlist;             /* Variable list (NULL terminated) */
  struct var_variable **vlist_user;        /* User vlist (NULL = determine) */
  struct rel_relation **rlist;             /* Relation list (NULL terminated) */
  struct rel_relation **rlist_user;        /* User rlist (NULL = none) */

  struct count_data v,r;             /* Variable and relation data */
  int32 maxndx;                /* Maximum index (capacity) */
  int32 mrows;                 /* minos idea of r.used. note that
                                         mrows =1 even if r.used =0 */

/* Arrays allocated at presolve */
  struct jacobian_data jacobian;
  mtx_coord_t *nzlist;   /* Complete list of jacobian elements, C indexed */
  real64
    *z,  /* minos workspace, includes hessian, etc */
    *rc, /* reduced costs as known to minos */
    *pi, /* lagrange multipliers as known to minos for nl constraints */
    *xn, /* variable values (scaled) */
    *bl, /* lower bound values (scaled) */
    *bu, /* upper bound values (scaled) */
    *a;  /* sparse jacobian (scaled) */
  int32
    *ha, /* row fortran indices for a */
    *ka, /* column fortran indices for a, ha */
    *hs; /* variable status (type) vector */

/* scalars */
   int32 itcount;   /* Iteration countdown */
   boolean basis;  /*  basis exists */
/* minos parms */
/* fortran files normally 5=stdin, 6=stdout, 0=no file, 9=`pwd`/minos.summary*/
   int32 ispecs,    /* fortran input unit number */
               iprint,    /* fortran output unit number */
               isumm,     /* fortran output unit number */
               inform,    /* return flag from minos */
               nb,        /* # free vars + # included rels */
               nwcore,    /* minos workspace size */
               njac,      /* # jacobian elements (linear+nonlinear) */
               nnobj;     /* number of nonlinear vars in objective */
};

static slv1_system_t g_sys;   /* Current system: used by FUNOBJ & FUNCON */


/**
 ***  Integrity checks
 ***  ----------------
 ***     check_system(sys)
 **/

#define OK        ((int)491031596)
#define DESTROYED ((int)729104829)
/* note deaddead would be int 3735936685 */
static int check_system(slv1_system_t sys)
/* Checks sys for NULL and for integrity. */
{

  if( sys == NULL ) {
     FPRINTF(stderr,"ERROR:  (slv1) check_system\n");
     FPRINTF(stderr,"        NULL system handle.\n");
     return 1;
  }

  switch( sys->integrity ) {
    case OK:
      return 0;
    case DESTROYED:
      FPRINTF(stderr,"ERROR:  (slv1) check_system\n");
      FPRINTF(stderr,"        System was recently destroyed.\n");
      return 1;
    default:
      FPRINTF(stderr,"ERROR:  (slv1) check_system\n");
      FPRINTF(stderr,"        System reused or never allocated.\n");
      return 1;
  }
}

/*********************************************************************
                  *** Memory management routines ***
free_unless_null(ptr)
zero(arr,nelts,type)
alloc_vector(len)
copy_vector(from,too,len)
free_vector(vec)
make_jacobian(sys)
destroy_jacobian(sys)
*********************************************************************/

static void free_unless_null(POINTER ptr)
{
  if( ptr != NULL )
    ascfree(ptr);
}


#define zero(arr,nelts,type)    \
   mem_zero_byte_cast((arr),0,(nelts)*sizeof(type))
/* Zeros an array of nelts objects, each having given type. */

#define alloc_vector(len) ((real64 *)ascmalloc((len)*sizeof(real64)))
#define alloc_zero_vector(len) ASC_NEW_ARRAY_CLEAR(real64,len)
#define copy_vector(from,too,len)  \
  mem_move_cast((from),(too),(len)*sizeof(real64))
#define free_vector(vec)  \
  free_unless_null((POINTER)(vec))

static void destroy_jacobian(slv1_system_t sys)
/* destroys, deallocates jacobian (sys,mtx, all rhs vectors attached to sys)*/
{
  check_system(sys);
  if( sys->jacobian.sys ) {
    int count = linsol_number_of_rhs(sys->jacobian.sys)-1;
    for( ; count >= 0; count-- )
      free_vector(linsol_get_rhs(sys->jacobian.sys,count));
    mtx_destroy(linsol_get_matrix(sys->jacobian.sys));
    mtx_destroy(sys->jacobian.objmtx);
    linsol_destroy(sys->jacobian.sys);
    sys->jacobian.sys=NULL;
    sys->jacobian.mtx=NULL;
    sys->jacobian.objmtx=NULL;
    sys->jacobian.rhs=NULL;
  }
}

static void make_jacobian(slv1_system_t sys)
/**
 ***  fills linsol, mtx, and one rhs for size sys->maxndx,
 ***  which is assumed set.
 **/
{
  destroy_jacobian(sys);
  sys->jacobian.sys = linsol_create();
  sys->jacobian.mtx = mtx_create();
  sys->jacobian.objmtx = mtx_create();
  mtx_set_order (sys->jacobian.mtx, sys->maxndx );
  mtx_set_order (sys->jacobian.objmtx, sys->maxndx );
  sys->jacobian.rhs = alloc_vector(sys->maxndx);
  linsol_set_matrix(sys->jacobian.sys,sys->jacobian.mtx);
  linsol_add_rhs(sys->jacobian.sys,sys->jacobian.rhs,0);
}
/*********************************************************************
                     *** General I/O routines ***
macros:
print_var_name(out,sys,var)
print_rel_name(out,sys,rel)
*********************************************************************/

static void print_vector (slv1_system_t sys, double * vec,
			  int len, char * name)
{
  int i;
  for (i=0; i<len; i++)
  FPRINTF(LIF(sys),"%s(%d) == %20.15g\n",name,i,vec[i]);
  FFLUSH(LIF(sys));
}

static void print_ivector (slv1_system_t sys, int * vec,int len, char * name) {
  int i;
  for (i=0; i<len; i++)
  FPRINTF(LIF(sys),"%s(%d) == %d\n",name,i,vec[i]);
  FFLUSH(LIF(sys));
}

#define print_var_name(a,b,c) slv_print_var_name((a),(b)->slv,(c))
#define print_rel_name(a,b,c) slv_print_rel_name((a),(b)->slv,(c))

static void print_output(FILE *out,slv1_system_t sys)
{
  struct rel_relation **rp;
  struct var_variable **vp;
  int mrows, nvars,c;
  real64 low,val,high,multiplier;
  int32 index;

  c=0;
  nvars = sys->v.used;
  FPRINTF(out,"%-6s %-12s %-12s %-12s %-12s %-12s\n",
	  "INDEX","LOWER","LEVEL","UPPER","MULTIPLIER","NAME");
  for (rp = sys->rlist;*rp != NULL; rp++) {
    if (rel_included(*rp) && rel_active(*rp) ) {
      index = rel_sindex(*rp);
      low = sys->bl[nvars+c];
      high = sys->bu[nvars+c];
      val = rel_residual(*rp);
      multiplier = sys->pi[c]; /* rel_multiplier(*rp); */
      FPRINTF(out,"  % -6d % -8.4e  % -8.4e  %- 8.4e  %- 8.4e ",
	      index,low,val,high,multiplier);
      print_rel_name(out,sys,*rp);
      PUTC('\n',out);
      c++;
    }
  }
}

/*****/

static void make_nominal_positive(slv1_system_t sys,struct var_variable *var)
/* Makes nominal value of var > 0 */
/* if 0, make it var value unless value 0, in which case, make it 1 */
{
  real64 n = var_nominal(var);

  if( n <= D_ZERO ) {
    FILE *fp = MIF(sys);
    if( n == D_ZERO ) {
      if ( (n=fabs(var_value(var))) > D_ZERO)
        var_set_nominal(var,n);
      else
        var_set_nominal(var,n = D_ONE);
      FPRINTF(fp,"ERROR:  (slv1) make_nominal_positive\n");
      FPRINTF(fp,"        Variable ");
      print_var_name(fp,sys,var);
      FPRINTF(fp,"        \nhas nominal value of zero.\n");
      FPRINTF(fp,"        Resetting to %g\n",n);
    } else {
      n = -n;
      FPRINTF(fp,"ERROR:  (slv1) make_nominal_positive\n");
      FPRINTF(fp,"        Variable ");
      print_var_name(fp,sys,var);
      FPRINTF(fp,"        \nhas negative nominal value.\n");
      FPRINTF(fp,"        Resetting to %g.\n",n);
      var_set_nominal(var,n);
    }
  }
}

/*********************************************************************
iteration_begins(sys)
iteration_ends(sys)
*********************************************************************/

static void iteration_begins(slv1_system_t sys)
/* Prepares sys for an iteration, increasing the iteration counts
   and starting the clock. */
{
  sys->clock = tm_cpu_time();
}

static void iteration_ends(slv1_system_t sys)
/* Prepares sys for exiting an iteration, stopping the clock and recording
   the cpu time, as well as updating the status. */
{
  double cpu_elapsed;   /* elapsed this iteration */
  boolean unsuccessful;

  cpu_elapsed = (double)(tm_cpu_time() - sys->clock);
  sys->s.cpu_elapsed += cpu_elapsed;

  if( !sys->s.converged ) {
    sys->s.block.cpu_elapsed += cpu_elapsed;
    sys->s.time_limit_exceeded =
      (sys->s.block.cpu_elapsed > sys->p.time_limit);
    sys->s.iteration_limit_exceeded =
      (sys->s.block.iteration > sys->p.iteration_limit);
  }

  unsuccessful = sys->s.diverged ||
                 sys->s.inconsistent ||
                 sys->s.iteration_limit_exceeded ||
                 sys->panic ||
                 sys->s.time_limit_exceeded;
  sys->s.ready_to_solve =  !unsuccessful && !sys->s.converged;
  sys->s.ok = !unsuccessful && sys->s.calc_ok && !sys->s.struct_singular;

}

static void install_nlvars(slv1_system_t sys,real64 *values)
/* *values Indexed by column: scaled by var nominal */
/*********************************************************************
Moves nonlinear variables from given array to the value field of
corresponding free variable. unscale in process. (mult x*varnom ->var)
*********************************************************************/
{
  int32 col;
  struct var_variable *var;
  for( col=0 ; col < sys->v.nonlinear ; ++col ) {
    var = sys->vlist[mtx_col_to_org(sys->jacobian.mtx,col)];
/*    var_set_value(var,values[col]*var_nominal(var)); */
    var_set_value(var,values[col]);
  }
}

static void install_lvars(slv1_system_t sys,real64 *values)
/*********************************************************************
Moves linear variables from given array to the value field
of corresponding free variable. unscale in process. (mult x*varnom ->var)
*********************************************************************/
{
  int32 col;
  struct var_variable *var;
  for( col=sys->v.nonlinear ; col < sys->v.used ; ++col ) {
    var = sys->vlist[mtx_col_to_org(sys->jacobian.mtx,col)];
/*    var_set_value(var,values[col]*var_nominal(var)); */
    var_set_value(var,values[col]);
  }
}

static void install_allvars(slv1_system_t sys,real64 *values)
/*********************************************************************
Moves variables from given array to the value field of each free variable.
unscale in process (mult x*varnom ->var)
*********************************************************************/
{
  int32 col;
  struct var_variable *var;
  for( col=0 ; col < sys->v.used ; ++col ) {
    var = sys->vlist[mtx_col_to_org(sys->jacobian.mtx,col)];
/*    var_set_value(var,values[col]*var_nominal(var)); */
    var_set_value(var,values[col]);
  }
}

static void calc_residuals(slv1_system_t sys)
/* Residuals are calculated, norm value is stored in status, and
   sys->s.calc_ok is set accordingly. Knows nothing about scaling */
{
  real64 sum,res;
  struct rel_relation **rp;

  calc_ok = TRUE;
  sum = D_ZERO;
  for( rp=sys->rlist ; *rp != NULL ; ++rp ) {
    if (rel_included(*rp) && rel_active(*rp) ) {
      res = relman_eval(*rp,&calc_ok);
      if( !relman_calc_satisfied(*rp,sys->p.tolerance.feasible) ||
           ( !rel_less(*rp) && !rel_greater(*rp) )
        ) {
        sum += calc_sqr_D0(res);
      }
    }
  }
  if (sum > D_ZERO)
    sys->s.block.residual = calc_sqrt_D0(sum);
  else sys->s.block.residual=sum;
 /* no nonlin vars -> funcon, funobj not called and we must set convergence */
  if (!(sys->v.nonlinear)) {
    if (sys->inform==0) {
      sys->s.converged=TRUE;
    } else {
      if (sys->inform !=3) {
        sys->s.diverged=TRUE;
      }
    }
  }

  if( sys->obj != NULL )
    relman_eval(sys->obj,&calc_ok);   /* Check for OK calculations */
  sys->s.calc_ok = calc_ok;
}

/*********************************************************************
                    *** MINOS subroutines ***
funobj(...)
funcon(...) int are assumed to be FORTRAN i4 in these functions.
*********************************************************************/

void FUNOBJ_SUB_NAME(int *mode,
                int *n,
                double *x,
                double *f,
                double *g,
                int *nstate,
                int *nprob,
                double *z,
                int *nwcore)
/*********************************************************************
Computes the objective function and its gradient.  Called by MINOS.
We should be scaling the obj value, I suspect.
IN
   mode     Calculate gradient iff mode == 2
   n        # of nonlinear obj variables (which is all nl in sys until smarter)
   x        Variable values (scaled) dimension n
   nstate   >=2 ==> final call. minos return will be nstate -2
   nprob    # of problems (so what?)
   z        minos workspace
   nwcore   minos workspace size

OUT
   mode     -1 ==> stop! set if Solv_C_CheckHalt is true.
   f        Value of objective function
   g        Vector of gradients dimension n of objective
            g scaled (mult by var_nominal)
*********************************************************************/
{
  FPRINTF(LIF(g_sys),"FUNOBJ called with nstate = %d\n",*nstate);
/* unscale x and shove it back into ASCEND */
  install_nlvars(g_sys,x);

  print_vector(g_sys,x,*n,"x");
  if (*mode!=0)
    print_vector(g_sys,g,*n,"g");
  print_vector(g_sys,f,1,"f-in");
  if( *nstate >= 2 ) {
    if( *nstate == 2 )
      g_sys->s.converged = TRUE;
    else if( *nstate != 5 )
           g_sys->s.diverged = TRUE;
    return;
  }
  if (*n!=g_sys->v.nonlinear) {
    FPRINTF(MIF(g_sys),"FUNOBJ called with confusing number of variables(%d)",
      *n);
    *mode=-1;
    g_sys->panic=TRUE;
    return;
  }
  calc_ok = TRUE;
  if( (*mode == 2) ) {
    mtx_coord_t nz;
    real64 value;
    var_filter_t vfilter;
    struct var_variable *var;

/* can't hurt, del(nothing) =0, shouldn't ever get here */
    zero(g,*n,double);
    if( g_sys->obj == NULL ) {
      *f = D_ZERO;
      return;
    }

    /* calc free & incident dF/dx  */
    vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_ACTIVE);
    vfilter.matchvalue = (VAR_INCIDENT )| VAR_ACTIVE;
    /*    vfilter.fixed = var_false;
    vfilter.incident = var_true;
    vfilter.in_block = var_ignore; */
    nz.row=(int32)0;
    /* this needs to change */
    mtx_mult_row_zero(g_sys->jacobian.objmtx,nz.row,mtx_ALL_COLS);
    /* discard status = */
      relman_diffs(g_sys->obj, &vfilter, g_sys->jacobian.objmtx,f);
    /* multiply gradient by nominal and put in minos gradient vector */
    nz.col = mtx_FIRST;
    while( value = mtx_next_in_row(g_sys->jacobian.objmtx,&nz,mtx_ALL_COLS),
	  nz.col != mtx_LAST ) {
      var = g_sys->vlist[mtx_col_to_org(g_sys->jacobian.objmtx,nz.col)];
#if MINOS_DEBUG
      if (nz.col >= *n) { /* take this bit out if it works all the time */
        FPRINTF(MIF(g_sys),
          "FUNOBJ stuffing a confused objective gradient(%d)", *n);
        *mode=-1;
        g_sys->panic=TRUE;
        return;
      }
#endif
/*      g[nz.col] = value * var_nominal(var); */
      g[nz.col] = value;
/* *((g_sys->obj->negate==TRUE)? D_ONE : D_ONE) */
      FPRINTF(LIF(g_sys),"d(obj)/d(");
      print_var_name(LIF(g_sys),g_sys,var);
      FPRINTF(LIF(g_sys),")=%20.16g\n",g[nz.col]);
      FPRINTF(LIF(g_sys),"g subscript stuffed: %d\n",nz.col);
    } /* for */
    /* end gradient */
  } else {
     *f = (g_sys->obj==NULL) ? D_ZERO : relman_eval(g_sys->obj,&calc_ok);
     /*  ((g_sys->obj->negate==TRUE)? D_ONE:D_ONE)*exprman_eval(g_sys->obj); */
  }

  iteration_ends(g_sys); /* timout to check interface */
  if ( Solv_C_CheckHalt() ) *mode=-1;
  iteration_begins(g_sys);

  FPRINTF(LIF(g_sys),"FUNOBJ returning obj= %g\nGradient\n",*f);
  if (*mode!=0)
    print_vector(g_sys,g,*n,"gout");
  else
    FPRINTF(LIF(g_sys),"no gradient called for.\n");
  if( !calc_ok )  {
    FPRINTF(MIF(g_sys),"!!FUNOBJ: Warning: calculation error(s).\n");
  }
}

void FUNCON_SUB_NAME( int *mode,
                 int *m,
                 int *n,
                 int  *njac,
                 double *x,
                 double *f,
                 double *g,
                 int *nstate,
                 int *nprob,
                 double *z,
                 int *nwcore)
/*********************************************************************
Computes the jacobian.  Called by MINOS.  Each call updates the
iteration counts.

We really should be scaling the nonlinear residuals. this is lunacy.

For now, don't do energy balances. :-(

Since all variables in non-linear relations/obj are assumed to appear
non-linearly, this routine does not require updating the lvars to be
correct. It will need to update lvars somehow if we sort variables
within a nonlinear equation.

IN
   mode     Calculate the gradient iff mode == 2
   m        # of nonlinear constraints (rows)
   n        # of nonlinear variables (cols)
   njac     # of varying jacobian elements as known by minos
   x        nonlinear variable values (index by column)
   nstate   >=2 ==> solution is optimal. (set converged true)
   nprob    # problems
   z        minos workspace
   nwcore   minos workspace size

OUT
   mode     -1 ==> stop!
   f        Computed residuals of nonlinear relations (scaled?)
   g        Function gradients (scaled (mult by var_nominal) )
      it  seems to be the case that g is a sans lin col/row elements.
*********************************************************************/
{
  int32 row,ndx,gndx,eqn_n;
  struct rel_relation **rp;
  rel_filter_t rfilter;
  var_filter_t vfilter;
  struct var_variable *var;

  FPRINTF(LIF(g_sys),"FUNCON was called with nstate = %d\n",*nstate);

  if (*n!=g_sys->v.nonlinear) {
    FPRINTF(MIF(g_sys),"FUNCON called with confusing number of variables(%d)",
      *n);
    *mode=-1;
    g_sys->panic=TRUE;
    return;
  }

  print_vector(g_sys,x,*n,"x");
  if (*mode!=0)
    print_vector(g_sys,g,*njac,"g");
  print_vector(g_sys,f,*m,"f-in");

  if( *nstate >= 2 ) {
    if( *nstate == 2 )
      g_sys->s.converged = TRUE;
    else
      if( *nstate != 5 )
        g_sys->s.diverged = TRUE;
    return;
  }

  install_nlvars(g_sys,x);
  if( *nstate >= 2 )
     return;

  rfilter.matchbits = (REL_INCLUDED | REL_ACTIVE);
  rfilter.matchvalue = (REL_INCLUDED | REL_ACTIVE);
  vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_ACTIVE);
  /*  vfilter.fixed = var_false;
  vfilter.incident = var_true;
  vfilter.in_block = var_ignore; */
  calc_ok = TRUE;

  if (*mode !=0) { /* skip jacobian if mode 0 */
    /* don't zero the nl jacobian g  as it also may have  constants */
    for( rp=g_sys->rlist; *rp != NULL; rp++ ) {
      eqn_n = rel_sindex(*rp);
      if( rel_apply_filter(*rp,&rfilter) ) {
        /* this needs to change */
        mtx_mult_row_zero(g_sys->jacobian.mtx,
                          mtx_org_to_row(g_sys->jacobian.mtx,eqn_n),
                          mtx_ALL_COLS);
        /* discard status = */
        relman_diffs(*rp,&vfilter,g_sys->jacobian.mtx,
           &(g_sys->jacobian.rhs[eqn_n]));
      }
    }

    if( !calc_ok ) {
      FPRINTF(MIF(g_sys),"!!FUNCON: Warning:jacobian calculation error(s).\n");
      *mode=-1;
    }

    FPRINTF(LIF(g_sys),"FUNCON jacobian and residuals calc-ed\n");
    for( gndx=ndx=0 ; ndx < g_sys->njac ; ++ndx )
    /* don't restuff lrows, just nl rows*/
      if( g_sys->nzlist[ndx].row < g_sys->r.nonlinear ) {
        var = g_sys->vlist[mtx_col_to_org(g_sys->jacobian.mtx,
                                          g_sys->nzlist[ndx].col)];
#if MINOS_DEBUG
        if (gndx>*njac){
          FPRINTF(MIF(g_sys), "FUNCON stuffing a confused jacobian(%d)", *n);
          g_sys->panic=TRUE;
          *mode=-1;
          return;
        }
#endif
        g[gndx++] = mtx_value(g_sys->jacobian.mtx , g_sys->nzlist+ndx);
        /* g[gndx++] = var_nominal(var) * mtx_value(g_sys->jacobian.mtx ,
                                                   g_sys->nzlist+ndx); */
     }
    FPRINTF(LIF(g_sys),"FUNCON minos jacobian stuffed \n");
  } else { /* calc functions only */
    for( rp=g_sys->rlist; *rp != NULL; rp++ ) {
      eqn_n = rel_sindex(*rp);
      if( rel_apply_filter(*rp,&rfilter) ) {
        g_sys->jacobian.rhs[eqn_n] = relman_eval(*rp,&calc_ok);
      }
    }
    if( !calc_ok ) {
      FPRINTF(MIF(g_sys),"!!FUNCON: Warning:jacobian calculation error(s).\n");
      *mode=-1;
    }
  }
  /* stuff function results */
  zero(f,*m,double);
  for( row=0 ; row < *m ; ++row )
    f[row] = g_sys->jacobian.rhs[mtx_row_to_org(g_sys->jacobian.mtx,row)];

  iteration_ends(g_sys); /* timout to check interface */
  if ( Solv_C_CheckHalt() ) *mode=-1;
  iteration_begins(g_sys);

  print_vector(g_sys,x,*n,"x");
  if (*mode!=0)
    print_vector(g_sys,g,*njac,"g");
  print_vector(g_sys,f,*m,"f-out");
}

void MATMOD_SUB_NAME( /* Lots of irrelevent arguments */ )
{
   FPRINTF(stderr,"ERROR:  (slv1) MATMOD_SUB_NAME\n");
   FPRINTF(stderr,"        This should never be called.\n");
}

/*********************************************************************
                     *** MINOS interface ***
make_specs(sys)
insure_bounds(sys,var)
make_bounds(sys)
invoke_minos(sys)
*********************************************************************/


#define THEIR_INFINITY (double)1.0e10

static int make_specs(slv1_system_t sys)
/* stuff the specs into minos */
{
  int i=0,j=0,out=0;
  double db;
  char *s72, *s56;
  FILE * fp, *fpl;
  check_system(sys);
  fp=MIF(sys);
  fpl=LIF(sys);
  sys->inform=0;
  s72=ASC_NEW_ARRAY(char,(73));
  s56=ASC_NEW_ARRAY(char,(57));
  if (!s56 || !s72) {
    FPRINTF(stderr,"ascmalloc failed in make_specs!");
    return (1);
  }

  if (!sys->iarray[SP1_PARAM]) out=6;
  sprintf(s72,"%-72s","Defaults");
  MIOPT(s72,&out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,"minos: make_specs: Unable to reset Defaults.\n");
    sys->inform=0;
    j++;
  }

  sprintf(s72,"%-72s","Timing 0");
  MIOPT(s72,&out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,"minos: make_specs: Unable to kill clock.\n");
    sys->inform=0;
    j++;
  }

  if (sys->iarray[SP1_COMPLETION])
    sprintf(s72,"%-72s","Completion full");
  else
    sprintf(s72,"%-72s","Completion partial");
  MIOPT(s72,&out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,"minos: make_specs: Unable to set completion.\n");
    sys->inform=0;
    j++;
  }

  sprintf(s56,"%-56s","Crash option");
  MIOPTI(s56,&(sys->iarray[SP1_CRASH]),
    &out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,"minos: make_specs: Unable to set crash option.\n");
    sys->inform=0;
    j++;
  }

  sprintf(s56,"%-56s","Derivative level");
  MIOPTI(s56,&(sys->iarray[SP1_DERIV]),
    &out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,"minos: make_specs: Unable to set derivative level.\n");
    sys->inform=0;
    j++;
  }

  sprintf(s72,"%-72s","Jacobian Sparse");
  MIOPT(s72,&out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,"minos: make_specs: Unable to specify jacobian is sparse.\n");
    sys->inform=0;
    j++;
  }

  db=-THEIR_INFINITY;
  sprintf(s56,"%-56s","Lower bound");
  MIOPTR(s56,&db,
         &out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,
      "minos: make_specs: Unable to set lower bound to -their_infinity.\n");
    sys->inform=0;
    j++;
  }

  i=(int)sys->p.iteration_limit;
  sprintf(s56,"%-56s","Major iterations");
  MIOPTI(s56,&i,
         &out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,"minos: make_specs: Unable to set major iteration limit.\n");
    sys->inform=0;
    j++;
  }

  i=sys->iarray[SP1_MINITS];
  sprintf(s56,"%-56s","Minor iterations");
  MIOPTI(s56,&i,
         &out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,"minos: make_specs: Unable to set minor iteration limit.\n");
    sys->inform=0;
    j++;
  }

  i=sys->iarray[SP1_MINITS] * (int)sys->p.iteration_limit;
  sprintf(s56,"%-56s","Iterations limit");
  MIOPTI(s56,&i,
         &out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,"minos: make_specs: Unable to set Iterations limit.\n");
    sys->inform=0;
    j++;
  }

  if (sys->p.tolerance.stationary > D_ZERO) {
    sprintf(s56,"%-56s","Optimality tolerance");
    MIOPTR(s56,&(sys->p.tolerance.stationary),
           &out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,"minos: make_specs: Unable to set Optimality tolerance.\n");
      sys->inform=0;
      j++;
    }
  }

  if (sys->p.tolerance.singular > D_ZERO) {
    sprintf(s56,"%-56s","Pivot tolerance");
    MIOPTR(s56,&(sys->p.tolerance.singular),
           &out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,"minos: make_specs: Unable to set Pivot tolerance.\n");
      sys->inform=0;
      j++;
    }
  }

  if (sys->p.tolerance.feasible > D_ZERO) {
    sprintf(s56,"%-56s","Feasibility tolerance");
    MIOPTR(s56,&(sys->p.tolerance.feasible),
           &out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,"minos: make_specs: Unable to set feasibility tolerance.\n");
      sys->inform=0;
      j++;
    }
    sprintf(s56,"%-56s","Row tolerance");
    MIOPTR(s56,&(sys->p.tolerance.feasible),
           &out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,"minos: make_specs: Unable to set row tolerance.\n");
      sys->inform=0;
      j++;
    }
  }

  sprintf(s56,"%-56s","Major damping parameter");
  MIOPTR(s56,&(sys->rarray[SP1_DAMP]),
         &out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,"minos: make_specs: Unable to set major damping parameter %g.\n",
        sys->rarray[SP1_DAMP]);
    sys->inform=0;
    j++;
  }

  sprintf(s56,"%-56s","Minor damping parameter");
  MIOPTR(s56,&(sys->rarray[SP1_MINDAMP]),
         &out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,"minos: make_specs: Unable to set minor damping parameter %g.\n",
        sys->rarray[SP1_MINDAMP]);
    sys->inform=0;
    j++;
  }

  if (sys->rarray[SP1_FPREC] > D_ZERO) {
    sprintf(s56,"%-56s","Function precision");
    MIOPTR(s56,&(sys->rarray[SP1_FPREC]),
           &out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,"minos: make_specs: Unable to set function precision.\n");
      sys->inform=0;
      j++;
    }
  }

  if (sys->rarray[SP1_LSTOL] > D_ZERO) {
    sprintf(s56,"%-56s","Linesearch tolerance");
    MIOPTR(s56,&(sys->rarray[SP1_LSTOL]),
           &out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,"minos: make_specs: Unable to set line search tolerance.\n");
      sys->inform=0;
      j++;
    }
  }

  if (sys->rarray[SP1_LUFTO] > D_ZERO) {
    sprintf(s56,"%-56s","Lu factor tolerance");
    MIOPTR(s56,&(sys->rarray[SP1_LUFTO]),
           &out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,"minos: make_specs: Unable to set Lu factor tolerance.\n");
      sys->inform=0;
      j++;
    }
  }

  if (sys->rarray[SP1_LUUTO] > D_ZERO) {
    sprintf(s56,"%-56s","Lu update tolerance");
    MIOPTR(s56,&(sys->rarray[SP1_LUUTO]),
           &out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,"minos: make_specs: Unable to set Lu update tolerance.\n");
      sys->inform=0;
      j++;
    }
  }

  if (sys->rarray[SP1_LUDTO] > D_ZERO) {
    sprintf(s56,"%-56s","Lu density tolerance");
    MIOPTR(s56,&(sys->rarray[SP1_LUDTO]),
           &out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,"minos: make_specs: Unable to set Lu density tolerance.\n");
      sys->inform=0;
      j++;
    }
  }

  if (sys->rarray[SP1_LUSTO] > D_ZERO) {
    sprintf(s56,"%-56s","Lu singularity tolerance");
    MIOPTR(s56,&(sys->rarray[SP1_LUSTO]),
           &out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,
        "minos: make_specs: Unable to set Lu singularity tolerance.\n");
      sys->inform=0;
      j++;
    }
  }

  if (sys->rarray[SP1_LUWTO] > D_ZERO) {
    sprintf(s56,"%-56s","Lu swap tolerance");
    MIOPTR(s56,&(sys->rarray[SP1_LUWTO]),
           &out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,"minos: make_specs: Unable to set Lu swap tolerance.\n");
      sys->inform=0;
      j++;
    }
  }

  if (sys->rarray[SP1_RADIUS] > D_ZERO) {
    sprintf(s56,"%-56s","Radius of convergence");
    MIOPTR(s56,&(sys->rarray[SP1_RADIUS]),
           &out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,"minos: make_specs: Unable to set Radius of convergence.\n");
      sys->inform=0;
      j++;
    }
  }

  if (sys->rarray[SP1_SUBSP] > D_ZERO) {
    sprintf(s56,"%-56s","Subspace tolerance");
    MIOPTR(s56,&(sys->rarray[SP1_SUBSP]),
           &out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,"minos: make_specs: Unable to set Subspace tolerance.\n");
      sys->inform=0;
      j++;
    }
  }

  if (sys->rarray[SP1_OBJLIM] > D_ZERO) {
    sprintf(s56,"%-56s","Unbounded objective value");
    MIOPTR(s56,&(sys->rarray[SP1_OBJLIM]),
           &out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,
        "minos: make_specs: Unable to set Unbounded objective value.\n");
      sys->inform=0;
      j++;
    }
  }

  if (sys->rarray[SP1_STEPLM] > D_ZERO) {
    sprintf(s56,"%-56s","Unbounded step size");
    MIOPTR(s56,&(sys->rarray[SP1_STEPLM]),
           &out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,
        "minos: make_specs: Unable to set Unbounded step size.\n");
      sys->inform=0;
      j++;
    }
  }

  if (sys->rarray[SP1_FDIFF] > D_ZERO) {
    sprintf(s56,"%-56s","Difference interval");
    MIOPTR(s56,&(sys->rarray[SP1_FDIFF]),
           &out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,"minos: make_specs: Unable to set difference interval.\n");
      sys->inform=0;
      j++;
    }
  }

  sprintf(s56,"%-56s","Weight on linear objective");
  MIOPTR(s56,&(sys->rarray[SP1_FDIFF]),
         &out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,
      "minos: make_specs: Unable to set Weight on linear objective.\n");
    sys->inform=0;
    j++;
  }

  sprintf(s56,"%-56s","Penalty parameter");
  MIOPTR(s56,&(sys->p.rho),
         &out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,
      "minos: make_specs: Unable to set Penalty parameter(rho)\n");
    sys->inform=0;
    j++;
  }

  if (sys->rarray[SP1_CDIFF] > D_ZERO) {
    sprintf(s56,"%-56s","Central difference interval");
    MIOPTR(s56,&(sys->rarray[SP1_CDIFF]),
           &out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,
        "minos: make_specs: Unable to set central difference interval.\n");
      sys->inform=0;
      j++;
    }
  }

  sprintf(s72,"%-72s","Minimize");
  MIOPT(s72,&out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,"minos: make_specs: Unable to set objective type\n");
    sys->inform=0;
    j++;
  }

  if (sys->iarray[SP1_SCALE])
    sprintf(s72,"%-72s","Scale yes");
  else
    sprintf(s72,"%-72s","Scale no");
  MIOPT(s72,&out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,"minos: make_specs: Unable to set scaling option\n");
    sys->inform=0;
    j++;
  }

  if (sys->iarray[SP1_USELG])
    sprintf(s72,"%-72s","Lagrangian yes");
  else
    sprintf(s72,"%-72s","Lagrangian no");
  MIOPT(s72,&out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,"minos: make_specs: Unable to set lagrangian option\n");
    sys->inform=0;
    j++;
  }

  sprintf(s72,"%-72s","Debug level 0");
  MIOPT(s72,&out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,"minos: make_specs: Unable to set debug option\n");
    sys->inform=0;
    j++;
  }

  if ( (i=sys->iarray[SP1_MULPR]) ) {
    sprintf(s56,"%-56s","Multiple price");
    MIOPTI(s56,&i,
      &out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,"minos: make_specs: Unable to set multiple price\n");
      sys->inform=0;
      j++;
    }
  }

  if ( (i=sys->iarray[SP1_PARPR]) ) {
    sprintf(s56,"%-56s","Partial price");
    MIOPTI(s56,&i,&out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,"minos: make_specs: Unable to set partial price\n");
      sys->inform=0;
      j++;
    }
  }

  sprintf(s56,"%-56s","Print level");
  MIOPTI(s56,&(sys->iarray[SP1_JFLXB]),
    &out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,"minos: make_specs: Unable to set print level\n");
    sys->inform=0;
    j++;
  }

  if (sys->iarray[SP1_SOLN])
    sprintf(s72,"%-72s","Solution yes");
  else
    sprintf(s72,"%-72s","Solution no");
  MIOPT(s72,&out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,"minos: make_specs: Unable to set Solution option\n");
    sys->inform=0;
    j++;
  }

  sprintf(s56,"%-56s","Verify level");
  MIOPTI(s56,&(sys->iarray[SP1_VERIFY]),
    &out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,"minos: make_specs: Unable to set verify option\n");
    sys->inform=0;
    j++;
  }
  if (sys->iarray[SP1_VERIFY]> -1) {
    sprintf(s56,"%-56s","Stop constraint check at column");
    MIOPTI(s56,&(sys->v.used),&out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,"minos: make_specs: Unable to set verify constraint option\n");
      sys->inform=0;
      j++;
    }
    sprintf(s56,"%-56s","Stop objective check at column");
    MIOPTI(s56,&(sys->v.used),&out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,"minos: make_specs: Unable to set verify objective option\n");
      sys->inform=0;
      j++;
    }
  }

  sprintf(s56,"%-56s","Summary frequency");		/* kaa */
  MIOPTI(s56,&(sys->iarray[SP1_LFREQ]),
    &out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,"minos: make_specs: Unable to set log freq. option\n");
    sys->inform=0;
    j++;
  }

  sprintf(s56,"%-56s","Check frequency");
  MIOPTI(s56,&(sys->iarray[SP1_CFREQ]),
    &out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,"minos: make_specs: Unable to set check freq. option\n");
    sys->inform=0;
    j++;
  }

  if (sys->iarray[SP1_EFREQ]>0) {
    sprintf(s56,"%-56s","Expand frequency");
    MIOPTI(s56,&(sys->iarray[SP1_EFREQ]),
      &out,&(sys->isumm), &(sys->inform));
    if ((sys->inform)) {
      FPRINTF(fp,"minos: make_specs: Unable to set expand frequency\n");
      sys->inform=0;
      j++;
    }
  }

  sprintf(s56,"%-56s","Factorization frequency");
  MIOPTI(s56,&(sys->iarray[SP1_FFREQ]),
    &out,&(sys->isumm), &(sys->inform));
  if ((sys->inform)) {
    FPRINTF(fp,"minos: make_specs: Unable to set factorization frequency\n");
    sys->inform=0;
    j++;
  }

  ascfree(s56); s56=NULL;
  ascfree(s72); s72=NULL;
  return (j);
}

static void insure_bounds(slv1_system_t sys,struct var_variable *var)
/**
 ***  Insures that the variable value is within ascend bounds.
 **/
{
  FILE *mif = MIF(sys);
  real64 val,low,high;

  if( sys->p.ignore_bounds )
     return;

  low = var_lower_bound(var);
  high = var_upper_bound(var);
  val = var_value(var);
  if( low > high ) {
    FPRINTF(mif,"Bounds for variable ");
    print_var_name(mif,sys,var);
    FPRINTF(mif," are inconsistent [%g,%g].\n",low,high);
    FPRINTF(mif,"Bounds will be swapped.\n");
    var_set_upper_bound(var, low);
    var_set_lower_bound(var, high);
    low = var_lower_bound(var);
    high = var_upper_bound(var);
  }

  if( low > val ) {
    FPRINTF(mif,"Variable ");
    print_var_name(mif,sys,var);
    FPRINTF(mif," was found below its lower bound.\n");
    FPRINTF(mif,"It will be moved to its lower bound.\n");
    var_set_value(var,low);
  } else if( val > high ) {
    FPRINTF(mif,"Variable ");
    print_var_name(mif,sys,var);
    FPRINTF(mif," was found above its upper bound.\n");
    FPRINTF(mif,"It will be moved to its upper bound.\n");
    var_set_value(var, high);
  }
}

static real64 get_linslack(slv1_system_t sys,
                                 int32 row,
                                 struct rel_relation *rel)
/* This returns the slack of a relation as if the relation were a linear
   equality. When solving a linear system A.x =b with simplex by rearranging
   the system to A.x +s =0, one can either fix RHS=b and drive s->0 or
   fix s=-b (by double bounding) and drive RHS->0. Since Minos doesn't
   have a _convenient_ way to set RHS, we will use the latter approach.
   Simplex lp doesn't notice a difference.
   s(x0)=LHS(x0)-RHS(x0)-A.x0 = resid -Ax. Scaling of A,x won't matter.
   Does not check sanity of rel,row given. nonlinear rels wil give not
   terribly meaningful answers.
   Assumes sys->jacobian.mtx, sys->nzlist stuffed.
   Ignores any fixed variables that have snuck into the jacobian.
*/
{
  real64 s=D_ZERO;
  int ndx;
  struct var_variable *var;

  check_system(sys);
  s=relman_eval(rel,&calc_ok);
  for( ndx=0 ; ndx < sys->njac ; ndx++ ) {
    if( sys->nzlist[ndx].row ==row && sys->nzlist[ndx].col<sys->v.used ) {
      var = sys->vlist[mtx_col_to_org(sys->jacobian.mtx,
                                      sys->nzlist[ndx].col)];
      s += -var_value(var)* mtx_value(sys->jacobian.mtx ,sys->nzlist+ndx);
    }
  }
  return (s);
}

static int make_bounds(slv1_system_t sys)
/* Init bl, bu, xn, a. all are scaled by nominals
   Note: bounds on slacks must be scaled if we start scaling the
   residuals in FUNCON */
{
  int32 row,col,ndx,gndx;
  struct var_variable **vp;
  struct var_variable *var;
  var_filter_t vfilter;
  struct rel_relation **rp;
  rel_filter_t rfilter;
  real64 dummy;

  for( vp = sys->vlist ; *vp != NULL ; ++vp )
    insure_bounds(sys,*vp);

/* check variable bounds */
  for( col=0 ; col < sys->v.used ; ++col ) {
    var = sys->vlist[mtx_col_to_org(sys->jacobian.mtx,col)];

#if MINOS_DEBUG
    FPRINTF(stderr,"setting bounds on ");
    print_var_name(MIF(sys),sys,var);
    FPRINTF(stderr,"\n");
#endif

    if( (sys->bl[col]=var_lower_bound(var)) <=
            -THEIR_INFINITY /* var_nominal(var) */)  {
      sys->bl[col]=-THEIR_INFINITY;
      print_var_name(LIF(sys),sys,var);
      FPRINTF(LIF(sys),
              " has MINOS lower bound (%g) tighter than ASCEND's.\n",
         (-THEIR_INFINITY /* var_nominal(var) */));
    }
    if( (sys->bu[col]=var_upper_bound(var)) >=
             THEIR_INFINITY /* var_nominal(var) */) {
      sys->bu[col]=THEIR_INFINITY;
      print_var_name(LIF(sys),sys,var);
      FPRINTF(LIF(sys)," MINOS upper bound (%g) tighter than ASCEND's.\n",
         (THEIR_INFINITY /* var_nominal(var) */));
    }
#if MINOS_DEBUG
    FPRINTF(stderr,"lo= %g , hi= %g\n", sys->bl[col], sys->bu[col]);
#endif
  }

/* scale variables by nominals */
  for( col=0 ; col < sys->v.used ; ++col ) {
    var = sys->vlist[mtx_col_to_org(sys->jacobian.mtx,col)];
    sys->xn[col]=var_value(var); /* var_nominal(var) */
  }

/* set total scaled jacobian */
  calc_ok = TRUE;
  rfilter.matchbits = (REL_INCLUDED | REL_ACTIVE);
  rfilter.matchvalue = (REL_INCLUDED | REL_ACTIVE);
  vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_ACTIVE);
  /*  vfilter.fixed = var_false;
  vfilter.incident = var_true;
  vfilter.in_block = var_ignore; */
  for( rp=sys->rlist; *rp != NULL; rp++ )
    if( rel_apply_filter(*rp,&rfilter) ) {
      /* this needs to  change */
      mtx_mult_row_zero(sys->jacobian.mtx,
                        mtx_org_to_row(sys->jacobian.mtx,rel_sindex(*rp)),
                        mtx_ALL_COLS);
      /* discard status = */
      relman_diffs(*rp,&vfilter,sys->jacobian.mtx,&dummy);
    }
  if( !calc_ok ) {
    FPRINTF(MIF(sys),"!!make_bounds: jacobian calculation error(s).\n");
    return (1);
  }

  for( gndx=ndx=0 ; ndx < sys->njac ; ++ndx )
    if( sys->nzlist[ndx].row < sys->r.used ) {
      var = sys->vlist[mtx_col_to_org(sys->jacobian.mtx,
                                         sys->nzlist[ndx].col)];
#if MINOS_DEBUG
     /* take this bit out if it works all the time */
      if ((gndx+1)>sys->njac){
        FPRINTF(MIF(sys),
          "make_bounds stuffing a confused gradient(%d)", ndx);
        return (1);
      }
#endif
       sys->a[gndx++] = var_nominal(var) * mtx_value(sys->jacobian.mtx ,
                                                    sys->nzlist+ndx);
    }

/* set slack bounds. do after xn, a are set */
  for (row=0; row < sys->r.used; row++) {
    struct rel_relation *rel = NULL;
    rel = sys->rlist[mtx_row_to_org(sys->jacobian.mtx,row)];
    ndx=row+sys->v.used; /* move up into slack region of xn, bl, bu */

    switch (rel_relop(rel)) {
      case e_equal:
        if (row<sys->r.nonlinear)
          sys->bl[ndx]=sys->bu[ndx]=D_ZERO;
        else
          sys->bl[ndx]=sys->bu[ndx]= get_linslack(sys,row,rel);
          /* starting residual -A.x0 for linear relations. */
        break;
      case e_notequal:
        sys->bl[ndx]= -THEIR_INFINITY;
        sys->bu[ndx]= THEIR_INFINITY;
        break;
      case e_less:
      case e_lesseq:
        if (row<sys->r.nonlinear) {
          sys->bl[ndx]=D_ZERO;
          sys->bu[ndx]=THEIR_INFINITY;
        } else {
          sys->bl[ndx]= get_linslack(sys,row,rel);
          sys->bu[ndx]=THEIR_INFINITY;
        }
        break;
      case e_greater:
      case e_greatereq:
        if (row<sys->r.nonlinear) {
          sys->bu[ndx]=D_ZERO;
          sys->bl[ndx]=-THEIR_INFINITY;
        } else {
          sys->bu[ndx]= get_linslack(sys,row,rel);
          sys->bl[ndx]=-THEIR_INFINITY;
        }
        break;
      default:
        FPRINTF(stderr,"ERROR:  (slv1) make_bounds\n");
        FPRINTF(stderr,"        Undigestible row (%d) passed to MINOS.\n",row);
        return (1);
    }
  } /*for*/
  return (0);
}

static void write_inform(slv1_system_t sys)
/* reports minos exit information */
{
  char * res=NULL; /* do not free this ptr ever */
  FILE * fp=MIF(sys);
  FPRINTF(fp,"\n ***** MINOS inform= %d ***** \n    ",sys->inform);
  switch (sys->inform) {
    case 0  : res="Optimal solution found.";
              break;
    case 1  : res="The problem is infeasible.";
              break;
    case 2  : res="The problem is unbounded (or badly scaled).";
              break;
    case 3  : res="Too many iterations.";
              break;
    case 4  : res="Stall. The solution has not changed lately.";
              break;
    case 5  : res="The Superbasics limit is too small.";
              break;
    case 6  : res="User requested termination in constraint or objective fcn.";
              break;
    case 7  : res="funobj seems to be giving incorrect gradients.";
              break;
    case 8  : res="funcon seems to be giving incorrect gradients.";
              break;
    case 9  : res="The current point cannot be improved.";
              break;
    case 10 : res="The basis is very ill-conditioned.";
              break;
    case 11 : res="Cannot find a superbasic to replace a basic variable.";
              break;
    case 12 : res="Basis factorization requested twice in a row.";
              break;
    case 13 : res="Near optimal solution found.";
              break;
    case 20 : res="Not enough storage for basis factorization.";
              break;
    case 21 : res="Error in basis package.";
              break;
    case 22 : res=
               "The basis is singular after several attempts to factorize it.";
              break;
    case 30 : /* fall through */
    case 31 : res="ASCEND system error.";
              break;
    case 32 : res="MINOS System error. Wrong number of basic variables.";
              break;
    case 40 : res="Fatal errors in the MPS file.(What mps file!?!?)";
              break;
    case 41 : res="Not enough storage to read the MPS file?!?!";
              break;
    case 42 : res="Not enough storage to solve the problem.";
              break;
    default : res="Unknown inform returned from MINOSS";
              break;
  }
  FPRINTF(fp,"%s\n",res);
  return;
}

/*
 * Sets up the interface to MINOS and then invokes it.
 */
static int invoke_minos(slv1_system_t sys)
{
  char *start; 			/* never free this ptr */
  static char names[48]="                                                ";
  static int  nname=1,iobj=0, name1=0,name2=0,mincor,ns,ninf;
  static double objadd=0.0,obj, sinf;
  struct var_variable **vp=NULL;
  check_system(sys);

  if (!sys->s.ready_to_solve)	/* fail if not presolved */
    return (1);
/*
 * Note: due to the bounding process by which linear relations get their
 * if (sys->basis)  start="Cold      ";
 * else start="Warm      ";
 * proper RHS, all starts are cold for now.
 */
  start="Cold      ";
  if (make_specs(sys)>0) {
    FPRINTF(MIF(sys),"Unable to invoke MINOS; bad return from make_specs.\n");
    sys->s.ready_to_solve=FALSE;
    return (1);
  }
  if (make_bounds(sys)>0) {
    FPRINTF(MIF(sys),"Unable to invoke MINOS; bad return from make_bounds.\n");
    sys->s.ready_to_solve=FALSE;
    return (1);
  }

  g_sys = sys;

  FPRINTF(LIF(sys),"About to call minos.\n");
  print_vector(sys,sys->xn,sys->v.used,"xn");
  print_vector(sys,sys->a,sys->njac,"a");
  print_vector(sys,sys->bl,sys->nb,"bl");
  print_vector(sys,sys->bu,sys->nb,"bu");
  /*
   * While !mem ok get more mem. final get will be ok and solve.
   */
  do {
    sys->inform=0;
    iteration_begins(sys);
    MINOSS(start,
          &(sys->mrows),       /* m */
          &(sys->v.used),      /* n */
          &(sys->nb),
          &(sys->njac),        /* ne */
          &nname,
          &(sys->r.nonlinear), /* nncon */
          &(sys->nnobj),
          &(sys->v.nonlinear), /* nnjac */
          &iobj,
          &objadd,
          &names[0],
          sys->a,
          sys->ha,
          sys->ka,
          sys->bl,
          sys->bu,
          &name1,              /* create name1, name2 if nname !=1 */
          &name2,
          sys->hs,
          sys->xn,
          sys->pi,
          sys->rc,
          &(sys->inform),
          &mincor,
          &ns,
          &ninf,
          &sinf,
          &obj,
          sys->z,
          &(sys->nwcore));        /* size of workspace in doubles */
    iteration_ends(sys);
    if (sys->inform==42) {
      free_vector(sys->z);
      sys->nwcore=2*mincor;
      sys->z=alloc_vector(sys->nwcore);
    }
    if (sys->inform==20) {
      free_vector(sys->z);
      sys->nwcore=2*sys->nwcore;
      sys->z=alloc_vector(sys->nwcore);
    }
    FPRINTF(LIF(sys), "MINOS workspace %d bytes.\n",
      (sys->nwcore * (int) sizeof(real64) ) );
  } while (sys->inform ==20 || sys->inform ==42);

  FPRINTF(LIF(sys),"After calling minos.\n");
  print_vector(sys,sys->xn,sys->v.used,"xn");
  print_vector(sys,sys->a,sys->njac,"a");
  print_vector(sys,sys->bl,sys->nb,"bl");
  print_vector(sys,sys->bu,sys->nb,"bu");
  print_vector(sys,sys->pi,sys->mrows,"pi");
  install_allvars(sys,sys->xn);
  sys->basis=TRUE;
  /*
   * note sys->inform==6 => user interface halt in
   * funobj/funcon or eval panic
   */
  write_inform(sys);
  GETCOMMON(&(sys->s.iteration),&(sys->s.block.iteration));
  for( vp = sys->vlist ; *vp != NULL ; ++vp )
     insure_bounds(sys,*vp);
  iteration_begins(sys);
  calc_residuals(sys);
  if (sys->iarray[SP1_LFREQ])		/* added kaa */
    print_output(stdout,sys);
  iteration_ends(sys);
  return 0;
}

/*********************************************************************
                   *** External routines ***
See slv.h
*********************************************************************/

static struct rel_relation **internal_rlist(struct rel_relation * *rlist)
/* Converts rlist to its non-NULL equivalent. */
{
  static struct rel_relation *empty_list[] = {NULL};
  return( rlist==NULL ? empty_list : rlist );
}

static SlvClientToken slv1_create(slv_system_t server, int *statusindex)
{
  slv1_system_t sys;

  sys = (slv1_system_t)ascmalloc( sizeof(struct slv1_system_structure) );
  mem_zero_byte_cast( sys , 0 , sizeof(struct slv1_system_structure) );
  sys->integrity = OK;

  /* Only need to initialize non-zeros */
  sys->p.output.more_important = stdout;
  sys->p.time_limit = (double)30.0;
  sys->p.iteration_limit = 20;
  sys->p.tolerance.singular = (double)1e-8;
  sys->p.tolerance.feasible = (double)1e-6;
  sys->p.tolerance.stationary = (double)1e-8;
  sys->p.partition = TRUE;
  sys->p.whose = slv1_solver_number;
  sys->p.rho = (double)0.1;
  sys->s.ok = TRUE;
  sys->s.calc_ok = TRUE;
  sys->rlist = internal_rlist(NULL);
  sys->nwcore=10000;
  sys->p.sp.iap=&(sys->iarray[0]);
  sys->p.sp.rap=&(sys->rarray[0]);
  sys->iarray[SP1_COMPLETION]=0;
  sys->iarray[SP1_CRASH]=1;
  sys->iarray[SP1_MINITS]=40;
  sys->iarray[SP1_DERIV]=3;
  sys->iarray[SP1_MULPR]=1;
  sys->iarray[SP1_USELG]=1;
  sys->iarray[SP1_VERIFY]=-1;
  sys->iarray[SP1_CFREQ]=30;
  sys->iarray[SP1_LFREQ]=10;
  sys->iarray[SP1_FFREQ]=50;
  sys->iarray[SP1_EFREQ]=10000;
  sys->iarray[SP1_LCONS]=1;  		/* added by kaa */
  sys->rarray[SP1_FPREC]=(double)1.0e-10;
  sys->rarray[SP1_LUFTO]=(double)10;
  sys->rarray[SP1_LUUTO]=(double)10;
  sys->rarray[SP1_LSTOL]=(double)0.1;
  sys->rarray[SP1_RADIUS]=(double)0.01;
  sys->rarray[SP1_SUBSP]=(double)0.5;
  sys->rarray[SP1_OBJLIM]=THEIR_INFINITY;
  sys->rarray[SP1_STEPLM]=(double)1.0e10;
  return(sys);
}

static void slv1_set_rel_list( slv1_system_t sys, struct rel_relation **rlist)
{
  static struct rel_relation *empty_list[] = {NULL};

  check_system(sys);
  sys->rlist_user = rlist;
  sys->rlist = (rlist == NULL ? empty_list : rlist);
  sys->s.ready_to_solve = FALSE;
}

static struct rel_relation **slv1_get_rel_list( slv1_system_t sys)
{
  check_system(sys);
  return( sys->rlist_user );
}

static void slv1_set_var_list(slv1_system_t sys,struct var_variable **vlist)
{
  static struct var_variable *empty_list[] = {NULL};
  check_system(sys);
  if( sys->vlist_user == NULL )
    if( sys->vlist != NULL && pl_length(sys->vlist) > 0 )
      ascfree( (POINTER)(sys->vlist) );
  sys->vlist_user = vlist;
  sys->vlist = (vlist==NULL ? empty_list : vlist);
  sys->s.ready_to_solve = FALSE;
}

static struct var_variable **slv1_get_var_list(slv1_system_t sys)
{
  check_system(sys);
  return( sys->vlist_user );
}

static int slv1_eligible_solver(slv_system_t server)
{
  struct rel_relation **rp;
  for( rp=slv_get_solvers_rel_list(server); *rp != NULL ; ++rp ) {
    if( rel_less(*rp) || rel_greater(*rp) ) return(FALSE);
  }
  return(TRUE);
}

static void slv1_get_parameters(slv_system_t server, SlvClientToken asys,
                         slv_parameters_t *parameters)
{
  slv1_system_t sys;
  sys = SLV1(asys);
  if (check_system(sys)) return;
  mem_copy_cast(&(sys->p),parameters,sizeof(slv_parameters_t));
}

static void slv1_set_parameters(slv_system_t server, SlvClientToken asys,
                         slv_parameters_t *parameters)
{
  slv1_system_t sys;
  sys = SLV1(asys);
  if (check_system(sys)) return;
  mem_copy_cast(parameters,&(sys->p),sizeof(slv_parameters_t));
}

static void slv1_get_status(slv_system_t server, SlvClientToken asys,
                            slv_status_t *status)
{
  slv1_system_t sys;
  sys = SLV1(asys);
  if (check_system(sys)) return;
  mem_copy_cast(&(sys->s),status,sizeof(slv_status_t));
}

static linsolqr_system_t slv1_get_linsolqr_sys(slv_system_t server,
                                               SlvClientToken asys)
{
  return( NULL );
}

static linsol_system_t slv1_get_linsol_sys(slv_system_t server,
                                           SlvClientToken asys)
{
  slv1_system_t sys;
  sys = SLV1(asys);
  if (check_system(sys)) return NULL;
  return sys->jacobian.sys;
}

void slv1_dump_internals(slv_system_t server,
			 SlvClientToken asys,int level)
{
  int i;
  slv1_system_t sys;
  sys = SLV1(asys);
  check_system(sys);
/*  for reference:
   struct jacobian_data jacobian;
   real64
     *z,   minos workspace, includes hessian, etc
     *rc,  reduced costs as known to minos
     *pi,  lagrange multipliers as known to minos for nl constraints
     *bl,  lower bound values (scaled) (nb)
     *bu,  upper bound values (scaled)(nb)
   int32
     *hs;  variable status (type) vector(nb)
*/
  if (level==5) { /*paramvec dump*/
    for (i=0;i<slv1_IA_SIZE;i++)
      FPRINTF(stderr,"iarray[%d]= %d\n",
	i,sys->iarray[i]);
	    for (i=0;i<slv1_RA_SIZE;i++)
      FPRINTF(stderr,"rarray[%d]= %.16g\n",
	i,sys->rarray[i]);
  }

  if (level==4)  /*nzlist dump*/
    for (i=0;i<sys->njac;i++)
      FPRINTF(stderr,"nzl[%d] r%d c%d\n",
	i,sys->nzlist[i].row,sys->nzlist[i].col);

  if (level==2) { /* dump jacobian */
    FPRINTF(stderr,"jacobian:\n");
    for (i=0;  i<sys->njac; i++)
      FPRINTF(stderr,"nzlist(%d)[%d(ha=%d),col=%d] = %g\n",
	i, sys->nzlist[i].row,sys->ha[i],sys->nzlist[i].col, sys->a[i]);
  }
  if (level==3) { /* dump x, col indexes */
    FPRINTF(stderr,"xn vector:\n");
    for (i=0; i<sys->nb; i++)
      FPRINTF(stderr,"jcol(%d)-ka>%d  %g < %g < %g\n",
	i,(i<sys->v.used)?sys->ka[i]:-1, sys->bl[i],sys->xn[i],sys->bu[i]);
  }
  if (level==1) {
    FPRINTF(stderr,"file channels:\n");
    FPRINTF(stderr,"ispecs = %d\n",sys->ispecs);
    FPRINTF(stderr,"iprint = %d\n",sys->iprint);
    FPRINTF(stderr,"isumm = %d\n",sys->isumm);
    write_inform(sys);
    FPRINTF(stderr," maxndx= %d\n",sys->maxndx);
    FPRINTF(stderr,"r.used (minos m) = %d (%d)\n",sys->r.used,sys->mrows);
    FPRINTF(stderr,"r.nonlinear (minos nncon )= %d\n",sys->r.nonlinear);
    FPRINTF(stderr,"v.used (minoss n) = %d\n",sys->v.used);
    FPRINTF(stderr,"v.nonlinear (minos nnjac) = %d\n",sys->v.nonlinear);
    FPRINTF(stderr,"nb= var+con = %d\n",sys->nb);
    FPRINTF(stderr,"nwcore = %d\n",sys->nwcore);
    FPRINTF(stderr,"njac (minos ne) = %d\n",sys->njac);
    FPRINTF(stderr,"nnobj = %d\n",sys->nnobj);
  }
  if (level>10)
    for (i=1;i<10;i++) slv1_dump_internals(server,asys,i);
}

/* sort a range (all with the same column index) in nzlist
so that it is increasing row order. predicated on sparsity.
*/

static int nzsort(mtx_coord_t *l, int32 begin, int32 end)
{
  int32 t,curi,curj;
  if (end<begin) {
#if MINOS_DEBUG
    FPRINTF(stderr, "Garbage sent to nzsort: begin %d, end %d.",begin,end);
#endif
    return 1;
   /* this implies that a var appears in the objective which does not
      appear in the constraints. */
  }
  t=end-begin;
  if (!t) return 0; /* 1 element -> do nothing */
  if (t==1) { /* if pair, be quick */
    if (l[begin].row > l[end].row ) { /* wrong ordered pair */
      t=l[end].row;
      l[end].row=l[begin].row;
      l[begin].row=t;
    }
    return 0;
  }
  for (curi=begin; curi<end; curi++) /* yes, it's the evil bubble sort */
    for (curj=curi+1; curj<=end; curj++)
      if ( l[curi].row > l[curj].row ) {
        t=l[curj].row;
        l[curj].row=l[curi].row;
        l[curi].row=t;
      }
  return 0;
}
/****
* Presolve creates a sparse jacobian matrix with rows/columns reordered:
* (vars in nonlinear rels)(obj vars not in rels if o nl)(linear vars)(unincid)
* and rows in order (nonlinear rels) (linear rels) (unincluded rels)
*
* The jacobian structure is recorded and manipulated via nzlist, an array
* of matrix coordinates. The coordinates refer to positions in the reordered
* matrix, not the original variable list positions. use row/col_to_org to find
* out what positions in the matrix go with what equations/variables.
* The nzlist structure is used to stuff minos a, the total jacobian, and
* minos g the nonlinear constraint derivatives in FUNCON. The array a
* is indexed by arrays ha, ka which must be correctly set for minos sparse
* jacobian option to work right. Note that the indexes in ha, ka count from
* 1 instead of 0.
*
* Elements of ha must be in ascending minos row order within a column.
* hence, elements of nzlist must also be in increasing mtx column,row
* order.
*
* notes: V.used is the number of vars that would pass filter_var_solver, and
*        hence the coord of the first fixed var, if there are any.
*        V.nonlinear is the number of vars in nonlinear rels/objective, and
*        hence the coord of the first linear var, if there are any.
*
*
****/
/* THIS USED TO RETURN AN INT ( = 1 for fail ) */
void slv1_presolve(slv_system_t server, SlvClientToken asys)
{
  slv1_system_t sys = SLV1(asys);
  struct var_variable **vp;
  var_filter_t vfilter;
  struct rel_relation **rp;
  rel_filter_t rfilter;
  mtx_range_t nl_rows;
  mtx_coord_t nz;
  real64 value;
  int32 col,el,ello,elhi,cap;

  check_system(sys);
  if( sys->vlist == NULL ) {
     FPRINTF(stderr,"ERROR:  (slv1) slv1_presolve\n");
     FPRINTF(stderr,"        Variable list was never set.\n");
     return;
  }
  if( sys->rlist == NULL ) {
     FPRINTF(stderr,"ERROR:  (slv1) slv1_presolve\n");
     FPRINTF(stderr,"        Relation list was never set.\n");
     return;
  }

/* iteration_begins() does no damage and is needed so */
/* iteration_ends() can be called afterwards.         */
  iteration_begins(sys);

/* Reset global iteration count and cpu-elapsed. */
  sys->s.iteration = 0;
  sys->s.cpu_elapsed = D_ZERO;
  sys->panic=FALSE;

/* Determine system's extent */
  if( sys->vlist_user == NULL ) {
    Asc_Panic(2, "slv1_presolve",
              "the logic in slve presolve needs to be modified\n");
  }

  sys->maxndx = 0;
  for( vp=sys->vlist,cap=0 ; *vp != NULL ; ++vp ) {
     var_set_sindex(*vp,cap++);
     var_set_in_block(*vp,FALSE);
  }
  sys->maxndx = cap;
  for( rp=sys->rlist,cap=0 ; *rp != NULL ; ++rp ) {
     rel_set_sindex(*rp,cap++);
     rel_set_in_block(*rp,FALSE);
     rel_set_satisfied(*rp,FALSE);
  }
  sys->maxndx = MAX(sys->maxndx,cap);

/* Set up jacobian system. sys,mtx,rhs should be created/destroyed together */
  make_jacobian(sys);

/* This is not the solver's job
   if( sys->obj != NULL )
    exprman_decide_incidence_obj(sys->obj);

  rfilter.matchbits = (REL_INCLUDED | REL_ACTIVE);
  rfilter.matchvalue = (REL_INCLUDED| REL_ACTIVE );
  for( rp=sys->rlist; *rp != NULL; ++rp )
    if( rel_apply_filter(*rp,&rfilter) )
      relman_decide_incidence(*rp);
*/

  vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INCIDENT | VAR _ACTIVE );
  /*
  vfilter.fixed = var_false;
  vfilter.incident = var_true;
  vfilter.in_block = var_ignore; */
  mtx_clear(sys->jacobian.mtx);
  mtx_clear(sys->jacobian.objmtx);
  sys->r.used = sys->r.nonlinear = 0;
  for( rp=sys->rlist; *rp != NULL; ++rp ) { /* if constraints, nl first */
    rel_set_in_block(*rp,FALSE);
    if( rel_apply_filter(*rp,&rfilter) ) {
      rel_set_in_block(*rp,TRUE);
      relman_get_incidence(*rp,&vfilter,sys->jacobian.mtx);
      mtx_swap_rows(sys->jacobian.mtx,rel_sindex(*rp),sys->r.used);
      if( !relman_is_linear(*rp,&vfilter) || sys->iarray[SP1_LCONS] )
        mtx_swap_rows(sys->jacobian.mtx,sys->r.used,sys->r.nonlinear++);
      ++(sys->r.used);
    }
  }

/* put all the nonlinear rows at the top */
  sys->mrows=(sys->r.used==0)? 1:sys->r.used;

/* This bit assumes all free vars in nonlinear relations occur nonlinearly.  */
/* We need badly a var_linear_in_rel/expression boolean */
/* move nl_vars left in constraints and obj mtx */
  nl_rows.low = 0;
  nl_rows.high = sys->r.nonlinear - 1;
  sys->v.used = sys->v.nonlinear = 0;
  for( vp=sys->vlist ; *vp != NULL ; vp++ ) {
    var_set_in_block(*vp,FALSE);
    if( var_apply_filter(*vp,&vfilter) ) {
      make_nominal_positive(sys,*vp);
      var_set_in_block(*vp,TRUE);
/* since solver variable (else wouldn't be here), move var left to edge of
   current solver variable region of jacobian and objective.
*/
      mtx_swap_cols(sys->jacobian.mtx,var_sindex(*vp),(nz.col=sys->v.used));
      mtx_swap_cols(sys->jacobian.objmtx,var_sindex(*vp),(nz.col=sys->v.used));
      nz.row = mtx_FIRST;
/* if this column (var) has an incidence in row range 0, r.nonlinear - 1, move
   it to the left and move up nonlinear marker.
*/
      value = mtx_next_in_col(sys->jacobian.mtx,&nz,&nl_rows);
      if( nz.row != mtx_LAST ) {
	 mtx_swap_cols(sys->jacobian.mtx,sys->v.used,sys->v.nonlinear);
	 mtx_swap_cols(sys->jacobian.objmtx,sys->v.used,sys->v.nonlinear++);
      }
/* move up solver variable marker
*/
      ++(sys->v.used);
    }
  }
    /* go on to the next variable in if */
/* end of for */

/* this bit assumes all free vars in nonlinear objective occur nonlinearly. */
  sys->nnobj=0; /* no nonlinear variables in obj if obj linear or null*/
  if( sys->obj != NULL ) {
    if (!relman_is_linear(sys->obj,&vfilter) || sys->iarray[SP1_LCONS]) {
      /* build a list of incident in obj */
      struct var_variable **objvars;
      int32 col;
      objvars = rel_incidence_list(sys->obj);
      for( vp=objvars ; *vp != NULL ; vp++ )
        if(!var_fixed(*vp) && var_active(*vp) &&
           ( col=mtx_org_to_col(sys->jacobian.objmtx,var_sindex(*vp)) ) >=
           sys->v.nonlinear ) {
          mtx_swap_cols(sys->jacobian.mtx,col,sys->v.nonlinear);
          mtx_swap_cols(sys->jacobian.objmtx,col,sys->v.nonlinear++);
          ++(sys->nnobj);
        }

      FPRINTF(LIF(sys),"Presolve: nnobj= %d,  v.nonlinear= %d\n",
             sys->nnobj,sys->v.nonlinear);
      sys->nnobj=sys->v.nonlinear=MAX(sys->nnobj,sys->v.nonlinear);
      ascfree( (POINTER)objvars );
    }
  }

/*************************************************************************
   From here on in the MINOS invocation algorithm, don't call clear
   on mtx_matrix_t's or you will screw up the nextinrow/col operators
   used for stuffing gradients. mtx_mult_row if 0s needed.
 *************************************************************************/

 /* count nonzeros */
  sys->njac = 0;
  for( nz.row=0 ; nz.row < sys->maxndx ; ++nz.row )
     sys->njac += mtx_nonzeros_in_row(sys->jacobian.mtx,nz.row,mtx_ALL_COLS);
  /* allocate nzlist with at least one element*/
  free_unless_null( (POINTER)(sys->nzlist) );
  sys->nzlist = (mtx_coord_t *)ascmalloc((sys->njac+1) * sizeof(mtx_coord_t));
  sys->nzlist[sys->njac].row=sys->nzlist[sys->njac].col=sys->r.used+5000000;

  /* copy coord to nzlist for use in calc routines */
  for( sys->njac=nz.col=0 ; nz.col < sys->maxndx ; ++nz.col ) {
    nz.row = mtx_FIRST;
    while( value = mtx_next_in_col(sys->jacobian.mtx,&nz,mtx_ALL_ROWS),
	  nz.row != mtx_LAST )
       if( value != D_ZERO )
	  mem_copy_cast(&nz , &(sys->nzlist[sys->njac++]) ,
			sizeof(mtx_coord_t) );
  }
  /* ascmalloc ha, ka */
  free_vector(sys->ha); /* create row index array, and init to 1s  */
  sys->ha=(int *)ascmalloc((sys->njac+1)*sizeof(int));
  for (nz.row=0;nz.row<=sys->njac;nz.row++)
    sys->ha[nz.row]=1; /* was nz.col */

  free_vector(sys->ka); /* create col offset array  */
  sys->ka=ASC_NEW_ARRAY_CLEAR(int,(sys->v.used+1));/*this is not padding*/

  /* secondary rowsort nzlist. already column sorted by creation process */
  if (sys->njac > 1) {
    ello=elhi=el=0;
    for (col=0; col <sys->v.used; col++) {
      /* find next colstart in nzlist */
      for (el=ello;sys->nzlist[el].col<=col;el++);
      elhi=el-1;         /* set this col end*/
      if (!nzsort(sys->nzlist,ello,elhi) ) ello=el;
      /* rowsort nzlist elements from this col */
    }
  }

  /* translate nzlist into ha, ka */
  /* arys number from 1 in fortran, as we have initted ha */
  for (nz.row=0; nz.row<sys->njac;nz.row++)
    sys->ha[nz.row]+=(sys->nzlist[nz.row].row);

  for (nz.row=nz.col=0; nz.row < sys->njac; nz.row++)
    if (sys->nzlist[nz.row].col > nz.col) sys->ka[++nz.col]=nz.row;
  /* bump up the ka to fortran */
  for (nz.row=0; nz.row<sys->v.used; ++(sys->ka[nz.row++]));
  sys->ka[sys->v.used]=sys->njac+1;


  /* never do this, or it may upset the consistency of next_in_row.
  mtx_clear_region(sys->jacobian.mtx,mtx_ENTIRE_MATRIX);
  */
  sys->basis =
  sys->s.over_defined =
  sys->s.under_defined =
  sys->s.struct_singular =
  sys->s.converged =
  sys->s.diverged =
  sys->s.inconsistent = FALSE;
  sys->s.block.number_of = 1;
  sys->nb=sys->mrows+sys->v.used;
  sys->nwcore=110*(pl_length(sys->rlist)+1);
  /* minos guide is 100*r.used. we tend to nonlinearity */

  sys->s.block.previous_total_size = 0;
  sys->s.block.current_block = 0;
  sys->s.block.current_size = sys->maxndx;
  sys->s.block.iteration = 0;
  sys->s.block.cpu_elapsed = D_ZERO;
  sys->s.calc_ok = TRUE;

  free_vector(sys->z); /* create minos workspace */
  sys->z=alloc_zero_vector(sys->nwcore);
  if (!sys->z) {
    FPRINTF(stderr,"Error mallocing minos workspace array!");
    return;
  }

  free_vector(sys->bl); /* create lowerbound array */
  sys->bl=alloc_zero_vector(sys->nb);
  if (!sys->bl) {
    FPRINTF(stderr,"Error mallocing lower bound array!");
    return;
  }

  free_vector(sys->bu); /* create upperbound array */
  sys->bu=alloc_zero_vector(sys->nb);
  if (!sys->bu) {
    FPRINTF(stderr,"Error mallocing upper bound array!");
    return;
  }

  free_vector(sys->hs); /* create var state flag array  */
  sys->hs=ASC_NEW_ARRAY_CLEAR(int,sys->nb);
  if (!sys->hs) {
    FPRINTF(stderr,"Error mallocing variable state array!");
    return;
  }

  free_vector(sys->pi); /* create lagrange multiplier array */
  sys->pi=alloc_zero_vector(sys->mrows);
  if (!sys->pi) {
    FPRINTF(stderr,"Error mallocing multiplier array!");
    return;
  }

  free_vector(sys->rc); /* create reduced cost array */
  sys->rc=alloc_zero_vector(sys->nb);
  if (!sys->rc) {
    FPRINTF(stderr,"Error mallocing reduced cost array!");
    return;
  }

  free_vector(sys->xn); /* create solution array */
  sys->xn=alloc_zero_vector(sys->nb);
  if (!sys->xn) {
    FPRINTF(stderr,"Error mallocing variable array!");
    return;
  }

  free_vector(sys->a); /* create jacobian element array */
  sys->a=alloc_zero_vector(sys->njac+1);
  if (!sys->a) {
    FPRINTF(stderr,"Error mallocing jacobian array!");
    return;
  }

/* set initial i/o unit options and nwcore.*/
  if (sys->iarray[SP1_SUMMY])
    sys->iprint=6;
  else
    sys->iprint=0;
  if (sys->iarray[SP1_FSUMY])
    sys->isumm=9;
  else
    sys->isumm=0;
  sys->inform=0;
  sys->ispecs=0;
  MISPEC(&(sys->ispecs),
         &(sys->iprint),
         &(sys->isumm),
         &(sys->nwcore),
         &(sys->inform));

  iteration_ends(sys);
  return;
}

boolean slv1_change_basis(slv1_system_t sys,int32 var, mtx_range_t *rng){
  return FALSE;
}
/* doesn't work right now */
static void slv1_resolve(slv_system_t server, SlvClientToken asys)
{
  struct rel_relation **rp;
  slv1_system_t sys;
  sys = SLV1(asys);
  check_system(sys);

/* iteration_begins() does no damage and is needed so */
/* iteration_ends() can be called afterwards.         */
   iteration_begins(sys);

/* Reset global iteration count and cpu-elapsed. */
  sys->s.iteration = 0;
  sys->s.cpu_elapsed = D_ZERO;

  for( rp=sys->rlist ; *rp != NULL ; ++rp )
    if( rel_included(*rp) && rel_active(*rp))
      rel_set_satisfied(*rp,FALSE);

  mtx_clear_region(sys->jacobian.mtx,mtx_ENTIRE_MATRIX);
  sys->s.over_defined =
    sys->s.under_defined =
    sys->s.struct_singular =
    sys->s.converged =
    sys->s.diverged =
    sys->s.inconsistent = FALSE;
  sys->s.block.number_of = 1;

  sys->s.block.previous_total_size = 0;
  sys->s.block.current_block = 0;
  sys->s.block.current_size = sys->maxndx;
  sys->s.block.iteration = 0;
  sys->s.block.cpu_elapsed = D_ZERO;
  sys->s.calc_ok = TRUE;

  sys->basis = FALSE;

  iteration_ends(sys);
}

/* the following 2 functions need to be combined (or something) */
static void slv1_solve(slv_system_t server, SlvClientToken asys)
{
  slv1_system_t sys;
  sys = SLV1(asys);
  if (server == NULL || sys==NULL) return;
  if (check_system(SLV1(sys))) return;
  if( !sys->s.ready_to_solve ) {
     FPRINTF(stderr,"ERROR:  (slv1) slv1_solve\n");
     FPRINTF(stderr,"        System is not ready to solve.\n");
     return;
  }
  invoke_minos(sys);
}

static void slv1_iterate(slv_system_t server, SlvClientToken asys)
{
  slv1_system_t sys;
  sys = SLV1(asys);
  if (server == NULL || sys==NULL) return;
  if (check_system(SLV1(sys))) return;
  slv1_solve(server,asys);
}

static mtx_matrix_t slv1_get_jacobian(slv_system_t server, SlvClientToken sys)
{
  if (server == NULL || sys==NULL) return NULL;
  if (check_system(SLV1(sys))) return NULL;
  return SLV1(sys)->jacobian.sys;
}

static int slv1_destroy(slv_system_t server, SlvClientToken asys)
{
  slv1_system_t sys;
  sys = SLV1(asys);
  if (check_system(sys)) return 1;
  slv1_set_var_list(sys,NULL);      /* also destroys list if internal */
  slv1_set_rel_list(sys,NULL);
  /* the above lists and obj will be destroyed by destroy_from_instance */

  destroy_jacobian(sys);
  free_unless_null( (POINTER)(sys->nzlist) );
  sys->nzlist=NULL;

   /* free int lists */
  free_unless_null( (POINTER)(sys->ka) );
  free_unless_null( (POINTER)(sys->hs) );
  free_unless_null( (POINTER)(sys->ha) );
  sys->ha=NULL;
  sys->hs=NULL;
  sys->ka=NULL;

   /* free double lists */
  free_vector(sys->z);
  free_vector(sys->rc);
  free_vector(sys->a);
  free_vector(sys->bu);
  free_vector(sys->bl);
  free_vector(sys->xn);
  free_vector(sys->pi);
  sys->pi=NULL;
  sys->xn=NULL;
  sys->bl=NULL;
  sys->bu=NULL;
  sys->a=NULL;
  sys->rc=NULL;
  sys->z=NULL;

  sys->integrity = DESTROYED;
  ascfree( (POINTER)sys );
  return 0;
}

int slv1_register(SlvFunctionsT *sft)
{
  if (sft==NULL)  {
    FPRINTF(stderr,"slv1_register called with NULL pointer\n");
    return 1;
  }

  sft->name = "MINOS";
  sft->ccreate = slv1_create;
  sft->cdestroy = slv1_destroy;
  sft->celigible = slv1_eligible_solver;
  sft->getparam = slv1_get_parameters;
  sft->setparam = slv1_set_parameters;
  sft->getstatus = slv1_get_status;
  sft->solve = slv1_solve;
  sft->presolve = slv1_presolve;
  sft->iterate = slv1_iterate;
  sft->resolve = slv1_resolve;
  sft->getlinsol = slv1_get_linsol_sys;
  sft->getlinsys = slv1_get_linsolqr_sys;
  sft->getsysmtx = slv1_get_jacobian;
  sft->dumpinternals = slv1_dump_internals;
  return 0;
}
#endif /* #else clause of DYNAMIC_MINOS */
#endif /* #else clause of !STATIC_MINOS && !DYNAMIC_MINOS */
