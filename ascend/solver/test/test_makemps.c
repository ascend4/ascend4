#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

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

#ifndef TEST_MAKEMPS_KEEP_FILES
#define TEST_MAKEMPS_KEEP_FILES 0
#endif

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

struct highs_var_expect{
	const char *ascend_substr;
	double expected;
	double tol;
};

static int highs_is_available(void){
	static int cached = -1;
	if(cached == -1){
		cached = (system("highs --version >/dev/null 2>&1") == 0) ? 1 : 0;
	}
	return cached;
}

static int run_highs(const char *mpsfile, const char *solfile){
	char cmd[1024];
	if(snprintf(cmd,sizeof(cmd)
		,"highs --model_file \"%s\" --solution_file \"%s\" >/dev/null 2>&1"
		,mpsfile,solfile
	) >= (int)sizeof(cmd)){
		return 0;
	}
	return system(cmd) == 0;
}

static int highs_solution_is_optimal(const char *solfile){
	FILE *f;
	char line[512];
	int saw_model_status = 0;

	f = fopen(solfile,"r");
	if(!f)return 0;
	while(fgets(line,sizeof(line),f)){
		if(saw_model_status){
			fclose(f);
			return strncmp(line,"Optimal",7) == 0;
		}
		if(strncmp(line,"Model status",12) == 0){
			saw_model_status = 1;
		}
	}
	fclose(f);
	return 0;
}

static int highs_solution_objective(const char *solfile, double *objective){
	FILE *f;
	char line[512];
	double v;

	f = fopen(solfile,"r");
	if(!f)return 0;
	while(fgets(line,sizeof(line),f)){
		if(sscanf(line,"Objective %lf",&v) == 1){
			*objective = v;
			fclose(f);
			return 1;
		}
	}
	fclose(f);
	return 0;
}

static int map_lookup_mps_name(const char *mapfile, const char *ascend_substr, char *mps_name, size_t mps_name_len){
	FILE *f;
	char line[1024];
	char tok[64];

	f = fopen(mapfile,"r");
	if(!f)return 0;
	while(fgets(line,sizeof(line),f)){
		if(sscanf(line,"%63s",tok) == 1 && tok[0] == 'C' && strstr(line,ascend_substr) != NULL){
			(void)snprintf(mps_name,mps_name_len,"%s",tok);
			fclose(f);
			return 1;
		}
	}
	fclose(f);
	return 0;
}

static int highs_solution_col_value(const char *solfile, const char *col_name, double *value){
	FILE *f;
	char line[512];
	char name[128];
	double v;
	int in_primal = 0;
	int in_columns = 0;

	f = fopen(solfile,"r");
	if(!f)return 0;
	while(fgets(line,sizeof(line),f)){
		if(strncmp(line,"# Primal solution values",24) == 0){
			in_primal = 1;
			continue;
		}
		if(in_primal && strncmp(line,"# Dual solution values",22) == 0){
			break;
		}
		if(in_primal && strncmp(line,"# Columns",9) == 0){
			in_columns = 1;
			continue;
		}
		if(in_primal && in_columns){
			if(line[0] == '#'){
				if(strncmp(line,"# Rows",6) == 0){
					in_columns = 0;
				}
				continue;
			}
			if(sscanf(line,"%127s %lf",name,&v) == 2 && 0 == strcmp(name,col_name)){
				*value = v;
				fclose(f);
				return 1;
			}
		}
	}
	fclose(f);
	return 0;
}

static void check_highs_solution(
	const char *mpsfile,
	const char *mapfile,
	double expected_objective,
	double objective_tol,
	const struct highs_var_expect *vars,
	int nvars
){
	char solfile[512];
	double obj = 0.0;
	int i;
	int ok = 1;

	if(!highs_is_available()){
		CONSOLE_DEBUG("Skipping HiGHS checks: 'highs' not found in PATH");
		return;
	}

	if(snprintf(solfile,sizeof(solfile),"%s.highs.sol",mpsfile) >= (int)sizeof(solfile)){
		CU_FAIL("HiGHS solution filename overflow");
		ok = 0;
		goto cleanup;
	}
	remove(solfile);

	if(!run_highs(mpsfile,solfile)){
		CU_FAIL("HiGHS execution failed");
		ok = 0;
		goto cleanup;
	}
	if(!highs_solution_is_optimal(solfile)){
		CU_FAIL("HiGHS did not report optimal status");
		ok = 0;
		goto cleanup;
	}
	if(!highs_solution_objective(solfile,&obj)){
		CU_FAIL("Unable to parse HiGHS objective");
		ok = 0;
		goto cleanup;
	}
	CONSOLE_DEBUG("HiGHS objective for %s: %.12g",mpsfile,obj);
	CU_ASSERT_DOUBLE_EQUAL(expected_objective,obj,objective_tol);
	if(fabs(expected_objective - obj) > objective_tol){
		ok = 0;
		goto cleanup;
	}

	for(i = 0; i < nvars; ++i){
		char mps_name[64];
		double value = 0.0;
		if(!map_lookup_mps_name(mapfile,vars[i].ascend_substr,mps_name,sizeof(mps_name))){
			CU_FAIL("Unable to map ASCEND variable name to MPS column name");
			ok = 0;
			break;
		}
		if(!highs_solution_col_value(solfile,mps_name,&value)){
			CU_FAIL("Unable to parse HiGHS primal column value");
			ok = 0;
			break;
		}
		CONSOLE_DEBUG("HiGHS %s (%s) = %.12g",vars[i].ascend_substr,mps_name,value);
		CU_ASSERT_DOUBLE_EQUAL(vars[i].expected,value,vars[i].tol);
		if(fabs(vars[i].expected - value) > fabs(vars[i].tol)){
			ok = 0;
			break;
		}
	}

cleanup:
	remove(solfile);
	(void)ok;
}

static void run_makemps_model(
	const char *module_path,
	const char *model_name,
	const char *mpsfile,
	const char *mapfile,
	const char *map_needle_1,
	const char *map_needle_2,
	double highs_expected_objective,
	double highs_objective_tol,
	const struct highs_var_expect *highs_vars,
	int highs_nvars,
	int run_highs_check,
	int integer_mode
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
			int integer_idx;
		slv_get_parameters(sys, &pp);
			filename_idx = find_param_index(&pp, "filename");
			nonlin_idx = find_param_index(&pp, "nonlin");
			integer_idx = find_param_index(&pp, "integer");
			CU_ASSERT_FATAL(filename_idx != -1);
			CU_ASSERT_FATAL(nonlin_idx != -1);
			CU_ASSERT_FATAL(integer_idx != -1);
			slv_set_char_parameter(&(SLV_PARAM_CHAR(&pp,filename_idx)),mpsfile);
			SLV_PARAM_BOOL(&pp,nonlin_idx) = FALSE;
			if(integer_mode >= 0){
				SLV_PARAM_INT(&pp,integer_idx) = integer_mode;
			}
			slv_set_parameters(sys, &pp);
		}

		(void)slv_presolve(sys);
		(void)slv_solve(sys);

		CU_ASSERT_FATAL(file_contains(mpsfile, "ROWS"));
	CU_ASSERT(file_contains(mpsfile, "COLUMNS"));
	CU_ASSERT(file_contains(mpsfile, "RHS"));
	CU_ASSERT(file_contains(mpsfile, "BOUNDS"));
	CU_ASSERT(file_contains(mpsfile, "ENDATA"));
	if(integer_mode == 0){
		CU_ASSERT(file_contains(mpsfile, "INTORG"));
	}
	CU_ASSERT_FATAL(file_contains(mapfile, "ASCEND/MPS Variable Name Mapping"));
	if(map_needle_1 != NULL){
		CU_ASSERT_FATAL(file_contains(mapfile, map_needle_1));
	}
	if(map_needle_2 != NULL){
		CU_ASSERT_FATAL(file_contains(mapfile, map_needle_2));
	}
	if(run_highs_check){
		check_highs_solution(
			mpsfile,
			mapfile,
			highs_expected_objective,
			highs_objective_tol,
			highs_vars,
			highs_nvars
		);
	}

cleanup:
	if(sys)system_destroy(sys);
	system_free_reused_mem();
	solver_destroy_engines();
	if(siminst)sim_destroy(siminst);
	Asc_CompilerDestroy();
#if !TEST_MAKEMPS_KEEP_FILES
	remove(mpsfile);
	remove(mapfile);
#endif
}

static void test_makemps_lp1(void){
	/* Baseline LP export: checks MPS/map files and external HiGHS solve/value mapping. */
	static const struct highs_var_expect expected_vars[] = {
		{".x", 2.0, 1e-7},
		{".y", 2.0, 1e-7},
		{".s1", 0.0, 1e-7},
		{".s2", 0.0, 1e-7}
	};
	run_makemps_model(
		"models/test/ipopt/lp1.a4c",
		"lp1",
		"test/makemps_lp1.mps",
		"test/makemps_lp1.map",
		NULL,
		NULL,
		-10.0,
		1e-7,
		expected_vars,
		4,
		1,
		-1
	);
}

static void test_makemps_lp_structured(void){
	/* Structured LP export: validates nested ASCEND variable-path to MPS name mapping. */
	static const struct highs_var_expect expected_vars[] = {
		{"x[1]", 2.0, 1e-7},
		{"x[2]", 2.0, 1e-7},
		{"row[1].s", 0.0, 1e-7},
		{"row[2].s", 0.0, 1e-7}
	};
	run_makemps_model(
		"models/test/ipopt/lp_structured.a4c",
		"lp_structured",
		"test/makemps_lp_structured.mps",
		"test/makemps_lp_structured.map",
		"x[1]",
		"row[1].s",
		-10.0,
		1e-7,
		expected_vars,
		4,
		1,
		-1
	);
}

static void test_makemps_lp_structured_table(void){
	/* Structured LP TABLE variant: same map and external HiGHS checks with TABLE constants. */
	static const struct highs_var_expect expected_vars[] = {
		{"x[1]", 2.0, 1e-7},
		{"x[2]", 2.0, 1e-7},
		{"row[1].s", 0.0, 1e-7},
		{"row[2].s", 0.0, 1e-7}
	};
	run_makemps_model(
		"models/test/ipopt/lp_structured_table.a4c",
		"lp_structured_table",
		"test/makemps_lp_structured_table.mps",
		"test/makemps_lp_structured_table.map",
		"x[1]",
		"row[1].s",
		-10.0,
		1e-7,
		expected_vars,
		4,
		1,
		-1
	);
}

static void test_makemps_mip_mixed(void){
	/* Mixed-integer export path: verifies integer-marked MPS output (INTORG markers). */
	run_makemps_model(
		"models/test/mip/mip_mixed.a4c",
		"mip_mixed",
		"test/makemps_mip_mixed.mps",
		"test/makemps_mip_mixed.map",
		".x",
		".w",
		0.0,
		1e-7,
		NULL,
		0,
		0,
		0
	);
}

static void test_makemps_mip_facility_location_table_labels(void){
	/* String-labeled TABLE facility-location: MIP export + external HiGHS regression check. */
	static const struct highs_var_expect expected_vars[] = {
		{"open['alpha']", 1.0, 1e-7},
		{"open['beta']", 0.0, 1e-7},
		{"open['gamma']", 1.0, 1e-7},
		{"assign['cust1']['alpha']", 1.0, 1e-7},
		{"assign['cust2']['gamma']", 1.0, 1e-7},
		{"assign['cust3']['gamma']", 1.0, 1e-7},
		{"assign['cust4']['gamma']", 1.0, 1e-7}
	};
	run_makemps_model(
		"models/test/mip/facility_location_table_labels.a4c",
		"mip_facility_location_table_labels",
		"test/makemps_mip_facility_location_table_labels.mps",
		"test/makemps_mip_facility_location_table_labels.map",
		"open['alpha']",
		"assign['cust1']['alpha']",
		470.0,
		1e-7,
		expected_vars,
		7,
		1,
		0
	);
}

static void test_makemps_mip_tsp_mtz8_table_labels(void){
	/* String-labeled TABLE TSP: MIP export + external HiGHS regression check. */
	static const struct highs_var_expect expected_vars[] = {
		{"x['a']['d']", 1.0, 1e-7},
		{"x['d']['b']", 1.0, 1e-7},
		{"x['b']['e']", 1.0, 1e-7},
		{"x['e']['f']", 1.0, 1e-7},
		{"x['f']['g']", 1.0, 1e-7},
		{"x['g']['c']", 1.0, 1e-7},
		{"x['c']['h']", 1.0, 1e-7},
		{"x['h']['a']", 1.0, 1e-7}
	};
	run_makemps_model(
		"models/test/mip/tsp_mtz8_table_labels.a4c",
		"mip_tsp_mtz8_table_labels",
		"test/makemps_mip_tsp_mtz8_table_labels.mps",
		"test/makemps_mip_tsp_mtz8_table_labels.map",
		"x['a']['d']",
		"x['h']['a']",
		166.0,
		1e-7,
		expected_vars,
		8,
		1,
		0
	);
}

static void test_makemps_trnsport(void){
	/* GAMS transportation LP benchmark: export then validate objective with external HiGHS. */
	run_makemps_model(
		"models/test/highs/trnsport.a4c",
		"trnsport",
		"test/makemps_trnsport.mps",
		"test/makemps_trnsport.map",
		"ship_seattle_newyork",
		"ship_sandiego_topeka",
		153.675,
		1e-7,
		NULL,
		0,
		1,
		-1
	);
}

static void test_makemps_blend_whiskas2(void){
	/* PuLP blending LP benchmark: objective plus variable mapping/values are regression checked. */
	static const struct highs_var_expect expected_vars[] = {
		{"chicken", 100.0 / 3.0, 1e-7},
		{"beef", 200.0 / 3.0, 1e-7}
	};
	run_makemps_model(
		"models/test/highs/blend_whiskas2.a4c",
		"blend_whiskas2",
		"test/makemps_blend_whiskas2.mps",
		"test/makemps_blend_whiskas2.map",
		"chicken",
		"beef",
		0.9666666666666667,
		1e-7,
		expected_vars,
		2,
		1,
		-1
	);
}

static void test_makemps_afiro(void){
	/* Netlib AFIRO LP benchmark: regression check for parser/compile/export on denser LP data. */
	run_makemps_model(
		"models/test/highs/afiro.a4c",
		"afiro",
		"test/makemps_afiro.mps",
		"test/makemps_afiro.map",
		"x02",
		"x39",
		-464.7531428571429,
		1e-5,
		NULL,
		0,
		0,
		-1
	);
}

static void test_makemps_rejects_nonlinear_default(void){
	/* Nonlinear model with nonlin=FALSE: must be rejected before writing MPS output. */
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
	/* Nonlinear model with nonlin=TRUE: linearization path should produce export output. */
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
	T(makemps_lp_structured_table) \
	T(makemps_mip_mixed) \
	T(makemps_mip_facility_location_table_labels) \
	T(makemps_mip_tsp_mtz8_table_labels) \
	T(makemps_trnsport) \
	T(makemps_blend_whiskas2) \
	T(makemps_afiro) \
	T(makemps_rejects_nonlinear_default) \
	T(makemps_linearises_nonlinear_when_enabled)

REGISTER_TESTS_SIMPLE(solver_makemps, TESTS)
