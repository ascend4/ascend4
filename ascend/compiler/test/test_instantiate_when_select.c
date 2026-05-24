#include <string.h>

#include <ascend/general/env.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/error.h>

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/simlist.h>

#include <test/common.h>
#include <test/assertimpl.h>

static void instantiate_case(const char *modelname, int expect_error){
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/instantiate/when_select.a4c", &status);
	(void)m;
	CU_ASSERT(status == 0);
	CU_ASSERT(zz_parse() == 0);
	CU_ASSERT(FindType(AddSymbol(modelname)) != NULL);

	error_reporter_tree_start();
	struct Instance *sim = SimsCreateInstance(AddSymbol(modelname), AddSymbol("sim1"), e_normal, NULL);
	int has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();

	if(expect_error){
		CU_ASSERT(has_error || sim == NULL);
	}else{
		CU_ASSERT(sim != NULL);
		CU_ASSERT(!has_error);
	}

	if(sim){
		sim_destroy(sim);
	}
	Asc_CompilerDestroy();
}

static void instantiate_case_extra(const char *modelname, int expect_error){
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/instantiate/when_select_extra.a4c", &status);
	(void)m;
	CU_ASSERT(status == 0);
	CU_ASSERT(zz_parse() == 0);
	CU_ASSERT(FindType(AddSymbol(modelname)) != NULL);

	error_reporter_tree_start();
	struct Instance *sim = SimsCreateInstance(AddSymbol(modelname), AddSymbol("sim1"), e_normal, NULL);
	int has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();

	if(expect_error){
		CU_ASSERT(has_error || sim == NULL);
	}else{
		CU_ASSERT(sim != NULL);
		CU_ASSERT(!has_error);
	}

	if(sim){
		sim_destroy(sim);
	}
	Asc_CompilerDestroy();
}

static void test_when_ok_unselected_cases(void){
	instantiate_case("when_ok_unselected_cases", 0);
}

static void test_when_ok_any_case(void){
	instantiate_case("when_ok_any_case", 0);
}

static void test_when_ok_star_case(void){
	instantiate_case("when_ok_star_case", 0);
}

static void test_when_ok_leading_star_case(void){
	instantiate_case("when_ok_leading_star_case", 0);
}

static void test_when_ok_wildcard_collision_first_wins(void){
	instantiate_case("when_ok_wildcard_collision_first_wins", 0);
}

static void test_when_case_if_parses(void){
	instantiate_case("when_case_if_parses", 0);
}

static void test_when_case_if_full_boolean_parses(void){
	instantiate_case("when_case_if_full_boolean_parses", 0);
}

static void test_when_applies_if_parses(void){
	instantiate_case("when_applies_if_parses", 0);
}

static void test_when_case_if_applies_if_rejected(void){
	instantiate_case("when_case_if_applies_if_rejected", 1);
}

static void test_select_ok_unselected_cases(void){
	instantiate_case("select_ok_unselected_cases", 0);
}

static void test_when_ok_no_match(void){
	instantiate_case_extra("when_ok_no_match", 0);
}

#define TESTS(T) \
	T(when_ok_unselected_cases) \
	T(when_ok_any_case) \
	T(when_ok_star_case) \
	T(when_ok_leading_star_case) \
	T(when_ok_wildcard_collision_first_wins) \
	T(when_case_if_parses) \
	T(when_case_if_full_boolean_parses) \
	T(when_applies_if_parses) \
	T(when_case_if_applies_if_rejected) \
	T(select_ok_unselected_cases) \
	T(when_ok_no_match)

REGISTER_TESTS_SIMPLE(compiler_instantiate_when_select, TESTS)
