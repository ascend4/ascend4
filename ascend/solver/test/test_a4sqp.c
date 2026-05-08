#include <stdio.h>
#include <string.h>
#include <math.h>

#include <ascend/general/env.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/packages.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/symtab.h>
#include <ascend/solver/solver.h>
#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>
#include <ascend/system/slv_param.h>
#include <ascend/system/var.h>
#include <ascend/utilities/ascDynaLoad.h>

#include <solvers/a4sqp/a4sqp_internal.h>
#include <solvers/a4sqp/a4sqp_qp_highs.h>

#include <test/common.h>

struct a4sqp_progress_capture {
	char buffer[1024];
	size_t len;
};

static int a4sqp_capture_progress(const char *solver_name, const char *message, void *user_data){
	struct a4sqp_progress_capture *capture = (struct a4sqp_progress_capture *)user_data;
	int wrote;
	if(capture == NULL){
		return 0;
	}
	wrote = snprintf(
		capture->buffer + capture->len,
		sizeof(capture->buffer) - capture->len,
		"%s:%s\n",
		solver_name != NULL ? solver_name : "",
		message != NULL ? message : ""
	);
	if(wrote > 0){
		capture->len += (size_t)wrote;
		if(capture->len >= sizeof(capture->buffer)){
			capture->len = sizeof(capture->buffer) - 1;
		}
	}
	return 0;
}

static int a4sqp_find_var_by_value(const struct A4SqpView *view, real64 value){
	int32 i;
	for(i = 0; i < view->n_var; ++i){
		if(fabs(view->var_value[i] - value) < 1e-9){
			return i;
		}
	}
	return -1;
}

static int a4sqp_find_rel_by_residual(
	const struct A4SqpView *view,
	enum rel_enum relop,
	real64 residual
){
	int32 i;
	for(i = 0; i < view->n_rel; ++i){
		if(view->relop[i] == relop && fabs(view->rel_residual[i] - residual) < 1e-9){
			return i;
		}
	}
	return -1;
}

static real64 a4sqp_jac_value(const struct A4SqpView *view, int32 row, int32 col_sindex){
	int32 k;
	for(k = view->jac_row_start[row]; k < view->jac_row_start[row + 1]; ++k){
		if(view->jac_col_sindex[k] == col_sindex){
			return view->jac_value[k];
		}
	}
	return 0.0;
}

static real64 a4sqp_scaled_jac_value(const struct A4SqpView *view, int32 row, int32 col_sindex){
	int32 k;
	for(k = view->jac_row_start[row]; k < view->jac_row_start[row + 1]; ++k){
		if(view->jac_col_sindex[k] == col_sindex){
			return view->scaled_jac_value[k];
		}
	}
	return 0.0;
}

static int find_param_index(const slv_parameters_t *pp, const char *name){
	int i;
	for(i=0; i<pp->num_parms; ++i){
		if(pp->parms[i].name != NULL && strcmp(pp->parms[i].name,name)==0){
			return i;
		}
	}
	return -1;
}

typedef int (*a4sqp_qp_highs_spike_fn)(struct A4SqpQpSpikeResult *);

static a4sqp_qp_highs_spike_fn a4sqp_load_qp_spike(void){
	const char *lib = "solvers/a4sqp/liba4sqp_ascend.so";
	DynamicF fn;

	if(Asc_DynamicLoad(lib,NULL) != 0){
		return NULL;
	}
	fn = Asc_DynamicFunction(lib,"a4sqp_qp_highs_spike");
	return (a4sqp_qp_highs_spike_fn)fn;
}

static void test_a4sqp_register(void){
	const SlvFunctionsT *solver;
	int solver_index;

	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/a4sqp"));

	solver_destroy_engines();
	if(0 != package_load("a4sqp",NULL)){
		CONSOLE_DEBUG("Skipping A4SQP test: solver package not available");
		goto cleanup;
	}

	solver_index = slv_lookup_client("A4SQP");
	CU_ASSERT_FATAL(solver_index != -1);

	solver = solver_engine_named("A4SQP");
	CU_ASSERT_PTR_NOT_NULL_FATAL(solver);
	CU_ASSERT_STRING_EQUAL(solver->name,"A4SQP");
	CU_ASSERT_PTR_NOT_NULL(solver->ccreate);
	CU_ASSERT_PTR_NOT_NULL(solver->cdestroy);
	CU_ASSERT_PTR_NOT_NULL(solver->presolve);
	CU_ASSERT_PTR_NOT_NULL(solver->iterate);
	CU_ASSERT_PTR_NOT_NULL(solver->solve);

cleanup:
	solver_destroy_engines();
	Asc_CompilerDestroy();
}

static void test_a4sqp_qp_highs_spike(void){
	a4sqp_qp_highs_spike_fn spike;
	struct A4SqpQpSpikeResult result;

	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/a4sqp"));

	solver_destroy_engines();
	if(0 != package_load("a4sqp",NULL)){
		CONSOLE_DEBUG("Skipping A4SQP test: solver package not available");
		goto cleanup;
	}

	spike = a4sqp_load_qp_spike();
	CU_ASSERT_PTR_NOT_NULL_FATAL(spike);
	CU_ASSERT_FATAL(0 == spike(&result));
	CU_ASSERT_EQUAL(result.highs_status,0);
	CU_ASSERT_EQUAL(result.highs_model_status,7);
	CU_ASSERT_DOUBLE_EQUAL(result.col_value[0],0.25,1e-8);
	CU_ASSERT_DOUBLE_EQUAL(result.col_value[1],0.75,1e-8);
	CU_ASSERT_DOUBLE_EQUAL(result.col_value[2],0.0,1e-8);
	CU_ASSERT_DOUBLE_EQUAL(result.row_value[0],1.0,1e-8);
	CU_ASSERT_DOUBLE_EQUAL(result.objective_value,7.53125,1e-8);

cleanup:
	(void)Asc_DynamicUnLoad("solvers/a4sqp/liba4sqp_ascend.so");
	solver_destroy_engines();
	Asc_CompilerDestroy();
}

static void test_a4sqp_basic_view_presolve(void){
	int status;
	int solver_index = -1;
	struct Instance *siminst = NULL;
	slv_system_t sys = NULL;
	slv_status_t slvstatus;
	struct Name *name = NULL;
	enum Proc_enum pe;
	struct a4sqp_progress_capture progress;
	struct A4SqpSystem *a4sys = NULL;
	const struct A4SqpView *view = NULL;
	int xcol;
	int ycol;
	int eqrow;
	int lerow;
	int gerow;
	int scale_idx;
	slv_parameters_t params;

	memset(&progress,0,sizeof(progress));

	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/a4sqp"));

	solver_destroy_engines();
	if(0 != package_load("a4sqp",NULL)){
		CONSOLE_DEBUG("Skipping A4SQP test: solver package not available");
		goto cleanup;
	}

	solver_index = slv_lookup_client("A4SQP");
	CU_ASSERT_FATAL(solver_index != -1);

	Asc_OpenModule("test/a4sqp/basic_view.a4c",&status);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("a4sqp_basic_view")) != NULL);

	siminst = SimsCreateInstance(AddSymbol("a4sqp_basic_view"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst != NULL);

	name = CreateIdName(AddSymbol("on_load"));
	pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe == Proc_all_ok);

	sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);
	CU_ASSERT_FATAL(slv_select_solver(sys,solver_index) != -1);

	slv_set_progress_callback(a4sqp_capture_progress,&progress);
	CU_ASSERT_FATAL(0 == slv_presolve(sys));
	slv_clear_progress_callback();

	slv_get_status(sys,&slvstatus);
	CU_ASSERT(slvstatus.ready_to_solve);
	CU_ASSERT(slvstatus.ok);
	CU_ASSERT(slvstatus.calc_ok);
	CU_ASSERT_STRING_NOT_EQUAL(progress.buffer,"");
	CU_ASSERT_PTR_NOT_NULL(strstr(progress.buffer,"A4SQP:view:"));
	CU_ASSERT_PTR_NOT_NULL(strstr(progress.buffer,"vars=2"));
	CU_ASSERT_PTR_NOT_NULL(strstr(progress.buffer,"objective=yes"));
	CU_ASSERT_PTR_NOT_NULL(strstr(progress.buffer,"jac_nnz="));
	CU_ASSERT_PTR_NOT_NULL(strstr(progress.buffer,"calc_errors=0"));
	CU_ASSERT_PTR_NOT_NULL(strstr(progress.buffer,"derivative_errors=0"));
	CU_ASSERT_PTR_NOT_NULL(strstr(progress.buffer,"unsupported_rels=0"));

	a4sys = (struct A4SqpSystem *)slv_get_client_token(sys);
	CU_ASSERT_PTR_NOT_NULL_FATAL(a4sys);
	view = &a4sys->view;

	CU_ASSERT_EQUAL(view->n_var,2);
	CU_ASSERT_EQUAL(view->n_rel,3);
	CU_ASSERT_PTR_NOT_NULL(view->obj);
	CU_ASSERT_EQUAL(view->calc_errors,0);
	CU_ASSERT_EQUAL(view->derivative_errors,0);
	CU_ASSERT_EQUAL(view->unsupported_rels,0);
	CU_ASSERT_EQUAL(view->jac_nnz,6);

	xcol = a4sqp_find_var_by_value(view,1.0);
	ycol = a4sqp_find_var_by_value(view,2.0);
	CU_ASSERT_FATAL(xcol != -1);
	CU_ASSERT_FATAL(ycol != -1);
	CU_ASSERT_DOUBLE_EQUAL(view->var_lower[xcol],-10.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->var_upper[xcol],10.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->var_lower[ycol],-10.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->var_upper[ycol],10.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->var_nominal[xcol],2.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->var_nominal[ycol],4.0,1e-9);
	CU_ASSERT_EQUAL(view->var_fixed[xcol],0);
	CU_ASSERT_EQUAL(view->var_fixed[ycol],0);

	eqrow = a4sqp_find_rel_by_residual(view,e_rel_equal,0.0);
	lerow = a4sqp_find_rel_by_residual(view,e_rel_lesseq,-5.0);
	gerow = a4sqp_find_rel_by_residual(view,e_rel_greatereq,4.0);
	CU_ASSERT_FATAL(eqrow != -1);
	CU_ASSERT_FATAL(lerow != -1);
	CU_ASSERT_FATAL(gerow != -1);

	CU_ASSERT_DOUBLE_EQUAL(view->rel_lower[eqrow],0.0,1e-12);
	CU_ASSERT_DOUBLE_EQUAL(view->rel_upper[eqrow],0.0,1e-12);
	CU_ASSERT_DOUBLE_EQUAL(view->rel_lower[lerow],var_NO_LOWER_BOUND,0.0);
	CU_ASSERT_DOUBLE_EQUAL(view->rel_upper[lerow],0.0,1e-12);
	CU_ASSERT_DOUBLE_EQUAL(view->rel_lower[gerow],0.0,1e-12);
	CU_ASSERT_DOUBLE_EQUAL(view->rel_upper[gerow],var_NO_UPPER_BOUND,0.0);

	CU_ASSERT_EQUAL(view->jac_row_start[0],0);
	CU_ASSERT_EQUAL(view->jac_row_start[view->n_rel],view->jac_nnz);
	CU_ASSERT_DOUBLE_EQUAL(a4sqp_jac_value(view,eqrow,view->var_sindex[xcol]),1.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sqp_jac_value(view,eqrow,view->var_sindex[ycol]),2.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sqp_jac_value(view,lerow,view->var_sindex[xcol]),1.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sqp_jac_value(view,lerow,view->var_sindex[ycol]),-1.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sqp_jac_value(view,gerow,view->var_sindex[xcol]),1.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sqp_jac_value(view,gerow,view->var_sindex[ycol]),1.0,1e-9);

	CU_ASSERT_DOUBLE_EQUAL(view->var_scale[xcol],2.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->var_scale[ycol],4.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->scaled_var_value[xcol],0.5,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->scaled_var_value[ycol],0.5,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->scaled_var_lower[xcol],-5.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->scaled_var_upper[xcol],5.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->scaled_var_lower[ycol],-2.5,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->scaled_var_upper[ycol],2.5,1e-9);

	CU_ASSERT_DOUBLE_EQUAL(view->rel_scale[eqrow],1.0 / sqrt(68.0),1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->rel_scale[lerow],1.0 / sqrt(20.0),1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->rel_scale[gerow],1.0 / sqrt(20.0),1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->scaled_rel_residual[eqrow],0.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->scaled_rel_residual[lerow],-5.0 / sqrt(20.0),1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->scaled_rel_residual[gerow],4.0 / sqrt(20.0),1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sqp_scaled_jac_value(view,eqrow,view->var_sindex[xcol]),2.0 / sqrt(68.0),1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sqp_scaled_jac_value(view,eqrow,view->var_sindex[ycol]),8.0 / sqrt(68.0),1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sqp_scaled_jac_value(view,lerow,view->var_sindex[xcol]),2.0 / sqrt(20.0),1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sqp_scaled_jac_value(view,lerow,view->var_sindex[ycol]),-4.0 / sqrt(20.0),1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sqp_scaled_jac_value(view,gerow,view->var_sindex[xcol]),2.0 / sqrt(20.0),1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sqp_scaled_jac_value(view,gerow,view->var_sindex[ycol]),4.0 / sqrt(20.0),1e-9);

	slv_get_parameters(sys,&params);
	scale_idx = find_param_index(&params,"scaleopt");
	CU_ASSERT_FATAL(scale_idx != -1);
	slv_set_char_parameter(&(SLV_PARAM_CHAR(&params,scale_idx)),"NONE");
	slv_set_parameters(sys,&params);
	CU_ASSERT_FATAL(0 == slv_presolve(sys));

	a4sys = (struct A4SqpSystem *)slv_get_client_token(sys);
	CU_ASSERT_PTR_NOT_NULL_FATAL(a4sys);
	view = &a4sys->view;
	xcol = a4sqp_find_var_by_value(view,1.0);
	ycol = a4sqp_find_var_by_value(view,2.0);
	eqrow = a4sqp_find_rel_by_residual(view,e_rel_equal,0.0);
	lerow = a4sqp_find_rel_by_residual(view,e_rel_lesseq,-5.0);
	gerow = a4sqp_find_rel_by_residual(view,e_rel_greatereq,4.0);
	CU_ASSERT_FATAL(xcol != -1);
	CU_ASSERT_FATAL(ycol != -1);
	CU_ASSERT_FATAL(eqrow != -1);
	CU_ASSERT_FATAL(lerow != -1);
	CU_ASSERT_FATAL(gerow != -1);
	CU_ASSERT_DOUBLE_EQUAL(view->var_scale[xcol],1.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->var_scale[ycol],1.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->rel_scale[eqrow],1.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->rel_scale[lerow],1.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->rel_scale[gerow],1.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->scaled_var_value[xcol],view->var_value[xcol],1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->scaled_var_value[ycol],view->var_value[ycol],1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->scaled_rel_residual[lerow],view->rel_residual[lerow],1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sqp_scaled_jac_value(view,eqrow,view->var_sindex[xcol]),1.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sqp_scaled_jac_value(view,eqrow,view->var_sindex[ycol]),2.0,1e-9);

cleanup:
	slv_clear_progress_callback();
	if(sys != NULL){
		system_destroy(sys);
		system_free_reused_mem();
	}
	if(siminst != NULL){
		sim_destroy(siminst);
	}
	solver_destroy_engines();
	Asc_CompilerDestroy();
}

#define TESTS(T) \
	T(a4sqp_register) \
	T(a4sqp_qp_highs_spike) \
	T(a4sqp_basic_view_presolve)

REGISTER_TESTS_SIMPLE(solver_a4sqp, TESTS)
