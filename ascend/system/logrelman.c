/*
 *  Logical Relation Manipulator Module
 *  Created: 04/97
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: logrelman.c,v $
 *  Date last modified: $Date: 1997/07/18 12:14:40 $
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

#include "logrelman.h"

#include <math.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/general/list.h>

#include <ascend/compiler/instance_enum.h>


#include <ascend/compiler/functype.h>
#include <ascend/compiler/safe.h>
#include <ascend/compiler/extfunc.h>

#include <ascend/compiler/expr_types.h>
#include <ascend/compiler/exprs.h>
#include <ascend/compiler/find.h>
#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/mathinst.h>
#include <ascend/compiler/relation_type.h>
#include <ascend/compiler/rel_blackbox.h>
#include <ascend/compiler/vlist.h>
#include <ascend/compiler/relation.h>
#include <ascend/compiler/packages.h>
#include <ascend/compiler/extcall.h>
#include <ascend/compiler/logical_relation.h>
#include <ascend/compiler/logrelation.h>
#include <ascend/compiler/logrel_util.h>
#include <ascend/compiler/logrel_io.h>

#include "slv_server.h"

#define IPTR(i) ((struct Instance *)(i))


void logrelman_get_incidence(struct logrel_relation *lrel,
                             dis_filter_t *filter,
                             mtx_matrix_t mtx)
{
  const struct dis_discrete **list;
  mtx_coord_t nz;
  int c,len;

  assert(lrel!=NULL && filter !=NULL && mtx != NULL);
  nz.row = logrel_sindex(lrel);
  len = logrel_n_incidences(lrel);

  list = logrel_incidence_list(lrel);
  for (c=0; c < len; c++) {
     if( dis_apply_filter(list[c],filter) ) {
	nz.col = dis_sindex(list[c]);
	mtx_fill_org_value(mtx,&nz,1.0);
     }
  }
}


int32 logrelman_eval(struct logrel_relation *lrel, int32 *status)
{
  int32 res;
  assert(lrel!=NULL);

  *status = (int32) LogRelCalcResidual(logrel_instance(lrel),&res);
  if (*status) { /* an error occured */
    res = 0;
  }
  logrel_set_residual(lrel,res);
  return res;
}


boolean logrelman_calc_satisfied( struct logrel_relation *lrel)
{
   real64 res;
   res = logrel_residual(lrel);

   if( res==0) {
      logrel_set_satisfied(lrel,FALSE);
   } else {
      logrel_set_satisfied(lrel,TRUE);
   }
   return( logrel_satisfied(lrel) );
}


int32 *logrelman_directly_solve(struct logrel_relation *lrel,
                                struct dis_discrete *solvefor,
                                int *able, int *nsolns, int perturb,
				struct gl_list_t *insts)
{
  int32 *value;
  int ndvars,n;
  const struct dis_discrete **dvlist;
  unsigned long dvindex;                 /* index to the compiler */
  ndvars = logrel_n_incidences(lrel);
  dvlist = logrel_incidence_list(lrel);
  dvindex = 0;
  for (n=0;n <ndvars; n++) {
    if (dvlist[n]==solvefor) {
      dvindex = n+1;                     /*compiler counts from 1 */
      break;
    }
  }

  value = LogRelFindBoolValues(IPTR(logrel_instance(lrel)),&(dvindex),
                               able,nsolns,perturb,insts);
  return value;
}


char *logrelman_make_string_infix(slv_system_t sys,
                                  struct logrel_relation *lrel, int style)
{
   char *sbeg;

   if (style) {
     sbeg = WriteLogRelToString(logrel_instance(lrel),slv_instance(sys));
   } else {
     sbeg = ASC_NEW_ARRAY(char,60);
     if (sbeg==NULL) return sbeg;
     sprintf(sbeg,"logrelman_make_string_infix not implemented.");
   }
   return(sbeg);
}


char *logrelman_make_string_postfix(slv_system_t sys,
                                  struct logrel_relation *lrel, int style)
{
   char  *sbeg;

   if (style) {
     sbeg = WriteLogRelPostfixToString(logrel_instance(lrel),slv_instance(sys));
   } else {
     sbeg = ASC_NEW_ARRAY(char,60);
     if (sbeg==NULL) return sbeg;
     sprintf(sbeg,"logrelman_make_string_postfix not implemented.");

   }
   return(sbeg);
}
