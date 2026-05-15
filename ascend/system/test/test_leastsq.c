/*
 * Explicit least-squares objective analysis tests.
 */

#include <string.h>
#include <stdio.h>
#include <math.h>

#include <ascend/general/env.h>
#include <ascend/general/platform.h>
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
#include <ascend/compiler/mathinst.h>
#include <ascend/compiler/relation.h>
#include <ascend/compiler/relation_util.h>

#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>
#include <ascend/system/rel.h>
#include <ascend/system/lsq.h>

#include <test/common.h>

static struct Instance *leastsq_load_sim_for_model(const char *filename, const char *modelname){
	int status;
	struct Instance *siminst;
	struct Name *name;
	enum Proc_enum pe;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv");

	Asc_OpenModule(filename,&status);
	CU_ASSERT_FATAL(status == 0);

	error_reporter_tree_start();
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(0 == error_reporter_tree_has_error());
	error_reporter_tree_end();

	CU_ASSERT_FATAL(FindType(AddSymbol(modelname)) != NULL);

	siminst = SimsCreateInstance(AddSymbol(modelname), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst != NULL);

	name = CreateIdName(AddSymbol("on_load"));
	error_reporter_tree_start();
	pe = Initialize(GetSimulationRoot(siminst), name, "sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	error_reporter_tree_end();
	CU_ASSERT(pe == Proc_all_ok || pe == Proc_name_not_found);

	return siminst;
}

static slv_system_t leastsq_build_system_for_model(const char *filename, const char *modelname, struct Instance **siminst_out){
	struct Instance *siminst;
	slv_system_t sys;

	siminst = leastsq_load_sim_for_model(filename, modelname);
	CU_ASSERT_FATAL(siminst != NULL);

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

static void leastsq_destroy_system(slv_system_t sys, struct Instance *siminst){
	if(sys != NULL){
		system_destroy(sys);
		system_free_reused_mem();
	}
	if(siminst != NULL){
		sim_destroy(siminst);
	}
	Asc_CompilerDestroy();
}

static void leastsq_expect(
	const char *modulefile,
	const char *typename,
	int expected_is_lsq,
	enum RelationLeastSquaresStatus expected_status,
	unsigned long expected_residuals,
	unsigned long expected_weighted,
	const double *expected_values,
	const double *expected_weights
){
	struct Instance *sim = NULL;
	slv_system_t sys;
	struct RelationLeastSquaresAnalysis analysis;
	const struct system_lsq_view *view;
	int result;

	sys = leastsq_build_system_for_model(modulefile, typename, &sim);

	result = system_analyse_lsq_objective(sys, SYSTEM_LSQ_ANALYSE_CLASSIFY, &analysis);
	CU_ASSERT_EQUAL(result, expected_is_lsq);
	CU_ASSERT_EQUAL(analysis.is_least_squares, expected_is_lsq);
	CU_ASSERT_EQUAL(analysis.status, expected_status);
	CU_ASSERT_EQUAL(analysis.residual_count, expected_residuals);
	CU_ASSERT_EQUAL(analysis.weighted_count, expected_weighted);
	CU_ASSERT_PTR_NOT_NULL(analysis.reason);
	CU_ASSERT_PTR_NULL(system_get_lsq_view(sys));

	result = system_analyse_lsq_objective(sys, SYSTEM_LSQ_ANALYSE_BUILD_VIEW, &analysis);
	CU_ASSERT_EQUAL(result, expected_is_lsq);
	view = system_get_lsq_view(sys);
	if(expected_is_lsq){
		unsigned long i;
		double values[8];
		double single;
		CU_ASSERT_PTR_NOT_NULL_FATAL(view);
		CU_ASSERT_EQUAL(view->valid, 1);
		CU_ASSERT_EQUAL(view->nresiduals, expected_residuals);
		CU_ASSERT_EQUAL(view->analysis.residual_count, expected_residuals);
		CU_ASSERT_PTR_NOT_NULL_FATAL(view->residuals);
		CU_ASSERT(expected_residuals <= sizeof(values)/sizeof(values[0]));
		CU_ASSERT_EQUAL(system_lsq_eval_residuals(sys, values), 0);
		for(i = 0; i < view->nresiduals; ++i){
			CU_ASSERT_PTR_NOT_NULL(view->residuals[i].objective);
			CU_ASSERT_PTR_NOT_NULL(view->residuals[i].residual_term);
			CU_ASSERT(view->residuals[i].weight > 0.0);
			if(expected_weights != NULL){
				CU_ASSERT_DOUBLE_EQUAL(view->residuals[i].weight, expected_weights[i], 1e-12);
			}
			CU_ASSERT_EQUAL(system_lsq_eval_residual(sys, i, &single), 0);
			CU_ASSERT_DOUBLE_EQUAL(values[i], single, 1e-12);
			if(expected_values != NULL){
				CU_ASSERT_DOUBLE_EQUAL(values[i], expected_values[i], 1e-12);
				{
					int columns[4];
					double jac_values[4];
					unsigned long nnz = 0;
					CU_ASSERT_EQUAL(system_lsq_eval_jacobian_row(sys, i, NULL, NULL, 0, &nnz), 0);
					CU_ASSERT_EQUAL(nnz, 1);
					CU_ASSERT_EQUAL(system_lsq_eval_jacobian_row(sys, i, columns, jac_values, 4, &nnz), 0);
					CU_ASSERT_EQUAL(nnz, 1);
					CU_ASSERT(columns[0] >= 0);
					CU_ASSERT_DOUBLE_EQUAL(jac_values[0], 1.0, 1e-12);
				}
			}else{
				int columns[16];
				double jac_values[16];
				unsigned long nnz = 0;
				unsigned long j;
				CU_ASSERT(isfinite(values[i]));
				CU_ASSERT_EQUAL(system_lsq_eval_jacobian_row(sys, i, NULL, NULL, 0, &nnz), 0);
				CU_ASSERT(nnz > 0);
				if(nnz <= 16){
					CU_ASSERT_EQUAL(system_lsq_eval_jacobian_row(sys, i, columns, jac_values, 16, &nnz), 0);
					for(j = 0; j < nnz; ++j){
						CU_ASSERT(columns[j] >= 0);
						CU_ASSERT(isfinite(jac_values[j]));
					}
				}
			}
		}
	}else{
		CU_ASSERT_PTR_NULL(view);
	}

	leastsq_destroy_system(sys, sim);
}

static void test_basic(void){
	const double values[] = {1.0, 2.0, 3.0};
	const double weights[] = {1.0, 1.0, 1.0};
	leastsq_expect("test/leastsq/patterns.a4c", "lsq_basic", 1, rel_lsq_ok, 3, 0, values, weights);
}

static void test_weighted(void){
	const double values[] = {1.0, 2.0, 3.0};
	const double weights[] = {0.5, 2.0, 4.0};
	leastsq_expect("test/leastsq/patterns.a4c", "lsq_weighted", 1, rel_lsq_ok, 3, 3, values, weights);
}

static void test_divided_square(void){
	const double values[] = {1.0, 2.0, 3.0};
	const double weights[] = {2.0, 0.5, 0.25};
	leastsq_expect("test/leastsq/patterns.a4c", "lsq_divided_square", 1, rel_lsq_ok, 3, 3, values, weights);
}

static void test_nested_sum(void){
	const double values[] = {1.0, 2.0, 3.0, 4.0};
	const double weights[] = {0.5, 1.0, 2.0, 4.0};
	leastsq_expect("test/leastsq/patterns.a4c", "lsq_nested_sum", 1, rel_lsq_ok, 4, 4, values, weights);
}

static void test_negative_weight(void){
	leastsq_expect("test/leastsq/patterns.a4c", "lsq_negative_weight", 0, rel_lsq_nonpositive_weight, 1, 1, NULL, NULL);
}

static void test_variable_weight(void){
	leastsq_expect("test/leastsq/patterns.a4c", "lsq_variable_weight", 0, rel_lsq_variable_weight, 0, 0, NULL, NULL);
}

static void test_linear_extra_term(void){
	leastsq_expect("test/leastsq/patterns.a4c", "lsq_linear_extra_term", 0, rel_lsq_not_sum_of_squares, 1, 0, NULL, NULL);
}

static void test_cross_product(void){
	leastsq_expect("test/leastsq/patterns.a4c", "lsq_cross_product", 0, rel_lsq_not_sum_of_squares, 0, 0, NULL, NULL);
}

static void test_ceri651a_shape(void){
	leastsq_expect("test/leastsq/ceri651a_objective.a4c", "ceri651a_objective", 1, rel_lsq_ok, 5, 0, NULL, NULL);
}

#define TESTS(T) \
	T(basic) \
	T(weighted) \
	T(divided_square) \
	T(nested_sum) \
	T(negative_weight) \
	T(variable_weight) \
	T(linear_extra_term) \
	T(cross_product) \
	T(ceri651a_shape)

REGISTER_TESTS_SIMPLE(system_leastsq, TESTS)
