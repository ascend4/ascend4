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
#include <ascend/compiler/instmacro.h>
#include <ascend/integrator/integrator.h>
#include <ascend/compiler/slvreq.h>

#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>
#include <ascend/solver/solver.h>
#include <ascend/system/slv_server.h>

#include <test/common.h>
#include <test/assertimpl.h>

typedef struct SlvReqC_struct{
	struct Instance *siminst;
	slv_system_t sys;
	SampleList * slist;
	IntegratorSystem *isys;
} SlvReqC;

SlvReqSetSolverFn slvreq_c_set_solver_integrate;
SlvReqSetOptionFn slvreq_c_set_option_integrate;
SlvReqDoSolveFn slvreq_c_do_solve_integrate;

/* a simple integrator reporter for testing */
int test_ida_reporter_init_integrate(struct IntegratorSystemStruct *integ) {
	return 0;
}

int test_ida_reporter_write_integrate(struct IntegratorSystemStruct *integ) {
	double val;
	val = var_value(integ->y[0]);
	CONSOLE_DEBUG("y[0] = %g", val);
	return 1;
}

int test_ida_reporter_writeobs_integrate(struct IntegratorSystemStruct *integ) {
	CONSOLE_DEBUG("x = %f", var_value(integ->x));
	return 0;
}

int test_ida_reporter_close_integrate(struct IntegratorSystemStruct *integ) {
	return 0;
}

IntegratorReporter test_ida_reporter_integrate = { test_ida_reporter_init_integrate,
		test_ida_reporter_write_integrate, test_ida_reporter_writeobs_integrate,
		test_ida_reporter_close_integrate };

int slvreq_c_set_integrator(const char *intgratorname, double from,double to,double steps,void *user_data){
	SlvReqC *S = (SlvReqC *)user_data;
	struct Instance *siminst = S->siminst;

	slv_system_t sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);
	S->sys = sys;

	IntegratorSystem *integ = integrator_new(sys,GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(integ != NULL);
	S->isys = integ;
	
	CU_ASSERT_FATAL(0 == integrator_set_engine(integ,intgratorname));

	CONSOLE_DEBUG("METHOD-ASSIGNED INTEGRATOR '%s'...",integ->internals->name);

	/* TODO assign an integrator reporter */
	integrator_set_reporter(integ, &test_ida_reporter_integrate);
	/*these settings should be setted by language??*/
	integrator_set_minstep(integ,0.001);
	integrator_set_maxstep(integ,1000);
	integrator_set_stepzero(integ,1);
	integrator_set_maxsubsteps(integ,100);

	/* set a linearly-distributed samplelist */
	double start = from, end = to;
	int num = (int)steps;
	dim_type d;
	SetDimFraction(d,D_TIME,CreateFraction(1,1));
	SampleList *samplelist = samplelist_new(num+1, &d);
	double val = start;
	double inc = (end-start)/(num);
	unsigned long i;

	S->slist = samplelist;
	for(i=0; i<=num; ++i){
		samplelist_set(samplelist,i,val);
		val += inc;
	}
	integrator_set_samples(integ,samplelist);

	return 0;
}
int slvreq_c_set_sub_solver(const char *solvername, void *user_data){
	SlvReqC *S = (SlvReqC *)user_data;
	//set inner solver
	int index = slv_lookup_client(solvername);
	CU_ASSERT_FATAL(index >= 0);
	slv_select_solver(S->sys,index);

	return 0;
}
/*
	This function actually does the job of setting the solver in our little
	'test' simulation environment.
*/
int slvreq_c_set_solver_integrate(const char *solvername, void *user_data){
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
int slvreq_c_set_option_integrate(const char *optionname, struct value_t *val, void *user_data){
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

int slvreq_c_do_solve_integrate(struct Instance *instance, void *user_data){
	SlvReqC *S = (SlvReqC *)user_data;
	int res = 0;
	if(S->sys==NULL) return SLVREQ_NO_SOLVER_SELECTED;

	/* perform problem analysis */
	CU_ASSERT_FATAL(0 == integrator_analyse(S->isys));

	CONSOLE_DEBUG("integrator_analyse ok");

	if(0 == integrator_solve(S->isys, 0, samplelist_length(S->slist)-1))
		CONSOLE_DEBUG("integrator goes well!");
	else {
		CONSOLE_DEBUG("integrator fail!");
		res = 1;
	}

	if(S->isys)
		integrator_free(S->isys);
	if(S->slist)
		samplelist_free(S->slist);

	if(res) return SLVREQ_PRESOLVE_FAIL;

	return 0;
}

static void test_boundary(void){
	/* set paths relative to test executable */
	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/ida" OSPATH_DIV "solvers/lrslv" OSPATH_DIV "solvers/lsode" OSPATH_DIV "solvers/qrslv");
	CU_ASSERT_FATAL(0 == package_load("qrslv",NULL));

	/* load the file */
	char path[PATH_MAX];
	strcpy((char *) path, "test/slvreq/shaun/");
#define FILESTEM "bouncingball"
	strncat(path, FILESTEM, PATH_MAX - strlen(path));
	strncat(path, ".a4c", PATH_MAX - strlen(path));

	int status;
	Asc_OpenModule(path, &status);
	CU_ASSERT(status == 0);

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* instantiate it */
	SlvReqC S;
	S.siminst = SimsCreateInstance(AddSymbol(FILESTEM),
			AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT(S.siminst!=NULL);
#undef FILESTEM
	/* do the solver hooks */
	S.sys = NULL;
	slvreq_assign_hooks_ex(S.siminst, &slvreq_c_set_solver_integrate, 
				&slvreq_c_set_option_integrate, 
				&slvreq_c_do_solve_integrate,
				&slvreq_c_set_integrator,
				&slvreq_c_set_sub_solver, &S);

        CONSOLE_DEBUG("RUNNING ON_LOAD");

	/** Call on_load */
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(S.siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);

	CU_ASSERT(pe==Proc_all_ok);

	/* all sorts of destruction */
	CONSOLE_DEBUG("DESTROYING NOW...");

	system_destroy(S.sys);
	system_free_reused_mem();

	/* destroy all that stuff */
	CONSOLE_DEBUG("Destroying instance tree");

	solver_destroy_engines();
	integrator_free_engines();
	sim_destroy(S.siminst);
	Asc_CompilerDestroy();
}

static void test_shm(void){
	/* set paths relative to test executable */
	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/ida" OSPATH_DIV "solvers/lrslv" OSPATH_DIV "solvers/lsode" OSPATH_DIV "solvers/qrslv");
	CU_ASSERT_FATAL(0 == package_load("qrslv",NULL));

	/* load the file */
	char path[PATH_MAX];
	strcpy((char *) path, "test/slvreq/shaun/");
#define FILESTEM "shm"
	strncat(path, FILESTEM, PATH_MAX - strlen(path));
	strncat(path, ".a4c", PATH_MAX - strlen(path));

	int status;
	Asc_OpenModule(path, &status);
	CU_ASSERT(status == 0);

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* instantiate it */
	SlvReqC S;
	S.siminst = SimsCreateInstance(AddSymbol(FILESTEM),
			AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT(S.siminst!=NULL);
#undef FILESTEM
	/* do the solver hooks */
	S.sys = NULL;
	slvreq_assign_hooks_ex(S.siminst, &slvreq_c_set_solver_integrate, 
				&slvreq_c_set_option_integrate, 
				&slvreq_c_do_solve_integrate,
				&slvreq_c_set_integrator,
				&slvreq_c_set_sub_solver, &S);

        CONSOLE_DEBUG("RUNNING ON_LOAD");

	/** Call on_load */
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(S.siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);

	CU_ASSERT(pe==Proc_all_ok);

	/* all sorts of destruction */
	CONSOLE_DEBUG("DESTROYING NOW...");

	system_destroy(S.sys);
	system_free_reused_mem();

	/* destroy all that stuff */
	CONSOLE_DEBUG("Destroying instance tree");

	solver_destroy_engines();
	integrator_free_engines();
	sim_destroy(S.siminst);
	Asc_CompilerDestroy();
}

/*===========================================================================*/
/* Registration information */

/* the list of tests */
#define TESTS(T) \
	T(boundary) \
	T(shm)

REGISTER_TESTS_SIMPLE(compiler_integrate, TESTS)
