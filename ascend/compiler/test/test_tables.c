/*
 * ASCEND modelling environment
 * TABLE-focused compiler tests.
 */

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdarg.h>

#include <ascend/general/env.h>
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

#include <test/common.h>

typedef struct{
	int error_count;
	int first_error_line;
	char all_error_msgs[4096];
} table_parse_error_capture_t;

static table_parse_error_capture_t g_table_parse_error_capture;

static void table_parse_error_capture_reset(void){
	memset(&g_table_parse_error_capture,0,sizeof(g_table_parse_error_capture));
}

static int table_parse_error_capture_cb(ERROR_REPORTER_CALLBACK_ARGS){
	char msg[512];
	int wrote_default;
	va_list args_copy;
	size_t used;

	va_copy(args_copy,args);
	vsnprintf(msg,sizeof(msg),fmt,args_copy);
	va_end(args_copy);

	if(sev & ASC_ERR_ERR){
		g_table_parse_error_capture.error_count++;
		used = strlen(g_table_parse_error_capture.all_error_msgs);
		if(used + 2 < sizeof(g_table_parse_error_capture.all_error_msgs)){
			if(used > 0){
				snprintf(
					g_table_parse_error_capture.all_error_msgs + used
					,sizeof(g_table_parse_error_capture.all_error_msgs) - used
					,"\n"
				);
				used = strlen(g_table_parse_error_capture.all_error_msgs);
			}
			snprintf(
				g_table_parse_error_capture.all_error_msgs + used
				,sizeof(g_table_parse_error_capture.all_error_msgs) - used
				,"%s",msg
			);
		}
		if(g_table_parse_error_capture.first_error_line == 0){
			g_table_parse_error_capture.first_error_line = line;
		}
	}

	va_copy(args_copy,args);
	wrote_default = error_reporter_default_callback(sev,filename,line,funcname,fmt,args_copy);
	va_end(args_copy);
	return wrote_default;
}

static void parse_module_expect_error(const char *modulefile, const char *typename, const char *msg_substr, int expect_type_rejected, int require_column_info){
	int status;
	int has_error;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	table_parse_error_capture_reset();
	error_reporter_set_callback(&table_parse_error_capture_cb);

	Asc_OpenModule(modulefile,&status);
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();

	CU_ASSERT(has_error == 1);
	CU_ASSERT(g_table_parse_error_capture.error_count > 0);
	CU_ASSERT(g_table_parse_error_capture.first_error_line > 0);
	if(require_column_info){
		CU_ASSERT(strstr(g_table_parse_error_capture.all_error_msgs,"column") != NULL);
	}
	if(msg_substr){
		CU_ASSERT(strstr(g_table_parse_error_capture.all_error_msgs,msg_substr) != NULL);
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
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	error_reporter_set_callback(&table_parse_error_capture_cb);

	Asc_OpenModule(modulefile,&status);
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT(0 == error_reporter_tree_has_error());
	error_reporter_tree_end();

	CU_ASSERT(FindType(AddSymbol(typename))!=NULL);

	table_parse_error_capture_reset();
	sim = SimsCreateInstance(AddSymbol(typename), AddSymbol("sim1"), e_normal, NULL);

	CU_ASSERT(g_table_parse_error_capture.error_count > 0);
	if(msg_substr){
		CU_ASSERT(strstr(g_table_parse_error_capture.all_error_msgs,msg_substr) != NULL);
	}

	if(sim != NULL){
		sim_destroy(sim);
	}
	error_reporter_set_callback(NULL);
	Asc_CompilerDestroy();
}

static struct Instance *open_parse_instantiate(const char *modulefile, const char *typename){
	int status;
	struct Instance *sim;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

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

static long fetch_int_table_cell_2d(struct Instance *root, const char *arrname, long i, long j){
	struct InstanceName rec;
	struct Instance *arr;
	struct Instance *row;
	struct Instance *inst;
	unsigned long pos;

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
	return GetIntegerAtomValue(inst);
}

static double fetch_real_table_cell_2d(struct Instance *root, const char *arrname, long i, long j){
	struct InstanceName rec;
	struct Instance *arr;
	struct Instance *row;
	struct Instance *inst;
	unsigned long pos;

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
	CU_ASSERT_FATAL(InstanceKind(inst)==REAL_CONSTANT_INST);
	CU_ASSERT_FATAL(AtomAssigned(inst));
	return RealAtomValue(inst);
}

static long fetch_int_table_cell_2d_is(struct Instance *root, const char *arrname, long i, const char *j){
	struct InstanceName rec;
	struct Instance *arr;
	struct Instance *row;
	struct Instance *inst;
	unsigned long pos;

	arr = ChildByChar(root,AddSymbol(arrname));
	CU_ASSERT_FATAL(arr != NULL);

	SetInstanceNameType(rec,IntArrayIndex);
	SetInstanceNameIntIndex(rec,i);
	pos = ChildSearch(arr,&rec);
	CU_ASSERT_FATAL(pos != 0);
	row = InstanceChild(arr,pos);
	CU_ASSERT_FATAL(row != NULL);

	SetInstanceNameType(rec,StrArrayIndex);
	SetInstanceNameStrIndex(rec,AddSymbol(j));
	pos = ChildSearch(row,&rec);
	CU_ASSERT_FATAL(pos != 0);
	inst = InstanceChild(row,pos);
	CU_ASSERT_FATAL(inst != NULL);
	CU_ASSERT_FATAL(InstanceKind(inst)==INTEGER_CONSTANT_INST);
	CU_ASSERT_FATAL(AtomAssigned(inst));
	return GetIntegerAtomValue(inst);
}

static long fetch_int_table_cell_2d_ss(struct Instance *root, const char *arrname, const char *i, const char *j){
	struct InstanceName rec;
	struct Instance *arr;
	struct Instance *row;
	struct Instance *inst;
	unsigned long pos;

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
	return GetIntegerAtomValue(inst);
}

static void test_parse_tables_v05(void){
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	Asc_OpenModule("test/compiler/tables_v05_parse.a4c",&status);
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT(0 == error_reporter_tree_has_error());
	error_reporter_tree_end();

	CU_ASSERT(FindType(AddSymbol("tables_v05_parse"))!=NULL);

	Asc_CompilerDestroy();
}

static void test_instantiate_tables_v05_positional(void){
	struct Instance *sim = open_parse_instantiate("test/compiler/tables_v05_instantiate.a4c","tables_v05_instantiate");
	struct Instance *root = GetSimulationRoot(sim);
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

static void test_instantiate_tables_v05_positional_csv_semicolon(void){
	struct Instance *sim = open_parse_instantiate("test/compiler/tables_v05_instantiate_positional_csv_semicolon.a4c","tables_v05_instantiate_positional_csv_semicolon");
	struct Instance *root = GetSimulationRoot(sim);
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

static void test_instantiate_tables_v05_dense_int_labels(void){
	struct Instance *sim = open_parse_instantiate("test/compiler/tables_v05_instantiate_dense_int.a4c","tables_v05_instantiate_dense_int");
	struct Instance *root = GetSimulationRoot(sim);
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

static void test_instantiate_tables_v05_dense_csv_semicolon(void){
	struct Instance *sim = open_parse_instantiate("test/compiler/tables_v05_instantiate_dense_csv_semicolon.a4c","tables_v05_instantiate_dense_csv_semicolon");
	struct Instance *root = GetSimulationRoot(sim);
	CU_ASSERT_FATAL(root!=NULL);

	CU_ASSERT(fetch_int_table_cell_2d_ss(root,"cost","alan","c3") == 23);
	CU_ASSERT(fetch_int_table_cell_2d_ss(root,"cost","alan","c1") == 21);
	CU_ASSERT(fetch_int_table_cell_2d_ss(root,"cost","alan","c2") == 22);
	CU_ASSERT(fetch_int_table_cell_2d_ss(root,"cost","bernhard","c3") == 13);
	CU_ASSERT(fetch_int_table_cell_2d_ss(root,"cost","bernhard","c1") == 11);
	CU_ASSERT(fetch_int_table_cell_2d_ss(root,"cost","bernhard","c2") == 12);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_instantiate_tables_v05_dense_string_labels(void){
	struct Instance *sim = open_parse_instantiate("test/compiler/tables_v05_instantiate_dense_string.a4c","tables_v05_instantiate_dense_string");
	struct Instance *root = GetSimulationRoot(sim);
	CU_ASSERT_FATAL(root!=NULL);

	CU_ASSERT(fetch_int_table_cell_2d_ss(root,"cost","north","x") == 11);
	CU_ASSERT(fetch_int_table_cell_2d_ss(root,"cost","north","y") == 12);
	CU_ASSERT(fetch_int_table_cell_2d_ss(root,"cost","south","x") == 21);
	CU_ASSERT(fetch_int_table_cell_2d_ss(root,"cost","south","y") == 22);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_instantiate_tables_v05_dense_implicit_sets(void){
	struct Instance *sim = open_parse_instantiate("test/compiler/tables_v05_instantiate_dense_implicit.a4c","tables_v05_instantiate_dense_implicit");
	struct Instance *root = GetSimulationRoot(sim);
	CU_ASSERT_FATAL(root!=NULL);

	CU_ASSERT(fetch_int_table_cell_2d_is(root,"cost",1,"a") == 11);
	CU_ASSERT(fetch_int_table_cell_2d_is(root,"cost",1,"b") == 12);
	CU_ASSERT(fetch_int_table_cell_2d_is(root,"cost",2,"a") == 21);
	CU_ASSERT(fetch_int_table_cell_2d_is(root,"cost",2,"b") == 22);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_instantiate_tables_v05_dense_real(void){
	struct Instance *sim = open_parse_instantiate("test/compiler/tables_v05_instantiate_dense_real.a4c","tables_v05_instantiate_dense_real");
	struct Instance *root = GetSimulationRoot(sim);
	CU_ASSERT_FATAL(root!=NULL);

	CU_ASSERT_DOUBLE_EQUAL(fetch_real_table_cell_2d(root,"cost",1,1), 11.5, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fetch_real_table_cell_2d(root,"cost",1,2), 12.75, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fetch_real_table_cell_2d(root,"cost",1,3), 13.125, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fetch_real_table_cell_2d(root,"cost",2,1), 21.25, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fetch_real_table_cell_2d(root,"cost",2,2), 22.875, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fetch_real_table_cell_2d(root,"cost",2,3), 23.5, 1e-12);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_instantiate_tables_v05_units(void){
	struct Instance *sim = open_parse_instantiate("test/compiler/tables_v05_instantiate_units.a4c","tables_v05_instantiate_units");
	struct Instance *root = GetSimulationRoot(sim);
	CU_ASSERT_FATAL(root!=NULL);

	CU_ASSERT_DOUBLE_EQUAL(fetch_real_table_cell_2d(root,"temp",1,1), 273.15, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fetch_real_table_cell_2d(root,"temp",1,2), 298.15, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fetch_real_table_cell_2d(root,"temp",2,1), 310.5, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fetch_real_table_cell_2d(root,"temp",2,2), 325.0, 1e-12);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_instantiate_unifac_constants_spotchecks(void){
	struct Instance *sim = open_parse_instantiate("components.a4l","UNIFAC_constants");
	struct Instance *root = GetSimulationRoot(sim);
	CU_ASSERT_FATAL(root!=NULL);

	CU_ASSERT_DOUBLE_EQUAL(fetch_real_table_cell_2d(root,"a",1,1), 0.0, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fetch_real_table_cell_2d(root,"a",1,2), 86.020, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fetch_real_table_cell_2d(root,"a",2,1), -35.36, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fetch_real_table_cell_2d(root,"a",3,18), -4.449, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fetch_real_table_cell_2d(root,"a",8,24), 1827.0, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fetch_real_table_cell_2d(root,"a",10,47), 6810.0, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fetch_real_table_cell_2d(root,"a",38,40), 0.0, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fetch_real_table_cell_2d(root,"a",40,38), 185.6, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fetch_real_table_cell_2d(root,"a",42,43), -2166.0, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fetch_real_table_cell_2d(root,"a",47,47), 0.0, 1e-12);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_parse_tables_v05_fail_table_header(void){
	parse_module_expect_error("test/compiler/tables_v05_fail_table_header.a4c","tables_v05_fail_table_header","syntax error",1,1);
}

static void test_parse_tables_v05_fail_table_badchar(void){
	parse_module_expect_error("test/compiler/tables_v05_fail_table_badchar.a4c","tables_v05_fail_table_badchar","Unexpected character",0,1);
}

static void test_parse_tables_v05_fail_table_bad_delimiter(void){
	parse_module_expect_error("test/compiler/tables_v05_fail_table_bad_delimiter.a4c","tables_v05_fail_table_bad_delimiter","syntax error",0,1);
}

static void test_instantiate_tables_v05_fail_positional_short_row(void){
	instantiate_module_expect_error("test/compiler/tables_v05_fail_positional_short_row.a4c","tables_v05_fail_positional_short_row",NULL);
}

static void test_instantiate_tables_v05_fail_positional_too_many_cols(void){
	instantiate_module_expect_error("test/compiler/tables_v05_fail_positional_too_many_cols.a4c","tables_v05_fail_positional_too_many_cols",NULL);
}

static void test_instantiate_tables_v05_fail_positional_too_few_rows(void){
	instantiate_module_expect_error("test/compiler/tables_v05_fail_positional_too_few_rows.a4c","tables_v05_fail_positional_too_few_rows",NULL);
}

static void test_instantiate_tables_v05_fail_positional_too_many_rows(void){
	instantiate_module_expect_error("test/compiler/tables_v05_fail_positional_too_many_rows.a4c","tables_v05_fail_positional_too_many_rows",NULL);
}

static void test_instantiate_tables_v05_fail_positional_invalid_row_label(void){
	instantiate_module_expect_error("test/compiler/tables_v05_fail_positional_invalid_row_label.a4c","tables_v05_fail_positional_invalid_row_label","POSITIONAL TABLE contains non-numeric token");
}

static void test_instantiate_tables_v05_fail_positional_invalid_col_delim(void){
	instantiate_module_expect_error("test/compiler/tables_v05_fail_positional_invalid_col_delim.a4c","tables_v05_fail_positional_invalid_col_delim","Unexpected sparse-token punctuation in POSITIONAL TABLE");
}

static void test_instantiate_tables_v05_fail_sparse_repeated_labels(void){
	instantiate_module_expect_error("test/compiler/tables_v05_fail_sparse_repeated_labels.a4c","tables_v05_fail_sparse_repeated_labels","TABLE header contains invalid punctuation");
}

static void test_instantiate_tables_v05_fail_positional_leading_delim(void){
	instantiate_module_expect_error("test/compiler/tables_v05_fail_positional_leading_delim.a4c","tables_v05_fail_positional_leading_delim","TABLE row cannot begin with a delimiter");
}

static void test_instantiate_tables_v05_fail_positional_trailing_delim(void){
	instantiate_module_expect_error("test/compiler/tables_v05_fail_positional_trailing_delim.a4c","tables_v05_fail_positional_trailing_delim","TABLE delimiter cannot follow a sign without a value");
}

static void test_instantiate_tables_v05_fail_positional_double_delim(void){
	instantiate_module_expect_error("test/compiler/tables_v05_fail_positional_double_delim.a4c","tables_v05_fail_positional_double_delim",NULL);
}

static void test_instantiate_tables_v05_fail_dense_bad_col_label(void){
	instantiate_module_expect_error("test/compiler/tables_v05_fail_dense_bad_col_label.a4c","tables_v05_fail_dense_bad_col_label","TABLE column label is not a member of second index set");
}

static void test_instantiate_tables_v05_fail_dense_bad_row_label_string(void){
	instantiate_module_expect_error("test/compiler/tables_v05_fail_dense_bad_row_label_string.a4c","tables_v05_fail_dense_bad_row_label_string","TABLE row label is not a member of first index set");
}

static void test_instantiate_tables_v05_fail_units_integer(void){
	instantiate_module_expect_error("test/compiler/tables_v05_fail_units_integer.a4c","tables_v05_fail_units_integer","TABLE units are not allowed for integer values");
}

static void test_instantiate_tables_v05_fail_units_invalid(void){
	instantiate_module_expect_error("test/compiler/tables_v05_fail_units_invalid.a4c","tables_v05_fail_units_invalid","TABLE units are invalid");
}

static void test_instantiate_tables_v05_fail_units_dimension_conflict(void){
	instantiate_module_expect_error("test/compiler/tables_v05_fail_units_dimension_conflict.a4c","tables_v05_fail_units_dimension_conflict","Dimensionally inconsistent assignment");
}

/* Exercise the real parser and instantiator, including deferred array expansion. */
static void run_vector_case(const char *name, const char *kind, const char *set,
    const char *type, const char *options, const char *body,
    const char *keys[3], const double expected[3], const char *error){
	char model[4096];
	int status;
	unsigned j;
	struct Instance *sim, *root, *arr;

	snprintf(model,sizeof(model),
		"MODEL vector_case;\n"
		"i IS_A set OF %s_constant;\n%s\n"
		"TABLE x[i] IS_A %s %s;\n%s\nEND TABLE;\nEND vector_case;\n",
		kind,set,type,options,body);
	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	table_parse_error_capture_reset();
	error_reporter_set_callback(&table_parse_error_capture_cb);
	/* Load dependencies before opening a string-backed scanner buffer. */
	Asc_OpenModule("atoms.a4l",&status);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(zz_parse() == 0);
	Asc_OpenStringModule(model,&status,name);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(zz_parse() == 0);
	CU_ASSERT_FATAL(g_table_parse_error_capture.error_count == 0);
	sim = SimsCreateInstance(AddSymbol("vector_case"),AddSymbol("sim1"),e_normal,NULL);
	CU_ASSERT_FATAL(sim != NULL);
	root = GetSimulationRoot(sim);
	CU_ASSERT_FATAL(root != NULL);
	arr = ChildByChar(root,AddSymbol("x"));
	CU_ASSERT_FATAL(arr != NULL);
	if(error){
		CU_ASSERT(g_table_parse_error_capture.error_count > 0);
		CU_ASSERT(strstr(g_table_parse_error_capture.all_error_msgs,error) != NULL);
		/* Invalid layouts and labels must not partially assign the vector. */
		for(j = 1; j <= NumberChildren(arr); ++j){
			CU_ASSERT(!AtomAssigned(InstanceChild(arr,j)));
		}
		if(strstr(error,"Ambiguous") && !*set){
			CU_ASSERT(!AtomAssigned(ChildByChar(root,AddSymbol("i"))));
		}
	}else{
		CU_ASSERT(g_table_parse_error_capture.error_count == 0);
		for(j = 0; j < 3 && keys[j] != NULL; ++j){
			struct InstanceName rec;
			struct Instance *cell;
			unsigned long pos;
			if(strcmp(kind,"integer") == 0){
				SetInstanceNameType(rec,IntArrayIndex);
				SetInstanceNameIntIndex(rec,strtol(keys[j],NULL,10));
			}else{
				SetInstanceNameType(rec,StrArrayIndex);
				SetInstanceNameStrIndex(rec,AddSymbol(keys[j]));
			}
			pos = ChildSearch(arr,&rec);
			CU_ASSERT_FATAL(pos != 0);
			cell = InstanceChild(arr,pos);
			CU_ASSERT_FATAL(cell != NULL);
			CU_ASSERT_FATAL(AtomAssigned(cell));
			if(InstanceKind(cell) == INTEGER_CONSTANT_INST){
				CU_ASSERT(GetIntegerAtomValue(cell) == (long)expected[j]);
			}else{
				CU_ASSERT_FATAL(InstanceKind(cell) == REAL_CONSTANT_INST);
				CU_ASSERT_DOUBLE_EQUAL(RealAtomValue(cell),expected[j],1e-10);
			}
		}
		CU_ASSERT(NumberChildren(arr) == j);
	}
	sim_destroy(sim);
	error_reporter_set_callback(NULL);
	Asc_CompilerDestroy();
}

#define VECTOR_TEST(NAME,KIND,SET,TYPE,OPTIONS,BODY,K1,V1,K2,V2,K3,V3,ERROR) \
static void test_vector_##NAME(void){ \
	const char *keys[3] = {K1,K2,K3}; \
	const double expected[3] = {V1,V2,V3}; \
	run_vector_case(#NAME,KIND,SET,TYPE,OPTIONS,BODY,keys,expected,ERROR); \
}
#define VECTOR_FAIL(NAME,KIND,SET,TYPE,OPTIONS,BODY,ERROR) \
	VECTOR_TEST(NAME,KIND,SET,TYPE,OPTIONS,BODY,NULL,0,NULL,0,NULL,0,ERROR)
#define SYMBOLS "i :== ['a','b','c'];"
#define INTEGERS "i :== [1..3];"

VECTOR_TEST(horizontal,"symbol",SYMBOLS,"factor_constant","",
	"c, a, b;\n3.5, 1.5, 2.5;", "a",1.5,"b",2.5,"c",3.5,NULL)
VECTOR_TEST(vertical,"symbol",SYMBOLS,"factor_constant","",
	"c 3.5\na 1.5\nb 2.5", "a",1.5,"b",2.5,"c",3.5,NULL)
VECTOR_TEST(vertical_csv,"symbol",SYMBOLS,"integer_constant","",
	"b,2; a,1; c,3;", "a",1,"b",2,"c",3,NULL)
VECTOR_TEST(horizontal_tabs,"symbol",SYMBOLS,"integer_constant","",
	"\tc\ta\tb\n\t3\t1\t2", "a",1,"b",2,"c",3,NULL)
VECTOR_TEST(vertical_tabs,"symbol",SYMBOLS,"integer_constant","",
	"c\t3\na\t1\nb\t2", "a",1,"b",2,"c",3,NULL)
VECTOR_TEST(horizontal_corner,"symbol",SYMBOLS,"integer_constant","",
	",c,a,b\n,3,1,2", "a",1,"b",2,"c",3,NULL)
VECTOR_TEST(horizontal_corner_header_only,"symbol",SYMBOLS,"integer_constant","",
	",c,a,b\n3,1,2", "a",1,"b",2,"c",3,NULL)
VECTOR_TEST(horizontal_integer,"integer",INTEGERS,"integer_constant","",
	"3 1 2\n30 10 20", "1",10,"2",20,"3",30,NULL)
VECTOR_TEST(vertical_integer,"integer",INTEGERS,"integer_constant","",
	"3 30\n1 10\n2 20", "1",10,"2",20,"3",30,NULL)
VECTOR_TEST(horizontal_colon,"integer","i :== [1..2];","integer_constant","",
	": 2,1\n20,10", "1",10,"2",20,NULL,0,NULL)
VECTOR_TEST(vertical_colon,"integer","i :== [1..2];","integer_constant","",
	"2: 20\n1: 10", "1",10,"2",20,NULL,0,NULL)
VECTOR_TEST(vertical_partial_colon,"integer","i :== [1..2];","integer_constant","",
	"2 20\n1: 10", "1",10,"2",20,NULL,0,NULL)
VECTOR_TEST(inferred_horizontal,"symbol","","factor_constant","",
	"z,a,b\n3,1,2", "a",1,"b",2,"z",3,NULL)
VECTOR_TEST(inferred_vertical,"symbol","","factor_constant","",
	"z 3\na 1\nb 2", "a",1,"b",2,"z",3,NULL)
VECTOR_TEST(inferred_integer,"integer","","integer_constant","",
	": 3,1,2\n30,10,20", "1",10,"2",20,"3",30,NULL)
VECTOR_TEST(quoted_labels,"symbol","","factor_constant","",
	"'New York', 'lead,zinc', '1'\n3,2,1", "New York",3,"lead,zinc",2,"1",1,NULL)
VECTOR_TEST(quoted_vertical,"symbol","","factor_constant","",
	"'New York': 3\n'lead,zinc': 2\n'1': 1", "New York",3,"lead,zinc",2,"1",1,NULL)
VECTOR_TEST(signed_values,"symbol",SYMBOLS,"factor_constant","",
	"a b c\n- 1.5, + 2.5, -3e-2", "a",-1.5,"b",2.5,"c",-0.03,NULL)
VECTOR_TEST(signed_labels,"integer","","integer_constant","",
	": -2, +1, 3\n20,10,30", "-2",20,"1",10,"3",30,NULL)
VECTOR_TEST(single_horizontal,"symbol","","factor_constant","",
	"a\n1", "a",1,NULL,0,NULL,0,NULL)
VECTOR_TEST(single_vertical,"symbol","","factor_constant","",
	"a 1", "a",1,NULL,0,NULL,0,NULL)
VECTOR_TEST(horizontal_units,"symbol",SYMBOLS,"time_constant","UNITS {h}",
	"a b c\n1 2 3", "a",3600,"b",7200,"c",10800,NULL)
VECTOR_TEST(vertical_units,"symbol",SYMBOLS,"time_constant","UNITS {h}",
	"a 1\nb 2\nc 3", "a",3600,"b",7200,"c",10800,NULL)
VECTOR_FAIL(ambiguous,"integer","i :== [1..2];","integer_constant","",
	"1 2\n3 4", "Ambiguous 1-D TABLE")
VECTOR_FAIL(ambiguous_inferred,"integer","","integer_constant","",
	"1 2\n3 4", "Ambiguous 1-D TABLE")
VECTOR_FAIL(duplicate,"symbol",SYMBOLS,"factor_constant","",
	"a a c\n1 2 3", "duplicate labels")
VECTOR_FAIL(duplicate_inferred,"symbol","","factor_constant","",
	"a 1\na 2\nc 3", "duplicate labels")
VECTOR_FAIL(nonmember,"symbol",SYMBOLS,"factor_constant","",
	"a b z\n1 2 3", "not a member")
VECTOR_FAIL(count,"symbol",SYMBOLS,"factor_constant","",
	"a b\n1 2", "count does not match")
VECTOR_FAIL(label_type,"integer",INTEGERS,"factor_constant","",
	"a b c\n1 2 3", "not a valid integer")
VECTOR_FAIL(short_values,"symbol",SYMBOLS,"factor_constant","",
	"a b c\n1 2", "Malformed labelled 1-D TABLE")
VECTOR_FAIL(extra_values,"symbol",SYMBOLS,"factor_constant","",
	"a b c\n1 2 3 4", "Malformed labelled 1-D TABLE")
VECTOR_FAIL(empty_cell,"symbol",SYMBOLS,"factor_constant","",
	"a,b,c\n1,,3", "empty data fields")
VECTOR_FAIL(empty_label,"symbol",SYMBOLS,"factor_constant","",
	"a,,c\n1,2,3", "Malformed labelled 1-D TABLE")
VECTOR_FAIL(trailing_comma,"symbol",SYMBOLS,"factor_constant","",
	"a,b,c\n1,2,3,", "Malformed labelled 1-D TABLE")
VECTOR_FAIL(vertical_empty,"symbol",SYMBOLS,"factor_constant","",
	"a,,1\nb,2\nc,3", "empty data fields")
VECTOR_FAIL(nonnumeric,"symbol",SYMBOLS,"factor_constant","",
	"a b c\n1 foo 3", "Malformed labelled 1-D TABLE")
VECTOR_FAIL(dangling_sign,"symbol",SYMBOLS,"factor_constant","",
	"a b c\n1 2 -", "Malformed labelled 1-D TABLE")
VECTOR_FAIL(mixed_colons,"integer","i :== [1..2];","integer_constant","",
	": 1 2\n3: 4", "Malformed labelled 1-D TABLE")
VECTOR_FAIL(empty,"symbol",SYMBOLS,"factor_constant","",
	"", "Malformed labelled 1-D TABLE")
VECTOR_FAIL(integer_units,"symbol",SYMBOLS,"integer_constant","UNITS {h}",
	"a b c\n1 2 3", "units are not allowed for integer")
VECTOR_FAIL(invalid_units,"symbol",SYMBOLS,"factor_constant","UNITS {bad_units_xyz}",
	"a b c\n1 2 3", "TABLE units are invalid")
VECTOR_FAIL(dimension_conflict,"symbol",SYMBOLS,"temperature_constant","UNITS {h}",
	"a b c\n1 2 3", "Dimensionally inconsistent assignment")

#undef SYMBOLS
#undef INTEGERS
#undef VECTOR_TEST
#undef VECTOR_FAIL

#define TESTS(T) \
	T(vector_horizontal) T(vector_vertical) T(vector_vertical_csv) \
	T(vector_horizontal_tabs) T(vector_vertical_tabs) \
	T(vector_horizontal_corner) T(vector_horizontal_corner_header_only) \
	T(vector_horizontal_integer) T(vector_vertical_integer) \
	T(vector_horizontal_colon) T(vector_vertical_colon) T(vector_vertical_partial_colon) \
	T(vector_inferred_horizontal) T(vector_inferred_vertical) T(vector_inferred_integer) \
	T(vector_quoted_labels) T(vector_quoted_vertical) T(vector_signed_values) T(vector_signed_labels) \
	T(vector_single_horizontal) T(vector_single_vertical) \
	T(vector_horizontal_units) T(vector_vertical_units) \
	T(vector_ambiguous) T(vector_ambiguous_inferred) T(vector_duplicate) \
	T(vector_duplicate_inferred) T(vector_nonmember) T(vector_count) T(vector_label_type) \
	T(vector_short_values) T(vector_extra_values) T(vector_empty_cell) T(vector_empty_label) \
	T(vector_trailing_comma) T(vector_vertical_empty) T(vector_nonnumeric) \
	T(vector_dangling_sign) T(vector_mixed_colons) T(vector_empty) \
	T(vector_integer_units) T(vector_invalid_units) T(vector_dimension_conflict) \
	T(parse_tables_v05) \
	T(instantiate_tables_v05_positional) \
	T(instantiate_tables_v05_positional_csv_semicolon) \
	T(instantiate_tables_v05_dense_int_labels) \
	T(instantiate_tables_v05_dense_csv_semicolon) \
	T(instantiate_tables_v05_dense_string_labels) \
	T(instantiate_tables_v05_dense_implicit_sets) \
	T(instantiate_tables_v05_dense_real) \
	T(instantiate_tables_v05_units) \
	T(instantiate_unifac_constants_spotchecks) \
	T(parse_tables_v05_fail_table_header) \
	T(parse_tables_v05_fail_table_badchar) \
	T(parse_tables_v05_fail_table_bad_delimiter) \
	T(instantiate_tables_v05_fail_positional_short_row) \
	T(instantiate_tables_v05_fail_positional_too_many_cols) \
	T(instantiate_tables_v05_fail_positional_too_few_rows) \
	T(instantiate_tables_v05_fail_positional_too_many_rows) \
	T(instantiate_tables_v05_fail_positional_invalid_row_label) \
	T(instantiate_tables_v05_fail_positional_invalid_col_delim) \
	T(instantiate_tables_v05_fail_sparse_repeated_labels) \
	T(instantiate_tables_v05_fail_positional_leading_delim) \
	T(instantiate_tables_v05_fail_positional_trailing_delim) \
	T(instantiate_tables_v05_fail_positional_double_delim) \
	T(instantiate_tables_v05_fail_dense_bad_col_label) \
	T(instantiate_tables_v05_fail_dense_bad_row_label_string) \
	T(instantiate_tables_v05_fail_units_integer) \
	T(instantiate_tables_v05_fail_units_invalid) \
	T(instantiate_tables_v05_fail_units_dimension_conflict)

REGISTER_TESTS_SIMPLE(compiler_tables, TESTS)
