/*	ASCEND modelling environment
	Copyright 1997, Carnegie Mellon University
	Copyright (C) 2006 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//**
	@file
	Tcl/Tk interface functions for the Integration feature
*//*
	by Kirk Abbott, Ben Allan, John Pye
	Created: 1/94
	Last in CVS: $Revision: 1.32 $ $Date: 2003/08/23 18:43:06 $ $Author: ballan $
*/

#define ASC_BUILDING_INTERFACE

#include <tcl.h>
#include <time.h>

#include <compiler/instance_io.h>
#include <compiler/units.h>

#include <utilities/ascConfig.h>
#include <integrator/integrator.h>
#include <integrator/lsode.h>
#include <integrator/samplelist.h>

#include "HelpProc.h"
#include "Integrators.h"
#include "BrowserQuery.h"
#include "Qlfdid.h"
#include "UnitsProc.h"
#include "BrowserProc.h"
#include "HelpProc.h"
#include "SolverGlobals.h"

#define SNULL (char *)NULL

static SampleList l_samplelist;

/*-------------------------------------------------------
  HANDLING OF OUTPUT FILES
*/

/* vars relating to output files */
static FILE *l_obs_file = NULL;
static char *l_obs_filename = NULL;

static FILE *l_y_file = NULL;
static char *l_y_filename = NULL;

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

void Asc_IntegPrintYHeader(FILE *fp, IntegratorSystem *blsys)
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
void Asc_IntegPrintObsHeader(FILE *fp, IntegratorSystem *blsys)
{
  long i,len, *obsip;
  char *name;
  struct Instance *in;
  int si;
  if (fp==NULL) {
    return;
  }
  if (blsys==NULL) {
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"called without data");
    return;
  }
  if (blsys->n_obs == 0) {
    return;
  }
  if (blsys->obs == NULL) {
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"called with NULL data");
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

/**
	@return 1 on success
*/
int Asc_IntegPrintYLine(FILE *fp, IntegratorSystem *blsys)
{
  long i,len;
  struct var_variable **vp;
  int si;
  if (fp==NULL) {
    return 0;
  }
  if (blsys==NULL) {
    FPRINTF(ASCERR,"WARNING: (Asc_IntegPrintYLine: called w/o data\n");
    return 0;
  }
  if (blsys->n_y == 0) {
    return 0;
  }
  if (blsys->y == NULL) {
    FPRINTF(ASCERR,"ERROR: (Asc_IntegPrintYHeader: called w/NULL data\n");
    return 0;
  }
  vp = blsys->y;
  len = blsys->n_y;
  si = l_print_option;
  FPRINTF(fp,BCOLNFMT,Asc_UnitlessValue(var_instance(blsys->x),si));
  for (i=0; i < len; i++) {
    FPRINTF(fp,BCOLNFMT, Asc_UnitlessValue(var_instance(vp[i]),si));
  }
  FPRINTF(fp,"\n");
  return 1;
}

/**
	@return 1 on success
*/
int Asc_IntegPrintObsLine(FILE *fp, IntegratorSystem *blsys){
  long i,len;
  struct var_variable **vp;
  int si;
  if (fp==NULL) {
    return 0;
  }
  if (blsys==NULL) {
    FPRINTF(ASCERR,"WARNING: (Asc_IntegPrintObsLine: called w/o data\n");
    return 0;
  }
  if (blsys->n_obs == 0) {
    return 0;
  }
  if (blsys->obs == NULL) {
    FPRINTF(ASCERR,"ERROR: (Asc_IntegPrintObsHeader: called w/NULL data\n");
    return 0;
  }
  vp = blsys->obs;
  len = blsys->n_obs;
  si = l_print_option;
  FPRINTF(fp,BCOLNFMT,Asc_UnitlessValue(var_instance(blsys->x),si));
  for (i=0; i < len; i++) {
    FPRINTF(fp,BCOLNFMT, Asc_UnitlessValue(var_instance(vp[i]),si));
  }
  FPRINTF(fp,"\n");
  return 1;
}


/*---------------------------------------------*/

int Asc_IntegSetYFileCmd(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  size_t len;

  UNUSED_PARAMETER(cdata);

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

/*---------------------------------------------*/

int Asc_IntegSetObsFileCmd(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  size_t len;

  UNUSED_PARAMETER(cdata);

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

/*---------------------------------------------*/

int Asc_IntegSetFileUnitsCmd(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[])
{
  UNUSED_PARAMETER(cdata);

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

/*---------------------------------------------*/

int Asc_IntegSetFileFormatCmd(ClientData cdata, Tcl_Interp *interp,
                             int argc, CONST84 char *argv[])
{
  UNUSED_PARAMETER(cdata);

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


/*---------------------------------------------------------------
  DEFINING THE TIMESTEPS FOR INTEGRATION
*/

int Asc_IntegGetXSamplesCmd(ClientData cdata, Tcl_Interp *interp,
                          int argc, CONST84 char *argv[])
{
  static char sval[40]; /* buffer long enough to hold a double printed */
  struct Units *du = NULL;
  const dim_type *dp;
  long i,len;
  double *uvalues = NULL;
  char *ustring;
  double *uv;
  int trydu=0, prec, stat=0;

  UNUSED_PARAMETER(cdata);

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

  len = samplelist_length(&l_samplelist);
  dp = samplelist_dim(&l_samplelist);

  if (len <1) {
    Tcl_SetResult(interp, "{} {}", TCL_STATIC);
    return TCL_OK;
  }

  if (trydu) {

    /* Allocate the space for the retrieved values... */
    uvalues = ASC_NEW_ARRAY(double,len);
    if (uvalues == NULL) {
      Tcl_SetResult(interp, "integrate_get_samples: Insufficient memory.",
                    TCL_STATIC);
      return TCL_ERROR;
    }

	/* Get the display units at string... */
    ustring = Asc_UnitDimString(dp,0);

    /* Get the conversion factor... */
    du = (struct Units *)LookupUnits(ustring);
    if (du == NULL) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"LookupUnits failed :-/");
      stat = 1;
    }else{
      stat = 0;
      /* fill 'uvalues' with scaled values (in output units) */
      uv = uvalues;
      for (i=0; i < len; i++) {
      	/* convert to output units */
        stat = Asc_UnitConvert(du,samplelist_get(&l_samplelist,i),uv,1);
        if (stat) {
          /* any problems, just stop */
          break;
        }
        uv++;
      }
    }
    if (stat) {
      /* there was a problem, so free the allocated space */
      ascfree(uvalues);
    }
  }

  /* give Tcl the units string */
  Tcl_AppendResult(interp,"{",ustring,"} {",SNULL);

  /* work out what precision we want */
  prec = Asc_UnitGetCPrec();

  len--; /* last one is a special case...? */
  if (!trydu || stat){
  	/* no unit conversion, or failed unit conversion: use the raw values */
  	for(i=0;i<len;i++){
  		sprintf(sval,"%.*g ",prec,samplelist_get(&l_samplelist,i));
    	Tcl_AppendResult(interp,sval,SNULL);
  	}
  }else{
  	for(i=0;i<len;i++){
  		sprintf(sval,"%.*g ",prec,uvalues[i]);
    	Tcl_AppendResult(interp,sval,SNULL);
  	}
  	ascfree(uvalues);
  }
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
  const dim_type *dp;
  dim_type *mydp;
  long i,len;
  double *uvalues = NULL;
  double *uv;
  int stat;

  UNUSED_PARAMETER(cdata);

  if (argc == 1) {
    samplelist_assign(&l_samplelist,0L,NULL,NULL);
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
  dp = (const dim_type *)UnitsDimensions(du);
#if INTDEBUG
  FPRINTF(ASCERR,"user dimen looks like\n");
  PrintDimen(ASCERR,dp);
#endif
  mydp = ASC_NEW(dim_type); /* we are giving away */
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
  uvalues = ASC_NEW_ARRAY(double,len); /* we are giving away */
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
  if(!samplelist_assign(&l_samplelist,len,uvalues,mydp)){
    Tcl_SetResult(interp, "integrate_set_samples: Insufficient memory.",
                  TCL_STATIC);
    ascfree(uvalues);
    ascfree(mydp);
    return TCL_ERROR;
  }
  return TCL_OK;
}

/*----------------------------------------------------------------
  FUNCTIONS THAT QUERY THE INTEGRATOR/SOLVER
*/

/**
	Tcl/Tk interface function: is the specified instance integrable.

	There is a problem with this part of the interface. The
	'integrator_isintegrable' function is being called before the
	'integrator_analyse' step, which means that it really doesn't have all the
	information that it needs to be able to say. I've reworked it so that
	you would expect to get errors back from the 'integrator_solve' step if
	there is any problem.

	All this function now does is to check that the instance is OK, and to
	check that a valid integrator engine is specified.
*/
int Asc_IntegInstIntegrableCmd(ClientData cdata,Tcl_Interp *interp,
                             int argc, CONST84 char *argv[])
{
  struct Instance *i=NULL;
  IntegratorEngine integrator=INTEG_UNKNOWN;
  int result=0;         /* 0 = FALSE; 1 = TRUE */

  UNUSED_PARAMETER(cdata);

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

  integrator = INTEG_UNKNOWN;

  if (strncmp(argv[2],"blsode",3)==0) {
    integrator = INTEG_LSODE;
#ifdef ASC_WITH_IDA
  }else if (strncmp(argv[2],"ida",3)==0) {
    integrator = INTEG_IDA;
#endif
  }

  result = (integrator != INTEG_UNKNOWN);
  if (result) {
    Tcl_SetResult(interp, "1", TCL_STATIC);
  } else {
    Tcl_SetResult(interp, "0", TCL_STATIC);
  }
  return TCL_OK;
}

/*-----------------------------------*/

/**
	Set up the Integrator.

	switches (in Tcl/Tk)
		-engine $name
		-i0 $stepindex
		-i1 $stepindex
		-dt0 $initstepsize
		-dtmin $minstep
		-dtmax $maxstep
		-moststeps $moststeps
*/
int Asc_IntegSetupCmd(ClientData cdata,Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{
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
  IntegratorReporter *reporter;
  IntegratorSystem *blsys;

  UNUSED_PARAMETER(cdata);

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

  reporter = Asc_GetIntegReporter();

  blsys = integrator_new(g_solvsys_cur,g_solvinst_cur);
  result = integrator_set_engine(blsys, engine);

  if(result) {
	integrator_free(blsys);
    Tcl_SetResult(interp, "Unsupported integrator", TCL_STATIC);
    Tcl_AppendResult(interp," ",engine,SNULL);
    return TCL_ERROR;
  } 

  integrator_set_reporter(blsys, reporter);
  integrator_set_samples(blsys,&l_samplelist);
  integrator_set_stepzero(blsys,dt0);
  integrator_set_minstep(blsys,dtmin);
  integrator_set_maxstep(blsys,dtmax);
  integrator_set_maxsubsteps(blsys,moststeps);

  result = integrator_analyse(blsys);
  if(result){
     integrator_free(blsys);
     Tcl_SetResult(interp, "integrate_analyse: error returned", TCL_STATIC);
     return TCL_ERROR;
  }

  /* go and solve it */
  integrator_solve(blsys, i0, i1);

  /* once solution is finished, free whatever we allocated */
  integrator_free(blsys);

  sprintf(buf, "%d", result);
  Tcl_SetResult(interp, buf, TCL_VOLATILE);
  return TCL_OK;
}

/********************************************************************/

int Asc_IntegCleanupCmd(ClientData cdata,Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  UNUSED_PARAMETER(cdata);
  (void)argv;     /* stop gcc whine about unused parameter */

  if (argc!=1) {
    Tcl_SetResult(interp, "integrate_cleanup takes no arguments", TCL_STATIC);
    return TCL_ERROR;
  }

  /* integrator_cleanup(); */
  return TCL_OK;
}

/*-------------------------------------------------------------------
  REPORTER FUNCTIONS
*/

FILE *integ_y_out;
FILE *integ_obs_out;

IntegratorReporter *Asc_GetIntegReporter(){
	IntegratorReporter *r;
	r = (IntegratorReporter *)ascmalloc(sizeof(IntegratorReporter));
	r->init = &Asc_IntegReporterInit;
	r->write = &Asc_IntegReporterWrite;
	r->write_obs = &Asc_IntegReporterWriteObs;
	r->close = &Asc_IntegReporterClose;
	CONSOLE_DEBUG("CREATED INTEGRATORREPORTER FOR TCL/TK INTERFACE");
	return r;
}

int Asc_IntegReporterInit(IntegratorSystem *blsys){
	int status = 1;

	CONSOLE_DEBUG("INITIALISING REPORTER");

	/* set up output files */
	integ_y_out = Asc_IntegOpenYFile();
	integ_obs_out = Asc_IntegOpenObsFile();

	CONSOLE_DEBUG("RELEASING FILES");

	Asc_IntegReleaseYFile();
	Asc_IntegReleaseObsFile();

	CONSOLE_DEBUG("WRITING HEADERS");

	/* write headers to yout, obsout and initial points */

	Asc_IntegPrintYHeader(integ_y_out,blsys);
	status &= Asc_IntegPrintYLine(integ_y_out,blsys);
	Asc_IntegPrintObsHeader(integ_obs_out,blsys);
	status &= Asc_IntegPrintObsLine(integ_obs_out,blsys);

	return status;
	return 0;
}

int Asc_IntegReporterWrite(IntegratorSystem *blsys){
	/* write out a line of stuff */
    return Asc_IntegPrintYLine(integ_y_out,blsys);
}

int Asc_IntegReporterWriteObs(IntegratorSystem *blsys){
	return Asc_IntegPrintObsLine(integ_obs_out,blsys);
	return 0;
}

int Asc_IntegReporterClose(IntegratorSystem *blsys){
	/* close the file streams */
	if (integ_y_out!=NULL) {
		fclose(integ_y_out);
	}

	if (integ_obs_out!=NULL) {
		fclose(integ_obs_out);
	}
	return 1;
}

