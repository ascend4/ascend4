/*
 *  DebugProc.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.64 $
 *  Version control file: $RCSfile: DebugProc.c,v $
 *  Date last modified: $Date: 2003/08/23 18:43:05 $
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

#include <signal.h>
#include <setjmp.h>
#include "tcl.h"
#include "utilities/ascConfig.h"
#include "utilities/ascSignal.h"
#include "utilities/ascMalloc.h"
#include "general/list.h"
#include "compiler/compiler.h"
#include "compiler/instance_enum.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/instance_name.h"
#include "compiler/atomvalue.h"
#include "compiler/instquery.h"
#include "compiler/types.h"
#include "compiler/mathinst.h"
#include "compiler/relation_type.h"
#include "compiler/extfunc.h"
#include "compiler/find.h"
#include "compiler/functype.h"
#include "compiler/safe.h"
#include "compiler/relation.h"
#include "compiler/relation_util.h"
#include "compiler/pending.h"
#include "compiler/symtab.h"
#include "solver/slv_types.h"
#include "solver/mtx.h"
#include "solver/calc.h"
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
#include "solver/slv_stdcalls.h"
#include "solver/system.h"
#include "solver/slvDOF.h"
#include "interface/old_utils.h"
#include "interface/HelpProc.h"
#include "interface/Qlfdid.h"
#include "interface/BrowserQuery.h"
#include "interface/DebugProc.h"
#include "interface/BrowserMethod.h"
#include "interface/DisplayProc.h"
#include "interface/HelpProc.h"
#include "interface/DebugProc.h"
#include "interface/SolverGlobals.h"
#include "interface/BrowserProc.h"
/* #include "slv5.h" *//* this is a sloppy mess due to slv5_calc_J */

#ifndef lint
static CONST char DebugProcID[] = "$Id: DebugProc.c,v 1.64 2003/08/23 18:43:05 ballan Exp $";
#endif


#define SAFE_FIX_ME 0
#define REIMPLEMENT 0
#define TORF(b) ((b) ? "TRUE" : "FALSE")
#define YORN(b) ((b) ? "YES" : "NO")
#define ONEORZERO(b) ((b) ? "1" : "0")
#define SNULL (char *)NULL
#define QLFDID_LENGTH 1023
#define DP_DEBUG TRUE

/*
 * This function needs to be fixed to deal with mtxless systems
 * much better.
 */
int Asc_DebuGetBlkOfVar(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  char * tmps;
  int32 col,numblock,ndx,maxvar,blow,bhigh;
  int status =TCL_OK;
  mtx_matrix_t mtx;
  mtx_region_t reg;
  struct var_variable **vp;
  var_filter_t vfilter;
  dof_t *d;
  const mtx_block_t *b;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,  "call is: dbg_get_blk_of_var <var index>\n");
    Tcl_SetResult(interp, "dbg_get_blk_of_var takes 1 arg", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "dbg_get_blk_of_var called with NULL pointer\n");
    Tcl_SetResult(interp, "dbg_get_blk_of_var called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  mtx = slv_get_sys_mtx(g_solvsys_cur);
  if (mtx==NULL) {
    /* this is a horrible hack and incorrect and all that */
    /* probably should issue a warning here */
    Tcl_SetResult(interp, "0", TCL_STATIC);
    return TCL_OK;
  }
  d = slv_get_dofdata(g_solvsys_cur);
  b = slv_get_solvers_blocks(g_solvsys_cur);
  assert(d!=NULL && b!=NULL);

  vp=slv_get_solvers_var_list(g_solvsys_cur);
  /*  maxvar=slv_get_num_solvers_vars(g_solvsys_cur); */
  vfilter.matchbits = (VAR_ACTIVE);
  vfilter.matchvalue = (VAR_ACTIVE);
  maxvar=slv_count_solvers_vars(g_solvsys_cur,&vfilter);
  ndx=maxvar;

  status=Tcl_GetInt(interp,argv[1],&ndx);
  if (ndx>=maxvar||status==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "get_blk_of_var: variable does not exist",
                  TCL_STATIC);
    FPRINTF(ASCERR,  "dbg_get_blk_of_var: variable index invalid\n");
    return TCL_ERROR;
  }
  col = mtx_org_to_col(mtx,ndx);
  blow = 0;
  bhigh = b->nblocks-1;
  numblock = -1;
  while( blow <= bhigh ) {
    int32 block_number = (blow+bhigh)/2;
    if( col > b->block[block_number].col.high ) {
      blow = block_number+1;
    } else if( col < b->block[block_number].col.low ) {
        bhigh = block_number-1;
      } else {
          reg = b->block[block_number];
          numblock = block_number;
          break;
        }
  }
  if ( var_fixed(vp[ndx]) || numblock<0 || !var_active(vp[ndx]) ) {
    Tcl_SetResult(interp, "none", TCL_STATIC);
    return TCL_OK;
  } else {
    tmps= (char *)ascmalloc((MAXIMUM_NUMERIC_LENGTH+1)*sizeof(char));
    sprintf(tmps,"%d",numblock);
    Tcl_AppendElement(interp,tmps);
    ascfree(tmps);
  }
  return TCL_OK;
}

/*
 * This function needs to be fixed to deal with mtxless systems
 * much better.
 */
int Asc_DebuGetBlkOfEqn(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  char * tmps;
  int32 row,numblock,ndx,maxrel,blow,bhigh;
  int status = TCL_OK;
  mtx_matrix_t mtx;
  mtx_region_t reg;
  struct rel_relation **rp;
  rel_filter_t rfilter;
  dof_t *d;
  const mtx_block_t *b;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,  "call is: dbg_get_blk_of_eqn <rel index>\n");
    Tcl_SetResult(interp, "dbg_get_blk_of_eqn takes 1 arg", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "dbg_get_blk_of_eqn called with NULL pointer\n");
    Tcl_SetResult(interp, "dbg_get_blk_of_eqn called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  mtx = slv_get_sys_mtx(g_solvsys_cur);
  if (mtx==NULL) {
    /* this is a horrible hack and incorrect and all that */
    /*probably should issue a warning here */
    Tcl_SetResult(interp, "0", TCL_STATIC);
    return TCL_OK;
  }
  d = slv_get_dofdata(g_solvsys_cur);
  b = slv_get_solvers_blocks(g_solvsys_cur);
  assert(d!=NULL && b!=NULL);

  rp=slv_get_solvers_rel_list(g_solvsys_cur);
  /*  maxrel=slv_get_num_solvers_rels(g_solvsys_cur); */
  rfilter.matchbits = (REL_ACTIVE);
  rfilter.matchvalue = (REL_ACTIVE);
  maxrel=slv_count_solvers_rels(g_solvsys_cur,&rfilter);
  ndx=maxrel;

  status=Tcl_GetInt(interp,argv[1],&ndx);
  if (ndx>=maxrel||status==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp,
                  "dbg_get_blk_of_eqn: equation requested does not exist",
                  TCL_STATIC);
    FPRINTF(ASCERR, "dbg_get_blk_of_eqn: relation index invalid.\n");
    return TCL_ERROR;
  }
  row = mtx_org_to_row(mtx,ndx);
  blow = 0;
  bhigh = b->nblocks-1;
  numblock = -1;
  while( blow <= bhigh ) {
    int32 block_number = (blow+bhigh)/2;
    if( row > b->block[block_number].row.high ) {
      blow = block_number+1;
    } else if( row < b->block[block_number].row.low ) {
        bhigh = block_number-1;
      } else {
          reg = b->block[block_number];
          numblock = block_number;
          break;
        }
  }
  if (numblock<0 || !rel_included(rp[ndx]) || !rel_active(rp[ndx])) {
    Tcl_SetResult(interp, "none", TCL_STATIC);
    return TCL_OK;
  } else {
    tmps= (char *)ascmalloc((MAXIMUM_NUMERIC_LENGTH+1)*sizeof(char));
    sprintf(tmps,"%d",numblock);
    Tcl_AppendElement(interp,tmps);
    ascfree(tmps);
  }
  return TCL_OK;
}

/*
 * this function deals ok with mtxless solvers
 */
int Asc_DebuGetBlkCoords(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  char * tmps;
  int32 numblock,ndx,maxblk;
  int status =TCL_OK;
  mtx_region_t reg;
  dof_t *d;
  const mtx_block_t *b;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,  "call is: dbg_get_blk_coords <blocknumber>\n");
    Tcl_SetResult(interp, "dbg_get_blk_coords takes 1 arg", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "dbg_get_blk_coords called with NULL pointer\n");
    Tcl_SetResult(interp, "dbg_get_blk_coords called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  d = slv_get_dofdata(g_solvsys_cur);
  b = slv_get_solvers_blocks(g_solvsys_cur);
  assert(d!=NULL && b!=NULL);

  numblock = b->nblocks-1;
  maxblk = ndx = INT_MAX;
  status=Tcl_GetInt(interp,argv[1],&ndx);
  if (ndx<0 ||ndx>=maxblk||status==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "dbg_get_blk_coords: block does not exist",
                  TCL_STATIC);
    FPRINTF(ASCERR,  "dbg_get_blk_coords: block index invalid\n");
    return TCL_ERROR;
  }
  if (ndx>numblock) {
    Tcl_SetResult(interp, "none", TCL_STATIC);
    return TCL_OK;
  } else {
    reg = b->block[ndx];
    tmps= (char *)ascmalloc((2*MAXIMUM_NUMERIC_LENGTH+1)*sizeof(char));
    sprintf(tmps,"%d %d %d %d",
            reg.col.low, reg.row.low, reg.col.high, reg.row.high);
    Tcl_AppendResult(interp,tmps,SNULL);
    ascfree(tmps);
  }
  return TCL_OK;
}

/*
 * needs to deal with mtxless systems.
 * quite likely spitting garbage.
 */
int Asc_DebuGetEqnOfVar(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  char * tmps;
  int32 num,maxvar,numeq;
  int tmpi,status=TCL_OK;
  mtx_matrix_t mtx;
  struct var_variable **vp;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,"call is: dbg_get_eqn_of_var <var Cindex> \n");
    Tcl_SetResult(interp, "dbg_get_eqn_of_var wants 1 arg", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "dbg_get_eqn_of_var called with NULL pointer\n");
    Tcl_SetResult(interp, "dbg_get_eqn_of_var called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  mtx = slv_get_sys_mtx(g_solvsys_cur);
  vp=slv_get_solvers_var_list(g_solvsys_cur);
  maxvar=slv_get_num_solvers_vars(g_solvsys_cur);

  tmpi=maxvar;
  status=Tcl_GetInt(interp,argv[1],&tmpi);
  if (tmpi<0 || tmpi >= maxvar) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(ASCERR,"dbg_get_eqn_of_var: arg is not variable number in list\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "dbg_get_eqn_of_var: invalid variable number",
                  TCL_STATIC);
    return status;
  }
  tmps= (char *)ascmalloc((MAXIMUM_NUMERIC_LENGTH+1)*sizeof(char));
  num=tmpi;

  numeq = mtx_row_to_org(mtx,mtx_org_to_col(mtx,num));
  if (numeq<0
      || numeq>=maxvar
      || var_fixed(vp[numeq])
      || !var_active(vp[numeq]) ) {
    Tcl_SetResult(interp, "none", TCL_STATIC);
  } else {
    sprintf(tmps,"%d",numeq);
    Tcl_AppendElement(interp,tmps);
  }
  ascfree(tmps);
  return TCL_OK;
}

int Asc_DebuGetVarPartition(ClientData cdata, Tcl_Interp *interp,
                          int argc, CONST84 char *argv[])
{
  char * tmps;
  int32 numblock,lastblock,c,maxvar;
  mtx_matrix_t mtx;
  dof_t *d;
  const mtx_block_t *b;

  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    FPRINTF(ASCERR,  "call is: dbg_get_varpartition <no args>\n");
    Tcl_SetResult(interp, "dbg_get_varpartition: takes no arguments.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "dbg_get_varpartition called with NULL pointer\n");
    Tcl_SetResult(interp, "dbg_get_varpartition called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  mtx = slv_get_sys_mtx(g_solvsys_cur);
  d = slv_get_dofdata(g_solvsys_cur);
  b = slv_get_solvers_blocks(g_solvsys_cur);
  assert(d!=NULL && b!=NULL);

  lastblock = b->nblocks;

  tmps= (char *)ascmalloc((MAXIMUM_NUMERIC_LENGTH+1)*sizeof(char));
  if (b->nblocks >1) {
    mtx_region_t reg;
    for (numblock=0; numblock <lastblock; numblock++) { /* over each block */
      reg = b->block[numblock];
      for( ; reg.col.low <= reg.col.high; reg.col.low++ ) {
        sprintf(tmps,"%d",mtx_col_to_org(mtx,reg.col.low));
        Tcl_AppendElement(interp,tmps);
      }
      sprintf(tmps,"/"); /* add block separator w/out extra whitespace */
      Tcl_AppendResult(interp,tmps,SNULL);
    }
  } else {
    struct var_variable **vp;
    vp=slv_get_solvers_var_list(g_solvsys_cur);
    maxvar=slv_get_num_solvers_vars(g_solvsys_cur);
    if (vp) {
      for (c=0; c<maxvar; c++) {
        if (!var_fixed(vp[c]) && var_incident(vp[c]) && var_active(vp[c]) ) {
          sprintf(tmps,"%d",var_sindex(vp[c]));
          Tcl_AppendElement(interp,tmps);
        } /* all in one block, no / needed */
      }
    }
  }
  ascfree(tmps);
  return TCL_OK;
}

int Asc_DebuGetEqnPartition(ClientData cdata, Tcl_Interp *interp,
                          int argc, CONST84 char *argv[])
{
  char * tmps;
  int32 numblock,lastblock,maxrel,c;
  mtx_matrix_t mtx;
  dof_t *d;
  const mtx_block_t *b;

  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    FPRINTF(ASCERR,  "call is: dbg_get_eqnpartition <no args>\n");
    Tcl_SetResult(interp, "dbg_get_eqnpartition: takes no arguments.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "dbg_get_eqnpartition called with NULL pointer\n");
    Tcl_SetResult(interp, "dbg_get_eqnpartition called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  mtx = slv_get_sys_mtx(g_solvsys_cur);
  d = slv_get_dofdata(g_solvsys_cur);
  b = slv_get_solvers_blocks(g_solvsys_cur);
  assert(d!=NULL && b!=NULL);

  lastblock = b->nblocks;

  tmps= (char *)ascmalloc((MAXIMUM_NUMERIC_LENGTH+1)*sizeof(char));
  if (b->nblocks >1) {
    mtx_region_t reg;
    for (numblock=0; numblock <lastblock; numblock++) { /* over each block */
      reg = b->block[numblock];
      for( ; reg.row.low <= reg.row.high; reg.row.low++ ) {
        sprintf(tmps,"%d",mtx_row_to_org(mtx,reg.row.low));
        Tcl_AppendElement(interp,tmps);
      }
      sprintf(tmps,"/"); /* add block separator w/out extra whitespace */
      Tcl_AppendResult(interp,tmps,SNULL);
    }
  } else {
    struct rel_relation **rp;
    rp=slv_get_solvers_rel_list(g_solvsys_cur);
    maxrel=slv_get_num_solvers_rels(g_solvsys_cur);
    if (rp) {
      for (c=0; c<maxrel; c++) {
        if (rel_included(rp[c]) && rel_active(rp[c])) {
          sprintf(tmps,"%d",rel_sindex(rp[c]));
          Tcl_AppendElement(interp,tmps);
        } /* all in one block, no / needed */
      }
    }
  }
  ascfree(tmps);
  return TCL_OK;
}

int Asc_DebuListVars(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{
  int status=TCL_OK,fil;
  var_filter_t vfilter;
  struct var_variable **vp;
  int32 maxvar,c;
  mtx_matrix_t mtx;
  dof_t *d;
  boolean vbool;
  char tmps[MAXIMUM_NUMERIC_LENGTH+1];

  (void)cdata;    /* stop gcc whine about unused parameter */

  if (( argc != 2 ) && ( argc != 3 )) {
    FPRINTF(ASCERR,"call is: dbg_list_vars <1 args> [not] \n");
    FPRINTF(ASCERR,"filter codes are:\n");
    FPRINTF(ASCERR,"0  all vars, a rather redundant thing to do\n");
    FPRINTF(ASCERR,"1  all vars incident\n");
    FPRINTF(ASCERR,"2  all vars fixed\n");
    FPRINTF(ASCERR,"3  all vars free\n");
    FPRINTF(ASCERR,"4  all vars assigned\n");
    FPRINTF(ASCERR,"5  all vars free & incident\n");
    FFLUSH(ASCERR);
    Tcl_SetResult(interp, "dbg_list_vars wants at least 1 arg", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "dbg_list_vars called with NULL pointer\n");
    Tcl_SetResult(interp,"dbg_list_vars called without slv_system",TCL_STATIC);
    return TCL_ERROR;
  }

  mtx = slv_get_sys_mtx(g_solvsys_cur);
  status=Tcl_GetInt(interp,argv[1],&fil);
  if(status!=TCL_OK) {
    FPRINTF(ASCERR,  "dbg_list_vars called with noninteger arg 1\n");
    Tcl_SetResult(interp,"dbg_list_vars first arg must be integer",TCL_STATIC);
    return TCL_ERROR;
  }

  d = slv_get_dofdata(g_solvsys_cur);

  switch (fil) {
    case 0: /*all*/
      vfilter.matchbits = (VAR_ACTIVE);
      vfilter.matchvalue = (VAR_ACTIVE);
      break;
    case 1:/*incid*/
      vfilter.matchbits = (VAR_INCIDENT | VAR_ACTIVE);
      vfilter.matchvalue = (VAR_INCIDENT | VAR_ACTIVE);
      break;
    case 2:/*fixed*/
      vfilter.matchbits = (VAR_FIXED | VAR_ACTIVE);
      vfilter.matchvalue = (VAR_FIXED | VAR_ACTIVE);
      break;
    case 3:/*free*/
      vfilter.matchbits = (VAR_FIXED | VAR_ACTIVE);
      vfilter.matchvalue = (VAR_ACTIVE);
      break;
    case 4:/*assigned*/
      if (!mtx) {
        FPRINTF(ASCERR,  "dbg_list_vars called with NULL mtx pointer\n");
        Tcl_SetResult(interp,"dbg_list_vars found bad system mtx", TCL_STATIC);
        return TCL_ERROR;
      }
      break;
    case 5:/*free*/
      vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_ACTIVE);
      vfilter.matchvalue = (VAR_INCIDENT | VAR_ACTIVE);
      break;
    default:
      Tcl_SetResult(interp, "dbg_list_vars: Unrecognized variable filter",
                    TCL_STATIC);
      return TCL_ERROR;
  }
  vp=slv_get_solvers_var_list(g_solvsys_cur);
  maxvar=slv_get_num_solvers_vars(g_solvsys_cur);
  for (c=0; c<maxvar; c++) {
    switch( fil ) {
      case 0: case 1:
      case 2: case 3:
      case 5:
        vbool = var_apply_filter(vp[c],&vfilter);
        break;
      case 4: {
        int32 col = mtx_org_to_col(mtx,var_sindex(vp[c]));
        vbool = ((col < d->structural_rank) && (col >= 0));
        break;
      }
    }
    if( argc == 3 ) {
      vbool = !vbool;
    }
    if( vbool ) {
      sprintf(&tmps[0],"%d",var_sindex(vp[c]));
      Tcl_AppendElement(interp,&tmps[0]);
    }
  }
  return TCL_OK;
}



int Asc_DebuListRels(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{
  int status=TCL_OK,fil;
  rel_filter_t rfilter;
  struct rel_relation **rp;
  int32 maxrel,c;
  mtx_matrix_t mtx;
  dof_t *d;
  boolean rbool;
  char tmps[MAXIMUM_NUMERIC_LENGTH+1];

  (void)cdata;    /* stop gcc whine about unused parameter */

  if (( argc != 2 ) && ( argc != 3 )) {
    FPRINTF(ASCERR,"call is: dbg_list_rels <1 args> [not] \n");
    FPRINTF(ASCERR,"filter codes are:\n");
    FPRINTF(ASCERR,"0  all relations, a rather redundant thing to do\n");
    FPRINTF(ASCERR,"1  all relations included\n");
    FPRINTF(ASCERR,"2  all equalities\n");
    FPRINTF(ASCERR,"3  all inequalities\n");
    FPRINTF(ASCERR,"4  all assigned relations\n");
    FFLUSH(ASCERR);
    Tcl_SetResult(interp, "dbg_list_rels wants at least 1 arg", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "dbg_list_rels called with NULL pointer\n");
    Tcl_SetResult(interp,"dbg_list_rels called without slv_system",TCL_STATIC);
    return TCL_ERROR;
  }

  mtx = slv_get_sys_mtx(g_solvsys_cur);
  status=Tcl_GetInt(interp,argv[1],&fil);
  if(status!=TCL_OK) {
    FPRINTF(ASCERR,  "dbg_list_rels called with noninteger arg 1\n");
    Tcl_SetResult(interp,"dbg_list_rels first arg must be integer",TCL_STATIC);
    return TCL_ERROR;
  }

  d = slv_get_dofdata(g_solvsys_cur);

  switch (fil) {
    case 0: /*all*/
      rfilter.matchbits = (REL_ACTIVE) ;
      rfilter.matchvalue = (REL_ACTIVE) ;
      break;
    case 1:/*included*/
      rfilter.matchbits = (REL_INCLUDED | REL_ACTIVE);
      rfilter.matchvalue =(REL_INCLUDED | REL_ACTIVE);
      break;
    case 2:/*equality*/
      rfilter.matchbits = (REL_EQUALITY | REL_ACTIVE);
      rfilter.matchvalue = (REL_EQUALITY | REL_ACTIVE);
      break;
    case 3:/*inequality*/
      rfilter.matchbits = (REL_EQUALITY | REL_ACTIVE);
      rfilter.matchvalue = ( REL_ACTIVE);
      break;
    case 4:/*assigned*/
      if (!mtx) {
        FPRINTF(ASCERR,  "dbg_list_rels called with NULL mtx pointer\n");
        Tcl_SetResult(interp, "dbg_list_rels found bad system mtx",TCL_STATIC);
        return TCL_ERROR;
      }
      break;
    default:
      Tcl_SetResult(interp, "dbg_list_rels: Unrecognized relation filter",
                    TCL_STATIC);
      return TCL_ERROR;
  }
  rp=slv_get_solvers_rel_list(g_solvsys_cur);
  maxrel=slv_get_num_solvers_rels(g_solvsys_cur);

  for ( c=0; c<maxrel; c++) {
    switch( fil ) {
      case 0: case 1:
      case 2: case 3:
        rbool = rel_apply_filter(rp[c],&rfilter);
        break;
      case 4: {
        int32 row = mtx_org_to_row(mtx,rel_sindex(rp[c]));
        rbool = ((row < d->structural_rank) && (row >= 0));
        break;
      }
    }
    if( argc == 3 ) {
      rbool = !rbool;
    }
    if( rbool ) {
      sprintf(&tmps[0],"%d",rel_sindex(rp[c]));
      Tcl_AppendElement(interp,&tmps[0]);
    }
  }
  return TCL_OK;
}

int Asc_DebuWriteVar(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{
  int tmpi,dev,status=TCL_OK;
  char tmps[QLFDID_LENGTH+1];
  int32 maxvar,varnum,ilist;
  struct var_variable **vp;
  var_filter_t vfilter;
  slv_system_t sys=NULL;
  FILE *fp;
  char *name=NULL;

  tmps[QLFDID_LENGTH]='\0';

  /*check sanity */
  if (argc !=5 && argc !=6) {
    FPRINTF(ASCERR,
            "call is: dbg/brow_write_var <dev> %s",
            " <var ndx> <fmt (#<8)>  <solver/master> [simname]\n");
    Tcl_SetResult(interp, "dbg/brow_write_var wants at least 4 args",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (!cdata) {
    sys=g_solvsys_cur;
  } else {
    sys=g_browsys_cur;
  }
  if (sys==NULL) {
    FPRINTF(ASCERR,  "dbg/brow_write_var called with NULL pointer\n");
    Tcl_SetResult(interp, "dbg/brow_write_var called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  /* get io option */
  tmpi=3;
  status=Tcl_GetInt(interp,argv[1],&tmpi);
  if (tmpi<0 || tmpi >2) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(ASCERR,"dbg/brow_write_var: first arg is 0,1, or 2\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "dbg/brow_write_var: invalid output dev",TCL_STATIC);
    return status;
  } else {
    dev=tmpi;
  }
  switch (dev) {
    case 0: fp=stdout;
            break;
    case 1: fp=ASCERR;
            break;
    case 2: fp=NULL;
            break;
    default : /* should never be here */
            FPRINTF(ASCERR,
              "dbg/brow_write_var called with strange i/o option!!\n");
            return TCL_ERROR;
  }

  /* get list option */
  tmpi=0;
  status=Tcl_GetInt(interp,argv[4],&tmpi);
  if (tmpi<0 || tmpi >1) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(ASCERR,
    "dbg/brow_write_var: last arg is 0 (solver list) or 1 (master list)\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "dbg/brow_write_var: invalid var list",TCL_STATIC);
    return status;
  } else {
    ilist =tmpi;
  }

  if (ilist == 0) {
    vp=slv_get_solvers_var_list(sys);
  } else {
    vp=slv_get_master_var_list(sys);
  }

  /*get variable index */
  maxvar=slv_get_num_solvers_vars(sys);
  tmpi=maxvar;
  status=Tcl_GetInt(interp,argv[2],&tmpi);
  if (tmpi<0 || tmpi >= maxvar) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(ASCERR,
      "dbg/brow_write_var: 2nd arg is not variable number in list\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "dbg/brow_write_var: invalid variable number",
                  TCL_STATIC);
    return status;
  } else {
    varnum=tmpi;
    vp = vp + varnum;
  }
  /* get detail option */
  status=Tcl_GetInt(interp,argv[3],&tmpi);
  if (tmpi<0 || tmpi >7) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(ASCERR,"dbg/brow_write_var: 3rd arg is not valid output format\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "dbg/brow_write_var: invalid output format #",
                  TCL_STATIC);
    return status;
  }
  /* tmpi is now the format option, don't change it. */
  if (tmpi>=3) { /*interface varindex*/
    switch (dev) {
      case 0:
      case 1:
         FPRINTF(fp,"<%d> ",varnum);
         break;
      case 2:
         sprintf(&tmps[0],"<%d>",varnum);
         Tcl_AppendElement(interp,&tmps[0]);
         break;
      default: break;
    }
  }
  if (tmpi>=0) { /* qlfdid */
    name = var_make_name(sys,*vp);
    switch (dev) {
      case 0:
      case 1:
        if( argc == 6 ) {
          FPRINTF(fp,"%s.",argv[5]);
        }
        FPRINTF(fp,"%s ",name);
        break;
      case 2:
        Tcl_AppendElement(interp,name);
        break;
      default: break;
    }
    if (name) {
      ascfree(name);
    }
    name=NULL;
  }
  if (tmpi>=1) {/* value */
    switch (dev) {
      case 0:
      case 1:
        FPRINTF(fp,"%g ",var_value(*vp));
        break;
      case 2:
        sprintf(&tmps[0],"%g",var_value(*vp));
        Tcl_AppendElement(interp,&tmps[0]);
        break;
      default: break;
    }
  }
  if (tmpi>=2) {/* dims */
    char *dimens;
    dimens = asc_make_dimensions(RealAtomDims(var_instance(*vp)));
    switch (dev) {
      case 0:
      case 1:
        FPRINTF(fp,"%s ",dimens);
        break;
      case 2:
        Tcl_AppendElement(interp,dimens);
        break;
      default: break;
    }
    ascfree(dimens);
  }
  if (tmpi>=4) {/* fixed flag */
    vfilter.matchbits = (VAR_FIXED | VAR_ACTIVE);
    vfilter.matchvalue = (VAR_FIXED | VAR_ACTIVE);
    switch (dev) {
      case 0:
      case 1:
        FPRINTF(fp," fixed=%s", TORF(var_apply_filter(*vp,&vfilter)));
        break;
      case 2:
        sprintf(&tmps[0],"fixed=%s", TORF(var_apply_filter(*vp,&vfilter)));
        Tcl_AppendElement(interp,&tmps[0]);
        break;
      default: break;
    }
  }
  if (tmpi>=5) {/* lower_bound */
    switch (dev) {
      case 0:
      case 1:
        FPRINTF(fp," %g ",var_lower_bound(*vp));
        break;
      case 2:
        sprintf(&tmps[0],"%g",var_lower_bound(*vp));
        Tcl_AppendElement(interp,&tmps[0]);
        break;
      default: break;
    }
  }
  if (tmpi>=6) {/* nominal */
    switch (dev) {
      case 0:
      case 1:
        FPRINTF(fp,"%g ",var_nominal(*vp));
        break;
      case 2:
        sprintf(&tmps[0],"%g",var_nominal(*vp));
        Tcl_AppendElement(interp,&tmps[0]);
        break;
      default: break;
    }
  }
  if (tmpi>=7) {/* upper_bound */
    switch (dev) {
      case 0:
      case 1:
        FPRINTF(fp,"%g ",var_upper_bound(*vp));
        break;
      case 2:
        sprintf(&tmps[0],"%g",var_upper_bound(*vp));
        Tcl_AppendElement(interp,&tmps[0]);
        break;
      default: break;
    }
  }
  if (dev<2) {
    FPRINTF(fp,"\n");
  }
  return TCL_OK;
}

int Asc_DebuWriteRel(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{
  int tmpi,dev,status=TCL_OK;
  char tmps[MAXIMUM_NUMERIC_LENGTH+1];
  int32 maxrel,relnum;
  struct rel_relation **rp;
  slv_system_t sys=NULL;
  FILE *fp;

  tmps[MAXIMUM_NUMERIC_LENGTH]='\0';
  /*check sanity */
  if (argc !=4 && argc !=5) {
    FPRINTF(ASCERR,
      "call is: dbg/brow_write_rel <dev> <rel ndx> <fmt (#<5)> [simname] \n");
    Tcl_SetResult(interp, "dbg/brow_write_rel wants at least 3 args",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (!cdata) {
    sys=g_solvsys_cur;
  } else {
    sys=g_browsys_cur;
  }
  if (sys==NULL) {
    FPRINTF(ASCERR,  "dbg/brow_write_rel called with NULL pointer\n");
    Tcl_SetResult(interp, "dbg/brow_write_rel called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  /* get io option */
  tmpi=3;
  status=Tcl_GetInt(interp,argv[1],&tmpi);
  if (tmpi<0 || tmpi >2) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(ASCERR,"dbg/brow_write_rel: first arg is 0,1, or 2\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "dbg/brow_write_rel: invalid output dev #",
                  TCL_STATIC);
    return status;
  } else {
    dev=tmpi;
  }
  switch (dev) {
    case 0: fp=stdout;
            break;
    case 1: fp=ASCERR;
            break;
    case 2: fp=NULL;
            break;
    default : /* should never be here */
            FPRINTF(ASCERR,
              "dbg/brow_write_rel called with strange i/o option!!\n");
            return TCL_ERROR;
  }
  /*get relation index */
  rp=slv_get_solvers_rel_list(sys);
  maxrel=slv_get_num_solvers_rels(sys);
  tmpi=maxrel;
  status=Tcl_GetInt(interp,argv[2],&tmpi);
  if (tmpi<0 || tmpi >= maxrel) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    Tcl_ResetResult(interp);
    FPRINTF(ASCERR,
      "dbg/brow_write_rel: 2nd arg is not relation number in list\n");
    Tcl_SetResult(interp, "dbg/brow_write_rel: invalid relation number",
                  TCL_STATIC);
    return status;
  } else {
    relnum=tmpi;
    rp = rp + relnum;
  }
  /* get detail option */
  status=Tcl_GetInt(interp,argv[3],&tmpi);
  if (tmpi<0 || tmpi >4) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    Tcl_ResetResult(interp);
    FPRINTF(ASCERR,"dbg/brow_write_rel: 3rd arg is not valid output format\n");
    Tcl_SetResult(interp, "dbg/brow_write_rel: invalid output format #",
                  TCL_STATIC);
    return status;
  }
  /* tmpi is now the format option, don't change it. */
  if (tmpi==4) { /* return only the relation string */
    char *infix=NULL;
    infix= relman_make_string_infix(sys,*rp);
    if (dev<2) {
      FPRINTF(fp,"%s\n",infix);
    } else {
      Tcl_AppendElement(interp, infix);
    }
    if (infix) {
      ascfree(infix);
    }
    return TCL_OK;
  }
  if (tmpi>=2) { /*interface relindex*/
    switch (dev) {
      case 0:
      case 1:
         FPRINTF(fp,"<%d> ",relnum);
         break;
      case 2:
         sprintf(&tmps[0],"<%d>",relnum);
         Tcl_AppendElement(interp,&tmps[0]);
         break;
      default: break;
    }
  }
  if (tmpi>=0) { /* qlfdid */
    char *name=NULL;
    name = rel_make_name(sys,*rp);
    switch (dev) {
      case 0:
      case 1:
        if( argc == 5 ) {
          FPRINTF(fp,"%s.",argv[4]);
        }
        FPRINTF(fp,"%s ",name);
        break;
      case 2:
        Tcl_AppendElement(interp,name);
        break;
      default: break;
    }
    if (name) {
      ascfree(name);
    }
  }
  if (tmpi>=1) {/* residual */
    double res=0;
    res=relman_eval(*rp,&calc_ok,SAFE_FIX_ME);
    switch (dev) {
      case 0:
      case 1:
        FPRINTF(fp,"%g ",res);
        break;
      case 2:
        sprintf(&tmps[0],"%g",res);
        Tcl_AppendElement(interp,&tmps[0]);
        break;
      default: break;
    }
  }
  if (tmpi>=3) {/* include flag */
    int truth;
    truth=(rel_included(*rp) && rel_active(*rp));
    switch (dev) {
      case 0:
      case 1:
        FPRINTF(fp," included and active=%s", TORF(truth));
        break;
      case 2:
        sprintf(&tmps[0],"included and active =%s", TORF(truth));
        Tcl_AppendElement(interp,&tmps[0]);
        break;
      default: break;
    }
  }
  if (dev<2) {
    FPRINTF(fp,"\n");
  }
  return TCL_OK;
}

int Asc_DebuWriteUnattachedVar(ClientData cdata, Tcl_Interp *interp,
                             int argc, CONST84 char *argv[])
{
  int tmpi,dev,status=TCL_OK;
  char tmps[QLFDID_LENGTH+1];
  int32 maxvar,c;
  struct var_variable **vp;
  var_filter_t vfilter;
  slv_system_t sys=NULL;
  FILE *fp;
  char *name=NULL;
  char *dimens;

  tmps[QLFDID_LENGTH]='\0';

  /*check sanity */
  if ( argc < 2 ) {
    FPRINTF(ASCERR,
      "call is: dbg_write_unattvar <dev> [simname] \n");
    Tcl_SetResult(interp, "dbg_write_unattvar wants 2 args", TCL_STATIC);
    return TCL_ERROR;
  }

  if (!cdata) {
    sys=g_solvsys_cur;
  } else {
    sys=g_browsys_cur;
  }
  if (sys==NULL) {
    FPRINTF(ASCERR,  "dbg_write_unattvar called with NULL pointer\n");
    Tcl_SetResult(interp, "dbg_write_var unattcalled without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }


  /* get io option */
  tmpi=3;
  status=Tcl_GetInt(interp,argv[1],&tmpi);

  if (tmpi<0 || tmpi >2) {
    status=TCL_ERROR;
  }

  if (status!=TCL_OK) {
    FPRINTF(ASCERR,"dbg_write_unattvar: first arg is 0,1, or 2\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "dbg_write_unattvar: invalid output dev",TCL_STATIC);
    return status;
  } else {
    dev=tmpi;
  }


  switch (dev) {
    case 0: fp=stdout;
            break;
    case 1: fp=ASCERR;
            break;
    case 2: fp=NULL;
            break;
    default : /* should never be here */
            FPRINTF(ASCERR,
              "dbg_write_unattvar called with strange i/o option!!\n");
            return TCL_ERROR;
  }

  /*get unattached variable list */
  vp=slv_get_solvers_unattached_list(sys);
  maxvar = slv_get_num_solvers_unattached(sys);

  vfilter.matchbits = (VAR_ACTIVE);
  vfilter.matchvalue = (VAR_ACTIVE);

  /* Writing the list of unattached variables */
  for (c=0; c<maxvar; c++) {
    if (var_apply_filter(vp[c],&vfilter)) {
      /* qlfdid */
      name = var_make_name(sys,vp[c]);
      switch (dev) {
        case 0:
        case 1:
          FPRINTF(fp,"%s ",name);
          break;
        case 2:
          Tcl_AppendElement(interp,name);
          break;
        default: break;
      }

      if (name) {
        ascfree(name);
        name=NULL;
      }

      /* value */
      switch (dev) {
        case 0:
        case 1:
          FPRINTF(fp,"%g ",var_value(vp[c]));
          break;
        case 2:
          sprintf(&tmps[0],"%g",var_value(vp[c]));
          Tcl_AppendElement(interp,&tmps[0]);
          break;
        default: break;
      }


      /* dims */
      dimens = asc_make_dimensions(RealAtomDims(var_instance(vp[c])));
      switch (dev) {
        case 0:
        case 1:
          FPRINTF(fp,"%s ",dimens);
          break;
        case 2:
          Tcl_AppendElement(interp,dimens);
          break;
        default: break;
      }
      ascfree(dimens);

      if (dev<2) {
        FPRINTF(fp,"\n");
      }
    }
  }

  return TCL_OK;
}

int Asc_DebuWriteObj(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{
  int tmpi,dev,status=TCL_OK;
  char tmps[MAXIMUM_NUMERIC_LENGTH+1];
  int32 maxrel,relnum;
  struct rel_relation **rp;
  slv_system_t sys=NULL;
  FILE *fp;

  tmps[MAXIMUM_NUMERIC_LENGTH]='\0';
  /*check sanity */
  if (argc !=4 && argc !=5) {
    FPRINTF(ASCERR,
      "call is: dbg/brow_write_obj <dev> <rel ndx> <fmt (#<5)> [simname] \n");
    Tcl_SetResult(interp, "dbg/brow_write_obj wants at least 3 args",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (!cdata) {
    sys=g_solvsys_cur;
  } else {
    sys=g_browsys_cur;
  }
  if (sys==NULL) {
    FPRINTF(ASCERR,  "dbg/brow_write_obj called with NULL pointer\n");
    Tcl_SetResult(interp, "dbg/brow_write_obj called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  /* get io option */
  tmpi=3;
  status=Tcl_GetInt(interp,argv[1],&tmpi);
  if (tmpi<0 || tmpi >2) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(ASCERR,"dbg/brow_write_obj: first arg is 0,1, or 2\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "dbg/brow_write_obj: invalid output dev #",
                  TCL_STATIC);
    return status;
  } else {
    dev=tmpi;
  }
  switch (dev) {
    case 0: fp=stdout;
            break;
    case 1: fp=ASCERR;
            break;
    case 2: fp=NULL;
            break;
    default : /* should never be here */
            FPRINTF(ASCERR,
              "dbg/brow_write_obj called with strange i/o option!!\n");
            return TCL_ERROR;
  }
  /*get relation index */
  rp=slv_get_solvers_obj_list(sys);
  maxrel=slv_get_num_solvers_objs(sys);
  tmpi=maxrel;
  status=Tcl_GetInt(interp,argv[2],&tmpi);
  if (tmpi<0 || tmpi >= maxrel) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    Tcl_ResetResult(interp);
    FPRINTF(ASCERR,
      "dbg/brow_write_obj: 2nd arg is not objective number in list\n");
    Tcl_SetResult(interp, "dbg/brow_write_obj: invalid objective number",
                  TCL_STATIC);
    return status;
  } else {
    relnum=tmpi;
    rp = rp + relnum;
  }
  /* get detail option */
  status=Tcl_GetInt(interp,argv[3],&tmpi);
  if (tmpi<0 || tmpi >4) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    Tcl_ResetResult(interp);
    FPRINTF(ASCERR,"dbg/brow_write_obj: 3rd arg is not valid output format\n");
    Tcl_SetResult(interp, "dbg/brow_write_obj: invalid output format #",
                  TCL_STATIC);
    return status;
  }
  /* tmpi is now the format option, don't change it. */
  if (tmpi==4) { /* return only the objective string */
    char *infix=NULL;
    infix= relman_make_string_infix(sys,*rp);
    if (dev<2) {
      FPRINTF(fp,"%s\n",infix);
    } else {
      Tcl_AppendElement(interp, infix);
    }
    if (infix) {
      ascfree(infix);
    }
    return TCL_OK;
  }
  if (tmpi>=2) { /*interface relindex*/
    switch (dev) {
      case 0:
      case 1:
         FPRINTF(fp,"<%d> ",relnum);
         break;
      case 2:
         sprintf(&tmps[0],"<%d>",relnum);
         Tcl_AppendElement(interp,&tmps[0]);
         break;
      default: break;
    }
  }
  if (tmpi>=0) { /* qlfdid */
    char *name=NULL;
    name = rel_make_name(sys,*rp);
    switch (dev) {
      case 0:
      case 1:
        if( argc == 5 ) {
          FPRINTF(fp,"%s.",argv[4]);
        }
        FPRINTF(fp,"%s ",name);
        break;
      case 2:
        Tcl_AppendElement(interp,name);
        break;
      default: break;
    }
    if (name) {
      ascfree(name);
    }
  }
  if (tmpi>=1) {/* residual */
    double res=0;
    res=relman_eval(*rp,&calc_ok,SAFE_FIX_ME);
    switch (dev) {
      case 0:
      case 1:
        FPRINTF(fp,"%g ",res);
        break;
      case 2:
        sprintf(&tmps[0],"%g",res);
        Tcl_AppendElement(interp,&tmps[0]);
        break;
      default: break;
    }
  }
  if (tmpi>=3) {/* include flag */
    int truth;
    truth=(rel_included(*rp) && rel_active(*rp));
    switch (dev) {
      case 0:
      case 1:
        FPRINTF(fp," included and active=%s", TORF(truth));
        break;
      case 2:
        sprintf(&tmps[0],"included and active =%s", TORF(truth));
        Tcl_AppendElement(interp,&tmps[0]);
        break;
      default: break;
    }
  }
  if (dev<2) {
    FPRINTF(fp,"\n");
  }
  return TCL_OK;
}

int Asc_DebuWriteVarAttr(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  int tmpi,status=TCL_OK;
  char tmps[QLFDID_LENGTH+1];
  int32 maxvar,varnum;
  struct var_variable **vp;
  struct Instance *i;
  char *name=NULL;
  char *dimens=NULL;
  slv_system_t sys;
  sys=g_solvsys_cur; /* may be null */

  tmps[QLFDID_LENGTH]='\0';
  /*check sanity */
  if ( argc != 2 ) {
    if (cdata) {
      FPRINTF(ASCERR, "call is: dbg_write_qlfattr <qlfdid>\n");
      Tcl_SetResult(interp, "dbg_write_qlfattr wants 1 arg", TCL_STATIC);
    } else {
      FPRINTF(ASCERR, "call is: dbg_write_varattr <var ndx>\n");
      Tcl_SetResult(interp, "dbg_write_varattr wants 1 arg", TCL_STATIC);
    }
    return TCL_ERROR;
  }
  if (!cdata) { /* dbg_write_varattr case */
    if (sys==NULL) {
      FPRINTF(ASCERR,  "dbg_write_varattr called with NULL pointer\n");
      Tcl_SetResult(interp, "dbg_write_varattr called without slv_system",
                    TCL_STATIC);
      return TCL_ERROR;
    }
    /*get variable index */
    vp=slv_get_solvers_var_list(sys);
    if (vp==NULL) {
      FPRINTF(ASCERR,  "dbg_write_varattr called with NULL varlist\n");
      Tcl_SetResult(interp, "dbg_write_varattr called without varlist",
                    TCL_STATIC);
      return TCL_ERROR;
    }
    maxvar=slv_get_num_solvers_vars(sys);
    tmpi=maxvar;
    status=Tcl_GetInt(interp,argv[1],&tmpi);
    if (tmpi<0 || tmpi >= maxvar) {
      status=TCL_ERROR;
    }
    if (status!=TCL_OK) {
      FPRINTF(ASCERR,"dbg_write_varattr: arg not variable number in list\n");
      Tcl_ResetResult(interp);
      Tcl_SetResult(interp, "dbg_write_varattr: invalid variable number",
                    TCL_STATIC);
      return status;
    } else {
      varnum=tmpi;
      vp = vp + varnum;
      i=var_instance(*vp);
    }
  } else { /* qlfattr case */ /* broken, since vars != instances */
#define VARS_EQ_INSTS 0
#if VARS_EQ_INST
    status = Asc_QlfdidSearch3(argv[1],0);
    if (status==0) {
      i = g_search_inst;
      vp = &i; /* this is in error */
    } else {
      Tcl_AppendResult(interp,"dbg_write_qlfattr: QlfdidSearch error",
                       argv[1]," not found.",SNULL);
      return TCL_ERROR;
    }
    if (InstanceKind(i)!=REAL_ATOM_INST) {
      Tcl_SetResult(interp,"dbg_write_qlfattr called on non-variable instance",
                    TCL_STATIC);
      return TCL_ERROR;
    }
  } /* vp and i now set to interesting instance */
  if (!vp || !i) {
    if (cdata) {
      FPRINTF(ASCERR, "dbg_write_qlfattr found NULL variable instance\n");
      Tcl_SetResult(interp,"dbg_write_qlfattr found NULL variable",TCL_STATIC);
    } else {
      FPRINTF(ASCERR, "dbg_write_varattr found NULL variable instance\n");
      Tcl_SetResult(interp,"dbg_write_varattr found NULL variable",TCL_STATIC);
    }
    return TCL_ERROR;
#else
    Tcl_SetResult(interp,
                  "dbg_write_qlfattr broken since vars no longer = instances.",
                  TCL_STATIC);
    return TCL_ERROR;
#endif
  }
  /* write type */
  Tcl_AppendResult(interp,"{TYPE: ",(char *)InstanceType(i),"} ",SNULL);
  /* write dims */
  dimens = asc_make_dimensions(RealAtomDims(var_instance(*vp)));
  Tcl_AppendResult(interp,"{DIMENSIONS: ",dimens,"}",SNULL);
  if (dimens) {
    ascfree(dimens);
  }
  dimens=NULL;
  /* write value */
  sprintf(tmps,"VALUE: %g",var_value(*vp));
  Tcl_AppendElement(interp,tmps);
  /* write qlfdid */
  if (cdata) {
    Tcl_AppendElement(interp,argv[1]);
  } else {
    name = var_make_name(sys,*vp); /* this is in error. no sys exists */
    Tcl_AppendElement(interp,name);
    if (name) {
      ascfree(name);
    }
    name=NULL;
  }
  if (Asc_DispWriteIpCmd(interp,i)) {
    Tcl_AppendElement(interp,"index: -1");
    Tcl_AppendElement(interp,"incident: -1");
    Tcl_AppendElement(interp,"in block: -1");
  }
  Tcl_AppendResult(interp," ",SNULL);
  Asc_BrowWriteAtomChildren(interp,i);
  return TCL_OK;
}

int Asc_DebuRelIncluded(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  int tmpi,status=TCL_OK;
  int32 maxrel,relnum;
  struct rel_relation **rp;
  slv_system_t sys=NULL;
  char res[40];

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR, "call is: dbg_rel_included <var ndx>\n");
    Tcl_SetResult(interp, "dbg_rel_included wants 1 arg", TCL_STATIC);
    return TCL_ERROR;
  }
  sys=g_solvsys_cur;
  if (sys==NULL) {
    FPRINTF(ASCERR,  "dbg_rel_included called with NULL pointer\n");
    Tcl_SetResult(interp, "dbg_rel_included called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  /*get relation index */
  rp=slv_get_solvers_rel_list(sys);
  maxrel=slv_get_num_solvers_rels(sys);
  tmpi=maxrel;
  status=Tcl_GetInt(interp,argv[1],&tmpi);
  if (tmpi<0 || tmpi >= maxrel) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(ASCERR, "dbg_rel_included: arg is not number in relation list\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "dbg_rel_included: invalid relation number",
                  TCL_STATIC);
    return status;
  } else {
    relnum=tmpi;
    rp = rp + relnum;
  }
  sprintf(res,"%d",(rel_included(*rp) && rel_active(*rp) ));
  Tcl_AppendResult(interp,res,SNULL);
  return TCL_OK;
}

int Asc_DebuVarFixed(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{
  int tmpi,status=TCL_OK;
  int32 maxvar,varnum;
  struct var_variable **vp;
  slv_system_t sys=NULL;
  char res[40];

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR, "call is: dbg_var_fixed <var ndx>\n");
    Tcl_SetResult(interp, "dbg_var_fixed wants 1 arg", TCL_STATIC);
    return TCL_ERROR;
  }
  sys=g_solvsys_cur;
  if (sys==NULL) {
    FPRINTF(ASCERR,  "dbg_var_fixed called with NULL pointer\n");
    Tcl_SetResult(interp,"dbg_var_fixed called without slv_system",TCL_STATIC);
    return TCL_ERROR;
  }
  /*get variable index */
  vp=slv_get_solvers_var_list(sys);
  maxvar=slv_get_num_solvers_vars(sys);
  tmpi=maxvar;
  status=Tcl_GetInt(interp,argv[1],&tmpi);
  if (tmpi<0 || tmpi >= maxvar) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(ASCERR, "dbg_var_fixed: arg is not number in variable list\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "dbg_var_fixed: invalid variable number",TCL_STATIC);
    return status;
  } else {
    varnum=tmpi;
    vp = vp + varnum;
  }
  sprintf(res,"%d",var_fixed(*vp));
  Tcl_AppendResult(interp,res,SNULL);
  return TCL_OK;
}


int Asc_DebuGetIncidence(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  int32 relnum,maxrel,ninc,c;
  struct rel_relation **rp=NULL;
  var_filter_t vfilter;
  const struct var_variable **vp=NULL;
  int status=TCL_OK;
  char *tmps=NULL;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,  "call is: dbg_get_incidence <rel index>\n");
    Tcl_SetResult(interp, "dbg_get_incidence takes 1 arg", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "dbg_get_incidence called with NULL pointer\n");
    Tcl_SetResult(interp, "dbg_get_incidence called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  rp=slv_get_solvers_rel_list(g_solvsys_cur);
  if (!rp) {
    FPRINTF(ASCERR,  "NULL relation list found in dbg_get_incidence\n");
    Tcl_SetResult(interp, "dbg_get_incidence called with null rellist",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  maxrel=slv_get_num_solvers_rels(g_solvsys_cur);
  relnum=maxrel;
  status=Tcl_GetInt(interp,argv[1],&relnum);
  if (relnum>=maxrel||status==TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp,
                  "dbg_get_incidence: equation requested does not exist",
                  TCL_STATIC);
    /*FPRINTF(ASCERR,  "dbg_get_incidence: relation index invalid.\n"); */
    return TCL_ERROR;
  }

  tmps= (char *)ascmalloc((MAXIMUM_NUMERIC_LENGTH+1)*sizeof(char));
  ninc = rel_n_incidences(rp[relnum]);
  vp=rel_incidence_list(rp[relnum]);

  vfilter.matchbits = (VAR_SVAR | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_SVAR | VAR_ACTIVE);

  if (vp) {
    for(c=0; c<ninc; c++ ) {
      if(var_apply_filter(vp[c],&vfilter)) {
        sprintf(tmps,"%d",var_sindex(vp[c]));
        Tcl_AppendElement(interp,tmps);
      }
    }
  }
  if (tmps) {
    ascfree(tmps);
  }
  return TCL_OK;
}

int Asc_DebuGetOrder(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char **argv)
{
  int32 ndx,rc,max;
  mtx_matrix_t mtx;
  char num[20];
  rel_filter_t rfilter;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,"call is: dbg_get_order <row,col> \n");
    Tcl_SetResult(interp, "dbg_get_order wants one arg", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,"dbg_get_order called with empty slv_system\n");
    Tcl_SetResult(interp, "dbg_get_order called with empty slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  mtx=slv_get_sys_mtx(g_solvsys_cur);
  if (!mtx) {
    FPRINTF(ASCERR,"dbg_get_order found no mtx. odd!\n");
    Tcl_SetResult(interp, "dbg_get_order found no mtx. odd!", TCL_STATIC);
    return TCL_ERROR;
  }
  max=mtx_order(mtx);
  if (argv[1][0]=='r') {
      rfilter.matchbits = (REL_INCLUDED | REL_ACTIVE);
      rfilter.matchvalue = (REL_INCLUDED | REL_ACTIVE);
      max=slv_count_solvers_rels(g_solvsys_cur,&rfilter);
  }
  for (rc=0;rc<max;rc++) {
    switch (argv[1][0]) {
      case 'r':
        ndx=mtx_row_to_org(mtx,rc);
        break;
      case 'c':
        ndx=mtx_col_to_org(mtx,rc);
        break;
      default:
        ndx=(-1);
        break;
    }
    sprintf(&num[0],"%d",ndx);
    Tcl_AppendElement(interp,(char *)&num[0]);
  }
  return TCL_OK;
}

int Asc_DebuWriteIncidence(ClientData cdata, Tcl_Interp *interp,
                         int argc, CONST84 char *argv[])
{
  int tmpi,dev,status;
  FILE * fp;
  mtx_matrix_t mtx;
  mtx_coord_t nz;
  int32 order,bnum,maxrel;
  int32 *tmp;
  mtx_region_t reg;
  real64 value;
  struct rel_relation **rp;
  char *line = (char *)ascmalloc(32);

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,"call is: dbg_write_incidence <device#> \n");
    Tcl_SetResult(interp, "dbg_write_incidence wants 1 arg", TCL_STATIC);
    return TCL_ERROR;
  }
  if (!g_solvsys_cur) {
    FPRINTF(ASCERR,  "dbg_write_incidence called with NULL pointer\n");
    Tcl_SetResult(interp, "dbg_write_incidence called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  rp=slv_get_solvers_rel_list(g_solvsys_cur);
  if (!rp) {
    FPRINTF(ASCERR,  "dbg_write_incidence called with NULL rellist\n");
    Tcl_SetResult(interp,
                  "dbg_write_incidence called on system without rel list",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  maxrel=slv_get_num_solvers_rels(g_solvsys_cur);

  /* get io option */
  tmpi=4;
  status=Tcl_GetInt(interp,argv[1],&tmpi);
  if (tmpi<0 || tmpi >3) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(ASCERR,"dbg_write_incidence: first arg is 0,1, or 2\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "dbg_write_incidence: invalid output dev #",
                  TCL_STATIC);
    return status;
  } else {
    dev=tmpi;
  }
  switch (dev) {
  case 0: fp=stdout;
  case 3: fp=stdout;
    break;
  case 1: fp=ASCERR;
    break;
  case 2: fp=NULL;
    break;
  default : /* should never be here */
    FPRINTF(ASCERR,"dbg_write_incidence called with strange i/o option\n");
    return TCL_ERROR;
  }
  if (dev==3) { /* an unpublished option for DOF debugging */
    tmpi=slv_get_selected_solver(g_solvsys_cur);
    slv_select_solver(g_solvsys_cur,32767);
  }
  mtx=slv_get_sys_mtx(g_solvsys_cur);
  if (!mtx) {
    FPRINTF(ASCERR,"dbg_get_order found no linsol matrix. odd!\n");
    Tcl_SetResult(interp, "dbg_get_order found no linsol matrix. odd!",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (dev==3) {
    slv_select_solver(g_solvsys_cur,tmpi);
  }
  order = mtx_order(mtx);
  tmp = (int32 *)ascmalloc(order*sizeof(int32));
  for( nz.row=0 ; nz.row < order ; ++nz.row ) {
#if DP_DEBUG
    if (dev <2) {
      FPRINTF(fp,"org row %4d",mtx_row_to_org(mtx,nz.row));
    }
#endif
    if( mtx_output_assigned(mtx) ) {
      bnum=mtx_block_containing_row(mtx,nz.row,&reg);
    } else {
      bnum = mtx_row_to_org(mtx,nz.row);
      if ( bnum >= 0 && bnum < maxrel
           && rel_included(rp[bnum]) && rel_active(rp[bnum]) ) {
        bnum=0;
      } else {
        bnum=-1;
      }
    }
    if (dev<2) {
      FPRINTF(fp,"block %4d: ",bnum);
    } else {
      sprintf( line, " {%4d:", bnum );
      Tcl_AppendResult(interp,line,SNULL);
    }
    mtx_zero_int32(tmp,order);
    nz.col=mtx_FIRST;
    while ( value=mtx_next_in_row(mtx,&nz,mtx_ALL_COLS), nz.col!=mtx_LAST) {
      tmp[nz.col] = 1;
    }
    for( nz.col=0; nz.col<order; nz.col++ ) {
      if (dev<2) {
        FPRINTF(fp,tmp[nz.col]? "X ": ". ");
      } else {
        Tcl_AppendResult(interp,(tmp[nz.col]? "X": "."),SNULL);
      }
    }
    if (dev<2) {
      PUTC('\n',fp);
    } else {
      Tcl_AppendResult(interp,"}\n",SNULL);
    }
  }
  ascfree(tmp);
  ascfree(line);
  return (status);
}

int Asc_DebuFindEligible(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  int32 *vip=NULL;
  char tmps[MAXIMUM_NUMERIC_LENGTH];
  int i,dev,status,len;
  FILE *fp;
  struct var_variable **vp;
  symchar *message;
  symchar *eligible;
  symchar *none;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,  "call is: dbg_find_eligible <out>\n");
    Tcl_SetResult(interp, "dbg_find_eligible wants output device.",TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "dbg_find_eligible called with NULL pointer\n");
    Tcl_SetResult(interp, "dbg_find_eligible called without slv_system",
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
    FPRINTF(ASCERR,"dbg_find_eligible: first arg is 0,1, or 2\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp,"dbg_find_eligible: invalid output dev #",TCL_STATIC);
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
    FPRINTF(ASCERR,"dbg_find_eligible called with strange i/o option\n");
    return TCL_ERROR;
  }
  eligible = AddSymbolL("eligible",8);
  message = AddSymbolL("message",7);
  none = AddSymbolL("none",4);
  len = slv_get_num_solvers_vars(g_solvsys_cur);
  vp = slv_get_solvers_var_list(g_solvsys_cur);
    for (i=0; i < len; i++) {
    Asc_BrowSetAtomAttribute(interp,(struct Instance *)var_instance(vp[i]),
                             message,SYMBOL_INST,&none);
  } 
  if (slvDOF_eligible(g_solvsys_cur,&vip)) {
    switch (dev) {
    case 0:
    case 1:
      FPRINTF(fp,"Degrees of freedom variable indices (fixable):\n");
      for (i=0;vip[i]>-1;i++) {
        FPRINTF(fp,"%d\n",vip[i]);
      }
      break;
    case 2:
      Tcl_AppendResult(interp,"{",SNULL);
      for (i=0;vip[i]>-1;i++) {
        sprintf(tmps,"%d ",vip[i]);
        Tcl_AppendResult(interp,tmps,SNULL);
      }
      Tcl_AppendResult(interp,"}",SNULL);
      break;
    default:
      FPRINTF(ASCERR,"wierdness in i/o!");
      break;
    }
    for (i=0;vip[i]>-1;i++) {
      Asc_BrowSetAtomAttribute(interp,var_instance(vp[vip[i]]),
                               message,SYMBOL_INST,&eligible);
    }
    ascfree(vip);
  } else {
    Tcl_SetResult(interp, "{}", TCL_STATIC);
  }
  return TCL_OK;
}

int Asc_DebuInstEligible(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  int32 *vip=NULL;
  struct var_variable **vp;
  char *tmps = NULL;
  int i,dev,status;
  enum inst_t ikind;
  unsigned long pc;
  FILE *fp;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,  "call is: brow_find_eligible <out>\n");
    Tcl_SetResult(interp,"brow_find_eligible wants output device.",TCL_STATIC);
    return TCL_ERROR;
  }
  if (!g_root) {
    FPRINTF(ASCERR,"brow_find_eligible: called without sim in browser.\n");
    Tcl_SetResult(interp, "focus browser before calling brow_find_eligible",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  ikind=InstanceKind(g_curinst);
  if (ikind!=MODEL_INST) {
    FPRINTF(ASCERR,  "Instance examined is not a solvable kind.\n");
    Tcl_SetResult(interp, "Instance kind not MODEL.", TCL_STATIC);
    return TCL_ERROR;
  }
  if ((pc=NumberPendingInstances(g_curinst))!=0) {
    FPRINTF(ASCERR,  "Instance examined is incomplete: %ld pendings.\n",pc);
    Tcl_SetResult(interp, "Instance has pendings: Not solvable.", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_browsys_cur != NULL) {
    system_destroy(g_browsys_cur);
    g_browsys_cur = NULL;
  }

  /* get io option */
  i=3;
  status=Tcl_GetInt(interp,argv[1],&i);
  if (i<-1 || i >2) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(ASCERR,"brow_find_eligible: first arg is -1,0,1, or 2\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "brow_find_eligible: invalid output dev #",
                  TCL_STATIC);
    return status;
  } else {
    dev=i;
  }
  switch (dev) {
  case -1:
    Tcl_SetResult(interp, "{}", TCL_STATIC);
    return TCL_OK;
  case 0:
    fp=stdout;
    break;
  case 1:
    fp=ASCERR;
    break;
  case 2:
    fp=NULL;
    break;
  default : /* should never be here */
    FPRINTF(ASCERR,"brow_find_eligible called with strange i/o option\n");
    return TCL_ERROR;
  }
  g_browsys_cur = system_build(g_curinst);
  if( g_browsys_cur == NULL ) {
    FPRINTF(ASCERR,"system_build returned NULL.\n");
    Tcl_SetResult(interp, "Bad relations found: DOF system not created.",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  if (slvDOF_eligible(g_browsys_cur,&vip)) {
    vp = slv_get_solvers_var_list(g_browsys_cur);
    switch (dev) {
    case 0:
    case 1:
      FPRINTF(fp,"Degrees of freedom variables (fixable):\n");
      if (vip[0] < 0) {
        FPRINTF(fp,"  None.\n");
      }
      for (i=0; vip[i] > -1; i++) {
        FPRINTF(fp,"  ");
        var_write_name(g_browsys_cur,vp[vip[i]],fp);
        FPRINTF(fp,"\n");
      }
      break;
    case 2:
      Tcl_AppendResult(interp,"{",SNULL);
      for (i=0;vip[i]>-1;i++) {
        tmps = var_make_name(g_browsys_cur,vp[vip[i]]);
        Tcl_AppendResult(interp,"{",tmps,"}",SNULL);
        ascfree(tmps);
        tmps = NULL;
        if (vip[i+1] > -1) {
          Tcl_AppendResult(interp," ",SNULL);
        }
      }
      Tcl_AppendResult(interp,"}",SNULL);
      break;
    default:
      FPRINTF(ASCERR,"wierdness in i/o!");
      break;
    }
    if (vip) {
      ascfree(vip);
    }
  } else {
    Tcl_SetResult(interp, "{}", TCL_STATIC);
  }
  system_destroy(g_browsys_cur);
  g_browsys_cur = NULL;
  return TCL_OK;
}

/*
 * Get a "globally" (several alternatives in conditional model) consistent
 * set of variables to be fixed which, if fixed, would let all the 
 * alternatives square and structurally consistent
 * 
 */
int Asc_DebuConsistencyAnalysis(ClientData cdata, Tcl_Interp *interp,
                                int argc, CONST84 char *argv[])
{
  int32 *vip=NULL;
  char tmps[MAXIMUM_NUMERIC_LENGTH];
  int i,dev,status,len;
  FILE *fp;
  struct var_variable **vp;
  symchar *message;
  symchar *consistent;
  symchar *none;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,  "call is: dbg_consistency_analysis <out>\n");
    Tcl_SetResult(interp, "dbg_consistency_analysis wants output device.",
		  TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "cdbg_consistency_analysis alled with NULL pointer\n");
    Tcl_SetResult(interp, "dbg_consistency_analysis called without slv_system",
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
    FPRINTF(ASCERR,":dbg_consistency_analysis first arg is 0,1, or 2\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp,"dbg_consistency_analysis: invalid output dev #",
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
    FPRINTF(ASCERR,
	    "dbg_consistency_analysis called with strange i/o option\n");
    return TCL_ERROR;
  }
  consistent = AddSymbolL("consistent",10);
  message = AddSymbolL("message",7);
  none = AddSymbolL("none",4);
  len = slv_get_num_master_vars(g_solvsys_cur);
  vp = slv_get_master_var_list(g_solvsys_cur);
  for (i=0; i < len; i++) {
    Asc_BrowSetAtomAttribute(interp,(struct Instance *)var_instance(vp[i]),
                             message,SYMBOL_INST,&none);
  } 
  if (consistency_analysis(g_solvsys_cur,&vip)) {
    switch (dev) {
    case 0:
    case 1:
      FPRINTF(fp,"Consistent set of fixable variables:\n");
      for (i=0;vip[i]>-1;i++) {
        FPRINTF(fp,"%d\n",vip[i]);
      }
      break;
    case 2:
      Tcl_AppendResult(interp,"{",SNULL);
      for (i=0;vip[i]>-1;i++) {
        sprintf(tmps,"%d ",vip[i]);
        Tcl_AppendResult(interp,tmps,SNULL);
      }
      Tcl_AppendResult(interp,"}",SNULL);
      break;
    default:
      FPRINTF(ASCERR,"wierdness in i/o!");
      break;
    }
    for (i=0;vip[i]>-1;i++) {
      Asc_BrowSetAtomAttribute(interp,var_instance(vp[vip[i]]),
                               message,SYMBOL_INST,&consistent);
    } 
    ascfree(vip);
  } else {
    Tcl_SetResult(interp, "{}", TCL_STATIC);
  }
  return TCL_OK;
}

/*
 * Get a set of eligible variables common to all the alternatives in
 * the problem
 */
int Asc_DebuFindGlobalEligible(ClientData cdata, Tcl_Interp *interp,
                               int argc, CONST84 char *argv[])
{
  int32 *vip=NULL;
  char tmps[MAXIMUM_NUMERIC_LENGTH];
  int i,dev,status,len;
  FILE *fp;
  struct var_variable **vp;
  symchar *message;
  symchar *eligible;
  symchar *none;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,  "call is: dbg_global_eligible <out>\n");
    Tcl_SetResult(interp, "dbg_global_eligible wants output device.",
		  TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "dbg_global_eligible called with NULL pointer\n");
    Tcl_SetResult(interp, "dbg_global_eligible called without slv_system",
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
    FPRINTF(ASCERR,":dbg_global_eligible first arg is 0,1, or 2\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp,"dbg_global_eligible: invalid output dev #",
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
    FPRINTF(ASCERR,
	    "dbg_global_eligible called with strange i/o option\n");
    return TCL_ERROR;
  }
  eligible = AddSymbolL("g_eligible",10);
  message = AddSymbolL("message",7);
  none = AddSymbolL("none",4);
  len = slv_get_num_master_vars(g_solvsys_cur);
  vp = slv_get_master_var_list(g_solvsys_cur);
  for (i=0; i < len; i++) {
    Asc_BrowSetAtomAttribute(interp,(struct Instance *)var_instance(vp[i]),
                             message,SYMBOL_INST,&none);
  }
  if (get_globally_consistent_eligible(g_solvsys_cur,&vip)) {
    switch (dev) {
    case 0:
    case 1:
      FPRINTF(fp,"Set of globally eligible variables:\n");
      for (i=0;vip[i]>-1;i++) {
        FPRINTF(fp,"%d\n",vip[i]);
      }
      break;
    case 2:
      Tcl_AppendResult(interp,"{",SNULL);
      for (i=0;vip[i]>-1;i++) {
        sprintf(tmps,"%d ",vip[i]);
        Tcl_AppendResult(interp,tmps,SNULL);
      }
      Tcl_AppendResult(interp,"}",SNULL);
      break;
    default:
      FPRINTF(ASCERR,"wierdness in i/o!");
      break;
    }
        for (i=0;vip[i]>-1;i++) {
      Asc_BrowSetAtomAttribute(interp,var_instance(vp[vip[i]]),
                               message,SYMBOL_INST,&eligible);
    }
    ascfree(vip);
  } else {
    Tcl_SetResult(interp, "{}", TCL_STATIC);
  }
  return TCL_OK;
}

/*
 * Find Active relations in the current solver system
 */
int Asc_DebuFindActive(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  FILE *fp;
  struct rel_relation **rp;
  struct rel_relation *rel;
  rel_filter_t rfilter;
  char tmps[MAXIMUM_NUMERIC_LENGTH];
  symchar *message;
  symchar *active;
  symchar *none;
  int32 *rip;
  int i,dev,status;
  int count,len,aclen;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,  "call is: dbg_find_activerels <out>\n");
    Tcl_SetResult(interp, "dbg_find_activerels wants output device.",
		  TCL_STATIC);
    return TCL_ERROR;
  }

  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,"dbg_find_activerels called with NULL pointer\n");
    Tcl_SetResult(interp, "dbg_find_activerels called without slv_system",
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
    FPRINTF(ASCERR,"dbg_find_activerels: first arg is 0,1, or 2\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp,"dbg_find_activerels: invalid output dev #",
		  TCL_STATIC);
    return status;
  } else {
    dev=i;
  }
  switch (dev) {
  case 0: fp=NULL;
    break;
  case 1: fp=ASCERR;
    break;
  case 2: fp=NULL;
    break;
  default : /* should never be here */
    FPRINTF(ASCERR,"dbg_find_activerels called with strange i/o option\n");
    return TCL_ERROR;
  }

  active = AddSymbolL("active",6);
  message = AddSymbolL("message",7);
  none = AddSymbolL("none",4);

  rfilter.matchbits = (REL_ACTIVE);
  rfilter.matchvalue = (REL_ACTIVE);

  rp = slv_get_solvers_rel_list(g_solvsys_cur);
  len = slv_get_num_solvers_rels(g_solvsys_cur);
  aclen = slv_count_solvers_rels(g_solvsys_cur,&rfilter);
  rip = (int32 *)ascmalloc(aclen*sizeof(int32));

  count =0;
  for (i=0; i < len; i++) {
    rel = rp[i];
    if (rel_apply_filter(rel,&rfilter)) {
      Asc_BrowSetAtomAttribute(interp,(struct Instance *)rel_instance(rel),
                               message,SYMBOL_INST,&active);
      rip[count] = i;
      count++;
    } else {
     Asc_BrowSetAtomAttribute(interp,(struct Instance *)rel_instance(rel),
                               message,SYMBOL_INST,&none);
    }
  }

  if (aclen>0) {
    switch (dev) {
      case 0:
        break;
      case 1:
        FPRINTF(fp,"Active relation indices:\n");
        for (i=0;i<aclen;i++) {
          FPRINTF(fp,"%d\n",rip[i]);
        }
        break;
      case 2:
        Tcl_AppendResult(interp,"{",SNULL);
        for (i=0;i<aclen;i++) {
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


/*
 * Find Active relations in the instance selected in the browser
 */
int Asc_DebuInstActive(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  FILE *fp;
  struct rel_relation **rp;
  struct rel_relation *rel;
  char *tmps = NULL;
  rel_filter_t rfilter;
  enum inst_t ikind;
  unsigned long pc;
  int i,dev,status,len,count,aclen;
  int32 *rip;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    FPRINTF(ASCERR,"call is: brow_find_activerels <out>\n");
    Tcl_SetResult(interp,"brow_find_activerels wants output device.",
		  TCL_STATIC);
    return TCL_ERROR;
  }

  if (!g_root) {
    FPRINTF(ASCERR,"brow_find_activerels: called without sim in browser.\n");
    Tcl_SetResult(interp, "focus browser before calling brow_find_activerels",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  ikind=InstanceKind(g_curinst);
  if (ikind!=MODEL_INST) {
    FPRINTF(ASCERR,  "Instance examined is not a solvable kind.\n");
    Tcl_SetResult(interp, "Instance kind not MODEL.", TCL_STATIC);
    return TCL_ERROR;
  }
  if ((pc=NumberPendingInstances(g_curinst))!=0) {
    FPRINTF(ASCERR,  "Instance examined is incomplete: %ld pendings.\n",pc);
    Tcl_SetResult(interp, "Instance has pendings: Not solvable.", TCL_STATIC);
    return TCL_ERROR;
  }

  if (g_browsys_cur != NULL) {
    system_destroy(g_browsys_cur);
    g_browsys_cur = NULL;
  }

  /* get io option */
  i=3;
  status=Tcl_GetInt(interp,argv[1],&i);
  if (i<-1 || i >2) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(ASCERR,"brow_find_activerels: first arg is -1,0,1, or 2\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "brow_find_activerels: invalid output dev #",
                  TCL_STATIC);
    return status;
  } else {
    dev=i;
  }
  switch (dev) {
  case -1:
    Tcl_SetResult(interp, "{}", TCL_STATIC);
    return TCL_OK;
  case 0:
    fp=stdout;
    break;
  case 1:
    fp=ASCERR;
    break;
  case 2:
    fp=NULL;
    break;
  default : /* should never be here */
    FPRINTF(ASCERR,"brow_find_activerels called with strange i/o option\n");
    return TCL_ERROR;
  }

  g_browsys_cur = system_build(g_curinst);
  if( g_browsys_cur == NULL ) {
    FPRINTF(ASCERR,"system_build returned NULL.\n");
    Tcl_SetResult(interp, "Bad relations found: DOF system not created.",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  rfilter.matchbits = (REL_ACTIVE);
  rfilter.matchvalue = (REL_ACTIVE);

  rp = slv_get_solvers_rel_list(g_browsys_cur);
  len = slv_get_num_solvers_rels(g_browsys_cur);
  aclen = slv_count_solvers_rels(g_browsys_cur,&rfilter);
  rip = (int32 *)ascmalloc(aclen*sizeof(int32));

  count =0;
  for (i=0; i < len; i++) {
    rel = rp[i];
    if (rel_apply_filter(rel,&rfilter)) {
      rip[count] = i;
      count++;
    }
  }

  if (aclen>0) {
    switch (dev) {
      case 0:
      case 1:
        FPRINTF(fp,"Active relations :\n");
        if (aclen == 0) {
          FPRINTF(fp,"  None.\n");
        }
        for (i=0; i<count; i++) {
          FPRINTF(fp,"  ");
          rel_write_name(g_browsys_cur,rp[rip[i]],fp);
          FPRINTF(fp,"\n");
        }
        break;
      case 2:
        Tcl_AppendResult(interp,"{",SNULL);
        for (i=0;i<count;i++) {
          tmps = rel_make_name(g_browsys_cur,rp[rip[i]]);
          Tcl_AppendResult(interp,"{",tmps,"}",SNULL);
          ascfree(tmps);
          tmps = NULL;
          if (i < count -1) {
            Tcl_AppendResult(interp," ",SNULL);
	  }
        }
        Tcl_AppendResult(interp,"}",SNULL);
        break;
      default:
        FPRINTF(ASCERR,"wierdness in i/o!");
        break;
      }

    if (rip) {
      ascfree(rip);
    }
  } else {
    Tcl_SetResult(interp, "{}", TCL_STATIC);
  }

  system_destroy(g_browsys_cur);
  g_browsys_cur = NULL;
  return TCL_OK;
}


/*
 *  Calculates the given region of the jacobian.  It is unscaled.
 *  var/rel _in_block flags will be set based on the region.
 *  returns calc_ok value.
 */
static boolean dbg_calc_jacobian(mtx_matrix_t mtx,
                                 mtx_region_t reg,
                                 struct rel_relation **rlist,
                                 struct var_variable **vlist)
{
  int32 row,col,maxrel,maxvar,c;
  var_filter_t vfilter;
  struct rel_relation *rel;
  struct var_variable *var;
  struct var_variable **vp;
  struct rel_relation **rp;
  double resid;

  calc_ok = TRUE;
  vfilter.matchbits = (VAR_INBLOCK | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INBLOCK | VAR_ACTIVE);
  mtx_clear_region(mtx,&reg);

  vp=vlist;
  rp=rlist;
  maxrel=slv_get_num_solvers_rels(g_solvsys_cur);
  maxvar=slv_get_num_solvers_vars(g_solvsys_cur);

  for (c=0;c<maxvar; c++) {
    var_set_in_block(vp[c],FALSE);
  }
  for (c=0;c<maxrel; c++) {
    rel_set_in_block(rp[c],FALSE);
  }

  for( col=reg.col.low; col <= reg.col.high; col++ ) {
     var = vlist[mtx_col_to_org(mtx,col)];
     var_set_in_block(var,TRUE);
  }
  for( row=reg.row.low; row <=reg.row.high; row++ ) {
    rel = rlist[mtx_row_to_org(mtx,row)];
    rel_set_in_block(rel,TRUE);
  }

  for( row = reg.row.low; row <= reg.row.high; row++ ) {
     rel = rlist[mtx_row_to_org(mtx,row)];
     relman_diffs(rel,&vfilter,mtx,&resid,SAFE_FIX_ME);

     /* added */
     rel_set_residual(rel,resid);
  }
  return(calc_ok);
}


#ifdef THIS_MAY_BE_UNUSED_CODE
static void dbg_invert_block(linsol_system_t lsys,
                             mtx_region_t *reg,
                             mtx_matrix_t mtx,
                             struct rel_relation **rp,
                             struct var_variable **vp) {
  int status=1;
  linsol_matrix_was_changed(lsys);
  status=dbg_calc_jacobian(mtx,*reg,rp,vp);
  if (!status) {
    FPRINTF(ASCERR,"Error in jacobian calculation: attempting check anyway.");
  }
  calc_ok=TRUE;
  linsol_reorder(lsys,reg);
  linsol_invert(lsys,reg);
}
#endif


static void dbg_factor_block(linsolqr_system_t lsys,
                             mtx_region_t *reg,
                             mtx_matrix_t mtx,
                             struct rel_relation **rp,
                             struct var_variable **vp) {
  int status=1, oldtiming;
  enum factor_method fmethod;
  enum reorder_method rmethod;
  linsolqr_matrix_was_changed(lsys);
  status=dbg_calc_jacobian(mtx,*reg,rp,vp);
  if (!status) {
    FPRINTF(ASCERR,"Error in jacobian calculation: attempting check anyway.");
  }
  calc_ok = TRUE;
  fmethod = linsolqr_fmethod(lsys);
  if (fmethod == 0) { /* Unknown factorization method. */
    FPRINTF(ASCERR,"factorization method = %s\n",
      linsolqr_fmethod_description(fmethod));
    fmethod = ranki_jz2;
    FPRINTF(ASCERR,"Setting factorization method = %s\n",
      linsolqr_fmethod_description(fmethod));
  }
  rmethod = linsolqr_rmethod(lsys);
  if ( strcmp(linsolqr_enum_to_rmethod(rmethod),"Unknown reordering method.")
       == 0) {
    FPRINTF(ASCERR,"Reorder method = %s\n",
            linsolqr_rmethod_description(rmethod));
    rmethod = spk1;
    FPRINTF(ASCERR,"Setting reorder method = %s\n",
            linsolqr_rmethod_description(rmethod));
  }
  linsolqr_reorder(lsys,reg,rmethod);
  oldtiming = g_linsolqr_timing;
  g_linsolqr_timing =0;
  linsolqr_factor(lsys,fmethod);
  g_linsolqr_timing =oldtiming;
}

int Asc_DebuNumBlockSing(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  struct rel_relation **rp;
  struct var_variable **vp;
  dof_t *d;
  const mtx_block_t *b;
  linsolqr_system_t lsys;
  int32 nr,nv,u,p,numblocks,cur_block;
  mtx_region_t region;
  mtx_matrix_t mtx;
  slv_status_t ss;
  int i,dev,status,rc;
  mtx_sparse_t *singrows = NULL, *singcols = NULL,
               *rowcoefs = NULL, *colcoefs = NULL;
  char tmps[MAXIMUM_NUMERIC_LENGTH];
  FILE *fp;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 4 ) {
    FPRINTF(ASCERR,
      "call is: dbg_num_block_singular <out#> <block#> <row,col>\n");
    Tcl_SetResult(interp,
                  "dbg_num_block_singular wants output dev & row or col.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "dbg_num_block_singular called with NULL pointer\n");
    Tcl_SetResult(interp, "dbg_num_block_singular called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  slv_get_status(g_solvsys_cur,&ss);
  rp=slv_get_solvers_rel_list(g_solvsys_cur);
  if (!rp) {
    FPRINTF(ASCERR,  "NULL relation list found in dbg_num_block_singular\n");
    Tcl_SetResult(interp, "dbg_num_block_singular called with null rellist",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  lsys = slv_get_linsolqr_sys(g_solvsys_cur);
  if (!lsys) {
    FPRINTF(ASCERR,  "NULL linsolqr sys found in dbg_num_singular\n");
    Tcl_SetResult(interp,
                  "dbg_num_block_singular called with null linsolqr sys",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  mtx = linsolqr_get_matrix(lsys);
  d = slv_get_dofdata(g_solvsys_cur);
  b = slv_get_solvers_blocks(g_solvsys_cur);
  numblocks = b->nblocks;

  if (!numblocks) {
    FPRINTF(ASCERR,  "dbg_num_block_singular: mtx not assigned yet.\n");
    Tcl_SetResult(interp, "dbg_num_block_singular called before presolve.",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  vp=slv_get_solvers_var_list(g_solvsys_cur);
  if (!vp) {
    FPRINTF(ASCERR,  "NULL variable list found in dbg_num_singular\n");
    Tcl_SetResult(interp, "dbg_num_block_singular called with null varlist",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  nr=slv_get_num_solvers_rels(g_solvsys_cur);
  nv=slv_get_num_solvers_vars(g_solvsys_cur);

  /* get io option */
  i=3;
  status=Tcl_GetInt(interp,argv[1],&i);
  if (i<0 || i >2) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(ASCERR,"dbg_num_block_singular: first arg is 0,1, or 2\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "dbg_num_block_singular: invalid output dev #",
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
       FPRINTF(ASCERR,
        "dbg_num_block_singular called with strange i/o option\n");
       return TCL_ERROR;
  }
  /* get block number */
  i=-1;
  status=Tcl_GetInt(interp,argv[2],&i);
  if (i<0 || i >= numblocks) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(ASCERR,"dbg_num_block_singular: second arg is a block number");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "dbg_num_block_singular: invalid block #",
                  TCL_STATIC);
    return status;
  } else {
    cur_block=i;
  }
  region = b->block[cur_block];
  linsolqr_set_region(lsys,region);
  if (setjmp(g_fpe_env)==0) {
    dbg_factor_block(lsys,&region,mtx,rp,vp);
  } else {
    FPRINTF(ASCERR, "Floating point exception in dbg_num_block_singular.\n");
    Tcl_SetResult(interp, " Float error in dbg_num_block_singular. ",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  switch (argv[3][0]) {
    case 'r':
      rc=0;
      break;
    case 'c':
      rc=1;   /* if want col dependency instead, rc is 1 */
      break;
    default:
      Tcl_SetResult(interp,
                    "dbg_num_block_singular:second arg is \"row\" or \"col\"",
                    TCL_STATIC);
      return TCL_ERROR;
  }
  if (!rc) {
    if (dev!=2) {
      FPRINTF(fp,"Checking block %d for numeric row dependency.\n",cur_block);
    }
    linsolqr_calc_row_dependencies(lsys);
    singrows = linsolqr_unpivoted_rows(lsys);
    if (singrows != NULL) {
      for (u = 0; u < singrows->len; u++) {
        if (dev==2) {
          sprintf(tmps,"{%d ",u);
          Tcl_AppendResult(interp,tmps,SNULL);
        } else {
          FPRINTF(fp,"Unpivoted row %d sum of:\n",singrows->idata[u]);
        }
        rowcoefs = linsolqr_row_dependence_coefs(lsys,singrows->idata[u]);
        for (p = 0; p < rowcoefs->len; p++) {
          if (dev==2) {
            sprintf(tmps,"{%d %.16g} ",rowcoefs->idata[p],rowcoefs->data[p]);
            Tcl_AppendResult(interp,tmps,SNULL);
          } else {
            FPRINTF(fp,"Row(%d) * %.16g\n",rowcoefs->idata[p],
              rowcoefs->data[p]);
          }
        }
        if (dev==2) {
          sprintf(tmps,"} ");
          Tcl_AppendResult(interp,tmps,SNULL);
        } else {
          FPRINTF(fp,"\n");
        }
      }
    }
    if (dev!=2) {
      FPRINTF(fp,"All rows checked.\n");
    }
  } else {
    if (dev!=2) {
    FPRINTF(fp,"Checking block %d for numeric column dependency.\n",cur_block);
    }
    linsolqr_calc_col_dependencies(lsys);
    singcols = linsolqr_unpivoted_cols(lsys);
    if ( singcols != NULL) {
      for (u = 0; u < singcols->len; u++) {
        if (dev==2) {
          sprintf(tmps,"{%d ",u);
          Tcl_AppendResult(interp,tmps,SNULL);
        } else {
          FPRINTF(fp,"Unpivoted column %d sum of:",singrows->idata[u]);
        }
        colcoefs = linsolqr_col_dependence_coefs(lsys,singcols->idata[u]);
        for (p = 0; p < colcoefs->len; p++) {
          if (dev==2) {
            sprintf(tmps,"{%d %.16g} ",colcoefs->idata[p],colcoefs->data[p]);
            Tcl_AppendResult(interp,tmps,SNULL);
          } else {
            FPRINTF(fp,"Column(%d) * %.16g\n",colcoefs->idata[p],
               colcoefs->data[p]);
          }
        }
      }
      if (dev==2) {
        sprintf(tmps,"} ");
        Tcl_AppendResult(interp,tmps,SNULL);
      } else {
        FPRINTF(fp,"\n");
      }
    }
    if (dev!=2) {
      FPRINTF(fp,"All columns checked.\n");
    }
  }
  mtx_destroy_sparse(singrows);
  mtx_destroy_sparse(singcols);
  mtx_destroy_sparse(rowcoefs);
  mtx_destroy_sparse(colcoefs);
  return TCL_OK;
}


int Asc_DebuStructSing(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  int32 *rip=NULL, *vip=NULL, *fip=NULL;
  struct rel_relation **rp;
  struct var_variable **vp;
  int i,dev,status;
  int32 relnum,maxrel;
  FILE *fp;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 3 ) {
    FPRINTF(ASCERR,  "call is: dbg_struct_singular <out> <relindex,-1>\n");
    Tcl_SetResult(interp,
                  "dbg_struct_singular wants output dev & relation index.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "dbg_struct_singular called with NULL pointer\n");
    Tcl_SetResult(interp, "dbg_struct_singular called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  rp=slv_get_solvers_rel_list(g_solvsys_cur);
  vp=slv_get_solvers_var_list(g_solvsys_cur);
  if (!rp) {
    FPRINTF(ASCERR,  "NULL relation list found in dbg_struct_singular\n");
    Tcl_SetResult(interp, "dbg_struct_singular called with null rellist",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (!vp) {
    FPRINTF(ASCERR,  "NULL variable list found in dbg_struct_singular\n");
    Tcl_SetResult(interp, "dbg_struct_singular called with null rellist",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  maxrel=slv_get_num_solvers_rels(g_solvsys_cur);
  relnum = maxrel;
  status = Tcl_GetInt(interp,argv[2],&relnum);
  if (relnum >= maxrel || status == TCL_ERROR) {
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp,
                  "dbg_struct_singular: equation checked does not exist",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (relnum < 0) {
    relnum = mtx_FIRST;
  }
  /* get io option */
  i=3;
  status=Tcl_GetInt(interp,argv[1],&i);
  if (i<0 || i >2) {
    status=TCL_ERROR;
  }
  if (status!=TCL_OK) {
    FPRINTF(ASCERR,"dbg_struct_singular: first arg is 0,1, or 2\n");
    Tcl_ResetResult(interp);
    Tcl_SetResult(interp, "dbg_struct_singular: invalid output dev #",
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
       FPRINTF(ASCERR,"dbg_struct_singular called with strange i/o option\n");
            return TCL_ERROR;
  }
  if (slvDOF_structsing(g_solvsys_cur,relnum,&vip,&rip,&fip)) {
    char tmps[MAXIMUM_NUMERIC_LENGTH];
    switch (dev) {
      case 0:
      case 1:
        FPRINTF(fp,"Relations in structural singularity:\n");
        if (rip[0] < 0) {
          FPRINTF(fp,"  None.\n");
        }
        for (i=0; rip[i] > -1; i++) {
          FPRINTF(fp,"  ");
          rel_write_name(g_solvsys_cur,rp[rip[i]],fp);
          FPRINTF(fp,"\n");
        }
        FPRINTF(fp,"Variables in structural singularity:\n");
        if (vip[0] < 0) {
          FPRINTF(fp,"  None.\n");
        }
        for (i=0; vip[i] > -1; i++) {
          FPRINTF(fp,"  ");
          var_write_name(g_solvsys_cur,vp[vip[i]],fp);
          FPRINTF(fp,"\n");
        }

        FPRINTF(fp,"Variables reducing structural singularity if freed:\n");
        if (fip[0] < 0) {
          FPRINTF(fp,"  None.\n");
        }
        for (i=0; fip[i] > -1; i++) {
          FPRINTF(fp,"  ");
          var_write_name(g_solvsys_cur,vp[fip[i]],fp);
          FPRINTF(fp,"\n");
        }
        break;
      case 2:
        Tcl_AppendResult(interp,"{",SNULL);
        for (i=0;rip[i]>-1;i++) {
          sprintf(tmps,"%d ",rip[i]);
          Tcl_AppendResult(interp,tmps,SNULL);
        }
        Tcl_AppendResult(interp,"} {",SNULL);
        for (i=0;vip[i]>-1;i++) {
          sprintf(tmps,"%d ",vip[i]);
          Tcl_AppendResult(interp,tmps,SNULL);
        }
        Tcl_AppendResult(interp,"} {",SNULL);
        for (i=0;fip[i]>-1;i++) {
          sprintf(tmps,"%d ",fip[i]);
          Tcl_AppendResult(interp,tmps,SNULL);
        }
        Tcl_AppendResult(interp,"}",SNULL);
        break;
      default:
        FPRINTF(ASCERR,"wierdness in i/o!");
        break;
    }
    if (vip) {
      ascfree(vip);
    }
    if (rip) {
      ascfree(rip);
    }
    if (fip) {
      ascfree(fip);
    }
  } else {
    Tcl_SetResult(interp, "{} {} {}", TCL_STATIC);
  }
  return TCL_OK;
}
int Asc_DebuVarFree2Nom(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  struct var_variable **vp;
  var_filter_t vfilter;
  int32 c,maxvar;

  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    FPRINTF(ASCERR,  "call is: var_free2nom <no args>\n");
    Tcl_SetResult(interp, "var_free2nom takes no arguments.", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "var_free2nom called with NULL pointer\n");
    Tcl_SetResult(interp, "var_free2nom called without slv_system",TCL_STATIC);
    return TCL_ERROR;
  }
  vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_ACTIVE);

  vp=slv_get_solvers_var_list(g_solvsys_cur);
  maxvar=slv_get_num_solvers_vars(g_solvsys_cur);

  for (c=0; c<maxvar; c++) { /*reset vars */
     if (var_apply_filter(vp[c],&vfilter)) {
        var_set_value(vp[c],var_nominal(vp[c]));
        /* FPRINTF(ASCERR,"%g\n",var_nominal(vp[c])); */
     }
  }

  return TCL_OK;
}

int Asc_DebuVarNom2Free(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{
  struct var_variable **vp;
  var_filter_t vfilter;
  int32 maxvar,c;

  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    FPRINTF(ASCERR,  "call is: var_nom2free <no args>\n");
    Tcl_SetResult(interp, "var_nom2free takes no arguments.", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "var_nom2free called with NULL pointer\n");
    Tcl_SetResult(interp, "var_nomfree called without slv_system", TCL_STATIC);
    return TCL_ERROR;
  }
  vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_ACTIVE);

  vp=slv_get_solvers_var_list(g_solvsys_cur);
  maxvar=slv_get_num_solvers_vars(g_solvsys_cur);

  for (c=0; c<maxvar; c++) {  /*reset vars */
    if (var_apply_filter(vp[c],&vfilter)) {
      var_set_nominal(vp[c],var_value(vp[c]));
      /* FPRINTF(ASCERR,"%g\n",var_value(vp[c])); */
    }
  }

  return TCL_OK;
}

/* since an fp error -> badness to automatic variables, and
since hp doesn't handle this as robustly as the decs tend to,
these little functions exist to isolate the chaos involved
in the jmps due to float errors.
*/
static int dbg_calc_nominal(struct rel_relation *rel) {
  double nom;
  enum Expr_enum dummy;
  if (setjmp(g_fpe_env)==0) {
    nom = CalcRelationNominal(rel_instance(rel));
    if (nom >0.0) {
      SetRelationNominal(
        (struct relation *)GetInstanceRelation(rel_instance(rel),&dummy), nom);
    }
    return 0;
  } else {
    return 2;
  }
}

int Asc_DebuCheckRelFp(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  struct rel_relation **rp,*rel;
  struct var_variable **vp;
  int32 i,maxrel;
  int status;
  struct Instance *rinst;
  char tmps[MAXIMUM_NUMERIC_LENGTH+1];

  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    FPRINTF(ASCERR,"call is: dbg_check_rels <no args>\n");
    Tcl_SetResult(interp, "dbg_check_rels wants no args", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "dbg_check_rels called with NULL pointer\n");
    Tcl_SetResult(interp, "dbg_check_rels called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  vp=slv_get_solvers_var_list(g_solvsys_cur);
  rp=slv_get_solvers_rel_list(g_solvsys_cur);
  maxrel=slv_get_num_solvers_rels(g_solvsys_cur);

  if (!vp || !rp ) {
    FPRINTF(ASCERR,  "dbg_check_rels called with NULL rel or var list\n");
    Tcl_SetResult(interp, "dbg_check_rels called without rels or vars",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
/* convert any int to a 0/1 int */
#define ISTRUE(a) ((a)!=0)
  for (i=0; i<maxrel; i++) {
    rel=rp[i];
    rinst =(struct Instance *)rel_instance(rel);
    status = RelationCalcExceptionsInfix(rinst);
    if (status != RCE_OK && status != RCE_BADINPUT) {
      sprintf(tmps,"%d %d %d %d %d",i,
        ISTRUE(RCE_ERR_LHS & status),
        ISTRUE(RCE_ERR_RHS & status),
        ISTRUE(RCE_ERR_LHSGRAD & status),
        ISTRUE(RCE_ERR_RHSGRAD & status));
      Tcl_AppendElement(interp, tmps);
    }
  }
  Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
#undef ISTRUE
/* if external relations special case. don't know what yet.
 * But the compiler while whine accordingly.
 */

  return TCL_OK;
}

int Asc_DebuCalcRelNominals(ClientData cdata, Tcl_Interp *interp,
                         int argc, CONST84 char *argv[])
{
  struct rel_relation **rp,**rl,*rel;
  struct var_variable **vp,**vl;
  int32 maxrel,i;
  int ls,rs;
  real64 nom;

  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    FPRINTF(ASCERR,"call is: dbg_calc_relnoms <no args>\n");
    Tcl_SetResult(interp, "dbg_calc_relnoms wants no args", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "dbg_calc_relnoms called with NULL pointer\n");
    Tcl_SetResult(interp, "dbg_calc_relnoms called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  vp = vl = slv_get_solvers_var_list(g_solvsys_cur);
  rp = rl = slv_get_solvers_rel_list(g_solvsys_cur);
  maxrel=slv_get_num_solvers_rels(g_solvsys_cur);

  if (!vp || !rp ) {
    FPRINTF(ASCERR,  "dbg_calc_relnoms called with NULL rel or var list\n");
    Tcl_SetResult(interp, "dbg_calc_relnoms called without rels or vars",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  for (i=0; i<maxrel; i++) {
    ls = rs = 0;
    rel = rl[i];
    if (rel_included(rel) && rel_active(rel) ) {
      if ( dbg_calc_nominal(rel) ) {
        nom = rel_nominal(rel);
        calc_ok = TRUE;
/* dead code...with no reason to live?
   ls = dbg_check_lhs(rel);
        calc_ok = TRUE;
        rs = dbg_check_rhs(rel);
        if (ls || rs ) {
          sprintf(tmps,"%d %d %d %g",i,ls,rs,nom);
          Tcl_AppendElement(interp, tmps);
        }
*/
      }
    }
  }
  return TCL_OK;
}


int Asc_DebuWriteSystem(ClientData cdata, Tcl_Interp *interp,
                    int argc, CONST84 char *argv[]) {
  rel_filter_t rfilter;
  var_filter_t vfilter;
  struct rel_relation **rp;
  struct var_variable **up;
  struct var_variable **vp;
  struct rel_relation *obj;
  slv_status_t ss;
  slv_parameters_t sp;
  int32 maxvar,maxrel,maxirel,maxivar,c;
  int32 maxuna, maxpar;
  char *objs=NULL;
  FILE *fp;

  if ( argc != 2 ) {
    FPRINTF(ASCERR,  "call is: dbg_write_slv0_sys <filepath>\n");
    Tcl_SetResult(interp, "dbg_write_slv0_sys takes 1 arg", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    FPRINTF(ASCERR,  "dbg_write_slv0_sys called with NULL pointer\n");
    Tcl_SetResult(interp, "dbg_write_slv0_sys called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  vp=slv_get_solvers_var_list(g_solvsys_cur);
  if (vp==NULL) {
    FPRINTF(ASCERR,  "dbg_write_slv0_sys called with NULL varlist\n");
    Tcl_SetResult(interp, "dbg_write_slv0_sys called without varlist",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  up=slv_get_solvers_unattached_list(g_solvsys_cur);
  if (up==NULL) {
    FPRINTF(ASCERR,  "There are no unattacehd variables in the model \n");
  }

  rp=slv_get_solvers_rel_list(g_solvsys_cur);
  if (rp==NULL) {
    FPRINTF(ASCERR,  "dbg_write_slv0_sys called with NULL rellist\n");
  }
  obj= slv_get_obj_relation(g_solvsys_cur);
  if (rp==NULL && obj==NULL) {
    FPRINTF(ASCERR,  "dbg_write_slv0_sys called without task.\n");
    Tcl_SetResult(interp,
                  "dbg_write_slv0_sys called without constraints or obj",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  slv_get_parameters(g_solvsys_cur,&sp);
  slv_get_status(g_solvsys_cur,&ss);
  /*
  if (!ss.ready_to_solve) {
    FPRINTF(ASCERR,  "dbg_write_slv0_sys called without ready_to_solve sys\n");
    Tcl_SetResult(interp, "system unready to solve. not written.", TCL_STATIC);
    return TCL_ERROR;
  }
  */

  rfilter.matchbits = (REL_INCLUDED | REL_ACTIVE);
  rfilter.matchvalue = (REL_INCLUDED | REL_ACTIVE);

  vfilter.matchbits = (VAR_INCIDENT | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_ACTIVE);

  maxrel=slv_get_num_solvers_rels(g_solvsys_cur);
  maxvar=slv_get_num_solvers_vars(g_solvsys_cur);

  maxirel=slv_count_solvers_rels(g_solvsys_cur,&rfilter);
  maxivar=slv_count_solvers_vars(g_solvsys_cur,&vfilter);

  fp=fopen(argv[1],"w");
  if (!fp) {
    FPRINTF(ASCERR, "dbg_write_slv0_sys unable to open %s.\n",argv[1]);
    Tcl_SetResult(interp,
                  "dbg_write_slv0_sys file open failed. system not written.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  FPRINTF(fp,"Solver:   \"slv\"\n\n");
  FPRINTF(fp,"Variables: %d\n",maxivar);
  FPRINTF(fp,"   Name   Value    Nominal  Lower   Upper   Fixed\n");
  for (c=0; c<maxvar; c++) {
    if (var_apply_filter(vp[c],&vfilter)) {
      if (cdata) {
        objs=var_make_name(g_solvsys_cur,vp[c]);
        FPRINTF(fp,"   \"%s\" %-16.8g", objs,var_value(vp[c]));
        if (objs) {
          ascfree(objs);
          objs=NULL;
        }
      } else {
        FPRINTF(fp,"   \"x%d\" %-16.8g", var_sindex(vp[c]),var_value(vp[c]));
      }
      FPRINTF(fp,
              " %-16.8g %-16.8g", var_nominal(vp[c]),var_lower_bound(vp[c]));
      FPRINTF(fp," %-16.8g %d\n",var_upper_bound(vp[c]),var_fixed(vp[c]));
    }
  }
  FPRINTF(fp,"\n");

  maxuna =slv_get_num_solvers_unattached(g_solvsys_cur);
  maxpar= slv_count_solvers_unattached(g_solvsys_cur,&vfilter);

  if (maxuna) {
    FPRINTF(fp,"Parameters: %d\n",maxpar);
    FPRINTF(fp,"   Name   Value\n");
    for (c=0; c<maxuna; c++) {
      if (var_apply_filter(up[c],&vfilter)) {
        if (cdata) {
          objs=var_make_name(g_solvsys_cur,up[c]);
          FPRINTF(fp,"   \"%s\" %-16.8g", objs,var_value(up[c]));
          FPRINTF(fp,"\n");
          if (objs) {
          ascfree(objs);
          objs=NULL;
          }
        } else {
          FPRINTF(fp,"   \"x%d\" %-16.8g", c,var_value(up[c]));
          FPRINTF(fp,"\n");
        }
      }
    }
    FPRINTF(fp,"\n");
  } else {
    FPRINTF(fp,"Parameters: 0\n");
  }
  if (obj) {
    if (cdata) {
      objs=relman_make_string_infix(g_solvsys_cur,obj);
    } else {
      objs=relman_make_xstring_infix(g_solvsys_cur,obj);
    }
    FPRINTF(fp,"Objective: \"%s\"\n\n",objs);
    if (objs) {
      ascfree(objs);
      objs=NULL;
    }
  } else {
    FPRINTF(fp,"Objective: \"\"\n\n");
  }
  FPRINTF(fp,"Boundaries:  0\n\n");
  FPRINTF(fp,"Relations: %d",maxirel);
  FPRINTF(fp,"\n");
  for (c=0; c<maxrel; c++) {
    if (rel_apply_filter(rp[c],&rfilter)) {
      FPRINTF(fp,"Relation Nominal: %.16g\n",rel_nominal(rp[c]));
      if (cdata) {
        objs=relman_make_string_infix(g_solvsys_cur,rp[c]);
      } else {
        objs=relman_make_xstring_infix(g_solvsys_cur,rp[c]);
      }
      FPRINTF(fp,"   \"%s\"\n                    Conditions: 0\n",objs);
      if (objs) {
        ascfree(objs);
        objs=NULL;
      }
    }
  }
  FPRINTF(fp,"Iterations:  %d\n",sp.iteration_limit);
  FPRINTF(fp,"Pivot:       %g\n",sp.tolerance.pivot);
  FPRINTF(fp,"Singular:    %g\n",sp.tolerance.singular);
  FPRINTF(fp,"Feasible:    %g\n",sp.tolerance.feasible);
  FPRINTF(fp,"Stationary:  %g\n",sp.tolerance.stationary);
  FPRINTF(fp,"Termination: %g\n",sp.tolerance.termination);
  FPRINTF(fp,"Partition:   %d\n",sp.partition);
  FPRINTF(fp,"Detail:      %d\n",(sp.output.less_important!=NULL)?1:0);
  FPRINTF(fp,"Rho:         %g\n\n",sp.rho);

  fclose(fp);
  return TCL_OK;
}

#define LONGHELP(b,ms) ((b)?ms:"")
int Asc_DebuHelpList(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{
  boolean detail=1;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc > 2 ) {
    FPRINTF(ASCERR,"call is: dbghelp [s,l] \n");
    Tcl_SetResult(interp, "Too many args to dbghelp. Want 0 or 1 args",
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
    PRINTF("%-23s%s\n","dbg_get_blk_of_var",
           LONGHELP(detail,"return partition number of var,if in partition"));
    PRINTF("%-23s%s\n","dbg_get_blk_of_eqn",
           LONGHELP(detail,"return partition number of eqn,if in partition"));
    PRINTF("%-23s%s\n","dbg_get_blk_coords",
           LONGHELP(detail,"return block upleft, lowright corners"));
    PRINTF("%-23s%s\n","dbg_get_eqn_of_var",
           LONGHELP(detail,"return equation number of var,if assigned."));
    PRINTF("%-23s%s\n","dbg_get_varpartition",
           LONGHELP(detail,"return variable list in partitioned order"));
    PRINTF("%-23s%s\n","dbg_get_eqnpartition",
           LONGHELP(detail,"return equation list in partitioned order"));

    PRINTF("%-23s%s\n","dbg_list_rels",
            LONGHELP(detail,"return index list of rels that match qualifier"));
    PRINTF("%-23s%s\n","dbg_list_vars",
            LONGHELP(detail,"return index list of vars that match qualifier"));
    PRINTF("%-23s%s\n","dbg_write_rel",
           LONGHELP(detail,"return relation description in various forms"));
    PRINTF("%-23s%s\n","dbg_write_var",
           LONGHELP(detail,"return variable description in various forms"));
    PRINTF("%-23s%s\n","dbg_write_unattvar",
           LONGHELP(detail,"return variable description for unattached"));
    PRINTF("%-23s%s\n","dbg_write_varattr",
           LONGHELP(detail,"return variable atom description from index"));
    PRINTF("%-23s%s\n","dbg_write_qlfattr",
           LONGHELP(detail,"return variable atom description from name"));
    PRINTF("%-23s%s\n","dbg_rel_included",
           LONGHELP(detail,"boolean return rel included flag"));
    PRINTF("%-23s%s\n","dbg_var_fixed",
           LONGHELP(detail,"boolean return var_fixed flag"));

    PRINTF("%-23s%s\n","dbg_get_incidence",
           LONGHELP(detail,"return list of variables incident in relation"));
    PRINTF("%-23s%s\n","dbg_get_order",
           LONGHELP(detail,"return mtx permuted list of vars/rels"));
    PRINTF("%-23s%s\n","dbg_write_incidence",
           LONGHELP(detail,"return incidence matrix"));
    PRINTF("%-23s%s\n","dbg_find_eligible",
           LONGHELP(detail,"return fixable vars, incident and not"));
    PRINTF("%-23s%s\n","dbg_consistency_analysis",
           LONGHELP(detail,
    "return set of fixable vars to square an overall conditional problem"));
    PRINTF("%-23s%s\n","dbg_global_eligible",
           LONGHELP(detail,
		    "return globally (all alternatives) fixable vars"));
    PRINTF("%-23s%s\n","dbg_find_activerels",
           LONGHELP(detail,"return active rels, included or not"));
    PRINTF("%-23s%s\n","dbg_num_block_singular",
           LONGHELP(detail,"return block row or column numeric dependency"));
    PRINTF("%-23s%s\n","dbg_struct_singular",
           LONGHELP(detail,"return eqns,vars,fixeds making S singularity"));

    PRINTF("%-23s%s\n","var_free2nom",
           LONGHELP(detail,"reset all free variables "
                    "to their nominal values"));
    PRINTF("%-23s%s\n","var_nom2free",
           LONGHELP(detail,"reset variables nominals to var values"));
#if REIMPLEMENT
    PRINTF("%-23s%s\n","dbg_check_rels",
           LONGHELP(detail,"check calculation of all rels at current values"));
    PRINTF("%-23s%s\n","dbg_calc_relnoms",
           LONGHELP(detail,"calculate of nominals of rels at current values"));
#endif
    PRINTF("%-23s%s\n","dbg_write_slv0_xsys",
           LONGHELP(detail,"put x-named system to filename for joe solver"));
    PRINTF("%-23s%s\n","dbg_write_slv0_sys",
           LONGHELP(detail,"put slv system to filename for joe solver"));
#if REIMPLEMENT
    PRINTF("%-23s%s\n","dbg_write_kirk_xsys",
           LONGHELP(detail,"put x-named system to filename for kirks code"));
    PRINTF("%-23s%s\n","dbg_write_gams_xsys",
           LONGHELP(detail,"put x-named system to filename for GAMS code"));
#endif

    PRINTF("%-23s%s\n","dbghelp",
           LONGHELP(detail,"dbghelp s(=names only) l(=this list)."));

    PRINTF("\n");
  }
  if ( argc == 1 ) {
    char * tmps;
    tmps= (char *)ascmalloc((MAXIMUM_NUMERIC_LENGTH+1)*sizeof(char));
    sprintf(tmps,"dbg_get_blk_of_var");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_get_blk_of_eqn");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_get_blk_coords");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_get_eqn_of_var");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_get_varpartition");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_get_eqnpartition");
    Tcl_AppendElement(interp,tmps);

    sprintf(tmps,"dbg_list_rels");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_list_vars");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_write_rel");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_write_var");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_write_unattvar");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_write_varattr");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_write_qlfattr");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_rel_included");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_var_fixed");
    Tcl_AppendElement(interp,tmps);

    sprintf(tmps,"dbg_get_incidence");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_get_order");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_write_incidence");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_find_eligible");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_consistency_analysis");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_global_eligible");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_find_activerels");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_num_block_singular");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_struct_singular");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"var_free2nom");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"var_nom2free");
#if REIMPLEMENT
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_check_rels");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_calc_relnoms");
#endif
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_write_slv0_sys");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_write_slv0_xsys");
#if REIMPLEMENT
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_write_kirk_xsys");
    Tcl_AppendElement(interp,tmps);
    sprintf(tmps,"dbg_write_gams_xsys");
    Tcl_AppendElement(interp,tmps);
#endif
    sprintf(tmps,"dbghelp");
    Tcl_AppendElement(interp,tmps);
    ascfree(tmps);
  }
  return TCL_OK;
}

int Asc_DebuWriteKirkSystem(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char *argv[])
{
  rel_filter_t rfilter;
  var_filter_t vfilter;
  struct rel_relation **rp;
  struct var_variable **vp;
  struct rel_relation  *obj;
  int32 maxvar,maxrel,maxirel,maxivar,c;
  char *objs=NULL, *lhs=NULL, *rhs=NULL;
  FILE *fp;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "Usage dbg_write_kirk_sys <filename>", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    Tcl_SetResult(interp, "dbg_write_kirk_sys called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  vp = slv_get_solvers_var_list(g_solvsys_cur);
  if (vp==NULL) {
    Tcl_SetResult(interp, "dbg_write_kirk_sys called without varlist",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  rp = slv_get_solvers_rel_list(g_solvsys_cur);
  if (rp==NULL) {
    Tcl_SetResult(interp,"Warning : dbg_write_kirk_sys called without rellist",
                  TCL_STATIC);
  }
  obj = slv_get_obj_relation(g_solvsys_cur);
  if (obj==NULL && rp==NULL) {                /* objectives are optional */
    Tcl_SetResult(interp,
                  "dbg_write_kirk_sys called without constraints or obj",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  fp=fopen(argv[1],"w");
  if (!fp) {
    FPRINTF(ASCERR, "dbg_write_kirk_sys unable to open %s.\n",argv[1]);
    Tcl_SetResult(interp,
                  "dbg_write_kirk_sys file open failed. system not written.",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  maxrel=slv_get_num_solvers_rels(g_solvsys_cur);
  maxvar=slv_get_num_solvers_vars(g_solvsys_cur);

  rfilter.matchbits = (REL_INCLUDED | REL_ACTIVE);
  rfilter.matchvalue = (REL_INCLUDED | REL_ACTIVE);

  vfilter.matchbits = (VAR_INCIDENT | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_ACTIVE);

  maxirel = slv_count_solvers_rels(g_solvsys_cur,&rfilter);
  maxivar = slv_count_solvers_vars(g_solvsys_cur,&vfilter);


  /*
   * This is where acutally generate the code.
   * It could be made a lot more efficient by writing
   * directly to a file rather than *making* the string and
   * then writing to a file.
   */
  FPRINTF(fp,"#Variables: %d\n",maxivar);
  FPRINTF(fp,"   #Name\tValue\n");
  for (c=0;c<maxvar;c++) {
    if (var_apply_filter(vp[c],&vfilter)) {
      FPRINTF(fp,"   x%d := %16.8g;\n", var_sindex(vp[c]),var_value(vp[c]));
    }
  }
  FPRINTF(fp,"\n");

  /*
   * Write out the objective function if one exists.
   */
  if (obj) {
    objs = relman_make_xstring_infix(g_solvsys_cur,obj);
    FPRINTF(fp,"#Objective: \"%s\"\n\n",objs);
    if (objs) {
      ascfree(objs);
    }
  }

  /*
   * Now write out the relations.
   */
  FPRINTF(fp,"#Boundaries:  0\n\n");
  FPRINTF(fp,"#Relations: %d\n",maxirel);
  for (c=0; c<maxrel; c++) {
    if (rel_apply_filter(rp[c],&rfilter)) {
#if REIMPLEMENT
/* # should probably change what this interface function does instead of */
/* # changing relman. */
      lhs = relman_make_xstring_infix(g_solvsys_cur,rel_lhs(rp[c]));
#else
    FPRINTF(ASCERR,"Asc_DebuWriteKirkSystem \n");
    FPRINTF(ASCERR,"relman funtions have to be reimplemented \n");
    exit;
#endif
      FPRINTF(fp,"%s  - (",lhs);
#if REIMPLEMENT
      rhs = relman_make_xstring_infix(g_solvsys_cur,rel_rhs(rp[c]));
#else
    FPRINTF(ASCERR,"Asc_DebuWriteKirkSystem \n");
    FPRINTF(ASCERR,"relman funtions have to be reimplemented \n");
    exit;
#endif
      FPRINTF(fp,"%s)  =  0.0;\n",rhs);
      if (lhs) {
        ascfree(lhs);
      }
      if (rhs) {
        ascfree(rhs);
      }
    }
  }
  fclose(fp);
  return TCL_OK;
}

int Asc_DebuWriteGAMSSystem(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char *argv[])
{
  struct rel_relation **rp;
  struct var_variable **vp;
  struct rel_relation  *obj;
  int32 maxvar,maxrel,c;
  char *objs=NULL;
  char *lhs=NULL, *rhs=NULL;
  char *var_name;
  real64 val_tmp;
  FILE *fp;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "dbg_write_gams_sys takes 1 arg", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    Tcl_SetResult(interp, "dbg_write_gams_sys called without slv_system",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  vp = slv_get_solvers_var_list(g_solvsys_cur);
  if (vp==NULL) {
    Tcl_SetResult(interp, "dbg_write_gams_sys called without varlist",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  rp =slv_get_solvers_rel_list(g_solvsys_cur);
  if (rp==NULL) {
    Tcl_SetResult(interp, "dbg_write_gams_sys called with NULL rellist",
                  TCL_STATIC);
  }
  obj= slv_get_obj_relation(g_solvsys_cur);
  if (rp==NULL && obj==NULL) {
    Tcl_SetResult(interp,
                  "dbg_write_gams_sys called without constraints or obj",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  maxrel=slv_get_num_solvers_rels(g_solvsys_cur);
  maxvar=slv_get_num_solvers_vars(g_solvsys_cur);

  fp=fopen(argv[1],"w");
  if (!fp) {
    Tcl_SetResult(interp,
                  "dbg_write_gams_sys file open failed. system not written.",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  FPRINTF(fp,"$Title Ascend Generated GAMS Model");
  FPRINTF(fp,"$offsymlist\n");
  FPRINTF(fp,"$offsymxref\n");
  FPRINTF(fp,"option limrow = 0;\n");
  FPRINTF(fp,"option limcol = 0;\n");
  FPRINTF(fp,"$inlinecom /* */\n\n");

  FPRINTF(fp,"variables\n");
  for(c=0; c<maxvar; c++) {
    if (var_incident(vp[c])) {
      var_name = var_make_name(g_solvsys_cur,vp[c]);
      FPRINTF(fp,"   x%d\t/* %s */\n", var_sindex(vp[c]),var_name);
      ascfree(var_name);
    }
  }

  FPRINTF(fp,"   ;\n\n");
  for (c=0; c<maxvar; c++) {
    if (var_incident(vp[c])) {
      val_tmp = ((var_lower_bound(vp[c]) < -1e04)
                 ? -1e04
                 : var_lower_bound(vp[c]));
      FPRINTF(fp,"   x%d.lo = %16.8g;\n",var_sindex(vp[c]), val_tmp);

      val_tmp = ((var_upper_bound(vp[c]) > 1e04)
                 ? 1e04
                 : var_upper_bound(vp[c]));
      FPRINTF(fp,"   x%d.up = %16.8g;\n", var_sindex(vp[c]), val_tmp);

      val_tmp = (var_value(vp[c]) > 1e04) ? 1e04 : var_value(vp[c]);
      FPRINTF(fp,"   x%d.l = %16.8g;\n",var_sindex(vp[c]), val_tmp);

      if (var_fixed(vp[c]) && var_active(vp[c]) ) {
        FPRINTF(fp,"   x%d.fx = %16.8g;\n", var_sindex(vp[c]),val_tmp);
      }
    }
  }

  FPRINTF(fp,"\n");
  if (obj) {
    FPRINTF(fp,"variables   obj_var;\n\n");
    FPRINTF(fp,"equations   obj_eqn;\n\n");
    objs = relman_make_xstring_infix(g_solvsys_cur,obj);
    FPRINTF(fp,"obj_eqn..   obj_var =g= %s;\n",objs);
    FPRINTF(fp,"\n\n");
    ascfree(objs);
    objs = NULL;
  }

  FPRINTF(fp,"equations \n");
  for (c=0; c<maxrel; c++) {
    if (rel_included(rp[c]) && rel_active(rp[c]) ) {
      FPRINTF(fp,"    rel_%d\n", rel_sindex(rp[c]));
    }
  }

  FPRINTF(fp,"    ;\n\n\n");
  for (c=0; c<maxrel;c++) {
    if (rel_included(rp[c]) && rel_active(rp[c]) ) {
#if REIMPLEMENT
      lhs = relman_make_xstring_infix(g_solvsys_cur,rel_lhs(rp[c]));
      rhs = relman_make_xstring_infix(g_solvsys_cur,rel_rhs(rp[c]));
#else
    FPRINTF(ASCERR,"Asc_DebuWriteGAMSSystem \n");
    FPRINTF(ASCERR,"relman funtions have to be reimplemented \n");
    exit;
#endif
      FPRINTF(fp,"rel_%d..   %s",rel_sindex(rp[c]),lhs);
      switch( GetInstanceRelationType(rel_instance(rp[c])) ) {
      case e_less:
      case e_lesseq:
        FPRINTF(fp," =l= ");
        break;
      case e_equal:
        FPRINTF(fp," =e= ");
        break;
      case e_greater:
      case e_greatereq:
        FPRINTF(fp," =g= ");
        break;
      default:
        break;
      }
      FPRINTF(fp,"%s;\n",rhs);
      if (lhs) {
        ascfree(lhs);
      }
      if (rhs) {
        ascfree(rhs);
      }
      lhs = rhs = NULL;
    }
  }

  FPRINTF(fp,"\n\n\n");

  FPRINTF(fp,"model test1 using /\n");
  for(c=0; c<maxrel;c++) {
    if (rel_included(rp[c]) && rel_active(rp[c]) ) {
      FPRINTF(fp,"    rel_%d\n", rel_sindex(rp[c]));
    }
  }
  if (obj) {
    FPRINTF(fp,"    obj_eqn\n");
  }
  FPRINTF(fp,"                  /;\n");

  FPRINTF(fp,"solve test1 using nlp minimizing obj_var;\n");
  fclose(fp);
  return TCL_OK;
}


/*
 * A lot more could be done with this function. It could be made
 * to take a block no, or a region as 4 coordinates. As it stands,
 * it tries to figure out the rank, and plots that region.
 */

int Asc_DebuMtxWritePlotCmd(ClientData cdata, Tcl_Interp *interp,
                         int argc, CONST84 char *argv[])
{
  FILE *fp = NULL;
  int rank, coeff_or_inverse = 0;
  int offset = 1;                        /* set to 0 for c-style indexing */
  linsol_system_t linsys;
  linsolqr_system_t linsysqr;
  mtx_matrix_t mtx = NULL;
  mtx_region_t reg;
  real64 *rhs;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc < 4 ) {
    Tcl_AppendResult(interp,"wrong # args: Usage :",
                     "dbg_mtxwriteplot file ?coeff?inv? ",
                     "?plot?mtx?csr?smms?", (char *)NULL);
    return TCL_ERROR;
  }
  if (g_solvsys_cur==NULL) {
    Tcl_SetResult(interp, "NULL solve system in dbg_mtxwriteplot", TCL_STATIC);
    return TCL_ERROR;
  }
  if (strncmp(argv[2],"coeff",3)==0) {
    coeff_or_inverse = 0;
  } else {
    coeff_or_inverse = 1;
  }

  fp = fopen(argv[1],"w");
  if (!fp) {
    Tcl_SetResult(interp, "Unable to create named file.\n", TCL_STATIC);
    return TCL_ERROR;
  }
  if (coeff_or_inverse==0) {
    /* we have a standard matrix fetch for coefficient matrices */
    mtx = slv_get_sys_mtx(g_solvsys_cur);
    if (mtx==NULL || mtx_order(mtx)<1) {
      FPRINTF(ASCERR,
        "Solve system does not have a valid coefficient matrix\n");
      goto error;
    }
  } else {
    /* WARNING: developers ui hack only! */
    switch(slv_get_selected_solver(g_solvsys_cur)) {
    case 0:
      linsys = slv_get_linsol_sys(g_solvsys_cur);
      mtx = linsol_get_inverse(linsys);
      rhs = linsol_get_rhs(linsys,1);
      break;
    case 3:
    case 5:
      linsysqr = slv_get_linsolqr_sys(g_solvsys_cur);
      mtx = linsolqr_get_factors(linsysqr);
      rhs = linsolqr_get_rhs(linsysqr,1);
      break;
    default:
      FPRINTF(ASCERR,"This solver is not supported for inverse plotting\n");
      break;
    }
  }
  if (mtx==NULL) {
    FPRINTF(ASCERR,"Null matrix found. Either this solver doesn't share\n");
    FPRINTF(ASCERR,"matrices or this system not presolved/inverted.\n");
    goto error;
  }

  if (coeff_or_inverse==0) {
    rank = mtx_symbolic_rank(mtx);
  } else {
    rank = mtx_order(mtx);
  }
  reg.row.low = reg.col.low = 0; /* might make into a parameter */
  reg.row.high = reg.col.high = rank - 1;

  /*
   * Decode the format of the matrix and write it out.
   */
  if (strncmp(argv[3],"plot",3)==0) {
    mtx_write_region_plot(fp,mtx,&reg);
  } else if (strncmp(argv[3],"mtx",3)==0) {
    mtx_write_region(fp,mtx,mtx_ENTIRE_MATRIX);
  } else if (strncmp(argv[3],"smms",3)==0) {
    mtx_write_region_smms(fp,mtx,&reg,offset);
  } else if (strncmp(argv[3],"csr",3)==0) {
    int orgcol,j;

    mtx_write_region_csr(fp,mtx,&reg,offset);                /* do matrix */
    if (!rhs) {
      goto error;
    }
    for (j=reg.col.low; j<=reg.col.high; j++) {                /* do rhs */
      orgcol = mtx_col_to_org(mtx,j);
      if (orgcol>=0) {
        FPRINTF(fp,"%20.8e\n",rhs[orgcol]);
      } else {
        FPRINTF(ASCERR,"Data is corrupted -- col index out of range\n");
      }
    }
    FPRINTF(fp,"\n\n");
  } else {
    FPRINTF(ASCERR,"Unknown format in dbg_mtxwrite\n");
    goto error;
  }


 error:
  if (fp) {
    fclose(fp);
  }
  return TCL_OK;
}

/*
 * See file slv5.c.
 * Someone can delete this file once I am gone : KAA.
 */
/*extern int slv5_calc_J();*/                /* KAA_DEBUG */

int Asc_DebuMtxCalcJacobianCmd(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[])
{
  int whichsolver;
  int result=TCL_ERROR;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_AppendResult(interp,"wrong # args :",
                     "Usage dbg_calc_jacobian whichsolver",(char *)NULL);
    return TCL_ERROR;
  }

  if (g_solvsys_cur==NULL) {
    Tcl_SetResult(interp, "Solve system does not exist", TCL_STATIC);
    return TCL_ERROR;
  }
  whichsolver = atoi(argv[1]);

  if (whichsolver!=5) {        /* slv5 */
    Tcl_SetResult(interp, "Invalid solver given -- only slv5 is valid",
                  TCL_STATIC);
    return TCL_ERROR;
  }
/*  result = slv5_calc_J(g_solvsys_cur); KHACK */
  if (result) {
    return TCL_ERROR;
  } else {
    return TCL_OK;
  }
}
