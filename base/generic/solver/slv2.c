/*
 *  opt rsqp
 *  by Dave Ternet and Ben Allan
 *  Created: 4/98
 *  Version: $Revision: 1.38 $
 *  Version control file: $RCSfile: slv2.c,v $
 *  Date last modified: $Date: 2000/01/25 02:27:26 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1998 Carnegie Mellon University
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

#include "slv2.h"

#include <math.h>
#include <stdarg.h>

#include <utilities/ascConfig.h>
#include <utilities/ascSignal.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <utilities/set.h>
#include <general/tm_time.h>
#include <utilities/mem.h>
#include <compiler/compiler.h>
#include <general/list.h>

#include <compiler/fractions.h>
#include <compiler/dimen.h>
#include <compiler/functype.h>
#include <compiler/func.h>

#include "calc.h"
#include "relman.h"
#include "slv_stdcalls.h"
#include "rsqp.h"
#include "block.h"

/*
 * name of this solver.
 */

#define NAME "rSQP"

/*
 * var/rel list reorderings to yield nice patterns for ma28
 * may be indicated by this enum.
 * Should be translated to a string list parameter later.
 * not all are yet implemented.
 */
enum slv2_reorderings {
  s2_natural,	/* rank not checked! more or less MODEL ordering. */
  s2_spk1,		/* partition/reorder arbitrary basis. */
  s2_tspk1,	/* partition/reorder arbitrary basis. */
  s2_teardrop,	/* partition/reorder arbitrary basis. */
  s2_tteardrop,	/* partition/reorder arbitrary basis. */
  s2_teardropfat,  /* partition/reorder (double border) arbitrary basis. */
  s2_tteardropfat, /* partition/reorder (double border) arbitrary basis. */
  s2_numeric,	/* partition/teardrop/unpartitioned lu for pivot size. */
  s2_random	/* yeah, eat that */
};

#if !defined(STATIC_OPTSQP) && !defined(DYNAMIC_OPTSQP)
int slv2_register(SlvFunctionsT *f)
{
  (void)f;  /* stop gcc whine about unused parameter */

  FPRINTF(stderr,"%s not compiled in this ASCEND IV.\n",NAME);
  return 1;
}
#else /* either STATIC_OPTSQP or DYNAMIC_OPTSQP is defined */

#ifdef DYNAMIC_OPTSQP

/* do dynamic loading stuff.   yeah, right */

#else /* following is used if STATIC_OPTSQP is defined */



/****************  SUBPARAMETERS FOR OPT_ASCEND  **************************

 Subparameter implemented:	(value/meaning)

 EPS       	Convergence tolerance on K-T error, default = 10e-7
 MAXIT		number of iterations allowed.
 KPRINT		Print output -> default = 0
 IDEBUG		0   -> no debug info printed
		1-9 -> debug info printed
 IOPTION	1 -> linesearch only
		2 -> trustregion only
 		3 -> linesearch and trustregion
 ICHOOSE	0 -> MA28 basis
		1 -> first m variables are independent
		2 -> last m variables are independent
 		3 -> get independent vars from d.dat
		4 -> get dependent vars from d.dat
 ICORR		0 -> No correction term
		1 -> Finite difference using multipliers
		2 -> Finite difference without multipliers
		3 -> Broyden update (not available)
		4 -> Limited memory Broyden (not available)
		5 -> Interactive correction
 I_MULT_FREE	0 -> with multipliers
		1 -> without multipliers
 IIEXACT	0 -> Exact penalty function
		1 -> Augmented Lagrangian
		2 -> Watchdog line search (for Maratos effect)

 IVV		0.0   -> use default trust region multipliers, VV=1.0
		value -> use input trust region multiplier, VV
 IDD		0 -> use default trust region shape
		1 -> use input trust region shape, DD

 IPMETHOD       0 -> use active-set method, QPKWIK
		1 -> use interior-point method, QPIP
 ILUSOLVE     271 -> use barrier27full()
              272 -> use barrier27aug()
              280 -> use barrier28big()
              281 -> use barrier28full()
              282 -> use barrier28aug()
              471 -> use barrier47full()
              472 -> use barrier47aug()
              480 -> use barrier48big()
              481 -> use barrier48full()  (default)
              482 -> use barrier48aug()
              483 -> use pre_qpip48new()  [try 481 first, then 484]
              484 -> use pre_qpip48infeas()
              485 -> use barrier48pd()
              486 -> use barrier48mpc()
              487 -> use barrier48ob1()
              488 -> use barrier48gondzio()
 IWARM		0 -> don't use warm start option for QP solver
		1 -> use warm start option for QP solver

 CNR            0 -> no output to file
                1 -> print extra output to file 'journal.num'
 TIMEMAX	number seconds allowed.
***********************************************************************/

#define SERVER (sys->slv)

/* updating to Ken's parameter array implementation which is more flexible */
#define slv2_PA_SIZE 17

#define EPS_PTR			(sys->parm_array[0])
#define EPS 			((*(real64 *)EPS_PTR))
#define MAXIT_PTR		(sys->parm_array[1])
#define MAXIT			((*(int32 *)MAXIT_PTR))
#define KPRINT_PTR		(sys->parm_array[2])
#define KPRINT			((*(int32 *)KPRINT_PTR))
#define IDEBUG_PTR		(sys->parm_array[3])
#define IDEBUG			((*(int32 *)IDEBUG_PTR))
#define IOPTION_PTR		(sys->parm_array[4])
#define IOPTION			((*(int32 *)IOPTION_PTR))
#define ICHOOSE_PTR		(sys->parm_array[5])
#define ICHOOSE			((*(int32 *)ICHOOSE_PTR))
#define ICORR_PTR		(sys->parm_array[6])
#define ICORR			((*(int32 *)ICORR_PTR))
#define I_MULT_FREE_PTR		(sys->parm_array[7])
#define I_MULT_FREE		((*(int32 *)I_MULT_FREE_PTR))
#define IIEXACT_PTR		(sys->parm_array[8])
#define IIEXACT			((*(int32 *)IIEXACT_PTR))
#define IVV_PTR			(sys->parm_array[9])	
#define IVV 			((*(real64 *)IVV_PTR))
#define IDD_PTR			(sys->parm_array[10])	
#define IDD 			((*(int32 *)IDD_PTR))
#define IPMETHOD_PTR		(sys->parm_array[11])	
#define IPMETHOD 		((*(int32 *)IPMETHOD_PTR))
#define ILUSOLVE_PTR		(sys->parm_array[12])	
#define ILUSOLVE		((*(char **)ILUSOLVE_PTR))
#define IWARM_PTR		(sys->parm_array[13])	
#define IWARM 			((*(int32 *)IWARM_PTR))
#define CNR_PTR			(sys->parm_array[14])	
#define CNR 			((*(int32 *)CNR_PTR))
#define TIMEMAX_PTR		(sys->parm_array[15])	
#define TIMEMAX 		((*(real64 *)TIMEMAX_PTR))
/* some ascend side stuff */
#define CUTOFF_PTR		(sys->parm_array[16])
#define CUTOFF			((*(int32 *)CUTOFF_PTR))


#define D_ZERO (double)0.0
#define D_ONE (double)1.0

#define SLV2(a) ((slv2_system_t)(a))

struct slv2_system_structure {

  /* initialized in slv2_create */

  int integrity;
  slv_parameters_t p;    /* the standard slv parameters and status */
  void *parm_array[slv2_PA_SIZE];
  struct slv_parameter padata[slv2_PA_SIZE];
  enum slv2_reorderings reorderstyle;	/* ascend matrix ordering */
  slv_status_t s;
  double clock;				/* cumulative cpu */

  rel_filter_t relfilter, *rfilter;
  var_filter_t varfilter, *vfilter;
  
  /* Problem definition from server */
  slv_system_t           slv;          /* slv_system_t back-link */
  struct rel_relation *obj;            /* Objective function: NULL = none */
  struct var_variable **vlist;         /* Variable list */
  struct rel_relation **rlist;         /* Relation list */

  /* computed at presolve */
  int32 rlen;	/* number of relations opt cares about. first in rlist */
  int32 vlen;	/* number of variables opt cares about. first in vlist */

  int32 maxndx;

  /* push space for common block controls. need to match rsqp.h sizes. */
  real64 rcontrol[RCONTROLSIZE];
  int32 icontrol[ICONTROLSIZE];
  
  /* Arrays allocated at presolve */
  real64 *gsparse;	/* space to store obj gradient from relman */
  int32 *gxsparse;	/* C indices of var in gsparse from relman. */
  int32 gnsparse;	/* size of g/gxsparse. */

  int32 *iw;		/* integer workspace */

  real64
    *rw, 		/* real workspace */
    *xn, 		/* variable values scaled by nominals*/
    *nominals,		/* variable scaling from presolve. */
    *bnds,		/* bound limits */
    *a,  		/* sparse jacobian, scaled by nominals, weights */
    *g,			/* object gradient scaled by nominals, objwt */
    *c,			/* residual scaled by weights */
    *weights,		/* relation scalings. */
    *trust;
  real64 f, objwt;	/* objective and its gradient scaling */

  int32 *avar, *acon; /* column indices and row indices of a */
  int32 *ivec, *jvec; /* jvec and ivec for relman_diff_harwell */

  int32
    md,
    nzd,
    ibnds,
    nfunc,
    ngrad,
    iwsize,
    rwsize;

  int32 inform,  /* opt status? */
        iluoption, /* ILUSOLVE translated */
        njac,    /* # jacobian elements (linear+nonlinear) */
        iter;	/* number of iterations */
};

/**
 ***  Integrity checks
 ***  ----------------
 ***     check_system(sys)
 **/

#define OK        ((int)492031596)
#define DESTROYED ((int)729104829)
/* note deaddead would be int 3735936685 */
static int check_system(slv2_system_t sys)
/* Checks sys for NULL and for integrity. */
{

  if( sys == NULL ) {
     FPRINTF(stderr,"ERROR:  (slv2) check_system\n");
     FPRINTF(stderr,"        NULL system handle.\n");
     return 1;
  }

  switch( sys->integrity ) {
    case OK:
      return 0;
    case DESTROYED:
      FPRINTF(stderr,"ERROR:  (slv2) check_system\n");
      FPRINTF(stderr,"        System was recently destroyed.\n");
      return 1;
    default:
      FPRINTF(stderr,"ERROR:  (slv2) check_system\n");
      FPRINTF(stderr,"        System reused or never allocated.\n");
      return 1;
  }
}

/*********************************************************************
                  *** Memory management routines ***
free_unless_null(ptr)
zero(arr,nelts,type)
alloc_vector(len)
alloc_ivector(len)
free_vector(vec)
*********************************************************************/

static void free_unless_null(POINTER ptr)
{
  if( ptr != NULL )
    ascfree(ptr);
}

static void zerodoublesF(real64 *a, int i)
{
  for (; i > 0; ) {
    i--;
    a[i] = 0.0;
  }
}

static void zerointsF(int32 *a, int i)
{
  for (; i > 0; ) {
    i--;
    a[i] = 0;
  }
}

static real64 *alloc_zero_vector(int len)
{
  double *result;
  result  = ASC_NEW_ARRAY(real64,len);
  if (result != NULL) {
    zerodoublesF(result,len);
  }
  return result;
}

static int32 *alloc_zero_ivector(int len)
{
  int32 *result;
  result  = ASC_NEW_ARRAY(int32,len);
  if (result != NULL) {
    zerointsF(result,len);
  }
  return result;
}

/*
 *  init an array to 0.
 */
#define zerodoubles(arr,nelts,type) zerodoublesF(arr,nelts)
/*
 * create real and integer arrays
 */
real64 *alloc_vector(int len)
{
  real64 *result;
  result = (real64 *)ascmalloc((len+1)*sizeof(real64));
  result[len] = OK;
  return result;
}
int32 *alloc_ivector(int len)
{
  int32 *result;
  result = (int32 *)ascmalloc((len+1)*sizeof(int32));
  result[len] = OK;
  return result;
}

/* destroy the return of both previous alloc_s */
#define free_vector(vec) free_unless_null((POINTER)(vec))

/*********************************************************************
                     *** General I/O routines ***
print_var_name(out,sys,var)
print_rel_name(out,sys,rel)
*********************************************************************/

static void print_vector (slv2_system_t sys, double * vec,int len, char *name)
{
  int i;
  for (i=0; i<len; i++)
  FPRINTF(LIF(sys),"%s(%d) == %20.15g\n",name,i,vec[i]);
  FFLUSH(LIF(sys));
}

static void print_ivector (slv2_system_t sys, int * vec,int len, char *name) {
  int i;
  for (i=0; i<len; i++)
  FPRINTF(LIF(sys),"%s(%d) == %d\n",name,i,vec[i]);
  FFLUSH(LIF(sys));
}

#define print_var_name(a,b,c) slv_print_var_name((a),(b)->slv,(c))
#define print_rel_name(a,b,c) slv_print_rel_name((a),(b)->slv,(c))

/*********************************************************************
    ** routines to work around the insanities of common blocks. **
*********************************************************************/

/* Every entry from an interface must do these or there will be
 * cross chatter among coexisting solvers such as occur in a
 * qp and an nlp alternating. Don't want the qp confusing the nlp.
 * The basic idea is that each slv2_system should push the common
 * info already existing into local storage and then set its own.
 * Substantial speed increase for using slv2_solve instead of iterate.
 * Dave alleges there is no internal common other than these which
 * persist across iterations. In theory this means we only
 * need to call initoptcommon at register time.
 */
void push_controls(int32 *icontrol, real64 *rcontrol)
{
  /* save the old controls to a local vector */
  GET_OPT_COMMON(rcontrol,
                 &(icontrol[0]),
                 &(icontrol[1]),
                 &(icontrol[2]),
                 &(icontrol[3]),
                 &(icontrol[4]),
                 &(icontrol[5]),
                 &(icontrol[6]),
                 &(icontrol[7]),
                 &(icontrol[8]),
                 &(icontrol[9]),
                 &(icontrol[10]),
                 &(icontrol[11])
                );
}
void pop_controls(int32 *icontrol, real64 *rcontrol)
{
  /* restore the old controls from a local vector */
  SET_OPT_COMMON(rcontrol,
                 &(icontrol[0]),
                 &(icontrol[1]),
                 &(icontrol[2]),
                 &(icontrol[3]),
                 &(icontrol[4]),
                 &(icontrol[5]),
                 &(icontrol[6]),
                 &(icontrol[7]),
                 &(icontrol[8]),
                 &(icontrol[9]),
                 &(icontrol[10]),
                 &(icontrol[11])
                );
}


/*********************************************************************
bookkeeping functions
iteration_begins(sys)
iteration_ends(sys)
*********************************************************************/

static void iteration_begins(slv2_system_t sys)
/* Prepares sys for an iteration, increasing the iteration counts
   and starting the clock. */
{
  sys->clock = tm_cpu_time();
}

static void iteration_ends(slv2_system_t sys)
/* Prepares sys for exiting an iteration, stopping the clock and recording
 * the cpu time, as well as updating the status. 
 */
{
  double cpu_elapsed;   /* elapsed this iteration */
  boolean unsuccessful;

  cpu_elapsed = (double)(tm_cpu_time() - sys->clock);
  sys->s.cpu_elapsed += cpu_elapsed;

  if( !sys->s.converged ) {
    sys->s.block.cpu_elapsed += cpu_elapsed;
    sys->s.time_limit_exceeded =
      (sys->s.block.cpu_elapsed > sys->p.time_limit);
  }

  unsuccessful = sys->s.diverged ||
                 sys->s.inconsistent ||
                 sys->s.iteration_limit_exceeded ||
                 sys->s.time_limit_exceeded;
/* maybe add this line in a little later
 *		 (sys->vlen <= sys->rlen);
 */
  sys->s.ready_to_solve =  !unsuccessful && !sys->s.converged;
  sys->s.ok = !unsuccessful && sys->s.calc_ok && !sys->s.struct_singular;

}


/* Residuals are calculated, norm value is stored in status, and
 * sys->s.calc_ok is set accordingly. Knows nothing about scaling.
 * this is out of date with slv3 standard, if that's a standard...
 * This function should not be used for the solution process.
 */
static void calc_residuals(slv2_system_t sys)
{
  real64 sum,res;
  struct rel_relation **rp;

  calc_ok = TRUE;
  sum = D_ZERO;
  Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
  for( rp=sys->rlist ; *rp != NULL ; ++rp ) {
    if (rel_included(*rp) && rel_active(*rp)) {
      res = relman_eval(*rp,&calc_ok,1/* safe always*/);
      if( !relman_calc_satisfied(*rp,sys->p.tolerance.feasible) ||
           ( !rel_less(*rp) && !rel_greater(*rp) )
        ) {
        sum += calc_sqr_D0(res);
      }
    }
  }
  Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
  if (sum > D_ZERO) {
    sys->s.block.residual = calc_sqrt_D0(sum);
  } else {
    sys->s.block.residual = sum;
  }

}

static void install_allvars(slv2_system_t sys, real64 *values)
/*********************************************************************
Moves variables from given array to the value field of each variable
so that ascend can do evaluation..
*********************************************************************/
{
  int32 col;
  struct var_variable *var;
  for( col=0 ; col < sys->vlen ; ++col ) {
    var = sys->vlist[col];
    var_set_value(var,values[col]*sys->nominals[col]);
  }
}



/*********************************************************************
          *** MODEL server utilities for OPT_ASCEND interface ***
*********************************************************************/

/*
 * slv2_calc_C -- constraint residuals.
 * slv2_calc_A -- constraint jacobian.
 * slv2_calc_F -- objective.
 * slv2_calc_G -- objective gradient.
 * 
 * All these functions should automagically scale the problem
 * from sys->nominals and sys->wts which should
 * be calculated only at the beginning or after throwing
 * out the accumulated Hessian following a huge error reduction.
 *
 * all return 0 if able to evaluate at sys->xn or nonzero
 * if some error was seen.
 */

static int slv2_calc_C(slv2_system_t sys)
{
  int32 row;

  calc_ok = TRUE;
  Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
  for( row = 0; row < sys->rlen; row++ ) {
    sys->c[row] = 
      relman_eval(sys->rlist[row],&calc_ok,1) * sys->weights[row];
  }
  Asc_SignalHandlerPop(SIGFPE,SIG_IGN);

  if( !calc_ok ) {
    FPRINTF(MIF(sys),"!Warning:residual calculation error(s).\n");
    return 1;
  }
  return 0;
}

/* 
 * computes scaled gradient a. if firsttime != 0, also
 * fill sys->acon, sys->avar, sys->ivec, sys->jvec, sys->weights.
 * return 0 if ok, -1 if severe error, n > 0 equation errors.
 */
static int slv2_calc_A(slv2_system_t sys, int firsttime)
{
  int32 k, row;
  real64 nominal;
  int result=0;

  Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
  if (!firsttime) {
    result = 
      relman_diff_harwell(sys->rlist, sys->vfilter,sys->rfilter,sys->rlen,0,3,
                          sys->a,NULL,NULL);
    if (result == 1) {
      Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
      return -1;
    }
    if (result == 0) {
      for (k = sys->njac-1; k >= 0; k--) {
        /* we computed nominals and weights so this is multiplication */
        sys->a[k] = 
          sys->a[k] * sys->weights[sys->ivec[k]] * sys->nominals[sys->jvec[k]];
      }
    } else {
      result = 1;
      FPRINTF(MIF(sys),"!Warning: jacobian calculation error(s).\n");
      for (k = sys->njac-1; k >= 0; k--) {
        if (isnan(sys->a[k]) || !finite(sys->a[k])) {
          sys->a[k] = 0; /* fp error --> singular row probably */
        }
        sys->a[k] = sys->a[k]
                    * sys->weights[sys->ivec[k]] * sys->nominals[sys->jvec[k]];
      }
    }
  } else {
    /* first time through compute weights, ivec, acon, jvec, avar */
    result =
      relman_diff_harwell(sys->rlist, sys->vfilter,sys->rfilter,sys->rlen,0,3,
                          sys->a,sys->ivec,sys->jvec);
    if (result == 1) {
      Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
      return -1;
    }
    if (result < 0) {
      FPRINTF(MIF(sys),
        "!Warning: jacobian calculation error(s) at initial point.\n");
      result = 1;
    }
    if (/* 2norm option */ 1) {
      k = 0;
      for (row = 0 ; row < sys->rlen; row++) {
        nominal = 0;
        while (sys->ivec[k] == row) {
          nominal += sys->a[k]*sys->a[k];
          k++;
        }
        nominal = sqrt(nominal);
        if (!isnan(nominal) && finite(nominal) && nominal != 0.0) {
          sys->weights[row] = 1/nominal;
        } else {
          result++;
          sys->weights[row] = 1;
        }
      }
    } else { 
      /* ascend relation scaling */
      for (row = 0 ; row < sys->rlen; row++) {
        nominal = relman_scale(sys->rlist[row]);
        nominal = sqrt(nominal);
        if (!isnan(nominal) && finite(nominal) && nominal != 0.0) {
          sys->weights[row] = 1/nominal;
        } else {
          result++;
          sys->weights[row] = 1;
        }
      }
    }
    for (k = sys->njac-1; k >= 0; k--) {
      sys->acon[k] = sys->ivec[k] + 1;
      sys->avar[k] = sys->jvec[k] + 1;
      sys->a[k] = sys->a[k] 
                  * sys->weights[sys->ivec[k]] * sys->nominals[sys->jvec[k]];
    }
  }
  Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
  FPRINTF(LIF(sys),"jacobian calculated\n");
  return result;
}

/* return 0 if ok 1 if error */
static int slv2_calc_F(slv2_system_t sys)
{
  int result=0;
  if( sys->obj != NULL ) {
    Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
    sys->f = relman_eval(sys->obj,&calc_ok,1) * sys->objwt;  
    if (isnan(sys->f) && !finite(sys->f)) {
      result = 1;
    }
    Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
  }
  sys->s.calc_ok = calc_ok;
  return result;
}

static int slv2_calc_G(slv2_system_t sys, int firsttime)
{
  int status, ndx, result = 0;
  real64 nominal;

  if ( sys->obj != NULL ) {
    Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
    status = relman_diff2(sys->obj, sys->vfilter, sys->gsparse,
                          sys->gxsparse, &(sys->gnsparse),0);
    if (firsttime) {
      if (/*2norm option */1) {
        nominal = 0;
        for (ndx = 0; ndx < sys->gnsparse; ndx++) {
          nominal += sys->gsparse[ndx] * sys->gsparse[ndx];
        }
        nominal = sqrt(nominal);
        if (!isnan(nominal) && finite(nominal) && nominal != 0.0) {
          sys->objwt = 1/nominal;
        } else {
          sys->objwt = 1;
          result++;
        }
      } else {
        nominal = relman_scale(sys->obj);
        nominal = sqrt(nominal);
        if (!isnan(nominal) && finite(nominal) && nominal != 0.0) {
          sys->objwt = 1/nominal;
        } else {
          result++;
          sys->objwt = 1;
        }
      }
    }

    for (ndx = 0; ndx < sys->gnsparse; ndx++) {
      if (!isnan(sys->gsparse[ndx]) && finite(sys->gsparse[ndx])) {
        sys->g[sys->gxsparse[ndx]] = sys->gsparse[ndx] 
           * sys->objwt * sys->nominals[sys->jvec[sys->gxsparse[ndx]]];
      } else {
        sys->g[sys->gxsparse[ndx]] = 0;
      }

      FPRINTF(LIF(sys),"d(obj)/d(");
      print_var_name(LIF(sys),sys,sys->vlist[ndx]);
      FPRINTF(LIF(sys),") = %20.16g\n",sys->g[ndx]);
      FPRINTF(LIF(sys),"g (f77) subscript stuffed: %d\n",
               sys->gxsparse[ndx]+1);

    }
    Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
  }
  if( !calc_ok || result) { 
    FPRINTF(MIF(sys),"!Warning: objective gradient calculation error(s).\n");
    return 1;
  }
  return 0;
}


/*********************************************************************
                     *** OPT_ASCEND interface ***
int scale_and_bounds(sys)
invoke_opt(sys)
*********************************************************************/


#define THEIR_INFINITY (double)1.0e20

/* Init bnds, xn, acon, avar, a.
 * sys->nominals is found.
 * bounds are scaled by the var nominals after checking ascend sanity.
 * init point (scaled by nominals) is moved inside theirinfinity bounds.
 * returns nonzero if gradient errors seen at initial point.
 */
static 
int scale_and_bounds(slv2_system_t sys)
{
  int32 col,ndx;
  real64 nominal;
  int result=0;
  struct var_variable **vp;
  struct var_variable *var;

  slv_ensure_bounds(SERVER,0,sys->vlen,MIF(sys));

  for( col=0 ; col < sys->vlen ; ++col ) {
    ndx = 2*col;
    var = sys->vlist[col];

    sys->xn[col] = var_value(var)/nominal;

    nominal = fabs(var_nominal(var));
    sys->nominals[ndx] = nominal;
#define NEWTONLIMIT 0.1 
    /* Needs to be moved to parameters list.
     * May want to move trust to solver_var instead of using this calc.
     */
    sys->trust[ndx] = nominal * NEWTONLIMIT;

    sys->bnds[ndx] = var_lower_bound(var)/nominal;
    if (sys->bnds[ndx] <= -THEIR_INFINITY)  {
      sys->bnds[ndx] = -THEIR_INFINITY;
      print_var_name(LIF(sys),sys,var);
      FPRINTF(LIF(sys), " has %s lower bound (%g) tighter than ASCEND's.\n",
              NAME,(-THEIR_INFINITY)*nominal);
    }

    ndx++;
    sys->bnds[ndx] = var_upper_bound(var)/nominal;
    if (sys->bnds[ndx] >= THEIR_INFINITY) {
      sys->bnds[ndx] = THEIR_INFINITY;
      print_var_name(LIF(sys),sys,var);
      FPRINTF(LIF(sys)," %s upper bound (%g) tighter than ASCEND's.\n",
         NAME, THEIR_INFINITY*nominal);
    }
  }

  /* init acon, avar, ivec, jvec, weights */
  if (slv2_calc_A(sys,1)) {
    result++; 
  }

  /* init objwt */
  if (slv2_calc_G(sys,1)) {
    result++; 
  }

  return result;
}

static void write_inform(slv2_system_t sys)
/* reports opt exit information */
{
  char * res=NULL; /* do not free this ptr ever */
  FILE * fp=MIF(sys);
  FPRINTF(fp,"\n ***** OPT_ASCEND inform= %d ***** \n    ",sys->inform);
  switch (sys->inform) {
    case -10: res="End of iteration.";
	      break;
    case -1 : res="Ready to start a problem.";
	      break;
    case 0  : res="Starting in OPT_ASCEND";
	      break;
    case 1  : res="Optimal solution found.";
              break;
    case 6  : res="User requested termination?";
	      break;
    case 10 : res="Optimal solution to square system found.";
	      break;
    case 42 : res="Need to change the size of iw and/or rw";
              break;
    case 55 : res="There is a problem with the bounds on a variable";
	      sys->s.diverged = TRUE;
              break;
    case 57 : res="Basis isn't chosen correctly, check ICHOOSE";
	      sys->s.diverged = TRUE;
              break;
    case 58 : res="Maximum number of iterations exceeded";
	      sys->s.iteration_limit_exceeded = TRUE;
	      break;
    case 73 : res="Line search failure";
	      sys->s.diverged = TRUE;
              break;
    case 78 : res="singular pivot in correction.f";
	      sys->s.diverged = TRUE;
              break;
    case 101 :
    case 102 :
    case 103 :
    case 104 :
    case 105 :
    case 106 :
    case 107 :
    case 108 :
    case 109 :
    case 110 :
    case 111 :
    case 112 :
    case 113 :
    case 114 :
    case 115 :
    case 116 :
    case 117 :
    case 118 :
    case 119 :
    case 120 : res="Iarray or Rarray sizes too small";
	      sys->s.diverged = TRUE;
	      break;
    case 177 :
    case 178 : res="null pointer returned allocing iw or rw";
	      sys->s.diverged = TRUE;
	      break;
    default : res="Unknown inform returned from OPT_ASCEND";
	      sys->s.diverged = TRUE;
              break;
  }
  FPRINTF(fp,"%s\n",res);
  return;
}

/*
 * returns 1 if we have not hit some opt or user stopping condition
 */
static
int go_again(slv2_system_t sys)
{
  if (Solv_C_CheckHalt()) {
    sys->inform = 6;
  }
  return   ((sys->inform !=   1 &&	/* optimal solution found */
	     sys->inform !=   6 &&	/* user interrupt */
	     sys->inform !=  10 &&	/* opt. sol. to sqare sys. found */
	     sys->inform != -10 &&	/* end of one iteration */
	     sys->inform !=  57 &&	/* ichoose incorrect */
	     sys->inform !=  58 &&	/* max iter. reached */
	     sys->inform !=  73 &&	/* line search failure */
	     sys->inform <= 100    ) || /* workspace sizes too small */
	     sys->inform ==  42);	/* reallocate memory */
}


/* Sets up the interface to OPT_ASCEND and then invokes it. */
static int invoke_opt(slv2_system_t sys)
{
  int  i;
  real64 fout,e1out,e2out,ynout,znout,alfaout;
  int32 nactout;
  


  if (!sys->s.ready_to_solve) return (1); /*fail if not presolved*/

  print_vector(sys,sys->xn,sys->vlen,"xn");
  print_vector(sys,sys->a,sys->njac,"a");
  print_vector(sys,sys->bnds,2*sys->vlen,"bnds");
  print_ivector(sys,sys->avar,sys->njac, "avar");
  print_ivector(sys,sys->acon,sys->njac, "acon");

  iteration_begins(sys);

  do {
    if( sys->inform ==  -1 /* first call from ascend after presolve. */ ||
	sys->inform ==   0 ||
	sys->inform == -10 ||
	sys->inform == -15 ||
	sys->inform == -25   ) {
      sys->ngrad = sys->ngrad + 1;
      slv2_calc_G(sys,0);
      slv2_calc_A(sys,0);
    }

    if( sys->inform ==  -1 ||
	sys->inform ==   0 ||
	sys->inform == -20 ||
	sys->inform == -25   ) {
      sys->nfunc = sys->nfunc + 1;
      slv2_calc_F(sys);
      slv2_calc_C(sys);
    }

    if ( IDEBUG != 0 ) {
      PRINTF("\nn=%3d\tm=%3d\tmd=%3d\n", sys->vlen,sys->rlen,sys->md);
      if ( sys->vlen <= 100 ) {
	for(i=0; i<sys->vlen; i++) {
	  PRINTF("\ti=%3d\tlo=%g\tx[i]=%g\thi=%g\n",
		i,sys->bnds[2*i],sys->xn[i],sys->bnds[2*i+1]);
        }
	for(i=0; i<sys->vlen; i++) {
	  PRINTF("\tg[%2d]=%g", i, sys->g[i]);
        }
      }
      PRINTF("\nf=%f\tnjac=%3d\tnzd=%3d\n", sys->f,sys->njac,sys->nzd);

      PRINTF("\n");
      if ( sys->rlen <= 100 ) {
	for(i=0; i<sys->rlen; i++) {
	  PRINTF("\tc[%2d]=%g", i, sys->c[i]);
        }
      }

      PRINTF("\n");
      if ( sys->njac <= 100 ) {
	for(i=0; i<sys->njac; i++) {
	  PRINTF("\ta[%3d]=%g", i, sys->a[i]);
        }
      }
      PRINTF("\nibnds=%2d\tinf=%.8f\n", sys->ibnds,sys->inform);
      PRINTF("\nliw=%d\tlrw=%d", sys->iwsize,sys->rwsize);
    }

    OPT_ASCEND(&(sys->vlen),			/* n */
	       &(sys->rlen),			/* m */
	       &(sys->md),			/* md */
	       (sys->xn),			/* x */
	       &(sys->f),			/* f */
	       (sys->g),			/* g */
	       (sys->c),			/* c */
	       (sys->a),			/* a */
	       &(sys->njac),			/* nz */
	       &(sys->nzd),			/* nzd */
	       (sys->avar),			/* avar */
	       (sys->acon),			/* acon */
	       &(sys->ibnds),			/* ibnds */
	       (sys->bnds),			/* bnds */
	       &(sys->inform),			/* inf */
	       (sys->trust),			/* trust */
	       &(sys->iwsize),			/* liw */
	       (sys->iw),			/* iw */
	       &(sys->rwsize),			/* lrw */
	       (sys->rw));			/* rw */

    if (sys->inform==42) {
      free_vector(sys->iw);
      free_vector(sys->rw);
      PRINTF("\niwsize = %d, rwsize = %d\n", sys->iwsize, sys->rwsize);
      sys->iw = alloc_ivector(sys->iwsize);
      if ( sys->iw == NULL ) {
	PRINTF("\nsys->iw is null, too much memory required");
	sys->inform == 177;
      }
      sys->rw = alloc_vector(sys->rwsize);
      if ( sys->rw == NULL ) {
	PRINTF("\nsys->rw is null, too much memory required");
	sys->inform == 178;
      }
      sys->inform = -1;
      FPRINTF(LIF(sys),
	"OPT_ASCEND workspace %d bytes.\n",(sys->rwsize*sizeof(real64)));
      FPRINTF(LIF(sys),
	"OPT_ASCEND workspace %d bytes.\n",(sys->iwsize*sizeof(int32)));
      continue;
    }
    GET_OPT_OUTPUT(&fout,&e1out,&e2out,&ynout,&znout,&alfaout,&nactout);
    FPRINTF(MIF(sys),"Iter\tObjective\tmax|glag|\tmax|Ci|\tYpYN\tZpZN\tNACT\talfa\n");
    FPRINTF(MIF(sys),"%d\t%.12g\t%g\t%g\t%g\t%g\t%d%g\n",
      sys->iter,fout,e1out,e2out,ynout,znout,nactout,alfaout);

  } while ( go_again(sys) );

  iteration_ends(sys);

  PRINTF("\niterations=%d\tfunc.evals=%d\tgrad.evals=%d",
         sys->iter,sys->nfunc,sys->ngrad);

  if (sys->inform == 1 || sys->inform == 10) {
    sys->s.converged = 1;
    sys->s.block.previous_total_size = sys->vlen;
  }

  sys->s.iteration = sys->nfunc;
  sys->s.block.iteration = sys->ngrad;

  print_vector(sys,sys->xn,sys->vlen,"x");
  print_vector(sys,sys->a,sys->njac,"a");
  print_vector(sys,sys->bnds,2*sys->vlen,"bnds");

  install_allvars(sys,sys->xn);

  write_inform(sys);

  iteration_begins(sys);
  calc_residuals(sys);
  iteration_ends(sys);
}


/*********************************************************************
                   *** External routines ***
See slv.h and rsqp.h
*********************************************************************/

static char *ilu_names[] = {
                        "271-barrier27full()", "272-barrier27aug()", /*0,1*/
  "280-barrier28big()", "281-barrier28full()", "282-barrier28aug()", /*2,3,4*/
                        "471-barrier47full()", "472-barrier27aug()", /*5,6*/
  "480-barrier48big()", "481-barrier48full()", "482-barrier48aug()", /*7,8,9*/
             "483-barrier48new()", "484-barrier48infeas()",          /*10,11*/
             "485-barrier48pd()",  "486-barrier48mpc()",             /*12,13*/
             "487-barrier48ob1()", "488-barrier48gondzio()"          /*14,15*/
};

/* converts string input to matching integer */
static 
int32 ilu_name_to_int(slv2_system_t sys)
{
  int option, status;
  char *s; 

  s = ILUSOLVE;

  status = sscanf(s,"%d",&option);
  if (status == 1) {
    /* matched leading integer */
#ifndef __CMU__
    /* harwell source for ma27,ma47,ma48 not available outside cmu normally */
    if (option < 280 || option > 289) {
      if (sys->iluoption < 280 || sys->iluoption >289) {
        sys->iluoption = 280;
      }
      return sys->iluoption;
    }
#endif
    sys->iluoption = option;
  }
  /* OTHERWISE leave default alone */
  return sys->iluoption;
}

/* converts integer input to matching string index */
/* returns 2 if nothing matched which should be 280 */
static int32 ilu_name_from_int(int32 option)
{
  int  i;
  char buf[20];

  sprintf(buf,"%d",option);
  for (i = 0; i < (sizeof(ilu_names)/sizeof(char *)); i++) {
    if (strncmp(buf,ilu_names[i],3)==0) {
      return i;
    }
  }
  return 2;
}

/* this may be called with NULL, NULL, p */
static
int32 slv2_get_default_parameters(slv_system_t server, SlvClientToken asys,
                                  slv_parameters_t *parameters)
{
  slv2_system_t sys; int isize; int rsize, dummy;
  real64 rcontrol[RCONTROLSIZE];
  int32 icontrol[ICONTROLSIZE];
  real64 rdefault[RCONTROLSIZE];
  int32 idefault[ICONTROLSIZE];
  union parm_arg lo,hi,val;
  struct slv_parameter *new_parms = NULL;
  int32 make_macros = 0;

  /* here call init common, call get common, and set our defaults
   * from those defaults subject to ascend limits on some.
   * reset common block in each major call from the interface
   * from our defaults since we don't know if there are two
   * systems sharing the same common.
   */
  
  push_controls(icontrol,rcontrol);
  INIT_OPT_DEFAULTS();

  push_controls(idefault,rdefault); /* don't pop this one, stuff defaults. */
  if (server != NULL && asys != NULL) {
    sys = SLV2(asys);
    make_macros = 1;
  }

#ifndef NDEBUG  /* keep purify from whining on UMR */
  lo.argr = hi.argr = val.argr = 0.0;
#endif

  if (parameters->parms == NULL) {
    new_parms = (struct slv_parameter *)
      ascmalloc((slv2_PA_SIZE)*sizeof(struct slv_parameter));
    if (new_parms == NULL) {
      pop_controls(icontrol,rcontrol);
      return -1;
    }
    parameters->parms = new_parms;
    parameters->dynamic_parms = 1;
  }
  parameters->num_parms = 0;

/*
 * define control parameters here.
 * BPARM is a boolean (or 0/1 int) parameter.
 * RPARM IS_A double parameter.
 * IPARM IS_A integer parameter (not 0/1).
 * CPARM IS_A array of strings parameter (rarely seen in fortran).
 */

  slv_define_parm(parameters, real_parm,
               "EPS", "convergence tolerance (EPS)",
               "Convergence tolerance on K-T error",
               U_p_real(val,rdefault[0]),
               U_p_real(lo,1e-20),U_p_real(hi,10), 1);
  SLV_RPARM_MACRO(EPS_PTR,parameters);

  slv_define_parm(parameters, int_parm,
               "MAXIT", "iteration limit (MAXIT)",
               "iteration limit (MAXIT)",
               U_p_int(val,idefault[0]),U_p_int(lo,1),U_p_int(hi,10000),1);
  SLV_IPARM_MACRO(MAXIT_PTR,parameters);

#ifndef __WIN32__ /* pc fortran cannot do prints to stdout (file 6) */
  slv_define_parm(parameters, bool_parm,
               "KPRINT", "print output (KPRINT)", "print output",
               U_p_bool(val,idefault[1]),U_p_bool(lo,0),U_p_bool(hi,1), 2);
  SLV_BPARM_MACRO(KPRINT_PTR,parameters);
#else
  /* force FALSE value */
  slv_define_parm(parameters, bool_parm,
               "KPRINT", "print output (KPRINT)", "print output",
               U_p_bool(val,0),U_p_bool(lo,0),U_p_bool(hi,0), 2);
  SLV_BPARM_MACRO(KPRINT_PTR,parameters);
#endif /* __WIN32__ */

#ifndef __WIN32__ /* pc fortran cannot do prints to stdout (file 6) */
  slv_define_parm(parameters, bool_parm,
               "IDEBUG", "print debug (IDEBUG)", "print debug",
               U_p_bool(val,idefault[2]),U_p_bool(lo,0),U_p_bool(hi,1), 2);
  SLV_BPARM_MACRO(IDEBUG_PTR,parameters);
#else
  /* force FALSE value */
  slv_define_parm(parameters, bool_parm,
               "IDEBUG", "print debug (IDEBUG)", "print debug",
               U_p_bool(val,0),U_p_bool(lo,0),U_p_bool(hi,0), 2);
  SLV_BPARM_MACRO(IDEBUG_PTR,parameters);
#endif /* __WIN32__ */

  slv_define_parm(parameters, int_parm,
               "IOPTION", "convergence option (IOPTION)",
               "1-linesearch, 2-trustregion, 3-both",
               U_p_int(val,idefault[3]),U_p_int(lo,1),U_p_int(hi,3),1);
  SLV_IPARM_MACRO(IOPTION_PTR,parameters);

  slv_define_parm(parameters, int_parm,
               "ICHOOSE", "basis selection (ICHOOSE)", 
               "selection: 0-numeric, 1-first columns, 2-last columns",
               U_p_int(val,idefault[4]),U_p_int(lo,0),U_p_int(hi,2),1);
  SLV_IPARM_MACRO(ICHOOSE_PTR,parameters);

#ifndef __WIN32__
  slv_define_parm(parameters, int_parm,
               "ICORR", "correction type (ICORR)", 
               "correction: 0-none, 1-FD with mult., 2-FD only, " /* no comma */
               "5-Interactive (FD/none)",
               U_p_int(val,idefault[5]),U_p_int(lo,0),U_p_int(hi,5),2);
  SLV_IPARM_MACRO(ICORR_PTR,parameters);
#else
  /* no interactive input on pcs */
  slv_define_parm(parameters, int_parm,
               "ICORR", "correction type (ICORR)", 
               "correction: 0-none, 1-Finite Difference with multiplier," 
               " 2-Finite Difference only",
               U_p_int(val,idefault[5]),U_p_int(lo,0),U_p_int(hi,2),2);
  SLV_IPARM_MACRO(ICORR_PTR,parameters);
#endif

  slv_define_parm(parameters, bool_parm,
               "I_MULT_FREE", "multiplier option (I_MULT_FREE)",
               "0-calculate multipliers, 1-multiplier free",
               U_p_bool(val,idefault[6]),U_p_bool(lo,0),U_p_bool(hi,1),2);
  SLV_BPARM_MACRO(I_MULT_FREE_PTR,parameters);

  slv_define_parm(parameters, int_parm,
               "IIEXACT", "merit function for linesearch (IIEXACT)",
               "0-none, 1-exact, 2-Watchdog",
               U_p_int(val, idefault[7]),U_p_int(lo, 0),U_p_int(hi,2),1);
  SLV_IPARM_MACRO(IIEXACT_PTR,parameters);

  slv_define_parm(parameters, bool_parm,
               "IPMETHOD", "Interior point QP solver (IPMETHOD)",
               "0-use active-set method, 1-use interior-point method",
               U_p_bool(val,idefault[8]),U_p_bool(lo,0),U_p_bool(hi,1), 1);
  SLV_BPARM_MACRO(IPMETHOD_PTR,parameters);


  slv_define_parm(parameters, char_parm,
               "ILUSOLVE", "Barrier method type for IP method (ILUSOLVE)",
               "combinations of many routines",
               U_p_string(val,ilu_names[ilu_name_from_int(idefault[9])]),
               U_p_strings(lo,ilu_names),
               U_p_int(hi,sizeof(ilu_names)/sizeof(char *)),1);
  SLV_CPARM_MACRO(ILUSOLVE_PTR,parameters);

  slv_define_parm(parameters, bool_parm,
               "IWARM", "warm start for QP solver (IWARM)",
               "0-don't use warm start strategy, 1-use warm start strategy",
               U_p_bool(val,idefault[10]),U_p_bool(lo,0),U_p_bool(hi,1), 2);
  SLV_BPARM_MACRO(IWARM_PTR,parameters);

  slv_define_parm(parameters, real_parm,
               "IVV", "initial trust region multiplier value (IVV)",
               "0.0-use default value, value-initial multiplier value",
               U_p_real(val,0.0),U_p_real(lo,0.0),U_p_real(hi,100000),2);
  SLV_RPARM_MACRO(IVV_PTR,parameters);

  slv_define_parm(parameters, bool_parm,
               "IDD", "input trust region shape (IDD)",
               "0-use default shape, 1-input trust region shape",
               U_p_bool(val,0),U_p_bool(lo,0),U_p_bool(hi,1), 2);
  SLV_BPARM_MACRO(IDD_PTR,parameters);

#ifndef __WIN32__ /* pc fortran cannot do prints to stdout (file 6) */
  slv_define_parm(parameters, bool_parm,
               "CNR", "file output option (CNR)",
               "0-no output to file, 1-print output to journal.num",
               U_p_bool(val,idefault[11]),U_p_bool(lo,0),U_p_bool(hi,1), 2);
  SLV_BPARM_MACRO(CNR_PTR,parameters);
#else
  /* force FALSE value */
  slv_define_parm(parameters, bool_parm,
               "CNR", "file output option (CNR)",
               "0-no output to file, 1-print output to journal.num",
               U_p_bool(val,0),U_p_bool(lo,0),U_p_bool(hi,0), 2);
  SLV_BPARM_MACRO(CNR_PTR,parameters);
#endif /* __WIN32__ */

  slv_define_parm(parameters, real_parm,
               "TIMEMAX", "maximum CPU time",
               "most CPU seconds allowed without returning to user",
               U_p_real(val,180),U_p_real(lo, 1),U_p_real(hi,1800), 1);
  SLV_RPARM_MACRO(TIMEMAX_PTR,parameters);

  slv_define_parm(parameters, int_parm,
               "cutoff", "block size cutoff (MODEL-based)",
               "block size cutoff ( MODEL-based)",
               U_p_int(val, 500),U_p_int(lo,0),U_p_int(hi,20000), 2);
  SLV_IPARM_MACRO(CUTOFF_PTR,parameters);

  if (parameters->num_parms != slv2_PA_SIZE) {
    FPRINTF(stderr,"%s parameter count (%d) != expected (%d) in %s.\n",
            NAME,parameters->num_parms,slv2_PA_SIZE,__FILE__);
  }

  pop_controls(icontrol,rcontrol);
  return 0;
}

/*********************** END lbopt interface ******************************/


/* pretty close. fixme */
static SlvClientToken slv2_create(slv_system_t server, int *statusindex)
{
  slv2_system_t sys;

  sys = (slv2_system_t)ascmalloc( sizeof(struct slv2_system_structure) );
  if (sys==NULL) {
    *statusindex = 1;
    return sys;
  }
  SERVER = server;
  /* init scalars for slv2 */
  sys->integrity = OK;
  sys->reorderstyle = s2_natural;

  sys->p.parms = sys->padata;
  sys->p.dynamic_parms = 0;
  sys->p.output.more_important = stdout;
  sys->p.output.less_important = stdout;
  sys->p.tolerance.feasible = (double)1e-6; /* ? what is rsqp param? */
  sys->p.whose = (*statusindex);

  sys->s.ok = TRUE;
  sys->s.calc_ok = TRUE;
  sys->s.costsize = 0;
  sys->s.cost = NULL; 

  /* fetch MODEL server info */

  sys->rlist = slv_get_solvers_rel_list(SERVER);
  sys->vlist = slv_get_solvers_var_list(SERVER);
  sys->obj = slv_get_obj_relation(SERVER);

  if (sys->vlist == NULL) {
    ascfree(sys);
    FPRINTF(stderr,"%s called with no variables.\n",NAME);
    *statusindex = -2;
    return NULL; 
  }
  if (sys->rlist == NULL && sys->obj == NULL) {
    ascfree(sys);
    FPRINTF(stderr,"%s called with no relations or objective.\n",NAME);
    *statusindex = -1;
    return NULL; 
  }

  /* run server checks */
  slv_check_var_initialization(SERVER);

  /* get control defaults from fortran */
  slv2_get_default_parameters(SERVER,(SlvClientToken)sys,&(sys->p));

  sys->rfilter = &(sys->relfilter);
  sys->vfilter = &(sys->varfilter);
  sys->relfilter.matchbits = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  sys->relfilter.matchvalue = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  sys->varfilter.matchbits = (VAR_INCIDENT | VAR_SVAR | VAR_FIXED | VAR_ACTIVE);
  sys->varfilter.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);

  /* init to NULL stuff to be used in presolve, etc */
  sys->acon	= NULL;
  sys->avar	= NULL;
  sys->gxsparse	= NULL;
  sys->ivec	= NULL;
  sys->jvec	= NULL;
  sys->iw	= NULL;

  sys->a	= NULL;
  sys->bnds	= NULL;
  sys->c	= NULL;
  sys->g	= NULL;
  sys->gsparse	= NULL;
  sys->nominals	= NULL;
  sys->xn	= NULL;
  sys->trust	= NULL;
  sys->rw	= NULL;
  sys->weights	= NULL;

  /* do this in presolve?? */
  sys->iwsize = 10000; /* ??? */
  sys->rwsize = 20000;

  *statusindex = 0;
  return (SlvClientToken)sys;
}


/*
 * 
 */
static
int slv2_destroy(slv_system_t server, SlvClientToken asys)
{
  slv2_system_t sys;
  (void) server;
  sys = SLV2(asys);
  if (check_system(sys)) return 1;

  slv_destroy_parms(&(sys->p));

   /* free int lists */
  free_vector(sys->acon );
  free_vector(sys->avar);
  free_vector(sys->gxsparse);
  free_vector(sys->ivec);
  free_vector(sys->jvec);
  free_vector(sys->iw);

  sys->acon	= NULL;
  sys->avar	= NULL;
  sys->gxsparse	= NULL;
  sys->ivec	= NULL;
  sys->jvec	= NULL;
  sys->iw	= NULL;

   /* free double lists */
  free_vector(sys->a);
  free_vector(sys->bnds);
  free_vector(sys->c);
  free_vector(sys->g);
  free_vector(sys->gsparse);
  free_vector(sys->nominals);
  free_vector(sys->xn);
  free_vector(sys->trust);
  free_vector(sys->rw);
  free_vector(sys->weights);

  sys->a	= NULL;
  sys->bnds	= NULL;
  sys->c	= NULL;
  sys->g	= NULL;
  sys->gsparse	= NULL;
  sys->nominals	= NULL;
  sys->xn	= NULL;
  sys->trust	= NULL;
  sys->rw	= NULL;
  sys->weights	= NULL;

  sys->integrity = DESTROYED;
  ascfree( (POINTER)sys );
  return 0;
}

/*************************************************************************
 * problem setup functions.
 ************************************************************************/

/* clear inblock flags after reordering so next block is happy */
static void clear_inblock(slv2_system_t sys, const mtx_block_t *b, int32 bnum)
{
  mtx_region_t reg;
  int32 row,col;

  reg = b->block[bnum];
  for( col = reg.col.low; col <= reg.col.high; col++ ) {
    var_set_in_block(sys->vlist[col],FALSE);
  }
  for( row = reg.row.low; row <= reg.row.high; row++ ) {
    rel_set_in_block(sys->rlist[row],FALSE);
  }
}

/* reset the block in question to square. since we
 * do *nothing* with blocks after the calling function, we don't care.
 */
static void square_block(const mtx_block_t *b, int32 bnum)
{
  if (b->block[bnum].col.high > b->block[bnum].row.high) {
    b->block[bnum].col.high = b->block[bnum].row.high;
    return;
  }
  if (b->block[bnum].row.high > b->block[bnum].col.high) {
    b->block[bnum].row.high = b->block[bnum].col.high;
    return;
  }
}

/* move equalities to rlist [0..rlen-1] and free vars to 0..vlen-1]
 * and calculate rlen, vlen.
 * Inequalities, if any, are dropped.
 * Opt has provisions that allow us to munge inequality to equality
 * if desired in future.
 * Preconditioning ma28 by reorderings can be done here.
 * potentially lots of wasted effort here.
 */
static
int slv2_rearrange_lists(slv2_system_t sys)
{
  int32 bnum,i;
  const mtx_block_t *b;
 
  /* calculate sys->reorderstyle from future string parameter here */

  slv_sort_rels_and_vars(SERVER,&(sys->rlen),&(sys->vlen));
  switch (sys->reorderstyle) {
  case s2_tspk1:
    slv_block_partition(SERVER); /* prolly --> rectangle block */
    b = slv_get_solvers_blocks(SERVER);
    if (b != NULL && b->nblocks > 0) {
      bnum = b->nblocks; 
      for (i=0; i < bnum; i++) {
        square_block(b,i);
        slv_spk1_reorder_block(SERVER,i,1);
        clear_inblock(sys,b,i);
      } 
    }
    break;
  case s2_spk1:
    slv_block_partition(SERVER); /* prolly --> rectangle block */
    b = slv_get_solvers_blocks(SERVER);
    if (b != NULL && b->nblocks > 0) {
      bnum = b->nblocks; 
      for (i=0; i < bnum; i++) {
        square_block(b,i);
        slv_spk1_reorder_block(SERVER,i,0);
        clear_inblock(sys,b,i);
      } 
    }
    break;
  case s2_tteardrop:
    slv_block_partition(SERVER); /* prolly --> rectangle block */
    b = slv_get_solvers_blocks(SERVER);
    if (b != NULL && b->nblocks > 0) {
      bnum = b->nblocks; 
      for (i=0; i < bnum; i++) {
        slv_tear_drop_reorder_block(SERVER,i,CUTOFF,0,mtx_TSPK1);
        clear_inblock(sys,b,i);
      } 
    }
    break;
  case s2_teardrop:
    slv_block_partition(SERVER); /* prolly --> rectangle block */
    b = slv_get_solvers_blocks(SERVER);
    if (b != NULL && b->nblocks > 0) {
      bnum = b->nblocks; 
      for (i=0; i < bnum; i++) {
        slv_tear_drop_reorder_block(SERVER,i,CUTOFF,0,mtx_SPK1);
        clear_inblock(sys,b,i);
      } 
    }
    break;
  case s2_tteardropfat:
    slv_block_partition(SERVER); /* prolly --> rectangle block */
    b = slv_get_solvers_blocks(SERVER);
    if (b != NULL && b->nblocks > 0) {
      bnum = b->nblocks; 
      for (i=0; i < bnum; i++) {
        slv_tear_drop_reorder_block(SERVER,i,CUTOFF,1,mtx_TSPK1);
        clear_inblock(sys,b,i);
      } 
    }
    break;
  case s2_teardropfat:
    slv_block_partition(SERVER); /* prolly --> rectangle block */
    b = slv_get_solvers_blocks(SERVER);
    if (b != NULL && b->nblocks > 0) {
      bnum = b->nblocks; 
      for (i=0; i < bnum; i++) {
        slv_tear_drop_reorder_block(SERVER,i,CUTOFF,1,mtx_SPK1);
        clear_inblock(sys,b,i);
      } 
    }
    break;
  case s2_random: /* not yet implemented */
  case s2_numeric: /* not yet implemented */
  case s2_natural: /* natural is the default */
  default:
    break;
  }
  return 0;
}

/*
 * Presolve allocates all the needed workspaces and initializes
 * control parameters.
PRESO
 */
static
void slv2_presolve(slv_system_t server, SlvClientToken asys)
{
  int32 big;
  struct var_variable **vp;
  struct rel_relation **rp;
  int32 col, row, ind, dummy;
  real64 value;

  slv2_system_t sys;
  (void) server;
  sys = SLV2(asys);

  big = INT_MAX/16;

  sys->s.ready_to_solve = FALSE;
  sys->nfunc = 0;
  sys->ngrad = 0;

  iteration_begins(sys);
  check_system(sys);
  if( sys->vlist == NULL ) {
     FPRINTF(stderr,"ERROR:  (slv2) slv2_presolve\n");
     FPRINTF(stderr,"        Variable list was never set.\n");
     return;
  }

  if( sys->rlist == NULL ) {
     FPRINTF(stderr,"ERROR:  (slv2) slv2_presolve\n");
     FPRINTF(stderr,"        Relation list was never set.\n");
     return;
  }

  /* Reset global iteration count and cpu-elapsed. */
  sys->s.iteration = 0L;
  sys->s.cpu_elapsed = D_ZERO;

  /* Do a variety of things according to options to improve ma28/opt.
   * This leaves rlen and vlen set to what opt needs to know about.
   * If there are no constraints, does nothing.
   */
  sys->rlen = slv_get_num_solvers_rels(SERVER);
  if (sys->rlen) {
    slv2_rearrange_lists(sys);
  }

  sys->s.block.current_reordered_block = -2;

  sys->md = sys->rlen;
  sys->ibnds = 1; /* bounded variables problem */


  /* count nonzeros */
  sys->njac = relman_jacobian_count(sys->rlist,sys->rlen,
                                    sys->vfilter,sys->rfilter, 
                                    &dummy);

  /* create avar array */
  free_vector(sys->avar); 
  sys->avar = alloc_ivector(sys->njac+1);
  free_vector(sys->jvec);
  sys->jvec = alloc_ivector(sys->njac+1);

  /* create acon array */
  free_vector(sys->acon);
  sys->acon = alloc_ivector(sys->njac+1);
  free_vector(sys->ivec);
  sys->ivec = alloc_ivector(sys->njac+1);

  
  /* create objective gradient space */
  free_vector(sys->g);
  free_vector(sys->gsparse);
  free_vector(sys->gxsparse);
  sys->g = alloc_vector(sys->vlen+1);
  if (sys->obj != NULL) {
    sys->gsparse = alloc_vector(rel_n_incidences(sys->obj));
    sys->gxsparse = alloc_ivector(rel_n_incidences(sys->obj));
  } else {
    sys->gsparse = NULL;
    sys->gxsparse = NULL;
  }

  /* create residual space */
  free_vector(sys->c);
  sys->c = alloc_vector(sys->rlen);

  free_vector(sys->weights);
  sys->weights = alloc_vector(sys->rlen);

  sys->nzd = MAX(1, sys->njac);


  CHECK_MEM(&(sys->inform),&big,&big,&(sys->vlen),&(sys->rlen),&(sys->nzd),
            &(sys->iwsize),&(sys->rwsize));
  free_vector(sys->iw); /* create opt integer workspace */
  sys->iw = alloc_ivector(sys->iwsize);
  if (!sys->iw) {
    FPRINTF(stderr,"Error mallocing opt integer workspace array!");
    return;
  }

  free_vector(sys->rw); /* create opt real workspace */
  sys->rw = alloc_vector(sys->rwsize);
  if ( !sys->rw) {
    FPRINTF(stderr,"Error mallocing opt real workspace array!");
    return;
  }

  free_vector(sys->bnds); /* create bounds array */
  sys->bnds = alloc_vector(2*(sys->vlen+1));
  if (!sys->bnds) {
    FPRINTF(stderr,"Error mallocing bnds array!");
    return;
  }

  free_vector(sys->xn); /* create solution array */
  sys->xn = alloc_vector(sys->vlen +1);

  free_vector(sys->nominals); /* create xn scaling array */
  sys->nominals = alloc_vector(sys->vlen);

  free_vector(sys->trust); /* create trust region step limit array */
  sys->trust = alloc_vector(sys->vlen +1);

  free_vector(sys->a); /* create jacobian element array */
  sys->a = alloc_vector(sys->njac+1);
  if (!sys->xn) {
    FPRINTF(stderr,"Error mallocing variable array!");
    return;
  }
  if (!sys->a) {
    FPRINTF(stderr,"Error mallocing jacobian array!");
    return;
  }

  if (scale_and_bounds(sys) > 0) {
    FPRINTF(MIF(sys),"Unable to presolve; bad initial point.\n");
    sys->s.ready_to_solve = FALSE;
    return;
  }

  sys->iluoption = ilu_name_to_int(sys);

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
  sys->s.block.iteration = 0L;
  sys->s.block.cpu_elapsed = D_ZERO;
  sys->s.calc_ok = TRUE;
  sys->s.ready_to_solve = TRUE;

  sys->inform = -1;

  iteration_ends(sys);
}

/***************************************************************************
 * Below here is plain vanilla ascendism
 */

static void
slv2_dump_internals(slv_system_t server, SlvClientToken sys,int level)
{
  /* what do we want to put here? */
}

static
int slv2_eligible_solver(slv_system_t server)
{
  return 1; /* fixme */
}

static
void slv2_get_parameters(slv_system_t server, SlvClientToken asys,
                         slv_parameters_t *parameters)
{
  slv2_system_t sys;
  (void) server;
  sys = SLV2(asys);
  if (check_system(sys)) return;
  mem_copy_cast(&(sys->p),parameters,sizeof(slv_parameters_t));
}

static void slv2_set_parameters(slv_system_t server, SlvClientToken asys,
                                slv_parameters_t *parameters)
{
  slv2_system_t sys;
  (void) server;
  sys = SLV2(asys);
  if (check_system(sys)) return;
  mem_copy_cast(parameters,&(sys->p),sizeof(slv_parameters_t));
}

static void slv2_get_status(slv_system_t server, SlvClientToken asys,
                            slv_status_t *status)
{
  slv2_system_t sys;
  (void) server;
  sys = SLV2(asys);
  if (check_system(sys)) return;
  mem_copy_cast(&(sys->s),status,sizeof(slv_status_t));
}

static
void real_iterate(slv_system_t server, SlvClientToken asys,int needpush)
{
  slv2_system_t sys;
  sys = SLV2(asys);
  check_system(sys);
  if( !sys->s.ready_to_solve ) {
    FPRINTF(stderr,"ERROR:  (slv2) slv2_iterate\n");
    FPRINTF(stderr,"	    Not ready to solve.\n");
    return;
  }

  if (needpush) {
    push_controls(sys->icontrol, sys->rcontrol);
  }
  invoke_opt(sys);
  if (needpush) {
    pop_controls(sys->icontrol, sys->rcontrol);
  }

}

static
void slv2_iterate(slv_system_t server, SlvClientToken asys)
{
  real_iterate(server, asys, 1);
}

static void slv2_solve(slv_system_t server, SlvClientToken asys)
{
  slv2_system_t sys;
  sys = SLV2(asys);
  if (server == NULL || sys==NULL) return;
  if (check_system(sys)) return;
  push_controls(sys->icontrol, sys->rcontrol);
  while( sys->s.ready_to_solve ) {
    real_iterate(server,asys,0);
  }
  pop_controls(sys->icontrol, sys->rcontrol);
}

int slv2_register(SlvFunctionsT *sft)
{
  if (sft==NULL)  {
    FPRINTF(stderr,"slv2_register called with NULL pointer\n");
    return 1;
  }

  sft->name = NAME;
  sft->ccreate = slv2_create;
  sft->cdestroy = slv2_destroy;
  sft->celigible = slv2_eligible_solver;
  sft->getdefparam = slv2_get_default_parameters;
  sft->get_parameters = slv2_get_parameters;
  sft->setparam = slv2_set_parameters;
  sft->getstatus = slv2_get_status;
  sft->solve = slv2_solve;
  sft->presolve = slv2_presolve;
  sft->iterate = slv2_iterate;
  sft->resolve = NULL;
  sft->getlinsol = NULL;
  sft->getlinsys = NULL;
  sft->get_sys_mtx = NULL;
  sft->dumpinternals = slv2_dump_internals;
  /* it should be safe, but unnecessary, to call this at every iteration
   * according to Dave.
   */
  INIT_OPT_COMMON();
  return 0;
}

#endif /* #else clause of DYNAMIC_OPTSQP */
#endif /* #else clause of !STATIC_OPTSQP && !DYNAMIC_OPTSQP */
