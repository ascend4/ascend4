/*	ASCEND modelling environment
	Copyright (C) 2026

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.
*/
/**
	@file
	Unit tests for instance display-units resolution.
*/

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/instantiate.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/instance_io.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/type_desc.h>
#include <ascend/compiler/units.h>

#include <test/common.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct Instance *owner_model_instance(struct Instance *inst){
	struct Instance *p = inst;
	while (p != NULL) {
		enum inst_t k = InstanceKind(p);
		if (k == MODEL_INST) {
			return p;
		}
		if (k == SIM_INST || NumberParents(p) < 1) {
			return NULL;
		}
		p = InstanceParent(p,1);
	}
	return NULL;
}

static char *model_scope_key(struct Instance *model_inst){
	const struct TypeDescription *td;
	const struct module_t *mod;
	const char *modfile = NULL;
	const char *mname = NULL;
	size_t nmod;
	size_t nname;
	char *scope;
	if (model_inst == NULL || InstanceKind(model_inst) != MODEL_INST) {
		return NULL;
	}
	td = InstanceTypeDesc(model_inst);
	if (td == NULL) {
		return NULL;
	}
	if (GetName(td) != NULL) {
		mname = SCP(GetName(td));
	}
	mod = GetModule(td);
	if (mod != NULL && Asc_ModuleFileName(mod) != NULL) {
		modfile = Asc_ModuleFileName(mod);
	}
	if ((modfile == NULL || *modfile == '\0') && (mname == NULL || *mname == '\0')) {
		return NULL;
	}
	nmod = (modfile != NULL) ? strlen(modfile) : 0;
	nname = (mname != NULL) ? strlen(mname) : 0;
	if (nmod == 0) {
		scope = (char *)malloc(nname + 1);
		if (scope != NULL) {
			memcpy(scope,mname,nname + 1);
		}
		return scope;
	}
	if (nname == 0) {
		scope = (char *)malloc(nmod + 1);
		if (scope != NULL) {
			memcpy(scope,modfile,nmod + 1);
		}
		return scope;
	}
	scope = (char *)malloc(nmod + 2 + nname + 1);
	if (scope != NULL) {
		snprintf(scope,nmod + 2 + nname + 1,"%s::%s",modfile,mname);
	}
	return scope;
}

static void test_displayunits_resolve(void){
	int status = 0;
	int has_error = 0;
	CONST char *model =
		"UNITS\n"
		"  W = {kg*m^2/s^3};\n"
		"  kW = {1e3*W};\n"
		"  MW = {1e6*W};\n"
		"  GW = {1e9*W};\n"
		"END UNITS;\n"
		"UNITS LADDER\n"
		"  W = {kg*m^2/s^3};\n"
		"  kW = {1e3*W};\n"
		"  MW = {1e6*W};\n"
		"  GW = {1e9*W};\n"
		"END UNITS LADDER;\n"
		"ATOM energy_rate REFINES real DIMENSION M*L^2/T^3 DEFAULT 9500 {MW};\n"
		"END energy_rate;\n"
		"MODEL display_units_test;\n"
		"  x IS_A energy_rate;\n"
		"  y IS_A energy_rate;\n"
		"END display_units_test;\n";
	struct Instance *sim = NULL;
	struct Instance *simroot = NULL;
	struct Instance *x = NULL;
	struct Instance *y = NULL;
	struct Instance *owner = NULL;
	struct UnitsOverridesDB *db = NULL;
	CONST struct Units *u = NULL;
	char *scope = NULL;
	char *xname = NULL;
	int have_scope_name_override = 0;

	CU_ASSERT(0 == Asc_CompilerInit(1));
	/*m =*/ Asc_OpenStringModule(model,&status,"displayunits");
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();
	CU_ASSERT(has_error == 0);

	sim = SimsCreateInstance(AddSymbol("display_units_test"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_PTR_NOT_NULL_FATAL(sim);
	simroot = GetSimulationRoot(sim);
	CU_ASSERT_PTR_NOT_NULL_FATAL(simroot);
	x = ChildByChar(simroot,AddSymbol("x"));
	y = ChildByChar(simroot,AddSymbol("y"));
	CU_ASSERT_PTR_NOT_NULL_FATAL(x);
	CU_ASSERT_PTR_NOT_NULL_FATAL(y);

	u = UnitsResolveDisplayForInstance(NULL,x,1,0.1,1000.0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_ASSERT_STRING_EQUAL(SCP(UnitsDescription(u)),"GW");
	u = UnitsResolveDisplayForInstance(NULL,x,0,0.1,1000.0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_ASSERT_STRING_EQUAL(SCP(UnitsDescription(u)),"MW");

	owner = owner_model_instance(x);
	CU_ASSERT_PTR_NOT_NULL_FATAL(owner);
	scope = model_scope_key(owner);
	CU_ASSERT_PTR_NOT_NULL_FATAL(scope);

	db = UnitsOverridesCreate();
	CU_ASSERT_PTR_NOT_NULL_FATAL(db);
	CU_ASSERT(0 == UnitsOverridesSet(db,UNITS_OVERRIDE_TYPE,"","energy_rate","kW"));

	if (*scope != '\0') {
		xname = WriteInstanceNameString(x,owner);
		CU_ASSERT_PTR_NOT_NULL_FATAL(xname);
		CU_ASSERT(0 == UnitsOverridesSet(db,UNITS_OVERRIDE_NAME,scope,xname,"MW"));
		have_scope_name_override = 1;
	}

	u = UnitsResolveDisplayForInstance(db,x,1,0.1,1000.0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	if (have_scope_name_override) {
		CU_ASSERT_STRING_EQUAL(SCP(UnitsDescription(u)),"MW");
	}else{
		CU_ASSERT_STRING_EQUAL(SCP(UnitsDescription(u)),"kW");
	}

	u = UnitsResolveDisplayForInstance(db,y,1,0.1,1000.0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_ASSERT_STRING_EQUAL(SCP(UnitsDescription(u)),"kW");

	if (xname != NULL) {
		ASC_FREE(xname);
	}
	if (scope != NULL) {
		free(scope);
	}
	UnitsOverridesDestroy(db);
	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_displayunits_model_scope_reuse(void){
	int status = 0;
	int has_error = 0;
	CONST char *model =
		"UNITS\n"
		"  W = {kg*m^2/s^3};\n"
		"  kW = {1e3*W};\n"
		"  MW = {1e6*W};\n"
		"END UNITS;\n"
		"UNITS LADDER\n"
		"  W = {kg*m^2/s^3};\n"
		"  kW = {1e3*W};\n"
		"  MW = {1e6*W};\n"
		"END UNITS LADDER;\n"
		"ATOM energy_rate REFINES real DIMENSION M*L^2/T^3 DEFAULT 2 {MW};\n"
		"END energy_rate;\n"
		"MODEL subm;\n"
		"  x IS_A energy_rate;\n"
		"END subm;\n"
		"MODEL top1;\n"
		"  a IS_A subm;\n"
		"END top1;\n"
		"MODEL top2;\n"
		"  b IS_A subm;\n"
		"END top2;\n";
	struct Instance *sim1 = NULL;
	struct Instance *sim2 = NULL;
	struct Instance *root1 = NULL;
	struct Instance *root2 = NULL;
	struct Instance *a = NULL;
	struct Instance *b = NULL;
	struct Instance *x1 = NULL;
	struct Instance *x2 = NULL;
	struct UnitsOverridesDB *db = NULL;
	CONST struct Units *u = NULL;
	char *scope = NULL;

	CU_ASSERT(0 == Asc_CompilerInit(1));
	/*m =*/ Asc_OpenStringModule(model,&status,"displayunits");
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();
	CU_ASSERT(has_error == 0);

	sim1 = SimsCreateInstance(AddSymbol("top1"), AddSymbol("sim1"), e_normal, NULL);
	sim2 = SimsCreateInstance(AddSymbol("top2"), AddSymbol("sim2"), e_normal, NULL);
	CU_ASSERT_PTR_NOT_NULL_FATAL(sim1);
	CU_ASSERT_PTR_NOT_NULL_FATAL(sim2);
	root1 = GetSimulationRoot(sim1);
	root2 = GetSimulationRoot(sim2);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root1);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root2);
	a = ChildByChar(root1,AddSymbol("a"));
	b = ChildByChar(root2,AddSymbol("b"));
	CU_ASSERT_PTR_NOT_NULL_FATAL(a);
	CU_ASSERT_PTR_NOT_NULL_FATAL(b);
	x1 = ChildByChar(a,AddSymbol("x"));
	x2 = ChildByChar(b,AddSymbol("x"));
	CU_ASSERT_PTR_NOT_NULL_FATAL(x1);
	CU_ASSERT_PTR_NOT_NULL_FATAL(x2);

	scope = model_scope_key(a);
	CU_ASSERT_PTR_NOT_NULL_FATAL(scope);
	db = UnitsOverridesCreate();
	CU_ASSERT_PTR_NOT_NULL_FATAL(db);
	CU_ASSERT(0 == UnitsOverridesSet(db,UNITS_OVERRIDE_TYPE,scope,"energy_rate","kW"));

	u = UnitsResolveDisplayForInstance(db,x1,1,0.1,1000.0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_ASSERT_STRING_EQUAL(SCP(UnitsDescription(u)),"kW");
	u = UnitsResolveDisplayForInstance(db,x2,1,0.1,1000.0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_ASSERT_STRING_EQUAL(SCP(UnitsDescription(u)),"kW");

	free(scope);
	UnitsOverridesDestroy(db);
	sim_destroy(sim1);
	sim_destroy(sim2);
	Asc_CompilerDestroy();
}

static void test_displayunits_set_unset_for_instance(void){
	int status = 0;
	int has_error = 0;
	CONST char *model =
		"UNITS\n"
		"  W = {kg*m^2/s^3};\n"
		"  kW = {1e3*W};\n"
		"  MW = {1e6*W};\n"
		"END UNITS;\n"
		"UNITS LADDER\n"
		"  W = {kg*m^2/s^3};\n"
		"  kW = {1e3*W};\n"
		"  MW = {1e6*W};\n"
		"END UNITS LADDER;\n"
		"ATOM energy_rate REFINES real DIMENSION M*L^2/T^3 DEFAULT 2 {MW};\n"
		"END energy_rate;\n"
		"MODEL t;\n"
		"  x IS_A energy_rate;\n"
		"  y IS_A energy_rate;\n"
		"END t;\n";
	struct Instance *sim = NULL;
	struct Instance *root = NULL;
	struct Instance *x = NULL;
	struct Instance *y = NULL;
	struct UnitsOverridesDB *db = NULL;
	CONST struct Units *u = NULL;

	CU_ASSERT(0 == Asc_CompilerInit(1));
	/*m =*/ Asc_OpenStringModule(model,&status,"displayunits");
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();
	CU_ASSERT(has_error == 0);

	sim = SimsCreateInstance(AddSymbol("t"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_PTR_NOT_NULL_FATAL(sim);
	root = GetSimulationRoot(sim);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);
	x = ChildByChar(root,AddSymbol("x"));
	y = ChildByChar(root,AddSymbol("y"));
	CU_ASSERT_PTR_NOT_NULL_FATAL(x);
	CU_ASSERT_PTR_NOT_NULL_FATAL(y);

	db = UnitsOverridesCreate();
	CU_ASSERT_PTR_NOT_NULL_FATAL(db);

	CU_ASSERT(0 == UnitsOverridesSetForInstance(
		db,x,UNITS_OVERRIDE_TYPE,0,"kW"
	));
	u = UnitsResolveDisplayForInstance(db,x,0,0.1,1000.0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_ASSERT_STRING_EQUAL(SCP(UnitsDescription(u)),"kW");
	u = UnitsResolveDisplayForInstance(db,y,0,0.1,1000.0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_ASSERT_STRING_EQUAL(SCP(UnitsDescription(u)),"kW");

	CU_ASSERT(0 == UnitsOverridesSetForInstance(
		db,x,UNITS_OVERRIDE_NAME,1,"MW"
	));
	u = UnitsResolveDisplayForInstance(db,x,0,0.1,1000.0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_ASSERT_STRING_EQUAL(SCP(UnitsDescription(u)),"MW");
	u = UnitsResolveDisplayForInstance(db,y,0,0.1,1000.0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_ASSERT_STRING_EQUAL(SCP(UnitsDescription(u)),"kW");

	CU_ASSERT(0 == UnitsOverridesUnsetForInstance(
		db,x,UNITS_OVERRIDE_NAME,1
	));
	u = UnitsResolveDisplayForInstance(db,x,0,0.1,1000.0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_ASSERT_STRING_EQUAL(SCP(UnitsDescription(u)),"kW");

	CU_ASSERT(0 == UnitsOverridesUnsetForInstance(
		db,x,UNITS_OVERRIDE_TYPE,0
	));
	u = UnitsResolveDisplayForInstance(db,x,0,0.1,1000.0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(u);
	CU_ASSERT_STRING_EQUAL(SCP(UnitsDescription(u)),"MW");

	UnitsOverridesDestroy(db);
	sim_destroy(sim);
	Asc_CompilerDestroy();
}

#define TESTS(T) \
	T(displayunits_resolve) \
	T(displayunits_model_scope_reuse) \
	T(displayunits_set_unset_for_instance)

REGISTER_TESTS_SIMPLE(compiler_displayunits, TESTS)
