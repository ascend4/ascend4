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
#include <ascend/compiler/name.h>
#include <ascend/compiler/sets.h>
#include <ascend/compiler/exprs.h>
#include <ascend/compiler/nameio.h>
#include <ascend/compiler/symtab.h>
#include <ascend/utilities/error.h>
#include <test/common.h>

//#define TEST_NAME_DEBUG
#ifdef TEST_NAME_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
#endif

static void test_test1(void){
	// test setup and destruction of the global list

	gl_init();
	gl_init_pool();
	name_init_pool();
    exprs_init_pool();
	sets_init_pool();
	InitSymbolTable();

	struct Name *n = CreateIdName(AddSymbol("var1"));
	CU_TEST(1==NameLength(n));
	CU_TEST(NAMEBIT_IDTY==NameId(n));
	CU_TEST(0==strcmp(SCP(NameIdPtr(n)),"var1"));
	CU_TEST(0==NameAuto(n));
	CU_TEST(0==NameCompound(n));
	char *s = WriteNameString(n);
	CU_TEST(0==strcmp(s,"var1"));
	ASC_FREE(s);

	MSG("bits = %d",n->bits);
	MSG("id = %d",NAMEBIT_IDTY);
	MSG("auto = %d",NAMEBIT_AUTO);
	MSG("NameAuto(n) = %d",NameAutoF(n));

	struct Name *n2 = CreateSystemIdName(AddSymbol("sys1"));
	CU_TEST(NAMEBIT_IDTY==NameId(n2));
	CU_TEST(NAMEBIT_AUTO==NameAuto(n2));

	struct Name *n3 = CreateIdName(AddSymbol("var2"));

	struct Name *n4 = CopyAppendNameNode(n,n3);
	CU_TEST(NameCompound(n4));
	s = WriteNameString(n4);
	CU_TEST(0==strcmp(s,"var1.var2"));
	ASC_FREE(s);

	struct Name *n5 = CreateIntegerElementName(37);
	struct Name *n6 = CopyAppendNameNode(n,n5);
	CU_TEST(0==NameCompound(n6));
	s = WriteNameString(n6);
	CU_TEST(0==strcmp(s,"var1[37]"));
	ASC_FREE(s);

	struct Name *n7 = CreateEnumElementName(AddSymbol("helmholtz"));
	struct Name *n8 = CopyAppendNameNode(n,n7);
	CU_TEST(0==NameCompound(n8));
	s = WriteNameString(n8);
	CU_TEST(0==strcmp(s,"var1['helmholtz']"));
	ASC_FREE(s);

	//name_report_pool();

	DestroySymbolTable();
	DestroyStringSpace();
	name_destroy_pool();
	exprs_destroy_pool();
	sets_destroy_pool();
	gl_destroy_pool();
}


/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(test1)

REGISTER_TESTS_SIMPLE(compiler_name, TESTS)

