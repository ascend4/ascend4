/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1990 Karl Michael Westerberg

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
	Relation manipulator module for the SLV solver.
*//*
	by Karl Michael Westerberg. Created 2/6/90
	Last in CVS: $Revision: 1.44 $ $Date: 1998/04/23 23:56:22 $ $Author: ballan $
*/

#include "relman.h"

#include <math.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <general/list.h>
#include <general/mathmacros.h>

#include <compiler/instance_enum.h>
#include <compiler/functype.h>
#include <compiler/safe.h>
#include <compiler/rootfind.h>
#include <compiler/extfunc.h>
#include <compiler/expr_types.h>
#include <compiler/find.h>
#include <compiler/atomvalue.h>
#include <compiler/mathinst.h>
#include <compiler/rel_blackbox.h>
#include <compiler/vlist.h>
#include <compiler/relation.h>
#include <compiler/relation_util.h>
#include <compiler/relation_io.h>

#include "slv_server.h"

/* #define DIFF_DEBUG */
/* #define EVAL_DEBUG */

#define IPTR(i) ((struct Instance *)(i))

#define KILL 0 /* compile dead code if kill = 1 */
#define REIMPLEMENT 0 /* code that needs to be reimplemented */


/**
	Temporarily allocates a given number of bytes.  The memory need
	not be freed, but the next call to this function will reuse the
	previous allocation. Memory returned will NOT be zeroed.
	Calling with nbytes==0 will free any memory allocated.
*/
static
void *rel_tmpalloc( int nbytes){
  static char *ptr = NULL;
  static int cap = 0;

  if (nbytes) {
    if( nbytes > cap ) {
      if( ptr != NULL ) ascfree(ptr);
      ptr = ASC_NEW_ARRAY(char,nbytes);
      cap = nbytes;
    }
  }else{
    if (ptr) ascfree(ptr);
    ptr=NULL;
    cap=0;
  }
  if ( cap >0) {
    return(ptr);
  } else  {
    return NULL;
  }
}


#define rel_tmpalloc_array(nelts,type)  \
   ((nelts) > 0 ? (type *)tmpalloc((nelts)*sizeof(type)) : NULL)
/**<
	Creates an array of "nelts" objects, each with type "type".
*/


void relman_free_reused_mem(void){
  /* rel_tmpalloc(0); */
  RelationFindRoots(NULL,0,0,0,0,NULL,NULL,NULL);
}


#if REIMPLEMENT
boolean relman_is_linear( struct rel_relation *rel, var_filter_t *filter){
   return (
      exprman_is_linear(rel,rel_lhs(rel),filter) &&
      exprman_is_linear(rel,rel_rhs(rel),filter)
   );
}

real64 relman_linear_coef(struct rel_relation *rel, struct var_variable *var
		, var_filter_t *filter
){
   return(
      exprman_linear_coef(rel,rel_lhs(rel),var,filter) -
      exprman_linear_coef(rel,rel_rhs(rel),var,filter)
   );
}
#endif


#if KILL
void relman_decide_incidence( struct rel_relation *rel){
  struct var_variable **list;
  int c;

  list = rel_incidence_list(rel);
  for( c=0; list[c] != NULL; c++ )
     var_set_incident(list[c],TRUE);
}
#endif


void relman_get_incidence(struct rel_relation *rel, var_filter_t *filter
		, mtx_matrix_t mtx
){
  const struct var_variable **list;
  mtx_coord_t nz;
  int c,len;

  assert(rel!=NULL && filter !=NULL && mtx != NULL);
  nz.row = rel_sindex(rel);
  len = rel_n_incidences(rel);

  list = rel_incidence_list(rel);
  for (c=0; c < len; c++) {
     if( var_apply_filter(list[c],filter) ) {
	nz.col = var_sindex(list[c]);
	mtx_fill_org_value(mtx,&nz,1.0);
     }
  }
}


#ifdef RELOCATE_GB_NEEDED
/*
 *********************************************************************
 * Code to deal with glassbox relations processing.
 *********************************************************************
 */

static double dsolve_scratch = 0.0;		/* some workspace */
#define DSOLVE_TOLERANCE 1.0e-08                /* no longer needed */

static
real64 *relman_glassbox_dsolve(struct rel_relation *rel
		, struct var_variable *solvefor
		, int *able
		, int *nsolns
		, real64 tolerance
){
  int n,m,j,dummy = 0;
  int mode, result;
  int index,solve_for_index = -1;
  struct Instance *var;
  CONST struct relation *cmplr_reln;
  enum Expr_enum type;
  CONST struct gl_list_t *vlist;
  double *f, *x, *g, value;
  double lower, upper, nominal;
  ExtEvalFunc **table;

  cmplr_reln = GetInstanceRelation(rel_instance(rel),&type);
  assert(type==e_glassbox);

  vlist = RelationVarList(cmplr_reln);
  m = rel_sindex(rel);
  n = (int)gl_length(vlist);
  f = ASC_NEW_ARRAY_CLEAR(double,(1 + 2*n));
  x = &f[1];
  g = &f[n+1];

  /*
   * Load x vector, and get the index into the x[vector]
   * of the variable that we are solving for.
   */
  for (j=0;j<n;j++) {
    var = (struct Instance *)gl_fetch(vlist,(unsigned long)(j+1));
    x[j] = RealAtomValue(var);
    if (var_instance(solvefor)== var)
      solve_for_index = j;
  }

  /*
   * Set up bounds and tolerance.
   */
  lower = var_lower_bound(solvefor);
  upper = var_upper_bound(solvefor);
  nominal = var_nominal(solvefor);
/*tolerance =  DSOLVE_TOLERANCE; now passed in. */

  /*
   * Get the evaluation function.
   */
  table = GetValueJumpTable(GlassBoxExtFunc(cmplr_reln));
  index = GlassBoxRelIndex(cmplr_reln);

  /** @TODO FIXME
	Call the rootfinder. A note of CAUTION:
	zbrent is going to call the evaluation function.
	It expects, that the results of this evaluation will be
	written to f[m]. GlassBox relations however always
	write to slot f[0]. To keep the world happy we send in
	a dummy = 0.  This needs to be made cleaner.
  */
  value = zbrent(table[index], &lower, &upper,
		 &mode, &dummy, &solve_for_index,
		 x, x , f, g, &tolerance, &result);
  if (result==0) {
    dsolve_scratch = value;
    ascfree((char *)f);
    *able = TRUE;
    *nsolns = 1;
    return &dsolve_scratch;
  }
  else{
    dsolve_scratch = 0.0;
    ascfree((char *)f);
    *able = FALSE;
    *nsolns = 0;
    return NULL;
  }
}


#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
real64 relman_glassbox_eval(struct rel_relation *rel){
  int n,m,mode,result;
  int index,j;
  CONST struct relation *cmplr_reln;
  enum Expr_enum type;
  CONST struct gl_list_t *vlist;
  double *f, *x, *g, value;
  ExtEvalFunc **table, *eval;

  cmplr_reln = GetInstanceRelation(rel_instance(rel),&type);
  assert(type==e_glassbox);

  vlist = RelationVarList(cmplr_reln);
  m = rel_sindex(rel);
  n = (int)gl_length(vlist);
  /* this needs to be reused memory, fergossake */
  f = ASC_NEW_ARRAY_CLEAR(double,(1 + 2*n));	/* resid */
  x = &f[1];						/* var values */
  g = &f[n+1];						/* gradient */

  for (j=0;j<n;j++) {
    x[j] = RealAtomValue(gl_fetch(vlist,(unsigned long)(j+1)));
  }

  table = GetValueJumpTable(GlassBoxExtFunc(cmplr_reln));
  index = GlassBoxRelIndex(cmplr_reln);

  /*
   * Remember to set up the fpe traps etc ....
   * and to monitor the return flag.
   */
  mode = 0;
  eval = table[index];
  result = (*eval)(&mode,&m,&n,x,NULL,f,g);
  value = f[0];
  ascfree((char *)f);
  return value;
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */


#define BROKENKIRK 0
#if BROKENKIRK
/* fills filter passing gradient elements to matrix */
/* this needs to be buried on the compiler side. */
void relman_map_grad2mtx( struct rel_relation *rel, var_filter_t *filter,
  mtx_matrix_t mtx, CONST struct gl_list_t *varlist, double *g, int m, int n
){
  mtx_coord_t nz;
  struct var_variable *var;
  double value;
  int j;

  nz.row = m; /* org row of glassbox reln? rel_sindex? */

  for(j=0;j<n;j++){
/*    var = (struct Instance *)gl_fetch(varlist,(unsigned long)(j+1)); */
    var = rel_incidence(rel)[j];
    if (var_apply_filter(var)) {
      nz.col = var_sindex(var);
      value = g[j] /* + mtx_value(mtx,&nz) no longer incrementing row */;
      mtx_fill_org_value(mtx,&nz, value);
    }
  }
}

real64 relman_glassbox_diffs( struct rel_relation *rel,
		var_filter_t *filter, mtx_matrix_t mtx
){
  int n,m,mode,result;
  int index,j;
  struct Instance *var;
  CONST struct relation *cmplr_reln;
  enum Expr_enum type;
  CONST struct gl_list_t *vlist;
  double *f, *x, *g, value;
  ExtEvalFunc **table, *eval;

  cmplr_reln = GetInstanceRelation(rel_instance(rel),&type);
  vlist = RelationVarList(cmplr_reln);
  m = rel_sindex(rel);
  n = (int)gl_length(vlist);
  /* this needs to be reused memory ! */
  f = ASC_NEW_ARRAY_CLEAR(double,(1 + 2*n));
  x = &f[1];
  g = &f[n+1];

  for (j=0;j<n;j++) {
    x[j] = RealAtomValue(gl_fetch(vlist,(unsigned long)j+1));
  }

  table = GetValueJumpTable(GlassBoxExtFunc(cmplr_reln));
  index = GlassBoxRelIndex(cmplr_reln);
  eval = table[index];
  result = (*eval)(0,&m,&n,x,NULL,f,g);

  table = GetDerivJumpTable(GlassBoxExtFunc(cmplr_reln));

  mode = 0;
  eval = table[index];
  result += (*eval)(&mode,&m,&n,x,NULL,f,g);
  relman_map_grad2mtx(rel,filter,mtx,vlist,g,m,n);

  /*
   * Remember to set up the fpe traps etc ....
   * and to monitor the return flag.
   */
  value = f[0];
  ascfree((char *)f);
  return value;
}

#else
#define relman_glassbox_diffs(rel,filter,mtx) abort()
#endif

#endif /* RELOCATE_GB_NEEDED */


real64 relman_eval(struct rel_relation *rel, int32 *calc_ok, int safe){
	real64 res;
	asc_assert(calc_ok!=NULL && rel!=NULL);
	if(rel->type == e_rel_token){
		if(!RelationCalcResidualBinary(
		        GetInstanceRelationOnly(IPTR(rel->instance)),&res)
		){
			*calc_ok = 1; /* calc_ok */
			rel_set_residual(rel,res);
			return res;
		}/* else {
			we don't care -- go on to the old handling which
			is reasonably correct, if slow.
		} */
	}

	if(safe){
		*calc_ok = RelationCalcResidualSafe(rel_instance(rel),&res);
		if(*calc_ok){
			/* this actually means there was an ERROR due to return protocol of ^^^ */
#ifdef EVAL_DEBUG
			CONSOLE_DEBUG("residual error, res = %g",res);
#endif
			safe_error_to_stderr( (enum safe_err *)calc_ok );
			*calc_ok = 0; /* error was returned: not ok */
		}else{
			*calc_ok = 1; /* all good */
		}
		/* always set the relation residual when using safe functions */
		rel_set_residual(rel,res);
		if(!(*calc_ok))CONSOLE_DEBUG("RELMAN_EVAL WAS NOT OK");
		return res;
	}

	*calc_ok = RelationCalcResidual(rel_instance(rel),&res);
	if(*calc_ok){
		/* an error occured */
		res = 1.0e8;
	}else{
		/* no error */
		rel_set_residual(rel,res);
	}
	*calc_ok = !(*calc_ok);
	if(!(*calc_ok))CONSOLE_DEBUG("RELMAN_EVAL WAS NOT OK");
	return res;
}


int32 relman_obj_direction(struct rel_relation *rel){
  assert(rel!=NULL);
  switch( RelationRelop(GetInstanceRelationOnly(IPTR(rel->instance))) ) {
  case e_minimize:
    return -1;
  case e_maximize:
    return 1;
  default:
    return 0;
  }
}


real64 relman_scale(struct rel_relation *rel){
  real64 relnom;
  assert(rel!=NULL);
  relnom = CalcRelationNominal(rel_instance(rel));
  if(relnom < 0.0000001) {
    /* take care of small relnoms and relnom = 0 error returns */
    relnom = 1.0;
  }
  rel_set_nominal(rel,relnom);
  return relnom;
}


#if REIMPLEMENT /* compiler */
real64 relman_diff(struct rel_relation *rel, struct var_variable *var,
                   int safe
){
		/* FIX FIX FIX meaning kirk couldn't be botghered... */
   real64 res = 0.0;
   switch(rel->type) {
   case e_glassbox:
   case e_blackbox:
     FPRINTF(stderr,"relman_diff not yet supported for black and glassbox\n");
     return 1.0e08;
   }
   res -= exprman_diff(rel,rel_rhs(rel),var);
   res += exprman_diff(rel,rel_lhs(rel),var);
   return( res );
}
#endif


/* return 0 on success */
int relman_diff2(struct rel_relation *rel, const var_filter_t *filter
		,real64 *derivatives, int32 *variables
		,int32 *count, int32 safe
){
  const struct var_variable **vlist=NULL;
  real64 *gradient;
  int32 len,c;
  int status;

  assert(rel!=NULL && filter!=NULL);
  len = rel_n_incidences(rel);
  vlist = rel_incidence_list(rel);

  gradient = (real64 *)rel_tmpalloc(len*sizeof(real64));
  assert(gradient !=NULL);
  *count = 0;
  if(safe){
    status =(int32)RelationCalcGradientSafe(rel_instance(rel),gradient);
    safe_error_to_stderr( (enum safe_err *)&status );
    /* always map when using safe functions */
    for (c=0; c < len; c++) {
      if (var_apply_filter(vlist[c],filter)) {
        variables[*count] = var_sindex(vlist[c]);
        derivatives[*count] = gradient[c];
        /* CONSOLE_DEBUG("Var %d = %f",var_sindex(vlist[c]),gradient[c]); */
        (*count)++;
      }
    }
	/* CONSOLE_DEBUG("RETURNING (SAFE) calc_ok=%d",status); */
	return status;
  }else{
    if((status=RelationCalcGradient(rel_instance(rel),gradient)) == 0) {
      /* successful */
      for (c=0; c < len; c++) {
        if (var_apply_filter(vlist[c],filter)) {
          variables[*count] = var_sindex(vlist[c]);
          derivatives[*count] = gradient[c];
          (*count)++;
        }
      }
    }
	/* SOLE_DEBUG("RETURNING (NON-SAFE) calc_ok=%d",status); */
 	return status;
  }
}


/* return 0 on success */
int relman_diff3(struct rel_relation *rel
		, const var_filter_t *filter
		, real64 *derivatives, struct var_variable **variables
		, int32 *count, int32 safe
){
  struct var_variable **vlist=NULL;
  real64 *gradient;
  int32 len,c;
  int status;

  assert(rel!=NULL && filter!=NULL);
  len = rel_n_incidences(rel);
  vlist = (struct var_variable**)rel_incidence_list(rel);

  gradient = (real64 *)rel_tmpalloc(len*sizeof(real64));
  assert(gradient !=NULL);
  *count = 0;
  if(safe){
#ifdef DIFF_DEBUG
	CONSOLE_DEBUG("SAFE EVALUATION");
#endif
    status =(int32)RelationCalcGradientSafe(rel_instance(rel),gradient);
    safe_error_to_stderr( (enum safe_err *)&status );
    /* always map when using safe functions */
    for (c=0; c < len; c++) {
      if (var_apply_filter(vlist[c],filter)) {
        variables[*count] = vlist[c];
        derivatives[*count] = gradient[c];
        /* CONSOLE_DEBUG("Var %d = %f",var_sindex(vlist[c]),gradient[c]); */
        (*count)++;
      }
    }
	/* CONSOLE_DEBUG("RETURNING (SAFE) calc_ok=%d",status); */
	return status;
  }else{
#ifdef DIFF_DEBUG
	CONSOLE_DEBUG("UNSAFE EVALUATION");
#endif
    if((status=RelationCalcGradient(rel_instance(rel),gradient)) == 0) {
      /* successful */
      for (c=0; c < len; c++) {
        if (var_apply_filter(vlist[c],filter)) {
          variables[*count] = vlist[c];
          derivatives[*count] = gradient[c];
          (*count)++;
        }
      }
    }
	/* SOLE_DEBUG("RETURNING (NON-SAFE) calc_ok=%d",status); */
 	return status;
  }
}


int relman_diff_grad(struct rel_relation *rel
		, const var_filter_t *filter
		, real64 *derivatives, int32 *variables_master
		, int32 *variables_solver, int32 *count, real64 *resid
		, int32 safe
){
  const struct var_variable **vlist=NULL;
  real64 *gradient;
  int32 len,c;
  int status;

  assert(rel!=NULL && filter!=NULL);
  len = rel_n_incidences(rel);
  vlist = rel_incidence_list(rel);

  gradient = (real64 *)rel_tmpalloc(len*sizeof(real64));
  assert(gradient !=NULL);
  *count = 0;
  if( safe ) {
	/* CONSOLE_DEBUG("..."); */
    status =(int32)RelationCalcResidGradSafe(rel_instance(rel),
					     resid,gradient);
    safe_error_to_stderr( (enum safe_err *)&status );
    /* always map when using safe functions */
    for (c=0; c < len; c++) {
      if (var_apply_filter(vlist[c],filter)) {
        variables_master[*count] = var_mindex(vlist[c]);
        variables_solver[*count] = var_sindex(vlist[c]);
	derivatives[*count] = gradient[c];
	(*count)++;
      }
    }
  }
  else {
    if((status=RelationCalcResidGrad(rel_instance(rel),resid,gradient))== 0) {
      /* successful */
      for (c=0; c < len; c++) {
        if (var_apply_filter(vlist[c],filter)) {
	  variables_master[*count] = var_mindex(vlist[c]);
	  variables_solver[*count] = var_sindex(vlist[c]);
	  derivatives[*count] = gradient[c];
	  (*count)++;
        }
      }
    }
  }

  return !status;  /* flip the status flag */
}


int32 relman_diff_harwell(struct rel_relation **rlist
		, var_filter_t *vfilter, rel_filter_t *rfilter
		, int32 rlen, int32 bias, int32 mORs
		, real64 *avec, int32 *ivec, int32 *jvec
){
  const struct var_variable **vlist = NULL;
  struct rel_relation *rel;
  real64 residual, *resid;
  real64 *gradient;
  mtx_coord_t coord;
  int32 len,c,r,k;
  int32 errcnt;
  enum safe_err status;

  if(rlist == NULL || vfilter == NULL || rfilter == NULL || avec == NULL
      || rlen < 0 || mORs >3 || mORs < 0 || bias <0 || bias > 1
  ){
    return 1;
  }
  resid = &residual;
  errcnt = k = 0;

  if(ivec==NULL || jvec==NULL){
    /*_skip stuffing ivec,jvec */
    for (r=0; r < rlen; r++) {
      rel = rlist[r];
      len = rel_n_incidences(rel);
      vlist = rel_incidence_list(rel);
      gradient = (real64 *)rel_tmpalloc(len*sizeof(real64));
      if (gradient == NULL) {
        return 1;
      }
	  /* CONSOLE_DEBUG("..."); */
      status = RelationCalcResidGradSafe(rel_instance(rel),resid,gradient);
      safe_error_to_stderr(&status);
      if (status) {
        errcnt--;
      }
      for (c=0; c < len; c++) {
        if (var_apply_filter(vlist[c],vfilter)) {
          avec[k] = gradient[c];
          k++;
        }
      }
    }
  }else{
    for (r=0; r < rlen; r++) {
      rel = rlist[r];
      len = rel_n_incidences(rel);
      vlist = rel_incidence_list(rel);
      gradient = (real64 *)rel_tmpalloc(len*sizeof(real64));
      if (gradient == NULL) {
        return 1;
      }
	  /* CONSOLE_DEBUG("..."); */
      status = RelationCalcResidGradSafe(rel_instance(rel),resid,gradient);
      safe_error_to_stderr(&status);
      if (status) {
        errcnt--;
      }
      if (mORs & 2) {
        coord.row = rel_sindex(rel);
      }else{
        coord.row = rel_mindex(rel);
      }
      for (c=0; c < len; c++) {
      if (var_apply_filter(vlist[c],vfilter)) {
          if (mORs & 1) {
            coord.col = var_sindex(vlist[c]);
          }else{
              coord.col = var_mindex(vlist[c]);
          }
          avec[k] = gradient[c];
          ivec[k] = coord.row;
          jvec[k] = coord.col;
          k++;
        }
      }
    }
  }
  return errcnt;
}


int32 relman_jacobian_count(struct rel_relation **rlist, int32 rlen
		, var_filter_t *vfilter
		, rel_filter_t *rfilter, int32 *max
){
  int32 len, result=0, row, count, c;
  const struct var_variable **list;
  struct rel_relation *rel;
  *max = 0;

  for( row = 0; row < rlen; row++ ) {
    rel = rlist[row];
    if (rel_apply_filter(rel,rfilter)) {
      len = rel_n_incidences(rel);
      list = rel_incidence_list(rel);
      count = 0;
      for (c=0; c < len; c++) {
        if( var_apply_filter(list[c],vfilter) ) {
          count++;
        }
      }
      result += count;
      *max = MAX(*max,count);
    }
  }
  return result;
}


int relman_diffs(struct rel_relation *rel
		, const var_filter_t *filter
		, mtx_matrix_t mtx, real64 *resid, int safe
){
  const struct var_variable **vlist=NULL;
  real64 *gradient;
  int32 len,c;
  mtx_coord_t coord;
  int status;

  assert(rel!=NULL && filter!=NULL && mtx != NULL);
  len = rel_n_incidences(rel);
  vlist = rel_incidence_list(rel);
  coord.row = rel_sindex(rel);
  assert(coord.row>=0 && coord.row < mtx_order(mtx));

  gradient = (real64 *)rel_tmpalloc(len*sizeof(real64));
  assert(gradient !=NULL);
  if( safe ) {
    status =(int32)RelationCalcResidGradSafe(rel_instance(rel),resid,gradient);
    safe_error_to_stderr( (enum safe_err *)&status );
    /* always map when using safe functions */
    for (c=0; c < len; c++) {
      if (var_apply_filter(vlist[c],filter)) {
        coord.col = var_sindex(vlist[c]);
        assert(coord.col >= 0 && coord.col < mtx_order(mtx));
        mtx_fill_org_value(mtx,&coord,gradient[c]);
      }
    }
  }
  else {
    if((status=RelationCalcResidGrad(rel_instance(rel),resid,gradient)) == 0) {
      /* successful */
      for (c=0; c < len; c++) {
        if (var_apply_filter(vlist[c],filter)) {
          coord.col = var_sindex(vlist[c]);
          assert(coord.col >= 0 && coord.col < mtx_order(mtx));
          mtx_fill_org_value(mtx,&coord,gradient[c]);
        }
      }
    }
  }
  /* flip the status flag */
  return !status;
}


#if REIMPLEMENT /* this needs to be reimplemented in the compiler */
real64 relman_diffs_orig( struct rel_relation *rel, var_filter_t *filter
		,mtx_matrix_t mtx
){
  real64 res = 0.0;
  int32 row;
  row = mtx_org_to_row(mtx,rel_sindex(rel));
  if (rel_extnodeinfo(rel)) {
    res -= ExtRel_Diffs_RHS(rel,filter,row,mtx);
    mtx_mult_row(mtx,row,-1.0,mtx_ALL_COLS);
    res += ExtRel_Diffs_LHS(rel,filter,row,mtx);
    return res;
  }else{
    res -= exprman_diffs(rel,rel_rhs(rel),filter,row,mtx);
    mtx_mult_row(mtx,row,-1.0,mtx_ALL_COLS);
    res += exprman_diffs(rel,rel_lhs(rel),filter,row,mtx);
    return(res);
  }
}
#endif


boolean relman_calc_satisfied( struct rel_relation *rel, real64 tolerance){
   real64 res;
   res = rel_residual(rel);
   if (!asc_finite(res)) {
      rel_set_satisfied(rel,FALSE);
      return( rel_satisfied(rel) );
   }
   if( res < 0.0 ) {
      if( rel_less(rel) ) {
         rel_set_satisfied(rel,TRUE);
         return( rel_satisfied(rel) );
      }
      if( rel_greater(rel) ) {
         rel_set_satisfied(rel,FALSE);
         return( rel_satisfied(rel) );
      }
      rel_set_satisfied(rel,(-res <= tolerance ));
      return( rel_satisfied(rel) );
   }
   if( res > 0.0 ) {
      if( rel_greater(rel) ) {
         rel_set_satisfied(rel,TRUE);
         return( rel_satisfied(rel) );
      }
      if( rel_less(rel) ) {
         rel_set_satisfied(rel,FALSE);
         return( rel_satisfied(rel) );
      }
      rel_set_satisfied(rel,(res <= tolerance ));
      return( rel_satisfied(rel) );
   }
   rel_set_satisfied(rel,rel_equal(rel)); /* strict >0 or <0 not satisfied */
   return( rel_satisfied(rel) );
}

boolean relman_calc_satisfied_scaled(struct rel_relation *rel, real64 tolerance)
{
   real64 res;
   res = rel_residual(rel);

   if( res < 0.0 )
      if( rel_less(rel) )
         rel_set_satisfied(rel,TRUE);
      else if( rel_greater(rel) )
         rel_set_satisfied(rel,FALSE);
      else
         rel_set_satisfied(rel,(-res <= tolerance * rel_nominal(rel)));
   else if( res > 0.0 )
      if( rel_greater(rel) )
         rel_set_satisfied(rel,TRUE);
      else if( rel_less(rel) )
         rel_set_satisfied(rel,FALSE);
      else
         rel_set_satisfied(rel,(res <= tolerance * rel_nominal(rel)));
   else
      rel_set_satisfied(rel,rel_equal(rel));
   return( rel_satisfied(rel) );
}


#if REIMPLEMENT
real64 *relman_directly_solve_new( struct rel_relation *rel
		,struct var_variable *solvefor, int *able, int *nsolns
		,real64 tolerance
){
  double *value;
   if( rel_less(rel) || rel_greater(rel) || !rel_equal(rel) ||
      rel_extnodeinfo(rel)) {
      *able = FALSE;
      *nsolns = 0;
      return(NULL);
   }
   else if (rel->type == e_glassbox) {
     value = relman_glassbox_dsolve(rel,solvefor,able,nsolns,tolerance);
     return value;
   }
   else{
     value =
       exprman_directly_solve(rel,rel_lhs(rel),rel_rhs(rel),
           solvefor,able,nsolns);
     return value;
   }
}
#else /* temporary */
real64 *relman_directly_solve_new( struct rel_relation *rel,
		struct var_variable *solvefor, int *able, int *nsolns, real64 tolerance
){
	double *value;
	if(rel_less(rel) ||
	       rel_greater(rel) ||
	       !rel_equal(rel) ||
	       rel_extnodeinfo(rel)
	){
		/* fall through; not able to directly solve */
	}else{
	   switch (rel->type ) {
			case e_rel_token:
				{
					int nvars,n;
					const struct var_variable **vlist;
					unsigned long vindex; /* index to the compiler */
					nvars = rel_n_incidences(rel);
					vlist = rel_incidence_list(rel);
					vindex = 0;
					for (n=0; n < nvars; n++) {
					    if (vlist[n]==solvefor) {
						    vindex = n+1; /*compiler counts from 1 */
						    break;
					    }
					}
					value = RelationFindRoots(IPTR(rel_instance(rel))
							, var_lower_bound(solvefor)
							, var_upper_bound(solvefor)
							, var_nominal(solvefor)
							, tolerance, &(vindex), able, nsolns
					);
					return value;
				}
			case e_rel_blackbox:
				CONSOLE_DEBUG("Attmpeting direct solve of blackbox");
				return blackbox_dsolve(
						IPTR(rel_instance(rel))
						,IPTR(var_instance(solvefor))
						,able,nsolns
				);
				/*
					we *could* directly solve if the solvefor variable is the output
					of this particular relation, but we don't support that at this stage
				*/
				break;
#if 0
			case e_rel_glassbox:
				/* I think glassbox functionality might be dead at the moment? */
				break;
#endif 
			default:
				/* anything else? */
				break;
		}
	}
	*able = FALSE;
	*nsolns = 0;
	return(NULL);	
}
#endif


char *dummyrelstring(slv_system_t sys, struct rel_relation *rel, int style){
  char *result;
  UNUSED_PARAMETER(sys);
  UNUSED_PARAMETER(rel);
  UNUSED_PARAMETER(style);
  result = ASC_NEW_ARRAY(char,80);
  sprintf(result,"relman_make_*string_*fix not implemented yet");
  return result;
}

/*
 * converst the relations vlist into an array of int indices suitable
 * for relation write string (compiler side indexing from 1, remember).
 * if mORs == TRUE master indices are used, else solver.
 */
static
void relman_cookup_indices(struct RXNameData *rd
		,struct rel_relation *rel,int mORs
){
  int nvars,n;
  const struct var_variable **vlist;

  nvars = rel_n_incidences(rel);
  rd->indices = rel_tmpalloc_array(1+nvars,int);
  if (rd->indices == NULL) {
    return;
  }
  vlist = rel_incidence_list(rel);
  if (mORs) {
    for (n = 0; n < nvars; n++) {
      rd->indices[n+1] = var_mindex(vlist[n]);
    }
  }else{
    for (n = 0; n < nvars; n++) {
      rd->indices[n+1] = var_sindex(vlist[n]);
    }
  }
}


char *relman_make_vstring_infix(slv_system_t sys
		,struct rel_relation *rel, int style
){
  char *sbeg;
  struct RXNameData rd = {"x",NULL,""};

  if (style) {
    sbeg = WriteRelationString(rel_instance(rel),slv_instance(sys),
                               NULL,NULL,relio_ascend,NULL);
  }else{
    /* need to cook up output indices */
    relman_cookup_indices(&rd,rel,1);
    sbeg = WriteRelationString(rel_instance(rel),slv_instance(sys),
                               (WRSNameFunc)RelationVarXName,
                               &rd,relio_ascend,NULL);
  }
  return(sbeg);
}


char *relman_make_vstring_postfix(slv_system_t sys,
		struct rel_relation *rel, int style
){
   char  *sbeg;

   if (style) {
     sbeg = WriteRelationPostfixString(rel_instance(rel),slv_instance(sys));
   }else{
#if REIMPLEMENT
     left = exprman_make_xstring_postfix(rel,sys,rel_lhs(rel));
     right = exprman_make_xstring_postfix(rel,sys,rel_rhs(rel));
#else
     sbeg = ASC_NEW_ARRAY(char,60);
     if (sbeg==NULL) return sbeg;
     sprintf(sbeg,"relman_make_xstring_postfix not reimplemented.");
#endif
   }
   return(sbeg);
}
