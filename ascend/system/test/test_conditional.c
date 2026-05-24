#include <ascend/general/list.h>
#include <ascend/utilities/ascEnvVar.h>

#include <string.h>

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/exprs.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/symtab.h>

#include <ascend/system/conditional.h>
#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>

#include <test/common.h>

static struct Expr *test_name_expr(const char *name){
	(void)name;
	return CreateIntExpr(1);
}

static unsigned long test_expr_len(const struct Expr *expr){
	unsigned long len = 0;
	while(expr != NULL){
		++len;
		expr = expr->next;
	}
	return len;
}

static struct when_case *test_case_with_value(int value){
	struct when_case *wc = when_case_create(NULL);
	CU_ASSERT_PTR_NOT_NULL_FATAL(wc);
	when_case_values_list(wc)[0] = value;
	return wc;
}

static void test_case_if_lowering(void){
	struct w_when when;
	struct when_case *c1;
	struct when_case *c2;
	struct when_case *otherwise;
	struct Expr *e1;
	struct Expr *e2;

	Asc_CompilerInit(1);

	when_create(NULL,&when);
	when.cases = gl_create(3L);
	CU_ASSERT_PTR_NOT_NULL_FATAL(when.cases);

	e1 = test_name_expr("cond1");
	e2 = test_name_expr("cond2");

	c1 = test_case_with_value(1);
	when_case_set_condition(c1,e1);
	gl_append_ptr(when.cases,c1);

	c2 = test_case_with_value(2);
	when_case_set_condition(c2,e2);
	gl_append_ptr(when.cases,c2);

	otherwise = test_case_with_value(-1);
	gl_append_ptr(when.cases,otherwise);

	CU_ASSERT_EQUAL(
		when_lower_classifier_regions(&when,WHEN_REGION_STEADY),0
	);

	CU_ASSERT_EQUAL(
		when_case_region_source(c1),WHEN_REGION_CASE_IF
	);
	CU_ASSERT_EQUAL(
		when_case_region_source(c2),WHEN_REGION_CASE_IF
	);
	CU_ASSERT_EQUAL(
		when_case_region_source(otherwise),WHEN_REGION_CASE_IF_OTHERWISE
	);

	CU_ASSERT_PTR_NOT_NULL(when_case_region_predicate(c1));
	CU_ASSERT_PTR_NOT_NULL(when_case_region_predicate(c2));
	CU_ASSERT_PTR_NOT_NULL(when_case_region_predicate(otherwise));
	CU_ASSERT_EQUAL(test_expr_len(when_case_region_predicate(c1)),1);
	CU_ASSERT_EQUAL(test_expr_len(when_case_region_predicate(c2)),4);
	CU_ASSERT_EQUAL(test_expr_len(when_case_region_predicate(otherwise)),5);

	when_destroy(&when);
	Asc_CompilerDestroy();
}

static void test_applies_if_lowering(void){
	struct w_when when;
	struct when_case *c1;
	struct when_case *c2;
	struct Expr *e1;
	struct Expr *e2;

	Asc_CompilerInit(1);

	when_create(NULL,&when);
	when.cases = gl_create(2L);
	CU_ASSERT_PTR_NOT_NULL_FATAL(when.cases);

	e1 = test_name_expr("region1");
	e2 = test_name_expr("region2");

	c1 = test_case_with_value(1);
	when_case_set_applies(c1,e1);
	gl_append_ptr(when.cases,c1);

	c2 = test_case_with_value(2);
	when_case_set_applies(c2,e2);
	gl_append_ptr(when.cases,c2);

	CU_ASSERT_EQUAL(
		when_lower_classifier_regions(&when,WHEN_REGION_STEADY),0
	);

	CU_ASSERT_EQUAL(when_case_region_source(c1),WHEN_REGION_APPLIES);
	CU_ASSERT_EQUAL(when_case_region_source(c2),WHEN_REGION_APPLIES);
	CU_ASSERT_PTR_NOT_NULL(when_case_region_predicate(c1));
	CU_ASSERT_PTR_NOT_NULL(when_case_region_predicate(c2));
	CU_ASSERT_EQUAL(test_expr_len(when_case_region_predicate(c1)),1);
	CU_ASSERT_EQUAL(test_expr_len(when_case_region_predicate(c2)),1);

	when_destroy(&when);
	Asc_CompilerDestroy();
}

static void test_case_if_missing_condition_rejected(void){
	struct w_when when;
	struct when_case *c1;
	struct when_case *c2;
	struct Expr *e1;

	Asc_CompilerInit(1);

	when_create(NULL,&when);
	when.cases = gl_create(2L);
	CU_ASSERT_PTR_NOT_NULL_FATAL(when.cases);

	e1 = test_name_expr("cond1");

	c1 = test_case_with_value(1);
	when_case_set_condition(c1,e1);
	gl_append_ptr(when.cases,c1);

	c2 = test_case_with_value(2);
	gl_append_ptr(when.cases,c2);

	CU_ASSERT_NOT_EQUAL(
		when_lower_classifier_regions(&when,WHEN_REGION_STEADY),0
	);

	when_destroy(&when);
	Asc_CompilerDestroy();
}

static void test_case_if_compact_guard_patterns(void){
	struct w_when when;
	struct when_case *c1;
	struct when_case *c2;
	struct when_case *otherwise;
	struct Expr *e1;
	struct Expr *e2;
	int32 nguards = -1;
	int32 nvalues = -1;
	int32 values[MAX_VAR_IN_LIST];

	Asc_CompilerInit(1);

	when_create(NULL,&when);
	when.cases = gl_create(3L);
	CU_ASSERT_PTR_NOT_NULL_FATAL(when.cases);

	e1 = test_name_expr("natural_boundary_1");
	e2 = test_name_expr("natural_boundary_2");

	c1 = test_case_with_value(1);
	when_case_set_condition(c1,e1);
	gl_append_ptr(when.cases,c1);

	c2 = test_case_with_value(2);
	when_case_set_condition(c2,e2);
	gl_append_ptr(when.cases,c2);

	otherwise = test_case_with_value(-1);
	gl_append_ptr(when.cases,otherwise);

	CU_ASSERT_EQUAL(when_case_if_guard_count(&when,&nguards),0);
	CU_ASSERT_EQUAL(nguards,2);
	CU_ASSERT_PTR_EQUAL(when_case_if_guard(&when,0),e1);
	CU_ASSERT_PTR_EQUAL(when_case_if_guard(&when,1),e2);
	CU_ASSERT_PTR_NULL(when_case_if_guard(&when,2));

	CU_ASSERT_EQUAL(when_case_if_pattern(&when,c1,values,&nvalues),0);
	CU_ASSERT_EQUAL(nvalues,2);
	CU_ASSERT_EQUAL(values[0],1);
	CU_ASSERT_EQUAL(values[1],-2);

	CU_ASSERT_EQUAL(when_case_if_pattern(&when,c2,values,&nvalues),0);
	CU_ASSERT_EQUAL(nvalues,2);
	CU_ASSERT_EQUAL(values[0],0);
	CU_ASSERT_EQUAL(values[1],1);

	CU_ASSERT_EQUAL(when_case_if_pattern(&when,otherwise,values,&nvalues),0);
	CU_ASSERT_EQUAL(nvalues,2);
	CU_ASSERT_EQUAL(values[0],0);
	CU_ASSERT_EQUAL(values[1],0);

	when_destroy(&when);
	Asc_CompilerDestroy();
}

static struct Expr *test_var_expr(const char *name){
	return CreateVarExpr(CreateIdName(AddSymbol(name)));
}

static struct Expr *test_less_expr(const char *name, double value){
	struct Expr *left = test_var_expr(name);
	struct Expr *right = CreateRealExpr(value,NULL);
	return JoinExprLists(JoinExprLists(left,right),CreateOpExpr(e_less));
}

static struct Expr *test_and_expr(struct Expr *left, struct Expr *right){
	return JoinExprLists(JoinExprLists(left,right),CreateOpExpr(e_and));
}

static void test_case_if_materialization_reuses_named_guards(void){
	struct w_when when;
	struct when_case *c1;
	struct when_case *c2;
	struct when_case *otherwise;
	struct Expr *e1;
	struct Expr *e2;
	struct when_guard_materialization plan;

	Asc_CompilerInit(1);

	when_create(NULL,&when);
	when.cases = gl_create(3L);
	CU_ASSERT_PTR_NOT_NULL_FATAL(when.cases);

	e1 = test_var_expr("existing_boundary_boolean");
	e2 = test_var_expr("existing_region_boolean");

	c1 = test_case_with_value(1);
	when_case_set_condition(c1,e1);
	gl_append_ptr(when.cases,c1);

	c2 = test_case_with_value(2);
	when_case_set_condition(c2,e2);
	gl_append_ptr(when.cases,c2);

	otherwise = test_case_with_value(-1);
	gl_append_ptr(when.cases,otherwise);

	CU_ASSERT_EQUAL(
		when_case_if_materialization_plan(&when,&plan),0
	);
	CU_ASSERT_EQUAL(plan.guard_booleans,2);
	CU_ASSERT_EQUAL(plan.reusable_named_guards,2);
	CU_ASSERT_EQUAL(plan.real_boundaries,0);
	CU_ASSERT_EQUAL(plan.logical_boundaries,0);
	CU_ASSERT_EQUAL(plan.boolean_ops,0);
	CU_ASSERT_EQUAL(plan.hidden_boolean_instances,0);
	CU_ASSERT_EQUAL(plan.hidden_relation_instances,0);
	CU_ASSERT_EQUAL(plan.hidden_logrel_instances,0);
	CU_ASSERT_EQUAL(plan.requires_named_instances,0);

	when_destroy(&when);
	Asc_CompilerDestroy();
}

static void test_case_if_guard_artifacts(void){
	struct w_when when;
	struct when_case *c1;
	struct when_case *c2;
	struct when_case *c3;
	struct when_case *otherwise;
	struct gl_list_t *artifacts;
	struct when_guard_artifact *a1;
	struct when_guard_artifact *a2;
	struct when_guard_artifact *a3;

	Asc_CompilerInit(1);

	when_create(NULL,&when);
	when.cases = gl_create(4L);
	CU_ASSERT_PTR_NOT_NULL_FATAL(when.cases);

	c1 = test_case_with_value(1);
	when_case_set_condition(c1,test_var_expr("existing_boolean"));
	when_case_set_source(c1,NULL,101);
	gl_append_ptr(when.cases,c1);

	c2 = test_case_with_value(2);
	when_case_set_condition(c2,test_less_expr("a",5.0));
	when_case_set_source(c2,NULL,102);
	gl_append_ptr(when.cases,c2);

	c3 = test_case_with_value(3);
	when_case_set_condition(
		c3,
		test_and_expr(test_less_expr("b",2.0),test_var_expr("flag"))
	);
	when_case_set_source(c3,NULL,103);
	gl_append_ptr(when.cases,c3);

	otherwise = test_case_with_value(-1);
	gl_append_ptr(when.cases,otherwise);

	artifacts = when_case_if_artifacts_create(&when);
	CU_ASSERT_PTR_NOT_NULL_FATAL(artifacts);
	CU_ASSERT_EQUAL(gl_length(artifacts),3);

	a1 = (struct when_guard_artifact *)gl_fetch(artifacts,1);
	a2 = (struct when_guard_artifact *)gl_fetch(artifacts,2);
	a3 = (struct when_guard_artifact *)gl_fetch(artifacts,3);
	CU_ASSERT_PTR_NOT_NULL_FATAL(a1);
	CU_ASSERT_PTR_NOT_NULL_FATAL(a2);
	CU_ASSERT_PTR_NOT_NULL_FATAL(a3);

	CU_ASSERT_EQUAL(a1->guard_index,0);
	CU_ASSERT_PTR_EQUAL(a1->source_case,c1);
	CU_ASSERT_EQUAL(a1->reuse_existing_boolean,1);
	CU_ASSERT_EQUAL(a1->generated_boolean,0);
	CU_ASSERT_EQUAL(a1->reusable_named_terms,1);
	CU_ASSERT_EQUAL(a1->real_boundaries,0);
	CU_ASSERT_EQUAL(a1->source_line,101);

	CU_ASSERT_EQUAL(a2->guard_index,1);
	CU_ASSERT_PTR_EQUAL(a2->source_case,c2);
	CU_ASSERT_EQUAL(a2->reuse_existing_boolean,0);
	CU_ASSERT_EQUAL(a2->generated_boolean,1);
	CU_ASSERT_EQUAL(a2->real_boundaries,1);
	CU_ASSERT_EQUAL(a2->reusable_named_terms,0);
	CU_ASSERT_EQUAL(a2->boolean_ops,0);
	CU_ASSERT_EQUAL(a2->source_line,102);

	CU_ASSERT_EQUAL(a3->guard_index,2);
	CU_ASSERT_PTR_EQUAL(a3->source_case,c3);
	CU_ASSERT_EQUAL(a3->reuse_existing_boolean,0);
	CU_ASSERT_EQUAL(a3->generated_boolean,1);
	CU_ASSERT_EQUAL(a3->real_boundaries,1);
	CU_ASSERT_EQUAL(a3->reusable_named_terms,1);
	CU_ASSERT_EQUAL(a3->boolean_ops,1);
	CU_ASSERT_EQUAL(a3->source_line,103);

	when_case_if_artifacts_destroy(artifacts);
	when_destroy(&when);
	Asc_CompilerDestroy();
}

static void test_system_build_and_lower_case_if(void){
	struct module_t *m;
	struct Instance *siminst;
	slv_system_t sys;
	struct w_when **whens;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/instantiate/when_select.a4c",&status);
	CU_ASSERT_PTR_NOT_NULL_FATAL(m);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("when_case_if_parses")) != NULL);

	siminst = SimsCreateInstance(
		AddSymbol("when_case_if_parses"), AddSymbol("sim1"), e_normal, NULL
	);
	CU_ASSERT_PTR_NOT_NULL_FATAL(siminst);

	sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_PTR_NOT_NULL_FATAL(sys);
	CU_ASSERT_EQUAL(slv_has_classifier_whens(sys),1);
	CU_ASSERT_EQUAL(
		slv_classifier_regions_lowered(sys,WHEN_REGION_STEADY),0
	);
	CU_ASSERT_EQUAL(slv_lower_classifier_whens(sys,WHEN_REGION_STEADY),0);
	CU_ASSERT_EQUAL(
		slv_classifier_regions_lowered(sys,WHEN_REGION_STEADY),1
	);

	whens = slv_get_master_when_list(sys);
	CU_ASSERT_PTR_NOT_NULL(whens);
	CU_ASSERT_PTR_NOT_NULL(whens[0]);
	CU_ASSERT_PTR_NOT_NULL(when_case_region_predicate(
		(struct when_case *)gl_fetch(when_cases_list(whens[0]),1)
	));

	system_destroy(sys);
	system_free_reused_mem();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_system_build_case_if_materialization_plan(void){
	struct module_t *m;
	struct Instance *siminst;
	slv_system_t sys;
	struct w_when **whens;
	struct when_guard_materialization plan;
	struct when_case *wc;
	const char *module_name;
	int status;
	int32 nguards = -1;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/instantiate/when_select.a4c",&status);
	CU_ASSERT_PTR_NOT_NULL_FATAL(m);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(
		FindType(AddSymbol("when_case_if_full_boolean_parses")) != NULL
	);

	siminst = SimsCreateInstance(
		AddSymbol("when_case_if_full_boolean_parses"),
		AddSymbol("sim1"), e_normal, NULL
	);
	CU_ASSERT_PTR_NOT_NULL_FATAL(siminst);

	sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_PTR_NOT_NULL_FATAL(sys);
	whens = slv_get_master_when_list(sys);
	CU_ASSERT_PTR_NOT_NULL_FATAL(whens);
	CU_ASSERT_PTR_NOT_NULL_FATAL(whens[0]);
	wc = (struct when_case *)gl_fetch(when_cases_list(whens[0]),1);
	CU_ASSERT_PTR_NOT_NULL_FATAL(wc);

	CU_ASSERT_EQUAL(when_case_if_guard_count(whens[0],&nguards),0);
	CU_ASSERT_EQUAL(nguards,1);
	CU_ASSERT_EQUAL(
		when_case_if_materialization_plan(whens[0],&plan),0
	);
	CU_ASSERT_EQUAL(plan.guard_booleans,1);
	CU_ASSERT_EQUAL(plan.reusable_named_guards,0);
	CU_ASSERT_EQUAL(plan.real_boundaries,2);
	CU_ASSERT_EQUAL(plan.logical_boundaries,0);
	CU_ASSERT_EQUAL(plan.boolean_ops,3);
	CU_ASSERT_EQUAL(plan.unsupported_dynamic_terms,0);
	CU_ASSERT_EQUAL(plan.hidden_boolean_instances,1);
	CU_ASSERT_EQUAL(plan.hidden_relation_instances,2);
	CU_ASSERT_EQUAL(plan.hidden_logrel_instances,1);
	CU_ASSERT_EQUAL(plan.requires_named_instances,1);
	CU_ASSERT_PTR_NOT_NULL(when_case_source_module(wc));
	CU_ASSERT(when_case_source_line(wc) > 0);
	module_name = Asc_ModuleBestName(when_case_source_module(wc));
	CU_ASSERT_PTR_NOT_NULL(module_name);
	CU_ASSERT_PTR_NOT_NULL(strstr(module_name,"when_select.a4c"));

	system_destroy(sys);
	system_free_reused_mem();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

#define TESTS(T) \
	T(case_if_lowering) \
	T(applies_if_lowering) \
	T(case_if_missing_condition_rejected) \
	T(case_if_compact_guard_patterns) \
	T(case_if_materialization_reuses_named_guards) \
	T(case_if_guard_artifacts) \
	T(system_build_and_lower_case_if) \
	T(system_build_case_if_materialization_plan)

REGISTER_TESTS_SIMPLE(system_conditional, TESTS)
