/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//**
	@file
	Unit test functions for compiler. Nothing here yet.
*/
#include <string.h>
#include <CUnit/CUnit.h>

#include <general/env.h>
#include <utilities/ascConfig.h>
#include <utilities/ascEnvVar.h>
#include <utilities/error.h>

#include <compiler/ascCompiler.h>
#include <compiler/module.h>
#include <compiler/parser.h>
#include <compiler/library.h>
#include <compiler/symtab.h>
#include <compiler/simlist.h>
#include <compiler/instquery.h>
#include <compiler/parentchild.h>
#include <compiler/atomvalue.h>

#include <assertimpl.h>

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

	struct module_t *m;
	int status;
	
	m = Asc_OpenStringModule(model, &status, ""/* name prefix*/);

	CONSOLE_DEBUG("Asc_OpenStringModule returns status=%d",status);
	CU_ASSERT(status==0); /* if successfully created */

	CONSOLE_DEBUG("Beginning parse of %s",Asc_ModuleName(m));
	status = zz_parse();

	CONSOLE_DEBUG("zz_parse returns status=%d",status);

	CU_ASSERT(status==0);

	struct gl_list_t *l = Asc_TypeByModule(m);
	CONSOLE_DEBUG("%lu library entries loaded from %s",gl_length(l),Asc_ModuleName(m));

	CU_ASSERT(gl_length(l)==2);
	gl_destroy(l);

/* CONSOLE_DEBUG("Asc_OpenStringModule returns status=%d",status); */
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

	/* CONSOLE_DEBUG("MODEL TEXT:\n%s",model); */

	struct module_t *m;
	int status;
	
	m = Asc_OpenStringModule(model, &status, ""/* name prefix*/);
	CU_ASSERT_FATAL(status==0); /* if successfully created */

	status = zz_parse();
	CU_ASSERT_FATAL(status==0);

	CU_ASSERT(FindType(AddSymbol("test1"))!=NULL);

	struct Instance *sim = SimsCreateInstance(AddSymbol("test1"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	/* check the simulation name */
	CONSOLE_DEBUG("Got simulation, name = %s",SCP(GetSimulationName(sim)));
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

	CONSOLE_DEBUG("Beginning parse of %s",Asc_ModuleName(m));
	status = zz_parse();

	CONSOLE_DEBUG("zz_parse returns status=%d",status);
	CU_ASSERT(status==0);

	struct gl_list_t *l = Asc_TypeByModule(m);
	CONSOLE_DEBUG("%lu library entries loaded from %s",gl_length(l),Asc_ModuleName(m));
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

	CONSOLE_DEBUG("Beginning parse of %s",Asc_ModuleName(m));
	status = zz_parse();

	CONSOLE_DEBUG("zz_parse returns status=%d",status);
	CU_ASSERT(status==0);

	struct gl_list_t *l = Asc_TypeByModule(m);
	CONSOLE_DEBUG("%lu library entries loaded from %s",gl_length(l),Asc_ModuleName(m));
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

	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	
	/* load the file */
	m = Asc_OpenModule("johnpye/testlog10.a4c",&status);
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

/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T,X) \
	T(init) \
	X T(fund_types) \
	X T(parse_string_module) \
	X T(instantiate_string) \
	X T(parse_basemodel) \
	X T(parse_file) \
	X T(instantiate_file)

/* you shouldn't need to change the following */

#define TESTDECL(TESTFN) {#TESTFN,test_##TESTFN}

#define X ,

static CU_TestInfo basics_test_list[] = {
	TESTS(TESTDECL,X)
	X CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
	{"compiler_basics", NULL, NULL, basics_test_list},
	CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_compiler_basics(void){
	return CU_register_suites(suites);
}
