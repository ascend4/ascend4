#include <stdio.h>

#include <ascend/general/env.h>
#include <ascend/general/list.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/packages.h>
#include <ascend/solver/solver.h>
#include <ascend/system/slv_client.h>

#include <test/common.h>

static void test_solver_list_lifecycle(void){
	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv"));

	/* start from a clean state */
	solver_destroy_engines();
	CONSOLE_DEBUG("solver list after destroy: len=%lu cap=%lu"
		, (unsigned long)gl_length(solver_get_engines())
		, (unsigned long)gl_capacity(solver_get_engines())
	);

	CU_ASSERT_FATAL(0 == package_load("qrslv", NULL));
	CU_ASSERT_FATAL(-1 != slv_lookup_client("QRSlv"));
	CONSOLE_DEBUG("solver list after qrslv load: len=%lu cap=%lu"
		, (unsigned long)gl_length(solver_get_engines())
		, (unsigned long)gl_capacity(solver_get_engines())
	);
	CU_ASSERT(gl_length(solver_get_engines()) >= 1);

	/* destroy and re-register */
	solver_destroy_engines();
	CONSOLE_DEBUG("solver list after destroy #2: len=%lu cap=%lu"
		, (unsigned long)gl_length(solver_get_engines())
		, (unsigned long)gl_capacity(solver_get_engines())
	);

	CU_ASSERT_FATAL(0 == package_load("qrslv", NULL));
	CU_ASSERT_FATAL(-1 != slv_lookup_client("QRSlv"));
	CONSOLE_DEBUG("solver list after qrslv reload: len=%lu cap=%lu"
		, (unsigned long)gl_length(solver_get_engines())
		, (unsigned long)gl_capacity(solver_get_engines())
	);
	CU_ASSERT(gl_length(solver_get_engines()) >= 1);

	solver_destroy_engines();
	Asc_CompilerDestroy();
}

#define TESTS(T) \
	T(solver_list_lifecycle)

REGISTER_TESTS_SIMPLE(solver_list, TESTS)
