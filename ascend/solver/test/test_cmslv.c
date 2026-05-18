#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <ascend/general/env.h>
#include <ascend/general/ospath.h>
#include <ascend/general/list.h>
#include <ascend/general/ltmatrix.h>

#include <ascend/general/platform.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/error.h>

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/relation_io.h>
#include <ascend/compiler/reverse_ad.h>
#include <ascend/compiler/relation_util.h>
#include <ascend/compiler/mathinst.h>
#include <ascend/compiler/watchpt.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/visitinst.h>
#include <ascend/compiler/functype.h>
#include <ascend/compiler/safe.h>
#include <ascend/compiler/qlfdid.h>
#include <ascend/compiler/instance_io.h>
#include <ascend/compiler/packages.h>

#include <ascend/compiler/slvreq.h>

#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>
#include <ascend/system/slv_param.h>
#include <ascend/solver/solver.h>
#include <ascend/system/slv_server.h>

#include <test/common.h>
//#define TCMSLV_DEBUG
#ifdef TCMSLV_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
#endif

struct cmslv_progress_capture{
	char *buffer;
	size_t cap;
	size_t len;
};

static struct cmslv_progress_capture g_cmslv_progress = {NULL,0,0};

static void cmslv_progress_begin(char *buffer, size_t cap){
	g_cmslv_progress.buffer = buffer;
	g_cmslv_progress.cap = cap;
	g_cmslv_progress.len = 0;
	if(buffer != NULL && cap > 0){
		buffer[0] = '\0';
	}
}

static void cmslv_progress_end(void){
	g_cmslv_progress.buffer = NULL;
	g_cmslv_progress.cap = 0;
	g_cmslv_progress.len = 0;
}

static int cmslv_capture_progress_callback(
	const char *solver_name, const char *message, void *user_data
){
	int wrote;
	char line[1024];
	(void)user_data;
	wrote = snprintf(
		line,sizeof(line),"solver=%s, %s"
		,(solver_name != NULL ? solver_name : "")
		,(message != NULL ? message : "")
	);
	if(wrote < 0 || g_cmslv_progress.buffer == NULL || g_cmslv_progress.cap == 0){
		return 0;
	}
	if(g_cmslv_progress.len >= g_cmslv_progress.cap - 1){
		return 0;
	}
	if((size_t)wrote >= g_cmslv_progress.cap - g_cmslv_progress.len){
		g_cmslv_progress.len = g_cmslv_progress.cap - 1;
		g_cmslv_progress.buffer[g_cmslv_progress.len] = '\0';
		return 0;
	}
	memcpy(g_cmslv_progress.buffer + g_cmslv_progress.len,line,(size_t)wrote);
	g_cmslv_progress.len += (size_t)wrote;
	if(g_cmslv_progress.len + 1 < g_cmslv_progress.cap){
		g_cmslv_progress.buffer[g_cmslv_progress.len++] = '\n';
		g_cmslv_progress.buffer[g_cmslv_progress.len] = '\0';
	}
	return 0;
}

static int cmslv_find_param_index(const slv_parameters_t *pp, const char *name){
	int i;
	for(i = 0; i < pp->num_parms; ++i){
		if(pp->parms[i].name != NULL && 0 == strcmp(pp->parms[i].name,name)){
			return i;
		}
	}
	return -1;
}

static int cmslv_set_char_param(slv_system_t sys, const char *name, const char *value){
	slv_parameters_t params;
	int idx;
	slv_get_parameters(sys,&params);
	idx = cmslv_find_param_index(&params,name);
	if(idx < 0 || SLV_PARAM_TYPE(&params,idx) != char_parm){
		return 1;
	}
	slv_set_char_parameter(&(SLV_PARAM_CHAR(&params,idx)),value);
	slv_set_parameters(sys,&params);
	return 0;
}

static int cmslv_load_required_package(const char *package){
	if(0 == package_load(package,NULL)){
		return 0;
	}
	CU_FAIL_FATAL("CMSlv test prerequisite solver package is not available.");
	return 1;
}

static int cmslv_load_optional_optimizer(const char *optsolver){
	const char *package;
	char message[160];

	if(optsolver == NULL){
		return 0;
	}
	if(strcmp(optsolver,"CONOPT") == 0){
		package = "conopt";
	}else if(strcmp(optsolver,"IPOPT") == 0){
		package = "ipopt";
	}else{
		return 0;
	}
	if(0 == package_load(package,NULL)){
		return 0;
	}
	snprintf(
		message,sizeof(message)
		,"CMSlv %s optimizer package is not available at runtime.",optsolver
	);
	ASC_TEST_PARTIAL_SKIP(message);
	return 1;
}

static int cmslv_optional_optimizer_selected(const char *optsolver){
	return optsolver != NULL
		&& (strcmp(optsolver,"CONOPT") == 0 || strcmp(optsolver,"IPOPT") == 0);
}

/*
	Test solving a simple CMSlv model
*/
static void test_cmslv(const char *filenamestem, const char *optsolver,
		int expect_boundary_progress
){

	struct module_t *m;
	struct Instance *siminst = NULL;
	slv_system_t sys = NULL;
	char progress[8192];

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv:solvers/lrslv:solvers/conopt:solvers/cmslv:solvers/ipopt");

	/* load the file */
	char path[PATH_MAX];
	strcpy((char *)path,"models/test/cmslv/");
	strncat(path, filenamestem, PATH_MAX - strlen(path));
	strncat(path, ".a4c", PATH_MAX - strlen(path));
	{
		int status;
		m = Asc_OpenModule(path,&status);
		if (!m) {
			MSG("test_cmslv: failed OpenModule");
		}
		CU_ASSERT(status == 0);
	}

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */
	CU_ASSERT(FindType(AddSymbol(filenamestem))!=NULL);

	/* instantiate it */
	siminst = SimsCreateInstance(AddSymbol(filenamestem), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst!=NULL);

    CONSOLE_DEBUG("RUNNING ON_LOADx");

	/** Call on_load */
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	/* assign solver */
	const char *solvername = "CMSlv";
	if(cmslv_load_required_package("lrslv")
		|| cmslv_load_required_package("qrslv")
		|| cmslv_load_required_package("cmslv")
		|| cmslv_load_optional_optimizer(optsolver)
	){
		sim_destroy(siminst);
		Asc_CompilerDestroy();
		return;
	}
	int index = slv_lookup_client(solvername);
	if(index == -1){
		sim_destroy(siminst);
		Asc_CompilerDestroy();
		CU_FAIL("CMSlv solver package loaded but did not register solver 'CMSlv'.");
		return;
	}

	sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);

	CU_ASSERT_FATAL(slv_select_solver(sys,index));
	CONSOLE_DEBUG("Assigned solver '%s'...",solvername);
	if(optsolver != NULL){
		CU_ASSERT_FATAL(0 == cmslv_set_char_param(sys,"optsolvers",optsolver));
	}

	{
		int presolve_status = slv_presolve(sys);
		if(presolve_status != 0 && cmslv_optional_optimizer_selected(optsolver)){
			char message[180];
			snprintf(
				message,sizeof(message)
				,"CMSlv %s optimizer path is not available for this build/runtime.",optsolver
			);
			ASC_TEST_PARTIAL_SKIP(message);
			if(sys)system_destroy(sys);
			system_free_reused_mem();
			solver_destroy_engines();
			sim_destroy(siminst);
			Asc_CompilerDestroy();
			return;
		}
		CU_ASSERT_FATAL(0 == presolve_status);
	}

	slv_status_t status;
	slv_get_status(sys, &status);
	CU_ASSERT_FATAL(status.ready_to_solve);

	cmslv_progress_begin(progress,sizeof(progress));
	slv_set_progress_callback(cmslv_capture_progress_callback,NULL);
	slv_solve(sys);
	slv_clear_progress_callback();
	cmslv_progress_end();
	slv_get_status(sys, &status);
	CU_ASSERT(status.ok);
	if(expect_boundary_progress){
		CU_ASSERT_PTR_NOT_NULL(strstr(progress,"solver=CMSlv"));
		CU_ASSERT_PTR_NOT_NULL(strstr(progress,"event=boundary_crossed"));
		CU_ASSERT_PTR_NOT_NULL(strstr(progress,"event=boundary_return_done"));
		CU_ASSERT_PTR_NOT_NULL(strstr(progress,"event=boundary_opt_start"));
		CU_ASSERT_PTR_NOT_NULL(strstr(progress,optsolver));
	}

	CONSOLE_DEBUG("Destroying system...");
	if(sys)system_destroy(sys);
	system_free_reused_mem();

	/* run 'self_test' method */
	CONSOLE_DEBUG("Running self-tests");
	name = CreateIdName(AddSymbol("self_test"));
	pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	/* destroy all that stuff */
	CONSOLE_DEBUG("Destroying instance tree");
	CU_ASSERT(siminst != NULL);

	solver_destroy_engines();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(linmassbal)\
	T(pipeline)\
	T(heatex)\
	T(reinitignore)\
	T(linmassbal_ipopt)\
	T(pipeline_ipopt)\
	T(heatex_ipopt)\
	T(reinitignore_ipopt)

static void test_linmassbal(void){ test_cmslv("linmassbal","CONOPT",1); }
static void test_pipeline(void){ test_cmslv("pipeline","CONOPT",1); }
static void test_heatex(void){ test_cmslv("heatex","CONOPT",1); }
static void test_reinitignore(void){ test_cmslv("reinitignore","CONOPT",0); }
static void test_linmassbal_ipopt(void){ test_cmslv("linmassbal","IPOPT",1); }
static void test_pipeline_ipopt(void){ test_cmslv("pipeline","IPOPT",1); }
static void test_heatex_ipopt(void){ test_cmslv("heatex","IPOPT",1); }
static void test_reinitignore_ipopt(void){ test_cmslv("reinitignore","IPOPT",0); }

REGISTER_TESTS_SIMPLE(solver_cmslv, TESTS);
