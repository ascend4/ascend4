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
	Unit test functions for blackbox parsing/loading/evaluating.
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

static struct Instance *load_model(const char *name, int assert_parse_ok, int *parsestatus){
	struct module_t *m;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	/* load the file */
	char path[PATH_MAX];
	strcpy((char *)path,"test/blackbox/");
	strncat(path, name, PATH_MAX - strlen(path));
	strncat(path, ".a4c", PATH_MAX - strlen(path));
	int openmodulestatus;
	m = Asc_OpenModule(path,&openmodulestatus);
	CU_ASSERT(openmodulestatus == 0);

	/* parse it */
	if(assert_parse_ok){
		CU_ASSERT((*parsestatus = zz_parse()) == 0);
	}else{
		*parsestatus = zz_parse();
		if(*parsestatus)return NULL;
	}

	/* instantiate it */
	struct Instance *sim = SimsCreateInstance(AddSymbol(name), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	return sim;
}

static void test_parsefail1(void){
	int parsestatus;
	struct Instance *sim = load_model("parsefail1", FALSE, &parsestatus);
	CU_ASSERT(parsestatus!=0);
	CU_ASSERT(sim==NULL);
	if(sim)sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_parsefail2(void){
	int parsestatus;
	struct Instance *sim = load_model("parsefail2", FALSE, &parsestatus);
	CU_ASSERT(parsestatus!=0);
	CU_ASSERT(sim==NULL);
	Asc_CompilerDestroy();
}

static void test_parsefail3(void){
	int parsestatus;
	struct Instance *sim = load_model("parsefail3", FALSE, &parsestatus);
	CU_ASSERT(parsestatus!=0);
	CU_ASSERT(sim==NULL);
	Asc_CompilerDestroy();
}

static void test_parsefail4(void){
	int parsestatus;
	struct Instance *sim = load_model("parsefail4", FALSE, &parsestatus);
	CU_ASSERT(parsestatus!=0);
	CU_ASSERT(sim==NULL);
	Asc_CompilerDestroy();
}

/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(parsefail1) \
	T(parsefail2) \
	T(parsefail3) \
	T(parsefail4)
	
REGISTER_TESTS_SIMPLE(compiler_blackbox, TESTS)

