/*	ASCEND modelling environment
	Copyright (C) 2023 John Pye

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
*/
#include <stdlib.h>
#include <stdio.h>
#include <ascend/utilities/error.h>

#include <test/common.h>
#include "test/assertimpl.h"

//#define TEST_ERROR_DEBUG
#ifdef TEST_ERROR_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(...) 
#endif

FILE *my_error_fp = NULL;

//static error_reporter_callback_t my_error_reporter;

const char *my_sev_str(const error_severity_t sev){
	switch(sev){
		case ASC_PROG_FATAL: return "PFAT";
		case ASC_PROG_ERROR: return "PERR";
		case ASC_PROG_WARNING: return "PWAR";
		case ASC_PROG_NOTE: return "PNOT";
		case ASC_USER_WARNING: return "WARN";
		case ASC_USER_ERROR: return "ERRO";
		case ASC_USER_NOTE: return "NOTE";
		case ASC_USER_SUCCESS: return "SUCC";
		default: return "XXXX";
	}
}

int my_error_reporter(ERROR_REPORTER_CALLBACK_ARGS);

int my_error_reporter(const error_severity_t sev, const char *filename
	, const int line
	, const char *funcname
	, const char *fmt
	, va_list args
){
	int n = 0;
#ifdef TEST_ERROR_DEBUG
	fprintf(stderr,"%s: reporting for %s:%d\n",__func__,filename==NULL?"(null)":filename, line);
#endif
	if(filename==NULL){
		n += fprintf(my_error_fp,"(%s)()(){",my_sev_str(sev));
	}else{
		n += fprintf(my_error_fp,"(%s)(%s:%d)(%s){",my_sev_str(sev),filename,line,funcname);
	}
	n += vfprintf(my_error_fp,fmt,args);
	n += fprintf(my_error_fp,"}");
	return n;
}


static void test_error(void){

	unsigned long prior_meminuse;
	prior_meminuse = ascmeminuse(); /* save meminuse() at start of test function */

	MSG("\nTesting error_reporter routines...");
	// create a rewindable temporary file for testing the error reporter...
#ifdef WIN32
	char tmpl[PATH_MAX];
	snprintf(tmpl,PATH_MAX,"%s\\.asctempXXXXXX",getenv("HOME"));
	fprintf(stderr,"tmpl = %s\n",tmpl);
	int fd = mkstemp(tmpl);
	if(-1==fd){
		perror("mkstemp");
		CU_FAIL("failed mkstemp");
		return;
	}
	FILE *tmp = fdopen(fd,"w+");
	if(tmp == NULL){
    	perror("fdopen");
#else
	FILE *tmp = tmpfile();
	if(tmp == NULL){
    	perror("tmpfile");
#endif
		CU_FAIL("failed to open tmpfile");
	}
	my_error_fp = tmp;
	error_reporter_set_callback(&my_error_reporter);
	const char *myfile=__FILE__;
	char output[4096], gold[4096];
	int x = 0;
	int myline = 0;
#define CHECKIT(STR)\
	fprintf(tmp,"\n");\
	rewind(tmp); \
	{ char *op = \
		fgets(output,4096,tmp); \
		if (op) { \
			MSG("OUTPUT: %s",output);\
			sprintf(gold,STR "\n",myfile,myline); \
			MSG("GOLD  : %s",gold);\
			CU_ASSERT(0==strcmp(output,gold));\
		} else { \
			CU_ASSERT(NULL=="fgets failed on tmpfile");\
		} \
	}\
	rewind(tmp);\
	output[0]='\0';
#define CHECKIT1(STR)\
	fprintf(tmp,"\n");\
	rewind(tmp); \
	{ char *op = \
		fgets(output,4096,tmp); \
		if (op) { \
			MSG("OUTPUT: [%s]",output);\
			sprintf(gold,STR "\n"); \
			MSG("GOLD  : [%s]",gold);\
			CU_ASSERT(0==strcmp(output,gold));\
		} else { \
			CU_ASSERT(NULL=="fgets failed on tmpfile");\
		} \
	} \
	rewind(tmp);\
	output[0]='\0';
	
	// some trivial error messsages:
	x = 5;
	myline=__LINE__;ERROR_REPORTER_HERE(ASC_PROG_NOTE,"nota bene! %d",x);
	CHECKIT("(PNOT)(%s:%d)(test_error){nota bene! 5}");

	x = 6;
	myline=__LINE__;ERROR_REPORTER_HERE(ASC_PROG_WARNING,"attention! %d",x);
	CHECKIT("(PWAR)(%s:%d)(test_error){attention! 6}");

	x = 7;
	myline=__LINE__;ERROR_REPORTER_HERE(ASC_PROG_ERROR,"very bad! %d",x);
	CHECKIT("(PERR)(%s:%d)(test_error){very bad! 7}");

	x = 8;
	myline=__LINE__;ERROR_REPORTER_HERE(ASC_USER_NOTE,"little memo! %d",x);
	CHECKIT("(NOTE)(%s:%d)(test_error){little memo! 8}");

	x = 9;
	myline=__LINE__;ERROR_REPORTER_HERE(ASC_USER_WARNING,"warning! %d",x);
	CHECKIT("(WARN)(%s:%d)(test_error){warning! 9}");

	x = 10;
	myline=__LINE__;ERROR_REPORTER_HERE(ASC_USER_ERROR,"error! %d",x);
	CHECKIT("(ERRO)(%s:%d)(test_error){error! 10}");

	x = 11;
	myline=__LINE__;ERROR_REPORTER_HERE(ASC_USER_SUCCESS,"success, apparently! %d",x);
	CHECKIT("(SUCC)(%s:%d)(test_error){success, apparently! 11}");

	// other macros
	
	int dd = -5;
	myline=__LINE__;ERROR_REPORTER_NOTE("something sth %d",dd); // with substitution arg
	CHECKIT("(PNOT)(%s:%d)(test_error){something sth -5}");

	myline=__LINE__;ERROR_REPORTER_NOTE("something else"); // no extra arg
	CHECKIT("(PNOT)(%s:%d)(test_error){something else}");

	myfile=NULL;
	myline=0;ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"something else"); // no extra arg
	CHECKIT1("(ERRO)()(){something else}");
	
	// test multi-line errors (error_reporter_start, etc)
	myfile="somefile.txt";myline=4;
	error_reporter_start(ASC_USER_WARNING,myfile,myline,"func1");
	FPRINTF(ASCERR,"half of ");
	FPRINTF(ASCERR,"my warning");
	error_reporter_end_flush();
	CHECKIT1("(WARN)(somefile.txt:4)(func1){half of my warning}");

	// test error reporter 'tree'

	//fprintf(stderr,"\n\n");	
	MSG("Testing error reporter 'tree' new/free");
	// test of error_reporter_tree_has_error and error_reporter_tree_clear:
	error_reporter_tree_start();
	error_reporter_tree_clear();


	//fprintf(stderr,"\n\n");	
	MSG("Testing error reporter 'tree'");
	// test of error_reporter_tree_has_error and error_reporter_tree_clear:
	error_reporter_tree_start();
	CU_ASSERT(0 == error_reporter_tree_has_error());
	error_reporter(ASC_PROG_ERROR,"here.txt",456,"nofunc","message2"); // message cached, triggers 'has_error'
	CU_ASSERT(1 == error_reporter_tree_has_error());
	MSG("about to clear");
	error_reporter_tree_clear();
	CU_ASSERT(0 == error_reporter_tree_has_error());
	MSG("here we are");
	ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"afailed"); // no active tree, should just get reported
	CHECKIT1("(ERRO)()(){afailed}");	


	//fprintf(stderr,"\n\n");	
	MSG("Testing reporter 'tree' with multiple outputs");

	// test of error_reporter_tree_has_error and error_reporter_tree_clear, multiple outputs
	error_reporter_tree_start();
	error_reporter(ASC_USER_WARNING,"here.txt",123,"nofunc","message1"); // no trigger
	CU_ASSERT(0 == error_reporter_tree_has_error());
	error_reporter(ASC_PROG_ERROR,"here.txt",456,"nofunc","message2"); // triggers 'has_error'
	CU_ASSERT(1 == error_reporter_tree_has_error());
	error_reporter(ASC_USER_WARNING,"here.txt",789,"nofunc","message3"); // no trigger
	CU_ASSERT(1 == error_reporter_tree_has_error()); // still flagged
	error_reporter_tree_clear();
	ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"bfailed");
	CHECKIT1("(ERRO)()(){bfailed}"); // and other output has been suppressed

	// error_reporter_tree_end()
	error_reporter_tree_start();
	error_reporter(ASC_USER_WARNING,"a.txt",456,"nofunc","message3"); // message will not trigger 'has_error'
#ifdef TEST_ERROR_DEBUG
	error_reporter_tree_dump(stderr);
#endif
	CU_ASSERT(0 == error_reporter_tree_has_error());
	error_reporter_tree_end(); // error messages get output
	CHECKIT1("(WARN)(a.txt:456)(nofunc){message3}");

	//fprintf(stderr,"\n\n");	
	MSG("Testing tree with different types of has_error detection");
#ifdef TEST_ERROR_DEBUG
	error_reporter_tree_dump(stderr);
#endif
	error_reporter_tree_start();
	error_reporter(ASC_USER_WARNING,"a.txt",123,"fn","message1"); // no trigger
#ifdef TEST_ERROR_DEBUG
	error_reporter_tree_dump(stderr);
#endif
	CU_ASSERT(0 == error_reporter_tree_has_error());
	error_reporter(ASC_PROG_ERROR,"a.txt",456,"fn","message2"); // triggers 'has_error'
	CU_ASSERT(1 == error_reporter_tree_has_error());
	error_reporter(ASC_USER_WARNING,"a.txt",789,"fn","message3"); // no trigger
	CU_ASSERT(1 == error_reporter_tree_has_error()); // still flagged
	error_reporter_tree_end();
	ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"dfailed");
	CHECKIT1("(WARN)(a.txt:123)(fn){message1}(PERR)(a.txt:456)(fn){message2}(WARN)(a.txt:789)(fn){message3}(ERRO)()(){dfailed}"); // other output suppressed



	//fprintf(stderr,"\n\n");	
	MSG("Testing with a nested tree_clear");
#ifdef TEST_ERROR_DEBUG
	error_reporter_tree_dump(stderr);
#endif
	error_reporter_tree_start();
	error_reporter_tree_start();
	MSG("inside nested tree...");
#ifdef TEST_ERROR_DEBUG
	error_reporter_tree_dump(stderr);
#endif
	error_reporter(ASC_USER_WARNING,"a.txt",123,"fn","message1"); // no trigger
	error_reporter(ASC_PROG_ERROR,"a.txt",456,"fn","message2"); // triggers 'has_error'
	error_reporter(ASC_USER_WARNING,"a.txt",789,"fn","message3"); // no trigger
	CU_ASSERT(1 == error_reporter_tree_has_error());
	MSG("clearing outer tree");
	error_reporter_tree_clear();
	CU_ASSERT(0 == error_reporter_tree_has_error());
	ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"dfailed");
	CU_ASSERT(1 == error_reporter_tree_has_error());
	error_reporter_tree_end();
	CHECKIT1("(ERRO)()(){dfailed}"); // other output suppressed



	fclose(tmp);

	CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(error)

REGISTER_TESTS_SIMPLE(utilities_error, TESTS)
/* vim: ts=4:noet:sw=4 */

