#include <ascend/general/list.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/utilities/ascEnvVar.h>

#include <string.h>

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/child.h>
#include <ascend/compiler/exprs.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/symtab.h>

#include <ascend/system/conditional.h>
#include <ascend/system/bnd.h>
#include <ascend/system/logrel.h>
#include <ascend/system/rel.h>
#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>

#include <test/common.h>

static struct Expr *test_name_expr(const char *name){
	return CreateVarExpr(CreateIdName(AddSymbol(name)));
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

static void test_applies_if_duplicate_rejected(void){
	struct w_when when;
	struct when_case *c1;
	struct when_case *c2;
	struct Expr *e1;
	struct Expr *e2;

	Asc_CompilerInit(1);

	when_create(NULL,&when);
	when.cases = gl_create(2L);
	CU_ASSERT_PTR_NOT_NULL_FATAL(when.cases);

	e1 = test_name_expr("same_region");
	e2 = test_name_expr("same_region");

	c1 = test_case_with_value(1);
	when_case_set_applies(c1,e1);
	gl_append_ptr(when.cases,c1);

	c2 = test_case_with_value(2);
	when_case_set_applies(c2,e2);
	gl_append_ptr(when.cases,c2);

	CU_ASSERT_NOT_EQUAL(
		when_lower_classifier_regions(&when,WHEN_REGION_STEADY),0
	);

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
	CU_ASSERT_EQUAL(plan.requires_generated_artifacts,0);

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

static void test_system_prepare_case_if_steady(void){
	struct module_t *m;
	struct Instance *siminst;
	slv_system_t sys;
	struct w_when **whens;
	struct when_case *wc;
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
	CU_ASSERT_EQUAL(slv_prepare_classifier_whens(sys,WHEN_REGION_STEADY),0);
	CU_ASSERT_EQUAL(
		slv_classifier_regions_lowered(sys,WHEN_REGION_STEADY),1
	);
	CU_ASSERT_EQUAL(slv_prepare_classifier_whens(sys,WHEN_REGION_STEADY),0);

	whens = slv_get_master_when_list(sys);
	CU_ASSERT_PTR_NOT_NULL_FATAL(whens);
	CU_ASSERT_PTR_NOT_NULL_FATAL(whens[0]);
	wc = (struct when_case *)gl_fetch(when_cases_list(whens[0]),1);
	CU_ASSERT_PTR_NOT_NULL_FATAL(wc);
	CU_ASSERT(wc->flags & WHEN_CASE_ACTIVE);

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
	CU_ASSERT_EQUAL(plan.requires_generated_artifacts,1);
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

static void test_system_prepare_case_if_generated_artifacts(void){
	struct module_t *m;
	struct Instance *siminst;
	struct Instance *root;
	slv_system_t sys;
	int status;
	int32 ncondrels, nlogrels, nbnds;
	struct rel_relation *rel0, *rel1;
	struct logrel_relation *logrel0;
	struct bnd_boundary *bnd0, *bnd1;

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
	root = GetSimulationRoot(siminst);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);
	SetRealAtomValue(ChildByChar(root,AddSymbol("a")),2.0,0U);
	SetRealAtomValue(ChildByChar(root,AddSymbol("x")),1.0,0U);
	SetRealAtomValue(ChildByChar(root,AddSymbol("y")),3.0,0U);

	sys = system_build(root);
	CU_ASSERT_PTR_NOT_NULL_FATAL(sys);
	ncondrels = slv_get_num_solvers_condrels(sys);
	nlogrels = slv_get_num_solvers_logrels(sys);
	nbnds = slv_get_num_solvers_bnds(sys);
	status = slv_prepare_classifier_whens(sys,WHEN_REGION_STEADY);
	CU_ASSERT_EQUAL_FATAL(status,0);
	CU_ASSERT_EQUAL(slv_get_num_classifier_rels(sys),2);
	CU_ASSERT_EQUAL(slv_get_num_classifier_logrels(sys),1);
	CU_ASSERT_EQUAL(slv_get_num_classifier_bnds(sys),3);
	CU_ASSERT_EQUAL(slv_get_num_solvers_condrels(sys),ncondrels + 2);
	CU_ASSERT_EQUAL(slv_get_num_solvers_logrels(sys),nlogrels + 1);
	CU_ASSERT_EQUAL(slv_get_num_solvers_bnds(sys),nbnds + 3);

	rel0 = slv_get_classifier_rel(sys,0);
	rel1 = slv_get_classifier_rel(sys,1);
	logrel0 = slv_get_classifier_logrel(sys,0);
	bnd0 = slv_get_classifier_bnd(sys,0);
	bnd1 = slv_get_classifier_bnd(sys,1);
	CU_ASSERT_PTR_NOT_NULL_FATAL(rel0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(rel1);
	CU_ASSERT_PTR_NOT_NULL_FATAL(logrel0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(bnd0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(bnd1);
	CU_ASSERT_EQUAL(rel_n_incidences(rel0) + rel_n_incidences(rel1),3);
	CU_ASSERT_EQUAL(logrel0->n_incidences,0);
	CU_ASSERT_PTR_NOT_NULL(bnd0->logrels);
	CU_ASSERT_PTR_NOT_NULL(bnd1->logrels);
	CU_ASSERT_EQUAL(gl_length(bnd0->logrels),1);
	CU_ASSERT_EQUAL(gl_length(bnd1->logrels),1);

	system_destroy(sys);
	system_free_reused_mem();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_system_prepare_case_if_boolean_eq_artifacts(void){
	struct module_t *m;
	struct Instance *siminst;
	struct Instance *root;
	slv_system_t sys;
	int status;
	int32 ncondrels, nlogrels, nbnds;
	struct rel_relation *rel0;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/instantiate/when_select.a4c",&status);
	CU_ASSERT_PTR_NOT_NULL_FATAL(m);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(
		FindType(AddSymbol("when_case_if_boolean_eq_parses")) != NULL
	);

	siminst = SimsCreateInstance(
		AddSymbol("when_case_if_boolean_eq_parses"),
		AddSymbol("sim1"), e_normal, NULL
	);
	CU_ASSERT_PTR_NOT_NULL_FATAL(siminst);
	root = GetSimulationRoot(siminst);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);
	SetRealAtomValue(ChildByChar(root,AddSymbol("a")),2.0,0U);

	sys = system_build(root);
	CU_ASSERT_PTR_NOT_NULL_FATAL(sys);
	ncondrels = slv_get_num_solvers_condrels(sys);
	nlogrels = slv_get_num_solvers_logrels(sys);
	nbnds = slv_get_num_solvers_bnds(sys);
	status = slv_prepare_classifier_whens(sys,WHEN_REGION_STEADY);
	CU_ASSERT_EQUAL_FATAL(status,0);
	CU_ASSERT_EQUAL(slv_get_num_classifier_rels(sys),1);
	CU_ASSERT_EQUAL(slv_get_num_classifier_logrels(sys),2);
	CU_ASSERT_EQUAL(slv_get_num_classifier_bnds(sys),3);
	CU_ASSERT_EQUAL(slv_get_num_solvers_condrels(sys),ncondrels + 1);
	CU_ASSERT_EQUAL(slv_get_num_solvers_logrels(sys),nlogrels + 2);
	CU_ASSERT_EQUAL(slv_get_num_solvers_bnds(sys),nbnds + 3);
	rel0 = slv_get_classifier_rel(sys,0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(rel0);
	CU_ASSERT_EQUAL(rel_n_incidences(rel0),1);

	system_destroy(sys);
	system_free_reused_mem();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_system_prepare_case_if_inline_satisfied_tolerance(void){
	struct module_t *m;
	struct Instance *siminst;
	struct Instance *root;
	slv_system_t sys;
	int status;
	struct bnd_boundary *bnd;
	struct w_when **whens;
	struct when_case *wc;
	int32 b, nbnds;
	int found = 0;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/instantiate/when_select.a4c",&status);
	CU_ASSERT_PTR_NOT_NULL_FATAL(m);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(
		FindType(AddSymbol("when_case_if_inline_satisfied_parses")) != NULL
	);

	siminst = SimsCreateInstance(
		AddSymbol("when_case_if_inline_satisfied_parses"),
		AddSymbol("sim1"), e_normal, NULL
	);
	CU_ASSERT_PTR_NOT_NULL_FATAL(siminst);
	root = GetSimulationRoot(siminst);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);
	SetRealAtomValue(ChildByChar(root,AddSymbol("a")),4.9999995,0U);

	sys = system_build(root);
	CU_ASSERT_PTR_NOT_NULL_FATAL(sys);
	status = slv_prepare_classifier_whens(sys,WHEN_REGION_STEADY);
	CU_ASSERT_EQUAL_FATAL(status,0);
	CU_ASSERT_EQUAL(slv_get_num_classifier_rels(sys),1);
	CU_ASSERT_EQUAL(slv_get_num_classifier_logrels(sys),1);
	CU_ASSERT_EQUAL(slv_get_num_classifier_bnds(sys),2);

	nbnds = slv_get_num_classifier_bnds(sys);
	for(b = 0; b < nbnds; ++b){
		bnd = slv_get_classifier_bnd(sys,b);
		if(bnd != NULL && bnd_kind(bnd) == e_bnd_rel){
			CU_ASSERT_DOUBLE_EQUAL(bnd_tolerance(bnd),1e-6,1e-12);
			found = 1;
		}
	}
	CU_ASSERT_TRUE(found);

	whens = slv_get_master_when_list(sys);
	CU_ASSERT_PTR_NOT_NULL_FATAL(whens);
	CU_ASSERT_PTR_NOT_NULL_FATAL(whens[0]);
	wc = (struct when_case *)gl_fetch(when_cases_list(whens[0]),1);
	CU_ASSERT_PTR_NOT_NULL_FATAL(wc);
	CU_ASSERT_FALSE(wc->flags & WHEN_CASE_ACTIVE);
	wc = (struct when_case *)gl_fetch(when_cases_list(whens[0]),2);
	CU_ASSERT_PTR_NOT_NULL_FATAL(wc);
	CU_ASSERT_TRUE(wc->flags & WHEN_CASE_ACTIVE);

	system_destroy(sys);
	system_free_reused_mem();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_system_prepare_applies_if_inline_satisfied_tolerance(void){
	struct module_t *m;
	struct Instance *siminst;
	struct Instance *root;
	slv_system_t sys;
	int status;
	struct bnd_boundary *bnd;
	int32 b, nbnds;
	int found = 0;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/instantiate/when_select.a4c",&status);
	CU_ASSERT_PTR_NOT_NULL_FATAL(m);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(
		FindType(AddSymbol("when_applies_if_inline_satisfied_parses")) != NULL
	);

	siminst = SimsCreateInstance(
		AddSymbol("when_applies_if_inline_satisfied_parses"),
		AddSymbol("sim1"), e_normal, NULL
	);
	CU_ASSERT_PTR_NOT_NULL_FATAL(siminst);
	root = GetSimulationRoot(siminst);
	CU_ASSERT_PTR_NOT_NULL_FATAL(root);
	SetRealAtomValue(ChildByChar(root,AddSymbol("a")),2.0,0U);

	sys = system_build(root);
	CU_ASSERT_PTR_NOT_NULL_FATAL(sys);
	status = slv_prepare_classifier_whens(sys,WHEN_REGION_STEADY);
	CU_ASSERT_EQUAL_FATAL(status,0);
	CU_ASSERT_EQUAL(slv_get_num_classifier_rels(sys),1);
	CU_ASSERT_EQUAL(slv_get_num_classifier_logrels(sys),1);
	CU_ASSERT_EQUAL(slv_get_num_classifier_bnds(sys),2);

	nbnds = slv_get_num_classifier_bnds(sys);
	for(b = 0; b < nbnds; ++b){
		bnd = slv_get_classifier_bnd(sys,b);
		if(bnd != NULL && bnd_kind(bnd) == e_bnd_rel){
			CU_ASSERT_DOUBLE_EQUAL(bnd_tolerance(bnd),1e-6,1e-12);
			found = 1;
		}
	}
	CU_ASSERT_TRUE(found);

	system_destroy(sys);
	system_free_reused_mem();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_system_prepare_applies_if_multiple_true_rejected(void){
	struct module_t *m;
	struct Instance *siminst;
	slv_system_t sys;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/instantiate/when_select.a4c",&status);
	CU_ASSERT_PTR_NOT_NULL_FATAL(m);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(
		FindType(AddSymbol("when_applies_if_multiple_true_rejected")) != NULL
	);

	siminst = SimsCreateInstance(
		AddSymbol("when_applies_if_multiple_true_rejected"),
		AddSymbol("sim1"), e_normal, NULL
	);
	CU_ASSERT_PTR_NOT_NULL_FATAL(siminst);

	sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_PTR_NOT_NULL_FATAL(sys);
	status = slv_prepare_classifier_whens(sys,WHEN_REGION_STEADY);
	CU_ASSERT_NOT_EQUAL(status,0);

	system_destroy(sys);
	system_free_reused_mem();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_system_prepare_case_if_boolean_encoding(void){
	struct module_t *m;
	struct Instance *siminst;
	slv_system_t sys;
	int status;
	struct slv_classifier_when_encoding *encoding;
	int32 v_slow[2] = {1,0};
	int32 v_slow_collision[2] = {1,1};
	int32 v_intermediate[2] = {0,1};
	int32 v_fast[2] = {0,0};
	int32 v_short[1] = {1};
	int32 v_bad[2] = {1,2};
	int32 matched_case = -1;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/cmslv/linmassbal_unit_case_if.a4c",&status);
	CU_ASSERT_PTR_NOT_NULL_FATAL(m);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("linmassbal_unit_case_if")) != NULL);

	siminst = SimsCreateInstance(
		AddSymbol("linmassbal_unit_case_if"),
		AddSymbol("sim1"), e_normal, NULL
	);
	CU_ASSERT_PTR_NOT_NULL_FATAL(siminst);

	sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_PTR_NOT_NULL_FATAL(sys);
	status = slv_prepare_classifier_whens(sys,WHEN_REGION_STEADY);
	CU_ASSERT_EQUAL_FATAL(status,0);

	CU_ASSERT_EQUAL(slv_get_num_classifier_when_encodings(sys),1);
	encoding = slv_get_classifier_when_encoding(sys,0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(encoding);
	CU_ASSERT_PTR_NOT_NULL_FATAL(encoding->when);
	CU_ASSERT_EQUAL(encoding->nguards,2);
	CU_ASSERT_EQUAL(encoding->ncases,3);

	CU_ASSERT_PTR_NOT_NULL(encoding->cases[0].wc);
	CU_ASSERT_EQUAL(encoding->cases[0].nvalues,2);
	CU_ASSERT_EQUAL(encoding->cases[0].values[0],1);
	CU_ASSERT_EQUAL(encoding->cases[0].values[1],-2);

	CU_ASSERT_PTR_NOT_NULL(encoding->cases[1].wc);
	CU_ASSERT_EQUAL(encoding->cases[1].nvalues,2);
	CU_ASSERT_EQUAL(encoding->cases[1].values[0],0);
	CU_ASSERT_EQUAL(encoding->cases[1].values[1],1);

	CU_ASSERT_PTR_NOT_NULL(encoding->cases[2].wc);
	CU_ASSERT_EQUAL(encoding->cases[2].nvalues,2);
	CU_ASSERT_EQUAL(encoding->cases[2].values[0],0);
	CU_ASSERT_EQUAL(encoding->cases[2].values[1],0);

	CU_ASSERT_EQUAL(slv_classifier_encoding_match_case(
		encoding,v_slow,2
	),0);
	CU_ASSERT_EQUAL(slv_classifier_encoding_match_case(
		encoding,v_slow_collision,2
	),0);
	CU_ASSERT_EQUAL(slv_classifier_encoding_match_case(
		encoding,v_intermediate,2
	),1);
	CU_ASSERT_EQUAL(slv_classifier_encoding_match_case(
		encoding,v_fast,2
	),2);
	CU_ASSERT_EQUAL(slv_classifier_encoding_match_case(
		encoding,v_short,1
	),-1);
	CU_ASSERT_EQUAL(slv_classifier_encoding_match_case(
		encoding,v_bad,2
	),-1);
	CU_ASSERT_EQUAL(slv_classifier_encoding_match_case(
		NULL,v_slow,2
	),-1);
	CU_ASSERT_EQUAL(slv_classifier_encoding_match_case(
		encoding,NULL,2
	),-1);

	CU_ASSERT_EQUAL(slv_reanalyze_with_classifier_encoding_values(
		sys,encoding,v_slow,2,&matched_case
	),1);
	CU_ASSERT_EQUAL(matched_case,-1);

	CU_ASSERT_EQUAL(slv_reanalyze_with_classifier_encoding_values(
		sys,encoding,v_fast,2,&matched_case
	),1);
	CU_ASSERT_EQUAL(matched_case,-1);

	matched_case = 99;
	CU_ASSERT_EQUAL(slv_reanalyze_with_classifier_encoding_values(
		sys,encoding,v_bad,2,&matched_case
	),1);
	CU_ASSERT_EQUAL(matched_case,-1);

	CU_ASSERT_PTR_NULL(slv_get_classifier_when_encoding(sys,1));

	system_destroy(sys);
	system_free_reused_mem();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_system_prepare_case_if_guard_logic(void){
	struct module_t *m;
	struct Instance *siminst;
	slv_system_t sys;
	int status;
	struct slv_classifier_when_encoding *nested;
	struct slv_classifier_when_encoding *interval;
	struct slv_classifier_when_encoding *named_interval;
	int32 nested_slow[2] = {1,1};
	int32 nested_impossible[2] = {1,0};
	int32 interval_low[2] = {1,0};
	int32 interval_high[2] = {0,1};
	int32 interval_middle[2] = {0,0};
	int32 interval_impossible[2] = {1,1};
	int32 matched_case = -1;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/cmslv/when_case_if_guard_logic.a4c",&status);
	CU_ASSERT_PTR_NOT_NULL_FATAL(m);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("when_case_if_guard_logic")) != NULL);

	siminst = SimsCreateInstance(
		AddSymbol("when_case_if_guard_logic"),
		AddSymbol("sim1"), e_normal, NULL
	);
	CU_ASSERT_PTR_NOT_NULL_FATAL(siminst);

	sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_PTR_NOT_NULL_FATAL(sys);
	status = slv_prepare_classifier_whens(sys,WHEN_REGION_STEADY);
	CU_ASSERT_EQUAL_FATAL(status,0);

	CU_ASSERT_EQUAL(slv_get_num_classifier_when_encodings(sys),3);
	nested = slv_get_classifier_when_encoding(sys,0);
	interval = slv_get_classifier_when_encoding(sys,1);
	named_interval = slv_get_classifier_when_encoding(sys,2);
	CU_ASSERT_PTR_NOT_NULL_FATAL(nested);
	CU_ASSERT_PTR_NOT_NULL_FATAL(interval);
	CU_ASSERT_PTR_NOT_NULL_FATAL(named_interval);
	CU_ASSERT_EQUAL(nested->nguards,2);
	CU_ASSERT_EQUAL(interval->nguards,2);
	CU_ASSERT_EQUAL(named_interval->nguards,2);

	CU_ASSERT_EQUAL(slv_classifier_encoding_guard_implies(nested,0,1),1);
	CU_ASSERT_EQUAL(slv_classifier_encoding_guard_implies(nested,1,0),0);
	CU_ASSERT_EQUAL(slv_classifier_encoding_guards_mutex(nested,0,1),0);
	CU_ASSERT_EQUAL(slv_classifier_encoding_tuple_admissible(
		nested,nested_slow,2
	),1);
	CU_ASSERT_EQUAL(slv_classifier_encoding_tuple_admissible(
		nested,nested_impossible,2
	),0);
	CU_ASSERT_EQUAL(slv_classifier_encoding_match_case(
		nested,nested_impossible,2
	),0);
	CU_ASSERT_EQUAL(slv_reanalyze_with_classifier_encoding_values(
		sys,nested,nested_impossible,2,&matched_case
	),1);
	CU_ASSERT_EQUAL(matched_case,-1);
	CU_ASSERT_EQUAL(slv_reanalyze_with_classifier_encoding_values(
		sys,nested,nested_slow,2,&matched_case
	),0);
	CU_ASSERT_EQUAL(matched_case,0);
	CU_ASSERT_EQUAL(slv_classifier_encoding_current_case(nested),0);
	CU_ASSERT_EQUAL(slv_classifier_encoding_current_tuple_admissible(nested),1);

	CU_ASSERT_EQUAL(slv_classifier_encoding_guard_implies(interval,0,1),0);
	CU_ASSERT_EQUAL(slv_classifier_encoding_guard_implies(interval,1,0),0);
	CU_ASSERT_EQUAL(slv_classifier_encoding_guards_mutex(interval,0,1),1);
	CU_ASSERT_EQUAL(slv_classifier_encoding_tuple_admissible(
		interval,interval_low,2
	),1);
	CU_ASSERT_EQUAL(slv_classifier_encoding_tuple_admissible(
		interval,interval_high,2
	),1);
	CU_ASSERT_EQUAL(slv_classifier_encoding_tuple_admissible(
		interval,interval_middle,2
	),1);
	CU_ASSERT_EQUAL(slv_classifier_encoding_tuple_admissible(
		interval,interval_impossible,2
	),0);

	CU_ASSERT_EQUAL(slv_classifier_encoding_guard_implies(named_interval,0,1),0);
	CU_ASSERT_EQUAL(slv_classifier_encoding_guard_implies(named_interval,1,0),0);
	CU_ASSERT_EQUAL(slv_classifier_encoding_guards_mutex(named_interval,0,1),1);
	CU_ASSERT_EQUAL(slv_classifier_encoding_tuple_admissible(
		named_interval,interval_low,2
	),1);
	CU_ASSERT_EQUAL(slv_classifier_encoding_tuple_admissible(
		named_interval,interval_high,2
	),1);
	CU_ASSERT_EQUAL(slv_classifier_encoding_tuple_admissible(
		named_interval,interval_middle,2
	),1);
	CU_ASSERT_EQUAL(slv_classifier_encoding_tuple_admissible(
		named_interval,interval_impossible,2
	),0);
	CU_ASSERT_EQUAL(slv_reanalyze_with_classifier_encoding_values(
		sys,named_interval,interval_low,2,&matched_case
	),0);
	CU_ASSERT_EQUAL(matched_case,0);
	CU_ASSERT_EQUAL(slv_classifier_encoding_current_case(named_interval),0);
	CU_ASSERT_EQUAL(
		slv_classifier_encoding_current_tuple_admissible(named_interval),1
	);

	system_destroy(sys);
	system_free_reused_mem();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

#define TESTS(T) \
	T(case_if_lowering) \
	T(applies_if_lowering) \
	T(applies_if_duplicate_rejected) \
	T(case_if_missing_condition_rejected) \
	T(case_if_compact_guard_patterns) \
	T(case_if_materialization_reuses_named_guards) \
	T(case_if_guard_artifacts) \
	T(system_build_and_lower_case_if) \
	T(system_prepare_case_if_steady) \
	T(system_build_case_if_materialization_plan) \
	T(system_prepare_case_if_generated_artifacts) \
	T(system_prepare_case_if_boolean_eq_artifacts) \
	T(system_prepare_case_if_inline_satisfied_tolerance) \
	T(system_prepare_applies_if_inline_satisfied_tolerance) \
	T(system_prepare_applies_if_multiple_true_rejected) \
	T(system_prepare_case_if_boolean_encoding) \
	T(system_prepare_case_if_guard_logic)

REGISTER_TESTS_SIMPLE(system_conditional, TESTS)
