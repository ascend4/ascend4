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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//*
	by Karl Michael Westerberg
	Created: 2/6/90
	Last in CVS: $Revision: 1.29 $ $Date: 2003/01/19 02:16:05 $ $Author: ballan $
*/

#include "system.h"

#include <ascend/general/ascMalloc.h>
#include <ascend/general/list.h>
#include <ascend/general/tm_time.h>

#include <ascend/compiler/instance_enum.h>
#include <ascend/compiler/check.h>

#include <ascend/linear/mtx.h>

#include "slv_client.h"
#include "diffvars.h"

#include "relman.h"
#include "slv_server.h"
#include "analyze.h"
#include "slv_common.h"

//#define ASC_SYSTEM_DEBUG
#ifdef ASC_SYSTEM_DEBUG
# define DOTIME 1
# define MSG CONSOLE_DEBUG
#else
# define MSG(...) 
# define DOTIME 0
#endif

#define IPTR(i) ((struct Instance *) (i))

slv_system_t system_build(SlvBackendToken inst){
  slv_system_t sys;
  int stat;

#if DOTIME
  double comptime;
  comptime = tm_cpu_time();
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
  if(stat){
    system_destroy(sys);
    sys = NULL;
    if(stat==2) {
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
    return sys;
  }

  slv_set_instance(sys,inst);

#if DOTIME
  comptime = tm_cpu_time() - comptime;
  if(comptime >= 2){
    MSG("System built (time %5.3f s)",comptime);
  }else if(comptime > 10e-3){
    MSG("System built (time %5.3f ms)",comptime*1e3);
  }else{
    MSG("System built (time %0.0f us)",comptime*1e6);
  }
#endif
  return(sys);
}

void system_destroy(slv_system_t sys){
	struct gl_list_t *symbollist;
	void *l;

#define FN(FUNCNAME) \
		l=(void*)FUNCNAME(sys); if(l!=NULL)ASC_FREE(l);
#define F(N) FN(slv_get_master_##N##_list)
	F(var); F(par); F(unattached); F(dvar); F(disunatt); F(rel);
	F(condrel); F(obj); F(logrel); F(condlogrel); F(when); F(bnd);
#undef F

#define F(N) FN(slv_get_solvers_##N##_list)
	F(var); F(par); F(unattached); F(dvar); F(disunatt); F(rel);
	F(condrel); F(obj); F(logrel); F(condlogrel); F(when); F(bnd);
#undef F
#undef FN

	system_diffvars_destroy(sys);

	symbollist=slv_get_symbol_list(sys);
	if(symbollist != NULL)DestroySymbolValuesList(symbollist);

	slv_set_solvers_blocks(sys,0,NULL);
	slv_set_solvers_log_blocks(sys,0,NULL);	/* free blocks lists */
	slv_destroy(sys); /* frees buf data */
}

void system_free_reused_mem(){
  mtx_free_reused_mem();
  linsolqr_free_reused_mem();
  analyze_free_reused_mem();
  relman_free_reused_mem();
}

