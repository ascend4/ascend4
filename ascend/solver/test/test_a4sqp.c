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

static int a4sqp_find_var_by_value_tol(const struct A4SqpView *view, real64 value, real64 tol){
	int32 i;
	for(i = 0; i < view->n_var; ++i){
		if(fabs(view->var_value[i] - value) < tol){
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

static real64 a4sqp_qp_a_value(const struct A4SqpQp *qp, int32 row, int32 col){
	int32 k;
	for(k = qp->a_start[col]; k < qp->a_start[col + 1]; ++k){
		if(qp->a_index[k] == row){
			return qp->a_value[k];
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

	CU_ASSERT_DOUBLE_EQUAL(view->obj_value,4.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->obj_gradient[xcol],0.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->obj_gradient[ycol],4.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->scaled_obj_gradient[xcol],0.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(view->scaled_obj_gradient[ycol],16.0,1e-9);

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

	slv_set_char_parameter(&(SLV_PARAM_CHAR(&params,scale_idx)),"ROW_2NORM");
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

	CU_ASSERT_FATAL(0 == slv_iterate(sys));
	CU_ASSERT_EQUAL(a4sys->qp.num_step_col,2);
	CU_ASSERT_EQUAL(a4sys->qp.num_elastic_pair,3);
	CU_ASSERT_EQUAL(a4sys->qp.num_col,8);
	CU_ASSERT_EQUAL(a4sys->qp.num_row,3);
	CU_ASSERT_EQUAL(a4sys->qp.num_nz,12);
	CU_ASSERT_EQUAL(a4sys->qp.q_num_nz,2);
	CU_ASSERT_EQUAL(a4sys->qp.highs_status,0);
	CU_ASSERT_EQUAL(a4sys->qp.highs_model_status,7);

	CU_ASSERT_EQUAL(a4sys->qp.col_kind[xcol],A4SQP_QP_COL_STEP);
	CU_ASSERT_EQUAL(a4sys->qp.col_kind[ycol],A4SQP_QP_COL_STEP);
	CU_ASSERT_EQUAL(a4sys->qp.col_var_index[xcol],xcol);
	CU_ASSERT_EQUAL(a4sys->qp.col_var_index[ycol],ycol);
	CU_ASSERT_DOUBLE_EQUAL(a4sys->qp.col_lower[xcol],-5.5,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sys->qp.col_upper[xcol],4.5,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sys->qp.col_lower[ycol],-3.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sys->qp.col_upper[ycol],2.0,1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sys->qp.col_cost[xcol],0.0,1e-12);
	CU_ASSERT_DOUBLE_EQUAL(a4sys->qp.col_cost[ycol],16.0,1e-9);

	CU_ASSERT_DOUBLE_EQUAL(a4sys->qp.row_lower[eqrow],0.0,1e-12);
	CU_ASSERT_DOUBLE_EQUAL(a4sys->qp.row_upper[eqrow],0.0,1e-12);
	CU_ASSERT_DOUBLE_EQUAL(a4sys->qp.row_lower[lerow],var_NO_LOWER_BOUND,0.0);
	CU_ASSERT_DOUBLE_EQUAL(a4sys->qp.row_upper[lerow],5.0 / sqrt(20.0),1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sys->qp.row_lower[gerow],-4.0 / sqrt(20.0),1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sys->qp.row_upper[gerow],var_NO_UPPER_BOUND,0.0);

	CU_ASSERT_DOUBLE_EQUAL(a4sqp_qp_a_value(&a4sys->qp,eqrow,xcol),2.0 / sqrt(68.0),1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sqp_qp_a_value(&a4sys->qp,eqrow,ycol),8.0 / sqrt(68.0),1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sqp_qp_a_value(&a4sys->qp,lerow,xcol),2.0 / sqrt(20.0),1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sqp_qp_a_value(&a4sys->qp,lerow,ycol),-4.0 / sqrt(20.0),1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sqp_qp_a_value(&a4sys->qp,gerow,xcol),2.0 / sqrt(20.0),1e-9);
	CU_ASSERT_DOUBLE_EQUAL(a4sqp_qp_a_value(&a4sys->qp,gerow,ycol),4.0 / sqrt(20.0),1e-9);

	CU_ASSERT_EQUAL(a4sys->qp.col_kind[view->n_var + 2 * eqrow],A4SQP_QP_COL_ELASTIC_LOWER);
	CU_ASSERT_EQUAL(a4sys->qp.col_kind[view->n_var + 2 * eqrow + 1],A4SQP_QP_COL_ELASTIC_UPPER);
	CU_ASSERT_DOUBLE_EQUAL(a4sys->qp.col_cost[view->n_var + 2 * eqrow],A4SQP_QP_DEFAULT_ELASTIC_PENALTY,1e-12);
	CU_ASSERT_DOUBLE_EQUAL(a4sys->qp.col_cost[view->n_var + 2 * eqrow + 1],A4SQP_QP_DEFAULT_ELASTIC_PENALTY,1e-12);
	CU_ASSERT_DOUBLE_EQUAL(a4sqp_qp_a_value(&a4sys->qp,eqrow,view->n_var + 2 * eqrow),1.0,1e-12);
	CU_ASSERT_DOUBLE_EQUAL(a4sqp_qp_a_value(&a4sys->qp,eqrow,view->n_var + 2 * eqrow + 1),-1.0,1e-12);

	CU_ASSERT(a4sys->qp.col_value[xcol] > 0.1);
	CU_ASSERT(a4sys->qp.col_value[ycol] < -0.1);
	CU_ASSERT_DOUBLE_EQUAL(a4sys->qp.col_value[view->n_var + 2 * eqrow],0.0,1e-8);
	CU_ASSERT_DOUBLE_EQUAL(a4sys->qp.row_value[eqrow],0.0,1e-8);

	CU_ASSERT(a4sys->last_alpha > 0.0);
	CU_ASSERT(a4sys->last_alpha <= 1.0);
	CU_ASSERT(a4sys->last_step_norm > 0.0);
	CU_ASSERT(a4sys->last_predicted_reduction > 0.0);
	CU_ASSERT(a4sys->last_model_merit_after < a4sys->last_merit_before);
	CU_ASSERT(a4sys->last_linearized_violation >= 0.0);
	CU_ASSERT(a4sys->last_merit_after < a4sys->last_merit_before);
	CU_ASSERT(a4sys->last_merit_before - a4sys->last_merit_after
		>= SLV_PARAM_REAL(&a4sys->params,A4SQP_PARAM_ARMIJO_COEFF)
			* a4sys->last_alpha * a4sys->last_predicted_reduction - 1e-9);
	CU_ASSERT_EQUAL(a4sys->line_search_failed,0);
	CU_ASSERT(a4sys->view.obj_value < 4.0);

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

static void test_a4sqp_basic_solve(void){
	int status;
	int solver_index = -1;
	struct Instance *siminst = NULL;
	slv_system_t sys = NULL;
	slv_status_t slvstatus;
	struct Name *name = NULL;
	enum Proc_enum pe;
	struct A4SqpSystem *a4sys = NULL;
	const struct A4SqpView *view = NULL;
	int xcol;
	int ycol;
	int eqrow;

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

	siminst = SimsCreateInstance(AddSymbol("a4sqp_basic_view"), AddSymbol("sim_solve"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst != NULL);

	name = CreateIdName(AddSymbol("on_load"));
	pe = Initialize(GetSimulationRoot(siminst),name,"sim_solve", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe == Proc_all_ok);

	sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);
	CU_ASSERT_FATAL(slv_select_solver(sys,solver_index) != -1);
	CU_ASSERT_FATAL(0 == slv_solve(sys));

	slv_get_status(sys,&slvstatus);
	CU_ASSERT(slvstatus.converged);
	CU_ASSERT(!slvstatus.diverged);
	CU_ASSERT(!slvstatus.iteration_limit_exceeded);
	CU_ASSERT(slvstatus.iteration > 0);
	CU_ASSERT(slvstatus.iteration <= 20);

	a4sys = (struct A4SqpSystem *)slv_get_client_token(sys);
	CU_ASSERT_PTR_NOT_NULL_FATAL(a4sys);
	view = &a4sys->view;
	xcol = a4sqp_find_var_by_value_tol(view,1.8,1e-3);
	ycol = a4sqp_find_var_by_value_tol(view,1.6,1e-3);
	eqrow = a4sqp_find_rel_by_residual(view,e_rel_equal,0.0);
	CU_ASSERT_FATAL(xcol != -1);
	CU_ASSERT_FATAL(ycol != -1);
	CU_ASSERT_FATAL(eqrow != -1);
	CU_ASSERT_DOUBLE_EQUAL(view->obj_value,3.2,1e-3);
	CU_ASSERT_DOUBLE_EQUAL(view->rel_residual[eqrow],0.0,1e-7);
	CU_ASSERT(a4sys->last_violation_max <= SLV_PARAM_REAL(&a4sys->params,A4SQP_PARAM_FEAS_TOL));
	CU_ASSERT(a4sys->last_step_norm <= SLV_PARAM_REAL(&a4sys->params,A4SQP_PARAM_STEP_TOL));

cleanup:
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

static void test_a4sqp_objective_only_iterate_contract(void){
	int status;
	int solver_index = -1;
	int res = 0;
	int spin = 0;
	struct Instance *siminst = NULL;
	slv_system_t sys = NULL;
	slv_status_t slvstatus;
	slv_parameters_t params;
	struct Name *name = NULL;
	enum Proc_enum pe;
	struct A4SqpSystem *a4sys = NULL;
	int max_iter_idx = -1;

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

	Asc_OpenModule("test/a4sqp/hs3.a4c",&status);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("hs3")) != NULL);

	siminst = SimsCreateInstance(AddSymbol("hs3"), AddSymbol("sim_hs3"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst != NULL);

	name = CreateIdName(AddSymbol("on_load_test"));
	pe = Initialize(GetSimulationRoot(siminst),name,"sim_hs3", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe == Proc_all_ok);

	sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);
	CU_ASSERT_FATAL(slv_select_solver(sys,solver_index) != -1);
	slv_get_parameters(sys,&params);
	max_iter_idx = find_param_index(&params,"max_iter");
	CU_ASSERT_FATAL(max_iter_idx != -1);
	SLV_PARAM_INT(&params,max_iter_idx) = 100;
	slv_set_parameters(sys,&params);
	CU_ASSERT_FATAL(0 == slv_presolve(sys));

	slv_get_status(sys,&slvstatus);
	CU_ASSERT(slvstatus.ready_to_solve);
	while(slvstatus.ready_to_solve && spin < 100){
		res = slv_iterate(sys);
		CU_ASSERT_EQUAL(res,0);
		slv_get_status(sys,&slvstatus);
		++spin;
	}

	CU_ASSERT_FATAL(spin < 100);
	CU_ASSERT(!slvstatus.ready_to_solve);
	CU_ASSERT(slvstatus.converged);
	CU_ASSERT(!slvstatus.iteration_limit_exceeded);
	CU_ASSERT(!slvstatus.diverged);
	CU_ASSERT(slvstatus.iteration > 0);
	CU_ASSERT(slvstatus.iteration <= 100);
	CU_ASSERT_EQUAL(slvstatus.block.number_of,1);
	CU_ASSERT(slvstatus.block.current_size >= 0);

	a4sys = (struct A4SqpSystem *)slv_get_client_token(sys);
	CU_ASSERT_PTR_NOT_NULL_FATAL(a4sys);
	CU_ASSERT_EQUAL(a4sys->view.n_rel,0);
	CU_ASSERT_PTR_NOT_NULL(a4sys->view.obj);
	CU_ASSERT(fabs(a4sys->view.var_value[0]) <= 1e-2);
	CU_ASSERT(a4sys->view.var_value[1] >= -1e-8);
	CU_ASSERT(a4sys->view.var_value[1] <= 1e-8);
	CU_ASSERT(a4sys->view.obj_value <= 1e-8);

	name = CreateIdName(AddSymbol("self_test"));
	pe = Initialize(GetSimulationRoot(siminst),name,"sim_hs3", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe == Proc_all_ok);

cleanup:
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

static void test_a4sqp_hs11_solve(void){
	int status;
	int solver_index = -1;
	struct Instance *siminst = NULL;
	slv_system_t sys = NULL;
	slv_status_t slvstatus;
	struct Name *name = NULL;
	enum Proc_enum pe;

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

	Asc_OpenModule("test/a4sqp/hs11.a4c",&status);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("hs11")) != NULL);

	siminst = SimsCreateInstance(AddSymbol("hs11"), AddSymbol("sim_hs11"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst != NULL);

	name = CreateIdName(AddSymbol("on_load_test"));
	pe = Initialize(GetSimulationRoot(siminst),name,"sim_hs11", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe == Proc_all_ok);

	sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);
	CU_ASSERT_FATAL(slv_select_solver(sys,solver_index) != -1);
	CU_ASSERT_FATAL(0 == slv_solve(sys));

	slv_get_status(sys,&slvstatus);
	CU_ASSERT(slvstatus.converged);
	CU_ASSERT(!slvstatus.diverged);
	CU_ASSERT(!slvstatus.iteration_limit_exceeded);
	CU_ASSERT(slvstatus.iteration > 0);
	CU_ASSERT(slvstatus.iteration <= 20);

	name = CreateIdName(AddSymbol("self_test"));
	pe = Initialize(GetSimulationRoot(siminst),name,"sim_hs11", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe == Proc_all_ok);

cleanup:
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

static void test_a4sqp_hs21_solve(void){
	int status;
	int solver_index = -1;
	struct Instance *siminst = NULL;
	slv_system_t sys = NULL;
	slv_status_t slvstatus;
	struct Name *name = NULL;
	enum Proc_enum pe;

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

	Asc_OpenModule("test/a4sqp/hs21.a4c",&status);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("hs21")) != NULL);

	siminst = SimsCreateInstance(AddSymbol("hs21"), AddSymbol("sim_hs21"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst != NULL);

	name = CreateIdName(AddSymbol("on_load_test"));
	pe = Initialize(GetSimulationRoot(siminst),name,"sim_hs21", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe == Proc_all_ok);

	sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);
	CU_ASSERT_FATAL(slv_select_solver(sys,solver_index) != -1);
	CU_ASSERT_FATAL(0 == slv_solve(sys));

	slv_get_status(sys,&slvstatus);
	CU_ASSERT(slvstatus.converged);
	CU_ASSERT(!slvstatus.diverged);
	CU_ASSERT(!slvstatus.iteration_limit_exceeded);
	CU_ASSERT(slvstatus.iteration > 0);
	CU_ASSERT(slvstatus.iteration <= 20);

	name = CreateIdName(AddSymbol("self_test"));
	pe = Initialize(GetSimulationRoot(siminst),name,"sim_hs21", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe == Proc_all_ok);

cleanup:
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
	T(a4sqp_basic_view_presolve) \
	T(a4sqp_basic_solve) \
	T(a4sqp_objective_only_iterate_contract) \
	T(a4sqp_hs11_solve) \
	T(a4sqp_hs21_solve)

REGISTER_TESTS_SIMPLE(solver_a4sqp, TESTS)
