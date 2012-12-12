/*	ASCEND modelling environment
	Copyright (C) 1998, 2006, 2007 Carnegie Mellon University
	Copyright (C) 1996 Benjamin Andrew Allan

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//*
	by Benjamin Andrew Allan
	5/19/96
	Last in CVS: $Revision: 1.28 $ $Date: 1998/06/16 16:53:04 $ $Author: mthomas $
*/

#include "slv_stdcalls.h"

#include <ascend/general/ascMalloc.h>
#include <ascend/general/mathmacros.h>

#include "relman.h"
#include "logrelman.h"

#define MIMDEBUG 0 /* slv_make_incidence_mtx debugging */

/*
	Here we play some hefty Jacobian reordering games.

	What we want to happen eventually is as follows:

	  - Get the free & incident pattern for include relations.
	  - Output-assign the Jacobian.
	  - BLT permute the Jacobian. If underspecified, fake rows
	    to make things appear square.
	  - For all leading square blocks apply reordering (kirk, etc),
	  - If trailing rectangular block, "apply clever kirkbased scheme not known."???

	At present, we aren't quite so clever. We:

	  - Get the free & incident pattern for include relations.
	  - Output-assign the Jacobian.
	  - BLT permute the square output assigned region of the Jacobian.
	  - For all leading square blocks apply reordering (kirk, etc)
	  - Collect the block list as part of the master data structure.
      - Set sindices for rels, vars as current rows/cols in matrix so
	    that the jacobian is 'naturally' preordered for all solvers.

	Solvers are still free to reorder their own matrices any way they like.
	It's probably a dumb idea, though.
*/

/*------------------------------------------------------------------------------
  MATRIX CREATION
*/

/* see slv_stdcalls.h */
int slv_make_incidence_mtx(slv_system_t sys, mtx_matrix_t mtx,
                               var_filter_t *vf,rel_filter_t *rf)
{
#if MIMDEBUG
  FILE *fp;
#endif
  int32 r, rlen,ord;
  struct rel_relation **rp;

  if (sys==NULL || mtx == NULL || vf == NULL || rf == NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called with null");
    return 1;
  }
  rp = slv_get_solvers_rel_list(sys);
  assert(rp!=NULL);
  rlen = slv_get_num_solvers_rels(sys);
  ord = MAX(rlen,slv_get_num_solvers_vars(sys));
  if (ord > mtx_order(mtx)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"undersized matrix");
    return 2;
  }
  for (r=0; r < rlen; r++) {
    if (rel_apply_filter(rp[r],rf)) {
      relman_get_incidence(rp[r],vf,mtx);
    }
  }
#if MIMDEBUG
  fp = fopen("/tmp/mim.plot","w+");
  if (fp !=NULL) {
    mtx_write_region_plot(fp,mtx,mtx_ENTIRE_MATRIX);
    fclose(fp);
  }
#endif
  return 0;
}

void slv_sort_rels_and_vars(slv_system_t sys
		,int32 *rel_count, int32 *var_count
){
  struct rel_relation **rp, **rtmp, *rel;
  struct var_variable **vp, **vtmp, *var;

  int32 nrow,ncol,rlen,vlen,rel_tmp_end;
  int32 r,c,var_tmp_end,len;
  var_filter_t vf;
  rel_filter_t rf;

  /* get rel and var info */
  rp = slv_get_solvers_rel_list(sys);
  vp = slv_get_solvers_var_list(sys);
  rlen = slv_get_num_solvers_rels(sys);
  vlen = slv_get_num_solvers_vars(sys);
  if (rlen ==0 || vlen == 0) return;

  assert(var_count != NULL && rel_count != NULL);
  *var_count = *rel_count = -1;

  /* allocate temp arrays */
  vtmp = ASC_NEW_ARRAY(struct var_variable *,vlen);
  if (vtmp == NULL) {
    return;
  }
  rtmp = ASC_NEW_ARRAY(struct rel_relation *,rlen);
  if (rtmp == NULL) {
    ascfree(vtmp);
    return;
  }

  /* set up filters */
  rf.matchbits = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  rf.matchvalue = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  vf.matchbits = (VAR_INCIDENT | VAR_SVAR | VAR_FIXED | VAR_ACTIVE);
  vf.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);

  /* count rows and cols */
  nrow = slv_count_solvers_rels(sys,&rf);
  ncol = slv_count_solvers_vars(sys,&vf);


  /* Sort out unincluded and inactive rels to
   * the end of the temporary relation list.
   */
  *rel_count = 0;
  rel_tmp_end = rlen -1;
  for (r=0; r < rlen; r++) {
    rel = rp[r];
    if (rel_apply_filter(rel,&rf)) {
      rtmp[*rel_count] = rel;
      (*rel_count)++;
    } else {
      rtmp[rel_tmp_end] = rel;
      rel_tmp_end--;
    }
  }

  /* sort the inactive out of the unincluded and move to end */
  len = rlen -1;
  for(c = rel_tmp_end + 1; len > c; ) {
    rel = rtmp[c];
    if (rel_active(rel)) {
      c++;
    } else {
      rtmp[c] = rtmp[len];
      rtmp[len] = rel;
      len--;
    }
  }

  /* Sort variable list */
  *var_count = 0;
  var_tmp_end = vlen - 1;
  for (c = 0; c < vlen; c++) {
    var = vp[c];
    if (var_apply_filter(var,&vf)) {
      vtmp[*var_count] = var;
      (*var_count)++;
    } else {
      vtmp[var_tmp_end] = var;
      var_tmp_end--;
    }
  }
  /* sort the inactive out of the unincluded and move to end */
  len = vlen -1;
  for(c = var_tmp_end + 1; len > c; ) {
    var = vtmp[c];
    if (var_active(var) && var_active(var)) {
      c++;
    } else {
      vtmp[c] = vtmp[len];
      vtmp[len] = var;
      len--;
    }
  }

  /* reset solver indicies */
  for (c = 0; c < vlen; c++) {
    vp[c] = vtmp[c];
    var_set_sindex(vp[c],c);
  }

  for (c = 0; c < rlen; c++) {
    rp[c] = rtmp[c];
    rel_set_sindex(rp[c],c);
  }
  ascfree(vtmp);
  ascfree(rtmp);
  return;
}

/*------------------------------------------------------------------------------
  QUERYING AND ENFORCING BOUNDS
*/

int slv_ensure_bounds(slv_system_t sys,int32 lo,int32 hi, FILE *mif)
{
  real64 val,low,high;
  int32 c,nchange=0;
  struct var_variable *var, **vp;

  vp = slv_get_solvers_var_list(sys);
  if (vp==NULL) return -1;
  for (c= lo; c <= hi; c++) {
    var = vp[c];
    low = var_lower_bound(var);
    high = var_upper_bound(var);
    val = var_value(var);
    if( low > high ) {
      if (mif!=NULL) {
		ERROR_REPORTER_START_NOLINE(ASC_PROG_ERR);
        FPRINTF(ASCERR,"Bounds for variable '");
        var_write_name(sys,var,ASCERR);
        FPRINTF(ASCERR,"' are inconsistent [%g,%g].\n",low,high);
        FPRINTF(ASCERR,"Bounds will be swapped.\n");
		error_reporter_end_flush();
      }
      var_set_upper_bound(var, low);
      var_set_lower_bound(var, high);
      low = var_lower_bound(var);
      high = var_upper_bound(var);
      nchange++;
    }

    if( low > val ) {
      if (mif!=NULL) {
		ERROR_REPORTER_START_NOLINE(ASC_PROG_ERR);
        FPRINTF(ASCERR,"Variable '");
        var_write_name(sys,var,ASCERR);
        FPRINTF(ASCERR,"' was set below its lower bound.\n");
        FPRINTF(ASCERR,"It will be moved to its lower bound.");
		error_reporter_end_flush();
      }
      var_set_value(var, low);
    } else {
      if( val > high ) {
        if (mif!=NULL) {
          ERROR_REPORTER_START_NOLINE(ASC_PROG_ERR);
          FPRINTF(ASCERR,"Variable '");
          var_write_name(sys,var,ASCERR);
          FPRINTF(ASCERR,"' was set above its upper bound.\n");
          FPRINTF(ASCERR,"It will be moved to its upper bound.");
		  error_reporter_end_flush();
        }
        var_set_value(var, high);
        nchange++;
      }
    }
  }
  return nchange;
}

/* return 0 on success (ie bounds are met) */
int slv_check_bounds(const slv_system_t sys
	, int32 lo,int32 hi
	, const char *label
){

  real64 val,low,high;
  int32 c,len;
  struct var_variable *var, **vp;
  int err = 0;

  //CONSOLE_DEBUG("Got lo = %d, hi =%d",lo,hi);

  if(label==NULL) label = "";
  if(sys==NULL) return -1;
  vp = slv_get_solvers_var_list(sys);
  if(vp==NULL) return -2;
  len = slv_get_num_solvers_vars(sys);
  if(hi < 0)hi+= len; /* so you can use -1 to mean 'the last' */
  if(lo < 0)lo+= len;
  if(lo > len || hi > len || lo < 0 || hi < 0 || lo > hi){
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid arguments (lo = %d, hi = %d, len = %d)",lo, hi, len);
    return -1;
  }

  for (c= lo; c <= hi; c++) {
    var = vp[c];
    low = var_lower_bound(var);
    high = var_upper_bound(var);
    val = var_value(var);
    if( low > high ) {
      ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
      FPRINTF(ASCERR,"Bounds for %s variable '",label);
      var_write_name(sys,var,ASCERR);
      FPRINTF(ASCERR,"' are inconsistent [%g,%g]",low,high);
      error_reporter_end_flush();
	  err = err | 0x1;
    }

    if(low > val){
      ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
      FPRINTF(ASCERR,"The %s variable '",label);
      var_write_name(sys,var,ASCERR);
      FPRINTF(ASCERR,"' was set below its lower bound.");
      error_reporter_end_flush();
      err = err | 0x2;
    }else if( val > high ){
      ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
        FPRINTF(ASCERR,"The %s variable '",label);
        var_write_name(sys,var,ASCERR);
        FPRINTF(ASCERR,"' was set above its upper bound.");
      error_reporter_end_flush();
      err = err | 0x4;
    }
  }
  return err;
}

/*------------------------------------------------------------------------------
  OUTPUT ASSIGNMENT AND PARTITIONG IN LOGICAL RELATIONS
*/

#define MLIMDEBUG 0
/*
 * Populates a matrix according to the sys solvers_dvars, solvers_logrels
 * lists and the filters given. mtx given must be created (not null)
 * and with order >= MAX( slv_get_num_solvers_logrels(sys),
 *                        slv_get_num_solvers_dvars(sys));
 * returns 0 if went ok.
 */
int slv_make_log_incidence_mtx(slv_system_t sys, mtx_matrix_t mtx,
                                   dis_filter_t *dvf,logrel_filter_t *lrf)
{
#if MLIMDEBUG
  FILE *fp;
#endif
  int32 lr, lrlen,ord;
  struct logrel_relation **lrp;

  if (sys==NULL || mtx == NULL || dvf == NULL || lrf == NULL) {
    FPRINTF(stderr,"make_log_incidence called with null\n");
    return 1;
  }
  lrp = slv_get_solvers_logrel_list(sys);
  assert(lrp!=NULL);
  lrlen = slv_get_num_solvers_logrels(sys);
  ord = MAX(lrlen,slv_get_num_solvers_dvars(sys));
  if (ord > mtx_order(mtx)) {
    FPRINTF(stderr,"make_incidence called with undersized matrix\n");
    return 2;
  }
  for (lr=0; lr < lrlen; lr++) {
    if (logrel_apply_filter(lrp[lr],lrf)) {
      logrelman_get_incidence(lrp[lr],dvf,mtx);
    }
  }
#if MLIMDEBUG
  fp = fopen("/tmp/mim.plot","w+");
  if (fp !=NULL) {
    mtx_write_region_plot(fp,mtx,mtx_ENTIRE_MATRIX);
    fclose(fp);
  }
#endif
  return 0;
}

#if USEDCODE

/* returns 0 if successful, 1 if insufficient memory. Does not change
 * the data in mtx. Orders the solvers_dvar list of the system to
 * match the permutation on the given mtx. It is assumed that
 * the org cols of mtx == dvar list position. Only the
 * dvars in range lo to hi of the dvar list are affected and
 * only these dvars should appear in the same org column range of
 * the mtx. This should not be called on blocks less than 3x3.
 */
static int reindex_dvars_from_mtx(slv_system_t sys, int32 lo, int32 hi,
                                  const mtx_matrix_t mtx)
{
  struct dis_discrete **dvtmp, **dvp;
  int32 c,v,vlen;

  if (lo >= hi +1) return 0; /* job too small */
  dvp = slv_get_solvers_dvar_list(sys);
  vlen = slv_get_num_solvers_dvars(sys);
  /* on dvtmp we DONT have the terminating null */
  dvtmp = ASC_NEW_ARRAY(struct dis_discrete *,vlen);
  if (dvtmp == NULL) {
    return 1;
  }
  /* copy pointers to dvtmp in order desired and copy back rather than sort */
  for (c=lo;c<=hi;c++) {
    v = mtx_col_to_org(mtx,c);
    dvtmp[c] = dvp[v];
  }
  /* copying back and re-sindexing */
  for (c=lo;c<=hi;c++) {
    dvp[c] = dvtmp[c];
    dis_set_sindex(dvp[c],c);
  }
  ascfree(dvtmp);
  return 0;
}

/* returns 0 if successful, 1 if insufficient memory. Does not change
 * the data in mtx. Orders the solvers_logrel list of the system to
 * match the permutation on the given mtx. It is assumed that
 * the org rows of mtx == logrel list position. Only the
 *logrels in range lo to hi of the logrel list are affected and
 * only these logrels should appear in the same org row range of
 * the input mtx. This should not be called on blocks less than 3x3.
 */
static int reindex_logrels_from_mtx(slv_system_t sys, int32 lo, int32 hi,
                                    const mtx_matrix_t mtx)
{
  struct logrel_relation **lrtmp, **lrp;
  int32 c,v,rlen;

  lrp = slv_get_solvers_logrel_list(sys);
  rlen = slv_get_num_solvers_logrels(sys);
  /* on lrtmp we DONT have the terminating null */
  lrtmp = ASC_NEW_ARRAY(struct logrel_relation *,rlen);
  if (lrtmp == NULL) {
    return 1;
  }
  /* copy pointers to lrtmp in order desired and copy back rather than sort.
   * do this only in the row range of interest. */
  for (c=lo;c<=hi;c++) {
    v = mtx_row_to_org(mtx,c);
#if RIDEBUG
    FPRINTF(stderr,"Old logrel sindex(org) %d becoming sindex(cur) %d\n",v,c);
#endif
    lrtmp[c] = lrp[v];
  }
  /* copying back and re-sindexing */
  for (c=lo;c<=hi;c++) {
    lrp[c] = lrtmp[c];
    logrel_set_sindex(lrp[c],c);
  }
  ascfree(lrtmp);
  return 0;
}

#endif /* USEDCODE */

