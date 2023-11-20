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
#include <ascend/compiler/notequery.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/library.h>

#include <test/common.h>
#include <test/assertimpl.h>

#ifdef ASC_WITH_PCRE
# include <pcre.h>
#endif

//#define NOTES_DEBUG
#ifdef NOTES_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
#endif


static void test_init(void){
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
	DEFINITION relation\n\
	    included IS_A boolean;\n\
	    message	IS_A symbol;\n\
	    included := TRUE;\n\
	    message := 'none';\n\
	END relation;\n\
	MODEL test1;\n\
		NOTES\n\
			'author' SELF {notesauthor}\n\
			'description' y {variable called 'y'}\n\
			'description' x {variable called 'x'}\n\
			'description' a['left'] {the left one} (*this note won't parse*)\n\
		END NOTES;\n\
		x \"hello\" IS_A real;\n\
		rel1: x - 1 = 0;\n\
		y[1..5] IS_A real;\n\
		a['left','right'] IS_A boolean;\n\
	END test1;\n\
	MODEL test2;\n\
		x \"yoohoo\" IS_A real;\n\
		x^2 - 4 = 0;\n\
	END test2;\n\
	MODEL test3;\n\
		x \"wahwah\" IS_A real;\n\
		x^2 - 9 = 0;\n\
	END test3;\n";

static void test_test2(void){

	Asc_CompilerInit(1);
	CU_ASSERT(FindType(AddSymbol("boolean"))!=NULL);

	struct module_t *m;
	int status;

	m = Asc_OpenStringModule(model_test2, &status, "mystr"/* name prefix*/);

	MSG("Asc_OpenStringModule returns status=%d",status);
	CU_ASSERT(status==0); /* if successfully created */

	MSG("Beginning parse of %s",Asc_ModuleName(m));
	status = zz_parse();

	MSG("zz_parse returns status=%d",status);
	CU_ASSERT(status==0);

	struct gl_list_t *l, *l2;
	l = Asc_TypeByModule(m);
	MSG("%lu library entries loaded from %s",gl_length(l),Asc_ModuleName(m));
	CU_ASSERT(gl_length(l)==4);
	gl_destroy(l);

	CU_ASSERT(FindType(AddSymbol("test1"))!=NULL);

	l = GetNotes(LibraryNote(),NOTESWILD,NOTESWILD,NOTESWILD,NOTESWILD,nd_wild);
	CU_ASSERT(gl_length(l)==6);
	int i;
	for(i=1;i<=gl_length(l);++i){
		struct Note *N = gl_fetch(l,i);
		l2 = GetExactNote(LibraryNote(),N);
		CU_ASSERT_FATAL(l2 != NULL);
		CU_ASSERT(gl_length(l2)==1);
		CU_ASSERT(gl_fetch(l2,1)==N);
		gl_destroy(l2);
		MSG("%s:%d (#%d) id='%s', type='%s', lang='%s', meth='%s': text='%s'"
			,GetNoteFilename(N),GetNoteLineNum(N),i,SCP(GetNoteId(N))
			,SCP(GetNoteType(N)),SCP(GetNoteLanguage(N)),SCP(GetNoteMethod(N))
			,BCS(GetNoteText(N))
		);
	}
	gl_destroy(l);

	// GetNotes(dbid,typename,language,id,method,notedata);
	l = GetNotes(LibraryNote(),AddSymbol("test1"),AddSymbol("inline"),AddSymbol("x"),NOTESWILD,nd_wild);
	CU_ASSERT(gl_length(l)==1);
	CU_ASSERT(0==strcmp(BCS(GetNoteText(gl_fetch(l,1))),"hello"));
	CU_ASSERT(0==strcmp(SCP(GetNoteId(gl_fetch(l,1))),"x"));
	CU_ASSERT(AddSymbol("inline")==GetNoteLanguage(gl_fetch(l,1)));
	CU_ASSERT(0==strcmp("mystr_global_1<0>",GetNoteFilename(gl_fetch(l,1))));
	CU_ASSERT(15==GetNoteLineNum(gl_fetch(l,1)));
	CU_ASSERT(NULL==GetNoteMethod(gl_fetch(l,1)));
	CU_ASSERT(AddSymbol("test1")==GetNoteType(gl_fetch(l,1)));
	gl_destroy(l);

	// GetNotesAllLanguages
	l = GetNotesAllLanguages(LibraryNote());
#ifdef NOTES_DEBUG
	for(int i=1;i<=gl_length(l);++i){
		MSG("language %d: '%s'",i,SCP(gl_fetch(l,i)));
	}
#endif
	CU_ASSERT(gl_length(l)==3);
	CU_ASSERT(AddSymbol("inline")==gl_fetch(l,1));
	CU_ASSERT(AddSymbol("author")==gl_fetch(l,2));
	CU_ASSERT(AddSymbol("description")==gl_fetch(l,3));
	gl_destroy(l);

	// HoldNoteData, HeldNotes, ReleaseNoteData
	void *h1, *h2;
	// clear all held notes
	ReleaseNoteData(LibraryNote(),(void*)0x1);
	// TODO attempt to store a bogus list
	l2 = gl_create(10);
	char *xx = "nothing";
	gl_append_ptr(l2,xx);
	h1 = HoldNoteData(LibraryNote(),l2);
	CU_ASSERT(h1 == NULL);
	gl_destroy(l2);
	// store some real notes
	l = GetNotes(LibraryNote(),NOTESWILD,AddSymbol("inline"),NOTESWILD,NOTESWILD,nd_wild);
	CU_ASSERT(gl_length(l)==3);
	h1 = HoldNoteData(LibraryNote(),l);
	CU_ASSERT(h1 != NULL);
	// retreive stored notes, check they are as expected
	l2 = HeldNotes(LibraryNote(),h1);
	CU_ASSERT(l2 != NULL);
	CU_ASSERT(l == l2);
	CU_ASSERT(0==strcmp(BCS(GetNoteText(gl_fetch(l2,1))),"wahwah"));
	CU_ASSERT(0==strcmp(BCS(GetNoteText(gl_fetch(l2,2))),"yoohoo"));
	CU_ASSERT(0==strcmp(BCS(GetNoteText(gl_fetch(l2,3))),"hello"));
	CU_ASSERT(3==gl_length(l2));
	ReleaseNoteData(LibraryNote(),h1);
	//gl_destroy(l);

	// release all held lists (shouldn't be any others there anyway
	ReleaseNoteData(LibraryNote(),(void *)0x1);

	l = GetNotes(LibraryNote(),NOTESWILD,AddSymbol("author"),NOTESWILD,NOTESWILD,nd_wild);
	CU_ASSERT(gl_length(l)==1);
	h1 = HoldNoteData(LibraryNote(),l);
	CU_ASSERT(h1 !=NULL);

	l2 = GetNotes(LibraryNote(),NOTESWILD,AddSymbol("description"),NOTESWILD,NOTESWILD,nd_wild);
	CU_ASSERT(gl_length(l2)==2);
	h2 = HoldNoteData(LibraryNote(),l2);
	CU_ASSERT(h2 !=NULL);

	// release all held lists (shouldn't be any others there anyway
	ReleaseNoteData(LibraryNote(),(void *)0x1);

	DestroyNotesOnType(LibraryNote(),AddSymbol("test1"));
	l = GetNotes(LibraryNote(),NOTESWILD,NOTESWILD,NOTESWILD,NOTESWILD,nd_wild);
	CU_ASSERT(gl_length(l)==2);
	gl_destroy(l);
	DestroyNotesOnType(LibraryNote(),AddSymbol("test2"));
	l = GetNotes(LibraryNote(),NOTESWILD,NOTESWILD,NOTESWILD,NOTESWILD,nd_wild);
	CU_ASSERT(gl_length(l)==1);
	gl_destroy(l);

	Asc_CompilerDestroy();
}


static void test_getnoteslist(void){
	Asc_CompilerInit(1);
	int status;
	(void)Asc_OpenStringModule(model_test2, &status, "mystr"/* name prefix*/);
	CU_ASSERT(status==0); /* if successfully created */
	status = zz_parse();
	CU_ASSERT(status==0);
	struct gl_list_t *l, *l2, *l3;

	// inDataListOrNull

	l2 = gl_create(2);
	gl_append_ptr(l2,(void *)AddSymbol("test1"));
	gl_append_ptr(l2,(void *)AddSymbol("test3"));
	l3 = gl_create(1);
	gl_append_ptr(l3,(void *)AddSymbol("x"));
	// GetNotes(dbid,typename,language,id,method,notedata);
	l = GetNotesList(LibraryNote(),l2,NOTESWILDLIST,l3,NOTESWILDLIST,NOTESWILDLIST);
	MSG("got %ld notes",gl_length(l));
	CU_ASSERT(gl_length(l)==3);
	CU_ASSERT(0==strcmp(BCS(GetNoteText(gl_fetch(l,1))),"wahwah"));
	CU_ASSERT(0==strcmp(BCS(GetNoteText(gl_fetch(l,2))),"hello"));
	CU_ASSERT(0==strcmp(BCS(GetNoteText(gl_fetch(l,3))),"variable called 'x'"));
#ifdef NOTES_DEBUG
	for(int i=1;i<=gl_length(l);++i){
		struct Note *N = gl_fetch(l,i);
		MSG("%s:%d (#%d) id='%s', type='%s', lang='%s', meth='%s': text='%s'"
			,GetNoteFilename(N),GetNoteLineNum(N),i,SCP(GetNoteId(N))
			,SCP(GetNoteType(N)),SCP(GetNoteLanguage(N)),SCP(GetNoteMethod(N))
			,BCS(GetNoteText(N))
		);
	}
#endif
	gl_destroy(l2);
	gl_destroy(l3);
	gl_destroy(l);

	// inDataListOrWild

	l2 = gl_create(2);
	gl_append_ptr(l2,(void*)AddSymbol("x"));
	l3 = gl_create(2);
	gl_append_ptr(l3,(void*)nd_empty);
	l = GetNotesList(LibraryNote(),NOTESWILDLIST,NOTESWILDLIST,l2,NOTESWILDLIST,l3);
	MSG("got %ld notes",gl_length(l));
	CU_ASSERT(gl_length(l)==4);
	CU_ASSERT(0==strcmp(BCS(GetNoteText(gl_fetch(l,1))),"wahwah"));
	CU_ASSERT(0==strcmp(BCS(GetNoteText(gl_fetch(l,2))),"yoohoo"));
	CU_ASSERT(0==strcmp(BCS(GetNoteText(gl_fetch(l,3))),"hello"));
	CU_ASSERT(0==strcmp(BCS(GetNoteText(gl_fetch(l,4))),"variable called 'x'"));

	gl_destroy(l2);
	gl_destroy(l3);
	gl_destroy(l);

	Asc_CompilerDestroy();
}

/*-----------------------------
  PATTERN-MATCHING SEARCHES IN NOTES USING PCRE
*/

#ifdef ASC_WITH_PCRE

typedef struct{
	pcre *re;
	pcre_extra *extra;
	int options;
#define OVECSIZE 10
	int ovec[OVECSIZE];
	int ovecsize;
} MyPCREData;

static NEInitFunc my_pcre_init;

static void *my_pcre_init(void *data, char *pattern){
	const char *errmsgptr = NULL;
	int erroffset;

	MyPCREData *mydata = (MyPCREData *)data;
	mydata->options = 0;
	mydata->ovecsize = OVECSIZE;	

	/* TODO use pcre_fullinfo to determine how much spec to allocated to ovec */
	
	pcre *code = pcre_compile(pattern, mydata->options, &errmsgptr, &erroffset, NULL);
	if(code == NULL){
		MSG("input: %s",pattern);
		MSG("       %*c^",erroffset,'-');
		MSG("REGEX ERROR: %s",errmsgptr);
		/* we don't need to free the errmsgptr */
		return NULL;
	}
	errmsgptr = NULL;
	mydata->extra = pcre_study(code, mydata->options, &errmsgptr);
	if(errmsgptr){
		MSG("PCRE_EXTRA ERROR: %s",errmsgptr);
	}
	return code;
}

static NECompareFunc my_pcre_exec;

static int my_pcre_exec(void *data, void *pcre, char *subject, char *start){
	int l = strlen(subject);
	int p = start - subject;
	MyPCREData *mydata = (MyPCREData *)data;
	if(p < 0){
		MSG("Start is before subject");
		return -1;
	}
	if(p >= l){
		MSG("Start is after subject");
		return -1;
	}
	int res = pcre_exec(pcre, mydata->extra, subject, l, p, mydata->options, mydata->ovec, mydata->ovecsize);
	if(res == PCRE_ERROR_NOMATCH)return 0;
	if(res < 0){
		MSG("Error %d returned",res);
		return -1;
	}
	// otherwise, something was matched.
	return 1;
}

#endif

static void test_re(void){
#ifdef ASC_WITH_PCRE
	Asc_CompilerInit(1);
	struct module_t *m;
	int status;
	m = Asc_OpenStringModule(model_test2, &status, "mystr"/* name prefix*/);
	CU_ASSERT(status==0); /* if successfully created */
	status = zz_parse();
	CU_ASSERT(status==0);
	struct gl_list_t *l = Asc_TypeByModule(m);
	CU_ASSERT(gl_length(l)==4);
	gl_destroy(l);

	MyPCREData mydata;

	struct NoteEngine *engine = NotesCreateEngine(&mydata, &my_pcre_init, &my_pcre_exec);

	l = GetMatchingNotes(LibraryNote(), "\\Balle\\B", NULL, engine);

	CU_ASSERT(gl_length(l) == 2);
#ifdef NOTES_DEBUG
	for(int i=1;i<=gl_length(l);++i){
		struct Note *N = gl_fetch(l,i);
		MSG("%s:%d (#%d) id='%s', type='%s', lang='%s', meth='%s': text='%s'"
			,GetNoteFilename(N),GetNoteLineNum(N),i,SCP(GetNoteId(N))
			,SCP(GetNoteType(N)),SCP(GetNoteLanguage(N)),SCP(GetNoteMethod(N))
			,BCS(GetNoteText(N))
		);
	}
#endif
	gl_destroy(l);

	NotesDestroyEngine(engine);

	Asc_CompilerDestroy();
#else
	CU_FAIL("PCRE support not available at compilation time");
#endif
}

#if 0
/*-------------
  notequery.c
*/

static void test_query(void){

	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/canvas/blocktypes.a4c",&status);
	CU_ASSERT(status==0);
	status = zz_parse();
	CU_ASSERT(status==0);

	struct gl_list_t *l = Asc_TypeByModule(m);

	/* there are only 8 things declared in system.a4l: */
	CU_ASSERT(gl_length(l)==4)

	/* but system.a4l also includes basemodel.a4l, which includes... */
	CU_ASSERT(FindType(AddSymbol("cmumodel"))!=NULL);

	Asc_CompilerDestroy();
}	
#endif

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(init) \
	T(test2) \
	T(getnoteslist) \
	T(re) /* \
	T(query) */

REGISTER_TESTS_SIMPLE(compiler_notes, TESTS)
