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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#include <ascend/compiler/childio.h>

#include <ascend/compiler/initialize.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/qlfdid.h>

#include <test/common.h>
#include <test/assertimpl.h>

//#define QLFDID_DEBUG
#ifdef QLFDID_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
#endif

static const char *model = "\n\
	DEFINITION relation\
	    included IS_A boolean;\
	    message	IS_A symbol;\
	    included := TRUE;\
	    message := 'none';\
	END relation;\
	MODEL test1;\n\
		x IS_A real;\n\
		rel1: x - 1 = 0;\n\
		y[1..5] IS_A real;\n\
		a['left','right'] IS_A boolean;\n\
	END test1;";

static void test_string1(void){

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

	CU_ASSERT(FindType(AddSymbol("test1"))!=NULL);

	struct Instance *sim = SimsCreateInstance(AddSymbol("test1"), AddSymbol("SIM1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	// add the simulation to the simlist
	CU_ASSERT_FATAL(Asc_SimsUniqueName(AddSymbol("SIM1")));
	gl_insert_sorted(g_simulation_list,sim,(CmpFunc)Asc_SimsCmpSim);

	/* check the simulation name */
	MSG("Got simulation, name = %s",SCP(GetSimulationName(sim)));
	CU_ASSERT_FATAL(GetSimulationName(sim)==AddSymbol("SIM1"));

	/* check for the expected instances */
	struct Instance *root = GetSimulationRoot(sim);
	CU_ASSERT_FATAL(root != NULL);

	struct Instance *sim1 = Asc_FindSimulationTop(AddSymbol("SIM1"));
	CU_ASSERT_FATAL(sim1 != NULL);
	CU_ASSERT_FATAL(sim1 == sim);

	struct Instance *root1 = Asc_FindSimulationRoot(AddSymbol("SIM1"));
	CU_ASSERT_FATAL(root1 != NULL);
	CU_ASSERT_FATAL(root1 == root);

	extern struct Instance *g_search_inst;
	g_search_inst = NULL;
	g_relative_inst = NULL;

	int res;
	res = Asc_QlfdidSearch3("SIM1.rel1.message", 0);
	CU_ASSERT(res==0)
	CU_ASSERT(g_search_inst != NULL);

	g_relative_inst = root;
	res = Asc_QlfdidSearch3("SIM1.rel1.message", 1);
	CU_ASSERT(res==1)

	g_relative_inst = root;
	res = Asc_QlfdidSearch3("rel1.message", 1);
	CU_ASSERT(res==0)
	CU_ASSERT(g_search_inst != NULL);

	g_relative_inst = NULL;
	CU_ASSERT(1 == Asc_QlfdidSearch3("rel1.message", 0));

	g_relative_inst = NULL;
	CU_ASSERT(0 == Asc_QlfdidSearch3("SIM1", 0));
	CU_ASSERT(1 == Asc_QlfdidSearch3("SIM2", 0));

	g_relative_inst = root;
	CU_ASSERT(0 == Asc_QlfdidSearch3("x", 1));
	CU_ASSERT(0 == Asc_QlfdidSearch3("y", 1));
	CU_ASSERT(1 == Asc_QlfdidSearch3("Y", 1));
	CU_ASSERT(1 == Asc_QlfdidSearch3("y[0]", 1));
	CU_ASSERT(0 == Asc_QlfdidSearch3("y[1]", 1));
	CU_ASSERT(0 == Asc_QlfdidSearch3("y[2]", 1));
	CU_ASSERT(0 == Asc_QlfdidSearch3("y[5]", 1));
	CU_ASSERT(1 == Asc_QlfdidSearch3("y[6]", 1));
	CU_ASSERT(1 == Asc_QlfdidSearch3("x[1]", 1));

	CU_ASSERT(0 == Asc_QlfdidSearch3("a['left']", 1));
	CU_ASSERT(0 == Asc_QlfdidSearch3("a['right']", 1));
	CU_ASSERT(1 == Asc_QlfdidSearch3("a['up']", 1));
	CU_ASSERT(1 == Asc_QlfdidSearch3("a['down.and.right']", 1));


	g_relative_inst = NULL;
	char temp[MAXIMUM_ID_LENGTH];
	struct gl_list_t *l1;

	CU_ASSERT(NULL != (l1 = Asc_BrowQlfdidSearch("SIM1.rel1",temp)));
	if(l1)Asc_SearchListDestroy(l1);
	CU_ASSERT(NULL == (l1 = Asc_BrowQlfdidSearch("SIM1.rel2",temp)));
	if(l1)Asc_SearchListDestroy(l1);
	CU_ASSERT(NULL != (l1 = Asc_BrowQlfdidSearch("SIM1.x",temp)));
	if(l1)Asc_SearchListDestroy(l1);
	CU_ASSERT(NULL != (l1 = Asc_BrowQlfdidSearch("SIM1.y",temp)));
	if(l1)Asc_SearchListDestroy(l1);
	CU_ASSERT(NULL != (l1 = Asc_BrowQlfdidSearch("SIM1.y[5]",temp)));
	if(l1)Asc_SearchListDestroy(l1);
	CU_ASSERT(NULL == (l1 = Asc_BrowQlfdidSearch("SIM1.y[6]",temp)));
	if(l1)Asc_SearchListDestroy(l1);
	CU_ASSERT(NULL == (l1 = Asc_BrowQlfdidSearch("SIM1.a['a.b.c']",temp)));
	if(l1)Asc_SearchListDestroy(l1);
	CU_ASSERT(NULL != (l1 = Asc_BrowQlfdidSearch("SIM1.a['left']",temp)));
	if(l1)Asc_SearchListDestroy(l1);
	CU_ASSERT(NULL == (l1 = Asc_BrowQlfdidSearch("SIM1.a['left'].d",temp)));
	if(l1)Asc_SearchListDestroy(l1);

#if 0
	/* you can't currently search for ['left'] relative to 'SIM1.a', not
	implemented */
	CU_ASSERT(0 == Asc_QlfdidSearch3("a", 1));
	g_relative_inst = g_search_inst;
	CU_ASSERT(0 == Asc_QlfdidSearch3("['left']", 1));
	CU_ASSERT(1 == Asc_QlfdidSearch3("[0]", 1));
	CU_ASSERT(1 == Asc_QlfdidSearch3("['up']", 1));
#endif

	// we don't need to destroy our simulation if we added it to the list
	//MSG("Destroying sim");
	//sim_destroy(sim);
	MSG("Destroying compiler");
	Asc_CompilerDestroy();
}

static void test_string2(void){

	char *s = Asc_MakeInitString(20);
	CU_ASSERT(NULL != s);
	CU_ASSERT
(strlen(s)==0);
	ASC_FREE(s);

	s = Asc_MakeInitString(-1);
	CU_ASSERT(NULL != s);
	CU_ASSERT(strlen(s)==0);

	strcpy(s,"hello");

	CU_ASSERT(0==strcmp(s,"hello"));
	Asc_ReInitString(s);
	CU_ASSERT(0==strcmp(s,""));

	ASC_FREE(s);
}


/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(string1) \
	T(string2)

REGISTER_TESTS_SIMPLE(compiler_qlfdid, TESTS)
