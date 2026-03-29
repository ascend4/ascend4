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
#include <ascend/general/ospath.h>
#include <ascend/general/platform.h>
#ifdef __WIN32__
# include <io.h>
# define TEST_CLOSEFD _close
#else
# include <unistd.h>
# define TEST_CLOSEFD close
#endif
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

#define EXPR_DEBUG
#ifdef EXPR_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
#endif

static void test_create(void){

	CU_ASSERT(0 == Asc_CompilerInit(0));

#define DECLVAR(NAME) struct Expr *NAME = CreateVarExpr(CreateIdName(AddSymbol(#NAME)));
	DECLVAR(A);
	DECLVAR(B);
	DECLVAR(C);

	struct Expr *AandB = JoinExprLists(B,JoinExprLists(A,CreateOpExpr(e_and)));
	struct Expr *C_or_AandB = JoinExprLists(AandB,JoinExprLists(C,CreateOpExpr(e_or)));

	(void)C_or_AandB;
	//CONSOLE_DEBUG("write expr, in postfix form: ");
	//WriteExpr(ASCERR,C_or_AandB);
	//FPRINTF(ASCERR,"\n\n");

	Asc_CompilerDestroy();
}


/*
	Testing for Franc Ivankovic's CNF conversion code, ongoing, Jan 2010 -- JP.
*/
static void test_boolrel(void){
	int status;

	Asc_CompilerInit(0); /* no simplification of expressions for this test */
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	/* load the file */
#define TESTFILE "boolrel"
	(void)Asc_OpenModule("test/compiler/" TESTFILE ".a4c",&status);
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


static void test_write(void){
	CU_ASSERT(0 == Asc_CompilerInit(0));

#define DECLVAR(NAME) struct Expr *NAME = CreateVarExpr(CreateIdName(AddSymbol(#NAME)));
	DECLVAR(A);
	DECLVAR(B);
	DECLVAR(C);

#define LEN 1024
	char s[LEN];
	char tmp_path[PATH_MAX] = "";
	int fd = ospath_mkstemp(tmp_path,sizeof(tmp_path),"asc_expr_");
	if(-1==fd){
		perror("ospath_mkstemp");
		CU_FAIL("failed ospath_mkstemp");
		Asc_CompilerDestroy();
		return;
	}
	FILE *tmp = fdopen(fd,"w+");
	if(tmp == NULL){
		perror("fdopen");
		CU_FAIL("failed to open temporary file");
		TEST_CLOSEFD(fd);
		remove(tmp_path);
		Asc_CompilerDestroy();
		return;
	}

	
	// trivial boolean expression

	struct Expr *AandB = JoinExprLists(B,JoinExprLists(A,CreateOpExpr(e_and)));
	struct Expr *C_or_AandB = JoinExprLists(AandB,JoinExprLists(C,CreateOpExpr(e_or)));

	WriteExpr(tmp,C_or_AandB);
	rewind(tmp);
	errno=0;
	memset(s,'\0',LEN);
	CU_TEST(fread(s,1,LEN,tmp));
	CU_TEST(0==strncmp(s,"B A AND C OR",strlen("B A AND C OR")));
	rewind(tmp);

	WriteExprInfix(tmp,C_or_AandB);
	rewind(tmp);
	errno=0;
	memset(s,'\0',LEN);
	CU_TEST(fread(s,1,LEN,tmp));
	CU_TEST(0==strncmp(s,"((B AND A) OR C)",strlen("((B AND A) OR C)")));
	rewind(tmp);

	struct Expr *Asubexpr = JoinExprLists(
		CreateVarExpr(CreateIdName(AddSymbol("A")))
		,CreateOpExpr(e_subexpr)
	);
	WriteExprInfix(tmp,Asubexpr);
	rewind(tmp);
	errno=0;
	memset(s,'\0',LEN);
	CU_TEST(fread(s,1,LEN,tmp));
	CU_TEST(0==strncmp(s,"(A)",strlen("(A)")));
	rewind(tmp);

	struct Expr *Aconst = JoinExprLists(
		CreateVarExpr(CreateIdName(AddSymbol("A")))
		,CreateOpExpr(e_const)
	);
	WriteExprInfix(tmp,Aconst);
	rewind(tmp);
	errno=0;
	memset(s,'\0',LEN);
	CU_TEST(fread(s,1,LEN,tmp));
	CU_TEST(0==strncmp(s,"A",strlen("A")));
	rewind(tmp);

	struct Expr *Apar = JoinExprLists(
		CreateVarExpr(CreateIdName(AddSymbol("A")))
		,CreateOpExpr(e_par)
	);
	WriteExprInfix(tmp,Apar);
	rewind(tmp);
	errno=0;
	memset(s,'\0',LEN);
	CU_TEST(fread(s,1,LEN,tmp));
	CU_TEST(0==strncmp(s,"A",strlen("A")));
	rewind(tmp);

	struct Expr *Ap357t35 = JoinExprLists(
		JoinExprLists(A,CreateOpExpr(e_plus))
		,JoinExprLists(JoinExprLists(CreateIntExpr(357),CreateOpExpr(e_times))
		,CreateRealExpr(3.5,Dimensionless()))
	);

	MSG("EXPR:");
	WriteExpr(ASCERR,Ap357t35);

	fclose(tmp);
	remove(tmp_path);
	Asc_CompilerDestroy();
}







/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(create) \
	T(boolrel) \
	T(write)	

REGISTER_TESTS_SIMPLE(compiler_expr, TESTS)

// vim:syntax=python:ts=2:sw=2:et
