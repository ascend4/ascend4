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
#include <ascend/general/ospath.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <ascend/general/env.h>
#include <ascend/general/list.h>
#include <ascend/general/platform.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/error.h>

#include <test/common.h>

//#define TEST_UNITS_DEBUG
#ifdef TEST_UNITS_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
#endif

static unsigned long get_num_units_defined(void){
  unsigned long c, nc = 0;
  struct Units *p;
  for(c=0;c<UNITS_HASH_SIZE;c++) {
    for(p = g_units_hash_table[c];p!=NULL;p=p->next)nc++;
  }
  return nc;
}

static void destroy_ladder_item_list(struct gl_list_t *items){
	unsigned long i, len;
	struct UnitLadderItem *item;
	if (items == NULL) {
		return;
	}
	len = gl_length(items);
	for (i = 1; i <= len; ++i) {
		item = (struct UnitLadderItem *)gl_fetch(items,i);
		DestroyUnitLadderItem(item);
	}
	gl_destroy(items);
}

static int make_temp_path(char *dst, size_t dstlen, const char *prefix){
	int fd;
	if (dst == NULL || dstlen == 0) {
		return -1;
	}
	fd = ospath_mkstemp(dst,dstlen,prefix);
	if (fd < 0) {
		return -1;
	}
	close(fd);
	return 0;
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
    MSG("Expecting error %d, got error %d, with string '%s'",ERRCODE,errcode,USTR);\
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

	EXPECT_ERROR("kg^0.3",6)

	EXPECT_ERROR("kg/m",0);
	EXPECT_ERROR("m/kg",0);
	EXPECT_ERROR("(m/kg",2);

	EXPECT_ERROR("(m/kg)",0);
	EXPECT_ERROR("s/(m/kg)",0);

	EXPECT_ERROR("kg/(m)",0);
	EXPECT_ERROR("kg/(m*K)",0);

	EXPECT_ERROR("3.5e9*m",0);

	EXPECT_ERROR("3.5e9e0",6);

	EXPECT_ERROR("m/kg)",9);

	EXPECT_ERROR("kg/(s/m))",9);

	EXPECT_ERROR("kg/(s/m",2);

	EXPECT_ERROR("(kg/m)^2",0);

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

	EXPECT_ERROR("2^(5)",0);
	EXPECT_ERROR("kg^(3/10)",0);
	EXPECT_ERROR("kg^(-3/10)",12);
	EXPECT_ERROR("kg^(-3/-10)",0);
	EXPECT_ERROR("kg^(3/-10)",12);

	EXPECT_ERROR("kg^(3/10/5)",13);
	EXPECT_ERROR("kg^(3/10",13);
	EXPECT_ERROR("kg^(3",13);


	DestroyUnitsTable();
	DestroyStringSpace();
	DestroySymbolTable();
	DestroyDimenList();
	gl_destroy_pool();
}

static void test_test4(void){
	const struct Units *u_w, *u_kw, *u_hp, *u_mw;
	struct gl_list_t *items;
	long ladder_id;

	gl_init_pool();
	gl_init();
	InitDimenList();
	InitSymbolTable();
	InitUnitsTable();

	items = gl_create(10L);
	gl_append_ptr(items,(char *)CreateUnitLadderItem(AddSymbol("W"),"kg*m^2/s^3",0,"test",1));
	gl_append_ptr(items,(char *)CreateUnitLadderItem(AddSymbol("kW"),"1e3*W",0,"test",2));
	gl_append_ptr(items,(char *)CreateUnitLadderItem(AddSymbol("MW"),"1e6*W",0,"test",3));
	CU_TEST(ProcessUnitLadder(items) == 0);
	destroy_ladder_item_list(items);

	u_w = LookupUnits("W");
	u_kw = LookupUnits("kW");
	u_mw = LookupUnits("MW");
	CU_TEST(NULL != u_w);
	CU_TEST(NULL != u_kw);
	CU_TEST(NULL != u_mw);
	ladder_id = UnitsLadderId(u_w);
	CU_TEST(ladder_id >= 0);
	CU_TEST(UnitsLadderId(u_kw) == ladder_id);
	CU_TEST(UnitsLadderId(u_mw) == ladder_id);
	CU_TEST(UnitsLadderRank(u_w) == 0);
	CU_TEST(UnitsLadderRank(u_kw) == 1);
	CU_TEST(UnitsLadderRank(u_mw) == 2);

	items = gl_create(10L);
	gl_append_ptr(items,(char *)CreateUnitLadderItem(AddSymbol("kW"),NULL,1,"test",4));
	gl_append_ptr(items,(char *)CreateUnitLadderItem(AddSymbol("hp"),"0.745699872*kW",0,"test",5));
	CU_TEST(ProcessUnitLadder(items) == 0);
	destroy_ladder_item_list(items);

	u_hp = LookupUnits("hp");
	CU_TEST(NULL != u_hp);
	CU_TEST(UnitsLadderId(u_hp) == ladder_id);
	CU_TEST(UnitsLadderRank(u_hp) == 2);
	CU_TEST(UnitsLadderRank(u_mw) == 3);
	CU_TEST(LookupUnitsByLadder(ladder_id,2) == u_hp);

	items = gl_create(10L);
	gl_append_ptr(items,(char *)CreateUnitLadderItem(AddSymbol("kg"),NULL,1,"test",6));
	gl_append_ptr(items,(char *)CreateUnitLadderItem(AddSymbol("slug"),"14.59390294*kg",0,"test",7));
	CU_TEST(ProcessUnitLadder(items) > 0);
	destroy_ladder_item_list(items);
	CU_TEST(NULL == LookupUnits("slug"));

	DestroyUnitsTable();
	DestroyStringSpace();
	DestroySymbolTable();
	DestroyDimenList();
	gl_destroy_pool();
}

static void define_unit(const char *name, const char *expr){
	struct UnitDefinition *ud = CreateUnitDef(AddSymbol(name),expr,"test_units.c",1);
	CU_ASSERT_PTR_NOT_NULL_FATAL(ud);
	ProcessUnitDef(ud);
	DestroyUnitDef(ud);
	CU_ASSERT_PTR_NOT_NULL_FATAL(LookupUnits(name));
}

static void test_test5(void){
	struct UnitsOverridesDB *db = NULL;
	struct UnitsOverridesDB *db2 = NULL;
	const struct Units *u;
	const dim_type *powerdim;
	const dim_type *lengthdim;
	unsigned loaded = 0, errors = 0;
	char fn1[PATH_MAX] = "";
	char fn2[PATH_MAX] = "";
	FILE *fp;
	char *defaultpath;
	int found_simroot_name = 0;
	int found_trimmed_name = 0;
	char line[512];

	gl_init_pool();
	gl_init();
	InitDimenList();
	InitSymbolTable();
	InitUnitsTable();

	define_unit("W","kg*m^2/s^3");
	define_unit("kW","1e3*W");
	define_unit("MW","1e6*W");
	define_unit("GW","1e9*W");

	db = UnitsOverridesCreate();
	CU_ASSERT_PTR_NOT_NULL_FATAL(db);

	CU_TEST(0 == UnitsOverridesSet(db,UNITS_OVERRIDE_TYPE,"","energy_rate","W"));
	CU_TEST(0 == UnitsOverridesSet(db,UNITS_OVERRIDE_TYPE,"models/johnpye/demo.a4c","energy_rate","kW"));
	CU_TEST(0 == UnitsOverridesSet(db,UNITS_OVERRIDE_NAME,"models/johnpye/demo.a4c","plant.tes.power","MW"));
	CU_TEST(0 != UnitsOverridesSet(db,UNITS_OVERRIDE_NAME,"","global.bad.name","MW"));

	powerdim = UnitsDimensions(LookupUnits("W"));
	CU_ASSERT_PTR_NOT_NULL_FATAL(powerdim);

	u = UnitsOverridesResolve(db,"models/johnpye/demo.a4c","energy_rate","plant.tes.power",powerdim);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_TEST(0 == strcmp(SCP(UnitsDescription(u)),"MW"));
	u = UnitsOverridesResolve(db,"models/johnpye/demo.a4c","energy_rate","sim1.plant.tes.power",powerdim);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_TEST(0 == strcmp(SCP(UnitsDescription(u)),"MW"));
#ifdef _WIN32
	u = UnitsOverridesResolve(db,"MODELS/JOHNPYE/DEMO.A4C","energy_rate","plant.tes.power",powerdim);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_TEST(0 == strcmp(SCP(UnitsDescription(u)),"MW"));
	u = UnitsOverridesResolve(db,"MODELS\\JOHNPYE\\DEMO.A4C","energy_rate","plant.tes.power",powerdim);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_TEST(0 == strcmp(SCP(UnitsDescription(u)),"MW"));
#endif

	u = UnitsOverridesResolve(db,"models/johnpye/demo.a4c","energy_rate","plant.other.power",powerdim);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_TEST(0 == strcmp(SCP(UnitsDescription(u)),"kW"));

	u = UnitsOverridesResolve(db,"models/other/demo.a4c","energy_rate","plant.tes.power",powerdim);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_TEST(0 == strcmp(SCP(UnitsDescription(u)),"W"));

	u = UnitsOverridesResolve(db,"","energy_rate","plant.tes.power",powerdim);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_TEST(0 == strcmp(SCP(UnitsDescription(u)),"W"));

	CU_TEST(0 == UnitsOverridesSet(db,UNITS_OVERRIDE_TYPE,"models/johnpye/demo.a4c","length_type","s"));
	lengthdim = UnitsDimensions(LookupUnits("m"));
	CU_ASSERT_PTR_NOT_NULL_FATAL(lengthdim);
	CU_TEST(NULL == UnitsOverridesResolve(db,"models/johnpye/demo.a4c","length_type","",lengthdim));
	CU_TEST(NULL == UnitsOverridesLookup(db,UNITS_OVERRIDE_TYPE,"models/johnpye/demo.a4c","length_type"));

	if (0 != make_temp_path(fn1,sizeof(fn1),"asc_uovr_1_")) {
		CU_FAIL("failed creating temporary file path");
		goto cleanup;
	}
	if (0 != make_temp_path(fn2,sizeof(fn2),"asc_uovr_2_")) {
		CU_FAIL("failed creating temporary file path");
		goto cleanup;
	}
	fp = fopen(fn1,"w");
	CU_ASSERT_PTR_NOT_NULL(fp);
	if (fp == NULL) {
		goto cleanup;
	}
	fprintf(fp,"[global]\n");
	fprintf(fp,"type.energy_rate = kW\n");
	fprintf(fp,"badkey = kW\n");
	fprintf(fp,"name.not_allowed = MW\n");
	fprintf(fp,"\n[models/johnpye/demo.a4c]\n");
	fprintf(fp,"type.energy_rate = MW\n");
	fprintf(fp,"name.plant.tes.power = GW\n");
	fprintf(fp,"name.broken = NOT_A_UNIT\n");
	fclose(fp);

	UnitsOverridesClear(db);
	CU_TEST(0 == UnitsOverridesLoad(db,fn1,&loaded,&errors));
	CU_TEST(loaded == 4);
	CU_TEST(errors >= 2);

	u = UnitsOverridesResolve(db,"models/johnpye/demo.a4c","energy_rate","plant.tes.power",powerdim);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_TEST(0 == strcmp(SCP(UnitsDescription(u)),"GW"));
	u = UnitsOverridesResolve(db,"models/johnpye/demo.a4c","energy_rate","sim1.plant.tes.power",powerdim);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_TEST(0 == strcmp(SCP(UnitsDescription(u)),"GW"));
	CU_TEST(NULL == UnitsOverridesLookup(db,UNITS_OVERRIDE_NAME,"models/johnpye/demo.a4c","broken"));

	CU_TEST(0 == UnitsOverridesSet(
		db,UNITS_OVERRIDE_NAME,"models/johnpye/demo.a4c","sim1.plant.rooted.power","MW"
	));
	CU_TEST(0 == UnitsOverridesSetSimroot(db,"sim1"));
	CU_TEST(0 == UnitsOverridesSave(db,fn2));
	fp = fopen(fn2,"r");
	CU_ASSERT_PTR_NOT_NULL(fp);
	if (fp == NULL) {
		goto cleanup;
	}
	while (fgets(line,sizeof(line),fp) != NULL) {
		if (strstr(line,"name.sim1.plant.rooted.power") != NULL) {
			found_simroot_name = 1;
		}
		if (strstr(line,"name.plant.rooted.power = MW") != NULL) {
			found_trimmed_name = 1;
		}
	}
	fclose(fp);
	CU_TEST(!found_simroot_name);
	CU_TEST(found_trimmed_name);

	db2 = UnitsOverridesCreate();
	CU_ASSERT_PTR_NOT_NULL(db2);
	if (db2 == NULL) {
		goto cleanup;
	}
	UnitsOverridesClear(db2);
	CU_TEST(0 == UnitsOverridesLoad(db2,fn2,&loaded,&errors));
	u = UnitsOverridesResolve(db2,"models/johnpye/demo.a4c","energy_rate","plant.tes.power",powerdim);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_TEST(0 == strcmp(SCP(UnitsDescription(u)),"GW"));
	u = UnitsOverridesResolve(db2,"models/johnpye/demo.a4c","energy_rate","sim1.plant.rooted.power",powerdim);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_TEST(0 == strcmp(SCP(UnitsDescription(u)),"MW"));

	defaultpath = UnitsOverridesDefaultPath();
	CU_ASSERT_PTR_NOT_NULL(defaultpath);
	if (defaultpath != NULL) {
		ASC_FREE(defaultpath);
	}

cleanup:
	UnitsOverridesDestroy(db2);
	UnitsOverridesDestroy(db);
	if (fn1[0] != '\0') remove(fn1);
	if (fn2[0] != '\0') remove(fn2);

	DestroyUnitsTable();
	DestroyStringSpace();
	DestroySymbolTable();
	DestroyDimenList();
	gl_destroy_pool();
}

static void test_test6(void){
	struct UnitsOverridesDB *db = NULL;
	const struct Units *u;
	const dim_type *powerdim;
	unsigned loaded = 0, errors = 0;
	char fn1[PATH_MAX] = "";
	char fn2[PATH_MAX] = "";
	FILE *fp;
	int found_entry = 0;
	char line[256];

	gl_init_pool();
	gl_init();
	InitDimenList();
	InitSymbolTable();
	InitUnitsTable();

	if (0 != make_temp_path(fn1,sizeof(fn1),"asc_uovr_lazy_1_")) {
		CU_FAIL("failed creating temporary file path");
		goto cleanup;
	}
	if (0 != make_temp_path(fn2,sizeof(fn2),"asc_uovr_lazy_2_")) {
		CU_FAIL("failed creating temporary file path");
		goto cleanup;
	}

	fp = fopen(fn1,"w");
	CU_ASSERT_PTR_NOT_NULL(fp);
	if (fp == NULL) {
		goto cleanup;
	}
	fprintf(fp,"[global]\n");
	fprintf(fp,"type.energy_rate = MW\n");
	fclose(fp);

	db = UnitsOverridesCreate();
	CU_ASSERT_PTR_NOT_NULL(db);
	if (db == NULL) {
		goto cleanup;
	}

	CU_TEST(0 == UnitsOverridesLoad(db,fn1,&loaded,&errors));
	CU_TEST(loaded == 1);
	CU_TEST(errors == 0);

	/* Save immediately: unresolved entries must not be dropped. */
	CU_TEST(0 == UnitsOverridesSave(db,fn2));
	fp = fopen(fn2,"r");
	CU_ASSERT_PTR_NOT_NULL(fp);
	if (fp == NULL) {
		goto cleanup;
	}
	while (fgets(line,sizeof(line),fp) != NULL) {
		if (strstr(line,"type.energy_rate = MW") != NULL) {
			found_entry = 1;
			break;
		}
	}
	fclose(fp);
	CU_TEST(found_entry);

	/* Define units after load, then resolve lazily. */
	define_unit("W","kg*m^2/s^3");
	define_unit("MW","1e6*W");
	powerdim = UnitsDimensions(LookupUnits("W"));
	CU_ASSERT_PTR_NOT_NULL_FATAL(powerdim);
	u = UnitsOverridesResolve(db,"","energy_rate","plant.power",powerdim);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_TEST(0 == strcmp(SCP(UnitsDescription(u)),"MW"));

cleanup:
	UnitsOverridesDestroy(db);
	if (fn1[0] != '\0') remove(fn1);
	if (fn2[0] != '\0') remove(fn2);

	DestroyUnitsTable();
	DestroyStringSpace();
	DestroySymbolTable();
	DestroyDimenList();
	gl_destroy_pool();
}

static void test_test7(void){
	struct UnitsOverridesDB *db = NULL;
	const struct Units *u;
	const dim_type *powerdim;
	unsigned loaded = 0, errors = 0;
	char fn1[PATH_MAX] = "";
	FILE *fp;

	gl_init_pool();
	gl_init();
	InitDimenList();
	InitSymbolTable();
	InitUnitsTable();

	/* Provide the target dimension, but not the requested override units yet. */
	define_unit("W","kg*m^2/s^3");
	powerdim = UnitsDimensions(LookupUnits("W"));
	CU_ASSERT_PTR_NOT_NULL_FATAL(powerdim);

	if (0 != make_temp_path(fn1,sizeof(fn1),"asc_uovr_ctx_1_")) {
		CU_FAIL("failed creating temporary file path");
		goto cleanup;
	}
	fp = fopen(fn1,"w");
	CU_ASSERT_PTR_NOT_NULL(fp);
	if (fp == NULL) {
		goto cleanup;
	}
	fprintf(fp,"[global]\n");
	fprintf(fp,"type.energy_rate = MW\n");
	fclose(fp);

	db = UnitsOverridesCreate();
	CU_ASSERT_PTR_NOT_NULL(db);
	if (db == NULL) {
		goto cleanup;
	}
	CU_TEST(0 == UnitsOverridesLoad(db,fn1,&loaded,&errors));
	CU_TEST(loaded == 1);
	CU_TEST(errors == 0);

	/*
	 * First resolve occurs in a context where MW is still undefined:
	 * must not remove the preference.
	 */
	u = UnitsOverridesResolve(db,"","energy_rate","plant.power",powerdim);
	CU_TEST(NULL == u);

	/* Later context provides MW; stored preference must now apply. */
	define_unit("MW","1e6*W");
	u = UnitsOverridesResolve(db,"","energy_rate","plant.power",powerdim);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_TEST(0 == strcmp(SCP(UnitsDescription(u)),"MW"));

cleanup:
	UnitsOverridesDestroy(db);
	if (fn1[0] != '\0') remove(fn1);

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
	T(test3) \
	T(test4) \
	T(test5) \
	T(test6) \
	T(test7)


REGISTER_TESTS_SIMPLE(compiler_units, TESTS)
