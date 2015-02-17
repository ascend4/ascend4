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

#include <ascend/compiler/initialize.h>

#include <test/common.h>
#include <test/assertimpl.h>

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

#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("Asc_OpenStringModule returns status=%d",status);
#endif
	CU_ASSERT(status==0); /* if successfully created */

#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("Beginning parse of %s",Asc_ModuleName(m));
#endif
	status = zz_parse();

#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("zz_parse returns status=%d",status);
#endif
	CU_ASSERT(status==0);

	struct gl_list_t *l = Asc_TypeByModule(m);
#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("%lu library entries loaded from %s",gl_length(l),Asc_ModuleName(m));
#endif

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
	CU_ASSERT(FindType(AddSymbol("boolean"))!=NULL);
#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("Boolean type found OK");
#endif
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
#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("Got simulation, name = %s",SCP(GetSimulationName(sim)));
#endif
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

#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("Beginning parse of %s",Asc_ModuleName(m));
#endif
	status = zz_parse();

#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("zz_parse returns status=%d",status);
#endif
	CU_ASSERT(status==0);

	struct gl_list_t *l = Asc_TypeByModule(m);
#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("%lu library entries loaded from %s",gl_length(l),Asc_ModuleName(m));
#endif
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

#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("Beginning parse of %s",Asc_ModuleName(m));
#endif
	status = zz_parse();

#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("zz_parse returns status=%d",status);
#endif
	CU_ASSERT(status==0);

	struct gl_list_t *l = Asc_TypeByModule(m);
#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("%lu library entries loaded from %s",gl_length(l),Asc_ModuleName(m));
#endif
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

static void test_initialize(void){
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	
	/* load the file */
#define TESTFILE "testinit"
	m = Asc_OpenModule("test/compiler/" TESTFILE ".a4c",&status);
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
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	
	/* load the file */
#define TESTFILE "stop"
	m = Asc_OpenModule("test/compiler/" TESTFILE ".a4c",&status);
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
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	
	/* load the file */
#define TESTFILE "stoponerror"
	m = Asc_OpenModule("test/compiler/" TESTFILE ".a4c",&status);
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
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	
	/* load the file */
#define TESTFILE "badassign"
	m = Asc_OpenModule("test/compiler/" TESTFILE ".a4c",&status);
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
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	
	/* load the file */
	m = Asc_OpenModule("test/canvas/simple_recycle.a4c",&status);
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
	T(type_info)

REGISTER_TESTS_SIMPLE(compiler_basics, TESTS)

