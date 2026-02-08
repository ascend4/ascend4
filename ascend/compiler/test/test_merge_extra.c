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

static void compile_reject_case(const char *modulepath, const char *modelname){
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule(modulepath, &status);
	(void)m;
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(zz_parse() == 0);
	int has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();

	CU_ASSERT(has_error);
	CU_ASSERT(FindType(AddSymbol(modelname)) == NULL);

	Asc_CompilerDestroy();
}

static void test_merge_fail_relations(void){
	compile_reject_case("test/merge/merge_compile_fail_rel.a4c",
			"merge_fail_relations");
}

static void test_merge_fail_logrels(void){
	compile_reject_case("test/merge/merge_compile_fail_rel.a4c",
			"merge_fail_logrels");
}

#define TESTS(T) \
	T(merge_fail_relations) \
	T(merge_fail_logrels)

REGISTER_TESTS_SIMPLE(compiler_merge_extra, TESTS)
