/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
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
*//*
	by Karl Michael Westerberg
	Created: 2/6/90
	Last in CVS: $Revision: 1.29 $ $Date: 2003/01/19 02:16:05 $ $Author: ballan $
*/

#include <utilities/ascConfig.h>
#include <compiler/instance_enum.h>
#include <compiler/compiler.h>
#include <compiler/check.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/tm_time.h>
#include "mtx.h"
#include "slv_types.h"
#include "var.h"
#include "discrete.h"
#include "conditional.h"
#include "linsolqr.h"
#define _SLV_SERVER_C_SEEN_
#include <compiler/extcall.h>
#include "rel.h"
#include "relman.h"
#include "logrel.h"
#include "bnd.h"
#include "slv_server.h"
#include "system.h"
#include "analyze.h"
#include "linsol.h"
#include "slv_common.h"
#include "slv_client.h"

#define IPTR(i) ((struct Instance *) (i))
#define DOTIME 1

slv_system_t system_build(SlvBackendToken inst)
{
  slv_system_t sys;
  int stat, i;
  struct ExtRelCache **ep;

#if DOTIME
  double time;
  time = tm_cpu_time();
#endif

  sys = slv_create();

  if (set_solver_types()) {
    system_destroy(sys);
    sys = NULL;
    return sys;
  }
  /* THIS MEANS I NEED THE boolean_var DEFINITION IN system.c  */
  if (set_boolean_types()){
    system_destroy(sys);
    sys = NULL;
    return sys;
  }
  stat = analyze_make_problem(sys,IPTR(inst));
  if (!stat) {
    slv_set_instance(sys,inst);
  } else {
    system_destroy(sys);
    sys = NULL;
    if (stat==2) {
	  ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
      FPRINTF(ASCERR,"Models sent to solver: \n");
      FPRINTF(ASCERR,"1 cannot have any pending parts\n");
      FPRINTF(ASCERR,"2 cannot have NULL or unfinished relations.\n");
      FPRINTF(ASCERR,"3 must have at least one variable.\n");
      FPRINTF(ASCERR,"4 must have at least one objective or relation.\n");
      FPRINTF(ASCERR,"5 must have at all WHEN-controlling values initialized.\n");
      FPRINTF(ASCERR,"Check pendings and problem structure.\n");
	  error_reporter_end_flush();
    }
  }

  /* perform the 'presolve' on the external relations, whatever that means */
  if( (ep=slv_get_extrel_list(sys))!=NULL ) {
    for( i = 0; ep[i]!=NULL; i++ ) {
      ExtRel_PreSolve(ep[i],FALSE);		/* allow them to cleanup */
    }
  }

#if DOTIME
  FPRINTF(stderr,"Time to build system = %g\n", (tm_cpu_time() - time));
#endif
  return(sys);
}

void system_destroy(slv_system_t sys)
{
   int i;
   struct var_variable **vp, **pp, **up;
   struct dis_discrete **dp, **udp;
   struct rel_relation **rp, **crp;
   struct rel_relation **op;
   struct logrel_relation **lp, **clp;
   struct bnd_boundary **bp;
   struct w_when **wp;
   struct ExtRelCache **ep;
   struct gl_list_t *symbollist;

   if((vp=slv_get_master_var_list(sys))!=NULL        )ascfree(vp);
   if((pp=slv_get_master_par_list(sys))!=NULL        )ascfree(pp);
   if((up=slv_get_master_unattached_list(sys))!=NULL )ascfree(up);
   if((dp=slv_get_master_dvar_list(sys))!=NULL       )ascfree(dp);
   if((udp=slv_get_master_disunatt_list(sys))!=NULL  )ascfree(udp);
   if((rp=slv_get_master_rel_list(sys))!=NULL        )ascfree(rp);
   if((crp=slv_get_master_condrel_list(sys))!=NULL   )ascfree(crp);
   if((op=slv_get_master_obj_list(sys))!=NULL        )ascfree(op);
   if((lp=slv_get_master_logrel_list(sys))!=NULL     )ascfree(lp);
   if((clp=slv_get_master_condlogrel_list(sys))!=NULL)ascfree(clp);
   if((wp=slv_get_master_when_list(sys))!=NULL       )ascfree(wp);
   if((bp=slv_get_master_bnd_list(sys))!=NULL        )ascfree(bp);
   if((vp=slv_get_solvers_var_list(sys))!=NULL       )ascfree(vp);
   if((pp=slv_get_solvers_par_list(sys))!=NULL       )ascfree(pp);
   if((up=slv_get_solvers_unattached_list(sys))!=NULL)ascfree(up);
   if((dp=slv_get_solvers_dvar_list(sys))!=NULL      )ascfree(dp);
   if((udp=slv_get_solvers_disunatt_list(sys))!=NULL )ascfree(udp);
   if((rp=slv_get_solvers_rel_list(sys))!=NULL       )ascfree(rp);
   if((crp=slv_get_solvers_condrel_list(sys))!=NULL  )ascfree(crp);
   if((op=slv_get_solvers_obj_list(sys))!=NULL       )ascfree(op);
   if((lp=slv_get_solvers_logrel_list(sys))!=NULL    )ascfree(lp);
   if((clp=slv_get_solvers_condlogrel_list(sys))!=NULL)ascfree(clp);
   if((wp=slv_get_solvers_when_list(sys))!=NULL      )ascfree(wp);
   if((bp=slv_get_solvers_bnd_list(sys))!=NULL       )ascfree(bp);

  symbollist=slv_get_symbol_list(sys);
  if(symbollist != NULL) {
     DestroySymbolValuesList(symbollist);
   }

   if( (ep=slv_get_extrel_list(sys))!=NULL ) {	/* extrels */
      for( i = 0; ep[i]; i++ ) {
	ExtRel_PreSolve(ep[i],FALSE);		/* allow them to cleanup */
	ExtRel_DestroyCache(ep[i]);
      }
      ascfree(ep);
   }
   slv_set_solvers_blocks(sys,0,NULL);
   slv_set_solvers_log_blocks(sys,0,NULL);	/* free blocks lists */
   slv_destroy(sys);				/* frees buf data */
}

void system_free_reused_mem()
{
  mtx_free_reused_mem();
  linsolqr_free_reused_mem();
  analyze_free_reused_mem();
  relman_free_reused_mem();
}

