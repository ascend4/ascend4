/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie Mellon University

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
*//**
	@file
	Unit test functions for blackbox parsing/loading/evaluating.
*/
#include <string.h>

#include <ascend/general/env.h>
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
#include <ascend/compiler/pending.h>

#include <ascend/compiler/initialize.h>

#include <ascend/compiler/packages.h>
#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>
#include <ascend/solver/solver.h>
#include <ascend/system/slv_server.h>

#include <test/common.h>
#include <test/assertimpl.h>

static struct Instance *load_model(const char *name,int should_have_error){
	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	/* load the file */
	char path[PATH_MAX];
	strcpy((char *)path,"test/blackbox/");
	strncat(path, name, PATH_MAX - strlen(path));
	strncat(path, ".a4c", PATH_MAX - strlen(path));
	int openmodulestatus;
	Asc_OpenModule(path,&openmodulestatus);
	CU_ASSERT(openmodulestatus == 0);

	/* parse it */

	error_reporter_tree_start();

	CU_ASSERT(zz_parse() == 0);

	CONSOLE_DEBUG("Parse completed");

	/* instantiate it */
	struct Instance *sim = SimsCreateInstance(AddSymbol(name), AddSymbol("sim1"), e_normal, NULL);
	if(error_reporter_tree_has_error()){
		if(!should_have_error){
			CU_FAIL("Unexpected failure in SimsCreateInstance");
		}
	}else{
		if(should_have_error){
			CU_FAIL("No error found although expected in SimsCreateInstance");
		}
	}
	error_reporter_tree_end();
	return sim;
}

static void test_parsefail1(void){
	struct Instance *sim = load_model("parsefail1",1);
	if(sim)sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_parsefail2(void){
	struct Instance *sim = load_model("parsefail2",1);
	if(sim)sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_parsefail3(void){
	struct Instance *sim = load_model("parsefail3",1);
	if(sim)sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_parsefail4(void){
	struct Instance *sim = load_model("parsefail4",1);
	if(sim)sim_destroy(sim);
	Asc_CompilerDestroy();
}


/**
	check that blackbox load and solves correctly
*/
static void load_solve_test(const char *filenamestem, const char *modelname){
	int status;

	/* load the file */
	char modelpath[PATH_MAX];
	strcpy((char *)modelpath,"test/blackbox/");
	strncat(modelpath, filenamestem, PATH_MAX - strlen(modelpath));
	strncat(modelpath, ".a4c", PATH_MAX - strlen(modelpath));
	
	/* load, parse/compile, instantiate, initialise, solve, self-test, destroy */
	Asc_CompilerInit(0);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv"));
	CU_TEST(0 == package_load("qrslv",NULL));
	int qrslv_index = slv_lookup_client("QRSlv");
	CU_ASSERT_FATAL(qrslv_index != -1);
	Asc_OpenModule(modelpath,&status);
	CU_ASSERT(status == 0);
	CU_ASSERT(0 == zz_parse());
	if(modelname==NULL)modelname = filenamestem;
	CU_ASSERT(FindType(AddSymbol(modelname))!=NULL);
	struct Instance *siminst = SimsCreateInstance(AddSymbol(modelname), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst!=NULL);
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);
	slv_system_t sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);
	CU_ASSERT_FATAL(slv_select_solver(sys,qrslv_index));
	CU_ASSERT_FATAL(0 == slv_presolve(sys));
	slv_status_t status1;
	slv_get_status(sys, &status1);
	CU_ASSERT_FATAL(status1.ready_to_solve);
	slv_solve(sys);
	slv_get_status(sys, &status1);
	CU_ASSERT(status1.ok);
	if(sys)system_destroy(sys);
	system_free_reused_mem();
	name = CreateIdName(AddSymbol("self_test"));
	pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);
	solver_destroy_engines();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_pass1(void){
	load_solve_test("pass","pass1");
}

static void test_pass2(void){
	load_solve_test("pass","pass2");
}

static void test_pass5(void){
	load_solve_test("pass5","pass5");
}

static void test_pass6(void){
	load_solve_test("pass5","pass6");
}

static void test_pass7(void){
	load_solve_test("passmerge","pass7");
}

static void test_pass8(void){
	load_solve_test("passmerge","pass8");
}

static void test_pass9(void){
	load_solve_test("passmerge","pass9");
}

static void test_pass10(void){
	load_solve_test("passmerge","pass10");
}

static void test_pass11(void){
	load_solve_test("passmerge","pass11");
}

static void test_pass12(void){
	load_solve_test("passmerge","pass12");
}

/*
static void test_pass13(void){
	load_solve_test("passmerge","pass13");
}*/

static void test_pass14(void){
	load_solve_test("passmerge","pass14");
}

static void test_pass20(void){
	load_solve_test("passarray","pass20");
}

static void test_pass22(void){
	load_solve_test("passarray","pass22");
}

static void test_pass23(void){
	load_solve_test("passarray","pass23");
}


/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(parsefail1) \
	T(parsefail2) \
	T(parsefail3) \
	T(parsefail4) \
	T(pass1) \
	T(pass2) \
	T(pass5) \
	T(pass6) \
	T(pass7) \
	T(pass8) \
	T(pass9) \
	T(pass10) \
	T(pass11) \
	T(pass12) \
	T(pass14) \
	T(pass20) \
	T(pass22) \
	T(pass23)

REGISTER_TESTS_SIMPLE(compiler_blackbox, TESTS)

