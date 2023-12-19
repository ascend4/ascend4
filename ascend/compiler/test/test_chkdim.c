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
	Unit test functions for compiler.
*/
#include <ascend/compiler/units.h>
#include <ascend/compiler/symtab.h>

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

#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>
#include <ascend/solver/solver.h>
#include <ascend/system/slv_server.h>
#include <ascend/system/chkdim.h>

#include <test/common.h>
#include <test/common.h>

#define TEST_CHKDIM_DEBUG
#ifdef TEST_CHKDIM_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
#endif

/* didn't work:
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv");

	CONSOLE_DEBUG("Opening '%s'...",filename);

	// load the file, parse it, and find our type definition
	Asc_OpenModule(filename,&status);
	CU_ASSERT(status == 0);
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT(FindType(AddSymbol(modelname))!=NULL);

	// instantiate it
	struct Instance *siminst = SimsCreateInstance(AddSymbol(modelname), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst!=NULL);

    CONSOLE_DEBUG("Running 'on_load'...");
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT_FATAL(pe==Proc_all_ok);

	slv_system_t sys = system_build(siminst);
	CU_ASSERT(sys != NULL);
*/

static void test_dimen_errors(const char *modelfile, const char *modelname, int shouldfail){
	int simplify=1;

	Asc_CompilerInit(simplify);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv");

	int status;

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
	
	int res = chkdim_check_system(sys);

	if(shouldfail){
		if(res){
			MSG("error(s) detected file %s, model %s, as expected",modelfile,modelname);
		}
		CU_ASSERT(res != 0);
	}else{
		if(!res){
			MSG("no errors in file %s, model %s, as expected",modelfile,modelname);
		}
		CU_ASSERT(res == 0);
	}

	if(sys)system_destroy(sys);
	system_free_reused_mem();

	/* destroy, clean up */
	CU_ASSERT(NULL != siminst)
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_chkdim1(){
	char *f="test/chkdim/chkdim1.a4c";
	char *m="chkdim1";
	test_dimen_errors(f,m,TRUE);
}

static void test_chkdim2(){
	char *f="test/chkdim/chkdim2.a4c";
	char *m="chkdim2";
	test_dimen_errors(f,m,TRUE);
}

static void test_chkdim3(){
	char *f="test/chkdim/chkdim2.a4c";
	char *m="chkdim3";
	test_dimen_errors(f,m,TRUE);
}

static void test_chkdim4(){
	char *f="test/chkdim/chkdim2.a4c";
	char *m="chkdim4";
	test_dimen_errors(f,m,TRUE);
}

static void test_chkdim5(){
	char *f="test/chkdim/chkdim2.a4c";
	char *m="chkdim5";
	test_dimen_errors(f,m,TRUE);
}

static void test_chkdim6(){
	char *f="test/chkdim/chkdim2.a4c";
	char *m="chkdim6";
	test_dimen_errors(f,m,TRUE);
}

static void test_chkdim7(){
	char *f="test/chkdim/chkdim2.a4c";
	char *m="chkdim7";
	test_dimen_errors(f,m,FALSE);
}

static void test_chkdim8(){
	char *f="test/chkdim/chkdim2.a4c";
	char *m="shape_circle";
	test_dimen_errors(f,m,FALSE);
}

/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(chkdim1) \
	T(chkdim2) \
	T(chkdim3) \
	T(chkdim4) \
	T(chkdim5) \
	T(chkdim6) \
	T(chkdim7) \
	T(chkdim8)

REGISTER_TESTS_SIMPLE(compiler_chkdim, TESTS)

