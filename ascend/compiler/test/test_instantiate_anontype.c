#include <stdio.h>
#include <string.h>

#include <ascend/general/env.h>
#include <ascend/general/platform.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/error.h>

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/anontype.h>

#include <test/common.h>
#include <test/assertimpl.h>

static void instantiate_case(const char *modelname, int expect_error){
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/instantiate/anontype.a4c", &status);
	(void)m;
	CU_ASSERT(status == 0);
	CU_ASSERT(zz_parse() == 0);
	if(!expect_error){
		CU_ASSERT(FindType(AddSymbol(modelname)) != NULL);
	}else if(FindType(AddSymbol(modelname)) == NULL){
		Asc_CompilerDestroy();
		return;
	}

	error_reporter_tree_start();
	struct Instance *sim = SimsCreateInstance(AddSymbol(modelname), AddSymbol("sim1"), e_normal, NULL);
	int has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();

	if(expect_error){
		CU_ASSERT(has_error || sim == NULL);
	}else{
		CU_ASSERT(sim != NULL);
		CU_ASSERT(!has_error);
		if(sim){
			struct Instance *root = InstanceChild(sim,1);
			if(!root){
				root = sim;
			}
			struct gl_list_t *atl = Asc_DeriveAnonList(root);
			CU_ASSERT(atl != NULL);
			if(atl){
				FILE *fp = tmpfile();
				CU_ASSERT(fp != NULL);
				if(fp){
					Asc_WriteAnonList(fp, atl, root, 0);
					fclose(fp);
				}
				Asc_DestroyAnonList(atl);
			}
		}
	}

	if(sim){
		sim_destroy(sim);
	}
	Asc_CompilerDestroy();
}

static void test_anontype_sc(void){ instantiate_case("test_sc_anon", 0); }
static void test_anontype_bc(void){ instantiate_case("test_bc_anon", 0); }
static void test_anontype_ic(void){ instantiate_case("test_ic_anon", 0); }
static void test_anontype_set(void){ instantiate_case("test_set_anon", 0); }
static void test_anontype_ai1(void){ instantiate_case("test_ai_anon1", 0); }
static void test_anontype_u(void){ instantiate_case("test_u_anon", 0); }
static void test_anontype_u2(void){ instantiate_case("test_u2_anon", 0); }
static void test_anontype_rel(void){ instantiate_case("test_rel_anon", 0); }
static void test_anontype_lrel(void){ instantiate_case("test_lrel_anon", 0); }
static void test_anontype_rel_impossible(void){ instantiate_case("test_rel_impossible_anon", 1); }
static void test_anontype_lrel_impossible(void){ instantiate_case("test_lrel_impossible_anon", 1); }
static void test_anontype_dummy(void){ instantiate_case("test_dummy_anon", 0); }
static void test_anontype_write(void){ instantiate_case("test_write_anon", 0); }

#define TESTS(T) \
	T(anontype_sc) \
	T(anontype_bc) \
	T(anontype_ic) \
	T(anontype_set) \
	T(anontype_ai1) \
	T(anontype_u) \
	T(anontype_u2) \
	T(anontype_rel) \
	T(anontype_lrel) \
	T(anontype_rel_impossible) \
	T(anontype_lrel_impossible) \
	T(anontype_dummy) \
	T(anontype_write)

REGISTER_TESTS_SIMPLE(compiler_instantiate_anontype, TESTS)
