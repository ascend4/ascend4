#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <ascend/general/env.h>
#include <ascend/general/ospath.h>

#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/error.h>

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/instance_io.h>

#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>
#include <ascend/system/slv_server.h>
#include <ascend/system/diffvars.h>
#include <ascend/system/diffvars_impl.h>

#include <test/common.h>

static slv_system_t build_system_for_model(const char *filename, const char *modelname, struct Instance **siminst_out){
	int status;
	struct Instance *siminst;
	struct Name *name;
	enum Proc_enum pe;
	slv_system_t sys;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/ida" OSPATH_DIV "solvers/lsode" OSPATH_DIV "solvers/qrslv");

	Asc_OpenModule(filename,&status);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol(modelname)) != NULL);

	siminst = SimsCreateInstance(AddSymbol(modelname), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst != NULL);

	name = CreateIdName(AddSymbol("on_load"));
	error_reporter_tree_start();
	pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	error_reporter_tree_end();
	CU_ASSERT(pe == Proc_all_ok || pe == Proc_name_not_found);

	error_reporter_tree_start();
	sys = system_build(GetSimulationRoot(siminst));
	error_reporter_tree_end();
	CU_ASSERT_FATAL(sys != NULL);

	if(siminst_out != NULL){
		*siminst_out = siminst;
	}else{
		sim_destroy(siminst);
	}
	return sys;
}

static void destroy_loaded_system(slv_system_t sys, struct Instance *siminst){
	if(sys != NULL){
		system_destroy(sys);
		system_free_reused_mem();
	}
	if(siminst != NULL){
		sim_destroy(siminst);
	}
	Asc_CompilerDestroy();
}

static void assert_diffvars_shape(slv_system_t sys, long ndiff, long nindep, short seq_len){
	SolverDiffVarCollection *diffvars = system_get_diffvars(sys);
	long i, ndiff_found = 0;
	CU_ASSERT_FATAL(diffvars != NULL);
	CU_ASSERT(diffvars->ndiff == ndiff);
	CU_ASSERT(diffvars->nindep == nindep);
	for(i = 0; i < diffvars->nseqs; ++i){
		if(diffvars->seqs[i].n > 1){
			++ndiff_found;
			CU_ASSERT(diffvars->seqs[i].n == seq_len);
		}
	}
	CU_ASSERT(ndiff_found == ndiff);
}

static void test_der_expr_direct_ok(void){
	struct Instance *siminst = NULL;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","der_expr_direct_ok",&siminst);
	assert_diffvars_shape(sys,1,1,2);
	destroy_loaded_system(sys,siminst);
}

static void test_der_expr_nested_ok(void){
	struct Instance *siminst = NULL;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","der_expr_nested_ok",&siminst);
	assert_diffvars_shape(sys,1,1,2);
	destroy_loaded_system(sys,siminst);
}

static void test_der_alias_scalar_ok(void){
	struct Instance *siminst = NULL;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","alias_der_alias_fail",&siminst);
	assert_diffvars_shape(sys,1,1,2);
	destroy_loaded_system(sys,siminst);
}

static void test_der_alias_array_known_gap(void){
	struct Instance *siminst = NULL;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","alias_der_array_alias_fail",&siminst);
	CU_ASSERT(system_get_diffvars(sys) == NULL);
	destroy_loaded_system(sys,siminst);
}

static void test_der_array_same_ok(void){
	struct Instance *siminst = NULL;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","alias_der_array_same",&siminst);
	assert_diffvars_shape(sys,3,1,2);
	destroy_loaded_system(sys,siminst);
}

#define TESTS(T) \
	T(der_expr_direct_ok) \
	T(der_expr_nested_ok) \
	T(der_alias_scalar_ok) \
	T(der_alias_array_known_gap) \
	T(der_array_same_ok)

REGISTER_TESTS_SIMPLE(system_der, TESTS)
