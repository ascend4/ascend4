/*
 *  Lsode.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.29 $
 *  Version control file: $RCSfile: Lsode.c,v $
 *  Date last modified: $Date: 2000/01/25 02:26:31 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the ASCEND Tcl/Tk interface
 *
 *  Copyright 1997, Carnegie Mellon University
 *
 *  The ASCEND Tcl/Tk interface is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The ASCEND Tcl/Tk interface is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

#ifndef NO_SIGNAL_TRAPS
#include <signal.h>
#include <setjmp.h>
#endif /* NO_SIGNAL_TRAPS */
#include "tcl.h"
#include "utilities/ascConfig.h"
#include "utilities/ascSignal.h"
#include "utilities/ascMalloc.h"
#include "utilities/ascPanic.h"
#include "general/tm_time.h"
#include "general/list.h"
#include "compiler/compiler.h"
#include "compiler/instance_enum.h"
#include "compiler/symtab.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/parentchild.h"
#include "compiler/module.h"
#include "compiler/library.h"
#include "compiler/child.h"
#include "compiler/type_desc.h"
#include "compiler/instance_name.h"
#include "compiler/atomvalue.h"
#include "solver/slv_types.h"
#include "solver/mtx.h"
#include "solver/var.h"
#include "solver/rel.h"
#include "solver/discrete.h"
#include "solver/conditional.h"
#include "solver/logrel.h"
#include "solver/bnd.h"
#include "solver/slv_common.h"
#include "solver/linsol.h"
#include "solver/linsolqr.h"
#include "solver/slv_client.h"
#include "compiler/types.h"
#include "compiler/functype.h"
#include "compiler/func.h"
#include "compiler/extfunc.h"
#include "compiler/extcall.h"
#include "compiler/relation_type.h"
#include "interface/old_utils.h"
#include "interface/Integrators.h"
#include "interface/Lsode.h"
#include "interface/Sensitivity.h"		/* see the packages dir */

#ifndef lint
static CONST char LsodeID[] = "$Id: Lsode.c,v 1.29 2000/01/25 02:26:31 ballan Exp $";
#endif


/*
 *  NOUNDERBARS --> FORTRAN compiler naming convention for subroutine
 *  is wierd. WIN32/CRAY is treated as special case
 */
#ifdef APOLLO
#define NOUNDERBARS TRUE
#endif
#ifdef _HPUX_SOURCE
#define NOUNDERBARS TRUE
#endif
/* AIX xlf will not suffix an underbar on a symbol
 * unless xlf is given the ``-qextname'' option
 */
#ifdef _AIX
#define NOUNDERBARS TRUE
#endif

#ifdef NOUNDERBARS
#define LSODE lsode
#define LSODE_JEX jex
#define LSODE_FEX fex
#define GETCOMMON get_lsode_common
#define XASCWV xascwv
#else
/* sun, __alpha, __sgi, ... */
#define LSODE lsode_
#define LSODE_JEX jex_
#define LSODE_FEX fex_
#define GETCOMMON get_lsode_common_
#define XASCWV xascwv_
#endif

#if defined(CRAY) || defined(__WIN32__)
#undef LSODE
#undef LSODE_JEX
#undef LSODE_FEX
#undef GETCOMMON
#undef XASCWV 
#define XASCWV XASCWV
#define LSODE LSODE
#define LSODE_JEX JEX
#define LSODE_FEX FEX
#define GETCOMMON GET_LSODE_COMMON
#endif

#define DOTIME FALSE

/* definitions of lsode supported children of atoms, etc */
/********************************************************************/
/* default input tolerances for lsode */
#define RTOLDEF 1e-6
#define ATOLDEF 1e-6
/* solver_var children expected for state variables */
static symchar *g_symbols[2];
#define STATERTOL g_symbols[0]
#define STATEATOL g_symbols[1]
static
void InitTolNames(void)
{
  STATERTOL = AddSymbol("ode_rtol");
  STATEATOL = AddSymbol("ode_atol");
}

/***** interface implementation notes *****/
/*
 * As fortran io is unreliably portable (vc5+digital fortran)
 * we have converted xerrwv to xascwv provided here.
 *
 * The lsode interface variable t is actually an array of
 * 2 doubles rather than just 1. The first is the the one
 * used by lsode. The second is used by LSODE_FEX to tell
 * what the last time it was called was. This is so the
 * C driver can tell if it needs to resolve d to compute
 * observation variables. If x[0]==x[1] we save ourselves
 * a solve.
 ! Note!!!!
 ! The above doesn't work since lsode doesn't use the same
 ! t internally that we hand it.
 */

enum Lsode_enum {
  lsode_none,				/* true on first call */
  lsode_function, lsode_derivative,	/* functions or gradients done */
  lsode_sparse, lsode_dense,		/* what type of backend should we */
  lsode_band, 				/* use for the integrator */
  lsode_ok, lsode_nok			/* bad return from func or grad */
};

static struct Lsode_Data {
  enum Lsode_enum lastcall;		/* type of last call; func or grad */
  enum Lsode_enum status;		/* solve status */
  int partitioned;			/* partioned func evals or not */
  struct rel_relation **rlist;		/* relations to differentiate */
  slv_system_t sys;			/* the main solve system */
} lsodesys = {lsode_none, lsode_ok, 1, NULL, NULL};


/*
 * Type of function used to evaluate derivative system.
 */
typedef void (*fex_t)( int * ,double * ,double *,double *);

/*
 * Type of function used to evaluate jacobian system.
 */
typedef void (*jex_t)(int *, double *, double *, int *, int *,
                      double *, int *);

/*
 *  void LSODE(&fex, &neq, y, &x, &xend, &itol, reltol, abtol, &itask,
 *               &istate, &iopt ,rwork, &lrw, iwork, &liw, &jex, &mf);
 *  This is the FORTRAN call to LSODE.
 */
extern void LSODE( fex_t
                  ,int *
                  ,double *
                  ,double *
                  ,double *
                  ,int *
                  ,double *
                  ,double *
                  ,int *
                  ,int *
                  ,int *
                  ,double *
                  ,int *
                  ,int *
                  ,int *
                  ,jex_t
                  ,int *
                  );

/********************************************************************/
/* allocates, fills, and returns the atol vector based on BLSODE */
/* State variables missing child ode_rtol will be defaulted to ATOLDEF */

static double *blsode_get_atol( struct Integ_system_t *blsys) {

  struct Instance *tol;
  double *atoli;
  int i,len;

  len = blsys->n_y;
  atoli = (double *)ascmalloc((blsys->n_y+1)*sizeof(double));
  if (atoli == NULL) {
    FPRINTF(stderr,"ERROR: (blsode_get_atol) Insufficient memory.\n");
    return atoli;
  }
  InitTolNames();
  for (i=0; i<len; i++) {
    tol = ChildByChar(var_instance(blsys->y[i]),STATEATOL);
    if (tol == NULL || !AtomAssigned(tol) ) {
      atoli[i] = ATOLDEF;
      FPRINTF(stderr,"WARNING: (blsode_get_atol)  Assuming atol = %3g\n",
        ATOLDEF);
      FPRINTF(stderr, "for ode_atol child undefined for state variable %ld.\n",
        blsys->y_id[i]);
    } else {
      atoli[i] = RealAtomValue(tol);
    }
  }
  atoli[len] = ATOLDEF;
  return atoli;
}

/********************************************************************/
/* Allocates, fills, and returns the rtol vector based on BLSODE */
/* State variables missing child ode_rtol will be defaulted to RTOLDEF */
static double *blsode_get_rtol( struct Integ_system_t *blsys) {

  struct Instance *tol;
  double *rtoli;
  int i,len;

  len = blsys->n_y;
  rtoli = (double *)ascmalloc((blsys->n_y+1)*sizeof(double));
  if (rtoli == NULL) {
    FPRINTF(stderr,"ERROR: (blsode_get_rtol) Insufficient memory.\n");
    return rtoli;
  }
  InitTolNames();
  for (i=0; i<len; i++) {
    tol = ChildByChar(var_instance(blsys->y[i]),STATERTOL);
    if (tol == NULL || !AtomAssigned(tol) ) {
      rtoli[i] = RTOLDEF;
      FPRINTF(stderr,"WARNING: (blsode_get_rtol)  Assuming rtol = %3g\n",
        ATOLDEF);
      FPRINTF(stderr, "for ode_rtol child undefined for state variable %ld.\n",
        blsys->y_id[i]);
    } else {
      rtoli[i] = RealAtomValue(tol);
    }
  }
  rtoli[len] = RTOLDEF;
  return rtoli;
}

/********************************************************************/

static void write_istate( int istate) {
  FPRINTF(stderr,"lsode integrator:");
  switch (istate) {
  case -1:
    FPRINTF(stderr,"Excess steps taken on this call (perhaps wrong MF).\n");
    break;
  case -2:
    FPRINTF(stderr,"Excess accuracy requested (tolerances too small).\n");
    break;
  case -3:
    FPRINTF(stderr,"Illegal input detected.\n");
    break;
  case -4:
    FPRINTF(stderr,"Repeated error test failures.\n");
    break;
  case -5:
    FPRINTF(stderr,"Repeated convergence failures.\n");
    break;
  case -6:
    FPRINTF(stderr,"Error weight became zero during problem.\n");
    break;
  case -7:
    FPRINTF(stderr,"User patience became zero during problem.\n");
    break;
  default:
    FPRINTF(stderr,"Unknown error code %d\n",istate);
    break;
  }
}
/********************************************************************/

static void freeMem(double *y, double *reltol, double *abtol, double *rwork,
                    int *iwork, double *obs, double *dydx)
{
  if (y != NULL) {
    ascfree((double *)y);
  }
  if (reltol != NULL) {
    ascfree((double *)reltol);
  }
  if (abtol != NULL) {
    ascfree((double *)abtol);
  }
  if (rwork != NULL) {
    ascfree((double *)rwork);
  }
  if (iwork != NULL) {
    ascfree((int *)iwork);
  }
  if (obs != NULL) {
    ascfree((double *)obs);
  }
  if (dydx != NULL) {
    ascfree((double *)dydx);
  }
}

/********************************************************************/
/*
 * The current way that we are getting the derivatives (if the problem
 * was solved partitioned) messes up the slv_system so that we *have*
 * to do a *presolve* rather than a simply a *resolve* before doing
 * function calls.  This code below attempts to handle these cases.
 */
static void LSODE_FEX( int *n_eq ,double *t ,double *y ,double *ydot)
{
  slv_status_t status;
/*  slv_parameters_t parameters; pity lsode doesn't allow error returns */
  int i;
  unsigned long ok;

#if DOTIME
  double time1,time2;
#endif

#if DOTIME
  FPRINTF(stderr,"\n** Calling for a function evaluation **\n");
  time1 = tm_cpu_time();
#endif

/*  t[1]=t[0]; can't do this. lsode calls us with a different t
 *  than the x we sent in.
 */
  Asc_IntegSetDX(t[0]);
  Asc_IntegSetDY(y);

#if DOTIME
  time2 = tm_cpu_time();
#endif

  switch(lsodesys.lastcall) {
  case lsode_none:		/* first call */
  case lsode_derivative:
    if (lsodesys.partitioned) {
      slv_presolve(lsodesys.sys);
    } else {
      slv_resolve(lsodesys.sys);
    }
    break;
  default:
  case lsode_function:
    slv_resolve(lsodesys.sys);
    break;
  }
  slv_solve(lsodesys.sys);
  slv_get_status(lsodesys.sys, &status);
  ok = Asc_IntegCheckStatus(status);

#if DOTIME
  time2 = tm_cpu_time() - time2;
#endif

  if (!ok) {
    FPRINTF(stderr,"Unable to compute the vector of derivatives with the\n");
    FPRINTF(stderr,"following values for the state variables:\n");
    for (i = 0; i< *n_eq; i++) {
      FPRINTF(stderr,"y[%4d] = %f\n",i, y[i]);
    }
    FPRINTF(stderr,"\n");
    lsodesys.status = lsode_nok;
  } else {
    lsodesys.status = lsode_ok;
  }
  Asc_IntegGetDDydx(ydot);

  lsodesys.lastcall = lsode_function;
#if DOTIME
  time1 = tm_cpu_time() - time1;
  FPRINTF(stderr,"** Function evalulation has been completed in %g**\n",time1);
  FPRINTF(stderr,"** True function call time =  %g**\n",time2);
#endif
}

/********************************************************************/

static void LSODE_JEX(int *neq ,double *t, double *y,
                      int *ml ,int *mu ,double *pd, int *nrpd)
{
  int nok = 0;
  int i,j;

  (void)t;        /* stop gcc whine about unused parameter */
  (void)y;        /* stop gcc whine about unused parameter */
  (void)ml;       /* stop gcc whine about unused parameter */
  (void)mu;       /* stop gcc whine about unused parameter */

#if DOTIME
  double time1;
#endif

#if DOTIME
  FPRINTF(stderr,"\n** Calling for a gradient evaluation **\n");
  time1 = tm_cpu_time();
#endif
  /*
   * Make the real call.
   */
  nok = Asc_BLsodeDerivatives(lsodesys.sys, g_intg_diff.dydx_dx,
                            g_intg_diff.input_indices, *neq,
                            g_intg_diff.output_indices, *nrpd);
  if (nok) {
    FPRINTF(stderr,"Error in computing the derivatives for the system\n");
    FPRINTF(stderr,"Failing...\n");
    lsodesys.status = lsode_nok;
    lsodesys.lastcall = lsode_derivative;
    return;
  } else {
    lsodesys.status = lsode_ok;
    lsodesys.lastcall = lsode_derivative;
  }
  /*
   * Map data from C based matrix to Fortan matrix.
   * We will send in a column major ordering vector for pd.
   */
  for (j=0;j<*neq;j++) {
    for (i=0;i<*nrpd;i++) {	/* column wise - hence rows change faster */
      *pd++ = g_intg_diff.dydx_dx[i][j];
    }
  }

#if DOTIME
  time1 = tm_cpu_time() - time1;
  FPRINTF(stderr,"********** Time to do gradient evaluation %g\n",time1);
#endif

  return;
}

/********************************************************************/


void Asc_BLsodeIntegrate(slv_system_t sys, unsigned long start_index ,
                      unsigned long finish_index, struct Integ_system_t *blsys)
{
  slv_status_t status;
  slv_parameters_t params;
  double x[2];
  double xend,xprev;
  unsigned long nsamples, neq;
  long nobs;
  int  itol, itask, mf, lrw, liw;
  unsigned long index;
  int istate, iopt;
  double * rwork;
  int * iwork;
  double *y, *abtol, *reltol, *obs, *dydx;
  int my_neq;
  FILE *y_out =NULL;
  FILE *obs_out =NULL;

  lsodesys.sys = sys;
  slv_get_status(lsodesys.sys, &status);

  if (status.struct_singular) {
    FPRINTF(stderr, "\n");
    FPRINTF(stderr, "Integration will not be performed.\n");
    FPRINTF(stderr, "The system is structurally singular.\n");
    lsodesys.status = lsode_nok;
    return;
  }

#if defined(STATIC_LSOD) || defined (DYNAMIC_LSOD)
/* here we assume integrators.c is in charge of dynamic loading */

  slv_get_parameters(lsodesys.sys,&params);
  lsodesys.partitioned = 1;

  nsamples = Asc_IntegGetNSamples();
  if (nsamples <2) {
    FPRINTF(stderr, "\n");
    FPRINTF(stderr, "Integration will not be performed.\n");
    FPRINTF(stderr, "The system has no end sample time defined.\n");
    lsodesys.status = lsode_nok;
    return;
  }
  neq = blsys->n_y;
  nobs = blsys->n_obs;

  x[0] = Asc_IntegGetDX();
  x[1] = x[0]-1; /* make sure we don't start with wierd x[1] */
  lrw = 22 + 9*neq + neq*neq;
  rwork = (double *)asccalloc(lrw+1, sizeof(double));
  liw = 20 + neq;
  iwork = (int *)asccalloc(liw+1, sizeof(int));
  y = Asc_IntegGetDY(NULL);
  reltol = blsode_get_rtol(blsys);
  abtol = blsode_get_atol(blsys);
  obs = Asc_IntegGetDObs(NULL);
  dydx = (double *)asccalloc(neq+1, sizeof(double));
  if (!y || !obs || !abtol || !reltol || !rwork || !iwork || !dydx) {
    freeMem(y,reltol,abtol,rwork,iwork,obs,dydx);
    FPRINTF(stderr,"Insufficient memory for blsode.\n");
    lsodesys.status = lsode_nok;
    return;
  }

  y_out = Asc_IntegOpenYFile();
  obs_out = Asc_IntegOpenObsFile();
  Asc_IntegReleaseYFile();
  Asc_IntegReleaseObsFile();
/*
 * Prepare args and call lsode.
 */
  itol = 4;
  itask = 1;
  istate = 1;
  iopt = 1;
  rwork[4] = Asc_IntegGetStepZero(blsys);
  rwork[5] = Asc_IntegGetStepMax(blsys);
  rwork[6] = Asc_IntegGetStepMin(blsys);
  iwork[5] = Asc_IntegGetMaxSteps(blsys);
  mf = 21;		/* gradients 22 -- implies finite diff */

  /* put the values from derivative system into the record */
  Asc_IntegSetXSamplei(start_index, x[0]);
  /* write headers to yout, obsout and initial points */
  Asc_IntegPrintYHeader(y_out,blsys);
  Asc_IntegPrintYLine(y_out,blsys);
  Asc_IntegPrintObsHeader(obs_out,blsys);
  Asc_IntegPrintObsLine(obs_out,blsys);

  my_neq = (int)neq;
/*
 * First time entering lsode, x is input. After that,
 * lsode uses x as output (y output is y(x)). To drive
 * the loop ahead in time, all we need to do is keep upping
 * xend.
 */
  for (index = start_index; index < finish_index; index++) {
    xend = Asc_IntegGetXSamplei(index+1);
    xprev = x[0];
    print_debug("BEFORE %lu BLSODE CALL\n", index);

#ifndef NO_SIGNAL_TRAPS
    if (setjmp(g_fpe_env)==0) {
#endif /* NO_SIGNAL_TRAPS */
      LSODE(&(LSODE_FEX), &my_neq, y, x, &xend,
            &itol, reltol, abtol, &itask, &istate,
            &iopt ,rwork, &lrw, iwork, &liw, &(LSODE_JEX), &mf);
#ifndef NO_SIGNAL_TRAPS
    } else {
      FPRINTF(stderr,
       "Integration terminated due to float error in LSODE call.\n");
      freeMem(y,reltol,abtol,rwork,iwork,obs,dydx);
      lsodesys.status = lsode_ok;		/* clean up before we go */
      lsodesys.lastcall = lsode_none;
      if (y_out!=NULL) {
        fclose(y_out);
      }
      if (obs_out!=NULL) {
        fclose(obs_out);
      }
      return;
    }
#endif /* NO_SIGNAL_TRAPS */

    print_debug("AFTER %lu LSODE CALL\n", index);
    /* this check is better done in fex,jex, but lsode takes no status */
    if (Solv_C_CheckHalt()) {
      if (istate >= 0) {
        istate=-7;
      }
    }
    if (istate < 0 ) {
      FPRINTF(stderr, "!! Asc_BLsodeIntegrate: ");
      FPRINTF(stderr, "index = %lu ", index);
      FPRINTF(stderr, "istate = %d ", istate);
      FPRINTF(stderr, "farthest point reached: x = %g\n",x[0]);
      write_istate(istate);
      freeMem(y,reltol,abtol,rwork,iwork,obs,dydx);
      if (y_out!=NULL) {
        fclose(y_out);
      }
      if (obs_out!=NULL) {
        fclose(obs_out);
      }
      return;
    }

    if (lsodesys.status==lsode_nok) {
      FPRINTF(stderr,
       "Integration terminated due to an error in derivative computations.\n"
      );
      freeMem(y,reltol,abtol,rwork,iwork,obs,dydx);
      lsodesys.status = lsode_ok;		/* clean up before we go */
      lsodesys.lastcall = lsode_none;
      if (y_out!=NULL) {
        fclose(y_out);
      }
      if (obs_out!=NULL) {
        fclose(obs_out);
      }
      return;
    }

    Asc_IntegSetXSamplei(index+1, x[0]);
    /* record when lsode actually came back */
    Asc_IntegSetDX(x[0]);
    Asc_IntegSetDY(y);
    /* put x,y in d in case lsode got x,y by interpolation, as it does  */
    Asc_IntegPrintYLine(y_out,blsys);
    if (nobs > 0) {
#ifndef NO_SIGNAL_TRAPS
      if (setjmp(g_fpe_env)==0) {
#endif /* NO_SIGNAL_TRAPS */
        /* solve for obs since d isn't necessarily already
           computed there though lsode's x and y may be.
           Note that since lsode usually steps beyond xend
           x1 usually wouldn't be x0 precisely if the x1/x0
           scheme worked, which it doesn't anyway. */
        LSODE_FEX(&my_neq, x, y, dydx);
        /* calculate observations, if any, at returned x and y. */
        obs = Asc_IntegGetDObs(obs);
        Asc_IntegPrintObsLine(obs_out,blsys);
#ifndef NO_SIGNAL_TRAPS
      } else {
        FPRINTF(stderr,
         "Integration terminated due to float error in LSODE FEX call.\n"
        );
        freeMem(y,reltol,abtol,rwork,iwork,obs,dydx);
        lsodesys.status = lsode_ok;               /* clean up before we go */
        lsodesys.lastcall = lsode_none;
        if (y_out!=NULL) {
          fclose(y_out);
        }
        if (obs_out!=NULL) {
          fclose(obs_out);
        }
        return;
      }
#endif /* NO_SIGNAL_TRAPS */
    }
    FPRINTF(stdout, "Integration completed from ");
    FPRINTF(stdout, "%3g to %3g\n",xprev,x[0]);
  }

  FPRINTF(stdout, "\n");
  FPRINTF(stdout, "Number of steps taken: %1d\n", iwork[10]);
  FPRINTF(stdout, "Number of function evaluations: %1d\n", iwork[11]);
  FPRINTF(stdout, "Number of Jacobian evaluations: %1d\n", iwork[12]);
  FPRINTF(stdout, "\n");

  freeMem(y,reltol,abtol,rwork,iwork,obs,dydx);
  /*
   * return the system to its original state.
   */
  lsodesys.status = lsode_ok;
  lsodesys.lastcall = lsode_none;
  if (y_out!=NULL) {
    fclose(y_out);
  }
  if (obs_out!=NULL) {
    fclose(obs_out);
  }
  FPRINTF(stdout, "blsode done.\n");

#else
 
  FPRINTF(stderr, "\n");
  FPRINTF(stderr, "Integration will not be performed.\n");
  FPRINTF(stderr, "LSODE binary not available.\n");
  lsodesys.status = lsode_nok;
  return;

#endif
}

/*
 * This function is xascwv, a compatible
 * replacement for xerrwv in lsode.f by Hindemarsh.
 */
/* Ported to C, approximately, by Benjamin Allan, 9/20/97.
 * This C source placed in the public domain, since lsode
 * itself is PD. This placedment in no way affects the GNU
 * licensing OF the rest OF the ascend system source code.
 * FORTRAN header:
 *-----------------------------------------------------------------------
 * subroutine xerrwv, constitute
 * a simplified version of the slatec error handling package.
 * written by a. c. hindmarsh at llnl.  version of march 30, 1987.
 * this version is in double precision.
 *
 * all arguments are input arguments.
 *
 * msg    = the message (hollerith literal or integer array).
 * nmes   = the length of msg (number of characters).
 * nerr   = the error number (not used).
 * level  = the error level..
 *          0 or 1 means recoverable (control returns to caller).
 *          2 means fatal (run is aborted--see note below).
 * ni     = number of integers (0, 1, or 2) to be printed with message.
 * i1,i2  = integers to be printed, depending on ni.
 * nr     = number of reals (0, 1, or 2) to be printed with message.
 * r1,r2  = reals to be printed, depending on nr.
 *
 * note..  this routine is machine-dependent and specialized for use
 * in limited context, in the following ways..
 * 1. the number of hollerith characters stored per word, denoted
 *    by ncpw below, is a data-loaded constant.
 * 2. the value of nmes is assumed to be at most 60.
 *    (multi-line messages are generated by repeated calls.)
 * 3. if level = 2, control passes to the statement   stop
 *    to abort the run.  this statement may be machine-dependent.
 * 4. r1 and r2 are assumed to be in double precision and are printed
 *    in d21.13 format.
 * 5. the common block /eh0001/ below is data-loaded (a machine-
 *    dependent feature) with default values.
 *    this block is needed for proper retention of parameters used by
 *    this routine which the user can reset by calling xsetf or xsetun.
 *    the variables in this block are as follows..
 *       mesflg = print control flag..
 *                1 means print all messages (the default).
 *                0 means no printing.
 *       lunit  = logical unit number for messages.
 *                the default is 6 (machine-dependent).
 *-----------------------------------------------------------------------
 * the following are instructions for installing this routine
 * in different machine environments.
 *
 * to change the default output unit, change the data statement
 * in the block data subprogram below.
 *
 * for a different number of characters per word, change the
 * data statement setting ncpw below, and format 10.  alternatives for
 * various computers are shown in comment cards.
 *
 * for a different run-abort command, change the statement following
 * statement 100 at the end.
 *-----------------------------------------------------------------------
 */
void XASCWV( char *msg, /* array of char/int not NULL ended, len *nmes */
             int *nmes, /* len of msg */
             int *nerr,
             int *level,
             int *ni,
             int *i1,
             int *i2,
             int *nr,
             double *r1,
             double *r2
           )
{
  (void)nerr;
  /*   ignore 
   *   integer i,lun, lunit, mesflg, ncpw, nch, nwds
   *   common /eh0001/ mesflg, lunit 
   *   data ncpw/4/
   *
   *   ncpw = sizeof(int);
   *   if (mesflg .eq. 0) go to 100 !ignore io suppresion 
   *   lun = lunit 
   *
   *   nch = min0(nmes,60)
   *   nwds = nch/ncpw
   *   if (nch .ne. nwds*ncpw) nwds = nwds + 1
   */
  /* write the message. lot easier in C, geez */
  FPRINTF(stderr,"%.*s\n",*nmes,msg);
  if (*ni == 1) {
    FPRINTF(stderr,"      in above message, i1 = %d\n",*i1);
  }
  if (*ni == 2) {
    FPRINTF(stderr,"      in above message, i1 = %d   i2 = %d\n",*i1,*i2);
  }
  if (*nr == 1) {
    FPRINTF(stderr,"      in above message, r1 = %21.13g\n", *r1);
  }
  if (*nr == 2) {
    FPRINTF(stderr,"      above, r1 = %21.13g   r2 = %21.13g\n", *r1,*r2);
  }
  if (*level != 2) {
    return;
  }
  /* NOT reached. lsode does NOT make level 2 calls in our version. */
  Asc_Panic(3,"xascwv", "LSODE really really confused");
  return; /* not reached */
}
