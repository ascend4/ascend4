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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//**
	@file
	Unit test functions for default_all automatic recursive methods.
*/
#include <string.h>
#include <CUnit/CUnit.h>

#include <ascend/general/env.h>
#include <ascend/utilities/ascConfig.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/error.h>

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/instance_io.h>
#include <ascend/compiler/instantiate.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/watchpt.h>

#include <test/assertimpl.h>


static void test_default1(void){

	struct module_t *m;
	int status;

	CONSOLE_DEBUG("Loading model...");

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	
	/* load the file */
	m = Asc_OpenModule("test/defaultall/test1.a4c",&status);
	CU_ASSERT(status == 0);

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */	
	CU_ASSERT(FindType(AddSymbol("test1"))!=NULL);

	/* instantiate it */
	struct Instance *sim = SimsCreateInstance(AddSymbol("test1"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	CONSOLE_DEBUG("Running on_load...");

	symchar *onload = AddSymbol("on_load");
	enum Proc_enum pe;
	pe = Initialize(sim,CreateIdName(onload),SCP(onload), ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT_FATAL(pe == Proc_all_ok);

	/* check for vars and rels */
	struct Instance *root = GetSimulationRoot(sim);
	struct Instance *inst, *a, *b;

	CU_ASSERT_FATAL((inst = ChildByChar(root,AddSymbol("s1"))) && InstanceKind(inst)==MODEL_INST);
	CU_ASSERT_FATAL((a = ChildByChar(inst,AddSymbol("a"))) && InstanceKind(a)==REAL_ATOM_INST); 
	CU_ASSERT_FATAL((b = ChildByChar(inst,AddSymbol("b"))) && InstanceKind(b)==REAL_ATOM_INST);

	CONSOLE_DEBUG("Checking values...");

	/* check that values have been correctly initialised */
	double va = RealAtomValue(a);
	double vb = RealAtomValue(b);
	CONSOLE_DEBUG("Value of 'a' = %f",va); 
	CU_ASSERT(vb==4.);
	CONSOLE_DEBUG("Value of 'b' = %f",vb); 
	CU_ASSERT(vb==8.);

	CONSOLE_DEBUG("Cleaning up...");
	/* clean up */
	sim_destroy(sim);
	Asc_CompilerDestroy();
}



static void test_default2(void){

	struct module_t *m;
	int status;

	CONSOLE_DEBUG("Loading model...");

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	
	/* load the file */
	m = Asc_OpenModule("test/defaultall/test2.a4c",&status);
	CU_ASSERT(status == 0);

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */	
	CU_ASSERT(FindType(AddSymbol("test2"))!=NULL);

	/* instantiate it */
	struct Instance *sim = SimsCreateInstance(AddSymbol("test2"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	CONSOLE_DEBUG("Running on_load...");

	symchar *onload = AddSymbol("on_load");
	enum Proc_enum pe;
	pe = Initialize(sim,CreateIdName(onload),SCP(onload), ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT_FATAL(pe == Proc_all_ok);

	/* check for vars and rels */
	struct Instance *root = GetSimulationRoot(sim);
	struct Instance *inst1, *inst2, *a, *b;

	CU_ASSERT_FATAL((inst1 = ChildByChar(root,AddSymbol("s2"))) && InstanceKind(inst1)==MODEL_INST);
	CU_ASSERT_FATAL((inst2 = ChildByChar(inst1,AddSymbol("s1a"))) && InstanceKind(inst2)==MODEL_INST);
	CU_ASSERT_FATAL((a = ChildByChar(inst2,AddSymbol("a"))) && InstanceKind(a)==REAL_ATOM_INST); 
	CU_ASSERT_FATAL((b = ChildByChar(inst2,AddSymbol("b"))) && InstanceKind(b)==REAL_ATOM_INST);

	CONSOLE_DEBUG("Checking values...");

	/* check that values have been correctly initialised */
	double va = RealAtomValue(a);
	double vb = RealAtomValue(b);
	CONSOLE_DEBUG("Value of 'a' = %f",va); 
	CU_ASSERT(vb==4.);
	CONSOLE_DEBUG("Value of 'b' = %f",vb); 
	CU_ASSERT(vb==8.);

	CONSOLE_DEBUG("Cleaning up...");
	/* clean up */
	sim_destroy(sim);
	Asc_CompilerDestroy();
}


/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T,X) \
	T(default1) \
	X T(default2)

/* you shouldn't need to change the following */

#define TESTDECL(TESTFN) {#TESTFN,test_##TESTFN}

#define X ,

static CU_TestInfo defaultall_test_list[] = {
	TESTS(TESTDECL,X)
	X CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
	{"packages_defaultall", NULL, NULL, defaultall_test_list},
	CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_packages_defaultall(void){
	return CU_register_suites(suites);
}

