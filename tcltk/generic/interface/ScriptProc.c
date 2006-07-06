/*
 *  ScriptProc.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.22 $
 *  Version control file: $RCSfile: ScriptProc.c,v $
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

#define ASC_BUILDING_INTERFACE
#include <time.h>
#include <tcl.h>
#include <tk.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <compiler/compiler.h>
#include <compiler/symtab.h>
#include <compiler/instance_enum.h>
#include <compiler/instquery.h>
#include <compiler/mergeinst.h>
#include "HelpProc.h"
#include "Qlfdid.h"
#include "BrowserProc.h"
#include "ScriptProc.h"

#ifndef lint
static CONST char ScriptProcID[] = "$Id: ScriptProc.c,v 1.22 2003/08/23 18:43:07 ballan Exp $";
#endif


#define SCRBUF_SIZE 1024

#if defined(sun) || defined(__sun__)
/* until sun headers are full ansi */
#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC  1000000         /* ANSI C clock ticks per sec */
#endif
#endif

/* this variable is linked to a tcl variable. */
int Asc_ScriptInterrupt = 0;

extern int Asc_ScriptConfigureInterrupt(int start, Tcl_Interp *interp)
{
  static char *varName;
  int result;
  /* assumes ansi NULL initialization of varName */
  if (start) {
    if (varName == NULL) {
      varName = ASC_NEW_ARRAY(char,60);
      if (varName == NULL) {
        return 1;
      }
      sprintf(varName,"%s","set ascScripVect(menubreak) 0");
      Asc_ScriptInterrupt = 0;
      result = Tcl_GlobalEval(interp,varName);
      if (result != TCL_OK) {
        return 2;
      }
/*
      Tcl_SetVar(interp,varName,"0",TCL_GLOBAL_ONLY);
 */
      sprintf(varName,"%s","ascScripVect(menubreak)");
      Tcl_LinkVar(interp,varName,
                  (char *)&Asc_ScriptInterrupt,TCL_LINK_INT);
    }  /* else double call, ignore it */
    return 0;
  } else {
    if (varName!=NULL) {
      sprintf(varName,"%s","ascScripVect(menubreak)");
      Tcl_UnlinkVar(interp, varName);
      ascfree(varName);
      varName = NULL;
      return 0;
    } else {
      return 1;
    }
  }
}

/*
 * Refine a qlfdid, if found, to the type specified, if possible.
 */
int Asc_ScriptRefineCmd(ClientData cdata, Tcl_Interp *interp,
                    int argc, CONST84 char *argv[])
{
  struct Instance *i;
  int status;

  if (argc!=4) {
    Tcl_SetResult(interp,"wrong # args : Usage srefine <type> search <qlfdid>",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  status = Asc_QlfdidSearch3(argv[3],0);
  if (status==0) {
    i = g_search_inst;
    if (!i) {
      Tcl_SetResult(interp, "srefine: NULL instance found in qlfdid search.",
                    TCL_STATIC);
      return TCL_ERROR;
    }
    status = Asc_BrowInstanceRefineCmd(cdata, interp,(int)3,argv);
  } else {
    Tcl_AppendResult(interp,"srefine: QlfdidSearch error,",
                     argv[3]," not found.",(char *)NULL);
  }
  return status;
}


int Asc_ScriptMergeCmd(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{
  struct Instance *i, *result=NULL;
  int status;

  UNUSED_PARAMETER(cdata);

  if (argc!=3) {
    Tcl_SetResult(interp, "wrong # args : Usage smerge <qlfdid> <qlfdid>",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  /* Process first qlfdid */
  status = Asc_QlfdidSearch3(argv[1],0);
  if (status==0) {
    i = g_search_inst;
    if (!i) {
      Tcl_SetResult(interp, "smerge: NULL instance found in qlfdid1 search.",
                    TCL_STATIC);
      return TCL_ERROR;
    }
  } else {
    Tcl_AppendResult(interp, "smerge: QlfdidSearch: ",argv[1],
                     " not found,",(char *)NULL);
    return TCL_ERROR;
  }

  /* Process second qlfdid */
  status = Asc_QlfdidSearch3(argv[2],0);
  if (status!=0) {
    Tcl_AppendResult(interp, "smerge: QlfdidSearch: ",argv[2],
                     " not found,",(char *)NULL);
    return TCL_ERROR;
  }
  if (!g_search_inst) {
    Tcl_SetResult(interp, "smerge: NULL instance found in qlfdid2 search.",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  switch(InstanceKind(i)) {
  case REAL_INST: case BOOLEAN_INST:
  case INTEGER_INST: case SYMBOL_INST:
  case SET_INST: case REL_INST:
    Tcl_AppendResult(interp,"AscendIV does not allow merging ",
                     "of children of Atoms.",argv[1],(char *)NULL);
    return TCL_ERROR;
  default:
    break;
  }
  switch(InstanceKind(g_search_inst)) {
  case REAL_INST: case BOOLEAN_INST:
  case INTEGER_INST: case SYMBOL_INST:
  case SET_INST: case REL_INST:
    Tcl_AppendResult(interp,"AscendIV does not allow merging ",
                     "of children of Atoms:",argv[2],(char *)NULL);
    return TCL_ERROR;
  default:
    break;
  }

  /* Do the merge */
  result = MergeInstances(i,g_search_inst);
  PostMergeCheck(result);
  if (!result) {
    Tcl_SetResult(interp, "Error in merging instances.", TCL_STATIC);
    return TCL_ERROR;
  }
  return TCL_OK;
}

int Asc_FastRaiseCmd(ClientData cdata, Tcl_Interp *interp,
                 int argc, CONST84 char *argv[])
{
  Tk_Window tkwin,mainwin;
  Window window;
  Display *display;

  UNUSED_PARAMETER(cdata);

  if (argc!=2) {
    Tcl_SetResult(interp, "wrong # args to asc_raise", TCL_STATIC);
    return TCL_ERROR;
  }
  mainwin = Tk_MainWindow(interp);
  tkwin = Tk_NameToWindow(interp,argv[1],mainwin);
  if (!tkwin) {
    return TCL_ERROR; /* a message should be in the interp->result */
  }
  display = Tk_Display(tkwin);
  window = Tk_WindowId(tkwin);
  XRaiseWindow(display,window);
  return TCL_OK;
}

int Asc_ScriptEvalCmd(ClientData cdata, Tcl_Interp *interp,
                  int argc, CONST84 char *argv[])
{
  /* see page 264 of the Tcl book. */
  int result;

  UNUSED_PARAMETER(cdata);

  if (argc!=2) {
    Tcl_SetResult(interp, "Error in running ScriptEval", TCL_STATIC);
    return TCL_ERROR;
  }
  if (Asc_ScriptInterrupt==1) {
    Asc_ScriptInterrupt = 0;
    Tcl_SetResult(interp, "Solver or Script interrupted by user", TCL_STATIC);
    return TCL_ERROR;
  }
  /* if (interp != g_interp) {
   *   FPRINTF(stderr,
   *     "ERROR: script_eval called from nested tcl scope. Trying anyway.\n");
   * }
   */
  result = Tcl_GlobalEval(interp,argv[1]);
  return result;
}

STDHLF(Asc_TimeCmd,(Asc_TimeCmdHL1,Asc_TimeCmdHL2,HLFSTOP));

int Asc_TimeCmd(ClientData cdata, Tcl_Interp *interp,
                  int argc, CONST84 char *argv[])
{
  clock_t max_clocks=0,min_clocks=0,avg_clocks=0,start=0,stop=0,dc=0,all_start=0,all_stop=0;
  double time_avg, real_time_avg;
  time_t time0, time1;
  int i,n=1, status=TCL_OK;
  char tmps[40];

  UNUSED_PARAMETER(cdata);

  ASCUSE;

  if (argc<2||argc>3) {
    Tcl_SetResult(interp, "call is: asc_clock {TCL script} iterations",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (argc==3) {
    status=Tcl_GetInt(interp,argv[2],&n);
    if (n<1 || status != TCL_OK) {
      Tcl_SetResult(interp, "asc_clock: called with bad number of iterations.",
                    TCL_STATIC);
      return status;
    }
  }
  time(&time0);
  all_start=clock();
  for (i=0; i<n; i++) {
    if (status != TCL_OK) {
      return status;
    }
    start=clock();
    status=Tcl_GlobalEval(interp,argv[1]);
    stop=clock();
    dc=stop-start;
    if (i==0) {
      min_clocks=max_clocks=dc;
    }
    if (dc> max_clocks) {
      max_clocks=dc;
    }
    if (dc< min_clocks) {
      min_clocks=dc;
    }
  }
  all_stop=clock();
  time(&time1);

  dc=all_stop-all_start;
  avg_clocks=dc/n;
  time_avg = ((double)dc) / ((double)CLOCKS_PER_SEC) / ((double)n);
#ifdef ASCDIFFTIME
  real_time_avg=(time1-time0)/((double)n);
#else
  real_time_avg=difftime(time1,time0)/n;
#endif

  sprintf(tmps,"%.8g",real_time_avg);
  Tcl_AppendElement(interp,tmps);

  sprintf(tmps,"%.8g",time_avg);
  Tcl_AppendElement(interp,tmps);

  sprintf(tmps,"%ld",(long)avg_clocks);
  Tcl_AppendElement(interp,tmps);

  sprintf(tmps,"%ld",(long)max_clocks);
  Tcl_AppendElement(interp,tmps);

  sprintf(tmps,"%ld",(long)min_clocks);
  Tcl_AppendElement(interp,tmps);

  sprintf(tmps,"%ld",(long)CLOCKS_PER_SEC);
  Tcl_AppendElement(interp,tmps);

  return TCL_OK;
}

/*
 *  String Compact: eat extra space in a string and detabify
 */
int Asc_StringCompact(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  size_t len,i ,c;
  int bracenest=0,wcnt;
  char *result=NULL;

  UNUSED_PARAMETER(cdata);

  if (argc != 2) {
    Tcl_SetResult(interp, "wrong args: stringcompact string", TCL_STATIC);
    return TCL_ERROR;
  }
  len = strlen(argv[1]);
  if (!len) {
    Tcl_SetResult(interp, "", TCL_STATIC);
    return TCL_OK;
  }
  result= (char *)ascmalloc(sizeof(char)*(len+1));
  if (result == NULL) {
    Tcl_SetResult(interp, "stringcompact: insufficient memory", TCL_STATIC);
    return TCL_ERROR;
  }
  memset(result,'#',len+1); /* fill whole array with #, including last spot */
  wcnt=1; /* trim leading whitespace */
  for (i=c=0; i < len; i++) {
    switch (argv[1][i]) {
    case '\t':
      if(!wcnt) {
        result[c++] = ' ';
        if (!bracenest) {
          wcnt++;
        }
      }
      break;
    case ' ':
     if(!wcnt) {
        result[c++] = argv[1][i];
        if (!bracenest) {
          wcnt++;
        }
      }
      break;
    case '{':
      wcnt = 0;
      bracenest++;
      result[c++] = argv[1][i];
      break;
    case '}':
      wcnt = 0;
      bracenest--;
      result[c++] = argv[1][i];
      if (bracenest < 0) {
        bracenest = 0;
      }
      break;
    default:
      wcnt = 0;
      result[c++] = argv[1][i];
    }
  }
  if (!bracenest && result[c]==' ') {
    result[c-1] = '\0';
  } else {
    result[c] = '\0';
  }
  Tcl_AppendResult(interp,result,(char *)NULL);
  ascfree(result);
  return TCL_OK;
}
