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

static void test_chkdim1(){
	char *modelfile="test/chkdim/chkdim1.a4c";
	char *modelname="chkdim1";
	int simplify=0;
	const char *librarypath="models";

	char env1[2*PATH_MAX];
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv");

	//int qrslv_index;
	//package_load("qrslv",NULL);
	//qrslv_index = slv_lookup_client("QRSlv");
	//CU_ASSERT_FATAL(qrslv_index != -1);

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
	
	struct rel_relation **rels = slv_get_master_rel_list(sys);
	CU_ASSERT(NULL!=rels);
	
	int32 numrels = slv_get_num_master_rels(sys);
	
	int OK = 1;
	for(int32 i=0; i<numrels; ++i){
		struct Instance *inst = rels[i]->instance;
		MSG("relinst = %p",inst);
		int res = chkdim_check_relation(inst); // returns 0 on success
		OK &= !res;
		MSG("res = %d",res);
	}
	if(!OK){MSG("error(s) were successfully detected");}
	CU_ASSERT(!OK);

	if(sys)system_destroy(sys);
	system_free_reused_mem();

	/* destroy, clean up */
	CU_ASSERT(NULL != siminst)
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}


/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(chkdim1)

REGISTER_TESTS_SIMPLE(compiler_chkdim, TESTS)

