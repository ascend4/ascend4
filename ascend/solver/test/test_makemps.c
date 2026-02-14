#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <ascend/general/env.h>
#include <ascend/general/ospath.h>

#include <ascend/utilities/ascEnvVar.h>

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/packages.h>

#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>
#include <ascend/system/slv_param.h>
#include <ascend/solver/solver.h>

#include <test/common.h>

static int find_param_index(const slv_parameters_t *pp, const char *name){
	int i;
	for(i=0; i<pp->num_parms; ++i){
		if(pp->parms[i].name != NULL && 0 == strcmp(pp->parms[i].name, name)){
			return i;
		}
	}
	return -1;
}

static int file_contains(const char *path, const char *needle){
	FILE *f = fopen(path, "rb");
	char *buf;
	long len;
	int found;

	if(!f)return 0;
	if(0 != fseek(f,0,SEEK_END)){
		fclose(f);
		return 0;
	}
	len = ftell(f);
	if(len < 0){
		fclose(f);
		return 0;
	}
	rewind(f);
	buf = (char *)malloc((size_t)len + 1);
	if(!buf){
		fclose(f);
		return 0;
	}
	if((size_t)len != fread(buf,1,(size_t)len,f)){
		free(buf);
		fclose(f);
		return 0;
	}
	buf[len] = '\0';
	found = strstr(buf, needle) != NULL;
	free(buf);
	fclose(f);
	return found;
}

static void run_makemps_model(
	const char *module_path,
	const char *model_name,
	const char *mpsfile,
	const char *mapfile,
	const char *map_needle_1,
	const char *map_needle_2
){
	int solver_index = -1;
	struct Instance *siminst = NULL;
	slv_system_t sys = NULL;

	remove(mpsfile);
	remove(mapfile);

	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/makemps"));

	if(0 != package_load("makemps",NULL)){
		CONSOLE_DEBUG("Skipping MakeMPS test: solver not available");
		goto cleanup;
	}
	solver_index = slv_lookup_client("MakeMPS");
	if(solver_index == -1){
		CONSOLE_DEBUG("Skipping MakeMPS test: solver not registered");
		goto cleanup;
	}

	{
		int status;
		Asc_OpenModule(module_path,&status);
		CU_ASSERT_FATAL(status == 0);
	}
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol(model_name)) != NULL);

	siminst = SimsCreateInstance(AddSymbol(model_name), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst != NULL);

	{
		struct Name *name = CreateIdName(AddSymbol("on_load"));
		enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
		CU_ASSERT(pe == Proc_all_ok);
	}

	sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);
	CU_ASSERT_FATAL(slv_select_solver(sys,solver_index) != -1);

		/* Configure export filename and keep strict linear mode. */
		{
			slv_parameters_t pp;
			int filename_idx;
			int nonlin_idx;
		slv_get_parameters(sys, &pp);
			filename_idx = find_param_index(&pp, "filename");
			nonlin_idx = find_param_index(&pp, "nonlin");
			CU_ASSERT_FATAL(filename_idx != -1);
			CU_ASSERT_FATAL(nonlin_idx != -1);
			slv_set_char_parameter(&(SLV_PARAM_CHAR(&pp,filename_idx)),mpsfile);
			SLV_PARAM_BOOL(&pp,nonlin_idx) = FALSE;
			slv_set_parameters(sys, &pp);
		}

		(void)slv_presolve(sys);
		(void)slv_solve(sys);

		CU_ASSERT_FATAL(file_contains(mpsfile, "ROWS"));
	CU_ASSERT(file_contains(mpsfile, "COLUMNS"));
	CU_ASSERT(file_contains(mpsfile, "RHS"));
	CU_ASSERT(file_contains(mpsfile, "BOUNDS"));
	CU_ASSERT(file_contains(mpsfile, "ENDATA"));
	CU_ASSERT_FATAL(file_contains(mapfile, "ASCEND/MPS Variable Name Mapping"));
	if(map_needle_1 != NULL){
		CU_ASSERT_FATAL(file_contains(mapfile, map_needle_1));
	}
	if(map_needle_2 != NULL){
		CU_ASSERT_FATAL(file_contains(mapfile, map_needle_2));
	}

cleanup:
	if(sys)system_destroy(sys);
	system_free_reused_mem();
	solver_destroy_engines();
	if(siminst)sim_destroy(siminst);
	Asc_CompilerDestroy();
	remove(mpsfile);
	remove(mapfile);
}

static void test_makemps_lp1(void){
	run_makemps_model(
		"models/test/ipopt/lp1.a4c",
		"lp1",
		"test/makemps_lp1.mps",
		"test/makemps_lp1.map",
		NULL,
		NULL
	);
}

static void test_makemps_lp_structured(void){
	run_makemps_model(
		"models/test/ipopt/lp_structured.a4c",
		"lp_structured",
		"test/makemps_lp_structured.mps",
		"test/makemps_lp_structured.map",
		"x[1]",
		"row[1].s"
	);
}

static void test_makemps_rejects_nonlinear_default(void){
	int solver_index = -1;
	struct Instance *siminst = NULL;
	slv_system_t sys = NULL;
	slv_status_t status;
	const char *mpsfile = "test/makemps_nonlinear_default.mps";
	const char *mapfile = "test/makemps_nonlinear_default.map";

	remove(mpsfile);
	remove(mapfile);

	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/makemps"));

	if(0 != package_load("makemps",NULL)){
		CONSOLE_DEBUG("Skipping MakeMPS nonlinear test: solver not available");
		goto cleanup;
	}
	solver_index = slv_lookup_client("MakeMPS");
	if(solver_index == -1){
		CONSOLE_DEBUG("Skipping MakeMPS nonlinear test: solver not registered");
		goto cleanup;
	}

	{
		int status_open;
		Asc_OpenModule("models/test/ipopt/test11.a4c",&status_open);
		CU_ASSERT_FATAL(status_open == 0);
	}
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("test11")) != NULL);

	siminst = SimsCreateInstance(AddSymbol("test11"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst != NULL);

	{
		struct Name *name = CreateIdName(AddSymbol("on_load"));
		enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
		CU_ASSERT(pe == Proc_all_ok);
	}

	sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);
	CU_ASSERT_FATAL(slv_select_solver(sys,solver_index) != -1);

	{
		slv_parameters_t pp;
		int filename_idx;
		int nonlin_idx;
		slv_get_parameters(sys, &pp);
		filename_idx = find_param_index(&pp, "filename");
		nonlin_idx = find_param_index(&pp, "nonlin");
		CU_ASSERT_FATAL(filename_idx != -1);
		CU_ASSERT_FATAL(nonlin_idx != -1);
		slv_set_char_parameter(&(SLV_PARAM_CHAR(&pp,filename_idx)),mpsfile);
		SLV_PARAM_BOOL(&pp,nonlin_idx) = FALSE;
		slv_set_parameters(sys, &pp);
	}

	(void)slv_presolve(sys);
	slv_get_status(sys,&status);
	CU_ASSERT_FALSE(status.ready_to_solve);

	(void)slv_solve(sys);
	CU_ASSERT_FALSE(file_contains(mpsfile, "ROWS"));
	CU_ASSERT_FALSE(file_contains(mapfile, "ASCEND/MPS Variable Name Mapping"));

cleanup:
	if(sys)system_destroy(sys);
	system_free_reused_mem();
	solver_destroy_engines();
	if(siminst)sim_destroy(siminst);
	Asc_CompilerDestroy();
	remove(mpsfile);
	remove(mapfile);
}

static void test_makemps_linearises_nonlinear_when_enabled(void){
	int solver_index = -1;
	struct Instance *siminst = NULL;
	slv_system_t sys = NULL;
	slv_status_t status;
	const char *mpsfile = "test/makemps_nonlinear_enabled.mps";
	const char *mapfile = "test/makemps_nonlinear_enabled.map";

	remove(mpsfile);
	remove(mapfile);

	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/makemps"));

	if(0 != package_load("makemps",NULL)){
		CONSOLE_DEBUG("Skipping MakeMPS nonlinear-enabled test: solver not available");
		goto cleanup;
	}
	solver_index = slv_lookup_client("MakeMPS");
	if(solver_index == -1){
		CONSOLE_DEBUG("Skipping MakeMPS nonlinear-enabled test: solver not registered");
		goto cleanup;
	}

	{
		int status_open;
		Asc_OpenModule("models/test/ipopt/test11.a4c",&status_open);
		CU_ASSERT_FATAL(status_open == 0);
	}
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("test11")) != NULL);

	siminst = SimsCreateInstance(AddSymbol("test11"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst != NULL);

	{
		struct Name *name = CreateIdName(AddSymbol("on_load"));
		enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
		CU_ASSERT(pe == Proc_all_ok);
	}

	sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);
	CU_ASSERT_FATAL(slv_select_solver(sys,solver_index) != -1);

	{
		slv_parameters_t pp;
		int filename_idx;
		int nonlin_idx;
		slv_get_parameters(sys, &pp);
		filename_idx = find_param_index(&pp, "filename");
		nonlin_idx = find_param_index(&pp, "nonlin");
		CU_ASSERT_FATAL(filename_idx != -1);
		CU_ASSERT_FATAL(nonlin_idx != -1);
		slv_set_char_parameter(&(SLV_PARAM_CHAR(&pp,filename_idx)),mpsfile);
		SLV_PARAM_BOOL(&pp,nonlin_idx) = TRUE;
		slv_set_parameters(sys, &pp);
	}

	(void)slv_presolve(sys);
	slv_get_status(sys,&status);
	CU_ASSERT_TRUE(status.ready_to_solve);

	(void)slv_solve(sys);
	CU_ASSERT_TRUE(file_contains(mpsfile, "ROWS"));
	CU_ASSERT_TRUE(file_contains(mpsfile, "COLUMNS"));
	CU_ASSERT_TRUE(file_contains(mpsfile, "RHS"));
	CU_ASSERT_TRUE(file_contains(mpsfile, "BOUNDS"));
	CU_ASSERT_TRUE(file_contains(mpsfile, "ENDATA"));
	CU_ASSERT_TRUE(file_contains(mapfile, "ASCEND/MPS Variable Name Mapping"));
	CU_ASSERT_TRUE(file_contains(mapfile, "x1"));
	CU_ASSERT_TRUE(file_contains(mapfile, "x2"));
	CU_ASSERT_TRUE(file_contains(mapfile, "x3"));

cleanup:
	if(sys)system_destroy(sys);
	system_free_reused_mem();
	solver_destroy_engines();
	if(siminst)sim_destroy(siminst);
	Asc_CompilerDestroy();
	remove(mpsfile);
	remove(mapfile);
}

#define TESTS(T) \
	T(makemps_lp1) \
	T(makemps_lp_structured) \
	T(makemps_rejects_nonlinear_default) \
	T(makemps_linearises_nonlinear_when_enabled)

REGISTER_TESTS_SIMPLE(solver_makemps, TESTS)
