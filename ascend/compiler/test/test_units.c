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
#include <ascend/general/list.h>
#include <ascend/general/platform.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/error.h>

#include <test/common.h>

static unsigned long get_num_units_defined(void){
  unsigned long c, nc = 0;
  struct Units *p;
  for(c=0;c<UNITS_HASH_SIZE;c++) {
    for(p = g_units_hash_table[c];p!=NULL;p=p->next)nc++;
  }
  return nc;
}

static void test_test1(void){
	// test setup and destruction of the global list

	gl_init_pool();
	gl_init();
	InitDimenList();
	InitSymbolTable();
	InitUnitsTable();

	// 10 dimensions plus wild plus dimensionless
	CU_TEST(12==get_num_units_defined());
	DumpUnits(stderr);

	DestroyUnitsTable();
	DestroyStringSpace();
	DestroySymbolTable();
	DestroyDimenList();
	gl_destroy_pool();
}


#define EXPECT_ERROR(USTR,ERRCODE) \
	u = FindOrDefineUnits(USTR,&pos,&errcode);\
	if(errcode && errcode!=ERRCODE){\
		CONSOLE_DEBUG("Expected error code %d, got %d",ERRCODE,errcode);\
		char **e1 = UnitsExplainError(USTR,errcode,pos);\
		CONSOLE_DEBUG("error: %s",e1[1]);\
		CONSOLE_DEBUG("-------%s  %s",e1[2],e1[0]);\
	}\
	CU_TEST(ERRCODE==errcode);\
	if(ERRCODE)CU_TEST(NULL==u) else CU_TEST(NULL!=u);


static void test_test2(void){
	// test setup and destruction of the global list

	gl_init_pool();
	gl_init();
	InitDimenList();
	InitSymbolTable();
	InitUnitsTable();

	// LookupUnits

	const struct Units *u = LookupUnits("kg");
	CU_TEST(NULL!=u);

	u = LookupUnits("MPa");
	CU_TEST(NULL==u);

	// CreateUnitDef

	struct UnitDefinition *ud;
	ud = CreateUnitDef(AddSymbol("N"),"kg*m/s^2","somefile.a4c",15);
	CU_TEST(NULL!=ud);
	CU_TEST(NULL==CreateUnitDef(NULL,"kg*m/s^2","somefile.a4c",15));
	CU_TEST(NULL==CreateUnitDef(AddSymbol("N"),NULL,"somefile.a4c",15));
	CU_TEST(NULL==CreateUnitDef(AddSymbol("N"),"kg*m/s^2",NULL,15));
	DestroyUnitDef(ud);

	// ProcessUnitDef

	ud = CreateUnitDef(AddSymbol("N"),"kg*m/s^2","somefile.a4c",16);
	ProcessUnitDef(ud);
	DestroyUnitDef(ud);

	u = LookupUnits("N");
	CU_TEST(NULL!=u);

	// CheckNewUnits (via ProcessUnitDef)

	ud = CreateUnitDef(AddSymbol("NN"),"kg*MMM/s^2","somefile.a4c",17);
	ProcessUnitDef(ud);
	DestroyUnitDef(ud);
	u = LookupUnits("NN");
	CU_TEST(NULL==u);

	unsigned long nc0 = get_num_units_defined();
	ud = CreateUnitDef(AddSymbol("m"),"s/kg","somefile.a4c",18);
	ProcessUnitDef(ud);
	DestroyUnitDef(ud);
	CU_TEST(nc0==get_num_units_defined()); // nothing added

	nc0 = get_num_units_defined();
	ud = CreateUnitDef(AddSymbol("m"),"s-kg","somefile.a4c",18);
	ProcessUnitDef(ud);
	DestroyUnitDef(ud);
	CU_TEST(nc0==get_num_units_defined()); // nothing added

	// parser checks...

	unsigned long pos = 359;
	int errcode = 229;
	u = FindOrDefineUnits("m", &pos, &errcode);
	CU_TEST(0==errcode);
	CU_TEST(NULL!=u);
	CU_TEST(0==pos);

	EXPECT_ERROR("NNN/mmm",1);

	EXPECT_ERROR("N",0);
	EXPECT_ERROR("N^2",0);
	EXPECT_ERROR("N^-2",0);
	EXPECT_ERROR("N/m",0);
	EXPECT_ERROR("N/m/kg",0);
	EXPECT_ERROR("N/m*kg",0);

	EXPECT_ERROR("N/(m",2);
	EXPECT_ERROR("N-m",3);
	EXPECT_ERROR("/m",8);
	EXPECT_ERROR("N/m*kg/",7);

	EXPECT_ERROR("N^1/2",0);
	char *s1 = UnitsStringSI(u);
	//CONSOLE_DEBUG("string = %s",s1);
	ASC_FREE(s1);

	EXPECT_ERROR("3.5*m",0);
	EXPECT_ERROR("3.5*m*",7);

	EXPECT_ERROR(".678e-2*m",0);

	EXPECT_ERROR("XaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaa\
XaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaa\
XaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaa\
XaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaa\
XaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaa\
YaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaa\
YaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaa\
YaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaa\
YaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaa\
YaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaa\
ZaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaaAaaaabaaaa"
		,5);

	EXPECT_ERROR("3.6N",6);

	EXPECT_ERROR("3.1415926a",6);

	EXPECT_ERROR("2.8e+12",0);
	EXPECT_ERROR("2.8e+1x",6);
	EXPECT_ERROR("2.8e+x",4);
	EXPECT_ERROR("2.8e+e",4);
	EXPECT_ERROR("2.8e-1",0);
	EXPECT_ERROR("2.8e-11",0);
	EXPECT_ERROR("2.8ex",4);
	EXPECT_ERROR("2.8x",6);
	EXPECT_ERROR("2.x",6);
	EXPECT_ERROR("2.",0);
	EXPECT_ERROR("2",0);
	EXPECT_ERROR(".234",0);
	EXPECT_ERROR(".x",4);
	EXPECT_ERROR(".23x",6);
	EXPECT_ERROR(".23e",4);
	EXPECT_ERROR(".23e1",0);
	EXPECT_ERROR(".23e12",0);
	EXPECT_ERROR(".23e+",4);
	EXPECT_ERROR(".23e+5",0);
	EXPECT_ERROR(".23e+5x",6);
	EXPECT_ERROR(".23e-5x",6);
	EXPECT_ERROR(".23e-",4);
	EXPECT_ERROR(".23e-x",4);

	EXPECT_ERROR(")234",9);

	EXPECT_ERROR("2^2",0);
	EXPECT_ERROR("2.5e-5^2",0);
	EXPECT_ERROR("2^-1",0);
	EXPECT_ERROR("2^+1",0);
	EXPECT_ERROR("$",3);

	EXPECT_ERROR("N^2/m^2",0);
	EXPECT_ERROR("N ^ 2 / m ^ 2",0);

	DestroyUnitsTable();
	DestroyStringSpace();
	DestroySymbolTable();
	DestroyDimenList();
	gl_destroy_pool();
}

static void test_test3(void){
	gl_init_pool();
	gl_init();
	InitDimenList();
	InitSymbolTable();
	InitUnitsTable();

	const struct Units *u;
	unsigned long pos = 359;
	int errcode = 229;

	EXPECT_ERROR("kg/m",0);
	EXPECT_ERROR("m/kg",0);
	EXPECT_ERROR("(m/kg",2);
	EXPECT_ERROR("m/kg)",9);

	EXPECT_ERROR("(m/kg)",0);
	EXPECT_ERROR("s/(m/kg)",0);

	EXPECT_ERROR("kg^0.3",6)
	EXPECT_ERROR("2^(5)",0);
	EXPECT_ERROR("(kg/m)^2",2);
	EXPECT_ERROR("kg^(3/10)",0);
	EXPECT_ERROR("kg/(m)",0);
	EXPECT_ERROR("kg/(m*K)",0);

	EXPECT_ERROR("3.5e9*m",0);

	EXPECT_ERROR("3.5e9e0",6);

	DestroyUnitsTable();
	DestroyStringSpace();
	DestroySymbolTable();
	DestroyDimenList();
	gl_destroy_pool();
}


/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(test1) \
	T(test2) \
	T(test3)


REGISTER_TESTS_SIMPLE(compiler_units, TESTS)

