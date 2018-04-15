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

	struct UnitDefinition *ud, *ud1;
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
	T(test2)


REGISTER_TESTS_SIMPLE(compiler_units, TESTS)

