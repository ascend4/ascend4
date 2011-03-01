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
	Unit test functions for compiler. Nothing here yet.
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

#include <ascend/compiler/initialize.h>

#include <test/common.h>
#include <test/assertimpl.h>

static struct Instance *load_model(const char *name){
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	
	/* load the file */
	m = Asc_OpenModule("test/compiler/fixfree.a4c",&status);
	CU_ASSERT(status == 0);

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* instantiate it */
	struct Instance *sim = SimsCreateInstance(AddSymbol(name), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	return sim;
}

#define GET_FIXED(VAR) \
	inst = ChildByChar(root,AddSymbol(VAR)); \
	CU_ASSERT_FATAL(inst!=NULL); \
	CU_ASSERT((InstanceKind(inst)==REAL_ATOM_INST)); \
	inst = ChildByChar(inst,AddSymbol("fixed")); \
	CU_ASSERT_FATAL(inst!=NULL);

#define CHECK_FIXED(VAR) \
	GET_FIXED(VAR);\
	CU_ASSERT(GetBooleanAtomValue(inst));
#define CHECK_FREE(VAR) \
	GET_FIXED(VAR);\
	CU_ASSERT(!GetBooleanAtomValue(inst));


static void test_test1(void){
	struct Instance *sim = load_model("test1");

	/* check for vars and rels */
	struct Instance *root = GetSimulationRoot(sim);
	struct Instance *inst;

	CHECK_FREE("x");
	CHECK_FREE("y");
	CHECK_FREE("z");

	/** Call on_load */
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(sim),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	CHECK_FREE("x");
	CHECK_FIXED("y");
	CHECK_FREE("z");
	
	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_test2(void){
	struct Instance *sim = load_model("test2");

	/* check for vars and rels */
	struct Instance *root = GetSimulationRoot(sim);
	struct Instance *inst;

	CHECK_FREE("x");
	CHECK_FREE("y");
	CHECK_FREE("z");

	/** Call on_load */
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(sim),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe!=Proc_all_ok);

	CHECK_FREE("y");
	CHECK_FIXED("x");
	CHECK_FREE("z"); /* we expect names after the wrong ones not to have been changed */
	
	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_test3(void){
	struct Instance *sim = load_model("test3");

	/* check for vars and rels */
	struct Instance *root = GetSimulationRoot(sim);
	struct Instance *inst;

	CHECK_FREE("x");
	CHECK_FREE("z");

	/** Call on_load */
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(sim),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe!=Proc_all_ok);

	CHECK_FIXED("x");
	CHECK_FREE("z");
	
	sim_destroy(sim);
	Asc_CompilerDestroy();
}


/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(test1) \
	T(test2) \
	T(test3)
	
REGISTER_TESTS_SIMPLE(compiler_fixfree, TESTS)

