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
#include <ascend/compiler/packages.h>

#include <ascend/compiler/slvreq.h>

#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>
#include <ascend/system/slv_param.h>
#include <ascend/system/discrete.h>
#include <ascend/solver/logblock.h>
#include <ascend/solver/solver.h>
#include <ascend/system/slv_server.h>

#include <test/common.h>

static int lrslv_find_param_index(const slv_parameters_t *pp, const char *name){
	int i;
	for(i = 0; i < pp->num_parms; ++i){
		if(pp->parms[i].name != NULL && 0 == strcmp(pp->parms[i].name,name)){
			return i;
		}
	}
	return -1;
}

static int lrslv_set_bool_param(slv_system_t sys, const char *name, int value){
	slv_parameters_t params;
	int idx;
	slv_get_parameters(sys,&params);
	idx = lrslv_find_param_index(&params,name);
	if(idx < 0 || SLV_PARAM_TYPE(&params,idx) != bool_parm){
		return 1;
	}
	SLV_PARAM_BOOL(&params,idx) = value ? 1 : 0;
	slv_set_parameters(sys,&params);
	return 0;
}

static struct dis_discrete *lrslv_find_dvar_by_instance(
		slv_system_t sys, struct Instance *inst
){
	struct dis_discrete **dvars;
	int i, ndvars;
	dvars = slv_get_solvers_dvar_list(sys);
	ndvars = slv_get_num_solvers_dvars(sys);
	for(i = 0; i < ndvars; ++i){
		if(dis_instance(dvars[i]) == inst){
			return dvars[i];
		}
	}
	return NULL;
}

static void test_boundaries(){
	//struct module_t *m;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models" OSPATH_DIV "solvers/lrslv");
	if(package_load("lrslv",NULL)){
		Asc_CompilerDestroy();
		CU_FAIL("Unable to load solver 'lrslv'");
		return;
	}

	/* load the file */
	char path[PATH_MAX] = "test/ida/boundaries.a4c";
	{
		int status;
		/* (DISUSED) m = */(void)Asc_OpenModule(path,&status);
		CU_ASSERT(status == 0);
	}

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* instantiate it */
	struct Instance *sim = SimsCreateInstance(AddSymbol("boundaries"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

    /** Call on_load */
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(sim),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	/* assign solver */
	const char *solvername = "LRSlv";
	int index = slv_lookup_client(solvername);
	CU_ASSERT_FATAL(index != -1);

	slv_system_t sys = system_build(GetSimulationRoot(sim));
	CU_ASSERT_FATAL(sys != NULL);

	CU_ASSERT_FATAL(slv_select_solver(sys,index));
	CONSOLE_DEBUG("Assigned solver '%s'...",solvername);

	CU_ASSERT_FATAL(0 == slv_presolve(sys));

	slv_status_t status;
	slv_get_status(sys, &status);
	CU_ASSERT_FATAL(status.ready_to_solve);

	slv_solve(sys);

	slv_get_status(sys, &status);
	CU_ASSERT(status.ok);

	/** Call check_satbefore */
	name = CreateIdName(AddSymbol("check_satbefore"));
	pe = Initialize(GetSimulationRoot(sim),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	/* Set t := 4 {s} */
	struct Instance *inst;
	CU_ASSERT((inst = ChildByChar(GetSimulationRoot(sim),AddSymbol("t"))) && InstanceKind(inst)==REAL_ATOM_INST);
	SetRealAtomValue(inst, 4.0, 0);
	CU_ASSERT(RealAtomValue(inst)==4.0);

	/* re-solve */
	CONSOLE_DEBUG("Attempting to re-solve model...");
	CU_ASSERT_FATAL(0 == slv_presolve(sys));
	slv_get_status(sys, &status);
	CU_ASSERT_FATAL(status.ready_to_solve);
	slv_solve(sys);
	slv_get_status(sys, &status);
	CU_ASSERT(status.ok);

	CONSOLE_DEBUG("t = %f", RealAtomValue(inst));

	/** Call check_satduring */
	name = CreateIdName(AddSymbol("check_satduring"));
	pe = Initialize(GetSimulationRoot(sim),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	/* all sorts of destruction */
	CONSOLE_DEBUG("Destroying system...");
	if(sys)system_destroy(sys);
	system_free_reused_mem();
	CU_ASSERT(sim != NULL);
	sim_destroy(sim);
	solver_destroy_engines();
	Asc_CompilerDestroy();
	CONSOLE_DEBUG("all gone!");
}

static void test_external_blocks(){
	char env1[2*PATH_MAX];
	int status;
	int lrslv_index;
	struct Instance *sim;
	struct Instance *root;
	struct Instance *fiction_inst;
	struct dis_discrete *fiction_dvar;
	struct dis_discrete **dvars;
	struct Name *name;
	enum Proc_enum pe;
	slv_system_t sys;
	slv_status_t solver_status;
	mtx_region_t *blocks;
	const mtx_block_t *installed;

	Asc_CompilerInit(1);
	snprintf(env1,2*PATH_MAX,ASC_ENV_LIBRARY "=%s","models" OSPATH_DIV "solvers/lrslv");
	CU_TEST(0 == Asc_PutEnv(env1));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/lrslv"));
	CU_ASSERT_FATAL(0 == package_load("lrslv",NULL));
	lrslv_index = slv_lookup_client("LRSlv");
	CU_ASSERT_FATAL(lrslv_index != -1);

	Asc_OpenModule("test/lrslv/onerel.a4c",&status);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("onerel")) != NULL);

	sim = SimsCreateInstance(AddSymbol("onerel"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim != NULL);
	root = GetSimulationRoot(sim);
	CU_ASSERT_FATAL(root != NULL);

	name = CreateIdName(AddSymbol("on_load"));
	pe = Initialize(root,name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT_FATAL(pe == Proc_all_ok);

	sys = system_build(root);
	CU_ASSERT_FATAL(sys != NULL);
	CU_ASSERT_FATAL(slv_select_solver(sys,lrslv_index));
	CU_ASSERT_FATAL(0 == lrslv_set_bool_param(sys,"external_blocks",1));

	fiction_inst = ChildByChar(root,AddSymbol("fiction"));
	CU_ASSERT_PTR_NOT_NULL_FATAL(fiction_inst);
	fiction_dvar = lrslv_find_dvar_by_instance(sys,fiction_inst);
	CU_ASSERT_PTR_NOT_NULL_FATAL(fiction_dvar);
	dvars = slv_get_solvers_dvar_list(sys);
	CU_ASSERT_PTR_NOT_NULL_FATAL(dvars);
	dvars[dis_sindex(fiction_dvar)] = dvars[0];
	dis_set_sindex(dvars[0],dis_sindex(fiction_dvar));
	dvars[0] = fiction_dvar;
	dis_set_sindex(fiction_dvar,0);

	blocks = ASC_NEW_ARRAY(mtx_region_t,1);
	CU_ASSERT_PTR_NOT_NULL_FATAL(blocks);
	blocks[0].row.low = 0;
	blocks[0].row.high = 0;
	blocks[0].col.low = 0;
	blocks[0].col.high = 0;
	slv_set_solvers_log_blocks(sys,1,blocks);

	CU_ASSERT_FATAL(0 == slv_presolve(sys));
	slv_get_status(sys,&solver_status);
	CU_ASSERT_FATAL(solver_status.ready_to_solve);
	installed = slv_get_solvers_log_blocks(sys);
	CU_ASSERT_PTR_NOT_NULL_FATAL(installed);
	CU_ASSERT_EQUAL(installed->nblocks,1);
	CU_ASSERT_FATAL(0 == slv_solve(sys));
	slv_get_status(sys,&solver_status);
	CU_ASSERT(solver_status.ok);
	CU_ASSERT(solver_status.converged);
	CU_ASSERT_FALSE(GetBooleanAtomValue(fiction_inst));

	if(sys)system_destroy(sys);
	system_free_reused_mem();
	solver_destroy_engines();
	sim_destroy(sim);
	Asc_CompilerDestroy();
}

/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(boundaries) \
	T(external_blocks)

REGISTER_TESTS_SIMPLE(solver_lrslv, TESTS)
