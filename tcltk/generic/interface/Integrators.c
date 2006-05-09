/*
 *  Integrators.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.32 $
 *  Version control file: $RCSfile: Integrators.c,v $
 *  Date last modified: $Date: 2003/08/23 18:43:06 $
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

/*
 * This module defines the general integration auxillaries for Ascend.
 */

#include <time.h>
#include <tcl.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/readln.h>
#include <general/list.h>
#include <general/dstring.h>
#include <compiler/compiler.h>
#include <compiler/instance_enum.h>
#include <compiler/fractions.h>
#include <compiler/dimen.h>
#include <compiler/units.h>
#include <compiler/module.h>
#include <compiler/library.h>
#include <compiler/types.h>
#include <compiler/child.h>
#include <compiler/type_desc.h>
#include <compiler/atomvalue.h>
#include <compiler/instance_name.h>
#include <compiler/instquery.h>
#include <compiler/parentchild.h>
#include <compiler/symtab.h>
#include <compiler/instance_io.h>
#include <solver/slv_types.h>
#include <solver/mtx.h>
#include <solver/var.h>
/*
 * note: the analytic jacobian routines (state matrix) depend on the
 * assumption that struct var_variable *<--> struct Instance *.
 */
#include <solver/rel.h>
#include <solver/discrete.h>
#include <solver/conditional.h>
#include <solver/logrel.h>
#include <solver/bnd.h>
#include <solver/slv_common.h>
#include <solver/linsol.h>
#include <solver/linsolqr.h>
#include <solver/slv_client.h>
#include "HelpProc.h"
#include "Integrators.h"
#include "BrowserQuery.h"
#include "Qlfdid.h"
#include "UnitsProc.h"
#include "BrowserProc.h"
#include "HelpProc.h"
#include "SolverGlobals.h"
#include "Lsode.h"

#ifndef lint
static CONST char IntegratorsID[] = "$Id: Integrators.c,v 1.32 2003/08/23 18:43:06 ballan Exp $";
#endif


#define SNULL (char *)NULL

static symchar *g_symbols[3];

/* The following names are of solver_var children or attributes
 * we support (at least temporarily) to determine who is a state and
 * who matching derivative.
 * These should be supported directly in a future solveratominst.
 */
#define STATEFLAG g_symbols[0]
/* Integer child. 0= algebraic, 1 = state, 2 = derivative, 3 = 2nd deriv etc */
#define STATEINDEX g_symbols[1]
/* Integer child. all variables with the same STATEINDEX value are taken to
 * be derivatives of the same state variable. We really need a compiler
 * that maintains this info by backpointers, but oh well until then.
 */
#define OBSINDEX g_symbols[2]
/* Integer child. All variables with OBSINDEX !=0 will be recorded in
 * the blsode output file. Tis someone else's job to grok this output.
 */

#define INTDEBUG 0

struct Integ_var_t {
  long index;
  long type;
  struct var_variable *i;
};
/* temp catcher of dynamic variable and observation variable data */

struct Integ_samples_t {
  long ns;
  double *x;
  dim_type *d;
};
/* an array of sample 'times' for reference during integration */

/* global to the world */
struct Integ_DiffData g_intg_diff = {0L,NULL,NULL,NULL,NULL,NULL,NULL};

/*Define items for integration interface. they are global to this file.
this globalness is bad. */
static slv_system_t g_intgsys_cur = NULL;
    /* derivative system children */
static struct var_variable *g_intginst_d_x = NULL;
    /* derivative system sizes */
static long g_intginst_d_n_eq, g_intginst_d_n_obs;
    /* derivative system array var lists. Not null terminated */
static struct var_variable **g_intginst_d_y_vars = NULL;
static struct var_variable **g_intginst_d_dydx_vars = NULL;
static struct var_variable **g_intginst_d_obs_vars = NULL;
/* sampling vector for feeding smarter integrators */
static struct Integ_samples_t g_intg_samples = {0L, NULL, NULL};

    /* Integ_system_build locally global var */
static struct Integ_system_t *l_isys = NULL;
static long l_nstates = 0;
static long l_nderivs = 0;

/* Macros for d. arrays.
   D_Y_VAR(i) returns the atom that corresponds to the FORTRAN y(i).
   D_DYDX_VAR(i) returns the atom that corresponds to the FORTRAN ydot(i).
   D_OBS_VAR(i) returns the atom that corresponds to the ASCEND d.obs[i].
*/
#define D_Y_VAR(i) g_intginst_d_y_vars[(i)-1]
#define D_DYDX_VAR(i) g_intginst_d_dydx_vars[(i)-1]
#define D_OBS_VAR(i) g_intginst_d_obs_vars[(i)-1]

/********************************************************************/

int Asc_IntegCheckStatus(slv_status_t status) {
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

/********************************************************************/
static FILE *l_y_file = NULL;
static FILE *l_obs_file = NULL;
static char *l_y_filename = NULL;
static char *l_obs_filename = NULL;
static int l_print_option = 1; /* default si */
static int l_print_fmt = 0; /* default variable */

FILE *Asc_IntegOpenYFile(void)
{
  if (l_y_filename==NULL) {
    return NULL;
  }
  l_y_file = fopen(l_y_filename,"a+");

  if (l_y_file==NULL) {
    FPRINTF(ASCERR,
      "WARNING: (integrate) Unable to open\n\t%s\nfor state output log.\n",
      l_y_filename);
  } else {
    time_t t;
    t = time((time_t *)NULL);
    FPRINTF(l_y_file,"DATASET %s", asctime(localtime(&t)));
    FFLUSH(l_y_file);
  }
  return l_y_file;
}

FILE *Asc_IntegOpenObsFile(void)
{
  if (l_obs_filename==NULL) {
    return NULL;
  }
  l_obs_file = fopen(l_obs_filename,"a+");
  if (l_obs_file==NULL) {
    FPRINTF(ASCERR,
      "WARNING: (integrate) Unable to open\n\t%s\nfor observation log.\n",
      l_obs_filename);
  } else {
    time_t t;
    t = time((time_t *)NULL);
    FPRINTF(l_obs_file,"DATASET %s", asctime(localtime(&t)));
    FFLUSH(l_obs_file);
  }
  return l_obs_file;
}

FILE *Asc_IntegGetYFile(void)
{
  return l_y_file;
}

FILE *Asc_IntegGetObsFile(void)
{
  return l_obs_file;
}

void Asc_IntegReleaseYFile(void)
{
  l_y_file = NULL;
}
void Asc_IntegReleaseObsFile(void)
{
  l_obs_file = NULL;
}
/********************************************************************/
/* string column layout: 26 char columns  if l_print_fmt else variable */
/* numeric format: leading space plus characters from Unit*Value */
/* index format: left padded long int */
/* headline format space plus - s */
#define BCOLSFMT (l_print_fmt ? "%-26s" : "\t%s")
#define BCOLNFMT (l_print_fmt ? " %-25s" : "\t%s")
#define BCOLIFMT (l_print_fmt ? " %25ld" : "\t%ld")
#define BCOLHFMT (l_print_fmt ? " -------------------------" : "\t---")

void Asc_IntegPrintYHeader(FILE *fp, struct Integ_system_t *blsys)
{
  long i,len, *yip;
  char *name;
  struct Instance *in;
  int si;
  if (fp==NULL) {
    return;
  }
  if (blsys==NULL) {
    FPRINTF(ASCERR,"WARNING: (Asc_IntegPrintYHeader: called w/o data\n");
    return;
  }
  if (blsys->n_y == 0) {
    return;
  }
  if (blsys->y == NULL) {
    FPRINTF(ASCERR,"ERROR: (Asc_IntegPrintYHeader: called w/NULL data\n");
    return;
  }
  len = blsys->n_y;
  yip = blsys->y_id;
  si = l_print_option;
  /* output indep var name */
  /* output dep var names */
  FPRINTF(fp,"States: (user index) (name) (units)\n");
  in = var_instance(blsys->x);
  FPRINTF(fp,"{indvar}");  /* indep id name */
  name = WriteInstanceNameString(in,g_solvinst_cur);
  FPRINTF(fp,"\t{%s}\t{%s}\n",name,Asc_UnitString(in,si));
  ascfree(name);
  for (i=0; i< len; i++) {
    in = var_instance(blsys->y[i]);
    FPRINTF(fp,"{%ld}",yip[i]);  /* user id # */
    name = WriteInstanceNameString(in,g_solvinst_cur);
    FPRINTF(fp,"\t{%s}\t{%s}\n",name,Asc_UnitString(in,si));
    ascfree(name);
  }
  FPRINTF(fp,BCOLSFMT,"indvar");
  for (i=0; i < len; i++) {
    FPRINTF(fp,BCOLIFMT,yip[i]);
  }
  FPRINTF(fp,"\n");
  for (i=0; i <= len; i++) {
    FPRINTF(fp,BCOLHFMT);
  }
  FPRINTF(fp,"\n");
}
/********************************************************************/
void Asc_IntegPrintObsHeader(FILE *fp, struct Integ_system_t *blsys)
{
  long i,len, *obsip;
  char *name;
  struct Instance *in;
  int si;
  if (fp==NULL) {
    return;
  }
  if (blsys==NULL) {
    FPRINTF(ASCERR,"WARNING: (Asc_IntegPrintObsHeader: called w/o data\n");
    return;
  }
  if (blsys->n_obs == 0) {
    return;
  }
  if (blsys->obs == NULL) {
    FPRINTF(ASCERR,"ERROR: (Asc_IntegPrintObsHeader: called w/NULL data\n");
    return;
  }
  len = blsys->n_obs;
  obsip = blsys->obs_id;
  si = l_print_option;
  FPRINTF(fp,"Observations: (user index) (name) (units)\n");
  /* output indep var name */
  /* output obs var names */
  in = var_instance(blsys->x);
  FPRINTF(fp,"{indvar}");  /* indep id name */
  name = WriteInstanceNameString(in,g_solvinst_cur);
  FPRINTF(fp,"\t{%s}\t{%s}\n",name,Asc_UnitString(in,si));
  ascfree(name);
  for (i=0; i< len; i++) {
    in = var_instance(blsys->obs[i]);
    FPRINTF(fp,"{%ld}",obsip[i]);  /* user id # */
    name = WriteInstanceNameString(in,g_solvinst_cur);
    FPRINTF(fp,"\t{%s}\t{%s}\n",name,Asc_UnitString(in,si));
    ascfree(name);
  }
  FPRINTF(fp,BCOLSFMT,"indvar");
  for (i=0; i < len; i++) {
    FPRINTF(fp,BCOLIFMT,obsip[i]);
  }
  FPRINTF(fp,"\n");
  for (i=0; i <= len; i++) {
    FPRINTF(fp,BCOLHFMT);
  }
  FPRINTF(fp,"\n");
}

/********************************************************************/
void Asc_IntegPrintYLine(FILE *fp, struct Integ_system_t *blsys)
{
  long i,len;
  struct var_variable **vp;
  int si;
  if (fp==NULL) {
    return;
  }
  if (blsys==NULL) {
    FPRINTF(ASCERR,"WARNING: (Asc_IntegPrintYLine: called w/o data\n");
    return;
  }
  if (blsys->n_y == 0) {
    return;
  }
  if (blsys->y == NULL) {
    FPRINTF(ASCERR,"ERROR: (Asc_IntegPrintYHeader: called w/NULL data\n");
    return;
  }
  vp = blsys->y;
  len = blsys->n_y;
  si = l_print_option;
  FPRINTF(fp,BCOLNFMT,Asc_UnitlessValue(var_instance(blsys->x),si));
  for (i=0; i < len; i++) {
    FPRINTF(fp,BCOLNFMT, Asc_UnitlessValue(var_instance(vp[i]),si));
  }
  FPRINTF(fp,"\n");
}

void Asc_IntegPrintObsLine(FILE *fp, struct Integ_system_t *blsys)
{
  long i,len;
  struct var_variable **vp;
  int si;
  if (fp==NULL) {
    return;
  }
  if (blsys==NULL) {
    FPRINTF(ASCERR,"WARNING: (Asc_IntegPrintObsLine: called w/o data\n");
    return;
  }
  if (blsys->n_obs == 0) {
    return;
  }
  if (blsys->obs == NULL) {
    FPRINTF(ASCERR,"ERROR: (Asc_IntegPrintObsHeader: called w/NULL data\n");
    return;
  }
  vp = blsys->obs;
  len = blsys->n_obs;
  si = l_print_option;
  FPRINTF(fp,BCOLNFMT,Asc_UnitlessValue(var_instance(blsys->x),si));
  for (i=0; i < len; i++) {
    FPRINTF(fp,BCOLNFMT, Asc_UnitlessValue(var_instance(vp[i]),si));
  }
  FPRINTF(fp,"\n");
}

int Asc_IntegSetYFileCmd(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  size_t len;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR, "integrate_set_y_file: called without filename.\n");
    Tcl_SetResult(interp,
                  "integrate_set_y_file <filename,""> called without arg.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (l_y_filename != NULL) {
    ascfree(l_y_filename);
  }
  len = strlen(argv[1]);
  if (len >0 ) {
    l_y_filename = Asc_MakeInitString((int)len);
    sprintf(l_y_filename,"%s",argv[1]);
  } else {
    l_y_filename = NULL;
  }
  return TCL_OK;
}

int Asc_IntegSetObsFileCmd(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  size_t len;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR, "integrate_set_obs_file: called without filename.\n");
    Tcl_SetResult(interp,
                  "integrate_set_obs_file <filename,""> called without arg.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (l_obs_filename != NULL) {
    ascfree(l_obs_filename);
  }
  len = strlen(argv[1]);
  if (len >0 ) {
    l_obs_filename = Asc_MakeInitString((int)len);
    sprintf(l_obs_filename,"%s",argv[1]);
  } else {
    l_obs_filename = NULL;
  }
  return TCL_OK;
}

int Asc_IntegSetFileUnitsCmd(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[])
{
  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR, "integrate_logunits: called without printoption.\n");
    Tcl_SetResult(interp,"integrate_logunits <display,si> called without arg.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  switch (argv[1][0]) {
  case 's':
    l_print_option = 1;
    break;
  case 'd':
    l_print_option = 0;
    break;
  default:
    FPRINTF(ASCERR,"integrate_logunits: called with bogus argument.\n");
    FPRINTF(ASCERR,"logunits remain set to %s.\n",
      (l_print_option ? "si":"display"));
    break;
  }
  return TCL_OK;
}

int Asc_IntegSetFileFormatCmd(ClientData cdata, Tcl_Interp *interp,
                             int argc, CONST84 char *argv[])
{
  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR, "integrate_logformat called without printoption.\n");
    Tcl_SetResult(interp,
                  "integrate_logformat <fixed,variable> called without arg.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  switch (argv[1][0]) {
  case 'f':
    l_print_fmt = 1;
    break;
  case 'v':
    l_print_fmt = 0;
    break;
  default:
    FPRINTF(ASCERR,"integrate_logformat: called with bogus argument.\n");
    FPRINTF(ASCERR,"logformat remains set to %s.\n",
      (l_print_fmt ? "fixed":"variable"));
    break;
  }
  return TCL_OK;
}


/********************************************************************/
/* stuff for handling a user defined list of step outside the ascend
   model */

int Asc_IntegSetXSamples(long n, double *values, dim_type *d)
{
  /* trash the old stuff, regardless.*/
  g_intg_samples.ns = 0L;
  if (g_intg_samples.x != NULL) {
    ascfree(g_intg_samples.x);
    g_intg_samples.x = NULL;
  }
  if (g_intg_samples.d != NULL) {
    ascfree(g_intg_samples.d);
    g_intg_samples.d = NULL;
  }
  /* if was a reset, return now */
  if (n <1 || values==NULL) {
    return 0;
  }
  /* store d given or copy and store WildDimension */
  if (d != NULL) {
    g_intg_samples.d = d;
#if INTDEBUG
    FPRINTF(ASCERR,"sample received dimen\n");
    PrintDimen(ASCERR,g_intg_samples.d);
#endif
  } else {
    g_intg_samples.d = (dim_type *)ascmalloc(sizeof(dim_type));
    if (g_intg_samples.d == NULL) {
      FPRINTF(ASCERR,"ERROR: (Asc_IntegSetXSamples) Insufficient memory.\n");
      return 1;
    }
    CopyDimensions(WildDimension(),g_intg_samples.d);
#if INTDEBUG
    FPRINTF(ASCERR,"copy of wild dimen looks like\n");
    PrintDimen(ASCERR,g_intg_samples.d);
#endif
  }
  g_intg_samples.ns = n;
  g_intg_samples.x = values;
  return 0;
}

int Asc_IntegGetMaxSteps(struct Integ_system_t *sys)
{
  return sys->maxsteps;
}

double Asc_IntegGetStepMax(struct Integ_system_t *sys)
{
  return sys->stepmax;
}

double Asc_IntegGetStepMin(struct Integ_system_t *sys)
{
  return sys->stepmin;
}

double Asc_IntegGetStepZero(struct Integ_system_t *sys)
{
  return sys->stepzero;
}

long Asc_IntegGetNSamples()
{
  return g_intg_samples.ns;
}

double Asc_IntegGetXSamplei(long i)
{
  if (i > -1 && i < g_intg_samples.ns) {
    return g_intg_samples.x[i];
  } else {
    FPRINTF(ASCERR,
            "WARNING: (Asc_IntegGetXSamplei) Undefined xsample %ld."
            " Returning 0.0.\n",
            i);
    return (double)0.0;
  }
}

void Asc_IntegSetXSamplei(long i,double xi)
{
  if (i > -1 && i < g_intg_samples.ns) {
    g_intg_samples.x[i] = xi;
  } else {
    FPRINTF(ASCERR,
      "WARNING: (Asc_IntegSetXSamplei) Undefined xsample %ld. Ignored.\n",
      i);
  }
}

dim_type *Asc_IntegGetXSamplesDim(void)
{
  return g_intg_samples.d;
}
/********************************************************************/
/* callbacks for manipulating a user defined list of steps */
int Asc_IntegGetXSamplesCmd(ClientData cdata, Tcl_Interp *interp,
                          int argc, CONST84 char *argv[])
{
  static char sval[40]; /* buffer long enough to hold a double printed */
  struct Units *du = NULL;
  dim_type *dp;
  long i,len;
  double *uvalues = NULL;
  char *ustring;
  double *uv;
  int trydu=0, prec, stat=0;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if (( argc < 1 ) || ( argc > 2 )) {
    Tcl_SetResult(interp,
                  "integrate_get_samples: expected 0 or 1 args [display]",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (argc==2) {
    trydu = 1;
    if( argv[1][0] != 'd') {
      Tcl_SetResult(interp, "integrate_get_samples: expected display but got ",
                    TCL_STATIC);
      Tcl_AppendResult(interp,argv[1],".",SNULL);
      return TCL_ERROR;
    }
  }
  len = g_intg_samples.ns;
  dp = g_intg_samples.d;
  if (len <1) {
    Tcl_SetResult(interp, "{} {}", TCL_STATIC);
    return TCL_OK;
  }

  if (trydu) {
    uvalues = (double *)ascmalloc(sizeof(double)*len);
    if (uvalues == NULL) {
      Tcl_SetResult(interp, "integrate_get_samples: Insufficient memory.",
                    TCL_STATIC);
      return TCL_ERROR;
    }
    ustring = Asc_UnitDimString(dp,0); /* get display unit string */
    du = (struct Units *)LookupUnits(ustring);
    if (du == NULL) {
      /*  a very bizarre thing to happen,
       *  since Asc_UnitDimString just made it
       */
      stat = 1;
    } else {
      stat = 0;
      uv = uvalues;
      for (i=0; i < len; i++) {
        stat = Asc_UnitConvert(du,g_intg_samples.x[i],uv,1);
        if (stat) {
          break;
        }
        uv++;
      }
    }
    if (stat) {
      ascfree(uvalues);
    }
  }
  /* if display not wanted or display -> conversion error */
  if (!trydu || stat) {
    uvalues = g_intg_samples.x;
    ustring = Asc_UnitDimString(dp,1); /* get si unit string */
  }

  Tcl_AppendResult(interp,"{",ustring,"} {",SNULL);
  prec = Asc_UnitGetCPrec();
  len--; /* last one is a special case */
  for (i=0; i<len; i++) {
    /* print number and a blank */
    sprintf(sval,"%.*g ",prec,uvalues[i]);
    Tcl_AppendResult(interp,sval,SNULL);
  }
  sprintf(sval,"%.*g",prec,uvalues[len]);
  Tcl_AppendResult(interp,sval,"}",SNULL);
  if (trydu && !stat) {
    ascfree(uvalues);
  }
  return TCL_OK;
}

int Asc_IntegSetXSamplesCmd(ClientData cdata, Tcl_Interp *interp,
                          int argc, CONST84 char *argv[])
{
  struct Units *du = NULL;
  dim_type *dp;
  dim_type *mydp;
  long i,len;
  double *uvalues = NULL;
  double *uv;
  int stat;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if (argc == 1) {
    Asc_IntegSetXSamples(0L,NULL,NULL);
    return TCL_OK;
  }

  if (argc <4) {
    Tcl_SetResult(interp,
                  "Syntax: integrate_set_samples"
                  " <units> <value [value...] value>",
                  TCL_STATIC);
    FPRINTF(ASCERR,"ERROR: integrate_set_samples needs at least 3 args.");
    return TCL_ERROR;
  }

  du = (struct Units *)LookupUnits(argv[1]);
  if (du == NULL) {
    Tcl_SetResult(interp, "integrate_set_samples: first arg not valid units.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  dp = (dim_type *)UnitsDimensions(du);
#if INTDEBUG
  FPRINTF(ASCERR,"user dimen looks like\n");
  PrintDimen(ASCERR,dp);
#endif
  mydp = (dim_type *)ascmalloc(sizeof(dim_type)); /* we are giving away */
  if (mydp == NULL) {
    Tcl_SetResult(interp, "integrate_set_samples: Insufficient memory",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  CopyDimensions(dp,mydp);
#if INTDEBUG
  FPRINTF(ASCERR,"copy of user dimen looks like\n");
  PrintDimen(ASCERR,mydp);
#endif

  len = argc -2;
  uvalues = (double *)ascmalloc(sizeof(double) * len); /* we are giving away */
  if (uvalues==NULL) {
    Tcl_SetResult(interp, "integrate_set_samples: Insufficient memory",
                  TCL_STATIC);
    ascfree(mydp);
    return TCL_ERROR;
  }
  stat = 0;
  uv = uvalues;
  for (i=0; i<len; i++) {
    if(Tcl_GetDouble(interp,argv[i+2],uv)!=TCL_OK) {
      stat = 1;
      break;
    }
    stat = Asc_UnitConvert(du,*uv,uv,0);
    if (stat) {
      break;
    }
    uv++;
  }
  Tcl_ResetResult(interp);
  if (stat) {
    Tcl_SetResult(interp, "integrate_set_samples: Invalid value given. (",
                  TCL_STATIC);
    Tcl_AppendResult(interp,argv[i+2],")",SNULL);
    ascfree(uvalues);
    ascfree(mydp);
    return TCL_ERROR;
  }
  if (Asc_IntegSetXSamples(len,uvalues,mydp)) {
    Tcl_SetResult(interp, "integrate_set_samples: Insufficient memory.",
                  TCL_STATIC);
    ascfree(uvalues);
    ascfree(mydp);
    return TCL_ERROR;
  }
  return TCL_OK;
}

/********************************************************************/

int Asc_IntegInstIntegrable(struct Instance *inst,
                          enum Integrator_type integrator)
{
  switch (integrator) {
  case BLSODE :
    if (inst == NULL) {
      return 0;
    }
    return 1;
  case UNKNOWN:
    FPRINTF(stderr, "UNKNOWN integrator is not supported.\n");
    return 0;
  default :
    FPRINTF(stderr, "ERRONEOUS integrator is not supported.\n");
    return 0;
  }
}

int Asc_IntegInstIntegrableCmd(ClientData cdata,Tcl_Interp *interp,
                             int argc, CONST84 char *argv[])
{
  struct Instance *i=NULL;
  enum Integrator_type integrator=UNKNOWN;
  int result=0;         /* 0 = FALSE; 1 = TRUE */

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 3 ) {
    Tcl_SetResult(interp, "integrate_able <solver,current,search> <lsode>",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  if (strncmp(argv[1],"solver",3)==0) {
    i = g_solvinst_cur;
  } else {
    if (strncmp(argv[1],"search",3)==0) {
      i = g_search_inst;
    } else {
      if (strncmp(argv[1],"current",3)==0) {
        i = g_curinst;
      } else {
        Tcl_SetResult(interp,
                      "integrate_able: arg 1 is current, search, or solver",
                      TCL_STATIC);
        return TCL_ERROR;
      }
    }
  }

  if (!i) {
    Tcl_SetResult(interp, "0", TCL_STATIC);
    FPRINTF(ASCERR,"NULL instance sent to integrate_able.\n");
    return TCL_OK;
  }

  if (strncmp(argv[2],"blsode",3)==0) {
    integrator = BLSODE;
  }
  /* someday will have an ivp homegrown. in anycase, ivp dis/en-ables btn */
  if (strncmp(argv[2],"ivp",3)==0) {
    integrator = UNKNOWN;
  }

  result = Asc_IntegInstIntegrable(i, integrator);
  if (result) {
    Tcl_SetResult(interp, "1", TCL_STATIC);
  } else {
    Tcl_SetResult(interp, "0", TCL_STATIC);
  }
  return TCL_OK;
}

/*** derivative parts **** x *****************************************/

double Asc_IntegGetDX() {
  return var_value(g_intginst_d_x);
}

void Asc_IntegSetDX( double value) {
  var_set_value(g_intginst_d_x, value);
  print_debug("set_d_x = %g\n", value);
}

/*** derivative parts **** y *****************************************/

double *Asc_IntegGetDY(double *y) {
  long i;

  if (y==NULL) {
    y = (double *)asccalloc((g_intginst_d_n_eq+1), sizeof(double));
    /* C y[0]  <==> ascend d.y[1]  <==>  f77 y(1) */
  }

  for (i=1; i<=g_intginst_d_n_eq; i++) {
    y[i-1] = var_value(D_Y_VAR(i));
    print_debug("*get_d_y[%ld] = ", i);
    print_debug("%g\n", y[i-1]);
  }
  return y;
}

void Asc_IntegSetDY(double *y) {
  long i;

  /* C y[0]  <==> ascend d.y[1]  <==>  f77 y(1) */

  for (i=1; i<=g_intginst_d_n_eq; i++) {
    var_set_value(D_Y_VAR(i),y[i-1]);
    print_debug("*set_d_y[%ld] = ", i);
    print_debug("%g\n", y[i-1]);
  }
}

/*** derivative parts **** dydx *****************************************/

double *Asc_IntegGetDDydx(double *dydx) {
  long i;

  if (dydx==NULL) {
    dydx = (double *)asccalloc((g_intginst_d_n_eq+1), sizeof(double));
    /* C dydx[0]  <==> ascend d.dydx[1]  <==>  f77 ydot(1) */
  }

  for (i=1; i<=g_intginst_d_n_eq; i++) {
    dydx[i-1] = var_value(D_DYDX_VAR(i));
    print_debug("*get_d_dydx[%ld] = ", i);
    print_debug("%g\n", dydx[i-1]);
  }
  return dydx;
}

void Asc_IntegSetDDydx(double *dydx) {
  long i;

  /* C dydx[0]  <==> ascend d.dydx[1]  <==>  f77 ydot(1) */

  for (i=1; i<=g_intginst_d_n_eq; i++) {
    var_set_value(D_DYDX_VAR(i),dydx[i-1]);
    print_debug("*set_d_dydx[%ld] = ", i);
    print_debug("%g\n", dydx[i-1]);
  }
}

/**** derivative parts * d.obs ******************************************/

/*
   This function takes the inst in the solver and returns the vector of
   observation variables that are located in the submodel d.obs array.
*/
double *Asc_IntegGetDObs(double *obsi) {
  long i;

  if (obsi==NULL) {
    obsi = (double *)asccalloc((g_intginst_d_n_obs+1),sizeof(double));
  }

  /* C obsi[0]  <==> ascend d.obs[1] */

  for (i=1; i<=g_intginst_d_n_obs; i++) {
    obsi[i-1] = var_value(D_OBS_VAR(i));
    print_debug("*get_d_obs[%ld] = ", i);
    print_debug("%g\n", obsi[i-1]);
  }
  return obsi;
}

/* All the KEEPIP stuff used to be here. its gone to cvs land */

/*
 *  takes the type of integrator and start and finish index and calls the
 *  appropriate integrator
 */
static void Integ_Solve( enum Integrator_type integrator,
                         int start_index, int finish_index,
                         struct Integ_system_t *blsys) {
  switch (integrator) {
  case BLSODE:
    Asc_BLsodeIntegrate(g_intgsys_cur,start_index, finish_index, blsys);
    return;
  default:
    FPRINTF(stderr, "The requested integrator is ");
    FPRINTF(stderr, "not currently available\n.");
    return;
  }
}

static double **MakeDenseMatrix(int nrows, int ncols)
{
  int c;
  double **result;
  assert(nrows>0);
  assert(ncols>0);
  result = (double **)ascmalloc(nrows*sizeof(double *));
  for (c=0;c<nrows;c++) {
    result[c] = (double *)asccalloc(ncols,sizeof(double));
  }
  return result;
}

static void DestroyDenseMatrix(double **matrix,int nrows)
{
  int c;
  if (matrix) {
    for (c=0;c<nrows;c++) {
      if (matrix[c]) {
        ascfree((char *)matrix[c]);
      }
    }
    ascfree((char *)matrix);
  }
}

/*
 * needs work. Assumes struct Instance* and struct var_variable*
 * are synonymous, which demonstrates the need for a method to take
 * an instance and ask the solvers for its global or local index
 * if var and inst are decoupled.
 */
static
int Integ_SetUpDiffs_BLsode(struct Integ_system_t *blsys) {
  long n_eqns;
  unsigned long nch,i;

  struct var_variable **vp;
  int *ip;

  g_intg_diff.n_eqns = n_eqns = blsys->n_y;
  g_intg_diff.input_indices = (int *)asccalloc(n_eqns, sizeof(int));
  g_intg_diff.output_indices = (int *)asccalloc(n_eqns, sizeof(int));
  g_intg_diff.y_vars = NULL;
  g_intg_diff.ydot_vars = NULL;
  g_intg_diff.dydx_dx = MakeDenseMatrix(n_eqns,n_eqns);


  /*
   * Let us now process what we consider *inputs* to the problem as
   * far as ASCEND is concerned; i.e. the state vars or the y_vars's
   * if you prefer.
   */
  nch = n_eqns;
  vp = g_intg_diff.y_vars =
    (struct var_variable **)ascmalloc((nch+1)*sizeof(struct var_variable *));
  ip = g_intg_diff.input_indices;
  for (i=0;i<nch;i++) {
    *vp = (struct var_variable *)blsys->y[i];
    *ip = var_sindex(*vp);
    vp++;
    ip++;
  }
  *vp = NULL;	/* terminate */

  /*
   * Let us now go for the outputs, ie the derivative terms.
   */
  vp = g_intg_diff.ydot_vars =
    (struct var_variable **)ascmalloc((nch+1)*sizeof(struct var_variable *));
  ip = g_intg_diff.output_indices;
  for (i=0;i<nch;i++) {
    *vp = (struct var_variable *)blsys->ydot[i];
    *ip = var_sindex(*vp);
    vp++;		/* dont assume that a var is synonymous with */
    ip++;		/* an Instance; that might/will change soon */
  }
  *vp = NULL;		/* terminate */

  return 0;
}

/* Build an analytic jacobian for solving the state system */
/*
 * This necessarily ugly piece of code attempts to create a unique
 * list of relations that explicitly contain the variables in the
 * given input list. The utility of this information is that we know
 * exactly which relations must be differentiated, to fill in the
 * df/dy matrix. If the problem has very few derivative terms, this will
 * be of great savings. If the problem arose from the discretization of
 * a pde, then this will be not so useful. The decision wether to use
 * this function or to simply differentiate the entire relations list
 * must be done before calling this function. Final Note: the callee
 * owns the array, but not the array elements.
 */

#define AVG_NUM_INCIDENT 4

/* Returns STATEFLAG child value if we have a correct dynamic variable.
   If we do not, returns 0. Doesn't check solver_var status.
   if index pointer is given, it is stuffed too.
   if return is 0, ignore index.
   to be correct either type and index are assigned integers or
   type is -1. */
static long DynamicVarInfo(struct var_variable *v,long *index)
{
  struct Instance *c, *d, *i;
  i = var_instance(v);
  assert(i!=NULL);
  c = ChildByChar(i,STATEFLAG);
  d = ChildByChar(i,STATEINDEX);
  /* lazy evaluation is important in the following if */
  if( c == NULL ||
      d == NULL ||
      InstanceKind(c) != INTEGER_INST ||
      InstanceKind(d) != INTEGER_INST ||
      !AtomAssigned(c) ||
      (!AtomAssigned(d) && GetIntegerAtomValue(c) != -1L)
      ) {
    return 0L;
  }
  if (index != NULL) {
    *index = GetIntegerAtomValue(d);
  }
  return GetIntegerAtomValue(c);
}

/* returns the pointer if we have a correct observation variable.
   If long is passed in, long will have the index value.
   Vars with UNDEFINED observation flags are 'incorrect.'
 */
static struct var_variable *ObservationVar(struct var_variable *v, long *index)
{
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

/* take the variable instance and set its obsid to index.
  id must be already assigned at least once. */
static void Integ_SetObsId(struct var_variable *v, long index)
{
  struct Instance *c, *i;
  i = var_instance(v);
  assert(i!=NULL);
  c = ChildByChar(i,OBSINDEX);
  if( c == NULL || InstanceKind(c) != INTEGER_INST || !AtomAssigned(c)) {
    return;
  }
  SetIntegerAtomValue(c,index,0);
}

/*
 * Sorts the interesting vars into our wonderful little lists.
 * Dislikes null input intensely.
 */
static void Integ_classify_vars(struct var_variable *var)
{
  struct Integ_var_t *info;
  long type,index;
  var_filter_t vfilt;

  assert(var != NULL && var_instance(var)!=NULL );
  vfilt.matchbits = VAR_SVAR;
  vfilt.matchvalue = VAR_SVAR;
  if( var_apply_filter(var,&vfilt) ) {
    type = DynamicVarInfo(var,&index);
    if ( type != 0) {
#if INTEG_DEBUG
      var_write_name(g_solvsys_cur,var,ASCERR);
      FPRINTF(ASCERR," type = %ld\n",type);
#endif
      /* ignore algebraics type == 0 */
      info = (struct Integ_var_t *)ascmalloc(sizeof(struct Integ_var_t));
      if (info == NULL) {
        FPRINTF(ASCERR,
          "ERROR: (Integ_system_build) Insufficient memory.\n");
        return;
      }
      info->type = type;
      info->index = index;
      info->i = var;
      if (type > 0L) {
        /* state or derivative */
        gl_append_ptr(l_isys->dynvars,(POINTER)info);
        if (type == 1) {
          l_nstates++;
        }
        if (type == 2) {
          l_nderivs++;
        }
      } else {
        if (type == -1L) {
          /* independent */
          gl_append_ptr(l_isys->indepvars,(POINTER)info);
        } else {
          /* probably should whine about unknown type here */
          ascfree(info);
        }
      }
      info = NULL;
    }
    if ( ObservationVar(var,&index) != NULL && index > 0L) {
      info = (struct Integ_var_t *)ascmalloc(sizeof(struct Integ_var_t));
      if (info == NULL) {
        FPRINTF(ASCERR,
          "ERROR: (Integ_system_build) Insufficient memory.\n");
        return;
      }
      info->type = 0L;
      info->index = index;
      info->i = var;
      gl_append_ptr(l_isys->obslist,(POINTER)info);
      info = NULL;
    }
  }
}


/* compares observation structs. NULLs should end up at far end. */
static int Integ_CmpObs(struct Integ_var_t *v1, struct Integ_var_t *v2)
{
  if (v1 == NULL) {
    return 1;
  }
  if (v2 == NULL) {
    return -1;
  }
  if (v1->index > v2->index) {
    return 1;
  }
  if (v1->index == v2->index) {
    return 0;
  }
  return -1;
}

/*
  Compares dynamic vars structs. NULLs should end up at far end.
  List should be sorted primarily by index and then by type, in order
  of increasing value of both.
*/
static int Integ_CmpDynVars(struct Integ_var_t *v1, struct Integ_var_t *v2)
{
  if (v1 == NULL) {
    return 1;
  }
  if (v2 == NULL) {
    return -1;
  }
  if (v1->index > v2->index) {
    return 1;
  }
  if (v1->index != v2->index) {
    return -1;
  }
  if (v1->type > v2->type) {
    return 1;
  }
  return -1;
}

/* trash nonnull contents of a system_t . does ree the pointer sent.*/
static void Integ_system_destroy(struct Integ_system_t *sys)
{
  if (sys==NULL) {
    return;
  }
  if (sys->states != NULL) {
    gl_destroy(sys->states);
  }
  if (sys->derivs != NULL) {
    gl_destroy(sys->derivs);
  }
  if (sys->dynvars != NULL) {
    gl_free_and_destroy(sys->dynvars);    /* we own the objects in dynvars */
  }
  if (sys->obslist != NULL) {
    gl_free_and_destroy(sys->obslist);    /* and obslist */
  }
  if (sys->indepvars != NULL) {
    gl_free_and_destroy(sys->indepvars);  /* and indepvars */
  }
  if (sys->y != NULL && !sys->ycount) {
    ascfree(sys->y);
  }
  if (sys->ydot != NULL && !sys->ydotcount) {
    ascfree(sys->ydot);
  }
  if (sys->y_id != NULL) {
    ascfree(sys->y_id);
  }
  if (sys->obs != NULL && !sys->obscount) {
    ascfree(sys->obs);
  }
  if (sys->obs_id != NULL) {
    ascfree(sys->obs_id);
  }
  ascfree(sys);
}

static
void IntegInitSymbols(void)
{
  STATEFLAG = AddSymbol("ode_type");
  STATEINDEX = AddSymbol("ode_id");
  OBSINDEX = AddSymbol("obs_id");
}

/*
 *  Returns a pointer to an ok struct Integ_system_t. Returns NULL
 *  if one cannot be built from the instance given.
 *  Diagnostics will be printed.
 *  The pointer returned will also be stored in l_isys if someone
 *  in Integrators.c needs it. Anyone who takes a copy of l_isys should
 *  then set l_isys to NULL and keep track of the sys from there.
 *  On exit the gl_list_t * in the returned pointer will all be NULL.
 */
static struct Integ_system_t *Integ_system_build(CONST slv_system_t slvsys)
{
  struct Integ_system_t *sys;
  struct Integ_var_t *v1,*v2;
  struct var_variable **vlist;
  long half,i,len,vlen;
  int happy=1;

  if (slvsys==NULL) {
    return NULL;
  }
  sys=(struct Integ_system_t *)asccalloc(1,sizeof(struct Integ_system_t));
  if (sys==NULL) {
    return sys;
  }
  l_isys = sys;
  IntegInitSymbols();

  /* collect potential states and derivatives */
  sys->indepvars = gl_create(10L);  /* t var info */
  sys->dynvars = gl_create(200L);  /* y ydot var info */
  sys->obslist = gl_create(100L);  /* obs info */
  if (sys->dynvars == NULL ||
      sys->obslist == NULL ||
      sys->indepvars == NULL ) {
    FPRINTF(ASCERR,"ERROR: (Integ_system_build) Insufficient memory.\n");
    Integ_system_destroy(sys);
    l_isys = NULL;
    return l_isys;
  }
  l_nstates = l_nderivs = 0;
  /* visit all the slv_system_t master var lists to collect vars */
  /* find the vars mostly in this one */
  vlist = slv_get_master_var_list(slvsys);
  vlen = slv_get_num_master_vars(slvsys);
  for (i=0;i<vlen;i++) {
    Integ_classify_vars(vlist[i]);
  }
  /* probably nothing here, but gotta check. */
  vlist = slv_get_master_par_list(slvsys);
  vlen = slv_get_num_master_pars(slvsys);
  for (i=0;i<vlen;i++) {
    Integ_classify_vars(vlist[i]);
  }
  /* might find t here */
  vlist = slv_get_master_unattached_list(slvsys);
  vlen = slv_get_num_master_unattached(slvsys);
  for (i=0;i<vlen;i++) {
    Integ_classify_vars(vlist[i]);
  }

  /* check the sanity of the independent variable */
  len = gl_length(sys->indepvars);
  if (!len) {
    FPRINTF(ASCERR,
      "ERROR: (Integ_system_build) No independent variable found.\n");
    Integ_system_destroy(sys);
    l_isys = NULL;
    return l_isys;
  }
  if (len > 1) {
    char *name;
    FPRINTF(ASCERR,
      "ERROR: (Integ_system_build) Excess %ld independent variables found:\n",
      len);
    for (i=1; i <=len;i++) {
      v1 = (struct Integ_var_t *)gl_fetch(sys->indepvars,i);
      if (v1 != NULL) {
        name = WriteInstanceNameString(var_instance(v1->i),g_solvinst_cur);
        if (name !=NULL) {
          FPRINTF(ASCERR,"\t\n");
          ascfree(name);
          name = NULL;
        }
      }
    }
    FPRINTF(ASCERR, "Set the %s on all but one of these to %s >= 0.\n",
            SCP(STATEFLAG),SCP(STATEFLAG));
    Integ_system_destroy(sys);
    l_isys = NULL;
    return l_isys;
  } else {
    v1 = (struct Integ_var_t *)gl_fetch(sys->indepvars,1);
    sys->x = v1->i;
  }
  /* check sanity of state and var lists */
  len = gl_length(sys->dynvars);
  half = len/2;
  if (len % 2 || len == 0L || l_nstates != l_nderivs ) {
    /* list length must be even for vars to pair off */
    FPRINTF(ASCERR,"ERROR: (Integ_system_build) n_y != n_ydot\n");
    FPRINTF(ASCERR,"\tor no dynamic vars found. Fix your indexing.\n");
    Integ_system_destroy(sys);
    l_isys = NULL;
    return l_isys;
  }
  gl_sort(sys->dynvars,(CmpFunc)Integ_CmpDynVars);
  if (gl_fetch(sys->dynvars,len)==NULL) {
    FPRINTF(ASCERR,"ERROR: (Integ_system_build) Mysterious NULL found.\n");
    FPRINTF(ASCERR,"\tPlease Report this to %s.\n",ASC_BIG_BUGMAIL);
    Integ_system_destroy(sys);
    l_isys = NULL;
    return l_isys;
  }
  sys->states = gl_create(half);   /* state vars Integ_var_t references */
  sys->derivs = gl_create(half);   /* derivative var atoms */
  for (i=1;i < len; i+=2) {
    v1 = (struct Integ_var_t *)gl_fetch(sys->dynvars,i);
    v2 = (struct Integ_var_t *)gl_fetch(sys->dynvars,i+1);
    if (v1->type!=1  || v2 ->type !=2 || v1->index != v2->index) {
      FPRINTF(ASCERR,"ERROR: (Integ_system_build) Mistyped or misindexed\n");
      FPRINTF(ASCERR,
             "\tdynamic variables: (%s = %ld,%s = %ld),(%s = %ld,%s = %ld).\n",
             SCP(STATEFLAG),v1->type,SCP(STATEINDEX),v1->index,
             SCP(STATEFLAG),v2->type,SCP(STATEINDEX),v2->index);
      happy=0;
      break;
    } else {
      gl_append_ptr(sys->states,(POINTER)v1);
      gl_append_ptr(sys->derivs,(POINTER)v2->i);
    }
  }
  if (!happy) {
    Integ_system_destroy(sys);
    l_isys = NULL;
    return l_isys;
  }
  sys->n_y = half;
  sys->y = (struct var_variable **)
    ascmalloc(sizeof(struct var_variable *)*half);
  sys->y_id = (long *)ascmalloc(sizeof(long)*half);
  sys->ydot = (struct var_variable **)
    ascmalloc(sizeof(struct var_variable *)*half);
  if (sys->y==NULL || sys->ydot==NULL || sys->y_id==NULL) {
    FPRINTF(ASCERR,"ERROR: (Integ_system_build) Insufficient memory.\n");
    Integ_system_destroy(sys);
    l_isys = NULL;
    return l_isys;
  }
  for (i = 1; i <= half; i++) {
    v1 = (struct Integ_var_t *)gl_fetch(sys->states,i);
    sys->y[i-1] = v1->i;
    sys->y_id[i-1] = v1->index;
    sys->ydot[i-1] = (struct var_variable *)gl_fetch(sys->derivs,i);
  }

  /* reindex observations. sort if the user mostly numbered. take
   * natural order if user just booleaned.
   */
  len = gl_length(sys->obslist);
  /* we shouldn't be seeing NULL here ever except if malloc fail. */
  if (len > 1L) {
    half = ((struct Integ_var_t *)gl_fetch(sys->obslist,1))->index;
    /* half != 0 now because we didn't collect 0 indexed vars */
    for (i=2; i <= len; i++) {
      if (half != ((struct Integ_var_t *)gl_fetch(sys->obslist,i))->index) {
        /* change seen. sort and go on */
        gl_sort(sys->obslist,(CmpFunc)Integ_CmpObs);
        break;
      }
    }
  }
  for (i = half = 1; i <= len; i++) {
    v2 = (struct Integ_var_t *)gl_fetch(sys->obslist,i);
    if (v2==NULL) {
      /* we shouldn't be seeing NULL here ever except if malloc fail. */
      gl_delete(sys->obslist,i,0); /* should not be gl_delete(so,i,1) */
    } else {
      Integ_SetObsId(v2->i,half);
      v2->index = half++;
    }
  }

  /* obslist now uniquely indexed, no nulls */
  /* make into arrays */
  half = gl_length(sys->obslist);
  sys->obs = (struct var_variable **)
    ascmalloc(sizeof(struct var_variable *)*half);
  sys->obs_id = (long *)ascmalloc(sizeof(long)*half);
  if ( sys->obs==NULL || sys->obs_id==NULL) {
    FPRINTF(ASCERR,"ERROR: (Integ_system_build) Insufficient memory.\n");
    Integ_system_destroy(sys);
    l_isys = NULL;
    return l_isys;
  }
  sys->n_obs = half;
  for (i = 1; i <= half; i++) {
    v2 = (struct Integ_var_t *)gl_fetch(sys->obslist,i);
    sys->obs[i-1] = v2->i;
    sys->obs_id[i-1] = v2->index;
  }

  /* don't need the gl_lists now that we have arrays for everyone */
  gl_destroy(sys->states);
  gl_destroy(sys->derivs);
  gl_free_and_destroy(sys->indepvars);  /* we own the objects in indepvars */
  gl_free_and_destroy(sys->dynvars);    /* we own the objects in dynvars */
  gl_free_and_destroy(sys->obslist);    /* and obslist */
  sys->states = NULL;
  sys->derivs = NULL;
  sys->indepvars = NULL;
  sys->dynvars = NULL;
  sys->obslist = NULL;
  return l_isys;
}

/* boolean return. 0 = bad 1 = ok. BLSODE required setup stuff. */
static int Integ_setup_blsode(enum Integrator_type integrator)
{
  struct Integ_system_t *blsys=NULL;

  g_intgsys_cur = g_solvsys_cur;
  if (g_intgsys_cur == NULL) {
    FPRINTF(stderr, "g_intgsys_cur not correctly assigned.\n");
    return 0;
  }
  /* verify integrator type ok. always passes for nonNULL inst. */
  if (!(Asc_IntegInstIntegrable(g_solvinst_cur, integrator))) {
    FPRINTF(stderr, "System is of wrong type to be ");
    FPRINTF(stderr, "integrated using BLSODE.\n");
    return 0;
  }

  /* this is a lie, but we will keep it.
     We handle any linsol/linsolqr based solver. */
  if (strcmp(slv_solver_name(slv_get_selected_solver(g_intgsys_cur)),"QRSlv")
      != 0) {
    FPRINTF(stderr, "QRSlv must be selected before integration.\n");
    return 0;
  }

  blsys = Integ_system_build(g_solvsys_cur);
  if (blsys == NULL) {
    FPRINTF(ASCERR,"Unable to build blsode system.\n");
    return 0;
  }

  g_intginst_d_n_eq = blsys->n_y;
  g_intginst_d_n_obs = blsys->n_obs;

  g_intginst_d_x = blsys->x;
  g_intginst_d_y_vars = blsys->y;
  g_intginst_d_dydx_vars = blsys->ydot;
  g_intginst_d_obs_vars = blsys->obs;
  blsys->ycount++;
  blsys->ydotcount++;
  blsys->obscount++;

  print_debugstring("After values\n");
  return 1;
}

/*
 *  takes the type of integrator and sets up the global variables into
 *  the current integration instance.
 */
static int Integ_setup(enum Integrator_type integrator,
                       long i0, long i1,
                       double dt0,double dtmin,double dtmax,
                       int moststeps)
{
  long nstep;
  unsigned long start_index=0, finish_index=0;
  struct Integ_system_t *blsys;

  switch (integrator) {
  case BLSODE:
    if (!Integ_setup_blsode(integrator)) {
      return 0;
    }
    blsys = l_isys;
    l_isys = NULL;
    break;
  default:
    FPRINTF(stderr, "The requested type of integrator is ");
    FPRINTF(stderr, "not currently available\n.");
    return 0;
  } /* END of integrator CASE */

  switch (integrator) {
  case BLSODE:
    nstep = Asc_IntegGetNSamples()-1;
    /* check for at least 2 steps and dimensionality of x vs steps here */
    break;
  default:
    nstep = 0;
    break;
  }
  if (i0<0 || i1 <0) {
    FPRINTF(stdout, "An integration interval had been defined ");
    FPRINTF(stdout, "from x[0] to x[%li].\n", nstep);
    FPRINTF(stdout, "Enter start index: ");
    start_index = (int)readlong((int)start_index);

    if (start_index >= (unsigned long)nstep) {
      FPRINTF(stderr, "ERROR: Start point < 0 OR start point >= %li.\n",nstep);
      Integ_system_destroy(blsys);
      return 0;
    }

    FPRINTF(stdout, "Enter finish index: ");
    finish_index = readlong((int)finish_index);

    if (finish_index > (unsigned long)nstep) {
      FPRINTF(stderr, "ERROR: finish point < 0 OR finish point > %li.\n",nstep);
      FPRINTF(stderr, "       finish point = %lu\n",finish_index);
      Integ_system_destroy(blsys);
      return 0;
    }
  } else {
    start_index=i0;
    finish_index =i1;
    if (start_index >= (unsigned long)nstep) {
      FPRINTF(stderr, "ERROR: Start point < 0 OR start point >= %li.\n",nstep);
      FPRINTF(stderr, "       Start point = %lu\n",start_index);
      Integ_system_destroy(blsys);
      return 0;
    }
    if (finish_index > (unsigned long)nstep) {
      FPRINTF(stderr, "ERROR: finish point < 0 OR finish point > %li.\n",nstep);
      FPRINTF(stderr, "       finish point = %lu\n",finish_index);
      Integ_system_destroy(blsys);
      return 0;
    }
  }
  if ((finish_index <= start_index) || (start_index >= finish_index)) {
    FPRINTF(stderr, "ERROR: Finish point <= start point OR ");
    FPRINTF(stderr, "start point >= finish point.\n");
    FPRINTF(stderr, "       Start point = %lu\n",start_index);
    FPRINTF(stderr, "       finish point = %lu\n",finish_index);
    Integ_system_destroy(blsys);
    return 0;
  }
  blsys->maxsteps = moststeps;
  blsys->stepzero = dt0;
  blsys->stepmin = dtmin;
  blsys->stepmax = dtmax;

  switch (integrator) {
  case BLSODE:
  /*
   * Set up the Jacobian for the system. Does not check
   * This assumes a clean sytem. Shut down will ensure a
   * clean system at the end.
   */
    if (Integ_SetUpDiffs_BLsode(blsys)) {
      return 0;
    }
    break;
  default:
    break;
  }
  Integ_Solve(integrator, start_index, finish_index,blsys);
  Integ_system_destroy(blsys);
  return 0;
}

/*
 * switches:
 * -engine $name
 * -i0 $stepindex
 * -i1 $stepindex
 * -dt0 $initstepsize
 * -dtmin $minstep
 * -dtmax $maxstep
 * -moststeps $moststeps
 */
int Asc_IntegSetupCmd(ClientData cdata,Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{
  enum Integrator_type integrator = UNKNOWN;
  char buf[MAXIMUM_NUMERIC_LENGTH];         /* string to hold integer */
  CONST84 char *engine = NULL;
  int result = 0;         /* 0 = FALSE; 1 = TRUE */
  long i0=(-1), i1=(-1);
  int ifound = 0;
  int k;
  int moststeps=0;
  double dt0=0, dtmin=0, dtmax=0;
  CONST84 char *cdt0=NULL, *cdtmin=NULL, *cdtmax=NULL, *cmoststeps=NULL,
       *ci0=NULL, *ci1=NULL;

  (void)cdata;    /* stop gcc whine about unused parameter */

  k = 1;
  while (k < (argc-1)) { /* arguments come in pairs */
    if (strcmp(argv[k],"-engine")==0) {
      engine = argv[k+1];
      k+=2;
      continue;
    }
    if (strcmp(argv[k],"-i1")==0) {
      ci1 = argv[k+1];
      k+=2;
      continue;
    }
    if (strcmp(argv[k],"-i0")==0) {
      ci0 = argv[k+1];
      k+=2;
      continue;
    }
    if (strcmp(argv[k],"-moststeps")==0) {
      cmoststeps = argv[k+1];
      k+=2;
      continue;
    }
    if (strcmp(argv[k],"-dtmax")==0) {
      cdtmax = argv[k+1];
      k+=2;
      continue;
    }
    if (strcmp(argv[k],"-dtmin")==0) {
      cdtmin = argv[k+1];
      k+=2;
      continue;
    }
    if (strcmp(argv[k],"-dt0")==0) {
      cdt0 = argv[k+1];
      k+=2;
      continue;
    }
    Tcl_AppendResult(interp,argv[0],": unrecognized option ",
                     argv[k],".",SNULL);
    return TCL_ERROR;
  }

  if (engine != NULL && strncmp(engine,"BLSODE",3)==0) {
    integrator = BLSODE;
    ifound=1;
  }
  if (!ifound) {
    Tcl_SetResult(interp, "Unsupported integrator", TCL_STATIC);
    Tcl_AppendResult(interp," ",engine,SNULL);
    return TCL_ERROR;
  }
  if (ci0 != NULL && ci1 != NULL) {
    /* get i0, i1 if both supplied. */
    long i;
    if (Tcl_ExprLong(interp,ci0,&i)==TCL_ERROR|| i<0) {
      Tcl_ResetResult(interp);
      Tcl_SetResult(interp, "integrate_setup: index i0 invalid", TCL_STATIC);
      return TCL_ERROR;
    }
    i0=i;
    if (Tcl_ExprLong(interp,ci1,&i)==TCL_ERROR|| i<i0) {
      Tcl_ResetResult(interp);
      Tcl_SetResult(interp, "integrate_setup: index i1 invalid", TCL_STATIC);
      return TCL_ERROR;
    }
    i1=i;
  }
  if (cdt0 != NULL) {
    if (Tcl_GetDouble(interp,cdt0,&dt0) != TCL_OK) {
      Tcl_ResetResult(interp);
      Tcl_AppendResult(interp, "integrate_setup: initial step length invalid",
                       " (",cdt0,")", SNULL);
      return TCL_ERROR;
    }
  }
  if (cdtmin != NULL) {
    if (Tcl_GetDouble(interp,cdtmin,&dtmin) != TCL_OK || dtmin < 0) {
      Tcl_ResetResult(interp);
      Tcl_AppendResult(interp, "integrate_setup: minimum step length invalid",
                       " (",cdtmin,")", SNULL);
      return TCL_ERROR;
    }
  }
  if (cdtmax != NULL) {
    if (Tcl_GetDouble(interp,cdtmax,&dtmax) != TCL_OK || dtmax < dtmin) {
      Tcl_ResetResult(interp);
      Tcl_AppendResult(interp, "integrate_setup: maximum step length invalid",
                       " (",cdtmax,")", SNULL);
      return TCL_ERROR;
    }
  }
  if (cmoststeps != NULL) {
    if (Tcl_GetInt(interp,cmoststeps,&moststeps) != TCL_OK || moststeps < 0) {
      Tcl_ResetResult(interp);
      Tcl_AppendResult(interp, "integrate_setup: maximum internal steps bad",
                       " (",cmoststeps,")", SNULL);
      return TCL_ERROR;
    }
  }
  result = Integ_setup(integrator,i0,i1,dt0,dtmin,dtmax,moststeps);
  sprintf(buf, "%d", result);
  Tcl_SetResult(interp, buf, TCL_VOLATILE);
  return TCL_OK;
}

/*
 *  Deallocates any memory used and sets all integration global points
 *  to NULL.
 */
static int Integ_Cleanup() {
  g_intgsys_cur = NULL;
  g_intginst_d_x = NULL;
  if(g_intginst_d_y_vars !=NULL) {
    ascfree(g_intginst_d_y_vars);
  }
  if(g_intginst_d_dydx_vars !=NULL) {
    ascfree(g_intginst_d_dydx_vars);
  }
  if(g_intginst_d_obs_vars !=NULL) {
    ascfree(g_intginst_d_obs_vars);
  }
  g_intginst_d_y_vars=NULL;
  g_intginst_d_dydx_vars=NULL;
  g_intginst_d_obs_vars=NULL;

  /*
   * Cleanup the derivative stuff.
   */
  if (g_intg_diff.input_indices) {
    ascfree((char *)g_intg_diff.input_indices);
  }
  if (g_intg_diff.output_indices) {
    ascfree((char *)g_intg_diff.output_indices);
  }
  if (g_intg_diff.y_vars) {
    ascfree((char *)g_intg_diff.y_vars);
  }
  if (g_intg_diff.ydot_vars) {
    ascfree((char *)g_intg_diff.ydot_vars);
  }
  if (g_intg_diff.rlist) {
    ascfree((char *)g_intg_diff.rlist);
  }
  if (g_intg_diff.dydx_dx) {
    DestroyDenseMatrix(g_intg_diff.dydx_dx, g_intg_diff.n_eqns);
  }

  g_intg_diff.input_indices = NULL;
  g_intg_diff.output_indices = NULL;
  g_intg_diff.y_vars = NULL;
  g_intg_diff.ydot_vars = NULL;
  g_intg_diff.dydx_dx =  NULL;
  g_intg_diff.rlist =  NULL;
  g_intg_diff.n_eqns = 0L;

  print_debugstring("integrate_cleanup\n");
  return 0;
}

/********************************************************************/

int Asc_IntegCleanupCmd(ClientData cdata,Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if (argc!=1) {
    Tcl_SetResult(interp, "integrate_cleanup takes no arguments", TCL_STATIC);
    return TCL_ERROR;
  }

  Integ_Cleanup();
  return TCL_OK;
}
