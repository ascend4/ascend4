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
#include <ascend/system/var.h>
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

/*===========================================================================*/
/* Registration information */

/* Includes same-address reorder notification, replacement with freed old
   storage, and reuse after another QRSlv client has partitioned the system. */
static void test_list_refresh(void){
	int parse, qr, n, i, pass;
	struct Instance *sim, *root;
	struct Name *method;
	slv_system_t sys;
	SlvClientToken original;
	struct var_variable **old, **replacement, *swap;
	struct rel_relation **oldrels, **newrels;
	slv_status_t status;
	unsigned long revision;
	mtx_matrix_t matrix;
	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv");
	CU_ASSERT_FATAL(0 == package_load("qrslv", NULL));
	qr = slv_lookup_client("QRSlv");
	Asc_OpenModule("test/qrslv/list_refresh.a4c", &parse);
	CU_ASSERT_FATAL(parse == 0 && zz_parse() == 0);
	sim = SimsCreateInstance(AddSymbol("list_refresh"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_PTR_NOT_NULL_FATAL(sim);
	root = GetSimulationRoot(sim);
	method = CreateIdName(AddSymbol("on_load"));
	CU_ASSERT_FATAL(Proc_all_ok == Initialize(root, method, "sim1", ASCERR, WP_STOPONERR, NULL, NULL));
	DestroyName(method);
	sys = system_build(root);
	CU_ASSERT_PTR_NOT_NULL_FATAL(sys);
	CU_ASSERT_FATAL(slv_select_solver(sys, qr) >= 0);
	original = slv_get_client_token(sys);
	CU_ASSERT_FATAL(0 == slv_presolve(sys));
	CU_ASSERT_FATAL(0 == slv_solve(sys));
	matrix = slv_get_sys_mtx(sys);
	revision = slv_get_solver_lists_revision(sys);

	/* Value-only solves must preserve structural state, including across an
	   unchanged-structure presolve (LSODE's post-Jacobian path). */
	for(i = 0; i < 10; ++i){
		SetRealAtomValue(ChildByChar(root, AddSymbol("p")), 8 + 2*i, 0);
		CU_ASSERT_FATAL(0 == slv_resolve(sys));
		CU_ASSERT_FATAL(0 == slv_solve(sys));
		CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(ChildByChar(root, AddSymbol("x"))), 5 + i, 1e-8);
		CU_ASSERT_EQUAL(slv_get_solver_lists_revision(sys), revision);
		CU_ASSERT_PTR_EQUAL(slv_get_sys_mtx(sys), matrix);
		CU_ASSERT_FATAL(0 == slv_presolve(sys));
		CU_ASSERT_PTR_EQUAL(slv_get_sys_mtx(sys), matrix);
	}
	for(pass = 0; pass < 4; ++pass){
		old = slv_get_solvers_var_list(sys);
		n = slv_get_num_solvers_vars(sys);
		if(pass == 0){
			swap = old[0]; old[0] = old[n-1]; old[n-1] = swap;
			for(i = 0; i < n; ++i) var_set_sindex(old[i], i);
			/* Setter must invalidate even with identical address and size. */
			slv_set_solvers_var_list(sys, old, n);
		}else if(pass == 1){
			replacement = ASC_NEW_ARRAY(struct var_variable *, n+1);
			memcpy(replacement, old, (n+1)*sizeof(*old));
			slv_set_solvers_var_list(sys, replacement, n);
			ASC_FREE(old);
		}else if(pass == 2){
			oldrels = slv_get_solvers_rel_list(sys);
			n = slv_get_num_solvers_rels(sys);
			newrels = ASC_NEW_ARRAY(struct rel_relation *, n+1);
			memcpy(newrels, oldrels, (n+1)*sizeof(*oldrels));
			slv_set_solvers_rel_list(sys, newrels, n);
			ASC_FREE(oldrels);
		}else{
			CU_ASSERT_FATAL(slv_switch_solver(sys, qr) >= 0);
			CU_ASSERT_FATAL(0 == slv_presolve(sys));
			CU_ASSERT_FATAL(0 == slv_solve(sys));
			slv_destroy_client(sys);
			slv_set_solver_index(sys, qr);
			slv_set_client_token(sys, original);
		}
		CU_ASSERT(0 != slv_resolve(sys)); /* reject stale lists, don't read them */
		CU_ASSERT(0 != slv_solve(sys));
		CU_ASSERT(0 != slv_iterate(sys));
		CU_ASSERT_FATAL(0 == slv_presolve(sys));
		CU_ASSERT_FATAL(0 == slv_solve(sys));
		slv_get_status(sys, &status);
		CU_ASSERT(status.converged);
		CU_ASSERT_PTR_EQUAL(slv_get_client_token(sys), original);
		CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(ChildByChar(root, AddSymbol("x"))), 14, 1e-8);
	}
	system_destroy(sys);
	system_free_reused_mem();
	solver_destroy_engines();
	sim_destroy(sim);
	Asc_CompilerDestroy();
}

#define TESTS1(T,X) \
	T(fixedbug513_no_simplify) \
	X T(fixedbug513_simplify) \
	X T(fixedbug567) \
	X T(fixedbug564) \
	X T(fixedbug564_repeat) \
	X T(singleton_sticky_resolve) \
	X T(list_refresh)

#define X
#define TESTS(T) TESTS1(T,X)

REGISTER_TESTS_SIMPLE(solver_qrslv, TESTS)
#undef X
