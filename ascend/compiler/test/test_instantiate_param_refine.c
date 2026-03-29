#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <ascend/general/env.h>
#include <ascend/general/ascMalloc.h>
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

typedef struct{
	int error_count;
	char all_error_msgs[4096];
} param_refine_error_capture_t;

static param_refine_error_capture_t g_param_refine_error_capture;

static void param_refine_error_capture_reset(void){
	memset(&g_param_refine_error_capture,0,sizeof(g_param_refine_error_capture));
}

static int param_refine_error_capture_cb(ERROR_REPORTER_CALLBACK_ARGS){
	char msg[1024];
	va_list args_copy;
	size_t used;
	int wrote_default;

	va_copy(args_copy,args);
	vsnprintf(msg,sizeof(msg),fmt,args_copy);
	va_end(args_copy);

	if(sev & ASC_ERR_ERR){
		g_param_refine_error_capture.error_count++;
		used = strlen(g_param_refine_error_capture.all_error_msgs);
		if(used + 2 < sizeof(g_param_refine_error_capture.all_error_msgs)){
			if(used > 0){
				snprintf(
					g_param_refine_error_capture.all_error_msgs + used
					,sizeof(g_param_refine_error_capture.all_error_msgs) - used
					,"\n"
				);
				used = strlen(g_param_refine_error_capture.all_error_msgs);
			}
			snprintf(
				g_param_refine_error_capture.all_error_msgs + used
				,sizeof(g_param_refine_error_capture.all_error_msgs) - used
				,"%s",msg
			);
		}
	}

	va_copy(args_copy,args);
	wrote_default = error_reporter_default_callback(sev,filename,line,funcname,fmt,args_copy);
	va_end(args_copy);
	return wrote_default;
}

static void instantiate_case(const char *modelname, int expect_error, const char *expected_message_substr, const char *expected_source_substr){
	struct module_t *m;
	struct Instance *sim;
	int status;
	int has_error;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/instantiate/param_refine_nested.a4c", &status);
	(void)m;
	CU_ASSERT(status == 0);
	CU_ASSERT(zz_parse() == 0);
	CU_ASSERT(FindType(AddSymbol(modelname)) != NULL);

	param_refine_error_capture_reset();
	error_reporter_set_callback(&param_refine_error_capture_cb);
	error_reporter_tree_start();
	sim = SimsCreateInstance(AddSymbol(modelname), AddSymbol("sim1"), e_normal, NULL);
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();
	error_reporter_set_callback(NULL);

	if(expect_error){
		CU_ASSERT(has_error || sim == NULL);
	}else{
		CU_ASSERT(sim != NULL);
		CU_ASSERT(!has_error);
	}

	if(expected_message_substr != NULL){
		CU_ASSERT(strstr(g_param_refine_error_capture.all_error_msgs,expected_message_substr) != NULL);
	}
	if(expected_source_substr != NULL){
		CU_ASSERT(strstr(g_param_refine_error_capture.all_error_msgs,expected_source_substr) != NULL);
	}

	if(sim){
		sim_destroy(sim);
	}
	Asc_CompilerDestroy();
}

static void test_param_refine_nested_direct_ok(void){
	instantiate_case("outer_direct_ok", 0, NULL, NULL);
}

static void test_param_refine_nested_refined_bug(void){
	instantiate_case("outer_refined_bug", 0, NULL, NULL);
}

static void test_param_refine_nested_where_direct_ok(void){
	instantiate_case("outer_checked_direct_ok", 0, NULL, NULL);
}

static void test_param_refine_nested_where_refined_ok(void){
	instantiate_case("outer_checked_refined_ok", 0, NULL, NULL);
}

static void test_param_refine_nested_where_refined_fail(void){
	instantiate_case(
		"outer_checked_refined_fail"
		,1
		,"Failed requirement: (n > 1)"
		,"n > 1;"
	);
}

#define TESTS(T) \
	T(param_refine_nested_direct_ok) \
	T(param_refine_nested_refined_bug) \
	T(param_refine_nested_where_direct_ok) \
	T(param_refine_nested_where_refined_ok) \
	T(param_refine_nested_where_refined_fail)

REGISTER_TESTS_SIMPLE(compiler_instantiate_param_refine, TESTS)
