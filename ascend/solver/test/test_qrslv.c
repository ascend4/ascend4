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
#include <ascend/system/var.h>
#include <ascend/system/rel.h>
#include <ascend/system/block.h>
#include <ascend/solver/solver.h>
#include <ascend/system/slv_server.h>

#include <test/common.h>

/**
	Reusable function for the standard process of loading, initialising, solving
	and testing a model using QRSlv. Any error from loading, solving, testing
	will result in the test failing.
*/
static void load_solve_test_qrslv(const char *librarypath, const char *modelfile, const char *modelname, int simplify){
	char env1[2*PATH_MAX];
	int status;
	int qrslv_index;

	/* initialise the compiler from scratch */
	Asc_CompilerInit(simplify);

	/* set the needed environment variables so that models, solvers can be found */
	snprintf(env1,2*PATH_MAX,ASC_ENV_LIBRARY "=%s",librarypath);
	CU_TEST(0 == Asc_PutEnv(env1));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv"));
	/* read back and display the ASCENDLIBRARY setting */
	char *lib = Asc_GetEnv(ASC_ENV_LIBRARY);
	CONSOLE_DEBUG("%s = %s\n",ASC_ENV_LIBRARY,lib);
	ASC_FREE(lib);

	/* load the QRSlv solver, presumably from the ASCENDSOLVERS path */
	package_load("qrslv",NULL);
	qrslv_index = slv_lookup_client("QRSlv");
	CU_ASSERT_FATAL(qrslv_index != -1);

	/* load the model file */
	Asc_OpenModule(modelfile,&status);
	CU_ASSERT(status == 0);
	if(status){
		Asc_CompilerDestroy();
		CU_FAIL_FATAL(failed to load module);
	}

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */
	CU_ASSERT(FindType(AddSymbol(modelname))!=NULL);

	/* instantiate it */
	struct Instance *siminst = SimsCreateInstance(AddSymbol(modelname), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst!=NULL);

	/* call on_load method */
	/* FIXME do we check that this method exists first? */
	CONSOLE_DEBUG("RUNNING METHOD 'on_load'");
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	/* 'build' the 'system' -- the flattened system of equations */
	slv_system_t sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);

	/* assign the solver to the system */
	CU_ASSERT_FATAL(slv_select_solver(sys,qrslv_index));
	CONSOLE_DEBUG("Assigned solver '%s'...",slv_solver_name(slv_get_selected_solver(sys)));

	/* presolve, check it's ready, then solve */
	CU_ASSERT_FATAL(0 == slv_presolve(sys));
	slv_status_t status1;
	slv_get_status(sys, &status1);
	CU_ASSERT_FATAL(status1.ready_to_solve);
	slv_solve(sys);
	/* check that solver status was 'ok' */
	slv_get_status(sys, &status1);
	CU_ASSERT(status1.ok);

	/* clean up the 'system' -- we don't need that any more */
	CONSOLE_DEBUG("Destroying system...");
	if(sys)system_destroy(sys);
	system_free_reused_mem();

	/* run 'self_test' method -- we can check there that the results are as expected */
	CONSOLE_DEBUG("Running self-tests");
	name = CreateIdName(AddSymbol("self_test"));
	pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	/* destroy the compiler data structures, hopefully all dynamically allocated memory */
	CONSOLE_DEBUG("Destroying instance tree");
	CU_ASSERT(siminst != NULL);
	solver_destroy_engines();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

/**
	Convenience function to load a model file "name.a4c" and then test the
	model called 'name' within that file, from the directory "models/test/qrslv".
*/
static void test_qrslv(const char *filenamestem, int simplify){
	/* load the file */
	char modelpath[PATH_MAX];
	strcpy((char *)modelpath,"test/qrslv/");
	strncat(modelpath, filenamestem, PATH_MAX - strlen(modelpath));
	strncat(modelpath, ".a4c", PATH_MAX - strlen(modelpath));
	
	load_solve_test_qrslv("models",modelpath,filenamestem,simplify);
}

static void test_fixedbug513_simplify(void){
	test_qrslv("bug513",1);
}

static void test_fixedbug513_no_simplify(void){
	test_qrslv("bug513",0);
}

/* http://ascend4.org/b567, sim_destroy crash (seen at r4354 in trunk). */
static void test_fixedbug567(void){
	/* this test doesn't use the method abouve, because we don't need to solve */
	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv"));
	char *lib = Asc_GetEnv(ASC_ENV_LIBRARY);
	CONSOLE_DEBUG("%s = %s\n",ASC_ENV_LIBRARY,lib);
	ASC_FREE(lib);

	package_load("qrslv",NULL);

	/* load the file */
	const char *path = "models/test/bug567/combinedcycle_fprops.a4c";
	int status;
	Asc_OpenModule(path,&status);
	CU_ASSERT(status == 0);
	if(status){
		Asc_CompilerDestroy();
		CU_FAIL_FATAL(failed to load module);
	}

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */
	const char *simtype = "combinedcycle_toluene";
	CU_ASSERT(FindType(AddSymbol(simtype))!=NULL);

	/* instantiate it */
	struct Instance *siminst = SimsCreateInstance(AddSymbol(simtype), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst!=NULL);

	/* destroy all that stuff */
	CONSOLE_DEBUG("Destroying instance tree");
	CU_ASSERT(siminst != NULL);

	solver_destroy_engines();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_fixedbug564(void){
	load_solve_test_qrslv("models","test/qrslv/akash_eos.a4c","akash_eos",1);
}

static void test_fixedbug564_repeat(void){
	load_solve_test_qrslv("models","test/qrslv/akash_eos.a4c","akash_eos",1);
	load_solve_test_qrslv("models","test/qrslv/akash_eos.a4c","akash_eos",1);
}

static struct Instance *child_by_name(struct Instance *inst, const char *name){
	struct Instance *child = ChildByChar(inst, AddSymbol(name));
	CU_ASSERT_FATAL(child != NULL);
	return child;
}

static struct var_variable *solver_var_by_instance(slv_system_t sys, struct Instance *inst){
	struct var_variable **vp;
	for(vp = slv_get_solvers_var_list(sys); *vp != NULL; ++vp){
		if(var_instance(*vp) == inst){
			return *vp;
		}
	}
	return NULL;
}

static int qrslv_find_param_index(const slv_parameters_t *pp, const char *name){
	int i;
	for(i = 0; i < pp->num_parms; ++i){
		if(pp->parms[i].name != NULL && 0 == strcmp(pp->parms[i].name,name)){
			return i;
		}
	}
	return -1;
}

static int qrslv_set_bool_param(slv_system_t sys, const char *name, int value){
	slv_parameters_t params;
	int idx;
	slv_get_parameters(sys,&params);
	idx = qrslv_find_param_index(&params,name);
	if(idx < 0 || SLV_PARAM_TYPE(&params,idx) != bool_parm){
		return 1;
	}
	SLV_PARAM_BOOL(&params,idx) = value ? 1 : 0;
	slv_set_parameters(sys,&params);
	return 0;
}

static void test_external_blocks(void){
	char env1[2*PATH_MAX];
	int status;
	int qrslv_index;
	struct Instance *siminst;
	struct Instance *root;
	struct Instance *x_inst;
	struct Instance *y_inst;
	struct Name *name;
	enum Proc_enum pe;
	slv_system_t sys;
	slv_status_t solver_status;
	const mtx_block_t *blocks;
	int preblocks;

	Asc_CompilerInit(1);

	snprintf(env1,2*PATH_MAX,ASC_ENV_LIBRARY "=%s","models");
	CU_TEST(0 == Asc_PutEnv(env1));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv"));

	package_load("qrslv",NULL);
	qrslv_index = slv_lookup_client("QRSlv");
	CU_ASSERT_FATAL(qrslv_index != -1);

	Asc_OpenModule("test/decomp/block_cases.a4c",&status);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("real_chain")) != NULL);

	siminst = SimsCreateInstance(AddSymbol("real_chain"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst != NULL);
	root = GetSimulationRoot(siminst);
	CU_ASSERT_FATAL(root != NULL);

	name = CreateIdName(AddSymbol("on_load"));
	pe = Initialize(root, name, "sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT_FATAL(pe == Proc_all_ok);

	sys = system_build(root);
	CU_ASSERT_FATAL(sys != NULL);

	CU_ASSERT_FATAL(0 == slv_block_partition(sys));
	blocks = slv_get_solvers_blocks(sys);
	CU_ASSERT_PTR_NOT_NULL_FATAL(blocks);
	CU_ASSERT_FATAL(blocks->nblocks > 1);
	preblocks = blocks->nblocks;

	CU_ASSERT_FATAL(slv_select_solver(sys, qrslv_index));
	CU_ASSERT_FATAL(0 == qrslv_set_bool_param(sys,"partition",0));
	CU_ASSERT_FATAL(0 == qrslv_set_bool_param(sys,"external_blocks",1));

	CU_ASSERT_FATAL(0 == slv_presolve(sys));
	slv_get_status(sys, &solver_status);
	CU_ASSERT_FATAL(solver_status.ready_to_solve);
	CU_ASSERT_EQUAL(solver_status.block.number_of, preblocks);
	CU_ASSERT_FATAL(0 == slv_solve(sys));
	slv_get_status(sys, &solver_status);
	CU_ASSERT(solver_status.ok);
	CU_ASSERT_EQUAL(solver_status.block.number_of, preblocks);

	x_inst = child_by_name(root, "x");
	y_inst = child_by_name(root, "y");
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(x_inst), 1.0, 1e-8);
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(y_inst), 2.0, 1e-8);

	if(sys)system_destroy(sys);
	system_free_reused_mem();
	solver_destroy_engines();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_external_single_block_scope(void){
	char env1[2*PATH_MAX];
	int status;
	int qrslv_index;
	struct Instance *siminst;
	struct Instance *root;
	struct Instance *x_inst;
	struct Instance *y_inst;
	struct Name *name;
	enum Proc_enum pe;
	slv_system_t sys;
	slv_status_t solver_status;
	struct rel_relation **rels;
	struct var_variable **vars;
	mtx_region_t *oneblock;

	Asc_CompilerInit(1);

	snprintf(env1,2*PATH_MAX,ASC_ENV_LIBRARY "=%s","models");
	CU_TEST(0 == Asc_PutEnv(env1));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv"));

	package_load("qrslv",NULL);
	qrslv_index = slv_lookup_client("QRSlv");
	CU_ASSERT_FATAL(qrslv_index != -1);

	Asc_OpenModule("test/decomp/block_cases.a4c",&status);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("real_chain")) != NULL);

	siminst = SimsCreateInstance(AddSymbol("real_chain"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst != NULL);
	root = GetSimulationRoot(siminst);
	CU_ASSERT_FATAL(root != NULL);

	name = CreateIdName(AddSymbol("on_load"));
	pe = Initialize(root, name, "sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT_FATAL(pe == Proc_all_ok);

	sys = system_build(root);
	CU_ASSERT_FATAL(sys != NULL);
	CU_ASSERT_FATAL(0 == slv_block_partition(sys));

	rels = slv_get_solvers_rel_list(sys);
	vars = slv_get_solvers_var_list(sys);
	CU_ASSERT_PTR_NOT_NULL_FATAL(rels);
	CU_ASSERT_PTR_NOT_NULL_FATAL(vars);
	CU_ASSERT_PTR_NOT_NULL_FATAL(rels[0]);
	CU_ASSERT_PTR_NOT_NULL_FATAL(rels[1]);
	CU_ASSERT_PTR_NOT_NULL_FATAL(vars[0]);
	CU_ASSERT_PTR_NOT_NULL_FATAL(vars[1]);

	rel_set_flagbit(rels[1],REL_INCLUDED,FALSE);
	var_set_fixed(vars[1],TRUE);
	oneblock = ASC_NEW(mtx_region_t);
	CU_ASSERT_PTR_NOT_NULL_FATAL(oneblock);
	oneblock->row.low = oneblock->row.high = 0;
	oneblock->col.low = oneblock->col.high = 0;
	slv_set_solvers_blocks(sys,1,oneblock);

	CU_ASSERT_FATAL(slv_select_solver(sys, qrslv_index));
	CU_ASSERT_FATAL(0 == qrslv_set_bool_param(sys,"partition",0));
	CU_ASSERT_FATAL(0 == qrslv_set_bool_param(sys,"external_blocks",1));

	CU_ASSERT_FATAL(0 == slv_presolve(sys));
	slv_get_status(sys, &solver_status);
	CU_ASSERT_FATAL(solver_status.ready_to_solve);
	CU_ASSERT_EQUAL(solver_status.block.number_of, 1);
	CU_ASSERT_FATAL(0 == slv_solve(sys));
	slv_get_status(sys, &solver_status);
	CU_ASSERT(solver_status.ok);
	CU_ASSERT_EQUAL(solver_status.block.number_of, 1);

	x_inst = child_by_name(root, "x");
	y_inst = child_by_name(root, "y");
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(x_inst), 1.0, 1e-8);
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(y_inst), 0.0, 1e-8);

	if(sys)system_destroy(sys);
	system_free_reused_mem();
	solver_destroy_engines();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_singleton_sticky_resolve(void){
	char env1[2*PATH_MAX];
	int status;
	int qrslv_index;
	struct Instance *siminst;
	struct Instance *root;
	struct Instance *d_inst;
	struct var_variable *d_var;
	struct Name *name;
	enum Proc_enum pe;
	slv_system_t sys;
	slv_status_t status1;
	slv_parameters_t pp;
	static const double d_expected = 4.202626828875603e-16;
	static const double d_stale = 1e-12;

	Asc_CompilerInit(1);

	snprintf(env1,2*PATH_MAX,ASC_ENV_LIBRARY "=%s","models");
	CU_TEST(0 == Asc_PutEnv(env1));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv"));

	package_load("qrslv",NULL);
	qrslv_index = slv_lookup_client("QRSlv");
	CU_ASSERT_FATAL(qrslv_index != -1);

	Asc_OpenModule("test/qrslv/singleton_sticky.a4c",&status);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("singleton_sticky")) != NULL);

	siminst = SimsCreateInstance(AddSymbol("singleton_sticky"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst != NULL);
	root = GetSimulationRoot(siminst);
	CU_ASSERT_FATAL(root != NULL);

	name = CreateIdName(AddSymbol("on_load"));
	pe = Initialize(root, name, "sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT_FATAL(pe == Proc_all_ok);

	sys = system_build(root);
	CU_ASSERT_FATAL(sys != NULL);
	CU_ASSERT_FATAL(slv_select_solver(sys, qrslv_index));

	slv_get_parameters(sys, &pp);
	CU_ASSERT_FATAL(0 == slv_param_char_choose(&pp, "convopt", "RELNOM_SCALE"));
	slv_set_parameters(sys, &pp);

	CU_ASSERT_FATAL(0 == slv_presolve(sys));
	slv_get_status(sys, &status1);
	CU_ASSERT_FATAL(status1.ready_to_solve);
	CU_ASSERT_FATAL(0 == slv_solve(sys));
	slv_get_status(sys, &status1);
	CU_ASSERT_FATAL(status1.ok);

	d_inst = child_by_name(root, "D");
	CU_ASSERT_FATAL(InstanceKind(d_inst) == REAL_ATOM_INST);
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(d_inst), d_expected, 1e-24);

	d_var = solver_var_by_instance(sys, d_inst);
	CU_ASSERT_FATAL(d_var != NULL);

	var_set_fixed(d_var, TRUE);
	var_set_value(d_var, d_stale);
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(d_inst), d_stale, 1e-24);
	var_set_fixed(d_var, FALSE);

	CU_ASSERT_FATAL(0 == slv_presolve(sys));
	slv_get_status(sys, &status1);
	CU_ASSERT_FATAL(status1.ready_to_solve);
	CU_ASSERT_FATAL(0 == slv_solve(sys));
	slv_get_status(sys, &status1);
	CU_ASSERT_FATAL(status1.ok);
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(d_inst), d_expected, 1e-24);
	CU_ASSERT_TRUE(RealAtomValue(d_inst) < 1e-15);

	if(sys)system_destroy(sys);
	system_free_reused_mem();
	solver_destroy_engines();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_nested_when_static_outer_true_inner_false(void){
	load_solve_test_qrslv(
		"models","test/qrslv/nested_when_static.a4c",
		"nested_when_static_outer_true_inner_false",1
	);
}

static void test_nested_when_static_outer_false_inner_true(void){
	load_solve_test_qrslv(
		"models","test/qrslv/nested_when_static.a4c",
		"nested_when_static_outer_false_inner_true",1
	);
}

static void test_when_case_if_rejected(void){
	int status;
	int qrslv_index;
	struct Instance *siminst;
	slv_system_t sys;

	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv"));

	package_load("qrslv",NULL);
	qrslv_index = slv_lookup_client("QRSlv");
	CU_ASSERT_FATAL(qrslv_index != -1);

	Asc_OpenModule("test/instantiate/when_select.a4c",&status);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("when_case_if_parses")) != NULL);

	siminst = SimsCreateInstance(
		AddSymbol("when_case_if_parses"), AddSymbol("sim1"), e_normal, NULL
	);
	CU_ASSERT_FATAL(siminst != NULL);

	sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);
	CU_ASSERT_EQUAL(slv_has_classifier_whens(sys),1);

	CU_ASSERT_FATAL(slv_select_solver(sys,qrslv_index));
	CU_ASSERT_NOT_EQUAL(slv_presolve(sys),0);

	if(sys)system_destroy(sys);
	system_free_reused_mem();
	solver_destroy_engines();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

/*===========================================================================*/
/* Registration information */

#define TESTS1(T,X) \
	T(fixedbug513_no_simplify) \
	X T(fixedbug513_simplify) \
	X T(fixedbug567) \
	X T(fixedbug564) \
	X T(fixedbug564_repeat) \
	T(external_blocks) \
	T(external_single_block_scope) \
	X T(singleton_sticky_resolve) \
	X T(nested_when_static_outer_true_inner_false) \
	X T(nested_when_static_outer_false_inner_true) \
	X T(when_case_if_rejected)

#define X
#define TESTS(T) TESTS1(T,X)

REGISTER_TESTS_SIMPLE(solver_qrslv, TESTS)
#undef X
