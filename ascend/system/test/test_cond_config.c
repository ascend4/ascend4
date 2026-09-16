/* Regression for case-list growth beyond its initial 40 slots. */
#include <ascend/general/ascMalloc.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/instquery.h>
#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>
#include <ascend/system/cond_config.h>
#include <test/common.h>

static void test_case_list_growth(void){
	int status, n=0, i, seen[171]={0};
	int32 *matches=NULL;
	struct Instance *sim=NULL;
	slv_system_t sys=NULL;
	struct gl_list_t *vars=NULL;
	struct dis_discrete **dv;
	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_OpenModule("test/compiler/many_when_cases.a4c",&status);
	CU_ASSERT(status==0);
	if(status)goto cleanup;
	status=zz_parse();
	CU_ASSERT(status==0);
	if(status)goto cleanup;
	sim=SimsCreateInstance(AddSymbol("many_when_cases"),AddSymbol("sim1"),e_normal,NULL);
	CU_ASSERT(sim!=NULL);
	if(!sim)goto cleanup;
	sys=system_build(GetSimulationRoot(sim));
	CU_ASSERT(sys!=NULL);
	if(!sys)goto cleanup;
	vars=gl_create(85);
	for(dv=slv_get_master_dvar_list(sys);*dv;++dv)gl_append_ptr(vars,*dv);
	CU_ASSERT(gl_length(vars)==85);
	matches=cases_matching(vars,&n);
	CU_ASSERT(matches!=NULL && n==85);
	if(!matches || n!=85)goto cleanup;
	CU_ASSERT(matches[0]==0);
	for(i=1;i<=n;++i){
		int c=matches[i];
		CU_ASSERT(c>0 && c<171);
		if(c<=0 || c>=171)continue;
		CU_ASSERT(c%2==1); /* TRUE is the first case of every WHEN. */
		CU_ASSERT(seen[c]==0);
		seen[c]=1;
	}
cleanup:
	ASC_FREE(matches);
	if(vars)gl_destroy(vars);
	if(sys)system_destroy(sys);
	system_free_reused_mem();
	if(sim)sim_destroy(sim);
	Asc_CompilerDestroy();
}

#define TESTS(T) T(case_list_growth)
REGISTER_TESTS_SIMPLE(system_cond_config,TESTS)
