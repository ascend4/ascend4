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
#include <ascend/compiler/packages.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/instance_io.h>
#include <ascend/compiler/qlfdid.h>
#include <ascend/compiler/derivinst.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/arrayinst.h>
#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/mathinst.h>
#include <ascend/compiler/relation.h>
#include <ascend/compiler/relation_util.h>

#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>
#include <ascend/system/slv_server.h>
#include <ascend/system/diffvars.h>
#include <ascend/system/diffvars_impl.h>
#include <ascend/system/var.h>
#include <ascend/system/relman.h>
#include <ascend/solver/solver.h>

#include <test/common.h>

static struct Instance *load_sim_for_model(const char *filename, const char *modelname){
	int status;
	struct Instance *siminst;
	struct Name *name;
	enum Proc_enum pe;

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

	return siminst;
}

static slv_system_t build_system_for_model(const char *filename, const char *modelname, struct Instance **siminst_out){
	struct Instance *siminst;
	slv_system_t sys;

	siminst = load_sim_for_model(filename, modelname);
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

static int ensure_qrslv_loaded(void){
	int qrslv_index;
	CU_ASSERT_FATAL(0 == package_load("qrslv",NULL));
	qrslv_index = slv_lookup_client("QRSlv");
	CU_ASSERT_FATAL(qrslv_index != -1);
	return qrslv_index;
}

static struct var_variable *find_derivative_var(slv_system_t sys){
	struct var_variable **vp;
	CU_ASSERT_FATAL(sys != NULL);
	vp = slv_get_solvers_var_list(sys);
	CU_ASSERT_FATAL(vp != NULL);
	for(; *vp != NULL; ++vp){
		struct Instance *inst = (struct Instance *)var_instance(*vp);
		if(inst != NULL && IsDerivativeInstance(inst)){
			return *vp;
		}
	}
	return NULL;
}

static void qrslv_presolve_or_fail(slv_system_t sys){
	slv_status_t status;
	int qrslv_index = ensure_qrslv_loaded();
	CU_ASSERT_FATAL(slv_select_solver(sys, qrslv_index));
	CU_ASSERT_FATAL(0 == slv_presolve(sys));
	slv_get_status(sys, &status);
	CU_ASSERT_FATAL(status.ready_to_solve);
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

static void assert_ida_relation_count(slv_system_t sys, int nrels){
	rel_filter_t rf;
	rf.matchbits = REL_INCLUDED | REL_EQUALITY | REL_ACTIVE;
	rf.matchvalue = REL_INCLUDED | REL_EQUALITY | REL_ACTIVE;
	CU_ASSERT(slv_count_solvers_rels(sys, &rf) == nrels);
}

static void assert_included_equality_relation_count(slv_system_t sys, int nrels){
	rel_filter_t rf;
	rf.matchbits = REL_INCLUDED | REL_EQUALITY;
	rf.matchvalue = REL_INCLUDED | REL_EQUALITY;
	CU_ASSERT(slv_count_solvers_rels(sys, &rf) == nrels);
}

static void test_der_expr_direct_ok(void){
	struct Instance *siminst = NULL;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","der_expr_direct_ok",&siminst);
	assert_diffvars_shape(sys,1,1,2);
	destroy_loaded_system(sys,siminst);
}

static void test_der_equation_direct_ok(void){
	struct Instance *siminst = NULL;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","der_equation_direct_ok",&siminst);
	assert_diffvars_shape(sys,1,1,2);
	/*
	 * `dy_dt = der(y)` is a modelling equation, not hidden derivative-chain
	 * metadata. It remains in the solver relation list alongside `dyn`.
	 */
	assert_ida_relation_count(sys,2);
	assert_included_equality_relation_count(sys,2);
	destroy_loaded_system(sys,siminst);
}

static void test_der_only_direct_ok(void){
	struct Instance *siminst = NULL;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","der_only_direct_ok",&siminst);
	assert_diffvars_shape(sys,1,1,2);
	assert_ida_relation_count(sys,1);
	assert_included_equality_relation_count(sys,1);
	destroy_loaded_system(sys,siminst);
}

static void test_der_expr_nested_ok(void){
	struct Instance *siminst = NULL;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","der_expr_nested_ok",&siminst);
	assert_diffvars_shape(sys,1,1,2);
	destroy_loaded_system(sys,siminst);
}

static void test_der_equation_nested_ok(void){
	struct Instance *siminst = NULL;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","der_equation_nested_ok",&siminst);
	assert_diffvars_shape(sys,1,1,2);
	assert_ida_relation_count(sys,2);
	assert_included_equality_relation_count(sys,2);
	destroy_loaded_system(sys,siminst);
}

static void test_der_only_nested_ok(void){
	struct Instance *siminst = NULL;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","der_only_nested_ok",&siminst);
	assert_diffvars_shape(sys,1,1,2);
	assert_ida_relation_count(sys,1);
	assert_included_equality_relation_count(sys,1);
	destroy_loaded_system(sys,siminst);
}

static void test_der_only_array_direct_ok(void){
	struct Instance *siminst = NULL;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","der_only_array_direct_ok",&siminst);
	assert_diffvars_shape(sys,3,1,2);
	destroy_loaded_system(sys,siminst);
}

static void test_der_only_array_nested_ok(void){
	struct Instance *siminst = NULL;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","der_only_array_nested_ok",&siminst);
	assert_diffvars_shape(sys,3,1,2);
	destroy_loaded_system(sys,siminst);
}

static void test_der_implicit_mass_ok(void){
	struct Instance *siminst = NULL;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","der_implicit_mass_ok",&siminst);
	assert_diffvars_shape(sys,1,1,2);
	destroy_loaded_system(sys,siminst);
}

static void test_der_mixed_ok(void){
	struct Instance *siminst = NULL;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","der_mixed_ok",&siminst);
	assert_diffvars_shape(sys,2,1,2);
	destroy_loaded_system(sys,siminst);
}

static void test_der_qlfdid_before_build_ok(void){
	struct Instance *siminst;
	struct Instance *root, *y, *deriv;
	extern struct Instance *g_search_inst;
	extern struct Instance *g_relative_inst;

	siminst = load_sim_for_model("test/ida/alias_der_wLINK.a4c","der_only_direct_ok");
	CU_ASSERT_FATAL(siminst != NULL);
	root = GetSimulationRoot(siminst);
	y = ChildByChar(root, AddSymbol("y"));
	CU_ASSERT_FATAL(y != NULL);
	deriv = InstanceEnsureDerivative(y);
	CU_ASSERT(deriv != NULL);

	g_search_inst = NULL;
	g_relative_inst = root;
	CU_ASSERT(0 == Asc_QlfdidSearch3("y.der", 1));
	deriv = g_search_inst;
	CU_ASSERT(deriv != NULL);
	if(deriv != NULL){
		CU_ASSERT_TRUE(IsDerivativeInstance(deriv));
		CU_ASSERT_TRUE(DerivativeInstanceBase(deriv) == y || DerivativeInstanceBase(deriv) == NextCliqueMember(y));
	}

	g_relative_inst = root;
	CU_ASSERT(1 == Asc_QlfdidSearch3("t.der", 1));
	destroy_loaded_system(NULL, siminst);
}

static void test_der_qlfdid_canonical_ok(void){
	struct Instance *siminst;
	struct Instance *root, *y, *deriv1, *deriv2;
	extern struct Instance *g_search_inst;
	extern struct Instance *g_relative_inst;

	siminst = load_sim_for_model("test/ida/alias_der_wLINK.a4c","der_only_nested_ok");
	CU_ASSERT_FATAL(siminst != NULL);
	root = GetSimulationRoot(siminst);
	CU_ASSERT_FATAL(root != NULL);

	g_search_inst = NULL;
	g_relative_inst = root;
	CU_ASSERT(0 == Asc_QlfdidSearch3("der(cell.y)", 1));
	deriv1 = g_search_inst;
	CU_ASSERT_FATAL(deriv1 != NULL);
	CU_ASSERT_TRUE(IsDerivativeInstance(deriv1));

	g_search_inst = NULL;
	g_relative_inst = root;
	CU_ASSERT(0 == Asc_QlfdidSearch3("cell.y.der", 1));
	deriv2 = g_search_inst;
	CU_ASSERT_PTR_EQUAL(deriv1, deriv2);

	y = ChildByChar(ChildByChar(root, AddSymbol("cell")), AddSymbol("y"));
	CU_ASSERT_FATAL(y != NULL);
	CU_ASSERT_PTR_EQUAL(DerivativeInstanceBase(deriv1), y);

	destroy_loaded_system(NULL, siminst);
}

static void test_der_qlfdid_after_build_reuses_instance(void){
	struct Instance *siminst = NULL;
	struct Instance *root, *deriv;
	slv_system_t sys;
	SolverDiffVarCollection *diffvars;
	extern struct Instance *g_search_inst;
	extern struct Instance *g_relative_inst;

	sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","der_only_direct_ok",&siminst);
	root = GetSimulationRoot(siminst);
	CU_ASSERT_FATAL(root != NULL);

	g_search_inst = NULL;
	g_relative_inst = root;
	CU_ASSERT(0 == Asc_QlfdidSearch3("y.der", 1));
	deriv = g_search_inst;
	CU_ASSERT(deriv != NULL);

	diffvars = system_get_diffvars(sys);
	CU_ASSERT(diffvars != NULL);
	if(diffvars != NULL){
		CU_ASSERT(diffvars->nseqs >= 1);
		if(diffvars->nseqs >= 1){
			CU_ASSERT(diffvars->seqs[0].n == 2);
			if(diffvars->seqs[0].n == 2 && deriv != NULL){
				CU_ASSERT_PTR_EQUAL(var_instance(diffvars->seqs[0].vars[1]), deriv);
			}
		}
	}

	destroy_loaded_system(sys,siminst);
}

static void test_der_dynamic_child_api_ok(void){
	struct Instance *siminst;
	struct Instance *root, *y, *deriv;
	symchar *name;

	siminst = load_sim_for_model("test/ida/alias_der_wLINK.a4c","der_only_direct_ok");
	CU_ASSERT_FATAL(siminst != NULL);
	root = GetSimulationRoot(siminst);
	CU_ASSERT_FATAL(root != NULL);
	y = ChildByChar(root, AddSymbol("y"));
	CU_ASSERT_FATAL(y != NULL);
	deriv = InstanceEnsureDerivative(y);
	CU_ASSERT_FATAL(deriv != NULL);

	CU_ASSERT_EQUAL(InstanceDynamicChildCount(y), 1UL);
	CU_ASSERT_PTR_EQUAL(InstanceDynamicChild(y, 1), deriv);
	CU_ASSERT_PTR_EQUAL(InstanceDynamicChildByChar(y, AddSymbol("der")), deriv);
	CU_ASSERT_EQUAL(InstanceDynamicChildIndex(y, deriv), 1UL);
	name = InstanceDynamicChildName(y, 1);
	CU_ASSERT_FATAL(name != NULL);
	CU_ASSERT_STRING_EQUAL(SCP(name), "der");

	destroy_loaded_system(NULL, siminst);
}

static void test_der_method_fix_assign_ok(void){
	struct Instance *siminst = NULL;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","der_method_fix_assign_ok",&siminst);
	SolverDiffVarCollection *diffvars = system_get_diffvars(sys);
	CU_ASSERT_FATAL(diffvars != NULL);
	CU_ASSERT_FATAL(diffvars->nseqs >= 1);
	CU_ASSERT_FATAL(diffvars->seqs[0].n == 2);
	CU_ASSERT_DOUBLE_EQUAL(var_value(diffvars->seqs[0].vars[1]), 2.0, 1e-12);
	CU_ASSERT_TRUE(var_fixed(diffvars->seqs[0].vars[1]));
	destroy_loaded_system(sys,siminst);
}

static void test_der_method_fix_assign_before_build_ok(void){
	struct Instance *siminst = load_sim_for_model("test/ida/alias_der_wLINK.a4c","der_method_fix_assign_ok");
	struct Instance *root, *y, *deriv, *fixed;
	CU_ASSERT_FATAL(siminst != NULL);
	root = GetSimulationRoot(siminst);
	CU_ASSERT_FATAL(root != NULL);
	y = ChildByChar(root, AddSymbol("y"));
	CU_ASSERT_FATAL(y != NULL);
	deriv = InstanceGetDerivative(y);
	CU_ASSERT_FATAL(deriv != NULL);
	CU_ASSERT_TRUE(IsDerivativeInstance(deriv));
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(deriv), 2.0, 1e-12);
	fixed = ChildByChar(deriv, AddSymbol("fixed"));
	CU_ASSERT_FATAL(fixed != NULL);
	CU_ASSERT_TRUE(GetBooleanAtomValue(fixed));
	destroy_loaded_system(NULL,siminst);
}

static void test_der_method_free_ok(void){
	struct Instance *siminst = NULL;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","der_method_free_ok",&siminst);
	SolverDiffVarCollection *diffvars = system_get_diffvars(sys);
	CU_ASSERT_FATAL(diffvars != NULL);
	CU_ASSERT_FATAL(diffvars->nseqs >= 1);
	CU_ASSERT_FATAL(diffvars->seqs[0].n == 2);
	CU_ASSERT_FALSE(var_fixed(diffvars->seqs[0].vars[1]));
	destroy_loaded_system(sys,siminst);
}

static void test_der_qrslv_default_fixed_ok(void){
	struct Instance *siminst = NULL;
	struct Instance *root, *x, *y, *deriv;
	struct var_variable *dvar;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","der_qrslv_default_fixed_ok",&siminst);

	qrslv_presolve_or_fail(sys);
	dvar = find_derivative_var(sys);
	CU_ASSERT_FATAL(dvar != NULL);
	CU_ASSERT_TRUE(var_flagbit(dvar, VAR_FIXED));
	CU_ASSERT_TRUE(var_potentially_fixed(dvar));

	slv_solve(sys);
	root = GetSimulationRoot(siminst);
	CU_ASSERT_FATAL(root != NULL);
	x = ChildByChar(root, AddSymbol("x"));
	y = ChildByChar(root, AddSymbol("y"));
	CU_ASSERT_FATAL(x != NULL);
	CU_ASSERT_FATAL(y != NULL);
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(x), 1.0, 1e-8);
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(y), 2.0, 1e-8);
	deriv = InstanceGetDerivative(x);
	CU_ASSERT_FATAL(deriv != NULL);
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(deriv), 0.0, 1e-12);

	destroy_loaded_system(sys,siminst);
}

static void test_der_qrslv_free_ok(void){
	struct Instance *siminst = NULL;
	struct Instance *root, *x, *y, *deriv;
	struct var_variable *dvar;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","der_qrslv_free_ok",&siminst);

	qrslv_presolve_or_fail(sys);
	dvar = find_derivative_var(sys);
	CU_ASSERT_FATAL(dvar != NULL);
	CU_ASSERT_FALSE(var_flagbit(dvar, VAR_FIXED));
	CU_ASSERT_FALSE(var_potentially_fixed(dvar));

	slv_solve(sys);
	root = GetSimulationRoot(siminst);
	CU_ASSERT_FATAL(root != NULL);
	x = ChildByChar(root, AddSymbol("x"));
	y = ChildByChar(root, AddSymbol("y"));
	CU_ASSERT_FATAL(x != NULL);
	CU_ASSERT_FATAL(y != NULL);
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(x), 1.0, 1e-8);
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(y), 5.0, 1e-8);
	deriv = InstanceGetDerivative(x);
	CU_ASSERT_FATAL(deriv != NULL);
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(deriv), 3.0, 1e-8);

	destroy_loaded_system(sys,siminst);
}

static void test_der_qrslv_assign_nonzero_ok(void){
	struct Instance *siminst = load_sim_for_model("test/ida/alias_der_wLINK.a4c","der_qrslv_assign_nonzero_ok");
	struct Instance *root, *x, *deriv, *fixed;
	CU_ASSERT_FATAL(siminst != NULL);
	root = GetSimulationRoot(siminst);
	CU_ASSERT_FATAL(root != NULL);
	x = ChildByChar(root, AddSymbol("x"));
	CU_ASSERT_FATAL(x != NULL);
	deriv = InstanceGetDerivative(x);
	CU_ASSERT_FATAL(deriv != NULL);
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(deriv), 5.0, 1e-12);
	fixed = ChildByChar(deriv, AddSymbol("fixed"));
	CU_ASSERT_FATAL(fixed != NULL);
	CU_ASSERT_FALSE(GetBooleanAtomValue(fixed));

	destroy_loaded_system(NULL,siminst);
}

static void test_der_qrslv_fix_assign_nonzero_ok(void){
	struct Instance *siminst = NULL;
	struct Instance *root, *x, *y, *deriv;
	struct var_variable *dvar;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","der_qrslv_fix_assign_nonzero_ok",&siminst);

	qrslv_presolve_or_fail(sys);
	dvar = find_derivative_var(sys);
	CU_ASSERT_FATAL(dvar != NULL);
	CU_ASSERT_TRUE(var_flagbit(dvar, VAR_FIXED));
	CU_ASSERT_FALSE(var_potentially_fixed(dvar));

	slv_solve(sys);
	root = GetSimulationRoot(siminst);
	CU_ASSERT_FATAL(root != NULL);
	x = ChildByChar(root, AddSymbol("x"));
	y = ChildByChar(root, AddSymbol("y"));
	CU_ASSERT_FATAL(x != NULL);
	CU_ASSERT_FATAL(y != NULL);
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(x), 1.0, 1e-8);
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(y), 7.0, 1e-8);
	deriv = InstanceGetDerivative(x);
	CU_ASSERT_FATAL(deriv != NULL);
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(deriv), 5.0, 1e-8);

	destroy_loaded_system(sys,siminst);
}

static void test_der_alias_scalar_ok(void){
	struct Instance *siminst = NULL;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","alias_der_alias_fail",&siminst);
	assert_diffvars_shape(sys,1,1,2);
	destroy_loaded_system(sys,siminst);
}

static void test_der_alias_array_ok(void){
	struct Instance *siminst = NULL;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","alias_der_array_alias_fail",&siminst);
	assert_diffvars_shape(sys,3,1,2);
	destroy_loaded_system(sys,siminst);
}

static void test_der_array_same_ok(void){
	struct Instance *siminst = NULL;
	slv_system_t sys = build_system_for_model("test/ida/alias_der_wLINK.a4c","alias_der_array_same",&siminst);
	assert_diffvars_shape(sys,3,1,2);
	destroy_loaded_system(sys,siminst);
}

struct der_visit_test {
	struct Instance *base, *deriv, *attribute;
	int bases, derivatives, attributes, sequence, base_position, derivative_position;
};

static void record_der_visit(struct Instance *inst, void *userdata){
	struct der_visit_test *v = userdata;
	++v->sequence;
	if(inst == v->base){ ++v->bases; v->base_position = v->sequence; }
	if(inst == v->deriv){ ++v->derivatives; v->derivative_position = v->sequence; }
	if(inst == v->attribute){ ++v->attributes; }
}

static void test_der_visit_coverage(void){
	struct Instance *sim = load_sim_for_model("test/ida/alias_der_wLINK.a4c", "der_only_direct_ok");
	struct Instance *root = GetSimulationRoot(sim);
	struct Instance *base = ChildByChar(root, AddSymbol("y"));
	struct der_visit_test v = {0};
	int depth, leaf;
	v.base = base;
	CU_ASSERT_PTR_NULL(InstancePeekDerivative(base));
	SilentVisitInstanceTreeTwoWithCoverage(root, record_der_visit, 0, 0, &v,
		INSTANCE_VISIT_MATERIALISED_DERIVATIVES);
	CU_ASSERT(v.bases == 1);
	CU_ASSERT_PTR_NULL(InstancePeekDerivative(base)); /* traversal must not create it */
	for(depth = 0; depth <= 1; ++depth){
		for(leaf = 0; leaf <= 1; ++leaf){
			memset(&v, 0, sizeof(v));
			v.base = base;
			v.deriv = InstanceEnsureDerivative(base);
			CU_ASSERT_FATAL(v.deriv != NULL);
			v.attribute = ChildByChar(v.deriv, AddSymbol("fixed"));
			SilentVisitInstanceTreeTwoWithCoverage(root, record_der_visit, depth, leaf, &v,
				INSTANCE_VISIT_MATERIALISED_DERIVATIVES);
			CU_ASSERT(v.bases == 1);
			CU_ASSERT(v.derivatives == 1);
			CU_ASSERT(v.attributes == leaf);
			CU_ASSERT(depth ? v.derivative_position < v.base_position
				: v.base_position < v.derivative_position);
			v.bases = v.derivatives = v.attributes = 0;
			SilentVisitInstanceTreeTwo(root, record_der_visit, depth, leaf, &v);
			CU_ASSERT(v.bases == 1);
			CU_ASSERT(v.derivatives == 0);
			CU_ASSERT(v.attributes == 0);
		}
	}
	destroy_loaded_system(NULL, sim);
}

/* Deliberately opaque non-NULL data, like the GUI's InstanceInterfaceData.
 * Neither analysis nor system destruction may interpret or change it. */
static void *der_test_interface(struct Instance *inst, void *userdata){
	return InstanceKind(inst) == REAL_ATOM_INST || InstanceKind(inst) == REL_INST
		? userdata : NULL;
}

static void check_der_test_interface(struct Instance *inst, void *userdata){
	if(der_test_interface(inst, userdata)){
		CU_ASSERT_PTR_EQUAL(GetInterfacePtr(inst), userdata);
	}
}

static void assert_coexist_math(slv_system_t sys){
	struct rel_relation **rels = slv_get_solvers_rel_list(sys);
	struct var_variable **master = slv_get_master_var_list(sys);
	int r;
	var_filter_t all = {0, 0};
	CU_ASSERT_FATAL(slv_get_num_solvers_rels(sys) == 3);
	for(r = 0; r < 3; ++r){
		real64 gradient[4];
		struct var_variable *vars[4];
		int32 count = 0, ok = 0, j;
		int nbase = 0, nder = 0;
		CU_ASSERT_DOUBLE_EQUAL(relman_eval(rels[r], &ok, 1), 0, 1e-7);
		CU_ASSERT(ok);
		CU_ASSERT(0 == relman_diff3(rels[r], &all, gradient, vars, &count, 1));
		CU_ASSERT_FATAL(count == 2);
		for(j = 0; j < count; ++j){
			struct Instance *inst = (struct Instance *)var_instance(vars[j]);
			CU_ASSERT_PTR_EQUAL(master[var_mindex(vars[j])], vars[j]);
			if(IsDerivativeInstance(inst)){
				++nder;
				CU_ASSERT_DOUBLE_EQUAL(gradient[j], 1, 1e-12);
				CU_ASSERT_PTR_EQUAL(InstancePeekDerivative(DerivativeInstanceBase(inst)), inst);
			}else{
				++nbase;
				CU_ASSERT_DOUBLE_EQUAL(gradient[j], 2 * RealAtomValue(inst), 1e-12);
			}
		}
		CU_ASSERT(nbase == 1 && nder == 1);
	}
}

static void solve_coexist_system(slv_system_t sys, int qrslv){
	slv_status_t status;
	CU_ASSERT(slv_select_solver(sys, qrslv) != -1);
	CU_ASSERT(0 == slv_presolve(sys));
	CU_ASSERT(0 == slv_solve(sys));
	slv_get_status(sys, &status);
	CU_ASSERT(status.converged);
	assert_coexist_math(sys);
}

static void run_der_system_coexist(int destroy_first){
	struct Instance *sim = load_sim_for_model("test/ida/der_coexist.a4c", "der_coexist");
	struct Instance *root = GetSimulationRoot(sim);
	struct Instance *base = ChildByChar(root, AddSymbol("alias_x"));
	struct Instance *deriv = InstancePeekDerivative(base);
	struct Instance *other = ChildByChar(root, AddSymbol("other_t"));
	struct gl_list_t *saved, *nested;
	slv_system_t a, b;
	struct var_variable *held;
	struct der_visit_test visits = {0};
	int gui_data = 1234567, temporary_data = 7654321, round;
	int qrslv = ensure_qrslv_loaded();
	CU_ASSERT_FATAL(deriv != NULL);
	visits.base = base;
	visits.deriv = deriv;
	SilentVisitInstanceTreeTwoWithCoverage(root, record_der_visit, 1, 0, &visits,
		INSTANCE_VISIT_MATERIALISED_DERIVATIVES);
	CU_ASSERT(visits.bases == 1 && visits.derivatives == 1); /* alias not visited twice */
	saved = PushInterfacePtrsWithCoverage(root, der_test_interface, 0, 1, &gui_data,
		INSTANCE_VISIT_MATERIALISED_DERIVATIVES);
	CU_ASSERT_FATAL(saved != NULL);
	nested = PushInterfacePtrsWithCoverage(root, der_test_interface, 0, 1, &temporary_data,
		INSTANCE_VISIT_MATERIALISED_DERIVATIVES);
	CU_ASSERT_FATAL(nested != NULL);
	CU_ASSERT_PTR_EQUAL(GetInterfacePtr(deriv), &temporary_data);
	PopInterfacePtrs(nested, NULL, NULL);
	CU_ASSERT_PTR_EQUAL(GetInterfacePtr(deriv), &gui_data);
	a = system_build(root);
	CU_ASSERT_FATAL(a != NULL);
	solve_coexist_system(a, qrslv);
	/* Presolve can reorder the solver list; retain a handle after that. */
	held = find_derivative_var(a);
	CU_ASSERT_FATAL(held != NULL);
	for(round = 0; round < 3; ++round){
		b = system_build(root);
		CU_ASSERT_FATAL(b != NULL);
		CU_ASSERT(find_derivative_var(b) != held);
		assert_diffvars_shape(b, 3, 1, 2);
		solve_coexist_system(b, qrslv);
		assert_coexist_math(a);
		SilentVisitInstanceTreeTwoWithCoverage(root, check_der_test_interface, 0, 0, &gui_data,
			INSTANCE_VISIT_MATERIALISED_DERIVATIVES);
		system_destroy(b);
		CU_ASSERT_PTR_EQUAL(find_derivative_var(a), held);
		CU_ASSERT_PTR_EQUAL(InstancePeekDerivative(base), deriv);
		SilentVisitInstanceTreeTwoWithCoverage(root, check_der_test_interface, 0, 0, &gui_data,
			INSTANCE_VISIT_MATERIALISED_DERIVATIVES);
	}
	/* Force failure AFTER pointer installation, then restore and retry. */
	SetIntegerAtomValue(ChildByChar(other, AddSymbol("ode_type")), -1, 0);
	error_reporter_tree_start();
	b = system_build(root);
	error_reporter_tree_end();
	CU_ASSERT_PTR_NULL(b);
	if(b != NULL) system_destroy(b);
	SetIntegerAtomValue(ChildByChar(other, AddSymbol("ode_type")), 0, 0);
	SilentVisitInstanceTreeTwoWithCoverage(root, check_der_test_interface, 0, 0, &gui_data,
		INSTANCE_VISIT_MATERIALISED_DERIVATIVES);
	CU_ASSERT_PTR_EQUAL(find_derivative_var(a), held);
	solve_coexist_system(a, qrslv);
	b = system_build(root);
	CU_ASSERT_FATAL(b != NULL);
	if(destroy_first){ system_destroy(a); a = b; }
	else{ system_destroy(b); }
	solve_coexist_system(a, qrslv);
	system_destroy(a);
	SilentVisitInstanceTreeTwoWithCoverage(root, check_der_test_interface, 0, 0, &gui_data,
		INSTANCE_VISIT_MATERIALISED_DERIVATIVES);
	CU_ASSERT(gui_data == 1234567 && temporary_data == 7654321);
	PopInterfacePtrs(saved, NULL, NULL);
	CU_ASSERT_PTR_NULL(GetInterfacePtr(deriv));
	system_free_reused_mem();
	destroy_loaded_system(NULL, sim);
}

static void test_der_coexist_destroy_first(void){ run_der_system_coexist(1); }
static void test_der_coexist_destroy_second(void){ run_der_system_coexist(0); }

static void test_der_coexist_initial(void){
	/* This is a backend coexistence test, not the integrator's INITIAL
	 * lifecycle (which still replaces its caller's normal system). */
	struct Instance *sim = load_sim_for_model("test/ida/initial.a4c", "ida_initial_dae");
	struct Instance *root = GetSimulationRoot(sim);
	struct Instance *y = ChildByChar(root, AddSymbol("y"));
	struct Instance *deriv = InstancePeekDerivative(y);
	slv_system_t normal, initial;
	struct var_variable **normal_vars;
	struct rel_relation **normal_rels;
	struct gl_list_t *saved;
	int qrslv = ensure_qrslv_loaded(), gui_data = 42, r;
	slv_status_t status;
	CU_ASSERT_FATAL(deriv != NULL);
	normal = system_build(root);
	CU_ASSERT_FATAL(normal != NULL);
	CU_ASSERT_PTR_NULL(GetInterfacePtr(deriv)); /* NULL must also be restored */
	normal_vars = slv_get_solvers_var_list(normal);
	normal_rels = slv_get_solvers_rel_list(normal);
	saved = PushInterfacePtrsWithCoverage(root, der_test_interface, 0, 1, &gui_data,
		INSTANCE_VISIT_MATERIALISED_DERIVATIVES);
	CU_ASSERT_FATAL(saved != NULL);
	initial = system_build_with_mode(root, SYSTEM_BUILD_INITIAL);
	CU_ASSERT_FATAL(initial != NULL);
	CU_ASSERT(slv_select_solver(initial, qrslv) != -1);
	CU_ASSERT(0 == slv_presolve(initial));
	CU_ASSERT(0 == slv_solve(initial));
	slv_get_status(initial, &status);
	CU_ASSERT(status.converged);
	system_destroy(initial);
	system_set_build_mode(root, SYSTEM_BUILD_NORMAL);
	CU_ASSERT_PTR_EQUAL(slv_get_solvers_var_list(normal), normal_vars);
	CU_ASSERT_PTR_EQUAL(slv_get_solvers_rel_list(normal), normal_rels);
	CU_ASSERT_PTR_EQUAL(InstancePeekDerivative(y), deriv);
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(y), 1, 1e-9);
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(deriv), -2, 1e-9);
	for(r = 0; r < slv_get_num_solvers_rels(normal); ++r){
		int32 ok = 0;
		CU_ASSERT_DOUBLE_EQUAL(relman_eval(normal_rels[r], &ok, 1), 0, 1e-9);
		CU_ASSERT(ok);
	}
	system_destroy(normal);
	SilentVisitInstanceTreeTwoWithCoverage(root, check_der_test_interface, 0, 0, &gui_data,
		INSTANCE_VISIT_MATERIALISED_DERIVATIVES);
	PopInterfacePtrs(saved, NULL, NULL);
	system_free_reused_mem();
	destroy_loaded_system(NULL, sim);
}

#define TESTS(T) \
	T(der_visit_coverage) \
	T(der_coexist_destroy_first) \
	T(der_coexist_destroy_second) \
	T(der_coexist_initial) \
	T(der_expr_direct_ok) \
	T(der_equation_direct_ok) \
	T(der_only_direct_ok) \
	T(der_expr_nested_ok) \
	T(der_equation_nested_ok) \
	T(der_only_nested_ok) \
	T(der_only_array_direct_ok) \
	T(der_only_array_nested_ok) \
	T(der_implicit_mass_ok) \
	T(der_mixed_ok) \
	T(der_qlfdid_before_build_ok) \
	T(der_qlfdid_canonical_ok) \
	T(der_qlfdid_after_build_reuses_instance) \
	T(der_dynamic_child_api_ok) \
	T(der_method_fix_assign_before_build_ok) \
	T(der_method_fix_assign_ok) \
	T(der_method_free_ok) \
	T(der_qrslv_default_fixed_ok) \
	T(der_qrslv_free_ok) \
	T(der_qrslv_assign_nonzero_ok) \
	T(der_qrslv_fix_assign_nonzero_ok) \
	T(der_alias_scalar_ok) \
	T(der_alias_array_ok) \
	T(der_array_same_ok)

REGISTER_TESTS_SIMPLE(system_der, TESTS)
