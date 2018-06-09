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
#include <ascend/general/list.h>
#include <ascend/general/platform.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/error.h>

#include <ascend/compiler/func.h>

#include <test/common.h>

static void test_test1(void){
	gl_init_pool();
	gl_init();
	InitDimenList();

	const struct Func *f = LookupFunc("lnxxxx");
	CU_ASSERT(NULL == f);
	
#define EPS 1e-20

	f = LookupFunc("ln");
	CU_ASSERT(NULL != f);	
	CU_ASSERT(FuncEval(f,1)==0);
	CU_ASSERT(FuncDeriv(f,1)==1);
	CU_ASSERT(FuncEval(f,exp(1))==1);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv(f,exp(1)),1/exp(1),EPS);
	CU_ASSERT_DOUBLE_EQUAL(FuncDeriv2(f,exp(1)),-1/exp(1)/exp(1),EPS);
	CU_ASSERT(0==strcmp(FuncName(f),"ln"));
	CU_ASSERT(0==strcmp(FuncCName(f),"log"));
	CU_ASSERT(0==strcmp(FuncDeriv1CName(f),"dln"));
	CU_ASSERT(0==strcmp(FuncDeriv2CName(f),"dln2"));
	CU_ASSERT_FATAL(NULL!=FuncDimens(f));
	CU_ASSERT(0==CmpDimen(FuncDimens(f),Dimensionless()));
	CU_ASSERT(F_LN==FuncId(f));

	f = LookupFunc("sin");
	CU_ASSERT(NULL != f);

	DestroyDimenList();
	gl_destroy_pool();
}

/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(test1)

REGISTER_TESTS_SIMPLE(compiler_func, TESTS)

