/*
 *  MtxProc.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.29 $
 *  Version control file: $RCSfile: MtxProc.c,v $
 *  Date last modified: $Date: 2003/08/23 18:43:07 $
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
#include <setjmp.h>
#include <signal.h>
#endif  /* NO_SIGNAL_TRAPS */
#include "tcl.h"
#include "tk.h"
#include "utilities/ascConfig.h"
#include "utilities/ascSignal.h"
#include "utilities/ascMalloc.h"
#include "utilities/mem.h"
#include "utilities/set.h"
#include "general/list.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/compiler.h"
#include "compiler/instance_enum.h"
#include "compiler/symtab.h"
#include "compiler/instance_name.h"
#include "solver/slv_types.h"
#include "solver/calc.h"
#include "solver/mtx.h"
#include "solver/var.h"
#include "solver/rel.h"
#include "solver/discrete.h"
#include "solver/conditional.h"
#include "solver/logrel.h"
#include "solver/bnd.h"
#include "solver/relman.h"
#include "solver/slv_common.h"
#include "solver/linsol.h"
#include "solver/linsolqr.h"
#include "solver/slv_client.h"
#include "solver/slv_interface.h"
#include "solver/system.h"
#include "compiler/types.h"
#include "compiler/functype.h"
#include "compiler/func.h"
#include "compiler/extfunc.h"
#include "compiler/extcall.h"
#include "compiler/relation_type.h"
#include "interface/old_utils.h"
#include "interface/HelpProc.h"
#include "interface/Qlfdid.h"
#include "interface/MtxProc.h"
#include "interface/DisplayProc.h"
#include "interface/HelpProc.h"
#include "interface/SolverGlobals.h"

#ifndef lint
static CONST char MtxProcID[] = "$Id: MtxProc.c,v 1.29 2003/08/23 18:43:07 ballan Exp $";
#endif


#define TORF(b) ((b) ? "TRUE" : "FALSE")
#define YORN(b) ((b) ? "YES" : "NO")
#define ONEORZERO(b) ((b) ? "1" : "0")
#define SNULL (char *)NULL
#define QLFDID_LENGTH 1023
#define MP_DEBUG TRUE

static int GPI_Error(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argc;     /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

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

struct mem_mine {
  unsigned int pr2e :1; /* the next 5 will be set 1 if we */
  unsigned int e2pr :1; /* should deallocate, and 0 if not */
  unsigned int pc2v :1;
  unsigned int v2pc :1;
  unsigned int vfixed :1;
};
struct mplotvars {
  int nprow; /* number of plotted rows  (== neqn always for now) */
  int neqn; /* number of relations. orgrows 0-neqns are relations */
  int npcol; /* number of plotted columns (== nfakevar) */
  int nvar; /* number of variables. orgcols 0-nvars-1 are variables */
  int nfakevar; /* nvar -nnincident + nslack (currently always == npcol) */
  struct mem_mine own;
  int *pr2e; /* len nprow */ /* plot row to eqn id */
  int *e2pr; /* len neqn */ /* eqn id to plot row */
  int *pc2v; /* len npcol */ /* plot column to var org col */
  int *v2pc; /* len nvar */ /* var org col to plot col */
  char *vfixed; /* len nvar */ /* fixed flags by var org col */
  struct var_variable **vlist;
  struct rel_relation **rlist;
};

/* fill in an mplotvars struct */
/* return is 0 if ok, 1 otherwise */
/* this function is in charge of setting the layout that will appear
   in the canvas (row and column ordering). right now it is doing
   its own arrangement based on the mtx structure associated with
   the solver. Eventually it would be much better if the solvers
   provided the display orderings themselves.
   Also this does a 1time query of all the var fixed flags.

  6/96 baa. THIS FUNCTION NEEDS TO BE REWRITTEN OR DONE AWAY WITH
  IN FAVOR OF JUST USING THE SOLVERS VAR/REL LIST ORDERINGS SINCE
  THE SOLVERS CAN NOW REORDER THESE LISTS WE DON'T NEED TO MESS
  WITH LOOKING AT THEIR MATRICES DIRECTLY. go go gadget abstraction
  boundary.

  10/96. Done the note of 6/96.
  Where previously we looked up a permutation on the mtx,
  we now assume that the solver_*_list from sys is in the
  desired order.
  Probably that means some or all of the machinery here is
  irrelevant.

  1/97 vrr. Modified to plot only active variables and
  relations.

*/
static int Fill_GPI_Plot_Data(struct mplotvars *pd, slv_system_t sys)
{
  int32 mord=-1, row,col,var,rel,plrow,plcol,uninclow;
  var_filter_t vincident;
  rel_filter_t ractive;

  vincident.matchbits = (VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
  vincident.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE );

  ractive.matchbits = (REL_ACTIVE);
  ractive.matchvalue = (REL_ACTIVE );

  if (ISNULL(sys) || ISNULL(pd)) {
    FPRINTF(stderr,"Fill_GPI_Plot_Data called with NULL!\n");
    return 1;
  }

  pd->vlist = slv_get_solvers_var_list(sys);
  pd->rlist = slv_get_solvers_rel_list(sys);
  if (pd->vlist==NULL || pd->rlist==NULL) {
    FPRINTF(stderr,"Mtx: (Fill_GPI_Plot_Data) Nothing to plot!\n");
    return 1;
  }
  pd->neqn = slv_count_solvers_rels(sys,&ractive);
  pd->nprow = pd->neqn;
  pd->nvar = slv_get_num_solvers_vars(sys);
  pd->npcol = slv_count_solvers_vars(sys,&vincident);
  pd->nfakevar = pd->npcol; /* this could change with autoslack solvers */
  pd->pr2e = (int *)ascmalloc(sizeof(int)*(pd->nprow +1));
  pd->e2pr = (int *)ascmalloc(sizeof(int)*(pd->neqn +1));
  pd->pc2v = (int *)ascmalloc(sizeof(int)*(pd->npcol +1));
  pd->v2pc = (int *)ascmalloc(sizeof(int)*(pd->nvar +1));
  pd->vfixed = (char *)asccalloc((pd->nvar +1),sizeof(char));
  pd->own.pr2e = 1;
  pd->own.e2pr = 1;
  pd->own.pc2v = 1;
  pd->own.v2pc = 1;
  pd->own.vfixed = 1;
  if ( ISNULL(pd->pr2e) || ISNULL(pd->e2pr) ||
       ISNULL(pd->pc2v) || ISNULL(pd->v2pc) ||
       ISNULL(pd->vfixed) ) {
    FPRINTF(stderr,"Mtx: (Fill_GPI_Plot_Data) Insufficient memory.\n");
    return 1;
  }
  mord =  MAX(pd->neqn,pd->nvar);
  /* fix up row permutations */
  plrow=plcol=-1;
  uninclow=pd->neqn; /* set lowwater mark for unincluded eqns */
  for (row=0;row<mord;row++) {
    rel = row;
    if (rel < pd->neqn) {
      if (rel_included(pd->rlist[rel]) && rel_active(pd->rlist[rel])) {
        plrow++;
        pd->pr2e[plrow] = rel;
        pd->e2pr[rel] = plrow;
      } else {
        uninclow--;
        pd->pr2e[uninclow] = rel;
        pd->e2pr[rel] = uninclow;
      }
    } /* else skip this row: it is nothing */
  }
  for (col = 0; col < mord; col++) {
    var = col;
    if (var < pd->nvar) {
      /* set fixed flag vector whether incident or not */
      if (var_fixed(pd->vlist[var])) {
        pd->vfixed[var]=1;
      }
      if (var_incident(pd->vlist[var]) && var_active(pd->vlist[var])) {
        plcol++;
        pd->pc2v[plcol] = var;
        pd->v2pc[var] = plcol;
      } else {
        /* nonincident vars dont get plot cols */
        pd->v2pc[var] = -1;  /* be safe if someone asks */
      }
    } /* else skip this col: it is nothing */
  }
  return 0;
}

/* free memory in an mplotvars struct, if it is our memory */
static void Empty_GPI_Plot_Data(struct mplotvars *pd) {
  if (NOTNULL(pd)) {
    if (pd->own.pr2e)   {ascfree(pd->pr2e); pd->pr2e=NULL;}
    if (pd->own.e2pr)   {ascfree(pd->e2pr); pd->e2pr=NULL;}
    if (pd->own.pc2v)   {ascfree(pd->pc2v); pd->pc2v=NULL;}
    if (pd->own.v2pc)   {ascfree(pd->v2pc); pd->v2pc=NULL;}
    if (pd->own.vfixed) {ascfree(pd->vfixed); pd->vfixed=NULL;}
  }
}
int Asc_MtxGUIPlotIncidence(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char *argv[])
{
/*
TCL: mtx_gui_plot_incidence <sf xoff yoff cname bmfg bmbg ra ca va ea>\n");
WARNING: If you mess with this arg list, update the SET_PLOT_STRING macro
         below.
*/
  int sf,xoff,yoff,status,i,j,x,y,vndx;
  Tk_Window tkcanwin, tkwin;
  slv_system_t sys;
  struct mplotvars pd;
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
  if (Fill_GPI_Plot_Data(&pd,sys)) {
    FPRINTF(stderr,"mtx_gui_plot_incidence: error calculating grid\n");
    Tcl_SetResult(interp, "mtx_gui_plot_incidence: C plot calculation error",
                  TCL_STATIC);
    Empty_GPI_Plot_Data(&pd);
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
            Empty_GPI_Plot_Data(&pd);
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
                Empty_GPI_Plot_Data(&pd);
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

  Empty_GPI_Plot_Data(&pd);
  Tcl_ResetResult(interp);
  ascfree(plotstring);
  return TCL_OK;
}

#define LONGHELP(b,ms) ((b)?ms:"")
int Asc_MtxHelpList(ClientData cdata, Tcl_Interp *interp,
                  int argc, CONST84 char *argv[])
{
  boolean detail=1;

  (void)cdata;    /* stop gcc whine about unused parameter */

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
