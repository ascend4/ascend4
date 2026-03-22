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
	qrslv_index = slv_lookup_client("QRSlv");
	if(qrslv_index == -1){
		CU_ASSERT_FATAL(0 == package_load("qrslv",NULL));
		qrslv_index = slv_lookup_client("QRSlv");
	}
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
	assert_ida_relation_count(sys,1);
	assert_included_equality_relation_count(sys,1);
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
	assert_ida_relation_count(sys,1);
	assert_included_equality_relation_count(sys,1);
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

#define TESTS(T) \
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
	T(der_alias_scalar_ok) \
	T(der_alias_array_ok) \
	T(der_array_same_ok)

REGISTER_TESTS_SIMPLE(system_der, TESTS)
