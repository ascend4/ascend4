/*	ASCEND modelling environment
	Copyright (C) 2018 Carnegie Mellon University

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
#include <ascend/compiler/notate.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/library.h>
#if 0
#include <ascend/compiler/module.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/childio.h>
#include <ascend/compiler/initialize.h>
#endif

#include <test/common.h>
#include <test/assertimpl.h>

#define NOTES_DEBUG
#ifdef NOTES_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
#endif



static void test_test1(void){
	CU_ASSERT(0 == Asc_CompilerInit(0));
	CU_ASSERT(LibraryNote()==AddSymbol("Loaded Libraries"));
	//CU_ASSERT(GlobalNote()==AddSymbol("All Known Files"));

	// test that compilerinit created the two expected notes databases
	struct gl_list_t *l = ListNotesDatabases();
	CU_ASSERT(gl_length(l)==2);
	CU_ASSERT(0!=gl_ptr_search(l,(void *)LibraryNote(),0));
	CU_ASSERT(0!=gl_ptr_search(l,(void *)AddSymbol("All Known Files"),0));
	gl_destroy(l);

	l = GetNotes(LibraryNote(),NOTESWILD,NOTESWILD,NOTESWILD,NOTESWILD,nd_wild);
	CU_ASSERT(0==gl_length(l));
	gl_destroy(l);

	Asc_CompilerDestroy();
}

static const char *model_test2 = "\n\
	DEFINITION relation\
	    included IS_A boolean;\
	    message	IS_A symbol;\
	    included := TRUE;\
	    message := 'none';\
	END relation;\
	MODEL test1;\n\
		NOTES\n\
			'author' SELF {notesauthor}\n\
			'description' y {variable called 'y'}\n\
			'description' a['left'] {the left one}\n\
		END NOTES;\n\
		x \"hello\" IS_A real;\n\
		rel1: x - 1 = 0;\n\
		y[1..5] IS_A real;\n\
		a['left','right'] IS_A boolean;\n\
	END test1;";

static void test_test2(void){

	Asc_CompilerInit(1);
	CU_ASSERT(FindType(AddSymbol("boolean"))!=NULL);

	struct module_t *m;
	int status;

	m = Asc_OpenStringModule(model_test2, &status, ""/* name prefix*/);

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

	l = GetNotes(LibraryNote(),NOTESWILD,NOTESWILD,NOTESWILD,NOTESWILD,nd_wild);
	CU_ASSERT(gl_length(l)>=3);
	int i;
	for(i=1;i<=gl_length(l);++i){
		struct Note *N = gl_fetch(l,i);
		MSG("%d: id='%s', type='%s'",i,SCP(GetNoteId(N)),SCP(GetNoteType(N)));
	}
	gl_destroy(l);

	Asc_CompilerDestroy();
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(test1) \
	T(test2)

REGISTER_TESTS_SIMPLE(compiler_notes, TESTS)
