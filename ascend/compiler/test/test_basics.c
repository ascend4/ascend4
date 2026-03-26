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
#include <ascend/compiler/slist.h>
#include <ascend/compiler/statio.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/type_desc.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/childio.h>
#include <ascend/compiler/instance_name.h>
#include <ascend/compiler/units.h>
#include <ascend/compiler/when_util.h>

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

static void test_initial_section_basic(void){
	const char *model = "(* INITIAL syntax smoke test *)\n\
		DEFINITION relation\n\
		    included IS_A boolean;\n\
		    initial IS_A boolean;\n\
		    message IS_A symbol;\n\
		    included := TRUE;\n\
		    initial := FALSE;\n\
		    message := 'none';\n\
		END relation;\n\
		MODEL test_initial_basic;\n\
			x IS_A real;\n\
			x_rel: x - 1 = 0;\n\
		INITIAL\n\
			x_init: x = 1;\n\
		END test_initial_basic;\n";

	int status;
	struct TypeDescription *t;
	struct Instance *sim;
	struct Instance *root;

	Asc_CompilerInit(1);
	Asc_OpenStringModule(model, &status, "");
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(zz_parse() == 0);

	t = FindType(AddSymbol("test_initial_basic"));
	CU_ASSERT_FATAL(t != NULL);
	CU_ASSERT_EQUAL(gl_length(GetList(GetStatementList(t))), 2);
	CU_ASSERT_EQUAL(gl_length(GetList(GetInitialStatementList(t))), 1);
	CU_ASSERT_EQUAL(GetExecutableStatementCount(t), 3);

	sim = SimsCreateInstance(AddSymbol("test_initial_basic"), AddSymbol("sim_initial"), e_normal, NULL);
	CU_ASSERT_FATAL(sim != NULL);
	root = GetSimulationRoot(sim);
	CU_ASSERT_FATAL(root != NULL);

	CU_ASSERT(ChildByChar(root, AddSymbol("x")) != NULL);
	CU_ASSERT(ChildByChar(root, AddSymbol("x_rel")) != NULL);
	CU_ASSERT_FATAL(ChildByChar(root, AddSymbol("x_init")) != NULL);
	CU_ASSERT(InstanceKind(ChildByChar(root, AddSymbol("x_init"))) == REL_INST);
	CU_ASSERT_FATAL(ChildByChar(ChildByChar(root, AddSymbol("x_init")), AddSymbol("initial")) != NULL);
	CU_ASSERT(GetBooleanAtomValue(ChildByChar(ChildByChar(root, AddSymbol("x_init")), AddSymbol("initial"))) == TRUE);
	CU_ASSERT(GetBooleanAtomValue(ChildByChar(ChildByChar(root, AddSymbol("x_init")), AddSymbol("included"))) == FALSE);
	CU_ASSERT(GetBooleanAtomValue(ChildByChar(ChildByChar(root, AddSymbol("x_rel")), AddSymbol("initial"))) == FALSE);

	SetInitialRelationInclusion(root, TRUE);
	CU_ASSERT(GetBooleanAtomValue(ChildByChar(ChildByChar(root, AddSymbol("x_init")), AddSymbol("included"))) == TRUE);
	SetInitialRelationInclusion(root, FALSE);
	CU_ASSERT(GetBooleanAtomValue(ChildByChar(ChildByChar(root, AddSymbol("x_init")), AddSymbol("included"))) == FALSE);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_initial_section_hierarchical(void){
	const char *model = "(* INITIAL hierarchy test *)\n\
		DEFINITION relation\n\
		    included IS_A boolean;\n\
		    initial IS_A boolean;\n\
		    message IS_A symbol;\n\
		    included := TRUE;\n\
		    initial := FALSE;\n\
		    message := 'none';\n\
		END relation;\n\
		MODEL child_initial;\n\
			y IS_A real;\n\
			y_rel: y - 2 = 0;\n\
		INITIAL\n\
			y_init: y = 3;\n\
		END child_initial;\n\
		MODEL parent_initial;\n\
			c IS_A child_initial;\n\
			parent_rel: c.y - 2 = 0;\n\
		INITIAL\n\
			parent_init: c.y = 4;\n\
		END parent_initial;\n";

	int status;
	struct Instance *sim;
	struct Instance *root;
	struct Instance *child;
	struct Instance *y_init;
	struct Instance *parent_init;

	Asc_CompilerInit(1);
	Asc_OpenStringModule(model, &status, "");
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(zz_parse() == 0);

	sim = SimsCreateInstance(AddSymbol("parent_initial"), AddSymbol("sim_parent_initial"), e_normal, NULL);
	CU_ASSERT_FATAL(sim != NULL);
	root = GetSimulationRoot(sim);
	CU_ASSERT_FATAL(root != NULL);
	child = ChildByChar(root, AddSymbol("c"));
	CU_ASSERT_FATAL(child != NULL);
	y_init = ChildByChar(child, AddSymbol("y_init"));
	parent_init = ChildByChar(root, AddSymbol("parent_init"));
	CU_ASSERT_FATAL(y_init != NULL);
	CU_ASSERT_FATAL(parent_init != NULL);

	CU_ASSERT(GetBooleanAtomValue(ChildByChar(ChildByChar(root, AddSymbol("parent_init")), AddSymbol("initial"))) == TRUE);
	CU_ASSERT(GetBooleanAtomValue(ChildByChar(ChildByChar(child, AddSymbol("y_init")), AddSymbol("initial"))) == TRUE);
	CU_ASSERT(GetBooleanAtomValue(ChildByChar(ChildByChar(root, AddSymbol("parent_init")), AddSymbol("included"))) == FALSE);
	CU_ASSERT(GetBooleanAtomValue(ChildByChar(ChildByChar(child, AddSymbol("y_init")), AddSymbol("included"))) == FALSE);

	SetInitialRelationInclusion(root, TRUE);
	CU_ASSERT(GetBooleanAtomValue(ChildByChar(ChildByChar(root, AddSymbol("parent_init")), AddSymbol("included"))) == TRUE);
	CU_ASSERT(GetBooleanAtomValue(ChildByChar(ChildByChar(child, AddSymbol("y_init")), AddSymbol("included"))) == TRUE);

	SetInitialRelationInclusion(root, FALSE);
	CU_ASSERT(GetBooleanAtomValue(ChildByChar(ChildByChar(root, AddSymbol("parent_init")), AddSymbol("included"))) == FALSE);
	CU_ASSERT(GetBooleanAtomValue(ChildByChar(ChildByChar(child, AddSymbol("y_init")), AddSymbol("included"))) == FALSE);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_initial_section_illegal_statement_rejected(void){
	int status;
	int has_error;
	const char *model = "\n\
		MODEL initial_illegal;\n\
			x IS_A real;\n\
		INITIAL\n\
			y IS_A real;\n\
		END initial_illegal;";

	Asc_CompilerInit(1);
	parse_error_capture_reset();
	error_reporter_set_callback(&parse_error_capture_cb);

	Asc_OpenStringModule(model, &status, "");
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();

	CU_ASSERT(has_error == 1);
	CU_ASSERT(g_parse_error_capture.error_count > 0);
	CU_ASSERT(strstr(g_parse_error_capture.all_error_msgs, "Statement not allowed in context") != NULL);
	CU_ASSERT(FindType(AddSymbol("initial_illegal")) == NULL);

	error_reporter_set_callback(NULL);
	Asc_CompilerDestroy();
}

static void test_pre_outside_reinit_rejected(void){
	int status;
	int has_error;
	const char *model = "\n\
		MODEL pre_illegal;\n\
			x, y IS_A real;\n\
			bad: y = pre(x);\n\
		END pre_illegal;";

	Asc_CompilerInit(1);
	parse_error_capture_reset();
	error_reporter_set_callback(&parse_error_capture_cb);

	Asc_OpenStringModule(model, &status, "");
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();

	CU_ASSERT(has_error == 1);
	CU_ASSERT(g_parse_error_capture.error_count > 0);
	CU_ASSERT(strstr(g_parse_error_capture.all_error_msgs, "pre(...) is only allowed inside REINIT") != NULL);
	CU_ASSERT(FindType(AddSymbol("pre_illegal")) == NULL);

	error_reporter_set_callback(NULL);
	Asc_CompilerDestroy();
}

static void test_pre_in_conditional_rejected(void){
	int status;
	int has_error;
	const char *model = "\n\
		MODEL pre_conditional_illegal;\n\
			x IS_A real;\n\
		CONDITIONAL\n\
			bad: pre(x) > 0;\n\
		END CONDITIONAL;\n\
		END pre_conditional_illegal;";

	Asc_CompilerInit(1);
	parse_error_capture_reset();
	error_reporter_set_callback(&parse_error_capture_cb);

	Asc_OpenStringModule(model, &status, "");
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();

	CU_ASSERT(has_error == 1);
	CU_ASSERT(g_parse_error_capture.error_count > 0);
	CU_ASSERT(strstr(g_parse_error_capture.all_error_msgs, "pre(...) is only allowed inside REINIT") != NULL);
	CU_ASSERT(FindType(AddSymbol("pre_conditional_illegal")) == NULL);

	error_reporter_set_callback(NULL);
	Asc_CompilerDestroy();
}

static void test_lowercase_der_statement_rejected(void){
	int status;
	int has_error;
	const char *model = "\n\
		MODEL der_stmt_case_illegal;\n\
			x, y IS_A real;\n\
			der(x, y);\n\
		END der_stmt_case_illegal;";

	Asc_CompilerInit(1);
	parse_error_capture_reset();
	error_reporter_set_callback(&parse_error_capture_cb);

	Asc_OpenStringModule(model, &status, "");
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();

	CU_ASSERT(has_error == 1);
	CU_ASSERT(g_parse_error_capture.error_count > 0);
	CU_ASSERT(FindType(AddSymbol("der_stmt_case_illegal")) == NULL);

	error_reporter_set_callback(NULL);
	Asc_CompilerDestroy();
}

static void test_uppercase_der_expr_rejected(void){
	int status;
	int has_error;
	const char *model = "\n\
		MODEL der_expr_case_illegal;\n\
			x, y IS_A real;\n\
			eq: y = DER(x);\n\
		END der_expr_case_illegal;";

	Asc_CompilerInit(1);
	parse_error_capture_reset();
	error_reporter_set_callback(&parse_error_capture_cb);

	Asc_OpenStringModule(model, &status, "");
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();

	CU_ASSERT(has_error == 1);
	CU_ASSERT(g_parse_error_capture.error_count > 0);
	CU_ASSERT(FindType(AddSymbol("der_expr_case_illegal")) == NULL);

	error_reporter_set_callback(NULL);
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
	unsigned long n = gl_length(l);
	MSG("%lu library entries loaded from %s",n,Asc_ModuleName(m));
	gl_destroy(l);

	/* system.a4l now declares 9 public types, including selector. */
	CU_ASSERT(n==9)

	/* here they are... */
	CU_ASSERT(FindType(AddSymbol("relation"))!=NULL);
	CU_ASSERT(FindType(AddSymbol("solver_var"))!=NULL);
	CU_ASSERT(FindType(AddSymbol("logic_relation"))!=NULL);
	CU_ASSERT(FindType(AddSymbol("solver_int"))!=NULL);
	CU_ASSERT(FindType(AddSymbol("generic_real"))!=NULL);
	CU_ASSERT(FindType(AddSymbol("boolean_var"))!=NULL);
	CU_ASSERT(FindType(AddSymbol("selector"))!=NULL);
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

static void test_atom_declared_units_from_default(void){
	int status;
	int has_error;
	struct TypeDescription *t;
	const char *model = "\n\
		UNITS\n\
			MW = {1e6*kg*m^2/s^3};\n\
		END UNITS;\n\
		ATOM atom_decl_units REFINES real DIMENSION M*L^2/T^3 DEFAULT 5 {MW};\n\
		END atom_decl_units;";

	Asc_CompilerInit(1);
	parse_error_capture_reset();
	error_reporter_set_callback(&parse_error_capture_cb);

	/*m =*/ Asc_OpenStringModule(model, &status, "");
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();
	CU_ASSERT(has_error == 0);

	t = FindType(AddSymbol("atom_decl_units"));
	CU_ASSERT_FATAL(t != NULL);
	CU_ASSERT(GetBaseType(t) == real_type);
	CU_ASSERT_FATAL(GetRealDeclaredUnits(t) != NULL);
	CU_ASSERT_STRING_EQUAL(SCP(GetRealDeclaredUnits(t)), "MW");

	error_reporter_set_callback(NULL);
	Asc_CompilerDestroy();
}

static void test_constant_units_clause_and_declared_units(void){
	int status;
	int has_error;
	const struct Units *u;
	struct TypeDescription *t;
	const char *model = "\n\
		UNITS\n\
			MWh = {3.6e9*kg*m^2/s^2};\n\
			USD_per_MWh = {USD/MWh};\n\
		END UNITS;\n\
		CONSTANT const_units_no_default REFINES real_constant UNITS {MWh};\n\
		CONSTANT const_units_with_default REFINES real_constant UNITS {USD_per_MWh} :== 20 {USD_per_MWh};";

	Asc_CompilerInit(1);
	parse_error_capture_reset();
	error_reporter_set_callback(&parse_error_capture_cb);

	/*m =*/ Asc_OpenStringModule(model, &status, "");
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();
	CU_ASSERT(has_error == 0);

	t = FindType(AddSymbol("const_units_no_default"));
	CU_ASSERT_FATAL(t != NULL);
	CU_ASSERT(GetBaseType(t) == real_constant_type);
	CU_ASSERT(ConstantDefaulted(t) == 0);
	CU_ASSERT_FATAL(GetConstantDeclaredUnits(t) != NULL);
	CU_ASSERT_STRING_EQUAL(SCP(GetConstantDeclaredUnits(t)), "MWh");
	u = LookupUnits("MWh");
	CU_ASSERT_FATAL(u != NULL);
	CU_ASSERT(GetConstantDimens(t) == UnitsDimensions(u));

	t = FindType(AddSymbol("const_units_with_default"));
	CU_ASSERT_FATAL(t != NULL);
	CU_ASSERT(GetBaseType(t) == real_constant_type);
	CU_ASSERT(ConstantDefaulted(t) == 1);
	CU_ASSERT_FATAL(GetConstantDeclaredUnits(t) != NULL);
	CU_ASSERT_STRING_EQUAL(SCP(GetConstantDeclaredUnits(t)), "USD_per_MWh");
	u = LookupUnits("USD_per_MWh");
	CU_ASSERT_FATAL(u != NULL);
	CU_ASSERT(GetConstantDimens(t) == UnitsDimensions(u));
	CU_ASSERT_DOUBLE_EQUAL(GetConstantDefReal(t), 20.0 * UnitsConvFactor(u), 1e-12);

	error_reporter_set_callback(NULL);
	Asc_CompilerDestroy();
}

static void test_constant_units_clause_invalid_units(void){
	int status;
	int has_error;
	const char *model = "\n\
		CONSTANT const_units_invalid REFINES real_constant UNITS {NO_SUCH_UNIT};";

	Asc_CompilerInit(1);
	parse_error_capture_reset();
	error_reporter_set_callback(&parse_error_capture_cb);

	/*m =*/ Asc_OpenStringModule(model, &status, "");
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();

	CU_ASSERT(has_error == 1);
	CU_ASSERT(g_parse_error_capture.error_count > 0);
	CU_ASSERT(strstr(g_parse_error_capture.all_error_msgs, "Undefined units") != NULL);
	CU_ASSERT(FindType(AddSymbol("const_units_invalid")) == NULL);

	error_reporter_set_callback(NULL);
	Asc_CompilerDestroy();
}

static void test_constant_units_clause_nonreal_rejected(void){
	int status;
	int has_error;
	const char *model = "\n\
		CONSTANT const_units_nonreal REFINES integer_constant UNITS {kg};";

	Asc_CompilerInit(1);
	parse_error_capture_reset();
	error_reporter_set_callback(&parse_error_capture_cb);

	/*m =*/ Asc_OpenStringModule(model, &status, "");
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();

	CU_ASSERT(has_error == 1);
	CU_ASSERT(g_parse_error_capture.error_count > 0);
	CU_ASSERT(strstr(g_parse_error_capture.all_error_msgs, "non-real type") != NULL);
	CU_ASSERT(FindType(AddSymbol("const_units_nonreal")) == NULL);

	error_reporter_set_callback(NULL);
	Asc_CompilerDestroy();
}

static void test_atom_declared_units_inherited_on_refine(void){
	int status;
	int has_error;
	struct TypeDescription *parent;
	struct TypeDescription *child;
	const char *model = "\n\
		UNITS\n\
			MW = {1e6*kg*m^2/s^3};\n\
		END UNITS;\n\
		ATOM atom_decl_units_parent REFINES real DIMENSION M*L^2/T^3 DEFAULT 5 {MW};\n\
		END atom_decl_units_parent;\n\
		ATOM atom_decl_units_child REFINES atom_decl_units_parent;\n\
		END atom_decl_units_child;";

	Asc_CompilerInit(1);
	parse_error_capture_reset();
	error_reporter_set_callback(&parse_error_capture_cb);

	/*m =*/ Asc_OpenStringModule(model, &status, "");
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();
	CU_ASSERT(has_error == 0);

	parent = FindType(AddSymbol("atom_decl_units_parent"));
	CU_ASSERT_FATAL(parent != NULL);
	CU_ASSERT(GetBaseType(parent) == real_type);
	CU_ASSERT_FATAL(GetRealDeclaredUnits(parent) != NULL);
	CU_ASSERT_STRING_EQUAL(SCP(GetRealDeclaredUnits(parent)), "MW");

	child = FindType(AddSymbol("atom_decl_units_child"));
	CU_ASSERT_FATAL(child != NULL);
	CU_ASSERT(GetBaseType(child) == real_type);
	CU_ASSERT_FATAL(GetRealDeclaredUnits(child) != NULL);
	CU_ASSERT_STRING_EQUAL(SCP(GetRealDeclaredUnits(child)), "MW");

	error_reporter_set_callback(NULL);
	Asc_CompilerDestroy();
}

static void test_atom_declared_units_null_without_units(void){
	int status;
	int has_error;
	struct TypeDescription *t;
	const char *model = "\n\
		ATOM atom_decl_units_null REFINES real DIMENSION M*L^2/T^3;\n\
		END atom_decl_units_null;";

	Asc_CompilerInit(1);
	parse_error_capture_reset();
	error_reporter_set_callback(&parse_error_capture_cb);

	/*m =*/ Asc_OpenStringModule(model, &status, "");
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();
	CU_ASSERT(has_error == 0);

	t = FindType(AddSymbol("atom_decl_units_null"));
	CU_ASSERT_FATAL(t != NULL);
	CU_ASSERT(GetBaseType(t) == real_type);
	CU_ASSERT(GetRealDeclaredUnits(t) == NULL);

	error_reporter_set_callback(NULL);
	Asc_CompilerDestroy();
}

static void test_constant_declared_units_inherited_on_refine(void){
	int status;
	int has_error;
	const struct Units *u;
	struct TypeDescription *parent;
	struct TypeDescription *child;
	const char *model = "\n\
		UNITS\n\
			MWh = {3.6e9*kg*m^2/s^2};\n\
		END UNITS;\n\
		CONSTANT const_units_parent REFINES real_constant UNITS {MWh};\n\
		CONSTANT const_units_child REFINES const_units_parent;";

	Asc_CompilerInit(1);
	parse_error_capture_reset();
	error_reporter_set_callback(&parse_error_capture_cb);

	/*m =*/ Asc_OpenStringModule(model, &status, "");
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();
	CU_ASSERT(has_error == 0);

	u = LookupUnits("MWh");
	CU_ASSERT_FATAL(u != NULL);

	parent = FindType(AddSymbol("const_units_parent"));
	CU_ASSERT_FATAL(parent != NULL);
	CU_ASSERT(GetBaseType(parent) == real_constant_type);
	CU_ASSERT_FATAL(GetConstantDeclaredUnits(parent) != NULL);
	CU_ASSERT_STRING_EQUAL(SCP(GetConstantDeclaredUnits(parent)), "MWh");
	CU_ASSERT(GetConstantDimens(parent) == UnitsDimensions(u));

	child = FindType(AddSymbol("const_units_child"));
	CU_ASSERT_FATAL(child != NULL);
	CU_ASSERT(GetBaseType(child) == real_constant_type);
	CU_ASSERT_FATAL(GetConstantDeclaredUnits(child) != NULL);
	CU_ASSERT_STRING_EQUAL(SCP(GetConstantDeclaredUnits(child)), "MWh");
	CU_ASSERT(GetConstantDimens(child) == UnitsDimensions(u));

	error_reporter_set_callback(NULL);
	Asc_CompilerDestroy();
}

static void test_constant_units_clause_mismatched_default_rejected(void){
	int status;
	int has_error;
	const char *model = "\n\
		UNITS\n\
			MWh = {3.6e9*kg*m^2/s^2};\n\
			MW = {1e6*kg*m^2/s^3};\n\
		END UNITS;\n\
		CONSTANT const_units_bad_default REFINES real_constant UNITS {MWh} :== 20 {MW};";

	Asc_CompilerInit(1);
	parse_error_capture_reset();
	error_reporter_set_callback(&parse_error_capture_cb);

	/*m =*/ Asc_OpenStringModule(model, &status, "");
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();

	CU_ASSERT(has_error == 1);
	CU_ASSERT(g_parse_error_capture.error_count > 0);
	CU_ASSERT(FindType(AddSymbol("const_units_bad_default")) == NULL);

	error_reporter_set_callback(NULL);
	Asc_CompilerDestroy();
}

static void test_units_ladder_define_and_extend(void){
	int status;
	int has_error;
	const struct Units *u_w;
	const struct Units *u_kw;
	const struct Units *u_hp;
	const struct Units *u_mw;
	long ladder_id;
	const char *model = "\n\
		UNITS LADDER\n\
			W = {kg*m^2/s^3};\n\
			kW = {1e3*W};\n\
			MW = {1e6*W};\n\
		END UNITS LADDER;\n\
		UNITS LADDER\n\
			kW;\n\
			hp = {0.745699872*kW};\n\
		END UNITS LADDER;";

	Asc_CompilerInit(1);
	parse_error_capture_reset();
	error_reporter_set_callback(&parse_error_capture_cb);

	/*m =*/ Asc_OpenStringModule(model, &status, "");
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();
	CU_ASSERT(has_error == 0);

	u_w = LookupUnits("W");
	u_kw = LookupUnits("kW");
	u_hp = LookupUnits("hp");
	u_mw = LookupUnits("MW");

	CU_ASSERT_FATAL(u_w != NULL);
	CU_ASSERT_FATAL(u_kw != NULL);
	CU_ASSERT_FATAL(u_hp != NULL);
	CU_ASSERT_FATAL(u_mw != NULL);

	ladder_id = UnitsLadderId(u_w);
	CU_ASSERT(ladder_id >= 0);
	CU_ASSERT(UnitsLadderId(u_kw) == ladder_id);
	CU_ASSERT(UnitsLadderId(u_hp) == ladder_id);
	CU_ASSERT(UnitsLadderId(u_mw) == ladder_id);

	CU_ASSERT(UnitsLadderRank(u_w) == 0);
	CU_ASSERT(UnitsLadderRank(u_kw) == 1);
	CU_ASSERT(UnitsLadderRank(u_hp) == 2);
	CU_ASSERT(UnitsLadderRank(u_mw) == 3);

	CU_ASSERT(LookupUnitsByLadder(ladder_id,0) == u_w);
	CU_ASSERT(LookupUnitsByLadder(ladder_id,1) == u_kw);
	CU_ASSERT(LookupUnitsByLadder(ladder_id,2) == u_hp);
	CU_ASSERT(LookupUnitsByLadder(ladder_id,3) == u_mw);

	error_reporter_set_callback(NULL);
	Asc_CompilerDestroy();
}

static void test_units_ladder_invalid_anchor_rejected(void){
	int status;
	int has_error;
	const struct Units *u;
	const char *model = "\n\
		UNITS LADDER\n\
			kg;\n\
			slug = {14.59390294*kg};\n\
		END UNITS LADDER;";

	Asc_CompilerInit(1);
	parse_error_capture_reset();
	error_reporter_set_callback(&parse_error_capture_cb);

	/*m =*/ Asc_OpenStringModule(model, &status, "");
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(0 == zz_parse());
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();
	CU_ASSERT(has_error == 1);
	CU_ASSERT(g_parse_error_capture.error_count > 0);
	CU_ASSERT(strstr(g_parse_error_capture.all_error_msgs, "anchor") != NULL);

	u = LookupUnits("slug");
	CU_ASSERT(u == NULL);

	error_reporter_set_callback(NULL);
	Asc_CompilerDestroy();
}


/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(init) \
	T(fund_types) \
	T(parse_string_module) \
	T(instantiate_string) \
	T(initial_section_basic) \
	T(initial_section_hierarchical) \
	T(initial_section_illegal_statement_rejected) \
	T(pre_outside_reinit_rejected) \
	T(pre_in_conditional_rejected) \
	T(lowercase_der_statement_rejected) \
	T(uppercase_der_expr_rejected) \
	T(parse_basemodel) \
	T(parse_file) \
	T(instantiate_file) \
	T(initialize) \
	T(stop) \
	T(stoponfailedassert) \
	T(badassign) \
	T(type_info) \
	T(badalias) \
	T(atom_declared_units_from_default) \
	T(constant_units_clause_and_declared_units) \
	T(constant_units_clause_invalid_units) \
	T(constant_units_clause_nonreal_rejected) \
	T(atom_declared_units_inherited_on_refine) \
	T(atom_declared_units_null_without_units) \
	T(constant_declared_units_inherited_on_refine) \
	T(constant_units_clause_mismatched_default_rejected) \
	T(units_ladder_define_and_extend) \
	T(units_ladder_invalid_anchor_rejected)

REGISTER_TESTS_SIMPLE(compiler_basics, TESTS)
