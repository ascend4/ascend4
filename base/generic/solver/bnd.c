/*
 *  Boundary Module
 *  Created: 04/97
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: bnd.c,v $
 *  Date last modified: $Date: 1997/07/18 12:13:54 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the SLV solver.
 *
 *  The SLV solver is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The SLV solver is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 *
 */

#include <math.h>
#include <utilities/ascConfig.h>
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/dstring.h>
#include <compiler/compiler.h>
#include <compiler/fractions.h>
#include <compiler/instance_enum.h>
#include <compiler/extfunc.h>
#include <compiler/extcall.h>
#include <compiler/functype.h>
#include <compiler/safe.h>
#include <compiler/dimen.h>
#include <compiler/expr_types.h>
#include <compiler/find.h>
#include <compiler/atomvalue.h>
#include <compiler/instquery.h>
#include <compiler/mathinst.h>
#include <compiler/parentchild.h>
#include <compiler/instance_io.h>
#define _SLV_SERVER_C_SEEN_
#include <utilities/mem.h>
#include "mtx.h"
#include "slv_types.h"
#include "var.h"
#include "rel.h"
#include "discrete.h"
#include "conditional.h"
#include "logrel.h"
#include "bnd.h"
#include "slv_server.h"

#define IPTR(i) ((struct Instance *)(i))
#ifndef DEFTOLERANCE
#define DEFTOLERANCE 1e-08
#endif /* DEFTOLERANCE */


struct bnd_boundary *bnd_create(struct bnd_boundary *newbnd)
{

  if (newbnd==NULL) {
    newbnd = ASC_NEW(struct bnd_boundary);
    assert(newbnd!=NULL);
    memset((char *)&(newbnd->cond),0,sizeof(union bnd_union));
    newbnd->kind = e_bnd_undefined;
    newbnd->logrels = NULL;
    newbnd->tolerance = DEFTOLERANCE;
    newbnd->mindex = -1;
    newbnd->sindex = -1;
    newbnd->model = -1;
    newbnd->flags = BND_IN_LOGREL;
  } else {
    memset((char *)&(newbnd->cond),0,sizeof(union bnd_union));
    newbnd->kind = e_bnd_undefined;
    newbnd->logrels = NULL;
    newbnd->tolerance = DEFTOLERANCE;
    newbnd->mindex = -1;
    newbnd->sindex = -1;
    newbnd->model = -1;
    newbnd->flags = BND_IN_LOGREL;
  }
  return(newbnd);
}


void bnd_destroy(struct bnd_boundary *bnd)
{
   if (bnd==NULL) return;
   if (bnd->logrels != NULL) {
     gl_destroy(bnd->logrels);
     bnd->logrels = NULL;
   }
}


enum bnd_enum bnd_kind(struct bnd_boundary *bnd)
{
  asc_assert(bnd!=NULL);
  return bnd->kind;
}


void bnd_set_kind(struct bnd_boundary *bnd, enum bnd_enum kind)
{
  asc_assert(bnd!=NULL);
  bnd->kind = kind;
}


void bnd_set_logrels(struct bnd_boundary *bnd,
                            struct gl_list_t *logrels)
{
  asc_assert(bnd!=NULL);
  bnd->logrels = logrels;
}


struct gl_list_t *bnd_logrels(struct bnd_boundary *bnd)
{
  asc_assert(bnd!=NULL);
  return bnd->logrels;
}


void bnd_set_tolerance(struct bnd_boundary *bnd,real64 tolerance)
{
  asc_assert(bnd!=NULL);
  bnd->tolerance = tolerance;
}


real64 bnd_tolerance(struct bnd_boundary *bnd)
{
  asc_assert(bnd!=NULL);
  return bnd->tolerance;
}


char *bnd_make_name(slv_system_t sys,struct bnd_boundary *bnd)
{
  enum bnd_enum kind;
  struct rel_relation *rel;
  struct logrel_relation *lrel;

  if ((NULL == sys) || (NULL == bnd)) {
    return NULL;
  }
  kind = bnd_kind(bnd);
  if (kind == e_bnd_rel) {
    rel = bnd_rel(bnd_real_cond(bnd));
    return WriteInstanceNameString(IPTR(rel_instance(rel)),
                                   IPTR(slv_instance(sys)));
  } else {
    lrel = bnd_logrel(bnd_log_cond(bnd));
    return WriteInstanceNameString(IPTR(logrel_instance(lrel)),
                                   IPTR(slv_instance(sys)));
  }
}


int32 bnd_mindex( struct bnd_boundary *bnd)
{
   asc_assert(bnd!=NULL);
   return( bnd->mindex );
}


void bnd_set_mindex( struct bnd_boundary *bnd, int32 index)
{
   asc_assert(bnd!=NULL);
   bnd->mindex = index;
}


int32 bnd_sindex( const struct bnd_boundary *bnd)
{
   asc_assert(bnd!=NULL);
   return( bnd->sindex );
}


void bnd_set_sindex( struct bnd_boundary *bnd, int32 index)
{
   asc_assert(bnd!=NULL);
   bnd->sindex = index;
}


int32 bnd_model(const struct bnd_boundary *bnd)
{
   asc_assert(bnd!=NULL);
   return((const int32) bnd->model );
}


void bnd_set_model( struct bnd_boundary *bnd, int32 index)
{
   asc_assert(bnd!=NULL);
   bnd->model = index;
}


struct var_variable **bnd_real_incidence(struct bnd_boundary *bnd)
{
  enum bnd_enum kind;
  struct var_variable **incidence;
  struct rel_relation *rel;
  kind = bnd_kind(bnd);     /* check for bnd==NULL done in bnd_kind() */
  if (kind == e_bnd_rel) {
    rel = bnd_rel(bnd_real_cond(bnd));
    incidence = rel_incidence_list_to_modify(rel);
    return incidence;
  } else {
    FPRINTF(stderr,"bnd_real_incidence called with incorrect boundary\n");
    return NULL;
  }
}


int32 bnd_n_real_incidences(struct bnd_boundary *bnd)
{
  enum bnd_enum kind;
  int32 n_incidences;
  struct rel_relation *rel;
  kind = bnd_kind(bnd);     /* check for bnd==NULL done in bnd_kind() */
  if (kind == e_bnd_rel) {
    rel = bnd_rel(bnd_real_cond(bnd));
    n_incidences = rel_n_incidences(rel);
    return n_incidences;
  } else {
    FPRINTF(stderr,"bnd_n_real_incidences called with incorrect boundary\n");
    return 0;
  }
}


int32 bnd_apply_filter( const struct bnd_boundary *bnd, bnd_filter_t *filter)
{
  if (bnd==NULL || filter==NULL) {
    FPRINTF(stderr,"bnd_apply_filter miscalled with NULL\n");
    return FALSE;
  }
  return ( (filter->matchbits & bnd->flags) ==
           (filter->matchbits & filter->matchvalue)  );
}


uint32 bnd_flags( struct bnd_boundary *bnd)
{
  asc_assert(bnd!=NULL);
  return bnd->flags;
}


void bnd_set_flags(struct bnd_boundary *bnd, unsigned int flags)
{
  asc_assert(bnd!=NULL);
  bnd->flags = flags;
}


uint32 bnd_flagbit(struct bnd_boundary *bnd, uint32 one)
{
  asc_assert(bnd!=NULL);
  return (bnd->flags & one);
}


void bnd_set_flagbit(struct bnd_boundary *bnd, uint32 field,uint32 one)
{
  asc_assert(bnd!=NULL);
  if (one) {
    bnd->flags |= field;
  } else {
    bnd->flags &= ~field;
  }
}


int32 bnd_status_cur( struct bnd_boundary *bnd)
{
  asc_assert(bnd!=NULL);
  if (bnd_cur_status(bnd)) {
    return 1;
  } else {
    return 0;
  }
}


int32 bnd_status_pre( struct bnd_boundary *bnd)
{
  asc_assert(bnd!=NULL);
  if (bnd_pre_status(bnd)) {
    return 1;
  } else {
    return 0;
  }
}
