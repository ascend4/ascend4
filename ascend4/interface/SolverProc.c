/*
 *  SolverProc.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.65 $
 *  Version control file: $RCSfile: SolverProc.c,v $
 *  Date last modified: $Date: 2003/08/23 18:43:08 $
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

#include <math.h>
#include "tcl.h"
#include "tk.h"
#include "utilities/ascConfig.h"
#include "utilities/ascSignal.h"
#include "utilities/ascMalloc.h"
#include "general/tm_time.h"
#include "general/list.h"
#include "general/dstring.h"
#include "compiler/compiler.h"
#include "compiler/instance_enum.h"
#include "compiler/symtab.h"
#include "compiler/instance_io.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/types.h"
#include "compiler/relation_type.h"
#include "compiler/extfunc.h"
#include "compiler/find.h"
#include "compiler/relation.h"
#include "compiler/functype.h"
#include "compiler/func.h"
#include "compiler/safe.h"
#include "compiler/relation_util.h"
#include "compiler/pending.h"
#include "compiler/instance_name.h"
#include "compiler/instquery.h"
#include "compiler/parentchild.h"
#include "compiler/check.h"
#include "compiler/stattypes.h"
#include "compiler/instantiate.h"
#include "compiler/watchpt.h"
#include "solver/slv_types.h"
#include "solver/slv_types.h"
#include "solver/mtx.h"
#include "solver/rel.h"
#include "solver/var.h"
#include "solver/relman.h"
#include "solver/discrete.h"
#include "solver/conditional.h"
#include "solver/logrel.h"
#include "solver/bnd.h"
#include "solver/slv_common.h"
#include "solver/linsol.h"
#include "solver/linsolqr.h"
#include "solver/slv_client.h"
#include "solver/slv_server.h"   /* KHACK: not sure if this should be here */
/* #include "solver/slv0.h" */
/* #include "solver/slv1.h" */
/* #include "solver/slv2.h" */
/* #include "solver/slv3.h" */
#include "solver/slv6.h"             /*  modified by CWS 5/95 */
#include "solver/slv7.h"
#include "solver/slv9a.h"
#include "solver/slv_interface.h"
#include "solver/system.h"
#include "solver/cond_config.h"
#include "interface/old_utils.h"
#include "interface/HelpProc.h"
#include "interface/SolverGlobals.h"
#include "interface/SolverProc.h"
#include "interface/DisplayProc.h"
#include "interface/Commands.h"
#include "interface/SimsProc.h"
#include "interface/BrowserProc.h"
#include "interface/BrowserQuery.h"
#include "interface/Qlfdid.h"
#include "interface/UnitsProc.h"   /* KHACK: not sure if this should be here */
#include "interface/ScriptProc.h"
#include "interface/Driver.h"

#ifndef lint
static CONST char SolverProcID[] = "$Id: SolverProc.c,v 1.65 2003/08/23 18:43:08 ballan Exp $";
#endif


#define QLFDID_LENGTH 1023
#define YORN(b) ((b) ? "YES" : "NO")
#define ONEORZERO(b) ((b) ? "1" : "0")
#define SNULL (char *)NULL
#define SP_DEBUG FALSE
/* if true, prints out extra error messages */

/* global variables: */

int g_solvinst_ndx, g_solvinst_limit;
extern unsigned long g_unresolved_count;

struct Instance *g_solvinst_root=NULL, /* root instan (child of simulation) */
                *g_solvinst_cur=NULL;  /* top model instance to be solved */

slv_system_t g_solvsys_cur=NULL;        /* a pointer to slv_system_structure */
slv_system_t g_browsys_cur=NULL;        /* a pointer to slv_system_structure */

void Asc_SolvMemoryCleanup()
{
  system_free_reused_mem();
}

static
void slv_trap_int(int sigval)
{
  Tcl_Interp *interp = g_interp;  /*  a local ptr to the global interp ptr */

  (void)sigval;   /* stop gcc whine about unused parameter */

  FPRINTF(stdout,"\nascend4: SIGINT caught.\n");
  Solv_C_CheckHalt_Flag = 1; /* need to set the tcl var */
  Tcl_SetVar2(interp,"ascSolvStatVect","menubreak","1",TCL_GLOBAL_ONLY);
  Asc_ScriptInterrupt = 1;
  Asc_SetMethodUserInterrupt(1);
  FPRINTF(stdout,"Ctrl-D or click Toolbox/exit/Confirm to quit.\n");
  Asc_SignalRecover(0);
}

int Asc_SolvTrapFP(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{

  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)interp;   /* stop gcc whine about unused parameter */
  (void)argc;     /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  ASCUSE;
  Asc_SignalHandlerPush(SIGFPE,Asc_SignalTrap);
  Asc_SignalHandlerPush(SIGINT,slv_trap_int);
  return TCL_OK;
}

int Asc_SolvUnTrapFP(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)interp;   /* stop gcc whine about unused parameter */
  (void)argc;     /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  Asc_SignalHandlerPop(SIGFPE,Asc_SignalTrap);
  return TCL_OK;
}

int Asc_SolvGetModKids(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  char tmps[QLFDID_LENGTH+1];
  struct Instance *modinst_root=NULL;  /* model instance */
  struct Instance *aryinst_root=NULL;  /* possible model instance */
  struct Instance *aryinst=NULL;       /* possible model instance kid*/
  struct InstanceName rec;
  enum inst_t ikind,aikind;
  unsigned long len,c,aryc,arylen;
  int status;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "expected get_model_children <qlfdid>", TCL_STATIC);
    return TCL_ERROR;
  }

  status = Asc_QlfdidSearch3(argv[1],0);
  if (status==0) {
    modinst_root = g_search_inst;	/* catch inst ptr */
  } else {
    Tcl_AppendResult(interp,"get_model_children: QlfdidSearch error: ",
                     argv[1], " not found",SNULL);
    return TCL_ERROR;
  }

  /* check that instance is model */
  ikind=InstanceKind(modinst_root);
  if (ikind!=MODEL_INST && ikind!=ARRAY_INT_INST && ikind!= ARRAY_ENUM_INST) {
    FPRINTF(ASCERR,  "Instance specified is not a model or array.\n");
    Tcl_SetResult(interp,
                  "Only MODEL and ARRAY instances may have model children.",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  len=NumberChildren(modinst_root);
  for (c=1;c<=len;c++) {
    ikind=InstanceKind(InstanceChild(modinst_root,c));
    switch (ikind) {
      case MODEL_INST:
        Tcl_AppendElement(interp,
         (char *)InstanceNameStr(ChildName(modinst_root,c)));
        break;
      case ARRAY_INT_INST:
      case ARRAY_ENUM_INST: /*dumpary names*/
        aryinst_root=InstanceChild(modinst_root,c);
        arylen=NumberChildren(aryinst_root);
        for (aryc=1;aryc<=arylen;aryc++) {
          aryinst=InstanceChild(aryinst_root,aryc);
          aikind=InstanceKind(aryinst);
          switch (aikind) {
            case MODEL_INST:
            case ARRAY_INT_INST: /* write array names in case any children */
            case ARRAY_ENUM_INST: /* are models */
              rec=ChildName(aryinst_root,aryc);
              Asc_BrowWriteNameRec(&tmps[0],&rec);
              Tcl_AppendResult(interp," {",
                InstanceNameStr(ChildName(modinst_root,c)),&tmps[0],"}",SNULL);
            default: /*write nothing */
              break;
          }
        }
        break;
      default: /* write nothing if its not a model or ary child */
        break;
    }
  }
  return TCL_OK;
}

int Asc_SolvIncompleteSim(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  unsigned long pendings;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,  "call is: slv_checksim <simname>\n");
    Tcl_SetResult(interp, "error in call to slv_checksim", TCL_STATIC);
    return TCL_ERROR;
  }
  g_solvinst_root = Asc_FindSimulationRoot(AddSymbol(argv[1]));
  if (!g_solvinst_root) {
    FPRINTF(ASCERR, "Solve called with NULL root instance.\n");
    Tcl_SetResult(interp, "Simulation specified not found.", TCL_STATIC);
    return TCL_ERROR;
  }
  pendings = NumberPendingInstances(g_solvinst_root);
  if (pendings>0) {
    FPRINTF(ASCERR,"Found %lu pendings.",pendings);
    Tcl_SetResult(interp, "1", TCL_STATIC);
  } else {
    Tcl_SetResult(interp, "0", TCL_STATIC);
  }
  return TCL_OK;
}

int Asc_SolvCheckSys(ClientData cdata, Tcl_Interp *interp,
                  int argc, CONST84 char *argv[])
{
  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argc;     /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if (g_solvsys_cur != NULL) {
    Tcl_SetResult(interp, "1", TCL_STATIC);
  } else {
    Tcl_SetResult(interp, "0", TCL_STATIC);
  }
    return TCL_OK;
}

int Asc_SolvGetObjList(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  int32 *rip=NULL;
  char tmps[MAXIMUM_NUMERIC_LENGTH];
  int i,dev,status;
  FILE *fp;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,  "call is: slv_get_obj_list <out>\n");
    Tcl_SetResult(interp, "slv_get_obj_list wants output device.", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "slv_get_obj_list called with NULL pointer\n");
    Tcl_SetResult(interp, "slv_get_obj_list called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  /* get io option */
  i=3;
  status=Tcl_GetInt(interp,argv[1],&i);
  if (i<0 || i >2) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(ASCERR,"slv_get_obj_list: first arg is 0,1, or 2\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "slv_get_obj_list: invalid output dev #",TCL_STATIC);
    return status;
  } else {
    dev=i;
  }
  switch (dev) {
  case 0: fp=stdout;
    break;
  case 1: fp=ASCERR;
    break;
  case 2: fp=NULL;
    break;
  default : /* should never be here */
    FPRINTF(ASCERR,"slv_get_obj_list called with strange i/o option\n");
    return TCL_ERROR;
  }
  if (slv_obj_select_list(g_solvsys_cur,&rip)) {
    switch (dev) {
    case 0:
    case 1:
      FPRINTF(fp,"Objective indices:\n");
      for (i=0;rip[i]>-1;i++) {
        FPRINTF(fp,"%d\n",rip[i]);
      }
      break;
    case 2:
      Tcl_AppendResult(interp,"{",SNULL);
      for (i=0;rip[i]>-1;i++) {
        sprintf(tmps,"%d ",rip[i]);
        Tcl_AppendResult(interp,tmps,SNULL);
      }
      Tcl_AppendResult(interp,"}",SNULL);
      break;
    default:
      FPRINTF(ASCERR,"wierdness in i/o!");
      break;
    }
    ascfree(rip);
  } else {
    Tcl_SetResult(interp, "{}", TCL_STATIC);
  }
  return TCL_OK;
}

int Asc_SolvSetObjByNum(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  int32 i,status,len;
  struct rel_relation **rlist=NULL;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,  "call is: slv_set_obj_by_num <num>\n");
    Tcl_SetResult(interp, "slv_set_obj_by_num wants objective number.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "slv_set_obj_by_num called with NULL pointer\n");
    Tcl_SetResult(interp, "slv_set_obj_by_num called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  /* get io option */
  i=0;
  status=Tcl_GetInt(interp,argv[1],&i);
  len = slv_get_num_solvers_objs(g_solvsys_cur);

  if (i == -1) { /* remove objective and return */
    slv_set_obj_relation(g_solvsys_cur,NULL);
    return TCL_OK;
  }
  if (i<0 || i >= len) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(ASCERR,"slv_set_obj_by_num: invalid objective number\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "slv_set_obj_by_num: invalid objective number",
                  TCL_STATIC);
    return status;
  } else {
    rlist = slv_get_solvers_obj_list(g_solvsys_cur);
    slv_set_obj_relation(g_solvsys_cur,rlist[i]);
  }
  return TCL_OK;
}

STDHLF(Asc_SolvGetObjNumCmd,(Asc_SolvGetObjNumCmdHL,HLFSTOP));
int Asc_SolvGetObjNumCmd(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  char tmps[MAXIMUM_NUMERIC_LENGTH];
  int num,i,dev,status;
  FILE *fp;

  ASCUSE;  /* see if first arg is -help */
  
  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,  "call is: slv_get_obj_num <out>\n");
    Tcl_SetResult(interp, "slv_get_obj_num wants output device.", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "slv_get_obj_num called with NULL pointer\n");
    Tcl_SetResult(interp, "slv_get_obj_num called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  /* get io option */
  i=3;
  status=Tcl_GetInt(interp,argv[1],&i);
  if (i<0 || i >2) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(ASCERR,"slv_get_obj_num: first arg is 0,1, or 2\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "slv_get_obj_num: invalid output dev #",TCL_STATIC);
    return status;
  } else {
    dev=i;
  }
  switch (dev) {
  case 0: fp=stdout;
    break;
  case 1: fp=ASCERR;
    break;
  case 2: fp=NULL;
    break;
  default : /* should never be here */
    FPRINTF(ASCERR,"slv_get_obj_num called with strange i/o option\n");
    return TCL_ERROR;
  }
  num = slv_get_obj_num(g_solvsys_cur);
  switch (dev) {
  case 0:
  case 1:
    FPRINTF(fp,"Objective index: ");
    FPRINTF(fp,"%d\n",num);
    break;
  case 2:
    sprintf(tmps,"%d ",num);
    Tcl_AppendResult(interp,tmps,SNULL);
    break;
  default:
    FPRINTF(ASCERR,"wierdness in i/o!");
    break;
  }
  return TCL_OK;
}

int Asc_SolvGetSlvParmsNew(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char *argv[])
{
  slv_parameters_t p;
  char *tmps = NULL;
  int solver;
  int status=TCL_OK;
  int i,j;
  p.num_parms = 0;
  p.parms = NULL;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,  "call is: slv_get_parmsnew <solver number>\n");
    Tcl_SetResult(interp, "error in call to slv_get_parmsnew", TCL_STATIC);
    return TCL_ERROR;
  }
  status=Tcl_GetInt(interp, argv[1], &solver);
  if ((solver<0) || (solver>=slv_number_of_solvers) || (status==TCL_ERROR)) {
    FPRINTF(ASCERR,  "slv_get_parmsnew: solver unknown!\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "slv_get_parmsnew: solver number unknown",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  slv_get_default_parameters(solver,&p);
  tmps= (char *)ascmalloc((MAXIMUM_NUMERIC_LENGTH+1)*sizeof(char));

  for (i = 0; i < p.num_parms; i++) {
    Tcl_AppendElement(interp,"New_Parm");
    switch (p.parms[i].type) {
    case int_parm:
      Tcl_AppendElement(interp,"int_parm");
      break;
    case bool_parm:
      Tcl_AppendElement(interp,"bool_parm");
      break;
    case real_parm:
      Tcl_AppendElement(interp,"real_parm");
      break;
    case char_parm:
      Tcl_AppendElement(interp,"char_parm");
      break;
    default:
      Tcl_AppendElement(interp,"error");
      continue;
    }

    Tcl_AppendElement(interp,p.parms[i].name);
    Tcl_AppendElement(interp,p.parms[i].interface_label);
    
    switch (p.parms[i].type) {
    case int_parm:
      sprintf(tmps,"%d",p.parms[i].info.i.value);
      Tcl_AppendElement(interp,tmps);
      sprintf(tmps,"%d",p.parms[i].info.i.high);
      Tcl_AppendElement(interp,tmps);
      sprintf(tmps,"%d",p.parms[i].info.i.low);
      Tcl_AppendElement(interp,tmps);
      break;
    case bool_parm:
      sprintf(tmps,"%d",p.parms[i].info.b.value);
      Tcl_AppendElement(interp,tmps);
      sprintf(tmps,"%d",p.parms[i].info.b.high);
      Tcl_AppendElement(interp,tmps);
      sprintf(tmps,"%d",p.parms[i].info.b.low);
      Tcl_AppendElement(interp,tmps);
      break;
    case real_parm:
      sprintf(tmps,"%.6e",p.parms[i].info.r.value);
      Tcl_AppendElement(interp,tmps);
      sprintf(tmps,"%.6e",p.parms[i].info.r.high);
      Tcl_AppendElement(interp,tmps);
      sprintf(tmps,"%.6e",p.parms[i].info.r.low);
      Tcl_AppendElement(interp,tmps);
      break;
    case char_parm:
      Tcl_AppendElement(interp,p.parms[i].info.c.value);
      sprintf(tmps,"%d",p.parms[i].info.c.high);
      Tcl_AppendElement(interp,tmps);
      for (j = 0; j < p.parms[i].info.c.high; j++) {
        Tcl_AppendElement(interp,p.parms[i].info.c.argv[j]);
      }
      break;
    default:
      FPRINTF(ASCERR,  "slv_get_parmsnew found unrecognized");
      FPRINTF(ASCERR,  " parameter type\n");
      break;
    }
    sprintf(tmps,"%d",p.parms[i].display);
    Tcl_AppendElement(interp,tmps);
    Tcl_AppendElement(interp,p.parms[i].description);
  }
  slv_destroy_parms(&p);
  ascfree(tmps);
  return TCL_OK;
}


int Asc_SolvSetSlvParmsNew(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  slv_parameters_t p;
  int tmp_int =0, solver,i,j;
  double tmp_double = 0.1;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "set_slv_parms called with NULL pointer\n");
    Tcl_SetResult(interp,"set_slv_parms called without slv_system",TCL_STATIC);
    return TCL_ERROR;
  }

  solver=0;
  if (Tcl_GetInt(interp,argv[1],&solver)==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "set_slv_parms: arg 1 invalid type", TCL_STATIC);
    return TCL_ERROR;
  }
  Tcl_ResetResult(interp);
  i = slv_get_selected_solver(g_solvsys_cur);

  if ( solver != i ) {
    /* THIS WHOLE CONTROL STRUCTURE IS SCREWED UP AT BOTH THE
     * C AND THE TCL LEVEL!!! 
     */
    slv_select_solver(g_solvsys_cur,solver);
/*    FPRINTF(ASCERR,"Warning: Solv_Set_Slv_Parms called ");
 *  FPRINTF(ASCERR,"with solver other than current solver\n");
 *  return TCL_OK;
 */
  }
  slv_get_parameters(g_solvsys_cur,&p);

  if ((argc - 2) != (p.num_parms)) {
    /* calling function in slot 0 and solver number in slot 1 */
    Tcl_SetResult(interp, "set_slv_parms called with wrong number of args.",
                  TCL_STATIC);
    FPRINTF(ASCERR,
      "set_slv_parms expected %d args for %s\n",(p.num_parms + 1),
       slv_solver_name(p.whose));
    FPRINTF(ASCERR, "actual argument count: %d\n", (argc - 1));
    FPRINTF(ASCERR, "expected argument count: %d\n", (p.num_parms + 1));
    return TCL_ERROR;
  }

  for (j = 2,i = 0; i < p.num_parms; j++,i++) {
    switch (p.parms[i].type) {
    case int_parm:
      if (Tcl_GetInt(interp,argv[j],&tmp_int)==TCL_ERROR) {
        Tcl_ResetResult(interp);
        FPRINTF(ASCERR,"set_slv_parms: arg %d of invalid type",j);
        Tcl_SetResult(interp, "set_slv_parms called with invalid type",
                      TCL_STATIC);
        return TCL_ERROR;
      }
      p.parms[i].info.i.value = tmp_int;
      break;

    case bool_parm:
      if (Tcl_GetInt(interp,argv[j],&tmp_int)==TCL_ERROR) {
        Tcl_ResetResult(interp);
        FPRINTF(ASCERR,"set_slv_parms: arg %d of invalid type",j);
        Tcl_SetResult(interp, "set_slv_parms called with invalid type",
                      TCL_STATIC);
        return TCL_ERROR;
      }
      p.parms[i].info.b.value = tmp_int;
      break;

    case real_parm:
      if (Tcl_GetDouble(interp,argv[j],&tmp_double)==TCL_ERROR) {
        Tcl_ResetResult(interp);
        FPRINTF(ASCERR,"set_slv_parms: arg %d of invalid type",j);
        Tcl_SetResult(interp, "set_slv_parms called with invalid type",
                      TCL_STATIC);
        return TCL_ERROR;
      }
      p.parms[i].info.r.value = tmp_double;
      break;

    case char_parm:
      slv_set_char_parameter(&(p.parms[i].info.c.value),(char *)argv[j]);
      break;
    default:
      FPRINTF(ASCERR,  "slv_get_parmsnew found unrecognized");
      FPRINTF(ASCERR,  " parameter type\n");
    }
  }
  slv_set_parameters(g_solvsys_cur,&p);
  return TCL_OK;
}


/* NBP is the number of basic parameters in the slv_parameters_t plus 1
   that we mess with in Asc_SolvGetSlvParms, Asc_SolvSetSlvParms.
   If you add a parameter to this that is handled here, up NBP */
#undef NBP
#define NBP 15
int Asc_SolvGetSlvParms(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  slv_parameters_t p;
  char *tmps = NULL;
  int cursolver;
  int solver;
  int status=TCL_OK;
  int i,n;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,  "call is: slv_get_parms <solver number>\n");
    Tcl_SetResult(interp, "error in call to slv_get_parms", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "slv_get_parms called with NULL pointer\n");
    Tcl_SetResult(interp,"slv_get_parms called without slv_system",TCL_STATIC);
    return TCL_ERROR;
  }
  status=Tcl_GetInt(interp, argv[1], &solver);
  /* following assumes solvers are numbered 0-n with no gaps */
  if ((solver<0) || (solver>=slv_number_of_solvers) || (status==TCL_ERROR)) {
    FPRINTF(ASCERR,  "slv_get_parms: solver unknown!\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "slv_get_parms: solver number unknown", TCL_STATIC);
    return TCL_ERROR;
  }

  /* get parameters for solver*/
  cursolver=slv_get_selected_solver(g_solvsys_cur);
  slv_select_solver(g_solvsys_cur,solver);
  slv_get_parameters(g_solvsys_cur,&p);
  tmps= (char *)ascmalloc((MAXIMUM_NUMERIC_LENGTH+1)*sizeof(char));

  sprintf(tmps,"%d", p.whose);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%.16g", p.time_limit);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%d", p.iteration_limit);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%.16g", p.tolerance.termination);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%.16g", p.tolerance.feasible);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%.16g", p.tolerance.pivot);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%.16g", p.tolerance.singular);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%.16g", p.tolerance.stationary);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%.16g", p.rho);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%s", ONEORZERO(p.partition));
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%s", ONEORZERO(p.ignore_bounds));
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%s", ONEORZERO(p.output.more_important!= NULL));
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%s", ONEORZERO(p.output.less_important!= NULL));
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%d", p.factor_option);
  Tcl_AppendElement(interp,tmps);

  if (p.sp.iap) {
    n = p.sp.ilen;
  } else {
    n = 0;
  }
  for (i=0;i<n;i++) {
      sprintf(tmps,"%d",p.sp.iap[i]);
      Tcl_AppendElement(interp,tmps);
  }
  if (p.sp.rap) {
    n = p.sp.rlen;
  } else {
    n = 0;
  }
  for (i=0;i<n;i++) {
      sprintf(tmps,"%.16g",p.sp.rap[i]);
      Tcl_AppendElement(interp,tmps);
  }
  if (p.sp.cap) {
    n = p.sp.clen;
  } else {
    n = 0;
  }
  for (i=0;i<n;i++) {
      Tcl_AppendElement(interp,p.sp.cap[i]);
  }
  ascfree(tmps);
  slv_select_solver(g_solvsys_cur,cursolver);
  return TCL_OK;
}

int Asc_SolvSetSlvParms(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  slv_parameters_t p;
  int tmpbool =0, solver,i,nia,nra;

  int nca = 0;   /*  modified by CWS 5/95 -
                     have one character subparameter too */

  int32 tmplong =100;
  double tmpdouble = 0.1;
  char *tmpchar;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "set_slv_parms called with NULL pointer\n");
    Tcl_SetResult(interp,"set_slv_parms called without slv_system",TCL_STATIC);
    return TCL_ERROR;
  }

  if (argc < NBP) {
    FPRINTF(ASCERR,  "call is: set_slv_parms <%d args>\n",NBP-1);
    FPRINTF(ASCERR,  "args are:\n");
    FPRINTF(ASCERR,  "solver number\n");
    FPRINTF(ASCERR,  "time_limit(sec)\n");
    FPRINTF(ASCERR,  "iteration_limit\n");

    FPRINTF(ASCERR,  "termination tolerance\n");
    FPRINTF(ASCERR,  "feasible tolerance\n");
    FPRINTF(ASCERR,  "pivot tolerance\n");
    FPRINTF(ASCERR,  "singular tolerance\n");
    FPRINTF(ASCERR,  "stationary tolerance\n");
    FPRINTF(ASCERR,  "rho\n");

    FPRINTF(ASCERR,  "partitioning enabled\n");
    FPRINTF(ASCERR,  "ignore bounds\n");
    FPRINTF(ASCERR,  "display more important messages\n");
    FPRINTF(ASCERR,  "display less important messages\n");
    FPRINTF(ASCERR,  "factor_option number\n");
    FPRINTF(ASCERR,  "plus engine specific int and real parms\n");

    FFLUSH(ASCERR);
    Tcl_SetResult(interp, "in set_slv_parms call", TCL_STATIC);
    return TCL_ERROR;
  }
  solver=0;
  if (Tcl_GetInt(interp,argv[1],&solver)==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "set_slv_parms: arg 1 invalid type", TCL_STATIC);
    return TCL_ERROR;
  }
  Tcl_ResetResult(interp);
  i=slv_get_selected_solver(g_solvsys_cur);

  if ( solver != i ) {
    /* THIS WHOLE CONTROL STRUCTURE IS SCREWED UP AT BOTH THE
       C AND THE TCL LEVEL!!! */
    slv_select_solver(g_solvsys_cur,solver);
/*    FPRINTF(ASCERR,"Warning: Solv_Set_Slv_Parms called ");
    FPRINTF(ASCERR,"with solver other than current solver\n");
    return TCL_OK;*/
  }
  slv_get_parameters(g_solvsys_cur,&p);

/*  if (p.whose!=solver) return TCL_OK; *//* fail quietly, user is an idiot */

  /* determine number of total parameters we need from user */
  if (p.sp.iap) {
    nia = p.sp.ilen;
  } else {
    nia = 0;
  }
  if (p.sp.rap) {
    nra = p.sp.rlen;
  } else {
    nra = 0;
  }
  if (p.sp.cap) {
    nca = p.sp.clen;
  } else {
    nca = 0;
  }
  if (argc != (NBP+nia+nra+nca)) { /*args 0 to NBP-1 are the slv0 standard */
    Tcl_SetResult(interp, "set_slv_parms called with wrong number of args.",
                  TCL_STATIC);
    FPRINTF(ASCERR,
      "set_slv_parms expected %d args for %s\n",(NBP -1+nia+nra+nca),
       slv_solver_name(p.whose));
    FPRINTF(ASCERR, "actual argument count: %d\n", argc);
    FPRINTF(ASCERR, "expected argument count: %d\n", NBP+nia+nra+nca);
    FPRINTF(ASCERR, "basic: %d\n", NBP-1);
    FPRINTF(ASCERR, "integer: %d\n", nia);
    FPRINTF(ASCERR, "double: %d\n", nra);
    FPRINTF(ASCERR, "string: %d\n", nca);
    return TCL_ERROR;
  }

  tmpdouble=p.time_limit;
  if( Tcl_GetDouble(interp,argv[2],&tmpdouble)==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "set_slv_parms: arg 2 invalid type", TCL_STATIC);
    return TCL_ERROR;
  }
  p.time_limit=fabs(tmpdouble);

  tmplong=p.iteration_limit;
  if (Tcl_GetInt(interp,argv[3],&tmplong)==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "set_slv_parms: arg 3 invalid type", TCL_STATIC);
    return TCL_ERROR;
  }
  p.iteration_limit = abs(tmplong);

  tmpdouble=p.tolerance.termination;
  if(Tcl_GetDouble(interp,argv[4],&tmpdouble)==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "set_slv_parms: arg 4 invalid type", TCL_STATIC);
    return TCL_ERROR;
  }
  p.tolerance.termination =fabs(tmpdouble);

  tmpdouble=p.tolerance.feasible;
  if (Tcl_GetDouble(interp,argv[5],&tmpdouble)==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "set_slv_parms: arg 5 invalid type", TCL_STATIC);
    return TCL_ERROR;
  }
  p.tolerance.feasible =fabs(tmpdouble);

  tmpdouble=p.tolerance.pivot;
  if (Tcl_GetDouble(interp,argv[6],&tmpdouble)==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "set_slv_parms: arg 6 invalid type", TCL_STATIC);
    return TCL_ERROR;
  }
  p.tolerance.pivot =fabs(tmpdouble);

  tmpdouble=p.tolerance.singular;
  if (Tcl_GetDouble(interp,argv[7],&tmpdouble)==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "set_slv_parms: arg 7 invalid type", TCL_STATIC);
    return TCL_ERROR;
  }
  p.tolerance.singular =fabs(tmpdouble);

  tmpdouble=p.tolerance.stationary;
  if (Tcl_GetDouble(interp,argv[8],&tmpdouble)==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "set_slv_parms: arg 8 invalid type", TCL_STATIC);
    return TCL_ERROR;
  }
  p.tolerance.stationary =fabs(tmpdouble);

  tmpdouble=p.rho;
  if (Tcl_GetDouble(interp,argv[9],&tmpdouble)==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "set_slv_parms: arg 9 invalid type", TCL_STATIC);
    return TCL_ERROR;
  }
  p.rho =fabs(tmpdouble);

  tmpbool=p.partition;
  if(Tcl_ExprBoolean(interp,argv[10],&tmpbool)==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "set_slv_parms: arg 10 invalid type", TCL_STATIC);
    return TCL_ERROR;
  }
  p.partition=tmpbool;

  tmpbool=p.ignore_bounds;
  if ( Tcl_ExprBoolean(interp,argv[11],&tmpbool)==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "set_slv_parms: arg 11 invalid type", TCL_STATIC);
    return TCL_ERROR;
  }
  p.ignore_bounds=tmpbool;

  if (Tcl_ExprBoolean(interp,argv[12],&tmpbool)==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "set_slv_parms: arg 12 invalid type", TCL_STATIC);
    return TCL_ERROR;
  }
  if (tmpbool) {
    p.output.more_important=ASCERR;
  } else {
    p.output.more_important=NULL;
  }

  if (Tcl_ExprBoolean(interp,argv[13],&tmpbool)==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "set_slv_parms: arg 13 invalid type", TCL_STATIC);
    return TCL_ERROR;
  }
  if (tmpbool) {
    p.output.less_important=ASCERR;
  } else {
    p.output.less_important=NULL;
  }

  tmplong=p.factor_option;
  if (Tcl_GetInt(interp,argv[14],&tmplong)==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "set_slv_parms: arg 14 invalid type", TCL_STATIC);
    return TCL_ERROR;
  }
  p.factor_option = abs(tmplong);

  for (i=0;i<nia;i++) {
    tmpbool=p.sp.iap[i];
    if (Tcl_GetInt(interp,argv[i+NBP],&tmpbool)==TCL_ERROR) {
      Tcl_ResetResult(interp);
      Tcl_SetResult(interp, "set_slv_parms: integer array arg of invalid type",
                    TCL_STATIC);
      FPRINTF(ASCERR,"int sub-parameter %d (%s) invalid\n",i,argv[i+NBP]);
      return TCL_ERROR;
    }
    p.sp.iap[i]=tmpbool;
  }

  for (i=0;i<nra;i++) {
    tmpdouble=p.sp.rap[i];
    if (Tcl_GetDouble(interp,argv[i+NBP+nia],&tmpdouble)==TCL_ERROR) {
      Tcl_ResetResult(interp);
      Tcl_SetResult(interp, "set_slv_parms: real array arg of invalid type",
                    TCL_STATIC);
        FPRINTF(ASCERR,"real sub-parameter %d (%s) invalid\n",
          i,argv[i+nia+NBP]);
      return TCL_ERROR;
    }
    p.sp.rap[i]=tmpdouble;
  }

   /*  modified by CWS 5/95
       Loop through and copy the strings from TCL land
       to the C side of things.  The strings are deallocated
       in slvI_destroy (slv6_destroy in this case).
    */

    for (i=0;i<nca;i++) {
        tmpchar =
          Asc_MakeInitString(strlen(argv[i+NBP+nia+nra])); /* allocate mem */
        strcpy(tmpchar, argv[i+NBP+nia+nra]);  /* make a copy of string */
        if (p.sp.cap[i] != NULL) {
          ascfree(p.sp.cap[i]);
        }
        /* deallocate old, if any */
        p.sp.cap[i] = tmpchar;  /* save pointer */
     }


  slv_set_parameters(g_solvsys_cur,&p);
  return TCL_OK;
}
#undef NBP

int Asc_SolvGetInstType(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  char * it;

  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    FPRINTF(ASCERR,  "call is: slv_get_insttype <no args>\n");
    Tcl_SetResult(interp, "error in call to slv_get_insttype", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
/*    FPRINTF(ASCERR,  "slv_get_insttype called with NULL pointer\n");
*/
    Tcl_SetResult(interp, "slv_get_insttype called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvinst_cur==NULL) {
/*  FPRINTF(ASCERR,  "slv_get_insttype called with NULL instance\n");
*/
    Tcl_SetResult(interp, "slv_get_insttype called without instance",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  it=(char *)InstanceType(g_solvinst_cur);
  Tcl_AppendElement(interp,it);
  return TCL_OK;
}

int Asc_SolvGetSlvStatPage(ClientData cdata, Tcl_Interp *interp,
                         int argc, CONST84 char *argv[])
{
  slv_status_t s;
  char * tmps=NULL;

  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    FPRINTF(ASCERR,  "call is: slv_get_stat_page <no args>\n");
    Tcl_SetResult(interp, "error in call to slv_get_stat_page", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "slv_get_stat_page called with NULL pointer\n");
    Tcl_SetResult(interp, "slv_get_stat_page called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  slv_get_status(g_solvsys_cur,&s);

  tmps= (char *)ascmalloc((MAXIMUM_NUMERIC_LENGTH+1)*sizeof(char));
  /*system status */
  sprintf(tmps,"%d",s.ok);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%d",s.over_defined);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%d",s.under_defined);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%d",s.struct_singular);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%d",s.ready_to_solve);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%d",s.converged);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%d",s.diverged);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%d",s.inconsistent);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%d",s.calc_ok);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%d",s.iteration_limit_exceeded);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%d",s.time_limit_exceeded);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%d",s.iteration);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%.16g",s.cpu_elapsed);
  Tcl_AppendElement(interp,tmps);

  /*block status*/
  sprintf(tmps,"%d",s.block.number_of);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%d",s.block.current_block);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%d",s.block.current_size);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%d",s.block.previous_total_size);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%d",s.block.iteration);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%.10g",s.block.cpu_elapsed);
  Tcl_AppendElement(interp,tmps);
  sprintf(tmps,"%.10g",s.block.residual);
  Tcl_AppendElement(interp,tmps);
  ascfree(tmps);
  return TCL_OK;
}
int Asc_SolvGetSlvCostPage(ClientData cdata, Tcl_Interp *interp,
                         int argc, CONST84 char *argv[])
{
  slv_status_t s;
  int i;

  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    FPRINTF(ASCERR,  "call is: slv_get_cost_page <no args>\n");
    Tcl_SetResult(interp, "error in call to slv_get_cost_page", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "slv_get_cost_page called with NULL pointer\n");
    Tcl_SetResult(interp, "slv_get_cost_page called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  slv_get_status(g_solvsys_cur,&s);

  if (s.cost)  {
    char * tmps=NULL;
    tmps= (char *)ascmalloc((MAXIMUM_NUMERIC_LENGTH+1)*sizeof(char));
    sprintf(tmps,"%s","\0");
    for (i=0;i<s.costsize;i++) {
      if (!i) {
        sprintf(tmps,"{%d ",s.cost[i].size);
      } else {
        sprintf(tmps," {%d ",s.cost[i].size);
      }
      Tcl_AppendResult(interp,tmps,SNULL);
      sprintf(tmps, "%d ",s.cost[i].iterations);
      Tcl_AppendResult(interp,tmps,SNULL);
      sprintf(tmps, "%d ",s.cost[i].funcs);
      Tcl_AppendResult(interp,tmps,SNULL);
      sprintf(tmps, "%d ",s.cost[i].jacs);
      Tcl_AppendResult(interp,tmps,SNULL);
      sprintf(tmps, "%.8g ",s.cost[i].time);
      Tcl_AppendResult(interp,tmps,SNULL);
      sprintf(tmps, "%.16g ",s.cost[i].resid);
      Tcl_AppendResult(interp,tmps,SNULL);
      sprintf(tmps, "%.8g ",s.cost[i].functime);
      Tcl_AppendResult(interp,tmps,SNULL);
      sprintf(tmps, "%.8g}",s.cost[i].jactime);
      Tcl_AppendResult(interp,tmps,SNULL);
    }
    ascfree(tmps);
  }
  return TCL_OK;
}

int Asc_SolvGetObjectiveVal(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  struct rel_relation *obj;

  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    FPRINTF(ASCERR,  "call is: slv_get_objval <no args>\n");
    Tcl_SetResult(interp, "error in call to slv_get_objval", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "slv_get_objval called with NULL pointer\n");
    Tcl_SetResult(interp, "slv_get_objval called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  obj= slv_get_obj_relation(g_solvsys_cur);
  if( obj == NULL ) {
    Tcl_SetResult(interp, "none", TCL_STATIC);
  } else {
    /* expect the solver to have updated the objects list valeus */
    Tcl_AppendResult(interp,Asc_UnitValue(rel_instance(obj)),SNULL);
#if 0
    tmps= (char *)ascmalloc((MAXIMUM_NUMERIC_LENGTH+1)*sizeof(char));
    val = relman_eval(obj,&calc_ok,1);
    sprintf(&tmps[0],"%.16g",val);
    Tcl_AppendElement(interp,&tmps[0]);
    ascfree(tmps);
    /* old code */
    val = exprman_eval(NULL/*bug*/,obj); /* broken */
    if (obj->negate) {
      val=-val;
    }
    /* obj->negate set TRUE by system_build */
    sprintf(&tmps[0],"%.16g",val);
    Tcl_AppendElement(interp,&tmps[0]);
#endif
  }
  return TCL_OK;
}

int Asc_SolvGetInstName(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  char *name=NULL;

  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    FPRINTF(ASCERR,"call is: slv_get_instname\n");
    Tcl_SetResult(interp, "slv_get_instname wants 0 args", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvinst_cur==NULL || g_solvinst_root==NULL) {
#if SP_DEBUG
    FPRINTF(ASCERR,  "slv_get_instname called with NULL pointer\n");
#endif
    Tcl_SetResult(interp, "none", TCL_STATIC);
    return TCL_OK;
  }
  if (g_solvinst_cur==g_solvinst_root) {
    Tcl_SetResult(interp, "&", TCL_STATIC);
    return TCL_OK;
  }
  name=WriteInstanceNameString(g_solvinst_cur,g_solvinst_root);
  Tcl_AppendResult(interp,name,SNULL);
  if (name) {
    ascfree(name);
  }
  return TCL_OK;
}

int Asc_SolvGetPathName(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  char *name=NULL;

  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    FPRINTF(ASCERR,"call is: slv_get_pathname\n");
    Tcl_SetResult(interp, "slv_get_pathname wants 0 args", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvinst_cur==NULL || g_solvinst_root==NULL) {
#if SP_DEBUG
    FPRINTF(ASCERR,  "slv_get_pathname called with NULL pointer\n");
#endif
    Tcl_SetResult(interp, "none", TCL_STATIC);
    return TCL_OK;
  }
  name = (char *)SCP(Asc_SimsFindSimulationName(g_solvinst_root));
  Tcl_AppendResult(interp,name,SNULL);
  name=NULL;
  if (g_solvinst_cur!=g_solvinst_root) {
    name=WriteInstanceNameString(g_solvinst_cur,g_solvinst_root);
    Tcl_AppendResult(interp,".",name,SNULL);
    if (name) {
      ascfree(name);
    }
  }
  return TCL_OK;
}

#if DELETEME
int Asc_Sims2Solve(ClientData cdata, Tcl_Interp *interp,
               int argc, CONST84 char *argv[])
{
  enum inst_t ikind;
  unsigned long pc;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,  "call is: slv_import_sim <simname>\n");
    Tcl_SetResult(interp, "slv_import_sim takes a simulation name arg.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  g_solvinst_root = Asc_FindSimulationRoot(argv[1]);
  if (!g_solvinst_root) {
    FPRINTF(ASCERR, "NULL simulation found by slv_import_sim.\n");
    Tcl_SetResult(interp, "Simulation specified not found.", TCL_STATIC);
    return TCL_ERROR;
  }
  g_solvinst_cur = g_solvinst_root;

  /* check that instance is model this shouldn't be possible.*/
  ikind=InstanceKind(g_solvinst_cur);
  if (ikind!=MODEL_INST) {
     FPRINTF(ASCERR,  "Instance imported is not a solvable kind.\n");
     Tcl_SetResult(interp, "Simulation kind not MODEL.", TCL_STATIC);
    return TCL_ERROR;
  }

  /* check instance is complete */
  if ((pc=NumberPendingInstances(g_solvinst_cur))!=0) {
    FPRINTF(ASCERR,  "Simulation imported is incomplete: %ld pendings.\n",pc);
    Tcl_SetResult(interp, "Simulation has pendings: Not imported.",TCL_STATIC);
    return TCL_ERROR;
  }
  /* flush old system */
  if (g_solvsys_cur != NULL) {
    slv_system_t systmp=g_solvsys_cur;
    system_destroy(systmp);
    g_solvsys_cur = NULL;
  }

  /* create system */
  if( g_solvsys_cur == NULL ) {
    g_solvsys_cur = system_build(g_solvinst_cur);
    if( g_solvsys_cur == NULL ) {
      FPRINTF(ASCERR,"system_build returned NULL.\n");
      Tcl_SetResult(interp, "Bad relations found: solve system not created.",
                    TCL_STATIC);
      return TCL_ERROR;
    }
    FPRINTF(ASCERR,"Presolving . . .\n");
    if (setjmp(g_fpe_env)==0) {
      slv_presolve(g_solvsys_cur);
    } else {
      FPRINTF(ASCERR, "Floating point exception in slv_presolve!!\n");
      Tcl_SetResult(interp, " Floating point exception in slv_presolve. Help!",
                    TCL_STATIC);
      return TCL_ERROR;
    }
    FPRINTF(ASCERR,"Presolving done.\n");
  }
  if( g_solvsys_cur == NULL ) {
    FPRINTF(ASCERR,"system_build returned NULL!\n");
    Tcl_SetResult(interp, "C error Asc_Sims2Solve: solve system not created.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  Tcl_SetResult(interp, "Solver instance created.", TCL_STATIC);
  return TCL_OK;
}

int Asc_Brow2Solve(ClientData cdata, Tcl_Interp *interp,
               int argc, CONST84 char *argv[])
{
  enum inst_t ikind;
  slv_system_t systmp;
  unsigned long pc;

  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    FPRINTF(ASCERR,  "call is: bexp_s <no args>\n");
    Tcl_SetResult(interp,"bexp_s takes current browser focus, no args allowed",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (!g_root) {
    FPRINTF(ASCERR,  "bexp_s:called without simulation in browser.\n");
    Tcl_SetResult(interp, "focus browser before calling bexp_s", TCL_STATIC);
    return TCL_ERROR;
  }
  /* check that instance is model */
  ikind=InstanceKind(g_curinst);
  if (ikind!=MODEL_INST) {
     FPRINTF(ASCERR,  "Instance exported is not a solvable kind.\n");
     Tcl_SetResult(interp, "Instance kind not MODEL.", TCL_STATIC);
    return TCL_ERROR;
  }

  /* check instance is complete */
  if ((pc=NumberPendingInstances(g_curinst))!=0) {
    FPRINTF(ASCERR,  "Instance exported is incomplete: %ld pendings.\n",pc);
    Tcl_SetResult(interp, "Instance has pendings: Not exported.", TCL_STATIC);
    return TCL_ERROR;
  }

  /* flush old system */
  if (g_solvsys_cur != NULL) {
    systmp=g_solvsys_cur;
    system_destroy(systmp);
    g_solvsys_cur = NULL;
  }

  /* copy browser instance tree and focus */
  g_solvinst_root=g_root;
  g_solvinst_cur=g_curinst;
  /* create system */
  if( g_solvsys_cur == NULL ) {
    g_solvsys_cur = system_build(g_solvinst_cur);
    if( g_solvsys_cur == NULL ) {
      FPRINTF(ASCERR,"system_build returned NULL.\n");
      Tcl_SetResult(interp, "Bad relations found: solve system not created.",
                    TCL_STATIC);
      return TCL_ERROR;
    }
    FPRINTF(ASCERR,"Presolving . . .\n");
    if (setjmp(g_fpe_env)==0) {
      slv_presolve(g_solvsys_cur);
    } else {
        FPRINTF(ASCERR, "Floating point exception in slv_presolve!!\n");
        Tcl_SetResult(interp,
                      " Floating point exception in slv_presolve. Help!",
                      TCL_STATIC);
        return TCL_ERROR;
    }
    FPRINTF(ASCERR,"Presolving done.\n");
  }
  if( g_solvsys_cur == NULL ) {
    FPRINTF(ASCERR,"system_build returned NULL!\n");
    Tcl_SetResult(interp, "C error Asc_Brow2Solve: solve system not created.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  Tcl_SetResult(interp, "Solver instance created.", TCL_STATIC);
  return TCL_OK;
}

int Asc_SolvSimInst(ClientData cdata, Tcl_Interp *interp,
                  int argc, CONST84 char *argv[])
{
  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,  "call is: ssolve <simname> \n");
    Tcl_SetResult(interp, "solvers available in Solve> 0:SLV, 1:MINOS",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  g_solvinst_root = Asc_FindSimulationRoot(argv[1]);
  if (!g_solvinst_root) {
    FPRINTF(ASCERR, "Solve called with NULL root instance.\n");
    Tcl_SetResult(interp, "Simulation specified not found.", TCL_STATIC);
    return TCL_ERROR;
  }
  g_solvinst_cur = g_solvinst_root;

  FPRINTF(ASCERR,"Windows will not update until you leave Solve>.\n");
  Solve(g_solvinst_cur);
  return TCL_OK;
}

#endif /* DELETEME */

/*
 *  Solves g_curinst with solver specified.
 *  This is for commandline use only.
 *  Just a wrapper of slv_interface.c Solve() for now.
 *  no proper type checking yet, sincle solve will trap it (usually)
 *  though there should be by 1-14-94
 */
int Asc_SolvCurInst(ClientData cdata, Tcl_Interp *interp,
                  int argc, CONST84 char *argv[])
{
  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,  "call is: solve\n");
    Tcl_SetResult(interp, "solvers available: 0:SLV, 1:MINOS", TCL_STATIC);
    return TCL_ERROR;
  }
  if (!g_curinst) {
    FPRINTF(ASCERR, "Solve called with NULL current instance.\n");
    Tcl_SetResult(interp, "NULL pointer received from Browser.", TCL_STATIC);
    return TCL_ERROR;
  }
  g_solvinst_cur=g_curinst;
  FPRINTF(ASCERR,"Windows will not update until you leave Solve>.\n");
  Solve(g_solvinst_cur);
  return TCL_OK;
}

int Asc_SolvGetVRCounts(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  int solver;
  int status=TCL_OK;
  char * tmps=NULL;
  int tmpi;
  var_filter_t vfilter;
  rel_filter_t rfilter;

  (void)cdata;    /* stop gcc whine about unused parameter */

  tmps= (char *)ascmalloc((MAXIMUM_NUMERIC_LENGTH+1)*sizeof(char));

  if ( argc != 2 ) {
    FPRINTF(ASCERR,  "call is: solve_get_vr <solver number> \n");
    Tcl_SetResult(interp, "call is: solve_get_vr <solver number>", TCL_STATIC);
    return TCL_ERROR;
  }
  status=Tcl_GetInt(interp, argv[1], &solver);
  if (status!=TCL_OK) {
    FPRINTF(ASCERR, "solve_get_vr called with bad solver number.\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "solve_get_vr called with bad solver number.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if ((solver < 0) || (solver >= slv_number_of_solvers)) {
    FPRINTF(ASCERR, "unknown solver (%d). Not selected!\n",solver);
    Tcl_SetResult(interp, "Solver not available.", TCL_STATIC);
    return TCL_ERROR;
  }
  if (!g_solvsys_cur) {
    FPRINTF(ASCERR, "solve_get_vr called with NULL system.\n");
    Tcl_SetResult(interp, "solve_get_vr: called with NULL system.",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  /*get total relation count   totrels */
  tmpi = slv_get_num_solvers_rels(g_solvsys_cur);
  sprintf(tmps,"%d",tmpi);
  Tcl_AppendElement(interp,tmps);

  /*get active relation count   rels */
  rfilter.matchbits = (REL_ACTIVE);
  rfilter.matchvalue = (REL_ACTIVE);
  tmpi=slv_count_solvers_rels(g_solvsys_cur,&rfilter);
  sprintf(tmps,"%d",tmpi);
  Tcl_AppendElement(interp,tmps);

  /*get included relation count   inc_rels */
  rfilter.matchbits = (REL_INCLUDED);
  rfilter.matchvalue = (REL_INCLUDED);
  tmpi=slv_count_solvers_rels(g_solvsys_cur,&rfilter);
  sprintf(tmps,"%d",tmpi);
  Tcl_AppendElement(interp,tmps);

  /*get total variable count  totvars */
  tmpi = slv_get_num_solvers_vars(g_solvsys_cur);
  sprintf(tmps,"%d",tmpi);
  Tcl_AppendElement(interp,tmps);

  /*get active variable count   vars*/
  vfilter.matchbits = (VAR_ACTIVE);
  vfilter.matchvalue = (VAR_ACTIVE);
  tmpi=slv_count_solvers_vars(g_solvsys_cur,&vfilter);
  sprintf(tmps,"%d",tmpi);
  Tcl_AppendElement(interp,tmps);

  /*get currently used (free & incident & active) variable count free_vars*/
  vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_ACTIVE);
  tmpi=slv_count_solvers_vars(g_solvsys_cur,&vfilter);
  sprintf(tmps,"%d",tmpi);
  Tcl_AppendElement(interp,tmps);

  /*get active equality  count   eqals*/
  rfilter.matchbits = (REL_EQUALITY | REL_ACTIVE);
  rfilter.matchvalue = (REL_EQUALITY | REL_ACTIVE);
  tmpi=slv_count_solvers_rels(g_solvsys_cur,&rfilter);
  sprintf(tmps,"%d",tmpi);
  Tcl_AppendElement(interp,tmps);

  /*get used (included and active equalities) relation count    inc_eqals*/
  rfilter.matchbits = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  rfilter.matchvalue = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  tmpi=slv_count_solvers_rels(g_solvsys_cur,&rfilter);
  sprintf(tmps,"%d",tmpi);
  Tcl_AppendElement(interp,tmps);

  /*get inequality count   ineqals*/
  rfilter.matchbits = (REL_EQUALITY | REL_ACTIVE);
  rfilter.matchvalue = (REL_ACTIVE);
  tmpi = slv_count_solvers_rels(g_solvsys_cur,&rfilter);
  sprintf(tmps,"%d",tmpi);
  Tcl_AppendElement(interp,tmps);

  /*get included inequality count  inc_ineqals*/
  rfilter.matchbits = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  rfilter.matchvalue = (REL_INCLUDED | REL_ACTIVE);
  tmpi = slv_count_solvers_rels(g_solvsys_cur,&rfilter);
  sprintf(tmps,"%d",tmpi);
  Tcl_AppendElement(interp,tmps);

  /* get unused (included and inactive equalities) relation count
   * in_inc_eqals
   */
  rfilter.matchbits = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  rfilter.matchvalue = (REL_INCLUDED | REL_EQUALITY);
  tmpi=slv_count_solvers_rels(g_solvsys_cur,&rfilter);
  sprintf(tmps,"%d",tmpi);
  Tcl_AppendElement(interp,tmps);

  /*get included inactive inequality count  in_inc_ineqals*/
  rfilter.matchbits = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  rfilter.matchvalue = (REL_INCLUDED);
  tmpi = slv_count_solvers_rels(g_solvsys_cur,&rfilter);
  sprintf(tmps,"%d",tmpi);
  Tcl_AppendElement(interp,tmps);

  /*get unincluded relation count   uninc_rels */
  rfilter.matchbits = (REL_INCLUDED);
  rfilter.matchvalue = 0;
  tmpi=slv_count_solvers_rels(g_solvsys_cur,&rfilter);
  sprintf(tmps,"%d",tmpi);
  Tcl_AppendElement(interp,tmps);

  /*get fixed and incident count   fixed_vars*/
  vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_FIXED | VAR_INCIDENT | VAR_ACTIVE);
  tmpi=slv_count_solvers_vars(g_solvsys_cur,&vfilter);
  sprintf(tmps,"%d",tmpi);
  Tcl_AppendElement(interp,tmps);

  /*get free and inactive incident count   in_free_vars*/
  vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INCIDENT);
  tmpi=slv_count_solvers_vars(g_solvsys_cur,&vfilter);
  sprintf(tmps,"%d",tmpi);
  Tcl_AppendElement(interp,tmps);

  /*get fixed and inactive incident count   in_fixed_vars*/
  vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_FIXED | VAR_INCIDENT);
  tmpi=slv_count_solvers_vars(g_solvsys_cur,&vfilter);
  sprintf(tmps,"%d",tmpi);
  Tcl_AppendElement(interp,tmps);

  /*get active unattached count   un_vars */
  vfilter.matchbits = (VAR_ACTIVE);
  vfilter.matchvalue = (VAR_ACTIVE);
  tmpi = slv_count_solvers_unattached(g_solvsys_cur,&vfilter);
  sprintf(tmps,"%d",tmpi);
  Tcl_AppendElement(interp,tmps);

  ascfree(tmps);
  return TCL_OK;
}

int Asc_SolvSlvDumpInt(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  int status,level;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR, "call is: slvdump <level>\n");
    Tcl_SetResult(interp, "Specify a level to slvdump.", TCL_STATIC);
    return TCL_ERROR;
  }
  status=Tcl_GetInt(interp,argv[1],&level);
  if (status!=TCL_OK) {
    FPRINTF(ASCERR, "slvdump called with non-integer level.\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "slvdump called with non-integer level.",TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur!=NULL) {
    slv_dump_internals(g_solvsys_cur,level);
  } else {
    FPRINTF(ASCERR, "slvdump called with NULL system.\n");
    Tcl_SetResult(interp, "Empty solver context.", TCL_STATIC);
    return TCL_ERROR;
  }
  return TCL_OK;
}


int Asc_SolvSlvPresolve(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    FPRINTF(ASCERR, "call is: presolve <no args>\n");
    Tcl_SetResult(interp, "no arguments allowed for presolve", TCL_STATIC);
    return TCL_ERROR;
  }

  if (setjmp(g_fpe_env)==0) {
    if (g_solvsys_cur!=NULL) {
      slv_presolve(g_solvsys_cur);
      return TCL_OK;
    } else {
      FPRINTF(ASCERR, "Presolve called with NULL system.\n");
      Tcl_SetResult(interp, "empty solver context.", TCL_STATIC);
      return TCL_ERROR;
    }
  } else {
      FPRINTF(ASCERR, "Floating point exception in slv_presolve!!\n");
      Tcl_SetResult(interp, " Floating point exception in slv_presolve. Help!",
                    TCL_STATIC);
      return TCL_ERROR;
  }
}

/* After modification of an instance included in a when var list or
 * after running a procedure, the system must be reconfigured to
 * account for structural changes in the configuration.
 * Asc_SolvReanalyze has to be executed after running a procedure.
 */
int Asc_SolvReanalyze(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{
  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    FPRINTF(ASCERR, "call is: slv_reanalyze <no args>\n");
    Tcl_SetResult(interp, "wong # arguments for slv_reanalyze", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur!=NULL) {
    system_reanalyze(g_solvsys_cur,NULL);
    return TCL_OK;
  } else {
    FPRINTF(ASCERR, "Reanalyze called with NULL system.\n");
    Tcl_SetResult(interp, "empty solver context.", TCL_STATIC);
    return TCL_ERROR;
  }
}

/*
 * This function needs to be fixed. Right now it does the same as
 * Asc_SolvReanalyze. Here, we are supposed to check if the boolean
 * instance modified is part of some whenvarlist, in the current
 * solver system. The instance to be checked is going to be sent
 * as the second argument to system_reanalyze.
 */
int Asc_SolvCheckAndReanalyze(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char *argv[])
{
  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR, "call is: slv_check_and_reanalyze <instance_name>\n");
    Tcl_SetResult(interp, "wong # arguments for slv_check_and_reanalyze",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur!=NULL) {
    system_reanalyze(g_solvsys_cur,NULL);
    return TCL_OK;
  } else {
    FPRINTF(ASCERR, "CheckAndReanalyze called with NULL system.\n");
    Tcl_SetResult(interp, "empty solver context.", TCL_STATIC);
    return TCL_ERROR;
  }
}

int Asc_SolvSlvResolve(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    FPRINTF(ASCERR, "call is: resolve <no args>\n");
    Tcl_SetResult(interp, "no arguments allowed for resolve", TCL_STATIC);
    return TCL_ERROR;
  }

  if (setjmp(g_fpe_env)==0) {
    if (g_solvsys_cur!=NULL) {
      slv_resolve(g_solvsys_cur);
      return TCL_OK;
    } else {
      FPRINTF(ASCERR, "Resolve called with NULL system.\n");
      Tcl_SetResult(interp, "empty solver context.", TCL_STATIC);
      return TCL_ERROR;
    }
  } else {
      FPRINTF(ASCERR, "Floating point exception in slv_resolve!!\n");
      Tcl_SetResult(interp, " Floating point exception in slv_resolve. Help!",
                    TCL_STATIC);
      return TCL_ERROR;
  }
}

/* invoking the name of the beast three times makes it come! */
int Asc_SolvSlvSolve(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{
  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    FPRINTF(ASCERR, "call is: slv_solve <no args>\n");
    Tcl_SetResult(interp, "no arguments allowed for slv_solve", TCL_STATIC);
    return TCL_ERROR;
  }
  if (setjmp(g_fpe_env)==0) {
    if (g_solvsys_cur!=NULL) {
      slv_solve(g_solvsys_cur);
      return TCL_OK;
    } else {
      FPRINTF(ASCERR, "slv_solve called with NULL system.\n");
      Tcl_SetResult(interp, " empty solver context.", TCL_STATIC);
      return TCL_ERROR;
    }
  } else {
      FPRINTF(ASCERR, "Floating point exception in slv_solve!!\n");
      Tcl_SetResult(interp, " Floating point exception in slv_solve. Help!",
                    TCL_STATIC);
      return TCL_ERROR;
  }
}

/* hide it out here from the exception clobber */
static int safe_status;
int Asc_SolvSlvIterate(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  slv_status_t s;
  int steps=1;
  double time=5.0,start,delta=0.0;
  safe_status=TCL_OK;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc > 3 ) {
    FPRINTF(ASCERR, "call is: slv_iterate <steps> [timelimit]\n");
    Tcl_SetResult(interp, "too many arguments to slv_iterate", TCL_STATIC);
    return TCL_ERROR;
  }
  if ( argc < 2 ) {
    FPRINTF(ASCERR, "call is: slv_iterate <steps> [timelimit]\n");
    Tcl_SetResult(interp, "need an iteration count for slv_iterate",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  safe_status=Tcl_GetInt(interp,argv[1],&steps);
  if (safe_status!=TCL_OK || steps <1) {
      FPRINTF(ASCERR, "slv_iterate called with bad step count.\n");
      Tcl_ResetResult(interp);
      Tcl_SetResult(interp, "slv_iterate called with bad step count.",
                    TCL_STATIC);
      return safe_status;
  }
  if ( argc == 3 ) {
    safe_status=Tcl_GetDouble(interp,argv[2],&time);
    if (safe_status!=TCL_OK || time <0.1) {
        FPRINTF(ASCERR, "slv_iterate called with bad time limit.\n");
        Tcl_ResetResult(interp);
        Tcl_SetResult(interp, "slv_iterate called with bad time limit.",
                      TCL_STATIC);
        return safe_status;
    }
  }
  Tcl_ResetResult(interp);
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR, "slv_iterate called with NULL system.\n");
    Tcl_SetResult(interp, " empty solver context.", TCL_STATIC);
    return TCL_ERROR;
  }

  start=tm_cpu_time();
  for (safe_status=0;safe_status<steps && delta <time;safe_status++) {
    if (setjmp(g_fpe_env)==0) {
      slv_get_status(g_solvsys_cur,&s);
      if (s.ready_to_solve && !Solv_C_CheckHalt_Flag) {
        slv_iterate(g_solvsys_cur);
      }
    } else {
      FPRINTF(ASCERR, "Floating point exception in slv_iterate!!\n");
      Tcl_SetResult(interp, " Floating point exception in slv_iterate. Help!",
                    TCL_STATIC);
      return TCL_ERROR;
    }
    delta=tm_cpu_time()-start;
  }
  return TCL_OK;
}

int Asc_SolvAvailSolver(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  int i;

  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argc;     /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  for ( i = 0; i < slv_number_of_solvers; i++ ) {
    Tcl_AppendElement(interp,(char *)slv_solver_name(i));
  }
  return TCL_OK;
}

int Asc_SolvLinsolNames(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argc;     /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  Tcl_AppendResult(interp,linsolqr_fmethods(),SNULL);
  return TCL_OK;
}

int Asc_SolvEligSolver(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  /*  KHACK: removed 'n' from call to slv_eligible_solver
   *  may need to remove 'n' from this function totaly
   */
  slv_parameters_t sp;
  int cur;
  int status=0;
  int n;
  int tmpi;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if (( argc < 2 ) || ( argc > 3 )) {
    FPRINTF(ASCERR, "call is: slv_eligible_solver <solver number> [all]\n");
    Tcl_SetResult(interp, "slv_eligible_solver: solver number expected",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur == NULL) {
    FPRINTF(ASCERR,  "slv_eligible_solver called with NULL pointer\n");
    Tcl_SetResult(interp, "slv_eligible_solver called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  slv_get_parameters(g_solvsys_cur,&sp);
  cur = slv_get_selected_solver(g_solvsys_cur);
  if (argc==3 && !!sp.output.less_important) {
    FPRINTF(ASCERR,"Solver   Name       ?Eligible\n");
    FPRINTF(ASCERR,"-----------------------------\n");
    for( n=0 ; n<slv_number_of_solvers ; ++n ) {
      FPRINTF(ASCERR, "%c%3d     %-11s    %s\n", ((n==cur) ? '*' : ' '), n,
              slv_solver_name(n), YORN(slv_eligible_solver(g_solvsys_cur)));
    }
  }
  status=Tcl_GetInt(interp, argv[1], &tmpi);
  Tcl_ResetResult(interp);
  if ((status==TCL_ERROR) || (tmpi<0) || (tmpi>=slv_number_of_solvers)) {
    Tcl_SetResult(interp,
                  "slv_eligible_solver: called with invalid solver number",
                  TCL_STATIC);
    return TCL_ERROR;
  } else {
    n = tmpi;
    if (slv_eligible_solver(g_solvsys_cur)) {
      Tcl_SetResult(interp, "1", TCL_STATIC);
    } else {
      Tcl_SetResult(interp, "0", TCL_STATIC);
    }
  }
  return TCL_OK;
}

int Asc_SolvSelectSolver(ClientData cdata, Tcl_Interp *interp,
                         int argc, CONST84 char *argv[])
{
  int status=TCL_OK;
  int solver;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR, "call is: slv_select_solver <N>\n");
    Tcl_SetResult(interp, "1 argument expected for slv_select_solver",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "slv_select_solver called with NULL pointer\n");
    Tcl_SetResult(interp, "slv_select_solver called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  status=Tcl_GetInt(interp, argv[1], &solver);
  if ((solver<0) || (solver>slv_number_of_solvers) || (status==TCL_ERROR)) {
    FPRINTF(ASCERR, "unknown solver (%d). Not selected!\n",solver);
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "Solver not available.", TCL_STATIC);
    return TCL_ERROR;
  } else {
    char num[8];
    int i = slv_get_selected_solver(g_solvsys_cur);
    if ( solver != i ) {
      i = slv_select_solver(g_solvsys_cur,solver);
    }
    sprintf(num,"%d",i);
    Tcl_AppendElement(interp,&num[0]);
    return TCL_OK;
  }
  /* not reached */
}

int Asc_SolvGetSelectedSolver(ClientData cdata, Tcl_Interp *interp,
                             int argc, CONST84 char *argv[])
{
  int solver;
  char * tmps=NULL;

  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  tmps= (char *)ascmalloc((MAXIMUM_NUMERIC_LENGTH+1)*sizeof(char));
  if ( argc != 1 ) {
    FPRINTF(ASCERR, "call is: slv_get_solver <N>\n");
    Tcl_SetResult(interp, "No args allowed for slv_get_solver", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "slv_get_solver called with NULL pointer\n");
    Tcl_SetResult(interp, "slv_get_solver called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  solver =  slv_get_selected_solver(g_solvsys_cur);
  sprintf(tmps,"%d", solver);
  Tcl_AppendElement(interp,tmps);
  ascfree(tmps);
  return TCL_OK;
}

int Asc_SolvFlushSolver(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  slv_system_t systmp;

  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)interp;   /* stop gcc whine about unused parameter */
  (void)argc;     /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if (g_solvsys_cur != NULL) {
    systmp=g_solvsys_cur;
    system_destroy(systmp);
    g_solvsys_cur = NULL;
    g_solvinst_cur = NULL;
    g_solvinst_root = NULL;
  }
  return TCL_OK;
}

int Asc_SolvMakeIndependent(ClientData cdata, Tcl_Interp *interp,
                          int argc, CONST84 char *argv[])
{
  int j,k,tmpi,status=TCL_OK;
  int32 maxvar,freevar;
  struct var_variable **vp=NULL;
  var_filter_t vfilter;
  slv_system_t sys=NULL;
  int32 *swapvars=NULL;
  int32 *unassvars=NULL;
  mtx_range_t rng;
  mtx_matrix_t mtx=NULL;
  char res[40];

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc < 2 ) {
    FPRINTF(ASCERR, "call is: slv_set_independent <ndx ...>\n");
    Tcl_SetResult(interp, "slv_set_independent wants at least 1 var index",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  sys=g_solvsys_cur;
  if (sys==NULL) {
    FPRINTF(ASCERR,  "slv_set_independent called with NULL pointer\n");
    Tcl_SetResult(interp, "slv_set_independent without slv_system",TCL_STATIC);
    return TCL_ERROR;
  }
  mtx=slv_get_sys_mtx(sys);
  if (mtx==NULL) {
    FPRINTF(ASCERR,"slv_set_independent found no matrix. odd!\n");
    Tcl_SetResult(interp, "slv_set_independent found no matrix. odd!",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  vp=slv_get_solvers_var_list(sys);
  if (vp==NULL) {
    FPRINTF(ASCERR,  "slv_set_independent called with NULL varlist\n");
    Tcl_SetResult(interp, "slv_set_independent called without varlist",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  maxvar=slv_get_num_solvers_vars(sys);

  vfilter.matchbits = (VAR_INCIDENT | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_ACTIVE);
  freevar=slv_count_solvers_vars(sys,&vfilter);
  rng.high=freevar-1;
  rng.low=mtx_symbolic_rank(mtx);
  if ( (argc-1) > (rng.high-rng.low+1) ) {
    FPRINTF(ASCERR,  "slv_set_independent called with too many vars\n");
    Tcl_SetResult(interp, "slv_set_independent called with too many vars",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  swapvars=(int32 *)ascmalloc(sizeof(int32)*(argc-1));
  k=rng.high-rng.low+1;
  unassvars=(int32 *)ascmalloc(sizeof(int32)*k);
  for (j=0;j<k;j++) {
    unassvars[j]=mtx_col_to_org(mtx,j+rng.low); /* current outsiders */
  }
  for (j=1;j<argc;j++) {
    tmpi=maxvar;
    status=Tcl_GetInt(interp,argv[j],&tmpi);
    if (tmpi<0 || tmpi >= maxvar) {
      status=TCL_ERROR;
    }
    if (status!=TCL_OK) {
      FPRINTF(ASCERR,
        "slv_set_independent: %d is not number in variable list\n",tmpi);
      Tcl_ResetResult(interp);
      Tcl_SetResult(interp, "slv_set_independent: invalid variable number",
                    TCL_STATIC);
      if (swapvars) {
        ascfree(swapvars);
      }
      if (unassvars) {
        ascfree(unassvars);
      }
      return status;
    } else {
      swapvars[j-1]=tmpi; /*var index numbers*/
    }
  }
  k=argc-1;
  for (j=0;j<k;j++) {
    if (slv_change_basis(sys,swapvars[j],&rng) ) {
      for (tmpi=rng.low;tmpi<=rng.high;tmpi++) {
        if (unassvars[tmpi-rng.low]!=mtx_col_to_org(mtx,tmpi)) {
          int32 tmpd;
          mtx_swap_cols(mtx,tmpi,rng.high);
          tmpd=unassvars[tmpi-rng.low];
          unassvars[tmpi-rng.low]=unassvars[rng.high-rng.low];
          unassvars[rng.high-rng.low]=tmpd;
          break;
        }
      }
      rng.high--;
    } else {
      char *name;
      name=var_make_name(sys,vp[swapvars[j]]);
      FPRINTF(ASCERR,"Unable to remove %s from the basis.\n",name);
      ascfree(name);
      sprintf(res,"%d",swapvars[j]);
      Tcl_AppendElement(interp,res);
    }
  }
  if (swapvars) {
    ascfree(swapvars);
  }
  if (unassvars) {
    ascfree(unassvars);
  }
  return TCL_OK;
}

int Asc_SolvImportQlfdid(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  int status, listc,prevs=0;
  char *temp=NULL;
  CONST84 char **listargv=NULL;
  slv_system_t systmp;
  enum inst_t ikind;
  struct Instance *solvinst_pot=NULL;  /* potential solve instance */
  struct Instance *solvinst_root_pot=NULL;  /* potential solve instance */

  if (argc<2 || argc>3) {
    Tcl_SetResult(interp, "slv_import_qlfdid <qlfdid> [test]", TCL_STATIC);
    return TCL_ERROR;
  }

  status=Asc_BrowQlfdidSearchCmd(cdata, interp, (int)2, argv);
  temp = strdup(Tcl_GetStringResult(interp));
  Tcl_ResetResult(interp);

  if (status==TCL_OK) {
    /* catch inst ptr */
    solvinst_pot = g_search_inst;
    /* catch root name */
    status=Tcl_SplitList(interp, temp, &listc, &listargv);
    if (status!=TCL_OK) { /* this should never happen */
      Tcl_Free((char *)listargv);
      Tcl_ResetResult(interp);
      Tcl_SetResult(interp, "slv_import_qlfdid: error in split list for sim",
                    TCL_STATIC);
      FPRINTF(ASCERR, "wierdness in slv_import_qlfdid splitlist.\n");
      solvinst_pot =NULL;
      if (temp) {
        ascfree(temp);
      }
      temp=NULL;
      return status;
    }
    /* catch root inst ptr */
    solvinst_root_pot = Asc_FindSimulationRoot(AddSymbol(listargv[0]));
    Tcl_Free((char *)listargv);
    if (!solvinst_root_pot) { /*an error we should never reach, knock wood */
      Tcl_ResetResult(interp);
      FPRINTF(ASCERR, "NULL simulation found by slv_import_qlfdid. %s\n",temp);
      Tcl_SetResult(interp,
                    "slv_import_qlfdid: Simulation specified not found.",
                    TCL_STATIC);
      if (temp) {
        ascfree(temp);
      }
      temp=NULL;
      return TCL_ERROR;
    }
  } else {
    /* failed. bail out. */
    Tcl_SetResult(interp, "slv_import_qlfdid: Asc_BrowQlfdidSearchCmd: ",
                  TCL_STATIC);
    Tcl_AppendResult(interp, temp, SNULL);
    FPRINTF(ASCERR, "slv_import_qlfdid: Asc_BrowQlfdidSearchCmd error\n");
    if (temp) {
      ascfree(temp);
    }
    temp=NULL;
    return status;
  }
  /* got something worth having */
  if (temp) {
    ascfree(temp);
  }
  temp=NULL;
  Tcl_ResetResult(interp);

  /* check that instance is model */
  ikind=InstanceKind(solvinst_pot);
  if (ikind!=MODEL_INST) {
    switch (argc) {
      case 3: /* just testing */
        Tcl_SetResult(interp, "1", TCL_STATIC);
        return TCL_OK;
      default: /*report import error */
        FPRINTF(ASCERR,  "Instance imported is not a solvable kind.\n");
        Tcl_SetResult(interp, "Instance kind not MODEL.", TCL_STATIC);
        return TCL_ERROR;
    }
  }

  /* check instance is complete */
  if (NumberPendingInstances(solvinst_pot)!=0) {
    switch (argc) {
      case 3: /* just testing */
        Tcl_SetResult(interp, "1", TCL_STATIC);
        CheckInstance(ASCERR,solvinst_pot);
        return TCL_OK;
      default: /*report import error */
        FPRINTF(ASCERR,  "Instance imported is incomplete: %ld pendings.\n",
                NumberPendingInstances(solvinst_pot));
        Tcl_SetResult(interp, "Instance has pendings: Not imported.",
                      TCL_STATIC);
        return TCL_ERROR;
    }
  }

  if ( argc == 2 ) { /*not just testing */
    /* Here we will check to see if we really need to do
        all of this work by:
        1) Checking if the potential and current instance pointers are equal
        2) Checking a global counter to see if the compiler has been called
    */
    if (g_solvsys_cur == NULL) {
      g_compiler_counter = 1; /* initialize compiler counter */
    }
    if (g_solvinst_cur == solvinst_pot && g_compiler_counter == 0
        && g_solvinst_cur != NULL) {
      prevs = slv_get_selected_solver(g_solvsys_cur);
      slv_select_solver(g_solvsys_cur,prevs);
      Tcl_SetResult(interp, "Solver instance created.", TCL_STATIC);
#if SP_DEBUG
      FPRINTF(ASCERR,"YOU JUST AVOIDED A TOTAL REBUILD\n");
#endif
      return TCL_OK;
    }

    /* flush old system */
    g_solvinst_cur=solvinst_pot;
    g_solvinst_root=solvinst_root_pot;
    if (g_solvsys_cur != NULL) {
      prevs = slv_get_selected_solver(g_solvsys_cur);
      systmp=g_solvsys_cur;
      system_destroy(systmp);
      g_solvsys_cur = NULL;
    }


    /* create system */
    if( g_solvsys_cur == NULL ) {
      g_solvsys_cur = system_build(g_solvinst_cur);
      if( g_solvsys_cur == NULL ) {
        FPRINTF(ASCERR,"system_build returned NULL.\n");
        Tcl_SetResult(interp, "Bad relations found: solve system not created.",
                      TCL_STATIC);
        return TCL_ERROR;
      }
    }

    if( g_solvsys_cur == NULL ) {
      FPRINTF(ASCERR,"system_build returned NULL!\n");
      Tcl_SetResult(interp, "importqlfdid:  solve system not created.",
                    TCL_STATIC);
      return TCL_ERROR;
    }
    slv_select_solver(g_solvsys_cur,prevs);
    Tcl_SetResult(interp, "Solver instance created.", TCL_STATIC);
    g_compiler_counter = 0;  /* set counter to 0 after full import */
  } else {
    Tcl_SetResult(interp, "0", TCL_STATIC);
  }
  return TCL_OK;
}

int Asc_SolvGetLnmEpsilon(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  char buf[MAXIMUM_NUMERIC_LENGTH];   /* string to hold integer */
  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc > 1 ) {
    Tcl_SetResult(interp, "slv_lnmget takes no argument.", TCL_STATIC);
    return TCL_ERROR;
  }
  sprintf(buf, "%g",FuncGetLnmEpsilon());
  Tcl_SetResult(interp, buf, TCL_VOLATILE);
  return TCL_OK;
}

int Asc_SolvSetLnmEpsilon(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  double eps;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "slv_lnmset takes 1 positive # argument.", TCL_STATIC);
    return TCL_ERROR;
  }
  eps=FuncGetLnmEpsilon();
  if( Tcl_GetDouble(interp,argv[1],&eps)==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "slv_lnmset: arg 1 not real number", TCL_STATIC);
    return TCL_ERROR;
  }
  if (eps < 0.5) {
    FuncSetLnmEpsilon(eps);
  } else {
    FPRINTF(ASCERR,"Modified log epsilon > 0.5 not allowed. Eps = %g.\n",eps);
  }
  return TCL_OK;
}

/*
 * Solv_C_CheckHalt_Flag is defined in slv.[ch].
 */
int Asc_SolvSetCHaltFlag(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  int value;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "wrong # args : Usage slv_set_haltflag", TCL_STATIC);
    return TCL_ERROR;
  }
  value = atoi(argv[1]);
  if (value) {
    Solv_C_CheckHalt_Flag = 1; /* any nonzero value will set the flag on. */
  } else {
    Solv_C_CheckHalt_Flag = 0; /* otherwise turn it off */
  }
  return TCL_OK;
}

#define LONGHELP(b,ms) ((b)?ms:"")
int Asc_SolvHelpList(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{
  boolean detail=1;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc > 2 ) {
    FPRINTF(ASCERR,"call is: slvhelp [s,l] \n");
    Tcl_SetResult(interp, "Too many args to slvhelp. Want 0 or 1 args",
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
    PRINTF("%-25s%s\n","slv_trapfp",
           LONGHELP(detail,"turn floating point traps on for solver"));
    PRINTF("%-25s%s\n","slv_untrapfp",
           LONGHELP(detail,"turn floating point traps off. take core dump."));
    PRINTF("%-25s%s\n","slv_checksim",
           LONGHELP(detail,"see if simulation has pendings:0ok,1incomplete"));
    PRINTF("%-25s%s\n","slv_checksys",
           LONGHELP(detail,"see if solver is occupied:0free,1busy"));
    PRINTF("%-25s%s\n","slv_get_parms",
           LONGHELP(detail,"get list of solver parameters."));
    PRINTF("%-25s%s\n","set_slv_parms",
           LONGHELP(detail,"set list of solver parameters."));
    PRINTF("%-25s%s\n","slv_get_insttype",
           LONGHELP(detail,"get typename of model instance being solved."));

    PRINTF("%-25s%s\n","slv_get_cost_page",
           LONGHELP(detail,"get list of block costs."));
    PRINTF("%-25s%s\n","slv_get_stat_page",
           LONGHELP(detail,"get list of status values."));
    PRINTF("%-25s%s\n","slv_get_objval",
           LONGHELP(detail,"get value of objective function"));
    PRINTF("%-25s%s\n","slv_get_instname",
           LONGHELP(detail,"get instance path name from instroot to instcur"));
    PRINTF("%-25s%s\n","slv_get_pathname",
           LONGHELP(detail,"get solver inst qlfdid"));
    PRINTF("%-25s%s\n","slvdump",
           LONGHELP(detail,"dump something about the solver insides."));

    PRINTF("%-25s%s\n","slv_reanalyze",
           LONGHELP(detail,"reanalyze the solver lists of g_solvsys_cur ."));
    PRINTF("%-25s%s\n","slv_check_and_reanalyze",
           LONGHELP(detail,"reanalyze g_solvsys_cur if a whenvar changes."));
    PRINTF("%-25s%s\n","slv_get_vr",
           LONGHELP(detail,"return some counts of rels/vars."));
    PRINTF("%-25s%s\n","slv_presolve",
           LONGHELP(detail,"call presolve on the g_solvsys_cur."));
    PRINTF("%-25s%s\n","slv_resolve",
           LONGHELP(detail,"call resolve on g_solvsys_cur."));
    PRINTF("%-25s%s\n","slv_solve",
           LONGHELP(detail,"call solve on g_solvsys_cur."));
    PRINTF("%-25s%s\n","slv_iterate",
           LONGHELP(detail,"call solve_iterate on g_solvsys_cur."));

    PRINTF("%-25s%s\n","slv_available",
           LONGHELP(detail,"list names of all known solvers"));
    PRINTF("%-25s%s\n","slv_linsol_names",
           LONGHELP(detail,"list names of all linear options for Slv class"));
    PRINTF("%-25s%s\n","slv_eligible_solver",
           LONGHELP(detail,"boolean check of current solver eligibility"));
    PRINTF("%-25s%s\n","slv_select_solver",
           LONGHELP(detail,"set solver to use."));
    PRINTF("%-25s%s\n","slv_get_solver",
           LONGHELP(detail,"return solver number in use."));
    PRINTF("%-25s%s\n","slv_flush_solver",
           LONGHELP(detail,"blow away g_solvsys_cur"));
    PRINTF("%-25s%s\n","slv_set_independent",
           LONGHELP(detail,"select set of independent (superbasic) vars"));

    PRINTF("%-25s%s\n","slv_import_qlfdid",
           LONGHELP(detail,"focus solver on qualified name, or test it."));
    PRINTF("%-25s%s\n","get_model_children",
           LONGHELP(detail,"return the list of MODEL children of a qlfdid"));
#if DELETEME
    PRINTF("%-25s%s\n","slv_import_sim",
           LONGHELP(detail,"focus solver on simname."));
#endif /* DELETEME */
    PRINTF("%-25s%s\n","slv_lnmget",
           LONGHELP(detail,"return lnm epsilon value"));
    PRINTF("%-25s%s\n","slv_lnmset",
           LONGHELP(detail,"set lnm epsilon value"));
    PRINTF("%-25s%s\n","integration commands",
           LONGHELP(detail,""));
    PRINTF("%-25s%s\n","integrate_able",
           LONGHELP(detail,"check solver problem for integrability"));
    PRINTF("%-25s%s\n","integrate_setup",
           LONGHELP(detail,"setup and integrate an ivp in solver"));
    PRINTF("%-25s%s\n","integrate_cleanup",
           LONGHELP(detail,"tidy up after an ivp in solver"));
    PRINTF("%-25s%s\n","slvhelp",
           LONGHELP(detail,"slvhelp s(=names only) l(=this list)."));

    PRINTF("\n");
  }
  if ( argc == 1 ) {
    char * tmps=NULL;
    tmps= (char *)ascmalloc((MAXIMUM_NUMERIC_LENGTH+1)*sizeof(char));
    sprintf(tmps,"slv_checksys");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_trapfp");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_untrapfp");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_checksim");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_get_parm");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"set_slv_parm");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_get_insttype");
    Tcl_AppendElement(interp,tmps);

    sprintf(tmps,"slv_get_cost_page");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_get_stat_page");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_get_objval");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_get_instname");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_get_pathname");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slvdump");
    Tcl_AppendElement(interp,tmps);

    sprintf(tmps,"slv_reanalyze");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_check_and_reanalyze");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_get_vr");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_presolve");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_resolve");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_solve");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_iterate");
    Tcl_AppendElement(interp,tmps);

    sprintf(tmps,"slv_available");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_linsol_names");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_eligible_solver");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_select_solver");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_get_solver");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_flush_solver");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_set_independent");
    Tcl_AppendElement(interp,tmps);

    sprintf(tmps,"slv_import_qlfdid");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_import_sim");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_lnmget");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slv_lnmset");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"integrate_able");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"integrate_setup");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"integrate_cleanup");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"slvhelp");
    Tcl_AppendElement(interp,tmps);
    ascfree(tmps);
  }
  return TCL_OK;
}


/*NOTE: Output is not terribly meaninful when put to stdout or ASCERR */
int Asc_SolvNearBounds(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  int32 *rip=NULL;
  real64 epsilon;
  char tmps[MAXIMUM_NUMERIC_LENGTH];
  int i,dev,status,count;
  FILE *fp;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 3 ) {
    FPRINTF(ASCERR,  "call is: slv_near_bounds epsilon <out>\n");
    Tcl_SetResult(interp, "slv_near_bounds wants epsilon and output device.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "slv_near_bounds called with NULL pointer\n");
    Tcl_SetResult(interp, "slv_near_bounds called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  /* get io option */
  i=3;
  status=Tcl_GetDouble(interp,argv[1],&epsilon);
  status=Tcl_GetInt(interp,argv[2],&i);
  if (i<0 || i >2) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(ASCERR,"slv_near_bounds: first arg is 0,1, or 2\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "slv_near_bounds: invalid output dev #", TCL_STATIC);
    return status;
  } else {
    dev=i;
  }
  switch (dev) {
  case 0: fp=stdout;
    break;
  case 1: fp=ASCERR;
    break;
  case 2: fp=NULL;
    break;
  default : /* should never be here */
    FPRINTF(ASCERR,"slv_near_bounds called with strange i/o option\n");
    return TCL_ERROR;
  }
  if ((count = slv_near_bounds(g_solvsys_cur,epsilon,&rip)) > 0) {
    count += 2;
    switch (dev) {
    case 0:
    case 1:
      FPRINTF(fp,"Objective indices:\n");
      for (i=0; i < count;i++) {
        FPRINTF(fp,"%d\n",rip[i]);
      }
      break;
    case 2:
      Tcl_AppendResult(interp,"{",SNULL);
      for (i=0; i < count;i++) {
        sprintf(tmps,"%d ",rip[i]);
        Tcl_AppendResult(interp,tmps,SNULL);
      }
      Tcl_AppendResult(interp,"}",SNULL);
      break;
    default:
      FPRINTF(ASCERR,"wierdness in i/o!");
      break;
    }
  } else {
    Tcl_SetResult(interp, "{}", TCL_STATIC);
  }
  if (rip) {
    ascfree(rip);
  }

  return TCL_OK;
}

/*NOTE: Output is not terribly meaninful when put to stdout or ASCERR */
int Asc_SolvFarFromNominal(ClientData cdata, Tcl_Interp *interp,
                          int argc, CONST84 char *argv[])
{
  int32 *rip=NULL;
  real64 bignum;
  char tmps[MAXIMUM_NUMERIC_LENGTH];
  int i,dev,status,count;
  FILE *fp;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 3 ) {
    FPRINTF(ASCERR,  "call is: slv_far_from_nom <bignum> <out>\n");
    Tcl_SetResult(interp,
                  "slv_far_from_nominals wants bignum and output device.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "slv_far_from_nominals called with NULL pointer\n");
    Tcl_SetResult(interp, "slv_far_from_nominals called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  /* get io option */
  i=3;
  status=Tcl_GetDouble(interp,argv[1],&bignum);
  status=Tcl_GetInt(interp,argv[2],&i);
  if (i<0 || i >2) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(ASCERR,"slv_far_from_nominals: first arg is 0,1, or 2\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "slv_far_from_nominals: invalid output dev #",
                  TCL_STATIC);
    return status;
  } else {
    dev=i;
  }
  switch (dev) {
  case 0: fp=stdout;
    break;
  case 1: fp=ASCERR;
    break;
  case 2: fp=NULL;
    break;
  default : /* should never be here */
    FPRINTF(ASCERR,"slv_far_from_nominals called with strange i/o option\n");
    return TCL_ERROR;
  }
  if ((count = slv_far_from_nominals(g_solvsys_cur,bignum,&rip)) > 0) {
    switch (dev) {
    case 0:
    case 1:
      FPRINTF(fp,"Objective indices:\n");
      for (i=0; i < count;i++) {
        FPRINTF(fp,"%d\n",rip[i]);
      }
      break;
    case 2:
      Tcl_AppendResult(interp,"{",SNULL);
      for (i=0; i < count;i++) {
        sprintf(tmps,"%d ",rip[i]);
        Tcl_AppendResult(interp,tmps,SNULL);
      }
      Tcl_AppendResult(interp,"}",SNULL);
      break;
    default:
      FPRINTF(ASCERR,"wierdness in i/o!");
      break;
    }
  } else {
    Tcl_SetResult(interp, "{}", TCL_STATIC);
  }
  if (rip) {
    ascfree(rip);
  }

  return TCL_OK;
}
