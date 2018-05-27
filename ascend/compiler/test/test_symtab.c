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
#include <ascend/compiler/symtab.h>
#include <ascend/utilities/error.h>
#include <ascend/general/ascMalloc.h>
#include <test/common.h>

//#define TEST_SYM_DEBUG
#ifdef TEST_SYM_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
#endif

static void test_test1(void){
	// test setup and destruction of the global list
	InitSymbolTable();

	symchar *s = AddSymbol("hello");

	CU_TEST(NULL!=AscFindSymbol(s));
	//CU_TEST(NULL==AscFindSymbol("hellox"));
	CU_TEST(NULL==AscFindSymbol(s+1));
	CU_TEST(NULL==AscFindSymbol(s-1));

	CU_TEST(0==strcmp(SCP(s),"hello"));

	char *c = ASC_NEW_ARRAY(char,3000);
	int i;
	for(i=0;i<3000;i++){
		if(i%10==0){
			c[i]='A' + (i/10)%26;
		}else{
			c[i]='0' + i%10;
		}
	}
	c[2999]='\0';
	MSG("c = '%s'",c);
	
	symchar *s2 = AddSymbol(c);
	CU_TEST(NULL!=AscFindSymbol(s2));
	CU_TEST(0==strcmp(AscFindSymbol(s2),c));

	PrintTab(1);

	DestroySymbolTable();
	DestroyStringSpace();
}

/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(test1)

REGISTER_TESTS_SIMPLE(compiler_symtab, TESTS)

