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

enum cmslv_progress_expect {
	CMSLV_PROGRESS_NONE,
	CMSLV_PROGRESS_BOUNDARY,
	CMSLV_PROGRESS_LINMASSBAL_CMSLV2,
	CMSLV_PROGRESS_GENERAL_CMSLV2,
	CMSLV_PROGRESS_SCHEDULER_CMSLV2,
	CMSLV_PROGRESS_BOUNDARY_LOCAL_CMSLV2,
	CMSLV_PROGRESS_BOUNDARY_LOCAL_COMPLETE_CMSLV2,
	CMSLV_PROGRESS_FLUIDBED_SWITCH_CMSLV2
};

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

static int cmslv_progress_event_field_int(
		const char *progress, const char *event, const char *field,
		long *value
){
	char key[96];
	const char *p;
	size_t event_len;
	size_t key_len;

	if(progress == NULL || event == NULL || field == NULL || value == NULL){
		return 0;
	}
	if(snprintf(key,sizeof(key),"%s=",field) >= (int)sizeof(key)){
		return 0;
	}
	event_len = strlen(event);
	key_len = strlen(key);
	for(p = progress; (p = strstr(p,event)) != NULL; p += event_len){
		const char *line_end = strchr(p,'\n');
		const char *field_pos = strstr(p,key);
		char *endptr;
		long parsed;
		if(field_pos == NULL || (line_end != NULL && field_pos > line_end)){
			continue;
		}
		parsed = strtol(field_pos + key_len,&endptr,10);
		if(endptr == field_pos + key_len){
			continue;
		}
		*value = parsed;
		return 1;
	}
	return 0;
}

static int cmslv_progress_event_field_at_least(
		const char *progress, const char *event, const char *field, long min
){
	long value;
	return cmslv_progress_event_field_int(progress,event,field,&value)
		&& value >= min;
}

static int cmslv_progress_event_field_equals(
		const char *progress, const char *event, const char *field, long expected
){
	long value;
	return cmslv_progress_event_field_int(progress,event,field,&value)
		&& value == expected;
}

static int cmslv_progress_contains_ordered(
		const char *progress, const char **needles, int nneedles
){
	const char *p = progress;
	int i;
	if(progress == NULL || needles == NULL){
		return 0;
	}
	for(i = 0; i < nneedles; ++i){
		p = strstr(p,needles[i]);
		if(p == NULL){
			return 0;
		}
		p += strlen(needles[i]);
	}
	return 1;
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

static int cmslv_set_bool_param(slv_system_t sys, const char *name, int value){
	slv_parameters_t params;
	int idx;
	slv_get_parameters(sys,&params);
	idx = cmslv_find_param_index(&params,name);
	if(idx < 0 || SLV_PARAM_TYPE(&params,idx) != bool_parm){
		return 1;
	}
	SLV_PARAM_BOOL(&params,idx) = value ? 1 : 0;
	slv_set_parameters(sys,&params);
	return 0;
}

static int cmslv_load_required_package(const char *package){
	char message[160];
	if(0 == package_load(package,NULL)){
		return 0;
	}
	snprintf(
		message,sizeof(message)
		,"CMSlv test prerequisite solver package '%s' is not available.",package
	);
	CU_FAIL_FATAL(message);
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
static void test_cmslv_mode(const char *filenamestem, const char *optsolver,
		enum cmslv_progress_expect progress_expect
){

	struct module_t *m;
	struct Instance *siminst = NULL;
	slv_system_t sys = NULL;
	char progress[65536];
	int cmslv2 = progress_expect == CMSLV_PROGRESS_LINMASSBAL_CMSLV2
		|| progress_expect == CMSLV_PROGRESS_GENERAL_CMSLV2
		|| progress_expect == CMSLV_PROGRESS_SCHEDULER_CMSLV2
		|| progress_expect == CMSLV_PROGRESS_BOUNDARY_LOCAL_CMSLV2
		|| progress_expect == CMSLV_PROGRESS_BOUNDARY_LOCAL_COMPLETE_CMSLV2
		|| progress_expect == CMSLV_PROGRESS_FLUIDBED_SWITCH_CMSLV2;
	int expect_boundary_progress = progress_expect == CMSLV_PROGRESS_BOUNDARY
		|| progress_expect == CMSLV_PROGRESS_LINMASSBAL_CMSLV2;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS
		"=solvers/qrslv" OSPATH_DIV
		"solvers/lrslv" OSPATH_DIV
		"solvers/conopt" OSPATH_DIV
		"solvers/cmslv" OSPATH_DIV
		"solvers/ipopt"
	);

	if(cmslv_load_required_package("lrslv")
		|| cmslv_load_required_package("qrslv")
		|| cmslv_load_required_package("cmslv")
		|| cmslv_load_optional_optimizer(optsolver)
	){
		Asc_CompilerDestroy();
		return;
	}

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
	if(progress_expect == CMSLV_PROGRESS_FLUIDBED_SWITCH_CMSLV2){
		CU_ASSERT(pe == Proc_all_ok || pe == Proc_slvreq_unhooked);
	}else{
		CU_ASSERT(pe==Proc_all_ok);
	}

	/* assign solver */
	const char *solvername = "CMSlv";
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
	if(cmslv2){
		CU_ASSERT_FATAL(0 == cmslv_set_bool_param(sys,"cmslv2",1));
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
	if(expect_boundary_progress
			|| progress_expect == CMSLV_PROGRESS_SCHEDULER_CMSLV2
			|| progress_expect == CMSLV_PROGRESS_BOUNDARY_LOCAL_CMSLV2
			|| progress_expect == CMSLV_PROGRESS_FLUIDBED_SWITCH_CMSLV2){
		CU_ASSERT_PTR_NOT_NULL(strstr(progress,"solver=CMSlv"));
		if(expect_boundary_progress){
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,"event=decomp_assess"));
		}
		CU_ASSERT_PTR_NOT_NULL(strstr(progress,"event=decomp_partition"));
		CU_ASSERT_PTR_NOT_NULL(strstr(progress,"event=cmslv2_plan"));
		CU_ASSERT_PTR_NOT_NULL(strstr(progress,"qrslv_blocks="));
		CU_ASSERT_PTR_NOT_NULL(strstr(progress,"lrslv_blocks="));
		CU_ASSERT_PTR_NOT_NULL(strstr(progress,"active_recommended="));
		CU_ASSERT_PTR_NOT_NULL(strstr(progress,"structural_safe="));
		if(progress_expect == CMSLV_PROGRESS_LINMASSBAL_CMSLV2
				|| progress_expect == CMSLV_PROGRESS_GENERAL_CMSLV2){
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,"cmslv2=1"));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,"event=cmslv2_scheduler"));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,"event=cmslv2_qrslv_handoff"));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,"event=nl_presolved"));
			if(progress_expect == CMSLV_PROGRESS_LINMASSBAL_CMSLV2){
				CU_ASSERT_PTR_NOT_NULL(strstr(progress,"action=defer_subsolver"));
				CU_ASSERT_PTR_NOT_NULL(strstr(progress,"mode=fallback"));
				CU_ASSERT_PTR_NOT_NULL(strstr(progress,"event=cmslv2_selector_consume"));
				CU_ASSERT_PTR_NOT_NULL(strstr(progress,"event=cmslv2_selector_search"));
				CU_ASSERT_PTR_NOT_NULL(strstr(progress,"event=cmslv2_selector_qrslv"));
				CU_ASSERT_PTR_NOT_NULL(strstr(progress,"phase=post_logic"));
				CU_ASSERT_PTR_NOT_NULL(strstr(progress,"mode=branch_search"));
				CU_ASSERT_PTR_NOT_NULL(strstr(progress,"tried="));
				CU_ASSERT_PTR_NOT_NULL(strstr(progress,"accepted="));
				CU_ASSERT_PTR_NOT_NULL(strstr(progress,"converged="));
				CU_ASSERT_PTR_NOT_NULL(strstr(progress,"matched_cases="));
				CU_ASSERT_PTR_NOT_NULL(strstr(progress,"active_subblocks="));
				CU_ASSERT_PTR_NOT_NULL(strstr(progress,"qrslv_subblocks="));
				CU_ASSERT_PTR_NOT_NULL(strstr(progress,"event=cmslv2_boundary"));
				CU_ASSERT_PTR_NOT_NULL(strstr(progress,"event=cmslv2_boundary_summary"));
				CU_ASSERT_PTR_NOT_NULL(strstr(progress,"mode=full_cmslv_boundary"));
				CU_ASSERT(cmslv_progress_event_field_at_least(
					progress,"event=cmslv2_plan","qrslv_blocks",4
				));
				CU_ASSERT(cmslv_progress_event_field_at_least(
					progress,"event=cmslv2_plan","lrslv_blocks",12
				));
				CU_ASSERT(cmslv_progress_event_field_at_least(
					progress,"event=cmslv2_plan","enum_blocks",3
				));
				CU_ASSERT(cmslv_progress_event_field_at_least(
					progress,"event=cmslv2_plan","boundary_blocks",1
				));
				CU_ASSERT(cmslv_progress_event_field_equals(
					progress,"event=cmslv2_qrslv_handoff","installed_blocks",0
				));
				CU_ASSERT(cmslv_progress_event_field_at_least(
					progress,
					"event=cmslv2_boundary_summary phase=boundary_at_zero action=optimize_done",
					"n_subregions",2
				));
			}
		}else if(progress_expect == CMSLV_PROGRESS_FLUIDBED_SWITCH_CMSLV2){
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,"cmslv2=1"));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,"driver=cmslv2"));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,"event=cmslv2_qrslv_solve"));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,"mode=fallback"));
			CU_ASSERT(cmslv_progress_event_field_at_least(
				progress,"event=cmslv2_plan","qrslv_blocks",8
			));
			CU_ASSERT(cmslv_progress_event_field_at_least(
				progress,"event=cmslv2_plan","lrslv_blocks",4
			));
		}else if(progress_expect == CMSLV_PROGRESS_SCHEDULER_CMSLV2){
			const char *scheduler_sequence[] = {
				"event=cmslv2_selector_consume phase=post_logic block=0",
				"event=cmslv2_qrslv_handoff phase=nl_presolve mode=external_blocks structural_start=1 structural_end=1",
				"event=boundary_return_done",
				"event=cmslv2_selector_consume phase=post_logic block=2",
				"event=cmslv2_lrslv_handoff phase=nl_presolve structural_start=3 structural_end=4",
				"event=cmslv2_qrslv_handoff phase=nl_presolve mode=fallback structural_start=-1 structural_end=-1 next_structural=5"
			};
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,"cmslv2=1"));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,"driver=cmslv2"));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_selector_qrslv phase=post_logic block=0 installed_blocks=1 installed_rows=1 installed_cols=1 converged=1 ok=1"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_selector_search phase=post_logic block=0 mode=branch_search dvars=1 tried=1 accepted=1"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_transition phase=post_logic solver=selector from_structural=0 next_structural=1 action=reanalyzed"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_qrslv_handoff phase=nl_presolve mode=external_blocks structural_start=1 structural_end=1 next_structural=2 installed_blocks=1 installed_rows=1 installed_cols=1"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_qrslv_solve iter=1 blocks=1 next_structural=2 pending_after=1"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_boundary_summary phase=boundary_crossed action=return_done mode=full_cmslv_boundary"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,"event=boundary_opt_start"));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_boundary_summary phase=boundary_at_zero action=optimize_done mode=full_cmslv_boundary"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_selector_search phase=post_logic block=2 mode=branch_search dvars=1 tried=1 accepted=1"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_transition phase=post_logic solver=selector from_structural=2 next_structural=3 action=reanalyzed"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_selector_consume phase=post_logic block=2 dvars=1 matched_cases=1 active_subblocks=1 qrslv_subblocks=1 lrslv_subblocks=0 unresolved_subblocks=0 action=advance next_structural=3"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_lrslv_handoff phase=nl_presolve structural_start=3 structural_end=4 installed_blocks=2 installed_rows=2 installed_cols=2 converged=1 ok=1"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_scheduler phase=nl_presolve structural_start=3 structural_end=4 action=lrslv_advance"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_qrslv_handoff phase=nl_presolve mode=fallback structural_start=-1 structural_end=-1 next_structural=5 installed_blocks=0"
			));
			CU_ASSERT(cmslv_progress_contains_ordered(
				progress,scheduler_sequence,
				sizeof(scheduler_sequence) / sizeof(scheduler_sequence[0])
			));
			CU_ASSERT(cmslv_progress_event_field_equals(
				progress,"event=cmslv2_plan","structural_blocks",5
			));
			CU_ASSERT(cmslv_progress_event_field_equals(
				progress,"event=cmslv2_plan","active_blocks",7
			));
			CU_ASSERT(cmslv_progress_event_field_equals(
				progress,"event=cmslv2_plan","qrslv_blocks",3
			));
			CU_ASSERT(cmslv_progress_event_field_equals(
				progress,"event=cmslv2_plan","lrslv_blocks",4
			));
			CU_ASSERT(cmslv_progress_event_field_equals(
				progress,"event=cmslv2_plan","enum_blocks",2
			));
			CU_ASSERT(cmslv_progress_event_field_equals(
				progress,"event=cmslv2_plan","boundary_blocks",0
			));
		}else if(progress_expect == CMSLV_PROGRESS_BOUNDARY_LOCAL_CMSLV2){
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,"cmslv2=1"));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,"driver=cmslv2"));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_boundary_local phase=boundary_at_zero block=0 action=scoped"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_boundary_local_traverse phase=boundary_at_zero block=0 action=ran"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_boundary_local_commit phase=boundary_at_zero block=0 action=accepted"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_boundary_local_commit phase=boundary_at_zero block=0 action=reverted reason=not_local_complete"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_boundary_local_validate phase=boundary_at_zero block=0 action=defer"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"reason=residual_not_satisfied"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_block_solved phase=boundary_at_zero block=0 solved=0"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_scheduler phase=boundary_at_zero structural_block=0 action=boundary_local_defer next_structural=0 reason=not_local_complete"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_qrslv_handoff phase=boundary_at_zero mode=suppressed structural_start=-1 structural_end=-1 next_structural=0 installed_blocks=0 installed_rows=0 installed_cols=0 lrslv_runs=0 local_qrslv_runs=0 boundary_local_complete=0"
			));
			CU_ASSERT(cmslv_progress_event_field_equals(
				progress,"event=cmslv2_boundary_local_traverse","at_boundary",1
			));
			CU_ASSERT(cmslv_progress_event_field_equals(
				progress,"event=cmslv2_boundary_local_traverse","optimize_ok",1
			));
			CU_ASSERT(cmslv_progress_event_field_equals(
				progress,"event=cmslv2_plan","boundary_blocks",1
			));
			CU_ASSERT(cmslv_progress_event_field_equals(
				progress,"event=cmslv2_plan","lrslv_blocks",2
			));
		}else if(progress_expect == CMSLV_PROGRESS_BOUNDARY_LOCAL_COMPLETE_CMSLV2){
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,"cmslv2=1"));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,"driver=cmslv2"));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_boundary_local phase=boundary_at_zero block=0 action=scoped"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_boundary_local_traverse phase=boundary_at_zero block=0 action=ran"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_boundary_local_commit phase=boundary_at_zero block=0 action=accepted"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_boundary_local_validate phase=boundary_at_zero block=0 action=local_complete"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_block_solved phase=boundary_at_zero block=0 solved=1"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_transition phase=boundary_at_zero solver=boundary from_structural=0 next_structural=1 action=reanalyzed"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_scheduler phase=boundary_at_zero structural_block=0 action=boundary_local_advance next_structural=1 reason=accepted local_complete=1"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_lrslv_handoff phase=boundary_at_zero structural_start=1 structural_end=1 installed_blocks=1 installed_rows=1 installed_cols=1 converged=1 ok=1"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_qrslv_local phase=boundary_at_zero structural_start=2 structural_end=2 installed_blocks=1 installed_rows=1 installed_cols=1 converged=1 ok=1"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_qrslv_handoff phase=boundary_at_zero mode=suppressed structural_start=-1 structural_end=-1 next_structural=3 installed_blocks=0 installed_rows=0 installed_cols=0 lrslv_runs=1 local_qrslv_runs=1 boundary_local_complete=1"
			));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,
				"event=cmslv2_scheduler phase=boundary_at_zero action=local_complete next_structural=3"
			));
			CU_ASSERT(cmslv_progress_event_field_equals(
				progress,"event=cmslv2_boundary_local_traverse","at_boundary",1
			));
			CU_ASSERT(cmslv_progress_event_field_equals(
				progress,"event=cmslv2_boundary_local_traverse","optimize_ok",1
			));
		}
		if(expect_boundary_progress){
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,"phase=reconfigure"));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,"event=boundary_crossed"));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,"event=boundary_return_done"));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,"event=boundary_opt_start"));
			CU_ASSERT_PTR_NOT_NULL(strstr(progress,optsolver));
		}
	}

	CONSOLE_DEBUG("Destroying system...");
	if(sys)system_destroy(sys);
	system_free_reused_mem();

	if(progress_expect != CMSLV_PROGRESS_BOUNDARY_LOCAL_CMSLV2
			&& progress_expect != CMSLV_PROGRESS_BOUNDARY_LOCAL_COMPLETE_CMSLV2
			&& progress_expect != CMSLV_PROGRESS_FLUIDBED_SWITCH_CMSLV2){
		/* run 'self_test' method */
		CONSOLE_DEBUG("Running self-tests");
		name = CreateIdName(AddSymbol("self_test"));
		pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
		CU_ASSERT(pe==Proc_all_ok);
	}

	/* destroy all that stuff */
	CONSOLE_DEBUG("Destroying instance tree");
	CU_ASSERT(siminst != NULL);

	solver_destroy_engines();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_cmslv(const char *filenamestem, const char *optsolver,
			int expect_boundary_progress
	){
	test_cmslv_mode(filenamestem,optsolver,
		expect_boundary_progress ? CMSLV_PROGRESS_BOUNDARY : CMSLV_PROGRESS_NONE
	);
}

static void test_qrslv_fallback_real_chain(void){
	struct module_t *m;
	struct Instance *siminst = NULL;
	struct Instance *root;
	struct Instance *x_inst;
	struct Instance *y_inst;
	struct Name *name;
	enum Proc_enum pe;
	slv_system_t sys = NULL;
	int status;
	int cmslv_index;
	slv_status_t solver_status;

	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(
		ASC_ENV_SOLVERS "=solvers/qrslv" OSPATH_DIV
		"solvers/lrslv" OSPATH_DIV "solvers/cmslv"
	));

	CU_ASSERT_FATAL(0 == package_load("qrslv",NULL));
	CU_ASSERT_FATAL(0 == package_load("lrslv",NULL));
	CU_ASSERT_FATAL(0 == package_load("cmslv",NULL));
	cmslv_index = slv_lookup_client("CMSlv");
	CU_ASSERT_FATAL(cmslv_index != -1);

	m = Asc_OpenModule("test/decomp/block_cases.a4c",&status);
	CU_ASSERT_PTR_NOT_NULL_FATAL(m);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("real_chain")) != NULL);

	siminst = SimsCreateInstance(
		AddSymbol("real_chain"), AddSymbol("sim1"), e_normal, NULL
	);
	CU_ASSERT_FATAL(siminst != NULL);
	root = GetSimulationRoot(siminst);
	CU_ASSERT_FATAL(root != NULL);

	name = CreateIdName(AddSymbol("on_load"));
	pe = Initialize(root,name,"sim1",ASCERR,WP_STOPONERR,NULL,NULL);
	CU_ASSERT_FATAL(pe == Proc_all_ok);

	sys = system_build(root);
	CU_ASSERT_FATAL(sys != NULL);
	CU_ASSERT_FATAL(slv_select_solver(sys,cmslv_index));
	CU_ASSERT_FATAL(0 == cmslv_set_char_param(sys,"convopt","RELNOM_SCALE"));

	CU_ASSERT_FATAL(0 == slv_presolve(sys));
	slv_get_status(sys,&solver_status);
	CU_ASSERT_FATAL(solver_status.ready_to_solve);
	CU_ASSERT_FATAL(0 == slv_solve(sys));
	slv_get_status(sys,&solver_status);
	CU_ASSERT(solver_status.ok);
	CU_ASSERT(solver_status.converged);
	CU_ASSERT_EQUAL(solver_status.block.number_of,2);

	x_inst = ChildByChar(root, AddSymbol("x"));
	y_inst = ChildByChar(root, AddSymbol("y"));
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(x_inst), 1.0, 1e-8);
	CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(y_inst), 2.0, 1e-8);

	if(sys)system_destroy(sys);
	system_free_reused_mem();
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
	T(reinitignore_ipopt)\
	T(linmassbal_cmslv2)\
	T(pipeline_cmslv2)\
	T(heatex_cmslv2)\
	T(reinitignore_cmslv2)\
	T(cmslv2_scheduler)\
	T(cmslv2_boundary_local)\
	T(cmslv2_boundary_local_complete)\
	T(cmslv2_fluidbed_switch_crash)\
	T(qrslv_fallback_real_chain)

static void test_linmassbal(void){ test_cmslv("linmassbal","CONOPT",1); }
static void test_pipeline(void){ test_cmslv("pipeline","CONOPT",1); }
static void test_heatex(void){ test_cmslv("heatex","CONOPT",1); }
static void test_reinitignore(void){ test_cmslv("reinitignore","CONOPT",0); }
static void test_linmassbal_ipopt(void){ test_cmslv("linmassbal","IPOPT",1); }
static void test_pipeline_ipopt(void){ test_cmslv("pipeline","IPOPT",1); }
static void test_heatex_ipopt(void){ test_cmslv("heatex","IPOPT",1); }
static void test_reinitignore_ipopt(void){ test_cmslv("reinitignore","IPOPT",0); }
static void test_linmassbal_cmslv2(void){
	test_cmslv_mode("linmassbal","CONOPT",CMSLV_PROGRESS_LINMASSBAL_CMSLV2);
}
static void test_pipeline_cmslv2(void){
	test_cmslv_mode("pipeline","CONOPT",CMSLV_PROGRESS_GENERAL_CMSLV2);
}
static void test_heatex_cmslv2(void){
	test_cmslv_mode("heatex","CONOPT",CMSLV_PROGRESS_GENERAL_CMSLV2);
}
static void test_reinitignore_cmslv2(void){
	test_cmslv_mode("reinitignore","CONOPT",CMSLV_PROGRESS_GENERAL_CMSLV2);
}
static void test_cmslv2_scheduler(void){
	test_cmslv_mode("cmslv2_scheduler","CONOPT",CMSLV_PROGRESS_SCHEDULER_CMSLV2);
}
static void test_cmslv2_boundary_local(void){
	test_cmslv_mode(
		"cmslv2_boundary_local","CONOPT",CMSLV_PROGRESS_BOUNDARY_LOCAL_CMSLV2
	);
}
static void test_cmslv2_boundary_local_complete(void){
	test_cmslv_mode(
		"cmslv2_boundary_local_complete","CONOPT",
		CMSLV_PROGRESS_BOUNDARY_LOCAL_COMPLETE_CMSLV2
	);
}
static void test_cmslv2_fluidbed_switch_crash(void){
	test_cmslv_mode(
		"cmslv2_fluidbed_switch_crash","IPOPT",
		CMSLV_PROGRESS_FLUIDBED_SWITCH_CMSLV2
	);
}

REGISTER_TESTS_SIMPLE(solver_cmslv, TESTS);
