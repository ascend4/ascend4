/*
 *  Solver Standard Clients
 *  by Benjamin Andrew Allan
 *  5/19/96
 *  Version: $Revision: 1.28 $
 *  Version control file: $RCSfile: slv_stdcalls.c,v $
 *  Date last modified: $Date: 1998/06/16 16:53:04 $
 *  Last modified by: $Author: mthomas $
 *
 *  Copyright(C) 1996 Benjamin Andrew Allan
 *  Copyright(C) 1998 Carnegie Mellon University
 *
 *  This file is part of the ASCEND IV math programming system.
 *  Here is where we register our normal solvers and keep things
 *  like reordering clients and so forth. This file should probably
 *  be broken up into one file-one client layout for the unregistered
 *  clients so that we can load them selectively.
 *
 *  The Ascend Math Programming System is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  The Ascend Math Programming System is distributed in hope that it
 *  will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check
 *  the file named COPYING.
 */

#include "utilities/ascConfig.h"
#include "compiler/compiler.h"
#include "utilities/ascMalloc.h"
#include "general/list.h"
#include "solver/mtx.h"
#include "solver/slv_types.h"
#include "solver/var.h"
#include "solver/rel.h"
#include "solver/discrete.h"
#include "solver/conditional.h"
#include "solver/logrel.h"
#include "solver/relman.h"
#include "solver/logrelman.h"
#include "solver/bnd.h"
#include "solver/slv_common.h"
#include "solver/linsol.h"
#include "solver/linsolqr.h"
#include "solver/slv_client.h"
/* header of registered clients */
#include "solver/slv0.h"
#include "solver/slv1.h"
#include "solver/slv2.h"
#include "solver/slv3.h"
#include "solver/slv6.h"
#include "solver/slv7.h"
#include "solver/slv8.h"
#include "solver/slv9.h"
#include "solver/slv9a.h"
#include "solver/model_reorder.h"
#include "solver/slv_stdcalls.h"

#define KILL 0 /* deleteme code. compile old code if kill = 1 */
#define NEEDSTOBEDONE 0 /* indicates code/comments that are not yet ready */
#define USECODE 0  /* Code in good shape but not used currently */

/*
 * Here we play some hefty jacobian reordering games.
 * What we want to happen eventually is as follows:
 *
 * Get the free & incident pattern for include relations.
 * Output assign jacobian.
 * BLT permute the jacobian. if underspecified, fake rows
 *	to make things appear square.
 * For all leading square blocks apply reordering (kirk, etc)
 * If trailing rectangular block, apply clever kirkbased scheme not known.
 *
 * At present, we aren't quite so clever. We:
 *
 * Get the free & incident pattern for include relations.
 * Output assign jacobian.
 * BLT permute the square output assigned region of the jacobian.
 * For all leading square blocks apply reordering (kirk, etc)
 *
 * Collect the block list as part of the master data structure.
 * Set sindices for rels, vars as current rows/cols in matrix so
 * that the jacobian is 'naturally' preordered for all solvers.
 *
 * Solvers are still free to reorder their own matrices any way they like;
 * it's probably a dumb idea, though.
 *
 * returns 0 if ok, 1 if out of memory.
 */

#define MIMDEBUG 0
/*
 * Populates a matrix according to the sys solvers_vars, solvers_rels
 * lists and the filters given. The filter should have at least
 * SVAR = 1 bit on. mtx given must be created (not null) and with
 * order >= MAX( slv_get_num_solvers_rels(sys),slv_get_num_solvers_vars(sys));
 * returns 0 if went ok.
 */
int slv_std_make_incidence_mtx(slv_system_t sys, mtx_matrix_t mtx,
                               var_filter_t *vf,rel_filter_t *rf)
{
#if MIMDEBUG
  FILE *fp;
#endif
  int32 r, rlen,ord;
  struct rel_relation **rp;

  if (sys==NULL || mtx == NULL || vf == NULL || rf == NULL) {
    FPRINTF(stderr,"make_incidence called with null\n");
    return 1;
  }
  rp = slv_get_solvers_rel_list(sys);
  assert(rp!=NULL);
  rlen = slv_get_num_solvers_rels(sys);
  ord = MAX(rlen,slv_get_num_solvers_vars(sys));
  if (ord > mtx_order(mtx)) {
    FPRINTF(stderr,"make_incidence called with undersized matrix\n");
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

#define RIDEBUG 0
/* returns 0 if successful, 1 if insufficient memory. Does not change
 * the data in mtx. Orders the solvers_var list of the system to
 * match the permutation on the given mtx. It is assumed that
 * the org cols of mtx == var list position. Only the
 * vars in range lo to hi of the var list are affected and
 * only these vars should appear in the same org column range of
 * the mtx. This should not be called on blocks less than 3x3.
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
  vtmp = (struct var_variable **)ascmalloc(vlen*sizeof(struct var_variable *));
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
/* returns 0 if successful, 1 if insufficient memory. Does not change
 * the data in mtx. Orders the solvers_rel list of the system to
 * match the permutation on the given mtx. It is assumed that
 * the org rows of mtx == rel list position. Only the
 * rels in range lo to hi of the rel list are affected and
 * only these rels should appear in the same org row range of
 * the input mtx. This should not be called on blocks less than 3x3.
 */
static int reindex_rels_from_mtx(slv_system_t sys, int32 lo, int32 hi,
                                 const mtx_matrix_t mtx)
{
  struct rel_relation **rtmp, **rp;
  int32 c,v,rlen;

  rp = slv_get_solvers_rel_list(sys);
  rlen = slv_get_num_solvers_rels(sys);
  /* on rtmp we DONT have the terminating null */
  rtmp = (struct rel_relation **)ascmalloc(rlen*sizeof(struct rel_relation *));
  if (rtmp == NULL) {
    return 1;
  }
  /* copy pointers to rtmp in order desired and copy back rather than sort.
   * do this only in the row range of interest. */
  for (c=lo;c<=hi;c++) {
    v = mtx_row_to_org(mtx,c);
#if RIDEBUG
    FPRINTF(stderr,"Old rel sindex (org) %d becoming sindex (cur) %d\n",v,c);
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


/* returns 0 if ok, OTHERWISE if madness detected.
 * doesn't grok inequalities.
 */
#define SBPDEBUG 0
int slv_block_partition_real(slv_system_t sys,int uppertriangular)
{
#if SBPDEBUG
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

  if (slv_std_make_incidence_mtx(sys,mtx,&vf,&rf)) {
    FPRINTF(stderr,
      "slv_block_partition: failure in creating incidence matrix.\n");
    mtx_destroy(mtx);
    return 1;
  }

  mtx_output_assign(mtx,rlen,vlen);
  rank = mtx_symbolic_rank(mtx);
  if (rank == 0 ) return 1; 	/* nothing to do, eh? */
  /* lot of whining about dof */
  if (rank < nrow) {
    FPRINTF(stderr,"System is row rank deficient (%d dependent equations)\n",
            nrow - rank);
  }
  if (rank < ncol) {
    if ( nrow != rank) {
      FPRINTF(stderr,"System is row rank deficient with %d excess columns.\n",
              ncol - rank);
    } else {
      FPRINTF(stderr,"System has %d degrees of freedom.\n", ncol - rank);
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
  if (uppertriangular) {
    mtx_ut_partition(mtx);
  } else {
    mtx_partition(mtx);
  }
  /* copy the block list. there is at least one if rank >=1 as above */
  len = mtx_number_of_blocks(mtx);
  newblocks = (mtx_region_t *)ascmalloc(len*sizeof(mtx_region_t));
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
  /* the next two lines assume inequalities are unincluded.
   */
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

#if SBPDEBUG
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


#if SBPDEBUG
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
  mtx_destroy(mtx);
  return 0;
}


#if 0 /* code not currently used */
#ifdef STATIC_HARWELL
extern void mc21b();
extern void mc13emod();
/* returns 0 if ok, OTHERWISE if madness detected.
 * doesn't grok inequalities.
 * CURRENTLY DOESN'T DO WELL WHEN NCOL<NROW
 */
#define SBPDEBUG 0
int slv_block_partition_harwell(slv_system_t sys)
{
#if SBPDEBUG
  FILE *fp;
#endif
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
  rtmp = (struct rel_relation **)ascmalloc(rlen*sizeof(struct rel_relation *));
  rel = (struct rel_relation *)ascmalloc(sizeof(struct rel_relation *));
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
  icn = (int32 *)ascmalloc(licn*sizeof(int32));
  ip = (int32 *)ascmalloc(size*sizeof(int32));
  lenr = (int32 *)ascmalloc(size*sizeof(int32));
  iperm = (int32 *)ascmalloc(size*sizeof(int32));
  iw1 = (int32 *)ascmalloc(size*sizeof(int32));
  iw2 = (int32 *)ascmalloc(size*sizeof(int32));
  iw3 = (int32 *)ascmalloc(size*sizeof(int32));
  arp = (int32 *)ascmalloc(size*sizeof(int32));
  ipnew = (int32 *)ascmalloc(size*sizeof(int32));
  ib = (int32 *)ascmalloc(size*sizeof(int32));
  row_perm = (int32 *)ascmalloc(size*sizeof(int32));


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
  newblocks = (mtx_region_t *)ascmalloc(len*sizeof(mtx_region_t));
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
#endif /*STATIC_HARWELL*/
#endif /* 0 */


void slv_sort_rels_and_vars(slv_system_t sys,
			    int32 *rel_count,
			    int32 *var_count)
{
  struct rel_relation **rp;
  struct rel_relation **rtmp;
  struct rel_relation *rel;
  struct var_variable **vp;
  struct var_variable **vtmp;
  struct var_variable *var;
  int32 nrow,ncol,rlen,vlen,order,rel_tmp_end;
  int32 r,c,var_tmp_end,len;
  var_filter_t vf;
  rel_filter_t rf;

  /* get rel and var info */
  rp = slv_get_solvers_rel_list(sys);
  vp = slv_get_solvers_var_list(sys);
  rlen = slv_get_num_solvers_rels(sys);
  vlen = slv_get_num_solvers_vars(sys);
  if (rlen ==0 || vlen == 0) return;
  order = MAX(rlen,vlen);

  assert(var_count != NULL && rel_count != NULL);
  *var_count = *rel_count = -1;

  /* allocate temp arrays */
  vtmp = (struct var_variable **)ascmalloc(vlen*sizeof(struct var_variable *));
  if (vtmp == NULL) {
    return;
  }
  rtmp = (struct rel_relation **)ascmalloc(rlen*sizeof(struct rel_relation *));
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
    newblocks = (mtx_region_t *)ascmalloc(sizeof(mtx_region_t));
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

  if (slv_std_make_incidence_mtx(sys,mtx,&vf,&rf)) {
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

/* global to get around the mr header */
static
enum mtx_reorder_method g_blockmethod = mtx_UNKNOWN;

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
                                enum mtx_reorder_method blockmethod)
{
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

  if (slv_std_make_incidence_mtx(sys,mtx,&vf,&rf)) {
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

int slv_insure_bounds(slv_system_t sys,int32 lo,int32 hi, FILE *mif)
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
        FPRINTF(mif,"Bounds for variable ");
        var_write_name(sys,var,mif);
        FPRINTF(mif," are inconsistent [%g,%g].\n",low,high);
        FPRINTF(mif,"Bounds will be swapped.\n");
      }
      var_set_upper_bound(var, low);
      var_set_lower_bound(var, high);
      low = var_lower_bound(var);
      high = var_upper_bound(var);
      nchange++;
    }

    if( low > val ) {
      if (mif!=NULL) {
        FPRINTF(mif,"Variable ");
        var_write_name(sys,var,mif);
        FPRINTF(mif," was initialized below its lower bound.\n");
        FPRINTF(mif,"It will be moved to its lower bound.\n");
      }
      var_set_value(var, low);
    } else {
      if( val > high ) {
        if (mif!=NULL) {
          FPRINTF(mif,"Variable ");
          var_write_name(sys,var,mif);
          FPRINTF(mif," was initialized above its upper bound.\n");
          FPRINTF(mif,"It will be moved to its upper bound.\n");
        }
        var_set_value(var, high);
        nchange++;
      }
    }
  }
  return nchange;
}

void slv_check_bounds(const slv_system_t sys,int32 lo,int32 hi,
                      FILE *mif,const char *label)
{
  real64 val,low,high;
  int32 c,len;
  struct var_variable *var, **vp;
  static char defaultlabel[] = "";

  if (label==NULL) label = defaultlabel;
  if (sys==NULL || mif == NULL) return;
  vp = slv_get_solvers_var_list(sys);
  if (vp==NULL) return;
  len = slv_get_num_solvers_vars(sys);
  if (lo > len || hi > len) {
    FPRINTF(stderr,"slv_check_bounds miscalled\n");
    return;
  }
  for (c= lo; c <= hi; c++) {
    var = vp[c];
    low = var_lower_bound(var);
    high = var_upper_bound(var);
    val = var_value(var);
    if( low > high ) {
      FPRINTF(mif,"Bounds for %s variable ",label);
      var_write_name(sys,var,mif);
      FPRINTF(mif,"\nare inconsistent [%g,%g].\n",low,high);
    }

    if( low > val ) {
      FPRINTF(mif,"%s variable ",label);
      var_write_name(sys,var,mif);
      FPRINTF(mif,"\nwas initialized below its lower bound.\n");
    } else {
      if( val > high ) {
        FPRINTF(mif,"%s variable ",label);
        var_write_name(sys,var,mif);
        FPRINTF(mif," was initialized above its upper bound.\n");
      }
    }
  }
  return;
}

int SlvRegisterStandardClients(void)
{
  int nclients = 0;
  int status;
#ifdef STATIC_SLV
  status = slv_register_client(slv0_register,NULL,NULL);
  if (status) {
    FPRINTF(stderr,"Unable to register slv0.\n");
  } else {
    nclients++;
  }
#endif
#ifdef STATIC_MINOS
  status = slv_register_client(slv1_register,NULL,NULL);
  if (status) {
    FPRINTF(stderr,"Unable to register slv1.\n");
  } else {
    nclients++;
  }
#endif
#ifdef STATIC_QRSLV
  status = slv_register_client(slv3_register,NULL,NULL);
  if (status) {
    FPRINTF(stderr,"Unable to register slv3.\n");
  } else {
    nclients++;
  }
#endif
#ifdef STATIC_CSLV
  status = slv_register_client(slv4_register,NULL,NULL);
  if (status) {
    FPRINTF(stderr,"Unable to register slv4.\n");
  } else {
    nclients++;
  }
#endif
#ifdef STATIC_LSSLV
  status = slv_register_client(slv5_register,NULL,NULL);
  if (status) {
    FPRINTF(stderr,"Unable to register slv5.\n");
  } else {
    nclients++;
  }
#endif
#ifdef STATIC_MPS
  status = slv_register_client(slv6_register,NULL,NULL);
  if (status) {
    FPRINTF(stderr,"Unable to register slv6.\n");
  } else {
    nclients++;
  }
#endif
#ifdef STATIC_NGSLV
  status = slv_register_client(slv7_register,NULL,NULL);
  if (status) {
    FPRINTF(stderr,"Unable to register slv7.\n");
  } else {
    nclients++;
  }
#endif
#ifdef STATIC_OPTSQP
  status = slv_register_client(slv2_register,NULL,NULL);
  if (status) {
    FPRINTF(stderr,"Unable to register slv2.\n");
  } else {
    nclients++;
  }
#endif
#if (defined(STATIC_CONOPT) || defined(DYNAMIC_CONOPT))
  status = slv_register_client(slv8_register,NULL,NULL);
  if (status) {
    FPRINTF(stderr,"Unable to register slv8.\n");
  } else {
    nclients++;
  }
#endif
#ifdef STATIC_CMSLV
  status = slv_register_client(slv9_register,NULL,NULL);
  if (status) {
    FPRINTF(stderr,"Unable to register slv9.\n");
  } else {
    nclients++;
  }
#endif
#ifdef STATIC_LRSLV
  status = slv_register_client(slv9a_register,NULL,NULL);
  if (status) {
    FPRINTF(stderr,"Unable to register slv9a.\n");
  } else {
    nclients++;
  }
#endif
  return nclients;
}



/************************************************************************\
          Output Assignment and partitiong in Logical Relations
\************************************************************************/

#define MLIMDEBUG 0
/*
 * Populates a matrix according to the sys solvers_dvars, solvers_logrels
 * lists and the filters given. mtx given must be created (not null)
 * and with order >= MAX( slv_get_num_solvers_logrels(sys),
 *                        slv_get_num_solvers_dvars(sys));
 * returns 0 if went ok.
 */
int slv_std_make_log_incidence_mtx(slv_system_t sys, mtx_matrix_t mtx,
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
  dvtmp = (struct dis_discrete **)
                                ascmalloc(vlen*sizeof(struct dis_discrete *));
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
  lrtmp = (struct logrel_relation **)
                             ascmalloc(rlen*sizeof(struct logrel_relation *));
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

/*
 * returns 0 if ok, OTHERWISE if madness detected.
 */
#define SLBPDEBUG 0
int slv_log_block_partition(slv_system_t sys)
{
#if SLBPDEBUG
  FILE *fp;
#endif
  struct logrel_relation **lrp;
  struct dis_discrete **dvp;
  mtx_region_t  *newblocks;
  dof_t *d;
  int32 nrow,ncol;
  int32 ncolpfix, nrowpun, vlenmnv;
  mtx_matrix_t mtx;
  int32 order, rank;
  int32 c,len,dvlen,r,lrlen;
  dis_filter_t dvf;
  logrel_filter_t lrf;

  lrp = slv_get_solvers_logrel_list(sys);
  dvp = slv_get_solvers_dvar_list(sys);
  lrlen = slv_get_num_solvers_logrels(sys);
  dvlen = slv_get_num_solvers_dvars(sys);
  if (lrlen ==0 || dvlen == 0) return 1;
  order = MAX(lrlen,dvlen);

  lrf.matchbits = (LOGREL_ACTIVE);
  lrf.matchvalue = (LOGREL_ACTIVE);
  dvf.matchbits = (DIS_INCIDENT | DIS_BVAR  | DIS_ACTIVE);
  dvf.matchvalue = (DIS_INCIDENT | DIS_BVAR  | DIS_ACTIVE);

  /* nrow plus unincluded logrels */
  nrowpun = slv_count_solvers_logrels(sys,&lrf);
  /* ncol plus fixed discrete vars */
  ncolpfix = slv_count_solvers_dvars(sys,&dvf);

  dvf.matchbits = (DIS_BVAR);
  dvf.matchvalue = 0;

  /* dvlen minus non boolean vars */
  vlenmnv = dvlen - slv_count_solvers_dvars(sys,&dvf);

  lrf.matchbits = (LOGREL_INCLUDED | LOGREL_ACTIVE);
  lrf.matchvalue = (LOGREL_INCLUDED | LOGREL_ACTIVE);
  dvf.matchbits = (DIS_INCIDENT | DIS_BVAR | DIS_FIXED | DIS_ACTIVE);
  dvf.matchvalue = (DIS_INCIDENT | DIS_BVAR | DIS_ACTIVE);

  nrow = slv_count_solvers_logrels(sys,&lrf);
  ncol = slv_count_solvers_dvars(sys,&dvf);

  mtx = slv_get_sys_mtx(sys);
  mtx_set_order(mtx,order);

  if (slv_std_make_log_incidence_mtx(sys,mtx,&dvf,&lrf)) {
    FPRINTF(stderr,
      "slv_log_block_partition: failure in creating incidence matrix.\n");
    mtx_destroy(mtx);
    return 1;
  }

  mtx_output_assign(mtx,lrlen,dvlen);
  rank = mtx_symbolic_rank(mtx);
  if (rank == 0 ) return 1;
  if (rank < nrow) {
    FPRINTF(stderr,"rank<nrow in slv_log_block_partition\n");
    FPRINTF(stderr,"dependent logical relations ? \n");
  }
  if (rank < ncol) {
    if ( nrow != rank) {
      FPRINTF(stderr,"rank<ncol and nrow!=rank in slv_log_block_partition.\n");
      FPRINTF(stderr,"Excess of columns ? \n");
    } else {
      FPRINTF(stderr,"rank<ncol but nrow==rank in slv_log_block_partition.\n");
      FPRINTF(stderr,"Degrees of freedom ? \n");
    }
  }
  if (ncol == nrow) {
    if (ncol == rank) {
      FPRINTF(stderr,"\n");
      FPRINTF(stderr,
             "System of logical relations does not need Logic Inference \n");
    }
    if (ncol != rank) {
      FPRINTF(stderr,"but ncol!=rank.\n");
      FPRINTF(stderr,"Rank deficient ? \n");
    } else {
      FPRINTF(stderr,"\n");
    }
  }
  mtx_partition(mtx);
  /* copy the block list. there is at least one if rank >=1 as above */
  len = mtx_number_of_blocks(mtx);
  newblocks = (mtx_region_t *)ascmalloc(len*sizeof(mtx_region_t));
  if (newblocks == NULL) {
    mtx_destroy(mtx);
    return 2;
  }
  for (c = 0 ; c < len; c++) {
    mtx_block(mtx,c,&(newblocks[c]));
  }
  slv_set_solvers_log_blocks(sys,len,newblocks); /* give it to the system */
  d = slv_get_log_dofdata(sys);
  d->structural_rank = rank;
  d->n_rows = nrow;
  d->n_cols = ncol;
  d->n_fixed = ncolpfix - ncol;
  d->n_unincluded = nrowpun - nrow;
  d->reorder.partition = 1;		/* yes */
  d->reorder.basis_selection = 0;	/* none yet */
  d->reorder.block_reordering = 0;	/* none */

#if SLBPDEBUG
  fp = fopen("/tmp/sbp1.plot","w+");
  if (fp !=NULL) {
    mtx_write_region_plot(fp,mtx,mtx_ENTIRE_MATRIX);
    fclose(fp);
  }
#endif


  len = vlenmnv - 1;  /* row of last possible boolean variable */
  for (c=ncol; len > c ; ) { /* sort the fixed out of inactive */
    r = mtx_col_to_org(mtx,c);
    if ( dis_fixed(dvp[r]) && dis_active(dvp[r])) {
      c++;
    } else {
      mtx_swap_cols(mtx,c,len);
      len--;
    }
  }

  /* Now, leftover columns are where we want them.
   * That is, unassigned incidence is next to assigned and fixed on right with
   * no columns which do not correspond to dis vars in the range 0..dvlen-1.
   */
  /* there aren't any nonincident dvars in the solvers dvarlist, so
   * we now have the columns in |assigned|dof|fixed|inactive|nonvars| order */

  /* rows 0->lrlen-1 should be actual logrels, though, and not phantoms.
   * At this point we should have
   * rows - assigned
   *      - unassigned w/incidence
   *      - unincluded and included w/out free incidence and inactive mixed.
   */

  /* sort unincluded logrelations to bottom of good region */
  /* this is needed because the idiot user may have fixed everything */
  /* in a logrelation, but we want those rows not to be confused with */
  /* unincluded rows.*/
  len = lrlen - 1;  /* row of last possible relation */
  /* sort the inactive out of the unassigned/unincluded */
  for (c=rank; len > c ; ) {
    r = mtx_row_to_org(mtx,c);
    if ( logrel_active(lrp[r])) {
      c++;
    } else {
      mtx_swap_rows(mtx,c,len);
      len--;
    }
  }

  /* sort the unincluded out of the unassigned */
  len = nrowpun - 1;  /* row of last possible unassigned/unincluded relation */
  for (c=rank; len > c ; ) {
    r = mtx_row_to_org(mtx,c);
    if ( logrel_included(lrp[r]) ) {
      c++;
    } else {
      mtx_swap_rows(mtx,c,len);
      len--;
    }
  }

#if SBPDEBUG
  fp = fopen("/tmp/sbp2.plot","w+");
  if (fp !=NULL) {
    mtx_write_region_plot(fp,mtx,mtx_ENTIRE_MATRIX);
    fclose(fp);
  }
#endif

  /* These functions are called for slv_block_partitioning, but I do not
   * know if I need them here:
   *
   * now, at last we have cols in the order we want the lists to
   * be handed to the solver. So, we need to reset the dvar sindex values
   * and reorder the lists.
   */

#if USEDCODE
   if (reindex_dvars_from_mtx(sys,0,dvlen-1,mtx)) {
     mtx_destroy(mtx);
     return 2;
   }
#endif /* USEDCODE */

  /*
   * now, at last we have rows jacobian in the order we want the lists to
   * be handed to the solvers. So, we need to reset the rel sindex values.
   */

#if USEDCODE
   if (reindex_logrels_from_mtx(sys,0,lrlen-1,mtx)) {
     mtx_destroy(mtx);
     return 2;
   }
#endif /* USEDCODE */

  return 0;
}

