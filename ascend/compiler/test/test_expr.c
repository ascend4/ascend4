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
#include <ascend/compiler/exprs.h>
#include <ascend/compiler/exprio.h>
#include <ascend/compiler/logrel_io.h>

#include <test/common.h>

static void test_create(void){

	CU_ASSERT(0 == Asc_CompilerInit(0));

#define DECLVAR(NAME) struct Expr *NAME = CreateVarExpr(CreateIdName(AddSymbol(#NAME)));
	DECLVAR(A);
	DECLVAR(B);
	DECLVAR(C);

	struct Expr *AandB = JoinExprLists(B,JoinExprLists(A,CreateOpExpr(e_and)));
	struct Expr *C_or_AandB = JoinExprLists(AandB,JoinExprLists(C,CreateOpExpr(e_or)));

	CONSOLE_DEBUG("write expr, in postfix form: ");
	WriteExpr(ASCERR,C_or_AandB);
	FPRINTF(ASCERR,"\n\n");

	Asc_CompilerDestroy();
}


/*
	Testing for Franc Ivankovic's CNF conversion code, ongoing, Jan 2010 -- JP.
*/
static void test_boolrel(void){
	struct module_t *m;
	int status;

	Asc_CompilerInit(0); /* no simplification of expressions for this test */
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	/* load the file */
#define TESTFILE "boolrel"
	m = Asc_OpenModule("test/compiler/" TESTFILE ".a4c",&status);
	CU_ASSERT(status == 0);

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */
	CU_ASSERT(FindType(AddSymbol(TESTFILE))!=NULL);

	/* instantiate it */
	struct Instance *sim = SimsCreateInstance(AddSymbol(TESTFILE), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	/* call the on_load method */
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	CU_ASSERT(Proc_all_ok == Initialize(GetSimulationRoot(sim),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL));

	/* Check that x := 2 was NOT executed (after error statement) */
	struct Instance *inst;
	struct Instance *root = GetSimulationRoot(sim);
	CU_ASSERT(NULL != root);
	CU_ASSERT(NULL != (inst = ChildByChar(root,AddSymbol("rel1"))));
	CONSOLE_DEBUG("Instance kind = %d",InstanceKind(inst));
	CU_ASSERT(InstanceKind(inst)==LREL_INST);

	char *out = WriteLogRelToString(inst,root);
	CONSOLE_DEBUG("Relation: %s",out);


	ASC_FREE(out);

	/* clean up */
	sim_destroy(sim);
	Asc_CompilerDestroy();
#undef TESTFILE
}


/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(create) \
	T(boolrel)

REGISTER_TESTS_SIMPLE(compiler_expr, TESTS)

