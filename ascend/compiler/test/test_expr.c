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
#include <CUnit/CUnit.h>

#include <ascend/general/env.h>
#include <ascend/general/platform.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/error.h>

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/exprio.h>
#include <ascend/compiler/exprs.h>
#include <ascend/compiler/symtab.h>

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


/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T,X) \
	T(create)

/* you shouldn't need to change the following */

#define TESTDECL(TESTFN) {#TESTFN,test_##TESTFN}

#define X ,

static CU_TestInfo expr_test_list[] = {
	TESTS(TESTDECL,X)
	X CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
	{"compiler_expr", NULL, NULL, expr_test_list},
	CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_compiler_expr(void){
	return CU_register_suites(suites);
}
