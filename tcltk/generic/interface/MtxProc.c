/*
 *  Incidence matrix routines
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
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

#include <tcl.h>
#include <tk.h>

#include <utilities/config.h>
#ifdef ASC_SIGNAL_TRAPS
#include <setjmp.h>
#include <general/except.h>
#endif

#include <utilities/ascConfig.h>
#include <utilities/ascSignal.h>
#include <utilities/ascMalloc.h>
#include <utilities/mem.h>
#include <utilities/set.h>
#include <general/list.h>
#include <compiler/fractions.h>
#include <compiler/dimen.h>

#include <compiler/instance_enum.h>
#include <compiler/symtab.h>
#include <compiler/instance_name.h>

#include <solver/incidence.h>

#include <compiler/expr_types.h>
#include <compiler/functype.h>
#include <compiler/func.h>
#include <compiler/extfunc.h>
#include <compiler/extcall.h>
#include <compiler/relation_type.h>

#include "old_utils.h"
#include "HelpProc.h"
#include "Qlfdid.h"
#include "MtxProc.h"
#include "DisplayProc.h"
#include "HelpProc.h"
#include "SolverGlobals.h"


#ifndef lint
static CONST char MtxProcID[] = "$Id: MtxProc.c,v 1.29 2003/08/23 18:43:07 ballan Exp $";
#endif


#define TORF(b) ((b) ? "TRUE" : "FALSE")
#define YORN(b) ((b) ? "YES" : "NO")
#define ONEORZERO(b) ((b) ? "1" : "0")
#define SNULL (char *)NULL
#define QLFDID_LENGTH 1023
#define MP_DEBUG TRUE

/* Some code moved to solver/incidence.h -- JP, 30 Jan 06 */

static int GPI_Error(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  /* stop gcc whine about unused parameters */
  (void)cdata;
  (void)argc;
  (void)argv;

  Tcl_ResetResult(interp);
  Tcl_SetResult(interp, "Error in args to mtx_gui_plot_incidence", TCL_STATIC);
  FPRINTF(stderr,
    "\nmtx_gui_plot_incidence <sf xoff yoff cname bmfg bmbg ra ca va ea>\n");
  FPRINTF(stderr,"\tsf is a bitmap size number 1-14\n");
  FPRINTF(stderr,"\txoff is upper left x position (pixels) \n");
  FPRINTF(stderr,"\tyoff is upper left y position (pixels) \n");
  FPRINTF(stderr,"\tcname is the name of an existing tk canvas\n");
  FPRINTF(stderr,"\tbmfg is the bitmap foreground color\n");
  FPRINTF(stderr,"\tbmbg is the bitmap background color\n");
  FPRINTF(stderr,"\tra, ca, va, ea are global arrays of index info\n");
  FPRINTF(stderr,"\tSee MtxProc.h for more details if needed.\n");
  return TCL_ERROR;
}

/*
TCL: mtx_gui_plot_incidence <sf xoff yoff cname bmfg bmbg ra ca va ea>\n");
WARNING: If you mess with this arg list, update the SET_PLOT_STRING macro
         below.rel_incidence_list
*/
int Asc_MtxGUIPlotIncidence(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char *argv[])
{

  int sf,xoff,yoff,status,i,j,x,y,vndx;
  Tk_Window tkcanwin, tkwin;
  slv_system_t sys;
  incidence_vars_t pd;
  struct rel_relation *rel;
  const struct var_variable **vp;
  char numstring[80],*plotstring;
  CONST84 char *svstat;
  int nvars,n;

  if ( argc != 11 ) {
    return GPI_Error(cdata, interp, argc, argv);
  }
  sf=1000;
  xoff=yoff=2;
  status=Tcl_GetInt(interp,argv[1],&sf);
  if (sf > 14 || sf <1 || status==TCL_ERROR) {
    FPRINTF(stderr,  "mtx_gui_plot_incidence: illegal sf given!\n");
    return GPI_Error(cdata, interp, argc, argv);
  }
  status=Tcl_GetInt(interp,argv[2],&xoff);
  if (status==TCL_ERROR) {
    FPRINTF(stderr,  "mtx_gui_plot_incidence: illegal xoff given!\n");
    return GPI_Error(cdata, interp, argc, argv);
  }
  status=Tcl_GetInt(interp,argv[3],&yoff);
  if (status==TCL_ERROR) {
    FPRINTF(stderr,  "mtx_gui_plot_incidence: illegal yoff given!\n");
    return GPI_Error(cdata, interp, argc, argv);
  }

  tkwin = Tk_MainWindow(interp);
  if (ISNULL(tkwin)) {
    FPRINTF(stderr,  "mtx_gui_plot_incidence: root window gone!\n");
    return GPI_Error(cdata, interp, argc, argv);
  }
  tkcanwin = Tk_NameToWindow(interp, argv[4], tkwin);
  if (ISNULL(tkcanwin)) {
    FPRINTF(stderr,  "mtx_gui_plot_incidence: illegal cname given!\n");
    return GPI_Error(cdata, interp, argc, argv);
  }
  /*
     we are not going to verify the colors, we are just going to handle
     errors if they do not plot properly.
  */
  sys = g_solvsys_cur;

  if (ISNULL(sys)) {
    FPRINTF(stderr,"mtx_gui_plot_incidence: called without slv sys\n");
    Tcl_SetResult(interp, "mtx_gui_plot_incidence: NULL solve system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
/* this needs to go
  mtx=slv_get_sys_mtx(sys);
  if (ISNULL(mtx)) {
    FPRINTF(stderr,"mtx_gui_plot_incidence: linear system has no mtx\n");
    Tcl_SetResult(interp, "mtx_gui_plot_incidence: C matrix missing! No Plot.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
*/
  if (build_incidence_data(sys,&pd)) {
    FPRINTF(stderr,"mtx_gui_plot_incidence: error calculating grid\n");
    Tcl_SetResult(interp, "mtx_gui_plot_incidence: C plot calculation error",
                  TCL_STATIC);
    free_incidence_data(&pd);
    return TCL_ERROR;
  }
#define MAX_BM_NAME_LEN 60
  /*for 2 lines, y is the length of the tcl string */
  y = strlen(argv[4]) /* window */
      + strlen(argv[5]) + strlen(argv[6]) /* colors */
      + 40 /* pixel coords x y */
      + MAX_BM_NAME_LEN /* bitmap name space */
      + 62; /* create bitmap -bitmap -anchor nw -background -foreground */
  plotstring=(char *)ascmalloc(sizeof(char)*y+1);
#undef MAX_BM_NAME_LEN

/* MACRO begin. really ugly. Dependent on arglist to function.*/
#define SET_PLOT_STRING(type) \
sprintf(plotstring, \
"%s create bitmap %d %d -bitmap %s%d -anchor nw -background %s -foreground %s"\
,argv[4],x,y,(type),sf,argv[6],argv[5])
/* MACRO end */

  for (i=0; i < pd.nprow; i++) {
    y=yoff+sf*i;
    rel=pd.rlist[pd.pr2e[i]];
    vp = rel_incidence_list(rel);
    if (rel_included(rel) && rel_active(rel)) { /* dense squares */
      nvars = rel_n_incidences(rel);
      for(n=0; n < nvars; n++ ) {
        if (var_flags(vp[n]) & VAR_SVAR) {
          vndx = var_sindex(vp[n]);
          x = xoff + sf*pd.v2pc[vndx];
          if (pd.vfixed[vndx]) {
            SET_PLOT_STRING("asc_sq_c");
            status=Tcl_GlobalEval(interp,plotstring);
          } else {
            SET_PLOT_STRING("asc_sq_");
            status=Tcl_GlobalEval(interp,plotstring);
          }
          if (status==TCL_ERROR) {
          FPRINTF(stderr,"Error plotting x%d y%d with:\n%s\n",x,y,plotstring);
            free_incidence_data(&pd);
            return status;
          }
        }
      }
    } else { /* hollow squares */
        if (rel_active(rel)) {
          nvars = rel_n_incidences(rel);
          for(n=0; n < nvars; n++ ) {
            if (var_flags(vp[n]) & VAR_SVAR) {
              vndx = var_sindex(vp[n]);
              x = xoff + sf*pd.v2pc[vndx];
              if (pd.vfixed[vndx]) {
                SET_PLOT_STRING("asc_sq_x");
              status=Tcl_GlobalEval(interp,plotstring);
              } else {
                SET_PLOT_STRING("asc_sq_h");
                status=Tcl_GlobalEval(interp,plotstring);
              }
              if (status==TCL_ERROR) {
                FPRINTF(stderr,"Error plotting x%d y%d with:\n%s\n",
                  x,y,plotstring);
                free_incidence_data(&pd);
                return status;
              }
            }
          }
        }
      }
  }
#undef SET_PLOT_STRING
/*
  at this point we can now use plotstring as just a big string buff
  for the array stuffing.
*/
  /* set arrays */
  /* we are only going to check the first setvar in each array */
  /* ra */
  sprintf(plotstring,"%d",pd.nprow);
  svstat=Tcl_SetVar2(interp,argv[7],"num",plotstring, TCL_GLOBAL_ONLY);
  if (ISNULL(svstat)) {
    FPRINTF(stderr,"mtx_gui_plot_incidence: Error setting %s(num)\n",argv[7]);
    return TCL_ERROR;
  }
  for (i=0; i < pd.nprow; i++) {
    sprintf(plotstring,"%d",i);
    sprintf(numstring,"%d",pd.pr2e[i]);
    Tcl_SetVar2(interp,argv[7],plotstring,numstring, TCL_GLOBAL_ONLY);
  }
  /* ca */
  sprintf(plotstring,"%d",pd.npcol);
  svstat=Tcl_SetVar2(interp,argv[8],"num",plotstring, TCL_GLOBAL_ONLY);
  if (ISNULL(svstat)) {
    FPRINTF(stderr,"mtx_gui_plot_incidence: Error setting %s(num)\n",argv[8]);
    return TCL_ERROR;
  }
  for (j=0; j < pd.npcol; j++) {
    sprintf(plotstring,"%d",j);
    sprintf(numstring,"%d",pd.pc2v[j]);
    Tcl_SetVar2(interp,argv[8],plotstring,numstring, TCL_GLOBAL_ONLY);
  }
  /* va */
  sprintf(plotstring,"%d",pd.nvar);
  svstat=Tcl_SetVar2(interp,argv[9],"num",plotstring, TCL_GLOBAL_ONLY);
  if (ISNULL(svstat)) {
    FPRINTF(stderr,"mtx_gui_plot_incidence: Error setting %s(num)\n",argv[9]);
    return TCL_ERROR;
  }
  for (j=0; j < pd.nvar; j++) {
    sprintf(plotstring,"%d",j);
    sprintf(numstring,"%d",pd.v2pc[j]);
    Tcl_SetVar2(interp,argv[9],plotstring,numstring, TCL_GLOBAL_ONLY);
  }
  /* ea */
  sprintf(plotstring,"%d",pd.neqn);
  svstat=Tcl_SetVar2(interp,argv[10],"num",plotstring, TCL_GLOBAL_ONLY);
  if (ISNULL(svstat)) {
    FPRINTF(stderr,"mtx_gui_plot_incidence: Error setting %s(num)\n",argv[10]);
    return TCL_ERROR;
  }
  for (i=0; i < pd.neqn; i++) {
    sprintf(plotstring,"%d",i);
    sprintf(numstring,"%d",pd.e2pr[i]);
    Tcl_SetVar2(interp,argv[10],plotstring,numstring, TCL_GLOBAL_ONLY);
  }

  free_incidence_data(&pd);
  Tcl_ResetResult(interp);
  ascfree(plotstring);
  return TCL_OK;
}

#define LONGHELP(b,ms) ((b)?ms:"")
int Asc_MtxHelpList(ClientData cdata, Tcl_Interp *interp,
                  int argc, CONST84 char *argv[])
{
  boolean detail=1;

  UNUSED_PARAMETER(cdata);

  if ( argc > 2 ) {
    FPRINTF(stderr,"call is: mtxhelp [s,l] \n");
    Tcl_SetResult(interp, "Too many args to mtxhelp. Want 0 or 1 args",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if ( argc == 2 ) {
    if (argv[1][0]=='s') {
      detail=0;
    }
    if (argv[1][0]=='l') {
      detail=1;
    }
    PRINTF("%-23s%s\n","mtx_gui_plot_incidence",
           LONGHELP(detail,"set TCL array/Tk canvas info"));
    PRINTF("%-23s%s\n","mtxhelp",
           LONGHELP(detail,"show this list"));
    PRINTF("\n");
  }
  if ( argc == 1 ) {
    char * tmps;
    tmps= (char *)ascmalloc((MAXIMUM_NUMERIC_LENGTH+1)*sizeof(char));
    sprintf(tmps,"mtx_gui_plot_incidence");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"mtxhelp");
    Tcl_AppendElement(interp,tmps);
    ascfree(tmps);
  }
  return TCL_OK;
}
