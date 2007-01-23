/*	ASCEND modelling environment
	Copyright (C) 1996 Benjamin Andrew Allan
	Copyright (C) 1998 Vicente Rico-Ramirez
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
*//** @file
	Degrees-of-freedom manager

	@TODO we need to usage information for this stuff.
*//*
	by Benjamin Andrew Allan and Vicente Rico-Ramirez, Created: 7/11/94
	Last in CVS: $Revision: 1.24 $ $Date: 1998/04/09 21:56:09 $ $Author: rv2a $
*/

#include "slvDOF.h"

#include <stdarg.h>

#include <utilities/ascSignal.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <general/list.h>
#include <general/mathmacros.h>
#include <utilities/mem.h>

#include "relman.h"
#include "cond_config.h"
#include "slv_common.h"
#include "slv_stdcalls.h"

#define SLVDOF(s) ((slvDOF_system_t)(s))

#define DEBUG FALSE
#define DEBUG_CONSISTENCY_ANALYSIS FALSE
#define ELDEBUG 0  /* print debug statements in eligible code */
#define KILL 0

struct jacobian_data{
   mtx_matrix_t           mtx;          /* Transpose gradient of residuals */
   mtx_region_t           reg;          /* Current block region */
};

typedef struct slvDOF_system_structure *slvDOF_system_t;

struct slvDOF_system_structure{

   /* problem definition */
   slv_system_t           slv;          /* slv_system_t back-link */
   struct var_variable    **vlist;      /* Variable list (NULL terminated) */
   struct rel_relation    **rlist;      /* Relation list (NULL terminated) */

   /* solver information */
   unsigned char          *rows;        /* marks on rows */
   unsigned char          *cols;        /* marks on cols */
   int32                  *rowlist;     /* list of newly marked rows */
   int32                  *collist;     /* list of newly marked cols */
   int                    integrity;    /* ? Has the system been created */
   slv_parameters_t       p;            /* Parameters */
   slv_status_t           s;            /* Status (as of iteration end) */
   int32                  cap;          /* Order of matrix/vectors */
   int32                  rank;         /* Symbolic rank of problem */
   int32                  vused;        /* Free and incident variables */
   int32                  vtotal;       /* all variables */
   int32                  rused;        /* Included relations */
   int32                  rtot;         /* rellist len */
   double                 clock;        /* CPU time */

   /* calculated data (scaled) */
   struct jacobian_data   J;            /* linearized system */
};


/*------------------------------------------------------------------------------
  INTEGRITY CHECKS
*/

/* what are these crazy numbers??? */
#define OK        ((int)13695914)
#define DESTROYED ((int)15784619)

/**
	Checks sys for NULL and for integrity.
*/
static int check_system(slvDOF_system_t sys){
	if(sys == NULL) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"system is NULL");
	    return 1;
	}

	switch(sys->integrity) {
	case OK:
	    return 0;
	case DESTROYED:
	    ERROR_REPORTER_HERE(ASC_PROG_ERR,"system was recently destroyed");
	    return 1;
	default:
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"system reused or never allocated");
	    return 1;
	}
}

/*------------------------------------------------------------------------------
  GENERAL INPUT/OUTPUT ROUTINES
*/

#define print_var_name(a,b,c) slv_print_var_name((a),(b->slv),(c))
#define print_rel_name(a,b,c) slv_print_rel_name((a),(b->slv),(c))

/*------------------------------------------------------------------------------
  EXTERNAL ROUTINES (see slv.h)
*/

static
slvDOF_system_t slvDOF_create()
{
   slvDOF_system_t sys;

   sys = (slvDOF_system_t)asccalloc(1,sizeof(struct slvDOF_system_structure) );
   sys->integrity = OK;
   sys->p.output.more_important = stdout;
   sys->p.output.less_important = NULL;
   sys->p.partition = TRUE;
   sys->s.ok = TRUE;
   sys->s.costsize = 0;
   sys->s.cost = NULL; /*redundant, but sanity preserving */

   return(sys);
}

static void destroy_matrices(slvDOF_system_t sys)
{
  if( sys->J.mtx ) {
    mtx_destroy(sys->J.mtx);
  }
  ASC_FREE(sys->rows);
  ASC_FREE(sys->cols);
  ASC_FREE(sys->rowlist);
  ASC_FREE(sys->collist);
}

static
int slvDOF_destroy(slvDOF_system_t sys)
{
  if (check_system(sys)) return 1;
  destroy_matrices(sys);
  sys->integrity = DESTROYED;
  if (sys->s.cost) ascfree(sys->s.cost);
  ascfree( (POINTER)sys );
  return 0;
}


#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
void slvDOF_get_parameters(sys,parameters)
slvDOF_system_t sys;
slv_parameters_t *parameters;
{
   check_system(sys);
   mem_copy_cast(&(sys->p),parameters,sizeof(slv_parameters_t));
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */

#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
void slvDOF_set_parameters(sys,parameters)
slvDOF_system_t sys;
slv_parameters_t *parameters;
{
   check_system(sys);
   mem_copy_cast(parameters,&(sys->p),sizeof(slv_parameters_t));
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */

#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
void slvDOF_get_status(sys,status)
slvDOF_system_t sys;
slv_status_t *status;
{
   check_system(sys);
   mem_copy_cast(&(sys->s),status,sizeof(slv_status_t));
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */

/**
	Performs structural analysis on the system, setting the flags in
	status.  The problem must be set up, the relation/variable list
	must be non-NULL. sys->cap, vtotal, rtot must be set.
*/
static void create_matrices(slv_system_t server,slvDOF_system_t sys){
   var_filter_t vfilter;
   rel_filter_t rfilter;

   sys->J.mtx = mtx_create();
   mtx_set_order(sys->J.mtx,sys->cap);

  /* Note: the server has marked incidence flags already. */

  /* count included equalities */
  rfilter.matchbits = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  rfilter.matchvalue = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  sys->rused = slv_count_solvers_rels(server,&rfilter);

  /* count free and incident solver vars */
  vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
  sys->vused = slv_count_solvers_vars(server,&vfilter);

  if(slv_std_make_incidence_mtx(server,sys->J.mtx,&vfilter,&rfilter)){
    ASC_PANIC("slv_std_make_indicidence_mtx failed");
  }

  /* Symbolic analysis */
  sys->rtot = slv_get_num_solvers_rels(server);
  sys->vtotal = slv_get_num_solvers_vars(server);
  mtx_output_assign(sys->J.mtx,sys->rtot,sys->vtotal);
  /* symbolic rank is set */
  sys->rank = mtx_symbolic_rank(sys->J.mtx);

  /* create integer,char workspaces. */
  sys->rows = ASC_NEW_ARRAY_CLEAR(unsigned char,sys->cap);
  sys->cols = ASC_NEW_ARRAY_CLEAR(unsigned char,sys->cap);
  sys->rowlist = ASC_NEW_ARRAY(int32,sys->rtot);
  sys->collist = ASC_NEW_ARRAY(int32,sys->vtotal);

  /* Initialize Status */
  sys->s.over_defined = (sys->rused > sys->vused);
  sys->s.under_defined = (sys->rused < sys->vused);
  sys->s.struct_singular = (sys->rank < sys->rused);
  sys->s.block.number_of = mtx_number_of_blocks(sys->J.mtx);
}

/**
	return 0 on success
*/
static int slvDOF_presolve(slv_system_t server, slvDOF_system_t sys){

	check_system(sys);
	sys->vlist = slv_get_solvers_var_list(server);
	sys->rlist = slv_get_solvers_rel_list(server);
	if( sys->vlist == NULL ) {
	    ERROR_REPORTER_HERE(ASC_PROG_ERR,"variable list not found");
	    return 1;
	} else if( sys->rlist == NULL ) {
    	ERROR_REPORTER_HERE(ASC_PROG_ERR,"relation list not found");
	    return 1;
	}

	sys->rtot= slv_get_num_solvers_rels(server);
	sys->vtotal= slv_get_num_solvers_vars(server);
	sys->cap = MAX(sys->rtot,sys->vtotal);

	destroy_matrices(sys);
	create_matrices(server,sys);

	/* Reset status */
	sys->s.iteration = 0;
	sys->s.cpu_elapsed = 0.0;
	sys->s.converged = sys->s.diverged = sys->s.inconsistent = FALSE;
	sys->s.block.previous_total_size = 0;
	sys->s.block.current_block = -1;
	return 0;
}

/* this can be coded recursively, but it's a dumb idea.
 * see dof.pas:partition_calculated_region.
 */
/* Note: it would be easy to return the list of unreachable relations
 * (those assigned to ineligible vars) as well. no known use for it though.
 */
int slvDOF_eligible(slv_system_t server, int32 **vil) {

  mtx_matrix_t mtx;
  int32 *va, *rowlist, *collist;
  unsigned char *rows=NULL, *cols=NULL;
  int32 rmax,cmax,mmax,ccur,r,c,rt,ct,ft,rank,vsize;
  int i,newrows,newcols;
  struct rel_relation **rp;
  struct var_variable **vp;
  var_filter_t vfilter;
  mtx_coord_t coord;
  slvDOF_system_t sys;

  if (server==NULL || vil == NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"system or vil is NULL");
    return 0;
  }

  sys = slvDOF_create();

  *vil = NULL; /* zero return pointer ahead of time */
  if(sys==NULL){
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"insufficient memory");
    return 0;
  }
  if(slvDOF_presolve(server,sys)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"failed presolve");
    /* need to destroy memory here */
    slvDOF_destroy(sys);
    return 0;
  }

  vp = sys->vlist;
  rp = sys->rlist;
  rowlist = sys->rowlist;
  collist = sys->collist;
  rows = sys->rows;
  cols = sys->cols;
  /* nonsingular and not empty; no list */
  rmax = sys->rused;
  rank = sys->rank;
  if (NULL == (mtx=sys->J.mtx)) return 0; /* there is no jacobian-- wierd */
  if (!mtx_check_matrix(mtx)) return 0; /* jacobian bad, very wierd */

  cmax=sys->vtotal;
  mmax=sys->cap;

  rt=ct=ft=0;
/* col marks:
	0 unvisited/ineligible
	1 visited
   row marks:
	0 unvisited
	1 visited
 */
/* initing things. */
  newcols = newrows = 0; /* next available ptrs in rellist,collist */
  /* add free incident unassigned vars to the collist. since
   * fake and fixed cols never have incidence, we will never
   * mark them, nor fake or unincluded rows.
   */
  vfilter.matchbits = (VAR_INCIDENT | VAR_FIXED | VAR_SVAR | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
  for (ccur=rank; ccur < mmax; ccur++) {
    i = mtx_col_to_org(mtx,ccur);
    if ( i < cmax ) {  /* if col real */
      if (var_apply_filter(vp[i],&vfilter)) {
        collist[newcols++] = ccur;
        cols[ccur] = 1;
        ct++;
#if ELDEBUG
        PRINTF("marking free unassigned col %d\n",ccur);
#endif
      }
    }
  }
  /* now mark up everyone who might be eligible */
  do {
    while (newcols > 0) {
      /* mark all rows crossing newly marked cols */
      /* should we allow incidence in unassigned rows to count?
       * we do.
       */
      coord.col = collist[--newcols];
#if ELDEBUG
      PRINTF("scanning col %d\n",coord.col);
#endif
      coord.row = mtx_FIRST;
      while (mtx_next_in_col(mtx,&coord,mtx_ALL_ROWS),
             coord.row != mtx_LAST ) {
#if ELDEBUG
        PRINTF("found row: %d rel:%d\n",coord.col,
               mtx_row_to_org(mtx,coord.row));
#endif
        if (!rows[coord.row]) {  /* if not here before, mark row */
          rows[coord.row] = 1;
          rowlist[newrows++] = coord.row;
          rt++;
        }
      }
    }
    while (newrows > 0) {
      /* set new mark flag on assigned col of new rows */
      r = rowlist[--newrows];
      if (!cols[r]) {
#if ELDEBUG
        PRINTF("marking col %d\n",r);
#endif
        cols[r] = 1;
        collist[newcols++] = r;
        ct++;
      }
    }
  } while (newcols>0);
  /* now remove the FALSE positive eligible vars */
  for (r = 0; r < rank; r++) {
    /* assignments in unreachable rows are ineligible */
    if (!rows[r] && cols[r]) {
#if ELDEBUG
      PRINTF("removing col %d with unassignable row from eligibles.\n",r);
#endif
      cols[r] = 0;
      ct--;
    }
  }
#if ELDEBUG
  PRINTF("ct= %d, rt= %d\n",ct,rt);
#endif
  va = *vil = ASC_NEW_ARRAY_CLEAR(int32,1+ct);
  vsize = ct;
  va[ct]=(-1);
  ct=0;

/* make up col report */
  for(c=0;c<mmax;c++){
    if(cols[c]==1){
#if ELDEBUG
      PRINTF("recording marked col %d\n",c);
      if(ct<vsize){;
#endif
        va[ct] = mtx_col_to_org(mtx,c);
        ct++;
#if ELDEBUG
      }else{
        PRINTF("unexpectedly ");
      }
      PRINTF("got eligible col %d\n",c);
#endif
    }
  }
#if ELDEBUG
  PRINTF("last ct %d\n",ct-1);
#endif
  slvDOF_destroy(sys);
  return 1;
}

/*
	This can be coded recursively, but it's a bad idea.
  	See dof.pas:find_swap_vars if you can find the pascal version.

	Turn your head sideways and you see it is the same code as
	find-eligible but with different output and skipping all the debug.

	@return 0 on success
*/
int slvDOF_structsing(slv_system_t server, int32 rwhy, int32 **vover
		,int32 **rcomb, int32 **vfixed
){
  mtx_matrix_t mtx;
  const struct var_variable **fv=NULL;
  int32 *va, *ra, *fa, *rowlist, *collist;
  unsigned char *rows=NULL, *cols=NULL;
  int32 rmax,cmax,mmax,rcur,r,c,rt,ct,ft,rank,var,varmax;
  int i,newrows,newcols;
  struct rel_relation **rp;
  struct var_variable **vp;
  var_filter_t vfilter;
  mtx_coord_t coord;
  slvDOF_system_t sys;

  if(server==NULL || vover == NULL || rcomb == NULL || vfixed == NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called with a NULL parameter");
    return 1;
  }

  sys = slvDOF_create();

  if(sys == NULL){
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"insufficient memory.");
    return 2;
  }
  if (slvDOF_presolve(server,sys)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"failed presolve");
    slvDOF_destroy(sys);
    return 3;
  }

  *vfixed = *vover = *rcomb = NULL; /* zero return pointers ahead of time */
  vp = sys->vlist;
  rp = sys->rlist;
  rowlist = sys->rowlist;
  collist = sys->collist;
  rows = sys->rows;
  cols = sys->cols;

  /* nonsingular and not empty; no list */
  rmax = sys->rused;
  rank = sys->rank;
  if(sys->rank == rmax && rmax > 0){
    slvDOF_destroy(sys);
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"rused = rank and rused > 0. dunno.");
    return 666; /* is this an error case? */
  }
  if((mtx=sys->J.mtx)==NULL){
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"No jacobian -- wierd.");
    return 4;
  }
  if(!mtx_check_matrix(mtx)){
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Bad jacobian -- v wierd.");
    return 5;
  }
  cmax=sys->vused;
  mmax=sys->cap;

  rt=ct=ft=0;
  if(rwhy > -1)rwhy = mtx_row_to_org(mtx,rwhy);

  /* 
    col marks:
     0 unvisited/ineligible
     1 visited
    row marks:
 	 0 unvisited
     1 visited
  */

  vfilter.matchbits = (VAR_INCIDENT | VAR_FIXED | VAR_SVAR | VAR_ACTIVE);
  vfilter.matchvalue = vfilter.matchbits;

  /* initing things. */
  newcols = newrows = 0; /* next available ptrs in rellist,collist */

  /* 
	add included unassigned user-interesting equations to rellist.
	ignore fake and unincluded rows.
  */
  for (rcur = rank; rcur < mmax; rcur++) {
    i = mtx_row_to_org(mtx,rcur);
    if ( i < sys->rtot && (rwhy == -1 || rcur == rwhy)) {  /* if row real */
      if (rel_included(rp[i]) && rel_active(rp[i])) {
        rowlist[newrows++] = rcur;
        rows[rcur] = 1;
        rt++;
      }
    }
  }

  /* now mark up everyone who might be singular */
  do {
    while (newrows > 0) {
      /* mark all cols crossing newly marked rows */
      coord.row = rowlist[--newrows];
      coord.col = mtx_FIRST;
      while (mtx_next_in_row(mtx,&coord,mtx_ALL_COLS),
             coord.col != mtx_LAST ) {
        if (coord.row==coord.col) continue; /* skip assigned var of row */
        if (!cols[coord.col]) {  /* if not here before, mark col */
          cols[coord.col] = 1;
          collist[newcols++] = coord.col;
          ct++;
        }
      }
    }
    while (newcols > 0) {
      /* set new mark flag on assigned row of new cols */
      c = collist[--newcols];
      if (!rows[c]) {
        rows[c] = 1;
        rowlist[newrows++] = c;
        rt++;
      }
    }
  } while (newrows>0);

  va = *vover = ASC_NEW_ARRAY_CLEAR(int32,1+ct);
  ra = *rcomb = ASC_NEW_ARRAY_CLEAR(int32,1+rt);
  va[ct] = ra[rt] = (-1);
  rt = ct = 0;

  /* mark also the fixed columns in marked equations while making row report */
  for (r=0; r<mmax; r++) {
    if (rows[r]>0) {
      ra[rt] = i = mtx_row_to_org(mtx,r);
      rt++;
      fv = rel_incidence_list(rp[i]);
      varmax = rel_n_incidences(rp[i]);
      if ( fv != NULL && varmax >0) {
        for (var = 0; var<varmax; var++) {
          if (var_apply_filter(fv[var],&vfilter)) {
            cols[mtx_org_to_col(mtx,var_sindex(fv[var]))] = 1;
          }
        }
      }
    }
  }

  /* make up free singular col report */
  for (c = 0; c < cmax; c++) {
    if (cols[c] == 1) {
        va[ct] = mtx_col_to_org(mtx,c);
        ct++;
    }
  }

  /* make up overspecified cols report */
  for (c=cmax;c<mmax;c++) {
    if (cols[c]) ft++;
  }
  fa = *vfixed = ASC_NEW_ARRAY_CLEAR(int32,1+ft);
  fa[ft] = (-1);
  ft=0;
  for (c = cmax; c < mmax; c++) {
    if (cols[c]>0) {
      fa[ft++] = mtx_col_to_org(mtx,c);
    }
  }
  slvDOF_destroy(sys);
  ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Completed structural singularity analysis");
  return 0;
}

/*------------------------------------------------------------------------------
  CONDITIONAL MODELLING
*/

/*
 *  Global Search Consistency Analysis
 *  ---------------------------------------------------------
 *  This Analysis performs a combinatorial search (over all the
 *  alternative sets of equations) to find a consitent
 *  partitioning of the variables (appropriate selection of 
 *  degrees of freedom). It checks first if the analysis is
 *  necessary (checks which conditional statements may
 *  change the structure of the system) and it will only consider
 *  for the analysis those statements which may change the structure
 *  of the system.
 */

/* auxiliary structures */
struct bool_values {
  int32            *pre_val;    /* previous values of dis_discrete */
  int32            *cur_val;    /* current values of dis_discrete  */
};

/*
	(see slvDOF.h)

	In each step of the combinatorial search it is necessary to find out
	if the alternative being analyzed is square, underspecified,
	structurally singular or overspecified.

	status = 1  ==> underspecified
	status = 2  ==> square
	status = 3  ==> structurally singular
	status = 4  ==> overspecifed
	status = 5  ==> <error>
*/
int32 slvDOF_status(slv_system_t server, int32 *status, int32 *dof){
  slvDOF_system_t sys;

  if(server==NULL){
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"system is NULL");
    *status = 5;
    return 0;
  }
  *dof = 0;
  sys = slvDOF_create();

  if(sys == NULL){
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"insufficient memory");
    *status = 5;
    return 0;
  }
  if(slvDOF_presolve(server,sys)){
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"failed presolve");
    *status = 5;
    slvDOF_destroy(sys);
    return 0;
  }

  if(sys->rank < sys->rused){
    *status = 3;
    *dof = 0;
    slvDOF_destroy(sys);
    return 1;    
  }

  if(sys->rused > sys->vused){
    *status = 4;
    slvDOF_destroy(sys);
	return 1;
  }

  if((sys->vused==sys->rused) && (sys->rank ==sys->rused)){
    *status = 2;
    slvDOF_destroy(sys);
    return 1;
  }

  if(sys->vused > sys->rused){
    *status = 1;
    *dof = sys->vused - sys->rused;
    slvDOF_destroy(sys);
    return 1;
  }

  return 1;
}


/**
	The first element of cur_cases is in position one. The result is
	the same array, but ordered and starting in position zero
 */
static int32 *reorder_cases(int32 *cur_cases, int32 ncases){

  int32 cur_case,pos = 0,tmp_num,c,ind;
  int32 *result;

  result = ASC_NEW_ARRAY(int32,ncases);
  for (c=1; c<=ncases; c++) {
    tmp_num = 0;
    for (ind=1; ind<=ncases; ind++) {
      cur_case = cur_cases[ind];
      if (tmp_num < cur_case) {
        pos = ind;
        tmp_num  = cur_case;
      }
    }
    cur_cases[pos] = 0;
    result[ncases-c] = tmp_num;
  }

  ASC_FREE(cur_cases);
  return result;
}

/**
	Get the eligible var list for each alternative
	
	@return 1 on success, 0 if the analysis has failed with the current parititioning
	or -1 if a memory problem has occurred
*/
static int32 get_eligible_vars(slv_system_t server,struct gl_list_t *disvars
		,int *combinations, int32 *terminate
){
  struct var_variable **vslist;
  struct var_variable **vmlist;
  struct var_variable *mvar, *svar;
  var_filter_t vfilter;
  int32 *cur_cases;
  int32 *correct_cases;
  int32 *vars;
  int32 v, count = 0, ind;
  int32 ncases;
  int32 mnum;
  int32 status,dof;

  vslist = slv_get_solvers_var_list(server);
  vmlist = slv_get_master_var_list(server);
  mnum = slv_get_num_master_vars(server);
  for (v=0; v<mnum; v++) {
    mvar = vmlist[v];
    var_set_eligible_in_subregion(mvar,FALSE);
  } 

  cur_cases = cases_matching(disvars,&ncases);
  correct_cases = reorder_cases(cur_cases,ncases);
  set_active_rels_in_subregion(server,correct_cases,ncases,disvars);
  set_active_vars_in_subregion(server);
  ASC_FREE(correct_cases);

#if DEBUG_CONSISTENCY_ANALYSIS
  FPRINTF(ASCERR,"Analyzing alternative:\n");
#endif /* DEBUG_CONSISTENCY_ANALYSIS */

  if(!slvDOF_status(server,(&status),(&dof))) {
   ERROR_REPORTER_HERE(ASC_PROG_ERR,"Aborted combinatorial search after error");
   return -1;
  }else{
    if (status == 3) {
#if DEBUG_CONSISTENCY_ANALYSIS
      CONSOLE_DEBUG("Alternative is structurally singular");
#endif /* DEBUG_CONSISTENCY_ANALYSIS */
      (*terminate) = 0;
      return 0;
    } else {
      if (status == 4) {
#if DEBUG_CONSISTENCY_ANALYSIS
      CONSOLE_DEBUG("Alternative is overspecified");
#endif /* DEBUG_CONSISTENCY_ANALYSIS */
         (*terminate) = 0;
         return 0;
      }
    }
  }

  if (status == 1) {
    (*terminate) = 0;
#if DEBUG_CONSISTENCY_ANALYSIS
    CONSOLE_DEBUG("Alternative is has % d degrees of freedom.",dof);
#endif /* DEBUG_CONSISTENCY_ANALYSIS */
    if (slvDOF_eligible(server,&(vars))) {
      count = 0;
      while (vars[count] != -1) {
        ind = vars[count];
        svar = vslist[ind];
        v = var_mindex(svar);
        mvar = vmlist[v];
        var_set_eligible_in_subregion(mvar,TRUE);
        count++;
      }
      ASC_FREE(vars);   
    }
    if (dof > count) {
#if DEBUG_CONSISTENCY_ANALYSIS
      CONSOLE_DEBUG("Alternative does not have enough eligible vars");
#endif /* DEBUG_CONSISTENCY_ANALYSIS */
      return 0;
    }
  }

  if (status == 2) {
#if DEBUG_CONSISTENCY_ANALYSIS
    CONSOLE_DEBUG("Alternative is square.");
#endif /* DEBUG_CONSISTENCY_ANALYSIS */
  }

  vfilter.matchbits = (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR
		       | VAR_ELIGIBLE_IN_SUBREGION);
  vfilter.matchvalue = (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR);

  for (v=0; v<mnum; v++) {
    mvar = vmlist[v];
    if(var_apply_filter(mvar,&vfilter)) {
      var_set_eligible(mvar,FALSE);
    }
    var_set_eligible_in_subregion(mvar,FALSE);
  }   
  (*combinations)++;
  return 1;
}


/*------------------------------------------------------------------------------
  CONSISTENCY ANALYSIS for CONDITIONAL MODELS
*/

/**
	Get the eligible set of variables for each of the combinations generated
	by modifying the values of the boolean variables
	
	@return 1 on success, 0 if analysis failed with current partitioning, or 
	-1 if a memory error occurred.
*/
static int32 do_search_consistency_combinations(slv_system_t server,
		struct gl_list_t *disvars,
		int32 dlen, int32 d,
		int32 *combinations,
		int32 *terminate
){
  struct dis_discrete *cur_dis;
  int32 dpo, result;

  if(d < dlen) {
    dpo = d + 1;
    cur_dis = (struct dis_discrete *)(gl_fetch(disvars,d));
    dis_set_boolean_value(cur_dis,TRUE);
    result = do_search_consistency_combinations(server,disvars,dlen,
				            dpo,combinations,terminate);
    if (result != 1) {
      return result;
    }

    dis_set_boolean_value(cur_dis,FALSE);
    result = do_search_consistency_combinations(server,disvars,dlen,
				            dpo,combinations,terminate);
    if (result != 1) {
      return result;
    }

  }else{
    if (d == dlen) {
      cur_dis = (struct dis_discrete *)(gl_fetch(disvars,d));
      dis_set_boolean_value(cur_dis,TRUE);
      result = get_eligible_vars(server,disvars,combinations,terminate);
      if (result != 1) {
        return result;
      }
      dis_set_boolean_value(cur_dis,FALSE);
      result = get_eligible_vars(server,disvars,combinations,terminate);
      if (result != 1) {
        return result;
      }
    }else{
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Wrong discrete var index as argument");
    }
  }
  return 1;
}

/*
 * Storing original values of boolean variables
 */
static void storing_original_bool_values(struct gl_list_t *bollist
		,struct bool_values *bval
){
  struct dis_discrete *dvar;
  int32 d, dlen;

  dlen = gl_length(bollist);
  bval->pre_val = ASC_NEW_ARRAY(int32,dlen);
  bval->cur_val = ASC_NEW_ARRAY(int32,dlen);
  for (d=1; d<=dlen; d++){
    dvar = (struct dis_discrete *)gl_fetch(bollist,d);
    bval->cur_val[d-1] = dis_value(dvar);
    bval->pre_val[d-1] = dis_previous_value(dvar);
  }
}

/*
 * Restoring original values of boolean variables
 */
static void restoring_original_bool_values(struct gl_list_t *bollist
		,struct bool_values *bval
){
  struct dis_discrete *dvar;
  int32 d, dlen;

  dlen = gl_length(bollist);
  for (d=1; d<=dlen; d++){
    dvar = (struct dis_discrete *)gl_fetch(bollist,d);
    dis_set_boolean_value(dvar,bval->cur_val[d-1]);
    dis_set_value(dvar,bval->cur_val[d-1]);
    dis_set_previous_value(dvar,bval->pre_val[d-1]);
  }
  ASC_FREE(bval->cur_val);
  ASC_FREE(bval->pre_val);
}

/*
 * Restoring orignal configuration of the system
 */
static void restore_configuration(slv_system_t server
		,struct gl_list_t *bollist
){
  int32 *cur_cases, *correct_cases;
  int32 ncases;

   cur_cases = cases_matching(bollist,&ncases);
   correct_cases = reorder_cases(cur_cases,ncases);
   set_active_rels_in_subregion(server,correct_cases,ncases,bollist);
   set_active_vars_in_subregion(server);
   ASC_FREE(correct_cases);
}

/*
 * Perform a combinatorial consistency analysis for all the alternatives
 * that we can get from the combination of boolean values for the boolean
 * variables given in the list passed as argument
 */

static int32 perform_combinatorial_consistency_analysis(slv_system_t server
		, struct gl_list_t *bollist
		, int32 *terminate
){
  struct var_variable **vmlist;
  struct var_variable *mvar;
  var_filter_t vfilter;
  int32 *globeli = NULL;
  int32 d, dlen, max_comb, combinations;
  int32 mnum, v, elnum;
  int32 result;
  int32 iter;

  /*
   * Initializing eligible bit for variables
   */
  vmlist = slv_get_master_var_list(server);
  mnum = slv_get_num_master_vars(server);
  for (v=0; v<mnum; v++) {
    mvar = vmlist[v];
    var_set_eligible(mvar,TRUE);
  } 

  dlen = gl_length(bollist);

  max_comb = 1;
  for (d = 1; d<=dlen; d++) {
    max_comb = max_comb * 2;
  }

  d = 1;
  combinations = 0;
#if DEBUG_CONSISTENCY_ANALYSIS
        CONSOLE_DEBUG("S e a r c h i n g");
#endif /* DEBUG_CONSISTENCY_ANALYSIS */
  result = do_search_consistency_combinations(server,bollist,dlen,d,
					      &(combinations),terminate);

  if (result != 1) {
#if DEBUG_CONSISTENCY_ANALYSIS
    CONSOLE_DEBUG("returning failed search after S e a r c h i n g");
#endif /* DEBUG_CONSISTENCY_ANALYSIS */
    return result;
  }  

  /*
   * Getting the "globally" eligible variables
   */
  vfilter.matchbits = (VAR_INCIDENT | VAR_SVAR | VAR_ELIGIBLE | VAR_FIXED);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_ELIGIBLE);
  elnum = slv_count_master_vars(server,&vfilter);

  if (elnum > 0) {
    globeli = ASC_NEW_ARRAY(int32,elnum);
    elnum = 0;
    for (v=0; v<mnum; v++) {
      mvar = vmlist[v];
      if(var_apply_filter(mvar,&vfilter)) {
#if DEBUG_CONSISTENCY_ANALYSIS
        CONSOLE_DEBUG("Eligible index = %d",v);
#endif /* DEBUG_CONSISTENCY_ANALYSIS */
        globeli[elnum] = v;
        elnum++;
      }
    }
  }

  /*
   * Recursively analysis
   */

  if ((*terminate) == 1) {
    if (elnum != 0) {
#if DEBUG_CONSISTENCY_ANALYSIS
      CONSOLE_DEBUG("ERROR: All alternatives are square but the Eligible set is not null");
#endif /* DEBUG_CONSISTENCY_ANALYSIS */
      ASC_FREE(globeli);
    }
    return 1;
  } else {
    if (elnum == 0) {
#if DEBUG_CONSISTENCY_ANALYSIS
      CONSOLE_DEBUG("No globally eligible variables to be fixed.");
#endif /* DEBUG_CONSISTENCY_ANALYSIS */
      return 0;
    }

    for (v=0; v<elnum; v++) {
      iter = 1;
      mvar = vmlist[globeli[v]];
      var_set_fixed(mvar,TRUE);
      var_set_potentially_fixed(mvar,TRUE);
#if DEBUG_CONSISTENCY_ANALYSIS
      CONSOLE_DEBUG("Fixing index = %d",globeli[v]);
      CONSOLE_DEBUG("N e s t e d   S e a r c h");
#endif /* DEBUG_CONSISTENCY_ANALYSIS */
      result = perform_combinatorial_consistency_analysis(server,bollist,
							  &iter);
      if (result != 1) {
#if DEBUG_CONSISTENCY_ANALYSIS
        CONSOLE_DEBUG("%d eliminated",globeli[v]);
#endif /* DEBUG_CONSISTENCY_ANALYSIS */
        var_set_fixed(mvar,FALSE);
        var_set_potentially_fixed(mvar,FALSE);
        continue;
      } else {
        if (iter == 1) {
          (*terminate) = 1;
#if DEBUG_CONSISTENCY_ANALYSIS
          CONSOLE_DEBUG("%d accepted",globeli[v]);
#endif /* DEBUG_CONSISTENCY_ANALYSIS */
          ASC_FREE(globeli);
          return 1;
        } else {
          var_set_fixed(mvar,FALSE);
          var_set_potentially_fixed(mvar,FALSE);
          continue;
        }
      }
    }
    ASC_FREE(globeli);
#if DEBUG_CONSISTENCY_ANALYSIS
    CONSOLE_DEBUG("returning 0 after nested search");
#endif /* DEBUG_CONSISTENCY_ANALYSIS */
    return 0;
  }
}

/*
 * Perform a combinatorial consistency analysis for all the alternatives
 * that we can get from the combination of boolean values for the boolean
 * variables given in the list passed as argument
 */

static int32 get_globally_consistent_set(slv_system_t server,
		struct gl_list_t *bollist,
		int32 **eliset
){
  struct var_variable **vmlist;
  struct var_variable *mvar;
  var_filter_t vfilter;
  int32 d, dlen, max_comb, combinations;
  int32 mnum, v, elnum;
  int32 terminate;
  int32 result;

  /*
   * Initializing eligible bit for variables
   */
  vmlist = slv_get_master_var_list(server);
  mnum = slv_get_num_master_vars(server);
  for (v=0; v<mnum; v++) {
    mvar = vmlist[v];
    var_set_eligible(mvar,TRUE);
  } 
  
  dlen = gl_length(bollist);

  max_comb = 1;
  for (d = 1; d<=dlen; d++) {
    max_comb = max_comb * 2;
  }

  /*
   * initializing
   */ 
  *eliset = NULL;
  terminate = 1;

  d = 1;
  combinations = 0;
  result = do_search_consistency_combinations(server,bollist,dlen,d,
					      &(combinations),&terminate);
  if (result != 1) {
    if (terminate == 0) {
#if DEBUG_CONSISTENCY_ANALYSIS
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"ERROR: some alternatives are either singular or"
		" overspecified. All the alternatives have to be"
		" either square or underspecified to complete the analysis"
	  );
#endif /* DEBUG_CONSISTENCY_ANALYSIS */
    }
    return 0;
  }

  /*
   * Getting the "globally" eligible variables
   */
  vfilter.matchbits = (VAR_INCIDENT | VAR_SVAR | VAR_ELIGIBLE | VAR_FIXED);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_ELIGIBLE);
  elnum = slv_count_master_vars(server,&vfilter);

  *eliset = (int32 *)ascmalloc((elnum+1)*sizeof(int32));
  elnum = 0;
  for (v=0; v<mnum; v++) {
    mvar = vmlist[v];
    if(var_apply_filter(mvar,&vfilter)) {
      (*eliset)[elnum] = v;
      elnum++;
    }
  }
  (*eliset)[elnum] = -1;

  if(elnum == 0) {
    if(terminate == 0) {
#if DEBUG_CONSISTENCY_ANALYSIS
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Some alternatives are underspecified, but there does"
		"not exist a set of eligible variables consistent "
		"with all the alternatives"
	  );
#endif /* DEBUG_CONSISTENCY_ANALYSIS */
    }else{
#if DEBUG_CONSISTENCY_ANALYSIS
      CONSOLE_DEBUG("All alternatives are already square");
#endif /* DEBUG_CONSISTENCY_ANALYSIS */
    }
    return 0;
  }else{
    if (terminate == 1) {
#if DEBUG_CONSISTENCY_ANALYSIS
      CONSOLE_DEBUG("All alternatives are square but the Eligible set is not null");
#endif /* DEBUG_CONSISTENCY_ANALYSIS */
    }
  }
  return 1;
}


/**
	Get the list of boolean variables which can potentially cause
	a change in the structure of the problem
*/
static 
struct gl_list_t *get_list_of_booleans_for_analysis(slv_system_t server){

  dis_filter_t dfilter;
  struct dis_discrete **boolist;
  struct dis_discrete *dvar;
  struct gl_list_t *bana;
  struct gl_list_t *whens;
  struct w_when *when;
  int32 d, dnum, w, wlen, numdcs;

  boolist = slv_get_solvers_dvar_list(server);
  dnum = slv_get_num_solvers_dvars(server);
  for (d=0; d<dnum; d++) {
    dvar = boolist[d];
    if(dis_inwhen(dvar) && dis_boolean(dvar)) {
      whens = dis_whens_list(dvar);
      if ( (whens != NULL) && (gl_length(whens) != 0)) {
        dis_set_changes_structure(dvar,FALSE);
        wlen = gl_length(whens);
        for (w=1; w<=wlen; w++) {
          when = (struct w_when *)gl_fetch(whens,w);
          if (when_changes_structure(when)) {
            dis_set_changes_structure(dvar,TRUE);
            break;
	  }      
	}
      } else {
        dis_set_changes_structure(dvar,FALSE);
      }
    }
  }

  dfilter.matchbits = (DIS_CHANGES_STRUCTURE);
  dfilter.matchvalue = (DIS_CHANGES_STRUCTURE);
  numdcs = slv_count_solvers_dvars(server,&dfilter);

  bana = gl_create(numdcs);
  for (d=0;d<dnum; d++) {
    dvar = boolist[d];
    if (dis_changes_structure(dvar)) {
      gl_append_ptr(bana,dvar);
    }
  }

  return bana;
}

/*
 * Get a set of eligible variables consitent for all the alternative
 * configurations
 */
int32 get_globally_consistent_eligible(slv_system_t server,int32 **eliset){

  struct dis_discrete **boolist;
  struct dis_discrete *dvar;
  struct gl_list_t *bana;
  struct bool_values bval;
  int32 need_consistency_analysis;
  int32 result, d, dnum;

  if (server==NULL || eliset == NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"get_globally_consistent_eligible called with NULL.");
    return 0;
  }

  boolist = slv_get_solvers_dvar_list(server);
  dnum = slv_get_num_solvers_dvars(server);
  need_consistency_analysis = slv_need_consistency(server);

  if(boolist == NULL ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Discrete Variable list was never set.");
    return 0;
  }

  if (!(need_consistency_analysis)) {
    ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Global analysis is not required. All the alternatives have the same structure.");
    return 1;
  }

  bana = get_list_of_booleans_for_analysis(server);

  if (bana == NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"List of boolean vars could not be found");
    return 0;
  }

  storing_original_bool_values(bana,&(bval));
  result = get_globally_consistent_set(server,bana,eliset);

  restoring_original_bool_values(bana,&(bval));
  restore_configuration(server,bana);
  for (d=0;d<dnum; d++) {
    dvar = boolist[d];
    if (dis_changes_structure(dvar)) {
      dis_set_changes_structure(dvar,FALSE);
    }
  }
  gl_destroy(bana);

  if (result == 1) {
    return 1;
  } else {
    return 0;
  }

}

/**
	Main function of the consistency analysis.
*/
int32 consistency_analysis(slv_system_t server,int32 **fixed){

  struct dis_discrete **boolist;
  struct dis_discrete *dvar;
  struct var_variable **vmlist;
  struct var_variable *mvar;  
  struct gl_list_t *bana;
  var_filter_t vfilter;
  struct bool_values bval;
  int32 need_consistency_analysis;
  int32 result, elnum, mnum, v;
  int32 terminate;
  int32 d, dnum;

  if (server==NULL || fixed == NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"consistency_analysis called with NULL.");
    return 0;
  }

  boolist = slv_get_solvers_dvar_list(server);
  dnum = slv_get_num_solvers_dvars(server);
  need_consistency_analysis = slv_need_consistency(server);;

  if(boolist == NULL ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Discrete Variable list was never set.");
    return 0;
  }

  if (!(need_consistency_analysis)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Consistency analysis is not required."
		" All the alternatives have the same structure");
    return 1;
  }

  bana = get_list_of_booleans_for_analysis(server);

  if (bana == NULL || gl_length(bana) == 0) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"List of boolean vars could not be found.");
    return 0;
  }

  storing_original_bool_values(bana,&(bval));

  /*
   * initializing
   */ 
  *fixed = NULL;
  terminate = 1;

  vmlist = slv_get_master_var_list(server);
  mnum = slv_get_num_master_vars(server);

  vfilter.matchbits = (VAR_POTENTIALLY_FIXED);
  vfilter.matchvalue = (VAR_POTENTIALLY_FIXED);

  result = perform_combinatorial_consistency_analysis(server,bana,&terminate);
  CONSOLE_DEBUG("perform_combinatorial_consistency_analysis gave %d",result);
  if (result == 1) {
  /*
   * Getting the set of eligible variables
   */
    elnum = slv_count_master_vars(server,&vfilter);
    *fixed = ASC_NEW_ARRAY(int32,elnum+1);
    elnum = 0;
    for (v=0; v<mnum; v++) {
      mvar = vmlist[v];
      if(var_apply_filter(mvar,&vfilter)) {
        var_set_fixed(mvar,FALSE);
        var_set_potentially_fixed(mvar,FALSE);
        (*fixed)[elnum] = v;
        elnum++;
      }
    }
    (*fixed)[elnum] = -1;
	CONSOLE_DEBUG("fixed[%d]=-1",elnum);

    for (d=0;d<dnum; d++) {
      dvar = boolist[d];
      if (dis_changes_structure(dvar)) {
        dis_set_changes_structure(dvar,FALSE);
      }
    }
    restoring_original_bool_values(bana,&(bval));
    restore_configuration(server,bana);
    gl_destroy(bana);
    return 1;
  } else {
    for (v=0; v<mnum; v++) {
      mvar = vmlist[v];
      if(var_apply_filter(mvar,&vfilter)) {
        var_set_fixed(mvar,FALSE);
        var_set_potentially_fixed(mvar,FALSE);
      }
    }

    for (d=0;d<dnum; d++) {
      dvar = boolist[d];
      if (dis_changes_structure(dvar)) {
        dis_set_changes_structure(dvar,FALSE);
      }
    }
    restoring_original_bool_values(bana,&(bval));
    restore_configuration(server,bana);
    gl_destroy(bana);
    return 0;
  }

}


