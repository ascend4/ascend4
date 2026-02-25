/*
 * ASCEND modelling environment
 * DATASET-focused compiler tests.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdarg.h>

#ifdef __WIN32__
# include <process.h>
# include <io.h>
# define DATASET_CLOSEFD _close
# define DATASET_PATHLIST_SEP ';'
#else
# include <unistd.h>
# define DATASET_CLOSEFD close
# define DATASET_PATHLIST_SEP ':'
#endif

#include <ascend/general/env.h>
#include <ascend/general/ospath.h>
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
#include <ascend/compiler/instance_name.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/name.h>

#include <test/common.h>

typedef struct{
	int error_count;
	char all_error_msgs[4096];
} dataset_parse_error_capture_t;

static dataset_parse_error_capture_t g_dataset_parse_error_capture;

static void dataset_parse_error_capture_reset(void){
	memset(&g_dataset_parse_error_capture,0,sizeof(g_dataset_parse_error_capture));
}

static int dataset_parse_error_capture_cb(ERROR_REPORTER_CALLBACK_ARGS){
	char msg[512];
	int wrote_default;
	va_list args_copy;
	size_t used;

	va_copy(args_copy,args);
	vsnprintf(msg,sizeof(msg),fmt,args_copy);
	va_end(args_copy);

	if(sev & ASC_ERR_ERR){
		g_dataset_parse_error_capture.error_count++;
		used = strlen(g_dataset_parse_error_capture.all_error_msgs);
		if(used + 2 < sizeof(g_dataset_parse_error_capture.all_error_msgs)){
			if(used > 0){
				snprintf(
					g_dataset_parse_error_capture.all_error_msgs + used
					,sizeof(g_dataset_parse_error_capture.all_error_msgs) - used
					,"\n"
				);
				used = strlen(g_dataset_parse_error_capture.all_error_msgs);
			}
			snprintf(
				g_dataset_parse_error_capture.all_error_msgs + used
				,sizeof(g_dataset_parse_error_capture.all_error_msgs) - used
				,"%s",msg
			);
		}
	}

	va_copy(args_copy,args);
	wrote_default = error_reporter_default_callback(sev,filename,line,funcname,fmt,args_copy);
	va_end(args_copy);
	return wrote_default;
}

static int dataset_set_library(const char *librarypath){
	char env[2 * PATH_MAX];
	if(snprintf(env,sizeof(env),ASC_ENV_LIBRARY "=%s",librarypath) >= (int)sizeof(env)){
		return 1;
	}
	return Asc_PutEnv(env);
}

static struct Instance *dataset_open_parse_instantiate(const char *librarypath, const char *modulefile, const char *typename){
	int status;
	struct Instance *sim;

	Asc_CompilerInit(1);
	CU_ASSERT_FATAL(0 == dataset_set_library(librarypath));

	Asc_OpenModule(modulefile,&status);
	CU_ASSERT_FATAL(status == 0);

	error_reporter_tree_start();
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(0 == error_reporter_tree_has_error());
	error_reporter_tree_end();

	CU_ASSERT_FATAL(FindType(AddSymbol(typename))!=NULL);

	sim = SimsCreateInstance(AddSymbol(typename), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);
	return sim;
}

static void dataset_run_methods(struct Instance *sim){
	struct Name *name;
	enum Proc_enum pe;

	name = CreateIdName(AddSymbol("on_load"));
	pe = Initialize(GetSimulationRoot(sim),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	name = CreateIdName(AddSymbol("self_test"));
	pe = Initialize(GetSimulationRoot(sim),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);
}

static void dataset_load_run_methods(const char *librarypath, const char *modulefile, const char *typename){
	struct Instance *sim = dataset_open_parse_instantiate(librarypath,modulefile,typename);
	dataset_run_methods(sim);
	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static long fetch_int_table_cell_2d(struct Instance *root, const char *arrname, long i, long j){
	struct InstanceName rec;
	struct Instance *arr;
	struct Instance *row;
	struct Instance *inst;
	unsigned long pos;
	long value;

	arr = ChildByChar(root,AddSymbol(arrname));
	CU_ASSERT_FATAL(arr != NULL);

	SetInstanceNameType(rec,IntArrayIndex);
	SetInstanceNameIntIndex(rec,i);
	pos = ChildSearch(arr,&rec);
	CU_ASSERT_FATAL(pos != 0);
	row = InstanceChild(arr,pos);
	CU_ASSERT_FATAL(row != NULL);

	SetInstanceNameIntIndex(rec,j);
	pos = ChildSearch(row,&rec);
	CU_ASSERT_FATAL(pos != 0);
	inst = InstanceChild(row,pos);
	CU_ASSERT_FATAL(inst != NULL);
	CU_ASSERT_FATAL(InstanceKind(inst)==INTEGER_CONSTANT_INST);
	CU_ASSERT_FATAL(AtomAssigned(inst));
	value = GetIntegerAtomValue(inst);
	return value;
}

static long fetch_int_table_cell_2d_ss(struct Instance *root, const char *arrname, const char *i, const char *j){
	struct InstanceName rec;
	struct Instance *arr;
	struct Instance *row;
	struct Instance *inst;
	unsigned long pos;
	long value;

	arr = ChildByChar(root,AddSymbol(arrname));
	CU_ASSERT_FATAL(arr != NULL);

	SetInstanceNameType(rec,StrArrayIndex);
	SetInstanceNameStrIndex(rec,AddSymbol(i));
	pos = ChildSearch(arr,&rec);
	CU_ASSERT_FATAL(pos != 0);
	row = InstanceChild(arr,pos);
	CU_ASSERT_FATAL(row != NULL);

	SetInstanceNameStrIndex(rec,AddSymbol(j));
	pos = ChildSearch(row,&rec);
	CU_ASSERT_FATAL(pos != 0);
	inst = InstanceChild(row,pos);
	CU_ASSERT_FATAL(inst != NULL);
	CU_ASSERT_FATAL(InstanceKind(inst)==INTEGER_CONSTANT_INST);
	CU_ASSERT_FATAL(AtomAssigned(inst));
	value = GetIntegerAtomValue(inst);
	return value;
}

static double fetch_real_table_cell_1d(struct Instance *root, const char *arrname, long i){
	struct InstanceName rec;
	struct Instance *arr;
	struct Instance *inst;
	unsigned long pos;
	double value;

	arr = ChildByChar(root,AddSymbol(arrname));
	CU_ASSERT_FATAL(arr != NULL);

	SetInstanceNameType(rec,IntArrayIndex);
	SetInstanceNameIntIndex(rec,i);
	pos = ChildSearch(arr,&rec);
	CU_ASSERT_FATAL(pos != 0);
	inst = InstanceChild(arr,pos);
	CU_ASSERT_FATAL(inst != NULL);
	CU_ASSERT_FATAL(InstanceKind(inst)==REAL_CONSTANT_INST);
	CU_ASSERT_FATAL(AtomAssigned(inst));
	value = RealAtomValue(inst);
	return value;
}

static void parse_module_expect_error(const char *modulefile, const char *typename, const char *msg_substr, int expect_type_rejected){
	int status;
	int has_error;

	Asc_CompilerInit(1);
	CU_ASSERT_FATAL(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));

	dataset_parse_error_capture_reset();
	error_reporter_set_callback(&dataset_parse_error_capture_cb);

	Asc_OpenModule(modulefile,&status);
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();

	CU_ASSERT(has_error == 1);
	CU_ASSERT(g_dataset_parse_error_capture.error_count > 0);
	if(msg_substr){
		CU_ASSERT(strstr(g_dataset_parse_error_capture.all_error_msgs,msg_substr) != NULL);
	}
	if(expect_type_rejected && typename){
		CU_ASSERT(FindType(AddSymbol(typename))==NULL);
	}

	error_reporter_set_callback(NULL);
	Asc_CompilerDestroy();
}

static void instantiate_module_expect_error(const char *modulefile, const char *typename, const char *msg_substr){
	int status;
	struct Instance *sim;

	Asc_CompilerInit(1);
	CU_ASSERT_FATAL(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));

	error_reporter_set_callback(&dataset_parse_error_capture_cb);

	Asc_OpenModule(modulefile,&status);
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT(0 == error_reporter_tree_has_error());
	error_reporter_tree_end();

	CU_ASSERT(FindType(AddSymbol(typename))!=NULL);

	dataset_parse_error_capture_reset();
	sim = SimsCreateInstance(AddSymbol(typename), AddSymbol("sim1"), e_normal, NULL);

	CU_ASSERT(g_dataset_parse_error_capture.error_count > 0);
	if(msg_substr){
		CU_ASSERT(strstr(g_dataset_parse_error_capture.all_error_msgs,msg_substr) != NULL);
	}

	if(sim != NULL){
		sim_destroy(sim);
	}
	error_reporter_set_callback(NULL);
	Asc_CompilerDestroy();
}

static int dataset_command_available(const char *command){
	char cmd[256];
	if(snprintf(cmd,sizeof(cmd),"%s --version >/dev/null 2>&1",command) >= (int)sizeof(cmd)){
		return 0;
	}
	return system(cmd) == 0;
}

static int dataset_build_temp_file_path(const char *filename, char *path, size_t path_len){
	char probe_path[PATH_MAX];
	struct FilePath *probe_fp = NULL;
	struct FilePath *dir_fp = NULL;
	struct FilePath *name_fp = NULL;
	struct FilePath *full_fp = NULL;
	char *full_str = NULL;
	int fd, ok = 0;

	if (filename == NULL || path == NULL || path_len == 0) {
		return 0;
	}
	fd = ospath_mkstemp(probe_path,sizeof(probe_path),"asc_dataset_");
	if (fd < 0) {
		return 0;
	}
	DATASET_CLOSEFD(fd);
	remove(probe_path);

	probe_fp = ospath_new(probe_path);
	if (probe_fp == NULL) goto cleanup;
	dir_fp = ospath_getdir(probe_fp);
	if (dir_fp == NULL) goto cleanup;
	name_fp = ospath_new_noclean(filename);
	if (name_fp == NULL) goto cleanup;
	full_fp = ospath_concat(dir_fp,name_fp);
	if (full_fp == NULL) goto cleanup;
	full_str = ospath_str(full_fp);
	if (full_str == NULL) goto cleanup;
	if (snprintf(path,path_len,"%s",full_str) >= (int)path_len) goto cleanup;
	ok = 1;

cleanup:
	if (full_str != NULL) ospath_free_str(full_str);
	ospath_free(full_fp);
	ospath_free(name_fp);
	ospath_free(dir_fp);
	ospath_free(probe_fp);
	return ok;
}

static int dataset_make_library_path_for_file(
	const char *file_path,
	char *librarypath,
	size_t librarypath_len
){
	struct FilePath *fp = NULL;
	struct FilePath *dir_fp = NULL;
	char *dir_str = NULL;
	int ok = 0;

	if (file_path == NULL || librarypath == NULL || librarypath_len == 0) {
		return 0;
	}
	fp = ospath_new(file_path);
	if (fp == NULL) goto cleanup;
	dir_fp = ospath_getdir(fp);
	if (dir_fp == NULL) goto cleanup;
	dir_str = ospath_str(dir_fp);
	if (dir_str == NULL) goto cleanup;
	if (snprintf(librarypath,librarypath_len,"%s%cmodels",dir_str,DATASET_PATHLIST_SEP) >= (int)librarypath_len) {
		goto cleanup;
	}
	ok = 1;

cleanup:
	if (dir_str != NULL) ospath_free_str(dir_str);
	ospath_free(dir_fp);
	ospath_free(fp);
	return ok;
}

static int dataset_prepare_melbourne_uncompressed(char *csv_path, size_t csv_path_len){
	char cmd[4 * PATH_MAX];

	if(!dataset_build_temp_file_path("086282TMY_60min.csv",csv_path,csv_path_len)){
		return 0;
	}
	if(snprintf(cmd,sizeof(cmd)
		,"xz -dc \"models/johnpye/dataset/086282TMY_60min.csv.xz\" > \"%s\""
		,csv_path
	) >= (int)sizeof(cmd)){
		return 0;
	}
	return system(cmd) == 0;
}

static int dataset_prepare_melbourne_gz(const char *csv_path, char *gz_path, size_t gz_path_len){
	char cmd[4 * PATH_MAX];
	if(snprintf(gz_path,gz_path_len,"%s.gz",csv_path) >= (int)gz_path_len){
		return 0;
	}
	if(snprintf(cmd,sizeof(cmd),"gzip -c \"%s\" > \"%s\"",csv_path,gz_path) >= (int)sizeof(cmd)){
		return 0;
	}
	return system(cmd) == 0;
}

static void test_instantiate_dataset_basic(void){
	struct Instance *sim;
	struct Instance *root;

	sim = dataset_open_parse_instantiate("models","test/compiler/dataset_basic.a4c","dataset_basic");
	root = GetSimulationRoot(sim);
	CU_ASSERT_FATAL(root!=NULL);

	CU_ASSERT(fetch_int_table_cell_2d(root,"cost",1,1) == 11);
	CU_ASSERT(fetch_int_table_cell_2d(root,"cost",1,2) == 12);
	CU_ASSERT(fetch_int_table_cell_2d(root,"cost",1,3) == 13);
	CU_ASSERT(fetch_int_table_cell_2d(root,"cost",2,1) == 21);
	CU_ASSERT(fetch_int_table_cell_2d(root,"cost",2,2) == 22);
	CU_ASSERT(fetch_int_table_cell_2d(root,"cost",2,3) == 23);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_instantiate_dataset_string(void){
	struct Instance *sim;
	struct Instance *root;

	sim = dataset_open_parse_instantiate("models","test/compiler/dataset_string.a4c","dataset_string");
	root = GetSimulationRoot(sim);
	CU_ASSERT_FATAL(root!=NULL);

	CU_ASSERT(fetch_int_table_cell_2d_ss(root,"cost","north","x") == 11);
	CU_ASSERT(fetch_int_table_cell_2d_ss(root,"cost","north","y") == 12);
	CU_ASSERT(fetch_int_table_cell_2d_ss(root,"cost","south","x") == 21);
	CU_ASSERT(fetch_int_table_cell_2d_ss(root,"cost","south","y") == 22);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_instantiate_dataset_real(void){
	struct Instance *sim;
	struct Instance *root;
	double value;

	sim = dataset_open_parse_instantiate("models","test/compiler/dataset_real.a4c","dataset_real");
	root = GetSimulationRoot(sim);
	CU_ASSERT_FATAL(root!=NULL);

	value = fetch_real_table_cell_1d(root,"load",1);
	CU_ASSERT(fabs(value - 1.5) < 1e-12);
	value = fetch_real_table_cell_1d(root,"load",2);
	CU_ASSERT(fabs(value - 2.25) < 1e-12);
	value = fetch_real_table_cell_1d(root,"load",3);
	CU_ASSERT(fabs(value - 3.75) < 1e-12);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_instantiate_dataset_isa_from(void){
	struct Instance *sim;
	struct Instance *root;
	double value;

	sim = dataset_open_parse_instantiate("models","test/compiler/dataset_isa_from.a4c","dataset_isa_from");
	root = GetSimulationRoot(sim);
	CU_ASSERT_FATAL(root!=NULL);

	value = fetch_real_table_cell_1d(root,"load",1);
	CU_ASSERT(fabs(value - 1.5) < 1e-12);
	value = fetch_real_table_cell_1d(root,"load",2);
	CU_ASSERT(fabs(value - 2.25) < 1e-12);
	value = fetch_real_table_cell_1d(root,"load",3);
	CU_ASSERT(fabs(value - 3.75) < 1e-12);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_instantiate_dataset_implicit_row_index(void){
	struct Instance *sim;
	struct Instance *root;
	double value;

	sim = dataset_open_parse_instantiate("models","test/compiler/dataset_implicit_row_index.a4c","dataset_implicit_row_index");
	root = GetSimulationRoot(sim);
	CU_ASSERT_FATAL(root!=NULL);

	value = fetch_real_table_cell_1d(root,"load",1);
	CU_ASSERT(fabs(value - 1.5) < 1e-12);
	value = fetch_real_table_cell_1d(root,"load",2);
	CU_ASSERT(fabs(value - 2.25) < 1e-12);
	value = fetch_real_table_cell_1d(root,"load",3);
	CU_ASSERT(fabs(value - 3.75) < 1e-12);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_instantiate_dataset_implicit_row_index_autoset(void){
	struct Instance *sim;
	struct Instance *root;
	double value;

	sim = dataset_open_parse_instantiate("models","test/compiler/dataset_implicit_row_index_autoset.a4c","dataset_implicit_row_index_autoset");
	root = GetSimulationRoot(sim);
	CU_ASSERT_FATAL(root!=NULL);

	value = fetch_real_table_cell_1d(root,"load",1);
	CU_ASSERT(fabs(value - 1.5) < 1e-12);
	value = fetch_real_table_cell_1d(root,"load",2);
	CU_ASSERT(fabs(value - 2.25) < 1e-12);
	value = fetch_real_table_cell_1d(root,"load",3);
	CU_ASSERT(fabs(value - 3.75) < 1e-12);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_parse_tables_v05_fail_dataset_missing_column(void){
	parse_module_expect_error(
		"test/compiler/tables_v05_fail_dataset_missing_column.a4c"
		, "tables_v05_fail_dataset_missing_column"
		, "syntax error"
		, 1
	);
}

static void test_parse_tables_v05_fail_dataset_missing_index(void){
	parse_module_expect_error(
		"test/compiler/tables_v05_fail_dataset_missing_index.a4c"
		, "tables_v05_fail_dataset_missing_index"
		, NULL
		, 1
	);
}

static void test_parse_tables_v05_fail_dataset_multi_units(void){
	parse_module_expect_error(
		"test/compiler/tables_v05_fail_dataset_multi_units.a4c"
		, "tables_v05_fail_dataset_multi_units"
		, NULL
		, 1
	);
}

static void test_instantiate_dataset_units_conflict(void){
	instantiate_module_expect_error(
		"test/compiler/dataset_units_conflict.a4c"
		, "dataset_units_conflict"
		, "units conflict"
	);
}

static void test_instantiate_dataset_units_bracket_conflict(void){
	instantiate_module_expect_error(
		"test/compiler/dataset_units_bracket_conflict.a4c"
		, "dataset_units_bracket_conflict"
		, "units conflict"
	);
}

static void test_instantiate_dataset_units_row_conflict(void){
	instantiate_module_expect_error(
		"test/compiler/dataset_units_row_conflict.a4c"
		, "dataset_units_row_conflict"
		, "units conflict"
	);
}

static void test_instantiate_dataset_implicit_row_index_multi_fail(void){
	instantiate_module_expect_error(
		"test/compiler/dataset_implicit_row_index_multi_fail.a4c"
		, "dataset_implicit_row_index_multi_fail"
		, "implicit row-index mode supports only one undeclared index set"
	);
}

static void test_melbourne_dni_xz(void){
#ifdef ASC_WITH_LZMA
	dataset_load_run_methods("models","johnpye/dataset/melbourne_dni.a4c","melbourne_dni");
#else
	CONSOLE_DEBUG("Skipping melbourne_dni_xz: requires compilation with liblzma");
	return;
#endif
}

static void test_melbourne_dni_uncompressed(void){
	char librarypath[3 * PATH_MAX];
	char csv_path[PATH_MAX];

	if(!dataset_command_available("xz")){
		CONSOLE_DEBUG("Skipping melbourne_dni_uncompressed: 'xz' command not available");
		return;
	}

	CU_ASSERT_FATAL(dataset_prepare_melbourne_uncompressed(csv_path,sizeof(csv_path)));
	CU_ASSERT_FATAL(dataset_make_library_path_for_file(csv_path,librarypath,sizeof(librarypath)));

	dataset_load_run_methods(
		librarypath,
		"johnpye/dataset/melbourne_dni.a4c",
		"melbourne_dni_uncompressed"
	);

	remove(csv_path);
}

static void test_melbourne_dni_gz(void){
#ifdef ASC_WITH_ZLIB
	char librarypath[3 * PATH_MAX];
	char csv_path[PATH_MAX];
	char gz_path[PATH_MAX];

	if(!dataset_command_available("xz") || !dataset_command_available("gzip")){
		CONSOLE_DEBUG("Skipping melbourne_dni_gz: 'xz' and/or 'gzip' command not available");
		return;
	}

	CU_ASSERT_FATAL(dataset_prepare_melbourne_uncompressed(csv_path,sizeof(csv_path)));
	CU_ASSERT_FATAL(dataset_make_library_path_for_file(csv_path,librarypath,sizeof(librarypath)));
	CU_ASSERT_FATAL(dataset_prepare_melbourne_gz(csv_path,gz_path,sizeof(gz_path)));

	dataset_load_run_methods(librarypath,"johnpye/dataset/melbourne_dni.a4c","melbourne_dni_gz");

	remove(gz_path);
	remove(csv_path);
#else
	CONSOLE_DEBUG("Skipping melbourne_dni_gz: requires compilation with zlib");
	return;
#endif
}

#define TESTS(T) \
	T(instantiate_dataset_basic) \
	T(instantiate_dataset_string) \
	T(instantiate_dataset_real) \
	T(instantiate_dataset_isa_from) \
	T(instantiate_dataset_implicit_row_index) \
	T(instantiate_dataset_implicit_row_index_autoset) \
	T(parse_tables_v05_fail_dataset_missing_column) \
	T(parse_tables_v05_fail_dataset_missing_index) \
	T(parse_tables_v05_fail_dataset_multi_units) \
	T(instantiate_dataset_units_conflict) \
	T(instantiate_dataset_units_bracket_conflict) \
	T(instantiate_dataset_units_row_conflict) \
	T(instantiate_dataset_implicit_row_index_multi_fail) \
	T(melbourne_dni_xz) \
	T(melbourne_dni_uncompressed) \
	T(melbourne_dni_gz)

REGISTER_TESTS_SIMPLE(compiler_dataset, TESTS)
