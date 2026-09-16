/* Shared LP export: independent of optional HiGHS/Gurobi plugins/licences. */
#include <limits.h>
#include <math.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/name.h>
#include <ascend/system/system.h>
#include <ascend/system/lp_utils.h>
#include <test/common.h>

struct fixture {
	struct Instance *sim;
	slv_system_t sys;
	mps_data_t m;
};

static int setup(struct fixture *f){
	int opened;
	struct Name *name;
	enum Proc_enum result;
	slv_status_t status = {0};
	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	/* Only instantiate the algebraic model: no Gurobi plugin is loaded. */
	Asc_OpenModule("models/test/gurobi/lp.a4c",&opened);
	if(opened || zz_parse())return 1;
	f->sim=SimsCreateInstance(AddSymbol("gurobi_offset"),AddSymbol("lp_export"),e_normal,NULL);
	if(!f->sim)return 1;
	name=CreateIdName(AddSymbol("on_load"));
	result=Initialize(GetSimulationRoot(f->sim),name,"lp_export",ASCERR,WP_STOPONERR,NULL,NULL);
	DestroyName(name);
	if(result!=Proc_all_ok)return 1;
	f->sys=system_build(GetSimulationRoot(f->sim));
	if(!f->sys)return 1;
	return lp_prepare(f->sys,&f->m,&status,0,0);
}

static void cleanup(struct fixture *f){
	lp_nuke_pointers(&f->m);
	if(f->sys)system_destroy(f->sys);
	system_free_reused_mem();
	if(f->sim)sim_destroy(f->sim);
	Asc_CompilerDestroy();
}

static int build(struct fixture *f, lp_sparse_t *p, const mps_data_t *m){
	return lp_sparse_build(p,m,slv_get_solvers_var_list(f->sys),slv_get_obj_relation(f->sys),-1e20,1e20);
}

static real64 coefficient(const lp_sparse_t *p, int col, int row){
	int k;
	for(k=p->start[col];k<p->start[col+1];++k){
		if(p->index[k]==row)return p->value[k];
	}
	return NAN;
}

static void test_permuted_sparse(void){
	struct fixture f={0};
	lp_sparse_t a={0}, b={0};
	int i, ready=setup(&f);
	CU_ASSERT(ready==0);
	if(ready)goto done;
	if(build(&f,&a,&f.m)){CU_FAIL("Baseline export failed");goto done;}
	mtx_swap_rows(f.m.Ac_mtx,0,f.m.crow);
	mtx_swap_cols(f.m.Ac_mtx,0,f.m.vused-1);
	if(build(&f,&b,&f.m)){CU_FAIL("Permuted export failed");goto done;}
	CU_ASSERT(a.num_col==b.num_col && a.num_row==b.num_row && a.num_nz==b.num_nz);
	if(a.num_col!=b.num_col || a.num_row!=b.num_row || a.num_nz!=b.num_nz)goto done;
	CU_ASSERT_DOUBLE_EQUAL(a.objective_offset,7,1e-10);
	CU_ASSERT_DOUBLE_EQUAL(b.objective_offset,7,1e-10);
	for(i=0;i<a.num_col;++i){
		int j;
		CU_ASSERT_DOUBLE_EQUAL(a.cost[i],b.cost[i],1e-10);
		CU_ASSERT(a.start[i+1]-a.start[i]==b.start[i+1]-b.start[i]);
		/* Matrix iteration order may change, so compare by compact row. */
		for(j=a.start[i];j<a.start[i+1];++j){
			real64 value=coefficient(&b,i,a.index[j]);
			CU_ASSERT(isfinite(value));
			CU_ASSERT_DOUBLE_EQUAL(a.value[j],value,1e-10);
		}
	}
done:
	lp_sparse_destroy(&a);lp_sparse_destroy(&b);cleanup(&f);
}

static void test_invalid_sparse_dimensions(void){
	struct fixture f={0};
	lp_sparse_t p={0};
	int i, ready=setup(&f);
	CU_ASSERT(ready==0);
	if(ready)goto done;
	CU_ASSERT(build(&f,NULL,&f.m)!=0);
	for(i=0;i<11;++i){
		mps_data_t bad=f.m;
		switch(i){
		case 0: bad.vused=-1;break;
		case 1: bad.vused=0;break;
		case 2: bad.vused=INT_MAX;break;
		case 3: bad.rused=-1;break;
		case 4: bad.rused=0;break;
		case 5: bad.cap=0;break;
		case 6: bad.crow=-1;break;
		case 7: bad.crow=bad.cap;break;
		case 8: bad.crow=0;break;
		case 9: bad.relopcol=NULL;break;
		case 10: bad.cap=INT_MAX;break;
		default: CU_FAIL("Invalid dimension test case");goto done;
		}
		CU_ASSERT(build(&f,&p,&f.m)==0); /* Rejection also releases old output. */
		CU_ASSERT(build(&f,&p,&bad)!=0);
		CU_ASSERT(p.start==NULL && p.num_col==0 && p.num_nz==0);
	}
done:
	lp_sparse_destroy(&p);cleanup(&f);
}

static void test_empty_and_nonfinite_sparse(void){
	struct fixture f={0};
	lp_sparse_t p={0};
	int i, ready=setup(&f);
	CU_ASSERT(ready==0);
	if(ready)goto done;
	f.m.bcol[0]=NAN;
	CU_ASSERT(build(&f,&p,&f.m)!=0);
	CU_ASSERT(p.start==NULL);
	for(i=0;i<f.m.rused;++i)f.m.relopcol[i]=0;
	CU_ASSERT(build(&f,&p,&f.m)==0);
	CU_ASSERT(p.num_row==0 && p.num_nz==0);
	if(p.start)CU_ASSERT(p.start[p.num_col]==0);
done:
	lp_sparse_destroy(&p);cleanup(&f);
}

#define TESTS(T) T(permuted_sparse) T(invalid_sparse_dimensions) T(empty_and_nonfinite_sparse)
REGISTER_TESTS_SIMPLE(system_lp_export,TESTS)
