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
*//*
	@file
	block partitioning for logic problems. Extracted from slv_stdcalls.*
*//*
	created John Pye 2007 from slv_stdcalls.h, orig by Ben Allan 1996.
*/

#include "logblock.h"

#include <utilities/ascMalloc.h>
#include <general/mathmacros.h>
#include <linear/mtx_basic.h>
#include <system/slv_stdcalls.h>
#include <system/logrelman.h>

#include "solver.h"

#define SLBPDEBUG 0 /* slv_log_block_partition debugging */

/*
 * returns 0 if ok, OTHERWISE if madness detected.
 */
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

  if (slv_make_log_incidence_mtx(sys,mtx,&dvf,&lrf)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"failure in creating incidence matrix.");
    mtx_destroy(mtx);
    return 1;
  }

  mtx_output_assign(mtx,lrlen,dvlen);
  rank = mtx_symbolic_rank(mtx);
  if (rank == 0 ) return 1;
  if (rank < nrow) {
    ERROR_REPORTER_HERE(ASC_PROG_WARNING,"rank < nrow. Dependent logical relations?");
  }
  if (rank < ncol) {
    if ( nrow != rank) {
	  ERROR_REPORTER_HERE(ASC_PROG_WARNING,"rank < ncol and nrow != rank. Excess of columns?");
    } else {
      ERROR_REPORTER_HERE(ASC_PROG_WARNING,"rank<ncol but nrow==rank. Degrees of freedom?");
    }
  }
  if (ncol == nrow) {
    if (ncol == rank) {
      ERROR_REPORTER_HERE(ASC_PROG_NOTE,"System of logical relations does not need Logic Inference.\n");
    }
    if (ncol != rank) {
      ERROR_REPORTER_HERE(ASC_PROG_WARNING,"but ncol!=rank. Rank deficient?");
    }
  }
  mtx_partition(mtx);
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

#if SLBPDEBUG
  fp = fopen("/tmp/sbp2.plot","w+");
  if(fp !=NULL){
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

