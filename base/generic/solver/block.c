/*	ASCEND modelling environment
	Copyright (C) 1998, 2006-2007 Carnegie Mellon University
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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//* @file
	Block partitioning implementation (for real-valued variables)
*//*
	slv_stdcalls.c by Benjamin Andrew Allan, May 1996.
	chopped out block-paritioning into separate file, John Pye, Jan 2007.
	Last in CVS: $Revision: 1.28 $ $Date: 1998/06/16 16:53:04 $ $Author: mthomas $
*/

#include "block.h"
#include <utilities/ascMalloc.h>
#include <utilities/ascPrint.h>
#include <utilities/ascPanic.h>
#include <general/mathmacros.h>
#include "slv_client.h"
#include "slv_stdcalls.h"
#include "model_reorder.h"

/* #define REINDEX_DEBUG */
/* #define BLOCKPARTITION_DEBUG */
/* #define CUT_DEBUG */

/* global to get around the mr header (for tear_subreorder) */
static
enum mtx_reorder_method g_blockmethod = mtx_UNKNOWN;

/*-----------------------------------------------------------------------------
  VAR/REL ORDERING (TO MATCH MATRIX PERMUTATIONS)
*/

/**
	Orders the solvers_var list of the system to match the permutation
	on the given mtx. Does not change the data in mtx.

	@return 0 on success, 1 on out-of-memory

	It is assumed that the org cols of mtx == var list position. Only the
	vars in range lo to hi of the var list are affected and
	only these vars should appear in the same org column range of
	the mtx. This should not be called on blocks less than 3x3.
*/
static int reindex_vars_from_mtx(slv_system_t sys, int32 lo, int32 hi,
                                 const mtx_matrix_t mtx)
{
  struct var_variable **vtmp, **vp;
  int32 c,v,vlen;

  if (lo >= hi +1) return 0; /* job too small */
  vp = slv_get_solvers_var_list(sys);
  vlen = slv_get_num_solvers_vars(sys);
  /* on vtmp we DONT have the terminating null */
  vtmp = ASC_NEW_ARRAY(struct var_variable *,vlen);
  if (vtmp == NULL) {
    return 1;
  }
  /* copy pointers to vtmp in order desired and copy back rather than sort */
  for (c=lo;c<=hi;c++) {
    v = mtx_col_to_org(mtx,c);
    vtmp[c] = vp[v];
  }
  /* copying back and re-sindexing */
  for (c=lo;c<=hi;c++) {
    vp[c] = vtmp[c];
    var_set_sindex(vp[c],c);
  }
  ascfree(vtmp);
  return 0;
}
/**
	Orders the solvers_rel list of the system to match the permutation
	on the given mtx. Does not change the data in mtx.

	@return 0 on success, 1 on out-of-memory

	It is assumed that the org rows of mtx == rel list position. Only the
	rels in range lo to hi of the rel list are affected and
	only these rels should appear in the same org row range of
	the input mtx. This should not be called on blocks less than 3x3.
*/
static int reindex_rels_from_mtx(slv_system_t sys, int32 lo, int32 hi,
                                 const mtx_matrix_t mtx)
{
  struct rel_relation **rtmp, **rp;
  int32 c,v,rlen;

  rp = slv_get_solvers_rel_list(sys);
  rlen = slv_get_num_solvers_rels(sys);
  /* on rtmp we DONT have the terminating null */
  rtmp = ASC_NEW_ARRAY(struct rel_relation*,rlen);
  if (rtmp == NULL) {
    return 1;
  }
  /* copy pointers to rtmp in order desired and copy back rather than sort.
   * do this only in the row range of interest. */
  for (c=lo;c<=hi;c++) {
    v = mtx_row_to_org(mtx,c);
#ifdef REINDEX_DEBUG
	if(c!=v){
	  CONSOLE_DEBUG("Old rel sindex (org) %d becoming sindex (cur) %d\n",v,c);
    }
#endif
    rtmp[c] = rp[v];
  }
  /* copying back and re-sindexing */
  for (c=lo;c<=hi;c++) {
    rp[c] = rtmp[c];
    rel_set_sindex(rp[c],c);
  }
  ascfree(rtmp);
  return 0;
}

/*------------------------------------------------------------------------------
  PARTITIONING INTO BLOCK LOWER/UPPER TRIANGULAR FORM
*/

/**
	Perform var and rel reordering to achieve block form.

	@param uppertrianguler if non-zero, BUT form. if 0, make BLT form.

	@callergraph
*/
int slv_block_partition_real(slv_system_t sys,int uppertriangular){
#ifdef BLOCKPARTITION_DEBUG
  FILE *fp;
#endif
  struct rel_relation **rp;
  struct var_variable **vp;
  mtx_region_t  *newblocks;
  dof_t *d;
  int32 nrow,ncol;
  int32 ncolpfix, nrowpun, vlenmnv;
  mtx_matrix_t mtx;
  int32 order, rank;
  int32 c,len,vlen,r,rlen;
  var_filter_t vf;
  rel_filter_t rf;

  /* CONSOLE_DEBUG("..."); */

  rp = slv_get_solvers_rel_list(sys);
  vp = slv_get_solvers_var_list(sys);
  rlen = slv_get_num_solvers_rels(sys);
  vlen = slv_get_num_solvers_vars(sys);
  if (rlen ==0 || vlen == 0) return 1;
  order = MAX(rlen,vlen);

  rf.matchbits = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  rf.matchvalue = (REL_ACTIVE);
  vf.matchbits = (VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
  vf.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);

  /* nrow plus unincluded rels and ineqs */
  nrowpun = slv_count_solvers_rels(sys,&rf);
  ncolpfix = slv_count_solvers_vars(sys,&vf); /* ncol plus fixed vars */

  vf.matchbits = (VAR_SVAR);
  vf.matchvalue = 0;
  vlenmnv = vlen - slv_count_solvers_vars(sys,&vf);  /* vlen minus non vars */

  rf.matchbits = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  rf.matchvalue = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  vf.matchbits = (VAR_INCIDENT | VAR_SVAR | VAR_FIXED | VAR_ACTIVE);
  vf.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);

  nrow = slv_count_solvers_rels(sys,&rf);
  ncol = slv_count_solvers_vars(sys,&vf);

  mtx = mtx_create();
  mtx_set_order(mtx,order);

  if (slv_make_incidence_mtx(sys,mtx,&vf,&rf)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"failure in creating incidence matrix.");
    mtx_destroy(mtx);
    return 1;
  }

  /* CONSOLE_DEBUG("FIRST REL = %p",rp[0]); */

  mtx_output_assign(mtx,rlen,vlen);
  rank = mtx_symbolic_rank(mtx);
  if (rank == 0 ) return 1; 	/* nothing to do, eh? */

  /* CONSOLE_DEBUG("FIRST REL = %p",rp[0]); */

  /* lot of whining about dof */
  if (rank < nrow) {
    ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"System is row rank deficient (%d dependent equations)",
            nrow - rank);
  }
  if (rank < ncol) {
    if ( nrow != rank) {
      ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"System is row rank deficient with %d excess columns.",
              ncol - rank);
    } else {
      ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"System has %d degrees of freedom.", ncol - rank);
    }
  }
  if (ncol == nrow) {
    if (ncol != rank) {
      ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"System is (%d) square but rank deficient.",ncol);
    } else {
      ERROR_REPORTER_NOLINE(ASC_USER_NOTE,"System is (%d) square.",ncol);
    }
  }
  if (uppertriangular) {
    mtx_ut_partition(mtx);
  } else {
    mtx_partition(mtx);
  }
  /* copy the block list. there is at least one if rank >=1 as above */
  len = mtx_number_of_blocks(mtx);
  newblocks = ASC_NEW_ARRAY(mtx_region_t,len);
  if (newblocks == NULL) {
    mtx_destroy(mtx);
    return 2;
  }
  for (c = 0 ; c < len; c++) {
    mtx_block(mtx,c,&(newblocks[c]));
  }
  slv_set_solvers_blocks(sys,len,newblocks); /* give it to the system */
  d = slv_get_dofdata(sys);
  d->structural_rank = rank;
  d->n_rows = nrow;
  d->n_cols = ncol;

  /* CONSOLE_DEBUG("FIRST REL = %p",rp[0]); */

  /* the next two lines assume inequalities are un-included. */
#define MINE 1
#if MINE
  d->n_fixed = ncolpfix - ncol;
  d->n_unincluded = nrowpun - nrow;
#else
  d->n_fixed = vlen - ncol;
  d->n_unincluded = rlen - nrow;
#endif
  d->reorder.partition = 1;		/* yes */
  d->reorder.basis_selection = 0;	/* none yet */
  d->reorder.block_reordering = 0;	/* none */

#ifdef BLOCKPARTITION_DEBUG
  fp = fopen("/tmp/sbp1.plot","w+");
  if (fp !=NULL) {
    mtx_write_region_plot(fp,mtx,mtx_ENTIRE_MATRIX);
    fclose(fp);
  }
#endif

#if MINE
  len = vlenmnv - 1;  /* row of last possible solver variable */
  for (c=ncol; len > c ; ) { /* sort the fixed out of inactive */
    r = mtx_col_to_org(mtx,c);
    if ( var_fixed(vp[r]) && var_active(vp[r])) {
      c++;
    } else {
      mtx_swap_cols(mtx,c,len);
      len--;
    }
  }
#endif

  /* Now, leftover columns are where we want them.
   * That is, unassigned incidence is next to assigned and fixed on right with
   * no columns which do not correspond to variables in the range 0..vlen-1.
   */
  /* there aren't any nonincident vars in the solvers varlist, so
   * we now have the columns in |assigned|dof|fixed|inactive|nonvars| order */

  /* rows 0->rlen-1 should be actual rels, though, and not phantoms.
   * At this point we should have
   * rows - assigned
   *      - unassigned w/incidence
   *      - unincluded and included w/out free incidence and inactive mixed.
   */
  /* sort unincluded relations to bottom of good region */
  /* this is needed because the idiot user may have fixed everything */
  /* in a relation, but we want those rows not to be confused with */
  /* unincluded rows.  we're sort of dropping inequalities on the floor.*/
  len = rlen - 1;  /* row of last possible relation */
  /* sort the inactive out of the unassigned/unincluded */
  for (c=rank; len > c ; ) {
    r = mtx_row_to_org(mtx,c);
    if ( rel_active(rp[r])) {
      c++;
    } else {
      mtx_swap_rows(mtx,c,len);
      len--;
    }
  }

#if MINE
  /* sort the unincluded out of the unassigned */
  len = nrowpun - 1;  /* row of last possible unassigned/unincluded relation */
  for (c=rank; len > c ; ) {
    r = mtx_row_to_org(mtx,c);
    if ( rel_included(rp[r]) ) {
      c++;
    } else {
      mtx_swap_rows(mtx,c,len);
      len--;
    }
  }
#endif


#ifdef BLOCKPARTITION_DEBUG
  fp = fopen("/tmp/sbp2.plot","w+");
  if (fp !=NULL) {
    mtx_write_region_plot(fp,mtx,mtx_ENTIRE_MATRIX);
    fclose(fp);
  }
#endif
  /* now, at last we have cols jacobian in the order we want the lists to
   * be handed to the solvers. So, we need to reset the var sindex values
   * and reorder the lists.
   */
  if (reindex_vars_from_mtx(sys,0,vlen-1,mtx)) {
    mtx_destroy(mtx);
    return 2;
  }
  /* now, at last we have rows jacobian in the order we want the lists to
   * be handed to the solvers. So, we need to reset the rel sindex values.
   */
  if (reindex_rels_from_mtx(sys,0,rlen-1,mtx)) {
    mtx_destroy(mtx);
    return 2;
  }

  /* CONSOLE_DEBUG("FIRST REL = %p",rp[0]); */

  mtx_destroy(mtx);
  return 0;
}


#if 0 /* code not currently used */
#\ifdef STATIC_HARWELL /* added the backslash so that syntax highlighting behaves */
/* note that you can't statically link to Harwell routines under the terms of the GPL */
extern void mc21b();
extern void mc13emod();
/* returns 0 if ok, OTHERWISE if madness detected.
 * doesn't grok inequalities.
 * CURRENTLY DOESN'T DO WELL WHEN NCOL<NROW
 */
#define BLOCKPARTITION_DEBUG 0
int slv_block_partition_harwell(slv_system_t sys)
{
#\if BLOCKPARTITION_DEBUG
  FILE *fp;
#\endif
  struct rel_relation **rp;
  struct rel_relation **rtmp;
  struct rel_relation *rel;
  struct var_variable **vp;
  struct var_variable **vtmp;
  struct var_variable *var;
  const struct var_variable **list;
  mtx_region_t  *newblocks;
  dof_t *d;
  int32 nrow,ncol,size;
  int32 order, rank;
  int32 c,len,vlen,r,v,rlen,col,row;
  int32 licn,rel_tmp_end,rel_count,var_perm_count,var_tmp_end,row_perm_count;
  int32 col_incidence,numnz,row_high,col_high,dummy_ptr,row_ptr,num;
  int32 *dummy_array_ptr;
  var_filter_t vf;
  rel_filter_t rf;
  int32 *icn, *ip, *lenr, *iperm, *iw1, *iw2, *iw3, *arp, *ipnew, *ib;
  int32 *row_perm;

  /* get rel and var info */
  rp = slv_get_solvers_rel_list(sys);
  vp = slv_get_solvers_var_list(sys);
  rlen = slv_get_num_solvers_rels(sys);
  vlen = slv_get_num_solvers_vars(sys);
  if (rlen ==0 || vlen == 0) return 1;
  order = MAX(rlen,vlen);

  /* allocate temp arrays */
  vtmp = (struct var_variable **)ascmalloc(vlen*sizeof(struct var_variable *));
  var = (struct var_variable *)ascmalloc(sizeof(struct var_variable *));
  if (vtmp == NULL || var ==NULL) {
    return 1;
  }
  rtmp = ASC_NEW_ARRAY(struct rel_relation *,rlen);
  rel = ASC_NEW(struct rel_relation);
  if (rtmp == NULL || rel ==NULL) {
    return 1;
  }

  /* set up filters */
  rf.matchbits = (REL_INCLUDED | REL_EQUALITY);
  rf.matchvalue = (REL_INCLUDED | REL_EQUALITY);
  vf.matchbits = (VAR_INCIDENT | VAR_SVAR | VAR_FIXED);
  vf.matchvalue = (VAR_INCIDENT | VAR_SVAR);

  /* count rows and cols */
  nrow = slv_count_solvers_rels(sys,&rf);
  ncol = slv_count_solvers_vars(sys,&vf);


  /* count incidence for equality relations that are included
     and active.  Sorts out unincluded and inactive rels to
     the end of the temporary relation list */
  licn = 0;
  rel_tmp_end = rlen -1;
  for (r=0; r < rlen; r++) {
    rel = rp[r];
    if (rel_apply_filter(rel,&rf) && rel_active(rel)) {
      len = rel_n_incidences(rel);
      list = rel_incidence_list(rel);
      for (c=0; c < len; c++) {
	if( var_apply_filter(list[c],&vf) ) {
	  licn++;
	}
      }
    } else {
      rtmp[rel_tmp_end] = rp[r];
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
  var_perm_count = 0;
  var_tmp_end = vlen - 1;
  for (c = 0; c < vlen; c++) {
    var = vp[c];
    if (var_apply_filter(var,&vf)) {
      vtmp[var_perm_count] = var;
      var_perm_count++;
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

  /* reset solver indicies to make life easier */
  for (c = 0; c < vlen; c++) {
    vp[c] = vtmp[c];
    var_set_sindex(vp[c],c);
  }

  size = MAX(nrow,ncol);
  /* Create vectors for fortran calls */
  icn = ASC_NEW_ARRAY(int32,licn);
  ip = ASC_NEW_ARRAY(int32,size);
  lenr = ASC_NEW_ARRAY(int32,size);
  iperm = ASC_NEW_ARRAY(int32,size);
  iw1 = ASC_NEW_ARRAY(int32,size);
  iw2 = ASC_NEW_ARRAY(int32,size);
  iw3 = ASC_NEW_ARRAY(int32,size);
  arp = ASC_NEW_ARRAY(int32,size);
  ipnew = ASC_NEW_ARRAY(int32,size);
  ib = ASC_NEW_ARRAY(int32,size);
  row_perm = ASC_NEW_ARRAY(int32,size);


/* Fill incidence vectors and place included relations w/o
 * incidence in temporary relation list before the unincluded
 * and inactive relations
 */
  col_incidence = 0;
  row_perm_count = 0;
  for (r=0; r < rlen; r++) {
    rel = rp[r];
    if (rel_apply_filter(rel,&rf) && rel_active(rel)) {
      len = rel_n_incidences(rel);
      if (len > 0) {
	ip[row_perm_count] = col_incidence + 1; /*FORTRAN*/
	lenr[row_perm_count] = len;
	row_perm[row_perm_count] = r;
	row_perm_count++;
	list = rel_incidence_list(rel);
	for (c=0; c < len; c++) {
	  if( var_apply_filter(list[c],&vf) ) {
	    col = var_sindex(list[c]);
	    icn[col_incidence] = col + 1; /*FORTRAN*/
	    col_incidence++;
	  }
	}
      } else {
	rtmp[rel_tmp_end] = rel;
	rel_tmp_end--;
      }
    }
  }

  licn = col_incidence;
  order = row_perm_count;
  mc21b(&order,icn,&licn,ip,lenr,iperm,&numnz,iw1,arp,iw2,iw3);
  if (order == numnz){
    FPRINTF(stderr,"they are equal\n");
  } else if(order == numnz + 1){
    FPRINTF(stderr,"ADD ONE\n");
  } else {
    FPRINTF(stderr,"no relationship\n");
  }

  if (rank == 0 ) {
    return 1; 	/* nothing to do, eh? */
  }
  row_high = nrow - 1;
  col_high = ncol -1;

  /* lot of whining about dof */
  if (rank < nrow) {
    FPRINTF(stderr,"System is row rank deficient (dependent equations)\n");
    row_high = rank - 1;
    /* KHACK: have found that this is executed erroneously */
  }
  if (rank < ncol) {
    if ( nrow != rank) {
      FPRINTF(stderr,"System is row rank deficient with excess columns.\n");
    } else {
      FPRINTF(stderr,"System has degrees of freedom.\n");
    }
  }
  if (ncol == nrow) {
    FPRINTF(stderr,"System is (%d) square",ncol);
    if (ncol != rank) {
      FPRINTF(stderr,"but rank deficient.\n");
    } else {
      FPRINTF(stderr,".\n");
    }
  }

  dummy_array_ptr = arp;
  arp = lenr;
  lenr = dummy_array_ptr;

  /* Fix row pointers for assigned relations. Put unassigned
     onto temporary relation list (before included w/o incidence)
     and put dummy dense rows into ipnew */
  dummy_ptr = licn + 1; /* FORTRAN */
  for (row = 0; row < order; row++) {
    row_ptr = iperm[row] - 1; /* FORTRAN */
    if (row_ptr != -1) { /* indicates unassigned row */
      ipnew[row_ptr] = ip[row];
      lenr[row_ptr] = arp[row];
    } else {
      ipnew[row_ptr] = dummy_ptr;
      dummy_ptr += ncol;
      lenr[row_ptr] = ncol;
      /* unassigned before fixed*/
      rtmp[rel_tmp_end] = rp[row_perm[row_ptr]];
      rel_tmp_end--;
    }
  }
  for (row = order; row < ncol; row++) {
    ipnew[row] = dummy_ptr;
    dummy_ptr += ncol;
    lenr[row] = ncol;
  }

  mc13emod(&size,icn,&licn,ipnew,lenr,arp,ib,&num,iw1,iw2,iw3);


  /* copy the block list. there is at least one if rank >=1 as above */
  len = num;
  newblocks = ASC_NEW_ARRAY(mtx_region_t,len);
  if (newblocks == NULL) {
    return 2;
  }
  /* set up locations of block corners with the row and column
     orderings which will be set soon */
  for (c = 0 ; c < len; c++) {
    newblocks[c].row.low = ib[c]-1;
    newblocks[c].col.low = newblocks[c].row.low;
  }
  for (c = 0 ; c < len -1; c++) {
    newblocks[c].row.high = newblocks[c+1].row.low -1;
    newblocks[c].col.high = newblocks[c].row.high;
  }
  newblocks[len - 1].row.high = row_high;
  newblocks[len - 1].col.high = col_high;

  slv_set_solvers_blocks(sys,len,newblocks); /* give it to the system */
  d = slv_get_dofdata(sys);
  d->structural_rank = rank;
  d->n_rows = nrow;
  d->n_cols = ncol;
  /* the next two lines assume only free and fixed incident solvervar
   * appear in the solvers var list and inequalities are unincluded.
   */
  d->n_fixed = vlen - ncol;
  d->n_unincluded = rlen - nrow;
  d->reorder.partition = 1;		/* yes */
  d->reorder.basis_selection = 0;	/* none yet */
  d->reorder.block_reordering = 0;	/* none */

  for (c = 0; c < ncol; c++) {
    v = arp[c] -1; /* FORTRAN */
    vtmp[c] = vp[v];
  }
  for (c = 0; c < vlen; c++) {
    vp[c] = vtmp[c];
    var_set_sindex(vp[c],c);
  }

  rel_count = 0;
  for (c = 0; c < size; c++) {
    r = arp[c] - 1;
    if (r < order){ /* weed out fake rows */
      r = iperm[r] - 1;
      if (r != -1) { /* unassigned already in rtmp */
	rtmp[rel_count] = rp[row_perm[r]];
	rel_count++;
      }
    }
  }
  for (c = 0; c < rlen; c++) {
    rp[c] = rtmp[c];
    rel_set_sindex(rp[c],c);
  }
  ascfree(vtmp);
  ascfree(rtmp);
  ascfree(icn);
  ascfree(ip);
  ascfree(lenr);
  ascfree(iperm);
  ascfree(iw1);
  ascfree(iw2);
  ascfree(iw3);
  ascfree(arp);
  ascfree(ipnew);
  ascfree(ib);
  return 0;
}
#\endif /*STATIC_HARWELL*/
#endif /* 0 */


int slv_block_unify(slv_system_t sys)
{
  dof_t *d;
  const mtx_block_t *mbt;
  mtx_region_t *newblocks;

  if (sys==NULL) return 1;

  d = slv_get_dofdata(sys);
  mbt = slv_get_solvers_blocks(sys);
  assert(d!=NULL && mbt!=NULL);
  if (d->structural_rank && mbt->nblocks >1) {
    newblocks = ASC_NEW(mtx_region_t);
    if (newblocks == NULL) return 2;
    newblocks->row.low = newblocks->col.low = 0;
    newblocks->row.high = d->structural_rank - 1;
    newblocks->col.high = d->n_cols - 1;
    if ( newblocks->col.high < 0 || newblocks->row.high < 0) {
      ascfree(newblocks);
      return 1;
    }
    slv_set_solvers_blocks(sys,1,newblocks);
  }
  return 0;
}

int slv_set_up_block(slv_system_t sys,int bnum)
{
  struct rel_relation **rp;
  struct var_variable **vp;
  mtx_region_t reg;
  const mtx_block_t *b;
  int32 c,vlen,rlen;
  var_filter_t vf;
  rel_filter_t rf;

  if (sys==NULL) return 1;
  rlen = slv_get_num_solvers_rels(sys);
  vlen = slv_get_num_solvers_vars(sys);
  if (rlen ==0 || vlen == 0) return 1;

  rp = slv_get_solvers_rel_list(sys);
  vp = slv_get_solvers_var_list(sys);
  assert(rp!=NULL);
  assert(vp!=NULL);

  rf.matchbits = (REL_INCLUDED | REL_EQUALITY | REL_INBLOCK | REL_ACTIVE);
  rf.matchvalue = (REL_INCLUDED | REL_EQUALITY | REL_INBLOCK | REL_ACTIVE);
  vf.matchbits =(VAR_INCIDENT |VAR_SVAR | VAR_FIXED |VAR_INBLOCK | VAR_ACTIVE);
  vf.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_INBLOCK | VAR_ACTIVE);

  b = slv_get_solvers_blocks(sys);
  assert(b!=NULL);
  if (bnum <0 || bnum >= b->nblocks || b->block == NULL) return 1;
  reg = b->block[bnum];
  for (c=reg.col.low; c<=reg.col.high; c++) {
    var_set_in_block(vp[c],1);
  }
  for (c=reg.row.low; c<=reg.row.high; c++) {
    rel_set_in_block(rp[c],1);
  }
  return 0;
}

/* returns 0 if ok, OTHERWISE if madness detected.
 */
int slv_spk1_reorder_block(slv_system_t sys,int bnum,int transpose)
{
  struct rel_relation **rp;
  struct var_variable **vp;
  mtx_region_t reg;
  const mtx_block_t *b;
  mtx_matrix_t mtx;
  mtx_coord_t coord;
  int32 c,vlen,rlen;
  var_filter_t vf;
  rel_filter_t rf;
  dof_t *d;

  if (sys==NULL) return 1;
  rlen = slv_get_num_solvers_rels(sys);
  vlen = slv_get_num_solvers_vars(sys);
  if (rlen ==0 || vlen == 0) return 1;

  rp = slv_get_solvers_rel_list(sys);
  vp = slv_get_solvers_var_list(sys);
  assert(rp!=NULL);
  assert(vp!=NULL);
  rf.matchbits = (REL_INCLUDED | REL_EQUALITY | REL_INBLOCK | REL_ACTIVE);
  rf.matchvalue = (REL_INCLUDED | REL_EQUALITY | REL_INBLOCK | REL_ACTIVE);
  vf.matchbits =(VAR_INCIDENT |VAR_SVAR | VAR_FIXED |VAR_INBLOCK | VAR_ACTIVE);
  vf.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_INBLOCK | VAR_ACTIVE);

  mtx = mtx_create();
  mtx_set_order(mtx,MAX(rlen,vlen));
  b = slv_get_solvers_blocks(sys);
  assert(b!=NULL);
  if (bnum <0 || bnum >= b->nblocks || b->block == NULL) return 1;
  reg = b->block[bnum];
  for (c=reg.col.low; c<=reg.col.high; c++) {
    var_set_in_block(vp[c],1);
  }
  for (c=reg.row.low; c<=reg.row.high; c++) {
    rel_set_in_block(rp[c],1);
  }
  if (reg.row.low != reg.col.low || reg.row.high != reg.col.high) {
    return 1; /* must be square */
    /* could also enforce minsize 3x3, but someone my call with a
     * partitionable region, so don't want to.
     */
  }

  if (slv_make_incidence_mtx(sys,mtx,&vf,&rf)) {
    FPRINTF(stderr,
      "slv_spk1_reorder_block: failure in creating incidence matrix.\n");
    mtx_destroy(mtx);
    return 1;
  }
  /* verify that block has no empty columns, though not checking diagonal */
  for (c = reg.row.low; c <= reg.row.high; c++) {
    coord.col = mtx_FIRST;
    coord.row = c;
    if (mtx_next_in_row(mtx,&coord,mtx_ALL_COLS), coord.col == mtx_LAST) {
      mtx_destroy(mtx);
      FPRINTF(stderr, "slv_spk1_reorder_block: empty row (%d) found.\n",c);
      return 1;
    }
    coord.row = mtx_FIRST;
    coord.col = c;
    if (mtx_next_in_col(mtx,&coord,mtx_ALL_ROWS), coord.row == mtx_LAST) {
      FPRINTF(stderr, "slv_spk1_reorder_block: empty col (%d) found.\n",c);
      mtx_destroy(mtx);
      return 1;
    }
  }
  if (transpose) {
    mtx_reorder(mtx,&reg,mtx_TSPK1);
  } else {
    mtx_reorder(mtx,&reg,mtx_SPK1);
  }
  if (reindex_vars_from_mtx(sys,reg.col.low,reg.col.high,mtx)) {
    mtx_destroy(mtx);
    return 2;
  }
  if (reindex_rels_from_mtx(sys,reg.row.low,reg.row.high,mtx)) {
    mtx_destroy(mtx);
    return 2;
  }
  d = slv_get_dofdata(sys);
  d->reorder.block_reordering = 1;	/* spk1 */

  mtx_destroy(mtx);
  return 0;
}

/*
	A function to be called on the leftover diagonal blocks of mr_bisect.
	This function should be thoroughly investigated, which it has not been.
*/
static int tear_subreorder(slv_system_t server,
                           mtx_matrix_t mtx, mtx_region_t *reg)
{
  assert(mtx != NULL && reg !=NULL);
  (void)server;
  if (g_blockmethod==mtx_UNKNOWN) {
    mtx_reorder(mtx,reg,mtx_SPK1);
  } else {
    mtx_reorder(mtx,reg,g_blockmethod);
  }
  return 0;
}

int slv_tear_drop_reorder_block(slv_system_t sys, int32 bnum,
		int32 cutoff, int two,
		enum mtx_reorder_method blockmethod
){
  struct rel_relation **rp;
  struct var_variable **vp;
  const mtx_block_t *b;
  dof_t *d;
  mr_reorder_t *mrsys;
  mtx_matrix_t mtx;
  mtx_region_t reg;
  mtx_coord_t coord;
  int32 c,vlen,rlen,modcount;
  var_filter_t vf;
  rel_filter_t rf;

  if (sys==NULL) return 1;
  rlen = slv_get_num_solvers_rels(sys);
  vlen = slv_get_num_solvers_vars(sys);
  if (rlen ==0 || vlen == 0) return 1;

  rp = slv_get_solvers_rel_list(sys);
  vp = slv_get_solvers_var_list(sys);
  assert(rp!=NULL);
  assert(vp!=NULL);
  rf.matchbits = (REL_INCLUDED | REL_EQUALITY | REL_INBLOCK | REL_ACTIVE);
  rf.matchvalue = (REL_INCLUDED | REL_EQUALITY | REL_INBLOCK| REL_ACTIVE);
  vf.matchbits =(VAR_INCIDENT |VAR_SVAR | VAR_FIXED |VAR_INBLOCK | VAR_ACTIVE);
  vf.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_INBLOCK | VAR_ACTIVE);

  mtx = mtx_create();
  mtx_set_order(mtx,MAX(rlen,vlen));
  b = slv_get_solvers_blocks(sys);
  assert(b!=NULL); /* probably shouldn't be an assert here ... */
  if (bnum <0 || bnum >= b->nblocks || b->block == NULL) {
    mtx_destroy(mtx);
    return 1;
  }
  reg = b->block[bnum];
  /* do the flag setting even if we don't do the reorder */
  for (c=reg.col.low; c<=reg.col.high; c++) {
    var_set_in_block(vp[c],1);
  }
  for (c=reg.row.low; c<=reg.row.high; c++) {
    rel_set_in_block(rp[c],1);
  }
  if (reg.row.low != reg.col.low || reg.row.high != reg.col.high) {
    mtx_destroy(mtx);
    return 1; /* must be square */
  }
  if (reg.row.high - reg.row.low < 3) {
    mtx_destroy(mtx);
    return 0; /* must be 3x3 or bigger to have any effect */
  }

  if (slv_make_incidence_mtx(sys,mtx,&vf,&rf)) {
    FPRINTF(stderr,
      "slv_tear_drop_reorder_block: failure in creating incidence matrix.\n");
    mtx_destroy(mtx);
    return 1;
  }
  /* verify that block has no empty columns, though not checking diagonal */
  for (c = reg.row.low; c <= reg.row.high; c++) {
    coord.col = mtx_FIRST;
    coord.row = c;
    if (mtx_next_in_row(mtx,&coord,mtx_ALL_COLS), coord.col == mtx_LAST) {
      mtx_destroy(mtx);
      FPRINTF(stderr, "slv_tear_drop_reorder_block: empty row (%d) found.\n",c);
      return 1;
    }
    coord.row = mtx_FIRST;
    coord.col = c;
    if (mtx_next_in_col(mtx,&coord,mtx_ALL_ROWS), coord.row == mtx_LAST) {
      FPRINTF(stderr, "slv_tear_drop_reorder_block: empty col (%d) found.\n",c);
      mtx_destroy(mtx);
      return 1;
    }
  }

  modcount = slv_get_num_models(sys);
  mrsys = mr_reorder_create(sys,mtx,modcount);
  if (mrsys==NULL) return 1;
  mrsys->cutoff = cutoff;
  g_blockmethod = blockmethod;
  if (!two) {
    mr_bisect_partition(mrsys,&reg,1,tear_subreorder);
  } else {
    mr_bisect_partition2(mrsys,&reg,1,tear_subreorder);
  }
#if 1
  FPRINTF(stderr,"Model-based reordering: (tear-style = %d)\n",two);
  FPRINTF(stderr,"Min. tearable size:\t%d\n",mrsys->cutoff);
  FPRINTF(stderr,"Tears:\t\t%d\n",mrsys->ntears);
  FPRINTF(stderr,"Partitionings:\t%d\n",mrsys->nblts);
#endif
  reg = b->block[bnum]; /* bisect likely munged reg */
  if (reindex_vars_from_mtx(sys,reg.col.low,reg.col.high,mtx)) {
    mtx_destroy(mtx);
    mr_reorder_destroy(mrsys);
    return 2;
  }
  if (reindex_rels_from_mtx(sys,reg.row.low,reg.row.high,mtx)) {
    mtx_destroy(mtx);
    mr_reorder_destroy(mrsys);
    return 2;
  }
  d = slv_get_dofdata(sys);
  d->reorder.block_reordering = 2;	/* tear_drop_baa */

  mtx_destroy(mtx);
  mr_reorder_destroy(mrsys);
  return 0;
}

/*------------------------------------------------------------------------------
  DEBUG OUTPUT for BLOCK STRUCTURE

  (it *would* belong in the 'mtx' section, but it outputs var and rel names
  instead of just numbers *)
*/

extern int system_block_debug(slv_system_t sys, FILE *fp){
	int i,j,nr,nc;
	dof_t *dof;
	char *relname, *varname;
	struct var_variable **vlist;
	struct rel_relation **rlist;
	mtx_region_t b;
	dof = slv_get_dofdata(sys);
	char s[80];
	char color;
	
	fprintf(fp,"\n\nSLV_SYSTEM BLOCK INFO\n\n");
	
	fprintf(fp,"Structural rank: %d\n",dof->structural_rank);
	fprintf(fp,"Included rels: %d\n",dof->n_rows);
	fprintf(fp,"Incident, free vars: %d\n",dof->n_cols);
	fprintf(fp,"Fixed vars: %d\n",dof->n_fixed);
	fprintf(fp,"Unincluded rels: %d\n",dof->n_unincluded);
	fprintf(fp,"Number of blocks: %d\n",dof->blocks.nblocks);

	vlist = slv_get_solvers_var_list(sys);
	rlist = slv_get_solvers_rel_list(sys);
	color = (fp == stderr || fp==stdout);
	for(i=0;i<dof->blocks.nblocks;++i){
		if(color){
			if(i%2)color_on(fp,"0;33");
			else color_on(fp,"0;30");
		}
		b = dof->blocks.block[i];
		nr = b.row.high - b.row.low + 1;
		nc = b.col.high - b.col.low + 1;
		snprintf(s,80,"BLOCK %d (%d x %d)",i,nr,nc);
		fprintf(fp,"%-18s",s);
		snprintf(s,80,"%-18s","");
		for(j=0;j<MAX(nr,nc); ++j){
			fprintf(fp,"%s%d",(j?s:""),j);
			if(j<nr){
				relname = rel_make_name(sys,rlist[b.row.low + j]);
				fprintf(fp,"\t%-20s",relname);
				ASC_FREE(relname);
			}else{
				fprintf(fp,"\t%-20s","");
			}
			if(j<nc){
				varname = var_make_name(sys,vlist[b.col.low + j]);
				fprintf(fp,"\t%-20s",varname);
				ASC_FREE(varname);
			}
			fprintf(fp,"\n");
		}
	}
	if(color)color_off(fp);
	return 0;				
}
	
/*------------------------------------------------------------------------------
  PARTITIONING for DIFFERENTIAL/ALGEBRAIC SYSTEMS
*/

/**
	This macro will generate 'system_var_list_debug(sys)' and 'system_rel_list_debug(sys)'
	and maybe other useful things if you're lucky.
*/
#define LIST_DEBUG(TYPE,FULLTYPE) \
	void system_##TYPE##_list_debug(slv_system_t sys){ \
		struct FULLTYPE **list; \
		int n, i; \
		n = slv_get_num_solvers_##TYPE##s(sys); \
		list = slv_get_solvers_##TYPE##_list(sys); \
	 \
		CONSOLE_DEBUG("printing " #TYPE " list"); \
		char *name; \
		for(i=0;i<n;++i){ \
			name = TYPE##_make_name(sys,list[i]); \
			fprintf(stderr,"%d: %s\n",i,name); \
			ASC_FREE(name); \
		} \
		fprintf(stderr,"-----\n\n"); \
	}

LIST_DEBUG(var,var_variable)
LIST_DEBUG(rel,rel_relation)

#ifdef CUT_DEBUG
# define MAYBE_WRITE_LIST(TYPE) system_##TYPE##_list_debug(sys)
# define MAYBE_CONSOLE_DEBUG(MSG,...) CONSOLE_DEBUG(MSG,#ARGS)
#else
# define MAYBE_WRITE_LIST(TYPE)
# define MAYBE_CONSOLE_DEBUG(MSG,...)
#endif

/**
	This is a big durtie macro to perform cuts on our solvers_*_lists.
	The function will start at position 'begin' and move through all elements
	of the list from that point to the end. Any 'good' items matching the filter
	are	moved to the start of the range traversed. And 'bad' ones that dont
	get moved to the end. At the end, the number of items found matching the
	filter is returned in 'numgood'. There will be that many 'good' items 
	in place from position 'begin' onwards.
*/
#define SYSTEM_CUT_LIST(TYPE,FULLTYPE) \
	int system_cut_##TYPE##s(slv_system_t sys, const int begin, const TYPE##_filter_t *filt, int *numgood){ \
		struct FULLTYPE **list, **start, **end, *swap; \
		int len, i; \
		char *name; \
	 \
		asc_assert(filt); \
	 \
		MAYBE_WRITE_LIST(TYPE); \
	 \
		list = slv_get_solvers_##TYPE##_list(sys); \
		len = slv_get_num_solvers_##TYPE##s(sys); \
		if(len==0){ \
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"There are no " #TYPE "s!"); \
			return 1; \
		} \
	 \
		MAYBE_CONSOLE_DEBUG("SORTING"); \
	 \
		start = list + begin; \
		end = list + len; \
		while(start < end){ \
			if(TYPE##_apply_filter(*start,filt)){ \
				start++; continue; \
			} \
			if(!TYPE##_apply_filter(*(--end),filt))continue; \
			swap = *end; \
			*end = *start; \
			*start = swap; \
			start++; \
		} \
	 \
		MAYBE_WRITE_LIST(TYPE); \
	 \
		MAYBE_CONSOLE_DEBUG("UPDATING"); \
	 \
		/* update the sindex for each after start */ \
		*numgood = 0; \
		for(i=begin;i<len;++i){ \
			name = TYPE##_make_name(sys,list[i]); \
			if(TYPE##_apply_filter(list[i],filt)){ \
				MAYBE_CONSOLE_DEBUG("%s: good",name); \
				(*numgood)++; \
			}else{ \
				MAYBE_CONSOLE_DEBUG("%s: bad",name); \
			} \
			ASC_FREE(name); \
			TYPE##_set_sindex(list[i],i); \
		} \
		MAYBE_CONSOLE_DEBUG("numgood = %d",*numgood); \
		 \
		return 0; \
	} 

SYSTEM_CUT_LIST(var,var_variable);
SYSTEM_CUT_LIST(rel,rel_relation);

