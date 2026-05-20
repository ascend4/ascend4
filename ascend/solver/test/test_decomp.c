#include <string.h>

#include <ascend/general/env.h>
#include <ascend/general/ospath.h>
#include <ascend/utilities/ascEnvVar.h>

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/watchpt.h>

#include <ascend/system/block.h>
#include <ascend/system/decomp.h>
#include <ascend/system/discrete.h>
#include <ascend/system/logrel.h>
#include <ascend/system/rel.h>
#include <ascend/system/slv_client.h>
#include <ascend/system/system.h>
#include <ascend/system/var.h>

#include <test/common.h>

struct decomp_fixture {
	struct Instance *siminst;
	struct Instance *root;
	slv_system_t sys;
	slv_decomp_partition_t decomp;
};

static void decomp_fixture_init(struct decomp_fixture *fx){
	memset(fx,0,sizeof(*fx));
	slv_decomp_init(&fx->decomp);
}

static void decomp_fixture_destroy(struct decomp_fixture *fx){
	slv_decomp_destroy(&fx->decomp);
	if(fx->sys != NULL){
		system_destroy(fx->sys);
	}
	system_free_reused_mem();
	if(fx->siminst != NULL){
		sim_destroy(fx->siminst);
	}
	Asc_CompilerDestroy();
}

static void decomp_load_model_expect(
		const char *modelname, struct decomp_fixture *fx, int decomp_status
){
	int status;
	struct module_t *m;
	struct Name *name;
	enum Proc_enum pe;

	decomp_fixture_init(fx);
	Asc_CompilerInit(1);
	CU_ASSERT_FATAL(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));

	m = Asc_OpenModule("test/decomp/block_cases.a4c",&status);
	CU_ASSERT_PTR_NOT_NULL_FATAL(m);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol(modelname)) != NULL);

	fx->siminst = SimsCreateInstance(
		AddSymbol(modelname),AddSymbol("sim1"),e_normal,NULL
	);
	CU_ASSERT_PTR_NOT_NULL_FATAL(fx->siminst);
	fx->root = GetSimulationRoot(fx->siminst);
	CU_ASSERT_PTR_NOT_NULL_FATAL(fx->root);

	name = CreateIdName(AddSymbol("on_load"));
	pe = Initialize(fx->root,name,"sim1",ASCERR,WP_STOPONERR,NULL,NULL);
	CU_ASSERT_FATAL(pe == Proc_all_ok);

	fx->sys = system_build(fx->root);
	CU_ASSERT_PTR_NOT_NULL_FATAL(fx->sys);
	CU_ASSERT_FATAL(decomp_status == slv_decomp_partition(fx->sys,&fx->decomp));
}

static void decomp_load_model(const char *modelname, struct decomp_fixture *fx){
	decomp_load_model_expect(modelname,fx,0);
}

static struct Instance *decomp_child(struct decomp_fixture *fx, const char *name){
	struct Instance *child = ChildByChar(fx->root,AddSymbol(name));
	CU_ASSERT_PTR_NOT_NULL_FATAL(child);
	return child;
}

static int decomp_var_org_col(struct decomp_fixture *fx, const char *name){
	struct Instance *inst = decomp_child(fx,name);
	struct var_variable **vars = slv_get_solvers_var_list(fx->sys);
	int32 i, n = slv_get_num_solvers_vars(fx->sys);
	for(i = 0; i < n; ++i){
		if(var_instance(vars[i]) == inst){
			return i;
		}
	}
	CU_FAIL_FATAL("variable was not found in solver variable list");
	return -1;
}

static int decomp_dvar_org_col(struct decomp_fixture *fx, const char *name){
	struct Instance *inst = decomp_child(fx,name);
	struct dis_discrete **dvars = slv_get_solvers_dvar_list(fx->sys);
	int32 i, n = slv_get_num_solvers_dvars(fx->sys);
	for(i = 0; i < n; ++i){
		if(dis_instance(dvars[i]) == inst){
			return fx->decomp.n_vars + i;
		}
	}
	CU_FAIL_FATAL("discrete variable was not found in solver discrete list");
	return -1;
}

static struct dis_discrete *decomp_dvar_by_name(
		struct decomp_fixture *fx, const char *name
){
	struct Instance *inst = decomp_child(fx,name);
	struct dis_discrete **dvars = slv_get_solvers_dvar_list(fx->sys);
	int32 i, n = slv_get_num_solvers_dvars(fx->sys);
	for(i = 0; i < n; ++i){
		if(dis_instance(dvars[i]) == inst){
			return dvars[i];
		}
	}
	CU_FAIL_FATAL("discrete variable was not found in solver discrete list");
	return NULL;
}

static int decomp_rel_org_row(struct decomp_fixture *fx, const char *name){
	struct Instance *inst = decomp_child(fx,name);
	struct rel_relation **rels = slv_get_solvers_rel_list(fx->sys);
	struct rel_relation **condrels = slv_get_solvers_condrel_list(fx->sys);
	int32 i, n = slv_get_num_solvers_rels(fx->sys);
	for(i = 0; i < n; ++i){
		if(rel_instance(rels[i]) == inst){
			return i;
		}
	}
	n = slv_get_num_solvers_condrels(fx->sys);
	for(i = 0; i < n; ++i){
		if(rel_instance(condrels[i]) == inst){
			return fx->decomp.n_rels + i;
		}
	}
	CU_FAIL_FATAL("relation was not found in solver relation lists");
	return -1;
}

static int decomp_logrel_org_row(struct decomp_fixture *fx, const char *name){
	struct Instance *inst = decomp_child(fx,name);
	struct logrel_relation **logrels = slv_get_solvers_logrel_list(fx->sys);
	struct logrel_relation **condlogrels = slv_get_solvers_condlogrel_list(fx->sys);
	int32 i, n = slv_get_num_solvers_logrels(fx->sys);
	for(i = 0; i < n; ++i){
		if(logrel_instance(logrels[i]) == inst){
			return fx->decomp.n_rels + fx->decomp.n_condrels + i;
		}
	}
	n = slv_get_num_solvers_condlogrels(fx->sys);
	for(i = 0; i < n; ++i){
		if(logrel_instance(condlogrels[i]) == inst){
			return fx->decomp.n_rels + fx->decomp.n_condrels
				+ fx->decomp.n_logrels + i;
		}
	}
	CU_FAIL_FATAL("logrelation was not found in solver logrelation lists");
	return -1;
}

static int decomp_has_edge(
		const slv_decomp_partition_t *decomp, int orgrow, int orgcol
){
	int32 i;
	for(i = 0; i < decomp->nnz; ++i){
		if(decomp->nz_rows[i] == orgrow && decomp->nz_cols[i] == orgcol){
			return 1;
		}
	}
	return 0;
}

static void test_real_matches_qrslv_blocks(void){
	struct decomp_fixture fx;
	const mtx_block_t *realblocks;
	decomp_load_model("real_chain",&fx);
	CU_ASSERT(fx.decomp.n_rels == 2);
	CU_ASSERT(fx.decomp.n_condrels == 0);
	CU_ASSERT(fx.decomp.n_logrels == 0);
	CU_ASSERT(fx.decomp.n_dvars == 0);
	CU_ASSERT(fx.decomp.nblocks == 2);
	CU_ASSERT_FATAL(0 == slv_block_partition(fx.sys));
	realblocks = slv_get_solvers_blocks(fx.sys);
	CU_ASSERT_PTR_NOT_NULL_FATAL(realblocks);
	CU_ASSERT_EQUAL(realblocks->nblocks,fx.decomp.nblocks);
	decomp_fixture_destroy(&fx);
}

static void test_boundary_and_when_edges(void){
	struct decomp_fixture fx;
	int logrow, xcol, posrow, bcol;
	decomp_load_model("boundary_when",&fx);
	CU_ASSERT(fx.decomp.n_vars >= 2);
	CU_ASSERT(fx.decomp.n_dvars >= 1);
	CU_ASSERT(fx.decomp.n_logrels >= 1);

	logrow = decomp_logrel_org_row(&fx,"l");
	xcol = decomp_var_org_col(&fx,"x");
	bcol = decomp_dvar_org_col(&fx,"b");
	CU_ASSERT_TRUE(decomp_has_edge(&fx.decomp,logrow,bcol));
	CU_ASSERT_TRUE(decomp_has_edge(&fx.decomp,logrow,xcol));

	posrow = decomp_rel_org_row(&fx,"pos");
	CU_ASSERT_TRUE(decomp_has_edge(&fx.decomp,posrow,bcol));
	decomp_fixture_destroy(&fx);
}

static void test_active_partition_prunes_when_rows(void){
	struct decomp_fixture fx;
	slv_decomp_partition_t active;
	int posrow, negrow, xcol, ycol, bcol;

	decomp_load_model("boundary_when",&fx);
	slv_decomp_init(&active);
	CU_ASSERT_EQUAL_FATAL(slv_decomp_partition_active(fx.sys,&active),0);

	posrow = decomp_rel_org_row(&fx,"pos");
	negrow = decomp_rel_org_row(&fx,"neg");
	xcol = decomp_var_org_col(&fx,"x");
	ycol = decomp_var_org_col(&fx,"y");
	bcol = decomp_dvar_org_col(&fx,"b");

	CU_ASSERT_TRUE(decomp_has_edge(&fx.decomp,posrow,bcol));
	CU_ASSERT_FALSE(decomp_has_edge(&active,posrow,bcol));
	CU_ASSERT_TRUE(decomp_has_edge(&active,posrow,xcol));
	CU_ASSERT_TRUE(decomp_has_edge(&active,posrow,ycol));
	CU_ASSERT_FALSE(decomp_has_edge(&active,negrow,bcol));
	CU_ASSERT_FALSE(decomp_has_edge(&active,negrow,xcol));
	CU_ASSERT_FALSE(decomp_has_edge(&active,negrow,ycol));

	slv_decomp_destroy(&active);
	decomp_fixture_destroy(&fx);
}

static void test_fixed_selector_is_not_coupling_edge(void){
	struct decomp_fixture fx;
	int posrow, bcol;
	decomp_load_model("fixed_selector_when",&fx);
	posrow = decomp_rel_org_row(&fx,"pos");
	bcol = decomp_dvar_org_col(&fx,"b");
	CU_ASSERT_FALSE(decomp_has_edge(&fx.decomp,posrow,bcol));
	decomp_fixture_destroy(&fx);
}

static void test_integer_when_selector(void){
	struct decomp_fixture fx;
	struct dis_discrete *mode;
	int onerow, modecol;
	int local;
	decomp_load_model("integer_selector_when",&fx);
	mode = decomp_dvar_by_name(&fx,"mode");
	CU_ASSERT_PTR_NOT_NULL_FATAL(mode);
	CU_ASSERT_EQUAL(dis_kind(mode),e_dis_integer_t);
	modecol = decomp_dvar_org_col(&fx,"mode");
	CU_ASSERT_EQUAL(slv_decomp_col_kind(&fx.decomp,modecol,&local),
		slv_decomp_col_dvar);
	onerow = decomp_rel_org_row(&fx,"one");
	CU_ASSERT_TRUE(decomp_has_edge(&fx.decomp,onerow,modecol));
	decomp_fixture_destroy(&fx);
}

static void test_solver_int_relation_column(void){
	struct decomp_fixture fx;
	int rrow, icol;
	struct var_variable **vars;
	decomp_load_model("solver_int_relation",&fx);
	rrow = decomp_rel_org_row(&fx,"r");
	icol = decomp_var_org_col(&fx,"i");
	vars = slv_get_solvers_var_list(fx.sys);
	CU_ASSERT_TRUE(var_flags(vars[icol]) & VAR_INTEGER);
	CU_ASSERT_TRUE(decomp_has_edge(&fx.decomp,rrow,icol));
	decomp_fixture_destroy(&fx);
}

static void test_public_kind_helpers(void){
	struct decomp_fixture fx;
	int i, local;
	int rrow, logrow, xcol, bcol;

	decomp_load_model("boundary_when",&fx);
	rrow = decomp_rel_org_row(&fx,"pos");
	logrow = decomp_logrel_org_row(&fx,"l");
	xcol = decomp_var_org_col(&fx,"x");
	bcol = decomp_dvar_org_col(&fx,"b");

	CU_ASSERT_EQUAL(slv_decomp_row_kind(&fx.decomp,rrow,&local),
		slv_decomp_row_rel);
	CU_ASSERT_EQUAL(local,rrow);
	CU_ASSERT_EQUAL(slv_decomp_row_kind(&fx.decomp,fx.decomp.n_rels,&local),
		slv_decomp_row_condrel);
	CU_ASSERT_EQUAL(local,0);
	CU_ASSERT_EQUAL(slv_decomp_row_kind(&fx.decomp,logrow,&local),
		slv_decomp_row_logrel);
	CU_ASSERT_EQUAL(local,0);
	CU_ASSERT_EQUAL(slv_decomp_row_kind(&fx.decomp,-1,&local),
		slv_decomp_row_invalid);
	CU_ASSERT_EQUAL(slv_decomp_row_kind(NULL,rrow,&local),
		slv_decomp_row_invalid);

	CU_ASSERT_EQUAL(slv_decomp_col_kind(&fx.decomp,xcol,&local),
		slv_decomp_col_var);
	CU_ASSERT_EQUAL(local,xcol);
	CU_ASSERT_EQUAL(slv_decomp_col_kind(&fx.decomp,bcol,&local),
		slv_decomp_col_dvar);
	CU_ASSERT_EQUAL(local,bcol - fx.decomp.n_vars);
	CU_ASSERT_EQUAL(slv_decomp_col_kind(&fx.decomp,fx.decomp.n_cols,&local),
		slv_decomp_col_invalid);
	CU_ASSERT_EQUAL(slv_decomp_col_kind(NULL,xcol,&local),
		slv_decomp_col_invalid);
	for(i = 0; i < fx.decomp.n_rows; ++i){
		CU_ASSERT_EQUAL(fx.decomp.row_cur[fx.decomp.row_org[i]],i);
	}
	for(i = 0; i < fx.decomp.n_cols; ++i){
		CU_ASSERT_EQUAL(fx.decomp.col_cur[fx.decomp.col_org[i]],i);
	}

	decomp_fixture_destroy(&fx);
}

static void test_conditional_logrel_row_kind(void){
	struct decomp_fixture fx;
	int logrow, local;
	decomp_load_model("conditional_logrel",&fx);
	CU_ASSERT_EQUAL(fx.decomp.n_logrels,0);
	CU_ASSERT_EQUAL(fx.decomp.n_condlogrels,1);
	logrow = decomp_logrel_org_row(&fx,"l");
	CU_ASSERT_EQUAL(slv_decomp_row_kind(&fx.decomp,logrow,&local),
		slv_decomp_row_condlogrel);
	CU_ASSERT_EQUAL(local,0);
	decomp_fixture_destroy(&fx);
}

static void test_conditional_relation_row_kind(void){
	struct decomp_fixture fx;
	int row, local;
	decomp_load_model("conditional_relation",&fx);
	CU_ASSERT_EQUAL(fx.decomp.n_condrels,1);
	row = decomp_rel_org_row(&fx,"c");
	CU_ASSERT_EQUAL(slv_decomp_row_kind(&fx.decomp,row,&local),
		slv_decomp_row_condrel);
	decomp_fixture_destroy(&fx);
}

static void test_when_logrel_edges(void){
	struct decomp_fixture fx;
	int logrow, acol;
	decomp_load_model("when_logrel",&fx);
	logrow = decomp_logrel_org_row(&fx,"ltrue");
	acol = decomp_dvar_org_col(&fx,"a");
	CU_ASSERT_TRUE(decomp_has_edge(&fx.decomp,logrow,acol));
	decomp_fixture_destroy(&fx);
}

static void test_null_inputs(void){
	slv_decomp_partition_t decomp;

	slv_decomp_init(NULL);
	slv_decomp_destroy(NULL);
	slv_decomp_init(&decomp);
	CU_ASSERT_EQUAL(slv_decomp_partition(NULL,&decomp),1);
	CU_ASSERT_EQUAL(slv_decomp_partition_active(NULL,&decomp),1);
	CU_ASSERT_EQUAL(slv_decomp_partition((slv_system_t)NULL,NULL),1);
	CU_ASSERT_EQUAL(slv_decomp_partition_active((slv_system_t)NULL,NULL),1);
	slv_decomp_destroy(&decomp);
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(real_matches_qrslv_blocks) \
	T(boundary_and_when_edges) \
	T(active_partition_prunes_when_rows) \
	T(fixed_selector_is_not_coupling_edge) \
	T(integer_when_selector) \
	T(solver_int_relation_column) \
	T(public_kind_helpers) \
	T(conditional_logrel_row_kind) \
	T(conditional_relation_row_kind) \
	T(when_logrel_edges) \
	T(null_inputs)

REGISTER_TESTS_SIMPLE(solver_decomp, TESTS)
