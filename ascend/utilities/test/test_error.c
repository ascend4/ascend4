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
# define TEST_ERROR_FPRINTF fprintf
#else
# define TEST_ERROR_FPRINTF(...) 
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
	if(filename==NULL){
		n += fprintf(my_error_fp,"(%s)()(){",my_sev_str(sev));
	}else{
		n += fprintf(my_error_fp,"(%s)(%s:%d)(%s){",my_sev_str(sev),filename,line,funcname);
	}
	n += vfprintf(my_error_fp,fmt,args);
	n += fprintf(my_error_fp,"}\n");
	return n;
}


static void test_error(void){

	unsigned long prior_meminuse;
	prior_meminuse = ascmeminuse(); /* save meminuse() at start of test function */

	// create a rewindable temporary file for testing the error reporter...
	FILE *tmp = tmpfile();
	if(tmp == NULL){
		CU_FAIL("failed to open tmpfile");
	}
	my_error_fp = tmp;
	error_reporter_set_callback(&my_error_reporter);
	const char *myfile=__FILE__;
	char output[4096], gold[4096];
	int x = 0;
	int myline = 0;
#define CHECKIT(STR)\
	rewind(tmp); \
	fgets(output,4096,tmp); \
	TEST_ERROR_FPRINTF(stderr,"\nOUTPUT: %s",output);\
	sprintf(gold,STR,myfile,myline); \
	TEST_ERROR_FPRINTF(stderr,"GOLD  : %s",gold);\
	CU_ASSERT(0==strcmp(output,gold));\
	rewind(tmp);
#define CHECKIT1(STR)\
	rewind(tmp); \
	fgets(output,4096,tmp); \
	TEST_ERROR_FPRINTF(stderr,"\nOUTPUT: %s",output);\
	sprintf(gold,STR); \
	TEST_ERROR_FPRINTF(stderr,"GOLD  : %s",gold);\
	CU_ASSERT(0==strcmp(output,gold));\
	rewind(tmp);
	
	// some trivial error messsages:
	x = 5;
	myline=__LINE__;ERROR_REPORTER_HERE(ASC_PROG_NOTE,"nota bene! %d",x);
	CHECKIT("(PNOT)(%s:%d)(test_error){nota bene! 5}\n");

	x = 6;
	myline=__LINE__;ERROR_REPORTER_HERE(ASC_PROG_WARNING,"attention! %d",x);
	CHECKIT("(PWAR)(%s:%d)(test_error){attention! 6}\n");

	x = 7;
	myline=__LINE__;ERROR_REPORTER_HERE(ASC_PROG_ERROR,"very bad! %d",x);
	CHECKIT("(PERR)(%s:%d)(test_error){very bad! 7}\n");

	x = 8;
	myline=__LINE__;ERROR_REPORTER_HERE(ASC_USER_NOTE,"little memo! %d",x);
	CHECKIT("(NOTE)(%s:%d)(test_error){little memo! 8}\n");

	x = 9;
	myline=__LINE__;ERROR_REPORTER_HERE(ASC_USER_WARNING,"warning! %d",x);
	CHECKIT("(WARN)(%s:%d)(test_error){warning! 9}\n");

	x = 10;
	myline=__LINE__;ERROR_REPORTER_HERE(ASC_USER_ERROR,"error! %d",x);
	CHECKIT("(ERRO)(%s:%d)(test_error){error! 10}\n");

	x = 11;
	myline=__LINE__;ERROR_REPORTER_HERE(ASC_USER_SUCCESS,"success, apparently! %d",x);
	CHECKIT("(SUCC)(%s:%d)(test_error){success, apparently! 11}\n");

	// other macros
	
	int dd = -5;
	myline=__LINE__;ERROR_REPORTER_DEBUG("something sth %d",dd); // with substitution arg
	CHECKIT("(PNOT)(%s:%d)(test_error){something sth -5}\n");

	myline=__LINE__;ERROR_REPORTER_DEBUG("something else"); // no extra arg
	CHECKIT("(PNOT)(%s:%d)(test_error){something else}\n");

	myfile=NULL;
	myline=0;ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"something else"); // no extra arg
	CHECKIT1("(ERRO)()(){something else}\n");
	
	// test multi-line errors (error_reporter_start, etc)
	myfile="somefile.txt";myline=4;
	error_reporter_start(ASC_USER_WARNING,myfile,myline,"func1");
	FPRINTF(ASCERR,"half of ");
	FPRINTF(ASCERR,"my warning");
	error_reporter_end_flush();
	CHECKIT1("(WARN)(somefile.txt:4)(func1){half of my warning}\n");

#if 0	
	// test error reporter 'tree'
	error_reporter_tree_start();
	CU_ASSERT(0 == error_reporter_tree_has_error());
	error_reporter(ASC_PROG_ERROR,"here.txt",456,"nofunc","message2");
	CU_ASSERT(1 == error_reporter_tree_has_error());
	error_reporter_tree_clear();
	error_reporter_tree_end();
	ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"failed");
	CHECKIT1("(ERRO)()(){failed}\n");	

	//error_reporter(ASC_USER_ERROR,"here.txt",234,"nofunc","message1");
	//CU_ASSERT(0 == error_reporter_tree_has_error());
	//error_reporter(ASC_PROG_ERROR,"here.txt",456,"nofunc","message2");
	//CU_ASSERT(1 == error_reporter_tree_has_error());
	///error_reporter(ASC_USER_ERROR,"here.txt",123,"nofunc","message3");
	//CU_ASSERT(1 == error_reporter_tree_has_error());
	//error_reporter_tree_end();
	//ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"failed");
	//CHECKIT1("(ERRO)()(){failled}\n");	
#endif

	CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(error)

REGISTER_TESTS_SIMPLE(utilities_error, TESTS)
/* vim: ts=4:noet:sw=4 */

