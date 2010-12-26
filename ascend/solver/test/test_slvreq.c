#include <CUnit/CUnit.h>

#include "test_slvreq.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <ascend/general/env.h>
#include <ascend/general/ospath.h>
#include <ascend/general/list.h>
#include <ascend/general/ltmatrix.h>

#include <ascend/general/platform.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/error.h>

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/relation_io.h>
#include <ascend/compiler/reverse_ad.h>
#include <ascend/compiler/relation_util.h>
#include <ascend/compiler/mathinst.h>
#include <ascend/compiler/watchpt.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/visitinst.h>
#include <ascend/compiler/functype.h>
#include <ascend/compiler/safe.h>
#include <ascend/compiler/qlfdid.h>
#include <ascend/compiler/instance_io.h>

#include <ascend/compiler/slvreq.h>

#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>
#include <ascend/solver/solver.h>
#include <ascend/system/slv_server.h>

typedef struct SlvReqC_struct{
	struct Instance *siminst;
	slv_system_t sys;
} SlvReqC;

SlvReqSetSolverFn slvreq_c_set_solver;
SlvReqSetOptionFn slvreq_c_set_option;
SlvReqDoSolveFn slvreq_c_do_solve;

/*
	This function actually does the job of setting the solver in our little
	'test' simulation environment.
*/
int slvreq_c_set_solver(const char *solvername, void *user_data){
	SlvReqC *S = (SlvReqC *)user_data;
	int index = slv_lookup_client(solvername);
	if(index == -1){
		CONSOLE_DEBUG("Failed to locate solve '%s'",solvername);
		return 1;
	}

	if(S->sys == NULL){
		CONSOLE_DEBUG("Building system...");
		S->sys = system_build(GetSimulationRoot(S->siminst));
	}

	if(slv_select_solver(S->sys,index) == -1){
		CONSOLE_DEBUG("Failed to select solver '%s' (solver was found, though)",solvername);
		return 2;
	}
	CONSOLE_DEBUG("Assigned solver '%s'...",solvername);
	return 0;
}

/*
	This function actually does the job of setting solver options in our little
	'test' simulation environment.
*/
int slvreq_c_set_option(const char *optionname, struct value_t *val, void *user_data){
	SlvReqC *S = (SlvReqC *)user_data;
	slv_parameters_t pp;
	if(S->sys == NULL){
		/* no solver has been assigned so the system isn't built*/
		return SLVREQ_OPTIONS_UNAVAILABLE;
	}
	slv_get_parameters(S->sys, &pp);
	/* look for the parameter by name */
	int i, index = -1;
	for(i=0;i<pp.num_parms;++i){
		if(strcmp(pp.parms[i].name,optionname)==0){
			index = i;
		}
	}
	if(index == -1){
		/* not found */
		return SLVREQ_INVALID_OPTION_NAME;
	}
	/* parameter has been found */
	switch(SLV_PARAM_TYPE(&pp,index)){
	case int_parm:
		if(ValueKind(*val)!=integer_value){
			CONSOLE_DEBUG("Wrong parameter type: expecting integer type");
			return SLVREQ_WRONG_OPTION_VALUE_TYPE;
		}
		CONSOLE_DEBUG("Setting %s = %ld...",optionname, IntegerValue(*val));
		SLV_PARAM_INT(&pp,index) = IntegerValue(*val);
		break;
	case bool_parm: 
		if(ValueKind(*val)!=boolean_value){
			CONSOLE_DEBUG("Wrong parameter type: expecting boolean type");
			return SLVREQ_WRONG_OPTION_VALUE_TYPE;
		}
		CONSOLE_DEBUG("Setting %s = %s...",optionname, BooleanValue(*val)?"TRUE":"FALSE");
		SLV_PARAM_BOOL(&pp,index) = BooleanValue(*val);
		break;
	case real_parm:
		if(ValueKind(*val)!=real_value){
			CONSOLE_DEBUG("Wrong parameter type: expecting real type");
			return SLVREQ_WRONG_OPTION_VALUE_TYPE;
		}
		CONSOLE_DEBUG("Setting %s = %f...",optionname, RealValue(*val));
		SLV_PARAM_REAL(&pp,index) = RealValue(*val);
		break;
	case char_parm:
		if(ValueKind(*val)!=symbol_value){
			CONSOLE_DEBUG("Wrong parameter type: expecting string (i.e. symbol) type");
			return SLVREQ_WRONG_OPTION_VALUE_TYPE;
		}
		CONSOLE_DEBUG("Setting %s = '%s'...",optionname, SCP(SymbolValue(*val)));
		slv_set_char_parameter(&(SLV_PARAM_CHAR(&pp,index)),SCP(SymbolValue(*val)));
		break;
	}

	slv_set_parameters(S->sys, &pp);

	return 0;
}

/*
	This function actually does the job of running solver in our little
	'test' simulation environment, and reporting the result.
*/
int slvreq_c_do_solve(struct Instance *instance, void *user_data){
	SlvReqC *S = (SlvReqC *)user_data;
	int res;
	if(S->sys==NULL)return SLVREQ_NO_SOLVER_SELECTED;
	
	res = slv_presolve(S->sys);
	if(res)return SLVREQ_PRESOLVE_FAIL;

	slv_status_t status;

	slv_get_status(S->sys, &status);
	if(status.ready_to_solve){
		res = slv_solve(S->sys);
		if(res){
			CONSOLE_DEBUG("slv_solve returned %d",res);
		}
	}

	slv_get_status(S->sys, &status);
	if(status.ok){
		CONSOLE_DEBUG("Solver completed OK");
		return 0;
	}
	
	if(status.diverged)CONSOLE_DEBUG("Solver diverged");
	if(status.inconsistent)CONSOLE_DEBUG("System is inconsistent");
	if(status.iteration_limit_exceeded)CONSOLE_DEBUG("Solver exceeded iteration limit");
	if(status.calc_ok != 0)CONSOLE_DEBUG("Solver had residual calculation errors");
	if(status.over_defined)CONSOLE_DEBUG("Solver system is over-defined");
	if(status.under_defined)CONSOLE_DEBUG("Solver system is under-defined");
	if(status.iteration_limit_exceeded)CONSOLE_DEBUG("Solver exceeded time limit");

	return SLVREQ_SOLVE_FAIL;
}

/*
	Test that the slvreq mechanism works within the C layer
*/
static void test_slvreq_c(void){

	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models:solvers/conopt:solvers/qrslv:solvers/cmslv:solvers/ida:solvers/lsode:solvers/ipopt");
	
	/* load the file */
#define TESTFILE "test2"
	m = Asc_OpenModule("test/slvreq/" TESTFILE ".a4c",&status);
	CU_ASSERT(status == 0);

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */	
	CU_ASSERT(FindType(AddSymbol(TESTFILE))!=NULL);

	/* instantiate it */
	SlvReqC S;
	S.siminst = SimsCreateInstance(AddSymbol(TESTFILE), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(S.siminst!=NULL);
#undef TESTFILE

	/* do the solver hooks */
	S.sys = NULL;
	slvreq_assign_hooks(S.siminst, &slvreq_c_set_solver, &slvreq_c_set_option, &slvreq_c_do_solve, &S);

    CONSOLE_DEBUG("RUNNING ON_LOAD");

	/** Call on_load */
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(S.siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	/* maybe if the model is really clever, it will have solved itself now! */

	/* all sorts of destruction */
	CONSOLE_DEBUG("DESTROYING NOW...");
	CU_ASSERT(S.siminst)
	if(S.sys)system_destroy(S.sys);

	system_free_reused_mem();
	sim_destroy(S.siminst);
	solver_destroy_engines();
	Asc_CompilerDestroy();
}



/*===========================================================================*/
/* Registration information */

static CU_TestInfo slvreq_test_list[] = {
  {"slvreq_c", test_slvreq_c},
  CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
  {"solver_slvreq", NULL, NULL, slvreq_test_list},
  CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_solver_slvreq(void)
{
  return CU_register_suites(suites);
}
