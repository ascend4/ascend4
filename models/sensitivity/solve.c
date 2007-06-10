/*	ASCEND modelling environment
	Copyright (C) 1996-2007 Carnegie Mellon University

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

	A little EXTERNAL method for solving models. Useful for cases where
	model initialisation is a bit tricky. See also models/johnpye/extpy and
	models/johnpye/solve.py.

	Should just work:

	IMPORT "sensitivity/solve";
	:  :
	MODEL mymodel;
		:  :
	METHODS
		:  :
		METHOD mysolve;
			EXTERNAL solve(SELF);
		END mysolve;
		:  :
	END mymodel;

	Sliced out of sensitivity.c because it's independent code.
*/

#include <math.h>

#include <general/mathmacros.h>
#include <utilities/ascMalloc.h>

#include <compiler/instquery.h>
#include <compiler/atomvalue.h>
#include <compiler/extfunc.h>

#include <packages/sensitivity.h>
#include <system/system.h>
#include <solver/solver.h>

ExtMethodRun do_solve_eval;
ASC_EXPORT int solve_register(void);

/**
	Build then presolve the solve an instance...
*/
int DoSolve(struct Instance *inst){
  slv_system_t sys;
  const SlvFunctionsT *S;

  sys = system_build(inst);
  if (!sys) {
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to build system");
    return 1;
  }
  S = solver_engine_named("QRSlv");
  if(!S){
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to locate solver 'QRSlv'");
  }
  (void)slv_select_solver(sys,S->number);
  slv_presolve(sys);
  CONSOLE_DEBUG("Calling slv_solve...");
  slv_solve(sys);
  CONSOLE_DEBUG("... completed slv_solve");	
  system_destroy(sys);
  return 0;
}

/**
	Calls 'DoSolve'

	@see DoSolve
*/
int do_solve_eval( struct Instance *i,
		struct gl_list_t *arglist, void *user_data
){
  unsigned long len;
  int result;
  struct Instance *inst;
  len = gl_length(arglist);

  (void)i; /* not used */

  if (len!=1) {
	ERROR_REPORTER_HERE(ASC_USER_ERROR,"Wrong number of args in (expected 1, got %d)",len);
    return 1;
  }
  inst = FetchElement(arglist,1,1);
  if (!inst)
    return 1;
  result = DoSolve(inst);
  return result;
}


#if 0
static int ReSolve(slv_system_t sys)
{
  if (!sys)
    return 1;
  slv_solve(sys);
  return 0;
}
#endif

/** Registration function */
int solve_register(void){
	int result;
	result = CreateUserFunctionMethod("do_solve",
		do_solve_eval,
		1,NULL,NULL,NULL
	);
	return result;
}
