/*	ASCEND modelling environment
	Copyright (C) 2018 John Pye

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//**
	@file
	Unit test functions for compiler. Nothing here yet.
*/
#include <string.h>
#include <stdio.h>
#include <math.h>

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
#include <ascend/compiler/childio.h>
#include <ascend/compiler/instance_name.h>

#include <ascend/compiler/initialize.h>

#include <test/common.h>
#include <test/assertimpl.h>

//#define BASICS_DEBUG
#ifdef BASICS_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
#endif

/* Define this locally when you want verbose parser-error traces for table tests. */
#define TEST_TABLES_DEBUG
#ifdef TEST_TABLES_DEBUG
# define TMSG CONSOLE_DEBUG
#else
# define TMSG(ARGS...) ((void)0)
#endif

typedef struct{
	int error_count;
	int first_error_line;
	char first_error_file[256];
	char first_error_msg[512];
	char all_error_msgs[4096];
} parse_error_capture_t;

static parse_error_capture_t g_parse_error_capture;

#ifdef TEST_TABLES_DEBUG
static const char *sev_to_str(error_severity_t sev){
	switch(sev){
	case ASC_USER_SUCCESS: return "SUCCESS";
	case ASC_USER_NOTE: return "USER_NOTE";
	case ASC_USER_WARNING: return "USER_WARNING";
	case ASC_USER_ERROR: return "USER_ERROR";
	case ASC_PROG_NOTE: return "PROG_NOTE";
	case ASC_PROG_WARNING: return "PROG_WARNING";
	case ASC_PROG_ERROR: return "PROG_ERROR";
	case ASC_PROG_FATAL: return "PROG_FATAL";
	default: return "UNKNOWN";
	}
}
#endif

static void parse_error_capture_reset(void){
	memset(&g_parse_error_capture,0,sizeof(g_parse_error_capture));
}

static int parse_error_capture_cb(ERROR_REPORTER_CALLBACK_ARGS){
	char msg[sizeof(g_parse_error_capture.first_error_msg)];
	int wrote_default;
	va_list args_copy;
	size_t used;

	va_copy(args_copy,args);
	vsnprintf(msg,sizeof(msg),fmt,args_copy);
	va_end(args_copy);

	TMSG("captured [%s] %s:%d: %s"
		,sev_to_str(sev)
		,filename ? filename : "(null)"
		,line
		,msg
	);
	if(sev & ASC_ERR_ERR){
		g_parse_error_capture.error_count++;
		used = strlen(g_parse_error_capture.all_error_msgs);
		if(used + 2 < sizeof(g_parse_error_capture.all_error_msgs)){
			if(used > 0){
				snprintf(
					g_parse_error_capture.all_error_msgs + used
					,sizeof(g_parse_error_capture.all_error_msgs) - used
					,"\n"
				);
				used = strlen(g_parse_error_capture.all_error_msgs);
			}
			snprintf(
				g_parse_error_capture.all_error_msgs + used
				,sizeof(g_parse_error_capture.all_error_msgs) - used
				,"%s",msg
			);
		}
		if(g_parse_error_capture.first_error_line == 0){
			g_parse_error_capture.first_error_line = line;
			if(filename){
				snprintf(g_parse_error_capture.first_error_file
					,sizeof(g_parse_error_capture.first_error_file)
					,"%s",filename
				);
			}
			snprintf(g_parse_error_capture.first_error_msg
				,sizeof(g_parse_error_capture.first_error_msg)
				,"%s",msg
			);
		}
	}

	/* Preserve normal console/error-stream output while also capturing metadata. */
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

	parse_error_capture_reset();
	error_reporter_set_callback(&parse_error_capture_cb);

	/*m =*/ Asc_OpenModule(modulefile,&status);
	CU_ASSERT(status == 0);
	TMSG("parsing module '%s' (expecting parse errors)",modulefile);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();
	TMSG("error_count=%d first_error_line=%d",g_parse_error_capture.error_count,g_parse_error_capture.first_error_line);
	TMSG("first_error='%s'",g_parse_error_capture.first_error_msg);
	TMSG("all_errors:\n%s",g_parse_error_capture.all_error_msgs);

	CU_ASSERT(has_error == 1);
	CU_ASSERT(g_parse_error_capture.error_count > 0);
	CU_ASSERT(g_parse_error_capture.first_error_line > 0);
	if(require_column_info){
		CU_ASSERT(strstr(g_parse_error_capture.all_error_msgs,"column") != NULL);
	}
	if(msg_substr){
		CU_ASSERT(strstr(g_parse_error_capture.all_error_msgs,msg_substr) != NULL);
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

	error_reporter_set_callback(&parse_error_capture_cb);

	/*m =*/ Asc_OpenModule(modulefile,&status);
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT(0 == error_reporter_tree_has_error());
	error_reporter_tree_end();

	CU_ASSERT(FindType(AddSymbol(typename))!=NULL);

	parse_error_capture_reset();
	sim = SimsCreateInstance(AddSymbol(typename), AddSymbol("sim1"), e_normal, NULL);

	CU_ASSERT(g_parse_error_capture.error_count > 0);
	if(msg_substr){
		CU_ASSERT(strstr(g_parse_error_capture.all_error_msgs,msg_substr) != NULL);
	}

	if(sim != NULL){
		sim_destroy(sim);
	}
	error_reporter_set_callback(NULL);
	Asc_CompilerDestroy();
}

static void test_init(void){

	CU_ASSERT(0 == Asc_CompilerInit(0));

	Asc_CompilerDestroy();
}


static void test_fund_types(void){
	Asc_CompilerInit(1);
	CU_ASSERT(FindType(AddSymbol("boolean"))!=NULL);
	CU_ASSERT(FindType(AddSymbol("boolean_constant"))!=NULL);
	CU_ASSERT(FindType(AddSymbol("integer"))!=NULL);
	CU_ASSERT(FindType(AddSymbol("integer_constant"))!=NULL);
	CU_ASSERT(FindType(AddSymbol("real"))!=NULL);
	CU_ASSERT(FindType(AddSymbol("real_constant"))!=NULL);
	CU_ASSERT(FindType(AddSymbol("set"))!=NULL);
	CU_ASSERT(FindType(AddSymbol("symbol"))!=NULL);
	CU_ASSERT(FindType(AddSymbol("symbol_constant"))!=NULL);
	Asc_CompilerDestroy();
}

static void test_parse_string_module(void){

	const char *model = "\n\
		DEFINITION relation\
		    included IS_A boolean;\
		    message	IS_A symbol;\
		    included := TRUE;\
		    message := 'none';\
		END relation;\
		MODEL test1;\n\
			x IS_A real;\n\
			x - 1 = 0;\n\
		END test1;";

	Asc_CompilerInit(1);
	CU_ASSERT(FindType(AddSymbol("boolean"))!=NULL);

	struct module_t *m;
	int status;

	m = Asc_OpenStringModule(model, &status, ""/* name prefix*/);

	MSG("Asc_OpenStringModule returns status=%d",status);
	CU_ASSERT(status==0); /* if successfully created */

	MSG("Beginning parse of %s",Asc_ModuleName(m));
	status = zz_parse();

	MSG("zz_parse returns status=%d",status);
	CU_ASSERT(status==0);

	struct gl_list_t *l = Asc_TypeByModule(m);
	MSG("%lu library entries loaded from %s",gl_length(l),Asc_ModuleName(m));

	CU_ASSERT(gl_length(l)==2);
	gl_destroy(l);

	MSG("Asc_OpenStringModule returns status=%d",status);
	Asc_CompilerDestroy();
}

static void test_instantiate_string(void){

	const char *model = "(* silly little model *)\n\
		DEFINITION relation\n\
		    included IS_A boolean;\n\
		    message	IS_A symbol;\n\
		    included := TRUE;\n\
		    message := 'none';\n\
		END relation;\n\
		MODEL test1;\n\
			x IS_A real;\n\
			x_rel: x - 1 = 0;\n\
		METHODS \n\
		METHOD on_load;\n\
		END on_load;\n\
		END test1;\n";

	Asc_CompilerInit(1);
	CU_ASSERT(FindType(AddSymbol("boolean"))!=NULL);
	MSG("Boolean type found OK");
	//MSG("MODEL TEXT:\n%s",model);

	//struct module_t *m;
	int status;

	/*m =*/ Asc_OpenStringModule(model, &status, ""/* name prefix*/);
	CU_ASSERT_FATAL(status==0); /* if successfully created */

	status = zz_parse();
	CU_ASSERT_FATAL(status==0);

	CU_ASSERT(FindType(AddSymbol("test1"))!=NULL);

	struct Instance *sim = SimsCreateInstance(AddSymbol("test1"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	/* check the simulation name */
	MSG("Got simulation, name = %s",SCP(GetSimulationName(sim)));
	CU_ASSERT_FATAL(GetSimulationName(sim)==AddSymbol("sim1"));

	/* check for the expected instances */
	struct Instance *root = GetSimulationRoot(sim);

	CU_ASSERT(ChildByChar(root, AddSymbol("non_existent_var_name")) == NULL);
	CU_ASSERT(ChildByChar(root, AddSymbol("x")) != NULL);
	CU_ASSERT_FATAL(ChildByChar(root, AddSymbol("x_rel")) != NULL);
	CU_ASSERT(NumberChildren(root)==2);

	/* check instances are of expected types */
	CU_ASSERT(InstanceKind(ChildByChar(root,AddSymbol("x_rel")))==REL_INST);
	CU_ASSERT(InstanceKind(ChildByChar(root,AddSymbol("x")))==REAL_ATOM_INST);
	CU_ASSERT(InstanceKind(ChildByChar(root,AddSymbol("x")))!=REAL_INST);

	/* check attributes on relation */
	struct Instance *xrel;
	xrel = ChildByChar(root,AddSymbol("x_rel"));
	CU_ASSERT_FATAL(xrel!=NULL);
	CU_ASSERT(InstanceKind(ChildByChar(xrel,AddSymbol("included")))==BOOLEAN_INST);
	CU_ASSERT(GetBooleanAtomValue(ChildByChar(xrel,AddSymbol("included")))==TRUE);
	CU_ASSERT_FATAL(ChildByChar(xrel,AddSymbol("message"))!=NULL);
	CU_ASSERT(InstanceKind(ChildByChar(xrel,AddSymbol("message")))==SYMBOL_INST);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_parse_basemodel(void){

	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("basemodel.a4l",&status);
	CU_ASSERT(status==0);

	MSG("Beginning parse of %s",Asc_ModuleName(m));
	status = zz_parse();

	MSG("zz_parse returns status=%d",status);
	CU_ASSERT(status==0);

	struct gl_list_t *l = Asc_TypeByModule(m);
	MSG("%lu library entries loaded from %s",gl_length(l),Asc_ModuleName(m));
	gl_destroy(l);

	/* there are only 8 things declared in system.a4l: */
	CU_ASSERT(gl_length(l)==4)

	/* but system.a4l also includes basemodel.a4l, which includes... */
	CU_ASSERT(FindType(AddSymbol("cmumodel"))!=NULL);

	Asc_CompilerDestroy();
}

static void test_parse_file(void){

	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("system.a4l",&status);
	CU_ASSERT(status==0);

	MSG("Beginning parse of %s",Asc_ModuleName(m));
	status = zz_parse();

	MSG("zz_parse returns status=%d",status);
	CU_ASSERT(status==0);

	struct gl_list_t *l = Asc_TypeByModule(m);
	MSG("%lu library entries loaded from %s",gl_length(l),Asc_ModuleName(m));
	gl_destroy(l);

	/* there are only 8 things declared in system.a4l: */
	CU_ASSERT(gl_length(l)==8)

	/* here they are... */
	CU_ASSERT(FindType(AddSymbol("relation"))!=NULL);
	CU_ASSERT(FindType(AddSymbol("solver_var"))!=NULL);
	CU_ASSERT(FindType(AddSymbol("logic_relation"))!=NULL);
	CU_ASSERT(FindType(AddSymbol("solver_int"))!=NULL);
	CU_ASSERT(FindType(AddSymbol("generic_real"))!=NULL);
	CU_ASSERT(FindType(AddSymbol("boolean_var"))!=NULL);
	CU_ASSERT(FindType(AddSymbol("solver_binary"))!=NULL);
	CU_ASSERT(FindType(AddSymbol("solver_semi"))!=NULL);

	/* but system.a4l also includes basemodel.a4l, which includes... */
	CU_ASSERT(FindType(AddSymbol("cmumodel"))!=NULL);

	Asc_CompilerDestroy();
}

static void test_instantiate_file(void){

	/*struct module_t *m;*/
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	/* load the file */
	/*m = */Asc_OpenModule("johnpye/testlog10.a4c",&status);
	CU_ASSERT(status == 0);

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */
	CU_ASSERT(FindType(AddSymbol("testlog10"))!=NULL);

	/* instantiate it */
	struct Instance *sim = SimsCreateInstance(AddSymbol("testlog10"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	/* check for vars and rels */
	struct Instance *root = GetSimulationRoot(sim);
	struct Instance *inst;

	CU_ASSERT(NumberChildren(root)==5);
	CU_ASSERT((inst = ChildByChar(root,AddSymbol("x"))) && InstanceKind(inst)==REAL_ATOM_INST);
	CU_ASSERT((inst = ChildByChar(root,AddSymbol("y"))) && InstanceKind(inst)==REAL_ATOM_INST);
	CU_ASSERT((inst = ChildByChar(root,AddSymbol("z"))) && InstanceKind(inst)==REAL_ATOM_INST);

	CU_ASSERT((inst = ChildByChar(root,AddSymbol("log_10_expr"))) && InstanceKind(inst)==REL_INST);
	CU_ASSERT((inst = ChildByChar(root,AddSymbol("log_e_expr"))) && InstanceKind(inst)==REL_INST);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_initialize(void){
	/*struct module_t *m;*/
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	/* load the file */
#define TESTFILE "testinit"
	/*m =*/ Asc_OpenModule("test/compiler/" TESTFILE ".a4c",&status);
	CU_ASSERT(status == 0);

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */
	CU_ASSERT(FindType(AddSymbol(TESTFILE))!=NULL);

	/* instantiate it */
	struct Instance *sim = SimsCreateInstance(AddSymbol(TESTFILE), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	/* check for vars and rels */
	struct Instance *root = GetSimulationRoot(sim);
	struct Instance *inst;

	CU_ASSERT(NumberChildren(root)==3);
	CU_ASSERT((inst = ChildByChar(root,AddSymbol("x"))) && InstanceKind(inst)==REAL_ATOM_INST);
	CU_ASSERT((inst = ChildByChar(root,AddSymbol("y"))) && InstanceKind(inst)==REAL_ATOM_INST);

	CU_ASSERT((inst = ChildByChar(root,AddSymbol("expr1"))) && InstanceKind(inst)==REL_INST);


	/** Call on_load */
	struct Name *name = CreateIdName(AddSymbol("on_load"));
    //CONSOLE_DEBUG("RUNNING ON_LOAD");
	enum Proc_enum pe = Initialize(GetSimulationRoot(sim),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	sim_destroy(sim);
	Asc_CompilerDestroy();
#undef TESTFILE
}

static void test_stop(void){
	/*struct module_t *m;*/
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	/* load the file */
#define TESTFILE "stop"
	/*m =*/ Asc_OpenModule("test/compiler/" TESTFILE ".a4c",&status);
	CU_ASSERT(status == 0);

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */
	CU_ASSERT(FindType(AddSymbol(TESTFILE))!=NULL);

	/* instantiate it */
	struct Instance *sim = SimsCreateInstance(AddSymbol(TESTFILE), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	/** Call on_load */
	struct Name *name = CreateIdName(AddSymbol("on_load"));

	enum Proc_enum pe = Initialize(GetSimulationRoot(sim),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe!=Proc_all_ok);

	struct Instance *inst;
	CU_ASSERT((inst = ChildByChar(GetSimulationRoot(sim),AddSymbol("x"))) && InstanceKind(inst)==REAL_ATOM_INST);
	CU_ASSERT(RealAtomValue(inst)==2.0);

	sim_destroy(sim);
	Asc_CompilerDestroy();
#undef TESTFILE
}


static void test_stoponfailedassert(void){
	/*struct module_t *m;*/
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	/* load the file */
#define TESTFILE "stoponerror"
	/*m =*/ Asc_OpenModule("test/compiler/" TESTFILE ".a4c",&status);
	CU_ASSERT(status == 0);

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */
	CU_ASSERT(FindType(AddSymbol(TESTFILE))!=NULL);

	/* instantiate it */
	struct Instance *sim = SimsCreateInstance(AddSymbol(TESTFILE), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	/** Call on_load */
	struct Name *name = CreateIdName(AddSymbol("on_load"));

	enum Proc_enum pe = Initialize(GetSimulationRoot(sim),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe!=Proc_all_ok);

	sim_destroy(sim);
	Asc_CompilerDestroy();
#undef TESTFILE
}

/*
	This is a test to check ascend bug #87.
*/
static void test_badassign(void){
	/*struct module_t *m;*/
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	/* load the file */
#define TESTFILE "badassign"
	/*m =*/ Asc_OpenModule("test/compiler/" TESTFILE ".a4c",&status);
	CU_ASSERT(status == 0);

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */
	CU_ASSERT(FindType(AddSymbol(TESTFILE))!=NULL);

	/* instantiate it */
	struct Instance *sim = SimsCreateInstance(AddSymbol(TESTFILE), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	/* Call on_load */
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(sim),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe!=Proc_all_ok); /* on_load should have returned error */

	/* Check that x := 2 was NOT executed (after error statement) */
	struct Instance *inst;
	CU_ASSERT((inst = ChildByChar(GetSimulationRoot(sim),AddSymbol("x"))) && InstanceKind(inst)==REAL_ATOM_INST);
	CU_ASSERT(RealAtomValue(inst)==1.0);

	/* clean up */
	sim_destroy(sim);
	Asc_CompilerDestroy();
#undef TESTFILE
}


static void test_type_info(void){
	/*struct module_t *m;*/
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	/* load the file */
	/*m =*/ Asc_OpenModule("test/canvas/simple_recycle.a4c",&status);
	CU_ASSERT(status == 0);

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */
	struct TypeDescription *T;
	T = FindType(AddSymbol("ammonia_flash"));

	CU_ASSERT(T != NULL);

	ChildListPtr CL;
	CL = GetChildList(T);

	WriteChildList(ASCERR,CL);

	Asc_CompilerDestroy();
}



/*
	Junk pointer in errors from parser "Rejected 'model2'"...
*/
static void test_badalias(void){
	/*struct module_t *m;*/
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	/* load the file */
#define TESTFILE "badalias"
	/*m =*/ Asc_OpenModule("test/compiler/" TESTFILE ".a4c",&status);
	CU_ASSERT(status == 0);

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* check that 'badalias' model was rejected */
	CU_ASSERT(FindType(AddSymbol(TESTFILE))==NULL);

	Asc_CompilerDestroy();
#undef TESTFILE
}

static void test_parse_tables_v05(void){
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	/*m =*/ Asc_OpenModule("test/compiler/tables_v05_parse.a4c",&status);
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT(0 == error_reporter_tree_has_error());
	error_reporter_tree_clear();

	CU_ASSERT(FindType(AddSymbol("tables_v05_parse"))!=NULL);

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

static long fetch_int_table_cell_2d_is(struct Instance *root, const char *arrname, long i, const char *j){
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

	SetInstanceNameType(rec,StrArrayIndex);
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

static void test_instantiate_tables_v05_positional(void){
	int status;
	struct Instance *sim;
	struct Instance *root;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	/*m =*/ Asc_OpenModule("test/compiler/tables_v05_instantiate.a4c",&status);
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT(0 == error_reporter_tree_has_error());
	error_reporter_tree_end();

	CU_ASSERT(FindType(AddSymbol("tables_v05_instantiate"))!=NULL);

	sim = SimsCreateInstance(AddSymbol("tables_v05_instantiate"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

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

static void test_instantiate_tables_v05_positional_csv_semicolon(void){
	int status;
	struct Instance *sim;
	struct Instance *root;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	/*m =*/ Asc_OpenModule("test/compiler/tables_v05_instantiate_positional_csv_semicolon.a4c",&status);
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT(0 == error_reporter_tree_has_error());
	error_reporter_tree_end();

	CU_ASSERT(FindType(AddSymbol("tables_v05_instantiate_positional_csv_semicolon"))!=NULL);

	sim = SimsCreateInstance(AddSymbol("tables_v05_instantiate_positional_csv_semicolon"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

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

static void test_instantiate_tables_v05_dense_int_labels(void){
	int status;
	struct Instance *sim;
	struct Instance *root;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	/*m =*/ Asc_OpenModule("test/compiler/tables_v05_instantiate_dense_int.a4c",&status);
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT(0 == error_reporter_tree_has_error());
	error_reporter_tree_end();

	CU_ASSERT(FindType(AddSymbol("tables_v05_instantiate_dense_int"))!=NULL);

	sim = SimsCreateInstance(AddSymbol("tables_v05_instantiate_dense_int"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);
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

static void test_instantiate_tables_v05_dense_csv_semicolon(void){
	int status;
	struct Instance *sim;
	struct Instance *root;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	/*m =*/ Asc_OpenModule("test/compiler/tables_v05_instantiate_dense_csv_semicolon.a4c",&status);
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT(0 == error_reporter_tree_has_error());
	error_reporter_tree_end();

	CU_ASSERT(FindType(AddSymbol("tables_v05_instantiate_dense_csv_semicolon"))!=NULL);

	sim = SimsCreateInstance(AddSymbol("tables_v05_instantiate_dense_csv_semicolon"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);
	root = GetSimulationRoot(sim);
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
	int status;
	struct Instance *sim;
	struct Instance *root;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	/*m =*/ Asc_OpenModule("test/compiler/tables_v05_instantiate_dense_string.a4c",&status);
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT(0 == error_reporter_tree_has_error());
	error_reporter_tree_end();

	CU_ASSERT(FindType(AddSymbol("tables_v05_instantiate_dense_string"))!=NULL);

	sim = SimsCreateInstance(AddSymbol("tables_v05_instantiate_dense_string"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);
	root = GetSimulationRoot(sim);
	CU_ASSERT_FATAL(root!=NULL);

	CU_ASSERT(fetch_int_table_cell_2d_ss(root,"cost","north","x") == 11);
	CU_ASSERT(fetch_int_table_cell_2d_ss(root,"cost","north","y") == 12);
	CU_ASSERT(fetch_int_table_cell_2d_ss(root,"cost","south","x") == 21);
	CU_ASSERT(fetch_int_table_cell_2d_ss(root,"cost","south","y") == 22);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_instantiate_tables_v05_dense_implicit_sets(void){
	int status;
	struct Instance *sim;
	struct Instance *root;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	/*m =*/ Asc_OpenModule("test/compiler/tables_v05_instantiate_dense_implicit.a4c",&status);
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT(0 == error_reporter_tree_has_error());
	error_reporter_tree_end();

	CU_ASSERT(FindType(AddSymbol("tables_v05_instantiate_dense_implicit"))!=NULL);

	sim = SimsCreateInstance(AddSymbol("tables_v05_instantiate_dense_implicit"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);
	root = GetSimulationRoot(sim);
	CU_ASSERT_FATAL(root!=NULL);

	CU_ASSERT(fetch_int_table_cell_2d_is(root,"cost",1,"a") == 11);
	CU_ASSERT(fetch_int_table_cell_2d_is(root,"cost",1,"b") == 12);
	CU_ASSERT(fetch_int_table_cell_2d_is(root,"cost",2,"a") == 21);
	CU_ASSERT(fetch_int_table_cell_2d_is(root,"cost",2,"b") == 22);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_parse_tables_v05_fail_table_header(void){
	parse_module_expect_error(
		"test/compiler/tables_v05_fail_table_header.a4c"
		, "tables_v05_fail_table_header"
		, "syntax error"
		, 1
		, 1
	);
}

static void test_parse_tables_v05_fail_table_badchar(void){
	parse_module_expect_error(
		"test/compiler/tables_v05_fail_table_badchar.a4c"
		, "tables_v05_fail_table_badchar"
		, "Unexpected character"
		, 0
		, 1
	);
}

static void test_parse_tables_v05_fail_table_bad_delimiter(void){
	parse_module_expect_error(
		"test/compiler/tables_v05_fail_table_bad_delimiter.a4c"
		, "tables_v05_fail_table_bad_delimiter"
		, "syntax error"
		, 0
		, 1
	);
}

static void test_instantiate_tables_v05_fail_positional_short_row(void){
	instantiate_module_expect_error(
		"test/compiler/tables_v05_fail_positional_short_row.a4c"
		, "tables_v05_fail_positional_short_row"
		, NULL
	);
}

static void test_instantiate_tables_v05_fail_positional_too_many_cols(void){
	instantiate_module_expect_error(
		"test/compiler/tables_v05_fail_positional_too_many_cols.a4c"
		, "tables_v05_fail_positional_too_many_cols"
		, NULL
	);
}

static void test_instantiate_tables_v05_fail_positional_too_few_rows(void){
	instantiate_module_expect_error(
		"test/compiler/tables_v05_fail_positional_too_few_rows.a4c"
		, "tables_v05_fail_positional_too_few_rows"
		, NULL
	);
}

static void test_instantiate_tables_v05_fail_positional_too_many_rows(void){
	instantiate_module_expect_error(
		"test/compiler/tables_v05_fail_positional_too_many_rows.a4c"
		, "tables_v05_fail_positional_too_many_rows"
		, NULL
	);
}

static void test_instantiate_tables_v05_fail_positional_invalid_row_label(void){
	instantiate_module_expect_error(
		"test/compiler/tables_v05_fail_positional_invalid_row_label.a4c"
		, "tables_v05_fail_positional_invalid_row_label"
		, "POSITIONAL TABLE contains non-numeric token"
	);
}

static void test_instantiate_tables_v05_fail_positional_invalid_col_delim(void){
	instantiate_module_expect_error(
		"test/compiler/tables_v05_fail_positional_invalid_col_delim.a4c"
		, "tables_v05_fail_positional_invalid_col_delim"
		, "Unexpected sparse-token punctuation in POSITIONAL TABLE"
	);
}

static void test_instantiate_tables_v05_fail_sparse_repeated_labels(void){
	instantiate_module_expect_error(
		"test/compiler/tables_v05_fail_sparse_repeated_labels.a4c"
		, "tables_v05_fail_sparse_repeated_labels"
		, "TABLE header contains invalid punctuation"
	);
}

static void test_instantiate_tables_v05_fail_positional_leading_delim(void){
	instantiate_module_expect_error(
		"test/compiler/tables_v05_fail_positional_leading_delim.a4c"
		, "tables_v05_fail_positional_leading_delim"
		, "TABLE row cannot begin with a delimiter"
	);
}

static void test_instantiate_tables_v05_fail_positional_trailing_delim(void){
	instantiate_module_expect_error(
		"test/compiler/tables_v05_fail_positional_trailing_delim.a4c"
		, "tables_v05_fail_positional_trailing_delim"
		, "TABLE delimiter cannot follow a sign without a value"
	);
}

static void test_instantiate_tables_v05_fail_positional_double_delim(void){
	instantiate_module_expect_error(
		"test/compiler/tables_v05_fail_positional_double_delim.a4c"
		, "tables_v05_fail_positional_double_delim"
		, NULL
	);
}

static void test_instantiate_tables_v05_fail_dense_bad_col_label(void){
	instantiate_module_expect_error(
		"test/compiler/tables_v05_fail_dense_bad_col_label.a4c"
		, "tables_v05_fail_dense_bad_col_label"
		, "TABLE column label is not a member of second index set"
	);
}

static void test_instantiate_tables_v05_fail_dense_bad_row_label_string(void){
	instantiate_module_expect_error(
		"test/compiler/tables_v05_fail_dense_bad_row_label_string.a4c"
		, "tables_v05_fail_dense_bad_row_label_string"
		, "TABLE row label is not a member of first index set"
	);
}


/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(init) \
	T(fund_types) \
	T(parse_string_module) \
	T(instantiate_string) \
	T(parse_basemodel) \
	T(parse_file) \
	T(instantiate_file) \
	T(initialize) \
	T(stop) \
	T(stoponfailedassert) \
	T(badassign) \
	T(type_info) \
	T(badalias) \
	T(parse_tables_v05) \
	T(instantiate_tables_v05_positional) \
	T(instantiate_tables_v05_positional_csv_semicolon) \
	T(instantiate_tables_v05_dense_int_labels) \
	T(instantiate_tables_v05_dense_csv_semicolon) \
	T(instantiate_tables_v05_dense_string_labels) \
	T(instantiate_tables_v05_dense_implicit_sets) \
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
	T(instantiate_tables_v05_fail_dense_bad_row_label_string)

REGISTER_TESTS_SIMPLE(compiler_basics, TESTS)
